/* This extension module exposes some types that are only available at the
 * C level.  It should not be used directly, but instead through the Python
 * level types modules, which imports this.
 */

#include "Python.h"
#include "structmember.h"

typedef struct
{
    PyObject_HEAD
    int member;
} Helper;

static PyMemberDef helper_members[] = {
    { "member", T_INT,  offsetof(Helper, member), READONLY,
      PyDoc_STR("A member descriptor")
    },
    { NULL }
};

static PyObject *
helper_getter(Helper *self, void *unused) 
{
    Py_RETURN_NONE;
}

static PyGetSetDef helper_getset[] = {
    { "getter", (getter)helper_getter, NULL,
      PyDoc_STR("A getset descriptor"),
    },
    { NULL }
};

static PyTypeObject HelperType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_types.Helper",				/* tp_name */
    sizeof(Helper),                             /* tp_basicsize */
    0,						/* tp_itemsize */
    0,						/* tp_dealloc */
    0,						/* tp_print */
    0,						/* tp_getattr */
    0,						/* tp_setattr */
    0,						/* tp_compare */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,						/* tp_as_sequence */
    0,						/* tp_as_mapping */
    0,                                          /* tp_hash */
    0,              				/* tp_call */
    0,          				/* tp_str */
    0,                                          /* tp_getattro */
    0,						/* tp_setattro */
    0,						/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    0,      					/* tp_doc */
    0,						/* tp_traverse */
    0,						/* tp_clear */
    0,                                          /* tp_richcompare */
    0,						/* tp_weaklistoffset */
    0,						/* tp_iter */
    0,						/* tp_iternext */
    0,      					/* tp_methods */
    helper_members,                             /* tp_members */
    helper_getset,                              /* tp_getset */
    0,						/* tp_base */
    0,						/* tp_dict */
    0,						/* tp_descr_get */
    0,						/* tp_descr_set */
    0,						/* tp_dictoffset */
    0,						/* tp_init */
    0,						/* tp_alloc */
    0,                                          /* tp_new */
    0,						/* tp_free */
};

PyMODINIT_FUNC
init_types(void)
{
    PyObject *m;

    m = Py_InitModule3("_types", NULL, "A types module helper");
    if (!m)
        return;

    if (PyType_Ready(&HelperType) < 0)
        return;

    Py_INCREF(&HelperType);
    PyModule_AddObject(m, "Helper", (PyObject *)&HelperType);
}

    
