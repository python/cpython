:mod:`packaging.util` --- Miscellaneous utility functions
=========================================================

.. module:: packaging.util
   :synopsis: Miscellaneous utility functions.


This module contains various helpers for the other modules.

.. XXX a number of functions are missing, but the module may be split first
   (it's ginormous right now, some things could go to compat for example)

.. function:: get_platform()

   Return a string that identifies the current platform.  This is used mainly to
   distinguish platform-specific build directories and platform-specific built
   distributions.  Typically includes the OS name and version and the
   architecture (as supplied by 'os.uname()'), although the exact information
   included depends on the OS; e.g. for IRIX the architecture isn't particularly
   important (IRIX only runs on SGI hardware), but for Linux the kernel version
   isn't particularly important.

   Examples of returned values:

   * ``linux-i586``
   * ``linux-alpha``
   * ``solaris-2.6-sun4u``
   * ``irix-5.3``
   * ``irix64-6.2``

   For non-POSIX platforms, currently just returns ``sys.platform``.

   For Mac OS X systems the OS version reflects the minimal version on which
   binaries will run (that is, the value of ``MACOSX_DEPLOYMENT_TARGET``
   during the build of Python), not the OS version of the current system.

   For universal binary builds on Mac OS X the architecture value reflects
   the univeral binary status instead of the architecture of the current
   processor. For 32-bit universal binaries the architecture is ``fat``,
   for 64-bit universal binaries the architecture is ``fat64``, and
   for 4-way universal binaries the architecture is ``universal``. Starting
   from Python 2.7 and Python 3.2 the architecture ``fat3`` is used for
   a 3-way universal build (ppc, i386, x86_64) and ``intel`` is used for
   a univeral build with the i386 and x86_64 architectures

   Examples of returned values on Mac OS X:

   * ``macosx-10.3-ppc``

   * ``macosx-10.3-fat``

   * ``macosx-10.5-universal``

   * ``macosx-10.6-intel``

   .. XXX reinvention of platform module?


.. function:: convert_path(pathname)

   Return 'pathname' as a name that will work on the native filesystem, i.e.
   split it on '/' and put it back together again using the current directory
   separator. Needed because filenames in the setup script are always supplied
   in Unix style, and have to be converted to the local convention before we
   can actually use them in the filesystem.  Raises :exc:`ValueError` on
   non-Unix-ish systems if *pathname* either starts or ends with a slash.


.. function:: change_root(new_root, pathname)

   Return *pathname* with *new_root* prepended.  If *pathname* is relative, this
   is equivalent to ``os.path.join(new_root,pathname)`` Otherwise, it requires
   making *pathname* relative and then joining the two, which is tricky on
   DOS/Windows.


.. function:: check_environ()

   Ensure that 'os.environ' has all the environment variables we guarantee that
   users can use in config files, command-line options, etc.  Currently this
   includes:

   * :envvar:`HOME` - user's home directory (Unix only)
   * :envvar:`PLAT` - description of the current platform, including hardware
     and OS (see :func:`get_platform`)


.. function:: find_executable(executable, path=None)

   Search the path for a given executable name.


.. function:: execute(func, args[, msg=None, verbose=0, dry_run=0])

   Perform some action that affects the outside world (for instance, writing to
   the filesystem).  Such actions are special because they are disabled by the
   *dry_run* flag.  This method takes care of all that bureaucracy for you;
   all you have to do is supply the function to call and an argument tuple for
   it (to embody the "external action" being performed), and an optional message
   to print.


.. function:: newer(source, target)

   Return true if *source* exists and is more recently modified than *target*,
   or if *source* exists and *target* doesn't. Return false if both exist and
   *target* is the same age or newer than *source*. Raise
   :exc:`PackagingFileError` if *source* does not exist.


.. function:: strtobool(val)

   Convert a string representation of truth to true (1) or false (0).

   True values are ``y``, ``yes``, ``t``, ``true``, ``on`` and ``1``; false
   values are ``n``, ``no``, ``f``, ``false``, ``off`` and ``0``.  Raises
   :exc:`ValueError` if *val* is anything else.


.. function:: byte_compile(py_files[, optimize=0, force=0, prefix=None, base_dir=None, verbose=1, dry_run=0, direct=None])

   Byte-compile a collection of Python source files to either :file:`.pyc` or
   :file:`.pyo` files in a :file:`__pycache__` subdirectory (see :pep:`3147`),
   or to the same directory when using the distutils2 backport on Python
   versions older than 3.2.

   *py_files* is a list of files to compile; any files that don't end in
   :file:`.py` are silently skipped.  *optimize* must be one of the following:

   * ``0`` - don't optimize (generate :file:`.pyc`)
   * ``1`` - normal optimization (like ``python -O``)
   * ``2`` - extra optimization (like ``python -OO``)

   If *force* is true, all files are recompiled regardless of timestamps.

   The source filename encoded in each :term:`bytecode` file defaults to the filenames
   listed in *py_files*; you can modify these with *prefix* and *basedir*.
   *prefix* is a string that will be stripped off of each source filename, and
   *base_dir* is a directory name that will be prepended (after *prefix* is
   stripped).  You can supply either or both (or neither) of *prefix* and
   *base_dir*, as you wish.

   If *dry_run* is true, doesn't actually do anything that would affect the
   filesystem.

   Byte-compilation is either done directly in this interpreter process with the
   standard :mod:`py_compile` module, or indirectly by writing a temporary
   script and executing it.  Normally, you should let :func:`byte_compile`
   figure out to use direct compilation or not (see the source for details).
   The *direct* flag is used by the script generated in indirect mode; unless
   you know what you're doing, leave it set to ``None``.

   This function is independent from the running Python's :option:`-O` or
   :option:`-B` options; it is fully controlled by the parameters passed in.
