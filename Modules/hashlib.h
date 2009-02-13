/* Common code for use by all hashlib related modules. */

/*
 * Given a PyObject* obj, fill in the Py_buffer* viewp with the result
 * of PyObject_GetBuffer.  Sets and exception and issues a returns
 * on any errors.
 */
#define GET_BUFFER_VIEW_OR_ERROUT(obj, viewp, error_return) do { \
        if (PyUnicode_Check((obj))) { \
            PyErr_SetString(PyExc_TypeError, \
                            "Unicode-objects must be encoded before hashing");\
            return error_return; \
        } \
        if (!PyObject_CheckBuffer((obj))) { \
            PyErr_SetString(PyExc_TypeError, \
                            "object supporting the buffer API required"); \
            return error_return; \
        } \
        if (PyObject_GetBuffer((obj), (viewp), PyBUF_SIMPLE) == -1) { \
            return error_return; \
        } \
        if ((viewp)->ndim > 1) { \
            PyErr_SetString(PyExc_BufferError, \
                            "Buffer must be single dimension"); \
            PyBuffer_Release((viewp)); \
            return error_return; \
        } \
    } while(0);
