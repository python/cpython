
#include "Python.h"
#include "import.h"
#include "cStringIO.h"
#include "structmember.h"

PyDoc_STRVAR(cStringIO_module_documentation,
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
"  value=an_output_stream.getvalue()\n"
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
"cStringIO.c,v 1.29 1999/06/15 14:10:27 jim Exp\n");

/* Declaration for file-like objects that manage data as strings

   The IOobject type should be though of as a common base type for
   Iobjects, which provide input (read-only) StringIO objects and
   Oobjects, which provide read-write objects.  Most of the methods
   depend only on common data.
*/

typedef struct {
  PyObject_HEAD
  char *buf;
  Py_ssize_t pos, string_size;
} IOobject;

#define IOOOBJECT(O) ((IOobject*)(O))

/* Declarations for objects of type StringO */

typedef struct { /* Subtype of IOobject */
  PyObject_HEAD
  char *buf;
  Py_ssize_t pos, string_size;

  Py_ssize_t buf_size;
  int softspace;
} Oobject;

/* Declarations for objects of type StringI */

typedef struct { /* Subtype of IOobject */
  PyObject_HEAD
  char *buf;
  Py_ssize_t pos, string_size;
  /* We store a reference to the object here in order to keep
     the buffer alive during the lifetime of the Iobject. */
  PyObject *pbuf;
} Iobject;

/* IOobject (common) methods */

PyDoc_STRVAR(IO_flush__doc__, "flush(): does nothing.");

static int
IO__opencheck(IOobject *self) {
    if (!self->buf) {
        PyErr_SetString(PyExc_ValueError,
                        "I/O operation on closed file");
        return 0;
    }
    return 1;
}

static PyObject *
IO_get_closed(IOobject *self, void *closure)
{
    PyObject *result = Py_False;

    if (self->buf == NULL)
        result = Py_True;
    Py_INCREF(result);
    return result;
}

static PyGetSetDef file_getsetlist[] = {
    {"closed", (getter)IO_get_closed, NULL, "True if the file is closed"},
    {0},
};

static PyObject *
IO_flush(IOobject *self, PyObject *unused) {

    if (!IO__opencheck(self)) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(IO_getval__doc__,
"getvalue([use_pos]) -- Get the string value."
"\n"
"If use_pos is specified and is a true value, then the string returned\n"
"will include only the text up to the current file position.\n");

static PyObject *
IO_cgetval(PyObject *self) {
    if (!IO__opencheck(IOOOBJECT(self))) return NULL;
    assert(IOOOBJECT(self)->pos >= 0);
    return PyString_FromStringAndSize(((IOobject*)self)->buf,
                                      ((IOobject*)self)->pos);
}

static PyObject *
IO_getval(IOobject *self, PyObject *args) {
    PyObject *use_pos=Py_None;
    int b;
    Py_ssize_t s;

    if (!IO__opencheck(self)) return NULL;
    if (!PyArg_UnpackTuple(args,"getval", 0, 1,&use_pos)) return NULL;

    b = PyObject_IsTrue(use_pos);
    if (b < 0)
        return NULL;
    if (b) {
              s=self->pos;
              if (s > self->string_size) s=self->string_size;
    }
    else
              s=self->string_size;
    assert(self->pos >= 0);
    return PyString_FromStringAndSize(self->buf, s);
}

PyDoc_STRVAR(IO_isatty__doc__, "isatty(): always returns 0");

static PyObject *
IO_isatty(IOobject *self, PyObject *unused) {
    if (!IO__opencheck(self)) return NULL;
    Py_INCREF(Py_False);
    return Py_False;
}

PyDoc_STRVAR(IO_read__doc__,
"read([s]) -- Read s characters, or the rest of the string");

static int
IO_cread(PyObject *self, char **output, Py_ssize_t  n) {
    Py_ssize_t l;

    if (!IO__opencheck(IOOOBJECT(self))) return -1;
    assert(IOOOBJECT(self)->pos >= 0);
    assert(IOOOBJECT(self)->string_size >= 0);
    l = ((IOobject*)self)->string_size - ((IOobject*)self)->pos;
    if (n < 0 || n > l) {
        n = l;
        if (n < 0) n=0;
    }
    if (n > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "length too large");
        return -1;
    }

    *output=((IOobject*)self)->buf + ((IOobject*)self)->pos;
    ((IOobject*)self)->pos += n;
    return (int)n;
}

