#include "parts.h"
#include <stddef.h>   // for offsetof()


// This defines two classes that contain all the simple member types, one
// using "new" Py_-prefixed API, and the other using "old" <structmember.h>.
// They should behave identically in Python.

typedef struct {
    char bool_member;
    char byte_member;
    unsigned char ubyte_member;
    short short_member;
    unsigned short ushort_member;
    int int_member;
    unsigned int uint_member;
    long long_member;
    unsigned long ulong_member;
    Py_ssize_t pyssizet_member;
    float float_member;
    double double_member;
    char inplace_member[6];
    long long longlong_member;
    unsigned long long ulonglong_member;
} all_structmembers;

typedef struct {
    PyObject_HEAD
    all_structmembers structmembers;
} test_structmembers;


static struct PyMemberDef test_members_newapi[] = {
    {"T_BOOL", Py_T_BOOL, offsetof(test_structmembers, structmembers.bool_member), 0, NULL},
    {"T_BYTE", Py_T_BYTE, offsetof(test_structmembers, structmembers.byte_member), 0, NULL},
    {"T_UBYTE", Py_T_UBYTE, offsetof(test_structmembers, structmembers.ubyte_member), 0, NULL},
    {"T_SHORT", Py_T_SHORT, offsetof(test_structmembers, structmembers.short_member), 0, NULL},
    {"T_USHORT", Py_T_USHORT, offsetof(test_structmembers, structmembers.ushort_member), 0, NULL},
    {"T_INT", Py_T_INT, offsetof(test_structmembers, structmembers.int_member), 0, NULL},
    {"T_UINT", Py_T_UINT, offsetof(test_structmembers, structmembers.uint_member), 0, NULL},
    {"T_LONG", Py_T_LONG, offsetof(test_structmembers, structmembers.long_member), 0, NULL},
    {"T_ULONG", Py_T_ULONG, offsetof(test_structmembers, structmembers.ulong_member), 0, NULL},
    {"T_PYSSIZET", Py_T_PYSSIZET, offsetof(test_structmembers, structmembers.pyssizet_member), 0, NULL},
    {"T_FLOAT", Py_T_FLOAT, offsetof(test_structmembers, structmembers.float_member), 0, NULL},
    {"T_DOUBLE", Py_T_DOUBLE, offsetof(test_structmembers, structmembers.double_member), 0, NULL},
    {"T_STRING_INPLACE", Py_T_STRING_INPLACE, offsetof(test_structmembers, structmembers.inplace_member), 0, NULL},
    {"T_LONGLONG", Py_T_LONGLONG, offsetof(test_structmembers, structmembers.longlong_member), 0, NULL},
    {"T_ULONGLONG", Py_T_ULONGLONG, offsetof(test_structmembers, structmembers.ulonglong_member), 0, NULL},
    {NULL}
};

static PyObject *
test_structmembers_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {
        "T_BOOL", "T_BYTE", "T_UBYTE", "T_SHORT", "T_USHORT",
        "T_INT", "T_UINT", "T_LONG", "T_ULONG", "T_PYSSIZET",
        "T_FLOAT", "T_DOUBLE", "T_STRING_INPLACE",
        "T_LONGLONG", "T_ULONGLONG",
        NULL};
    static const char fmt[] = "|bbBhHiIlknfds#LK";
    test_structmembers *ob;
    const char *s = NULL;
    Py_ssize_t string_len = 0;
    ob = PyObject_New(test_structmembers, type);
    if (ob == NULL) {
        return NULL;
    }
    memset(&ob->structmembers, 0, sizeof(all_structmembers));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, fmt, keywords,
                                     &ob->structmembers.bool_member,
                                     &ob->structmembers.byte_member,
                                     &ob->structmembers.ubyte_member,
                                     &ob->structmembers.short_member,
                                     &ob->structmembers.ushort_member,
                                     &ob->structmembers.int_member,
                                     &ob->structmembers.uint_member,
                                     &ob->structmembers.long_member,
                                     &ob->structmembers.ulong_member,
                                     &ob->structmembers.pyssizet_member,
                                     &ob->structmembers.float_member,
                                     &ob->structmembers.double_member,
                                     &s, &string_len,
                                     &ob->structmembers.longlong_member,
                                     &ob->structmembers.ulonglong_member))
    {
        Py_DECREF(ob);
        return NULL;
    }
    if (s != NULL) {
        if (string_len > 5) {
            Py_DECREF(ob);
            PyErr_SetString(PyExc_ValueError, "string too long");
            return NULL;
        }
        strcpy(ob->structmembers.inplace_member, s);
    }
    else {
        strcpy(ob->structmembers.inplace_member, "");
    }
    return (PyObject *)ob;
}

