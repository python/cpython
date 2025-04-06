/*
  ToDo:

  Get rid of the checker (and also the converters) field in PyCFuncPtrObject and
  StgInfo, and replace them by slot functions in StgInfo.

  think about a buffer-like object (memory? bytes?)

  Should POINTER(c_char) and POINTER(c_wchar) have a .value property?
  What about c_char and c_wchar arrays then?

  Add from_mmap, from_file, from_string metaclass methods.

  Maybe we can get away with from_file (calls read) and with a from_buffer
  method?

  And what about the to_mmap, to_file, to_str(?) methods?  They would clobber
  the namespace, probably. So, functions instead? And we already have memmove...
*/

/*

Name                    methods, members, getsets
==============================================================================

PyCStructType_Type              __new__(), from_address(), __mul__(), from_param()
UnionType_Type                  __new__(), from_address(), __mul__(), from_param()
PyCPointerType_Type             __new__(), from_address(), __mul__(), from_param(), set_type()
PyCArrayType_Type               __new__(), from_address(), __mul__(), from_param()
PyCSimpleType_Type              __new__(), from_address(), __mul__(), from_param()

PyCData_Type
  Struct_Type                   __new__(), __init__()
  PyCPointer_Type               __new__(), __init__(), _as_parameter_, contents
  PyCArray_Type                 __new__(), __init__(), _as_parameter_, __get/setitem__(), __len__()
  Simple_Type                   __new__(), __init__(), _as_parameter_

PyCField_Type

==============================================================================

class methods
-------------

It has some similarity to the byref() construct compared to pointer()
from_address(addr)
    - construct an instance from a given memory block (sharing this memory block)

from_param(obj)
    - typecheck and convert a Python object into a C function call parameter
      The result may be an instance of the type, or an integer or tuple
      (typecode, value[, obj])

instance methods/properties
---------------------------

_as_parameter_
    - convert self into a C function call parameter
      This is either an integer, or a 3-tuple (typecode, value, obj)

functions
---------

sizeof(cdata)
    - return the number of bytes the buffer contains

sizeof(ctype)
    - return the number of bytes the buffer of an instance would contain

byref(cdata)

addressof(cdata)

pointer(cdata)

POINTER(ctype)

bytes(cdata)
    - return the buffer contents as a sequence of bytes (which is currently a string)

*/

/*
 * PyCStructType_Type
 * UnionType_Type
 * PyCPointerType_Type
 * PyCArrayType_Type
 * PyCSimpleType_Type
 *
 * PyCData_Type
 * Struct_Type
 * Union_Type
 * PyCArray_Type
 * Simple_Type
 * PyCPointer_Type
 * PyCField_Type
 *
 */
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
// windows.h must be included before pycore internal headers
#ifdef MS_WIN32
#  include <windows.h>
#endif

#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCall()
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString()
#include "pycore_pyatomic_ft_wrappers.h"
#ifdef MS_WIN32
#  include "pycore_modsupport.h"  // _PyArg_NoKeywords()
#endif


#include <ffi.h>
#ifdef MS_WIN32
#include <malloc.h>
#ifndef IS_INTRESOURCE
#define IS_INTRESOURCE(x) (((size_t)(x) >> 16) == 0)
#endif
#else
#include <dlfcn.h>
#endif
#include "ctypes.h"

#include "pycore_long.h"          // _PyLong_GetZero()

/*[clinic input]
module _ctypes
class _ctypes.CFuncPtr "PyCFuncPtrObject *" "&PyCFuncPtr_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=58e8c99474bc631e]*/

#define clinic_state() (get_module_state_by_class(cls))
#define clinic_state_sub() (get_module_state_by_class(cls->tp_base))
#include "clinic/_ctypes.c.h"
#undef clinic_state
#undef clinic_state_sub

/****************************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *key;
    PyObject *dict;
} DictRemoverObject;

#define _DictRemoverObject_CAST(op)     ((DictRemoverObject *)(op))

static int
_DictRemover_traverse(PyObject *myself, visitproc visit, void *arg)
{
    DictRemoverObject *self = _DictRemoverObject_CAST(myself);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->key);
    Py_VISIT(self->dict);
    return 0;
}

static int
_DictRemover_clear(PyObject *myself)
{
    DictRemoverObject *self = _DictRemoverObject_CAST(myself);
    Py_CLEAR(self->key);
    Py_CLEAR(self->dict);
    return 0;
}

static void
_DictRemover_dealloc(PyObject *myself)
{
    PyTypeObject *tp = Py_TYPE(myself);
    PyObject_GC_UnTrack(myself);
    (void)_DictRemover_clear(myself);
    tp->tp_free(myself);
    Py_DECREF(tp);
}

static PyObject *
_DictRemover_call(PyObject *myself, PyObject *args, PyObject *kw)
{
    DictRemoverObject *self = _DictRemoverObject_CAST(myself);
    if (self->key && self->dict) {
        if (-1 == PyDict_DelItem(self->dict, self->key)) {
            PyErr_FormatUnraisable("Exception ignored while "
                                   "calling _ctypes.DictRemover");
        }
        Py_CLEAR(self->key);
        Py_CLEAR(self->dict);
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(dictremover_doc, "deletes a key from a dictionary");

static PyType_Slot dictremover_slots[] = {
    {Py_tp_dealloc, _DictRemover_dealloc},
    {Py_tp_traverse, _DictRemover_traverse},
    {Py_tp_clear, _DictRemover_clear},
    {Py_tp_call, _DictRemover_call},
    {Py_tp_doc, (void *)dictremover_doc},
    {0, NULL},
};

static PyType_Spec dictremover_spec = {
    .name = "_ctypes.DictRemover",
    .basicsize = sizeof(DictRemoverObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = dictremover_slots,
};

int
PyDict_SetItemProxy(ctypes_state *st, PyObject *dict, PyObject *key, PyObject *item)
{
    PyObject *obj;
    DictRemoverObject *remover;
    PyObject *proxy;
    int result;

    obj = _PyObject_CallNoArgs((PyObject *)st->DictRemover_Type);
    if (obj == NULL)
        return -1;

    remover = (DictRemoverObject *)obj;
    assert(remover->key == NULL);
    assert(remover->dict == NULL);
    remover->key = Py_NewRef(key);
    remover->dict = Py_NewRef(dict);

    proxy = PyWeakref_NewProxy(item, obj);
    Py_DECREF(obj);
    if (proxy == NULL)
        return -1;

    result = PyDict_SetItem(dict, key, proxy);
    Py_DECREF(proxy);
    return result;
}

static int
_PyDict_GetItemProxy(PyObject *dict, PyObject *key, PyObject **presult)
{
    int rc = PyDict_GetItemRef(dict, key, presult);
    PyObject *item = *presult;
    if (item && PyWeakref_CheckProxy(item)) {
        rc = PyWeakref_GetRef(item, presult);
        Py_DECREF(item);
    }
    return rc;
}

/******************************************************************/

/*
  Allocate a memory block for a pep3118 format string, filled with
  a suitable PEP 3118 type code corresponding to the given ctypes
  type. Returns NULL on failure, with the error indicator set.

  This produces type codes in the standard size mode (cf. struct module),
  since the endianness may need to be swapped to a non-native one
  later on.
 */
static char *
_ctypes_alloc_format_string_for_type(char code, int big_endian)
{
    char *result;
    char pep_code = '\0';

    switch (code) {
#if SIZEOF_INT == 2
    case 'i': pep_code = 'h'; break;
    case 'I': pep_code = 'H'; break;
#elif SIZEOF_INT == 4
    case 'i': pep_code = 'i'; break;
    case 'I': pep_code = 'I'; break;
#elif SIZEOF_INT == 8
    case 'i': pep_code = 'q'; break;
    case 'I': pep_code = 'Q'; break;
#else
# error SIZEOF_INT has an unexpected value
#endif /* SIZEOF_INT */
#if SIZEOF_LONG == 4
    case 'l': pep_code = 'l'; break;
    case 'L': pep_code = 'L'; break;
#elif SIZEOF_LONG == 8
    case 'l': pep_code = 'q'; break;
    case 'L': pep_code = 'Q'; break;
#else
# error SIZEOF_LONG has an unexpected value
#endif /* SIZEOF_LONG */
#if SIZEOF__BOOL == 1
    case '?': pep_code = '?'; break;
#elif SIZEOF__BOOL == 2
    case '?': pep_code = 'H'; break;
#elif SIZEOF__BOOL == 4
    case '?': pep_code = 'L'; break;
#elif SIZEOF__BOOL == 8
    case '?': pep_code = 'Q'; break;
#else
# error SIZEOF__BOOL has an unexpected value
#endif /* SIZEOF__BOOL */
    default:
        /* The standard-size code is the same as the ctypes one */
        pep_code = code;
        break;
    }

    result = PyMem_Malloc(3);
    if (result == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    result[0] = big_endian ? '>' : '<';
    result[1] = pep_code;
    result[2] = '\0';
    return result;
}

/*
  Allocate a memory block for a pep3118 format string, copy prefix (if
  non-null) and suffix into it.  Returns NULL on failure, with the error
  indicator set.  If called with a suffix of NULL the error indicator must
  already be set.
 */
static char *
_ctypes_alloc_format_string(const char *prefix, const char *suffix)
{
    size_t len;
    char *result;

    if (suffix == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    len = strlen(suffix);
    if (prefix)
        len += strlen(prefix);
    result = PyMem_Malloc(len + 1);
    if (result == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    if (prefix)
        strcpy(result, prefix);
    else
        result[0] = '\0';
    strcat(result, suffix);
    return result;
}

/*
  Allocate a memory block for a pep3118 format string, adding
  the given prefix (if non-null), an additional shape prefix, and a suffix.
  Returns NULL on failure, with the error indicator set.  If called with
  a suffix of NULL the error indicator must already be set.
 */
static char *
_ctypes_alloc_format_string_with_shape(int ndim, const Py_ssize_t *shape,
                                       const char *prefix, const char *suffix)
{
    char *new_prefix;
    char *result;
    char buf[32];
    Py_ssize_t prefix_len;
    int k;

    prefix_len = 32 * ndim + 3;
    if (prefix)
        prefix_len += strlen(prefix);
    new_prefix = PyMem_Malloc(prefix_len);
    if (new_prefix == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    new_prefix[0] = '\0';
    if (prefix)
        strcpy(new_prefix, prefix);
    if (ndim > 0) {
        /* Add the prefix "(shape[0],shape[1],...,shape[ndim-1])" */
        strcat(new_prefix, "(");
        for (k = 0; k < ndim; ++k) {
            if (k < ndim-1) {
                sprintf(buf, "%zd,", shape[k]);
            } else {
                sprintf(buf, "%zd)", shape[k]);
            }
            strcat(new_prefix, buf);
        }
    }
    result = _ctypes_alloc_format_string(new_prefix, suffix);
    PyMem_Free(new_prefix);
    return result;
}

/* StructParamObject and StructParam_Type are used in _ctypes_callproc()
   for argument.keep to call PyMem_Free(ptr) on Py_DECREF(argument).

   StructUnionType_paramfunc() creates such object when a ctypes Structure is
   passed by copy to a C function. */
typedef struct {
    PyObject_HEAD
    void *ptr;
    PyObject *keep;  // If set, a reference to the original CDataObject.
} StructParamObject;

#define _StructParamObject_CAST(op) ((StructParamObject *)(op))

static int
StructParam_traverse(PyObject *myself, visitproc visit, void *arg)
{
    StructParamObject *self = _StructParamObject_CAST(myself);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->keep);
    return 0;
}

static int
StructParam_clear(PyObject *myself)
{
    StructParamObject *self = _StructParamObject_CAST(myself);
    Py_CLEAR(self->keep);
    return 0;
}

static void
StructParam_dealloc(PyObject *myself)
{
    StructParamObject *self = _StructParamObject_CAST(myself);
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(myself);
    (void)StructParam_clear(myself);
    PyMem_Free(self->ptr);
    tp->tp_free(myself);
    Py_DECREF(tp);
}

static PyType_Slot structparam_slots[] = {
    {Py_tp_traverse, StructParam_traverse},
    {Py_tp_clear, StructParam_clear},
    {Py_tp_dealloc, StructParam_dealloc},
    {0, NULL},
};

static PyType_Spec structparam_spec = {
    .name = "_ctypes.StructParam_Type",
    .basicsize = sizeof(StructParamObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_DISALLOW_INSTANTIATION),
    .slots = structparam_slots,
};

/*
  CType_Type - a base metaclass. Its instances (classes) have a StgInfo.
  */

/*[clinic input]
class _ctypes.CType_Type "PyObject *" "clinic_state()->CType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=8389fc5b74a84f2a]*/

static int
CType_Type_traverse(PyObject *self, visitproc visit, void *arg)
{
    StgInfo *info = _PyStgInfo_FromType_NoState(self);
    if (!info) {
        PyErr_FormatUnraisable("Exception ignored while "
                               "calling ctypes traverse function %R", self);
    }
    if (info) {
        Py_VISIT(info->proto);
        Py_VISIT(info->argtypes);
        Py_VISIT(info->converters);
        Py_VISIT(info->restype);
        Py_VISIT(info->checker);
        Py_VISIT(info->module);
    }
    Py_VISIT(Py_TYPE(self));
    return PyType_Type.tp_traverse(self, visit, arg);
}

void
ctype_clear_stginfo(StgInfo *info)
{
    assert(info);
    Py_CLEAR(info->proto);
    Py_CLEAR(info->argtypes);
    Py_CLEAR(info->converters);
    Py_CLEAR(info->restype);
    Py_CLEAR(info->checker);
    Py_CLEAR(info->module);
}

static int
CType_Type_clear(PyObject *self)
{
    StgInfo *info = _PyStgInfo_FromType_NoState(self);
    if (!info) {
        PyErr_FormatUnraisable("Exception ignored while "
                               "clearing ctypes %R", self);
    }
    if (info) {
        ctype_clear_stginfo(info);
    }
    return PyType_Type.tp_clear(self);
}

static void
CType_Type_dealloc(PyObject *self)
{
    StgInfo *info = _PyStgInfo_FromType_NoState(self);
    if (!info) {
        PyErr_FormatUnraisable("Exception ignored while "
                               "deallocating ctypes %R", self);
    }
    if (info) {
        PyMem_Free(info->ffi_type_pointer.elements);
        info->ffi_type_pointer.elements = NULL;
        PyMem_Free(info->format);
        info->format = NULL;
        PyMem_Free(info->shape);
        info->shape = NULL;
        ctype_clear_stginfo(info);
    }

    PyTypeObject *tp = Py_TYPE(self);
    PyType_Type.tp_dealloc(self);
    Py_DECREF(tp);
}

/*[clinic input]
_ctypes.CType_Type.__sizeof__

    cls: defining_class
    /
Return memory consumption of the type object.
[clinic start generated code]*/

static PyObject *
_ctypes_CType_Type___sizeof___impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=c68c235be84d03f3 input=d064433b6110d1ce]*/
{
    Py_ssize_t size = Py_TYPE(self)->tp_basicsize;
    size += Py_TYPE(self)->tp_itemsize * Py_SIZE(self);

    ctypes_state *st = get_module_state_by_class(cls);
    StgInfo *info;
    if (PyStgInfo_FromType(st, self, &info) < 0) {
        return NULL;
    }
    if (info) {
        if (info->format) {
            size += strlen(info->format) + 1;
        }
        if (info->ffi_type_pointer.elements) {
            size += (info->length + 1) * sizeof(ffi_type *);
        }
        size += info->ndim * sizeof(Py_ssize_t);
    }

    return PyLong_FromSsize_t(size);
}

static PyObject *
CType_Type_repeat(PyObject *self, Py_ssize_t length);


static PyMethodDef ctype_methods[] = {
    _CTYPES_CTYPE_TYPE___SIZEOF___METHODDEF
    {0},
};

static PyType_Slot ctype_type_slots[] = {
    {Py_tp_token, Py_TP_USE_SPEC},
    {Py_tp_traverse, CType_Type_traverse},
    {Py_tp_clear, CType_Type_clear},
    {Py_tp_dealloc, CType_Type_dealloc},
    {Py_tp_methods, ctype_methods},
    // Sequence protocol.
    {Py_sq_repeat, CType_Type_repeat},
    {0, NULL},
};

PyType_Spec pyctype_type_spec = {
    .name = "_ctypes.CType_Type",
    .basicsize = -(Py_ssize_t)sizeof(StgInfo),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE |
              Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_BASETYPE ),
    .slots = ctype_type_slots,
};

/*
  PyCStructType_Type - a meta type/class.  Creating a new class using this one as
  __metaclass__ will call the constructor StructUnionType_new.
  It initializes the C accessible fields somehow.
*/

static PyCArgObject *
StructUnionType_paramfunc(ctypes_state *st, CDataObject *self)
{
    PyCArgObject *parg;
    PyObject *obj;
    void *ptr;

    if ((size_t)self->b_size > sizeof(void*)) {
        ptr = PyMem_Malloc(self->b_size);
        if (ptr == NULL) {
            return NULL;
        }
        locked_memcpy_from(ptr, self, self->b_size);

        /* Create a Python object which calls PyMem_Free(ptr) in
           its deallocator. The object will be destroyed
           at _ctypes_callproc() cleanup. */
        PyTypeObject *tp = st->StructParam_Type;
        obj = tp->tp_alloc(tp, 0);
        if (obj == NULL) {
            PyMem_Free(ptr);
            return NULL;
        }

        StructParamObject *struct_param = (StructParamObject *)obj;
        struct_param->ptr = ptr;
        struct_param->keep = Py_NewRef(self);
    } else {
        ptr = self->b_ptr;
        obj = Py_NewRef(self);
    }

    parg = PyCArgObject_new(st);
    if (parg == NULL) {
        Py_DECREF(obj);
        return NULL;
    }

    StgInfo *stginfo;
    if (PyStgInfo_FromObject(st, (PyObject *)self, &stginfo) < 0) {
        Py_DECREF(obj);
        return NULL;
    }
    assert(stginfo); /* Cannot be NULL for structure/union instances */

    parg->tag = 'V';
    parg->pffi_type = &stginfo->ffi_type_pointer;
    parg->value.p = ptr;
    parg->size = self->b_size;
    parg->obj = obj;
    return parg;
}

static int
StructUnionType_init(PyObject *self, PyObject *args, PyObject *kwds, int isStruct)
{
    PyObject *fields;

    PyObject *attrdict = PyType_GetDict((PyTypeObject *)self);
    if (!attrdict) {
        return -1;
    }

    /* keep this for bw compatibility */
    int r = PyDict_Contains(attrdict, &_Py_ID(_abstract_));
    if (r > 0) {
        Py_DECREF(attrdict);
        return 0;
    }
    if (r < 0) {
        Py_DECREF(attrdict);
        return -1;
    }

    ctypes_state *st = get_module_state_by_def(Py_TYPE(self));
    StgInfo *info = PyStgInfo_Init(st, (PyTypeObject *)self);
    if (!info) {
        Py_DECREF(attrdict);
        return -1;
    }

    info->format = _ctypes_alloc_format_string(NULL, "B");
    if (info->format == NULL) {
        Py_DECREF(attrdict);
        return -1;
    }

    info->paramfunc = StructUnionType_paramfunc;

    if (PyDict_GetItemRef(attrdict, &_Py_ID(_fields_), &fields) < 0) {
        Py_DECREF(attrdict);
        return -1;
    }
    Py_CLEAR(attrdict);
    if (fields) {
        if (PyObject_SetAttr(self, &_Py_ID(_fields_), fields) < 0) {
            Py_DECREF(fields);
            return -1;
        }
        Py_DECREF(fields);
        return 0;
    }
    else {
        StgInfo *baseinfo;
        if (PyStgInfo_FromType(st, (PyObject *)((PyTypeObject *)self)->tp_base,
                               &baseinfo) < 0) {
            return -1;
        }
        if (baseinfo == NULL) {
            return 0;
        }
        int ret = 0;
        STGINFO_LOCK(baseinfo);
        /* copy base info */
        ret = PyCStgInfo_clone(info, baseinfo);
        if (ret >= 0) {
            stginfo_set_dict_final_lock_held(baseinfo); /* set the 'final' flag in the baseclass info */
        }
        STGINFO_UNLOCK();
        return ret;
    }
    return 0;
}

static int
PyCStructType_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return StructUnionType_init(self, args, kwds, 1);
}

static int
UnionType_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return StructUnionType_init(self, args, kwds, 0);
}

/*[clinic input]
class _ctypes.CDataType "PyObject *" "clinic_state()->CType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=466a505a93d73156]*/


/*[clinic input]
_ctypes.CDataType.from_address as CDataType_from_address

    type: self
    cls: defining_class
    value: object
    /

C.from_address(integer) -> C instance

Access a C instance at the specified address.
[clinic start generated code]*/

static PyObject *
CDataType_from_address_impl(PyObject *type, PyTypeObject *cls,
                            PyObject *value)
/*[clinic end generated code: output=5be4a7c0d9aa6c74 input=827a22cefe380c01]*/
{
    void *buf;
    if (!PyLong_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer expected");
        return NULL;
    }
    buf = (void *)PyLong_AsVoidPtr(value);
    if (PyErr_Occurred())
        return NULL;
    ctypes_state *st = get_module_state_by_class(cls);
    return PyCData_AtAddress(st, type, buf);
}

static int
KeepRef(CDataObject *target, Py_ssize_t index, PyObject *keep);

/*[clinic input]
_ctypes.CDataType.from_buffer as CDataType_from_buffer

    type: self
    cls: defining_class
    obj: object
    offset: Py_ssize_t = 0
    /

C.from_buffer(object, offset=0) -> C instance

Create a C instance from a writeable buffer.
[clinic start generated code]*/

static PyObject *
CDataType_from_buffer_impl(PyObject *type, PyTypeObject *cls, PyObject *obj,
                           Py_ssize_t offset)
/*[clinic end generated code: output=57604e99635abd31 input=0f36cedd105ca28d]*/
{
    PyObject *mv;
    PyObject *result;
    Py_buffer *buffer;

    ctypes_state *st = get_module_state_by_class(cls);
    StgInfo *info;
    if (PyStgInfo_FromType(st, type, &info) < 0) {
        return NULL;
    }
    if (!info) {
        PyErr_SetString(PyExc_TypeError, "abstract class");
        return NULL;
    }

    mv = PyMemoryView_FromObject(obj);
    if (mv == NULL)
        return NULL;

    buffer = PyMemoryView_GET_BUFFER(mv);

    if (buffer->readonly) {
        PyErr_SetString(PyExc_TypeError,
            "underlying buffer is not writable");
        Py_DECREF(mv);
        return NULL;
    }

    if (!PyBuffer_IsContiguous(buffer, 'C')) {
        PyErr_SetString(PyExc_TypeError,
            "underlying buffer is not C contiguous");
        Py_DECREF(mv);
        return NULL;
    }

    if (offset < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "offset cannot be negative");
        Py_DECREF(mv);
        return NULL;
    }

    if (info->size > buffer->len - offset) {
        PyErr_Format(PyExc_ValueError,
                     "Buffer size too small "
                     "(%zd instead of at least %zd bytes)",
                     buffer->len, info->size + offset);
        Py_DECREF(mv);
        return NULL;
    }

    if (PySys_Audit("ctypes.cdata/buffer", "nnn",
                    (Py_ssize_t)buffer->buf, buffer->len, offset) < 0) {
        Py_DECREF(mv);
        return NULL;
    }

    result = PyCData_AtAddress(st, type, (char *)buffer->buf + offset);
    if (result == NULL) {
        Py_DECREF(mv);
        return NULL;
    }

    if (-1 == KeepRef((CDataObject *)result, -1, mv)) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

