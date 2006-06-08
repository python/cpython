/* swi

   RISC OS swi functions

 1.00               Chris Stretch


 1.01  12 May 1999  Laurence Tratt

   * Changed swi.error to be a class based exception rather than string based
   * Added swi.ArgError which is generated for errors when the user passes invalid arguments to
       functions etc
   * Added "errnum" attribute to swi.error, so one can now check to see what the error number was

 1.02  03 March 2002 Dietmar Schwertberger
   * Added string, integer, integers, tuple and tuples
*/

#include "oslib/os.h"
#include <kernel.h>
#include "Python.h"


#define PyBlock_Check(op) ((op)->ob_type == &PyBlockType)


static PyObject *SwiError; /* Exception swi.error */
static PyObject *ArgError; /* Exception swi.ArgError */
static os_error *e;

static PyObject *swi_oserror(void)
{ PyErr_SetString(SwiError,e->errmess);
  PyObject_SetAttrString(PyErr_Occurred(), "errnum", PyInt_FromLong(e->errnum));
  return 0;
}

static PyObject *swi_error(char *s)
{ PyErr_SetString(ArgError,s);
  return 0;
}

typedef struct
{ PyObject_HEAD
  void *block;
  int length; /*length in bytes*/
  int heap;
} PyBlockObject;

static PyTypeObject PyBlockType;

/* block commands */

static PyObject *PyBlock_New(PyObject *self,PyObject *args)
{ int size;
  PyBlockObject *b;
  PyObject *init=0;
  if(!PyArg_ParseTuple(args,"i|O",&size,&init)) return NULL;
  if(size<1) size=1;
  b=PyObject_NEW(PyBlockObject,&PyBlockType);
  if(!b) return NULL;
  b->block=malloc(4*size);
  if(!b->block)
  { Py_DECREF(b);
    return PyErr_NoMemory();
  }
  b->length=4*size;
  b->heap=1;
  if(init)
  { if(PyString_Check(init))
    { int n=PyString_Size(init);
      if (n>4*size) n=4*size;
      memcpy(b->block,PyString_AsString(init),n);
      memset((char*)b->block+n,0,4*size-n);
    }
    else
    { int n,k;
      long *p=(long*)b->block;
      if(!PyList_Check(init)) goto fail;
      n=PyList_Size(init);
      if (n>size) n=size;
      for(k=0;k<n;k++)
      { PyObject *q=PyList_GetItem(init,k);
        if(!PyInt_Check(q)) goto fail;
        p[k]=PyInt_AsLong(q);
      }
      for(;k<size;k++) p[k]=0;
    }
  }
  return (PyObject *)b;
  fail:PyErr_SetString(PyExc_TypeError,
     "block initialiser must be string or list of integers");
  Py_DECREF(b);
  return NULL;
}

static PyObject *PyRegister(PyObject *self,PyObject *args)
{ int size,ptr;
  PyBlockObject *b;
  if(!PyArg_ParseTuple(args,"ii",&size,&ptr)) return NULL;
  if(size<1) size=1;
  b=PyObject_NEW(PyBlockObject,&PyBlockType);
  if(!b) return NULL;
  b->block=(void*)ptr;
  b->length=4*size;
  b->heap=0;
  return (PyObject *)b;
}

static PyObject *PyBlock_ToString(PyBlockObject *self,PyObject *arg)
{ int s=0,e=self->length;
  if(!PyArg_ParseTuple(arg,"|ii",&s,&e)) return NULL;
  if(s<0||e>self->length||s>e)
  { PyErr_SetString(PyExc_IndexError,"block index out of range");
    return NULL;
  }
  return PyString_FromStringAndSize((char*)self->block+s,e-s);
}

static PyObject *PyBlock_NullString(PyBlockObject *self,PyObject *arg)
{ int s=0,e=self->length,i;
  char *p=(char*)self->block;
  if(!PyArg_ParseTuple(arg,"|ii",&s,&e)) return NULL;
  if(s<0||e>self->length||s>e)
  { PyErr_SetString(PyExc_IndexError,"block index out of range");
    return NULL;
  }
  for(i=s;i<e;i++) if(p[i]==0) break;
  return PyString_FromStringAndSize((char*)self->block+s,i-s);
}

