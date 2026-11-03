// The emulator as the host of a recomp module. The module plays the game in a process of its own; this
// process owns the menu, the module's options and the store, and sleeps while the game plays: it waits only on
// the pad and on a pipe the module holds, with no timer and no other thread.
//
// Two pipes, passed to the module as DEADUI_HOST=<read fd>,<write fd>. Lines of text:
//   to the module:   pause, resume, set <key> <value>, quit
//   from the module: ready (its first vblank), paused (nothing more of it reaches the screen)
// The module's pipe at its end: the module has ended. Its settings reach it at the start as --set key=value.
//
// Nothing here falls back on anything. A module that cannot run says why, on the screen and in the log, over
// this process's own menu, from which Core and the store can still be reached.
#pragma once
#include "recomp.h"

#include <functional>
#include <string>

namespace deadui::host
{

// the module played until the player leaves it: what this process is to exit with. controls: pushes the program's
// Controls page, which reads the pad itself while it is shown (ui::Screen::readsPad)
int run(recomp::Module module, const std::string& disc, std::function<void()> controls = {});

}
