****************
Configure Python
****************

.. highlight:: sh


.. _build-requirements:

Build Requirements
==================

To build CPython, you will need:

* A `C11 <https://en.cppreference.com/w/c/11>`_ compiler. `Optional C11
  features
  <https://en.wikipedia.org/wiki/C11_(C_standard_revision)#Optional_features>`_
  are not required.

* On Windows, Microsoft Visual Studio 2017 or later is required.

* Support for `IEEE 754 <https://en.wikipedia.org/wiki/IEEE_754>`_
  floating-point numbers and `floating-point Not-a-Number (NaN)
  <https://en.wikipedia.org/wiki/NaN#Floating_point>`_.

* Support for threads.

.. versionchanged:: 3.5
   On Windows, Visual Studio 2015 or later is now required.

.. versionchanged:: 3.6
   Selected C99 features, like ``<stdint.h>`` and ``static inline`` functions,
   are now required.

.. versionchanged:: 3.7
   Thread support is now required.

.. versionchanged:: 3.11
   C11 compiler, IEEE 754 and NaN support are now required.
   On Windows, Visual Studio 2017 or later is required.

See also :pep:`7` "Style Guide for C Code" and :pep:`11` "CPython platform
support".


.. _optional-module-requirements:

Requirements for optional modules
---------------------------------

Some :term:`optional modules <optional module>` of the standard library
require third-party libraries installed for development
(for example, header files must be available).

Missing requirements are reported in the ``configure`` output.
Modules that are missing due to missing dependencies are listed near the end
of the ``make`` output,
sometimes using an internal name, for example, ``_ctypes`` for :mod:`ctypes`
module.

If you distribute a CPython interpreter without optional modules,
it's best practice to advise users, who generally expect that
standard library modules are available.

Dependencies to build optional modules are:

.. list-table::
   :header-rows: 1
   :align: left

   * - Dependency
     - Minimum version
     - Python module
   * - `libbz2 <https://sourceware.org/bzip2/>`_
     -
     - :mod:`bz2`
   * - `libffi <https://sourceware.org/libffi/>`_
     - 3.3.0 recommended
     - :mod:`ctypes`
   * - `liblzma <https://tukaani.org/xz/>`_
     -
     - :mod:`lzma`
   * - `libmpdec <https://www.bytereef.org/mpdecimal/doc/libmpdec/>`_
     - 2.5.0
     - :mod:`decimal` [1]_
   * - `libreadline <https://tiswww.case.edu/php/chet/readline/rltop.html>`_ or
       `libedit <https://www.thrysoee.dk/editline/>`_ [2]_
     -
     - :mod:`readline`
   * - `libuuid <https://linux.die.net/man/3/libuuid>`_
     -
     - ``_uuid`` [3]_
   * - `ncurses <https://gnu.org/software/ncurses/ncurses.html>`_ [4]_
     -
     - :mod:`curses`
   * - `OpenSSL <https://openssl-library.org/>`_
     - | 3.0.18 recommended
       | (1.1.1 minimum)
     - :mod:`ssl`, :mod:`hashlib` [5]_
   * - `SQLite <https://sqlite.org/>`_
     - 3.15.2
     - :mod:`sqlite3`
   * - `Tcl/Tk <https://www.tcl-lang.org/>`_
     - 8.5.12
     - :mod:`tkinter`, :ref:`IDLE <idle>`, :mod:`turtle`
   * - `zlib <https://www.zlib.net>`_
     - 1.2.2.1
     - :mod:`zlib`, :mod:`gzip`, :mod:`ensurepip`
   * - `zstd <https://facebook.github.io/zstd/>`_
     - 1.4.5
     - :mod:`compression.zstd`

.. [1] If *libmpdec* is not available, the :mod:`decimal` module will use
   a pure-Python implementation.
   See :option:`--with-system-libmpdec` for details.
.. [2] See :option:`--with-readline` for choosing the backend for the
   :mod:`readline` module.
.. [3] The :mod:`uuid` module uses ``_uuid`` to generate "safe" UUIDs.
   See the module documentation for details.
.. [4] The :mod:`curses` module requires the ``libncurses`` or ``libncursesw``
   library.
   The :mod:`curses.panel` module additionally requires the ``libpanel`` or
   ``libpanelw`` library.
.. [5] If OpenSSL is not available, the :mod:`hashlib` module will use
   bundled implementations of several hash functions.
   See :option:`--with-builtin-hashlib-hashes` for *forcing* usage of OpenSSL.

Note that the table does not include all optional modules; in particular,
platform-specific modules like :mod:`winreg` are not listed here.

.. seealso::

   * The `devguide <https://devguide.python.org/getting-started/setup-building/#install-dependencies>`_
     includes a full list of dependencies required to build all modules and
     instructions on how to install them on common platforms.
   * :option:`--with-system-expat` allows building with an external
     `libexpat <https://libexpat.github.io/>`_ library.
   * :ref:`configure-options-for-dependencies`

.. versionchanged:: 3.1
   Tcl/Tk version 8.3.1 is now required for :mod:`tkinter`.

.. versionchanged:: 3.5
   Tcl/Tk version 8.4 is now required for :mod:`tkinter`.

.. versionchanged:: 3.7
   OpenSSL 1.0.2 is now required for :mod:`hashlib` and :mod:`ssl`.

.. versionchanged:: 3.10
   OpenSSL 1.1.1 is now required for :mod:`hashlib` and :mod:`ssl`.
   SQLite 3.7.15 is now required for :mod:`sqlite3`.

.. versionchanged:: 3.11
   Tcl/Tk version 8.5.12 is now required for :mod:`tkinter`.

.. versionchanged:: 3.13
   SQLite 3.15.2 is now required for :mod:`sqlite3`.


Generated files
===============

To reduce build dependencies, Python source code contains multiple generated
files. Commands to regenerate all generated files::

    make regen-all
    make regen-stdlib-module-names
    make regen-limited-abi
    make regen-configure

The ``Makefile.pre.in`` file documents generated files, their inputs, and tools used
to regenerate them. Search for ``regen-*`` make targets.

configure script
----------------

The ``make regen-configure`` command regenerates the ``aclocal.m4`` file and
the ``configure`` script using the ``Tools/build/regen-configure.sh`` shell
script which uses an Ubuntu container to get the same tools versions and have a
reproducible output.

The container is optional, the following command can be run locally::

    autoreconf -ivf -Werror

