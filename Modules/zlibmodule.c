/* zlibmodule.c -- gzip-compatible data compression */
/* See http://www.gzip.org/zlib/ */

/* Windows users:  read Python's PCbuild\readme.txt */


#include "Python.h"
#include "zlib.h"

#ifdef WITH_THREAD
#include "pythread.h"

/* #defs ripped off from _tkinter.c, even though the situation here is much
   simpler, because we don't have to worry about waiting for Tcl
   events!  And, since zlib itself is threadsafe, we don't need to worry
   about re-entering zlib functions.

   What we _do_ have to worry about is releasing the global lock _in
   general_ in the zlibmodule functions, because of all the calls to
   Python functions, which assume that the global lock is held.  So
   only two types of calls are wrapped in Py_BEGIN/END_ALLOW_THREADS:
   those that grab the zlib lock, and those that involve other
   time-consuming functions where we need to worry about holding up
   other Python threads.

   We don't need to worry about the string inputs being modified out
   from underneath us, because string objects are immutable.  However,
   we do need to make sure we take on ownership, so that the strings
   are not deleted out from under us during a thread swap.

   N.B.

   Since ENTER_ZLIB and LEAVE_ZLIB only need to be called on functions
   that modify the components of preexisting de/compress objects, it
   could prove to be a performance gain on multiprocessor machines if
   there was an de/compress object-specific lock.  However, for the
   moment the ENTER_ZLIB and LEAVE_ZLIB calls are global for ALL
   de/compress objects.

 */

static PyThread_type_lock zlib_lock = NULL; /* initialized on module load */

#define ENTER_ZLIB \
       { Py_BEGIN_ALLOW_THREADS PyThread_acquire_lock(zlib_lock, 1); \
         Py_END_ALLOW_THREADS

#define LEAVE_ZLIB \
       PyThread_release_lock(zlib_lock); }

#else

#define ENTER_ZLIB
#define LEAVE_ZLIB

#endif

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
    PyObject *unconsumed_tail;
    int is_initialised;
} compobject;

