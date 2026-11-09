// Rayman 2: The Great Escape: what this port tells dreamcomp. Game facts with sources:
// docs/GAME-INTERNALS.md.
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>

#include "dream/render/display_list.h"
#include "dream/runtime/system.h"
#include "dreamcomp/hook.h"
#include "dreamcomp/port.h"
#include "dreamcomp/settings.h"

namespace {

// Cutscene letterbox (docs/GAME-INTERNALS.md): two untextured black quads drawn over the full
// view, 640 wide, one from y = 0 and one down to y = 480 (opaque, translucent while fading).
bool letterbox_bar(const dream::render::Polygon& p, const std::vector<dream::render::Vertex>& vs) {
    if (p.count != 4 || (p.pcw & 0x8u) || p.first + 4 > vs.size())
        return false;
    float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
    for (unsigned i = 0; i < 4; ++i) {
        const auto& v = vs[p.first + i];
        if ((v.base & 0x00FFFFFFu) != 0 || v.z != vs[p.first].z)
            return false;
        x0 = std::min(x0, v.x), x1 = std::max(x1, v.x);
        y0 = std::min(y0, v.y), y1 = std::max(y1, v.y);
    }
    const bool full_width = std::fabs(x0) < 0.5f && std::fabs(x1 - 640.0f) < 0.5f;
    const bool at_edge = std::fabs(y0) < 0.5f || std::fabs(y1 - 480.0f) < 0.5f;
    return full_width && at_edge && y1 - y0 < 200.0f;  // a full-screen fade is left alone
}

void on_frame(dream::render::Frame& frame, dreamcomp::Settings& settings) {
    if (settings.get_bool("cinema_bars", true))
        return;
    for (auto& list : frame.lists)
        list.erase(std::remove_if(list.begin(), list.end(),
                                  [&](const auto& p) { return letterbox_bar(p, frame.vertices); }),
                   list.end());
}

const dreamcomp::PortInfo::Toggle kToggles[] = {
    {"cinema_bars", "Cutscene bars",
     "The black bands the game draws over the top and bottom of the picture during cutscenes."
     "<br/><br/><b>Off</b> shows the whole picture; subtitles stay where they are.",
     true},
};

// The game has its own 16:9 mode (OPTIONS > VIDEO > RESOLUTION), drawn squeezed into 640x480;
// the engine unsqueezes it. Wider shapes start from that mode (docs/GAME-INTERNALS.md
// "Widescreen"): the camera's horizontal FOV is widened so the vertical view stays the game's
// 16:9 one, and the game's own 2D squeeze factor is narrowed to keep the HUD's proportions.
// The player picks 16:9 in the game's menu once and saves it (SAVE OPTIONS); the save is
// applied at boot below. With 4:3 picked, an Expanded window stretches the 4:3 view: by
// choice, the port never forces the mode against the player's save.
constexpr std::uint32_t kWideFlag = 0x8C0C52DC;  // byte, 1 = the game's 16:9 mode
constexpr std::uint32_t kHudScale = 0x8C0C52E4;  // 2D width factor, 1/1.2 in 16:9 mode
constexpr float kGameWide = 16.0f / 9.0f;
float g_fov_widen = 1.0f;  // tan(h/2) factor over the game's 16:9 view; 1 = untouched

bool game_wide(dream::Memory& m) { return m.read8(kWideFlag) != 0; }

// SAVE OPTIONS stores the resolution in RAYMAN_2.SY_ and fn_0c04c61c decodes it into
// kSavedMode at boot, applying the volumes but not the mode (docs/GAME-INTERNALS.md "Saving").
constexpr std::uint32_t kSavedMode = 0x8C2D0F4C;  // 1 = 16:9 saved
constexpr std::uint32_t kMode = 0x8C2D0F44;       // the menu's current setting

// The widening each perspective camera was last derived with, and the cam+136 it then got. Some
// cameras are derived only when created (the intro logo's, drawn and placed with it), so when
// the window changes shape their fields keep the old widening: rescale them to the new one
// (below), and unproject undoes the one they hold. An entry whose cam+136 no longer matches was
// re-derived, changed or freed: it is dropped, never written.
struct CamWiden {
    float k, f136;
};
std::unordered_map<std::uint32_t, CamWiden> g_cam_widen;

float effective_widen(dream::Memory& m) { return game_wide(m) ? g_fov_widen : 1.0f; }

void rescale_stale_cameras(dream::Memory& m) {
    const float k = effective_widen(m);
    for (auto it = g_cam_widen.begin(); it != g_cam_widen.end();) {
        const std::uint32_t cam = it->first;
        CamWiden& w = it->second;
        if (dreamcomp::read_f32(m, cam + 136) != w.f136) {
            it = g_cam_widen.erase(it);
            continue;
        }
        if (w.k != k) {  // cam+136 = 0.1 / tan(h/2), cam+144 = -3200 tan(h/2) / tan(v/2)
            w.f136 *= w.k / k;
            dreamcomp::write_f32(m, cam + 136, w.f136);
            dreamcomp::write_f32(m, cam + 144, dreamcomp::read_f32(m, cam + 144) * k / w.k);
            w.k = k;
        }
        ++it;
    }
}

void apply_widescreen(dream::System& sys, float aspect) {
    g_fov_widen = aspect > 4.0f / 3.0f + 0.01f ? aspect / kGameWide : 1.0f;
    if (game_wide(sys.memory))
        dreamcomp::write_f32(sys.memory, kHudScale, (1.0f / 1.2f) / g_fov_widen);
    rescale_stale_cameras(sys.memory);
}

}  // namespace

