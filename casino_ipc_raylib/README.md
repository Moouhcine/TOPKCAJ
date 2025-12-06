# Casino IPC Raylib

Mini-jeu démonstrateur d'IPC POSIX (mémoire partagée + mutex/process-shared + message queue) et viewer 2D Raylib 60 FPS. Deux exécutables backend (`casino_server`, `player`) pilotent l'état partagé ; un viewer Raylib lit cet état et affiche une salle de casino stylisée.

## Dépendances
- Ubuntu/Debian, g++ >= 9 (C++17)
- POSIX IPC : `-pthread -lrt`
- Raylib 4.x (installable via `sudo apt install libraylib-dev`)
- Python 3 (pour la génération d'assets avec Pillow optionnel)

## Arborescence
```
casino_ipc_raylib/
  backend/        # serveurs IPC + joueurs
  viewer/         # viewer Raylib 2D
  scripts/        # génération assets + helpers
```

## Compilation
Backend :
```bash
cd backend
make
```
Viewer :
```bash
cd viewer
make   # utilise pkg-config raylib ; fallback -lraylib -lX11 -pthread -lrt -lm -ldl
```

Note : si vous avez compilé Raylib depuis les sources dans /usr/local, ajoutez `export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH` avant d'exécuter `viewer`.

## Génération d'assets
```bash
python3 scripts/gen_assets.py
```
- Si Pillow n'est pas installé : `pip install pillow` recommandé. Le script tombera sinon sur des PNG/PPM simples et le viewer a un fallback primitives si les assets manquent.

## Exécution démo
Depuis la racine du projet :
```bash
bash scripts/run_demo.sh
```
- Lance `casino_server --players 6` en arrière-plan, le viewer Raylib (1280x720 @60 FPS), puis 6 processus `player` qui envoient des mises aléatoires.
- `scripts/clean_ipc.sh` supprime la SHM et la MQ (`/casino_ipc_shared`, `/casino_ipc_mq`).

## Paramètres / CLI
- `casino_server --players N --seed S` : nombre de joueurs (<=16) et graine RNG.
- `player <id>` : id unique 0..15.
- `scene.json` ou `scene.tmj` (Tiled JSON) : positions et paramètres slots/UI. Le viewer charge `scene.json` en priorité, sinon `scene.tmj`, sinon `viewer/layout.txt`.

Slots :
- Chaque spin coûte 20 crédits ; un spin ne démarre qu'une fois toutes les 3s par joueur.
- Résultat gagnant : 3 symboles identiques (gain aléatoire entre 50 et 300). Perte : 3 symboles différents (delta = -20).
- La banque commune (jackpot) est mise à jour à chaque spin (+gain ou -coût) et affichée en UI.

## Notes IPC
- Mémoire partagée POSIX (`shm_open`) contenant l'état du casino + mutex process-shared (`pthread_mutexattr_setpshared`).
- Message queue POSIX (`mq_open`) pour transmettre les mises des joueurs au serveur.
- Le viewer verrouille le mutex brièvement pour copier un snapshot local, garantissant un blocage minimal.
- Assurez-vous que `/dev/mqueue` est monté (sinon : `sudo mount -t mqueue none /dev/mqueue`) pour que `mq_open` fonctionne. En environnement rootless, lancez `scripts/run_demo.sh` en dehors du sandbox si nécessaire.

## Level design externe (Tiled / LDtk)
- Créez votre scène dans un éditeur 2D (Tiled conseillé). Ajoutez une couche d'objets "slots" avec des objets de type `slot` et les propriétés (float) : `slotScale`, `symbolScale`, `playerScale`, `windowW`, `windowH`, `windowOffsetX`, `windowOffsetY`. Placez les objets aux positions désirées.
- Propriétés globales UI au niveau de la map : `panelScale`, `panelOffsetX`, `panelOffsetY`, `barOffsetY`, `logOffsetY`, et/ou valeurs par défaut `slotScale`, `symbolScale`, `playerScale`, `windowW`, `windowH`, `windowOffsetX`, `windowOffsetY`.
- Exportez en JSON (`scene.tmj`) à la racine du projet. Au lancement, le viewer lit `scene.json` puis `scene.tmj` pour appliquer les positions/params. `layout.txt` reste un fallback de compat.

## Tests rapides
- Race condition : lancer plusieurs `player` (scripts/run_demo.sh) et observer la stabilité de `tick`/`rounds` et les transitions d'anim dans le viewer.

## Limitations
- Si Raylib manque, l'édition/compilation du viewer échouera : installer `libraylib-dev` ou compiler Raylib localement puis ajuster `viewer/Makefile` (variable `RAYLIB_FLAGS`).
- Les assets générés sont placeholders HD (gradients/shapes). Le viewer fonctionne sans assets grâce aux fallbacks primitifs.

## Structure backend (OS)
- Mutex process-shared stocké en SHM (PTHREAD_PROCESS_SHARED).
- MQ séparée pour les mises (non bloquante côté serveur).
- Option sémaphore possible mais le mutex suffit ici.
