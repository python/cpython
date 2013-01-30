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

    * ``clear`` -- a Boolean value which, if True, will delete the contents of
      any existing target directory, before creating the environment.

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

An example of extending ``EnvBuilder``
--------------------------------------

The following script shows how to extend :class:`EnvBuilder` by implementing a
subclass which installs Distribute and pip into a created venv::

    import os
    import os.path
    from subprocess import Popen, PIPE
    import sys
    from threading import Thread
    from urllib.parse import urlparse
    from urllib.request import urlretrieve
    import venv

    class DistributeEnvBuilder(venv.EnvBuilder):
        """
        This builder installs Distribute and pip so that you can pip or
        easy_install other packages into the created environment.

        :param nodist: If True, Distribute is not installed into the created
                       environment.
        :param nopip: If True, pip is not installed into the created
                      environment.
        :param progress: If Distribute or pip are installed, the progress of the
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
            environment being created.

            :param context: The information for the environment creation request
                            being processed.
            """
            if not self.nodist:
                self.install_distribute(context)
            if not self.nopip:
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

        def install_script(self, context, name, url):
            _, _, path, _, _, _ = urlparse(url)
            fn = os.path.split(path)[-1]
            binpath = context.bin_path
            distpath = os.path.join(binpath, fn)
            # Download script into the env's binaries folder
            urlretrieve(url, distpath)
            progress = self.progress
            if progress is not None:
                progress('Installing %s' %name, 'main')
            else:
                sys.stderr.write('Installing %s ' % name)
                sys.stderr.flush()
            # Install in the env
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

        def install_distribute(self, context):
            """
            Install Distribute in the environment.

            :param context: The information for the environment creation request
                            being processed.
            """
            url = 'http://python-distribute.org/distribute_setup.py'
            self.install_script(context, 'distribute', url)
            # clear up the distribute archive which gets downloaded
            pred = lambda o: o.startswith('distribute-') and o.endswith('.tar.gz')
            files = filter(pred, os.listdir(context.bin_path))
            for f in files:
                f = os.path.join(context.bin_path, f)
                os.unlink(f)

        def install_pip(self, context):
            """
            Install pip in the environment.

            :param context: The information for the environment creation request
                            being processed.
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
                                help='A directory to create the environment in.')
            parser.add_argument('--no-distribute', default=False,
                                action='store_true', dest='nodist',
                                help="Don't install Distribute in the virtual "
                                     "environment.")
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
                                                   'environment directory if it '
                                                   'already exists, before '
                                                   'environment creation.')
            parser.add_argument('--upgrade', default=False, action='store_true',
                                dest='upgrade', help='Upgrade the environment '
                                                   'directory to use this version '
                                                   'of Python, assuming Python '
                                                   'has been upgraded in-place.')
            parser.add_argument('--verbose', default=False, action='store_true',
                                dest='verbose', help='Display the output '
                                                   'from the scripts which '
                                                   'install Distribute and pip.')
            options = parser.parse_args(args)
            if options.upgrade and options.clear:
                raise ValueError('you cannot supply --upgrade and --clear together.')
            builder = DistributeEnvBuilder(system_site_packages=options.system_site,
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
<https://gist.github.com/4673395>`_.
