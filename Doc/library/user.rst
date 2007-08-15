
:mod:`user` --- User-specific configuration hook
================================================

.. module:: user
   :synopsis: A standard way to reference user-specific modules.


.. index::
   pair: .pythonrc.py; file
   triple: user; configuration; file

As a policy, Python doesn't run user-specified code on startup of Python
programs.  (Only interactive sessions execute the script specified in the
:envvar:`PYTHONSTARTUP` environment variable if it exists).

However, some programs or sites may find it convenient to allow users to have a
standard customization file, which gets run when a program requests it.  This
module implements such a mechanism.  A program that wishes to use the mechanism
must execute the statement ::

   import user

.. index:: builtin: exec

The :mod:`user` module looks for a file :file:`.pythonrc.py` in the user's home
directory and if it can be opened, executes it (using :func:`exec`) in its
own (the module :mod:`user`'s) global namespace.  Errors during this phase are
not caught; that's up to the program that imports the :mod:`user` module, if it
wishes.  The home directory is assumed to be named by the :envvar:`HOME`
environment variable; if this is not set, the current directory is used.

The user's :file:`.pythonrc.py` could conceivably test for ``sys.version`` if it
wishes to do different things depending on the Python version.

A warning to users: be very conservative in what you place in your
:file:`.pythonrc.py` file.  Since you don't know which programs will use it,
changing the behavior of standard modules or functions is generally not a good
idea.

A suggestion for programmers who wish to use this mechanism: a simple way to let
users specify options for your package is to have them define variables in their
:file:`.pythonrc.py` file that you test in your module.  For example, a module
:mod:`spam` that has a verbosity level can look for a variable
``user.spam_verbose``, as follows::

   import user

   verbose = bool(getattr(user, "spam_verbose", 0))

(The three-argument form of :func:`getattr` is used in case the user has not
defined ``spam_verbose`` in their :file:`.pythonrc.py` file.)

Programs with extensive customization needs are better off reading a
program-specific customization file.

Programs with security or privacy concerns should *not* import this module; a
user can easily break into a program by placing arbitrary code in the
:file:`.pythonrc.py` file.

Modules for general use should *not* import this module; it may interfere with
the operation of the importing program.


.. seealso::

   Module :mod:`site`
      Site-wide customization mechanism.

