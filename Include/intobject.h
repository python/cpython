/* Integer object interface

   This header files exists to make porting code to Python 3.0 easier. It
   defines aliases from PyInt_* to PyLong_*. Only PyInt_GetMax() and
   PyInt_CheckExact() remain in longobject.h.
 */

#ifndef Py_INTOBJECT_H
#define Py_INTOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#warning "DeprecationWarning: intobject.h is going to be removed in 3.1"

#define PyInt_Check(op) PyLong_Check(op)
#define PyInt_FromString PyLong_FromString
#define PyInt_FromUnicode PyLong_FromUnicode
#define PyInt_FromLong PyLong_FromLong
#define PyInt_FromSize_t PyLong_FromSize_t
#define PyInt_FromSsize_t PyLong_FromSsize_t
#define PyInt_AsLong PyLong_AsLong
#define PyInt_AsSsize_t PyLong_AsSsize_t
#define PyInt_AsUnsignedLongMask PyLong_AsUnsignedLongMask
#define PyInt_AsUnsignedLongLongMask PyLong_AsUnsignedLongLongMask
#define PyInt_AS_LONG PyLong_AS_LONG

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTOBJECT_H */
