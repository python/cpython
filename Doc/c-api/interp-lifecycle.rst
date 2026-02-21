.. highlight:: c

.. _initialization:

Interpreter initialization and finalization
===========================================

See :ref:`Python Initialization Configuration <init-config>` for details
on how to configure the interpreter prior to initialization.

.. _pre-init-safe:

Before Python initialization
----------------------------

In an application embedding Python, the :c:func:`Py_Initialize` function must
be called before using any other Python/C API functions; with the exception of
a few functions and the :ref:`global configuration variables
<global-conf-vars>`.

The following functions can be safely called before Python is initialized:

* Functions that initialize the interpreter:

  * :c:func:`Py_Initialize`
  * :c:func:`Py_InitializeEx`
  * :c:func:`Py_InitializeFromConfig`
  * :c:func:`Py_BytesMain`
  * :c:func:`Py_Main`
  * the runtime pre-initialization functions covered in :ref:`init-config`

* Configuration functions:

  * :c:func:`PyImport_AppendInittab`
  * :c:func:`PyImport_ExtendInittab`
  * :c:func:`!PyInitFrozenExtensions`
  * :c:func:`PyMem_SetAllocator`
  * :c:func:`PyMem_SetupDebugHooks`
  * :c:func:`PyObject_SetArenaAllocator`
  * :c:func:`Py_SetProgramName`
  * :c:func:`Py_SetPythonHome`
  * the configuration functions covered in :ref:`init-config`

* Informative functions:

  * :c:func:`Py_IsInitialized`
  * :c:func:`PyMem_GetAllocator`
  * :c:func:`PyObject_GetArenaAllocator`
  * :c:func:`Py_GetBuildInfo`
  * :c:func:`Py_GetCompiler`
  * :c:func:`Py_GetCopyright`
  * :c:func:`Py_GetPlatform`
  * :c:func:`Py_GetVersion`
  * :c:func:`Py_IsInitialized`

* Utilities:

  * :c:func:`Py_DecodeLocale`
  * the status reporting and utility functions covered in :ref:`init-config`

* Memory allocators:

  * :c:func:`PyMem_RawMalloc`
  * :c:func:`PyMem_RawRealloc`
  * :c:func:`PyMem_RawCalloc`
  * :c:func:`PyMem_RawFree`

* Synchronization:

  * :c:func:`PyMutex_Lock`
  * :c:func:`PyMutex_Unlock`

.. note::

   Despite their apparent similarity to some of the functions listed above,
   the following functions **should not be called** before the interpreter has
   been initialized: :c:func:`Py_EncodeLocale`, :c:func:`PyEval_InitThreads`, and
   :c:func:`Py_RunMain`.


.. _global-conf-vars:

Global configuration variables
------------------------------

Python has variables for the global configuration to control different features
and options. By default, these flags are controlled by :ref:`command line
options <using-on-interface-options>`.

When a flag is set by an option, the value of the flag is the number of times
that the option was set. For example, ``-b`` sets :c:data:`Py_BytesWarningFlag`
to 1 and ``-bb`` sets :c:data:`Py_BytesWarningFlag` to 2.


