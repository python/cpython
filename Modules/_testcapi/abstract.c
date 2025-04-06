#define PY_SSIZE_T_CLEAN
#include "parts.h"
#include "util.h"


static PyObject *
object_repr(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyObject_Repr(arg);
}

static PyObject *
object_ascii(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyObject_ASCII(arg);
}

static PyObject *
object_str(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyObject_Str(arg);
}

static PyObject *
object_bytes(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyObject_Bytes(arg);
}

static PyObject *
object_getattr(PyObject *self, PyObject *args)
{
    PyObject *obj, *attr_name;
    if (!PyArg_ParseTuple(args, "OO", &obj, &attr_name)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(attr_name);
    return PyObject_GetAttr(obj, attr_name);
}

static PyObject *
object_getattrstring(PyObject *self, PyObject *args)
{
    PyObject *obj;
    const char *attr_name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#", &obj, &attr_name, &size)) {
        return NULL;
    }
    NULLABLE(obj);
    return PyObject_GetAttrString(obj, attr_name);
}

static PyObject *
object_hasattr(PyObject *self, PyObject *args)
{
    PyObject *obj, *attr_name;
    if (!PyArg_ParseTuple(args, "OO", &obj, &attr_name)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(attr_name);
    return PyLong_FromLong(PyObject_HasAttr(obj, attr_name));
}

static PyObject *
object_hasattrstring(PyObject *self, PyObject *args)
{
    PyObject *obj;
    const char *attr_name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#", &obj, &attr_name, &size)) {
        return NULL;
    }
    NULLABLE(obj);
    return PyLong_FromLong(PyObject_HasAttrString(obj, attr_name));
}

static PyObject *
object_setattr(PyObject *self, PyObject *args)
{
    PyObject *obj, *attr_name, *value;
    if (!PyArg_ParseTuple(args, "OOO", &obj, &attr_name, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(attr_name);
    NULLABLE(value);
    RETURN_INT(PyObject_SetAttr(obj, attr_name, value));
}

static PyObject *
object_setattrstring(PyObject *self, PyObject *args)
{
    PyObject *obj, *value;
    const char *attr_name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#O", &obj, &attr_name, &size, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    RETURN_INT(PyObject_SetAttrString(obj, attr_name, value));
}

static PyObject *
object_delattr(PyObject *self, PyObject *args)
{
    PyObject *obj, *attr_name;
if (!PyArg_ParseTuple(args, "OO", &obj, &attr_name)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(attr_name);
    RETURN_INT(PyObject_DelAttr(obj, attr_name));
}

static PyObject *
object_delattrstring(PyObject *self, PyObject *args)
{
    PyObject *obj;
    const char *attr_name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#", &obj, &attr_name, &size)) {
        return NULL;
    }
    NULLABLE(obj);
    RETURN_INT(PyObject_DelAttrString(obj, attr_name));
}


static PyObject *
mapping_check(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyMapping_Check(obj));
}

static PyObject *
mapping_size(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyMapping_Size(obj));
}

static PyObject *
mapping_length(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyMapping_Length(obj));
}

static PyObject *
object_getitem(PyObject *self, PyObject *args)
{
    PyObject *mapping, *key;
    if (!PyArg_ParseTuple(args, "OO", &mapping, &key)) {
        return NULL;
    }
    NULLABLE(mapping);
    NULLABLE(key);
    return PyObject_GetItem(mapping, key);
}

static PyObject *
mapping_getitemstring(PyObject *self, PyObject *args)
{
    PyObject *mapping;
    const char *key;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#", &mapping, &key, &size)) {
        return NULL;
    }
    NULLABLE(mapping);
    return PyMapping_GetItemString(mapping, key);
}

static PyObject *
mapping_haskey(PyObject *self, PyObject *args)
{
    PyObject *mapping, *key;
    if (!PyArg_ParseTuple(args, "OO", &mapping, &key)) {
        return NULL;
    }
    NULLABLE(mapping);
    NULLABLE(key);
    return PyLong_FromLong(PyMapping_HasKey(mapping, key));
}

static PyObject *
mapping_haskeystring(PyObject *self, PyObject *args)
{
    PyObject *mapping;
    const char *key;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#", &mapping, &key, &size)) {
        return NULL;
    }
    NULLABLE(mapping);
    return PyLong_FromLong(PyMapping_HasKeyString(mapping, key));
}

static PyObject *
object_setitem(PyObject *self, PyObject *args)
{
    PyObject *mapping, *key, *value;
    if (!PyArg_ParseTuple(args, "OOO", &mapping, &key, &value)) {
        return NULL;
    }
    NULLABLE(mapping);
    NULLABLE(key);
    NULLABLE(value);
    RETURN_INT(PyObject_SetItem(mapping, key, value));
}

