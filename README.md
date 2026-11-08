# TheGreatRecomp - Rayman 2: The Great Escape (Dreamcast), recompilé statiquement

Recompilation statique de Rayman 2 (Dreamcast, version USA `V1.003`, 2000-02-22) :
le code machine SH-4 de `1ST_READ.BIN` est traduit en C à l'avance puis compilé
en binaire natif. Pas un émulateur.

Basé sur [dcrecomp](https://github.com/sp00nznet/dcrecomp) (submodule) et
calqué sur la structure de [chuchu](https://github.com/sp00nznet/chuchu).

## Cible

| | |
| --- | --- |
| Exécutable | `1ST_READ.BIN`, 935 648 octets, chargé à `0x8C010000`, main à `0x8C0CC540` |
| SDK | Katana : Shinobi 1.61, Kamui2 16.3.3.0 (pas de Ninja), `MANATEE.DRV` |
| Moteur | OpenSpace (Ubisoft), couche 3D GLI au-dessus de Kamui2 |
| Hors périmètre | `2_DP.BIN`, `SG_DPLDR.BIN`, `PWBROWSER/` (navigateur en ligne, à confirmer) |

## Build

Il faut ton propre dump. Aucune donnée du jeu n'est distribuée ici.

Place le dump Redump (cue + 3 pistes) dans `disc/`. `disc/` et `extracted/`
sont ignorés par git.

```
git submodule update --init
# Correctif du recompilateur, pas encore en amont (voir patches/).
git -C dcrecomp apply ../patches/dcrecomp-jsr-delay-slot-across-boundary.patch

# 1. Extraire IP.BIN et 1ST_READ.BIN (--all pour tout le disque).
python tools/extract_gdrom.py "disc/Rayman 2 - The Great Escape (USA) (En,Fr,De,Es,It) (Track 3).bin" extracted

# 2. Traduire l'exécutable en C. Écrit src/game/ et include/game/.
python dcrecomp/tools/static_recompile.py extracted/1ST_READ.BIN src/game include/game
python tools/generate_stubs.py

# 3. Build.
cmake -B build -G "Visual Studio 17 2022" -A x64 \
      -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release --parallel 8
```

SDL2 et GLEW viennent de vcpkg. Sans le toolchain file, CMake configure un
build sans fenêtre.

Le jeu lit aussi son système de fichiers sur la piste brute à l'exécution :
`disc/` doit garder la piste 3. Le chemin est dans `src/main.c` (`GAME_TRACK3`).

## État

Squelette seulement. Prochaines étapes :

- lancer `static_recompile.py` et corriger les fonctions manquantes ;
- trouver le dispatcher IRQ/VBlank du jeu (voir le TODO dans `src/main.c`) ;
- identifier le compilateur (SHC Hitachi ou GCC).

## Licence

dcrecomp est sous GPL-2.0, et `tools/generate_stubs.py` en est une copie.
