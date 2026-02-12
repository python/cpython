:mod:`!venv` --- Creation of virtual environments
=================================================

.. module:: venv
   :synopsis: Creation of virtual environments.

.. moduleauthor:: Vinay Sajip <vinay_sajip@yahoo.co.uk>
.. sectionauthor:: Vinay Sajip <vinay_sajip@yahoo.co.uk>

.. versionadded:: 3.3

**Source code:** :source:`Lib/venv/`

.. index:: pair: Environments; virtual

--------------

.. _venv-def:
.. _venv-intro:

The :mod:`!venv` module supports creating lightweight "virtual environments",
each with their own independent set of Python packages installed in
their :mod:`site` directories.
A virtual environment is created on top of an existing
Python installation, known as the virtual environment's "base" Python, and by
default is isolated from the packages in the base environment,
so that only those explicitly installed in the virtual environment are
available. See :ref:`sys-path-init-virtual-environments` and :mod:`site`'s
:ref:`virtual environments documentation <site-virtual-environments-configuration>`
for more information.

When used from within a virtual environment, common installation tools such as
:pypi:`pip` will install Python packages into a virtual environment
without needing to be told to do so explicitly.

A virtual environment is (amongst other things):

* Used to contain a specific Python interpreter and software libraries and
  binaries which are needed to support a project (library or application). These
  are by default isolated from software in other virtual environments and Python
  interpreters and libraries installed in the operating system.

* Contained in a directory, conventionally named ``.venv`` or ``venv`` in
  the project directory, or under a container directory for lots of virtual
  environments, such as ``~/.virtualenvs``.

* Not checked into source control systems such as Git.

* Considered as disposable -- it should be simple to delete and recreate it from
  scratch. You don't place any project code in the environment.

* Not considered as movable or copyable -- you just recreate the same
  environment in the target location.

See :pep:`405` for more background on Python virtual environments.

.. seealso::

   `Python Packaging User Guide: Creating and using virtual environments
   <https://packaging.python.org/guides/installing-using-pip-and-virtual-environments/#create-and-use-virtual-environments>`__

.. include:: ../includes/wasm-mobile-notavail.rst

Creating virtual environments
-----------------------------

:ref:`Virtual environments <venv-def>` are created by executing the ``venv``
module:

.. code-block:: shell

    python -m venv /path/to/new/virtual/environment

This creates the target directory (including parent directories as needed)
and places a :file:`pyvenv.cfg` file in it with a ``home`` key
pointing to the Python installation from which the command was run.
It also creates a :file:`bin` (or :file:`Scripts` on Windows) subdirectory
containing a copy or symlink of the Python executable
(as appropriate for the platform or arguments used at environment creation time).
It also creates a :file:`lib/pythonX.Y/site-packages` subdirectory
(on Windows, this is :file:`Lib\\site-packages`).
If an existing directory is specified, it will be re-used.

.. versionchanged:: 3.5
   The use of ``venv`` is now recommended for creating virtual environments.

.. deprecated-removed:: 3.6 3.8
   :program:`pyvenv` was the recommended tool for creating virtual environments
   for Python 3.3 and 3.4, and replaced in 3.5 by executing ``venv`` directly.

.. highlight:: none

On Windows, invoke the ``venv`` command as follows:

.. code-block:: ps1con

   PS> python -m venv C:\path\to\new\virtual\environment

The command, if run with ``-h``, will show the available options::

   usage: venv [-h] [--system-site-packages] [--symlinks | --copies] [--clear]
               [--upgrade] [--without-pip] [--prompt PROMPT] [--upgrade-deps]
               [--without-scm-ignore-files]
               ENV_DIR [ENV_DIR ...]

   Creates virtual Python environments in one or more target directories.

   Once an environment has been created, you may wish to activate it, e.g. by
   sourcing an activate script in its bin directory.

.. _venv-cli:
.. program:: venv

