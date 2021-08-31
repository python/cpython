.. highlight:: c

.. _init-config:

***********************************
Python Initialization Configuration
***********************************

.. versionadded:: 3.8

Structures:

* :c:type:`PyConfig`
* :c:type:`PyPreConfig`
* :c:type:`PyStatus`
* :c:type:`PyWideStringList`

Functions:

* :c:func:`PyConfig_Clear`
* :c:func:`PyConfig_InitIsolatedConfig`
* :c:func:`PyConfig_InitPythonConfig`
* :c:func:`PyConfig_Read`
* :c:func:`PyConfig_SetArgv`
* :c:func:`PyConfig_SetBytesArgv`
* :c:func:`PyConfig_SetBytesString`
* :c:func:`PyConfig_SetString`
* :c:func:`PyConfig_SetWideStringList`
* :c:func:`PyPreConfig_InitIsolatedConfig`
* :c:func:`PyPreConfig_InitPythonConfig`
* :c:func:`PyStatus_Error`
* :c:func:`PyStatus_Exception`
* :c:func:`PyStatus_Exit`
* :c:func:`PyStatus_IsError`
* :c:func:`PyStatus_IsExit`
* :c:func:`PyStatus_NoMemory`
* :c:func:`PyStatus_Ok`
* :c:func:`PyWideStringList_Append`
* :c:func:`PyWideStringList_Insert`
* :c:func:`Py_ExitStatusException`
* :c:func:`Py_InitializeFromConfig`
* :c:func:`Py_PreInitialize`
* :c:func:`Py_PreInitializeFromArgs`
* :c:func:`Py_PreInitializeFromBytesArgs`
* :c:func:`Py_RunMain`
* :c:func:`Py_GetArgcArgv`

The preconfiguration (``PyPreConfig`` type) is stored in
``_PyRuntime.preconfig`` and the configuration (``PyConfig`` type) is stored in
``PyInterpreterState.config``.

See also :ref:`Initialization, Finalization, and Threads <initialization>`.

.. seealso::
   :pep:`587` "Python Initialization Configuration".


PyWideStringList
----------------

.. c:type:: PyWideStringList

   List of ``wchar_t*`` strings.

   If *length* is non-zero, *items* must be non-``NULL`` and all strings must be
   non-``NULL``.

   Methods:

   .. c:function:: PyStatus PyWideStringList_Append(PyWideStringList *list, const wchar_t *item)

      Append *item* to *list*.

      Python must be preinitialized to call this function.

   .. c:function:: PyStatus PyWideStringList_Insert(PyWideStringList *list, Py_ssize_t index, const wchar_t *item)

      Insert *item* into *list* at *index*.

      If *index* is greater than or equal to *list* length, append *item* to
      *list*.

      *index* must be greater than or equal to 0.

      Python must be preinitialized to call this function.

   Structure fields:

   .. c:member:: Py_ssize_t length

      List length.

   .. c:member:: wchar_t** items

      List items.

PyStatus
--------

.. c:type:: PyStatus

   Structure to store an initialization function status: success, error
   or exit.

   For an error, it can store the C function name which created the error.

   Structure fields:

   .. c:member:: int exitcode

      Exit code. Argument passed to ``exit()``.

   .. c:member:: const char *err_msg

      Error message.

   .. c:member:: const char *func

      Name of the function which created an error, can be ``NULL``.

   Functions to create a status:

   .. c:function:: PyStatus PyStatus_Ok(void)

      Success.

   .. c:function:: PyStatus PyStatus_Error(const char *err_msg)

      Initialization error with a message.

   .. c:function:: PyStatus PyStatus_NoMemory(void)

      Memory allocation failure (out of memory).

   .. c:function:: PyStatus PyStatus_Exit(int exitcode)

      Exit Python with the specified exit code.

   Functions to handle a status:

   .. c:function:: int PyStatus_Exception(PyStatus status)

      Is the status an error or an exit? If true, the exception must be
      handled; by calling :c:func:`Py_ExitStatusException` for example.

   .. c:function:: int PyStatus_IsError(PyStatus status)

      Is the result an error?

   .. c:function:: int PyStatus_IsExit(PyStatus status)

      Is the result an exit?

   .. c:function:: void Py_ExitStatusException(PyStatus status)

      Call ``exit(exitcode)`` if *status* is an exit. Print the error
      message and exit with a non-zero exit code if *status* is an error.  Must
      only be called if ``PyStatus_Exception(status)`` is non-zero.

