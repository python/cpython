#ifndef Py_LIMITED_API
#ifndef Py_PYDEBUG_H
#define Py_PYDEBUG_H
#ifdef __cplusplus
extern "C" {
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