.. option:: ENV_DIR

   A required argument specifying the directory to create the environment in.

.. option:: --system-site-packages

   Give the virtual environment access to the system site-packages directory.

.. option:: --symlinks

   Try to use symlinks rather than copies, when symlinks are not the default for the platform.

.. option:: --copies

   Try to use copies rather than symlinks, even when symlinks are the default for the platform.

.. option:: --clear

   Delete the contents of the environment directory if it already exists, before environment creation.

.. option:: --upgrade

   Upgrade the environment directory to use this version of Python, assuming Python has been upgraded in-place.

.. option:: --without-pip

   Skips installing or upgrading pip in the virtual environment (pip is bootstrapped by default).

.. option:: --prompt <PROMPT>

   Provides an alternative prompt prefix for this environment.

.. option:: --upgrade-deps

   Upgrade core dependencies (pip) to the latest version in PyPI.

.. option:: --without-scm-ignore-files

   Skips adding SCM ignore files to the environment directory (Git is supported by default).


.. versionchanged:: 3.4
   Installs pip by default, added the ``--without-pip``  and ``--copies``
   options.

.. versionchanged:: 3.4
   In earlier versions, if the target directory already existed, an error was
   raised, unless the ``--clear`` or ``--upgrade`` option was provided.

.. versionchanged:: 3.9
   Add ``--upgrade-deps`` option to upgrade pip + setuptools to the latest on PyPI.

.. versionchanged:: 3.12

   ``setuptools`` is no longer a core venv dependency.

.. versionchanged:: 3.13

   Added the ``--without-scm-ignore-files`` option.
.. versionchanged:: 3.13
   ``venv`` now creates a :file:`.gitignore` file for Git by default.

.. note::
   While symlinks are supported on Windows, they are not recommended. Of
   particular note is that double-clicking ``python.exe`` in File Explorer
   will resolve the symlink eagerly and ignore the virtual environment.

.. note::
   On Microsoft Windows, it may be required to enable the ``Activate.ps1``
   script by setting the execution policy for the user. You can do this by
   issuing the following PowerShell command:

   .. code-block:: powershell

      PS C:\> Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

   See `About Execution Policies
   <https://go.microsoft.com/fwlink/?LinkID=135170>`_
   for more information.

The created :file:`pyvenv.cfg` file also includes the
``include-system-site-packages`` key, set to ``true`` if ``venv`` is
run with the ``--system-site-packages`` option, ``false`` otherwise.

Unless the ``--without-pip`` option is given, :mod:`ensurepip` will be
invoked to bootstrap ``pip`` into the virtual environment.

Multiple paths can be given to ``venv``, in which case an identical virtual
environment will be created, according to the given options, at each provided
path.

.. _venv-explanation:

How venvs work
--------------

When a Python interpreter is running from a virtual environment,
:data:`sys.prefix` and :data:`sys.exec_prefix`
point to the directories of the virtual environment,
whereas :data:`sys.base_prefix` and :data:`sys.base_exec_prefix`
point to those of the base Python used to create the environment.
It is sufficient to check
``sys.prefix != sys.base_prefix`` to determine if the current interpreter is
running from a virtual environment.

A virtual environment may be "activated" using a script in its binary directory
(``bin`` on POSIX; ``Scripts`` on Windows).
This will prepend that directory to your :envvar:`PATH`, so that running
:program:`python` will invoke the environment's Python interpreter
and you can run installed scripts without having to use their full path.
The invocation of the activation script is platform-specific
(:samp:`{<venv>}` must be replaced by the path to the directory
containing the virtual environment):

