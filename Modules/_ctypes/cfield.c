#include "Python.h"
#include "pycore_byteswap.h"      // _Py_bswap32()

#include <ffi.h>
#ifdef MS_WIN32
#include <windows.h>
#endif
#include "ctypes.h"


#define CTYPES_CFIELD_CAPSULE_NAME_PYMEM "_ctypes/cfield.c pymem"

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
static PyObject *
PyCField_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    CFieldObject *obj;
    obj = (CFieldObject *)type->tp_alloc(type, 0);
    return (PyObject *)obj;
}

/*
 * Expects the size, index and offset for the current field in *psize and
 * *poffset, stores the total size so far in *psize, the offset for the next
 * field in *poffset, the alignment requirements for the current field in
 * *palign, and returns a field desriptor for this field.
 */
/*
 * bitfields extension:
 * bitsize != 0: this is a bit field.
 * pbitofs points to the current bit offset, this will be updated.
 * prev_desc points to the type of the previous bitfield, if any.
 */
PyObject *
PyCField_FromDesc(PyObject *desc, Py_ssize_t index,
                Py_ssize_t *pfield_size, int bitsize, int *pbitofs,
                Py_ssize_t *psize, Py_ssize_t *poffset, Py_ssize_t *palign,
                int pack, int big_endian)
{
    CFieldObject *self;
    PyObject *proto;
    Py_ssize_t size, align;
    SETFUNC setfunc = NULL;
    GETFUNC getfunc = NULL;
    StgDictObject *dict;
    int fieldtype;
#define NO_BITFIELD 0
#define NEW_BITFIELD 1
#define CONT_BITFIELD 2
#define EXPAND_BITFIELD 3

    self = (CFieldObject *)_PyObject_CallNoArg((PyObject *)&PyCField_Type);
    if (self == NULL)
        return NULL;
    dict = PyType_stgdict(desc);
    if (!dict) {
        PyErr_SetString(PyExc_TypeError,
                        "has no _stginfo_");
        Py_DECREF(self);
        return NULL;
    }
    if (bitsize /* this is a bitfield request */
        && *pfield_size /* we have a bitfield open */
#ifdef MS_WIN32
        /* MSVC, GCC with -mms-bitfields */
        && dict->size * 8 == *pfield_size
#else
        /* GCC */
        && dict->size * 8 <= *pfield_size
#endif
        && (*pbitofs + bitsize) <= *pfield_size) {
        /* continue bit field */
        fieldtype = CONT_BITFIELD;
#ifndef MS_WIN32
    } else if (bitsize /* this is a bitfield request */
        && *pfield_size /* we have a bitfield open */
        && dict->size * 8 >= *pfield_size
        && (*pbitofs + bitsize) <= dict->size * 8) {
        /* expand bit field */
        fieldtype = EXPAND_BITFIELD;
#endif
    } else if (bitsize) {
        /* start new bitfield */
        fieldtype = NEW_BITFIELD;
        *pbitofs = 0;
        *pfield_size = dict->size * 8;
    } else {
        /* not a bit field */
        fieldtype = NO_BITFIELD;
        *pbitofs = 0;
        *pfield_size = 0;
    }

    size = dict->size;
    proto = desc;

    /*  Field descriptors for 'c_char * n' are be scpecial cased to
        return a Python string instead of an Array object instance...
    */
    if (PyCArrayTypeObject_Check(proto)) {
        StgDictObject *adict = PyType_stgdict(proto);
        StgDictObject *idict;
        if (adict && adict->proto) {
            idict = PyType_stgdict(adict->proto);
            if (!idict) {
                PyErr_SetString(PyExc_TypeError,
                                "has no _stginfo_");
                Py_DECREF(self);
                return NULL;
            }
            if (idict->getfunc == _ctypes_get_fielddesc("c")->getfunc) {
                struct fielddesc *fd = _ctypes_get_fielddesc("s");
                getfunc = fd->getfunc;
                setfunc = fd->setfunc;
            }
#ifdef CTYPES_UNICODE
            if (idict->getfunc == _ctypes_get_fielddesc("u")->getfunc) {
                struct fielddesc *fd = _ctypes_get_fielddesc("U");
                getfunc = fd->getfunc;
                setfunc = fd->setfunc;
            }
#endif
        }
    }

    self->setfunc = setfunc;
    self->getfunc = getfunc;
    self->index = index;

    Py_INCREF(proto);
    self->proto = proto;

    switch (fieldtype) {
    case NEW_BITFIELD:
        if (big_endian)
            self->size = (bitsize << 16) + *pfield_size - *pbitofs - bitsize;
        else
            self->size = (bitsize << 16) + *pbitofs;
        *pbitofs = bitsize;
        /* fall through */
    case NO_BITFIELD:
        if (pack)
            align = min(pack, dict->align);
        else
            align = dict->align;
        if (align && *poffset % align) {
            Py_ssize_t delta = align - (*poffset % align);
            *psize += delta;
            *poffset += delta;
        }

        if (bitsize == 0)
            self->size = size;
        *psize += size;

        self->offset = *poffset;
        *poffset += size;

        *palign = align;
        break;

    case EXPAND_BITFIELD:
        *poffset += dict->size - *pfield_size/8;
        *psize += dict->size - *pfield_size/8;

        *pfield_size = dict->size * 8;

        if (big_endian)
            self->size = (bitsize << 16) + *pfield_size - *pbitofs - bitsize;
        else
            self->size = (bitsize << 16) + *pbitofs;

        self->offset = *poffset - size; /* poffset is already updated for the NEXT field */
        *pbitofs += bitsize;
        break;

    case CONT_BITFIELD:
        if (big_endian)
            self->size = (bitsize << 16) + *pfield_size - *pbitofs - bitsize;
        else
            self->size = (bitsize << 16) + *pbitofs;

        self->offset = *poffset - size; /* poffset is already updated for the NEXT field */
        *pbitofs += bitsize;
        break;
    }

    return (PyObject *)self;
}

