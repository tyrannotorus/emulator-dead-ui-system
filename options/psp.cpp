// The emulator's settings on the PSP.
#include "options.h"

namespace deadui::options
{

const std::vector<Page>& pages()
{
	static const std::vector<Page> list {
		{ "Console", "The PSP's own settings.", {
			{ "core", "Language", "Language", "The PSP's language, for games that have it. Takes effect on restart.",
					{ { "Japanese", 0 }, { "English", 1 }, { "French", 2 }, { "Spanish", 3 }, { "German", 4 }, { "Italian", 5 },
					  { "Dutch", 6 }, { "Portuguese", 7 }, { "Russian", 8 }, { "Korean", 9 } }, false, 1 },
		} },
		{ "Video", "", {
			{ "video", "ShowFPS", "Show FPS", "Shows the frame rate.", { { "Off", 0 }, { "On", 1 } }, true, 0 },
			{ "video", "Resolution", "Resolution", "The picture's resolution: the PSP's, or twice it.",
					{ { "480x272", 1 }, { "960x544", 2 } }, false, 1 },
			{ "video", "Picture", "Picture", "Original is the PSP's shape. Fill stretches it to the screen. Crop fills the screen, cutting off the sides.",
					{ { "Original", 0 }, { "Fill", 1 }, { "Crop", 2 } }, false, 0 },
			{ "video", "FrameSkip", "Frame Skip", "Skips drawing frames when the game falls behind.",
					{ { "Off", 0 }, { "Auto", 1 } }, false, 0 },
			{ "video", "Mipmaps", "Far Textures", "Smooth is much faster and doesn't shimmer. Sharp is the PSP's own.",
					{ { "Sharp", 0 }, { "Smooth", 1 } }, false, 1 },
			{ "video", "FrameRate", "Frame Rate", "God of War games only. 30 stays at full speed. 60 is smoother but slows the game where it can't keep up.",
					{ { "30", 30 }, { "60", 60 } }, true, 30 },
		} },
	};
	return list;
}

}
