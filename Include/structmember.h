/* Deprecated header.  `PyMemberDef`, `PyMember_GetOne` and `PyMember_SetOne`
 * were moved to `descrobject.h`. */

#ifndef Py_STRUCTMEMBER_H
#define Py_STRUCTMEMBER_H
#ifdef __cplusplus
extern "C" {
#endif

/* The following names are provided for backward compatibility. */
#define READONLY            PY_READONLY
#define READ_RESTRICTED     PY_READ_RESTRICTED
#define PY_WRITE_RESTRICTED 4
#define RESTRICTED          (READ_RESTRICTED | PY_WRITE_RESTRICTED)

#ifdef __cplusplus
}
#endif
#endif /* !Py_STRUCTMEMBER_H */