The generated files can change depending on the exact versions of the
tools used.
The container that CPython uses has
`Autoconf <https://gnu.org/software/autoconf>`_ 2.72,
``aclocal`` from `Automake <https://www.gnu.org/software/automake>`_ 1.16.5,
and `pkg-config <https://www.freedesktop.org/wiki/Software/pkg-config/>`_ 1.8.1.

.. versionchanged:: 3.13
   Autoconf 2.71 and aclocal 1.16.5 and are now used to regenerate
   :file:`configure`.

.. versionchanged:: 3.14
   Autoconf 2.72 is now used to regenerate :file:`configure`.


.. _configure-options:

Configure Options
=================

List all :file:`configure` script options using::

    ./configure --help

See also the :file:`Misc/SpecialBuilds.txt` in the Python source distribution.

General Options
---------------

.. option:: --enable-loadable-sqlite-extensions

   Support loadable extensions in the :mod:`!_sqlite` extension module (default
   is no) of the :mod:`sqlite3` module.

   See the :meth:`sqlite3.Connection.enable_load_extension` method of the
   :mod:`sqlite3` module.

   .. versionadded:: 3.6

.. option:: --disable-ipv6

   Disable IPv6 support (enabled by default if supported), see the
   :mod:`socket` module.

.. option:: --enable-big-digits=[15|30]

   Define the size in bits of Python :class:`int` digits: 15 or 30 bits.

   By default, the digit size is 30.

   Define the ``PYLONG_BITS_IN_DIGIT`` to ``15`` or ``30``.

   See :data:`sys.int_info.bits_per_digit <sys.int_info>`.

.. option:: --with-suffix=SUFFIX

   Set the Python executable suffix to *SUFFIX*.

   The default suffix is ``.exe`` on Windows and macOS (``python.exe``
   executable), ``.js`` on Emscripten node, ``.html`` on Emscripten browser,
   ``.wasm`` on WASI, and an empty string on other platforms (``python``
   executable).

   .. versionchanged:: 3.11
      The default suffix on WASM platform is one of ``.js``, ``.html``
      or ``.wasm``.

.. option:: --with-tzpath=<list of absolute paths separated by pathsep>

   Select the default time zone search path for :const:`zoneinfo.TZPATH`.
   See the :ref:`Compile-time configuration
   <zoneinfo_data_compile_time_config>` of the :mod:`zoneinfo` module.

   Default: ``/usr/share/zoneinfo:/usr/lib/zoneinfo:/usr/share/lib/zoneinfo:/etc/zoneinfo``.

   See :data:`os.pathsep` path separator.

   .. versionadded:: 3.9

.. option:: --without-decimal-contextvar

   Build the ``_decimal`` extension module using a thread-local context rather
   than a coroutine-local context (default), see the :mod:`decimal` module.

   See :const:`decimal.HAVE_CONTEXTVAR` and the :mod:`contextvars` module.

   .. versionadded:: 3.9

.. option:: --with-dbmliborder=<list of backend names>

   Override order to check db backends for the :mod:`dbm` module

   A valid value is a colon (``:``) separated string with the backend names:

   * ``ndbm``;
   * ``gdbm``;
   * ``bdb``.

.. option:: --without-c-locale-coercion

   Disable C locale coercion to a UTF-8 based locale (enabled by default).

   Don't define the ``PY_COERCE_C_LOCALE`` macro.

   See :envvar:`PYTHONCOERCECLOCALE` and the :pep:`538`.

.. option:: --with-platlibdir=DIRNAME

   Python library directory name (default is ``lib``).

   Fedora and SuSE use ``lib64`` on 64-bit platforms.

   See :data:`sys.platlibdir`.

   .. versionadded:: 3.9

.. option:: --with-wheel-pkg-dir=PATH

   Directory of wheel packages used by the :mod:`ensurepip` module
   (none by default).

   Some Linux distribution packaging policies recommend against bundling
   dependencies. For example, Fedora installs wheel packages in the
   ``/usr/share/python-wheels/`` directory and don't install the
   :mod:`!ensurepip._bundled` package.

   .. versionadded:: 3.10

.. option:: --with-pkg-config=[check|yes|no]

   Whether configure should use :program:`pkg-config` to detect build
   dependencies.

   * ``check`` (default): :program:`pkg-config` is optional
   * ``yes``: :program:`pkg-config` is mandatory
   * ``no``: configure does not use :program:`pkg-config` even when present

   .. versionadded:: 3.11

.. option:: --with-missing-stdlib-config=FILE

   Path to a `JSON <https://www.json.org/json-en.html>`_ configuration file
   containing custom error messages for missing :term:`standard library` modules.

   This option is intended for Python distributors who wish to provide
   distribution-specific guidance when users encounter standard library
   modules that are missing or packaged separately.

   The JSON file should map missing module names to custom error message strings.
   For example, if your distribution packages :mod:`tkinter` and
   :mod:`_tkinter` separately and excludes :mod:`!_gdbm` for legal reasons,
   the configuration could contain:

   .. code-block:: json

      {
          "_gdbm": "The '_gdbm' module is not available in this distribution",
          "tkinter": "Install the python-tk package to use tkinter",
          "_tkinter": "Install the python-tk package to use tkinter",
      }

   .. versionadded:: 3.15

.. option:: --enable-pystats

   Turn on internal Python performance statistics gathering.

   By default, statistics gathering is off. Use ``python3 -X pystats`` command
   or set ``PYTHONSTATS=1`` environment variable to turn on statistics
   gathering at Python startup.

   At Python exit, dump statistics if statistics gathering was on and not
   cleared.

   Effects:

   * Add :option:`-X pystats <-X>` command line option.
   * Add :envvar:`!PYTHONSTATS` environment variable.
   * Define the ``Py_STATS`` macro.
   * Add functions to the :mod:`sys` module:

     * :func:`!sys._stats_on`: Turns on statistics gathering.
     * :func:`!sys._stats_off`: Turns off statistics gathering.
     * :func:`!sys._stats_clear`: Clears the statistics.
     * :func:`!sys._stats_dump`: Dump statistics to file, and clears the statistics.

   The statistics will be dumped to a arbitrary (probably unique) file in
   ``/tmp/py_stats/`` (Unix) or ``C:\temp\py_stats\`` (Windows). If that
   directory does not exist, results will be printed on stderr.

   Use ``Tools/scripts/summarize_stats.py`` to read the stats.

   Statistics:

   * Opcode:

     * Specialization: success, failure, hit, deferred, miss, deopt, failures;
     * Execution count;
     * Pair count.

   * Call:

     * Inlined Python calls;
     * PyEval calls;
     * Frames pushed;
     * Frame object created;
     * Eval calls: vector, generator, legacy, function VECTORCALL, build class,
       slot, function "ex", API, method.

   * Object:

     * incref and decref;
     * interpreter incref and decref;
     * allocations: all, 512 bytes, 4 kiB, big;
     * free;
     * to/from free lists;
     * dictionary materialized/dematerialized;
     * type cache;
     * optimization attempts;
     * optimization traces created/executed;
     * uops executed.

   * Garbage collector:

     * Garbage collections;
     * Objects visited;
     * Objects collected.

   .. versionadded:: 3.11