static inline PyObject *
generic_pycdata_new(ctypes_state *st,
                    PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *
GenericPyCData_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

/*[clinic input]
_ctypes.CDataType.from_buffer_copy as CDataType_from_buffer_copy

    type: self
    cls: defining_class
    buffer: Py_buffer
    offset: Py_ssize_t = 0
    /

C.from_buffer_copy(object, offset=0) -> C instance

Create a C instance from a readable buffer.
[clinic start generated code]*/

static PyObject *
CDataType_from_buffer_copy_impl(PyObject *type, PyTypeObject *cls,
                                Py_buffer *buffer, Py_ssize_t offset)
/*[clinic end generated code: output=c8fc62b03e5cc6fa input=2a81e11b765a6253]*/
{
    PyObject *result;

    ctypes_state *st = get_module_state_by_class(cls);
    StgInfo *info;
    if (PyStgInfo_FromType(st, type, &info) < 0) {
        return NULL;
    }
    if (!info) {
        PyErr_SetString(PyExc_TypeError, "abstract class");
        return NULL;
    }

    if (offset < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "offset cannot be negative");
        return NULL;
    }

    if (info->size > buffer->len - offset) {
        PyErr_Format(PyExc_ValueError,
                     "Buffer size too small (%zd instead of at least %zd bytes)",
                     buffer->len, info->size + offset);
        return NULL;
    }

    if (PySys_Audit("ctypes.cdata/buffer", "nnn",
                    (Py_ssize_t)buffer->buf, buffer->len, offset) < 0) {
        return NULL;
    }

    result = generic_pycdata_new(st, (PyTypeObject *)type, NULL, NULL);
    if (result != NULL) {
        locked_memcpy_to((CDataObject *) result, (char *)buffer->buf + offset, info->size);
    }
    return result;
}

/*[clinic input]
_ctypes.CDataType.in_dll as CDataType_in_dll

    type: self
    cls: defining_class
    dll: object
    name: str
    /

C.in_dll(dll, name) -> C instance

Access a C instance in a dll.
[clinic start generated code]*/

static PyObject *
CDataType_in_dll_impl(PyObject *type, PyTypeObject *cls, PyObject *dll,
                      const char *name)
/*[clinic end generated code: output=d0e5c43b66bfa21f input=f85bf281477042b4]*/
{
    PyObject *obj;
    void *handle;
    void *address;

    if (PySys_Audit("ctypes.dlsym", "Os", dll, name) < 0) {
        return NULL;
    }

    obj = PyObject_GetAttrString(dll, "_handle");
    if (!obj)
        return NULL;
    if (!PyLong_Check(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "the _handle attribute of the second argument must be an integer");
        Py_DECREF(obj);
        return NULL;
    }
    handle = (void *)PyLong_AsVoidPtr(obj);
    Py_DECREF(obj);
    if (PyErr_Occurred()) {
        PyErr_SetString(PyExc_ValueError,
                        "could not convert the _handle attribute to a pointer");
        return NULL;
    }

#undef USE_DLERROR
#ifdef MS_WIN32
    Py_BEGIN_ALLOW_THREADS
    address = (void *)GetProcAddress(handle, name);
    Py_END_ALLOW_THREADS
#else
    #ifdef __CYGWIN__
        // dlerror() isn't very helpful on cygwin
    #else
        #define USE_DLERROR
        /* dlerror() always returns the latest error.
         *
         * Clear the previous value before calling dlsym(),
         * to ensure we can tell if our call resulted in an error.
         */
        (void)dlerror();
    #endif
    address = (void *)dlsym(handle, name);
#endif

    if (address) {
        ctypes_state *st = get_module_state_by_def(Py_TYPE(type));
        return PyCData_AtAddress(st, type, address);
    }

    #ifdef USE_DLERROR
    const char *dlerr = dlerror();
    if (dlerr) {
        _PyErr_SetLocaleString(PyExc_ValueError, dlerr);
        return NULL;
    }
    #endif
#undef USE_DLERROR
    PyErr_Format(PyExc_ValueError, "symbol '%s' not found", name);
    return NULL;
}

/*[clinic input]
_ctypes.CDataType.from_param as CDataType_from_param

    type: self
    cls: defining_class
    value: object
    /

Convert a Python object into a function call parameter.
[clinic start generated code]*/

static PyObject *
CDataType_from_param_impl(PyObject *type, PyTypeObject *cls, PyObject *value)
/*[clinic end generated code: output=8da9e34263309f9e input=275a52c4899ddff0]*/
{
    PyObject *as_parameter;
    int res = PyObject_IsInstance(value, type);
    if (res == -1)
        return NULL;
    if (res) {
        return Py_NewRef(value);
    }
    ctypes_state *st = get_module_state_by_class(cls);
    if (PyCArg_CheckExact(st, value)) {
        PyCArgObject *p = (PyCArgObject *)value;
        PyObject *ob = p->obj;
        const char *ob_name;
        StgInfo *info;
        if (PyStgInfo_FromType(st, type, &info) < 0) {
            return NULL;
        }
        /* If we got a PyCArgObject, we must check if the object packed in it
           is an instance of the type's info->proto */
        if(info && ob) {
            res = PyObject_IsInstance(ob, info->proto);
            if (res == -1)
                return NULL;
            if (res) {
                return Py_NewRef(value);
            }
        }
        ob_name = (ob) ? Py_TYPE(ob)->tp_name : "???";
        PyErr_Format(PyExc_TypeError,
                     "expected %s instance instead of pointer to %s",
                     ((PyTypeObject *)type)->tp_name, ob_name);
        return NULL;
    }

    if (PyObject_GetOptionalAttr(value, &_Py_ID(_as_parameter_), &as_parameter) < 0) {
        return NULL;
    }
    if (as_parameter) {
        if (_Py_EnterRecursiveCall(" while processing _as_parameter_")) {
            Py_DECREF(as_parameter);
            return NULL;
        }
        value = CDataType_from_param_impl(type, cls, as_parameter);
        Py_DECREF(as_parameter);
        _Py_LeaveRecursiveCall();
        return value;
    }
    PyErr_Format(PyExc_TypeError,
                 "expected %s instance instead of %s",
                 ((PyTypeObject *)type)->tp_name,
                 Py_TYPE(value)->tp_name);
    return NULL;
}

static PyMethodDef CDataType_methods[] = {
    CDATATYPE_FROM_PARAM_METHODDEF
    CDATATYPE_FROM_ADDRESS_METHODDEF
    CDATATYPE_FROM_BUFFER_METHODDEF
    CDATATYPE_FROM_BUFFER_COPY_METHODDEF
    CDATATYPE_IN_DLL_METHODDEF
    { NULL, NULL },
};

static PyObject *
CType_Type_repeat(PyObject *self, Py_ssize_t length)
{
    if (length < 0)
        return PyErr_Format(PyExc_ValueError,
                            "Array length must be >= 0, not %zd",
                            length);
    ctypes_state *st = get_module_state_by_def(Py_TYPE(self));
    return PyCArrayType_from_ctype(st, self, length);
}

static int
_structunion_setattro(PyObject *self, PyObject *key, PyObject *value, int is_struct)
{
    /* XXX Should we disallow deleting _fields_? */
    if (PyUnicode_Check(key)
            && _PyUnicode_EqualToASCIIString(key, "_fields_"))
    {
        if (PyCStructUnionType_update_stginfo(self, value, is_struct) < 0) {
            return -1;
        }
    }

    return PyType_Type.tp_setattro(self, key, value);
}

static int
PyCStructType_setattro(PyObject *self, PyObject *key, PyObject *value)
{
    return _structunion_setattro(self, key, value, 1);
}

static int
UnionType_setattro(PyObject *self, PyObject *key, PyObject *value)
{
    return _structunion_setattro(self, key, value, 0);
}

static PyType_Slot pycstruct_type_slots[] = {
    {Py_tp_setattro, PyCStructType_setattro},
    {Py_tp_doc, PyDoc_STR("metatype for the CData Objects")},
    {Py_tp_methods, CDataType_methods},
    {Py_tp_init, PyCStructType_init},
    {0, NULL},
};

static PyType_Spec pycstruct_type_spec = {
    .name = "_ctypes.PyCStructType",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pycstruct_type_slots,
};

static PyType_Slot union_type_slots[] = {
    {Py_tp_setattro, UnionType_setattro},
    {Py_tp_doc, PyDoc_STR("metatype for the Union Objects")},
    {Py_tp_methods, CDataType_methods},
    {Py_tp_init, UnionType_init},
    {0, NULL},
};

static PyType_Spec union_type_spec = {
    .name = "_ctypes.UnionType",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = union_type_slots,
};

/******************************************************************/

/*

The PyCPointerType_Type metaclass must ensure that the subclass of Pointer can be
created. It must check for a _type_ attribute in the class. Since are no
runtime created properties, a CField is probably *not* needed ?

class IntPointer(Pointer):
    _type_ = "i"

The PyCPointer_Type provides the functionality: a contents method/property, a
size property/method, and the sequence protocol.

*/

/*[clinic input]
class _ctypes.PyCPointerType "PyObject *" "clinic_state()->PyCPointerType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=c45e96c1f7645ab7]*/


static int
PyCPointerType_SetProto(ctypes_state *st, StgInfo *stginfo, PyObject *proto)
{
    if (!proto || !PyType_Check(proto)) {
        PyErr_SetString(PyExc_TypeError,
                        "_type_ must be a type");
        return -1;
    }
    StgInfo *info;
    if (PyStgInfo_FromType(st, proto, &info) < 0) {
        return -1;
    }
    if (!info) {
        PyErr_SetString(PyExc_TypeError,
                        "_type_ must have storage info");
        return -1;
    }
    Py_INCREF(proto);
    Py_XSETREF(stginfo->proto, proto);
    return 0;
}

static PyCArgObject *
PyCPointerType_paramfunc(ctypes_state *st, CDataObject *self)
{
    PyCArgObject *parg;

    parg = PyCArgObject_new(st);
    if (parg == NULL)
        return NULL;

    parg->tag = 'P';
    parg->pffi_type = &ffi_type_pointer;
    parg->obj = Py_NewRef(self);
    parg->value.p = locked_deref(self);
    return parg;
}

static int
PyCPointerType_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *proto;
    PyObject *typedict;

    typedict = PyTuple_GetItem(args, 2);
    if (!typedict) {
        return -1;
    }

/*
  stginfo items size, align, length contain info about pointers itself,
  stginfo->proto has info about the pointed to type!
*/
    ctypes_state *st = get_module_state_by_def(Py_TYPE(self));
    StgInfo *stginfo = PyStgInfo_Init(st, (PyTypeObject *)self);
    if (!stginfo) {
        return -1;
    }
    stginfo->size = sizeof(void *);
    stginfo->align = _ctypes_get_fielddesc("P")->pffi_type->alignment;
    stginfo->length = 1;
    stginfo->ffi_type_pointer = ffi_type_pointer;
    stginfo->paramfunc = PyCPointerType_paramfunc;
    stginfo->flags |= TYPEFLAG_ISPOINTER;

    if (PyDict_GetItemRef(typedict, &_Py_ID(_type_), &proto) < 0) {
        return -1;
    }
    if (proto) {
        const char *current_format;
        if (PyCPointerType_SetProto(st, stginfo, proto) < 0) {
            Py_DECREF(proto);
            return -1;
        }
        StgInfo *iteminfo;
        if (PyStgInfo_FromType(st, proto, &iteminfo) < 0) {
            Py_DECREF(proto);
            return -1;
        }
        /* PyCPointerType_SetProto has verified proto has a stginfo. */
        assert(iteminfo);
        /* If iteminfo->format is NULL, then this is a pointer to an
           incomplete type.  We create a generic format string
           'pointer to bytes' in this case.  XXX Better would be to
           fix the format string later...
        */
        current_format = iteminfo->format ? iteminfo->format : "B";
        if (iteminfo->shape != NULL) {
            /* pointer to an array: the shape needs to be prefixed */
            stginfo->format = _ctypes_alloc_format_string_with_shape(
                iteminfo->ndim, iteminfo->shape, "&", current_format);
        } else {
            stginfo->format = _ctypes_alloc_format_string("&", current_format);
        }
        Py_DECREF(proto);
        if (stginfo->format == NULL) {
            return -1;
        }
    }

    return 0;
}

/*[clinic input]
_ctypes.PyCPointerType.set_type as PyCPointerType_set_type

    self: self(type="PyTypeObject *")
    cls: defining_class
    type: object
    /
[clinic start generated code]*/

static PyObject *
PyCPointerType_set_type_impl(PyTypeObject *self, PyTypeObject *cls,
                             PyObject *type)
/*[clinic end generated code: output=51459d8f429a70ac input=67e1e8df921f123e]*/
{
    PyObject *attrdict = PyType_GetDict(self);
    if (!attrdict) {
        return NULL;
    }
    ctypes_state *st = get_module_state_by_class(cls);
    StgInfo *info;
    if (PyStgInfo_FromType(st, (PyObject *)self, &info) < 0) {
        Py_DECREF(attrdict);
        return NULL;
    }
    if (!info) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        Py_DECREF(attrdict);
        return NULL;
    }

    if (PyCPointerType_SetProto(st, info, type) < 0) {
        Py_DECREF(attrdict);
        return NULL;
    }

    if (-1 == PyDict_SetItem(attrdict, &_Py_ID(_type_), type)) {
        Py_DECREF(attrdict);
        return NULL;
    }

    Py_DECREF(attrdict);
    Py_RETURN_NONE;
}

static PyObject *_byref(ctypes_state *, PyObject *);

/*[clinic input]
_ctypes.PyCPointerType.from_param as PyCPointerType_from_param

    type: self
    cls: defining_class
    value: object
    /

Convert a Python object into a function call parameter.
[clinic start generated code]*/

static PyObject *
PyCPointerType_from_param_impl(PyObject *type, PyTypeObject *cls,
                               PyObject *value)
/*[clinic end generated code: output=a4b32d929aabaf64 input=6c231276e3997884]*/
{
    if (value == Py_None) {
        /* ConvParam will convert to a NULL pointer later */
        return Py_NewRef(value);
    }

    ctypes_state *st = get_module_state_by_class(cls);
    StgInfo *typeinfo;
    if (PyStgInfo_FromType(st, type, &typeinfo) < 0) {
        return NULL;
    }
    if (!typeinfo) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return NULL;
    }

    /* If we expect POINTER(<type>), but receive a <type> instance, accept
       it by calling byref(<type>).
    */
    assert(typeinfo->proto);
    switch (PyObject_IsInstance(value, typeinfo->proto)) {
    case 1:
        Py_INCREF(value); /* _byref steals a refcount */
        return _byref(st, value);
    case -1:
        return NULL;
    default:
        break;
    }

    if (PointerObject_Check(st, value) || ArrayObject_Check(st, value)) {
        /* Array instances are also pointers when
           the item types are the same.
        */
        StgInfo *v;
        if (PyStgInfo_FromObject(st, value, &v) < 0) {
            return NULL;
        }
        assert(v); /* Cannot be NULL for pointer or array objects */
        int ret = PyObject_IsSubclass(v->proto, typeinfo->proto);
        if (ret < 0) {
            return NULL;
        }
        if (ret) {
            return Py_NewRef(value);
        }
    }
    return CDataType_from_param_impl(type, cls, value);
}

static PyMethodDef PyCPointerType_methods[] = {
    CDATATYPE_FROM_ADDRESS_METHODDEF
    CDATATYPE_FROM_BUFFER_METHODDEF
    CDATATYPE_FROM_BUFFER_COPY_METHODDEF
    CDATATYPE_IN_DLL_METHODDEF
    PYCPOINTERTYPE_FROM_PARAM_METHODDEF
    PYCPOINTERTYPE_SET_TYPE_METHODDEF
    { NULL, NULL },
};

static PyType_Slot pycpointer_type_slots[] = {
    {Py_tp_doc, PyDoc_STR("metatype for the Pointer Objects")},
    {Py_tp_methods, PyCPointerType_methods},
    {Py_tp_init, PyCPointerType_init},
    {0, NULL},
};

static PyType_Spec pycpointer_type_spec = {
    .name = "_ctypes.PyCPointerType",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pycpointer_type_slots,
};


/******************************************************************/
/*
  PyCArrayType_Type
*/
/*
  PyCArrayType_init ensures that the new Array subclass created has a _length_
  attribute, and a _type_ attribute.
*/
/*[clinic input]
class _ctypes.PyCArrayType_Type "CDataObject *" "clinic_state()->PyCArrayType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6340cbaead1bf3f3]*/

/*[clinic input]
@critical_section
@setter
_ctypes.PyCArrayType_Type.raw
[clinic start generated code]*/

static int
_ctypes_PyCArrayType_Type_raw_set_impl(CDataObject *self, PyObject *value)
/*[clinic end generated code: output=cf9b2a9fd92e9ecb input=a3717561efc45efd]*/
{
    char *ptr;
    Py_ssize_t size;
    Py_buffer view;

    if (value == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete attribute");
        return -1;
    }
    if (PyObject_GetBuffer(value, &view, PyBUF_SIMPLE) < 0)
        return -1;
    size = view.len;
    ptr = view.buf;
    if (size > self->b_size) {
        PyErr_SetString(PyExc_ValueError,
                        "byte string too long");
        goto fail;
    }

    locked_memcpy_to(self, ptr, size);

    PyBuffer_Release(&view);
    return 0;
 fail:
    PyBuffer_Release(&view);
    return -1;
}

/*[clinic input]
@critical_section
@getter
_ctypes.PyCArrayType_Type.raw
[clinic start generated code]*/

static PyObject *
_ctypes_PyCArrayType_Type_raw_get_impl(CDataObject *self)
/*[clinic end generated code: output=3a90be6f43764e31 input=4c49bbb715235ba7]*/
{
    return PyBytes_FromStringAndSize(self->b_ptr, self->b_size);
}

/*[clinic input]
@critical_section
@getter
_ctypes.PyCArrayType_Type.value
[clinic start generated code]*/

static PyObject *
_ctypes_PyCArrayType_Type_value_get_impl(CDataObject *self)
/*[clinic end generated code: output=fb0636f4d8875483 input=2432a2aeb1ed78d1]*/
{
    Py_ssize_t i;
    PyObject *res;
    char *ptr = self->b_ptr;
    for (i = 0; i < self->b_size; ++i)
        if (*ptr++ == '\0')
            break;
    res = PyBytes_FromStringAndSize(self->b_ptr, i);
    return res;
}

/*[clinic input]
@critical_section
@setter
_ctypes.PyCArrayType_Type.value
[clinic start generated code]*/

static int
_ctypes_PyCArrayType_Type_value_set_impl(CDataObject *self, PyObject *value)
/*[clinic end generated code: output=39ad655636a28dd5 input=e2e6385fc6ab1a29]*/
{
    const char *ptr;
    Py_ssize_t size;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "can't delete attribute");
        return -1;
    }

    if (!PyBytes_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "bytes expected instead of %s instance",
                     Py_TYPE(value)->tp_name);
        return -1;
    } else
        Py_INCREF(value);
    size = PyBytes_GET_SIZE(value);
    if (size > self->b_size) {
        PyErr_SetString(PyExc_ValueError,
                        "byte string too long");
        Py_DECREF(value);
        return -1;
    }

    ptr = PyBytes_AS_STRING(value);
    memcpy(self->b_ptr, ptr, size);
    if (size < self->b_size)
        self->b_ptr[size] = '\0';
    Py_DECREF(value);

    return 0;
}

static PyGetSetDef CharArray_getsets[] = {
    _CTYPES_PYCARRAYTYPE_TYPE_RAW_GETSETDEF
    _CTYPES_PYCARRAYTYPE_TYPE_VALUE_GETSETDEF
    { NULL, NULL }
};

static PyObject *
WCharArray_get_value_lock_held(PyObject *op)
{
    Py_ssize_t i;
    PyObject *res;
    CDataObject *self = _CDataObject_CAST(op);
    wchar_t *ptr = (wchar_t *)self->b_ptr;
    for (i = 0; i < self->b_size/(Py_ssize_t)sizeof(wchar_t); ++i)
        if (*ptr++ == (wchar_t)0)
            break;
    res = PyUnicode_FromWideChar((wchar_t *)self->b_ptr, i);
    return res;
}

static PyObject *
WCharArray_get_value(PyObject *op, void *Py_UNUSED(ignored))
{
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(op);
    res = WCharArray_get_value_lock_held(op);
    Py_END_CRITICAL_SECTION();
    return res;
}

static int
WCharArray_set_value_lock_held(PyObject *op, PyObject *value)
{
    CDataObject *self = _CDataObject_CAST(op);

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "can't delete attribute");
        return -1;
    }
    if (!PyUnicode_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                        "unicode string expected instead of %s instance",
                        Py_TYPE(value)->tp_name);
        return -1;
    }

    Py_ssize_t size = self->b_size / sizeof(wchar_t);
    Py_ssize_t len = PyUnicode_AsWideChar(value, NULL, 0);
    if (len < 0) {
        return -1;
    }
    // PyUnicode_AsWideChar() returns number of wchars including trailing null byte,
    // when it is called with NULL.
    assert(len > 0);
    if (len - 1 > size) {
        PyErr_SetString(PyExc_ValueError, "string too long");
        return -1;
    }
    Py_ssize_t rc;
    rc = PyUnicode_AsWideChar(value, (wchar_t *)self->b_ptr, size);
    return rc < 0 ? -1 : 0;
}

static int
WCharArray_set_value(PyObject *op, PyObject *value, void *Py_UNUSED(ignored))
{
    int rc;
    Py_BEGIN_CRITICAL_SECTION(op);
    rc = WCharArray_set_value_lock_held(op, value);
    Py_END_CRITICAL_SECTION();
    return rc;
}

static PyGetSetDef WCharArray_getsets[] = {
    { "value", WCharArray_get_value, WCharArray_set_value, "string value" },
    { NULL, NULL }
};

/*
  The next function is copied from Python's typeobject.c.

  It is used to attach getsets to a type *after* it
  has been created: Arrays of characters have additional getsets to treat them
  as strings.
 */

static int
add_getset(PyTypeObject *type, PyGetSetDef *gsp)
{
    PyObject *dict = type->tp_dict;
    for (; gsp->name != NULL; gsp++) {
        PyObject *descr;
        descr = PyDescr_NewGetSet(type, gsp);
        if (descr == NULL)
            return -1;
        if (PyDict_SetItemString(dict, gsp->name, descr) < 0) {
            Py_DECREF(descr);
            return -1;
        }
        Py_DECREF(descr);
    }
    return 0;
}

static PyCArgObject *
PyCArrayType_paramfunc(ctypes_state *st, CDataObject *self)
{
    PyCArgObject *p = PyCArgObject_new(st);
    if (p == NULL)
        return NULL;
    p->tag = 'P';
    p->pffi_type = &ffi_type_pointer;
    p->value.p = (char *)self->b_ptr;
    p->obj = Py_NewRef(self);
    return p;
}