static void
zlib_error(z_stream zst, int err, char *msg)
{
    if (zst.msg == Z_NULL)
	PyErr_Format(ZlibError, "Error %d %s", err, msg);
    else
	PyErr_Format(ZlibError, "Error %d %s: %.200s", err, msg, zst.msg);
}

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
    if (self->unused_data == NULL) {
	Py_DECREF(self);
	return NULL;
    }
    self->unconsumed_tail = PyString_FromString("");
    if (self->unconsumed_tail == NULL) {
	Py_DECREF(self);
	return NULL;
    }
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
    PyObject *ReturnVal = NULL;
    Byte *input, *output;
    int length, level=Z_DEFAULT_COMPRESSION, err;
    z_stream zst;
    int return_error;
    PyObject * inputString;
  
    /* require Python string object, optional 'level' arg */
    if (!PyArg_ParseTuple(args, "S|i:compress", &inputString, &level))
	return NULL;

    /* now get a pointer to the internal string */
    if (PyString_AsStringAndSize(inputString, (char**)&input, &length) == -1)
	return NULL;

    zst.avail_out = length + length/1000 + 12 + 1;

    output=(Byte*)malloc(zst.avail_out);
    if (output==NULL) 
    {
	PyErr_SetString(PyExc_MemoryError,
			"Can't allocate memory to compress data");
	return NULL;
    }

    /* Past the point of no return.  From here on out, we need to make sure
       we clean up mallocs & INCREFs. */

    Py_INCREF(inputString);	/* increment so that we hold ref */

    zst.zalloc=(alloc_func)NULL;
    zst.zfree=(free_func)Z_NULL;
    zst.next_out=(Byte *)output;
    zst.next_in =(Byte *)input;
    zst.avail_in=length;
    err=deflateInit(&zst, level);

    return_error = 0;
    switch(err) 
    {
    case(Z_OK):
	break;
    case(Z_MEM_ERROR):
	PyErr_SetString(PyExc_MemoryError,
			"Out of memory while compressing data");
	return_error = 1;
	break;
    case(Z_STREAM_ERROR):
	PyErr_SetString(ZlibError,
			"Bad compression level");
	return_error = 1;
	break;
    default:
        deflateEnd(&zst);
	zlib_error(zst, err, "while compressing data");
	return_error = 1;
    }

    if (!return_error) {
	Py_BEGIN_ALLOW_THREADS;
        err = deflate(&zst, Z_FINISH);
	Py_END_ALLOW_THREADS;

	if (err != Z_STREAM_END) {
	    zlib_error(zst, err, "while compressing data");
	    deflateEnd(&zst);
	    return_error = 1;
	}
    
	if (!return_error) {
	    err=deflateEnd(&zst);
	    if (err == Z_OK)
		ReturnVal = PyString_FromStringAndSize((char *)output, 
						       zst.total_out);
	    else 
		zlib_error(zst, err, "while finishing compression");
	}
    }

    free(output);
    Py_DECREF(inputString);

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
    int return_error;
    PyObject * inputString;

    if (!PyArg_ParseTuple(args, "S|ii:decompress", 
			  &inputString, &wsize, &r_strlen))
	return NULL;
    if (PyString_AsStringAndSize(inputString, (char**)&input, &length) == -1)
	return NULL;

    if (r_strlen <= 0)
	r_strlen = 1;

    zst.avail_in = length;
    zst.avail_out = r_strlen;

    if (!(result_str = PyString_FromStringAndSize(NULL, r_strlen)))
	return NULL;

    /* Past the point of no return.  From here on out, we need to make sure
       we clean up mallocs & INCREFs. */

    Py_INCREF(inputString);	/* increment so that we hold ref */

    zst.zalloc = (alloc_func)NULL;
    zst.zfree = (free_func)Z_NULL;
    zst.next_out = (Byte *)PyString_AsString(result_str);
    zst.next_in = (Byte *)input;
    err = inflateInit2(&zst, wsize);

    return_error = 0;
    switch(err) {
    case(Z_OK):
	break;
    case(Z_MEM_ERROR):      
	PyErr_SetString(PyExc_MemoryError,
			"Out of memory while decompressing data");
	return_error = 1;
    default:
        inflateEnd(&zst);
	zlib_error(zst, err, "while preparing to decompress data");
	return_error = 1;
    }

    do {
	if (return_error)
	    break;

	Py_BEGIN_ALLOW_THREADS
	err=inflate(&zst, Z_FINISH);
	Py_END_ALLOW_THREADS

	switch(err) {
	case(Z_STREAM_END):
	    break;
	case(Z_BUF_ERROR):
	    /*
	     * If there is at least 1 byte of room according to zst.avail_out
	     * and we get this error, assume that it means zlib cannot
	     * process the inflate call() due to an error in the data.
	     */
	    if (zst.avail_out > 0) {
		PyErr_Format(ZlibError, "Error %i while decompressing data",
			     err);
		inflateEnd(&zst);
		return_error = 1;
		break;
	    }
	    /* fall through */
	case(Z_OK):
	    /* need more memory */
	    if (_PyString_Resize(&result_str, r_strlen << 1) == -1) {
		inflateEnd(&zst);
		result_str = NULL;
		return_error = 1;
	    }
	    zst.next_out = (unsigned char *)PyString_AsString(result_str) \
		+ r_strlen;
	    zst.avail_out = r_strlen;
	    r_strlen = r_strlen << 1;
	    break;
	default:
	    inflateEnd(&zst);
	    zlib_error(zst, err, "while decompressing data");
	    return_error = 1;
	}
    } while (err != Z_STREAM_END);

    if (!return_error) {
	err = inflateEnd(&zst);
	if (err != Z_OK) {
	    zlib_error(zst, err, "while finishing data decompression");
	    return NULL;
	}
    }

    if (!return_error)
	_PyString_Resize(&result_str, zst.total_out);
    else
	Py_XDECREF(result_str);	/* sets result_str == NULL, if not already */
    Py_DECREF(inputString);

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
	zlib_error(self->zst, err, "while creating compression object");
        Py_DECREF(self);
	return NULL;
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
	zlib_error(self->zst, err, "while creating decompression object");
        Py_DECREF(self);
	return NULL;
  }
}

