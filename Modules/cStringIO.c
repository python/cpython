/*

  $Id$

  A simple fast partial StringIO replacement.



     Copyright 

       Copyright 1996 Digital Creations, L.C., 910 Princess Anne
       Street, Suite 300, Fredericksburg, Virginia 22401 U.S.A. All
       rights reserved.  Copyright in this software is owned by DCLC,
       unless otherwise indicated. Permission to use, copy and
       distribute this software is hereby granted, provided that the
       above copyright notice appear in all copies and that both that
       copyright notice and this permission notice appear. Note that
       any product, process or technology described in this software
       may be the subject of other Intellectual Property rights
       reserved by Digital Creations, L.C. and are not licensed
       hereunder.

     Trademarks 

       Digital Creations & DCLC, are trademarks of Digital Creations, L.C..
       All other trademarks are owned by their respective companies. 

     No Warranty 

       The software is provided "as is" without warranty of any kind,
       either express or implied, including, but not limited to, the
       implied warranties of merchantability, fitness for a particular
       purpose, or non-infringement. This software could include
       technical inaccuracies or typographical errors. Changes are
       periodically made to the software; these changes will be
       incorporated in new editions of the software. DCLC may make
       improvements and/or changes in this software at any time
       without notice.

     Limitation Of Liability 

       In no event will DCLC be liable for direct, indirect, special,
       incidental, economic, cover, or consequential damages arising
       out of the use of or inability to use this software even if
       advised of the possibility of such damages. Some states do not
       allow the exclusion or limitation of implied warranties or
       limitation of liability for incidental or consequential
       damages, so the above limitation or exclusion may not apply to
       you.

    If you have questions regarding this software,
    contact:
   
      info@digicool.com
      Digital Creations L.C.  
   
      (540) 371-6909


*/
static char cStringIO_module_documentation[] = 
"A simple fast partial StringIO replacement.\n"
"\n"
"This module provides a simple useful replacement for\n"
"the StringIO module that is written in C.  It does not provide the\n"
"full generality if StringIO, but it provides anough for most\n"
"applications and is especially useful in conjuction with the\n"
"pickle module.\n"
"\n"
"Usage:\n"
"\n"
"  from cStringIO import StringIO\n"
"\n"
"  an_output_stream=StringIO()\n"
"  an_output_stream.write(some_stuff)\n"
"  ...\n"
"  value=an_output_stream.getvalue() # str(an_output_stream) works too!\n"
"\n"
"  an_input_stream=StringIO(a_string)\n"
"  spam=an_input_stream.readline()\n"
"  spam=an_input_stream.read(5)\n"
"  an_input_stream.reset()           # OK, start over, note no seek yet\n"
"  spam=an_input_stream.read()       # and read it all\n"
"  \n"
"If someone else wants to provide a more complete implementation,\n"
"go for it. :-)  \n"
"\n"
"$Id$\n"
;

#include "Python.h"
#include "import.h"
#include "cStringIO.h"

#define UNLESS(E) if(!(E))

/* Declarations for objects of type StringO */

typedef struct {
  PyObject_HEAD
  char *buf;
  int pos, string_size, buf_size, closed, softspace;
} Oobject;

/* Declarations for objects of type StringI */

typedef struct {
  PyObject_HEAD
  char *buf;
  int pos, string_size, closed;
  PyObject *pbuf;
} Iobject;

static char O_reset__doc__[] = 
"reset() -- Reset the file position to the beginning"
;

static PyObject *
O_reset(Oobject *self, PyObject *args) {
  self->pos = 0;

  Py_INCREF(Py_None);
  return Py_None;
}


static char O_tell__doc__[] =
"tell() -- get the current position.";

static PyObject *
O_tell(Oobject *self, PyObject *args) {
  return PyInt_FromLong(self->pos);
}


static char O_seek__doc__[] =
"seek(position)       -- set the current position\n"
"seek(position, mode) -- mode 0: absolute; 1: relative; 2: relative to EOF";

