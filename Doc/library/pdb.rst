
.. _debugger:

:mod:`pdb` --- The Python Debugger
==================================

.. module:: pdb
   :synopsis: The Python debugger for interactive interpreters.


.. index:: single: debugging

The module :mod:`pdb` defines an interactive source code debugger for Python
programs.  It supports setting (conditional) breakpoints and single stepping at
the source line level, inspection of stack frames, source code listing, and
evaluation of arbitrary Python code in the context of any stack frame.  It also
supports post-mortem debugging and can be called under program control.

.. index::
   single: Pdb (class in pdb)
   module: bdb
   module: cmd

The debugger is extensible --- it is actually defined as the class :class:`Pdb`.
This is currently undocumented but easily understood by reading the source.  The
extension interface uses the modules :mod:`bdb` (undocumented) and :mod:`cmd`.

The debugger's prompt is ``(Pdb)``. Typical usage to run a program under control
of the debugger is::

   >>> import pdb
   >>> import mymodule
   >>> pdb.run('mymodule.test()')
   > <string>(0)?()
   (Pdb) continue
   > <string>(1)?()
   (Pdb) continue
   NameError: 'spam'
   > <string>(1)?()
   (Pdb) 

:file:`pdb.py` can also be invoked as a script to debug other scripts.  For
example::

   python -m pdb myscript.py

When invoked as a script, pdb will automatically enter post-mortem debugging if
the program being debugged exits abnormally. After post-mortem debugging (or
after normal exit of the program), pdb will restart the program. Automatic
restarting preserves pdb's state (such as breakpoints) and in most cases is more
useful than quitting the debugger upon program's exit.

Typical usage to inspect a crashed program is::

   >>> import pdb
   >>> import mymodule
   >>> mymodule.test()
   Traceback (most recent call last):
     File "<stdin>", line 1, in ?
     File "./mymodule.py", line 4, in test
       test2()
     File "./mymodule.py", line 3, in test2
       print(spam)
   NameError: spam
   >>> pdb.pm()
   > ./mymodule.py(3)test2()
   -> print(spam)
   (Pdb) 

The module defines the following functions; each enters the debugger in a
slightly different way:


.. function:: run(statement[, globals[, locals]])

   Execute the *statement* (given as a string) under debugger control.  The
   debugger prompt appears before any code is executed; you can set breakpoints and
   type ``continue``, or you can step through the statement using ``step`` or
   ``next`` (all these commands are explained below).  The optional *globals* and
   *locals* arguments specify the environment in which the code is executed; by
   default the dictionary of the module :mod:`__main__` is used.  (See the
   explanation of the built-in :func:`exec` or :func:`eval` functions.)


.. function:: runeval(expression[, globals[, locals]])

   Evaluate the *expression* (given as a string) under debugger control.  When
   :func:`runeval` returns, it returns the value of the expression.  Otherwise this
   function is similar to :func:`run`.


.. function:: runcall(function[, argument, ...])

   Call the *function* (a function or method object, not a string) with the given
   arguments.  When :func:`runcall` returns, it returns whatever the function call
   returned.  The debugger prompt appears as soon as the function is entered.


.. function:: set_trace()

   Enter the debugger at the calling stack frame.  This is useful to hard-code a
   breakpoint at a given point in a program, even if the code is not otherwise
   being debugged (e.g. when an assertion fails).


.. function:: post_mortem([traceback])

   Enter post-mortem debugging of the given *traceback* object.  If no 
   *traceback* is given, it uses the one of the exception that is currently
   being handled (an exception must be being handled if the default is to be
   used).


.. function:: pm()

   Enter post-mortem debugging of the traceback found in ``sys.last_traceback``.


.. _debugger-commands:

Debugger Commands
=================

The debugger recognizes the following commands.  Most commands can be
abbreviated to one or two letters; e.g. ``h(elp)`` means that either ``h`` or
``help`` can be used to enter the help command (but not ``he`` or ``hel``, nor
``H`` or ``Help`` or ``HELP``).  Arguments to commands must be separated by
whitespace (spaces or tabs).  Optional arguments are enclosed in square brackets
(``[]``) in the command syntax; the square brackets must not be typed.
Alternatives in the command syntax are separated by a vertical bar (``|``).

Entering a blank line repeats the last command entered.  Exception: if the last
command was a ``list`` command, the next 11 lines are listed.

Commands that the debugger doesn't recognize are assumed to be Python statements
and are executed in the context of the program being debugged.  Python
statements can also be prefixed with an exclamation point (``!``).  This is a
powerful way to inspect the program being debugged; it is even possible to
change a variable or call a function.  When an exception occurs in such a
statement, the exception name is printed but the debugger's state is not
changed.

