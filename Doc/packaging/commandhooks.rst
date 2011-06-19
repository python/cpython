.. TODO integrate this in commandref and configfile

.. _packaging-command-hooks:

=============
Command hooks
=============

Packaging provides a way of extending its commands by the use of pre- and
post-command hooks.  Hooks are Python functions (or any callable object) that
take a command object as argument.  They're specified in :ref:`config files
<packaging-config-filenames>` using their fully qualified names.  After a
command is finalized (its options are processed), the pre-command hooks are
executed, then the command itself is run, and finally the post-command hooks are
executed.

See also global setup hooks in :ref:`setupcfg-spec`.


.. _packaging-finding-hooks:

Finding hooks
=============

As a hook is configured with a Python dotted name, it must either be defined in
a module installed on the system, or in a module present in the project
directory, where the :file:`setup.cfg` file lives::

  # file: _setuphooks.py

  def hook(install_cmd):
      metadata = install_cmd.dist.metadata
      print('Hooked while installing %r %s!' % (metadata['Name'],
                                                metadata['Version']))

Then you need to configure it in :file:`setup.cfg`::

  [install_dist]
  pre-hook.a = _setuphooks.hook

Packaging will add the project directory to :data:`sys.path` and find the
``_setuphooks`` module.

Hooks defined in different config files (system-wide, user-wide and
project-wide) do not override each other as long as they are specified with
different aliases (additional names after the dot).  The alias in the example
above is ``a``.
