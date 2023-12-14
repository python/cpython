/* Implementation helper: a struct that looks like a tuple.
   See timemodule and posixmodule for example uses.

   The structseq helper is considered an internal CPython implementation
   detail.  Docs for modules using structseqs should call them
   "named tuples" (be sure to include a space between the two
   words and add a link back to the term in Docs/glossary.rst).
*/

#include "Python.h"
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_modsupport.h"    // _PyArg_NoPositional()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_structseq.h"     // PyStructSequence_InitType()
#include "pycore_tuple.h"         // _PyTuple_FromArray()

static const char visible_length_key[] = "n_sequence_fields";
static const char real_length_key[] = "n_fields";
static const char unnamed_fields_key[] = "n_unnamed_fields";
static const char match_args_key[] = "__match_args__";

/* Fields with this name have only a field index, not a field name.
   They are only allowed for indices < n_visible_fields. */
const char * const PyStructSequence_UnnamedField = "unnamed field";

static Py_ssize_t
get_type_attr_as_size(PyTypeObject *tp, PyObject *name)
{
    PyObject *v = PyDict_GetItemWithError(_PyType_GetDict(tp), name);
    if (v == NULL && !PyErr_Occurred()) {
        PyErr_Format(PyExc_TypeError,
                     "Missed attribute '%U' of type %s",
                     name, tp->tp_name);
        return -1;
    }
    return PyLong_AsSsize_t(v);
}

#define VISIBLE_SIZE(op) Py_SIZE(op)
#define VISIBLE_SIZE_TP(tp) \
    get_type_attr_as_size(tp, &_Py_ID(n_sequence_fields))
#define REAL_SIZE_TP(tp) \
    get_type_attr_as_size(tp, &_Py_ID(n_fields))
#define REAL_SIZE(op) REAL_SIZE_TP(Py_TYPE(op))

#define UNNAMED_FIELDS_TP(tp) \
    get_type_attr_as_size(tp, &_Py_ID(n_unnamed_fields))
#define UNNAMED_FIELDS(op) UNNAMED_FIELDS_TP(Py_TYPE(op))


PyObject *
PyStructSequence_New(PyTypeObject *type)
{
    PyStructSequence *obj;
    Py_ssize_t size = REAL_SIZE_TP(type), i;
    if (size < 0) {
        return NULL;
    }
    Py_ssize_t vsize = VISIBLE_SIZE_TP(type);
    if (vsize < 0) {
        return NULL;
    }

    obj = PyObject_GC_NewVar(PyStructSequence, type, size);
    if (obj == NULL)
        return NULL;
    /* Hack the size of the variable object, so invisible fields don't appear
     to Python code. */
    Py_SET_SIZE(obj, vsize);
    for (i = 0; i < size; i++)
        obj->ob_item[i] = NULL;

    return (PyObject*)obj;
}

void
PyStructSequence_SetItem(PyObject *op, Py_ssize_t index, PyObject *value)
{
    PyTupleObject *tuple = _PyTuple_CAST(op);
    assert(0 <= index);
#ifndef NDEBUG
    Py_ssize_t n_fields = REAL_SIZE(op);
    assert(n_fields >= 0);
    assert(index < n_fields);
#endif
    tuple->ob_item[index] = value;
}

PyObject*
PyStructSequence_GetItem(PyObject *op, Py_ssize_t index)
{
    assert(0 <= index);
#ifndef NDEBUG
    Py_ssize_t n_fields = REAL_SIZE(op);
    assert(n_fields >= 0);
    assert(index < n_fields);
#endif
    return PyTuple_GET_ITEM(op, index);
}


static int
structseq_traverse(PyStructSequence *obj, visitproc visit, void *arg)
{
    if (Py_TYPE(obj)->tp_flags & Py_TPFLAGS_HEAPTYPE) {
        Py_VISIT(Py_TYPE(obj));
    }
    Py_ssize_t i, size;
    size = REAL_SIZE(obj);
    for (i = 0; i < size; ++i) {
        Py_VISIT(obj->ob_item[i]);
    }
    return 0;
}

