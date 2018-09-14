#include <Python.h>
#include <inttypes.h>
#include <stdbool.h>
#include "longintrepr.h"
#include "math.h"

static PyObject *
_serializer_serialize_long(PyObject *self, PyObject *args)
{
    PyObject *digit, *digits, *comma, *joined_digits, *c_repr;
    PyLongObject* longobj;
    char buf[100];
    if (!PyArg_ParseTuple(args, "O!", (PyObject*)&PyLong_Type, &longobj))
        return NULL;
    Py_ssize_t i, n_digits = Py_ABS(Py_SIZE(longobj));

    digits = PyList_New(n_digits);
    for(i = 0; i < n_digits; i++) {
#if PYLONG_BITS_IN_DIGIT == 30
        sprintf(buf, "%" PRIu32, (longobj->ob_digit[i]));
#else
        sprintf(buf, "%" PRIu16, (longobj->ob_digit[i]));
#endif
        digit = PyUnicode_FromString(buf);
        if(PyList_SetItem(digits, i, digit) == -1) {
            Py_DECREF(digit);
            return NULL;
        }
    }

    comma = PyUnicode_FromString(", ");
    joined_digits = PyUnicode_Join(comma, digits);
    Py_DECREF(comma);
    Py_DECREF(digits);

    c_repr = PyUnicode_FromFormat(
        "{PyVarObject_HEAD_INIT_RC(%%d, &PyLong_Type, %zd) {%U} }",
        Py_SIZE(longobj), joined_digits
    );
    Py_DECREF(joined_digits);

    digit = PyLong_FromSsize_t(n_digits);
    digits = PyTuple_Pack(2, digit, c_repr);
    Py_DECREF(digit);
    Py_DECREF(c_repr);

    return digits;
}

static PyObject *
_serializer_serialize_float(PyObject *self, PyObject *args)
{
    char buf[100];
    PyFloatObject* floatobj;
    if (!PyArg_ParseTuple(args, "O!", &PyFloat_Type, (PyObject*)&floatobj))
        return NULL;

    if (isnan(floatobj->ob_fval)) {
        return PyUnicode_FromString(
            "{PyObject_HEAD_INIT_RC(%d, &PyFloat_Type) NAN}");
    }

    if (isinf(floatobj->ob_fval)) {
        if (signbit(floatobj->ob_fval)) {
            return PyUnicode_FromString(
                "{PyObject_HEAD_INIT_RC(%d, &PyFloat_Type) -INFINITY}");
        } else {
            return PyUnicode_FromString(
                "{PyObject_HEAD_INIT_RC(%d, &PyFloat_Type) INFINITY}");
        }
    }
    sprintf(buf, "%f", floatobj->ob_fval);
    return PyUnicode_FromFormat(
        "{PyObject_HEAD_INIT_RC(%%d, &PyFloat_Type) %s}", buf);
}

static PyObject *
_serializer_serialize_complex(PyObject *self, PyObject *args)
{
    PyComplexObject *complexobj;
    if (!PyArg_ParseTuple(args, "O!", &PyComplex_Type, (PyObject*)&complexobj))
        return NULL;
    char buf[200];
    sprintf(buf, "{PyObject_HEAD_INIT_RC(%%d, &PyComplex_Type) {%f, %f}}",
            complexobj->cval.real, complexobj->cval.imag);
    return PyUnicode_FromString(buf);
}

