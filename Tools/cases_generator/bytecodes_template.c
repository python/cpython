#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_call.h"          // _PyObject_FastCallDictTstate()
#include "pycore_ceval.h"         // _PyEval_SignalAsyncExc()
#include "pycore_code.h"
#include "pycore_function.h"
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_moduleobject.h"  // PyModuleObject
#include "pycore_opcode.h"        // EXTRA_CASES
#include "pycore_pyerrors.h"      // _PyErr_Fetch()
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_range.h"         // _PyRangeIterObject
#include "pycore_sliceobject.h"   // _PyBuildSlice_ConsumeRefs
#include "pycore_sysmodule.h"     // _PySys_Audit()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_emscripten_signal.h"  // _Py_CHECK_EMSCRIPTEN_SIGNALS

#include "pycore_dict.h"
#include "dictobject.h"
#include "pycore_frame.h"
#include "opcode.h"
#include "pydtrace.h"
#include "setobject.h"
#include "structmember.h"         // struct PyMemberDef, T_OFFSET_EX

void _PyFloat_ExactDealloc(PyObject *);
void _PyUnicode_ExactDealloc(PyObject *);

#define SET_TOP(v)        (stack_pointer[-1] = (v))
#define PEEK(n)           (stack_pointer[-(n)])

#define GETLOCAL(i)     (frame->localsplus[i])

#define inst(name) case name:
#define family(name) static int family_##name

#define NAME_ERROR_MSG \
    "name '%.200s' is not defined"

typedef struct {
    PyObject *kwnames;
} CallShape;

static void
dummy_func(
    PyThreadState *tstate,
    _PyInterpreterFrame *frame,
    unsigned char opcode,
    unsigned int oparg,
    _Py_atomic_int * const eval_breaker,
    _PyCFrame cframe,
    PyObject *names,
    PyObject *consts,
    _Py_CODEUNIT *next_instr,
    PyObject **stack_pointer,
    CallShape call_shape,
    _Py_CODEUNIT *first_instr,
    int throwflag,
    binaryfunc binary_ops[]
)
{
    switch (opcode) {

        /* BEWARE!
           It is essential that any operation that fails must goto error
           and that all operation that succeed call DISPATCH() ! */

// BEGIN BYTECODES //
// INSERT CASES HERE //
// END BYTECODES //

    }
 error:;
 exception_unwind:;
 handle_eval_breaker:;
 resume_frame:;
 resume_with_error:;
 start_frame:;
 unbound_local_error:;
}

// Families go below this point //