static int
PyCField_set(CFieldObject *self, PyObject *inst, PyObject *value)
{
    CDataObject *dst;
    char *ptr;
    if (!CDataObject_Check(inst)) {
        PyErr_SetString(PyExc_TypeError,
                        "not a ctype instance");
        return -1;
    }
    dst = (CDataObject *)inst;
    ptr = dst->b_ptr + self->offset;
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "can't delete attribute");
        return -1;
    }
    return PyCData_set(inst, self->proto, self->setfunc, value,
                     self->index, self->size, ptr);
}

static PyObject *
PyCField_get(CFieldObject *self, PyObject *inst, PyTypeObject *type)
{
    CDataObject *src;
    if (inst == NULL) {
        Py_INCREF(self);
        return (PyObject *)self;
    }
    if (!CDataObject_Check(inst)) {
        PyErr_SetString(PyExc_TypeError,
                        "not a ctype instance");
        return NULL;
    }
    src = (CDataObject *)inst;
    return PyCData_get(self->proto, self->getfunc, inst,
                     self->index, self->size, src->b_ptr + self->offset);
}

static PyObject *
PyCField_get_offset(PyObject *self, void *data)
{
    return PyLong_FromSsize_t(((CFieldObject *)self)->offset);
}

static PyObject *
PyCField_get_size(PyObject *self, void *data)
{
    return PyLong_FromSsize_t(((CFieldObject *)self)->size);
}

static PyGetSetDef PyCField_getset[] = {
    { "offset", PyCField_get_offset, NULL, "offset in bytes of this field" },
    { "size", PyCField_get_size, NULL, "size in bytes of this field" },
    { NULL, NULL, NULL, NULL },
};

static int
PyCField_traverse(CFieldObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->proto);
    return 0;
}

static int
PyCField_clear(CFieldObject *self)
{
    Py_CLEAR(self->proto);
    return 0;
}

static void
PyCField_dealloc(PyObject *self)
{
    PyCField_clear((CFieldObject *)self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
PyCField_repr(CFieldObject *self)
{
    PyObject *result;
    Py_ssize_t bits = self->size >> 16;
    Py_ssize_t size = self->size & 0xFFFF;
    const char *name;

    name = ((PyTypeObject *)self->proto)->tp_name;

    if (bits)
        result = PyUnicode_FromFormat(
            "<Field type=%s, ofs=%zd:%zd, bits=%zd>",
            name, self->offset, size, bits);
    else
        result = PyUnicode_FromFormat(
            "<Field type=%s, ofs=%zd, size=%zd>",
            name, self->offset, size);
    return result;
}

PyTypeObject PyCField_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.CField",                                   /* tp_name */
    sizeof(CFieldObject),                       /* tp_basicsize */
    0,                                          /* tp_itemsize */
    PyCField_dealloc,                                   /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)PyCField_repr,                            /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    "Structure/Union member",                   /* tp_doc */
    (traverseproc)PyCField_traverse,                    /* tp_traverse */
    (inquiry)PyCField_clear,                            /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    PyCField_getset,                                    /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    (descrgetfunc)PyCField_get,                 /* tp_descr_get */
    (descrsetfunc)PyCField_set,                 /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    PyCField_new,                               /* tp_new */
    0,                                          /* tp_free */
};


/******************************************************************/
/*
  Accessor functions
*/

/* Derived from Modules/structmodule.c:
   Helper routine to get a Python integer and raise the appropriate error
   if it isn't one */

