/* zlibmodule.c -- gzip-compatible data compression */
/* See http://www.cdrom.com/pub/infozip/zlib/ */
/* See http://www.winimage.com/zLibDll for Windows */

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

/* The output buffer will be increased in chunks of DEFAULTALLOC bytes. */
#define DEFAULTALLOC (16*1024)
#define PyInit_zlib initzlib

staticforward PyTypeObject Comptype;
staticforward PyTypeObject Decomptype;

static PyObject *ZlibError;

typedef struct 
{
  PyObject_HEAD
  z_stream zst;
  PyObject *unused_data;
  int is_initialised;
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
newcompobject(PyTypeObject *type)
{
        compobject *self;
        self = PyObject_New(compobject, type);
        if (self == NULL)
                return NULL;
	self->is_initialised = 0;
	self->unused_data = PyString_FromString("");
        return self;
}

static char compress__doc__[] = 
"compress(string) -- Compress string using the default compression level, "
"returning a string containing compressed data.\n"
"compress(string, level) -- Compress string, using the chosen compression "
"level (from 1 to 9).  Return a string containing the compressed data.\n"
;

static PyObject *
PyZlib_compress(PyObject *self, PyObject *args)
{
  PyObject *ReturnVal;
  Byte *input, *output;
  int length, level=Z_DEFAULT_COMPRESSION, err;
  z_stream zst;
  
  if (!PyArg_ParseTuple(args, "s#|i:compress", &input, &length, &level))
    return NULL;
  zst.avail_out = length + length/1000 + 12 + 1;
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
    case(Z_STREAM_ERROR):
      PyErr_SetString(ZlibError,
                      "Bad compression level");
      free(output);
      return NULL;
    default:
      {
	if (zst.msg == Z_NULL)
	    PyErr_Format(ZlibError, "Error %i while compressing data",
			 err); 
	else
	    PyErr_Format(ZlibError, "Error %i while compressing data: %.200s",
			 err, zst.msg);  
        deflateEnd(&zst);
        free(output);
        return NULL;
      }
    }
 
  err=deflate(&zst, Z_FINISH);
  switch(err)
  {
    case(Z_STREAM_END):
      break;
      /* Are there other errors to be trapped here? */
    default: 
    {
	if (zst.msg == Z_NULL)
	    PyErr_Format(ZlibError, "Error %i while compressing data",
			 err); 
	else
	    PyErr_Format(ZlibError, "Error %i while compressing data: %.200s",
			 err, zst.msg);  
	deflateEnd(&zst);
	free(output);
	return NULL;
    }
  }
  err=deflateEnd(&zst);
  if (err!=Z_OK) 
  {
      if (zst.msg == Z_NULL)
	  PyErr_Format(ZlibError, "Error %i while finishing compression",
		       err); 
      else
	  PyErr_Format(ZlibError,
		       "Error %i while finishing compression: %.200s",
		       err, zst.msg);  
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
PyZlib_decompress(PyObject *self, PyObject *args)
{
  PyObject *result_str;
  Byte *input;
  int length, err;
  int wsize=DEF_WBITS, r_strlen=DEFAULTALLOC;
  z_stream zst;
  if (!PyArg_ParseTuple(args, "s#|ii:decompress", &input, &length, &wsize, &r_strlen))
    return NULL;

  if (r_strlen <= 0)
      r_strlen = 1;

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
      Py_DECREF(result_str);
      return NULL;
    default:
      {
	if (zst.msg == Z_NULL)
	    PyErr_Format(ZlibError, "Error %i preparing to decompress data",
			 err); 
	else
	    PyErr_Format(ZlibError,
			 "Error %i while preparing to decompress data: %.200s",
			 err, zst.msg);  
        inflateEnd(&zst);
	Py_DECREF(result_str);
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
	case(Z_BUF_ERROR):
	    /*
	     * If there is at least 1 byte of room according to zst.avail_out
	     * and we get this error, assume that it means zlib cannot
	     * process the inflate call() due to an error in the data.
	     */
	    if (zst.avail_out > 0)
            {
              PyErr_Format(ZlibError, "Error %i while decompressing data",
                           err);
              inflateEnd(&zst);
              Py_DECREF(result_str);
              return NULL;
            }
	    /* fall through */
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
	      if (zst.msg == Z_NULL)
		  PyErr_Format(ZlibError, "Error %i while decompressing data",
			       err); 
	      else
		  PyErr_Format(ZlibError,
			       "Error %i while decompressing data: %.200s",
			       err, zst.msg);  
            inflateEnd(&zst);
	    Py_DECREF(result_str);
            return NULL;
          }
        }
    } while(err!=Z_STREAM_END);
  
  err=inflateEnd(&zst);
  if (err!=Z_OK) 
  {
      if (zst.msg == Z_NULL)
	  PyErr_Format(ZlibError,
		       "Error %i while finishing data decompression",
		       err); 
      else
	  PyErr_Format(ZlibError,
		       "Error %i while finishing data decompression: %.200s",
		       err, zst.msg);  
      Py_DECREF(result_str);
      return NULL;
    }
  _PyString_Resize(&result_str, zst.total_out);
  return result_str;
}

