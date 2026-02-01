:mod:`!cmd` --- Support for line-oriented command interpreters
==============================================================

.. module:: cmd
   :synopsis: Build line-oriented command interpreters.

.. sectionauthor:: Eric S. Raymond <esr@snark.thyrsus.com>

**Source code:** :source:`Lib/cmd.py`

--------------

The :class:`Cmd` class provides a simple framework for writing line-oriented
command interpreters.  These are often useful for test harnesses, administrative
tools, and prototypes that will later be wrapped in a more sophisticated
interface.

.. class:: Cmd(completekey='tab', stdin=None, stdout=None)

   A :class:`Cmd` instance or subclass instance is a line-oriented interpreter
   framework.  There is no good reason to instantiate :class:`Cmd` itself; rather,
   it's useful as a superclass of an interpreter class you define yourself in order
   to inherit :class:`Cmd`'s methods and encapsulate action methods.

   The optional argument *completekey* is the :mod:`readline` name of a completion
   key; it defaults to :kbd:`Tab`. If *completekey* is not :const:`None` and
   :mod:`readline` is available, command completion is done automatically.

   The default, ``'tab'``, is treated specially, so that it refers to the
   :kbd:`Tab` key on every :data:`readline.backend`.
   Specifically, if :data:`readline.backend` is ``editline``,
   ``Cmd`` will use ``'^I'`` instead of ``'tab'``.
   Note that other values are not treated this way, and might only work
   with a specific backend.

   The optional arguments *stdin* and *stdout* specify the  input and output file
   objects that the Cmd instance or subclass  instance will use for input and
   output. If not specified, they will default to :data:`sys.stdin` and
   :data:`sys.stdout`.

   If you want a given *stdin* to be used, make sure to set the instance's
   :attr:`use_rawinput` attribute to ``False``, otherwise *stdin* will be
   ignored.

   .. versionchanged:: 3.13
      ``completekey='tab'`` is replaced by ``'^I'`` for ``editline``.

   .. note::

      Subtle behaviors of ``cmd.Cmd``:

      * Command handler methods (``do_<command>``) should return ``True`` to
        indicate that the command loop should terminate. Any other return
        value continues the loop.

      * If the user presses Enter on an empty line, the default behavior is
        to repeat the last nonempty command entered. This can be disabled by
        overriding :meth:`emptyline`.

      * If no matching ``do_<command>`` method is found, the
        :meth:`default` method is called.

      * Exceptions raised inside command handlers are not caught by default
        and will terminate the command loop unless handled explicitly.


.. _cmd-objects:

Cmd Objects
-----------

A :class:`Cmd` instance has the following methods:


.. method:: Cmd.cmdloop(intro=None)

   Repeatedly issue a prompt, accept input, parse an initial prefix off the
   received input, and dispatch to action methods, passing them the remainder of
   the line as argument.

   The optional argument is a banner or intro string to be issued before the first
   prompt (this overrides the :attr:`intro` class attribute).

   If the :mod:`readline` module is loaded, input will automatically inherit
   :program:`bash`\ -like history-list editing (e.g. :kbd:`Control-P` scrolls back
   to the last command, :kbd:`Control-N` forward to the next one, :kbd:`Control-F`
   moves the cursor to the right non-destructively, :kbd:`Control-B` moves the
   cursor to the left non-destructively, etc.).

   An end-of-file on input is passed back as the string ``'EOF'``.

   .. index::
      single: ? (question mark); in a command interpreter
      single: ! (exclamation); in a command interpreter

   An interpreter instance will recognize a command name ``foo`` if and only if it
   has a method :meth:`!do_foo`.  As a special case, a line beginning with the
   character ``'?'`` is dispatched to the method :meth:`do_help`.  As another
   special case, a line beginning with the character ``'!'`` is dispatched to the
   method :meth:`!do_shell` (if such a method is defined).

   This method will return when the :meth:`postcmd` method returns a true value.
   The *stop* argument to :meth:`postcmd` is the return value from the command's
   corresponding :meth:`!do_\*` method.

   If completion is enabled, completing commands will be done automatically, and
   completing of commands args is done by calling :meth:`!complete_foo` with
   arguments *text*, *line*, *begidx*, and *endidx*.  *text* is the string prefix
   we are attempting to match: all returned matches must begin with it. *line* is
   the current input line with leading whitespace removed, *begidx* and *endidx*
   are the beginning and ending indexes of the prefix text, which could be used to
   provide different completion depending upon which position the argument is in.
