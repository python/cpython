#include "Python.h"
#include "code.h"
#include "structmember.h"

#define NAME_CHARS \
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz"

/* all_name_chars(s): true iff all chars in s are valid NAME_CHARS */

static int
all_name_chars(unsigned char *s)
{
    static char ok_name_char[256];
    static unsigned char *name_chars = (unsigned char *)NAME_CHARS;

    if (ok_name_char[*name_chars] == 0) {
        unsigned char *p;
        for (p = name_chars; *p; p++)
            ok_name_char[*p] = 1;
    }
    while (*s) {
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
        if (v == NULL || !PyString_CheckExact(v)) {
            Py_FatalError("non-string found in code slot");
        }
        PyString_InternInPlace(&PyTuple_GET_ITEM(tuple, i));
    }
}


PyCodeObject *
PyCode_New(int argcount, int nlocals, int stacksize, int flags,
           PyObject *code, PyObject *consts, PyObject *names,
           PyObject *varnames, PyObject *freevars, PyObject *cellvars,
           PyObject *filename, PyObject *name, int firstlineno,
           PyObject *lnotab)
{
    PyCodeObject *co;
    Py_ssize_t i;
    /* Check argument types */
    if (argcount < 0 || nlocals < 0 ||
        code == NULL ||
        consts == NULL || !PyTuple_Check(consts) ||
        names == NULL || !PyTuple_Check(names) ||
        varnames == NULL || !PyTuple_Check(varnames) ||
        freevars == NULL || !PyTuple_Check(freevars) ||
        cellvars == NULL || !PyTuple_Check(cellvars) ||
        name == NULL || !PyString_Check(name) ||
        filename == NULL || !PyString_Check(filename) ||
        lnotab == NULL || !PyString_Check(lnotab) ||
        !PyObject_CheckReadBuffer(code)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    intern_strings(names);
    intern_strings(varnames);
    intern_strings(freevars);
    intern_strings(cellvars);
    /* Intern selected string constants */
    for (i = PyTuple_Size(consts); --i >= 0; ) {
        PyObject *v = PyTuple_GetItem(consts, i);
        if (!PyString_Check(v))
            continue;
        if (!all_name_chars((unsigned char *)PyString_AS_STRING(v)))
            continue;
        PyString_InternInPlace(&PyTuple_GET_ITEM(consts, i));
    }
    co = PyObject_NEW(PyCodeObject, &PyCode_Type);
    if (co != NULL) {
        co->co_argcount = argcount;
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
        Py_INCREF(filename);
        co->co_filename = filename;
        Py_INCREF(name);
        co->co_name = name;
        co->co_firstlineno = firstlineno;
        Py_INCREF(lnotab);
        co->co_lnotab = lnotab;
        co->co_zombieframe = NULL;
        co->co_weakreflist = NULL;
    }
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
        emptystring = PyString_FromString("");
        if (emptystring == NULL)
            goto failed;
    }
    if (nulltuple == NULL) {
        nulltuple = PyTuple_New(0);
        if (nulltuple == NULL)
            goto failed;
    }
    funcname_ob = PyString_FromString(funcname);
    if (funcname_ob == NULL)
        goto failed;
    filename_ob = PyString_FromString(filename);
    if (filename_ob == NULL)
        goto failed;

    result = PyCode_New(0,                      /* argcount */
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
        if (PyString_CheckExact(item)) {
            Py_INCREF(item);
        }
        else if (!PyString_Check(item)) {
            PyErr_Format(
                PyExc_TypeError,
                "name tuples must contain only "
                "strings, not '%.500s'",
                item->ob_type->tp_name);
            Py_DECREF(newtuple);
            return NULL;
        }
        else {
            item = PyString_FromStringAndSize(
                PyString_AS_STRING(item),
                PyString_GET_SIZE(item));
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
"code(argcount, nlocals, stacksize, flags, codestring, constants, names,\n\
      varnames, filename, name, firstlineno, lnotab[, freevars[, cellvars]])\n\
\n\
Create a code object.  Not for the faint of heart.");

static PyObject *
code_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    int argcount;
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

    if (!PyArg_ParseTuple(args, "iiiiSO!O!O!SSiS|O!O!:code",
                          &argcount, &nlocals, &stacksize, &flags,
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

    co = (PyObject *)PyCode_New(argcount, nlocals, stacksize, flags,
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
    Py_XDECREF(co->co_code);
    Py_XDECREF(co->co_consts);
    Py_XDECREF(co->co_names);
    Py_XDECREF(co->co_varnames);
    Py_XDECREF(co->co_freevars);
    Py_XDECREF(co->co_cellvars);
    Py_XDECREF(co->co_filename);
    Py_XDECREF(co->co_name);
    Py_XDECREF(co->co_lnotab);
    if (co->co_zombieframe != NULL)
        PyObject_GC_Del(co->co_zombieframe);
    if (co->co_weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject*)co);
    PyObject_DEL(co);
}

static PyObject *
code_repr(PyCodeObject *co)
{
    char buf[500];
    int lineno = -1;
    char *filename = "???";
    char *name = "???";

    if (co->co_firstlineno != 0)
        lineno = co->co_firstlineno;
    if (co->co_filename && PyString_Check(co->co_filename))
        filename = PyString_AS_STRING(co->co_filename);
    if (co->co_name && PyString_Check(co->co_name))
        name = PyString_AS_STRING(co->co_name);
    PyOS_snprintf(buf, sizeof(buf),
                  "<code object %.100s at %p, file \"%.300s\", line %d>",
                  name, co, filename, lineno);
    return PyString_FromString(buf);
}

static int
code_compare(PyCodeObject *co, PyCodeObject *cp)
{
    int cmp;
    cmp = PyObject_Compare(co->co_name, cp->co_name);
    if (cmp) return cmp;
    cmp = co->co_argcount - cp->co_argcount;
    if (cmp) goto normalize;
    cmp = co->co_nlocals - cp->co_nlocals;
    if (cmp) goto normalize;
    cmp = co->co_flags - cp->co_flags;
    if (cmp) goto normalize;
    cmp = co->co_firstlineno - cp->co_firstlineno;
    if (cmp) goto normalize;
    cmp = PyObject_Compare(co->co_code, cp->co_code);
    if (cmp) return cmp;
    cmp = PyObject_Compare(co->co_consts, cp->co_consts);
    if (cmp) return cmp;
    cmp = PyObject_Compare(co->co_names, cp->co_names);
    if (cmp) return cmp;
    cmp = PyObject_Compare(co->co_varnames, cp->co_varnames);
    if (cmp) return cmp;
    cmp = PyObject_Compare(co->co_freevars, cp->co_freevars);
    if (cmp) return cmp;
    cmp = PyObject_Compare(co->co_cellvars, cp->co_cellvars);
    return cmp;

 normalize:
    if (cmp > 0)
        return 1;
    else if (cmp < 0)
        return -1;
    else
        return 0;
}

PyObject*
_PyCode_ConstantKey(PyObject *op)
{
    PyObject *key;

    /* Py_None is a singleton */
    if (op == Py_None
       || PyInt_CheckExact(op)
       || PyLong_CheckExact(op)
       || PyBool_Check(op)
       || PyBytes_CheckExact(op)
#ifdef Py_USING_UNICODE
       || PyUnicode_CheckExact(op)
#endif
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
#ifndef WITHOUT_COMPLEX
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
#endif
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

        key = PyTuple_Pack(3, Py_TYPE(op), op, tuple);
        Py_DECREF(tuple);
    }
    else if (PyFrozenSet_CheckExact(op)) {
        Py_ssize_t pos = 0;
        PyObject *item;
        long hash;
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

        key = PyTuple_Pack(3, Py_TYPE(op), op, set);
        Py_DECREF(set);
        return key;
    }
    else {
        /* for other types, use the object identifier as an unique identifier
         * to ensure that they are seen as unequal. */
        PyObject *obj_id = PyLong_FromVoidPtr(op);
        if (obj_id == NULL)
            return NULL;

        key = PyTuple_Pack(3, Py_TYPE(op), op, obj_id);
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

        /* Py3K warning if types are not equal and comparison
        isn't == or !=  */
        if (PyErr_WarnPy3k("code inequality comparisons not supported "
                           "in 3.x", 1) < 0) {
            return NULL;
        }

        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    co = (PyCodeObject *)self;
    cp = (PyCodeObject *)other;

    eq = PyObject_RichCompareBool(co->co_name, cp->co_name, Py_EQ);
    if (eq <= 0) goto unequal;
    eq = co->co_argcount == cp->co_argcount;
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

static long
code_hash(PyCodeObject *co)
{
    long h, h0, h1, h2, h3, h4, h5, h6;
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
        co->co_argcount ^ co->co_nlocals ^ co->co_flags;
    if (h == -1) h = -2;
    return h;
}

/* XXX code objects need to participate in GC? */

PyTypeObject PyCode_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "code",
    sizeof(PyCodeObject),
    0,
    (destructor)code_dealloc,           /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    (cmpfunc)code_compare,              /* tp_compare */
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
    offsetof(PyCodeObject, co_weakreflist), /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
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
    int size = PyString_Size(co->co_lnotab) / 2;
    unsigned char *p = (unsigned char*)PyString_AsString(co->co_lnotab);
    int line = co->co_firstlineno;
    int addr = 0;
    while (--size >= 0) {
        addr += *p++;
        if (addr > addrq)
            break;
        line += *p++;
    }
    return line;
}

/* Update *bounds to describe the first and one-past-the-last instructions in
   the same line as lasti.  Return the number of that line. */
int
_PyCode_CheckLineNumber(PyCodeObject* co, int lasti, PyAddrPair *bounds)
{
    int size, addr, line;
    unsigned char* p;

    p = (unsigned char*)PyString_AS_STRING(co->co_lnotab);
    size = PyString_GET_SIZE(co->co_lnotab) / 2;

    addr = 0;
    line = co->co_firstlineno;
    assert(line > 0);

    /* possible optimization: if f->f_lasti == instr_ub
       (likely to be a common case) then we already know
       instr_lb -- if we stored the matching value of p
       somwhere we could skip the first while loop. */

    /* See lnotab_notes.txt for the description of
       co_lnotab.  A point to remember: increments to p
       come in (addr, line) pairs. */

    bounds->ap_lower = 0;
    while (size > 0) {
        if (addr + *p > lasti)
            break;
        addr += *p++;
        if (*p)
            bounds->ap_lower = addr;
        line += *p++;
        --size;
    }

    if (size > 0) {
        while (--size >= 0) {
            addr += *p++;
            if (*p++)
                break;
        }
        bounds->ap_upper = addr;
    }
    else {
        bounds->ap_upper = INT_MAX;
    }

    return line;
}
