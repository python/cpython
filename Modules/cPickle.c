/*
 * cPickle.c,v 1.71 1999/07/11 13:30:34 jim Exp
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

static char cPickle_module_documentation[] =
"C implementation and optimization of the Python pickle module\n"
"\n"
"cPickle.c,v 1.71 1999/07/11 13:30:34 jim Exp\n"
;

#include "Python.h"
#include "cStringIO.h"
#include "structmember.h"

#ifndef Py_eval_input
#include <graminit.h>
#define Py_eval_input eval_input
#endif /* Py_eval_input */

#include <errno.h>



#define DEL_LIST_SLICE(list, from, to) (PyList_SetSlice(list, from, to, NULL))

#define WRITE_BUF_SIZE 256

/* --------------------------------------------------------------------------
NOTES on format codes.
XXX much more is needed here

Integer types
BININT1         8-bit unsigned integer; followed by 1 byte.
BININT2         16-bit unsigned integer; followed by 2 bytes, little-endian.
BININT          32-bit signed integer; followed by 4 bytes, little-endian.
INT             Integer; natural decimal string conversion, then newline.
                CAUTION:  INT-reading code can't assume that what follows
                fits in a Python int, because the size of Python ints varies
                across platforms.
LONG            Long (unbounded) integer; repr(i), then newline.
-------------------------------------------------------------------------- */

#define MARK        '('
#define STOP        '.'
#define POP         '0'
#define POP_MARK    '1'
#define DUP         '2'
#define FLOAT       'F'
#define BINFLOAT    'G'
#define INT         'I'
#define BININT      'J'
#define BININT1     'K'
#define LONG        'L'
#define BININT2     'M'
#define NONE        'N'
#define PERSID      'P'
#define BINPERSID   'Q'
#define REDUCE      'R'
#define STRING      'S'
#define BINSTRING   'T'
#define SHORT_BINSTRING 'U'
#define UNICODE     'V'
#define BINUNICODE  'X'
#define APPEND      'a'
#define BUILD       'b'
#define GLOBAL      'c'
#define DICT        'd'
#define EMPTY_DICT  '}'
#define APPENDS     'e'
#define GET         'g'
#define BINGET      'h'
#define INST        'i'
#define LONG_BINGET 'j'
#define LIST        'l'
#define EMPTY_LIST  ']'
#define OBJ         'o'
#define PUT         'p'
#define BINPUT      'q'
#define LONG_BINPUT 'r'
#define SETITEM     's'
#define TUPLE       't'
#define EMPTY_TUPLE ')'
#define SETITEMS    'u'

static char MARKv = MARK;

static PyObject *PickleError;
static PyObject *PicklingError;
static PyObject *UnpickleableError;
static PyObject *UnpicklingError;
static PyObject *BadPickleGet;


static PyObject *dispatch_table;
static PyObject *safe_constructors;
static PyObject *empty_tuple;

static PyObject *__class___str, *__getinitargs___str, *__dict___str,
  *__getstate___str, *__setstate___str, *__name___str, *__reduce___str,
  *write_str, *__safe_for_unpickling___str, *append_str,
  *read_str, *readline_str, *__main___str, *__basicnew___str,
  *copy_reg_str, *dispatch_table_str, *safe_constructors_str;

/*************************************************************************
 Internal Data type for pickle data.                                     */

typedef struct {
     PyObject_HEAD
     int length, size;
     PyObject **data;
} Pdata;

static void
Pdata_dealloc(Pdata *self) {
    int i;
    PyObject **p;

    for (i=self->length, p=self->data; --i >= 0; p++) Py_DECREF(*p);

    if (self->data) free(self->data);

    PyObject_Del(self);
}

static PyTypeObject PdataType = {
    PyObject_HEAD_INIT(NULL) 0, "cPickle.Pdata", sizeof(Pdata), 0,
    (destructor)Pdata_dealloc,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0L,0L,0L,0L, ""
};

#define Pdata_Check(O) ((O)->ob_type == &PdataType)

static PyObject *
Pdata_New(void) {
    Pdata *self;

    if (!( self = PyObject_New(Pdata, &PdataType)))  return NULL;
    self->size=8;
    self->length=0;
    self->data=malloc(self->size * sizeof(PyObject*));
    if (self->data) return (PyObject*)self;
    Py_DECREF(self);
    return PyErr_NoMemory();
}

static int
stackUnderflow(void) {
    PyErr_SetString(UnpicklingError, "unpickling stack underflow");
    return -1;
}

static int
Pdata_clear(Pdata *self, int clearto) {
    int i;
    PyObject **p;

    if (clearto < 0) return stackUnderflow();
    if (clearto >= self->length) return 0;

    for (i=self->length, p=self->data+clearto; --i >= clearto; p++)
      Py_DECREF(*p);
    self->length=clearto;

    return 0;
}


static int
Pdata_grow(Pdata *self) {
  if (! self->size) {
      PyErr_NoMemory();
      return -1;
  }
  self->size *= 2;
  self->data = realloc(self->data, self->size*sizeof(PyObject*));
  if (! self->data) {
    self->size = 0;
    PyErr_NoMemory();
    return -1;
  }
  return 0;
}

#define PDATA_POP(D,V) {                                      \
    if ((D)->length) V=D->data[--((D)->length)];              \
    else {                                                    \
        PyErr_SetString(UnpicklingError, "bad pickle data");  \
        V=NULL;                                               \
    }                                                         \
}


static PyObject *
Pdata_popTuple(Pdata *self, int start) {
    PyObject *r;
    int i, j, l;

    l=self->length-start;
    if (!( r=PyTuple_New(l)))  return NULL;
    for (i=start, j=0 ; j < l; i++, j++)
        PyTuple_SET_ITEM(r, j, self->data[i]);

    self->length=start;
    return r;
}

static PyObject *
Pdata_popList(Pdata *self, int start) {
    PyObject *r;
    int i, j, l;

    l=self->length-start;
    if (!( r=PyList_New(l)))  return NULL;
    for (i=start, j=0 ; j < l; i++, j++)
        PyList_SET_ITEM(r, j, self->data[i]);

    self->length=start;
    return r;
}

#define PDATA_APPEND_(D,O,ER) { \
  if (Pdata_Append(((Pdata*)(D)), O) < 0) return ER; \
}

#define PDATA_APPEND(D,O,ER) {                                 \
    if (((Pdata*)(D))->length == ((Pdata*)(D))->size &&        \
        Pdata_grow((Pdata*)(D)) < 0)                           \
        return ER;                                             \
    Py_INCREF(O);                                              \
    ((Pdata*)(D))->data[((Pdata*)(D))->length++]=O;            \
}

#define PDATA_PUSH(D,O,ER) {                                   \
    if (((Pdata*)(D))->length == ((Pdata*)(D))->size &&        \
        Pdata_grow((Pdata*)(D)) < 0) {                         \
        Py_DECREF(O);                                          \
        return ER;                                             \
    }                                                          \
    ((Pdata*)(D))->data[((Pdata*)(D))->length++]=O;            \
}

/*************************************************************************/

#define ARG_TUP(self, o) {                          \
  if (self->arg || (self->arg=PyTuple_New(1))) {    \
      Py_XDECREF(PyTuple_GET_ITEM(self->arg,0));    \
      PyTuple_SET_ITEM(self->arg,0,o);              \
  }                                                 \
  else {                                            \
      Py_DECREF(o);                                 \
  }                                                 \
}

#define FREE_ARG_TUP(self) {                        \
    if (self->arg->ob_refcnt > 1) {                 \
      Py_DECREF(self->arg);                         \
      self->arg=NULL;                               \
    }                                               \
  }

typedef struct Picklerobject {
    PyObject_HEAD
    FILE *fp;
    PyObject *write;
    PyObject *file;
    PyObject *memo;
    PyObject *arg;
    PyObject *pers_func;
    PyObject *inst_pers_func;
    int bin;
    int fast; /* Fast mode doesn't save in memo, don't use if circ ref */
    int (*write_func)(struct Picklerobject *, char *, int);
    char *write_buf;
    int buf_size;
    PyObject *dispatch_table;
    int fast_container; /* count nested container dumps */
    PyObject *fast_memo;
} Picklerobject;

#ifndef PY_CPICKLE_FAST_LIMIT
#define PY_CPICKLE_FAST_LIMIT 50
#endif

staticforward PyTypeObject Picklertype;

typedef struct Unpicklerobject {
     PyObject_HEAD
     FILE *fp;
     PyObject *file;
     PyObject *readline;
     PyObject *read;
     PyObject *memo;
     PyObject *arg;
     Pdata *stack;
     PyObject *mark;
     PyObject *pers_func;
     PyObject *last_string;
     int *marks;
     int num_marks;
     int marks_size;
     int (*read_func)(struct Unpicklerobject *, char **, int);
     int (*readline_func)(struct Unpicklerobject *, char **);
     int buf_size;
     char *buf;
     PyObject *safe_constructors;
     PyObject *find_class;
} Unpicklerobject;

staticforward PyTypeObject Unpicklertype;

/* Forward decls that need the above structs */
static int save(Picklerobject *, PyObject *, int);
static int put2(Picklerobject *, PyObject *);

int
cPickle_PyMapping_HasKey(PyObject *o, PyObject *key) {
    PyObject *v;

    if ((v = PyObject_GetItem(o,key))) {
        Py_DECREF(v);
        return 1;
    }

    PyErr_Clear();
    return 0;
}

static
PyObject *
cPickle_ErrFormat(PyObject *ErrType, char *stringformat, char *format, ...)
{
  va_list va;
  PyObject *args=0, *retval=0;
  va_start(va, format);

  if (format) args = Py_VaBuildValue(format, va);
  va_end(va);
  if (format && ! args) return NULL;
  if (stringformat && !(retval=PyString_FromString(stringformat))) return NULL;

  if (retval) {
      if (args) {
          PyObject *v;
          v=PyString_Format(retval, args);
          Py_DECREF(retval);
          Py_DECREF(args);
          if (! v) return NULL;
          retval=v;
        }
    }
  else
    if (args) retval=args;
    else {
        PyErr_SetObject(ErrType,Py_None);
        return NULL;
      }
  PyErr_SetObject(ErrType,retval);
  Py_DECREF(retval);
  return NULL;
}

static int
write_file(Picklerobject *self, char *s, int  n) {
    size_t nbyteswritten;

    if (s == NULL) {
        return 0;
    }

    Py_BEGIN_ALLOW_THREADS
    nbyteswritten = fwrite(s, sizeof(char), n, self->fp);
    Py_END_ALLOW_THREADS
    if (nbyteswritten != (size_t)n) {
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }

    return n;
}

static int
write_cStringIO(Picklerobject *self, char *s, int  n) {
    if (s == NULL) {
        return 0;
    }

    if (PycStringIO->cwrite((PyObject *)self->file, s, n) != n) {
        return -1;
    }

    return n;
}

static int
write_none(Picklerobject *self, char *s, int  n) {
    if (s == NULL) return 0;
    return n;
}

static int
write_other(Picklerobject *self, char *s, int  n) {
    PyObject *py_str = 0, *junk = 0;

    if (s == NULL) {
        if (!( self->buf_size ))  return 0;
        if (!( py_str =
            PyString_FromStringAndSize(self->write_buf, self->buf_size))) 
            return -1;
    }
    else {
        if (self->buf_size && (n + self->buf_size) > WRITE_BUF_SIZE) {
            if (write_other(self, NULL, 0) < 0)
                return -1;
        }

        if (n > WRITE_BUF_SIZE) {
            if (!( py_str =
                PyString_FromStringAndSize(s, n))) 
                return -1;
        }
        else {
            memcpy(self->write_buf + self->buf_size, s, n);
            self->buf_size += n;
            return n;
        }
    }

    if (self->write) {
        /* object with write method */
        ARG_TUP(self, py_str);
        if (self->arg) {
            junk = PyObject_Call(self->write, self->arg, NULL);
            FREE_ARG_TUP(self);
        }
        if (junk) Py_DECREF(junk);
        else return -1;
      }
    else
      PDATA_PUSH(self->file, py_str, -1);

    self->buf_size = 0;
    return n;
}


static int
read_file(Unpicklerobject *self, char **s, int  n) {
    size_t nbytesread;

    if (self->buf_size == 0) {
        int size;

        size = ((n < 32) ? 32 : n);
        if (!( self->buf = (char *)malloc(size * sizeof(char))))  {
            PyErr_NoMemory();
            return -1;
        }

        self->buf_size = size;
    }
    else if (n > self->buf_size) {
        if (!( self->buf = (char *)realloc(self->buf, n * sizeof(char))))  {
            PyErr_NoMemory();
            return -1;
        }

        self->buf_size = n;
    }

    Py_BEGIN_ALLOW_THREADS
    nbytesread = fread(self->buf, sizeof(char), n, self->fp);
    Py_END_ALLOW_THREADS
    if (nbytesread != (size_t)n) {
        if (feof(self->fp)) {
            PyErr_SetNone(PyExc_EOFError);
            return -1;
        }

        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }

    *s = self->buf;

    return n;
}


static int
readline_file(Unpicklerobject *self, char **s) {
    int i;

    if (self->buf_size == 0) {
        if (!( self->buf = (char *)malloc(40 * sizeof(char))))  {
            PyErr_NoMemory();
            return -1;
        }

        self->buf_size = 40;
    }

    i = 0;
    while (1) {
        for (; i < (self->buf_size - 1); i++) {
            if (feof(self->fp) || (self->buf[i] = getc(self->fp)) == '\n') {
                self->buf[i + 1] = '\0';
                *s = self->buf;
                return i + 1;
            }
        }

        if (!( self->buf = (char *)realloc(self->buf,
            (self->buf_size * 2) * sizeof(char))))  {
            PyErr_NoMemory();
            return -1;
        }

        self->buf_size *= 2;
    }

}


static int
read_cStringIO(Unpicklerobject *self, char **s, int  n) {
    char *ptr;

    if (PycStringIO->cread((PyObject *)self->file, &ptr, n) != n) {
        PyErr_SetNone(PyExc_EOFError);
        return -1;
    }

    *s = ptr;

    return n;
}


static int
readline_cStringIO(Unpicklerobject *self, char **s) {
    int n;
    char *ptr;

    if ((n = PycStringIO->creadline((PyObject *)self->file, &ptr)) < 0) {
        return -1;
    }

    *s = ptr;

    return n;
}


