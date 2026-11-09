# TheGreatRecomp - Rayman 2: The Great Escape (Dreamcast), port PC natif

Port PC de Rayman 2 (Dreamcast, version USA `17707N V1.003`, 2000-02-22) construit avec
[dreamcomp](https://github.com/danielgomesvieira2000/dreamcomp) : le code SH-4 de `1ST_READ.BIN`
est traduit en C++ à l'avance et lié à du matériel Dreamcast émulé (PowerVR2 en Vulkan, AICA,
GD-ROM, Maple). dreamcomp est le submodule `dreamcomp/`.

**Aucune donnée du jeu ici.** Il faut ton propre dump Redump (cue + 3 pistes), placé dans
`disc/` (ignoré par git, comme tout ce qui en est extrait).

## Build (Windows)

Prérequis : Visual Studio avec clang-cl, CMake, Ninja, Python 3.10+, SDK Vulkan.

```
git submodule update --init --recursive
python dreamcomp/tools/dc.py doctor        # ce qui manque
python dreamcomp/tools/dc.py deps          # SDL3 dans dreamcomp/.deps/
python dreamcomp/tools/dc.py setup . --disc "disc/Rayman 2 - The Great Escape (USA) (En,Fr,De,Es,It).cue"
python dreamcomp/tools/dc.py build .
python dreamcomp/tools/dc.py run . --window
```

`setup` vérifie le SHA-1 du boot (`game/rayman2.toml`) et n'extrait que `1ST_READ.BIN`.

Sur un Visual Studio en français, le `rc.exe` du SDK échoue (`RC1107`) sur FreeType : configurer
une première fois avec `-DCMAKE_RC_COMPILER=llvm-rc` (voir `docs/PORTING.md`).

Clavier : flèches = croix, Z X A S = A B X Y, Entrée = Start, Q W = gâchettes.

## Documentation

| | |
|---|---|
| `docs/PLAN.md` | phases, décisions, points vérifiés |
| `docs/GAME-INTERNALS.md` | adresses et mécanismes du jeu |
| `docs/PORTING.md` | pièges de toolchain |
| `docs/findings/` | relevés, dont le premier essai sur dcrecomp |

## Licence

GPL-2.0, comme dreamcomp.