static void
structseq_dealloc(PyStructSequence *obj)
{
    Py_ssize_t i, size;
    PyObject_GC_UnTrack(obj);

    PyTypeObject *tp = Py_TYPE(obj);
    size = REAL_SIZE(obj);
    for (i = 0; i < size; ++i) {
        Py_XDECREF(obj->ob_item[i]);
    }
    PyObject_GC_Del(obj);
    if (_PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE)) {
        Py_DECREF(tp);
    }
}

/*[clinic input]
class structseq "PyStructSequence *" "NULL"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=9d781c6922c77752]*/

#include "clinic/structseq.c.h"

/*[clinic input]
@classmethod
structseq.__new__ as structseq_new
    sequence as arg: object
    dict: object(c_default="NULL") = {}
[clinic start generated code]*/

static PyObject *
structseq_new_impl(PyTypeObject *type, PyObject *arg, PyObject *dict)
/*[clinic end generated code: output=baa082e788b171da input=90532511101aa3fb]*/
{
    PyStructSequence *res = NULL;
    Py_ssize_t len, min_len, max_len, i, n_unnamed_fields;

    min_len = VISIBLE_SIZE_TP(type);
    if (min_len < 0) {
        return NULL;
    }
    max_len = REAL_SIZE_TP(type);
    if (max_len < 0) {
        return NULL;
    }
    n_unnamed_fields = UNNAMED_FIELDS_TP(type);
    if (n_unnamed_fields < 0) {
        return NULL;
    }

    arg = PySequence_Fast(arg, "constructor requires a sequence");

    if (!arg) {
        return NULL;
    }

    if (dict && !PyDict_Check(dict)) {
        PyErr_Format(PyExc_TypeError,
                     "%.500s() takes a dict as second arg, if any",
                     type->tp_name);
        Py_DECREF(arg);
        return NULL;
    }

    len = PySequence_Fast_GET_SIZE(arg);
    if (min_len != max_len) {
        if (len < min_len) {
            PyErr_Format(PyExc_TypeError,
                "%.500s() takes an at least %zd-sequence (%zd-sequence given)",
                type->tp_name, min_len, len);
            Py_DECREF(arg);
            return NULL;
        }

        if (len > max_len) {
            PyErr_Format(PyExc_TypeError,
                "%.500s() takes an at most %zd-sequence (%zd-sequence given)",
                type->tp_name, max_len, len);
            Py_DECREF(arg);
            return NULL;
        }
    }
    else {
        if (len != min_len) {
            PyErr_Format(PyExc_TypeError,
                         "%.500s() takes a %zd-sequence (%zd-sequence given)",
                         type->tp_name, min_len, len);
            Py_DECREF(arg);
            return NULL;
        }
    }

    res = (PyStructSequence*) PyStructSequence_New(type);
    if (res == NULL) {
        Py_DECREF(arg);
        return NULL;
    }
    for (i = 0; i < len; ++i) {
        PyObject *v = PySequence_Fast_GET_ITEM(arg, i);
        res->ob_item[i] = Py_NewRef(v);
    }
    Py_DECREF(arg);
    if (dict != NULL && PyDict_GET_SIZE(dict) > 0) {
        Py_ssize_t n_found_keys = 0;
        for (i = len; i < max_len; ++i) {
            PyObject *ob = NULL;
            const char *name = type->tp_members[i - n_unnamed_fields].name;
            if (PyDict_GetItemStringRef(dict, name, &ob) < 0) {
                Py_DECREF(res);
                return NULL;
            }
            if (ob == NULL) {
                ob = Py_NewRef(Py_None);
            }
            else {
                ++n_found_keys;
            }
            res->ob_item[i] = ob;
        }
        if (PyDict_GET_SIZE(dict) > n_found_keys) {
            PyErr_Format(PyExc_TypeError,
                         "%.500s() got duplicate or unexpected field name(s)",
                         type->tp_name);
            Py_DECREF(res);
            return NULL;
        }
    } else {
        for (i = len; i < max_len; ++i) {
            res->ob_item[i] = Py_NewRef(Py_None);
        }
    }

    _PyObject_GC_TRACK(res);
    return (PyObject*) res;
}


