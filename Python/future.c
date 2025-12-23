#include "Python.h"
#include "pycore_ast.h"           // _PyAST_GetDocString()
#include "pycore_symtable.h"      // _PyFutureFeatures
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString()

#define UNDEFINED_FUTURE_FEATURE "future feature %.100s is not defined"

static int
future_check_features(_PyFutureFeatures *ff, stmt_ty s, PyObject *filename)
{
    Py_ssize_t i;

    assert(s->kind == ImportFrom_kind);

    asdl_alias_seq *names = s->v.ImportFrom.names;
    for (i = 0; i < asdl_seq_LEN(names); i++) {
        alias_ty name = (alias_ty)asdl_seq_GET(names, i);
        const char *feature = PyUnicode_AsUTF8(name->name);
        if (!feature)
            return 0;
        if (strcmp(feature, FUTURE_NESTED_SCOPES) == 0) {
            continue;
        } else if (strcmp(feature, FUTURE_GENERATORS) == 0) {
            continue;
        } else if (strcmp(feature, FUTURE_DIVISION) == 0) {
            continue;
        } else if (strcmp(feature, FUTURE_ABSOLUTE_IMPORT) == 0) {
            continue;
        } else if (strcmp(feature, FUTURE_WITH_STATEMENT) == 0) {
            continue;
        } else if (strcmp(feature, FUTURE_PRINT_FUNCTION) == 0) {
            continue;
        } else if (strcmp(feature, FUTURE_UNICODE_LITERALS) == 0) {
            continue;
        } else if (strcmp(feature, FUTURE_BARRY_AS_BDFL) == 0) {
            ff->ff_features |= CO_FUTURE_BARRY_AS_BDFL;
        } else if (strcmp(feature, FUTURE_GENERATOR_STOP) == 0) {
            continue;
        } else if (strcmp(feature, FUTURE_ANNOTATIONS) == 0) {
            ff->ff_features |= CO_FUTURE_ANNOTATIONS;
        } else if (strcmp(feature, "braces") == 0) {
            PyErr_SetString(PyExc_SyntaxError,
                            "not a chance");
            PyErr_RangedSyntaxLocationObject(filename,
                                             name->lineno,
                                             name->col_offset + 1,
                                             name->end_lineno,
                                             name->end_col_offset + 1);
            return 0;
        } else {
            PyErr_Format(PyExc_SyntaxError,
                         UNDEFINED_FUTURE_FEATURE, feature);
            PyErr_RangedSyntaxLocationObject(filename,
                                             name->lineno,
                                             name->col_offset + 1,
                                             name->end_lineno,
                                             name->end_col_offset + 1);
            return 0;
        }
    }
    return 1;
}

static int
future_parse(_PyFutureFeatures *ff, mod_ty mod, PyObject *filename)
{
    if (!(mod->kind == Module_kind || mod->kind == Interactive_kind)) {
        return 1;
    }

    Py_ssize_t n = asdl_seq_LEN(mod->v.Module.body);
    if (n == 0) {
        return 1;
    }

    Py_ssize_t i = 0;
    if (_PyAST_GetDocString(mod->v.Module.body) != NULL) {
        i++;
    }

    for (; i < n; i++) {
        stmt_ty s = (stmt_ty)asdl_seq_GET(mod->v.Module.body, i);

        /* The only things that can precede a future statement
         *  are another future statement and a doc string.
         */

        if (s->kind == ImportFrom_kind && s->v.ImportFrom.level == 0) {
            identifier modname = s->v.ImportFrom.module;
            if (modname &&
                _PyUnicode_EqualToASCIIString(modname, "__future__")) {
                if (!future_check_features(ff, s, filename)) {
                    return 0;
                }
                ff->ff_location = SRC_LOCATION_FROM_AST(s);
            }
            else {
                return 1;
            }
        }
        else {
            return 1;
        }
    }
    return 1;
}


int
_PyFuture_FromAST(mod_ty mod, PyObject *filename, _PyFutureFeatures *ff)
{
    ff->ff_features = 0;
    ff->ff_location = (_Py_SourceLocation){-1, -1, -1, -1};

    if (!future_parse(ff, mod, filename)) {
        return 0;
    }
    return 1;
}
