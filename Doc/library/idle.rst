.. _idle:

IDLE
====

.. index::
   single: IDLE
   single: Python Editor
   single: Integrated Development Environment

.. moduleauthor:: Guido van Rossum <guido@Python.org>

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

IDLE has two main window types, the Shell window and the Editor window.  It is
possible to have multiple editor windows simultaneously.  Output windows, such
as used for Edit / Find in Files, are a subtype of edit window.  They currently
have the same top menu as Editor windows but a different default title and
context menu.

IDLE's menus dynamically change based on which window is currently selected.
Each menu documented below indicates which window type it is associated with.
Click on the dotted line at the top of a menu to "tear it off": a separate
window containing the menu is created (for Unix and Windows only).

File menu (Shell and Editor)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

New File
   Create a new file editing window.

Open...
   Open an existing file with an Open dialog.

Recent Files
   Open a list of recent files.  Click one to open it.

Open Module...
   Open an existing module (searches sys.path).

.. index::
   single: Class browser
   single: Path browser

Class Browser
   Show functions, classes, and methods in the current Editor file in a
   tree structure.  In the shell, open a module first.

Path Browser
   Show sys.path directories, modules, functions, classes and methods in a
   tree structure.

Save
   Save the current window to the associated file, if there is one.  Windows
   that have been changed since being opened or last saved have a \* before
   and after the window title.  If there is no associated file,
   do Save As instead.

Save As...
   Save the current window with a Save As dialog.  The file saved becomes the
   new associated file for the window.

Save Copy As...
   Save the current window to different file without changing the associated
   file.

Print Window
   Print the current window to the default printer.

Close
   Close the current window (ask to save if unsaved).

Exit
   Close all windows and quit IDLE (ask to save unsaved windows).

Edit menu (Shell and Editor)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Undo
   Undo the last change to the current window.  A maximum of 1000 changes may
   be undone.

Redo
   Redo the last undone change to the current window.

Cut
   Copy selection into the system-wide clipboard; then delete the selection.

Copy
   Copy selection into the system-wide clipboard.

Paste
   Insert contents of the system-wide clipboard into the current window.

The clipboard functions are also available in context menus.

Select All
   Select the entire contents of the current window.

Find...
   Open a search dialog with many options

Find Again
   Repeat the last search, if there is one.

Find Selection
   Search for the currently selected string, if there is one.

Find in Files...
   Open a file search dialog.  Put results in an new output window.

Replace...
   Open a search-and-replace dialog.

Go to Line
   Move cursor to the line number requested and make that line visible.

Show Completions
   Open a scrollable list allowing selection of keywords and attributes. See
   Completions in the Tips sections below.

Expand Word
   Expand a prefix you have typed to match a full word in the same window;
   repeat to get a different expansion.

Show call tip
   After an unclosed parenthesis for a function, open a small window with
   function parameter hints.

Show surrounding parens
   Highlight the surrounding parenthesis.

