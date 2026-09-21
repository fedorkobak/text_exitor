# Text Editor

C++17 / Qt Widgets text editor for Windows 10, Ubuntu, and Linux Mint. The
existing Qt 5 path and Windows 7 target definitions are retained. Qt Widgets
provides native desktop editing, Unicode, accessibility, font dialogs, and
incremental text layout with the same application code on both platforms.

**The current feature update is source-only. It has not been compiled or run.**
Existing executables under `build/` and `dist/` predate this update. The commands
below are instructions for the next build, not a record of completed validation.

## Features and behavior

- Multiple document tabs; New, Open, Save, Save As, close-tab, Undo/Redo,
  clipboard operations, drag-and-drop, and UTF-8 text with optional BOM.
- New files default to `.txt`. Existing extensions are preserved. Loading
  rejects malformed UTF-8, binary NUL bytes, inaccessible paths, and directories.
- Saves use atomic replacement. The original remains intact after a failed
  save. Existing symlinks are followed. Saving a document whose on-disk content
  changed prompts before overwriting it. Duplicate paths/aliases select the
  already-open tab; Save As cannot overwrite another open tab.
- **Settings → Blocks Color**: editable, scrollable table of prefix/color rules.
  A paragraph begins at the start of the document or after a blank line. Only
  its first line is tested. Prefixes are case-sensitive; the longest wins,
  with the later rule winning ties. Empty or whitespace-only lines end blocks.
- **Settings → Phrases Color**: case-insensitive literal phrase rules, including
  spaces, English, Cyrillic, and supplementary Unicode characters. Matching
  uses Unicode simple case folding, independent of locale. Rules match within
  a logical line; embedded line breaks are rejected. Accents are not normalized.
  All overlapping matches are considered; the later rule wins overlap colors.
- **Find Text** stays on the right. Typing highlights all occurrences in the
  active document. Enter/F3 moves forward, Shift+F3 moves backward, with wrap.
  Ctrl+F focuses the query. Search ignores case and supports overlapping matches.
- **Coloring Settings → Find Color** defaults to red. Color swatches open a
  picker. Closing a changed block/phrase/find-color dialog asks Save, Discard,
  or Cancel. Save failures keep the coloring dialog available for retry.
- Highlight priority is tree-sitter syntax, then **Blocks < Phrases < Find**.
  Configured colors are backgrounds; black/white foregrounds are selected for
  readability. The selection uses the theme selection color. Three independent
  menu/toolbar toggles turn Blocks, Phrases, and Find coloring on/off.
  Visual formatting never enters saved text or the Undo history.
- **Settings → Editor Font** changes family, size, and styles across all tabs.
  Ctrl+mouse-wheel zoom is global and persistent, with a 6–96 point range.
- **Settings → Appearance** selects Day/Night and customizes each theme's
  editor, window/menu, and selection colors. Text and syntax colors adapt for
  contrast. Font availability still depends on the operating system.
- The permanent status bar shows Unicode code-point count, line, and column.
  Each newline counts as one character; a surrogate pair counts as one;
  combining marks count separately. Columns count text characters, not visual
  tab stops. Updating statistics recounts affected lines rather than the document.
- **View → Word Wrap** offers Automatic (default), Always, and Never. Automatic
  wraps restored/smaller windows and disables wrapping when maximized/fullscreen.
  Wrapped editors have no horizontal scrollbar.
- Optional tree-sitter highlighting remains available for C/C++ and JSON via
  extension detection or the Language menu. Other files default to plain text.

## Settings and sessions

Configuration uses Qt's writable `AppConfigLocation`, not the executable folder.
With the application's organization/name, the usual locations are:

