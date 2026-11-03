// host.h
#include "host.h"
#include "options.h"
#include "ui.h"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace deadui::host
{
namespace
{

volatile std::sig_atomic_t stopAsked = 0;

constexpr int PauseWaitMs = 5000;
constexpr int QuitWaitMs = 8000;
constexpr int TermWaitMs = 3000;
constexpr int ReleaseWaitMs = 1000;
constexpr int HungWaitMs = 1000;
constexpr int CannotRun = 127;	// the exit of a child whose exec failed

// the pages of fb0 this process draws on: under the module's own
constexpr u32 FirstPage = 14, Pages = 2;

unsigned long long cpuMicroseconds()
{
	timespec t {};
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
	return (unsigned long long)t.tv_sec * 1000000ull + (unsigned long long)t.tv_nsec / 1000ull;
}

// the panel, written directly
class Screen final : public ui::Display
{
public:
	bool begin(int& width, int& height) override
	{
		if (!map())
			return false;
		width = (int)var.xres;
		height = (int)var.yres;
		return true;
	}

	void show(const u32 *pixels) override
	{
		const u32 row = (FirstPage + page) * var.yres;
		for (u32 y = 0; y < var.yres; y++)
			std::memcpy(memory + (size_t)(row + y) * fix.line_length, pixels + (size_t)y * var.xres, (size_t)var.xres * 4);
		var.yoffset = row;
		if (ioctl(fd, FBIOPAN_DISPLAY, &var) != 0)
			std::printf(DEADUI_APP_NAME ": host: cannot show the menu's page: %s\n", std::strerror(errno));
		page = (page + 1) % Pages;
	}

	void end(After) override {}

	// the picture on the screen now, as RGB24, every other pixel: it is only blurred for the menu's backdrop, and
	// fb0 is slow to read
	bool picture(std::vector<u8>& rgb, int& width, int& height)
	{
		if (!map() || ioctl(fd, FBIOGET_VSCREENINFO, &var) != 0)
			return false;
		constexpr u32 Step = 2;
		width = (int)(var.xres / Step);
		height = (int)(var.yres / Step);
		rgb.resize((size_t)width * height * 3);
		const size_t first = (size_t)var.yoffset * fix.line_length + (size_t)var.xoffset * 4;
		size_t out = 0;
		for (u32 y = 0; y < (u32)height; y++)
		{
			const u32 *row = (const u32 *)(memory + first + (size_t)y * Step * fix.line_length);
			for (u32 x = 0; x < (u32)width; x++)
			{
				const u32 p = row[x * Step];
				rgb[out++] = (u8)(p >> 16);
				rgb[out++] = (u8)(p >> 8);
				rgb[out++] = (u8)p;
			}
		}
		return true;
	}

	// nothing of the screen kept while the game plays
	void release()
	{
		if (memory != nullptr)
			munmap(memory, size);
		if (fd >= 0)
			close(fd);
		memory = nullptr;
		fd = -1;
	}

private:
	bool map()
	{
		if (memory != nullptr)
			return true;
		fd = open("/dev/fb0", O_RDWR | O_CLOEXEC);
		if (fd < 0 || ioctl(fd, FBIOGET_VSCREENINFO, &var) != 0 || ioctl(fd, FBIOGET_FSCREENINFO, &fix) != 0
				|| var.bits_per_pixel != 32 || (FirstPage + Pages) * var.yres > var.yres_virtual)
		{
			std::printf(DEADUI_APP_NAME ": host: no screen to draw the menu on: %s\n", std::strerror(errno));
			release();
			return false;
		}
		size = (size_t)fix.smem_len;
		void *mapped = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (mapped == MAP_FAILED)
		{
			std::printf(DEADUI_APP_NAME ": host: cannot map the screen: %s\n", std::strerror(errno));
			release();
			return false;
		}
		memory = (unsigned char *)mapped;
		return true;
	}

	int fd = -1;
	unsigned char *memory = nullptr;
	size_t size = 0;
	u32 page = 0;
	fb_var_screeninfo var {};
	fb_fix_screeninfo fix {};
};

Screen screen;

int openPad()
{
	char path[32], name[128];
	for (int i = 0; i < 16; i++)
	{
		std::snprintf(path, sizeof(path), "/dev/input/event%d", i);
		const int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
		if (fd < 0)
			continue;
		if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0 && std::strcmp(name, "TRIMUI Player1") == 0)
			return fd;
		close(fd);
	}
	std::printf(DEADUI_APP_NAME ": host: no pad (TRIMUI Player1): the menu cannot be opened\n");
	return -1;
}

// the pad's buttons and d-pad as the menu's
class PadToMenu
{
public:
	void press(const input_event& ev)
	{
		if (ev.type == EV_KEY && ev.value != 2)
		{
			const ui::Button b = ev.code == BTN_EAST ? ui::Button::A : ev.code == BTN_SOUTH ? ui::Button::B
					: ev.code == BTN_WEST ? ui::Button::X : ev.code == BTN_MODE ? ui::Button::Menu
					: ui::Button::None;
			if (b != ui::Button::None)
				ui::button(b, ev.value == 1);
		}
		else if (ev.type == EV_ABS && (ev.code == ABS_HAT0X || ev.code == ABS_HAT0Y))
		{
			int& was = ev.code == ABS_HAT0X ? hatX : hatY;
			const ui::Button less = ev.code == ABS_HAT0X ? ui::Button::Left : ui::Button::Up;
			const ui::Button more = ev.code == ABS_HAT0X ? ui::Button::Right : ui::Button::Down;
			if (was != 0)
				ui::button(was < 0 ? less : more, false);
			if (ev.value != 0)
				ui::button(ev.value < 0 ? less : more, true);
			was = ev.value;
		}
	}

private:
	int hatX = 0;
	int hatY = 0;
};

enum class Outcome { Resume, Quit, Switch, Stopped };

// a module's process and the pipes to it
class Child
{
public:
	~Child() { closePipes(); }

	bool start(const recomp::Module& module, const std::string& disc, std::string& why)
	{
		int toChild[2], fromChild[2];
		if (pipe2(toChild, O_CLOEXEC) != 0 || pipe2(fromChild, O_CLOEXEC) != 0)
		{
			why = std::string("no pipe: ") + std::strerror(errno);
			return false;
		}
		const std::string exec = module.dir + "/" + module.exec;
		std::vector<std::string> args { exec, disc };
		for (const recomp::Setting& setting : module.settings)
		{
			args.push_back("--set");
			args.push_back(setting.key + "=" + recomp::get(module.id + "." + setting.key, setting.def));
		}
		std::vector<char *> argv;
		for (std::string& arg : args)
			argv.push_back(&arg[0]);
		argv.push_back(nullptr);
		char fds[32];
		std::snprintf(fds, sizeof(fds), "%d,%d", toChild[0], fromChild[1]);
		setenv("DEADUI_HOST", fds, 1);
		std::fflush(stdout);

		pid = fork();
		if (pid < 0)
		{
			why = std::string("cannot start: ") + std::strerror(errno);
			return false;
		}
		if (pid == 0)
		{
			// a host killed outright leaves no module behind
			prctl(PR_SET_PDEATHSIG, SIGTERM);
			signal(SIGPIPE, SIG_DFL);
			signal(SIGTERM, SIG_DFL);
			fcntl(toChild[0], F_SETFD, 0);
			fcntl(fromChild[1], F_SETFD, 0);
			execv(exec.c_str(), argv.data());
			std::printf(DEADUI_APP_NAME ": host: %s cannot run: %s\n", exec.c_str(), std::strerror(errno));
			std::fflush(stdout);
			_exit(CannotRun);
		}
		close(toChild[0]);
		close(fromChild[1]);
		in = fromChild[0];
		out = toChild[1];
		fcntl(in, F_SETFL, O_NONBLOCK);
		std::printf(DEADUI_APP_NAME ": host: %s %s started, pid %d\n", exec.c_str(), module.version.c_str(), (int)pid);
		std::fflush(stdout);
		return true;
	}

	int status() const { return in; }
	bool running() const { return pid > 0; }

	void send(const std::string& line)
	{
		if (out < 0)
			return;
		const std::string text = line + "\n";
		if (write(out, text.data(), text.size()) != (ssize_t)text.size())
			std::printf(DEADUI_APP_NAME ": host: the module did not take \"%s\": %s\n", line.c_str(), std::strerror(errno));
	}

	// what the module has said; false once its pipe has ended
	bool readLines(std::vector<std::string>& lines)
	{
		char data[256];
		for (;;)
		{
			const ssize_t size = read(in, data, sizeof(data));
			if (size == 0)
				return false;
			if (size < 0)
				return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
			pending.append(data, (size_t)size);
			for (size_t end; (end = pending.find('\n')) != std::string::npos; pending.erase(0, end + 1))
				lines.push_back(pending.substr(0, end));
		}
	}

	// a line from the module within the time; false if not, or if it has ended
	bool waitFor(const std::string& line, int ms)
	{
		const long long until = now() + ms;
		for (;;)
		{
			std::vector<std::string> lines;
			const bool open = readLines(lines);
			if (std::find(lines.begin(), lines.end(), line) != lines.end())
				return true;
			if (!open)
				return false;
			const long long left = until - now();
			if (left <= 0)
				return false;
			pollfd f { in, POLLIN, 0 };
			poll(&f, 1, (int)left);
		}
	}

	// why it ended, once it has; empty if it ended as it should
	std::string reap()
	{
		int code = 0;
		if (pid > 0 && waitpid(pid, &code, 0) != pid)
			code = 0;
		pid = -1;
		closePipes();
		if (WIFSIGNALED(code))
		{
			char text[64];
			std::snprintf(text, sizeof(text), "signal %d", WTERMSIG(code));
			return text;
		}
		if (WIFEXITED(code) && WEXITSTATUS(code) == CannotRun)
			return "it cannot run";
		if (WIFEXITED(code) && WEXITSTATUS(code) != 0)
		{
			char text[64];
			std::snprintf(text, sizeof(text), "exit code %d", WEXITSTATUS(code));
			return text;
		}
		return "";
	}

	// ended: asked to, then told to, then made to
	void stop()
	{
		if (pid <= 0)
			return;
		send("quit");
		if (!waitEnd(QuitWaitMs))
		{
			std::printf(DEADUI_APP_NAME ": host: the module did not quit: SIGTERM\n");
			kill(pid, SIGTERM);
			if (!waitEnd(TermWaitMs))
			{
				std::printf(DEADUI_APP_NAME ": host: the module did not end: SIGKILL\n");
				kill(pid, SIGKILL);
			}
		}
		const std::string why = reap();
		if (!why.empty())
			std::printf(DEADUI_APP_NAME ": host: the module ended stopping it: %s\n", why.c_str());
		std::fflush(stdout);
	}

	// it did not answer: no point asking it to quit
	void stopHung()
	{
		if (pid <= 0)
			return;
		kill(pid, SIGTERM);
		kill(pid, SIGCONT);
		if (!waitEnd(HungWaitMs))
		{
			std::printf(DEADUI_APP_NAME ": host: the module did not end: SIGKILL\n");
			kill(pid, SIGKILL);
		}
		reap();
	}

	static long long now()
	{
		timespec t {};
		clock_gettime(CLOCK_MONOTONIC, &t);
		return (long long)t.tv_sec * 1000ll + t.tv_nsec / 1000000ll;
	}

private:
	bool waitEnd(int ms)
	{
		const long long until = now() + ms;
		std::vector<std::string> lines;
		while (readLines(lines))
		{
			const long long left = until - now();
			if (left <= 0)
				return false;
			pollfd f { in, POLLIN, 0 };
			poll(&f, 1, (int)left);
		}
		return true;
	}

	void closePipes()
	{
		if (in >= 0)
			close(in);
		if (out >= 0)
			close(out);
		in = out = -1;
		pending.clear();
	}

	pid_t pid = -1;
	int in = -1;
	int out = -1;
	std::string pending;
};

Child child;
int pad = -1;
constexpr const char *LabelCells = "/tmp/deadui-label-cells";

std::string moduleId;
std::string moduleTitle;
std::string menuTitle;
std::string discPath;

// Show FPS: the module has no fonts, so the counter's characters are drawn once, here, for it to compose
void sendCounter(bool on)
{
	if (on && ui::writeLabelCells(LabelCells, "F:0123456789. "))
		child.send(std::string("set label ") + LabelCells);
	child.send(std::string("set fps ") + (on ? "on" : "off"));
}

ui::Host hostPage(bool playing)
{
	ui::Host h;
	h.core = ui::Core::Recomp;
	h.disc = discPath;
	h.modulePlaying = playing;
	h.emulatorSettings = options::file();
	h.recompSetting = [](const std::string& key, const std::string& value) {
		if (key == "fps")
			sendCounter(value == "on");
		else
			child.send("set " + key + " " + value);
	};
	return h;
}

// the menu until a result: over the picture given, the module (if any) paused
Outcome menuLoop(const std::vector<u8>& frame, int width, int height, bool playing, const std::string& problem)
{
	ui::setDisplay(&screen);
	if (!ui::open(frame, width, height, ui::mainScreen(menuTitle, hostPage(playing))))
	{
		std::printf(DEADUI_APP_NAME ": host: the menu cannot be opened\n");
		std::fflush(stdout);
		screen.release();
		return playing ? Outcome::Resume : Outcome::Quit;
	}
	if (!problem.empty())
		ui::message(problem);
	PadToMenu toMenu;
	bool ended = false;
	for (;;)
	{
		if (stopAsked)
			ui::finish(ui::Result::Quit);
		pollfd f[2] = { { pad, POLLIN, 0 }, { playing && !ended ? child.status() : -1, POLLIN, 0 } };
		poll(f, 2, 16);
		input_event ev;
		while (pad >= 0 && read(pad, &ev, sizeof(ev)) == (ssize_t)sizeof(ev))
			toMenu.press(ev);
		if (playing && !ended && (f[1].revents & (POLLIN | POLLHUP)))
		{
			std::vector<std::string> lines;
			if (!child.readLines(lines))
				ended = true;
		}
		switch (ui::frame())
		{
		case ui::Result::Open:
			if (!ended)
				continue;
			// the module died under the menu: this page goes, and the player is told
			ui::finish(ui::Result::Resume);
			ui::frame();
			[[fallthrough]];
		case ui::Result::Resume:
		case ui::Result::Restart:
			screen.release();
			return ended ? Outcome::Stopped : Outcome::Resume;
		case ui::Result::Switch:
			screen.release();
			return Outcome::Switch;
		case ui::Result::Quit:
			screen.release();
			return Outcome::Quit;
		}
	}
}

// A button still down reaches the game the moment it resumes, the one that chose Continue above all: the
// resume waits for them to be let go, for this long at most.
void waitReleased(int ms)
{
	const long long until = Child::now() + ms;
	while (pad >= 0)
	{
		unsigned char keys[KEY_MAX / 8 + 1] = {};
		input_absinfo x {}, y {};
		const bool key = ioctl(pad, EVIOCGKEY(sizeof(keys)), keys) >= 0
				&& std::any_of(keys, keys + sizeof(keys), [](unsigned char bits) { return bits != 0; });
		const bool hat = (ioctl(pad, EVIOCGABS(ABS_HAT0X), &x) >= 0 && x.value != 0)
				|| (ioctl(pad, EVIOCGABS(ABS_HAT0Y), &y) >= 0 && y.value != 0);
		if (!key && !hat)
			return;
		if (Child::now() >= until)
		{
			std::printf(DEADUI_APP_NAME ": host: a button is still down after %d ms: resuming with it down\n", ms);
			return;
		}
		pollfd f { pad, POLLIN, 0 };
		poll(&f, 1, 10);
		input_event ev;
		while (read(pad, &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {}
	}
}

// the module's run: the host asleep on the pad and the module's pipe until one of them has something
Outcome play(std::string& problem)
{
	long wakes = 0;
	unsigned long long cpuAtStart = cpuMicroseconds();
	bool ready = false;
	for (;;)
	{
		pollfd f[2] = { { pad, POLLIN, 0 }, { child.status(), POLLIN, 0 } };
		const int n = poll(f, 2, -1);
		if (stopAsked)
		{
			child.stop();
			return Outcome::Quit;
		}
		if (n < 0)
			continue;
		wakes++;
		if (f[1].revents & (POLLIN | POLLHUP))
		{
			std::vector<std::string> lines;
			const bool open = child.readLines(lines);
			if (!ready && std::find(lines.begin(), lines.end(), "ready") != lines.end())
			{
				ready = true;
				if (recomp::get(moduleId + ".fps", "off") == "on")
					sendCounter(true);
			}
			if (!open)
			{
				const std::string why = child.reap();
				if (why.empty())
				{
					std::printf(DEADUI_APP_NAME ": host: the module ended\n");
					std::fflush(stdout);
					return Outcome::Quit;
				}
				problem = moduleTitle + " stopped (" + why + ")";
				std::printf(DEADUI_APP_NAME ": host: the module stopped: %s\n", why.c_str());
				std::fflush(stdout);
				return Outcome::Stopped;
			}
		}
		bool menu = false;
		input_event ev;
		while (pad >= 0 && read(pad, &ev, sizeof(ev)) == (ssize_t)sizeof(ev))
			menu = menu || (ev.type == EV_KEY && ev.code == BTN_MODE && ev.value == 1);
		if (!menu)
			continue;
		if (!ready)
		{
			std::printf(DEADUI_APP_NAME ": host: MENU before the module is ready: ignored\n");
			std::fflush(stdout);
			continue;
		}

		std::printf(DEADUI_APP_NAME ": host: %ld wakes, %llu us of CPU, while the game played\n", wakes,
				cpuMicroseconds() - cpuAtStart);
		std::fflush(stdout);
		child.send("pause");
		if (!child.waitFor("paused", PauseWaitMs))
		{
			problem = moduleTitle + " did not respond";
			std::printf(DEADUI_APP_NAME ": host: the module did not pause in %d ms\n", PauseWaitMs);
			std::fflush(stdout);
			child.stopHung();
			return Outcome::Stopped;
		}
		std::vector<u8> frame;
		int width = 0, height = 0;
		if (!screen.picture(frame, width, height))
		{
			child.send("resume");
			continue;
		}
		std::printf(DEADUI_APP_NAME ": host: the module paused, the menu opens at %lld ms\n", Child::now());
		std::fflush(stdout);
		switch (menuLoop(frame, width, height, true, ""))
		{
		case Outcome::Resume:
			waitReleased(ReleaseWaitMs);
			child.send("resume");
			std::printf(DEADUI_APP_NAME ": host: resume sent at %lld ms\n", Child::now());
			std::fflush(stdout);
			wakes = 0;
			cpuAtStart = cpuMicroseconds();
			break;
		case Outcome::Stopped:
		{
			const std::string why = child.reap();
			problem = moduleTitle + " stopped" + (why.empty() ? "" : " (" + why + ")");
			std::printf(DEADUI_APP_NAME ": host: the module stopped under the menu: %s\n", why.c_str());
			std::fflush(stdout);
			return Outcome::Stopped;
		}
		case Outcome::Switch:
			child.stop();
			return Outcome::Switch;
		case Outcome::Quit:
			child.stop();
			return Outcome::Quit;
		}
	}
}

void stopHandler(int)
{
	stopAsked = 1;
}

}	// namespace

int run(recomp::Module module, const std::string& disc)
{
	signal(SIGPIPE, SIG_IGN);
	struct sigaction stop {};
	stop.sa_handler = stopHandler;
	sigaction(SIGTERM, &stop, nullptr);
	sigaction(SIGINT, &stop, nullptr);

	discPath = disc;
	const char *rom = std::getenv("DEADCAST_ROM");
	moduleTitle = module.title.empty() ? module.name : module.title;
	menuTitle = ui::gameName(rom != nullptr && *rom != '\0' ? rom : disc);
	moduleId = module.id;
	const std::string shown = disc.substr(disc.find_last_of('/') + 1);
	recomp::setRunning(module.id);
	pad = openPad();

	const std::string manifest = module.dir + "/module.cfg";
	for (;;)
	{
		std::string problem;
		Outcome outcome = Outcome::Stopped;
		if (!recomp::readManifest(manifest, module))
			problem = moduleTitle + " cannot be read";
		else if (module.host != recomp::HostFormat)
			problem = moduleTitle + " needs an update";
		else
		{
			ui::putPage("Loading...", shown);
			if (!child.start(module, disc, problem))
				problem = moduleTitle + " " + problem;
			else
				outcome = play(problem);
		}
		if (outcome == Outcome::Stopped)
		{
			std::printf(DEADUI_APP_NAME ": host: %s\n", problem.c_str());
			std::fflush(stdout);
			int width = 0, height = 0;
			if (!screen.begin(width, height))
				return 1;
			const std::vector<u8> black((size_t)width * height * 3, 0);
			outcome = menuLoop(black, width, height, false, problem);
			if (outcome == Outcome::Resume)
				continue;
		}
		if (outcome == Outcome::Switch)
		{
			ui::putPage("Loading...", shown);
			return recomp::SwitchExit;
		}
		ui::putPage("", "");
		return 0;
	}
}

}
