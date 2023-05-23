#ifndef Py_LIMITED_API
#ifndef Py_PYDEBUG_H
#define Py_PYDEBUG_H
#ifdef __cplusplus
extern "C" {
#endif

Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_DebugFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_VerboseFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_QuietFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_InteractiveFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_InspectFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_OptimizeFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_NoSiteFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_BytesWarningFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_FrozenFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_IgnoreEnvironmentFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_DontWriteBytecodeFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_NoUserSiteDirectory;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_UnbufferedStdioFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_HashRandomizationFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_IsolatedFlag;

#ifdef MS_WINDOWS
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_LegacyWindowsFSEncodingFlag;
Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_LegacyWindowsStdioFlag;
#endif

/* this is a wrapper around getenv() that pays attention to
   Py_IgnoreEnvironmentFlag.  It should be used for getting variables like
   PYTHONPATH and PYTHONHOME from the environment */
PyAPI_FUNC(char*) Py_GETENV(const char *name);

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYDEBUG_H */
#endif /* Py_LIMITED_API */