Format menu (Editor window only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Indent Region
   Shift selected lines right by the indent width (default 4 spaces).

Dedent Region
   Shift selected lines left by the indent width (default 4 spaces).

Comment Out Region
   Insert ## in front of selected lines.

Uncomment Region
   Remove leading # or ## from selected lines.

Tabify Region
   Turn *leading* stretches of spaces into tabs. (Note: We recommend using
   4 space blocks to indent Python code.)

Untabify Region
   Turn *all* tabs into the correct number of spaces.

Toggle Tabs
   Open a dialog to switch between indenting with spaces and tabs.

New Indent Width
   Open a dialog to change indent width. The accepted default by the Python
   community is 4 spaces.

Format Paragraph
   Reformat the current blank-line-delimited paragraph in comment block or
   multiline string or selected line in a string.  All lines in the
   paragraph will be formatted to less than N columns, where N defaults to 72.

Strip trailing whitespace
   Remove any space characters after the last non-space character of a line.

.. index::
   single: Run script

Run menu (Editor window only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Python Shell
   Open or wake up the Python Shell window.

Check Module
   Check the syntax of the module currently open in the Editor window. If the
   module has not been saved IDLE will either prompt the user to save or
   autosave, as selected in the General tab of the Idle Settings dialog.  If
   there is a syntax error, the approximate location is indicated in the
   Editor window.

Run Module
   Do Check Module (above).  If no error, restart the shell to clean the
   environment, then execute the module.

Shell menu (Shell window only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

View Last Restart
  Scroll the shell window to the last Shell restart.

Restart Shell
  Restart the shell to clean the environment.

Debug menu (Shell window only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Go to File/Line
   Look on the current line. with the cursor, and the line above for a filename
   and line number.  If found, open the file if not already open, and show the
   line.  Use this to view source lines referenced in an exception traceback
   and lines found by Find in Files. Also available in the context menu of
   the Shell window and Output windows.

.. index::
   single: debugger
   single: stack viewer

Debugger (toggle)
   When actived, code entered in the Shell or run from an Editor will run
   under the debugger.  In the Editor, breakpoints can be set with the context
   menu.  This feature is still incomplete and somewhat experimental.

Stack Viewer
   Show the stack traceback of the last exception in a tree widget, with
   access to locals and globals.

Auto-open Stack Viewer
   Toggle automatically opening the stack viewer on an unhandled exception.

Options menu (Shell and Editor)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Configure IDLE
   Open a configuration dialog.  Fonts, indentation, keybindings, and color
   themes may be altered.  Startup Preferences may be set, and additional
   help sources can be specified.  Non-default user setting are saved in a
   .idlerc directory in the user's home directory.  Problems caused by bad user
   configuration files are solved by editing or deleting one or more of the
   files in .idlerc.

Configure Extensions
   Open a configuration dialog for setting preferences for extensions
   (discussed below).  See note above about the location of user settings.

Code Context (toggle)(Editor Window only)
   Open a pane at the top of the edit window which shows the block context
   of the code which has scrolled above the top of the window.

Window menu (Shell and Editor)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Zoom Height
   Toggles the window between normal size and maximum height. The initial size
   defaults to 40 lines by 80 chars unless changed on the General tab of the
   Configure IDLE dialog.

The rest of this menu lists the names of all open windows; select one to bring
it to the foreground (deiconifying it if necessary).

Help menu (Shell and Editor)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

About IDLE
   Display version, copyright, license, credits, and more.

IDLE Help
   Display a help file for IDLE detailing the menu options, basic editing and
   navigation, and other tips.

Python Docs
   Access local Python documentation, if installed, or start a web browser
   and open docs.python.org showing the latest Python documentation.

Turtle Demo
   Run the turtledemo module with example python code and turtle drawings.

Additional help sources may be added here with the Configure IDLE dialog under
the General tab.

.. index::
   single: Cut
   single: Copy
   single: Paste
   single: Set Breakpoint
   single: Clear Breakpoint
   single: breakpoints

Context Menus
^^^^^^^^^^^^^^^^^^^^^^^^^^

Open a context menu by right-clicking in a window (Control-click on OS X).
Context menus have the standard clipboard functions also on the Edit menu.

Cut
   Copy selection into the system-wide clipboard; then delete the selection.

Copy
   Copy selection into the system-wide clipboard.

Paste
   Insert contents of the system-wide clipboard into the current window.

Editor windows also have breakpoint functions.  Lines with a breakpoint set are
specially marked.  Breakpoints only have an effect when running under the
debugger.  Breakpoints for a file are saved in the user's .idlerc directory.

Set Breakpoint
   Set a breakpoint on the current line.

Clear Breakpoint
   Clear the breakpoint on that line.

Shell and Output windows have the following.

Go to file/line
   Same as in Debug menu.


Editing and navigation
----------------------

In this section, 'C' refers to the Control key on Windows and Unix and
the Command key on Mac OSX.

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
executed in the Tk namespace, so this file is not useful for importing
functions to be used from IDLE's Python shell.


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
   ``sys.argv[1:...]``  and ``sys.argv[0]`` set to the script name.  If the
   script name is '-', no script is executed but an interactive Python session
   is started;    the arguments are still available in ``sys.argv``.

Running without a subprocess
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If IDLE is started with the -n command line switch it will run in a
single process and will not create the subprocess which runs the RPC
Python execution server.  This can be useful if Python cannot create
the subprocess or the RPC socket interface on your platform.  However,
in this mode user code is not isolated from IDLE itself.  Also, the
environment is not restarted when Run/Run Module (F5) is selected.  If
your code has been modified, you must reload() the affected modules and
re-import any specific items (e.g. from foo import baz) if the changes
are to take effect.  For these reasons, it is preferable to run IDLE
with the default subprocess if at all possible.

.. deprecated:: 3.4


Help and preferences
--------------------

Additional help sources
^^^^^^^^^^^^^^^^^^^^^^^

IDLE includes a help menu entry called "Python Docs" that will open the
extensive sources of help, including tutorials, available at docs.python.org.
Selected URLs can be added or removed from the help menu at any time using the
Configure IDLE dialog. See the IDLE help option in the help menu of IDLE for
more information.


Setting preferences
^^^^^^^^^^^^^^^^^^^

The font preferences, highlighting, keys, and general preferences can be
changed via Configure IDLE on the Option menu.  Keys can be user defined;
IDLE ships with four built in key sets. In addition a user can create a
custom key set in the Configure IDLE dialog under the keys tab.


Extensions
^^^^^^^^^^

IDLE contains an extension facility.  Peferences for extensions can be
changed with Configure Extensions. See the beginning of config-extensions.def
in the idlelib directory for further information.  The default extensions
are currently:

* FormatParagraph

* AutoExpand

* ZoomHeight

* ScriptBinding

* CallTips

* ParenMatch

* AutoComplete

* CodeContext

* RstripExtension