static PyObject *
O_seek(Oobject *self, PyObject *args) {
  int position, mode = 0;

  UNLESS(PyArg_ParseTuple(args, "i|i", &position, &mode))
  {
    return NULL;
  }

  if (mode == 2)
  {
    position += self->string_size;
  }
  else if (mode == 1)
  {
    position += self->pos;
  }

  self->pos = (position > self->string_size ? self->string_size : 
	       (position < 0 ? 0 : position));

  Py_INCREF(Py_None);
  return Py_None;
}

static char O_read__doc__[] = 
"read([s]) -- Read s characters, or the rest of the string"
;

static int
O_cread(PyObject *self, char **output, int  n) {
  int l;

  l = ((Oobject*)self)->string_size - ((Oobject*)self)->pos;  
  if (n < 0 || n > l)
  {
    n = l;
  }

  *output=((Oobject*)self)->buf + ((Oobject*)self)->pos;
  ((Oobject*)self)->pos += n;
  return n;
}

static PyObject *
O_read(Oobject *self, PyObject *args) {
  int n = -1;
  char *output;

  UNLESS(PyArg_ParseTuple(args, "|i", &n)) return NULL;

  n=O_cread((PyObject*)self,&output,n);

  return PyString_FromStringAndSize(output, n);
}


static char O_readline__doc__[] = 
"readline() -- Read one line"
;

static int
O_creadline(PyObject *self, char **output) {
  char *n, *s;
  int l;

  for (n = ((Oobject*)self)->buf + ((Oobject*)self)->pos,
	 s = ((Oobject*)self)->buf + ((Oobject*)self)->string_size; 
       n < s && *n != '\n'; n++);
  if (n < s) n++;
 
  *output=((Oobject*)self)->buf + ((Oobject*)self)->pos;
  l = n - ((Oobject*)self)->buf - ((Oobject*)self)->pos;
  ((Oobject*)self)->pos += l;
  return l;
}

static PyObject *
O_readline(Oobject *self, PyObject *args) {
  int n;
  char *output;

  n=O_creadline((PyObject*)self,&output);
  return PyString_FromStringAndSize(output, n);
}

static char O_write__doc__[] = 
"write(s) -- Write a string to the file"
"\n\nNote (hack:) writing None resets the buffer"
;


static int
O_cwrite(PyObject *self, char *c, int  l) {
  int newl;

  newl=((Oobject*)self)->pos+l;
  if(newl >= ((Oobject*)self)->buf_size)
    {
      ((Oobject*)self)->buf_size*=2;
      if(((Oobject*)self)->buf_size <= newl) ((Oobject*)self)->buf_size=newl+1;
      UNLESS(((Oobject*)self)->buf=
	     (char*)realloc(((Oobject*)self)->buf,
			    (((Oobject*)self)->buf_size) *sizeof(char)))
	{
	  PyErr_SetString(PyExc_MemoryError,"out of memory");
	  ((Oobject*)self)->buf_size=((Oobject*)self)->pos=0;
	  return -1;
	}
    }

  memcpy(((Oobject*)((Oobject*)self))->buf+((Oobject*)self)->pos,c,l);

  ((Oobject*)self)->pos += l;

  if (((Oobject*)self)->string_size < ((Oobject*)self)->pos)
    {
      ((Oobject*)self)->string_size = ((Oobject*)self)->pos;
    }

  return l;
}