// fn_0c037134(?, viewport, cam) derives each camera's projection, clip planes and culling from
// cam+120 (horizontal) and cam+124 (vertical); most cameras every frame. Perspective types
// 102/103: cam+120 is the FOV (radians, full angle), widened for the call only, so cameras
// whose FOV is set once (pause menu) widen too and nothing compounds. Orthographic types
// 100/101: type 101 copies cam+120 into cam+124, so only their x scale cam+140 is narrowed,
// after the call.
namespace {
std::uint32_t g_cam = 0;  // camera the entry hook saw, 0 = none (no recursion seen)
std::uint32_t g_cam_type = 0;
float g_cam_h = 0.0f, g_cam_wide = 0.0f, g_cam_k = 1.0f;
}  // namespace

DC_HOOK_ENTRY(project_camera) {
    g_cam = c.r[6];
    g_cam_type = m.read32(g_cam + 8);
    g_cam_k = effective_widen(m);
    g_cam_h = dreamcomp::read_f32(m, g_cam + 120);
    g_cam_wide = g_cam_h;
    const bool persp = (g_cam_type == 102 || g_cam_type == 103) && g_cam_h > 0.0f && g_cam_h < 3.0f;
    if (persp && g_cam_k != 1.0f) {
        g_cam_wide = 2.0f * std::atan(std::tan(g_cam_h * 0.5f) * g_cam_k);
        dreamcomp::write_f32(m, g_cam + 120, g_cam_wide);
    }
    return false;
}

DC_HOOK_EXIT(project_camera) {
    if (!g_cam)
        return;
    if (g_cam_type == 100 || g_cam_type == 101) {
        if (g_cam_k != 1.0f)
            dreamcomp::write_f32(m, g_cam + 140, dreamcomp::read_f32(m, g_cam + 140) / g_cam_k);
    } else if (g_cam_type == 102 || g_cam_type == 103) {
        if (dreamcomp::read_f32(m, g_cam + 120) == g_cam_wide) {  // unless the call re-derived it
            dreamcomp::write_f32(m, g_cam + 120, g_cam_h);
            g_cam_widen[g_cam] = {g_cam_wide != g_cam_h ? g_cam_k : 1.0f,
                                  dreamcomp::read_f32(m, g_cam + 136)};
        } else {
            g_cam_widen.erase(g_cam);
        }
    }
    g_cam = 0;
}

