// The emulator's settings on the PlayStation. No BIOS row: the PlayStation's own BIOS always plays (its startup screen
// too), the core's built-in one only when there is no BIOS file.
#include "options.h"

namespace deadui::options
{

const std::vector<Page>& pages()
{
	static const std::vector<Page> list {
		{ "Console", "The PlayStation's region and disc speed.", {
			{ "core", "pcsx_rearmed_region", "Region", "The console's region. Takes effect on restart.",
					{ { "Auto", 0 }, { "NTSC", 1 }, { "PAL", 2 } }, false, 0 },
			{ "core", "pcsx_rearmed_cd_turbo", "Fast Loading", "Reads the disc far faster than a PlayStation. A game that "
					"crashes or hangs with it can have it off on its own.", { { "Off", 0 }, { "On", 1 } }, false, 1 },
		} },
		{ "Video", "", {
			{ "deadstation", "ShowFPS", "Show FPS", "Shows the frame rate.", { { "Off", 0 }, { "On", 1 } }, true, 0 },
			{ "deadstation", "Resolution", "Resolution", "The picture's resolution: the PlayStation's, or the screen's own with the game drawn at twice its resolution.",
					{ { "640x480", 0 }, { "1024x768", 1 } }, false, 0 },
			{ "deadstation", "Smoothing", "Smoothing", "How the picture is scaled to the screen.",
					{ { "Sharp", 0 }, { "Smooth", 1 } }, false, 1 },
			{ "core", "pcsx_rearmed_dithering", "Dithering", "The PlayStation's dithering of colours.",
					{ { "Off", 0 }, { "On", 1 } }, false, 1 },
		} },
		{ "Audio", "", {
			{ "core", "pcsx_rearmed_spu_reverb", "Reverb", "The sound chip's reverb.", { { "Off", 0 }, { "On", 1 } }, false, 1 },
			{ "core", "pcsx_rearmed_spu_interpolation", "Interpolation", "How the sound chip's samples are smoothed.",
					{ { "Simple", 0 }, { "Gaussian", 1 }, { "Cubic", 2 }, { "Off", 3 } }, false, 0 },
		} },
	};
	return list;
}

}
