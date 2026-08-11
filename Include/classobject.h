/* Limited C API of PyMethod API
 */

#ifndef Py_CLASSOBJECT_H
#define Py_CLASSOBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03100000
/* Return a new method object, with `func` being any callable object and
 * `self` the instance the method should be bound.
 *
 * `func` is the function that will be called when the method is called.
 * `self` must not be NULL.
 */
PyAPI_FUNC(PyObject *) PyMethod_New(PyObject *, PyObject *);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_CLASSOBJECT_H */