static PyObject *
_serializer_serialize_bytes(PyObject *self, PyObject *args)
{
    PyBytesObject *bytesobj;
    PyObject *chr, *sep, *c_repr, *chrs;
    Py_ssize_t i, sz;
    if (!PyArg_ParseTuple(args, "O!", &PyBytes_Type, (PyObject*)&bytesobj))
        return NULL;

    sz = PyBytes_Size((PyObject *)bytesobj);
    chrs = PyList_New(sz + 1);
    for(i = 0; i <= sz; i++) {
        chr = PyUnicode_FromFormat("0x%x", 0xFF & bytesobj->ob_sval[i]);
        if(PyList_SetItem(chrs, i, chr) == -1) {
            Py_DECREF(chr);
            return NULL;
        }
    }

    sep = PyUnicode_FromString(", ");
    chr = PyUnicode_Join(sep, chrs);
    Py_DECREF(sep);
    Py_DECREF(chrs);

    c_repr = PyUnicode_FromFormat(
        "{PyVarObject_HEAD_INIT_RC(%%d, &PyBytes_Type, %zd) -1, {%U}}",
        sz, chr);
    Py_DECREF(chr);

    chr = PyLong_FromSsize_t(sz + 1);
    chrs = PyTuple_Pack(2, chr, c_repr);
    Py_DECREF(chr);
    Py_DECREF(c_repr);
    return chrs;
}

static PyObject *
_format_setitem_list(PyObject *list)
{
    PyObject *comma_str = PyUnicode_FromFormat(", "),
             *items = PyUnicode_Join(comma_str, list),
             *formatted_items;
    items = PyUnicode_Join(comma_str, list);
    Py_DECREF(comma_str);
    if (items == NULL)
        return NULL;
    formatted_items = PyUnicode_FromFormat(
        "{%U}", items
    );
    Py_DECREF(items);
    return formatted_items;
}

static PyObject *
_serialize_setentry(PyObject *arg, Py_ssize_t idx,
                    const char *sym_name, PyObject *null_str,
                    PyObject *callback, PyObject *static_hash_fn,
                    PyObject *to_hash)
{
    Py_hash_t item_hash;
    PyObject *item, *formatted_item, *v, *static_hash;
    if(arg == NULL) {
        formatted_item = null_str;
        Py_INCREF(null_str);
    }
    else {
        static_hash = PyObject_CallFunctionObjArgs(static_hash_fn,
                                                   arg, NULL);
        if (static_hash == NULL)
            return NULL;

        item_hash = PyLong_AsSsize_t(static_hash);
        if (item_hash == -1 && PyErr_Occurred()) {
            Py_DECREF(static_hash);
            return NULL;
        }
        Py_DECREF(static_hash);

        Py_INCREF(arg);
        item = PyObject_CallFunctionObjArgs(callback, arg, NULL);
        if (item == NULL)
        {
            Py_DECREF(arg);
            return NULL;
        }
        if(item_hash != -1) {
            formatted_item = PyUnicode_FromFormat(
                "{(PyObject*) %U, %zd}",
                item, item_hash);
        }
        else {
            formatted_item = PyUnicode_FromFormat(
                "{(PyObject*) %U, -1}", item);
            if(to_hash != NULL) {
                v = PyUnicode_FromFormat(
                        "{(PyObject *)&%s.obj, %d}", sym_name, idx);
                if (PyList_Append(to_hash, v) == -1) {
                    Py_DECREF(item);
                    Py_DECREF(arg);
                    Py_DECREF(v);
                    return NULL;
                }
            }
        }
        Py_DECREF(item);
        Py_DECREF(arg);
    }
    return formatted_item;
}

static PyObject *
_serialize_frozenset_table(PySetObject *frozen_set, const char *sym_name,
                           PyObject *callback, PyObject *static_hash_fn,
                           PyObject *to_hash, _Bool is_external)
{
    Py_ssize_t i = 0,
               i_max = is_external ? frozen_set->mask + 1 : PySet_MINSIZE;
    PyObject *list, *null_str, *arg, *ret, *formatted_item;
    null_str = PyUnicode_FromFormat("{NULL, 0}");
    list = PyList_New(i_max);
    for(i = 0; i < i_max; i++) {
        if (frozen_set == NULL)
            arg = NULL;
        else
            arg = is_external ?
                frozen_set->table[i].key : frozen_set->smalltable[i].key;
        formatted_item = _serialize_setentry(arg, i, sym_name, null_str,
                                             callback, static_hash_fn, to_hash);
        if (formatted_item == NULL) {
            Py_DECREF(list);
            Py_DECREF(null_str);
            return NULL;
        }
        PyList_SetItem(list, i, formatted_item);
    }
    Py_DECREF(null_str);
    ret = _format_setitem_list(list);
    Py_DECREF(list);
    return ret;
}