.. _free-threading-build:

.. option:: --disable-gil

   .. c:macro:: Py_GIL_DISABLED
      :no-typesetting:

   Enables support for running Python without the :term:`global interpreter
   lock` (GIL): free threading build.

   Defines the ``Py_GIL_DISABLED`` macro and adds ``"t"`` to
   :data:`sys.abiflags`.

   See :ref:`whatsnew313-free-threaded-cpython` for more detail.

   .. versionadded:: 3.13

.. option:: --enable-experimental-jit=[no|yes|yes-off|interpreter]

   Indicate how to integrate the :ref:`experimental just-in-time compiler <whatsnew314-jit-compiler>`.

   * ``no``: Don't build the JIT.
   * ``yes``: Enable the JIT. To disable it at runtime, set the environment
     variable :envvar:`PYTHON_JIT=0 <PYTHON_JIT>`.
   * ``yes-off``: Build the JIT, but disable it by default. To enable it at
     runtime, set the environment variable :envvar:`PYTHON_JIT=1 <PYTHON_JIT>`.
   * ``interpreter``: Enable the "JIT interpreter" (only useful for those
     debugging the JIT itself). To disable it at runtime, set the environment
     variable :envvar:`PYTHON_JIT=0 <PYTHON_JIT>`.

   ``--enable-experimental-jit=no`` is the default behavior if the option is not
   provided, and ``--enable-experimental-jit`` is shorthand for
   ``--enable-experimental-jit=yes``.  See :file:`Tools/jit/README.md` for more
   information, including how to install the necessary build-time dependencies.

   .. note::

      When building CPython with JIT enabled, ensure that your system has Python 3.11 or later installed.

   .. versionadded:: 3.13

.. option:: PKG_CONFIG

   Path to ``pkg-config`` utility.

.. option:: PKG_CONFIG_LIBDIR
.. option:: PKG_CONFIG_PATH

   ``pkg-config`` options.


C compiler options
------------------

.. option:: CC

   C compiler command.

.. option:: CFLAGS

   C compiler flags.

.. option:: CPP

   C preprocessor command.

.. option:: CPPFLAGS

   C preprocessor flags, e.g. :samp:`-I{include_dir}`.


Linker options
--------------

.. option:: LDFLAGS

   Linker flags, e.g. :samp:`-L{library_directory}`.

.. option:: LIBS

   Libraries to pass to the linker, e.g. :samp:`-l{library}`.

.. option:: MACHDEP

   Name for machine-dependent library files.


.. _configure-options-for-dependencies:

Options for third-party dependencies
------------------------------------

.. versionadded:: 3.11

.. option:: BZIP2_CFLAGS
.. option:: BZIP2_LIBS

   C compiler and linker flags to link Python to ``libbz2``, used by :mod:`bz2`
   module, overriding ``pkg-config``.

.. option:: CURSES_CFLAGS
.. option:: CURSES_LIBS

   C compiler and linker flags for ``libncurses`` or ``libncursesw``, used by
   :mod:`curses` module, overriding ``pkg-config``.

.. option:: GDBM_CFLAGS
.. option:: GDBM_LIBS

   C compiler and linker flags for ``gdbm``.

.. option:: LIBEDIT_CFLAGS
.. option:: LIBEDIT_LIBS

   C compiler and linker flags for ``libedit``, used by :mod:`readline` module,
   overriding ``pkg-config``.

.. option:: LIBFFI_CFLAGS
.. option:: LIBFFI_LIBS

   C compiler and linker flags for ``libffi``, used by :mod:`ctypes` module,
   overriding ``pkg-config``.

.. option:: LIBMPDEC_CFLAGS
.. option:: LIBMPDEC_LIBS

   C compiler and linker flags for ``libmpdec``, used by :mod:`decimal` module,
   overriding ``pkg-config``.

   .. note::

      These environment variables have no effect unless
      :option:`--with-system-libmpdec` is specified.

.. option:: LIBLZMA_CFLAGS
.. option:: LIBLZMA_LIBS

   C compiler and linker flags for ``liblzma``, used by :mod:`lzma` module,
   overriding ``pkg-config``.

.. option:: LIBREADLINE_CFLAGS
.. option:: LIBREADLINE_LIBS

   C compiler and linker flags for ``libreadline``, used by :mod:`readline`
   module, overriding ``pkg-config``.

.. option:: LIBSQLITE3_CFLAGS
.. option:: LIBSQLITE3_LIBS

   C compiler and linker flags for ``libsqlite3``, used by :mod:`sqlite3`
   module, overriding ``pkg-config``.

.. option:: LIBUUID_CFLAGS
.. option:: LIBUUID_LIBS

   C compiler and linker flags for ``libuuid``, used by :mod:`uuid` module,
   overriding ``pkg-config``.

.. option:: LIBZSTD_CFLAGS
.. option:: LIBZSTD_LIBS

   C compiler and linker flags for ``libzstd``, used by :mod:`compression.zstd` module,
   overriding ``pkg-config``.

   .. versionadded:: 3.14

.. option:: PANEL_CFLAGS
.. option:: PANEL_LIBS

   C compiler and linker flags for PANEL, overriding ``pkg-config``.

   C compiler and linker flags for ``libpanel`` or ``libpanelw``, used by
   :mod:`curses.panel` module, overriding ``pkg-config``.

.. option:: TCLTK_CFLAGS
.. option:: TCLTK_LIBS

   C compiler and linker flags for TCLTK, overriding ``pkg-config``.

.. option:: ZLIB_CFLAGS
.. option:: ZLIB_LIBS

   C compiler and linker flags for ``libzlib``, used by :mod:`gzip` module,
   overriding ``pkg-config``.


WebAssembly Options
-------------------

