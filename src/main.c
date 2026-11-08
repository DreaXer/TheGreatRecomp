/**
 * Rayman 2: The Great Escape (Dreamcast, USA) - Static Recompilation
 *
 * Native port built from the original SH-4 binary translated ahead of time
 * to C. Not an emulator. Uses the dcrecomp framework for CPU state, hardware
 * abstraction and platform. Bring-up layout follows sp00nznet/chuchu.
 *
 * Target: 1ST_READ.BIN, V1.003 (2000-02-22), Katana SDK:
 * Shinobi 1.61, Kamui2 16.3.3.0, MANATEE.DRV sound driver.
 */

#include "recompiler/sh4_cpu.h"
#include "hal/dc_hardware.h"
#include "hal/dc_bios.h"
#include "hal/pvr2.h"
#include "platform/platform.h"
#include "game/game_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DC_SCREEN_WIDTH  640
#define DC_SCREEN_HEIGHT 480

#define GAME_DATA_DIR "extracted"
#define GAME_TITLE    "Rayman 2: The Great Escape"
#define GAME_TRACK3   "disc/Rayman 2 - The Great Escape (USA) (En,Fr,De,Es,It) (Track 3).bin"

/* Standard Katana bootstrap, same as ChuChu Rocket!: set CCR, jump to main.
 *
 *   0x8C01000C  mov.l @(0x8C010024), r0   ; r0 = 0xFF00001C (CCR)
 *   0x8C01000E  mov.l @r0, r1
 *   0x8C010010  mov.l @(0x8C010028), r2   ; r2 = 0x000089AF
 *   0x8C010012  and   r2, r1
 *   0x8C010014  mov.w @(0x8C010020), r2   ; r2 = 0x0800
 *   0x8C010016  or    r2, r1
 *   0x8C010018  mov.l r1, @r0             ; write CCR
 *   0x8C01001A  mov.l @(0x8C01002C), r0   ; r0 = 0x8C0CC540
 *   0x8C01001C  jmp   @r0                 ; -> the real main
 */
#define GAME_ENTRY_FUNC func_8C010000

void sh4_call_indirect(SH4CPU *cpu);
void sh4_jump_indirect(SH4CPU *cpu);

static SH4CPU g_cpu;
static DCHardware *g_hw = NULL;

/* IRQ entry: Shinobi's interrupt dispatcher.
 *
 * On hardware it is reached through VBR+0x600 via a trampoline copied to low
 * RAM, which the recompiler never sees, so it is registered explicitly.
 * 0x8C0DCF40 pushes r14-r10/r8 itself, reads SB_ISTERR & mask (+28) then
 * SB_ISTNRM & mask (+24) from the state block at 0x8C2EF304, and calls the
 * handler registered for each pending bit (bit -> slot table at 0x8C0F4374,
 * slots at 0x8C2EF334, 16 bytes each). Render-done (bit 2) goes to Kamui's
 * ISR at 0x8C0D4676, which ends in the end-of-render callback (0x8C0385C0,
 * registered via 0x8C0DFD00 -> 0x8C0DD580): it clears bit 0 of 0x8C0C5400,
 * which the frame loop at 0x8C0385D8 spins on.
 * 0x8C0DD020 is a second dispatcher with the same masks, not wired yet. */
static void r2_irq_handler(SH4CPU *cpu) {
    func_8C0DCF40(cpu);
}

static int load_game_binary(SH4CPU *cpu, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: Cannot open %s\n", path);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint32_t load_offset = GAME_LOAD_ADDR & cpu->ram_mask;
    if ((uint32_t)(load_offset + size) > cpu->ram_size) {
        fprintf(stderr, "ERROR: Binary too large (%ld bytes)\n", size);
        fclose(f);
        return -1;
    }

    size_t read = fread(cpu->ram + load_offset, 1, size, f);
    fclose(f);

    printf("[BOOT] Loaded %s: %zu bytes at 0x%08X\n", path, read, GAME_LOAD_ADDR);
    return 0;
}

