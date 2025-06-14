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
