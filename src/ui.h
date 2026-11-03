// The menu over a paused game and the start page, drawn on the CPU so that they need nothing of the host's
// renderer. Shared by an emulator and its recomp modules, so that each one plays with the same menu: a module
// adds no menu of its own. A host (the emulator, a module) gives it the screen, the
// menu's buttons, the paused game's picture and its own pages; it needs nothing of the emulator.
//
// Every page looks like the first: its title in a pill at the top, its rows (or a body of its own), the
// buttons at the bottom right and the selected row's description (or the page's status) at the bottom
// left, over the paused game blurred. Opening or leaving a page slides its rows; the pills stay in place.
#pragma once
#include "canvas.h"
#include "options.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace deadui::ui
{

// the screen: full-screen BGRA pictures instead of the game's frames while a page is up
class Display
{
public:
	virtual ~Display() = default;
	// pictures from now on; false if there is no screen to draw on
	virtual bool begin(int& width, int& height) = 0;
	virtual void show(const u32 *pixels) = 0;
	// after the pictures: the game's last frame again (it goes on), its next one (a game starting), or the
	// last picture left up (quitting)
	enum class After { LastFrame, NextFrame, Stay };
	virtual void end(After after) = 0;
};
void setDisplay(Display *display);

// the menu's buttons, which the host maps its own onto
enum class Button { None, Up, Down, Left, Right, A, B, X, Menu };

// The layout on the Brick: its 1x sizes, three times as large
constexpr int Scale = 3;
constexpr int S(int v) { return v * Scale; }
constexpr int Padding = S(5);
constexpr int PillSize = S(30);
constexpr int ButtonSize = S(20);
constexpr int ButtonMargin = S(5);
constexpr int ButtonPadding = S(12);
constexpr u32 White = 0xffffff;
constexpr u32 Gray = 0x999999;
constexpr u32 LightGray = 0x7f7f7f;
constexpr u32 TextGray = 0xb4b4b4;		// text under a page's line
constexpr u32 Black = 0x000000;

// Noto Sans Bold at the layout's sizes
struct Fonts
{
	Font large;		// the main list, messages
	Font medium;	// a button's letter
	Font small;		// options, hints
	Font tiny;		// values, descriptions
	Font page;		// the start page's line
	Font pageText;	// and the text under it
};
Fonts& fonts();
// what pages are drawn on, the screen's size
Canvas& canvas();

// A row of a page: does something (A), picks one of its values (Left/Right), shows what a control is
// bound to (A waits for a control, X clears it), or names the section under it (the cursor goes past it)
enum class Row { Action, Choice, Binding, Header };
struct Item
{
	std::string name;
	std::string desc;
	Row row = Row::Action;
	std::vector<std::string> values;	// Choice: what it may be; Binding: what it is bound to
	int value = 0;
	std::function<void(int)> change;	// Choice: a value picked
	std::function<void()> confirm;		// A
	std::function<void(int)> shift;		// Left/Right on an Action, -1 or 1 (Save, Load: the slot)
	std::function<void()> clear;		// X on a Binding
	std::function<bool()> available;	// while false: greyed, and the cursor goes past it
	std::function<void(int ox)> aside;	// drawn at the right while selected (Save, Load: the slot's picture)
	bool downloadIcon = false;			// before its name (the store's: an update is available)
};
Item entry(const std::string& name, const std::string& desc, std::function<void()> confirm);
bool enabled(const Item& item);

using Buttons = std::vector<std::pair<const char *, const char *>>;
struct Screen
{
	std::string title;
	std::vector<Item> items;
	std::string desc;						// unless the selected item has its own
	std::function<std::string()> status;	// at the bottom left, whatever is selected (the main page's: the network)
	int selected = 0;
	int start = 0;							// the first row shown
	std::function<void()> back;				// B, and MENU on the first page; else the page before again
	std::function<void()> tick;				// every frame while shown (redraw(), finish() when something changed)
	bool readsPad = false;					// its tick brings the pad's presses (button()): a host's own reading waits
	// a page of its own (the link's): its body drawn at ox, its buttons and its input, instead of rows
	std::function<void(int ox)> body;
	std::function<Buttons()> buttons;
	std::function<void(Button)> act;
};

// what plays the game: the emulator, or a recomp module (a recompiled build of the game)
enum class Core { Emulator, Recomp };
// what the host brings to the main page and Options
struct Host
{
	Core core = Core::Emulator;					// the core this process plays the game with
	bool modulePlaying = false;					// a hosted module's process is there, paused (not one that has stopped)
	std::string disc;							// the disc played (its recomp module found by it)
	Item save;									// the emulator's save states (a module: none, disabled)
	Item load;
	std::string version;						// under Options' rows with the emulator picked: the emulator's
	// the emulator's settings (options.h): the emulator's own; a module's, emu.cfg
	options::Store emulatorSettings;
	// the emulator's own pages in Options (a module's host: Controls if the program brings it; Local Multiplayer, none)
	std::function<void()> controls;
	std::function<void()> localMultiplayer;
	// a recomp setting changed (key, value): a module applies it, if it can while playing
	std::function<void(const std::string&, const std::string&)> recompSetting;
};
// the first page: Continue, Save, Load, Options, Recomp Store, Quit, the
// network's status at the bottom left
Screen mainScreen(const std::string& title, Host host);

// the menu over the paused game: its picture (RGB24, the backdrop, blurred and darkened) and the first page
bool open(const std::vector<u8>& frame, int frameWidth, int frameHeight, Screen first);
bool isOpen();
bool pageReadsPad();					// the page shown reads the pad itself
void push(Screen screen);
void pop();
Screen& page();							// the page shown
size_t depth();							// pages open (1: the first)
void move(Screen& screen, int step);	// the selected row, to the next enabled one that way
void redraw();
// over everything until A or B
void message(const std::string& text);
// A does yes (its button saying yesHint), B no
void ask(const std::string& text, const char *yesHint, std::function<void()> yes, std::function<void()> no = {});
// a page saying what is being done (the line, the text under it) while work runs: it may take seconds
void work(const std::string& line, const std::string& under, std::function<void()> fn);
// over the page instead ("Checking...")
void busy(const std::string& text, std::function<void()> fn);
// a Binding row waiting for a control (the host reads it: the menu's buttons are not)
void awaitControl(bool waiting);
bool awaitingControl();
// a press for a module's Controls row waiting for one, by the labels on the device; anything else is ignored
void controlPressed(const std::string& input);
void button(Button button, bool pressed);

// Restart: the game again from its start (two players); Switch: in the other core (its choice stored)
enum class Result { Open, Resume, Quit, Restart, Switch };
void finish(Result result);
// every frame while open: repeats held directions, redraws when needed; closes on a result
Result frame();

// the start page, until the game's first frame: a line, the disc's name under it
bool showStart(const std::string& line, const std::string& under);
bool starting();
void startLine(const std::string& line, const std::string& under);
void endStart();
// a page of the host's own over everything (the BIOS page): drawn by draw while open, A or B quits
bool showPage(std::function<void()> draw);
// a page straight onto the screen as it is (fb0's page on show, in its mode), between two programs:
// The emulator handing the game to a recomp module, or back. Stays until the next one draws.
void putPage(const std::string& line, const std::string& under);

// the font itself, as a TrueType file in the binary: for anything else the program draws text with
void fontData(const unsigned char *&data, size_t& size);
// small text on its dark box as BGRA pixels, its size in width and height (none without the font): Show FPS and the
// emulator's messages, for a frame of this height; built from the same characters a module's counter is
std::vector<u32> label(const std::string& text, int frameHeight, int& width, int& height);
// text broken into lines that fit that width as labels
std::vector<std::string> labelLines(const std::string& text, int frameHeight, int maxWidth);
// a counter's characters, for a process with no fonts to compose (a hosted module): 32-bit values, its padding and
// the characters' height, how many, then for each its code, its width and its pixels. False if not written.
bool writeLabelCells(const std::string& file, const std::string& chars);

// a page's line in white and text under it in grey, centered on black
void drawPage(const std::string& line, const std::string& under);
// lines of text centered in a rectangle
void drawMessage(Font& font, const std::string& message, int x, int y, int w, int h);
// text broken at spaces into lines no wider than width
std::vector<std::string> wrap(Font& font, const std::string& text, int width);

// the network: a default route other than over the link to another Brick, checked every second while open
bool network();
std::string networkStatus();

// the game's name: a file's, without its extensions or trailing (...) and [...]
std::string gameName(const std::string& path);

}
