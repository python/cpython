/* zlibmodule.c -- gzip-compatible data compression */

#include <Python.h>
#include <zlib.h>

/* The following parameters are copied from zutil.h, version 0.95 */
#define DEFLATED   8
#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
#define DEF_WBITS MAX_WBITS

/* The output buffer will be increased in chunks of ADDCHUNK bytes. */
#define ADDCHUNK 2048
#define PyInit_zlib initzlib

staticforward PyTypeObject Comptype;
staticforward PyTypeObject Decomptype;

static PyObject *ZlibError;

typedef struct 
{
  PyObject_HEAD
  z_stream zst;
} compobject;

static compobject *
newcompobject(type)
     PyTypeObject *type;
{
        compobject *self;
        self = PyObject_NEW(compobject, type);
        if (self == NULL)
                return NULL;
        return self;
}

static PyObject *
PyZlib_compress(self, args)
        PyObject *self;
        PyObject *args;
{
  PyObject *ReturnVal;
  Byte *input, *output;
  int length, level=Z_DEFAULT_COMPRESSION, err;
  z_stream zst;
  
  if (!PyArg_ParseTuple(args, "s#|i", &input, &length, &level))
    return NULL;
  zst.avail_out=length+length/1000+12+1;
  output=(Byte*)malloc(zst.avail_out);
  if (output==NULL) 
    {
      PyErr_SetString(PyExc_MemoryError,
                      "Can't allocate memory to compress data");
      return NULL;
    }
  zst.zalloc=(alloc_func)zst.zfree=(free_func)Z_NULL;
  zst.next_out=(Byte *)output;
  zst.next_in =(Byte *)input;
  zst.avail_in=length;
  err=deflateInit(&zst, level);
  switch(err) 
    {
    case(Z_OK):
      break;
    case(Z_MEM_ERROR):
      PyErr_SetString(PyExc_MemoryError,
                      "Out of memory while compressing data");
      free(output);
      return NULL;
      break;
    case(Z_STREAM_ERROR):
      PyErr_SetString(ZlibError,
                      "Bad compression level");
      free(output);
      return NULL;
      break;
    default:
      {
        char temp[500];
	if (zst.msg==Z_NULL) zst.msg="";
        sprintf(temp, "Error %i while compressing data [%s]", err, zst.msg);
        PyErr_SetString(ZlibError, temp);
        deflateEnd(&zst);
        free(output);
        return NULL;
      }
      break;
    }
 
  err=deflate(&zst, Z_FINISH);
  switch(err)
    {
    case(Z_STREAM_END):
      break;
      /* Are there other errors to be trapped here? */
    default: 
      {
         char temp[500];
	if (zst.msg==Z_NULL) zst.msg="";
         sprintf(temp, "Error %i while compressing data [%s]", err, zst.msg);
         PyErr_SetString(ZlibError, temp);
         deflateEnd(&zst);
         free(output);
         return NULL;
       }
     }
  err=deflateEnd(&zst);
  if (err!=Z_OK) 
    {
      char temp[500];
	if (zst.msg==Z_NULL) zst.msg="";
      sprintf(temp, "Error %i while finishing data compression [%s]",
              err, zst.msg);
      PyErr_SetString(ZlibError, temp);
      free(output);
      return NULL;
    }
  ReturnVal=PyString_FromStringAndSize(output, zst.total_out);
  free(output);
  return ReturnVal;
}

