#ifndef Py_INTERNAL_FRAMEOBJECT_H
#define Py_INTERNAL_FRAMEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

struct _frame;
typedef struct _frame PyFrameObject;

void _PyFrame_PostEvalCleanup(PyFrameObject *f);     /* For _PyEval_EvalFrame() */

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAMEOBJECT_H */
