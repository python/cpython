/*[clinic input]
preserve
[clinic start generated code]*/

static int
_testmultiphase_StateAccessType___init___impl(StateAccessTypeObject *self,
                                              PyTypeObject *cls);

static int
_testmultiphase_StateAccessType___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;

    PyTypeObject *cls;

    cls = PyType_DefiningTypeFromSlotFunc(Py_TYPE(self),
                                          Py_tp_init,
                                          &_testmultiphase_StateAccessType___init__);
    if (cls == NULL) {
        goto exit;
    }
    return_value = _testmultiphase_StateAccessType___init___impl((StateAccessTypeObject *)self, cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_testmultiphase_StateAccessType_get_defining_module__doc__,
"get_defining_module($self, /)\n"
"--\n"
"\n"
"This method returns module of the defining class.");

#define _TESTMULTIPHASE_STATEACCESSTYPE_GET_DEFINING_MODULE_METHODDEF    \
    {"get_defining_module", (PyCFunction)_testmultiphase_StateAccessType_get_defining_module, METH_METHOD|METH_VARARGS|METH_KEYWORDS, _testmultiphase_StateAccessType_get_defining_module__doc__},

static PyObject *
_testmultiphase_StateAccessType_get_defining_module_impl(StateAccessTypeObject *self,
                                                         PyTypeObject *cls);

static PyObject *
_testmultiphase_StateAccessType_get_defining_module(StateAccessTypeObject *self, PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;

    return_value = _testmultiphase_StateAccessType_get_defining_module_impl(self, cls);

    return return_value;
}

PyDoc_STRVAR(_testmultiphase_StateAccessType_get_instance_count__doc__,
"get_instance_count($self, /)\n"
"--\n"
"\n"
"This method returns module of the defining class.");

#define _TESTMULTIPHASE_STATEACCESSTYPE_GET_INSTANCE_COUNT_METHODDEF    \
    {"get_instance_count", (PyCFunction)_testmultiphase_StateAccessType_get_instance_count, METH_NOARGS, _testmultiphase_StateAccessType_get_instance_count__doc__},

static PyObject *
_testmultiphase_StateAccessType_get_instance_count_impl(StateAccessTypeObject *self);

static PyObject *
_testmultiphase_StateAccessType_get_instance_count(StateAccessTypeObject *self, PyObject *Py_UNUSED(ignored))
{
    return _testmultiphase_StateAccessType_get_instance_count_impl(self);
}
/*[clinic end generated code: output=455ce989b3cd9429 input=a9049054013a1b77]*/