static PyObject *
PyZlib_decompress(self, args)
        PyObject *self;
        PyObject *args;
{
  PyObject *ReturnVal;
  Byte *input, *output;
  int length, err;
  z_stream zst;
  if (!PyArg_ParseTuple(args, "s#", &input, &length))
    return NULL;
  
  zst.avail_in=length;
  zst.avail_out=length=length*2;
  output=(Byte*)malloc(zst.avail_out);
  if (output==NULL) 
    {
      PyErr_SetString(PyExc_MemoryError,
                      "Can't allocate memory to decompress data");
      return NULL;
    }
  zst.zalloc=(alloc_func)zst.zfree=(free_func)Z_NULL;
  zst.next_out=(Byte *)output;
  zst.next_in =(Byte *)input;
  err=inflateInit(&zst);
  switch(err)
    {
    case(Z_OK):
      break;
    case(Z_MEM_ERROR):      
      PyErr_SetString(PyExc_MemoryError,
                      "Out of memory while decompressing data");
      free(output);
      return NULL;
    default:
      {
        char temp[500];
	if (zst.msg==Z_NULL) zst.msg="";
        sprintf(temp, "Error %i while preparing to decompress data [%s]",
                err, zst.msg);
        PyErr_SetString(ZlibError, temp);
        inflateEnd(&zst);
        free(output);
        return NULL;
      }
    }
  do 
    {
      err=inflate(&zst, Z_FINISH);
      switch(err) 
        {
        case(Z_OK):
        case(Z_STREAM_END):
          output=(Byte *)realloc(output, length+ADDCHUNK);
          if (output==NULL) 
            {
              PyErr_SetString(PyExc_MemoryError,
                              "Out of memory while decompressing data");
              inflateEnd(&zst);
              return NULL;
            }
          zst.next_out=output+length;
          zst.avail_out=ADDCHUNK;
          length += ADDCHUNK;
          break;
        default:
          {
            char temp[500];
	if (zst.msg==Z_NULL) zst.msg="";
            sprintf(temp, "Error %i while decompressing data: [%s]",
                    err, zst.msg);
            PyErr_SetString(ZlibError, temp);
            inflateEnd(&zst);
            return NULL;
          }
        }
    } while(err!=Z_STREAM_END);
  
  err=inflateEnd(&zst);
  if (err!=Z_OK) 
    {
      char temp[500];
	if (zst.msg==Z_NULL) zst.msg="";
      sprintf(temp, "Error %i while finishing data decompression [%s]",
              err, zst.msg);
      PyErr_SetString(ZlibError, temp);
      free(output);
      return NULL;
    }
  ReturnVal=PyString_FromStringAndSize(output, zst.total_out);
  free(output);
  return ReturnVal;
}

static PyObject *
PyZlib_compressobj(selfptr, args)
        PyObject *selfptr;
        PyObject *args;
{
  compobject *self;
  int level=Z_DEFAULT_COMPRESSION, method=DEFLATED;
  int wbits=MAX_WBITS, memLevel=DEF_MEM_LEVEL, strategy=0, err;
  /* XXX Argh!  Is there a better way to have multiple levels of */
  /* optional arguments? */
  if (!PyArg_ParseTuple(args, "iiiii", &level, &method, &wbits, &memLevel, &strategy))
    {
     PyErr_Clear();
     if (!PyArg_ParseTuple(args, "iiii", &level, &method, &wbits,
			   &memLevel))
       {     
	PyErr_Clear();
	if (!PyArg_ParseTuple(args, "iii", &level, &method, &wbits))
	  {
	   PyErr_Clear();
	   if (!PyArg_ParseTuple(args, "ii", &level, &method))
	     {
	      PyErr_Clear();
	      if (!PyArg_ParseTuple(args, "i", &level))
		{
		 PyErr_Clear();
		 if (!PyArg_ParseTuple(args, ""))
		   return (NULL);
		}
	     }
	  }
       }
    }
  self=newcompobject(&Comptype);
  if (self==NULL) return(NULL);
  self->zst.zalloc=(alloc_func)self->zst.zfree=(free_func)Z_NULL;
  err=deflateInit2(&self->zst, level, method, wbits, memLevel, strategy);
  switch(err)
    {
    case (Z_OK):
      return (PyObject*)self;
      break;
    case (Z_MEM_ERROR):
      PyErr_SetString(PyExc_MemoryError,
                      "Can't allocate memory for compression object");
      return NULL;
      break;
    case(Z_STREAM_ERROR):
      PyErr_SetString(PyExc_ValueError,
                      "Invalid compression level");
      return NULL;
      break;
    default:
      {
	char temp[500];
	if (self->zst.msg==Z_NULL) self->zst.msg="";
        sprintf(temp, "Error %i while creating compression object [%s]",
		err, self->zst.msg);
	PyErr_SetString(ZlibError, temp);
	return NULL;
	break;      
      }
    }
}

