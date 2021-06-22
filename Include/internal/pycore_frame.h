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
    int lasti;       /* Last instruction if called */
    int stackdepth;  /* Depth of value stack */
    PyObject *stack[1];
} _PyFrame;

#define FRAME_SPECIALS_SIZE ((sizeof(_PyFrame)-1)/sizeof(PyObject *))

int _PyFrame_TakeLocals(PyFrameObject *f);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAME_H */
