/*
     $Id$

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
""
;

#include "Python.h"
#include "cStringIO.h"
#include "graminit.h"

#include <errno.h>

static PyObject *ErrorObject;

#ifdef __cplusplus
#define ARG(T, N) T N
#define ARGDECL(T, N)
#else
#define ARG(T, N) N
#define ARGDECL(T, N) T N;
#endif

#define UNLESS(E) if (!(E))
#define ASSIGN(V,E) {PyObject *__e; __e=(E); Py_XDECREF(V); (V)=__e;}
#define UNLESS_ASSIGN(V,E) ASSIGN(V,E) UNLESS(V)

#define DEL_LIST_SLICE(list, from, to) \
        (PyList_SetSlice(list, from, to, empty_list))

#define APPEND    'a'
#define BUILD     'b'
#define DUP       '2'
#define GET       'g'
#define BINGET    'h'
#define INST      'i'
#define OBJ       'o'
#define MARK      '('
static char MARKv = MARK;
#define PUT       'p'
#define BINPUT    'q'
#define POP       '0'
#define SETITEM   's'
#define STOP      '.'
#define CLASS     'c'
#define DICT      'd'
#define LIST      'l'
#define TUPLE     't'
#define NONE      'N'
#define INT       'I'
#define BININT    'J'
#define BININT1   'K'
#define BININT2   'M'
#define BININT3   'O'
#define LONG      'L'
#define FLOAT     'F'
#define STRING    'S'
#define BINSTRING 'T'
#define SHORT_BINSTRING 'U'
#define PERSID    'P'


PyTypeObject *BuiltinFunctionType;

/* atol function from string module */
static PyObject *atol_func;

static PyObject *PicklingError;
static PyObject *UnpicklingError;

static PyObject *class_map;
static PyObject *empty_list, *empty_tuple;

static PyObject *save();


typedef struct
{
  PyObject_HEAD
  FILE *fp;
  PyObject *write;
  PyObject *file;
  PyObject *memo;
  PyObject *arg;
  PyObject *pers_func;
  char *mark;
  int bin;
  int (*write_func)();
} Picklerobject;

staticforward PyTypeObject Picklertype;


typedef struct
{
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
  int *marks;
  int num_marks;
  int marks_size;
  int (*read_func)();
  int (*readline_func)();
} Unpicklerobject;
 
staticforward PyTypeObject Unpicklertype;


static int 
write_file(ARG(Picklerobject *, self), ARG(char *, s), ARG(int, n))
    ARGDECL(Picklerobject *, self)
    ARGDECL(char *, s)
    ARGDECL(int, n)
{
  if (fwrite(s, sizeof(char), n, self->fp) != n)
  {
    PyErr_SetFromErrno(PyExc_IOError);
    return -1;
  }

  return n;
}


static int 
write_cStringIO(ARG(Picklerobject *, self), ARG(char *, s), ARG(int, n))
    ARGDECL(Picklerobject *, self)
    ARGDECL(char *, s)
    ARGDECL(int, n)
{
  if ((*PycStringIO_cwrite)((PyObject *)self->file, s, n) != n)
  {
    return -1;
  }

  return n;
}


static int 
write_other(ARG(Picklerobject *, self), ARG(char *, s), ARG(int, n))
    ARGDECL(Picklerobject *, self)
    ARGDECL(char *, s)
    ARGDECL(int, n)
{
  PyObject *py_str, *junk;

  UNLESS(py_str = PyString_FromStringAndSize(s, n))
    return -1;

  UNLESS(self->arg)
    UNLESS(self->arg = PyTuple_New(1))
    {
      Py_DECREF(py_str);
      return -1;
    }

  if (PyTuple_SetItem(self->arg, 0, py_str) == -1)
  {
    Py_DECREF(py_str);
    return -1;
  }

  Py_INCREF(py_str);
  UNLESS(junk = PyObject_CallObject(self->write, self->arg))
  {
    Py_DECREF(py_str);
    return -1;
  }

  Py_DECREF(junk);

  return n;
}


static int 
read_file(ARG(Unpicklerobject *, self), ARG(char **, s), ARG(int, n))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(char **, s)
    ARGDECL(int, n)
{
  if (fread(*s, sizeof(char), n, self->fp) != n)
  {  
    if (feof(self->fp))
    {
      PyErr_SetNone(PyExc_EOFError);
      return -1;
    }

    PyErr_SetFromErrno(PyExc_IOError);
    return -1;
  }

  return n;
}


static int 
readline_file(ARG(Unpicklerobject *, self), ARG(char **, s))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(char **, s)
{
  int size, i;
  char *str;

  UNLESS(str = (char *)malloc(100))
  {
    PyErr_SetString(PyExc_MemoryError, "out of memory");
    return -1;
  }

  size = 100;
  i = 0;

  while (1)
  {
    for (; i < size; i++)
    {
      if (feof(self->fp) || (str[i] = getc(self->fp)) == '\n')
      {
        str[i] = 0;
        *s = str;
        return i;
      }
    }

    UNLESS(str = realloc(str, size += 100))
    {
      PyErr_SetString(PyExc_MemoryError, "out of memory");
      return -1;
    }
  }
}    


static int 
read_cStringIO(ARG(Unpicklerobject *, self), ARG(char **, s), ARG(int, n))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(char **, s)
    ARGDECL(int, n)
{
  char *ptr;

  if ((*PycStringIO_cread)((PyObject *)self->file, &ptr, n) != n)
  {
    PyErr_SetNone(PyExc_EOFError);
    return -1;
  }

  memcpy(*s, ptr, n);
  return n;
}


static int 
readline_cStringIO(ARG(Unpicklerobject *, self), ARG(char **, s))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(char **, s)
{
  int n;
  char *ptr, *str;

  if ((n = (*PycStringIO_creadline)((PyObject *)self->file, &ptr)) == -1)
  {
    return -1;
  }

  UNLESS(str = (char *)malloc(n * sizeof(char) + 1))
  {
    PyErr_SetString(PyExc_MemoryError, "out of memory");
    return -1;
  }

  memcpy(str, ptr, n);

  str[((str[n - 1] == '\n') ? --n : n)] = 0;

  *s = str;
  return n;
}


static int 
read_other(ARG(Unpicklerobject *, self), ARG(char **, s), ARG(int, n))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(char **, s)
    ARGDECL(int, n)
{
  PyObject *bytes, *str;

  UNLESS(bytes = PyInt_FromLong(n))
  {
    if (!PyErr_Occurred())
      PyErr_SetNone(PyExc_EOFError);

    return -1;
  }

  UNLESS(self->arg)
    UNLESS(self->arg = PyTuple_New(1))
    {
      Py_DECREF(bytes);
      return -1;
    }

  if (PyTuple_SetItem(self->arg, 0, bytes) == -1)
  {
    Py_DECREF(bytes);
    return -1;
  }
  Py_INCREF(bytes);

  UNLESS(str = PyObject_CallObject(self->read, self->arg))
  {
    Py_DECREF(bytes);
    return -1;
  }

  memcpy(*s, PyString_AsString(str), n);

  Py_DECREF(bytes);
  Py_DECREF(str);

  return n;
}


static int 
readline_other(ARG(Unpicklerobject *, self), ARG(char **, s))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(char **, s)
{
  PyObject *str;
  char *c_str;
  int size;

  UNLESS(str = PyObject_CallObject(self->readline, empty_tuple))
  {
    return -1;
  }

  size = PyString_Size(str);

  UNLESS(c_str = (char *)malloc((size + 1) * sizeof(char)))
  {
    PyErr_SetString(PyExc_MemoryError, "out of memory");
    return -1;
  }

  memcpy(c_str, PyString_AsString(str), size);

  if (size > 0)
  {
    c_str[((c_str[size - 1] == '\n') ? --size : size)] = 0;
  }

  *s = c_str;

  Py_DECREF(str);

  return size;
}


static int
put(ARG(Picklerobject *, self), ARG(PyObject *, ob))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, ob)
{
  char c_str[30];
  int p, len;
  PyObject *py_ob_id = 0, *memo_len = 0;

  if ((p = PyDict_Size(self->memo)) == -1)
    return -1;

  if (!self->bin || (p >= 256))
  {
    c_str[0] = PUT;
    sprintf(c_str + 1, "%d\n", p);
    len = strlen(c_str);
  }
  else
  {
    c_str[0] = BINPUT;
    c_str[1] = p;
    len = 2;
  }

  if ((*self->write_func)(self, c_str, len) == -1)
    return -1;

  UNLESS(py_ob_id = PyInt_FromLong((long)ob))
    return -1;

  UNLESS(memo_len = PyInt_FromLong(p))
    goto err;

  if (PyDict_SetItem(self->memo, py_ob_id, memo_len) == -1)
    goto err;

  Py_DECREF(py_ob_id);
  Py_DECREF(memo_len);

  return 1;

err:
  Py_XDECREF(py_ob_id);
  Py_XDECREF(memo_len);

  return -1;
}


static int
safe(ARG(PyObject *, ob))
    ARGDECL(PyObject *, ob)
{
  PyTypeObject *type;
  PyObject *this_item;
  int len, res, i;
  
  type = ob->ob_type;

  if (type == &PyInt_Type    || 
      type == &PyFloat_Type  || 
      type == &PyString_Type ||
      ob == Py_None)
  {
    return 1;
  }

  if (type == &PyTuple_Type)
  {
    len = PyTuple_Size(ob);
    for (i = 0; i < len; i++)
    {
      UNLESS(this_item = PyTuple_GET_ITEM((PyTupleObject *)ob, i))
        return -1;
    
      if ((res = safe(this_item)) == 1)
        continue;

      return res;
    }

    return 1;
  }

  return 0;
}