static int
read_other(Unpicklerobject *self, char **s, int  n) {
    PyObject *bytes, *str=0;

    if (!( bytes = PyInt_FromLong(n)))  return -1;

    ARG_TUP(self, bytes);
    if (self->arg) {
        str = PyObject_Call(self->read, self->arg, NULL);
        FREE_ARG_TUP(self);
    }
    if (! str) return -1;

    Py_XDECREF(self->last_string);
    self->last_string = str;

    if (! (*s = PyString_AsString(str))) return -1;
    return n;
}


static int
readline_other(Unpicklerobject *self, char **s) {
    PyObject *str;
    int str_size;

    if (!( str = PyObject_CallObject(self->readline, empty_tuple)))  {
        return -1;
    }

    if ((str_size = PyString_Size(str)) < 0)
      return -1;

    Py_XDECREF(self->last_string);
    self->last_string = str;

    if (! (*s = PyString_AsString(str)))
      return -1;

    return str_size;
}


static char *
pystrndup(char *s, int l) {
  char *r;
  if (!( r=malloc((l+1)*sizeof(char))))  return (char*)PyErr_NoMemory();
  memcpy(r,s,l);
  r[l]=0;
  return r;
}


static int
get(Picklerobject *self, PyObject *id) {
    PyObject *value, *mv;
    long c_value;
    char s[30];
    size_t len;

    if (!( mv = PyDict_GetItem(self->memo, id)))  {
        PyErr_SetObject(PyExc_KeyError, id);
        return -1;
      }

    if (!( value = PyTuple_GetItem(mv, 0))) 
        return -1;

    if (!( PyInt_Check(value)))  {
      PyErr_SetString(PicklingError, "no int where int expected in memo");
      return -1;
    }
    c_value = PyInt_AS_LONG((PyIntObject*)value);

    if (!self->bin) {
        s[0] = GET;
        PyOS_snprintf(s + 1, sizeof(s) - 1, "%ld\n", c_value);
        len = strlen(s);
    }
    else if (Pdata_Check(self->file)) {
        if (write_other(self, NULL, 0) < 0) return -1;
        PDATA_APPEND(self->file, mv, -1);
        return 0;
      }
    else {
        if (c_value < 256) {
            s[0] = BINGET;
            s[1] = (int)(c_value & 0xff);
            len = 2;
        }
        else {
            s[0] = LONG_BINGET;
            s[1] = (int)(c_value & 0xff);
            s[2] = (int)((c_value >> 8)  & 0xff);
            s[3] = (int)((c_value >> 16) & 0xff);
            s[4] = (int)((c_value >> 24) & 0xff);
            len = 5;
        }
    }

    if ((*self->write_func)(self, s, len) < 0)
        return -1;

    return 0;
}


static int
put(Picklerobject *self, PyObject *ob) {
    if (ob->ob_refcnt < 2 || self->fast) 
        return 0;

    return put2(self, ob);
}


static int
put2(Picklerobject *self, PyObject *ob) {
    char c_str[30];
    int p;
    size_t len;
    int res = -1;
    PyObject *py_ob_id = 0, *memo_len = 0, *t = 0;

    if (self->fast)
	return 0;

    if ((p = PyDict_Size(self->memo)) < 0)
        goto finally;

    p++;  /* Make sure memo keys are positive! */

    if (!( py_ob_id = PyLong_FromVoidPtr(ob))) 
        goto finally;

    if (!( memo_len = PyInt_FromLong(p))) 
        goto finally;

    if (!( t = PyTuple_New(2))) 
        goto finally;

    PyTuple_SET_ITEM(t, 0, memo_len);
    Py_INCREF(memo_len);
    PyTuple_SET_ITEM(t, 1, ob);
    Py_INCREF(ob);

    if (PyDict_SetItem(self->memo, py_ob_id, t) < 0)
        goto finally;

    if (!self->bin) {
        c_str[0] = PUT;
        PyOS_snprintf(c_str + 1, sizeof(c_str) - 1, "%d\n", p);
        len = strlen(c_str);
    }
    else if (Pdata_Check(self->file)) {
        if (write_other(self, NULL, 0) < 0) return -1;
        PDATA_APPEND(self->file, memo_len, -1);
        res=0;          /* Job well done ;) */
        goto finally;
    }
    else {
        if (p >= 256) {
            c_str[0] = LONG_BINPUT;
            c_str[1] = (int)(p & 0xff);
            c_str[2] = (int)((p >> 8)  & 0xff);
            c_str[3] = (int)((p >> 16) & 0xff);
            c_str[4] = (int)((p >> 24) & 0xff);
            len = 5;
        }
        else {
            c_str[0] = BINPUT;
            c_str[1] = p;
            len = 2;
        }
    }

    if ((*self->write_func)(self, c_str, len) < 0)
        goto finally;

    res = 0;

finally:
    Py_XDECREF(py_ob_id);
    Py_XDECREF(memo_len);
    Py_XDECREF(t);

    return res;
}

#define PyImport_Import cPickle_Import

static PyObject *
PyImport_Import(PyObject *module_name) {
  static PyObject *silly_list=0, *__builtins___str=0, *__import___str;
  static PyObject *standard_builtins=0;
  PyObject *globals=0, *__import__=0, *__builtins__=0, *r=0;

  if (!( silly_list ))  {
      if (!( __import___str=PyString_FromString("__import__"))) 
        return NULL;
      if (!( __builtins___str=PyString_FromString("__builtins__"))) 
        return NULL;
      if (!( silly_list=Py_BuildValue("[s]","__doc__"))) 
        return NULL;
    }

  if ((globals=PyEval_GetGlobals())) {
      Py_INCREF(globals);
      if (!( __builtins__=PyObject_GetItem(globals,__builtins___str))) 
        goto err;
    }
  else {
      PyErr_Clear();

      if (!( standard_builtins ||
             (standard_builtins=PyImport_ImportModule("__builtin__")))) 
        return NULL;

      __builtins__=standard_builtins;
      Py_INCREF(__builtins__);
      if (!( globals = Py_BuildValue("{sO}", "__builtins__", __builtins__))) 
        goto err;
    }

  if (PyDict_Check(__builtins__)) {
    if (!( __import__=PyObject_GetItem(__builtins__,__import___str)))  goto err;
  }
  else {
    if (!( __import__=PyObject_GetAttr(__builtins__,__import___str)))  goto err;
  }

  if (!( r=PyObject_CallFunction(__import__,"OOOO",
                                 module_name, globals, globals, silly_list))) 
    goto err;

  Py_DECREF(globals);
  Py_DECREF(__builtins__);
  Py_DECREF(__import__);

  return r;
err:
  Py_XDECREF(globals);
  Py_XDECREF(__builtins__);
  Py_XDECREF(__import__);
  return NULL;
}

static PyObject *
whichmodule(PyObject *global, PyObject *global_name) {
    int i, j;
    PyObject *module = 0, *modules_dict = 0,
        *global_name_attr = 0, *name = 0;

    module = PyObject_GetAttrString(global, "__module__");
    if (module) return module;
    PyErr_Clear();

    if (!( modules_dict = PySys_GetObject("modules"))) 
        return NULL;

    i = 0;
    while ((j = PyDict_Next(modules_dict, &i, &name, &module))) {

        if (PyObject_Compare(name, __main___str)==0) continue;

        if (!( global_name_attr = PyObject_GetAttr(module, global_name)))  {
            PyErr_Clear();
            continue;
        }

        if (global_name_attr != global) {
            Py_DECREF(global_name_attr);
            continue;
        }

        Py_DECREF(global_name_attr);

        break;
    }

    /* The following implements the rule in pickle.py added in 1.5
       that used __main__ if no module is found.  I don't actually
       like this rule. jlf
     */
    if (!j) {
        j=1;
        name=__main___str;
    }

    Py_INCREF(name);
    return name;
}


static int
fast_save_enter(Picklerobject *self, PyObject *obj)
{
    /* if fast_container < 0, we're doing an error exit. */
    if (++self->fast_container >= PY_CPICKLE_FAST_LIMIT) {
	PyObject *key = NULL;
	if (self->fast_memo == NULL) {
	    self->fast_memo = PyDict_New();
	    if (self->fast_memo == NULL) {
		self->fast_container = -1;
		return 0;
	    }
	}
	key = PyLong_FromVoidPtr(obj);
	if (key == NULL)
	    return 0;
	if (PyDict_GetItem(self->fast_memo, key)) {
	    PyErr_Format(PyExc_ValueError,
 "fast mode: can't pickle cyclic objects including object type %s at %p",
			 obj->ob_type->tp_name, obj);
	    self->fast_container = -1;
	    return 0;
	}
	if (PyDict_SetItem(self->fast_memo, key, Py_None) < 0) {
	    self->fast_container = -1;
	    return 0;
	}
    }
    return 1;
}

int 
fast_save_leave(Picklerobject *self, PyObject *obj)
{
    if (self->fast_container-- >= PY_CPICKLE_FAST_LIMIT) {
	PyObject *key = PyLong_FromVoidPtr(obj);
	if (key == NULL)
	    return 0;
	if (PyDict_DelItem(self->fast_memo, key) < 0) {
	    return 0;
	}
    }
    return 1;
}

static int
save_none(Picklerobject *self, PyObject *args) {
    static char none = NONE;
    if ((*self->write_func)(self, &none, 1) < 0)
        return -1;

    return 0;
}


static int
save_int(Picklerobject *self, PyObject *args) {
    char c_str[32];
    long l = PyInt_AS_LONG((PyIntObject *)args);
    int len = 0;

    if (!self->bin
#if SIZEOF_LONG > 4
        || l >  0x7fffffffL
        || l < -0x80000000L
#endif
       ) {
        /* Text-mode pickle, or long too big to fit in the 4-byte
         * signed BININT format:  store as a string.
         */
        c_str[0] = INT;
        PyOS_snprintf(c_str + 1, sizeof(c_str) - 1, "%ld\n", l);
        if ((*self->write_func)(self, c_str, strlen(c_str)) < 0)
            return -1;
    }
    else {
        /* Binary pickle and l fits in a signed 4-byte int. */
        c_str[1] = (int)( l        & 0xff);
        c_str[2] = (int)((l >> 8)  & 0xff);
        c_str[3] = (int)((l >> 16) & 0xff);
        c_str[4] = (int)((l >> 24) & 0xff);

        if ((c_str[4] == 0) && (c_str[3] == 0)) {
            if (c_str[2] == 0) {
                c_str[0] = BININT1;
                len = 2;
            }
            else {
                c_str[0] = BININT2;
                len = 3;
            }
        }
        else {
            c_str[0] = BININT;
            len = 5;
        }

        if ((*self->write_func)(self, c_str, len) < 0)
            return -1;
    }

    return 0;
}


static int
save_long(Picklerobject *self, PyObject *args) {
    int size, res = -1;
    PyObject *repr = 0;

    static char l = LONG;

    if (!( repr = PyObject_Repr(args))) 
        goto finally;

    if ((size = PyString_Size(repr)) < 0)
        goto finally;

    if ((*self->write_func)(self, &l, 1) < 0)
        goto finally;

    if ((*self->write_func)(self,
        PyString_AS_STRING((PyStringObject *)repr), size) < 0)
        goto finally;

    if ((*self->write_func)(self, "\n", 1) < 0)
        goto finally;

    res = 0;

finally:
    Py_XDECREF(repr);

    return res;
}


static int
save_float(Picklerobject *self, PyObject *args) {
    double x = PyFloat_AS_DOUBLE((PyFloatObject *)args);

    if (self->bin) {
        int s, e;
        double f;
        long fhi, flo;
        char str[9];
        unsigned char *p = (unsigned char *)str;

        *p = BINFLOAT;
        p++;

        if (x < 0) {
            s = 1;
            x = -x;
        }
        else
            s = 0;

        f = frexp(x, &e);

        /* Normalize f to be in the range [1.0, 2.0) */
        if (0.5 <= f && f < 1.0) {
            f *= 2.0;
            e--;
        }
        else if (f == 0.0) {
            e = 0;
        }
        else {
            PyErr_SetString(PyExc_SystemError,
                            "frexp() result out of range");
            return -1;
        }

        if (e >= 1024) {
            /* XXX 1024 itself is reserved for Inf/NaN */
            PyErr_SetString(PyExc_OverflowError,
                            "float too large to pack with d format");
            return -1;
        }
        else if (e < -1022) {
            /* Gradual underflow */
            f = ldexp(f, 1022 + e);
            e = 0;
        }
        else if (!(e == 0 && f == 0.0)) {
            e += 1023;
            f -= 1.0; /* Get rid of leading 1 */
        }

        /* fhi receives the high 28 bits; flo the low 24 bits (== 52 bits) */
        f *= 268435456.0; /* 2**28 */
        fhi = (long) floor(f); /* Truncate */
        f -= (double)fhi;
        f *= 16777216.0; /* 2**24 */
        flo = (long) floor(f + 0.5); /* Round */

        /* First byte */
        *p = (s<<7) | (e>>4);
        p++;

        /* Second byte */
        *p = (unsigned char) (((e&0xF)<<4) | (fhi>>24));
        p++;

        /* Third byte */
        *p = (unsigned char) ((fhi>>16) & 0xFF);
        p++;

        /* Fourth byte */
        *p = (unsigned char) ((fhi>>8) & 0xFF);
        p++;

        /* Fifth byte */
        *p = (unsigned char) (fhi & 0xFF);
        p++;

        /* Sixth byte */
        *p = (unsigned char) ((flo>>16) & 0xFF);
        p++;

        /* Seventh byte */
        *p = (unsigned char) ((flo>>8) & 0xFF);
        p++;

        /* Eighth byte */
        *p = (unsigned char) (flo & 0xFF);

        if ((*self->write_func)(self, str, 9) < 0)
            return -1;
    }
    else {
        char c_str[250];
        c_str[0] = FLOAT;
        PyOS_snprintf(c_str + 1, sizeof(c_str) - 1, "%.17g\n", x);

        if ((*self->write_func)(self, c_str, strlen(c_str)) < 0)
            return -1;
    }

    return 0;
}


