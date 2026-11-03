// The emulator's settings on the Dreamcast. Not listed: the TV standard and the cable, kept NTSC and VGA for the
// Brick's screen.
#include "options.h"

namespace deadui::options
{

const std::vector<Page>& pages()
{
	static const std::vector<Page> list {
		{ "Console", "The Dreamcast's region and language.", {
			{ "config", "Dreamcast.Region", "Region", "The console's region. Takes effect on restart.",
					{ { "Japan", 0 }, { "USA", 1 }, { "Europe", 2 }, { "Default", 3 } }, false, 1 },
			{ "config", "Dreamcast.Language", "Language", "The console's language. Takes effect on restart.",
					{ { "Japanese", 0 }, { "English", 1 }, { "German", 2 }, { "French", 3 }, { "Spanish", 4 },
					  { "Italian", 5 }, { "Default", 6 } }, false, 1 },
		} },
		{ "Video", "", {
			{ "config", "rend.ShowFPS", "Show FPS", "Shows the frame rate.", { { "Off", 0 }, { "On", 1 } }, true, 0 },
			{ "config", "pvr.AutoSkipFrame", "Frame Skip", "Skips drawing frames when the game falls behind.",
					{ { "Off", 0 }, { "Normal", 1 }, { "Maximum", 2 } }, false, 0 },
			{ "config", "rend.TextureFiltering", "Texture Filter", "How the game's textures are smoothed.",
					{ { "Default", 0 }, { "Nearest", 1 }, { "Linear", 2 } }, false, 0 },
		} },
		{ "Audio", "", {
			{ "config", "aica.DSPEnabled", "Audio DSP", "The sound chip's effects, such as reverb. Costs speed.",
					{ { "Off", 0 }, { "On", 1 } }, true, 0 },
			{ "audio", "VmuSound", "VMU Sounds", "The VMU's beeps.", { { "Off", 0 }, { "On", 1 } }, true, 0 },
		} },
	};
	return list;
}

}
