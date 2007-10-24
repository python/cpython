#include "Python.h"

#include <ffi.h>
#ifdef MS_WIN32
#include <windows.h>
#endif
#include "ctypes.h"

/******************************************************************/
/*
  CField_Type
*/
static PyObject *
CField_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
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
CField_FromDesc(PyObject *desc, Py_ssize_t index,
		Py_ssize_t *pfield_size, int bitsize, int *pbitofs,
		Py_ssize_t *psize, Py_ssize_t *poffset, Py_ssize_t *palign,
		int pack, int big_endian)
{
	CFieldObject *self;
	PyObject *proto;
	Py_ssize_t size, align, length;
	SETFUNC setfunc = NULL;
	GETFUNC getfunc = NULL;
	StgDictObject *dict;
	int fieldtype;
#define NO_BITFIELD 0
#define NEW_BITFIELD 1
#define CONT_BITFIELD 2
#define EXPAND_BITFIELD 3

	self = (CFieldObject *)PyObject_CallObject((PyObject *)&CField_Type,
						   NULL);
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
#if defined(MS_WIN32) && !defined(__MINGW32__)
	    && dict->size * 8 == *pfield_size /* MSVC */
#else
	    && dict->size * 8 <= *pfield_size /* GCC, MINGW */
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
	length = dict->length;
	proto = desc;

	/*  Field descriptors for 'c_char * n' are be scpecial cased to
	    return a Python string instead of an Array object instance...
	*/
	if (ArrayTypeObject_Check(proto)) {
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
			if (idict->getfunc == getentry("c")->getfunc) {
				struct fielddesc *fd = getentry("s");
				getfunc = fd->getfunc;
				setfunc = fd->setfunc;
			}
#ifdef CTYPES_UNICODE
			if (idict->getfunc == getentry("u")->getfunc) {
				struct fielddesc *fd = getentry("U");
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
		/* XXX needs more */
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
CField_set(CFieldObject *self, PyObject *inst, PyObject *value)
{
	CDataObject *dst;
	char *ptr;
	assert(CDataObject_Check(inst));
	dst = (CDataObject *)inst;
	ptr = dst->b_ptr + self->offset;
	return CData_set(inst, self->proto, self->setfunc, value,
			 self->index, self->size, ptr);
}

static PyObject *
CField_get(CFieldObject *self, PyObject *inst, PyTypeObject *type)
{
	CDataObject *src;
	if (inst == NULL) {
		Py_INCREF(self);
		return (PyObject *)self;
	}
	assert(CDataObject_Check(inst));
	src = (CDataObject *)inst;
	return CData_get(self->proto, self->getfunc, inst,
			 self->index, self->size, src->b_ptr + self->offset);
}

static PyObject *
CField_get_offset(PyObject *self, void *data)
{
	return PyInt_FromSsize_t(((CFieldObject *)self)->offset);
}

static PyObject *
CField_get_size(PyObject *self, void *data)
{
	return PyInt_FromSsize_t(((CFieldObject *)self)->size);
}

static PyGetSetDef CField_getset[] = {
	{ "offset", CField_get_offset, NULL, "offset in bytes of this field" },
	{ "size", CField_get_size, NULL, "size in bytes of this field" },
	{ NULL, NULL, NULL, NULL },
};

static int
CField_traverse(CFieldObject *self, visitproc visit, void *arg)
{
	Py_VISIT(self->proto);
	return 0;
}

static int
CField_clear(CFieldObject *self)
{
	Py_CLEAR(self->proto);
	return 0;
}

static void
CField_dealloc(PyObject *self)
{
	CField_clear((CFieldObject *)self);
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *
CField_repr(CFieldObject *self)
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

PyTypeObject CField_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes.CField",				/* tp_name */
	sizeof(CFieldObject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	CField_dealloc,				/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)CField_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
	"Structure/Union member",		/* tp_doc */
	(traverseproc)CField_traverse,		/* tp_traverse */
	(inquiry)CField_clear,			/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	0,					/* tp_members */
	CField_getset,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	(descrgetfunc)CField_get,		/* tp_descr_get */
	(descrsetfunc)CField_set,		/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	CField_new,				/* tp_new */
	0,					/* tp_free */
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
	if (!PyInt_Check(v)) {
		PyErr_Format(PyExc_TypeError,
			     "int expected instead of %s instance",
			     v->ob_type->tp_name);
		return -1;
	}
	x = PyInt_AsUnsignedLongMask(v);
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
	if (!PyInt_Check(v)) {
		PyErr_Format(PyExc_TypeError,
			     "int expected instead of %s instance",
			     v->ob_type->tp_name);
		return -1;
	}
	x = PyInt_AsUnsignedLongMask(v);
	if (x == -1 && PyErr_Occurred())
		return -1;
	*p = x;
	return 0;
}

#ifdef HAVE_LONG_LONG

/* Same, but handling native long long. */

static int
get_longlong(PyObject *v, PY_LONG_LONG *p)
{
	PY_LONG_LONG x;
	if (!PyInt_Check(v)) {
		PyErr_Format(PyExc_TypeError,
			     "int expected instead of %s instance",
			     v->ob_type->tp_name);
		return -1;
	}
	x = PyInt_AsUnsignedLongLongMask(v);
	if (x == -1 && PyErr_Occurred())
		return -1;
	*p = x;
	return 0;
}

/* Same, but handling native unsigned long long. */

static int
get_ulonglong(PyObject *v, unsigned PY_LONG_LONG *p)
{
	unsigned PY_LONG_LONG x;
	if (!PyInt_Check(v)) {
		PyErr_Format(PyExc_TypeError,
			     "int expected instead of %s instance",
			     v->ob_type->tp_name);
		return -1;
	}
	x = PyInt_AsUnsignedLongLongMask(v);
	if (x == -1 && PyErr_Occurred())
		return -1;
	*p = x;
	return 0;
}

#endif

/*****************************************************************
 * Integer fields, with bitfield support
 */

/* how to decode the size field, for integer get/set functions */
#define LOW_BIT(x)  ((x) & 0xFFFF)
#define NUM_BITS(x) ((x) >> 16)

/* This seems nore a compiler issue than a Windows/non-Windows one */
#ifdef MS_WIN32
#  define BIT_MASK(size) ((1 << NUM_BITS(size))-1)
#else
#  define BIT_MASK(size) ((1LL << NUM_BITS(size))-1)
#endif

/* This macro CHANGES the first parameter IN PLACE. For proper sign handling,
   we must first shift left, then right.
*/
#define GET_BITFIELD(v, size)						\
	if (NUM_BITS(size)) {						\
		v <<= (sizeof(v)*8 - LOW_BIT(size) - NUM_BITS(size));	\
		v >>= (sizeof(v)*8 - NUM_BITS(size));			\
	}

/* This macro RETURNS the first parameter with the bit field CHANGED. */
#define SET(x, v, size)							\
	(NUM_BITS(size) ?						\
	 ( ( x & ~(BIT_MASK(size) << LOW_BIT(size)) ) | ( (v & BIT_MASK(size)) << LOW_BIT(size) ) ) \
	 : v)

/* byte swapping macros */
#define SWAP_2(v)				\
	( ( (v >> 8) & 0x00FF) |		\
	  ( (v << 8) & 0xFF00) )

#define SWAP_4(v)			\
	( ( (v & 0x000000FF) << 24 ) |  \
	  ( (v & 0x0000FF00) <<  8 ) |  \
	  ( (v & 0x00FF0000) >>  8 ) |  \
	  ( ((v >> 24) & 0xFF)) )

#ifdef _MSC_VER
#define SWAP_8(v)				\
	( ( (v & 0x00000000000000FFL) << 56 ) |  \
	  ( (v & 0x000000000000FF00L) << 40 ) |  \
	  ( (v & 0x0000000000FF0000L) << 24 ) |  \
	  ( (v & 0x00000000FF000000L) <<  8 ) |  \
	  ( (v & 0x000000FF00000000L) >>  8 ) |  \
	  ( (v & 0x0000FF0000000000L) >> 24 ) |  \
	  ( (v & 0x00FF000000000000L) >> 40 ) |  \
	  ( ((v >> 56) & 0xFF)) )
#else
#define SWAP_8(v)				\
	( ( (v & 0x00000000000000FFLL) << 56 ) |  \
	  ( (v & 0x000000000000FF00LL) << 40 ) |  \
	  ( (v & 0x0000000000FF0000LL) << 24 ) |  \
	  ( (v & 0x00000000FF000000LL) <<  8 ) |  \
	  ( (v & 0x000000FF00000000LL) >>  8 ) |  \
	  ( (v & 0x0000FF0000000000LL) >> 24 ) |  \
	  ( (v & 0x00FF000000000000LL) >> 40 ) |  \
	  ( ((v >> 56) & 0xFF)) )
#endif

#define SWAP_INT SWAP_4

#if SIZEOF_LONG == 4
# define SWAP_LONG SWAP_4
#elif SIZEOF_LONG == 8
# define SWAP_LONG SWAP_8
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
#define _RET(X) Py_INCREF(Py_None); return Py_None
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
	*(signed char *)ptr = (signed char)SET(*(signed char *)ptr, (signed char)val, size);
	_RET(value);
}


static PyObject *
b_get(void *ptr, Py_ssize_t size)
{
	signed char val = *(signed char *)ptr;
	GET_BITFIELD(val, size);
	return PyInt_FromLong(val);
}

static PyObject *
B_set(void *ptr, PyObject *value, Py_ssize_t size)
{
	unsigned long val;
	if (get_ulong(value, &val) < 0)
		return NULL;
	*(unsigned char *)ptr = (unsigned char)SET(*(unsigned char*)ptr,
						   (unsigned short)val, size);
	_RET(value);
}


static PyObject *
B_get(void *ptr, Py_ssize_t size)
{
	unsigned char val = *(unsigned char *)ptr;
	GET_BITFIELD(val, size);
	return PyInt_FromLong(val);
}

static PyObject *
h_set(void *ptr, PyObject *value, Py_ssize_t size)
{
	long val;
	short x;
	if (get_long(value, &val) < 0)
		return NULL;
	memcpy(&x, ptr, sizeof(x));
	x = SET(x, (short)val, size);
	memcpy(ptr, &x, sizeof(x));
	_RET(value);
}


static PyObject *
h_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
	long val;
	short field;
	if (get_long(value, &val) < 0)
		return NULL;
	memcpy(&field, ptr, sizeof(field));
	field = SWAP_2(field);
	field = SET(field, (short)val, size);
	field = SWAP_2(field);
	memcpy(ptr, &field, sizeof(field));
	_RET(value);
}

static PyObject *
h_get(void *ptr, Py_ssize_t size)
{
	short val;
	memcpy(&val, ptr, sizeof(val));
	GET_BITFIELD(val, size);
	return PyInt_FromLong((long)val);
}

static PyObject *
h_get_sw(void *ptr, Py_ssize_t size)
{
	short val;
	memcpy(&val, ptr, sizeof(val));
	val = SWAP_2(val);
	GET_BITFIELD(val, size);
	return PyInt_FromLong(val);
}

static PyObject *
H_set(void *ptr, PyObject *value, Py_ssize_t size)
{
	unsigned long val;
	unsigned short x;
	if (get_ulong(value, &val) < 0)
		return NULL;
	memcpy(&x, ptr, sizeof(x));
	x = SET(x, (unsigned short)val, size);
	memcpy(ptr, &x, sizeof(x));
	_RET(value);
}

static PyObject *
H_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
	unsigned long val;
	unsigned short field;
	if (get_ulong(value, &val) < 0)
		return NULL;
	memcpy(&field, ptr, sizeof(field));
	field = SWAP_2(field);
	field = SET(field, (unsigned short)val, size);
	field = SWAP_2(field);
	memcpy(ptr, &field, sizeof(field));
	_RET(value);
}


