// Recomp modules: recompiled builds of single games, played where one is installed for the disc and
// chosen as its core. The emulator hosts them (host.h): a module is the game only, and the menu, Options
// and the store are the emulator's. Each lives in its own folder, <home>/recomp/<id>/, with its
// executable, a store.json (what its page in the Recomp Store says, download.h) and a manifest,
// module.cfg:
//
//   id = mygame                     the folder's name
//   name = My Game
//   version = 1.0.0
//   exec = mygame                   the executable, beside the manifest
//   host = 1                        the format it is hosted in (HostFormat): not checked
//   disc = 0123abcd...              each disc it plays, by the SHA-1 its CHD's header records
//   title = My Game                 what the menu calls it
//   setting = scale | Resolution | 1=640x480, panel=1024x768 | panel
//                                   each setting, shown in Options: key | name | value=label, ... | default
//   control = a | A Button | A      each control the player may remap, on its Controls page: key | name | default,
//                                   the pad's inputs by their labels, comma separated, or NONE
//
// A module has no settings of its own in the game: it declares them here, Options shows them, the
// store below keeps them, and the host hands them to it.
#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace deadui::recomp
{

// The emulator's own folder (DEADUI_HOME_ENV): its settings, the modules
std::string home();

// the SHA-1 a CHD's header records (version 5), in lowercase hex; empty for anything else
std::string chdSha1(const std::string& path);

struct Setting
{
	std::string key;
	std::string name;
	std::vector<std::pair<std::string, std::string>> values;	// value, label
	std::string def;
};
struct Control
{
	std::string key;
	std::string name;
	std::string def;
};
struct Module
{
	std::string id;
	std::string name;
	std::string title;
	std::string version;
	std::string exec;
	std::string dir;
	std::vector<std::string> discs;
	std::vector<Setting> settings;
	std::vector<Control> controls;
	int host = 0;
};
// a file's text; empty if it cannot be read
std::string readText(const std::string& file);
// key = value lines, # comments; a key may come more than once
std::vector<std::pair<std::string, std::string>> keyValues(const std::string& text);
bool readManifest(const std::string& file, Module& module);
// every module installed
std::vector<Module> allInstalled();
// every module installed that plays a disc (its CHD's SHA-1), by title: a disc may have several. Only a folder
// named as its manifest's id counts: never what an install or a deletion left half done (<id>.new, <id>.old)
std::vector<Module> installedFor(const std::string& discSha1);
// the module this process hosts, if any
void setRunning(const std::string& id);
std::string runningModule();

// a file, or a folder and everything in it; true if gone
bool removeTree(const std::string& path);

// Deleting a module takes everything of it: its folder, its data (<home>/recomp-data/<id>/: saves,
// settings, cache) and its core in recomp.cfg. It is marked first (<home>/recomp/<id>.delete) and
// the mark goes last, so a deletion cut short is finished by tidy(). The module being hosted
// is only marked: tidy() deletes it at the emulator's next start, once it has ended.
void markForDeletion(const std::string& id);
bool deleteModule(const std::string& id);	// true if all of it is gone
// At the emulator's start, before anything: the deletions marked done, and what an install cut short left
// behind (<id>.download, <id>.new, <id>.old) removed
void tidy();

// The core chosen for a disc in Options: one of its modules is hosted until the player leaves it, and this
// process then exits. Returns only if the emulator is the core.
void runChosen(const std::string& disc, std::function<void()> controls = {});
// The format of a module's hosting this emulator speaks (module.cfg's host, host.h)
constexpr int HostFormat = 1;
// The exit code that asks the launcher to start the game again, in the core now chosen
constexpr int SwitchExit = 86;

// What the menu picks, key = value lines, each change saved at once. In <home>/recomp.cfg: core.<module> (recomp: this
// module is the core of the discs it plays, emulator: not) and app.version (the emulator's, which a module cannot tell
// by itself). Each module's own settings and controls are in its folder of saves, <home>/recomp-data/<module>/settings.cfg,
// and go when it is deleted (moduleGet, moduleSet). The module never reads that file: the host hands it the values.
std::string get(const std::string& key, const std::string& def = "");
void set(const std::string& key, const std::string& value);
std::string moduleGet(const std::string& id, const std::string& key, const std::string& def = "");
void moduleSet(const std::string& id, const std::string& key, const std::string& value);

}
