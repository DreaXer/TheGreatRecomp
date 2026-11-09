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

## Cutscene letterbox (2026-11-09)

The bands are not a smaller viewport: the camera keeps the full 640x480 view and the game draws
two quads over it, last in the opaque list (`dreamcomp_ta_dump` on frame 12500 of PLAN.md's
sequence, intro cutscene in the cell). The user saw them fade in and out over the picture.

| Quad | y | Words |
|---|---|---|
| top | 0 - 19 | `pcw=808c0182` (untextured), `isp=80000001`, `tsp=20800000`, all four vertices `base ff000000`, z = 4 |
| bottom | 367 - 480 | same |

Clip registers stay full screen through the cutscene (`FB_X_CLIP` 0x027F0000, `FB_Y_CLIP`
0x01DF0000, `TA_GLOB_TILE_CLIP` 0x000E0013). Subtitles are drawn in the translucent list at
z = 512 inside the bottom band, so they stay readable without it.

`src/rayman2.cpp` drops them when the Graphics setting **Cutscene bars** (`cinema_bars`) is off:
untextured quads, black (RGB 0, any alpha, for the fade), flat z, x 0 - 640, touching y = 0 or
y = 480, less than 200 high (a full-screen fade is kept). Checked: frame 12500 shows the whole
picture with the subtitle in place; the user confirmed it in play. Not checked: other cutscenes,
and whether a gameplay screen draws a matching quad (none seen in the cell).

## Widescreen (2026-11-09)

OPTIONS > VIDEO > RESOLUTION toggles the game's 16:9 mode (anamorphic, drawn into 640x480).
Found by diffing RAM at frame 7500 with the mode toggled once and twice (same menu screen).
The menu shows the mode you would switch *to*, and SAVE OPTIONS keeps it on the VMU.

| Address | 4:3 | 16:9 | What |
|---|---|---|---|
| 0x8C0C52DC (byte) | 0 | 1 | mode flag; set by fn_0c03703a, read by ~15 functions (2D and camera) |
| 0x8C0C52E0 | 1.0 | 1.2 | horizontal FOV factor |
| 0x8C0C52E4 | 1.0 | 0.8333 | 2D width factor: 2D is drawn at this width, centred (`(100 - 100*f)/2` offset in fn_0c01aea0) |

Saving (2026-11-09, scripted runs on VMU copies): OPTIONS > SAVE OPTIONS ("SAVED SUCCESSFULLY")
rewrites `RAYMAN_2.SY_` (2 blocks, first 197; contents scrambled: saving 4:3 instead of 16:9
changes bytes 0x18880, 0x188A5 and a 2-byte check at 0x18A46). At boot the file is read and
decoded: the only RAM word that differs between a 16:9 and a 4:3 save is 0x8C2D0F4C (1 / 0).
Nothing copies it to the mode: the flag is written only by the boot reset fn_0c04c3c0 (mode 0,
volumes 127, frame 307) and by the menu through `fn_0c04c31e(mode)` (stores 0x8C2D0F44, calls
fn_0c03703a); not by loading the save either (flag still 0 after LOAD). Whether the Dreamcast
does the same is not checked. The loader is fn_0c04c61c (reads each field through
`fn_0c06777e(dest, ...)`, kSavedMode from 0x8C04C73C, then sets the volumes); an exit hook on
it never ran (not investigated).

Port fix (`src/rayman2.cpp`, user's choice: follow the player's save, never force 16:9):
entry+exit hook on 0x8C06777E; right after the loader's kSavedMode field (r4 = 0x8C2D0F4C,
pr = 0x0C04C740), a 1 sets 0x8C2D0F44 = 1 and the flag = 1 as the menu does, and the cameras
follow through fn_0c037134's mode check. Checked: boot with a 16:9 save -> flag 1, E0 1.2,
E4 0.8333, VIDEO menu at 21:9 unstretched and showing "MODE 4/3" (the mode to switch to); with
a 4:3 save -> flag 0, "MODE 16/9"; 0 untranslated / 0 unmapped over 9500 frames with LOAD;
user, window, own VMU: both directions (save, restart) restored. fn_0c014460 (level load) forces 4:3 in level `Raycap` and puts
16:9 back after it (0x8C0C42A5 remembers).

Camera, `fn_0c079ea0(cam, fov, ratio)`: `cam+120 = factor * fov` (horizontal FOV, full angle,
radians), `cam+124 = fov * ratio` (vertical), `cam+236 = ratio`. The gameplay camera calls it
from 0x8C0586F6 when its FOV differs from the stored one, which in 16:9 mode (factor != 1) is
every game frame. From cam+120/124 the game derives each frame `cam+136 = 0.1 / tan(h/2)`, the
side/top clip planes (cam+172..) and culling. Measured on the video menu's waterfall
(`dreamcomp_ta_dump`, polygons matched by tsp/tcw/z): 16:9 vs 4:3 moves x by 0.78 =
tan(0.6)/tan(0.72) and y by 0.884 = tan(0.45)/tan(0.5): the game's 16:9 also widens the vertical
view (ratio 0.75 -> 0.8333, inference: the caller derives it from the mode).

