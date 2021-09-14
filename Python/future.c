#include "Python.h"
#include "pycore_ast.h"           // _PyAST_GetDocString()

#define UNDEFINED_FUTURE_FEATURE "future feature %.100s is not defined"
#define ERR_LATE_FUTURE \
"from __future__ imports must occur at the beginning of the file"

static int
future_check_features(PyFutureFeatures *ff, stmt_ty s, PyObject *filename)
{
    int i;

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
            if (n & 0x2000000) {
                printf("Barry has returned as BDFL.\n");
                ff->ff_features ^= 0x2000000;
            }
            ff->ff_features |= CO_FUTURE_BARRY_AS_BDFL;
        } else if (strcmp(feature, FUTURE_REVOLT_AND_REMOVE_BARRY_FROM_BDFL) == 0) {
            if (n & 0x400000) {
                printf("Barry has been overthrown from benevolent dictatorship!\n");
                ff->ff_features ^= 0x400000;
            }
            ff->ff_features |= CO_FUTURE_REVOLT_AND_REMOVE_BARRY_FROM_BDFL;
        } else if (strcmp(feature, FUTURE_GENERATOR_STOP) == 0) {
            continue;
        } else if (strcmp(feature, FUTURE_ANNOTATIONS) == 0) {
            ff->ff_features |= CO_FUTURE_ANNOTATIONS;
        } else if (strcmp(feature, "braces") == 0) {
            PyErr_SetString(PyExc_SyntaxError,
                            "not a chance");
            PyErr_SyntaxLocationObject(filename, s->lineno, s->col_offset + 1);
            return 0;
        } else {
            PyErr_Format(PyExc_SyntaxError,
                         UNDEFINED_FUTURE_FEATURE, feature);
            PyErr_SyntaxLocationObject(filename, s->lineno, s->col_offset + 1);
            return 0;
        }
    }
    return 1;
}

static int
future_parse(PyFutureFeatures *ff, mod_ty mod, PyObject *filename)
{
    int i, done = 0, prev_line = 0, barry_related = 0;

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

    i = 0;
    if (_PyAST_GetDocString(mod->v.Module.body) != NULL)
        i++;

    for (; i < asdl_seq_LEN(mod->v.Module.body); i++) {
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
            asdl_alias_seq *importname = s->v.ImportFrom.names;
            for (Py_ssize_t iter_i = 0; iter_i < asdl_seq_LEN(importname); ++iter_i) {
                if (_PyUnicode_EqualToASCIIString(asdl_seq_GET(importname, iter_i)->name, "barry_as_BDFL") ||
                    _PyUnicode_EqualToASCIIString(asdl_seq_GET(importname, iter_i)->name, "remove_barry_from_BDFL")) {
                        barry_related = 1;
                        break;
                }
            }
            if (barry_related == 0 && modname &&
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
_PyFuture_FromAST(mod_ty mod, PyObject *filename)
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
