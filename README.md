# Text Editor

A small C++17 Qt Widgets editor: tabs, UTF-8 file loading, atomic Save/Save As,
unsaved-change prompts, undo/redo, search with wraparound, word wrap, and
tree-sitter syntax highlighting for C/C++ and JSON. Other files open as plain
text. Select a language manually with the Language menu.

## Linux

Dependencies: CMake 3.16+, a C/C++17 compiler, Qt 6 development packages
(or Qt 5.15), and optionally Ninja. On Ubuntu: `sudo apt install build-essential
cmake ninja-build qt6-base-dev`.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/bin/text_editor
# Optional: open files from the command line
./build/bin/text_editor README.md src/main.cpp
```

The executable is **build/bin/text_editor**. Qt and system shared libraries
are required on the destination Linux machine; this is not a static universal
Linux binary. Build on the oldest Linux distribution you intend to support.

### Portable Linux executable

Build a self-contained AppImage in Docker when the destination computer should
not need Qt packages installed:

```sh
./scripts/build-linux-portable.sh
./dist/linux-portable/TextEditor-x86_64.AppImage
```

The AppImage bundles Qt and its platform plugins. It still uses the destination
Linux kernel, glibc-compatible system interface, display server, graphics
drivers, fonts, and desktop services. It is therefore portable rather than a
fully static Linux binary. The container builds it on Ubuntu 20.04 with Qt 5 to
support a wider range of current x86-64 distributions.

## Windows 7 and newer

Use **Qt 5.15.2 with its matching MinGW 8.1 64-bit kit** to target Windows 7.
Qt 6 does not support Windows 7. A Qt 5 build can also be used on newer Windows,
but test it on each target OS. Qt 5 and Windows 7 are legacy platforms.
The code supports both Qt majors; this repository's Linux build uses Qt 6.

Build on a Windows development machine with CMake, Ninja, Qt 5.15.2, and its
matching MinGW compiler. Put that compiler and Qt's `bin` directory on PATH.
Use a CMake/Ninja version compatible with the host OS (building on Windows 10
for a Windows 7 target is preferable). In PowerShell:

```powershell
$env:PATH = "C:\Qt\5.15.2\mingw81_64\bin;C:\Qt\Tools\mingw810_64\bin;" + $env:PATH
cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DEDITOR_QT_MAJOR=5 -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/mingw81_64
cmake --build build-win --parallel 2
ctest --test-dir build-win --output-on-failure
cmake --install build-win --prefix dist
windeployqt --release --compiler-runtime --no-translations dist/bin/text_editor.exe
.\dist\bin\text_editor.exe
```

Distribute the entire **dist** folder, including Qt DLLs and the `platforms`
plugin directory; the `.exe` alone is not portable. Include the Qt license
notices and meet the license terms for the Qt distribution you use. Parser
license notices are installed under `dist/share/text_editor/licenses`.
The tree-sitter runtime and grammars are linked statically into the executable.
For a modern Windows-only Qt 6 build, select `-DEDITOR_QT_MAJOR=6` with a matching
Qt 6 compiler kit; that executable will not run on Windows 7.

Windows builds and Windows 7 runtime compatibility must be verified on Windows;
the Linux executable is not a Windows executable.

### Cross-build on Linux with Docker

Docker can build a self-contained 64-bit Windows executable with MXE's archived
prebuilt Qt 5.15.2, GCC 5.5, and MinGW-w64 8.0 toolchain for Ubuntu 20.04. The
first run downloads the toolchain packages; Docker caches that layer for
subsequent application builds.

```sh
./scripts/build-windows-docker.sh
```

The output is `dist/windows/bin/text_editor.exe`, with dependency license files
under `dist/windows/share`. This build links Qt statically. If you distribute it,
review and comply with the Qt license obligations for static linking. The PE
target is configured for Windows 7 (`_WIN32_WINNT=0x0601`), but it still needs a
real Windows 7 runtime test before compatibility can be guaranteed.

Qt's platform explanation: https://www.qt.io/blog/qt6-development-hosts-and-targets

## Dependencies and limitations

CMake downloads checksum-pinned tree-sitter 0.20.8, tree-sitter-cpp 0.20.3,
and tree-sitter-json 0.20.2 sources on first configuration. No tree-sitter CLI,
Rust, Node.js, or system tree-sitter library is needed. Later builds reuse the
downloaded sources. For offline builds, prepopulate CMake FetchContent sources
with `FETCHCONTENT_SOURCE_DIR_TS`, `FETCHCONTENT_SOURCE_DIR_CPP`, and
`FETCHCONTENT_SOURCE_DIR_JSON`.

Text is UTF-8; UTF-8 BOM and the detected LF/CRLF/CR convention are preserved.
Mixed line endings are normalized to the detected convention on save.
Invalid UTF-8 and binary files are rejected to avoid lossy conversion. Saving
uses QSaveFile's atomic replacement. External modifications are not monitored.

Highlighting traverses actual tree-sitter syntax nodes and reparses 120 ms
after edits. UTF-16 parsing keeps Qt and tree-sitter positions aligned, including
emoji. Parsing is limited to 50 ms per update; on timeout the document remains
editable without highlighting. Incremental parsing and configurable highlight
queries are not implemented. This is intended for ordinary text/source files,
not multi-gigabyte files.

Disable the Qt Test dependency with `-DBUILD_TESTING=OFF` if needed.