static PyObject *
IO_read(IOobject *self, PyObject *args) {
    Py_ssize_t n = -1;
    char *output = NULL;

    if (!PyArg_ParseTuple(args, "|n:read", &n)) return NULL;

    if ( (n=IO_cread((PyObject*)self,&output,n)) < 0) return NULL;

    return PyString_FromStringAndSize(output, n);
}

PyDoc_STRVAR(IO_readline__doc__, "readline() -- Read one line");

static int
IO_creadline(PyObject *self, char **output) {
    char *n, *start, *end;
    Py_ssize_t len;

    if (!IO__opencheck(IOOOBJECT(self))) return -1;

    n = start = ((IOobject*)self)->buf + ((IOobject*)self)->pos;
    end = ((IOobject*)self)->buf + ((IOobject*)self)->string_size;
    while (n < end && *n != '\n')
        n++;

    if (n < end) n++;

    len = n - start;
    if (len > INT_MAX)
        len = INT_MAX;

    *output=start;

    assert(IOOOBJECT(self)->pos <= PY_SSIZE_T_MAX - len);
    assert(IOOOBJECT(self)->pos >= 0);
    assert(IOOOBJECT(self)->string_size >= 0);

    ((IOobject*)self)->pos += len;
    return (int)len;
}

static PyObject *
IO_readline(IOobject *self, PyObject *args) {
    int n, m=-1;
    char *output;

    if (args)
        if (!PyArg_ParseTuple(args, "|i:readline", &m)) return NULL;

    if( (n=IO_creadline((PyObject*)self,&output)) < 0) return NULL;
    if (m >= 0 && m < n) {
        m = n - m;
        n -= m;
        self->pos -= m;
    }
    assert(IOOOBJECT(self)->pos >= 0);
    return PyString_FromStringAndSize(output, n);
}

PyDoc_STRVAR(IO_readlines__doc__, "readlines() -- Read all lines");