static int
PyCArrayType_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *length_attr, *type_attr;
    Py_ssize_t length;
    Py_ssize_t itemsize, itemalign;

    /* Initialize these variables to NULL so that we can simplify error
       handling by using Py_XDECREF.  */
    type_attr = NULL;

    if (PyObject_GetOptionalAttr(self, &_Py_ID(_length_), &length_attr) < 0) {
        goto error;
    }
    if (!length_attr) {
        PyErr_SetString(PyExc_AttributeError,
                        "class must define a '_length_' attribute");
        goto error;
    }

    if (!PyLong_Check(length_attr)) {
        Py_DECREF(length_attr);
        PyErr_SetString(PyExc_TypeError,
                        "The '_length_' attribute must be an integer");
        goto error;
    }

    if (_PyLong_IsNegative((PyLongObject *)length_attr)) {
        Py_DECREF(length_attr);
        PyErr_SetString(PyExc_ValueError,
                        "The '_length_' attribute must not be negative");
        goto error;
    }

    length = PyLong_AsSsize_t(length_attr);
    Py_DECREF(length_attr);
    if (length == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            PyErr_SetString(PyExc_OverflowError,
                            "The '_length_' attribute is too large");
        }
        goto error;
    }

    if (PyObject_GetOptionalAttr(self, &_Py_ID(_type_), &type_attr) < 0) {
        goto error;
    }
    if (!type_attr) {
        PyErr_SetString(PyExc_AttributeError,
                        "class must define a '_type_' attribute");
        goto error;
    }

    ctypes_state *st = get_module_state_by_def(Py_TYPE(self));
    StgInfo *stginfo = PyStgInfo_Init(st, (PyTypeObject*)self);
    if (!stginfo) {
        goto error;
    }

    StgInfo *iteminfo;
    if (PyStgInfo_FromType(st, type_attr, &iteminfo) < 0) {
        goto error;
    }
    if (!iteminfo) {
        PyErr_SetString(PyExc_TypeError,
                        "_type_ must have storage info");
        goto error;
    }

    assert(iteminfo->format);
    stginfo->format = _ctypes_alloc_format_string(NULL, iteminfo->format);
    if (stginfo->format == NULL)
        goto error;
    stginfo->ndim = iteminfo->ndim + 1;
    stginfo->shape = PyMem_Malloc(sizeof(Py_ssize_t) * stginfo->ndim);
    if (stginfo->shape == NULL) {
        PyErr_NoMemory();
        goto error;
    }
    stginfo->shape[0] = length;
    if (stginfo->ndim > 1) {
        memmove(&stginfo->shape[1], iteminfo->shape,
            sizeof(Py_ssize_t) * (stginfo->ndim - 1));
    }

    itemsize = iteminfo->size;
    if (itemsize != 0 && length > PY_SSIZE_T_MAX / itemsize) {
        PyErr_SetString(PyExc_OverflowError,
                        "array too large");
        goto error;
    }

    itemalign = iteminfo->align;

    if (iteminfo->flags & (TYPEFLAG_ISPOINTER | TYPEFLAG_HASPOINTER))
        stginfo->flags |= TYPEFLAG_HASPOINTER;

    stginfo->size = itemsize * length;
    stginfo->align = itemalign;
    stginfo->length = length;
    stginfo->proto = type_attr;
    type_attr = NULL;

    stginfo->paramfunc = &PyCArrayType_paramfunc;

    /* Arrays are passed as pointers to function calls. */
    stginfo->ffi_type_pointer = ffi_type_pointer;

    /* Special case for character arrays.
       A permanent annoyance: char arrays are also strings!
    */
    if (iteminfo->getfunc == _ctypes_get_fielddesc("c")->getfunc) {
        if (-1 == add_getset((PyTypeObject*)self, CharArray_getsets))
            goto error;
    }
    else if (iteminfo->getfunc == _ctypes_get_fielddesc("u")->getfunc) {
        if (-1 == add_getset((PyTypeObject*)self, WCharArray_getsets))
            goto error;
    }

    return 0;
error:
    Py_XDECREF(type_attr);
    return -1;
}

static PyType_Slot pycarray_type_slots[] = {
    {Py_tp_doc, PyDoc_STR("metatype for the Array Objects")},
    {Py_tp_methods, CDataType_methods},
    {Py_tp_init, PyCArrayType_init},
    {0, NULL},
};

static PyType_Spec pycarray_type_spec = {
    .name = "_ctypes.PyCArrayType",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pycarray_type_slots,
};

/******************************************************************/
/*
  PyCSimpleType_Type
*/
/*

PyCSimpleType_init ensures that the new Simple_Type subclass created has a valid
_type_ attribute.

*/

/*[clinic input]
class _ctypes.PyCSimpleType "PyObject *" "clinic_state()->PyCSimpleType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=d5a45772668e7f49]*/

/*[clinic input]
class _ctypes.c_wchar_p "PyObject *" "clinic_state_sub()->PyCSimpleType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=468de7283d622d47]*/

/*[clinic input]
class _ctypes.c_char_p "PyObject *" "clinic_state_sub()->PyCSimpleType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e750865616e7dcea]*/

/*[clinic input]
class _ctypes.c_void_p "PyObject *" "clinic_state_sub()->PyCSimpleType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=dd4d9646c56f43a9]*/


/*[clinic input]
_ctypes.c_wchar_p.from_param as c_wchar_p_from_param

    type: self
    cls: defining_class
    value: object
    /
[clinic start generated code]*/

static PyObject *
c_wchar_p_from_param_impl(PyObject *type, PyTypeObject *cls, PyObject *value)
/*[clinic end generated code: output=e453949a2f725a4c input=d322c7237a319607]*/
{
    PyObject *as_parameter;
    int res;
    if (value == Py_None) {
        Py_RETURN_NONE;
    }
    ctypes_state *st = get_module_state_by_class(cls->tp_base);
    if (PyUnicode_Check(value)) {
        PyCArgObject *parg;
        struct fielddesc *fd = _ctypes_get_fielddesc("Z");

        parg = PyCArgObject_new(st);
        if (parg == NULL)
            return NULL;
        parg->pffi_type = &ffi_type_pointer;
        parg->tag = 'Z';
        parg->obj = fd->setfunc(&parg->value, value, 0);
        if (parg->obj == NULL) {
            Py_DECREF(parg);
            return NULL;
        }
        return (PyObject *)parg;
    }
    res = PyObject_IsInstance(value, type);
    if (res == -1)
        return NULL;
    if (res) {
        return Py_NewRef(value);
    }
    if (ArrayObject_Check(st, value) || PointerObject_Check(st, value)) {
        /* c_wchar array instance or pointer(c_wchar(...)) */
        StgInfo *it;
        if (PyStgInfo_FromObject(st, value, &it) < 0) {
            return NULL;
        }
        assert(it); /* Cannot be NULL for pointer or array objects */
        StgInfo *info = NULL;
        if (it && it->proto) {
            if (PyStgInfo_FromType(st, it->proto, &info) < 0) {
                return NULL;
            }
        }
        if (info && (info->setfunc == _ctypes_get_fielddesc("u")->setfunc)) {
            return Py_NewRef(value);
        }
    }
    if (PyCArg_CheckExact(st, value)) {
        /* byref(c_char(...)) */
        PyCArgObject *a = (PyCArgObject *)value;
        StgInfo *info;
        if (PyStgInfo_FromObject(st, a->obj, &info) < 0) {
            return NULL;
        }
        if (info && (info->setfunc == _ctypes_get_fielddesc("u")->setfunc)) {
            return Py_NewRef(value);
        }
    }

    if (PyObject_GetOptionalAttr(value, &_Py_ID(_as_parameter_), &as_parameter) < 0) {
        return NULL;
    }
    if (as_parameter) {
        if (_Py_EnterRecursiveCall(" while processing _as_parameter_")) {
            Py_DECREF(as_parameter);
            return NULL;
        }
        value = c_wchar_p_from_param_impl(type, cls, as_parameter);
        Py_DECREF(as_parameter);
        _Py_LeaveRecursiveCall();
        return value;
    }
    PyErr_Format(PyExc_TypeError,
                 "'%.200s' object cannot be interpreted "
                 "as ctypes.c_wchar_p", Py_TYPE(value)->tp_name);
    return NULL;
}

/*[clinic input]
_ctypes.c_char_p.from_param as c_char_p_from_param

    type: self
    cls: defining_class
    value: object
    /
[clinic start generated code]*/

static PyObject *
c_char_p_from_param_impl(PyObject *type, PyTypeObject *cls, PyObject *value)
/*[clinic end generated code: output=219652ab7c174aa1 input=6cf0d1b6bb4ede11]*/
{
    PyObject *as_parameter;
    int res;
    if (value == Py_None) {
        Py_RETURN_NONE;
    }
    ctypes_state *st = get_module_state_by_class(cls->tp_base);
    if (PyBytes_Check(value)) {
        PyCArgObject *parg;
        struct fielddesc *fd = _ctypes_get_fielddesc("z");

        parg = PyCArgObject_new(st);
        if (parg == NULL)
            return NULL;
        parg->pffi_type = &ffi_type_pointer;
        parg->tag = 'z';
        parg->obj = fd->setfunc(&parg->value, value, 0);
        if (parg->obj == NULL) {
            Py_DECREF(parg);
            return NULL;
        }
        return (PyObject *)parg;
    }
    res = PyObject_IsInstance(value, type);
    if (res == -1)
        return NULL;
    if (res) {
        return Py_NewRef(value);
    }
    if (ArrayObject_Check(st, value) || PointerObject_Check(st, value)) {
        /* c_char array instance or pointer(c_char(...)) */
        StgInfo *it;
        if (PyStgInfo_FromObject(st, value, &it) < 0) {
            return NULL;
        }
        assert(it); /* Cannot be NULL for pointer or array objects */
        StgInfo *info = NULL;
        if (it && it->proto) {
            if (PyStgInfo_FromType(st, it->proto, &info) < 0) {
                return NULL;
            }
        }
        if (info && (info->setfunc == _ctypes_get_fielddesc("c")->setfunc)) {
            return Py_NewRef(value);
        }
    }
    if (PyCArg_CheckExact(st, value)) {
        /* byref(c_char(...)) */
        PyCArgObject *a = (PyCArgObject *)value;
        StgInfo *info;
        if (PyStgInfo_FromObject(st, a->obj, &info) < 0) {
            return NULL;
        }
        if (info && (info->setfunc == _ctypes_get_fielddesc("c")->setfunc)) {
            return Py_NewRef(value);
        }
    }

    if (PyObject_GetOptionalAttr(value, &_Py_ID(_as_parameter_), &as_parameter) < 0) {
        return NULL;
    }
    if (as_parameter) {
        if (_Py_EnterRecursiveCall(" while processing _as_parameter_")) {
            Py_DECREF(as_parameter);
            return NULL;
        }
        value = c_char_p_from_param_impl(type, cls, as_parameter);
        Py_DECREF(as_parameter);
        _Py_LeaveRecursiveCall();
        return value;
    }
    PyErr_Format(PyExc_TypeError,
                 "'%.200s' object cannot be interpreted "
                 "as ctypes.c_char_p", Py_TYPE(value)->tp_name);
    return NULL;
}

/*[clinic input]
_ctypes.c_void_p.from_param as c_void_p_from_param

    type: self
    cls: defining_class
    value: object
    /
[clinic start generated code]*/

static PyObject *
c_void_p_from_param_impl(PyObject *type, PyTypeObject *cls, PyObject *value)
/*[clinic end generated code: output=984d0075b6038cc7 input=0e8b343fc19c77d4]*/
{
    PyObject *as_parameter;
    int res;

/* None */
    if (value == Py_None) {
        Py_RETURN_NONE;
    }
    ctypes_state *st = get_module_state_by_class(cls->tp_base);

    /* Should probably allow buffer interface as well */
/* int, long */
    if (PyLong_Check(value)) {
        PyCArgObject *parg;
        struct fielddesc *fd = _ctypes_get_fielddesc("P");

        parg = PyCArgObject_new(st);
        if (parg == NULL)
            return NULL;
        parg->pffi_type = &ffi_type_pointer;
        parg->tag = 'P';
        parg->obj = fd->setfunc(&parg->value, value, sizeof(void*));
        if (parg->obj == NULL) {
            Py_DECREF(parg);
            return NULL;
        }
        return (PyObject *)parg;
    }
    /* XXX struni: remove later */
/* bytes */
    if (PyBytes_Check(value)) {
        PyCArgObject *parg;
        struct fielddesc *fd = _ctypes_get_fielddesc("z");

        parg = PyCArgObject_new(st);
        if (parg == NULL)
            return NULL;
        parg->pffi_type = &ffi_type_pointer;
        parg->tag = 'z';
        parg->obj = fd->setfunc(&parg->value, value, 0);
        if (parg->obj == NULL) {
            Py_DECREF(parg);
            return NULL;
        }
        return (PyObject *)parg;
    }
/* unicode */
    if (PyUnicode_Check(value)) {
        PyCArgObject *parg;
        struct fielddesc *fd = _ctypes_get_fielddesc("Z");

        parg = PyCArgObject_new(st);
        if (parg == NULL)
            return NULL;
        parg->pffi_type = &ffi_type_pointer;
        parg->tag = 'Z';
        parg->obj = fd->setfunc(&parg->value, value, 0);
        if (parg->obj == NULL) {
            Py_DECREF(parg);
            return NULL;
        }
        return (PyObject *)parg;
    }
/* c_void_p instance (or subclass) */
    res = PyObject_IsInstance(value, type);
    if (res == -1)
        return NULL;
    if (res) {
        /* c_void_p instances */
        return Py_NewRef(value);
    }
/* ctypes array or pointer instance */
    if (ArrayObject_Check(st, value) || PointerObject_Check(st, value)) {
        /* Any array or pointer is accepted */
        return Py_NewRef(value);
    }
/* byref(...) */
    if (PyCArg_CheckExact(st, value)) {
        /* byref(c_xxx()) */
        PyCArgObject *a = (PyCArgObject *)value;
        if (a->tag == 'P') {
            return Py_NewRef(value);
        }
    }
/* function pointer */
    if (PyCFuncPtrObject_Check(st, value)) {
        PyCArgObject *parg;
        PyCFuncPtrObject *func;
        func = (PyCFuncPtrObject *)value;
        parg = PyCArgObject_new(st);
        if (parg == NULL)
            return NULL;
        parg->pffi_type = &ffi_type_pointer;
        parg->tag = 'P';
        Py_INCREF(value);
        // Function pointers don't change their contents, no need to lock
        parg->value.p = *(void **)func->b_ptr;
        parg->obj = value;
        return (PyObject *)parg;
    }
/* c_char_p, c_wchar_p */
    StgInfo *stgi;
    if (PyStgInfo_FromObject(st, value, &stgi) < 0) {
        return NULL;
    }
    if (stgi
        && CDataObject_Check(st, value)
        && stgi->proto
        && PyUnicode_Check(stgi->proto))
    {
        PyCArgObject *parg;

        switch (PyUnicode_AsUTF8(stgi->proto)[0]) {
        case 'z': /* c_char_p */
        case 'Z': /* c_wchar_p */
            parg = PyCArgObject_new(st);
            if (parg == NULL)
                return NULL;
            parg->pffi_type = &ffi_type_pointer;
            parg->tag = 'Z';
            parg->obj = Py_NewRef(value);
            /* Remember: b_ptr points to where the pointer is stored! */
            parg->value.p = locked_deref((CDataObject *)value);
            return (PyObject *)parg;
        }
    }

    if (PyObject_GetOptionalAttr(value, &_Py_ID(_as_parameter_), &as_parameter) < 0) {
        return NULL;
    }
    if (as_parameter) {
        if (_Py_EnterRecursiveCall(" while processing _as_parameter_")) {
            Py_DECREF(as_parameter);
            return NULL;
        }
        value = c_void_p_from_param_impl(type, cls, as_parameter);
        Py_DECREF(as_parameter);
        _Py_LeaveRecursiveCall();
        return value;
    }
    PyErr_Format(PyExc_TypeError,
                 "'%.200s' object cannot be interpreted "
                 "as ctypes.c_void_p", Py_TYPE(value)->tp_name);
    return NULL;
}

static PyMethodDef c_void_p_methods[] = {C_VOID_P_FROM_PARAM_METHODDEF {0}};
static PyMethodDef c_char_p_methods[] = {C_CHAR_P_FROM_PARAM_METHODDEF {0}};
static PyMethodDef c_wchar_p_methods[] = {C_WCHAR_P_FROM_PARAM_METHODDEF {0}};

static PyObject *CreateSwappedType(ctypes_state *st, PyTypeObject *type,
                                   PyObject *args, PyObject *kwds,
                                   PyObject *proto, struct fielddesc *fmt)
{
    PyTypeObject *result;
    PyObject *name = PyTuple_GET_ITEM(args, 0);
    PyObject *newname;
    PyObject *swapped_args;
    Py_ssize_t i;

    swapped_args = PyTuple_New(PyTuple_GET_SIZE(args));
    if (!swapped_args)
        return NULL;

    assert(st->swapped_suffix != NULL);
    newname = PyUnicode_Concat(name, st->swapped_suffix);
    if (newname == NULL) {
        Py_DECREF(swapped_args);
        return NULL;
    }

    PyTuple_SET_ITEM(swapped_args, 0, newname);
    for (i=1; i<PyTuple_GET_SIZE(args); ++i) {
        PyObject *v = PyTuple_GET_ITEM(args, i);
        Py_INCREF(v);
        PyTuple_SET_ITEM(swapped_args, i, v);
    }

    /* create the new instance (which is a class,
       since we are a metatype!) */
    result = (PyTypeObject *)PyType_Type.tp_new(type, swapped_args, kwds);
    Py_DECREF(swapped_args);
    if (result == NULL)
        return NULL;

    StgInfo *stginfo = PyStgInfo_Init(st, result);
    if (!stginfo) {
        Py_DECREF(result);
        return NULL;
    }

    stginfo->ffi_type_pointer = *fmt->pffi_type;
    stginfo->align = fmt->pffi_type->alignment;
    stginfo->length = 0;
    stginfo->size = fmt->pffi_type->size;
    stginfo->setfunc = fmt->setfunc_swapped;
    stginfo->getfunc = fmt->getfunc_swapped;

    stginfo->proto = Py_NewRef(proto);

    return (PyObject *)result;
}

static PyCArgObject *
PyCSimpleType_paramfunc(ctypes_state *st, CDataObject *self)
{
    const char *fmt;
    PyCArgObject *parg;
    struct fielddesc *fd;

    StgInfo *info;
    if (PyStgInfo_FromObject(st, (PyObject *)self, &info) < 0) {
        return NULL;
    }
    assert(info); /* Cannot be NULL for CDataObject instances */
    fmt = PyUnicode_AsUTF8(info->proto);
    assert(fmt);

    fd = _ctypes_get_fielddesc(fmt);
    assert(fd);

    parg = PyCArgObject_new(st);
    if (parg == NULL)
        return NULL;

    parg->tag = fmt[0];
    parg->pffi_type = fd->pffi_type;
    parg->obj = Py_NewRef(self);
    locked_memcpy_from(&parg->value, self, self->b_size);
    return parg;
}

static int
PyCSimpleType_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *proto;
    const char *proto_str;
    Py_ssize_t proto_len;
    PyMethodDef *ml;
    struct fielddesc *fmt;

    if (PyType_Type.tp_init(self, args, kwds) < 0) {
        return -1;
    }
    if (PyObject_GetOptionalAttr(self, &_Py_ID(_type_), &proto) < 0) {
        return -1;
    }
    if (!proto) {
        PyErr_SetString(PyExc_AttributeError,
                        "class must define a '_type_' attribute");
  error:
        Py_XDECREF(proto);
        return -1;
    }
    if (PyUnicode_Check(proto)) {
        proto_str = PyUnicode_AsUTF8AndSize(proto, &proto_len);
        if (!proto_str)
            goto error;
    } else {
        PyErr_SetString(PyExc_TypeError,
            "class must define a '_type_' string attribute");
        goto error;
    }
    if (proto_len != 1) {
        PyErr_SetString(PyExc_ValueError,
                        "class must define a '_type_' attribute "
                        "which must be a string of length 1");
        goto error;
    }
    fmt = _ctypes_get_fielddesc(proto_str);
    if (!fmt) {
        PyErr_Format(PyExc_AttributeError,
                     "class must define a '_type_' attribute which must be\n"
                     "a single character string containing one of the\n"
                     "supported types: '%s'.",
                     _ctypes_get_simple_type_chars());
        goto error;
    }

    ctypes_state *st = get_module_state_by_def(Py_TYPE(self));
    StgInfo *stginfo = PyStgInfo_Init(st, (PyTypeObject *)self);
    if (!stginfo) {
        goto error;
    }

    if (!fmt->pffi_type->elements) {
        stginfo->ffi_type_pointer = *fmt->pffi_type;
    }
    else {
        const size_t els_size = sizeof(fmt->pffi_type->elements);
        stginfo->ffi_type_pointer.size = fmt->pffi_type->size;
        stginfo->ffi_type_pointer.alignment = fmt->pffi_type->alignment;
        stginfo->ffi_type_pointer.type = fmt->pffi_type->type;
        stginfo->ffi_type_pointer.elements = PyMem_Malloc(els_size);
        memcpy(stginfo->ffi_type_pointer.elements,
               fmt->pffi_type->elements, els_size);
    }
    stginfo->align = fmt->pffi_type->alignment;
    stginfo->length = 0;
    stginfo->size = fmt->pffi_type->size;
    stginfo->setfunc = fmt->setfunc;
    stginfo->getfunc = fmt->getfunc;
#ifdef WORDS_BIGENDIAN
    stginfo->format = _ctypes_alloc_format_string_for_type(proto_str[0], 1);
#else
    stginfo->format = _ctypes_alloc_format_string_for_type(proto_str[0], 0);
#endif
    if (stginfo->format == NULL) {
        Py_DECREF(proto);
        return -1;
    }

    stginfo->paramfunc = PyCSimpleType_paramfunc;
/*
    if (self->tp_base != st->Simple_Type) {
        stginfo->setfunc = NULL;
        stginfo->getfunc = NULL;
    }
*/

    /* This consumes the refcount on proto which we have */
    stginfo->proto = proto;

    /* Install from_param class methods in ctypes base classes.
       Overrides the PyCSimpleType_from_param generic method.
     */
    if (((PyTypeObject *)self)->tp_base == st->Simple_Type) {
        switch (*proto_str) {
        case 'z': /* c_char_p */
            ml = c_char_p_methods;
            stginfo->flags |= TYPEFLAG_ISPOINTER;
            break;
        case 'Z': /* c_wchar_p */
            ml = c_wchar_p_methods;
            stginfo->flags |= TYPEFLAG_ISPOINTER;
            break;
        case 'P': /* c_void_p */
            ml = c_void_p_methods;
            stginfo->flags |= TYPEFLAG_ISPOINTER;
            break;
        case 's':
        case 'X':
        case 'O':
            ml = NULL;
            stginfo->flags |= TYPEFLAG_ISPOINTER;
            break;
        default:
            ml = NULL;
            break;
        }

        if (ml) {
            PyObject *meth;
            int x;
            meth = PyDescr_NewClassMethod((PyTypeObject*)self, ml);
            if (!meth) {
                return -1;
            }
            PyObject *name = PyUnicode_FromString(ml->ml_name);
            if (name == NULL) {
                Py_DECREF(meth);
                return -1;
            }
            PyUnicode_InternInPlace(&name);
            x = PyDict_SetItem(((PyTypeObject*)self)->tp_dict, name, meth);
            Py_DECREF(name);
            Py_DECREF(meth);
            if (x == -1) {
                return -1;
            }
        }
    }

    PyTypeObject *type = Py_TYPE(self);
    if (type == st->PyCSimpleType_Type
        && fmt->setfunc_swapped
        && fmt->getfunc_swapped)
    {
        PyObject *swapped = CreateSwappedType(st, type, args, kwds,
                                              proto, fmt);
        if (swapped == NULL) {
            return -1;
        }
        StgInfo *sw_info;
        if (PyStgInfo_FromType(st, swapped, &sw_info) < 0) {
            Py_DECREF(swapped);
            return -1;
        }
        assert(sw_info);
#ifdef WORDS_BIGENDIAN
        PyObject_SetAttrString(self, "__ctype_le__", swapped);
        PyObject_SetAttrString(self, "__ctype_be__", self);
        PyObject_SetAttrString(swapped, "__ctype_be__", self);
        PyObject_SetAttrString(swapped, "__ctype_le__", swapped);
        /* We are creating the type for the OTHER endian */
        sw_info->format = _ctypes_alloc_format_string("<", stginfo->format+1);
#else
        PyObject_SetAttrString(self, "__ctype_be__", swapped);
        PyObject_SetAttrString(self, "__ctype_le__", self);
        PyObject_SetAttrString(swapped, "__ctype_le__", self);
        PyObject_SetAttrString(swapped, "__ctype_be__", swapped);
        /* We are creating the type for the OTHER endian */
        sw_info->format = _ctypes_alloc_format_string(">", stginfo->format+1);
#endif
        Py_DECREF(swapped);
        if (PyErr_Occurred()) {
            return -1;
        }
    };

    return 0;
}

