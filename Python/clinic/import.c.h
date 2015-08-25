/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_imp_lock_held__doc__,
"lock_held($module, /)\n"
"--\n"
"\n"
"Return True if the import lock is currently held, else False.\n"
"\n"
"On platforms without threads, return False.");

#define _IMP_LOCK_HELD_METHODDEF    \
    {"lock_held", (PyCFunction)_imp_lock_held, METH_NOARGS, _imp_lock_held__doc__},

static PyObject *
_imp_lock_held_impl(PyModuleDef *module);

static PyObject *
_imp_lock_held(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return _imp_lock_held_impl(module);
}

PyDoc_STRVAR(_imp_acquire_lock__doc__,
"acquire_lock($module, /)\n"
"--\n"
"\n"
"Acquires the interpreter\'s import lock for the current thread.\n"
"\n"
"This lock should be used by import hooks to ensure thread-safety when importing\n"
"modules. On platforms without threads, this function does nothing.");

#define _IMP_ACQUIRE_LOCK_METHODDEF    \
    {"acquire_lock", (PyCFunction)_imp_acquire_lock, METH_NOARGS, _imp_acquire_lock__doc__},

static PyObject *
_imp_acquire_lock_impl(PyModuleDef *module);

static PyObject *
_imp_acquire_lock(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return _imp_acquire_lock_impl(module);
}

PyDoc_STRVAR(_imp_release_lock__doc__,
"release_lock($module, /)\n"
"--\n"
"\n"
"Release the interpreter\'s import lock.\n"
"\n"
"On platforms without threads, this function does nothing.");

#define _IMP_RELEASE_LOCK_METHODDEF    \
    {"release_lock", (PyCFunction)_imp_release_lock, METH_NOARGS, _imp_release_lock__doc__},

static PyObject *
_imp_release_lock_impl(PyModuleDef *module);

static PyObject *
_imp_release_lock(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return _imp_release_lock_impl(module);
}

PyDoc_STRVAR(_imp__fix_co_filename__doc__,
"_fix_co_filename($module, code, path, /)\n"
"--\n"
"\n"
"Changes code.co_filename to specify the passed-in file path.\n"
"\n"
"  code\n"
"    Code object to change.\n"
"  path\n"
"    File path to use.");

#define _IMP__FIX_CO_FILENAME_METHODDEF    \
    {"_fix_co_filename", (PyCFunction)_imp__fix_co_filename, METH_VARARGS, _imp__fix_co_filename__doc__},

static PyObject *
_imp__fix_co_filename_impl(PyModuleDef *module, PyCodeObject *code,
                           PyObject *path);

static PyObject *
_imp__fix_co_filename(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyCodeObject *code;
    PyObject *path;

    if (!PyArg_ParseTuple(args, "O!U:_fix_co_filename",
        &PyCode_Type, &code, &path))
        goto exit;
    return_value = _imp__fix_co_filename_impl(module, code, path);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp_create_builtin__doc__,
"create_builtin($module, spec, /)\n"
"--\n"
"\n"
"Create an extension module.");

#define _IMP_CREATE_BUILTIN_METHODDEF    \
    {"create_builtin", (PyCFunction)_imp_create_builtin, METH_O, _imp_create_builtin__doc__},

PyDoc_STRVAR(_imp_extension_suffixes__doc__,
"extension_suffixes($module, /)\n"
"--\n"
"\n"
"Returns the list of file suffixes used to identify extension modules.");

#define _IMP_EXTENSION_SUFFIXES_METHODDEF    \
    {"extension_suffixes", (PyCFunction)_imp_extension_suffixes, METH_NOARGS, _imp_extension_suffixes__doc__},

static PyObject *
_imp_extension_suffixes_impl(PyModuleDef *module);

static PyObject *
_imp_extension_suffixes(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return _imp_extension_suffixes_impl(module);
}

PyDoc_STRVAR(_imp_init_frozen__doc__,
"init_frozen($module, name, /)\n"
"--\n"
"\n"
"Initializes a frozen module.");

#define _IMP_INIT_FROZEN_METHODDEF    \
    {"init_frozen", (PyCFunction)_imp_init_frozen, METH_O, _imp_init_frozen__doc__},

static PyObject *
_imp_init_frozen_impl(PyModuleDef *module, PyObject *name);

static PyObject *
_imp_init_frozen(PyModuleDef *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *name;

    if (!PyArg_Parse(arg, "U:init_frozen", &name))
        goto exit;
    return_value = _imp_init_frozen_impl(module, name);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp_get_frozen_object__doc__,
"get_frozen_object($module, name, /)\n"
"--\n"
"\n"
"Create a code object for a frozen module.");

#define _IMP_GET_FROZEN_OBJECT_METHODDEF    \
    {"get_frozen_object", (PyCFunction)_imp_get_frozen_object, METH_O, _imp_get_frozen_object__doc__},

static PyObject *
_imp_get_frozen_object_impl(PyModuleDef *module, PyObject *name);