static PyObject *
_serializer_serialize_frozenset(PyObject *self, PyObject *args)
{
    PySetObject *frozen_set;
    PyObject *serialize_callback, *static_hash_fn,
             *itable, *ext_table, *body, *to_hash, *t, *static_hash;
    Py_hash_t fz_hash;
    const char* sym_name;
    if (!PyArg_ParseTuple(args, "sO!OO", &sym_name,
                          &PyFrozenSet_Type, (PyObject*)&frozen_set,
                          &serialize_callback, &static_hash_fn))
        return NULL;

    if (!PyCallable_Check(serialize_callback)) {
        PyErr_SetString(PyExc_TypeError,
                        "2nd parameter must be callable");
        return NULL;
    }

    if (!PyCallable_Check(static_hash_fn)) {
        PyErr_SetString(PyExc_TypeError,
                        "3rd parameter must be callable");
        return NULL;
    }

    static_hash = PyObject_CallFunctionObjArgs(static_hash_fn,
                                               frozen_set, NULL);
    if (static_hash == NULL)
        return NULL;

    fz_hash = PyLong_AsSsize_t(static_hash);
    if (fz_hash == -1 && PyErr_Occurred()) {
        Py_DECREF(static_hash);
        return NULL;
    }
    Py_DECREF(static_hash);

    to_hash = PyList_New(0);
    if (frozen_set->table == frozen_set->smalltable) {
        itable = _serialize_frozenset_table(
            frozen_set, sym_name, serialize_callback,
            static_hash_fn, to_hash, false);
        if (itable == NULL) {
            Py_DECREF(to_hash);
            return NULL;
        }
        body = PyUnicode_FromFormat(
            "{{NULL, NULL, _GC_UNTRACKED},"
            "{PyObject_HEAD_INIT_RC(%%d, &PyFrozenSet_Type)"
            " .fill=%zd, .used=%zd, .mask=%zd,"
            " .table=(setentry*)&(%s.obj.smalltable),"
            " .hash=%zd, .finger=%zd, .smalltable=%U,"
            " .weakreflist=NULL}}",
            frozen_set->fill,
            frozen_set->used,
            frozen_set->mask,
            sym_name,
            fz_hash,
            frozen_set->finger,
            itable
        );
    }
    else {
        itable = _serialize_frozenset_table(
            frozen_set, sym_name, serialize_callback,
            static_hash_fn, to_hash, false);
        if (itable == NULL) {
            Py_DECREF(to_hash);
            return NULL;
        }
        ext_table = _serialize_frozenset_table(
            frozen_set, sym_name, serialize_callback,
            static_hash_fn, to_hash, true);
        if (ext_table == NULL) {
            Py_DECREF(to_hash);
            Py_DECREF(itable);
            return NULL;
        }
        body = PyUnicode_FromFormat(
            "{{NULL, NULL, _GC_UNTRACKED},"
            "{PyObject_HEAD_INIT_RC(%%d, &PyFrozenSet_Type)"
            " .fill=%zd, .used=%zd, .mask=%zd,"
            " .table=(setentry*)&((setentry[])%U),"
            " .hash=-1, .finger=%zd, .smalltable=%U,"
            " .weakreflist=NULL}}",
            frozen_set->fill,
            frozen_set->used,
            frozen_set->mask,
            ext_table,
            frozen_set->finger,
            itable
        );
        Py_DECREF(ext_table);
    }
    Py_DECREF(itable);
    t = PyTuple_Pack(2, to_hash, body);
    Py_DECREF(body);
    Py_DECREF(to_hash);
    return t;
}