static void
Comp_dealloc(compobject *self)
{
    ENTER_ZLIB

    if (self->is_initialised)
      deflateEnd(&self->zst);
    Py_XDECREF(self->unused_data);
    Py_XDECREF(self->unconsumed_tail);
    PyObject_Del(self);

    LEAVE_ZLIB
}

static void
Decomp_dealloc(compobject *self)
{
    ENTER_ZLIB

    if (self->is_initialised)
      inflateEnd(&self->zst);
    Py_XDECREF(self->unused_data);
    Py_XDECREF(self->unconsumed_tail);
    PyObject_Del(self);

    LEAVE_ZLIB
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
    int err, inplen, length = DEFAULTALLOC;
    PyObject *RetVal;
    Byte *input;
    unsigned long start_total_out;
    int return_error;
    PyObject * inputString;
  
    if (!PyArg_ParseTuple(args, "S:compress", &inputString))
	return NULL;
    if (PyString_AsStringAndSize(inputString, (char**)&input, &inplen) == -1)
	return NULL;

    if (!(RetVal = PyString_FromStringAndSize(NULL, length)))
	return NULL;

    ENTER_ZLIB

    Py_INCREF(inputString);
  
    start_total_out = self->zst.total_out;
    self->zst.avail_in = inplen;
    self->zst.next_in = input;
    self->zst.avail_out = length;
    self->zst.next_out = (unsigned char *)PyString_AsString(RetVal);

    Py_BEGIN_ALLOW_THREADS
    err = deflate(&(self->zst), Z_NO_FLUSH);
    Py_END_ALLOW_THREADS

    return_error = 0;

    /* while Z_OK and the output buffer is full, there might be more output,
       so extend the output buffer and try again */
    while (err == Z_OK && self->zst.avail_out == 0) {
	if (_PyString_Resize(&RetVal, length << 1) == -1)  {
	    return_error = 1;
	    break;
	}
	self->zst.next_out = (unsigned char *)PyString_AsString(RetVal) \
	    + length;
	self->zst.avail_out = length;
	length = length << 1;
    
	Py_BEGIN_ALLOW_THREADS
	err = deflate(&(self->zst), Z_NO_FLUSH);
	Py_END_ALLOW_THREADS
    }
    /* We will only get Z_BUF_ERROR if the output buffer was full but 
       there wasn't more output when we tried again, so it is not an error
       condition. 
    */  

    if (!return_error) {
	if (err != Z_OK && err != Z_BUF_ERROR) {
	    if (self->zst.msg == Z_NULL)
		PyErr_Format(ZlibError, "Error %i while compressing",
			     err); 
	    else
		PyErr_Format(ZlibError, "Error %i while compressing: %.200s",
			     err, self->zst.msg);  

	    return_error = 1;
	    Py_DECREF(RetVal);
	}
    }

    if (return_error)
	RetVal = NULL;		/* should have been handled by DECREF */
    else
	_PyString_Resize(&RetVal, self->zst.total_out - start_total_out);

    Py_DECREF(inputString);

    LEAVE_ZLIB

    return RetVal;
}

static char decomp_decompress__doc__[] =
"decompress(data, max_length) -- Return a string containing\n"
"the decompressed version of the data.\n\n"
"After calling this function, some of the input data may still\n"
"be stored in internal buffers for later processing.\n"
"Call the flush() method to clear these buffers.\n"
"If the max_length parameter is specified then the return value will be\n"
"no longer than max_length.  Unconsumed input data will be stored in\n"
"the unconsumed_tail attribute."
;

