/* Frame object interface */

#ifndef Py_FRAMEOBJECT_H
#define Py_FRAMEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pyframe.h"

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_FRAMEOBJECT_H
#  include "cpython/frameobject.h"
#  undef Py_CPYTHON_FRAMEOBJECT_H
#endif

typedef enum _framestate {
    FRAME_CREATED = -2,
    FRAME_SUSPENDED = -1,
    FRAME_EXECUTING = 0,
    FRAME_COMPLETED = 1,
    FRAME_CLEARED = 4
} PyFrameState;

PyAPI_FUNC(PyFrameState) PyFrame_GetState(PyFrameObject *frame);


#ifdef __cplusplus
}
#endif
#endif /* !Py_FRAMEOBJECT_H */
