IDLE 0.2 - 01/01/99
-------------------

This is a *very* early preliminary release of IDLE, my own attempt at
a Tkinter-based IDE for Python.  It has the following features:

- multi-window text editor with multiple undo and Python colorizing
- Python shell (a.k.a. interactive interpreter) window subclass
- debugger
- 100% pure Python
- works on Windows and Unix (probably works on Mac too)

The main program is in the file "idle"; on Windows you can use idle.pyw
to avoid popping up a DOS console.  Any arguments passed are interpreted
as files that will be opened for editing.

IDLE requires Python 1.5.2, so it is currently only usable with the
Python 1.5.2 beta distribution (luckily, IDLE is bundled with Python
1.5.2).

Please send feedback to the Python newsgroup, comp.lang.python.

--Guido van Rossum (home page: http://www.python.org/~guido/)

======================================================================

TO DO:

- "GO" command
- "Modularize" command
- command expansion from keywords, module contents, other buffers, etc.
- "Recent documents" menu item
- more emacsisms:
  - parentheses matching
  - reindent, reformat text etc.
  - M-[, M-] to move by paragraphs
  - smart stuff with whitespace around Return
  - filter region?
  - incremental search?
  - ^K should cut to buffer
  - command to fill text paragraphs
- restructure state sensitive code to avoid testing flags all the time
- finish debugger
- object browser instead of current stack viewer
- persistent user state (e.g. window and cursor positions, bindings)
- make backups when saving
- check file mtimes at various points
- interface with RCS/CVS/Perforce ???
- status bar?
- better help?
- don't open second class browser on same module

Details:

- when there's a selection, left/right arrow should go to either
  end of the selection
- ^O (on Unix -- open-line) should honor autoindent
- after paste, show end of pasted text
- on Windows, should turn short filename to long filename (not only in argv!)
  (shouldn't this be done -- or undone -- by ntpath.normpath?)

Structural problems:

- too much knowledge in FileList about EditorWindow (for example)
- Several occurrences of scrollable listbox with title and certain
  behavior; should create base class to generalize this

======================================================================

Comparison to PTUI
------------------

+ PTUI has a status line

+ PTUI's help is better (HTML!)

+ PTUI can attach a shell to any module

+ PTUI's auto indent is better
  (understands that "if a: # blah, blah" opens a block)

+ IDLE requires 4x backspace to dedent a line

+ PTUI has more bells and whistles:
  open multiple
  append
  modularize
  examine
  go

? PTUI's fontify is faster but synchronous (and still too slow);
  does a lousy job if editing affects lines below

- PTUI's shell is worse:
  no coloring;
  no editing of multi-line commands;
  ^P seems to permanently remove some text from the buffer

- PTUI's undo is worse:
  no redo;
  one char at a time

- PTUI's GUI is a tad ugly:
  I don't like the multiple buffers in one window model;
  I don't like the big buttons at the top of the widow

- PTUI lacks an integrated debugger

- PTUI lacks a class browser

- PTUI lacks many of IDLE's features:
  - expand word
  - regular expression search
  - search files (grep)

======================================================================

Notes after trying to run Grail
-------------------------------

- Grail does stuff to sys.path based on sys.argv[0]; you must set
sys.argv[0] to something decent first (it is normally set to the path of
the idle script).

- Grail must be exec'ed in __main__ because that's imported by some
other parts of Grail.

- Grail uses a module called History and so does idle :-(

======================================================================
