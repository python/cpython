.. highlight:: c

.. _init-config:

***********************************
Python Initialization Configuration
***********************************


.. _pyinitconfig_api:

PyInitConfig C API
==================

.. versionadded:: 3.14

Python can be initialized with :c:func:`Py_InitializeFromInitConfig`.

The :c:func:`Py_RunMain` function can be used to write a customized Python
program.

See also :ref:`Initialization, Finalization, and Threads <initialization>`.

.. seealso::
   :pep:`741` "Python Configuration C API".


Example
-------

Example of customized Python always running with the :ref:`Python Development
Mode <devmode>` enabled; return ``-1`` on error:

.. code-block:: c

    int init_python(void)
    {
        PyInitConfig *config = PyInitConfig_Create();
        if (config == NULL) {
            printf("PYTHON INIT ERROR: memory allocation failed\n");
            return -1;
        }

        // Enable the Python Development Mode
        if (PyInitConfig_SetInt(config, "dev_mode", 1) < 0) {
            goto error;
        }

        // Initialize Python with the configuration
        if (Py_InitializeFromInitConfig(config) < 0) {
            goto error;
        }
        PyInitConfig_Free(config);
        return 0;

    error:
        {
            // Display the error message.
            //
            // This uncommon braces style is used, because you cannot make
            // goto targets point to variable declarations.
            const char *err_msg;
            (void)PyInitConfig_GetError(config, &err_msg);
            printf("PYTHON INIT ERROR: %s\n", err_msg);
            PyInitConfig_Free(config);
            return -1;
        }
    }

Create Config
-------------

.. c:struct:: PyInitConfig

   Opaque structure to configure the Python initialization.


.. c:function:: PyInitConfig* PyInitConfig_Create(void)

   Create a new initialization configuration using :ref:`Isolated Configuration
   <init-isolated-conf>` default values.

   It must be freed by :c:func:`PyInitConfig_Free`.

   Return ``NULL`` on memory allocation failure.


.. c:function:: void PyInitConfig_Free(PyInitConfig *config)

   Free memory of the initialization configuration *config*.

   If *config* is ``NULL``, no operation is performed.


Error Handling
--------------

.. c:function:: int PyInitConfig_GetError(PyInitConfig* config, const char **err_msg)

   Get the *config* error message.

   * Set *\*err_msg* and return ``1`` if an error is set.
   * Set *\*err_msg* to ``NULL`` and return ``0`` otherwise.

   An error message is an UTF-8 encoded string.

   If *config* has an exit code, format the exit code as an error
   message.

   The error message remains valid until another ``PyInitConfig``
   function is called with *config*. The caller doesn't have to free the
   error message.


.. c:function:: int PyInitConfig_GetExitCode(PyInitConfig* config, int *exitcode)

   Get the *config* exit code.

   * Set *\*exitcode* and return ``1`` if *config* has an exit code set.
   * Return ``0`` if *config* has no exit code set.

   Only the ``Py_InitializeFromInitConfig()`` function can set an exit
   code if the ``parse_argv`` option is non-zero.

   An exit code can be set when parsing the command line failed (exit
   code ``2``) or when a command line option asks to display the command
   line help (exit code ``0``).


Get Options
-----------

The configuration option *name* parameter must be a non-NULL null-terminated
UTF-8 encoded string. See :ref:`Configuration Options <pyinitconfig-opts>`.

.. c:function:: int PyInitConfig_HasOption(PyInitConfig *config, const char *name)

   Test if the configuration has an option called *name*.

   Return ``1`` if the option exists, or return ``0`` otherwise.


.. c:function:: int PyInitConfig_GetInt(PyInitConfig *config, const char *name, int64_t *value)

   Get an integer configuration option.

   * Set *\*value*, and return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


.. c:function:: int PyInitConfig_GetStr(PyInitConfig *config, const char *name, char **value)

   Get a string configuration option as a null-terminated UTF-8
   encoded string.

   * Set *\*value*, and return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.

   *\*value* can be set to ``NULL`` if the option is an optional string and the
   option is unset.

   On success, the string must be released with ``free(value)`` if it's not
   ``NULL``.


.. c:function:: int PyInitConfig_GetStrList(PyInitConfig *config, const char *name, size_t *length, char ***items)

   Get a string list configuration option as an array of
   null-terminated UTF-8 encoded strings.

   * Set *\*length* and *\*value*, and return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.

   On success, the string list must be released with
   ``PyInitConfig_FreeStrList(length, items)``.


.. c:function:: void PyInitConfig_FreeStrList(size_t length, char **items)

   Free memory of a string list created by
   ``PyInitConfig_GetStrList()``.


Set Options
-----------

The configuration option *name* parameter must be a non-NULL null-terminated
UTF-8 encoded string. See :ref:`Configuration Options <pyinitconfig-opts>`.

Some configuration options have side effects on other options. This logic is
only implemented when ``Py_InitializeFromInitConfig()`` is called, not by the
"Set" functions below. For example, setting ``dev_mode`` to ``1`` does not set
``faulthandler`` to ``1``.

.. c:function:: int PyInitConfig_SetInt(PyInitConfig *config, const char *name, int64_t value)

   Set an integer configuration option.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


.. c:function:: int PyInitConfig_SetStr(PyInitConfig *config, const char *name, const char *value)

   Set a string configuration option from a null-terminated UTF-8
   encoded string. The string is copied.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


.. c:function:: int PyInitConfig_SetStrList(PyInitConfig *config, const char *name, size_t length, char * const *items)

   Set a string list configuration option from an array of
   null-terminated UTF-8 encoded strings. The string list is copied.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


Module
------

.. c:function:: int PyInitConfig_AddModule(PyInitConfig *config, const char *name, PyObject* (*initfunc)(void))

   Add a built-in extension module to the table of built-in modules.

   The new module can be imported by the name *name*, and uses the function
   *initfunc* as the initialization function called on the first attempted
   import.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.

   If Python is initialized multiple times, ``PyInitConfig_AddModule()`` must
   be called at each Python initialization.

   Similar to the :c:func:`PyImport_AppendInittab` function.


Initialize Python
-----------------

.. c:function:: int Py_InitializeFromInitConfig(PyInitConfig *config)

   Initialize Python from the initialization configuration.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.
   * Set an exit code in *config* and return ``-1`` if Python wants to
     exit.

   See ``PyInitConfig_GetExitcode()`` for the exit code case.


.. _pyinitconfig-opts:

Configuration Options
=====================

