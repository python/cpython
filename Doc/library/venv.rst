:mod:`venv` --- Creation of virtual environments
================================================

.. module:: venv
   :synopsis: Creation of virtual environments.

.. moduleauthor:: Vinay Sajip <vinay_sajip@yahoo.co.uk>
.. sectionauthor:: Vinay Sajip <vinay_sajip@yahoo.co.uk>

.. versionadded:: 3.3

**Source code:** :source:`Lib/venv/`

.. index:: pair: Environments; virtual

--------------

The :mod:`venv` module provides support for creating lightweight "virtual
environments" with their own site directories, optionally isolated from system
site directories.  Each virtual environment has its own Python binary (which
matches the version of the binary that was used to create this environment) and
can have its own independent set of installed Python packages in its site
directories.

See :pep:`405` for more information about Python virtual environments.

.. seealso::

   `Python Packaging User Guide: Creating and using virtual environments
   <https://packaging.python.org/installing/#creating-virtual-environments>`__


Creating virtual environments
-----------------------------

.. include:: /using/venv-create.inc


.. _venv-def:

.. note:: A virtual environment is a Python environment such that the Python
   interpreter, libraries and scripts installed into it are isolated from those
   installed in other virtual environments, and (by default) any libraries
   installed in a "system" Python, i.e., one which is installed as part of your
   operating system.

   A virtual environment is a directory tree which contains Python executable
   files and other files which indicate that it is a virtual environment.

   Common installation tools such as setuptools_ and pip_ work as
   expected with virtual environments. In other words, when a virtual
   environment is active, they install Python packages into the virtual
   environment without needing to be told to do so explicitly.

   When a virtual environment is active (i.e., the virtual environment's Python
   interpreter is running), the attributes :attr:`sys.prefix` and
   :attr:`sys.exec_prefix` point to the base directory of the virtual
   environment, whereas :attr:`sys.base_prefix` and
   :attr:`sys.base_exec_prefix` point to the non-virtual environment Python
   installation which was used to create the virtual environment. If a virtual
   environment is not active, then :attr:`sys.prefix` is the same as
   :attr:`sys.base_prefix` and :attr:`sys.exec_prefix` is the same as
   :attr:`sys.base_exec_prefix` (they all point to a non-virtual environment
   Python installation).

   When a virtual environment is active, any options that change the
   installation path will be ignored from all :mod:`distutils` configuration
   files to prevent projects being inadvertently installed outside of the
   virtual environment.

   When working in a command shell, users can make a virtual environment active
   by running an ``activate`` script in the virtual environment's executables
   directory (the precise filename and command to use the file is
   shell-dependent), which prepends the virtual environment's directory for
   executables to the ``PATH`` environment variable for the running shell. There
   should be no need in other circumstances to activate a virtual
   environment; scripts installed into virtual environments have a "shebang"
   line which points to the virtual environment's Python interpreter. This means
   that the script will run with that interpreter regardless of the value of
   ``PATH``. On Windows, "shebang" line processing is supported if you have the
   Python Launcher for Windows installed (this was added to Python in 3.3 - see
   :pep:`397` for more details). Thus, double-clicking an installed script in a
   Windows Explorer window should run the script with the correct interpreter
   without there needing to be any reference to its virtual environment in
   ``PATH``.


.. _venv-api:

API
---

.. highlight:: python

The high-level method described above makes use of a simple API which provides
mechanisms for third-party virtual environment creators to customize environment
creation according to their needs, the :class:`EnvBuilder` class.

