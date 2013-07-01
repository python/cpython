.. _idle:

IDLE
====

.. moduleauthor:: Guido van Rossum <guido@Python.org>

.. index::
   single: IDLE
   single: Python Editor
   single: Integrated Development Environment

IDLE is the Python IDE built with the :mod:`tkinter` GUI toolkit.

IDLE has the following features:

* coded in 100% pure Python, using the :mod:`tkinter` GUI toolkit

* cross-platform: works on Windows, Unix, and Mac OS X

* multi-window text editor with multiple undo, Python colorizing,
  smart indent, call tips, and many other features

* Python shell window (a.k.a. interactive interpreter)

* debugger (not complete, but you can set breakpoints, view and step)


Menus
-----

IDLE has two window types, the Shell window and the Editor window. It is
possible to have multiple editor windows simultaneously. IDLE's
menus dynamically change based on which window is currently selected. Each menu
documented below indicates which window type it is associated with. Click on
the dotted line at the top of a menu to "tear it off": a separate window
containing the menu is created (for Unix and Windows only).

File menu (Shell and Editor)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

New file
   Create a new file editing window

Open...
   Open an existing file

Open module...
   Open an existing module (searches sys.path)

Recent Files
   Open a list of recent files

Class browser
   Show classes and methods in current file

Path browser
   Show sys.path directories, modules, classes and methods

.. index::
   single: Class browser
   single: Path browser

Save
   Save current window to the associated file (unsaved windows have a
   \* before and after the window title)

Save As...
   Save current window to new file, which becomes the associated file

Save Copy As...
   Save current window to different file without changing the associated file

Print Window
   Print the current window

Close
   Close current window (asks to save if unsaved)

Exit
   Close all windows and quit IDLE (asks to save if unsaved)


Edit menu (Shell and Editor)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Undo
   Undo last change to current window (a maximum of 1000 changes may be undone)

Redo
   Redo last undone change to current window

Cut
   Copy selection into system-wide clipboard; then delete the selection

Copy
   Copy selection into system-wide clipboard

Paste
   Insert system-wide clipboard into window

Select All
   Select the entire contents of the edit buffer

Find...
   Open a search dialog box with many options

Find again
   Repeat last search

Find selection
   Search for the string in the selection

Find in Files...
   Open a search dialog box for searching files

Replace...
   Open a search-and-replace dialog box

Go to line
   Ask for a line number and show that line

Expand word
   Expand the word you have typed to match another word in the same buffer;
   repeat to get a different expansion

Show call tip
   After an unclosed parenthesis for a function, open a small window with
   function parameter hints

Show surrounding parens
   Highlight the surrounding parenthesis

Show Completions
   Open a scroll window allowing selection keywords and attributes. See
   Completions below.