.. list-table::
   :header-rows: 1

   * - Option
     - PyConfig/PyPreConfig member
     - Type
     - Visibility
   * - ``"allocator"``
     - :c:member:`allocator <PyPreConfig.allocator>`
     - ``int``
     - Read-only
   * - ``"argv"``
     - :c:member:`argv <PyConfig.argv>`
     - ``list[str]``
     - Public
   * - ``"base_exec_prefix"``
     - :c:member:`base_exec_prefix <PyConfig.base_exec_prefix>`
     - ``str``
     - Public
   * - ``"base_executable"``
     - :c:member:`base_executable <PyConfig.base_executable>`
     - ``str``
     - Public
   * - ``"base_prefix"``
     - :c:member:`base_prefix <PyConfig.base_prefix>`
     - ``str``
     - Public
   * - ``"buffered_stdio"``
     - :c:member:`buffered_stdio <PyConfig.buffered_stdio>`
     - ``bool``
     - Read-only
   * - ``"bytes_warning"``
     - :c:member:`bytes_warning <PyConfig.bytes_warning>`
     - ``int``
     - Public
   * - ``"check_hash_pycs_mode"``
     - :c:member:`check_hash_pycs_mode <PyConfig.check_hash_pycs_mode>`
     - ``str``
     - Read-only
   * - ``"code_debug_ranges"``
     - :c:member:`code_debug_ranges <PyConfig.code_debug_ranges>`
     - ``bool``
     - Read-only
   * - ``"coerce_c_locale"``
     - :c:member:`coerce_c_locale <PyPreConfig.coerce_c_locale>`
     - ``bool``
     - Read-only
   * - ``"coerce_c_locale_warn"``
     - :c:member:`coerce_c_locale_warn <PyPreConfig.coerce_c_locale_warn>`
     - ``bool``
     - Read-only
   * - ``"configure_c_stdio"``
     - :c:member:`configure_c_stdio <PyConfig.configure_c_stdio>`
     - ``bool``
     - Read-only
   * - ``"configure_locale"``
     - :c:member:`configure_locale <PyPreConfig.configure_locale>`
     - ``bool``
     - Read-only
   * - ``"cpu_count"``
     - :c:member:`cpu_count <PyConfig.cpu_count>`
     - ``int``
     - Read-only
   * - ``"dev_mode"``
     - :c:member:`dev_mode <PyConfig.dev_mode>`
     - ``bool``
     - Read-only
   * - ``"dump_refs"``
     - :c:member:`dump_refs <PyConfig.dump_refs>`
     - ``bool``
     - Read-only
   * - ``"dump_refs_file"``
     - :c:member:`dump_refs_file <PyConfig.dump_refs_file>`
     - ``str``
     - Read-only
   * - ``"exec_prefix"``
     - :c:member:`exec_prefix <PyConfig.exec_prefix>`
     - ``str``
     - Public
   * - ``"executable"``
     - :c:member:`executable <PyConfig.executable>`
     - ``str``
     - Public
   * - ``"faulthandler"``
     - :c:member:`faulthandler <PyConfig.faulthandler>`
     - ``bool``
     - Read-only
   * - ``"filesystem_encoding"``
     - :c:member:`filesystem_encoding <PyConfig.filesystem_encoding>`
     - ``str``
     - Read-only
   * - ``"filesystem_errors"``
     - :c:member:`filesystem_errors <PyConfig.filesystem_errors>`
     - ``str``
     - Read-only
   * - ``"hash_seed"``
     - :c:member:`hash_seed <PyConfig.hash_seed>`
     - ``int``
     - Read-only
   * - ``"home"``
     - :c:member:`home <PyConfig.home>`
     - ``str``
     - Read-only
   * - ``"import_time"``
     - :c:member:`import_time <PyConfig.import_time>`
     - ``bool``
     - Read-only
   * - ``"inspect"``
     - :c:member:`inspect <PyConfig.inspect>`
     - ``bool``
     - Public
   * - ``"install_signal_handlers"``
     - :c:member:`install_signal_handlers <PyConfig.install_signal_handlers>`
     - ``bool``
     - Read-only
   * - ``"int_max_str_digits"``
     - :c:member:`int_max_str_digits <PyConfig.int_max_str_digits>`
     - ``int``
     - Public
   * - ``"interactive"``
     - :c:member:`interactive <PyConfig.interactive>`
     - ``bool``
     - Public
   * - ``"isolated"``
     - :c:member:`isolated <PyConfig.isolated>`
     - ``bool``
     - Read-only
   * - ``"legacy_windows_fs_encoding"``
     - :c:member:`legacy_windows_fs_encoding <PyPreConfig.legacy_windows_fs_encoding>`
     - ``bool``
     - Read-only
   * - ``"legacy_windows_stdio"``
     - :c:member:`legacy_windows_stdio <PyConfig.legacy_windows_stdio>`
     - ``bool``
     - Read-only
   * - ``"malloc_stats"``
     - :c:member:`malloc_stats <PyConfig.malloc_stats>`
     - ``bool``
     - Read-only
   * - ``"module_search_paths"``
     - :c:member:`module_search_paths <PyConfig.module_search_paths>`
     - ``list[str]``
     - Public
   * - ``"optimization_level"``
     - :c:member:`optimization_level <PyConfig.optimization_level>`
     - ``int``
     - Public
   * - ``"orig_argv"``
     - :c:member:`orig_argv <PyConfig.orig_argv>`
     - ``list[str]``
     - Read-only
   * - ``"parse_argv"``
     - :c:member:`parse_argv <PyConfig.parse_argv>`
     - ``bool``
     - Read-only
   * - ``"parser_debug"``
     - :c:member:`parser_debug <PyConfig.parser_debug>`
     - ``bool``
     - Public
   * - ``"pathconfig_warnings"``
     - :c:member:`pathconfig_warnings <PyConfig.pathconfig_warnings>`
     - ``bool``
     - Read-only
   * - ``"perf_profiling"``
     - :c:member:`perf_profiling <PyConfig.perf_profiling>`
     - ``bool``
     - Read-only
   * - ``"platlibdir"``
     - :c:member:`platlibdir <PyConfig.platlibdir>`
     - ``str``
     - Public
   * - ``"prefix"``
     - :c:member:`prefix <PyConfig.prefix>`
     - ``str``
     - Public
   * - ``"program_name"``
     - :c:member:`program_name <PyConfig.program_name>`
     - ``str``
     - Read-only
   * - ``"pycache_prefix"``
     - :c:member:`pycache_prefix <PyConfig.pycache_prefix>`
     - ``str``
     - Public
   * - ``"quiet"``
     - :c:member:`quiet <PyConfig.quiet>`
     - ``bool``
     - Public
   * - ``"run_command"``
     - :c:member:`run_command <PyConfig.run_command>`
     - ``str``
     - Read-only
   * - ``"run_filename"``
     - :c:member:`run_filename <PyConfig.run_filename>`
     - ``str``
     - Read-only
   * - ``"run_module"``
     - :c:member:`run_module <PyConfig.run_module>`
     - ``str``
     - Read-only
   * - ``"run_presite"``
     - :c:member:`run_presite <PyConfig.run_presite>`
     - ``str``
     - Read-only
   * - ``"safe_path"``
     - :c:member:`safe_path <PyConfig.safe_path>`
     - ``bool``
     - Read-only
   * - ``"show_ref_count"``
     - :c:member:`show_ref_count <PyConfig.show_ref_count>`
     - ``bool``
     - Read-only
   * - ``"site_import"``
     - :c:member:`site_import <PyConfig.site_import>`
     - ``bool``
     - Read-only
   * - ``"skip_source_first_line"``
     - :c:member:`skip_source_first_line <PyConfig.skip_source_first_line>`
     - ``bool``
     - Read-only
   * - ``"stdio_encoding"``
     - :c:member:`stdio_encoding <PyConfig.stdio_encoding>`
     - ``str``
     - Read-only
   * - ``"stdio_errors"``
     - :c:member:`stdio_errors <PyConfig.stdio_errors>`
     - ``str``
     - Read-only
   * - ``"stdlib_dir"``
     - :c:member:`stdlib_dir <PyConfig.stdlib_dir>`
     - ``str``
     - Public
   * - ``"tracemalloc"``
     - :c:member:`tracemalloc <PyConfig.tracemalloc>`
     - ``int``
     - Read-only
   * - ``"use_environment"``
     - :c:member:`use_environment <PyConfig.use_environment>`
     - ``bool``
     - Public
   * - ``"use_frozen_modules"``
     - :c:member:`use_frozen_modules <PyConfig.use_frozen_modules>`
     - ``bool``
     - Read-only
   * - ``"use_hash_seed"``
     - :c:member:`use_hash_seed <PyConfig.use_hash_seed>`
     - ``bool``
     - Read-only
   * - ``"use_system_logger"``
     - :c:member:`use_system_logger <PyConfig.use_system_logger>`
     - ``bool``
     - Read-only
   * - ``"user_site_directory"``
     - :c:member:`user_site_directory <PyConfig.user_site_directory>`
     - ``bool``
     - Read-only
   * - ``"utf8_mode"``
     - :c:member:`utf8_mode <PyPreConfig.utf8_mode>`
     - ``bool``
     - Read-only
   * - ``"verbose"``
     - :c:member:`verbose <PyConfig.verbose>`
     - ``int``
     - Public
   * - ``"warn_default_encoding"``
     - :c:member:`warn_default_encoding <PyConfig.warn_default_encoding>`
     - ``bool``
     - Read-only
   * - ``"warnoptions"``
     - :c:member:`warnoptions <PyConfig.warnoptions>`
     - ``list[str]``
     - Public
   * - ``"write_bytecode"``
     - :c:member:`write_bytecode <PyConfig.write_bytecode>`
     - ``bool``
     - Public
   * - ``"xoptions"``
     - :c:member:`xoptions <PyConfig.xoptions>`
     - ``dict[str, str]``
     - Public
   * - ``"_pystats"``
     - :c:member:`_pystats <PyConfig._pystats>`
     - ``bool``
     - Read-only

Visibility:

* Public: Can by get by :c:func:`PyConfig_Get` and set by
  :c:func:`PyConfig_Set`.
* Read-only: Can by get by :c:func:`PyConfig_Get`, but cannot be set by
  :c:func:`PyConfig_Set`.


Runtime Python configuration API
================================

At runtime, it's possible to get and set configuration options using
:c:func:`PyConfig_Get` and  :c:func:`PyConfig_Set` functions.

