/*
 * History: First version dated from 3/97, derived from my SCMLIB version
 * for win16.
 */
/*
 * Related Work:
 *      - calldll       http://www.nightmare.com/software.html
 *      - libffi        http://sourceware.cygnus.com/libffi/
 *      - ffcall        http://clisp.cons.org/~haible/packages-ffcall.html
 *   and, of course, Don Beaudry's MESS package, but this is more ctypes
 *   related.
 */


/*
  How are functions called, and how are parameters converted to C ?

  1. _ctypes.c::PyCFuncPtr_call receives an argument tuple 'inargs' and a
  keyword dictionary 'kwds'.

  2. After several checks, _build_callargs() is called which returns another
  tuple 'callargs'.  This may be the same tuple as 'inargs', a slice of
  'inargs', or a completely fresh tuple, depending on several things (is it a
  COM method?, are 'paramflags' available?).

  3. _build_callargs also calculates bitarrays containing indexes into
  the callargs tuple, specifying how to build the return value(s) of
  the function.

  4. _ctypes_callproc is then called with the 'callargs' tuple.  _ctypes_callproc first
  allocates two arrays.  The first is an array of 'struct argument' items, the
  second array has 'void *' entries.

  5. If 'converters' are present (converters is a sequence of argtypes'
  from_param methods), for each item in 'callargs' converter is called and the
  result passed to ConvParam.  If 'converters' are not present, each argument
  is directly passed to ConvParm.

  6. For each arg, ConvParam stores the contained C data (or a pointer to it,
  for structures) into the 'struct argument' array.

  7. Finally, a loop fills the 'void *' array so that each item points to the
  data contained in or pointed to by the 'struct argument' array.

  8. The 'void *' argument array is what _call_function_pointer
  expects. _call_function_pointer then has very little to do - only some
  libffi specific stuff, then it calls ffi_call.

  So, there are 4 data structures holding processed arguments:
  - the inargs tuple (in PyCFuncPtr_call)
  - the callargs tuple (in PyCFuncPtr_call)
  - the 'struct arguments' array
  - the 'void *' array

 */

#include "Python.h"
#include "structmember.h"         // PyMemberDef

#ifdef MS_WIN32
#include <windows.h>
#include <tchar.h>
#else
#include "ctypes_dlfcn.h"
#endif

#ifdef MS_WIN32
#include <malloc.h>
#endif

#include <ffi.h>
#include "ctypes.h"
#ifdef HAVE_ALLOCA_H
/* AIX needs alloca.h for alloca() */
#include <alloca.h>
#endif

#ifdef _Py_MEMORY_SANITIZER
#include <sanitizer/msan_interface.h>
#endif

#if defined(_DEBUG) || defined(__MINGW32__)
/* Don't use structured exception handling on Windows if this is defined.
   MingW, AFAIK, doesn't support it.
*/
#define DONT_USE_SEH
#endif

#define CTYPES_CAPSULE_NAME_PYMEM "_ctypes pymem"

static void pymem_destructor(PyObject *ptr)
{
    void *p = PyCapsule_GetPointer(ptr, CTYPES_CAPSULE_NAME_PYMEM);
    if (p) {
        PyMem_Free(p);
    }
}

/*
  ctypes maintains thread-local storage that has space for two error numbers:
  private copies of the system 'errno' value and, on Windows, the system error code
  accessed by the GetLastError() and SetLastError() api functions.

  Foreign functions created with CDLL(..., use_errno=True), when called, swap
  the system 'errno' value with the private copy just before the actual
  function call, and swapped again immediately afterwards.  The 'use_errno'
  parameter defaults to False, in this case 'ctypes_errno' is not touched.

  On Windows, foreign functions created with CDLL(..., use_last_error=True) or
  WinDLL(..., use_last_error=True) swap the system LastError value with the
  ctypes private copy.

  The values are also swapped immediately before and after ctypes callback
  functions are called, if the callbacks are constructed using the new
  optional use_errno parameter set to True: CFUNCTYPE(..., use_errno=TRUE) or
  WINFUNCTYPE(..., use_errno=True).

  New ctypes functions are provided to access the ctypes private copies from
  Python:

  - ctypes.set_errno(value) and ctypes.set_last_error(value) store 'value' in
    the private copy and returns the previous value.

  - ctypes.get_errno() and ctypes.get_last_error() returns the current ctypes
    private copies value.
*/

/*
  This function creates and returns a thread-local Python object that has
  space to store two integer error numbers; once created the Python object is
  kept alive in the thread state dictionary as long as the thread itself.
*/
PyObject *
_ctypes_get_errobj(int **pspace)
{
    PyObject *dict = PyThreadState_GetDict();
    PyObject *errobj;
    static PyObject *error_object_name;
    if (dict == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot get thread state");
        return NULL;
    }
    if (error_object_name == NULL) {
        error_object_name = PyUnicode_InternFromString("ctypes.error_object");
        if (error_object_name == NULL)
            return NULL;
    }
    errobj = PyDict_GetItemWithError(dict, error_object_name);
    if (errobj) {
        if (!PyCapsule_IsValid(errobj, CTYPES_CAPSULE_NAME_PYMEM)) {
            PyErr_SetString(PyExc_RuntimeError,
                "ctypes.error_object is an invalid capsule");
            return NULL;
        }
        Py_INCREF(errobj);
    }
    else if (!PyErr_Occurred()) {
        void *space = PyMem_Calloc(2, sizeof(int));
        if (space == NULL)
            return NULL;
        errobj = PyCapsule_New(space, CTYPES_CAPSULE_NAME_PYMEM, pymem_destructor);
        if (errobj == NULL) {
            PyMem_Free(space);
            return NULL;
        }
        if (-1 == PyDict_SetItem(dict, error_object_name,
                                 errobj)) {
            Py_DECREF(errobj);
            return NULL;
        }
    }
    else {
        return NULL;
    }
    *pspace = (int *)PyCapsule_GetPointer(errobj, CTYPES_CAPSULE_NAME_PYMEM);
    return errobj;
}

static PyObject *
get_error_internal(PyObject *self, PyObject *args, int index)
{
    int *space;
    PyObject *errobj = _ctypes_get_errobj(&space);
    PyObject *result;

    if (errobj == NULL)
        return NULL;
    result = PyLong_FromLong(space[index]);
    Py_DECREF(errobj);
    return result;
}

static PyObject *
set_error_internal(PyObject *self, PyObject *args, int index)
{
    int new_errno, old_errno;
    PyObject *errobj;
    int *space;

    if (!PyArg_ParseTuple(args, "i", &new_errno)) {
        return NULL;
    }
    errobj = _ctypes_get_errobj(&space);
    if (errobj == NULL)
        return NULL;
    old_errno = space[index];
    space[index] = new_errno;
    Py_DECREF(errobj);
    return PyLong_FromLong(old_errno);
}

static PyObject *
get_errno(PyObject *self, PyObject *args)
{
    if (PySys_Audit("ctypes.get_errno", NULL) < 0) {
        return NULL;
    }
    return get_error_internal(self, args, 0);
}

static PyObject *
set_errno(PyObject *self, PyObject *args)
{
    if (PySys_Audit("ctypes.set_errno", "O", args) < 0) {
        return NULL;
    }
    return set_error_internal(self, args, 0);
}