.. note::
   Internally, Python uses macros which set ``PyStatus.func``,
   whereas functions to create a status set ``func`` to ``NULL``.

Example::

    PyStatus alloc(void **ptr, size_t size)
    {
        *ptr = PyMem_RawMalloc(size);
        if (*ptr == NULL) {
            return PyStatus_NoMemory();
        }
        return PyStatus_Ok();
    }

    int main(int argc, char **argv)
    {
        void *ptr;
        PyStatus status = alloc(&ptr, 16);
        if (PyStatus_Exception(status)) {
            Py_ExitStatusException(status);
        }
        PyMem_Free(ptr);
        return 0;
    }


PyPreConfig
-----------

.. c:type:: PyPreConfig

   Structure used to preinitialize Python:

   * Set the Python memory allocator
   * Configure the LC_CTYPE locale
   * Set the UTF-8 mode

   Function to initialize a preconfiguration:

   .. c:function:: void PyPreConfig_InitPythonConfig(PyPreConfig *preconfig)

      Initialize the preconfiguration with :ref:`Python Configuration
      <init-python-config>`.

   .. c:function:: void PyPreConfig_InitIsolatedConfig(PyPreConfig *preconfig)

      Initialize the preconfiguration with :ref:`Isolated Configuration
      <init-isolated-conf>`.

   Structure fields:

   .. c:member:: int allocator

      Name of the memory allocator:

      * ``PYMEM_ALLOCATOR_NOT_SET`` (``0``): don't change memory allocators
        (use defaults)
      * ``PYMEM_ALLOCATOR_DEFAULT`` (``1``): default memory allocators
      * ``PYMEM_ALLOCATOR_DEBUG`` (``2``): default memory allocators with
        debug hooks
      * ``PYMEM_ALLOCATOR_MALLOC`` (``3``): force usage of ``malloc()``
      * ``PYMEM_ALLOCATOR_MALLOC_DEBUG`` (``4``): force usage of
        ``malloc()`` with debug hooks
      * ``PYMEM_ALLOCATOR_PYMALLOC`` (``5``): :ref:`Python pymalloc memory
        allocator <pymalloc>`
      * ``PYMEM_ALLOCATOR_PYMALLOC_DEBUG`` (``6``): :ref:`Python pymalloc
        memory allocator <pymalloc>` with debug hooks

      ``PYMEM_ALLOCATOR_PYMALLOC`` and ``PYMEM_ALLOCATOR_PYMALLOC_DEBUG``
      are not supported if Python is configured using ``--without-pymalloc``

      See :ref:`Memory Management <memory>`.

   .. c:member:: int configure_locale

      Set the LC_CTYPE locale to the user preferred locale? If equals to 0, set
      :c:member:`coerce_c_locale` and :c:member:`coerce_c_locale_warn` to 0.

   .. c:member:: int coerce_c_locale

      If equals to 2, coerce the C locale; if equals to 1, read the LC_CTYPE
      locale to decide if it should be coerced.

   .. c:member:: int coerce_c_locale_warn

      If non-zero, emit a warning if the C locale is coerced.

   .. c:member:: int dev_mode

      See :c:member:`PyConfig.dev_mode`.

   .. c:member:: int isolated

      See :c:member:`PyConfig.isolated`.

   .. c:member:: int legacy_windows_fs_encoding (Windows only)

      If non-zero, disable UTF-8 Mode, set the Python filesystem encoding to
      ``mbcs``, set the filesystem error handler to ``replace``.

      Only available on Windows. ``#ifdef MS_WINDOWS`` macro can be used for
      Windows specific code.

   .. c:member:: int parse_argv

      If non-zero, :c:func:`Py_PreInitializeFromArgs` and
      :c:func:`Py_PreInitializeFromBytesArgs` parse their ``argv`` argument the
      same way the regular Python parses command line arguments: see
      :ref:`Command Line Arguments <using-on-cmdline>`.

   .. c:member:: int use_environment

      See :c:member:`PyConfig.use_environment`.

   .. c:member:: int utf8_mode

      If non-zero, enable the UTF-8 mode.

Preinitialization with PyPreConfig
----------------------------------

Functions to preinitialize Python:

.. c:function:: PyStatus Py_PreInitialize(const PyPreConfig *preconfig)

   Preinitialize Python from *preconfig* preconfiguration.