static PyObject *
whichmodule(ARG(PyObject *, class))
    ARGDECL(PyObject *, class)
{
  int has_key, i, j;
  PyObject *module = 0, *modules_dict = 0, *class_name = 0, *class_name_attr = 0, 
      *name = 0;
  char *name_str, *class_name_str;

  static PyObject *__main__str;

  if ((has_key = PyMapping_HasKey(class_map, class)) == -1)
    return NULL;

  if (has_key)
  {
    return ((module = PyDict_GetItem(class_map, class)) ? module : NULL);
  }

  UNLESS(modules_dict = PySys_GetObject("modules"))
    return NULL;

  UNLESS(class_name = ((PyClassObject *)class)->cl_name)
  {
    PyErr_SetString(PicklingError, "class has no name");
    return NULL;
  }

  UNLESS(class_name_str = PyString_AsString(class_name))
    return NULL;

  i = 0;
  while ((j = PyDict_Next(modules_dict, &i, &name, &module)))
  {
    UNLESS(name_str = PyString_AsString(name))
      return NULL;

    if (!strcmp(name_str, "__main__"))
      continue;

    UNLESS(class_name_attr = PyObject_GetAttr(module, class_name))
    {
      PyErr_Clear();
      continue;
    }

    if (class_name_attr != class)
    {
      Py_DECREF(class_name_attr);
      continue;
    }

    Py_DECREF(class_name_attr);

    break;
  }
    
  if (!j) /* previous while exited normally */
  {
    UNLESS(__main__str)
      UNLESS(__main__str = PyString_FromString("__main__"))
        return NULL;

    name = __main__str;
  }
  else    /* previous while exited via break */
  {
    Py_INCREF(name);
  }
  
  PyDict_SetItem(class_map, class, name);

  return name;
}


