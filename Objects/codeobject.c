#include <stdbool.h>

#include "Python.h"
#include "code.h"
#include "opcode.h"
#include "structmember.h"         // PyMemberDef
#include "pycore_code.h"          // _PyOpcache
#include "pycore_interp.h"        // PyInterpreterState.co_extra_freefuncs
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "clinic/codeobject.c.h"

/* Holder for co_extra information */
typedef struct {
    Py_ssize_t ce_size;
    void *ce_extras[1];
} _PyCodeObjectExtra;

/*[clinic input]
class code "PyCodeObject *" "&PyCode_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=78aa5d576683bb4b]*/

/* all_name_chars(s): true iff s matches [a-zA-Z0-9_]* */
static int
all_name_chars(PyObject *o)
{
    const unsigned char *s, *e;

    if (!PyUnicode_IS_ASCII(o))
        return 0;

    s = PyUnicode_1BYTE_DATA(o);
    e = s + PyUnicode_GET_LENGTH(o);
    for (; s != e; s++) {
        if (!Py_ISALNUM(*s) && *s != '_')
            return 0;
    }
    return 1;
}

static int
intern_strings(PyObject *tuple)
{
    Py_ssize_t i;

    for (i = PyTuple_GET_SIZE(tuple); --i >= 0; ) {
        PyObject *v = PyTuple_GET_ITEM(tuple, i);
        if (v == NULL || !PyUnicode_CheckExact(v)) {
            PyErr_SetString(PyExc_SystemError,
                            "non-string found in code slot");
            return -1;
        }
        PyUnicode_InternInPlace(&_PyTuple_ITEMS(tuple)[i]);
    }
    return 0;
}

/* Intern selected string constants */
static int
intern_string_constants(PyObject *tuple, int *modified)
{
    for (Py_ssize_t i = PyTuple_GET_SIZE(tuple); --i >= 0; ) {
        PyObject *v = PyTuple_GET_ITEM(tuple, i);
        if (PyUnicode_CheckExact(v)) {
            if (PyUnicode_READY(v) == -1) {
                return -1;
            }

            if (all_name_chars(v)) {
                PyObject *w = v;
                PyUnicode_InternInPlace(&v);
                if (w != v) {
                    PyTuple_SET_ITEM(tuple, i, v);
                    if (modified) {
                        *modified = 1;
                    }
                }
            }
        }
        else if (PyTuple_CheckExact(v)) {
            if (intern_string_constants(v, NULL) < 0) {
                return -1;
            }
        }
        else if (PyFrozenSet_CheckExact(v)) {
            PyObject *w = v;
            PyObject *tmp = PySequence_Tuple(v);
            if (tmp == NULL) {
                return -1;
            }
            int tmp_modified = 0;
            if (intern_string_constants(tmp, &tmp_modified) < 0) {
                Py_DECREF(tmp);
                return -1;
            }
            if (tmp_modified) {
                v = PyFrozenSet_New(tmp);
                if (v == NULL) {
                    Py_DECREF(tmp);
                    return -1;
                }

                PyTuple_SET_ITEM(tuple, i, v);
                Py_DECREF(w);
                if (modified) {
                    *modified = 1;
                }
            }
            Py_DECREF(tmp);
        }
    }
    return 0;
}

