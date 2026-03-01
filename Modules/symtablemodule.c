#include "Python.h"
#include "pycore_pythonrun.h"     // _Py_SourceAsString()
#include "pycore_symtable.h"      // struct symtable

#include "clinic/symtablemodule.c.h"
/*[clinic input]
module _symtable
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f4685845a7100605]*/


/*[clinic input]
_symtable.symtable

    source:    object
    filename:  unicode_fs_decoded
    startstr:  str
    /
    *
    module as modname: object = None

Return symbol and scope dictionaries used internally by compiler.
[clinic start generated code]*/

static PyObject *
_symtable_symtable_impl(PyObject *module, PyObject *source,
                        PyObject *filename, const char *startstr,
                        PyObject *modname)
/*[clinic end generated code: output=235ec5a87a9ce178 input=fbf9adaa33c7070d]*/
{
    struct symtable *st;
    PyObject *t;
    int start;
    PyCompilerFlags cf = _PyCompilerFlags_INIT;
    PyObject *source_copy = NULL;

    cf.cf_flags = PyCF_SOURCE_IS_UTF8;

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
        Py_XDECREF(source_copy);
        return NULL;
    }
    if (modname == Py_None) {
        modname = NULL;
    }
    else if (!PyUnicode_Check(modname)) {
        PyErr_Format(PyExc_TypeError,
                     "symtable() argument 'module' must be str or None, not %T",
                     modname);
        Py_XDECREF(source_copy);
        return NULL;
    }
    st = _Py_SymtableStringObjectFlags(str, filename, start, &cf, modname);
    Py_XDECREF(source_copy);
    if (st == NULL) {
        return NULL;
    }
    t = Py_NewRef(st->st_top);
    _PySymtable_Free(st);
    return t;
}

static PyMethodDef symtable_methods[] = {
    _SYMTABLE_SYMTABLE_METHODDEF
    {NULL,              NULL}           /* sentinel */
};

static int
symtable_init_constants(PyObject *m)
{
    if (PyModule_AddIntMacro(m, USE) < 0) return -1;
    if (PyModule_AddIntMacro(m, DEF_GLOBAL) < 0) return -1;
    if (PyModule_AddIntMacro(m, DEF_NONLOCAL) < 0) return -1;
    if (PyModule_AddIntMacro(m, DEF_LOCAL) < 0) return -1;
    if (PyModule_AddIntMacro(m, DEF_PARAM) < 0) return -1;
    if (PyModule_AddIntMacro(m, DEF_TYPE_PARAM) < 0) return -1;
    if (PyModule_AddIntMacro(m, DEF_FREE_CLASS) < 0) return -1;
    if (PyModule_AddIntMacro(m, DEF_IMPORT) < 0) return -1;
    if (PyModule_AddIntMacro(m, DEF_BOUND) < 0) return -1;
    if (PyModule_AddIntMacro(m, DEF_ANNOT) < 0) return -1;
    if (PyModule_AddIntMacro(m, DEF_COMP_ITER) < 0) return -1;
    if (PyModule_AddIntMacro(m, DEF_COMP_CELL) < 0) return -1;

    if (PyModule_AddIntConstant(m, "TYPE_FUNCTION", FunctionBlock) < 0)
        return -1;
    if (PyModule_AddIntConstant(m, "TYPE_CLASS", ClassBlock) < 0)
        return -1;
    if (PyModule_AddIntConstant(m, "TYPE_MODULE", ModuleBlock) < 0)
        return -1;
    if (PyModule_AddIntConstant(m, "TYPE_ANNOTATION", AnnotationBlock) < 0)
        return -1;
    if (PyModule_AddIntConstant(m, "TYPE_TYPE_ALIAS", TypeAliasBlock) < 0)
        return -1;
    if (PyModule_AddIntConstant(m, "TYPE_TYPE_PARAMETERS", TypeParametersBlock) < 0)
        return -1;
    if (PyModule_AddIntConstant(m, "TYPE_TYPE_VARIABLE", TypeVariableBlock) < 0)
        return -1;

    if (PyModule_AddIntMacro(m, LOCAL) < 0) return -1;
    if (PyModule_AddIntMacro(m, GLOBAL_EXPLICIT) < 0) return -1;
    if (PyModule_AddIntMacro(m, GLOBAL_IMPLICIT) < 0) return -1;
    if (PyModule_AddIntMacro(m, FREE) < 0) return -1;
    if (PyModule_AddIntMacro(m, CELL) < 0) return -1;

    if (PyModule_AddIntConstant(m, "SCOPE_OFF", SCOPE_OFFSET) < 0) return -1;
    if (PyModule_AddIntMacro(m, SCOPE_MASK) < 0) return -1;

    return 0;
}

static PyModuleDef_Slot symtable_slots[] = {
    {Py_mod_exec, symtable_init_constants},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef symtablemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_symtable",
    .m_size = 0,
    .m_methods = symtable_methods,
    .m_slots = symtable_slots,
};

PyMODINIT_FUNC
PyInit__symtable(void)
{
    return PyModuleDef_Init(&symtablemodule);
}
