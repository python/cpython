.. TODO integrate this in commandref and configfile

=============
Command hooks
=============

Packaging provides a way of extending its commands by the use of pre- and
post- command hooks. The hooks are simple Python functions (or any callable
objects) and are specified in the config file using their full qualified names.
The pre-hooks are run after the command is finalized (its options are
processed), but before it is run. The post-hooks are run after the command
itself. Both types of hooks receive an instance of the command object.

See also global setup hooks in :ref:`setupcfg-spec`.


Sample usage of hooks
=====================

Firstly, you need to make sure your hook is present in the path. This is usually
done by dropping them to the same folder where `setup.py` file lives ::

  # file: myhooks.py
  def my_install_hook(install_cmd):
      print "Oh la la! Someone is installing my project!"

Then, you need to point to it in your `setup.cfg` file, under the appropriate
command section ::

  [install_dist]
  pre-hook.project = myhooks.my_install_hook

The hooks defined in different config files (system-wide, user-wide and
package-wide) do not override each other as long as they are specified with
different aliases (additional names after the dot). The alias in the example
above is ``project``.
