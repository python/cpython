/*
     cPickle.c,v 1.48 1997/12/07 14:37:39 jim Exp

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
*/

static char cPickle_module_documentation[] = 
"C implementation and optimization of the Python pickle module\n"
"\n"
"cPickle.c,v 1.48 1997/12/07 14:37:39 jim Exp\n"
;

#include "Python.h"
#include "cStringIO.h"
#include "mymath.h"

#ifndef Py_eval_input
#include <graminit.h>
#define Py_eval_input eval_input
#endif /* Py_eval_input */

#include <errno.h>

#define UNLESS(E) if (!(E))

#define DEL_LIST_SLICE(list, from, to) (PyList_SetSlice(list, from, to, NULL))

#define WRITE_BUF_SIZE 256


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

/* atol function from string module */
static PyObject *atol_func;

static PyObject *PicklingError;
static PyObject *UnpicklingError;

static PyObject *dispatch_table;
static PyObject *safe_constructors;
static PyObject *class_map;
static PyObject *empty_tuple;

static PyObject *__class___str, *__getinitargs___str, *__dict___str,
  *__getstate___str, *__setstate___str, *__name___str, *__reduce___str,
  *write_str, *__safe_for_unpickling___str, *append_str,
  *read_str, *readline_str, *__main___str, *__basicnew___str,
  *copy_reg_str, *dispatch_table_str, *safe_constructors_str;

static int save();
static int put2();

typedef struct {
     PyObject_HEAD
     FILE *fp;
     PyObject *write;
     PyObject *file;
     PyObject *memo;
     PyObject *arg;
     PyObject *pers_func;
     PyObject *inst_pers_func;
     int bin;
     int (*write_func)();
     char *write_buf;
     int buf_size;
     PyObject *dispatch_table;
     PyObject *class_map;
} Picklerobject;

staticforward PyTypeObject Picklertype;

typedef struct {
     PyObject_HEAD
     FILE *fp;
     PyObject *file;
     PyObject *readline;
     PyObject *read;
     PyObject *memo;
     PyObject *arg;
     PyObject *stack;
     PyObject *mark;
     PyObject *pers_func;
     PyObject *last_string;
     int *marks;
     int num_marks;
     int marks_size;
     int (*read_func)();
     int (*readline_func)();
     int buf_size;
     char *buf;
     PyObject *safe_constructors;
     PyObject *class_map;
} Unpicklerobject;
 
staticforward PyTypeObject Unpicklertype;

int 
cPickle_PyMapping_HasKey(PyObject *o, PyObject *key) {
    PyObject *v;

    if((v = PyObject_GetItem(o,key))) {
        Py_DECREF(v);
        return 1;
    }

    PyErr_Clear();
    return 0;
}

