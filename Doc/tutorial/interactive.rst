.. _tut-interacting:

**************************************************
Interactive Input Editing and History Substitution
**************************************************

Some versions of the Python interpreter support editing of the current input
line and history substitution, similar to facilities found in the Korn shell and
the GNU Bash shell.  This is implemented using the *GNU Readline* library, which
supports Emacs-style and vi-style editing.  This library has its own
documentation which I won't duplicate here; however, the basics are easily
explained.  The interactive editing and history described here are optionally
available in the Unix and Cygwin versions of the interpreter.

This chapter does *not* document the editing facilities of Mark Hammond's
PythonWin package or the Tk-based environment, IDLE, distributed with Python.
The command line history recall which operates within DOS boxes on NT and some
other DOS and Windows flavors  is yet another beast.


.. _tut-lineediting:

Line Editing
============

If supported, input line editing is active whenever the interpreter prints a
primary or secondary prompt.  The current line can be edited using the
conventional Emacs control characters.  The most important of these are:
:kbd:`C-A` (Control-A) moves the cursor to the beginning of the line, :kbd:`C-E`
to the end, :kbd:`C-B` moves it one position to the left, :kbd:`C-F` to the
right.  Backspace erases the character to the left of the cursor, :kbd:`C-D` the
character to its right. :kbd:`C-K` kills (erases) the rest of the line to the
right of the cursor, :kbd:`C-Y` yanks back the last killed string.
:kbd:`C-underscore` undoes the last change you made; it can be repeated for
cumulative effect.


.. _tut-history:

History Substitution
====================

History substitution works as follows.  All non-empty input lines issued are
saved in a history buffer, and when a new prompt is given you are positioned on
a new line at the bottom of this buffer. :kbd:`C-P` moves one line up (back) in
the history buffer, :kbd:`C-N` moves one down.  Any line in the history buffer
can be edited; an asterisk appears in front of the prompt to mark a line as
modified.  Pressing the :kbd:`Return` key passes the current line to the
interpreter.  :kbd:`C-R` starts an incremental reverse search; :kbd:`C-S` starts
a forward search.


.. _tut-keybindings:

Key Bindings
============

The key bindings and some other parameters of the Readline library can be
customized by placing commands in an initialization file called
:file:`~/.inputrc`.  Key bindings have the form ::

   key-name: function-name

or ::

   "string": function-name

and options can be set with ::

   set option-name value

For example::

   # I prefer vi-style editing:
   set editing-mode vi

   # Edit using a single line:
   set horizontal-scroll-mode On

   # Rebind some keys:
   Meta-h: backward-kill-word
   "\C-u": universal-argument
   "\C-x\C-r": re-read-init-file

Note that the default binding for :kbd:`Tab` in Python is to insert a :kbd:`Tab`
character instead of Readline's default filename completion function.  If you
insist, you can override this by putting ::

   Tab: complete

in your :file:`~/.inputrc`.  (Of course, this makes it harder to type indented
continuation lines if you're accustomed to using :kbd:`Tab` for that purpose.)

.. index::
   module: rlcompleter
   module: readline

Automatic completion of variable and module names is optionally available.  To
enable it in the interpreter's interactive mode, add the following to your
startup file: [#]_  ::

   import rlcompleter, readline
   readline.parse_and_bind('tab: complete')

This binds the :kbd:`Tab` key to the completion function, so hitting the
:kbd:`Tab` key twice suggests completions; it looks at Python statement names,
the current local variables, and the available module names.  For dotted
expressions such as ``string.a``, it will evaluate the expression up to the
final ``'.'`` and then suggest completions from the attributes of the resulting
object.  Note that this may execute application-defined code if an object with a
:meth:`__getattr__` method is part of the expression.

A more capable startup file might look like this example.  Note that this
deletes the names it creates once they are no longer needed; this is done since
the startup file is executed in the same namespace as the interactive commands,
and removing the names avoids creating side effects in the interactive
environment.  You may find it convenient to keep some of the imported modules,
such as :mod:`os`, which turn out to be needed in most sessions with the
interpreter. ::

   # Add auto-completion and a stored history file of commands to your Python
   # interactive interpreter. Requires Python 2.0+, readline. Autocomplete is
   # bound to the Esc key by default (you can change it - see readline docs).
   #
   # Store the file in ~/.pystartup, and set an environment variable to point
   # to it:  "export PYTHONSTARTUP=/max/home/itamar/.pystartup" in bash.
   #
   # Note that PYTHONSTARTUP does *not* expand "~", so you have to put in the
   # full path to your home directory.

   import atexit
   import os
   import readline
   import rlcompleter

   historyPath = os.path.expanduser("~/.pyhistory")

   def save_history(historyPath=historyPath):
       import readline
       readline.write_history_file(historyPath)

   if os.path.exists(historyPath):
       readline.read_history_file(historyPath)

   atexit.register(save_history)
   del os, atexit, readline, rlcompleter, save_history, historyPath


.. _tut-commentary:

Commentary
==========

This facility is an enormous step forward compared to earlier versions of the
interpreter; however, some wishes are left: It would be nice if the proper
indentation were suggested on continuation lines (the parser knows if an indent
token is required next).  The completion mechanism might use the interpreter's
symbol table.  A command to check (or even suggest) matching parentheses,
quotes, etc., would also be useful.

.. %
   Do we mention IPython? DUBOIS

.. rubric:: Footnotes

.. [#] Python will execute the contents of a file identified by the
   :envvar:`PYTHONSTARTUP` environment variable when you start an interactive
   interpreter.

