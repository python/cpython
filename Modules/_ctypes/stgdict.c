#include "Python.h"
#include <ffi.h>
#ifdef MS_WIN32
#include <windows.h>
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
StgDict_init(StgDictObject *self, PyObject *args, PyObject *kwds)
{
	if (PyDict_Type.tp_init((PyObject *)self, args, kwds) < 0)
		return -1;
	return 0;
}

static int
StgDict_clear(StgDictObject *self)
{
	Py_CLEAR(self->proto);
	Py_CLEAR(self->argtypes);
	Py_CLEAR(self->converters);
	Py_CLEAR(self->restype);
	Py_CLEAR(self->checker);
	return 0;
}

static void
StgDict_dealloc(StgDictObject *self)
{
	StgDict_clear(self);
	PyMem_Free(self->ffi_type_pointer.elements);
	PyDict_Type.tp_dealloc((PyObject *)self);
}

int
StgDict_clone(StgDictObject *dst, StgDictObject *src)
{
	char *d, *s;
	int size;

	StgDict_clear(dst);
	PyMem_Free(dst->ffi_type_pointer.elements);
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

	if (src->ffi_type_pointer.elements == NULL)
		return 0;
	size = sizeof(ffi_type *) * (src->length + 1);
	dst->ffi_type_pointer.elements = PyMem_Malloc(size);
	if (dst->ffi_type_pointer.elements == NULL)
		return -1;
	memcpy(dst->ffi_type_pointer.elements,
	       src->ffi_type_pointer.elements,
	       size);
	return 0;
}

PyTypeObject StgDict_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"StgDict",
	sizeof(StgDictObject),
	0,
	(destructor)StgDict_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	0,					/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	(initproc)StgDict_init,			/* tp_init */
	0,					/* tp_alloc */
	0,					/* tp_new */
	0,					/* tp_free */
};

/* May return NULL, but does not set an exception! */
StgDictObject *
PyType_stgdict(PyObject *obj)
{
	PyTypeObject *type;

	if (!PyType_Check(obj))
		return NULL;
	type = (PyTypeObject *)obj;
	if (!type->tp_dict || !StgDict_Check(type->tp_dict))
		return NULL;
	return (StgDictObject *)type->tp_dict;
}

/* May return NULL, but does not set an exception! */
StgDictObject *
PyObject_stgdict(PyObject *self)
{
	return PyType_stgdict((PyObject *)self->ob_type);
}

#if 0
/* work in progress: anonymous structure fields */
int
GetFields(PyObject *desc, int *pindex, int *psize, int *poffset, int *palign, int pack);

{
	int i;
	PyObject *tuples = PyObject_GetAttrString(desc, "_fields_");
	if (tuples == NULL)
		return -1;
	if (!PyTuple_Check(tuples))
		return -1; /* leak */
	for (i = 0; i < PyTuple_GET_SIZE(tuples); ++i) {
		char *fname;
		PyObject *dummy;
		CFieldObject *field;
		PyObject *pair = PyTuple_GET_ITEM(tuples, i);
		if (!PyArg_ParseTuple(pair, "sO", &fname, &dummy))
			return -1; /* leak */
		field = PyObject_GetAttrString(desc, fname);
		Py_DECREF(field);
	}
}
#endif