static PyObject *
structseq_repr(PyStructSequence *obj)
{
    PyTypeObject *typ = Py_TYPE(obj);
    _PyUnicodeWriter writer;

    /* Write "typename(" */
    PyObject *type_name = PyUnicode_DecodeUTF8(typ->tp_name,
                                               strlen(typ->tp_name),
                                               NULL);
    if (type_name == NULL) {
        return NULL;
    }

    _PyUnicodeWriter_Init(&writer);
    writer.overallocate = 1;
    /* count 5 characters per item: "x=1, " */
    writer.min_length = (PyUnicode_GET_LENGTH(type_name) + 1
                         + VISIBLE_SIZE(obj) * 5 + 1);

    if (_PyUnicodeWriter_WriteStr(&writer, type_name) < 0) {
        Py_DECREF(type_name);
        goto error;
    }
    Py_DECREF(type_name);

    if (_PyUnicodeWriter_WriteChar(&writer, '(') < 0) {
        goto error;
    }

    for (Py_ssize_t i=0; i < VISIBLE_SIZE(obj); i++) {
        if (i > 0) {
            /* Write ", " */
            if (_PyUnicodeWriter_WriteASCIIString(&writer, ", ", 2) < 0) {
                goto error;
            }
        }

        /* Write "name=repr" */
        const char *name_utf8 = typ->tp_members[i].name;
        if (name_utf8 == NULL) {
            PyErr_Format(PyExc_SystemError, "In structseq_repr(), member %zd name is NULL"
                         " for type %.500s", i, typ->tp_name);
            goto error;
        }

        PyObject *name = PyUnicode_DecodeUTF8(name_utf8, strlen(name_utf8), NULL);
        if (name == NULL) {
            goto error;
        }
        if (_PyUnicodeWriter_WriteStr(&writer, name) < 0) {
            Py_DECREF(name);
            goto error;
        }
        Py_DECREF(name);

        if (_PyUnicodeWriter_WriteChar(&writer, '=') < 0) {
            goto error;
        }

        PyObject *value = PyStructSequence_GetItem((PyObject*)obj, i);
        assert(value != NULL);
        PyObject *repr = PyObject_Repr(value);
        if (repr == NULL) {
            goto error;
        }
        if (_PyUnicodeWriter_WriteStr(&writer, repr) < 0) {
            Py_DECREF(repr);
            goto error;
        }
        Py_DECREF(repr);
    }

    if (_PyUnicodeWriter_WriteChar(&writer, ')') < 0) {
        goto error;
    }

    return _PyUnicodeWriter_Finish(&writer);

error:
    _PyUnicodeWriter_Dealloc(&writer);
    return NULL;
}


static PyObject *
structseq_reduce(PyStructSequence* self, PyObject *Py_UNUSED(ignored))
{
    PyObject* tup = NULL;
    PyObject* dict = NULL;
    PyObject* result;
    Py_ssize_t n_fields, n_visible_fields, n_unnamed_fields, i;

    n_fields = REAL_SIZE(self);
    if (n_fields < 0) {
        return NULL;
    }
    n_visible_fields = VISIBLE_SIZE(self);
    n_unnamed_fields = UNNAMED_FIELDS(self);
    if (n_unnamed_fields < 0) {
        return NULL;
    }
    tup = _PyTuple_FromArray(self->ob_item, n_visible_fields);
    if (!tup)
        goto error;

    dict = PyDict_New();
    if (!dict)
        goto error;

    for (i = n_visible_fields; i < n_fields; i++) {
        const char *n = Py_TYPE(self)->tp_members[i-n_unnamed_fields].name;
        if (PyDict_SetItemString(dict, n, self->ob_item[i]) < 0)
            goto error;
    }

    result = Py_BuildValue("(O(OO))", Py_TYPE(self), tup, dict);

    Py_DECREF(tup);
    Py_DECREF(dict);

    return result;

error:
    Py_XDECREF(tup);
    Py_XDECREF(dict);
    return NULL;
}


