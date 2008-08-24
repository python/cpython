/*****************************************************************
  This file should be kept compatible with Python 2.3, see PEP 291.
 *****************************************************************/

#include "Python.h"
#include <ffi.h>
#ifdef MS_WIN32
#include <windows.h>
#include <malloc.h>
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
	self->format = NULL;
	self->ndim = 0;
	self->shape = NULL;
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
	PyMem_Free(self->format);
	PyMem_Free(self->shape);
	PyMem_Free(self->ffi_type_pointer.elements);
	PyDict_Type.tp_dealloc((PyObject *)self);
}

int
StgDict_clone(StgDictObject *dst, StgDictObject *src)
{
	char *d, *s;
	Py_ssize_t size;

	StgDict_clear(dst);
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
		if (dst->format == NULL)
			return -1;
		strcpy(dst->format, src->format);
	}
	if (src->shape) {
		dst->shape = PyMem_Malloc(sizeof(Py_ssize_t) * src->ndim);
		if (dst->shape == NULL)
			return -1;
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

PyTypeObject StgDict_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
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
	if (!PyType_HasFeature(type, Py_TPFLAGS_HAVE_CLASS))
		return NULL;
	if (!type->tp_dict || !StgDict_CheckExact(type->tp_dict))
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
	PyTypeObject *type = self->ob_type;
	if (!PyType_HasFeature(type, Py_TPFLAGS_HAVE_CLASS))
		return NULL;
	if (!type->tp_dict || !StgDict_CheckExact(type->tp_dict))
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
		if (Py_TYPE(fdescr) != &CField_Type) {
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
 		new_descr = (CFieldObject *)PyObject_CallObject((PyObject *)&CField_Type, NULL);
		if (new_descr == NULL) {
			Py_DECREF(fdescr);
			Py_DECREF(fieldlist);
			return -1;
		}
		assert(Py_TYPE(new_descr) == &CField_Type);
 		new_descr->size = fdescr->size;
 		new_descr->offset = fdescr->offset + offset;
 		new_descr->index = fdescr->index + index;
 		new_descr->proto = fdescr->proto;
 		Py_XINCREF(new_descr->proto);
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

	anon = PyObject_GetAttrString(type, "_anonymous_");
	if (anon == NULL) {
		PyErr_Clear();
		return 0;
	}
	anon_names = PySequence_Fast(anon, "_anonymous_ must be a sequence");
	Py_DECREF(anon);
	if (anon_names == NULL)
		return -1;

	for (i = 0; i < PySequence_Fast_GET_SIZE(anon_names); ++i) {
		PyObject *fname = PySequence_Fast_GET_ITEM(anon_names, i); /* borrowed */
		CFieldObject *descr = (CFieldObject *)PyObject_GetAttr(type, fname);
		if (descr == NULL) {
			Py_DECREF(anon_names);
			return -1;
		}
		assert(Py_TYPE(descr) == &CField_Type);
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
  Retrive the (optional) _pack_ attribute from a type, the _fields_ attribute,
  and create an StgDictObject.  Used for Structure and Union subclasses.
*/
int
StructUnionType_update_stgdict(PyObject *type, PyObject *fields, int isStruct)
{
	StgDictObject *stgdict, *basedict;
	Py_ssize_t len, offset, size, align, i;
	Py_ssize_t union_size, total_align;
	Py_ssize_t field_size = 0;
	int bitofs;
	PyObject *isPacked;
	int pack = 0;
	Py_ssize_t ffi_ofs;
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
	if (basedict && !use_broken_old_ctypes_semantics) {
		size = offset = basedict->size;
		align = basedict->align;
		union_size = 0;
		total_align = align ? align : 1;
		stgdict->ffi_type_pointer.type = FFI_TYPE_STRUCT;
		stgdict->ffi_type_pointer.elements = PyMem_Malloc(sizeof(ffi_type *) * (basedict->length + len + 1));
		if (stgdict->ffi_type_pointer.elements == NULL) {
			PyErr_NoMemory();
			return -1;
		}
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
		if (stgdict->ffi_type_pointer.elements == NULL) {
			PyErr_NoMemory();
			return -1;
		}
		memset(stgdict->ffi_type_pointer.elements, 0,
		       sizeof(ffi_type *) * (len + 1));
		ffi_ofs = 0;
	}

	assert(stgdict->format == NULL);
	if (isStruct && !isPacked) {
		stgdict->format = alloc_format_string(NULL, "T{");
	} else {
		/* PEP3118 doesn't support union, or packed structures (well,
		   only standard packing, but we dont support the pep for
		   that). Use 'B' for bytes. */
		stgdict->format = alloc_format_string(NULL, "B");
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
#if (PY_VERSION_HEX < 0x02050000)
				     "second item in _fields_ tuple (index %d) must be a C type",
#else
				     "second item in _fields_ tuple (index %zd) must be a C type",
#endif
				     i);
			return -1;
		}
		stgdict->ffi_type_pointer.elements[ffi_ofs + i] = &dict->ffi_type_pointer;
		if (dict->flags & (TYPEFLAG_ISPOINTER | TYPEFLAG_HASPOINTER))
			stgdict->flags |= TYPEFLAG_HASPOINTER;
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
		if (isStruct && !isPacked) {
			char *fieldfmt = dict->format ? dict->format : "B";
			char *fieldname = PyString_AsString(name);
			char *ptr;
			Py_ssize_t len = strlen(fieldname) + strlen(fieldfmt);
			char *buf = alloca(len + 2 + 1);

			sprintf(buf, "%s:%s:", fieldfmt, fieldname);

			ptr = stgdict->format;
			stgdict->format = alloc_format_string(stgdict->format, buf);
			PyMem_Free(ptr);

			if (stgdict->format == NULL) {
				Py_DECREF(pair);
				return -1;
			}
		}
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
			return -1;
		}
		if (-1 == PyObject_SetAttr(type, name, prop)) {
			Py_DECREF(prop);
			Py_DECREF(pair);
			return -1;
		}
		Py_DECREF(pair);
		Py_DECREF(prop);
	}
#undef realdict

	if (isStruct && !isPacked) {
		char *ptr = stgdict->format;
		stgdict->format = alloc_format_string(stgdict->format, "}");
		PyMem_Free(ptr);
		if (stgdict->format == NULL)
			return -1;
	}

	if (!isStruct)
		size = union_size;

	/* Adjust the size according to the alignment requirements */
	size = ((size + total_align - 1) / total_align) * total_align;

	stgdict->ffi_type_pointer.alignment = Py_SAFE_DOWNCAST(total_align,
							       Py_ssize_t,
							       unsigned short);
	stgdict->ffi_type_pointer.size = size;

	stgdict->size = size;
	stgdict->align = total_align;
	stgdict->length = len;	/* ADD ffi_ofs? */

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
