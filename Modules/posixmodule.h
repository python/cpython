/* Declarations shared between the different POSIX-related modules */

#ifndef Py_POSIXMODULE_H
#define Py_POSIXMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef MS_WINDOWS
PyAPI_FUNC(PyObject *) _PyInt_FromUid(uid_t);
PyAPI_FUNC(PyObject *) _PyInt_FromGid(gid_t);
PyAPI_FUNC(int) _Py_Uid_Converter(PyObject *, void *);
PyAPI_FUNC(int) _Py_Gid_Converter(PyObject *, void *);
#endif /* MS_WINDOWS */

#ifdef __cplusplus
}
#endif
#endif /* !Py_POSIXMODULE_H */
