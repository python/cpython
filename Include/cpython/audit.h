#ifndef _Py_CPYTHON_AUDIT_H
#  error "this header file must not be included directly"
#endif


typedef int(*Py_AuditHookFunction)(const char *, PyObject *, void *);

PyAPI_FUNC(int) PySys_AddAuditHook(Py_AuditHookFunction, void*);