static PyObject *
IO_readlines(IOobject *self, PyObject *args) {
    int n;
    char *output;
    PyObject *result, *line;
    Py_ssize_t hint = 0, length = 0;

    if (!PyArg_ParseTuple(args, "|n:readlines", &hint)) return NULL;

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
        if (PyList_Append (result, line) == -1) {
            Py_DECREF (line);
            goto err;
        }
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

PyDoc_STRVAR(IO_reset__doc__,
"reset() -- Reset the file position to the beginning");

static PyObject *
IO_reset(IOobject *self, PyObject *unused) {

    if (!IO__opencheck(self)) return NULL;

    self->pos = 0;

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(IO_tell__doc__, "tell() -- get the current position.");

static PyObject *
IO_tell(IOobject *self, PyObject *unused) {

    if (!IO__opencheck(self)) return NULL;

    assert(self->pos >= 0);
    return PyInt_FromSsize_t(self->pos);
}

PyDoc_STRVAR(IO_truncate__doc__,
"truncate(): truncate the file at the current position.");

static PyObject *
IO_truncate(IOobject *self, PyObject *args) {
    Py_ssize_t pos = -1;

    if (!IO__opencheck(self)) return NULL;
    if (!PyArg_ParseTuple(args, "|n:truncate", &pos)) return NULL;

    if (PyTuple_Size(args) == 0) {
        /* No argument passed, truncate to current position */
        pos = self->pos;
    }

    if (pos < 0) {
        errno = EINVAL;
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }

    if (self->string_size > pos) self->string_size = pos;
    self->pos = self->string_size;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
IO_iternext(Iobject *self)
{
    PyObject *next;
    next = IO_readline((IOobject *)self, NULL);
    if (!next)
        return NULL;
    if (!PyString_GET_SIZE(next)) {
        Py_DECREF(next);
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
    return next;
}




/* Read-write object methods */

PyDoc_STRVAR(IO_seek__doc__,
"seek(position)       -- set the current position\n"
"seek(position, mode) -- mode 0: absolute; 1: relative; 2: relative to EOF");

static PyObject *
IO_seek(Iobject *self, PyObject *args) {
    Py_ssize_t position;
    int mode = 0;

    if (!IO__opencheck(IOOOBJECT(self))) return NULL;
    if (!PyArg_ParseTuple(args, "n|i:seek", &position, &mode))
        return NULL;

    if (mode == 2) {
        position += self->string_size;
    }
    else if (mode == 1) {
        position += self->pos;
    }

    if (position < 0) position=0;

    self->pos=position;

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(O_write__doc__,
"write(s) -- Write a string to the file"
"\n\nNote (hack:) writing None resets the buffer");


static int
O_cwrite(PyObject *self, const char *c, Py_ssize_t  len) {
    Py_ssize_t newpos;
    Oobject *oself;
    char *newbuf;

    if (!IO__opencheck(IOOOBJECT(self))) return -1;
    oself = (Oobject *)self;

    if (len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "length too large");
        return -1;
    }
    assert(len >= 0);
    if (oself->pos >= PY_SSIZE_T_MAX - len) {
        PyErr_SetString(PyExc_OverflowError,
                        "new position too large");
        return -1;
    }
    newpos = oself->pos + len;
    if (newpos >= oself->buf_size) {
        size_t newsize = oself->buf_size;
        newsize *= 2;
        if (newsize <= (size_t)newpos || newsize > PY_SSIZE_T_MAX) {
            assert(newpos < PY_SSIZE_T_MAX - 1);
            newsize = newpos + 1;
        }
        newbuf = (char*)realloc(oself->buf, newsize);
        if (!newbuf) {
            PyErr_SetString(PyExc_MemoryError,"out of memory");
            return -1;
        }
        oself->buf_size = (Py_ssize_t)newsize;
        oself->buf = newbuf;
    }

    if (oself->string_size < oself->pos) {
        /* In case of overseek, pad with null bytes the buffer region between
           the end of stream and the current position.

          0   lo      string_size                           hi
          |   |<---used--->|<----------available----------->|
          |   |            <--to pad-->|<---to write--->    |
          0   buf                   position
        */
        memset(oself->buf + oself->string_size, '\0',
               (oself->pos - oself->string_size) * sizeof(char));
    }

    memcpy(oself->buf + oself->pos, c, len);

    oself->pos = newpos;

    if (oself->string_size < oself->pos) {
        oself->string_size = oself->pos;
    }

    return (int)len;
}

static PyObject *
O_write(Oobject *self, PyObject *args) {
    char *c;
    int l;

    if (!PyArg_ParseTuple(args, "t#:write", &c, &l)) return NULL;

    if (O_cwrite((PyObject*)self,c,l) < 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(O_close__doc__, "close(): explicitly release resources held.");

static PyObject *
O_close(Oobject *self, PyObject *unused) {
    if (self->buf != NULL) free(self->buf);
    self->buf = NULL;

    self->pos = self->string_size = self->buf_size = 0;

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(O_writelines__doc__,
"writelines(sequence_of_strings) -> None.  Write the strings to the file.\n"
"\n"
"Note that newlines are not added.  The sequence can be any iterable object\n"
"producing strings. This is equivalent to calling write() for each string.");
static PyObject *
O_writelines(Oobject *self, PyObject *args) {
    PyObject *it, *s;

    it = PyObject_GetIter(args);
    if (it == NULL)
        return NULL;
    while ((s = PyIter_Next(it)) != NULL) {
        Py_ssize_t n;
        char *c;
        if (PyString_AsStringAndSize(s, &c, &n) == -1) {
            Py_DECREF(it);
            Py_DECREF(s);
            return NULL;
        }
        if (O_cwrite((PyObject *)self, c, n) == -1) {
            Py_DECREF(it);
            Py_DECREF(s);
            return NULL;
           }
           Py_DECREF(s);
       }

       Py_DECREF(it);

       /* See if PyIter_Next failed */
       if (PyErr_Occurred())
           return NULL;

       Py_RETURN_NONE;
}
static struct PyMethodDef O_methods[] = {
  /* Common methods: */
  {"flush",     (PyCFunction)IO_flush,    METH_NOARGS,  IO_flush__doc__},
  {"getvalue",  (PyCFunction)IO_getval,   METH_VARARGS, IO_getval__doc__},
  {"isatty",    (PyCFunction)IO_isatty,   METH_NOARGS,  IO_isatty__doc__},
  {"read",      (PyCFunction)IO_read,     METH_VARARGS, IO_read__doc__},
  {"readline",  (PyCFunction)IO_readline, METH_VARARGS, IO_readline__doc__},
  {"readlines", (PyCFunction)IO_readlines,METH_VARARGS, IO_readlines__doc__},
  {"reset",     (PyCFunction)IO_reset,    METH_NOARGS,  IO_reset__doc__},
  {"seek",      (PyCFunction)IO_seek,     METH_VARARGS, IO_seek__doc__},
  {"tell",      (PyCFunction)IO_tell,     METH_NOARGS,  IO_tell__doc__},
  {"truncate",  (PyCFunction)IO_truncate, METH_VARARGS, IO_truncate__doc__},

  /* Read-write StringIO specific  methods: */
  {"close",      (PyCFunction)O_close,      METH_NOARGS,  O_close__doc__},
  {"write",      (PyCFunction)O_write,      METH_VARARGS, O_write__doc__},
  {"writelines", (PyCFunction)O_writelines, METH_O,       O_writelines__doc__},
  {NULL,         NULL}          /* sentinel */
};

static PyMemberDef O_memberlist[] = {
    {"softspace",       T_INT,  offsetof(Oobject, softspace),   0,
     "flag indicating that a space needs to be printed; used by print"},
     /* getattr(f, "closed") is implemented without this table */
    {NULL} /* Sentinel */
};

static void
O_dealloc(Oobject *self) {
    if (self->buf != NULL)
        free(self->buf);
    PyObject_Del(self);
}

PyDoc_STRVAR(Otype__doc__, "Simple type for output to strings.");

static PyTypeObject Otype = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cStringIO.StringO",          /*tp_name*/
  sizeof(Oobject),              /*tp_basicsize*/
  0,                            /*tp_itemsize*/
  /* methods */
  (destructor)O_dealloc,        /*tp_dealloc*/
  0,                            /*tp_print*/
  0,                            /*tp_getattr */
  0,                            /*tp_setattr */
  0,                            /*tp_compare*/
  0,                            /*tp_repr*/
  0,                            /*tp_as_number*/
  0,                            /*tp_as_sequence*/
  0,                            /*tp_as_mapping*/
  0,                            /*tp_hash*/
  0     ,                       /*tp_call*/
  0,                            /*tp_str*/
  0,                            /*tp_getattro */
  0,                            /*tp_setattro */
  0,                            /*tp_as_buffer */
  Py_TPFLAGS_DEFAULT,           /*tp_flags*/
  Otype__doc__,                 /*tp_doc */
  0,                            /*tp_traverse */
  0,                            /*tp_clear */
  0,                            /*tp_richcompare */
  0,                            /*tp_weaklistoffset */
  PyObject_SelfIter,            /*tp_iter */
  (iternextfunc)IO_iternext,    /*tp_iternext */
  O_methods,                    /*tp_methods */
  O_memberlist,                 /*tp_members */
  file_getsetlist,              /*tp_getset */
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

    self->buf = (char *)malloc(size);
    if (!self->buf) {
              PyErr_SetString(PyExc_MemoryError,"out of memory");
              self->buf_size = 0;
              Py_DECREF(self);
              return NULL;
      }

    self->buf_size=size;
    return (PyObject*)self;
}

/* End of code for StringO objects */
/* -------------------------------------------------------- */

static PyObject *
I_close(Iobject *self, PyObject *unused) {
    Py_CLEAR(self->pbuf);
    self->buf = NULL;

    self->pos = self->string_size = 0;

    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef I_methods[] = {
  /* Common methods: */
  {"flush",     (PyCFunction)IO_flush,    METH_NOARGS,  IO_flush__doc__},
  {"getvalue",  (PyCFunction)IO_getval,   METH_VARARGS, IO_getval__doc__},
  {"isatty",    (PyCFunction)IO_isatty,   METH_NOARGS,  IO_isatty__doc__},
  {"read",      (PyCFunction)IO_read,     METH_VARARGS, IO_read__doc__},
  {"readline",  (PyCFunction)IO_readline, METH_VARARGS, IO_readline__doc__},
  {"readlines", (PyCFunction)IO_readlines,METH_VARARGS, IO_readlines__doc__},
  {"reset",     (PyCFunction)IO_reset,    METH_NOARGS,  IO_reset__doc__},
  {"seek",      (PyCFunction)IO_seek,     METH_VARARGS, IO_seek__doc__},
  {"tell",      (PyCFunction)IO_tell,     METH_NOARGS,  IO_tell__doc__},
  {"truncate",  (PyCFunction)IO_truncate, METH_VARARGS, IO_truncate__doc__},

  /* Read-only StringIO specific  methods: */
  {"close",     (PyCFunction)I_close,    METH_NOARGS,  O_close__doc__},
  {NULL,        NULL}
};

static void
I_dealloc(Iobject *self) {
  Py_XDECREF(self->pbuf);
  PyObject_Del(self);
}


PyDoc_STRVAR(Itype__doc__,
"Simple type for treating strings as input file streams");

static PyTypeObject Itype = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cStringIO.StringI",                  /*tp_name*/
  sizeof(Iobject),                      /*tp_basicsize*/
  0,                                    /*tp_itemsize*/
  /* methods */
  (destructor)I_dealloc,                /*tp_dealloc*/
  0,                                    /*tp_print*/
  0,                                    /* tp_getattr */
  0,                                    /*tp_setattr*/
  0,                                    /*tp_compare*/
  0,                                    /*tp_repr*/
  0,                                    /*tp_as_number*/
  0,                                    /*tp_as_sequence*/
  0,                                    /*tp_as_mapping*/
  0,                                    /*tp_hash*/
  0,                                    /*tp_call*/
  0,                                    /*tp_str*/
  0,                                    /* tp_getattro */
  0,                                    /* tp_setattro */
  0,                                    /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                   /* tp_flags */
  Itype__doc__,                         /* tp_doc */
  0,                                    /* tp_traverse */
  0,                                    /* tp_clear */
  0,                                    /* tp_richcompare */
  0,                                    /* tp_weaklistoffset */
  PyObject_SelfIter,                    /* tp_iter */
  (iternextfunc)IO_iternext,            /* tp_iternext */
  I_methods,                            /* tp_methods */
  0,                                    /* tp_members */
  file_getsetlist,                      /* tp_getset */
};

static PyObject *
newIobject(PyObject *s) {
  Iobject *self;
  char *buf;
  Py_ssize_t size;

  if (PyUnicode_Check(s)) {
    if (PyObject_AsCharBuffer(s, (const char **)&buf, &size) != 0)
      return NULL;
  }
  else if (PyObject_AsReadBuffer(s, (const void **)&buf, &size)) {
    PyErr_Format(PyExc_TypeError, "expected read buffer, %.200s found",
                 s->ob_type->tp_name);
    return NULL;
  }

  self = PyObject_New(Iobject, &Itype);
  if (!self) return NULL;
  Py_INCREF(s);
  self->buf=buf;
  self->string_size=size;
  self->pbuf=s;
  self->pos=0;

  return (PyObject*)self;
}

/* End of code for StringI objects */
/* -------------------------------------------------------- */


PyDoc_STRVAR(IO_StringIO__doc__,
"StringIO([s]) -- Return a StringIO-like stream for reading or writing");

static PyObject *
IO_StringIO(PyObject *self, PyObject *args) {
  PyObject *s=0;

  if (!PyArg_UnpackTuple(args, "StringIO", 0, 1, &s)) return NULL;

  if (s) return newIobject(s);
  return newOobject(128);
}

/* List of methods defined in the module */

static struct PyMethodDef IO_methods[] = {
  {"StringIO",  (PyCFunction)IO_StringIO,
   METH_VARARGS,        IO_StringIO__doc__},
  {NULL,                NULL}           /* sentinel */
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

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initcStringIO(void) {
  PyObject *m, *d, *v;


  /* Create the module and add the functions */
  m = Py_InitModule4("cStringIO", IO_methods,
                     cStringIO_module_documentation,
                     (PyObject*)NULL,PYTHON_API_VERSION);
  if (m == NULL) return;

  /* Add some symbolic constants to the module */
  d = PyModule_GetDict(m);

  /* Export C API */
  Py_TYPE(&Itype)=&PyType_Type;
  Py_TYPE(&Otype)=&PyType_Type;
  if (PyType_Ready(&Otype) < 0) return;
  if (PyType_Ready(&Itype) < 0) return;
  v = PyCapsule_New(&CAPI, PycStringIO_CAPSULE_NAME, NULL);
  PyDict_SetItemString(d,"cStringIO_CAPI", v);
  Py_XDECREF(v);

  /* Export Types */
  PyDict_SetItemString(d,"InputType",  (PyObject*)&Itype);
  PyDict_SetItemString(d,"OutputType", (PyObject*)&Otype);

  /* Maybe make certain warnings go away */
  if (0) PycString_IMPORT;
}