static PyObject *PyBlock_CtrlString(PyBlockObject *self,PyObject *arg)
{ int s=0,e=self->length,i;
  char *p=(char*)self->block;
  if(!PyArg_ParseTuple(arg,"|ii",&s,&e)) return NULL;
  if(s<0||e>self->length||s>e)
  { PyErr_SetString(PyExc_IndexError,"block index out of range");
    return NULL;
  }
  for(i=s;i<e;i++) if(p[i]<32) break;
  return PyString_FromStringAndSize((char*)self->block+s,i-s);
}

static PyObject *PyBlock_PadString(PyBlockObject *self,PyObject *arg)
{ int s=0,e=self->length,n,m;
  char *str;
  char c;
  char *p=(char*)self->block;
  if(!PyArg_ParseTuple(arg,"s#c|ii",&str,&n,&c,&s,&e)) return NULL;
  if(s<0||e>self->length||s>e)
  { PyErr_SetString(PyExc_IndexError,"block index out of range");
    return NULL;
  }
  m=e-s;
  if(n>m) n=m;
  memcpy(p+s,str,n);memset(p+s+n,c,m-n);
  Py_INCREF(Py_None);return Py_None;
}

static PyObject *PyBlock_BitSet(PyBlockObject *self,PyObject *arg)
{ int i,x,y;
  int *p=(int*)self->block;
  if(!PyArg_ParseTuple(arg,"iii",&i,&x,&y)) return NULL;
  if(i<0||i>=self->length/4)
  { PyErr_SetString(PyExc_IndexError,"block index out of range");
    return NULL;
  }
  p[i]=(p[i]&y)^x;
  Py_INCREF(Py_None);return Py_None;
}

static PyObject *PyBlock_Resize(PyBlockObject *self,PyObject *arg)
{ int n;
  if(!PyArg_ParseTuple(arg,"i",&n)) return NULL;
  if(n<1) n=1;
  if(self->heap)
  { void *v=realloc(self->block,4*n);
    if (!v) return PyErr_NoMemory();
    self->block=v;
  }
  self->length=4*n;
  Py_INCREF(Py_None);return Py_None;
}

static PyObject *PyBlock_ToFile(PyBlockObject *self,PyObject *arg)
{ int s=0,e=self->length/4;
  PyObject *f;
  FILE *fp;
  if(!PyArg_ParseTuple(arg,"O|ii",&f,&s,&e)) return NULL;
  fp=PyFile_AsFile(f);
  if (!fp)
  { PyErr_SetString(PyExc_TypeError, "arg must be open file");
    return NULL;
  }
  fwrite((int*)(self->block)+s,4,e-s,fp);
  Py_INCREF(Py_None);return Py_None;
}

static struct PyMethodDef PyBlock_Methods[]=
{ { "tostring",(PyCFunction)PyBlock_ToString,1},
  { "padstring",(PyCFunction)PyBlock_PadString,1},
  { "nullstring",(PyCFunction)PyBlock_NullString,1},
  { "ctrlstring",(PyCFunction)PyBlock_CtrlString,1},
  { "bitset",(PyCFunction)PyBlock_BitSet,1},
  { "resize",(PyCFunction)PyBlock_Resize,1},
  { "tofile",(PyCFunction)PyBlock_ToFile,1},
  { NULL,NULL}		/* sentinel */
};

static int block_len(PyBlockObject *b)
{ return b->length/4;
}

static PyObject *block_concat(PyBlockObject *b,PyBlockObject *c)
{ PyErr_SetString(PyExc_IndexError,"block concatenation not implemented");
  return NULL;
}

static PyObject *block_repeat(PyBlockObject *b,Py_ssize_t i)
{ PyErr_SetString(PyExc_IndexError,"block repetition not implemented");
  return NULL;
}

static PyObject *block_item(PyBlockObject *b,Py_ssize_t i)
{ if(i<0||4*i>=b->length)
  { PyErr_SetString(PyExc_IndexError,"block index out of range");
    return NULL;
  }
  return PyInt_FromLong(((long*)(b->block))[i]);
}

