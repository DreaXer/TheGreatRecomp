// Rayman 2: The Great Escape: what this port tells dreamcomp. Game facts with sources:
// docs/GAME-INTERNALS.md.
#include "dreamcomp/hook.h"
#include "dreamcomp/port.h"

namespace {

const dreamcomp::PortInfo kInfo = [] {
    dreamcomp::PortInfo p;
    p.id = "rayman2";
    p.title = "Rayman 2: The Great Escape";
    return p;
}();
dreamcomp::RegisterPort g_register(kInfo);

}  // namespace