.. c:function:: PyStatus Py_PreInitializeFromBytesArgs(const PyPreConfig *preconfig, int argc, char * const *argv)

   Preinitialize Python from *preconfig* preconfiguration and command line
   arguments (bytes strings).

.. c:function:: PyStatus Py_PreInitializeFromArgs(const PyPreConfig *preconfig, int argc, wchar_t * const * argv)

   Preinitialize Python from *preconfig* preconfiguration and command line
   arguments (wide strings).

The caller is responsible to handle exceptions (error or exit) using
:c:func:`PyStatus_Exception` and :c:func:`Py_ExitStatusException`.

For :ref:`Python Configuration <init-python-config>`
(:c:func:`PyPreConfig_InitPythonConfig`), if Python is initialized with
command line arguments, the command line arguments must also be passed to
preinitialize Python, since they have an effect on the pre-configuration
like encodings. For example, the :option:`-X utf8 <-X>` command line option
enables the UTF-8 Mode.

``PyMem_SetAllocator()`` can be called after :c:func:`Py_PreInitialize` and
before :c:func:`Py_InitializeFromConfig` to install a custom memory allocator.
It can be called before :c:func:`Py_PreInitialize` if
:c:member:`PyPreConfig.allocator` is set to ``PYMEM_ALLOCATOR_NOT_SET``.

Python memory allocation functions like :c:func:`PyMem_RawMalloc` must not be
used before Python preinitialization, whereas calling directly ``malloc()`` and
``free()`` is always safe. :c:func:`Py_DecodeLocale` must not be called before
the preinitialization.

Example using the preinitialization to enable the UTF-8 Mode::

    PyStatus status;
    PyPreConfig preconfig;
    PyPreConfig_InitPythonConfig(&preconfig);

    preconfig.utf8_mode = 1;

    status = Py_PreInitialize(&preconfig);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    /* at this point, Python will speak UTF-8 */

    Py_Initialize();
    /* ... use Python API here ... */
    Py_Finalize();


PyConfig
--------