static PyObject *
PyZlib_objdecompress(compobject *self, PyObject *args)
{
    int err, inplen, old_length, length = DEFAULTALLOC;
    int max_length = 0;
    PyObject *RetVal;
    Byte *input;
    unsigned long start_total_out;
    int return_error;
    PyObject * inputString;

    if (!PyArg_ParseTuple(args, "S|i:decompress", &inputString, &max_length))
	return NULL;
    if (max_length < 0) {
	PyErr_SetString(PyExc_ValueError,
			"max_length must be greater than zero");
	return NULL;
    }

    if (PyString_AsStringAndSize(inputString, (char**)&input, &inplen) == -1)
	return NULL;

    /* limit amount of data allocated to max_length */
    if (max_length && length > max_length) 
	length = max_length;
    if (!(RetVal = PyString_FromStringAndSize(NULL, length)))
	return NULL;

    ENTER_ZLIB
    return_error = 0;

    Py_INCREF(inputString);

    start_total_out = self->zst.total_out;
    self->zst.avail_in = inplen;
    self->zst.next_in = input;
    self->zst.avail_out = length;
    self->zst.next_out = (unsigned char *)PyString_AsString(RetVal);

    Py_BEGIN_ALLOW_THREADS
    err = inflate(&(self->zst), Z_SYNC_FLUSH);
    Py_END_ALLOW_THREADS

    /* While Z_OK and the output buffer is full, there might be more output.
       So extend the output buffer and try again.
    */
    while (err == Z_OK && self->zst.avail_out == 0) { 
	/* If max_length set, don't continue decompressing if we've already
	   reached the limit.
	*/
	if (max_length && length >= max_length)
	    break;

	/* otherwise, ... */
	old_length = length;
	length = length << 1;
	if (max_length && length > max_length) 
	    length = max_length;

	if (_PyString_Resize(&RetVal, length) == -1) {
	    return_error = 1;
	    break;
	}
	self->zst.next_out = (unsigned char *)PyString_AsString(RetVal) \
	    + old_length;
	self->zst.avail_out = length - old_length;

	Py_BEGIN_ALLOW_THREADS
	err = inflate(&(self->zst), Z_SYNC_FLUSH);
	Py_END_ALLOW_THREADS
    }

    /* Not all of the compressed data could be accomodated in the output buffer
       of specified size. Return the unconsumed tail in an attribute.*/
    if(max_length) {
	Py_DECREF(self->unconsumed_tail);
	self->unconsumed_tail = PyString_FromStringAndSize(self->zst.next_in, 
							   self->zst.avail_in);
	if(!self->unconsumed_tail)
	    return_error = 1;
    }

    /* The end of the compressed data has been reached, so set the 
       unused_data attribute to a string containing the remainder of the 
       data in the string.  Note that this is also a logical place to call 
       inflateEnd, but the old behaviour of only calling it on flush() is
       preserved.
    */
    if (!return_error) {
	if (err == Z_STREAM_END) {
	    Py_XDECREF(self->unused_data);  /* Free original empty string */
	    self->unused_data = PyString_FromStringAndSize(
		(char *)self->zst.next_in, self->zst.avail_in);
	    if (self->unused_data == NULL) {
		Py_DECREF(RetVal);
		return_error = 1;
	    }
	    /* We will only get Z_BUF_ERROR if the output buffer was full 
	       but there wasn't more output when we tried again, so it is
	       not an error condition.
	    */
	} else if (err != Z_OK && err != Z_BUF_ERROR) {
	    if (self->zst.msg == Z_NULL)
		PyErr_Format(ZlibError, "Error %i while decompressing",
			     err); 
	    else
		PyErr_Format(ZlibError, "Error %i while decompressing: %.200s",
			     err, self->zst.msg);  
	    Py_DECREF(RetVal);
	    return_error = 1;
	}
    }

    if (!return_error) {
	_PyString_Resize(&RetVal, self->zst.total_out - start_total_out);
    }
    else
	RetVal = NULL;		/* should be handled by DECREF */

    Py_DECREF(inputString);

    LEAVE_ZLIB

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
    int err, length = DEFAULTALLOC;
    PyObject *RetVal;
    int flushmode = Z_FINISH;
    unsigned long start_total_out;
    int return_error;

    if (!PyArg_ParseTuple(args, "|i:flush", &flushmode))
	return NULL;

    /* Flushing with Z_NO_FLUSH is a no-op, so there's no point in
       doing any work at all; just return an empty string. */
    if (flushmode == Z_NO_FLUSH) {
	return PyString_FromStringAndSize(NULL, 0);
    }

    if (!(RetVal = PyString_FromStringAndSize(NULL, length)))
	return NULL;

    ENTER_ZLIB
  
    start_total_out = self->zst.total_out;
    self->zst.avail_in = 0;
    self->zst.avail_out = length;
    self->zst.next_out = (unsigned char *)PyString_AsString(RetVal);

    Py_BEGIN_ALLOW_THREADS
    err = deflate(&(self->zst), flushmode);
    Py_END_ALLOW_THREADS

    return_error = 0;

    /* while Z_OK and the output buffer is full, there might be more output,
       so extend the output buffer and try again */
    while (err == Z_OK && self->zst.avail_out == 0) {
	if (_PyString_Resize(&RetVal, length << 1) == -1)  {
	    return_error = 1;
	    break;
	}
	self->zst.next_out = (unsigned char *)PyString_AsString(RetVal) \
	    + length;
	self->zst.avail_out = length;
	length = length << 1;

	Py_BEGIN_ALLOW_THREADS
	err = deflate(&(self->zst), flushmode);
	Py_END_ALLOW_THREADS
    }

    /* If flushmode is Z_FINISH, we also have to call deflateEnd() to free
       various data structures. Note we should only get Z_STREAM_END when 
       flushmode is Z_FINISH, but checking both for safety*/
    if (!return_error) {
	if (err == Z_STREAM_END && flushmode == Z_FINISH) {
	    err = deflateEnd(&(self->zst));
	    if (err != Z_OK) {
		if (self->zst.msg == Z_NULL)
		    PyErr_Format(ZlibError, "Error %i from deflateEnd()",
				 err); 
		else
		    PyErr_Format(ZlibError,
				 "Error %i from deflateEnd(): %.200s",
				 err, self->zst.msg);  

		Py_DECREF(RetVal);
		return_error = 1;
	    } else
		self->is_initialised = 0;

	    /* We will only get Z_BUF_ERROR if the output buffer was full 
	       but there wasn't more output when we tried again, so it is 
	       not an error condition.
	    */
	} else if (err!=Z_OK && err!=Z_BUF_ERROR) {
	    if (self->zst.msg == Z_NULL)
		PyErr_Format(ZlibError, "Error %i while flushing",
			     err); 
	    else
		PyErr_Format(ZlibError, "Error %i while flushing: %.200s",
			     err, self->zst.msg);  
	    Py_DECREF(RetVal);
	    return_error = 1;
	}
    }

    if (!return_error)
	_PyString_Resize(&RetVal, self->zst.total_out - start_total_out);
    else
	RetVal = NULL;		/* should have been handled by DECREF */
    
    LEAVE_ZLIB

	return RetVal;
}

