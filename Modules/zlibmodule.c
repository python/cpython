/* zlibmodule.c -- gzip-compatible data compression */

#include "Python.h"
#include "zlib.h"

/* The following parameters are copied from zutil.h, version 0.95 */
#define DEFLATED   8
#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
#define DEF_WBITS MAX_WBITS

/* The output buffer will be increased in chunks of ADDCHUNK bytes. */
#define DEFAULTALLOC 16*1024
#define PyInit_zlib initzlib

staticforward PyTypeObject Comptype;
staticforward PyTypeObject Decomptype;

static PyObject *ZlibError;

typedef struct 
{
  PyObject_HEAD
  z_stream zst;
} compobject;

static char compressobj__doc__[] = 
"compressobj() -- Return a compressor object.\n"
"compressobj(level) -- Return a compressor object, using the given compression level.\n"
;

static char decompressobj__doc__[] = 
"decompressobj() -- Return a decompressor object.\n"
"decompressobj(wbits) -- Return a decompressor object, setting the window buffer size to wbits.\n"
;

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

static char compress__doc__[] = 
"compress(string) -- Compress string using the default compression level, "
"returning a string containing compressed data.\n"
"compress(string, level) -- Compress string, using the chosen compression "
"level (from 1 to 9).  Return a string containing the compressed data.\n"
;

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
  zst.zalloc=(alloc_func)NULL;
  zst.zfree=(free_func)Z_NULL;
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
  ReturnVal=PyString_FromStringAndSize((char *)output, zst.total_out);
  free(output);
  return ReturnVal;
}

static char decompress__doc__[] = 
"decompress(string) -- Decompress the data in string, returning a string containing the decompressed data.\n"
"decompress(string, wbits) -- Decompress the data in string with a window buffer size of wbits.\n"
"decompress(string, wbits, bufsize) -- Decompress the data in string with a window buffer size of wbits and an initial output buffer size of bufsize.\n"
;

