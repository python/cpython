.. _tools-and-scripts:

Additional Tools and Scripts
============================

pyvenv - Creating virtual environments
--------------------------------------

Creation of :ref:`virtual environments <venv-def>` is done by executing the
``pyvenv`` script::

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

    usage: pyvenv [-h] [--system-site-packages] [--symlinks] [--clear]
                  [--upgrade] ENV_DIR [ENV_DIR ...]

    Creates virtual Python environments in one or more target directories.

    positional arguments:
      ENV_DIR             A directory to create the environment in.

    optional arguments:
      -h, --help             show this help message and exit
      --system-site-packages Give access to the global site-packages dir to the
                             virtual environment.
      --symlinks             Try to use symlinks rather than copies, when symlinks
                             are not the default for the platform.
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

Once a venv has been created, it can be "activated" using a script in the
venv's binary directory. The invocation of the script is platform-specific: on
a Posix platform, you would typically do::

    $ source <venv>/bin/activate

whereas on Windows, you might do::

    C:\> <venv>/Scripts/activate

if you are using the ``cmd.exe`` shell, or perhaps::

    PS C:\> <venv>/Scripts/Activate.ps1

if you use PowerShell.

You don't specifically *need* to activate an environment; activation just
prepends the venv's binary directory to your path, so that "python" invokes the
venv's Python interpreter and you can run installed scripts without having to
use their full path. However, all scripts installed in a venv should be
runnable without activating it, and run with the venv's Python automatically.

You can deactivate a venv by typing "deactivate" in your shell. The exact
mechanism is platform-specific: for example, the Bash activation script defines
a "deactivate" function, whereas on Windows there are separate scripts called
``deactivate.bat`` and ``Deactivate.ps1`` which are installed when the venv is
created.

