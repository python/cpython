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

* cross-platform: works on Windows and Unix

* multi-window text editor with multiple undo, Python colorizing and many other
  features, e.g. smart indent and call tips

* Python shell window (a.k.a. interactive interpreter)

* debugger (not complete, but you can set breakpoints, view  and step)


Menus
-----


File menu
^^^^^^^^^

New window
   create a new editing window

Open...
   open an existing file

Open module...
   open an existing module (searches sys.path)

Class browser
   show classes and methods in current file

Path browser
   show sys.path directories, modules, classes and methods

.. index::
   single: Class browser
   single: Path browser

Save
   save current window to the associated file (unsaved windows have a \* before and
   after the window title)

Save As...
   save current window to new file, which becomes the associated file

Save Copy As...
   save current window to different file without changing the associated file

Close
   close current window (asks to save if unsaved)

Exit
   close all windows and quit IDLE (asks to save if unsaved)


Edit menu
^^^^^^^^^

Undo
   Undo last change to current window (max 1000 changes)

Redo
   Redo last undone change to current window

Cut
   Copy selection into system-wide clipboard; then delete selection

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

Indent region
   Shift selected lines right 4 spaces

Dedent region
   Shift selected lines left 4 spaces

Comment out region
   Insert ## in front of selected lines

Uncomment region
   Remove leading # or ## from selected lines

Tabify region
   Turns *leading* stretches of spaces into tabs

Untabify region
   Turn *all* tabs into the right number of spaces

Expand word
   Expand the word you have typed to match another word in the same buffer; repeat
   to get a different expansion

Format Paragraph
   Reformat the current blank-line-separated paragraph

Import module
   Import or reload the current module

Run script
   Execute the current file in the __main__ namespace

.. index::
   single: Import module
   single: Run script


Windows menu
^^^^^^^^^^^^

Zoom Height
   toggles the window between normal size (24x80) and maximum height.

The rest of this menu lists the names of all open windows; select one to bring
it to the foreground (deiconifying it if necessary).


Debug menu (in the Python Shell window only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Go to file/line
   look around the insert point for a filename and linenumber, open the file, and
   show the line.

Open stack viewer
   show the stack traceback of the last exception

Debugger toggle
   Run commands in the shell under the debugger

JIT Stack viewer toggle
   Open stack viewer on traceback

.. index::
   single: stack viewer
   single: debugger


Basic editing and navigation
----------------------------

* :kbd:`Backspace` deletes to the left; :kbd:`Del` deletes to the right

* Arrow keys and :kbd:`Page Up`/:kbd:`Page Down` to move around

* :kbd:`Home`/:kbd:`End` go to begin/end of line

* :kbd:`C-Home`/:kbd:`C-End` go to begin/end of file

* Some :program:`Emacs` bindings may also work, including :kbd:`C-B`,
  :kbd:`C-P`, :kbd:`C-A`, :kbd:`C-E`, :kbd:`C-D`, :kbd:`C-L`


Automatic indentation
^^^^^^^^^^^^^^^^^^^^^

After a block-opening statement, the next line is indented by 4 spaces (in the
Python Shell window by one tab).  After certain keywords (break, return etc.)
the next line is dedented.  In leading indentation, :kbd:`Backspace` deletes up
to 4 spaces if they are there. :kbd:`Tab` inserts 1-4 spaces (in the Python
Shell window one tab). See also the indent/dedent region commands in the edit
menu.


Python Shell window
^^^^^^^^^^^^^^^^^^^

* :kbd:`C-C` interrupts executing command

* :kbd:`C-D` sends end-of-file; closes window if typed at a ``>>>`` prompt

* :kbd:`Alt-p` retrieves previous command matching what you have typed

* :kbd:`Alt-n` retrieves next

* :kbd:`Return` while on any previous command retrieves that command

* :kbd:`Alt-/` (Expand word) is also useful here

.. index:: single: indentation


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
Idle first checks for ``IDLESTARTUP``; if ``IDLESTARTUP`` is present the file
referenced is run.  If ``IDLESTARTUP`` is not present, Idle checks for
``PYTHONSTARTUP``.  Files referenced by these environment variables are
convenient places to store functions that are used frequently from the Idle
shell, or for executing import statements to import common modules.

In addition, ``Tk`` also loads a startup file if it is present.  Note that the
Tk file is loaded unconditionally.  This additional file is ``.Idle.py`` and is
looked for in the user's home directory.  Statements in this file will be
executed in the Tk namespace, so this file is not useful for importing functions
to be used from Idle's Python shell.


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