static PyObject *
H_get(void *ptr, Py_ssize_t size)
{
	unsigned short val;
	memcpy(&val, ptr, sizeof(val));
	GET_BITFIELD(val, size);
	return PyInt_FromLong(val);
}

static PyObject *
H_get_sw(void *ptr, Py_ssize_t size)
{
	unsigned short val;
	memcpy(&val, ptr, sizeof(val));
	val = SWAP_2(val);
	GET_BITFIELD(val, size);
	return PyInt_FromLong(val);
}

static PyObject *
i_set(void *ptr, PyObject *value, Py_ssize_t size)
{
	long val;
	int x;
	if (get_long(value, &val) < 0)
		return NULL;
	memcpy(&x, ptr, sizeof(x));
	x = SET(x, (int)val, size);
	memcpy(ptr, &x, sizeof(x));
	_RET(value);
}

static PyObject *
i_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
	long val;
	int field;
	if (get_long(value, &val) < 0)
		return NULL;
	memcpy(&field, ptr, sizeof(field));
	field = SWAP_INT(field);
	field = SET(field, (int)val, size);
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
	return PyInt_FromLong(val);
}

static PyObject *
i_get_sw(void *ptr, Py_ssize_t size)
{
	int val;
	memcpy(&val, ptr, sizeof(val));
	val = SWAP_INT(val);
	GET_BITFIELD(val, size);
	return PyInt_FromLong(val);
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

#ifdef HAVE_C99_BOOL
#define BOOL_TYPE _Bool
#else
#define BOOL_TYPE char
#undef SIZEOF__BOOL
#define SIZEOF__BOOL 1
#endif

static PyObject *
t_set(void *ptr, PyObject *value, Py_ssize_t size)
{
	switch (PyObject_IsTrue(value)) {
	case -1:
		return NULL;
	case 0:
		*(BOOL_TYPE *)ptr = 0;
		_RET(value);
	default:
		*(BOOL_TYPE *)ptr = 1;
		_RET(value);
	}
}

static PyObject *
t_get(void *ptr, Py_ssize_t size)
{
	return PyBool_FromLong((long)*(BOOL_TYPE *)ptr);
}

static PyObject *
I_set(void *ptr, PyObject *value, Py_ssize_t size)
{
	unsigned long val;
	unsigned int x;
	if (get_ulong(value, &val) < 0)
		return  NULL;
	memcpy(&x, ptr, sizeof(x));
	x = SET(x, (unsigned int)val, size);
	memcpy(ptr, &x, sizeof(x));
	_RET(value);
}

static PyObject *
I_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
	unsigned long val;
	unsigned int field;
	if (get_ulong(value, &val) < 0)
		return  NULL;
	memcpy(&field, ptr, sizeof(field));
	field = (unsigned int)SET(field, (unsigned int)val, size);
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
	x = SET(x, val, size);
	memcpy(ptr, &x, sizeof(x));
	_RET(value);
}

