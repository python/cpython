#include "Python.h"
#include "pycore_import.h"

#include <string.h>

PyDoc_STRVAR(frozen_utils__doc__,
"Expose the modules that were frozen together with the Python interpreter");

static int
frozen_utils_exec(PyObject *module)
{
    return 0;
}

PyDoc_STRVAR(frozen_utils_get_frozen_modules__doc__,
"get_frozen_modules() -> List[str]\n\nReturn all of the Python modules & submodules that have \
been frozen with the Python interpreter.");

int
is_frozen_modules_end(const struct _frozen* current_frozen)
{
    // The array end in a _frozen instance that is completely zeroed out.
    struct _frozen empty_frozen;
    memset(&empty_frozen, 0, sizeof(struct _frozen));

    return current_frozen == NULL || memcmp(&empty_frozen, current_frozen, sizeof(struct _frozen)) == 0;
}

void
add_modules(PyObject *frozen_list, const struct _frozen* frozen_modules_start)
{
    const struct _frozen *current_frozen = NULL;
    for (current_frozen = frozen_modules_start; !is_frozen_modules_end(current_frozen); current_frozen++)
    {
        PyObject *name_str = PyUnicode_FromString(current_frozen->name);
        PyList_Append(frozen_list, name_str);
    }
}

static PyObject *
get_frozen_modules(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *frozen_modules_list = PyList_New(0);

    add_modules(frozen_modules_list, _PyImport_FrozenBootstrap);
    add_modules(frozen_modules_list, _PyImport_FrozenStdlib);
    add_modules(frozen_modules_list, PyImport_FrozenModules);
    add_modules(frozen_modules_list, _PyImport_FrozenTest);

    return frozen_modules_list;
}

static PyMethodDef frozen_utils_methods[] = {
    {"get_frozen_modules", _PyCFunction_CAST(get_frozen_modules), METH_FASTCALL, frozen_utils_get_frozen_modules__doc__},
    {NULL, NULL}
};

static PyModuleDef_Slot frozen_utils_slots[] = {
    {Py_mod_exec, frozen_utils_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

static struct PyModuleDef frozen_utils_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "frozen_utils",
    .m_doc = frozen_utils__doc__,
    .m_size = 0,
    .m_methods = frozen_utils_methods,
    .m_slots = frozen_utils_slots,
};

PyMODINIT_FUNC
PyInit_frozen_utils(void)
{
    return PyModuleDef_Init(&frozen_utils_module);
}