PyCodeObject *
PyCode_NewWithPosOnlyArgs(int argcount, int posonlyargcount, int kwonlyargcount,
                          int nlocals, int stacksize, int flags,
                          PyObject *code, PyObject *consts, PyObject *names,
                          PyObject *varnames, PyObject *freevars, PyObject *cellvars,
                          PyObject *filename, PyObject *name, int firstlineno,
                          PyObject *lnotab)
{
    PyCodeObject *co;
    Py_ssize_t *cell2arg = NULL;
    Py_ssize_t i, n_cellvars, n_varnames, total_args;

    /* Check argument types */
    if (argcount < posonlyargcount || posonlyargcount < 0 ||
        kwonlyargcount < 0 || nlocals < 0 ||
        stacksize < 0 || flags < 0 ||
        code == NULL || !PyBytes_Check(code) ||
        consts == NULL || !PyTuple_Check(consts) ||
        names == NULL || !PyTuple_Check(names) ||
        varnames == NULL || !PyTuple_Check(varnames) ||
        freevars == NULL || !PyTuple_Check(freevars) ||
        cellvars == NULL || !PyTuple_Check(cellvars) ||
        name == NULL || !PyUnicode_Check(name) ||
        filename == NULL || !PyUnicode_Check(filename) ||
        lnotab == NULL || !PyBytes_Check(lnotab)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    /* Ensure that strings are ready Unicode string */
    if (PyUnicode_READY(name) < 0) {
        return NULL;
    }
    if (PyUnicode_READY(filename) < 0) {
        return NULL;
    }

    if (intern_strings(names) < 0) {
        return NULL;
    }
    if (intern_strings(varnames) < 0) {
        return NULL;
    }
    if (intern_strings(freevars) < 0) {
        return NULL;
    }
    if (intern_strings(cellvars) < 0) {
        return NULL;
    }
    if (intern_string_constants(consts, NULL) < 0) {
        return NULL;
    }

    /* Make sure that code is indexable with an int, this is
       a long running assumption in ceval.c and many parts of
       the interpreter. */
    if (PyBytes_GET_SIZE(code) > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "co_code larger than INT_MAX");
        return NULL;
    }

    /* Check for any inner or outer closure references */
    n_cellvars = PyTuple_GET_SIZE(cellvars);
    if (!n_cellvars && !PyTuple_GET_SIZE(freevars)) {
        flags |= CO_NOFREE;
    } else {
        flags &= ~CO_NOFREE;
    }

    n_varnames = PyTuple_GET_SIZE(varnames);
    if (argcount <= n_varnames && kwonlyargcount <= n_varnames) {
        /* Never overflows. */
        total_args = (Py_ssize_t)argcount + (Py_ssize_t)kwonlyargcount +
                      ((flags & CO_VARARGS) != 0) + ((flags & CO_VARKEYWORDS) != 0);
    }
    else {
        total_args = n_varnames + 1;
    }
    if (total_args > n_varnames) {
        PyErr_SetString(PyExc_ValueError, "code: varnames is too small");
        return NULL;
    }

    /* Create mapping between cells and arguments if needed. */
    if (n_cellvars) {
        bool used_cell2arg = false;
        cell2arg = PyMem_NEW(Py_ssize_t, n_cellvars);
        if (cell2arg == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
        /* Find cells which are also arguments. */
        for (i = 0; i < n_cellvars; i++) {
            Py_ssize_t j;
            PyObject *cell = PyTuple_GET_ITEM(cellvars, i);
            cell2arg[i] = CO_CELL_NOT_AN_ARG;
            for (j = 0; j < total_args; j++) {
                PyObject *arg = PyTuple_GET_ITEM(varnames, j);
                int cmp = PyUnicode_Compare(cell, arg);
                if (cmp == -1 && PyErr_Occurred()) {
                    PyMem_FREE(cell2arg);
                    return NULL;
                }
                if (cmp == 0) {
                    cell2arg[i] = j;
                    used_cell2arg = true;
                    break;
                }
            }
        }
        if (!used_cell2arg) {
            PyMem_FREE(cell2arg);
            cell2arg = NULL;
        }
    }
    co = PyObject_New(PyCodeObject, &PyCode_Type);
    if (co == NULL) {
        if (cell2arg)
            PyMem_FREE(cell2arg);
        return NULL;
    }
    co->co_argcount = argcount;
    co->co_posonlyargcount = posonlyargcount;
    co->co_kwonlyargcount = kwonlyargcount;
    co->co_nlocals = nlocals;
    co->co_stacksize = stacksize;
    co->co_flags = flags;
    Py_INCREF(code);
    co->co_code = code;
    Py_INCREF(consts);
    co->co_consts = consts;
    Py_INCREF(names);
    co->co_names = names;
    Py_INCREF(varnames);
    co->co_varnames = varnames;
    Py_INCREF(freevars);
    co->co_freevars = freevars;
    Py_INCREF(cellvars);
    co->co_cellvars = cellvars;
    co->co_cell2arg = cell2arg;
    Py_INCREF(filename);
    co->co_filename = filename;
    Py_INCREF(name);
    co->co_name = name;
    co->co_firstlineno = firstlineno;
    Py_INCREF(lnotab);
    co->co_lnotab = lnotab;
    co->co_zombieframe = NULL;
    co->co_weakreflist = NULL;
    co->co_extra = NULL;

    co->co_opcache_map = NULL;
    co->co_opcache = NULL;
    co->co_opcache_flag = 0;
    co->co_opcache_size = 0;
    return co;
}

PyCodeObject *
PyCode_New(int argcount, int kwonlyargcount,
           int nlocals, int stacksize, int flags,
           PyObject *code, PyObject *consts, PyObject *names,
           PyObject *varnames, PyObject *freevars, PyObject *cellvars,
           PyObject *filename, PyObject *name, int firstlineno,
           PyObject *lnotab)
{
    return PyCode_NewWithPosOnlyArgs(argcount, 0, kwonlyargcount, nlocals,
                                     stacksize, flags, code, consts, names,
                                     varnames, freevars, cellvars, filename,
                                     name, firstlineno, lnotab);
}

int
_PyCode_InitOpcache(PyCodeObject *co)
{
    Py_ssize_t co_size = PyBytes_Size(co->co_code) / sizeof(_Py_CODEUNIT);
    co->co_opcache_map = (unsigned char *)PyMem_Calloc(co_size, 1);
    if (co->co_opcache_map == NULL) {
        return -1;
    }

    _Py_CODEUNIT *opcodes = (_Py_CODEUNIT*)PyBytes_AS_STRING(co->co_code);
    Py_ssize_t opts = 0;

    for (Py_ssize_t i = 0; i < co_size;) {
        unsigned char opcode = _Py_OPCODE(opcodes[i]);
        i++;  // 'i' is now aligned to (next_instr - first_instr)

        // TODO: LOAD_METHOD, LOAD_ATTR
        if (opcode == LOAD_GLOBAL) {
            opts++;
            co->co_opcache_map[i] = (unsigned char)opts;
            if (opts > 254) {
                break;
            }
        }
    }

    if (opts) {
        co->co_opcache = (_PyOpcache *)PyMem_Calloc(opts, sizeof(_PyOpcache));
        if (co->co_opcache == NULL) {
            PyMem_FREE(co->co_opcache_map);
            return -1;
        }
    }
    else {
        PyMem_FREE(co->co_opcache_map);
        co->co_opcache_map = NULL;
        co->co_opcache = NULL;
    }

    co->co_opcache_size = (unsigned char)opts;
    return 0;
}

PyCodeObject *
PyCode_NewEmpty(const char *filename, const char *funcname, int firstlineno)
{
    static PyObject *emptystring = NULL;
    static PyObject *nulltuple = NULL;
    PyObject *filename_ob = NULL;
    PyObject *funcname_ob = NULL;
    PyCodeObject *result = NULL;
    if (emptystring == NULL) {
        emptystring = PyBytes_FromString("");
        if (emptystring == NULL)
            goto failed;
    }
    if (nulltuple == NULL) {
        nulltuple = PyTuple_New(0);
        if (nulltuple == NULL)
            goto failed;
    }
    funcname_ob = PyUnicode_FromString(funcname);
    if (funcname_ob == NULL)
        goto failed;
    filename_ob = PyUnicode_DecodeFSDefault(filename);
    if (filename_ob == NULL)
        goto failed;

    result = PyCode_NewWithPosOnlyArgs(
                0,                    /* argcount */
                0,                              /* posonlyargcount */
                0,                              /* kwonlyargcount */
                0,                              /* nlocals */
                0,                              /* stacksize */
                0,                              /* flags */
                emptystring,                    /* code */
                nulltuple,                      /* consts */
                nulltuple,                      /* names */
                nulltuple,                      /* varnames */
                nulltuple,                      /* freevars */
                nulltuple,                      /* cellvars */
                filename_ob,                    /* filename */
                funcname_ob,                    /* name */
                firstlineno,                    /* firstlineno */
                emptystring                     /* lnotab */
                );

failed:
    Py_XDECREF(funcname_ob);
    Py_XDECREF(filename_ob);
    return result;
}

#define OFF(x) offsetof(PyCodeObject, x)

static PyMemberDef code_memberlist[] = {
    {"co_argcount",     T_INT,          OFF(co_argcount),        READONLY},
    {"co_posonlyargcount",      T_INT,  OFF(co_posonlyargcount), READONLY},
    {"co_kwonlyargcount",       T_INT,  OFF(co_kwonlyargcount),  READONLY},
    {"co_nlocals",      T_INT,          OFF(co_nlocals),         READONLY},
    {"co_stacksize",T_INT,              OFF(co_stacksize),       READONLY},
    {"co_flags",        T_INT,          OFF(co_flags),           READONLY},
    {"co_code",         T_OBJECT,       OFF(co_code),            READONLY},
    {"co_consts",       T_OBJECT,       OFF(co_consts),          READONLY},
    {"co_names",        T_OBJECT,       OFF(co_names),           READONLY},
    {"co_varnames",     T_OBJECT,       OFF(co_varnames),        READONLY},
    {"co_freevars",     T_OBJECT,       OFF(co_freevars),        READONLY},
    {"co_cellvars",     T_OBJECT,       OFF(co_cellvars),        READONLY},
    {"co_filename",     T_OBJECT,       OFF(co_filename),        READONLY},
    {"co_name",         T_OBJECT,       OFF(co_name),            READONLY},
    {"co_firstlineno", T_INT,           OFF(co_firstlineno),     READONLY},
    {"co_lnotab",       T_OBJECT,       OFF(co_lnotab),          READONLY},
    {NULL}      /* Sentinel */
};

/* Helper for code_new: return a shallow copy of a tuple that is
   guaranteed to contain exact strings, by converting string subclasses
   to exact strings and complaining if a non-string is found. */
static PyObject*
validate_and_copy_tuple(PyObject *tup)
{
    PyObject *newtuple;
    PyObject *item;
    Py_ssize_t i, len;

    len = PyTuple_GET_SIZE(tup);
    newtuple = PyTuple_New(len);
    if (newtuple == NULL)
        return NULL;

    for (i = 0; i < len; i++) {
        item = PyTuple_GET_ITEM(tup, i);
        if (PyUnicode_CheckExact(item)) {
            Py_INCREF(item);
        }
        else if (!PyUnicode_Check(item)) {
            PyErr_Format(
                PyExc_TypeError,
                "name tuples must contain only "
                "strings, not '%.500s'",
                Py_TYPE(item)->tp_name);
            Py_DECREF(newtuple);
            return NULL;
        }
        else {
            item = _PyUnicode_Copy(item);
            if (item == NULL) {
                Py_DECREF(newtuple);
                return NULL;
            }
        }
        PyTuple_SET_ITEM(newtuple, i, item);
    }

    return newtuple;
}

/*[clinic input]
@classmethod
code.__new__ as code_new

    argcount: int
    posonlyargcount: int
    kwonlyargcount: int
    nlocals: int
    stacksize: int
    flags: int
    codestring as code: object(subclass_of="&PyBytes_Type")
    constants as consts: object(subclass_of="&PyTuple_Type")
    names: object(subclass_of="&PyTuple_Type")
    varnames: object(subclass_of="&PyTuple_Type")
    filename: unicode
    name: unicode
    firstlineno: int
    lnotab: object(subclass_of="&PyBytes_Type")
    freevars: object(subclass_of="&PyTuple_Type", c_default="NULL") = ()
    cellvars: object(subclass_of="&PyTuple_Type", c_default="NULL") = ()
    /

Create a code object.  Not for the faint of heart.
[clinic start generated code]*/

static PyObject *
code_new_impl(PyTypeObject *type, int argcount, int posonlyargcount,
              int kwonlyargcount, int nlocals, int stacksize, int flags,
              PyObject *code, PyObject *consts, PyObject *names,
              PyObject *varnames, PyObject *filename, PyObject *name,
              int firstlineno, PyObject *lnotab, PyObject *freevars,
              PyObject *cellvars)
/*[clinic end generated code: output=612aac5395830184 input=85e678ea4178f234]*/
{
    PyObject *co = NULL;
    PyObject *ournames = NULL;
    PyObject *ourvarnames = NULL;
    PyObject *ourfreevars = NULL;
    PyObject *ourcellvars = NULL;

    if (PySys_Audit("code.__new__", "OOOiiiiii",
                    code, filename, name, argcount, posonlyargcount,
                    kwonlyargcount, nlocals, stacksize, flags) < 0) {
        goto cleanup;
    }

    if (argcount < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: argcount must not be negative");
        goto cleanup;
    }

    if (posonlyargcount < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: posonlyargcount must not be negative");
        goto cleanup;
    }

    if (kwonlyargcount < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: kwonlyargcount must not be negative");
        goto cleanup;
    }
    if (nlocals < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: nlocals must not be negative");
        goto cleanup;
    }

    ournames = validate_and_copy_tuple(names);
    if (ournames == NULL)
        goto cleanup;
    ourvarnames = validate_and_copy_tuple(varnames);
    if (ourvarnames == NULL)
        goto cleanup;
    if (freevars)
        ourfreevars = validate_and_copy_tuple(freevars);
    else
        ourfreevars = PyTuple_New(0);
    if (ourfreevars == NULL)
        goto cleanup;
    if (cellvars)
        ourcellvars = validate_and_copy_tuple(cellvars);
    else
        ourcellvars = PyTuple_New(0);
    if (ourcellvars == NULL)
        goto cleanup;

    co = (PyObject *)PyCode_NewWithPosOnlyArgs(argcount, posonlyargcount,
                                               kwonlyargcount,
                                               nlocals, stacksize, flags,
                                               code, consts, ournames,
                                               ourvarnames, ourfreevars,
                                               ourcellvars, filename,
                                               name, firstlineno, lnotab);
  cleanup:
    Py_XDECREF(ournames);
    Py_XDECREF(ourvarnames);
    Py_XDECREF(ourfreevars);
    Py_XDECREF(ourcellvars);
    return co;
}

static void
code_dealloc(PyCodeObject *co)
{
    if (co->co_opcache != NULL) {
        PyMem_FREE(co->co_opcache);
    }
    if (co->co_opcache_map != NULL) {
        PyMem_FREE(co->co_opcache_map);
    }
    co->co_opcache_flag = 0;
    co->co_opcache_size = 0;

    if (co->co_extra != NULL) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        _PyCodeObjectExtra *co_extra = co->co_extra;

        for (Py_ssize_t i = 0; i < co_extra->ce_size; i++) {
            freefunc free_extra = interp->co_extra_freefuncs[i];

            if (free_extra != NULL) {
                free_extra(co_extra->ce_extras[i]);
            }
        }

        PyMem_Free(co_extra);
    }

    Py_XDECREF(co->co_code);
    Py_XDECREF(co->co_consts);
    Py_XDECREF(co->co_names);
    Py_XDECREF(co->co_varnames);
    Py_XDECREF(co->co_freevars);
    Py_XDECREF(co->co_cellvars);
    Py_XDECREF(co->co_filename);
    Py_XDECREF(co->co_name);
    Py_XDECREF(co->co_lnotab);
    if (co->co_cell2arg != NULL)
        PyMem_FREE(co->co_cell2arg);
    if (co->co_zombieframe != NULL)
        PyObject_GC_Del(co->co_zombieframe);
    if (co->co_weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject*)co);
    PyObject_DEL(co);
}