#ifdef MS_WIN32

static PyObject *
get_last_error(PyObject *self, PyObject *args)
{
    if (PySys_Audit("ctypes.get_last_error", NULL) < 0) {
        return NULL;
    }
    return get_error_internal(self, args, 1);
}

static PyObject *
set_last_error(PyObject *self, PyObject *args)
{
    if (PySys_Audit("ctypes.set_last_error", "O", args) < 0) {
        return NULL;
    }
    return set_error_internal(self, args, 1);
}

PyObject *ComError;

static WCHAR *FormatError(DWORD code)
{
    WCHAR *lpMsgBuf;
    DWORD n;
    n = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       code,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
               (LPWSTR) &lpMsgBuf,
               0,
               NULL);
    if (n) {
        while (iswspace(lpMsgBuf[n-1]))
            --n;
        lpMsgBuf[n] = L'\0'; /* rstrip() */
    }
    return lpMsgBuf;
}

#ifndef DONT_USE_SEH
static void SetException(DWORD code, EXCEPTION_RECORD *pr)
{
    if (PySys_Audit("ctypes.seh_exception", "I", code) < 0) {
        /* An exception was set by the audit hook */
        return;
    }

    /* The 'code' is a normal win32 error code so it could be handled by
    PyErr_SetFromWindowsErr(). However, for some errors, we have additional
    information not included in the error code. We handle those here and
    delegate all others to the generic function. */
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        /* The thread attempted to read from or write
           to a virtual address for which it does not
           have the appropriate access. */
        if (pr->ExceptionInformation[0] == 0)
            PyErr_Format(PyExc_OSError,
                         "exception: access violation reading %p",
                         pr->ExceptionInformation[1]);
        else
            PyErr_Format(PyExc_OSError,
                         "exception: access violation writing %p",
                         pr->ExceptionInformation[1]);
        break;

    case EXCEPTION_BREAKPOINT:
        /* A breakpoint was encountered. */
        PyErr_SetString(PyExc_OSError,
                        "exception: breakpoint encountered");
        break;

    case EXCEPTION_DATATYPE_MISALIGNMENT:
        /* The thread attempted to read or write data that is
           misaligned on hardware that does not provide
           alignment. For example, 16-bit values must be
           aligned on 2-byte boundaries, 32-bit values on
           4-byte boundaries, and so on. */
        PyErr_SetString(PyExc_OSError,
                        "exception: datatype misalignment");
        break;

    case EXCEPTION_SINGLE_STEP:
        /* A trace trap or other single-instruction mechanism
           signaled that one instruction has been executed. */
        PyErr_SetString(PyExc_OSError,
                        "exception: single step");
        break;

    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        /* The thread attempted to access an array element
           that is out of bounds, and the underlying hardware
           supports bounds checking. */
        PyErr_SetString(PyExc_OSError,
                        "exception: array bounds exceeded");
        break;

    case EXCEPTION_FLT_DENORMAL_OPERAND:
        /* One of the operands in a floating-point operation
           is denormal. A denormal value is one that is too
           small to represent as a standard floating-point
           value. */
        PyErr_SetString(PyExc_OSError,
                        "exception: floating-point operand denormal");
        break;

    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        /* The thread attempted to divide a floating-point
           value by a floating-point divisor of zero. */
        PyErr_SetString(PyExc_OSError,
                        "exception: float divide by zero");
        break;

    case EXCEPTION_FLT_INEXACT_RESULT:
        /* The result of a floating-point operation cannot be
           represented exactly as a decimal fraction. */
        PyErr_SetString(PyExc_OSError,
                        "exception: float inexact");
        break;

    case EXCEPTION_FLT_INVALID_OPERATION:
        /* This exception represents any floating-point
           exception not included in this list. */
        PyErr_SetString(PyExc_OSError,
                        "exception: float invalid operation");
        break;

    case EXCEPTION_FLT_OVERFLOW:
        /* The exponent of a floating-point operation is
           greater than the magnitude allowed by the
           corresponding type. */
        PyErr_SetString(PyExc_OSError,
                        "exception: float overflow");
        break;

    case EXCEPTION_FLT_STACK_CHECK:
        /* The stack overflowed or underflowed as the result
           of a floating-point operation. */
        PyErr_SetString(PyExc_OSError,
                        "exception: stack over/underflow");
        break;

    case EXCEPTION_STACK_OVERFLOW:
        /* The stack overflowed or underflowed as the result
           of a floating-point operation. */
        PyErr_SetString(PyExc_OSError,
                        "exception: stack overflow");
        break;

    case EXCEPTION_FLT_UNDERFLOW:
        /* The exponent of a floating-point operation is less
           than the magnitude allowed by the corresponding
           type. */
        PyErr_SetString(PyExc_OSError,
                        "exception: float underflow");
        break;

    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        /* The thread attempted to divide an integer value by
           an integer divisor of zero. */
        PyErr_SetString(PyExc_OSError,
                        "exception: integer divide by zero");
        break;

    case EXCEPTION_INT_OVERFLOW:
        /* The result of an integer operation caused a carry
           out of the most significant bit of the result. */
        PyErr_SetString(PyExc_OSError,
                        "exception: integer overflow");
        break;

    case EXCEPTION_PRIV_INSTRUCTION:
        /* The thread attempted to execute an instruction
           whose operation is not allowed in the current
           machine mode. */
        PyErr_SetString(PyExc_OSError,
                        "exception: privileged instruction");
        break;

    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        /* The thread attempted to continue execution after a
           noncontinuable exception occurred. */
        PyErr_SetString(PyExc_OSError,
                        "exception: nocontinuable");
        break;

    default:
        PyErr_SetFromWindowsErr(code);
        break;
    }
}

