// Need limited C API version 3.12 for PyType_FromMetaclass()
#include "pyconfig.h"   // Py_GIL_DISABLED
#if !defined(Py_GIL_DISABLED) && !defined(Py_LIMITED_API)
#  define Py_LIMITED_API 0x030c0000
#endif

#include "parts.h"
#include <stddef.h>               // max_align_t
#include <string.h>               // memset

#include "clinic/heaptype_relative.c.h"

static PyType_Slot empty_slots[] = {
    {0, NULL},
};

static PyObject *
make_sized_heaptypes(PyObject *module, PyObject *args)
{
    PyObject *base = NULL;
    PyObject *sub = NULL;
    PyObject *instance = NULL;
    PyObject *result = NULL;

    int extra_base_size, basicsize;

    int r = PyArg_ParseTuple(args, "ii", &extra_base_size, &basicsize);
    if (!r) {
        goto finally;
    }

    PyType_Spec base_spec = {
        .name = "_testcapi.Base",
        .basicsize = sizeof(PyObject) + extra_base_size,
        .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .slots = empty_slots,
    };
    PyType_Spec sub_spec = {
        .name = "_testcapi.Sub",
        .basicsize = basicsize,
        .flags = Py_TPFLAGS_DEFAULT,
        .slots = empty_slots,
    };

    base = PyType_FromMetaclass(NULL, module, &base_spec, NULL);
    if (!base) {
        goto finally;
    }
    sub = PyType_FromMetaclass(NULL, module, &sub_spec, base);
    if (!sub) {
        goto finally;
    }
    instance = PyObject_CallNoArgs(sub);
    if (!instance) {
        goto finally;
    }
    char *data_ptr = PyObject_GetTypeData(instance, (PyTypeObject *)sub);
    if (!data_ptr) {
        goto finally;
    }
    Py_ssize_t data_size = PyType_GetTypeDataSize((PyTypeObject *)sub);
    if (data_size < 0) {
        goto finally;
    }

    result = Py_BuildValue("OOOKnn", base, sub, instance,
                           (unsigned long long)data_ptr,
                           (Py_ssize_t)(data_ptr - (char*)instance),
                           data_size);
  finally:
    Py_XDECREF(base);
    Py_XDECREF(sub);
    Py_XDECREF(instance);
    return result;
}

static PyObject *
var_heaptype_set_data_to_3s(
    PyObject *self, PyTypeObject *defining_class,
    PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    void *data_ptr = PyObject_GetTypeData(self, defining_class);
    if (!data_ptr) {
        return NULL;
    }
    Py_ssize_t data_size = PyType_GetTypeDataSize(defining_class);
    if (data_size < 0) {
        return NULL;
    }
    memset(data_ptr, 3, data_size);
    Py_RETURN_NONE;
}

static PyObject *
var_heaptype_get_data(PyObject *self, PyTypeObject *defining_class,
                      PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    void *data_ptr = PyObject_GetTypeData(self, defining_class);
    if (!data_ptr) {
        return NULL;
    }
    Py_ssize_t data_size = PyType_GetTypeDataSize(defining_class);
    if (data_size < 0) {
        return NULL;
    }
    return PyBytes_FromStringAndSize(data_ptr, data_size);
}

static PyMethodDef var_heaptype_methods[] = {
    {"set_data_to_3s", _PyCFunction_CAST(var_heaptype_set_data_to_3s),
        METH_METHOD | METH_FASTCALL | METH_KEYWORDS},
    {"get_data", _PyCFunction_CAST(var_heaptype_get_data),
        METH_METHOD | METH_FASTCALL | METH_KEYWORDS},
    {NULL},
};

static PyObject *
subclass_var_heaptype(PyObject *module, PyObject *args)
{
    PyObject *result = NULL;

    PyObject *base; // borrowed from args
    int basicsize, itemsize;
    long pfunc;

    int r = PyArg_ParseTuple(args, "Oiil", &base, &basicsize, &itemsize, &pfunc);
    if (!r) {
        goto finally;
    }

    PyType_Slot slots[] = {
        {Py_tp_methods, var_heaptype_methods},
        {0, NULL},
    };

    PyType_Spec sub_spec = {
        .name = "_testcapi.Sub",
        .basicsize = basicsize,
        .itemsize = itemsize,
        .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_ITEMS_AT_END,
        .slots = slots,
    };

    result = PyType_FromMetaclass(NULL, module, &sub_spec, base);
  finally:
    return result;
}

static PyObject *
subclass_heaptype(PyObject *module, PyObject *args)
{
    PyObject *result = NULL;

    PyObject *base; // borrowed from args
    int basicsize, itemsize;

    int r = PyArg_ParseTuple(args, "Oii", &base, &basicsize, &itemsize);
    if (!r) {
        goto finally;
    }

    PyType_Slot slots[] = {
        {Py_tp_methods, var_heaptype_methods},
        {0, NULL},
    };

    PyType_Spec sub_spec = {
        .name = "_testcapi.Sub",
        .basicsize = basicsize,
        .itemsize = itemsize,
        .flags = Py_TPFLAGS_DEFAULT,
        .slots = slots,
    };

    result = PyType_FromMetaclass(NULL, module, &sub_spec, base);
  finally:
    return result;
}

