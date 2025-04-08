/* Declarations shared between the different POSIX-related modules */

#ifndef Py_POSIXMODULE_H
#define Py_POSIXMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>          // uid_t
#endif

#ifndef MS_WINDOWS
extern PyObject* _PyLong_FromUid(uid_t);

// Export for 'grp' shared extension
PyAPI_FUNC(PyObject*) _PyLong_FromGid(gid_t);

// Export for '_posixsubprocess' shared extension
PyAPI_FUNC(int) _Py_Uid_Converter(PyObject *, uid_t *);

// Export for 'grp' shared extension
PyAPI_FUNC(int) _Py_Gid_Converter(PyObject *, gid_t *);
#endif   // !MS_WINDOWS

#if (defined(PYPTHREAD_SIGMASK) || defined(HAVE_SIGWAIT) \
     || defined(HAVE_SIGWAITINFO) || defined(HAVE_SIGTIMEDWAIT))
#  define HAVE_SIGSET_T
#endif

extern int _Py_Sigset_Converter(PyObject *, void *);

#ifdef __cplusplus
}
#endif
#endif   // !Py_POSIXMODULE_H