static int
get_long(PyObject *v, long *p)
{
    long x;

    if (PyFloat_Check(v)) {
        PyErr_SetString(PyExc_TypeError,
                        "int expected instead of float");
        return -1;
    }
    x = PyLong_AsUnsignedLongMask(v);
    if (x == -1 && PyErr_Occurred())
        return -1;
    *p = x;
    return 0;
}

/* Same, but handling unsigned long */

static int
get_ulong(PyObject *v, unsigned long *p)
{
    unsigned long x;

    if (PyFloat_Check(v)) {
        PyErr_SetString(PyExc_TypeError,
                        "int expected instead of float");
        return -1;
    }
    x = PyLong_AsUnsignedLongMask(v);
    if (x == (unsigned long)-1 && PyErr_Occurred())
        return -1;
    *p = x;
    return 0;
}

/* Same, but handling native long long. */

static int
get_longlong(PyObject *v, long long *p)
{
    long long x;
    if (PyFloat_Check(v)) {
        PyErr_SetString(PyExc_TypeError,
                        "int expected instead of float");
        return -1;
    }
    x = PyLong_AsUnsignedLongLongMask(v);
    if (x == -1 && PyErr_Occurred())
        return -1;
    *p = x;
    return 0;
}

/* Same, but handling native unsigned long long. */

static int
get_ulonglong(PyObject *v, unsigned long long *p)
{
    unsigned long long x;
    if (PyFloat_Check(v)) {
        PyErr_SetString(PyExc_TypeError,
                        "int expected instead of float");
        return -1;
    }
    x = PyLong_AsUnsignedLongLongMask(v);
    if (x == (unsigned long long)-1 && PyErr_Occurred())
        return -1;
    *p = x;
    return 0;
}

/*****************************************************************
 * Integer fields, with bitfield support
 */

/* how to decode the size field, for integer get/set functions */
#define LOW_BIT(x)  ((x) & 0xFFFF)
#define NUM_BITS(x) ((x) >> 16)

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
     ( ( (type)x & ~(BIT_MASK(type, size) << LOW_BIT(size)) ) | ( ((type)v & BIT_MASK(type, size)) << LOW_BIT(size) ) ) \
     : (type)v)

#if SIZEOF_SHORT == 2
#  define SWAP_SHORT _Py_bswap16
#else
#  error "unsupported short size"
#endif

#if SIZEOF_INT == 4
#  define SWAP_INT _Py_bswap32
#else
#  error "unsupported int size"
#endif

#if SIZEOF_LONG == 4
#  define SWAP_LONG _Py_bswap32
#elif SIZEOF_LONG == 8
#  define SWAP_LONG _Py_bswap64
#else
#  error "unsupported long size"
#endif

#if SIZEOF_LONG_LONG == 8
#  define SWAP_LONG_LONG _Py_bswap64
#else
#  error "unsupported long long size"
#endif

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
 * integer accessor methods, supporting bit fields
 */

static PyObject *
b_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    long val;
    if (get_long(value, &val) < 0)
        return NULL;
    *(signed char *)ptr = SET(signed char, *(signed char *)ptr, val, size);
    _RET(value);
}


static PyObject *
b_get(void *ptr, Py_ssize_t size)
{
    signed char val = *(signed char *)ptr;
    GET_BITFIELD(val, size);
    return PyLong_FromLong(val);
}

static PyObject *
B_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    unsigned long val;
    if (get_ulong(value, &val) < 0)
        return NULL;
    *(unsigned char *)ptr = SET(unsigned char, *(unsigned char*)ptr, val, size);
    _RET(value);
}


static PyObject *
B_get(void *ptr, Py_ssize_t size)
{
    unsigned char val = *(unsigned char *)ptr;
    GET_BITFIELD(val, size);
    return PyLong_FromLong(val);
}

static PyObject *
h_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    long val;
    short x;
    if (get_long(value, &val) < 0)
        return NULL;
    memcpy(&x, ptr, sizeof(x));
    x = SET(short, x, val, size);
    memcpy(ptr, &x, sizeof(x));
    _RET(value);
}


static PyObject *
h_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
    long val;
    short field;
    if (get_long(value, &val) < 0) {
        return NULL;
    }
    memcpy(&field, ptr, sizeof(field));
    field = SWAP_SHORT(field);
    field = SET(short, field, val, size);
    field = SWAP_SHORT(field);
    memcpy(ptr, &field, sizeof(field));
    _RET(value);
}

static PyObject *
h_get(void *ptr, Py_ssize_t size)
{
    short val;
    memcpy(&val, ptr, sizeof(val));
    GET_BITFIELD(val, size);
    return PyLong_FromLong((long)val);
}