static PyObject *
l_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
	long val;
	long field;
	if (get_long(value, &val) < 0)
		return NULL;
	memcpy(&field, ptr, sizeof(field));
	field = SWAP_LONG(field);
	field = (long)SET(field, val, size);
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
	return PyInt_FromLong(val);
}

static PyObject *
l_get_sw(void *ptr, Py_ssize_t size)
{
	long val;
	memcpy(&val, ptr, sizeof(val));
	val = SWAP_LONG(val);
	GET_BITFIELD(val, size);
	return PyInt_FromLong(val);
}

static PyObject *
L_set(void *ptr, PyObject *value, Py_ssize_t size)
{
	unsigned long val;
	unsigned long x;
	if (get_ulong(value, &val) < 0)
		return  NULL;
	memcpy(&x, ptr, sizeof(x));
	x = SET(x, val, size);
	memcpy(ptr, &x, sizeof(x));
	_RET(value);
}

static PyObject *
L_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
	unsigned long val;
	unsigned long field;
	if (get_ulong(value, &val) < 0)
		return  NULL;
	memcpy(&field, ptr, sizeof(field));
	field = SWAP_LONG(field);
	field = (unsigned long)SET(field, val, size);
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

#ifdef HAVE_LONG_LONG
static PyObject *
q_set(void *ptr, PyObject *value, Py_ssize_t size)
{
	PY_LONG_LONG val;
	PY_LONG_LONG x;
	if (get_longlong(value, &val) < 0)
		return NULL;
	memcpy(&x, ptr, sizeof(x));
	x = SET(x, val, size);
	memcpy(ptr, &x, sizeof(x));
	_RET(value);
}