.. class:: EnvBuilder(system_site_packages=False, clear=False, \
                      symlinks=False, upgrade=False, with_pip=False, \
                      prompt=None)

    The :class:`EnvBuilder` class accepts the following keyword arguments on
    instantiation:

    * ``system_site_packages`` -- a Boolean value indicating that the system Python
      site-packages should be available to the environment (defaults to ``False``).

    * ``clear`` -- a Boolean value which, if true, will delete the contents of
      any existing target directory, before creating the environment.

    * ``symlinks`` -- a Boolean value indicating whether to attempt to symlink the
      Python binary rather than copying.

    * ``upgrade`` -- a Boolean value which, if true, will upgrade an existing
      environment with the running Python - for use when that Python has been
      upgraded in-place (defaults to ``False``).

    * ``with_pip`` -- a Boolean value which, if true, ensures pip is
      installed in the virtual environment. This uses :mod:`ensurepip` with
      the ``--default-pip`` option.

    * ``prompt`` -- a String to be used after virtual environment is activated
      (defaults to ``None`` which means directory name of the environment would
      be used).

    .. versionchanged:: 3.4
       Added the ``with_pip`` parameter

    .. versionadded:: 3.6
       Added the ``prompt`` parameter

    Creators of third-party virtual environment tools will be free to use the
    provided :class:`EnvBuilder` class as a base class.

    The returned env-builder is an object which has a method, ``create``:

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

        Creates the environment directory and all necessary directories, and
        returns a context object.  This is just a holder for attributes (such as
        paths), for use by the other methods. The directories are allowed to
        exist already, as long as either ``clear`` or ``upgrade`` were
        specified to allow operating on an existing environment directory.

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

    .. method:: post_setup(context)

        A placeholder method which can be overridden in third party
        implementations to pre-install packages in the virtual environment or
        perform other post-creation steps.

    .. versionchanged:: 3.7.2
       Windows now uses redirector scripts for ``python[w].exe`` instead of
       copying the actual binaries. In 3.7.2 only :meth:`setup_python` does
       nothing unless running from a build in the source tree.

    .. versionchanged:: 3.7.3
       Windows copies the redirector scripts as part of :meth:`setup_python`
       instead of :meth:`setup_scripts`. This was not the case in 3.7.2.
       When using symlinks, the original executables will be linked.

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

        * ``__VENV_PROMPT__`` is replaced with the prompt (the environment
          name surrounded by parentheses and with a following space)

        * ``__VENV_BIN_NAME__`` is replaced with the name of the bin directory
          (either ``bin`` or ``Scripts``).

        * ``__VENV_PYTHON__`` is replaced with the absolute path of the
          environment's executable.

        The directories are allowed to exist (for when an existing environment
        is being upgraded).

There is also a module-level convenience function:

.. function:: create(env_dir, system_site_packages=False, clear=False, \
                     symlinks=False, with_pip=False, prompt=None)

    Create an :class:`EnvBuilder` with the given keyword arguments, and call its
    :meth:`~EnvBuilder.create` method with the *env_dir* argument.

    .. versionadded:: 3.3

    .. versionchanged:: 3.4
       Added the ``with_pip`` parameter

    .. versionchanged:: 3.6
       Added the ``prompt`` parameter

An example of extending ``EnvBuilder``
--------------------------------------

