# Dead UI System

The menu, start page, recomp module store and module host shared by emulators for NextUI handhelds. Drawn on
the CPU, so that it needs nothing of the renderer. Recomp modules do not link it: the emulator hosts them (`src/host.h`).

## Use

Add this repository as a submodule, then in CMake:

```cmake
set(DEADUI_APP_NAME MyEmulator)     # shown to the player and in its log
set(DEADUI_HOME_ENV MYEMU_HOME)     # the environment variable naming its folder
set(DEADUI_LINK_PREFIX mylink)      # network interfaces that are not the internet
set(DEADUI_STORE_URL https://example.com/catalog.cfg)  # the list of recomp modules
set(DEADUI_CONSOLE dreamcast)       # options/<console>.cpp: the emulator's settings
add_subdirectory(ui)
target_link_libraries(myemulator PRIVATE deadui)
```

The host gives it a screen (`ui::setDisplay`), its buttons and its own pages (`ui.h`). The store's list is a text
file at `DEADUI_STORE_URL`, in the format `download.h` describes.

## Files

- `src/`: pages, drawing, the store, downloads, installed modules and the module host
- `options/`: the emulator's settings for each console
- `toolchain/`: the tg5040 cross toolchain (`toolchain.sh`, `devlibs.sh`, `tg5040.cmake`)
- `third_party/`: FreeType (submodule) and nlohmann/json

## Licences

MIT (`LICENSE`). Noto Sans Bold is SIL OFL 1.1 (`fonts/NotoSans-OFL.txt`), nlohmann/json is MIT and FreeType
is under its FreeType License.
