#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
// windows.h must be included before pycore internal headers
#ifdef MS_WIN32
#  include <windows.h>
#endif

#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include <ffi.h>
#ifdef MS_WIN32
#  include <malloc.h>
#endif
#include "ctypes.h"

/******************************************************************/
/*
  StdDict - a dictionary subclass, containing additional C accessible fields

  XXX blabla more
*/

/* Seems we need this, otherwise we get problems when calling
 * PyDict_SetItem() (ma_lookup is NULL)
 */
static int
PyCStgDict_init(StgDictObject *self, PyObject *args, PyObject *kwds)
{
    if (PyDict_Type.tp_init((PyObject *)self, args, kwds) < 0)
        return -1;
    self->format = NULL;
    self->ndim = 0;
    self->shape = NULL;
    return 0;
}

static int
PyCStgDict_clear(StgDictObject *self)
{
    Py_CLEAR(self->proto);
    Py_CLEAR(self->argtypes);
    Py_CLEAR(self->converters);
    Py_CLEAR(self->restype);
    Py_CLEAR(self->checker);
    return 0;
}

static void
PyCStgDict_dealloc(StgDictObject *self)
{
    PyCStgDict_clear(self);
    PyMem_Free(self->format);
    PyMem_Free(self->shape);
    PyMem_Free(self->ffi_type_pointer.elements);
    PyDict_Type.tp_dealloc((PyObject *)self);
}

static PyObject *
PyCStgDict_sizeof(StgDictObject *self, void *unused)
{
    Py_ssize_t res;

    res = _PyDict_SizeOf((PyDictObject *)self);
    res += sizeof(StgDictObject) - sizeof(PyDictObject);
    if (self->format)
        res += strlen(self->format) + 1;
    res += self->ndim * sizeof(Py_ssize_t);
    if (self->ffi_type_pointer.elements)
        res += (self->length + 1) * sizeof(ffi_type *);
    return PyLong_FromSsize_t(res);
}

