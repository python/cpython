
:mod:`cmd` --- Support for line-oriented command interpreters
=============================================================

.. module:: cmd
   :synopsis: Build line-oriented command interpreters.
.. sectionauthor:: Eric S. Raymond <esr@snark.thyrsus.com>


The :class:`Cmd` class provides a simple framework for writing line-oriented
command interpreters.  These are often useful for test harnesses, administrative
tools, and prototypes that will later be wrapped in a more sophisticated
interface.


.. class:: Cmd([completekey[, stdin[, stdout]]])

   A :class:`Cmd` instance or subclass instance is a line-oriented interpreter
   framework.  There is no good reason to instantiate :class:`Cmd` itself; rather,
   it's useful as a superclass of an interpreter class you define yourself in order
   to inherit :class:`Cmd`'s methods and encapsulate action methods.

   The optional argument *completekey* is the :mod:`readline` name of a completion
   key; it defaults to :kbd:`Tab`. If *completekey* is not :const:`None` and
   :mod:`readline` is available, command completion is done automatically.

   The optional arguments *stdin* and *stdout* specify the  input and output file
   objects that the Cmd instance or subclass  instance will use for input and
   output. If not specified, they will default to *sys.stdin* and *sys.stdout*.


.. _cmd-objects:

Cmd Objects
-----------

A :class:`Cmd` instance has the following methods:


.. method:: Cmd.cmdloop([intro])

   Repeatedly issue a prompt, accept input, parse an initial prefix off the
   received input, and dispatch to action methods, passing them the remainder of
   the line as argument.

   The optional argument is a banner or intro string to be issued before the first
   prompt (this overrides the :attr:`intro` class member).

   If the :mod:`readline` module is loaded, input will automatically inherit
   :program:`bash`\ -like history-list editing (e.g. :kbd:`Control-P` scrolls back
   to the last command, :kbd:`Control-N` forward to the next one, :kbd:`Control-F`
   moves the cursor to the right non-destructively, :kbd:`Control-B` moves the
   cursor to the left non-destructively, etc.).

   An end-of-file on input is passed back as the string ``'EOF'``.

   An interpreter instance will recognize a command name ``foo`` if and only if it
   has a method :meth:`do_foo`.  As a special case, a line beginning with the
   character ``'?'`` is dispatched to the method :meth:`do_help`.  As another
   special case, a line beginning with the character ``'!'`` is dispatched to the
   method :meth:`do_shell` (if such a method is defined).

   This method will return when the :meth:`postcmd` method returns a true value.
   The *stop* argument to :meth:`postcmd` is the return value from the command's
   corresponding :meth:`do_\*` method.

   If completion is enabled, completing commands will be done automatically, and
   completing of commands args is done by calling :meth:`complete_foo` with
   arguments *text*, *line*, *begidx*, and *endidx*.  *text* is the string prefix
   we are attempting to match: all returned matches must begin with it. *line* is
   the current input line with leading whitespace removed, *begidx* and *endidx*
   are the beginning and ending indexes of the prefix text, which could be used to
   provide different completion depending upon which position the argument is in.

   All subclasses of :class:`Cmd` inherit a predefined :meth:`do_help`. This
   method, called with an argument ``'bar'``, invokes the corresponding method
   :meth:`help_bar`.  With no argument, :meth:`do_help` lists all available help
   topics (that is, all commands with corresponding :meth:`help_\*` methods), and
   also lists any undocumented commands.


.. method:: Cmd.onecmd(str)

   Interpret the argument as though it had been typed in response to the prompt.
   This may be overridden, but should not normally need to be; see the
   :meth:`precmd` and :meth:`postcmd` methods for useful execution hooks.  The
   return value is a flag indicating whether interpretation of commands by the
   interpreter should stop.  If there is a :meth:`do_\*` method for the command
   *str*, the return value of that method is returned, otherwise the return value
   from the :meth:`default` method is returned.


.. method:: Cmd.emptyline()

   Method called when an empty line is entered in response to the prompt. If this
   method is not overridden, it repeats the last nonempty command entered.


.. method:: Cmd.default(line)

   Method called on an input line when the command prefix is not recognized. If
   this method is not overridden, it prints an error message and returns.


.. method:: Cmd.completedefault(text, line, begidx, endidx)

   Method called to complete an input line when no command-specific
   :meth:`complete_\*` method is available.  By default, it returns an empty list.


.. method:: Cmd.precmd(line)

   Hook method executed just before the command line *line* is interpreted, but
   after the input prompt is generated and issued.  This method is a stub in
   :class:`Cmd`; it exists to be overridden by subclasses.  The return value is
   used as the command which will be executed by the :meth:`onecmd` method; the
   :meth:`precmd` implementation may re-write the command or simply return *line*
   unchanged.


.. method:: Cmd.postcmd(stop, line)

   Hook method executed just after a command dispatch is finished.  This method is
   a stub in :class:`Cmd`; it exists to be overridden by subclasses.  *line* is the
   command line which was executed, and *stop* is a flag which indicates whether
   execution will be terminated after the call to :meth:`postcmd`; this will be the
   return value of the :meth:`onecmd` method.  The return value of this method will
   be used as the new value for the internal flag which corresponds to *stop*;
   returning false will cause interpretation to continue.


.. method:: Cmd.preloop()

   Hook method executed once when :meth:`cmdloop` is called.  This method is a stub
   in :class:`Cmd`; it exists to be overridden by subclasses.


.. method:: Cmd.postloop()

   Hook method executed once when :meth:`cmdloop` is about to return. This method
   is a stub in :class:`Cmd`; it exists to be overridden by subclasses.

Instances of :class:`Cmd` subclasses have some public instance variables:


.. attribute:: Cmd.prompt

   The prompt issued to solicit input.


.. attribute:: Cmd.identchars

   The string of characters accepted for the command prefix.


.. attribute:: Cmd.lastcmd

   The last nonempty command prefix seen.


.. attribute:: Cmd.intro

   A string to issue as an intro or banner.  May be overridden by giving the
   :meth:`cmdloop` method an argument.


.. attribute:: Cmd.doc_header

   The header to issue if the help output has a section for documented commands.


.. attribute:: Cmd.misc_header

   The header to issue if the help output has a section for miscellaneous  help
   topics (that is, there are :meth:`help_\*` methods without corresponding
   :meth:`do_\*` methods).


.. attribute:: Cmd.undoc_header

   The header to issue if the help output has a section for undocumented  commands
   (that is, there are :meth:`do_\*` methods without corresponding :meth:`help_\*`
   methods).


.. attribute:: Cmd.ruler

   The character used to draw separator lines under the help-message headers.  If
   empty, no ruler line is drawn.  It defaults to ``'='``.


.. attribute:: Cmd.use_rawinput

   A flag, defaulting to true.  If true, :meth:`cmdloop` uses :func:`input` to
   display a prompt and read the next command; if false, :meth:`sys.stdout.write`
   and :meth:`sys.stdin.readline` are used. (This means that by importing
   :mod:`readline`, on systems that support it, the interpreter will automatically
   support :program:`Emacs`\ -like line editing  and command-history keystrokes.)