static PyObject *
_serializer_serialize_tuple(PyObject *self, PyObject *args)
{
    PyTupleObject *tupleobject;
    PyObject *callback, *tuple_repr, *items, *arg, *item, *formatted_item;
    PyObject *sep, *size, *ret_val;
    PyObject *gc_head_size;
    Py_ssize_t i, sz;
    if (!PyArg_ParseTuple(args, "O!O",
                          &PyTuple_Type, (PyObject*)&tupleobject, &callback))
        return NULL;
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "2nd parameter must be callable");
        return NULL;
    }
    sz = PyTuple_Size((PyObject*)tupleobject);
    items = PyList_New(sz);
    for(i = 0; i < sz; i++) {
        arg = PyTuple_GetItem((PyObject*)tupleobject, i);
        if (arg == NULL) {
            return NULL;
        }
        Py_INCREF(arg);
        item = PyObject_CallFunctionObjArgs(callback, arg, NULL);
        if (item == NULL) {
            Py_DECREF(arg);
            Py_DECREF(items);
            return NULL;
        }
        if (arg == Py_None || arg == Py_True || arg == Py_False) {
            formatted_item = item;
            Py_INCREF(item);
        }
        else {
            formatted_item = PyUnicode_FromFormat(
                "(PyObject*) %U", item);
        }
        Py_DECREF(arg);
        Py_DECREF(item);
        if(PyList_SetItem(items, i, formatted_item) == -1) {
            Py_DECREF(formatted_item);
            return NULL;
        }
    }
    sep = PyUnicode_FromString(", ");
    item = PyUnicode_Join(sep, items);
    Py_DECREF(sep);
    Py_DECREF(items);

    if (PyUnicode_GetLength(item) == 0) {
        tuple_repr = PyUnicode_FromFormat(
            "{{NULL, NULL, _GC_UNTRACKED},"
            "{PyVarObject_HEAD_INIT_RC(%%d, &PyTuple_Type, %zd)}}",
            sz);
    }
    else {
        tuple_repr = PyUnicode_FromFormat(
            "{{NULL, NULL, _GC_UNTRACKED},"
            "PyVarObject_HEAD_INIT_RC(%%d, &PyTuple_Type, %zd) {%U}}",
            sz, item);
    }
    Py_DECREF(item);
    gc_head_size = PyLong_FromSize_t(sizeof(PyGC_Head));
    size = PyLong_FromSsize_t(sz);
    ret_val = PyTuple_Pack(3, size, gc_head_size, tuple_repr);
    Py_DECREF(size);
    Py_DECREF(gc_head_size);
    Py_DECREF(tuple_repr);
    return ret_val;
}

