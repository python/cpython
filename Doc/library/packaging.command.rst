:mod:`packaging.command` --- Standard Packaging commands
========================================================

.. module:: packaging.command
   :synopsis: Standard packaging commands.


This subpackage contains one module for each standard Packaging command, such as
:command:`build`  or :command:`upload`.  Each command is implemented as a
separate module, with the command name as the name of the module and of the
class defined therein.



:mod:`packaging.command.cmd` --- Abstract base class for Packaging commands
===========================================================================

.. module:: packaging.command.cmd
   :synopsis: Abstract base class for commands.


This module supplies the abstract base class :class:`Command`.  This class is
subclassed by the modules in the packaging.command subpackage.


.. class:: Command(dist)

   Abstract base class for defining command classes, the "worker bees" of the
   Packaging.  A useful analogy for command classes is to think of them as
   subroutines with local variables called *options*.  The options are declared
   in :meth:`initialize_options` and defined (given their final values) in
   :meth:`finalize_options`, both of which must be defined by every command
   class.  The distinction between the two is necessary because option values
   might come from the outside world (command line, config file, ...), and any
   options dependent on other options must be computed after these outside
   influences have been processed --- hence :meth:`finalize_options`.  The body
   of the subroutine, where it does all its work based on the values of its
   options, is the :meth:`run` method, which must also be implemented by every
   command class.

   The class constructor takes a single argument *dist*, a
   :class:`~packaging.dist.Distribution` instance.


Creating a new Packaging command
--------------------------------

This section outlines the steps to create a new Packaging command.

.. XXX the following paragraph is focused on the stdlib; expand it to document
   how to write and register a command in third-party projects

A new command lives in a module in the :mod:`packaging.command` package. There
is a sample template in that directory called :file:`command_template`.  Copy
this file to a new module with the same name as the new command you're
implementing.  This module should implement a class with the same name as the
module (and the command).  So, for instance, to create the command
``peel_banana`` (so that users can run ``setup.py peel_banana``), you'd copy
:file:`command_template` to :file:`packaging/command/peel_banana.py`, then edit
it so that it's implementing the class :class:`peel_banana`, a subclass of
:class:`Command`.  It must define the following methods:

.. method:: Command.initialize_options()

   Set default values for all the options that this command supports.  Note that
   these defaults may be overridden by other commands, by the setup script, by
   config files, or by the command line.  Thus, this is not the place to code
   dependencies between options; generally, :meth:`initialize_options`
   implementations are just a bunch of ``self.foo = None`` assignments.


.. method:: Command.finalize_options()

   Set final values for all the options that this command supports. This is
   always called as late as possible, i.e. after any option assignments from the
   command line or from other commands have been done.  Thus, this is the place
   to code option dependencies: if *foo* depends on *bar*, then it is safe to
   set *foo* from *bar* as long as *foo* still has the same value it was
   assigned in :meth:`initialize_options`.


.. method:: Command.run()

   A command's raison d'etre: carry out the action it exists to perform,
   controlled by the options initialized in :meth:`initialize_options`,
   customized by other commands, the setup script, the command line, and config
   files, and finalized in :meth:`finalize_options`.  All terminal output and
   filesystem interaction should be done by :meth:`run`.


Command classes may define this attribute:


.. attribute:: Command.sub_commands

   *sub_commands* formalizes the notion of a "family" of commands,
   e.g. ``install_dist`` as the parent with sub-commands ``install_lib``,
   ``install_headers``, etc.  The parent of a family of commands defines
   *sub_commands* as a class attribute; it's a list of 2-tuples ``(command_name,
   predicate)``, with *command_name* a string and *predicate* a function, a
   string or ``None``.  *predicate* is a method of the parent command that
   determines whether the corresponding command is applicable in the current
   situation.  (E.g. ``install_headers`` is only applicable if we have any C
   header files to install.)  If *predicate* is ``None``, that command is always
   applicable.

   *sub_commands* is usually defined at the *end* of a class, because
   predicates can be methods of the class, so they must already have been
   defined.  The canonical example is the :command:`install_dist` command.

.. XXX document how to add a custom command to another one's subcommands
