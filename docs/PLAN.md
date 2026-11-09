# Rayman 2: The Great Escape port -- plan

## Standing constraints

- No game data in the repo (dreamcomp docs/LEGAL.md); audit pre-commit hook installed.
- One verified change at a time; enhancements behind settings, on once verified.
- No black bars; high frame rate never by changing game logic rate.

## Phases

| Phase | Entry gate | Exit criteria (visible behaviour) | Status |
|---|---|---|---|
| 00 Survey | disc present | identity, SDK, files recorded | done 2026-11-08 (`findings/phase-00.md`) |
| 01 Boot | config builds | menus reachable; 0 untranslated / 0 unmapped over 1800 frames | in progress: 0 / 0 over 3000 frames with start@600,a@1500,a@2000; title screen complete and the VMU menu drawn over the waterfall (2026-11-09, after the TA buffer fix); Ubi Soft logo frames still black; gameplay reached 2026-11-09: 0 / 0 over 22000 frames (VMU menu, jump pack, main menu, New Game, intro cutscene, Rayman controlled in the prison-ship cell) |
| 02 Audio | 01 | music + effects audible | |
| 03 Full play | 02 | complete playthrough, saves load | |
| 04 Enhancements | 03 | per dc-enhance | |

## Decisions

| Date | Decision | Options | Why | By |
|---|---|---|---|---|
| 2026-11-08 | Build on dreamcomp, not sp00nznet/dcrecomp | dcrecomp (first bring-up), dreamcomp | dcrecomp lacked textures, fog, sound mixing and frame pacing; dreamcomp has Flycast-derived hardware and a playing reference port | user |
| 2026-11-08 | USA disc only | USA, USA + EUR | the USA disc already has French; a second binary doubles config work | Claude, accepted |

## Verified checkpoints

| Date | Port commit | dreamcomp commit | What was confirmed, by whom |
|---|---|---|---|
| 2026-11-09 | d9a001b | 7ce6d63 | Claude, `dc.py shots` up to frame 22000 (`start@600,a@1500,a@2000,a@2600,a@3200,a@3800,start@4400,a@5100,start@6000,start@7000,start@8000,a@9000`, then `a` every 1000 frames from 13500): game created on the VMU, intro cutscene with subtitles, Globox tutorial in the cell, Rayman moving; 0 untranslated, 0 unmapped. Not played by hand yet |
| 2026-11-09 | d9a001b + uncommitted | 7ce6d63 + uncommitted | User, window at 180 Hz with interpolation: 16:9 through the game's own RESOLUTION option and Aspect ratio = Expanded works; after the interpolation fixes (dreamcomp engine-changes 2026-11-09) no more torn characters, lines in water and cliffs, cut trees, VMU-menu HUD jumps or name-entry letter jumps |
| 2026-11-09 | 214d790 + uncommitted | d6dfbb9 + playbook T28 | User, window: ultrawide (window resizing, subtitles, HUD, pause menu, Ubi Soft and Rayman 2 logos of a new game) with the game in its 16:9 mode; Claude, TA at 21:9 and shots at 32:9 (GAME-INTERNALS "Widescreen") |
| 2026-11-09 | 214d790 + uncommitted | d6dfbb9 + playbook T28 | User, window, own VMU: the resolution saved with SAVE OPTIONS comes back at boot, 16:9 and 4:3 (GAME-INTERNALS "Saving") |