static PyObject *
PyZlib_decompressobj(selfptr, args)
        PyObject *selfptr;
        PyObject *args;
{
  int wbits=DEF_WBITS, err;
  compobject *self;
  if (!PyArg_ParseTuple(args, "|i", &wbits))
    {
     return NULL;
    }  
  self=newcompobject(&Decomptype);
  if (self==NULL) return(NULL);
  self->zst.zalloc=(alloc_func)self->zst.zfree=(free_func)Z_NULL;
  /* XXX If illegal values of wbits are allowed to get here, Python
     coredumps, instead of raising an exception as it should. 
     This is a bug in zlib 0.95; I have reported it. */
  err=inflateInit2(&self->zst, wbits);
  switch(err)
    {
    case (Z_OK):
      return (PyObject*)self;
      break;
    case (Z_MEM_ERROR):
      PyErr_SetString(PyExc_MemoryError,
                      "Can't allocate memory for decompression object");
      return NULL;
      break;
    default:
      {
	char temp[500];
	if (self->zst.msg==Z_NULL) self->zst.msg="";
        sprintf(temp, "Error %i while creating decompression object [%s]",
		err, self->zst.msg);	
	PyErr_SetString(ZlibError, temp);
	return NULL;
	break;      
      }
    }
}

static void
Comp_dealloc(self)
        compobject *self;
{
  int err;
  err=deflateEnd(&self->zst);  /* Deallocate zstream structure */
}

static void
Decomp_dealloc(self)
        compobject *self;
{
  int err;
  err=inflateEnd(&self->zst);	/* Deallocate zstream structure */
}

static PyObject *
PyZlib_objcompress(self, args)
        compobject *self;
        PyObject *args;
{
  int length=0, err, inplen;
  Byte *buf=NULL;
  PyObject *RetVal;
  Byte *input;
  
  if (!PyArg_ParseTuple(args, "s#", &input, &inplen))
    return NULL;
  self->zst.avail_in=inplen;
  self->zst.next_in=input;
  do 
    {
      buf=(Byte *)realloc(buf, length+ADDCHUNK);
      if (buf==NULL) 
	{
	  PyErr_SetString(PyExc_MemoryError,
			  "Can't allocate memory to compress data");
	  return NULL;
	}
      self->zst.next_out=buf+length;
      self->zst.avail_out=ADDCHUNK;
      length += ADDCHUNK;
      err=deflate(&(self->zst), Z_NO_FLUSH);
    } while (self->zst.avail_in!=0 && err==Z_OK);
  if (err!=Z_OK) 
    {
      char temp[500];
	if (self->zst.msg==Z_NULL) self->zst.msg="";
      sprintf(temp, "Error %i while compressing [%s]",
	      err, self->zst.msg);
      PyErr_SetString(ZlibError, temp);
      return NULL;
    }
  RetVal=PyString_FromStringAndSize(buf, self->zst.next_out-buf);
  free(buf);
  return RetVal;
}

static PyObject *
PyZlib_objdecompress(self, args)
        compobject *self;
        PyObject *args;
{
  int length=0, err, inplen;
  Byte *buf=NULL;
  PyObject *RetVal;
  Byte *input;
  if (!PyArg_ParseTuple(args, "s#", &input, &inplen))
    return NULL;
  self->zst.avail_in=inplen;
  self->zst.next_in=input;
  do 
    {
      buf=(Byte *)realloc(buf, length+ADDCHUNK);
      if (buf==NULL) 
	{
	  PyErr_SetString(PyExc_MemoryError,
			  "Can't allocate memory to decompress data");
	  return NULL;
	}
      self->zst.next_out=buf+length;
      self->zst.avail_out=ADDCHUNK;
      length += ADDCHUNK;
      err=inflate(&(self->zst), Z_NO_FLUSH);
    } while (self->zst.avail_in!=0 && err==Z_OK);
  if (err!=Z_OK && err!=Z_STREAM_END) 
    {
      char temp[500];
	if (self->zst.msg==Z_NULL) self->zst.msg="";
      sprintf(temp, "Error %i while decompressing [%s]",
	      err, self->zst.msg);
      PyErr_SetString(ZlibError, temp);
      return NULL;
    }
  RetVal=PyString_FromStringAndSize(buf, self->zst.next_out-buf);
  free(buf);
  return RetVal;
}