Format menu (Editor window only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Indent region
   Shift selected lines right by the indent width (default 4 spaces)

Dedent region
   Shift selected lines left by the indent width (default 4 spaces)

Comment out region
   Insert ## in front of selected lines

Uncomment region
   Remove leading # or ## from selected lines

Tabify region
   Turns *leading* stretches of spaces into tabs. (Note: We recommend using
   4 space blocks to indent Python code.)

Untabify region
   Turn *all* tabs into the correct number of spaces

Toggle tabs
   Open a dialog to switch between indenting with spaces and tabs.

New Indent Width
   Open a dialog to change indent width. The accepted default by the Python
   community is 4 spaces.

Format Paragraph
   Reformat the current blank-line-separated paragraph. All lines in the
   paragraph will be formatted to less than 80 columns.

Strip trailing whitespace
   Removes any space characters after the end of the last non-space character

.. index::
   single: Import module
   single: Run script


Run menu (Editor window only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Python Shell
   Open or wake up the Python Shell window

Check module
   Check the syntax of the module currently open in the Editor window. If the
   module has not been saved IDLE will prompt the user to save the code.

Run module
   Restart the shell to clean the environment, then execute the currently
   open module.  If the module has not been saved IDLE will prompt the user
   to save the code.

Shell menu (Shell window only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

View Last Restart
  Scroll the shell window to the last Shell restart

Restart Shell
  Restart the shell to clean the environment

Debug menu (Shell window only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Go to file/line
   Look around the insert point for a filename and line number, open the file,
   and show the line.  Useful to view the source lines referenced in an
   exception traceback. Available in the context menu of the Shell window.

Debugger (toggle)
   This feature is not complete and considered experimental. Run commands in
   the shell under the debugger

Stack viewer
   Show the stack traceback of the last exception

Auto-open Stack Viewer
   Toggle automatically opening the stack viewer on unhandled exception

.. index::
   single: stack viewer
   single: debugger

Options menu (Shell and Editor)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Configure IDLE
   Open a configuration dialog.  Fonts, indentation, keybindings, and color
   themes may be altered.  Startup Preferences may be set, and additional
   help sources can be specified.

Code Context (toggle)(Editor Window only)
   Open a pane at the top of the edit window which shows the block context
   of the section of code which is scrolling off the top of the window.

Windows menu (Shell and Editor)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Zoom Height
   Toggles the window between normal size (40x80 initial setting) and maximum
   height. The initial size is in the Configure IDLE dialog under the general
   tab.

The rest of this menu lists the names of all open windows; select one to bring
it to the foreground (deiconifying it if necessary).

Help menu (Shell and Editor)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

About IDLE
   Version, copyright, license, credits

IDLE Help
   Display a help file for IDLE detailing the menu options, basic editing and
   navigation, and other tips.

Python Docs
   Access local Python documentation, if installed. Or will start a web browser
   and open docs.python.org showing the latest Python documentation.

Additional help sources may be added here with the Configure IDLE dialog under
the General tab.

Editor Window context menu
^^^^^^^^^^^^^^^^^^^^^^^^^^

* Right-click in Editor window (Control-click on OS X)

Cut
   Copy selection into system-wide clipboard; then delete selection

Copy
   Copy selection into system-wide clipboard

Paste
   Insert system-wide clipboard into window

Set Breakpoint
   Sets a breakpoint.  Breakpoints are only enabled when the debugger is open.

Clear Breakpoint
   Clears the breakpoint on that line.

.. index::
   single: Cut
   single: Copy
   single: Paste
   single: Set Breakpoint
   single: Clear Breakpoint
   single: breakpoints


Shell Window context menu
^^^^^^^^^^^^^^^^^^^^^^^^^

* Right-click in Python Shell window (Control-click on OS X)

Cut
   Copy selection into system-wide clipboard; then delete selection

Copy
   Copy selection into system-wide clipboard

Paste
   Insert system-wide clipboard into window

Go to file/line
   Same as in Debug menu.


Editing and navigation
----------------------

* :kbd:`Backspace` deletes to the left; :kbd:`Del` deletes to the right

* :kbd:`C-Backspace` delete word left; :kbd:`C-Del` delete word to the right

* Arrow keys and :kbd:`Page Up`/:kbd:`Page Down` to move around

* :kbd:`C-LeftArrow` and :kbd:`C-RightArrow` moves by words

* :kbd:`Home`/:kbd:`End` go to begin/end of line

* :kbd:`C-Home`/:kbd:`C-End` go to begin/end of file

* Some useful Emacs bindings are inherited from Tcl/Tk:

   * :kbd:`C-a` beginning of line

   * :kbd:`C-e` end of line

   * :kbd:`C-k` kill line (but doesn't put it in clipboard)

   * :kbd:`C-l` center window around the insertion point

   * :kbd:`C-b` go backwards one character without deleting (usually you can
     also use the cursor key for this)

   * :kbd:`C-f` go forward one character without deleting (usually you can
     also use the cursor key for this)

   * :kbd:`C-p` go up one line (usually you can also use the cursor key for
     this)

   * :kbd:`C-d` delete next character

Standard keybindings (like :kbd:`C-c` to copy and :kbd:`C-v` to paste)
may work.  Keybindings are selected in the Configure IDLE dialog.


Automatic indentation
^^^^^^^^^^^^^^^^^^^^^

After a block-opening statement, the next line is indented by 4 spaces (in the
Python Shell window by one tab).  After certain keywords (break, return etc.)
the next line is dedented.  In leading indentation, :kbd:`Backspace` deletes up
to 4 spaces if they are there. :kbd:`Tab` inserts spaces (in the Python
Shell window one tab), number depends on Indent width. Currently tabs
are restricted to four spaces due to Tcl/Tk limitations.

See also the indent/dedent region commands in the edit menu.

Completions
^^^^^^^^^^^

Completions are supplied for functions, classes, and attributes of classes,
both built-in and user-defined. Completions are also provided for
filenames.

The AutoCompleteWindow (ACW) will open after a predefined delay (default is
two seconds) after a '.' or (in a string) an os.sep is typed. If after one
of those characters (plus zero or more other characters) a tab is typed
the ACW will open immediately if a possible continuation is found.

If there is only one possible completion for the characters entered, a
:kbd:`Tab` will supply that completion without opening the ACW.

'Show Completions' will force open a completions window, by default the
:kbd:`C-space` will open a completions window. In an empty
string, this will contain the files in the current directory. On a
blank line, it will contain the built-in and user-defined functions and
classes in the current name spaces, plus any modules imported. If some
characters have been entered, the ACW will attempt to be more specific.

If a string of characters is typed, the ACW selection will jump to the
entry most closely matching those characters.  Entering a :kbd:`tab` will
cause the longest non-ambiguous match to be entered in the Editor window or
Shell.  Two :kbd:`tab` in a row will supply the current ACW selection, as
will return or a double click.  Cursor keys, Page Up/Down, mouse selection,
and the scroll wheel all operate on the ACW.

"Hidden" attributes can be accessed by typing the beginning of hidden
name after a '.', e.g. '_'. This allows access to modules with
``__all__`` set, or to class-private attributes.

Completions and the 'Expand Word' facility can save a lot of typing!

Completions are currently limited to those in the namespaces. Names in
an Editor window which are not via ``__main__`` and :data:`sys.modules` will
not be found.  Run the module once with your imports to correct this situation.
Note that IDLE itself places quite a few modules in sys.modules, so
much can be found by default, e.g. the re module.

If you don't like the ACW popping up unbidden, simply make the delay
longer or disable the extension.  Or another option is the delay could
be set to zero. Another alternative to preventing ACW popups is to
disable the call tips extension.

Python Shell window
^^^^^^^^^^^^^^^^^^^

* :kbd:`C-c` interrupts executing command

* :kbd:`C-d` sends end-of-file; closes window if typed at a ``>>>`` prompt
  (this is :kbd:`C-z` on Windows).

* :kbd:`Alt-/` (Expand word) is also useful to reduce typing

  Command history

  * :kbd:`Alt-p` retrieves previous command matching what you have typed. On
    OS X use :kbd:`C-p`.

  * :kbd:`Alt-n` retrieves next. On OS X use :kbd:`C-n`.

  * :kbd:`Return` while on any previous command retrieves that command


Syntax colors
-------------

The coloring is applied in a background "thread," so you may occasionally see
uncolorized text.  To change the color scheme, edit the ``[Colors]`` section in
:file:`config.txt`.

Python syntax colors:
   Keywords
      orange

   Strings
      green

   Comments
      red

   Definitions
      blue

Shell colors:
   Console output
      brown

   stdout
      blue

   stderr
      dark green

   stdin
      black


Startup
-------

Upon startup with the ``-s`` option, IDLE will execute the file referenced by
the environment variables :envvar:`IDLESTARTUP` or :envvar:`PYTHONSTARTUP`.
IDLE first checks for ``IDLESTARTUP``; if ``IDLESTARTUP`` is present the file
referenced is run.  If ``IDLESTARTUP`` is not present, IDLE checks for
``PYTHONSTARTUP``.  Files referenced by these environment variables are
convenient places to store functions that are used frequently from the IDLE
shell, or for executing import statements to import common modules.

In addition, ``Tk`` also loads a startup file if it is present.  Note that the
Tk file is loaded unconditionally.  This additional file is ``.Idle.py`` and is
looked for in the user's home directory.  Statements in this file will be
executed in the Tk namespace, so this file is not useful for importing functions
to be used from IDLE's Python shell.


Command line usage
^^^^^^^^^^^^^^^^^^

::

   idle.py [-c command] [-d] [-e] [-s] [-t title] [arg] ...

   -c command  run this command
   -d          enable debugger
   -e          edit mode; arguments are files to be edited
   -s          run $IDLESTARTUP or $PYTHONSTARTUP first
   -t title    set title of shell window

If there are arguments:

#. If ``-e`` is used, arguments are files opened for editing and
   ``sys.argv`` reflects the arguments passed to IDLE itself.

#. Otherwise, if ``-c`` is used, all arguments are placed in
   ``sys.argv[1:...]``, with ``sys.argv[0]`` set to ``'-c'``.

#. Otherwise, if neither ``-e`` nor ``-c`` is used, the first
   argument is a script which is executed with the remaining arguments in
   ``sys.argv[1:...]``  and ``sys.argv[0]`` set to the script name.  If the script
   name is '-', no script is executed but an interactive Python session is started;
   the arguments are still available in ``sys.argv``.


Additional help sources
-----------------------

IDLE includes a help menu entry called "Python Docs" that will open the
extensive sources of help, including tutorials, available at docs.python.org.
Selected URLs can be added or removed from the help menu at any time using the
Configure IDLE dialog. See the IDLE help option in the help menu of IDLE for
more information.


Other preferences
-----------------

The font preferences, highlighting, keys, and general preferences can be
changed via the Configure IDLE menu option.  Be sure to note that
keys can be user defined, IDLE ships with four built in key sets. In
addition a user can create a custom key set in the Configure IDLE dialog
under the keys tab.

Extensions
----------

IDLE contains an extension facility.  See the beginning of
config-extensions.def in the idlelib directory for further information.  The
default extensions are currently:

* FormatParagraph

* AutoExpand

* ZoomHeight

* ScriptBinding

* CallTips

* ParenMatch

* AutoComplete

* CodeContext