.. c:type:: PyConfig

   Structure containing most parameters to configure Python.

   Structure methods:

   .. c:function:: void PyConfig_InitPythonConfig(PyConfig *config)

      Initialize configuration with :ref:`Python Configuration
      <init-python-config>`.

   .. c:function:: void PyConfig_InitIsolatedConfig(PyConfig *config)

      Initialize configuration with :ref:`Isolated Configuration
      <init-isolated-conf>`.

   .. c:function:: PyStatus PyConfig_SetString(PyConfig *config, wchar_t * const *config_str, const wchar_t *str)

      Copy the wide character string *str* into ``*config_str``.

      Preinitialize Python if needed.

   .. c:function:: PyStatus PyConfig_SetBytesString(PyConfig *config, wchar_t * const *config_str, const char *str)

      Decode *str* using ``Py_DecodeLocale()`` and set the result into ``*config_str``.

      Preinitialize Python if needed.

   .. c:function:: PyStatus PyConfig_SetArgv(PyConfig *config, int argc, wchar_t * const *argv)

      Set command line arguments from wide character strings.

      Preinitialize Python if needed.

   .. c:function:: PyStatus PyConfig_SetBytesArgv(PyConfig *config, int argc, char * const *argv)

      Set command line arguments: decode bytes using :c:func:`Py_DecodeLocale`.

      Preinitialize Python if needed.

   .. c:function:: PyStatus PyConfig_SetWideStringList(PyConfig *config, PyWideStringList *list, Py_ssize_t length, wchar_t **items)

      Set the list of wide strings *list* to *length* and *items*.

      Preinitialize Python if needed.

   .. c:function:: PyStatus PyConfig_Read(PyConfig *config)

      Read all Python configuration.

      Fields which are already initialized are left unchanged.

      Preinitialize Python if needed.

   .. c:function:: void PyConfig_Clear(PyConfig *config)

      Release configuration memory.

   Most ``PyConfig`` methods preinitialize Python if needed. In that case, the
   Python preinitialization configuration in based on the :c:type:`PyConfig`.
   If configuration fields which are in common with :c:type:`PyPreConfig` are
   tuned, they must be set before calling a :c:type:`PyConfig` method:

   * :c:member:`~PyConfig.dev_mode`
   * :c:member:`~PyConfig.isolated`
   * :c:member:`~PyConfig.parse_argv`
   * :c:member:`~PyConfig.use_environment`

   Moreover, if :c:func:`PyConfig_SetArgv` or :c:func:`PyConfig_SetBytesArgv`
   is used, this method must be called first, before other methods, since the
   preinitialization configuration depends on command line arguments (if
   :c:member:`parse_argv` is non-zero).

   The caller of these methods is responsible to handle exceptions (error or
   exit) using ``PyStatus_Exception()`` and ``Py_ExitStatusException()``.

   Structure fields:

   .. c:member:: PyWideStringList argv

      Command line arguments, :data:`sys.argv`. See
      :c:member:`~PyConfig.parse_argv` to parse :c:member:`~PyConfig.argv` the
      same way the regular Python parses Python command line arguments. If
      :c:member:`~PyConfig.argv` is empty, an empty string is added to ensure
      that :data:`sys.argv` always exists and is never empty.

   .. c:member:: wchar_t* base_exec_prefix

      :data:`sys.base_exec_prefix`.

   .. c:member:: wchar_t* base_executable

      :data:`sys._base_executable`: ``__PYVENV_LAUNCHER__`` environment
      variable value, or copy of :c:member:`PyConfig.executable`.

   .. c:member:: wchar_t* base_prefix

      :data:`sys.base_prefix`.

   .. c:member:: wchar_t* platlibdir

      :data:`sys.platlibdir`: platform library directory name, set at configure time
      by ``--with-platlibdir``, overrideable by the ``PYTHONPLATLIBDIR``
      environment variable.

      .. versionadded:: 3.9

   .. c:member:: int buffered_stdio

      If equals to 0, enable unbuffered mode, making the stdout and stderr
      streams unbuffered.

      stdin is always opened in buffered mode.

   .. c:member:: int bytes_warning

      If equals to 1, issue a warning when comparing :class:`bytes` or
      :class:`bytearray` with :class:`str`, or comparing :class:`bytes` with
      :class:`int`. If equal or greater to 2, raise a :exc:`BytesWarning`
      exception.

   .. c:member:: wchar_t* check_hash_pycs_mode

      Control the validation behavior of hash-based ``.pyc`` files (see
      :pep:`552`): :option:`--check-hash-based-pycs` command line option value.

      Valid values: ``always``, ``never`` and ``default``.

      The default value is: ``default``.

   .. c:member:: int configure_c_stdio

      If non-zero, configure C standard streams (``stdio``, ``stdout``,
      ``stdout``). For example, set their mode to ``O_BINARY`` on Windows.

   .. c:member:: int dev_mode

      If non-zero, enable the :ref:`Python Development Mode <devmode>`.

   .. c:member:: int dump_refs

      If non-zero, dump all objects which are still alive at exit.

      ``Py_TRACE_REFS`` macro must be defined in build.

   .. c:member:: wchar_t* exec_prefix

      :data:`sys.exec_prefix`.

   .. c:member:: wchar_t* executable

      :data:`sys.executable`.

   .. c:member:: int faulthandler

      If non-zero, call :func:`faulthandler.enable` at startup.

   .. c:member:: wchar_t* filesystem_encoding

      Filesystem encoding, :func:`sys.getfilesystemencoding`.

   .. c:member:: wchar_t* filesystem_errors

      Filesystem encoding errors, :func:`sys.getfilesystemencodeerrors`.

   .. c:member:: unsigned long hash_seed
   .. c:member:: int use_hash_seed

      Randomized hash function seed.

      If :c:member:`~PyConfig.use_hash_seed` is zero, a seed is chosen randomly
      at Pythonstartup, and :c:member:`~PyConfig.hash_seed` is ignored.

   .. c:member:: wchar_t* home

      Python home directory.

      Initialized from :envvar:`PYTHONHOME` environment variable value by
      default.

   .. c:member:: int import_time

      If non-zero, profile import time.

   .. c:member:: int inspect

      Enter interactive mode after executing a script or a command.

   .. c:member:: int install_signal_handlers

      Install signal handlers?

   .. c:member:: int interactive

      Interactive mode.

   .. c:member:: int isolated

      If greater than 0, enable isolated mode:

      * :data:`sys.path` contains neither the script's directory (computed from
        ``argv[0]`` or the current directory) nor the user's site-packages
        directory.
      * Python REPL doesn't import :mod:`readline` nor enable default readline
        configuration on interactive prompts.
      * Set :c:member:`~PyConfig.use_environment` and
        :c:member:`~PyConfig.user_site_directory` to 0.

   .. c:member:: int legacy_windows_stdio

      If non-zero, use :class:`io.FileIO` instead of
      :class:`io.WindowsConsoleIO` for :data:`sys.stdin`, :data:`sys.stdout`
      and :data:`sys.stderr`.

      Only available on Windows. ``#ifdef MS_WINDOWS`` macro can be used for
      Windows specific code.

   .. c:member:: int malloc_stats

      If non-zero, dump statistics on :ref:`Python pymalloc memory allocator
      <pymalloc>` at exit.

      The option is ignored if Python is built using ``--without-pymalloc``.

   .. c:member:: wchar_t* pythonpath_env

      Module search paths as a string separated by ``DELIM``
      (:data:`os.path.pathsep`).

      Initialized from :envvar:`PYTHONPATH` environment variable value by
      default.

   .. c:member:: PyWideStringList module_search_paths
   .. c:member:: int module_search_paths_set

      :data:`sys.path`. If :c:member:`~PyConfig.module_search_paths_set` is
      equal to 0, the :c:member:`~PyConfig.module_search_paths` is overridden
      by the function calculating the :ref:`Path Configuration
      <init-path-config>`.

   .. c:member:: int optimization_level

      Compilation optimization level:

      * 0: Peephole optimizer (and ``__debug__`` is set to ``True``)
      * 1: Remove assertions, set ``__debug__`` to ``False``
      * 2: Strip docstrings

   .. c:member:: int parse_argv

      If non-zero, parse :c:member:`~PyConfig.argv` the same way the regular
      Python command line arguments, and strip Python arguments from
      :c:member:`~PyConfig.argv`: see :ref:`Command Line Arguments
      <using-on-cmdline>`.

   .. c:member:: int parser_debug

      If non-zero, turn on parser debugging output (for expert only, depending
      on compilation options).

   .. c:member:: int pathconfig_warnings

      If equal to 0, suppress warnings when calculating the :ref:`Path
      Configuration <init-path-config>` (Unix only, Windows does not log any
      warning). Otherwise, warnings are written into ``stderr``.

   .. c:member:: wchar_t* prefix

      :data:`sys.prefix`.

   .. c:member:: wchar_t* program_name

      Program name. Used to initialize :c:member:`~PyConfig.executable`, and in
      early error messages.

   .. c:member:: wchar_t* pycache_prefix

      :data:`sys.pycache_prefix`: ``.pyc`` cache prefix.

      If ``NULL``, :data:`sys.pycache_prefix` is set to ``None``.

   .. c:member:: int quiet

      Quiet mode. For example, don't display the copyright and version messages
      in interactive mode.

   .. c:member:: wchar_t* run_command

      ``python3 -c COMMAND`` argument. Used by :c:func:`Py_RunMain`.

   .. c:member:: wchar_t* run_filename

      ``python3 FILENAME`` argument. Used by :c:func:`Py_RunMain`.

   .. c:member:: wchar_t* run_module

      ``python3 -m MODULE`` argument. Used by :c:func:`Py_RunMain`.

   .. c:member:: int show_ref_count

      Show total reference count at exit?

      Set to 1 by :option:`-X showrefcount <-X>` command line option.

      Need a debug build of Python (``Py_REF_DEBUG`` macro must be defined).

   .. c:member:: int site_import

      Import the :mod:`site` module at startup?

   .. c:member:: int skip_source_first_line

      Skip the first line of the source?

   .. c:member:: wchar_t* stdio_encoding
   .. c:member:: wchar_t* stdio_errors

      Encoding and encoding errors of :data:`sys.stdin`, :data:`sys.stdout` and
      :data:`sys.stderr`.

   .. c:member:: int tracemalloc

      If non-zero, call :func:`tracemalloc.start` at startup.

   .. c:member:: int use_environment

      If greater than 0, use :ref:`environment variables <using-on-envvars>`.

   .. c:member:: int user_site_directory

      If non-zero, add user site directory to :data:`sys.path`.

   .. c:member:: int verbose

      If non-zero, enable verbose mode.

   .. c:member:: PyWideStringList warnoptions

      :data:`sys.warnoptions`: options of the :mod:`warnings` module to build
      warnings filters: lowest to highest priority.

      The :mod:`warnings` module adds :data:`sys.warnoptions` in the reverse
      order: the last :c:member:`PyConfig.warnoptions` item becomes the first
      item of :data:`warnings.filters` which is checked first (highest
      priority).

   .. c:member:: int write_bytecode

      If non-zero, write ``.pyc`` files.

      :data:`sys.dont_write_bytecode` is initialized to the inverted value of
      :c:member:`~PyConfig.write_bytecode`.

   .. c:member:: PyWideStringList xoptions

      :data:`sys._xoptions`.

   .. c:member:: int _use_peg_parser

      Enable PEG parser? Default: 1.

      Set to 0 by :option:`-X oldparser <-X>` and :envvar:`PYTHONOLDPARSER`.

      See also :pep:`617`.

      .. deprecated-removed:: 3.9 3.10