static int
save_string(Picklerobject *self, PyObject *args, int doput) {
    int size, len;
    PyObject *repr=0;

    if ((size = PyString_Size(args)) < 0)
      return -1;

    if (!self->bin) {
        char *repr_str;

        static char string = STRING;

        if (!( repr = PyObject_Repr(args))) 
            return -1;

        if ((len = PyString_Size(repr)) < 0)
          goto err;
        repr_str = PyString_AS_STRING((PyStringObject *)repr);

        if ((*self->write_func)(self, &string, 1) < 0)
            goto err;

        if ((*self->write_func)(self, repr_str, len) < 0)
            goto err;

        if ((*self->write_func)(self, "\n", 1) < 0)
            goto err;

        Py_XDECREF(repr);
    }
    else {
        int i;
        char c_str[5];

        if ((size = PyString_Size(args)) < 0)
          return -1;

        if (size < 256) {
            c_str[0] = SHORT_BINSTRING;
            c_str[1] = size;
            len = 2;
        }
        else {
            c_str[0] = BINSTRING;
            for (i = 1; i < 5; i++)
                c_str[i] = (int)(size >> ((i - 1) * 8));
            len = 5;
        }

        if ((*self->write_func)(self, c_str, len) < 0)
            return -1;

        if (size > 128 && Pdata_Check(self->file)) {
            if (write_other(self, NULL, 0) < 0) return -1;
            PDATA_APPEND(self->file, args, -1);
          }
        else {
          if ((*self->write_func)(self,
              PyString_AS_STRING((PyStringObject *)args), size) < 0)
            return -1;
        }
    }

    if (doput)
      if (put(self, args) < 0)
        return -1;

    return 0;

err:
    Py_XDECREF(repr);
    return -1;
}


#ifdef Py_USING_UNICODE
/* A copy of PyUnicode_EncodeRawUnicodeEscape() that also translates
   backslash and newline characters to \uXXXX escapes. */
static PyObject *
modified_EncodeRawUnicodeEscape(const Py_UNICODE *s, int size)
{
    PyObject *repr;
    char *p;
    char *q;

    static const char *hexdigit = "0123456789ABCDEF";

    repr = PyString_FromStringAndSize(NULL, 6 * size);
    if (repr == NULL)
        return NULL;
    if (size == 0)
	return repr;

    p = q = PyString_AS_STRING(repr);
    while (size-- > 0) {
        Py_UNICODE ch = *s++;
	/* Map 16-bit characters to '\uxxxx' */
	if (ch >= 256 || ch == '\\' || ch == '\n') {
            *p++ = '\\';
            *p++ = 'u';
            *p++ = hexdigit[(ch >> 12) & 0xf];
            *p++ = hexdigit[(ch >> 8) & 0xf];
            *p++ = hexdigit[(ch >> 4) & 0xf];
            *p++ = hexdigit[ch & 15];
        }
	/* Copy everything else as-is */
	else
            *p++ = (char) ch;
    }
    *p = '\0';
    if (_PyString_Resize(&repr, p - q))
	goto onError;

    return repr;

 onError:
    Py_DECREF(repr);
    return NULL;
}


static int
save_unicode(Picklerobject *self, PyObject *args, int doput) {
    int size, len;
    PyObject *repr=0;

    if (!PyUnicode_Check(args))
      return -1;

    if (!self->bin) {
        char *repr_str;
        static char string = UNICODE;

        if (!( repr = modified_EncodeRawUnicodeEscape(
		PyUnicode_AS_UNICODE(args), PyUnicode_GET_SIZE(args)))) 
            return -1;

        if ((len = PyString_Size(repr)) < 0)
          goto err;
        repr_str = PyString_AS_STRING((PyStringObject *)repr);

        if ((*self->write_func)(self, &string, 1) < 0)
            goto err;

        if ((*self->write_func)(self, repr_str, len) < 0)
            goto err;

        if ((*self->write_func)(self, "\n", 1) < 0)
            goto err;

        Py_XDECREF(repr);
    }
    else {
        int i;
        char c_str[5];

        if (!( repr = PyUnicode_AsUTF8String(args))) 
            return -1;

        if ((size = PyString_Size(repr)) < 0)
	    goto err;

	c_str[0] = BINUNICODE;
	for (i = 1; i < 5; i++)
	    c_str[i] = (int)(size >> ((i - 1) * 8));
	len = 5;

        if ((*self->write_func)(self, c_str, len) < 0)
            goto err;

        if (size > 128 && Pdata_Check(self->file)) {
            if (write_other(self, NULL, 0) < 0)
		goto err;
            PDATA_APPEND(self->file, repr, -1);
          }
        else {
          if ((*self->write_func)(self, PyString_AS_STRING(repr), size) < 0)
	      goto err;
        }

	Py_DECREF(repr);
    }

    if (doput)
      if (put(self, args) < 0)
        return -1;

    return 0;

err:
    Py_XDECREF(repr);
    return -1;
}
#endif


static int
save_tuple(Picklerobject *self, PyObject *args) {
    PyObject *element = 0, *py_tuple_id = 0;
    int len, i, res = -1;

    static char tuple = TUPLE;

    if ((*self->write_func)(self, &MARKv, 1) < 0)
        goto finally;

    if ((len = PyTuple_Size(args)) < 0)
        goto finally;

    for (i = 0; i < len; i++) {
        if (!( element = PyTuple_GET_ITEM((PyTupleObject *)args, i))) 
            goto finally;

        if (save(self, element, 0) < 0)
            goto finally;
    }

    if (!( py_tuple_id = PyLong_FromVoidPtr(args))) 
        goto finally;

    if (len) {
	if (PyDict_GetItem(self->memo, py_tuple_id)) {
            if (self->bin) {
                static char pop_mark = POP_MARK;

                if ((*self->write_func)(self, &pop_mark, 1) < 0)
                    goto finally;
            }
            else {
                static char pop = POP;

                for (i = 0; i <= len; i++) {
                    if ((*self->write_func)(self, &pop, 1) < 0)
                        goto finally;
                }
            }

            if (get(self, py_tuple_id) < 0)
                goto finally;

            res = 0;
            goto finally;
        }
    }

    if ((*self->write_func)(self, &tuple, 1) < 0) {
        goto finally;
    }

    if (put(self, args) < 0)
        goto finally;

    res = 0;

finally:
    Py_XDECREF(py_tuple_id);

    return res;
}

static int
save_empty_tuple(Picklerobject *self, PyObject *args) {
    static char tuple = EMPTY_TUPLE;

    return (*self->write_func)(self, &tuple, 1);
}


static int
save_list(Picklerobject *self, PyObject *args) {
    PyObject *element = 0;
    int s_len, len, i, using_appends, res = -1;
    char s[3];

    static char append = APPEND, appends = APPENDS;

    if (self->fast && !fast_save_enter(self, args))
	goto finally;

    if (self->bin) {
        s[0] = EMPTY_LIST;
        s_len = 1;
    }
    else {
        s[0] = MARK;
        s[1] = LIST;
        s_len = 2;
    }

    if ((len = PyList_Size(args)) < 0)
        goto finally;

    if ((*self->write_func)(self, s, s_len) < 0)
        goto finally;

    if (len == 0) {
        if (put(self, args) < 0)
            goto finally;
    }
    else {
        if (put2(self, args) < 0)
            goto finally;
    }

    if ((using_appends = (self->bin && (len > 1))))
        if ((*self->write_func)(self, &MARKv, 1) < 0)
            goto finally;

    for (i = 0; i < len; i++) {
        if (!( element = PyList_GET_ITEM((PyListObject *)args, i))) 
            goto finally;

        if (save(self, element, 0) < 0)
            goto finally;

        if (!using_appends) {
            if ((*self->write_func)(self, &append, 1) < 0)
                goto finally;
        }
    }

    if (using_appends) {
        if ((*self->write_func)(self, &appends, 1) < 0)
            goto finally;
    }

    res = 0;

finally:
    if (self->fast && !fast_save_leave(self, args))
	res = -1;

    return res;
}


static int
save_dict(Picklerobject *self, PyObject *args) {
    PyObject *key = 0, *value = 0;
    int i, len, res = -1, using_setitems;
    char s[3];

    static char setitem = SETITEM, setitems = SETITEMS;

    if (self->fast && !fast_save_enter(self, args))
	goto finally;

    if (self->bin) {
        s[0] = EMPTY_DICT;
        len = 1;
    }
    else {
        s[0] = MARK;
        s[1] = DICT;
        len = 2;
    }

    if ((*self->write_func)(self, s, len) < 0)
        goto finally;

    if ((len = PyDict_Size(args)) < 0)
        goto finally;

    if (len == 0) {
        if (put(self, args) < 0)
            goto finally;
    }
    else {
        if (put2(self, args) < 0)
            goto finally;
    }

    if ((using_setitems = (self->bin && (PyDict_Size(args) > 1))))
        if ((*self->write_func)(self, &MARKv, 1) < 0)
            goto finally;

    i = 0;
    while (PyDict_Next(args, &i, &key, &value)) {
        if (save(self, key, 0) < 0)
            goto finally;

        if (save(self, value, 0) < 0)
            goto finally;

        if (!using_setitems) {
            if ((*self->write_func)(self, &setitem, 1) < 0)
                goto finally;
        }
    }

    if (using_setitems) {
        if ((*self->write_func)(self, &setitems, 1) < 0)
            goto finally;
    }

    res = 0;

finally:
    if (self->fast && !fast_save_leave(self, args))
	res = -1;

    return res;
}


static int
save_inst(Picklerobject *self, PyObject *args) {
    PyObject *class = 0, *module = 0, *name = 0, *state = 0,
             *getinitargs_func = 0, *getstate_func = 0, *class_args = 0;
    char *module_str, *name_str;
    int module_size, name_size, res = -1;

    static char inst = INST, obj = OBJ, build = BUILD;

    if (self->fast && !fast_save_enter(self, args))
	goto finally;

    if ((*self->write_func)(self, &MARKv, 1) < 0)
        goto finally;

    if (!( class = PyObject_GetAttr(args, __class___str))) 
        goto finally;

    if (self->bin) {
        if (save(self, class, 0) < 0)
            goto finally;
    }

    if ((getinitargs_func = PyObject_GetAttr(args, __getinitargs___str))) {
        PyObject *element = 0;
        int i, len;

        if (!( class_args =
            PyObject_Call(getinitargs_func, empty_tuple, NULL))) 
            goto finally;

        if ((len = PyObject_Size(class_args)) < 0)
            goto finally;

        for (i = 0; i < len; i++) {
            if (!( element = PySequence_GetItem(class_args, i))) 
                goto finally;

            if (save(self, element, 0) < 0) {
                Py_DECREF(element);
                goto finally;
            }

            Py_DECREF(element);
        }
    }
    else {
        PyErr_Clear();
    }

    if (!self->bin) {
        if (!( name = ((PyClassObject *)class)->cl_name ))  {
            PyErr_SetString(PicklingError, "class has no name");
            goto finally;
        }

        if (!( module = whichmodule(class, name))) 
            goto finally;


        if ((module_size = PyString_Size(module)) < 0 ||
           (name_size = PyString_Size(name)) < 0)
          goto finally;

        module_str = PyString_AS_STRING((PyStringObject *)module);
        name_str   = PyString_AS_STRING((PyStringObject *)name);

        if ((*self->write_func)(self, &inst, 1) < 0)
            goto finally;

        if ((*self->write_func)(self, module_str, module_size) < 0)
            goto finally;

        if ((*self->write_func)(self, "\n", 1) < 0)
            goto finally;

        if ((*self->write_func)(self, name_str, name_size) < 0)
            goto finally;

        if ((*self->write_func)(self, "\n", 1) < 0)
            goto finally;
    }
    else if ((*self->write_func)(self, &obj, 1) < 0) {
        goto finally;
    }

    if ((getstate_func = PyObject_GetAttr(args, __getstate___str))) {
        if (!( state = PyObject_Call(getstate_func, empty_tuple, NULL))) 
            goto finally;
    }
    else {
        PyErr_Clear();

        if (!( state = PyObject_GetAttr(args, __dict___str)))  {
            PyErr_Clear();
            res = 0;
            goto finally;
        }
    }

    if (!PyDict_Check(state)) {
        if (put2(self, args) < 0)
            goto finally;
    }
    else {
        if (put(self, args) < 0)
            goto finally;
    }

    if (save(self, state, 0) < 0)
        goto finally;

    if ((*self->write_func)(self, &build, 1) < 0)
        goto finally;

    res = 0;

finally:
    if (self->fast && !fast_save_leave(self, args))
	res = -1;

    Py_XDECREF(module);
    Py_XDECREF(class);
    Py_XDECREF(state);
    Py_XDECREF(getinitargs_func);
    Py_XDECREF(getstate_func);
    Py_XDECREF(class_args);

    return res;
}


static int
save_global(Picklerobject *self, PyObject *args, PyObject *name) {
    PyObject *global_name = 0, *module = 0, *mod = 0, *moddict = 0, *klass = 0;
    char *name_str, *module_str;
    int module_size, name_size, res = -1;

    static char global = GLOBAL;

    if (name) {
        global_name = name;
        Py_INCREF(global_name);
    }
    else {
        if (!( global_name = PyObject_GetAttr(args, __name___str))) 
            goto finally;
    }

    if (!( module = whichmodule(args, global_name))) 
        goto finally;

    if ((module_size = PyString_Size(module)) < 0 ||
        (name_size = PyString_Size(global_name)) < 0)
      goto finally;

    module_str = PyString_AS_STRING((PyStringObject *)module);
    name_str   = PyString_AS_STRING((PyStringObject *)global_name);

    mod = PyImport_ImportModule(module_str);
    if (mod == NULL) {
        /* Py_ErrClear(); ?? */
	cPickle_ErrFormat(PicklingError,
			  "Can't pickle %s: it's not found as %s.%s",
			  "OSS", args, module, global_name);
	goto finally;
    }
    moddict = PyModule_GetDict(mod);        /* borrowed ref */
    klass = PyDict_GetItemString(moddict, name_str);        /* borrowed ref */
    if (klass == NULL) {
	cPickle_ErrFormat(PicklingError,
			  "Can't pickle %s: it's not found as %s.%s",
			  "OSS", args, module, global_name);
	goto finally;
    }
    if (klass != args) {
	cPickle_ErrFormat(PicklingError,
			  "Can't pickle %s: it's not the same object as %s.%s",
			  "OSS", args, module, global_name);
	goto finally;
    }

    if ((*self->write_func)(self, &global, 1) < 0)
        goto finally;

    if ((*self->write_func)(self, module_str, module_size) < 0)
        goto finally;

    if ((*self->write_func)(self, "\n", 1) < 0)
        goto finally;

    if ((*self->write_func)(self, name_str, name_size) < 0)
        goto finally;

    if ((*self->write_func)(self, "\n", 1) < 0)
        goto finally;

    if (put(self, args) < 0)
        goto finally;

    res = 0;

finally:
    Py_XDECREF(module);
    Py_XDECREF(global_name);
    Py_XDECREF(mod);

    return res;
}