.. c:var:: int Py_BytesWarningFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.bytes_warning` should be used instead, see :ref:`Python
   Initialization Configuration <init-config>`.

   Issue a warning when comparing :class:`bytes` or :class:`bytearray` with
   :class:`str` or :class:`bytes` with :class:`int`.  Issue an error if greater
   or equal to ``2``.

   Set by the :option:`-b` option.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_DebugFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.parser_debug` should be used instead, see :ref:`Python
   Initialization Configuration <init-config>`.

   Turn on parser debugging output (for expert only, depending on compilation
   options).

   Set by the :option:`-d` option and the :envvar:`PYTHONDEBUG` environment
   variable.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_DontWriteBytecodeFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.write_bytecode` should be used instead, see :ref:`Python
   Initialization Configuration <init-config>`.

   If set to non-zero, Python won't try to write ``.pyc`` files on the
   import of source modules.

   Set by the :option:`-B` option and the :envvar:`PYTHONDONTWRITEBYTECODE`
   environment variable.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_FrozenFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.pathconfig_warnings` should be used instead, see
   :ref:`Python Initialization Configuration <init-config>`.

   Private flag used by ``_freeze_module`` and ``frozenmain`` programs.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_HashRandomizationFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.hash_seed` and :c:member:`PyConfig.use_hash_seed` should
   be used instead, see :ref:`Python Initialization Configuration
   <init-config>`.

   Set to ``1`` if the :envvar:`PYTHONHASHSEED` environment variable is set to
   a non-empty string.

   If the flag is non-zero, read the :envvar:`PYTHONHASHSEED` environment
   variable to initialize the secret hash seed.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_IgnoreEnvironmentFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.use_environment` should be used instead, see
   :ref:`Python Initialization Configuration <init-config>`.

   Ignore all :envvar:`!PYTHON*` environment variables, e.g.
   :envvar:`PYTHONPATH` and :envvar:`PYTHONHOME`, that might be set.

   Set by the :option:`-E` and :option:`-I` options.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_InspectFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.inspect` should be used instead, see
   :ref:`Python Initialization Configuration <init-config>`.

   When a script is passed as first argument or the :option:`-c` option is used,
   enter interactive mode after executing the script or the command, even when
   :data:`sys.stdin` does not appear to be a terminal.

   Set by the :option:`-i` option and the :envvar:`PYTHONINSPECT` environment
   variable.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_InteractiveFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.interactive` should be used instead, see
   :ref:`Python Initialization Configuration <init-config>`.

   Set by the :option:`-i` option.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_IsolatedFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.isolated` should be used instead, see
   :ref:`Python Initialization Configuration <init-config>`.

   Run Python in isolated mode. In isolated mode :data:`sys.path` contains
   neither the script's directory nor the user's site-packages directory.

   Set by the :option:`-I` option.

   .. versionadded:: 3.4

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_LegacyWindowsFSEncodingFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyPreConfig.legacy_windows_fs_encoding` should be used instead, see
   :ref:`Python Initialization Configuration <init-config>`.

   If the flag is non-zero, use the ``mbcs`` encoding with ``replace`` error
   handler, instead of the UTF-8 encoding with ``surrogatepass`` error handler,
   for the :term:`filesystem encoding and error handler`.

   Set to ``1`` if the :envvar:`PYTHONLEGACYWINDOWSFSENCODING` environment
   variable is set to a non-empty string.

   See :pep:`529` for more details.

   .. availability:: Windows.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_LegacyWindowsStdioFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.legacy_windows_stdio` should be used instead, see
   :ref:`Python Initialization Configuration <init-config>`.

   If the flag is non-zero, use :class:`io.FileIO` instead of
   :class:`!io._WindowsConsoleIO` for :mod:`sys` standard streams.

   Set to ``1`` if the :envvar:`PYTHONLEGACYWINDOWSSTDIO` environment
   variable is set to a non-empty string.

   See :pep:`528` for more details.

   .. availability:: Windows.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_NoSiteFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.site_import` should be used instead, see
   :ref:`Python Initialization Configuration <init-config>`.

   Disable the import of the module :mod:`site` and the site-dependent
   manipulations of :data:`sys.path` that it entails.  Also disable these
   manipulations if :mod:`site` is explicitly imported later (call
   :func:`site.main` if you want them to be triggered).

   Set by the :option:`-S` option.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_NoUserSiteDirectory

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.user_site_directory` should be used instead, see
   :ref:`Python Initialization Configuration <init-config>`.

   Don't add the :data:`user site-packages directory <site.USER_SITE>` to
   :data:`sys.path`.

   Set by the :option:`-s` and :option:`-I` options, and the
   :envvar:`PYTHONNOUSERSITE` environment variable.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_OptimizeFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.optimization_level` should be used instead, see
   :ref:`Python Initialization Configuration <init-config>`.

   Set by the :option:`-O` option and the :envvar:`PYTHONOPTIMIZE` environment
   variable.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_QuietFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.quiet` should be used instead, see :ref:`Python
   Initialization Configuration <init-config>`.

   Don't display the copyright and version messages even in interactive mode.

   Set by the :option:`-q` option.

   .. versionadded:: 3.2

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_UnbufferedStdioFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.buffered_stdio` should be used instead, see :ref:`Python
   Initialization Configuration <init-config>`.

   Force the stdout and stderr streams to be unbuffered.

   Set by the :option:`-u` option and the :envvar:`PYTHONUNBUFFERED`
   environment variable.

   .. deprecated-removed:: 3.12 3.15


