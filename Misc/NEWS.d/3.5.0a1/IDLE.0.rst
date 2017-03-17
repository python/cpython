- Issue #20577: Configuration of the max line length for the FormatParagraph
  extension has been moved from the General tab of the Idle preferences dialog
  to the FormatParagraph tab of the Config Extensions dialog.
  Patch by Tal Einat.

- Issue #16893: Update Idle doc chapter to match current Idle and add new
  information.

- Issue #3068: Add Idle extension configuration dialog to Options menu.
  Changes are written to HOME/.idlerc/config-extensions.cfg.
  Original patch by Tal Einat.

- Issue #16233: A module browser (File : Class Browser, Alt+C) requires an
  editor window with a filename.  When Class Browser is requested otherwise,
  from a shell, output window, or 'Untitled' editor, Idle no longer displays
  an error box.  It now pops up an Open Module box (Alt+M). If a valid name
  is entered and a module is opened, a corresponding browser is also opened.

- Issue #4832: Save As to type Python files automatically adds .py to the
  name you enter (even if your system does not display it).  Some systems
  automatically add .txt when type is Text files.

- Issue #21986: Code objects are not normally pickled by the pickle module.
  To match this, they are no longer pickled when running under Idle.

- Issue #17390: Adjust Editor window title; remove 'Python',
  move version to end.

- Issue #14105: Idle debugger breakpoints no longer disappear
  when inserting or deleting lines.

- Issue #17172: Turtledemo can now be run from Idle.
  Currently, the entry is on the Help menu, but it may move to Run.
  Patch by Ramchandra Apt and Lita Cho.

- Issue #21765: Add support for non-ascii identifiers to HyperParser.

- Issue #21940: Add unittest for WidgetRedirector. Initial patch by Saimadhav
  Heblikar.

- Issue #18592: Add unittest for SearchDialogBase. Patch by Phil Webster.

- Issue #21694: Add unittest for ParenMatch. Patch by Saimadhav Heblikar.

- Issue #21686: add unittest for HyperParser. Original patch by Saimadhav
  Heblikar.

- Issue #12387: Add missing upper(lower)case versions of default Windows key
  bindings for Idle so Caps Lock does not disable them. Patch by Roger Serwy.

- Issue #21695: Closing a Find-in-files output window while the search is
  still in progress no longer closes Idle.

- Issue #18910: Add unittest for textView. Patch by Phil Webster.

- Issue #18292: Add unittest for AutoExpand. Patch by Saihadhav Heblikar.

- Issue #18409: Add unittest for AutoComplete. Patch by Phil Webster.

- Issue #21477: htest.py - Improve framework, complete set of tests.
  Patches by Saimadhav Heblikar

- Issue #18104: Add idlelib/idle_test/htest.py with a few sample tests to begin
  consolidating and improving human-validated tests of Idle. Change other files
  as needed to work with htest.  Running the module as __main__ runs all tests.

- Issue #21139: Change default paragraph width to 72, the PEP 8 recommendation.

- Issue #21284: Paragraph reformat test passes after user changes reformat width.

- Issue #17654: Ensure IDLE menus are customized properly on OS X for
  non-framework builds and for all variants of Tk.

- Issue #23180: Rename IDLE "Windows" menu item to "Window".
  Patch by Al Sweigart.