static DWORD HandleException(EXCEPTION_POINTERS *ptrs,
                             DWORD *pdw, EXCEPTION_RECORD *record)
{
    *pdw = ptrs->ExceptionRecord->ExceptionCode;
    *record = *ptrs->ExceptionRecord;
    /* We don't want to catch breakpoint exceptions, they are used to attach
     * a debugger to the process.
     */
    if (*pdw == EXCEPTION_BREAKPOINT)
        return EXCEPTION_CONTINUE_SEARCH;
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

static PyObject *
check_hresult(PyObject *self, PyObject *args)
{
    HRESULT hr;
    if (!PyArg_ParseTuple(args, "i", &hr))
        return NULL;
    if (FAILED(hr))
        return PyErr_SetFromWindowsErr(hr);
    return PyLong_FromLong(hr);
}

#endif

/**************************************************************/

PyCArgObject *
PyCArgObject_new(void)
{
    PyCArgObject *p;
    p = PyObject_New(PyCArgObject, &PyCArg_Type);
    if (p == NULL)
        return NULL;
    p->pffi_type = NULL;
    p->tag = '\0';
    p->obj = NULL;
    memset(&p->value, 0, sizeof(p->value));
    return p;
}

static void
PyCArg_dealloc(PyCArgObject *self)
{
    Py_XDECREF(self->obj);
    PyObject_Del(self);
}

static int
is_literal_char(unsigned char c)
{
    return c < 128 && _PyUnicode_IsPrintable(c) && c != '\\' && c != '\'';
}

static PyObject *
PyCArg_repr(PyCArgObject *self)
{
    char buffer[256];
    switch(self->tag) {
    case 'b':
    case 'B':
        sprintf(buffer, "<cparam '%c' (%d)>",
            self->tag, self->value.b);
        break;
    case 'h':
    case 'H':
        sprintf(buffer, "<cparam '%c' (%d)>",
            self->tag, self->value.h);
        break;
    case 'i':
    case 'I':
        sprintf(buffer, "<cparam '%c' (%d)>",
            self->tag, self->value.i);
        break;
    case 'l':
    case 'L':
        sprintf(buffer, "<cparam '%c' (%ld)>",
            self->tag, self->value.l);
        break;

    case 'q':
    case 'Q':
        sprintf(buffer,
#ifdef MS_WIN32
            "<cparam '%c' (%I64d)>",
#else
            "<cparam '%c' (%lld)>",
#endif
            self->tag, self->value.q);
        break;
    case 'd':
        sprintf(buffer, "<cparam '%c' (%f)>",
            self->tag, self->value.d);
        break;
    case 'f':
        sprintf(buffer, "<cparam '%c' (%f)>",
            self->tag, self->value.f);
        break;

    case 'c':
        if (is_literal_char((unsigned char)self->value.c)) {
            sprintf(buffer, "<cparam '%c' ('%c')>",
                self->tag, self->value.c);
        }
        else {
            sprintf(buffer, "<cparam '%c' ('\\x%02x')>",
                self->tag, (unsigned char)self->value.c);
        }
        break;

/* Hm, are these 'z' and 'Z' codes useful at all?
   Shouldn't they be replaced by the functionality of c_string
   and c_wstring ?
*/
    case 'z':
    case 'Z':
    case 'P':
        sprintf(buffer, "<cparam '%c' (%p)>",
            self->tag, self->value.p);
        break;

    default:
        if (is_literal_char((unsigned char)self->tag)) {
            sprintf(buffer, "<cparam '%c' at %p>",
                (unsigned char)self->tag, (void *)self);
        }
        else {
            sprintf(buffer, "<cparam 0x%02x at %p>",
                (unsigned char)self->tag, (void *)self);
        }
        break;
    }
    return PyUnicode_FromString(buffer);
}

static PyMemberDef PyCArgType_members[] = {
    { "_obj", T_OBJECT,
      offsetof(PyCArgObject, obj), READONLY,
      "the wrapped object" },
    { NULL },
};

PyTypeObject PyCArg_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "CArgObject",
    sizeof(PyCArgObject),
    0,
    (destructor)PyCArg_dealloc,                 /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)PyCArg_repr,                      /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    PyCArgType_members,                         /* tp_members */
};

/****************************************************************/
/*
 * Convert a PyObject * into a parameter suitable to pass to an
 * C function call.
 *
 * 1. Python integers are converted to C int and passed by value.
 *    Py_None is converted to a C NULL pointer.
 *
 * 2. 3-tuples are expected to have a format character in the first
 *    item, which must be 'i', 'f', 'd', 'q', or 'P'.
 *    The second item will have to be an integer, float, double, long long
 *    or integer (denoting an address void *), will be converted to the
 *    corresponding C data type and passed by value.
 *
 * 3. Other Python objects are tested for an '_as_parameter_' attribute.
 *    The value of this attribute must be an integer which will be passed
 *    by value, or a 2-tuple or 3-tuple which will be used according
 *    to point 2 above. The third item (if any), is ignored. It is normally
 *    used to keep the object alive where this parameter refers to.
 *    XXX This convention is dangerous - you can construct arbitrary tuples
 *    in Python and pass them. Would it be safer to use a custom container
 *    datatype instead of a tuple?
 *
 * 4. Other Python objects cannot be passed as parameters - an exception is raised.
 *
 * 5. ConvParam will store the converted result in a struct containing format
 *    and value.
 */

union result {
    char c;
    char b;
    short h;
    int i;
    long l;
    long long q;
    long double D;
    double d;
    float f;
    void *p;
};

struct argument {
    ffi_type *ffi_type;
    PyObject *keep;
    union result value;
};

/*
 * Convert a single Python object into a PyCArgObject and return it.
 */
static int ConvParam(PyObject *obj, Py_ssize_t index, struct argument *pa)
{
    StgDictObject *dict;
    pa->keep = NULL; /* so we cannot forget it later */

    dict = PyObject_stgdict(obj);
    if (dict) {
        PyCArgObject *carg;
        assert(dict->paramfunc);
        /* If it has an stgdict, it is a CDataObject */
        carg = dict->paramfunc((CDataObject *)obj);
        if (carg == NULL)
            return -1;
        pa->ffi_type = carg->pffi_type;
        memcpy(&pa->value, &carg->value, sizeof(pa->value));
        pa->keep = (PyObject *)carg;
        return 0;
    }

    if (PyCArg_CheckExact(obj)) {
        PyCArgObject *carg = (PyCArgObject *)obj;
        pa->ffi_type = carg->pffi_type;
        Py_INCREF(obj);
        pa->keep = obj;
        memcpy(&pa->value, &carg->value, sizeof(pa->value));
        return 0;
    }

    /* check for None, integer, string or unicode and use directly if successful */
    if (obj == Py_None) {
        pa->ffi_type = &ffi_type_pointer;
        pa->value.p = NULL;
        return 0;
    }

    if (PyLong_Check(obj)) {
        pa->ffi_type = &ffi_type_sint;
        pa->value.i = (long)PyLong_AsUnsignedLong(obj);
        if (pa->value.i == -1 && PyErr_Occurred()) {
            PyErr_Clear();
            pa->value.i = PyLong_AsLong(obj);
            if (pa->value.i == -1 && PyErr_Occurred()) {
                PyErr_SetString(PyExc_OverflowError,
                                "int too long to convert");
                return -1;
            }
        }
        return 0;
    }

    if (PyBytes_Check(obj)) {
        pa->ffi_type = &ffi_type_pointer;
        pa->value.p = PyBytes_AsString(obj);
        Py_INCREF(obj);
        pa->keep = obj;
        return 0;
    }

#ifdef CTYPES_UNICODE
    if (PyUnicode_Check(obj)) {
        pa->ffi_type = &ffi_type_pointer;
        pa->value.p = PyUnicode_AsWideCharString(obj, NULL);
        if (pa->value.p == NULL)
            return -1;
        pa->keep = PyCapsule_New(pa->value.p, CTYPES_CAPSULE_NAME_PYMEM, pymem_destructor);
        if (!pa->keep) {
            PyMem_Free(pa->value.p);
            return -1;
        }
        return 0;
    }
#endif

    {
        _Py_IDENTIFIER(_as_parameter_);
        PyObject *arg;
        if (_PyObject_LookupAttrId(obj, &PyId__as_parameter_, &arg) < 0) {
            return -1;
        }
        /* Which types should we exactly allow here?
           integers are required for using Python classes
           as parameters (they have to expose the '_as_parameter_'
           attribute)
        */
        if (arg) {
            int result;
            result = ConvParam(arg, index, pa);
            Py_DECREF(arg);
            return result;
        }
        PyErr_Format(PyExc_TypeError,
                     "Don't know how to convert parameter %d",
                     Py_SAFE_DOWNCAST(index, Py_ssize_t, int));
        return -1;
    }
}