.. c:var:: int Py_VerboseFlag

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.verbose` should be used instead, see :ref:`Python
   Initialization Configuration <init-config>`.

   Print a message each time a module is initialized, showing the place
   (filename or built-in module) from which it is loaded.  If greater or equal
   to ``2``, print a message for each file that is checked for when
   searching for a module. Also provides information on module cleanup at exit.

   Set by the :option:`-v` option and the :envvar:`PYTHONVERBOSE` environment
   variable.

   .. deprecated-removed:: 3.12 3.15


Initializing and finalizing the interpreter
-------------------------------------------

.. c:function:: void Py_Initialize()

   .. index::
      single: PyEval_InitThreads()
      single: modules (in module sys)
      single: path (in module sys)
      pair: module; builtins
      pair: module; __main__
      pair: module; sys
      triple: module; search; path
      single: Py_FinalizeEx (C function)

   Initialize the Python interpreter.  In an application embedding Python,
   this should be called before using any other Python/C API functions; see
   :ref:`Before Python Initialization <pre-init-safe>` for the few exceptions.

   This initializes the table of loaded modules (``sys.modules``), and creates
   the fundamental modules :mod:`builtins`, :mod:`__main__` and :mod:`sys`.
   It also initializes the module search path (``sys.path``). It does not set
   ``sys.argv``; use the :ref:`Python Initialization Configuration <init-config>`
   API for that. This is a no-op when called for a second time (without calling
   :c:func:`Py_FinalizeEx` first).  There is no return value; it is a fatal
   error if the initialization fails.

   Use :c:func:`Py_InitializeFromConfig` to customize the
   :ref:`Python Initialization Configuration <init-config>`.

   .. note::
      On Windows, changes the console mode from ``O_TEXT`` to ``O_BINARY``,
      which will also affect non-Python uses of the console using the C Runtime.


.. c:function:: void Py_InitializeEx(int initsigs)

   This function works like :c:func:`Py_Initialize` if *initsigs* is ``1``. If
   *initsigs* is ``0``, it skips initialization registration of signal handlers,
   which may be useful when CPython is embedded as part of a larger application.

   Use :c:func:`Py_InitializeFromConfig` to customize the
   :ref:`Python Initialization Configuration <init-config>`.


.. c:function:: PyStatus Py_InitializeFromConfig(const PyConfig *config)

   Initialize Python from *config* configuration, as described in
   :ref:`init-from-config`.

   See the :ref:`init-config` section for details on pre-initializing the
   interpreter, populating the runtime configuration structure, and querying
   the returned status structure.


.. c:function:: int Py_IsInitialized()

   Return true (nonzero) when the Python interpreter has been initialized, false
   (zero) if not.  After :c:func:`Py_FinalizeEx` is called, this returns false until
   :c:func:`Py_Initialize` is called again.


.. c:function:: int Py_IsFinalizing()

   Return true (non-zero) if the main Python interpreter is
   :term:`shutting down <interpreter shutdown>`. Return false (zero) otherwise.

   .. versionadded:: 3.13


.. c:function:: int Py_FinalizeEx()

   Undo all initializations made by :c:func:`Py_Initialize` and subsequent use of
   Python/C API functions, and destroy all sub-interpreters (see
   :c:func:`Py_NewInterpreter` below) that were created and not yet destroyed since
   the last call to :c:func:`Py_Initialize`.  This is a no-op when called for a second
   time (without calling :c:func:`Py_Initialize` again first).

   Since this is the reverse of :c:func:`Py_Initialize`, it should be called
   in the same thread with the same interpreter active.  That means
   the main thread and the main interpreter.
   This should never be called while :c:func:`Py_RunMain` is running.

   Normally the return value is ``0``.
   If there were errors during finalization (flushing buffered data),
   ``-1`` is returned.

   Note that Python will do a best effort at freeing all memory allocated by the Python
   interpreter.  Therefore, any C-Extension should make sure to correctly clean up all
   of the previously allocated PyObjects before using them in subsequent calls to
   :c:func:`Py_Initialize`.  Otherwise it could introduce vulnerabilities and incorrect
   behavior.

   This function is provided for a number of reasons.  An embedding application
   might want to restart Python without having to restart the application itself.
   An application that has loaded the Python interpreter from a dynamically
   loadable library (or DLL) might want to free all memory allocated by Python
   before unloading the DLL. During a hunt for memory leaks in an application a
   developer might want to free all memory allocated by Python before exiting from
   the application.

   **Bugs and caveats:** The destruction of modules and objects in modules is done
   in random order; this may cause destructors (:meth:`~object.__del__` methods) to fail
   when they depend on other objects (even functions) or modules.  Dynamically
   loaded extension modules loaded by Python are not unloaded.  Small amounts of
   memory allocated by the Python interpreter may not be freed (if you find a leak,
   please report it).  Memory tied up in circular references between objects is not
   freed.  Interned strings will all be deallocated regardless of their reference count.
   Some memory allocated by extension modules may not be freed.  Some extensions may not
   work properly if their initialization routine is called more than once; this can
   happen if an application calls :c:func:`Py_Initialize` and :c:func:`Py_FinalizeEx`
   more than once.  :c:func:`Py_FinalizeEx` must not be called recursively from
   within itself.  Therefore, it must not be called by any code that may be run
   as part of the interpreter shutdown process, such as :py:mod:`atexit`
   handlers, object finalizers, or any code that may be run while flushing the
   stdout and stderr files.

   .. audit-event:: cpython._PySys_ClearAuditHooks "" c.Py_FinalizeEx

   .. versionadded:: 3.6


.. c:function:: void Py_Finalize()

   This is a backwards-compatible version of :c:func:`Py_FinalizeEx` that
   disregards the return value.


.. c:function:: int Py_BytesMain(int argc, char **argv)

   Similar to :c:func:`Py_Main` but *argv* is an array of bytes strings,
   allowing the calling application to delegate the text decoding step to
   the CPython runtime.

   .. versionadded:: 3.8


.. c:function:: int Py_Main(int argc, wchar_t **argv)

   The main program for the standard interpreter, encapsulating a full
   initialization/finalization cycle, as well as additional
   behaviour to implement reading configurations settings from the environment
   and command line, and then executing ``__main__`` in accordance with
   :ref:`using-on-cmdline`.

   This is made available for programs which wish to support the full CPython
   command line interface, rather than just embedding a Python runtime in a
   larger application.

   The *argc* and *argv* parameters are similar to those which are passed to a
   C program's :c:func:`main` function, except that the *argv* entries are first
   converted to ``wchar_t`` using :c:func:`Py_DecodeLocale`. It is also
   important to note that the argument list entries may be modified to point to
   strings other than those passed in (however, the contents of the strings
   pointed to by the argument list are not modified).

   The return value is ``2`` if the argument list does not represent a valid
   Python command line, and otherwise the same as :c:func:`Py_RunMain`.

   In terms of the CPython runtime configuration APIs documented in the
   :ref:`runtime configuration <init-config>` section (and without accounting
   for error handling), ``Py_Main`` is approximately equivalent to::

      PyConfig config;
      PyConfig_InitPythonConfig(&config);
      PyConfig_SetArgv(&config, argc, argv);
      Py_InitializeFromConfig(&config);
      PyConfig_Clear(&config);

      Py_RunMain();

   In normal usage, an embedding application will call this function
   *instead* of calling :c:func:`Py_Initialize`, :c:func:`Py_InitializeEx` or
   :c:func:`Py_InitializeFromConfig` directly, and all settings will be applied
   as described elsewhere in this documentation. If this function is instead
   called *after* a preceding runtime initialization API call, then exactly
   which environmental and command line configuration settings will be updated
   is version dependent (as it depends on which settings correctly support
   being modified after they have already been set once when the runtime was
   first initialized).


.. c:function:: int Py_RunMain(void)

   Executes the main module in a fully configured CPython runtime.

   Executes the command (:c:member:`PyConfig.run_command`), the script
   (:c:member:`PyConfig.run_filename`) or the module
   (:c:member:`PyConfig.run_module`) specified on the command line or in the
   configuration. If none of these values are set, runs the interactive Python
   prompt (REPL) using the ``__main__`` module's global namespace.

   If :c:member:`PyConfig.inspect` is not set (the default), the return value
   will be ``0`` if the interpreter exits normally (that is, without raising
   an exception), the exit status of an unhandled :exc:`SystemExit`, or ``1``
   for any other unhandled exception.

   If :c:member:`PyConfig.inspect` is set (such as when the :option:`-i` option
   is used), rather than returning when the interpreter exits, execution will
   instead resume in an interactive Python prompt (REPL) using the ``__main__``
   module's global namespace. If the interpreter exited with an exception, it
   is immediately raised in the REPL session. The function return value is
   then determined by the way the *REPL session* terminates: ``0``, ``1``, or
   the status of a :exc:`SystemExit`, as specified above.

   This function always finalizes the Python interpreter before it returns.

   See :ref:`Python Configuration <init-python-config>` for an example of a
   customized Python that always runs in isolated mode using
   :c:func:`Py_RunMain`.

.. c:function:: int PyUnstable_AtExit(PyInterpreterState *interp, void (*func)(void *), void *data)

   Register an :mod:`atexit` callback for the target interpreter *interp*.
   This is similar to :c:func:`Py_AtExit`, but takes an explicit interpreter and
   data pointer for the callback.

   There must be an :term:`attached thread state` for *interp*.

   .. versionadded:: 3.13


.. _cautions-regarding-runtime-finalization:

Cautions regarding runtime finalization
---------------------------------------

In the late stage of :term:`interpreter shutdown`, after attempting to wait for
non-daemon threads to exit (though this can be interrupted by
:class:`KeyboardInterrupt`) and running the :mod:`atexit` functions, the runtime
is marked as *finalizing*: :c:func:`Py_IsFinalizing` and
:func:`sys.is_finalizing` return true.  At this point, only the *finalization
thread* that initiated finalization (typically the main thread) is allowed to
acquire the :term:`GIL`.

If any thread, other than the finalization thread, attempts to attach a :term:`thread state`
during finalization, either explicitly or
implicitly, the thread enters **a permanently blocked state**
where it remains until the program exits.  In most cases this is harmless, but this can result
in deadlock if a later stage of finalization attempts to acquire a lock owned by the
blocked thread, or otherwise waits on the blocked thread.

Gross? Yes. This prevents random crashes and/or unexpectedly skipped C++
finalizations further up the call stack when such threads were forcibly exited
here in CPython 3.13 and earlier. The CPython runtime :term:`thread state` C APIs
have never had any error reporting or handling expectations at :term:`thread state`
attachment time that would've allowed for graceful exit from this situation. Changing that
would require new stable C APIs and rewriting the majority of C code in the
CPython ecosystem to use those with error handling.


Process-wide parameters
-----------------------

.. c:function:: void Py_SetProgramName(const wchar_t *name)

   .. index::
      single: Py_Initialize()
      single: main()

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.program_name` should be used instead, see :ref:`Python
   Initialization Configuration <init-config>`.

   This function should be called before :c:func:`Py_Initialize` is called for
   the first time, if it is called at all.  It tells the interpreter the value
   of the ``argv[0]`` argument to the :c:func:`main` function of the program
   (converted to wide characters).
   This is used by some other functions below to find
   the Python run-time libraries relative to the interpreter executable.  The
   default value is ``'python'``.  The argument should point to a
   zero-terminated wide character string in static storage whose contents will not
   change for the duration of the program's execution.  No code in the Python
   interpreter will change the contents of this storage.

   Use :c:func:`Py_DecodeLocale` to decode a bytes string to get a
   :c:expr:`wchar_t*` string.

   .. deprecated-removed:: 3.11 3.15


