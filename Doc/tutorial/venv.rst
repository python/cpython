
.. _tut-venv:

*********************************
Virtual Environments and Packages
*********************************

Introduction
============

Python applications will often use packages and modules that don't
come as part of the standard library.  Applications will sometimes
need a specific version of a library, because the application may
require that a particular bug has been fixed or the application may be
written using an obsolete version of the library's interface.

This means it may not be possible for one Python installation to meet
the requirements of every application.  If application A needs version
1.0 of a particular module but application B needs version 2.0, then
the requirements are in conflict and installing either version 1.0 or 2.0
will leave one application unable to run.

The solution for this problem is to create a :term:`virtual environment`, a
self-contained directory tree that contains a Python installation for a
particular version of Python, plus a number of additional packages.

Different applications can then use different virtual environments.
To resolve the earlier example of conflicting requirements,
application A can have its own virtual environment with version 1.0
installed while application B has another virtual environment with version 2.0.
If application B requires a library be upgraded to version 3.0, this will
not affect application A's environment.


Creating Virtual Environments
=============================

The module used to create and manage virtual environments is called
:mod:`venv`.  :mod:`venv` will install the Python version from which
the command was run (as reported by the :option:`--version` option).
For instance, executing the command with ``python3.12`` will install
version 3.12.

To create a virtual environment, decide upon a directory where you want to
place it, and run the :mod:`venv` module as a script with the directory path::

   python -m venv tutorial-env

This will create the ``tutorial-env`` directory if it doesn't exist,
and also create directories inside it containing a copy of the Python
interpreter and various supporting files.

A common directory location for a virtual environment is ``.venv``.
This name keeps the directory typically hidden in your shell and thus
out of the way while giving it a name that explains why the directory
exists. It also prevents clashing with ``.env`` environment variable
definition files that some tooling supports.

Once you've created a virtual environment, you may activate it.

On Windows, run::

  tutorial-env\Scripts\activate

On Unix or MacOS, run::

  source tutorial-env/bin/activate

(This script is written for the bash shell.  If you use the
:program:`csh` or :program:`fish` shells, there are alternate
``activate.csh`` and ``activate.fish`` scripts you should use
instead.)

Activating the virtual environment will change your shell's prompt to show what
virtual environment you're using, and modify the environment so that running
``python`` will get you that particular version and installation of Python.
For example:

.. code-block:: console

  $ source ~/envs/tutorial-env/bin/activate
  (tutorial-env) $ python
  Python 3.5.1 (default, May  6 2016, 10:59:36)
    ...
  >>> import sys
  >>> sys.path
  ['', '/usr/local/lib/python35.zip', ...,
  '~/envs/tutorial-env/lib/python3.5/site-packages']
  >>>

To deactivate a virtual environment, type::

    deactivate

into the terminal.

Managing Packages with pip
==========================

You can install, upgrade, and remove packages using a program called
:program:`pip`.  By default ``pip`` will install packages from the `Python
Package Index <https://pypi.org>`_.  You can browse the Python
Package Index by going to it in your web browser.

``pip`` has a number of subcommands: "install", "uninstall",
"freeze", etc.  (Consult the :ref:`installing-index` guide for
complete documentation for ``pip``.)

You can install the latest version of a package by specifying a package's name:

.. code-block:: console

  (tutorial-env) $ python -m pip install novas
  Collecting novas
    Downloading novas-3.1.1.3.tar.gz (136kB)
  Installing collected packages: novas
    Running setup.py install for novas
  Successfully installed novas-3.1.1.3

You can also install a specific version of a package by giving the
package name  followed by ``==`` and the version number:

.. code-block:: console

  (tutorial-env) $ python -m pip install requests==2.6.0
  Collecting requests==2.6.0
    Using cached requests-2.6.0-py2.py3-none-any.whl
  Installing collected packages: requests
  Successfully installed requests-2.6.0

If you re-run this command, ``pip`` will notice that the requested
version is already installed and do nothing.  You can supply a
different version number to get that version, or you can run ``python
-m pip install --upgrade`` to upgrade the package to the latest version:

.. code-block:: console

  (tutorial-env) $ python -m pip install --upgrade requests
  Collecting requests
  Installing collected packages: requests
    Found existing installation: requests 2.6.0
      Uninstalling requests-2.6.0:
        Successfully uninstalled requests-2.6.0
  Successfully installed requests-2.7.0

``python -m pip uninstall`` followed by one or more package names will
remove the packages from the virtual environment.

``python -m pip show`` will display information about a particular package:

.. code-block:: console

  (tutorial-env) $ python -m pip show requests
  ---
  Metadata-Version: 2.0
  Name: requests
  Version: 2.7.0
  Summary: Python HTTP for Humans.
  Home-page: http://python-requests.org
  Author: Kenneth Reitz
  Author-email: me@kennethreitz.com
  License: Apache 2.0
  Location: /Users/akuchling/envs/tutorial-env/lib/python3.4/site-packages
  Requires:

``python -m pip list`` will display all of the packages installed in
the virtual environment:

.. code-block:: console

  (tutorial-env) $ python -m pip list
  novas (3.1.1.3)
  numpy (1.9.2)
  pip (7.0.3)
  requests (2.7.0)
  setuptools (16.0)

``python -m pip freeze`` will produce a similar list of the installed packages,
but the output uses the format that ``python -m pip install`` expects.
A common convention is to put this list in a ``requirements.txt`` file:

.. code-block:: console

  (tutorial-env) $ python -m pip freeze > requirements.txt
  (tutorial-env) $ cat requirements.txt
  novas==3.1.1.3
  numpy==1.9.2
  requests==2.7.0

The ``requirements.txt`` can then be committed to version control and
shipped as part of an application.  Users can then install all the
necessary packages with ``install -r``:

.. code-block:: console

  (tutorial-env) $ python -m pip install -r requirements.txt
  Collecting novas==3.1.1.3 (from -r requirements.txt (line 1))
    ...
  Collecting numpy==1.9.2 (from -r requirements.txt (line 2))
    ...
  Collecting requests==2.7.0 (from -r requirements.txt (line 3))
    ...
  Installing collected packages: novas, numpy, requests
    Running setup.py install for novas
  Successfully installed novas-3.1.1.3 numpy-1.9.2 requests-2.7.0

``pip`` has many more options.  Consult the :ref:`installing-index`
guide for complete documentation for ``pip``.  When you've written
a package and want to make it available on the Python Package Index,
consult the `Python packaging user guide`_.

.. _Python Packaging User Guide: https://packaging.python.org/en/latest/tutorials/packaging-projects/
