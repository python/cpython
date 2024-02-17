#include "parts.h"
#include <stddef.h>               // offsetof()


static struct PyModuleDef *_testcapimodule = NULL;  // set at initialization

/* Tests for heap types (PyType_From*) */

static PyObject *pytype_fromspec_meta(PyObject* self, PyObject *meta)
{
    if (!PyType_Check(meta)) {
        PyErr_SetString(
            PyExc_TypeError,
            "pytype_fromspec_meta: must be invoked with a type argument!");
        return NULL;
    }

    PyType_Slot HeapCTypeViaMetaclass_slots[] = {
        {0},
    };

    PyType_Spec HeapCTypeViaMetaclass_spec = {
        "_testcapi.HeapCTypeViaMetaclass",
        sizeof(PyObject),
        0,
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        HeapCTypeViaMetaclass_slots
    };

    return PyType_FromMetaclass(
        (PyTypeObject *) meta, NULL, &HeapCTypeViaMetaclass_spec, NULL);
}


static PyType_Slot empty_type_slots[] = {
    {0, 0},
};

static PyType_Spec MinimalMetaclass_spec = {
    .name = "_testcapi.MinimalMetaclass",
    .basicsize = sizeof(PyHeapTypeObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = empty_type_slots,
};

static PyType_Spec MinimalType_spec = {
    .name = "_testcapi.MinimalSpecType",
    .basicsize = 0,  // Updated later
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = empty_type_slots,
};


static PyObject *
test_from_spec_metatype_inheritance(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *metaclass = NULL;
    PyObject *class = NULL;
    PyObject *new = NULL;
    PyObject *subclasses = NULL;
    PyObject *result = NULL;
    int r;

    metaclass = PyType_FromSpecWithBases(&MinimalMetaclass_spec, (PyObject*)&PyType_Type);
    if (metaclass == NULL) {
        goto finally;
    }
    class = PyObject_CallFunction(metaclass, "s(){}", "TestClass");
    if (class == NULL) {
        goto finally;
    }

    MinimalType_spec.basicsize = (int)(((PyTypeObject*)class)->tp_basicsize);
    new = PyType_FromSpecWithBases(&MinimalType_spec, class);
    if (new == NULL) {
        goto finally;
    }
    if (Py_TYPE(new) != (PyTypeObject*)metaclass) {
        PyErr_SetString(PyExc_AssertionError,
                "Metaclass not set properly!");
        goto finally;
    }

    /* Assert that __subclasses__ is updated */
    subclasses = PyObject_CallMethod(class, "__subclasses__", "");
    if (!subclasses) {
        goto finally;
    }
    r = PySequence_Contains(subclasses, new);
    if (r < 0) {
        goto finally;
    }
    if (r == 0) {
        PyErr_SetString(PyExc_AssertionError,
                "subclasses not set properly!");
        goto finally;
    }

    result = Py_NewRef(Py_None);

finally:
    Py_XDECREF(metaclass);
    Py_XDECREF(class);
    Py_XDECREF(new);
    Py_XDECREF(subclasses);
    return result;
}


static PyObject *
test_from_spec_invalid_metatype_inheritance(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *metaclass_a = NULL;
    PyObject *metaclass_b = NULL;
    PyObject *class_a = NULL;
    PyObject *class_b = NULL;
    PyObject *bases = NULL;
    PyObject *new = NULL;
    PyObject *meta_error_string = NULL;
    PyObject *exc = NULL;
    PyObject *result = NULL;
    PyObject *message = NULL;
    PyObject *args = NULL;

    metaclass_a = PyType_FromSpecWithBases(&MinimalMetaclass_spec, (PyObject*)&PyType_Type);
    if (metaclass_a == NULL) {
        goto finally;
    }
    metaclass_b = PyType_FromSpecWithBases(&MinimalMetaclass_spec, (PyObject*)&PyType_Type);
    if (metaclass_b == NULL) {
        goto finally;
    }
    class_a = PyObject_CallFunction(metaclass_a, "s(){}", "TestClassA");
    if (class_a == NULL) {
        goto finally;
    }

    class_b = PyObject_CallFunction(metaclass_b, "s(){}", "TestClassB");
    if (class_b == NULL) {
        goto finally;
    }

    bases = PyTuple_Pack(2, class_a, class_b);
    if (bases == NULL) {
        goto finally;
    }

    /*
     * The following should raise a TypeError due to a MetaClass conflict.
     */
    new = PyType_FromSpecWithBases(&MinimalType_spec, bases);
    if (new != NULL) {
        PyErr_SetString(PyExc_AssertionError,
                "MetaType conflict not recognized by PyType_FromSpecWithBases");
            goto finally;
    }

    // Assert that the correct exception was raised
    if (PyErr_ExceptionMatches(PyExc_TypeError)) {
        exc = PyErr_GetRaisedException();
        args = PyException_GetArgs(exc);
        if (!PyTuple_Check(args) || PyTuple_Size(args) != 1) {
            PyErr_SetString(PyExc_AssertionError,
                    "TypeError args are not a one-tuple");
            goto finally;
        }
        message = Py_NewRef(PyTuple_GET_ITEM(args, 0));
        meta_error_string = PyUnicode_FromString("metaclass conflict:");
        if (meta_error_string == NULL) {
            goto finally;
        }
        int res = PyUnicode_Contains(message, meta_error_string);
        if (res < 0) {
            goto finally;
        }
        if (res == 0) {
            PyErr_SetString(PyExc_AssertionError,
                    "TypeError did not include expected message.");
            goto finally;
        }
        result = Py_NewRef(Py_None);
    }
finally:
    Py_XDECREF(metaclass_a);
    Py_XDECREF(metaclass_b);
    Py_XDECREF(bases);
    Py_XDECREF(new);
    Py_XDECREF(meta_error_string);
    Py_XDECREF(exc);
    Py_XDECREF(message);
    Py_XDECREF(class_a);
    Py_XDECREF(class_b);
    Py_XDECREF(args);
    return result;
}


static PyObject *
simple_str(PyObject *self) {
    return PyUnicode_FromString("<test>");
}


static PyObject *
test_type_from_ephemeral_spec(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    // Test that a heap type can be created from a spec that's later deleted
    // (along with all its contents).
    // All necessary data must be copied and held by the class
    PyType_Spec *spec = NULL;
    char *name = NULL;
    char *doc = NULL;
    PyType_Slot *slots = NULL;
    PyObject *class = NULL;
    PyObject *instance = NULL;
    PyObject *obj = NULL;
    PyObject *result = NULL;

    /* create a spec (and all its contents) on the heap */

    const char NAME[] = "testcapi._Test";
    const char DOC[] = "a test class";

    spec = PyMem_New(PyType_Spec, 1);
    if (spec == NULL) {
        PyErr_NoMemory();
        goto finally;
    }
    name = PyMem_New(char, sizeof(NAME));
    if (name == NULL) {
        PyErr_NoMemory();
        goto finally;
    }
    memcpy(name, NAME, sizeof(NAME));

    doc = PyMem_New(char, sizeof(DOC));
    if (doc == NULL) {
        PyErr_NoMemory();
        goto finally;
    }
    memcpy(doc, DOC, sizeof(DOC));

    spec->name = name;
    spec->basicsize = sizeof(PyObject);
    spec->itemsize = 0;
    spec->flags = Py_TPFLAGS_DEFAULT;
    slots = PyMem_New(PyType_Slot, 3);
    if (slots == NULL) {
        PyErr_NoMemory();
        goto finally;
    }
    slots[0].slot = Py_tp_str;
    slots[0].pfunc = simple_str;
    slots[1].slot = Py_tp_doc;
    slots[1].pfunc = doc;
    slots[2].slot = 0;
    slots[2].pfunc = NULL;
    spec->slots = slots;

    /* create the class */

    class = PyType_FromSpec(spec);
    if (class == NULL) {
        goto finally;
    }

    /* deallocate the spec (and all contents) */

    // (Explicitly overwrite memory before freeing,
    // so bugs show themselves even without the debug allocator's help.)
    memset(spec, 0xdd, sizeof(PyType_Spec));
    PyMem_Del(spec);
    spec = NULL;
    memset(name, 0xdd, sizeof(NAME));
    PyMem_Del(name);
    name = NULL;
    memset(doc, 0xdd, sizeof(DOC));
    PyMem_Del(doc);
    doc = NULL;
    memset(slots, 0xdd, 3 * sizeof(PyType_Slot));
    PyMem_Del(slots);
    slots = NULL;

    /* check that everything works */

    PyTypeObject *class_tp = (PyTypeObject *)class;
    PyHeapTypeObject *class_ht = (PyHeapTypeObject *)class;
    assert(strcmp(class_tp->tp_name, "testcapi._Test") == 0);
    assert(strcmp(PyUnicode_AsUTF8(class_ht->ht_name), "_Test") == 0);
    assert(strcmp(PyUnicode_AsUTF8(class_ht->ht_qualname), "_Test") == 0);
    assert(strcmp(class_tp->tp_doc, "a test class") == 0);

    // call and check __str__
    instance = PyObject_CallNoArgs(class);
    if (instance == NULL) {
        goto finally;
    }
    obj = PyObject_Str(instance);
    if (obj == NULL) {
        goto finally;
    }
    assert(strcmp(PyUnicode_AsUTF8(obj), "<test>") == 0);
    Py_CLEAR(obj);

    result = Py_NewRef(Py_None);
  finally:
    PyMem_Del(spec);
    PyMem_Del(name);
    PyMem_Del(doc);
    PyMem_Del(slots);
    Py_XDECREF(class);
    Py_XDECREF(instance);
    Py_XDECREF(obj);
    return result;
}

PyType_Slot repeated_doc_slots[] = {
    {Py_tp_doc, "A class used for tests·"},
    {Py_tp_doc, "A class used for tests"},
    {0, 0},
};

PyType_Spec repeated_doc_slots_spec = {
    .name = "RepeatedDocSlotClass",
    .basicsize = sizeof(PyObject),
    .slots = repeated_doc_slots,
};

typedef struct {
    PyObject_HEAD
    int data;
} HeapCTypeWithDataObject;


static struct PyMemberDef members_to_repeat[] = {
    {"Py_T_INT", Py_T_INT, offsetof(HeapCTypeWithDataObject, data), 0, NULL},
    {NULL}
};

PyType_Slot repeated_members_slots[] = {
    {Py_tp_members, members_to_repeat},
    {Py_tp_members, members_to_repeat},
    {0, 0},
};

PyType_Spec repeated_members_slots_spec = {
    .name = "RepeatedMembersSlotClass",
    .basicsize = sizeof(HeapCTypeWithDataObject),
    .slots = repeated_members_slots,
};

static PyObject *
create_type_from_repeated_slots(PyObject *self, PyObject *variant_obj)
{
    PyObject *class = NULL;
    int variant = PyLong_AsLong(variant_obj);
    if (PyErr_Occurred()) {
        return NULL;
    }
    switch (variant) {
        case 0:
            class = PyType_FromSpec(&repeated_doc_slots_spec);
            break;
        case 1:
            class = PyType_FromSpec(&repeated_members_slots_spec);
            break;
        default:
            PyErr_SetString(PyExc_ValueError, "bad test variant");
            break;
        }
    return class;
}


static PyObject *
make_immutable_type_with_base(PyObject *self, PyObject *base)
{
    assert(PyType_Check(base));
    PyType_Spec ImmutableSubclass_spec = {
        .name = "ImmutableSubclass",
        .basicsize = (int)((PyTypeObject*)base)->tp_basicsize,
        .slots = empty_type_slots,
        .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    };
    return PyType_FromSpecWithBases(&ImmutableSubclass_spec, base);
}

static PyObject *
make_type_with_base(PyObject *self, PyObject *base)
{
    assert(PyType_Check(base));
    PyType_Spec ImmutableSubclass_spec = {
        .name = "_testcapi.Subclass",
        .basicsize = (int)((PyTypeObject*)base)->tp_basicsize,
        .slots = empty_type_slots,
        .flags = Py_TPFLAGS_DEFAULT,
    };
    return PyType_FromSpecWithBases(&ImmutableSubclass_spec, base);
}


static PyObject *
pyobject_getitemdata(PyObject *self, PyObject *o)
{
    void *pointer = PyObject_GetItemData(o);
    if (pointer == NULL) {
        return NULL;
    }
    return PyLong_FromVoidPtr(pointer);
}


static PyMethodDef TestMethods[] = {
    {"pytype_fromspec_meta",    pytype_fromspec_meta,            METH_O},
    {"test_type_from_ephemeral_spec", test_type_from_ephemeral_spec, METH_NOARGS},
    {"create_type_from_repeated_slots",
        create_type_from_repeated_slots, METH_O},
    {"test_from_spec_metatype_inheritance", test_from_spec_metatype_inheritance,
     METH_NOARGS},
    {"test_from_spec_invalid_metatype_inheritance",
     test_from_spec_invalid_metatype_inheritance,
     METH_NOARGS},
    {"make_immutable_type_with_base", make_immutable_type_with_base, METH_O},
    {"make_type_with_base", make_type_with_base, METH_O},
    {"pyobject_getitemdata", pyobject_getitemdata, METH_O},
    {NULL},
};


PyDoc_STRVAR(heapdocctype__doc__,
"HeapDocCType(arg1, arg2)\n"
"--\n"
"\n"
"somedoc");

typedef struct {
    PyObject_HEAD
} HeapDocCTypeObject;

static PyType_Slot HeapDocCType_slots[] = {
    {Py_tp_doc, (char*)heapdocctype__doc__},
    {0},
};

static PyType_Spec HeapDocCType_spec = {
    "_testcapi.HeapDocCType",
    sizeof(HeapDocCTypeObject),
    0,
    Py_TPFLAGS_DEFAULT,
    HeapDocCType_slots
};

typedef struct {
    PyObject_HEAD
} NullTpDocTypeObject;

static PyType_Slot NullTpDocType_slots[] = {
    {Py_tp_doc, NULL},
    {0, 0},
};

static PyType_Spec NullTpDocType_spec = {
    "_testcapi.NullTpDocType",
    sizeof(NullTpDocTypeObject),
    0,
    Py_TPFLAGS_DEFAULT,
    NullTpDocType_slots
};


PyDoc_STRVAR(heapgctype__doc__,
"A heap type with GC, and with overridden dealloc.\n\n"
"The 'value' attribute is set to 10 in __init__.");

typedef struct {
    PyObject_HEAD
    int value;
} HeapCTypeObject;

static struct PyMemberDef heapctype_members[] = {
    {"value", Py_T_INT, offsetof(HeapCTypeObject, value)},
    {NULL} /* Sentinel */
};

static int
heapctype_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    ((HeapCTypeObject *)self)->value = 10;
    return 0;
}