The following script shows how to extend :class:`EnvBuilder` by implementing a
subclass which installs setuptools and pip into a created virtual environment::

    import os
    import os.path
    from subprocess import Popen, PIPE
    import sys
    from threading import Thread
    from urllib.parse import urlparse
    from urllib.request import urlretrieve
    import venv

    class ExtendedEnvBuilder(venv.EnvBuilder):
        """
        This builder installs setuptools and pip so that you can pip or
        easy_install other packages into the created virtual environment.

        :param nodist: If true, setuptools and pip are not installed into the
                       created virtual environment.
        :param nopip: If true, pip is not installed into the created
                      virtual environment.
        :param progress: If setuptools or pip are installed, the progress of the
                         installation can be monitored by passing a progress
                         callable. If specified, it is called with two
                         arguments: a string indicating some progress, and a
                         context indicating where the string is coming from.
                         The context argument can have one of three values:
                         'main', indicating that it is called from virtualize()
                         itself, and 'stdout' and 'stderr', which are obtained
                         by reading lines from the output streams of a subprocess
                         which is used to install the app.

                         If a callable is not specified, default progress
                         information is output to sys.stderr.
        """

        def __init__(self, *args, **kwargs):
            self.nodist = kwargs.pop('nodist', False)
            self.nopip = kwargs.pop('nopip', False)
            self.progress = kwargs.pop('progress', None)
            self.verbose = kwargs.pop('verbose', False)
            super().__init__(*args, **kwargs)

        def post_setup(self, context):
            """
            Set up any packages which need to be pre-installed into the
            virtual environment being created.

            :param context: The information for the virtual environment
                            creation request being processed.
            """
            os.environ['VIRTUAL_ENV'] = context.env_dir
            if not self.nodist:
                self.install_setuptools(context)
            # Can't install pip without setuptools
            if not self.nopip and not self.nodist:
                self.install_pip(context)

        def reader(self, stream, context):
            """
            Read lines from a subprocess' output stream and either pass to a progress
            callable (if specified) or write progress information to sys.stderr.
            """
            progress = self.progress
            while True:
                s = stream.readline()
                if not s:
                    break
                if progress is not None:
                    progress(s, context)
                else:
                    if not self.verbose:
                        sys.stderr.write('.')
                    else:
                        sys.stderr.write(s.decode('utf-8'))
                    sys.stderr.flush()
            stream.close()

        def install_script(self, context, name, url):
            _, _, path, _, _, _ = urlparse(url)
            fn = os.path.split(path)[-1]
            binpath = context.bin_path
            distpath = os.path.join(binpath, fn)
            # Download script into the virtual environment's binaries folder
            urlretrieve(url, distpath)
            progress = self.progress
            if self.verbose:
                term = '\n'
            else:
                term = ''
            if progress is not None:
                progress('Installing %s ...%s' % (name, term), 'main')
            else:
                sys.stderr.write('Installing %s ...%s' % (name, term))
                sys.stderr.flush()
            # Install in the virtual environment
            args = [context.env_exe, fn]
            p = Popen(args, stdout=PIPE, stderr=PIPE, cwd=binpath)
            t1 = Thread(target=self.reader, args=(p.stdout, 'stdout'))
            t1.start()
            t2 = Thread(target=self.reader, args=(p.stderr, 'stderr'))
            t2.start()
            p.wait()
            t1.join()
            t2.join()
            if progress is not None:
                progress('done.', 'main')
            else:
                sys.stderr.write('done.\n')
            # Clean up - no longer needed
            os.unlink(distpath)

        def install_setuptools(self, context):
            """
            Install setuptools in the virtual environment.

            :param context: The information for the virtual environment
                            creation request being processed.
            """
            url = 'https://bitbucket.org/pypa/setuptools/downloads/ez_setup.py'
            self.install_script(context, 'setuptools', url)
            # clear up the setuptools archive which gets downloaded
            pred = lambda o: o.startswith('setuptools-') and o.endswith('.tar.gz')
            files = filter(pred, os.listdir(context.bin_path))
            for f in files:
                f = os.path.join(context.bin_path, f)
                os.unlink(f)

        def install_pip(self, context):
            """
            Install pip in the virtual environment.

            :param context: The information for the virtual environment
                            creation request being processed.
            """
            url = 'https://raw.github.com/pypa/pip/master/contrib/get-pip.py'
            self.install_script(context, 'pip', url)

    def main(args=None):
        compatible = True
        if sys.version_info < (3, 3):
            compatible = False
        elif not hasattr(sys, 'base_prefix'):
            compatible = False
        if not compatible:
            raise ValueError('This script is only for use with '
                             'Python 3.3 or later')
        else:
            import argparse

            parser = argparse.ArgumentParser(prog=__name__,
                                             description='Creates virtual Python '
                                                         'environments in one or '
                                                         'more target '
                                                         'directories.')
            parser.add_argument('dirs', metavar='ENV_DIR', nargs='+',
                                help='A directory in which to create the
                                     'virtual environment.')
            parser.add_argument('--no-setuptools', default=False,
                                action='store_true', dest='nodist',
                                help="Don't install setuptools or pip in the "
                                     "virtual environment.")
            parser.add_argument('--no-pip', default=False,
                                action='store_true', dest='nopip',
                                help="Don't install pip in the virtual "
                                     "environment.")
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
                                                   'from the scripts which '
                                                   'install setuptools and pip.')
            options = parser.parse_args(args)
            if options.upgrade and options.clear:
                raise ValueError('you cannot supply --upgrade and --clear together.')
            builder = ExtendedEnvBuilder(system_site_packages=options.system_site,
                                           clear=options.clear,
                                           symlinks=options.symlinks,
                                           upgrade=options.upgrade,
                                           nodist=options.nodist,
                                           nopip=options.nopip,
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


This script is also available for download `online
<https://gist.github.com/vsajip/4673395>`_.


.. _setuptools: https://pypi.org/project/setuptools/
.. _pip: https://pypi.org/project/pip/
