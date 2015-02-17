/*
 * Declarations shared between the different parts of the io module
 */

/* ABCs */
extern PyTypeObject PyIOBase_Type;
extern PyTypeObject PyRawIOBase_Type;
extern PyTypeObject PyBufferedIOBase_Type;
extern PyTypeObject PyTextIOBase_Type;

/* Concrete classes */
extern PyTypeObject PyFileIO_Type;
extern PyTypeObject PyBytesIO_Type;
extern PyTypeObject PyStringIO_Type;
extern PyTypeObject PyBufferedReader_Type;
extern PyTypeObject PyBufferedWriter_Type;
extern PyTypeObject PyBufferedRWPair_Type;
extern PyTypeObject PyBufferedRandom_Type;
extern PyTypeObject PyTextIOWrapper_Type;
extern PyTypeObject PyIncrementalNewlineDecoder_Type;


extern int _PyIO_ConvertSsize_t(PyObject *, void *);

/* These functions are used as METH_NOARGS methods, are normally called
 * with args=NULL, and return a new reference.
 * BUT when args=Py_True is passed, they return a borrowed reference.
 */
extern PyObject* _PyIOBase_check_readable(PyObject *self, PyObject *args);
extern PyObject* _PyIOBase_check_writable(PyObject *self, PyObject *args);
extern PyObject* _PyIOBase_check_seekable(PyObject *self, PyObject *args);
extern PyObject* _PyIOBase_check_closed(PyObject *self, PyObject *args);

/* Helper for finalization.
   This function will revive an object ready to be deallocated and try to
   close() it. It returns 0 if the object can be destroyed, or -1 if it
   is alive again. */
extern int _PyIOBase_finalize(PyObject *self);

/* Returns true if the given FileIO object is closed.
   Doesn't check the argument type, so be careful! */
extern int _PyFileIO_closed(PyObject *self);

/* Shortcut to the core of the IncrementalNewlineDecoder.decode method */
extern PyObject *_PyIncrementalNewlineDecoder_decode(
    PyObject *self, PyObject *input, int final);

/* Finds the first line ending between `start` and `end`.
   If found, returns the index after the line ending and doesn't touch
   `*consumed`.
   If not found, returns -1 and sets `*consumed` to the number of characters
   which can be safely put aside until another search.

   NOTE: for performance reasons, `end` must point to a NUL character ('\0').
   Otherwise, the function will scan further and return garbage. */
extern Py_ssize_t _PyIO_find_line_ending(
    int translated, int universal, PyObject *readnl,
    int kind, char *start, char *end, Py_ssize_t *consumed);

/* Return 1 if an EnvironmentError with errno == EINTR is set (and then
   clears the error indicator), 0 otherwise.
   Should only be called when PyErr_Occurred() is true.
*/
extern int _PyIO_trap_eintr(void);

#define DEFAULT_BUFFER_SIZE (8 * 1024)  /* bytes */

/*
 * Offset type for positioning.
 */

/* Printing a variable of type off_t (with e.g., PyUnicode_FromFormat)
   correctly and without producing compiler warnings is surprisingly painful.
   We identify an integer type whose size matches off_t and then: (1) cast the
   off_t to that integer type and (2) use the appropriate conversion
   specification.  The cast is necessary: gcc complains about formatting a
   long with "%lld" even when both long and long long have the same
   precision. */

#ifdef MS_WINDOWS

/* Windows uses long long for offsets */
typedef PY_LONG_LONG Py_off_t;
# define PyLong_AsOff_t     PyLong_AsLongLong
# define PyLong_FromOff_t   PyLong_FromLongLong
# define PY_OFF_T_MAX       PY_LLONG_MAX
# define PY_OFF_T_MIN       PY_LLONG_MIN
# define PY_OFF_T_COMPAT    PY_LONG_LONG /* type compatible with off_t */
# define PY_PRIdOFF         "lld"        /* format to use for that type */

#else

/* Other platforms use off_t */
typedef off_t Py_off_t;
#if (SIZEOF_OFF_T == SIZEOF_SIZE_T)
# define PyLong_AsOff_t     PyLong_AsSsize_t
# define PyLong_FromOff_t   PyLong_FromSsize_t
# define PY_OFF_T_MAX       PY_SSIZE_T_MAX
# define PY_OFF_T_MIN       PY_SSIZE_T_MIN
# define PY_OFF_T_COMPAT    Py_ssize_t
# define PY_PRIdOFF         "zd"
#elif (HAVE_LONG_LONG && SIZEOF_OFF_T == SIZEOF_LONG_LONG)
# define PyLong_AsOff_t     PyLong_AsLongLong
# define PyLong_FromOff_t   PyLong_FromLongLong
# define PY_OFF_T_MAX       PY_LLONG_MAX
# define PY_OFF_T_MIN       PY_LLONG_MIN
# define PY_OFF_T_COMPAT    PY_LONG_LONG
# define PY_PRIdOFF         "lld"
#elif (SIZEOF_OFF_T == SIZEOF_LONG)
# define PyLong_AsOff_t     PyLong_AsLong
# define PyLong_FromOff_t   PyLong_FromLong
# define PY_OFF_T_MAX       LONG_MAX
# define PY_OFF_T_MIN       LONG_MIN
# define PY_OFF_T_COMPAT    long
# define PY_PRIdOFF         "ld"
#else
# error off_t does not match either size_t, long, or long long!
#endif

#endif

extern Py_off_t PyNumber_AsOff_t(PyObject *item, PyObject *err);

/* Implementation details */

/* IO module structure */

extern PyModuleDef _PyIO_Module;

typedef struct {
    int initialized;
    PyObject *locale_module;

    PyObject *unsupported_operation;
} _PyIO_State;

#define IO_MOD_STATE(mod) ((_PyIO_State *)PyModule_GetState(mod))
#define IO_STATE() _PyIO_get_module_state()

extern _PyIO_State *_PyIO_get_module_state(void);
extern PyObject *_PyIO_get_locale_module(_PyIO_State *);

extern PyObject *_PyIO_str_close;
extern PyObject *_PyIO_str_closed;
extern PyObject *_PyIO_str_decode;
extern PyObject *_PyIO_str_encode;
extern PyObject *_PyIO_str_fileno;
extern PyObject *_PyIO_str_flush;
extern PyObject *_PyIO_str_getstate;
extern PyObject *_PyIO_str_isatty;
extern PyObject *_PyIO_str_newlines;
extern PyObject *_PyIO_str_nl;
extern PyObject *_PyIO_str_read;
extern PyObject *_PyIO_str_read1;
extern PyObject *_PyIO_str_readable;
extern PyObject *_PyIO_str_readall;
extern PyObject *_PyIO_str_readinto;
extern PyObject *_PyIO_str_readline;
extern PyObject *_PyIO_str_reset;
extern PyObject *_PyIO_str_seek;
extern PyObject *_PyIO_str_seekable;
extern PyObject *_PyIO_str_setstate;
extern PyObject *_PyIO_str_tell;
extern PyObject *_PyIO_str_truncate;
extern PyObject *_PyIO_str_writable;
extern PyObject *_PyIO_str_write;

extern PyObject *_PyIO_empty_str;
extern PyObject *_PyIO_empty_bytes;
extern PyObject *_PyIO_zero;

extern PyTypeObject _PyBytesIOBuffer_Type;
