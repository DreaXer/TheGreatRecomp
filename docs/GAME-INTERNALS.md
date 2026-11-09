# Rayman 2 -- game internals

Facts measured on the USA disc (SHA-1 in `game/rayman2.toml`). Found during the dcrecomp
bring-up (2026-11-08); addresses are RAM (P1). Inference is marked as such.

## Interrupts

| Address | What |
|---|---|
| 0x8C0DCF40 | Shinobi interrupt dispatcher: pushes r14-r10/r8 itself, reads SB_ISTERR & mask (+28) then SB_ISTNRM & mask (+24) from the state block at 0x8C2EF304, calls the handler of one pending bit |
| 0x8C0DD020 | second dispatcher, same masks (not needed on dcrecomp) |
| 0x8C0F4374 | bit -> slot table (22 words); slots at 0x8C2EF334, 16 bytes each (fn at +0, arg at +12) |
| 0x8C0D4676 | Kamui render-done ISR (SB_ISTNRM bit 2, slot 20); does nothing unless 0x8C2EE6F8 (render pending) is set; ends by calling 0x8C0D2BA0 |
| 0x8C0EC880 | handler for the TA list-end and CH2-DMA bits (6-11, 19, 21) |

## Frame loop

| Address | What |
|---|---|
| 0x8C0C5400 | render flags; bit 0 = render in flight, bit 1 = second phase |
| 0x8C0386E4 | waits for bit 0 clear, sets bits 0-1, starts the frame |
| 0x8C0385D8 | spins while bit 0 set |
| 0x8C0385C0 | end-of-render callback: clears bit 0; registered via 0x8C0DFD00 -> 0x8C0DD580 (Kamui callback table) |
| 0x8C0625B8 | 6-way state switch on 0x8C2D4968 |

## Display lists (dreamcomp, 2026-11-09)

Two parameter buffers, alternating each frame: object lists at 0x000000 / 0x400000
(`TA_ISP_BASE` 0x00000000 / 0x00400000, `TA_OL_BASE` 0x00090B80 / 0x00490B80). Per frame, from
`DREAM_PVR_WATCH=8,14,124,128,144,20`:

1. `SOFTRESET` 1 then 0, `TA_LIST_INIT` for buffer B, again `SOFTRESET` + `TA_LIST_INIT` (same B)
2. ~2,600 parameter words (opaque scenery first)
3. `PARAM_BASE` = A, `STARTRENDER` (the previous frame's buffer)
4. ~9,900 more words into B (sky, water, translucent, title text, menus)

Never writes `TA_LIST_CONT`. The engine had to keep one display list per buffer (dreamcomp
`docs/engine-changes.md`, playbook T26); before that only the cliffs were drawn.

## Title screen and attract demo

The title screen plays a recorded demo behind the logo (inference: the stream is read about
1.4 s after boot, long before any idle timeout).

| Address | What |
|---|---|
| 0x8C3057C0 | where `DCDATA\DEMOS\PLUM_00\DEMO000.SAV` is loaded (bytes match the file) |
| 0x8C0C59AC | demo stream pointer; 0x8C0C59B4 / 0x8C0C59B0 position / end |
| 0x8C049A44 | per-frame demo interpreter: u16 op, then 1 or 2 bytes (`0x2000`), object table `[[0x8C0C59C4]] + index*0x2C` |
| 0x8C0489C8 | stream reader advancing 0x8C0C59AC; ops 4-19 via a braf table at 0x8C048A38 |

On dcrecomp the interpreter looped on object 20 (`[[[obj]+4]+4] == 0`, so the handler that
advances the stream never ran); under a window the game went on to the demo, too fast.

## Recompiler trap found here

A `jsr @rn` whose delay slot is also a function entry (0x8C0D4762 -> slot 0x8C0D4764) was
emitted without the call on dcrecomp (`docs/findings/dcrecomp/`). Check dream-recomp for the
same case if the render-done callback never runs.
