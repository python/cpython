/*
 * cStringIO.c,v 1.29 1999/06/15 14:10:27 jim Exp
 * 
 * Copyright (c) 1996-1998, Digital Creations, Fredericksburg, VA, USA.  
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   o Redistributions of source code must retain the above copyright
 *     notice, this list of conditions, and the disclaimer that follows.
 * 
 *   o Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions, and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 
 *   o All advertising materials mentioning features or use of this
 *     software must display the following acknowledgement:
 * 
 *       This product includes software developed by Digital Creations
 *       and its contributors.
 * 
 *   o Neither the name of Digital Creations nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 * 
 * THIS SOFTWARE IS PROVIDED BY DIGITAL CREATIONS AND CONTRIBUTORS *AS
 * IS* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL DIGITAL
 * CREATIONS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 # 
 # If you have questions regarding this software, contact:
 #
 #   Digital Creations, L.C.
 #   910 Princess Ann Street
 #   Fredericksburge, Virginia  22401
 #
 #   info@digicool.com
 #
 #   (540) 371-6909
 */
static char cStringIO_module_documentation[] = 
"A simple fast partial StringIO replacement.\n"
"\n"
"This module provides a simple useful replacement for\n"
"the StringIO module that is written in C.  It does not provide the\n"
"full generality of StringIO, but it provides enough for most\n"
"applications and is especially useful in conjunction with the\n"
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
"  an_input_stream.seek(0)           # OK, start over\n"
"  spam=an_input_stream.read()       # and read it all\n"
"  \n"
"If someone else wants to provide a more complete implementation,\n"
"go for it. :-)  \n"
"\n"
"cStringIO.c,v 1.29 1999/06/15 14:10:27 jim Exp\n"
;

#include "Python.h"
#include "import.h"
#include "cStringIO.h"

#define UNLESS(E) if (!(E))


/* Declaration for file-like objects that manage data as strings 

   The IOobject type should be though of as a common base type for
   Iobjects, which provide input (read-only) StringIO objects and
   Oobjects, which provide read-write objects.  Most of the methods
   depend only on common data.
*/

typedef struct {
  PyObject_HEAD
  char *buf;
  int pos, string_size;
} IOobject;

#define IOOOBJECT(O) ((IOobject*)(O))

/* Declarations for objects of type StringO */

typedef struct { /* Subtype of IOobject */
  PyObject_HEAD
  char *buf;
  int pos, string_size;

  int buf_size, softspace;
} Oobject;

/* Declarations for objects of type StringI */

typedef struct { /* Subtype of IOobject */
  PyObject_HEAD
  char *buf;
  int pos, string_size;

  PyObject *pbuf;
} Iobject;

/* IOobject (common) methods */

static char IO_flush__doc__[] = "flush(): does nothing.";

static int
IO__opencheck(IOobject *self) {
        UNLESS (self->buf) {
                PyErr_SetString(PyExc_ValueError,
                                "I/O operation on closed file");
                return 0;
        }
        return 1;
}

static PyObject *
IO_flush(IOobject *self, PyObject *args) {

        UNLESS (IO__opencheck(self)) return NULL;
        UNLESS (PyArg_ParseTuple(args, ":flush")) return NULL;

        Py_INCREF(Py_None);
        return Py_None;
}

static char IO_getval__doc__[] = 
   "getvalue([use_pos]) -- Get the string value."
   "\n"
   "If use_pos is specified and is a true value, then the string returned\n"
   "will include only the text up to the current file position.\n"
;

static PyObject *
IO_cgetval(PyObject *self) {
        UNLESS (IO__opencheck(IOOOBJECT(self))) return NULL;
        return PyString_FromStringAndSize(((IOobject*)self)->buf,
                                          ((IOobject*)self)->pos);
}