int
PyCStgDict_clone(StgDictObject *dst, StgDictObject *src)
{
    char *d, *s;
    Py_ssize_t size;

    PyCStgDict_clear(dst);
    PyMem_Free(dst->ffi_type_pointer.elements);
    PyMem_Free(dst->format);
    dst->format = NULL;
    PyMem_Free(dst->shape);
    dst->shape = NULL;
    dst->ffi_type_pointer.elements = NULL;

    d = (char *)dst;
    s = (char *)src;
    memcpy(d + sizeof(PyDictObject),
           s + sizeof(PyDictObject),
           sizeof(StgDictObject) - sizeof(PyDictObject));

    Py_XINCREF(dst->proto);
    Py_XINCREF(dst->argtypes);
    Py_XINCREF(dst->converters);
    Py_XINCREF(dst->restype);
    Py_XINCREF(dst->checker);

    if (src->format) {
        dst->format = PyMem_Malloc(strlen(src->format) + 1);
        if (dst->format == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        strcpy(dst->format, src->format);
    }
    if (src->shape) {
        dst->shape = PyMem_Malloc(sizeof(Py_ssize_t) * src->ndim);
        if (dst->shape == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        memcpy(dst->shape, src->shape,
               sizeof(Py_ssize_t) * src->ndim);
    }

    if (src->ffi_type_pointer.elements == NULL)
        return 0;
    size = sizeof(ffi_type *) * (src->length + 1);
    dst->ffi_type_pointer.elements = PyMem_Malloc(size);
    if (dst->ffi_type_pointer.elements == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    memcpy(dst->ffi_type_pointer.elements,
           src->ffi_type_pointer.elements,
           size);
    return 0;
}

static struct PyMethodDef PyCStgDict_methods[] = {
    {"__sizeof__", (PyCFunction)PyCStgDict_sizeof, METH_NOARGS},
    {NULL, NULL}                /* sentinel */
};

PyTypeObject PyCStgDict_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "StgDict",
    sizeof(StgDictObject),
    0,
    (destructor)PyCStgDict_dealloc,             /* tp_dealloc */
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
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    PyCStgDict_methods,                         /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)PyCStgDict_init,                          /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
    0,                                          /* tp_free */
};

/* May return NULL, but does not set an exception! */
StgDictObject *
PyType_stgdict(PyObject *obj)
{
    PyTypeObject *type;

    if (!PyType_Check(obj))
        return NULL;
    type = (PyTypeObject *)obj;
    if (!type->tp_dict || !PyCStgDict_CheckExact(type->tp_dict))
        return NULL;
    return (StgDictObject *)type->tp_dict;
}

/* May return NULL, but does not set an exception! */
/*
  This function should be as fast as possible, so we don't call PyType_stgdict
  above but inline the code, and avoid the PyType_Check().
*/
StgDictObject *
PyObject_stgdict(PyObject *self)
{
    PyTypeObject *type = Py_TYPE(self);
    if (!type->tp_dict || !PyCStgDict_CheckExact(type->tp_dict))
        return NULL;
    return (StgDictObject *)type->tp_dict;
}

/* descr is the descriptor for a field marked as anonymous.  Get all the
 _fields_ descriptors from descr->proto, create new descriptors with offset
 and index adjusted, and stuff them into type.
 */
static int
MakeFields(PyObject *type, CFieldObject *descr,
           Py_ssize_t index, Py_ssize_t offset)
{
    Py_ssize_t i;
    PyObject *fields;
    PyObject *fieldlist;

    fields = PyObject_GetAttrString(descr->proto, "_fields_");
    if (fields == NULL)
        return -1;
    fieldlist = PySequence_Fast(fields, "_fields_ must be a sequence");
    Py_DECREF(fields);
    if (fieldlist == NULL)
        return -1;

    ctypes_state *st = GLOBAL_STATE();
    PyTypeObject *cfield_tp = st->PyCField_Type;
    for (i = 0; i < PySequence_Fast_GET_SIZE(fieldlist); ++i) {
        PyObject *pair = PySequence_Fast_GET_ITEM(fieldlist, i); /* borrowed */
        PyObject *fname, *ftype, *bits;
        CFieldObject *fdescr;
        CFieldObject *new_descr;
        /* Convert to PyArg_UnpackTuple... */
        if (!PyArg_ParseTuple(pair, "OO|O", &fname, &ftype, &bits)) {
            Py_DECREF(fieldlist);
            return -1;
        }
        fdescr = (CFieldObject *)PyObject_GetAttr(descr->proto, fname);
        if (fdescr == NULL) {
            Py_DECREF(fieldlist);
            return -1;
        }
        if (!Py_IS_TYPE(fdescr, cfield_tp)) {
            PyErr_SetString(PyExc_TypeError, "unexpected type");
            Py_DECREF(fdescr);
            Py_DECREF(fieldlist);
            return -1;
        }
        if (fdescr->anonymous) {
            int rc = MakeFields(type, fdescr,
                                index + fdescr->index,
                                offset + fdescr->offset);
            Py_DECREF(fdescr);
            if (rc == -1) {
                Py_DECREF(fieldlist);
                return -1;
            }
            continue;
        }
        new_descr = (CFieldObject *)cfield_tp->tp_alloc(cfield_tp, 0);
        if (new_descr == NULL) {
            Py_DECREF(fdescr);
            Py_DECREF(fieldlist);
            return -1;
        }
        assert(Py_IS_TYPE(new_descr, cfield_tp));
        new_descr->size = fdescr->size;
        new_descr->offset = fdescr->offset + offset;
        new_descr->index = fdescr->index + index;
        new_descr->proto = Py_XNewRef(fdescr->proto);
        new_descr->getfunc = fdescr->getfunc;
        new_descr->setfunc = fdescr->setfunc;

        Py_DECREF(fdescr);

        if (-1 == PyObject_SetAttr(type, fname, (PyObject *)new_descr)) {
            Py_DECREF(fieldlist);
            Py_DECREF(new_descr);
            return -1;
        }
        Py_DECREF(new_descr);
    }
    Py_DECREF(fieldlist);
    return 0;
}

/* Iterate over the names in the type's _anonymous_ attribute, if present,
 */
static int
MakeAnonFields(PyObject *type)
{
    PyObject *anon;
    PyObject *anon_names;
    Py_ssize_t i;

    if (_PyObject_LookupAttr(type, &_Py_ID(_anonymous_), &anon) < 0) {
        return -1;
    }
    if (anon == NULL) {
        return 0;
    }
    anon_names = PySequence_Fast(anon, "_anonymous_ must be a sequence");
    Py_DECREF(anon);
    if (anon_names == NULL)
        return -1;

    ctypes_state *st = GLOBAL_STATE();
    PyTypeObject *cfield_tp = st->PyCField_Type;
    for (i = 0; i < PySequence_Fast_GET_SIZE(anon_names); ++i) {
        PyObject *fname = PySequence_Fast_GET_ITEM(anon_names, i); /* borrowed */
        CFieldObject *descr = (CFieldObject *)PyObject_GetAttr(type, fname);
        if (descr == NULL) {
            Py_DECREF(anon_names);
            return -1;
        }
        if (!Py_IS_TYPE(descr, cfield_tp)) {
            PyErr_Format(PyExc_AttributeError,
                         "'%U' is specified in _anonymous_ but not in "
                         "_fields_",
                         fname);
            Py_DECREF(anon_names);
            Py_DECREF(descr);
            return -1;
        }
        descr->anonymous = 1;

        /* descr is in the field descriptor. */
        if (-1 == MakeFields(type, (CFieldObject *)descr,
                             ((CFieldObject *)descr)->index,
                             ((CFieldObject *)descr)->offset)) {
            Py_DECREF(descr);
            Py_DECREF(anon_names);
            return -1;
        }
        Py_DECREF(descr);
    }

    Py_DECREF(anon_names);
    return 0;
}

/*
  Allocate a memory block for a pep3118 format string, copy prefix (if
  non-null) into it and append `{padding}x` to the end.
  Returns NULL on failure, with the error indicator set.
*/
char *
_ctypes_alloc_format_padding(const char *prefix, Py_ssize_t padding)
{
    /* int64 decimal characters + x + null */
    char buf[19 + 1 + 1];

    assert(padding > 0);

    if (padding == 1) {
        /* Use x instead of 1x, for brevity */
        return _ctypes_alloc_format_string(prefix, "x");
    }

    int ret = PyOS_snprintf(buf, sizeof(buf), "%zdx", padding); (void)ret;
    assert(0 <= ret && ret < (Py_ssize_t)sizeof(buf));
    return _ctypes_alloc_format_string(prefix, buf);
}

/*
  Retrieve the (optional) _pack_ attribute from a type, the _fields_ attribute,
  and create an StgDictObject.  Used for Structure and Union subclasses.
*/
int
PyCStructUnionType_update_stgdict(PyObject *type, PyObject *fields, int isStruct)
{
    StgDictObject *stgdict, *basedict;
    Py_ssize_t len, offset, size, align, i;
    Py_ssize_t union_size, total_align, aligned_size;
    Py_ssize_t field_size = 0;
    int bitofs;
    PyObject *tmp;
    int pack;
    Py_ssize_t ffi_ofs;
    int big_endian;
    int arrays_seen = 0;

    if (fields == NULL)
        return 0;

    if (_PyObject_LookupAttr(type, &_Py_ID(_swappedbytes_), &tmp) < 0) {
        return -1;
    }
    if (tmp) {
        Py_DECREF(tmp);
        big_endian = !PY_BIG_ENDIAN;
    }
    else {
        big_endian = PY_BIG_ENDIAN;
    }

    if (_PyObject_LookupAttr(type, &_Py_ID(_pack_), &tmp) < 0) {
        return -1;
    }
    if (tmp) {
        pack = _PyLong_AsInt(tmp);
        Py_DECREF(tmp);
        if (pack < 0) {
            if (!PyErr_Occurred() ||
                PyErr_ExceptionMatches(PyExc_TypeError) ||
                PyErr_ExceptionMatches(PyExc_OverflowError))
            {
                PyErr_SetString(PyExc_ValueError,
                                "_pack_ must be a non-negative integer");
            }
            return -1;
        }
    }
    else {
        /* Setting `_pack_ = 0` amounts to using the default alignment */
        pack = 0;
    }

    len = PySequence_Size(fields);
    if (len == -1) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_SetString(PyExc_TypeError,
                            "'_fields_' must be a sequence of pairs");
        }
        return -1;
    }

    stgdict = PyType_stgdict(type);
    if (!stgdict) {
        PyErr_SetString(PyExc_TypeError,
                        "ctypes state is not initialized");
        return -1;
    }
    /* If this structure/union is already marked final we cannot assign
       _fields_ anymore. */

    if (stgdict->flags & DICTFLAG_FINAL) {/* is final ? */
        PyErr_SetString(PyExc_AttributeError,
                        "_fields_ is final");
        return -1;
    }

    if (stgdict->format) {
        PyMem_Free(stgdict->format);
        stgdict->format = NULL;
    }

    if (stgdict->ffi_type_pointer.elements)
        PyMem_Free(stgdict->ffi_type_pointer.elements);

    basedict = PyType_stgdict((PyObject *)((PyTypeObject *)type)->tp_base);
    if (basedict) {
        stgdict->flags |= (basedict->flags &
                           (TYPEFLAG_HASUNION | TYPEFLAG_HASBITFIELD));
    }
    if (!isStruct) {
        stgdict->flags |= TYPEFLAG_HASUNION;
    }
    if (basedict) {
        size = offset = basedict->size;
        align = basedict->align;
        union_size = 0;
        total_align = align ? align : 1;
        stgdict->ffi_type_pointer.type = FFI_TYPE_STRUCT;
        stgdict->ffi_type_pointer.elements = PyMem_New(ffi_type *, basedict->length + len + 1);
        if (stgdict->ffi_type_pointer.elements == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        memset(stgdict->ffi_type_pointer.elements, 0,
               sizeof(ffi_type *) * (basedict->length + len + 1));
        if (basedict->length > 0) {
            memcpy(stgdict->ffi_type_pointer.elements,
                   basedict->ffi_type_pointer.elements,
                   sizeof(ffi_type *) * (basedict->length));
        }
        ffi_ofs = basedict->length;
    } else {
        offset = 0;
        size = 0;
        align = 0;
        union_size = 0;
        total_align = 1;
        stgdict->ffi_type_pointer.type = FFI_TYPE_STRUCT;
        stgdict->ffi_type_pointer.elements = PyMem_New(ffi_type *, len + 1);
        if (stgdict->ffi_type_pointer.elements == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        memset(stgdict->ffi_type_pointer.elements, 0,
               sizeof(ffi_type *) * (len + 1));
        ffi_ofs = 0;
    }

    assert(stgdict->format == NULL);
    if (isStruct) {
        stgdict->format = _ctypes_alloc_format_string(NULL, "T{");
    } else {
        /* PEP3118 doesn't support union. Use 'B' for bytes. */
        stgdict->format = _ctypes_alloc_format_string(NULL, "B");
    }
    if (stgdict->format == NULL)
        return -1;

    for (i = 0; i < len; ++i) {
        PyObject *name = NULL, *desc = NULL;
        PyObject *pair = PySequence_GetItem(fields, i);
        PyObject *prop;
        StgDictObject *dict;
        int bitsize = 0;

        if (!pair || !PyArg_ParseTuple(pair, "UO|i", &name, &desc, &bitsize)) {
            PyErr_SetString(PyExc_TypeError,
                            "'_fields_' must be a sequence of (name, C type) pairs");
            Py_XDECREF(pair);
            return -1;
        }
        if (PyCArrayTypeObject_Check(desc))
            arrays_seen = 1;
        dict = PyType_stgdict(desc);
        if (dict == NULL) {
            Py_DECREF(pair);
            PyErr_Format(PyExc_TypeError,
                         "second item in _fields_ tuple (index %zd) must be a C type",
                         i);
            return -1;
        }
        stgdict->ffi_type_pointer.elements[ffi_ofs + i] = &dict->ffi_type_pointer;
        if (dict->flags & (TYPEFLAG_ISPOINTER | TYPEFLAG_HASPOINTER))
            stgdict->flags |= TYPEFLAG_HASPOINTER;
        stgdict->flags |= dict->flags & (TYPEFLAG_HASUNION | TYPEFLAG_HASBITFIELD);
        dict->flags |= DICTFLAG_FINAL; /* mark field type final */
        if (PyTuple_Size(pair) == 3) { /* bits specified */
            stgdict->flags |= TYPEFLAG_HASBITFIELD;
            switch(dict->ffi_type_pointer.type) {
            case FFI_TYPE_UINT8:
            case FFI_TYPE_UINT16:
            case FFI_TYPE_UINT32:
            case FFI_TYPE_SINT64:
            case FFI_TYPE_UINT64:
                break;

            case FFI_TYPE_SINT8:
            case FFI_TYPE_SINT16:
            case FFI_TYPE_SINT32:
                if (dict->getfunc != _ctypes_get_fielddesc("c")->getfunc
                    && dict->getfunc != _ctypes_get_fielddesc("u")->getfunc
                    )
                    break;
                /* else fall through */
            default:
                PyErr_Format(PyExc_TypeError,
                             "bit fields not allowed for type %s",
                             ((PyTypeObject *)desc)->tp_name);
                Py_DECREF(pair);
                return -1;
            }
            if (bitsize <= 0 || bitsize > dict->size * 8) {
                PyErr_SetString(PyExc_ValueError,
                                "number of bits invalid for bit field");
                Py_DECREF(pair);
                return -1;
            }
        } else
            bitsize = 0;

        if (isStruct) {
            const char *fieldfmt = dict->format ? dict->format : "B";
            const char *fieldname = PyUnicode_AsUTF8(name);
            char *ptr;
            Py_ssize_t len;
            char *buf;
            Py_ssize_t last_size = size;
            Py_ssize_t padding;

            if (fieldname == NULL)
            {
                Py_DECREF(pair);
                return -1;
            }

            /* construct the field now, as `prop->offset` is `offset` with
               corrected alignment */
            prop = PyCField_FromDesc(desc, i,
                                   &field_size, bitsize, &bitofs,
                                   &size, &offset, &align,
                                   pack, big_endian);
            if (prop == NULL) {
                Py_DECREF(pair);
                return -1;
            }

            /* number of bytes between the end of the last field and the start
               of this one */
            padding = ((CFieldObject *)prop)->offset - last_size;

            if (padding > 0) {
                ptr = stgdict->format;
                stgdict->format = _ctypes_alloc_format_padding(ptr, padding);
                PyMem_Free(ptr);
                if (stgdict->format == NULL) {
                    Py_DECREF(pair);
                    Py_DECREF(prop);
                    return -1;
                }
            }

            len = strlen(fieldname) + strlen(fieldfmt);

            buf = PyMem_Malloc(len + 2 + 1);
            if (buf == NULL) {
                Py_DECREF(pair);
                Py_DECREF(prop);
                PyErr_NoMemory();
                return -1;
            }
            sprintf(buf, "%s:%s:", fieldfmt, fieldname);

            ptr = stgdict->format;
            if (dict->shape != NULL) {
                stgdict->format = _ctypes_alloc_format_string_with_shape(
                    dict->ndim, dict->shape, stgdict->format, buf);
            } else {
                stgdict->format = _ctypes_alloc_format_string(stgdict->format, buf);
            }
            PyMem_Free(ptr);
            PyMem_Free(buf);

            if (stgdict->format == NULL) {
                Py_DECREF(pair);
                Py_DECREF(prop);
                return -1;
            }
        } else /* union */ {
            size = 0;
            offset = 0;
            align = 0;
            prop = PyCField_FromDesc(desc, i,
                                   &field_size, bitsize, &bitofs,
                                   &size, &offset, &align,
                                   pack, big_endian);
            if (prop == NULL) {
                Py_DECREF(pair);
                return -1;
            }
            union_size = max(size, union_size);
        }
        total_align = max(align, total_align);

        if (-1 == PyObject_SetAttr(type, name, prop)) {
            Py_DECREF(prop);
            Py_DECREF(pair);
            return -1;
        }
        Py_DECREF(pair);
        Py_DECREF(prop);
    }

    if (!isStruct) {
        size = union_size;
    }

    /* Adjust the size according to the alignment requirements */
    aligned_size = ((size + total_align - 1) / total_align) * total_align;

    if (isStruct) {
        char *ptr;
        Py_ssize_t padding;

        /* Pad up to the full size of the struct */
        padding = aligned_size - size;
        if (padding > 0) {
            ptr = stgdict->format;
            stgdict->format = _ctypes_alloc_format_padding(ptr, padding);
            PyMem_Free(ptr);
            if (stgdict->format == NULL) {
                return -1;
            }
        }

        ptr = stgdict->format;
        stgdict->format = _ctypes_alloc_format_string(stgdict->format, "}");
        PyMem_Free(ptr);
        if (stgdict->format == NULL)
            return -1;
    }

    stgdict->ffi_type_pointer.alignment = Py_SAFE_DOWNCAST(total_align,
                                                           Py_ssize_t,
                                                           unsigned short);
    stgdict->ffi_type_pointer.size = aligned_size;

    stgdict->size = aligned_size;
    stgdict->align = total_align;
    stgdict->length = ffi_ofs + len;

/*
 * The value of MAX_STRUCT_SIZE depends on the platform Python is running on.
 */
#if defined(__aarch64__) || defined(__arm__) || defined(_M_ARM64)
#  define MAX_STRUCT_SIZE 32
#elif defined(__powerpc64__)
#  define MAX_STRUCT_SIZE 64
#else
#  define MAX_STRUCT_SIZE 16
#endif

    if (arrays_seen && (size <= MAX_STRUCT_SIZE)) {
        /*
         * See bpo-22273 and gh-110190. Arrays are normally treated as
         * pointers, which is fine when an array name is being passed as
         * parameter, but not when passing structures by value that contain
         * arrays.
         * Small structures passed by value are passed in registers, and in
         * order to do this, libffi needs to know the true type of the array
         * members of structs. Treating them as pointers breaks things.
         *
         * Small structures have different sizes depending on the platform
         * where Python is running on:
         *
         *      * x86-64: 16 bytes or less
         *      * Arm platforms (both 32 and 64 bit): 32 bytes or less
         *      * PowerPC 64 Little Endian: 64 bytes or less
         *
         * In that case, there can't be more than 16, 32 or 64 elements after
         * unrolling arrays, as we (will) disallow bitfields.
         * So we can collect the true ffi_type values in a fixed-size local
         * array on the stack and, if any arrays were seen, replace the
         * ffi_type_pointer.elements with a more accurate set, to allow
         * libffi to marshal them into registers correctly.
         * It means one more loop over the fields, but if we got here,
         * the structure is small, so there aren't too many of those.
         *
         * Although the passing in registers is specific to the above
         * platforms, the array-in-struct vs. pointer problem is general.
         * But we restrict the type transformation to small structs
         * nonetheless.
         *
         * Note that although a union may be small in terms of memory usage, it
         * could contain many overlapping declarations of arrays, e.g.
         *
         * union {
         *     unsigned int_8 foo [16];
         *     unsigned uint_8 bar [16];
         *     unsigned int_16 baz[8];
         *     unsigned uint_16 bozz[8];
         *     unsigned int_32 fizz[4];
         *     unsigned uint_32 buzz[4];
         * }
         *
         * which is still only 16 bytes in size. We need to convert this into
         * the following equivalent for libffi:
         *
         * union {
         *     struct { int_8 e1; int_8 e2; ... int_8 e_16; } f1;
         *     struct { uint_8 e1; uint_8 e2; ... uint_8 e_16; } f2;
         *     struct { int_16 e1; int_16 e2; ... int_16 e_8; } f3;
         *     struct { uint_16 e1; uint_16 e2; ... uint_16 e_8; } f4;
         *     struct { int_32 e1; int_32 e2; ... int_32 e_4; } f5;
         *     struct { uint_32 e1; uint_32 e2; ... uint_32 e_4; } f6;
         * }
         *
         * The same principle applies for a struct 32 or 64 bytes in size.
         *
         * So the struct/union needs setting up as follows: all non-array
         * elements copied across as is, and all array elements replaced with
         * an equivalent struct which has as many fields as the array has
         * elements, plus one NULL pointer.
         */

        Py_ssize_t num_ffi_type_pointers = 0;  /* for the dummy fields */
        Py_ssize_t num_ffi_types = 0;  /* for the dummy structures */
        size_t alloc_size;  /* total bytes to allocate */
        void *type_block;  /* to hold all the type information needed */
        ffi_type **element_types;  /* of this struct/union */
        ffi_type **dummy_types;  /* of the dummy struct elements */
        ffi_type *structs;  /* point to struct aliases of arrays */
        Py_ssize_t element_index;  /* index into element_types for this */
        Py_ssize_t dummy_index = 0; /* index into dummy field pointers */
        Py_ssize_t struct_index = 0; /* index into dummy structs */

        /* first pass to see how much memory to allocate */
        for (i = 0; i < len; ++i) {
            PyObject *name, *desc;
            PyObject *pair = PySequence_GetItem(fields, i);
            StgDictObject *dict;
            int bitsize = 0;

            if (pair == NULL) {
                return -1;
            }
            if (!PyArg_ParseTuple(pair, "UO|i", &name, &desc, &bitsize)) {
                PyErr_SetString(PyExc_TypeError,
                    "'_fields_' must be a sequence of (name, C type) pairs");
                Py_DECREF(pair);
                return -1;
            }
            dict = PyType_stgdict(desc);
            if (dict == NULL) {
                Py_DECREF(pair);
                PyErr_Format(PyExc_TypeError,
                    "second item in _fields_ tuple (index %zd) must be a C type",
                    i);
                return -1;
            }
            if (!PyCArrayTypeObject_Check(desc)) {
                /* Not an array. Just need an ffi_type pointer. */
                num_ffi_type_pointers++;
            }
            else {
                /* It's an array. */
                Py_ssize_t length = dict->length;
                StgDictObject *edict;

                edict = PyType_stgdict(dict->proto);
                if (edict == NULL) {
                    Py_DECREF(pair);
                    PyErr_Format(PyExc_TypeError,
                        "second item in _fields_ tuple (index %zd) must be a C type",
                        i);
                    return -1;
                }
                /*
                 * We need one extra ffi_type to hold the struct, and one
                 * ffi_type pointer per array element + one for a NULL to
                 * mark the end.
                 */
                num_ffi_types++;
                num_ffi_type_pointers += length + 1;
            }
            Py_DECREF(pair);
        }

        /*
         * At this point, we know we need storage for some ffi_types and some
         * ffi_type pointers. We'll allocate these in one block.
         * There are three sub-blocks of information: the ffi_type pointers to
         * this structure/union's elements, the ffi_type_pointers to the
         * dummy fields standing in for array elements, and the
         * ffi_types representing the dummy structures.
         */
        alloc_size = (ffi_ofs + 1 + len + num_ffi_type_pointers) * sizeof(ffi_type *) +
                        num_ffi_types * sizeof(ffi_type);
        type_block = PyMem_Malloc(alloc_size);

        if (type_block == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        /*
         * the first block takes up ffi_ofs + len + 1 which is the pointers *
         * for this struct/union. The second block takes up
         * num_ffi_type_pointers, so the sum of these is ffi_ofs + len + 1 +
         * num_ffi_type_pointers as allocated above. The last bit is the
         * num_ffi_types structs.
         */
        element_types = (ffi_type **) type_block;
        dummy_types = &element_types[ffi_ofs + len + 1];
        structs = (ffi_type *) &dummy_types[num_ffi_type_pointers];

        if (num_ffi_types > 0) {
            memset(structs, 0, num_ffi_types * sizeof(ffi_type));
        }
        if (ffi_ofs && (basedict != NULL)) {
            memcpy(element_types,
                basedict->ffi_type_pointer.elements,
                ffi_ofs * sizeof(ffi_type *));
        }
        element_index = ffi_ofs;

        /* second pass to actually set the type pointers */
        for (i = 0; i < len; ++i) {
            PyObject *name, *desc;
            PyObject *pair = PySequence_GetItem(fields, i);
            StgDictObject *dict;
            int bitsize = 0;

            if (pair == NULL) {
                PyMem_Free(type_block);
                return -1;
            }
            /* In theory, we made this call in the first pass, so it *shouldn't*
             * fail. However, you never know, and the code above might change
             * later - keeping the check in here is a tad defensive but it
             * will affect program size only slightly and performance hardly at
             * all.
             */
            if (!PyArg_ParseTuple(pair, "UO|i", &name, &desc, &bitsize)) {
                PyErr_SetString(PyExc_TypeError,
                                "'_fields_' must be a sequence of (name, C type) pairs");
                Py_DECREF(pair);
                PyMem_Free(type_block);
                return -1;
            }
            dict = PyType_stgdict(desc);
            /* Possibly this check could be avoided, but see above comment. */
            if (dict == NULL) {
                Py_DECREF(pair);
                PyMem_Free(type_block);
                PyErr_Format(PyExc_TypeError,
                             "second item in _fields_ tuple (index %zd) must be a C type",
                             i);
                return -1;
            }
            assert(element_index < (ffi_ofs + len)); /* will be used below */
            if (!PyCArrayTypeObject_Check(desc)) {
                /* Not an array. Just copy over the element ffi_type. */
                element_types[element_index++] = &dict->ffi_type_pointer;
            }
            else {
                Py_ssize_t length = dict->length;
                StgDictObject *edict;

                edict = PyType_stgdict(dict->proto);
                if (edict == NULL) {
                    Py_DECREF(pair);
                    PyMem_Free(type_block);
                    PyErr_Format(PyExc_TypeError,
                                 "second item in _fields_ tuple (index %zd) must be a C type",
                                 i);
                    return -1;
                }
                element_types[element_index++] = &structs[struct_index];
                structs[struct_index].size = length * edict->ffi_type_pointer.size;
                structs[struct_index].alignment = edict->ffi_type_pointer.alignment;
                structs[struct_index].type = FFI_TYPE_STRUCT;
                structs[struct_index].elements = &dummy_types[dummy_index];
                ++struct_index;
                /* Copy over the element's type, length times. */
                while (length > 0) {
                    assert(dummy_index < (num_ffi_type_pointers));
                    dummy_types[dummy_index++] = &edict->ffi_type_pointer;
                    length--;
                }
                assert(dummy_index < (num_ffi_type_pointers));
                dummy_types[dummy_index++] = NULL;
            }
            Py_DECREF(pair);
        }

        element_types[element_index] = NULL;
        /*
         * Replace the old elements with the new, taking into account
         * base class elements where necessary.
         */
        assert(stgdict->ffi_type_pointer.elements);
        PyMem_Free(stgdict->ffi_type_pointer.elements);
        stgdict->ffi_type_pointer.elements = element_types;
    }

    /* We did check that this flag was NOT set above, it must not
       have been set until now. */
    if (stgdict->flags & DICTFLAG_FINAL) {
        PyErr_SetString(PyExc_AttributeError,
                        "Structure or union cannot contain itself");
        return -1;
    }
    stgdict->flags |= DICTFLAG_FINAL;

    return MakeAnonFields(type);
}