static PyObject *
q_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
	PY_LONG_LONG val;
	PY_LONG_LONG field;
	if (get_longlong(value, &val) < 0)
		return NULL;
	memcpy(&field, ptr, sizeof(field));
	field = SWAP_8(field);
	field = (PY_LONG_LONG)SET(field, val, size);
	field = SWAP_8(field);
	memcpy(ptr, &field, sizeof(field));
	_RET(value);
}

static PyObject *
q_get(void *ptr, Py_ssize_t size)
{
	PY_LONG_LONG val;
	memcpy(&val, ptr, sizeof(val));
	GET_BITFIELD(val, size);
	return PyLong_FromLongLong(val);
}

static PyObject *
q_get_sw(void *ptr, Py_ssize_t size)
{
	PY_LONG_LONG val;
	memcpy(&val, ptr, sizeof(val));
	val = SWAP_8(val);
	GET_BITFIELD(val, size);
	return PyLong_FromLongLong(val);
}

static PyObject *
Q_set(void *ptr, PyObject *value, Py_ssize_t size)
{
	unsigned PY_LONG_LONG val;
	unsigned PY_LONG_LONG x;
	if (get_ulonglong(value, &val) < 0)
		return NULL;
	memcpy(&x, ptr, sizeof(x));
	x = SET(x, val, size);
	memcpy(ptr, &x, sizeof(x));
	_RET(value);
}

static PyObject *
Q_set_sw(void *ptr, PyObject *value, Py_ssize_t size)
{
	unsigned PY_LONG_LONG val;
	unsigned PY_LONG_LONG field;
	if (get_ulonglong(value, &val) < 0)
		return NULL;
	memcpy(&field, ptr, sizeof(field));
	field = SWAP_8(field);
	field = (unsigned PY_LONG_LONG)SET(field, val, size);
	field = SWAP_8(field);
	memcpy(ptr, &field, sizeof(field));
	_RET(value);
}