.. option:: --enable-wasm-dynamic-linking

   Turn on dynamic linking support for WASM.

   Dynamic linking enables ``dlopen``. File size of the executable
   increases due to limited dead code elimination and additional features.

   .. versionadded:: 3.11

.. option:: --enable-wasm-pthreads

   Turn on pthreads support for WASM.

   .. versionadded:: 3.11


Install Options
---------------

.. option:: --prefix=PREFIX

   Install architecture-independent files in PREFIX. On Unix, it
   defaults to :file:`/usr/local`.

   This value can be retrieved at runtime using :data:`sys.prefix`.

   As an example, one can use ``--prefix="$HOME/.local/"`` to install
   a Python in its home directory.

.. option:: --exec-prefix=EPREFIX

   Install architecture-dependent files in EPREFIX, defaults to :option:`--prefix`.

   This value can be retrieved at runtime using :data:`sys.exec_prefix`.

.. option:: --disable-test-modules

   Don't build nor install test modules, like the :mod:`test` package or the
   :mod:`!_testcapi` extension module (built and installed by default).

   .. versionadded:: 3.10

.. option:: --with-ensurepip=[upgrade|install|no]

   Select the :mod:`ensurepip` command run on Python installation:

   * ``upgrade`` (default): run ``python -m ensurepip --altinstall --upgrade``
     command.
   * ``install``: run ``python -m ensurepip --altinstall`` command;
   * ``no``: don't run ensurepip;

   .. versionadded:: 3.6


Performance options
-------------------

Configuring Python using ``--enable-optimizations --with-lto`` (PGO + LTO) is
recommended for best performance. The experimental ``--enable-bolt`` flag can
also be used to improve performance.

.. option:: --enable-optimizations

   Enable Profile Guided Optimization (PGO) using :envvar:`PROFILE_TASK`
   (disabled by default).

   The C compiler Clang requires ``llvm-profdata`` program for PGO. On
   macOS, GCC also requires it: GCC is just an alias to Clang on macOS.

   Disable also semantic interposition in libpython if ``--enable-shared`` and
   GCC is used: add ``-fno-semantic-interposition`` to the compiler and linker
   flags.

   .. note::

      During the build, you may encounter compiler warnings about
      profile data not being available for some source files.
      These warnings are harmless, as only a subset of the code is exercised
      during profile data acquisition.
      To disable these warnings on Clang, manually suppress them by adding
      ``-Wno-profile-instr-unprofiled`` to :envvar:`CFLAGS`.

   .. versionadded:: 3.6

   .. versionchanged:: 3.10
      Use ``-fno-semantic-interposition`` on GCC.

.. envvar:: PROFILE_TASK

   Environment variable used in the Makefile: Python command line arguments for
   the PGO generation task.

   Default: ``-m test --pgo --timeout=$(TESTTIMEOUT)``.

   .. versionadded:: 3.8

   .. versionchanged:: 3.13
      Task failure is no longer ignored silently.

.. option:: --with-lto=[full|thin|no|yes]

   Enable Link Time Optimization (LTO) in any build (disabled by default).

   The C compiler Clang requires ``llvm-ar`` for LTO (``ar`` on macOS), as well
   as an LTO-aware linker (``ld.gold`` or ``lld``).

   .. versionadded:: 3.6

   .. versionadded:: 3.11
      To use ThinLTO feature, use ``--with-lto=thin`` on Clang.

   .. versionchanged:: 3.12
      Use ThinLTO as the default optimization policy on Clang if the compiler accepts the flag.

.. option:: --enable-bolt

   Enable usage of the `BOLT post-link binary optimizer
   <https://github.com/llvm/llvm-project/tree/main/bolt>`_ (disabled by
   default).

   BOLT is part of the LLVM project but is not always included in their binary
   distributions. This flag requires that ``llvm-bolt`` and ``merge-fdata``
   are available.

   BOLT is still a fairly new project so this flag should be considered
   experimental for now. Because this tool operates on machine code its success
   is dependent on a combination of the build environment + the other
   optimization configure args + the CPU architecture, and not all combinations
   are supported.
   BOLT versions before LLVM 16 are known to crash BOLT under some scenarios.
   Use of LLVM 16 or newer for BOLT optimization is strongly encouraged.

   The :envvar:`!BOLT_INSTRUMENT_FLAGS` and :envvar:`!BOLT_APPLY_FLAGS`
   :program:`configure` variables can be defined to override the default set of
   arguments for :program:`llvm-bolt` to instrument and apply BOLT data to
   binaries, respectively.

   .. versionadded:: 3.12

.. option:: BOLT_APPLY_FLAGS

   Arguments to ``llvm-bolt`` when creating a `BOLT optimized binary
   <https://github.com/facebookarchive/BOLT>`_.

   .. versionadded:: 3.12

.. option:: BOLT_INSTRUMENT_FLAGS

   Arguments to ``llvm-bolt`` when instrumenting binaries.

   .. versionadded:: 3.12

.. option:: --with-computed-gotos

   Enable computed gotos in evaluation loop (enabled by default on supported
   compilers).

.. option:: --with-tail-call-interp

   Enable interpreters using tail calls in CPython. If enabled, enabling PGO
   (:option:`--enable-optimizations`) is highly recommended. This option specifically
   requires a C compiler with proper tail call support, and the
   `preserve_none <https://clang.llvm.org/docs/AttributeReference.html#preserve-none>`_
   calling convention. For example, Clang 19 and newer supports this feature.

   .. versionadded:: 3.14

.. option:: --without-mimalloc

   Disable the fast :ref:`mimalloc <mimalloc>` allocator
   (enabled by default).

   See also :envvar:`PYTHONMALLOC` environment variable.

.. option:: --without-pymalloc

   Disable the specialized Python memory allocator :ref:`pymalloc <pymalloc>`
   (enabled by default).

   See also :envvar:`PYTHONMALLOC` environment variable.

.. option:: --without-doc-strings

   Disable static documentation strings to reduce the memory footprint (enabled
   by default). Documentation strings defined in Python are not affected.

   Don't define the ``WITH_DOC_STRINGS`` macro.

   See the ``PyDoc_STRVAR()`` macro.

.. option:: --enable-profiling

   Enable C-level code profiling with ``gprof`` (disabled by default).

.. option:: --with-strict-overflow

   Add ``-fstrict-overflow`` to the C compiler flags (by default we add
   ``-fno-strict-overflow`` instead).

