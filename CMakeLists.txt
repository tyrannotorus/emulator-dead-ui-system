# deadui: an emulator's menu, start page and recomp module store, drawn on the CPU, built into the emulator and
# into each of its recomp modules. The parent project sets, before add_subdirectory():
#   DEADUI_APP_NAME     the emulator's name, shown to the player and in its log
#   DEADUI_HOME_ENV     the environment variable naming the emulator's folder
#   DEADUI_LINK_PREFIX  the network interfaces that are not a connection to the internet
#   DEADUI_STORE_URL    the address of the list of recomp modules
#   DEADUI_CONSOLE      options/<console>.cpp: Emulator Options' settings
# and finds or leaves to this file CURL, ZLIB and freetype.
foreach(setting DEADUI_APP_NAME DEADUI_HOME_ENV DEADUI_LINK_PREFIX DEADUI_STORE_URL DEADUI_CONSOLE)
    if(NOT ${setting})
        message(FATAL_ERROR "deadui: ${setting} is not set")
    endif()
endforeach()

add_library(deadui STATIC
        src/canvas.cpp
        src/canvas.h
        src/download.cpp
        src/download.h
        src/host.cpp
        src/host.h
        src/options.cpp
        src/options.h
        src/recomp.cpp
        src/recomp.h
        src/sha256.cpp
        src/sha256.h
        src/ui.cpp
        src/ui.h
        options/${DEADUI_CONSOLE}.cpp)
target_include_directories(deadui PUBLIC src)
target_include_directories(deadui PRIVATE third_party/json)
target_compile_features(deadui PUBLIC cxx_std_17)
target_compile_definitions(deadui PRIVATE
        DEADUI_APP_NAME="${DEADUI_APP_NAME}"
        DEADUI_HOME_ENV="${DEADUI_HOME_ENV}"
        DEADUI_LINK_PREFIX="${DEADUI_LINK_PREFIX}"
        DEADUI_STORE_URL="${DEADUI_STORE_URL}")

if(NOT TARGET CURL::libcurl)
    find_package(CURL REQUIRED)
endif()
if(NOT TARGET ZLIB::ZLIB)
    find_package(ZLIB REQUIRED)
endif()
if(NOT TARGET freetype)
    add_subdirectory(third_party/freetype ${CMAKE_CURRENT_BINARY_DIR}/freetype EXCLUDE_FROM_ALL)
endif()
target_link_libraries(deadui PRIVATE freetype CURL::libcurl ZLIB::ZLIB)

# the menu's font, in the library itself
set(DEADUI_FONT_ZIP ${CMAKE_CURRENT_SOURCE_DIR}/fonts/NotoSans-Bold.ttf.zip)
set(DEADUI_FONT ${CMAKE_CURRENT_BINARY_DIR}/font/NotoSans-Bold.ttf)
if(NOT EXISTS ${DEADUI_FONT} OR ${DEADUI_FONT_ZIP} IS_NEWER_THAN ${DEADUI_FONT})
    file(ARCHIVE_EXTRACT INPUT ${DEADUI_FONT_ZIP} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/font)
endif()
target_compile_definitions(deadui PRIVATE DEADUI_FONT="${DEADUI_FONT}")
set_source_files_properties(src/ui.cpp PROPERTIES OBJECT_DEPENDS ${DEADUI_FONT})