static PyObject *
structseq_replace(PyStructSequence *self, PyObject *args, PyObject *kwargs)
{
    PyStructSequence *result = NULL;
    Py_ssize_t n_fields, n_unnamed_fields, i;

    if (!_PyArg_NoPositional("__replace__", args)) {
        return NULL;
    }

    n_fields = REAL_SIZE(self);
    if (n_fields < 0) {
        return NULL;
    }
    n_unnamed_fields = UNNAMED_FIELDS(self);
    if (n_unnamed_fields < 0) {
        return NULL;
    }
    if (n_unnamed_fields > 0) {
        PyErr_Format(PyExc_TypeError,
                     "__replace__() is not supported for %.500s "
                     "because it has unnamed field(s)",
                     Py_TYPE(self)->tp_name);
        return NULL;
    }

    result = (PyStructSequence *) PyStructSequence_New(Py_TYPE(self));
    if (!result) {
        return NULL;
    }

    if (kwargs != NULL) {
        // We do not support types with unnamed fields, so we can iterate over
        // i >= n_visible_fields case without slicing with (i - n_unnamed_fields).
        for (i = 0; i < n_fields; ++i) {
            PyObject *ob;
            if (PyDict_PopString(kwargs, Py_TYPE(self)->tp_members[i].name,
                                 &ob) < 0) {
                goto error;
            }
            if (ob == NULL) {
                ob = Py_NewRef(self->ob_item[i]);
            }
            result->ob_item[i] = ob;
        }
        // Check if there are any unexpected fields.
        if (PyDict_GET_SIZE(kwargs) > 0) {
            PyObject *names = PyDict_Keys(kwargs);
            if (names) {
                PyErr_Format(PyExc_TypeError, "Got unexpected field name(s): %R", names);
                Py_DECREF(names);
            }
            goto error;
        }
    }
    else
    {
        // Just create a copy of the original.
        for (i = 0; i < n_fields; ++i) {
            result->ob_item[i] = Py_NewRef(self->ob_item[i]);
        }
    }

    return (PyObject *)result;

error:
    Py_DECREF(result);
    return NULL;
}

static PyMethodDef structseq_methods[] = {
    {"__reduce__", (PyCFunction)structseq_reduce, METH_NOARGS, NULL},
    {"__replace__", _PyCFunction_CAST(structseq_replace), METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL, NULL}  // sentinel
};

static Py_ssize_t
count_members(PyStructSequence_Desc *desc, Py_ssize_t *n_unnamed_members) {
    Py_ssize_t i;

    *n_unnamed_members = 0;
    for (i = 0; desc->fields[i].name != NULL; ++i) {
        if (desc->fields[i].name == PyStructSequence_UnnamedField) {
            (*n_unnamed_members)++;
        }
    }
    return i;
}

static int
initialize_structseq_dict(PyStructSequence_Desc *desc, PyObject* dict,
                          Py_ssize_t n_members, Py_ssize_t n_unnamed_members) {
    PyObject *v;

#define SET_DICT_FROM_SIZE(key, value)                                         \
    do {                                                                       \
        v = PyLong_FromSsize_t(value);                                         \
        if (v == NULL) {                                                       \
            return -1;                                                         \
        }                                                                      \
        if (PyDict_SetItemString(dict, key, v) < 0) {                          \
            Py_DECREF(v);                                                      \
            return -1;                                                         \
        }                                                                      \
        Py_DECREF(v);                                                          \
    } while (0)

    SET_DICT_FROM_SIZE(visible_length_key, desc->n_in_sequence);
    SET_DICT_FROM_SIZE(real_length_key, n_members);
    SET_DICT_FROM_SIZE(unnamed_fields_key, n_unnamed_members);

    // Prepare and set __match_args__
    Py_ssize_t i, k;
    PyObject* keys = PyTuple_New(desc->n_in_sequence);
    if (keys == NULL) {
        return -1;
    }

    for (i = k = 0; i < desc->n_in_sequence; ++i) {
        if (desc->fields[i].name == PyStructSequence_UnnamedField) {
            continue;
        }
        PyObject* new_member = PyUnicode_FromString(desc->fields[i].name);
        if (new_member == NULL) {
            goto error;
        }
        PyTuple_SET_ITEM(keys, k, new_member);
        k++;
    }

    if (_PyTuple_Resize(&keys, k) == -1) {
        goto error;
    }

    if (PyDict_SetItemString(dict, match_args_key, keys) < 0) {
        goto error;
    }

    Py_DECREF(keys);
    return 0;

error:
    Py_DECREF(keys);
    return -1;
}