If ``parse_argv`` is non-zero, ``argv`` arguments are parsed the same
way the regular Python parses command line arguments, and Python
arguments are stripped from ``argv``: see :ref:`Command Line Arguments
<using-on-cmdline>`.

The ``xoptions`` options are parsed to set other options: see :option:`-X`
option.

.. versionchanged:: 3.9

   The ``show_alloc_count`` field has been removed.


Initialization with PyConfig
----------------------------

Function to initialize Python:

.. c:function:: PyStatus Py_InitializeFromConfig(const PyConfig *config)

   Initialize Python from *config* configuration.

The caller is responsible to handle exceptions (error or exit) using
:c:func:`PyStatus_Exception` and :c:func:`Py_ExitStatusException`.

If :c:func:`PyImport_FrozenModules`, :c:func:`PyImport_AppendInittab` or
:c:func:`PyImport_ExtendInittab` are used, they must be set or called after
Python preinitialization and before the Python initialization. If Python is
initialized multiple times, :c:func:`PyImport_AppendInittab` or
:c:func:`PyImport_ExtendInittab` must be called before each Python
initialization.

Example setting the program name::

    void init_python(void)
    {
        PyStatus status;

        PyConfig config;
        PyConfig_InitPythonConfig(&config);

        /* Set the program name. Implicitly preinitialize Python. */
        status = PyConfig_SetString(&config, &config.program_name,
                                    L"/path/to/my_program");
        if (PyStatus_Exception(status)) {
            goto fail;
        }

        status = Py_InitializeFromConfig(&config);
        if (PyStatus_Exception(status)) {
            goto fail;
        }
        PyConfig_Clear(&config);
        return;

    fail:
        PyConfig_Clear(&config);
        Py_ExitStatusException(status);
    }