Multiple commands may be entered on a single line, separated by ``;;``.  (A
single ``;`` is not used as it is the separator for multiple commands in a line
that is passed to the Python parser.) No intelligence is applied to separating
the commands; the input is split at the first ``;;`` pair, even if it is in the
middle of a quoted string.

The debugger supports aliases.  Aliases can have parameters which allows one a
certain level of adaptability to the context under examination.

.. index::
   pair: .pdbrc; file
   triple: debugger; configuration; file

If a file :file:`.pdbrc`  exists in the user's home directory or in the current
directory, it is read in and executed as if it had been typed at the debugger
prompt. This is particularly useful for aliases.  If both files exist, the one
in the home directory is read first and aliases defined there can be overridden
by the local file.

h(elp) [*command*]
   Without argument, print the list of available commands.  With a *command* as
   argument, print help about that command.  ``help pdb`` displays the full
   documentation file; if the environment variable :envvar:`PAGER` is defined, the
   file is piped through that command instead.  Since the *command* argument must
   be an identifier, ``help exec`` must be entered to get help on the ``!``
   command.

w(here)
   Print a stack trace, with the most recent frame at the bottom.  An arrow
   indicates the current frame, which determines the context of most commands.

d(own)
   Move the current frame one level down in the stack trace (to a newer frame).

u(p)
   Move the current frame one level up in the stack trace (to an older frame).