static PyObject *
code_sizeof(PyCodeObject *co, PyObject *Py_UNUSED(args))
{
    Py_ssize_t res = _PyObject_SIZE(Py_TYPE(co));
    _PyCodeObjectExtra *co_extra = (_PyCodeObjectExtra*) co->co_extra;

    if (co->co_cell2arg != NULL && co->co_cellvars != NULL) {
        res += PyTuple_GET_SIZE(co->co_cellvars) * sizeof(Py_ssize_t);
    }
    if (co_extra != NULL) {
        res += sizeof(_PyCodeObjectExtra) +
               (co_extra->ce_size-1) * sizeof(co_extra->ce_extras[0]);
    }
    if (co->co_opcache != NULL) {
        assert(co->co_opcache_map != NULL);
        // co_opcache_map
        res += PyBytes_GET_SIZE(co->co_code) / sizeof(_Py_CODEUNIT);
        // co_opcache
        res += co->co_opcache_size * sizeof(_PyOpcache);
    }
    return PyLong_FromSsize_t(res);
}

/*[clinic input]
code.replace

    *
    co_argcount: int(c_default="self->co_argcount") = -1
    co_posonlyargcount: int(c_default="self->co_posonlyargcount") = -1
    co_kwonlyargcount: int(c_default="self->co_kwonlyargcount") = -1
    co_nlocals: int(c_default="self->co_nlocals") = -1
    co_stacksize: int(c_default="self->co_stacksize") = -1
    co_flags: int(c_default="self->co_flags") = -1
    co_firstlineno: int(c_default="self->co_firstlineno") = -1
    co_code: PyBytesObject(c_default="(PyBytesObject *)self->co_code") = None
    co_consts: object(subclass_of="&PyTuple_Type", c_default="self->co_consts") = None
    co_names: object(subclass_of="&PyTuple_Type", c_default="self->co_names") = None
    co_varnames: object(subclass_of="&PyTuple_Type", c_default="self->co_varnames") = None
    co_freevars: object(subclass_of="&PyTuple_Type", c_default="self->co_freevars") = None
    co_cellvars: object(subclass_of="&PyTuple_Type", c_default="self->co_cellvars") = None
    co_filename: unicode(c_default="self->co_filename") = None
    co_name: unicode(c_default="self->co_name") = None
    co_lnotab: PyBytesObject(c_default="(PyBytesObject *)self->co_lnotab") = None

Return a copy of the code object with new values for the specified fields.
[clinic start generated code]*/