More complete example modifying the default configuration, read the
configuration, and then override some parameters::

    PyStatus init_python(const char *program_name)
    {
        PyStatus status;

        PyConfig config;
        PyConfig_InitPythonConfig(&config);

        /* Set the program name before reading the configuration
           (decode byte string from the locale encoding).

           Implicitly preinitialize Python. */
        status = PyConfig_SetBytesString(&config, &config.program_name,
                                      program_name);
        if (PyStatus_Exception(status)) {
            goto done;
        }

        /* Read all configuration at once */
        status = PyConfig_Read(&config);
        if (PyStatus_Exception(status)) {
            goto done;
        }

        /* Append our custom search path to sys.path */
        status = PyWideStringList_Append(&config.module_search_paths,
                                         L"/path/to/more/modules");
        if (PyStatus_Exception(status)) {
            goto done;
        }

        /* Override executable computed by PyConfig_Read() */
        status = PyConfig_SetString(&config, &config.executable,
                                    L"/path/to/my_executable");
        if (PyStatus_Exception(status)) {
            goto done;
        }

        status = Py_InitializeFromConfig(&config);

    done:
        PyConfig_Clear(&config);
        return status;
    }


.. _init-isolated-conf:

Isolated Configuration
----------------------

:c:func:`PyPreConfig_InitIsolatedConfig` and
:c:func:`PyConfig_InitIsolatedConfig` functions create a configuration to
isolate Python from the system. For example, to embed Python into an
application.

This configuration ignores global configuration variables, environments
variables, command line arguments (:c:member:`PyConfig.argv` is not parsed)
and user site directory. The C standard streams (ex: ``stdout``) and the
LC_CTYPE locale are left unchanged. Signal handlers are not installed.

Configuration files are still used with this configuration. Set the
:ref:`Path Configuration <init-path-config>` ("output fields") to ignore these
configuration files and avoid the function computing the default path
configuration.