static PyObject *
PyZlib_flush(self, args)
        compobject *self;
        PyObject *args;
{
  int length=0, err;
  Byte *buf=NULL;
  PyObject *RetVal;
  
  if (!PyArg_NoArgs(args))
    return NULL;
  self->zst.avail_in=0;
  do 
    {
      buf=(Byte *)realloc(buf, length+ADDCHUNK);
      if (buf==NULL) 
	{
	  PyErr_SetString(PyExc_MemoryError,
			  "Can't allocate memory to compress data");
	  return NULL;
	}
      self->zst.next_out=buf+length;
      self->zst.avail_out=ADDCHUNK;
      length += ADDCHUNK;
      err=deflate(&(self->zst), Z_FINISH);
    } while (err==Z_OK);
  if (err!=Z_STREAM_END) 
    {
      char temp[500];
	if (self->zst.msg==Z_NULL) self->zst.msg="";
      sprintf(temp, "Error %i while compressing [%s]",
	      err, self->zst.msg);
      PyErr_SetString(ZlibError, temp);
      return NULL;
    }
  RetVal=PyString_FromStringAndSize(buf, self->zst.next_out-buf);
  free(buf);
  err=deflateEnd(&(self->zst));
  if (err!=Z_OK) 
    {
      char temp[500];
	if (self->zst.msg==Z_NULL) self->zst.msg="";
      sprintf(temp, "Error %i while flushing compression object [%s]",
	      err, self->zst.msg);
      PyErr_SetString(ZlibError, temp);
      return NULL;
    }
  return RetVal;
}

static PyObject *
PyZlib_unflush(self, args)
        compobject *self;
        PyObject *args;
{
  int length=0, err;
  Byte *buf=NULL;
  PyObject *RetVal;
  
  if (!PyArg_NoArgs(args))
    return NULL;
  self->zst.avail_in=0;
  do 
    {
      buf=(Byte *)realloc(buf, length+ADDCHUNK);
      if (buf==NULL) 
	{
	  PyErr_SetString(PyExc_MemoryError,
			  "Can't allocate memory to decompress data");
	  return NULL;
	}
      self->zst.next_out=buf+length;
      length += ADDCHUNK;
      err=inflate(&(self->zst), Z_FINISH);
    } while (err==Z_OK);
  if (err!=Z_STREAM_END) 
    {
      char temp[500];
	if (self->zst.msg==Z_NULL) self->zst.msg="";
      sprintf(temp, "Error %i while decompressing [%s]",
	      err, self->zst.msg);
      PyErr_SetString(ZlibError, temp);
      return NULL;
    }
  RetVal=PyString_FromStringAndSize(buf, self->zst.next_out - buf);
  free(buf);
  err=inflateEnd(&(self->zst));
  if (err!=Z_OK) 
    {
      char temp[500];
	if (self->zst.msg==Z_NULL) self->zst.msg="";
      sprintf(temp, "Error %i while flushing decompression object [%s]",
	      err, self->zst.msg);
      PyErr_SetString(ZlibError, temp);
      return NULL;
    }
  return RetVal;
}

static PyMethodDef comp_methods[] =
{
        {"compress", PyZlib_objcompress, 1},
        {"flush", PyZlib_flush, 0},
        {NULL, NULL}
};

static PyMethodDef Decomp_methods[] =
{
        {"decompress", PyZlib_objdecompress, 1},
        {"flush", PyZlib_unflush, 0},
        {NULL, NULL}
};