static PyMemberDef *
initialize_members(PyStructSequence_Desc *desc,
                   Py_ssize_t n_members, Py_ssize_t n_unnamed_members)
{
    PyMemberDef *members;

    members = PyMem_NEW(PyMemberDef, n_members - n_unnamed_members + 1);
    if (members == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    Py_ssize_t i, k;
    for (i = k = 0; i < n_members; ++i) {
        if (desc->fields[i].name == PyStructSequence_UnnamedField) {
            continue;
        }

        /* The names and docstrings in these MemberDefs are statically */
        /* allocated so it is expected that they'll outlive the MemberDef */
        members[k].name = desc->fields[i].name;
        members[k].type = _Py_T_OBJECT;
        members[k].offset = offsetof(PyStructSequence, ob_item)
          + i * sizeof(PyObject*);
        members[k].flags = Py_READONLY;
        members[k].doc = desc->fields[i].doc;
        k++;
    }
    members[k].name = NULL;

    return members;
}


static void
initialize_static_fields(PyTypeObject *type, PyStructSequence_Desc *desc,
                         PyMemberDef *tp_members, unsigned long tp_flags)
{
    type->tp_name = desc->name;
    type->tp_basicsize = sizeof(PyStructSequence) - sizeof(PyObject *);
    type->tp_itemsize = sizeof(PyObject *);
    type->tp_dealloc = (destructor)structseq_dealloc;
    type->tp_repr = (reprfunc)structseq_repr;
    type->tp_doc = desc->doc;
    type->tp_base = &PyTuple_Type;
    type->tp_methods = structseq_methods;
    type->tp_new = structseq_new;
    type->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | tp_flags;
    type->tp_traverse = (traverseproc) structseq_traverse;
    type->tp_members = tp_members;
}

static int
initialize_static_type(PyTypeObject *type, PyStructSequence_Desc *desc,
                       Py_ssize_t n_members, Py_ssize_t n_unnamed_members) {
    /* initialize_static_fields() should have been called already. */
    if (PyType_Ready(type) < 0) {
        return -1;
    }
    Py_INCREF(type);

    if (initialize_structseq_dict(
            desc, _PyType_GetDict(type), n_members, n_unnamed_members) < 0) {
        Py_DECREF(type);
        return -1;
    }

    return 0;
}

int
_PyStructSequence_InitBuiltinWithFlags(PyInterpreterState *interp,
                                       PyTypeObject *type,
                                       PyStructSequence_Desc *desc,
                                       unsigned long tp_flags)
{
    Py_ssize_t n_unnamed_members;
    Py_ssize_t n_members = count_members(desc, &n_unnamed_members);
    PyMemberDef *members = NULL;

    if ((type->tp_flags & Py_TPFLAGS_READY) == 0) {
        assert(type->tp_name == NULL);
        assert(type->tp_members == NULL);
        assert(type->tp_base == NULL);

        members = initialize_members(desc, n_members, n_unnamed_members);
        if (members == NULL) {
            goto error;
        }
        initialize_static_fields(type, desc, members, tp_flags);

        _Py_SetImmortal(type);
    }
#ifndef NDEBUG
    else {
        // Ensure that the type was initialized.
        assert(type->tp_name != NULL);
        assert(type->tp_members != NULL);
        assert(type->tp_base == &PyTuple_Type);
        assert((type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN));
        assert(_Py_IsImmortal(type));
    }
#endif

    if (_PyStaticType_InitBuiltin(interp, type) < 0) {
        PyErr_Format(PyExc_RuntimeError,
                     "Can't initialize builtin type %s",
                     desc->name);
        goto error;
    }

    if (initialize_structseq_dict(
            desc, _PyType_GetDict(type), n_members, n_unnamed_members) < 0)
    {
        goto error;
    }

    return 0;

error:
    if (members != NULL) {
        PyMem_Free(members);
    }
    return -1;
}

int
PyStructSequence_InitType2(PyTypeObject *type, PyStructSequence_Desc *desc)
{
    PyMemberDef *members;
    Py_ssize_t n_members, n_unnamed_members;

#ifdef Py_TRACE_REFS
    /* if the type object was traced, remove it first
       before overwriting its storage */
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (_PyRefchain_IsTraced(interp, (PyObject *)type)) {
        _Py_ForgetReference((PyObject *)type);
    }
#endif

    /* PyTypeObject has already been initialized */
    if (Py_REFCNT(type) != 0) {
        PyErr_BadInternalCall();
        return -1;
    }

    n_members = count_members(desc, &n_unnamed_members);
    members = initialize_members(desc, n_members, n_unnamed_members);
    if (members == NULL) {
        return -1;
    }
    initialize_static_fields(type, desc, members, 0);
    if (initialize_static_type(type, desc, n_members, n_unnamed_members) < 0) {
        PyMem_Free(members);
        return -1;
    }
    return 0;
}

void
PyStructSequence_InitType(PyTypeObject *type, PyStructSequence_Desc *desc)
{
    (void)PyStructSequence_InitType2(type, desc);
}


/* This is exposed in the internal API, not the public API.
   It is only called on builtin static types, which are all
   initialized via _PyStructSequence_InitBuiltinWithFlags(). */

void
_PyStructSequence_FiniBuiltin(PyInterpreterState *interp, PyTypeObject *type)
{
    // Ensure that the type is initialized
    assert(type->tp_name != NULL);
    assert(type->tp_base == &PyTuple_Type);
    assert((type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN));
    assert(_Py_IsImmortal(type));

    // Cannot delete a type if it still has subclasses
    if (_PyType_HasSubclasses(type)) {
        // XXX Shouldn't this be an error?
        return;
    }

    _PyStaticType_Dealloc(interp, type);

    if (_Py_IsMainInterpreter(interp)) {
        // Undo _PyStructSequence_InitBuiltinWithFlags().
        type->tp_name = NULL;
        PyMem_Free(type->tp_members);
        type->tp_members = NULL;
        type->tp_base = NULL;
    }
}


PyTypeObject *
_PyStructSequence_NewType(PyStructSequence_Desc *desc, unsigned long tp_flags)
{
    PyMemberDef *members;
    PyTypeObject *type;
    PyType_Slot slots[8];
    PyType_Spec spec;
    Py_ssize_t n_members, n_unnamed_members;

    /* Initialize MemberDefs */
    n_members = count_members(desc, &n_unnamed_members);
    members = initialize_members(desc, n_members, n_unnamed_members);
    if (members == NULL) {
        return NULL;
    }

    /* Initialize Slots */
    slots[0] = (PyType_Slot){Py_tp_dealloc, (destructor)structseq_dealloc};
    slots[1] = (PyType_Slot){Py_tp_repr, (reprfunc)structseq_repr};
    slots[2] = (PyType_Slot){Py_tp_doc, (void *)desc->doc};
    slots[3] = (PyType_Slot){Py_tp_methods, structseq_methods};
    slots[4] = (PyType_Slot){Py_tp_new, structseq_new};
    slots[5] = (PyType_Slot){Py_tp_members, members};
    slots[6] = (PyType_Slot){Py_tp_traverse, (traverseproc)structseq_traverse};
    slots[7] = (PyType_Slot){0, 0};

    /* Initialize Spec */
    /* The name in this PyType_Spec is statically allocated so it is */
    /* expected that it'll outlive the PyType_Spec */
    spec.name = desc->name;
    spec.basicsize = sizeof(PyStructSequence) - sizeof(PyObject *);
    spec.itemsize = sizeof(PyObject *);
    spec.flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | tp_flags;
    spec.slots = slots;

    type = (PyTypeObject *)PyType_FromSpecWithBases(&spec, (PyObject *)&PyTuple_Type);
    PyMem_Free(members);
    if (type == NULL) {
        return NULL;
    }

    if (initialize_structseq_dict(
            desc, _PyType_GetDict(type), n_members, n_unnamed_members) < 0) {
        Py_DECREF(type);
        return NULL;
    }

    return type;
}


PyTypeObject *
PyStructSequence_NewType(PyStructSequence_Desc *desc)
{
    return _PyStructSequence_NewType(desc, 0);
}