static PyObject *block_slice(PyBlockObject *b,Py_ssize_t i,Py_ssize_t j)
{ Py_ssize_t n,k;
  long *p=b->block;
  PyObject *result;
  if(j>b->length/4) j=b->length/4;
  if(i<0||i>j)
  { PyErr_SetString(PyExc_IndexError,"block index out of range");
    return NULL;
  }
  n=j-i;
  result=PyList_New(n);
  for(k=0;k<n;k++) PyList_SetItem(result,k,PyInt_FromSsize_t(p[i+k]));
  return result;
}

static int block_ass_item(PyBlockObject *b,Py_ssize_t i,PyObject *v)
{ if(i<0||i>=b->length/4)
  { PyErr_SetString(PyExc_IndexError,"block index out of range");
    return -1;
  }
  if(!PyInt_Check(v))
  { PyErr_SetString(PyExc_TypeError,"block item must be integer");
    return -1;
  }
  ((long*)(b->block))[i]=PyInt_AsLong(v);
  return 0;
}

static int block_ass_slice(PyBlockObject *b,Py_ssize_t i,Py_ssize_t j,PyObject *v)
{ Py_ssize_t n,k;
  long *p=b->block;
  if(j>b->length/4) j=b->length/4;
  if(i<0||i>j)
  { PyErr_SetString(PyExc_IndexError,"block index out of range");
    return -1;
  }
  if(!PyList_Check(v)) goto fail;
  n=PyList_Size(v);
  if(n>j-i) n=j-i;
  for(k=0;k<n;k++)
  { PyObject *q=PyList_GetItem(v,k);
    if(!PyInt_Check(q)) goto fail;
    p[i+k]=PyInt_AsLong(q);
  }
  for(;k<j-i;k++) p[i+k]=0;
  return 0;
  fail:PyErr_SetString(PyExc_TypeError,"block slice must be integer list");
  return -1;
}

static PySequenceMethods block_as_sequence=
{ (inquiry)block_len,		/*sq_length*/
  (binaryfunc)block_concat,		/*sq_concat*/
  (ssizeargfunc)block_repeat,		/*sq_repeat*/
  (ssizeargfunc)block_item,		/*sq_item*/
  (ssizessizeargfunc)block_slice,		/*sq_slice*/
  (ssizeobjargproc)block_ass_item,	/*sq_ass_item*/
  (ssizessizeobjargproc)block_ass_slice,	/*sq_ass_slice*/
};

static PyObject *PyBlock_GetAttr(PyBlockObject *s,char *name)
{
  if (!strcmp(name, "length")) return PyInt_FromLong((long)s->length);
  if (!strcmp(name, "start")) return PyInt_FromLong((long)s->block);
  if (!strcmp(name,"end")) return PyInt_FromLong(((long)(s->block)+s->length));
  if (!strcmp(name, "__members__"))
  { PyObject *list = PyList_New(3);
    if (list)
    { PyList_SetItem(list, 0, PyString_FromString("length"));
      PyList_SetItem(list, 1, PyString_FromString("start"));
      PyList_SetItem(list, 2, PyString_FromString("end"));
      if (PyErr_Occurred()) { Py_DECREF(list);list = NULL;}
    }
    return list;
  }
  return Py_FindMethod(PyBlock_Methods, (PyObject*) s,name);
}

static void PyBlock_Dealloc(PyBlockObject *b)
{
    if(b->heap) {
        if (b->heap)
            ;
        else
            PyMem_DEL(b->block);
    }
  PyMem_DEL(b);
}

static PyTypeObject PyBlockType=
{ PyObject_HEAD_INIT(&PyType_Type)
  0,				/*ob_size*/
  "block",			/*tp_name*/
  sizeof(PyBlockObject),	/*tp_size*/
  0,				/*tp_itemsize*/
	/* methods */
  (destructor)PyBlock_Dealloc,	/*tp_dealloc*/
  0,				/*tp_print*/
  (getattrfunc)PyBlock_GetAttr,	/*tp_getattr*/
  0,				/*tp_setattr*/
  0,				/*tp_compare*/
  0,				/*tp_repr*/
  0,				/*tp_as_number*/
  &block_as_sequence,		/*tp_as_sequence*/
  0,				/*tp_as_mapping*/
  0,                            /*tp_hash*/
};