static PyObject *
Comp_getattr(self, name)
     compobject *self;
     char *name;
{
        return Py_FindMethod(comp_methods, (PyObject *)self, name);
}

static PyObject *
Decomp_getattr(self, name)
     compobject *self;
     char *name;
{
        return Py_FindMethod(Decomp_methods, (PyObject *)self, name);
}

static PyObject *
PyZlib_adler32(self, args)
     PyObject *self, *args;
{
 uLong adler32val=adler32(0L, Z_NULL, 0);
 Byte *buf;
 int len;
 
 if (!PyArg_ParseTuple(args, "s#|l", &buf, &len, &adler32val))
   {
    return NULL;
   }
 adler32val=adler32(adler32val, buf, len);
 return Py_BuildValue("l", adler32val);
}
     

static PyObject *
PyZlib_crc32(self, args)
     PyObject *self, *args;
{
 uLong crc32val=crc32(0L, Z_NULL, 0);
 Byte *buf;
 int len;
 if (!PyArg_ParseTuple(args, "s#|l", &buf, &len, &crc32val))
   {
    return NULL;
   }
 crc32val=crc32(crc32val, buf, len);
 return Py_BuildValue("l", crc32val);
}
     

static PyMethodDef zlib_methods[] =
{
	{"adler32", PyZlib_adler32, 1},	 
        {"compress", PyZlib_compress, 1},
        {"compressobj", PyZlib_compressobj, 1},
	{"crc32", PyZlib_crc32, 1},	 
        {"decompress", PyZlib_decompress, 1},
        {"decompressobj", PyZlib_decompressobj, 1},
        {NULL, NULL}
};

statichere PyTypeObject Comptype = {
        PyObject_HEAD_INIT(&PyType_Type)
        0,
        "Compress",
        sizeof(compobject),
        0,
        (destructor)Comp_dealloc,       /*tp_dealloc*/
        0,                              /*tp_print*/
        (getattrfunc)Comp_getattr,      /*tp_getattr*/
        0,                              /*tp_setattr*/
        0,                              /*tp_compare*/
        0,                              /*tp_repr*/
        0,                              /*tp_as_number*/
        0,                              /*tp_as_sequence*/
        0,                              /*tp_as_mapping*/
};

statichere PyTypeObject Decomptype = {
        PyObject_HEAD_INIT(&PyType_Type)
        0,
        "Decompress",
        sizeof(compobject),
        0,
        (destructor)Decomp_dealloc,     /*tp_dealloc*/
        0,                              /*tp_print*/
        (getattrfunc)Decomp_getattr,    /*tp_getattr*/
        0,                              /*tp_setattr*/
        0,                              /*tp_compare*/
        0,                              /*tp_repr*/
        0,                              /*tp_as_number*/
        0,                              /*tp_as_sequence*/
        0,                              /*tp_as_mapping*/
};

/* The following insint() routine was blatantly ripped off from 
   socketmodule.c */ 

/* Convenience routine to export an integer value.
   For simplicity, errors (which are unlikely anyway) are ignored. */
static void
insint(d, name, value)
     PyObject *d;
     char *name;
     int value;
{
	PyObject *v = PyInt_FromLong((long) value);
	if (v == NULL) {
		/* Don't bother reporting this error */
		PyErr_Clear();
	}
	else {
		PyDict_SetItemString(d, name, v);
		Py_DECREF(v);
	}
}

void
PyInit_zlib()
{
        PyObject *m, *d;
        m = Py_InitModule("zlib", zlib_methods);
        d = PyModule_GetDict(m);
        ZlibError = Py_BuildValue("s", "zlib.error");
        PyDict_SetItemString(d, "error", ZlibError);
	insint(d, "MAX_WBITS", MAX_WBITS);
	insint(d, "DEFLATED", DEFLATED);
	insint(d, "DEF_MEM_LEVEL", DEF_MEM_LEVEL);
	
        if (PyErr_Occurred())
                Py_FatalError("can't initialize module zlib");
}