/*
 * This is a *class method*.
 * Convert a parameter into something that ConvParam can handle.
 */

/*[clinic input]
_ctypes.PyCSimpleType.from_param as PyCSimpleType_from_param

    type: self
    cls: defining_class
    value: object
    /

Convert a Python object into a function call parameter.
[clinic start generated code]*/

static PyObject *
PyCSimpleType_from_param_impl(PyObject *type, PyTypeObject *cls,
                              PyObject *value)
/*[clinic end generated code: output=8a8453d9663e3a2e input=61cc48ce3a87a570]*/
{
    const char *fmt;
    PyCArgObject *parg;
    struct fielddesc *fd;
    PyObject *as_parameter;
    int res;

    /* If the value is already an instance of the requested type,
       we can use it as is */
    res = PyObject_IsInstance(value, type);
    if (res == -1)
        return NULL;
    if (res) {
        return Py_NewRef(value);
    }

    ctypes_state *st = get_module_state_by_class(cls);
    StgInfo *info;
    if (PyStgInfo_FromType(st, type, &info) < 0) {
        return NULL;
    }
    if (!info) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return NULL;
    }

    /* I think we can rely on this being a one-character string */
    fmt = PyUnicode_AsUTF8(info->proto);
    assert(fmt);

    fd = _ctypes_get_fielddesc(fmt);
    assert(fd);

    parg = PyCArgObject_new(st);
    if (parg == NULL)
        return NULL;

    parg->tag = fmt[0];
    parg->pffi_type = fd->pffi_type;
    parg->obj = fd->setfunc(&parg->value, value, info->size);
    if (parg->obj)
        return (PyObject *)parg;
    PyObject *exc = PyErr_GetRaisedException();
    Py_DECREF(parg);

    if (PyObject_GetOptionalAttr(value, &_Py_ID(_as_parameter_), &as_parameter) < 0) {
        Py_XDECREF(exc);
        return NULL;
    }
    if (as_parameter) {
        if (_Py_EnterRecursiveCall(" while processing _as_parameter_")) {
            Py_DECREF(as_parameter);
            Py_XDECREF(exc);
            return NULL;
        }
        value = PyCSimpleType_from_param_impl(type, cls, as_parameter);
        Py_DECREF(as_parameter);
        Py_XDECREF(exc);
        _Py_LeaveRecursiveCall();
        return value;
    }
    if (exc) {
        PyErr_SetRaisedException(exc);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "wrong type");
    }
    return NULL;
}

static PyMethodDef PyCSimpleType_methods[] = {
    PYCSIMPLETYPE_FROM_PARAM_METHODDEF
    CDATATYPE_FROM_ADDRESS_METHODDEF
    CDATATYPE_FROM_BUFFER_METHODDEF
    CDATATYPE_FROM_BUFFER_COPY_METHODDEF
    CDATATYPE_IN_DLL_METHODDEF
    { NULL, NULL },
};

static PyType_Slot pycsimple_type_slots[] = {
    {Py_tp_doc, PyDoc_STR("metatype for the PyCSimpleType Objects")},
    {Py_tp_methods, PyCSimpleType_methods},
    {Py_tp_init, PyCSimpleType_init},
    {0, NULL},
};

static PyType_Spec pycsimple_type_spec = {
    .name = "_ctypes.PyCSimpleType",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pycsimple_type_slots,
};

/******************************************************************/
/*
  PyCFuncPtrType_Type
 */

static PyObject *
converters_from_argtypes(ctypes_state *st, PyObject *ob)
{
    PyObject *converters;
    Py_ssize_t i;

    ob = PySequence_Tuple(ob); /* new reference */
    if (!ob) {
        PyErr_SetString(PyExc_TypeError,
                        "_argtypes_ must be a sequence of types");
        return NULL;
    }

    Py_ssize_t nArgs = PyTuple_GET_SIZE(ob);
    if (nArgs > CTYPES_MAX_ARGCOUNT) {
        Py_DECREF(ob);
        PyErr_Format(st->PyExc_ArgError,
                     "_argtypes_ has too many arguments (%zi), maximum is %i",
                     nArgs, CTYPES_MAX_ARGCOUNT);
        return NULL;
    }

    converters = PyTuple_New(nArgs);
    if (!converters) {
        Py_DECREF(ob);
        return NULL;
    }

    /* I have to check if this is correct. Using c_char, which has a size
       of 1, will be assumed to be pushed as only one byte!
       Aren't these promoted to integers by the C compiler and pushed as 4 bytes?
    */

    for (i = 0; i < nArgs; ++i) {
        PyObject *cnv;
        PyObject *tp = PyTuple_GET_ITEM(ob, i);
/*
 *      The following checks, relating to bpo-16575 and bpo-16576, have been
 *      disabled. The reason is that, although there is a definite problem with
 *      how libffi handles unions (https://github.com/libffi/libffi/issues/33),
 *      there are numerous libraries which pass structures containing unions
 *      by values - especially on Windows but examples also exist on Linux
 *      (https://bugs.python.org/msg359834).
 *
 *      It may not be possible to get proper support for unions and bitfields
 *      until support is forthcoming in libffi, but for now, adding the checks
 *      has caused problems in otherwise-working software, which suggests it
 *      is better to disable the checks.
 *
 *      Although specific examples reported relate specifically to unions and
 *      not bitfields, the bitfields check is also being disabled as a
 *      precaution.

        StgInfo *stginfo;
        if (PyStgInfo_FromType(st, tp, &stginfo) < 0) {
            return -1;
        }

        // TYPEFLAG_HASUNION and TYPEFLAG_HASBITFIELD used to be set
        // if there were any unions/bitfields;
        // if the check is re-enabled we either need to loop here or
        // restore the flag
        if (stginfo != NULL) {
            if (stginfo->flags & TYPEFLAG_HASUNION) {
                Py_DECREF(converters);
                Py_DECREF(ob);
                if (!PyErr_Occurred()) {
                    PyErr_Format(PyExc_TypeError,
                                 "item %zd in _argtypes_ passes a union by "
                                 "value, which is unsupported.",
                                 i + 1);
                }
                return NULL;
            }
            if (stginfo->flags & TYPEFLAG_HASBITFIELD) {
                Py_DECREF(converters);
                Py_DECREF(ob);
                if (!PyErr_Occurred()) {
                    PyErr_Format(PyExc_TypeError,
                                 "item %zd in _argtypes_ passes a struct/"
                                 "union with a bitfield by value, which is "
                                 "unsupported.",
                                 i + 1);
                }
                return NULL;
            }
        }
 */

        if (PyObject_GetOptionalAttr(tp, &_Py_ID(from_param), &cnv) <= 0) {
            Py_DECREF(converters);
            Py_DECREF(ob);
            if (!PyErr_Occurred()) {
                PyErr_Format(PyExc_TypeError,
                             "item %zd in _argtypes_ has no from_param method",
                             i+1);
            }
            return NULL;
        }
        PyTuple_SET_ITEM(converters, i, cnv);
    }
    Py_DECREF(ob);
    return converters;
}

static int
make_funcptrtype_dict(ctypes_state *st, PyObject *attrdict, StgInfo *stginfo)
{
    PyObject *ob;
    PyObject *converters = NULL;

    stginfo->align = _ctypes_get_fielddesc("P")->pffi_type->alignment;
    stginfo->length = 1;
    stginfo->size = sizeof(void *);
    stginfo->setfunc = NULL;
    stginfo->getfunc = NULL;
    stginfo->ffi_type_pointer = ffi_type_pointer;

    if (PyDict_GetItemRef(attrdict, &_Py_ID(_flags_), &ob) < 0) {
        return -1;
    }
    if (!ob || !PyLong_Check(ob)) {
        PyErr_SetString(PyExc_TypeError,
                "class must define _flags_ which must be an integer");
        Py_XDECREF(ob);
        return -1;
    }
    stginfo->flags = PyLong_AsUnsignedLongMask(ob) | TYPEFLAG_ISPOINTER;
    Py_DECREF(ob);

    /* _argtypes_ is optional... */
    if (PyDict_GetItemRef(attrdict, &_Py_ID(_argtypes_), &ob) < 0) {
        return -1;
    }
    if (ob) {
        converters = converters_from_argtypes(st, ob);
        if (!converters) {
            Py_DECREF(ob);
            return -1;
        }
        stginfo->argtypes = ob;
        stginfo->converters = converters;
    }

    if (PyDict_GetItemRef(attrdict, &_Py_ID(_restype_), &ob) < 0) {
        return -1;
    }
    if (ob) {
        StgInfo *info;
        if (PyStgInfo_FromType(st, ob, &info) < 0) {
            Py_DECREF(ob);
            return -1;
        }
        if (ob != Py_None && !info && !PyCallable_Check(ob)) {
            PyErr_SetString(PyExc_TypeError,
                "_restype_ must be a type, a callable, or None");
            Py_DECREF(ob);
            return -1;
        }
        stginfo->restype = ob;
        if (PyObject_GetOptionalAttr(ob, &_Py_ID(_check_retval_),
                                   &stginfo->checker) < 0)
        {
            return -1;
        }
    }
/* XXX later, maybe.
    if (PyDict_GetItemRef(attrdict, &_Py _ID(_errcheck_), &ob) < 0) {
        return -1;
    }
    if (ob) {
        if (!PyCallable_Check(ob)) {
            PyErr_SetString(PyExc_TypeError,
                "_errcheck_ must be callable");
            Py_DECREF(ob);
            return -1;
        }
        stginfo->errcheck = ob;
    }
*/
    return 0;
}

static PyCArgObject *
PyCFuncPtrType_paramfunc(ctypes_state *st, CDataObject *self)
{
    PyCArgObject *parg;

    parg = PyCArgObject_new(st);
    if (parg == NULL)
        return NULL;

    parg->tag = 'P';
    parg->pffi_type = &ffi_type_pointer;
    parg->obj = Py_NewRef(self);
    parg->value.p = locked_deref(self);
    return parg;
}

static int
PyCFuncPtrType_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *attrdict = PyType_GetDict((PyTypeObject *)self);
    if (!attrdict) {
        return -1;
    }

    ctypes_state *st = get_module_state_by_def(Py_TYPE(self));
    StgInfo *stginfo = PyStgInfo_Init(st, (PyTypeObject *)self);
    if (!stginfo) {
        Py_DECREF(attrdict);
        return -1;
    }

    stginfo->paramfunc = PyCFuncPtrType_paramfunc;

    /* We do NOT expose the function signature in the format string.  It
       is impossible, generally, because the only requirement for the
       argtypes items is that they have a .from_param method - we do not
       know the types of the arguments (although, in practice, most
       argtypes would be a ctypes type).
    */
    stginfo->format = _ctypes_alloc_format_string(NULL, "X{}");
    if (stginfo->format == NULL) {
        Py_DECREF(attrdict);
        return -1;
    }
    stginfo->flags |= TYPEFLAG_ISPOINTER;

    if (make_funcptrtype_dict(st, attrdict, stginfo) < 0) {
        Py_DECREF(attrdict);
        return -1;
    }

    Py_DECREF(attrdict);
    return 0;
}

static PyType_Slot pycfuncptr_type_slots[] = {
    {Py_tp_doc, PyDoc_STR("metatype for C function pointers")},
    {Py_tp_methods, CDataType_methods},
    {Py_tp_init, PyCFuncPtrType_init},
    {0, NULL},
};

static PyType_Spec pycfuncptr_type_spec = {
    .name = "_ctypes.PyCFuncPtrType",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pycfuncptr_type_slots,
};


/*****************************************************************
 * Code to keep needed objects alive
 */

static CDataObject *
PyCData_GetContainer(CDataObject *self)
{
    while (self->b_base) {
        self = self->b_base;
    }
    if (self->b_objects == NULL) {
        if (self->b_length) {
            self->b_objects = PyDict_New();
            if (self->b_objects == NULL)
                return NULL;
        } else {
            self->b_objects = Py_NewRef(Py_None);
        }
    }
    return self;
}

static PyObject *
GetKeepedObjects(CDataObject *target)
{
    CDataObject *container;
    container = PyCData_GetContainer(target);
    if (container == NULL)
        return NULL;
    return container->b_objects;
}

static PyObject *
unique_key(CDataObject *target, Py_ssize_t index)
{
    char string[256];
    char *cp = string;
    size_t bytes_left;

    Py_BUILD_ASSERT(sizeof(string) - 1 > sizeof(Py_ssize_t) * 2);
    cp += sprintf(cp, "%x", Py_SAFE_DOWNCAST(index, Py_ssize_t, int));
    while (target->b_base) {
        bytes_left = sizeof(string) - (cp - string) - 1;
        /* Hex format needs 2 characters per byte */
        if (bytes_left < sizeof(Py_ssize_t) * 2) {
            PyErr_SetString(PyExc_ValueError,
                            "ctypes object structure too deep");
            return NULL;
        }
        cp += sprintf(cp, ":%x", Py_SAFE_DOWNCAST(target->b_index, Py_ssize_t, int));
        target = target->b_base;
    }
    return PyUnicode_FromStringAndSize(string, cp-string);
}

/*
 * Keep a reference to 'keep' in the 'target', at index 'index'.
 *
 * If 'keep' is None, do nothing.
 *
 * Otherwise create a dictionary (if it does not yet exist) id the root
 * objects 'b_objects' item, which will store the 'keep' object under a unique
 * key.
 *
 * The unique_key helper travels the target's b_base pointer down to the root,
 * building a string containing hex-formatted indexes found during traversal,
 * separated by colons.
 *
 * The index tuple is used as a key into the root object's b_objects dict.
 *
 * Note: This function steals a refcount of the third argument, even if it
 * fails!
 */
static int
KeepRef(CDataObject *target, Py_ssize_t index, PyObject *keep)
{
    int result;
    CDataObject *ob;
    PyObject *key;

/* Optimization: no need to store None */
    if (keep == Py_None) {
        Py_DECREF(Py_None);
        return 0;
    }
    ob = PyCData_GetContainer(target);
    if (ob == NULL) {
        Py_DECREF(keep);
        return -1;
    }
    if (ob->b_objects == NULL || !PyDict_CheckExact(ob->b_objects)) {
        Py_XSETREF(ob->b_objects, keep); /* refcount consumed */
        return 0;
    }
    key = unique_key(target, index);
    if (key == NULL) {
        Py_DECREF(keep);
        return -1;
    }
    result = PyDict_SetItem(ob->b_objects, key, keep);
    Py_DECREF(key);
    Py_DECREF(keep);
    return result;
}

/******************************************************************/
/*
  PyCData_Type
 */

/*[clinic input]
class _ctypes.PyCData "PyObject *" "clinic_state()->PyCData_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=ac13df38dee3c22c]*/


static int
PyCData_traverse(PyObject *op, visitproc visit, void *arg)
{
    CDataObject *self = _CDataObject_CAST(op);
    Py_VISIT(self->b_objects);
    Py_VISIT((PyObject *)self->b_base);
    PyTypeObject *type = Py_TYPE(self);
    Py_VISIT(type);
    return 0;
}

static int
PyCData_clear(PyObject *op)
{
    CDataObject *self = _CDataObject_CAST(op);
    Py_CLEAR(self->b_objects);
    if ((self->b_needsfree)
        && _CDataObject_HasExternalBuffer(self))
        PyMem_Free(self->b_ptr);
    self->b_ptr = NULL;
    Py_CLEAR(self->b_base);
    return 0;
}

static void
PyCData_dealloc(PyObject *self)
{
    PyTypeObject *type = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    (void)PyCData_clear(self);
    type->tp_free(self);
    Py_DECREF(type);
}

static PyMemberDef PyCData_members[] = {
    { "_b_base_", _Py_T_OBJECT,
      offsetof(CDataObject, b_base), Py_READONLY,
      "the base object" },
    { "_b_needsfree_", Py_T_INT,
      offsetof(CDataObject, b_needsfree), Py_READONLY,
      "whether the object owns the memory or not" },
    { "_objects", _Py_T_OBJECT,
      offsetof(CDataObject, b_objects), Py_READONLY,
      "internal objects tree (NEVER CHANGE THIS OBJECT!)"},
    { NULL },
};

/* Find the innermost type of an array type, returning a borrowed reference */
static PyObject *
PyCData_item_type(ctypes_state *st, PyObject *type)
{
    if (PyCArrayTypeObject_Check(st, type)) {
        PyObject *elem_type;

        /* asserts used here as these are all guaranteed by construction */
        StgInfo *stg_info;
        if (PyStgInfo_FromType(st, type, &stg_info) < 0) {
            return NULL;
        }
        assert(stg_info);
        elem_type = stg_info->proto;
        assert(elem_type);
        return PyCData_item_type(st, elem_type);
    }
    else {
        return type;
    }
}

static int
PyCData_NewGetBuffer(PyObject *myself, Py_buffer *view, int flags)
{
    CDataObject *self = _CDataObject_CAST(myself);

    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(myself)));
    StgInfo *info;
    if (PyStgInfo_FromObject(st, myself, &info) < 0) {
        return -1;
    }
    assert(info);

    PyObject *item_type = PyCData_item_type(st, (PyObject*)Py_TYPE(myself));
    if (item_type == NULL) {
        return 0;
    }

    if (view == NULL) return 0;

    StgInfo *item_info;
    if (PyStgInfo_FromType(st, item_type, &item_info) < 0) {
        return -1;
    }
    assert(item_info);

    view->buf = self->b_ptr;
    view->obj = Py_NewRef(myself);
    view->len = self->b_size;
    view->readonly = 0;
    /* use default format character if not set */
    view->format = info->format ? info->format : "B";
    view->ndim = info->ndim;
    view->shape = info->shape;
    view->itemsize = item_info->size;
    view->strides = NULL;
    view->suboffsets = NULL;
    view->internal = NULL;
    return 0;
}

/*
 * CData objects are mutable, so they cannot be hashable!
 */
static Py_hash_t
PyCData_nohash(PyObject *self)
{
    PyErr_SetString(PyExc_TypeError, "unhashable type");
    return -1;
}

/*[clinic input]
@critical_section
_ctypes.PyCData.__reduce__

    myself: self
    cls: defining_class
    /
[clinic start generated code]*/

static PyObject *
_ctypes_PyCData___reduce___impl(PyObject *myself, PyTypeObject *cls)
/*[clinic end generated code: output=eaad97e111599294 input=6a464e1a1e2bbdbd]*/
{
    CDataObject *self = _CDataObject_CAST(myself);

    ctypes_state *st = get_module_state_by_class(cls);
    StgInfo *info;
    if (PyStgInfo_FromObject(st, myself, &info) < 0) {
        return NULL;
    }
    assert(info);

    if (info->flags & (TYPEFLAG_ISPOINTER|TYPEFLAG_HASPOINTER)) {
        PyErr_SetString(PyExc_ValueError,
                        "ctypes objects containing pointers cannot be pickled");
        return NULL;
    }
    PyObject *dict = PyObject_GetAttrString(myself, "__dict__");
    if (dict == NULL) {
        return NULL;
    }
    PyObject *bytes;
    bytes = PyBytes_FromStringAndSize(self->b_ptr, self->b_size);
    return Py_BuildValue("O(O(NN))", st->_unpickle, Py_TYPE(myself), dict,
                         bytes);
}

/*[clinic input]
@critical_section
_ctypes.PyCData.__setstate__

    myself: self
    dict: object(subclass_of="&PyDict_Type")
    data: str(accept={str, robuffer}, zeroes=True)
    /
[clinic start generated code]*/

static PyObject *
_ctypes_PyCData___setstate___impl(PyObject *myself, PyObject *dict,
                                  const char *data, Py_ssize_t data_length)
/*[clinic end generated code: output=8bd4c0a5b4f254bd input=124f5070258254c6]*/
{
    CDataObject *self = _CDataObject_CAST(myself);

    if (data_length > self->b_size) {
        data_length = self->b_size;
    }
    memmove(self->b_ptr, data, data_length);
    PyObject *mydict = PyObject_GetAttrString(myself, "__dict__");
    if (mydict == NULL) {
        return NULL;
    }
    if (!PyDict_Check(mydict)) {
        PyErr_Format(PyExc_TypeError,
                     "%.200s.__dict__ must be a dictionary, not %.200s",
                     Py_TYPE(myself)->tp_name, Py_TYPE(mydict)->tp_name);
        Py_DECREF(mydict);
        return NULL;
    }
    int res = PyDict_Update(mydict, dict);
    Py_DECREF(mydict);
    if (res == -1)
        return NULL;
    Py_RETURN_NONE;
}

/*[clinic input]
_ctypes.PyCData.__ctypes_from_outparam__

default __ctypes_from_outparam__ method returns self.
[clinic start generated code]*/

static PyObject *
_ctypes_PyCData___ctypes_from_outparam___impl(PyObject *self)
/*[clinic end generated code: output=a7facc849097b549 input=910c5fec33e268c9]*/
{
    return Py_NewRef(self);
}

static PyMethodDef PyCData_methods[] = {
    _CTYPES_PYCDATA___CTYPES_FROM_OUTPARAM___METHODDEF
    _CTYPES_PYCDATA___SETSTATE___METHODDEF
    _CTYPES_PYCDATA___REDUCE___METHODDEF
    { NULL, NULL },
};

static PyType_Slot pycdata_slots[] = {
    {Py_tp_dealloc, PyCData_dealloc},
    {Py_tp_hash, PyCData_nohash},
    {Py_tp_doc, PyDoc_STR("XXX to be provided")},
    {Py_tp_traverse, PyCData_traverse},
    {Py_tp_clear, PyCData_clear},
    {Py_tp_methods, PyCData_methods},
    {Py_tp_members, PyCData_members},
    {Py_bf_getbuffer, PyCData_NewGetBuffer},
    {0, NULL},
};

static PyType_Spec pycdata_spec = {
    .name = "_ctypes._CData",
    .basicsize = sizeof(CDataObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION),
    .slots = pycdata_slots,
};

