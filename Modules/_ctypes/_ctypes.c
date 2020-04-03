/*
  ToDo:

  Get rid of the checker (and also the converters) field in PyCFuncPtrObject and
  StgDictObject, and replace them by slot functions in StgDictObject.

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
UnionType_Type          __new__(), from_address(), __mul__(), from_param()
PyCPointerType_Type     __new__(), from_address(), __mul__(), from_param(), set_type()
PyCArrayType_Type               __new__(), from_address(), __mul__(), from_param()
PyCSimpleType_Type              __new__(), from_address(), __mul__(), from_param()

PyCData_Type
  Struct_Type           __new__(), __init__()
  PyCPointer_Type               __new__(), __init__(), _as_parameter_, contents
  PyCArray_Type         __new__(), __init__(), _as_parameter_, __get/setitem__(), __len__()
  Simple_Type           __new__(), __init__(), _as_parameter_

PyCField_Type
PyCStgDict_Type

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
 * PyCStgDict_Type
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

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "structmember.h"

#include <ffi.h>
#ifdef MS_WIN32
#include <windows.h>
#include <malloc.h>
#ifndef IS_INTRESOURCE
#define IS_INTRESOURCE(x) (((size_t)(x) >> 16) == 0)
#endif
#else
#include "ctypes_dlfcn.h"
#endif
#include "ctypes.h"

PyObject *PyExc_ArgError = NULL;

/* This dict maps ctypes types to POINTER types */
PyObject *_ctypes_ptrtype_cache = NULL;

static PyTypeObject Simple_Type;

/* a callable object used for unpickling */
static PyObject *_unpickle;



/****************************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *key;
    PyObject *dict;
} DictRemoverObject;

static void
_DictRemover_dealloc(PyObject *myself)
{
    DictRemoverObject *self = (DictRemoverObject *)myself;
    Py_XDECREF(self->key);
    Py_XDECREF(self->dict);
    Py_TYPE(self)->tp_free(myself);
}

static PyObject *
_DictRemover_call(PyObject *myself, PyObject *args, PyObject *kw)
{
    DictRemoverObject *self = (DictRemoverObject *)myself;
    if (self->key && self->dict) {
        if (-1 == PyDict_DelItem(self->dict, self->key)) {
            _PyErr_WriteUnraisableMsg("on calling _ctypes.DictRemover", NULL);
        }
        Py_CLEAR(self->key);
        Py_CLEAR(self->dict);
    }
    Py_RETURN_NONE;
}

static PyTypeObject DictRemover_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.DictRemover",                      /* tp_name */
    sizeof(DictRemoverObject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    _DictRemover_dealloc,                       /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    _DictRemover_call,                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
/* XXX should participate in GC? */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    "deletes a key from a dictionary",          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
    0,                                          /* tp_free */
};

int
PyDict_SetItemProxy(PyObject *dict, PyObject *key, PyObject *item)
{
    PyObject *obj;
    DictRemoverObject *remover;
    PyObject *proxy;
    int result;

    obj = _PyObject_CallNoArg((PyObject *)&DictRemover_Type);
    if (obj == NULL)
        return -1;

    remover = (DictRemoverObject *)obj;
    assert(remover->key == NULL);
    assert(remover->dict == NULL);
    Py_INCREF(key);
    remover->key = key;
    Py_INCREF(dict);
    remover->dict = dict;

    proxy = PyWeakref_NewProxy(item, obj);
    Py_DECREF(obj);
    if (proxy == NULL)
        return -1;

    result = PyDict_SetItem(dict, key, proxy);
    Py_DECREF(proxy);
    return result;
}

PyObject *
PyDict_GetItemProxy(PyObject *dict, PyObject *key)
{
    PyObject *result;
    PyObject *item = PyDict_GetItemWithError(dict, key);

    if (item == NULL)
        return NULL;
    if (!PyWeakref_CheckProxy(item))
        return item;
    result = PyWeakref_GET_OBJECT(item);
    if (result == Py_None)
        return NULL;
    return result;
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
char *
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
char *
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
                sprintf(buf, "%"PY_FORMAT_SIZE_T"d,", shape[k]);
            } else {
                sprintf(buf, "%"PY_FORMAT_SIZE_T"d)", shape[k]);
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
} StructParamObject;


static void
StructParam_dealloc(PyObject *myself)
{
    StructParamObject *self = (StructParamObject *)myself;
    PyMem_Free(self->ptr);
    Py_TYPE(self)->tp_free(myself);
}


static PyTypeObject StructParam_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_ctypes.StructParam_Type",
    .tp_basicsize = sizeof(StructParamObject),
    .tp_dealloc = StructParam_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
};


/*
  PyCStructType_Type - a meta type/class.  Creating a new class using this one as
  __metaclass__ will call the constructor StructUnionType_new.  It replaces the
  tp_dict member with a new instance of StgDict, and initializes the C
  accessible fields somehow.
*/

static PyCArgObject *
StructUnionType_paramfunc(CDataObject *self)
{
    PyCArgObject *parg;
    PyObject *obj;
    StgDictObject *stgdict;
    void *ptr;

    if ((size_t)self->b_size > sizeof(void*)) {
        ptr = PyMem_Malloc(self->b_size);
        if (ptr == NULL) {
            return NULL;
        }
        memcpy(ptr, self->b_ptr, self->b_size);

        /* Create a Python object which calls PyMem_Free(ptr) in
           its deallocator. The object will be destroyed
           at _ctypes_callproc() cleanup. */
        obj = (&StructParam_Type)->tp_alloc(&StructParam_Type, 0);
        if (obj == NULL) {
            PyMem_Free(ptr);
            return NULL;
        }

        StructParamObject *struct_param = (StructParamObject *)obj;
        struct_param->ptr = ptr;
    } else {
        ptr = self->b_ptr;
        obj = (PyObject *)self;
        Py_INCREF(obj);
    }

    parg = PyCArgObject_new();
    if (parg == NULL) {
        Py_DECREF(obj);
        return NULL;
    }

    parg->tag = 'V';
    stgdict = PyObject_stgdict((PyObject *)self);
    assert(stgdict); /* Cannot be NULL for structure/union instances */
    parg->pffi_type = &stgdict->ffi_type_pointer;
    parg->value.p = ptr;
    parg->size = self->b_size;
    parg->obj = obj;
    return parg;
}

static PyObject *
StructUnionType_new(PyTypeObject *type, PyObject *args, PyObject *kwds, int isStruct)
{
    PyTypeObject *result;
    PyObject *fields;
    StgDictObject *dict;
    _Py_IDENTIFIER(_abstract_);
    _Py_IDENTIFIER(_fields_);

    /* create the new instance (which is a class,
       since we are a metatype!) */
    result = (PyTypeObject *)PyType_Type.tp_new(type, args, kwds);
    if (!result)
        return NULL;

    /* keep this for bw compatibility */
    if (_PyDict_GetItemIdWithError(result->tp_dict, &PyId__abstract_))
        return (PyObject *)result;
    if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }

    dict = (StgDictObject *)_PyObject_CallNoArg((PyObject *)&PyCStgDict_Type);
    if (!dict) {
        Py_DECREF(result);
        return NULL;
    }
    if (!isStruct) {
        dict->flags |= TYPEFLAG_HASUNION;
    }
    /* replace the class dict by our updated stgdict, which holds info
       about storage requirements of the instances */
    if (-1 == PyDict_Update((PyObject *)dict, result->tp_dict)) {
        Py_DECREF(result);
        Py_DECREF((PyObject *)dict);
        return NULL;
    }
    Py_SETREF(result->tp_dict, (PyObject *)dict);
    dict->format = _ctypes_alloc_format_string(NULL, "B");
    if (dict->format == NULL) {
        Py_DECREF(result);
        return NULL;
    }

    dict->paramfunc = StructUnionType_paramfunc;

    fields = _PyDict_GetItemIdWithError((PyObject *)dict, &PyId__fields_);
    if (fields) {
        if (_PyObject_SetAttrId((PyObject *)result, &PyId__fields_, fields) < 0) {
            Py_DECREF(result);
            return NULL;
        }
        return (PyObject *)result;
    }
    else if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }
    else {
        StgDictObject *basedict = PyType_stgdict((PyObject *)result->tp_base);

        if (basedict == NULL)
            return (PyObject *)result;
        /* copy base dict */
        if (-1 == PyCStgDict_clone(dict, basedict)) {
            Py_DECREF(result);
            return NULL;
        }
        dict->flags &= ~DICTFLAG_FINAL; /* clear the 'final' flag in the subclass dict */
        basedict->flags |= DICTFLAG_FINAL; /* set the 'final' flag in the baseclass dict */
        return (PyObject *)result;
    }
}

static PyObject *
PyCStructType_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    return StructUnionType_new(type, args, kwds, 1);
}

static PyObject *
UnionType_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    return StructUnionType_new(type, args, kwds, 0);
}

static const char from_address_doc[] =
"C.from_address(integer) -> C instance\naccess a C instance at the specified address";

static PyObject *
CDataType_from_address(PyObject *type, PyObject *value)
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
    return PyCData_AtAddress(type, buf);
}

static const char from_buffer_doc[] =
"C.from_buffer(object, offset=0) -> C instance\ncreate a C instance from a writeable buffer";

static int
KeepRef(CDataObject *target, Py_ssize_t index, PyObject *keep);

static PyObject *
CDataType_from_buffer(PyObject *type, PyObject *args)
{
    PyObject *obj;
    PyObject *mv;
    PyObject *result;
    Py_buffer *buffer;
    Py_ssize_t offset = 0;

    StgDictObject *dict = PyType_stgdict(type);
    if (!dict) {
        PyErr_SetString(PyExc_TypeError, "abstract class");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O|n:from_buffer", &obj, &offset))
        return NULL;

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

    if (dict->size > buffer->len - offset) {
        PyErr_Format(PyExc_ValueError,
                     "Buffer size too small "
                     "(%zd instead of at least %zd bytes)",
                     buffer->len, dict->size + offset);
        Py_DECREF(mv);
        return NULL;
    }

    if (PySys_Audit("ctypes.cdata/buffer", "nnn",
                    (Py_ssize_t)buffer->buf, buffer->len, offset) < 0) {
        Py_DECREF(mv);
        return NULL;
    }

    result = PyCData_AtAddress(type, (char *)buffer->buf + offset);
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

static const char from_buffer_copy_doc[] =
"C.from_buffer_copy(object, offset=0) -> C instance\ncreate a C instance from a readable buffer";

static PyObject *
GenericPyCData_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *
CDataType_from_buffer_copy(PyObject *type, PyObject *args)
{
    Py_buffer buffer;
    Py_ssize_t offset = 0;
    PyObject *result;
    StgDictObject *dict = PyType_stgdict(type);
    if (!dict) {
        PyErr_SetString(PyExc_TypeError, "abstract class");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "y*|n:from_buffer_copy", &buffer, &offset))
        return NULL;

    if (offset < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "offset cannot be negative");
        PyBuffer_Release(&buffer);
        return NULL;
    }

    if (dict->size > buffer.len - offset) {
        PyErr_Format(PyExc_ValueError,
                     "Buffer size too small (%zd instead of at least %zd bytes)",
                     buffer.len, dict->size + offset);
        PyBuffer_Release(&buffer);
        return NULL;
    }

    if (PySys_Audit("ctypes.cdata/buffer", "nnn",
                    (Py_ssize_t)buffer.buf, buffer.len, offset) < 0) {
        PyBuffer_Release(&buffer);
        return NULL;
    }

    result = GenericPyCData_new((PyTypeObject *)type, NULL, NULL);
    if (result != NULL) {
        memcpy(((CDataObject *)result)->b_ptr,
               (char *)buffer.buf + offset, dict->size);
    }
    PyBuffer_Release(&buffer);
    return result;
}

static const char in_dll_doc[] =
"C.in_dll(dll, name) -> C instance\naccess a C instance in a dll";

