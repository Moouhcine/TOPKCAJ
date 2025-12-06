#!/usr/bin/env python3
import os
from pathlib import Path
from random import randint

BASE = Path(__file__).resolve().parent.parent / "viewer" / "assets"

try:
    from PIL import Image, ImageDraw, ImageFilter
    PIL_AVAILABLE = True
except Exception:
    PIL_AVAILABLE = False


def ensure_dir(path: Path):
    path.mkdir(parents=True, exist_ok=True)


def save_ppm(path: Path, w: int, h: int, color):
    ensure_dir(path.parent)
    r, g, b = color
    with open(path, "w", encoding="ascii") as f:
        f.write(f"P3\n{w} {h}\n255\n")
        for _ in range(w * h):
            f.write(f"{r} {g} {b}\n")


def make_gradient(img: Image.Image, top, bottom):
    w, h = img.size
    draw = ImageDraw.Draw(img)
    for y in range(h):
        ratio = y / max(1, h - 1)
        color = tuple(int(top[i] * (1 - ratio) + bottom[i] * ratio) for i in range(3))
        draw.line([(0, y), (w, y)], fill=color)


def make_soft_rect(draw: ImageDraw.ImageDraw, box, fill, outline, radius=16):
    x0, y0, x1, y1 = box
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=3)


def render_pillow_assets():
    assets = {
        "sprites/casino/floor.png": (1280, 720, (10, 140, 120), (8, 90, 80)),
        "sprites/casino/wall.png": (1280, 260, (40, 40, 60), (20, 20, 30)),
        "sprites/casino/table.png": (512, 256, (32, 80, 50), (20, 40, 30)),
        "sprites/casino/slot_machine.png": (256, 384, (180, 40, 40), (80, 10, 10)),
        "sprites/casino/slot_reel_symbols.png": (256, 256, (240, 240, 240), (210, 210, 210)),
        "sprites/ui/panel.png": (512, 128, (30, 50, 70), (10, 20, 40)),
        "sprites/ui/button.png": (256, 96, (50, 180, 230), (20, 100, 160)),
        "sprites/ui/icon_coin.png": (96, 96, (240, 200, 40), (200, 140, 20)),
    }

    for rel, (w, h, top, bot) in assets.items():
        path = BASE / rel
        ensure_dir(path.parent)
        img = Image.new("RGB", (w, h), color=top)
        make_gradient(img, top, bot)
        draw = ImageDraw.Draw(img)
        make_soft_rect(draw, (6, 6, w - 6, h - 6), None, (255, 255, 255), radius=18)
        img = img.filter(ImageFilter.GaussianBlur(radius=0.4))
        if "table" in rel:
            center = (w // 2, h // 2)
            draw.ellipse((40, 40, w - 40, h - 40), fill=(0, 120, 60), outline=(255, 230, 160), width=6)
            draw.ellipse((w//2 - 40, h//2 - 40, w//2 + 40, h//2 + 40), outline=(255, 255, 255), width=3)
        if "slot_machine" in rel:
            draw.rectangle((30, 40, w - 30, h - 60), fill=(230, 230, 230), outline=(255, 255, 255), width=4)
            for i in range(3):
                x0 = 40 + i * 70
                draw.rectangle((x0, 60, x0 + 60, 200), fill=(250, 250, 250), outline=(200, 200, 200))
                draw.text((x0 + 20, 80), "7", fill=(220, 40, 40))
            draw.rectangle((80, h - 100, w - 80, h - 60), fill=(80, 20, 20), outline=(255, 255, 255))
        if "slot_reel_symbols" in rel:
            for i in range(5):
                c = (randint(120, 240), randint(80, 220), randint(80, 220))
                draw.rectangle((10, i * 50 + 10, w - 10, i * 50 + 50), fill=c, outline=(60, 60, 60))
        if "icon_coin" in rel:
            draw.ellipse((10, 10, w - 10, h - 10), fill=(240, 200, 30), outline=(255, 255, 255), width=4)
            draw.line((w//2, 16, w//2, h - 16), fill=(180, 120, 10), width=5)
        img.save(path)

    def make_player_sheet(rel, color1, color2):
        w, h, frames = 128 * 4, 256, 4
        path = BASE / rel
        ensure_dir(path.parent)
        img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        for i in range(frames):
            x = i * 128
            make_soft_rect(draw, (x + 18, 30, x + 110, 210), fill=color1, outline=color2, radius=20)
            draw.ellipse((x + 40, 40, x + 88, 100), fill=color2, outline=(255, 255, 255), width=3)
            swing = (-6, -2, 6, 2)[i % 4]
            draw.rectangle((x + 50 + swing, 100, x + 74 + swing, 190), fill=(30, 30, 30))
            draw.ellipse((x + 38, 14, x + 92, 60), fill=(255, 224, 200), outline=(80, 60, 40))
            draw.ellipse((x + 50, 36, x + 56, 42), fill=(0, 0, 0))
            draw.ellipse((x + 74, 36, x + 80, 42), fill=(0, 0, 0))
        img.save(path)

    make_player_sheet("sprites/players/player_idle.png", (70, 130, 240), (20, 50, 160))
    make_player_sheet("sprites/players/player_walk.png", (60, 200, 120), (20, 80, 40))
    make_player_sheet("sprites/players/player_win.png", (240, 190, 60), (200, 120, 20))


def render_ppm_assets():
    fallback_assets = {
        "sprites/casino/floor.ppm": (640, 360, (30, 120, 100)),
        "sprites/casino/wall.ppm": (640, 160, (30, 30, 50)),
        "sprites/casino/table.ppm": (256, 128, (30, 70, 40)),
        "sprites/ui/panel.ppm": (256, 80, (40, 60, 80)),
    }
    for rel, (w, h, color) in fallback_assets.items():
        path = BASE / rel
        save_ppm(path, w, h, color)


def main():
    ensure_dir(BASE)
    ensure_dir(BASE / "sprites" / "casino")
    ensure_dir(BASE / "sprites" / "players")
    ensure_dir(BASE / "sprites" / "ui")
    ensure_dir(BASE / "sfx")
    ensure_dir(BASE / "fonts")

    if PIL_AVAILABLE:
        print("[gen_assets] Generating PNG assets with Pillow…")
        render_pillow_assets()
        print("[gen_assets] Done (PNG placeholders).")
    else:
        print("[gen_assets] Pillow not available; writing simple PPM placeholders. Install with 'pip install pillow' for better assets.")
        render_ppm_assets()
        print("[gen_assets] Done (PPM fallback).")


if __name__ == "__main__":
    main()
