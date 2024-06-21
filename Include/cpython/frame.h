/* Interpreter Frame */

#ifndef Py_CPYTHON_FRAME_H
#  error "this header file must not be included directly"
#endif

typedef struct _PyInterpreterFrame {
    PyObject *f_executable; /* Strong reference (code object or None) */
    struct _PyInterpreterFrame *previous;
    PyObject *f_funcobj; /* Strong reference. Only valid if not on C stack */
    PyObject *f_globals; /* Borrowed reference. Only valid if not on C stack */
    PyObject *f_builtins; /* Borrowed reference. Only valid if not on C stack */
    PyObject *f_locals; /* Strong reference, may be NULL. Only valid if not on C stack */
    PyFrameObject *frame_obj; /* Strong reference, may be NULL. Only valid if not on C stack */
    _Py_CODEUNIT *instr_ptr; /* Instruction currently executing (or about to begin) */
    int stacktop;  /* Offset of TOS from localsplus  */
    uint16_t return_offset;  /* Only relevant during a function call */
    char owner;
    /* Locals and stack */
    PyObject *localsplus[1];
} _PyInterpreterFrame;