static PyObject *
code_replace_impl(PyCodeObject *self, int co_argcount,
                  int co_posonlyargcount, int co_kwonlyargcount,
                  int co_nlocals, int co_stacksize, int co_flags,
                  int co_firstlineno, PyBytesObject *co_code,
                  PyObject *co_consts, PyObject *co_names,
                  PyObject *co_varnames, PyObject *co_freevars,
                  PyObject *co_cellvars, PyObject *co_filename,
                  PyObject *co_name, PyBytesObject *co_lnotab)
/*[clinic end generated code: output=25c8e303913bcace input=d9051bc8f24e6b28]*/
{
#define CHECK_INT_ARG(ARG) \
        if (ARG < 0) { \
            PyErr_SetString(PyExc_ValueError, \
                            #ARG " must be a positive integer"); \
            return NULL; \
        }

    CHECK_INT_ARG(co_argcount);
    CHECK_INT_ARG(co_posonlyargcount);
    CHECK_INT_ARG(co_kwonlyargcount);
    CHECK_INT_ARG(co_nlocals);
    CHECK_INT_ARG(co_stacksize);
    CHECK_INT_ARG(co_flags);
    CHECK_INT_ARG(co_firstlineno);

#undef CHECK_INT_ARG

    if (PySys_Audit("code.__new__", "OOOiiiiii",
                    co_code, co_filename, co_name, co_argcount,
                    co_posonlyargcount, co_kwonlyargcount, co_nlocals,
                    co_stacksize, co_flags) < 0) {
        return NULL;
    }

    return (PyObject *)PyCode_NewWithPosOnlyArgs(
        co_argcount, co_posonlyargcount, co_kwonlyargcount, co_nlocals,
        co_stacksize, co_flags, (PyObject*)co_code, co_consts, co_names,
        co_varnames, co_freevars, co_cellvars, co_filename, co_name,
        co_firstlineno, (PyObject*)co_lnotab);
}

static PyObject *
code_repr(PyCodeObject *co)
{
    int lineno;
    if (co->co_firstlineno != 0)
        lineno = co->co_firstlineno;
    else
        lineno = -1;
    if (co->co_filename && PyUnicode_Check(co->co_filename)) {
        return PyUnicode_FromFormat(
            "<code object %U at %p, file \"%U\", line %d>",
            co->co_name, co, co->co_filename, lineno);
    } else {
        return PyUnicode_FromFormat(
            "<code object %U at %p, file ???, line %d>",
            co->co_name, co, lineno);
    }
}

PyObject*
_PyCode_ConstantKey(PyObject *op)
{
    PyObject *key;

    /* Py_None and Py_Ellipsis are singletons. */
    if (op == Py_None || op == Py_Ellipsis
       || PyLong_CheckExact(op)
       || PyUnicode_CheckExact(op)
          /* code_richcompare() uses _PyCode_ConstantKey() internally */
       || PyCode_Check(op))
    {
        /* Objects of these types are always different from object of other
         * type and from tuples. */
        Py_INCREF(op);
        key = op;
    }
    else if (PyBool_Check(op) || PyBytes_CheckExact(op)) {
        /* Make booleans different from integers 0 and 1.
         * Avoid BytesWarning from comparing bytes with strings. */
        key = PyTuple_Pack(2, Py_TYPE(op), op);
    }
    else if (PyFloat_CheckExact(op)) {
        double d = PyFloat_AS_DOUBLE(op);
        /* all we need is to make the tuple different in either the 0.0
         * or -0.0 case from all others, just to avoid the "coercion".
         */
        if (d == 0.0 && copysign(1.0, d) < 0.0)
            key = PyTuple_Pack(3, Py_TYPE(op), op, Py_None);
        else
            key = PyTuple_Pack(2, Py_TYPE(op), op);
    }
    else if (PyComplex_CheckExact(op)) {
        Py_complex z;
        int real_negzero, imag_negzero;
        /* For the complex case we must make complex(x, 0.)
           different from complex(x, -0.) and complex(0., y)
           different from complex(-0., y), for any x and y.
           All four complex zeros must be distinguished.*/
        z = PyComplex_AsCComplex(op);
        real_negzero = z.real == 0.0 && copysign(1.0, z.real) < 0.0;
        imag_negzero = z.imag == 0.0 && copysign(1.0, z.imag) < 0.0;
        /* use True, False and None singleton as tags for the real and imag
         * sign, to make tuples different */
        if (real_negzero && imag_negzero) {
            key = PyTuple_Pack(3, Py_TYPE(op), op, Py_True);
        }
        else if (imag_negzero) {
            key = PyTuple_Pack(3, Py_TYPE(op), op, Py_False);
        }
        else if (real_negzero) {
            key = PyTuple_Pack(3, Py_TYPE(op), op, Py_None);
        }
        else {
            key = PyTuple_Pack(2, Py_TYPE(op), op);
        }
    }
    else if (PyTuple_CheckExact(op)) {
        Py_ssize_t i, len;
        PyObject *tuple;

        len = PyTuple_GET_SIZE(op);
        tuple = PyTuple_New(len);
        if (tuple == NULL)
            return NULL;

        for (i=0; i < len; i++) {
            PyObject *item, *item_key;

            item = PyTuple_GET_ITEM(op, i);
            item_key = _PyCode_ConstantKey(item);
            if (item_key == NULL) {
                Py_DECREF(tuple);
                return NULL;
            }

            PyTuple_SET_ITEM(tuple, i, item_key);
        }

        key = PyTuple_Pack(2, tuple, op);
        Py_DECREF(tuple);
    }
    else if (PyFrozenSet_CheckExact(op)) {
        Py_ssize_t pos = 0;
        PyObject *item;
        Py_hash_t hash;
        Py_ssize_t i, len;
        PyObject *tuple, *set;

        len = PySet_GET_SIZE(op);
        tuple = PyTuple_New(len);
        if (tuple == NULL)
            return NULL;

        i = 0;
        while (_PySet_NextEntry(op, &pos, &item, &hash)) {
            PyObject *item_key;

            item_key = _PyCode_ConstantKey(item);
            if (item_key == NULL) {
                Py_DECREF(tuple);
                return NULL;
            }

            assert(i < len);
            PyTuple_SET_ITEM(tuple, i, item_key);
            i++;
        }
        set = PyFrozenSet_New(tuple);
        Py_DECREF(tuple);
        if (set == NULL)
            return NULL;

        key = PyTuple_Pack(2, set, op);
        Py_DECREF(set);
        return key;
    }
    else {
        /* for other types, use the object identifier as a unique identifier
         * to ensure that they are seen as unequal. */
        PyObject *obj_id = PyLong_FromVoidPtr(op);
        if (obj_id == NULL)
            return NULL;

        key = PyTuple_Pack(2, obj_id, op);
        Py_DECREF(obj_id);
    }
    return key;
}

static PyObject *
code_richcompare(PyObject *self, PyObject *other, int op)
{
    PyCodeObject *co, *cp;
    int eq;
    PyObject *consts1, *consts2;
    PyObject *res;

    if ((op != Py_EQ && op != Py_NE) ||
        !PyCode_Check(self) ||
        !PyCode_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    co = (PyCodeObject *)self;
    cp = (PyCodeObject *)other;

    eq = PyObject_RichCompareBool(co->co_name, cp->co_name, Py_EQ);
    if (!eq) goto unequal;
    eq = co->co_argcount == cp->co_argcount;
    if (!eq) goto unequal;
    eq = co->co_posonlyargcount == cp->co_posonlyargcount;
    if (!eq) goto unequal;
    eq = co->co_kwonlyargcount == cp->co_kwonlyargcount;
    if (!eq) goto unequal;
    eq = co->co_nlocals == cp->co_nlocals;
    if (!eq) goto unequal;
    eq = co->co_flags == cp->co_flags;
    if (!eq) goto unequal;
    eq = co->co_firstlineno == cp->co_firstlineno;
    if (!eq) goto unequal;
    eq = PyObject_RichCompareBool(co->co_code, cp->co_code, Py_EQ);
    if (eq <= 0) goto unequal;

    /* compare constants */
    consts1 = _PyCode_ConstantKey(co->co_consts);
    if (!consts1)
        return NULL;
    consts2 = _PyCode_ConstantKey(cp->co_consts);
    if (!consts2) {
        Py_DECREF(consts1);
        return NULL;
    }
    eq = PyObject_RichCompareBool(consts1, consts2, Py_EQ);
    Py_DECREF(consts1);
    Py_DECREF(consts2);
    if (eq <= 0) goto unequal;

    eq = PyObject_RichCompareBool(co->co_names, cp->co_names, Py_EQ);
    if (eq <= 0) goto unequal;
    eq = PyObject_RichCompareBool(co->co_varnames, cp->co_varnames, Py_EQ);
    if (eq <= 0) goto unequal;
    eq = PyObject_RichCompareBool(co->co_freevars, cp->co_freevars, Py_EQ);
    if (eq <= 0) goto unequal;
    eq = PyObject_RichCompareBool(co->co_cellvars, cp->co_cellvars, Py_EQ);
    if (eq <= 0) goto unequal;

    if (op == Py_EQ)
        res = Py_True;
    else
        res = Py_False;
    goto done;

  unequal:
    if (eq < 0)
        return NULL;
    if (op == Py_NE)
        res = Py_True;
    else
        res = Py_False;

  done:
    Py_INCREF(res);
    return res;
}

static Py_hash_t
code_hash(PyCodeObject *co)
{
    Py_hash_t h, h0, h1, h2, h3, h4, h5, h6;
    h0 = PyObject_Hash(co->co_name);
    if (h0 == -1) return -1;
    h1 = PyObject_Hash(co->co_code);
    if (h1 == -1) return -1;
    h2 = PyObject_Hash(co->co_consts);
    if (h2 == -1) return -1;
    h3 = PyObject_Hash(co->co_names);
    if (h3 == -1) return -1;
    h4 = PyObject_Hash(co->co_varnames);
    if (h4 == -1) return -1;
    h5 = PyObject_Hash(co->co_freevars);
    if (h5 == -1) return -1;
    h6 = PyObject_Hash(co->co_cellvars);
    if (h6 == -1) return -1;
    h = h0 ^ h1 ^ h2 ^ h3 ^ h4 ^ h5 ^ h6 ^
        co->co_argcount ^ co->co_posonlyargcount ^ co->co_kwonlyargcount ^
        co->co_nlocals ^ co->co_flags;
    if (h == -1) h = -2;
    return h;
}

/* XXX code objects need to participate in GC? */

static struct PyMethodDef code_methods[] = {
    {"__sizeof__", (PyCFunction)code_sizeof, METH_NOARGS},
    CODE_REPLACE_METHODDEF
    {NULL, NULL}                /* sentinel */
};

PyTypeObject PyCode_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "code",
    sizeof(PyCodeObject),
    0,
    (destructor)code_dealloc,           /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    (reprfunc)code_repr,                /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)code_hash,                /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    code_new__doc__,                    /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    code_richcompare,                   /* tp_richcompare */
    offsetof(PyCodeObject, co_weakreflist),     /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    code_methods,                       /* tp_methods */
    code_memberlist,                    /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    code_new,                           /* tp_new */
};

