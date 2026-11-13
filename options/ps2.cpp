// The emulator's settings on the PlayStation 2.
#include "options.h"

namespace deadui::options
{

const std::vector<Page>& pages()
{
	static const std::vector<Page> list {
		{ "Console", "PlayStation 2 settings.", {
			{ "core", "FastDisc", "Fast Disc", "Faster disc loading.", { { "Off", 0 }, { "On", 1 } }, false, 0 },
		} },
		{ "Emulator", "Deadstation 2 settings.", {
			{ "video", "ShowFPS", "Show FPS", "Frame rate overlay.", { { "Off", 0 }, { "On", 1 } }, true, 1 },
			{ "video", "Picture", "Picture", "Picture shape.", { { "Original", 0 }, { "Fill", 1 } }, false, 0 },
		} },
	};
	return list;
}

}