static PyObject *
CDataType_in_dll(PyObject *type, PyObject *args)
{
    PyObject *dll;
    char *name;
    PyObject *obj;
    void *handle;
    void *address;

    if (!PyArg_ParseTuple(args, "Os:in_dll", &dll, &name))
        return NULL;
    if (PySys_Audit("ctypes.dlsym", "O", args) < 0) {
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

#ifdef MS_WIN32
    Py_BEGIN_ALLOW_THREADS
    address = (void *)GetProcAddress(handle, name);
    Py_END_ALLOW_THREADS
    if (!address) {
        PyErr_Format(PyExc_ValueError,
                     "symbol '%s' not found",
                     name);
        return NULL;
    }
#else
    address = (void *)ctypes_dlsym(handle, name);
    if (!address) {
#ifdef __CYGWIN__
/* dlerror() isn't very helpful on cygwin */
        PyErr_Format(PyExc_ValueError,
                     "symbol '%s' not found",
                     name);
#else
        PyErr_SetString(PyExc_ValueError, ctypes_dlerror());
#endif
        return NULL;
    }
#endif
    return PyCData_AtAddress(type, address);
}

static const char from_param_doc[] =
"Convert a Python object into a function call parameter.";

static PyObject *
CDataType_from_param(PyObject *type, PyObject *value)
{
    _Py_IDENTIFIER(_as_parameter_);
    PyObject *as_parameter;
    int res = PyObject_IsInstance(value, type);
    if (res == -1)
        return NULL;
    if (res) {
        Py_INCREF(value);
        return value;
    }
    if (PyCArg_CheckExact(value)) {
        PyCArgObject *p = (PyCArgObject *)value;
        PyObject *ob = p->obj;
        const char *ob_name;
        StgDictObject *dict;
        dict = PyType_stgdict(type);

        /* If we got a PyCArgObject, we must check if the object packed in it
           is an instance of the type's dict->proto */
        if(dict && ob) {
            res = PyObject_IsInstance(ob, dict->proto);
            if (res == -1)
                return NULL;
            if (res) {
                Py_INCREF(value);
                return value;
            }
        }
        ob_name = (ob) ? Py_TYPE(ob)->tp_name : "???";
        PyErr_Format(PyExc_TypeError,
                     "expected %s instance instead of pointer to %s",
                     ((PyTypeObject *)type)->tp_name, ob_name);
        return NULL;
    }

    if (_PyObject_LookupAttrId(value, &PyId__as_parameter_, &as_parameter) < 0) {
        return NULL;
    }
    if (as_parameter) {
        value = CDataType_from_param(type, as_parameter);
        Py_DECREF(as_parameter);
        return value;
    }
    PyErr_Format(PyExc_TypeError,
                 "expected %s instance instead of %s",
                 ((PyTypeObject *)type)->tp_name,
                 Py_TYPE(value)->tp_name);
    return NULL;
}

static PyMethodDef CDataType_methods[] = {
    { "from_param", CDataType_from_param, METH_O, from_param_doc },
    { "from_address", CDataType_from_address, METH_O, from_address_doc },
    { "from_buffer", CDataType_from_buffer, METH_VARARGS, from_buffer_doc, },
    { "from_buffer_copy", CDataType_from_buffer_copy, METH_VARARGS, from_buffer_copy_doc, },
    { "in_dll", CDataType_in_dll, METH_VARARGS, in_dll_doc },
    { NULL, NULL },
};

static PyObject *
CDataType_repeat(PyObject *self, Py_ssize_t length)
{
    if (length < 0)
        return PyErr_Format(PyExc_ValueError,
                            "Array length must be >= 0, not %zd",
                            length);
    return PyCArrayType_from_ctype(self, length);
}

static PySequenceMethods CDataType_as_sequence = {
    0,                          /* inquiry sq_length; */
    0,                          /* binaryfunc sq_concat; */
    CDataType_repeat,           /* intargfunc sq_repeat; */
    0,                          /* intargfunc sq_item; */
    0,                          /* intintargfunc sq_slice; */
    0,                          /* intobjargproc sq_ass_item; */
    0,                          /* intintobjargproc sq_ass_slice; */
    0,                          /* objobjproc sq_contains; */

    0,                          /* binaryfunc sq_inplace_concat; */
    0,                          /* intargfunc sq_inplace_repeat; */
};

static int
CDataType_clear(PyTypeObject *self)
{
    StgDictObject *dict = PyType_stgdict((PyObject *)self);
    if (dict)
        Py_CLEAR(dict->proto);
    return PyType_Type.tp_clear((PyObject *)self);
}

static int
CDataType_traverse(PyTypeObject *self, visitproc visit, void *arg)
{
    StgDictObject *dict = PyType_stgdict((PyObject *)self);
    if (dict)
        Py_VISIT(dict->proto);
    return PyType_Type.tp_traverse((PyObject *)self, visit, arg);
}

static int
PyCStructType_setattro(PyObject *self, PyObject *key, PyObject *value)
{
    /* XXX Should we disallow deleting _fields_? */
    if (-1 == PyType_Type.tp_setattro(self, key, value))
        return -1;

    if (value && PyUnicode_Check(key) &&
        _PyUnicode_EqualToASCIIString(key, "_fields_"))
        return PyCStructUnionType_update_stgdict(self, value, 1);
    return 0;
}


static int
UnionType_setattro(PyObject *self, PyObject *key, PyObject *value)
{
    /* XXX Should we disallow deleting _fields_? */
    if (-1 == PyObject_GenericSetAttr(self, key, value))
        return -1;

    if (PyUnicode_Check(key) &&
        _PyUnicode_EqualToASCIIString(key, "_fields_"))
        return PyCStructUnionType_update_stgdict(self, value, 0);
    return 0;
}


PyTypeObject PyCStructType_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.PyCStructType",                            /* tp_name */
    0,                                          /* tp_basicsize */
    0,                                          /* tp_itemsize */
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    &CDataType_as_sequence,                     /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    PyCStructType_setattro,                     /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    "metatype for the CData Objects",           /* tp_doc */
    (traverseproc)CDataType_traverse,           /* tp_traverse */
    (inquiry)CDataType_clear,                   /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    CDataType_methods,                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    PyCStructType_new,                                  /* tp_new */
    0,                                          /* tp_free */
};

static PyTypeObject UnionType_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.UnionType",                        /* tp_name */
    0,                                          /* tp_basicsize */
    0,                                          /* tp_itemsize */
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    &CDataType_as_sequence,             /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    UnionType_setattro,                         /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    "metatype for the CData Objects",           /* tp_doc */
    (traverseproc)CDataType_traverse,           /* tp_traverse */
    (inquiry)CDataType_clear,                   /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    CDataType_methods,                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    UnionType_new,                              /* tp_new */
    0,                                          /* tp_free */
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

static int
PyCPointerType_SetProto(StgDictObject *stgdict, PyObject *proto)
{
    if (!proto || !PyType_Check(proto)) {
        PyErr_SetString(PyExc_TypeError,
                        "_type_ must be a type");
        return -1;
    }
    if (!PyType_stgdict(proto)) {
        PyErr_SetString(PyExc_TypeError,
                        "_type_ must have storage info");
        return -1;
    }
    Py_INCREF(proto);
    Py_XSETREF(stgdict->proto, proto);
    return 0;
}

static PyCArgObject *
PyCPointerType_paramfunc(CDataObject *self)
{
    PyCArgObject *parg;

    parg = PyCArgObject_new();
    if (parg == NULL)
        return NULL;

    parg->tag = 'P';
    parg->pffi_type = &ffi_type_pointer;
    Py_INCREF(self);
    parg->obj = (PyObject *)self;
    parg->value.p = *(void **)self->b_ptr;
    return parg;
}

static PyObject *
PyCPointerType_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyTypeObject *result;
    StgDictObject *stgdict;
    PyObject *proto;
    PyObject *typedict;
    _Py_IDENTIFIER(_type_);

    typedict = PyTuple_GetItem(args, 2);
    if (!typedict)
        return NULL;
/*
  stgdict items size, align, length contain info about pointers itself,
  stgdict->proto has info about the pointed to type!
*/
    stgdict = (StgDictObject *)_PyObject_CallNoArg(
        (PyObject *)&PyCStgDict_Type);
    if (!stgdict)
        return NULL;
    stgdict->size = sizeof(void *);
    stgdict->align = _ctypes_get_fielddesc("P")->pffi_type->alignment;
    stgdict->length = 1;
    stgdict->ffi_type_pointer = ffi_type_pointer;
    stgdict->paramfunc = PyCPointerType_paramfunc;
    stgdict->flags |= TYPEFLAG_ISPOINTER;

    proto = _PyDict_GetItemIdWithError(typedict, &PyId__type_); /* Borrowed ref */
    if (proto) {
        StgDictObject *itemdict;
        const char *current_format;
        if (-1 == PyCPointerType_SetProto(stgdict, proto)) {
            Py_DECREF((PyObject *)stgdict);
            return NULL;
        }
        itemdict = PyType_stgdict(proto);
        /* PyCPointerType_SetProto has verified proto has a stgdict. */
        assert(itemdict);
        /* If itemdict->format is NULL, then this is a pointer to an
           incomplete type.  We create a generic format string
           'pointer to bytes' in this case.  XXX Better would be to
           fix the format string later...
        */
        current_format = itemdict->format ? itemdict->format : "B";
        if (itemdict->shape != NULL) {
            /* pointer to an array: the shape needs to be prefixed */
            stgdict->format = _ctypes_alloc_format_string_with_shape(
                itemdict->ndim, itemdict->shape, "&", current_format);
        } else {
            stgdict->format = _ctypes_alloc_format_string("&", current_format);
        }
        if (stgdict->format == NULL) {
            Py_DECREF((PyObject *)stgdict);
            return NULL;
        }
    }
    else if (PyErr_Occurred()) {
        Py_DECREF((PyObject *)stgdict);
        return NULL;
    }

    /* create the new instance (which is a class,
       since we are a metatype!) */
    result = (PyTypeObject *)PyType_Type.tp_new(type, args, kwds);
    if (result == NULL) {
        Py_DECREF((PyObject *)stgdict);
        return NULL;
    }

    /* replace the class dict by our updated spam dict */
    if (-1 == PyDict_Update((PyObject *)stgdict, result->tp_dict)) {
        Py_DECREF(result);
        Py_DECREF((PyObject *)stgdict);
        return NULL;
    }
    Py_SETREF(result->tp_dict, (PyObject *)stgdict);

    return (PyObject *)result;
}


static PyObject *
PyCPointerType_set_type(PyTypeObject *self, PyObject *type)
{
    StgDictObject *dict;
    _Py_IDENTIFIER(_type_);

    dict = PyType_stgdict((PyObject *)self);
    if (!dict) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return NULL;
    }

    if (-1 == PyCPointerType_SetProto(dict, type))
        return NULL;

    if (-1 == _PyDict_SetItemId((PyObject *)dict, &PyId__type_, type))
        return NULL;

    Py_RETURN_NONE;
}

static PyObject *_byref(PyObject *);

static PyObject *
PyCPointerType_from_param(PyObject *type, PyObject *value)
{
    StgDictObject *typedict;

    if (value == Py_None) {
        /* ConvParam will convert to a NULL pointer later */
        Py_INCREF(value);
        return value;
    }

    typedict = PyType_stgdict(type);
    if (!typedict) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return NULL;
    }

    /* If we expect POINTER(<type>), but receive a <type> instance, accept
       it by calling byref(<type>).
    */
    switch (PyObject_IsInstance(value, typedict->proto)) {
    case 1:
        Py_INCREF(value); /* _byref steals a refcount */
        return _byref(value);
    case -1:
        return NULL;
    default:
        break;
    }

    if (PointerObject_Check(value) || ArrayObject_Check(value)) {
        /* Array instances are also pointers when
           the item types are the same.
        */
        StgDictObject *v = PyObject_stgdict(value);
        assert(v); /* Cannot be NULL for pointer or array objects */
        int ret = PyObject_IsSubclass(v->proto, typedict->proto);
        if (ret < 0) {
            return NULL;
        }
        if (ret) {
            Py_INCREF(value);
            return value;
        }
    }
    return CDataType_from_param(type, value);
}

static PyMethodDef PyCPointerType_methods[] = {
    { "from_address", CDataType_from_address, METH_O, from_address_doc },
    { "from_buffer", CDataType_from_buffer, METH_VARARGS, from_buffer_doc, },
    { "from_buffer_copy", CDataType_from_buffer_copy, METH_VARARGS, from_buffer_copy_doc, },
    { "in_dll", CDataType_in_dll, METH_VARARGS, in_dll_doc},
    { "from_param", (PyCFunction)PyCPointerType_from_param, METH_O, from_param_doc},
    { "set_type", (PyCFunction)PyCPointerType_set_type, METH_O },
    { NULL, NULL },
};

PyTypeObject PyCPointerType_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.PyCPointerType",                                   /* tp_name */
    0,                                          /* tp_basicsize */
    0,                                          /* tp_itemsize */
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    &CDataType_as_sequence,             /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    "metatype for the Pointer Objects",         /* tp_doc */
    (traverseproc)CDataType_traverse,           /* tp_traverse */
    (inquiry)CDataType_clear,                   /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    PyCPointerType_methods,                     /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    PyCPointerType_new,                         /* tp_new */
    0,                                          /* tp_free */
};


/******************************************************************/
/*
  PyCArrayType_Type
*/
/*
  PyCArrayType_new ensures that the new Array subclass created has a _length_
  attribute, and a _type_ attribute.
*/

static int
CharArray_set_raw(CDataObject *self, PyObject *value, void *Py_UNUSED(ignored))
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

    memcpy(self->b_ptr, ptr, size);

    PyBuffer_Release(&view);
    return 0;
 fail:
    PyBuffer_Release(&view);
    return -1;
}

static PyObject *
CharArray_get_raw(CDataObject *self, void *Py_UNUSED(ignored))
{
    return PyBytes_FromStringAndSize(self->b_ptr, self->b_size);
}

static PyObject *
CharArray_get_value(CDataObject *self, void *Py_UNUSED(ignored))
{
    Py_ssize_t i;
    char *ptr = self->b_ptr;
    for (i = 0; i < self->b_size; ++i)
        if (*ptr++ == '\0')
            break;
    return PyBytes_FromStringAndSize(self->b_ptr, i);
}

static int
CharArray_set_value(CDataObject *self, PyObject *value, void *Py_UNUSED(ignored))
{
    char *ptr;
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
    { "raw", (getter)CharArray_get_raw, (setter)CharArray_set_raw,
      "value", NULL },
    { "value", (getter)CharArray_get_value, (setter)CharArray_set_value,
      "string value"},
    { NULL, NULL }
};

#ifdef CTYPES_UNICODE
static PyObject *
WCharArray_get_value(CDataObject *self, void *Py_UNUSED(ignored))
{
    Py_ssize_t i;
    wchar_t *ptr = (wchar_t *)self->b_ptr;
    for (i = 0; i < self->b_size/(Py_ssize_t)sizeof(wchar_t); ++i)
        if (*ptr++ == (wchar_t)0)
            break;
    return PyUnicode_FromWideChar((wchar_t *)self->b_ptr, i);
}

static int
WCharArray_set_value(CDataObject *self, PyObject *value, void *Py_UNUSED(ignored))
{
    Py_ssize_t result = 0;

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
    } else
        Py_INCREF(value);

    Py_ssize_t len = PyUnicode_AsWideChar(value, NULL, 0);
    if (len < 0) {
        return -1;
    }
    // PyUnicode_AsWideChar() returns number of wchars including trailing null byte,
    // when it is called with NULL.
    if (((size_t)len-1) > self->b_size/sizeof(wchar_t)) {
        PyErr_SetString(PyExc_ValueError, "string too long");
        result = -1;
        goto done;
    }
    result = PyUnicode_AsWideChar(value,
                                  (wchar_t *)self->b_ptr,
                                  self->b_size/sizeof(wchar_t));
    if (result >= 0 && (size_t)result < self->b_size/sizeof(wchar_t))
        ((wchar_t *)self->b_ptr)[result] = (wchar_t)0;
  done:
    Py_DECREF(value);

    return result >= 0 ? 0 : -1;
}

static PyGetSetDef WCharArray_getsets[] = {
    { "value", (getter)WCharArray_get_value, (setter)WCharArray_set_value,
      "string value"},
    { NULL, NULL }
};
#endif

/*
  The next three functions copied from Python's typeobject.c.

  They are used to attach methods, members, or getsets to a type *after* it
  has been created: Arrays of characters have additional getsets to treat them
  as strings.
 */
