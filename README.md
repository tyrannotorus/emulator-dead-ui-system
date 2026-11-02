# Dead UI System

The menu, start page and recomp module store shared by emulators for NextUI handhelds and their recomp
modules. Drawn on the CPU, so that it needs nothing of the host's renderer.

## Use

Add this repository as a submodule, then in CMake:

```cmake
set(DEADUI_APP_NAME MyEmulator)     # shown to the player and in its log
set(DEADUI_HOME_ENV MYEMU_HOME)     # the environment variable naming its folder
set(DEADUI_LINK_PREFIX mylink)      # network interfaces that are not the internet
set(DEADUI_RUN_MARKER /tmp/myemu-recomp-run)  # exists while a recomp module runs
set(DEADUI_CONSOLE dreamcast)       # options/<console>.cpp: Emulator Options' settings
add_subdirectory(ui)
target_link_libraries(myemulator PRIVATE deadui)
```

The host gives it a screen (`ui::setDisplay`), its buttons and its own pages (`ui.h`). The emulator hands the
store its catalog as it starts (`recomp::publishCatalog`, `download.h`).

## Files

- `src/`: pages, drawing, the store, downloads and installed modules
- `options/`: Emulator Options' settings for each console
- `toolchain/`: the tg5040 cross toolchain (`toolchain.sh`, `devlibs.sh`, `tg5040.cmake`)
- `third_party/`: FreeType (submodule) and nlohmann/json

## Licences

MIT (`LICENSE`). Noto Sans Bold is SIL OFL 1.1 (`fonts/NotoSans-OFL.txt`), nlohmann/json is MIT and FreeType
is under its FreeType License.
