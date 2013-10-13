:mod:`code` --- Interpreter base classes
========================================

.. module:: code
   :synopsis: Facilities to implement read-eval-print loops.


The ``code`` module provides facilities to implement read-eval-print loops in
Python.  Two classes and convenience functions are included which can be used to
build applications which provide an interactive interpreter prompt.


.. class:: InteractiveInterpreter(locals=None)

   This class deals with parsing and interpreter state (the user's namespace); it
   does not deal with input buffering or prompting or input file naming (the
   filename is always passed in explicitly). The optional *locals* argument
   specifies the dictionary in which code will be executed; it defaults to a newly
   created dictionary with key ``'__name__'`` set to ``'__console__'`` and key
   ``'__doc__'`` set to ``None``.


.. class:: InteractiveConsole(locals=None, filename="<console>")

   Closely emulate the behavior of the interactive Python interpreter. This class
   builds on :class:`InteractiveInterpreter` and adds prompting using the familiar
   ``sys.ps1`` and ``sys.ps2``, and input buffering.


.. function:: interact(banner=None, readfunc=None, local=None)

   Convenience function to run a read-eval-print loop.  This creates a new
   instance of :class:`InteractiveConsole` and sets *readfunc* to be used as
   the :meth:`InteractiveConsole.raw_input` method, if provided.  If *local* is
   provided, it is passed to the :class:`InteractiveConsole` constructor for
   use as the default namespace for the interpreter loop.  The :meth:`interact`
   method of the instance is then run with *banner* passed as the banner to
   use, if provided.  The console object is discarded after use.


.. function:: compile_command(source, filename="<input>", symbol="single")

   This function is useful for programs that want to emulate Python's interpreter
   main loop (a.k.a. the read-eval-print loop).  The tricky part is to determine
   when the user has entered an incomplete command that can be completed by
   entering more text (as opposed to a complete command or a syntax error).  This
   function *almost* always makes the same decision as the real interpreter main
   loop.

   *source* is the source string; *filename* is the optional filename from which
   source was read, defaulting to ``'<input>'``; and *symbol* is the optional
   grammar start symbol, which should be either ``'single'`` (the default) or
   ``'eval'``.

   Returns a code object (the same as ``compile(source, filename, symbol)``) if the
   command is complete and valid; ``None`` if the command is incomplete; raises
   :exc:`SyntaxError` if the command is complete and contains a syntax error, or
   raises :exc:`OverflowError` or :exc:`ValueError` if the command contains an
   invalid literal.


.. _interpreter-objects:

Interactive Interpreter Objects
-------------------------------


.. method:: InteractiveInterpreter.runsource(source, filename="<input>", symbol="single")

   Compile and run some source in the interpreter. Arguments are the same as for
   :func:`compile_command`; the default for *filename* is ``'<input>'``, and for
   *symbol* is ``'single'``.  One several things can happen:

   * The input is incorrect; :func:`compile_command` raised an exception
     (:exc:`SyntaxError` or :exc:`OverflowError`).  A syntax traceback will be
     printed by calling the :meth:`showsyntaxerror` method.  :meth:`runsource`
     returns ``False``.

   * The input is incomplete, and more input is required; :func:`compile_command`
     returned ``None``. :meth:`runsource` returns ``True``.

   * The input is complete; :func:`compile_command` returned a code object.  The
     code is executed by calling the :meth:`runcode` (which also handles run-time
     exceptions, except for :exc:`SystemExit`). :meth:`runsource` returns ``False``.

   The return value can be used to decide whether to use ``sys.ps1`` or ``sys.ps2``
   to prompt the next line.


.. method:: InteractiveInterpreter.runcode(code)

   Execute a code object. When an exception occurs, :meth:`showtraceback` is called
   to display a traceback.  All exceptions are caught except :exc:`SystemExit`,
   which is allowed to propagate.

   A note about :exc:`KeyboardInterrupt`: this exception may occur elsewhere in
   this code, and may not always be caught.  The caller should be prepared to deal
   with it.


.. method:: InteractiveInterpreter.showsyntaxerror(filename=None)

   Display the syntax error that just occurred.  This does not display a stack
   trace because there isn't one for syntax errors. If *filename* is given, it is
   stuffed into the exception instead of the default filename provided by Python's
   parser, because it always uses ``'<string>'`` when reading from a string. The
   output is written by the :meth:`write` method.


.. method:: InteractiveInterpreter.showtraceback()

   Display the exception that just occurred.  We remove the first stack item
   because it is within the interpreter object implementation. The output is
   written by the :meth:`write` method.


.. method:: InteractiveInterpreter.write(data)

   Write a string to the standard error stream (``sys.stderr``). Derived classes
   should override this to provide the appropriate output handling as needed.


.. _console-objects:

Interactive Console Objects
---------------------------

The :class:`InteractiveConsole` class is a subclass of
:class:`InteractiveInterpreter`, and so offers all the methods of the
interpreter objects as well as the following additions.


.. method:: InteractiveConsole.interact(banner=None)

   Closely emulate the interactive Python console. The optional *banner* argument
   specify the banner to print before the first interaction; by default it prints a
   banner similar to the one printed by the standard Python interpreter, followed
   by the class name of the console object in parentheses (so as not to confuse
   this with the real interpreter -- since it's so close!).

   .. versionchanged:: 3.4
      To suppress printing any banner, pass an empty string.


.. method:: InteractiveConsole.push(line)

   Push a line of source text to the interpreter. The line should not have a
   trailing newline; it may have internal newlines.  The line is appended to a
   buffer and the interpreter's :meth:`runsource` method is called with the
   concatenated contents of the buffer as source.  If this indicates that the
   command was executed or invalid, the buffer is reset; otherwise, the command is
   incomplete, and the buffer is left as it was after the line was appended.  The
   return value is ``True`` if more input is required, ``False`` if the line was
   dealt with in some way (this is the same as :meth:`runsource`).


.. method:: InteractiveConsole.resetbuffer()

   Remove any unhandled source text from the input buffer.


.. method:: InteractiveConsole.raw_input(prompt="")

   Write a prompt and read a line.  The returned line does not include the trailing
   newline.  When the user enters the EOF key sequence, :exc:`EOFError` is raised.
   The base implementation reads from ``sys.stdin``; a subclass may replace this
   with a different implementation.

