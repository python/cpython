#include <Python.h>
#include <pegen_interface.h>

PyObject *
_Py_parse_file(PyObject *self, PyObject *args)
{
    char *filename;

    if (!PyArg_ParseTuple(args, "s", &filename)) {
        return NULL;
    }

    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    mod_ty res = PyPegen_ASTFromFile(filename, arena);
    if (res == NULL) {
        PyArena_Free(arena);
        return NULL;
    }
    PyObject *result = PyAST_mod2obj(res);

    PyArena_Free(arena);
    return result;
}

PyObject *
_Py_parse_string(PyObject *self, PyObject *args)
{
    char *the_string;

    if (!PyArg_ParseTuple(args, "s", &the_string)) {
        return NULL;
    }

    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    mod_ty res = PyPegen_ASTFromString(the_string, arena);
    if (res == NULL) {
        PyArena_Free(arena);
        return NULL;
    }
    PyObject *result = PyAST_mod2obj(res);

    PyArena_Free(arena);
    return result;
}

static PyMethodDef ParseMethods[] = {
    {"parse_file", (PyCFunction)(void (*)(void))_Py_parse_file, METH_VARARGS, "Parse a file."},
    {"parse_string", (PyCFunction)(void (*)(void))_Py_parse_string, METH_VARARGS,
     "Parse a string."},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static struct PyModuleDef parsemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "peg_parser",
    .m_doc = "A parser.",
    .m_methods = ParseMethods,
};

PyMODINIT_FUNC
PyInit_peg_parser(void)
{
    return PyModule_Create(&parsemodule);
}
