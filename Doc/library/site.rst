:mod:`!site` --- Site-specific configuration hook
=================================================

.. module:: site
   :synopsis: Module responsible for site-specific configuration.

**Source code:** :source:`Lib/site.py`

--------------

.. highlight:: none

**This module is automatically imported during initialization.** The automatic
import can be suppressed using the interpreter's :option:`-S` option.

.. index:: triple: module; search; path

Importing this module normally appends site-specific paths to the module search path
and adds :ref:`callables <site-consts>`, including :func:`help` to the built-in
namespace. However, Python startup option :option:`-S` blocks this, and this module
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
:file:`lib/python{X.Y[t]}/site-packages` (on Unix and macOS). (The
optional suffix "t" indicates the :term:`free-threaded build`, and is
appended if ``"t"`` is present in the :data:`sys.abiflags` constant.)
For each
of the distinct head-tail combinations, it sees if it refers to an existing
directory, and if so, adds it to ``sys.path`` and also inspects the newly
added path for configuration files.

.. versionchanged:: 3.5
   Support for the "site-python" directory has been removed.

.. versionchanged:: 3.13
   On Unix, :term:`Free threading <free threading>` Python installations are
   identified by the "t" suffix in the version-specific directory name, such as
   :file:`lib/python3.13t/`.

.. versionchanged:: 3.14

   :mod:`!site` is no longer responsible for updating :data:`sys.prefix` and
   :data:`sys.exec_prefix` on :ref:`sys-path-init-virtual-environments`. This is
   now done during the :ref:`path initialization <sys-path-init>`. As a result,
   under :ref:`sys-path-init-virtual-environments`, :data:`sys.prefix` and
   :data:`sys.exec_prefix` no longer depend on the :mod:`!site` initialization,
   and are therefore unaffected by :option:`-S`.

.. _site-virtual-environments-configuration:

When running under a :ref:`virtual environment <sys-path-init-virtual-environments>`,
the ``pyvenv.cfg`` file in :data:`sys.prefix` is checked for site-specific
configurations. If the ``include-system-site-packages`` key exists and is set to
``true`` (case-insensitive), the system-level prefixes will be searched for
site-packages, otherwise they won't.  If the system-level prefixes are not searched then
the user site prefixes are also implicitly not searched for site-packages.

.. index::
   single: # (hash); comment
   pair: statement; import

The :mod:`!site` module recognizes two startup configuration files of the form
:file:`{name}.pth` for path configurations, and :file:`{name}.start` for
pre-first-line code execution.  Both files can exist in one of the four
directories mentioned above.  Within each directory, these files are sorted
alphabetically by filename, then parsed in sorted order.

.. _site-pth-files:

Path extensions (:file:`.pth` files)
------------------------------------

:file:`{name}.pth` contains additional items (one per line) to be appended to
``sys.path``.  Items that name non-existing directories are never added to
``sys.path``, and no check is made that the item refers to a directory rather
than a file.  No item is added to ``sys.path`` more than once.  Blank lines
and lines beginning with ``#`` are skipped.

For backward compatibility, lines starting with ``import`` (followed by space
or tab) are executed with :func:`exec`.

.. versionchanged:: 3.15

   ``import`` lines in :file:`{name}.pth` are ignored when a :ref:`matching
   <site-start-files>` :file:`{name}.start` file exists.

.. deprecated-removed:: 3.15 3.20

   ``import`` lines in :file:`{name}.pth` files are deprecated and will be
   silently ignored in Python 3.18 and 3.19.  In Python 3.20 a warning will be
   produced for ``import`` lines in :file:`{name}.pth` files.


.. _site-start-files:

Startup entry points (:file:`.start` files)
-------------------------------------------

.. versionadded:: 3.15

A startup entry point file is a file whose name has the form
:file:`{name}.start` and exists in one of the site-packages directories
described above.  Each file specifies entry points to be called during
interpreter startup, using the ``pkg.mod:callable`` syntax understood by
:func:`pkgutil.resolve_name`.