static PyObject *
Q_get(void *ptr, Py_ssize_t size)
{
	unsigned PY_LONG_LONG val;
	memcpy(&val, ptr, sizeof(val));
	GET_BITFIELD(val, size);
	return PyLong_FromUnsignedLongLong(val);
}

static PyObject *
Q_get_sw(void *ptr, Py_ssize_t size)
{
	unsigned PY_LONG_LONG val;
	memcpy(&val, ptr, sizeof(val));
	val = SWAP_8(val);
	GET_BITFIELD(val, size);
	return PyLong_FromUnsignedLongLong(val);
}
#endif

/*****************************************************************
 * non-integer accessor methods, not supporting bit fields
 */


static PyObject *
D_set(void *ptr, PyObject *value, Py_ssize_t size)
{
	long double x;

	x = PyFloat_AsDouble(value);
	if (x == -1 && PyErr_Occurred()) {
		PyErr_Format(PyExc_TypeError,
			     " float expected instead of %s instance",
			     value->ob_type->tp_name);
		return NULL;
	}
	memcpy(ptr, &x, sizeof(long double));
	_RET(value);
}

static PyObject *
D_get(void *ptr, Py_ssize_t size)
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
	if (x == -1 && PyErr_Occurred()) {
		PyErr_Format(PyExc_TypeError,
			     " float expected instead of %s instance",
			     value->ob_type->tp_name);
		return NULL;
	}
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
	if (x == -1 && PyErr_Occurred()) {
		PyErr_Format(PyExc_TypeError,
			     " float expected instead of %s instance",
			     value->ob_type->tp_name);
		return NULL;
	}
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
	if (x == -1 && PyErr_Occurred()) {
		PyErr_Format(PyExc_TypeError,
			     " float expected instead of %s instance",
			     value->ob_type->tp_name);
		return NULL;
	}
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
	if (x == -1 && PyErr_Occurred()) {
		PyErr_Format(PyExc_TypeError,
			     " float expected instead of %s instance",
			     value->ob_type->tp_name);
		return NULL;
	}
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
	if (PyUnicode_Check(value)) {
		value = PyUnicode_AsEncodedString(value,
						  conversion_mode_encoding,
						  conversion_mode_errors);
		if (value == NULL)
			return NULL;
		if (PyBytes_GET_SIZE(value) != 1) {
			Py_DECREF(value);
			goto error;
		}
		*(char *)ptr = PyBytes_AsString(value)[0];
		Py_DECREF(value);
		_RET(value);
	}
	if (PyBytes_Check(value) && PyBytes_GET_SIZE(value) == 1) {
		*(char *)ptr = PyBytes_AsString(value)[0];
		_RET(value);
	}
	if (PyInt_Check(value))
	{
		long longval = PyInt_AS_LONG(value);
		if (longval < 0 || longval >= 256)
			goto error;
		*(char *)ptr = (char)longval;
		_RET(value);
	}
  error:
	PyErr_Format(PyExc_TypeError,
		     "one character string expected");
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
	if (PyBytes_Check(value)) {
		value = PyUnicode_FromEncodedObject(value,
						    conversion_mode_encoding,
						    conversion_mode_errors);
		if (!value)
			return NULL;
	} else if (!PyUnicode_Check(value)) {
		PyErr_Format(PyExc_TypeError,
				"unicode string expected instead of %s instance",
				value->ob_type->tp_name);
		return NULL;
	} else
		Py_INCREF(value);

	len = PyUnicode_GET_SIZE(value);
	if (len != 1) {
		Py_DECREF(value);
		PyErr_SetString(PyExc_TypeError,
				"one character unicode string expected");
		return NULL;
	}

	*(wchar_t *)ptr = PyUnicode_AS_UNICODE(value)[0];
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
	PyObject *result;
	Py_ssize_t len;
	Py_UNICODE *p;

	size /= sizeof(wchar_t); /* we count character units here, not bytes */

	result = PyUnicode_FromWideChar((wchar_t *)ptr, size);
	if (!result)
		return NULL;
	/* We need 'result' to be able to count the characters with wcslen,
	   since ptr may not be NUL terminated.  If the length is smaller (if
	   it was actually NUL terminated, we construct a new one and throw
	   away the result.
	*/
	/* chop off at the first NUL character, if any. */
	p = PyUnicode_AS_UNICODE(result);
	for (len = 0; len < size; ++len)
		if (!p[len])
			break;

	if (len < size) {
		PyObject *ob = PyUnicode_FromWideChar((wchar_t *)ptr, len);
		Py_DECREF(result);
		return ob;
	}
	return result;
}