static PyObject *
mapping_setitemstring(PyObject *self, PyObject *args)
{
    PyObject *mapping, *value;
    const char *key;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#O", &mapping, &key, &size, &value)) {
        return NULL;
    }
    NULLABLE(mapping);
    NULLABLE(value);
    RETURN_INT(PyMapping_SetItemString(mapping, key, value));
}

static PyObject *
object_delitem(PyObject *self, PyObject *args)
{
    PyObject *mapping, *key;
    if (!PyArg_ParseTuple(args, "OO", &mapping, &key)) {
        return NULL;
    }
    NULLABLE(mapping);
    NULLABLE(key);
    RETURN_INT(PyObject_DelItem(mapping, key));
}

static PyObject *
mapping_delitem(PyObject *self, PyObject *args)
{
    PyObject *mapping, *key;
    if (!PyArg_ParseTuple(args, "OO", &mapping, &key)) {
        return NULL;
    }
    NULLABLE(mapping);
    NULLABLE(key);
    RETURN_INT(PyMapping_DelItem(mapping, key));
}

static PyObject *
mapping_delitemstring(PyObject *self, PyObject *args)
{
    PyObject *mapping;
    const char *key;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#", &mapping, &key, &size)) {
        return NULL;
    }
    NULLABLE(mapping);
    RETURN_INT(PyMapping_DelItemString(mapping, key));
}

static PyObject *
mapping_keys(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    return PyMapping_Keys(obj);
}

static PyObject *
mapping_values(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    return PyMapping_Values(obj);
}

static PyObject *
mapping_items(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    return PyMapping_Items(obj);
}


static PyObject *
sequence_check(PyObject* self, PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PySequence_Check(obj));
}

static PyObject *
sequence_size(PyObject* self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PySequence_Size(obj));
}

static PyObject *
sequence_length(PyObject* self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PySequence_Length(obj));
}

static PyObject *
sequence_concat(PyObject *self, PyObject *args)
{
    PyObject *seq1, *seq2;
    if (!PyArg_ParseTuple(args, "OO", &seq1, &seq2)) {
        return NULL;
    }
    NULLABLE(seq1);
    NULLABLE(seq2);

    return PySequence_Concat(seq1, seq2);
}

static PyObject *
sequence_repeat(PyObject *self, PyObject *args)
{
    PyObject *seq;
    Py_ssize_t count;
    if (!PyArg_ParseTuple(args, "On", &seq, &count)) {
        return NULL;
    }
    NULLABLE(seq);

    return PySequence_Repeat(seq, count);
}

static PyObject *
sequence_inplaceconcat(PyObject *self, PyObject *args)
{
    PyObject *seq1, *seq2;
    if (!PyArg_ParseTuple(args, "OO", &seq1, &seq2)) {
        return NULL;
    }
    NULLABLE(seq1);
    NULLABLE(seq2);

    return PySequence_InPlaceConcat(seq1, seq2);
}

static PyObject *
sequence_inplacerepeat(PyObject *self, PyObject *args)
{
    PyObject *seq;
    Py_ssize_t count;
    if (!PyArg_ParseTuple(args, "On", &seq, &count)) {
        return NULL;
    }
    NULLABLE(seq);

    return PySequence_InPlaceRepeat(seq, count);
}

static PyObject *
sequence_getitem(PyObject *self, PyObject *args)
{
    PyObject *seq;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &seq, &i)) {
        return NULL;
    }
    NULLABLE(seq);

    return PySequence_GetItem(seq, i);
}

static PyObject *
sequence_setitem(PyObject *self, PyObject *args)
{
    Py_ssize_t i;
    PyObject *seq, *val;
    if (!PyArg_ParseTuple(args, "OnO", &seq, &i, &val)) {
        return NULL;
    }
    NULLABLE(seq);
    NULLABLE(val);

    RETURN_INT(PySequence_SetItem(seq, i, val));
}


static PyObject *
sequence_delitem(PyObject *self, PyObject *args)
{
    Py_ssize_t i;
    PyObject *seq;
    if (!PyArg_ParseTuple(args, "On", &seq, &i)) {
        return NULL;
    }
    NULLABLE(seq);

    RETURN_INT(PySequence_DelItem(seq, i));
}

static PyObject *
sequence_setslice(PyObject* self, PyObject *args)
{
    PyObject *sequence, *obj;
    Py_ssize_t i1, i2;
    if (!PyArg_ParseTuple(args, "OnnO", &sequence, &i1, &i2, &obj)) {
        return NULL;
    }
    NULLABLE(sequence);
    NULLABLE(obj);

    RETURN_INT(PySequence_SetSlice(sequence, i1, i2, obj));
}

