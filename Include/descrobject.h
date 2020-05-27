/* Descriptors */
#ifndef Py_DESCROBJECT_H
#define Py_DESCROBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef PyObject *(*getter)(PyObject *, void *);
typedef int (*setter)(PyObject *, PyObject *, void *);

typedef struct PyGetSetDef {
    const char *name;
    getter get;
    setter set;
    const char *doc;
    void *closure;
} PyGetSetDef;

/* Interface to map C struct members to Python object attributes */

/* An array of PyMemberDef structures defines the name, type and offset
   of selected members of a C structure.  These can be read by
   PyMember_GetOne() and set by PyMember_SetOne() (except if their READONLY
   flag is set).  The array must be terminated with an entry whose name
   pointer is NULL. */

typedef struct PyMemberDef {
    const char *name;
    int type;
    Py_ssize_t offset;
    int flags;
    const char *doc;
} PyMemberDef;

/* PyMemberDef Types */
#define T_SHORT     0
#define T_INT       1
#define T_LONG      2
#define T_FLOAT     3
#define T_DOUBLE    4
#define T_STRING    5
#define T_OBJECT    6
/* XXX the ordering here is weird for binary compatibility */
#define T_CHAR      7   /* 1-character string */
#define T_BYTE      8   /* 8-bit signed int */
/* unsigned variants: */
#define T_UBYTE     9
#define T_USHORT    10
#define T_UINT      11
#define T_ULONG     12

/* Added by Jack: strings contained in the structure */
#define T_STRING_INPLACE    13

/* Added by Lillo: bools contained in the structure (assumed char) */
#define T_BOOL      14

#define T_OBJECT_EX 16  /* Like T_OBJECT, but raises AttributeError
                           when the value is NULL, instead of
                           converting to None. */
#define T_LONGLONG      17
#define T_ULONGLONG     18

#define T_PYSSIZET      19      /* Py_ssize_t */
#define T_NONE          20      /* Value is always None */


/* PyMemberDef Flags */
#define PY_READONLY        1
#define PY_READ_RESTRICTED 2


#ifndef Py_LIMITED_API
typedef PyObject *(*wrapperfunc)(PyObject *self, PyObject *args,
                                 void *wrapped);

typedef PyObject *(*wrapperfunc_kwds)(PyObject *self, PyObject *args,
                                      void *wrapped, PyObject *kwds);

struct wrapperbase {
    const char *name;
    int offset;
    void *function;
    wrapperfunc wrapper;
    const char *doc;
    int flags;
    PyObject *name_strobj;
};

/* Flags for above struct */
#define PyWrapperFlag_KEYWORDS 1 /* wrapper function takes keyword args */

/* Various kinds of descriptor objects */

typedef struct {
    PyObject_HEAD
    PyTypeObject *d_type;
    PyObject *d_name;
    PyObject *d_qualname;
} PyDescrObject;

#define PyDescr_COMMON PyDescrObject d_common

#define PyDescr_TYPE(x) (((PyDescrObject *)(x))->d_type)
#define PyDescr_NAME(x) (((PyDescrObject *)(x))->d_name)

typedef struct {
    PyDescr_COMMON;
    PyMethodDef *d_method;
    vectorcallfunc vectorcall;
} PyMethodDescrObject;

typedef struct {
    PyDescr_COMMON;
    struct PyMemberDef *d_member;
} PyMemberDescrObject;

typedef struct {
    PyDescr_COMMON;
    PyGetSetDef *d_getset;
} PyGetSetDescrObject;

typedef struct {
    PyDescr_COMMON;
    struct wrapperbase *d_base;
    void *d_wrapped; /* This can be any function pointer */
} PyWrapperDescrObject;
#endif /* Py_LIMITED_API */

PyAPI_DATA(PyTypeObject) PyClassMethodDescr_Type;
PyAPI_DATA(PyTypeObject) PyGetSetDescr_Type;
PyAPI_DATA(PyTypeObject) PyMemberDescr_Type;
PyAPI_DATA(PyTypeObject) PyMethodDescr_Type;
PyAPI_DATA(PyTypeObject) PyWrapperDescr_Type;
PyAPI_DATA(PyTypeObject) PyDictProxy_Type;
#ifndef Py_LIMITED_API
PyAPI_DATA(PyTypeObject) _PyMethodWrapper_Type;
#endif /* Py_LIMITED_API */

PyAPI_FUNC(PyObject *) PyDescr_NewMethod(PyTypeObject *, PyMethodDef *);
PyAPI_FUNC(PyObject *) PyDescr_NewClassMethod(PyTypeObject *, PyMethodDef *);
struct PyMemberDef; /* forward declaration for following prototype */
PyAPI_FUNC(PyObject *) PyDescr_NewMember(PyTypeObject *,
                                               struct PyMemberDef *);
PyAPI_FUNC(PyObject *) PyDescr_NewGetSet(PyTypeObject *,
                                               struct PyGetSetDef *);
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) PyDescr_NewWrapper(PyTypeObject *,
                                                struct wrapperbase *, void *);
#define PyDescr_IsData(d) (Py_TYPE(d)->tp_descr_set != NULL)
#endif

PyAPI_FUNC(PyObject *) PyDictProxy_New(PyObject *);
PyAPI_FUNC(PyObject *) PyWrapper_New(PyObject *, PyObject *);

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) PyMember_GetOne(const char *, struct PyMemberDef *);
PyAPI_FUNC(int) PyMember_SetOne(char *, struct PyMemberDef *, PyObject *);
#endif

PyAPI_DATA(PyTypeObject) PyProperty_Type;
#ifdef __cplusplus
}
#endif
#endif /* !Py_DESCROBJECT_H */

