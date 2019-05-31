#include "Python.h"

#include "code.h"
#include "Python-ast.h"
#include "symtable.h"

#include "clinic/symtablemodule.c.h"
/*[clinic input]
module _symtable
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f4685845a7100605]*/


/*[clinic input]
_symtable.symtable

    source:    object
    filename:  object(converter='PyUnicode_FSDecoder')
    startstr:  str
    /

Return symbol and scope dictionaries used internally by compiler.
[clinic start generated code]*/

static PyObject *
_symtable_symtable_impl(PyObject *module, PyObject *source,
                        PyObject *filename, const char *startstr)
/*[clinic end generated code: output=59eb0d5fc7285ac4 input=9dd8a50c0c36a4d7]*/
{
    struct symtable *st;
    PyObject *t;
    int start;
    PyCompilerFlags cf;
    PyObject *source_copy = NULL;

    cf.cf_flags = PyCF_SOURCE_IS_UTF8;
    cf.cf_feature_version = PY_MINOR_VERSION;

    const char *str = _Py_SourceAsString(source, "symtable", "string or bytes", &cf, &source_copy);
    if (str == NULL) {
        return NULL;
    }

    if (strcmp(startstr, "exec") == 0)
        start = Py_file_input;
    else if (strcmp(startstr, "eval") == 0)
        start = Py_eval_input;
    else if (strcmp(startstr, "single") == 0)
        start = Py_single_input;
    else {
        PyErr_SetString(PyExc_ValueError,
           "symtable() arg 3 must be 'exec' or 'eval' or 'single'");
        Py_DECREF(filename);
        Py_XDECREF(source_copy);
        return NULL;
    }
    st = _Py_SymtableStringObjectFlags(str, filename, start, &cf);
    Py_DECREF(filename);
    Py_XDECREF(source_copy);
    if (st == NULL) {
        return NULL;
    }
    t = (PyObject *)st->st_top;
    Py_INCREF(t);
    PyMem_Free((void *)st->st_future);
    PySymtable_Free(st);
    return t;
}

static PyMethodDef symtable_methods[] = {
    _SYMTABLE_SYMTABLE_METHODDEF
    {NULL,              NULL}           /* sentinel */
};

static struct PyModuleDef symtablemodule = {
    PyModuleDef_HEAD_INIT,
    "_symtable",
    NULL,
    -1,
    symtable_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__symtable(void)
{
    PyObject *m;

    if (PyType_Ready(&PySTEntry_Type) < 0)
        return NULL;

    m = PyModule_Create(&symtablemodule);
    if (m == NULL)
        return NULL;
    PyModule_AddIntMacro(m, USE);
    PyModule_AddIntMacro(m, DEF_GLOBAL);
    PyModule_AddIntMacro(m, DEF_NONLOCAL);
    PyModule_AddIntMacro(m, DEF_LOCAL);
    PyModule_AddIntMacro(m, DEF_PARAM);
    PyModule_AddIntMacro(m, DEF_FREE);
    PyModule_AddIntMacro(m, DEF_FREE_CLASS);
    PyModule_AddIntMacro(m, DEF_IMPORT);
    PyModule_AddIntMacro(m, DEF_BOUND);
    PyModule_AddIntMacro(m, DEF_ANNOT);

    PyModule_AddIntConstant(m, "TYPE_FUNCTION", FunctionBlock);
    PyModule_AddIntConstant(m, "TYPE_CLASS", ClassBlock);
    PyModule_AddIntConstant(m, "TYPE_MODULE", ModuleBlock);

    PyModule_AddIntMacro(m, LOCAL);
    PyModule_AddIntMacro(m, GLOBAL_EXPLICIT);
    PyModule_AddIntMacro(m, GLOBAL_IMPLICIT);
    PyModule_AddIntMacro(m, FREE);
    PyModule_AddIntMacro(m, CELL);

    PyModule_AddIntConstant(m, "SCOPE_OFF", SCOPE_OFFSET);
    PyModule_AddIntMacro(m, SCOPE_MASK);

    if (PyErr_Occurred()) {
        Py_DECREF(m);
        m = 0;
    }
    return m;
}
