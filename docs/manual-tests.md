# Manual acceptance checklist

The source update has not been built or executed. Run this checklist after the
next build on Windows 10, Ubuntu, and Linux Mint. Test Windows 7 separately if
the retained legacy target is needed. Use temporary files and configuration.

- [ ] Launch the packaged application on a PC without Qt/developer tools.
      On Linux test both X11 and Wayland with XWayland. Confirm the right-side
      Find Text panel and bottom statistics remain visible on resize.
- [ ] Open English, Cyrillic, emoji, combining marks, an empty UTF-8 file,
      UTF-8 BOM, LF, CRLF, and CR samples. Edit, undo, redo, and save. Inspect
      saved bytes for BOM/newline preservation and absence of highlight markup.
- [ ] Create an untitled file and Save without an extension: verify `.txt`.
      Save As with a chosen extension; test overwrite confirmation and cancellation.
- [ ] Open several files, reorder tabs, and attempt duplicate opens using relative
      paths, symlinks, and Windows path case variants. Confirm one tab per file.
      Attempt Save As onto another open tab; it must refuse.
- [ ] Drop one and multiple files on the editor, tab area, and window. Test
      missing paths, directories, unreadable files, malformed UTF-8, and binary
      content. Errors must not replace or destroy the current document.
- [ ] Add Blocks Color rules `N` (yellow), `NOTE` (green), and another `NOTE`
      (blue). Verify longest-prefix matching and the later-rule tie break.
      Continuation lines stay colored until an empty/whitespace-only line.
      A matching prefix inside an already-started paragraph must not restart it.
- [ ] Edit/remove a paragraph prefix or separator, then undo/redo. Verify
      downstream block colors update until the next separator.
- [ ] Add phrases `Important Message`, `ВАЖНО`, `aba`, and `ba` in distinct
      colors. Check all case variants, spaces, Cyrillic, emoji offsets, and
      overlapping occurrences in `ababa`. The later phrase wins overlaps.
- [ ] Try empty and multiline rules; verify validation. Add many thousands of
      rules and scroll/edit/remove selections without the dialog growing.
- [ ] Click color swatches. Leave each changed block, phrase, Find Color, and
      theme dialog using Close, Esc, and the title-bar close button. Exercise
      Save, Discard, and Cancel. Unchanged dialogs should close without prompting.
- [ ] Search for a phrase also matched by a block and phrase rule. Verify
      **Blocks < Phrases < Find**. Find defaults to red. Change its color and
      confirm it survives restart. Toggle each highlight system independently;
      file content, modified state, and Undo history must be unaffected.
- [ ] Enter/change/clear a Find query; check all occurrences, case-insensitive
      matches, overlaps, no matches, next/previous, wraparound, Ctrl+F, F3, and
      Shift+F3. Switch tabs and confirm the current query highlights the new tab.
- [ ] Select Day/Night and customize backgrounds, UI, and selection colors.
      Check menus, dialogs, normal text, syntax text, caret, selected text, and
      every highlight type remain readable. Restart to check persistence.
- [ ] Change editor font family, size, bold, and italic. Check all current and
      newly opened tabs. Use Ctrl+wheel both directions, including at 6 and
      96 points; regular wheel scrolling must still work. Restart to verify font.
- [ ] Check character totals and cursor line/column after typing, deleting,
      paste, replacing the whole document, and undo/redo. Emoji count once,
      newline once, combining marks separately; counts must update immediately.
- [ ] Resize, maximize, restore, and move between screens. Test Automatic,
      Always, and Never wrap. No horizontal scrollbar should appear when wrapping.
- [ ] Restart with several open files and verify order/active tab. Delete/move
      one file externally and restart: valid files reopen, errors are reported,
      invalid entries disappear from `op_doc.ini`. Closing a tab removes its path.
- [ ] Modify several tabs; exit with Save, Don't Save, and Cancel in sequence.
      Cancel on a later tab must retain every tab. Cancel a Save As dialog and
      induce a failed save; both must abort closing. Explicitly discarded edits
      must not return on restart; the underlying saved files should reopen.
- [ ] Make the configuration directory unwritable. Change coloring/font/theme
      settings and close the application. Confirm visible errors, retained
      configuration, retry/cancel options, and no silent document loss.
- [ ] Corrupt part of `settings.ini` and `op_doc.ini`; test invalid colors,
      font sizes, booleans, empty rules, and a huge declared array size. Valid
      data should load where possible; malformed files should be backed up or
      writes disabled if backup fails. Verify a useful warning.
- [ ] Change/delete a loaded file externally and Save. Confirm overwrite
      requires explicit approval. Induce disk-full/permission failures using a
      disposable test filesystem; previous file bytes and unsaved tab must remain.
- [ ] Edit a representative large text file, a long paragraph, and a single
      very long line with many rules. Check typing, scrolling, query changes,
      and rule changes; record practical limits. Larger files may take time to
      load/layout but ordinary edits should only rehighlight affected blocks.
- [ ] Open C/C++ and JSON, test syntax updates and custom color priority. For
      input above the syntax-size threshold, verify the status note and continued
      block/phrase/search highlighting and editing.
- [ ] In a disposable Linux environment remove a required runtime/font component
      or unset the display. Verify the package's dependency guidance and font
      warnings. Test AppImage extract-and-run when FUSE is unavailable.

Also run `ctest --test-dir build --output-on-failure` and the corresponding
Windows test build. If Qt atomic-save tests fail only inside a restricted
runner, check whether its filesystem permits linking unnamed temporary files;
do not replace atomic saves with unsafe direct overwrites to accommodate it.
