# Rayman 2: The Great Escape port -- plan

## Standing constraints

- No game data in the repo (dreamcomp docs/LEGAL.md); audit pre-commit hook installed.
- One verified change at a time; enhancements behind settings, on once verified.
- No black bars; high frame rate never by changing game logic rate.

## Phases

| Phase | Entry gate | Exit criteria (visible behaviour) | Status |
|---|---|---|---|
| 00 Survey | disc present | identity, SDK, files recorded | done 2026-11-08 (`findings/phase-00.md`) |
| 01 Boot | config builds | menus reachable; 0 untranslated / 0 unmapped over 1800 frames | in progress: 0 / 0 over 3000 frames with start@600,a@1500,a@2000; title screen complete and the VMU menu drawn over the waterfall (2026-11-09, after the TA buffer fix); Ubi Soft logo frames still black; gameplay not reached |
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