static PyMemberDef *
heaptype_with_member_extract_and_check_memb(PyObject *self)
{
    PyMemberDef *def = PyType_GetSlot(Py_TYPE(self), Py_tp_members);
    if (!def) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError, "tp_members is NULL");
        }
        return NULL;
    }
    if (!def[0].name) {
        PyErr_SetString(PyExc_ValueError, "tp_members[0] is NULL");
        return NULL;
    }
    if (def[1].name) {
        PyErr_SetString(PyExc_ValueError, "tp_members[1] is not NULL");
        return NULL;
    }
    if (strcmp(def[0].name, "memb")) {
        PyErr_SetString(PyExc_ValueError, "tp_members[0] is not for `memb`");
        return NULL;
    }
    if (def[0].flags) {
        PyErr_SetString(PyExc_ValueError, "tp_members[0] has flags set");
        return NULL;
    }
    return def;
}

static PyObject *
heaptype_with_member_get_memb(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyMemberDef *def = heaptype_with_member_extract_and_check_memb(self);
    return PyMember_GetOne((const char *)self, def);
}

static PyObject *
heaptype_with_member_set_memb(PyObject *self, PyObject *value)
{
    PyMemberDef *def = heaptype_with_member_extract_and_check_memb(self);
    int r = PyMember_SetOne((char *)self, def, value);
    if (r < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
get_memb_offset(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyMemberDef *def = heaptype_with_member_extract_and_check_memb(self);
    return PyLong_FromSsize_t(def->offset);
}

static PyObject *
heaptype_with_member_get_memb_relative(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyMemberDef def = {"memb", Py_T_BYTE, sizeof(PyObject), Py_RELATIVE_OFFSET};
    return PyMember_GetOne((const char *)self, &def);
}

static PyObject *
heaptype_with_member_set_memb_relative(PyObject *self, PyObject *value)
{
    PyMemberDef def = {"memb", Py_T_BYTE, sizeof(PyObject), Py_RELATIVE_OFFSET};
    int r = PyMember_SetOne((char *)self, &def, value);
    if (r < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

typedef struct {
    int padding;  // just so the offset isn't 0
    PyObject *dict;
} HeapCTypeWithDictStruct;

static void
heapctypewithrelativedict_dealloc(PyObject* self)
{
    PyTypeObject *tp = Py_TYPE(self);
    HeapCTypeWithDictStruct *data = PyObject_GetTypeData(self, tp);
    Py_XDECREF(data->dict);
    PyObject_Free(self);
    Py_DECREF(tp);
}

static PyType_Spec HeapCTypeWithRelativeDict_spec = {
    .name = "_testcapi.HeapCTypeWithRelativeDict",
    .basicsize = -(int)sizeof(HeapCTypeWithDictStruct),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = (PyType_Slot[]) {
        {Py_tp_dealloc, heapctypewithrelativedict_dealloc},
        {Py_tp_getset, (PyGetSetDef[]) {
            {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict},
            {NULL} /* Sentinel */
        }},
        {Py_tp_members, (PyMemberDef[]) {
            {"dictobj", _Py_T_OBJECT,
             offsetof(HeapCTypeWithDictStruct, dict),
             Py_RELATIVE_OFFSET},
            {"__dictoffset__", Py_T_PYSSIZET,
             offsetof(HeapCTypeWithDictStruct, dict),
             Py_READONLY | Py_RELATIVE_OFFSET},
            {NULL} /* Sentinel */
        }},
        {0, 0},
    }
};

typedef struct {
    char padding;  // just so the offset isn't 0
    PyObject *weakreflist;
} HeapCTypeWithWeakrefStruct;

static void
heapctypewithrelativeweakref_dealloc(PyObject* self)
{
    PyTypeObject *tp = Py_TYPE(self);
    HeapCTypeWithWeakrefStruct *data = PyObject_GetTypeData(self, tp);
    if (data->weakreflist != NULL) {
        PyObject_ClearWeakRefs(self);
    }
    Py_XDECREF(data->weakreflist);
    PyObject_Free(self);
    Py_DECREF(tp);
}

static PyType_Spec HeapCTypeWithRelativeWeakref_spec = {
    .name = "_testcapi.HeapCTypeWithRelativeWeakref",
    .basicsize = -(int)sizeof(HeapCTypeWithWeakrefStruct),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = (PyType_Slot[]) {
        {Py_tp_dealloc, heapctypewithrelativeweakref_dealloc},
        {Py_tp_members, (PyMemberDef[]) {
            {"weakreflist", _Py_T_OBJECT,
             offsetof(HeapCTypeWithWeakrefStruct, weakreflist),
             Py_RELATIVE_OFFSET},
            {"__weaklistoffset__", Py_T_PYSSIZET,
             offsetof(HeapCTypeWithWeakrefStruct, weakreflist),
             Py_READONLY | Py_RELATIVE_OFFSET},
            {NULL} /* Sentinel */
        }},
        {0, 0},
    }
};

static PyMethodDef heaptype_with_member_methods[] = {
    {"get_memb", heaptype_with_member_get_memb, METH_NOARGS},
    {"set_memb", heaptype_with_member_set_memb, METH_O},
    {"get_memb_offset", get_memb_offset, METH_NOARGS},
    {"get_memb_relative", heaptype_with_member_get_memb_relative, METH_NOARGS},
    {"set_memb_relative", heaptype_with_member_set_memb_relative, METH_O},
    {NULL},
};

/*[clinic input]
make_heaptype_with_member

    extra_base_size: int = 0
    basicsize: int = 0
    member_offset: int = 0
    add_relative_flag: bool = False
    *
    member_name: str = "memb"
    member_flags: int = 0
    member_type: int(c_default="Py_T_BYTE") = -1

[clinic start generated code]*/

static PyObject *
make_heaptype_with_member_impl(PyObject *module, int extra_base_size,
                               int basicsize, int member_offset,
                               int add_relative_flag,
                               const char *member_name, int member_flags,
                               int member_type)
/*[clinic end generated code: output=7005db9a07396997 input=007e29cdbe1d3390]*/
{
    PyObject *base = NULL;
    PyObject *result = NULL;

    PyType_Spec base_spec = {
        .name = "_testcapi.Base",
        .basicsize = sizeof(PyObject) + extra_base_size,
        .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .slots = empty_slots,
    };
    base = PyType_FromMetaclass(NULL, module, &base_spec, NULL);
    if (!base) {
        goto finally;
    }

    PyMemberDef members[] = {
        {member_name, member_type, member_offset,
            member_flags | (add_relative_flag ? Py_RELATIVE_OFFSET : 0)},
        {0},
    };
    PyType_Slot slots[] = {
        {Py_tp_members, members},
        {Py_tp_methods, heaptype_with_member_methods},
        {0, NULL},
    };

    PyType_Spec sub_spec = {
        .name = "_testcapi.Sub",
        .basicsize = basicsize,
        .flags = Py_TPFLAGS_DEFAULT,
        .slots = slots,
    };

    result = PyType_FromMetaclass(NULL, module, &sub_spec, base);
  finally:
    Py_XDECREF(base);
    return result;
}


static PyObject *
test_alignof_max_align_t(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    // We define ALIGNOF_MAX_ALIGN_T even if the compiler doesn't support
    // max_align_t. Double-check that it's correct.
    assert(ALIGNOF_MAX_ALIGN_T > 0);
    assert(ALIGNOF_MAX_ALIGN_T >= _Alignof(long long));
    assert(ALIGNOF_MAX_ALIGN_T >= _Alignof(long double));
    assert(ALIGNOF_MAX_ALIGN_T >= _Alignof(void*));
    assert(ALIGNOF_MAX_ALIGN_T >= _Alignof(void (*)(void)));

    // Ensure it's a power of two
    assert((ALIGNOF_MAX_ALIGN_T & (ALIGNOF_MAX_ALIGN_T - 1)) == 0);

    Py_RETURN_NONE;
}

static PyMethodDef TestMethods[] = {
    {"make_sized_heaptypes", make_sized_heaptypes, METH_VARARGS},
    {"subclass_var_heaptype", subclass_var_heaptype, METH_VARARGS},
    {"subclass_heaptype", subclass_heaptype, METH_VARARGS},
    MAKE_HEAPTYPE_WITH_MEMBER_METHODDEF
    {"test_alignof_max_align_t", test_alignof_max_align_t, METH_NOARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_HeaptypeRelative(PyObject *m)
{
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }

    if (PyModule_AddIntMacro(m, ALIGNOF_MAX_ALIGN_T) < 0) {
        return -1;
    }

#define ADD_FROM_SPEC(SPEC) do {                                \
        PyObject *tp = PyType_FromSpec(SPEC);                   \
        if (!tp) {                                              \
            return -1;                                          \
        }                                                       \
        if (PyModule_AddType(m, (PyTypeObject *)tp) < 0) {      \
            return -1;                                          \
        }                                                       \
    } while (0)

    PyObject *tp;

    tp = PyType_FromSpec(&HeapCTypeWithRelativeDict_spec);
    if (!tp) {
        return -1;
    }
    if (PyModule_AddType(m, (PyTypeObject *)tp) < 0) {
        return -1;
    }
    Py_DECREF(tp);

    tp = PyType_FromSpec(&HeapCTypeWithRelativeWeakref_spec);
    if (!tp) {
        return -1;
    }
    if (PyModule_AddType(m, (PyTypeObject *)tp) < 0) {
        return -1;
    }
    Py_DECREF(tp);

    if (PyModule_AddIntMacro(m, Py_T_PYSSIZET) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(m, Py_READONLY) < 0) {
        return -1;
    }

    return 0;
}