static PyObject *
PyZlib_compressobj(PyObject *selfptr, PyObject *args)
{
  compobject *self;
  int level=Z_DEFAULT_COMPRESSION, method=DEFLATED;
  int wbits=MAX_WBITS, memLevel=DEF_MEM_LEVEL, strategy=0, err;

  if (!PyArg_ParseTuple(args, "|iiiii:compressobj", &level, &method, &wbits,
			&memLevel, &strategy))
      return NULL;

  self = newcompobject(&Comptype);
  if (self==NULL) return(NULL);
  self->zst.zalloc = (alloc_func)NULL;
  self->zst.zfree = (free_func)Z_NULL;
  err = deflateInit2(&self->zst, level, method, wbits, memLevel, strategy);
  switch(err)
    {
    case (Z_OK):
      self->is_initialised = 1;
      return (PyObject*)self;
    case (Z_MEM_ERROR):
      Py_DECREF(self);
      PyErr_SetString(PyExc_MemoryError,
                      "Can't allocate memory for compression object");
      return NULL;
    case(Z_STREAM_ERROR):
      Py_DECREF(self);
      PyErr_SetString(PyExc_ValueError,
                      "Invalid initialization option");
      return NULL;
    default:
      {
	if (self->zst.msg == Z_NULL)
	    PyErr_Format(ZlibError,
			 "Error %i while creating compression object",
			 err); 
	else
	    PyErr_Format(ZlibError,
			 "Error %i while creating compression object: %.200s",
			 err, self->zst.msg);  
        Py_DECREF(self);
	return NULL;
      }
    }
}

static PyObject *
PyZlib_decompressobj(PyObject *selfptr, PyObject *args)
{
  int wbits=DEF_WBITS, err;
  compobject *self;
  if (!PyArg_ParseTuple(args, "|i:decompressobj", &wbits))
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
      self->is_initialised = 1;
      return (PyObject*)self;
    case(Z_STREAM_ERROR):
      Py_DECREF(self);
      PyErr_SetString(PyExc_ValueError,
                      "Invalid initialization option");
      return NULL;
    case (Z_MEM_ERROR):
      Py_DECREF(self);
      PyErr_SetString(PyExc_MemoryError,
                      "Can't allocate memory for decompression object");
      return NULL;
    default:
    {
	if (self->zst.msg == Z_NULL)
	    PyErr_Format(ZlibError,
			 "Error %i while creating decompression object",
			 err); 
	else
	    PyErr_Format(ZlibError,
			 "Error %i while creating decompression object: %.200s",
			 err, self->zst.msg);  
        Py_DECREF(self);
	return NULL;
    }
  }
}

static void
Comp_dealloc(compobject *self)
{
    if (self->is_initialised)
      deflateEnd(&self->zst);
    Py_XDECREF(self->unused_data);
    PyObject_Del(self);
}

static void
Decomp_dealloc(compobject *self)
{
    inflateEnd(&self->zst);
    Py_XDECREF(self->unused_data);
    PyObject_Del(self);
}

static char comp_compress__doc__[] =
"compress(data) -- Return a string containing a compressed version of the data.\n\n"
"After calling this function, some of the input data may still\n"
"be stored in internal buffers for later processing.\n"
"Call the flush() method to clear these buffers."
;


static PyObject *
PyZlib_objcompress(compobject *self, PyObject *args)
{
  int err = Z_OK, inplen;
  int length = DEFAULTALLOC;
  PyObject *RetVal;
  Byte *input;
  unsigned long start_total_out;
  
  if (!PyArg_ParseTuple(args, "s#:compress", &input, &inplen))
      return NULL;
  self->zst.avail_in = inplen;
  self->zst.next_in = input;
  if (!(RetVal = PyString_FromStringAndSize(NULL, length))) {
      PyErr_SetString(PyExc_MemoryError,
		      "Can't allocate memory to compress data");
      return NULL;
  }
  start_total_out = self->zst.total_out;
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
      if (self->zst.msg == Z_NULL)
	  PyErr_Format(ZlibError, "Error %i while compressing",
		       err); 
      else
	  PyErr_Format(ZlibError, "Error %i while compressing: %.200s",
		       err, self->zst.msg);  
      Py_DECREF(RetVal);
      return NULL;
    }
  _PyString_Resize(&RetVal, self->zst.total_out - start_total_out);
  return RetVal;
}