#if defined(MS_WIN32) && !defined(_WIN32_WCE)
/*
Per: https://msdn.microsoft.com/en-us/library/7572ztz4.aspx
To be returned by value in RAX, user-defined types must have a length
of 1, 2, 4, 8, 16, 32, or 64 bits
*/
int can_return_struct_as_int(size_t s)
{
    return s == 1 || s == 2 || s == 4;
}

int can_return_struct_as_sint64(size_t s)
{
#ifdef _M_ARM
    // 8 byte structs cannot be returned in a register on ARM32
    return 0;
#else
    return s == 8;
#endif
}
#endif


ffi_type *_ctypes_get_ffi_type(PyObject *obj)
{
    StgDictObject *dict;
    if (obj == NULL)
        return &ffi_type_sint;
    dict = PyType_stgdict(obj);
    if (dict == NULL)
        return &ffi_type_sint;
#if defined(MS_WIN32) && !defined(_WIN32_WCE)
    /* This little trick works correctly with MSVC.
       It returns small structures in registers
    */
    if (dict->ffi_type_pointer.type == FFI_TYPE_STRUCT) {
        if (can_return_struct_as_int(dict->ffi_type_pointer.size))
            return &ffi_type_sint32;
        else if (can_return_struct_as_sint64 (dict->ffi_type_pointer.size))
            return &ffi_type_sint64;
    }
#endif
    return &dict->ffi_type_pointer;
}


/*
 * libffi uses:
 *
 * ffi_status ffi_prep_cif(ffi_cif *cif, ffi_abi abi,
 *                         unsigned int nargs,
 *                         ffi_type *rtype,
 *                         ffi_type **atypes);
 *
 * and then
 *
 * void ffi_call(ffi_cif *cif, void *fn, void *rvalue, void **avalues);
 */
static int _call_function_pointer(int flags,
                                  PPROC pProc,
                                  void **avalues,
                                  ffi_type **atypes,
                                  ffi_type *restype,
                                  void *resmem,
                                  int argcount)
{
    PyThreadState *_save = NULL; /* For Py_BLOCK_THREADS and Py_UNBLOCK_THREADS */
    PyObject *error_object = NULL;
    int *space;
    ffi_cif cif;
    int cc;
#if defined(MS_WIN32) && !defined(DONT_USE_SEH)
    DWORD dwExceptionCode = 0;
    EXCEPTION_RECORD record;
#endif
    /* XXX check before here */
    if (restype == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "No ffi_type for result");
        return -1;
    }

    cc = FFI_DEFAULT_ABI;
#if defined(MS_WIN32) && !defined(MS_WIN64) && !defined(_WIN32_WCE) && !defined(_M_ARM)
    if ((flags & FUNCFLAG_CDECL) == 0)
        cc = FFI_STDCALL;
#endif
    if (FFI_OK != ffi_prep_cif(&cif,
                               cc,
                               argcount,
                               restype,
                               atypes)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "ffi_prep_cif failed");
        return -1;
    }

    if (flags & (FUNCFLAG_USE_ERRNO | FUNCFLAG_USE_LASTERROR)) {
        error_object = _ctypes_get_errobj(&space);
        if (error_object == NULL)
            return -1;
    }
    if ((flags & FUNCFLAG_PYTHONAPI) == 0)
        Py_UNBLOCK_THREADS
    if (flags & FUNCFLAG_USE_ERRNO) {
        int temp = space[0];
        space[0] = errno;
        errno = temp;
    }
#ifdef MS_WIN32
    if (flags & FUNCFLAG_USE_LASTERROR) {
        int temp = space[1];
        space[1] = GetLastError();
        SetLastError(temp);
    }
#ifndef DONT_USE_SEH
    __try {
#endif
#endif
                ffi_call(&cif, (void *)pProc, resmem, avalues);
#ifdef MS_WIN32
#ifndef DONT_USE_SEH
    }
    __except (HandleException(GetExceptionInformation(),
                              &dwExceptionCode, &record)) {
        ;
    }
#endif
    if (flags & FUNCFLAG_USE_LASTERROR) {
        int temp = space[1];
        space[1] = GetLastError();
        SetLastError(temp);
    }
#endif
    if (flags & FUNCFLAG_USE_ERRNO) {
        int temp = space[0];
        space[0] = errno;
        errno = temp;
    }
    if ((flags & FUNCFLAG_PYTHONAPI) == 0)
        Py_BLOCK_THREADS
    Py_XDECREF(error_object);
#ifdef MS_WIN32
#ifndef DONT_USE_SEH
    if (dwExceptionCode) {
        SetException(dwExceptionCode, &record);
        return -1;
    }
#endif
#endif
    if ((flags & FUNCFLAG_PYTHONAPI) && PyErr_Occurred())
        return -1;
    return 0;
}

/*
 * Convert the C value in result into a Python object, depending on restype.
 *
 * - If restype is NULL, return a Python integer.
 * - If restype is None, return None.
 * - If restype is a simple ctypes type (c_int, c_void_p), call the type's getfunc,
 *   pass the result to checker and return the result.
 * - If restype is another ctypes type, return an instance of that.
 * - Otherwise, call restype and return the result.
 */
static PyObject *GetResult(PyObject *restype, void *result, PyObject *checker)
{
    StgDictObject *dict;
    PyObject *retval, *v;

    if (restype == NULL)
        return PyLong_FromLong(*(int *)result);

    if (restype == Py_None) {
        Py_RETURN_NONE;
    }

    dict = PyType_stgdict(restype);
    if (dict == NULL)
        return PyObject_CallFunction(restype, "i", *(int *)result);

    if (dict->getfunc && !_ctypes_simple_instance(restype)) {
        retval = dict->getfunc(result, dict->size);
        /* If restype is py_object (detected by comparing getfunc with
           O_get), we have to call Py_DECREF because O_get has already
           called Py_INCREF.
        */
        if (dict->getfunc == _ctypes_get_fielddesc("O")->getfunc) {
            Py_DECREF(retval);
        }
    } else
        retval = PyCData_FromBaseObj(restype, NULL, 0, result);

    if (!checker || !retval)
        return retval;

    v = PyObject_CallOneArg(checker, retval);
    if (v == NULL)
        _PyTraceback_Add("GetResult", "_ctypes/callproc.c", __LINE__-2);
    Py_DECREF(retval);
    return v;
}

/*
 * Raise a new exception 'exc_class', adding additional text to the original
 * exception string.
 */
void _ctypes_extend_error(PyObject *exc_class, const char *fmt, ...)
{
    va_list vargs;
    PyObject *tp, *v, *tb, *s, *cls_str, *msg_str;

    va_start(vargs, fmt);
    s = PyUnicode_FromFormatV(fmt, vargs);
    va_end(vargs);
    if (!s)
        return;

    PyErr_Fetch(&tp, &v, &tb);
    PyErr_NormalizeException(&tp, &v, &tb);
    cls_str = PyObject_Str(tp);
    if (cls_str) {
        PyUnicode_AppendAndDel(&s, cls_str);
        PyUnicode_AppendAndDel(&s, PyUnicode_FromString(": "));
        if (s == NULL)
            goto error;
    } else
        PyErr_Clear();
    msg_str = PyObject_Str(v);
    if (msg_str)
        PyUnicode_AppendAndDel(&s, msg_str);
    else {
        PyErr_Clear();
        PyUnicode_AppendAndDel(&s, PyUnicode_FromString("???"));
    }
    if (s == NULL)
        goto error;
    PyErr_SetObject(exc_class, s);
error:
    Py_XDECREF(tp);
    Py_XDECREF(v);
    Py_XDECREF(tb);
    Py_XDECREF(s);
}