static int
heapgcctype_traverse(HeapCTypeObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static void
heapgcctype_dealloc(HeapCTypeObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    PyObject_GC_Del(self);
    Py_DECREF(tp);
}

static PyType_Slot HeapGcCType_slots[] = {
    {Py_tp_init, heapctype_init},
    {Py_tp_members, heapctype_members},
    {Py_tp_dealloc, heapgcctype_dealloc},
    {Py_tp_traverse, heapgcctype_traverse},
    {Py_tp_doc, (char*)heapgctype__doc__},
    {0, 0},
};

static PyType_Spec HeapGcCType_spec = {
    "_testcapi.HeapGcCType",
    sizeof(HeapCTypeObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    HeapGcCType_slots
};

PyDoc_STRVAR(heapctype__doc__,
"A heap type without GC, but with overridden dealloc.\n\n"
"The 'value' attribute is set to 10 in __init__.");

static void
heapctype_dealloc(HeapCTypeObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_Free(self);
    Py_DECREF(tp);
}

static PyType_Slot HeapCType_slots[] = {
    {Py_tp_init, heapctype_init},
    {Py_tp_members, heapctype_members},
    {Py_tp_dealloc, heapctype_dealloc},
    {Py_tp_doc, (char*)heapctype__doc__},
    {0, 0},
};

static PyType_Spec HeapCType_spec = {
    "_testcapi.HeapCType",
    sizeof(HeapCTypeObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    HeapCType_slots
};

PyDoc_STRVAR(heapctypesubclass__doc__,
"Subclass of HeapCType, without GC.\n\n"
"__init__ sets the 'value' attribute to 10 and 'value2' to 20.");

typedef struct {
    HeapCTypeObject base;
    int value2;
} HeapCTypeSubclassObject;

static int
heapctypesubclass_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    /* Call __init__ of the superclass */
    if (heapctype_init(self, args, kwargs) < 0) {
        return -1;
    }
    /* Initialize additional element */
    ((HeapCTypeSubclassObject *)self)->value2 = 20;
    return 0;
}

static struct PyMemberDef heapctypesubclass_members[] = {
    {"value2", Py_T_INT, offsetof(HeapCTypeSubclassObject, value2)},
    {NULL} /* Sentinel */
};

static PyType_Slot HeapCTypeSubclass_slots[] = {
    {Py_tp_init, heapctypesubclass_init},
    {Py_tp_members, heapctypesubclass_members},
    {Py_tp_doc, (char*)heapctypesubclass__doc__},
    {0, 0},
};

static PyType_Spec HeapCTypeSubclass_spec = {
    "_testcapi.HeapCTypeSubclass",
    sizeof(HeapCTypeSubclassObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    HeapCTypeSubclass_slots
};

PyDoc_STRVAR(heapctypewithbuffer__doc__,
"Heap type with buffer support.\n\n"
"The buffer is set to [b'1', b'2', b'3', b'4']");

typedef struct {
    HeapCTypeObject base;
    char buffer[4];
} HeapCTypeWithBufferObject;

static int
heapctypewithbuffer_getbuffer(HeapCTypeWithBufferObject *self, Py_buffer *view, int flags)
{
    self->buffer[0] = '1';
    self->buffer[1] = '2';
    self->buffer[2] = '3';
    self->buffer[3] = '4';
    return PyBuffer_FillInfo(
        view, (PyObject*)self, (void *)self->buffer, 4, 1, flags);
}

static void
heapctypewithbuffer_releasebuffer(HeapCTypeWithBufferObject *self, Py_buffer *view)
{
    assert(view->obj == (void*) self);
}

static PyType_Slot HeapCTypeWithBuffer_slots[] = {
    {Py_bf_getbuffer, heapctypewithbuffer_getbuffer},
    {Py_bf_releasebuffer, heapctypewithbuffer_releasebuffer},
    {Py_tp_doc, (char*)heapctypewithbuffer__doc__},
    {0, 0},
};

static PyType_Spec HeapCTypeWithBuffer_spec = {
    "_testcapi.HeapCTypeWithBuffer",
    sizeof(HeapCTypeWithBufferObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    HeapCTypeWithBuffer_slots
};

PyDoc_STRVAR(heapctypesubclasswithfinalizer__doc__,
"Subclass of HeapCType with a finalizer that reassigns __class__.\n\n"
"__class__ is set to plain HeapCTypeSubclass during finalization.\n"
"__init__ sets the 'value' attribute to 10 and 'value2' to 20.");

static int
heapctypesubclasswithfinalizer_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyTypeObject *base = (PyTypeObject *)PyType_GetSlot(Py_TYPE(self), Py_tp_base);
    initproc base_init = PyType_GetSlot(base, Py_tp_init);
    base_init(self, args, kwargs);
    return 0;
}

static void
heapctypesubclasswithfinalizer_finalize(PyObject *self)
{
    PyObject *oldtype = NULL, *newtype = NULL, *refcnt = NULL;

    /* Save the current exception, if any. */
    PyObject *exc = PyErr_GetRaisedException();

    if (_testcapimodule == NULL) {
        goto cleanup_finalize;
    }
    PyObject *m = PyState_FindModule(_testcapimodule);
    if (m == NULL) {
        goto cleanup_finalize;
    }
    oldtype = PyObject_GetAttrString(m, "HeapCTypeSubclassWithFinalizer");
    if (oldtype == NULL) {
        goto cleanup_finalize;
    }
    newtype = PyObject_GetAttrString(m, "HeapCTypeSubclass");
    if (newtype == NULL) {
        goto cleanup_finalize;
    }

    if (PyObject_SetAttrString(self, "__class__", newtype) < 0) {
        goto cleanup_finalize;
    }
    refcnt = PyLong_FromSsize_t(Py_REFCNT(oldtype));
    if (refcnt == NULL) {
        goto cleanup_finalize;
    }
    if (PyObject_SetAttrString(oldtype, "refcnt_in_del", refcnt) < 0) {
        goto cleanup_finalize;
    }
    Py_DECREF(refcnt);
    refcnt = PyLong_FromSsize_t(Py_REFCNT(newtype));
    if (refcnt == NULL) {
        goto cleanup_finalize;
    }
    if (PyObject_SetAttrString(newtype, "refcnt_in_del", refcnt) < 0) {
        goto cleanup_finalize;
    }

cleanup_finalize:
    Py_XDECREF(oldtype);
    Py_XDECREF(newtype);
    Py_XDECREF(refcnt);

    /* Restore the saved exception. */
    PyErr_SetRaisedException(exc);
}

static PyType_Slot HeapCTypeSubclassWithFinalizer_slots[] = {
    {Py_tp_init, heapctypesubclasswithfinalizer_init},
    {Py_tp_members, heapctypesubclass_members},
    {Py_tp_finalize, heapctypesubclasswithfinalizer_finalize},
    {Py_tp_doc, (char*)heapctypesubclasswithfinalizer__doc__},
    {0, 0},
};

static PyType_Spec HeapCTypeSubclassWithFinalizer_spec = {
    "_testcapi.HeapCTypeSubclassWithFinalizer",
    sizeof(HeapCTypeSubclassObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_FINALIZE,
    HeapCTypeSubclassWithFinalizer_slots
};

static PyType_Slot HeapCTypeMetaclass_slots[] = {
    {0},
};

static PyType_Spec HeapCTypeMetaclass_spec = {
    "_testcapi.HeapCTypeMetaclass",
    sizeof(PyHeapTypeObject),
    sizeof(PyMemberDef),
    Py_TPFLAGS_DEFAULT,
    HeapCTypeMetaclass_slots
};

static PyObject *
heap_ctype_metaclass_custom_tp_new(PyTypeObject *tp, PyObject *args, PyObject *kwargs)
{
    return PyType_Type.tp_new(tp, args, kwargs);
}

static PyType_Slot HeapCTypeMetaclassCustomNew_slots[] = {
    { Py_tp_new, heap_ctype_metaclass_custom_tp_new },
    {0},
};

static PyType_Spec HeapCTypeMetaclassCustomNew_spec = {
    "_testcapi.HeapCTypeMetaclassCustomNew",
    sizeof(PyHeapTypeObject),
    sizeof(PyMemberDef),
    Py_TPFLAGS_DEFAULT,
    HeapCTypeMetaclassCustomNew_slots
};

static PyType_Spec HeapCTypeMetaclassNullNew_spec = {
    .name = "_testcapi.HeapCTypeMetaclassNullNew",
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .slots = empty_type_slots
};


typedef struct {
    PyObject_HEAD
    PyObject *dict;
} HeapCTypeWithDictObject;

static void
heapctypewithdict_dealloc(HeapCTypeWithDictObject* self)
{

    PyTypeObject *tp = Py_TYPE(self);
    Py_XDECREF(self->dict);
    PyObject_Free(self);
    Py_DECREF(tp);
}

static PyGetSetDef heapctypewithdict_getsetlist[] = {
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict},
    {NULL} /* Sentinel */
};

static struct PyMemberDef heapctypewithdict_members[] = {
    {"dictobj", _Py_T_OBJECT, offsetof(HeapCTypeWithDictObject, dict)},
    {"__dictoffset__", Py_T_PYSSIZET, offsetof(HeapCTypeWithDictObject, dict), Py_READONLY},
    {NULL} /* Sentinel */
};

static PyType_Slot HeapCTypeWithDict_slots[] = {
    {Py_tp_members, heapctypewithdict_members},
    {Py_tp_getset, heapctypewithdict_getsetlist},
    {Py_tp_dealloc, heapctypewithdict_dealloc},
    {0, 0},
};

static PyType_Spec HeapCTypeWithDict_spec = {
    "_testcapi.HeapCTypeWithDict",
    sizeof(HeapCTypeWithDictObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    HeapCTypeWithDict_slots
};

static PyType_Spec HeapCTypeWithDict2_spec = {
    "_testcapi.HeapCTypeWithDict2",
    sizeof(HeapCTypeWithDictObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    HeapCTypeWithDict_slots
};

static int
heapmanaged_traverse(HeapCTypeObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return PyObject_VisitManagedDict((PyObject *)self, visit, arg);
}

static int
heapmanaged_clear(HeapCTypeObject *self)
{
    PyObject_ClearManagedDict((PyObject *)self);
    return 0;
}

static void
heapmanaged_dealloc(HeapCTypeObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_ClearManagedDict((PyObject *)self);
    PyObject_GC_UnTrack(self);
    PyObject_GC_Del(self);
    Py_DECREF(tp);
}

static PyType_Slot HeapCTypeWithManagedDict_slots[] = {
    {Py_tp_traverse, heapmanaged_traverse},
    {Py_tp_getset, heapctypewithdict_getsetlist},
    {Py_tp_clear, heapmanaged_clear},
    {Py_tp_dealloc, heapmanaged_dealloc},
    {0, 0},
};

static PyType_Spec  HeapCTypeWithManagedDict_spec = {
    "_testcapi.HeapCTypeWithManagedDict",
    sizeof(PyObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_MANAGED_DICT,
    HeapCTypeWithManagedDict_slots
};

static void
heapctypewithmanagedweakref_dealloc(PyObject* self)
{

    PyTypeObject *tp = Py_TYPE(self);
    PyObject_ClearWeakRefs(self);
    PyObject_GC_UnTrack(self);
    PyObject_GC_Del(self);
    Py_DECREF(tp);
}

static PyType_Slot HeapCTypeWithManagedWeakref_slots[] = {
    {Py_tp_traverse, heapgcctype_traverse},
    {Py_tp_getset, heapctypewithdict_getsetlist},
    {Py_tp_dealloc, heapctypewithmanagedweakref_dealloc},
    {0, 0},
};

static PyType_Spec  HeapCTypeWithManagedWeakref_spec = {
    "_testcapi.HeapCTypeWithManagedWeakref",
    sizeof(PyObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_MANAGED_WEAKREF,
    HeapCTypeWithManagedWeakref_slots
};

static struct PyMemberDef heapctypewithnegativedict_members[] = {
    {"dictobj", _Py_T_OBJECT, offsetof(HeapCTypeWithDictObject, dict)},
    {"__dictoffset__", Py_T_PYSSIZET, -(Py_ssize_t)sizeof(void*), Py_READONLY},
    {NULL} /* Sentinel */
};

static PyType_Slot HeapCTypeWithNegativeDict_slots[] = {
    {Py_tp_members, heapctypewithnegativedict_members},
    {Py_tp_getset, heapctypewithdict_getsetlist},
    {Py_tp_dealloc, heapctypewithdict_dealloc},
    {0, 0},
};

static PyType_Spec HeapCTypeWithNegativeDict_spec = {
    "_testcapi.HeapCTypeWithNegativeDict",
    sizeof(HeapCTypeWithDictObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    HeapCTypeWithNegativeDict_slots
};

typedef struct {
    PyObject_HEAD
    PyObject *weakreflist;
} HeapCTypeWithWeakrefObject;

static struct PyMemberDef heapctypewithweakref_members[] = {
    {"weakreflist", _Py_T_OBJECT, offsetof(HeapCTypeWithWeakrefObject, weakreflist)},
    {"__weaklistoffset__", Py_T_PYSSIZET,
      offsetof(HeapCTypeWithWeakrefObject, weakreflist), Py_READONLY},
    {NULL} /* Sentinel */
};

static void
heapctypewithweakref_dealloc(HeapCTypeWithWeakrefObject* self)
{

    PyTypeObject *tp = Py_TYPE(self);
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);
    Py_XDECREF(self->weakreflist);
    PyObject_Free(self);
    Py_DECREF(tp);
}

static PyType_Slot HeapCTypeWithWeakref_slots[] = {
    {Py_tp_members, heapctypewithweakref_members},
    {Py_tp_dealloc, heapctypewithweakref_dealloc},
    {0, 0},
};

static PyType_Spec HeapCTypeWithWeakref_spec = {
    "_testcapi.HeapCTypeWithWeakref",
    sizeof(HeapCTypeWithWeakrefObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    HeapCTypeWithWeakref_slots
};

static PyType_Spec HeapCTypeWithWeakref2_spec = {
    "_testcapi.HeapCTypeWithWeakref2",
    sizeof(HeapCTypeWithWeakrefObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    HeapCTypeWithWeakref_slots
};

PyDoc_STRVAR(heapctypesetattr__doc__,
"A heap type without GC, but with overridden __setattr__.\n\n"
"The 'value' attribute is set to 10 in __init__ and updated via attribute setting.");

typedef struct {
    PyObject_HEAD
    long value;
} HeapCTypeSetattrObject;

static struct PyMemberDef heapctypesetattr_members[] = {
    {"pvalue", Py_T_LONG, offsetof(HeapCTypeSetattrObject, value)},
    {NULL} /* Sentinel */
};

static int
heapctypesetattr_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    ((HeapCTypeSetattrObject *)self)->value = 10;
    return 0;
}

static void
heapctypesetattr_dealloc(HeapCTypeSetattrObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_Free(self);
    Py_DECREF(tp);
}

static int
heapctypesetattr_setattro(HeapCTypeSetattrObject *self, PyObject *attr, PyObject *value)
{
    PyObject *svalue = PyUnicode_FromString("value");
    if (svalue == NULL)
        return -1;
    int eq = PyObject_RichCompareBool(svalue, attr, Py_EQ);
    Py_DECREF(svalue);
    if (eq < 0)
        return -1;
    if (!eq) {
        return PyObject_GenericSetAttr((PyObject*) self, attr, value);
    }
    if (value == NULL) {
        self->value = 0;
        return 0;
    }
    PyObject *ivalue = PyNumber_Long(value);
    if (ivalue == NULL)
        return -1;
    long v = PyLong_AsLong(ivalue);
    Py_DECREF(ivalue);
    if (v == -1 && PyErr_Occurred())
        return -1;
    self->value = v;
    return 0;
}

static PyType_Slot HeapCTypeSetattr_slots[] = {
    {Py_tp_init, heapctypesetattr_init},
    {Py_tp_members, heapctypesetattr_members},
    {Py_tp_setattro, heapctypesetattr_setattro},
    {Py_tp_dealloc, heapctypesetattr_dealloc},
    {Py_tp_doc, (char*)heapctypesetattr__doc__},
    {0, 0},
};

static PyType_Spec HeapCTypeSetattr_spec = {
    "_testcapi.HeapCTypeSetattr",
    sizeof(HeapCTypeSetattrObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    HeapCTypeSetattr_slots
};

PyDoc_STRVAR(HeapCCollection_doc,
"Tuple-like heap type that uses PyObject_GetItemData for items.");

static PyObject*
HeapCCollection_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)
{
    PyObject *self = NULL;
    PyObject *result = NULL;

    Py_ssize_t size = PyTuple_GET_SIZE(args);
    self = subtype->tp_alloc(subtype, size);
    if (!self) {
        goto finally;
    }
    PyObject **data = PyObject_GetItemData(self);
    if (!data) {
        goto finally;
    }

    for (Py_ssize_t i = 0; i < size; i++) {
        data[i] = Py_NewRef(PyTuple_GET_ITEM(args, i));
    }

    result = self;
    self = NULL;
  finally:
    Py_XDECREF(self);
    return result;
}

static Py_ssize_t
HeapCCollection_length(PyVarObject *self)
{
    return Py_SIZE(self);
}

static PyObject*
HeapCCollection_item(PyObject *self, Py_ssize_t i)
{
    if (i < 0 || i >= Py_SIZE(self)) {
        return PyErr_Format(PyExc_IndexError, "index %zd out of range", i);
    }
    PyObject **data = PyObject_GetItemData(self);
    if (!data) {
        return NULL;
    }
    return Py_NewRef(data[i]);
}

static int
HeapCCollection_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyObject **data = PyObject_GetItemData(self);
    if (!data) {
        return -1;
    }
    for (Py_ssize_t i = 0; i < Py_SIZE(self); i++) {
        Py_VISIT(data[i]);
    }
    return 0;
}

static int
HeapCCollection_clear(PyObject *self)
{
    PyObject **data = PyObject_GetItemData(self);
    if (!data) {
        return -1;
    }
    Py_ssize_t size = Py_SIZE(self);
    Py_SET_SIZE(self, 0);
    for (Py_ssize_t i = 0; i < size; i++) {
        Py_CLEAR(data[i]);
    }
    return 0;
}

static void
HeapCCollection_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    HeapCCollection_clear(self);
    PyObject_GC_UnTrack(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyType_Slot HeapCCollection_slots[] = {
    {Py_tp_new, HeapCCollection_new},
    {Py_sq_length, HeapCCollection_length},
    {Py_sq_item, HeapCCollection_item},
    {Py_tp_traverse, HeapCCollection_traverse},
    {Py_tp_clear, HeapCCollection_clear},
    {Py_tp_dealloc, HeapCCollection_dealloc},
    {Py_tp_doc, (void *)HeapCCollection_doc},
    {0, 0},
};

static PyType_Spec HeapCCollection_spec = {
    .name = "_testcapi.HeapCCollection",
    .basicsize = sizeof(PyVarObject),
    .itemsize = sizeof(PyObject*),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_ITEMS_AT_END),
    .slots = HeapCCollection_slots,
};

int
_PyTestCapi_Init_Heaptype(PyObject *m) {
    _testcapimodule = PyModule_GetDef(m);

    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }

#define ADD(name, value) do { \
        if (PyModule_Add(m, name, value) < 0) { \
            return -1; \
        } \
    } while (0)

    PyObject *HeapDocCType = PyType_FromSpec(&HeapDocCType_spec);
    ADD("HeapDocCType", HeapDocCType);

    /* bpo-41832: Add a new type to test PyType_FromSpec()
       now can accept a NULL tp_doc slot. */
    PyObject *NullTpDocType = PyType_FromSpec(&NullTpDocType_spec);
    ADD("NullTpDocType", NullTpDocType);

    PyObject *HeapGcCType = PyType_FromSpec(&HeapGcCType_spec);
    ADD("HeapGcCType", HeapGcCType);

    PyObject *HeapCType = PyType_FromSpec(&HeapCType_spec);
    if (HeapCType == NULL) {
        return -1;
    }
    PyObject *subclass_bases = PyTuple_Pack(1, HeapCType);
    Py_DECREF(HeapCType);
    if (subclass_bases == NULL) {
        return -1;
    }
    PyObject *HeapCTypeSubclass = PyType_FromSpecWithBases(&HeapCTypeSubclass_spec, subclass_bases);
    Py_DECREF(subclass_bases);
    ADD("HeapCTypeSubclass", HeapCTypeSubclass);

    PyObject *HeapCTypeWithDict = PyType_FromSpec(&HeapCTypeWithDict_spec);
    ADD("HeapCTypeWithDict", HeapCTypeWithDict);

    PyObject *HeapCTypeWithDict2 = PyType_FromSpec(&HeapCTypeWithDict2_spec);
    ADD("HeapCTypeWithDict2", HeapCTypeWithDict2);

    PyObject *HeapCTypeWithNegativeDict = PyType_FromSpec(&HeapCTypeWithNegativeDict_spec);
    ADD("HeapCTypeWithNegativeDict", HeapCTypeWithNegativeDict);

    PyObject *HeapCTypeWithManagedDict = PyType_FromSpec(&HeapCTypeWithManagedDict_spec);
    ADD("HeapCTypeWithManagedDict", HeapCTypeWithManagedDict);

    PyObject *HeapCTypeWithManagedWeakref = PyType_FromSpec(&HeapCTypeWithManagedWeakref_spec);
    ADD("HeapCTypeWithManagedWeakref", HeapCTypeWithManagedWeakref);

    PyObject *HeapCTypeWithWeakref = PyType_FromSpec(&HeapCTypeWithWeakref_spec);
    ADD("HeapCTypeWithWeakref", HeapCTypeWithWeakref);

    PyObject *HeapCTypeWithWeakref2 = PyType_FromSpec(&HeapCTypeWithWeakref2_spec);
    ADD("HeapCTypeWithWeakref2", HeapCTypeWithWeakref2);

    PyObject *HeapCTypeWithBuffer = PyType_FromSpec(&HeapCTypeWithBuffer_spec);
    ADD("HeapCTypeWithBuffer", HeapCTypeWithBuffer);

    PyObject *HeapCTypeSetattr = PyType_FromSpec(&HeapCTypeSetattr_spec);
    ADD("HeapCTypeSetattr", HeapCTypeSetattr);

    PyObject *subclass_with_finalizer_bases = PyTuple_Pack(1, HeapCTypeSubclass);
    if (subclass_with_finalizer_bases == NULL) {
        return -1;
    }
    PyObject *HeapCTypeSubclassWithFinalizer = PyType_FromSpecWithBases(
        &HeapCTypeSubclassWithFinalizer_spec, subclass_with_finalizer_bases);
    Py_DECREF(subclass_with_finalizer_bases);
    ADD("HeapCTypeSubclassWithFinalizer", HeapCTypeSubclassWithFinalizer);

    PyObject *HeapCTypeMetaclass = PyType_FromMetaclass(
        &PyType_Type, m, &HeapCTypeMetaclass_spec, (PyObject *) &PyType_Type);
    ADD("HeapCTypeMetaclass", HeapCTypeMetaclass);

    PyObject *HeapCTypeMetaclassCustomNew = PyType_FromMetaclass(
        &PyType_Type, m, &HeapCTypeMetaclassCustomNew_spec, (PyObject *) &PyType_Type);
    ADD("HeapCTypeMetaclassCustomNew", HeapCTypeMetaclassCustomNew);

    PyObject *HeapCTypeMetaclassNullNew = PyType_FromMetaclass(
        &PyType_Type, m, &HeapCTypeMetaclassNullNew_spec, (PyObject *) &PyType_Type);
    ADD("HeapCTypeMetaclassNullNew", HeapCTypeMetaclassNullNew);

    PyObject *HeapCCollection = PyType_FromMetaclass(
        NULL, m, &HeapCCollection_spec, NULL);
    if (HeapCCollection == NULL) {
        return -1;
    }
    int rc = PyModule_AddType(m, (PyTypeObject *)HeapCCollection);
    Py_DECREF(HeapCCollection);
    if (rc < 0) {
        return -1;
    }

    return 0;
}