static int
save_pers(Picklerobject *self, PyObject *args, PyObject *f) {
    PyObject *pid = 0;
    int size, res = -1;

    static char persid = PERSID, binpersid = BINPERSID;

    Py_INCREF(args);
    ARG_TUP(self, args);
    if (self->arg) {
        pid = PyObject_Call(f, self->arg, NULL);
        FREE_ARG_TUP(self);
    }
    if (! pid) return -1;

    if (pid != Py_None) {
        if (!self->bin) {
            if (!PyString_Check(pid)) {
                PyErr_SetString(PicklingError,
                    "persistent id must be string");
                goto finally;
            }

            if ((*self->write_func)(self, &persid, 1) < 0)
                goto finally;

            if ((size = PyString_Size(pid)) < 0)
                goto finally;

            if ((*self->write_func)(self,
                PyString_AS_STRING((PyStringObject *)pid), size) < 0)
                goto finally;

            if ((*self->write_func)(self, "\n", 1) < 0)
                goto finally;

            res = 1;
            goto finally;
        }
        else if (save(self, pid, 1) >= 0) {
            if ((*self->write_func)(self, &binpersid, 1) < 0)
                res = -1;
            else
                res = 1;
        }

        goto finally;
    }

    res = 0;

finally:
    Py_XDECREF(pid);

    return res;
}


static int
save_reduce(Picklerobject *self, PyObject *callable,
            PyObject *tup, PyObject *state, PyObject *ob) {
    static char reduce = REDUCE, build = BUILD;

    if (save(self, callable, 0) < 0)
        return -1;

    if (save(self, tup, 0) < 0)
        return -1;

    if ((*self->write_func)(self, &reduce, 1) < 0)
        return -1;

    if (ob != NULL) {
        if (state && !PyDict_Check(state)) {
            if (put2(self, ob) < 0)
                return -1;
        }
        else {
            if (put(self, ob) < 0)
                return -1;
        }
    }

    if (state) {
        if (save(self, state, 0) < 0)
            return -1;

        if ((*self->write_func)(self, &build, 1) < 0)
            return -1;
    }

    return 0;
}

static int
save(Picklerobject *self, PyObject *args, int  pers_save) {
    PyTypeObject *type;
    PyObject *py_ob_id = 0, *__reduce__ = 0, *t = 0, *arg_tup = 0,
             *callable = 0, *state = 0;
    int res = -1, tmp, size;

    if (!pers_save && self->pers_func) {
        if ((tmp = save_pers(self, args, self->pers_func)) != 0) {
            res = tmp;
            goto finally;
        }
    }

    if (args == Py_None) {
        res = save_none(self, args);
        goto finally;
    }

    type = args->ob_type;

    switch (type->tp_name[0]) {
        case 'i':
            if (type == &PyInt_Type) {
                res = save_int(self, args);
                goto finally;
            }
            break;

        case 'l':
            if (type == &PyLong_Type) {
                res = save_long(self, args);
                goto finally;
            }
            break;

        case 'f':
            if (type == &PyFloat_Type) {
                res = save_float(self, args);
                goto finally;
            }
            break;

        case 't':
            if (type == &PyTuple_Type && PyTuple_Size(args)==0) {
                if (self->bin) res = save_empty_tuple(self, args);
                else          res = save_tuple(self, args);
                goto finally;
            }
            break;

        case 's':
            if ((type == &PyString_Type) && (PyString_GET_SIZE(args) < 2)) {
                res = save_string(self, args, 0);
                goto finally;
            }

#ifdef Py_USING_UNICODE
        case 'u':
            if ((type == &PyUnicode_Type) && (PyString_GET_SIZE(args) < 2)) {
                res = save_unicode(self, args, 0);
                goto finally;
            }
#endif
    }

    if (args->ob_refcnt > 1) {
        if (!( py_ob_id = PyLong_FromVoidPtr(args))) 
            goto finally;

	if (PyDict_GetItem(self->memo, py_ob_id)) {
            if (get(self, py_ob_id) < 0)
                goto finally;

            res = 0;
            goto finally;
        }
    }

    switch (type->tp_name[0]) {
        case 's':
            if (type == &PyString_Type) {
                res = save_string(self, args, 1);
                goto finally;
            }
            break;

#ifdef Py_USING_UNICODE
        case 'u':
            if (type == &PyUnicode_Type) {
                res = save_unicode(self, args, 1);
                goto finally;
            }
            break;
#endif

        case 't':
            if (type == &PyTuple_Type) {
                res = save_tuple(self, args);
                goto finally;
            }
	    if (type == &PyType_Type) {
		res = save_global(self, args, NULL);
		goto finally;
	    }
            break;

        case 'l':
            if (type == &PyList_Type) {
                res = save_list(self, args);
                goto finally;
            }
            break;

        case 'd':
            if (type == &PyDict_Type) {
                res = save_dict(self, args);
                goto finally;
            }
            break;

        case 'i':
            if (type == &PyInstance_Type) {
                res = save_inst(self, args);
                goto finally;
            }
            break;

        case 'c':
            if (type == &PyClass_Type) {
                res = save_global(self, args, NULL);
                goto finally;
            }
            break;

        case 'f':
            if (type == &PyFunction_Type) {
                res = save_global(self, args, NULL);
                goto finally;
            }
            break;

        case 'b':
            if (type == &PyCFunction_Type) {
                res = save_global(self, args, NULL);
                goto finally;
            }
    }

    if (PyType_IsSubtype(type, &PyType_Type)) {
	res = save_global(self, args, NULL);
	goto finally;
    }

    if (!pers_save && self->inst_pers_func) {
        if ((tmp = save_pers(self, args, self->inst_pers_func)) != 0) {
            res = tmp;
            goto finally;
        }
    }

    if ((__reduce__ = PyDict_GetItem(dispatch_table, (PyObject *)type))) {
        Py_INCREF(__reduce__);

        Py_INCREF(args);
        ARG_TUP(self, args);
        if (self->arg) {
            t = PyObject_Call(__reduce__, self->arg, NULL);
            FREE_ARG_TUP(self);
        }
        if (! t) goto finally;
    }
    else {
        PyErr_Clear();

        if ((__reduce__ = PyObject_GetAttr(args, __reduce___str))) {
            if (!( t = PyObject_Call(__reduce__, empty_tuple, NULL))) 
                goto finally;
        }
        else {
            PyErr_Clear();
        }
    }

    if (t) {
        if (PyString_Check(t)) {
            res = save_global(self, args, t);
            goto finally;
        }

        if (!PyTuple_Check(t)) {
            cPickle_ErrFormat(PicklingError, "Value returned by %s must "
                "be a tuple", "O", __reduce__);
            goto finally;
        }

        size = PyTuple_Size(t);

        if ((size != 3) && (size != 2)) {
            cPickle_ErrFormat(PicklingError, "tuple returned by %s must "
                "contain only two or three elements", "O", __reduce__);
                goto finally;
        }

        callable = PyTuple_GET_ITEM(t, 0);

        arg_tup = PyTuple_GET_ITEM(t, 1);

        if (size > 2) {
            state = PyTuple_GET_ITEM(t, 2);
        }

        if (!( PyTuple_Check(arg_tup) || arg_tup==Py_None ))  {
            cPickle_ErrFormat(PicklingError, "Second element of tuple "
                "returned by %s must be a tuple", "O", __reduce__);
            goto finally;
        }

        res = save_reduce(self, callable, arg_tup, state, args);
        goto finally;
    }

    PyErr_SetObject(UnpickleableError, args);

finally:
    Py_XDECREF(py_ob_id);
    Py_XDECREF(__reduce__);
    Py_XDECREF(t);

    return res;
}


static int
dump(Picklerobject *self, PyObject *args) {
    static char stop = STOP;

    if (save(self, args, 0) < 0)
        return -1;

    if ((*self->write_func)(self, &stop, 1) < 0)
        return -1;

    if ((*self->write_func)(self, NULL, 0) < 0)
        return -1;

    return 0;
}

static PyObject *
Pickle_clear_memo(Picklerobject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args,":clear_memo")) 
	return NULL;
    if (self->memo) 
	PyDict_Clear(self->memo);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Pickle_getvalue(Picklerobject *self, PyObject *args) {
  int l, i, rsize, ssize, clear=1, lm;
  long ik;
  PyObject *k, *r;
  char *s, *p, *have_get;
  Pdata *data;

  /* Can be called by Python code or C code */
  if (args && !PyArg_ParseTuple(args, "|i:getvalue", &clear)) 
      return NULL;

  /* Check to make sure we are based on a list */
  if (! Pdata_Check(self->file)) {
      PyErr_SetString(PicklingError,
                      "Attempt to getvalue() a non-list-based pickler");
      return NULL;
  }

  /* flush write buffer */
  if (write_other(self, NULL, 0) < 0) return NULL;

  data=(Pdata*)self->file;
  l=data->length;

  /* set up an array to hold get/put status */
  if ((lm=PyDict_Size(self->memo)) < 0) return NULL;
  lm++;
  if (! (have_get=malloc((lm)*sizeof(char)))) return PyErr_NoMemory();
  memset(have_get,0,lm);

  /* Scan for gets. */
  for (rsize=0, i=l; --i >= 0; ) {
      k=data->data[i];

      if (PyString_Check(k)) {
        rsize += PyString_GET_SIZE(k);
      }

      else if (PyInt_Check(k)) { /* put */
        ik=PyInt_AS_LONG((PyIntObject*)k);
        if (ik >= lm || ik==0) {
          PyErr_SetString(PicklingError,
                          "Invalid get data");
          return NULL;
        }
        if (have_get[ik]) { /* with matching get */
          if (ik < 256) rsize += 2;
          else rsize+=5;
        }
      }

      else if (! (PyTuple_Check(k) &&
                  PyTuple_GET_SIZE(k) == 2 &&
                  PyInt_Check((k=PyTuple_GET_ITEM(k,0))))
               ) {
        PyErr_SetString(PicklingError,
                        "Unexpected data in internal list");
        return NULL;
      }

      else { /* put */
        ik=PyInt_AS_LONG((PyIntObject*)k);
        if (ik >= lm || ik==0) {
          PyErr_SetString(PicklingError,
                          "Invalid get data");
          return NULL;
        }
        have_get[ik]=1;
        if (ik < 256) rsize += 2;
        else rsize+=5;
      }

    }

  /* Now generate the result */
  if (!( r=PyString_FromStringAndSize(NULL,rsize)))  goto err;
  s=PyString_AS_STRING((PyStringObject*)r);

  for (i=0; i<l; i++) {
      k=data->data[i];

      if (PyString_Check(k)) {
        ssize=PyString_GET_SIZE(k);
        if (ssize) {
            p=PyString_AS_STRING((PyStringObject*)k);
            while (--ssize >= 0) *s++=*p++;
          }
      }

      else if (PyTuple_Check(k)) { /* get */
        ik=PyInt_AS_LONG((PyIntObject*)PyTuple_GET_ITEM(k,0));
        if (ik < 256) {
          *s++ = BINGET;
          *s++ = (int)(ik & 0xff);
        }
        else {
          *s++ = LONG_BINGET;
          *s++ = (int)(ik & 0xff);
          *s++ = (int)((ik >> 8)  & 0xff);
          *s++ = (int)((ik >> 16) & 0xff);
          *s++ = (int)((ik >> 24) & 0xff);
        }
      }

      else { /* put */
        ik=PyInt_AS_LONG((PyIntObject*)k);

        if (have_get[ik]) { /* with matching get */
          if (ik < 256) {
            *s++ = BINPUT;
            *s++ = (int)(ik & 0xff);
          }
          else {
            *s++ = LONG_BINPUT;
            *s++ = (int)(ik & 0xff);
            *s++ = (int)((ik >> 8)  & 0xff);
            *s++ = (int)((ik >> 16) & 0xff);
            *s++ = (int)((ik >> 24) & 0xff);
          }
        }
      }

    }

  if (clear) {
    PyDict_Clear(self->memo);
    Pdata_clear(data,0);
  }

  free(have_get);
  return r;
err:
  free(have_get);
  return NULL;
}

static PyObject *
Pickler_dump(Picklerobject *self, PyObject *args) {
    PyObject *ob;
    int get=0;

    if (!( PyArg_ParseTuple(args, "O|i:dump", &ob, &get))) 
        return NULL;

    if (dump(self, ob) < 0)
        return NULL;

    if (get) return Pickle_getvalue(self, NULL);

    /* XXX Why does dump() return self? */
    Py_INCREF(self);
    return (PyObject*)self;
}


static struct PyMethodDef Pickler_methods[] = {
  {"dump",          (PyCFunction)Pickler_dump,  METH_VARARGS,
   "dump(object) --"
   "Write an object in pickle format to the object's pickle stream\n"
  },
  {"clear_memo",  (PyCFunction)Pickle_clear_memo,  METH_VARARGS,
   "clear_memo() -- Clear the picklers memo"},
  {"getvalue",  (PyCFunction)Pickle_getvalue,  METH_VARARGS,
   "getvalue() -- Finish picking a list-based pickle"},
  {NULL,                NULL}           /* sentinel */
};


static Picklerobject *
newPicklerobject(PyObject *file, int bin) {
    Picklerobject *self;

    if (!( self = PyObject_New(Picklerobject, &Picklertype))) 
        return NULL;

    self->fp = NULL;
    self->write = NULL;
    self->memo = NULL;
    self->arg = NULL;
    self->pers_func = NULL;
    self->inst_pers_func = NULL;
    self->write_buf = NULL;
    self->bin = bin;
    self->fast = 0;
    self->fast_container = 0;
    self->fast_memo = NULL;
    self->buf_size = 0;
    self->dispatch_table = NULL;

    if (file)
      Py_INCREF(file);
    else
      file=Pdata_New();

    if (!( self->file = file )) 
      goto err;

    if (!( self->memo = PyDict_New())) 
       goto err;

    if (PyFile_Check(file)) {
        self->fp = PyFile_AsFile(file);
	if (self->fp == NULL) {
	    PyErr_SetString(PyExc_ValueError, "I/O operation on closed file");
	    goto err;
	}
        self->write_func = write_file;
    }
    else if (PycStringIO_OutputCheck(file)) {
        self->write_func = write_cStringIO;
    }
    else if (file == Py_None) {
        self->write_func = write_none;
    }
    else {
        self->write_func = write_other;

        if (! Pdata_Check(file)) {
          if (!( self->write = PyObject_GetAttr(file, write_str)))  {
            PyErr_Clear();
            PyErr_SetString(PyExc_TypeError, "argument must have 'write' "
                            "attribute");
            goto err;
          }
        }

        if (!( self->write_buf =
            (char *)malloc(WRITE_BUF_SIZE * sizeof(char))))  {
            PyErr_NoMemory();
            goto err;
        }
    }

    if (PyEval_GetRestricted()) {
        /* Restricted execution, get private tables */
        PyObject *m;

        if (!( m=PyImport_Import(copy_reg_str)))  goto err;
        self->dispatch_table=PyObject_GetAttr(m, dispatch_table_str);
        Py_DECREF(m);
        if (!( self->dispatch_table ))  goto err;
    }
    else {
        self->dispatch_table=dispatch_table;
        Py_INCREF(dispatch_table);
    }

    return self;

err:
    Py_DECREF((PyObject *)self);
    return NULL;
}


