#include "Python.h"
#include "compile.h"
#include "symtable.h"
#include "graminit.h"
#include "structmember.h"

/* The compiler uses this function to load a PySymtableEntry object
   for a code block.  Each block is loaded twice, once during the
   symbol table pass and once during the code gen pass.  Entries
   created during the first pass are cached for the second pass, using
   the st_symbols dictionary.  

   The cache is keyed by st_nscopes.  Each code block node in a
   module's parse tree can be assigned a unique id based on the order
   in which the nodes are visited by the compiler.  This strategy
   works so long as the symbol table and codegen passes visit the same
   nodes in the same order.
*/


PyObject *
PySymtableEntry_New(struct symtable *st, char *name, int type, int lineno)
{
	PySymtableEntryObject *ste = NULL;
	PyObject *k, *v;

	k = PyInt_FromLong(st->st_nscopes++);
	if (k == NULL)
		goto fail;
	v = PyDict_GetItem(st->st_symbols, k);
	if (v) {
		Py_DECREF(k);
		Py_INCREF(v);
		return v;
	}
	
	ste = (PySymtableEntryObject *)PyObject_New(PySymtableEntryObject,
						    &PySymtableEntry_Type);
	ste->ste_table = st;
	ste->ste_id = k;

	v = PyString_FromString(name);
	if (v == NULL)
		goto fail;
	ste->ste_name = v;
	
	v = PyDict_New();
	if (v == NULL)
	    goto fail;
	ste->ste_symbols = v;

	v = PyList_New(0);
	if (v == NULL)
	    goto fail;
	ste->ste_varnames = v;

	v = PyList_New(0);
	if (v == NULL)
	    goto fail;
	ste->ste_children = v;

	ste->ste_optimized = 0;
	ste->ste_opt_lineno = 0;
	ste->ste_tmpname = 0;
	ste->ste_lineno = lineno;
	switch (type) {
	case funcdef:
	case lambdef:
	case testlist_gexp: /* generator expression */
	case argument:      /* generator expression */
		ste->ste_type = TYPE_FUNCTION;
		break;
	case classdef:
		ste->ste_type = TYPE_CLASS;
		break;
	case single_input:
	case eval_input:
	case file_input:
		ste->ste_type = TYPE_MODULE;
		break;
	}

	if (st->st_cur == NULL)
		ste->ste_nested = 0;
	else if (st->st_cur->ste_nested 
		 || st->st_cur->ste_type == TYPE_FUNCTION)
		ste->ste_nested = 1;
	else
		ste->ste_nested = 0;
	ste->ste_child_free = 0;
	ste->ste_generator = 0;

	if (PyDict_SetItem(st->st_symbols, ste->ste_id, (PyObject *)ste) < 0)
	    goto fail;
	
	return (PyObject *)ste;
 fail:
	Py_XDECREF(ste);
	return NULL;
}

static PyObject *
ste_repr(PySymtableEntryObject *ste)
{
	char buf[256];

	PyOS_snprintf(buf, sizeof(buf),
		      "<symtable entry %.100s(%ld), line %d>",
		      PyString_AS_STRING(ste->ste_name),
		      PyInt_AS_LONG(ste->ste_id),
		      ste->ste_lineno);
	return PyString_FromString(buf);
}

static void
ste_dealloc(PySymtableEntryObject *ste)
{
	ste->ste_table = NULL;
	Py_XDECREF(ste->ste_id);
	Py_XDECREF(ste->ste_name);
	Py_XDECREF(ste->ste_symbols);
	Py_XDECREF(ste->ste_varnames);
	Py_XDECREF(ste->ste_children);
	PyObject_Del(ste);
}

#define OFF(x) offsetof(PySymtableEntryObject, x)

static PyMemberDef ste_memberlist[] = {
	{"id",       T_OBJECT, OFF(ste_id), READONLY},
	{"name",     T_OBJECT, OFF(ste_name), READONLY},
	{"symbols",  T_OBJECT, OFF(ste_symbols), READONLY},
	{"varnames", T_OBJECT, OFF(ste_varnames), READONLY},
	{"children", T_OBJECT, OFF(ste_children), READONLY},
	{"type",     T_INT,    OFF(ste_type), READONLY},
	{"lineno",   T_INT,    OFF(ste_lineno), READONLY},
	{"optimized",T_INT,    OFF(ste_optimized), READONLY},
	{"nested",   T_INT,    OFF(ste_nested), READONLY},
	{NULL}
};

PyTypeObject PySymtableEntry_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"symtable entry",
	sizeof(PySymtableEntryObject),
	0,
	(destructor)ste_dealloc,                /* tp_dealloc */
	0,                                      /* tp_print */
	0,			               /* tp_getattr */
	0,					/* tp_setattr */
	0,			                /* tp_compare */
	(reprfunc)ste_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,			                /* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,	                /* tp_flags */
 	0,					/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	ste_memberlist,				/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	0,					/* tp_new */
};
