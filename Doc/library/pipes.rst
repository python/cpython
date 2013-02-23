:mod:`pipes` --- Interface to shell pipelines
=============================================

.. module:: pipes
   :platform: Unix
   :synopsis: A Python interface to Unix shell pipelines.
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>

**Source code:** :source:`Lib/pipes.py`

--------------

The :mod:`pipes` module defines a class to abstract the concept of a *pipeline*
--- a sequence of converters from one file to  another.

Because the module uses :program:`/bin/sh` command lines, a POSIX or compatible
shell for :func:`os.system` and :func:`os.popen` is required.


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


.. function:: quote(s)

   .. deprecated:: 1.6
      Prior to Python 2.7, this function was not publicly documented.  It is
      finally exposed publicly in Python 3.3 as the
      :func:`quote <shlex.quote>` function in the :mod:`shlex` module.

   Return a shell-escaped version of the string *s*.  The returned value is a
   string that can safely be used as one token in a shell command line, for
   cases where you cannot use a list.

   This idiom would be unsafe::

      >>> filename = 'somefile; rm -rf ~'
      >>> command = 'ls -l {}'.format(filename)
      >>> print command  # executed by a shell: boom!
      ls -l somefile; rm -rf ~

   :func:`quote` lets you plug the security hole::

      >>> command = 'ls -l {}'.format(quote(filename))
      >>> print command
      ls -l 'somefile; rm -rf ~'
      >>> remote_command = 'ssh home {}'.format(quote(command))
      >>> print remote_command
      ssh home 'ls -l '"'"'somefile; rm -rf ~'"'"''

   The quoting is compatible with UNIX shells and with :func:`shlex.split`:

      >>> remote_command = shlex.split(remote_command)
      >>> remote_command
      ['ssh', 'home', "ls -l 'somefile; rm -rf ~'"]
      >>> command = shlex.split(remote_command[-1])
      >>> command
      ['ls', '-l', 'somefile; rm -rf ~']


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

