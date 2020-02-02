/* Frame object interface */

#ifndef Py_FRAMEOBJECT_H
#define Py_FRAMEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/* There are currently no frame related APIs in the stable ABI
 * (they're all in the full CPython-specific API)
 */

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_FRAMEOBJECT_H
#  include  "cpython/frameobject.h"
#  undef Py_CPYTHON_FRAMEOBJECT_H
#endif


// TODO: Replace this with the revised API described in
// https://discuss.python.org/t/pep-558-defined-semantics-for-locals/2936/11
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03090000
/* Fast locals proxy for reliable write-through from trace functions */
PyTypeObject PyFastLocalsProxy_Type;
#define _PyFastLocalsProxy_CheckExact(self) \
    (Py_TYPE(self) == &PyFastLocalsProxy_Type)

/* Access the frame locals mapping */
PyAPI_FUNC(PyObject *) PyFrame_GetPyLocals(PyFrameObject *); // = locals()
PyAPI_FUNC(PyObject *) PyFrame_GetLocalsAttribute(PyFrameObject *);  // = frame.f_locals
#endif



#ifdef __cplusplus
}
#endif
#endif /* !Py_FRAMEOBJECT_H */