.. option:: --without-remote-debug

   Deactivate remote debugging support described in :pep:`768` (enabled by default).
   When this flag is provided the code that allows the interpreter to schedule the
   execution of a Python file in a separate process as described in :pep:`768` is
   not compiled. This includes both the functionality to schedule code to be executed
   and the functionality to receive code to be executed.

   .. c:macro:: Py_REMOTE_DEBUG

      This macro is defined by default, unless Python is configured with
      :option:`--without-remote-debug`.

      Note that even if the macro is defined, remote debugging may not be
      available (for example, on an incompatible platform).

   .. versionadded:: 3.14


.. _debug-build:

Python Debug Build
------------------

A debug build is Python built with the :option:`--with-pydebug` configure
option.

Effects of a debug build:

* Display all warnings by default: the list of default warning filters is empty
  in the :mod:`warnings` module.
* Add ``d`` to :data:`sys.abiflags`.
* Add :func:`!sys.gettotalrefcount` function.
* Add :option:`-X showrefcount <-X>` command line option.
* Add :option:`-d` command line option and :envvar:`PYTHONDEBUG` environment
  variable to debug the parser.
* Add support for the ``__lltrace__`` variable: enable low-level tracing in the
  bytecode evaluation loop if the variable is defined.
* Install :ref:`debug hooks on memory allocators <default-memory-allocators>`
  to detect buffer overflow and other memory errors.
* Define ``Py_DEBUG`` and ``Py_REF_DEBUG`` macros.
* Add runtime checks: code surrounded by ``#ifdef Py_DEBUG`` and ``#endif``.
  Enable ``assert(...)`` and ``_PyObject_ASSERT(...)`` assertions: don't set
  the ``NDEBUG`` macro (see also the :option:`--with-assertions` configure
  option). Main runtime checks:

  * Add sanity checks on the function arguments.
  * Unicode and int objects are created with their memory filled with a pattern
    to detect usage of uninitialized objects.
  * Ensure that functions which can clear or replace the current exception are
    not called with an exception raised.
  * Check that deallocator functions don't change the current exception.
  * The garbage collector (:func:`gc.collect` function) runs some basic checks
    on objects consistency.
  * The :c:macro:`!Py_SAFE_DOWNCAST()` macro checks for integer underflow and
    overflow when downcasting from wide types to narrow types.

See also the :ref:`Python Development Mode <devmode>` and the
:option:`--with-trace-refs` configure option.

.. versionchanged:: 3.8
   Release builds and debug builds are now ABI compatible: defining the
   ``Py_DEBUG`` macro no longer implies the ``Py_TRACE_REFS`` macro (see the
   :option:`--with-trace-refs` option).


Debug options
-------------

.. option:: --with-pydebug

   :ref:`Build Python in debug mode <debug-build>`: define the ``Py_DEBUG``
   macro (disabled by default).

.. option:: --with-trace-refs

   Enable tracing references for debugging purpose (disabled by default).

   Effects:

   * Define the ``Py_TRACE_REFS`` macro.
   * Add :func:`sys.getobjects` function.
   * Add :envvar:`PYTHONDUMPREFS` environment variable.

   The :envvar:`PYTHONDUMPREFS` environment variable can be used to dump
   objects and reference counts still alive at Python exit.

   :ref:`Statically allocated objects <static-types>` are not traced.

   .. versionadded:: 3.8

   .. versionchanged:: 3.13
      This build is now ABI compatible with release build and :ref:`debug build
      <debug-build>`.

.. option:: --with-assertions

   Build with C assertions enabled (default is no): ``assert(...);`` and
   ``_PyObject_ASSERT(...);``.

   If set, the ``NDEBUG`` macro is not defined in the :envvar:`OPT` compiler
   variable.

   See also the :option:`--with-pydebug` option (:ref:`debug build
   <debug-build>`) which also enables assertions.

   .. versionadded:: 3.6

.. option:: --with-valgrind

   Enable Valgrind support (default is no).

.. option:: --with-dtrace

   Enable DTrace support (default is no).

   See :ref:`Instrumenting CPython with DTrace and SystemTap
   <instrumentation>`.

   .. versionadded:: 3.6

.. option:: --with-address-sanitizer

   Enable AddressSanitizer memory error detector, ``asan`` (default is no).
   To improve ASan detection capabilities you may also want to combine this
   with :option:`--without-pymalloc` to disable the specialized small-object
   allocator whose allocations are not tracked by ASan.

   .. versionadded:: 3.6

.. option:: --with-memory-sanitizer

   Enable MemorySanitizer allocation error detector, ``msan`` (default is no).

   .. versionadded:: 3.6

.. option:: --with-undefined-behavior-sanitizer

   Enable UndefinedBehaviorSanitizer undefined behaviour detector, ``ubsan``
   (default is no).

   .. versionadded:: 3.6

.. option:: --with-thread-sanitizer

   Enable ThreadSanitizer data race detector, ``tsan``
   (default is no).

   .. versionadded:: 3.13


Linker options
--------------

.. option:: --enable-shared

   Enable building a shared Python library: ``libpython`` (default is no).

.. option:: --without-static-libpython

   Do not build ``libpythonMAJOR.MINOR.a`` and do not install ``python.o``
   (built and enabled by default).

   .. versionadded:: 3.10


Libraries options
-----------------

.. option:: --with-libs='lib1 ...'

   Link against additional libraries (default is no).

.. option:: --with-system-expat

   Build the :mod:`!pyexpat` module using an installed ``expat`` library
   (default is no).

.. option:: --with-system-libmpdec

   Build the ``_decimal`` extension module using an installed ``mpdecimal``
   library, see the :mod:`decimal` module (default is yes).

   .. versionadded:: 3.3

   .. versionchanged:: 3.13
      Default to using the installed ``mpdecimal`` library.

   .. versionchanged:: 3.15

      A bundled copy of the library will no longer be selected
      implicitly if an installed ``mpdecimal`` library is not found.
      In Python 3.15 only, it can still be selected explicitly using
      ``--with-system-libmpdec=no`` or ``--without-system-libmpdec``.

   .. deprecated-removed:: 3.13 3.16
      A copy of the ``mpdecimal`` library sources will no longer be distributed
      with Python 3.16.

   .. seealso:: :option:`LIBMPDEC_CFLAGS` and :option:`LIBMPDEC_LIBS`.

.. option:: --with-readline=readline|editline

   Designate a backend library for the :mod:`readline` module.

   * readline: Use readline as the backend.
   * editline: Use editline as the backend.

   .. versionadded:: 3.10