Each non-blank line that does not begin with ``#`` must contain an entry
point reference in the form ``pkg.mod:callable``.  The colon and callable
portion are mandatory.  Each callable is invoked with no arguments, and
any return value is discarded.

:file:`.start` files are processed after all :file:`.pth` path extensions
have been applied to :data:`sys.path`, ensuring that paths are available
before any startup code runs.

Unlike :data:`sys.path` extensions from :file:`.pth` files, duplicate entry
points are **not** de-duplicated --- if an entry point appears more than once,
it will be called more than once.

If an exception occurs during resolution or invocation of an entry point,
a traceback is printed to :data:`sys.stderr` and processing continues with
the remaining entry points.

:file:`.start` files must be encoded in UTF-8.

:pep:`829` defined the original specification for these features.

.. note::

   If a :file:`{name}.start` file exists alongside a :file:`{name}.pth` file
   with the same base name, any ``import`` lines in the :file:`.pth` file are
   ignored in favor of the entry points in the :file:`.start` file.

.. note::

   Executable lines (``import`` lines in :file:`{name}.pth` files and
   :file:`{name}.start` file entry points) are always run at Python startup
   (unless :option:`-S` is given to disable the ``site.py`` module entirely),
   regardless of whether a particular module is actually going to be used.

.. note::

   :file:`{name}.start` files invoke :func:`pkgutil.resolve_name` with
   ``strict=True``, which requires the full ``pkg.mod:callable`` form.

.. versionchanged:: 3.13

   The :file:`.pth` files are now decoded by UTF-8 at first and then by the
   :term:`locale encoding` if it fails.

.. deprecated-removed:: 3.15 3.20

   Decoding :file:`{name}.pth` files in any encoding other than ``utf-8-sig``
   is deprecated in Python 3.15, and support for decoding from the locale
   encoding will be removed in Python 3.20.

.. versionchanged:: 3.15

   :file:`.pth` file lines starting with ``import`` are deprecated.  During
   the deprecation period, such lines are still executed, but a diagnostic
   message is emitted when the :option:`-v` flag is given.  If a
   :file:`{name}.start` file with the same base name exists, ``import`` lines
   in :file:`{name}.pth` files are silently ignored.  See
   :ref:`site-start-files` and :pep:`829`.

   Errors on individual lines no longer abort processing of the rest of the
   file.  Each error is reported and the remaining lines continue to be
   processed.

.. index::
   single: package
   triple: path; configuration; file


Startup file examples
---------------------

For example, suppose ``sys.prefix`` and ``sys.exec_prefix`` are set to
:file:`/usr/local`.  The Python X.Y library is then installed in
:file:`/usr/local/lib/python{X.Y}`.  Suppose this has
a subdirectory :file:`/usr/local/lib/python{X.Y}/site-packages` with three
sub-subdirectories, :file:`foo`, :file:`bar` and :file:`spam`, and two path
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

Let's say that there is also a :file:`foo.start` file containing the
following::

    # foo package startup code

    foo.submod:initialize()

Now, after ``sys.path`` has been extended as above, and before Python turns
control over to user code, the ``foo.submod`` module is imported and the
``initialize()`` function from that module is called.


.. _site-migration-guide:

Migrating from ``import`` lines in ``.pth`` files to ``.start`` files
---------------------------------------------------------------------

If your package currently ships a :file:`{name}.pth` file, you can keep all
``sys.path`` extension lines unchanged.  Only ``import`` lines need to be
migrated.

To migrate, create a callable (taking zero arguments) within an importable
module in your package.  Reference it as a ``pkg.mod:callable`` entry point
in a matching :file:`{name}.start` file.  Move everything on your ``import``
line after the first semi-colon into the ``callable()`` function.

If your package must straddle older Pythons that do not support :pep:`829`
and newer Pythons that do, change the ``import`` lines in your
:file:`{name}.pth` to use the following form:

