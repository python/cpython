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

#ifdef __cplusplus
}
#endif
#endif /* !Py_FRAMEOBJECT_H */