/*
static int
add_methods(PyTypeObject *type, PyMethodDef *meth)
{
    PyObject *dict = type->tp_dict;
    for (; meth->ml_name != NULL; meth++) {
        PyObject *descr;
        descr = PyDescr_NewMethod(type, meth);
        if (descr == NULL)
            return -1;
        if (PyDict_SetItemString(dict, meth->ml_name, descr) < 0) {
            Py_DECREF(descr);
            return -1;
        }
        Py_DECREF(descr);
    }
    return 0;
}

static int
add_members(PyTypeObject *type, PyMemberDef *memb)
{
    PyObject *dict = type->tp_dict;
    for (; memb->name != NULL; memb++) {
        PyObject *descr;
        descr = PyDescr_NewMember(type, memb);
        if (descr == NULL)
            return -1;
        if (PyDict_SetItemString(dict, memb->name, descr) < 0) {
            Py_DECREF(descr);
            return -1;
        }
        Py_DECREF(descr);
    }
    return 0;
}
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
PyCArrayType_paramfunc(CDataObject *self)
{
    PyCArgObject *p = PyCArgObject_new();
    if (p == NULL)
        return NULL;
    p->tag = 'P';
    p->pffi_type = &ffi_type_pointer;
    p->value.p = (char *)self->b_ptr;
    Py_INCREF(self);
    p->obj = (PyObject *)self;
    return p;
}

static PyObject *
PyCArrayType_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _Py_IDENTIFIER(_length_);
    _Py_IDENTIFIER(_type_);
    PyTypeObject *result;
    StgDictObject *stgdict;
    StgDictObject *itemdict;
    PyObject *length_attr, *type_attr;
    Py_ssize_t length;
    Py_ssize_t itemsize, itemalign;

    /* create the new instance (which is a class,
       since we are a metatype!) */
    result = (PyTypeObject *)PyType_Type.tp_new(type, args, kwds);
    if (result == NULL)
        return NULL;

    /* Initialize these variables to NULL so that we can simplify error
       handling by using Py_XDECREF.  */
    stgdict = NULL;
    type_attr = NULL;

    if (_PyObject_LookupAttrId((PyObject *)result, &PyId__length_, &length_attr) < 0) {
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

    if (_PyLong_Sign(length_attr) == -1) {
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

    if (_PyObject_LookupAttrId((PyObject *)result, &PyId__type_, &type_attr) < 0) {
        goto error;
    }
    if (!type_attr) {
        PyErr_SetString(PyExc_AttributeError,
                        "class must define a '_type_' attribute");
        goto error;
    }

    stgdict = (StgDictObject *)_PyObject_CallNoArg(
        (PyObject *)&PyCStgDict_Type);
    if (!stgdict)
        goto error;

    itemdict = PyType_stgdict(type_attr);
    if (!itemdict) {
        PyErr_SetString(PyExc_TypeError,
                        "_type_ must have storage info");
        goto error;
    }

    assert(itemdict->format);
    stgdict->format = _ctypes_alloc_format_string(NULL, itemdict->format);
    if (stgdict->format == NULL)
        goto error;
    stgdict->ndim = itemdict->ndim + 1;
    stgdict->shape = PyMem_Malloc(sizeof(Py_ssize_t) * stgdict->ndim);
    if (stgdict->shape == NULL) {
        PyErr_NoMemory();
        goto error;
    }
    stgdict->shape[0] = length;
    if (stgdict->ndim > 1) {
        memmove(&stgdict->shape[1], itemdict->shape,
            sizeof(Py_ssize_t) * (stgdict->ndim - 1));
    }

    itemsize = itemdict->size;
    if (itemsize != 0 && length > PY_SSIZE_T_MAX / itemsize) {
        PyErr_SetString(PyExc_OverflowError,
                        "array too large");
        goto error;
    }

    itemalign = itemdict->align;

    if (itemdict->flags & (TYPEFLAG_ISPOINTER | TYPEFLAG_HASPOINTER))
        stgdict->flags |= TYPEFLAG_HASPOINTER;

    stgdict->size = itemsize * length;
    stgdict->align = itemalign;
    stgdict->length = length;
    stgdict->proto = type_attr;
    type_attr = NULL;

    stgdict->paramfunc = &PyCArrayType_paramfunc;

    /* Arrays are passed as pointers to function calls. */
    stgdict->ffi_type_pointer = ffi_type_pointer;

    /* replace the class dict by our updated spam dict */
    if (-1 == PyDict_Update((PyObject *)stgdict, result->tp_dict))
        goto error;
    Py_SETREF(result->tp_dict, (PyObject *)stgdict);  /* steal the reference */
    stgdict = NULL;

    /* Special case for character arrays.
       A permanent annoyance: char arrays are also strings!
    */
    if (itemdict->getfunc == _ctypes_get_fielddesc("c")->getfunc) {
        if (-1 == add_getset(result, CharArray_getsets))
            goto error;
#ifdef CTYPES_UNICODE
    } else if (itemdict->getfunc == _ctypes_get_fielddesc("u")->getfunc) {
        if (-1 == add_getset(result, WCharArray_getsets))
            goto error;
#endif
    }

    return (PyObject *)result;
error:
    Py_XDECREF((PyObject*)stgdict);
    Py_XDECREF(type_attr);
    Py_DECREF(result);
    return NULL;
}

PyTypeObject PyCArrayType_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.PyCArrayType",                     /* tp_name */
    0,                                          /* tp_basicsize */
    0,                                          /* tp_itemsize */
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    &CDataType_as_sequence,                     /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "metatype for the Array Objects",           /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    CDataType_methods,                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    PyCArrayType_new,                                   /* tp_new */
    0,                                          /* tp_free */
};


/******************************************************************/
/*
  PyCSimpleType_Type
*/
/*

PyCSimpleType_new ensures that the new Simple_Type subclass created has a valid
_type_ attribute.

*/

static const char SIMPLE_TYPE_CHARS[] = "cbBhHiIlLdfuzZqQPXOv?g";

static PyObject *
c_wchar_p_from_param(PyObject *type, PyObject *value)
{
    _Py_IDENTIFIER(_as_parameter_);
    PyObject *as_parameter;
    int res;
    if (value == Py_None) {
        Py_RETURN_NONE;
    }
    if (PyUnicode_Check(value)) {
        PyCArgObject *parg;
        struct fielddesc *fd = _ctypes_get_fielddesc("Z");

        parg = PyCArgObject_new();
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
        Py_INCREF(value);
        return value;
    }
    if (ArrayObject_Check(value) || PointerObject_Check(value)) {
        /* c_wchar array instance or pointer(c_wchar(...)) */
        StgDictObject *dt = PyObject_stgdict(value);
        StgDictObject *dict;
        assert(dt); /* Cannot be NULL for pointer or array objects */
        dict = dt && dt->proto ? PyType_stgdict(dt->proto) : NULL;
        if (dict && (dict->setfunc == _ctypes_get_fielddesc("u")->setfunc)) {
            Py_INCREF(value);
            return value;
        }
    }
    if (PyCArg_CheckExact(value)) {
        /* byref(c_char(...)) */
        PyCArgObject *a = (PyCArgObject *)value;
        StgDictObject *dict = PyObject_stgdict(a->obj);
        if (dict && (dict->setfunc == _ctypes_get_fielddesc("u")->setfunc)) {
            Py_INCREF(value);
            return value;
        }
    }

    if (_PyObject_LookupAttrId(value, &PyId__as_parameter_, &as_parameter) < 0) {
        return NULL;
    }
    if (as_parameter) {
        value = c_wchar_p_from_param(type, as_parameter);
        Py_DECREF(as_parameter);
        return value;
    }
    /* XXX better message */
    PyErr_SetString(PyExc_TypeError,
                    "wrong type");
    return NULL;
}

static PyObject *
c_char_p_from_param(PyObject *type, PyObject *value)
{
    _Py_IDENTIFIER(_as_parameter_);
    PyObject *as_parameter;
    int res;
    if (value == Py_None) {
        Py_RETURN_NONE;
    }
    if (PyBytes_Check(value)) {
        PyCArgObject *parg;
        struct fielddesc *fd = _ctypes_get_fielddesc("z");

        parg = PyCArgObject_new();
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
        Py_INCREF(value);
        return value;
    }
    if (ArrayObject_Check(value) || PointerObject_Check(value)) {
        /* c_char array instance or pointer(c_char(...)) */
        StgDictObject *dt = PyObject_stgdict(value);
        StgDictObject *dict;
        assert(dt); /* Cannot be NULL for pointer or array objects */
        dict = dt && dt->proto ? PyType_stgdict(dt->proto) : NULL;
        if (dict && (dict->setfunc == _ctypes_get_fielddesc("c")->setfunc)) {
            Py_INCREF(value);
            return value;
        }
    }
    if (PyCArg_CheckExact(value)) {
        /* byref(c_char(...)) */
        PyCArgObject *a = (PyCArgObject *)value;
        StgDictObject *dict = PyObject_stgdict(a->obj);
        if (dict && (dict->setfunc == _ctypes_get_fielddesc("c")->setfunc)) {
            Py_INCREF(value);
            return value;
        }
    }

    if (_PyObject_LookupAttrId(value, &PyId__as_parameter_, &as_parameter) < 0) {
        return NULL;
    }
    if (as_parameter) {
        value = c_char_p_from_param(type, as_parameter);
        Py_DECREF(as_parameter);
        return value;
    }
    /* XXX better message */
    PyErr_SetString(PyExc_TypeError,
                    "wrong type");
    return NULL;
}

static PyObject *
c_void_p_from_param(PyObject *type, PyObject *value)
{
    _Py_IDENTIFIER(_as_parameter_);
    StgDictObject *stgd;
    PyObject *as_parameter;
    int res;

/* None */
    if (value == Py_None) {
        Py_RETURN_NONE;
    }
    /* Should probably allow buffer interface as well */
/* int, long */
    if (PyLong_Check(value)) {
        PyCArgObject *parg;
        struct fielddesc *fd = _ctypes_get_fielddesc("P");

        parg = PyCArgObject_new();
        if (parg == NULL)
            return NULL;
        parg->pffi_type = &ffi_type_pointer;
        parg->tag = 'P';
        parg->obj = fd->setfunc(&parg->value, value, 0);
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

        parg = PyCArgObject_new();
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

        parg = PyCArgObject_new();
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
        Py_INCREF(value);
        return value;
    }
/* ctypes array or pointer instance */
    if (ArrayObject_Check(value) || PointerObject_Check(value)) {
        /* Any array or pointer is accepted */
        Py_INCREF(value);
        return value;
    }
/* byref(...) */
    if (PyCArg_CheckExact(value)) {
        /* byref(c_xxx()) */
        PyCArgObject *a = (PyCArgObject *)value;
        if (a->tag == 'P') {
            Py_INCREF(value);
            return value;
        }
    }
/* function pointer */
    if (PyCFuncPtrObject_Check(value)) {
        PyCArgObject *parg;
        PyCFuncPtrObject *func;
        func = (PyCFuncPtrObject *)value;
        parg = PyCArgObject_new();
        if (parg == NULL)
            return NULL;
        parg->pffi_type = &ffi_type_pointer;
        parg->tag = 'P';
        Py_INCREF(value);
        parg->value.p = *(void **)func->b_ptr;
        parg->obj = value;
        return (PyObject *)parg;
    }
/* c_char_p, c_wchar_p */
    stgd = PyObject_stgdict(value);
    if (stgd && CDataObject_Check(value) && stgd->proto && PyUnicode_Check(stgd->proto)) {
        PyCArgObject *parg;

        switch (PyUnicode_AsUTF8(stgd->proto)[0]) {
        case 'z': /* c_char_p */
        case 'Z': /* c_wchar_p */
            parg = PyCArgObject_new();
            if (parg == NULL)
                return NULL;
            parg->pffi_type = &ffi_type_pointer;
            parg->tag = 'Z';
            Py_INCREF(value);
            parg->obj = value;
            /* Remember: b_ptr points to where the pointer is stored! */
            parg->value.p = *(void **)(((CDataObject *)value)->b_ptr);
            return (PyObject *)parg;
        }
    }

    if (_PyObject_LookupAttrId(value, &PyId__as_parameter_, &as_parameter) < 0) {
        return NULL;
    }
    if (as_parameter) {
        value = c_void_p_from_param(type, as_parameter);
        Py_DECREF(as_parameter);
        return value;
    }
    /* XXX better message */
    PyErr_SetString(PyExc_TypeError,
                    "wrong type");
    return NULL;
}

static PyMethodDef c_void_p_method = { "from_param", c_void_p_from_param, METH_O };
static PyMethodDef c_char_p_method = { "from_param", c_char_p_from_param, METH_O };
static PyMethodDef c_wchar_p_method = { "from_param", c_wchar_p_from_param, METH_O };

static PyObject *CreateSwappedType(PyTypeObject *type, PyObject *args, PyObject *kwds,
                                   PyObject *proto, struct fielddesc *fmt)
{
    PyTypeObject *result;
    StgDictObject *stgdict;
    PyObject *name = PyTuple_GET_ITEM(args, 0);
    PyObject *newname;
    PyObject *swapped_args;
    static PyObject *suffix;
    Py_ssize_t i;

    swapped_args = PyTuple_New(PyTuple_GET_SIZE(args));
    if (!swapped_args)
        return NULL;

    if (suffix == NULL)
#ifdef WORDS_BIGENDIAN
        suffix = PyUnicode_InternFromString("_le");
#else
        suffix = PyUnicode_InternFromString("_be");
#endif
    if (suffix == NULL) {
        Py_DECREF(swapped_args);
        return NULL;
    }

    newname = PyUnicode_Concat(name, suffix);
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

    stgdict = (StgDictObject *)_PyObject_CallNoArg(
        (PyObject *)&PyCStgDict_Type);
    if (!stgdict) {
        Py_DECREF(result);
        return NULL;
    }

    stgdict->ffi_type_pointer = *fmt->pffi_type;
    stgdict->align = fmt->pffi_type->alignment;
    stgdict->length = 0;
    stgdict->size = fmt->pffi_type->size;
    stgdict->setfunc = fmt->setfunc_swapped;
    stgdict->getfunc = fmt->getfunc_swapped;

    Py_INCREF(proto);
    stgdict->proto = proto;

    /* replace the class dict by our updated spam dict */
    if (-1 == PyDict_Update((PyObject *)stgdict, result->tp_dict)) {
        Py_DECREF(result);
        Py_DECREF((PyObject *)stgdict);
        return NULL;
    }
    Py_SETREF(result->tp_dict, (PyObject *)stgdict);

    return (PyObject *)result;
}