/*
  Retrive the (optional) _pack_ attribute from a type, the _fields_ attribute,
  and create an StgDictObject.  Used for Structure and Union subclasses.
*/
int
StructUnionType_update_stgdict(PyObject *type, PyObject *fields, int isStruct)
{
	StgDictObject *stgdict, *basedict;
	int len, offset, size, align, i;
	int union_size, total_align;
	int field_size = 0;
	int bitofs;
	PyObject *isPacked;
	int pack = 0;
	int ffi_ofs;
	int big_endian;

	/* HACK Alert: I cannot be bothered to fix ctypes.com, so there has to
	   be a way to use the old, broken sematics: _fields_ are not extended
	   but replaced in subclasses.
	   
	   XXX Remove this in ctypes 1.0!
	*/
	int use_broken_old_ctypes_semantics;

	if (fields == NULL)
		return 0;

#ifdef WORDS_BIGENDIAN
	big_endian = PyObject_HasAttrString(type, "_swappedbytes_") ? 0 : 1;
#else
	big_endian = PyObject_HasAttrString(type, "_swappedbytes_") ? 1 : 0;
#endif

	use_broken_old_ctypes_semantics = \
		PyObject_HasAttrString(type, "_use_broken_old_ctypes_structure_semantics_");

	isPacked = PyObject_GetAttrString(type, "_pack_");
	if (isPacked) {
		pack = PyInt_AsLong(isPacked);
		if (pack < 0 || PyErr_Occurred()) {
			Py_XDECREF(isPacked);
			PyErr_SetString(PyExc_ValueError,
					"_pack_ must be a non-negative integer");
			return -1;
		}
		Py_DECREF(isPacked);
	} else
		PyErr_Clear();

	len = PySequence_Length(fields);
	if (len == -1) {
		PyErr_SetString(PyExc_TypeError,
				"'_fields_' must be a sequence of pairs");
		return -1;
	}

	stgdict = PyType_stgdict(type);
	if (!stgdict)
		return -1;
	if (stgdict->flags & DICTFLAG_FINAL) {/* is final ? */
		PyErr_SetString(PyExc_AttributeError,
				"_fields_ is final");
		return -1;
	}
	/* XXX This should probably be moved to a point when all this
	   stuff is sucessfully finished. */
	stgdict->flags |= DICTFLAG_FINAL;	/* set final */

	if (stgdict->ffi_type_pointer.elements)
		PyMem_Free(stgdict->ffi_type_pointer.elements);

	basedict = PyType_stgdict((PyObject *)((PyTypeObject *)type)->tp_base);
	if (basedict && !use_broken_old_ctypes_semantics) {
		size = offset = basedict->size;
		align = basedict->align;
		union_size = 0;
		total_align = align ? align : 1;
		stgdict->ffi_type_pointer.type = FFI_TYPE_STRUCT;
		stgdict->ffi_type_pointer.elements = PyMem_Malloc(sizeof(ffi_type *) * (basedict->length + len + 1));
		memset(stgdict->ffi_type_pointer.elements, 0,
		       sizeof(ffi_type *) * (basedict->length + len + 1));
		memcpy(stgdict->ffi_type_pointer.elements,
		       basedict->ffi_type_pointer.elements,
		       sizeof(ffi_type *) * (basedict->length));
		ffi_ofs = basedict->length;
	} else {
		offset = 0;
		size = 0;
		align = 0;
		union_size = 0;
		total_align = 1;
		stgdict->ffi_type_pointer.type = FFI_TYPE_STRUCT;
		stgdict->ffi_type_pointer.elements = PyMem_Malloc(sizeof(ffi_type *) * (len + 1));
		memset(stgdict->ffi_type_pointer.elements, 0,
		       sizeof(ffi_type *) * (len + 1));
		ffi_ofs = 0;
	}

#define realdict ((PyObject *)&stgdict->dict)
	for (i = 0; i < len; ++i) {
		PyObject *name = NULL, *desc = NULL;
		PyObject *pair = PySequence_GetItem(fields, i);
		PyObject *prop;
		StgDictObject *dict;
		int bitsize = 0;

		if (!pair || !PyArg_ParseTuple(pair, "OO|i", &name, &desc, &bitsize)) {
			PyErr_SetString(PyExc_AttributeError,
					"'_fields_' must be a sequence of pairs");
			Py_XDECREF(pair);
			return -1;
		}
		dict = PyType_stgdict(desc);
		if (dict == NULL) {
			Py_DECREF(pair);
			PyErr_Format(PyExc_TypeError,
				     "second item in _fields_ tuple (index %d) must be a C type",
				     i);
			return -1;
		}
		stgdict->ffi_type_pointer.elements[ffi_ofs + i] = &dict->ffi_type_pointer;
		dict->flags |= DICTFLAG_FINAL; /* mark field type final */
		if (PyTuple_Size(pair) == 3) { /* bits specified */
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
				if (dict->getfunc != getentry("c")->getfunc
#ifdef CTYPES_UNICODE
				    && dict->getfunc != getentry("u")->getfunc
#endif
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
			prop = CField_FromDesc(desc, i,
					       &field_size, bitsize, &bitofs,
					       &size, &offset, &align,
					       pack, big_endian);
		} else /* union */ {
			size = 0;
			offset = 0;
			align = 0;
			prop = CField_FromDesc(desc, i,
					       &field_size, bitsize, &bitofs,
					       &size, &offset, &align,
					       pack, big_endian);
			union_size = max(size, union_size);
		}
		total_align = max(align, total_align);

		if (!prop) {
			Py_DECREF(pair);
			Py_DECREF((PyObject *)stgdict);
			return -1;
		}
		if (-1 == PyDict_SetItem(realdict, name, prop)) {
			Py_DECREF(prop);
			Py_DECREF(pair);
			Py_DECREF((PyObject *)stgdict);
			return -1;
		}
		Py_DECREF(pair);
		Py_DECREF(prop);
	}
#undef realdict
	if (!isStruct)
		size = union_size;

	/* Adjust the size according to the alignment requirements */
	size = ((size + total_align - 1) / total_align) * total_align;

	stgdict->ffi_type_pointer.alignment = total_align;
	stgdict->ffi_type_pointer.size = size;

	stgdict->size = size;
	stgdict->align = total_align;
	stgdict->length = len;	/* ADD ffi_ofs? */
	return 0;
}
