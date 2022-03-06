/* Declarations shared between the different POSIX-related modules */

#ifndef Py_POSIXMODULE_H
#define Py_POSIXMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef Py_LIMITED_API
#ifndef MS_WINDOWS
PyAPI_FUNC(PyObject *) _PyLong_FromUid(uid_t);
PyAPI_FUNC(PyObject *) _PyLong_FromGid(gid_t);
PyAPI_FUNC(int) _Py_Uid_Converter(PyObject *, uid_t *);
PyAPI_FUNC(int) _Py_Gid_Converter(PyObject *, gid_t *);
#endif /* MS_WINDOWS */

#if defined(PYPTHREAD_SIGMASK) || defined(HAVE_SIGWAIT) || \
        defined(HAVE_SIGWAITINFO) || defined(HAVE_SIGTIMEDWAIT)
# define HAVE_SIGSET_T
#endif

#ifdef HAVE_SIGSET_T
PyAPI_FUNC(int) _Py_Sigset_Converter(PyObject *, void *);
#endif /* HAVE_SIGSET_T */
#endif /* Py_LIMITED_API */

#ifdef __cplusplus
}
#endif
#endif /* !Py_POSIXMODULE_H */
