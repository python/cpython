:mod:`venv` --- Creation of virtual environments
================================================

.. module:: venv
   :synopsis: Creation of virtual environments.
.. moduleauthor:: Vinay Sajip <vinay_sajip@yahoo.co.uk>
.. sectionauthor:: Vinay Sajip <vinay_sajip@yahoo.co.uk>


.. index:: pair: Environments; virtual

.. versionadded:: 3.3

**Source code:** :source:`Lib/venv.py`

--------------

The :mod:`venv` module provides support for creating lightweight "virtual
environments" with their own site directories, optionally isolated from system
site directories.  Each virtual environment has its own Python binary (allowing
creation of environments with various Python versions) and can have its own
independent set of installed Python packages in its site directories.


Creating virtual environments
-----------------------------

Creation of virtual environments is simplest executing the ``pyvenv`` script::

    pyvenv /path/to/new/virtual/environment

Running this command creates the target directory (creating any parent
directories that don't exist already) and places a ``pyvenv.cfg`` file in it
with a ``home`` key pointing to the Python installation the command was run
from.  It also creates a ``bin`` (or ``Scripts`` on Windows) subdirectory
containing a copy of the ``python`` binary (or binaries, in the case of
Windows).  It also creates an (initially empty) ``lib/pythonX.Y/site-packages``
subdirectory (on Windows, this is ``Lib\site-packages``).

.. highlight:: none

On Windows, you may have to invoke the ``pyvenv`` script as follows, if you
don't have the relevant PATH and PATHEXT settings::

    c:\Temp>c:\Python33\python c:\Python33\Tools\Scripts\pyvenv.py myenv

or equivalently::

    c:\Temp>c:\Python33\python -m venv myenv

The command, if run with ``-h``, will show the available options::

    usage: pyvenv [-h] [--system-site-packages] [--symlink] [--clear]
                  [--upgrade] ENV_DIR [ENV_DIR ...]

    Creates virtual Python environments in one or more target directories.

    positional arguments:
      ENV_DIR             A directory to create the environment in.

    optional arguments:
      -h, --help             show this help message and exit
      --system-site-packages Give access to the global site-packages dir to the
                             virtual environment.
      --symlink              Attempt to symlink rather than copy.
      --clear                Delete the environment directory if it already exists.
                             If not specified and the directory exists, an error is
                             raised.
      --upgrade              Upgrade the environment directory to use this version
                             of Python, assuming Python has been upgraded in-place.

If the target directory already exists an error will be raised, unless the
``--clear`` or ``--upgrade`` option was provided.

The created ``pyvenv.cfg`` file also includes the
``include-system-site-packages`` key, set to ``true`` if ``venv`` is run with
the ``--system-site-packages`` option, ``false`` otherwise.

Multiple paths can be given to ``pyvenv``, in which case an identical virtualenv
will be created, according to the given options, at each provided path.


API
---

.. highlight:: python

The high-level method described above makes use of a simple API which provides
mechanisms for third-party virtual environment creators to customize environment
creation according to their needs, the :class:`EnvBuilder` class.

.. class:: EnvBuilder(system_site_packages=False, clear=False, symlinks=False, upgrade=False)

    The :class:`EnvBuilder` class accepts the following keyword arguments on
    instantiation:

    * ``system_site_packages`` -- a Boolean value indicating that the system Python
      site-packages should be available to the environment (defaults to ``False``).

    * ``clear`` -- a Boolean value which, if True, will delete any existing target
      directory instead of raising an exception (defaults to ``False``).

    * ``symlinks`` -- a Boolean value indicating whether to attempt to symlink the
      Python binary (and any necessary DLLs or other binaries,
      e.g. ``pythonw.exe``), rather than copying. Defaults to ``True`` on Linux and
      Unix systems, but ``False`` on Windows and Mac OS X.

    .. XXX it also takes "upgrade"!


    Creators of third-party virtual environment tools will be free to use the
    provided ``EnvBuilder`` class as a base class.

    The returned env-builder is an object which has a method, ``create``:

    .. method:: create(env_dir)

        This method takes as required argument the path (absolute or relative to
        the current directory) of the target directory which is to contain the
        virtual environment.  The ``create`` method will either create the
        environment in the specified directory, or raise an appropriate
        exception.

        The ``create`` method of the ``EnvBuilder`` class illustrates the hooks
        available for subclass customization::

            def create(self, env_dir):
                """
                Create a virtualized Python environment in a directory.
                env_dir is the target directory to create an environment in.
                """
                env_dir = os.path.abspath(env_dir)
                context = self.create_directories(env_dir)
                self.create_configuration(context)
                self.setup_python(context)
                self.setup_scripts(context)
                self.post_setup(context)

        Each of the methods :meth:`create_directories`,
        :meth:`create_configuration`, :meth:`setup_python`,
        :meth:`setup_scripts` and :meth:`post_setup` can be overridden.

    .. method:: create_directories(env_dir)

        Creates the environment directory and all necessary directories, and
        returns a context object.  This is just a holder for attributes (such as
        paths), for use by the other methods.

    .. method:: create_configuration(context)

        Creates the ``pyvenv.cfg`` configuration file in the environment.

    .. method:: setup_python(context)

        Creates a copy of the Python executable (and, under Windows, DLLs) in
        the environment.

    .. method:: setup_scripts(context)

        Installs activation scripts appropriate to the platform into the virtual
        environment.

    .. method:: post_setup(context)

        A placeholder method which can be overridden in third party
        implementations to pre-install packages in the virtual environment or
        perform other post-creation steps.

    In addition, :class:`EnvBuilder` provides this utility method that can be
    called from :meth:`setup_scripts` or :meth:`post_setup` in subclasses to
    assist in installing custom scripts into the virtual environment.

    .. method:: install_scripts(context, path)

        *path* is the path to a directory that should contain subdirectories
        "common", "posix", "nt", each containing scripts destined for the bin
        directory in the environment.  The contents of "common" and the
        directory corresponding to :data:`os.name` are copied after some text
        replacement of placeholders:

        * ``__VENV_DIR__`` is replaced with the absolute path of the environment
          directory.

        * ``__VENV_NAME__`` is replaced with the environment name (final path
          segment of environment directory).

        * ``__VENV_BIN_NAME__`` is replaced with the name of the bin directory
          (either ``bin`` or ``Scripts``).

        * ``__VENV_PYTHON__`` is replaced with the absolute path of the
          environment's executable.


There is also a module-level convenience function:

.. function:: create(env_dir, system_site_packages=False, clear=False, symlinks=False)

    Create an :class:`EnvBuilder` with the given keyword arguments, and call its
    :meth:`~EnvBuilder.create` method with the *env_dir* argument.
