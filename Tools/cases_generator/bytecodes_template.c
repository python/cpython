#include "Python.h"

#include "opcode.h"
#include "pycore_atomic.h"
#include "pycore_frame.h"

void _PyFloat_ExactDealloc(PyObject *);
void _PyUnicode_ExactDealloc(PyObject *);

#define SET_TOP(v)        (stack_pointer[-1] = (v))
#define GETLOCAL(i)     (frame->localsplus[i])

#define inst(name, stack_effect) case name:
#define family(name) static int family_##name

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
    PyObject **stack_pointer
)
{
    switch (opcode) {

// BEGIN BYTECODES //
// INSERT CASES HERE //
// END BYTECODES //

    }
 handle_eval_breaker:;
 unbound_local_error:;
 error:;
}

// Families go below this point //