static PyObject *
h_get_sw(void *ptr, Py_ssize_t size)
{
    short val;
    memcpy(&val, ptr, sizeof(val));
    val = SWAP_SHORT(val);
    GET_BITFIELD(val, size);
    return PyLong_FromLong(val);
}

static PyObject *
H_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    unsigned long val;
    unsigned short x;
    if (get_ulong(value, &val) < 0)
        return NULL;
    memcpy(&x, ptr, sizeof(x));
    x = SET(unsigned short, x, val, size);
    memcpy(ptr, &x, sizeof(x));
    _RET(value);
}

static PyObject *
H_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
    unsigned long val;
    unsigned short field;
    if (get_ulong(value, &val) < 0) {
        return NULL;
    }
    memcpy(&field, ptr, sizeof(field));
    field = SWAP_SHORT(field);
    field = SET(unsigned short, field, val, size);
    field = SWAP_SHORT(field);
    memcpy(ptr, &field, sizeof(field));
    _RET(value);
}


static PyObject *
H_get(void *ptr, Py_ssize_t size)
{
    unsigned short val;
    memcpy(&val, ptr, sizeof(val));
    GET_BITFIELD(val, size);
    return PyLong_FromLong(val);
}

static PyObject *
H_get_sw(void *ptr, Py_ssize_t size)
{
    unsigned short val;
    memcpy(&val, ptr, sizeof(val));
    val = SWAP_SHORT(val);
    GET_BITFIELD(val, size);
    return PyLong_FromLong(val);
}

static PyObject *
i_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    long val;
    int x;
    if (get_long(value, &val) < 0)
        return NULL;
    memcpy(&x, ptr, sizeof(x));
    x = SET(int, x, val, size);
    memcpy(ptr, &x, sizeof(x));
    _RET(value);
}

static PyObject *
i_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
    long val;
    int field;
    if (get_long(value, &val) < 0) {
        return NULL;
    }
    memcpy(&field, ptr, sizeof(field));
    field = SWAP_INT(field);
    field = SET(int, field, val, size);
    field = SWAP_INT(field);
    memcpy(ptr, &field, sizeof(field));
    _RET(value);
}


static PyObject *
i_get(void *ptr, Py_ssize_t size)
{
    int val;
    memcpy(&val, ptr, sizeof(val));
    GET_BITFIELD(val, size);
    return PyLong_FromLong(val);
}

static PyObject *
i_get_sw(void *ptr, Py_ssize_t size)
{
    int val;
    memcpy(&val, ptr, sizeof(val));
    val = SWAP_INT(val);
    GET_BITFIELD(val, size);
    return PyLong_FromLong(val);
}

#ifdef MS_WIN32
/* short BOOL - VARIANT_BOOL */
static PyObject *
vBOOL_set(void *ptr, PyObject *value, Py_ssize_t size)
{
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
vBOOL_get(void *ptr, Py_ssize_t size)
{
    return PyBool_FromLong((long)*(short int *)ptr);
}
#endif

static PyObject *
bool_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    switch (PyObject_IsTrue(value)) {
    case -1:
        return NULL;
    case 0:
        *(_Bool *)ptr = 0;
        _RET(value);
    default:
        *(_Bool *)ptr = 1;
        _RET(value);
    }
}

static PyObject *
bool_get(void *ptr, Py_ssize_t size)
{
    return PyBool_FromLong((long)*(_Bool *)ptr);
}

static PyObject *
I_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    unsigned long val;
    unsigned int x;
    if (get_ulong(value, &val) < 0)
        return  NULL;
    memcpy(&x, ptr, sizeof(x));
    x = SET(unsigned int, x, val, size);
    memcpy(ptr, &x, sizeof(x));
    _RET(value);
}

static PyObject *
I_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
    unsigned long val;
    unsigned int field;
    if (get_ulong(value, &val) < 0) {
        return  NULL;
    }
    memcpy(&field, ptr, sizeof(field));
    field = SWAP_INT(field);
    field = SET(unsigned int, field, (unsigned int)val, size);
    field = SWAP_INT(field);
    memcpy(ptr, &field, sizeof(field));
    _RET(value);
}


static PyObject *
I_get(void *ptr, Py_ssize_t size)
{
    unsigned int val;
    memcpy(&val, ptr, sizeof(val));
    GET_BITFIELD(val, size);
    return PyLong_FromUnsignedLong(val);
}

static PyObject *
I_get_sw(void *ptr, Py_ssize_t size)
{
    unsigned int val;
    memcpy(&val, ptr, sizeof(val));
    val = SWAP_INT(val);
    GET_BITFIELD(val, size);
    return PyLong_FromUnsignedLong(val);
}

static PyObject *
l_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    long val;
    long x;
    if (get_long(value, &val) < 0)
        return NULL;
    memcpy(&x, ptr, sizeof(x));
    x = SET(long, x, val, size);
    memcpy(ptr, &x, sizeof(x));
    _RET(value);
}