static char decomp_flush__doc__[] =
"flush() -- Return a string containing any remaining decompressed data.  "
"The decompressor object can no longer be used after this call."
;

static PyObject *
PyZlib_unflush(compobject *self, PyObject *args)
/*decompressor flush is a no-op because all pending data would have been 
  flushed by the decompress method. However, this routine previously called
  inflateEnd, causing any further decompress or flush calls to raise 
  exceptions. This behaviour has been preserved.*/
{
    int err;
    PyObject * retval;
  
    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    ENTER_ZLIB

    err = inflateEnd(&(self->zst));
    if (err != Z_OK) {
	zlib_error(self->zst, err, "from inflateEnd()");
	retval = NULL;
    } else {
	self->is_initialised = 0;
	retval = PyString_FromStringAndSize(NULL, 0);
    }

    LEAVE_ZLIB

    return retval;
}

static PyMethodDef comp_methods[] =
{
    {"compress", (binaryfunc)PyZlib_objcompress, METH_VARARGS, 
                 comp_compress__doc__},
    {"flush", (binaryfunc)PyZlib_flush, METH_VARARGS, 
              comp_flush__doc__},
    {NULL, NULL}
};

static PyMethodDef Decomp_methods[] =
{
    {"decompress", (binaryfunc)PyZlib_objdecompress, METH_VARARGS, 
                   decomp_decompress__doc__},
    {"flush", (binaryfunc)PyZlib_unflush, METH_VARARGS, 
              decomp_flush__doc__},
    {NULL, NULL}
};