/* swi commands */

static PyObject *swi_swi(PyObject *self,PyObject *args)
{ PyObject *name,*format,*result,*v;
  int swino,carry,rno=0,j,n;
  char *swiname,*fmt,*outfmt;
  _kernel_swi_regs r;
  PyBlockObject *ao;
  if(args==NULL||!PyTuple_Check(args)||(n=PyTuple_Size(args))<2)
  { PyErr_BadArgument(); return NULL;}
  name=PyTuple_GetItem(args,0);
  if(!PyArg_Parse(name,"i",&swino))
  { PyErr_Clear();
    if(!PyArg_Parse(name,"s",&swiname)) return NULL;
    e=xos_swi_number_from_string(swiname,&swino);
    if(e) return swi_oserror();
  }
  format=PyTuple_GetItem(args,1);
  if(!PyArg_Parse(format,"s",&fmt)) return NULL;
  j=2;
  for(;;fmt++)
  { switch(*fmt)
    { case '.': rno++;continue;
      case ';':case 0: goto swicall;
      case '0':case '1':case '2':case '3':case '4':
      case '5':case '6':case '7':case '8':case '9':
        r.r[rno++]=*fmt-'0';continue;
      case '-':r.r[rno++]=-1;continue;
    }
    if(j>=n) return swi_error("Too few arguments");
    v=PyTuple_GetItem(args,j++);
    switch(*fmt)
    { case 'i':if(!PyArg_Parse(v,"i",&r.r[rno])) return NULL;
               break;
      case 's':if(!PyArg_Parse(v,"s",(char**)(&r.r[rno]))) return NULL;
               break;
      case 'b':if(!PyArg_Parse(v,"O",(PyObject**)&ao)) return NULL;
               if(!PyBlock_Check(v)) return swi_error("Not a block");
               r.r[rno]=(int)(ao->block);
               break;
      case 'e':if(!PyArg_Parse(v,"O",(PyObject**)&ao)) return NULL;
               if(!PyBlock_Check(v)) return swi_error("Not a block");
               r.r[rno]=(int)(ao->block)+ao->length;
               break;
      default:return swi_error("Odd format character");
    }
    rno++;
  }
  swicall:e=(os_error*)_kernel_swi_c(swino,&r,&r,&carry);
  if(e) return swi_oserror();
  if(*fmt==0) { Py_INCREF(Py_None);return Py_None;}
  n=0;
  for(outfmt=++fmt;*outfmt;outfmt++)  switch(*outfmt)
  { case 'i':case 's':case '*':n++;break;
    case '.':break;
    default:return swi_error("Odd format character");
  }
  if(n==0) { Py_INCREF(Py_None);return Py_None;}
  if(n!=1)
  { result=PyTuple_New(n);
    if(!result) return NULL;
  }
  rno=0;j=0;
  for(;*fmt;fmt++)
  {  switch(*fmt)
    { case 'i':v=PyInt_FromLong((long)r.r[rno++]); break;
      case 's':v=PyString_FromString((char*)(r.r[rno++])); break;
      case '.':rno++; continue;
      case '*':v=PyInt_FromLong((long)carry); break;
    }
    if(!v) goto fail;
    if(n==1) return v;
    PyTuple_SetItem(result,j,v);
    j++;
  }
  return result;
  fail:Py_DECREF(result);return 0;
}

static PyObject *swi_string(PyObject *self, PyObject *arg)
{ char *s;
  int l=-1;
  if(!PyArg_ParseTuple(arg,"i|i",(unsigned int *)&s, &l)) return NULL;
  if (l==-1)
    l = strlen(s);
  return PyString_FromStringAndSize((char*)s, l);
}

static char swi_string__doc__[] =
"string(address[, length]) -> string\n\
Read a null terminated string from the given address.";


static PyObject *swi_integer(PyObject *self, PyObject *arg)
{ int *i;

  if(!PyArg_ParseTuple(arg,"i",(unsigned int *)&i))
    return NULL;
  return PyInt_FromLong(*i);
}

static char swi_integer__doc__[] =
"integer(address) -> string\n\
Read an integer from the given address.";


