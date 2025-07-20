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

/* --- _PyBytesWriter ----------------------------------------------------- */

/* The _PyBytesWriter structure is big: it contains an embedded "stack buffer".
   A _PyBytesWriter variable must be declared at the end of variables in a
   function to optimize the memory allocation on the stack. */
typedef struct {
    /* bytes, bytearray or NULL (when the small buffer is used) */
    PyObject *buffer;

    /* Number of allocated size. */
    Py_ssize_t allocated;

    /* Minimum number of allocated bytes,
       incremented by _PyBytesWriter_Prepare() */
    Py_ssize_t min_size;

    /* If non-zero, use a bytearray instead of a bytes object for buffer. */
    int use_bytearray;

    /* If non-zero, overallocate the buffer (default: 0).
       This flag must be zero if use_bytearray is non-zero. */
    int overallocate;

    /* Stack buffer */
    int use_small_buffer;
    char small_buffer[512];
} _PyBytesWriter;

/* Initialize a bytes writer

   By default, the overallocation is disabled. Set the overallocate attribute
   to control the allocation of the buffer.

   Export _PyBytesWriter API for '_pickle' shared extension. */
PyAPI_FUNC(void) _PyBytesWriter_Init(_PyBytesWriter *writer);

/* Get the buffer content and reset the writer.
   Return a bytes object, or a bytearray object if use_bytearray is non-zero.
   Raise an exception and return NULL on error. */
PyAPI_FUNC(PyObject *) _PyBytesWriter_Finish(_PyBytesWriter *writer,
    void *str);

/* Deallocate memory of a writer (clear its internal buffer). */
PyAPI_FUNC(void) _PyBytesWriter_Dealloc(_PyBytesWriter *writer);

/* Allocate the buffer to write size bytes.
   Return the pointer to the beginning of buffer data.
   Raise an exception and return NULL on error. */
PyAPI_FUNC(void*) _PyBytesWriter_Alloc(_PyBytesWriter *writer,
    Py_ssize_t size);

/* Ensure that the buffer is large enough to write *size* bytes.
   Add size to the writer minimum size (min_size attribute).

   str is the current pointer inside the buffer.
   Return the updated current pointer inside the buffer.
   Raise an exception and return NULL on error. */
PyAPI_FUNC(void*) _PyBytesWriter_Prepare(_PyBytesWriter *writer,
    void *str,
    Py_ssize_t size);

/* Resize the buffer to make it larger.
   The new buffer may be larger than size bytes because of overallocation.
   Return the updated current pointer inside the buffer.
   Raise an exception and return NULL on error.

   Note: size must be greater than the number of allocated bytes in the writer.

   This function doesn't use the writer minimum size (min_size attribute).

   See also _PyBytesWriter_Prepare().
   */
PyAPI_FUNC(void*) _PyBytesWriter_Resize(_PyBytesWriter *writer,
    void *str,
    Py_ssize_t size);

/* Write bytes.
   Raise an exception and return NULL on error. */
PyAPI_FUNC(void*) _PyBytesWriter_WriteBytes(_PyBytesWriter *writer,
    void *str,
    const void *bytes,
    Py_ssize_t size);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_BYTESOBJECT_H */
