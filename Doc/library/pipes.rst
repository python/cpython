:mod:`pipes` --- Interface to shell pipelines
=============================================

.. module:: pipes
   :platform: Unix
   :synopsis: A Python interface to Unix shell pipelines.
   :deprecated:

.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>

**Source code:** :source:`Lib/pipes.py`

.. deprecated-removed:: 3.11 3.13
   The :mod:`pipes` module is deprecated
   (see :pep:`PEP 594 <594#pipes>` for details).
   Please use the :mod:`subprocess` module instead.

--------------

The :mod:`pipes` module defines a class to abstract the concept of a *pipeline*
--- a sequence of converters from one file to  another.

Because the module uses :program:`/bin/sh` command lines, a POSIX or compatible
shell for :func:`os.system` and :func:`os.popen` is required.

.. availability:: Unix, not VxWorks.

The :mod:`pipes` module defines the following class:


.. class:: Template()

   An abstraction of a pipeline.

Example::

   >>> import pipes
   >>> t = pipes.Template()
   >>> t.append('tr a-z A-Z', '--')
   >>> f = t.open('pipefile', 'w')
   >>> f.write('hello world')
   >>> f.close()
   >>> open('pipefile').read()
   'HELLO WORLD'


.. _template-objects:

Template Objects
----------------

Template objects following methods:


.. method:: Template.reset()

   Restore a pipeline template to its initial state.


.. method:: Template.clone()

   Return a new, equivalent, pipeline template.


.. method:: Template.debug(flag)

   If *flag* is true, turn debugging on. Otherwise, turn debugging off. When
   debugging is on, commands to be executed are printed, and the shell is given
   ``set -x`` command to be more verbose.


.. method:: Template.append(cmd, kind)

   Append a new action at the end. The *cmd* variable must be a valid bourne shell
   command. The *kind* variable consists of two letters.

   The first letter can be either of ``'-'`` (which means the command reads its
   standard input), ``'f'`` (which means the commands reads a given file on the
   command line) or ``'.'`` (which means the commands reads no input, and hence
   must be first.)

   Similarly, the second letter can be either of ``'-'`` (which means  the command
   writes to standard output), ``'f'`` (which means the  command writes a file on
   the command line) or ``'.'`` (which means the command does not write anything,
   and hence must be last.)


.. method:: Template.prepend(cmd, kind)

   Add a new action at the beginning. See :meth:`append` for explanations of the
   arguments.


.. method:: Template.open(file, mode)

   Return a file-like object, open to *file*, but read from or written to by the
   pipeline.  Note that only one of ``'r'``, ``'w'`` may be given.


.. method:: Template.copy(infile, outfile)

   Copy *infile* to *outfile* through the pipe.