static int
PyCData_MallocBuffer(CDataObject *obj, StgInfo *info)
{
    /* We don't have to lock in this function, because it's only
     * used in constructors and therefore does not have concurrent
     * access.
     */
   assert (Py_REFCNT(obj) == 1);
   assert(stginfo_get_dict_final(info) == 1);

    if ((size_t)info->size <= sizeof(obj->b_value)) {
        /* No need to call malloc, can use the default buffer */
        obj->b_ptr = (char *)&obj->b_value;
        /* The b_needsfree flag does not mean that we actually did
           call PyMem_Malloc to allocate the memory block; instead it
           means we are the *owner* of the memory and are responsible
           for freeing resources associated with the memory.  This is
           also the reason that b_needsfree is exposed to Python.
         */
        obj->b_needsfree = 1;
    } else {
        /* In python 2.4, and ctypes 0.9.6, the malloc call took about
           33% of the creation time for c_int().
        */
        obj->b_ptr = (char *)PyMem_Malloc(info->size);
        if (obj->b_ptr == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        obj->b_needsfree = 1;
        memset(obj->b_ptr, 0, info->size);
    }
    obj->b_size = info->size;
    return 0;
}

PyObject *
PyCData_FromBaseObj(ctypes_state *st,
                    PyObject *type, PyObject *base, Py_ssize_t index, char *adr)
{
    CDataObject *cmem;

    assert(PyType_Check(type));

    StgInfo *info;
    if (PyStgInfo_FromType(st, type, &info) < 0) {
        return NULL;
    }
    if (!info) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return NULL;
    }

    stginfo_set_dict_final(info);
    cmem = (CDataObject *)((PyTypeObject *)type)->tp_alloc((PyTypeObject *)type, 0);
    if (cmem == NULL) {
        return NULL;
    }
    assert(CDataObject_Check(st, cmem));
    cmem->b_length = info->length;
    cmem->b_size = info->size;
    if (base) { /* use base's buffer */
        assert(CDataObject_Check(st, base));
        cmem->b_ptr = adr;
        cmem->b_needsfree = 0;
        cmem->b_base = (CDataObject *)Py_NewRef(base);
        cmem->b_index = index;
    } else { /* copy contents of adr */
        if (-1 == PyCData_MallocBuffer(cmem, info)) {
            Py_DECREF(cmem);
            return NULL;
        }
        memcpy(cmem->b_ptr, adr, info->size);
        cmem->b_index = index;
    }
    return (PyObject *)cmem;
}

/*
 Box a memory block into a CData instance.
*/
PyObject *
PyCData_AtAddress(ctypes_state *st, PyObject *type, void *buf)
{
    CDataObject *pd;

    if (PySys_Audit("ctypes.cdata", "n", (Py_ssize_t)buf) < 0) {
        return NULL;
    }

    assert(PyType_Check(type));

    StgInfo *info;
    if (PyStgInfo_FromType(st, type, &info) < 0) {
        return NULL;
    }
    if (!info) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return NULL;
    }

    stginfo_set_dict_final(info);

    pd = (CDataObject *)((PyTypeObject *)type)->tp_alloc((PyTypeObject *)type, 0);
    if (!pd) {
        return NULL;
    }
    assert(CDataObject_Check(st, pd));
    pd->b_ptr = (char *)buf;
    pd->b_length = info->length;
    pd->b_size = info->size;
    return (PyObject *)pd;
}

/*
  This function returns TRUE for c_int, c_void_p, and these kind of
  classes.  FALSE otherwise FALSE also for subclasses of c_int and
  such.
*/
int _ctypes_simple_instance(ctypes_state *st, PyObject *obj)
{
    PyTypeObject *type = (PyTypeObject *)obj;

    if (PyCSimpleTypeObject_Check(st, type)) {
        return type->tp_base != st->Simple_Type;
    }
    return 0;
}

PyObject *
PyCData_get(ctypes_state *st, PyObject *type, GETFUNC getfunc, PyObject *src,
          Py_ssize_t index, Py_ssize_t size, char *adr)
{
    CDataObject *cdata = _CDataObject_CAST(src);
    if (getfunc) {
        PyObject *res;
        LOCK_PTR(cdata);
        res = getfunc(adr, size);
        UNLOCK_PTR(cdata);
        return res;
    }
    assert(type);
    StgInfo *info;
    if (PyStgInfo_FromType(st, type, &info) < 0) {
        return NULL;
    }
    if (info && info->getfunc && !_ctypes_simple_instance(st, type)) {
        PyObject *res;
        LOCK_PTR(cdata);
        res = info->getfunc(adr, size);
        UNLOCK_PTR(cdata);
        return res;
    }
    return PyCData_FromBaseObj(st, type, src, index, adr);
}

/*
  Helper function for PyCData_set below.
*/
static PyObject *
_PyCData_set(ctypes_state *st,
           CDataObject *dst, PyObject *type, SETFUNC setfunc, PyObject *value,
           Py_ssize_t size, char *ptr)
{
    CDataObject *src;
    int err;

    if (setfunc) {
        PyObject *res;
        LOCK_PTR(dst);
        res = setfunc(ptr, value, size);
        UNLOCK_PTR(dst);
        return res;
    }
    if (!CDataObject_Check(st, value)) {
        StgInfo *info;
        if (PyStgInfo_FromType(st, type, &info) < 0) {
            return NULL;
        }
        if (info && info->setfunc) {
            PyObject *res;
            LOCK_PTR(dst);
            res = info->setfunc(ptr, value, size);
            UNLOCK_PTR(dst);
            return res;
        }
        /*
           If value is a tuple, we try to call the type with the tuple
           and use the result!
        */
        assert(PyType_Check(type));
        if (PyTuple_Check(value)) {
            PyObject *ob;
            PyObject *result;
            ob = PyObject_CallObject(type, value);
            if (ob == NULL) {
                _ctypes_extend_error(PyExc_RuntimeError, "(%s) ",
                                  ((PyTypeObject *)type)->tp_name);
                return NULL;
            }
            result = _PyCData_set(st, dst, type, setfunc, ob,
                                size, ptr);
            Py_DECREF(ob);
            return result;
        } else if (value == Py_None && PyCPointerTypeObject_Check(st, type)) {
            LOCK_PTR(dst);
            *(void **)ptr = NULL;
            UNLOCK_PTR(dst);
            Py_RETURN_NONE;
        } else {
            PyErr_Format(PyExc_TypeError,
                         "expected %s instance, got %s",
                         ((PyTypeObject *)type)->tp_name,
                         Py_TYPE(value)->tp_name);
            return NULL;
        }
    }
    src = (CDataObject *)value;

    err = PyObject_IsInstance(value, type);
    if (err == -1)
        return NULL;
    if (err) {
        locked_memcpy_from(ptr, src, size);

        if (PyCPointerTypeObject_Check(st, type)) {
            /* XXX */
        }

        value = GetKeepedObjects(src);
        if (value == NULL)
            return NULL;

        return Py_NewRef(value);
    }

    if (PyCPointerTypeObject_Check(st, type)
        && ArrayObject_Check(st, value)) {
        PyObject *keep;

        StgInfo *p1, *p2;
        if (PyStgInfo_FromObject(st, value, &p1) < 0) {
            return NULL;
        }
        assert(p1); /* Cannot be NULL for array instances */
        if (PyStgInfo_FromType(st, type, &p2) < 0) {
            return NULL;
        }
        assert(p2); /* Cannot be NULL for pointer types */

        if (p1->proto != p2->proto) {
            PyErr_Format(PyExc_TypeError,
                         "incompatible types, %s instance instead of %s instance",
                         Py_TYPE(value)->tp_name,
                         ((PyTypeObject *)type)->tp_name);
            return NULL;
        }
        LOCK_PTR(src);
        *(void **)ptr = src->b_ptr;
        UNLOCK_PTR(src);

        keep = GetKeepedObjects(src);
        if (keep == NULL)
            return NULL;

        /*
          We are assigning an array object to a field which represents
          a pointer. This has the same effect as converting an array
          into a pointer. So, again, we have to keep the whole object
          pointed to (which is the array in this case) alive, and not
          only it's object list.  So we create a tuple, containing
          b_objects list PLUS the array itself, and return that!
        */
        return PyTuple_Pack(2, keep, value);
    }
    PyErr_Format(PyExc_TypeError,
                 "incompatible types, %s instance instead of %s instance",
                 Py_TYPE(value)->tp_name,
                 ((PyTypeObject *)type)->tp_name);
    return NULL;
}

/*
 * Set a slice in object 'dst', which has the type 'type',
 * to the value 'value'.
 */
int
PyCData_set(ctypes_state *st,
          PyObject *dst, PyObject *type, SETFUNC setfunc, PyObject *value,
          Py_ssize_t index, Py_ssize_t size, char *ptr)
{
    CDataObject *mem = (CDataObject *)dst;
    PyObject *result;

    if (!CDataObject_Check(st, dst)) {
        PyErr_SetString(PyExc_TypeError,
                        "not a ctype instance");
        return -1;
    }

    result = _PyCData_set(st, mem, type, setfunc, value,
                        size, ptr);
    if (result == NULL)
        return -1;

    /* KeepRef steals a refcount from it's last argument */
    /* If KeepRef fails, we are stumped.  The dst memory block has already
       been changed */
    return KeepRef(mem, index, result);
}


/******************************************************************/
static PyObject *
GenericPyCData_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ctypes_state *st = get_module_state_by_def(Py_TYPE(type));
    return generic_pycdata_new(st, type, args, kwds);
}

static inline PyObject *
generic_pycdata_new(ctypes_state *st,
                    PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    CDataObject *obj;

    StgInfo *info;
    if (PyStgInfo_FromType(st, (PyObject *)type, &info) < 0) {
        return NULL;
    }
    if (!info) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return NULL;
    }

    stginfo_set_dict_final(info);

    obj = (CDataObject *)type->tp_alloc(type, 0);
    if (!obj)
        return NULL;

    obj->b_base = NULL;
    obj->b_index = 0;
    obj->b_objects = NULL;
    obj->b_length = info->length;

    if (-1 == PyCData_MallocBuffer(obj, info)) {
        Py_DECREF(obj);
        return NULL;
    }
    return (PyObject *)obj;
}
/*****************************************************************/
/*
  PyCFuncPtr_Type
*/

/*[clinic input]
@critical_section
@setter
_ctypes.CFuncPtr.errcheck
[clinic start generated code]*/

static int
_ctypes_CFuncPtr_errcheck_set_impl(PyCFuncPtrObject *self, PyObject *value)
/*[clinic end generated code: output=6580cf1ffdf3b9fb input=84930bb16c490b33]*/
{
    if (value && !PyCallable_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "the errcheck attribute must be callable");
        return -1;
    }
    Py_XINCREF(value);
    Py_XSETREF(self->errcheck, value);
    return 0;
}

/*[clinic input]
@critical_section
@getter
_ctypes.CFuncPtr.errcheck

a function to check for errors
[clinic start generated code]*/

static PyObject *
_ctypes_CFuncPtr_errcheck_get_impl(PyCFuncPtrObject *self)
/*[clinic end generated code: output=dfa6fb5c6f90fd14 input=4672135fef37819f]*/
{
    if (self->errcheck) {
        return Py_NewRef(self->errcheck);
    }
    Py_RETURN_NONE;
}

/*[clinic input]
@setter
@critical_section
_ctypes.CFuncPtr.restype
[clinic start generated code]*/

static int
_ctypes_CFuncPtr_restype_set_impl(PyCFuncPtrObject *self, PyObject *value)
/*[clinic end generated code: output=0be0a086abbabf18 input=683c3bef4562ccc6]*/
{
    PyObject *checker, *oldchecker;
    if (value == NULL) {
        oldchecker = self->checker;
        self->checker = NULL;
        Py_CLEAR(self->restype);
        Py_XDECREF(oldchecker);
        return 0;
    }
    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));
    StgInfo *info;
    if (PyStgInfo_FromType(st, value, &info) < 0) {
        return -1;
    }
    if (value != Py_None && !info && !PyCallable_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "restype must be a type, a callable, or None");
        return -1;
    }
    if (PyObject_GetOptionalAttr(value, &_Py_ID(_check_retval_), &checker) < 0) {
        return -1;
    }
    oldchecker = self->checker;
    self->checker = checker;
    Py_INCREF(value);
    Py_XSETREF(self->restype, value);
    Py_XDECREF(oldchecker);
    return 0;
}

/*[clinic input]
@getter
@critical_section
_ctypes.CFuncPtr.restype

specify the result type
[clinic start generated code]*/

static PyObject *
_ctypes_CFuncPtr_restype_get_impl(PyCFuncPtrObject *self)
/*[clinic end generated code: output=c8f44cd16f1dee5e input=5e3ed95116204fd2]*/
{
    if (self->restype) {
        return Py_NewRef(self->restype);
    }
    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));
    StgInfo *info;
    if (PyStgInfo_FromObject(st, (PyObject *)self, &info) < 0) {
        return NULL;
    }
    assert(info); /* Cannot be NULL for PyCFuncPtrObject instances */
    if (info->restype) {
        return Py_NewRef(info->restype);
    } else {
        Py_RETURN_NONE;
    }
}

/*[clinic input]
@setter
@critical_section
_ctypes.CFuncPtr.argtypes
[clinic start generated code]*/

static int
_ctypes_CFuncPtr_argtypes_set_impl(PyCFuncPtrObject *self, PyObject *value)
/*[clinic end generated code: output=596a36e2ae89d7d1 input=c4627573e980aa8b]*/
{
    PyObject *converters;

    if (value == NULL || value == Py_None) {
        Py_CLEAR(self->converters);
        Py_CLEAR(self->argtypes);
    } else {
        ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));
        converters = converters_from_argtypes(st, value);
        if (!converters)
            return -1;
        Py_XSETREF(self->converters, converters);
        Py_INCREF(value);
        Py_XSETREF(self->argtypes, value);
    }
    return 0;
}

/*[clinic input]
@getter
@critical_section
_ctypes.CFuncPtr.argtypes

specify the argument types
[clinic start generated code]*/

static PyObject *
_ctypes_CFuncPtr_argtypes_get_impl(PyCFuncPtrObject *self)
/*[clinic end generated code: output=c46b05a1b0f99172 input=37a8a545a56f8ae2]*/
{
    if (self->argtypes) {
        return Py_NewRef(self->argtypes);
    }
    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));
    StgInfo *info;
    if (PyStgInfo_FromObject(st, (PyObject *)self, &info) < 0) {
        return NULL;
    }
    assert(info); /* Cannot be NULL for PyCFuncPtrObject instances */
    if (info->argtypes) {
        return Py_NewRef(info->argtypes);
    } else {
        Py_RETURN_NONE;
    }
}

static PyGetSetDef PyCFuncPtr_getsets[] = {
    _CTYPES_CFUNCPTR_ERRCHECK_GETSETDEF
    _CTYPES_CFUNCPTR_RESTYPE_GETSETDEF
    _CTYPES_CFUNCPTR_ARGTYPES_GETSETDEF
    { NULL, NULL }
};

#ifdef MS_WIN32
static PPROC FindAddress(void *handle, const char *name, PyObject *type)
{
    PPROC address;
#ifdef MS_WIN64
    /* win64 has no stdcall calling conv, so it should
       also not have the name mangling of it.
    */
    Py_BEGIN_ALLOW_THREADS
    address = (PPROC)GetProcAddress(handle, name);
    Py_END_ALLOW_THREADS
    return address;
#else
    char *mangled_name;
    int i;

    Py_BEGIN_ALLOW_THREADS
    address = (PPROC)GetProcAddress(handle, name);
    Py_END_ALLOW_THREADS
    if (address)
        return address;
    if (((size_t)name & ~0xFFFF) == 0) {
        return NULL;
    }

    ctypes_state *st = get_module_state_by_def(Py_TYPE(type));
    StgInfo *info;
    if (PyStgInfo_FromType(st, (PyObject *)type, &info) < 0) {
        return NULL;
    }
    /* It should not happen that info is NULL, but better be safe */
    if (info==NULL || info->flags & FUNCFLAG_CDECL)
        return address;

    /* for stdcall, try mangled names:
       funcname -> _funcname@<n>
       where n is 0, 4, 8, 12, ..., 128
     */
    mangled_name = alloca(strlen(name) + 1 + 1 + 1 + 3); /* \0 _ @ %d */
    if (!mangled_name)
        return NULL;
    for (i = 0; i < 32; ++i) {
        sprintf(mangled_name, "_%s@%d", name, i*4);
        Py_BEGIN_ALLOW_THREADS
        address = (PPROC)GetProcAddress(handle, mangled_name);
        Py_END_ALLOW_THREADS
        if (address)
            return address;
    }
    return NULL;
#endif
}
#endif

/* Return 1 if usable, 0 else and exception set. */
static int
_check_outarg_type(ctypes_state *st, PyObject *arg, Py_ssize_t index)
{
    if (PyCPointerTypeObject_Check(st, arg)) {
        return 1;
    }
    if (PyCArrayTypeObject_Check(st, arg)) {
        return 1;
    }
    StgInfo *info;
    if (PyStgInfo_FromType(st, arg, &info) < 0) {
        return -1;
    }
    if (info
        /* simple pointer types, c_void_p, c_wchar_p, BSTR, ... */
        && PyUnicode_Check(info->proto)
/* We only allow c_void_p, c_char_p and c_wchar_p as a simple output parameter type */
        && (strchr("PzZ", PyUnicode_AsUTF8(info->proto)[0]))) {
        return 1;
    }

    PyErr_Format(PyExc_TypeError,
                 "'out' parameter %d must be a pointer type, not %s",
                 Py_SAFE_DOWNCAST(index, Py_ssize_t, int),
                 PyType_Check(arg) ?
                 ((PyTypeObject *)arg)->tp_name :
             Py_TYPE(arg)->tp_name);
    return 0;
}

/* Returns 1 on success, 0 on error */
static int
_validate_paramflags(ctypes_state *st, PyTypeObject *type, PyObject *paramflags)
{
    Py_ssize_t i, len;
    PyObject *argtypes;

    StgInfo *info;
    if (PyStgInfo_FromType(st, (PyObject *)type, &info) < 0) {
        return -1;
    }
    if (!info) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return 0;
    }
    argtypes = info->argtypes;

    if (paramflags == NULL || info->argtypes == NULL)
        return 1;

    if (!PyTuple_Check(paramflags)) {
        PyErr_SetString(PyExc_TypeError,
                        "paramflags must be a tuple or None");
        return 0;
    }

    len = PyTuple_GET_SIZE(paramflags);
    if (len != PyTuple_GET_SIZE(info->argtypes)) {
        PyErr_SetString(PyExc_ValueError,
                        "paramflags must have the same length as argtypes");
        return 0;
    }

    for (i = 0; i < len; ++i) {
        PyObject *item = PyTuple_GET_ITEM(paramflags, i);
        int flag;
        PyObject *name = Py_None;
        PyObject *defval;
        PyObject *typ;
        if (!PyArg_ParseTuple(item, "i|OO", &flag, &name, &defval) ||
            !(name == Py_None || PyUnicode_Check(name)))
        {
            PyErr_SetString(PyExc_TypeError,
                   "paramflags must be a sequence of (int [,string [,value]]) tuples");
            return 0;
        }
        typ = PyTuple_GET_ITEM(argtypes, i);
        switch (flag & (PARAMFLAG_FIN | PARAMFLAG_FOUT | PARAMFLAG_FLCID)) {
        case 0:
        case PARAMFLAG_FIN:
        case PARAMFLAG_FIN | PARAMFLAG_FLCID:
        case PARAMFLAG_FIN | PARAMFLAG_FOUT:
            break;
        case PARAMFLAG_FOUT:
            if (!_check_outarg_type(st, typ, i+1))
                return 0;
            break;
        default:
            PyErr_Format(PyExc_TypeError,
                         "paramflag value %d not supported",
                         flag);
            return 0;
        }
    }
    return 1;
}

static int
_get_name(PyObject *obj, void *arg)
{
    const char **pname = (const char **)arg;
#ifdef MS_WIN32
    if (PyLong_Check(obj)) {
        /* We have to use MAKEINTRESOURCEA for Windows CE.
           Works on Windows as well, of course.
        */
        *pname = MAKEINTRESOURCEA(PyLong_AsUnsignedLongMask(obj) & 0xFFFF);
        return 1;
    }
#endif
    if (PyBytes_Check(obj)) {
        *pname = PyBytes_AS_STRING(obj);
        return *pname ? 1 : 0;
    }
    if (PyUnicode_Check(obj)) {
        *pname = PyUnicode_AsUTF8(obj);
        return *pname ? 1 : 0;
    }
    PyErr_SetString(PyExc_TypeError,
                    "function name must be string, bytes object or integer");
    return 0;
}


static PyObject *
PyCFuncPtr_FromDll(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    const char *name;
    int (* address)(void);
    PyObject *ftuple;
    PyObject *dll;
    PyObject *obj;
    PyCFuncPtrObject *self;
    void *handle;
    PyObject *paramflags = NULL;

    if (!PyArg_ParseTuple(args, "O|O", &ftuple, &paramflags))
        return NULL;
    if (paramflags == Py_None)
        paramflags = NULL;

    ftuple = PySequence_Tuple(ftuple);
    if (!ftuple)
        /* Here ftuple is a borrowed reference */
        return NULL;

    if (!PyArg_ParseTuple(ftuple, "O&O;illegal func_spec argument",
                          _get_name, &name, &dll))
    {
        Py_DECREF(ftuple);
        return NULL;
    }

#ifdef MS_WIN32
    if (PySys_Audit("ctypes.dlsym",
                    ((uintptr_t)name & ~0xFFFF) ? "Os" : "On",
                    dll, name) < 0) {
        Py_DECREF(ftuple);
        return NULL;
    }
#else
    if (PySys_Audit("ctypes.dlsym", "Os", dll, name) < 0) {
        Py_DECREF(ftuple);
        return NULL;
    }
#endif

    obj = PyObject_GetAttrString(dll, "_handle");
    if (!obj) {
        Py_DECREF(ftuple);
        return NULL;
    }
    if (!PyLong_Check(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "the _handle attribute of the second argument must be an integer");
        Py_DECREF(ftuple);
        Py_DECREF(obj);
        return NULL;
    }
    handle = (void *)PyLong_AsVoidPtr(obj);
    Py_DECREF(obj);
    if (PyErr_Occurred()) {
        PyErr_SetString(PyExc_ValueError,
                        "could not convert the _handle attribute to a pointer");
        Py_DECREF(ftuple);
        return NULL;
    }

#undef USE_DLERROR
#ifdef MS_WIN32
    address = FindAddress(handle, name, (PyObject *)type);
    if (!address) {
        if (!IS_INTRESOURCE(name))
            PyErr_Format(PyExc_AttributeError,
                         "function '%s' not found",
                         name);
        else
            PyErr_Format(PyExc_AttributeError,
                         "function ordinal %d not found",
                         (WORD)(size_t)name);
        Py_DECREF(ftuple);
        return NULL;
    }
#else
    #ifdef __CYGWIN__
        //dlerror() isn't very helpful on cygwin */
    #else
        #define USE_DLERROR
        /* dlerror() always returns the latest error.
         *
         * Clear the previous value before calling dlsym(),
         * to ensure we can tell if our call resulted in an error.
         */
        (void)dlerror();
    #endif
    address = (PPROC)dlsym(handle, name);

    if (!address) {
    #ifdef USE_DLERROR
        const char *dlerr = dlerror();
        if (dlerr) {
            _PyErr_SetLocaleString(PyExc_AttributeError, dlerr);
            Py_DECREF(ftuple);
            return NULL;
        }
    #endif
        PyErr_Format(PyExc_AttributeError, "function '%s' not found", name);
        Py_DECREF(ftuple);
        return NULL;
    }
#endif
#undef USE_DLERROR
    ctypes_state *st = get_module_state_by_def(Py_TYPE(type));
    if (!_validate_paramflags(st, type, paramflags)) {
        Py_DECREF(ftuple);
        return NULL;
    }

    self = (PyCFuncPtrObject *)generic_pycdata_new(st, type, args, kwds);
    if (!self) {
        Py_DECREF(ftuple);
        return NULL;
    }

    self->paramflags = Py_XNewRef(paramflags);

    // No other threads can have this object, no need to
    // lock it.
    *(void **)self->b_ptr = address;
    Py_INCREF(dll);
    Py_DECREF(ftuple);
    if (-1 == KeepRef((CDataObject *)self, 0, dll)) {
        Py_DECREF((PyObject *)self);
        return NULL;
    }

    self->callable = Py_NewRef(self);
    return (PyObject *)self;
}