// fn_0c06777e(dest, ...) reads one field of the file; the loader calls it for kSavedMode from
// 0x8C04C73C (pr 0x0C04C740). The loader's own exit is never reached by an exit hook (not
// investigated), so the mode is set right after that field. Set it as the menu's fn_0c04c31e
// does; each camera then takes the mode through fn_0c037134's own check (camera mode != flag
// -> fn_0c03703a), as after a menu toggle. Applied both ways: the boot may already have set
// the remembered mode (below).
//
// The VMU menu (and so the load) comes ~25 s after boot, and the screens before it would be
// stretched in a wide window. So the port remembers the mode in its settings (`game_mode` =
// adjusted / 4:3) when a save is loaded and when the player switches it in the VIDEO menu
// (fn_0c04c31e called from the menu, 0x0C030640; not the Raycap level's forced 4:3), and sets
// it right after the game's boot reset fn_0c04c3c0 (frame 307). SAVE OPTIONS itself was not
// found (it does not go through fn_0c06777e).
namespace {
bool g_reading_mode = false;
int g_remember = -1;      // mode to store in the settings at the next vblank, -1 = none
bool g_boot_reset = false, g_boot_done = false;

void set_game_mode(dream::Memory& m, bool wide) {
    m.write32(kMode, wide ? 1 : 0);
    m.write8(kWideFlag, wide ? 1 : 0);
}
}  // namespace

DC_HOOK_ENTRY(read_option_field) {
    g_reading_mode = c.r[4] == kSavedMode && c.pr == 0x0C04C740;
    return false;
}

DC_HOOK_EXIT(read_option_field) {
    if (!g_reading_mode)
        return;
    g_reading_mode = false;
    const bool wide = m.read32(kSavedMode) == 1;
    if (wide != game_wide(m))
        set_game_mode(m, wide);
    g_remember = wide ? 1 : 0;
}

DC_HOOK_ENTRY(set_resolution) {  // fn_0c04c31e(mode)
    if (c.pr == 0x0C030644)      // the VIDEO menu
        g_remember = c.r[4] == 1 ? 1 : 0;
    return false;
}

DC_HOOK_ENTRY(boot_reset) {  // fn_0c04c3c0: volumes 127, mode 4:3
    g_boot_reset = true;
    return false;
}

// fn_0c036f00(obj, out, sx, sy, depth) turns a screen point into camera space with the current
// camera [[obj+96]]: perspective uses cam+136 (0.1 / tan(h/2), widened: / k) and cam+144 (y
// scale, -3200 * tan(h/2) / tan(v/2), widened: * k). Screen-anchored objects (the intro logos)
// are placed with it, so with the widened values they came closer and grew by k. Give it the
// game's 16:9 values for the call: they keep their 16:9 size, centred, like the HUD.
namespace {
std::uint32_t g_unproj_cam = 0;
float g_unproj_136 = 0.0f, g_unproj_144 = 0.0f;
}  // namespace

DC_HOOK_ENTRY(unproject) {
    g_unproj_cam = 0;
    const std::uint32_t cam = m.read32(m.read32(c.r[4] + 96));
    const std::uint32_t type = m.read32(cam + 8);
    const auto it = g_cam_widen.find(cam);
    if ((type != 102 && type != 103) || it == g_cam_widen.end() || it->second.k == 1.0f ||
        dreamcomp::read_f32(m, cam + 136) != it->second.f136)
        return false;
    const float k = it->second.k;
    g_unproj_cam = cam;
    g_unproj_136 = dreamcomp::read_f32(m, cam + 136);
    g_unproj_144 = dreamcomp::read_f32(m, cam + 144);
    dreamcomp::write_f32(m, cam + 136, g_unproj_136 * k);
    dreamcomp::write_f32(m, cam + 144, g_unproj_144 / k);
    return false;
}

DC_HOOK_EXIT(unproject) {
    if (!g_unproj_cam)
        return;
    dreamcomp::write_f32(m, g_unproj_cam + 136, g_unproj_136);
    dreamcomp::write_f32(m, g_unproj_cam + 144, g_unproj_144);
    g_unproj_cam = 0;
}