/* BIOS-like state the game expects to find already set up. */
static void dc_bios_init(SH4CPU *cpu) {
    /* The BIOS leaves "SEGA" repeating through this region. */
    uint32_t bios_start = 0x8C00C000 & cpu->ram_mask;
    uint32_t bios_end   = 0x8C00F400 & cpu->ram_mask;
    uint32_t sega = 0x41474553u; /* "SEGA" little-endian */
    for (uint32_t off = bios_start; off < bios_end; off += 4) {
        memcpy(cpu->ram + off, &sega, 4);
    }

    cpu->pc     = GAME_LOAD_ADDR;
    cpu->r[15]  = 0x8C00FC00u;   /* stack pointer */
    cpu->vbr    = 0x8C00F400u;   /* vector base */
    cpu->sr     = 0x700000F0u;   /* supervisor, interrupts masked */
    cpu->fpscr  = 0x00040001u;   /* DN=1, round to nearest */
    cpu->pr     = 0;             /* top of the call chain */
}

int main(int argc, char *argv[]) {
    const char *datadir = argc > 1 ? argv[1] : GAME_DATA_DIR;

    /* Unbuffered: the process is often killed, and the tail matters most. */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("=== %s ===\n", GAME_TITLE);
    printf("Static Recompilation (Sega Dreamcast / SH-4), powered by dcrecomp\n\n");

    sh4_init_ex(&g_cpu, DC_RAM_SIZE_16MB);

    g_hw = dc_hw_init();
    if (!g_hw) {
        fprintf(stderr, "FATAL: Hardware init failed\n");
        sh4_destroy(&g_cpu);
        return 1;
    }

    sh4_set_hardware(g_hw);
    sh4_set_cpu_ref(&g_cpu);

    dc_pvr_init(g_hw);
    dc_maple_init(g_hw);
    dc_aica_init(g_hw);
    dc_gdrom_init(g_hw);

    char path[512];
    snprintf(path, sizeof(path), "%s/1ST_READ.BIN", datadir);
    if (load_game_binary(&g_cpu, path) < 0) {
        printf("\nNo game binary at '%s'. Extract the disc first:\n", path);
        printf("  python tools/extract_gdrom.py \"%s\" extracted\n", GAME_TRACK3);
        goto cleanup;
    }

    if (platform_init(DC_SCREEN_WIDTH, DC_SCREEN_HEIGHT, GAME_TITLE) < 0) {
        fprintf(stderr, "FATAL: Platform init failed\n");
        goto cleanup;
    }

    pvr2_ta_init();
    if (pvr2_render_init(DC_SCREEN_WIDTH, DC_SCREEN_HEIGHT) < 0) {
        fprintf(stderr, "WARNING: PVR2 renderer init failed (running without rendering)\n");
    }

    dc_bios_init(&g_cpu);
    sh4_bios_install_vectors(&g_cpu);

    /* The game asks the drive for FADs (LBA + 150). This disc has a single
     * high-density data track: track 3, LBA 45000 = FAD 45150. */
    sh4_bios_set_gdrom_track(GAME_TRACK3, 45150);

    sh4_set_irq_handler(r2_irq_handler);

    printf("[BOOT] Starting game execution at 0x%08X...\n\n", g_cpu.pc);

    GAME_ENTRY_FUNC(&g_cpu);

    printf("[BOOT] Entry returned, entering main loop\n");

    uint64_t last_time = platform_get_ticks_ms();
    int frames = 0;

    while (g_cpu.running && platform_poll_events(g_hw)) {
        dc_pvr_wait_vblank(g_hw);
        platform_swap_buffers();

        frames++;
        uint64_t now = platform_get_ticks_ms();
        if (now - last_time >= 1000) {
            char title[128];
            snprintf(title, sizeof(title), "%s [%d FPS]", GAME_TITLE, frames);
            platform_set_title(title);
            frames = 0;
            last_time = now;
        }

        uint64_t frame_time = platform_get_ticks_ms() - now;
        if (frame_time < 16) {
            platform_sleep_ms(16 - (uint32_t)frame_time);
        }
    }

cleanup:
    printf("\n[BOOT] Shutting down...\n");
    pvr2_render_destroy();
    pvr2_ta_destroy();
    platform_shutdown();
    dc_hw_destroy(g_hw);
    sh4_destroy(&g_cpu);
    return 0;
}