#ifdef MS_WIN32

static PyObject *
GetComError(HRESULT errcode, GUID *riid, IUnknown *pIunk)
{
    HRESULT hr;
    ISupportErrorInfo *psei = NULL;
    IErrorInfo *pei = NULL;
    BSTR descr=NULL, helpfile=NULL, source=NULL;
    GUID guid;
    DWORD helpcontext=0;
    LPOLESTR progid;
    PyObject *obj;
    LPOLESTR text;

    /* We absolutely have to release the GIL during COM method calls,
       otherwise we may get a deadlock!
    */
    Py_BEGIN_ALLOW_THREADS

    hr = pIunk->lpVtbl->QueryInterface(pIunk, &IID_ISupportErrorInfo, (void **)&psei);
    if (FAILED(hr))
        goto failed;

    hr = psei->lpVtbl->InterfaceSupportsErrorInfo(psei, riid);
    psei->lpVtbl->Release(psei);
    if (FAILED(hr))
        goto failed;

    hr = GetErrorInfo(0, &pei);
    if (hr != S_OK)
        goto failed;

    pei->lpVtbl->GetDescription(pei, &descr);
    pei->lpVtbl->GetGUID(pei, &guid);
    pei->lpVtbl->GetHelpContext(pei, &helpcontext);
    pei->lpVtbl->GetHelpFile(pei, &helpfile);
    pei->lpVtbl->GetSource(pei, &source);

    pei->lpVtbl->Release(pei);

  failed:
    Py_END_ALLOW_THREADS

    progid = NULL;
    ProgIDFromCLSID(&guid, &progid);

    text = FormatError(errcode);
    obj = Py_BuildValue(
        "iu(uuuiu)",
        errcode,
        text,
        descr, source, helpfile, helpcontext,
        progid);
    if (obj) {
        PyErr_SetObject(ComError, obj);
        Py_DECREF(obj);
    }
    LocalFree(text);

    if (descr)
        SysFreeString(descr);
    if (helpfile)
        SysFreeString(helpfile);
    if (source)
        SysFreeString(source);

    return NULL;
}
#endif

#if (defined(__x86_64__) && (defined(__MINGW64__) || defined(__CYGWIN__))) || \
    defined(__aarch64__) || defined(__riscv)
#define CTYPES_PASS_BY_REF_HACK
#define POW2(x) (((x & ~(x - 1)) == x) ? x : 0)
#define IS_PASS_BY_REF(x) (x > 8 || !POW2(x))
#endif

/*
 * Requirements, must be ensured by the caller:
 * - argtuple is tuple of arguments
 * - argtypes is either NULL, or a tuple of the same size as argtuple
 *
 * - XXX various requirements for restype, not yet collected
 */
PyObject *_ctypes_callproc(PPROC pProc,
                    PyObject *argtuple,
#ifdef MS_WIN32
                    IUnknown *pIunk,
                    GUID *iid,
#endif
                    int flags,
                    PyObject *argtypes, /* misleading name: This is a tuple of
                                           methods, not types: the .from_param
                                           class methods of the types */
            PyObject *restype,
            PyObject *checker)
{
    Py_ssize_t i, n, argcount, argtype_count;
    void *resbuf;
    struct argument *args, *pa;
    ffi_type **atypes;
    ffi_type *rtype;
    void **avalues;
    PyObject *retval = NULL;

    n = argcount = PyTuple_GET_SIZE(argtuple);
#ifdef MS_WIN32
    /* an optional COM object this pointer */
    if (pIunk)
        ++argcount;
#endif

    args = (struct argument *)alloca(sizeof(struct argument) * argcount);
    if (!args) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(args, 0, sizeof(struct argument) * argcount);
    argtype_count = argtypes ? PyTuple_GET_SIZE(argtypes) : 0;
#ifdef MS_WIN32
    if (pIunk) {
        args[0].ffi_type = &ffi_type_pointer;
        args[0].value.p = pIunk;
        pa = &args[1];
    } else
#endif
        pa = &args[0];

    /* Convert the arguments */
    for (i = 0; i < n; ++i, ++pa) {
        PyObject *converter;
        PyObject *arg;
        int err;

        arg = PyTuple_GET_ITEM(argtuple, i);            /* borrowed ref */
        /* For cdecl functions, we allow more actual arguments
           than the length of the argtypes tuple.
           This is checked in _ctypes::PyCFuncPtr_Call
        */
        if (argtypes && argtype_count > i) {
            PyObject *v;
            converter = PyTuple_GET_ITEM(argtypes, i);
            v = PyObject_CallOneArg(converter, arg);
            if (v == NULL) {
                _ctypes_extend_error(PyExc_ArgError, "argument %zd: ", i+1);
                goto cleanup;
            }

            err = ConvParam(v, i+1, pa);
            Py_DECREF(v);
            if (-1 == err) {
                _ctypes_extend_error(PyExc_ArgError, "argument %zd: ", i+1);
                goto cleanup;
            }
        } else {
            err = ConvParam(arg, i+1, pa);
            if (-1 == err) {
                _ctypes_extend_error(PyExc_ArgError, "argument %zd: ", i+1);
                goto cleanup; /* leaking ? */
            }
        }
    }

    rtype = _ctypes_get_ffi_type(restype);
    resbuf = alloca(max(rtype->size, sizeof(ffi_arg)));

#ifdef _Py_MEMORY_SANITIZER
    /* ffi_call actually initializes resbuf, but from asm, which
     * MemorySanitizer can't detect. Avoid false positives from MSan. */
    if (resbuf != NULL) {
        __msan_unpoison(resbuf, max(rtype->size, sizeof(ffi_arg)));
    }
#endif
    avalues = (void **)alloca(sizeof(void *) * argcount);
    atypes = (ffi_type **)alloca(sizeof(ffi_type *) * argcount);
    if (!resbuf || !avalues || !atypes) {
        PyErr_NoMemory();
        goto cleanup;
    }
    for (i = 0; i < argcount; ++i) {
        atypes[i] = args[i].ffi_type;
#ifdef CTYPES_PASS_BY_REF_HACK
        size_t size = atypes[i]->size;
        if (IS_PASS_BY_REF(size)) {
            void *tmp = alloca(size);
            if (atypes[i]->type == FFI_TYPE_STRUCT)
                memcpy(tmp, args[i].value.p, size);
            else
                memcpy(tmp, (void*)&args[i].value, size);

            avalues[i] = tmp;
        }
        else
#endif
        if (atypes[i]->type == FFI_TYPE_STRUCT)
            avalues[i] = (void *)args[i].value.p;
        else
            avalues[i] = (void *)&args[i].value;
    }

    if (-1 == _call_function_pointer(flags, pProc, avalues, atypes,
                                     rtype, resbuf,
                                     Py_SAFE_DOWNCAST(argcount,
                                                      Py_ssize_t,
                                                      int)))
        goto cleanup;

#ifdef WORDS_BIGENDIAN
    /* libffi returns the result in a buffer with sizeof(ffi_arg). This
       causes problems on big endian machines, since the result buffer
       address cannot simply be used as result pointer, instead we must
       adjust the pointer value:
     */
    /*
      XXX I should find out and clarify why this is needed at all,
      especially why adjusting for ffi_type_float must be avoided on
      64-bit platforms.
     */
    if (rtype->type != FFI_TYPE_FLOAT
        && rtype->type != FFI_TYPE_STRUCT
        && rtype->size < sizeof(ffi_arg))
        resbuf = (char *)resbuf + sizeof(ffi_arg) - rtype->size;
#endif

#ifdef MS_WIN32
    if (iid && pIunk) {
        if (*(int *)resbuf & 0x80000000)
            retval = GetComError(*(HRESULT *)resbuf, iid, pIunk);
        else
            retval = PyLong_FromLong(*(int *)resbuf);
    } else if (flags & FUNCFLAG_HRESULT) {
        if (*(int *)resbuf & 0x80000000)
            retval = PyErr_SetFromWindowsErr(*(int *)resbuf);
        else
            retval = PyLong_FromLong(*(int *)resbuf);
    } else
#endif
        retval = GetResult(restype, resbuf, checker);
  cleanup:
    for (i = 0; i < argcount; ++i)
        Py_XDECREF(args[i].keep);
    return retval;
}

