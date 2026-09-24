# Task: Build a Cross-Platform Text Editor

Create a complete, production-ready desktop text editor optimized for:

* Windows 10
* Ubuntu Linux
* Linux Mint

The application should be lightweight, responsive, suitable for editing large plain-text documents, and distributed as a standalone executable/application whenever technically possible.

The editor is primarily intended for plain-text editing with configurable real-time text highlighting.

## 1. General Requirements

The editor must:

* Support Windows 10, Ubuntu, and Linux Mint.
* Support UTF-8 text.
* Correctly display and edit English, Cyrillic, and other Unicode characters supported by the selected GUI framework and installed fonts.
* Use `.txt` as the default file extension when saving a new document.
* Support multiple simultaneously opened documents.
* Support Undo and Redo.
* Support drag-and-drop file opening.
* Have configurable Day and Night themes.
* Store persistent application settings in an INI file.
* Store the list of currently opened documents separately in `op_doc.ini`.
* Be designed with good performance in mind, including reasonably large text files and large highlighting configuration lists.

Choose an appropriate modern cross-platform framework and explain the choice before implementing the application.

Prefer a compiled/native or easily distributable solution with minimal runtime dependencies.

## 2. Block Highlighting

Implement configurable block highlighting.

The user must be able to configure it through:

`Settings -> Blocks Color`

Each rule consists of:

* A user-defined character sequence indicating the beginning of a block.
* A font (foreground) color associated with that sequence.

Display each rule approximately as:

`<block-start sequence>    <color icon>`

The color icon must visually display the currently selected color. Clicking it must open a color picker allowing the user to change the color.

A text block is defined as follows:

* A block starts when its text begins with one of the configured sequences.
* A block ends at an empty line.
* Therefore, blocks are separated by empty lines/paragraphs.
* The corresponding configured color is applied to the font of all text in the complete block.

`Blocks Color` must change only the text foreground color, not the text background. The editor's theme background must remain visible.

Block highlighting must update dynamically in real time while the document is being edited.

The `Blocks Color` configuration must not impose an artificial fixed limit on the number of rules. It should be limited only by practical system resources.

When leaving the `Blocks Color` configuration screen after making changes, ask the user whether the modified settings should be saved.

Persist the configuration in the application's INI settings.

## 3. Phrase Highlighting

Implement configurable phrase highlighting through:

`Settings -> Phrases Color`

The user must be able to define arbitrary text phrases, including phrases containing spaces.

Phrase matching must be case-insensitive.

For example, if the configured phrase is:

`Important Message`

all of the following must match:

`Important Message`

`important message`

`IMPORTANT MESSAGE`

`ImPoRtAnT MeSsAgE`

Each phrase has its own configurable font (foreground) color. `Phrases Color` must apply that color to the matched text without changing the text background.

Display rules approximately as:

`<phrase>    <color icon>`

The icon visually displays the selected color. Clicking it opens a color picker.

Phrase highlighting must update dynamically in real time while the user edits the document.

The number of phrase rules must not have an artificial fixed limit.

When leaving this configuration screen after changes, ask whether the settings should be saved.

Persist these settings in the application INI file.

## 4. Find Text and Search Highlighting

A `Find Text` area must always be visible on the right side of the main application interface.

It must contain at least:

* A text input field for the search query.
* Controls necessary for searching/navigating through matches.

All occurrences of the current search text must be highlighted dynamically in the document.

The search highlight color is configured through:

`Coloring Settings -> Find Color`

Display the current Find color using a graphical color icon. Clicking the icon opens a color picker.

The default Find Color must be:

`Red`

Changes must be persisted in the application's INI file.

When leaving the settings screen after changing this setting, ask whether the change should be saved.

## 5. Highlighting Priority

Highlighting may overlap. The following priority order must be respected:

1. Blocks Color — lowest priority
2. Phrases Color — medium priority
3. Find Color — highest priority

For example, if text belongs to a highlighted block and also matches a configured phrase, the phrase's font color must override the block's font color. Neither rule changes the text background.

If the same text also matches the current Find Text query, Find Color must override both Block and Phrase highlighting.

Implement the highlighting system so that these priorities are deterministic.

## 6. Highlighting Controls

The main menu/interface must contain three checkboxes allowing the user to independently enable or disable:

* Blocks Color
* Phrases Color
* Find Color

Changing these checkboxes must immediately update the displayed document without modifying the underlying text.

All three highlighting systems must operate dynamically in real time.

Highlighting is visual formatting only and must never modify the actual contents of the text file.

## 7. Editor Font

Provide:

`Settings -> Editor Font`

The user must be able to globally change the font used for editable text, including at least:

* Font family
* Font size
* Relevant supported font style/options

This functionality is particularly important for visually impaired users.

Persist the selected font configuration in the application INI file.

## 8. Zoom

Support:

`Ctrl + Mouse Wheel Up`

to increase the displayed editor font size, and:

`Ctrl + Mouse Wheel Down`

to decrease it.

This should behave as editor zoom/text scaling.

It should not modify the actual document.

Define reasonable minimum and maximum zoom/font sizes to prevent invalid UI states.

## 9. Status Bar

The bottom status/information bar must always display:

* Total number of characters in the current document
* Current line number
* Current position/column within the line

The information must update immediately when the cursor position or document content changes.

## 10. Drag and Drop

When a user drags a supported text file from the operating system onto the editor window, open that file for editing.

