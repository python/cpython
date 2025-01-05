Pending Removal in Python 3.14
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* The ``ma_version_tag`` field in :c:type:`PyDictObject` for extension modules
  (:pep:`699`; :gh:`101193`).

* Creating :c:data:`immutable types <Py_TPFLAGS_IMMUTABLETYPE>` with mutable
  bases (:gh:`95388`).

* Functions to configure Python's initialization, deprecated in Python 3.11:

  * :c:func:`!PySys_SetArgvEx()`:
    Set :c:member:`PyConfig.argv` instead.
  * :c:func:`!PySys_SetArgv()`:
    Set :c:member:`PyConfig.argv` instead.
  * :c:func:`!Py_SetProgramName()`:
    Set :c:member:`PyConfig.program_name` instead.
  * :c:func:`!Py_SetPythonHome()`:
    Set :c:member:`PyConfig.home` instead.

  The :c:func:`Py_InitializeFromConfig` API should be used with
  :c:type:`PyConfig` instead.

* Global configuration variables:

  * :c:var:`Py_DebugFlag`:
    Use :c:member:`PyConfig.parser_debug` instead.
  * :c:var:`Py_VerboseFlag`:
    Use :c:member:`PyConfig.verbose` instead.
  * :c:var:`Py_QuietFlag`:
    Use :c:member:`PyConfig.quiet` instead.
  * :c:var:`Py_InteractiveFlag`:
    Use :c:member:`PyConfig.interactive` instead.
  * :c:var:`Py_InspectFlag`:
    Use :c:member:`PyConfig.inspect` instead.
  * :c:var:`Py_OptimizeFlag`:
    Use :c:member:`PyConfig.optimization_level` instead.
  * :c:var:`Py_NoSiteFlag`:
    Use :c:member:`PyConfig.site_import` instead.
  * :c:var:`Py_BytesWarningFlag`:
    Use :c:member:`PyConfig.bytes_warning` instead.
  * :c:var:`Py_FrozenFlag`:
    Use :c:member:`PyConfig.pathconfig_warnings` instead.
  * :c:var:`Py_IgnoreEnvironmentFlag`:
    Use :c:member:`PyConfig.use_environment` instead.
  * :c:var:`Py_DontWriteBytecodeFlag`:
    Use :c:member:`PyConfig.write_bytecode` instead.
  * :c:var:`Py_NoUserSiteDirectory`:
    Use :c:member:`PyConfig.user_site_directory` instead.
  * :c:var:`Py_UnbufferedStdioFlag`:
    Use :c:member:`PyConfig.buffered_stdio` instead.
  * :c:var:`Py_HashRandomizationFlag`:
    Use :c:member:`PyConfig.use_hash_seed`
    and :c:member:`PyConfig.hash_seed` instead.
  * :c:var:`Py_IsolatedFlag`:
    Use :c:member:`PyConfig.isolated` instead.
  * :c:var:`Py_LegacyWindowsFSEncodingFlag`:
    Use :c:member:`PyPreConfig.legacy_windows_fs_encoding` instead.
  * :c:var:`Py_LegacyWindowsStdioFlag`:
    Use :c:member:`PyConfig.legacy_windows_stdio` instead.
  * :c:var:`!Py_FileSystemDefaultEncoding`:
    Use :c:member:`PyConfig.filesystem_encoding` instead.
  * :c:var:`!Py_HasFileSystemDefaultEncoding`:
    Use :c:member:`PyConfig.filesystem_encoding` instead.
  * :c:var:`!Py_FileSystemDefaultEncodeErrors`:
    Use :c:member:`PyConfig.filesystem_errors` instead.
  * :c:var:`!Py_UTF8Mode`:
    Use :c:member:`PyPreConfig.utf8_mode` instead.
    (see :c:func:`Py_PreInitialize`)

  The :c:func:`Py_InitializeFromConfig` API should be used with
  :c:type:`PyConfig` instead.