#define PyErr_Format PyErr_JFFormat
static
PyObject *
#ifdef HAVE_STDARG_PROTOTYPES
/* VARARGS 2 */
PyErr_Format(PyObject *ErrType, char *stringformat, char *format, ...) {
#else
/* VARARGS */
PyErr_Format(va_alist) va_dcl {
#endif
  va_list va;
  PyObject *args=0, *retval=0;
#ifdef HAVE_STDARG_PROTOTYPES
  va_start(va, format);
#else
  PyObject *ErrType;
  char *stringformat, *format;
  va_start(va);
  ErrType = va_arg(va, PyObject *);
  stringformat   = va_arg(va, char *);
  format   = va_arg(va, char *);
#endif
  
  if(format) args = Py_VaBuildValue(format, va);
  va_end(va);
  if(format && ! args) return NULL;
  if(stringformat && !(retval=PyString_FromString(stringformat))) return NULL;

  if(retval) {
      if(args) {
	  PyObject *v;
	  v=PyString_Format(retval, args);
	  Py_DECREF(retval);
	  Py_DECREF(args);
	  if(! v) return NULL;
	  retval=v;
	}
    }
  else
    if(args) retval=args;
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
    if (s == NULL) {
        return 0;
    }

    if ((int)fwrite(s, sizeof(char), n, self->fp) != n) {
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
    int res = -1;

    if (s == NULL) {
        UNLESS(self->buf_size) return 0;
        UNLESS(py_str = 
            PyString_FromStringAndSize(self->write_buf, self->buf_size))
            goto finally;
    }
    else {
        if (self->buf_size && (n + self->buf_size) > WRITE_BUF_SIZE) {
            if (write_other(self, NULL, 0) < 0)
                goto finally;
        }

        if (n > WRITE_BUF_SIZE) {    
            UNLESS(py_str = 
                PyString_FromStringAndSize(s, n))
                goto finally;
        }
        else {
            memcpy(self->write_buf + self->buf_size, s, n);
            self->buf_size += n;
            res = n;
            goto finally;
        }
    }

    UNLESS(self->arg)
        UNLESS(self->arg = PyTuple_New(1))
            goto finally;

    Py_INCREF(py_str);
    if (PyTuple_SetItem(self->arg, 0, py_str) < 0)
        goto finally;

    UNLESS(junk = PyObject_CallObject(self->write, self->arg))
        goto finally;
    Py_DECREF(junk);

    self->buf_size = 0;
 
    res = n;

finally:
    Py_XDECREF(py_str);

    return res;
}


static int 
read_file(Unpicklerobject *self, char **s, int  n) {

    if (self->buf_size == 0) {
        int size;

        size = ((n < 32) ? 32 : n); 
        UNLESS(self->buf = (char *)malloc(size * sizeof(char))) {
            PyErr_NoMemory();
            return -1;
        }

        self->buf_size = size;
    }
    else if (n > self->buf_size) {
        UNLESS(self->buf = (char *)realloc(self->buf, n * sizeof(char))) {
            PyErr_NoMemory();
            return -1;
        }
 
        self->buf_size = n;
    }
            
    if ((int)fread(self->buf, sizeof(char), n, self->fp) != n) {  
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
        UNLESS(self->buf = (char *)malloc(40 * sizeof(char))) {
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

        UNLESS(self->buf = (char *)realloc(self->buf, 
            (self->buf_size * 2) * sizeof(char))) {
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
    PyObject *bytes, *str;
    int res = -1;

    UNLESS(bytes = PyInt_FromLong(n)) {
        if (!PyErr_Occurred())
            PyErr_SetNone(PyExc_EOFError);

        goto finally;
    }

    UNLESS(self->arg)
        UNLESS(self->arg = PyTuple_New(1))
            goto finally;

    Py_INCREF(bytes);
    if (PyTuple_SetItem(self->arg, 0, bytes) < 0)
        goto finally;

    UNLESS(str = PyObject_CallObject(self->read, self->arg))
        goto finally;

    Py_XDECREF(self->last_string);
    self->last_string = str;

    *s = PyString_AsString(str);

    res = n;

finally:
     Py_XDECREF(bytes);

     return res;
}


static int 
readline_other(Unpicklerobject *self, char **s) {
    PyObject *str;
    int str_size;

    UNLESS(str = PyObject_CallObject(self->readline, empty_tuple)) {
        return -1;
    }

    str_size = PyString_Size(str);

    Py_XDECREF(self->last_string);
    self->last_string = str;

    *s = PyString_AsString(str);

    return str_size;
}


static char *
pystrndup(char *s, int l) {
  char *r;
  UNLESS(r=malloc((l+1)*sizeof(char))) return (char*)PyErr_NoMemory();
  memcpy(r,s,l);
  r[l]=0;
  return r;
}


static int
get(Picklerobject *self, PyObject *id) {
    PyObject *value = 0;
    long c_value;
    char s[30];
    int len;

    UNLESS(value = PyDict_GetItem(self->memo, id))
        return -1;

    UNLESS(value = PyTuple_GetItem(value, 0))
        return -1;
        
    c_value = PyInt_AsLong(value);

    if (!self->bin) {
        s[0] = GET;
        sprintf(s + 1, "%ld\n", c_value);
        len = strlen(s);
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
    if (ob->ob_refcnt < 2)
        return 0;

    return put2(self, ob);
}

  
static int
put2(Picklerobject *self, PyObject *ob) {
    char c_str[30];
    int p, len, res = -1;
    PyObject *py_ob_id = 0, *memo_len = 0, *t = 0;
    if ((p = PyDict_Size(self->memo)) < 0)
        goto finally;

    if (!self->bin) {
        c_str[0] = PUT;
        sprintf(c_str + 1, "%d\n", p);
        len = strlen(c_str);
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

    UNLESS(py_ob_id = PyInt_FromLong((long)ob))
        goto finally;

    UNLESS(memo_len = PyInt_FromLong(p))
        goto finally;

    UNLESS(t = PyTuple_New(2))
        goto finally;

    PyTuple_SET_ITEM(t, 0, memo_len);
    Py_INCREF(memo_len);
    PyTuple_SET_ITEM(t, 1, ob);
    Py_INCREF(ob);

    if (PyDict_SetItem(self->memo, py_ob_id, t) < 0)
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

  UNLESS(silly_list) {
      UNLESS(__import___str=PyString_FromString("__import__")) return NULL;
      UNLESS(__builtins___str=PyString_FromString("__builtins__")) return NULL;
      UNLESS(silly_list=Py_BuildValue("[s]","__doc__")) return NULL;
    }

  if((globals=PyEval_GetGlobals())) {
      Py_INCREF(globals);
      UNLESS(__builtins__=PyObject_GetItem(globals,__builtins___str)) goto err;
    }
  else {
      PyErr_Clear();

      UNLESS(standard_builtins ||
	     (standard_builtins=PyImport_ImportModule("__builtin__")))
	return NULL;
      
      __builtins__=standard_builtins;
      Py_INCREF(__builtins__);
      UNLESS(globals = Py_BuildValue("{sO}", "__builtins__", __builtins__))
	goto err;
    }

  if(PyDict_Check(__builtins__)) {
    UNLESS(__import__=PyObject_GetItem(__builtins__,__import___str)) goto err;
  }
  else {
    UNLESS(__import__=PyObject_GetAttr(__builtins__,__import___str)) goto err;
  }

  UNLESS(r=PyObject_CallFunction(__import__,"OOOO",
				 module_name, globals, globals, silly_list))
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
whichmodule(PyObject *class_map, PyObject *global, PyObject *global_name) {
    int i, j;
    PyObject *module = 0, *modules_dict = 0,
        *global_name_attr = 0, *name = 0;

    module = PyObject_GetAttrString(global, "__module__");
    if (module) return module;
    PyErr_Clear();

    if ((module = PyDict_GetItem(class_map, global))) {
        Py_INCREF(module);
        return module;
    }
    else {
        PyErr_Clear();
    }

    UNLESS(modules_dict = PySys_GetObject("modules"))
        return NULL;

    i = 0;
    while ((j = PyDict_Next(modules_dict, &i, &name, &module))) {

        if(PyObject_Compare(name, __main___str)==0) continue;
      
        UNLESS(global_name_attr = PyObject_GetAttr(module, global_name)) {
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
    if(!j) {
        j=1;
        name=__main___str;
    }
    
    /*
    if (!j) {
        PyErr_Format(PicklingError, "Could not find module for %s.", 
            "O", global_name);
        return NULL;
    }
    */

    PyDict_SetItem(class_map, global, name);

    Py_INCREF(name);
    return name;
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
	|| (l >> 32)
#endif
	    ) {
	        /* Save extra-long ints in non-binary mode, so that
		   we can use python long parsing code to restore,
		   if necessary. */
        c_str[0] = INT;
        sprintf(c_str + 1, "%ld\n", l);
        if ((*self->write_func)(self, c_str, strlen(c_str)) < 0)
            return -1;
    }
    else {
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

    UNLESS(repr = PyObject_Repr(args))
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

#ifdef FORMAT_1_3
    if (self->bin) {
        int s, e;
        double f;
        long fhi, flo;
        char str[9], *p = str;

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
        else {
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
        *p = ((e&0xF)<<4) | (fhi>>24);
        p++;

        /* Third byte */
        *p = (fhi>>16) & 0xFF;
        p++;

        /* Fourth byte */
        *p = (fhi>>8) & 0xFF;
        p++;

        /* Fifth byte */
        *p = fhi & 0xFF;
        p++;

        /* Sixth byte */
        *p = (flo>>16) & 0xFF;
        p++;

        /* Seventh byte */
        *p = (flo>>8) & 0xFF;
        p++;

        /* Eighth byte */
        *p = flo & 0xFF;

        if ((*self->write_func)(self, str, 9) < 0)
            return -1;
    }
    else
#endif
    {
        char c_str[250];
        c_str[0] = FLOAT;
        sprintf(c_str + 1, "%f\n", x);

        if ((*self->write_func)(self, c_str, strlen(c_str)) < 0)
            return -1;
    }

    return 0;
}


static int
save_string(Picklerobject *self, PyObject *args, int doput) {
    int size, len;

    size = PyString_Size(args);

    if (!self->bin) {
        PyObject *repr;
        char *repr_str;

        static char string = STRING;

        UNLESS(repr = PyObject_Repr(args))
            return -1;

        repr_str = PyString_AS_STRING((PyStringObject *)repr);
        len = PyString_Size(repr);

        if ((*self->write_func)(self, &string, 1) < 0)
            return -1;

        if ((*self->write_func)(self, repr_str, len) < 0)
            return -1;

        if ((*self->write_func)(self, "\n", 1) < 0)
            return -1;

        Py_XDECREF(repr);
    }
    else {
        int i;
        char c_str[5];

        size = PyString_Size(args);

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

        if ((*self->write_func)(self, 
            PyString_AS_STRING((PyStringObject *)args), size) < 0)
            return -1;
    }

    if (doput)
      if (put(self, args) < 0)
	return -1;

    return 0;
}


static int
save_tuple(Picklerobject *self, PyObject *args) {
    PyObject *element = 0, *py_tuple_id = 0;
    int len, i, has_key, res = -1;

    static char tuple = TUPLE;

    if ((*self->write_func)(self, &MARKv, 1) < 0)
        goto finally;

    if ((len = PyTuple_Size(args)) < 0)  
        goto finally;

    for (i = 0; i < len; i++) {
        UNLESS(element = PyTuple_GET_ITEM((PyTupleObject *)args, i))  
            goto finally;
    
        if (save(self, element, 0) < 0)
            goto finally;
    }

    UNLESS(py_tuple_id = PyInt_FromLong((long)args))
        goto finally;

    if (len) {
        if ((has_key = cPickle_PyMapping_HasKey(self->memo, py_tuple_id)) < 0)
            goto finally;

        if (has_key) {
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

    if(self->bin) {
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
        UNLESS(element = PyList_GET_ITEM((PyListObject *)args, i))  
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

    return res;
}


static int
save_dict(Picklerobject *self, PyObject *args) {
    PyObject *key = 0, *value = 0;
    int i, len, res = -1, using_setitems;
    char s[3];

    static char setitem = SETITEM, setitems = SETITEMS;

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

    return res;
}


static int  
save_inst(Picklerobject *self, PyObject *args) {
    PyObject *class = 0, *module = 0, *name = 0, *state = 0, 
             *getinitargs_func = 0, *getstate_func = 0, *class_args = 0;
    char *module_str, *name_str;
    int module_size, name_size, res = -1;

    static char inst = INST, obj = OBJ, build = BUILD;

    if ((*self->write_func)(self, &MARKv, 1) < 0)
        goto finally;

    UNLESS(class = PyObject_GetAttr(args, __class___str))
        goto finally;

    if (self->bin) {
        if (save(self, class, 0) < 0)
            goto finally;
    }

    if ((getinitargs_func = PyObject_GetAttr(args, __getinitargs___str))) {
        PyObject *element = 0;
        int i, len;

        UNLESS(class_args = 
            PyObject_CallObject(getinitargs_func, empty_tuple))
            goto finally;

        if ((len = PyObject_Length(class_args)) < 0)  
            goto finally;

        for (i = 0; i < len; i++) {
            UNLESS(element = PySequence_GetItem(class_args, i)) 
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
        UNLESS(name = ((PyClassObject *)class)->cl_name) {
            PyErr_SetString(PicklingError, "class has no name");
            goto finally;
        }

        UNLESS(module = whichmodule(self->class_map, class, name))
            goto finally;
    
        module_str = PyString_AS_STRING((PyStringObject *)module);
        module_size = PyString_Size(module);
        name_str   = PyString_AS_STRING((PyStringObject *)name);
        name_size = PyString_Size(name);

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
        UNLESS(state = PyObject_CallObject(getstate_func, empty_tuple))
            goto finally;
    }
    else {
        PyErr_Clear();

        UNLESS(state = PyObject_GetAttr(args, __dict___str)) {
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
    PyObject *global_name = 0, *module = 0;
    char *name_str, *module_str; 
    int module_size, name_size, res = -1;

    static char global = GLOBAL;

    if (name) {
        global_name = name;
        Py_INCREF(global_name);
    }
    else {
        UNLESS(global_name = PyObject_GetAttr(args, __name___str))
            goto finally;
    }

    UNLESS(module = whichmodule(self->class_map, args, global_name))
        goto finally;

    module_str = PyString_AS_STRING((PyStringObject *)module);
    module_size = PyString_Size(module);
    name_str   = PyString_AS_STRING((PyStringObject *)global_name);
    name_size = PyString_Size(global_name);

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

    return res;
}

static int
save_pers(Picklerobject *self, PyObject *args, PyObject *f) {
    PyObject *pid = 0;
    int size, res = -1;

    static char persid = PERSID, binpersid = BINPERSID;

    UNLESS(self->arg)
        UNLESS(self->arg = PyTuple_New(1))
            goto finally;
    
    Py_INCREF(args);      
    if (PyTuple_SetItem(self->arg, 0, args) < 0)
        goto finally;
    
    UNLESS(pid = PyObject_CallObject(f, self->arg))
        goto finally;

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
	        if(self->bin) res = save_empty_tuple(self, args);
	        else          res = save_tuple(self, args);
                goto finally;
            }

        case 's':
            if ((type == &PyString_Type) && (PyString_Size(args) < 2)) {
                res = save_string(self, args, 0);
                goto finally;
            }
    }

    if (args->ob_refcnt > 1) {
        long ob_id;
        int  has_key;

        ob_id = (long)args;

        UNLESS(py_ob_id = PyInt_FromLong(ob_id))
            goto finally;

        if ((has_key = cPickle_PyMapping_HasKey(self->memo, py_ob_id)) < 0)
            goto finally;

        if (has_key) {
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

        case 't':
            if (type == &PyTuple_Type) {
                res = save_tuple(self, args);
                goto finally;
	    }

        case 'l':
            if (type == &PyList_Type) {
                res = save_list(self, args);
                goto finally;
            }

        case 'd':
            if (type == &PyDict_Type) {
                res = save_dict(self, args);
                goto finally; 
            }

        case 'i':
            if (type == &PyInstance_Type) {
                res = save_inst(self, args);
                goto finally;
            }

        case 'c':
            if (type == &PyClass_Type) {
                res = save_global(self, args, NULL);
                goto finally;
            }

        case 'f':
            if (type == &PyFunction_Type) {
                res = save_global(self, args, NULL);
                goto finally;
            }

        case 'b':
            if (type == &PyCFunction_Type) {
                res = save_global(self, args, NULL);
                goto finally;
            }
    }

    if (!pers_save && self->inst_pers_func) {
        if ((tmp = save_pers(self, args, self->inst_pers_func)) != 0) {
            res = tmp;
            goto finally;
        }
    }

    if ((__reduce__ = PyDict_GetItem(dispatch_table, (PyObject *)type))) {
        Py_INCREF(__reduce__);

        UNLESS(self->arg)
            UNLESS(self->arg = PyTuple_New(1))
                goto finally;

        Py_INCREF(args);
        if (PyTuple_SetItem(self->arg, 0, args) < 0)
            goto finally;

        UNLESS(t = PyObject_CallObject(__reduce__, self->arg))
            goto finally;
    }        
    else {
        PyErr_Clear();

        if ((__reduce__ = PyObject_GetAttr(args, __reduce___str))) {
            UNLESS(t = PyObject_CallObject(__reduce__, empty_tuple))
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
            PyErr_Format(PicklingError, "Value returned by %s must "
                "be a tuple", "O", __reduce__);
            goto finally;
        }

        size = PyTuple_Size(t);
        
        if ((size != 3) && (size != 2)) {
            PyErr_Format(PicklingError, "tuple returned by %s must "     
                "contain only two or three elements", "O", __reduce__);
                goto finally;
        }
        
        callable = PyTuple_GET_ITEM(t, 0);

        arg_tup = PyTuple_GET_ITEM(t, 1);

        if (size > 2) {
            state = PyTuple_GET_ITEM(t, 2);
        }

        UNLESS(PyTuple_Check(arg_tup) || arg_tup==Py_None) {
            PyErr_Format(PicklingError, "Second element of tuple "
                "returned by %s must be a tuple", "O", __reduce__);
            goto finally;
        }

        res = save_reduce(self, callable, arg_tup, state, args);
        goto finally;
    }

    /*
    if (PyObject_HasAttrString(args, "__class__")) {
        res = save_inst(self, args);
        goto finally;
    }
    */

    PyErr_Format(PicklingError, "Cannot pickle %s objects.", 
        "O", (PyObject *)type);

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
Pickler_dump(Picklerobject *self, PyObject *args) {
    PyObject *ob;

    UNLESS(PyArg_ParseTuple(args, "O", &ob))
        return NULL;

    if (dump(self, ob) < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
dump_special(Picklerobject *self, PyObject *args) {
    static char stop = STOP;

    PyObject *callable, *arg_tup, *state = NULL;
    
    UNLESS(PyArg_ParseTuple(args, "OO|O", &callable, &arg_tup, &state))
        return NULL;

    UNLESS(PyTuple_Check(arg_tup)) {
        PyErr_SetString(PicklingError, "Second arg to dump_special must "
            "be tuple");
        return NULL;
    }

    if (save_reduce(self, callable, arg_tup, state, NULL) < 0)
        return NULL;

    if ((*self->write_func)(self, &stop, 1) < 0)
        return NULL;

    if ((*self->write_func)(self, NULL, 0) < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Pickle_clear_memo(Picklerobject *self, PyObject *args) {
  if(self->memo) PyDict_Clear(self->memo);
  Py_INCREF(Py_None);
  return Py_None;
}

static struct PyMethodDef Pickler_methods[] = {
  {"dump",          (PyCFunction)Pickler_dump,  1,
   "dump(object) --"
   "Write an object in pickle format to the object's pickle stream\n"
  },
  {"dump_special",  (PyCFunction)dump_special,  1,
   ""},
  {"clear_memo",  (PyCFunction)Pickle_clear_memo,  1,
   "clear_memo() -- Clear the picklers memo"},
  {NULL,                NULL}           /* sentinel */
};


static Picklerobject *
newPicklerobject(PyObject *file, int bin) {
    Picklerobject *self;

    UNLESS(self = PyObject_NEW(Picklerobject, &Picklertype))
        return NULL;

    self->fp = NULL;
    self->write = NULL;
    self->memo = NULL;
    self->arg = NULL;
    self->pers_func = NULL;
    self->inst_pers_func = NULL;
    self->write_buf = NULL;
    self->bin = bin;
    self->buf_size = 0;
    self->class_map = NULL;
    self->dispatch_table = NULL;

    Py_INCREF(file);
    self->file = file;

    UNLESS(self->memo = PyDict_New()) {
       Py_XDECREF((PyObject *)self);
       return NULL;
    }

    if (PyFile_Check(file)) {
        self->fp = PyFile_AsFile(file);
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

        UNLESS(self->write = PyObject_GetAttr(file, write_str)) {
            PyErr_Clear();
            PyErr_SetString(PyExc_TypeError, "argument must have 'write' "
                "attribute");
	    goto err;
        }

        UNLESS(self->write_buf = 
            (char *)malloc(WRITE_BUF_SIZE * sizeof(char))) { 
            PyErr_NoMemory();
	    goto err;
        }
    }

    if(PyEval_GetRestricted()) {
	/* Restricted execution, get private tables */
	PyObject *m;

	UNLESS(self->class_map=PyDict_New()) goto err;
	UNLESS(m=PyImport_Import(copy_reg_str)) goto err;
	self->dispatch_table=PyObject_GetAttr(m, dispatch_table_str);
	Py_DECREF(m);
	UNLESS(self->dispatch_table) goto err;
    }
    else {
	self->class_map=class_map;
	Py_INCREF(class_map);
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
    PyObject *file;
    int bin = 0;

    UNLESS(PyArg_ParseTuple(args, "O|i", &file, &bin))  return NULL;
    return (PyObject *)newPicklerobject(file, bin);
}


static void
Pickler_dealloc(Picklerobject *self) {
    Py_XDECREF(self->write);
    Py_XDECREF(self->memo);
    Py_XDECREF(self->arg);
    Py_XDECREF(self->file);
    Py_XDECREF(self->pers_func);
    Py_XDECREF(self->inst_pers_func);
    Py_XDECREF(self->class_map);
    Py_XDECREF(self->dispatch_table);

    if (self->write_buf) {    
        free(self->write_buf);
    }

    PyMem_DEL(self);
}


static PyObject *
Pickler_getattr(Picklerobject *self, char *name) {
    if (strcmp(name, "persistent_id") == 0) {
        if (!self->pers_func) {
            PyErr_SetString(PyExc_AttributeError, name);
            return NULL;
        }

        Py_INCREF(self->pers_func);
        return self->pers_func;
    }

    if (strcmp(name, "memo") == 0) {
        if (!self->memo) {
            PyErr_SetString(PyExc_AttributeError, name);
            return NULL;
        }

        Py_INCREF(self->memo);
        return self->memo;
    }

    if (strcmp(name, "PicklingError") == 0) {
        Py_INCREF(PicklingError);
        return PicklingError;
    }
  
    return Py_FindMethod(Pickler_methods, (PyObject *)self, name);
}


int 
Pickler_setattr(Picklerobject *self, char *name, PyObject *value) {
    if (strcmp(name, "persistent_id") == 0) {
        Py_XDECREF(self->pers_func);
        self->pers_func = value;
        Py_INCREF(value);
        return 0;
    }

    if (strcmp(name, "inst_persistent_id") == 0) {
        Py_XDECREF(self->inst_pers_func);
        self->inst_pers_func = value;
        Py_INCREF(value);
        return 0;
    }

    PyErr_SetString(PyExc_AttributeError, name);
    return -1;
}


static char Picklertype__doc__[] =
"Objects that know how to pickle objects\n"
;

static PyTypeObject Picklertype = {
    PyObject_HEAD_INIT(NULL)
    0,                            /*ob_size*/
    "Pickler",                    /*tp_name*/
    sizeof(Picklerobject),                /*tp_basicsize*/
    0,                            /*tp_itemsize*/
    /* methods */
    (destructor)Pickler_dealloc,  /*tp_dealloc*/
    (printfunc)0,         /*tp_print*/
    (getattrfunc)Pickler_getattr, /*tp_getattr*/
    (setattrfunc)Pickler_setattr, /*tp_setattr*/
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
    Picklertype__doc__ /* Documentation string */
};

static PyObject *
find_class(PyObject *class_map,
	   PyObject *py_module_name, PyObject *py_global_name) {
    PyObject *global = 0, *t = 0, *module;

    UNLESS(t = PyTuple_New(2)) return NULL;

    PyTuple_SET_ITEM((PyTupleObject *)t, 0, py_module_name);
    Py_INCREF(py_module_name);
    PyTuple_SET_ITEM((PyTupleObject *)t, 1, py_global_name);
    Py_INCREF(py_global_name);

    global=PyDict_GetItem(class_map, t);

    if (global) {
      Py_DECREF(t);
      Py_INCREF(global);
      return global;
    }

    PyErr_Clear();

    UNLESS(module=PyImport_Import(py_module_name)) return NULL;
    global=PyObject_GetAttr(module, py_global_name);
    Py_DECREF(module);
    UNLESS(global) return NULL;

    if (PyDict_SetItem(class_map, t, global) < 0) global=NULL;
    Py_DECREF(t);

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
    if (PyList_Append(self->stack, Py_None) < 0)
        return -1;

    return 0;
}


static int
load_int(Unpicklerobject *self) {
    PyObject *py_int = 0;
    char *endptr, *s;
    int len, res = -1;
    long l;

    if ((len = (*self->readline_func)(self, &s)) < 0) return -1;
    UNLESS(s=pystrndup(s,len)) return -1;

    errno = 0;
    l = strtol(s, &endptr, 0);

    if (errno || (*endptr != '\n') || (endptr[1] != '\0')) {
        /* Hm, maybe we've got something long.  Let's try reading
	   it as a Python long object. */
        errno=0;
        UNLESS(py_int=PyLong_FromString(s,&endptr,0)) goto finally;

	if ((*endptr != '\n') || (endptr[1] != '\0')) {
	    PyErr_SetString(PyExc_ValueError,
			    "could not convert string to int");
	    goto finally;
	}
    }
    else {
        UNLESS(py_int = PyInt_FromLong(l)) goto finally;
    }

    if (PyList_Append(self->stack, py_int) < 0) goto finally;

    res = 0;

finally:
    free(s);
    Py_XDECREF(py_int);

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

    return l;
}


static int
load_binintx(Unpicklerobject *self, char *s, int  x) {
    PyObject *py_int = 0;
    long l;

    l = calc_binint(s, x);

    UNLESS(py_int = PyInt_FromLong(l))
        return -1;
    
    if (PyList_Append(self->stack, py_int) < 0) {
        Py_DECREF(py_int);
        return -1;
    }

    Py_DECREF(py_int);
    
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
    UNLESS(s=pystrndup(s,len)) return -1;

    UNLESS(l = PyLong_FromString(s, &end, 0))
        goto finally;

    if (PyList_Append(self->stack, l) < 0)
        goto finally;

    res = 0;

finally:
    free(s);
    Py_XDECREF(l);

    return res;
}

 
static int
load_float(Unpicklerobject *self) {
    PyObject *py_float = 0;
    char *endptr, *s;
    int len, res = -1;
    double d;

    if ((len = (*self->readline_func)(self, &s)) < 0) return -1;
    UNLESS(s=pystrndup(s,len)) return -1;

    errno = 0;
    d = strtod(s, &endptr);

    if (errno || (endptr[0] != '\n') || (endptr[1] != '\0')) {
        PyErr_SetString(PyExc_ValueError, 
        "could not convert string to float");
        goto finally;
    }

    UNLESS(py_float = PyFloat_FromDouble(d))
        goto finally;

    if (PyList_Append(self->stack, py_float) < 0)
        goto finally;

    res = 0;

finally:
    free(s);
    Py_XDECREF(py_float);

    return res;
}

static int
load_binfloat(Unpicklerobject *self) {
    PyObject *py_float = 0;
    int s, e, res = -1;
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

    UNLESS(py_float = PyFloat_FromDouble(x))
        goto finally;

    if (PyList_Append(self->stack, py_float) < 0) 
        goto finally;

    res = 0;

finally:
    Py_XDECREF(py_float);

    return res;
}

static int
load_string(Unpicklerobject *self) {
    PyObject *str = 0;
    int len, res = -1, nslash;
    char *s, q, *p;

    static PyObject *eval_dict = 0;

    if ((len = (*self->readline_func)(self, &s)) < 0) return -1;
    UNLESS(s=pystrndup(s,len)) return -1;

    /* Check for unquoted quotes (evil strings) */
    q=*s;
    if(q != '"' && q != '\'') goto insecure;
    for(p=s+1, nslash=0; *p; p++)
      {
	if(*p==q && nslash%2==0) break;
	if(*p=='\\') nslash++;
	else nslash=0;
      }
    if(*p==q)
      {
	for(p++; *p; p++) if(*p > ' ') goto insecure;
      }
    else goto insecure;
    /********************************************/

    UNLESS(eval_dict)
        UNLESS(eval_dict = Py_BuildValue("{s{}}", "__builtins__"))
            goto finally;

    UNLESS(str = PyRun_String(s, Py_eval_input, eval_dict, eval_dict))
        goto finally;

    if (PyList_Append(self->stack, str) < 0)
        goto finally;

    res = 0;

finally:
    free(s);
    Py_XDECREF(str);

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
    int res = -1;
    char *s;

    if ((*self->read_func)(self, &s, 4) < 0)
        goto finally;

    l = calc_binint(s, 4);

    if ((*self->read_func)(self, &s, l) < 0)
        goto finally;

    UNLESS(py_string = PyString_FromStringAndSize(s, l))
        goto finally;

    if (PyList_Append(self->stack, py_string) < 0)
        goto finally;

    res = 0;

finally:
    Py_XDECREF(py_string);

    return res;
}


static int
load_short_binstring(Unpicklerobject *self) {
    PyObject *py_string = 0;
    unsigned char l;  
    int res = -1;
    char *s;

    if ((*self->read_func)(self, &s, 1) < 0)
        return -1;

    l = (unsigned char)s[0];

    if ((*self->read_func)(self, &s, l) < 0)
        goto finally;

    UNLESS(py_string = PyString_FromStringAndSize(s, l))
        goto finally;

    if (PyList_Append(self->stack, py_string) < 0)
        goto finally;

    res = 0;

finally:
    Py_XDECREF(py_string);

    return res;
} 


static int
load_tuple(Unpicklerobject *self) {
    PyObject *tup = 0, *slice = 0, *list = 0;
    int i, j, res = -1;

    if ((i = marker(self)) < 0)
        goto finally;

    if ((j = PyList_Size(self->stack)) < 0)  
        goto finally;

    UNLESS(slice = PyList_GetSlice(self->stack, i, j))
        goto finally;
  
    UNLESS(tup = PySequence_Tuple(slice))
        goto finally;

    UNLESS(list = PyList_New(1))
        goto finally;

    Py_INCREF(tup);
    if (PyList_SetItem(list, 0, tup) < 0)
        goto finally;

    if (PyList_SetSlice(self->stack, i, j, list) < 0)
        goto finally;

    res = 0;

finally:
    Py_XDECREF(tup);
    Py_XDECREF(list);
    Py_XDECREF(slice);

    return res;
}

static int
load_empty_tuple(Unpicklerobject *self) {
    PyObject *tup = 0;
    int res;

    UNLESS(tup=PyTuple_New(0)) return -1;
    res=PyList_Append(self->stack, tup);
    Py_DECREF(tup);
    return res;
}

static int
load_empty_list(Unpicklerobject *self) {
    PyObject *list = 0;
    int res;

    UNLESS(list=PyList_New(0)) return -1;
    res=PyList_Append(self->stack, list);
    Py_DECREF(list);
    return res;
}

static int
load_empty_dict(Unpicklerobject *self) {
    PyObject *dict = 0;
    int res;

    UNLESS(dict=PyDict_New()) return -1;
    res=PyList_Append(self->stack, dict);
    Py_DECREF(dict);
    return res;
}


static int
load_list(Unpicklerobject *self) {
    PyObject *list = 0, *slice = 0;
    int i, j, l, res = -1;

    if ((i = marker(self)) < 0)
        goto finally;

    if ((j = PyList_Size(self->stack)) < 0)
        goto finally;

    UNLESS(slice = PyList_GetSlice(self->stack, i, j))
        goto finally;

    if((l=PyList_Size(slice)) < 0)
        goto finally;

    if(l) {
      UNLESS(list = PyList_New(1))
        goto finally;

      Py_INCREF(slice);
      if (PyList_SetItem(list, 0, slice) < 0)
        goto finally;
      
      if (PyList_SetSlice(self->stack, i, j, list) < 0)
        goto finally;
    } else {
      if(PyList_Append(self->stack,slice) < 0)
	goto finally;
    }

    res = 0;

finally:
    Py_XDECREF(list);
    Py_XDECREF(slice);

    return res;
}

static int
load_dict(Unpicklerobject *self) {
    PyObject *list = 0, *dict = 0, *key = 0, *value = 0;
    int i, j, k, res = -1;

    if ((i = marker(self)) < 0)
        goto finally;

    if ((j = PyList_Size(self->stack)) < 0)
        goto finally;

    UNLESS(dict = PyDict_New())
        goto finally;

    for (k = i; k < j; k += 2) {
        UNLESS(key = PyList_GET_ITEM((PyListObject *)self->stack, k))
            goto finally;

        UNLESS(value = PyList_GET_ITEM((PyListObject *)self->stack, k + 1))
            goto finally;

        if (PyDict_SetItem(dict, key, value) < 0)
            goto finally;
    }

    if(j) {

      UNLESS(list = PyList_New(1))
        goto finally;

      Py_INCREF(dict);
      if (PyList_SetItem(list, 0, dict) < 0)
        goto finally;

      if (PyList_SetSlice(self->stack, i, j, list) < 0)
        goto finally;
    }
    else 
      if(PyList_Append(self->stack, dict) < 0)
	goto finally;

    res = 0;

finally:
    Py_XDECREF(dict);
    Py_XDECREF(list);

    return res;
}

static PyObject *
Instance_New(PyObject *cls, PyObject *args) {
  int has_key;
  PyObject *safe=0, *r=0;

  if (PyClass_Check(cls)) {
      int l;
      
      if((l=PyObject_Length(args)) < 0) goto err;
      UNLESS(l) {
	  PyObject *__getinitargs__;

	  UNLESS(__getinitargs__=PyObject_GetAttr(cls, __getinitargs___str)) {
	      /* We have a class with no __getinitargs__, so bypass usual
		 construction  */
	      PyInstanceObject *inst;

	      PyErr_Clear();
	      UNLESS(inst=PyObject_NEW(PyInstanceObject, &PyInstance_Type))
		goto err;
	      inst->in_class=(PyClassObject*)cls;
	      Py_INCREF(cls);
	      UNLESS(inst->in_dict=PyDict_New()) {
		Py_DECREF(inst);
		goto err;
	      }

	      return (PyObject *)inst;
	    }
	  Py_DECREF(__getinitargs__);
	}
      
      if((r=PyInstance_New(cls, args, NULL))) return r;
      else goto err;
    }
       
  
  if ((has_key = cPickle_PyMapping_HasKey(safe_constructors, cls)) < 0)
    goto err;
    
  if (!has_key)
    if(!(safe = PyObject_GetAttr(cls, __safe_for_unpickling___str)) ||
       !PyObject_IsTrue(safe)) {
      PyErr_Format(UnpicklingError, "%s is not safe for unpickling", "O", cls);
      Py_XDECREF(safe);
      return NULL;
  }

  if(args==Py_None)
    {
      /* Special case, call cls.__basicnew__() */
      PyObject *basicnew;
      
      UNLESS(basicnew=PyObject_GetAttr(cls, __basicnew___str)) return NULL;
      r=PyObject_CallObject(basicnew, NULL);
      Py_DECREF(basicnew);
      if(r) return r;
    }

  if((r=PyObject_CallObject(cls, args))) return r;

err:
  {
    PyObject *tp, *v, *tb;

    PyErr_Fetch(&tp, &v, &tb);
    if((r=Py_BuildValue("OOO",v,cls,args))) {
	Py_XDECREF(v);
	v=r;
      }
    PyErr_Restore(tp,v,tb);
  }
  return NULL;
}
  

static int
load_obj(Unpicklerobject *self) {
    PyObject *class = 0, *slice = 0, *tup = 0, *obj = 0;
    int i, len, res = -1;

    if ((i = marker(self)) < 0)
        goto finally;

    if ((len = PyList_Size(self->stack)) < 0)
        goto finally;

    UNLESS(slice = PyList_GetSlice(self->stack, i + 1, len))
        goto finally;

    UNLESS(tup = PySequence_Tuple(slice))
        goto finally;

    class = PyList_GET_ITEM((PyListObject *)self->stack, i);
    Py_INCREF(class);

    UNLESS(obj = Instance_New(class, tup))
        goto finally;

    if (DEL_LIST_SLICE(self->stack, i, len) < 0)
        goto finally;

    if (PyList_Append(self->stack, obj) < 0)
        goto finally;
  
    res = 0;

finally:
    
    Py_XDECREF(class);
    Py_XDECREF(slice);
    Py_XDECREF(tup);
    Py_XDECREF(obj);

    return res;
}


static int
load_inst(Unpicklerobject *self) {
    PyObject *arg_tup = 0, *arg_slice = 0, *class = 0, *obj = 0,
             *module_name = 0, *class_name = 0;
    int i, j, len, res = -1;
    char *s;

    if ((i = marker(self)) < 0) goto finally;

    if ((j = PyList_Size(self->stack)) < 0) goto finally;

    UNLESS(arg_slice = PyList_GetSlice(self->stack, i, j)) goto finally;

    UNLESS(arg_tup = PySequence_Tuple(arg_slice)) goto finally;

    if (DEL_LIST_SLICE(self->stack, i, j) < 0) goto finally;

    if ((len = (*self->readline_func)(self, &s)) < 0) goto finally;

    UNLESS(module_name = PyString_FromStringAndSize(s, len - 1)) goto finally;

    if ((len = (*self->readline_func)(self, &s)) < 0) goto finally;

    UNLESS(class_name = PyString_FromStringAndSize(s, len - 1)) goto finally;

    UNLESS(class = find_class(self->class_map, module_name, class_name))
        goto finally;

    UNLESS(obj = Instance_New(class, arg_tup)) goto finally;

    if (PyList_Append(self->stack, obj) < 0) goto finally;

    res = 0;

finally:
    Py_XDECREF(class);
    Py_XDECREF(arg_slice);
    Py_XDECREF(arg_tup);
    Py_XDECREF(obj);
    Py_XDECREF(module_name);
    Py_XDECREF(class_name);

    return res;
}


static int
load_global(Unpicklerobject *self) {
    PyObject *class = 0, *module_name = 0, *class_name = 0;
    int res = -1, len;
    char *s;

    if ((len = (*self->readline_func)(self, &s)) < 0)
        goto finally;

    UNLESS(module_name = PyString_FromStringAndSize(s, len - 1))
        goto finally;

    if ((len = (*self->readline_func)(self, &s)) < 0)
        goto finally;

    UNLESS(class_name = PyString_FromStringAndSize(s, len - 1))
        goto finally;

    UNLESS(class = find_class(self->class_map, module_name, class_name))
        goto finally;

    if (PyList_Append(self->stack, class) < 0)
        goto finally;

    res = 0;

finally:
    Py_XDECREF(class);
    Py_XDECREF(module_name);
    Py_XDECREF(class_name);

    return res;
}


static int
load_persid(Unpicklerobject *self) {
    PyObject *pid = 0, *pers_load_val = 0;
    int len, res = -1;
    char *s;

    if (self->pers_func) {
        if ((len = (*self->readline_func)(self, &s)) < 0)
            goto finally;
  
        UNLESS(pid = PyString_FromStringAndSize(s, len - 1))
           goto finally;

	if(PyList_Check(self->pers_func)) {
	    if(PyList_Append(self->pers_func, pid) < 0) goto finally;
	    pers_load_val=pid;
	    Py_INCREF(pid);
	  }
	else {
	    UNLESS(self->arg)
	      UNLESS(self->arg = PyTuple_New(1))
	        goto finally;

	    Py_INCREF(pid);
	    if (PyTuple_SetItem(self->arg, 0, pid) < 0)
	      goto finally;
      
	    UNLESS(pers_load_val = 
		   PyObject_CallObject(self->pers_func, self->arg))
	      goto finally;
	  }
	if (PyList_Append(self->stack, pers_load_val) < 0)
	  goto finally;
    }
    else {
      PyErr_SetString(UnpicklingError,
		      "A load persistent id instruction was encountered,\n"
		      "but no persistent_load function was specified.");
      goto finally;
    }

    res = 0;

finally:
    Py_XDECREF(pid);
    Py_XDECREF(pers_load_val);

    return res;
}


static int
load_binpersid(Unpicklerobject *self) {
    PyObject *pid = 0, *pers_load_val = 0;
    int len, res = -1;

    if (self->pers_func) {
        if ((len = PyList_Size(self->stack)) < 0)
            goto finally;

        pid = PyList_GET_ITEM((PyListObject *)self->stack, len - 1);
        Py_INCREF(pid);
 
        if (DEL_LIST_SLICE(self->stack, len - 1, len) < 0)
            goto finally;

	if(PyList_Check(self->pers_func)) {
	    if(PyList_Append(self->pers_func, pid) < 0) goto finally;
	    pers_load_val=pid;
	    Py_INCREF(pid);
	  }
	else {
	  UNLESS(self->arg)
            UNLESS(self->arg = PyTuple_New(1))
	    goto finally;

	  Py_INCREF(pid);
	  if (PyTuple_SetItem(self->arg, 0, pid) < 0)
            goto finally;

	  UNLESS(pers_load_val = 
		 PyObject_CallObject(self->pers_func, self->arg))
            goto finally;
	}
        if (PyList_Append(self->stack, pers_load_val) < 0)
            goto finally;
    }
    else {
      PyErr_SetString(UnpicklingError,
		      "A load persistent id instruction was encountered,\n"
		      "but no persistent_load function was specified.");
      goto finally;
    }

    res = 0;

finally:
    Py_XDECREF(pid);
    Py_XDECREF(pers_load_val);

    return res;
}


static int
load_pop(Unpicklerobject *self) {
    int len;

    if ((len = PyList_Size(self->stack)) < 0)  
        return -1;

    if ((self->num_marks > 0) && 
        (self->marks[self->num_marks - 1] == len))
        self->num_marks--;
    else if (DEL_LIST_SLICE(self->stack, len - 1, len) < 0)  
        return -1;

    return 0;
}


static int
load_pop_mark(Unpicklerobject *self) {
    int i, len;

    if ((i = marker(self)) < 0)
        return -1;

    if ((len = PyList_Size(self->stack)) < 0)
        return -1;

    if (DEL_LIST_SLICE(self->stack, i, len) < 0)
        return -1;

    return 0;
}


static int
load_dup(Unpicklerobject *self) {
    PyObject *last;
    int len;

    if ((len = PyList_Size(self->stack)) < 0)
        return -1;
  
    UNLESS(last = PyList_GetItem(self->stack, len - 1))  
        return -1;

    if (PyList_Append(self->stack, last) < 0)
        return -1;

    return 0;
}


static int
load_get(Unpicklerobject *self) {
    PyObject *py_str = 0, *value = 0;
    int len, res = -1;
    char *s;

    if ((len = (*self->readline_func)(self, &s)) < 0)
        goto finally;

    UNLESS(py_str = PyString_FromStringAndSize(s, len - 1))
        goto finally;
  
    UNLESS(value = PyDict_GetItem(self->memo, py_str))  
        goto finally;

    if (PyList_Append(self->stack, value) < 0)
        goto finally;

    res = 0;

finally:
    Py_XDECREF(py_str);

    return res;
}


static int
load_binget(Unpicklerobject *self) {
    PyObject *py_key = 0, *value = 0;
    unsigned char key;
    int res = -1;
    char *s;

    if ((*self->read_func)(self, &s, 1) < 0)
        goto finally;

    key = (unsigned char)s[0];

    UNLESS(py_key = PyInt_FromLong((long)key))
        goto finally;

    UNLESS(value = PyDict_GetItem(self->memo, py_key))  
        goto finally;

    if (PyList_Append(self->stack, value) < 0)
        goto finally;

    res = 0;

finally:
    Py_XDECREF(py_key);

    return res;
}


static int
load_long_binget(Unpicklerobject *self) {
    PyObject *py_key = 0, *value = 0;
    unsigned char c, *s;
    long key;
    int res = -1;

    if ((*self->read_func)(self, &s, 4) < 0)
        goto finally;

    c = (unsigned char)s[0];
    key = (long)c;
    c = (unsigned char)s[1];
    key |= (long)c << 8;
    c = (unsigned char)s[2];
    key |= (long)c << 16;
    c = (unsigned char)s[3];
    key |= (long)c << 24;

    UNLESS(py_key = PyInt_FromLong(key))
        goto finally;

    UNLESS(value = PyDict_GetItem(self->memo, py_key))  
        goto finally;

    if (PyList_Append(self->stack, value) < 0)
        goto finally;

    res = 0;

finally:
    Py_XDECREF(py_key);

    return res;
}


static int
load_put(Unpicklerobject *self) {
    PyObject *py_str = 0, *value = 0;
    int len, res = -1;
    char *s;

    if ((len = (*self->readline_func)(self, &s)) < 0)
        goto finally;

    UNLESS(py_str = PyString_FromStringAndSize(s, len - 1))
        goto finally;

    if ((len = PyList_Size(self->stack)) < 0)
        goto finally;

    UNLESS(value = PyList_GetItem(self->stack, len - 1))  
        goto finally;

    if (PyDict_SetItem(self->memo, py_str, value) < 0)  
        goto finally;

    res = 0;

finally:
    Py_XDECREF(py_str);

    return res;
}


static int
load_binput(Unpicklerobject *self) {
    PyObject *py_key = 0, *value = 0;
    unsigned char key, *s;
    int len, res = -1;

    if ((*self->read_func)(self, &s, 1) < 0)
        goto finally;

    key = (unsigned char)s[0];

    UNLESS(py_key = PyInt_FromLong((long)key))
        goto finally;

    if ((len = PyList_Size(self->stack)) < 0)
        goto finally;

    UNLESS(value = PyList_GetItem(self->stack, len - 1))  
        goto finally;

    if (PyDict_SetItem(self->memo, py_key, value) < 0)  
        goto finally;

    res = 0;

finally:
    Py_XDECREF(py_key);

    return res;
}


static int
load_long_binput(Unpicklerobject *self) {
    PyObject *py_key = 0, *value = 0;
    long key;
    unsigned char c, *s;
    int len, res = -1;

    if ((*self->read_func)(self, &s, 4) < 0)
        goto finally;

    c = (unsigned char)s[0];
    key = (long)c;
    c = (unsigned char)s[1];
    key |= (long)c << 8;
    c = (unsigned char)s[2];
    key |= (long)c << 16;
    c = (unsigned char)s[3];
    key |= (long)c << 24;

    UNLESS(py_key = PyInt_FromLong(key))
        goto finally;

    if ((len = PyList_Size(self->stack)) < 0)
        goto finally;

    UNLESS(value = PyList_GetItem(self->stack, len - 1))  
        goto finally;

    if (PyDict_SetItem(self->memo, py_key, value) < 0)  
        goto finally;

    res = 0;

finally:
    Py_XDECREF(py_key);

    return res;
}


static int 
do_append(Unpicklerobject *self, int  x) {
    PyObject *value = 0, *list = 0, *append_method = 0;
    int len, i;

    if ((len = PyList_Size(self->stack)) < 0)  
        return -1;

    UNLESS(list = PyList_GetItem(self->stack, x - 1))  
        goto err;

    if (PyList_Check(list)) {
        PyObject *slice = 0;
        int list_len;
       
        UNLESS(slice = PyList_GetSlice(self->stack, x, len))
            return -1;
  
        list_len = PyList_Size(list);
        if (PyList_SetSlice(list, list_len, list_len, slice) < 0) {
            Py_DECREF(slice);
            return -1;
        }

        Py_DECREF(slice);
    }
    else {

        UNLESS(append_method = PyObject_GetAttr(list, append_str))
            return -1;
         
        for (i = x; i < len; i++) {
	    PyObject *junk;

            UNLESS(value = PyList_GetItem(self->stack, i))  
                return -1;

	    UNLESS(self->arg)
	      UNLESS(self->arg = PyTuple_New(1)) 
	        goto err;
	    
	    Py_INCREF(value);
	    if (PyTuple_SetItem(self->arg, 0, value) < 0) 
	      goto err;
	    
	    UNLESS(junk = PyObject_CallObject(append_method, self->arg)) 
	      goto err;
	    Py_DECREF(junk);
        }
    }

    if (DEL_LIST_SLICE(self->stack, x, len) < 0)  
        goto err;

    Py_XDECREF(append_method);

    return 0;

err:
    Py_XDECREF(append_method);
 
    return -1;
}

    
static int
load_append(Unpicklerobject *self) {
    return do_append(self, PyList_Size(self->stack) - 1);
}


static int
load_appends(Unpicklerobject *self) {
    return do_append(self, marker(self));
}


static int
do_setitems(Unpicklerobject *self, int  x) {
    PyObject *value = 0, *key = 0, *dict = 0;
    int len, i, res = -1;

    if ((len = PyList_Size(self->stack)) < 0)  
        goto finally;

    UNLESS(dict = PyList_GetItem(self->stack, x - 1))
        goto finally;

    for (i = x; i < len; i += 2) {
        UNLESS(key = PyList_GetItem(self->stack, i))  
            goto finally;

        UNLESS(value = PyList_GetItem(self->stack, i + 1))
            goto finally;

        if (PyObject_SetItem(dict, key, value) < 0)  
            goto finally;
    }

    if (DEL_LIST_SLICE(self->stack, x, len) < 0)  
        goto finally;

    res = 0;

finally:

    return res;
}


static int
load_setitem(Unpicklerobject *self) {
    return do_setitems(self, PyList_Size(self->stack) - 2);
}


static int
load_setitems(Unpicklerobject *self) {
    return do_setitems(self, marker(self));
}


static int
load_build(Unpicklerobject *self) {
    PyObject *value = 0, *inst = 0, *instdict = 0, *d_key = 0, *d_value = 0, 
             *junk = 0, *__setstate__ = 0;
    int len, i, res = -1;

    if ((len = PyList_Size(self->stack)) < 0)
        goto finally;

    UNLESS(value = PyList_GetItem(self->stack, len - 1))
        goto finally; 
    Py_INCREF(value);

    if (DEL_LIST_SLICE(self->stack, len - 1, len) < 0)
        goto finally;

    UNLESS(inst = PyList_GetItem(self->stack, len - 2))
        goto finally;

    UNLESS(__setstate__ = PyObject_GetAttr(inst, __setstate___str)) {
        PyErr_Clear();

        UNLESS(instdict = PyObject_GetAttr(inst, __dict___str))
            goto finally;

        i = 0;
        while (PyDict_Next(value, &i, &d_key, &d_value)) {
            if (PyObject_SetItem(instdict, d_key, d_value) < 0)
                goto finally;
        }
    }
    else {
        UNLESS(self->arg)
            UNLESS(self->arg = PyTuple_New(1))
                goto finally;

        Py_INCREF(value);
        if (PyTuple_SetItem(self->arg, 0, value) < 0)
            goto finally;

        UNLESS(junk = PyObject_CallObject(__setstate__, self->arg))
            goto finally;
        Py_DECREF(junk);
    }

    res = 0;

finally:
  Py_XDECREF(value);
  Py_XDECREF(instdict);
  Py_XDECREF(__setstate__);
  
  return res;
}


static int
load_mark(Unpicklerobject *self) {
    int len;

    if ((len = PyList_Size(self->stack)) < 0)
        return -1;

    if (!self->marks_size) {
        self->marks_size = 20;
        UNLESS(self->marks = (int *)malloc(self->marks_size * sizeof(int))) {
            PyErr_NoMemory();
            return -1;
        } 
    }
    else if ((self->num_marks + 1) >= self->marks_size) {
        UNLESS(self->marks = (int *)realloc(self->marks,
            (self->marks_size + 20) * sizeof(int))) {
            PyErr_NoMemory();
            return -1;
        }

        self->marks_size += 20;
    }

    self->marks[self->num_marks++] = len;

    return 0;
}

static int
load_reduce(Unpicklerobject *self) {
    PyObject *callable = 0, *arg_tup = 0, *ob = 0;
    int len, res = -1;

    if ((len = PyList_Size(self->stack)) < 0)
        goto finally;

    UNLESS(arg_tup = PyList_GetItem(self->stack, len - 1))
        goto finally;

    UNLESS(callable = PyList_GetItem(self->stack, len - 2))
        goto finally;

    UNLESS(ob = Instance_New(callable, arg_tup))
        goto finally;

    if (PyList_Append(self->stack, ob) < 0)
        goto finally;

    if (DEL_LIST_SLICE(self->stack, len - 2, len) < 0)
        goto finally;

    res = 0;

finally:
    Py_XDECREF(ob);

    return res;
}
    
static PyObject *
load(Unpicklerobject *self) {
    PyObject *stack = 0, *err = 0, *val = 0;
    int len;
    char *s;

    UNLESS(stack = PyList_New(0))
        goto err;

    self->stack = stack;
    self->num_marks = 0;

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

#ifdef FORMAT_1_3
            case BINFLOAT:
                if (load_binfloat(self) < 0)
                    break;
                continue;
#endif

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

            default: 
                PyErr_Format(UnpicklingError, "invalid load key, '%s'.", 
                    "c", s[0]);
                goto err;
        }

        break;
    }

    if ((err = PyErr_Occurred()) == PyExc_EOFError) {
        PyErr_SetNone(PyExc_EOFError);
        goto err;
    }    

    if (err) goto err;

    if ((len = PyList_Size(stack)) < 0) goto err;

    UNLESS(val = PyList_GetItem(stack, len - 1)) goto err;
    Py_INCREF(val);

    Py_DECREF(stack);

    self->stack=NULL;
    return val;

err:
    self->stack=NULL;
    Py_XDECREF(stack);

    return NULL;
}
    

/* No-load functions to support noload, which is used to
   find persistent references. */

static int
noload_obj(Unpicklerobject *self) {
    int i, len;

    if ((i = marker(self)) < 0) return -1;
    if ((len = PyList_Size(self->stack)) < 0) return -1;
    return DEL_LIST_SLICE(self->stack, i+1, len);
}


static int
noload_inst(Unpicklerobject *self) {
    int i, j;
    char *s;

    if ((i = marker(self)) < 0) return -1;
    if ((j = PyList_Size(self->stack)) < 0) return -1;
    if (DEL_LIST_SLICE(self->stack, i, j) < 0) return -1;
    if ((*self->readline_func)(self, &s) < 0) return -1;
    if ((*self->readline_func)(self, &s) < 0) return -1;
    return PyList_Append(self->stack, Py_None);
}

static int
noload_global(Unpicklerobject *self) {
    char *s;

    if ((*self->readline_func)(self, &s) < 0) return -1;
    if ((*self->readline_func)(self, &s) < 0) return -1;
    return PyList_Append(self->stack, Py_None);
}

static int
noload_reduce(Unpicklerobject *self) {
    int len;

    if ((len = PyList_Size(self->stack)) < 0) return -1;
    if (DEL_LIST_SLICE(self->stack, len - 2, len) < 0) return -1;
    return PyList_Append(self->stack, Py_None);
}

static int
noload_build(Unpicklerobject *self) {
  int len;

  if ((len = PyList_Size(self->stack)) < 0) return -1;
  return DEL_LIST_SLICE(self->stack, len - 1, len);
}


static PyObject *
noload(Unpicklerobject *self) {
    PyObject *stack = 0, *err = 0, *val = 0;
    int len;
    char *s;

    UNLESS(stack = PyList_New(0))
        goto err;

    self->stack = stack;
    self->num_marks = 0;

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
                PyErr_Format(UnpicklingError, "invalid load key, '%s'.", 
                    "c", s[0]);
                goto err;
        }

        break;
    }

    if ((err = PyErr_Occurred()) == PyExc_EOFError) {
        PyErr_SetNone(PyExc_EOFError);
        goto err;
    }    

    if (err) goto err;

    if ((len = PyList_Size(stack)) < 0) goto err;

    UNLESS(val = PyList_GetItem(stack, len - 1)) goto err;
    Py_INCREF(val);

    Py_DECREF(stack);

    self->stack=NULL;
    return val;

err:
    self->stack=NULL;
    Py_XDECREF(stack);

    return NULL;
}
    

static PyObject *
Unpickler_load(Unpicklerobject *self, PyObject *args) {
    UNLESS(PyArg_ParseTuple(args, "")) 
        return NULL;

    return load(self);
}

static PyObject *
Unpickler_noload(Unpicklerobject *self, PyObject *args) {
    UNLESS(PyArg_ParseTuple(args, "")) 
        return NULL;

    return noload(self);
}


static struct PyMethodDef Unpickler_methods[] = {
  {"load",         (PyCFunction)Unpickler_load,   1,
   "load() -- Load a pickle"
  },
  {"noload",         (PyCFunction)Unpickler_noload,   1,
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

    UNLESS(self = PyObject_NEW(Unpicklerobject, &Unpicklertype))
        return NULL;

    self->file = NULL;
    self->arg = NULL;
    self->stack = NULL;
    self->pers_func = NULL;
    self->last_string = NULL;
    self->marks = NULL;
    self->num_marks = 0;
    self->marks_size = 0;
    self->buf_size = 0;
    self->read = NULL;
    self->readline = NULL;    

    UNLESS(self->memo = PyDict_New()) {
       Py_XDECREF((PyObject *)self);
       return NULL;
    }

    Py_INCREF(f);
    self->file = f;

    /* Set read, readline based on type of f */
    if (PyFile_Check(f)) {
        self->fp = PyFile_AsFile(f);
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

        UNLESS((self->readline = PyObject_GetAttr(f, readline_str)) &&
            (self->read = PyObject_GetAttr(f, read_str))) {
            PyErr_Clear();
            PyErr_SetString( PyExc_TypeError, "argument must have 'read' and "
                "'readline' attributes" );
	    goto err;
        }
    }

    if(PyEval_GetRestricted()) {
	/* Restricted execution, get private tables */
	PyObject *m;

	UNLESS(self->class_map=PyDict_New()) goto err;
	UNLESS(m=PyImport_Import(copy_reg_str)) goto err;
	self->safe_constructors=PyObject_GetAttr(m, safe_constructors_str);
	Py_DECREF(m);
	UNLESS(self->safe_constructors) goto err;
    }
    else {
	self->class_map=class_map;
	Py_INCREF(class_map);
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
  
    UNLESS(PyArg_ParseTuple(args, "O", &file))
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
    Py_XDECREF(self->class_map);
    Py_XDECREF(self->safe_constructors);

    if (self->marks) {
        free(self->marks);
    }

    if (self->buf_size) {
        free(self->buf);
    }
    
    PyMem_DEL(self);
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

    if (!strcmp(name, "memo")) {
        if (!self->memo) {
            PyErr_SetString(PyExc_AttributeError, name);
            return NULL;
        }

        Py_INCREF(self->memo);
        return self->memo;
    }

    if (!strcmp(name, "stack")) {
        if (!self->stack) {
            PyErr_SetString(PyExc_AttributeError, name);
            return NULL;
        }

        Py_INCREF(self->stack);
        return self->stack;
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

    UNLESS(PyArg_ParseTuple(args, "OO|i", &ob, &file, &bin))
        goto finally;

    UNLESS(pickler = newPicklerobject(file, bin))
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

    UNLESS(PyArg_ParseTuple(args, "O|i", &ob, &bin))
        goto finally;

    UNLESS(file = PycStringIO->NewOutput(128))
        goto finally;

    UNLESS(pickler = newPicklerobject(file, bin))
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

    UNLESS(PyArg_ParseTuple(args, "O", &ob))
        goto finally;

    UNLESS(unpickler = newUnpicklerobject(ob))
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

    UNLESS(PyArg_ParseTuple(args, "O", &ob))
        goto finally;

    UNLESS(file = PycStringIO->NewInput(ob))
        goto finally;
  
    UNLESS(unpickler = newUnpicklerobject(file))
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
    "Unpickler",                  /*tp_name*/
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
  {"dump",         (PyCFunction)cpm_dump,         1,
   "dump(object, file, [binary]) --"
   "Write an object in pickle format to the given file\n"
   "\n"
   "If the optional argument, binary, is provided and is true, then the\n"
   "pickle will be written in binary format, which is more space and\n"
   "computationally efficient. \n"
  },
  {"dumps",        (PyCFunction)cpm_dumps,        1,
   "dumps(object, [binary]) --"
   "Return a string containing an object in pickle format\n"
   "\n"
   "If the optional argument, binary, is provided and is true, then the\n"
   "pickle will be written in binary format, which is more space and\n"
   "computationally efficient. \n"
  },
  {"load",         (PyCFunction)cpm_load,         1,
   "load(file) -- Load a pickle from the given file"},
  {"loads",        (PyCFunction)cpm_loads,        1,
   "loads(string) -- Load a pickle from the given string"},
  {"Pickler",      (PyCFunction)get_Pickler,      1,
   "Pickler(file, [binary]) -- Create a pickler\n"
   "\n"
   "If the optional argument, binary, is provided and is true, then\n"
   "pickles will be written in binary format, which is more space and\n"
   "computationally efficient. \n"
  },
  {"Unpickler",    (PyCFunction)get_Unpickler,    1,
   "Unpickler(file) -- Create an unpickler"},
  { NULL, NULL }
};


#define CHECK_FOR_ERRORS(MESS) \
if(PyErr_Occurred()) { \
    PyObject *__sys_exc_type, *__sys_exc_value, *__sys_exc_traceback; \
    PyErr_Fetch( &__sys_exc_type, &__sys_exc_value, &__sys_exc_traceback); \
    fprintf(stderr, # MESS ":\n\t"); \
    PyObject_Print(__sys_exc_type, stderr,0); \
    fprintf(stderr,", "); \
    PyObject_Print(__sys_exc_value, stderr,0); \
    fprintf(stderr,"\n"); \
    fflush(stderr); \
    Py_FatalError(# MESS); \
}


static int
init_stuff(PyObject *module, PyObject *module_dict) {
    PyObject *string, *copy_reg;

#define INIT_STR(S) UNLESS(S ## _str=PyString_FromString(#S)) return -1;

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

    UNLESS(copy_reg = PyImport_ImportModule("copy_reg"))
        return -1;

    /* These next few are special because we want to use different
       ones in restricted mode. */

    UNLESS(dispatch_table = PyObject_GetAttr(copy_reg, dispatch_table_str))
        return -1;

    UNLESS(safe_constructors = PyObject_GetAttr(copy_reg,
						safe_constructors_str))
        return -1;

    Py_DECREF(copy_reg);

    UNLESS(class_map = PyDict_New()) return -1;

    /* Down to here ********************************** */

    UNLESS(string = PyImport_ImportModule("string"))
        return -1;

    UNLESS(atol_func = PyObject_GetAttrString(string, "atol"))
        return -1;

    Py_DECREF(string);

    UNLESS(empty_tuple = PyTuple_New(0))
        return -1;

    UNLESS(PicklingError = PyString_FromString("cPickle.PicklingError"))
        return -1;

    if (PyDict_SetItemString(module_dict, "PicklingError", 
        PicklingError) < 0)
        return -1;

    UNLESS(UnpicklingError = PyString_FromString("cPickle.UnpicklingError"))
        return -1;

    if (PyDict_SetItemString(module_dict, "UnpicklingError",
        UnpicklingError) < 0)
        return -1;

    PycString_IMPORT;
 
    return 0;
}

void
initcPickle() {
    PyObject *m, *d, *v;
    char *rev="1.48";
    PyObject *format_version;
    PyObject *compatible_formats;

    Picklertype.ob_type = &PyType_Type;
    Unpicklertype.ob_type = &PyType_Type;

    /* Create the module and add the functions */
    m = Py_InitModule4("cPickle", cPickle_methods,
                     cPickle_module_documentation,
                     (PyObject*)NULL,PYTHON_API_VERSION);

    /* Add some symbolic constants to the module */
    d = PyModule_GetDict(m);
    PyDict_SetItemString(d,"__version__", v = PyString_FromString(rev));
    Py_XDECREF(v);

#ifdef FORMAT_1_3
    format_version = PyString_FromString("1.3");
    compatible_formats = Py_BuildValue("[sss]", "1.0", "1.1", "1.2");
#else
    format_version = PyString_FromString("1.2");
    compatible_formats = Py_BuildValue("[ss]", "1.0", "1.1");
#endif

    PyDict_SetItemString(d, "format_version", format_version);
    PyDict_SetItemString(d, "compatible_formats", compatible_formats);
    Py_XDECREF(format_version);
    Py_XDECREF(compatible_formats);

    init_stuff(m, d);
    CHECK_FOR_ERRORS("can't initialize module cPickle");
}
