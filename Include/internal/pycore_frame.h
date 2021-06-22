#ifndef Py_INTERNAL_FRAME_H
#define Py_INTERNAL_FRAME_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _py_frame {
    PyObject *globals;
    PyObject *builtins;
    PyObject *locals;
    PyCodeObject *code;
    PyFrameObject *frame_obj;
    int lasti;       /* Last instruction if called */
    int stackdepth;  /* Depth of value stack */
    PyObject *stack[1];
} _PyFrame;

#define FRAME_SPECIALS_SIZE ((sizeof(_PyFrame)-1)/sizeof(PyObject *))

int _PyFrame_TakeLocals(PyFrameObject *f);

static inline void
_PyFrame_InitializeSpecials(_PyFrame *frame, PyFrameConstructor *con, PyObject *locals)
{
    frame->code = (PyCodeObject *)Py_NewRef(con->fc_code);
    frame->builtins = Py_NewRef(con->fc_builtins);
    frame->globals = Py_NewRef(con->fc_globals);
    frame->locals = Py_XNewRef(locals);
    frame->frame_obj = NULL;
    frame->lasti = -1;
}



#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAME_H */
