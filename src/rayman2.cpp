// Rayman 2: The Great Escape: what this port tells dreamcomp. Game facts with sources:
// docs/GAME-INTERNALS.md.
#include <algorithm>
#include <cmath>

#include "dream/render/display_list.h"
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

// The game has its own 16:9 mode (OPTIONS > VIDEO > RESOLUTION), drawn squeezed into 640x480.
// The engine only has to unsqueeze it, so nothing is written to the game.
// shortcut: the player must also pick 16:9 in the game's menu (else the 4:3 view is stretched);
// upgrade by finding that option's RAM flag and setting it here.
void apply_widescreen(dream::System&, float) {}

const dreamcomp::PortInfo kInfo = [] {
    dreamcomp::PortInfo p;
    p.id = "rayman2";
    p.title = "Rayman 2: The Great Escape";
    p.widescreen = &apply_widescreen;
    p.max_aspect = 16.0f / 9.0f;  // the game's own mode is 16:9 only
    p.on_frame = &on_frame;
    p.graphics_toggles = kToggles;
    p.graphics_toggle_count = 1;
    return p;
}();
dreamcomp::RegisterPort g_register(kInfo);

}  // namespace