+-------------+------------+--------------------------------------------------+
| Platform    | Shell      | Command to activate virtual environment          |
+=============+============+==================================================+
| POSIX       | bash/zsh   | :samp:`$ source {<venv>}/bin/activate`           |
|             +------------+--------------------------------------------------+
|             | fish       | :samp:`$ source {<venv>}/bin/activate.fish`      |
|             +------------+--------------------------------------------------+
|             | csh/tcsh   | :samp:`$ source {<venv>}/bin/activate.csh`       |
|             +------------+--------------------------------------------------+
|             | pwsh       | :samp:`$ {<venv>}/bin/Activate.ps1`              |
+-------------+------------+--------------------------------------------------+
| Windows     | cmd.exe    | :samp:`C:\\> {<venv>}\\Scripts\\activate.bat`    |
|             +------------+--------------------------------------------------+
|             | PowerShell | :samp:`PS C:\\> {<venv>}\\Scripts\\Activate.ps1` |
+-------------+------------+--------------------------------------------------+

.. versionadded:: 3.4
   :program:`fish` and :program:`csh` activation scripts.

.. versionadded:: 3.8
   PowerShell activation scripts installed under POSIX for PowerShell Core
   support.

You don't specifically *need* to activate a virtual environment,
as you can just specify the full path to that environment's
Python interpreter when invoking Python.
Furthermore, all scripts installed in the environment
should be runnable without activating it.

In order to achieve this, scripts installed into virtual environments have
a "shebang" line which points to the environment's Python interpreter,
:samp:`#!/{<path-to-venv>}/bin/python`.
This means that the script will run with that interpreter regardless of the
value of :envvar:`PATH`. On Windows, "shebang" line processing is supported if
you have the :ref:`launcher` installed. Thus, double-clicking an installed
script in a Windows Explorer window should run it with the correct interpreter
without the environment needing to be activated or on the :envvar:`PATH`.

When a virtual environment has been activated, the :envvar:`!VIRTUAL_ENV`
environment variable is set to the path of the environment.
Since explicitly activating a virtual environment is not required to use it,
:envvar:`!VIRTUAL_ENV` cannot be relied upon to determine
whether a virtual environment is being used.

.. warning:: Because scripts installed in environments should not expect the
   environment to be activated, their shebang lines contain the absolute paths
   to their environment's interpreters. Because of this, environments are
   inherently non-portable, in the general case. You should always have a
   simple means of recreating an environment (for example, if you have a
   requirements file ``requirements.txt``, you can invoke ``pip install -r
   requirements.txt`` using the environment's ``pip`` to install all of the
   packages needed by the environment). If for any reason you need to move the
   environment to a new location, you should recreate it at the desired
   location and delete the one at the old location. If you move an environment
   because you moved a parent directory of it, you should recreate the
   environment in its new location. Otherwise, software installed into the
   environment may not work as expected.

You can deactivate a virtual environment by typing ``deactivate`` in your shell.
The exact mechanism is platform-specific and is an internal implementation
detail (typically, a script or shell function will be used).


.. _venv-api:

API
---

.. highlight:: python

The high-level method described above makes use of a simple API which provides
mechanisms for third-party virtual environment creators to customize environment
creation according to their needs, the :class:`EnvBuilder` class.