- Linux: `~/.config/TextEditor/TextEditor/` (or under `$XDG_CONFIG_HOME`).
- Windows: `%LOCALAPPDATA%\TextEditor\TextEditor\`.

`settings.ini` stores coloring rules, all coloring toggles, font/zoom, theme,
theme colors, wrap preference, window geometry, and search-panel width.
`op_doc.ini` separately stores opened file paths, order, and the active path.
Both use explicit INI format and temporary-file/atomic-replacement writes.

Startup reopens valid session files, skips missing/unreadable/invalid files,
reports skipped entries, and rewrites the cleaned session list. Opening,
saving, reordering, switching, and closing tabs synchronize this list. Application
shutdown retains the open list for the next launch. Modified tabs are prompted
sequentially; Cancel aborts exit without closing any tabs.

Untitled buffers and unsaved edits are not crash-recovery snapshots; save them
to disk when prompted. This version should be used as a single running instance
per configuration directory; concurrent instances share settings/session files.

Invalid setting values use defaults with a warning. Corrupt INI files are backed
up as `*.invalid-<unique-id>` before replacement; if the backup cannot be made,
writes to that file are disabled for the run. Rules with empty text or invalid
colors are skipped and reported. Write failures are visible and exit offers
Retry, Cancel, or an explicit close-without-saving-settings option.

## Structure

```text
CMakeLists.txt                  Qt 5/6 targets, pinned parsers, tests, installation
src/
  main.cpp, startup.*           Application startup and font diagnostics
  window.*                      Menus, document tabs/lifecycle, preferences UI
  editor.*                      Editable document, Unicode statistics, zoom/drop
  fileio.*                      UTF-8 validation, newline/BOM handling, safe saves
  highlighter.*                 Incremental visual layers and tree-sitter adapter
  rules.*                       Prefix trie and shared phrase matching automaton
  searchpanel.*                 Permanent query panel and match navigation
  settings.*                    Validated INI settings and separate session store
  theme.*                       Day/Night application palettes
  dialogs.*                     Rule table model and color/theme dialogs
