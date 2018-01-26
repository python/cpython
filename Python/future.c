#include "Python.h"
#include "Python-ast.h"
#include "node.h"
#include "token.h"
#include "graminit.h"
#include "code.h"
#include "symtable.h"

#define UNDEFINED_FUTURE_FEATURE "future feature %.100s is not defined"
#define ERR_LATE_FUTURE \
"from __future__ imports must occur at the beginning of the file"

static int
future_check_features(PyFutureFeatures *ff, stmt_ty s, PyObject *filename)
{
    int i;
    asdl_seq *names;

    assert(s->kind == ImportFrom_kind);

    names = s->v.ImportFrom.names;
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
            PyErr_SyntaxLocationObject(filename, s->lineno, s->col_offset);
            return 0;
        } else {
            PyErr_Format(PyExc_SyntaxError,
                         UNDEFINED_FUTURE_FEATURE, feature);
            PyErr_SyntaxLocationObject(filename, s->lineno, s->col_offset);
            return 0;
        }
    }
    return 1;
}

static int
future_parse(PyFutureFeatures *ff, mod_ty mod, PyObject *filename)
{
    int i, done = 0, prev_line = 0;

    if (!(mod->kind == Module_kind || mod->kind == Interactive_kind))
        return 1;

    if (asdl_seq_LEN(mod->v.Module.body) == 0)
        return 1;

    /* A subsequent pass will detect future imports that don't
       appear at the beginning of the file.  There's one case,
       however, that is easier to handle here: A series of imports
       joined by semi-colons, where the first import is a future
       statement but some subsequent import has the future form
       but is preceded by a regular import.
    */

    for (i = 0; i < asdl_seq_LEN(mod->v.Module.body); i++) {
        stmt_ty s = (stmt_ty)asdl_seq_GET(mod->v.Module.body, i);

        if (done && s->lineno > prev_line)
            return 1;
        prev_line = s->lineno;

        /* The tests below will return from this function unless it is
           still possible to find a future statement.  The only things
           that can precede a future statement are another future
           statement and a doc string.
        */

        if (s->kind == ImportFrom_kind) {
            identifier modname = s->v.ImportFrom.module;
            if (modname &&
                _PyUnicode_EqualToASCIIString(modname, "__future__")) {
                if (done) {
                    PyErr_SetString(PyExc_SyntaxError,
                                    ERR_LATE_FUTURE);
                    PyErr_SyntaxLocationObject(filename, s->lineno, s->col_offset);
                    return 0;
                }
                if (!future_check_features(ff, s, filename))
                    return 0;
                ff->ff_lineno = s->lineno;
            }
            else {
                done = 1;
            }
        }
        else {
            done = 1;
        }
    }
    return 1;
}


PyFutureFeatures *
PyFuture_FromASTObject(mod_ty mod, PyObject *filename)
{
    PyFutureFeatures *ff;

    ff = (PyFutureFeatures *)PyObject_Malloc(sizeof(PyFutureFeatures));
    if (ff == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    ff->ff_features = 0;
    ff->ff_lineno = -1;

    if (!future_parse(ff, mod, filename)) {
        PyObject_Free(ff);
        return NULL;
    }
    return ff;
}


PyFutureFeatures *
PyFuture_FromAST(mod_ty mod, const char *filename_str)
{
    PyFutureFeatures *ff;
    PyObject *filename;

    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    ff = PyFuture_FromASTObject(mod, filename);
    Py_DECREF(filename);
    return ff;
}
