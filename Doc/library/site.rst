:mod:`site` --- Site-specific configuration hook
================================================

.. module:: site
   :synopsis: Module responsible for site-specific configuration.

**Source code:** :source:`Lib/site.py`

--------------

.. highlight:: none

**This module is automatically imported during initialization.** The automatic
import can be suppressed using the interpreter's :option:`-S` option.

.. index:: triple: module; search; path

Importing this module will append site-specific paths to the module search path
and add a few builtins, unless :option:`-S` was used.  In that case, this module
can be safely imported with no automatic modifications to the module search path
or additions to the builtins.  To explicitly trigger the usual site-specific
additions, call the :func:`main` function.

.. versionchanged:: 3.3
   Importing the module used to trigger paths manipulation even when using
   :option:`-S`.

.. index::
   pair: site-packages; directory

It starts by constructing up to four directories from a head and a tail part.
For the head part, it uses ``sys.prefix`` and ``sys.exec_prefix``; empty heads
are skipped.  For the tail part, it uses the empty string and then
:file:`lib/site-packages` (on Windows) or
:file:`lib/python{X.Y}/site-packages` (on Unix and macOS).  For each
of the distinct head-tail combinations, it sees if it refers to an existing
directory, and if so, adds it to ``sys.path`` and also inspects the newly
added path for configuration files.

.. versionchanged:: 3.5
   Support for the "site-python" directory has been removed.

If a file named "pyvenv.cfg" exists one directory above sys.executable,
sys.prefix and sys.exec_prefix are set to that directory and
it is also checked for site-packages (sys.base_prefix and
sys.base_exec_prefix will always be the "real" prefixes of the Python
installation). If "pyvenv.cfg" (a bootstrap configuration file) contains
the key "include-system-site-packages" set to anything other than "true"
(case-insensitive), the system-level prefixes will not be
searched for site-packages; otherwise they will.

.. index::
   single: # (hash); comment
   pair: statement; import

A path configuration file is a file whose name has the form :file:`{name}.pth`
and exists in one of the four directories mentioned above; its contents are
additional items (one per line) to be added to ``sys.path``.  Non-existing items
are never added to ``sys.path``, and no check is made that the item refers to a
directory rather than a file.  No item is added to ``sys.path`` more than
once.  Blank lines and lines beginning with ``#`` are skipped.  Lines starting
with ``import`` (followed by space or tab) are executed.

.. note::

   An executable line in a :file:`.pth` file is run at every Python startup,
   regardless of whether a particular module is actually going to be used.
   Its impact should thus be kept to a minimum.
   The primary intended purpose of executable lines is to make the
   corresponding module(s) importable
   (load 3rd-party import hooks, adjust :envvar:`PATH` etc).
   Any other initialization is supposed to be done upon a module's
   actual import, if and when it happens.
   Limiting a code chunk to a single line is a deliberate measure
   to discourage putting anything more complex here.

.. index::
   single: package
   triple: path; configuration; file

For example, suppose ``sys.prefix`` and ``sys.exec_prefix`` are set to
:file:`/usr/local`.  The Python X.Y library is then installed in
:file:`/usr/local/lib/python{X.Y}`.  Suppose this has
a subdirectory :file:`/usr/local/lib/python{X.Y}/site-packages` with three
subsubdirectories, :file:`foo`, :file:`bar` and :file:`spam`, and two path
configuration files, :file:`foo.pth` and :file:`bar.pth`.  Assume
:file:`foo.pth` contains the following::

   # foo package configuration

   foo
   bar
   bletch

and :file:`bar.pth` contains::

   # bar package configuration

   bar

Then the following version-specific directories are added to
``sys.path``, in this order::

   /usr/local/lib/pythonX.Y/site-packages/bar
   /usr/local/lib/pythonX.Y/site-packages/foo

Note that :file:`bletch` is omitted because it doesn't exist; the :file:`bar`
directory precedes the :file:`foo` directory because :file:`bar.pth` comes
alphabetically before :file:`foo.pth`; and :file:`spam` is omitted because it is
not mentioned in either path configuration file.

:mod:`sitecustomize`
--------------------

.. module:: sitecustomize

After these path manipulations, an attempt is made to import a module named
:mod:`sitecustomize`, which can perform arbitrary site-specific customizations.
It is typically created by a system administrator in the site-packages
directory.  If this import fails with an :exc:`ImportError` or its subclass
exception, and the exception's :attr:`~ImportError.name`
attribute equals to ``'sitecustomize'``,
it is silently ignored.  If Python is started without output streams available, as
with :file:`pythonw.exe` on Windows (which is used by default to start IDLE),
attempted output from :mod:`sitecustomize` is ignored.  Any other exception
causes a silent and perhaps mysterious failure of the process.

:mod:`usercustomize`
--------------------

.. module:: usercustomize

After this, an attempt is made to import a module named :mod:`usercustomize`,
which can perform arbitrary user-specific customizations, if
:data:`~site.ENABLE_USER_SITE` is true.  This file is intended to be created in the
user site-packages directory (see below), which is part of ``sys.path`` unless
disabled by :option:`-s`.  If this import fails with an :exc:`ImportError` or
its subclass exception, and the exception's :attr:`~ImportError.name`
attribute equals to ``'usercustomize'``, it is silently ignored.