b(reak) [[*filename*:]\ *lineno* | *function*\ [, *condition*]]
   With a *lineno* argument, set a break there in the current file.  With a
   *function* argument, set a break at the first executable statement within that
   function. The line number may be prefixed with a filename and a colon, to
   specify a breakpoint in another file (probably one that hasn't been loaded yet).
   The file is searched on ``sys.path``. Note that each breakpoint is assigned a
   number to which all the other breakpoint commands refer.

   If a second argument is present, it is an expression which must evaluate to true
   before the breakpoint is honored.

   Without argument, list all breaks, including for each breakpoint, the number of
   times that breakpoint has been hit, the current ignore count, and the associated
   condition if any.

tbreak [[*filename*:]\ *lineno* | *function*\ [, *condition*]]
   Temporary breakpoint, which is removed automatically when it is first hit.  The
   arguments are the same as break.

cl(ear) [*bpnumber* [*bpnumber ...*]]
   With a space separated list of breakpoint numbers, clear those breakpoints.
   Without argument, clear all breaks (but first ask confirmation).

disable [*bpnumber* [*bpnumber ...*]]
   Disables the breakpoints given as a space separated list of breakpoint numbers.
   Disabling a breakpoint means it cannot cause the program to stop execution, but
   unlike clearing a breakpoint, it remains in the list of breakpoints and can be
   (re-)enabled.

enable [*bpnumber* [*bpnumber ...*]]
   Enables the breakpoints specified.

ignore *bpnumber* [*count*]
   Sets the ignore count for the given breakpoint number.  If count is omitted, the
   ignore count is set to 0.  A breakpoint becomes active when the ignore count is
   zero.  When non-zero, the count is decremented each time the breakpoint is
   reached and the breakpoint is not disabled and any associated condition
   evaluates to true.

condition *bpnumber* [*condition*]
   Condition is an expression which must evaluate to true before the breakpoint is
   honored.  If condition is absent, any existing condition is removed; i.e., the
   breakpoint is made unconditional.

commands [*bpnumber*]
   Specify a list of commands for breakpoint number *bpnumber*.  The commands
   themselves appear on the following lines.  Type a line containing just 'end' to
   terminate the commands. An example::

      (Pdb) commands 1
      (com) print some_variable
      (com) end
      (Pdb)

   To remove all commands from a breakpoint, type commands and follow it
   immediately with  end; that is, give no commands.

   With no *bpnumber* argument, commands refers to the last breakpoint set.

   You can use breakpoint commands to start your program up again. Simply use the
   continue command, or step, or any other command that resumes execution.

   Specifying any command resuming execution (currently continue, step, next,
   return, jump, quit and their abbreviations) terminates the command list (as if
   that command was immediately followed by end). This is because any time you
   resume execution (even with a simple next or step), you may encounter another
   breakpoint--which could have its own command list, leading to ambiguities about
   which list to execute.

   If you use the 'silent' command in the command list, the usual message about
   stopping at a breakpoint is not printed.  This may be desirable for breakpoints
   that are to print a specific message and then continue.  If none of the other
   commands print anything, you see no sign that the breakpoint was reached.

s(tep)
   Execute the current line, stop at the first possible occasion (either in a
   function that is called or on the next line in the current function).

n(ext)
   Continue execution until the next line in the current function is reached or it
   returns.  (The difference between ``next`` and ``step`` is that ``step`` stops
   inside a called function, while ``next`` executes called functions at (nearly)
   full speed, only stopping at the next line in the current function.)

r(eturn)
   Continue execution until the current function returns.

c(ont(inue))
   Continue execution, only stop when a breakpoint is encountered.

j(ump) *lineno*
   Set the next line that will be executed.  Only available in the bottom-most
   frame.  This lets you jump back and execute code again, or jump forward to skip
   code that you don't want to run.

   It should be noted that not all jumps are allowed --- for instance it is not
   possible to jump into the middle of a :keyword:`for` loop or out of a
   :keyword:`finally` clause.

l(ist) [*first*\ [, *last*]]
   List source code for the current file.  Without arguments, list 11 lines around
   the current line or continue the previous listing.  With one argument, list 11
   lines around at that line.  With two arguments, list the given range; if the
   second argument is less than the first, it is interpreted as a count.

a(rgs)
   Print the argument list of the current function.

p(rint) *expression*
   Evaluate the *expression* in the current context and print its value.

pp *expression*
   Like the ``p`` command, except the value of the expression is pretty-printed
   using the :mod:`pprint` module.

alias [*name* [command]]
   Creates an alias called *name* that executes *command*.  The command must *not*
   be enclosed in quotes.  Replaceable parameters can be indicated by ``%1``,
   ``%2``, and so on, while ``%*`` is replaced by all the parameters.  If no
   command is given, the current alias for *name* is shown. If no arguments are
   given, all aliases are listed.

   Aliases may be nested and can contain anything that can be legally typed at the
   pdb prompt.  Note that internal pdb commands *can* be overridden by aliases.
   Such a command is then hidden until the alias is removed.  Aliasing is
   recursively applied to the first word of the command line; all other words in
   the line are left alone.

   As an example, here are two useful aliases (especially when placed in the
   :file:`.pdbrc` file)::

      #Print instance variables (usage "pi classInst")
      alias pi for k in %1.__dict__.keys(): print("%1.",k,"=",%1.__dict__[k])
      #Print instance variables in self
      alias ps pi self

unalias *name*
   Deletes the specified alias.

[!]\ *statement*
   Execute the (one-line) *statement* in the context of the current stack frame.
   The exclamation point can be omitted unless the first word of the statement
   resembles a debugger command. To set a global variable, you can prefix the
   assignment command with a ``global`` command on the same line, e.g.::

      (Pdb) global list_options; list_options = ['-l']
      (Pdb)

run [*args* ...]
   Restart the debugged python program. If an argument is supplied, it is split
   with "shlex" and the result is used as the new sys.argv. History, breakpoints,
   actions and debugger options are preserved. "restart" is an alias for "run".

q(uit)
   Quit from the debugger. The program being executed is aborted.


.. _debugger-hooks:

How It Works
============

Some changes were made to the interpreter:

* ``sys.settrace(func)`` sets the global trace function

* there can also a local trace function (see later)

Trace functions have three arguments: *frame*, *event*, and *arg*. *frame* is
the current stack frame.  *event* is a string: ``'call'``, ``'line'``,
``'return'``, ``'exception'``, ``'c_call'``, ``'c_return'``, or
``'c_exception'``. *arg* depends on the event type.

The global trace function is invoked (with *event* set to ``'call'``) whenever a
new local scope is entered; it should return a reference to the local trace
function to be used that scope, or ``None`` if the scope shouldn't be traced.

The local trace function should return a reference to itself (or to another
function for further tracing in that scope), or ``None`` to turn off tracing in
that scope.

Instance methods are accepted (and very useful!) as trace functions.

The events have the following meaning:

``'call'``
   A function is called (or some other code block entered).  The global trace
   function is called; *arg* is ``None``; the return value specifies the local
   trace function.

``'line'``
   The interpreter is about to execute a new line of code (sometimes multiple line
   events on one line exist).  The local trace function is called; *arg* is
   ``None``; the return value specifies the new local trace function.

``'return'``
   A function (or other code block) is about to return.  The local trace function
   is called; *arg* is the value that will be returned.  The trace function's
   return value is ignored.

``'exception'``
   An exception has occurred.  The local trace function is called; *arg* is a
   triple ``(exception, value, traceback)``; the return value specifies the new
   local trace function.

``'c_call'``
   A C function is about to be called.  This may be an extension function or a
   builtin.  *arg* is the C function object.

``'c_return'``
   A C function has returned. *arg* is ``None``.

``'c_exception'``
   A C function has thrown an exception.  *arg* is ``None``.

Note that as an exception is propagated down the chain of callers, an
``'exception'`` event is generated at each level.

For more information on code and frame objects, refer to :ref:`types`.