static PyObject *
get_Pickler(PyObject *self, PyObject *args) {
    PyObject *file = NULL;
    int bin = 1;

    if (!PyArg_ParseTuple(args, "|i:Pickler", &bin)) {
        PyErr_Clear();
        bin = 0;
        if (!PyArg_ParseTuple(args, "O|i:Pickler", &file, &bin))
	    return NULL;
    }
    return (PyObject *)newPicklerobject(file, bin);
}


static void
Pickler_dealloc(Picklerobject *self) {
    Py_XDECREF(self->write);
    Py_XDECREF(self->memo);
    Py_XDECREF(self->fast_memo);
    Py_XDECREF(self->arg);
    Py_XDECREF(self->file);
    Py_XDECREF(self->pers_func);
    Py_XDECREF(self->inst_pers_func);
    Py_XDECREF(self->dispatch_table);

    if (self->write_buf) {
        free(self->write_buf);
    }

    PyObject_Del(self);
}

static PyObject *
Pickler_get_pers_func(Picklerobject *p)
{
    if (p->pers_func == NULL)
	PyErr_SetString(PyExc_AttributeError, "persistent_id");
    else
	Py_INCREF(p->pers_func);
    return p->pers_func;
}

static int
Pickler_set_pers_func(Picklerobject *p, PyObject *v)
{
    if (v == NULL) {
        PyErr_SetString(PyExc_TypeError,
			"attribute deletion is not supported");
        return -1;
    }
    Py_XDECREF(p->pers_func);
    Py_INCREF(v);
    p->pers_func = v;
    return 0;
}

static int
Pickler_set_inst_pers_func(Picklerobject *p, PyObject *v)
{
    if (v == NULL) {
        PyErr_SetString(PyExc_TypeError,
			"attribute deletion is not supported");
        return -1;
    }
    Py_XDECREF(p->inst_pers_func);
    Py_INCREF(v);
    p->inst_pers_func = v;
    return 0;
}

static PyObject *
Pickler_get_memo(Picklerobject *p)
{
    if (p->memo == NULL)
	PyErr_SetString(PyExc_AttributeError, "memo");
    else
	Py_INCREF(p->memo);
    return p->memo;
}

static int
Pickler_set_memo(Picklerobject *p, PyObject *v)
{
    if (v == NULL) {
        PyErr_SetString(PyExc_TypeError,
			"attribute deletion is not supported");
        return -1;
    }
    if (!PyDict_Check(v)) {
	PyErr_SetString(PyExc_TypeError, "memo must be a dictionary");
	return -1;
    }
    Py_XDECREF(p->memo);
    Py_INCREF(v);
    p->memo = v;
    return 0;
}

static PyObject *
Pickler_get_error(Picklerobject *p)
{
    /* why is this an attribute on the Pickler? */
    Py_INCREF(PicklingError);
    return PicklingError;
}

static PyMemberDef Pickler_members[] = {
    {"binary", T_INT, offsetof(Picklerobject, bin)},
    {"fast", T_INT, offsetof(Picklerobject, fast)},
    {NULL}
};

static PyGetSetDef Pickler_getsets[] = {
    {"persistent_id", (getter)Pickler_get_pers_func, 
                     (setter)Pickler_set_pers_func},
    {"inst_persistent_id", NULL, (setter)Pickler_set_inst_pers_func},
    {"memo", (getter)Pickler_get_memo, (setter)Pickler_set_memo},
    {"PicklingError", (getter)Pickler_get_error, NULL},
    {NULL}
};

static char Picklertype__doc__[] =
"Objects that know how to pickle objects\n";

static PyTypeObject Picklertype = {
    PyObject_HEAD_INIT(NULL)
    0,                            /*ob_size*/
    "cPickle.Pickler",            /*tp_name*/
    sizeof(Picklerobject),              /*tp_basicsize*/
    0,
    (destructor)Pickler_dealloc,	/* tp_dealloc */
    0,					/* tp_print */
    0,			 		/* tp_getattr */
    0,			 		/* tp_setattr */
    0,					/* tp_compare */
    0,		 			/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    0,					/* tp_hash */
    0,					/* tp_call */
    0,					/* tp_str */
    0,	/* set below */			/* tp_getattro */
    0,	/* set below */			/* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    Picklertype__doc__,			/* tp_doc */
    0,					/* tp_traverse */
    0,					/* tp_clear */
    0,					/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    0,					/* tp_iter */
    0,					/* tp_iternext */
    Pickler_methods,			/* tp_methods */
    Pickler_members,			/* tp_members */
    Pickler_getsets,			/* tp_getset */
};

static PyObject *
find_class(PyObject *py_module_name, PyObject *py_global_name, PyObject *fc) {
    PyObject *global = 0, *module;

    if (fc) {
      if (fc==Py_None) {
	PyErr_SetString(UnpicklingError,
			"Global and instance pickles are not supported.");
	return NULL;
      }
      return PyObject_CallFunction(fc, "OO", py_module_name, py_global_name);
    }

    module = PySys_GetObject("modules");
    if (module == NULL)
      return NULL;

    module = PyDict_GetItem(module, py_module_name);
    if (module == NULL) {
      module = PyImport_Import(py_module_name);
      if (!module)
          return NULL;
      global = PyObject_GetAttr(module, py_global_name);
      Py_DECREF(module);
    }
    else
      global = PyObject_GetAttr(module, py_global_name);
    return global;
}

static int
marker(Unpicklerobject *self) {
    if (self->num_marks < 1) {
        PyErr_SetString(UnpicklingError, "could not find MARK");
        return -1;
    }

    return self->marks[--self->num_marks];
}


static int
load_none(Unpicklerobject *self) {
    PDATA_APPEND(self->stack, Py_None, -1);
    return 0;
}

static int
bad_readline(void) {
    PyErr_SetString(UnpicklingError, "pickle data was truncated");
    return -1;
}

static int
load_int(Unpicklerobject *self) {
    PyObject *py_int = 0;
    char *endptr, *s;
    int len, res = -1;
    long l;

    if ((len = (*self->readline_func)(self, &s)) < 0) return -1;
    if (len < 2) return bad_readline();
    if (!( s=pystrndup(s,len)))  return -1;

    errno = 0;
    l = strtol(s, &endptr, 0);

    if (errno || (*endptr != '\n') || (endptr[1] != '\0')) {
        /* Hm, maybe we've got something long.  Let's try reading
           it as a Python long object. */
        errno = 0;
        py_int = PyLong_FromString(s, NULL, 0);
        if (py_int == NULL) {
            PyErr_SetString(PyExc_ValueError,
                            "could not convert string to int");
            goto finally;
        }
    }
    else {
        if (!( py_int = PyInt_FromLong(l)))  goto finally;
    }

    free(s);
    PDATA_PUSH(self->stack, py_int, -1);
    return 0;

finally:
    free(s);

    return res;
}


static long
calc_binint(char *s, int  x) {
    unsigned char c;
    int i;
    long l;

    for (i = 0, l = 0L; i < x; i++) {
        c = (unsigned char)s[i];
        l |= (long)c << (i * 8);
    }
#if SIZEOF_LONG > 4
    /* Unlike BININT1 and BININT2, BININT (more accurately BININT4)
     * is signed, so on a box with longs bigger than 4 bytes we need
     * to extend a BININT's sign bit to the full width.
     */
    if (x == 4 && l & (1L << 31))
        l |= (~0L) << 32;
#endif
    return l;
}


static int
load_binintx(Unpicklerobject *self, char *s, int  x) {
    PyObject *py_int = 0;
    long l;

    l = calc_binint(s, x);

    if (!( py_int = PyInt_FromLong(l))) 
        return -1;

    PDATA_PUSH(self->stack, py_int, -1);
    return 0;
}


static int
load_binint(Unpicklerobject *self) {
    char *s;

    if ((*self->read_func)(self, &s, 4) < 0)
        return -1;

    return load_binintx(self, s, 4);
}


static int
load_binint1(Unpicklerobject *self) {
    char *s;

    if ((*self->read_func)(self, &s, 1) < 0)
        return -1;

    return load_binintx(self, s, 1);
}


static int
load_binint2(Unpicklerobject *self) {
    char *s;

    if ((*self->read_func)(self, &s, 2) < 0)
        return -1;

    return load_binintx(self, s, 2);
}

static int
load_long(Unpicklerobject *self) {
    PyObject *l = 0;
    char *end, *s;
    int len, res = -1;

    if ((len = (*self->readline_func)(self, &s)) < 0) return -1;
    if (len < 2) return bad_readline();
    if (!( s=pystrndup(s,len)))  return -1;

    if (!( l = PyLong_FromString(s, &end, 0))) 
        goto finally;

    free(s);
    PDATA_PUSH(self->stack, l, -1);
    return 0;

finally:
    free(s);

    return res;
}


static int
load_float(Unpicklerobject *self) {
    PyObject *py_float = 0;
    char *endptr, *s;
    int len, res = -1;
    double d;

    if ((len = (*self->readline_func)(self, &s)) < 0) return -1;
    if (len < 2) return bad_readline();
    if (!( s=pystrndup(s,len)))  return -1;

    errno = 0;
    d = strtod(s, &endptr);

    if (errno || (endptr[0] != '\n') || (endptr[1] != '\0')) {
        PyErr_SetString(PyExc_ValueError,
        "could not convert string to float");
        goto finally;
    }

    if (!( py_float = PyFloat_FromDouble(d))) 
        goto finally;

    free(s);
    PDATA_PUSH(self->stack, py_float, -1);
    return 0;

finally:
    free(s);

    return res;
}

static int
load_binfloat(Unpicklerobject *self) {
    PyObject *py_float = 0;
    int s, e;
    long fhi, flo;
    double x;
    char *p;

    if ((*self->read_func)(self, &p, 8) < 0)
        return -1;

    /* First byte */
    s = (*p>>7) & 1;
    e = (*p & 0x7F) << 4;
    p++;

    /* Second byte */
    e |= (*p>>4) & 0xF;
    fhi = (*p & 0xF) << 24;
    p++;

    /* Third byte */
    fhi |= (*p & 0xFF) << 16;
    p++;

    /* Fourth byte */
    fhi |= (*p & 0xFF) << 8;
    p++;

    /* Fifth byte */
    fhi |= *p & 0xFF;
    p++;

    /* Sixth byte */
    flo = (*p & 0xFF) << 16;
    p++;

    /* Seventh byte */
    flo |= (*p & 0xFF) << 8;
    p++;

    /* Eighth byte */
    flo |= *p & 0xFF;

    x = (double)fhi + (double)flo / 16777216.0; /* 2**24 */
    x /= 268435456.0; /* 2**28 */

    /* XXX This sadly ignores Inf/NaN */
    if (e == 0)
        e = -1022;
    else {
        x += 1.0;
        e -= 1023;
    }
    x = ldexp(x, e);

    if (s)
        x = -x;

    if (!( py_float = PyFloat_FromDouble(x)))  return -1;

    PDATA_PUSH(self->stack, py_float, -1);
    return 0;
}

static int
load_string(Unpicklerobject *self) {
    PyObject *str = 0;
    int len, res = -1, nslash;
    char *s, q, *p;

    static PyObject *eval_dict = 0;

    if ((len = (*self->readline_func)(self, &s)) < 0) return -1;
    if (len < 2) return bad_readline();
    if (!( s=pystrndup(s,len)))  return -1;

    /* Check for unquoted quotes (evil strings) */
    q=*s;
    if (q != '"' && q != '\'') goto insecure;
    for (p=s+1, nslash=0; *p; p++) {
        if (*p==q && nslash%2==0) break;
        if (*p=='\\') nslash++;
        else nslash=0;
    }
    if (*p == q) {
        for (p++; *p; p++)
            if (*(unsigned char *)p > ' ')
                goto insecure;
    }
    else
        goto insecure;
    /********************************************/

    if (!( eval_dict )) 
        if (!( eval_dict = Py_BuildValue("{s{}}", "__builtins__"))) 
            goto finally;

    if (!( str = PyRun_String(s, Py_eval_input, eval_dict, eval_dict))) 
        goto finally;

    free(s);
    PDATA_PUSH(self->stack, str, -1);
    return 0;

finally:
    free(s);

    return res;

insecure:
    free(s);
    PyErr_SetString(PyExc_ValueError,"insecure string pickle");
    return -1;
}


static int
load_binstring(Unpicklerobject *self) {
    PyObject *py_string = 0;
    long l;
    char *s;

    if ((*self->read_func)(self, &s, 4) < 0) return -1;

    l = calc_binint(s, 4);

    if ((*self->read_func)(self, &s, l) < 0)
        return -1;

    if (!( py_string = PyString_FromStringAndSize(s, l))) 
        return -1;

    PDATA_PUSH(self->stack, py_string, -1);
    return 0;
}


static int
load_short_binstring(Unpicklerobject *self) {
    PyObject *py_string = 0;
    unsigned char l;
    char *s;

    if ((*self->read_func)(self, &s, 1) < 0)
        return -1;

    l = (unsigned char)s[0];

    if ((*self->read_func)(self, &s, l) < 0) return -1;

    if (!( py_string = PyString_FromStringAndSize(s, l)))  return -1;

    PDATA_PUSH(self->stack, py_string, -1);
    return 0;
}


#ifdef Py_USING_UNICODE
static int
load_unicode(Unpicklerobject *self) {
    PyObject *str = 0;
    int len, res = -1;
    char *s;

    if ((len = (*self->readline_func)(self, &s)) < 0) return -1;
    if (len < 1) return bad_readline();

    if (!( str = PyUnicode_DecodeRawUnicodeEscape(s, len - 1, NULL))) 
        goto finally;

    PDATA_PUSH(self->stack, str, -1);
    return 0;

finally:
    return res;
}
#endif


#ifdef Py_USING_UNICODE
static int
load_binunicode(Unpicklerobject *self) {
    PyObject *unicode;
    long l;
    char *s;

    if ((*self->read_func)(self, &s, 4) < 0) return -1;

    l = calc_binint(s, 4);

    if ((*self->read_func)(self, &s, l) < 0)
        return -1;

    if (!( unicode = PyUnicode_DecodeUTF8(s, l, NULL))) 
        return -1;

    PDATA_PUSH(self->stack, unicode, -1);
    return 0;
}
#endif


