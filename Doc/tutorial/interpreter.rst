.. _tut-using:

****************************
Using the Python Interpreter
****************************


.. _tut-invoking:

Invoking the Interpreter
========================

The Python interpreter is usually installed as :file:`/usr/local/bin/python3.4`
on those machines where it is available; putting :file:`/usr/local/bin` in your
Unix shell's search path makes it possible to start it by typing the command:

.. code-block:: text

   python3.4

to the shell. [#]_ Since the choice of the directory where the interpreter lives
is an installation option, other places are possible; check with your local
Python guru or system administrator.  (E.g., :file:`/usr/local/python` is a
popular alternative location.)

On Windows machines, the Python installation is usually placed in
:file:`C:\\Python34`, though you can change this when you're running the
installer.  To add this directory to your path,  you can type the following
command into the command prompt in a DOS box::

   set path=%path%;C:\python34

Typing an end-of-file character (:kbd:`Control-D` on Unix, :kbd:`Control-Z` on
Windows) at the primary prompt causes the interpreter to exit with a zero exit
status.  If that doesn't work, you can exit the interpreter by typing the
following command: ``quit()``.

The interpreter's line-editing features include interactive editing, history
substitution and code completion on systems that support readline.  Perhaps the
quickest check to see whether command line editing is supported is typing
Control-P to the first Python prompt you get.  If it beeps, you have command
line editing; see Appendix :ref:`tut-interacting` for an introduction to the
keys.  If nothing appears to happen, or if ``^P`` is echoed, command line
editing isn't available; you'll only be able to use backspace to remove
characters from the current line.

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

All command line options are described in :ref:`using-on-general`.


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

   $ python3.4
   Python 3.4 (default, Mar 16 2014, 09:25:04)
   [GCC 4.8.2] on linux
   Type "help", "copyright", "credits" or "license" for more information.
   >>>

.. XXX update for new releases

Continuation lines are needed when entering a multi-line construct. As an
example, take a look at this :keyword:`if` statement::

   >>> the_world_is_flat = True
   >>> if the_world_is_flat:
   ...     print("Be careful not to fall off!")
   ...
   Be careful not to fall off!


For more on interactive mode, see :ref:`tut-interac`.


.. _tut-interp:

The Interpreter and Its Environment
===================================


.. _tut-source-encoding:

Source Code Encoding
--------------------

By default, Python source files are treated as encoded in UTF-8.  In that
encoding, characters of most languages in the world can be used simultaneously
in string literals, identifiers and comments --- although the standard library
only uses ASCII characters for identifiers, a convention that any portable code
should follow.  To display all these characters properly, your editor must
recognize that the file is UTF-8, and it must use a font that supports all the
characters in the file.

It is also possible to specify a different encoding for source files.  In order
to do this, put one more special comment line right after the ``#!`` line to
define the source file encoding::

   # -*- coding: encoding -*-

With that declaration, everything in the source file will be treated as having
the encoding *encoding* instead of UTF-8.  The list of possible encodings can be
found in the Python Library Reference, in the section on :mod:`codecs`.

For example, if your editor of choice does not support UTF-8 encoded files and
insists on using some other encoding, say Windows-1252, you can write::

   # -*- coding: cp-1252 -*-

and still use all characters in the Windows-1252 character set in the source
files.  The special encoding comment must be in the *first or second* line
within the file.


.. rubric:: Footnotes

.. [#] On Unix, the Python 3.x interpreter is by default not installed with the
   executable named ``python``, so that it does not conflict with a
   simultaneously installed Python 2.x executable.
