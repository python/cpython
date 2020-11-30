/* Implementation helper: a struct that looks like a tuple.
   See timemodule and posixmodule for example uses.

   The structseq helper is considered an internal CPython implementation
   detail.  Docs for modules using structseqs should call them
   "named tuples" (be sure to include a space between the two
   words and add a link back to the term in Docs/glossary.rst).
*/

#include "Python.h"
#include "pycore_tuple.h"         // _PyTuple_FromArray()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "structmember.h"         // PyMemberDef

static const char visible_length_key[] = "n_sequence_fields";
static const char real_length_key[] = "n_fields";
static const char unnamed_fields_key[] = "n_unnamed_fields";

/* Fields with this name have only a field index, not a field name.
   They are only allowed for indices < n_visible_fields. */
const char * const PyStructSequence_UnnamedField = "unnamed field";

_Py_IDENTIFIER(n_sequence_fields);
_Py_IDENTIFIER(n_fields);
_Py_IDENTIFIER(n_unnamed_fields);

static ssize_t
get_type_attr_as_size(PyTypeObject *tp, _Py_Identifier *id)
{
    PyObject *name = _PyUnicode_FromId(id);
    if (name == NULL) {
        return -1;
    }
    PyObject *v = PyDict_GetItemWithError(tp->tp_dict, name);
    if (v == NULL && !PyErr_Occurred()) {
        PyErr_Format(PyExc_TypeError,
                     "Missed attribute '%U' of type %s",
                     name, tp->tp_name);
    }
    return PyLong_AsSsize_t(v);
}

#define VISIBLE_SIZE(op) Py_SIZE(op)
#define VISIBLE_SIZE_TP(tp) get_type_attr_as_size(tp, &PyId_n_sequence_fields)
#define REAL_SIZE_TP(tp) get_type_attr_as_size(tp, &PyId_n_fields)
#define REAL_SIZE(op) REAL_SIZE_TP(Py_TYPE(op))

#define UNNAMED_FIELDS_TP(tp) get_type_attr_as_size(tp, &PyId_n_unnamed_fields)
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
PyStructSequence_SetItem(PyObject* op, Py_ssize_t i, PyObject* v)
{
    PyStructSequence_SET_ITEM(op, i, v);
}

PyObject*
PyStructSequence_GetItem(PyObject* op, Py_ssize_t i)
{
    return PyStructSequence_GET_ITEM(op, i);
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
    PyTypeObject *tp;
    PyObject_GC_UnTrack(obj);

    tp = (PyTypeObject *) Py_TYPE(obj);
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
    PyObject *ob;
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
        Py_INCREF(v);
        res->ob_item[i] = v;
    }
    Py_DECREF(arg);
    for (; i < max_len; ++i) {
        if (dict == NULL) {
            ob = Py_None;
        }
        else {
            ob = _PyDict_GetItemStringWithError(dict,
                type->tp_members[i-n_unnamed_fields].name);
            if (ob == NULL) {
                if (PyErr_Occurred()) {
                    Py_DECREF(res);
                    return NULL;
                }
                ob = Py_None;
            }
        }
        Py_INCREF(ob);
        res->ob_item[i] = ob;
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

        PyObject *value = PyStructSequence_GET_ITEM(obj, i);
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

static PyMethodDef structseq_methods[] = {
    {"__reduce__", (PyCFunction)structseq_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
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
    return 0;
}

static void
initialize_members(PyStructSequence_Desc *desc, PyMemberDef* members,
                   Py_ssize_t n_members) {
    Py_ssize_t i, k;

    for (i = k = 0; i < n_members; ++i) {
        if (desc->fields[i].name == PyStructSequence_UnnamedField) {
            continue;
        }

        /* The names and docstrings in these MemberDefs are statically */
        /* allocated so it is expected that they'll outlive the MemberDef */
        members[k].name = desc->fields[i].name;
        members[k].type = T_OBJECT;
        members[k].offset = offsetof(PyStructSequence, ob_item)
          + i * sizeof(PyObject*);
        members[k].flags = READONLY;
        members[k].doc = desc->fields[i].doc;
        k++;
    }
    members[k].name = NULL;
}

int
PyStructSequence_InitType2(PyTypeObject *type, PyStructSequence_Desc *desc)
{
    PyMemberDef *members;
    Py_ssize_t n_members, n_unnamed_members;

#ifdef Py_TRACE_REFS
    /* if the type object was chained, unchain it first
       before overwriting its storage */
    if (type->ob_base.ob_base._ob_next) {
        _Py_ForgetReference((PyObject *)type);
    }
#endif

    /* PyTypeObject has already been initialized */
    if (Py_REFCNT(type) != 0) {
        PyErr_BadInternalCall();
        return -1;
    }

    type->tp_name = desc->name;
    type->tp_basicsize = sizeof(PyStructSequence) - sizeof(PyObject *);
    type->tp_itemsize = sizeof(PyObject *);
    type->tp_dealloc = (destructor)structseq_dealloc;
    type->tp_repr = (reprfunc)structseq_repr;
    type->tp_doc = desc->doc;
    type->tp_base = &PyTuple_Type;
    type->tp_methods = structseq_methods;
    type->tp_new = structseq_new;
    type->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC;
    type->tp_traverse = (traverseproc) structseq_traverse;

    n_members = count_members(desc, &n_unnamed_members);
    members = PyMem_NEW(PyMemberDef, n_members - n_unnamed_members + 1);
    if (members == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    initialize_members(desc, members, n_members);
    type->tp_members = members;

    if (PyType_Ready(type) < 0) {
        PyMem_FREE(members);
        return -1;
    }
    Py_INCREF(type);

    if (initialize_structseq_dict(
            desc, type->tp_dict, n_members, n_unnamed_members) < 0) {
        PyMem_FREE(members);
        Py_DECREF(type);
        return -1;
    }

    return 0;
}

void
PyStructSequence_InitType(PyTypeObject *type, PyStructSequence_Desc *desc)
{
    (void)PyStructSequence_InitType2(type, desc);
}

PyTypeObject *
PyStructSequence_NewType(PyStructSequence_Desc *desc)
{
    PyMemberDef *members;
    PyTypeObject *type;
    PyType_Slot slots[8];
    PyType_Spec spec;
    Py_ssize_t n_members, n_unnamed_members;

    /* Initialize MemberDefs */
    n_members = count_members(desc, &n_unnamed_members);
    members = PyMem_NEW(PyMemberDef, n_members - n_unnamed_members + 1);
    if (members == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    initialize_members(desc, members, n_members);

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
    spec.flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC;
    spec.slots = slots;

    type = (PyTypeObject *)PyType_FromSpecWithBases(&spec, (PyObject *)&PyTuple_Type);
    PyMem_FREE(members);
    if (type == NULL) {
        return NULL;
    }

    if (initialize_structseq_dict(
            desc, type->tp_dict, n_members, n_unnamed_members) < 0) {
        Py_DECREF(type);
        return NULL;
    }

    return type;
}

int _PyStructSequence_Init(void)
{
    if (_PyUnicode_FromId(&PyId_n_sequence_fields) == NULL
        || _PyUnicode_FromId(&PyId_n_fields) == NULL
        || _PyUnicode_FromId(&PyId_n_unnamed_fields) == NULL)
        return -1;

    return 0;
}