static PyObject *
l_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
    long val;
    long field;
    if (get_long(value, &val) < 0) {
        return NULL;
    }
    memcpy(&field, ptr, sizeof(field));
    field = SWAP_LONG(field);
    field = SET(long, field, val, size);
    field = SWAP_LONG(field);
    memcpy(ptr, &field, sizeof(field));
    _RET(value);
}


static PyObject *
l_get(void *ptr, Py_ssize_t size)
{
    long val;
    memcpy(&val, ptr, sizeof(val));
    GET_BITFIELD(val, size);
    return PyLong_FromLong(val);
}

static PyObject *
l_get_sw(void *ptr, Py_ssize_t size)
{
    long val;
    memcpy(&val, ptr, sizeof(val));
    val = SWAP_LONG(val);
    GET_BITFIELD(val, size);
    return PyLong_FromLong(val);
}

static PyObject *
L_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    unsigned long val;
    unsigned long x;
    if (get_ulong(value, &val) < 0)
        return  NULL;
    memcpy(&x, ptr, sizeof(x));
    x = SET(unsigned long, x, val, size);
    memcpy(ptr, &x, sizeof(x));
    _RET(value);
}

static PyObject *
L_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
    unsigned long val;
    unsigned long field;
    if (get_ulong(value, &val) < 0) {
        return  NULL;
    }
    memcpy(&field, ptr, sizeof(field));
    field = SWAP_LONG(field);
    field = SET(unsigned long, field, val, size);
    field = SWAP_LONG(field);
    memcpy(ptr, &field, sizeof(field));
    _RET(value);
}


static PyObject *
L_get(void *ptr, Py_ssize_t size)
{
    unsigned long val;
    memcpy(&val, ptr, sizeof(val));
    GET_BITFIELD(val, size);
    return PyLong_FromUnsignedLong(val);
}

static PyObject *
L_get_sw(void *ptr, Py_ssize_t size)
{
    unsigned long val;
    memcpy(&val, ptr, sizeof(val));
    val = SWAP_LONG(val);
    GET_BITFIELD(val, size);
    return PyLong_FromUnsignedLong(val);
}

static PyObject *
q_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    long long val;
    long long x;
    if (get_longlong(value, &val) < 0)
        return NULL;
    memcpy(&x, ptr, sizeof(x));
    x = SET(long long, x, val, size);
    memcpy(ptr, &x, sizeof(x));
    _RET(value);
}

static PyObject *
q_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
    long long val;
    long long field;
    if (get_longlong(value, &val) < 0) {
        return NULL;
    }
    memcpy(&field, ptr, sizeof(field));
    field = SWAP_LONG_LONG(field);
    field = SET(long long, field, val, size);
    field = SWAP_LONG_LONG(field);
    memcpy(ptr, &field, sizeof(field));
    _RET(value);
}

static PyObject *
q_get(void *ptr, Py_ssize_t size)
{
    long long val;
    memcpy(&val, ptr, sizeof(val));
    GET_BITFIELD(val, size);
    return PyLong_FromLongLong(val);
}

static PyObject *
q_get_sw(void *ptr, Py_ssize_t size)
{
    long long val;
    memcpy(&val, ptr, sizeof(val));
    val = SWAP_LONG_LONG(val);
    GET_BITFIELD(val, size);
    return PyLong_FromLongLong(val);
}

static PyObject *
Q_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    unsigned long long val;
    unsigned long long x;
    if (get_ulonglong(value, &val) < 0)
        return NULL;
    memcpy(&x, ptr, sizeof(x));
    x = SET(long long, x, val, size);
    memcpy(ptr, &x, sizeof(x));
    _RET(value);
}

static PyObject *
Q_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
    unsigned long long val;
    unsigned long long field;
    if (get_ulonglong(value, &val) < 0) {
        return NULL;
    }
    memcpy(&field, ptr, sizeof(field));
    field = SWAP_LONG_LONG(field);
    field = SET(unsigned long long, field, val, size);
    field = SWAP_LONG_LONG(field);
    memcpy(ptr, &field, sizeof(field));
    _RET(value);
}

static PyObject *
Q_get(void *ptr, Py_ssize_t size)
{
    unsigned long long val;
    memcpy(&val, ptr, sizeof(val));
    GET_BITFIELD(val, size);
    return PyLong_FromUnsignedLongLong(val);
}

static PyObject *
Q_get_sw(void *ptr, Py_ssize_t size)
{
    unsigned long long val;
    memcpy(&val, ptr, sizeof(val));
    val = SWAP_LONG_LONG(val);
    GET_BITFIELD(val, size);
    return PyLong_FromUnsignedLongLong(val);
}

