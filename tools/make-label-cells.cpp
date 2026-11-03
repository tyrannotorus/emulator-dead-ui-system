// Draws the small text's characters from the font into the files src/ui.cpp builds in. See make-label-cells.sh.
// The format, 32-bit values: its padding, the characters' height and how many, then for each its code, its width and its
// pixels (BGRA, width x height).
#include "canvas.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

using namespace deadui;

// the emulator's yellow at 70% over its dark box
static constexpr u32 Box = 0x202020;
static constexpr u32 Text = 0xbcbc0a;

static bool write(FT_Library library, const std::vector<char>& ttf, int px, int pad, const std::string& chars, const std::string& file)
{
	Font font;
	if (!font.init(library, (const u8 *)ttf.data(), ttf.size(), px))
		return false;
	std::FILE *f = std::fopen(file.c_str(), "wb");
	if (f == nullptr)
		return false;
	const u32 height = (u32)font.lineHeight();
	const u32 head[3] = { (u32)pad, height, (u32)chars.size() };
	bool written = std::fwrite(head, sizeof(head[0]), 3, f) == 3;
	for (const char ch : chars)
	{
		const std::string text(1, ch);
		const int width = std::max(font.measure(text), 1);
		Canvas c;
		c.resize(width, (int)height);
		c.clear(Box);
		c.text(font, text, 0, font.ascent(), Text);
		const u32 cell[2] = { (u32)(unsigned char)ch, (u32)width };
		written = written && std::fwrite(cell, sizeof(cell[0]), 2, f) == 2
				&& std::fwrite(c.data(), sizeof(u32), (size_t)width * height, f) == (size_t)width * height;
	}
	return std::fclose(f) == 0 && written;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::fprintf(stderr, "usage: make-label-cells FONT.ttf OUT_DIR\n");
		return 2;
	}
	std::ifstream in(argv[1], std::ios::binary);
	const std::vector<char> ttf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	FT_Library library;
	if (ttf.empty() || FT_Init_FreeType(&library) != 0)
		return 1;
	const std::string out = argv[2];
	std::string printable;
	for (char ch = ' '; ch <= '~'; ch++)
		printable += ch;
	// for frames drawn at 640 x 480, which the display engine scales to the panel: as tall there as the next
	const bool frame = write(library, ttf, 18, 3, printable, out + "/label-cells-480.bin");
	// a module's frame counter, on frames at the panel's size: only what it may show
	const bool counter = write(library, ttf, 30, 6, "0123456789. ", out + "/label-cells-counter.bin");
	return frame && counter ? 0 : 1;
}
