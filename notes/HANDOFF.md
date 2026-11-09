# Passage de relais — HISTORIQUE détaillé

> **Pour reprendre le projet, lire d'abord `notes/REPRISE.md`** (état actuel, sujets ouverts, commandes).
> Ce fichier garde l'historique : ce qui a été fait, comment, et les pièges. Ajouter une section par session.

## Session du 2026-11-08 / 09 (Opus) — de zéro aux menus, de dcrecomp à dreamcomp

Point de départ : le dump Redump US (cue + 3 pistes) seul dans le dossier.

### Fait
1. **Relevé du disque** : piste 3 = zone haute densité (LBA 45000). `IP.BIN` : `SEGA SEGAKATANA`, région U, V1.003,
   2000-02-22. Boot `1ST_READ.BIN` (935 648 octets) : Shinobi 1.61, Kamui2 16.3.3.0, `MANATEE.DRV`, pas de Ninja
   (moteur OpenSpace avec sa couche GLI sur Kamui2), pas Windows CE. `2_DP.BIN` / `PWBROWSER/` = navigateur en ligne
   (déduit des noms). Détails : `docs/findings/phase-00.md`.
2. **Premier essai sur sp00nznet/dcrecomp** (squelette de chuchu) : extraction, recompilation (6 590 fonctions),
   build MSVC, IRQ Shinobi branché (`0x8C0DCF40`), puis trois correctifs dans dcrecomp :
   - recompilateur : un `jsr` dont le delay slot est aussi une entrée de fonction (0x8C0D4762) était émis sans l'appel
     (le callback de fin de rendu ne tournait jamais) ;
   - rendu : textures PowerVR2 (twiddle, VQ, stride, palettes, mipmaps, shading, blending), absentes de dcrecomp ;
   - framebuffer : format lu sur les bits 2:0 de `FB_R_CTRL` au lieu de 3:2 (logo Ubi Soft doublé, couleurs fausses).
   Patches gardés dans `docs/findings/dcrecomp/`. Résultat : logo 3D texturé, puis blocage dans la démo de fond.
3. **Migration sur dreamcomp** (`danielgomesvieira2000/dreamcomp`, moteur dream-recomp dérivé de Flycast), proposée par
   l'utilisateur (utilisé par les recomps Dreamcast de Quiver Launcher). Dépôt restructuré en port : `game/`, `src/`,
   `docs/`, `LICENSE` GPL-2.0, `.gitignore` « pas de données du jeu », dreamcomp en sous-module. Installés : SDK Vulkan
   (winget), SDL3 (`dc.py deps`), pillow.
4. **Boucle de boot** : 4 relocations (stubs d'interruption copiés sous VBR `0x8C000000`, entrée `0x8C000600`) et
   4 fonctions en plus, toutes trouvées par `--suggest-config`. Résultat : 0 non traduit, 0 non mappé, son actif.
5. **Menus, ciel et eau invisibles** (signalé par l'utilisateur, captures) : le jeu alterne deux tampons de display list
   et lance le rendu de l'un pendant qu'il remplit l'autre, puis fait un `SOFTRESET` du TA. Le moteur n'avait qu'un flux :
   il dessinait le début du tampon en cours (falaises opaques) et le `SOFTRESET` jetait le reste. Correctif moteur
   (`pvr/core.cpp`, `core.h`) : un flux par tampon (`TA_ISP_BASE & 0xF00000`), rendu du flux de `PARAM_BASE`,
   `SOFTRESET` / `TA_LIST_CONT` ne vident plus rien. Consigné dans `engine-changes.md` et le playbook (T26), commité
   sur le fork `DreaXer/dreamcomp`, branche `rayman2` (`7ce6d63`).
6. Commit du port par l'utilisateur : `d9a001b`.

### Pièges rencontrés (à ne pas refaire)
- Le mode auto a refusé d'exécuter du code téléchargé (outils dcrecomp) sans accord explicite de l'utilisateur.
- dcrecomp ne journalise que les 8 premières lectures GD-ROM : « aucun fichier lu » était faux (200+ lectures).
- `assert` d'un self-test désactivés en Release (`NDEBUG`) : le test passait sans rien vérifier (`#undef NDEBUG`).
- CMake 3.31 installé ne connaît pas VS 2026 : utiliser celui de VS ou le Ninja de `dc.py`.
- `RC1107: invalid usage` sur `ftver.rc` (FreeType) : contourné avec `-DCMAKE_RC_COMPILER=llvm-rc` à la première
  configuration (`docs/PORTING.md`). D'après les notes de 40WinksRecomp, la vraie cause est probablement le cmake/ninja
  du toolchain retcomm placé en tête du PATH (non vérifié ici).
- Hypothèse fausse gardée pour mémoire : `TA_LIST_CONT` n'est jamais écrit par Rayman 2 ; c'est `SOFTRESET` qui vidait
  le flux. Toujours tracer les registres (`DREAM_PVR_WATCH`) avant de corriger.
- Le jeton GitHub fine-grained de l'utilisateur ne couvrait que TheGreatRecomp : push vers le fork refusé (403) tant
  que `DreaXer/dreamcomp` n'y était pas ajouté.
- Pointer le sous-module vers le fork avant de commiter le port, et pousser le sous-module en premier.

### À faire
Voir `notes/REPRISE.md`, section 4 (tests en jeu en premier).