#ifdef MS_WIN32
static PyObject *
PyCFuncPtr_FromVtblIndex(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyCFuncPtrObject *self;
    int index;
    char *name = NULL;
    PyObject *paramflags = NULL;
    GUID *iid = NULL;
    Py_ssize_t iid_len = 0;

    if (!PyArg_ParseTuple(args, "is|Oz#", &index, &name, &paramflags, &iid, &iid_len))
        return NULL;
    if (paramflags == Py_None)
        paramflags = NULL;

    ctypes_state *st = get_module_state_by_def(Py_TYPE(type));
    if (!_validate_paramflags(st, type, paramflags)) {
        return NULL;
    }
    self = (PyCFuncPtrObject *)generic_pycdata_new(st, type, args, kwds);
    self->index = index + 0x1000;
    self->paramflags = Py_XNewRef(paramflags);
    if (iid_len == sizeof(GUID))
        self->iid = iid;
    return (PyObject *)self;
}
#endif

/*
  PyCFuncPtr_new accepts different argument lists in addition to the standard
  _basespec_ keyword arg:

  one argument form
  "i" - function address
  "O" - must be a callable, creates a C callable function

  two or more argument forms (the third argument is a paramflags tuple)
  "(sO)|..." - (function name, dll object (with an integer handle)), paramflags
  "(iO)|..." - (function ordinal, dll object (with an integer handle)), paramflags
  "is|..." - vtable index, method name, creates callable calling COM vtbl
*/
static PyObject *
PyCFuncPtr_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyCFuncPtrObject *self;
    PyObject *callable;
    CThunkObject *thunk;

    if (PyTuple_GET_SIZE(args) == 0)
        return GenericPyCData_new(type, args, kwds);

    if (1 <= PyTuple_GET_SIZE(args) && PyTuple_Check(PyTuple_GET_ITEM(args, 0)))
        return PyCFuncPtr_FromDll(type, args, kwds);

#ifdef MS_WIN32
    if (2 <= PyTuple_GET_SIZE(args) && PyLong_Check(PyTuple_GET_ITEM(args, 0)))
        return PyCFuncPtr_FromVtblIndex(type, args, kwds);
#endif

    if (1 == PyTuple_GET_SIZE(args)
        && (PyLong_Check(PyTuple_GET_ITEM(args, 0)))) {
        CDataObject *ob;
        void *ptr = PyLong_AsVoidPtr(PyTuple_GET_ITEM(args, 0));
        if (ptr == NULL && PyErr_Occurred())
            return NULL;
        ob = (CDataObject *)GenericPyCData_new(type, args, kwds);
        if (ob == NULL)
            return NULL;
        *(void **)ob->b_ptr = ptr;
        return (PyObject *)ob;
    }

    if (!PyArg_ParseTuple(args, "O", &callable))
        return NULL;
    if (!PyCallable_Check(callable)) {
        PyErr_SetString(PyExc_TypeError,
                        "argument must be callable or integer function address");
        return NULL;
    }

    /* XXX XXX This would allow passing additional options.  For COM
       method *implementations*, we would probably want different
       behaviour than in 'normal' callback functions: return a HRESULT if
       an exception occurs in the callback, and print the traceback not
       only on the console, but also to OutputDebugString() or something
       like that.
    */
/*
    if (kwds && _PyDict_GetItemIdWithError(kwds, &PyId_options)) {
        ...
    }
    else if (PyErr_Occurred()) {
        return NULL;
    }
*/

    ctypes_state *st = get_module_state_by_def(Py_TYPE(type));
    StgInfo *info;
    if (PyStgInfo_FromType(st, (PyObject *)type, &info) < 0) {
        return NULL;
    }
    /* XXXX Fails if we do: 'PyCFuncPtr(lambda x: x)' */
    if (!info || !info->argtypes) {
        PyErr_SetString(PyExc_TypeError,
               "cannot construct instance of this class:"
            " no argtypes");
        return NULL;
    }

    thunk = _ctypes_alloc_callback(st,
                                  callable,
                                  info->argtypes,
                                  info->restype,
                                  info->flags);
    if (!thunk)
        return NULL;

    self = (PyCFuncPtrObject *)generic_pycdata_new(st, type, args, kwds);
    if (self == NULL) {
        Py_DECREF(thunk);
        return NULL;
    }

    self->callable = Py_NewRef(callable);

    self->thunk = thunk;
    *(void **)self->b_ptr = (void *)thunk->pcl_exec;

    Py_INCREF((PyObject *)thunk); /* for KeepRef */
    if (-1 == KeepRef((CDataObject *)self, 0, (PyObject *)thunk)) {
        Py_DECREF((PyObject *)self);
        return NULL;
    }
    return (PyObject *)self;
}


/*
  _byref consumes a refcount to its argument
*/
static PyObject *
_byref(ctypes_state *st, PyObject *obj)
{
    PyCArgObject *parg;

    if (!CDataObject_Check(st, obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "expected CData instance");
        return NULL;
    }

    parg = PyCArgObject_new(st);
    if (parg == NULL) {
        Py_DECREF(obj);
        return NULL;
    }

    parg->tag = 'P';
    parg->pffi_type = &ffi_type_pointer;
    parg->obj = obj;
    parg->value.p = ((CDataObject *)obj)->b_ptr;
    return (PyObject *)parg;
}

static PyObject *
_get_arg(int *pindex, PyObject *name, PyObject *defval, PyObject *inargs, PyObject *kwds)
{
    PyObject *v;

    if (*pindex < PyTuple_GET_SIZE(inargs)) {
        v = PyTuple_GET_ITEM(inargs, *pindex);
        ++*pindex;
        return Py_NewRef(v);
    }
    if (kwds && name) {
        if (PyDict_GetItemRef(kwds, name, &v) < 0) {
            return NULL;
        }
        if (v) {
            ++*pindex;
            return v;
        }
    }
    if (defval) {
        return Py_NewRef(defval);
    }
    /* we can't currently emit a better error message */
    if (name)
        PyErr_Format(PyExc_TypeError,
                     "required argument '%S' missing", name);
    else
        PyErr_Format(PyExc_TypeError,
                     "not enough arguments");
    return NULL;
}

/*
 This function implements higher level functionality plus the ability to call
 functions with keyword arguments by looking at parameter flags.  parameter
 flags is a tuple of 1, 2 or 3-tuples.  The first entry in each is an integer
 specifying the direction of the data transfer for this parameter - 'in',
 'out' or 'inout' (zero means the same as 'in').  The second entry is the
 parameter name, and the third is the default value if the parameter is
 missing in the function call.

 This function builds and returns a new tuple 'callargs' which contains the
 parameters to use in the call.  Items on this tuple are copied from the
 'inargs' tuple for 'in' and 'in, out' parameters, and constructed from the
 'argtypes' tuple for 'out' parameters.  It also calculates numretvals which
 is the number of return values for the function, outmask/inoutmask are
 bitmasks containing indexes into the callargs tuple specifying which
 parameters have to be returned.  _build_result builds the return value of the
 function.
*/
static PyObject *
_build_callargs(ctypes_state *st, PyCFuncPtrObject *self, PyObject *argtypes,
                PyObject *inargs, PyObject *kwds,
                int *poutmask, int *pinoutmask, unsigned int *pnumretvals)
{
    PyObject *paramflags = self->paramflags;
    PyObject *callargs;
    Py_ssize_t i, len;
    int inargs_index = 0;
    /* It's a little bit difficult to determine how many arguments the
    function call requires/accepts.  For simplicity, we count the consumed
    args and compare this to the number of supplied args. */
    Py_ssize_t actual_args;

    *poutmask = 0;
    *pinoutmask = 0;
    *pnumretvals = 0;

    /* Trivial cases, where we either return inargs itself, or a slice of it. */
    if (argtypes == NULL || paramflags == NULL || PyTuple_GET_SIZE(argtypes) == 0) {
#ifdef MS_WIN32
        if (self->index)
            return PyTuple_GetSlice(inargs, 1, PyTuple_GET_SIZE(inargs));
#endif
        return Py_NewRef(inargs);
    }

    len = PyTuple_GET_SIZE(argtypes);
    callargs = PyTuple_New(len); /* the argument tuple we build */
    if (callargs == NULL)
        return NULL;

#ifdef MS_WIN32
    /* For a COM method, skip the first arg */
    if (self->index) {
        inargs_index = 1;
    }
#endif
    for (i = 0; i < len; ++i) {
        PyObject *item = PyTuple_GET_ITEM(paramflags, i);
        PyObject *ob;
        unsigned int flag;
        PyObject *name = NULL;
        PyObject *defval = NULL;

        /* This way seems to be ~2 us faster than the PyArg_ParseTuple
           calls below. */
        /* We HAVE already checked that the tuple can be parsed with "i|ZO", so... */
        Py_ssize_t tsize = PyTuple_GET_SIZE(item);
        flag = PyLong_AsUnsignedLongMask(PyTuple_GET_ITEM(item, 0));
        name = tsize > 1 ? PyTuple_GET_ITEM(item, 1) : NULL;
        defval = tsize > 2 ? PyTuple_GET_ITEM(item, 2) : NULL;

        switch (flag & (PARAMFLAG_FIN | PARAMFLAG_FOUT | PARAMFLAG_FLCID)) {
        case PARAMFLAG_FIN | PARAMFLAG_FLCID:
            /* ['in', 'lcid'] parameter.  Always taken from defval,
             if given, else the integer 0. */
            if (defval == NULL) {
                defval = _PyLong_GetZero();
            }
            Py_INCREF(defval);
            PyTuple_SET_ITEM(callargs, i, defval);
            break;
        case (PARAMFLAG_FIN | PARAMFLAG_FOUT):
            *pinoutmask |= (1 << i); /* mark as inout arg */
            (*pnumretvals)++;
            _Py_FALLTHROUGH;
        case 0:
        case PARAMFLAG_FIN:
            /* 'in' parameter.  Copy it from inargs. */
            ob =_get_arg(&inargs_index, name, defval, inargs, kwds);
            if (ob == NULL)
                goto error;
            PyTuple_SET_ITEM(callargs, i, ob);
            break;
        case PARAMFLAG_FOUT:
            /* XXX Refactor this code into a separate function. */
            /* 'out' parameter.
               argtypes[i] must be a POINTER to a c type.

               Cannot by supplied in inargs, but a defval will be used
               if available.  XXX Should we support getting it from kwds?
            */
            if (defval) {
                /* XXX Using mutable objects as defval will
                   make the function non-threadsafe, unless we
                   copy the object in each invocation */
                Py_INCREF(defval);
                PyTuple_SET_ITEM(callargs, i, defval);
                *poutmask |= (1 << i); /* mark as out arg */
                (*pnumretvals)++;
                break;
            }
            ob = PyTuple_GET_ITEM(argtypes, i);
            StgInfo *info;
            if (PyStgInfo_FromType(st, ob, &info) < 0) {
                goto error;
            }
            if (info == NULL) {
                /* Cannot happen: _validate_paramflags()
                  would not accept such an object */
                PyErr_Format(PyExc_RuntimeError,
                             "NULL stginfo unexpected");
                goto error;
            }
            if (PyUnicode_Check(info->proto)) {
                PyErr_Format(
                    PyExc_TypeError,
                    "%s 'out' parameter must be passed as default value",
                    ((PyTypeObject *)ob)->tp_name);
                goto error;
            }
            if (PyCArrayTypeObject_Check(st, ob)) {
                ob = _PyObject_CallNoArgs(ob);
            }
            else {
                /* Create an instance of the pointed-to type */
                ob = _PyObject_CallNoArgs(info->proto);
            }
            /*
               XXX Is the following correct any longer?
               We must not pass a byref() to the array then but
               the array instance itself. Then, we cannot retrieve
               the result from the PyCArgObject.
            */
            if (ob == NULL)
                goto error;
            /* The .from_param call that will occur later will pass this
               as a byref parameter. */
            PyTuple_SET_ITEM(callargs, i, ob);
            *poutmask |= (1 << i); /* mark as out arg */
            (*pnumretvals)++;
            break;
        default:
            PyErr_Format(PyExc_ValueError,
                         "paramflag %u not yet implemented", flag);
            goto error;
            break;
        }
    }

    /* We have counted the arguments we have consumed in 'inargs_index'.  This
       must be the same as len(inargs) + len(kwds), otherwise we have
       either too much or not enough arguments. */

    actual_args = PyTuple_GET_SIZE(inargs) + (kwds ? PyDict_GET_SIZE(kwds) : 0);
    if (actual_args != inargs_index) {
        /* When we have default values or named parameters, this error
           message is misleading.  See unittests/test_paramflags.py
         */
        PyErr_Format(PyExc_TypeError,
                     "call takes exactly %d arguments (%zd given)",
                     inargs_index, actual_args);
        goto error;
    }

    /* outmask is a bitmask containing indexes into callargs.  Items at
       these indexes contain values to return.
     */
    return callargs;
  error:
    Py_DECREF(callargs);
    return NULL;
}

/* See also:
   http://msdn.microsoft.com/library/en-us/com/html/769127a1-1a14-4ed4-9d38-7cf3e571b661.asp
*/
/*
  Build return value of a function.

  Consumes the refcount on result and callargs.
*/
static PyObject *
_build_result(PyObject *result, PyObject *callargs,
              int outmask, int inoutmask, unsigned int numretvals)
{
    unsigned int i, index;
    int bit;
    PyObject *tup = NULL;

    if (callargs == NULL)
        return result;
    if (result == NULL || numretvals == 0) {
        Py_DECREF(callargs);
        return result;
    }
    Py_DECREF(result);

    /* tup will not be allocated if numretvals == 1 */
    /* allocate tuple to hold the result */
    if (numretvals > 1) {
        tup = PyTuple_New(numretvals);
        if (tup == NULL) {
            Py_DECREF(callargs);
            return NULL;
        }
    }

    index = 0;
    for (bit = 1, i = 0; i < 32; ++i, bit <<= 1) {
        PyObject *v;
        if (bit & inoutmask) {
            v = PyTuple_GET_ITEM(callargs, i);
            Py_INCREF(v);
            if (numretvals == 1) {
                Py_DECREF(callargs);
                return v;
            }
            PyTuple_SET_ITEM(tup, index, v);
            index++;
        } else if (bit & outmask) {

            v = PyTuple_GET_ITEM(callargs, i);
            v = PyObject_CallMethodNoArgs(v, &_Py_ID(__ctypes_from_outparam__));
            if (v == NULL || numretvals == 1) {
                Py_DECREF(callargs);
                return v;
            }
            PyTuple_SET_ITEM(tup, index, v);
            index++;
        }
        if (index == numretvals)
            break;
    }

    Py_DECREF(callargs);
    return tup;
}

static PyObject *
PyCFuncPtr_call_lock_held(PyObject *op, PyObject *inargs, PyObject *kwds)
{
    PyObject *restype;
    PyObject *converters;
    PyObject *checker;
    PyObject *argtypes;
    PyObject *result;
    PyObject *callargs;
    PyObject *errcheck;
#ifdef MS_WIN32
    IUnknown *piunk = NULL;
#endif
    void *pProc = NULL;
    PyCFuncPtrObject *self = _PyCFuncPtrObject_CAST(op);

    int inoutmask;
    int outmask;
    unsigned int numretvals;

    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));
    StgInfo *info;
    if (PyStgInfo_FromObject(st, op, &info) < 0) {
        return NULL;
    }
    assert(info); /* Cannot be NULL for PyCFuncPtrObject instances */

    restype = self->restype ? self->restype : info->restype;
    converters = self->converters ? self->converters : info->converters;
    checker = self->checker ? self->checker : info->checker;
    argtypes = self->argtypes ? self->argtypes : info->argtypes;
/* later, we probably want to have an errcheck field in stginfo */
    errcheck = self->errcheck /* ? self->errcheck : info->errcheck */;


    pProc = *(void **)self->b_ptr;
#ifdef MS_WIN32
    if (self->index) {
        /* It's a COM method */
        CDataObject *this;
        this = (CDataObject *)PyTuple_GetItem(inargs, 0); /* borrowed ref! */
        if (!this) {
            PyErr_SetString(PyExc_ValueError,
                            "native com method call without 'this' parameter");
            return NULL;
        }
        if (!CDataObject_Check(st, this)) {
            PyErr_SetString(PyExc_TypeError,
                            "Expected a COM this pointer as first argument");
            return NULL;
        }
        /* there should be more checks? No, in Python */
        /* First arg is a pointer to an interface instance */
        if (!this->b_ptr || *(void **)this->b_ptr == NULL) {
            PyErr_SetString(PyExc_ValueError,
                            "NULL COM pointer access");
            return NULL;
        }
        piunk = *(IUnknown **)this->b_ptr;
        if (NULL == piunk->lpVtbl) {
            PyErr_SetString(PyExc_ValueError,
                            "COM method call without VTable");
            return NULL;
        }
        pProc = ((void **)piunk->lpVtbl)[self->index - 0x1000];
    }
#endif
    callargs = _build_callargs(st, self, argtypes,
                               inargs, kwds,
                               &outmask, &inoutmask, &numretvals);
    if (callargs == NULL)
        return NULL;

    if (converters) {
        int required = Py_SAFE_DOWNCAST(PyTuple_GET_SIZE(converters),
                                        Py_ssize_t, int);
        int actual = Py_SAFE_DOWNCAST(PyTuple_GET_SIZE(callargs),
                                      Py_ssize_t, int);

        if ((info->flags & FUNCFLAG_CDECL) == FUNCFLAG_CDECL) {
            /* For cdecl functions, we allow more actual arguments
               than the length of the argtypes tuple.
            */
            if (required > actual) {
                Py_DECREF(callargs);
                PyErr_Format(PyExc_TypeError,
              "this function takes at least %d argument%s (%d given)",
                                 required,
                                 required == 1 ? "" : "s",
                                 actual);
                return NULL;
            }
        } else if (required != actual) {
            Py_DECREF(callargs);
            PyErr_Format(PyExc_TypeError,
                 "this function takes %d argument%s (%d given)",
                     required,
                     required == 1 ? "" : "s",
                     actual);
            return NULL;
        }
    }

    result = _ctypes_callproc(st,
                       pProc,
                       callargs,
#ifdef MS_WIN32
                       piunk,
                       self->iid,
#endif
                       info->flags,
                       converters,
                       restype,
                       checker);
/* The 'errcheck' protocol */
    if (result != NULL && errcheck) {
        PyObject *v = PyObject_CallFunctionObjArgs(errcheck,
                                                   result,
                                                   self,
                                                   callargs,
                                                   NULL);
        /* If the errcheck function failed, return NULL.
           If the errcheck function returned callargs unchanged,
           continue normal processing.
           If the errcheck function returned something else,
           use that as result.
        */
        if (v == NULL || v != callargs) {
            Py_DECREF(result);
            Py_DECREF(callargs);
            return v;
        }
        Py_DECREF(v);
    }

    return _build_result(result, callargs,
                         outmask, inoutmask, numretvals);
}

static PyObject *
PyCFuncPtr_call(PyObject *op, PyObject *inargs, PyObject *kwds)
{
    PyObject *result;
    Py_BEGIN_CRITICAL_SECTION(op);
    result = PyCFuncPtr_call_lock_held(op, inargs, kwds);
    Py_END_CRITICAL_SECTION();
    return result;
}

static int
PyCFuncPtr_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyCFuncPtrObject *self = _PyCFuncPtrObject_CAST(op);
    Py_VISIT(self->callable);
    Py_VISIT(self->restype);
    Py_VISIT(self->checker);
    Py_VISIT(self->errcheck);
    Py_VISIT(self->argtypes);
    Py_VISIT(self->converters);
    Py_VISIT(self->paramflags);
    Py_VISIT(self->thunk);
    return PyCData_traverse(op, visit, arg);
}

static int
PyCFuncPtr_clear(PyObject *op)
{
    PyCFuncPtrObject *self = _PyCFuncPtrObject_CAST(op);
    Py_CLEAR(self->callable);
    Py_CLEAR(self->restype);
    Py_CLEAR(self->checker);
    Py_CLEAR(self->errcheck);
    Py_CLEAR(self->argtypes);
    Py_CLEAR(self->converters);
    Py_CLEAR(self->paramflags);
    Py_CLEAR(self->thunk);
    return PyCData_clear(op);
}

static void
PyCFuncPtr_dealloc(PyObject *self)
{
    PyObject_GC_UnTrack(self);
    (void)PyCFuncPtr_clear(self);
    PyTypeObject *type = Py_TYPE(self);
    type->tp_free(self);
    Py_DECREF(type);
}

static PyObject *
PyCFuncPtr_repr(PyObject *op)
{
    PyCFuncPtrObject *self = _PyCFuncPtrObject_CAST(op);
#ifdef MS_WIN32
    if (self->index)
        return PyUnicode_FromFormat("<COM method offset %d: %s at %p>",
                                   self->index - 0x1000,
                                   Py_TYPE(self)->tp_name,
                                   self);
#endif
    return PyUnicode_FromFormat("<%s object at %p>",
                               Py_TYPE(self)->tp_name,
                               self);
}

static int
PyCFuncPtr_bool(PyObject *op)
{
    PyCFuncPtrObject *self = _PyCFuncPtrObject_CAST(op);
    return ((*(void **)self->b_ptr != NULL)
#ifdef MS_WIN32
        || (self->index != 0)
#endif
        );
}

static PyType_Slot pycfuncptr_slots[] = {
    {Py_tp_dealloc, PyCFuncPtr_dealloc},
    {Py_tp_repr, PyCFuncPtr_repr},
    {Py_tp_call, PyCFuncPtr_call},
    {Py_tp_doc, PyDoc_STR("Function Pointer")},
    {Py_tp_traverse, PyCFuncPtr_traverse},
    {Py_tp_clear, PyCFuncPtr_clear},
    {Py_tp_getset, PyCFuncPtr_getsets},
    {Py_tp_new, PyCFuncPtr_new},
    {Py_bf_getbuffer, PyCData_NewGetBuffer},
    {Py_nb_bool, PyCFuncPtr_bool},
    {0, NULL},
};