Handle invalid, missing, inaccessible, or unsupported files gracefully.

## 11. Open Documents Persistence

Maintain the list of opened documents in a separate file:

`op_doc.ini`

Whenever the relevant document state changes, keep this file synchronized with the currently opened files.

On application startup:

1. Read `op_doc.ini`.
2. Attempt to reopen every document that was open during the previous session.
3. If a document can no longer be opened — for example, because it was deleted, moved, or is inaccessible — do not open it.
4. Remove invalid entries from `op_doc.ini`.
5. Continue opening the remaining valid documents instead of failing application startup.

## 12. Unsaved Documents on Exit

When closing the application, detect all modified documents.

For every modified document, sequentially ask the user whether it should be saved.

The user must have normal choices such as:

* Save
* Don't Save
* Cancel

`Cancel` should abort application shutdown.

Do not silently discard modified documents.

## 13. Word Wrap

Enable Word Wrap when the application working window is smaller than the available screen/workspace.

The behavior should react appropriately when the window is resized.

Avoid horizontal scrolling when Word Wrap is active.

## 14. Day / Night Mode

Allow the user to change the application's appearance between:

* Day mode
* Night mode

The theme must affect the editor background and application/menu UI.

Ensure that normal text, selection, cursor, block highlights, phrase highlights, and search highlights remain readable in both themes.

Persist the selected theme.

## 15. Settings Persistence

Persist all settings from the Settings menus in an INI configuration file.

This includes, at minimum:

* Block highlighting rules and colors
* Phrase highlighting rules and colors
* Find Color
* Editor font
* Day/Night theme
* Highlight enable/disable states
* Other persistent editor preferences introduced during implementation

Use a platform-appropriate application configuration directory rather than assuming that the executable directory is writable.

The configuration file may use `.ini` or another unique application-specific extension if there is a technical reason for doing so.

## 16. Performance Requirements

The application should remain responsive during normal editing.

Real-time highlighting must not unnecessarily rescan and reformat the complete document after every individual keystroke when a more efficient incremental approach is possible.

Design the highlighting engine so that:

* Editing remains responsive.
* Large Blocks Color and Phrases Color rule sets are supported.
* Large documents remain practical to edit.
* Search highlighting updates efficiently.
* Highlighting does not alter the actual text.
* Highlighting priority is preserved.

If appropriate for the selected GUI framework, use incremental syntax/highlighting mechanisms instead of rebuilding rich-text formatting manually after every edit.

## 17. Application Packaging

The final application should be distributable as a single executable/application package as far as reasonably possible for each supported platform.

Provide build instructions for:

* Windows 10
* Ubuntu
* Linux Mint

The end user should not normally need to install a development environment.

If the application cannot start because a required operating-system component, library, runtime, font subsystem, or other dependency is missing, provide a useful error message explaining:

1. What component is missing.
2. Why it is required.
3. How the user can install it.

Do not fail with an unexplained technical exception whenever this can reasonably be avoided.

## 18. Reliability

Handle at least the following conditions gracefully:

* File does not exist.
* File was moved after the previous session.
* Permission denied.
* Invalid configuration file.
* Partially corrupted configuration file.
* Unsupported or malformed text encoding.
* Failure to save a document.
* Failure to write settings.
* Duplicate attempts to open the same file.
* Very long lines.
* Empty files.
* Empty highlighting rules.
* Overlapping phrase matches.
* Overlapping Block/Phrase/Find highlighting.

Never silently destroy user data.

Use safe file-writing techniques for configuration and important state files where practical, such as writing to a temporary file and atomically replacing the previous configuration.

## 19. UI Expectations

The interface should be simple and optimized for text editing rather than visually complex.

A reasonable layout is:

* Standard application menu at the top.
* Document tabs for multiple opened files.
* Main text editor occupying most of the window.
* Permanently visible `Find Text` area on the right.
* Status bar at the bottom.
* Quick checkboxes/toggles for Blocks, Phrases, and Find highlighting.

Settings dialogs for Blocks Color and Phrases Color should remain usable even with a very large number of configured rules. Use a scrollable/list-based interface rather than allowing the settings window to grow indefinitely.

## 20. Implementation Requirements

Do not provide only a prototype or pseudocode.

Create a complete buildable application.

Use a clean project structure and separate major responsibilities, including:

* Main application/window
* Document management
* File loading/saving
* Highlighting engine
* Block rules
* Phrase rules
* Find/search handling
* Settings management
* Session/open-document persistence
* Theme handling
* UI dialogs
* Application startup and error handling

Avoid putting the entire application into one source file.

Before writing the implementation, briefly describe:

1. The programming language and GUI framework you selected.
2. Why they are appropriate for Windows 10, Ubuntu, and Linux Mint.
3. How the real-time highlighting engine will work.
4. How highlighting priority will be implemented.
5. How settings and `op_doc.ini` will be stored.
6. How the application will be packaged for each operating system.

Then implement the complete project.

After implementation, provide:

* The complete source code.
* Project directory structure.
* Build configuration.
* Dependency information.
* Step-by-step build instructions for Windows.
* Step-by-step build instructions for Ubuntu/Linux Mint.
* Instructions for creating distributable binaries/packages.
* A short manual test checklist covering all requirements above.

Do not omit a requested feature merely to simplify the implementation. If a requirement has a technical ambiguity, choose a sensible implementation, document the decision, and keep the architecture flexible enough to change it later.
