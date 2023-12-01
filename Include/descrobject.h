/* Descriptors */
#ifndef Py_DESCROBJECT_H
#define Py_DESCROBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef PyObject *(*getter)(PyObject *, void *);
typedef int (*setter)(PyObject *, PyObject *, void *);

struct PyGetSetDef {
    const char *name;
    getter get;
    setter set;
    const char *doc;
    void *closure;
};

PyAPI_DATA(PyTypeObject) PyClassMethodDescr_Type;
PyAPI_DATA(PyTypeObject) PyGetSetDescr_Type;
PyAPI_DATA(PyTypeObject) PyMemberDescr_Type;
PyAPI_DATA(PyTypeObject) PyMethodDescr_Type;
PyAPI_DATA(PyTypeObject) PyWrapperDescr_Type;
PyAPI_DATA(PyTypeObject) PyDictProxy_Type;
PyAPI_DATA(PyTypeObject) PyProperty_Type;

PyAPI_FUNC(PyObject *) PyDescr_NewMethod(PyTypeObject *, PyMethodDef *);
PyAPI_FUNC(PyObject *) PyDescr_NewClassMethod(PyTypeObject *, PyMethodDef *);
PyAPI_FUNC(PyObject *) PyDescr_NewMember(PyTypeObject *, PyMemberDef *);
PyAPI_FUNC(PyObject *) PyDescr_NewGetSet(PyTypeObject *, PyGetSetDef *);

PyAPI_FUNC(PyObject *) PyDictProxy_New(PyObject *);
PyAPI_FUNC(PyObject *) PyWrapper_New(PyObject *, PyObject *);


/* An array of PyMemberDef structures defines the name, type and offset
   of selected members of a C structure.  These can be read by
   PyMember_GetOne() and set by PyMember_SetOne() (except if their READONLY
   flag is set).  The array must be terminated with an entry whose name
   pointer is NULL. */
struct PyMemberDef {
    const char *name;
    int type;
    Py_ssize_t offset;
    int flags;
    const char *doc;
};

// These constants used to be in structmember.h, not prefixed by Py_.
// (structmember.h now has aliases to the new names.)

/* Types */
#define Py_T_SHORT     0
#define Py_T_INT       1
#define Py_T_LONG      2
#define Py_T_FLOAT     3
#define Py_T_DOUBLE    4
#define Py_T_STRING    5
#define _Py_T_OBJECT   6  // Deprecated, use Py_T_OBJECT_EX instead
/* the ordering here is weird for binary compatibility */
#define Py_T_CHAR      7   /* 1-character string */
#define Py_T_BYTE      8   /* 8-bit signed int */
/* unsigned variants: */
#define Py_T_UBYTE     9
#define Py_T_USHORT    10
#define Py_T_UINT      11
#define Py_T_ULONG     12

/* Added by Jack: strings contained in the structure */
#define Py_T_STRING_INPLACE    13

/* Added by Lillo: bools contained in the structure (assumed char) */
#define Py_T_BOOL      14

#define Py_T_OBJECT_EX 16
#define Py_T_LONGLONG  17
#define Py_T_ULONGLONG 18

#define Py_T_PYSSIZET  19      /* Py_ssize_t */
#define _Py_T_NONE     20 // Deprecated. Value is always None.

/* Flags */
#define Py_READONLY            1
#define Py_AUDIT_READ          2 // Added in 3.10, harmless no-op before that
#define _Py_WRITE_RESTRICTED   4 // Deprecated, no-op. Do not reuse the value.
#define Py_RELATIVE_OFFSET     8

PyAPI_FUNC(PyObject *) PyMember_GetOne(const char *, PyMemberDef *);
PyAPI_FUNC(int) PyMember_SetOne(char *, PyMemberDef *, PyObject *);

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_DESCROBJECT_H
#  include "cpython/descrobject.h"
#  undef Py_CPYTHON_DESCROBJECT_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_DESCROBJECT_H */
