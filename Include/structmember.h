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
#define T_SHORT     PY_T_SHORT
#define T_INT       PY_T_INT
#define T_LONG      PY_T_LONG
#define T_FLOAT     PY_T_FLOAT
#define T_DOUBLE    PY_T_DOUBLE
#define T_STRING    PY_T_STRING
#define T_OBJECT    _PY_T_OBJECT
#define T_CHAR      PY_T_CHAR
#define T_BYTE      PY_T_BYTE
#define T_UBYTE     PY_T_UBYTE
#define T_USHORT    PY_T_USHORT
#define T_UINT      PY_T_UINT
#define T_ULONG     PY_T_ULONG
#define T_STRING_INPLACE    PY_T_STRING_INPLACE
#define T_BOOL      PY_T_BOOL
#define T_OBJECT_EX PY_T_OBJECT_EX
#define T_LONGLONG  PY_T_LONGLONG
#define T_ULONGLONG PY_T_ULONGLONG
#define T_PYSSIZET  PY_T_PYSSIZET
#define T_NONE      _PY_T_NONE

/* Flags */
#define READONLY            PY_READONLY
#define READ_RESTRICTED     PY_AUDIT_READ
#define PY_WRITE_RESTRICTED _PY_WRITE_RESTRICTED
#define RESTRICTED          (READ_RESTRICTED | PY_WRITE_RESTRICTED)


#ifdef __cplusplus
}
#endif
#endif /* !Py_STRUCTMEMBER_H */
