Creation of :ref:`virtual environments <venv-def>` is done by executing the
``pyvenv`` script::

    pyvenv /path/to/new/virtual/environment

Running this command creates the target directory (creating any parent
directories that don't exist already) and places a ``pyvenv.cfg`` file in it
with a ``home`` key pointing to the Python installation the command was run
from.  It also creates a ``bin`` (or ``Scripts`` on Windows) subdirectory
containing a copy of the ``python`` binary (or binaries, in the case of
Windows).  It also creates an (initially empty) ``lib/pythonX.Y/site-packages``
subdirectory (on Windows, this is ``Lib\site-packages``).

.. seealso::

   `Python Packaging User Guide: Creating and using virtual environments
   <https://packaging.python.org/en/latest/installing.html#virtual-environments>`__

.. highlight:: none

On Windows, you may have to invoke the ``pyvenv`` script as follows, if you
don't have the relevant PATH and PATHEXT settings::

    c:\Temp>c:\Python35\python c:\Python35\Tools\Scripts\pyvenv.py myenv

or equivalently::

    c:\Temp>c:\Python35\python -m venv myenv

The command, if run with ``-h``, will show the available options::

    usage: venv [-h] [--system-site-packages] [--symlinks] [--clear]
                [--upgrade] [--without-pip] ENV_DIR [ENV_DIR ...]

    Creates virtual Python environments in one or more target directories.

    positional arguments:
      ENV_DIR             A directory to create the environment in.

    optional arguments:
      -h, --help             show this help message and exit
      --system-site-packages Give access to the global site-packages dir to the
                             virtual environment.
      --symlinks             Try to use symlinks rather than copies, when symlinks
                             are not the default for the platform.
      --copies               Try to use copies rather than symlinks, even when
                             symlinks are the default for the platform.
      --clear                Delete the environment directory if it already exists.
                             If not specified and the directory exists, an error is
                             raised.
      --upgrade              Upgrade the environment directory to use this version
                             of Python, assuming Python has been upgraded in-place.
      --without-pip          Skips installing or upgrading pip in the virtual
                             environment (pip is bootstrapped by default)

Depending on how the ``venv`` functionality has been invoked, the usage message
may vary slightly, e.g. referencing ``pyvenv`` rather than ``venv``.

.. versionchanged:: 3.4
   Installs pip by default, added the ``--without-pip``  and ``--copies``
   options

.. versionchanged:: 3.4
   In earlier versions, if the target directory already existed, an error was
   raised, unless the ``--clear`` or ``--upgrade`` option was provided. Now,
   if an existing directory is specified, its contents are removed and
   the directory is processed as if it had been newly created.

The created ``pyvenv.cfg`` file also includes the
``include-system-site-packages`` key, set to ``true`` if ``venv`` is
run with the ``--system-site-packages`` option, ``false`` otherwise.

Unless the ``--without-pip`` option is given, :mod:`ensurepip` will be
invoked to bootstrap ``pip`` into the virtual environment.

Multiple paths can be given to ``pyvenv``, in which case an identical
virtualenv will be created, according to the given options, at each
provided path.

Once a venv has been created, it can be "activated" using a script in the
venv's binary directory. The invocation of the script is platform-specific:

+-------------+-----------------+-----------------------------------------+
| Platform    | Shell           | Command to activate virtual environment |
+=============+=================+=========================================+
| Posix       | bash/zsh        | $ source <venv>/bin/activate            |
+-------------+-----------------+-----------------------------------------+
|             | fish            | $ . <venv>/bin/activate.fish            |
+-------------+-----------------+-----------------------------------------+
|             | csh/tcsh        | $ source <venv>/bin/activate.csh        |
+-------------+-----------------+-----------------------------------------+
| Windows     | cmd.exe         | C:\> <venv>/Scripts/activate.bat        |
+-------------+-----------------+-----------------------------------------+
|             | PowerShell      | PS C:\> <venv>/Scripts/Activate.ps1     |
+-------------+-----------------+-----------------------------------------+

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

.. versionadded:: 3.4
   ``fish`` and ``csh`` activation scripts.
