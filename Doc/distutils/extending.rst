.. _extending:

*******************
Extending Distutils
*******************

Distutils can be extended in various ways.  Most extensions take the form of new
commands or replacements for existing commands.  New commands may be written to
support new types of platform-specific packaging, for example, while
replacements for existing commands may be made to modify details of how the
command operates on a package.

Most extensions of the distutils are made within :file:`setup.py` scripts that
want to modify existing commands; many simply add a few file extensions that
should be copied into packages in addition to :file:`.py` files as a
convenience.

Most distutils command implementations are subclasses of the :class:`Command`
class from :mod:`distutils.cmd`.  New commands may directly inherit from
:class:`Command`, while replacements often derive from :class:`Command`
indirectly, directly subclassing the command they are replacing.  Commands are
required to derive from :class:`Command`.

.. % \section{Extending existing commands}
.. % \label{extend-existing}

.. % \section{Writing new commands}
.. % \label{new-commands}
.. % \XXX{Would an uninstall command be a good example here?}


Integrating new commands
========================

There are different ways to integrate new command implementations into
distutils.  The most difficult is to lobby for the inclusion of the new features
in distutils itself, and wait for (and require) a version of Python that
provides that support.  This is really hard for many reasons.

The most common, and possibly the most reasonable for most needs, is to include
the new implementations with your :file:`setup.py` script, and cause the
:func:`distutils.core.setup` function use them::

   from distutils.command.build_py import build_py as _build_py
   from distutils.core import setup

   class build_py(_build_py):
       """Specialized Python source builder."""

       # implement whatever needs to be different...

   setup(cmdclass={'build_py': build_py},
         ...)

This approach is most valuable if the new implementations must be used to use a
particular package, as everyone interested in the package will need to have the
new command implementation.

Beginning with Python 2.4, a third option is available, intended to allow new
commands to be added which can support existing :file:`setup.py` scripts without
requiring modifications to the Python installation.  This is expected to allow
third-party extensions to provide support for additional packaging systems, but
the commands can be used for anything distutils commands can be used for.  A new
configuration option, :option:`command_packages` (command-line option
:option:`--command-packages`), can be used to specify additional packages to be
searched for modules implementing commands.  Like all distutils options, this
can be specified on the command line or in a configuration file.  This option
can only be set in the ``[global]`` section of a configuration file, or before
any commands on the command line.  If set in a configuration file, it can be
overridden from the command line; setting it to an empty string on the command
line causes the default to be used.  This should never be set in a configuration
file provided with a package.

This new option can be used to add any number of packages to the list of
packages searched for command implementations; multiple package names should be
separated by commas.  When not specified, the search is only performed in the
:mod:`distutils.command` package.  When :file:`setup.py` is run with the option
:option:`--command-packages` :option:`distcmds,buildcmds`, however, the packages
:mod:`distutils.command`, :mod:`distcmds`, and :mod:`buildcmds` will be searched
in that order.  New commands are expected to be implemented in modules of the
same name as the command by classes sharing the same name.  Given the example
command line option above, the command :command:`bdist_openpkg` could be
implemented by the class :class:`distcmds.bdist_openpkg.bdist_openpkg` or
:class:`buildcmds.bdist_openpkg.bdist_openpkg`.


Adding new distribution types
=============================

Commands that create distributions (files in the :file:`dist/` directory) need
to add ``(command, filename)`` pairs to ``self.distribution.dist_files`` so that
:command:`upload` can upload it to PyPI.  The *filename* in the pair contains no
path information, only the name of the file itself.  In dry-run mode, pairs
should still be added to represent what would have been created.


