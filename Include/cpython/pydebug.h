#ifndef Py_LIMITED_API
#ifndef Py_PYDEBUG_H
#define Py_PYDEBUG_H
#ifdef __cplusplus
extern "C" {
#endif

Py_DEPRECATED(3.12) PyAPI_DATA(int) Py_InteractiveFlag;

/* this is a wrapper around getenv() that pays attention to
   PyConfig.use_environment.  It should be used for getting variables like
   PYTHONPATH and PYTHONHOME from the environment */
PyAPI_FUNC(char*) Py_GETENV(const char *name);

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYDEBUG_H */
#endif /* Py_LIMITED_API */