The configuration option *name* parameter must be a non-NULL null-terminated
UTF-8 encoded string. See :ref:`Configuration Options <pyinitconfig-opts>`.

Some options are read from the :mod:`sys` attributes. For example, the option
``"argv"`` is read from :data:`sys.argv`.


.. c:function:: PyObject* PyConfig_Get(const char *name)

   Get the current runtime value of a configuration option as a Python object.

   * Return a new reference on success.
   * Set an exception and return ``NULL`` on error.

   The object type depends on the configuration option. It can be:

   * ``bool``
   * ``int``
   * ``str``
   * ``list[str]``
   * ``dict[str, str]``

   The caller must have an :term:`attached thread state`. The function cannot
   be called before Python initialization nor after Python finalization.

   .. versionadded:: 3.14


.. c:function:: int PyConfig_GetInt(const char *name, int *value)

   Similar to :c:func:`PyConfig_Get`, but get the value as a C int.

   * Return ``0`` on success.
   * Set an exception and return ``-1`` on error.

   .. versionadded:: 3.14


.. c:function:: PyObject* PyConfig_Names(void)

   Get all configuration option names as a ``frozenset``.

   * Return a new reference on success.
   * Set an exception and return ``NULL`` on error.

   The caller must have an :term:`attached thread state`. The function cannot
   be called before Python initialization nor after Python finalization.

   .. versionadded:: 3.14


.. c:function:: int PyConfig_Set(const char *name, PyObject *value)

   Set the current runtime value of a configuration option.

   * Raise a :exc:`ValueError` if there is no option *name*.
   * Raise a :exc:`ValueError` if *value* is an invalid value.
   * Raise a :exc:`ValueError` if the option is read-only (cannot be set).
   * Raise a :exc:`TypeError` if *value* has not the proper type.

   The caller must have an :term:`attached thread state`. The function cannot
   be called before Python initialization nor after Python finalization.

   .. versionadded:: 3.14


.. _pyconfig_api:

PyConfig C API
==============

.. versionadded:: 3.8

Python can be initialized with :c:func:`Py_InitializeFromConfig` and the
:c:type:`PyConfig` structure. It can be preinitialized with
:c:func:`Py_PreInitialize` and the :c:type:`PyPreConfig` structure.

There are two kinds of configuration:

* The :ref:`Python Configuration <init-python-config>` can be used to build a
  customized Python which behaves as the regular Python. For example,
  environment variables and command line arguments are used to configure
  Python.

* The :ref:`Isolated Configuration <init-isolated-conf>` can be used to embed
  Python into an application. It isolates Python from the system. For example,
  environment variables are ignored, the LC_CTYPE locale is left unchanged and
  no signal handler is registered.

The :c:func:`Py_RunMain` function can be used to write a customized Python
program.

See also :ref:`Initialization, Finalization, and Threads <initialization>`.

.. seealso::
   :pep:`587` "Python Initialization Configuration".


Example
-------

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
            goto exception;
        }

        status = Py_InitializeFromConfig(&config);
        if (PyStatus_Exception(status)) {
            goto exception;
        }
        PyConfig_Clear(&config);

        return Py_RunMain();

    exception:
        PyConfig_Clear(&config);
        if (PyStatus_IsExit(status)) {
            return status.exitcode;
        }
        /* Display the error message and exit the process with
           non-zero exit code */
        Py_ExitStatusException(status);
    }


PyWideStringList
----------------

.. c:type:: PyWideStringList

   List of ``wchar_t*`` strings.

   If *length* is non-zero, *items* must be non-``NULL`` and all strings must be
   non-``NULL``.

   .. c:namespace:: NULL

   Methods:

   .. c:function:: PyStatus PyWideStringList_Append(PyWideStringList *list, const wchar_t *item)

      Append *item* to *list*.

      Python must be preinitialized to call this function.

   .. c:function:: PyStatus PyWideStringList_Insert(PyWideStringList *list, Py_ssize_t index, const wchar_t *item)

      Insert *item* into *list* at *index*.

      If *index* is greater than or equal to *list* length, append *item* to
      *list*.

      *index* must be greater than or equal to ``0``.

      Python must be preinitialized to call this function.

   .. c:namespace:: PyWideStringList

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

   .. c:namespace:: NULL

   Functions to create a status:

   .. c:function:: PyStatus PyStatus_Ok(void)

      Success.

   .. c:function:: PyStatus PyStatus_Error(const char *err_msg)

      Initialization error with a message.

      *err_msg* must not be ``NULL``.

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

   Structure used to preinitialize Python.

   .. c:namespace:: NULL

   Function to initialize a preconfiguration:

   .. c:function:: void PyPreConfig_InitPythonConfig(PyPreConfig *preconfig)

      Initialize the preconfiguration with :ref:`Python Configuration
      <init-python-config>`.

   .. c:function:: void PyPreConfig_InitIsolatedConfig(PyPreConfig *preconfig)

      Initialize the preconfiguration with :ref:`Isolated Configuration
      <init-isolated-conf>`.

   .. c:namespace:: PyPreConfig

   Structure fields:

   .. c:member:: int allocator

      Name of the Python memory allocators:

      * ``PYMEM_ALLOCATOR_NOT_SET`` (``0``): don't change memory allocators
        (use defaults).
      * ``PYMEM_ALLOCATOR_DEFAULT`` (``1``): :ref:`default memory allocators
        <default-memory-allocators>`.
      * ``PYMEM_ALLOCATOR_DEBUG`` (``2``): :ref:`default memory allocators
        <default-memory-allocators>` with :ref:`debug hooks
        <pymem-debug-hooks>`.
      * ``PYMEM_ALLOCATOR_MALLOC`` (``3``): use ``malloc()`` of the C library.
      * ``PYMEM_ALLOCATOR_MALLOC_DEBUG`` (``4``): force usage of
        ``malloc()`` with :ref:`debug hooks <pymem-debug-hooks>`.
      * ``PYMEM_ALLOCATOR_PYMALLOC`` (``5``): :ref:`Python pymalloc memory
        allocator <pymalloc>`.
      * ``PYMEM_ALLOCATOR_PYMALLOC_DEBUG`` (``6``): :ref:`Python pymalloc
        memory allocator <pymalloc>` with :ref:`debug hooks
        <pymem-debug-hooks>`.
      * ``PYMEM_ALLOCATOR_MIMALLOC`` (``6``): use ``mimalloc``, a fast
        malloc replacement.
      * ``PYMEM_ALLOCATOR_MIMALLOC_DEBUG`` (``7``): use ``mimalloc``, a fast
        malloc replacement with :ref:`debug hooks <pymem-debug-hooks>`.


      ``PYMEM_ALLOCATOR_PYMALLOC`` and ``PYMEM_ALLOCATOR_PYMALLOC_DEBUG`` are
      not supported if Python is :option:`configured using --without-pymalloc
      <--without-pymalloc>`.

      ``PYMEM_ALLOCATOR_MIMALLOC`` and ``PYMEM_ALLOCATOR_MIMALLOC_DEBUG`` are
      not supported if Python is :option:`configured using --without-mimalloc
      <--without-mimalloc>` or if the underlying atomic support isn't
      available.

      See :ref:`Memory Management <memory>`.

      Default: ``PYMEM_ALLOCATOR_NOT_SET``.

   .. c:member:: int configure_locale

      Set the LC_CTYPE locale to the user preferred locale.

      If equals to ``0``, set :c:member:`~PyPreConfig.coerce_c_locale` and
      :c:member:`~PyPreConfig.coerce_c_locale_warn` members to ``0``.

      See the :term:`locale encoding`.

      Default: ``1`` in Python config, ``0`` in isolated config.

   .. c:member:: int coerce_c_locale

      If equals to ``2``, coerce the C locale.

      If equals to ``1``, read the LC_CTYPE locale to decide if it should be
      coerced.

      See the :term:`locale encoding`.

      Default: ``-1`` in Python config, ``0`` in isolated config.

   .. c:member:: int coerce_c_locale_warn

      If non-zero, emit a warning if the C locale is coerced.

      Default: ``-1`` in Python config, ``0`` in isolated config.

   .. c:member:: int dev_mode

      :ref:`Python Development Mode <devmode>`: see
      :c:member:`PyConfig.dev_mode`.

      Default: ``-1`` in Python mode, ``0`` in isolated mode.

   .. c:member:: int isolated

      Isolated mode: see :c:member:`PyConfig.isolated`.

      Default: ``0`` in Python mode, ``1`` in isolated mode.

   .. c:member:: int legacy_windows_fs_encoding

      If non-zero:

      * Set :c:member:`PyPreConfig.utf8_mode` to ``0``,
      * Set :c:member:`PyConfig.filesystem_encoding` to ``"mbcs"``,
      * Set :c:member:`PyConfig.filesystem_errors` to ``"replace"``.

      Initialized from the :envvar:`PYTHONLEGACYWINDOWSFSENCODING` environment
      variable value.

      Only available on Windows. ``#ifdef MS_WINDOWS`` macro can be used for
      Windows specific code.

      Default: ``0``.

   .. c:member:: int parse_argv

      If non-zero, :c:func:`Py_PreInitializeFromArgs` and
      :c:func:`Py_PreInitializeFromBytesArgs` parse their ``argv`` argument the
      same way the regular Python parses command line arguments: see
      :ref:`Command Line Arguments <using-on-cmdline>`.

      Default: ``1`` in Python config, ``0`` in isolated config.

   .. c:member:: int use_environment

      Use :ref:`environment variables <using-on-envvars>`? See
      :c:member:`PyConfig.use_environment`.

      Default: ``1`` in Python config and ``0`` in isolated config.

   .. c:member:: int utf8_mode

      If non-zero, enable the :ref:`Python UTF-8 Mode <utf8-mode>`.

      Set to ``0`` or ``1`` by the :option:`-X utf8 <-X>` command line option
      and the :envvar:`PYTHONUTF8` environment variable.

      Also set to ``1`` if the ``LC_CTYPE`` locale is ``C`` or ``POSIX``.

      Default: ``-1`` in Python config and ``0`` in isolated config.