/*****************************************************************
 * non-integer accessor methods, not supporting bit fields
 */


static PyObject *
g_set(void *ptr, PyObject *value, Py_ssize_t size)
{
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
    long double val;
    memcpy(&val, ptr, sizeof(long double));
    return PyFloat_FromDouble(val);
}

static PyObject *
d_set(void *ptr, PyObject *value, Py_ssize_t size)
{
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
    double val;
    memcpy(&val, ptr, sizeof(val));
    return PyFloat_FromDouble(val);
}

static PyObject *
d_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
    double x;

    x = PyFloat_AsDouble(value);
    if (x == -1 && PyErr_Occurred())
        return NULL;
#ifdef WORDS_BIGENDIAN
    if (_PyFloat_Pack8(x, (unsigned char *)ptr, 1))
        return NULL;
#else
    if (_PyFloat_Pack8(x, (unsigned char *)ptr, 0))
        return NULL;
#endif
    _RET(value);
}

static PyObject *
d_get_sw(void *ptr, Py_ssize_t size)
{
#ifdef WORDS_BIGENDIAN
    return PyFloat_FromDouble(_PyFloat_Unpack8(ptr, 1));
#else
    return PyFloat_FromDouble(_PyFloat_Unpack8(ptr, 0));
#endif
}

static PyObject *
f_set(void *ptr, PyObject *value, Py_ssize_t size)
{
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
    float val;
    memcpy(&val, ptr, sizeof(val));
    return PyFloat_FromDouble(val);
}

static PyObject *
f_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
    float x;

    x = (float)PyFloat_AsDouble(value);
    if (x == -1 && PyErr_Occurred())
        return NULL;
#ifdef WORDS_BIGENDIAN
    if (_PyFloat_Pack4(x, (unsigned char *)ptr, 1))
        return NULL;
#else
    if (_PyFloat_Pack4(x, (unsigned char *)ptr, 0))
        return NULL;
#endif
    _RET(value);
}

static PyObject *
f_get_sw(void *ptr, Py_ssize_t size)
{
#ifdef WORDS_BIGENDIAN
    return PyFloat_FromDouble(_PyFloat_Unpack4(ptr, 1));
#else
    return PyFloat_FromDouble(_PyFloat_Unpack4(ptr, 0));
#endif
}

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
    PyObject *ob = *(PyObject **)ptr;
    if (ob == NULL) {
        if (!PyErr_Occurred())
            /* Set an error if not yet set */
            PyErr_SetString(PyExc_ValueError,
                            "PyObject is NULL");
        return NULL;
    }
    Py_INCREF(ob);
    return ob;
}

static PyObject *
O_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    /* Hm, does the memory block need it's own refcount or not? */
    *(PyObject **)ptr = value;
    Py_INCREF(value);
    return value;
}


static PyObject *
c_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    if (PyBytes_Check(value) && PyBytes_GET_SIZE(value) == 1) {
        *(char *)ptr = PyBytes_AS_STRING(value)[0];
        _RET(value);
    }
    if (PyByteArray_Check(value) && PyByteArray_GET_SIZE(value) == 1) {
        *(char *)ptr = PyByteArray_AS_STRING(value)[0];
        _RET(value);
    }
    if (PyLong_Check(value))
    {
        long longval = PyLong_AsLong(value);
        if (longval < 0 || longval >= 256)
            goto error;
        *(char *)ptr = (char)longval;
        _RET(value);
    }
  error:
    PyErr_Format(PyExc_TypeError,
                 "one character bytes, bytearray or integer expected");
    return NULL;
}


static PyObject *
c_get(void *ptr, Py_ssize_t size)
{
    return PyBytes_FromStringAndSize((char *)ptr, 1);
}

#ifdef CTYPES_UNICODE
/* u - a single wchar_t character */
static PyObject *
u_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    Py_ssize_t len;
    wchar_t chars[2];
    if (!PyUnicode_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                        "unicode string expected instead of %s instance",
                        Py_TYPE(value)->tp_name);
        return NULL;
    } else
        Py_INCREF(value);

    len = PyUnicode_AsWideChar(value, chars, 2);
    if (len != 1) {
        Py_DECREF(value);
        PyErr_SetString(PyExc_TypeError,
                        "one character unicode string expected");
        return NULL;
    }

    *(wchar_t *)ptr = chars[0];
    Py_DECREF(value);

    _RET(value);
}


static PyObject *
u_get(void *ptr, Py_ssize_t size)
{
    return PyUnicode_FromWideChar((wchar_t *)ptr, 1);
}

/* U - a unicode string */
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
    } else if (size < length-1)
        /* copy terminating NUL character if there is space */
        size += 1;

    if (PyUnicode_AsWideChar(value, (wchar_t *)ptr, size) == -1) {
        return NULL;
    }

    Py_INCREF(value);
    return value;
}

