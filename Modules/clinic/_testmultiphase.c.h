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
    PyObject *return_value = NULL;
    static const char * const _keywords[] = { NULL};
    static _PyArg_Parser _parser = {":get_defining_module", _keywords, 0};

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser
        )) {
        goto exit;
    }
    return_value = _testmultiphase_StateAccessType_get_defining_module_impl(self, cls);

exit:
    return return_value;
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
    static _PyArg_Parser _parser = {"|i$p:increment_count_clinic", _keywords, 0};
    int n = 1;
    int twice = 0;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &n, &twice)) {
        goto exit;
    }
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
    PyObject *return_value = NULL;
    static const char * const _keywords[] = { NULL};
    static _PyArg_Parser _parser = {":get_count", _keywords, 0};

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser
        )) {
        goto exit;
    }
    return_value = _testmultiphase_StateAccessType_get_count_impl(self, cls);

exit:
    return return_value;
}
/*[clinic end generated code: output=f01137bb3b373e14 input=a9049054013a1b77]*/
