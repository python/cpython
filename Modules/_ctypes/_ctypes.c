/*
  ToDo:

  Get rid of the checker (and also the converters) field in CFuncPtrObject and
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

Name			methods, members, getsets
==============================================================================

StructType_Type		__new__(), from_address(), __mul__(), from_param()
UnionType_Type		__new__(), from_address(), __mul__(), from_param()
PointerType_Type	__new__(), from_address(), __mul__(), from_param(), set_type()
ArrayType_Type		__new__(), from_address(), __mul__(), from_param()
SimpleType_Type		__new__(), from_address(), __mul__(), from_param()

CData_Type
  Struct_Type		__new__(), __init__()
  Pointer_Type		__new__(), __init__(), _as_parameter_, contents
  Array_Type		__new__(), __init__(), _as_parameter_, __get/setitem__(), __len__()
  Simple_Type		__new__(), __init__(), _as_parameter_

CField_Type
StgDict_Type

==============================================================================

class methods
-------------

It has some similarity to the byref() construct compared to pointer()
from_address(addr)
	- construct an instance from a given memory block (sharing this memory block)

from_param(obj)
	- typecheck and convert a Python object into a C function call parameter
	  the result may be an instance of the type, or an integer or tuple
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
 * StgDict_Type
 * StructType_Type
 * UnionType_Type
 * PointerType_Type
 * ArrayType_Type
 * SimpleType_Type
 *
 * CData_Type
 * Struct_Type
 * Union_Type
 * Array_Type
 * Simple_Type
 * Pointer_Type
 * CField_Type
 *
 */

#include "Python.h"
#include "structmember.h"

#include <ffi.h>
#ifdef MS_WIN32
#include <windows.h>
#include <malloc.h>
#ifndef IS_INTRESOURCE
#define IS_INTRESOURCE(x) (((size_t)(x) >> 16) == 0)
#endif
# ifdef _WIN32_WCE
/* Unlike desktop Windows, WinCE has both W and A variants of
   GetProcAddress, but the default W version is not what we want */
#  undef GetProcAddress
#  define GetProcAddress GetProcAddressA
# endif
#else
#include "ctypes_dlfcn.h"
#endif
#include "ctypes.h"

PyObject *PyExc_ArgError;
static PyTypeObject Simple_Type;

char *conversion_mode_encoding = NULL;
char *conversion_mode_errors = NULL;


/******************************************************************/
/*
  StructType_Type - a meta type/class.  Creating a new class using this one as
  __metaclass__ will call the contructor StructUnionType_new.  It replaces the
  tp_dict member with a new instance of StgDict, and initializes the C
  accessible fields somehow.
*/

static PyCArgObject *
StructUnionType_paramfunc(CDataObject *self)
{
	PyCArgObject *parg;
	StgDictObject *stgdict;
	
	parg = new_CArgObject();
	if (parg == NULL)
		return NULL;

	parg->tag = 'V';
	stgdict = PyObject_stgdict((PyObject *)self);
	assert(stgdict); /* Cannot be NULL for structure/union instances */
	parg->pffi_type = &stgdict->ffi_type_pointer;
	/* For structure parameters (by value), parg->value doesn't contain the structure
	   data itself, instead parg->value.p *points* to the structure's data
	   See also _ctypes.c, function _call_function_pointer().
	*/
	parg->value.p = self->b_ptr;
	parg->size = self->b_size;
	Py_INCREF(self);
	parg->obj = (PyObject *)self;
	return parg;	
}

static PyObject *
StructUnionType_new(PyTypeObject *type, PyObject *args, PyObject *kwds, int isStruct)
{
	PyTypeObject *result;
	PyObject *fields;
	StgDictObject *dict;

	/* create the new instance (which is a class,
	   since we are a metatype!) */
	result = (PyTypeObject *)PyType_Type.tp_new(type, args, kwds);
	if (!result)
		return NULL;

	/* keep this for bw compatibility */
	if (PyDict_GetItemString(result->tp_dict, "_abstract_"))
		return (PyObject *)result;

	dict = (StgDictObject *)PyObject_CallObject((PyObject *)&StgDict_Type, NULL);
	if (!dict) {
		Py_DECREF(result);
		return NULL;
	}
	/* replace the class dict by our updated stgdict, which holds info
	   about storage requirements of the instances */
	if (-1 == PyDict_Update((PyObject *)dict, result->tp_dict)) {
		Py_DECREF(result);
		Py_DECREF((PyObject *)dict);
		return NULL;
	}
	Py_DECREF(result->tp_dict);
	result->tp_dict = (PyObject *)dict;

	dict->paramfunc = StructUnionType_paramfunc;

	fields = PyDict_GetItemString((PyObject *)dict, "_fields_");
	if (!fields) {
		StgDictObject *basedict = PyType_stgdict((PyObject *)result->tp_base);

		if (basedict == NULL)
			return (PyObject *)result;
		/* copy base dict */
		if (-1 == StgDict_clone(dict, basedict)) {
			Py_DECREF(result);
			return NULL;
		}
		dict->flags &= ~DICTFLAG_FINAL; /* clear the 'final' flag in the subclass dict */
		basedict->flags |= DICTFLAG_FINAL; /* set the 'final' flag in the baseclass dict */
		return (PyObject *)result;
	}

	if (-1 == PyObject_SetAttrString((PyObject *)result, "_fields_", fields)) {
		Py_DECREF(result);
		return NULL;
	}
	return (PyObject *)result;
}

static PyObject *
StructType_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	return StructUnionType_new(type, args, kwds, 1);
}

static PyObject *
UnionType_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	return StructUnionType_new(type, args, kwds, 0);
}

static char from_address_doc[] =
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
	return CData_AtAddress(type, buf);
}

static char in_dll_doc[] =
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
	address = (void *)GetProcAddress(handle, name);
	if (!address) {
		PyErr_Format(PyExc_ValueError,
			     "symbol '%s' not found",
			     name);
		return NULL;
	}
#else
	address = (void *)ctypes_dlsym(handle, name);
	if (!address) {
		PyErr_Format(PyExc_ValueError,
#ifdef __CYGWIN__
/* dlerror() isn't very helpful on cygwin */
			     "symbol '%s' not found (%s) ",
			     name,
#endif
			     ctypes_dlerror());
		return NULL;
	}
#endif
	return CData_AtAddress(type, address);
}

static char from_param_doc[] =
"Convert a Python object into a function call parameter.";

static PyObject *
CDataType_from_param(PyObject *type, PyObject *value)
{
	PyObject *as_parameter;
	if (1 == PyObject_IsInstance(value, type)) {
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
		if(dict && ob
		   && PyObject_IsInstance(ob, dict->proto)) {
			Py_INCREF(value);
			return value;
		}
		ob_name = (ob) ? Py_TYPE(ob)->tp_name : "???";
		PyErr_Format(PyExc_TypeError,
			     "expected %s instance instead of pointer to %s",
			     ((PyTypeObject *)type)->tp_name, ob_name);
		return NULL;
	}

	as_parameter = PyObject_GetAttrString(value, "_as_parameter_");
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
	return CreateArrayType(self, length);
}

static PySequenceMethods CDataType_as_sequence = {
	0,			/* inquiry sq_length; */
	0,			/* binaryfunc sq_concat; */
	CDataType_repeat,	/* intargfunc sq_repeat; */
	0,			/* intargfunc sq_item; */
	0,			/* intintargfunc sq_slice; */
	0,			/* intobjargproc sq_ass_item; */
	0,			/* intintobjargproc sq_ass_slice; */
	0,			/* objobjproc sq_contains; */
	
	0,			/* binaryfunc sq_inplace_concat; */
	0,			/* intargfunc sq_inplace_repeat; */
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
StructType_setattro(PyObject *self, PyObject *key, PyObject *value)
{
	/* XXX Should we disallow deleting _fields_? */
	if (-1 == PyType_Type.tp_setattro(self, key, value))
		return -1;
	
	if (value && PyUnicode_Check(key) &&
	    /* XXX struni PyUnicode_AsString can fail (also in other places)! */
	    0 == strcmp(PyUnicode_AsString(key), "_fields_"))
		return StructUnionType_update_stgdict(self, value, 1);
	return 0;
}


static int
UnionType_setattro(PyObject *self, PyObject *key, PyObject *value)
{
	/* XXX Should we disallow deleting _fields_? */
	if (-1 == PyObject_GenericSetAttr(self, key, value))
		return -1;
	
	if (PyUnicode_Check(key) &&
	    0 == strcmp(PyUnicode_AsString(key), "_fields_"))
		return StructUnionType_update_stgdict(self, value, 0);
	return 0;
}


PyTypeObject StructType_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes.StructType",			/* tp_name */
	0,					/* tp_basicsize */
	0,					/* tp_itemsize */
	0,					/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,			       		/* tp_repr */
	0,					/* tp_as_number */
	&CDataType_as_sequence,			/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	StructType_setattro,			/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /* tp_flags */
	"metatype for the CData Objects",	/* tp_doc */
	(traverseproc)CDataType_traverse,	/* tp_traverse */
	(inquiry)CDataType_clear,		/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	CDataType_methods,			/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	StructType_new,				/* tp_new */
	0,					/* tp_free */
};

static PyTypeObject UnionType_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes.UnionType",			/* tp_name */
	0,					/* tp_basicsize */
	0,					/* tp_itemsize */
	0,					/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,			       		/* tp_repr */
	0,					/* tp_as_number */
	&CDataType_as_sequence,		/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	UnionType_setattro,			/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /* tp_flags */
	"metatype for the CData Objects",	/* tp_doc */
	(traverseproc)CDataType_traverse,	/* tp_traverse */
	(inquiry)CDataType_clear,		/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	CDataType_methods,			/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	UnionType_new,				/* tp_new */
	0,					/* tp_free */
};


/******************************************************************/

/*

The PointerType_Type metaclass must ensure that the subclass of Pointer can be
created. It must check for a _type_ attribute in the class. Since are no
runtime created properties, a CField is probably *not* needed ?

class IntPointer(Pointer):
    _type_ = "i"

The Pointer_Type provides the functionality: a contents method/property, a
size property/method, and the sequence protocol.

*/

static int
PointerType_SetProto(StgDictObject *stgdict, PyObject *proto)
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
	Py_XDECREF(stgdict->proto);
	stgdict->proto = proto;
	return 0;
}