tests/editor_tests.cpp          File, highlight, session, settings, and UI tests
docs/manual-tests.md            Acceptance checklist for the next build
packaging/                     Desktop file, icon, and Linux runtime diagnostics
Dockerfile.windows             Existing static MXE/Qt 5 Windows build
Dockerfile.linux-appimage       Existing Qt 5 AppImage build plus startup diagnostics
scripts/                       Docker build/export commands
```

Qt's [QSyntaxHighlighter](https://doc.qt.io/archives/qt-5.15/qsyntaxhighlighter.html)
handles edited text blocks and propagates paragraph state as necessary. Prefix
rules use a trie; phrase rules use a shared Aho–Corasick matcher, scanning each
line in proportion to its length plus the number of matches. A configuration or
query change refreshes blocks in scheduled batches. Tables use a model/view
implementation, without one widget per rule or a fixed rule-count limit.

Qt can still spend time laying out a very long line or propagating a change
through a very long paragraph. File loading/saving is synchronous, and the
editor keeps the document in memory. It is suitable for ordinary and reasonably
large text files, not multi-gigabyte streaming logs. Inputs exceeding the Qt 5
text-buffer capacity check (approximately 1 GiB of file data) are rejected before
allocation; available memory can impose a lower practical limit.

Tree-sitter parsing is debounced by 120 ms, uses UTF-16 positions, and has a
50 ms parse timeout plus a 25 ms traversal budget. Syntax parsing is paused
above two million UTF-16 code units (shown in the status bar); configurable
block/phrase/find highlighting remains active. The optional syntax adapter
currently reparses its bounded input; normal plain-text edits never perform
a whole-document parse or copy for highlighting.

## Build on Ubuntu / Linux Mint

1. Install CMake 3.16+, Ninja, C/C++17 tools, and Qt development packages:

   ```sh
   sudo apt install build-essential cmake ninja-build qt6-base-dev
   ```

   On systems using Qt 5, install `qtbase5-dev` and add
   `-DEDITOR_QT_MAJOR=5` to the configure command. Qt 5.12+ is supported by the
   source; the AppImage container uses Ubuntu 20.04's Qt 5.12.8.

2. Configure and build:

   ```sh
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel 2
   ```

3. Run the tests and application:

   ```sh
   ctest --test-dir build --output-on-failure
   ./build/bin/text_editor
   ./build/bin/text_editor README.md src/main.cpp
   ```

The native `build/bin/text_editor` requires its matching Qt runtime installed.
Use the AppImage for distribution without requiring Qt packages on other PCs.

## Create the portable Linux application

1. Install/start Docker and give your account permission to use its daemon.
2. From the repository run:

   ```sh
   ./scripts/build-linux-portable.sh
   ```

3. Copy `dist/linux-portable/TextEditor-x86_64.AppImage` to the destination and run:

   ```sh
   chmod +x TextEditor-x86_64.AppImage
   ./TextEditor-x86_64.AppImage
   ```

This preserves the existing AppImage approach: Qt, its XCB plugin, tree-sitter,
and supporting libraries are bundled in one file. It still requires compatible
Linux system libraries, a desktop display, graphics drivers, and fonts. It is
not a fully static Linux executable. Wayland desktops need XWayland for this
XCB package. Docker caches build dependencies. No developer tools are needed
on the destination computer.

If FUSE is unavailable, try:

```sh
./TextEditor-x86_64.AppImage --appimage-extract-and-run
```

The added AppRun hook checks loader dependencies and provides Ubuntu/Mint
runtime installation guidance before Qt starts. It writes to stderr and also
opens an error dialog when `zenity` is available. The application checks for
missing fonts. Errors that happen before AppRun executes (such as a damaged
AppImage, an unsupported CPU, or a missing shell) must be diagnosed by the OS.

## Windows build and distribution

The static Windows build principle is unchanged. From Linux with Docker:

```sh
./scripts/build-windows-docker.sh
```

This uses the archived MXE Qt 5.15.2 / GCC 5.5 / MinGW-w64 8.0 toolchain and
produces `dist/windows/bin/text_editor.exe`. Qt and tree-sitter are statically
linked; Windows system DLLs are still required. Keep the license notices in
`dist/windows/share` when distributing. The target remains Windows 7+
(`_WIN32_WINNT=0x0601`); test on Windows 10 and separately on Windows 7 if you
need that legacy compatibility. No new Windows runtime testing has been done.

To build natively on Windows 10 using a Qt SDK:

1. Install CMake, Ninja, Qt 5.15.2, and its matching MinGW 8.1 64-bit kit.
2. Open PowerShell in this project and adapt the SDK paths below.
3. Configure, compile, test, install, and deploy the runtime:

   ```powershell
   $env:PATH = "C:\Qt\5.15.2\mingw81_64\bin;C:\Qt\Tools\mingw810_64\bin;" + $env:PATH
   cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DEDITOR_QT_MAJOR=5 -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/mingw81_64
   cmake --build build-win --parallel 2
   ctest --test-dir build-win --output-on-failure
   cmake --install build-win --prefix dist/native-windows
   windeployqt --release --compiler-runtime --no-translations dist/native-windows/bin/text_editor.exe
   .\dist\native-windows\bin\text_editor.exe
   ```

The ordinary Qt SDK route creates a dynamically linked application: distribute
the whole `dist/native-windows` folder, including DLLs and `platforms`. Use the
existing Docker/MXE route for the single static executable. Qt 6 is also
selectable with a matching SDK/compiler but does not target Windows 7. A
missing Windows system DLL is reported by Windows before this application can
run; use a supported Windows installation and its normal system repair/update
tools. Missing Qt DLLs in a native SDK build mean deployment is incomplete.

## Dependency and validation notes

CMake downloads checksum-pinned tree-sitter 0.20.8, tree-sitter-cpp 0.20.3,
and tree-sitter-json 0.20.2 on the first configuration. No tree-sitter CLI,
Rust, Node.js, or system parser package is required. Subsequent builds reuse
the sources. Offline builds can set `FETCHCONTENT_SOURCE_DIR_TS`,
`FETCHCONTENT_SOURCE_DIR_CPP`, and `FETCHCONTENT_SOURCE_DIR_JSON` to prepared
source directories. Disable the Qt Test dependency with `-DBUILD_TESTING=OFF`.

UTF-8 BOM and the dominant LF/CRLF/CR convention are retained. Mixed line
endings normalize to that convention when saved. File change detection checks
content immediately before an overwrite prompt; this is not cross-process
file locking and cannot eliminate an external writer racing a save.

Parser licenses are installed under `share/text_editor/licenses`. Preserve Qt
and dependency notices and comply with the licenses for the selected build,
including the obligations applicable to static Qt distribution.

Tests use temporary configuration directories, independent of the real user
session. They cover Unicode/file round-trips, invalid input, highlight priority,
overlaps, block propagation, Unicode statistics, large rule lists, settings
validation, failed writes, session pruning, search, and unsaved-close behavior.
See [the manual checklist](docs/manual-tests.md) for platform and GUI acceptance
checks that must follow compilation.