.. option:: --without-readline

   Don't build the :mod:`readline` module (built by default).

   Don't define the ``HAVE_LIBREADLINE`` macro.

   .. versionadded:: 3.10

.. option:: --with-libm=STRING

   Override ``libm`` math library to *STRING* (default is system-dependent).

.. option:: --with-libc=STRING

   Override ``libc`` C library to *STRING* (default is system-dependent).

.. option:: --with-openssl=DIR

   Root of the OpenSSL directory.

   .. versionadded:: 3.7

.. option:: --with-openssl-rpath=[no|auto|DIR]

   Set runtime library directory (rpath) for OpenSSL libraries:

   * ``no`` (default): don't set rpath;
   * ``auto``: auto-detect rpath from :option:`--with-openssl` and
     ``pkg-config``;
   * *DIR*: set an explicit rpath.

   .. versionadded:: 3.10


Security Options
----------------

.. option:: --with-hash-algorithm=[fnv|siphash13|siphash24]

   Select hash algorithm for use in ``Python/pyhash.c``:

   * ``siphash13`` (default);
   * ``siphash24``;
   * ``fnv``.

   .. versionadded:: 3.4

   .. versionadded:: 3.11
      ``siphash13`` is added and it is the new default.

.. option:: --with-builtin-hashlib-hashes=md5,sha1,sha256,sha512,sha3,blake2

   Built-in hash modules:

   * ``md5``;
   * ``sha1``;
   * ``sha256``;
   * ``sha512``;
   * ``sha3`` (with shake);
   * ``blake2``.

   .. versionadded:: 3.9

.. option:: --with-ssl-default-suites=[python|openssl|STRING]

   Override the OpenSSL default cipher suites string:

   * ``python`` (default): use Python's preferred selection;
   * ``openssl``: leave OpenSSL's defaults untouched;
   * *STRING*: use a custom string

   See the :mod:`ssl` module.

   .. versionadded:: 3.7

   .. versionchanged:: 3.10

      The settings ``python`` and *STRING* also set TLS 1.2 as minimum
      protocol version.

.. option:: --disable-safety

   Disable compiler options that are `recommended by OpenSSF`_ for security reasons with no performance overhead.
   If this option is not enabled, CPython will be built based on safety compiler options with no slow down.
   When this option is enabled, CPython will not be built with the compiler options listed below.

   The following compiler options are disabled with :option:`!--disable-safety`:

   * `-fstack-protector-strong`_: Enable run-time checks for stack-based buffer overflows.
   * `-Wtrampolines`_: Enable warnings about trampolines that require executable stacks.

   .. _recommended by OpenSSF: https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md
   .. _-fstack-protector-strong: https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md#enable-run-time-checks-for-stack-based-buffer-overflows
   .. _-Wtrampolines: https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md#enable-warning-about-trampolines-that-require-executable-stacks

   .. versionadded:: 3.14

.. option:: --enable-slower-safety

   Enable compiler options that are `recommended by OpenSSF`_ for security reasons which require overhead.
   If this option is not enabled, CPython will not be built based on safety compiler options which performance impact.
   When this option is enabled, CPython will be built with the compiler options listed below.

   The following compiler options are enabled with :option:`!--enable-slower-safety`:

   * `-D_FORTIFY_SOURCE=3`_: Fortify sources with compile- and run-time checks for unsafe libc usage and buffer overflows.

   .. _-D_FORTIFY_SOURCE=3: https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md#fortify-sources-for-unsafe-libc-usage-and-buffer-overflows

   .. versionadded:: 3.14


macOS Options
-------------

See :source:`Mac/README.rst`.

.. option:: --enable-universalsdk
.. option:: --enable-universalsdk=SDKDIR

   Create a universal binary build. *SDKDIR* specifies which macOS SDK should
   be used to perform the build (default is no).

.. option:: --enable-framework
.. option:: --enable-framework=INSTALLDIR

   Create a Python.framework rather than a traditional Unix install. Optional
   *INSTALLDIR* specifies the installation path (default is no).

.. option:: --with-universal-archs=ARCH

   Specify the kind of universal binary that should be created. This option is
   only valid when :option:`--enable-universalsdk` is set.

   Options:

   * ``universal2`` (x86-64 and arm64);
   * ``32-bit`` (PPC and i386);
   * ``64-bit``  (PPC64 and x86-64);
   * ``3-way`` (i386, PPC and x86-64);
   * ``intel`` (i386 and x86-64);
   * ``intel-32`` (i386);
   * ``intel-64`` (x86-64);
   * ``all``  (PPC, i386, PPC64 and x86-64).

   Note that values for this configuration item are *not* the same as the
   identifiers used for universal binary wheels on macOS. See the Python
   Packaging User Guide for details on the `packaging platform compatibility
   tags used on macOS
   <https://packaging.python.org/en/latest/specifications/platform-compatibility-tags/#macos>`_

.. option:: --with-framework-name=FRAMEWORK

   Specify the name for the python framework on macOS only valid when
   :option:`--enable-framework` is set (default: ``Python``).

.. option:: --with-app-store-compliance
.. option:: --with-app-store-compliance=PATCH-FILE

   The Python standard library contains strings that are known to trigger
   automated inspection tool errors when submitted for distribution by
   the macOS and iOS App Stores. If enabled, this option will apply the list of
   patches that are known to correct app store compliance. A custom patch
   file can also be specified. This option is disabled by default.

   .. versionadded:: 3.13

iOS Options
-----------

See :source:`iOS/README.rst`.

.. option:: --enable-framework=INSTALLDIR

   Create a Python.framework. Unlike macOS, the *INSTALLDIR* argument
   specifying the installation path is mandatory.

.. option:: --with-framework-name=FRAMEWORK

   Specify the name for the framework (default: ``Python``).


Cross Compiling Options
-----------------------

Cross compiling, also known as cross building, can be used to build Python
for another CPU architecture or platform. Cross compiling requires a Python
interpreter for the build platform. The version of the build Python must match
the version of the cross compiled host Python.

.. option:: --build=BUILD

   configure for building on BUILD, usually guessed by :program:`config.guess`.

.. option:: --host=HOST

   cross-compile to build programs to run on HOST (target platform)

.. option:: --with-build-python=path/to/python

   path to build ``python`` binary for cross compiling

   .. versionadded:: 3.11

