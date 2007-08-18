/* Bytes object interface */

#ifndef Py_BYTESOBJECT_H
#define Py_BYTESOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

/* Type PyBytesObject represents a mutable array of bytes.
 * The Python API is that of a sequence;
 * the bytes are mapped to ints in [0, 256).
 * Bytes are not characters; they may be used to encode characters.
 * The only way to go between bytes and str/unicode is via encoding
 * and decoding.
 * For the concenience of C programmers, the bytes type is considered
 * to contain a char pointer, not an unsigned char pointer.
 */

/* Object layout */
typedef struct {
    PyObject_VAR_HEAD
    int ob_exports; /* how many buffer exports */
    Py_ssize_t ob_alloc; /* How many bytes allocated */
    char *ob_bytes;
} PyBytesObject;

/* Type object */
PyAPI_DATA(PyTypeObject) PyBytes_Type;

/* Type check macros */
#define PyBytes_Check(self) PyObject_TypeCheck(self, &PyBytes_Type)
#define PyBytes_CheckExact(self) (Py_Type(self) == &PyBytes_Type)

/* Direct API functions */
PyAPI_FUNC(PyObject *) PyBytes_FromObject(PyObject *);
PyAPI_FUNC(PyObject *) PyBytes_Concat(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyBytes_FromStringAndSize(const char *, Py_ssize_t);
PyAPI_FUNC(Py_ssize_t) PyBytes_Size(PyObject *);
PyAPI_FUNC(char *) PyBytes_AsString(PyObject *);
PyAPI_FUNC(int) PyBytes_Resize(PyObject *, Py_ssize_t);

/* Macros, trading safety for speed */
#define PyBytes_AS_STRING(self) (assert(PyBytes_Check(self)),((PyBytesObject *)(self))->ob_bytes)
#define PyBytes_GET_SIZE(self)  (assert(PyBytes_Check(self)),Py_Size(self))

#ifdef __cplusplus
}
#endif
#endif /* !Py_BYTESOBJECT_H */