static PyObject *
sequence_delslice(PyObject *self, PyObject *args)
{
    PyObject *sequence;
    Py_ssize_t i1, i2;
    if (!PyArg_ParseTuple(args, "Onn", &sequence, &i1, &i2)) {
        return NULL;
    }
    NULLABLE(sequence);

    RETURN_INT(PySequence_DelSlice(sequence, i1, i2));
}

static PyObject *
sequence_count(PyObject *self, PyObject *args)
{
    PyObject *seq, *value;
    if (!PyArg_ParseTuple(args, "OO", &seq, &value)) {
        return NULL;
    }
    NULLABLE(seq);
    NULLABLE(value);

    RETURN_SIZE(PySequence_Count(seq, value));
}

static PyObject *
sequence_contains(PyObject *self, PyObject *args)
{
    PyObject *seq, *value;
    if (!PyArg_ParseTuple(args, "OO", &seq, &value)) {
        return NULL;
    }
    NULLABLE(seq);
    NULLABLE(value);

    RETURN_INT(PySequence_Contains(seq, value));
}

static PyObject *
sequence_index(PyObject *self, PyObject *args)
{
    PyObject *seq, *value;
    if (!PyArg_ParseTuple(args, "OO", &seq, &value)) {
        return NULL;
    }
    NULLABLE(seq);
    NULLABLE(value);

    RETURN_SIZE(PySequence_Index(seq, value));
}

static PyObject *
sequence_list(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    return PySequence_List(obj);
}

static PyObject *
sequence_tuple(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    return PySequence_Tuple(obj);
}


static PyMethodDef test_methods[] = {
    {"object_repr", object_repr, METH_O},
    {"object_ascii", object_ascii, METH_O},
    {"object_str", object_str, METH_O},
    {"object_bytes", object_bytes, METH_O},

    {"object_getattr", object_getattr, METH_VARARGS},
    {"object_getattrstring", object_getattrstring, METH_VARARGS},
    {"object_hasattr", object_hasattr, METH_VARARGS},
    {"object_hasattrstring", object_hasattrstring, METH_VARARGS},
    {"object_setattr", object_setattr, METH_VARARGS},
    {"object_setattrstring", object_setattrstring, METH_VARARGS},
    {"object_delattr", object_delattr, METH_VARARGS},
    {"object_delattrstring", object_delattrstring, METH_VARARGS},

    {"mapping_check", mapping_check, METH_O},
    {"mapping_size", mapping_size, METH_O},
    {"mapping_length", mapping_length, METH_O},
    {"object_getitem", object_getitem, METH_VARARGS},
    {"mapping_getitemstring", mapping_getitemstring, METH_VARARGS},
    {"mapping_haskey", mapping_haskey, METH_VARARGS},
    {"mapping_haskeystring", mapping_haskeystring, METH_VARARGS},
    {"object_setitem", object_setitem, METH_VARARGS},
    {"mapping_setitemstring", mapping_setitemstring, METH_VARARGS},
    {"object_delitem", object_delitem, METH_VARARGS},
    {"mapping_delitem", mapping_delitem, METH_VARARGS},
    {"mapping_delitemstring", mapping_delitemstring, METH_VARARGS},
    {"mapping_keys", mapping_keys, METH_O},
    {"mapping_values", mapping_values, METH_O},
    {"mapping_items", mapping_items, METH_O},

    {"sequence_check", sequence_check, METH_O},
    {"sequence_size", sequence_size, METH_O},
    {"sequence_length", sequence_length, METH_O},
    {"sequence_concat", sequence_concat, METH_VARARGS},
    {"sequence_repeat", sequence_repeat, METH_VARARGS},
    {"sequence_inplaceconcat", sequence_inplaceconcat, METH_VARARGS},
    {"sequence_inplacerepeat", sequence_inplacerepeat, METH_VARARGS},
    {"sequence_getitem", sequence_getitem, METH_VARARGS},
    {"sequence_setitem", sequence_setitem, METH_VARARGS},
    {"sequence_delitem", sequence_delitem, METH_VARARGS},
    {"sequence_setslice", sequence_setslice, METH_VARARGS},
    {"sequence_delslice", sequence_delslice, METH_VARARGS},
    {"sequence_count", sequence_count, METH_VARARGS},
    {"sequence_contains", sequence_contains, METH_VARARGS},
    {"sequence_index", sequence_index, METH_VARARGS},
    {"sequence_list", sequence_list, METH_O},
    {"sequence_tuple", sequence_tuple, METH_O},

    {NULL},
};

int
_PyTestCapi_Init_Abstract(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
