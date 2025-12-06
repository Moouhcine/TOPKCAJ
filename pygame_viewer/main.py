import json
import math
import random
import sys
from pathlib import Path
from typing import Dict, List, Tuple

import pygame

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent
CASINO_JSON = PROJECT_ROOT / "Casino.json"
ASSETS_DIR = PROJECT_ROOT / "assets"

BG_COLOR = (11, 111, 32)  # #0B6F20 from LDtk level

# Mapping LDtk identifiers -> asset filenames
SYM_MAP = {
    "Symbole_7": "symbole 7.png",
    "Symbol_diamant": "symbole diamant.png",
    "Symbole_diamant": "symbole diamant.png",  # safety alias
    "Symbole_fraise": "symbole fraise.png",
    "Symbole_cloche": "symbole cloche.png",
}

PLAYER_FILE = "joueur.png"
MACHINE_FILE = "machine.png"

class Entity:
    def __init__(self, kind: str, pos: Tuple[int, int], size: Tuple[int, int], identifier: str):
        self.kind = kind
        self.x, self.y = pos
        self.w, self.h = size
        self.identifier = identifier

class Scene:
    def __init__(self, level_size: Tuple[int, int]):
        self.level_w, self.level_h = level_size
        self.players: List[Entity] = []
        self.machines: List[Entity] = []
        self.symbols: List[Entity] = []

    @staticmethod
    def from_ldtk(path: Path) -> "Scene":
        with path.open("r", encoding="utf-8") as f:
            data = json.load(f)
        level = data["levels"][0]
        scene = Scene((level.get("pxWid", 1920), level.get("pxHei", 1080)))
        for layer in level.get("layerInstances", []):
            ident = layer.get("__identifier")
            ents = layer.get("entityInstances", [])
            for e in ents:
                pos = tuple(e.get("px", [0, 0]))
                size = (e.get("width", 0), e.get("height", 0))
                ent_ident = e.get("__identifier", "")
                ent = Entity(ident, pos, size, ent_ident)
                if ident == "Player":
                    scene.players.append(ent)
                elif ident == "SlotMachine":
                    scene.machines.append(ent)
                elif ident == "Symbole":
                    scene.symbols.append(ent)
        return scene

class SpriteCache:
    def __init__(self, assets_dir: Path):
        self.assets_dir = assets_dir
        self.cache: Dict[str, pygame.Surface] = {}

    def load(self, filename: str) -> pygame.Surface:
        if filename in self.cache:
            return self.cache[filename]
        path = self.assets_dir / filename
        if path.exists():
            surf = pygame.image.load(path.as_posix()).convert_alpha()
        else:
            surf = pygame.Surface((64, 64), pygame.SRCALPHA)
            surf.fill((200, 60, 60, 255))
        self.cache[filename] = surf
        return surf

class MachineSpin:
    def __init__(self, symbol_indices: List[int]):
        self.symbol_indices = symbol_indices  # indices into Scene.symbols
        self.is_spinning = False
        self.spin_end = 0
        self.last_start = 0

    def start(self, now: float):
        self.is_spinning = True
        self.last_start = now
        self.spin_end = now + random.uniform(1.2, 2.0)

    def update(self, now: float):
        if self.is_spinning and now >= self.spin_end:
            self.is_spinning = False


def match_symbols_to_machines(scene: Scene) -> Dict[int, List[int]]:
    # Assign each symbol to nearest machine (distance squared)
    assignments: Dict[int, List[int]] = {i: [] for i in range(len(scene.machines))}
    for idx_sym, sym in enumerate(scene.symbols):
        best_idx = 0
        best_dist = float("inf")
        for idx_machine, mach in enumerate(scene.machines):
            dx = (mach.x - sym.x)
            dy = (mach.y - sym.y)
            d = dx * dx + dy * dy
            if d < best_dist:
                best_dist = d
                best_idx = idx_machine
        assignments[best_idx].append(idx_sym)
    return assignments


def scale_surface(src: pygame.Surface, target_size: Tuple[int, int]) -> pygame.Surface:
    if src.get_width() == 0 or src.get_height() == 0:
        return src
    return pygame.transform.smoothscale(src, target_size)


def main():
    pygame.init()
    if not CASINO_JSON.exists():
        print(f"[error] {CASINO_JSON} introuvable")
        sys.exit(1)

    scene = Scene.from_ldtk(CASINO_JSON)
    screen = pygame.display.set_mode((scene.level_w, scene.level_h))
    pygame.display.set_caption("Casino LDtk Viewer (Pygame)")

    sprites = SpriteCache(ASSETS_DIR)
    machine_sprite = sprites.load(MACHINE_FILE)
    player_sprite = sprites.load(PLAYER_FILE)
    sym_surf_map: Dict[str, pygame.Surface] = {k: sprites.load(v) for k, v in SYM_MAP.items()}

    # Pre-scale machines/players to LDtk entity sizes
    machine_scaled = scale_surface(machine_sprite, (scene.machines[0].w, scene.machines[0].h)) if scene.machines else machine_sprite
    player_scaled = scale_surface(player_sprite, (scene.players[0].w, scene.players[0].h)) if scene.players else player_sprite

    # Symbol surfaces scaled to 16x16 (LDtk size)
    symbol_scaled: Dict[str, pygame.Surface] = {}
    for ident, surf in sym_surf_map.items():
        symbol_scaled[ident] = scale_surface(surf, (16, 16))

    assignments = match_symbols_to_machines(scene)
    machine_spins: Dict[int, MachineSpin] = {}
    for mach_idx, sym_indices in assignments.items():
        machine_spins[mach_idx] = MachineSpin(sym_indices)

    clock = pygame.time.Clock()
    running = True
    next_spin_time = 0.0

    while running:
        now = pygame.time.get_ticks() / 1000.0
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                running = False

        # Trigger spins periodically
        if now >= next_spin_time and machine_spins:
            mach_idx = random.choice(list(machine_spins.keys()))
            machine_spins[mach_idx].start(now)
            next_spin_time = now + random.uniform(0.8, 1.8)

        for spin in machine_spins.values():
            spin.update(now)

        screen.fill(BG_COLOR)

        # Draw machines
        for mach in scene.machines:
            screen.blit(machine_scaled, (mach.x, mach.y))

        # Draw symbols (with shuffle when spinning)
        for mach_idx, sym_indices in assignments.items():
            spin = machine_spins[mach_idx]
            for si in sym_indices:
                sym_ent = scene.symbols[si]
                surf = symbol_scaled.get(sym_ent.identifier, None)
                if surf is None:
                    continue
                dest_y = sym_ent.y
                if spin.is_spinning:
                    # Shuffle effect: jitter + cycle through sprites
                    surf = random.choice(list(symbol_scaled.values()))
                    dest_y += math.sin(now * 20 + si) * 6
                screen.blit(surf, (sym_ent.x, dest_y))

        # Draw players on top
        for i, pl in enumerate(scene.players):
            screen.blit(player_scaled, (pl.x, pl.y))

        pygame.display.flip()
        clock.tick(60)

    pygame.quit()


if __name__ == "__main__":
    main()