static PyObject *
U_set(void *ptr, PyObject *value, Py_ssize_t length)
{
	Py_ssize_t size;

	/* It's easier to calculate in characters than in bytes */
	length /= sizeof(wchar_t);

	if (PyBytes_Check(value)) {
		value = PyUnicode_FromEncodedObject(value,
						    conversion_mode_encoding,
						    conversion_mode_errors);
		if (!value)
			return NULL;
	} else if (!PyUnicode_Check(value)) {
		PyErr_Format(PyExc_TypeError,
				"unicode string expected instead of %s instance",
				value->ob_type->tp_name);
		return NULL;
	} else
		Py_INCREF(value);
	size = PyUnicode_GET_SIZE(value);
	if (size > length) {
		PyErr_Format(PyExc_ValueError,
			     "string too long (%zd, maximum length %zd)",
			     size, length);
		Py_DECREF(value);
		return NULL;
	} else if (size < length-1)
		/* copy terminating NUL character if there is space */
		size += 1;
	PyUnicode_AsWideChar((PyUnicodeObject *)value, (wchar_t *)ptr, size);
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

	return PyUnicode_FromStringAndSize((char *)ptr, (Py_ssize_t)i);
}

static PyObject *
s_set(void *ptr, PyObject *value, Py_ssize_t length)
{
	char *data;
	Py_ssize_t size;

	if (PyUnicode_Check(value)) {
		value = PyUnicode_AsEncodedString(value,
						  conversion_mode_encoding,
						  conversion_mode_errors);
		if (value == NULL)
			return NULL;
		assert(PyBytes_Check(value));
	} else if(PyBytes_Check(value)) {
		Py_INCREF(value);
	} else {
		PyErr_Format(PyExc_TypeError,
			     "expected string, %s found",
			     value->ob_type->tp_name);
		return NULL;
	}

	data = PyBytes_AsString(value);
	if (!data)
		return NULL;
	size = strlen(data);
	if (size < length) {
		/* This will copy the leading NUL character
		 * if there is space for it.
		 */
		++size;
	} else if (size > length) {
		PyErr_Format(PyExc_ValueError,
			     "string too long (%zd, maximum length %zd)",
			     size, length);
		Py_DECREF(value);
		return NULL;
	}
	/* Also copy the terminating NUL character if there is space */
	memcpy((char *)ptr, data, size);

	Py_DECREF(value);
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
		*(char **)ptr = PyBytes_AsString(value);
		Py_INCREF(value);
		return value;
	} else if (PyUnicode_Check(value)) {
		PyObject *str = PyUnicode_AsEncodedString(value,
							  conversion_mode_encoding,
							  conversion_mode_errors);
		if (str == NULL)
			return NULL;
		assert(PyBytes_Check(str));
		*(char **)ptr = PyBytes_AS_STRING(str);
		return str;
	} else if (PyInt_Check(value)) {
#if SIZEOF_VOID_P == SIZEOF_LONG_LONG
		*(char **)ptr = (char *)PyInt_AsUnsignedLongLongMask(value);
#else
		*(char **)ptr = (char *)PyInt_AsUnsignedLongMask(value);
#endif
		_RET(value);
	}
	PyErr_Format(PyExc_TypeError,
		     "string or integer address expected instead of %s instance",
		     value->ob_type->tp_name);
	return NULL;
}

static PyObject *
z_get(void *ptr, Py_ssize_t size)
{
	/* XXX What about invalid pointers ??? */
	if (*(void **)ptr) {
#if defined(MS_WIN32) && !defined(_WIN32_WCE)
		if (IsBadStringPtrA(*(char **)ptr, -1)) {
			PyErr_Format(PyExc_ValueError,
				     "invalid string pointer %p",
				     *(char **)ptr);
			return NULL;
		}
#endif
		return PyUnicode_FromString(*(char **)ptr);
	} else {
		Py_INCREF(Py_None);
		return Py_None;
	}
}