.. _init-python-config:

Python Configuration
--------------------

:c:func:`PyPreConfig_InitPythonConfig` and :c:func:`PyConfig_InitPythonConfig`
functions create a configuration to build a customized Python which behaves as
the regular Python.

Environments variables and command line arguments are used to configure
Python, whereas global configuration variables are ignored.

This function enables C locale coercion (:pep:`538`) and UTF-8 Mode
(:pep:`540`) depending on the LC_CTYPE locale, :envvar:`PYTHONUTF8` and
:envvar:`PYTHONCOERCECLOCALE` environment variables.

Example of customized Python always running in isolated mode::

    int main(int argc, char **argv)
    {
        PyStatus status;

        PyConfig config;
        PyConfig_InitPythonConfig(&config);
        config.isolated = 1;

        /* Decode command line arguments.
           Implicitly preinitialize Python (in isolated mode). */
        status = PyConfig_SetBytesArgv(&config, argc, argv);
        if (PyStatus_Exception(status)) {
            goto fail;
        }

        status = Py_InitializeFromConfig(&config);
        if (PyStatus_Exception(status)) {
            goto fail;
        }
        PyConfig_Clear(&config);

        return Py_RunMain();

    fail:
        PyConfig_Clear(&config);
        if (PyStatus_IsExit(status)) {
            return status.exitcode;
        }
        /* Display the error message and exit the process with
           non-zero exit code */
        Py_ExitStatusException(status);
    }


.. _init-path-config:

Path Configuration
------------------

:c:type:`PyConfig` contains multiple fields for the path configuration:

* Path configuration inputs:

  * :c:member:`PyConfig.home`
  * :c:member:`PyConfig.platlibdir`
  * :c:member:`PyConfig.pathconfig_warnings`
  * :c:member:`PyConfig.program_name`
  * :c:member:`PyConfig.pythonpath_env`
  * current working directory: to get absolute paths
  * ``PATH`` environment variable to get the program full path
    (from :c:member:`PyConfig.program_name`)
  * ``__PYVENV_LAUNCHER__`` environment variable
  * (Windows only) Application paths in the registry under
    "Software\Python\PythonCore\X.Y\PythonPath" of HKEY_CURRENT_USER and
    HKEY_LOCAL_MACHINE (where X.Y is the Python version).

* Path configuration output fields:

  * :c:member:`PyConfig.base_exec_prefix`
  * :c:member:`PyConfig.base_executable`
  * :c:member:`PyConfig.base_prefix`
  * :c:member:`PyConfig.exec_prefix`
  * :c:member:`PyConfig.executable`
  * :c:member:`PyConfig.module_search_paths_set`,
    :c:member:`PyConfig.module_search_paths`
  * :c:member:`PyConfig.prefix`

If at least one "output field" is not set, Python calculates the path
configuration to fill unset fields. If
:c:member:`~PyConfig.module_search_paths_set` is equal to 0,
:c:member:`~PyConfig.module_search_paths` is overridden and
:c:member:`~PyConfig.module_search_paths_set` is set to 1.

It is possible to completely ignore the function calculating the default
path configuration by setting explicitly all path configuration output
fields listed above. A string is considered as set even if it is non-empty.
``module_search_paths`` is considered as set if
``module_search_paths_set`` is set to 1. In this case, path
configuration input fields are ignored as well.

Set :c:member:`~PyConfig.pathconfig_warnings` to 0 to suppress warnings when
calculating the path configuration (Unix only, Windows does not log any warning).

If :c:member:`~PyConfig.base_prefix` or :c:member:`~PyConfig.base_exec_prefix`
fields are not set, they inherit their value from :c:member:`~PyConfig.prefix`
and :c:member:`~PyConfig.exec_prefix` respectively.

:c:func:`Py_RunMain` and :c:func:`Py_Main` modify :data:`sys.path`:

* If :c:member:`~PyConfig.run_filename` is set and is a directory which contains a
  ``__main__.py`` script, prepend :c:member:`~PyConfig.run_filename` to
  :data:`sys.path`.
* If :c:member:`~PyConfig.isolated` is zero:

  * If :c:member:`~PyConfig.run_module` is set, prepend the current directory
    to :data:`sys.path`. Do nothing if the current directory cannot be read.
  * If :c:member:`~PyConfig.run_filename` is set, prepend the directory of the
    filename to :data:`sys.path`.
  * Otherwise, prepend an empty string to :data:`sys.path`.

