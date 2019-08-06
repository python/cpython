/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_abc__reset_registry__doc__,
"_reset_registry($module, self, /)\n"
"--\n"
"\n"
"Internal ABC helper to reset registry of a given class.\n"
"\n"
"Should be only used by refleak.py");

#define _ABC__RESET_REGISTRY_METHODDEF    \
    {"_reset_registry", (PyCFunction)_abc__reset_registry, METH_O, _abc__reset_registry__doc__},

PyDoc_STRVAR(_abc__reset_caches__doc__,
"_reset_caches($module, self, /)\n"
"--\n"
"\n"
"Internal ABC helper to reset both caches of a given class.\n"
"\n"
"Should be only used by refleak.py");

#define _ABC__RESET_CACHES_METHODDEF    \
    {"_reset_caches", (PyCFunction)_abc__reset_caches, METH_O, _abc__reset_caches__doc__},

PyDoc_STRVAR(_abc__get_dump__doc__,
"_get_dump($module, self, /)\n"
"--\n"
"\n"
"Internal ABC helper for cache and registry debugging.\n"
"\n"
"Return shallow copies of registry, of both caches, and\n"
"negative cache version. Don\'t call this function directly,\n"
"instead use ABC._dump_registry() for a nice repr.");

#define _ABC__GET_DUMP_METHODDEF    \
    {"_get_dump", (PyCFunction)_abc__get_dump, METH_O, _abc__get_dump__doc__},

PyDoc_STRVAR(_abc__abc_init__doc__,
"_abc_init($module, self, /)\n"
"--\n"
"\n"
"Internal ABC helper for class set-up. Should be never used outside abc module.");

#define _ABC__ABC_INIT_METHODDEF    \
    {"_abc_init", (PyCFunction)_abc__abc_init, METH_O, _abc__abc_init__doc__},

PyDoc_STRVAR(_abc__abc_register__doc__,
"_abc_register($module, self, subclass, /)\n"
"--\n"
"\n"
"Internal ABC helper for subclasss registration. Should be never used outside abc module.");

#define _ABC__ABC_REGISTER_METHODDEF    \
    {"_abc_register", (PyCFunction)(void(*)(void))_abc__abc_register, METH_FASTCALL, _abc__abc_register__doc__},

static PyObject *
_abc__abc_register_impl(PyObject *module, PyObject *self, PyObject *subclass);

static PyObject *
_abc__abc_register(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *self;
    PyObject *subclass;

    if (!_PyArg_CheckPositional("_abc_register", nargs, 2, 2)) {
        goto exit;
    }
    self = args[0];
    subclass = args[1];
    return_value = _abc__abc_register_impl(module, self, subclass);

exit:
    return return_value;
}

PyDoc_STRVAR(_abc__abc_instancecheck__doc__,
"_abc_instancecheck($module, self, instance, /)\n"
"--\n"
"\n"
"Internal ABC helper for instance checks. Should be never used outside abc module.");

#define _ABC__ABC_INSTANCECHECK_METHODDEF    \
    {"_abc_instancecheck", (PyCFunction)(void(*)(void))_abc__abc_instancecheck, METH_FASTCALL, _abc__abc_instancecheck__doc__},

static PyObject *
_abc__abc_instancecheck_impl(PyObject *module, PyObject *self,
                             PyObject *instance);

static PyObject *
_abc__abc_instancecheck(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *self;
    PyObject *instance;

    if (!_PyArg_CheckPositional("_abc_instancecheck", nargs, 2, 2)) {
        goto exit;
    }
    self = args[0];
    instance = args[1];
    return_value = _abc__abc_instancecheck_impl(module, self, instance);

exit:
    return return_value;
}

PyDoc_STRVAR(_abc__abc_subclasscheck__doc__,
"_abc_subclasscheck($module, self, subclass, /)\n"
"--\n"
"\n"
"Internal ABC helper for subclasss checks. Should be never used outside abc module.");

#define _ABC__ABC_SUBCLASSCHECK_METHODDEF    \
    {"_abc_subclasscheck", (PyCFunction)(void(*)(void))_abc__abc_subclasscheck, METH_FASTCALL, _abc__abc_subclasscheck__doc__},

static PyObject *
_abc__abc_subclasscheck_impl(PyObject *module, PyObject *self,
                             PyObject *subclass);

static PyObject *
_abc__abc_subclasscheck(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *self;
    PyObject *subclass;

    if (!_PyArg_CheckPositional("_abc_subclasscheck", nargs, 2, 2)) {
        goto exit;
    }
    self = args[0];
    subclass = args[1];
    return_value = _abc__abc_subclasscheck_impl(module, self, subclass);

exit:
    return return_value;
}

PyDoc_STRVAR(_abc_get_cache_token__doc__,
"get_cache_token($module, /)\n"
"--\n"
"\n"
"Returns the current ABC cache token.\n"
"\n"
"The token is an opaque object (supporting equality testing) identifying the\n"
"current version of the ABC cache for virtual subclasses. The token changes\n"
"with every call to register() on any ABC.");

#define _ABC_GET_CACHE_TOKEN_METHODDEF    \
    {"get_cache_token", (PyCFunction)_abc_get_cache_token, METH_NOARGS, _abc_get_cache_token__doc__},

static PyObject *
_abc_get_cache_token_impl(PyObject *module);

static PyObject *
_abc_get_cache_token(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _abc_get_cache_token_impl(module);
}
/*[clinic end generated code: output=2544b4b5ae50a089 input=a9049054013a1b77]*/