namespace {

// The game's "16:9" resolution label (and the "restore 16:9 mode?" question) renamed, since the
// mode now follows the window's shape. Texts come from DCDATA/TEXTS/<LANGUAGE>.LNG, loaded at
// kTexts with its pointers already resolved (docs/GAME-INTERNALS.md "Texts"). Each string is
// recognised by length + FNV-1a of the original bytes (no game text in this source), edited
// in place by splices on the original (applied from the end), shorter or equal, NUL-padded.
// Once edited its hash no longer matches, so the check each vblank is a few reads.
constexpr std::uint32_t kTexts = 0x8CCCBB00;

struct Splice {
    unsigned pos, del;
    const char* ins;
};
struct TextEdit {
    std::uint32_t offset;
    unsigned len;
    std::uint32_t fnv;
    Splice splices[2];
};
// Generated from the five .LNG files; French texts carry no accents, the new words need none.
const TextEdit kTextEdits[] = {
    {0x05F28, 9, 0x52FC1DDDu, {{0, 9, "ADJUSTED"}}},                    // english label
    {0x06A9A, 87, 0x4D4584BEu, {{35, 9, "ADJUSTED"}}},                  // english question
    {0x06320, 12, 0xC7777D6Eu, {{0, 12, "AJUSTE"}}},                    // french label
    {0x06F8C, 90, 0x1C82713Bu, {{39, 7, "AJUSTE"}}},                    // french question
    {0x06166, 10, 0xAE4DB9DBu, {{0, 10, "Angepasst"}}},                 // german label
    {0x06D01, 96, 0x911D1605u, {{7, 20, "Angepassten"}}},               // german question
    {0x05ECD, 13, 0x67673A30u, {{0, 13, "Adattata"}}},                  // italian label
    {0x06AFB, 85, 0xE61E8C48u, {{7, 6, "R"}, {39, 4, "adattata"}}},    // italian question
    {0x05F80, 9, 0x655E0603u, {{0, 9, "AJUSTADO"}}},                    // spanish label
};

void rename_wide_mode(dream::Memory& m) {
    for (const TextEdit& e : kTextEdits) {
        const std::uint32_t at = kTexts + e.offset;
        if (m.read8(at + e.len) != 0)
            continue;
        std::string s(e.len, '\0');
        std::uint32_t h = 0x811C9DC5u;
        for (unsigned i = 0; i < e.len; ++i) {
            s[i] = static_cast<char>(m.read8(at + i));
            h = (h ^ static_cast<std::uint8_t>(s[i])) * 0x01000193u;
        }
        if (h != e.fnv)
            continue;
        for (int i = 1; i >= 0; --i)  // splices are listed by position
            if (e.splices[i].ins)
                s.replace(e.splices[i].pos, e.splices[i].del, e.splices[i].ins);
        s.resize(e.len, '\0');  // never longer: checked when the table was made
        for (unsigned i = 0; i < e.len; ++i)
            m.write8(at + i, static_cast<std::uint8_t>(s[i]));
    }
}

void on_vblank(dream::System& sys, dreamcomp::Settings& settings) {
    rename_wide_mode(sys.memory);
    if (g_boot_reset && !g_boot_done) {  // the reset has run: set the remembered mode
        g_boot_done = true;
        if (settings.get("game_mode") == "adjusted")
            set_game_mode(sys.memory, true);
    }
    if (g_remember >= 0) {
        const char* v = g_remember ? "adjusted" : "4:3";
        g_remember = -1;
        if (settings.get("game_mode") != v) {
            settings.set("game_mode", v);
            settings.save();
        }
    }
}

const dreamcomp::PortInfo kInfo = [] {
    dreamcomp::PortInfo p;
    p.id = "rayman2";
    p.title = "Rayman 2: The Great Escape";
    p.languages = "english,french,german,spanish,italian";  // DCDATA/TEXTS/*.LNG on the USA disc
    p.widescreen = &apply_widescreen;
    p.max_aspect = 32.0f / 9.0f;  // past 16:9 through project_camera / unproject
    p.on_frame = &on_frame;
    p.on_vblank = &on_vblank;
    p.graphics_toggles = kToggles;
    p.graphics_toggle_count = 1;
    return p;
}();
dreamcomp::RegisterPort g_register(kInfo);

}  // namespace
