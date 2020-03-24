
/* Testing module for multi-phase initialization of extension modules (PEP 489)
 */

#include "Python.h"

/* Example objects */
typedef struct {
    PyObject_HEAD
    PyObject            *x_attr;        /* Attributes dictionary */
} ExampleObject;

typedef struct {
    PyObject *integer;
} testmultiphase_state;

/* Example methods */

static int
Example_traverse(ExampleObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->x_attr);
    return 0;
}

static void
Example_finalize(ExampleObject *self)
{
    Py_CLEAR(self->x_attr);
}

static PyObject *
Example_demo(ExampleObject *self, PyObject *args)
{
    PyObject *o = NULL;
    if (!PyArg_ParseTuple(args, "|O:demo", &o))
        return NULL;
    if (o != NULL && PyUnicode_Check(o)) {
        Py_INCREF(o);
        return o;
    }
    Py_RETURN_NONE;
}


static PyMethodDef Example_methods[] = {
    {"demo",            (PyCFunction)Example_demo,  METH_VARARGS,
        PyDoc_STR("demo() -> None")},
    {NULL,              NULL}           /* sentinel */
};

static PyObject *
Example_getattro(ExampleObject *self, PyObject *name)
{
    if (self->x_attr != NULL) {
        PyObject *v = PyDict_GetItemWithError(self->x_attr, name);
        if (v != NULL) {
            Py_INCREF(v);
            return v;
        }
        else if (PyErr_Occurred()) {
            return NULL;
        }
    }
    return PyObject_GenericGetAttr((PyObject *)self, name);
}

static int
Example_setattr(ExampleObject *self, const char *name, PyObject *v)
{
    if (self->x_attr == NULL) {
        self->x_attr = PyDict_New();
        if (self->x_attr == NULL)
            return -1;
    }
    if (v == NULL) {
        int rv = PyDict_DelItemString(self->x_attr, name);
        if (rv < 0 && PyErr_ExceptionMatches(PyExc_KeyError))
            PyErr_SetString(PyExc_AttributeError,
                "delete non-existing Example attribute");
        return rv;
    }
    else
        return PyDict_SetItemString(self->x_attr, name, v);
}

static PyType_Slot Example_Type_slots[] = {
    {Py_tp_doc, "The Example type"},
    {Py_tp_finalize, Example_finalize},
    {Py_tp_traverse, Example_traverse},
    {Py_tp_getattro, Example_getattro},
    {Py_tp_setattr, Example_setattr},
    {Py_tp_methods, Example_methods},
    {0, 0},
};

static PyType_Spec Example_Type_spec = {
    "_testimportexec.Example",
    sizeof(ExampleObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    Example_Type_slots
};

/* Function of two integers returning integer */

PyDoc_STRVAR(testexport_foo_doc,
"foo(i,j)\n\
\n\
Return the sum of i and j.");

static PyObject *
testexport_foo(PyObject *self, PyObject *args)
{
    long i, j;
    long res;
    if (!PyArg_ParseTuple(args, "ll:foo", &i, &j))
        return NULL;
    res = i + j;
    return PyLong_FromLong(res);
}

/* Test that PyState registration fails  */

PyDoc_STRVAR(call_state_registration_func_doc,
"register_state(0): call PyState_FindModule()\n\
register_state(1): call PyState_AddModule()\n\
register_state(2): call PyState_RemoveModule()");

static PyObject *
call_state_registration_func(PyObject *mod, PyObject *args)
{
    int i, ret;
    PyModuleDef *def = PyModule_GetDef(mod);
    if (def == NULL) {
        return NULL;
    }
    if (!PyArg_ParseTuple(args, "i:call_state_registration_func", &i))
        return NULL;
    switch (i) {
        case 0:
            mod = PyState_FindModule(def);
            if (mod == NULL) {
                Py_RETURN_NONE;
            }
            return mod;
        case 1:
            ret = PyState_AddModule(mod, def);
            if (ret != 0) {
                return NULL;
            }
            break;
        case 2:
            ret = PyState_RemoveModule(def);
            if (ret != 0) {
                return NULL;
            }
            break;
    }
    Py_RETURN_NONE;
}


static PyType_Slot Str_Type_slots[] = {
    {Py_tp_base, NULL}, /* filled out in module exec function */
    {0, 0},
};

static PyType_Spec Str_Type_spec = {
    "_testimportexec.Str",
    0,
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    Str_Type_slots
};

static PyMethodDef testexport_methods[] = {
    {"foo",             testexport_foo,         METH_VARARGS,
        testexport_foo_doc},
    {"call_state_registration_func",  call_state_registration_func,
        METH_VARARGS, call_state_registration_func_doc},
    {NULL,              NULL}           /* sentinel */
};

static int execfunc(PyObject *m)
{
    PyObject *temp = NULL;

    /* Due to cross platform compiler issues the slots must be filled
     * here. It's required for portability to Windows without requiring
     * C++. */
    Str_Type_slots[0].pfunc = &PyUnicode_Type;

    /* Add a custom type */
    temp = PyType_FromSpec(&Example_Type_spec);
    if (temp == NULL)
        goto fail;
    if (PyModule_AddObject(m, "Example", temp) != 0)
        goto fail;

    /* Add an exception type */
    temp = PyErr_NewException("_testimportexec.error", NULL, NULL);
    if (temp == NULL)
        goto fail;
    if (PyModule_AddObject(m, "error", temp) != 0)
        goto fail;

    /* Add Str */
    temp = PyType_FromSpec(&Str_Type_spec);
    if (temp == NULL)
        goto fail;
    if (PyModule_AddObject(m, "Str", temp) != 0)
        goto fail;

    if (PyModule_AddIntConstant(m, "int_const", 1969) != 0)
        goto fail;

    if (PyModule_AddStringConstant(m, "str_const", "something different") != 0)
        goto fail;

    return 0;
 fail:
    return -1;
}

/* Helper for module definitions; there'll be a lot of them */

#define TEST_MODULE_DEF(name, slots, methods) { \
    PyModuleDef_HEAD_INIT,                      /* m_base */ \
    name,                                       /* m_name */ \
    PyDoc_STR("Test module " name),             /* m_doc */ \
    0,                                          /* m_size */ \
    methods,                                    /* m_methods */ \
    slots,                                      /* m_slots */ \
    NULL,                                       /* m_traverse */ \
    NULL,                                       /* m_clear */ \
    NULL,                                       /* m_free */ \
}

