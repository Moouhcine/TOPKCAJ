# Casino LDtk Viewer (Pygame)

Viewer Pygame qui recrée exactement la scène définie dans `Casino.json` (export LDtk) avec les assets de la racine (`assets/`). Positions, tailles et correspondances symboles/machines/joueurs sont lues depuis le JSON pour rester 1:1 avec ton niveau LDtk. Les symboles ont un effet de shuffle pendant les spins.

## Prérequis
- Python 3
- Pygame (`pip install pygame`)

## Lancer
Depuis `pygame_viewer/` :
```bash
python3 main.py
```
La fenêtre utilise la taille du niveau LDtk (1920x1080 d'après `Casino.json`).

## Comportement
- Fond couleur `#0B6F20` comme dans le level LDtk.
- Les machines, joueurs et symboles sont placés aux coordonnées LDtk (`SlotMachine`, `Player`, `Symbole`). Les tailles proviennent des dimensions d'entité LDtk (machines 256x256, joueurs 200x200, symboles 16x16) avec un scaling automatique selon les PNG.
- Les sprites sont chargés depuis `../assets/` : `machine.png`, `joueur.png`, `symbole 7.png`, `symbole diamant.png`, `symbole cloche.png`, `symbole fraise.png`. Si un sprite manque, un placeholder coloré s'affiche.
- Effet shuffle : chaque machine déclenche périodiquement un spin (aléatoire). Pendant le spin, les trois symboles proches de la machine « défilent » avec un léger décalage vertical avant de se stabiliser sur leur symbole d'origine.

## Notes
- Aucun fichier existant du projet d'origine n'est modifié ; tout est contenu dans `pygame_viewer/`.
- Si tu veux changer les assets, remplace simplement les PNG dans `../assets/`. Si tu modifies `Casino.json`, le viewer s'auto-adapte au nouveau layout.