static PyType_Slot test_structmembers_slots[] = {
    {Py_tp_new, test_structmembers_new},
    {Py_tp_members, test_members_newapi},
    {0},
};

static PyType_Spec test_structmembers_spec = {
    .name = "_testcapi._test_structmembersType_NewAPI",
    .flags = Py_TPFLAGS_DEFAULT,
    .basicsize = sizeof(test_structmembers),
    .slots = test_structmembers_slots,
};

#include <structmember.h>

static struct PyMemberDef test_members[] = {
    {"T_BOOL", T_BOOL, offsetof(test_structmembers, structmembers.bool_member), 0, NULL},
    {"T_BYTE", T_BYTE, offsetof(test_structmembers, structmembers.byte_member), 0, NULL},
    {"T_UBYTE", T_UBYTE, offsetof(test_structmembers, structmembers.ubyte_member), 0, NULL},
    {"T_SHORT", T_SHORT, offsetof(test_structmembers, structmembers.short_member), 0, NULL},
    {"T_USHORT", T_USHORT, offsetof(test_structmembers, structmembers.ushort_member), 0, NULL},
    {"T_INT", T_INT, offsetof(test_structmembers, structmembers.int_member), 0, NULL},
    {"T_UINT", T_UINT, offsetof(test_structmembers, structmembers.uint_member), 0, NULL},
    {"T_LONG", T_LONG, offsetof(test_structmembers, structmembers.long_member), 0, NULL},
    {"T_ULONG", T_ULONG, offsetof(test_structmembers, structmembers.ulong_member), 0, NULL},
    {"T_PYSSIZET", T_PYSSIZET, offsetof(test_structmembers, structmembers.pyssizet_member), 0, NULL},
    {"T_FLOAT", T_FLOAT, offsetof(test_structmembers, structmembers.float_member), 0, NULL},
    {"T_DOUBLE", T_DOUBLE, offsetof(test_structmembers, structmembers.double_member), 0, NULL},
    {"T_STRING_INPLACE", T_STRING_INPLACE, offsetof(test_structmembers, structmembers.inplace_member), 0, NULL},
    {"T_LONGLONG", T_LONGLONG, offsetof(test_structmembers, structmembers.longlong_member), 0, NULL},
    {"T_ULONGLONG", T_ULONGLONG, offsetof(test_structmembers, structmembers.ulonglong_member), 0, NULL},
    {NULL}
};


static void
test_structmembers_free(PyObject *ob)
{
    PyObject_Free(ob);
}

/* Designated initializers would work too, but this does test the *old* API */
static PyTypeObject test_structmembersType_OldAPI= {
    PyVarObject_HEAD_INIT(NULL, 0)
    "test_structmembersType_OldAPI",
    sizeof(test_structmembers),         /* tp_basicsize */
    0,                                  /* tp_itemsize */
    test_structmembers_free,            /* destructor tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    PyObject_GenericSetAttr,            /* tp_setattro */
    0,                                  /* tp_as_buffer */
    0,                                  /* tp_flags */
    "Type containing all structmember types",
    0,                                  /* traverseproc tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    test_members,                       /* tp_members */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    test_structmembers_new,             /* tp_new */
};


int
_PyTestCapi_Init_Structmember(PyObject *m)
{
    int res;
    res = PyType_Ready(&test_structmembersType_OldAPI);
    if (res < 0) {
        return -1;
    }
    res = PyModule_AddObjectRef(
        m,
        "_test_structmembersType_OldAPI",
        (PyObject *)&test_structmembersType_OldAPI);
    if (res < 0) {
        return -1;
    }

    PyObject *test_structmembersType_NewAPI = PyType_FromModuleAndSpec(
        m, &test_structmembers_spec, NULL);
    if (!test_structmembersType_NewAPI) {
        return -1;
    }
    res = PyModule_AddType(m, (PyTypeObject*)test_structmembersType_NewAPI);
    Py_DECREF(test_structmembersType_NewAPI);
    if (res < 0) {
        return -1;
    }

    return 0;
}