Note that for some non-Unix systems, ``sys.prefix`` and ``sys.exec_prefix`` are
empty, and the path manipulations are skipped; however the import of
:mod:`sitecustomize` and :mod:`usercustomize` is still attempted.

.. currentmodule:: site

.. _rlcompleter-config:

Readline configuration
----------------------

On systems that support :mod:`readline`, this module will also import and
configure the :mod:`rlcompleter` module, if Python is started in
:ref:`interactive mode <tut-interactive>` and without the :option:`-S` option.
The default behavior is enable tab-completion and to use
:file:`~/.python_history` as the history save file.  To disable it, delete (or
override) the :data:`sys.__interactivehook__` attribute in your
:mod:`sitecustomize` or :mod:`usercustomize` module or your
:envvar:`PYTHONSTARTUP` file.

.. versionchanged:: 3.4
   Activation of rlcompleter and history was made automatic.


Module contents
---------------

.. data:: PREFIXES

   A list of prefixes for site-packages directories.


.. data:: ENABLE_USER_SITE

   Flag showing the status of the user site-packages directory.  ``True`` means
   that it is enabled and was added to ``sys.path``.  ``False`` means that it
   was disabled by user request (with :option:`-s` or
   :envvar:`PYTHONNOUSERSITE`).  ``None`` means it was disabled for security
   reasons (mismatch between user or group id and effective id) or by an
   administrator.


.. data:: USER_SITE

   Path to the user site-packages for the running Python.  Can be ``None`` if
   :func:`getusersitepackages` hasn't been called yet.  Default value is
   :file:`~/.local/lib/python{X.Y}/site-packages` for UNIX and non-framework
   macOS builds, :file:`~/Library/Python/{X.Y}/lib/python/site-packages` for macOS
   framework builds, and :file:`{%APPDATA%}\\Python\\Python{XY}\\site-packages`
   on Windows.  This directory is a site directory, which means that
   :file:`.pth` files in it will be processed.


.. data:: USER_BASE

   Path to the base directory for the user site-packages.  Can be ``None`` if
   :func:`getuserbase` hasn't been called yet.  Default value is
   :file:`~/.local` for UNIX and macOS non-framework builds,
   :file:`~/Library/Python/{X.Y}` for macOS framework builds, and
   :file:`{%APPDATA%}\\Python` for Windows.  This value is used by Distutils to
   compute the installation directories for scripts, data files, Python modules,
   etc. for the :ref:`user installation scheme <sysconfig-user-scheme>`.
   See also :envvar:`PYTHONUSERBASE`.


.. function:: main()

   Adds all the standard site-specific directories to the module search
   path.  This function is called automatically when this module is imported,
   unless the Python interpreter was started with the :option:`-S` flag.

   .. versionchanged:: 3.3
      This function used to be called unconditionally.


.. function:: addsitedir(sitedir, known_paths=None)

   Add a directory to sys.path and process its :file:`.pth` files.  Typically
   used in :mod:`sitecustomize` or :mod:`usercustomize` (see above).


.. function:: getsitepackages()

   Return a list containing all global site-packages directories.

   .. versionadded:: 3.2


.. function:: getuserbase()

   Return the path of the user base directory, :data:`USER_BASE`.  If it is not
   initialized yet, this function will also set it, respecting
   :envvar:`PYTHONUSERBASE`.

   .. versionadded:: 3.2


.. function:: getusersitepackages()

   Return the path of the user-specific site-packages directory,
   :data:`USER_SITE`.  If it is not initialized yet, this function will also set
   it, respecting :data:`USER_BASE`.  To determine if the user-specific
   site-packages was added to ``sys.path`` :data:`ENABLE_USER_SITE` should be
   used.

   .. versionadded:: 3.2


.. _site-commandline:

Command Line Interface
----------------------

.. program:: site

The :mod:`site` module also provides a way to get the user directories from the
command line:

.. code-block:: shell-session

   $ python3 -m site --user-site
   /home/user/.local/lib/python3.3/site-packages

If it is called without arguments, it will print the contents of
:data:`sys.path` on the standard output, followed by the value of
:data:`USER_BASE` and whether the directory exists, then the same thing for
:data:`USER_SITE`, and finally the value of :data:`ENABLE_USER_SITE`.

.. option:: --user-base

   Print the path to the user base directory.

.. option:: --user-site

   Print the path to the user site-packages directory.

If both options are given, user base and user site will be printed (always in
this order), separated by :data:`os.pathsep`.

If any option is given, the script will exit with one of these values: ``0`` if
the user site-packages directory is enabled, ``1`` if it was disabled by the
user, ``2`` if it is disabled for security reasons or by an administrator, and a
value greater than 2 if there is an error.

.. seealso::

   * :pep:`370` -- Per user site-packages directory
   * :ref:`sys-path-init` -- The initialization of :data:`sys.path`.