.. class:: EnvBuilder(system_site_packages=False, clear=False, \
                      symlinks=False, upgrade=False, with_pip=False, \
                      prompt=None, upgrade_deps=False, \
                      *, scm_ignore_files=frozenset())

    The :class:`EnvBuilder` class accepts the following keyword arguments on
    instantiation:

    * *system_site_packages* -- a boolean value indicating that the system Python
      site-packages should be available to the environment (defaults to ``False``).

    * *clear* -- a boolean value which, if true, will delete the contents of
      any existing target directory, before creating the environment.

    * *symlinks* -- a boolean value indicating whether to attempt to symlink the
      Python binary rather than copying.

    * *upgrade* -- a boolean value which, if true, will upgrade an existing
      environment with the running Python - for use when that Python has been
      upgraded in-place (defaults to ``False``).

    * *with_pip* -- a boolean value which, if true, ensures pip is
      installed in the virtual environment. This uses :mod:`ensurepip` with
      the ``--default-pip`` option.

    * *prompt* -- a string to be used after virtual environment is activated
      (defaults to ``None`` which means directory name of the environment would
      be used). If the special string ``"."`` is provided, the basename of the
      current directory is used as the prompt.

    * *upgrade_deps* -- Update the base venv modules to the latest on PyPI

    * *scm_ignore_files* -- Create ignore files based for the specified source
      control managers (SCM) in the iterable. Support is defined by having a
      method named ``create_{scm}_ignore_file``. The only value supported by
      default is ``"git"`` via :meth:`create_git_ignore_file`.


    .. versionchanged:: 3.4
       Added the ``with_pip`` parameter

    .. versionchanged:: 3.6
       Added the ``prompt`` parameter

    .. versionchanged:: 3.9
       Added the ``upgrade_deps`` parameter

    .. versionchanged:: 3.13
       Added the ``scm_ignore_files`` parameter

    :class:`EnvBuilder` may be used as a base class.

    .. method:: create(env_dir)

        Create a virtual environment by specifying the target directory
        (absolute or relative to the current directory) which is to contain the
        virtual environment.  The ``create`` method will either create the
        environment in the specified directory, or raise an appropriate
        exception.

        The ``create`` method of the :class:`EnvBuilder` class illustrates the
        hooks available for subclass customization::

            def create(self, env_dir):
                """
                Create a virtualized Python environment in a directory.
                env_dir is the target directory to create an environment in.
                """
                env_dir = os.path.abspath(env_dir)
                context = self.ensure_directories(env_dir)
                self.create_configuration(context)
                self.setup_python(context)
                self.setup_scripts(context)
                self.post_setup(context)

        Each of the methods :meth:`ensure_directories`,
        :meth:`create_configuration`, :meth:`setup_python`,
        :meth:`setup_scripts` and :meth:`post_setup` can be overridden.

    .. method:: ensure_directories(env_dir)

        Creates the environment directory and all necessary subdirectories that
        don't already exist, and returns a context object.  This context object
        is just a holder for attributes (such as paths) for use by the other
        methods.  If the :class:`EnvBuilder` is created with the arg
        ``clear=True``, contents of the environment directory will be cleared
        and then all necessary subdirectories will be recreated.

        The returned context object is a :class:`types.SimpleNamespace` with the
        following attributes:

        * ``env_dir`` - The location of the virtual environment. Used for
          ``__VENV_DIR__`` in activation scripts (see :meth:`install_scripts`).

        * ``env_name`` - The name of the virtual environment. Used for
          ``__VENV_NAME__`` in activation scripts (see :meth:`install_scripts`).

        * ``prompt`` - The prompt to be used by the activation scripts. Used for
          ``__VENV_PROMPT__`` in activation scripts (see :meth:`install_scripts`).

        * ``executable`` - The underlying Python executable used by the virtual
          environment. This takes into account the case where a virtual environment
          is created from another virtual environment.

        * ``inc_path`` - The include path for the virtual environment.

        * ``lib_path`` - The purelib path for the virtual environment.

        * ``platlib_path`` - The platlib path for the virtual environment.

        * ``bin_path`` - The script path for the virtual environment.

        * ``bin_name`` - The name of the script path relative to the virtual
          environment location. Used for ``__VENV_BIN_NAME__`` in activation
          scripts (see :meth:`install_scripts`).

        * ``env_exe`` - The name of the Python interpreter in the virtual
          environment. Used for ``__VENV_PYTHON__`` in activation scripts
          (see :meth:`install_scripts`).

        * ``env_exec_cmd`` - The name of the Python interpreter, taking into
          account filesystem redirections. This can be used to run Python in
          the virtual environment.


        .. versionchanged:: 3.11
           The *venv*
           :ref:`sysconfig installation scheme <installation_paths>`
           is used to construct the paths of the created directories.

        .. versionchanged:: 3.12
           The attribute ``lib_path`` was added to the context, and the context
           object was documented.

        .. versionchanged:: 3.15
           The attribute ``platlib_path`` was added to the context.

    .. method:: create_configuration(context)

        Creates the ``pyvenv.cfg`` configuration file in the environment.

    .. method:: setup_python(context)

        Creates a copy or symlink to the Python executable in the environment.
        On POSIX systems, if a specific executable ``python3.x`` was used,
        symlinks to ``python`` and ``python3`` will be created pointing to that
        executable, unless files with those names already exist.

    .. method:: setup_scripts(context)

        Installs activation scripts appropriate to the platform into the virtual
        environment.

    .. method:: upgrade_dependencies(context)

       Upgrades the core venv dependency packages (currently :pypi:`pip`)
       in the environment. This is done by shelling out to the
       ``pip`` executable in the environment.

       .. versionadded:: 3.9
       .. versionchanged:: 3.12

          :pypi:`setuptools` is no longer a core venv dependency.

    .. method:: post_setup(context)

        A placeholder method which can be overridden in third party
        implementations to pre-install packages in the virtual environment or
        perform other post-creation steps.

    .. method:: install_scripts(context, path)

        This method can be
        called from :meth:`setup_scripts` or :meth:`post_setup` in subclasses to
        assist in installing custom scripts into the virtual environment.

        *path* is the path to a directory that should contain subdirectories
        ``common``, ``posix``, ``nt``; each containing scripts destined for the
        ``bin`` directory in the environment.  The contents of ``common`` and the
        directory corresponding to :data:`os.name` are copied after some text
        replacement of placeholders:

        * ``__VENV_DIR__`` is replaced with the absolute path of the environment
          directory.

        * ``__VENV_NAME__`` is replaced with the environment name (final path
          segment of environment directory).

        * ``__VENV_PROMPT__`` is replaced with the prompt (the environment
          name surrounded by parentheses and with a following space)

        * ``__VENV_BIN_NAME__`` is replaced with the name of the bin directory
          (either ``bin`` or ``Scripts``).

        * ``__VENV_PYTHON__`` is replaced with the absolute path of the
          environment's executable.

        The directories are allowed to exist (for when an existing environment
        is being upgraded).

    .. method:: create_git_ignore_file(context)

       Creates a ``.gitignore`` file within the virtual environment that causes
       the entire directory to be ignored by the Git source control manager.

       .. versionadded:: 3.13

    .. versionchanged:: 3.7.2
       Windows now uses redirector scripts for ``python[w].exe`` instead of
       copying the actual binaries. In 3.7.2 only :meth:`setup_python` does
       nothing unless running from a build in the source tree.

    .. versionchanged:: 3.7.3
       Windows copies the redirector scripts as part of :meth:`setup_python`
       instead of :meth:`setup_scripts`. This was not the case in 3.7.2.
       When using symlinks, the original executables will be linked.