If :c:member:`~PyConfig.site_import` is non-zero, :data:`sys.path` can be
modified by the :mod:`site` module. If
:c:member:`~PyConfig.user_site_directory` is non-zero and the user's
site-package directory exists, the :mod:`site` module appends the user's
site-package directory to :data:`sys.path`.

The following configuration files are used by the path configuration:

* ``pyvenv.cfg``
* ``python._pth`` (Windows only)
* ``pybuilddir.txt`` (Unix only)

The ``__PYVENV_LAUNCHER__`` environment variable is used to set
:c:member:`PyConfig.base_executable`


Py_RunMain()
------------

.. c:function:: int Py_RunMain(void)

   Execute the command (:c:member:`PyConfig.run_command`), the script
   (:c:member:`PyConfig.run_filename`) or the module
   (:c:member:`PyConfig.run_module`) specified on the command line or in the
   configuration.

   By default and when if :option:`-i` option is used, run the REPL.

   Finally, finalizes Python and returns an exit status that can be passed to
   the ``exit()`` function.

See :ref:`Python Configuration <init-python-config>` for an example of
customized Python always running in isolated mode using
:c:func:`Py_RunMain`.


Py_GetArgcArgv()
----------------

.. c:function:: void Py_GetArgcArgv(int *argc, wchar_t ***argv)

   Get the original command line arguments, before Python modified them.


Multi-Phase Initialization Private Provisional API
--------------------------------------------------

This section is a private provisional API introducing multi-phase
initialization, the core feature of the :pep:`432`:

* "Core" initialization phase, "bare minimum Python":

  * Builtin types;
  * Builtin exceptions;
  * Builtin and frozen modules;
  * The :mod:`sys` module is only partially initialized
    (ex: :data:`sys.path` doesn't exist yet).

* "Main" initialization phase, Python is fully initialized:

  * Install and configure :mod:`importlib`;
  * Apply the :ref:`Path Configuration <init-path-config>`;
  * Install signal handlers;
  * Finish :mod:`sys` module initialization (ex: create :data:`sys.stdout`
    and :data:`sys.path`);
  * Enable optional features like :mod:`faulthandler` and :mod:`tracemalloc`;
  * Import the :mod:`site` module;
  * etc.

Private provisional API:

* :c:member:`PyConfig._init_main`: if set to 0,
  :c:func:`Py_InitializeFromConfig` stops at the "Core" initialization phase.
* :c:member:`PyConfig._isolated_interpreter`: if non-zero,
  disallow threads, subprocesses and fork.

.. c:function:: PyStatus _Py_InitializeMain(void)

   Move to the "Main" initialization phase, finish the Python initialization.

No module is imported during the "Core" phase and the ``importlib`` module is
not configured: the :ref:`Path Configuration <init-path-config>` is only
applied during the "Main" phase. It may allow to customize Python in Python to
override or tune the :ref:`Path Configuration <init-path-config>`, maybe
install a custom :data:`sys.meta_path` importer or an import hook, etc.

It may become possible to calculatin the :ref:`Path Configuration
<init-path-config>` in Python, after the Core phase and before the Main phase,
which is one of the :pep:`432` motivation.

The "Core" phase is not properly defined: what should be and what should
not be available at this phase is not specified yet. The API is marked
as private and provisional: the API can be modified or even be removed
anytime until a proper public API is designed.

Example running Python code between "Core" and "Main" initialization
phases::

    void init_python(void)
    {
        PyStatus status;

        PyConfig config;
        PyConfig_InitPythonConfig(&config);
        config._init_main = 0;

        /* ... customize 'config' configuration ... */

        status = Py_InitializeFromConfig(&config);
        PyConfig_Clear(&config);
        if (PyStatus_Exception(status)) {
            Py_ExitStatusException(status);
        }

        /* Use sys.stderr because sys.stdout is only created
           by _Py_InitializeMain() */
        int res = PyRun_SimpleString(
            "import sys; "
            "print('Run Python code before _Py_InitializeMain', "
                   "file=sys.stderr)");
        if (res < 0) {
            exit(1);
        }

        /* ... put more configuration code here ... */

        status = _Py_InitializeMain();
        if (PyStatus_Exception(status)) {
            Py_ExitStatusException(status);
        }
    }