static int
load_tuple(Unpicklerobject *self) {
    PyObject *tup;
    int i;

    if ((i = marker(self)) < 0) return -1;
    if (!( tup=Pdata_popTuple(self->stack, i)))  return -1;
    PDATA_PUSH(self->stack, tup, -1);
    return 0;
}

static int
load_empty_tuple(Unpicklerobject *self) {
    PyObject *tup;

    if (!( tup=PyTuple_New(0)))  return -1;
    PDATA_PUSH(self->stack, tup, -1);
    return 0;
}

static int
load_empty_list(Unpicklerobject *self) {
    PyObject *list;

    if (!( list=PyList_New(0)))  return -1;
    PDATA_PUSH(self->stack, list, -1);
    return 0;
}

static int
load_empty_dict(Unpicklerobject *self) {
    PyObject *dict;

    if (!( dict=PyDict_New()))  return -1;
    PDATA_PUSH(self->stack, dict, -1);
    return 0;
}


static int
load_list(Unpicklerobject *self) {
    PyObject *list = 0;
    int i;

    if ((i = marker(self)) < 0) return -1;
    if (!( list=Pdata_popList(self->stack, i)))  return -1;
    PDATA_PUSH(self->stack, list, -1);
    return 0;
}

static int
load_dict(Unpicklerobject *self) {
    PyObject *dict, *key, *value;
    int i, j, k;

    if ((i = marker(self)) < 0) return -1;
    j=self->stack->length;

    if (!( dict = PyDict_New()))  return -1;

    for (k = i+1; k < j; k += 2) {
        key  =self->stack->data[k-1];
        value=self->stack->data[k  ];
        if (PyDict_SetItem(dict, key, value) < 0) {
            Py_DECREF(dict);
            return -1;
        }
    }
    Pdata_clear(self->stack, i);
    PDATA_PUSH(self->stack, dict, -1);
    return 0;
}

static PyObject *
Instance_New(PyObject *cls, PyObject *args) {
  int has_key;
  PyObject *safe=0, *r=0;

  if (PyClass_Check(cls)) {
      int l;

      if ((l=PyObject_Size(args)) < 0) goto err;
      if (!( l ))  {
          PyObject *__getinitargs__;

          if (!( __getinitargs__=PyObject_GetAttr(cls, __getinitargs___str)))  {
              /* We have a class with no __getinitargs__, so bypass usual
                 construction  */
              PyObject *inst;

              PyErr_Clear();
              if (!( inst=PyInstance_NewRaw(cls, NULL))) 
                goto err;
              return inst;
            }
          Py_DECREF(__getinitargs__);
        }

      if ((r=PyInstance_New(cls, args, NULL))) return r;
      else goto err;
    }

  /* Is safe_constructors always a dict? */
  has_key = cPickle_PyMapping_HasKey(safe_constructors, cls);
  if (!has_key)
    if (!(safe = PyObject_GetAttr(cls, __safe_for_unpickling___str)) ||
       !PyObject_IsTrue(safe)) {
      cPickle_ErrFormat(UnpicklingError,
                        "%s is not safe for unpickling", "O", cls);
      Py_XDECREF(safe);
      return NULL;
  }

  if (args==Py_None) {
      /* Special case, call cls.__basicnew__() */
      PyObject *basicnew;

      if (!( basicnew=PyObject_GetAttr(cls, __basicnew___str)))  return NULL;
      r=PyObject_CallObject(basicnew, NULL);
      Py_DECREF(basicnew);
      if (r) return r;
    }

  if ((r=PyObject_CallObject(cls, args))) return r;

err:
  {
    PyObject *tp, *v, *tb;

    PyErr_Fetch(&tp, &v, &tb);
    if ((r=Py_BuildValue("OOO",v,cls,args))) {
        Py_XDECREF(v);
        v=r;
      }
    PyErr_Restore(tp,v,tb);
  }
  return NULL;
}


static int
load_obj(Unpicklerobject *self) {
    PyObject *class, *tup, *obj=0;
    int i;

    if ((i = marker(self)) < 0) return -1;
    if (!( tup=Pdata_popTuple(self->stack, i+1)))  return -1;
    PDATA_POP(self->stack, class);
    if (class) {
        obj = Instance_New(class, tup);
        Py_DECREF(class);
    }
    Py_DECREF(tup);

    if (! obj) return -1;
    PDATA_PUSH(self->stack, obj, -1);
    return 0;
}


static int
load_inst(Unpicklerobject *self) {
    PyObject *tup, *class=0, *obj=0, *module_name, *class_name;
    int i, len;
    char *s;

    if ((i = marker(self)) < 0) return -1;

    if ((len = (*self->readline_func)(self, &s)) < 0) return -1;
    if (len < 2) return bad_readline();
    if (!( module_name = PyString_FromStringAndSize(s, len - 1)))  return -1;

    if ((len = (*self->readline_func)(self, &s)) >= 0) {
        if (len < 2) return bad_readline();
        if ((class_name = PyString_FromStringAndSize(s, len - 1))) {
            class = find_class(module_name, class_name, self->find_class);
            Py_DECREF(class_name);
        }
    }
    Py_DECREF(module_name);

    if (! class) return -1;

    if ((tup=Pdata_popTuple(self->stack, i))) {
        obj = Instance_New(class, tup);
        Py_DECREF(tup);
    }
    Py_DECREF(class);

    if (! obj) return -1;

    PDATA_PUSH(self->stack, obj, -1);
    return 0;
}


static int
load_global(Unpicklerobject *self) {
    PyObject *class = 0, *module_name = 0, *class_name = 0;
    int len;
    char *s;

    if ((len = (*self->readline_func)(self, &s)) < 0) return -1;
    if (len < 2) return bad_readline();
    if (!( module_name = PyString_FromStringAndSize(s, len - 1)))  return -1;

    if ((len = (*self->readline_func)(self, &s)) >= 0) {
        if (len < 2) return bad_readline();
        if ((class_name = PyString_FromStringAndSize(s, len - 1))) {
            class = find_class(module_name, class_name, self->find_class);
            Py_DECREF(class_name);
        }
    }
    Py_DECREF(module_name);

    if (! class) return -1;
    PDATA_PUSH(self->stack, class, -1);
    return 0;
}


static int
load_persid(Unpicklerobject *self) {
    PyObject *pid = 0;
    int len;
    char *s;

    if (self->pers_func) {
        if ((len = (*self->readline_func)(self, &s)) < 0) return -1;
	if (len < 2) return bad_readline();

        if (!( pid = PyString_FromStringAndSize(s, len - 1)))  return -1;

        if (PyList_Check(self->pers_func)) {
            if (PyList_Append(self->pers_func, pid) < 0) {
                Py_DECREF(pid);
                return -1;
            }
        }
        else {
            ARG_TUP(self, pid);
            if (self->arg) {
                pid = PyObject_Call(self->pers_func, self->arg, NULL);
                FREE_ARG_TUP(self);
            }
        }

        if (! pid) return -1;

        PDATA_PUSH(self->stack, pid, -1);
        return 0;
    }
    else {
      PyErr_SetString(UnpicklingError,
                      "A load persistent id instruction was encountered,\n"
                      "but no persistent_load function was specified.");
      return -1;
    }
}

static int
load_binpersid(Unpicklerobject *self) {
    PyObject *pid = 0;

    if (self->pers_func) {
        PDATA_POP(self->stack, pid);
        if (! pid) return -1;

        if (PyList_Check(self->pers_func)) {
            if (PyList_Append(self->pers_func, pid) < 0) {
                Py_DECREF(pid);
                return -1;
            }
          }
        else {
            ARG_TUP(self, pid);
            if (self->arg) {
                pid = PyObject_Call(self->pers_func, self->arg, NULL);
                FREE_ARG_TUP(self);
            }
            if (! pid) return -1;
        }

        PDATA_PUSH(self->stack, pid, -1);
        return 0;
    }
    else {
      PyErr_SetString(UnpicklingError,
                      "A load persistent id instruction was encountered,\n"
                      "but no persistent_load function was specified.");
      return -1;
    }
}


static int
load_pop(Unpicklerobject *self) {
    int len;

    if (!( (len=self->stack->length) > 0 ))  return stackUnderflow();

    /* Note that we split the (pickle.py) stack into two stacks,
       an object stack and a mark stack. We have to be clever and
       pop the right one. We do this by looking at the top of the
       mark stack.
    */

    if ((self->num_marks > 0) &&
        (self->marks[self->num_marks - 1] == len))
        self->num_marks--;
    else {
        len--;
        Py_DECREF(self->stack->data[len]);
	self->stack->length=len;
    }

    return 0;
}


static int
load_pop_mark(Unpicklerobject *self) {
    int i;

    if ((i = marker(self)) < 0)
        return -1;

    Pdata_clear(self->stack, i);

    return 0;
}


static int
load_dup(Unpicklerobject *self) {
    PyObject *last;
    int len;

    if ((len = self->stack->length) <= 0) return stackUnderflow();
    last=self->stack->data[len-1];
    Py_INCREF(last);
    PDATA_PUSH(self->stack, last, -1);
    return 0;
}


static int
load_get(Unpicklerobject *self) {
    PyObject *py_str = 0, *value = 0;
    int len;
    char *s;
    int rc;

    if ((len = (*self->readline_func)(self, &s)) < 0) return -1;
    if (len < 2) return bad_readline();

    if (!( py_str = PyString_FromStringAndSize(s, len - 1)))  return -1;

    value = PyDict_GetItem(self->memo, py_str);
    if (! value) {
        PyErr_SetObject(BadPickleGet, py_str);
        rc = -1;
    } else {
      PDATA_APPEND(self->stack, value, -1);
      rc = 0;
    }

    Py_DECREF(py_str);
    return rc;
}


static int
load_binget(Unpicklerobject *self) {
    PyObject *py_key = 0, *value = 0;
    unsigned char key;
    char *s;
    int rc;

    if ((*self->read_func)(self, &s, 1) < 0) return -1;

    key = (unsigned char)s[0];
    if (!( py_key = PyInt_FromLong((long)key)))  return -1;

    value = PyDict_GetItem(self->memo, py_key);
    if (! value) {
        PyErr_SetObject(BadPickleGet, py_key);
        rc = -1;
    } else {
      PDATA_APPEND(self->stack, value, -1);
      rc = 0;
    }

    Py_DECREF(py_key);
    return rc;
}


static int
load_long_binget(Unpicklerobject *self) {
    PyObject *py_key = 0, *value = 0;
    unsigned char c;
    char *s;
    long key;
    int rc;

    if ((*self->read_func)(self, &s, 4) < 0) return -1;

    c = (unsigned char)s[0];
    key = (long)c;
    c = (unsigned char)s[1];
    key |= (long)c << 8;
    c = (unsigned char)s[2];
    key |= (long)c << 16;
    c = (unsigned char)s[3];
    key |= (long)c << 24;

    if (!( py_key = PyInt_FromLong((long)key)))  return -1;

    value = PyDict_GetItem(self->memo, py_key);
    if (! value) {
        PyErr_SetObject(BadPickleGet, py_key);
        rc = -1;
    } else {
      PDATA_APPEND(self->stack, value, -1);
      rc = 0;
    }

    Py_DECREF(py_key);
    return rc;
}


static int
load_put(Unpicklerobject *self) {
    PyObject *py_str = 0, *value = 0;
    int len, l;
    char *s;

    if ((l = (*self->readline_func)(self, &s)) < 0) return -1;
    if (l < 2) return bad_readline();
    if (!( len=self->stack->length ))  return stackUnderflow();
    if (!( py_str = PyString_FromStringAndSize(s, l - 1)))  return -1;
    value=self->stack->data[len-1];
    l=PyDict_SetItem(self->memo, py_str, value);
    Py_DECREF(py_str);
    return l;
}


static int
load_binput(Unpicklerobject *self) {
    PyObject *py_key = 0, *value = 0;
    unsigned char key;
    char *s;
    int len;

    if ((*self->read_func)(self, &s, 1) < 0) return -1;
    if (!( (len=self->stack->length) > 0 ))  return stackUnderflow();

    key = (unsigned char)s[0];

    if (!( py_key = PyInt_FromLong((long)key)))  return -1;
    value=self->stack->data[len-1];
    len=PyDict_SetItem(self->memo, py_key, value);
    Py_DECREF(py_key);
    return len;
}


static int
load_long_binput(Unpicklerobject *self) {
    PyObject *py_key = 0, *value = 0;
    long key;
    unsigned char c;
    char *s;
    int len;

    if ((*self->read_func)(self, &s, 4) < 0) return -1;
    if (!( len=self->stack->length ))  return stackUnderflow();

    c = (unsigned char)s[0];
    key = (long)c;
    c = (unsigned char)s[1];
    key |= (long)c << 8;
    c = (unsigned char)s[2];
    key |= (long)c << 16;
    c = (unsigned char)s[3];
    key |= (long)c << 24;

    if (!( py_key = PyInt_FromLong(key)))  return -1;
    value=self->stack->data[len-1];
    len=PyDict_SetItem(self->memo, py_key, value);
    Py_DECREF(py_key);
    return len;
}


static int
do_append(Unpicklerobject *self, int  x) {
    PyObject *value = 0, *list = 0, *append_method = 0;
    int len, i;

    if (!( (len=self->stack->length) >= x && x > 0 ))  return stackUnderflow();
    if (len==x) return 0;       /* nothing to do */

    list=self->stack->data[x-1];

    if (PyList_Check(list)) {
        PyObject *slice;
        int list_len;

        slice=Pdata_popList(self->stack, x);
        list_len = PyList_GET_SIZE(list);
        i=PyList_SetSlice(list, list_len, list_len, slice);
        Py_DECREF(slice);
        return i;
    }
    else {

        if (!( append_method = PyObject_GetAttr(list, append_str))) 
            return -1;

        for (i = x; i < len; i++) {
            PyObject *junk;

            value=self->stack->data[i];
            junk=0;
            ARG_TUP(self, value);
            if (self->arg) {
              junk = PyObject_Call(append_method, self->arg, NULL);
              FREE_ARG_TUP(self);
            }
            if (! junk) {
                Pdata_clear(self->stack, i+1);
                self->stack->length=x;
                Py_DECREF(append_method);
                return -1;
            }
            Py_DECREF(junk);
        }
        self->stack->length=x;
        Py_DECREF(append_method);
    }

    return 0;
}


static int
load_append(Unpicklerobject *self) {
    return do_append(self, self->stack->length - 1);
}


static int
load_appends(Unpicklerobject *self) {
    return do_append(self, marker(self));
}