static PyCArgObject *
PointerType_paramfunc(CDataObject *self)
{
	PyCArgObject *parg;

	parg = new_CArgObject();
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
PointerType_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyTypeObject *result;
	StgDictObject *stgdict;
	PyObject *proto;
	PyObject *typedict;

	typedict = PyTuple_GetItem(args, 2);
	if (!typedict)
		return NULL;
/*
  stgdict items size, align, length contain info about pointers itself,
  stgdict->proto has info about the pointed to type!
*/
	stgdict = (StgDictObject *)PyObject_CallObject(
		(PyObject *)&StgDict_Type, NULL);
	if (!stgdict)
		return NULL;
	stgdict->size = sizeof(void *);
	stgdict->align = getentry("P")->pffi_type->alignment;
	stgdict->length = 1;
	stgdict->ffi_type_pointer = ffi_type_pointer;
	stgdict->paramfunc = PointerType_paramfunc;

	proto = PyDict_GetItemString(typedict, "_type_"); /* Borrowed ref */
	if (proto && -1 == PointerType_SetProto(stgdict, proto)) {
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
	Py_DECREF(result->tp_dict);
	result->tp_dict = (PyObject *)stgdict;

	return (PyObject *)result;
}


static PyObject *
PointerType_set_type(PyTypeObject *self, PyObject *type)
{
	StgDictObject *dict;

	dict = PyType_stgdict((PyObject *)self);
	assert(dict);

	if (-1 == PointerType_SetProto(dict, type))
		return NULL;

	if (-1 == PyDict_SetItemString((PyObject *)dict, "_type_", type))
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *_byref(PyObject *);

static PyObject *
PointerType_from_param(PyObject *type, PyObject *value)
{
	StgDictObject *typedict;

	if (value == Py_None)
		return PyLong_FromLong(0); /* NULL pointer */

	typedict = PyType_stgdict(type);
	assert(typedict); /* Cannot be NULL for pointer types */

	/* If we expect POINTER(<type>), but receive a <type> instance, accept
	   it by calling byref(<type>).
	*/
	switch (PyObject_IsInstance(value, typedict->proto)) {
	case 1:
		Py_INCREF(value); /* _byref steals a refcount */
		return _byref(value);
	case -1:
		PyErr_Clear();
		break;
	default:
 		break;
	}

 	if (PointerObject_Check(value) || ArrayObject_Check(value)) {
 		/* Array instances are also pointers when
 		   the item types are the same.
 		*/
 		StgDictObject *v = PyObject_stgdict(value);
		assert(v); /* Cannot be NULL for pointer or array objects */
 		if (PyObject_IsSubclass(v->proto, typedict->proto)) {
  			Py_INCREF(value);
  			return value;
  		}
  	}
     	return CDataType_from_param(type, value);
}

static PyMethodDef PointerType_methods[] = {
	{ "from_address", CDataType_from_address, METH_O, from_address_doc },
	{ "in_dll", CDataType_in_dll, METH_VARARGS, in_dll_doc},
	{ "from_param", (PyCFunction)PointerType_from_param, METH_O, from_param_doc},
	{ "set_type", (PyCFunction)PointerType_set_type, METH_O },
	{ NULL, NULL },
};

PyTypeObject PointerType_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes.PointerType",				/* tp_name */
	0,					/* tp_basicsize */
	0,					/* tp_itemsize */
	0,					/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,			       		/* tp_repr */
	0,					/* tp_as_number */
	&CDataType_as_sequence,		/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /* tp_flags */
	"metatype for the Pointer Objects",	/* tp_doc */
	(traverseproc)CDataType_traverse,	/* tp_traverse */
	(inquiry)CDataType_clear,		/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	PointerType_methods,			/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	PointerType_new,			/* tp_new */
	0,					/* tp_free */
};


/******************************************************************/
/*
  ArrayType_Type
*/
/*
  ArrayType_new ensures that the new Array subclass created has a _length_
  attribute, and a _type_ attribute.
*/

static int
CharArray_set_raw(CDataObject *self, PyObject *value)
{
	char *ptr;
	Py_ssize_t size;
        Py_buffer view;

        if (PyObject_GetBuffer(value, &view, PyBUF_SIMPLE) < 0)
		return -1;
        size = view.len;
	ptr = view.buf;
	if (size > self->b_size) {
		PyErr_SetString(PyExc_ValueError,
				"string too long");
		goto fail;
	}

	memcpy(self->b_ptr, ptr, size);

	PyObject_ReleaseBuffer(value, &view);
	return 0;
 fail:
	PyObject_ReleaseBuffer(value, &view);
        return -1;
}

static PyObject *
CharArray_get_raw(CDataObject *self)
{
	return PyString_FromStringAndSize(self->b_ptr, self->b_size);
}

static PyObject *
CharArray_get_value(CDataObject *self)
{
	int i;
	char *ptr = self->b_ptr;
	for (i = 0; i < self->b_size; ++i)
		if (*ptr++ == '\0')
			break;
	return PyString_FromStringAndSize(self->b_ptr, i);
}

static int
CharArray_set_value(CDataObject *self, PyObject *value)
{
	char *ptr;
	Py_ssize_t size;

	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"can't delete attribute");
		return -1;
	}

	if (PyUnicode_Check(value)) {
		value = PyUnicode_AsEncodedString(value,
						  conversion_mode_encoding,
						  conversion_mode_errors);
		if (!value)
			return -1;
	} else if (!PyString_Check(value)) {
		PyErr_Format(PyExc_TypeError,
			     "str/bytes expected instead of %s instance",
			     Py_TYPE(value)->tp_name);
		return -1;
	} else
		Py_INCREF(value);
	size = PyString_GET_SIZE(value);
	if (size > self->b_size) {
		PyErr_SetString(PyExc_ValueError,
				"string too long");
		Py_DECREF(value);
		return -1;
	}

	ptr = PyString_AS_STRING(value);
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
WCharArray_get_value(CDataObject *self)
{
	unsigned int i;
	wchar_t *ptr = (wchar_t *)self->b_ptr;
	for (i = 0; i < self->b_size/sizeof(wchar_t); ++i)
		if (*ptr++ == (wchar_t)0)
			break;
	return PyUnicode_FromWideChar((wchar_t *)self->b_ptr, i);
}

static int
WCharArray_set_value(CDataObject *self, PyObject *value)
{
	Py_ssize_t result = 0;

	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"can't delete attribute");
		return -1;
	}
	if (PyString_Check(value)) {
		value = PyUnicode_FromEncodedObject(value,
						    conversion_mode_encoding,
						    conversion_mode_errors);
		if (!value)
			return -1;
	} else if (!PyUnicode_Check(value)) {
		PyErr_Format(PyExc_TypeError,
				"unicode string expected instead of %s instance",
				Py_TYPE(value)->tp_name);
		return -1;
	} else
		Py_INCREF(value);
	if ((unsigned)PyUnicode_GET_SIZE(value) > self->b_size/sizeof(wchar_t)) {
		PyErr_SetString(PyExc_ValueError,
				"string too long");
		result = -1;
		goto done;
	}
	result = PyUnicode_AsWideChar((PyUnicodeObject *)value,
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
		if (PyDict_SetItemString(dict,meth->ml_name, descr) < 0)
			return -1;
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
		if (PyDict_SetItemString(dict, memb->name, descr) < 0)
			return -1;
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
		if (PyDict_SetItemString(dict, gsp->name, descr) < 0)
			return -1;
		Py_DECREF(descr);
	}
	return 0;
}

static PyCArgObject *
ArrayType_paramfunc(CDataObject *self)
{
	PyCArgObject *p = new_CArgObject();
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
ArrayType_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyTypeObject *result;
	StgDictObject *stgdict;
	StgDictObject *itemdict;
	PyObject *proto;
	PyObject *typedict;
	long length;
	int overflow;
	Py_ssize_t itemsize, itemalign;

	typedict = PyTuple_GetItem(args, 2);
	if (!typedict)
		return NULL;

	proto = PyDict_GetItemString(typedict, "_length_"); /* Borrowed ref */
	if (!proto || !PyLong_Check(proto)) {
		PyErr_SetString(PyExc_AttributeError,
				"class must define a '_length_' attribute, "
				"which must be a positive integer");
		return NULL;
	}
	length = PyLong_AsLongAndOverflow(proto, &overflow);
	if (overflow) {
		PyErr_SetString(PyExc_OverflowError,
				"The '_length_' attribute is too large");
		return NULL;
	}

	proto = PyDict_GetItemString(typedict, "_type_"); /* Borrowed ref */
	if (!proto) {
		PyErr_SetString(PyExc_AttributeError,
				"class must define a '_type_' attribute");
		return NULL;
	}

	stgdict = (StgDictObject *)PyObject_CallObject(
		(PyObject *)&StgDict_Type, NULL);
	if (!stgdict)
		return NULL;

	itemdict = PyType_stgdict(proto);
	if (!itemdict) {
		PyErr_SetString(PyExc_TypeError,
				"_type_ must have storage info");
		Py_DECREF((PyObject *)stgdict);
		return NULL;
	}

	itemsize = itemdict->size;
	if (length * itemsize < 0) {
		PyErr_SetString(PyExc_OverflowError,
				"array too large");
		return NULL;
	}

	itemalign = itemdict->align;

	stgdict->size = itemsize * length;
	stgdict->align = itemalign;
	stgdict->length = length;
	Py_INCREF(proto);
	stgdict->proto = proto;

	stgdict->paramfunc = &ArrayType_paramfunc;

	/* Arrays are passed as pointers to function calls. */
	stgdict->ffi_type_pointer = ffi_type_pointer;

	/* create the new instance (which is a class,
	   since we are a metatype!) */
	result = (PyTypeObject *)PyType_Type.tp_new(type, args, kwds);
	if (result == NULL)
		return NULL;

	/* replace the class dict by our updated spam dict */
	if (-1 == PyDict_Update((PyObject *)stgdict, result->tp_dict)) {
		Py_DECREF(result);
		Py_DECREF((PyObject *)stgdict);
		return NULL;
	}
	Py_DECREF(result->tp_dict);
	result->tp_dict = (PyObject *)stgdict;

	/* Special case for character arrays.
	   A permanent annoyance: char arrays are also strings!
	*/
	if (itemdict->getfunc == getentry("c")->getfunc) {
		if (-1 == add_getset(result, CharArray_getsets))
			return NULL;
#ifdef CTYPES_UNICODE
	} else if (itemdict->getfunc == getentry("u")->getfunc) {
		if (-1 == add_getset(result, WCharArray_getsets))
			return NULL;
#endif
	}

	return (PyObject *)result;
}

