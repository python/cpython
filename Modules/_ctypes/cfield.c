#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
// windows.h must be included before pycore internal headers
#ifdef MS_WIN32
#  include <windows.h>
#endif

#include "pycore_bitutils.h"      // _Py_bswap32()
#include "pycore_call.h"          // _PyObject_CallNoArgs()

#include <ffi.h>
#include "ctypes.h"

#define CTYPES_CFIELD_CAPSULE_NAME_PYMEM "_ctypes/cfield.c pymem"

/*[clinic input]
module _ctypes
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=476a19c49b31a75c]*/

#include "clinic/cfield.c.h"

static void pymem_destructor(PyObject *ptr)
{
    void *p = PyCapsule_GetPointer(ptr, CTYPES_CFIELD_CAPSULE_NAME_PYMEM);
    if (p) {
        PyMem_Free(p);
    }
}


/******************************************************************/
/*
  PyCField_Type
*/
/*[clinic input]
class _ctypes.CField "PyObject *" "PyObject"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=602817ea3ffc709c]*/

static inline
Py_ssize_t NUM_BITS(Py_ssize_t bitsize);
static inline
Py_ssize_t LOW_BIT(Py_ssize_t offset);


/*[clinic input]
@classmethod
_ctypes.CField.__new__ as PyCField_new

    *
    name: object(subclass_of='&PyUnicode_Type')
    type as proto: object
    byte_size: Py_ssize_t
    byte_offset: Py_ssize_t
    index: Py_ssize_t
    _internal_use: bool
    bit_size as bit_size_obj: object = None
    bit_offset as bit_offset_obj: object = None

[clinic start generated code]*/

static PyObject *
PyCField_new_impl(PyTypeObject *type, PyObject *name, PyObject *proto,
                  Py_ssize_t byte_size, Py_ssize_t byte_offset,
                  Py_ssize_t index, int _internal_use,
                  PyObject *bit_size_obj, PyObject *bit_offset_obj)
/*[clinic end generated code: output=3f2885ee4108b6e2 input=b343436e33c0d782]*/
{
    CFieldObject* self = NULL;

    if (!_internal_use) {
        // Do not instantiate outside ctypes, yet.
        // The constructor is internal API and may change without warning.
        PyErr_Format(PyExc_TypeError, "cannot create %T object", type);
        goto error;
    }
    if (byte_size < 0) {
        PyErr_Format(PyExc_ValueError,
                     "byte size of field %R must not be negative, got %zd",
                     name, byte_size);
        goto error;
    }

    ctypes_state *st = get_module_state_by_class(type);
    StgInfo *info;
    if (PyStgInfo_FromType(st, proto, &info) < 0) {
        goto error;
    }
    if (info == NULL) {
        PyErr_Format(PyExc_TypeError,
                     "type of field %R must be a C type", name);
        goto error;
    }
    if (byte_size != info->size) {
        PyErr_Format(PyExc_ValueError,
                     "byte size of field %R (%zd) does not match type size (%zd)",
                     name, byte_size, info->size);
        goto error;
    }

    Py_ssize_t bitfield_size = 0;
    Py_ssize_t bit_offset = 0;
    if (bit_size_obj != Py_None) {
        // It's a bit field!
        switch(info->ffi_type_pointer.type) {
            case FFI_TYPE_UINT8:
            case FFI_TYPE_UINT16:
            case FFI_TYPE_UINT32:
            case FFI_TYPE_SINT64:
            case FFI_TYPE_UINT64:
                break;

            case FFI_TYPE_SINT8:
            case FFI_TYPE_SINT16:
            case FFI_TYPE_SINT32:
                if (info->getfunc != _ctypes_get_fielddesc("c")->getfunc
                    && info->getfunc != _ctypes_get_fielddesc("u")->getfunc)
                {
                    break;
                }
                _Py_FALLTHROUGH;  /* else fall through */
            default:
                PyErr_Format(PyExc_TypeError,
                             "bit fields not allowed for type %s",
                             ((PyTypeObject*)proto)->tp_name);
                goto error;
        }

        if (byte_size > 100) {
            // Bitfields must "live" in a field defined by a ffi type,
            // so they're limited to about 8 bytes.
            // This check is here to avoid overflow in later checks.
            PyErr_Format(PyExc_ValueError,
                         "bit field %R size too large, got %zd",
                         name, byte_size);
            goto error;
        }
        bitfield_size = PyLong_AsSsize_t(bit_size_obj);
        if ((bitfield_size <= 0) || (bitfield_size > 255)) {
            if (!PyErr_Occurred()) {
                PyErr_Format(PyExc_ValueError,
                             "bit size of field %R out of range, got %zd",
                             name, bitfield_size);
            }
            goto error;
        }
        bit_offset = PyLong_AsSsize_t(bit_offset_obj);
        if ((bit_offset < 0) || (bit_offset > 255)) {
            if (!PyErr_Occurred()) {
                PyErr_Format(PyExc_ValueError,
                             "bit offset of field %R out of range, got %zd",
                             name, bit_offset);
            }
            goto error;
        }
        if ((bitfield_size + bit_offset) > byte_size * 8) {
            PyErr_Format(
                PyExc_ValueError,
                "bit field %R overflows its type (%zd + %zd >= %zd)",
                name, bit_offset, byte_size*8);
            goto error;
        }
    }
    else {
        if (bit_offset_obj != Py_None) {
            PyErr_Format(
                PyExc_ValueError,
                "field %R: bit_offset must be specified if bit_size is",
                name);
            goto error;
        }
    }

    self = _CFieldObject_CAST(type->tp_alloc(type, 0));
    if (!self) {
        return NULL;
    }
    self->name = PyUnicode_FromObject(name);
    if (!self->name) {
        goto error;
    }
    assert(PyUnicode_CheckExact(self->name));

    self->proto = Py_NewRef(proto);
    self->byte_size = byte_size;
    self->byte_offset = byte_offset;
    self->bitfield_size = (uint8_t)bitfield_size;
    self->bit_offset = (uint8_t)bit_offset;

    self->index = index;

    /*  Field descriptors for 'c_char * n' are be special cased to
        return a Python string instead of an Array object instance...
    */
    self->setfunc = NULL;
    self->getfunc = NULL;
    if (PyCArrayTypeObject_Check(st, proto)) {
        StgInfo *ainfo;
        if (PyStgInfo_FromType(st, proto, &ainfo) < 0) {
            goto error;
        }

        if (ainfo && ainfo->proto) {
            StgInfo *iinfo;
            if (PyStgInfo_FromType(st, ainfo->proto, &iinfo) < 0) {
                goto error;
            }
            if (!iinfo) {
                PyErr_SetString(PyExc_TypeError,
                                "has no _stginfo_");
                goto error;
            }
            if (iinfo->getfunc == _ctypes_get_fielddesc("c")->getfunc) {
                struct fielddesc *fd = _ctypes_get_fielddesc("s");
                self->getfunc = fd->getfunc;
                self->setfunc = fd->setfunc;
            }
            if (iinfo->getfunc == _ctypes_get_fielddesc("u")->getfunc) {
                struct fielddesc *fd = _ctypes_get_fielddesc("U");
                self->getfunc = fd->getfunc;
                self->setfunc = fd->setfunc;
            }
        }
    }

    return (PyObject *)self;
error:
    Py_XDECREF(self);
    return NULL;
}

