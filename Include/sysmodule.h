
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

DL_IMPORT(void) PySys_WriteStdout(const char *format, ...)
			__attribute__((format(printf, 1, 2)));
DL_IMPORT(void) PySys_WriteStderr(const char *format, ...)
			__attribute__((format(printf, 1, 2)));

extern DL_IMPORT(PyObject *) _PySys_TraceFunc, *_PySys_ProfileFunc;
extern DL_IMPORT(int) _PySys_CheckInterval;

DL_IMPORT(void) PySys_ResetWarnOptions(void);
DL_IMPORT(void) PySys_AddWarnOption(char *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_SYSMODULE_H */
