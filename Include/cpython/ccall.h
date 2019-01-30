#ifndef Py_CPYTHON_CCALL_H
#  error "this header file must not be included directly"
#endif

/* The C call protocol (PEP 580) */

/* Unspecified function pointer.
 * See below for concrete function pointer types */
typedef void *(*PyCFunc)(void);

typedef struct {
    uint32_t  cc_flags;
    PyCFunc   cc_func;    /* C function to call */
    PyObject *cc_parent;  /* class or module or anything, may be NULL */
} PyCCallDef;

typedef struct {
    const PyCCallDef *cr_ccall;
    PyObject         *cr_self;  /* __self__ argument for methods */
} PyCCallRoot;

/* Various function pointer types used for cc_func. The letters indicate
   the kind of argument:
     D: const PyCCallDef pointer
     S: __self__
     A: *args or single argument
     V: C array for METH_FASTCALL (2 args: pointer and length)
     K: **kwargs
   */
typedef PyObject *(*PyCFunc_SA)(PyObject *, PyObject *);
typedef PyObject *(*PyCFunc_SAK)(PyObject *, PyObject *, PyObject *);
typedef PyObject *(*PyCFunc_SV)(PyObject *, PyObject *const *, Py_ssize_t);
typedef PyObject *(*PyCFunc_SVK)(PyObject *, PyObject *const *, Py_ssize_t, PyObject *);
typedef PyObject *(*PyCFunc_DS)(const PyCCallDef *, PyObject *);
typedef PyObject *(*PyCFunc_DSA)(const PyCCallDef *, PyObject *, PyObject *);
typedef PyObject *(*PyCFunc_DSAK)(const PyCCallDef *, PyObject *, PyObject *, PyObject *);
typedef PyObject *(*PyCFunc_DSV)(const PyCCallDef *, PyObject *, PyObject *const *, Py_ssize_t);
typedef PyObject *(*PyCFunc_DSVK)(const PyCCallDef *, PyObject *, PyObject *const *, Py_ssize_t, PyObject *);

PyAPI_FUNC(int) _PyCCallDef_FromMethodDef(PyCCallDef *cc,
                                          const PyMethodDef *ml,
                                          PyObject *parent);

/* Base signature: one of 4 possibilities, exactly one must be given. */
#define CCALL_VARARGS          0x00000000
#define CCALL_FASTCALL         0x00000002
#define CCALL_NOARGS           0x00000004
#define CCALL_O                0x00000006
#define CCALL_BASESIGNATURE    (CCALL_VARARGS | CCALL_FASTCALL | CCALL_O | CCALL_NOARGS)

/* Two extra signature flags: CCALL_KEYWORDS can be combined with CCALL_VARARGS
   and CCALL_FASTCALL, while CCALL_DEFARG can be combined with anything. */
#define CCALL_KEYWORDS         0x00000008
#define CCALL_DEFARG           0x00000001
#define CCALL_SIGNATURE        (CCALL_BASESIGNATURE | CCALL_KEYWORDS | CCALL_DEFARG)

/* The rationale for the numerical values of the signature flags is as follows:
   - the valid combinations give precisely the range 0, ..., 11. This allows
     the compiler to implement the main switch statement in PyCCall_FastCall()
     using a jump table.
   - in a few places we are explicitly checking for CCALL_VARARGS, which is
     fastest when CCALL_VARARGS == 0
*/

/* Other flags, these are single bits */
#define CCALL_OBJCLASS         0x00000010
#define CCALL_SELFARG          0x00000020

/* Hack to add a special error message for print >> f */
#define _CCALL_BUILTIN_PRINT   0x00010000

/* In order to "support" PyCFunction_GetFlags(), we store the METH_xxx flags
   from the PyMethodDef in the top 12 bits of cc_flags. At the time that
   PyCFunction_GetFlags was deprecated, only the lower 9 bits of ml_flags
   were used, so 12 bits is sufficient. */
#define _CCALL_METH_FLAGS      0x00100000

#define PyCCall_Check(op) (Py_TYPE(op)->tp_flags & Py_TPFLAGS_HAVE_CCALL)

PyAPI_FUNC(PyObject *) PyCCall_Call(PyObject *func, PyObject *args, PyObject *kwds);
PyAPI_FUNC(PyObject *) PyCCall_FastCall(
    PyObject *func,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *keywords);

#define PyCCall_CCALLROOT(func) ((const PyCCallRoot*)(((char*)func) + Py_TYPE(func)->tp_ccalloffset))
#define PyCCall_CCALLDEF(func) (PyCCall_CCALLROOT(func)->cr_ccall)
#define PyCCall_SELF(func) (PyCCall_CCALLROOT(func)->cr_self)
#define PyCCall_FLAGS(func) (PyCCall_CCALLDEF(func)->cc_flags)

PyAPI_FUNC(PyObject *) PyCCall_GenericGetQualname(PyObject *, void *);
PyAPI_FUNC(PyObject *) PyCCall_GenericGetParent(PyObject *, void *);