static inline Py_ssize_t
_pack_legacy_size(CFieldObject *field)
{
    if (field->bitfield_size) {
        Py_ssize_t bit_offset = field->bit_offset;
        return (field->bitfield_size << 16) | bit_offset;
    }
    return field->byte_size;
}

static int
PyCField_set_lock_held(PyObject *op, PyObject *inst, PyObject *value)
{
    CDataObject *dst;
    char *ptr;
    CFieldObject *self = _CFieldObject_CAST(op);
    ctypes_state *st = get_module_state_by_class(Py_TYPE(self));
    if (!CDataObject_Check(st, inst)) {
        PyErr_SetString(PyExc_TypeError,
                        "not a ctype instance");
        return -1;
    }
    dst = _CDataObject_CAST(inst);
    ptr = dst->b_ptr + self->byte_offset;
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "can't delete attribute");
        return -1;
    }
    return PyCData_set(st, inst, self->proto, self->setfunc, value,
                       self->index, _pack_legacy_size(self), ptr);
}

static int
PyCField_set(PyObject *op, PyObject *inst, PyObject *value)
{
    int res;
    Py_BEGIN_CRITICAL_SECTION(inst);
    res = PyCField_set_lock_held(op, inst, value);
    Py_END_CRITICAL_SECTION();
    return res;
}

static PyObject *
PyCField_get(PyObject *op, PyObject *inst, PyObject *type)
{
    CDataObject *src;
    CFieldObject *self = _CFieldObject_CAST(op);
    if (inst == NULL) {
        return Py_NewRef(self);
    }
    ctypes_state *st = get_module_state_by_class(Py_TYPE(self));
    if (!CDataObject_Check(st, inst)) {
        PyErr_SetString(PyExc_TypeError,
                        "not a ctype instance");
        return NULL;
    }
    src = _CDataObject_CAST(inst);
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(inst);
    res = PyCData_get(st, self->proto, self->getfunc, inst,
                       self->index, _pack_legacy_size(self),
                       src->b_ptr + self->byte_offset);
    Py_END_CRITICAL_SECTION();
    return res;
}

static PyObject *
PyCField_get_legacy_size(PyObject *self, void *Py_UNUSED(closure))
{
    CFieldObject *field = _CFieldObject_CAST(self);
    return PyLong_FromSsize_t(_pack_legacy_size(field));
}

static PyObject *
PyCField_get_bit_size(PyObject *self, void *Py_UNUSED(closure))
{
    CFieldObject *field = _CFieldObject_CAST(self);
    if (field->bitfield_size) {
        return PyLong_FromSsize_t(field->bitfield_size);
    }
    if (field->byte_size < PY_SSIZE_T_MAX / 8) {
        return PyLong_FromSsize_t(field->byte_size * 8);
    }

    // If the bit size overflows Py_ssize_t, we don't try fitting it in
    // a bigger C type. Use Python ints.
    PyObject *byte_size_obj = NULL;
    PyObject *eight = NULL;
    PyObject *result = NULL;

    byte_size_obj = PyLong_FromSsize_t(field->byte_size);
    if (!byte_size_obj) {
        goto finally;
    }
    eight = PyLong_FromLong(8);
    if (!eight) {
        goto finally;
    }
    result = PyNumber_Multiply(byte_size_obj, eight);
finally:
    Py_XDECREF(byte_size_obj);
    Py_XDECREF(eight);
    return result;
}

static PyObject *
PyCField_is_bitfield(PyObject *self, void *Py_UNUSED(closure))
{
    return PyBool_FromLong(_CFieldObject_CAST(self)->bitfield_size);
}

static PyObject *
PyCField_is_anonymous(PyObject *self, void *Py_UNUSED(closure))
{
    return PyBool_FromLong(_CFieldObject_CAST(self)->anonymous);
}

static PyGetSetDef PyCField_getset[] = {
    { "size", PyCField_get_legacy_size, NULL,
        PyDoc_STR("size in bytes of this field. For bitfields, this is a "
                  "legacy packed value; use byte_size instead") },

    { "bit_size", PyCField_get_bit_size, NULL,
        PyDoc_STR("size of this field in bits") },
    { "is_bitfield", PyCField_is_bitfield, NULL,
        PyDoc_STR("true if this is a bitfield") },
    { "is_anonymous", PyCField_is_anonymous, NULL,
        PyDoc_STR("true if this field is anonymous") },
    { NULL },
};

static PyMemberDef PyCField_members[] = {
    { "name",
        .type = Py_T_OBJECT_EX,
        .offset = offsetof(CFieldObject, name),
        .flags = Py_READONLY,
        .doc = PyDoc_STR("name of this field") },
    { "type",
        .type = Py_T_OBJECT_EX,
        .offset = offsetof(CFieldObject, proto),
        .flags = Py_READONLY,
        .doc = PyDoc_STR("type of this field") },
    { "offset",
        .type = Py_T_PYSSIZET,
        .offset = offsetof(CFieldObject, byte_offset),
        .flags = Py_READONLY,
        .doc = PyDoc_STR(
            "offset in bytes of this field (same as byte_offset)") },
    { "byte_offset",
        .type = Py_T_PYSSIZET,
        .offset = offsetof(CFieldObject, byte_offset),
        .flags = Py_READONLY,
        .doc = PyDoc_STR("offset in bytes of this field. "
                         "For bitfields: excludes bit_offset.") },
    { "byte_size",
        .type = Py_T_PYSSIZET,
        .offset = offsetof(CFieldObject, byte_size),
        .flags = Py_READONLY,
        .doc = PyDoc_STR("size of this field in bytes") },
    { "bit_offset",
        .type = Py_T_UBYTE,
        .offset = offsetof(CFieldObject, bit_offset),
        .flags = Py_READONLY,
        .doc = PyDoc_STR("additional offset in bits (relative to byte_offset);"
                         " zero for non-bitfields") },
    { NULL },
};

static int
PyCField_traverse(PyObject *op, visitproc visit, void *arg)
{
    CFieldObject *self = _CFieldObject_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->proto);
    return 0;
}

static int
PyCField_clear(PyObject *op)
{
    CFieldObject *self = _CFieldObject_CAST(op);
    Py_CLEAR(self->proto);
    Py_CLEAR(self->name);
    return 0;
}

static void
PyCField_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    (void)PyCField_clear(self);
    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
PyCField_repr(PyObject *self)
{
    CFieldObject *field = _CFieldObject_CAST(self);
    PyObject *result;
    const char *tp_name = ((PyTypeObject *)field->proto)->tp_name;

    if (field->bitfield_size) {
        result = PyUnicode_FromFormat(
            "<%T %R type=%s, ofs=%zd, bit_size=%zd, bit_offset=%zd>",
            self,
            field->name, tp_name, field->byte_offset,
            (Py_ssize_t)field->bitfield_size,
            (Py_ssize_t)field->bit_offset);
    }
    else {
        result = PyUnicode_FromFormat(
            "<%T %R type=%s, ofs=%zd, size=%zd>",
            self,
            field->name, tp_name, field->byte_offset,
            field->byte_size);
    }
    return result;
}

static PyType_Slot cfield_slots[] = {
    {Py_tp_new, PyCField_new},
    {Py_tp_dealloc, PyCField_dealloc},
    {Py_tp_repr, PyCField_repr},
    {Py_tp_doc, (void *)PyDoc_STR("Structure/Union member")},
    {Py_tp_traverse, PyCField_traverse},
    {Py_tp_clear, PyCField_clear},
    {Py_tp_getset, PyCField_getset},
    {Py_tp_members, PyCField_members},
    {Py_tp_descr_get, PyCField_get},
    {Py_tp_descr_set, PyCField_set},
    {0, NULL},
};

