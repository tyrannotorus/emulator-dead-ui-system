// Recomp modules: what the store offers, and installing one in <home>/recomp/<id>/ (recomp.h). A file is trusted
// by the catalog's SHA-256, never by where it came from: anything else fails the check and is deleted. (The
// Brick has no CA certificates, so the connection is not verified; the checksum is what decides.)
#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace deadui::recomp
{

// a module the Recomp Store offers
struct StoreEntry
{
	std::string id;
	std::string name;
	std::string version;
	std::string url;			// its file
	uint64_t size = 0;			// its bytes
	std::string sha256;
	std::string title;			// what the menu calls it
	std::string info;			// its store.json
	std::string needs;			// the oldest version of the emulator that can run it, if any is too old
	std::vector<std::string> discs;
};

// What a module's page in the store says, from the store.json every module comes with. It travels in the
// catalog too, to be read before the module is downloaded.
struct ModuleInfo
{
	struct Format
	{
		std::string format;		// CHD, BIN/CUE, CDI...
		std::string region;
		std::string sha1;
	};
	std::string description;
	std::vector<Format> formats;			// the discs it plays
	std::vector<std::string> features;		// a line each
	std::vector<std::string> changelog;		// what changed in this version, a line each
	std::string repo;						// a link
};
// from store.json's text; whatever is missing or malformed stays empty
ModuleInfo readInfo(const std::string& json);

// What the store offers: the catalog the emulator published as it started, for itself and its modules. In a test,
// the file at DEADUI_TEST_CATALOG's address instead, if it can be fetched: it carries no signature, so nothing but
// a test may ever read one. As text: key = value lines, a module = line starting each entry, the other keys named
// as above (info = its store.json on one line), disc = once a disc.
void publishCatalog(const std::vector<StoreEntry>& entries);
// It may take a while: on a thread of its own, given up when cancel turns true.
std::vector<StoreEntry> storeCatalog(const std::atomic<bool>& cancel);

// fetched, checked, unpacked and installed, replacing the module installed before in one rename;
// done and total (bytes) as it goes, stopped when cancel turns true (until it is checked: then unpacking
// says so, and done and total count the archive unpacked). Empty when installed, else what went wrong.
std::string install(const StoreEntry& entry, std::atomic<uint64_t>& done, std::atomic<uint64_t>& total,
		const std::atomic<bool>& cancel, std::atomic<bool>& unpacking);

}
