EXPERIMENTAL LOADER IDLE 2000-05-29
-----------------------------------

   David Scherer  <dscherer@cmu.edu>

This is a modification of the CVS version of IDLE 0.5, updated as of
2000-03-09.  It is alpha software and might be unstable.  If it breaks,
you get to keep both pieces.

If you have problems or suggestions, you should either contact me or
post to the list at http://www.python.org/mailman/listinfo/idle-dev
(making it clear that you are using this modified version of IDLE).

Changes:

  The ExecBinding module, a replacement for ScriptBinding, executes
  programs in a separate process, piping standard I/O through an RPC
  mechanism to an OnDemandOutputWindow in IDLE.  It supports executing
  unnamed programs (through a temporary file).  It does not yet support
  debugging.

  When running programs with ExecBinding, tracebacks will be clipped
  to exclude system modules.  If, however, a system module calls back
  into the user program, that part of the traceback will be shown.

  The OnDemandOutputWindow class has been improved.  In particular,
  it now supports a readline() function used to implement user input,
  and a scroll_clear() operation which is used to hide the output of
  a previous run by scrolling it out of the window.

  Startup behavior has been changed.  By default IDLE starts up with
  just a blank editor window, rather than an interactive window.  Opening
  a file in such a blank window replaces the (nonexistent) contents of
  that window instead of creating another window.  Because of the need to
  have a well-known port for the ExecBinding protocol, only one copy of
  IDLE can be running.  Additional invocations use the RPC mechanism to
  report their command line arguments to the copy already running.

  The menus have been reorganized.  In particular, the excessively large
  'edit' menu has been split up into 'edit', 'format', and 'run'.

  'Python Documentation' now works on Windows, if the win32api module is
  present.

  A few key bindings have been changed: F1 now loads Python Documentation
  instead of the IDLE help; shift-TAB is now a synonym for unindent.

New modules:
  ExecBinding.py         Executes program through loader
  loader.py              Bootstraps user program
  protocol.py            RPC protocol
  Remote.py              User-process interpreter
  spawn.py               OS-specific code to start programs

Files modified:
  autoindent.py          ( bindings tweaked )
  bindings.py            ( menus reorganized )
  config.txt             ( execbinding enabled )
  editorwindow.py        ( new menus, fixed 'Python Documentation' )
  filelist.py            ( hook for "open in same window" )
  formatparagraph.py     ( bindings tweaked )
  idle.bat               ( removed absolute pathname )
  idle.pyw               ( weird bug due to import with same name? )
  iobinding.py           ( open in same window, EOL convention )
  keydefs.py             ( bindings tweaked )
  outputwindow.py        ( readline, scroll_clear, etc )
  pyshell.py             ( changed startup behavior )
  readme.txt             ( <Recursion on file with id=1234567> )

IDLE 0.5 - February 2000
------------------------

This is an early release of IDLE, my own attempt at a Tkinter-based
IDE for Python.

For news about this release, see the file NEWS.txt.  (For a more
detailed change log, see the file ChangeLog.)

FEATURES

IDLE has the following features:

- coded in 100% pure Python, using the Tkinter GUI toolkit (i.e. Tcl/Tk)

- cross-platform: works on Windows and Unix (on the Mac, there are
currently problems with Tcl/Tk)

- multi-window text editor with multiple undo, Python colorizing
and many other features, e.g. smart indent and call tips

- Python shell window (a.k.a. interactive interpreter)

- debugger (not complete, but you can set breakpoints, view  and step)

USAGE

The main program is in the file "idle.py"; on Unix, you should be able
to run it by typing "./idle.py" to your shell.  On Windows, you can
run it by double-clicking it; you can use idle.pyw to avoid popping up
a DOS console.  If you want to pass command line arguments on Windows,
use the batch file idle.bat.

Command line arguments: files passed on the command line are executed,
not opened for editing, unless you give the -e command line option.
Try "./idle.py -h" to see other command line options.

IDLE requires Python 1.5.2, so it is currently only usable with a
Python 1.5.2 distribution.  (An older version of IDLE is distributed
with Python 1.5.2; you can drop this version on top of it.)

COPYRIGHT

IDLE is covered by the standard Python copyright notice
(http://www.python.org/doc/Copyright.html).

FEEDBACK

(removed, since Guido probably doesn't want complaints about my
changes)

--Guido van Rossum (home page: http://www.python.org/~guido/)