static char decomp_decompress__doc__[] =
"decompress(data) -- Return a string containing the decompressed version of the data.\n\n"
"After calling this function, some of the input data may still\n"
"be stored in internal buffers for later processing.\n"
"Call the flush() method to clear these buffers."
;

static PyObject *
PyZlib_objdecompress(compobject *self, PyObject *args)
{
  int length, err, inplen;
  PyObject *RetVal;
  Byte *input;
  unsigned long start_total_out;

  if (!PyArg_ParseTuple(args, "s#:decompress", &input, &inplen))
    return NULL;
  start_total_out = self->zst.total_out;
  RetVal = PyString_FromStringAndSize(NULL, DEFAULTALLOC);
  self->zst.avail_in = inplen;
  self->zst.next_in = input;
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

  if (err != Z_OK && err != Z_STREAM_END) 
  {
      if (self->zst.msg == Z_NULL)
	  PyErr_Format(ZlibError, "Error %i while decompressing",
		       err); 
      else
	  PyErr_Format(ZlibError, "Error %i while decompressing: %.200s",
		       err, self->zst.msg);  
      Py_DECREF(RetVal);
      return NULL;
    }

  if (err == Z_STREAM_END)
  {
      /* The end of the compressed data has been reached, so set 
         the unused_data attribute to a string containing the 
         remainder of the data in the string. */
      int pos = self->zst.next_in - input;  /* Position in the string */
      Py_XDECREF(self->unused_data);  /* Free the original, empty string */

      self->unused_data = PyString_FromStringAndSize((char *)input+pos,
						     inplen-pos);
      if (self->unused_data == NULL) return NULL;
  }

  _PyString_Resize(&RetVal, self->zst.total_out - start_total_out);
  return RetVal;
}

static char comp_flush__doc__[] =
"flush( [mode] ) -- Return a string containing any remaining compressed data.\n"
"mode can be one of the constants Z_SYNC_FLUSH, Z_FULL_FLUSH, Z_FINISH; the \n"
"default value used when mode is not specified is Z_FINISH.\n"
"If mode == Z_FINISH, the compressor object can no longer be used after\n"
"calling the flush() method.  Otherwise, more data can still be compressed.\n"
;

static PyObject *
PyZlib_flush(compobject *self, PyObject *args)
{
  int length=DEFAULTALLOC, err = Z_OK;
  PyObject *RetVal;
  int flushmode = Z_FINISH;
  unsigned long start_total_out;

  if (!PyArg_ParseTuple(args, "|i:flush", &flushmode))
      return NULL;

  /* Flushing with Z_NO_FLUSH is a no-op, so there's no point in
     doing any work at all; just return an empty string. */
  if (flushmode == Z_NO_FLUSH)
    {
      return PyString_FromStringAndSize(NULL, 0);
    }
  
  self->zst.avail_in = 0;
  self->zst.next_in = Z_NULL;
  if (!(RetVal = PyString_FromStringAndSize(NULL, length))) {
      PyErr_SetString(PyExc_MemoryError,
		      "Can't allocate memory to compress data");
      return NULL;
  }
  start_total_out = self->zst.total_out;
  self->zst.next_out = (unsigned char *)PyString_AsString(RetVal);
  self->zst.avail_out = length;

  /* When flushing the zstream, there's no input data.  
     If zst.avail_out == 0, that means that more output space is
     needed to complete the flush operation. */ 
  while (1) {
      err = deflate(&(self->zst), flushmode);

      /* If the output is Z_OK, and there's still room in the output
	 buffer, then the flush is complete. */
      if ( (err == Z_OK) && self->zst.avail_out > 0) break;

      /* A nonzero return indicates some sort of error (but see 
	 the comment for the error handler below) */
      if ( err != Z_OK ) break;

      /* There's no space left for output, so increase the buffer and loop 
	 again */
      if (_PyString_Resize(&RetVal, length << 1) == -1)  {
	PyErr_SetString(PyExc_MemoryError,
			"Can't allocate memory to compress data");
	return NULL;
      }
      self->zst.next_out = (unsigned char *)PyString_AsString(RetVal) + length;
      self->zst.avail_out = length;
      length = length << 1;
  }

  /* Raise an exception indicating an error.  The condition for
     detecting a error is kind of complicated; Z_OK indicates no
     error, but if the flushmode is Z_FINISH, then Z_STREAM_END is
     also not an error. */
  if (err!=Z_OK && !(flushmode == Z_FINISH && err == Z_STREAM_END) )
  {
      if (self->zst.msg == Z_NULL)
	  PyErr_Format(ZlibError, "Error %i while flushing",
		       err); 
      else
	  PyErr_Format(ZlibError, "Error %i while flushing: %.200s",
		       err, self->zst.msg);  
      Py_DECREF(RetVal);
      return NULL;
  }

  /* If flushmode is Z_FINISH, we also have to call deflateEnd() to
     free various data structures */

  if (flushmode == Z_FINISH) {
    err=deflateEnd(&(self->zst));
    if (err!=Z_OK) {
      if (self->zst.msg == Z_NULL)
	PyErr_Format(ZlibError, "Error %i from deflateEnd()",
		     err); 
      else
	PyErr_Format(ZlibError,
		     "Error %i from deflateEnd(): %.200s",
		     err, self->zst.msg);  
      Py_DECREF(RetVal);
      return NULL;
    }
  }
  _PyString_Resize(&RetVal, self->zst.total_out - start_total_out);
  return RetVal;
}

