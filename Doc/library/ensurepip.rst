:mod:`!ensurepip` --- Bootstrapping the ``pip`` installer
=========================================================

.. module:: ensurepip
   :synopsis: Bootstrapping the "pip" installer into an existing Python
              installation or virtual environment.

.. versionadded:: 3.4

**Source code:** :source:`Lib/ensurepip`

--------------

The :mod:`ensurepip` package provides support for bootstrapping the ``pip``
installer into an existing Python installation or virtual environment. This
bootstrapping approach reflects the fact that ``pip`` is an independent
project with its own release cycle, and the latest available stable version
is bundled with maintenance and feature releases of the CPython reference
interpreter.

In most cases, end users of Python shouldn't need to invoke this module
directly (as ``pip`` should be bootstrapped by default), but it may be
needed if installing ``pip`` was skipped when installing Python (or
when creating a virtual environment) or after explicitly uninstalling
``pip``.

.. note::

   This module *does not* access the internet. All of the components
   needed to bootstrap ``pip`` are included as internal parts of the
   package.

.. seealso::

   :ref:`installing-index`
      The end user guide for installing Python packages

   :pep:`453`: Explicit bootstrapping of pip in Python installations
      The original rationale and specification for this module.

.. include:: ../includes/wasm-mobile-notavail.rst

Command line interface
----------------------

.. program:: ensurepip

The command line interface is invoked using the interpreter's ``-m`` switch.

The simplest possible invocation is::

    python -m ensurepip

This invocation will install ``pip`` if it is not already installed,
but otherwise does nothing. To ensure the installed version of ``pip``
is at least as recent as the one available in ``ensurepip``, pass the
``--upgrade`` option::

    python -m ensurepip --upgrade

By default, ``pip`` is installed into the current virtual environment
(if one is active) or into the system site packages (if there is no
active virtual environment). The installation location can be controlled
through two additional command line options:

.. option:: --root <dir>

   Installs ``pip`` relative to the given root directory rather than the root
   of the currently active virtual environment (if any) or the default root
   for the current Python installation.

.. option:: --user

   Installs ``pip`` into the user site packages directory rather than globally
   for the current Python installation (this option is not permitted inside an
   active virtual environment).

By default, the scripts ``pipX`` and ``pipX.Y`` will be installed (where
X.Y stands for the version of Python used to invoke ``ensurepip``). The
scripts installed can be controlled through two additional command line
options:

.. option:: --altinstall

   If an alternate installation is requested, the ``pipX`` script will *not* be
   installed.

.. option:: --default-pip

   If a "default pip" installation is requested, the ``pip`` script will be
   installed in addition to the two regular scripts.

Providing both of the script selection options will trigger an exception.

Module API
----------

:mod:`ensurepip` exposes two functions for programmatic use:

.. function:: version()

   Returns a string specifying the available version of pip that will be
   installed when bootstrapping an environment.

.. function:: bootstrap(root=None, upgrade=False, user=False, \
                        altinstall=False, default_pip=False, \
                        verbosity=0)

   Bootstraps ``pip`` into the current or designated environment.

   *root* specifies an alternative root directory to install relative to.
   If *root* is ``None``, then installation uses the default install location
   for the current environment.

   *upgrade* indicates whether or not to upgrade an existing installation
   of an earlier version of ``pip`` to the available version.

   *user* indicates whether to use the user scheme rather than installing
   globally.

   By default, the scripts ``pipX`` and ``pipX.Y`` will be installed (where
   X.Y stands for the current version of Python).

   If *altinstall* is set, then ``pipX`` will *not* be installed.

   If *default_pip* is set, then ``pip`` will be installed in addition to
   the two regular scripts.

   Setting both *altinstall* and *default_pip* will trigger
   :exc:`ValueError`.

   *verbosity* controls the level of output to :data:`sys.stdout` from the
   bootstrapping operation.

   .. audit-event:: ensurepip.bootstrap root ensurepip.bootstrap

   .. note::

      The bootstrapping process has side effects on both ``sys.path`` and
      ``os.environ``. Invoking the command line interface in a subprocess
      instead allows these side effects to be avoided.

   .. note::

      The bootstrapping process may install additional modules required by
      ``pip``, but other software should not assume those dependencies will
      always be present by default (as the dependencies may be removed in a
      future version of ``pip``).