PyType_Spec cfield_spec = {
    .name = "ctypes.CField",
    .basicsize = sizeof(CFieldObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = cfield_slots,
};


/*****************************************************************
 * Integer fields, with bitfield support
 */

/* how to decode the size field, for integer get/set functions */
static inline
Py_ssize_t LOW_BIT(Py_ssize_t offset) {
    return offset & 0xFFFF;
}
static inline
Py_ssize_t NUM_BITS(Py_ssize_t bitsize) {
    return bitsize >> 16;
}

/* Doesn't work if NUM_BITS(size) == 0, but it never happens in SET() call. */
#define BIT_MASK(type, size) (((((type)1 << (NUM_BITS(size) - 1)) - 1) << 1) + 1)

/* This macro CHANGES the first parameter IN PLACE. For proper sign handling,
   we must first shift left, then right.
*/
#define GET_BITFIELD(v, size)                                           \
    if (NUM_BITS(size)) {                                               \
        v <<= (sizeof(v)*8 - LOW_BIT(size) - NUM_BITS(size));           \
        v >>= (sizeof(v)*8 - NUM_BITS(size));                           \
    }

/* This macro RETURNS the first parameter with the bit field CHANGED. */
#define SET(type, x, v, size)                                                 \
    (NUM_BITS(size) ?                                                   \
     ( ( (type)(x) & ~(BIT_MASK(type, size) << LOW_BIT(size)) ) | ( ((type)(v) & BIT_MASK(type, size)) << LOW_BIT(size) ) ) \
     : (type)(v))

/*****************************************************************
 * The setter methods return an object which must be kept alive, to keep the
 * data valid which has been stored in the memory block.  The ctypes object
 * instance inserts this object into its 'b_objects' list.
 *
 * For simple Python types like integers or characters, there is nothing that
 * has to been kept alive, so Py_None is returned in these cases.  But this
 * makes inspecting the 'b_objects' list, which is accessible from Python for
 * debugging, less useful.
 *
 * So, defining the _CTYPES_DEBUG_KEEP symbol returns the original value
 * instead of Py_None.
 */

#ifdef _CTYPES_DEBUG_KEEP
#define _RET(x) Py_INCREF(x); return x
#else
#define _RET(X) Py_RETURN_NONE
#endif

/*****************************************************************
 * accessor methods for fixed-width integers (e.g. int8_t, uint64_t),
 * supporting bit fields.
 * These are named e.g. `i8_set`/`i8_get` or `u64_set`/`u64_get`,
 * and are all alike, so they're defined using a macro.
 */

#define FIXINT_GETSET(TAG, CTYPE, NBITS, PYAPI_FROMFUNC)                      \
    static PyObject *                                                         \
    TAG ## _set(void *ptr, PyObject *value, Py_ssize_t size_arg)              \
    {                                                                         \
        assert(NUM_BITS(size_arg) || (size_arg == (NBITS) / 8));              \
        CTYPE val;                                                            \
        if (PyLong_Check(value)                                               \
            && PyUnstable_Long_IsCompact((PyLongObject *)value))              \
        {                                                                     \
            val = (CTYPE)PyUnstable_Long_CompactValue(                        \
                      (PyLongObject *)value);                                 \
        }                                                                     \
        else {                                                                \
            Py_ssize_t res = PyLong_AsNativeBytes(                            \
                value, &val, (NBITS) / 8,                                     \
                Py_ASNATIVEBYTES_NATIVE_ENDIAN                                \
                | Py_ASNATIVEBYTES_ALLOW_INDEX);                              \
            if (res < 0) {                                                    \
                return NULL;                                                  \
            }                                                                 \
        }                                                                     \
        CTYPE prev;                                                           \
        memcpy(&prev, ptr, (NBITS) / 8);                                      \
        val = SET(CTYPE, prev, val, size_arg);                                \
        memcpy(ptr, &val, (NBITS) / 8);                                       \
        _RET(value);                                                          \
    }                                                                         \
                                                                              \
    static PyObject *                                                         \
    TAG ## _get(void *ptr, Py_ssize_t size_arg)                               \
    {                                                                         \
        assert(NUM_BITS(size_arg) || (size_arg == (NBITS) / 8));              \
        CTYPE val;                                                            \
        memcpy(&val, ptr, sizeof(val));                                       \
        GET_BITFIELD(val, size_arg);                                          \
        return PYAPI_FROMFUNC(val);                                           \
    }                                                                         \
    ///////////////////////////////////////////////////////////////////////////

/* Another macro for byte-swapped variants (e.g. `i8_set_sw`/`i8_get_sw`) */

#define FIXINT_GETSET_SW(TAG, CTYPE, NBITS, PYAPI_FROMFUNC, PY_SWAPFUNC)      \
    static PyObject *                                                         \
    TAG ## _set_sw(void *ptr, PyObject *value, Py_ssize_t size_arg)           \
    {                                                                         \
        CTYPE val;                                                            \
        PyObject *res = TAG ## _set(&val, value, (NBITS) / 8);                \
        if (res == NULL) {                                                    \
            return NULL;                                                      \
        }                                                                     \
        Py_DECREF(res);                                                       \
        CTYPE field;                                                          \
        memcpy(&field, ptr, sizeof(field));                                   \
        field = PY_SWAPFUNC(field);                                           \
        field = SET(CTYPE, field, val, size_arg);                             \
        field = PY_SWAPFUNC(field);                                           \
        memcpy(ptr, &field, sizeof(field));                                   \
        _RET(value);                                                          \
    }                                                                         \
                                                                              \
    static PyObject *                                                         \
    TAG ## _get_sw(void *ptr, Py_ssize_t size_arg)                            \
    {                                                                         \
        assert(NUM_BITS(size_arg) || (size_arg == (NBITS) / 8));              \
        CTYPE val;                                                            \
        memcpy(&val, ptr, sizeof(val));                                       \
        val = PY_SWAPFUNC(val);                                               \
        GET_BITFIELD(val, size_arg);                                          \
        return PYAPI_FROMFUNC(val);                                           \
    }                                                                         \
    ///////////////////////////////////////////////////////////////////////////

/* These macros are expanded for all supported combinations of byte sizes
 * (1, 2, 4, 8), signed and unsigned, native and swapped byteorder.
 * That's a lot, so generate the list with Argument Clinic (`make clinic`).
 */

/*[python input]
for nbits in 8, 16, 32, 64:
    for sgn in 'i', 'u':
        u = 'u' if sgn == 'u' else ''
        U = u.upper()
        apibits = max(nbits, 32)
        parts = [
            f'{sgn}{nbits}',
            f'{u}int{nbits}_t',
            f'{nbits}',
            f'PyLong_From{U}Int{apibits}',
        ]
        print(f'FIXINT_GETSET({", ".join(parts)})')
        if nbits > 8:
            parts.append(f'_Py_bswap{nbits}')
            print(f'FIXINT_GETSET_SW({", ".join(parts)})')
[python start generated code]*/
FIXINT_GETSET(i8, int8_t, 8, PyLong_FromInt32)
FIXINT_GETSET(u8, uint8_t, 8, PyLong_FromUInt32)
FIXINT_GETSET(i16, int16_t, 16, PyLong_FromInt32)
FIXINT_GETSET_SW(i16, int16_t, 16, PyLong_FromInt32, _Py_bswap16)
FIXINT_GETSET(u16, uint16_t, 16, PyLong_FromUInt32)
FIXINT_GETSET_SW(u16, uint16_t, 16, PyLong_FromUInt32, _Py_bswap16)
FIXINT_GETSET(i32, int32_t, 32, PyLong_FromInt32)
FIXINT_GETSET_SW(i32, int32_t, 32, PyLong_FromInt32, _Py_bswap32)
FIXINT_GETSET(u32, uint32_t, 32, PyLong_FromUInt32)
FIXINT_GETSET_SW(u32, uint32_t, 32, PyLong_FromUInt32, _Py_bswap32)
FIXINT_GETSET(i64, int64_t, 64, PyLong_FromInt64)
FIXINT_GETSET_SW(i64, int64_t, 64, PyLong_FromInt64, _Py_bswap64)
FIXINT_GETSET(u64, uint64_t, 64, PyLong_FromUInt64)
FIXINT_GETSET_SW(u64, uint64_t, 64, PyLong_FromUInt64, _Py_bswap64)
/*[python end generated code: output=3d60c96fa58e07d5 input=0b7e166f2ea18e70]*/

// For one-byte types, swapped variants are the same as native
#define i8_set_sw i8_set
#define i8_get_sw i8_get
#define u8_set_sw u8_set
#define u8_get_sw u8_get

#undef FIXINT_GETSET
#undef FIXINT_GETSET_SW

/*****************************************************************
 * non-integer accessor methods, not supporting bit fields
 */

#ifndef MS_WIN32
/* http://msdn.microsoft.com/en-us/library/cc237864.aspx */
#define VARIANT_FALSE 0x0000
#define VARIANT_TRUE 0xFFFF
#endif
/* v: short BOOL - VARIANT_BOOL */
static PyObject *
v_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(short int)));
    switch (PyObject_IsTrue(value)) {
    case -1:
        return NULL;
    case 0:
        *(short int *)ptr = VARIANT_FALSE;
        _RET(value);
    default:
        *(short int *)ptr = VARIANT_TRUE;
        _RET(value);
    }
}