static PyObject *
IO_getval(IOobject *self, PyObject *args) {
        PyObject *use_pos=Py_None;
        int s;

        UNLESS (IO__opencheck(self)) return NULL;
        UNLESS (PyArg_ParseTuple(args,"|O:getval",&use_pos)) return NULL;

        if (PyObject_IsTrue(use_pos)) {
                  s=self->pos;
                  if (s > self->string_size) s=self->string_size;
        }
        else
                  s=self->string_size;
        return PyString_FromStringAndSize(self->buf, s);
}

static char IO_isatty__doc__[] = "isatty(): always returns 0";

static PyObject *
IO_isatty(IOobject *self, PyObject *args) {

        UNLESS (PyArg_ParseTuple(args, ":isatty")) return NULL;

        return PyInt_FromLong(0);
}

static char IO_read__doc__[] = 
"read([s]) -- Read s characters, or the rest of the string"
;

static int
IO_cread(PyObject *self, char **output, int  n) {
        int l;

        UNLESS (IO__opencheck(IOOOBJECT(self))) return -1;
        l = ((IOobject*)self)->string_size - ((IOobject*)self)->pos;  
        if (n < 0 || n > l) {
                n = l;
                if (n < 0) n=0;
        }

        *output=((IOobject*)self)->buf + ((IOobject*)self)->pos;
        ((IOobject*)self)->pos += n;
        return n;
}

static PyObject *
IO_read(IOobject *self, PyObject *args) {
        int n = -1;
        char *output;

        UNLESS (PyArg_ParseTuple(args, "|i:read", &n)) return NULL;

        if ( (n=IO_cread((PyObject*)self,&output,n)) < 0) return NULL;

        return PyString_FromStringAndSize(output, n);
}

static char IO_readline__doc__[] = 
"readline() -- Read one line"
;

static int
IO_creadline(PyObject *self, char **output) {
        char *n, *s;
        int l;

        UNLESS (IO__opencheck(IOOOBJECT(self))) return -1;

        for (n = ((IOobject*)self)->buf + ((IOobject*)self)->pos,
               s = ((IOobject*)self)->buf + ((IOobject*)self)->string_size; 
             n < s && *n != '\n'; n++);
        if (n < s) n++;

        *output=((IOobject*)self)->buf + ((IOobject*)self)->pos;
        l = n - ((IOobject*)self)->buf - ((IOobject*)self)->pos;
        ((IOobject*)self)->pos += l;
        return l;
}

static PyObject *
IO_readline(IOobject *self, PyObject *args) {
        int n, m=-1;
        char *output;

        UNLESS (PyArg_ParseTuple(args, "|i:readline", &m)) return NULL;

        if( (n=IO_creadline((PyObject*)self,&output)) < 0) return NULL;
        if (m >= 0 && m < n) {
                m = n - m;
                n -= m;
                self->pos -= m;
        }
        return PyString_FromStringAndSize(output, n);
}

static char IO_readlines__doc__[] = 
"readlines() -- Read all lines"
;

static PyObject *
IO_readlines(IOobject *self, PyObject *args) {
	int n;
	char *output;
	PyObject *result, *line;
        int hint = 0, length = 0;
	
        UNLESS (PyArg_ParseTuple(args, "|i:readlines", &hint)) return NULL;

	result = PyList_New(0);
	if (!result)
		return NULL;

	while (1){
		if ( (n = IO_creadline((PyObject*)self,&output)) < 0)
                        goto err;
		if (n == 0)
			break;
		line = PyString_FromStringAndSize (output, n);
		if (!line) 
                        goto err;
		PyList_Append (result, line);
		Py_DECREF (line);
                length += n;
                if (hint > 0 && length >= hint)
			break;
	}
	return result;
 err:
        Py_DECREF(result);
        return NULL;
}

static char IO_reset__doc__[] = 
"reset() -- Reset the file position to the beginning"
;

static PyObject *
IO_reset(IOobject *self, PyObject *args) {

        UNLESS (IO__opencheck(self)) return NULL;
        UNLESS (PyArg_ParseTuple(args, ":reset")) return NULL;

        self->pos = 0;

        Py_INCREF(Py_None);
        return Py_None;
}

