/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Type object implementation */

#include "Python.h"

/* Type object implementation */

static PyObject *
type_getattr(t, name)
	PyTypeObject *t;
	char *name;
{
	if (strcmp(name, "__name__") == 0)
		return PyString_FromString(t->tp_name);
	if (strcmp(name, "__doc__") == 0) {
		char *doc = t->tp_doc;
		if (doc != NULL)
			return PyString_FromString(doc);
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (strcmp(name, "__members__") == 0)
		return Py_BuildValue("[ss]", "__doc__", "__name__");
	PyErr_SetString(PyExc_AttributeError, name);
	return NULL;
}

static PyObject *
type_repr(v)
	PyTypeObject *v;
{
	char buf[100];
	sprintf(buf, "<type '%.80s'>", v->tp_name);
	return PyString_FromString(buf);
}

PyTypeObject PyType_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/* Number of items for varobject */
	"type",			/* Name of this type */
	sizeof(PyTypeObject),	/* Basic object size */
	0,			/* Item size for varobject */
	0,			/*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)type_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	(reprfunc)type_repr,	/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	0,			/*tp_xxx1*/
	0,			/*tp_xxx2*/
	0,			/*tp_xxx3*/
	0,			/*tp_xxx4*/
	"Define the behavior of a particular type of object.",
};