static PyObject *
_imp_get_frozen_object(PyModuleDef *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *name;

    if (!PyArg_Parse(arg, "U:get_frozen_object", &name))
        goto exit;
    return_value = _imp_get_frozen_object_impl(module, name);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp_is_frozen_package__doc__,
"is_frozen_package($module, name, /)\n"
"--\n"
"\n"
"Returns True if the module name is of a frozen package.");

#define _IMP_IS_FROZEN_PACKAGE_METHODDEF    \
    {"is_frozen_package", (PyCFunction)_imp_is_frozen_package, METH_O, _imp_is_frozen_package__doc__},

static PyObject *
_imp_is_frozen_package_impl(PyModuleDef *module, PyObject *name);

static PyObject *
_imp_is_frozen_package(PyModuleDef *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *name;

    if (!PyArg_Parse(arg, "U:is_frozen_package", &name))
        goto exit;
    return_value = _imp_is_frozen_package_impl(module, name);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp_is_builtin__doc__,
"is_builtin($module, name, /)\n"
"--\n"
"\n"
"Returns True if the module name corresponds to a built-in module.");

#define _IMP_IS_BUILTIN_METHODDEF    \
    {"is_builtin", (PyCFunction)_imp_is_builtin, METH_O, _imp_is_builtin__doc__},

static PyObject *
_imp_is_builtin_impl(PyModuleDef *module, PyObject *name);

static PyObject *
_imp_is_builtin(PyModuleDef *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *name;

    if (!PyArg_Parse(arg, "U:is_builtin", &name))
        goto exit;
    return_value = _imp_is_builtin_impl(module, name);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp_is_frozen__doc__,
"is_frozen($module, name, /)\n"
"--\n"
"\n"
"Returns True if the module name corresponds to a frozen module.");

#define _IMP_IS_FROZEN_METHODDEF    \
    {"is_frozen", (PyCFunction)_imp_is_frozen, METH_O, _imp_is_frozen__doc__},

static PyObject *
_imp_is_frozen_impl(PyModuleDef *module, PyObject *name);

static PyObject *
_imp_is_frozen(PyModuleDef *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *name;

    if (!PyArg_Parse(arg, "U:is_frozen", &name))
        goto exit;
    return_value = _imp_is_frozen_impl(module, name);

exit:
    return return_value;
}

#if defined(HAVE_DYNAMIC_LOADING)

PyDoc_STRVAR(_imp_create_dynamic__doc__,
"create_dynamic($module, spec, file=None, /)\n"
"--\n"
"\n"
"Create an extension module.");

#define _IMP_CREATE_DYNAMIC_METHODDEF    \
    {"create_dynamic", (PyCFunction)_imp_create_dynamic, METH_VARARGS, _imp_create_dynamic__doc__},

static PyObject *
_imp_create_dynamic_impl(PyModuleDef *module, PyObject *spec, PyObject *file);

static PyObject *
_imp_create_dynamic(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *spec;
    PyObject *file = NULL;

    if (!PyArg_UnpackTuple(args, "create_dynamic",
        1, 2,
        &spec, &file))
        goto exit;
    return_value = _imp_create_dynamic_impl(module, spec, file);

exit:
    return return_value;
}

#endif /* defined(HAVE_DYNAMIC_LOADING) */

#if defined(HAVE_DYNAMIC_LOADING)

PyDoc_STRVAR(_imp_exec_dynamic__doc__,
"exec_dynamic($module, mod, /)\n"
"--\n"
"\n"
"Initialize an extension module.");

#define _IMP_EXEC_DYNAMIC_METHODDEF    \
    {"exec_dynamic", (PyCFunction)_imp_exec_dynamic, METH_O, _imp_exec_dynamic__doc__},

static int
_imp_exec_dynamic_impl(PyModuleDef *module, PyObject *mod);

static PyObject *
_imp_exec_dynamic(PyModuleDef *module, PyObject *mod)
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _imp_exec_dynamic_impl(module, mod);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_DYNAMIC_LOADING) */

PyDoc_STRVAR(_imp_exec_builtin__doc__,
"exec_builtin($module, mod, /)\n"
"--\n"
"\n"
"Initialize a built-in module.");

#define _IMP_EXEC_BUILTIN_METHODDEF    \
    {"exec_builtin", (PyCFunction)_imp_exec_builtin, METH_O, _imp_exec_builtin__doc__},

static int
_imp_exec_builtin_impl(PyModuleDef *module, PyObject *mod);

static PyObject *
_imp_exec_builtin(PyModuleDef *module, PyObject *mod)
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _imp_exec_builtin_impl(module, mod);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#ifndef _IMP_CREATE_DYNAMIC_METHODDEF
    #define _IMP_CREATE_DYNAMIC_METHODDEF
#endif /* !defined(_IMP_CREATE_DYNAMIC_METHODDEF) */

#ifndef _IMP_EXEC_DYNAMIC_METHODDEF
    #define _IMP_EXEC_DYNAMIC_METHODDEF
#endif /* !defined(_IMP_EXEC_DYNAMIC_METHODDEF) */
/*[clinic end generated code: output=32324a5e46cdfc4b input=a9049054013a1b77]*/