static char IO_tell__doc__[] =
"tell() -- get the current position.";

static PyObject *
IO_tell(IOobject *self, PyObject *args) {

        UNLESS (IO__opencheck(self)) return NULL;
        UNLESS (PyArg_ParseTuple(args, ":tell")) return NULL;

        return PyInt_FromLong(self->pos);
}

static char IO_truncate__doc__[] = 
"truncate(): truncate the file at the current position.";

static PyObject *
IO_truncate(IOobject *self, PyObject *args) {
        int pos = -1;
	
        UNLESS (IO__opencheck(self)) return NULL;
        UNLESS (PyArg_ParseTuple(args, "|i:truncate", &pos)) return NULL;
        if (pos < 0) pos = self->pos;

        if (self->string_size > pos) self->string_size = pos;

        Py_INCREF(Py_None);
        return Py_None;
}




/* Read-write object methods */

static char O_seek__doc__[] =
"seek(position)       -- set the current position\n"
"seek(position, mode) -- mode 0: absolute; 1: relative; 2: relative to EOF";

static PyObject *
O_seek(Oobject *self, PyObject *args) {
        int position, mode = 0;

        UNLESS (IO__opencheck(IOOOBJECT(self))) return NULL;
        UNLESS (PyArg_ParseTuple(args, "i|i:seek", &position, &mode)) 
                return NULL;

        if (mode == 2) {
                position += self->string_size;
        }
        else if (mode == 1) {
                position += self->pos;
        }

        if (position > self->buf_size) {
                  self->buf_size*=2;
                  if (self->buf_size <= position) self->buf_size=position+1;
                  UNLESS (self->buf=(char*)
                          realloc(self->buf,self->buf_size*sizeof(char))) {
                      self->buf_size=self->pos=0;
                      return PyErr_NoMemory();
                    }
          }
        else if (position < 0) position=0;

        self->pos=position;

        while (--position >= self->string_size) self->buf[position]=0;

        Py_INCREF(Py_None);
        return Py_None;
}

static char O_write__doc__[] = 
"write(s) -- Write a string to the file"
"\n\nNote (hack:) writing None resets the buffer"
;


static int
O_cwrite(PyObject *self, char *c, int  l) {
        int newl;

        UNLESS (IO__opencheck(IOOOBJECT(self))) return -1;

        newl=((Oobject*)self)->pos+l;
        if (newl >= ((Oobject*)self)->buf_size) {
            ((Oobject*)self)->buf_size*=2;
            if (((Oobject*)self)->buf_size <= newl) 
                    ((Oobject*)self)->buf_size=newl+1;
            UNLESS (((Oobject*)self)->buf=
                   (char*)realloc(
                        ((Oobject*)self)->buf,
                        (((Oobject*)self)->buf_size) *sizeof(char))) {
                    PyErr_SetString(PyExc_MemoryError,"out of memory");
                    ((Oobject*)self)->buf_size=((Oobject*)self)->pos=0;
                    return -1;
              }
          }

        memcpy(((Oobject*)((Oobject*)self))->buf+((Oobject*)self)->pos,c,l);

        ((Oobject*)self)->pos += l;

        if (((Oobject*)self)->string_size < ((Oobject*)self)->pos) {
            ((Oobject*)self)->string_size = ((Oobject*)self)->pos;
          }

        return l;
}

static PyObject *
O_write(Oobject *self, PyObject *args) {
        PyObject *s;
        char *c;
        int l;

        UNLESS (PyArg_ParseTuple(args, "O:write", &s)) return NULL;

        UNLESS (-1 != (l=PyString_Size(s))) return NULL;
        UNLESS (c=PyString_AsString(s)) return NULL;
        if (O_cwrite((PyObject*)self,c,l) < 0) return NULL;

        Py_INCREF(Py_None);
        return Py_None;
}