static int
do_setitems(Unpicklerobject *self, int  x) {
    PyObject *value = 0, *key = 0, *dict = 0;
    int len, i, r=0;

    if (!( (len=self->stack->length) >= x
            && x > 0 ))  return stackUnderflow();

    dict=self->stack->data[x-1];

    for (i = x+1; i < len; i += 2) {
        key  =self->stack->data[i-1];
        value=self->stack->data[i  ];
        if (PyObject_SetItem(dict, key, value) < 0) {
            r=-1;
            break;
        }
    }

    Pdata_clear(self->stack, x);

    return r;
}


static int
load_setitem(Unpicklerobject *self) {
    return do_setitems(self, self->stack->length - 2);
}

static int
load_setitems(Unpicklerobject *self) {
    return do_setitems(self, marker(self));
}


static int
load_build(Unpicklerobject *self) {
    PyObject *value = 0, *inst = 0, *instdict = 0, *d_key = 0, *d_value = 0,
             *junk = 0, *__setstate__ = 0;
    int i, r = 0;

    if (self->stack->length < 2) return stackUnderflow();
    PDATA_POP(self->stack, value);
    if (! value) return -1;
    inst=self->stack->data[self->stack->length-1];

    if ((__setstate__ = PyObject_GetAttr(inst, __setstate___str))) {
        ARG_TUP(self, value);
        if (self->arg) {
            junk = PyObject_Call(__setstate__, self->arg, NULL);
            FREE_ARG_TUP(self);
        }
        Py_DECREF(__setstate__);
        if (! junk) return -1;
        Py_DECREF(junk);
        return 0;
    }

    PyErr_Clear();
    if ((instdict = PyObject_GetAttr(inst, __dict___str))) {
        i = 0;
        while (PyDict_Next(value, &i, &d_key, &d_value)) {
            if (PyObject_SetItem(instdict, d_key, d_value) < 0) {
                r=-1;
                break;
            }
        }
        Py_DECREF(instdict);
    }
    else r=-1;

    Py_XDECREF(value);

    return r;
}


static int
load_mark(Unpicklerobject *self) {
    int s;

    /* Note that we split the (pickle.py) stack into two stacks, an
       object stack and a mark stack. Here we push a mark onto the
       mark stack.
    */

    if ((self->num_marks + 1) >= self->marks_size) {
        s=self->marks_size+20;
        if (s <= self->num_marks) s=self->num_marks + 1;
        if (self->marks == NULL)
            self->marks=(int *)malloc(s * sizeof(int));
        else
            self->marks=(int *)realloc(self->marks, s * sizeof(int));
        if (! self->marks) {
            PyErr_NoMemory();
            return -1;
        }
        self->marks_size = s;
    }

    self->marks[self->num_marks++] = self->stack->length;

    return 0;
}

static int
load_reduce(Unpicklerobject *self) {
    PyObject *callable = 0, *arg_tup = 0, *ob = 0;

    PDATA_POP(self->stack, arg_tup);
    if (! arg_tup) return -1;
    PDATA_POP(self->stack, callable);
    if (callable) {
        ob = Instance_New(callable, arg_tup);
        Py_DECREF(callable);
    }
    Py_DECREF(arg_tup);

    if (! ob) return -1;

    PDATA_PUSH(self->stack, ob, -1);
    return 0;
}

static PyObject *
load(Unpicklerobject *self) {
    PyObject *err = 0, *val = 0;
    char *s;

    self->num_marks = 0;
    if (self->stack->length) Pdata_clear(self->stack, 0);

    while (1) {
        if ((*self->read_func)(self, &s, 1) < 0)
            break;

        switch (s[0]) {
            case NONE:
                if (load_none(self) < 0)
                    break;
                continue;

            case BININT:
                 if (load_binint(self) < 0)
                     break;
                 continue;

            case BININT1:
                if (load_binint1(self) < 0)
                    break;
                continue;

            case BININT2:
                if (load_binint2(self) < 0)
                    break;
                continue;

            case INT:
                if (load_int(self) < 0)
                    break;
                continue;

            case LONG:
                if (load_long(self) < 0)
                    break;
                continue;

            case FLOAT:
                if (load_float(self) < 0)
                    break;
                continue;

            case BINFLOAT:
                if (load_binfloat(self) < 0)
                    break;
                continue;

            case BINSTRING:
                if (load_binstring(self) < 0)
                    break;
                continue;

            case SHORT_BINSTRING:
                if (load_short_binstring(self) < 0)
                    break;
                continue;

            case STRING:
                if (load_string(self) < 0)
                    break;
                continue;

#ifdef Py_USING_UNICODE
            case UNICODE:
                if (load_unicode(self) < 0)
                    break;
                continue;

            case BINUNICODE:
                if (load_binunicode(self) < 0)
                    break;
                continue;
#endif

            case EMPTY_TUPLE:
                if (load_empty_tuple(self) < 0)
                    break;
                continue;

            case TUPLE:
                if (load_tuple(self) < 0)
                    break;
                continue;

            case EMPTY_LIST:
                if (load_empty_list(self) < 0)
                    break;
                continue;

            case LIST:
                if (load_list(self) < 0)
                    break;
                continue;

            case EMPTY_DICT:
                if (load_empty_dict(self) < 0)
                    break;
                continue;

            case DICT:
                if (load_dict(self) < 0)
                    break;
                continue;

            case OBJ:
                if (load_obj(self) < 0)
                    break;
                continue;

            case INST:
                if (load_inst(self) < 0)
                    break;
                continue;

            case GLOBAL:
                if (load_global(self) < 0)
                    break;
                continue;

            case APPEND:
                if (load_append(self) < 0)
                    break;
                continue;

            case APPENDS:
                if (load_appends(self) < 0)
                    break;
                continue;

            case BUILD:
                if (load_build(self) < 0)
                    break;
                continue;

            case DUP:
                if (load_dup(self) < 0)
                    break;
                continue;

            case BINGET:
                if (load_binget(self) < 0)
                    break;
                continue;

            case LONG_BINGET:
                if (load_long_binget(self) < 0)
                    break;
                continue;

            case GET:
                if (load_get(self) < 0)
                    break;
                continue;

            case MARK:
                if (load_mark(self) < 0)
                    break;
                continue;

            case BINPUT:
                if (load_binput(self) < 0)
                    break;
                continue;

            case LONG_BINPUT:
                if (load_long_binput(self) < 0)
                    break;
                continue;

            case PUT:
                if (load_put(self) < 0)
                    break;
                continue;

            case POP:
                if (load_pop(self) < 0)
                    break;
                continue;

            case POP_MARK:
                if (load_pop_mark(self) < 0)
                    break;
                continue;

            case SETITEM:
                if (load_setitem(self) < 0)
                    break;
                continue;

            case SETITEMS:
                if (load_setitems(self) < 0)
                    break;
                continue;

            case STOP:
                break;

            case PERSID:
                if (load_persid(self) < 0)
                    break;
                continue;

            case BINPERSID:
                if (load_binpersid(self) < 0)
                    break;
                continue;

            case REDUCE:
                if (load_reduce(self) < 0)
                    break;
                continue;

            case '\0':
                /* end of file */
                PyErr_SetNone(PyExc_EOFError);
                break;
                
            default:
                cPickle_ErrFormat(UnpicklingError, "invalid load key, '%s'.",
                    "c", s[0]);
                return NULL;
        }

        break;
    }

    if ((err = PyErr_Occurred())) {
        if (err == PyExc_EOFError) {
            PyErr_SetNone(PyExc_EOFError);
        }
        return NULL;
    }

    PDATA_POP(self->stack, val);
    return val;
}


/* No-load functions to support noload, which is used to
   find persistent references. */

static int
noload_obj(Unpicklerobject *self) {
    int i;

    if ((i = marker(self)) < 0) return -1;
    return Pdata_clear(self->stack, i+1);
}


static int
noload_inst(Unpicklerobject *self) {
    int i;
    char *s;

    if ((i = marker(self)) < 0) return -1;
    Pdata_clear(self->stack, i);
    if ((*self->readline_func)(self, &s) < 0) return -1;
    if ((*self->readline_func)(self, &s) < 0) return -1;
    PDATA_APPEND(self->stack, Py_None,-1);
    return 0;
}

static int
noload_global(Unpicklerobject *self) {
    char *s;

    if ((*self->readline_func)(self, &s) < 0) return -1;
    if ((*self->readline_func)(self, &s) < 0) return -1;
    PDATA_APPEND(self->stack, Py_None,-1);
    return 0;
}

static int
noload_reduce(Unpicklerobject *self) {

    if (self->stack->length < 2) return stackUnderflow();
    Pdata_clear(self->stack, self->stack->length-2);
    PDATA_APPEND(self->stack, Py_None,-1);
    return 0;
}

static int
noload_build(Unpicklerobject *self) {

  if (self->stack->length < 1) return stackUnderflow();
  Pdata_clear(self->stack, self->stack->length-1);
  return 0;
}


static PyObject *
noload(Unpicklerobject *self) {
    PyObject *err = 0, *val = 0;
    char *s;

    self->num_marks = 0;
    Pdata_clear(self->stack, 0);

    while (1) {
        if ((*self->read_func)(self, &s, 1) < 0)
            break;

        switch (s[0]) {
            case NONE:
                if (load_none(self) < 0)
                    break;
                continue;

            case BININT:
                 if (load_binint(self) < 0)
                     break;
                 continue;

            case BININT1:
                if (load_binint1(self) < 0)
                    break;
                continue;

            case BININT2:
                if (load_binint2(self) < 0)
                    break;
                continue;

            case INT:
                if (load_int(self) < 0)
                    break;
                continue;

            case LONG:
                if (load_long(self) < 0)
                    break;
                continue;

            case FLOAT:
                if (load_float(self) < 0)
                    break;
                continue;

            case BINFLOAT:
                if (load_binfloat(self) < 0)
                    break;
                continue;

            case BINSTRING:
                if (load_binstring(self) < 0)
                    break;
                continue;

            case SHORT_BINSTRING:
                if (load_short_binstring(self) < 0)
                    break;
                continue;

            case STRING:
                if (load_string(self) < 0)
                    break;
                continue;

#ifdef Py_USING_UNICODE
            case UNICODE:
                if (load_unicode(self) < 0)
                    break;
                continue;

            case BINUNICODE:
                if (load_binunicode(self) < 0)
                    break;
                continue;
#endif

            case EMPTY_TUPLE:
                if (load_empty_tuple(self) < 0)
                    break;
                continue;

            case TUPLE:
                if (load_tuple(self) < 0)
                    break;
                continue;

            case EMPTY_LIST:
                if (load_empty_list(self) < 0)
                    break;
                continue;

            case LIST:
                if (load_list(self) < 0)
                    break;
                continue;

            case EMPTY_DICT:
                if (load_empty_dict(self) < 0)
                    break;
                continue;

            case DICT:
                if (load_dict(self) < 0)
                    break;
                continue;

            case OBJ:
                if (noload_obj(self) < 0)
                    break;
                continue;

            case INST:
                if (noload_inst(self) < 0)
                    break;
                continue;

            case GLOBAL:
                if (noload_global(self) < 0)
                    break;
                continue;

            case APPEND:
                if (load_append(self) < 0)
                    break;
                continue;

            case APPENDS:
                if (load_appends(self) < 0)
                    break;
                continue;

            case BUILD:
                if (noload_build(self) < 0)
                    break;
                continue;

            case DUP:
                if (load_dup(self) < 0)
                    break;
                continue;

            case BINGET:
                if (load_binget(self) < 0)
                    break;
                continue;

            case LONG_BINGET:
                if (load_long_binget(self) < 0)
                    break;
                continue;

            case GET:
                if (load_get(self) < 0)
                    break;
                continue;

            case MARK:
                if (load_mark(self) < 0)
                    break;
                continue;

            case BINPUT:
                if (load_binput(self) < 0)
                    break;
                continue;

            case LONG_BINPUT:
                if (load_long_binput(self) < 0)
                    break;
                continue;

            case PUT:
                if (load_put(self) < 0)
                    break;
                continue;

            case POP:
                if (load_pop(self) < 0)
                    break;
                continue;

            case POP_MARK:
                if (load_pop_mark(self) < 0)
                    break;
                continue;

            case SETITEM:
                if (load_setitem(self) < 0)
                    break;
                continue;

            case SETITEMS:
                if (load_setitems(self) < 0)
                    break;
                continue;

            case STOP:
                break;

            case PERSID:
                if (load_persid(self) < 0)
                    break;
                continue;

            case BINPERSID:
                if (load_binpersid(self) < 0)
                    break;
                continue;

            case REDUCE:
                if (noload_reduce(self) < 0)
                    break;
                continue;

            default:
                cPickle_ErrFormat(UnpicklingError, "invalid load key, '%s'.",
                    "c", s[0]);
                return NULL;
        }

        break;
    }

    if ((err = PyErr_Occurred())) {
        if (err == PyExc_EOFError) {
            PyErr_SetNone(PyExc_EOFError);
        }
        return NULL;
    }

    PDATA_POP(self->stack, val);
    return val;
}


static PyObject *
Unpickler_load(Unpicklerobject *self, PyObject *args) {
    if (!( PyArg_ParseTuple(args, ":load"))) 
        return NULL;

    return load(self);
}

static PyObject *
Unpickler_noload(Unpicklerobject *self, PyObject *args) {
    if (!( PyArg_ParseTuple(args, ":noload"))) 
        return NULL;

    return noload(self);
}


static struct PyMethodDef Unpickler_methods[] = {
  {"load",         (PyCFunction)Unpickler_load,   METH_VARARGS,
   "load() -- Load a pickle"
  },
  {"noload",         (PyCFunction)Unpickler_noload,   METH_VARARGS,
   "noload() -- not load a pickle, but go through most of the motions\n"
   "\n"
   "This function can be used to read past a pickle without instantiating\n"
   "any objects or importing any modules.  It can also be used to find all\n"
   "persistent references without instantiating any objects or importing\n"
   "any modules.\n"
  },
  {NULL,              NULL}           /* sentinel */
};