.. code-block:: python

   import pkg.mod; pkg.mod.callable()

Older Pythons will execute these ``import`` lines, while newer Pythons will
ignore them in favor of the :file:`{name}.start` file.  After the straddling
period, remove all ``import`` lines from your :file:`.pth` files.


:mod:`!sitecustomize`
---------------------

.. module:: sitecustomize

After these path manipulations, an attempt is made to import a module named
:mod:`!sitecustomize`, which can perform arbitrary site-specific customizations.
It is typically created by a system administrator in the site-packages
directory.  If this import fails with an :exc:`ImportError` or its subclass
exception, and the exception's :attr:`~ImportError.name`
attribute equals ``'sitecustomize'``,
it is silently ignored.  If Python is started without output streams available, as
with :file:`pythonw.exe` on Windows (which is used by default to start IDLE),
attempted output from :mod:`!sitecustomize` is ignored.  Any other exception
causes a silent and perhaps mysterious failure of the process.

:mod:`!usercustomize`
---------------------

.. module:: usercustomize

After this, an attempt is made to import a module named :mod:`!usercustomize`,
which can perform arbitrary user-specific customizations, if
:data:`~site.ENABLE_USER_SITE` is true.  This file is intended to be created in the
user site-packages directory (see below), which is part of ``sys.path`` unless
disabled by :option:`-s`.  If this import fails with an :exc:`ImportError` or
its subclass exception, and the exception's :attr:`~ImportError.name`
attribute equals ``'usercustomize'``, it is silently ignored.

Note that for some non-Unix systems, ``sys.prefix`` and ``sys.exec_prefix`` are
empty, and the path manipulations are skipped; however the import of
:mod:`sitecustomize` and :mod:`!usercustomize` is still attempted.

.. currentmodule:: site

.. _rlcompleter-config:

Readline configuration
----------------------

On systems that support :mod:`readline`, this module will also import and
configure the :mod:`rlcompleter` module, if Python is started in
:ref:`interactive mode <tut-interactive>` and without the :option:`-S` option.
The default behavior is to enable tab completion and to use
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
   :file:`~/.local/lib/python{X.Y}[t]/site-packages` for UNIX and non-framework
   macOS builds, :file:`~/Library/Python/{X.Y}/lib/python/site-packages` for macOS
   framework builds, and :file:`{%APPDATA%}\\Python\\Python{XY}\\site-packages`
   on Windows.  The optional "t" indicates the free-threaded build.  This
   directory is a site directory, which means that :file:`.pth` files in it
   will be processed.


.. data:: USER_BASE

   Path to the base directory for the user site-packages.  Can be ``None`` if
   :func:`getuserbase` hasn't been called yet.  Default value is
   :file:`~/.local` for UNIX and macOS non-framework builds,
   :file:`~/Library/Python/{X.Y}` for macOS framework builds, and
   :file:`{%APPDATA%}\\Python` for Windows.  This value is used to
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

   Add a directory to sys.path and process its :file:`.pth` and
   :file:`.start` files.  Typically used in :mod:`sitecustomize` or
   :mod:`usercustomize` (see above).

   The *known_paths* argument is an optional set of case-normalized paths
   used to prevent duplicate :data:`sys.path` entries.  When ``None`` (the
   default), the set is built from the current :data:`sys.path`.

   .. versionchanged:: 3.15
      Also processes :file:`.start` files.  See :ref:`site-start-files`.
      All :file:`.pth` and :file:`.start` files are now read and
      accumulated before any path extensions, ``import`` line execution,
      or entry point invocations take place.


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

Command-line interface
----------------------

.. program:: site

The :mod:`!site` module also provides a way to get the user directories from the
command line:

.. code-block:: shell-session

   $ python -m site --user-site
   /home/user/.local/lib/python3.11/site-packages

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
   * :pep:`829` -- Startup entry points and the deprecation of import lines in ``.pth`` files
   * :ref:`sys-path-init` -- The initialization of :data:`sys.path`.