static PyType_Spec pycfuncptr_spec = {
    .name = "_ctypes.CFuncPtr",
    .basicsize = sizeof(PyCFuncPtrObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pycfuncptr_slots,
};

/*****************************************************************/
/*
  Struct_Type
*/
/*
  This function is called to initialize a Structure or Union with positional
  arguments. It calls itself recursively for all Structure or Union base
  classes, then retrieves the _fields_ member to associate the argument
  position with the correct field name.

  Returns -1 on error, or the index of next argument on success.
 */
static Py_ssize_t
_init_pos_args(PyObject *self, PyTypeObject *type,
               PyObject *args, PyObject *kwds,
               Py_ssize_t index)
{
    PyObject *fields;
    Py_ssize_t i;

    ctypes_state *st = get_module_state_by_def(Py_TYPE(type));
    StgInfo *baseinfo;
    if (PyStgInfo_FromType(st, (PyObject *)type->tp_base, &baseinfo) < 0) {
        return -1;
    }
    if (baseinfo) {
        index = _init_pos_args(self, type->tp_base,
                               args, kwds,
                               index);
        if (index == -1)
            return -1;
    }

    StgInfo *info;
    if (PyStgInfo_FromType(st, (PyObject *)type, &info) < 0) {
        return -1;
    }
    assert(info);

    PyObject *attrdict = PyType_GetDict(type);
    assert(attrdict);

    fields = PyDict_GetItemWithError((PyObject *)attrdict, &_Py_ID(_fields_));
    Py_CLEAR(attrdict);
    if (fields == NULL) {
        if (PyErr_Occurred()) {
            return -1;
        }
        return index;
    }

    for (i = index;
         i < info->length && i < PyTuple_GET_SIZE(args);
         ++i) {
        PyObject *pair = PySequence_GetItem(fields, i - index);
        PyObject *name, *val;
        int res;
        if (!pair)
            return -1;
        name = PySequence_GetItem(pair, 0);
        if (!name) {
            Py_DECREF(pair);
            return -1;
        }
        val = PyTuple_GET_ITEM(args, i);
        if (kwds) {
            res = PyDict_Contains(kwds, name);
            if (res != 0) {
                if (res > 0) {
                    PyErr_Format(PyExc_TypeError,
                                 "duplicate values for field %R",
                                 name);
                }
                Py_DECREF(pair);
                Py_DECREF(name);
                return -1;
            }
        }

        res = PyObject_SetAttr(self, name, val);
        Py_DECREF(pair);
        Py_DECREF(name);
        if (res == -1)
            return -1;
    }
    return info->length;
}

static int
Struct_init(PyObject *self, PyObject *args, PyObject *kwds)
{
/* Optimization possible: Store the attribute names _fields_[x][0]
 * in C accessible fields somewhere ?
 */
    if (!PyTuple_Check(args)) {
        PyErr_SetString(PyExc_TypeError,
                        "args not a tuple?");
        return -1;
    }
    if (PyTuple_GET_SIZE(args)) {
        Py_ssize_t res = _init_pos_args(self, Py_TYPE(self),
                                        args, kwds, 0);
        if (res == -1)
            return -1;
        if (res < PyTuple_GET_SIZE(args)) {
            PyErr_SetString(PyExc_TypeError,
                            "too many initializers");
            return -1;
        }
    }

    if (kwds) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while(PyDict_Next(kwds, &pos, &key, &value)) {
            if (-1 == PyObject_SetAttr(self, key, value))
                return -1;
        }
    }
    return 0;
}

static PyType_Slot pycstruct_slots[] = {
    {Py_tp_doc, PyDoc_STR("Structure base class")},
    {Py_tp_init, Struct_init},
    {Py_tp_new, GenericPyCData_new},
    {Py_bf_getbuffer, PyCData_NewGetBuffer},
    {0, NULL},
};

static PyType_Spec pycstruct_spec = {
    .name = "_ctypes.Structure",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pycstruct_slots,
};

static PyType_Slot pycunion_slots[] = {
    {Py_tp_doc, PyDoc_STR("Union base class")},
    {Py_tp_init, Struct_init},
    {Py_tp_new, GenericPyCData_new},
    {Py_bf_getbuffer, PyCData_NewGetBuffer},
    {0, NULL},
};

static PyType_Spec pycunion_spec = {
    .name = "_ctypes.Union",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pycunion_slots,
};


/******************************************************************/
/*
  PyCArray_Type
*/
static int
Array_init(PyObject *self, PyObject *args, PyObject *kw)
{
    Py_ssize_t i;
    Py_ssize_t n;

    if (!PyTuple_Check(args)) {
        PyErr_SetString(PyExc_TypeError,
                        "args not a tuple?");
        return -1;
    }
    n = PyTuple_GET_SIZE(args);
    for (i = 0; i < n; ++i) {
        PyObject *v;
        v = PyTuple_GET_ITEM(args, i);
        if (-1 == PySequence_SetItem(self, i, v))
            return -1;
    }
    return 0;
}

static PyObject *
Array_item(PyObject *myself, Py_ssize_t index)
{
    CDataObject *self = _CDataObject_CAST(myself);
    Py_ssize_t offset, size;

    if (index < 0 || index >= self->b_length) {
        PyErr_SetString(PyExc_IndexError,
                        "invalid index");
        return NULL;
    }

    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));
    StgInfo *stginfo;
    if (PyStgInfo_FromObject(st, myself, &stginfo) < 0) {
        return NULL;
    }

    /* Would it be clearer if we got the item size from
       stginfo->proto's stginfo?
    */
    size = stginfo->size / stginfo->length;
    offset = index * size;

    return PyCData_get(st, stginfo->proto, stginfo->getfunc, myself,
                       index, size, self->b_ptr + offset);
}

static PyObject *
Array_subscript(PyObject *myself, PyObject *item)
{
    CDataObject *self = _CDataObject_CAST(myself);

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += self->b_length;
        return Array_item(myself, i);
    }
    else if (PySlice_Check(item)) {
        PyObject *proto;
        PyObject *np;
        Py_ssize_t start, stop, step, slicelen, i;
        size_t cur;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelen = PySlice_AdjustIndices(self->b_length, &start, &stop, step);

        ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));
        StgInfo *stginfo;
        if (PyStgInfo_FromObject(st, myself, &stginfo) < 0) {
            return NULL;
        }
        assert(stginfo); /* Cannot be NULL for array object instances */
        proto = stginfo->proto;
        StgInfo *iteminfo;
        if (PyStgInfo_FromType(st, proto, &iteminfo) < 0) {
            return NULL;
        }
        assert(iteminfo); /* proto is the item type of the array, a
                             ctypes type, so this cannot be NULL */

        if (iteminfo->getfunc == _ctypes_get_fielddesc("c")->getfunc) {
            char *ptr = (char *)self->b_ptr;
            char *dest;

            if (slicelen <= 0)
                return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
            if (step == 1) {
                PyObject *res;
                LOCK_PTR(self);
                res = PyBytes_FromStringAndSize(ptr + start,
                                                slicelen);
                UNLOCK_PTR(self);
                return res;
            }
            dest = (char *)PyMem_Malloc(slicelen);

            if (dest == NULL)
                return PyErr_NoMemory();

            LOCK_PTR(self);
            for (cur = start, i = 0; i < slicelen;
                 cur += step, i++) {
                dest[i] = ptr[cur];
            }
            UNLOCK_PTR(self);

            np = PyBytes_FromStringAndSize(dest, slicelen);
            PyMem_Free(dest);
            return np;
        }
        if (iteminfo->getfunc == _ctypes_get_fielddesc("u")->getfunc) {
            wchar_t *ptr = (wchar_t *)self->b_ptr;
            wchar_t *dest;

            if (slicelen <= 0)
                return Py_GetConstant(Py_CONSTANT_EMPTY_STR);
            if (step == 1) {
                PyObject *res;
                LOCK_PTR(self);
                res = PyUnicode_FromWideChar(ptr + start,
                                                       slicelen);
                UNLOCK_PTR(self);
                return res;
            }

            dest = PyMem_New(wchar_t, slicelen);
            if (dest == NULL) {
                PyErr_NoMemory();
                return NULL;
            }

            LOCK_PTR(self);
            for (cur = start, i = 0; i < slicelen;
                 cur += step, i++) {
                dest[i] = ptr[cur];
            }
            UNLOCK_PTR(self);

            np = PyUnicode_FromWideChar(dest, slicelen);
            PyMem_Free(dest);
            return np;
        }

        np = PyList_New(slicelen);
        if (np == NULL)
            return NULL;

        for (cur = start, i = 0; i < slicelen;
             cur += step, i++) {
            PyObject *v = Array_item(myself, cur);
            if (v == NULL) {
                Py_DECREF(np);
                return NULL;
            }
            PyList_SET_ITEM(np, i, v);
        }
        return np;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "indices must be integers");
        return NULL;
    }

}

static int
Array_ass_item(PyObject *myself, Py_ssize_t index, PyObject *value)
{
    CDataObject *self = _CDataObject_CAST(myself);
    Py_ssize_t size, offset;
    char *ptr;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Array does not support item deletion");
        return -1;
    }

    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));
    StgInfo *stginfo;
    if (PyStgInfo_FromObject(st, myself, &stginfo) < 0) {
        return -1;
    }
    assert(stginfo); /* Cannot be NULL for array object instances */

    if (index < 0 || index >= stginfo->length) {
        PyErr_SetString(PyExc_IndexError,
                        "invalid index");
        return -1;
    }
    size = stginfo->size / stginfo->length;
    offset = index * size;
    ptr = self->b_ptr + offset;

    return PyCData_set(st, myself, stginfo->proto, stginfo->setfunc, value,
                       index, size, ptr);
}

static int
Array_ass_subscript(PyObject *myself, PyObject *item, PyObject *value)
{
    CDataObject *self = _CDataObject_CAST(myself);

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Array does not support item deletion");
        return -1;
    }

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred())
            return -1;
        if (i < 0)
            i += self->b_length;
        return Array_ass_item(myself, i, value);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelen, otherlen, i;
        size_t cur;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return -1;
        }
        slicelen = PySlice_AdjustIndices(self->b_length, &start, &stop, step);
        if ((step < 0 && start < stop) ||
            (step > 0 && start > stop))
            stop = start;

        otherlen = PySequence_Length(value);
        if (otherlen != slicelen) {
            PyErr_SetString(PyExc_ValueError,
                "Can only assign sequence of same size");
            return -1;
        }
        for (cur = start, i = 0; i < otherlen; cur += step, i++) {
            PyObject *item = PySequence_GetItem(value, i);
            int result;
            if (item == NULL)
                return -1;
            result = Array_ass_item(myself, cur, item);
            Py_DECREF(item);
            if (result == -1)
                return -1;
        }
        return 0;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "indices must be integer");
        return -1;
    }
}

static Py_ssize_t
Array_length(PyObject *myself)
{
    CDataObject *self = _CDataObject_CAST(myself);
    return self->b_length;
}

static PyMethodDef Array_methods[] = {
    {"__class_getitem__",    Py_GenericAlias,
    METH_O|METH_CLASS,       PyDoc_STR("See PEP 585")},
    { NULL, NULL }
};

PyDoc_STRVAR(array_doc,
"Abstract base class for arrays.\n"
"\n"
"The recommended way to create concrete array types is by multiplying any\n"
"ctypes data type with a non-negative integer. Alternatively, you can subclass\n"
"this type and define _length_ and _type_ class variables. Array elements can\n"
"be read and written using standard subscript and slice accesses for slice\n"
"reads, the resulting object is not itself an Array."
);

static PyType_Slot pycarray_slots[] = {
    {Py_tp_doc, (char*)array_doc},
    {Py_tp_methods, Array_methods},
    {Py_tp_init, Array_init},
    {Py_tp_new, GenericPyCData_new},
    {Py_bf_getbuffer, PyCData_NewGetBuffer},
    {Py_sq_length, Array_length},
    {Py_sq_item, Array_item},
    {Py_sq_ass_item, Array_ass_item},
    {Py_mp_length, Array_length},
    {Py_mp_subscript, Array_subscript},
    {Py_mp_ass_subscript, Array_ass_subscript},
    {0, NULL},
};

static PyType_Spec pycarray_spec = {
    .name = "_ctypes.Array",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pycarray_slots,
};

PyObject *
PyCArrayType_from_ctype(ctypes_state *st, PyObject *itemtype, Py_ssize_t length)
{
    PyObject *key;
    char name[256];
    PyObject *len;

    assert(st->array_cache != NULL);
    len = PyLong_FromSsize_t(length);
    if (len == NULL)
        return NULL;
    key = PyTuple_Pack(2, itemtype, len);
    Py_DECREF(len);
    if (!key)
        return NULL;

    PyObject *result;
    if (_PyDict_GetItemProxy(st->array_cache, key, &result) != 0) {
        // found or error
        Py_DECREF(key);
        return result;
    }
    // not found
    if (!PyType_Check(itemtype)) {
        PyErr_SetString(PyExc_TypeError,
                        "Expected a type object");
        Py_DECREF(key);
        return NULL;
    }
#ifdef MS_WIN64
    sprintf(name, "%.200s_Array_%Id",
        ((PyTypeObject *)itemtype)->tp_name, length);
#else
    sprintf(name, "%.200s_Array_%ld",
        ((PyTypeObject *)itemtype)->tp_name, (long)length);
#endif
    result = PyObject_CallFunction((PyObject *)st->PyCArrayType_Type,
                                   "s(O){s:n,s:O}",
                                   name,
                                   st->PyCArray_Type,
                                   "_length_",
                                   length,
                                   "_type_",
                                   itemtype
        );
    if (result == NULL) {
        Py_DECREF(key);
        return NULL;
    }
    if (PyDict_SetItemProxy(st, st->array_cache, key, result) < 0) {
        Py_DECREF(key);
        Py_DECREF(result);
        return NULL;
    }
    Py_DECREF(key);
    return result;
}


/******************************************************************/
/*
  Simple_Type
*/

/*[clinic input]
class _ctypes.Simple "CDataObject *" "clinic_state()->Simple_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e0493451fecf8cd4]*/

/*[clinic input]
@critical_section
@setter
_ctypes.Simple.value
[clinic start generated code]*/

static int
_ctypes_Simple_value_set_impl(CDataObject *self, PyObject *value)
/*[clinic end generated code: output=f267186118939863 input=977af9dc9e71e857]*/
{
    PyObject *result;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "can't delete attribute");
        return -1;
    }

    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));
    StgInfo *info;
    if (PyStgInfo_FromObject(st, (PyObject *)self, &info) < 0) {
        return -1;
    }
    assert(info); /* Cannot be NULL for CDataObject instances */
    assert(info->setfunc);

    result = info->setfunc(self->b_ptr, value, info->size);
    if (!result)
        return -1;

    /* consumes the refcount the setfunc returns */
    return KeepRef(self, 0, result);
}


static int
Simple_init(PyObject *self, PyObject *args, PyObject *kw)
{
    PyObject *value = NULL;
    if (!PyArg_UnpackTuple(args, "__init__", 0, 1, &value))
        return -1;
    if (value)
        return _ctypes_Simple_value_set(self, value, NULL);
    return 0;
}


/*[clinic input]
@critical_section
@getter
_ctypes.Simple.value
[clinic start generated code]*/

static PyObject *
_ctypes_Simple_value_get_impl(CDataObject *self)
/*[clinic end generated code: output=ce5a26570830a243 input=3ed3f735cec89282]*/
{
    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));
    StgInfo *info;
    if (PyStgInfo_FromObject(st, (PyObject *)self, &info) < 0) {
        return NULL;
    }
    assert(info); /* Cannot be NULL for CDataObject instances */
    assert(info->getfunc);
    PyObject *res;
    res = info->getfunc(self->b_ptr, self->b_size);
    return res;
}

static PyGetSetDef Simple_getsets[] = {
    _CTYPES_SIMPLE_VALUE_GETSETDEF
    { NULL, NULL }
};

/*[clinic input]
_ctypes.Simple.__ctypes_from_outparam__ as Simple_from_outparm

    self: self
    cls: defining_class
    /
[clinic start generated code]*/

static PyObject *
Simple_from_outparm_impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=6c61d90da8aa9b4f input=0f362803fb4629d5]*/
{
    ctypes_state *st = get_module_state_by_class(cls);
    if (_ctypes_simple_instance(st, (PyObject *)Py_TYPE(self))) {
        return Py_NewRef(self);
    }
    /* call stginfo->getfunc */
    return _ctypes_Simple_value_get(self, NULL);
}

static PyMethodDef Simple_methods[] = {
    SIMPLE_FROM_OUTPARM_METHODDEF
    { NULL, NULL },
};

static int
Simple_bool(PyObject *op)
{
    int cmp;
    CDataObject *self = _CDataObject_CAST(op);
    LOCK_PTR(self);
    cmp = memcmp(self->b_ptr, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", self->b_size);
    UNLOCK_PTR(self);
    return cmp;
}

/* "%s(%s)" % (self.__class__.__name__, self.value) */
static PyObject *
Simple_repr(PyObject *self)
{
    PyObject *val, *result;
    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));

    if (Py_TYPE(self)->tp_base != st->Simple_Type) {
        return PyUnicode_FromFormat("<%s object at %p>",
                                   Py_TYPE(self)->tp_name, self);
    }

    val = _ctypes_Simple_value_get(self, NULL);
    if (val == NULL)
        return NULL;

    result = PyUnicode_FromFormat("%s(%R)",
                                  Py_TYPE(self)->tp_name, val);
    Py_DECREF(val);
    return result;
}

static PyType_Slot pycsimple_slots[] = {
    {Py_tp_repr, &Simple_repr},
    {Py_tp_doc, PyDoc_STR("XXX to be provided")},
    {Py_tp_methods, Simple_methods},
    {Py_tp_getset, Simple_getsets},
    {Py_tp_init, Simple_init},
    {Py_tp_new, GenericPyCData_new},
    {Py_bf_getbuffer, PyCData_NewGetBuffer},
    {Py_nb_bool, Simple_bool},
    {0, NULL},
};

static PyType_Spec pycsimple_spec = {
    .name = "_ctypes._SimpleCData",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pycsimple_slots,
};


/******************************************************************/
/*
  PyCPointer_Type
*/
static PyObject *
Pointer_item_lock_held(PyObject *myself, Py_ssize_t index)
{
    CDataObject *self = _CDataObject_CAST(myself);
    Py_ssize_t size;
    Py_ssize_t offset;
    PyObject *proto;
    void *deref = *(void **)self->b_ptr;

    if (deref == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "NULL pointer access");
        return NULL;
    }

    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(myself)));
    StgInfo *stginfo;
    if (PyStgInfo_FromObject(st, myself, &stginfo) < 0) {
        return NULL;
    }
    assert(stginfo); /* Cannot be NULL for pointer object instances */

    proto = stginfo->proto;
    assert(proto);

    StgInfo *iteminfo;
    if (PyStgInfo_FromType(st, proto, &iteminfo) < 0) {
        return NULL;
    }
    assert(iteminfo); /* proto is the item type of the pointer, a ctypes
                         type, so this cannot be NULL */

    size = iteminfo->size;
    offset = index * iteminfo->size;

    return PyCData_get(st, proto, stginfo->getfunc, myself,
                       index, size, (char *)((char *)deref + offset));
}

static PyObject *
Pointer_item(PyObject *myself, Py_ssize_t index)
{
    CDataObject *self = _CDataObject_CAST(myself);
    PyObject *res;
    // TODO: The plan is to make LOCK_PTR() a mutex instead of a critical
    // section someday, so when that happens, this needs to get refactored
    // to be re-entrant safe.
    // This goes for all the locks here.
    LOCK_PTR(self);
    res = Pointer_item_lock_held(myself, index);
    UNLOCK_PTR(self);
    return res;
}

static int
Pointer_ass_item_lock_held(PyObject *myself, Py_ssize_t index, PyObject *value)
{
    CDataObject *self = _CDataObject_CAST(myself);
    Py_ssize_t size;
    Py_ssize_t offset;
    PyObject *proto;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Pointer does not support item deletion");
        return -1;
    }

    void *deref = *(void **)self->b_ptr;
    if (deref == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "NULL pointer access");
        return -1;
    }

    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(myself)));
    StgInfo *stginfo;
    if (PyStgInfo_FromObject(st, myself, &stginfo) < 0) {
        return -1;
    }
    assert(stginfo); /* Cannot be NULL for pointer instances */

    proto = stginfo->proto;
    assert(proto);

    StgInfo *iteminfo;
    if (PyStgInfo_FromType(st, proto, &iteminfo) < 0) {
        return -1;
    }
    assert(iteminfo); /* Cannot be NULL because the itemtype of a pointer
                         is always a ctypes type */

    size = iteminfo->size;
    offset = index * iteminfo->size;

    return PyCData_set(st, myself, proto, stginfo->setfunc, value,
                       index, size, ((char *)deref + offset));
}

static int
Pointer_ass_item(PyObject *myself, Py_ssize_t index, PyObject *value)
{
    CDataObject *self = _CDataObject_CAST(myself);
    int res;
    LOCK_PTR(self);
    res = Pointer_ass_item_lock_held(myself, index, value);
    UNLOCK_PTR(self);
    return res;
}

static PyObject *
Pointer_get_contents_lock_held(PyObject *self, void *closure)
{
    void *deref = *(void **)_CDataObject_CAST(self)->b_ptr;
    if (deref == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "NULL pointer access");
        return NULL;
    }

    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));
    StgInfo *stginfo;
    if (PyStgInfo_FromObject(st, self, &stginfo) < 0) {
        return NULL;
    }
    assert(stginfo); /* Cannot be NULL for pointer instances */

    return PyCData_FromBaseObj(st, stginfo->proto, self, 0, deref);
}

static PyObject *
Pointer_get_contents(PyObject *myself, void *closure)
{
    CDataObject *self = _CDataObject_CAST(myself);
    PyObject *res;
    LOCK_PTR(self);
    res = Pointer_get_contents_lock_held(myself, closure);
    UNLOCK_PTR(self);
    return res;
}

static int
Pointer_set_contents(PyObject *op, PyObject *value, void *closure)
{
    CDataObject *dst;
    PyObject *keep;
    CDataObject *self = _CDataObject_CAST(op);

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Pointer does not support item deletion");
        return -1;
    }
    ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(self)));
    StgInfo *stginfo;
    if (PyStgInfo_FromObject(st, op, &stginfo) < 0) {
        return -1;
    }
    assert(stginfo); /* Cannot be NULL for pointer instances */
    assert(stginfo->proto);
    if (!CDataObject_Check(st, value)) {
        int res = PyObject_IsInstance(value, stginfo->proto);
        if (res == -1)
            return -1;
        if (!res) {
            PyErr_Format(PyExc_TypeError,
                         "expected %s instead of %s",
                         ((PyTypeObject *)(stginfo->proto))->tp_name,
                         Py_TYPE(value)->tp_name);
            return -1;
        }
    }

    dst = (CDataObject *)value;
    if (dst != self) {
        LOCK_PTR(dst);
        locked_deref_assign(self, dst->b_ptr);
        UNLOCK_PTR(dst);
    } else {
        LOCK_PTR(self);
        *((void **)self->b_ptr) = dst->b_ptr;
        UNLOCK_PTR(self);
    }

    /*
       A Pointer instance must keep the value it points to alive.  So, a
       pointer instance has b_length set to 2 instead of 1, and we set
       'value' itself as the second item of the b_objects list, additionally.
    */
    Py_INCREF(value);
    if (-1 == KeepRef(self, 1, value))
        return -1;

    keep = GetKeepedObjects(dst);
    if (keep == NULL)
        return -1;

    Py_INCREF(keep);
    return KeepRef(self, 0, keep);
}

static PyGetSetDef Pointer_getsets[] = {
    { "contents", Pointer_get_contents, Pointer_set_contents,
      "the object this pointer points to (read-write)", NULL },
    { NULL, NULL }
};

static int
Pointer_init(PyObject *self, PyObject *args, PyObject *kw)
{
    PyObject *value = NULL;
    if (!PyArg_UnpackTuple(args, "POINTER", 0, 1, &value))
        return -1;
    if (value == NULL)
        return 0;
    return Pointer_set_contents(self, value, NULL);
}

static PyObject *
Pointer_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    ctypes_state *st = get_module_state_by_def(Py_TYPE(type));
    StgInfo *info;
    if (PyStgInfo_FromType(st, (PyObject *)type, &info) < 0) {
        return NULL;
    }
    if (!info || !info->proto) {
        PyErr_SetString(PyExc_TypeError,
                        "Cannot create instance: has no _type_");
        return NULL;
    }
    return generic_pycdata_new(st, type, args, kw);
}

static int
copy_pointer_to_list_lock_held(PyObject *myself, PyObject *np, Py_ssize_t len,
                               Py_ssize_t start, Py_ssize_t step)
{
    Py_ssize_t i;
    size_t cur;
    for (cur = start, i = 0; i < len; cur += step, i++) {
        PyObject *v = Pointer_item_lock_held(myself, cur);
        if (!v) {
            return -1;
        }
        PyList_SET_ITEM(np, i, v);
    }

    return 0;
}