static PyObject *
_serialize_unicode(PyObject *op, Py_ssize_t size) {
    PyObject *char_str, *char_list, *sep, *joined_chars, *body,
             *extra_char_type_str, *extra_chars_obj, *obj_type_str,
             *packed_values;
    char *kind_chr, *extra_char_type, *obj_type = "PyCompactUnicodeObject",
        *body_template =
            "{{{PyObject_HEAD_INIT_RC(%%d, &PyUnicode_Type) %zd, -1, "
            "{SSTATE_NOT_INTERNED, %s, 1, %d, 1}, NULL }"
            ", 0, NULL, 0}, {%U}}";
    int is_ascii = 0;
    Py_ssize_t i;

    char_list = PyList_New(size + 1);
    enum PyUnicode_Kind kind = PyUnicode_KIND(op);
    if (PyUnicode_IS_COMPACT_ASCII(op) || kind == PyUnicode_1BYTE_KIND) {
        Py_UCS1 *ch = (Py_UCS1*)PyUnicode_DATA(op);
        for(i = 0; i <= size; i++) {
            char_str = PyUnicode_FromFormat("0x%x",
                ch[i] & 0xFF);
            if(PyList_SetItem(char_list, i, char_str) == -1) {
                Py_DECREF(char_list);
                Py_DECREF(char_str);
                return NULL;
            }
        }
        if (PyUnicode_IS_COMPACT_ASCII(op)) {
            body_template =
                "{{PyObject_HEAD_INIT_RC(%%d, &PyUnicode_Type) %zd, -1, "
                "{SSTATE_NOT_INTERNED, %s, 1, %d, 1}, NULL }, {%U}}";
            obj_type = "PyASCIIObject";
            is_ascii = 1;
        }
        kind_chr = "PyUnicode_1BYTE_KIND";
        extra_char_type = "uint8_t";
    }
    else if (kind == PyUnicode_2BYTE_KIND) {
        Py_UCS2 *ch = (Py_UCS2*)PyUnicode_DATA(op);
        for(i = 0; i <= size; i++) {
            char_str = PyUnicode_FromFormat("0x%x",
                ch[i] & 0xFFFF);
            if(PyList_SetItem(char_list, i, char_str) == -1) {
                Py_DECREF(char_list);
                Py_DECREF(char_str);
                return NULL;
            }
        }
        kind_chr = "PyUnicode_2BYTE_KIND";
        extra_char_type = "uint16_t";
    }
    else if (kind == PyUnicode_4BYTE_KIND) {
        Py_UCS4 *ch = (Py_UCS4*)PyUnicode_DATA(op);
        for(i = 0; i <= size; i++) {
            char_str = PyUnicode_FromFormat("0x%x",
                ch[i]);
            if(PyList_SetItem(char_list, i, char_str) == -1) {
                Py_DECREF(char_list);
                Py_DECREF(char_str);
                return NULL;
            }
        }
        kind_chr = "PyUnicode_4BYTE_KIND";
        extra_char_type = "uint32_t";
    }
    else if (kind == PyUnicode_WCHAR_KIND) {
        PyErr_SetString(PyExc_TypeError,
                        "Cannot handle PyUnicode_WCHAR_KIND, yet");
        return NULL;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "Unsupported unicode string type");
        return NULL;
    }
    sep = PyUnicode_FromString(", ");
    joined_chars = PyUnicode_Join(sep, char_list);
    Py_DECREF(sep);
    Py_DECREF(char_list);
    body = PyUnicode_FromFormat(
        body_template,
        size, kind_chr, is_ascii, joined_chars);
    Py_DECREF(joined_chars);
    obj_type_str = PyUnicode_FromString(obj_type);
    extra_char_type_str = PyUnicode_FromString(extra_char_type);
    extra_chars_obj = PyLong_FromSsize_t(size + 1);
    packed_values = PyTuple_Pack(
        4, obj_type_str, extra_char_type_str, extra_chars_obj, body);
    Py_DECREF(obj_type_str);
    Py_DECREF(extra_char_type_str);
    Py_DECREF(extra_chars_obj);
    Py_DECREF(body);
    return packed_values;
}

static PyObject *
_serializer_serialize_unicode(PyObject *self, PyObject *args)
{
    PyObject *op;
    Py_ssize_t sz;
    if (!PyArg_ParseTuple(args, "O!", &PyUnicode_Type, &op))
        return NULL;
    sz = PyUnicode_GetLength(op);
    if(sz == 0) {
        PyObject *obj_type = PyUnicode_FromString("PyASCIIObject");
        PyObject *extra_chars = PyLong_FromSsize_t(sz + 1);
        PyObject *extra_char_type = PyUnicode_FromString("uint8_t");
        PyObject *body = PyUnicode_FromString(
            "{{PyObject_HEAD_INIT_RC(%d, &PyUnicode_Type) 0, -1, "
            "{SSTATE_NOT_INTERNED, PyUnicode_1BYTE_KIND, 1, 1, 1}, NULL }, "
            "{0}}");
        PyObject *tp = PyTuple_Pack(
            4, obj_type, extra_char_type, extra_chars, body);
        Py_DECREF(extra_char_type);
        Py_DECREF(extra_chars);
        Py_DECREF(obj_type);
        Py_DECREF(body);
        return tp;
    }
    return _serialize_unicode(op, sz);
}

static PyObject *
_serializer_compile_code(PyObject *self, PyObject *args)
{
    PyObject  *text, *name;
    if (!PyArg_ParseTuple(args, "O!O!",
                          &PyUnicode_Type, (PyObject*)&name,
                          &PyUnicode_Type, (PyObject*)&text))
        return NULL;
    return Py_CompileStringObject(
        PyUnicode_AsUTF8(text), name, Py_file_input, NULL, 0);
}

