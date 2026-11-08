
/* os module interface */

#ifndef Py_OSMODULE_H
#define Py_OSMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= _Py_PACK_VERSION(3, 6)
PyAPI_FUNC(PyObject *) PyOS_FSPath(PyObject *path);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_OSMODULE_H */