static char O_close__doc__[] = "close(): explicitly release resources held.";

static PyObject *
O_close(Oobject *self, PyObject *args) {

        UNLESS (PyArg_ParseTuple(args, ":close")) return NULL;

        if (self->buf != NULL) free(self->buf);
        self->buf = NULL;

        self->pos = self->string_size = self->buf_size = 0;

        Py_INCREF(Py_None);
        return Py_None;
}


static char O_writelines__doc__[] =
"writelines(sequence_of_strings): write each string";
static PyObject *
O_writelines(Oobject *self, PyObject *args) {
        PyObject *tmp = 0;
        static PyObject *string_joinfields = 0;

        UNLESS (PyArg_ParseTuple(args, "O:writelines", &args)) return NULL;

        if (!string_joinfields) {
                UNLESS (tmp = PyImport_ImportModule("string")) return NULL;
                string_joinfields=PyObject_GetAttrString(tmp, "joinfields");
                Py_DECREF(tmp);
                UNLESS (string_joinfields) return NULL;
        }

        if (PyObject_Size(args) < 0) return NULL;

        tmp = PyObject_CallFunction(string_joinfields, "Os", args, "");
        UNLESS (tmp) return NULL;

        args = Py_BuildValue("(O)", tmp);
        Py_DECREF(tmp);
        UNLESS (args) return NULL;

        tmp = O_write(self, args);
        Py_DECREF(args);
        return tmp;
}

static struct PyMethodDef O_methods[] = {
  /* Common methods: */
  {"flush",     (PyCFunction)IO_flush,    METH_VARARGS, IO_flush__doc__},
  {"getvalue",  (PyCFunction)IO_getval,   METH_VARARGS, IO_getval__doc__},
  {"isatty",    (PyCFunction)IO_isatty,   METH_VARARGS, IO_isatty__doc__},
  {"read",	(PyCFunction)IO_read,     METH_VARARGS, IO_read__doc__},
  {"readline",	(PyCFunction)IO_readline, METH_VARARGS, IO_readline__doc__},
  {"readlines",	(PyCFunction)IO_readlines,METH_VARARGS, IO_readlines__doc__},
  {"reset",	(PyCFunction)IO_reset,	  METH_VARARGS, IO_reset__doc__},
  {"tell",      (PyCFunction)IO_tell,     METH_VARARGS, IO_tell__doc__},
  {"truncate",  (PyCFunction)IO_truncate, METH_VARARGS, IO_truncate__doc__},

  /* Read-write StringIO specific  methods: */
  {"close",      (PyCFunction)O_close,      METH_VARARGS, O_close__doc__},
  {"seek",       (PyCFunction)O_seek,       METH_VARARGS, O_seek__doc__},
  {"write",	 (PyCFunction)O_write,      METH_VARARGS, O_write__doc__},
  {"writelines", (PyCFunction)O_writelines, METH_VARARGS, O_writelines__doc__},
  {NULL,	 NULL}		/* sentinel */
};

