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

/* Wrap void* pointers to be passed between C modules */

#include "Python.h"


/* Declarations for objects of type PyCObject */

typedef struct {
	PyObject_HEAD
	void *cobject;
	void (*destructor) Py_PROTO((void *));
} PyCObject;

PyObject *
PyCObject_FromVoidPtr(cobj, destr)
	void *cobj;
	void (*destr) Py_PROTO((void *));
{
	PyCObject *self;
	
	self = PyObject_NEW(PyCObject, &PyCObject_Type);
	if (self == NULL)
		return NULL;
	self->cobject=cobj;
	self->destructor=destr;
	return (PyObject *)self;
}

void *
PyCObject_AsVoidPtr(self)
	PyObject *self;
{
        if(self)
	  {
	    if(self->ob_type == &PyCObject_Type)
	      return ((PyCObject *)self)->cobject;
	    PyErr_SetString(PyExc_TypeError,
			    "PyCObject_AsVoidPtr with non-C-object");
	  }
	if(! PyErr_Occurred())
	    PyErr_SetString(PyExc_TypeError,
			    "PyCObject_AsVoidPtr called with null pointer");
	return NULL;
}

void *
PyCObject_Import(module_name, name)
     char *module_name;
     char *name;
{
  PyObject *m, *c;
  void *r=NULL;
  
  if(m=PyImport_ImportModule(module_name))
    {
      if(c=PyObject_GetAttrString(m,name))
	{
	  r=PyCObject_AsVoidPtr(c);
	  Py_DECREF(c);
	}
      Py_DECREF(m);
    }

  return r;
}

static void
PyCObject_dealloc(self)
	PyCObject *self;
{
        if(self->destructor) (self->destructor)(self->cobject);
	PyMem_DEL(self);
}


static char PyCObject_Type__doc__[] = 
"C objects to be exported from one extension module to another\n\
\n\
C objects are used for communication between extension modules.  They\n\
provide a way for an extension module to export a C interface to other\n\
extension modules, so that extension modules can use the Python import\n\
mechanism to link to one another.\n"
;

PyTypeObject PyCObject_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"PyCObject",			/*tp_name*/
	sizeof(PyCObject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)PyCObject_dealloc,	/*tp_dealloc*/
	(printfunc)0,		/*tp_print*/
	(getattrfunc)0,	/*tp_getattr*/
	(setattrfunc)0,	/*tp_setattr*/
	(cmpfunc)0,		/*tp_compare*/
	(reprfunc)0,		/*tp_repr*/
	0,			/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)0,		/*tp_hash*/
	(ternaryfunc)0,		/*tp_call*/
	(reprfunc)0,		/*tp_str*/

	/* Space for future expansion */
	0L,0L,0L,0L,
	PyCObject_Type__doc__ /* Documentation string */
};
