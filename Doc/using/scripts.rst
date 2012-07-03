.. _tools-and-scripts:

Additional Tools and Scripts
============================

pyvenv - Creating virtual environments
--------------------------------------

Creation of virtual environments is done by executing the ``pyvenv``
script::

    pyvenv /path/to/new/virtual/environment

Running this command creates the target directory (creating any parent
directories that don't exist already) and places a ``pyvenv.cfg`` file
in it with a ``home`` key pointing to the Python installation the
command was run from.  It also creates a ``bin`` (or ``Scripts`` on
Windows) subdirectory containing a copy of the ``python`` binary (or
binaries, in the case of Windows).
It also creates an (initially empty) ``lib/pythonX.Y/site-packages``
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

If the target directory already exists an error will be raised, unless
the ``--clear`` or ``--upgrade`` option was provided.

The created ``pyvenv.cfg`` file also includes the
``include-system-site-packages`` key, set to ``true`` if ``venv`` is
run with the ``--system-site-packages`` option, ``false`` otherwise.

Multiple paths can be given to ``pyvenv``, in which case an identical
virtualenv will be created, according to the given options, at each
provided path.

.. note:: A virtual environment (also called a ``venv``) is a Python
   environment such that the Python interpreter, libraries and scripts
   installed into it are isolated from those installed in other virtual
   environments, and (by default) any libraries installed in a "system" Python,
   i.e. one which is installed as part of your operating system.

   A venv is a directory tree which contains Python executable files and
   other files which indicate that it is a venv.

   Common installation tools such as ``distribute`` and ``pip`` work as
   expected with venvs - i.e. when a venv is active, they install Python
   packages into the venv without needing to be told to do so explicitly.

   When a venv is active (i.e. the venv's Python interpreter is running), the
   attributes :attr:`sys.prefix` and :attr:`sys.exec_prefix` point to the base
   directory of the venv, whereas :attr:`sys.base_prefix` and
   :attr:`sys.base_exec_prefix` point to the non-venv Python installation
   which was used to create the venv. If a venv is not active, then
   :attr:`sys.prefix` is the same as :attr:`sys.base_prefix` and
   :attr:`sys.exec_prefix` is the same as :attr:`sys.base_exec_prefix` (they
   all point to a non-venv Python installation).