#ifdef CTYPES_UNICODE
static PyObject *
Z_set(void *ptr, PyObject *value, Py_ssize_t size)
{
	if (value == Py_None) {
		*(wchar_t **)ptr = NULL;
		Py_INCREF(value);
		return value;
	}
	if (PyInt_Check(value) || PyLong_Check(value)) {
#if SIZEOF_VOID_P == SIZEOF_LONG_LONG
		*(wchar_t **)ptr = (wchar_t *)PyInt_AsUnsignedLongLongMask(value);
#else
		*(wchar_t **)ptr = (wchar_t *)PyInt_AsUnsignedLongMask(value);
#endif
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (PyBytes_Check(value)) {
		value = PyUnicode_FromEncodedObject(value,
						    conversion_mode_encoding,
						    conversion_mode_errors);
		if (!value)
			return NULL;
	} else if (!PyUnicode_Check(value)) {
		PyErr_Format(PyExc_TypeError,
			     "unicode string or integer address expected instead of %s instance",
			     value->ob_type->tp_name);
		return NULL;
	} else
		Py_INCREF(value);
#ifdef HAVE_USABLE_WCHAR_T
	/* HAVE_USABLE_WCHAR_T means that Py_UNICODE and wchar_t is the same
	   type.  So we can copy directly.  Hm, are unicode objects always NUL
	   terminated in Python, internally?
	 */
	*(wchar_t **)ptr = PyUnicode_AS_UNICODE(value);
	return value;
#else
	{
		/* We must create a wchar_t* buffer from the unicode object,
		   and keep it alive */
		PyObject *keep;
		wchar_t *buffer;

		int size = PyUnicode_GET_SIZE(value);
		size += 1; /* terminating NUL */
		size *= sizeof(wchar_t);
		buffer = (wchar_t *)PyMem_Malloc(size);
		if (!buffer)
			return PyErr_NoMemory();
		memset(buffer, 0, size);
		keep = PyCObject_FromVoidPtr(buffer, PyMem_Free);
		if (!keep) {
			PyMem_Free(buffer);
			return NULL;
		}
		*(wchar_t **)ptr = (wchar_t *)buffer;
		if (-1 == PyUnicode_AsWideChar((PyUnicodeObject *)value,
					       buffer, PyUnicode_GET_SIZE(value))) {
			Py_DECREF(value);
			Py_DECREF(keep);
			return NULL;
		}
		Py_DECREF(value);
		return keep;
	}
#endif
}

static PyObject *
Z_get(void *ptr, Py_ssize_t size)
{
	wchar_t *p;
	p = *(wchar_t **)ptr;
	if (p) {
#if defined(MS_WIN32) && !defined(_WIN32_WCE)
		if (IsBadStringPtrW(*(wchar_t **)ptr, -1)) {
			PyErr_Format(PyExc_ValueError,
				     "invalid string pointer %p",
				     *(wchar_t **)ptr);
			return NULL;
		}
#endif
		return PyUnicode_FromWideChar(p, wcslen(p));
	} else {
		Py_INCREF(Py_None);
		return Py_None;
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
	} else if (PyBytes_Check(value)) {
		value = PyUnicode_FromEncodedObject(value,
						    conversion_mode_encoding,
						    conversion_mode_errors);
		if (!value)
			return NULL;
	} else if (PyUnicode_Check(value)) {
		Py_INCREF(value); /* for the descref below */
	} else {
		PyErr_Format(PyExc_TypeError,
				"unicode string expected instead of %s instance",
				value->ob_type->tp_name);
		return NULL;
	}

	/* create a BSTR from value */
	if (value) {
		Py_ssize_t size = PyUnicode_GET_SIZE(value);
		if ((unsigned) size != size) {
			PyErr_SetString(PyExc_ValueError, "String too long for BSTR");
			return NULL;
		}
		bstr = SysAllocStringLen(PyUnicode_AS_UNICODE(value),
					 (unsigned)size);
		Py_DECREF(value);
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
		Py_INCREF(Py_None);
		return Py_None;
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

	if (!PyInt_Check(value) && !PyLong_Check(value)) {
		PyErr_SetString(PyExc_TypeError,
				"cannot be converted to pointer");
		return NULL;
	}

#if SIZEOF_VOID_P <= SIZEOF_LONG
	v = (void *)PyInt_AsUnsignedLongMask(value);
#else
#ifndef HAVE_LONG_LONG
#   error "PyLong_AsVoidPtr: sizeof(void*) > sizeof(long), but no long long"
#elif SIZEOF_LONG_LONG < SIZEOF_VOID_P
#   error "PyLong_AsVoidPtr: sizeof(PY_LONG_LONG) < sizeof(void*)"
#endif
	v = (void *)PyInt_AsUnsignedLongLongMask(value);
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
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyLong_FromVoidPtr(*(void **)ptr);
}

static struct fielddesc formattable[] = {
	{ 's', s_set, s_get, &ffi_type_pointer},
	{ 'b', b_set, b_get, &ffi_type_schar},
	{ 'B', B_set, B_get, &ffi_type_uchar},
	{ 'c', c_set, c_get, &ffi_type_schar},
	{ 'd', d_set, d_get, &ffi_type_double, d_set_sw, d_get_sw},
	{ 'D', D_set, D_get, &ffi_type_longdouble},
	{ 'f', f_set, f_get, &ffi_type_float, f_set_sw, f_get_sw},
	{ 'h', h_set, h_get, &ffi_type_sshort, h_set_sw, h_get_sw},
	{ 'H', H_set, H_get, &ffi_type_ushort, H_set_sw, H_get_sw},
	{ 'i', i_set, i_get, &ffi_type_sint, i_set_sw, i_get_sw},
	{ 'I', I_set, I_get, &ffi_type_uint, I_set_sw, I_get_sw},
/* XXX Hm, sizeof(int) == sizeof(long) doesn't hold on every platform */
/* As soon as we can get rid of the type codes, this is no longer a problem */
#if SIZEOF_LONG == 4
	{ 'l', l_set, l_get, &ffi_type_sint, l_set_sw, l_get_sw},
	{ 'L', L_set, L_get, &ffi_type_uint, L_set_sw, L_get_sw},
#elif SIZEOF_LONG == 8
	{ 'l', l_set, l_get, &ffi_type_slong, l_set_sw, l_get_sw},
	{ 'L', L_set, L_get, &ffi_type_ulong, L_set_sw, L_get_sw},
#else
# error
#endif
#ifdef HAVE_LONG_LONG
	{ 'q', q_set, q_get, &ffi_type_slong, q_set_sw, q_get_sw},
	{ 'Q', Q_set, Q_get, &ffi_type_ulong, Q_set_sw, Q_get_sw},
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
	{ 't', t_set, t_get, &ffi_type_uchar}, /* Also fallback for no native _Bool support */
#elif SIZEOF__BOOL == SIZEOF_SHORT
	{ 't', t_set, t_get, &ffi_type_ushort},
#elif SIZEOF__BOOL == SIZEOF_INT
	{ 't', t_set, t_get, &ffi_type_uint, I_set_sw, I_get_sw},
#elif SIZEOF__BOOL == SIZEOF_LONG
	{ 't', t_set, t_get, &ffi_type_ulong, L_set_sw, L_get_sw},
#elif SIZEOF__BOOL == SIZEOF_LONG_LONG
	{ 't', t_set, t_get, &ffi_type_ulong, Q_set_sw, Q_get_sw},
#endif /* SIZEOF__BOOL */
	{ 'O', O_set, O_get, &ffi_type_pointer},
	{ 0, NULL, NULL, NULL},
};

/*
  Ideas: Implement VARIANT in this table, using 'V' code.
  Use '?' as code for BOOL.
*/

struct fielddesc *
getentry(const char *fmt)
{
	static int initialized = 0;
	struct fielddesc *table = formattable;

	if (!initialized) {
		initialized = 1;
#ifdef CTYPES_UNICODE
		if (sizeof(wchar_t) == sizeof(short))
			getentry("u")->pffi_type = &ffi_type_sshort;
		else if (sizeof(wchar_t) == sizeof(int))
			getentry("u")->pffi_type = &ffi_type_sint;
		else if (sizeof(wchar_t) == sizeof(long))
			getentry("u")->pffi_type = &ffi_type_slong;
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
#define INT_ALIGN (sizeof(s_int) - sizeof(int))
#define LONG_ALIGN (sizeof(s_long) - sizeof(long))
*/
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

#ifdef HAVE_LONG_LONG
typedef struct { char c; PY_LONG_LONG x; } s_long_long;
#define LONG_LONG_ALIGN (sizeof(s_long_long) - sizeof(PY_LONG_LONG))
#endif

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

ffi_type ffi_type_uint32 = { 4, 4, FFI_TYPE_UINT32 };
ffi_type ffi_type_sint32 = { 4, 4, FFI_TYPE_SINT32 };

ffi_type ffi_type_uint64 = { 8, LONG_LONG_ALIGN, FFI_TYPE_UINT64 };
ffi_type ffi_type_sint64 = { 8, LONG_LONG_ALIGN, FFI_TYPE_SINT64 };

ffi_type ffi_type_float = { sizeof(float), FLOAT_ALIGN, FFI_TYPE_FLOAT };
ffi_type ffi_type_double = { sizeof(double), DOUBLE_ALIGN, FFI_TYPE_DOUBLE };
ffi_type ffi_type_longdouble = { sizeof(long double), LONGDOUBLE_ALIGN,
				 FFI_TYPE_LONGDOUBLE };

/* ffi_type ffi_type_longdouble */

ffi_type ffi_type_pointer = { sizeof(void *), VOID_P_ALIGN, FFI_TYPE_POINTER };

/*---------------- EOF ----------------*/