static int
_parse_voidp(PyObject *obj, void **address)
{
    *address = PyLong_AsVoidPtr(obj);
    if (*address == NULL)
        return 0;
    return 1;
}

#ifdef MS_WIN32

static const char format_error_doc[] =
"FormatError([integer]) -> string\n\
\n\
Convert a win32 error code into a string. If the error code is not\n\
given, the return value of a call to GetLastError() is used.\n";
static PyObject *format_error(PyObject *self, PyObject *args)
{
    PyObject *result;
    wchar_t *lpMsgBuf;
    DWORD code = 0;
    if (!PyArg_ParseTuple(args, "|i:FormatError", &code))
        return NULL;
    if (code == 0)
        code = GetLastError();
    lpMsgBuf = FormatError(code);
    if (lpMsgBuf) {
        result = PyUnicode_FromWideChar(lpMsgBuf, wcslen(lpMsgBuf));
        LocalFree(lpMsgBuf);
    } else {
        result = PyUnicode_FromString("<no description>");
    }
    return result;
}

static const char load_library_doc[] =
"LoadLibrary(name, load_flags) -> handle\n\
\n\
Load an executable (usually a DLL), and return a handle to it.\n\
The handle may be used to locate exported functions in this\n\
module. load_flags are as defined for LoadLibraryEx in the\n\
Windows API.\n";
static PyObject *load_library(PyObject *self, PyObject *args)
{
    const WCHAR *name;
    PyObject *nameobj;
    int load_flags = 0;
    HMODULE hMod;
    DWORD err;

    if (!PyArg_ParseTuple(args, "U|i:LoadLibrary", &nameobj, &load_flags))
        return NULL;

    name = _PyUnicode_AsUnicode(nameobj);
    if (!name)
        return NULL;

    if (PySys_Audit("ctypes.dlopen", "O", nameobj) < 0) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    /* bpo-36085: Limit DLL search directories to avoid pre-loading
     * attacks and enable use of the AddDllDirectory function.
     */
    hMod = LoadLibraryExW(name, NULL, (DWORD)load_flags);
    err = hMod ? 0 : GetLastError();
    Py_END_ALLOW_THREADS

    if (err == ERROR_MOD_NOT_FOUND) {
        PyErr_Format(PyExc_FileNotFoundError,
                     ("Could not find module '%.500S' (or one of its "
                      "dependencies). Try using the full path with "
                      "constructor syntax."),
                     nameobj);
        return NULL;
    } else if (err) {
        return PyErr_SetFromWindowsErr(err);
    }
#ifdef _WIN64
    return PyLong_FromVoidPtr(hMod);
#else
    return Py_BuildValue("i", hMod);
#endif
}

static const char free_library_doc[] =
"FreeLibrary(handle) -> void\n\
\n\
Free the handle of an executable previously loaded by LoadLibrary.\n";
static PyObject *free_library(PyObject *self, PyObject *args)
{
    void *hMod;
    BOOL result;
    DWORD err;
    if (!PyArg_ParseTuple(args, "O&:FreeLibrary", &_parse_voidp, &hMod))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    result = FreeLibrary((HMODULE)hMod);
    err = result ? 0 : GetLastError();
    Py_END_ALLOW_THREADS

    if (!result) {
        return PyErr_SetFromWindowsErr(err);
    }
    Py_RETURN_NONE;
}

static const char copy_com_pointer_doc[] =
"CopyComPointer(src, dst) -> HRESULT value\n";

static PyObject *
copy_com_pointer(PyObject *self, PyObject *args)
{
    PyObject *p1, *p2, *r = NULL;
    struct argument a, b;
    IUnknown *src, **pdst;
    if (!PyArg_ParseTuple(args, "OO:CopyComPointer", &p1, &p2))
        return NULL;
    a.keep = b.keep = NULL;

    if (-1 == ConvParam(p1, 0, &a) || -1 == ConvParam(p2, 1, &b))
        goto done;
    src = (IUnknown *)a.value.p;
    pdst = (IUnknown **)b.value.p;

    if (pdst == NULL)
        r = PyLong_FromLong(E_POINTER);
    else {
        if (src)
            src->lpVtbl->AddRef(src);
        *pdst = src;
        r = PyLong_FromLong(S_OK);
    }
  done:
    Py_XDECREF(a.keep);
    Py_XDECREF(b.keep);
    return r;
}
#else

static PyObject *py_dl_open(PyObject *self, PyObject *args)
{
    PyObject *name, *name2;
    const char *name_str;
    void * handle;
#if HAVE_DECL_RTLD_LOCAL
    int mode = RTLD_NOW | RTLD_LOCAL;
#else
    /* cygwin doesn't define RTLD_LOCAL */
    int mode = RTLD_NOW;
#endif
    if (!PyArg_ParseTuple(args, "O|i:dlopen", &name, &mode))
        return NULL;
    mode |= RTLD_NOW;
    if (name != Py_None) {
        if (PyUnicode_FSConverter(name, &name2) == 0)
            return NULL;
        if (PyBytes_Check(name2))
            name_str = PyBytes_AS_STRING(name2);
        else
            name_str = PyByteArray_AS_STRING(name2);
    } else {
        name_str = NULL;
        name2 = NULL;
    }
    if (PySys_Audit("ctypes.dlopen", "s", name_str) < 0) {
        return NULL;
    }
    handle = ctypes_dlopen(name_str, mode);
    Py_XDECREF(name2);
    if (!handle) {
        const char *errmsg = ctypes_dlerror();
        if (!errmsg)
            errmsg = "dlopen() error";
        PyErr_SetString(PyExc_OSError,
                               errmsg);
        return NULL;
    }
    return PyLong_FromVoidPtr(handle);
}

