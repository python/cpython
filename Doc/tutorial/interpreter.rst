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
:file:`C:\\Python27`, though you can change this when you're running the
installer.  To add this directory to your path,  you can type the following
command into the command prompt in a DOS box::

   set path=%path%;C:\python27

Typing an end-of-file character (:kbd:`Control-D` on Unix, :kbd:`Control-Z` on
Windows) at the primary prompt causes the interpreter to exit with a zero exit
status.  If that doesn't work, you can exit the interpreter by typing the
following command: ``quit()``.

The interpreter's line-editing features usually aren't very sophisticated.  On
Unix, whoever installed the interpreter may have enabled support for the GNU
readline library, which adds more elaborate interactive editing and history
features. Perhaps the quickest check to see whether command line editing is
supported is typing :kbd:`Control-P` to the first Python prompt you get.  If it beeps,
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
characters that are special to the shell, it is usually advised to quote
*command* in its entirety with single quotes.

Some Python modules are also useful as scripts.  These can be invoked using
``python -m module [arg] ...``, which executes the source file for *module* as
if you had spelled out its full name on the command line.

When a script file is used, it is sometimes useful to be able to run the script
and enter interactive mode afterwards.  This can be done by passing :option:`-i`
before the script.

All command-line options are described in :ref:`using-on-general`.


.. _tut-argpassing:

Argument Passing
----------------

When known to the interpreter, the script name and additional arguments
thereafter are turned into a list of strings and assigned to the ``argv``
variable in the ``sys`` module.  You can access this list by executing ``import
sys``.  The length of the list is at least one; when no script and no arguments
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
   Python 2.7 (#1, Feb 28 2010, 00:02:06)
   Type "help", "copyright", "credits" or "license" for more information.
   >>>

Continuation lines are needed when entering a multi-line construct. As an
example, take a look at this :keyword:`if` statement::

   >>> the_world_is_flat = 1
   >>> if the_world_is_flat:
   ...     print "Be careful not to fall off!"
   ...
   Be careful not to fall off!


For more on interactive mode, see :ref:`tut-interac`.


.. _tut-interp:

The Interpreter and Its Environment
===================================


.. _tut-source-encoding:

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
164.  This script, when saved in the ISO-8859-15 encoding, will print the value
8364 (the Unicode code point corresponding to the Euro symbol) and then exit::

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