static PyObject *
Pointer_subscript(PyObject *myself, PyObject *item)
{
    CDataObject *self = _CDataObject_CAST(myself);
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        return Pointer_item(myself, i);
    }
    else if (PySlice_Check(item)) {
        PySliceObject *slice = (PySliceObject *)item;
        Py_ssize_t start, stop, step;
        PyObject *np;
        PyObject *proto;
        Py_ssize_t i, len;
        size_t cur;

        /* Since pointers have no length, and we want to apply
           different semantics to negative indices than normal
           slicing, we have to dissect the slice object ourselves.*/
        if (slice->step == Py_None) {
            step = 1;
        }
        else {
            step = PyNumber_AsSsize_t(slice->step,
                                      PyExc_ValueError);
            if (step == -1 && PyErr_Occurred())
                return NULL;
            if (step == 0) {
                PyErr_SetString(PyExc_ValueError,
                                "slice step cannot be zero");
                return NULL;
            }
        }
        if (slice->start == Py_None) {
            if (step < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "slice start is required "
                                "for step < 0");
                return NULL;
            }
            start = 0;
        }
        else {
            start = PyNumber_AsSsize_t(slice->start,
                                       PyExc_ValueError);
            if (start == -1 && PyErr_Occurred())
                return NULL;
        }
        if (slice->stop == Py_None) {
            PyErr_SetString(PyExc_ValueError,
                            "slice stop is required");
            return NULL;
        }
        stop = PyNumber_AsSsize_t(slice->stop,
                                  PyExc_ValueError);
        if (stop == -1 && PyErr_Occurred())
            return NULL;
        if ((step > 0 && start > stop) ||
            (step < 0 && start < stop))
            len = 0;
        else if (step > 0)
            len = (stop - start - 1) / step + 1;
        else
            len = (stop - start + 1) / step + 1;

        ctypes_state *st = get_module_state_by_def(Py_TYPE(Py_TYPE(myself)));
        StgInfo *stginfo;
        if (PyStgInfo_FromObject(st, myself, &stginfo) < 0) {
            return NULL;
        }
        assert(stginfo); /* Cannot be NULL for pointer instances */
        proto = stginfo->proto;
        assert(proto);
        StgInfo *iteminfo;
        if (PyStgInfo_FromType(st, proto, &iteminfo) < 0) {
            return NULL;
        }
        assert(iteminfo);
        if (iteminfo->getfunc == _ctypes_get_fielddesc("c")->getfunc) {
            char *dest;

            if (len <= 0)
                return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
            if (step == 1) {
                PyObject *res;
                LOCK_PTR(self);
                char *ptr = *(void **)self->b_ptr;
                res = PyBytes_FromStringAndSize(ptr + start,
                                                len);
                UNLOCK_PTR(self);
                return res;
            }
            dest = (char *)PyMem_Malloc(len);
            if (dest == NULL)
                return PyErr_NoMemory();
            LOCK_PTR(self);
            char *ptr = *(void **)self->b_ptr;
            for (cur = start, i = 0; i < len; cur += step, i++) {
                dest[i] = ptr[cur];
            }
            UNLOCK_PTR(self);
            np = PyBytes_FromStringAndSize(dest, len);
            PyMem_Free(dest);
            return np;
        }
        if (iteminfo->getfunc == _ctypes_get_fielddesc("u")->getfunc) {
            wchar_t *dest;

            if (len <= 0)
                return Py_GetConstant(Py_CONSTANT_EMPTY_STR);
            if (step == 1) {
                PyObject *res;
                LOCK_PTR(self);
                wchar_t *ptr = *(wchar_t **)self->b_ptr;
                res = PyUnicode_FromWideChar(ptr + start,
                                             len);
                UNLOCK_PTR(self);
                return res;
            }
            dest = PyMem_New(wchar_t, len);
            if (dest == NULL)
                return PyErr_NoMemory();
            LOCK_PTR(self);
            wchar_t *ptr = *(wchar_t **)self->b_ptr;
            for (cur = start, i = 0; i < len; cur += step, i++) {
                dest[i] = ptr[cur];
            }
            UNLOCK_PTR(self);
            np = PyUnicode_FromWideChar(dest, len);
            PyMem_Free(dest);
            return np;
        }

        np = PyList_New(len);
        if (np == NULL)
            return NULL;

        int res;
        LOCK_PTR(self);
        res = copy_pointer_to_list_lock_held(myself, np, len, start, step);
        UNLOCK_PTR(self);
        if (res < 0) {
            Py_DECREF(np);
            return NULL;
        }

        return np;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "Pointer indices must be integer");
        return NULL;
    }
}

static int
Pointer_bool(PyObject *self)
{
    return locked_deref(_CDataObject_CAST(self)) != NULL;
}

static PyType_Slot pycpointer_slots[] = {
    {Py_tp_doc, (void *)PyDoc_STR("XXX to be provided")},
    {Py_tp_getset, Pointer_getsets},
    {Py_tp_init, Pointer_init},
    {Py_tp_new, Pointer_new},
    {Py_bf_getbuffer, PyCData_NewGetBuffer},
    {Py_nb_bool, Pointer_bool},
    {Py_mp_subscript, Pointer_subscript},
    {Py_sq_item, Pointer_item},
    {Py_sq_ass_item, Pointer_ass_item},
    {0, NULL},
};

static PyType_Spec pycpointer_spec = {
    .name = "_ctypes._Pointer",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pycpointer_slots,
};

/******************************************************************/
/*
 *  Module initialization.
 */

PyDoc_STRVAR(_ctypes__doc__,
"Create and manipulate C compatible data types in Python.");

#ifdef MS_WIN32

PyDoc_STRVAR(comerror_doc, "Raised when a COM method call failed.");

int
comerror_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *hresult, *text, *details;
    PyObject *a;
    int status;

    if (!_PyArg_NoKeywords(Py_TYPE(self)->tp_name, kwds))
        return -1;

    if (!PyArg_ParseTuple(args, "OOO:COMError", &hresult, &text, &details))
        return -1;

    a = PySequence_GetSlice(args, 1, PyTuple_GET_SIZE(args));
    if (!a)
        return -1;
    status = PyObject_SetAttrString(self, "args", a);
    Py_DECREF(a);
    if (status < 0)
        return -1;

    if (PyObject_SetAttrString(self, "hresult", hresult) < 0)
        return -1;

    if (PyObject_SetAttrString(self, "text", text) < 0)
        return -1;

    if (PyObject_SetAttrString(self, "details", details) < 0)
        return -1;

    Py_INCREF(args);
    Py_SETREF(((PyBaseExceptionObject *)self)->args, args);

    return 0;
}

static int
comerror_clear(PyObject *self)
{
    return ((PyTypeObject *)PyExc_BaseException)->tp_clear(self);
}

static int
comerror_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return ((PyTypeObject *)PyExc_BaseException)->tp_traverse(self, visit, arg);
}

static void
comerror_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    (void)comerror_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyType_Slot comerror_slots[] = {
    {Py_tp_doc, (void *)PyDoc_STR(comerror_doc)},
    {Py_tp_init, comerror_init},
    {Py_tp_traverse, comerror_traverse},
    {Py_tp_dealloc, comerror_dealloc},
    {Py_tp_clear, comerror_clear},
    {0, NULL},
};

static PyType_Spec comerror_spec = {
    .name = "_ctypes.COMError",
    .basicsize = sizeof(PyBaseExceptionObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = comerror_slots,
};

#endif  // MS_WIN32

static PyObject *
string_at(const char *ptr, int size)
{
    if (PySys_Audit("ctypes.string_at", "ni", (Py_ssize_t)ptr, size) < 0) {
        return NULL;
    }
    if (size == -1)
        return PyBytes_FromStringAndSize(ptr, strlen(ptr));
    return PyBytes_FromStringAndSize(ptr, size);
}

static int
cast_check_pointertype(ctypes_state *st, PyObject *arg)
{
    if (PyCPointerTypeObject_Check(st, arg)) {
        return 1;
    }
    if (PyCFuncPtrTypeObject_Check(st, arg)) {
        return 1;
    }
    StgInfo *info;
    if (PyStgInfo_FromType(st, arg, &info) < 0) {
        return 0;
    }
    if (info != NULL && info->proto != NULL) {
        if (PyUnicode_Check(info->proto)
            && (strchr("sPzUZXO", PyUnicode_AsUTF8(info->proto)[0]))) {
            /* simple pointer types, c_void_p, c_wchar_p, BSTR, ... */
            return 1;
        }
    }
    PyErr_Format(PyExc_TypeError,
                 "cast() argument 2 must be a pointer type, not %s",
                 PyType_Check(arg)
                 ? ((PyTypeObject *)arg)->tp_name
                 : Py_TYPE(arg)->tp_name);
    return 0;
}

static PyObject *
cast(void *ptr, PyObject *src, PyObject *ctype)
{
    PyObject *mod = PyType_GetModuleByDef(Py_TYPE(ctype), &_ctypesmodule);
    if (!mod) {
        PyErr_SetString(PyExc_TypeError,
                        "cast() argument 2 must be a pointer type");
        return NULL;
    }
    ctypes_state *st = get_module_state(mod);

    CDataObject *result;
    if (cast_check_pointertype(st, ctype) == 0) {
        return NULL;
    }
    result = (CDataObject *)_PyObject_CallNoArgs(ctype);
    if (result == NULL)
        return NULL;

    /*
      The casted objects '_objects' member:

      It must certainly contain the source objects one.
      It must contain the source object itself.
     */
    if (CDataObject_Check(st, src)) {
        CDataObject *obj = (CDataObject *)src;
        CDataObject *container;

        /* PyCData_GetContainer will initialize src.b_objects, we need
           this so it can be shared */
        container = PyCData_GetContainer(obj);
        if (container == NULL)
            goto failed;

        /* But we need a dictionary! */
        if (obj->b_objects == Py_None) {
            Py_DECREF(Py_None);
            obj->b_objects = PyDict_New();
            if (obj->b_objects == NULL)
                goto failed;
        }
        result->b_objects = Py_XNewRef(obj->b_objects);
        if (result->b_objects && PyDict_CheckExact(result->b_objects)) {
            PyObject *index;
            int rc;
            index = PyLong_FromVoidPtr((void *)src);
            if (index == NULL)
                goto failed;
            rc = PyDict_SetItem(result->b_objects, index, src);
            Py_DECREF(index);
            if (rc == -1)
                goto failed;
        }
    }
    /* Should we assert that result is a pointer type? */
    locked_memcpy_to(result, &ptr, sizeof(void *));
    return (PyObject *)result;

  failed:
    Py_DECREF(result);
    return NULL;
}


static PyObject *
wstring_at(const wchar_t *ptr, int size)
{
    Py_ssize_t ssize = size;
    if (PySys_Audit("ctypes.wstring_at", "nn", (Py_ssize_t)ptr, ssize) < 0) {
        return NULL;
    }
    if (ssize == -1)
        ssize = wcslen(ptr);
    return PyUnicode_FromWideChar(ptr, ssize);
}

static PyObject *
memoryview_at(void *ptr, Py_ssize_t size, int readonly)
{
    if (PySys_Audit("ctypes.memoryview_at", "nni",
                    (Py_ssize_t)ptr, size, readonly) < 0) {
        return NULL;
    }
    if (size < 0) {
        PyErr_Format(PyExc_ValueError,
                     "memoryview_at: size is negative (or overflowed): %zd",
                     size);
        return NULL;
    }
    return PyMemoryView_FromMemory(ptr, size,
                                   readonly ? PyBUF_READ : PyBUF_WRITE);
}

static int
_ctypes_add_types(PyObject *mod)
{
#define TYPE_READY(TYPE) \
    if (PyType_Ready(TYPE) < 0) { \
        return -1; \
    }
#define CREATE_TYPE(TP, SPEC, META, BASE) do {                      \
    PyObject *type = PyType_FromMetaclass(META, mod, SPEC,          \
                                          (PyObject *)BASE);        \
    if (type == NULL) {                                             \
        return -1;                                                  \
    }                                                               \
    TP = (PyTypeObject *)type;                                      \
} while (0)

#define MOD_ADD_TYPE(TP, SPEC, META, BASE) do {                     \
    CREATE_TYPE(TP, SPEC, META, BASE);                              \
    if (PyModule_AddType(mod, (PyTypeObject *)(TP)) < 0) {          \
        return -1;                                                  \
    }                                                               \
} while (0)

    ctypes_state *st = get_module_state(mod);

    /* Note:
       ob_type is the metatype (the 'type'), defaults to PyType_Type,
       tp_base is the base type, defaults to 'object' aka PyBaseObject_Type.
    */
    CREATE_TYPE(st->PyCArg_Type, &carg_spec, NULL, NULL);
    CREATE_TYPE(st->PyCThunk_Type, &cthunk_spec, NULL, NULL);
    CREATE_TYPE(st->PyCData_Type, &pycdata_spec, NULL, NULL);

    // Common Metaclass
    CREATE_TYPE(st->PyCType_Type, &pyctype_type_spec,
                NULL, &PyType_Type);

    /*************************************************
     *
     * Metaclasses
     */
    CREATE_TYPE(st->PyCStructType_Type, &pycstruct_type_spec,
                NULL, st->PyCType_Type);
    CREATE_TYPE(st->UnionType_Type, &union_type_spec,
                NULL, st->PyCType_Type);
    CREATE_TYPE(st->PyCPointerType_Type, &pycpointer_type_spec,
                NULL, st->PyCType_Type);
    CREATE_TYPE(st->PyCArrayType_Type, &pycarray_type_spec,
                NULL, st->PyCType_Type);
    CREATE_TYPE(st->PyCSimpleType_Type, &pycsimple_type_spec,
                NULL, st->PyCType_Type);
    CREATE_TYPE(st->PyCFuncPtrType_Type, &pycfuncptr_type_spec,
                NULL, st->PyCType_Type);

    /*************************************************
     *
     * Classes using a custom metaclass
     */

    MOD_ADD_TYPE(st->Struct_Type, &pycstruct_spec,
                 st->PyCStructType_Type, st->PyCData_Type);
    MOD_ADD_TYPE(st->Union_Type, &pycunion_spec,
                 st->UnionType_Type, st->PyCData_Type);
    MOD_ADD_TYPE(st->PyCPointer_Type, &pycpointer_spec,
                 st->PyCPointerType_Type, st->PyCData_Type);
    MOD_ADD_TYPE(st->PyCArray_Type, &pycarray_spec,
                 st->PyCArrayType_Type, st->PyCData_Type);
    MOD_ADD_TYPE(st->Simple_Type, &pycsimple_spec,
                 st->PyCSimpleType_Type, st->PyCData_Type);
    MOD_ADD_TYPE(st->PyCFuncPtr_Type, &pycfuncptr_spec,
                 st->PyCFuncPtrType_Type, st->PyCData_Type);

    /*************************************************
     *
     * Simple classes
     */

    MOD_ADD_TYPE(st->PyCField_Type, &cfield_spec, NULL, NULL);

    /*************************************************
     *
     * Other stuff
     */

    CREATE_TYPE(st->DictRemover_Type, &dictremover_spec, NULL, NULL);
    CREATE_TYPE(st->StructParam_Type, &structparam_spec, NULL, NULL);

#ifdef MS_WIN32
    CREATE_TYPE(st->PyComError_Type, &comerror_spec, NULL, PyExc_Exception);
#endif

#undef TYPE_READY
#undef MOD_ADD_TYPE
#undef CREATE_TYPE
    return 0;
}


static int
_ctypes_add_objects(PyObject *mod)
{
#define MOD_ADD(name, expr) \
    do { \
        if (PyModule_Add(mod, name, (expr)) < 0) { \
            return -1; \
        } \
    } while (0)

    ctypes_state *st = get_module_state(mod);
    MOD_ADD("_pointer_type_cache", Py_NewRef(st->_ctypes_ptrtype_cache));

#ifdef MS_WIN32
    MOD_ADD("COMError", Py_NewRef(st->PyComError_Type));
    MOD_ADD("FUNCFLAG_HRESULT", PyLong_FromLong(FUNCFLAG_HRESULT));
    MOD_ADD("FUNCFLAG_STDCALL", PyLong_FromLong(FUNCFLAG_STDCALL));
#endif
    MOD_ADD("FUNCFLAG_CDECL", PyLong_FromLong(FUNCFLAG_CDECL));
    MOD_ADD("FUNCFLAG_USE_ERRNO", PyLong_FromLong(FUNCFLAG_USE_ERRNO));
    MOD_ADD("FUNCFLAG_USE_LASTERROR", PyLong_FromLong(FUNCFLAG_USE_LASTERROR));
    MOD_ADD("FUNCFLAG_PYTHONAPI", PyLong_FromLong(FUNCFLAG_PYTHONAPI));
    MOD_ADD("__version__", PyUnicode_FromString("1.1.0"));

    MOD_ADD("_memmove_addr", PyLong_FromVoidPtr(memmove));
    MOD_ADD("_memset_addr", PyLong_FromVoidPtr(memset));
    MOD_ADD("_string_at_addr", PyLong_FromVoidPtr(string_at));
    MOD_ADD("_cast_addr", PyLong_FromVoidPtr(cast));
    MOD_ADD("_wstring_at_addr", PyLong_FromVoidPtr(wstring_at));
    MOD_ADD("_memoryview_at_addr", PyLong_FromVoidPtr(memoryview_at));

/* If RTLD_LOCAL is not defined (Windows!), set it to zero. */
#if !HAVE_DECL_RTLD_LOCAL
#  define RTLD_LOCAL 0
#endif

/* If RTLD_GLOBAL is not defined (cygwin), set it to the same value as
   RTLD_LOCAL. */
#if !HAVE_DECL_RTLD_GLOBAL
#  define RTLD_GLOBAL RTLD_LOCAL
#endif
    MOD_ADD("RTLD_LOCAL", PyLong_FromLong(RTLD_LOCAL));
    MOD_ADD("RTLD_GLOBAL", PyLong_FromLong(RTLD_GLOBAL));
    MOD_ADD("CTYPES_MAX_ARGCOUNT", PyLong_FromLong(CTYPES_MAX_ARGCOUNT));
    MOD_ADD("ArgumentError", Py_NewRef(st->PyExc_ArgError));
    MOD_ADD("SIZEOF_TIME_T", PyLong_FromSsize_t(SIZEOF_TIME_T));
    return 0;
#undef MOD_ADD
}


static int
_ctypes_mod_exec(PyObject *mod)
{
    // See https://github.com/python/cpython/issues/128485
    // This allocates some memory and then frees it to ensure that the
    // the dlmalloc allocator initializes itself to avoid data races
    // in free-threading.
    void *codeloc = NULL;
    void *ptr = Py_ffi_closure_alloc(sizeof(void *), &codeloc);
    if (ptr == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    Py_ffi_closure_free(ptr);

    ctypes_state *st = get_module_state(mod);
    st->_unpickle = PyObject_GetAttrString(mod, "_unpickle");
    if (st->_unpickle == NULL) {
        return -1;
    }

    st->_ctypes_ptrtype_cache = PyDict_New();
    if (st->_ctypes_ptrtype_cache == NULL) {
        return -1;
    }

    st->PyExc_ArgError = PyErr_NewException("ctypes.ArgumentError", NULL, NULL);
    if (!st->PyExc_ArgError) {
        return -1;
    }

    st->array_cache = PyDict_New();
    if (st->array_cache == NULL) {
        return -1;
    }

#ifdef WORDS_BIGENDIAN
    st->swapped_suffix = PyUnicode_InternFromString("_le");
#else
    st->swapped_suffix = PyUnicode_InternFromString("_be");
#endif
    if (st->swapped_suffix == NULL) {
        return -1;
    }

    st->error_object_name = PyUnicode_InternFromString("ctypes.error_object");
    if (st->error_object_name == NULL) {
        return -1;
    }

    if (_ctypes_add_types(mod) < 0) {
        return -1;
    }

    if (_ctypes_add_objects(mod) < 0) {
        return -1;
    }
    return 0;
}


static int
module_traverse(PyObject *module, visitproc visit, void *arg) {
    ctypes_state *st = get_module_state(module);
    Py_VISIT(st->_ctypes_ptrtype_cache);
    Py_VISIT(st->_unpickle);
    Py_VISIT(st->array_cache);
    Py_VISIT(st->error_object_name);
    Py_VISIT(st->PyExc_ArgError);
    Py_VISIT(st->swapped_suffix);

    Py_VISIT(st->DictRemover_Type);
    Py_VISIT(st->PyCArg_Type);
    Py_VISIT(st->PyCField_Type);
    Py_VISIT(st->PyCThunk_Type);
    Py_VISIT(st->StructParam_Type);
    Py_VISIT(st->PyCStructType_Type);
    Py_VISIT(st->UnionType_Type);
    Py_VISIT(st->PyCPointerType_Type);
    Py_VISIT(st->PyCArrayType_Type);
    Py_VISIT(st->PyCSimpleType_Type);
    Py_VISIT(st->PyCFuncPtrType_Type);
    Py_VISIT(st->PyCData_Type);
    Py_VISIT(st->Struct_Type);
    Py_VISIT(st->Union_Type);
    Py_VISIT(st->PyCArray_Type);
    Py_VISIT(st->Simple_Type);
    Py_VISIT(st->PyCPointer_Type);
    Py_VISIT(st->PyCFuncPtr_Type);
#ifdef MS_WIN32
    Py_VISIT(st->PyComError_Type);
#endif
    Py_VISIT(st->PyCType_Type);
    return 0;
}

static int
module_clear(PyObject *module) {
    ctypes_state *st = get_module_state(module);
    Py_CLEAR(st->_ctypes_ptrtype_cache);
    Py_CLEAR(st->_unpickle);
    Py_CLEAR(st->array_cache);
    Py_CLEAR(st->error_object_name);
    Py_CLEAR(st->PyExc_ArgError);
    Py_CLEAR(st->swapped_suffix);

    Py_CLEAR(st->DictRemover_Type);
    Py_CLEAR(st->PyCArg_Type);
    Py_CLEAR(st->PyCField_Type);
    Py_CLEAR(st->PyCThunk_Type);
    Py_CLEAR(st->StructParam_Type);
    Py_CLEAR(st->PyCStructType_Type);
    Py_CLEAR(st->UnionType_Type);
    Py_CLEAR(st->PyCPointerType_Type);
    Py_CLEAR(st->PyCArrayType_Type);
    Py_CLEAR(st->PyCSimpleType_Type);
    Py_CLEAR(st->PyCFuncPtrType_Type);
    Py_CLEAR(st->PyCData_Type);
    Py_CLEAR(st->Struct_Type);
    Py_CLEAR(st->Union_Type);
    Py_CLEAR(st->PyCArray_Type);
    Py_CLEAR(st->Simple_Type);
    Py_CLEAR(st->PyCPointer_Type);
    Py_CLEAR(st->PyCFuncPtr_Type);
#ifdef MS_WIN32
    Py_CLEAR(st->PyComError_Type);
#endif
    Py_CLEAR(st->PyCType_Type);
    return 0;
}

static void
module_free(void *module)
{
    (void)module_clear((PyObject *)module);
}

static PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, _ctypes_mod_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

struct PyModuleDef _ctypesmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_ctypes",
    .m_doc = _ctypes__doc__,
    .m_size = sizeof(ctypes_state),
    .m_methods = _ctypes_module_methods,
    .m_slots = module_slots,
    .m_traverse = module_traverse,
    .m_clear = module_clear,
    .m_free = module_free,
};

PyMODINIT_FUNC
PyInit__ctypes(void)
{
    return PyModuleDef_Init(&_ctypesmodule);
}