static PyObject *py_dl_close(PyObject *self, PyObject *args)
{
    void *handle;

    if (!PyArg_ParseTuple(args, "O&:dlclose", &_parse_voidp, &handle))
        return NULL;
    if (dlclose(handle)) {
        PyErr_SetString(PyExc_OSError,
                               ctypes_dlerror());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *py_dl_sym(PyObject *self, PyObject *args)
{
    char *name;
    void *handle;
    void *ptr;

    if (!PyArg_ParseTuple(args, "O&s:dlsym",
                          &_parse_voidp, &handle, &name))
        return NULL;
    if (PySys_Audit("ctypes.dlsym/handle", "O", args) < 0) {
        return NULL;
    }
    ptr = ctypes_dlsym((void*)handle, name);
    if (!ptr) {
        PyErr_SetString(PyExc_OSError,
                               ctypes_dlerror());
        return NULL;
    }
    return PyLong_FromVoidPtr(ptr);
}
#endif

/*
 * Only for debugging so far: So that we can call CFunction instances
 *
 * XXX Needs to accept more arguments: flags, argtypes, restype
 */
static PyObject *
call_function(PyObject *self, PyObject *args)
{
    void *func;
    PyObject *arguments;
    PyObject *result;

    if (!PyArg_ParseTuple(args,
                          "O&O!",
                          &_parse_voidp, &func,
                          &PyTuple_Type, &arguments))
        return NULL;
    if (PySys_Audit("ctypes.call_function", "nO",
                    (Py_ssize_t)func, arguments) < 0) {
        return NULL;
    }

    result =  _ctypes_callproc((PPROC)func,
                        arguments,
#ifdef MS_WIN32
                        NULL,
                        NULL,
#endif
                        0, /* flags */
                NULL, /* self->argtypes */
                NULL, /* self->restype */
                NULL); /* checker */
    return result;
}

/*
 * Only for debugging so far: So that we can call CFunction instances
 *
 * XXX Needs to accept more arguments: flags, argtypes, restype
 */
static PyObject *
call_cdeclfunction(PyObject *self, PyObject *args)
{
    void *func;
    PyObject *arguments;
    PyObject *result;

    if (!PyArg_ParseTuple(args,
                          "O&O!",
                          &_parse_voidp, &func,
                          &PyTuple_Type, &arguments))
        return NULL;
    if (PySys_Audit("ctypes.call_function", "nO",
                    (Py_ssize_t)func, arguments) < 0) {
        return NULL;
    }

    result =  _ctypes_callproc((PPROC)func,
                        arguments,
#ifdef MS_WIN32
                        NULL,
                        NULL,
#endif
                        FUNCFLAG_CDECL, /* flags */
                        NULL, /* self->argtypes */
                        NULL, /* self->restype */
                        NULL); /* checker */
    return result;
}

/*****************************************************************
 * functions
 */
static const char sizeof_doc[] =
"sizeof(C type) -> integer\n"
"sizeof(C instance) -> integer\n"
"Return the size in bytes of a C instance";

static PyObject *
sizeof_func(PyObject *self, PyObject *obj)
{
    StgDictObject *dict;

    dict = PyType_stgdict(obj);
    if (dict)
        return PyLong_FromSsize_t(dict->size);

    if (CDataObject_Check(obj))
        return PyLong_FromSsize_t(((CDataObject *)obj)->b_size);
    PyErr_SetString(PyExc_TypeError,
                    "this type has no size");
    return NULL;
}

static const char alignment_doc[] =
"alignment(C type) -> integer\n"
"alignment(C instance) -> integer\n"
"Return the alignment requirements of a C instance";

static PyObject *
align_func(PyObject *self, PyObject *obj)
{
    StgDictObject *dict;

    dict = PyType_stgdict(obj);
    if (dict)
        return PyLong_FromSsize_t(dict->align);

    dict = PyObject_stgdict(obj);
    if (dict)
        return PyLong_FromSsize_t(dict->align);

    PyErr_SetString(PyExc_TypeError,
                    "no alignment info");
    return NULL;
}

static const char byref_doc[] =
"byref(C instance[, offset=0]) -> byref-object\n"
"Return a pointer lookalike to a C instance, only usable\n"
"as function argument";

/*
 * We must return something which can be converted to a parameter,
 * but still has a reference to self.
 */
static PyObject *
byref(PyObject *self, PyObject *args)
{
    PyCArgObject *parg;
    PyObject *obj;
    PyObject *pyoffset = NULL;
    Py_ssize_t offset = 0;

    if (!PyArg_UnpackTuple(args, "byref", 1, 2,
                           &obj, &pyoffset))
        return NULL;
    if (pyoffset) {
        offset = PyNumber_AsSsize_t(pyoffset, NULL);
        if (offset == -1 && PyErr_Occurred())
            return NULL;
    }
    if (!CDataObject_Check(obj)) {
        PyErr_Format(PyExc_TypeError,
                     "byref() argument must be a ctypes instance, not '%s'",
                     Py_TYPE(obj)->tp_name);
        return NULL;
    }

    parg = PyCArgObject_new();
    if (parg == NULL)
        return NULL;

    parg->tag = 'P';
    parg->pffi_type = &ffi_type_pointer;
    Py_INCREF(obj);
    parg->obj = obj;
    parg->value.p = (char *)((CDataObject *)obj)->b_ptr + offset;
    return (PyObject *)parg;
}

static const char addressof_doc[] =
"addressof(C instance) -> integer\n"
"Return the address of the C instance internal buffer";

static PyObject *
addressof(PyObject *self, PyObject *obj)
{
    if (!CDataObject_Check(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "invalid type");
        return NULL;
    }
    if (PySys_Audit("ctypes.addressof", "(O)", obj) < 0) {
        return NULL;
    }
    return PyLong_FromVoidPtr(((CDataObject *)obj)->b_ptr);
}

static int
converter(PyObject *obj, void **address)
{
    *address = PyLong_AsVoidPtr(obj);
    return *address != NULL;
}

static PyObject *
My_PyObj_FromPtr(PyObject *self, PyObject *args)
{
    PyObject *ob;
    if (!PyArg_ParseTuple(args, "O&:PyObj_FromPtr", converter, &ob)) {
        return NULL;
    }
    if (PySys_Audit("ctypes.PyObj_FromPtr", "(O)", ob) < 0) {
        return NULL;
    }
    Py_INCREF(ob);
    return ob;
}

static PyObject *
My_Py_INCREF(PyObject *self, PyObject *arg)
{
    Py_INCREF(arg); /* that's what this function is for */
    Py_INCREF(arg); /* that for returning it */
    return arg;
}

static PyObject *
My_Py_DECREF(PyObject *self, PyObject *arg)
{
    Py_DECREF(arg); /* that's what this function is for */
    Py_INCREF(arg); /* that's for returning it */
    return arg;
}

static PyObject *
resize(PyObject *self, PyObject *args)
{
    CDataObject *obj;
    StgDictObject *dict;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args,
                          "On:resize",
                          &obj, &size))
        return NULL;

    dict = PyObject_stgdict((PyObject *)obj);
    if (dict == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "excepted ctypes instance");
        return NULL;
    }
    if (size < dict->size) {
        PyErr_Format(PyExc_ValueError,
                     "minimum size is %zd",
                     dict->size);
        return NULL;
    }
    if (obj->b_needsfree == 0) {
        PyErr_Format(PyExc_ValueError,
                     "Memory cannot be resized because this object doesn't own it");
        return NULL;
    }
    if ((size_t)size <= sizeof(obj->b_value)) {
        /* internal default buffer is large enough */
        obj->b_size = size;
        goto done;
    }
    if (!_CDataObject_HasExternalBuffer(obj)) {
        /* We are currently using the objects default buffer, but it
           isn't large enough any more. */
        void *ptr = PyMem_Calloc(1, size);
        if (ptr == NULL)
            return PyErr_NoMemory();
        memmove(ptr, obj->b_ptr, obj->b_size);
        obj->b_ptr = ptr;
        obj->b_size = size;
    } else {
        void * ptr = PyMem_Realloc(obj->b_ptr, size);
        if (ptr == NULL)
            return PyErr_NoMemory();
        obj->b_ptr = ptr;
        obj->b_size = size;
    }
  done:
    Py_RETURN_NONE;
}

