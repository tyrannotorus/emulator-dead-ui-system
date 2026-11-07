#include "options.h"
#include "recomp.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

namespace deadui::options
{
namespace
{

std::string gamePath;		// games/<disc id>.cfg, "" without a game
std::vector<Page> extended;	// pages() with the game's own settings, once added

std::string trim(const std::string& s)
{
	const size_t a = s.find_first_not_of(" \t\r");
	if (a == std::string::npos)
		return "";
	return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
}

std::string path()
{
	return recomp::home() + "/emu.cfg";
}

std::vector<std::string> read(const std::string& file)
{
	std::vector<std::string> lines;
	std::ifstream in(file);
	for (std::string line; std::getline(in, line); )
		lines.push_back(line);
	return lines;
}

// where a setting is in the file: its line, else the line after its section's last (-1: no section)
struct Place
{
	int line = -1;
	int end = -1;
};

Place find(const std::vector<std::string>& lines, const Setting& setting)
{
	Place place;
	bool in = false;
	for (int i = 0; i < (int)lines.size(); i++)
	{
		const std::string t = trim(lines[i]);
		if (t.empty() || t[0] == ';')
			continue;
		if (t[0] == '[')
		{
			const size_t close = t.find(']');
			in = close != std::string::npos && t.substr(1, close - 1) == setting.section;
			if (in)
				place.end = i + 1;
			continue;
		}
		if (!in)
			continue;
		place.end = i + 1;
		const size_t eq = t.find('=');
		if (eq != std::string::npos && trim(t.substr(0, eq)) == setting.key)
			place.line = i;
	}
	return place;
}

// the setting's value in the file: false if it isn't there
bool value(const std::string& file, const Setting& setting, int& value)
{
	const std::vector<std::string> lines = read(file);
	const Place place = find(lines, setting);
	if (place.line < 0)
		return false;
	const std::string& line = lines[place.line];
	const std::string text = trim(line.substr(line.find('=') + 1));
	if (setting.flag)
		value = strcasecmp(text.c_str(), "yes") == 0 || strcasecmp(text.c_str(), "true") == 0
				|| strcasecmp(text.c_str(), "on") == 0 || text == "1";
	else
	{
		// as the emulator reads a number: hexadecimal after 0x, else decimal
		const bool hex = text.size() > 1 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X');
		value = (int)std::strtoul(text.c_str(), nullptr, hex ? 16 : 10);
	}
	return true;
}

int get(const Setting& setting)
{
	int v;
	return value(path(), setting, v) ? v : setting.fallback;
}

// the file's lines written beside it, then in its place (a game's file: its folder made first)
void write(const std::string& file, const std::vector<std::string>& lines)
{
	if (file == gamePath)
		mkdir((recomp::home() + "/games").c_str(), 0755);
	const std::string temp = file + ".new";
	FILE *out = std::fopen(temp.c_str(), "w");
	if (out == nullptr)
	{
		std::printf(DEADUI_APP_NAME ": %s cannot be written\n", temp.c_str());
		return;
	}
	for (const std::string& l : lines)
		std::fprintf(out, "%s\n", l.c_str());
	const bool written = std::fflush(out) == 0 && fsync(fileno(out)) == 0;
	std::fclose(out);
	if (!written || std::rename(temp.c_str(), file.c_str()) != 0)
	{
		std::remove(temp.c_str());
		std::printf(DEADUI_APP_NAME ": %s cannot be written\n", file.c_str());
	}
}

// the file again, the setting's line changed or added
void setIn(const std::string& file, const Setting& setting, int value)
{
	std::vector<std::string> lines = read(file);
	const std::string line = std::string(setting.key) + " = "
			+ (setting.flag ? (value != 0 ? "yes" : "no") : std::to_string(value));
	const Place place = find(lines, setting);
	if (place.line >= 0)
		lines[place.line] = line;
	else if (place.end >= 0)
		lines.insert(lines.begin() + place.end, line);
	else
	{
		if (!lines.empty() && !trim(lines.back()).empty())
			lines.push_back("");
		lines.push_back(std::string("[") + setting.section + "]");
		lines.push_back(line);
	}
	write(file, lines);
}

void set(const Setting& setting, int value)
{
	setIn(path(), setting, value);
}

// the game's file without the setting: gone with its last one
void drop(const Setting& setting)
{
	if (gamePath.empty())
		return;
	std::vector<std::string> lines = read(gamePath);
	const Place place = find(lines, setting);
	if (place.line < 0)
		return;
	lines.erase(lines.begin() + place.line);
	for (const std::string& l : lines)
		if (l.find('=') != std::string::npos)
		{
			write(gamePath, lines);
			return;
		}
	if (std::remove(gamePath.c_str()) != 0)
		std::printf(DEADUI_APP_NAME ": %s cannot be removed\n", gamePath.c_str());
}

// a setting only some games have: never a default
bool own(const Setting& setting)
{
	return std::strcmp(setting.section, "game") == 0;
}

int fallback(const Setting& setting)
{
	return own(setting) ? setting.fallback : get(setting);
}

int gameGet(const Setting& setting)
{
	int v;
	return !gamePath.empty() && value(gamePath, setting, v) ? v : fallback(setting);
}

void gameSet(const Setting& setting, int value)
{
	if (gamePath.empty())
		set(setting, value);
	else if (value == fallback(setting))
		drop(setting);
	else
		setIn(gamePath, setting, value);
}

}	// namespace

Store file()
{
	return { get, set };
}

void setGame(const std::string& discId)
{
	gamePath = discId.empty() ? "" : recomp::home() + "/games/" + discId + ".cfg";
}

bool hasGame()
{
	return !gamePath.empty();
}

std::string gameFile(const std::string& suffix)
{
	return gamePath.empty() ? "" : gamePath.substr(0, gamePath.size() - 4) + suffix;
}

Store game()
{
	return { gameGet, gameSet };
}

void saveAsDefault(const Page& page)
{
	for (const Setting& setting : page.settings)
	{
		if (own(setting))
			continue;
		const int v = gameGet(setting);
		if (v != get(setting))
			set(setting, v);
		drop(setting);
	}
}

void addGameSettings(const char *page, std::vector<Setting> settings)
{
	if (extended.empty())
		extended = pages();
	for (Page& p : extended)
		if (std::strcmp(p.name, page) == 0)
			p.settings.insert(p.settings.end(), settings.begin(), settings.end());
}

const std::vector<Page>& gamePages()
{
	return extended.empty() ? pages() : extended;
}

}
