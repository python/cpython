- Issue #15348: Stop the debugger engine (normally in a user process)
  before closing the debugger window (running in the IDLE process).
  This prevents the RuntimeErrors that were being caught and ignored.

- Issue #24455: Prevent IDLE from hanging when a) closing the shell while the
  debugger is active (15347); b) closing the debugger with the [X] button
  (15348); and c) activating the debugger when already active (24455).
  The patch by Mark Roseman does this by making two changes.
  1. Suspend and resume the gui.interaction method with the tcl vwait
  mechanism intended for this purpose (instead of root.mainloop & .quit).
  2. In gui.run, allow any existing interaction to terminate first.

- Change 'The program' to 'Your program' in an IDLE 'kill program?' message
  to make it clearer that the program referred to is the currently running
  user program, not IDLE itself.

- Issue #24750: Improve the appearance of the IDLE editor window status bar.
  Patch by Mark Roseman.

- Issue #25313: Change the handling of new built-in text color themes to better
  address the compatibility problem introduced by the addition of IDLE Dark.
  Consistently use the revised idleConf.CurrentTheme everywhere in idlelib.

- Issue #24782: Extension configuration is now a tab in the IDLE Preferences
  dialog rather than a separate dialog.  The former tabs are now a sorted
  list.  Patch by Mark Roseman.

- Issue #22726: Re-activate the config dialog help button with some content
  about the other buttons and the new IDLE Dark theme.

- Issue #24820: IDLE now has an 'IDLE Dark' built-in text color theme.
  It is more or less IDLE Classic inverted, with a cobalt blue background.
  Strings, comments, keywords, ... are still green, red, orange, ... .
  To use it with IDLEs released before November 2015, hit the
  'Save as New Custom Theme' button and enter a new name,
  such as 'Custom Dark'.  The custom theme will work with any IDLE
  release, and can be modified.

- Issue #25224: README.txt is now an idlelib index for IDLE developers and
  curious users.  The previous user content is now in the IDLE doc chapter.
  'IDLE' now means 'Integrated Development and Learning Environment'.

- Issue #24820: Users can now set breakpoint colors in
  Settings -> Custom Highlighting.  Original patch by Mark Roseman.

- Issue #24972: Inactive selection background now matches active selection
  background, as configured by users, on all systems.  Found items are now
  always highlighted on Windows.  Initial patch by Mark Roseman.

- Issue #24570: Idle: make calltip and completion boxes appear on Macs
  affected by a tk regression.  Initial patch by Mark Roseman.

- Issue #24988: Idle ScrolledList context menus (used in debugger)
  now work on Mac Aqua.  Patch by Mark Roseman.

- Issue #24801: Make right-click for context menu work on Mac Aqua.
  Patch by Mark Roseman.

- Issue #25173: Associate tkinter messageboxes with a specific widget.
  For Mac OSX, make them a 'sheet'.  Patch by Mark Roseman.

- Issue #25198: Enhance the initial html viewer now used for Idle Help.
  * Properly indent fixed-pitch text (patch by Mark Roseman).
  * Give code snippet a very Sphinx-like light blueish-gray background.
  * Re-use initial width and height set by users for shell and editor.
  * When the Table of Contents (TOC) menu is used, put the section header
  at the top of the screen.

- Issue #25225: Condense and rewrite Idle doc section on text colors.

- Issue #21995: Explain some differences between IDLE and console Python.

- Issue #22820: Explain need for *print* when running file from Idle editor.

- Issue #25224: Doc: augment Idle feature list and no-subprocess section.

- Issue #25219: Update doc for Idle command line options.
  Some were missing and notes were not correct.

- Issue #24861: Most of idlelib is private and subject to change.
  Use idleib.idle.* to start Idle. See idlelib.__init__.__doc__.

- Issue #25199: Idle: add synchronization comments for future maintainers.

- Issue #16893: Replace help.txt with help.html for Idle doc display.
  The new idlelib/help.html is rstripped Doc/build/html/library/idle.html.
  It looks better than help.txt and will better document Idle as released.
  The tkinter html viewer that works for this file was written by Mark Roseman.
  The now unused EditorWindow.HelpDialog class and helt.txt file are deprecated.

- Issue #24199: Deprecate unused idlelib.idlever with possible removal in 3.6.

- Issue #24790: Remove extraneous code (which also create 2 & 3 conflicts).