PyTypeObject ArrayType_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes.ArrayType",			/* tp_name */
	0,					/* tp_basicsize */
	0,					/* tp_itemsize */
	0,					/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,			       		/* tp_repr */
	0,					/* tp_as_number */
	&CDataType_as_sequence,			/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"metatype for the Array Objects",	/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	CDataType_methods,			/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	ArrayType_new,				/* tp_new */
	0,					/* tp_free */
};


/******************************************************************/
/*
  SimpleType_Type
*/
/*

SimpleType_new ensures that the new Simple_Type subclass created has a valid
_type_ attribute.

*/

static char *SIMPLE_TYPE_CHARS = "cbBhHiIlLdfuzZqQPXOvtD";

static PyObject *
c_wchar_p_from_param(PyObject *type, PyObject *value)
{
	PyObject *as_parameter;
	if (value == Py_None) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (PyUnicode_Check(value) || PyString_Check(value)) {
		PyCArgObject *parg;
		struct fielddesc *fd = getentry("Z");

		parg = new_CArgObject();
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
	if (PyObject_IsInstance(value, type)) {
		Py_INCREF(value);
		return value;
	}
	if (ArrayObject_Check(value) || PointerObject_Check(value)) {
		/* c_wchar array instance or pointer(c_wchar(...)) */
		StgDictObject *dt = PyObject_stgdict(value);
		StgDictObject *dict;
		assert(dt); /* Cannot be NULL for pointer or array objects */
		dict = dt && dt->proto ? PyType_stgdict(dt->proto) : NULL;
		if (dict && (dict->setfunc == getentry("u")->setfunc)) {
			Py_INCREF(value);
			return value;
		}
	}
	if (PyCArg_CheckExact(value)) {
		/* byref(c_char(...)) */
		PyCArgObject *a = (PyCArgObject *)value;
		StgDictObject *dict = PyObject_stgdict(a->obj);
		if (dict && (dict->setfunc == getentry("u")->setfunc)) {
			Py_INCREF(value);
			return value;
		}
	}

	as_parameter = PyObject_GetAttrString(value, "_as_parameter_");
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
	PyObject *as_parameter;
	if (value == Py_None) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (PyString_Check(value) || PyUnicode_Check(value)) {
		PyCArgObject *parg;
		struct fielddesc *fd = getentry("z");

		parg = new_CArgObject();
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
	if (PyObject_IsInstance(value, type)) {
		Py_INCREF(value);
		return value;
	}
	if (ArrayObject_Check(value) || PointerObject_Check(value)) {
		/* c_char array instance or pointer(c_char(...)) */
		StgDictObject *dt = PyObject_stgdict(value);
		StgDictObject *dict;
		assert(dt); /* Cannot be NULL for pointer or array objects */
		dict = dt && dt->proto ? PyType_stgdict(dt->proto) : NULL;
		if (dict && (dict->setfunc == getentry("c")->setfunc)) {
			Py_INCREF(value);
			return value;
		}
	}
	if (PyCArg_CheckExact(value)) {
		/* byref(c_char(...)) */
		PyCArgObject *a = (PyCArgObject *)value;
		StgDictObject *dict = PyObject_stgdict(a->obj);
		if (dict && (dict->setfunc == getentry("c")->setfunc)) {
			Py_INCREF(value);
			return value;
		}
	}

	as_parameter = PyObject_GetAttrString(value, "_as_parameter_");
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
	StgDictObject *stgd;
	PyObject *as_parameter;

/* None */
	if (value == Py_None) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	/* Should probably allow buffer interface as well */
/* int, long */
	if (PyLong_Check(value)) {
		PyCArgObject *parg;
		struct fielddesc *fd = getentry("P");

		parg = new_CArgObject();
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
/* string */
	if (PyString_Check(value)) {
		PyCArgObject *parg;
		struct fielddesc *fd = getentry("z");

		parg = new_CArgObject();
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
/* bytes */
	if (PyBytes_Check(value)) {
		PyCArgObject *parg;
		struct fielddesc *fd = getentry("z");

		parg = new_CArgObject();
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
		struct fielddesc *fd = getentry("Z");

		parg = new_CArgObject();
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
	if (PyObject_IsInstance(value, type)) {
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
	if (CFuncPtrObject_Check(value)) {
		PyCArgObject *parg;
		CFuncPtrObject *func;
		func = (CFuncPtrObject *)value;
		parg = new_CArgObject();
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

		switch (PyUnicode_AsString(stgd->proto)[0]) {
		case 'z': /* c_char_p */
		case 'Z': /* c_wchar_p */
			parg = new_CArgObject();
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

	as_parameter = PyObject_GetAttrString(value, "_as_parameter_");
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
		suffix = PyUnicode_FromString("_le");
#else
		suffix = PyUnicode_FromString("_be");
#endif

	newname = PyUnicode_Concat(name, suffix);
	if (newname == NULL) {
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

	stgdict = (StgDictObject *)PyObject_CallObject(
		(PyObject *)&StgDict_Type, NULL);
	if (!stgdict) /* XXX leaks result! */
		return NULL;

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
	Py_DECREF(result->tp_dict);
	result->tp_dict = (PyObject *)stgdict;

	return (PyObject *)result;
}

static PyCArgObject *
SimpleType_paramfunc(CDataObject *self)
{
	StgDictObject *dict;
	char *fmt;
	PyCArgObject *parg;
	struct fielddesc *fd;
	
	dict = PyObject_stgdict((PyObject *)self);
	assert(dict); /* Cannot be NULL for CDataObject instances */
	fmt = PyUnicode_AsString(dict->proto);
	assert(fmt);

	fd = getentry(fmt);
	assert(fd);
	
	parg = new_CArgObject();
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
SimpleType_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
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

	proto = PyObject_GetAttrString((PyObject *)result, "_type_"); /* new ref */
	if (!proto) {
		PyErr_SetString(PyExc_AttributeError,
				"class must define a '_type_' attribute");
  error:
		Py_XDECREF(proto);
		Py_XDECREF(result);
		return NULL;
	}
	if (PyUnicode_Check(proto)) {
		PyObject *v = _PyUnicode_AsDefaultEncodedString(proto, NULL);
		if (!v)
			goto error;
		proto_str = PyString_AS_STRING(v);
		proto_len = PyString_GET_SIZE(v);
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
	fmt = getentry(proto_str);
	if (fmt == NULL) {
		Py_DECREF((PyObject *)result);
		PyErr_Format(PyExc_ValueError,
			     "_type_ '%s' not supported", proto_str);
		return NULL;
	}

	stgdict = (StgDictObject *)PyObject_CallObject(
		(PyObject *)&StgDict_Type, NULL);
	if (!stgdict)
		return NULL;

	stgdict->ffi_type_pointer = *fmt->pffi_type;
	stgdict->align = fmt->pffi_type->alignment;
	stgdict->length = 0;
	stgdict->size = fmt->pffi_type->size;
	stgdict->setfunc = fmt->setfunc;
	stgdict->getfunc = fmt->getfunc;

	stgdict->paramfunc = SimpleType_paramfunc;
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
	Py_DECREF(result->tp_dict);
	result->tp_dict = (PyObject *)stgdict;

	/* Install from_param class methods in ctypes base classes.
	   Overrides the SimpleType_from_param generic method.
	 */
	if (result->tp_base == &Simple_Type) {
		switch (*proto_str) {
		case 'z': /* c_char_p */
			ml = &c_char_p_method;
			break;
		case 'Z': /* c_wchar_p */
			ml = &c_wchar_p_method;
			break;
		case 'P': /* c_void_p */
			ml = &c_void_p_method;
			break;
		default:
			ml = NULL;
			break;
		}
			
		if (ml) {
			PyObject *meth;
			int x;
			meth = PyDescr_NewClassMethod(result, ml);
			if (!meth)
				return NULL;
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

	if (type == &SimpleType_Type && fmt->setfunc_swapped && fmt->getfunc_swapped) {
		PyObject *swapped = CreateSwappedType(type, args, kwds,
						      proto, fmt);
		if (swapped == NULL) {
			Py_DECREF(result);
			return NULL;
		}
#ifdef WORDS_BIGENDIAN
		PyObject_SetAttrString((PyObject *)result, "__ctype_le__", swapped);
		PyObject_SetAttrString((PyObject *)result, "__ctype_be__", (PyObject *)result);
		PyObject_SetAttrString(swapped, "__ctype_be__", (PyObject *)result);
		PyObject_SetAttrString(swapped, "__ctype_le__", swapped);
#else
		PyObject_SetAttrString((PyObject *)result, "__ctype_be__", swapped);
		PyObject_SetAttrString((PyObject *)result, "__ctype_le__", (PyObject *)result);
		PyObject_SetAttrString(swapped, "__ctype_le__", (PyObject *)result);
		PyObject_SetAttrString(swapped, "__ctype_be__", swapped);
#endif
		Py_DECREF(swapped);
	};

	return (PyObject *)result;
}

/*
 * This is a *class method*.
 * Convert a parameter into something that ConvParam can handle.
 */
static PyObject *
SimpleType_from_param(PyObject *type, PyObject *value)
{
	StgDictObject *dict;
	char *fmt;
	PyCArgObject *parg;
	struct fielddesc *fd;
	PyObject *as_parameter;

	/* If the value is already an instance of the requested type,
	   we can use it as is */
	if (1 == PyObject_IsInstance(value, type)) {
		Py_INCREF(value);
		return value;
	}

	dict = PyType_stgdict(type);
	assert(dict);

	/* I think we can rely on this being a one-character string */
	fmt = PyUnicode_AsString(dict->proto);
	assert(fmt);
	
	fd = getentry(fmt);
	assert(fd);
	
	parg = new_CArgObject();
	if (parg == NULL)
		return NULL;

	parg->tag = fmt[0];
	parg->pffi_type = fd->pffi_type;
	parg->obj = fd->setfunc(&parg->value, value, 0);
	if (parg->obj)
		return (PyObject *)parg;
	PyErr_Clear();
	Py_DECREF(parg);

	as_parameter = PyObject_GetAttrString(value, "_as_parameter_");
	if (as_parameter) {
		value = SimpleType_from_param(type, as_parameter);
		Py_DECREF(as_parameter);
		return value;
	}
	PyErr_SetString(PyExc_TypeError,
			"wrong type");
	return NULL;
}

static PyMethodDef SimpleType_methods[] = {
	{ "from_param", SimpleType_from_param, METH_O, from_param_doc },
	{ "from_address", CDataType_from_address, METH_O, from_address_doc },
	{ "in_dll", CDataType_in_dll, METH_VARARGS, in_dll_doc},
	{ NULL, NULL },
};

PyTypeObject SimpleType_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes.SimpleType",				/* tp_name */
	0,					/* tp_basicsize */
	0,					/* tp_itemsize */
	0,					/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,			       		/* tp_repr */
	0,					/* tp_as_number */
	&CDataType_as_sequence,		/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"metatype for the SimpleType Objects",	/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	SimpleType_methods,			/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	SimpleType_new,				/* tp_new */
	0,					/* tp_free */
};

/******************************************************************/
/*
  CFuncPtrType_Type
 */

static PyObject *
converters_from_argtypes(PyObject *ob)
{
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
	if (!converters)
		return NULL;
		
	/* I have to check if this is correct. Using c_char, which has a size
	   of 1, will be assumed to be pushed as only one byte!
	   Aren't these promoted to integers by the C compiler and pushed as 4 bytes?
	*/

	for (i = 0; i < nArgs; ++i) {
		PyObject *tp = PyTuple_GET_ITEM(ob, i);
		PyObject *cnv = PyObject_GetAttrString(tp, "from_param");
		if (!cnv)
			goto argtypes_error_1;
		PyTuple_SET_ITEM(converters, i, cnv);
	}
	Py_DECREF(ob);
	return converters;

  argtypes_error_1:
	Py_XDECREF(converters);
	Py_DECREF(ob);
	PyErr_Format(PyExc_TypeError,
		     "item %zd in _argtypes_ has no from_param method",
		     i+1);
	return NULL;
}

static int
make_funcptrtype_dict(StgDictObject *stgdict)
{
	PyObject *ob;
	PyObject *converters = NULL;

	stgdict->align = getentry("P")->pffi_type->alignment;
	stgdict->length = 1;
	stgdict->size = sizeof(void *);
	stgdict->setfunc = NULL;
	stgdict->getfunc = NULL;
	stgdict->ffi_type_pointer = ffi_type_pointer;

	ob = PyDict_GetItemString((PyObject *)stgdict, "_flags_");
	if (!ob || !PyLong_Check(ob)) {
		PyErr_SetString(PyExc_TypeError,
		    "class must define _flags_ which must be an integer");
		return -1;
	}
	stgdict->flags = PyLong_AS_LONG(ob);

	/* _argtypes_ is optional... */
	ob = PyDict_GetItemString((PyObject *)stgdict, "_argtypes_");
	if (ob) {
		converters = converters_from_argtypes(ob);
		if (!converters)
			goto error;
		Py_INCREF(ob);
		stgdict->argtypes = ob;
		stgdict->converters = converters;
	}

	ob = PyDict_GetItemString((PyObject *)stgdict, "_restype_");
	if (ob) {
		if (ob != Py_None && !PyType_stgdict(ob) && !PyCallable_Check(ob)) {
			PyErr_SetString(PyExc_TypeError,
				"_restype_ must be a type, a callable, or None");
			return -1;
		}
		Py_INCREF(ob);
		stgdict->restype = ob;
		stgdict->checker = PyObject_GetAttrString(ob, "_check_retval_");
		if (stgdict->checker == NULL)
			PyErr_Clear();
	}
/* XXX later, maybe.
	ob = PyDict_GetItemString((PyObject *)stgdict, "_errcheck_");
	if (ob) {
		if (!PyCallable_Check(ob)) {
			PyErr_SetString(PyExc_TypeError,
				"_errcheck_ must be callable");
			return -1;
		}
		Py_INCREF(ob);
		stgdict->errcheck = ob;
	}
*/
	return 0;

  error:
	Py_XDECREF(converters);
	return -1;

}

static PyCArgObject *
CFuncPtrType_paramfunc(CDataObject *self)
{
	PyCArgObject *parg;
	
	parg = new_CArgObject();
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
CFuncPtrType_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyTypeObject *result;
	StgDictObject *stgdict;

	stgdict = (StgDictObject *)PyObject_CallObject(
		(PyObject *)&StgDict_Type, NULL);
	if (!stgdict)
		return NULL;

	stgdict->paramfunc = CFuncPtrType_paramfunc;

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
	Py_DECREF(result->tp_dict);
	result->tp_dict = (PyObject *)stgdict;

	if (-1 == make_funcptrtype_dict(stgdict)) {
		Py_DECREF(result);
		return NULL;
	}

	return (PyObject *)result;
}

PyTypeObject CFuncPtrType_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes.CFuncPtrType",			/* tp_name */
	0,					/* tp_basicsize */
	0,					/* tp_itemsize */
	0,					/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,			       		/* tp_repr */
	0,					/* tp_as_number */
	&CDataType_as_sequence,			/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /* tp_flags */
	"metatype for C function pointers",	/* tp_doc */
	(traverseproc)CDataType_traverse,	/* tp_traverse */
	(inquiry)CDataType_clear,		/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	CDataType_methods,			/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	CFuncPtrType_new,			/* tp_new */
	0,					/* tp_free */
};


/*****************************************************************
 * Code to keep needed objects alive
 */

static CDataObject *
CData_GetContainer(CDataObject *self)
{
	while (self->b_base)
		self = self->b_base;
	if (self->b_objects == NULL) {
		if (self->b_length) {
			self->b_objects = PyDict_New();
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
	return CData_GetContainer(target)->b_objects;
}

static PyObject *
unique_key(CDataObject *target, Py_ssize_t index)
{
	char string[256];
	char *cp = string;
	size_t bytes_left;

	assert(sizeof(string) - 1 > sizeof(Py_ssize_t) * 2);
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
	ob = CData_GetContainer(target);
	if (ob->b_objects == NULL || !PyDict_Check(ob->b_objects)) {
		Py_XDECREF(ob->b_objects);
		ob->b_objects = keep; /* refcount consumed */
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
  CData_Type
 */
static int
CData_traverse(CDataObject *self, visitproc visit, void *arg)
{
	Py_VISIT(self->b_objects);
	Py_VISIT((PyObject *)self->b_base);
	return 0;
}

static int
CData_clear(CDataObject *self)
{
	StgDictObject *dict = PyObject_stgdict((PyObject *)self);
	assert(dict); /* Cannot be NULL for CDataObject instances */
	Py_CLEAR(self->b_objects);
	if ((self->b_needsfree)
	    && ((size_t)dict->size > sizeof(self->b_value)))
		PyMem_Free(self->b_ptr);
	self->b_ptr = NULL;
	Py_CLEAR(self->b_base);
	return 0;
}

static void
CData_dealloc(PyObject *self)
{
	CData_clear((CDataObject *)self);
	Py_TYPE(self)->tp_free(self);
}

static PyMemberDef CData_members[] = {
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

static int CData_GetBuffer(PyObject *_self, Py_buffer *view, int flags)
{
	CDataObject *self = (CDataObject *)_self;
        return PyBuffer_FillInfo(view, self->b_ptr, self->b_size, 0, flags);
}

static PyBufferProcs CData_as_buffer = {
	CData_GetBuffer,
        NULL,
};

/*
 * CData objects are mutable, so they cannot be hashable!
 */
static long
CData_nohash(PyObject *self)
{
	PyErr_SetString(PyExc_TypeError, "unhashable type");
	return -1;
}

/*
 * default __ctypes_from_outparam__ method returns self.
 */
static PyObject *
CData_from_outparam(PyObject *self, PyObject *args)
{
	Py_INCREF(self);
	return self;
}

static PyMethodDef CData_methods[] = {
	{ "__ctypes_from_outparam__", CData_from_outparam, METH_NOARGS, },
	{ NULL, NULL },
};

PyTypeObject CData_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes._CData",
	sizeof(CDataObject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	CData_dealloc,				/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	CData_nohash,				/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	&CData_as_buffer,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"XXX to be provided",			/* tp_doc */
	(traverseproc)CData_traverse,		/* tp_traverse */
	(inquiry)CData_clear,			/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	CData_methods,				/* tp_methods */
	CData_members,				/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	0,					/* tp_new */
	0,					/* tp_free */
};

static int CData_MallocBuffer(CDataObject *obj, StgDictObject *dict)
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
CData_FromBaseObj(PyObject *type, PyObject *base, Py_ssize_t index, char *adr)
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
		if (-1 == CData_MallocBuffer(cmem, dict)) {
			return NULL;
			Py_DECREF(cmem);
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
CData_AtAddress(PyObject *type, void *buf)
{
	CDataObject *pd;
	StgDictObject *dict;

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
int IsSimpleSubType(PyObject *obj)
{
	PyTypeObject *type = (PyTypeObject *)obj;

	if (SimpleTypeObject_Check(type))
		return type->tp_base != &Simple_Type;
	return 0;
}

PyObject *
CData_get(PyObject *type, GETFUNC getfunc, PyObject *src,
	  Py_ssize_t index, Py_ssize_t size, char *adr)
{
	StgDictObject *dict;
	if (getfunc)
		return getfunc(adr, size);
	assert(type);
	dict = PyType_stgdict(type);
	if (dict && dict->getfunc && !IsSimpleSubType(type))
		return dict->getfunc(adr, size);
	return CData_FromBaseObj(type, src, index, adr);
}

/*
  Helper function for CData_set below.
*/
static PyObject *
_CData_set(CDataObject *dst, PyObject *type, SETFUNC setfunc, PyObject *value,
	   Py_ssize_t size, char *ptr)
{
	CDataObject *src;

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
				Extend_Error_Info(PyExc_RuntimeError, "(%s) ",
						  ((PyTypeObject *)type)->tp_name);
				return NULL;
			}
			result = _CData_set(dst, type, setfunc, ob,
					    size, ptr);
			Py_DECREF(ob);
			return result;
		} else if (value == Py_None && PointerTypeObject_Check(type)) {
			*(void **)ptr = NULL;
			Py_INCREF(Py_None);
			return Py_None;
		} else {
			PyErr_Format(PyExc_TypeError,
				     "expected %s instance, got %s",
				     ((PyTypeObject *)type)->tp_name,
				     Py_TYPE(value)->tp_name);
			return NULL;
		}
	}
	src = (CDataObject *)value;

	if (PyObject_IsInstance(value, type)) {
		memcpy(ptr,
		       src->b_ptr,
		       size);

		if (PointerTypeObject_Check(type))
			/* XXX */;

		value = GetKeepedObjects(src);
		Py_INCREF(value);
		return value;
	}

	if (PointerTypeObject_Check(type)
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
		/*
		  We are assigning an array object to a field which represents
		  a pointer. This has the same effect as converting an array
		  into a pointer. So, again, we have to keep the whole object
		  pointed to (which is the array in this case) alive, and not
		  only it's object list.  So we create a tuple, containing
		  b_objects list PLUS the array itself, and return that!
		*/
		return Py_BuildValue("(OO)", keep, value);
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
CData_set(PyObject *dst, PyObject *type, SETFUNC setfunc, PyObject *value,
	  Py_ssize_t index, Py_ssize_t size, char *ptr)
{
	CDataObject *mem = (CDataObject *)dst;
	PyObject *result;

	if (!CDataObject_Check(dst)) {
		PyErr_SetString(PyExc_TypeError,
				"not a ctype instance");
		return -1;
	}

	result = _CData_set(mem, type, setfunc, value,
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
GenericCData_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
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
			
	if (-1 == CData_MallocBuffer(obj, dict)) {
		Py_DECREF(obj);
		return NULL;
	}
	return (PyObject *)obj;
}
/*****************************************************************/
/*
  CFuncPtr_Type
*/

static int
CFuncPtr_set_errcheck(CFuncPtrObject *self, PyObject *ob)
{
	if (ob && !PyCallable_Check(ob)) {
		PyErr_SetString(PyExc_TypeError,
				"the errcheck attribute must be callable");
		return -1;
	}
	Py_XDECREF(self->errcheck);
	Py_XINCREF(ob);
	self->errcheck = ob;
	return 0;
}

static PyObject *
CFuncPtr_get_errcheck(CFuncPtrObject *self)
{
	if (self->errcheck) {
		Py_INCREF(self->errcheck);
		return self->errcheck;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static int
CFuncPtr_set_restype(CFuncPtrObject *self, PyObject *ob)
{
	if (ob == NULL) {
		Py_XDECREF(self->restype);
		self->restype = NULL;
		Py_XDECREF(self->checker);
		self->checker = NULL;
		return 0;
	}
	if (ob != Py_None && !PyType_stgdict(ob) && !PyCallable_Check(ob)) {
		PyErr_SetString(PyExc_TypeError,
				"restype must be a type, a callable, or None");
		return -1;
	}
	Py_XDECREF(self->checker);
	Py_XDECREF(self->restype);
	Py_INCREF(ob);
	self->restype = ob;
	self->checker = PyObject_GetAttrString(ob, "_check_retval_");
	if (self->checker == NULL)
		PyErr_Clear();
	return 0;
}

static PyObject *
CFuncPtr_get_restype(CFuncPtrObject *self)
{
	StgDictObject *dict;
	if (self->restype) {
		Py_INCREF(self->restype);
		return self->restype;
	}
	dict = PyObject_stgdict((PyObject *)self);
	assert(dict); /* Cannot be NULL for CFuncPtrObject instances */
	if (dict->restype) {
		Py_INCREF(dict->restype);
		return dict->restype;
	} else {
		Py_INCREF(Py_None);
		return Py_None;
	}
}

static int
CFuncPtr_set_argtypes(CFuncPtrObject *self, PyObject *ob)
{
	PyObject *converters;

	if (ob == NULL || ob == Py_None) {
		Py_XDECREF(self->converters);
		self->converters = NULL;
		Py_XDECREF(self->argtypes);
		self->argtypes = NULL;
	} else {
		converters = converters_from_argtypes(ob);
		if (!converters)
			return -1;
		Py_XDECREF(self->converters);
		self->converters = converters;
		Py_XDECREF(self->argtypes);
		Py_INCREF(ob);
		self->argtypes = ob;
	}
	return 0;
}

static PyObject *
CFuncPtr_get_argtypes(CFuncPtrObject *self)
{
	StgDictObject *dict;
	if (self->argtypes) {
		Py_INCREF(self->argtypes);
		return self->argtypes;
	}
	dict = PyObject_stgdict((PyObject *)self);
	assert(dict); /* Cannot be NULL for CFuncPtrObject instances */
	if (dict->argtypes) {
		Py_INCREF(dict->argtypes);
		return dict->argtypes;
	} else {
		Py_INCREF(Py_None);
		return Py_None;
	}
}

static PyGetSetDef CFuncPtr_getsets[] = {
	{ "errcheck", (getter)CFuncPtr_get_errcheck, (setter)CFuncPtr_set_errcheck,
	  "a function to check for errors", NULL },
	{ "restype", (getter)CFuncPtr_get_restype, (setter)CFuncPtr_set_restype,
	  "specify the result type", NULL },
	{ "argtypes", (getter)CFuncPtr_get_argtypes,
	  (setter)CFuncPtr_set_argtypes,
	  "specify the argument types", NULL },
	{ NULL, NULL }
};

#ifdef MS_WIN32
static PPROC FindAddress(void *handle, char *name, PyObject *type)
{
#ifdef MS_WIN64
	/* win64 has no stdcall calling conv, so it should
	   also not have the name mangling of it.
	*/
	return (PPROC)GetProcAddress(handle, name);
#else
	PPROC address;
	char *mangled_name;
	int i;
	StgDictObject *dict;

	address = (PPROC)GetProcAddress(handle, name);
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
		address = (PPROC)GetProcAddress(handle, mangled_name);
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

	if (PointerTypeObject_Check(arg))
		return 1;

	if (ArrayTypeObject_Check(arg))
		return 1;

	dict = PyType_stgdict(arg);
	if (dict
	    /* simple pointer types, c_void_p, c_wchar_p, BSTR, ... */
	    && PyUnicode_Check(dict->proto)
/* We only allow c_void_p, c_char_p and c_wchar_p as a simple output parameter type */
	    && (strchr("PzZ", PyUnicode_AsString(dict->proto)[0]))) {
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
	assert(dict); /* Cannot be NULL. 'type' is a CFuncPtr type. */
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
_get_name(PyObject *obj, char **pname)
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
	if (PyString_Check(obj)) {
		*pname = PyString_AS_STRING(obj);
		return *pname ? 1 : 0;
	}
	if (PyUnicode_Check(obj)) {
		*pname = PyUnicode_AsString(obj);
		return *pname ? 1 : 0;
	}
	PyErr_SetString(PyExc_TypeError,
			"function name must be string or integer");
	return 0;
}


static PyObject *
CFuncPtr_FromDll(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	char *name;
	int (* address)(void);
	PyObject *dll;
	PyObject *obj;
	CFuncPtrObject *self;
	void *handle;
	PyObject *paramflags = NULL;

	if (!PyArg_ParseTuple(args, "(O&O)|O", _get_name, &name, &dll, &paramflags))
		return NULL;
	if (paramflags == Py_None)
		paramflags = NULL;

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
		return NULL;
	}
#else
	address = (PPROC)ctypes_dlsym(handle, name);
	if (!address) {
		PyErr_Format(PyExc_AttributeError,
#ifdef __CYGWIN__
/* dlerror() isn't very helpful on cygwin */
			     "function '%s' not found (%s) ",
			     name,
#endif
			     ctypes_dlerror());
		return NULL;
	}
#endif
	if (!_validate_paramflags(type, paramflags))
		return NULL;

	self = (CFuncPtrObject *)GenericCData_new(type, args, kwds);
	if (!self)
		return NULL;

	Py_XINCREF(paramflags);
	self->paramflags = paramflags;

	*(void **)self->b_ptr = address;

	Py_INCREF((PyObject *)dll); /* for KeepRef */
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
CFuncPtr_FromVtblIndex(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	CFuncPtrObject *self;
	int index;
	char *name = NULL;
	PyObject *paramflags = NULL;
	GUID *iid = NULL;
	int iid_len = 0;

	if (!PyArg_ParseTuple(args, "is|Oz#", &index, &name, &paramflags, &iid, &iid_len))
		return NULL;
	if (paramflags == Py_None)
		paramflags = NULL;

	if (!_validate_paramflags(type, paramflags))
		return NULL;

	self = (CFuncPtrObject *)GenericCData_new(type, args, kwds);
	self->index = index + 0x1000;
	Py_XINCREF(paramflags);
	self->paramflags = paramflags;
	if (iid_len == sizeof(GUID))
		self->iid = iid;
	return (PyObject *)self;
}
#endif

/*
  CFuncPtr_new accepts different argument lists in addition to the standard
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
CFuncPtr_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	CFuncPtrObject *self;
	PyObject *callable;
	StgDictObject *dict;
	ffi_info *thunk;

	if (PyTuple_GET_SIZE(args) == 0)
		return GenericCData_new(type, args, kwds);

	if (1 <= PyTuple_GET_SIZE(args) && PyTuple_Check(PyTuple_GET_ITEM(args, 0)))
		return CFuncPtr_FromDll(type, args, kwds);

#ifdef MS_WIN32
	if (2 <= PyTuple_GET_SIZE(args) && PyLong_Check(PyTuple_GET_ITEM(args, 0)))
		return CFuncPtr_FromVtblIndex(type, args, kwds);
#endif

	if (1 == PyTuple_GET_SIZE(args)
	    && (PyLong_Check(PyTuple_GET_ITEM(args, 0)))) {
		CDataObject *ob;
		void *ptr = PyLong_AsVoidPtr(PyTuple_GET_ITEM(args, 0));
		if (ptr == NULL && PyErr_Occurred())
			return NULL;
		ob = (CDataObject *)GenericCData_new(type, args, kwds);
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

	/* XXX XXX This would allow to pass additional options.  For COM
	   method *implementations*, we would probably want different
	   behaviour than in 'normal' callback functions: return a HRESULT if
	   an exception occurrs in the callback, and print the traceback not
	   only on the console, but also to OutputDebugString() or something
	   like that.
	*/
/*
	if (kwds && PyDict_GetItemString(kwds, "options")) {
		...
	}
*/

	dict = PyType_stgdict((PyObject *)type);
	/* XXXX Fails if we do: 'CFuncPtr(lambda x: x)' */
	if (!dict || !dict->argtypes) {
		PyErr_SetString(PyExc_TypeError,
		       "cannot construct instance of this class:"
			" no argtypes");
		return NULL;
	}

	/*****************************************************************/
	/* The thunk keeps unowned references to callable and dict->argtypes
	   so we have to keep them alive somewhere else: callable is kept in self,
	   dict->argtypes is in the type's stgdict.
	*/
	thunk = AllocFunctionCallback(callable,
				      dict->argtypes,
				      dict->restype,
				      dict->flags & FUNCFLAG_CDECL);
	if (!thunk)
		return NULL;

	self = (CFuncPtrObject *)GenericCData_new(type, args, kwds);
	if (self == NULL)
		return NULL;

	Py_INCREF(callable);
	self->callable = callable;

	self->thunk = thunk;
	*(void **)self->b_ptr = *(void **)thunk;

	/* We store ourself in self->b_objects[0], because the whole instance
	   must be kept alive if stored in a structure field, for example.
	   Cycle GC to the rescue! And we have a unittest proving that this works
	   correctly...
	*/

	Py_INCREF((PyObject *)self); /* for KeepRef */
	if (-1 == KeepRef((CDataObject *)self, 0, (PyObject *)self)) {
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

	parg = new_CArgObject();
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
	if (kwds && (v = PyDict_GetItem(kwds, name))) {
		++*pindex;
		Py_INCREF(v);
		return v;
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
_build_callargs(CFuncPtrObject *self, PyObject *argtypes,
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
		int flag;
		PyObject *name = NULL;
		PyObject *defval = NULL;

		/* This way seems to be ~2 us faster than the PyArg_ParseTuple
		   calls below. */
		/* We HAVE already checked that the tuple can be parsed with "i|ZO", so... */
		Py_ssize_t tsize = PyTuple_GET_SIZE(item);
		flag = PyLong_AS_LONG(PyTuple_GET_ITEM(item, 0));
		name = tsize > 1 ? PyTuple_GET_ITEM(item, 1) : NULL;
		defval = tsize > 2 ? PyTuple_GET_ITEM(item, 2) : NULL;

		switch (flag & (PARAMFLAG_FIN | PARAMFLAG_FOUT | PARAMFLAG_FLCID)) {
		case PARAMFLAG_FIN | PARAMFLAG_FLCID:
			/* ['in', 'lcid'] parameter.  Always taken from defval,
			 if given, else the integer 0. */
			if (defval == NULL) {
				defval = PyLong_FromLong(0);
				if (defval == NULL)
					goto error;
			} else
				Py_INCREF(defval);
			PyTuple_SET_ITEM(callargs, i, defval);
			break;
		case (PARAMFLAG_FIN | PARAMFLAG_FOUT):
			*pinoutmask |= (1 << i); /* mark as inout arg */
			(*pnumretvals)++;
			/* fall through to PARAMFLAG_FIN... */
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
			if (ArrayTypeObject_Check(ob))
				ob = PyObject_CallObject(ob, NULL);
			else
				/* Create an instance of the pointed-to type */
				ob = PyObject_CallObject(dict->proto, NULL);
			/*			   
			   XXX Is the following correct any longer?
			   We must not pass a byref() to the array then but
			   the array instance itself. Then, we cannot retrive
			   the result from the PyCArgObject.
			*/
			if (ob == NULL)
				goto error;
			/* The .from_param call that will ocurr later will pass this
			   as a byref parameter. */
			PyTuple_SET_ITEM(callargs, i, ob);
			*poutmask |= (1 << i); /* mark as out arg */
			(*pnumretvals)++;
			break;
		default:
			PyErr_Format(PyExc_ValueError,
				     "paramflag %d not yet implemented", flag);
			goto error;
			break;
		}
	}

	/* We have counted the arguments we have consumed in 'inargs_index'.  This
	   must be the same as len(inargs) + len(kwds), otherwise we have
	   either too much or not enough arguments. */

	actual_args = PyTuple_GET_SIZE(inargs) + (kwds ? PyDict_Size(kwds) : 0);
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
			v = PyObject_CallMethod(v, "__ctypes_from_outparam__", NULL);
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
CFuncPtr_call(CFuncPtrObject *self, PyObject *inargs, PyObject *kwds)
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

	assert(dict); /* Cannot be NULL for CFuncPtrObject instances */
	restype = self->restype ? self->restype : dict->restype;
	converters = self->converters ? self->converters : dict->converters;
	checker = self->checker ? self->checker : dict->checker;
	argtypes = self->argtypes ? self->argtypes : dict->argtypes;
/* later, we probably want to have an errcheck field in stgdict */
	errcheck = self->errcheck /* ? self->errcheck : dict->errcheck */;


	pProc = *(void **)self->b_ptr;
	if (pProc == NULL) {
		PyErr_SetString(PyExc_ValueError,
				"attempt to call NULL function pointer");
		return NULL;
	}
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
		/* First arg is an pointer to an interface instance */
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

	result = _CallProc(pProc,
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
		/* If the errcheck funtion failed, return NULL.
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
CFuncPtr_traverse(CFuncPtrObject *self, visitproc visit, void *arg)
{
	Py_VISIT(self->callable);
	Py_VISIT(self->restype);
	Py_VISIT(self->checker);
	Py_VISIT(self->errcheck);
	Py_VISIT(self->argtypes);
	Py_VISIT(self->converters);
	Py_VISIT(self->paramflags);
	return CData_traverse((CDataObject *)self, visit, arg);
}

static int
CFuncPtr_clear(CFuncPtrObject *self)
{
	Py_CLEAR(self->callable);
	Py_CLEAR(self->restype);
	Py_CLEAR(self->checker);
	Py_CLEAR(self->errcheck);
	Py_CLEAR(self->argtypes);
	Py_CLEAR(self->converters);
	Py_CLEAR(self->paramflags);

	if (self->thunk) {
		FreeClosure(self->thunk->pcl);
		PyMem_Free(self->thunk);
		self->thunk = NULL;
	}

	return CData_clear((CDataObject *)self);
}

static void
CFuncPtr_dealloc(CFuncPtrObject *self)
{
	CFuncPtr_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
CFuncPtr_repr(CFuncPtrObject *self)
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

PyTypeObject CFuncPtr_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes.CFuncPtr",
	sizeof(CFuncPtrObject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)CFuncPtr_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)CFuncPtr_repr,		/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	(ternaryfunc)CFuncPtr_call,		/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	&CData_as_buffer,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"Function Pointer",			/* tp_doc */
	(traverseproc)CFuncPtr_traverse,	/* tp_traverse */
	(inquiry)CFuncPtr_clear,		/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	0,					/* tp_members */
	CFuncPtr_getsets,			/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
        CFuncPtr_new,				/* tp_new */
	0,					/* tp_free */
};

/*****************************************************************/
/*
  Struct_Type
*/
static int
IBUG(char *msg)
{
	PyErr_Format(PyExc_RuntimeError,
			"inconsistent state in CDataObject (%s)", msg);
	return -1;
}

static int
Struct_init(PyObject *self, PyObject *args, PyObject *kwds)
{
	int i;
	PyObject *fields;

/* Optimization possible: Store the attribute names _fields_[x][0]
 * in C accessible fields somewhere ?
 */

/* Check this code again for correctness! */

	if (!PyTuple_Check(args)) {
		PyErr_SetString(PyExc_TypeError,
				"args not a tuple?");
		return -1;
	}
	if (PyTuple_GET_SIZE(args)) {
		fields = PyObject_GetAttrString(self, "_fields_");
		if (!fields) {
			PyErr_Clear();
			fields = PyTuple_New(0);
			if (!fields)
				return -1;
		}

		if (PyTuple_GET_SIZE(args) > PySequence_Length(fields)) {
			Py_DECREF(fields);
			PyErr_SetString(PyExc_ValueError,
					"too many initializers");
			return -1;
		}

		for (i = 0; i < PyTuple_GET_SIZE(args); ++i) {
			PyObject *pair = PySequence_GetItem(fields, i);
			PyObject *name;
			PyObject *val;
			if (!pair) {
				Py_DECREF(fields);
				return IBUG("_fields_[i] failed");
			}

			name = PySequence_GetItem(pair, 0);
			if (!name) {
				Py_DECREF(pair);
				Py_DECREF(fields);
				return IBUG("_fields_[i][0] failed");
			}

			val = PyTuple_GET_ITEM(args, i);
			if (-1 == PyObject_SetAttr(self, name, val)) {
				Py_DECREF(pair);
				Py_DECREF(name);
				Py_DECREF(fields);
				return -1;
			}

			Py_DECREF(name);
			Py_DECREF(pair);
		}
		Py_DECREF(fields);
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
	sizeof(CDataObject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	0,					/* tp_dealloc */
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
	&CData_as_buffer,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"Structure base class",			/* tp_doc */
	(traverseproc)CData_traverse,		/* tp_traverse */
	(inquiry)CData_clear,			/* tp_clear */
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
	Struct_init,				/* tp_init */
	0,					/* tp_alloc */
	GenericCData_new,			/* tp_new */
	0,					/* tp_free */
};

static PyTypeObject Union_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes.Union",
	sizeof(CDataObject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	0,					/* tp_dealloc */
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
	&CData_as_buffer,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"Union base class",			/* tp_doc */
	(traverseproc)CData_traverse,		/* tp_traverse */
	(inquiry)CData_clear,			/* tp_clear */
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
	Struct_init,				/* tp_init */
	0,					/* tp_alloc */
	GenericCData_new,			/* tp_new */
	0,					/* tp_free */
};


/******************************************************************/
/*
  Array_Type
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
Array_item(PyObject *_self, Py_ssize_t index)
{
	CDataObject *self = (CDataObject *)_self;
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

	return CData_get(stgdict->proto, stgdict->getfunc, (PyObject *)self,
			 index, size, self->b_ptr + offset);
}

static PyObject *
Array_subscript(PyObject *_self, PyObject *item)
{
	CDataObject *self = (CDataObject *)_self;

	if (PyIndex_Check(item)) {
		Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		
		if (i == -1 && PyErr_Occurred())
			return NULL;
		if (i < 0)
			i += self->b_length;
		return Array_item(_self, i);
	}
	else if PySlice_Check(item) {
		StgDictObject *stgdict, *itemdict;
		PyObject *proto;
		PyObject *np;
		Py_ssize_t start, stop, step, slicelen, cur, i;
		
		if (PySlice_GetIndicesEx((PySliceObject *)item,
					 self->b_length, &start, &stop,
					 &step, &slicelen) < 0) {
			return NULL;
		}
		
		stgdict = PyObject_stgdict((PyObject *)self);
		assert(stgdict); /* Cannot be NULL for array object instances */
		proto = stgdict->proto;
		itemdict = PyType_stgdict(proto);
		assert(itemdict); /* proto is the item type of the array, a
				     ctypes type, so this cannot be NULL */

		if (itemdict->getfunc == getentry("c")->getfunc) {
			char *ptr = (char *)self->b_ptr;
			char *dest;

			if (slicelen <= 0)
				return PyString_FromStringAndSize("", 0);
			if (step == 1) {
				return PyString_FromStringAndSize(ptr + start,
								 slicelen);
			}
			dest = (char *)PyMem_Malloc(slicelen);

			if (dest == NULL)
				return PyErr_NoMemory();

			for (cur = start, i = 0; i < slicelen;
			     cur += step, i++) {
				dest[i] = ptr[cur];
			}

			np = PyString_FromStringAndSize(dest, slicelen);
			PyMem_Free(dest);
			return np;
		}
#ifdef CTYPES_UNICODE
		if (itemdict->getfunc == getentry("u")->getfunc) {
			wchar_t *ptr = (wchar_t *)self->b_ptr;
			wchar_t *dest;
			
			if (slicelen <= 0)
				return PyUnicode_FromUnicode(NULL, 0);
			if (step == 1) {
				return PyUnicode_FromWideChar(ptr + start,
							      slicelen);
			}

			dest = (wchar_t *)PyMem_Malloc(
						slicelen * sizeof(wchar_t));
			
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
			PyObject *v = Array_item(_self, cur);
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
Array_ass_item(PyObject *_self, Py_ssize_t index, PyObject *value)
{
	CDataObject *self = (CDataObject *)_self;
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

	return CData_set((PyObject *)self, stgdict->proto, stgdict->setfunc, value,
			 index, size, ptr);
}

static int
Array_ass_subscript(PyObject *_self, PyObject *item, PyObject *value)
{
	CDataObject *self = (CDataObject *)_self;
	
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
		return Array_ass_item(_self, i, value);
	}
	else if (PySlice_Check(item)) {
		Py_ssize_t start, stop, step, slicelen, otherlen, i, cur;
		
		if (PySlice_GetIndicesEx((PySliceObject *)item,
					 self->b_length, &start, &stop,
					 &step, &slicelen) < 0) {
			return -1;
		}
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
			result = Array_ass_item(_self, cur, item);
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
Array_length(PyObject *_self)
{
	CDataObject *self = (CDataObject *)_self;
	return self->b_length;
}

static PySequenceMethods Array_as_sequence = {
	Array_length,				/* sq_length; */
	0,					/* sq_concat; */
	0,					/* sq_repeat; */
	Array_item,				/* sq_item; */
	0,					/* sq_slice; */
	Array_ass_item,				/* sq_ass_item; */
	0,					/* sq_ass_slice; */
	0,					/* sq_contains; */
	
	0,					/* sq_inplace_concat; */
	0,					/* sq_inplace_repeat; */
};

static PyMappingMethods Array_as_mapping = {
	Array_length,
	Array_subscript,
	Array_ass_subscript,
};

PyTypeObject Array_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes.Array",
	sizeof(CDataObject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	0,					/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	&Array_as_sequence,			/* tp_as_sequence */
	&Array_as_mapping,			/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	&CData_as_buffer,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"XXX to be provided",			/* tp_doc */
	(traverseproc)CData_traverse,		/* tp_traverse */
	(inquiry)CData_clear,			/* tp_clear */
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
	(initproc)Array_init,			/* tp_init */
	0,					/* tp_alloc */
        GenericCData_new,			/* tp_new */
	0,					/* tp_free */
};

PyObject *
CreateArrayType(PyObject *itemtype, Py_ssize_t length)
{
	static PyObject *cache;
	PyObject *key;
	PyObject *result;
	char name[256];

	if (cache == NULL) {
		cache = PyDict_New();
		if (cache == NULL)
			return NULL;
	}
	key = Py_BuildValue("(On)", itemtype, length);
	if (!key)
		return NULL;
	result = PyDict_GetItem(cache, key);
	if (result) {
		Py_INCREF(result);
		Py_DECREF(key);
		return result;
	}

	if (!PyType_Check(itemtype)) {
		PyErr_SetString(PyExc_TypeError,
				"Expected a type object");
		return NULL;
	}
#ifdef MS_WIN64
	sprintf(name, "%.200s_Array_%Id",
		((PyTypeObject *)itemtype)->tp_name, length);
#else
	sprintf(name, "%.200s_Array_%ld",
		((PyTypeObject *)itemtype)->tp_name, (long)length);
#endif

	result = PyObject_CallFunction((PyObject *)&ArrayType_Type,
				       "U(O){s:n,s:O}",
				       name,
				       &Array_Type,
				       "_length_",
				       length,
				       "_type_",
				       itemtype
		);
	if (!result)
		return NULL;
	PyDict_SetItem(cache, key, result);
	Py_DECREF(key);
	return result;
}


/******************************************************************/
/*
  Simple_Type
*/

static int
Simple_set_value(CDataObject *self, PyObject *value)
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
		return Simple_set_value(self, value);
	return 0;
}

static PyObject *
Simple_get_value(CDataObject *self)
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
	if (IsSimpleSubType((PyObject *)Py_TYPE(self))) {
		Py_INCREF(self);
		return self;
	}
	/* call stgdict->getfunc */
	return Simple_get_value((CDataObject *)self);
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
	PyObject *val, *name, *args, *result;
	static PyObject *format;

	if (Py_TYPE(self)->tp_base != &Simple_Type) {
		return PyUnicode_FromFormat("<%s object at %p>",
					   Py_TYPE(self)->tp_name, self);
	}

	if (format == NULL) {
		format = PyUnicode_FromString("%s(%r)");
		if (format == NULL)
			return NULL;
	}

	val = Simple_get_value(self);
	if (val == NULL)
		return NULL;

	name = PyUnicode_FromString(Py_TYPE(self)->tp_name);
	if (name == NULL) {
		Py_DECREF(val);
		return NULL;
	}

	args = PyTuple_Pack(2, name, val);
	Py_DECREF(name);
	Py_DECREF(val);
	if (args == NULL)
		return NULL;

	result = PyUnicode_Format(format, args);
	Py_DECREF(args);
	return result;
}

static PyTypeObject Simple_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes._SimpleCData",
	sizeof(CDataObject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	0,					/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)&Simple_repr,			/* tp_repr */
	&Simple_as_number,			/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	&CData_as_buffer,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"XXX to be provided",			/* tp_doc */
	(traverseproc)CData_traverse,		/* tp_traverse */
	(inquiry)CData_clear,			/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	Simple_methods,				/* tp_methods */
	0,					/* tp_members */
	Simple_getsets,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	(initproc)Simple_init,			/* tp_init */
	0,					/* tp_alloc */
        GenericCData_new,			/* tp_new */
	0,					/* tp_free */
};

/******************************************************************/
/*
  Pointer_Type
*/
static PyObject *
Pointer_item(PyObject *_self, Py_ssize_t index)
{
	CDataObject *self = (CDataObject *)_self;
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

	return CData_get(proto, stgdict->getfunc, (PyObject *)self,
			 index, size, (*(char **)self->b_ptr) + offset);
}

static int
Pointer_ass_item(PyObject *_self, Py_ssize_t index, PyObject *value)
{
	CDataObject *self = (CDataObject *)_self;
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
	assert(stgdict); /* Cannot be NULL fr pointer instances */

	proto = stgdict->proto;
	assert(proto);

	itemdict = PyType_stgdict(proto);
	assert(itemdict); /* Cannot be NULL because the itemtype of a pointer
			     is always a ctypes type */

	size = itemdict->size;
	offset = index * itemdict->size;

	return CData_set((PyObject *)self, proto, stgdict->setfunc, value,
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
	assert(stgdict); /* Cannot be NULL fr pointer instances */
	return CData_FromBaseObj(stgdict->proto,
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
	assert(stgdict); /* Cannot be NULL fr pointer instances */
	assert(stgdict->proto);
	if (!CDataObject_Check(value) 
	    || 0 == PyObject_IsInstance(value, stgdict->proto)) {
		/* XXX PyObject_IsInstance could return -1! */
		PyErr_Format(PyExc_TypeError,
			     "expected %s instead of %s",
			     ((PyTypeObject *)(stgdict->proto))->tp_name,
			     Py_TYPE(value)->tp_name);
		return -1;
	}

	dst = (CDataObject *)value;
	*(void **)self->b_ptr = dst->b_ptr;

	/* 
	   A Pointer instance must keep a the value it points to alive.  So, a
	   pointer instance has b_length set to 2 instead of 1, and we set
	   'value' itself as the second item of the b_objects list, additionally.
	*/
	Py_INCREF(value);
	if (-1 == KeepRef(self, 1, value))
		return -1;

	keep = GetKeepedObjects(dst);
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

	if (!PyArg_ParseTuple(args, "|O:POINTER", &value))
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
	return GenericCData_new(type, args, kw);
}

static PyObject *
Pointer_subscript(PyObject *_self, PyObject *item)
{
	CDataObject *self = (CDataObject *)_self;
	if (PyIndex_Check(item)) {
		Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return NULL;
		return Pointer_item(_self, i);
	}
	else if (PySlice_Check(item)) {
		PySliceObject *slice = (PySliceObject *)item;
		Py_ssize_t start, stop, step;
		PyObject *np;
		StgDictObject *stgdict, *itemdict;
		PyObject *proto;
		Py_ssize_t i, len, cur;

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
		if (itemdict->getfunc == getentry("c")->getfunc) {
			char *ptr = *(char **)self->b_ptr;
			char *dest;
			
			if (len <= 0)
                        	return PyString_FromStringAndSize("", 0);
			if (step == 1) {
				return PyString_FromStringAndSize(ptr + start,
								 len);
			}
			dest = (char *)PyMem_Malloc(len);
			if (dest == NULL)
				return PyErr_NoMemory();
			for (cur = start, i = 0; i < len; cur += step, i++) {
				dest[i] = ptr[cur];
			}
			np = PyString_FromStringAndSize(dest, len);
			PyMem_Free(dest);
			return np;
		}
#ifdef CTYPES_UNICODE
		if (itemdict->getfunc == getentry("u")->getfunc) {
			wchar_t *ptr = *(wchar_t **)self->b_ptr;
			wchar_t *dest;
			
			if (len <= 0)
                        	return PyUnicode_FromUnicode(NULL, 0);
			if (step == 1) {
				return PyUnicode_FromWideChar(ptr + start,
							      len);
			}
			dest = (wchar_t *)PyMem_Malloc(len * sizeof(wchar_t));
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
			PyObject *v = Pointer_item(_self, cur);
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
	0,					/* inquiry sq_length; */
	0,					/* binaryfunc sq_concat; */
	0,					/* intargfunc sq_repeat; */
	Pointer_item,				/* intargfunc sq_item; */
	0,					/* intintargfunc sq_slice; */
	Pointer_ass_item,			/* intobjargproc sq_ass_item; */
	0,					/* intintobjargproc sq_ass_slice; */
	0,					/* objobjproc sq_contains; */
	/* Added in release 2.0 */
	0,					/* binaryfunc sq_inplace_concat; */
	0,					/* intargfunc sq_inplace_repeat; */
};

static PyMappingMethods Pointer_as_mapping = {
	0,
	Pointer_subscript,
};

static int
Pointer_bool(CDataObject *self)
{
	return *(void **)self->b_ptr != NULL;
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

PyTypeObject Pointer_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_ctypes._Pointer",
	sizeof(CDataObject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	0,					/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	&Pointer_as_number,			/* tp_as_number */
	&Pointer_as_sequence,			/* tp_as_sequence */
	&Pointer_as_mapping,			/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	&CData_as_buffer,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"XXX to be provided",			/* tp_doc */
	(traverseproc)CData_traverse,		/* tp_traverse */
	(inquiry)CData_clear,			/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	0,					/* tp_members */
	Pointer_getsets,			/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	(initproc)Pointer_init,			/* tp_init */
	0,					/* tp_alloc */
	Pointer_new,				/* tp_new */
	0,					/* tp_free */
};


/******************************************************************/
/*
 *  Module initialization.
 */

static char *module_docs =
"Create and manipulate C compatible data types in Python.";

#ifdef MS_WIN32

static char comerror_doc[] = "Raised when a COM method call failed.";

int
comerror_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *hresult, *text, *details;
    PyBaseExceptionObject *bself;
    PyObject *a;
    int status;

    if (!_PyArg_NoKeywords(Py_TYPE(self)->tp_name, kwds))
        return -1;

    if (!PyArg_ParseTuple(args, "OOO:COMError", &hresult, &text, &details))
	    return -1;

    a = PySequence_GetSlice(args, 1, PySequence_Size(args));
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

    bself = (PyBaseExceptionObject *)self;
    Py_DECREF(bself->args);
    bself->args = args;
    Py_INCREF(bself->args);

    return 0;
}

static PyTypeObject PyComError_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ctypes.COMError",         /* tp_name */
    sizeof(PyBaseExceptionObject), /* tp_basicsize */
    0,                          /* tp_itemsize */
    0,                          /* tp_dealloc */
    0,                          /* tp_print */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_compare */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
        | Py_TPFLAGS_HAVE_GC,   /* tp_flags */
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
	ComError = (PyObject*)&PyComError_Type;
	return 0;
}

#endif

static PyObject *
string_at(const char *ptr, int size)
{
	if (size == -1)
		return PyString_FromStringAndSize(ptr, strlen(ptr));
	return PyString_FromStringAndSize(ptr, size);
}

static int
cast_check_pointertype(PyObject *arg)
{
	StgDictObject *dict;

	if (PointerTypeObject_Check(arg))
		return 1;
	if (CFuncPtrTypeObject_Check(arg))
		return 1;
	dict = PyType_stgdict(arg);
	if (dict) {
		if (PyUnicode_Check(dict->proto)
		    && (strchr("sPzUZXO", PyUnicode_AsString(dict->proto)[0]))) {
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
	result = (CDataObject *)PyObject_CallFunctionObjArgs(ctype, NULL);
	if (result == NULL)
		return NULL;

	/*
	  The casted objects '_objects' member:

	  It must certainly contain the source objects one.
	  It must contain the source object itself.
	 */
	if (CDataObject_Check(src)) {
		CDataObject *obj = (CDataObject *)src;
		/* CData_GetContainer will initialize src.b_objects, we need
		   this so it can be shared */
		CData_GetContainer(obj);
		/* But we need a dictionary! */
		if (obj->b_objects == Py_None) {
			Py_DECREF(Py_None);
			obj->b_objects = PyDict_New();
			if (obj->b_objects == NULL)
				goto failed;
		}
		Py_XINCREF(obj->b_objects);
		result->b_objects = obj->b_objects;
		if (result->b_objects && PyDict_Check(result->b_objects)) {
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
	if (ssize == -1)
		ssize = wcslen(ptr);
	return PyUnicode_FromWideChar(ptr, ssize);
}
#endif

PyMODINIT_FUNC
init_ctypes(void)
{
	PyObject *m;

/* Note:
   ob_type is the metatype (the 'type'), defaults to PyType_Type,
   tp_base is the base type, defaults to 'object' aka PyBaseObject_Type.
*/
#ifdef WITH_THREAD
	PyEval_InitThreads();
#endif
	m = Py_InitModule3("_ctypes", module_methods, module_docs);
	if (!m)
		return;

	if (PyType_Ready(&PyCArg_Type) < 0)
		return;

	/* StgDict is derived from PyDict_Type */
	StgDict_Type.tp_base = &PyDict_Type;
	if (PyType_Ready(&StgDict_Type) < 0)
		return;

	/*************************************************
	 *
	 * Metaclasses
	 */

	StructType_Type.tp_base = &PyType_Type;
	if (PyType_Ready(&StructType_Type) < 0)
		return;

	UnionType_Type.tp_base = &PyType_Type;
	if (PyType_Ready(&UnionType_Type) < 0)
		return;

	PointerType_Type.tp_base = &PyType_Type;
	if (PyType_Ready(&PointerType_Type) < 0)
		return;

	ArrayType_Type.tp_base = &PyType_Type;
	if (PyType_Ready(&ArrayType_Type) < 0)
		return;

	SimpleType_Type.tp_base = &PyType_Type;
	if (PyType_Ready(&SimpleType_Type) < 0)
		return;

	CFuncPtrType_Type.tp_base = &PyType_Type;
	if (PyType_Ready(&CFuncPtrType_Type) < 0)
		return;

	/*************************************************
	 *
	 * Classes using a custom metaclass
	 */

	if (PyType_Ready(&CData_Type) < 0)
		return;

	Py_TYPE(&Struct_Type) = &StructType_Type;
	Struct_Type.tp_base = &CData_Type;
	if (PyType_Ready(&Struct_Type) < 0)
		return;
	PyModule_AddObject(m, "Structure", (PyObject *)&Struct_Type);

	Py_TYPE(&Union_Type) = &UnionType_Type;
	Union_Type.tp_base = &CData_Type;
	if (PyType_Ready(&Union_Type) < 0)
		return;
	PyModule_AddObject(m, "Union", (PyObject *)&Union_Type);

	Py_TYPE(&Pointer_Type) = &PointerType_Type;
	Pointer_Type.tp_base = &CData_Type;
	if (PyType_Ready(&Pointer_Type) < 0)
		return;
	PyModule_AddObject(m, "_Pointer", (PyObject *)&Pointer_Type);

	Py_TYPE(&Array_Type) = &ArrayType_Type;
	Array_Type.tp_base = &CData_Type;
	if (PyType_Ready(&Array_Type) < 0)
		return;
	PyModule_AddObject(m, "Array", (PyObject *)&Array_Type);

	Py_TYPE(&Simple_Type) = &SimpleType_Type;
	Simple_Type.tp_base = &CData_Type;
	if (PyType_Ready(&Simple_Type) < 0)
		return;
	PyModule_AddObject(m, "_SimpleCData", (PyObject *)&Simple_Type);

	Py_TYPE(&CFuncPtr_Type) = &CFuncPtrType_Type;
	CFuncPtr_Type.tp_base = &CData_Type;
	if (PyType_Ready(&CFuncPtr_Type) < 0)
		return;
	PyModule_AddObject(m, "CFuncPtr", (PyObject *)&CFuncPtr_Type);

	/*************************************************
	 *
	 * Simple classes
	 */

	/* CField_Type is derived from PyBaseObject_Type */
	if (PyType_Ready(&CField_Type) < 0)
		return;

	/*************************************************
	 *
	 * Other stuff
	 */

#ifdef MS_WIN32
	if (create_comerror() < 0)
		return;
	PyModule_AddObject(m, "COMError", ComError);

	PyModule_AddObject(m, "FUNCFLAG_HRESULT", PyLong_FromLong(FUNCFLAG_HRESULT));
	PyModule_AddObject(m, "FUNCFLAG_STDCALL", PyLong_FromLong(FUNCFLAG_STDCALL));
#endif
	PyModule_AddObject(m, "FUNCFLAG_CDECL", PyLong_FromLong(FUNCFLAG_CDECL));
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
#ifndef RTLD_LOCAL
#define RTLD_LOCAL 0
#endif

/* If RTLD_GLOBAL is not defined (cygwin), set it to the same value as
   RTLD_LOCAL.
*/
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL RTLD_LOCAL
#endif

	PyModule_AddObject(m, "RTLD_LOCAL", PyLong_FromLong(RTLD_LOCAL));
	PyModule_AddObject(m, "RTLD_GLOBAL", PyLong_FromLong(RTLD_GLOBAL));
	
	PyExc_ArgError = PyErr_NewException("ctypes.ArgumentError", NULL, NULL);
	if (PyExc_ArgError) {
		Py_INCREF(PyExc_ArgError);
		PyModule_AddObject(m, "ArgumentError", PyExc_ArgError);
	}
	/*************************************************
	 *
	 * Others...
	 */
	init_callbacks_in_module(m);
}

/*
 Local Variables:
 compile-command: "cd .. && python setup.py -q build -g && python setup.py -q build install --home ~"
 End:
*/