static PyObject *
save_none(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  static char none[] = { NONE };

  if ((*self->write_func)(self, none, 1) == -1)  
    return NULL;

  Py_INCREF(Py_None);
  return Py_None;
}

      
static PyObject *
save_int(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  char c_str[25];
  long l = PyInt_AS_LONG((PyIntObject *)args);
  int len;

  if (!self->bin)
  {
    c_str[0] = INT;
    sprintf(c_str + 1, "%ld\n", l);
    if ((*self->write_func)(self, c_str, strlen(c_str)) == -1)
      return NULL;
  }
  else
  {
    c_str[1] = (int)(l & 0xff);
    c_str[2] = (int)((l >> 8)  & 0xff);
    c_str[3] = (int)((l >> 16) & 0xff);
    c_str[4] = (int)((l >> 24) & 0xff);

    if (!c_str[4])
    {
      if (!c_str[3])
      {
        if (!c_str[2])
        {
          c_str[0] = BININT3;
          len = 2;
        }
        else
        {
          c_str[0] = BININT2;
          len = 3;
        }
      }
      else 
      {
        c_str[0] = BININT1;
        len = 4;
      }
    }
    else
    {
      c_str[0] = BININT;
      len = 5;
    }

    if ((*self->write_func)(self, c_str, len) == -1)
      return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
save_long(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  char *c_str;
  int size;
  PyObject *repr = 0;

  UNLESS(repr = PyObject_Repr(args))
    return NULL;

  if ((size = PyString_Size(repr)) == -1)
  {
    Py_DECREF(repr);
    return NULL;
  }

  UNLESS(c_str = (char *)malloc((size + 2) * sizeof(char)))
  {
    Py_DECREF(repr);
    PyErr_SetString(PyExc_MemoryError, "out of memory");
    return NULL;
  }

  c_str[0] = LONG;
  memcpy(c_str + 1, PyString_AS_STRING((PyStringObject *)repr), size);
  c_str[size + 1] = '\n';

  Py_DECREF(repr);

  if ((*self->write_func)(self, c_str, size + 2) == -1)
  {
    free(c_str);
    return NULL;
  }

  free(c_str);

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
save_float(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  char c_str[250];

  c_str[0] = FLOAT;
  sprintf(c_str + 1, "%f\n", PyFloat_AS_DOUBLE((PyFloatObject *)args));

  if ((*self->write_func)(self, c_str, strlen(c_str)) == -1)
    return NULL;

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
save_string(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  char *c_str;
  int size, len;

  if (!self->bin)
  {
    PyObject *repr;
    char *repr_str;

    UNLESS(repr = PyObject_Repr(args))
      return NULL;

    repr_str = PyString_AS_STRING((PyStringObject *)repr);
    size = PyString_Size(repr);

    UNLESS(c_str = (char *)malloc((size + 2) * sizeof(char)))
    {
      PyErr_SetString(PyExc_MemoryError, "out of memory");
      return NULL;
    }
   
    c_str[0] = STRING;
    memcpy(c_str + 1, repr_str, size);
    c_str[size + 1] = '\n';

    len = size + 2;

    Py_XDECREF(repr);
  }
  else
  {
    size = PyString_Size(args);

    UNLESS(c_str = (char *)malloc((size + 30) * sizeof(char)))
    {
      PyErr_SetString(PyExc_MemoryError, "out of memory");
      return NULL;
    }

    if (size < 256)
    {
      c_str[0] = SHORT_BINSTRING;
      c_str[1] = size;
      len = 2;
    }
    else  
    {
      c_str[0] = BINSTRING;
      sprintf(c_str + 1, "%d\n", size);
      len = strlen(c_str);
    }

    memcpy(c_str + len, PyString_AS_STRING((PyStringObject *)args), size);

    len += size;
  }

  if ((*self->write_func)(self, c_str, len) == -1)
  {
    free(c_str);
    return NULL;
  }

  free(c_str);

  if (args->ob_refcnt > 1)
  {
    if (put(self, args) == -1)
    {
      return NULL;
    }
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
save_tuple(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *element = 0, *junk = 0, *py_tuple_id = 0;
  int len, i, dict_size;

  static char tuple = TUPLE;

  if ((*self->write_func)(self, &MARKv, 1) == -1)
    return NULL;

  UNLESS(py_tuple_id = PyInt_FromLong((long)args))  return NULL;

  if ((len = PyTuple_Size(args)) == -1)  
    goto err;

  for (i = 0; i < len; i++)
  {
    UNLESS(element = PyTuple_GET_ITEM((PyTupleObject *)args, i))  
      goto err;
    
    dict_size = PyDict_Size(self->memo);

    UNLESS(junk = save(self, element))
      goto err;
    Py_DECREF(junk);

    if (((PyDict_Size(self->memo) - dict_size) > 1)  && 
        PyMapping_HasKey(self->memo, py_tuple_id))
    {
      char c_str[30];
      long c_value;
      int c_str_len;
      PyObject *value;
      static char pop = POP;

      while (i-- >= 0)
      {
        if ((*self->write_func)(self, &pop, 1) == -1)
          goto err;
      }

      UNLESS(value = PyDict_GetItem(self->memo, py_tuple_id))
        goto err;

      c_value = PyInt_AsLong(value);

      if (self->bin && (c_value < 256))
      {
        c_str[0] = BINGET;
        c_str[1] = c_value;
        c_str_len = 2;
      }
      else
      {
        c_str[0] = GET;
        sprintf(c_str + 1, "%ld\n", c_value);
        c_str_len = strlen(c_str);
      }

      if ((*self->write_func)(self, c_str, c_str_len) == -1)
        goto err;

      break;
    }
  }

  if (i >= len)
  {
    if ((*self->write_func)(self, &tuple, 1) == -1)
      goto err;
    
    if (args->ob_refcnt > 1)
    {
      if (put(self, args) == -1)
      {
        goto err;
      }
    }
  }
 
  Py_DECREF(py_tuple_id);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(py_tuple_id);

  return NULL;
}


static PyObject *
save_list(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *element = 0, *junk = 0;
  int len, i, safe_val;
  static char append = APPEND, list = LIST;

  if ((*self->write_func)(self, &MARKv, 1) == -1)
    return NULL;

  if ((len = PyList_Size(args)) == -1)
    return NULL;

  for (i = 0; i < len; i++)
  {
    UNLESS(element = PyList_GET_ITEM((PyListObject *)args, i))  
      return NULL;

    if ((safe_val = safe(element)) == -1)  
      return NULL;
    UNLESS(safe_val)
      break;

    UNLESS(junk = save(self, element))  
      return NULL;
    Py_DECREF(junk);
  }

  if (args->ob_refcnt > 1)
  {
    if (put(self, args) == -1)
    {
      return NULL;
    }
  }

  if ((*self->write_func)(self, &list, 1) == -1)
    return NULL;

  for (; i < len; i++)
  {
    UNLESS(element = PyList_GET_ITEM((PyListObject *)args, i))  
      return NULL;

    UNLESS(junk = save(self, element))  
      return NULL;
    Py_DECREF(junk);

    if ((*self->write_func)(self, &append, 1) == -1)
      return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
save_dict(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *key = 0, *value = 0, *junk = 0;
  int i, safe_key, safe_value;

  static char setitem = SETITEM, dict = DICT;

  if ((*self->write_func)(self, &MARKv, 1) == -1)
    return NULL;

  i = 0;
  
  while (PyDict_Next(args, &i, &key, &value))
  {
    if ((safe_key = safe(key)) == -1)  
      return NULL;

    UNLESS(safe_key)  
      break;
   
    if ((safe_value = safe(value)) == -1)  
      return NULL;
   
    UNLESS(safe_value)  
      break;

    UNLESS(junk = save(self, key))
      return NULL;
    Py_DECREF(junk);

    UNLESS(junk = save(self, value))
      return NULL;
    Py_DECREF(junk);
  }

  if ((*self->write_func)(self, &dict, 1) == -1)
    return NULL;

  if (args->ob_refcnt > 1)
  {
    if (put(self, args) == -1)
    {
      return NULL;
    }
  }

  while (PyDict_Next(args, &i, &key, &value))
  {
    UNLESS(junk = save(self, key))
      return NULL;
    Py_DECREF(junk);

    UNLESS(junk = save(self, value))
      return NULL;
    Py_DECREF(junk);

    if ((*self->write_func)(self, &setitem, 1) == -1)
      return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *  
save_inst(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *class = 0, *module = 0, *name = 0,
           *junk = 0, *state = 0, *getinitargs_func = 0, *getstate_func = 0;
  char *module_str, *name_str, *c_str;
  int module_size, name_size, size;
  static char build = BUILD;

  if ((*self->write_func)(self, &MARKv, 1) == -1)
    return NULL;

  UNLESS(class = PyObject_GetAttrString(args, "__class__"))
    return NULL;

  if (self->bin)
  {
    UNLESS(junk = save(self, class))
      goto err;
    Py_DECREF(junk);
  }

  if ((getinitargs_func = PyObject_GetAttrString(args, "__getinitargs__")))
  {
    PyObject *class_args = 0, *element = 0;
    int i, len;

    UNLESS(class_args = PyObject_CallObject(getinitargs_func, empty_tuple))
    {
      Py_DECREF(getinitargs_func);
      goto err;
    }

    if ((len = PyObject_Length(class_args)) == -1)  
    {
      Py_DECREF(class_args);
      goto err;
    }

    for (i = 0; i < len; i++)
    {
      UNLESS(element = PySequence_GetItem(class_args, i)) 
      { 
        Py_DECREF(class_args); 
        goto err;
      }

      UNLESS(junk = save(self, element))
      {
        Py_DECREF(element); Py_DECREF(class_args);
        goto err;
      }

      Py_DECREF(junk);
      Py_DECREF(element);
    }

    Py_DECREF(class_args);
  }
  else
  {
    PyErr_Clear();
  }

  if (!self->bin)
  {
    UNLESS(module = whichmodule(class))
      goto err;
    
    UNLESS(name = ((PyClassObject *)class)->cl_name)
    {
      PyErr_SetString(PicklingError, "class has no name");
      goto err;
    }

    module_str = PyString_AS_STRING((PyStringObject *)module);
    module_size = PyString_Size(module);
    name_str   = PyString_AS_STRING((PyStringObject *)name);
    name_size = PyString_Size(name);

    size = name_size + module_size + 3;

    UNLESS(c_str = (char *)malloc(size * sizeof(char)))
    {
      PyErr_SetString(PyExc_MemoryError, "out of memory");
      return NULL;
    }

    c_str[0] = INST;
    memcpy(c_str + 1, module_str, module_size);
    c_str[module_size + 1] = '\n';
    memcpy(c_str + module_size + 2, name_str, name_size);
    c_str[module_size + name_size + 2] = '\n';

    if ((*self->write_func)(self, c_str, size) == -1)
    {
      free(c_str);
      goto err;
                                                                                                                                                                      }

    free(c_str);
  }

  if (args->ob_refcnt > 1)
  {
    if (put(self, args) == -1)
    {
      goto err;
    }
  }

  if ((getstate_func = PyObject_GetAttrString(args, "__getstate__")))
  {
    UNLESS(state = PyObject_CallObject(getstate_func, empty_tuple))
    {
      Py_DECREF(getstate_func);
      goto err;
    }
  }
  else
  {
    PyErr_Clear();

    UNLESS(state = PyObject_GetAttrString(args, "__dict__"))
      goto err;
  }

  UNLESS(junk = save(self, state))
    goto err;
  Py_DECREF(junk);

  Py_XDECREF(getinitargs_func);
  Py_XDECREF(getstate_func);
  Py_XDECREF(module);
  Py_XDECREF(state);

  if ((*self->write_func)(self, &build, 1) == -1)
    return NULL;

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(getinitargs_func);
  Py_XDECREF(getstate_func);
  Py_XDECREF(module);
  Py_XDECREF(state);

  return NULL;
}


static PyObject *
save_class(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *module = 0, *name = 0;
  char *name_str, *module_str, *c_str;
  int module_size, name_size, size;

  UNLESS(module = whichmodule(args))
    return NULL;

  UNLESS(name = ((PyClassObject *)args)->cl_name)
  {
    PyErr_SetString(PicklingError, "class has no name");
    goto err;
  }

  module_str = PyString_AS_STRING((PyStringObject *)module);
  module_size = PyString_Size(module);
  name_str   = PyString_AS_STRING((PyStringObject *)name);
  name_size = PyString_Size(name);

  size = name_size + module_size + 3;

  UNLESS(c_str = (char *)malloc(size * sizeof(char)))
  {
    PyErr_SetString(PyExc_MemoryError, "out of memory");
    return NULL;
  }

  c_str[0] = CLASS;
  memcpy(c_str + 1, module_str, module_size);
  c_str[module_size + 1] = '\n';
  memcpy(c_str + module_size + 2, name_str, name_size);
  c_str[module_size + name_size + 2] = '\n';

  if ((*self->write_func)(self, c_str, size) == -1)
  {
    free(c_str);
    goto err;
  }
 
  free(c_str);

  if (args->ob_refcnt > 1)
  {
    if (put(self, args) == -1)
    {
      goto err;
    }
  }

  Py_DECREF(module);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(module);

  return NULL;
}


static PyObject *
save(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyTypeObject *type;
  char *error_str, *name_c;
  PyObject *name, *name_repr;

  if (self->pers_func)
  {
    PyObject *pid;
    int size;
    
    UNLESS(self->arg)
      UNLESS(self->arg = PyTuple_New(1))
        return NULL;
    
    if (PyTuple_SetItem(self->arg, 0, args) == -1)
      return NULL;
    Py_INCREF(args);
    
    UNLESS(pid = PyObject_CallObject(self->pers_func, self->arg))
      return NULL;

    if (pid != Py_None)
    {
      char *pid_str;

      if ((size = PyString_Size(pid)) == -1)
      {
        Py_DECREF(pid);
        return NULL;
      }

      UNLESS(pid_str = (char *)malloc((2 + size) * sizeof(char)))
      {
        PyErr_SetString(PyExc_MemoryError, "out of memory");
        Py_DECREF(pid);
        return NULL;
      }

      pid_str[0] = PERSID;
      memcpy(pid_str + 1, PyString_AS_STRING((PyStringObject *)pid), size);
      pid_str[size + 1] = '\n';

      Py_DECREF(pid);
  
      if ((*self->write_func)(self, pid_str, size + 2) == -1)
      {
        free(pid_str);
        return NULL;
      }

      free(pid_str);

      Py_INCREF(Py_None);
      return Py_None;
    }

    Py_DECREF(pid);
  }

  if (args == Py_None)
  {
    return save_none(self, args);
  }

  type = args->ob_type;

  if (type == &PyInt_Type)
  {
    return save_int(self, args);
  }

  if (type == &PyLong_Type)
  {
    return save_long(self, args);
  }

  if (type == &PyFloat_Type)
  {
    return save_float(self, args);
  }

  if (args->ob_refcnt > 1)
  {
    long ob_id;
    int  has_key;
    PyObject *py_ob_id;

    ob_id = (long)args;
  
    UNLESS(py_ob_id = PyInt_FromLong(ob_id))
      return NULL;

    if ((has_key = PyMapping_HasKey(self->memo, py_ob_id)) == -1)
    {
      Py_DECREF(py_ob_id);
      return NULL;
    }

    if (has_key)
    {
      PyObject *value;
      long c_value;
      char get_str[30];
      int len;

      UNLESS(value = PyDict_GetItem(self->memo, py_ob_id))
      {
        Py_DECREF(py_ob_id);
        return NULL;
      }

      Py_DECREF(py_ob_id);

      c_value = PyInt_AsLong(value);

      if (self->bin && (c_value < 256))
      {
        get_str[0] = BINGET;
        get_str[1] = c_value;
        len = 2;
      }
      else
      {
        get_str[0] = GET;
        sprintf(get_str + 1, "%ld\n", c_value);
        len = strlen(get_str);
      }

      if ((*self->write_func)(self, get_str, len) == -1)
        return NULL;

      Py_INCREF(Py_None);
      return Py_None;
    }

    Py_DECREF(py_ob_id);
  }

  if (type == &PyString_Type)
  {
    return save_string(self, args);
  }

  if (type == &PyTuple_Type)
  {
    return save_tuple(self, args);
  }

  if (type == &PyList_Type)
  {
    return save_list(self, args);
  }

  if (type == &PyDict_Type)
  {
    return save_dict(self, args);
  }

  if (type == &PyInstance_Type)
  {
    return save_inst(self, args);
  }

  if (type == &PyClass_Type)
  {
    return save_class(self, args);
  }

  if (PyObject_HasAttrString(args, "__class__"))
  {
    return save_inst(self, args);
  }

  UNLESS(name = PyObject_GetAttrString((PyObject *)type, "__name__"))
    return NULL;

  UNLESS(name_repr = PyObject_Repr(name))
  {
    Py_DECREF(name);
  }

  name_c = PyString_AsString(name_repr);

  UNLESS(error_str = (char *)malloc((strlen(name_c) + 25) * sizeof(char)))
  {
    PyErr_SetString(PyExc_MemoryError, "out of memory");
    Py_DECREF(name);
    Py_DECREF(name_repr);
    return NULL;
  }

  sprintf(error_str, "Cannot pickle %s objects.", name_c);

  Py_DECREF(name);
  Py_DECREF(name_repr);

  PyErr_SetString(PicklingError, error_str);

  free(error_str);
  
  return NULL;
}


static PyObject *
Pickler_dump(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *junk;
  static char stop = STOP;

  UNLESS(PyArg_Parse(args, "O", &args))  return NULL;

  UNLESS(junk = save(self, args))  return NULL;
  Py_DECREF(junk);

  if ((*self->write_func)(self, &stop, 1) == -1)
    return NULL;

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
write(ARG(Picklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Picklerobject *, self)
    ARGDECL(PyObject *, args)
{
  char *ptr;
  int size;

  UNLESS(ptr = PyString_AsString(args))
    return NULL;

  if ((size = PyString_Size(args)) == -1)
    return NULL;

  if ((*self->write_func)(self, ptr, size) == -1)
    return NULL;

  Py_INCREF(Py_None);
  return Py_None;
}


static struct PyMethodDef Pickler_methods[] = {
  {"save",          (PyCFunction)save,          0, ""},
  {"dump",          (PyCFunction)Pickler_dump,  0, ""},
  {"save_none",     (PyCFunction)save_none,     0, ""},
  {"save_int",      (PyCFunction)save_int,      0, ""},
  {"save_long",     (PyCFunction)save_long,     0, ""},
  {"save_float",    (PyCFunction)save_float,    0, ""},
  {"save_string",   (PyCFunction)save_string,   0, ""},
  {"save_tuple",    (PyCFunction)save_tuple,    0, ""},
  {"save_list",     (PyCFunction)save_list,     0, ""},
  {"save_dict",     (PyCFunction)save_dict,     0, ""},
  {"save_inst",     (PyCFunction)save_inst,     0, ""},
  {"save_class",    (PyCFunction)save_class,    0, ""},
  {"write",         (PyCFunction)write,         0, ""},
  {NULL,		NULL}		/* sentinel */
};


static Picklerobject *
newPicklerobject(ARG(PyObject *, file), ARG(int, bin))
    ARGDECL(PyObject *, file)
    ARGDECL(int, bin)
{
  Picklerobject *self;
  PyObject *memo = 0;

  UNLESS(memo = PyDict_New())  goto err;

  UNLESS(self = PyObject_NEW(Picklerobject, &Picklertype))  
    goto err;

  if (PyFile_Check(file))
  {
    self->fp = PyFile_AsFile(file);
    self->write_func = write_file;
    self->write = NULL;
  }
  else if (PycStringIO_OutputCheck(file))
  {
    self->fp = NULL;
    self->write_func = write_cStringIO;
    self->write = NULL;
  }
  else
  {
    PyObject *write; 

    self->fp = NULL;
    self->write_func = write_other;

    UNLESS(write = PyObject_GetAttrString(file, "write"))
      goto err;

    self->write = write;
  }

  Py_INCREF(file);

  self->file  = file;
  self->bin   = bin;
  self->memo  = memo;
  self->arg   = NULL;
  self->pers_func = NULL;

  return self;

err:
  Py_XDECREF((PyObject *)self);
  Py_XDECREF(memo);
  return NULL;
}


static PyObject *
get_Pickler(ARG(PyObject *, self), ARG(PyObject *, args))
    ARGDECL(PyObject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *file;
  int bin = 0;

  UNLESS(PyArg_ParseTuple(args, "O|i", &file, &bin))  return NULL;
  return (PyObject *)newPicklerobject(file, bin);
}


static void
Pickler_dealloc(ARG(Picklerobject *, self))
    ARGDECL(Picklerobject *, self)
{
  Py_XDECREF(self->write);
  Py_XDECREF(self->memo);
  Py_XDECREF(self->arg);
  Py_XDECREF(self->file);
  Py_XDECREF(self->pers_func);
  PyMem_DEL(self);
}


static PyObject *
Pickler_getattr(ARG(Picklerobject *, self), ARG(char *, name))
    ARGDECL(Picklerobject *, self)
    ARGDECL(char *, name)
{
  if (!strcmp(name, "persistent_id"))
  {
    if (!self->pers_func)
    {
      PyErr_SetString(PyExc_NameError, name);
      return NULL;
    }

    Py_INCREF(self->pers_func);
    return self->pers_func;
  }

  if (!strcmp(name, "memo"))
  {
    if (!self->memo)
    {
      PyErr_SetString(PyExc_NameError, name);
      return NULL;
    }

    Py_INCREF(self->memo);
    return self->memo;
  }

  if (!strcmp(name, "PicklingError"))
  {
    Py_INCREF(PicklingError);
    return PicklingError;
  }
  
  return Py_FindMethod(Pickler_methods, (PyObject *)self, name);
}


int 
Pickler_setattr(ARG(Picklerobject *, self), ARG(char *, name), ARG(PyObject *, value))
    ARGDECL(Picklerobject *, self)
    ARGDECL(char *, name)
    ARGDECL(PyObject *, value)
{
  if (!strcmp(name, "persistent_id"))
  {
    Py_XDECREF(self->pers_func);
    self->pers_func = value;
    Py_INCREF(value);
    return 0;
  }

  return -1;
}


static char Picklertype__doc__[] = "";

static PyTypeObject Picklertype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"Pickler",			/*tp_name*/
	sizeof(Picklerobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)Pickler_dealloc,	/*tp_dealloc*/
	(printfunc)0,		/*tp_print*/
	(getattrfunc)Pickler_getattr,	/*tp_getattr*/
	(setattrfunc)Pickler_setattr,	/*tp_setattr*/
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
	Picklertype__doc__ /* Documentation string */
};


static PyObject *
find_class(ARG(char *, module_name), ARG(char *, class_name))
    ARGDECL(char *, module_name)
    ARGDECL(char *, class_name)
{
  PyObject *import = 0, *class = 0, *py_module_name = 0, *py_class_name = 0,
           *t = 0;
  char *error_str;
  int has_key;

  UNLESS(py_module_name = PyString_FromString(module_name))
    return NULL;

  UNLESS(py_class_name = PyString_FromString(class_name))
    goto err;

  UNLESS(t = PyTuple_New(2))
    goto err;

  UNLESS(PyTuple_SET_ITEM((PyTupleObject *)t, 0, py_module_name))
    goto err;
  Py_INCREF(py_module_name);
  
  UNLESS(PyTuple_SET_ITEM((PyTupleObject *)t, 1, py_class_name))
    goto err;
  Py_INCREF(py_module_name);

  if ((has_key = PyMapping_HasKey(class_map, t)) == -1)
    goto err;

  if (has_key)
  {    
    UNLESS(class = PyDict_GetItem(class_map, t))
      goto err;

    Py_INCREF(class);
  
    Py_DECREF(py_module_name);
    Py_DECREF(py_class_name);
    Py_DECREF(t);

    return class;
  }

  if (!(import = PyImport_ImportModule(module_name)) ||
      !(class = PyObject_GetAttrString(import, class_name)))
  {
    UNLESS(error_str = (char *)malloc((strlen(module_name) + 
        strlen(class_name)  + 40) * sizeof(char)))
    {
      PyErr_SetString(PyExc_MemoryError, "out of memory");
      goto err;
    }

    sprintf(error_str, "Failed to import class %s from module %s",
        class_name, module_name);

    PyErr_SetString(PyExc_SystemError, error_str);
   
    free(error_str);
    goto err;
  }

  if (class->ob_type == BuiltinFunctionType)
  {
    UNLESS(error_str = (char *)malloc((strlen(module_name) + 
        strlen(class_name)  + 45) * sizeof(char)))
    {
      PyErr_SetString(PyExc_MemoryError, "out of memory");
      goto err;
    }

    sprintf(error_str, "Imported object %s from module %s is not a class",
        class_name, module_name);

    PyErr_SetString(PyExc_SystemError, error_str);
   
    free(error_str);
    goto err;
  }

  if (PyDict_SetItem(class_map, t, class) == -1)
    goto err;

  Py_DECREF(t);
  Py_DECREF(py_module_name);
  Py_DECREF(py_class_name);
  Py_DECREF(import);

  return class;

err:
  Py_XDECREF(import);
  Py_XDECREF(class);
  Py_XDECREF(t);
  Py_XDECREF(py_module_name);
  Py_XDECREF(py_class_name);
 
  return NULL;
}


int
marker(ARG(Unpicklerobject *, self))
    ARGDECL(Unpicklerobject *, self)
{
  if (!self->num_marks)
    return -1;

  return self->marks[--self->num_marks];
}

    
static PyObject *
load_none(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  if (PyList_Append(self->stack, Py_None) == -1)
    return NULL;

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
load_int(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_int = 0;
  char *s, *endptr;
  int len;
  long l;

  if ((len = (*self->readline_func)(self, &s)) == -1)
    return NULL;

  errno = 0;
  l = strtol(s, &endptr, 0);

  free(s);
  
  if (errno || strlen(endptr))
  {
    PyErr_SetString(PyExc_ValueError, "could not convert string to int");
    goto err;
  }

  UNLESS(py_int = PyInt_FromLong(l))
    goto err;

  if (PyList_Append(self->stack, py_int) == -1)
    goto err;

  Py_DECREF(py_int);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(py_int);

  return NULL;
}


static PyObject *
load_binint(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_int = 0;
  char *s;
  unsigned char c;
  long l;

  UNLESS(s = (char *)malloc(4 * sizeof(char)))
  {
   PyErr_SetString(PyExc_MemoryError, "out of memory");
    return NULL;
  }

  if ((*self->read_func)(self, &s, 4) == -1)
  {
    free(s);
    return NULL;
  }

  c = (unsigned char)s[0];
  l  = (long)c;
  c = (unsigned char)s[1];
  l |= (long)c << 8;
  c = (unsigned char)s[2];
  l |= (long)c << 16;
  c = (unsigned char)s[3];
  l |= (long)c << 24;

  free(s);

  UNLESS(py_int = PyInt_FromLong(l))
    return NULL;

  if (PyList_Append(self->stack, py_int) == -1)
  {
    Py_DECREF(py_int);
    return NULL;
  }

  Py_DECREF(py_int);
    
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
load_binint1(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_int = 0;
  char *s;
  unsigned char c;
  long l;

  UNLESS(s = (char *)malloc(3 * sizeof(char)))
  {
    PyErr_SetString(PyExc_MemoryError, "out of memory");
    return NULL;
  }

  if ((*self->read_func)(self, &s, 3) == -1)
  {
    free(s);
    return NULL;
  }

  c = (unsigned char)s[0];
  l  = (long)c;
  c = (unsigned char)s[1];
  l |= (long)c << 8;
  c = (unsigned char)s[2];
  l |= (long)c << 16;

  free(s);

  UNLESS(py_int = PyInt_FromLong(l))
    return NULL;

  if (PyList_Append(self->stack, py_int) == -1)
  {
    Py_DECREF(py_int);
    return NULL;
  }

  Py_DECREF(py_int);

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
load_binint2(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_int = 0;
  char *s;
  unsigned char c;
  long l;

  UNLESS(s = (char *)malloc(2 * sizeof(char)))
  {
    PyErr_SetString(PyExc_MemoryError, "out of memory");
    return NULL;
  }

  if ((*self->read_func)(self, &s, 2) == -1)
    return NULL;

  c = (unsigned char)s[0];
  l  = (long)c;
  c = (unsigned char)s[1];
  l |= (long)c << 8;

  free(s);

  UNLESS(py_int = PyInt_FromLong(l))
    return NULL;

  if (PyList_Append(self->stack, py_int) == -1)
  {
    Py_DECREF(py_int);
    return NULL;
  }

  Py_DECREF(py_int);
    
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
load_binint3(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_int = 0;
  char *s;
  unsigned char c;
  long l;

  UNLESS(s = (char *)malloc(sizeof(char)))
  {
    PyErr_SetString(PyExc_MemoryError, "out of memory");
    return NULL;
  }

  if ((*self->read_func)(self, &s, 1) == -1)
  {
    free(s);
    return NULL;
  }

  c = (unsigned char)s[0];
  l  = (long)c;

  free(s);

  UNLESS(py_int = PyInt_FromLong(l))
    return NULL;

  if (PyList_Append(self->stack, py_int) == -1)
  {
    Py_DECREF(py_int);
    return NULL;
  }

  Py_DECREF(py_int);
    
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
load_long(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_str = 0, *l = 0;
  char *s;
  int len;

  static PyObject *arg;

  if ((len = (*self->readline_func)(self, &s)) == -1)
    return NULL;

  UNLESS(py_str = PyString_FromStringAndSize(s, len))
  {
    free(s);
    return NULL;
  }

  free(s);

  UNLESS(arg)
  {
    UNLESS(arg = Py_BuildValue("(Oi)", py_str, 0))
      return NULL;
  }
  else
  {
    if (PyTuple_SetItem(arg, 0, py_str) == -1)
      goto err;
    Py_INCREF(py_str);
  }

  UNLESS(l = PyObject_CallObject(atol_func, arg))
    goto err;
  
  if (PyList_Append(self->stack, l) == -1)
    goto err;

  Py_DECREF(py_str);
  Py_DECREF(l);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(l);
  Py_XDECREF(py_str);

  return NULL;
}

 
static PyObject *
load_float(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_float = 0;
  char *s, *endptr;
  int len;
  double d;

  if ((len = (*self->readline_func)(self, &s)) == -1)
    return NULL;

  errno = 0;
  d = strtod(s, &endptr);

  free(s);
  
  if (errno || strlen(endptr))
  {
    PyErr_SetString(PyExc_ValueError, "could not convert string to long");
    goto err;
  }

  UNLESS(py_float = PyFloat_FromDouble(d))
    goto err;

  if (PyList_Append(self->stack, py_float) == -1)
    goto err;

  Py_DECREF(py_float);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(py_float);

  return NULL;
}


static PyObject *
load_string(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *str = 0;
  char *s;
  int len;
  static PyObject *eval_dict = 0;

  UNLESS(eval_dict)
    UNLESS(eval_dict = Py_BuildValue("{s{}}", "__builtins__"))
      return NULL;

  if ((len = (*self->readline_func)(self, &s)) == -1)
    return NULL;

  UNLESS(str = PyRun_String(s, eval_input, eval_dict, eval_dict))
  {
    free(s);
    goto err;
  }

  free(s);

  if (PyList_Append(self->stack, str) == -1)
    goto err;

  Py_DECREF(str);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(str);

  return NULL;
}


static PyObject *
load_binstring(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_string = 0;
  char *s;
  int len, i;

  if ((len = (*self->readline_func)(self, &s)) == -1)
    return NULL;

  i = atoi(s);

  if (i > len)
  {
    free(s);

    UNLESS(s = (char *)malloc(i * sizeof(char)))
    {
      PyErr_SetString(PyExc_MemoryError, "out of memory");
      return NULL;
    }
  }

  if ((*self->read_func)(self, &s, i) == -1)
  {
    free(s);
    return NULL;
  }

  UNLESS(py_string = PyString_FromStringAndSize(s, i))
  {
    free(s);
    return NULL;
  }

  free(s);

  if (PyList_Append(self->stack, py_string) == -1)
    goto err;

  Py_DECREF(py_string);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(py_string);

  return NULL;
}


static PyObject *
load_short_binstring(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_string = 0;
  char *s;
  unsigned char l;  


  UNLESS(s = (char *)malloc(sizeof(char)))
  {
    PyErr_SetString(PyExc_MemoryError, "out of memory");
    return NULL;
  }

  if ((*self->read_func)(self, &s, 1) == -1)
  {
    free(s);
    return NULL;
  }

  l = (unsigned char)s[0];

  if (l > 1)
  {
    free(s);

    UNLESS(s = (char *)malloc(l * sizeof(char)))
    {  
      PyErr_SetString(PyExc_MemoryError, "out of memory");
      return NULL;
    }
  }

  if ((*self->read_func)(self, &s, l) == -1)
  {
    free(s);
    return NULL;
  }

  UNLESS(py_string = PyString_FromStringAndSize(s, l))
  {
    free(s);
    return NULL;
  }

  free(s);

  if (PyList_Append(self->stack, py_string) == -1)
  {
    Py_DECREF(py_string);
    return NULL;
  }

  Py_DECREF(py_string);

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
load_tuple(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *tup = 0, *slice = 0, *list = 0;
  int i, j;

  if ((i = marker(self)) == -1)
    return NULL;

  if ((j = PyList_Size(self->stack)) == -1)  
    goto err;

  UNLESS(slice = PyList_GetSlice(self->stack, i, j))
    goto err;
  
  UNLESS(tup = PySequence_Tuple(slice))
    goto err;

  UNLESS(list = PyList_New(1))
    goto err;

  if (PyList_SetItem(list, 0, tup) == -1)
    goto err;

  Py_INCREF(tup);

  if (PyList_SetSlice(self->stack, i, j, list) == -1)
    goto err;

  Py_DECREF(tup);
  Py_DECREF(list);
  Py_DECREF(slice);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(tup);
  Py_XDECREF(list);
  Py_XDECREF(slice);

  return NULL;
}


static PyObject *
load_list(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *list = 0, *slice = 0;
  int i, j;

  if ((i = marker(self)) == -1)
    return NULL;

  if ((j = PyList_Size(self->stack)) == -1)
    goto err;

  UNLESS(slice = PyList_GetSlice(self->stack, i, j))
    goto err;

  UNLESS(list = PyList_New(1))
    goto err;

  if (PyList_SetItem(list, 0, slice) == -1)
    goto err;
  Py_INCREF(slice);

  if (PyList_SetSlice(self->stack, i, j, list) == -1)
    goto err;

  Py_DECREF(list);
  Py_DECREF(slice);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(list);
  Py_XDECREF(slice);

  return NULL;
}


static PyObject *
load_dict(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *list = 0, *dict = 0, *key = 0, *value = 0;
  int i, j, k;

  if ((i = marker(self)) == -1)
    return NULL;

  if ((j = PyList_Size(self->stack)) == -1)
    goto err;

  UNLESS(dict = PyDict_New())
    goto err;

  for (k = i; k < j; k += 2)
  {
    UNLESS(key = PyList_GET_ITEM((PyListObject *)self->stack, k))
      goto err;

    UNLESS(value = PyList_GET_ITEM((PyListObject *)self->stack, k + 1))
      goto err;

    if (PyDict_SetItem(dict, key, value) == -1)
      goto err;
  }

  UNLESS(list = PyList_New(1))
    goto err;

  if (PyList_SetItem(list, 0, dict) == -1)
    goto err;
  Py_INCREF(dict);

  if (PyList_SetSlice(self->stack, i, j, list) == -1)
    goto err;

  Py_DECREF(list);
  Py_DECREF(dict);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(dict);
  Py_XDECREF(list);

  return NULL;
}


static PyObject *
load_obj(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *class = 0, *slice = 0, *tup = 0, *obj = 0;
  long i;
  int len;

  if ((i = marker(self)) == -1)
    return NULL;

  class = PyList_GET_ITEM((PyListObject *)self->stack, i);
  Py_INCREF(class);

  if ((len = PyList_Size(self->stack)) == -1)
    goto err;

  UNLESS(slice = PyList_GetSlice(self->stack, i + 1, len))
    goto err;

  UNLESS(tup = PySequence_Tuple(slice))
    goto err;

  if (DEL_LIST_SLICE(self->stack, i, len) == -1)
    goto err;

  UNLESS(obj = PyInstance_New(class, tup, NULL))
    goto err;

  if (PyList_Append(self->stack, obj) == -1)
    goto err;

  Py_DECREF(class);
  Py_DECREF(slice);
  Py_DECREF(tup);
  Py_DECREF(obj);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(class);
  Py_XDECREF(slice);
  Py_XDECREF(tup);
  Py_XDECREF(obj);

  return NULL;
}


static PyObject *
load_inst(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *arg_tup = 0, *arg_slice = 0, *class = 0, *obj = 0;
  int i, j;
  char *s, *module_name, *class_name;

  if ((i = marker(self)) == -1)
    return NULL;

  if ((j = PyList_Size(self->stack)) == -1)
    goto err;

  UNLESS(arg_slice = PyList_GetSlice(self->stack, i, j))
    goto err;

  UNLESS(arg_tup = PySequence_Tuple(arg_slice))
    goto err;

  if (DEL_LIST_SLICE(self->stack, i, j) == -1)
    goto err;

  if ((*self->readline_func)(self, &s) == -1)
    goto err;

  module_name = s;

  if ((*self->readline_func)(self, &s) == -1)
  {
    free(module_name);
    goto err;
  }

  class_name = s;

  UNLESS(class = find_class(module_name, class_name))
  {
    free(module_name);
    free(class_name);
    goto err;
  }

  free(module_name);
  free(class_name);

  UNLESS(obj = PyInstance_New(class, arg_tup, NULL))
    goto err;

  if (PyList_Append(self->stack, obj) == -1)
    goto err;

  Py_DECREF(arg_slice);
  Py_DECREF(arg_tup);
  Py_DECREF(class);
  Py_DECREF(obj);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(arg_slice);
  Py_XDECREF(arg_tup);
  Py_XDECREF(class);
  Py_XDECREF(obj);

  return NULL;
}


static PyObject *
load_class(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *class = 0;
  char *s, *module_name, *class_name;

  if ((*self->readline_func)(self, &s) == -1)
    return NULL;

  module_name = s;

  if ((*self->readline_func)(self, &s) == -1)
  {
    free(module_name);
    return NULL;
  }

  class_name = s;

  UNLESS(class = find_class(module_name, class_name))
  {
    free(module_name);
    free(class_name);
    return NULL;
  }

  free(module_name);
  free(class_name);

  if (PyList_Append(self->stack, class) == -1)
    goto err;

  Py_DECREF(class);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(class);

  return NULL;
}


static PyObject *
load_persid(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *pid = 0, *pers_load_val = 0;
  char *s;
  int len;

  if (self->pers_func)
  {
    if ((len = (*self->readline_func)(self, &s)) == -1)
      return NULL;
  
    UNLESS(pid = PyString_FromStringAndSize(s, len))
    { 
      free(s);
      return NULL;
    }

    free(s);

    UNLESS(self->arg)
      UNLESS(self->arg = PyTuple_New(1))
        goto err;

    if (PyTuple_SetItem(self->arg, 0, pid) == -1)
      goto err;
    Py_INCREF(pid);
      
    UNLESS(pers_load_val = PyObject_CallObject(self->pers_func, self->arg))
      goto err;

    if (PyList_Append(self->stack, pers_load_val) == -1)
      goto err;

    Py_DECREF(pid);
    Py_DECREF(pers_load_val);
  
    Py_INCREF(Py_None);
    return Py_None;
  }

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(pid);
  Py_XDECREF(pers_load_val);

  return NULL;
}


static PyObject *
load_pop(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  int len;

  if ((len = PyList_Size(self->stack)) == -1)  
    return NULL;

  if (DEL_LIST_SLICE(self->stack, len - 1, len) == -1)  
    return NULL;

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
load_dup(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *last;
  int len;

  if ((len = PyList_Size(self->stack)) == -1)
    return NULL;
  
  UNLESS(last = PyList_GetItem(self->stack, len - 1))  
    return NULL;

  if (PyList_Append(self->stack, last) == -1)
    return NULL;

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
load_get(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_str = 0, *value = 0;
  char *s;
  int len;

  if ((len = (*self->readline_func)(self, &s)) == -1)
    return NULL;

  UNLESS(py_str = PyString_FromStringAndSize(s, len))
  {
    free(s);
    return NULL;
  }

  free(s);
  
  UNLESS(value = PyDict_GetItem(self->memo, py_str))  
    goto err;

  if (PyList_Append(self->stack, value) == -1)
    goto err;

  Py_DECREF(py_str);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(py_str);

  return NULL;
}


static PyObject *
load_binget(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_key = 0, *value = 0;
  char *s;
  unsigned char key;

  UNLESS(s = (char *)malloc(sizeof(char)))
  {
    PyErr_SetString(PyExc_MemoryError, "out of memory");
    return NULL;
  }

  if ((*self->read_func)(self, &s, 1) == -1)
  {
    free(s);
    return NULL;
  }

  key = (unsigned char)s[0];
  free(s);

  UNLESS(py_key = PyInt_FromLong((long)key))
    return NULL;

  UNLESS(value = PyDict_GetItem(self->memo, py_key))  
    goto err;

  if (PyList_Append(self->stack, value) == -1)
    return NULL;

  Py_DECREF(py_key);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(py_key);

  return NULL;
}


static PyObject *
load_put(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_str = 0, *value = 0;
  int len;
  char *s;

  if ((len = (*self->readline_func)(self, &s)) == -1)
    return NULL;

  UNLESS(py_str = PyString_FromStringAndSize(s, len))
  {
    free(s);
    return NULL;
  }

  free(s);

  if ((len = PyList_Size(self->stack)) == -1)
    goto err;

  UNLESS(value = PyList_GetItem(self->stack, len - 1))  
    goto err;

  if (PyDict_SetItem(self->memo, py_str, value) == -1)  
    goto err;

  Py_DECREF(py_str);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(py_str);

  return NULL;
}


static PyObject *
load_binput(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *py_key = 0, *value = 0;
  char *s;
  unsigned char key;
  int len;

  UNLESS(s = (char *)malloc(sizeof(char)))
  {
    PyErr_SetString(PyExc_MemoryError, "out of memory");
    return NULL;
  }

  if ((*self->read_func)(self, &s, 1) == -1)
    return NULL;

  key = (unsigned char)s[0];

  free(s);

  UNLESS(py_key = PyInt_FromLong((long)key))
    return NULL;

  if ((len = PyList_Size(self->stack)) == -1)
    goto err;

  UNLESS(value = PyList_GetItem(self->stack, len - 1))  
    goto err;

  if (PyDict_SetItem(self->memo, py_key, value) == -1)  
    goto err;

  Py_DECREF(py_key);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(py_key);

  return NULL;
}


static PyObject *
load_append(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *value = 0, *list = 0;
  int len;
  static PyObject *append_str = 0;

  UNLESS(append_str)
    UNLESS(append_str = PyString_FromString("append"))
      return NULL;

  if ((len = PyList_Size(self->stack)) == -1)  
    return NULL;

  UNLESS(value = PyList_GetItem(self->stack, len - 1))  
    return NULL;
  Py_INCREF(value);

  if (DEL_LIST_SLICE(self->stack, len - 1, len) == -1)  
    goto err;

  UNLESS(list = PyList_GetItem(self->stack, len - 2))  
    goto err;

  if (PyList_Check(list))
  {
    if (PyList_Append(list, value) == -1)
      goto err;
  }
  else
  {
    PyObject *append_method, *junk;

    UNLESS(append_method = PyObject_GetAttr(list, append_str))
      return NULL;

    UNLESS(self->arg)
      UNLESS(self->arg = PyTuple_New(1))
      {
        Py_DECREF(append_method);
        goto err;
      }

    if (PyTuple_SetItem(self->arg, 0, value) == -1)
    {
      Py_DECREF(append_method);
      goto err;
    }
    Py_INCREF(value);

    UNLESS(junk = PyObject_CallObject(append_method, self->arg))
    {
      Py_DECREF(append_method);
      goto err;
    }
    Py_DECREF(junk);

    Py_DECREF(append_method);
  }

  Py_DECREF(value);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(value);
 
  return NULL;
}


static PyObject *
load_setitem(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *value = 0, *key = 0, *dict = 0;
  int len;

  if ((len = PyList_Size(self->stack)) == -1)  
    return NULL;

  UNLESS(value = PyList_GetItem(self->stack, len - 1))
    return NULL;
  Py_INCREF(value);

  UNLESS(key = PyList_GetItem(self->stack, len - 2))  
    goto err;
  Py_INCREF(key);

  if (DEL_LIST_SLICE(self->stack, len - 2, len) == -1)  
    goto err;

  UNLESS(dict = PyList_GetItem(self->stack, len - 3))  
    goto err;

  if (PyObject_SetItem(dict, key, value) == -1)  
    goto err;

  Py_DECREF(value);
  Py_DECREF(key);
  
  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(value);
  Py_XDECREF(key);

  return NULL;
}


static PyObject *
load_build(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *value = 0, *inst = 0, *instdict = 0, *d_key = 0, *d_value = 0, 
           *junk = 0;
  static PyObject *py_string__dict__;
  int len, i;

  UNLESS(py_string__dict__)
    UNLESS(py_string__dict__ = PyString_FromString("__dict__"))
      return NULL;

  if ((len = PyList_Size(self->stack)) == -1)
    goto err;

  UNLESS(value = PyList_GetItem(self->stack, len - 1))
    goto err; 
  Py_INCREF(value);

  if (DEL_LIST_SLICE(self->stack, len - 1, len) == -1)
    goto err;

  UNLESS(inst = PyList_GetItem(self->stack, len - 2))
    goto err;

  UNLESS(PyObject_HasAttrString(inst, "__setstate__"))
  {
    UNLESS(instdict = PyObject_GetAttr(inst, py_string__dict__))
      goto err;

    i = 0;
    while (PyDict_Next(value, &i, &d_key, &d_value))
    {
      if (PyObject_SetItem(instdict, d_key, d_value) == -1)
        goto err;
    }
  }
  else
  {
    UNLESS(junk = PyObject_CallMethod(inst, "__setstate__", "O", value))
      goto err;
    Py_DECREF(junk);
  }

  Py_DECREF(value);
  Py_XDECREF(instdict);

  Py_INCREF(Py_None);
  return Py_None;

err:
  Py_XDECREF(value);
  Py_XDECREF(instdict);
  
  return NULL;
}


static PyObject *
load_mark(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  int len;
  
  if ((len = PyList_Size(self->stack)) == -1)
    return NULL;

  if (!self->num_marks)
  {
    UNLESS(self->marks = (int *)malloc(20 * sizeof(int)))
    {
      PyErr_SetString(PyExc_MemoryError, "out of memory");
      return NULL;
    }
 
    self->marks_size = 20;
  }
  else if ((self->num_marks + 1) > self->marks_size)
  {
    UNLESS(self->marks = (int *)realloc(self->marks, (self->marks_size + 20) * sizeof(int)))
    {
      PyErr_SetString(PyExc_MemoryError, "out of memory");
      return NULL;
    }

    self->marks_size += 20;
  }

  self->marks[self->num_marks++] = len;

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *  
load_eof(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyErr_SetNone(PyExc_EOFError);
  return NULL;
}


static PyObject *
Unpickler_load(ARG(Unpicklerobject *, self), ARG(PyObject *, args))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *stack = 0, *key = 0, *junk = 0, *err = 0,
           *val = 0, *str = 0, *key_repr = 0;
  char c;
  char *c_str;
  int len;

  c_str=&c;

  UNLESS(stack = PyList_New(0))
    goto err;

  self->stack = stack;
  self->num_marks = 0;

  while (1)
  {
    if ((*self->read_func)(self, &c_str, 1) == -1)
      break;

    switch (c_str[0])
    {
      case NONE:
        UNLESS(junk = load_none(self, NULL))
          break;
        continue;

      case BININT:
        UNLESS(junk = load_binint(self, NULL))
          break;
        continue;

      case BININT1:
        UNLESS(junk = load_binint1(self, NULL))
          break;
        continue;

      case BININT2:
        UNLESS(junk = load_binint2(self, NULL))
          break;
        continue;

      case BININT3:
        UNLESS(junk = load_binint3(self, NULL))
          break;
        continue;
 
      case INT:
        UNLESS(junk = load_int(self, NULL))
          break;
        continue;
      
      case LONG:
        UNLESS(junk = load_long(self, NULL))
          break;
        continue;

      case FLOAT:
        UNLESS(junk = load_float(self, NULL))
          break;
        continue;

      case BINSTRING:
        UNLESS(junk = load_binstring(self, NULL))
          break;
        continue;

      case SHORT_BINSTRING:
        UNLESS(junk = load_short_binstring(self, NULL))
          break;
        continue;

      case STRING:
        UNLESS(junk = load_string(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case TUPLE:
        UNLESS(junk = load_tuple(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case LIST:
        UNLESS(junk = load_list(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case DICT:
        UNLESS(junk = load_dict(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case OBJ:
        UNLESS(junk = load_obj(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case INST:
        UNLESS(junk = load_inst(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case CLASS:
        UNLESS(junk = load_class(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case APPEND:
        UNLESS(junk = load_append(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case BUILD:
        UNLESS(junk = load_build(self, NULL))
          break;
        Py_DECREF(junk);
        continue;
  
      case DUP:
        UNLESS(junk = load_dup(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case BINGET:
        UNLESS(junk = load_binget(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case GET:
        UNLESS(junk = load_get(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case MARK:
        UNLESS(junk = load_mark(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case BINPUT:
        UNLESS(junk = load_binput(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case PUT:
        UNLESS(junk = load_put(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case POP:
        UNLESS(junk = load_pop(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case SETITEM:
        UNLESS(junk = load_setitem(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      case STOP:
        break;

      case PERSID:
        UNLESS(junk = load_persid(self, NULL))
          break;
        Py_DECREF(junk);
        continue;

      default: 
        UNLESS(key = PyString_FromStringAndSize(c_str, 1))
          return NULL;
  
        UNLESS(key_repr = PyObject_Repr(key))
          return NULL;
        
        UNLESS(str = PyString_FromStringAndSize(NULL, 19 + PyString_Size(key_repr)))
          return NULL;

        sprintf(PyString_AS_STRING((PyStringObject *)str), 
            "invalid load key, \%s.", PyString_AS_STRING((PyStringObject *)key_repr));

        PyErr_SetObject(UnpicklingError, str);

        Py_DECREF(str);
        Py_DECREF(key);
        Py_DECREF(key_repr);

        return NULL;
    }

    break;
  }

  if ((err = PyErr_Occurred()) == PyExc_EOFError)
  {
    return load_eof(self, NULL);
  }    

  if (err)    
    return NULL;

  if ((len = PyList_Size(self->stack)) == -1)  
    return NULL;

  UNLESS(val = PyList_GetItem(self->stack, len - 1))  
    return NULL;
  Py_INCREF(val);

  if (DEL_LIST_SLICE(self->stack, len - 1, len) == -1)
  {
    Py_DECREF(val);
    return NULL;
  }

  return val;

err:
  Py_XDECREF(stack);

  return NULL;
}


static struct PyMethodDef Unpickler_methods[] = 
{
  {"load",         (PyCFunction)Unpickler_load,   0, ""},
  {"load_none",    (PyCFunction)load_none,        0, ""},
  {"load_int",     (PyCFunction)load_int,         0, ""},
  {"load_long",    (PyCFunction)load_long,        0, ""},
  {"load_float",   (PyCFunction)load_float,       0, ""},
  {"load_string",  (PyCFunction)load_string,      0, ""},
  {"load_tuple",   (PyCFunction)load_tuple,       0, ""},
  {"load_list",    (PyCFunction)load_list,        0, ""},
  {"load_dict",    (PyCFunction)load_dict,        0, ""},
  {"load_inst",    (PyCFunction)load_inst,        0, ""},
  {"load_class",   (PyCFunction)load_class,       0, ""},
  {"load_persid",  (PyCFunction)load_persid,      0, ""},
  {"load_pop",     (PyCFunction)load_pop,         0, ""},
  {"load_dup",     (PyCFunction)load_dup,         0, ""},
  {"load_get",     (PyCFunction)load_get,         0, ""},
  {"load_put",     (PyCFunction)load_put,         0, ""},
  {"load_append",  (PyCFunction)load_append,      0, ""},
  {"load_setitem", (PyCFunction)load_setitem,     0, ""},
  {"load_build",   (PyCFunction)load_build,       0, ""},
  {"load_mark",    (PyCFunction)load_mark,        0, ""},
  {"load_eof",     (PyCFunction)load_eof,         0, ""},
  {NULL,		NULL}		/* sentinel */
};


static Unpicklerobject *
newUnpicklerobject(ARG(PyObject *, f))
    ARGDECL(PyObject *, f)
{
  Unpicklerobject *self;
  PyObject *memo = 0;
	
  UNLESS(memo = PyDict_New())
    goto err;

  UNLESS(self = PyObject_NEW(Unpicklerobject, &Unpicklertype))  goto err;

  if (PyFile_Check(f))
  {
    self->fp = PyFile_AsFile(f);
    self->read_func = read_file;
    self->readline_func = readline_file;
    self->read = NULL;
    self->readline = NULL;
  }
  else if (PycStringIO_InputCheck(f))
  {
    self->fp = NULL;
    self->read_func = read_cStringIO;
    self->readline_func = readline_cStringIO;
    self->read = NULL;
    self->readline = NULL;
  }
  else
  {
    PyObject *readline, *read;

    self->fp = NULL;
    self->read_func = read_other;
    self->readline_func = readline_other;

    UNLESS(readline = PyObject_GetAttrString(f, "readline"))
      goto err;

    UNLESS(read = PyObject_GetAttrString(f, "read"))
    {
      Py_DECREF(readline);
      goto err;
    }
  
    self->read = read; 
    self->readline = readline;
  }

  Py_INCREF(f);
  
  self->file = f;
  self->memo   = memo;
  self->arg    = NULL;
  self->stack  = NULL;
  self->pers_func = NULL;
  self->marks = NULL;
  self->num_marks = 0;
  self->marks_size = 0;

  return self;

err:
  Py_XDECREF(memo);
  Py_XDECREF((PyObject *)self);

  return NULL;
}


static PyObject *
get_Unpickler(ARG(PyObject *, self), ARG(PyObject *, args))
    ARGDECL(PyObject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *file;

  UNLESS(PyArg_Parse(args, "O", &file))  return NULL;
  return (PyObject *)newUnpicklerobject(file);
}


static void
Unpickler_dealloc(ARG(Unpicklerobject *, self))
    ARGDECL(Unpicklerobject *, self)
{
  Py_XDECREF(self->readline);
  Py_XDECREF(self->read);
  Py_XDECREF(self->file);
  Py_XDECREF(self->memo);
  Py_XDECREF(self->stack);
  Py_XDECREF(self->pers_func);
  Py_XDECREF(self->arg);
  free(self->marks);
  PyMem_DEL(self);
}


static PyObject *
Unpickler_getattr(ARG(Unpicklerobject *, self), ARG(char *, name))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(char *, name)
{
  if (!strcmp(name, "persistent_load"))
  {
    if (!self->pers_func)
    {
      PyErr_SetString(PyExc_NameError, name);
      return NULL;
    }

    Py_INCREF(self->pers_func);
    return self->pers_func;
  }

  if (!strcmp(name, "memo"))
  {
    if (!self->memo)
    {
      PyErr_SetString(PyExc_NameError, name);
      return NULL;
    }

    Py_INCREF(self->memo);
    return self->memo;
  }

  if (!strcmp(name, "UnpicklingError"))
  {
    Py_INCREF(UnpicklingError);
    return UnpicklingError;
  }

  return Py_FindMethod(Unpickler_methods, (PyObject *)self, name);
}


static int
Unpickler_setattr(ARG(Unpicklerobject *, self), ARG(char *, name), ARG(PyObject *, value))
    ARGDECL(Unpicklerobject *, self)
    ARGDECL(char *, name)
    ARGDECL(PyObject *, value)
{
  if (!strcmp(name, "persistent_load"))
  {
    Py_XDECREF(self->pers_func);
    self->pers_func = value;
    Py_INCREF(value);
    return 0;
  }

  return -1;
}


static PyObject *
dump(ARG(PyObject *, self), ARG(PyObject *, args))
    ARGDECL(PyObject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *ob, *file, *ret_val;
  Picklerobject *pickler;
  int bin = 0;

  UNLESS(PyArg_ParseTuple(args, "OO|i", &ob, &file, &bin))
    return NULL;

  UNLESS(pickler = newPicklerobject(file, bin))
    return NULL;

  UNLESS(ret_val = Pickler_dump(pickler, ob))
  {
    Pickler_dealloc(pickler);
    return NULL;
  }

  Pickler_dealloc(pickler);

  return ret_val;
}


static PyObject *
dumps(ARG(PyObject *, self), ARG(PyObject *, args))
    ARGDECL(PyObject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *ob, *file, *pickle_str;
  Picklerobject *pickler;
  int bin = 0;

  UNLESS(PyArg_ParseTuple(args, "O|i", &ob, &bin))
    return NULL;

  UNLESS(file = (*PycStringIO_NewOutput)(128))
    return NULL;

  UNLESS(pickler = newPicklerobject(file, bin))
  {
    Py_DECREF(file);
    return NULL;
  }

  UNLESS(Pickler_dump(pickler, ob))
  {
    Pickler_dealloc(pickler);
    Py_DECREF(file);
    return NULL;
  }

  Pickler_dealloc(pickler);

  UNLESS(pickle_str = (*PycStringIO_cgetvalue)((PyObject *)file))
  {
    Py_DECREF(file);
    return NULL;
  }

  Py_DECREF(file);

  return pickle_str;
}  
  

static PyObject *
cpm_load(ARG(PyObject *, self), ARG(PyObject *, args))
    ARGDECL(PyObject *, self)
    ARGDECL(PyObject *, args)
{
  Unpicklerobject *unpickler;
  PyObject *load_result;

  UNLESS(PyArg_Parse(args, "O", &args))
    return NULL;

  UNLESS(unpickler = newUnpicklerobject(args))
    return NULL;

  UNLESS(load_result = Unpickler_load(unpickler, NULL))
  {
    Unpickler_dealloc(unpickler);
    return NULL;
  }

  Py_DECREF(unpickler);

  return load_result;
}


static PyObject *
loads(ARG(PyObject *, self), ARG(PyObject *, args))
    ARGDECL(PyObject *, self)
    ARGDECL(PyObject *, args)
{
  PyObject *file, *load_result;
  Unpicklerobject *unpickler;

  UNLESS(PyArg_Parse(args, "O", &args))
    return NULL;

  UNLESS(file = (*PycStringIO_NewInput)(args))
    return NULL;
  
  UNLESS(unpickler = newUnpicklerobject(file))
  {
    Py_DECREF(file);
    return NULL;
  }

  UNLESS(load_result = Unpickler_load(unpickler, NULL))
  {
    Unpickler_dealloc(unpickler);
    Py_DECREF(file);
    return NULL;
  }

  Py_DECREF(file);
  Unpickler_dealloc(unpickler);

  return load_result;
}


static char Unpicklertype__doc__[] = "";

static PyTypeObject Unpicklertype = 
{
  PyObject_HEAD_INIT(&PyType_Type)
  0,				/*ob_size*/
  "Unpickler",			/*tp_name*/
  sizeof(Unpicklerobject),		/*tp_basicsize*/
  0,				/*tp_itemsize*/
  /* methods */
  (destructor)Unpickler_dealloc,	/*tp_dealloc*/
  (printfunc)0,		/*tp_print*/
  (getattrfunc)Unpickler_getattr,	/*tp_getattr*/
  (setattrfunc)Unpickler_setattr,	/*tp_setattr*/
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
  Unpicklertype__doc__ /* Documentation string */
};


static struct PyMethodDef cPickle_methods[] =
{
  {"dump",         (PyCFunction)dump,             1, ""},
  {"dumps",        (PyCFunction)dumps,            1, ""},
  {"load",         (PyCFunction)cpm_load,         0, ""},
  {"loads",        (PyCFunction)loads,            0, ""},
  {"Pickler",      (PyCFunction)get_Pickler,      1, ""},
  {"Unpickler",    (PyCFunction)get_Unpickler,    0, ""},
  { NULL, NULL }
};


static int
init_stuff()
{
  PyObject *builtins, *apply_func, *string;

  UNLESS(builtins = PyImport_ImportModule("__builtin__"))
    return 1;

  UNLESS(apply_func = PyObject_GetAttrString(builtins, "apply"))
    return 1;

  BuiltinFunctionType = apply_func->ob_type;

  Py_DECREF(apply_func);

  Py_DECREF(builtins);

  UNLESS(string = PyImport_ImportModule("string"))
    return 1;

  UNLESS(atol_func = PyObject_GetAttrString(string, "atol"))
    return 1;

  Py_DECREF(string);

  UNLESS(empty_list = PyList_New(0))
    return 1;

  UNLESS(empty_tuple = PyTuple_New(0))
    return 1;

  UNLESS(class_map = PyDict_New())
    return 1;

  UNLESS(PicklingError = PyString_FromString("cPickle.PicklingError"))
    return 1;

  UNLESS(UnpicklingError = PyString_FromString("cPickle.UnpicklingError"))
    return 1;

  PycString_IMPORT;
  return 0;
}


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


/* Initialization function for the module (*must* be called initcPickle) */
void
initcPickle()
{
  PyObject *m, *d;

  /* Create the module and add the functions */
  m = Py_InitModule4("cPickle", cPickle_methods,
      cPickle_module_documentation,
      (PyObject*)NULL,PYTHON_API_VERSION);

  /* Add some symbolic constants to the module */
  d = PyModule_GetDict(m);
  ErrorObject = PyString_FromString("cPickle.error");
  PyDict_SetItemString(d, "error", ErrorObject);

  /* XXXX Add constants here */

  if (init_stuff())
	  Py_FatalError("can't initialize module cPickle");

  CHECK_FOR_ERRORS("can't initialize module cPickle");

  PyDict_SetItemString(d, "PicklingError", PicklingError);
  CHECK_FOR_ERRORS("can't initialize module cPickle:  error creating PicklingError");

  PyDict_SetItemString(d, "UnpicklingError", UnpicklingError);
  CHECK_FOR_ERRORS("can't initialize module cPickle:  error creating UnpicklingError");  
}
