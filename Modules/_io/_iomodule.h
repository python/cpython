/*
 * Declarations shared between the different parts of the io module
 */

#include "exports.h"

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

#ifndef Py_LIMITED_API
#ifdef MS_WINDOWS
extern PyTypeObject PyWindowsConsoleIO_Type;
PyAPI_DATA(PyObject *) _PyWindowsConsoleIO_Type;
#define PyWindowsConsoleIO_Check(op) (PyObject_TypeCheck((op), (PyTypeObject*)_PyWindowsConsoleIO_Type))
#endif /* MS_WINDOWS */
#endif /* Py_LIMITED_API */

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
   Otherwise, the function will scan further and return garbage.

   There are three modes, in order of priority:
   * translated: Only find \n (assume newlines already translated)
   * universal: Use universal newlines algorithm
   * Otherwise, the line ending is specified by readnl, a str object */
extern Py_ssize_t _PyIO_find_line_ending(
    int translated, int universal, PyObject *readnl,
    int kind, const char *start, const char *end, Py_ssize_t *consumed);

/* Return 1 if an OSError with errno == EINTR is set (and then
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
typedef long long Py_off_t;
# define PyLong_AsOff_t     PyLong_AsLongLong
# define PyLong_FromOff_t   PyLong_FromLongLong
# define PY_OFF_T_MAX       LLONG_MAX
# define PY_OFF_T_MIN       LLONG_MIN
# define PY_OFF_T_COMPAT    long long    /* type compatible with off_t */
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
#elif (SIZEOF_OFF_T == SIZEOF_LONG_LONG)
# define PyLong_AsOff_t     PyLong_AsLongLong
# define PyLong_FromOff_t   PyLong_FromLongLong
# define PY_OFF_T_MAX       LLONG_MAX
# define PY_OFF_T_MIN       LLONG_MIN
# define PY_OFF_T_COMPAT    long long
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

#ifdef MS_WINDOWS
extern char _PyIO_get_console_type(PyObject *);
#endif

extern Py_EXPORTED_SYMBOL PyTypeObject _PyBytesIOBuffer_Type;
