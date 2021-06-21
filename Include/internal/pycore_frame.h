#ifndef Py_INTERNAL_FRAME_H
#define Py_INTERNAL_FRAME_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _py_frame_specials {
    PyObject *globals;
    PyObject *builtins;
    PyObject *locals;
    PyCodeObject *code;
    int lasti;     /* Last instruction if called */
    PyObject *stack[1];
} _PyFrameSpecials;

#define FRAME_SPECIALS_SIZE ((sizeof(_PyFrameSpecials)-1)/sizeof(PyObject *))

int _PyFrame_TakeLocals(PyFrameObject *f);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAME_H */