.. option:: CONFIG_SITE=file

   An environment variable that points to a file with configure overrides.

   Example *config.site* file:

   .. code-block:: ini

      # config.site-aarch64
      ac_cv_buggy_getaddrinfo=no
      ac_cv_file__dev_ptmx=yes
      ac_cv_file__dev_ptc=no

.. option:: HOSTRUNNER

   Program to run CPython for the host platform for cross-compilation.

   .. versionadded:: 3.11


Cross compiling example::

   CONFIG_SITE=config.site-aarch64 ../configure \
       --build=x86_64-pc-linux-gnu \
       --host=aarch64-unknown-linux-gnu \
       --with-build-python=../x86_64/python


Python Build System
===================

Main files of the build system
------------------------------

* :file:`configure.ac` => :file:`configure`;
* :file:`Makefile.pre.in` => :file:`Makefile` (created by :file:`configure`);
* :file:`pyconfig.h` (created by :file:`configure`);
* :file:`Modules/Setup`: C extensions built by the Makefile using
  :file:`Module/makesetup` shell script;

Main build steps
----------------

* C files (``.c``) are built as object files (``.o``).
* A static ``libpython`` library (``.a``) is created from objects files.
* ``python.o`` and the static ``libpython`` library are linked into the
  final ``python`` program.
* C extensions are built by the Makefile (see :file:`Modules/Setup`).

Main Makefile targets
---------------------

make
^^^^

