#include "hashlib_buffer.h"

int
_Py_hashlib_data_argument(PyObject **res, PyObject *data, PyObject *string)
{
    if (data != NULL && string == NULL) {
        // called as H(data) or H(data=...)
        *res = data;
        return 1;
    }
    else if (data == NULL && string != NULL) {
        // called as H(string=...)
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                         "the 'string' keyword parameter is deprecated since "
                         "Python 3.15 and slated for removal in Python 3.19; "
                         "use the 'data' keyword parameter or pass the data "
                         "to hash as a positional argument instead", 1) < 0)
        {
            *res = NULL;
            return -1;
        }
        *res = string;
        return 1;
    }
    else if (data == NULL && string == NULL) {
        // fast path when no data is given
        assert(!PyErr_Occurred());
        *res = NULL;
        return 0;
    }
    else {
        // called as H(data=..., string)
        *res = NULL;
        PyErr_SetString(PyExc_TypeError,
                        "'data' and 'string' are mutually exclusive "
                        "and support for 'string' keyword parameter "
                        "is slated for removal in a future version.");
        return -1;
    }
}

int
_Py_hashlib_get_buffer_view(PyObject *obj, Py_buffer *view)
{
    if (PyUnicode_Check(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "Strings must be encoded before hashing");
        return -1;
    }
    if (!PyObject_CheckBuffer(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "object supporting the buffer API required");
        return -1;
    }
    if (PyObject_GetBuffer(obj, view, PyBUF_SIMPLE) == -1) {
        return -1;
    }
    if (view->ndim > 1) {
        PyErr_SetString(PyExc_BufferError,
                        "Buffer must be single dimension");
        PyBuffer_Release(view);
        return -1;
    }
    return 0;
}