static PyObject *
v_get(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(short int)));
    return PyBool_FromLong((long)*(short int *)ptr);
}

/* bool ('?'): bool (i.e. _Bool) */
static PyObject *
bool_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(bool)));
    switch (PyObject_IsTrue(value)) {
    case -1:
        return NULL;
    case 0:
        *(bool *)ptr = 0;
        _RET(value);
    default:
        *(bool *)ptr = 1;
        _RET(value);
    }
}

static PyObject *
bool_get(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(bool)));
    return PyBool_FromLong((long)*(bool *)ptr);
}

/* g: long double */
static PyObject *
g_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(long double)));
    long double x;

    x = PyFloat_AsDouble(value);
    if (x == -1 && PyErr_Occurred())
        return NULL;
    memcpy(ptr, &x, sizeof(long double));
    _RET(value);
}

static PyObject *
g_get(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(long double)));
    long double val;
    memcpy(&val, ptr, sizeof(long double));
    return PyFloat_FromDouble(val);
}

/* d: double */
static PyObject *
d_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(double)));
    double x;

    x = PyFloat_AsDouble(value);
    if (x == -1 && PyErr_Occurred())
        return NULL;
    memcpy(ptr, &x, sizeof(double));
    _RET(value);
}

static PyObject *
d_get(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(double)));
    double val;
    memcpy(&val, ptr, sizeof(val));
    return PyFloat_FromDouble(val);
}

#if defined(Py_FFI_SUPPORT_C_COMPLEX)

/* We don't use _Complex types here, using arrays instead, as the C11+
   standard says: "Each complex type has the same representation and alignment
   requirements as an array type containing exactly two elements of the
   corresponding real type; the first element is equal to the real part, and
   the second element to the imaginary part, of the complex number." */

/* D: double complex */
static PyObject *
D_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == 2*sizeof(double)));
    Py_complex c = PyComplex_AsCComplex(value);

    if (c.real == -1 && PyErr_Occurred()) {
        return NULL;
    }
    double x[2] = {c.real, c.imag};
    memcpy(ptr, &x, sizeof(x));
    _RET(value);
}

static PyObject *
D_get(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == 2*sizeof(double)));
    double x[2];

    memcpy(&x, ptr, sizeof(x));
    return PyComplex_FromDoubles(x[0], x[1]);
}

/* F: float complex */
static PyObject *
F_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == 2*sizeof(float)));
    Py_complex c = PyComplex_AsCComplex(value);

    if (c.real == -1 && PyErr_Occurred()) {
        return NULL;
    }
    float x[2] = {(float)c.real, (float)c.imag};
    memcpy(ptr, &x, sizeof(x));
    _RET(value);
}

static PyObject *
F_get(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == 2*sizeof(float)));
    float x[2];

    memcpy(&x, ptr, sizeof(x));
    return PyComplex_FromDoubles(x[0], x[1]);
}

/* G: long double complex */
static PyObject *
G_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == 2*sizeof(long double)));
    Py_complex c = PyComplex_AsCComplex(value);

    if (c.real == -1 && PyErr_Occurred()) {
        return NULL;
    }
    long double x[2] = {c.real, c.imag};
    memcpy(ptr, &x, sizeof(x));
    _RET(value);
}

static PyObject *
G_get(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == 2*sizeof(long double)));
    long double x[2];

    memcpy(&x, ptr, sizeof(x));
    return PyComplex_FromDoubles((double)x[0], (double)x[1]);
}
#endif

/* d: double */
static PyObject *
d_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(double)));
    double x;

    x = PyFloat_AsDouble(value);
    if (x == -1 && PyErr_Occurred())
        return NULL;
#ifdef WORDS_BIGENDIAN
    if (PyFloat_Pack8(x, ptr, 1))
        return NULL;
#else
    if (PyFloat_Pack8(x, ptr, 0))
        return NULL;
#endif
    _RET(value);
}

static PyObject *
d_get_sw(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(double)));
#ifdef WORDS_BIGENDIAN
    return PyFloat_FromDouble(PyFloat_Unpack8(ptr, 1));
#else
    return PyFloat_FromDouble(PyFloat_Unpack8(ptr, 0));
#endif
}

/* f: float */
static PyObject *
f_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(float)));
    float x;

    x = (float)PyFloat_AsDouble(value);
    if (x == -1 && PyErr_Occurred())
        return NULL;
    memcpy(ptr, &x, sizeof(x));
    _RET(value);
}

static PyObject *
f_get(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(float)));
    float val;
    memcpy(&val, ptr, sizeof(val));
    return PyFloat_FromDouble(val);
}

static PyObject *
f_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(float)));
    float x;

    x = (float)PyFloat_AsDouble(value);
    if (x == -1 && PyErr_Occurred())
        return NULL;
#ifdef WORDS_BIGENDIAN
    if (PyFloat_Pack4(x, ptr, 1))
        return NULL;
#else
    if (PyFloat_Pack4(x, ptr, 0))
        return NULL;
#endif
    _RET(value);
}

static PyObject *
f_get_sw(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(float)));
#ifdef WORDS_BIGENDIAN
    return PyFloat_FromDouble(PyFloat_Unpack4(ptr, 1));
#else
    return PyFloat_FromDouble(PyFloat_Unpack4(ptr, 0));
#endif
}