static PyCArgObject *
PyCSimpleType_paramfunc(CDataObject *self)
{
    StgDictObject *dict;
    const char *fmt;
    PyCArgObject *parg;
    struct fielddesc *fd;

    dict = PyObject_stgdict((PyObject *)self);
    assert(dict); /* Cannot be NULL for CDataObject instances */
    fmt = PyUnicode_AsUTF8(dict->proto);
    assert(fmt);

    fd = _ctypes_get_fielddesc(fmt);
    assert(fd);

    parg = PyCArgObject_new();
    if (parg == NULL)
        return NULL;

    parg->tag = fmt[0];
    parg->pffi_type = fd->pffi_type;
    Py_INCREF(self);
    parg->obj = (PyObject *)self;
    memcpy(&parg->value, self->b_ptr, self->b_size);
    return parg;
}

static PyObject *
PyCSimpleType_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _Py_IDENTIFIER(_type_);
    PyTypeObject *result;
    StgDictObject *stgdict;
    PyObject *proto;
    const char *proto_str;
    Py_ssize_t proto_len;
    PyMethodDef *ml;
    struct fielddesc *fmt;

    /* create the new instance (which is a class,
       since we are a metatype!) */
    result = (PyTypeObject *)PyType_Type.tp_new(type, args, kwds);
    if (result == NULL)
        return NULL;

    if (_PyObject_LookupAttrId((PyObject *)result, &PyId__type_, &proto) < 0) {
        return NULL;
    }
    if (!proto) {
        PyErr_SetString(PyExc_AttributeError,
                        "class must define a '_type_' attribute");
  error:
        Py_XDECREF(proto);
        Py_DECREF(result);
        return NULL;
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
    if (!strchr(SIMPLE_TYPE_CHARS, *proto_str)) {
        PyErr_Format(PyExc_AttributeError,
                     "class must define a '_type_' attribute which must be\n"
                     "a single character string containing one of '%s'.",
                     SIMPLE_TYPE_CHARS);
        goto error;
    }
    fmt = _ctypes_get_fielddesc(proto_str);
    if (fmt == NULL) {
        PyErr_Format(PyExc_ValueError,
                     "_type_ '%s' not supported", proto_str);
        goto error;
    }

    stgdict = (StgDictObject *)_PyObject_CallNoArg(
        (PyObject *)&PyCStgDict_Type);
    if (!stgdict)
        goto error;

    stgdict->ffi_type_pointer = *fmt->pffi_type;
    stgdict->align = fmt->pffi_type->alignment;
    stgdict->length = 0;
    stgdict->size = fmt->pffi_type->size;
    stgdict->setfunc = fmt->setfunc;
    stgdict->getfunc = fmt->getfunc;
#ifdef WORDS_BIGENDIAN
    stgdict->format = _ctypes_alloc_format_string_for_type(proto_str[0], 1);
#else
    stgdict->format = _ctypes_alloc_format_string_for_type(proto_str[0], 0);
#endif
    if (stgdict->format == NULL) {
        Py_DECREF(result);
        Py_DECREF(proto);
        Py_DECREF((PyObject *)stgdict);
        return NULL;
    }

    stgdict->paramfunc = PyCSimpleType_paramfunc;
/*
    if (result->tp_base != &Simple_Type) {
        stgdict->setfunc = NULL;
        stgdict->getfunc = NULL;
    }
*/

    /* This consumes the refcount on proto which we have */
    stgdict->proto = proto;

    /* replace the class dict by our updated spam dict */
    if (-1 == PyDict_Update((PyObject *)stgdict, result->tp_dict)) {
        Py_DECREF(result);
        Py_DECREF((PyObject *)stgdict);
        return NULL;
    }
    Py_SETREF(result->tp_dict, (PyObject *)stgdict);

    /* Install from_param class methods in ctypes base classes.
       Overrides the PyCSimpleType_from_param generic method.
     */
    if (result->tp_base == &Simple_Type) {
        switch (*proto_str) {
        case 'z': /* c_char_p */
            ml = &c_char_p_method;
            stgdict->flags |= TYPEFLAG_ISPOINTER;
            break;
        case 'Z': /* c_wchar_p */
            ml = &c_wchar_p_method;
            stgdict->flags |= TYPEFLAG_ISPOINTER;
            break;
        case 'P': /* c_void_p */
            ml = &c_void_p_method;
            stgdict->flags |= TYPEFLAG_ISPOINTER;
            break;
        case 's':
        case 'X':
        case 'O':
            ml = NULL;
            stgdict->flags |= TYPEFLAG_ISPOINTER;
            break;
        default:
            ml = NULL;
            break;
        }

        if (ml) {
            PyObject *meth;
            int x;
            meth = PyDescr_NewClassMethod(result, ml);
            if (!meth) {
                Py_DECREF(result);
                return NULL;
            }
            x = PyDict_SetItemString(result->tp_dict,
                                     ml->ml_name,
                                     meth);
            Py_DECREF(meth);
            if (x == -1) {
                Py_DECREF(result);
                return NULL;
            }
        }
    }

    if (type == &PyCSimpleType_Type && fmt->setfunc_swapped && fmt->getfunc_swapped) {
        PyObject *swapped = CreateSwappedType(type, args, kwds,
                                              proto, fmt);
        StgDictObject *sw_dict;
        if (swapped == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        sw_dict = PyType_stgdict(swapped);
#ifdef WORDS_BIGENDIAN
        PyObject_SetAttrString((PyObject *)result, "__ctype_le__", swapped);
        PyObject_SetAttrString((PyObject *)result, "__ctype_be__", (PyObject *)result);
        PyObject_SetAttrString(swapped, "__ctype_be__", (PyObject *)result);
        PyObject_SetAttrString(swapped, "__ctype_le__", swapped);
        /* We are creating the type for the OTHER endian */
        sw_dict->format = _ctypes_alloc_format_string("<", stgdict->format+1);
#else
        PyObject_SetAttrString((PyObject *)result, "__ctype_be__", swapped);
        PyObject_SetAttrString((PyObject *)result, "__ctype_le__", (PyObject *)result);
        PyObject_SetAttrString(swapped, "__ctype_le__", (PyObject *)result);
        PyObject_SetAttrString(swapped, "__ctype_be__", swapped);
        /* We are creating the type for the OTHER endian */
        sw_dict->format = _ctypes_alloc_format_string(">", stgdict->format+1);
#endif
        Py_DECREF(swapped);
        if (PyErr_Occurred()) {
            Py_DECREF(result);
            return NULL;
        }
    };

    return (PyObject *)result;
}

/*
 * This is a *class method*.
 * Convert a parameter into something that ConvParam can handle.
 */
static PyObject *
PyCSimpleType_from_param(PyObject *type, PyObject *value)
{
    _Py_IDENTIFIER(_as_parameter_);
    StgDictObject *dict;
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
        Py_INCREF(value);
        return value;
    }

    dict = PyType_stgdict(type);
    if (!dict) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return NULL;
    }

    /* I think we can rely on this being a one-character string */
    fmt = PyUnicode_AsUTF8(dict->proto);
    assert(fmt);

    fd = _ctypes_get_fielddesc(fmt);
    assert(fd);

    parg = PyCArgObject_new();
    if (parg == NULL)
        return NULL;

    parg->tag = fmt[0];
    parg->pffi_type = fd->pffi_type;
    parg->obj = fd->setfunc(&parg->value, value, 0);
    if (parg->obj)
        return (PyObject *)parg;
    PyErr_Clear();
    Py_DECREF(parg);

    if (_PyObject_LookupAttrId(value, &PyId__as_parameter_, &as_parameter) < 0) {
        return NULL;
    }
    if (as_parameter) {
        if (Py_EnterRecursiveCall("while processing _as_parameter_")) {
            Py_DECREF(as_parameter);
            return NULL;
        }
        value = PyCSimpleType_from_param(type, as_parameter);
        Py_LeaveRecursiveCall();
        Py_DECREF(as_parameter);
        return value;
    }
    PyErr_SetString(PyExc_TypeError,
                    "wrong type");
    return NULL;
}

static PyMethodDef PyCSimpleType_methods[] = {
    { "from_param", PyCSimpleType_from_param, METH_O, from_param_doc },
    { "from_address", CDataType_from_address, METH_O, from_address_doc },
    { "from_buffer", CDataType_from_buffer, METH_VARARGS, from_buffer_doc, },
    { "from_buffer_copy", CDataType_from_buffer_copy, METH_VARARGS, from_buffer_copy_doc, },
    { "in_dll", CDataType_in_dll, METH_VARARGS, in_dll_doc},
    { NULL, NULL },
};

PyTypeObject PyCSimpleType_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.PyCSimpleType",                                    /* tp_name */
    0,                                          /* tp_basicsize */
    0,                                          /* tp_itemsize */
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    &CDataType_as_sequence,             /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "metatype for the PyCSimpleType Objects",           /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    PyCSimpleType_methods,                      /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    PyCSimpleType_new,                                  /* tp_new */
    0,                                          /* tp_free */
};

/******************************************************************/
/*
  PyCFuncPtrType_Type
 */

static PyObject *
converters_from_argtypes(PyObject *ob)
{
    _Py_IDENTIFIER(from_param);
    PyObject *converters;
    Py_ssize_t i;
    Py_ssize_t nArgs;

    ob = PySequence_Tuple(ob); /* new reference */
    if (!ob) {
        PyErr_SetString(PyExc_TypeError,
                        "_argtypes_ must be a sequence of types");
        return NULL;
    }

    nArgs = PyTuple_GET_SIZE(ob);
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

        StgDictObject *stgdict = PyType_stgdict(tp);

        if (stgdict != NULL) {
            if (stgdict->flags & TYPEFLAG_HASUNION) {
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
            if (stgdict->flags & TYPEFLAG_HASBITFIELD) {
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

        if (_PyObject_LookupAttrId(tp, &PyId_from_param, &cnv) <= 0) {
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
make_funcptrtype_dict(StgDictObject *stgdict)
{
    PyObject *ob;
    PyObject *converters = NULL;
    _Py_IDENTIFIER(_flags_);
    _Py_IDENTIFIER(_argtypes_);
    _Py_IDENTIFIER(_restype_);
    _Py_IDENTIFIER(_check_retval_);

    stgdict->align = _ctypes_get_fielddesc("P")->pffi_type->alignment;
    stgdict->length = 1;
    stgdict->size = sizeof(void *);
    stgdict->setfunc = NULL;
    stgdict->getfunc = NULL;
    stgdict->ffi_type_pointer = ffi_type_pointer;

    ob = _PyDict_GetItemIdWithError((PyObject *)stgdict, &PyId__flags_);
    if (!ob || !PyLong_Check(ob)) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_TypeError,
                "class must define _flags_ which must be an integer");
        }
        return -1;
    }
    stgdict->flags = PyLong_AsUnsignedLongMask(ob) | TYPEFLAG_ISPOINTER;

    /* _argtypes_ is optional... */
    ob = _PyDict_GetItemIdWithError((PyObject *)stgdict, &PyId__argtypes_);
    if (ob) {
        converters = converters_from_argtypes(ob);
        if (!converters)
            return -1;
        Py_INCREF(ob);
        stgdict->argtypes = ob;
        stgdict->converters = converters;
    }
    else if (PyErr_Occurred()) {
        return -1;
    }

    ob = _PyDict_GetItemIdWithError((PyObject *)stgdict, &PyId__restype_);
    if (ob) {
        if (ob != Py_None && !PyType_stgdict(ob) && !PyCallable_Check(ob)) {
            PyErr_SetString(PyExc_TypeError,
                "_restype_ must be a type, a callable, or None");
            return -1;
        }
        Py_INCREF(ob);
        stgdict->restype = ob;
        if (_PyObject_LookupAttrId(ob, &PyId__check_retval_,
                                   &stgdict->checker) < 0)
        {
            return -1;
        }
    }
    else if (PyErr_Occurred()) {
        return -1;
    }
/* XXX later, maybe.
    ob = _PyDict_GetItemIdWithError((PyObject *)stgdict, &PyId__errcheck_);
    if (ob) {
        if (!PyCallable_Check(ob)) {
            PyErr_SetString(PyExc_TypeError,
                "_errcheck_ must be callable");
            return -1;
        }
        Py_INCREF(ob);
        stgdict->errcheck = ob;
    }
    else if (PyErr_Occurred()) {
        return -1;
    }
*/
    return 0;
}

static PyCArgObject *
PyCFuncPtrType_paramfunc(CDataObject *self)
{
    PyCArgObject *parg;

    parg = PyCArgObject_new();
    if (parg == NULL)
        return NULL;

    parg->tag = 'P';
    parg->pffi_type = &ffi_type_pointer;
    Py_INCREF(self);
    parg->obj = (PyObject *)self;
    parg->value.p = *(void **)self->b_ptr;
    return parg;
}

static PyObject *
PyCFuncPtrType_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyTypeObject *result;
    StgDictObject *stgdict;

    stgdict = (StgDictObject *)_PyObject_CallNoArg(
        (PyObject *)&PyCStgDict_Type);
    if (!stgdict)
        return NULL;

    stgdict->paramfunc = PyCFuncPtrType_paramfunc;
    /* We do NOT expose the function signature in the format string.  It
       is impossible, generally, because the only requirement for the
       argtypes items is that they have a .from_param method - we do not
       know the types of the arguments (although, in practice, most
       argtypes would be a ctypes type).
    */
    stgdict->format = _ctypes_alloc_format_string(NULL, "X{}");
    if (stgdict->format == NULL) {
        Py_DECREF((PyObject *)stgdict);
        return NULL;
    }
    stgdict->flags |= TYPEFLAG_ISPOINTER;

    /* create the new instance (which is a class,
       since we are a metatype!) */
    result = (PyTypeObject *)PyType_Type.tp_new(type, args, kwds);
    if (result == NULL) {
        Py_DECREF((PyObject *)stgdict);
        return NULL;
    }

    /* replace the class dict by our updated storage dict */
    if (-1 == PyDict_Update((PyObject *)stgdict, result->tp_dict)) {
        Py_DECREF(result);
        Py_DECREF((PyObject *)stgdict);
        return NULL;
    }
    Py_SETREF(result->tp_dict, (PyObject *)stgdict);

    if (-1 == make_funcptrtype_dict(stgdict)) {
        Py_DECREF(result);
        return NULL;
    }

    return (PyObject *)result;
}

PyTypeObject PyCFuncPtrType_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.PyCFuncPtrType",                           /* tp_name */
    0,                                          /* tp_basicsize */
    0,                                          /* tp_itemsize */
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    &CDataType_as_sequence,                     /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    "metatype for C function pointers",         /* tp_doc */
    (traverseproc)CDataType_traverse,           /* tp_traverse */
    (inquiry)CDataType_clear,                   /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    CDataType_methods,                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    PyCFuncPtrType_new,                         /* tp_new */
    0,                                          /* tp_free */
};