static PyModuleDef_Slot main_slots[] = {
    {Py_mod_exec, execfunc},
    {0, NULL},
};

static PyModuleDef main_def = TEST_MODULE_DEF("main", main_slots, testexport_methods);

PyMODINIT_FUNC
PyInit__testmultiphase(PyObject *spec)
{
    return PyModuleDef_Init(&main_def);
}


/**** Importing a non-module object ****/

static PyModuleDef def_nonmodule;
static PyModuleDef def_nonmodule_with_methods;

/* Create a SimpleNamespace(three=3) */
static PyObject*
createfunc_nonmodule(PyObject *spec, PyModuleDef *def)
{
    PyObject *dct, *ns, *three;

    if (def != &def_nonmodule && def != &def_nonmodule_with_methods) {
        PyErr_SetString(PyExc_SystemError, "def does not match");
        return NULL;
    }

    dct = PyDict_New();
    if (dct == NULL)
        return NULL;

    three = PyLong_FromLong(3);
    if (three == NULL) {
        Py_DECREF(dct);
        return NULL;
    }
    PyDict_SetItemString(dct, "three", three);
    Py_DECREF(three);

    ns = _PyNamespace_New(dct);
    Py_DECREF(dct);
    return ns;
}

static PyModuleDef_Slot slots_create_nonmodule[] = {
    {Py_mod_create, createfunc_nonmodule},
    {0, NULL},
};

static PyModuleDef def_nonmodule = TEST_MODULE_DEF(
    "_testmultiphase_nonmodule", slots_create_nonmodule, NULL);

PyMODINIT_FUNC
PyInit__testmultiphase_nonmodule(PyObject *spec)
{
    return PyModuleDef_Init(&def_nonmodule);
}

PyDoc_STRVAR(nonmodule_bar_doc,
"bar(i,j)\n\
\n\
Return the difference of i - j.");

static PyObject *
nonmodule_bar(PyObject *self, PyObject *args)
{
    long i, j;
    long res;
    if (!PyArg_ParseTuple(args, "ll:bar", &i, &j))
        return NULL;
    res = i - j;
    return PyLong_FromLong(res);
}

static PyMethodDef nonmodule_methods[] = {
    {"bar", nonmodule_bar, METH_VARARGS, nonmodule_bar_doc},
    {NULL, NULL}           /* sentinel */
};

static PyModuleDef def_nonmodule_with_methods = TEST_MODULE_DEF(
    "_testmultiphase_nonmodule_with_methods", slots_create_nonmodule, nonmodule_methods);

PyMODINIT_FUNC
PyInit__testmultiphase_nonmodule_with_methods(PyObject *spec)
{
    return PyModuleDef_Init(&def_nonmodule_with_methods);
}

/**** Non-ASCII-named modules ****/