/* O: Python object */
/*
  py_object refcounts:

  1. If we have a py_object instance, O_get must Py_INCREF the returned
  object, of course.  If O_get is called from a function result, no py_object
  instance is created - so callproc.c::GetResult has to call Py_DECREF.

  2. The memory block in py_object owns a refcount.  So, py_object must call
  Py_DECREF on destruction.  Maybe only when b_needsfree is non-zero.
*/
static PyObject *
O_get(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(PyObject *)));
    PyObject *ob = *(PyObject **)ptr;
    if (ob == NULL) {
        if (!PyErr_Occurred())
            /* Set an error if not yet set */
            PyErr_SetString(PyExc_ValueError,
                            "PyObject is NULL");
        return NULL;
    }
    return Py_NewRef(ob);
}

static PyObject *
O_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(PyObject *)));
    /* Hm, does the memory block need it's own refcount or not? */
    *(PyObject **)ptr = value;
    return Py_NewRef(value);
}


/* c: a single byte-character */
static PyObject *
c_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(char)));
    if (PyBytes_Check(value)) {
        if (PyBytes_GET_SIZE(value) != 1) {
            PyErr_Format(PyExc_TypeError,
                        "one character bytes, bytearray, or an integer "
                        "in range(256) expected, not bytes of length %zd",
                        PyBytes_GET_SIZE(value));
            return NULL;
        }
        *(char *)ptr = PyBytes_AS_STRING(value)[0];
        _RET(value);
    }
    if (PyByteArray_Check(value)) {
        if (PyByteArray_GET_SIZE(value) != 1) {
            PyErr_Format(PyExc_TypeError,
                        "one character bytes, bytearray, or an integer "
                        "in range(256) expected, not bytearray of length %zd",
                        PyByteArray_GET_SIZE(value));
            return NULL;
        }
        *(char *)ptr = PyByteArray_AS_STRING(value)[0];
        _RET(value);
    }
    if (PyLong_Check(value)) {
        int overflow;
        long longval = PyLong_AsLongAndOverflow(value, &overflow);
        if (longval == -1 && PyErr_Occurred()) {
            return NULL;
        }
        if (overflow || longval < 0 || longval >= 256) {
            PyErr_SetString(PyExc_TypeError, "integer not in range(256)");
            return NULL;
        }
        *(char *)ptr = (char)longval;
        _RET(value);
    }
    PyErr_Format(PyExc_TypeError,
                 "one character bytes, bytearray, or an integer "
                 "in range(256) expected, not %T",
                 value);
    return NULL;
}


static PyObject *
c_get(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(char)));
    return PyBytes_FromStringAndSize((char *)ptr, 1);
}

/* u: a single wchar_t character */
static PyObject *
u_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(wchar_t)));
    Py_ssize_t len;
    wchar_t chars[2];
    if (!PyUnicode_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "a unicode character expected, not instance of %T",
                     value);
        return NULL;
    }

    len = PyUnicode_AsWideChar(value, chars, 2);
    if (len != 1) {
        if (PyUnicode_GET_LENGTH(value) != 1) {
            PyErr_Format(PyExc_TypeError,
                         "a unicode character expected, not a string of length %zd",
                         PyUnicode_GET_LENGTH(value));
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "the string %A cannot be converted to a single wchar_t character",
                         value);
        }
        return NULL;
    }

    *(wchar_t *)ptr = chars[0];

    _RET(value);
}


static PyObject *
u_get(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(wchar_t)));
    return PyUnicode_FromWideChar((wchar_t *)ptr, 1);
}

/* U: a wchar_t* unicode string */
static PyObject *
U_get(void *ptr, Py_ssize_t size)
{
    Py_ssize_t len;
    wchar_t *p;

    size /= sizeof(wchar_t); /* we count character units here, not bytes */

    /* We need 'result' to be able to count the characters with wcslen,
       since ptr may not be NUL terminated.  If the length is smaller (if
       it was actually NUL terminated, we construct a new one and throw
       away the result.
    */
    /* chop off at the first NUL character, if any. */
    p = (wchar_t*)ptr;
    for (len = 0; len < size; ++len) {
        if (!p[len])
            break;
    }

    return PyUnicode_FromWideChar((wchar_t *)ptr, len);
}

static PyObject *
U_set(void *ptr, PyObject *value, Py_ssize_t length)
{
    /* It's easier to calculate in characters than in bytes */
    length /= sizeof(wchar_t);

    if (!PyUnicode_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                        "unicode string expected instead of %s instance",
                        Py_TYPE(value)->tp_name);
        return NULL;
    }

    Py_ssize_t size = PyUnicode_AsWideChar(value, NULL, 0);
    if (size < 0) {
        return NULL;
    }
    // PyUnicode_AsWideChar() returns number of wchars including trailing null byte,
    // when it is called with NULL.
    size--;
    assert(size >= 0);
    if (size > length) {
        PyErr_Format(PyExc_ValueError,
                     "string too long (%zd, maximum length %zd)",
                     size, length);
        return NULL;
    }
    if (PyUnicode_AsWideChar(value, (wchar_t *)ptr, length) == -1) {
        return NULL;
    }

    return Py_NewRef(value);
}


/* s: a byte string */
static PyObject *
s_get(void *ptr, Py_ssize_t size)
{
    Py_ssize_t i;
    char *p;

    p = (char *)ptr;
    for (i = 0; i < size; ++i) {
        if (*p++ == '\0')
            break;
    }

    return PyBytes_FromStringAndSize((char *)ptr, (Py_ssize_t)i);
}

static PyObject *
s_set(void *ptr, PyObject *value, Py_ssize_t length)
{
    const char *data;
    Py_ssize_t size;

    if(!PyBytes_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "expected bytes, %s found",
                     Py_TYPE(value)->tp_name);
        return NULL;
    }

    data = PyBytes_AS_STRING(value);
    // bpo-39593: Use strlen() to truncate the string at the first null character.
    size = strlen(data);

    if (size < length) {
        /* This will copy the terminating NUL character
         * if there is space for it.
         */
        ++size;
    } else if (size > length) {
        PyErr_Format(PyExc_ValueError,
                     "bytes too long (%zd, maximum length %zd)",
                     size, length);
        return NULL;
    }
    /* Also copy the terminating NUL character if there is space */
    memcpy((char *)ptr, data, size);

    _RET(value);
}

/* z: a byte string, can be set from integer pointer */
static PyObject *
z_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    if (value == Py_None) {
        *(char **)ptr = NULL;
        return Py_NewRef(value);
    }
    if (PyBytes_Check(value)) {
        *(const char **)ptr = PyBytes_AsString(value);
        return Py_NewRef(value);
    } else if (PyLong_Check(value)) {
#if SIZEOF_VOID_P == SIZEOF_LONG_LONG
        *(char **)ptr = (char *)PyLong_AsUnsignedLongLongMask(value);
#else
        *(char **)ptr = (char *)PyLong_AsUnsignedLongMask(value);
#endif
        _RET(value);
    }
    PyErr_Format(PyExc_TypeError,
                 "bytes or integer address expected instead of %s instance",
                 Py_TYPE(value)->tp_name);
    return NULL;
}

static PyObject *
z_get(void *ptr, Py_ssize_t size)
{
    /* XXX What about invalid pointers ??? */
    if (*(void **)ptr) {
        return PyBytes_FromStringAndSize(*(char **)ptr,
                                         strlen(*(char **)ptr));
    } else {
        Py_RETURN_NONE;
    }
}

