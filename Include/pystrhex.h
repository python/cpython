#ifndef Py_STRHEX_H
#define Py_STRHEX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Returns a str() containing the hex representation of argbuf. */
PyAPI_FUNC(PyObject*) _Py_strhex(const char* argbuf, const Py_ssize_t arglen);
/* Returns a bytes() containing the ASCII hex representation of argbuf. */
PyAPI_FUNC(PyObject*) _Py_strhex_bytes(const char* argbuf, const Py_ssize_t arglen);

#ifdef __cplusplus
}
#endif

#endif /* !Py_STRHEX_H */