static PyObject *
Comp_getattr(compobject *self, char *name)
{
  /* No ENTER/LEAVE_ZLIB is necessary because this fn doesn't touch
     internal data. */

  return Py_FindMethod(comp_methods, (PyObject *)self, name);
}

static PyObject *
Decomp_getattr(compobject *self, char *name)
{
    PyObject * retval;

    ENTER_ZLIB

    if (strcmp(name, "unused_data") == 0) {  
	Py_INCREF(self->unused_data);
	retval = self->unused_data;
    } else if (strcmp(name, "unconsumed_tail") == 0) {  
	Py_INCREF(self->unconsumed_tail);
	retval = self->unconsumed_tail;
    } else 
	retval = Py_FindMethod(Decomp_methods, (PyObject *)self, name);

    LEAVE_ZLIB

    return retval;
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
    uLong adler32val = adler32(0L, Z_NULL, 0);
    Byte *buf;
    int len;
 
    if (!PyArg_ParseTuple(args, "s#|l:adler32", &buf, &len, &adler32val))
	return NULL;
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
    uLong crc32val = crc32(0L, Z_NULL, 0);
    Byte *buf;
    int len;
    if (!PyArg_ParseTuple(args, "s#|l:crc32", &buf, &len, &crc32val))
	return NULL;
    crc32val = crc32(crc32val, buf, len);
    return PyInt_FromLong(crc32val);
}
     

static PyMethodDef zlib_methods[] =
{
    {"adler32", (PyCFunction)PyZlib_adler32, METH_VARARGS, 
                adler32__doc__},
    {"compress", (PyCFunction)PyZlib_compress,  METH_VARARGS, 
                 compress__doc__},
    {"compressobj", (PyCFunction)PyZlib_compressobj, METH_VARARGS, 
                    compressobj__doc__},
    {"crc32", (PyCFunction)PyZlib_crc32, METH_VARARGS, 
              crc32__doc__},	 
    {"decompress", (PyCFunction)PyZlib_decompress, METH_VARARGS, 
                   decompress__doc__},
    {"decompressobj", (PyCFunction)PyZlib_decompressobj, METH_VARARGS, 
                   decompressobj__doc__},
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

    PyModule_AddIntConstant(m, "MAX_WBITS", MAX_WBITS);
    PyModule_AddIntConstant(m, "DEFLATED", DEFLATED);
    PyModule_AddIntConstant(m, "DEF_MEM_LEVEL", DEF_MEM_LEVEL);
    PyModule_AddIntConstant(m, "Z_BEST_SPEED", Z_BEST_SPEED);
    PyModule_AddIntConstant(m, "Z_BEST_COMPRESSION", Z_BEST_COMPRESSION);
    PyModule_AddIntConstant(m, "Z_DEFAULT_COMPRESSION", Z_DEFAULT_COMPRESSION);
    PyModule_AddIntConstant(m, "Z_FILTERED", Z_FILTERED);
    PyModule_AddIntConstant(m, "Z_HUFFMAN_ONLY", Z_HUFFMAN_ONLY);
    PyModule_AddIntConstant(m, "Z_DEFAULT_STRATEGY", Z_DEFAULT_STRATEGY);
    
    PyModule_AddIntConstant(m, "Z_FINISH", Z_FINISH);
    PyModule_AddIntConstant(m, "Z_NO_FLUSH", Z_NO_FLUSH);
    PyModule_AddIntConstant(m, "Z_SYNC_FLUSH", Z_SYNC_FLUSH);
    PyModule_AddIntConstant(m, "Z_FULL_FLUSH", Z_FULL_FLUSH);
    
    ver = PyString_FromString(ZLIB_VERSION);
    if (ver != NULL) {
	PyDict_SetItemString(d, "ZLIB_VERSION", ver);
	Py_DECREF(ver);
    }

#ifdef WITH_THREAD
    zlib_lock = PyThread_allocate_lock();
#endif // WITH_THREAD
}