static PyModuleDef def_nonascii_latin = { \
    PyModuleDef_HEAD_INIT,                      /* m_base */
    "_testmultiphase_nonascii_latin",           /* m_name */
    PyDoc_STR("Module named in Czech"),         /* m_doc */
    0,                                          /* m_size */
    NULL,                                       /* m_methods */
    NULL,                                       /* m_slots */
    NULL,                                       /* m_traverse */
    NULL,                                       /* m_clear */
    NULL,                                       /* m_free */
};

PyMODINIT_FUNC
PyInitU__testmultiphase_zkouka_naten_evc07gi8e(PyObject *spec)
{
    return PyModuleDef_Init(&def_nonascii_latin);
}

static PyModuleDef def_nonascii_kana = { \
    PyModuleDef_HEAD_INIT,                      /* m_base */
    "_testmultiphase_nonascii_kana",            /* m_name */
    PyDoc_STR("Module named in Japanese"),      /* m_doc */
    0,                                          /* m_size */
    NULL,                                       /* m_methods */
    NULL,                                       /* m_slots */
    NULL,                                       /* m_traverse */
    NULL,                                       /* m_clear */
    NULL,                                       /* m_free */
};

PyMODINIT_FUNC
PyInitU_eckzbwbhc6jpgzcx415x(PyObject *spec)
{
    return PyModuleDef_Init(&def_nonascii_kana);
}

/*** Module with a single-character name ***/

PyMODINIT_FUNC
PyInit_x(PyObject *spec)
{
    return PyModuleDef_Init(&main_def);
}

/**** Testing NULL slots ****/

static PyModuleDef null_slots_def = TEST_MODULE_DEF(
    "_testmultiphase_null_slots", NULL, NULL);

PyMODINIT_FUNC
PyInit__testmultiphase_null_slots(PyObject *spec)
{
    return PyModuleDef_Init(&null_slots_def);
}

/**** Problematic modules ****/

static PyModuleDef_Slot slots_bad_large[] = {
    {_Py_mod_LAST_SLOT + 1, NULL},
    {0, NULL},
};

static PyModuleDef def_bad_large = TEST_MODULE_DEF(
    "_testmultiphase_bad_slot_large", slots_bad_large, NULL);

PyMODINIT_FUNC
PyInit__testmultiphase_bad_slot_large(PyObject *spec)
{
    return PyModuleDef_Init(&def_bad_large);
}

static PyModuleDef_Slot slots_bad_negative[] = {
    {-1, NULL},
    {0, NULL},
};

static PyModuleDef def_bad_negative = TEST_MODULE_DEF(
    "_testmultiphase_bad_slot_negative", slots_bad_negative, NULL);

PyMODINIT_FUNC
PyInit__testmultiphase_bad_slot_negative(PyObject *spec)
{
    return PyModuleDef_Init(&def_bad_negative);
}

static PyModuleDef def_create_int_with_state = { \
    PyModuleDef_HEAD_INIT,                      /* m_base */
    "create_with_state",                        /* m_name */
    PyDoc_STR("Not a PyModuleObject object, but requests per-module state"),
    10,                                         /* m_size */
    NULL,                                       /* m_methods */
    slots_create_nonmodule,                     /* m_slots */
    NULL,                                       /* m_traverse */
    NULL,                                       /* m_clear */
    NULL,                                       /* m_free */
};

PyMODINIT_FUNC
PyInit__testmultiphase_create_int_with_state(PyObject *spec)
{
    return PyModuleDef_Init(&def_create_int_with_state);
}


static PyModuleDef def_negative_size = { \
    PyModuleDef_HEAD_INIT,                      /* m_base */
    "negative_size",                            /* m_name */
    PyDoc_STR("PyModuleDef with negative m_size"),
    -1,                                         /* m_size */
    NULL,                                       /* m_methods */
    slots_create_nonmodule,                     /* m_slots */
    NULL,                                       /* m_traverse */
    NULL,                                       /* m_clear */
    NULL,                                       /* m_free */
};

PyMODINIT_FUNC
PyInit__testmultiphase_negative_size(PyObject *spec)
{
    return PyModuleDef_Init(&def_negative_size);
}


static PyModuleDef uninitialized_def = TEST_MODULE_DEF("main", main_slots, testexport_methods);

PyMODINIT_FUNC
PyInit__testmultiphase_export_uninitialized(PyObject *spec)
{
    return (PyObject*) &uninitialized_def;
}

PyMODINIT_FUNC
PyInit__testmultiphase_export_null(PyObject *spec)
{
    return NULL;
}

PyMODINIT_FUNC
PyInit__testmultiphase_export_raise(PyObject *spec)
{
    PyErr_SetString(PyExc_SystemError, "bad export function");
    return NULL;
}

PyMODINIT_FUNC
PyInit__testmultiphase_export_unreported_exception(PyObject *spec)
{
    PyErr_SetString(PyExc_SystemError, "bad export function");
    return PyModuleDef_Init(&main_def);
}