/*****************************************************************
 * Code to keep needed objects alive
 */

static CDataObject *
PyCData_GetContainer(CDataObject *self)
{
    while (self->b_base)
        self = self->b_base;
    if (self->b_objects == NULL) {
        if (self->b_length) {
            self->b_objects = PyDict_New();
            if (self->b_objects == NULL)
                return NULL;
        } else {
            Py_INCREF(Py_None);
            self->b_objects = Py_None;
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
static int
PyCData_traverse(CDataObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->b_objects);
    Py_VISIT((PyObject *)self->b_base);
    return 0;
}

static int
PyCData_clear(CDataObject *self)
{
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
    PyCData_clear((CDataObject *)self);
    Py_TYPE(self)->tp_free(self);
}

static PyMemberDef PyCData_members[] = {
    { "_b_base_", T_OBJECT,
      offsetof(CDataObject, b_base), READONLY,
      "the base object" },
    { "_b_needsfree_", T_INT,
      offsetof(CDataObject, b_needsfree), READONLY,
      "whether the object owns the memory or not" },
    { "_objects", T_OBJECT,
      offsetof(CDataObject, b_objects), READONLY,
      "internal objects tree (NEVER CHANGE THIS OBJECT!)"},
    { NULL },
};

static int PyCData_NewGetBuffer(PyObject *myself, Py_buffer *view, int flags)
{
    CDataObject *self = (CDataObject *)myself;
    StgDictObject *dict = PyObject_stgdict(myself);
    Py_ssize_t i;

    if (view == NULL) return 0;

    view->buf = self->b_ptr;
    view->obj = myself;
    Py_INCREF(myself);
    view->len = self->b_size;
    view->readonly = 0;
    /* use default format character if not set */
    view->format = dict->format ? dict->format : "B";
    view->ndim = dict->ndim;
    view->shape = dict->shape;
    view->itemsize = self->b_size;
    if (view->itemsize) {
        for (i = 0; i < view->ndim; ++i) {
            view->itemsize /= dict->shape[i];
        }
    }
    view->strides = NULL;
    view->suboffsets = NULL;
    view->internal = NULL;
    return 0;
}

static PyBufferProcs PyCData_as_buffer = {
    PyCData_NewGetBuffer,
    NULL,
};

/*
 * CData objects are mutable, so they cannot be hashable!
 */
static Py_hash_t
PyCData_nohash(PyObject *self)
{
    PyErr_SetString(PyExc_TypeError, "unhashable type");
    return -1;
}

static PyObject *
PyCData_reduce(PyObject *myself, PyObject *args)
{
    CDataObject *self = (CDataObject *)myself;

    if (PyObject_stgdict(myself)->flags & (TYPEFLAG_ISPOINTER|TYPEFLAG_HASPOINTER)) {
        PyErr_SetString(PyExc_ValueError,
                        "ctypes objects containing pointers cannot be pickled");
        return NULL;
    }
    PyObject *dict = PyObject_GetAttrString(myself, "__dict__");
    if (dict == NULL) {
        return NULL;
    }
    return Py_BuildValue("O(O(NN))", _unpickle, Py_TYPE(myself), dict,
                         PyBytes_FromStringAndSize(self->b_ptr, self->b_size));
}

static PyObject *
PyCData_setstate(PyObject *myself, PyObject *args)
{
    void *data;
    Py_ssize_t len;
    int res;
    PyObject *dict, *mydict;
    CDataObject *self = (CDataObject *)myself;
    if (!PyArg_ParseTuple(args, "O!s#",
                          &PyDict_Type, &dict, &data, &len))
    {
        return NULL;
    }
    if (len > self->b_size)
        len = self->b_size;
    memmove(self->b_ptr, data, len);
    mydict = PyObject_GetAttrString(myself, "__dict__");
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
    res = PyDict_Update(mydict, dict);
    Py_DECREF(mydict);
    if (res == -1)
        return NULL;
    Py_RETURN_NONE;
}

/*
 * default __ctypes_from_outparam__ method returns self.
 */
static PyObject *
PyCData_from_outparam(PyObject *self, PyObject *args)
{
    Py_INCREF(self);
    return self;
}

static PyMethodDef PyCData_methods[] = {
    { "__ctypes_from_outparam__", PyCData_from_outparam, METH_NOARGS, },
    { "__reduce__", PyCData_reduce, METH_NOARGS, },
    { "__setstate__", PyCData_setstate, METH_VARARGS, },
    { NULL, NULL },
};

PyTypeObject PyCData_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes._CData",
    sizeof(CDataObject),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
    PyCData_dealloc,                                    /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    PyCData_nohash,                             /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    &PyCData_as_buffer,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "XXX to be provided",                       /* tp_doc */
    (traverseproc)PyCData_traverse,             /* tp_traverse */
    (inquiry)PyCData_clear,                     /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    PyCData_methods,                                    /* tp_methods */
    PyCData_members,                                    /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
    0,                                          /* tp_free */
};

static int PyCData_MallocBuffer(CDataObject *obj, StgDictObject *dict)
{
    if ((size_t)dict->size <= sizeof(obj->b_value)) {
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
        obj->b_ptr = (char *)PyMem_Malloc(dict->size);
        if (obj->b_ptr == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        obj->b_needsfree = 1;
        memset(obj->b_ptr, 0, dict->size);
    }
    obj->b_size = dict->size;
    return 0;
}

PyObject *
PyCData_FromBaseObj(PyObject *type, PyObject *base, Py_ssize_t index, char *adr)
{
    CDataObject *cmem;
    StgDictObject *dict;

    assert(PyType_Check(type));
    dict = PyType_stgdict(type);
    if (!dict) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return NULL;
    }
    dict->flags |= DICTFLAG_FINAL;
    cmem = (CDataObject *)((PyTypeObject *)type)->tp_alloc((PyTypeObject *)type, 0);
    if (cmem == NULL)
        return NULL;
    assert(CDataObject_Check(cmem));

    cmem->b_length = dict->length;
    cmem->b_size = dict->size;
    if (base) { /* use base's buffer */
        assert(CDataObject_Check(base));
        cmem->b_ptr = adr;
        cmem->b_needsfree = 0;
        Py_INCREF(base);
        cmem->b_base = (CDataObject *)base;
        cmem->b_index = index;
    } else { /* copy contents of adr */
        if (-1 == PyCData_MallocBuffer(cmem, dict)) {
            Py_DECREF(cmem);
            return NULL;
        }
        memcpy(cmem->b_ptr, adr, dict->size);
        cmem->b_index = index;
    }
    return (PyObject *)cmem;
}

/*
 Box a memory block into a CData instance.
*/
PyObject *
PyCData_AtAddress(PyObject *type, void *buf)
{
    CDataObject *pd;
    StgDictObject *dict;

    if (PySys_Audit("ctypes.cdata", "n", (Py_ssize_t)buf) < 0) {
        return NULL;
    }

    assert(PyType_Check(type));
    dict = PyType_stgdict(type);
    if (!dict) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return NULL;
    }
    dict->flags |= DICTFLAG_FINAL;

    pd = (CDataObject *)((PyTypeObject *)type)->tp_alloc((PyTypeObject *)type, 0);
    if (!pd)
        return NULL;
    assert(CDataObject_Check(pd));
    pd->b_ptr = (char *)buf;
    pd->b_length = dict->length;
    pd->b_size = dict->size;
    return (PyObject *)pd;
}

/*
  This function returns TRUE for c_int, c_void_p, and these kind of
  classes.  FALSE otherwise FALSE also for subclasses of c_int and
  such.
*/
int _ctypes_simple_instance(PyObject *obj)
{
    PyTypeObject *type = (PyTypeObject *)obj;

    if (PyCSimpleTypeObject_Check(type))
        return type->tp_base != &Simple_Type;
    return 0;
}

PyObject *
PyCData_get(PyObject *type, GETFUNC getfunc, PyObject *src,
          Py_ssize_t index, Py_ssize_t size, char *adr)
{
    StgDictObject *dict;
    if (getfunc)
        return getfunc(adr, size);
    assert(type);
    dict = PyType_stgdict(type);
    if (dict && dict->getfunc && !_ctypes_simple_instance(type))
        return dict->getfunc(adr, size);
    return PyCData_FromBaseObj(type, src, index, adr);
}

