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
   
      Jim Fulton, jim@digicool.com
      Digital Creations L.C.  
   
      (540) 371-6909


  $Log$
  Revision 2.2  1997/01/06 22:57:52  guido
  Jim's latest version.

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

  We still need to take out some cStringIO oddities.

  Revision 1.6  1996/10/15 18:42:07  jim
  Added lots of casts to make warnings go away.

  Revision 1.5  1996/10/11 21:03:42  jim
  *** empty log message ***

  Revision 1.4  1996/10/11 21:02:15  jim
  *** empty log message ***

  Revision 1.3  1996/10/07 20:51:38  chris
  *** empty log message ***

  Revision 1.2  1996/07/18 13:08:34  jfulton
  *** empty log message ***

  Revision 1.1  1996/07/15 17:06:33  jfulton
  Initial version.


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
;

#include "Python.h"
#include "import.h"

static PyObject *ErrorObject;

#ifdef __cplusplus
#define ARG(T,N) T N
#define ARGDECL(T,N)
#else
#define ARG(T,N) N
#define ARGDECL(T,N) T N;
#endif

#define ASSIGN(V,E) {PyObject *__e; __e=(E); Py_XDECREF(V); (V)=__e;}
#define UNLESS(E) if(!(E))
#define UNLESS_ASSIGN(V,E) ASSIGN(V,E) UNLESS(V)
#define Py_ASSIGN(P,E) if(!PyObject_AssignExpression(&(P),(E))) return NULL

/* ----------------------------------------------------- */

/* Declarations for objects of type StringO */

typedef struct {
  PyObject_HEAD
  char *buf;
  int pos, string_size, buf_size, closed;
} Oobject;

staticforward PyTypeObject Otype;

/* ---------------------------------------------------------------- */

/* Declarations for objects of type StringI */

typedef struct {
  PyObject_HEAD
  char *buf;
  int pos, string_size, closed;
  PyObject *pbuf;
} Iobject;

staticforward PyTypeObject Itype;



/* ---------------------------------------------------------------- */

static char O_reset__doc__[] = 
"reset() -- Reset the file position to the beginning"
;

static PyObject *
O_reset(ARG(Oobject*, self), ARG(PyObject*, args))
    ARGDECL(Oobject*, self)
    ARGDECL(PyObject*, args)
{
  self->pos = 0;

  Py_INCREF(Py_None);
  return Py_None;
}


static char O_tell__doc__[] =
"tell() -- get the current position.";

static PyObject *
O_tell(ARG(Oobject*, self), ARG(PyObject*, args))
    ARGDECL(Oobject*, self)
    ARGDECL(PyObject*, args)
{
  return PyInt_FromLong(self->pos);
}


static char O_seek__doc__[] =
"seek(position)       -- set the current position\n"
"seek(position, mode) -- mode 0: absolute; 1: relative; 2: relative to EOF";

