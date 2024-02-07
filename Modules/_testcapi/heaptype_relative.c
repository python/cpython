#include "pyconfig.h"   // Py_GIL_DISABLED

#ifndef Py_GIL_DISABLED
#define Py_LIMITED_API 0x030c0000 // 3.12
#endif

#include "parts.h"
#include <stddef.h>               // max_align_t
#include <string.h>               // memset

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
    PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
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
                      PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
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

static PyMethodDef heaptype_with_member_methods[] = {
    {"get_memb", heaptype_with_member_get_memb, METH_NOARGS},
    {"set_memb", heaptype_with_member_set_memb, METH_O},
    {"get_memb_offset", get_memb_offset, METH_NOARGS},
    {"get_memb_relative", heaptype_with_member_get_memb_relative, METH_NOARGS},
    {"set_memb_relative", heaptype_with_member_set_memb_relative, METH_O},
    {NULL},
};

static PyObject *
make_heaptype_with_member(PyObject *module, PyObject *args)
{
    PyObject *base = NULL;
    PyObject *result = NULL;

    int extra_base_size, basicsize, offset, add_flag;

    int r = PyArg_ParseTuple(args, "iiip", &extra_base_size, &basicsize, &offset, &add_flag);
    if (!r) {
        goto finally;
    }

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
        {"memb", Py_T_BYTE, offset, add_flag ? Py_RELATIVE_OFFSET : 0},
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
    {"make_heaptype_with_member", make_heaptype_with_member, METH_VARARGS},
    {"test_alignof_max_align_t", test_alignof_max_align_t, METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_HeaptypeRelative(PyObject *m) {
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }

    if (PyModule_AddIntMacro(m, ALIGNOF_MAX_ALIGN_T) < 0) {
        return -1;
    }

    return 0;
}