static char decomp_flush__doc__[] =
"flush() -- Return a string containing any remaining decompressed data.  "
"The decompressor object can no longer be used after this call."
;

static PyObject *
PyZlib_unflush(compobject *self, PyObject *args)
{
  int length=0, err;
  PyObject *RetVal;
  
  if (!PyArg_ParseTuple(args, ""))
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

  /* I suspect that Z_BUF_ERROR is the only error code we need to check for 
     in the following loop, but will leave the Z_OK in for now to avoid
     destabilizing this function. --amk */
  err = Z_OK;
  while ( err == Z_OK )
  {
      err = inflate(&(self->zst), Z_FINISH);
      if ( ( err == Z_OK || err == Z_BUF_ERROR ) && self->zst.avail_out == 0)
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
	  err = Z_OK;
      }
  }
  if (err!=Z_STREAM_END) 
  {
      if (self->zst.msg == Z_NULL)
	  PyErr_Format(ZlibError, "Error %i while decompressing",
		       err); 
      else
	  PyErr_Format(ZlibError, "Error %i while decompressing: %.200s",
		       err, self->zst.msg);  
      Py_DECREF(RetVal);
      return NULL;
  }
  err=inflateEnd(&(self->zst));
  if (err!=Z_OK) 
  {
      if (self->zst.msg == Z_NULL)
	  PyErr_Format(ZlibError,
		       "Error %i while flushing decompression object",
		       err); 
      else
	  PyErr_Format(ZlibError,
		       "Error %i while flushing decompression object: %.200s",
		       err, self->zst.msg);  
      Py_DECREF(RetVal);
      return NULL;
  }
  _PyString_Resize(&RetVal, 
		   (char *)self->zst.next_out - PyString_AsString(RetVal));
  return RetVal;
}

static PyMethodDef comp_methods[] =
{
        {"compress", (binaryfunc)PyZlib_objcompress, 
	 METH_VARARGS, comp_compress__doc__},
        {"flush", (binaryfunc)PyZlib_flush, 
	 METH_VARARGS, comp_flush__doc__},
        {NULL, NULL}
};

static PyMethodDef Decomp_methods[] =
{
        {"decompress", (binaryfunc)PyZlib_objdecompress, 
	 METH_VARARGS, decomp_decompress__doc__},
        {"flush", (binaryfunc)PyZlib_unflush, 
	 METH_VARARGS, decomp_flush__doc__},
        {NULL, NULL}
};

static PyObject *
Comp_getattr(compobject *self, char *name)
{
        return Py_FindMethod(comp_methods, (PyObject *)self, name);
}

static PyObject *
Decomp_getattr(compobject *self, char *name)
{
        if (strcmp(name, "unused_data") == 0) 
	  {  
	    Py_INCREF(self->unused_data);
	    return self->unused_data;
	  }
        return Py_FindMethod(Decomp_methods, (PyObject *)self, name);
}

