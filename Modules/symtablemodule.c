#include "Python.h"
#include "pycore_ast.h"           // PyAST_Check()
#include "pycore_pyarena.h"       // _PyArena_New()
#include "pycore_pythonrun.h"     // _Py_SourceAsString()
#include "pycore_symtable.h"      // struct symtable

#include "clinic/symtablemodule.c.h"
/*[clinic input]
module _symtable
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f4685845a7100605]*/


static struct symtable *
symtable_from_ast(PyObject *source, PyObject *filename, int compile_mode)
{
    PyArena *arena = _PyArena_New();
    if (arena == NULL) {
        return NULL;
    }
    struct symtable *st = NULL;
    mod_ty mod = PyAST_obj2mod(source, arena, compile_mode);
    if (mod == NULL || !_PyAST_Validate(mod)) {
        goto finally;
    }
    _PyFutureFeatures future;
    if (!_PyFuture_FromAST(mod, filename, &future)) {
        goto finally;
    }
    st = _PySymtable_Build(mod, filename, &future);
finally:
    _PyArena_Free(arena);
    return st;
}


/*[clinic input]
_symtable.symtable

    source:    object
    filename:  unicode_fs_decoded
    startstr:  str
    /
    *
    module as modname: object = None

Return symbol and scope dictionaries used internally by compiler.

The source can be a string, a bytes object, or an AST object.
[clinic start generated code]*/

static PyObject *
_symtable_symtable_impl(PyObject *module, PyObject *source,
                        PyObject *filename, const char *startstr,
                        PyObject *modname)
/*[clinic end generated code: output=235ec5a87a9ce178 input=6cadac0485f576a7]*/
{
    struct symtable *st;
    PyObject *t;
    int compile_mode;

    if (strcmp(startstr, "exec") == 0)
        compile_mode = 0;
    else if (strcmp(startstr, "eval") == 0)
        compile_mode = 1;
    else if (strcmp(startstr, "single") == 0)
        compile_mode = 2;
    else {
        PyErr_SetString(PyExc_ValueError,
           "symtable() arg 3 must be 'exec' or 'eval' or 'single'");
        return NULL;
    }
    if (modname == Py_None) {
        modname = NULL;
    }
    else if (!PyUnicode_Check(modname)) {
        PyErr_Format(PyExc_TypeError,
                     "symtable() argument 'module' must be str or None, not %T",
                     modname);
        return NULL;
    }

    if (PyAST_Check(source)) {
        st = symtable_from_ast(source, filename, compile_mode);
    }
    else {
        static const int starts[] = {
            Py_file_input, Py_eval_input, Py_single_input};
        PyCompilerFlags cf = _PyCompilerFlags_INIT;
        PyObject *source_copy = NULL;

        cf.cf_flags = PyCF_SOURCE_IS_UTF8;
        const char *str = _Py_SourceAsString(source, "symtable",
                                             "string, bytes or AST",
                                             &cf, &source_copy);
        if (str == NULL) {
            return NULL;
        }
        st = _Py_SymtableStringObjectFlags(str, filename,
                                           starts[compile_mode], &cf,
                                           modname);
        Py_XDECREF(source_copy);
    }
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
    _Py_ABI_SLOT,
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