.. _c-preinit:

Preinitialize Python with PyPreConfig
-------------------------------------

The preinitialization of Python:

* Set the Python memory allocators (:c:member:`PyPreConfig.allocator`)
* Configure the LC_CTYPE locale (:term:`locale encoding`)
* Set the :ref:`Python UTF-8 Mode <utf8-mode>`
  (:c:member:`PyPreConfig.utf8_mode`)

The current preconfiguration (``PyPreConfig`` type) is stored in
``_PyRuntime.preconfig``.

Functions to preinitialize Python:

.. c:function:: PyStatus Py_PreInitialize(const PyPreConfig *preconfig)

   Preinitialize Python from *preconfig* preconfiguration.

   *preconfig* must not be ``NULL``.

.. c:function:: PyStatus Py_PreInitializeFromBytesArgs(const PyPreConfig *preconfig, int argc, char * const *argv)

   Preinitialize Python from *preconfig* preconfiguration.

   Parse *argv* command line arguments (bytes strings) if
   :c:member:`~PyPreConfig.parse_argv` of *preconfig* is non-zero.

   *preconfig* must not be ``NULL``.

.. c:function:: PyStatus Py_PreInitializeFromArgs(const PyPreConfig *preconfig, int argc, wchar_t * const * argv)

   Preinitialize Python from *preconfig* preconfiguration.

   Parse *argv* command line arguments (wide strings) if
   :c:member:`~PyPreConfig.parse_argv` of *preconfig* is non-zero.

   *preconfig* must not be ``NULL``.

The caller is responsible to handle exceptions (error or exit) using
:c:func:`PyStatus_Exception` and :c:func:`Py_ExitStatusException`.

For :ref:`Python Configuration <init-python-config>`
(:c:func:`PyPreConfig_InitPythonConfig`), if Python is initialized with
command line arguments, the command line arguments must also be passed to
preinitialize Python, since they have an effect on the pre-configuration
like encodings. For example, the :option:`-X utf8 <-X>` command line option
enables the :ref:`Python UTF-8 Mode <utf8-mode>`.

``PyMem_SetAllocator()`` can be called after :c:func:`Py_PreInitialize` and
before :c:func:`Py_InitializeFromConfig` to install a custom memory allocator.
It can be called before :c:func:`Py_PreInitialize` if
:c:member:`PyPreConfig.allocator` is set to ``PYMEM_ALLOCATOR_NOT_SET``.

Python memory allocation functions like :c:func:`PyMem_RawMalloc` must not be
used before the Python preinitialization, whereas calling directly ``malloc()``
and ``free()`` is always safe. :c:func:`Py_DecodeLocale` must not be called
before the Python preinitialization.

Example using the preinitialization to enable
the :ref:`Python UTF-8 Mode <utf8-mode>`::

    PyStatus status;
    PyPreConfig preconfig;
    PyPreConfig_InitPythonConfig(&preconfig);

    preconfig.utf8_mode = 1;

    status = Py_PreInitialize(&preconfig);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    /* at this point, Python speaks UTF-8 */

    Py_Initialize();
    /* ... use Python API here ... */
    Py_Finalize();


PyConfig
--------