static PyObject *swi_integers(PyObject *self, PyObject *arg)
{ int *i;
  int c=-1;
  PyObject *result, *result1;
  
  if(!PyArg_ParseTuple(arg,"i|i",(unsigned int *)&i, &c)) return NULL;
  result=PyList_New(0);
  if (result) {
    while ( c>0 || (c==-1 && *i) ) {
      result1 = PyInt_FromLong((long)*i);
      if (!result1) {
        Py_DECREF(result);
        return NULL;
      }
      if (PyList_Append(result, result1)!=0) {
        Py_DECREF(result);
        Py_DECREF(result1);
        return NULL;
      };
      i++;
      if (c!=-1)
        c--;
    }
  }
  return result;
}

static char swi_integers__doc__[] =
"integers(address[, count]) -> string\n\
Either read a null terminated list of integers or\n\
a list of given length from the given address.";


static PyObject *swi_tuples(PyObject *self, PyObject *arg)
{
  unsigned char *i;         /* points to current */
  int c=-1, l=4, j, zero;         /* count, length, index */
  PyObject *result, *result1, *result11;
  
  if(!PyArg_ParseTuple(arg,"i|ii",(unsigned int *)&i, &l, &c)) return NULL;
  result=PyList_New(0);
  if (result) {
    while (c) {
      result1 = PyTuple_New(l);
      if (!result1) {
        Py_DECREF(result);
        return NULL;
      }
      zero = (c==-1); /* check for zeros? */
      for(j=0;j<l;j++) {
        if (zero && *i)
          zero = 0;   /* non-zero found */
        result11 = PyInt_FromLong((long)(*i));
        if (!result11) {
          Py_DECREF(result);
          return NULL;
        }
        PyTuple_SetItem(result1, j, result11);
        i++;
      }
      if (c==-1 && zero) {
        Py_DECREF(result1);
        c = 0;
        break;
      }
      if (PyList_Append(result, result1)!=0) {
        Py_DECREF(result1);
        Py_DECREF(result);
        return NULL;
      }
      if (c!=-1)
        c--;
    }
  }
  return result;
}

static char swi_tuples__doc__[] =
"tuples(address[, length=4[, count]]) -> string\n\
Either read a null terminated list of byte tuples or\n\
a list of given length from the given address.";


static PyObject *swi_tuple(PyObject *self, PyObject *arg)
{
  unsigned char *i;         /* points to current */
  int c=1, j;
  PyObject *result, *result1;
  
  if(!PyArg_ParseTuple(arg,"i|i",(unsigned int *)&i, &c)) return NULL;
  result = PyTuple_New(c);
  if (!result)
    return NULL;
  for(j=0;j<c;j++) {
    result1 = PyInt_FromLong((long)(i[j]));
    if (!result1) {
      Py_DECREF(result);
      return NULL;
    }
    PyTuple_SetItem(result, j, result1);
  }
  return result;
}

static char swi_tuple__doc__[] =
"tuple(address[, count=1]]) -> tuple\n\
Read count bytes from given address.";


static PyMethodDef SwiMethods[]=
{ { "swi", swi_swi, METH_VARARGS},
  { "block", PyBlock_New, METH_VARARGS},
  { "register", PyRegister, METH_VARARGS},
  { "string", swi_string, METH_VARARGS, swi_string__doc__},
  { "integer", swi_integer, METH_VARARGS, swi_integer__doc__},
  { "integers", swi_integers, METH_VARARGS, swi_integers__doc__},
  { "tuples", swi_tuples, METH_VARARGS, swi_tuples__doc__},
  { "tuple", swi_tuple, METH_VARARGS, swi_tuple__doc__},
  { NULL,NULL,0,NULL}		 /* Sentinel */
};


void initswi()
{ PyObject *m, *d;
  m = Py_InitModule("swi", SwiMethods);
  d = PyModule_GetDict(m);
  SwiError=PyErr_NewException("swi.error", NULL, NULL);
  PyDict_SetItemString(d,"error",SwiError);
  ArgError=PyErr_NewException("swi.ArgError", NULL, NULL);
  PyDict_SetItemString(d,"ArgError",ArgError);
}
