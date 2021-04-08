****************
Configure Python
****************

.. _configure-options:

Configure Options
=================

List all ``./configure`` script options using::

    ./configure --help

See also the :file:`Misc/SpecialBuilds.txt` in the Python source distribution.

General Options
---------------

.. cmdoption:: --enable-loadable-sqlite-extensions

   Support loadable extensions in the :mod:`_sqlite` extension module (default
   is no), see the :mod:`sqlite3` module.

   .. versionadded:: 3.6

.. cmdoption:: --disable-ipv6

   Disable IPv6 support (enabled by default if supported), see the
   :mod:`socket` module.

.. cmdoption:: --enable-big-digits[=15|30]

   Use big digits (15 or 30 bits) for Python :class:`int` numbers (default is
   system-dependent).

   See :data:`sys.int_info.bits_per_digit <sys.int_info>`.

.. cmdoption:: --with-cxx-main[=COMPILER]

   Compile the Python ``main()`` function and link Python executable with C++
   compiler specified in *COMPILER* (default is ``$CXX``).

.. cmdoption:: --with-suffix=SUFFIX

   Set executable suffix to *SUFFIX* (default is ``.exe``).

.. cmdoption:: --with-tzpath=<list of absolute paths separated by pathsep>

   Select the default time zone search path for :data:`zoneinfo.TZPATH`,
   see the :mod:`zoneinfo` module.

   .. versionadded:: 3.9

.. cmdoption:: --without-decimal-contextvar

   Build the ``_decimal`` extension module using a thread-local context rather
   than a coroutine-local context (default), see the :mod:`decimal` module.

   See :data:`decimal.HAVE_CONTEXTVAR`.

   .. versionadded:: 3.9

.. cmdoption:: --with-dbmliborder=db1:db2:...

   Override order to check db backends for the :mod:`dbm` module

   A valid value is a colon separated string with the backend names:

   * ``ndbm``;
   * ``gdbm``;
   * ``bdb``.

.. cmdoption:: --with-c-locale-coercion

   Enable C locale coercion to a UTF-8 based locale (default is yes).

   See :envvar:`PYTHONCOERCECLOCALE` and the :pep:`538`.

.. cmdoption:: --with-platlibdir=DIRNAME

   Python library directory name (default is ``lib``).

   Fedora and SuSE use ``lib64`` on 64-bit platforms.

   See :data:`sys.platlibdir`.

   .. versionadded:: 3.9

.. cmdoption:: --with-wheel-pkg-dir=PATH

   Directory of wheel packages used by the :mod:`ensurepip` module
   (none by default).

   Some Linux distribution packaging policies recommend against bundling
   dependencies. For example, Fedora installs wheel packages in the
   ``/usr/share/python-wheels/`` directory and don't install the
   :mod:`ensurepip._bundled` package.

   .. versionadded:: 3.10


Install Options
---------------

.. cmdoption:: --disable-test-modules

   Don't build nor install test modules, like the :mod:`test` package or the
   :mod:`_testcapi` extension module (built and installed by default).

   .. versionadded:: 3.10

.. cmdoption:: --with-ensurepip[=install|upgrade|no]

   ``install`` or ``upgrade`` using bundled pip of the :mod:`ensurepip` module,
   when installing Python (default is ``upgrade``).

   .. versionadded:: 3.6


Performance options
-------------------

Configuring Python using ``--enable-optimizations --with-lto`` (PGO + LTO) is
recommended for best performance.

.. cmdoption:: --enable-optimizations

   Enable Profile Guided Optimization (PGO) using :envvar:`PROFILE_TASK`
   (disabled by default).

   Disable also semantic interposition in libpython if ``--enable-shared`` and
   GCC is used: add ``-fno-semantic-interposition`` to the compiler and linker
   flags.

   .. versionadded:: 3.6

   .. versionchanged:: 3.10
      Use ``-fno-semantic-interposition`` on GCC.

.. envvar:: PROFILE_TASK

   Environment variable used in the Makefile: Python command line arguments for
   the PGO generation task.

   Default: ``-m test --pgo --timeout=$(TESTTIMEOUT)``.

   .. versionadded:: 3.8

.. cmdoption:: --with-lto

   Enable Link Time Optimization (LTO) in any build (disabled by default).

   .. versionadded:: 3.6

.. cmdoption:: --with-computed-gotos

   Enable computed gotos in evaluation loop (enabled by default on supported
   compilers).