static PyObject *
_serializer_serialize_code(PyObject *self, PyObject *args)
{
    PyCodeObject *code;
    PyObject *callback;
    PyObject *co_code = NULL, *co_consts = NULL, *co_names = NULL,
             *co_varnames = NULL, *co_freevars = NULL, *co_cellvars = NULL,
             *co_cell2arg = NULL, *co_filename = NULL, *co_name = NULL,
             *co_lnotab = NULL, *null_str = NULL, *output_str = NULL;
    Py_ssize_t i;

    if (!PyArg_ParseTuple(args, "O!O", &PyCode_Type,
                          (PyObject*)&code, &callback))
        return NULL;

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError,
                        "2nd parameter must be callable");
        return NULL;
    }

    null_str = PyUnicode_FromString("NULL");
    if (code->co_code) {
        co_code = PyObject_CallFunctionObjArgs(callback,
                                               code->co_code,
                                               NULL);
        if (co_code == NULL) {
            goto _serialize_code_error;
        }
    } else {
        co_code = null_str;
        Py_INCREF(null_str);
    }

    if (code->co_consts) {
        co_consts = PyObject_CallFunctionObjArgs(callback,
                                                 code->co_consts,
                                                 NULL);
        if (co_consts == NULL) {
            goto _serialize_code_error;
        }
    } else {
        co_consts = null_str;
        Py_INCREF(null_str);
    }

    if (code->co_names) {
        co_names = PyObject_CallFunctionObjArgs(callback,
                                                code->co_names,
                                                NULL);
        if (co_names == NULL) {
            goto _serialize_code_error;
        }
    } else {
        co_names = null_str;
        Py_INCREF(null_str);
    }

    if (code->co_varnames) {
        co_varnames = PyObject_CallFunctionObjArgs(callback,
                                                   code->co_varnames,
                                                   NULL);
        if (co_varnames == NULL) {
            goto _serialize_code_error;
        }
    } else {
        co_varnames = null_str;
        Py_INCREF(null_str);
    }

    if (code->co_freevars) {
        co_freevars = PyObject_CallFunctionObjArgs(callback,
                                                   code->co_freevars,
                                                   NULL);
        if (co_freevars == NULL) {
            goto _serialize_code_error;
        }
    } else {
        co_freevars = null_str;
        Py_INCREF(null_str);
    }

    if (code->co_cellvars) {
        co_cellvars = PyObject_CallFunctionObjArgs(callback,
                                                   code->co_cellvars,
                                                   NULL);
        if (co_cellvars == NULL) {
            goto _serialize_code_error;
        }
    } else {
        co_cellvars = null_str;
        Py_INCREF(null_str);
    }

    if (code->co_filename) {
        co_filename = PyObject_CallFunctionObjArgs(callback,
                                                   code->co_filename,
                                                   NULL);
        if (co_filename == NULL) {
            goto _serialize_code_error;
        }
    } else {
        co_filename = null_str;
        Py_INCREF(null_str);
    }

    if (code->co_name) {
        co_name = PyObject_CallFunctionObjArgs(callback,
                                               code->co_name,
                                               NULL);
        if (co_name == NULL) {
            goto _serialize_code_error;
        }
    } else {
        co_name = null_str;
        Py_INCREF(null_str);
    }

    if (code->co_lnotab) {
        co_lnotab = PyObject_CallFunctionObjArgs(callback,
                                                 code->co_lnotab,
                                                 NULL);
        if (co_lnotab == NULL) {
            goto _serialize_code_error;
        }
    } else {
        co_lnotab = null_str;
        Py_INCREF(null_str);
    }

    if(code->co_cell2arg != NULL) {
        PyObject *chars = PyList_New(PyTuple_GET_SIZE(code->co_cellvars));
        for (i = 0; i < PyTuple_GET_SIZE(code->co_cellvars); i++) {
            PyObject *c = PyUnicode_FromFormat(
                "\\x%x", (int)(code->co_cell2arg[i] & 0xFF));
            if(PyList_SetItem(chars, i, c) == -1) {
                Py_DECREF(chars);
                goto _serialize_code_error;
            }
        }
        PyObject *sep = PyUnicode_FromString("");
        co_cell2arg = PyUnicode_Join(sep, chars);
        Py_DECREF(sep);
        Py_DECREF(chars);

        if(co_cell2arg == NULL) {
            goto _serialize_code_error;
        }
        PyObject *tmp = co_cell2arg;
        co_cell2arg = PyUnicode_FromFormat(
            "(unsigned char *)&\"%U\"", co_cell2arg);
        Py_DECREF(tmp);
    } else {
        co_cell2arg = null_str;
        Py_INCREF(null_str);
    }

    output_str = PyUnicode_FromFormat(
        "{\n\tPyObject_HEAD_INIT_RC(%%d, &PyCode_Type)\n"
        /* co_argcount, co_kwonlyargcount, co_nlocals,
         * co_stacksize, co_flags, co_firstlineno */
        "\t%d, %d, %d, %d, %d, %d,\n"
        /* co_code, co_consts, co_names */
        "\t(PyObject*)%U, (PyObject*)%U, (PyObject*)%U,\n"
        /* co_varnames, co_freevars, co_cellvars */
        "\t(PyObject*)%U, (PyObject*)%U, (PyObject*)%U,\n"
        /* co_cell2arg, co_filename, co_name, co_lnotab */
        "\t%U, (PyObject*)%U, (PyObject*)%U, (PyObject*)%U,\n"
        "\tNULL, NULL, NULL\n}",
        code->co_argcount, code->co_kwonlyargcount,
        code->co_nlocals, code->co_stacksize,
        code->co_flags, code->co_firstlineno,
        co_code, co_consts, co_names,
        co_varnames, co_freevars, co_cellvars,
        co_cell2arg, co_filename, co_name, co_lnotab);

