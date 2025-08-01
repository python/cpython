#ifndef Py_PYABIINFO_H
#define Py_PYABIINFO_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030F0000
PyAPI_FUNC(PyObject *) PyAbiInfo_GetInfo(void);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYABIINFO_H */