static PyObject *
unpickle(PyObject *self, PyObject *args)
{
    PyObject *typ, *state, *meth, *obj, *result;
    _Py_IDENTIFIER(__new__);
    _Py_IDENTIFIER(__setstate__);

    if (!PyArg_ParseTuple(args, "OO!", &typ, &PyTuple_Type, &state))
        return NULL;
    obj = _PyObject_CallMethodIdOneArg(typ, &PyId___new__, typ);
    if (obj == NULL)
        return NULL;

    meth = _PyObject_GetAttrId(obj, &PyId___setstate__);
    if (meth == NULL) {
        goto error;
    }

    result = PyObject_Call(meth, state, NULL);
    Py_DECREF(meth);
    if (result == NULL) {
        goto error;
    }
    Py_DECREF(result);

    return obj;

error:
    Py_DECREF(obj);
    return NULL;
}

static PyObject *
POINTER(PyObject *self, PyObject *cls)
{
    PyObject *result;
    PyTypeObject *typ;
    PyObject *key;
    char *buf;

    result = PyDict_GetItemWithError(_ctypes_ptrtype_cache, cls);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    else if (PyErr_Occurred()) {
        return NULL;
    }
    if (PyUnicode_CheckExact(cls)) {
        const char *name = PyUnicode_AsUTF8(cls);
        if (name == NULL)
            return NULL;
        buf = PyMem_Malloc(strlen(name) + 3 + 1);
        if (buf == NULL)
            return PyErr_NoMemory();
        sprintf(buf, "LP_%s", name);
        result = PyObject_CallFunction((PyObject *)Py_TYPE(&PyCPointer_Type),
                                       "s(O){}",
                                       buf,
                                       &PyCPointer_Type);
        PyMem_Free(buf);
        if (result == NULL)
            return result;
        key = PyLong_FromVoidPtr(result);
        if (key == NULL) {
            Py_DECREF(result);
            return NULL;
        }
    } else if (PyType_Check(cls)) {
        typ = (PyTypeObject *)cls;
        buf = PyMem_Malloc(strlen(typ->tp_name) + 3 + 1);
        if (buf == NULL)
            return PyErr_NoMemory();
        sprintf(buf, "LP_%s", typ->tp_name);
        result = PyObject_CallFunction((PyObject *)Py_TYPE(&PyCPointer_Type),
                                       "s(O){sO}",
                                       buf,
                                       &PyCPointer_Type,
                                       "_type_", cls);
        PyMem_Free(buf);
        if (result == NULL)
            return result;
        Py_INCREF(cls);
        key = cls;
    } else {
        PyErr_SetString(PyExc_TypeError, "must be a ctypes type");
        return NULL;
    }
    if (-1 == PyDict_SetItem(_ctypes_ptrtype_cache, key, result)) {
        Py_DECREF(result);
        Py_DECREF(key);
        return NULL;
    }
    Py_DECREF(key);
    return result;
}

static PyObject *
pointer(PyObject *self, PyObject *arg)
{
    PyObject *result;
    PyObject *typ;

    typ = PyDict_GetItemWithError(_ctypes_ptrtype_cache, (PyObject *)Py_TYPE(arg));
    if (typ) {
        return PyObject_CallOneArg(typ, arg);
    }
    else if (PyErr_Occurred()) {
        return NULL;
    }
    typ = POINTER(NULL, (PyObject *)Py_TYPE(arg));
    if (typ == NULL)
        return NULL;
    result = PyObject_CallOneArg(typ, arg);
    Py_DECREF(typ);
    return result;
}

static PyObject *
buffer_info(PyObject *self, PyObject *arg)
{
    StgDictObject *dict = PyType_stgdict(arg);
    PyObject *shape;
    Py_ssize_t i;

    if (dict == NULL)
        dict = PyObject_stgdict(arg);
    if (dict == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "not a ctypes type or object");
        return NULL;
    }
    shape = PyTuple_New(dict->ndim);
    if (shape == NULL)
        return NULL;
    for (i = 0; i < (int)dict->ndim; ++i)
        PyTuple_SET_ITEM(shape, i, PyLong_FromSsize_t(dict->shape[i]));

    if (PyErr_Occurred()) {
        Py_DECREF(shape);
        return NULL;
    }
    return Py_BuildValue("siN", dict->format, dict->ndim, shape);
}

PyMethodDef _ctypes_module_methods[] = {
    {"get_errno", get_errno, METH_NOARGS},
    {"set_errno", set_errno, METH_VARARGS},
    {"POINTER", POINTER, METH_O },
    {"pointer", pointer, METH_O },
    {"_unpickle", unpickle, METH_VARARGS },
    {"buffer_info", buffer_info, METH_O, "Return buffer interface information"},
    {"resize", resize, METH_VARARGS, "Resize the memory buffer of a ctypes instance"},
#ifdef MS_WIN32
    {"get_last_error", get_last_error, METH_NOARGS},
    {"set_last_error", set_last_error, METH_VARARGS},
    {"CopyComPointer", copy_com_pointer, METH_VARARGS, copy_com_pointer_doc},
    {"FormatError", format_error, METH_VARARGS, format_error_doc},
    {"LoadLibrary", load_library, METH_VARARGS, load_library_doc},
    {"FreeLibrary", free_library, METH_VARARGS, free_library_doc},
    {"_check_HRESULT", check_hresult, METH_VARARGS},
#else
    {"dlopen", py_dl_open, METH_VARARGS,
     "dlopen(name, flag={RTLD_GLOBAL|RTLD_LOCAL}) open a shared library"},
    {"dlclose", py_dl_close, METH_VARARGS, "dlclose a library"},
    {"dlsym", py_dl_sym, METH_VARARGS, "find symbol in shared library"},
#endif
    {"alignment", align_func, METH_O, alignment_doc},
    {"sizeof", sizeof_func, METH_O, sizeof_doc},
    {"byref", byref, METH_VARARGS, byref_doc},
    {"addressof", addressof, METH_O, addressof_doc},
    {"call_function", call_function, METH_VARARGS },
    {"call_cdeclfunction", call_cdeclfunction, METH_VARARGS },
    {"PyObj_FromPtr", My_PyObj_FromPtr, METH_VARARGS },
    {"Py_INCREF", My_Py_INCREF, METH_O },
    {"Py_DECREF", My_Py_DECREF, METH_O },
    {NULL,      NULL}        /* Sentinel */
};

/*
 Local Variables:
 compile-command: "cd .. && python setup.py -q build -g && python setup.py -q build install --home ~"
 End:
*/