.. c:function:: const char* Py_GetVersion()

   Return the version of this Python interpreter.  This is a string that looks
   something like ::

      "3.0a5+ (py3k:63103M, May 12 2008, 00:53:55) \n[GCC 4.2.3]"

   .. index:: single: version (in module sys)

   The first word (up to the first space character) is the current Python version;
   the first characters are the major and minor version separated by a
   period.  The returned string points into static storage; the caller should not
   modify its value.  The value is available to Python code as :data:`sys.version`.

   See also the :c:var:`Py_Version` constant.


.. c:function:: const char* Py_GetPlatform()

   .. index:: single: platform (in module sys)

   Return the platform identifier for the current platform.  On Unix, this is
   formed from the "official" name of the operating system, converted to lower
   case, followed by the major revision number; e.g., for Solaris 2.x, which is
   also known as SunOS 5.x, the value is ``'sunos5'``.  On macOS, it is
   ``'darwin'``.  On Windows, it is ``'win'``.  The returned string points into
   static storage; the caller should not modify its value.  The value is available
   to Python code as ``sys.platform``.


.. c:function:: const char* Py_GetCopyright()

   Return the official copyright string for the current Python version, for example

   ``'Copyright 1991-1995 Stichting Mathematisch Centrum, Amsterdam'``

   .. index:: single: copyright (in module sys)

   The returned string points into static storage; the caller should not modify its
   value.  The value is available to Python code as ``sys.copyright``.


