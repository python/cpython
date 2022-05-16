/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_testmultiphase_StateAccessType_get_defining_module__doc__,
"get_defining_module($self, /)\n"
"--\n"
"\n"
"Return the module of the defining class.\n"
"\n"
"Also tests that result of _PyType_GetModuleByDef matches defining_class\'s\n"
"module.");

#define _TESTMULTIPHASE_STATEACCESSTYPE_GET_DEFINING_MODULE_METHODDEF    \
    {"get_defining_module", (PyCFunction)(void(*)(void))_testmultiphase_StateAccessType_get_defining_module, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _testmultiphase_StateAccessType_get_defining_module__doc__},

static PyObject *
_testmultiphase_StateAccessType_get_defining_module_impl(StateAccessTypeObject *self,
                                                         PyTypeObject *cls);

static PyObject *
_testmultiphase_StateAccessType_get_defining_module(StateAccessTypeObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs) {
        PyErr_SetString(PyExc_TypeError, "get_defining_module() takes no arguments");
        return NULL;
    }
    return _testmultiphase_StateAccessType_get_defining_module_impl(self, cls);
}

PyDoc_STRVAR(_testmultiphase_StateAccessType_getmodulebydef_bad_def__doc__,
"getmodulebydef_bad_def($self, /)\n"
"--\n"
"\n"
"Test that result of _PyType_GetModuleByDef with a bad def is NULL.");

#define _TESTMULTIPHASE_STATEACCESSTYPE_GETMODULEBYDEF_BAD_DEF_METHODDEF    \
    {"getmodulebydef_bad_def", (PyCFunction)(void(*)(void))_testmultiphase_StateAccessType_getmodulebydef_bad_def, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _testmultiphase_StateAccessType_getmodulebydef_bad_def__doc__},

static PyObject *
_testmultiphase_StateAccessType_getmodulebydef_bad_def_impl(StateAccessTypeObject *self,
                                                            PyTypeObject *cls);

static PyObject *
_testmultiphase_StateAccessType_getmodulebydef_bad_def(StateAccessTypeObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs) {
        PyErr_SetString(PyExc_TypeError, "getmodulebydef_bad_def() takes no arguments");
        return NULL;
    }
    return _testmultiphase_StateAccessType_getmodulebydef_bad_def_impl(self, cls);
}

PyDoc_STRVAR(_testmultiphase_StateAccessType_increment_count_clinic__doc__,
"increment_count_clinic($self, /, n=1, *, twice=False)\n"
"--\n"
"\n"
"Add \'n\' from the module-state counter.\n"
"\n"
"Pass \'twice\' to double that amount.\n"
"\n"
"This tests Argument Clinic support for defining_class.");

#define _TESTMULTIPHASE_STATEACCESSTYPE_INCREMENT_COUNT_CLINIC_METHODDEF    \
    {"increment_count_clinic", (PyCFunction)(void(*)(void))_testmultiphase_StateAccessType_increment_count_clinic, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _testmultiphase_StateAccessType_increment_count_clinic__doc__},

static PyObject *
_testmultiphase_StateAccessType_increment_count_clinic_impl(StateAccessTypeObject *self,
                                                            PyTypeObject *cls,
                                                            int n, int twice);

static PyObject *
_testmultiphase_StateAccessType_increment_count_clinic(StateAccessTypeObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"n", "twice", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "increment_count_clinic", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int n = 1;
    int twice = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        n = _PyLong_AsInt(args[0]);
        if (n == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    twice = PyObject_IsTrue(args[1]);
    if (twice < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _testmultiphase_StateAccessType_increment_count_clinic_impl(self, cls, n, twice);

exit:
    return return_value;
}

PyDoc_STRVAR(_testmultiphase_StateAccessType_get_count__doc__,
"get_count($self, /)\n"
"--\n"
"\n"
"Return the value of the module-state counter.");

#define _TESTMULTIPHASE_STATEACCESSTYPE_GET_COUNT_METHODDEF    \
    {"get_count", (PyCFunction)(void(*)(void))_testmultiphase_StateAccessType_get_count, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _testmultiphase_StateAccessType_get_count__doc__},

static PyObject *
_testmultiphase_StateAccessType_get_count_impl(StateAccessTypeObject *self,
                                               PyTypeObject *cls);

static PyObject *
_testmultiphase_StateAccessType_get_count(StateAccessTypeObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs) {
        PyErr_SetString(PyExc_TypeError, "get_count() takes no arguments");
        return NULL;
    }
    return _testmultiphase_StateAccessType_get_count_impl(self, cls);
}
/*[clinic end generated code: output=ec5029d1275cbf94 input=a9049054013a1b77]*/