static PyObject *
O_write(Oobject *self, PyObject *args) {
  PyObject *s;
  char *c;
  int l;

  UNLESS(PyArg_Parse(args, "O", &s)) return NULL;
  UNLESS(-1 != (l=PyString_Size(s))) return NULL;
  UNLESS(c=PyString_AsString(s)) return NULL;
  UNLESS(-1 != O_cwrite((PyObject*)self,c,l)) return NULL;

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
O_getval(Oobject *self, PyObject *args) {
  PyObject *use_pos;
  int s;

  use_pos=Py_None;
  UNLESS(PyArg_ParseTuple(args,"|O",&use_pos)) return NULL;
  if(PyObject_IsTrue(use_pos)) s=self->pos;
  else                         s=self->string_size;
  return PyString_FromStringAndSize(self->buf, s);
}

static PyObject *
O_cgetval(PyObject *self) {
  return PyString_FromStringAndSize(((Oobject*)self)->buf,
				    ((Oobject*)self)->pos);
}

static char O_truncate__doc__[] = 
"truncate(): truncate the file at the current position.";

static PyObject *
O_truncate(Oobject *self, PyObject *args) {
  self->string_size = self->pos;
  Py_INCREF(Py_None);
  return Py_None;
}

static char O_isatty__doc__[] = "isatty(): always returns 0";

static PyObject *
O_isatty(Oobject *self, PyObject *args) {
  return PyInt_FromLong(0);
}

static char O_close__doc__[] = "close(): explicitly release resources held.";

static PyObject *
O_close(Oobject *self, PyObject *args) {
  free(self->buf);

  self->pos = self->string_size = self->buf_size = 0;
  self->closed = 1;

  Py_INCREF(Py_None);
  return Py_None;
}

static char O_flush__doc__[] = "flush(): does nothing.";

static PyObject *
O_flush(Oobject *self, PyObject *args) {
  Py_INCREF(Py_None);
  return Py_None;
}


static char O_writelines__doc__[] = "blah";
static PyObject *
O_writelines(Oobject *self, PyObject *args) {
  PyObject *string_module = 0;
  static PyObject *string_joinfields = 0;

  UNLESS(PyArg_Parse(args, "O", args))
  {
    return NULL;
  }

  if (!string_joinfields)
  {
    UNLESS(string_module = PyImport_ImportModule("string"))
    {
      return NULL;
    }

    UNLESS(string_joinfields=
        PyObject_GetAttrString(string_module, "joinfields"))
    {
      return NULL;
    }

    Py_DECREF(string_module);
  }

  if (PyObject_Length(args) == -1)
  {
    return NULL;
  }

  return O_write(self, 
      PyObject_CallFunction(string_joinfields, "Os", args, ""));
}

static struct PyMethodDef O_methods[] = {
  {"write",	 (PyCFunction)O_write,        0,      O_write__doc__},
  {"read",       (PyCFunction)O_read,         1,      O_read__doc__},
  {"readline",   (PyCFunction)O_readline,     0,      O_readline__doc__},
  {"reset",      (PyCFunction)O_reset,        0,      O_reset__doc__},
  {"seek",       (PyCFunction)O_seek,         1,      O_seek__doc__},
  {"tell",       (PyCFunction)O_tell,         0,      O_tell__doc__},
  {"getvalue",   (PyCFunction)O_getval,       1,
   "getvalue([use_pos]) -- Get the string value."
   "\n"
   "If use_pos is specified and is a true value, then the string returned\n"
   "will include only the text up to the current file position.\n"
  },
  {"truncate",   (PyCFunction)O_truncate,     0,      O_truncate__doc__},
  {"isatty",     (PyCFunction)O_isatty,       0,      O_isatty__doc__},
  {"close",      (PyCFunction)O_close,        0,      O_close__doc__},
  {"flush",      (PyCFunction)O_flush,        0,      O_flush__doc__},
  {"writelines", (PyCFunction)O_writelines,   0,      O_writelines__doc__},
  {NULL,		NULL}		/* sentinel */
};


static void
O_dealloc(Oobject *self) {
  free(self->buf);
  PyMem_DEL(self);
}

static PyObject *
O_getattr(Oobject *self, char *name) {
  if (strcmp(name, "softspace") == 0) {
	  return PyInt_FromLong(self->softspace);
  }
  return Py_FindMethod(O_methods, (PyObject *)self, name);
}

static int
O_setattr(Oobject *self, char *name, PyObject *value) {
	long x;
	if (strcmp(name, "softspace") != 0) {
		PyErr_SetString(PyExc_AttributeError, name);
		return -1;
	}
	x = PyInt_AsLong(value);
	if (x == -1 && PyErr_Occurred())
		return -1;
	self->softspace = x;
	return 0;
}

static char Otype__doc__[] = 
"Simple type for output to strings."
;

static PyTypeObject Otype = {
  PyObject_HEAD_INIT(NULL)
  0,	       		/*ob_size*/
  "StringO",     		/*tp_name*/
  sizeof(Oobject),       	/*tp_basicsize*/
  0,	       		/*tp_itemsize*/
  /* methods */
  (destructor)O_dealloc,	/*tp_dealloc*/
  (printfunc)0,		/*tp_print*/
  (getattrfunc)O_getattr,	/*tp_getattr*/
  (setattrfunc)O_setattr,	/*tp_setattr*/
  (cmpfunc)0,		/*tp_compare*/
  (reprfunc)0,		/*tp_repr*/
  0,			/*tp_as_number*/
  0,			/*tp_as_sequence*/
  0,			/*tp_as_mapping*/
  (hashfunc)0,		/*tp_hash*/
  (ternaryfunc)0,		/*tp_call*/
  (reprfunc)0,		/*tp_str*/
  
  /* Space for future expansion */
  0L,0L,0L,0L,
  Otype__doc__ 		/* Documentation string */
};

static PyObject *
newOobject(int  size) {
  Oobject *self;
	
  self = PyObject_NEW(Oobject, &Otype);
  if (self == NULL)
    return NULL;
  self->pos=0;
  self->closed = 0;
  self->string_size = 0;
  self->softspace = 0;

  UNLESS(self->buf=malloc(size*sizeof(char)))
    {
      PyErr_SetString(PyExc_MemoryError,"out of memory");
      self->buf_size = 0;
      return NULL;
    }

  self->buf_size=size;
  return (PyObject*)self;
}

/* End of code for StringO objects */
/* -------------------------------------------------------- */

static PyObject *
I_close(Iobject *self, PyObject *args) {
  Py_DECREF(self->pbuf);

  self->pos = self->string_size = 0;
  self->closed = 1;

  Py_INCREF(Py_None);
  return Py_None;
}

static struct PyMethodDef I_methods[] = {
  {"read",	(PyCFunction)O_read,         1,      O_read__doc__},
  {"readline",	(PyCFunction)O_readline,	0,	O_readline__doc__},
  {"reset",	(PyCFunction)O_reset,	0,	O_reset__doc__},
  {"seek",      (PyCFunction)O_seek,         1,      O_seek__doc__},  
  {"tell",      (PyCFunction)O_tell,         0,      O_tell__doc__},
  {"truncate",  (PyCFunction)O_truncate,     0,      O_truncate__doc__},
  {"isatty",    (PyCFunction)O_isatty,       0,      O_isatty__doc__},
  {"close",     (PyCFunction)I_close,        0,      O_close__doc__},
  {"flush",     (PyCFunction)O_flush,        0,      O_flush__doc__},
  {NULL,		NULL}		/* sentinel */
};

static void
I_dealloc(Iobject *self) {
  Py_DECREF(self->pbuf);
  PyMem_DEL(self);
}

static PyObject *
I_getattr(Iobject *self, char *name) {
  return Py_FindMethod(I_methods, (PyObject *)self, name);
}

static char Itype__doc__[] = 
"Simple type for treating strings as input file streams"
;

static PyTypeObject Itype = {
  PyObject_HEAD_INIT(NULL)
  0,		       	/*ob_size*/
  "StringI",	       	/*tp_name*/
  sizeof(Iobject),       	/*tp_basicsize*/
  0,		       	/*tp_itemsize*/
  /* methods */
  (destructor)I_dealloc,	/*tp_dealloc*/
  (printfunc)0,		/*tp_print*/
  (getattrfunc)I_getattr,	/*tp_getattr*/
  (setattrfunc)0,		/*tp_setattr*/
  (cmpfunc)0,		/*tp_compare*/
  (reprfunc)0,		/*tp_repr*/
  0,			/*tp_as_number*/
  0,			/*tp_as_sequence*/
  0,			/*tp_as_mapping*/
  (hashfunc)0,		/*tp_hash*/
  (ternaryfunc)0,		/*tp_call*/
  (reprfunc)0,		/*tp_str*/
  
  /* Space for future expansion */
  0L,0L,0L,0L,
  Itype__doc__ 		/* Documentation string */
};

static PyObject *
newIobject(PyObject *s) {
  Iobject *self;
  char *buf;
  int size;
	
  UNLESS(buf=PyString_AsString(s)) return NULL;
  UNLESS(-1 != (size=PyString_Size(s))) return NULL;
  UNLESS(self = PyObject_NEW(Iobject, &Itype)) return NULL;
  Py_INCREF(s);
  self->buf=buf;
  self->string_size=size;
  self->pbuf=s;
  self->pos=0;
  self->closed = 0;
  
  return (PyObject*)self;
}

/* End of code for StringI objects */
/* -------------------------------------------------------- */


static char IO_StringIO__doc__[] =
"StringIO([s]) -- Return a StringIO-like stream for reading or writing"
;

static PyObject *
IO_StringIO(PyObject *self, PyObject *args) {
  PyObject *s=0;

  UNLESS(PyArg_ParseTuple(args, "|O", &s)) return NULL;
  if(s) return newIobject(s);
  return newOobject(128);
}

/* List of methods defined in the module */

static struct PyMethodDef IO_methods[] = {
  {"StringIO",	(PyCFunction)IO_StringIO,	1,	IO_StringIO__doc__},
  {NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initcStringIO) */

static struct PycStringIO_CAPI CAPI = {
  O_cread,
  O_creadline,
  O_cwrite,
  O_cgetval,
  newOobject,
  newIobject,
  &Itype,
  &Otype,
};

void
initcStringIO() {
  PyObject *m, *d;


  /* Create the module and add the functions */
  m = Py_InitModule4("cStringIO", IO_methods,
		     cStringIO_module_documentation,
		     (PyObject*)NULL,PYTHON_API_VERSION);

  /* Add some symbolic constants to the module */
  d = PyModule_GetDict(m);
  
  /* Export C API */
  Itype.ob_type=&PyType_Type;
  Otype.ob_type=&PyType_Type;
  PyDict_SetItemString(d,"cStringIO_CAPI", PyCObject_FromVoidPtr(&CAPI,NULL));

  /* Export Types */
  PyDict_SetItemString(d,"InputType",  (PyObject*)&Itype);
  PyDict_SetItemString(d,"OutputType", (PyObject*)&Otype);

  /* Maybe make certain warnings go away */
  if(0) PycString_IMPORT;
  
  /* Check for errors */
  if (PyErr_Occurred()) Py_FatalError("can't initialize module cStringIO");
}


/******************************************************************************

  $Log$
  Revision 2.6  1997/08/13 03:14:41  guido
  cPickle release 0.3 from Jim Fulton

  Revision 1.21  1997/06/19 18:51:42  jim
  Added ident string.

  Revision 1.20  1997/06/13 20:50:50  jim
  - Various changes to make gcc -Wall -pedantic happy, including
    getting rid of staticforward declarations and adding pretend use
    of two statics defined in .h file.

  Revision 1.19  1997/06/02 18:15:17  jim
  Merged in guido's changes.

  Revision 1.18  1997/05/07 16:26:47  jim
  getvalue() can nor be given an argument.  If this argument is true,
  then getvalue returns the text upto the current position.  Otherwise
  it returns all of the text.  The default value of the argument is
  false.

  Revision 1.17  1997/04/17 18:02:46  chris
  getvalue() now returns entire string, not just the string up to
  current position

  Revision 2.5  1997/04/11 19:56:06  guido
  My own patch: support writable 'softspace' attribute.

  > Jim asked: What is softspace for?
  
  It's an old feature.  The print statement uses this to remember
  whether it should insert a space before the next item or not.
  Implementation is in fileobject.c.

  Revision 1.11  1997/01/23 20:45:01  jim
  ANSIfied it.
  Changed way C API was exported.

  Revision 1.10  1997/01/02 15:19:55  chris
  checked in to be sure repository is up to date.

  Revision 1.9  1996/12/27 21:40:29  jim
  Took out some lamosities in interface, like returning self from
  write.

  Revision 1.8  1996/12/23 15:52:49  jim
  Added ifdef to check for CObject before using it.

  Revision 1.7  1996/12/23 15:22:35  jim
  Finished implementation, adding full compatibility with StringIO, and
  then some.

 *****************************************************************************/
