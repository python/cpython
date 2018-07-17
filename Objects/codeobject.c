#include <stdbool.h>

#include "Python.h"
#include "code.h"
#include "structmember.h"

#define NAME_CHARS \
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz"

/* Holder for co_extra information */
typedef struct {
    Py_ssize_t ce_size;
    void **ce_extras;
} _PyCodeObjectExtra;

/* all_name_chars(s): true iff all chars in s are valid NAME_CHARS */

static int
all_name_chars(PyObject *o)
{
    static char ok_name_char[256];
    static const unsigned char *name_chars = (unsigned char *)NAME_CHARS;
    const unsigned char *s, *e;

    if (!PyUnicode_IS_ASCII(o))
        return 0;

    if (ok_name_char[*name_chars] == 0) {
        const unsigned char *p;
        for (p = name_chars; *p; p++)
            ok_name_char[*p] = 1;
    }
    s = PyUnicode_1BYTE_DATA(o);
    e = s + PyUnicode_GET_LENGTH(o);
    while (s != e) {
        if (ok_name_char[*s++] == 0)
            return 0;
    }
    return 1;
}

static void
intern_strings(PyObject *tuple)
{
    Py_ssize_t i;

    for (i = PyTuple_GET_SIZE(tuple); --i >= 0; ) {
        PyObject *v = PyTuple_GET_ITEM(tuple, i);
        if (v == NULL || !PyUnicode_CheckExact(v)) {
            Py_FatalError("non-string found in code slot");
        }
        PyUnicode_InternInPlace(&PyTuple_GET_ITEM(tuple, i));
    }
}

/* Intern selected string constants */
static int
intern_string_constants(PyObject *tuple)
{
    int modified = 0;
    Py_ssize_t i;

    for (i = PyTuple_GET_SIZE(tuple); --i >= 0; ) {
        PyObject *v = PyTuple_GET_ITEM(tuple, i);
        if (PyUnicode_CheckExact(v)) {
            if (PyUnicode_READY(v) == -1) {
                PyErr_Clear();
                continue;
            }
            if (all_name_chars(v)) {
                PyObject *w = v;
                PyUnicode_InternInPlace(&v);
                if (w != v) {
                    PyTuple_SET_ITEM(tuple, i, v);
                    modified = 1;
                }
            }
        }
        else if (PyTuple_CheckExact(v)) {
            intern_string_constants(v);
        }
        else if (PyFrozenSet_CheckExact(v)) {
            PyObject *w = v;
            PyObject *tmp = PySequence_Tuple(v);
            if (tmp == NULL) {
                PyErr_Clear();
                continue;
            }
            if (intern_string_constants(tmp)) {
                v = PyFrozenSet_New(tmp);
                if (v == NULL) {
                    PyErr_Clear();
                }
                else {
                    PyTuple_SET_ITEM(tuple, i, v);
                    Py_DECREF(w);
                    modified = 1;
                }
            }
            Py_DECREF(tmp);
        }
    }
    return modified;
}