/* Z: a wchar* string, can be set from integer pointer */
static PyObject *
Z_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    PyObject *keep;
    wchar_t *buffer;
    Py_ssize_t bsize;

    if (value == Py_None) {
        *(wchar_t **)ptr = NULL;
        return Py_NewRef(value);
    }
    if (PyLong_Check(value)) {
#if SIZEOF_VOID_P == SIZEOF_LONG_LONG
        *(wchar_t **)ptr = (wchar_t *)PyLong_AsUnsignedLongLongMask(value);
#else
        *(wchar_t **)ptr = (wchar_t *)PyLong_AsUnsignedLongMask(value);
#endif
        Py_RETURN_NONE;
    }
    if (!PyUnicode_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "unicode string or integer address expected instead of %s instance",
                     Py_TYPE(value)->tp_name);
        return NULL;
    }

    /* We must create a wchar_t* buffer from the unicode object,
       and keep it alive */
    buffer = PyUnicode_AsWideCharString(value, &bsize);
    if (!buffer)
        return NULL;
    keep = PyCapsule_New(buffer, CTYPES_CFIELD_CAPSULE_NAME_PYMEM, pymem_destructor);
    if (!keep) {
        PyMem_Free(buffer);
        return NULL;
    }
    *(wchar_t **)ptr = buffer;
    return keep;
}

static PyObject *
Z_get(void *ptr, Py_ssize_t size)
{
    wchar_t *p;
    p = *(wchar_t **)ptr;
    if (p) {
        return PyUnicode_FromWideChar(p, -1);
    } else {
        Py_RETURN_NONE;
    }
}


#ifdef MS_WIN32
/* X: COM BSTR (wide-char string to be handled handled using Windows API) */
static PyObject *
X_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    BSTR bstr;

    /* convert value into a PyUnicodeObject or NULL */
    if (Py_None == value) {
        value = NULL;
    } else if (!PyUnicode_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                        "unicode string expected instead of %s instance",
                        Py_TYPE(value)->tp_name);
        return NULL;
    }

    /* create a BSTR from value */
    if (value) {
        Py_ssize_t wsize;
        wchar_t *wvalue = PyUnicode_AsWideCharString(value, &wsize);
        if (wvalue == NULL) {
            return NULL;
        }
        if ((unsigned) wsize != wsize) {
            PyErr_SetString(PyExc_ValueError, "String too long for BSTR");
            PyMem_Free(wvalue);
            return NULL;
        }
        bstr = SysAllocStringLen(wvalue, (unsigned)wsize);
        PyMem_Free(wvalue);
    } else
        bstr = NULL;

    /* free the previous contents, if any */
    if (*(BSTR *)ptr)
        SysFreeString(*(BSTR *)ptr);

    /* and store it */
    *(BSTR *)ptr = bstr;

    /* We don't need to keep any other object */
    _RET(value);
}


static PyObject *
X_get(void *ptr, Py_ssize_t size)
{
    BSTR p;
    p = *(BSTR *)ptr;
    if (p)
        return PyUnicode_FromWideChar(p, SysStringLen(p));
    else {
        /* Hm, it seems NULL pointer and zero length string are the
           same in BSTR, see Don Box, p 81
        */
        Py_RETURN_NONE;
    }
}
#endif

/* P: generic pointer */
static PyObject *
P_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(void *)));
    void *v;
    if (value == Py_None) {
        *(void **)ptr = NULL;
        _RET(value);
    }

    if (!PyLong_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "cannot be converted to pointer");
        return NULL;
    }

#if SIZEOF_VOID_P <= SIZEOF_LONG
    v = (void *)PyLong_AsUnsignedLongMask(value);
#else
#if SIZEOF_LONG_LONG < SIZEOF_VOID_P
#   error "PyLong_AsVoidPtr: sizeof(long long) < sizeof(void*)"
#endif
    v = (void *)PyLong_AsUnsignedLongLongMask(value);
#endif

    if (PyErr_Occurred())
        return NULL;

    *(void **)ptr = v;
    _RET(value);
}

static PyObject *
P_get(void *ptr, Py_ssize_t size)
{
    assert(NUM_BITS(size) || (size == sizeof(void *)));
    if (*(void **)ptr == NULL) {
        Py_RETURN_NONE;
    }
    return PyLong_FromVoidPtr(*(void **)ptr);
}

/* Table with info about all formats.
 * Must be accessed via _ctypes_get_fielddesc, which initializes it on
 * first use. After initialization it's treated as constant & read-only.
 */

struct formattable {
/*[python input]
for nbytes in 8, 16, 32, 64:
    for sgn in 'i', 'u':
        print(f'    struct fielddesc fmt_{sgn}{nbytes};')
for code in 'sbBcdFDGgfhHiIlLqQPzuUZXvO':
    print(f'    struct fielddesc fmt_{code};')
[python start generated code]*/
    struct fielddesc fmt_i8;
    struct fielddesc fmt_u8;
    struct fielddesc fmt_i16;
    struct fielddesc fmt_u16;
    struct fielddesc fmt_i32;
    struct fielddesc fmt_u32;
    struct fielddesc fmt_i64;
    struct fielddesc fmt_u64;
    struct fielddesc fmt_s;
    struct fielddesc fmt_b;
    struct fielddesc fmt_B;
    struct fielddesc fmt_c;
    struct fielddesc fmt_d;
    struct fielddesc fmt_F;
    struct fielddesc fmt_D;
    struct fielddesc fmt_G;
    struct fielddesc fmt_g;
    struct fielddesc fmt_f;
    struct fielddesc fmt_h;
    struct fielddesc fmt_H;
    struct fielddesc fmt_i;
    struct fielddesc fmt_I;
    struct fielddesc fmt_l;
    struct fielddesc fmt_L;
    struct fielddesc fmt_q;
    struct fielddesc fmt_Q;
    struct fielddesc fmt_P;
    struct fielddesc fmt_z;
    struct fielddesc fmt_u;
    struct fielddesc fmt_U;
    struct fielddesc fmt_Z;
    struct fielddesc fmt_X;
    struct fielddesc fmt_v;
    struct fielddesc fmt_O;
/*[python end generated code: output=f5a07c066fedaca6 input=ffa5d46c29dfb07a]*/

    // bool has code '?':
    struct fielddesc fmt_bool;

    // always contains NULLs:
    struct fielddesc fmt_nil;

    // Result of _ctypes_get_simple_type_chars. Initialized just after
    // the rest of formattable, so we stash it here.
    char simple_type_chars[26];
};

static struct formattable formattable;


/* Get fielddesc info for a fixed-width integer.
 * N.B: - must be called after (or from) _ctypes_init_fielddesc!
 *      - nbytes must be one of the supported values
 */

static inline struct fielddesc *
_ctypes_fixint_fielddesc(Py_ssize_t nbytes, bool is_signed)
{
#define _PACK(NBYTES, SGN) ((NBYTES<<2) + (SGN ? 1 : 0))
    switch (_PACK(nbytes, is_signed)) {
/*[python input]
for nbytes in 8, 16, 32, 64:
    for sgn in 'i', 'u':
        is_signed = sgn == 'i'
        print(f'    case (_PACK({nbytes // 8}, {int(is_signed)})): '
              + f'return &formattable.fmt_{sgn}{nbytes};')
[python start generated code]*/
    case (_PACK(1, 1)): return &formattable.fmt_i8;
    case (_PACK(1, 0)): return &formattable.fmt_u8;
    case (_PACK(2, 1)): return &formattable.fmt_i16;
    case (_PACK(2, 0)): return &formattable.fmt_u16;
    case (_PACK(4, 1)): return &formattable.fmt_i32;
    case (_PACK(4, 0)): return &formattable.fmt_u32;
    case (_PACK(8, 1)): return &formattable.fmt_i64;
    case (_PACK(8, 0)): return &formattable.fmt_u64;
/*[python end generated code: output=0194ba35c4d64ff3 input=ee9f6f5bb872d645]*/
#undef _PACK
    }
    /* ctypes currently only supports platforms where the basic integer types
     * (`char`, `short`, `int`, `long`, `long long`) have 1, 2, 4, or 8 bytes
     * (i.e. 8 to 64 bits).
     */
    Py_UNREACHABLE();
}


