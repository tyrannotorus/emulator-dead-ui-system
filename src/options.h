// The emulator's settings: one list, shown the same in the emulator and in a recomp module, from
// options/<console>.cpp. The emulator shows and changes its own settings as it plays; a module keeps them in
// emu.cfg, the emulator's file, for the emulator's next start (the two never run at once). Each game has its own
// values over emu.cfg's, the defaults (game() below).
#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace deadui::options
{

// a setting as emu.cfg keeps it, [section] key = value: a number, or yes/no for a flag
struct Setting
{
	const char *section;
	const char *key;
	const char *name;
	const char *desc;
	std::vector<std::pair<const char *, int>> choices;	// label, value
	bool flag;
	int fallback;		// the emulator's default, for a file without it
};

// Console (the console's own settings), Emulator (the emulator's)
struct Page
{
	const char *name;
	const char *desc;
	std::vector<Setting> settings;
};
const std::vector<Page>& pages();

// where the values are: the emulator's settings, or emu.cfg
struct Store
{
	std::function<int(const Setting&)> get;
	std::function<void(const Setting&, int)> set;	// kept at once
};
// emu.cfg in the emulator's home (recomp::home()), read and written as the emulator reads it
Store file();

// Each game's own values: games/<disc id>.cfg in the emulator's home, as emu.cfg, with only what differs from emu.cfg
// (the defaults). A setting only some games have is in the "game" section, never a default: its fallback is.
// the game played, by its disc id, before its settings are read ("": none, a change is a default)
void setGame(const std::string& discId);
bool hasGame();
// a file of the game's own beside its settings, games/<disc id><suffix> (".controls.cfg"): "" without a game
std::string gameFile(const std::string& suffix);
// a value as the game plays it: the game's, else the default; a change kept for the game (one back to the default
// dropped from its file)
Store game();
// a page's settings, as the game has them, as the defaults for every game: the game then follows the defaults
// (its own settings stay its own)
void saveAsDefault(const Page& page);
// this game's own settings, at the end of one of pages() (by its name), before Options is shown
void addGameSettings(const char *page, std::vector<Setting> settings);
// pages() with the game's own settings: Options' pages
const std::vector<Page>& gamePages();

}