static Unpicklerobject *
newUnpicklerobject(PyObject *f) {
    Unpicklerobject *self;

    if (!( self = PyObject_New(Unpicklerobject, &Unpicklertype))) 
        return NULL;

    self->file = NULL;
    self->arg = NULL;
    self->stack = (Pdata*)Pdata_New();
    self->pers_func = NULL;
    self->last_string = NULL;
    self->marks = NULL;
    self->num_marks = 0;
    self->marks_size = 0;
    self->buf_size = 0;
    self->read = NULL;
    self->readline = NULL;
    self->safe_constructors = NULL;
    self->find_class = NULL;

    if (!( self->memo = PyDict_New())) 
       goto err;

    Py_INCREF(f);
    self->file = f;

    /* Set read, readline based on type of f */
    if (PyFile_Check(f)) {
        self->fp = PyFile_AsFile(f);
	if (self->fp == NULL) {
	    PyErr_SetString(PyExc_ValueError, "I/O operation on closed file");
	    goto err;
	}
        self->read_func = read_file;
        self->readline_func = readline_file;
    }
    else if (PycStringIO_InputCheck(f)) {
        self->fp = NULL;
        self->read_func = read_cStringIO;
        self->readline_func = readline_cStringIO;
    }
    else {

        self->fp = NULL;
        self->read_func = read_other;
        self->readline_func = readline_other;

        if (!( (self->readline = PyObject_GetAttr(f, readline_str)) &&
            (self->read = PyObject_GetAttr(f, read_str))))  {
            PyErr_Clear();
            PyErr_SetString( PyExc_TypeError, "argument must have 'read' and "
                "'readline' attributes" );
            goto err;
        }
    }

    if (PyEval_GetRestricted()) {
        /* Restricted execution, get private tables */
        PyObject *m;

        if (!( m=PyImport_Import(copy_reg_str)))  goto err;
        self->safe_constructors=PyObject_GetAttr(m, safe_constructors_str);
        Py_DECREF(m);
        if (!( self->safe_constructors ))  goto err;
    }
    else {
        self->safe_constructors=safe_constructors;
        Py_INCREF(safe_constructors);
    }

    return self;

err:
    Py_DECREF((PyObject *)self);
    return NULL;
}


static PyObject *
get_Unpickler(PyObject *self, PyObject *args) {
    PyObject *file;

    if (!( PyArg_ParseTuple(args, "O:Unpickler", &file))) 
        return NULL;
    return (PyObject *)newUnpicklerobject(file);
}


static void
Unpickler_dealloc(Unpicklerobject *self) {
    Py_XDECREF(self->readline);
    Py_XDECREF(self->read);
    Py_XDECREF(self->file);
    Py_XDECREF(self->memo);
    Py_XDECREF(self->stack);
    Py_XDECREF(self->pers_func);
    Py_XDECREF(self->arg);
    Py_XDECREF(self->last_string);
    Py_XDECREF(self->safe_constructors);

    if (self->marks) {
        free(self->marks);
    }

    if (self->buf_size) {
        free(self->buf);
    }

    PyObject_Del(self);
}


static PyObject *
Unpickler_getattr(Unpicklerobject *self, char *name) {
    if (!strcmp(name, "persistent_load")) {
        if (!self->pers_func) {
            PyErr_SetString(PyExc_AttributeError, name);
            return NULL;
        }

        Py_INCREF(self->pers_func);
        return self->pers_func;
    }

    if (!strcmp(name, "find_global")) {
        if (!self->find_class) {
            PyErr_SetString(PyExc_AttributeError, name);
            return NULL;
        }

        Py_INCREF(self->find_class);
        return self->find_class;
    }

    if (!strcmp(name, "memo")) {
        if (!self->memo) {
            PyErr_SetString(PyExc_AttributeError, name);
            return NULL;
        }

        Py_INCREF(self->memo);
        return self->memo;
    }

    if (!strcmp(name, "UnpicklingError")) {
        Py_INCREF(UnpicklingError);
        return UnpicklingError;
    }

    return Py_FindMethod(Unpickler_methods, (PyObject *)self, name);
}


static int
Unpickler_setattr(Unpicklerobject *self, char *name, PyObject *value) {

    if (!strcmp(name, "persistent_load")) {
        Py_XDECREF(self->pers_func);
        self->pers_func = value;
        Py_XINCREF(value);
        return 0;
    }

    if (!strcmp(name, "find_global")) {
        Py_XDECREF(self->find_class);
        self->find_class = value;
        Py_XINCREF(value);
        return 0;
    }

    if (! value) {
        PyErr_SetString(PyExc_TypeError,
                        "attribute deletion is not supported");
        return -1;
    }

    if (strcmp(name, "memo") == 0) {
        if (!PyDict_Check(value)) {
	    PyErr_SetString(PyExc_TypeError, "memo must be a dictionary");
	    return -1;
        }
        Py_XDECREF(self->memo);
        self->memo = value;
        Py_INCREF(value);
        return 0;
    }

    PyErr_SetString(PyExc_AttributeError, name);
    return -1;
}


static PyObject *
cpm_dump(PyObject *self, PyObject *args) {
    PyObject *ob, *file, *res = NULL;
    Picklerobject *pickler = 0;
    int bin = 0;

    if (!( PyArg_ParseTuple(args, "OO|i", &ob, &file, &bin))) 
        goto finally;

    if (!( pickler = newPicklerobject(file, bin))) 
        goto finally;

    if (dump(pickler, ob) < 0)
        goto finally;

    Py_INCREF(Py_None);
    res = Py_None;

finally:
    Py_XDECREF(pickler);

    return res;
}


static PyObject *
cpm_dumps(PyObject *self, PyObject *args) {
    PyObject *ob, *file = 0, *res = NULL;
    Picklerobject *pickler = 0;
    int bin = 0;

    if (!( PyArg_ParseTuple(args, "O|i:dumps", &ob, &bin))) 
        goto finally;

    if (!( file = PycStringIO->NewOutput(128))) 
        goto finally;

    if (!( pickler = newPicklerobject(file, bin))) 
        goto finally;

    if (dump(pickler, ob) < 0)
        goto finally;

    res = PycStringIO->cgetvalue(file);

finally:
    Py_XDECREF(pickler);
    Py_XDECREF(file);

    return res;
}


static PyObject *
cpm_load(PyObject *self, PyObject *args) {
    Unpicklerobject *unpickler = 0;
    PyObject *ob, *res = NULL;

    if (!( PyArg_ParseTuple(args, "O:load", &ob))) 
        goto finally;

    if (!( unpickler = newUnpicklerobject(ob))) 
        goto finally;

    res = load(unpickler);

finally:
    Py_XDECREF(unpickler);

    return res;
}


static PyObject *
cpm_loads(PyObject *self, PyObject *args) {
    PyObject *ob, *file = 0, *res = NULL;
    Unpicklerobject *unpickler = 0;

    if (!( PyArg_ParseTuple(args, "S:loads", &ob))) 
        goto finally;

    if (!( file = PycStringIO->NewInput(ob))) 
        goto finally;

    if (!( unpickler = newUnpicklerobject(file))) 
        goto finally;

    res = load(unpickler);

finally:
    Py_XDECREF(file);
    Py_XDECREF(unpickler);

    return res;
}


static char Unpicklertype__doc__[] =
"Objects that know how to unpickle";

static PyTypeObject Unpicklertype = {
    PyObject_HEAD_INIT(NULL)
    0,                            /*ob_size*/
    "cPickle.Unpickler",                  /*tp_name*/
    sizeof(Unpicklerobject),              /*tp_basicsize*/
    0,                            /*tp_itemsize*/
    /* methods */
    (destructor)Unpickler_dealloc,        /*tp_dealloc*/
    (printfunc)0,         /*tp_print*/
    (getattrfunc)Unpickler_getattr,       /*tp_getattr*/
    (setattrfunc)Unpickler_setattr,       /*tp_setattr*/
    (cmpfunc)0,           /*tp_compare*/
    (reprfunc)0,          /*tp_repr*/
    0,                    /*tp_as_number*/
    0,            /*tp_as_sequence*/
    0,            /*tp_as_mapping*/
    (hashfunc)0,          /*tp_hash*/
    (ternaryfunc)0,               /*tp_call*/
    (reprfunc)0,          /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    Unpicklertype__doc__ /* Documentation string */
};

static struct PyMethodDef cPickle_methods[] = {
  {"dump",         (PyCFunction)cpm_dump,         METH_VARARGS,
   "dump(object, file, [binary]) --"
   "Write an object in pickle format to the given file\n"
   "\n"
   "If the optional argument, binary, is provided and is true, then the\n"
   "pickle will be written in binary format, which is more space and\n"
   "computationally efficient. \n"
  },
  {"dumps",        (PyCFunction)cpm_dumps,        METH_VARARGS,
   "dumps(object, [binary]) --"
   "Return a string containing an object in pickle format\n"
   "\n"
   "If the optional argument, binary, is provided and is true, then the\n"
   "pickle will be written in binary format, which is more space and\n"
   "computationally efficient. \n"
  },
  {"load",         (PyCFunction)cpm_load,         METH_VARARGS,
   "load(file) -- Load a pickle from the given file"},
  {"loads",        (PyCFunction)cpm_loads,        METH_VARARGS,
   "loads(string) -- Load a pickle from the given string"},
  {"Pickler",      (PyCFunction)get_Pickler,      METH_VARARGS,
   "Pickler(file, [binary]) -- Create a pickler\n"
   "\n"
   "If the optional argument, binary, is provided and is true, then\n"
   "pickles will be written in binary format, which is more space and\n"
   "computationally efficient. \n"
  },
  {"Unpickler",    (PyCFunction)get_Unpickler,    METH_VARARGS,
   "Unpickler(file) -- Create an unpickler"},
  { NULL, NULL }
};

static int
init_stuff(PyObject *module_dict) {
    PyObject *copy_reg, *t, *r;

#define INIT_STR(S) if (!( S ## _str=PyString_InternFromString(#S)))  return -1;

    INIT_STR(__class__);
    INIT_STR(__getinitargs__);
    INIT_STR(__dict__);
    INIT_STR(__getstate__);
    INIT_STR(__setstate__);
    INIT_STR(__name__);
    INIT_STR(__main__);
    INIT_STR(__reduce__);
    INIT_STR(write);
    INIT_STR(__safe_for_unpickling__);
    INIT_STR(append);
    INIT_STR(read);
    INIT_STR(readline);
    INIT_STR(copy_reg);
    INIT_STR(dispatch_table);
    INIT_STR(safe_constructors);
    INIT_STR(__basicnew__);

    if (!( copy_reg = PyImport_ImportModule("copy_reg"))) 
        return -1;

    /* These next few are special because we want to use different
       ones in restricted mode. */

    if (!( dispatch_table = PyObject_GetAttr(copy_reg, dispatch_table_str))) 
        return -1;

    if (!( safe_constructors = PyObject_GetAttr(copy_reg,
						 safe_constructors_str))) 
        return -1;

    Py_DECREF(copy_reg);

    /* Down to here ********************************** */

    if (!( empty_tuple = PyTuple_New(0))) 
        return -1;

    /* Ugh */
    if (!( t=PyImport_ImportModule("__builtin__")))  return -1;
    if (PyDict_SetItemString(module_dict, "__builtins__", t) < 0)
      return -1;

    if (!( t=PyDict_New()))  return -1;
    if (!( r=PyRun_String(
       "def __init__(self, *args): self.args=args\n\n"
       "def __str__(self):\n"
       "  return self.args and ('%s' % self.args[0]) or '(what)'\n",
       Py_file_input,
       module_dict, t)  ))  return -1;
    Py_DECREF(r);

    if (!( PickleError = PyErr_NewException("cPickle.PickleError", NULL, t))) 
      return -1;

    Py_DECREF(t);


    if (!( PicklingError = PyErr_NewException("cPickle.PicklingError",
					       PickleError, NULL))) 
      return -1;

    if (!( t=PyDict_New()))  return -1;
    if (!( r=PyRun_String(
       "def __init__(self, *args): self.args=args\n\n"
       "def __str__(self):\n"
       "  a=self.args\n"
       "  a=a and type(a[0]) or '(what)'\n"
       "  return 'Cannot pickle %s objects' % a\n"
       , Py_file_input,
       module_dict, t)  ))  return -1;
    Py_DECREF(r);

    if (!( UnpickleableError = PyErr_NewException(
                "cPickle.UnpickleableError", PicklingError, t))) 
      return -1;

    Py_DECREF(t);

    if (!( UnpicklingError = PyErr_NewException("cPickle.UnpicklingError",
   					         PickleError, NULL))) 
      return -1;

    if (PyDict_SetItemString(module_dict, "PickleError",
        PickleError) < 0)
        return -1;

    if (PyDict_SetItemString(module_dict, "PicklingError",
        PicklingError) < 0)
        return -1;

    if (PyDict_SetItemString(module_dict, "UnpicklingError",
        UnpicklingError) < 0)
        return -1;

    if (PyDict_SetItemString(module_dict, "UnpickleableError",
        UnpickleableError) < 0)
        return -1;

    if (!( BadPickleGet = PyString_FromString("cPickle.BadPickleGet"))) 
        return -1;

    if (PyDict_SetItemString(module_dict, "BadPickleGet",
        BadPickleGet) < 0)
        return -1;

    PycString_IMPORT;

    return 0;
}

#ifndef DL_EXPORT	/* declarations for DLL import/export */
#define DL_EXPORT(RTYPE) RTYPE
#endif
DL_EXPORT(void)
initcPickle(void) {
    PyObject *m, *d, *di, *v, *k;
    int i;
    char *rev="1.71";
    PyObject *format_version;
    PyObject *compatible_formats;

    Picklertype.ob_type = &PyType_Type;
    Picklertype.tp_getattro = PyObject_GenericGetAttr;
    Picklertype.tp_setattro = PyObject_GenericSetAttr;
    Unpicklertype.ob_type = &PyType_Type;
    PdataType.ob_type = &PyType_Type;

    /* Initialize some pieces. We need to do this before module creation,
       so we're forced to use a temporary dictionary. :(
    */
    di=PyDict_New();
    if (!di) return;
    if (init_stuff(di) < 0) return;

    /* Create the module and add the functions */
    m = Py_InitModule4("cPickle", cPickle_methods,
                     cPickle_module_documentation,
                     (PyObject*)NULL,PYTHON_API_VERSION);

    /* Add some symbolic constants to the module */
    d = PyModule_GetDict(m);
    PyDict_SetItemString(d,"__version__", v = PyString_FromString(rev));
    Py_XDECREF(v);

    /* Copy data from di. Waaa. */
    for (i=0; PyDict_Next(di, &i, &k, &v); ) {
        if (PyObject_SetItem(d, k, v) < 0) {
            Py_DECREF(di);
            return;
        }
    }
    Py_DECREF(di);

    format_version = PyString_FromString("1.3");
    compatible_formats = Py_BuildValue("[sss]", "1.0", "1.1", "1.2");

    PyDict_SetItemString(d, "format_version", format_version);
    PyDict_SetItemString(d, "compatible_formats", compatible_formats);
    Py_XDECREF(format_version);
    Py_XDECREF(compatible_formats);
}