static PyObject *
O_seek(ARG(Oobject*, self), ARG(PyObject*, args))
    ARGDECL(Oobject*, self)
    ARGDECL(PyObject*, args)
{
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
O_cread(ARG(Oobject*, self), ARG(char**, output), ARG(int, n))
    ARGDECL(Oobject*, self)
    ARGDECL(char**, output)
    ARGDECL(int, n)
{
  int l;

  l = self->string_size - self->pos;  
  if (n < 0 || n > l)
  {
    n = l;
  }

  *output=self->buf + self->pos;
  self->pos += n;
  return n;
}

static PyObject *
O_read(ARG(Oobject*, self), ARG(PyObject*, args))
    ARGDECL(Oobject*, self)
    ARGDECL(PyObject*, args)
{
  int n = -1;
  char *output;

  UNLESS(PyArg_ParseTuple(args, "|i", &n)) return NULL;

  n=O_cread(self,&output,n);

  return PyString_FromStringAndSize(output, n);
}


static char O_readline__doc__[] = 
"readline() -- Read one line"
;

static int
O_creadline(ARG(Oobject*, self), ARG(char**, output))
    ARGDECL(Oobject*, self)
    ARGDECL(char**, output)
{
  char *n, *s;
  int l;

  for (n = self->buf + self->pos, s = self->buf + self->string_size; 
       n < s && *n != '\n'; n++);
  if (n < s) n++;
 
  *output=self->buf + self->pos;
  l = n - self->buf - self->pos;
  self->pos += l;
  return l;
}

static PyObject *
O_readline(ARG(Oobject*, self), ARG(PyObject*, args))
    ARGDECL(Oobject*, self)
    ARGDECL(PyObject*, args)
{
  int n;
  char *output;

  n=O_creadline(self,&output);
  return PyString_FromStringAndSize(output, n);
}

static char O_write__doc__[] = 
"write(s) -- Write a string to the file"
"\n\nNote (hack:) writing None resets the buffer"
;


static int
O_cwrite(ARG(Oobject*, self), ARG(char*, c), ARG(int, l))
    ARGDECL(Oobject*, self)
    ARGDECL(char*, c)
    ARGDECL(int, l)
{
  PyObject *s;
  char *b;
  int newl, space_needed;

  newl=self->pos+l;
  if(newl > self->buf_size)
    {
      self->buf_size*=2;
      if(self->buf_size < newl) self->buf_size=newl;
      UNLESS(self->buf=(char*)realloc(self->buf, self->buf_size*sizeof(char)))
	{
	  PyErr_SetString(PyExc_MemoryError,"out of memory");
	  self->buf_size=self->pos=0;
	  return -1;
	}
    }

  memcpy(self->buf+self->pos,c,l);

  self->pos += l;

  if (self->string_size < self->pos)
    {
      self->string_size = self->pos;
    }

  return l;
}

static PyObject *
O_write(ARG(Oobject*, self), ARG(PyObject*, args))
    ARGDECL(Oobject*, self)
    ARGDECL(PyObject*, args)
{
  PyObject *s;
  char *c, *b;
  int l, newl, space_needed;

  UNLESS(PyArg_Parse(args, "O", &s)) return NULL;
  UNLESS(-1 != (l=PyString_Size(s))) return NULL;
  UNLESS(c=PyString_AsString(s)) return NULL;
  UNLESS(-1 != O_cwrite(self,c,l)) return NULL;

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
O_getval(ARG(Oobject*, self), ARG(PyObject*, args))
    ARGDECL(Oobject*, self)
    ARGDECL(PyObject*, args)
{
  return PyString_FromStringAndSize(self->buf,self->pos);
}

static char O_truncate__doc__[] = 
"truncate(): truncate the file at the current position.";

static PyObject *
O_truncate(ARG(Oobject*, self), ARG(PyObject*, args))
    ARGDECL(Oobject*, self)
    ARGDECL(PyObject*, args)
{
  self->string_size = self->pos;
  Py_INCREF(Py_None);
  return Py_None;
}

static char O_isatty__doc__[] = "isatty(): always returns 0";

static PyObject *
O_isatty(ARG(Oobject*, self), ARG(PyObject*, args))
    ARGDECL(Oobject*, self)
    ARGDECL(PyObject*, args)
{
  return PyInt_FromLong(0);
}

static char O_close__doc__[] = "close(): explicitly release resources held.";

static PyObject *
O_close(ARG(Oobject*, self), ARG(PyObject*, args))
    ARGDECL(Oobject*, self)
    ARGDECL(PyObject*, args)
{
  free(self->buf);

  self->pos = self->string_size = self->buf_size = 0;
  self->closed = 1;

  Py_INCREF(Py_None);
  return Py_None;
}

static char O_flush__doc__[] = "flush(): does nothing.";

static PyObject *
O_flush(ARG(Oobject*, self), ARG(PyObject*, args))
    ARGDECL(Oobject*, self)
    ARGDECL(PyObject*, args)
{
  Py_INCREF(Py_None);
  return Py_None;
}


static char O_writelines__doc__[] = "blah";
static PyObject *
O_writelines(ARG(Oobject*, self), ARG(PyObject*, args))
    ARGDECL(Oobject*, self)
    ARGDECL(PyObject*, args)
{
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
  {"read",       (PyCFunction)O_read,         1,      O_read__doc__},
  {"readline",   (PyCFunction)O_readline,     0,      O_readline__doc__},
  {"write",	 (PyCFunction)O_write,        0,      O_write__doc__},
  {"reset",      (PyCFunction)O_reset,        0,      O_reset__doc__},
  {"seek",       (PyCFunction)O_seek,         1,      O_seek__doc__},
  {"tell",       (PyCFunction)O_tell,         0,      O_tell__doc__},
  {"getvalue",   (PyCFunction)O_getval,       0,      "getvalue() -- Get the string value"},
  {"truncate",   (PyCFunction)O_truncate,     0,      O_truncate__doc__},
  {"isatty",     (PyCFunction)O_isatty,       0,      O_isatty__doc__},
  {"close",      (PyCFunction)O_close,        0,      O_close__doc__},
  {"flush",      (PyCFunction)O_flush,        0,      O_flush__doc__},
  {"writelines", (PyCFunction)O_writelines,   0,      O_writelines__doc__},
  {NULL,		NULL}		/* sentinel */
};

/* ---------- */


static Oobject *
newOobject(ARG(int, size))
    ARGDECL(int, size)
{
  Oobject *self;
	
  self = PyObject_NEW(Oobject, &Otype);
  if (self == NULL)
    return NULL;
  self->pos=0;
  self->closed = 0;
  self->string_size = 0;

  UNLESS(self->buf=malloc(size*sizeof(char)))
    {
      PyErr_SetString(PyExc_MemoryError,"out of memory");
      self->buf_size = 0;
      return NULL;
    }

  self->buf_size=size;
  return self;
}


static void
O_dealloc(ARG(Oobject*, self))
    ARGDECL(Oobject*, self)
{
  free(self->buf);
  PyMem_DEL(self);
}

static PyObject *
O_getattr(ARG(Oobject*, self), ARG(char*, name))
    ARGDECL(Oobject*, self)
    ARGDECL(char*, name)
{
  return Py_FindMethod(O_methods, (PyObject *)self, name);
}

static char Otype__doc__[] = 
"Simple type for output to strings."
;

static PyTypeObject Otype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,	       		/*ob_size*/
	"StringO",     		/*tp_name*/
	sizeof(Oobject),       	/*tp_basicsize*/
	0,	       		/*tp_itemsize*/
	/* methods */
	(destructor)O_dealloc,	/*tp_dealloc*/
	(printfunc)0,		/*tp_print*/
	(getattrfunc)O_getattr,	/*tp_getattr*/
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
	Otype__doc__ 		/* Documentation string */
};

/* End of code for StringO objects */
/* -------------------------------------------------------- */

static PyObject *
I_close(ARG(Iobject*, self), ARG(PyObject*, args))
    ARGDECL(Iobject*, self)
    ARGDECL(PyObject*, args)
{
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

/* ---------- */


static Iobject *
newIobject(ARG(PyObject*, s))
    ARGDECL(PyObject*, s)
{
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
  
  return self;
}


static void
I_dealloc(ARG(Iobject*, self))
    ARGDECL(Iobject*, self)
{
  Py_DECREF(self->pbuf);
  PyMem_DEL(self);
}

static PyObject *
I_getattr(ARG(Iobject*, self), ARG(char*, name))
    ARGDECL(Iobject*, self)
    ARGDECL(char*, name)
{
  return Py_FindMethod(I_methods, (PyObject *)self, name);
}

static char Itype__doc__[] = 
"Simple type for treating strings as input file streams"
;

static PyTypeObject Itype = {
	PyObject_HEAD_INIT(&PyType_Type)
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

/* End of code for StringI objects */
/* -------------------------------------------------------- */


static char IO_StringIO__doc__[] =
"StringIO([s]) -- Return a StringIO-like stream for reading or writing"
;

static PyObject *
IO_StringIO(ARG(PyObject*, self), ARG(PyObject*, args))
    ARGDECL(PyObject*, self)
    ARGDECL(PyObject*, args)
{
  PyObject *s=0;

  UNLESS(PyArg_ParseTuple(args, "|O", &s)) return NULL;
  if(s) return (PyObject *)newIobject(s);
  return (PyObject *)newOobject(128);
}

/* List of methods defined in the module */

static struct PyMethodDef IO_methods[] = {
  {"StringIO",	(PyCFunction)IO_StringIO,	1,	IO_StringIO__doc__},
  {NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initcStringIO) */

void
initcStringIO()
{
  PyObject *m, *d;

  /* Create the module and add the functions */
  m = Py_InitModule4("cStringIO", IO_methods,
		     cStringIO_module_documentation,
		     (PyObject*)NULL,PYTHON_API_VERSION);

  /* Add some symbolic constants to the module */
  d = PyModule_GetDict(m);
  ErrorObject = PyString_FromString("cStringIO.error");
  PyDict_SetItemString(d, "error", ErrorObject);
  
#ifdef Py_COBJECT_H
  /* Export C API */
  PyDict_SetItemString(d,"cread", PyCObject_FromVoidPtr(O_cread,NULL));
  PyDict_SetItemString(d,"creadline", PyCObject_FromVoidPtr(O_creadline,NULL));
  PyDict_SetItemString(d,"cwrite", PyCObject_FromVoidPtr(O_cwrite,NULL));
  PyDict_SetItemString(d,"cgetvalue", PyCObject_FromVoidPtr(O_getval,NULL));
  PyDict_SetItemString(d,"NewInput", PyCObject_FromVoidPtr(newIobject,NULL));
  PyDict_SetItemString(d,"NewOutput", PyCObject_FromVoidPtr(newOobject,NULL));
#endif
  
  PyDict_SetItemString(d,"InputType",  (PyObject*)&Itype);
  PyDict_SetItemString(d,"OutputType", (PyObject*)&Otype);
  

  /* Check for errors */
  if (PyErr_Occurred())
    Py_FatalError("can't initialize module cStringIO");
}

