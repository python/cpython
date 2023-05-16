#ifndef Py_STRUCTMEMBER_H
#define Py_STRUCTMEMBER_H
#ifdef __cplusplus
extern "C" {
#endif


/* Interface to map C struct members to Python object attributes
 *
 * This header is deprecated: new code should not use stuff from here.
 * New definitions are in descrobject.h.
 *
 * However, there's nothing wrong with old code continuing to use it,
 * and there's not much mainenance overhead in maintaining a few aliases.
 * So, don't be too eager to convert old code.
 *
 * It uses names not prefixed with Py_.
 * It is also *not* included from Python.h and must be included individually.
 */

#include <stddef.h> /* For offsetof (not always provided by Python.h) */

/* Types */
#define T_SHORT     Py_T_SHORT
#define T_INT       Py_T_INT
#define T_LONG      Py_T_LONG
#define T_FLOAT     Py_T_FLOAT
#define T_DOUBLE    Py_T_DOUBLE
#define T_STRING    Py_T_STRING
#define T_OBJECT    _Py_T_OBJECT
#define T_CHAR      Py_T_CHAR
#define T_BYTE      Py_T_BYTE
#define T_UBYTE     Py_T_UBYTE
#define T_USHORT    Py_T_USHORT
#define T_UINT      Py_T_UINT
#define T_ULONG     Py_T_ULONG
#define T_STRING_INPLACE    Py_T_STRING_INPLACE
#define T_BOOL      Py_T_BOOL
#define T_OBJECT_EX Py_T_OBJECT_EX
#define T_LONGLONG  Py_T_LONGLONG
#define T_ULONGLONG Py_T_ULONGLONG
#define T_PYSSIZET  Py_T_PYSSIZET
#define T_NONE      _Py_T_NONE

/* Flags */
#define READONLY            Py_READONLY
#define PY_AUDIT_READ        Py_AUDIT_READ
#define READ_RESTRICTED     Py_AUDIT_READ
#define PY_WRITE_RESTRICTED _Py_WRITE_RESTRICTED
#define RESTRICTED          (READ_RESTRICTED | PY_WRITE_RESTRICTED)


#ifdef __cplusplus
}
#endif
#endif /* !Py_STRUCTMEMBER_H */