/*
  Helper function for PyCData_set below.
*/
static PyObject *
_PyCData_set(CDataObject *dst, PyObject *type, SETFUNC setfunc, PyObject *value,
           Py_ssize_t size, char *ptr)
{
    CDataObject *src;
    int err;

    if (setfunc)
        return setfunc(ptr, value, size);

    if (!CDataObject_Check(value)) {
        StgDictObject *dict = PyType_stgdict(type);
        if (dict && dict->setfunc)
            return dict->setfunc(ptr, value, size);
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
            result = _PyCData_set(dst, type, setfunc, ob,
                                size, ptr);
            Py_DECREF(ob);
            return result;
        } else if (value == Py_None && PyCPointerTypeObject_Check(type)) {
            *(void **)ptr = NULL;
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
        memcpy(ptr,
               src->b_ptr,
               size);

        if (PyCPointerTypeObject_Check(type)) {
            /* XXX */
        }

        value = GetKeepedObjects(src);
        if (value == NULL)
            return NULL;

        Py_INCREF(value);
        return value;
    }

    if (PyCPointerTypeObject_Check(type)
        && ArrayObject_Check(value)) {
        StgDictObject *p1, *p2;
        PyObject *keep;
        p1 = PyObject_stgdict(value);
        assert(p1); /* Cannot be NULL for array instances */
        p2 = PyType_stgdict(type);
        assert(p2); /* Cannot be NULL for pointer types */

        if (p1->proto != p2->proto) {
            PyErr_Format(PyExc_TypeError,
                         "incompatible types, %s instance instead of %s instance",
                         Py_TYPE(value)->tp_name,
                         ((PyTypeObject *)type)->tp_name);
            return NULL;
        }
        *(void **)ptr = src->b_ptr;

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
PyCData_set(PyObject *dst, PyObject *type, SETFUNC setfunc, PyObject *value,
          Py_ssize_t index, Py_ssize_t size, char *ptr)
{
    CDataObject *mem = (CDataObject *)dst;
    PyObject *result;

    if (!CDataObject_Check(dst)) {
        PyErr_SetString(PyExc_TypeError,
                        "not a ctype instance");
        return -1;
    }

    result = _PyCData_set(mem, type, setfunc, value,
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
    CDataObject *obj;
    StgDictObject *dict;

    dict = PyType_stgdict((PyObject *)type);
    if (!dict) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return NULL;
    }
    dict->flags |= DICTFLAG_FINAL;

    obj = (CDataObject *)type->tp_alloc(type, 0);
    if (!obj)
        return NULL;

    obj->b_base = NULL;
    obj->b_index = 0;
    obj->b_objects = NULL;
    obj->b_length = dict->length;

    if (-1 == PyCData_MallocBuffer(obj, dict)) {
        Py_DECREF(obj);
        return NULL;
    }
    return (PyObject *)obj;
}
/*****************************************************************/
/*
  PyCFuncPtr_Type
*/

static int
PyCFuncPtr_set_errcheck(PyCFuncPtrObject *self, PyObject *ob, void *Py_UNUSED(ignored))
{
    if (ob && !PyCallable_Check(ob)) {
        PyErr_SetString(PyExc_TypeError,
                        "the errcheck attribute must be callable");
        return -1;
    }
    Py_XINCREF(ob);
    Py_XSETREF(self->errcheck, ob);
    return 0;
}

static PyObject *
PyCFuncPtr_get_errcheck(PyCFuncPtrObject *self, void *Py_UNUSED(ignored))
{
    if (self->errcheck) {
        Py_INCREF(self->errcheck);
        return self->errcheck;
    }
    Py_RETURN_NONE;
}

static int
PyCFuncPtr_set_restype(PyCFuncPtrObject *self, PyObject *ob, void *Py_UNUSED(ignored))
{
    _Py_IDENTIFIER(_check_retval_);
    PyObject *checker, *oldchecker;
    if (ob == NULL) {
        oldchecker = self->checker;
        self->checker = NULL;
        Py_CLEAR(self->restype);
        Py_XDECREF(oldchecker);
        return 0;
    }
    if (ob != Py_None && !PyType_stgdict(ob) && !PyCallable_Check(ob)) {
        PyErr_SetString(PyExc_TypeError,
                        "restype must be a type, a callable, or None");
        return -1;
    }
    if (_PyObject_LookupAttrId(ob, &PyId__check_retval_, &checker) < 0) {
        return -1;
    }
    oldchecker = self->checker;
    self->checker = checker;
    Py_INCREF(ob);
    Py_XSETREF(self->restype, ob);
    Py_XDECREF(oldchecker);
    return 0;
}

static PyObject *
PyCFuncPtr_get_restype(PyCFuncPtrObject *self, void *Py_UNUSED(ignored))
{
    StgDictObject *dict;
    if (self->restype) {
        Py_INCREF(self->restype);
        return self->restype;
    }
    dict = PyObject_stgdict((PyObject *)self);
    assert(dict); /* Cannot be NULL for PyCFuncPtrObject instances */
    if (dict->restype) {
        Py_INCREF(dict->restype);
        return dict->restype;
    } else {
        Py_RETURN_NONE;
    }
}

static int
PyCFuncPtr_set_argtypes(PyCFuncPtrObject *self, PyObject *ob, void *Py_UNUSED(ignored))
{
    PyObject *converters;

    if (ob == NULL || ob == Py_None) {
        Py_CLEAR(self->converters);
        Py_CLEAR(self->argtypes);
    } else {
        converters = converters_from_argtypes(ob);
        if (!converters)
            return -1;
        Py_XSETREF(self->converters, converters);
        Py_INCREF(ob);
        Py_XSETREF(self->argtypes, ob);
    }
    return 0;
}

static PyObject *
PyCFuncPtr_get_argtypes(PyCFuncPtrObject *self, void *Py_UNUSED(ignored))
{
    StgDictObject *dict;
    if (self->argtypes) {
        Py_INCREF(self->argtypes);
        return self->argtypes;
    }
    dict = PyObject_stgdict((PyObject *)self);
    assert(dict); /* Cannot be NULL for PyCFuncPtrObject instances */
    if (dict->argtypes) {
        Py_INCREF(dict->argtypes);
        return dict->argtypes;
    } else {
        Py_RETURN_NONE;
    }
}

static PyGetSetDef PyCFuncPtr_getsets[] = {
    { "errcheck", (getter)PyCFuncPtr_get_errcheck, (setter)PyCFuncPtr_set_errcheck,
      "a function to check for errors", NULL },
    { "restype", (getter)PyCFuncPtr_get_restype, (setter)PyCFuncPtr_set_restype,
      "specify the result type", NULL },
    { "argtypes", (getter)PyCFuncPtr_get_argtypes,
      (setter)PyCFuncPtr_set_argtypes,
      "specify the argument types", NULL },
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
    StgDictObject *dict;

    Py_BEGIN_ALLOW_THREADS
    address = (PPROC)GetProcAddress(handle, name);
    Py_END_ALLOW_THREADS
    if (address)
        return address;
    if (((size_t)name & ~0xFFFF) == 0) {
        return NULL;
    }

    dict = PyType_stgdict((PyObject *)type);
    /* It should not happen that dict is NULL, but better be safe */
    if (dict==NULL || dict->flags & FUNCFLAG_CDECL)
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
_check_outarg_type(PyObject *arg, Py_ssize_t index)
{
    StgDictObject *dict;

    if (PyCPointerTypeObject_Check(arg))
        return 1;

    if (PyCArrayTypeObject_Check(arg))
        return 1;

    dict = PyType_stgdict(arg);
    if (dict
        /* simple pointer types, c_void_p, c_wchar_p, BSTR, ... */
        && PyUnicode_Check(dict->proto)
/* We only allow c_void_p, c_char_p and c_wchar_p as a simple output parameter type */
        && (strchr("PzZ", PyUnicode_AsUTF8(dict->proto)[0]))) {
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
_validate_paramflags(PyTypeObject *type, PyObject *paramflags)
{
    Py_ssize_t i, len;
    StgDictObject *dict;
    PyObject *argtypes;

    dict = PyType_stgdict((PyObject *)type);
    if (!dict) {
        PyErr_SetString(PyExc_TypeError,
                        "abstract class");
        return 0;
    }
    argtypes = dict->argtypes;

    if (paramflags == NULL || dict->argtypes == NULL)
        return 1;

    if (!PyTuple_Check(paramflags)) {
        PyErr_SetString(PyExc_TypeError,
                        "paramflags must be a tuple or None");
        return 0;
    }

    len = PyTuple_GET_SIZE(paramflags);
    if (len != PyTuple_GET_SIZE(dict->argtypes)) {
        PyErr_SetString(PyExc_ValueError,
                        "paramflags must have the same length as argtypes");
        return 0;
    }

    for (i = 0; i < len; ++i) {
        PyObject *item = PyTuple_GET_ITEM(paramflags, i);
        int flag;
        char *name;
        PyObject *defval;
        PyObject *typ;
        if (!PyArg_ParseTuple(item, "i|ZO", &flag, &name, &defval)) {
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
            if (!_check_outarg_type(typ, i+1))
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
_get_name(PyObject *obj, const char **pname)
{
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
    address = (PPROC)ctypes_dlsym(handle, name);
    if (!address) {
#ifdef __CYGWIN__
/* dlerror() isn't very helpful on cygwin */
        PyErr_Format(PyExc_AttributeError,
                     "function '%s' not found",
                     name);
#else
        PyErr_SetString(PyExc_AttributeError, ctypes_dlerror());
#endif
        Py_DECREF(ftuple);
        return NULL;
    }
#endif
    if (!_validate_paramflags(type, paramflags)) {
        Py_DECREF(ftuple);
        return NULL;
    }

    self = (PyCFuncPtrObject *)GenericPyCData_new(type, args, kwds);
    if (!self) {
        Py_DECREF(ftuple);
        return NULL;
    }

    Py_XINCREF(paramflags);
    self->paramflags = paramflags;

    *(void **)self->b_ptr = address;
    Py_INCREF(dll);
    Py_DECREF(ftuple);
    if (-1 == KeepRef((CDataObject *)self, 0, dll)) {
        Py_DECREF((PyObject *)self);
        return NULL;
    }

    Py_INCREF(self);
    self->callable = (PyObject *)self;
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

    if (!_validate_paramflags(type, paramflags))
        return NULL;

    self = (PyCFuncPtrObject *)GenericPyCData_new(type, args, kwds);
    self->index = index + 0x1000;
    Py_XINCREF(paramflags);
    self->paramflags = paramflags;
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
    StgDictObject *dict;
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

    dict = PyType_stgdict((PyObject *)type);
    /* XXXX Fails if we do: 'PyCFuncPtr(lambda x: x)' */
    if (!dict || !dict->argtypes) {
        PyErr_SetString(PyExc_TypeError,
               "cannot construct instance of this class:"
            " no argtypes");
        return NULL;
    }

    thunk = _ctypes_alloc_callback(callable,
                                  dict->argtypes,
                                  dict->restype,
                                  dict->flags);
    if (!thunk)
        return NULL;

    self = (PyCFuncPtrObject *)GenericPyCData_new(type, args, kwds);
    if (self == NULL) {
        Py_DECREF(thunk);
        return NULL;
    }

    Py_INCREF(callable);
    self->callable = callable;

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
_byref(PyObject *obj)
{
    PyCArgObject *parg;
    if (!CDataObject_Check(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "expected CData instance");
        return NULL;
    }

    parg = PyCArgObject_new();
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
        Py_INCREF(v);
        return v;
    }
    if (kwds && name) {
        v = PyDict_GetItemWithError(kwds, name);
        if (v) {
            ++*pindex;
            Py_INCREF(v);
            return v;
        }
        else if (PyErr_Occurred()) {
            return NULL;
        }
    }
    if (defval) {
        Py_INCREF(defval);
        return defval;
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
_build_callargs(PyCFuncPtrObject *self, PyObject *argtypes,
                PyObject *inargs, PyObject *kwds,
                int *poutmask, int *pinoutmask, unsigned int *pnumretvals)
{
    PyObject *paramflags = self->paramflags;
    PyObject *callargs;
    StgDictObject *dict;
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
        Py_INCREF(inargs);
        return inargs;
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
            if (defval == NULL)
                defval = _PyLong_Zero;
            Py_INCREF(defval);
            PyTuple_SET_ITEM(callargs, i, defval);
            break;
        case (PARAMFLAG_FIN | PARAMFLAG_FOUT):
            *pinoutmask |= (1 << i); /* mark as inout arg */
            (*pnumretvals)++;
            /* fall through */
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
            dict = PyType_stgdict(ob);
            if (dict == NULL) {
                /* Cannot happen: _validate_paramflags()
                  would not accept such an object */
                PyErr_Format(PyExc_RuntimeError,
                             "NULL stgdict unexpected");
                goto error;
            }
            if (PyUnicode_Check(dict->proto)) {
                PyErr_Format(
                    PyExc_TypeError,
                    "%s 'out' parameter must be passed as default value",
                    ((PyTypeObject *)ob)->tp_name);
                goto error;
            }
            if (PyCArrayTypeObject_Check(ob))
                ob = _PyObject_CallNoArg(ob);
            else
                /* Create an instance of the pointed-to type */
                ob = _PyObject_CallNoArg(dict->proto);
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
            _Py_IDENTIFIER(__ctypes_from_outparam__);

            v = PyTuple_GET_ITEM(callargs, i);
            v = _PyObject_CallMethodIdNoArgs(v, &PyId___ctypes_from_outparam__);
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
PyCFuncPtr_call(PyCFuncPtrObject *self, PyObject *inargs, PyObject *kwds)
{
    PyObject *restype;
    PyObject *converters;
    PyObject *checker;
    PyObject *argtypes;
    StgDictObject *dict = PyObject_stgdict((PyObject *)self);
    PyObject *result;
    PyObject *callargs;
    PyObject *errcheck;
#ifdef MS_WIN32
    IUnknown *piunk = NULL;
#endif
    void *pProc = NULL;

    int inoutmask;
    int outmask;
    unsigned int numretvals;

    assert(dict); /* Cannot be NULL for PyCFuncPtrObject instances */
    restype = self->restype ? self->restype : dict->restype;
    converters = self->converters ? self->converters : dict->converters;
    checker = self->checker ? self->checker : dict->checker;
    argtypes = self->argtypes ? self->argtypes : dict->argtypes;
/* later, we probably want to have an errcheck field in stgdict */
    errcheck = self->errcheck /* ? self->errcheck : dict->errcheck */;


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
        if (!CDataObject_Check(this)) {
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
    callargs = _build_callargs(self, argtypes,
                               inargs, kwds,
                               &outmask, &inoutmask, &numretvals);
    if (callargs == NULL)
        return NULL;

    if (converters) {
        int required = Py_SAFE_DOWNCAST(PyTuple_GET_SIZE(converters),
                                        Py_ssize_t, int);
        int actual = Py_SAFE_DOWNCAST(PyTuple_GET_SIZE(callargs),
                                      Py_ssize_t, int);

        if ((dict->flags & FUNCFLAG_CDECL) == FUNCFLAG_CDECL) {
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

    result = _ctypes_callproc(pProc,
                       callargs,
#ifdef MS_WIN32
                       piunk,
                       self->iid,
#endif
                       dict->flags,
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

static int
PyCFuncPtr_traverse(PyCFuncPtrObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callable);
    Py_VISIT(self->restype);
    Py_VISIT(self->checker);
    Py_VISIT(self->errcheck);
    Py_VISIT(self->argtypes);
    Py_VISIT(self->converters);
    Py_VISIT(self->paramflags);
    Py_VISIT(self->thunk);
    return PyCData_traverse((CDataObject *)self, visit, arg);
}

static int
PyCFuncPtr_clear(PyCFuncPtrObject *self)
{
    Py_CLEAR(self->callable);
    Py_CLEAR(self->restype);
    Py_CLEAR(self->checker);
    Py_CLEAR(self->errcheck);
    Py_CLEAR(self->argtypes);
    Py_CLEAR(self->converters);
    Py_CLEAR(self->paramflags);
    Py_CLEAR(self->thunk);
    return PyCData_clear((CDataObject *)self);
}

static void
PyCFuncPtr_dealloc(PyCFuncPtrObject *self)
{
    PyCFuncPtr_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
PyCFuncPtr_repr(PyCFuncPtrObject *self)
{
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
PyCFuncPtr_bool(PyCFuncPtrObject *self)
{
    return ((*(void **)self->b_ptr != NULL)
#ifdef MS_WIN32
        || (self->index != 0)
#endif
        );
}

static PyNumberMethods PyCFuncPtr_as_number = {
    0, /* nb_add */
    0, /* nb_subtract */
    0, /* nb_multiply */
    0, /* nb_remainder */
    0, /* nb_divmod */
    0, /* nb_power */
    0, /* nb_negative */
    0, /* nb_positive */
    0, /* nb_absolute */
    (inquiry)PyCFuncPtr_bool, /* nb_bool */
};

PyTypeObject PyCFuncPtr_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.PyCFuncPtr",
    sizeof(PyCFuncPtrObject),                           /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)PyCFuncPtr_dealloc,             /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)PyCFuncPtr_repr,                  /* tp_repr */
    &PyCFuncPtr_as_number,                      /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    (ternaryfunc)PyCFuncPtr_call,               /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    &PyCData_as_buffer,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Function Pointer",                         /* tp_doc */
    (traverseproc)PyCFuncPtr_traverse,          /* tp_traverse */
    (inquiry)PyCFuncPtr_clear,                  /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    PyCFuncPtr_getsets,                         /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    PyCFuncPtr_new,                             /* tp_new */
    0,                                          /* tp_free */
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
    StgDictObject *dict;
    PyObject *fields;
    Py_ssize_t i;
    _Py_IDENTIFIER(_fields_);

    if (PyType_stgdict((PyObject *)type->tp_base)) {
        index = _init_pos_args(self, type->tp_base,
                               args, kwds,
                               index);
        if (index == -1)
            return -1;
    }

    dict = PyType_stgdict((PyObject *)type);
    fields = _PyDict_GetItemIdWithError((PyObject *)dict, &PyId__fields_);
    if (fields == NULL) {
        if (PyErr_Occurred()) {
            return -1;
        }
        return index;
    }

    for (i = 0;
         i < dict->length && (i+index) < PyTuple_GET_SIZE(args);
         ++i) {
        PyObject *pair = PySequence_GetItem(fields, i);
        PyObject *name, *val;
        int res;
        if (!pair)
            return -1;
        name = PySequence_GetItem(pair, 0);
        if (!name) {
            Py_DECREF(pair);
            return -1;
        }
        val = PyTuple_GET_ITEM(args, i + index);
        if (kwds) {
            if (PyDict_GetItemWithError(kwds, name)) {
                PyErr_Format(PyExc_TypeError,
                            "duplicate values for field %R",
                            name);
                Py_DECREF(pair);
                Py_DECREF(name);
                return -1;
            }
            else if (PyErr_Occurred()) {
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
    return index + dict->length;
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

static PyTypeObject Struct_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.Structure",
    sizeof(CDataObject),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    &PyCData_as_buffer,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Structure base class",                     /* tp_doc */
    (traverseproc)PyCData_traverse,             /* tp_traverse */
    (inquiry)PyCData_clear,                     /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    Struct_init,                                /* tp_init */
    0,                                          /* tp_alloc */
    GenericPyCData_new,                         /* tp_new */
    0,                                          /* tp_free */
};

static PyTypeObject Union_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.Union",
    sizeof(CDataObject),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    &PyCData_as_buffer,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Union base class",                         /* tp_doc */
    (traverseproc)PyCData_traverse,             /* tp_traverse */
    (inquiry)PyCData_clear,                     /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    Struct_init,                                /* tp_init */
    0,                                          /* tp_alloc */
    GenericPyCData_new,                         /* tp_new */
    0,                                          /* tp_free */
};


/******************************************************************/
/*
  PyCArray_Type
*/
static int
Array_init(CDataObject *self, PyObject *args, PyObject *kw)
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
        if (-1 == PySequence_SetItem((PyObject *)self, i, v))
            return -1;
    }
    return 0;
}

static PyObject *
Array_item(PyObject *myself, Py_ssize_t index)
{
    CDataObject *self = (CDataObject *)myself;
    Py_ssize_t offset, size;
    StgDictObject *stgdict;


    if (index < 0 || index >= self->b_length) {
        PyErr_SetString(PyExc_IndexError,
                        "invalid index");
        return NULL;
    }

    stgdict = PyObject_stgdict((PyObject *)self);
    assert(stgdict); /* Cannot be NULL for array instances */
    /* Would it be clearer if we got the item size from
       stgdict->proto's stgdict?
    */
    size = stgdict->size / stgdict->length;
    offset = index * size;

    return PyCData_get(stgdict->proto, stgdict->getfunc, (PyObject *)self,
                     index, size, self->b_ptr + offset);
}

static PyObject *
Array_subscript(PyObject *myself, PyObject *item)
{
    CDataObject *self = (CDataObject *)myself;

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += self->b_length;
        return Array_item(myself, i);
    }
    else if (PySlice_Check(item)) {
        StgDictObject *stgdict, *itemdict;
        PyObject *proto;
        PyObject *np;
        Py_ssize_t start, stop, step, slicelen, i;
        size_t cur;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelen = PySlice_AdjustIndices(self->b_length, &start, &stop, step);

        stgdict = PyObject_stgdict((PyObject *)self);
        assert(stgdict); /* Cannot be NULL for array object instances */
        proto = stgdict->proto;
        itemdict = PyType_stgdict(proto);
        assert(itemdict); /* proto is the item type of the array, a
                             ctypes type, so this cannot be NULL */

        if (itemdict->getfunc == _ctypes_get_fielddesc("c")->getfunc) {
            char *ptr = (char *)self->b_ptr;
            char *dest;

            if (slicelen <= 0)
                return PyBytes_FromStringAndSize("", 0);
            if (step == 1) {
                return PyBytes_FromStringAndSize(ptr + start,
                                                 slicelen);
            }
            dest = (char *)PyMem_Malloc(slicelen);

            if (dest == NULL)
                return PyErr_NoMemory();

            for (cur = start, i = 0; i < slicelen;
                 cur += step, i++) {
                dest[i] = ptr[cur];
            }

            np = PyBytes_FromStringAndSize(dest, slicelen);
            PyMem_Free(dest);
            return np;
        }
#ifdef CTYPES_UNICODE
        if (itemdict->getfunc == _ctypes_get_fielddesc("u")->getfunc) {
            wchar_t *ptr = (wchar_t *)self->b_ptr;
            wchar_t *dest;

            if (slicelen <= 0)
                return PyUnicode_New(0, 0);
            if (step == 1) {
                return PyUnicode_FromWideChar(ptr + start,
                                              slicelen);
            }

            dest = PyMem_New(wchar_t, slicelen);
            if (dest == NULL) {
                PyErr_NoMemory();
                return NULL;
            }

            for (cur = start, i = 0; i < slicelen;
                 cur += step, i++) {
                dest[i] = ptr[cur];
            }

            np = PyUnicode_FromWideChar(dest, slicelen);
            PyMem_Free(dest);
            return np;
        }
#endif

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
    CDataObject *self = (CDataObject *)myself;
    Py_ssize_t size, offset;
    StgDictObject *stgdict;
    char *ptr;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Array does not support item deletion");
        return -1;
    }

    stgdict = PyObject_stgdict((PyObject *)self);
    assert(stgdict); /* Cannot be NULL for array object instances */
    if (index < 0 || index >= stgdict->length) {
        PyErr_SetString(PyExc_IndexError,
                        "invalid index");
        return -1;
    }
    size = stgdict->size / stgdict->length;
    offset = index * size;
    ptr = self->b_ptr + offset;

    return PyCData_set((PyObject *)self, stgdict->proto, stgdict->setfunc, value,
                     index, size, ptr);
}

static int
Array_ass_subscript(PyObject *myself, PyObject *item, PyObject *value)
{
    CDataObject *self = (CDataObject *)myself;

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
    CDataObject *self = (CDataObject *)myself;
    return self->b_length;
}

static PySequenceMethods Array_as_sequence = {
    Array_length,                               /* sq_length; */
    0,                                          /* sq_concat; */
    0,                                          /* sq_repeat; */
    Array_item,                                 /* sq_item; */
    0,                                          /* sq_slice; */
    Array_ass_item,                             /* sq_ass_item; */
    0,                                          /* sq_ass_slice; */
    0,                                          /* sq_contains; */

    0,                                          /* sq_inplace_concat; */
    0,                                          /* sq_inplace_repeat; */
};

static PyMappingMethods Array_as_mapping = {
    Array_length,
    Array_subscript,
    Array_ass_subscript,
};

PyTypeObject PyCArray_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.Array",
    sizeof(CDataObject),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    &Array_as_sequence,                         /* tp_as_sequence */
    &Array_as_mapping,                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    &PyCData_as_buffer,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "XXX to be provided",                       /* tp_doc */
    (traverseproc)PyCData_traverse,             /* tp_traverse */
    (inquiry)PyCData_clear,                     /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)Array_init,                       /* tp_init */
    0,                                          /* tp_alloc */
    GenericPyCData_new,                         /* tp_new */
    0,                                          /* tp_free */
};

PyObject *
PyCArrayType_from_ctype(PyObject *itemtype, Py_ssize_t length)
{
    static PyObject *cache;
    PyObject *key;
    PyObject *result;
    char name[256];
    PyObject *len;

    if (cache == NULL) {
        cache = PyDict_New();
        if (cache == NULL)
            return NULL;
    }
    len = PyLong_FromSsize_t(length);
    if (len == NULL)
        return NULL;
    key = PyTuple_Pack(2, itemtype, len);
    Py_DECREF(len);
    if (!key)
        return NULL;
    result = PyDict_GetItemProxy(cache, key);
    if (result) {
        Py_INCREF(result);
        Py_DECREF(key);
        return result;
    }
    else if (PyErr_Occurred()) {
        Py_DECREF(key);
        return NULL;
    }

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

    result = PyObject_CallFunction((PyObject *)&PyCArrayType_Type,
                                   "s(O){s:n,s:O}",
                                   name,
                                   &PyCArray_Type,
                                   "_length_",
                                   length,
                                   "_type_",
                                   itemtype
        );
    if (result == NULL) {
        Py_DECREF(key);
        return NULL;
    }
    if (-1 == PyDict_SetItemProxy(cache, key, result)) {
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

static int
Simple_set_value(CDataObject *self, PyObject *value, void *Py_UNUSED(ignored))
{
    PyObject *result;
    StgDictObject *dict = PyObject_stgdict((PyObject *)self);

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "can't delete attribute");
        return -1;
    }
    assert(dict); /* Cannot be NULL for CDataObject instances */
    assert(dict->setfunc);
    result = dict->setfunc(self->b_ptr, value, dict->size);
    if (!result)
        return -1;

    /* consumes the refcount the setfunc returns */
    return KeepRef(self, 0, result);
}

static int
Simple_init(CDataObject *self, PyObject *args, PyObject *kw)
{
    PyObject *value = NULL;
    if (!PyArg_UnpackTuple(args, "__init__", 0, 1, &value))
        return -1;
    if (value)
        return Simple_set_value(self, value, NULL);
    return 0;
}

static PyObject *
Simple_get_value(CDataObject *self, void *Py_UNUSED(ignored))
{
    StgDictObject *dict;
    dict = PyObject_stgdict((PyObject *)self);
    assert(dict); /* Cannot be NULL for CDataObject instances */
    assert(dict->getfunc);
    return dict->getfunc(self->b_ptr, self->b_size);
}

static PyGetSetDef Simple_getsets[] = {
    { "value", (getter)Simple_get_value, (setter)Simple_set_value,
      "current value", NULL },
    { NULL, NULL }
};

static PyObject *
Simple_from_outparm(PyObject *self, PyObject *args)
{
    if (_ctypes_simple_instance((PyObject *)Py_TYPE(self))) {
        Py_INCREF(self);
        return self;
    }
    /* call stgdict->getfunc */
    return Simple_get_value((CDataObject *)self, NULL);
}

static PyMethodDef Simple_methods[] = {
    { "__ctypes_from_outparam__", Simple_from_outparm, METH_NOARGS, },
    { NULL, NULL },
};

static int Simple_bool(CDataObject *self)
{
    return memcmp(self->b_ptr, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", self->b_size);
}

static PyNumberMethods Simple_as_number = {
    0, /* nb_add */
    0, /* nb_subtract */
    0, /* nb_multiply */
    0, /* nb_remainder */
    0, /* nb_divmod */
    0, /* nb_power */
    0, /* nb_negative */
    0, /* nb_positive */
    0, /* nb_absolute */
    (inquiry)Simple_bool, /* nb_bool */
};

/* "%s(%s)" % (self.__class__.__name__, self.value) */
static PyObject *
Simple_repr(CDataObject *self)
{
    PyObject *val, *result;

    if (Py_TYPE(self)->tp_base != &Simple_Type) {
        return PyUnicode_FromFormat("<%s object at %p>",
                                   Py_TYPE(self)->tp_name, self);
    }

    val = Simple_get_value(self, NULL);
    if (val == NULL)
        return NULL;

    result = PyUnicode_FromFormat("%s(%R)",
                                  Py_TYPE(self)->tp_name, val);
    Py_DECREF(val);
    return result;
}

static PyTypeObject Simple_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes._SimpleCData",
    sizeof(CDataObject),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)&Simple_repr,                     /* tp_repr */
    &Simple_as_number,                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    &PyCData_as_buffer,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "XXX to be provided",                       /* tp_doc */
    (traverseproc)PyCData_traverse,             /* tp_traverse */
    (inquiry)PyCData_clear,                     /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    Simple_methods,                             /* tp_methods */
    0,                                          /* tp_members */
    Simple_getsets,                             /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)Simple_init,                      /* tp_init */
    0,                                          /* tp_alloc */
    GenericPyCData_new,                         /* tp_new */
    0,                                          /* tp_free */
};

/******************************************************************/
/*
  PyCPointer_Type
*/
static PyObject *
Pointer_item(PyObject *myself, Py_ssize_t index)
{
    CDataObject *self = (CDataObject *)myself;
    Py_ssize_t size;
    Py_ssize_t offset;
    StgDictObject *stgdict, *itemdict;
    PyObject *proto;

    if (*(void **)self->b_ptr == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "NULL pointer access");
        return NULL;
    }

    stgdict = PyObject_stgdict((PyObject *)self);
    assert(stgdict); /* Cannot be NULL for pointer object instances */

    proto = stgdict->proto;
    assert(proto);
    itemdict = PyType_stgdict(proto);
    assert(itemdict); /* proto is the item type of the pointer, a ctypes
                         type, so this cannot be NULL */

    size = itemdict->size;
    offset = index * itemdict->size;

    return PyCData_get(proto, stgdict->getfunc, (PyObject *)self,
                     index, size, (*(char **)self->b_ptr) + offset);
}

static int
Pointer_ass_item(PyObject *myself, Py_ssize_t index, PyObject *value)
{
    CDataObject *self = (CDataObject *)myself;
    Py_ssize_t size;
    Py_ssize_t offset;
    StgDictObject *stgdict, *itemdict;
    PyObject *proto;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Pointer does not support item deletion");
        return -1;
    }

    if (*(void **)self->b_ptr == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "NULL pointer access");
        return -1;
    }

    stgdict = PyObject_stgdict((PyObject *)self);
    assert(stgdict); /* Cannot be NULL for pointer instances */

    proto = stgdict->proto;
    assert(proto);

    itemdict = PyType_stgdict(proto);
    assert(itemdict); /* Cannot be NULL because the itemtype of a pointer
                         is always a ctypes type */

    size = itemdict->size;
    offset = index * itemdict->size;

    return PyCData_set((PyObject *)self, proto, stgdict->setfunc, value,
                     index, size, (*(char **)self->b_ptr) + offset);
}

static PyObject *
Pointer_get_contents(CDataObject *self, void *closure)
{
    StgDictObject *stgdict;

    if (*(void **)self->b_ptr == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "NULL pointer access");
        return NULL;
    }

    stgdict = PyObject_stgdict((PyObject *)self);
    assert(stgdict); /* Cannot be NULL for pointer instances */
    return PyCData_FromBaseObj(stgdict->proto,
                             (PyObject *)self, 0,
                             *(void **)self->b_ptr);
}

static int
Pointer_set_contents(CDataObject *self, PyObject *value, void *closure)
{
    StgDictObject *stgdict;
    CDataObject *dst;
    PyObject *keep;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Pointer does not support item deletion");
        return -1;
    }
    stgdict = PyObject_stgdict((PyObject *)self);
    assert(stgdict); /* Cannot be NULL for pointer instances */
    assert(stgdict->proto);
    if (!CDataObject_Check(value)) {
        int res = PyObject_IsInstance(value, stgdict->proto);
        if (res == -1)
            return -1;
        if (!res) {
            PyErr_Format(PyExc_TypeError,
                         "expected %s instead of %s",
                         ((PyTypeObject *)(stgdict->proto))->tp_name,
                         Py_TYPE(value)->tp_name);
            return -1;
        }
    }

    dst = (CDataObject *)value;
    *(void **)self->b_ptr = dst->b_ptr;

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
    { "contents", (getter)Pointer_get_contents,
      (setter)Pointer_set_contents,
      "the object this pointer points to (read-write)", NULL },
    { NULL, NULL }
};

static int
Pointer_init(CDataObject *self, PyObject *args, PyObject *kw)
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
    StgDictObject *dict = PyType_stgdict((PyObject *)type);
    if (!dict || !dict->proto) {
        PyErr_SetString(PyExc_TypeError,
                        "Cannot create instance: has no _type_");
        return NULL;
    }
    return GenericPyCData_new(type, args, kw);
}

static PyObject *
Pointer_subscript(PyObject *myself, PyObject *item)
{
    CDataObject *self = (CDataObject *)myself;
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
        StgDictObject *stgdict, *itemdict;
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

        stgdict = PyObject_stgdict((PyObject *)self);
        assert(stgdict); /* Cannot be NULL for pointer instances */
        proto = stgdict->proto;
        assert(proto);
        itemdict = PyType_stgdict(proto);
        assert(itemdict);
        if (itemdict->getfunc == _ctypes_get_fielddesc("c")->getfunc) {
            char *ptr = *(char **)self->b_ptr;
            char *dest;

            if (len <= 0)
                return PyBytes_FromStringAndSize("", 0);
            if (step == 1) {
                return PyBytes_FromStringAndSize(ptr + start,
                                                 len);
            }
            dest = (char *)PyMem_Malloc(len);
            if (dest == NULL)
                return PyErr_NoMemory();
            for (cur = start, i = 0; i < len; cur += step, i++) {
                dest[i] = ptr[cur];
            }
            np = PyBytes_FromStringAndSize(dest, len);
            PyMem_Free(dest);
            return np;
        }
#ifdef CTYPES_UNICODE
        if (itemdict->getfunc == _ctypes_get_fielddesc("u")->getfunc) {
            wchar_t *ptr = *(wchar_t **)self->b_ptr;
            wchar_t *dest;

            if (len <= 0)
                return PyUnicode_New(0, 0);
            if (step == 1) {
                return PyUnicode_FromWideChar(ptr + start,
                                              len);
            }
            dest = PyMem_New(wchar_t, len);
            if (dest == NULL)
                return PyErr_NoMemory();
            for (cur = start, i = 0; i < len; cur += step, i++) {
                dest[i] = ptr[cur];
            }
            np = PyUnicode_FromWideChar(dest, len);
            PyMem_Free(dest);
            return np;
        }
#endif

        np = PyList_New(len);
        if (np == NULL)
            return NULL;

        for (cur = start, i = 0; i < len; cur += step, i++) {
            PyObject *v = Pointer_item(myself, cur);
            PyList_SET_ITEM(np, i, v);
        }
        return np;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "Pointer indices must be integer");
        return NULL;
    }
}

static PySequenceMethods Pointer_as_sequence = {
    0,                                          /* inquiry sq_length; */
    0,                                          /* binaryfunc sq_concat; */
    0,                                          /* intargfunc sq_repeat; */
    Pointer_item,                               /* intargfunc sq_item; */
    0,                                          /* intintargfunc sq_slice; */
    Pointer_ass_item,                           /* intobjargproc sq_ass_item; */
    0,                                          /* intintobjargproc sq_ass_slice; */
    0,                                          /* objobjproc sq_contains; */
    /* Added in release 2.0 */
    0,                                          /* binaryfunc sq_inplace_concat; */
    0,                                          /* intargfunc sq_inplace_repeat; */
};

static PyMappingMethods Pointer_as_mapping = {
    0,
    Pointer_subscript,
};

static int
Pointer_bool(CDataObject *self)
{
    return (*(void **)self->b_ptr != NULL);
}

static PyNumberMethods Pointer_as_number = {
    0, /* nb_add */
    0, /* nb_subtract */
    0, /* nb_multiply */
    0, /* nb_remainder */
    0, /* nb_divmod */
    0, /* nb_power */
    0, /* nb_negative */
    0, /* nb_positive */
    0, /* nb_absolute */
    (inquiry)Pointer_bool, /* nb_bool */
};

PyTypeObject PyCPointer_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes._Pointer",
    sizeof(CDataObject),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    &Pointer_as_number,                         /* tp_as_number */
    &Pointer_as_sequence,                       /* tp_as_sequence */
    &Pointer_as_mapping,                        /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    &PyCData_as_buffer,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "XXX to be provided",                       /* tp_doc */
    (traverseproc)PyCData_traverse,             /* tp_traverse */
    (inquiry)PyCData_clear,                     /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    Pointer_getsets,                            /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)Pointer_init,                     /* tp_init */
    0,                                          /* tp_alloc */
    Pointer_new,                                /* tp_new */
    0,                                          /* tp_free */
};