_serialize_code_error:
    Py_XDECREF(co_cell2arg);
    Py_XDECREF(co_lnotab);
    Py_XDECREF(co_name);
    Py_XDECREF(co_filename);
    Py_XDECREF(co_cellvars);
    Py_XDECREF(co_freevars);
    Py_XDECREF(co_varnames);
    Py_XDECREF(co_names);
    Py_XDECREF(co_consts);
    Py_XDECREF(co_code);
    Py_XDECREF(null_str);
    return output_str;
}

static PyMethodDef _SerializerMethods[] = {
    {"serialize_long", _serializer_serialize_long, METH_VARARGS,
     "Serialize a PyLongObject object into a C structure."},
    {"serialize_float", _serializer_serialize_float, METH_VARARGS,
     "Serialize a PyFloatObject object into a C structure."},
    {"serialize_bytes", _serializer_serialize_bytes, METH_VARARGS,
     "Serialize a PyBytesObject object into a C structure."},
    {"serialize_tuple", _serializer_serialize_tuple, METH_VARARGS,
     "Serialize a PyTupleObject into a C structure."},
    {"serialize_frozenset", _serializer_serialize_frozenset, METH_VARARGS,
     "Serialize a frozenset into a C structure."},
    {"serialize_unicode", _serializer_serialize_unicode, METH_VARARGS,
     "Serialize a PyUnicodeObject into a C structure."},
    {"serialize_code", _serializer_serialize_code, METH_VARARGS,
     "Serialize a code object into a C structure."},
    {"serialize_complex", _serializer_serialize_complex, METH_VARARGS,
     "Serialize a complex object into a C structure."},
    {"compile_code", _serializer_compile_code, METH_VARARGS,
     "Compile a string into a code object."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef _serializermodule = {
    PyModuleDef_HEAD_INIT,
    "_serializer",
    "A helper module to serialize Python Objects to static C structures",
    -1,
    _SerializerMethods
};

PyMODINIT_FUNC
PyInit__serializer(void)
{
    return PyModule_Create(&_serializermodule);
}
