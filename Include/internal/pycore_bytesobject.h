#ifndef Py_INTERNAL_BYTESOBJECT_H
#define Py_INTERNAL_BYTESOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyObject* _PyBytes_FormatEx(
    const char *format,
    Py_ssize_t format_len,
    PyObject *args,
    int use_bytearray);

extern PyObject* _PyBytes_FromHex(
    PyObject *string,
    int use_bytearray);

// Helper for PyBytes_DecodeEscape that detects invalid escape chars.
// Export for test_peg_generator.
PyAPI_FUNC(PyObject*) _PyBytes_DecodeEscape2(const char *, Py_ssize_t,
                                             const char *,
                                             int *, const char **);


// Substring Search.
//
// Returns the index of the first occurrence of
// a substring ("needle") in a larger text ("haystack").
// If the needle is not found, return -1.
// If the needle is found, add offset to the index.
//
// Export for 'mmap' shared extension.
PyAPI_FUNC(Py_ssize_t)
_PyBytes_Find(const char *haystack, Py_ssize_t len_haystack,
              const char *needle, Py_ssize_t len_needle,
              Py_ssize_t offset);

// Same as above, but search right-to-left.
// Export for 'mmap' shared extension.
PyAPI_FUNC(Py_ssize_t)
_PyBytes_ReverseFind(const char *haystack, Py_ssize_t len_haystack,
                     const char *needle, Py_ssize_t len_needle,
                     Py_ssize_t offset);


// Helper function to implement the repeat and inplace repeat methods on a
// buffer.
//
// len_dest is assumed to be an integer multiple of len_src.
// If src equals dest, then assume the operation is inplace.
//
// This method repeately doubles the number of bytes copied to reduce
// the number of invocations of memcpy.
//
// Export for 'array' shared extension.
PyAPI_FUNC(void)
_PyBytes_Repeat(char* dest, Py_ssize_t len_dest,
    const char* src, Py_ssize_t len_src);

/* _PyBytesObject_SIZE gives the basic size of a bytes object; any memory allocation
   for a bytes object of length n should request PyBytesObject_SIZE + n bytes.

   Using _PyBytesObject_SIZE instead of sizeof(PyBytesObject) saves
   3 or 7 bytes per bytes object allocation on a typical system.
*/
#define _PyBytesObject_SIZE (offsetof(PyBytesObject, ob_sval) + 1)

/* --- PyBytesWriter ------------------------------------------------------ */

struct PyBytesWriter {
    char small_buffer[256];
    PyObject *obj;
    Py_ssize_t size;
    int use_bytearray;
    int overallocate;
};

// Export for '_testcapi' shared extension
PyAPI_FUNC(PyBytesWriter*) _PyBytesWriter_CreateByteArray(Py_ssize_t size);

static inline Py_ssize_t
_PyBytesWriter_GetSize(PyBytesWriter *writer)
{
    return writer->size;
}

static inline char*
_PyBytesWriter_GetData(PyBytesWriter *writer)
{
    if (writer->obj == NULL) {
        return writer->small_buffer;
    }
    else if (writer->use_bytearray) {
        return PyByteArray_AS_STRING(writer->obj);
    }
    else {
        return PyBytes_AS_STRING(writer->obj);
    }
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_BYTESOBJECT_H */