/* Use co_lnotab to compute the line number from a bytecode index, addrq.  See
   lnotab_notes.txt for the details of the lnotab representation.
*/

int
PyCode_Addr2Line(PyCodeObject *co, int addrq)
{
    Py_ssize_t size = PyBytes_Size(co->co_lnotab) / 2;
    unsigned char *p = (unsigned char*)PyBytes_AsString(co->co_lnotab);
    int line = co->co_firstlineno;
    int addr = 0;
    while (--size >= 0) {
        addr += *p++;
        if (addr > addrq)
            break;
        line += (signed char)*p;
        p++;
    }
    return line;
}

/* Update *bounds to describe the first and one-past-the-last instructions in
   the same line as lasti.  Return the number of that line. */
int
_PyCode_CheckLineNumber(PyCodeObject* co, int lasti, PyAddrPair *bounds)
{
    Py_ssize_t size;
    int addr, line;
    unsigned char* p;

    p = (unsigned char*)PyBytes_AS_STRING(co->co_lnotab);
    size = PyBytes_GET_SIZE(co->co_lnotab) / 2;

    addr = 0;
    line = co->co_firstlineno;
    assert(line > 0);

    /* possible optimization: if f->f_lasti == instr_ub
       (likely to be a common case) then we already know
       instr_lb -- if we stored the matching value of p
       somewhere we could skip the first while loop. */

    /* See lnotab_notes.txt for the description of
       co_lnotab.  A point to remember: increments to p
       come in (addr, line) pairs. */

    bounds->ap_lower = 0;
    while (size > 0) {
        if (addr + *p > lasti)
            break;
        addr += *p++;
        if ((signed char)*p)
            bounds->ap_lower = addr;
        line += (signed char)*p;
        p++;
        --size;
    }

    if (size > 0) {
        while (--size >= 0) {
            addr += *p++;
            if ((signed char)*p)
                break;
            p++;
        }
        bounds->ap_upper = addr;
    }
    else {
        bounds->ap_upper = INT_MAX;
    }

    return line;
}