static PyObject *
PyZlib_decompress(self, args)
        PyObject *self;
        PyObject *args;
{
  PyObject *result_str;
  Byte *input;
  int length, err;
  int wsize=DEF_WBITS, r_strlen=DEFAULTALLOC;
  z_stream zst;
  if (!PyArg_ParseTuple(args, "s#|ii", &input, &length, &wsize, &r_strlen))
    return NULL;
  
  zst.avail_in=length;
  zst.avail_out=r_strlen;
  if (!(result_str = PyString_FromStringAndSize(NULL, r_strlen)))
  {
      PyErr_SetString(PyExc_MemoryError,
                      "Can't allocate memory to decompress data");
      return NULL;
  }
  zst.zalloc=(alloc_func)NULL;
  zst.zfree=(free_func)Z_NULL;
  zst.next_out=(Byte *)PyString_AsString(result_str);
  zst.next_in =(Byte *)input;
  err=inflateInit2(&zst, wsize);
  switch(err)
    {
    case(Z_OK):
      break;
    case(Z_MEM_ERROR):      
      PyErr_SetString(PyExc_MemoryError,
                      "Out of memory while decompressing data");
      return NULL;
    default:
      {
        char temp[500];
	if (zst.msg==Z_NULL) zst.msg="";
        sprintf(temp, "Error %i while preparing to decompress data [%s]",
                err, zst.msg);
        PyErr_SetString(ZlibError, temp);
        inflateEnd(&zst);
        return NULL;
      }
    }
  do 
    {
      err=inflate(&zst, Z_FINISH);
      switch(err) 
        {
        case(Z_STREAM_END):
	    break;
        case(Z_OK):
	    /* need more memory */
	    if (_PyString_Resize(&result_str, r_strlen << 1) == -1)
            {
              PyErr_SetString(PyExc_MemoryError,
                              "Out of memory while decompressing data");
              inflateEnd(&zst);
              return NULL;
            }
	    zst.next_out = (unsigned char *)PyString_AsString(result_str) + r_strlen;
	    zst.avail_out=r_strlen;
	    r_strlen = r_strlen << 1;
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
      return NULL;
    }
  _PyString_Resize(&result_str, zst.total_out);
  return result_str;
}

static PyObject *
PyZlib_compressobj(selfptr, args)
        PyObject *selfptr;
        PyObject *args;
{
  compobject *self;
  int level=Z_DEFAULT_COMPRESSION, method=DEFLATED;
  int wbits=MAX_WBITS, memLevel=DEF_MEM_LEVEL, strategy=0, err;

  if (!PyArg_ParseTuple(args, "|iiiii", &level, &method, &wbits,
			&memLevel, &strategy))
      return NULL;

  self=newcompobject(&Comptype);
  if (self==NULL) return(NULL);
  self->zst.zalloc=(alloc_func)NULL;
  self->zst.zfree=(free_func)Z_NULL;
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
  self->zst.zalloc=(alloc_func)NULL;
  self->zst.zfree=(free_func)Z_NULL;
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
  PyMem_DEL(self);
}

static void
Decomp_dealloc(self)
        compobject *self;
{
  int err;
  err=inflateEnd(&self->zst);	/* Deallocate zstream structure */
  PyMem_DEL(self);
}

static char comp_compress__doc__[] =
"compress(data) -- Return a string containing a compressed version of the data.\n\n"
"After calling this function, some of the input data may still\n"
"be stored in internal buffers for later processing.\n"
"Call the flush() method to clear these buffers."
;


static PyObject *
PyZlib_objcompress(self, args)
        compobject *self;
        PyObject *args;
{
  int err = Z_OK, inplen;
  int length = DEFAULTALLOC;
  PyObject *RetVal;
  Byte *input;
  
  if (!PyArg_ParseTuple(args, "s#", &input, &inplen))
      return NULL;
  self->zst.avail_in=inplen;
  self->zst.next_in=input;
  if (!(RetVal = PyString_FromStringAndSize(NULL, length))) {
      PyErr_SetString(PyExc_MemoryError,
		      "Can't allocate memory to compress data");
      return NULL;
  }
  self->zst.next_out = (unsigned char *)PyString_AsString(RetVal);
  self->zst.avail_out = length;
  while (self->zst.avail_in != 0 && err == Z_OK)
  {
      err = deflate(&(self->zst), Z_NO_FLUSH);
      if (self->zst.avail_out <= 0) {
	  if (_PyString_Resize(&RetVal, length << 1) == -1)  {
	      PyErr_SetString(PyExc_MemoryError,
			      "Can't allocate memory to compress data");
	      return NULL;
	  }
	  self->zst.next_out = (unsigned char *)PyString_AsString(RetVal) + length;
	  self->zst.avail_out = length;
	  length = length << 1;
      }
  }
  if (err != Z_OK) 
    {
      char temp[500];
      if (self->zst.msg==Z_NULL) self->zst.msg="";
      sprintf(temp, "Error %i while compressing [%s]",
	      err, self->zst.msg);
      PyErr_SetString(ZlibError, temp);
      return NULL;
    }
  _PyString_Resize(&RetVal, self->zst.total_out);
  return RetVal;
}

static char decomp_decompress__doc__[] =
"decompress(data) -- Return a string containing the decompressed version of the data.\n\n"
"After calling this function, some of the input data may still\n"
"be stored in internal buffers for later processing.\n"
"Call the flush() method to clear these buffers."
;

static PyObject *
PyZlib_objdecompress(self, args)
        compobject *self;
        PyObject *args;
{
  int length, err, inplen;
  PyObject *RetVal;
  Byte *input;
  if (!PyArg_ParseTuple(args, "s#", &input, &inplen))
    return NULL;
  RetVal = PyString_FromStringAndSize(NULL, DEFAULTALLOC);
  self->zst.avail_in=inplen;
  self->zst.next_in=input;
  self->zst.avail_out = length = DEFAULTALLOC;
  self->zst.next_out = (unsigned char *)PyString_AsString(RetVal);
  err = Z_OK;

  while (self->zst.avail_in != 0 && err == Z_OK)
  {
      err = inflate(&(self->zst), Z_NO_FLUSH);
      if (err == Z_OK && self->zst.avail_out <= 0) 
      {
	  if (_PyString_Resize(&RetVal, length << 1) == -1)
	  {
	      PyErr_SetString(PyExc_MemoryError,
			      "Can't allocate memory to compress data");
	      return NULL;
	  }
	  self->zst.next_out = (unsigned char *)PyString_AsString(RetVal) + length;
	  self->zst.avail_out = length;
	  length = length << 1;
      }
  }

  if (err!=Z_OK && err!=Z_STREAM_END) 
    {
      char temp[500];
	if (self->zst.msg==Z_NULL) self->zst.msg="";
      sprintf(temp, "Error %i while decompressing [%s]",
	      err, self->zst.msg);
      PyErr_SetString(ZlibError, temp);
      return NULL;
    }
  _PyString_Resize(&RetVal, self->zst.total_out);
  return RetVal;
}

static char comp_flush__doc__[] =
"flush() -- Return a string containing any remaining compressed data.  "
"The compressor object can no longer be used after this call."
;

static PyObject *
PyZlib_flush(self, args)
        compobject *self;
        PyObject *args;
{
  int length=DEFAULTALLOC, err = Z_OK;
  PyObject *RetVal;
  
  if (!PyArg_NoArgs(args))
      return NULL;
  self->zst.avail_in = 0;
  self->zst.next_in = Z_NULL;
  if (!(RetVal = PyString_FromStringAndSize(NULL, length))) {
      PyErr_SetString(PyExc_MemoryError,
		      "Can't allocate memory to compress data");
      return NULL;
  }
  self->zst.next_out = (unsigned char *)PyString_AsString(RetVal);
  self->zst.avail_out = length;
  while (err == Z_OK)
  {
      err = deflate(&(self->zst), Z_FINISH);
      if (self->zst.avail_out <= 0) {
	  if (_PyString_Resize(&RetVal, length << 1) == -1)  {
	      PyErr_SetString(PyExc_MemoryError,
			      "Can't allocate memory to compress data");
	      return NULL;
	  }
	  self->zst.next_out = (unsigned char *)PyString_AsString(RetVal) + length;
	  self->zst.avail_out = length;
	  length = length << 1;
      }
  }
  if (err!=Z_STREAM_END) {
      char temp[500];
      if (self->zst.msg==Z_NULL) self->zst.msg="";
      sprintf(temp, "Error %i while compressing [%s]",
	      err, self->zst.msg);
      PyErr_SetString(ZlibError, temp);
      return NULL;
  }
  err=deflateEnd(&(self->zst));
  if (err!=Z_OK) {
      char temp[500];
      if (self->zst.msg==Z_NULL) self->zst.msg="";
      sprintf(temp, "Error %i while flushing compression object [%s]",
	      err, self->zst.msg);
      PyErr_SetString(ZlibError, temp);
      return NULL;
  }
  _PyString_Resize(&RetVal,
		   (char *)self->zst.next_out - PyString_AsString(RetVal));
  return RetVal;
}

static char decomp_flush__doc__[] =
"flush() -- Return a string containing any remaining decompressed data.  "
"The decompressor object can no longer be used after this call."
;

static PyObject *
PyZlib_unflush(self, args)
        compobject *self;
        PyObject *args;
{
  int length=0, err;
  PyObject *RetVal;
  
  if (!PyArg_NoArgs(args))
      return NULL;
  if (!(RetVal = PyString_FromStringAndSize(NULL, DEFAULTALLOC)))
  {
      PyErr_SetString(PyExc_MemoryError,
		      "Can't allocate memory to decompress data");
      return NULL;
  }
  self->zst.avail_in=0;
  self->zst.next_out = (unsigned char *)PyString_AsString(RetVal);
  length = self->zst.avail_out = DEFAULTALLOC;

  err = Z_OK;
  while (err == Z_OK)
  {
      err = inflate(&(self->zst), Z_FINISH);
      if (err == Z_OK && self->zst.avail_out == 0) 
      {
	  if (_PyString_Resize(&RetVal, length << 1) == -1)
	  {
	      PyErr_SetString(PyExc_MemoryError,
			      "Can't allocate memory to decompress data");
	      return NULL;
	  }
	  self->zst.next_out = (unsigned char *)PyString_AsString(RetVal) + length;
	  self->zst.avail_out = length;
	  length = length << 1;
      }
  }
  if (err!=Z_STREAM_END) 
    {
      char temp[500];
      if (self->zst.msg==Z_NULL) self->zst.msg="";
      sprintf(temp, "Error %i while decompressing [%s]",
	      err, self->zst.msg);
      PyErr_SetString(ZlibError, temp);
      return NULL;
    }
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
  _PyString_Resize(&RetVal, 
		   (char *)self->zst.next_out - PyString_AsString(RetVal));
  return RetVal;
}

static PyMethodDef comp_methods[] =
{
        {"compress", (binaryfunc)PyZlib_objcompress, 1, comp_compress__doc__},
        {"flush", (binaryfunc)PyZlib_flush, 0, comp_flush__doc__},
        {NULL, NULL}
};

static PyMethodDef Decomp_methods[] =
{
        {"decompress", (binaryfunc)PyZlib_objdecompress, 1, decomp_decompress__doc__},
        {"flush", (binaryfunc)PyZlib_unflush, 0, decomp_flush__doc__},
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

static char adler32__doc__[] = 
"adler32(string) -- Compute an Adler-32 checksum of string, using "
"a default starting value, and returning an integer value.\n"
"adler32(string, value) -- Compute an Adler-32 checksum of string, using "
"the starting value provided, and returning an integer value\n"
;

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
     
static char crc32__doc__[] = 
"crc32(string) -- Compute a CRC-32 checksum of string, using "
"a default starting value, and returning an integer value.\n"
"crc32(string, value) -- Compute a CRC-32 checksum of string, using "
"the starting value provided, and returning an integer value.\n"
;

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
	{"adler32", (PyCFunction)PyZlib_adler32, 1, adler32__doc__},	 
        {"compress", (PyCFunction)PyZlib_compress, 1, compress__doc__},
        {"compressobj", (PyCFunction)PyZlib_compressobj, 1, compressobj__doc__},
	{"crc32", (PyCFunction)PyZlib_crc32, 1, crc32__doc__},	 
        {"decompress", (PyCFunction)PyZlib_decompress, 1, decompress__doc__},
        {"decompressobj", (PyCFunction)PyZlib_decompressobj, 1, decompressobj__doc__},
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

static char zlib_module_documentation[]=
"The functions in this module allow compression and decompression "
"using the zlib library, which is based on GNU zip.  \n\n"
"adler32(string) -- Compute an Adler-32 checksum.\n"
"adler32(string, start) -- Compute an Adler-32 checksum using a given starting value.\n"
"compress(string) -- Compress a string.\n"
"compress(string, level) -- Compress a string with the given level of compression (1--9).\n"
"compressobj([level]) -- Return a compressor object.\n"
"crc32(string) -- Compute a CRC-32 checksum.\n"
"crc32(string, start) -- Compute a CRC-32 checksum using a given starting value.\n"
"decompress(string,[wbites],[bufsize]) -- Decompresses a compressed string.\n"
"decompressobj([wbits]) -- Return a decompressor object (wbits=window buffer size).\n\n"
"Compressor objects support compress() and flush() methods; decompressor \n"
"objects support decompress() and flush()."
;

void
PyInit_zlib()
{
        PyObject *m, *d;
        m = Py_InitModule4("zlib", zlib_methods,
			   zlib_module_documentation,
			   (PyObject*)NULL,PYTHON_API_VERSION);
        d = PyModule_GetDict(m);
        ZlibError = Py_BuildValue("s", "zlib.error");
        PyDict_SetItemString(d, "error", ZlibError);
	insint(d, "MAX_WBITS", MAX_WBITS);
	insint(d, "DEFLATED", DEFLATED);
	insint(d, "DEF_MEM_LEVEL", DEF_MEM_LEVEL);
	
        if (PyErr_Occurred())
                Py_FatalError("can't initialize module zlib");
}
