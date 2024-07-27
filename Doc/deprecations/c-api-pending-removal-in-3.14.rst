Pending Removal in Python 3.14
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* The ``ma_version_tag`` field in :c:type:`PyDictObject` for extension modules
  (:pep:`699`; :gh:`101193`).

* Creating :c:data:`immutable types <Py_TPFLAGS_IMMUTABLETYPE>` with mutable
  bases (:gh:`95388`).

* Functions to configure Python's initialization, deprecated in Python 3.11:

  * ``PySys_SetArgvEx()``: set :c:member:`PyConfig.argv`
  * ``PySys_SetArgv()``: set :c:member:`PyConfig.argv`
  * ``Py_SetProgramName()``: set :c:member:`PyConfig.program_name`
  * ``Py_SetPythonHome()``: set :c:member:`PyConfig.home`

  The :c:func:`Py_InitializeFromConfig` API should be used with
  :c:type:`PyConfig` instead.

* Global configuration variables:

  * :c:var:`Py_DebugFlag`: use :c:member:`PyConfig.parser_debug`
  * :c:var:`Py_VerboseFlag`: use :c:member:`PyConfig.verbose`
  * :c:var:`Py_QuietFlag`: use :c:member:`PyConfig.quiet`
  * :c:var:`Py_InteractiveFlag`: use :c:member:`PyConfig.interactive`
  * :c:var:`Py_InspectFlag`: use :c:member:`PyConfig.inspect`
  * :c:var:`Py_OptimizeFlag`: use :c:member:`PyConfig.optimization_level`
  * :c:var:`Py_NoSiteFlag`: use :c:member:`PyConfig.site_import`
  * :c:var:`Py_BytesWarningFlag`: use :c:member:`PyConfig.bytes_warning`
  * :c:var:`Py_FrozenFlag`: use :c:member:`PyConfig.pathconfig_warnings`
  * :c:var:`Py_IgnoreEnvironmentFlag`: use :c:member:`PyConfig.use_environment`
  * :c:var:`Py_DontWriteBytecodeFlag`: use :c:member:`PyConfig.write_bytecode`
  * :c:var:`Py_NoUserSiteDirectory`: use :c:member:`PyConfig.user_site_directory`
  * :c:var:`Py_UnbufferedStdioFlag`: use :c:member:`PyConfig.buffered_stdio`
  * :c:var:`Py_HashRandomizationFlag`: use :c:member:`PyConfig.use_hash_seed`
    and :c:member:`PyConfig.hash_seed`
  * :c:var:`Py_IsolatedFlag`: use :c:member:`PyConfig.isolated`
  * :c:var:`Py_LegacyWindowsFSEncodingFlag`: use :c:member:`PyPreConfig.legacy_windows_fs_encoding`
  * :c:var:`Py_LegacyWindowsStdioFlag`: use :c:member:`PyConfig.legacy_windows_stdio`
  * :c:var:`!Py_FileSystemDefaultEncoding`: use :c:member:`PyConfig.filesystem_encoding`
  * :c:var:`!Py_HasFileSystemDefaultEncoding`: use :c:member:`PyConfig.filesystem_encoding`
  * :c:var:`!Py_FileSystemDefaultEncodeErrors`: use :c:member:`PyConfig.filesystem_errors`
  * :c:var:`!Py_UTF8Mode`: use :c:member:`PyPreConfig.utf8_mode` (see :c:func:`Py_PreInitialize`)

  The :c:func:`Py_InitializeFromConfig` API should be used with
  :c:type:`PyConfig` instead.
