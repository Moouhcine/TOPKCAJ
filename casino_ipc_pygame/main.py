import pygame
import json
import multiprocessing
import time
import os

# --- Configuration Globale ---
SCREEN_WIDTH = 1920
SCREEN_HEIGHT = 1080
FPS = 60
LDYK_FILE = 'casino_layout.ldtk'
ASSET_DIR = './' # Assurez-vous que vos PNG sont ici

# --- Data Structure pour la Mémoire Partagée ---
shared_bank = multiprocessing.Value('i', 1000) # Start with 1000 credits
bank_lock = multiprocessing.Lock()

# --- Fonctions IPC / Simulation de Jeu ---

def player_process(player_id, bank, lock):
    """Represents a single player process interacting with the shared bank."""
    print(f"Player {player_id} started (PID: {multiprocessing.current_process().pid})")
    # ... (Le reste de la fonction player_process reste identique)
    time.sleep(player_id * 0.1)

    while True:
        time.sleep(2) 
        result = time.time() % 2 

        if result < 1.0:
            with lock:
                bank.value += 50
                print(f"Player {player_id} won 50 credits! Bank total: {bank.value}")
        else:
            with lock:
                if bank.value >= 10:
                    bank.value -= 10
                    print(f"Player {player_id} lost 10 credits. Bank total: {bank.value}")
                else:
                    print(f"Player {player_id} wanted to play, but bank is too low.")

# --- Fonctions Graphiques (Pygame + LDtk Loader) ---

def load_ldtk_data(filename):
    """Loads and organizes entity data from the LDtk JSON file."""
    with open(filename, 'r') as f:
        data = json.load(f)
    
    entities = {}
    for toc_entry in data['toc']:
        identifier = toc_entry['identifier']
        # The actual entity instances data is here:
        instances_data = toc_entry.get('instancesData', [])
        if instances_data:
             # Store only relevant instance data: x, y, width, height, identifier
             entities[identifier] = [
                 {'x': inst['worldX'], 'y': inst['worldY'], 'w': inst['widPx'], 'h': inst['heiPx']}
                 for inst in instances_data
             ]
             
    return entities

def draw_scene(screen, entities_data, assets):
    screen.fill((50, 50, 50)) # Grey background

    # Iterate through the organized entities dictionary
    for entity_type, instances in entities_data.items():
        if entity_type in assets:
            sprite_img = assets[entity_type]
            for instance in instances:
                screen.blit(sprite_img, (instance['x'], instance['y']))
        else:
            # Fallback for entities without specific loaded assets (e.g. if symbols aren't loaded)
            for instance in instances:
                pygame.draw.rect(screen, (255, 0, 0), (instance['x'], instance['y'], instance['w'], instance['h']), 1)


def main():
    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("Casino IPC Simulator (LDtk Layout)")
    clock = pygame.time.Clock()

    # 1. Load Assets
    assets = {
        'Player': pygame.image.load(os.path.join(ASSET_DIR, 'player.png')).convert_alpha(),
        'Machine': pygame.image.load(os.path.join(ASSET_DIR, 'slot_machine.png')).convert_alpha(),
        'Symbole_7': pygame.image.load(os.path.join(ASSET_DIR, 'symbole_7.png')).convert_alpha(),
        'Symbol_diamant': pygame.image.load(os.path.join(ASSET_DIR, 'symbole_diamant.png')).convert_alpha(),
    }

    # 2. Load Layout
    entities_data = load_ldtk_data(LDYK_FILE)

    # 3. Start Player Processes (IPC implementation)
    players = []
    num_players = 6
    for i in range(num_players):
        p = multiprocessing.Process(target=player_process, args=(i+1, shared_bank, bank_lock))
        p.start()
        players.append(p)

    # --- Game Loop (Main GUI process) ---
    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        # Drawing the scene from LDtk data
        draw_scene(screen, entities_data, assets)

        # Draw the shared bank balance in the GUI
        font = pygame.font.Font(None, 74)
        with bank_lock:
            bank_text = font.render(f"Bank: {shared_bank.value} Credits", True, (255, 255, 255))
        screen.blit(bank_text, (SCREEN_WIDTH // 2 - bank_text.get_width() // 2, 50))

        pygame.display.flip()
        clock.tick(FPS)

    # Clean up processes when game loop ends
    for p in players:
        p.terminate()
        p.join()
    pygame.quit()

if __name__ == '__main__':
    multiprocessing.freeze_support() 
    main()
