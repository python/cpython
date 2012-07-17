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

.. include:: /using/venv-create.inc


.. _venv-def:

.. note:: A virtual environment (also called a ``venv``) is a Python
   environment such that the Python interpreter, libraries and scripts
   installed into it are isolated from those installed in other virtual
   environments, and (by default) any libraries installed in a "system" Python,
   i.e. one which is installed as part of your operating system.

   A venv is a directory tree which contains Python executable files and
   other files which indicate that it is a venv.

   Common installation tools such as ``Distribute`` and ``pip`` work as
   expected with venvs - i.e. when a venv is active, they install Python
   packages into the venv without needing to be told to do so explicitly.
   Of course, you need to install them into the venv first: this could be
   done by running ``distribute_setup.py`` with the venv activated,
   followed by running ``easy_install pip``. Alternatively, you could download
   the source tarballs and run ``python setup.py install`` after unpacking,
   with the venv activated.

   When a venv is active (i.e. the venv's Python interpreter is running), the
   attributes :attr:`sys.prefix` and :attr:`sys.exec_prefix` point to the base
   directory of the venv, whereas :attr:`sys.base_prefix` and
   :attr:`sys.base_exec_prefix` point to the non-venv Python installation
   which was used to create the venv. If a venv is not active, then
   :attr:`sys.prefix` is the same as :attr:`sys.base_prefix` and
   :attr:`sys.exec_prefix` is the same as :attr:`sys.base_exec_prefix` (they
   all point to a non-venv Python installation).


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
      Unix systems, but ``False`` on Windows.

    * ``upgrade`` -- a Boolean value which, if True, will upgrade an existing
      environment with the running Python - for use when that Python has been
      upgraded in-place (defaults to ``False``).



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