#endif

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
    size = strlen(data); /* XXX Why not Py_SIZE(value)? */
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

static PyObject *
z_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    if (value == Py_None) {
        *(char **)ptr = NULL;
        Py_INCREF(value);
        return value;
    }
    if (PyBytes_Check(value)) {
        *(const char **)ptr = PyBytes_AsString(value);
        Py_INCREF(value);
        return value;
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

#ifdef CTYPES_UNICODE
static PyObject *
Z_set(void *ptr, PyObject *value, Py_ssize_t size)
{
    PyObject *keep;
    wchar_t *buffer;

    if (value == Py_None) {
        *(wchar_t **)ptr = NULL;
        Py_INCREF(value);
        return value;
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
    buffer = PyUnicode_AsWideCharString(value, NULL);
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
        return PyUnicode_FromWideChar(p, wcslen(p));
    } else {
        Py_RETURN_NONE;
    }
}
#endif

#ifdef MS_WIN32
static PyObject *
BSTR_set(void *ptr, PyObject *value, Py_ssize_t size)
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
BSTR_get(void *ptr, Py_ssize_t size)
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

static PyObject *
P_set(void *ptr, PyObject *value, Py_ssize_t size)
{
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
    if (*(void **)ptr == NULL) {
        Py_RETURN_NONE;
    }
    return PyLong_FromVoidPtr(*(void **)ptr);
}

static struct fielddesc formattable[] = {
    { 's', s_set, s_get, &ffi_type_pointer},
    { 'b', b_set, b_get, &ffi_type_schar},
    { 'B', B_set, B_get, &ffi_type_uchar},
    { 'c', c_set, c_get, &ffi_type_schar},
    { 'd', d_set, d_get, &ffi_type_double, d_set_sw, d_get_sw},
    { 'g', g_set, g_get, &ffi_type_longdouble},
    { 'f', f_set, f_get, &ffi_type_float, f_set_sw, f_get_sw},
    { 'h', h_set, h_get, &ffi_type_sshort, h_set_sw, h_get_sw},
    { 'H', H_set, H_get, &ffi_type_ushort, H_set_sw, H_get_sw},
    { 'i', i_set, i_get, &ffi_type_sint, i_set_sw, i_get_sw},
    { 'I', I_set, I_get, &ffi_type_uint, I_set_sw, I_get_sw},
/* XXX Hm, sizeof(int) == sizeof(long) doesn't hold on every platform */
/* As soon as we can get rid of the type codes, this is no longer a problem */
#if SIZEOF_LONG == 4
    { 'l', l_set, l_get, &ffi_type_sint32, l_set_sw, l_get_sw},
    { 'L', L_set, L_get, &ffi_type_uint32, L_set_sw, L_get_sw},
#elif SIZEOF_LONG == 8
    { 'l', l_set, l_get, &ffi_type_sint64, l_set_sw, l_get_sw},
    { 'L', L_set, L_get, &ffi_type_uint64, L_set_sw, L_get_sw},
#else
# error
#endif
#if SIZEOF_LONG_LONG == 8
    { 'q', q_set, q_get, &ffi_type_sint64, q_set_sw, q_get_sw},
    { 'Q', Q_set, Q_get, &ffi_type_uint64, Q_set_sw, Q_get_sw},
#else
# error
#endif
    { 'P', P_set, P_get, &ffi_type_pointer},
    { 'z', z_set, z_get, &ffi_type_pointer},
#ifdef CTYPES_UNICODE
    { 'u', u_set, u_get, NULL}, /* ffi_type set later */
    { 'U', U_set, U_get, &ffi_type_pointer},
    { 'Z', Z_set, Z_get, &ffi_type_pointer},
#endif
#ifdef MS_WIN32
    { 'X', BSTR_set, BSTR_get, &ffi_type_pointer},
    { 'v', vBOOL_set, vBOOL_get, &ffi_type_sshort},
#endif
#if SIZEOF__BOOL == 1
    { '?', bool_set, bool_get, &ffi_type_uchar}, /* Also fallback for no native _Bool support */
#elif SIZEOF__BOOL == SIZEOF_SHORT
    { '?', bool_set, bool_get, &ffi_type_ushort},
#elif SIZEOF__BOOL == SIZEOF_INT
    { '?', bool_set, bool_get, &ffi_type_uint, I_set_sw, I_get_sw},
#elif SIZEOF__BOOL == SIZEOF_LONG
    { '?', bool_set, bool_get, &ffi_type_ulong, L_set_sw, L_get_sw},
#elif SIZEOF__BOOL == SIZEOF_LONG_LONG
    { '?', bool_set, bool_get, &ffi_type_ulong, Q_set_sw, Q_get_sw},
#endif /* SIZEOF__BOOL */
    { 'O', O_set, O_get, &ffi_type_pointer},
    { 0, NULL, NULL, NULL},
};

/*
  Ideas: Implement VARIANT in this table, using 'V' code.
  Use '?' as code for BOOL.
*/

struct fielddesc *
_ctypes_get_fielddesc(const char *fmt)
{
    static int initialized = 0;
    struct fielddesc *table = formattable;

    if (!initialized) {
        initialized = 1;
#ifdef CTYPES_UNICODE
        if (sizeof(wchar_t) == sizeof(short))
            _ctypes_get_fielddesc("u")->pffi_type = &ffi_type_sshort;
        else if (sizeof(wchar_t) == sizeof(int))
            _ctypes_get_fielddesc("u")->pffi_type = &ffi_type_sint;
        else if (sizeof(wchar_t) == sizeof(long))
            _ctypes_get_fielddesc("u")->pffi_type = &ffi_type_slong;
#endif
    }

    for (; table->code; ++table) {
        if (table->code == fmt[0])
            return table;
    }
    return NULL;
}

typedef struct { char c; char x; } s_char;
typedef struct { char c; short x; } s_short;
typedef struct { char c; int x; } s_int;
typedef struct { char c; long x; } s_long;
typedef struct { char c; float x; } s_float;
typedef struct { char c; double x; } s_double;
typedef struct { char c; long double x; } s_long_double;
typedef struct { char c; char *x; } s_char_p;
typedef struct { char c; void *x; } s_void_p;

/*
#define CHAR_ALIGN (sizeof(s_char) - sizeof(char))
#define SHORT_ALIGN (sizeof(s_short) - sizeof(short))
#define LONG_ALIGN (sizeof(s_long) - sizeof(long))
*/
#define INT_ALIGN (sizeof(s_int) - sizeof(int))
#define FLOAT_ALIGN (sizeof(s_float) - sizeof(float))
#define DOUBLE_ALIGN (sizeof(s_double) - sizeof(double))
#define LONGDOUBLE_ALIGN (sizeof(s_long_double) - sizeof(long double))

/* #define CHAR_P_ALIGN (sizeof(s_char_p) - sizeof(char*)) */
#define VOID_P_ALIGN (sizeof(s_void_p) - sizeof(void*))

/*
#ifdef HAVE_USABLE_WCHAR_T
typedef struct { char c; wchar_t x; } s_wchar;
typedef struct { char c; wchar_t *x; } s_wchar_p;

#define WCHAR_ALIGN (sizeof(s_wchar) - sizeof(wchar_t))
#define WCHAR_P_ALIGN (sizeof(s_wchar_p) - sizeof(wchar_t*))
#endif
*/

typedef struct { char c; long long x; } s_long_long;
#define LONG_LONG_ALIGN (sizeof(s_long_long) - sizeof(long long))

/* from ffi.h:
typedef struct _ffi_type
{
    size_t size;
    unsigned short alignment;
    unsigned short type;
    struct _ffi_type **elements;
} ffi_type;
*/

/* align and size are bogus for void, but they must not be zero */
ffi_type ffi_type_void = { 1, 1, FFI_TYPE_VOID };

ffi_type ffi_type_uint8 = { 1, 1, FFI_TYPE_UINT8 };
ffi_type ffi_type_sint8 = { 1, 1, FFI_TYPE_SINT8 };

ffi_type ffi_type_uint16 = { 2, 2, FFI_TYPE_UINT16 };
ffi_type ffi_type_sint16 = { 2, 2, FFI_TYPE_SINT16 };

ffi_type ffi_type_uint32 = { 4, INT_ALIGN, FFI_TYPE_UINT32 };
ffi_type ffi_type_sint32 = { 4, INT_ALIGN, FFI_TYPE_SINT32 };

ffi_type ffi_type_uint64 = { 8, LONG_LONG_ALIGN, FFI_TYPE_UINT64 };
ffi_type ffi_type_sint64 = { 8, LONG_LONG_ALIGN, FFI_TYPE_SINT64 };

ffi_type ffi_type_float = { sizeof(float), FLOAT_ALIGN, FFI_TYPE_FLOAT };
ffi_type ffi_type_double = { sizeof(double), DOUBLE_ALIGN, FFI_TYPE_DOUBLE };

#ifdef ffi_type_longdouble
#undef ffi_type_longdouble
#endif
  /* This is already defined on OSX */
ffi_type ffi_type_longdouble = { sizeof(long double), LONGDOUBLE_ALIGN,
                                 FFI_TYPE_LONGDOUBLE };

ffi_type ffi_type_pointer = { sizeof(void *), VOID_P_ALIGN, FFI_TYPE_POINTER };

/*---------------- EOF ----------------*/