There is also a module-level convenience function:

.. function:: create(env_dir, system_site_packages=False, clear=False, \
                     symlinks=False, with_pip=False, prompt=None, \
                     upgrade_deps=False, *, scm_ignore_files=frozenset())

    Create an :class:`EnvBuilder` with the given keyword arguments, and call its
    :meth:`~EnvBuilder.create` method with the *env_dir* argument.

    .. versionadded:: 3.3

    .. versionchanged:: 3.4
       Added the *with_pip* parameter

    .. versionchanged:: 3.6
       Added the *prompt* parameter

    .. versionchanged:: 3.9
       Added the *upgrade_deps* parameter

    .. versionchanged:: 3.13
       Added the *scm_ignore_files* parameter

An example of extending ``EnvBuilder``
--------------------------------------

The following script shows how to extend :class:`EnvBuilder` by implementing a
subclass which pre-installs packages into a created virtual environment.

Since :mod:`!venv` bootstraps ``pip`` by default (via :mod:`ensurepip`), the
example uses ``pip`` to install additional packages after the environment is
created::

    import os
    import subprocess
    import sys
    import venv


    class ExtendedEnvBuilder(venv.EnvBuilder):
        """
        This builder installs additional packages into the created
        virtual environment using pip.

        :param packages: A list of packages to install after creating
                         the virtual environment.
        :param verbose: If true, display the output from pip.
        """

        def __init__(self, *args, **kwargs):
            self.packages = kwargs.pop('packages', [])
            self.verbose = kwargs.pop('verbose', False)
            super().__init__(*args, **kwargs)

        def post_setup(self, context):
            """
            Install additional packages into the virtual environment.

            :param context: The information for the virtual environment
                            creation request being processed.
            """
            if self.packages:
                self.install_packages(context)

        def install_packages(self, context):
            """
            Install the specified packages using pip.

            :param context: The information for the virtual environment
                            creation request being processed.
            """
            # Use the pip installed in the virtual environment
            args = [context.env_exec_cmd, '-m', 'pip', 'install']
            if not self.verbose:
                args.append('-q')
            args.extend(self.packages)
            sys.stderr.write('Installing packages: %s ...\n'
                             % ', '.join(self.packages))
            result = subprocess.run(args, capture_output=not self.verbose)
            if result.returncode != 0:
                sys.stderr.write('Package installation failed '
                                 '(exit code %d).\n' % result.returncode)
                if result.stderr:
                    sys.stderr.write(result.stderr.decode('utf-8'))
            else:
                sys.stderr.write('done.\n')


    def main(args=None):
        import argparse

        parser = argparse.ArgumentParser(prog=__name__,
                                         description='Creates virtual Python '
                                                     'environments in one or '
                                                     'more target '
                                                     'directories.')
        parser.add_argument('dirs', metavar='ENV_DIR', nargs='+',
                            help='A directory in which to create the '
                                 'virtual environment.')
        parser.add_argument('--packages', nargs='*', default=[],
                            help='Packages to install after environment '
                                 'creation.')
        parser.add_argument('--system-site-packages', default=False,
                            action='store_true', dest='system_site',
                            help='Give the virtual environment access to the '
                                 'system site-packages dir.')
        if os.name == 'nt':
            use_symlinks = False
        else:
            use_symlinks = True
        parser.add_argument('--symlinks', default=use_symlinks,
                            action='store_true', dest='symlinks',
                            help='Try to use symlinks rather than copies, '
                                 'when symlinks are not the default for '
                                 'the platform.')
        parser.add_argument('--clear', default=False, action='store_true',
                            dest='clear', help='Delete the contents of the '
                                               'virtual environment '
                                               'directory if it already '
                                               'exists, before virtual '
                                               'environment creation.')
        parser.add_argument('--upgrade', default=False, action='store_true',
                            dest='upgrade', help='Upgrade the virtual '
                                                 'environment directory to '
                                                 'use this version of '
                                                 'Python, assuming Python '
                                                 'has been upgraded '
                                                 'in-place.')
        parser.add_argument('--verbose', default=False, action='store_true',
                            dest='verbose', help='Display the output '
                                                 'from pip install.')
        options = parser.parse_args(args)
        if options.upgrade and options.clear:
            raise ValueError('you cannot supply --upgrade and --clear together.')
        builder = ExtendedEnvBuilder(system_site_packages=options.system_site,
                                       clear=options.clear,
                                       symlinks=options.symlinks,
                                       upgrade=options.upgrade,
                                       packages=options.packages,
                                       verbose=options.verbose)
        for d in options.dirs:
            builder.create(d)

    if __name__ == '__main__':
        rc = 1
        try:
            main()
            rc = 0
        except Exception as e:
            print('Error: %s' % e, file=sys.stderr)
        sys.exit(rc)