PyCodeObject *
PyCode_New(int argcount, int kwonlyargcount,
           int nlocals, int stacksize, int flags,
           PyObject *code, PyObject *consts, PyObject *names,
           PyObject *varnames, PyObject *freevars, PyObject *cellvars,
           PyObject *filename, PyObject *name, int firstlineno,
           PyObject *lnotab)
{
    PyCodeObject *co;
    unsigned char *cell2arg = NULL;
    Py_ssize_t i, n_cellvars, n_varnames, total_args;

    /* Check argument types */
    if (argcount < 0 || kwonlyargcount < 0 || nlocals < 0 ||
        code == NULL ||
        consts == NULL || !PyTuple_Check(consts) ||
        names == NULL || !PyTuple_Check(names) ||
        varnames == NULL || !PyTuple_Check(varnames) ||
        freevars == NULL || !PyTuple_Check(freevars) ||
        cellvars == NULL || !PyTuple_Check(cellvars) ||
        name == NULL || !PyUnicode_Check(name) ||
        filename == NULL || !PyUnicode_Check(filename) ||
        lnotab == NULL || !PyBytes_Check(lnotab) ||
        !PyObject_CheckReadBuffer(code)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    /* Ensure that the filename is a ready Unicode string */
    if (PyUnicode_READY(filename) < 0)
        return NULL;

    intern_strings(names);
    intern_strings(varnames);
    intern_strings(freevars);
    intern_strings(cellvars);
    intern_string_constants(consts);

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
        Py_ssize_t alloc_size = sizeof(unsigned char) * n_cellvars;
        bool used_cell2arg = false;
        cell2arg = PyMem_MALLOC(alloc_size);
        if (cell2arg == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
        memset(cell2arg, CO_CELL_NOT_AN_ARG, alloc_size);
        /* Find cells which are also arguments. */
        for (i = 0; i < n_cellvars; i++) {
            Py_ssize_t j;
            PyObject *cell = PyTuple_GET_ITEM(cellvars, i);
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
    co = PyObject_NEW(PyCodeObject, &PyCode_Type);
    if (co == NULL) {
        if (cell2arg)
            PyMem_FREE(cell2arg);
        return NULL;
    }
    co->co_argcount = argcount;
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
    return co;
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

    result = PyCode_New(0,                      /* argcount */
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
    {"co_argcount",     T_INT,          OFF(co_argcount),       READONLY},
    {"co_kwonlyargcount",       T_INT,  OFF(co_kwonlyargcount), READONLY},
    {"co_nlocals",      T_INT,          OFF(co_nlocals),        READONLY},
    {"co_stacksize",T_INT,              OFF(co_stacksize),      READONLY},
    {"co_flags",        T_INT,          OFF(co_flags),          READONLY},
    {"co_code",         T_OBJECT,       OFF(co_code),           READONLY},
    {"co_consts",       T_OBJECT,       OFF(co_consts),         READONLY},
    {"co_names",        T_OBJECT,       OFF(co_names),          READONLY},
    {"co_varnames",     T_OBJECT,       OFF(co_varnames),       READONLY},
    {"co_freevars",     T_OBJECT,       OFF(co_freevars),       READONLY},
    {"co_cellvars",     T_OBJECT,       OFF(co_cellvars),       READONLY},
    {"co_filename",     T_OBJECT,       OFF(co_filename),       READONLY},
    {"co_name",         T_OBJECT,       OFF(co_name),           READONLY},
    {"co_firstlineno", T_INT,           OFF(co_firstlineno),    READONLY},
    {"co_lnotab",       T_OBJECT,       OFF(co_lnotab),         READONLY},
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
                item->ob_type->tp_name);
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

PyDoc_STRVAR(code_doc,
"code(argcount, kwonlyargcount, nlocals, stacksize, flags, codestring,\n\
      constants, names, varnames, filename, name, firstlineno,\n\
      lnotab[, freevars[, cellvars]])\n\
\n\
Create a code object.  Not for the faint of heart.");

static PyObject *
code_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    int argcount;
    int kwonlyargcount;
    int nlocals;
    int stacksize;
    int flags;
    PyObject *co = NULL;
    PyObject *code;
    PyObject *consts;
    PyObject *names, *ournames = NULL;
    PyObject *varnames, *ourvarnames = NULL;
    PyObject *freevars = NULL, *ourfreevars = NULL;
    PyObject *cellvars = NULL, *ourcellvars = NULL;
    PyObject *filename;
    PyObject *name;
    int firstlineno;
    PyObject *lnotab;

    if (!PyArg_ParseTuple(args, "iiiiiSO!O!O!UUiS|O!O!:code",
                          &argcount, &kwonlyargcount,
                              &nlocals, &stacksize, &flags,
                          &code,
                          &PyTuple_Type, &consts,
                          &PyTuple_Type, &names,
                          &PyTuple_Type, &varnames,
                          &filename, &name,
                          &firstlineno, &lnotab,
                          &PyTuple_Type, &freevars,
                          &PyTuple_Type, &cellvars))
        return NULL;

    if (argcount < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: argcount must not be negative");
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

    co = (PyObject *)PyCode_New(argcount, kwonlyargcount,
                                nlocals, stacksize, flags,
                                code, consts, ournames, ourvarnames,
                                ourfreevars, ourcellvars, filename,
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
    if (co->co_extra != NULL) {
        __PyCodeExtraState *state = __PyCodeExtraState_Get();
        _PyCodeObjectExtra *co_extra = co->co_extra;

        for (Py_ssize_t i = 0; i < co_extra->ce_size; i++) {
            freefunc free_extra = state->co_extra_freefuncs[i];

            if (free_extra != NULL) {
                free_extra(co_extra->ce_extras[i]);
            }
        }

        PyMem_Free(co_extra->ce_extras);
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
code_sizeof(PyCodeObject *co, void *unused)
{
    Py_ssize_t res = _PyObject_SIZE(Py_TYPE(co));
    _PyCodeObjectExtra *co_extra = (_PyCodeObjectExtra*) co->co_extra;

    if (co->co_cell2arg != NULL && co->co_cellvars != NULL)
        res += PyTuple_GET_SIZE(co->co_cellvars) * sizeof(Py_ssize_t);

    if (co_extra != NULL)
        res += co_extra->ce_size * sizeof(co_extra->ce_extras[0]);

    return PyLong_FromSsize_t(res);
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

    /* Py_None and Py_Ellipsis are singleton */
    if (op == Py_None || op == Py_Ellipsis
       || PyLong_CheckExact(op)
       || PyBool_Check(op)
       || PyBytes_CheckExact(op)
       || PyUnicode_CheckExact(op)
          /* code_richcompare() uses _PyCode_ConstantKey() internally */
       || PyCode_Check(op)) {
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
    if (eq <= 0) goto unequal;
    eq = co->co_argcount == cp->co_argcount;
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
        co->co_argcount ^ co->co_kwonlyargcount ^
        co->co_nlocals ^ co->co_flags;
    if (h == -1) h = -2;
    return h;
}

/* XXX code objects need to participate in GC? */

static struct PyMethodDef code_methods[] = {
    {"__sizeof__", (PyCFunction)code_sizeof, METH_NOARGS},
    {NULL, NULL}                /* sentinel */
};

PyTypeObject PyCode_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "code",
    sizeof(PyCodeObject),
    0,
    (destructor)code_dealloc,           /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
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
    code_doc,                           /* tp_doc */
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
    __PyCodeExtraState *state = __PyCodeExtraState_Get();

    if (!PyCode_Check(code) || index < 0 ||
            index >= state->co_extra_user_count) {
        PyErr_BadInternalCall();
        return -1;
    }

    PyCodeObject *o = (PyCodeObject*) code;
    _PyCodeObjectExtra *co_extra = (_PyCodeObjectExtra *) o->co_extra;

    if (co_extra == NULL) {
        co_extra = PyMem_Malloc(sizeof(_PyCodeObjectExtra));
        if (co_extra == NULL) {
            return -1;
        }

        co_extra->ce_extras = PyMem_Malloc(
            state->co_extra_user_count * sizeof(void*));
        if (co_extra->ce_extras == NULL) {
            PyMem_Free(co_extra);
            return -1;
        }

        co_extra->ce_size = state->co_extra_user_count;

        for (Py_ssize_t i = 0; i < co_extra->ce_size; i++) {
            co_extra->ce_extras[i] = NULL;
        }

        o->co_extra = co_extra;
    }
    else if (co_extra->ce_size <= index) {
        void** ce_extras = PyMem_Realloc(
            co_extra->ce_extras, state->co_extra_user_count * sizeof(void*));

        if (ce_extras == NULL) {
            return -1;
        }

        for (Py_ssize_t i = co_extra->ce_size;
             i < state->co_extra_user_count;
             i++) {
            ce_extras[i] = NULL;
        }

        co_extra->ce_extras = ce_extras;
        co_extra->ce_size = state->co_extra_user_count;
    }

    if (co_extra->ce_extras[index] != NULL) {
        freefunc free = state->co_extra_freefuncs[index];
        if (free != NULL) {
            free(co_extra->ce_extras[index]);
        }
    }

    co_extra->ce_extras[index] = extra;
    return 0;
}