.. cmdoption:: --without-pymalloc

   Disable the specialized Python memory allocator :ref:`pymalloc <pymalloc>`
   (enabled by default).

   See also :envvar:`PYTHONMALLOC` environment variable.

.. cmdoption:: --without-doc-strings

   Disable static documentation strings to reduce the memory footprint (enabled
   by default). Documentation strings defined in Python are not affected.

   If used, the ``WITH_DOC_STRINGS`` macro is not defined. See the
   ``PyDoc_STRVAR()`` macro.

.. cmdoption:: --enable-profiling

   Enable C-level code profiling with ``gprof`` (disabled by default).


.. _debug-build:

Debug build
-----------

A debug build is Python built with the :option:`--with-pydebug` configure
option.

Effects of a debug build:

* Define ``Py_DEBUG`` and ``Py_REF_DEBUG`` macros.
* Add ``d`` to :data:`sys.abiflags`.
* Add :func:`sys.gettotalrefcount` function.
* Add :option:`-X showrefcount <-X>` command line option.
* Add :envvar:`PYTHONTHREADDEBUG` environment variable.
* The list of default warning filters is empty in the :mod:`warnings` module.
* Install debug hooks on memory allocators to detect buffer overflow and other
  memory errors: see :c:func:`PyMem_SetupDebugHooks`.