static PyObject*
createfunc_null(PyObject *spec, PyModuleDef *def)
{
    return NULL;
}

static PyModuleDef_Slot slots_create_null[] = {
    {Py_mod_create, createfunc_null},
    {0, NULL},
};

static PyModuleDef def_create_null = TEST_MODULE_DEF(
    "_testmultiphase_create_null", slots_create_null, NULL);

PyMODINIT_FUNC
PyInit__testmultiphase_create_null(PyObject *spec)
{
    return PyModuleDef_Init(&def_create_null);
}

static PyObject*
createfunc_raise(PyObject *spec, PyModuleDef *def)
{
    PyErr_SetString(PyExc_SystemError, "bad create function");
    return NULL;
}

static PyModuleDef_Slot slots_create_raise[] = {
    {Py_mod_create, createfunc_raise},
    {0, NULL},
};

static PyModuleDef def_create_raise = TEST_MODULE_DEF(
    "_testmultiphase_create_null", slots_create_raise, NULL);

PyMODINIT_FUNC
PyInit__testmultiphase_create_raise(PyObject *spec)
{
    return PyModuleDef_Init(&def_create_raise);
}

static PyObject*
createfunc_unreported_exception(PyObject *spec, PyModuleDef *def)
{
    PyErr_SetString(PyExc_SystemError, "bad create function");
    return PyModule_New("foo");
}

static PyModuleDef_Slot slots_create_unreported_exception[] = {
    {Py_mod_create, createfunc_unreported_exception},
    {0, NULL},
};

static PyModuleDef def_create_unreported_exception = TEST_MODULE_DEF(
    "_testmultiphase_create_unreported_exception", slots_create_unreported_exception, NULL);

PyMODINIT_FUNC
PyInit__testmultiphase_create_unreported_exception(PyObject *spec)
{
    return PyModuleDef_Init(&def_create_unreported_exception);
}

static PyModuleDef_Slot slots_nonmodule_with_exec_slots[] = {
    {Py_mod_create, createfunc_nonmodule},
    {Py_mod_exec, execfunc},
    {0, NULL},
};

static PyModuleDef def_nonmodule_with_exec_slots = TEST_MODULE_DEF(
    "_testmultiphase_nonmodule_with_exec_slots", slots_nonmodule_with_exec_slots, NULL);

PyMODINIT_FUNC
PyInit__testmultiphase_nonmodule_with_exec_slots(PyObject *spec)
{
    return PyModuleDef_Init(&def_nonmodule_with_exec_slots);
}

static int
execfunc_err(PyObject *mod)
{
    return -1;
}

static PyModuleDef_Slot slots_exec_err[] = {
    {Py_mod_exec, execfunc_err},
    {0, NULL},
};

static PyModuleDef def_exec_err = TEST_MODULE_DEF(
    "_testmultiphase_exec_err", slots_exec_err, NULL);

PyMODINIT_FUNC
PyInit__testmultiphase_exec_err(PyObject *spec)
{
    return PyModuleDef_Init(&def_exec_err);
}

static int
execfunc_raise(PyObject *spec)
{
    PyErr_SetString(PyExc_SystemError, "bad exec function");
    return -1;
}

static PyModuleDef_Slot slots_exec_raise[] = {
    {Py_mod_exec, execfunc_raise},
    {0, NULL},
};

static PyModuleDef def_exec_raise = TEST_MODULE_DEF(
    "_testmultiphase_exec_raise", slots_exec_raise, NULL);

PyMODINIT_FUNC
PyInit__testmultiphase_exec_raise(PyObject *mod)
{
    return PyModuleDef_Init(&def_exec_raise);
}

static int
execfunc_unreported_exception(PyObject *mod)
{
    PyErr_SetString(PyExc_SystemError, "bad exec function");
    return 0;
}

static PyModuleDef_Slot slots_exec_unreported_exception[] = {
    {Py_mod_exec, execfunc_unreported_exception},
    {0, NULL},
};

static PyModuleDef def_exec_unreported_exception = TEST_MODULE_DEF(
    "_testmultiphase_exec_unreported_exception", slots_exec_unreported_exception, NULL);

PyMODINIT_FUNC
PyInit__testmultiphase_exec_unreported_exception(PyObject *spec)
{
    return PyModuleDef_Init(&def_exec_unreported_exception);
}

/*** Helper for imp test ***/

static PyModuleDef imp_dummy_def = TEST_MODULE_DEF("imp_dummy", main_slots, testexport_methods);

PyMODINIT_FUNC
PyInit_imp_dummy(PyObject *spec)
{
    return PyModuleDef_Init(&imp_dummy_def);
}

