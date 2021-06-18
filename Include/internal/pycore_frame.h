#ifndef Py_INTERNAL_FRAME_H
#define Py_INTERNAL_FRAME_H
#ifdef __cplusplus
extern "C" {
#endif

enum {
    FRAME_SPECIALS_GLOBALS_OFFSET = 0,
    FRAME_SPECIALS_BUILTINS_OFFSET = 1,
    FRAME_SPECIALS_LOCALS_OFFSET = 2,
    FRAME_SPECIALS_CODE_OFFSET = 3,
    FRAME_SPECIALS_SIZE = 4
};

static inline PyObject **
_PyFrame_Specials(PyFrameObject *f) {
    return &f->f_valuestack[-FRAME_SPECIALS_SIZE];
}

/* Returns a *borrowed* reference. */
static inline PyObject *
_PyFrame_GetGlobals(PyFrameObject *f)
{
    return _PyFrame_Specials(f)[FRAME_SPECIALS_GLOBALS_OFFSET];
}

/* Returns a *borrowed* reference. */
static inline PyObject *
_PyFrame_GetBuiltins(PyFrameObject *f)
{
    return _PyFrame_Specials(f)[FRAME_SPECIALS_BUILTINS_OFFSET];
}

/* Returns a *borrowed* reference. */
static inline PyCodeObject *
_PyFrame_GetCode(PyFrameObject *f)
{
    return (PyCodeObject *)_PyFrame_Specials(f)[FRAME_SPECIALS_CODE_OFFSET];
}

int _PyFrame_TakeLocals(PyFrameObject *f);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAME_H */