.. c:type:: PyConfig

   Structure containing most parameters to configure Python.

   When done, the :c:func:`PyConfig_Clear` function must be used to release the
   configuration memory.

   .. c:namespace:: NULL

   Structure methods:

   .. c:function:: void PyConfig_InitPythonConfig(PyConfig *config)

      Initialize configuration with the :ref:`Python Configuration
      <init-python-config>`.

   .. c:function:: void PyConfig_InitIsolatedConfig(PyConfig *config)

      Initialize configuration with the :ref:`Isolated Configuration
      <init-isolated-conf>`.

   .. c:function:: PyStatus PyConfig_SetString(PyConfig *config, wchar_t * const *config_str, const wchar_t *str)

      Copy the wide character string *str* into ``*config_str``.

      :ref:`Preinitialize Python <c-preinit>` if needed.

   .. c:function:: PyStatus PyConfig_SetBytesString(PyConfig *config, wchar_t * const *config_str, const char *str)

      Decode *str* using :c:func:`Py_DecodeLocale` and set the result into
      ``*config_str``.

      :ref:`Preinitialize Python <c-preinit>` if needed.

   .. c:function:: PyStatus PyConfig_SetArgv(PyConfig *config, int argc, wchar_t * const *argv)

      Set command line arguments (:c:member:`~PyConfig.argv` member of
      *config*) from the *argv* list of wide character strings.

      :ref:`Preinitialize Python <c-preinit>` if needed.

   .. c:function:: PyStatus PyConfig_SetBytesArgv(PyConfig *config, int argc, char * const *argv)

      Set command line arguments (:c:member:`~PyConfig.argv` member of
      *config*) from the *argv* list of bytes strings. Decode bytes using
      :c:func:`Py_DecodeLocale`.

      :ref:`Preinitialize Python <c-preinit>` if needed.

   .. c:function:: PyStatus PyConfig_SetWideStringList(PyConfig *config, PyWideStringList *list, Py_ssize_t length, wchar_t **items)

      Set the list of wide strings *list* to *length* and *items*.

      :ref:`Preinitialize Python <c-preinit>` if needed.

   .. c:function:: PyStatus PyConfig_Read(PyConfig *config)

      Read all Python configuration.

      Fields which are already initialized are left unchanged.

      Fields for :ref:`path configuration <init-path-config>` are no longer
      calculated or modified when calling this function, as of Python 3.11.

      The :c:func:`PyConfig_Read` function only parses
      :c:member:`PyConfig.argv` arguments once: :c:member:`PyConfig.parse_argv`
      is set to ``2`` after arguments are parsed. Since Python arguments are
      stripped from :c:member:`PyConfig.argv`, parsing arguments twice would
      parse the application options as Python options.

      :ref:`Preinitialize Python <c-preinit>` if needed.

      .. versionchanged:: 3.10
         The :c:member:`PyConfig.argv` arguments are now only parsed once,
         :c:member:`PyConfig.parse_argv` is set to ``2`` after arguments are
         parsed, and arguments are only parsed if
         :c:member:`PyConfig.parse_argv` equals ``1``.

      .. versionchanged:: 3.11
         :c:func:`PyConfig_Read` no longer calculates all paths, and so fields
         listed under :ref:`Python Path Configuration <init-path-config>` may
         no longer be updated until :c:func:`Py_InitializeFromConfig` is
         called.

   .. c:function:: void PyConfig_Clear(PyConfig *config)

      Release configuration memory.

   Most ``PyConfig`` methods :ref:`preinitialize Python <c-preinit>` if needed.
   In that case, the Python preinitialization configuration
   (:c:type:`PyPreConfig`) in based on the :c:type:`PyConfig`. If configuration
   fields which are in common with :c:type:`PyPreConfig` are tuned, they must
   be set before calling a :c:type:`PyConfig` method:

   * :c:member:`PyConfig.dev_mode`
   * :c:member:`PyConfig.isolated`
   * :c:member:`PyConfig.parse_argv`
   * :c:member:`PyConfig.use_environment`

   Moreover, if :c:func:`PyConfig_SetArgv` or :c:func:`PyConfig_SetBytesArgv`
   is used, this method must be called before other methods, since the
   preinitialization configuration depends on command line arguments (if
   :c:member:`~PyConfig.parse_argv` is non-zero).

   The caller of these methods is responsible to handle exceptions (error or
   exit) using ``PyStatus_Exception()`` and ``Py_ExitStatusException()``.

   .. c:namespace:: PyConfig

   Structure fields:

   .. c:member:: PyWideStringList argv

      .. index::
         single: main()
         single: argv (in module sys)

      Set :data:`sys.argv` command line arguments based on
      :c:member:`~PyConfig.argv`.  These parameters are similar to those passed
      to the program's :c:func:`main` function with the difference that the
      first entry should refer to the script file to be executed rather than
      the executable hosting the Python interpreter.  If there isn't a script
      that will be run, the first entry in :c:member:`~PyConfig.argv` can be an
      empty string.

      Set :c:member:`~PyConfig.parse_argv` to ``1`` to parse
      :c:member:`~PyConfig.argv` the same way the regular Python parses Python
      command line arguments and then to strip Python arguments from
      :c:member:`~PyConfig.argv`.

      If :c:member:`~PyConfig.argv` is empty, an empty string is added to
      ensure that :data:`sys.argv` always exists and is never empty.

      Default: ``NULL``.

      See also the :c:member:`~PyConfig.orig_argv` member.

   .. c:member:: int safe_path

      If equals to zero, ``Py_RunMain()`` prepends a potentially unsafe path to
      :data:`sys.path` at startup:

      * If :c:member:`argv[0] <PyConfig.argv>` is equal to ``L"-m"``
        (``python -m module``), prepend the current working directory.
      * If running a script (``python script.py``), prepend the script's
        directory.  If it's a symbolic link, resolve symbolic links.
      * Otherwise (``python -c code`` and ``python``), prepend an empty string,
        which means the current working directory.

      Set to ``1`` by the :option:`-P` command line option and the
      :envvar:`PYTHONSAFEPATH` environment variable.

      Default: ``0`` in Python config, ``1`` in isolated config.

      .. versionadded:: 3.11

   .. c:member:: wchar_t* base_exec_prefix

      :data:`sys.base_exec_prefix`.

      Default: ``NULL``.

      Part of the :ref:`Python Path Configuration <init-path-config>` output.

      See also :c:member:`PyConfig.exec_prefix`.

   .. c:member:: wchar_t* base_executable

      Python base executable: :data:`sys._base_executable`.

      Set by the :envvar:`__PYVENV_LAUNCHER__` environment variable.

      Set from :c:member:`PyConfig.executable` if ``NULL``.

      Default: ``NULL``.

      Part of the :ref:`Python Path Configuration <init-path-config>` output.

      See also :c:member:`PyConfig.executable`.

   .. c:member:: wchar_t* base_prefix

      :data:`sys.base_prefix`.

      Default: ``NULL``.

      Part of the :ref:`Python Path Configuration <init-path-config>` output.

      See also :c:member:`PyConfig.prefix`.

   .. c:member:: int buffered_stdio

      If equals to ``0`` and :c:member:`~PyConfig.configure_c_stdio` is non-zero,
      disable buffering on the C streams stdout and stderr.

      Set to ``0`` by the :option:`-u` command line option and the
      :envvar:`PYTHONUNBUFFERED` environment variable.

      stdin is always opened in buffered mode.

      Default: ``1``.

   .. c:member:: int bytes_warning

      If equals to ``1``, issue a warning when comparing :class:`bytes` or
      :class:`bytearray` with :class:`str`, or comparing :class:`bytes` with
      :class:`int`.

      If equal or greater to ``2``, raise a :exc:`BytesWarning` exception in these
      cases.

      Incremented by the :option:`-b` command line option.

      Default: ``0``.

   .. c:member:: int warn_default_encoding

      If non-zero, emit a :exc:`EncodingWarning` warning when :class:`io.TextIOWrapper`
      uses its default encoding. See :ref:`io-encoding-warning` for details.

      Default: ``0``.

      .. versionadded:: 3.10

   .. c:member:: int code_debug_ranges

      If equals to ``0``, disables the inclusion of the end line and column
      mappings in code objects. Also disables traceback printing carets to
      specific error locations.

      Set to ``0`` by the :envvar:`PYTHONNODEBUGRANGES` environment variable
      and by the :option:`-X no_debug_ranges <-X>` command line option.

      Default: ``1``.

      .. versionadded:: 3.11

   .. c:member:: wchar_t* check_hash_pycs_mode

      Control the validation behavior of hash-based ``.pyc`` files:
      value of the :option:`--check-hash-based-pycs` command line option.

      Valid values:

      - ``L"always"``: Hash the source file for invalidation regardless of
        value of the 'check_source' flag.
      - ``L"never"``: Assume that hash-based pycs always are valid.
      - ``L"default"``: The 'check_source' flag in hash-based pycs
        determines invalidation.

      Default: ``L"default"``.

      See also :pep:`552` "Deterministic pycs".

   .. c:member:: int configure_c_stdio

      If non-zero, configure C standard streams:

      * On Windows, set the binary mode (``O_BINARY``) on stdin, stdout and
        stderr.
      * If :c:member:`~PyConfig.buffered_stdio` equals zero, disable buffering
        of stdin, stdout and stderr streams.
      * If :c:member:`~PyConfig.interactive` is non-zero, enable stream
        buffering on stdin and stdout (only stdout on Windows).

      Default: ``1`` in Python config, ``0`` in isolated config.

   .. c:member:: int dev_mode

      If non-zero, enable the :ref:`Python Development Mode <devmode>`.

      Set to ``1`` by the :option:`-X dev <-X>` option and the
      :envvar:`PYTHONDEVMODE` environment variable.

      Default: ``-1`` in Python mode, ``0`` in isolated mode.

   .. c:member:: int dump_refs

      Dump Python references?

      If non-zero, dump all objects which are still alive at exit.

      Set to ``1`` by the :envvar:`PYTHONDUMPREFS` environment variable.

      Needs a special build of Python with the ``Py_TRACE_REFS`` macro defined:
      see the :option:`configure --with-trace-refs option <--with-trace-refs>`.

      Default: ``0``.

   .. c:member:: wchar_t* dump_refs_file

      Filename where to dump Python references.

      Set by the :envvar:`PYTHONDUMPREFSFILE` environment variable.

      Default: ``NULL``.

      .. versionadded:: 3.11

   .. c:member:: wchar_t* exec_prefix

      The site-specific directory prefix where the platform-dependent Python
      files are installed: :data:`sys.exec_prefix`.

      Default: ``NULL``.

      Part of the :ref:`Python Path Configuration <init-path-config>` output.

      See also :c:member:`PyConfig.base_exec_prefix`.

   .. c:member:: wchar_t* executable

      The absolute path of the executable binary for the Python interpreter:
      :data:`sys.executable`.

      Default: ``NULL``.

      Part of the :ref:`Python Path Configuration <init-path-config>` output.

      See also :c:member:`PyConfig.base_executable`.

   .. c:member:: int faulthandler

      Enable faulthandler?

      If non-zero, call :func:`faulthandler.enable` at startup.

      Set to ``1`` by :option:`-X faulthandler <-X>` and the
      :envvar:`PYTHONFAULTHANDLER` environment variable.

      Default: ``-1`` in Python mode, ``0`` in isolated mode.

   .. c:member:: wchar_t* filesystem_encoding

      :term:`Filesystem encoding <filesystem encoding and error handler>`:
      :func:`sys.getfilesystemencoding`.

      On macOS, Android and VxWorks: use ``"utf-8"`` by default.

      On Windows: use ``"utf-8"`` by default, or ``"mbcs"`` if
      :c:member:`~PyPreConfig.legacy_windows_fs_encoding` of
      :c:type:`PyPreConfig` is non-zero.

      Default encoding on other platforms:

      * ``"utf-8"`` if :c:member:`PyPreConfig.utf8_mode` is non-zero.
      * ``"ascii"`` if Python detects that ``nl_langinfo(CODESET)`` announces
        the ASCII encoding, whereas the ``mbstowcs()`` function
        decodes from a different encoding (usually Latin1).
      * ``"utf-8"`` if ``nl_langinfo(CODESET)`` returns an empty string.
      * Otherwise, use the :term:`locale encoding`:
        ``nl_langinfo(CODESET)`` result.

      At Python startup, the encoding name is normalized to the Python codec
      name. For example, ``"ANSI_X3.4-1968"`` is replaced with ``"ascii"``.

      See also the :c:member:`~PyConfig.filesystem_errors` member.

   .. c:member:: wchar_t* filesystem_errors

      :term:`Filesystem error handler <filesystem encoding and error handler>`:
      :func:`sys.getfilesystemencodeerrors`.

      On Windows: use ``"surrogatepass"`` by default, or ``"replace"``  if
      :c:member:`~PyPreConfig.legacy_windows_fs_encoding` of
      :c:type:`PyPreConfig` is non-zero.

      On other platforms: use ``"surrogateescape"`` by default.

      Supported error handlers:

      * ``"strict"``
      * ``"surrogateescape"``
      * ``"surrogatepass"`` (only supported with the UTF-8 encoding)

      See also the :c:member:`~PyConfig.filesystem_encoding` member.

   .. c:member:: int use_frozen_modules

      If non-zero, use frozen modules.

      Set by the :envvar:`PYTHON_FROZEN_MODULES` environment variable.

      Default: ``1`` in a release build, or ``0`` in a :ref:`debug build
      <debug-build>`.

   .. c:member:: unsigned long hash_seed
   .. c:member:: int use_hash_seed

      Randomized hash function seed.

      If :c:member:`~PyConfig.use_hash_seed` is zero, a seed is chosen randomly
      at Python startup, and :c:member:`~PyConfig.hash_seed` is ignored.

      Set by the :envvar:`PYTHONHASHSEED` environment variable.

      Default *use_hash_seed* value: ``-1`` in Python mode, ``0`` in isolated
      mode.

   .. c:member:: wchar_t* home

      Set the default Python "home" directory, that is, the location of the
      standard Python libraries (see :envvar:`PYTHONHOME`).

      Set by the :envvar:`PYTHONHOME` environment variable.

      Default: ``NULL``.

      Part of the :ref:`Python Path Configuration <init-path-config>` input.

   .. c:member:: int import_time

      If non-zero, profile import time.

      Set the ``1`` by the :option:`-X importtime <-X>` option and the
      :envvar:`PYTHONPROFILEIMPORTTIME` environment variable.

      Default: ``0``.

   .. c:member:: int inspect

      Enter interactive mode after executing a script or a command.

      If greater than ``0``, enable inspect: when a script is passed as first
      argument or the -c option is used, enter interactive mode after executing
      the script or the command, even when :data:`sys.stdin` does not appear to
      be a terminal.

      Incremented by the :option:`-i` command line option. Set to ``1`` if the
      :envvar:`PYTHONINSPECT` environment variable is non-empty.

      Default: ``0``.

   .. c:member:: int install_signal_handlers

      Install Python signal handlers?

      Default: ``1`` in Python mode, ``0`` in isolated mode.

   .. c:member:: int interactive

      If greater than ``0``, enable the interactive mode (REPL).

      Incremented by the :option:`-i` command line option.

      Default: ``0``.

   .. c:member:: int int_max_str_digits

      Configures the :ref:`integer string conversion length limitation
      <int_max_str_digits>`.  An initial value of ``-1`` means the value will
      be taken from the command line or environment or otherwise default to
      4300 (:data:`sys.int_info.default_max_str_digits`).  A value of ``0``
      disables the limitation.  Values greater than zero but less than 640
      (:data:`sys.int_info.str_digits_check_threshold`) are unsupported and
      will produce an error.

      Configured by the :option:`-X int_max_str_digits <-X>` command line
      flag or the :envvar:`PYTHONINTMAXSTRDIGITS` environment variable.

      Default: ``-1`` in Python mode.  4300
      (:data:`sys.int_info.default_max_str_digits`) in isolated mode.

      .. versionadded:: 3.12

   .. c:member:: int cpu_count

      If the value of :c:member:`~PyConfig.cpu_count` is not ``-1`` then it will
      override the return values of :func:`os.cpu_count`,
      :func:`os.process_cpu_count`, and :func:`multiprocessing.cpu_count`.

      Configured by the :samp:`-X cpu_count={n|default}` command line
      flag or the :envvar:`PYTHON_CPU_COUNT` environment variable.

      Default: ``-1``.

      .. versionadded:: 3.13

   .. c:member:: int isolated

      If greater than ``0``, enable isolated mode:

      * Set :c:member:`~PyConfig.safe_path` to ``1``:
        don't prepend a potentially unsafe path to :data:`sys.path` at Python
        startup, such as the current directory, the script's directory or an
        empty string.
      * Set :c:member:`~PyConfig.use_environment` to ``0``: ignore ``PYTHON``
        environment variables.
      * Set :c:member:`~PyConfig.user_site_directory` to ``0``: don't add the user
        site directory to :data:`sys.path`.
      * Python REPL doesn't import :mod:`readline` nor enable default readline
        configuration on interactive prompts.

      Set to ``1`` by the :option:`-I` command line option.

      Default: ``0`` in Python mode, ``1`` in isolated mode.

      See also the :ref:`Isolated Configuration <init-isolated-conf>` and
      :c:member:`PyPreConfig.isolated`.

   .. c:member:: int legacy_windows_stdio

      If non-zero, use :class:`io.FileIO` instead of
      :class:`!io._WindowsConsoleIO` for :data:`sys.stdin`, :data:`sys.stdout`
      and :data:`sys.stderr`.

      Set to ``1`` if the :envvar:`PYTHONLEGACYWINDOWSSTDIO` environment
      variable is set to a non-empty string.

      Only available on Windows. ``#ifdef MS_WINDOWS`` macro can be used for
      Windows specific code.

      Default: ``0``.

      See also the :pep:`528` (Change Windows console encoding to UTF-8).

   .. c:member:: int malloc_stats

      If non-zero, dump statistics on :ref:`Python pymalloc memory allocator
      <pymalloc>` at exit.

      Set to ``1`` by the :envvar:`PYTHONMALLOCSTATS` environment variable.

      The option is ignored if Python is :option:`configured using
      the --without-pymalloc option <--without-pymalloc>`.

      Default: ``0``.

   .. c:member:: wchar_t* platlibdir

      Platform library directory name: :data:`sys.platlibdir`.

      Set by the :envvar:`PYTHONPLATLIBDIR` environment variable.

      Default: value of the ``PLATLIBDIR`` macro which is set by the
      :option:`configure --with-platlibdir option <--with-platlibdir>`
      (default: ``"lib"``, or ``"DLLs"`` on Windows).

      Part of the :ref:`Python Path Configuration <init-path-config>` input.

      .. versionadded:: 3.9

      .. versionchanged:: 3.11
         This macro is now used on Windows to locate the standard
         library extension modules, typically under ``DLLs``. However,
         for compatibility, note that this value is ignored for any
         non-standard layouts, including in-tree builds and virtual
         environments.

   .. c:member:: wchar_t* pythonpath_env

      Module search paths (:data:`sys.path`) as a string separated by ``DELIM``
      (:data:`os.pathsep`).

      Set by the :envvar:`PYTHONPATH` environment variable.

      Default: ``NULL``.

      Part of the :ref:`Python Path Configuration <init-path-config>` input.

   .. c:member:: PyWideStringList module_search_paths
   .. c:member:: int module_search_paths_set

      Module search paths: :data:`sys.path`.

      If :c:member:`~PyConfig.module_search_paths_set` is equal to ``0``,
      :c:func:`Py_InitializeFromConfig` will replace
      :c:member:`~PyConfig.module_search_paths` and sets
      :c:member:`~PyConfig.module_search_paths_set` to ``1``.

      Default: empty list (``module_search_paths``) and ``0``
      (``module_search_paths_set``).

      Part of the :ref:`Python Path Configuration <init-path-config>` output.

   .. c:member:: int optimization_level

      Compilation optimization level:

      * ``0``: Peephole optimizer, set ``__debug__`` to ``True``.
      * ``1``: Level 0, remove assertions, set ``__debug__`` to ``False``.
      * ``2``: Level 1, strip docstrings.

      Incremented by the :option:`-O` command line option. Set to the
      :envvar:`PYTHONOPTIMIZE` environment variable value.

      Default: ``0``.

   .. c:member:: PyWideStringList orig_argv

      The list of the original command line arguments passed to the Python
      executable: :data:`sys.orig_argv`.

      If :c:member:`~PyConfig.orig_argv` list is empty and
      :c:member:`~PyConfig.argv` is not a list only containing an empty
      string, :c:func:`PyConfig_Read` copies :c:member:`~PyConfig.argv` into
      :c:member:`~PyConfig.orig_argv` before modifying
      :c:member:`~PyConfig.argv` (if :c:member:`~PyConfig.parse_argv` is
      non-zero).

      See also the :c:member:`~PyConfig.argv` member and the
      :c:func:`Py_GetArgcArgv` function.

      Default: empty list.

      .. versionadded:: 3.10

   .. c:member:: int parse_argv

      Parse command line arguments?

      If equals to ``1``, parse :c:member:`~PyConfig.argv` the same way the regular
      Python parses :ref:`command line arguments <using-on-cmdline>`, and strip
      Python arguments from :c:member:`~PyConfig.argv`.

      The :c:func:`PyConfig_Read` function only parses
      :c:member:`PyConfig.argv` arguments once: :c:member:`PyConfig.parse_argv`
      is set to ``2`` after arguments are parsed. Since Python arguments are
      stripped from :c:member:`PyConfig.argv`, parsing arguments twice would
      parse the application options as Python options.

      Default: ``1`` in Python mode, ``0`` in isolated mode.

      .. versionchanged:: 3.10
         The :c:member:`PyConfig.argv` arguments are now only parsed if
         :c:member:`PyConfig.parse_argv` equals to ``1``.

   .. c:member:: int parser_debug

      Parser debug mode. If greater than ``0``, turn on parser debugging output (for expert only, depending
      on compilation options).

      Incremented by the :option:`-d` command line option. Set to the
      :envvar:`PYTHONDEBUG` environment variable value.

      Needs a :ref:`debug build of Python <debug-build>` (the ``Py_DEBUG`` macro
      must be defined).

      Default: ``0``.

   .. c:member:: int pathconfig_warnings

      If non-zero, calculation of path configuration is allowed to log
      warnings into ``stderr``. If equals to ``0``, suppress these warnings.

      Default: ``1`` in Python mode, ``0`` in isolated mode.

      Part of the :ref:`Python Path Configuration <init-path-config>` input.

      .. versionchanged:: 3.11
         Now also applies on Windows.

   .. c:member:: wchar_t* prefix

      The site-specific directory prefix where the platform independent Python
      files are installed: :data:`sys.prefix`.

      Default: ``NULL``.

      Part of the :ref:`Python Path Configuration <init-path-config>` output.

      See also :c:member:`PyConfig.base_prefix`.

   .. c:member:: wchar_t* program_name

      Program name used to initialize :c:member:`~PyConfig.executable` and in
      early error messages during Python initialization.

      * On macOS, use :envvar:`PYTHONEXECUTABLE` environment variable if set.
      * If the ``WITH_NEXT_FRAMEWORK`` macro is defined, use
        :envvar:`__PYVENV_LAUNCHER__` environment variable if set.
      * Use ``argv[0]`` of :c:member:`~PyConfig.argv` if available and
        non-empty.
      * Otherwise, use ``L"python"`` on Windows, or ``L"python3"`` on other
        platforms.

      Default: ``NULL``.

      Part of the :ref:`Python Path Configuration <init-path-config>` input.

   .. c:member:: wchar_t* pycache_prefix

      Directory where cached ``.pyc`` files are written:
      :data:`sys.pycache_prefix`.

      Set by the :option:`-X pycache_prefix=PATH <-X>` command line option and
      the :envvar:`PYTHONPYCACHEPREFIX` environment variable.
      The command-line option takes precedence.

      If ``NULL``, :data:`sys.pycache_prefix` is set to ``None``.

      Default: ``NULL``.

   .. c:member:: int quiet

      Quiet mode. If greater than ``0``, don't display the copyright and version at
      Python startup in interactive mode.

      Incremented by the :option:`-q` command line option.

      Default: ``0``.

   .. c:member:: wchar_t* run_command

      Value of the :option:`-c` command line option.

      Used by :c:func:`Py_RunMain`.

      Default: ``NULL``.

   .. c:member:: wchar_t* run_filename

      Filename passed on the command line: trailing command line argument
      without :option:`-c` or :option:`-m`. It is used by the
      :c:func:`Py_RunMain` function.

      For example, it is set to ``script.py`` by the ``python3 script.py arg``
      command line.

      See also the :c:member:`PyConfig.skip_source_first_line` option.

      Default: ``NULL``.

   .. c:member:: wchar_t* run_module

      Value of the :option:`-m` command line option.

      Used by :c:func:`Py_RunMain`.

      Default: ``NULL``.

   .. c:member:: wchar_t* run_presite

      ``package.module`` path to module that should be imported before
      ``site.py`` is run.

      Set by the :option:`-X presite=package.module <-X>` command-line
      option and the :envvar:`PYTHON_PRESITE` environment variable.
      The command-line option takes precedence.

      Needs a :ref:`debug build of Python <debug-build>` (the ``Py_DEBUG`` macro
      must be defined).

      Default: ``NULL``.

   .. c:member:: int show_ref_count

      Show total reference count at exit (excluding :term:`immortal` objects)?

      Set to ``1`` by :option:`-X showrefcount <-X>` command line option.

      Needs a :ref:`debug build of Python <debug-build>` (the ``Py_REF_DEBUG``
      macro must be defined).

      Default: ``0``.

   .. c:member:: int site_import

      Import the :mod:`site` module at startup?

      If equal to zero, disable the import of the module site and the
      site-dependent manipulations of :data:`sys.path` that it entails.

      Also disable these manipulations if the :mod:`site` module is explicitly
      imported later (call :func:`site.main` if you want them to be triggered).

      Set to ``0`` by the :option:`-S` command line option.

      :data:`sys.flags.no_site <sys.flags>` is set to the inverted value of
      :c:member:`~PyConfig.site_import`.

      Default: ``1``.

   .. c:member:: int skip_source_first_line

      If non-zero, skip the first line of the :c:member:`PyConfig.run_filename`
      source.

      It allows the usage of non-Unix forms of ``#!cmd``. This is intended for
      a DOS specific hack only.

      Set to ``1`` by the :option:`-x` command line option.

      Default: ``0``.

   .. c:member:: wchar_t* stdio_encoding
   .. c:member:: wchar_t* stdio_errors

      Encoding and encoding errors of :data:`sys.stdin`, :data:`sys.stdout` and
      :data:`sys.stderr` (but :data:`sys.stderr` always uses
      ``"backslashreplace"`` error handler).

      Use the :envvar:`PYTHONIOENCODING` environment variable if it is
      non-empty.

      Default encoding:

      * ``"UTF-8"`` if :c:member:`PyPreConfig.utf8_mode` is non-zero.
      * Otherwise, use the :term:`locale encoding`.

      Default error handler:

      * On Windows: use ``"surrogateescape"``.
      * ``"surrogateescape"`` if :c:member:`PyPreConfig.utf8_mode` is non-zero,
        or if the LC_CTYPE locale is "C" or "POSIX".
      * ``"strict"`` otherwise.

      See also :c:member:`PyConfig.legacy_windows_stdio`.

   .. c:member:: int tracemalloc

      Enable tracemalloc?

      If non-zero, call :func:`tracemalloc.start` at startup.

      Set by :option:`-X tracemalloc=N <-X>` command line option and by the
      :envvar:`PYTHONTRACEMALLOC` environment variable.

      Default: ``-1`` in Python mode, ``0`` in isolated mode.

   .. c:member:: int perf_profiling

      Enable the Linux ``perf`` profiler support?

      If equals to ``1``, enable support for the Linux ``perf`` profiler.

      If equals to ``2``, enable support for the Linux ``perf`` profiler with
      DWARF JIT support.

      Set to ``1`` by :option:`-X perf <-X>` command-line option and the
      :envvar:`PYTHONPERFSUPPORT` environment variable.

      Set to ``2`` by the :option:`-X perf_jit <-X>` command-line option and
      the :envvar:`PYTHON_PERF_JIT_SUPPORT` environment variable.

      Default: ``-1``.

      .. seealso::
         See :ref:`perf_profiling` for more information.

      .. versionadded:: 3.12

   .. c:member:: wchar_t* stdlib_dir

      Directory of the Python standard library.

      Default: ``NULL``.

      .. versionadded:: 3.11

   .. c:member:: int use_environment

      Use :ref:`environment variables <using-on-envvars>`?

      If equals to zero, ignore the :ref:`environment variables
      <using-on-envvars>`.

      Set to ``0`` by the :option:`-E` environment variable.

      Default: ``1`` in Python config and ``0`` in isolated config.

   .. c:member:: int use_system_logger

      If non-zero, ``stdout`` and ``stderr`` will be redirected to the system
      log.

      Only available on macOS 10.12 and later, and on iOS.

      Default: ``0`` (don't use the system log) on macOS; ``1`` on iOS (use the
      system log).

      .. versionadded:: 3.14

   .. c:member:: int user_site_directory

      If non-zero, add the user site directory to :data:`sys.path`.

      Set to ``0`` by the :option:`-s` and :option:`-I` command line options.

      Set to ``0`` by the :envvar:`PYTHONNOUSERSITE` environment variable.

      Default: ``1`` in Python mode, ``0`` in isolated mode.

   .. c:member:: int verbose

      Verbose mode. If greater than ``0``, print a message each time a module is
      imported, showing the place (filename or built-in module) from which
      it is loaded.

      If greater than or equal to ``2``, print a message for each file that is
      checked for when searching for a module. Also provides information on
      module cleanup at exit.

      Incremented by the :option:`-v` command line option.

      Set by the :envvar:`PYTHONVERBOSE` environment variable value.

      Default: ``0``.

   .. c:member:: PyWideStringList warnoptions

      Options of the :mod:`warnings` module to build warnings filters, lowest
      to highest priority: :data:`sys.warnoptions`.

      The :mod:`warnings` module adds :data:`sys.warnoptions` in the reverse
      order: the last :c:member:`PyConfig.warnoptions` item becomes the first
      item of :data:`warnings.filters` which is checked first (highest
      priority).

      The :option:`-W` command line options adds its value to
      :c:member:`~PyConfig.warnoptions`, it can be used multiple times.

      The :envvar:`PYTHONWARNINGS` environment variable can also be used to add
      warning options. Multiple options can be specified, separated by commas
      (``,``).

      Default: empty list.

   .. c:member:: int write_bytecode

      If equal to ``0``, Python won't try to write ``.pyc`` files on the import of
      source modules.

      Set to ``0`` by the :option:`-B` command line option and the
      :envvar:`PYTHONDONTWRITEBYTECODE` environment variable.

      :data:`sys.dont_write_bytecode` is initialized to the inverted value of
      :c:member:`~PyConfig.write_bytecode`.

      Default: ``1``.

   .. c:member:: PyWideStringList xoptions

      Values of the :option:`-X` command line options: :data:`sys._xoptions`.

      Default: empty list.

   .. c:member:: int _pystats

      If non-zero, write performance statistics at Python exit.

      Need a special build with the ``Py_STATS`` macro:
      see :option:`--enable-pystats`.

      Default: ``0``.

