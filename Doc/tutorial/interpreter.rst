.. _tut-using:

****************************
Using the Python Interpreter
****************************


.. _tut-invoking:

Invoking the Interpreter
========================

The Python interpreter is usually installed as :file:`/usr/local/bin/python` on
those machines where it is available; putting :file:`/usr/local/bin` in your
Unix shell's search path makes it possible to start it by typing the command ::

   python

to the shell.  Since the choice of the directory where the interpreter lives is
an installation option, other places are possible; check with your local Python
guru or system administrator.  (E.g., :file:`/usr/local/python` is a popular
alternative location.)

On Windows machines, the Python installation is usually placed in
:file:`C:\\Python26`, though you can change this when you're running the
installer.  To add this directory to your path,  you can type the following
command into the command prompt in a DOS box::

   set path=%path%;C:\python26

Typing an end-of-file character (:kbd:`Control-D` on Unix, :kbd:`Control-Z` on
Windows) at the primary prompt causes the interpreter to exit with a zero exit
status.  If that doesn't work, you can exit the interpreter by typing the
following commands: ``import sys; sys.exit()``.

The interpreter's line-editing features usually aren't very sophisticated.  On
Unix, whoever installed the interpreter may have enabled support for the GNU
readline library, which adds more elaborate interactive editing and history
features. Perhaps the quickest check to see whether command line editing is
supported is typing Control-P to the first Python prompt you get.  If it beeps,
you have command line editing; see Appendix :ref:`tut-interacting` for an
introduction to the keys.  If nothing appears to happen, or if ``^P`` is echoed,
command line editing isn't available; you'll only be able to use backspace to
remove characters from the current line.

The interpreter operates somewhat like the Unix shell: when called with standard
input connected to a tty device, it reads and executes commands interactively;
when called with a file name argument or with a file as standard input, it reads
and executes a *script* from that file.

A second way of starting the interpreter is ``python -c command [arg] ...``,
which executes the statement(s) in *command*, analogous to the shell's
:option:`-c` option.  Since Python statements often contain spaces or other
characters that are special to the shell, it is best to quote  *command* in its
entirety with double quotes.

Some Python modules are also useful as scripts.  These can be invoked using
``python -m module [arg] ...``, which executes the source file for *module* as
if you had spelled out its full name on the command line.

Note that there is a difference between ``python file`` and ``python <file``.
In the latter case, input requests from the program, such as calls to
:func:`input` and :func:`raw_input`, are satisfied from *file*.  Since this file
has already been read until the end by the parser before the program starts
executing, the program will encounter end-of-file immediately.  In the former
case (which is usually what you want) they are satisfied from whatever file or
device is connected to standard input of the Python interpreter.

When a script file is used, it is sometimes useful to be able to run the script
and enter interactive mode afterwards.  This can be done by passing :option:`-i`
before the script.  (This does not work if the script is read from standard
input, for the same reason as explained in the previous paragraph.)


.. _tut-argpassing:

Argument Passing
----------------

When known to the interpreter, the script name and additional arguments
thereafter are passed to the script in the variable ``sys.argv``, which is a
list of strings.  Its length is at least one; when no script and no arguments
are given, ``sys.argv[0]`` is an empty string.  When the script name is given as
``'-'`` (meaning  standard input), ``sys.argv[0]`` is set to ``'-'``.  When
:option:`-c` *command* is used, ``sys.argv[0]`` is set to ``'-c'``.  When
:option:`-m` *module* is used, ``sys.argv[0]``  is set to the full name of the
located module.  Options found after  :option:`-c` *command* or :option:`-m`
*module* are not consumed  by the Python interpreter's option processing but
left in ``sys.argv`` for  the command or module to handle.


.. _tut-interactive:

Interactive Mode
----------------