static char adler32__doc__[] = 
"adler32(string) -- Compute an Adler-32 checksum of string, using "
"a default starting value, and returning an integer value.\n"
"adler32(string, value) -- Compute an Adler-32 checksum of string, using "
"the starting value provided, and returning an integer value\n"
;

static PyObject *
PyZlib_adler32(PyObject *self, PyObject *args)
{
    uLong adler32val=adler32(0L, Z_NULL, 0);
    Byte *buf;
    int len;
 
    if (!PyArg_ParseTuple(args, "s#|l:adler32", &buf, &len, &adler32val))
    {
	return NULL;
    }
    adler32val = adler32(adler32val, buf, len);
    return PyInt_FromLong(adler32val);
}
     
static char crc32__doc__[] = 
"crc32(string) -- Compute a CRC-32 checksum of string, using "
"a default starting value, and returning an integer value.\n"
"crc32(string, value) -- Compute a CRC-32 checksum of string, using "
"the starting value provided, and returning an integer value.\n"
;

static PyObject *
PyZlib_crc32(PyObject *self, PyObject *args)
{
    uLong crc32val=crc32(0L, Z_NULL, 0);
    Byte *buf;
    int len;
    if (!PyArg_ParseTuple(args, "s#|l:crc32", &buf, &len, &crc32val))
    {
	return NULL;
    }
    crc32val = crc32(crc32val, buf, len);
    return PyInt_FromLong(crc32val);
}
     

static PyMethodDef zlib_methods[] =
{
	{"adler32", (PyCFunction)PyZlib_adler32, 
	 METH_VARARGS, adler32__doc__},	 
	{"compress", (PyCFunction)PyZlib_compress, 
	 METH_VARARGS, compress__doc__},
	{"compressobj", (PyCFunction)PyZlib_compressobj, 
	 METH_VARARGS, compressobj__doc__},
	{"crc32", (PyCFunction)PyZlib_crc32, 
	 METH_VARARGS, crc32__doc__},	 
	{"decompress", (PyCFunction)PyZlib_decompress, 
	 METH_VARARGS, decompress__doc__},
	{"decompressobj", (PyCFunction)PyZlib_decompressobj, 
	 METH_VARARGS, decompressobj__doc__},
	{NULL, NULL}
};

statichere PyTypeObject Comptype = {
        PyObject_HEAD_INIT(0)
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
        PyObject_HEAD_INIT(0)
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
insint(PyObject *d, char *name, int value)
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
"decompress(string,[wbits],[bufsize]) -- Decompresses a compressed string.\n"
"decompressobj([wbits]) -- Return a decompressor object (wbits=window buffer size).\n\n"
"Compressor objects support compress() and flush() methods; decompressor \n"
"objects support decompress() and flush()."
;

DL_EXPORT(void)
PyInit_zlib(void)
{
        PyObject *m, *d, *ver;
        Comptype.ob_type = &PyType_Type;
        Decomptype.ob_type = &PyType_Type;
        m = Py_InitModule4("zlib", zlib_methods,
			   zlib_module_documentation,
			   (PyObject*)NULL,PYTHON_API_VERSION);
        d = PyModule_GetDict(m);
        ZlibError = PyErr_NewException("zlib.error", NULL, NULL);
        if (ZlibError != NULL)
                PyDict_SetItemString(d, "error", ZlibError);

	insint(d, "MAX_WBITS", MAX_WBITS);
	insint(d, "DEFLATED", DEFLATED);
	insint(d, "DEF_MEM_LEVEL", DEF_MEM_LEVEL);
	insint(d, "Z_BEST_SPEED", Z_BEST_SPEED);
	insint(d, "Z_BEST_COMPRESSION", Z_BEST_COMPRESSION);
	insint(d, "Z_DEFAULT_COMPRESSION", Z_DEFAULT_COMPRESSION);
	insint(d, "Z_FILTERED", Z_FILTERED);
	insint(d, "Z_HUFFMAN_ONLY", Z_HUFFMAN_ONLY);
	insint(d, "Z_DEFAULT_STRATEGY", Z_DEFAULT_STRATEGY);

	insint(d, "Z_FINISH", Z_FINISH);
	insint(d, "Z_NO_FLUSH", Z_NO_FLUSH);
	insint(d, "Z_SYNC_FLUSH", Z_SYNC_FLUSH);
	insint(d, "Z_FULL_FLUSH", Z_FULL_FLUSH);

	ver = PyString_FromString(ZLIB_VERSION);
        if (ver != NULL) {
                PyDict_SetItemString(d, "ZLIB_VERSION", ver);
                Py_DECREF(ver);
        }
}