If :c:member:`~PyConfig.parse_argv` is non-zero, :c:member:`~PyConfig.argv`
arguments are parsed the same way the regular Python parses :ref:`command line
arguments <using-on-cmdline>`, and Python arguments are stripped from
:c:member:`~PyConfig.argv`.

The :c:member:`~PyConfig.xoptions` options are parsed to set other options: see
the :option:`-X` command line option.

.. versionchanged:: 3.9

   The ``show_alloc_count`` field has been removed.


.. _init-from-config:

Initialization with PyConfig
----------------------------

Initializing the interpreter from a populated configuration struct is handled
by calling :c:func:`Py_InitializeFromConfig`.

The caller is responsible to handle exceptions (error or exit) using
:c:func:`PyStatus_Exception` and :c:func:`Py_ExitStatusException`.

If :c:func:`PyImport_FrozenModules`, :c:func:`PyImport_AppendInittab` or
:c:func:`PyImport_ExtendInittab` are used, they must be set or called after
Python preinitialization and before the Python initialization. If Python is
initialized multiple times, :c:func:`PyImport_AppendInittab` or
:c:func:`PyImport_ExtendInittab` must be called before each Python
initialization.

The current configuration (``PyConfig`` type) is stored in
``PyInterpreterState.config``.

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
            goto exception;
        }

        status = Py_InitializeFromConfig(&config);
        if (PyStatus_Exception(status)) {
            goto exception;
        }
        PyConfig_Clear(&config);
        return;

    exception:
        PyConfig_Clear(&config);
        Py_ExitStatusException(status);
    }

