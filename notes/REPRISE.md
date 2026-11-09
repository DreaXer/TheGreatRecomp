# REPRISE — point d'entrée pour continuer le projet

Écrit le 2026-11-09 (Opus), à la fin de la session qui a migré le port sur dreamcomp et affiché les menus.
**À lire en premier**, puis `dreamcomp/CLAUDE.md` (règles du framework), puis `notes/HANDOFF.md` (historique, pièges).
- Avec Claude Code : dire « Lis notes/REPRISE.md et dreamcomp/CLAUDE.md, puis continue sur le sujet X. »
- Une discussion par sujet (liste en section 4). Le modèle conseillé est indiqué sur chaque sujet.

## 1. Le projet en une minute

Port PC natif de **Rayman 2: The Great Escape** (Dreamcast, USA `17707N V1.003`, 2000-02-22) construit avec
**dreamcomp** : le code SH-4 de `1ST_READ.BIN` est traduit en C++ à l'avance et lié à du matériel Dreamcast émulé
(PowerVR2 en Vulkan, AICA + ARM7, GD-ROM, Maple), dérivé de Flycast. Pas de décompilation.

| Chemin | Contenu |
|---|---|
| `dreamcomp/` | sous-module : fork `DreaXer/dreamcomp`, branche `rayman2` (correctif moteur TA, voir HANDOFF) |
| `game/rayman2.toml` | config du port : SHA-1 du boot, relocations (stubs d'interruption), fonctions en plus |
| `game/CMakeLists.txt`, `src/rayman2.cpp` | déclaration du port (`PortInfo`), hooks futurs |
| `docs/PLAN.md` | phases, décisions, points vérifiés |
| `docs/GAME-INTERNALS.md` | adresses et mécanismes du jeu (interruptions, boucle de frame, display lists, démo) |
| `docs/PORTING.md` | pièges de toolchain |
| `docs/findings/` | relevé du disque (`phase-00.md`), patches du premier essai sur dcrecomp |
| `disc/` | dump Redump de l'utilisateur (cue + 3 pistes). Ignoré par git, ne jamais le modifier |
| `game/extracted/`, `extracted/` | extraits du disque (ignorés). `extracted/` = disque complet, sert à chercher des données |
| `build/game/rayman2-recomp.exe` | l'exécutable |

L'utilisateur fait lui-même les commits et les push (y compris dans le sous-module).

## 2. État au 2026-11-09

**Fonctionne** (captures `dc.py shots`, et testé par l'utilisateur en fenêtre) :
logo RAYMAN 2 en 3D, écran titre complet (ciel, « PRESS START BUTTON »), menu de choix de VMU devant la cascade,
eau, son actif (118 key-ons AICA sur 1800 frames ; pas encore écouté avec attention).
Rapport sur 3000 frames (`start@600,a@1500,a@2000`) : **0 cible non traduite, 0 accès mémoire non mappé**.

Pas encore fait / pas vérifié : le logo Ubi Soft (frames 120 et 300 noires), la partie au-delà du menu VMU,
les sauvegardes, la vitesse en fenêtre (l'utilisateur a vu une démo trop rapide sous dcrecomp, pas sous dreamcomp).

## 3. Lancer, compiler, déboguer

Depuis la racine du dépôt :

```
python dreamcomp/tools/dc.py doctor                      # ce qui manque
python dreamcomp/tools/dc.py build .                     # compile build/game/rayman2-recomp.exe
python dreamcomp/tools/dc.py run . --window              # jouer (Entrée = Start, Z X A S = A B X Y, flèches)
python dreamcomp/tools/dc.py report . --frames 3000 --press start@600 -- --suggest-config game/suggested.toml
python dreamcomp/tools/dc.py shots . --at 500,900,1500 --press start@600   # shots/sheet.png
```

- Après un `report` qui affiche `untranslated call targets: N` : coller `game/suggested.toml` dans `game/rayman2.toml`
  (avec un commentaire : comment et quand), recompiler, relancer jusqu'à 0.
- Essais scriptés (`shots`, `report`) : ajouter `-- --vmu <scratchpad>/vmu_test.bin`, sinon le jeu écrit dans le VMU
  de l'utilisateur (`%APPDATA%\dreamcomp\rayman2\vmu_a1.bin`). Le 2026-11-09 une partie « AAA » y a été créée ainsi.
- Display list d'une frame : `build/game/rayman2-recomp.exe --config game/rayman2.toml --disc "<cue>" --max-frames N
  --press start@600 --dump-ta traces/x.ta --dump-ta-at-frame N`, puis `build/dreamcomp/dreamcomp_ta_dump.exe traces/x.ta`
  (cible à compiler une fois : `dc.py build . --target dreamcomp_ta_dump`).
- Registres PVR : `DREAM_PVR_WATCH=8,14,124,128,144,20` (hex, offsets depuis 0x005F8000) affiche chaque écriture.
- Tout changement sous `dreamcomp/engine/` : une ligne dans `dreamcomp/docs/engine-changes.md` dans le même commit
  (l'audit échoue sinon) ; un piège valable pour tous les jeux : une entrée `dreamcomp/docs/playbook/README.md`.
- Avant un commit : `python dreamcomp/tools/audit.py assets --repo .` (le hook pre-commit le lance aussi).

## 4. Sujets ouverts (une discussion chacun)

| # | Sujet | Comment commencer | Modèle |
|---|---|---|---|
| 1 | **Tests en jeu** : aller du menu VMU à la première partie et au premier niveau ; relever chaque cible non traduite. 2026-11-09 : cellule du bateau atteinte, 0 non traduit ; défauts d'interpolation corrigés et confirmés (playbook T27) ; la suite du niveau reste à jouer | jouer avec `dc.py run . --window --suggest-config game/suggested.toml` (sans `--` : `run` le transmet tel quel et l'exe le refuse), ou scripter avec `--press` ; skill `/dc-playtest` | Opus (diagnostics), Sonnet pour boucler suggest/rebuild |
| 2 | Logo Ubi Soft absent (écran noir avant le titre) | `dc.py shots . --at 60,120,200,300` ; le logo était écrit directement en VRAM (framebuffer) sous dcrecomp | Opus |
| 3 | Vitesse et attente de frame | vérifier le temps réel en fenêtre ; boucle d'attente du jeu à 0x8C0385D8 (voir GAME-INTERNALS) = candidat « idle skip » (playbook T16, `docs/HOOKS.md`) | Opus |
| 4 | Son : écoute attentive, musique de l'écran titre et des menus | `dc.py run . --window` | Sonnet |
| 5 | Sauvegardes VMU | choisir le VMU A1 dans le menu, sauvegarder, relancer | Sonnet |
| 6 | Proposer le correctif TA en amont (dreamcomp / dream-recomp) | seulement si l'utilisateur le demande | Sonnet |
| 7 | Améliorations (widescreen, résolution) | 16:9 fait le 2026-11-09 : `src/rayman2.cpp` déclare un widescreen anamorphique vide plafonné à 16:9 ; le joueur choisit aussi 16/9 dans OPTIONS > VIDEO > RESOLUTION (désormais ADJUSTED). Ultrawide fait (sujet 9). Reste : résolution interne, autres améliorations ; skill `/dc-enhance` | Opus |
| 8 | ~~Bandes noires des cinématiques~~ **fait le 2026-11-09** : deux quads noirs dessinés par-dessus l'image (GAME-INTERNALS « Cutscene letterbox ») ; réglage Graphics « Cutscene bars » (`cinema_bars`) via `PortInfo::graphics_toggles` + `PortInfo::on_frame` (ajoutés à dreamcomp, hors `engine/`). Confirmé en jeu par l'utilisateur. Reste : vérifier les autres cinématiques | — | — |
| 9 | ~~Ultrawide~~ **fait le 2026-11-09**, tout confirmé en fenêtre par l'utilisateur (16:9 à 32:9, formes libres, redimensionnement en direct). Hooks : projection caméra par frame 0x8C037134, dé-projection 0x8C036F00 (logos et objets ancrés à l'écran gardés en 16:9), caméras calculées une seule fois remises à l'échelle quand la fenêtre change ; facteur 2D 0x8C0C52E4 (GAME-INTERNALS « Widescreen », playbook T28). Résolution : celle sauvegardée sur le VMU est réappliquée au chargement, et retenue dans `game_mode` pour le démarrage suivant (« Saving ») ; jamais forcée. Libellé 16/9 renommé dans les 5 langues (« Texts ») ; ligne Language dans le launcher (dreamcomp `--language`). Restes mineurs : coin bas-gauche de l'écran titre découvert en large (décor 4:3 du jeu) ; HUD et caméras orthographiques des autres niveaux non vus ; fonction de SAVE OPTIONS non trouvée | — | — |

## 5. Règles de travail retenues

- Réponses en français. L'utilisateur commite et pousse lui-même.
- Une correction vérifiée à la fois : un correctif est fini quand le symptôme a disparu sur une capture ou un rapport.
- Aucune donnée du jeu dans un commit (disque, extraits, code généré, captures) : `.gitignore` et l'audit le garantissent.
- Les corrections génériques vont dans `dreamcomp/engine/` (fork `DreaXer/dreamcomp`, branche `rayman2`) ;
  celles propres au jeu dans `game/rayman2.toml` ou `src/rayman2.cpp`.