int
_PyCode_GetExtra(PyObject *code, Py_ssize_t index, void **extra)
{
    if (!PyCode_Check(code)) {
        PyErr_BadInternalCall();
        return -1;
    }

    PyCodeObject *o = (PyCodeObject*) code;
    _PyCodeObjectExtra *co_extra = (_PyCodeObjectExtra*) o->co_extra;

    if (co_extra == NULL || co_extra->ce_size <= index) {
        *extra = NULL;
        return 0;
    }

    *extra = co_extra->ce_extras[index];
    return 0;
}


int
_PyCode_SetExtra(PyObject *code, Py_ssize_t index, void *extra)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();

    if (!PyCode_Check(code) || index < 0 ||
            index >= interp->co_extra_user_count) {
        PyErr_BadInternalCall();
        return -1;
    }

    PyCodeObject *o = (PyCodeObject*) code;
    _PyCodeObjectExtra *co_extra = (_PyCodeObjectExtra *) o->co_extra;

    if (co_extra == NULL || co_extra->ce_size <= index) {
        Py_ssize_t i = (co_extra == NULL ? 0 : co_extra->ce_size);
        co_extra = PyMem_Realloc(
                co_extra,
                sizeof(_PyCodeObjectExtra) +
                (interp->co_extra_user_count-1) * sizeof(void*));
        if (co_extra == NULL) {
            return -1;
        }
        for (; i < interp->co_extra_user_count; i++) {
            co_extra->ce_extras[i] = NULL;
        }
        co_extra->ce_size = interp->co_extra_user_count;
        o->co_extra = co_extra;
    }

    if (co_extra->ce_extras[index] != NULL) {
        freefunc free = interp->co_extra_freefuncs[index];
        if (free != NULL) {
            free(co_extra->ce_extras[index]);
        }
    }

    co_extra->ce_extras[index] = extra;
    return 0;
}