More complete example modifying the default configuration, read the
configuration, and then override some parameters. Note that since
3.11, many parameters are not calculated until initialization, and
so values cannot be read from the configuration structure. Any values
set before initialize is called will be left unchanged by
initialization::

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

        /* Specify sys.path explicitly */
        /* If you want to modify the default set of paths, finish
           initialization first and then use PySys_GetObject("path") */
        config.module_search_paths_set = 1;
        status = PyWideStringList_Append(&config.module_search_paths,
                                         L"/path/to/stdlib");
        if (PyStatus_Exception(status)) {
            goto done;
        }
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

This configuration ignores global configuration variables, environment
variables, command line arguments (:c:member:`PyConfig.argv` is not parsed)
and user site directory. The C standard streams (ex: ``stdout``) and the
LC_CTYPE locale are left unchanged. Signal handlers are not installed.

Configuration files are still used with this configuration to determine
paths that are unspecified. Ensure :c:member:`PyConfig.home` is specified
to avoid computing the default path configuration.


.. _init-python-config:

Python Configuration
--------------------

:c:func:`PyPreConfig_InitPythonConfig` and :c:func:`PyConfig_InitPythonConfig`
functions create a configuration to build a customized Python which behaves as
the regular Python.

Environments variables and command line arguments are used to configure
Python, whereas global configuration variables are ignored.