/******************************************************************/
/*
 *  Module initialization.
 */

static const char module_docs[] =
"Create and manipulate C compatible data types in Python.";

#ifdef MS_WIN32

static const char comerror_doc[] = "Raised when a COM method call failed.";

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

static PyTypeObject PyComError_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.COMError",         /* tp_name */
    sizeof(PyBaseExceptionObject), /* tp_basicsize */
    0,                          /* tp_itemsize */
    0,                          /* tp_dealloc */
    0,                          /* tp_vectorcall_offset */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_as_async */
    0,                          /* tp_repr */
    0,                          /* tp_as_number */
    0,                          /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash */
    0,                          /* tp_call */
    0,                          /* tp_str */
    0,                          /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    PyDoc_STR(comerror_doc),    /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    0,                          /* tp_methods */
    0,                          /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    (initproc)comerror_init,    /* tp_init */
    0,                          /* tp_alloc */
    0,                          /* tp_new */
};


static int
create_comerror(void)
{
    PyComError_Type.tp_base = (PyTypeObject*)PyExc_Exception;
    if (PyType_Ready(&PyComError_Type) < 0)
        return -1;
    Py_INCREF(&PyComError_Type);
    ComError = (PyObject*)&PyComError_Type;
    return 0;
}

#endif

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
cast_check_pointertype(PyObject *arg)
{
    StgDictObject *dict;

    if (PyCPointerTypeObject_Check(arg))
        return 1;
    if (PyCFuncPtrTypeObject_Check(arg))
        return 1;
    dict = PyType_stgdict(arg);
    if (dict != NULL && dict->proto != NULL) {
        if (PyUnicode_Check(dict->proto)
            && (strchr("sPzUZXO", PyUnicode_AsUTF8(dict->proto)[0]))) {
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
    CDataObject *result;
    if (0 == cast_check_pointertype(ctype))
        return NULL;
    result = (CDataObject *)_PyObject_CallNoArg(ctype);
    if (result == NULL)
        return NULL;

    /*
      The casted objects '_objects' member:

      It must certainly contain the source objects one.
      It must contain the source object itself.
     */
    if (CDataObject_Check(src)) {
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
        Py_XINCREF(obj->b_objects);
        result->b_objects = obj->b_objects;
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
    memcpy(result->b_ptr, &ptr, sizeof(void *));
    return (PyObject *)result;

  failed:
    Py_DECREF(result);
    return NULL;
}

#ifdef CTYPES_UNICODE
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
#endif


static struct PyModuleDef _ctypesmodule = {
    PyModuleDef_HEAD_INIT,
    "_ctypes",
    module_docs,
    -1,
    _ctypes_module_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__ctypes(void)
{
    PyObject *m;

/* Note:
   ob_type is the metatype (the 'type'), defaults to PyType_Type,
   tp_base is the base type, defaults to 'object' aka PyBaseObject_Type.
*/
    m = PyModule_Create(&_ctypesmodule);
    if (!m)
        return NULL;

    _ctypes_ptrtype_cache = PyDict_New();
    if (_ctypes_ptrtype_cache == NULL)
        return NULL;

    PyModule_AddObject(m, "_pointer_type_cache", (PyObject *)_ctypes_ptrtype_cache);

    _unpickle = PyObject_GetAttrString(m, "_unpickle");
    if (_unpickle == NULL)
        return NULL;

    if (PyType_Ready(&PyCArg_Type) < 0)
        return NULL;

    if (PyType_Ready(&PyCThunk_Type) < 0)
        return NULL;

    /* StgDict is derived from PyDict_Type */
    PyCStgDict_Type.tp_base = &PyDict_Type;
    if (PyType_Ready(&PyCStgDict_Type) < 0)
        return NULL;

    /*************************************************
     *
     * Metaclasses
     */

    PyCStructType_Type.tp_base = &PyType_Type;
    if (PyType_Ready(&PyCStructType_Type) < 0)
        return NULL;

    UnionType_Type.tp_base = &PyType_Type;
    if (PyType_Ready(&UnionType_Type) < 0)
        return NULL;

    PyCPointerType_Type.tp_base = &PyType_Type;
    if (PyType_Ready(&PyCPointerType_Type) < 0)
        return NULL;

    PyCArrayType_Type.tp_base = &PyType_Type;
    if (PyType_Ready(&PyCArrayType_Type) < 0)
        return NULL;

    PyCSimpleType_Type.tp_base = &PyType_Type;
    if (PyType_Ready(&PyCSimpleType_Type) < 0)
        return NULL;

    PyCFuncPtrType_Type.tp_base = &PyType_Type;
    if (PyType_Ready(&PyCFuncPtrType_Type) < 0)
        return NULL;

    /*************************************************
     *
     * Classes using a custom metaclass
     */

    if (PyType_Ready(&PyCData_Type) < 0)
        return NULL;

    Py_SET_TYPE(&Struct_Type, &PyCStructType_Type);
    Struct_Type.tp_base = &PyCData_Type;
    if (PyType_Ready(&Struct_Type) < 0)
        return NULL;
    Py_INCREF(&Struct_Type);
    PyModule_AddObject(m, "Structure", (PyObject *)&Struct_Type);

    Py_SET_TYPE(&Union_Type, &UnionType_Type);
    Union_Type.tp_base = &PyCData_Type;
    if (PyType_Ready(&Union_Type) < 0)
        return NULL;
    Py_INCREF(&Union_Type);
    PyModule_AddObject(m, "Union", (PyObject *)&Union_Type);

    Py_SET_TYPE(&PyCPointer_Type, &PyCPointerType_Type);
    PyCPointer_Type.tp_base = &PyCData_Type;
    if (PyType_Ready(&PyCPointer_Type) < 0)
        return NULL;
    Py_INCREF(&PyCPointer_Type);
    PyModule_AddObject(m, "_Pointer", (PyObject *)&PyCPointer_Type);

    Py_SET_TYPE(&PyCArray_Type, &PyCArrayType_Type);
    PyCArray_Type.tp_base = &PyCData_Type;
    if (PyType_Ready(&PyCArray_Type) < 0)
        return NULL;
    Py_INCREF(&PyCArray_Type);
    PyModule_AddObject(m, "Array", (PyObject *)&PyCArray_Type);

    Py_SET_TYPE(&Simple_Type, &PyCSimpleType_Type);
    Simple_Type.tp_base = &PyCData_Type;
    if (PyType_Ready(&Simple_Type) < 0)
        return NULL;
    Py_INCREF(&Simple_Type);
    PyModule_AddObject(m, "_SimpleCData", (PyObject *)&Simple_Type);

    Py_SET_TYPE(&PyCFuncPtr_Type, &PyCFuncPtrType_Type);
    PyCFuncPtr_Type.tp_base = &PyCData_Type;
    if (PyType_Ready(&PyCFuncPtr_Type) < 0)
        return NULL;
    Py_INCREF(&PyCFuncPtr_Type);
    PyModule_AddObject(m, "CFuncPtr", (PyObject *)&PyCFuncPtr_Type);

    /*************************************************
     *
     * Simple classes
     */

    /* PyCField_Type is derived from PyBaseObject_Type */
    if (PyType_Ready(&PyCField_Type) < 0)
        return NULL;

    /*************************************************
     *
     * Other stuff
     */

    DictRemover_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&DictRemover_Type) < 0)
        return NULL;

    if (PyType_Ready(&StructParam_Type) < 0) {
        return NULL;
    }

#ifdef MS_WIN32
    if (create_comerror() < 0)
        return NULL;
    PyModule_AddObject(m, "COMError", ComError);

    PyModule_AddObject(m, "FUNCFLAG_HRESULT", PyLong_FromLong(FUNCFLAG_HRESULT));
    PyModule_AddObject(m, "FUNCFLAG_STDCALL", PyLong_FromLong(FUNCFLAG_STDCALL));
#endif
    PyModule_AddObject(m, "FUNCFLAG_CDECL", PyLong_FromLong(FUNCFLAG_CDECL));
    PyModule_AddObject(m, "FUNCFLAG_USE_ERRNO", PyLong_FromLong(FUNCFLAG_USE_ERRNO));
    PyModule_AddObject(m, "FUNCFLAG_USE_LASTERROR", PyLong_FromLong(FUNCFLAG_USE_LASTERROR));
    PyModule_AddObject(m, "FUNCFLAG_PYTHONAPI", PyLong_FromLong(FUNCFLAG_PYTHONAPI));
    PyModule_AddStringConstant(m, "__version__", "1.1.0");

    PyModule_AddObject(m, "_memmove_addr", PyLong_FromVoidPtr(memmove));
    PyModule_AddObject(m, "_memset_addr", PyLong_FromVoidPtr(memset));
    PyModule_AddObject(m, "_string_at_addr", PyLong_FromVoidPtr(string_at));
    PyModule_AddObject(m, "_cast_addr", PyLong_FromVoidPtr(cast));
#ifdef CTYPES_UNICODE
    PyModule_AddObject(m, "_wstring_at_addr", PyLong_FromVoidPtr(wstring_at));
#endif

/* If RTLD_LOCAL is not defined (Windows!), set it to zero. */
#if !HAVE_DECL_RTLD_LOCAL
#define RTLD_LOCAL 0
#endif

/* If RTLD_GLOBAL is not defined (cygwin), set it to the same value as
   RTLD_LOCAL.
*/
#if !HAVE_DECL_RTLD_GLOBAL
#define RTLD_GLOBAL RTLD_LOCAL
#endif

    PyModule_AddObject(m, "RTLD_LOCAL", PyLong_FromLong(RTLD_LOCAL));
    PyModule_AddObject(m, "RTLD_GLOBAL", PyLong_FromLong(RTLD_GLOBAL));

    PyExc_ArgError = PyErr_NewException("ctypes.ArgumentError", NULL, NULL);
    if (PyExc_ArgError) {
        Py_INCREF(PyExc_ArgError);
        PyModule_AddObject(m, "ArgumentError", PyExc_ArgError);
    }
    return m;
}

/*
 Local Variables:
 compile-command: "cd .. && python setup.py -q build -g && python setup.py -q build install --home ~"
 End:
*/