For the most part, when rebuilding after editing some code or
refreshing your checkout from upstream, all you need to do is execute
``make``, which (per Make's semantics) builds the default target, the
first one defined in the Makefile.  By tradition (including in the
CPython project) this is usually the ``all`` target. The
``configure`` script expands an ``autoconf`` variable,
``@DEF_MAKE_ALL_RULE@`` to describe precisely which targets ``make
all`` will build. The three choices are:

* ``profile-opt`` (configured with ``--enable-optimizations``)
* ``build_wasm`` (chosen if the host platform matches ``wasm32-wasi*`` or
  ``wasm32-emscripten``)
* ``build_all`` (configured without explicitly using either of the others)

Depending on the most recent source file changes, Make will rebuild
any targets (object files and executables) deemed out-of-date,
including running ``configure`` again if necessary. Source/target
dependencies are many and maintained manually however, so Make
sometimes doesn't have all the information necessary to correctly
detect all targets which need to be rebuilt.  Depending on which
targets aren't rebuilt, you might experience a number of problems. If
you have build or test problems which you can't otherwise explain,
``make clean && make`` should work around most dependency problems, at
the expense of longer build times.


make platform
^^^^^^^^^^^^^

Build the ``python`` program, but don't build the standard library
extension modules. This generates a file named ``platform`` which
contains a single line describing the details of the build platform,
e.g., ``macosx-14.3-arm64-3.12`` or ``linux-x86_64-3.13``.


make profile-opt
^^^^^^^^^^^^^^^^

Build Python using profile-guided optimization (PGO).  You can use the
configure :option:`--enable-optimizations` option to make this the
default target of the ``make`` command (``make all`` or just
``make``).



make clean
^^^^^^^^^^

Remove built files.


make distclean
^^^^^^^^^^^^^^

In addition to the work done by ``make clean``, remove files
created by the configure script.  ``configure`` will have to be run
before building again. [#]_


make install
^^^^^^^^^^^^

Build the ``all`` target and install Python.


make test
^^^^^^^^^

Build the ``all`` target and run the Python test suite with the
``--fast-ci`` option without GUI tests. Variables:

* ``TESTOPTS``: additional regrtest command-line options.
* ``TESTPYTHONOPTS``: additional Python command-line options.
* ``TESTTIMEOUT``: timeout in seconds (default: 10 minutes).


make ci
^^^^^^^

This is similar to ``make test``, but uses the ``-ugui`` to also run GUI tests.

.. versionadded:: 3.14


make buildbottest
^^^^^^^^^^^^^^^^^

This is similar to ``make test``, but uses the ``--slow-ci``
option and default timeout of 20 minutes, instead of ``--fast-ci`` option.


make regen-all
^^^^^^^^^^^^^^

Regenerate (almost) all generated files. These include (but are not
limited to) bytecode cases, and parser generator file.
``make regen-stdlib-module-names`` and ``autoconf`` must be run
separately for the remaining `generated files <#generated-files>`_.


C extensions
------------

Some C extensions are built as built-in modules, like the ``sys`` module.
They are built with the ``Py_BUILD_CORE_BUILTIN`` macro defined.
Built-in modules have no ``__file__`` attribute:

.. code-block:: pycon

    >>> import sys
    >>> sys
    <module 'sys' (built-in)>
    >>> sys.__file__
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    AttributeError: module 'sys' has no attribute '__file__'

Other C extensions are built as dynamic libraries, like the ``_asyncio`` module.
They are built with the ``Py_BUILD_CORE_MODULE`` macro defined.
Example on Linux x86-64:

.. code-block:: pycon

    >>> import _asyncio
    >>> _asyncio
    <module '_asyncio' from '/usr/lib64/python3.9/lib-dynload/_asyncio.cpython-39-x86_64-linux-gnu.so'>
    >>> _asyncio.__file__
    '/usr/lib64/python3.9/lib-dynload/_asyncio.cpython-39-x86_64-linux-gnu.so'

:file:`Modules/Setup` is used to generate Makefile targets to build C extensions.
At the beginning of the files, C extensions are built as built-in modules.
Extensions defined after the ``*shared*`` marker are built as dynamic libraries.

The :c:macro:`!PyAPI_FUNC()`, :c:macro:`!PyAPI_DATA()` and
:c:macro:`PyMODINIT_FUNC` macros of :file:`Include/exports.h` are defined
differently depending if the ``Py_BUILD_CORE_MODULE`` macro is defined:

* Use ``Py_EXPORTED_SYMBOL`` if the ``Py_BUILD_CORE_MODULE`` is defined
* Use ``Py_IMPORTED_SYMBOL`` otherwise.

If the ``Py_BUILD_CORE_BUILTIN`` macro is used by mistake on a C extension
built as a shared library, its :samp:`PyInit_{xxx}()` function is not exported,
causing an :exc:`ImportError` on import.


Compiler and linker flags
=========================

Options set by the ``./configure`` script and environment variables and used by
``Makefile``.

Preprocessor flags
------------------

.. envvar:: CONFIGURE_CPPFLAGS

   Value of :envvar:`CPPFLAGS` variable passed to the ``./configure`` script.

   .. versionadded:: 3.6

.. envvar:: CPPFLAGS

   (Objective) C/C++ preprocessor flags, e.g. :samp:`-I{include_dir}` if you have
   headers in a nonstandard directory *include_dir*.

   Both :envvar:`CPPFLAGS` and :envvar:`LDFLAGS` need to contain the shell's
   value to be able to build extension modules using the
   directories specified in the environment variables.

.. envvar:: BASECPPFLAGS

   .. versionadded:: 3.4

.. envvar:: PY_CPPFLAGS

   Extra preprocessor flags added for building the interpreter object files.

   Default: ``$(BASECPPFLAGS) -I. -I$(srcdir)/Include $(CONFIGURE_CPPFLAGS) $(CPPFLAGS)``.

   .. versionadded:: 3.2

Compiler flags
--------------

.. envvar:: CC

   C compiler command.

   Example: ``gcc -pthread``.

.. envvar:: CXX

   C++ compiler command.

   Example: ``g++ -pthread``.

.. envvar:: CFLAGS

   C compiler flags.

.. envvar:: CFLAGS_NODIST

   :envvar:`CFLAGS_NODIST` is used for building the interpreter and stdlib C
   extensions.  Use it when a compiler flag should *not* be part of
   :envvar:`CFLAGS` once Python is installed (:gh:`65320`).

   In particular, :envvar:`CFLAGS` should not contain:

   * the compiler flag ``-I`` (for setting the search path for include files).
     The ``-I`` flags are processed from left to right, and any flags in
     :envvar:`CFLAGS` would take precedence over user- and package-supplied ``-I``
     flags.

   * hardening flags such as ``-Werror`` because distributions cannot control
     whether packages installed by users conform to such heightened
     standards.

   .. versionadded:: 3.5

.. envvar:: COMPILEALL_OPTS

   Options passed to the :mod:`compileall` command line when building PYC files
   in ``make install``. Default: ``-j0``.

   .. versionadded:: 3.12

.. envvar:: EXTRA_CFLAGS

   Extra C compiler flags.

.. envvar:: CONFIGURE_CFLAGS

   Value of :envvar:`CFLAGS` variable passed to the ``./configure``
   script.

   .. versionadded:: 3.2

.. envvar:: CONFIGURE_CFLAGS_NODIST

   Value of :envvar:`CFLAGS_NODIST` variable passed to the ``./configure``
   script.

   .. versionadded:: 3.5

.. envvar:: BASECFLAGS

   Base compiler flags.

.. envvar:: OPT

   Optimization flags.

.. envvar:: CFLAGS_ALIASING

   Strict or non-strict aliasing flags used to compile ``Python/dtoa.c``.

   .. versionadded:: 3.7

.. envvar:: CCSHARED

   Compiler flags used to build a shared library.

   For example, ``-fPIC`` is used on Linux and on BSD.

.. envvar:: CFLAGSFORSHARED

   Extra C flags added for building the interpreter object files.

   Default: ``$(CCSHARED)`` when :option:`--enable-shared` is used, or an empty
   string otherwise.

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

   Compiler flags to build a standard library extension module as a built-in
   module, like the :mod:`posix` module.

   Default: ``$(PY_STDMODULE_CFLAGS) -DPy_BUILD_CORE_BUILTIN``.

   .. versionadded:: 3.8

.. envvar:: PURIFY

   Purify command. Purify is a memory debugger program.

   Default: empty string (not used).


Linker flags
------------

.. envvar:: LINKCC

   Linker command used to build programs like ``python`` and ``_testembed``.

   Default: ``$(PURIFY) $(CC)``.

.. envvar:: CONFIGURE_LDFLAGS

   Value of :envvar:`LDFLAGS` variable passed to the ``./configure`` script.

   Avoid assigning :envvar:`CFLAGS`, :envvar:`LDFLAGS`, etc. so users can use
   them on the command line to append to these values without stomping the
   pre-set values.

   .. versionadded:: 3.2

.. envvar:: LDFLAGS_NODIST

   :envvar:`LDFLAGS_NODIST` is used in the same manner as
   :envvar:`CFLAGS_NODIST`.  Use it when a linker flag should *not* be part of
   :envvar:`LDFLAGS` once Python is installed (:gh:`65320`).

   In particular, :envvar:`LDFLAGS` should not contain:

   * the compiler flag ``-L`` (for setting the search path for libraries).
     The ``-L`` flags are processed from left to right, and any flags in
     :envvar:`LDFLAGS` would take precedence over user- and package-supplied ``-L``
     flags.

.. envvar:: CONFIGURE_LDFLAGS_NODIST

   Value of :envvar:`LDFLAGS_NODIST` variable passed to the ``./configure``
   script.

   .. versionadded:: 3.8

.. envvar:: LDFLAGS

   Linker flags, e.g. :samp:`-L{lib_dir}` if you have libraries in a nonstandard
   directory *lib_dir*.

   Both :envvar:`CPPFLAGS` and :envvar:`LDFLAGS` need to contain the shell's
   value to be able to build extension modules using the
   directories specified in the environment variables.

.. envvar:: LIBS

   Linker flags to pass libraries to the linker when linking the Python
   executable.

   Example: ``-lrt``.

.. envvar:: LDSHARED

   Command to build a shared library.

   Default: ``@LDSHARED@ $(PY_LDFLAGS)``.

.. envvar:: BLDSHARED

   Command to build ``libpython`` shared library.

   Default: ``@BLDSHARED@ $(PY_CORE_LDFLAGS)``.

.. envvar:: PY_LDFLAGS

   Default: ``$(CONFIGURE_LDFLAGS) $(LDFLAGS)``.

.. envvar:: PY_LDFLAGS_NODIST

   Default: ``$(CONFIGURE_LDFLAGS_NODIST) $(LDFLAGS_NODIST)``.

   .. versionadded:: 3.8

.. envvar:: PY_CORE_LDFLAGS

   Linker flags used for building the interpreter object files.

   .. versionadded:: 3.8


.. rubric:: Footnotes

.. [#] ``git clean -fdx`` is an even more extreme way to "clean" your
   checkout. It removes all files not known to Git.
   When bug hunting using ``git bisect``, this is
   `recommended between probes <https://github.com/python/cpython/issues/114505#issuecomment-1907021718>`_
   to guarantee a completely clean build. **Use with care**, as it
   will delete all files not checked into Git, including your
   new, uncommitted work.