This function enables C locale coercion (:pep:`538`)
and :ref:`Python UTF-8 Mode <utf8-mode>`
(:pep:`540`) depending on the LC_CTYPE locale, :envvar:`PYTHONUTF8` and
:envvar:`PYTHONCOERCECLOCALE` environment variables.


.. _init-path-config:

Python Path Configuration
-------------------------

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
:c:member:`~PyConfig.module_search_paths_set` is equal to ``0``,
:c:member:`~PyConfig.module_search_paths` is overridden and
:c:member:`~PyConfig.module_search_paths_set` is set to ``1``.

It is possible to completely ignore the function calculating the default
path configuration by setting explicitly all path configuration output
fields listed above. A string is considered as set even if it is non-empty.
``module_search_paths`` is considered as set if
``module_search_paths_set`` is set to ``1``. In this case,
``module_search_paths`` will be used without modification.

Set :c:member:`~PyConfig.pathconfig_warnings` to ``0`` to suppress warnings when
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
* ``._pth`` file (ex: ``python._pth``)
* ``pybuilddir.txt`` (Unix only)

If a ``._pth`` file is present:

* Set :c:member:`~PyConfig.isolated` to ``1``.
* Set :c:member:`~PyConfig.use_environment` to ``0``.
* Set :c:member:`~PyConfig.site_import` to ``0``.
* Set :c:member:`~PyConfig.safe_path` to ``1``.

If :c:member:`~PyConfig.home` is not set and a ``pyvenv.cfg`` file is present in
the same directory as :c:member:`~PyConfig.executable`, or its parent,
:c:member:`~PyConfig.prefix` and :c:member:`~PyConfig.exec_prefix` are set that
location. When this happens, :c:member:`~PyConfig.base_prefix` and
:c:member:`~PyConfig.base_exec_prefix` still keep their value, pointing to the
base installation. See :ref:`sys-path-init-virtual-environments` for more
information.

The ``__PYVENV_LAUNCHER__`` environment variable is used to set
:c:member:`PyConfig.base_executable`.

.. versionchanged:: 3.14

   :c:member:`~PyConfig.prefix`, and :c:member:`~PyConfig.exec_prefix`, are now
   set to the ``pyvenv.cfg`` directory. This was previously done by :mod:`site`,
   therefore affected by :option:`-S`.


Py_GetArgcArgv()
================

.. c:function:: void Py_GetArgcArgv(int *argc, wchar_t ***argv)

   Get the original command line arguments, before Python modified them.

   See also :c:member:`PyConfig.orig_argv` member.

Delaying main module execution
==============================

In some embedding use cases, it may be desirable to separate interpreter initialization
from the execution of the main module.

This separation can be achieved by setting ``PyConfig.run_command`` to the empty
string during initialization (to prevent the interpreter from dropping into the
interactive prompt), and then subsequently executing the desired main module
code using ``__main__.__dict__`` as the global namespace.
