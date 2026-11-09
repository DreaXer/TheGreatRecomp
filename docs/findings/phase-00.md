# Phase 00 -- survey (2026-11-08)

| | |
|---|---|
| Disc | Redump `Rayman 2 - The Great Escape (USA) (En,Fr,De,Es,It)`: tracks 1 (data, LBA 0), 2 (audio, 721), 3 (data, 45000) |
| Identity | 17707N V1.003 (2000-02-22), region U, `dcdisc info`: CRC ok, not Windows CE |
| Boot file | `1ST_READ.BIN`, 935,648 bytes, SHA-1 `0bee95a8c55d3b22f363c6ac35205540b2710c7d`, loaded at 0x8C010000 |
| Bootstrap | standard Katana: 0x8C01000C sets CCR, jumps to main at 0x8C0CC540 |
| SDK | `Shinobi Ver 1.61 Build:Jul 23 1999`, `kamui2 Ver 16,3,3,0 Build:Sep 30 1999` (also `KAMUI-Darkness`); no Ninja strings: OpenSpace's own GLI layer sits on Kamui2 |
| Sound | `MANATEE.DRV` (Sega AICA driver), Ubi's `Moteur sonore ACP - Version M5.8.4` on top |
| Engine | Ubi OpenSpace; ~70 source file names in assert strings (`Actions/Brain.c`, `Engine.c`, ...) |
| Other code files | `2_DP.BIN` (3.6 MB), `SG_DPLDR.BIN`, `PWBROWSER/`: Dream Passport / PlanetWeb browser (inference from names and a `sg_dpldr.bin` reference); not translated |
| Data | `DCDATA/` 87 level folders (`.DAT`, `.TEX`, `.TMP`), sound banks `.OSB`, attract demos `DCDATA/DEMOS/*/DEMO000.SAV` |
| Periph | VGA box, Puru Puru pack, VMU, full pad with analog |
| Earlier work | a first bring-up on sp00nznet/dcrecomp (git history of this repo, `docs/findings/dcrecomp/`): reached the 3D title logo with textures |