Per frame, `fn_0c037134(?, viewport, cam)` (r6 = cam) derives every camera's projection, clip
planes and culling from cam+120/124: types 102/103 perspective, 100/101 orthographic
(`cam+140 = -1 / (viewport_w * cam+120)`). It first calls fn_0c03703a when the camera's stored
mode differs from the flag.

It is called from the camera update (fn_0c058646, game logic), not from drawing, so the derived
fields stay as computed until the next frame. Perspective derived fields: `cam+136 = 0.1 /
tan(h/2)`, `cam+140 = -3200` (constant), `cam+144 = -3200 * tan(h/2) / tan(v/2)` (measured on
the menu camera, 4:3 vs 16:9 mode). Orthographic: `cam+140 = -c / (viewport_w * cam+120)`;
type 101 copies cam+120 into cam+124 first.

`fn_0c036f00(obj, out, sx, sy, depth)` turns a screen point into camera space with the camera
`[[obj+96]]`: it divides by cam+140/144 and scales by cam+136. The intro's Ubi Soft logo
(textured quads at the near plane, 1/w ~ 4.03, x 84..556 at frame 8900 of the load-AAA
sequence) is placed with it: given widened fields it came closer (1/w 4.069 at 32:9) and grew
by k in both directions.

Wider than 16:9 (`src/rayman2.cpp`), `k = aspect / (16/9)`, while the flag is set:
- entry+exit hook on 0x8C037134: perspective cam+120 widened to `2 atan(tan(h/2) * k)` for the
  call, the game's value put back at exit unless the call rewrote it; orthographic cam+140
  divided by k after the call (x only);
- entry+exit hook on 0x8C036F00: cam+136 * k and cam+144 / k (the 16:9 values) for the call, so
  screen-anchored objects keep their 16:9 size, centred, like the HUD (user's choice);
- each vblank: `0x8C0C52E4 = 0.8333 / k`.

Earlier versions, as found: hooking the FOV setter fn_0c079ea0 left menu and logo cameras whose
FOV is set once stretched (pause-menu lum, logos; user, window); widening cameras only made the
logos fill the wider window and run off its edges (user, window), fixed by the unproject hook.

Checked: 16:9 TA identical to the unhooked build; 21:9 x 0.7619 (= 1/1.3125), y 1.000; cell at
frame 22500 (21:9, 32:9): Globox the same size and distance from centre, subtitle box the same
width and centred, polygons on both edges (no cull cliff); pause menu at 32:9: lum round, masks
and text in proportion; Ubi Soft logo at 21:9 (TA, frame 8900): same 1/w and y as at 16:9,
x 140..500 = 320 + (x16 - 320) / 1.3125, rest of the scene x 0.7619 / y 1.000; at 32:9 (shot):
centred, text ~505x55 px as at 16:9; 0 untranslated / 0 unmapped over 23000 frames; window
resizing (user, first version); Ubi Soft and Rayman 2 logos of a new game (user, window,
final version). Not checked: what else uses orthographic cameras, other levels' HUD.

## Texts (2026-11-09)

`DCDATA/TEXTS/<LANGUAGE>.LNG` (ENGLISH, FRENCH, GERMAN, ITALIAN, SPANISH): a table of absolute
pointers, then NUL-terminated strings; loaded at 0x8CCCBB00 (every language's first pointer is
0x8CCCC8DC, already resolved for that base; English label found at base + 0x5F28 in RAM). The
font shows `;` as `/` ("16;9"). Format codes: `/L:` new line, `/C:` centred, `/oN:` offset. French
strings use no byte >= 0x80; the others do (umlauts, accented vowels).

The resolution label and the "restore 16:9 mode?" question (none in Spanish) are renamed by
`src/rayman2.cpp` each vblank (user's request: the mode follows the window): EN `ADJUSTED`,
FR `AJUSTE`, DE `Angepasst`, IT `Adattata`, ES `AJUSTADO`, each fitting the original length;
strings are matched by length + FNV-1a so no game text is in the source. The TV message
"PLEASE SWITCH YOUR TELEVISION SET TO" reuses the label. Checked in the VIDEO menu and the TV
message (shots), all five languages: the language comes from the console flash, so a scratch
copy of the synthesised flash (`--dump-flash`) had its language set (factory digits at
0x1A003/0x1A0A3, syscfg block 5 byte 5 with its CRC) and was passed with `--flash`. The font
draws every letter upper case. Not checked: the "restore" question (where the game asks it).

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