.. c:function:: const char* Py_GetCompiler()

   Return an indication of the compiler used to build the current Python version,
   in square brackets, for example::

      "[GCC 2.7.2.2]"

   .. index:: single: version (in module sys)

   The returned string points into static storage; the caller should not modify its
   value.  The value is available to Python code as part of the variable
   ``sys.version``.


.. c:function:: const char* Py_GetBuildInfo()

   Return information about the sequence number and build date and time of the
   current Python interpreter instance, for example ::

      "#67, Aug  1 1997, 22:34:28"

   .. index:: single: version (in module sys)

   The returned string points into static storage; the caller should not modify its
   value.  The value is available to Python code as part of the variable
   ``sys.version``.


.. c:function:: void PySys_SetArgvEx(int argc, wchar_t **argv, int updatepath)

   .. index::
      single: main()
      single: Py_FatalError()
      single: argv (in module sys)

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.argv`, :c:member:`PyConfig.parse_argv` and
   :c:member:`PyConfig.safe_path` should be used instead, see :ref:`Python
   Initialization Configuration <init-config>`.

   Set :data:`sys.argv` based on *argc* and *argv*.  These parameters are
   similar to those passed to the program's :c:func:`main` function with the
   difference that the first entry should refer to the script file to be
   executed rather than the executable hosting the Python interpreter.  If there
   isn't a script that will be run, the first entry in *argv* can be an empty
   string.  If this function fails to initialize :data:`sys.argv`, a fatal
   condition is signalled using :c:func:`Py_FatalError`.

   If *updatepath* is zero, this is all the function does.  If *updatepath*
   is non-zero, the function also modifies :data:`sys.path` according to the
   following algorithm:

   - If the name of an existing script is passed in ``argv[0]``, the absolute
     path of the directory where the script is located is prepended to
     :data:`sys.path`.
   - Otherwise (that is, if *argc* is ``0`` or ``argv[0]`` doesn't point
     to an existing file name), an empty string is prepended to
     :data:`sys.path`, which is the same as prepending the current working
     directory (``"."``).

   Use :c:func:`Py_DecodeLocale` to decode a bytes string to get a
   :c:expr:`wchar_t*` string.

   See also :c:member:`PyConfig.orig_argv` and :c:member:`PyConfig.argv`
   members of the :ref:`Python Initialization Configuration <init-config>`.

   .. note::
      It is recommended that applications embedding the Python interpreter
      for purposes other than executing a single script pass ``0`` as *updatepath*,
      and update :data:`sys.path` themselves if desired.
      See :cve:`2008-5983`.

      On versions before 3.1.3, you can achieve the same effect by manually
      popping the first :data:`sys.path` element after having called
      :c:func:`PySys_SetArgv`, for example using::

         PyRun_SimpleString("import sys; sys.path.pop(0)\n");

   .. versionadded:: 3.1.3

   .. deprecated-removed:: 3.11 3.15


.. c:function:: void PySys_SetArgv(int argc, wchar_t **argv)

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.argv` and :c:member:`PyConfig.parse_argv` should be used
   instead, see :ref:`Python Initialization Configuration <init-config>`.

   This function works like :c:func:`PySys_SetArgvEx` with *updatepath* set
   to ``1`` unless the :program:`python` interpreter was started with the
   :option:`-I`.

   Use :c:func:`Py_DecodeLocale` to decode a bytes string to get a
   :c:expr:`wchar_t*` string.

   See also :c:member:`PyConfig.orig_argv` and :c:member:`PyConfig.argv`
   members of the :ref:`Python Initialization Configuration <init-config>`.

   .. versionchanged:: 3.4 The *updatepath* value depends on :option:`-I`.

   .. deprecated-removed:: 3.11 3.15


.. c:function:: void Py_SetPythonHome(const wchar_t *home)

   This API is kept for backward compatibility: setting
   :c:member:`PyConfig.home` should be used instead, see :ref:`Python
   Initialization Configuration <init-config>`.

   Set the default "home" directory, that is, the location of the standard
   Python libraries.  See :envvar:`PYTHONHOME` for the meaning of the
   argument string.

   The argument should point to a zero-terminated character string in static
   storage whose contents will not change for the duration of the program's
   execution.  No code in the Python interpreter will change the contents of
   this storage.

   Use :c:func:`Py_DecodeLocale` to decode a bytes string to get a
   :c:expr:`wchar_t*` string.

   .. deprecated-removed:: 3.11 3.15
