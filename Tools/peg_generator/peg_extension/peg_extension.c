#include "pegen.h"

PyObject *
_build_return_object(mod_ty module, int mode, PyObject *filename_ob, PyArena *arena)
{
    PyObject *result = NULL;

    if (mode == 2) {
        result = (PyObject *)PyAST_CompileObject(module, filename_ob, NULL, -1, arena);
    } else if (mode == 1) {
        result = PyAST_mod2obj(module);
    } else {
        result = Py_None;
        Py_INCREF(result);
    }

    return result;
}

static PyObject *
parse_file(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"file", "mode", NULL};
    const char *filename;
    int mode = 2;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|i", keywords, &filename, &mode)) {
        return NULL;
    }
    if (mode < 0 || mode > 2) {
        return PyErr_Format(PyExc_ValueError, "Bad mode, must be 0 <= mode <= 2");
    }

    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    PyObject *result = NULL;

    PyObject *filename_ob = PyUnicode_FromString(filename);
    if (filename_ob == NULL) {
        goto error;
    }

    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    mod_ty res = _PyPegen_run_parser_from_file(filename, Py_file_input, filename_ob, &flags, arena);
    if (res == NULL) {
        goto error;
    }

    result = _build_return_object(res, mode, filename_ob, arena);

error:
    Py_XDECREF(filename_ob);
    PyArena_Free(arena);
    return result;
}

static PyObject *
parse_string(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"str", "mode", NULL};
    const char *the_string;
    int mode = 2;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|i", keywords, &the_string, &mode)) {
        return NULL;
    }
    if (mode < 0 || mode > 2) {
        return PyErr_Format(PyExc_ValueError, "Bad mode, must be 0 <= mode <= 2");
    }

    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    PyObject *result = NULL;

    PyObject *filename_ob = PyUnicode_FromString("<string>");
    if (filename_ob == NULL) {
        goto error;
    }

    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    mod_ty res = _PyPegen_run_parser_from_string(the_string, Py_file_input, filename_ob,
                                        &flags, arena);
    if (res == NULL) {
        goto error;
    }
    result = _build_return_object(res, mode, filename_ob, arena);

error:
    Py_XDECREF(filename_ob);
    PyArena_Free(arena);
    return result;
}

static PyObject *
clear_memo_stats()
{
    _PyPegen_clear_memo_statistics();
    Py_RETURN_NONE;
}

static PyObject *
get_memo_stats()
{
    return _PyPegen_get_memo_statistics();
}

// TODO: Write to Python's sys.stdout instead of C's stdout.
static PyObject *
dump_memo_stats()
{
    PyObject *list = _PyPegen_get_memo_statistics();
    if (list == NULL) {
        return NULL;
    }
    Py_ssize_t len = PyList_Size(list);
    for (Py_ssize_t i = 0; i < len; i++) {
        PyObject *value = PyList_GetItem(list, i);  // Borrowed reference.
        long count = PyLong_AsLong(value);
        if (count < 0) {
            break;
        }
        if (count > 0) {
            printf("%4ld %9ld\n", i, count);
        }
    }
    Py_DECREF(list);
    Py_RETURN_NONE;
}

static PyMethodDef ParseMethods[] = {
    {"parse_file", (PyCFunction)(void(*)(void))parse_file, METH_VARARGS|METH_KEYWORDS, "Parse a file."},
    {"parse_string", (PyCFunction)(void(*)(void))parse_string, METH_VARARGS|METH_KEYWORDS, "Parse a string."},
    {"clear_memo_stats", clear_memo_stats, METH_NOARGS},
    {"dump_memo_stats", dump_memo_stats, METH_NOARGS},
    {"get_memo_stats", get_memo_stats, METH_NOARGS},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef parsemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "parse",
    .m_doc = "A parser.",
    .m_methods = ParseMethods,
};

PyMODINIT_FUNC
PyInit_parse(void)
{
    return PyModule_Create(&parsemodule);
}
