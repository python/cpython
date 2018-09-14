/* Frozen python modules */

#ifndef Py_CHILLEDMODULES_H
#define Py_CHILLEDMODULES_H
#ifdef __cplusplus
extern "C" {
#endif


extern void _PyFrozenModules_Init(void);
extern PyObject* _PyFrozenModule_Lookup(PyObject* name);
extern PyObject* _PyFrozenModule_GetCode(PyObject* name, int *needs_path);
extern void _PyFrozenModules_Finalize(void);
extern void _PyFrozenModules_Disable(void);
extern void _PyFrozenModules_Enable(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_CHILLEDMODULES_H */