* Build Python with assertions (don't set ``NDEBUG`` macro):
  ``assert(...);`` and ``_PyObject_ASSERT(...);`` are removed.
  See also the :option:`--with-assertions` configure option.
* Add runtime checks, code surroundeded by ``#ifdef Py_DEBUG`` and ``#endif``.
* Unicode and int objects are created with their memory filled with a pattern
  to help detecting uninitialized bytes.
* Many functions ensure that are not called with an exception raised, since
  they can clear or replace the current exception.
* The garbage collector (:func:`gc.collect` function) runs some quick checks on
  consistency.
* Add support for the ``__ltrace__`` variable: enable low-level tracing in the
  bytecode evaluation loop if the variable is defined.

See also the :ref:`Python Development Mode <devmode>` and the
:option:`--with-trace-refs` configure option.

.. versionchanged:: 3.8
   Release builds and debug builds are now ABI compatible: defining the
   ``Py_DEBUG`` macro no longer implies the ``Py_TRACE_REFS`` macro, which
   introduces the only ABI incompatibility.


Debug options
-------------

.. cmdoption:: --with-pydebug

   :ref:`Build Python in debug mode <debug-build>` (disabled by default).

.. cmdoption:: --with-trace-refs

   Enable tracing references for debugging purpose (disabled by default).

   Effects:

   * Define the ``Py_TRACE_REFS`` macro.
   * Add :func:`sys.getobjects` function.
   * Add :envvar:`PYTHONDUMPREFS` environment variable.

   This build is not ABI compatible with release build (default build) or debug
   build (``Py_DEBUG`` macro).

   .. versionadded:: 3.8

.. cmdoption:: --with-assertions

   Build with C assertions enabled (default is no).

   If set, the ``NDEBUG`` macro is not defined in the :envvar:`OPT` compiler
   variable.

   See also the :option:`--with-pydebug` option (:ref:`debug build
   <debug-build>`) which also enables assertions.

   .. versionadded:: 3.6

.. cmdoption:: --with-valgrind

   Enable Valgrind support (default is no).

.. cmdoption:: --with-dtrace

   Enable DTrace support (default is no).

   .. versionadded:: 3.6

.. cmdoption:: --with-address-sanitizer

   Enable AddressSanitizer memory error detector, 'asan' (default is no).

   .. versionadded:: 3.6

.. cmdoption:: --with-memory-sanitizer

   Enable MemorySanitizer allocation error detector, 'msan' (default is no).

   .. versionadded:: 3.6

.. cmdoption:: --with-undefined-behavior-sanitizer

   Enable UndefinedBehaviorSanitizer undefined behaviour detector, 'ubsan'
   (default is no).

   .. versionadded:: 3.6


Linker options
--------------

.. cmdoption:: --enable-shared

   Enable building a shared Python library: "libpython" (default is no).

.. cmdoption:: --without-static-libpython

   Do not build ``libpythonMAJOR.MINOR.a`` and do not install ``python.o``
   (built and enabled by default).

   .. versionadded:: 3.10


Libraries options
-----------------

.. cmdoption:: --with-libs='lib1 ...'

   Link against additional libraries (default is no).

.. cmdoption:: --with-system-expat

   Build the :mod:`pyexpat` module using an installed ``expat`` library
   (default is no).

.. cmdoption:: --with-system-ffi

   Build the :mod:`_ctypes` extension module using an installed ``ffi``
   library, see the :mod:`ctypes` module (default is system-dependent).

.. cmdoption:: --with-system-libmpdec

   Build the ``_decimal`` extension module using an installed ``libmpdec``
   library, see the :mod:`decimal` module (default is no).

   .. versionadded:: 3.3

.. cmdoption:: --with(out)-readline[=editline]

   Use ``editline`` for backend or disable the :mod:`readline` module.

   .. versionadded:: 3.10

.. cmdoption:: --with-tcltk-includes='-I...'

   Override search for Tcl and Tk include files.

.. cmdoption:: --with-tcltk-libs='-L...'

   Override search for Tcl and Tk libraries.

.. cmdoption:: --with-libm=STRING

   Override ``libm`` math library to *STRING* (default is system-dependent).

.. cmdoption:: --with-libc=STRING

   Override ``libc`` C library to *STRING* (default is system-dependent).

.. cmdoption:: --with-openssl=DIR

   Root of the OpenSSL directory.

.. cmdoption:: --with-openssl-rpath=[DIR|auto|no]

   Set runtime library directory (rpath) for OpenSSL libraries:

   * ``no`` (default): don't set rpath;
   * ``auto``: auto-detect rpath from :option:`--with-openssl` and
     ``pkg-config``;
   * *DIR*: set an explicit rpath.

   .. versionadded:: 3.10


Security Options
----------------

.. cmdoption:: --with-hash-algorithm=[fnv|siphash24]

   Select hash algorithm for use in ``Python/pyhash.c``:

   * ``fnv``;
   * ``siphash24`` (default).

   .. versionadded:: 3.4

.. cmdoption:: --with-builtin-hashlib-hashes=md5,sha1,sha256,sha512,sha3,blake2

   Built-in hash modules:

   * ``md5``;
   * ``sha1``;
   * ``sha256``;
   * ``sha512``;
   * ``sha3`` (with shake);
   * ``blake2``.

   .. versionadded:: 3.9

.. cmdoption:: --with-ssl-default-suites=[python|openssl|STRING]

   Override the OpenSSL default cipher suites string:

   * ``python``: use Python's preferred selection (default);
   * ``openssl``: leave OpenSSL's defaults untouched;
   * *STRING*: use a custom string, PROTOCOL_SSLv2 ignores the setting.

   See the :mod:`ssl` module.

   .. versionadded:: 3.7


macOS Options
-------------

See ``Mac/README.rst``.

.. cmdoption:: --enable-universalsdk[=SDKDIR]

   Create a universal binary build. *SDKDIR* specifies which macOS SDK should
   be used to perform the build (default is no).

.. cmdoption:: --enable-framework[=INSTALLDIR]

   Create a Python.framework rather than a traditional Unix install. Optional
   *INSTALLDIR* specifies the installation path (default is no).

.. cmdoption:: --with-universal-archs=ARCH

   Specify the kind of universal binary that should be created. this option is
   only valid when :option:`--enable-universalsdk` is set.

   Options are:

   * ``universal2``;
   * ``32-bit``;
   * ``64-bit``;
   * ``3-way``;
   * ``intel``;
   * ``intel-32``;
   * ``intel-64``;
   * ``all``.

.. cmdoption:: --with-framework-name=FRAMEWORK

   Specify the name for the python framework on macOS only valid when
   :option:`--enable-framework` is set (default: ``Python``).


Compiler and linker flags
=========================

Options set by the ``./configure`` script, ``Makefile`` and by environment
variables.

Preprocessor flags
------------------

.. envvar:: CPP

   C preprocessor.

.. envvar:: CONFIGURE_CPPFLAGS

   .. versionadded:: 3.6

.. envvar:: CPPFLAGS

   (Objective) C/C++ preprocessor flags, e.g. ``-I<include dir>`` if you have
   headers in a nonstandard directory ``<include dir>``.

   Both :envvar:`CPPFLAGS` and :envvar:`LDFLAGS` need to contain the shell's
   value for setup.py to be able to build extension modules using the
   directories specified in the environment variables.

.. envvar:: BASECPPFLAGS

   .. versionadded:: 3.4

.. envvar:: MULTIARCH_CPPFLAGS

   .. versionadded:: 3.6

.. envvar:: PY_CPPFLAGS

   Extra preprocessor flags added for building the interpreter object files.

   Default: ``$(BASECPPFLAGS) -I. -I$(srcdir)/Include $(CONFIGURE_CPPFLAGS) $(CPPFLAGS)``.

   .. versionadded:: 3.2

Compiler flags
--------------

.. envvar:: CC

   C compiler command.

.. envvar:: CFLAGS

   C compiler flags.

.. envvar:: CFLAGS_NODIST

   :envvar:`CFLAGS_NODIST` is used for building the interpreter and stdlib C
   extensions.  Use it when a compiler flag should *not* be part of the
   distutils :envvar:`CFLAGS` once Python is installed (:issue:`21121`).

   .. versionadded:: 3.5

.. envvar:: EXTRA_CFLAGS

   Extra C compiler flags.

.. envvar:: CONFIGURE_CFLAGS

   .. versionadded:: 3.2

.. envvar:: CONFIGURE_CFLAGS_NODIST

   .. versionadded:: 3.5

.. envvar:: BASECFLAGS

.. envvar:: OPT

   Optimization flags.

.. envvar:: CFLAGS_ALIASING

   Strict or non-strict aliasing flags used to compile ``Python/dtoa.c``.

   .. versionadded:: 3.7

.. envvar:: CFLAGSFORSHARED

   Extra C flags added for building the interpreter object files.

.. envvar:: PY_CFLAGS

   Default: ``$(BASECFLAGS) $(OPT) $(CONFIGURE_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS)``.

.. envvar:: PY_CFLAGS_NODIST

   Default: ``$(CONFIGURE_CFLAGS_NODIST) $(CFLAGS_NODIST) -I$(srcdir)/Include/internal``.

   .. versionadded:: 3.5

.. envvar:: PY_STDMODULE_CFLAGS

   C flags used for building the interpreter object files.

   Default: ``$(PY_CFLAGS) $(PY_CFLAGS_NODIST) $(PY_CPPFLAGS) $(CFLAGSFORSHARED)``.

   .. versionadded:: 3.7

.. envvar:: PY_CORE_CFLAGS

   Default: ``$(PY_STDMODULE_CFLAGS) -DPy_BUILD_CORE``.

   .. versionadded:: 3.2

.. envvar:: PY_BUILTIN_MODULE_CFLAGS

   Default: ``$(PY_STDMODULE_CFLAGS) -DPy_BUILD_CORE_BUILTIN``.

   .. versionadded:: 3.8


Linker flags
------------

.. envvar:: CONFIGURE_LDFLAGS

   Avoid assigning :envvar:`CFLAGS`, :envvar:`LDFLAGS`, etc. so users can use
   them on the command line to append to these values without stomping the
   pre-set values.

   .. versionadded:: 3.2

.. envvar:: LDFLAGS_NODIST

   :envvar:`LDFLAGS_NODIST` is used in the same manner as
   :envvar:`CFLAGS_NODIST`.  Use it when a linker flag should *not* be part of
   the distutils :envvar:`LDFLAGS` once Python is installed (:issue:`35257`).

.. envvar:: CONFIGURE_LDFLAGS_NODIST

   .. versionadded:: 3.8

.. envvar:: LDFLAGS

   Linker flags, e.g. ``-L<lib dir>`` if you have libraries in a nonstandard
   directory ``<lib dir>``.

   Both :envvar:`CPPFLAGS` and :envvar:`LDFLAGS` need to contain the shell's
   value for setup.py to be able to build extension modules using the
   directories specified in the environment variables.

.. envvar:: LIBS

   Libraries to pass to the linker, e.g. ``-l<library>``.

.. envvar:: LDSHARED

   Command to build a shared library.

   Default: ``@LDSHARED@ $(PY_LDFLAGS)``.

.. envvar:: BLDSHARED

   Command to build libpython shared library.

   Default: ``@BLDSHARED@ $(PY_CORE_LDFLAGS)``.

.. envvar:: PY_LDFLAGS

   Default: ``$(CONFIGURE_LDFLAGS) $(LDFLAGS)``.

.. envvar:: PY_LDFLAGS_NODIST

   Default: ``$(CONFIGURE_LDFLAGS_NODIST) $(LDFLAGS_NODIST)``.

   .. versionadded:: 3.8

.. envvar:: PY_CORE_LDFLAGS

   Linker flags used for building the interpreter object files.

   .. versionadded:: 3.8