static void
O_dealloc(Oobject *self) {
        if (self->buf != NULL)
                free(self->buf);
        PyObject_Del(self);
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
	if (x < 0 && PyErr_Occurred())
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

        self = PyObject_New(Oobject, &Otype);
        if (self == NULL)
                return NULL;
        self->pos=0;
        self->string_size = 0;
        self->softspace = 0;

        UNLESS (self->buf=malloc(size*sizeof(char))) {
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

        UNLESS (PyArg_ParseTuple(args, ":close")) return NULL;

        Py_XDECREF(self->pbuf);
        self->pbuf = NULL;
        self->buf = NULL;

        self->pos = self->string_size = 0;

        Py_INCREF(Py_None);
        return Py_None;
}

static PyObject *
I_seek(Iobject *self, PyObject *args) {
        int position, mode = 0;

        UNLESS (IO__opencheck(IOOOBJECT(self))) return NULL;
        UNLESS (PyArg_ParseTuple(args, "i|i:seek", &position, &mode)) 
                return NULL;

        if (mode == 2) position += self->string_size;
        else if (mode == 1) position += self->pos;

        if (position < 0) position=0;

        self->pos=position;

        Py_INCREF(Py_None);
        return Py_None;
}

static struct PyMethodDef I_methods[] = {
  /* Common methods: */
  {"flush",     (PyCFunction)IO_flush,    METH_VARARGS, IO_flush__doc__},
  {"getvalue",  (PyCFunction)IO_getval,   METH_VARARGS, IO_getval__doc__},
  {"isatty",    (PyCFunction)IO_isatty,   METH_VARARGS, IO_isatty__doc__},
  {"read",	(PyCFunction)IO_read,     METH_VARARGS, IO_read__doc__},
  {"readline",	(PyCFunction)IO_readline, METH_VARARGS, IO_readline__doc__},
  {"readlines",	(PyCFunction)IO_readlines,METH_VARARGS, IO_readlines__doc__},
  {"reset",	(PyCFunction)IO_reset,	  METH_VARARGS, IO_reset__doc__},
  {"tell",      (PyCFunction)IO_tell,     METH_VARARGS, IO_tell__doc__},
  {"truncate",  (PyCFunction)IO_truncate, METH_VARARGS, IO_truncate__doc__},

  /* Read-only StringIO specific  methods: */
  {"close",     (PyCFunction)I_close,    METH_VARARGS, O_close__doc__},
  {"seek",      (PyCFunction)I_seek,     METH_VARARGS, O_seek__doc__},  
  {NULL,	NULL}
};

static void
I_dealloc(Iobject *self) {
  Py_XDECREF(self->pbuf);
  PyObject_Del(self);
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

  if (!PyString_Check(s)) {
      PyErr_Format(PyExc_TypeError, "expected string, %.200s found",
		   s->ob_type->tp_name);
      return NULL;
  }
  buf = PyString_AS_STRING(s);
  size = PyString_GET_SIZE(s);
  UNLESS (self = PyObject_New(Iobject, &Itype)) return NULL;
  Py_INCREF(s);
  self->buf=buf;
  self->string_size=size;
  self->pbuf=s;
  self->pos=0;
  
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

  if (!PyArg_ParseTuple(args, "|O:StringIO", &s)) return NULL;

  if (s) return newIobject(s);
  return newOobject(128);
}

/* List of methods defined in the module */

static struct PyMethodDef IO_methods[] = {
  {"StringIO",	(PyCFunction)IO_StringIO,	
   METH_VARARGS,	IO_StringIO__doc__},
  {NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initcStringIO) */

static struct PycStringIO_CAPI CAPI = {
  IO_cread,
  IO_creadline,
  O_cwrite,
  IO_cgetval,
  newOobject,
  newIobject,
  &Itype,
  &Otype,
};

#ifndef DL_EXPORT	/* declarations for DLL import/export */
#define DL_EXPORT(RTYPE) RTYPE
#endif
DL_EXPORT(void)
initcStringIO(void) {
  PyObject *m, *d, *v;


  /* Create the module and add the functions */
  m = Py_InitModule4("cStringIO", IO_methods,
		     cStringIO_module_documentation,
		     (PyObject*)NULL,PYTHON_API_VERSION);

  /* Add some symbolic constants to the module */
  d = PyModule_GetDict(m);
  
  /* Export C API */
  Itype.ob_type=&PyType_Type;
  Otype.ob_type=&PyType_Type;
  PyDict_SetItemString(d,"cStringIO_CAPI",
		       v = PyCObject_FromVoidPtr(&CAPI,NULL));
  Py_XDECREF(v);

  /* Export Types */
  PyDict_SetItemString(d,"InputType",  (PyObject*)&Itype);
  PyDict_SetItemString(d,"OutputType", (PyObject*)&Otype);

  /* Maybe make certain warnings go away */
  if (0) PycString_IMPORT;
}
