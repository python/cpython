
/* System module interface */

#ifndef Py_SYSMODULE_H
#define Py_SYSMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

DL_IMPORT(PyObject *) PySys_GetObject(char *);
DL_IMPORT(int) PySys_SetObject(char *, PyObject *);
DL_IMPORT(FILE *) PySys_GetFile(char *, FILE *);
DL_IMPORT(void) PySys_SetArgv(int, char **);
DL_IMPORT(void) PySys_SetPath(char *);

DL_IMPORT(void) PySys_WriteStdout(const char *format, ...);
DL_IMPORT(void) PySys_WriteStderr(const char *format, ...);

extern DL_IMPORT(PyObject *) _PySys_TraceFunc, *_PySys_ProfileFunc;
extern DL_IMPORT(int) _PySys_CheckInterval;

#ifdef __cplusplus
}
#endif
#endif /* !Py_SYSMODULE_H */