When commands are read from a tty, the interpreter is said to be in *interactive
mode*.  In this mode it prompts for the next command with the *primary prompt*,
usually three greater-than signs (``>>>``); for continuation lines it prompts
with the *secondary prompt*, by default three dots (``...``). The interpreter
prints a welcome message stating its version number and a copyright notice
before printing the first prompt::

   python
   Python 2.6 (#1, Feb 28 2007, 00:02:06)
   Type "help", "copyright", "credits" or "license" for more information.
   >>>

Continuation lines are needed when entering a multi-line construct. As an
example, take a look at this :keyword:`if` statement::

   >>> the_world_is_flat = 1
   >>> if the_world_is_flat:
   ...     print "Be careful not to fall off!"
   ... 
   Be careful not to fall off!


.. _tut-interp:

The Interpreter and Its Environment
===================================


.. _tut-error:

Error Handling
--------------

When an error occurs, the interpreter prints an error message and a stack trace.
In interactive mode, it then returns to the primary prompt; when input came from
a file, it exits with a nonzero exit status after printing the stack trace.
(Exceptions handled by an :keyword:`except` clause in a :keyword:`try` statement
are not errors in this context.)  Some errors are unconditionally fatal and
cause an exit with a nonzero exit; this applies to internal inconsistencies and
some cases of running out of memory.  All error messages are written to the
standard error stream; normal output from executed commands is written to
standard output.

Typing the interrupt character (usually Control-C or DEL) to the primary or
secondary prompt cancels the input and returns to the primary prompt. [#]_
Typing an interrupt while a command is executing raises the
:exc:`KeyboardInterrupt` exception, which may be handled by a :keyword:`try`
statement.


.. _tut-scripts:

Executable Python Scripts
-------------------------

On BSD'ish Unix systems, Python scripts can be made directly executable, like
shell scripts, by putting the line ::

   #! /usr/bin/env python

(assuming that the interpreter is on the user's :envvar:`PATH`) at the beginning
of the script and giving the file an executable mode.  The ``#!`` must be the
first two characters of the file.  On some platforms, this first line must end
with a Unix-style line ending (``'\n'``), not a Mac OS (``'\r'``) or Windows
(``'\r\n'``) line ending.  Note that the hash, or pound, character, ``'#'``, is
used to start a comment in Python.

The script can be given an executable mode, or permission, using the
:program:`chmod` command::

   $ chmod +x myscript.py

On Windows systems, there is no notion of an "executable mode".  The Python
installer automatically associates ``.py`` files with ``python.exe`` so that
a double-click on a Python file will run it as a script.  The extension can
also be ``.pyw``, in that case, the console window that normally appears is
suppressed.


Source Code Encoding
--------------------

It is possible to use encodings different than ASCII in Python source files. The
best way to do it is to put one more special comment line right after the ``#!``
line to define the source file encoding::

   # -*- coding: encoding -*- 


With that declaration, all characters in the source file will be treated as
having the encoding *encoding*, and it will be possible to directly write
Unicode string literals in the selected encoding.  The list of possible
encodings can be found in the Python Library Reference, in the section on
:mod:`codecs`.

For example, to write Unicode literals including the Euro currency symbol, the
ISO-8859-15 encoding can be used, with the Euro symbol having the ordinal value
164.  This script will print the value 8364 (the Unicode codepoint corresponding
to the Euro symbol) and then exit::

   # -*- coding: iso-8859-15 -*-

   currency = u"€"
   print ord(currency)

If your editor supports saving files as ``UTF-8`` with a UTF-8 *byte order mark*
(aka BOM), you can use that instead of an encoding declaration. IDLE supports
this capability if ``Options/General/Default Source Encoding/UTF-8`` is set.
Notice that this signature is not understood in older Python releases (2.2 and
earlier), and also not understood by the operating system for script files with
``#!`` lines (only used on Unix systems).

By using UTF-8 (either through the signature or an encoding declaration),
characters of most languages in the world can be used simultaneously in string
literals and comments.  Using non-ASCII characters in identifiers is not
supported. To display all these characters properly, your editor must recognize
that the file is UTF-8, and it must use a font that supports all the characters
in the file.


.. _tut-startup:

The Interactive Startup File
----------------------------

When you use Python interactively, it is frequently handy to have some standard
commands executed every time the interpreter is started.  You can do this by
setting an environment variable named :envvar:`PYTHONSTARTUP` to the name of a
file containing your start-up commands.  This is similar to the :file:`.profile`
feature of the Unix shells.

.. XXX This should probably be dumped in an appendix, since most people
   don't use Python interactively in non-trivial ways.

This file is only read in interactive sessions, not when Python reads commands
from a script, and not when :file:`/dev/tty` is given as the explicit source of
commands (which otherwise behaves like an interactive session).  It is executed
in the same namespace where interactive commands are executed, so that objects
that it defines or imports can be used without qualification in the interactive
session. You can also change the prompts ``sys.ps1`` and ``sys.ps2`` in this
file.

If you want to read an additional start-up file from the current directory, you
can program this in the global start-up file using code like ``if
os.path.isfile('.pythonrc.py'): execfile('.pythonrc.py')``.  If you want to use
the startup file in a script, you must do this explicitly in the script::

   import os
   filename = os.environ.get('PYTHONSTARTUP')
   if filename and os.path.isfile(filename):
       execfile(filename)


.. rubric:: Footnotes

.. [#] A problem with the GNU Readline package may prevent this.