/* Macro to call _ctypes_fixint_fielddesc for a given C type. */

_Py_COMP_DIAG_PUSH
#if defined(__GNUC__) && (__GNUC__ < 14)
/* The signedness check expands to an expression that's always true or false.
 * Older GCC gives a '-Wtype-limits' warning for this, which is a GCC bug
 * (docs say it should "not warn for constant expressions"):
 *      https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86647
 * Silence that warning.
 */
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#define FIXINT_FIELDDESC_FOR(C_TYPE) \
    _ctypes_fixint_fielddesc(sizeof(C_TYPE), (C_TYPE)-1 < 0)


/* Delayed initialization. Windows cannot statically reference dynamically
   loaded addresses from DLLs. */
void
_ctypes_init_fielddesc(void)
{
    /* Fixed-width integers */

/*[python input]
for nbytes in 8, 16, 32, 64:
    for sgn in 'i', 'u':
        is_signed = sgn == 'i'
        u = 'u' if sgn == 'u' else 's'
        parts = [
            f"0",
            f'&ffi_type_{u}int{nbytes}',
            f'{sgn}{nbytes}_set',
            f'{sgn}{nbytes}_get',
            f'{sgn}{nbytes}_set_sw',
            f'{sgn}{nbytes}_get_sw',
        ]
        print(f'    formattable.fmt_{sgn}{nbytes} = (struct fielddesc){{')
        print(f'            {", ".join(parts)} }};')
[python start generated code]*/
    formattable.fmt_i8 = (struct fielddesc){
            0, &ffi_type_sint8, i8_set, i8_get, i8_set_sw, i8_get_sw };
    formattable.fmt_u8 = (struct fielddesc){
            0, &ffi_type_uint8, u8_set, u8_get, u8_set_sw, u8_get_sw };
    formattable.fmt_i16 = (struct fielddesc){
            0, &ffi_type_sint16, i16_set, i16_get, i16_set_sw, i16_get_sw };
    formattable.fmt_u16 = (struct fielddesc){
            0, &ffi_type_uint16, u16_set, u16_get, u16_set_sw, u16_get_sw };
    formattable.fmt_i32 = (struct fielddesc){
            0, &ffi_type_sint32, i32_set, i32_get, i32_set_sw, i32_get_sw };
    formattable.fmt_u32 = (struct fielddesc){
            0, &ffi_type_uint32, u32_set, u32_get, u32_set_sw, u32_get_sw };
    formattable.fmt_i64 = (struct fielddesc){
            0, &ffi_type_sint64, i64_set, i64_get, i64_set_sw, i64_get_sw };
    formattable.fmt_u64 = (struct fielddesc){
            0, &ffi_type_uint64, u64_set, u64_get, u64_set_sw, u64_get_sw };
/*[python end generated code: output=16806fe0ca3a9c4c input=96348a06e575f801]*/


    /* Native C integers.
     * These use getters/setters for fixed-width ints but have their own
     * `code` and `pffi_type`.
     */

/*[python input]
for base_code, base_c_type in [
    ('b', 'char'),
    ('h', 'short'),
    ('i', 'int'),
    ('l', 'long'),
    ('q', 'long long'),
]:
    for code, c_type, ffi_type in [
        (base_code, 'signed ' + base_c_type, 's' + base_c_type),
        (base_code.upper(), 'unsigned ' + base_c_type, 'u' + base_c_type),
    ]:
        print(f'    formattable.fmt_{code} = *FIXINT_FIELDDESC_FOR({c_type});')
        print(f"    formattable.fmt_{code}.code = '{code}';")
        if base_code == 'q':
            # ffi doesn't have `long long`; keep use the fixint type
            pass
        else:
            print(f'    formattable.fmt_{code}.pffi_type = &ffi_type_{ffi_type};')
[python start generated code]*/
    formattable.fmt_b = *FIXINT_FIELDDESC_FOR(signed char);
    formattable.fmt_b.code = 'b';
    formattable.fmt_b.pffi_type = &ffi_type_schar;
    formattable.fmt_B = *FIXINT_FIELDDESC_FOR(unsigned char);
    formattable.fmt_B.code = 'B';
    formattable.fmt_B.pffi_type = &ffi_type_uchar;
    formattable.fmt_h = *FIXINT_FIELDDESC_FOR(signed short);
    formattable.fmt_h.code = 'h';
    formattable.fmt_h.pffi_type = &ffi_type_sshort;
    formattable.fmt_H = *FIXINT_FIELDDESC_FOR(unsigned short);
    formattable.fmt_H.code = 'H';
    formattable.fmt_H.pffi_type = &ffi_type_ushort;
    formattable.fmt_i = *FIXINT_FIELDDESC_FOR(signed int);
    formattable.fmt_i.code = 'i';
    formattable.fmt_i.pffi_type = &ffi_type_sint;
    formattable.fmt_I = *FIXINT_FIELDDESC_FOR(unsigned int);
    formattable.fmt_I.code = 'I';
    formattable.fmt_I.pffi_type = &ffi_type_uint;
    formattable.fmt_l = *FIXINT_FIELDDESC_FOR(signed long);
    formattable.fmt_l.code = 'l';
    formattable.fmt_l.pffi_type = &ffi_type_slong;
    formattable.fmt_L = *FIXINT_FIELDDESC_FOR(unsigned long);
    formattable.fmt_L.code = 'L';
    formattable.fmt_L.pffi_type = &ffi_type_ulong;
    formattable.fmt_q = *FIXINT_FIELDDESC_FOR(signed long long);
    formattable.fmt_q.code = 'q';
    formattable.fmt_Q = *FIXINT_FIELDDESC_FOR(unsigned long long);
    formattable.fmt_Q.code = 'Q';
/*[python end generated code: output=873c87a2e6b5075a input=ee814ca263aac18e]*/


    /* Other types have bespoke setters and getters named `@_set` and `@_get`,
     * where `@` is the type code.
     * Some have swapped variants, `@_set_sw` and `@_get_sw`
     */

#define _TABLE_ENTRY(SYMBOL, FFI_TYPE, ...)                                   \
    formattable.fmt_ ## SYMBOL =                                              \
        (struct fielddesc){(#SYMBOL)[0], (FFI_TYPE), __VA_ARGS__};            \
    ///////////////////////////////////////////////////////////////////////////

#define TABLE_ENTRY(SYMBOL, FFI_TYPE)                                         \
    _TABLE_ENTRY(SYMBOL, FFI_TYPE, SYMBOL ## _set, SYMBOL ## _get)            \
    ///////////////////////////////////////////////////////////////////////////

#define TABLE_ENTRY_SW(SYMBOL, FFI_TYPE)                                      \
    _TABLE_ENTRY(SYMBOL, FFI_TYPE, SYMBOL ## _set,                            \
        SYMBOL ## _get, SYMBOL ## _set_sw, SYMBOL ## _get_sw)                 \
    ///////////////////////////////////////////////////////////////////////////

    TABLE_ENTRY_SW(d, &ffi_type_double);
#if defined(Py_FFI_SUPPORT_C_COMPLEX)
    if (Py_FFI_COMPLEX_AVAILABLE) {
        TABLE_ENTRY(D, &ffi_type_complex_double);
        TABLE_ENTRY(F, &ffi_type_complex_float);
        TABLE_ENTRY(G, &ffi_type_complex_longdouble);
    }
#endif
    TABLE_ENTRY(g, &ffi_type_longdouble);
    TABLE_ENTRY_SW(f, &ffi_type_float);
    TABLE_ENTRY(v, &ffi_type_sshort);    /* vBOOL */

    // ctypes.c_char is signed for FFI, even where C wchar_t is unsigned.
    TABLE_ENTRY(c, _ctypes_fixint_fielddesc(sizeof(char), true)->pffi_type);
    // ctypes.c_wchar is signed for FFI, even where C wchar_t is unsigned.
    TABLE_ENTRY(u, _ctypes_fixint_fielddesc(sizeof(wchar_t), true)->pffi_type);

    TABLE_ENTRY(s, &ffi_type_pointer);
    TABLE_ENTRY(P, &ffi_type_pointer);
    TABLE_ENTRY(z, &ffi_type_pointer);
    TABLE_ENTRY(U, &ffi_type_pointer);
    TABLE_ENTRY(Z, &ffi_type_pointer);
#ifdef MS_WIN32
    TABLE_ENTRY(X, &ffi_type_pointer);
#endif
    TABLE_ENTRY(O, &ffi_type_pointer);

#undef TABLE_ENTRY_SW
#undef TABLE_ENTRY
#undef _TABLE_ENTRY

    /* bool has code '?', fill it in manually */

    // ctypes.c_bool is unsigned for FFI, even where C bool is signed.
    formattable.fmt_bool = *_ctypes_fixint_fielddesc(sizeof(bool), false);
    formattable.fmt_bool.code = '?';
    formattable.fmt_bool.setfunc = bool_set;
    formattable.fmt_bool.getfunc = bool_get;

/*[python input]
all_chars = "cbBhHiIlLdDFGfuzZqQPXOv?g"
print(f'    assert(sizeof(formattable.simple_type_chars) == {len(all_chars)+1});')
print(f'    int i = 0;')
for char in all_chars:
    ident_char = {'?': 'bool'}.get(char, char)
    print(f"    if (formattable.fmt_{ident_char}.code) "
          + f"formattable.simple_type_chars[i++] = '{char}';")
print(f"    formattable.simple_type_chars[i] = 0;")
[python start generated code]*/
    assert(sizeof(formattable.simple_type_chars) == 26);
    int i = 0;
    if (formattable.fmt_c.code) formattable.simple_type_chars[i++] = 'c';
    if (formattable.fmt_b.code) formattable.simple_type_chars[i++] = 'b';
    if (formattable.fmt_B.code) formattable.simple_type_chars[i++] = 'B';
    if (formattable.fmt_h.code) formattable.simple_type_chars[i++] = 'h';
    if (formattable.fmt_H.code) formattable.simple_type_chars[i++] = 'H';
    if (formattable.fmt_i.code) formattable.simple_type_chars[i++] = 'i';
    if (formattable.fmt_I.code) formattable.simple_type_chars[i++] = 'I';
    if (formattable.fmt_l.code) formattable.simple_type_chars[i++] = 'l';
    if (formattable.fmt_L.code) formattable.simple_type_chars[i++] = 'L';
    if (formattable.fmt_d.code) formattable.simple_type_chars[i++] = 'd';
    if (formattable.fmt_D.code) formattable.simple_type_chars[i++] = 'D';
    if (formattable.fmt_F.code) formattable.simple_type_chars[i++] = 'F';
    if (formattable.fmt_G.code) formattable.simple_type_chars[i++] = 'G';
    if (formattable.fmt_f.code) formattable.simple_type_chars[i++] = 'f';
    if (formattable.fmt_u.code) formattable.simple_type_chars[i++] = 'u';
    if (formattable.fmt_z.code) formattable.simple_type_chars[i++] = 'z';
    if (formattable.fmt_Z.code) formattable.simple_type_chars[i++] = 'Z';
    if (formattable.fmt_q.code) formattable.simple_type_chars[i++] = 'q';
    if (formattable.fmt_Q.code) formattable.simple_type_chars[i++] = 'Q';
    if (formattable.fmt_P.code) formattable.simple_type_chars[i++] = 'P';
    if (formattable.fmt_X.code) formattable.simple_type_chars[i++] = 'X';
    if (formattable.fmt_O.code) formattable.simple_type_chars[i++] = 'O';
    if (formattable.fmt_v.code) formattable.simple_type_chars[i++] = 'v';
    if (formattable.fmt_bool.code) formattable.simple_type_chars[i++] = '?';
    if (formattable.fmt_g.code) formattable.simple_type_chars[i++] = 'g';
    formattable.simple_type_chars[i] = 0;
/*[python end generated code: output=2aa52670d1570f18 input=cff3e7cb95adac61]*/

}
#undef FIXINT_FIELDDESC_FOR
_Py_COMP_DIAG_POP

char *
_ctypes_get_simple_type_chars(void) {
    return formattable.simple_type_chars;
}

struct fielddesc *
_ctypes_get_fielddesc(const char *fmt)
{
    struct fielddesc *result = NULL;
    switch(fmt[0]) {
/*[python input]
for code in 'sbBcdDFGgfhHiIlLqQPzuUZXvO':
    print(f"        case '{code}': result = &formattable.fmt_{code}; break;")
[python start generated code]*/
        case 's': result = &formattable.fmt_s; break;
        case 'b': result = &formattable.fmt_b; break;
        case 'B': result = &formattable.fmt_B; break;
        case 'c': result = &formattable.fmt_c; break;
        case 'd': result = &formattable.fmt_d; break;
        case 'D': result = &formattable.fmt_D; break;
        case 'F': result = &formattable.fmt_F; break;
        case 'G': result = &formattable.fmt_G; break;
        case 'g': result = &formattable.fmt_g; break;
        case 'f': result = &formattable.fmt_f; break;
        case 'h': result = &formattable.fmt_h; break;
        case 'H': result = &formattable.fmt_H; break;
        case 'i': result = &formattable.fmt_i; break;
        case 'I': result = &formattable.fmt_I; break;
        case 'l': result = &formattable.fmt_l; break;
        case 'L': result = &formattable.fmt_L; break;
        case 'q': result = &formattable.fmt_q; break;
        case 'Q': result = &formattable.fmt_Q; break;
        case 'P': result = &formattable.fmt_P; break;
        case 'z': result = &formattable.fmt_z; break;
        case 'u': result = &formattable.fmt_u; break;
        case 'U': result = &formattable.fmt_U; break;
        case 'Z': result = &formattable.fmt_Z; break;
        case 'X': result = &formattable.fmt_X; break;
        case 'v': result = &formattable.fmt_v; break;
        case 'O': result = &formattable.fmt_O; break;
/*[python end generated code: output=6e5c91940732fde9 input=902223feffc2fe38]*/
        case '?': result = &formattable.fmt_bool; break;
    }
    if (!result || !result->code) {
        return NULL;
    }
    assert(result->pffi_type);
    assert(result->setfunc);
    assert(result->getfunc);
    return result;
}

/*
  Ideas: Implement VARIANT in this table, using 'V' code.
*/

/*---------------- EOF ----------------*/
