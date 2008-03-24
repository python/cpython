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
	Py_BEGIN_ALLOW_THREADS \
	PyThread_acquire_lock(zlib_lock, 1); \
	Py_END_ALLOW_THREADS

#define LEAVE_ZLIB \
	PyThread_release_lock(zlib_lock);

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

static PyTypeObject Comptype;
static PyTypeObject Decomptype;

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

PyDoc_STRVAR(compressobj__doc__,
"compressobj([level]) -- Return a compressor object.\n"
"\n"
"Optional arg level is the compression level, in 1-9.");

PyDoc_STRVAR(decompressobj__doc__,
"decompressobj([wbits]) -- Return a decompressor object.\n"
"\n"
"Optional arg wbits is the window buffer size.");

static compobject *
newcompobject(PyTypeObject *type)
{
    compobject *self;
    self = PyObject_New(compobject, type);
    if (self == NULL)
	return NULL;
    self->is_initialised = 0;
    self->unused_data = PyBytes_FromStringAndSize("", 0);
    if (self->unused_data == NULL) {
	Py_DECREF(self);
	return NULL;
    }
    self->unconsumed_tail = PyBytes_FromStringAndSize("", 0);
    if (self->unconsumed_tail == NULL) {
	Py_DECREF(self);
	return NULL;
    }
    return self;
}

PyDoc_STRVAR(compress__doc__,
"compress(string[, level]) -- Returned compressed string.\n"
"\n"
"Optional arg level is the compression level, in 1-9.");

static PyObject *
PyZlib_compress(PyObject *self, PyObject *args)
{
    PyObject *ReturnVal = NULL;
    Byte *input, *output;
    int length, level=Z_DEFAULT_COMPRESSION, err;
    z_stream zst;

    /* require Python string object, optional 'level' arg */
    if (!PyArg_ParseTuple(args, "s#|i:compress", &input, &length, &level))
	return NULL;

    zst.avail_out = length + length/1000 + 12 + 1;

    output = (Byte*)malloc(zst.avail_out);
    if (output == NULL) {
	PyErr_SetString(PyExc_MemoryError,
			"Can't allocate memory to compress data");
	return NULL;
    }

    /* Past the point of no return.  From here on out, we need to make sure
       we clean up mallocs & INCREFs. */

    zst.zalloc = (alloc_func)NULL;
    zst.zfree = (free_func)Z_NULL;
    zst.next_out = (Byte *)output;
    zst.next_in = (Byte *)input;
    zst.avail_in = length;
    err = deflateInit(&zst, level);

    switch(err) {
    case(Z_OK):
	break;
    case(Z_MEM_ERROR):
	PyErr_SetString(PyExc_MemoryError,
			"Out of memory while compressing data");
	goto error;
    case(Z_STREAM_ERROR):
	PyErr_SetString(ZlibError,
			"Bad compression level");
	goto error;
    default:
        deflateEnd(&zst);
	zlib_error(zst, err, "while compressing data");
	goto error;
    }

    Py_BEGIN_ALLOW_THREADS;
    err = deflate(&zst, Z_FINISH);
    Py_END_ALLOW_THREADS;

    if (err != Z_STREAM_END) {
	zlib_error(zst, err, "while compressing data");
	deflateEnd(&zst);
	goto error;
    }

    err=deflateEnd(&zst);
    if (err == Z_OK)
	ReturnVal = PyBytes_FromStringAndSize((char *)output,
                                              zst.total_out);
    else
	zlib_error(zst, err, "while finishing compression");

 error:
    free(output);

    return ReturnVal;
}

PyDoc_STRVAR(decompress__doc__,
"decompress(string[, wbits[, bufsize]]) -- Return decompressed string.\n"
"\n"
"Optional arg wbits is the window buffer size.  Optional arg bufsize is\n"
"the initial output buffer size.");

static PyObject *
PyZlib_decompress(PyObject *self, PyObject *args)
{
    PyObject *result_str;
    Byte *input;
    int length, err;
    int wsize=DEF_WBITS;
    Py_ssize_t r_strlen=DEFAULTALLOC;
    z_stream zst;

    if (!PyArg_ParseTuple(args, "s#|in:decompress",
			  &input, &length, &wsize, &r_strlen))
	return NULL;

    if (r_strlen <= 0)
	r_strlen = 1;

    zst.avail_in = length;
    zst.avail_out = r_strlen;

    if (!(result_str = PyBytes_FromStringAndSize(NULL, r_strlen)))
	return NULL;

    zst.zalloc = (alloc_func)NULL;
    zst.zfree = (free_func)Z_NULL;
    zst.next_out = (Byte *)PyBytes_AS_STRING(result_str);
    zst.next_in = (Byte *)input;
    err = inflateInit2(&zst, wsize);

    switch(err) {
    case(Z_OK):
	break;
    case(Z_MEM_ERROR):
	PyErr_SetString(PyExc_MemoryError,
			"Out of memory while decompressing data");
	goto error;
    default:
        inflateEnd(&zst);
	zlib_error(zst, err, "while preparing to decompress data");
	goto error;
    }

    do {
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
		goto error;
	    }
	    /* fall through */
	case(Z_OK):
	    /* need more memory */
	    if (PyBytes_Resize(result_str, r_strlen << 1) < 0) {
		inflateEnd(&zst);
		goto error;
	    }
	    zst.next_out =
                (unsigned char *)PyBytes_AS_STRING(result_str) + r_strlen;
	    zst.avail_out = r_strlen;
	    r_strlen = r_strlen << 1;
	    break;
	default:
	    inflateEnd(&zst);
	    zlib_error(zst, err, "while decompressing data");
	    goto error;
	}
    } while (err != Z_STREAM_END);

    err = inflateEnd(&zst);
    if (err != Z_OK) {
	zlib_error(zst, err, "while finishing data decompression");
	goto error;
    }

    if (PyBytes_Resize(result_str, zst.total_out) < 0)
        goto error;

    return result_str;

 error:
    Py_XDECREF(result_str);
    return NULL;
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
    if (self==NULL)
	return(NULL);
    self->zst.zalloc = (alloc_func)NULL;
    self->zst.zfree = (free_func)Z_NULL;
    self->zst.next_in = NULL;
    self->zst.avail_in = 0;
    err = deflateInit2(&self->zst, level, method, wbits, memLevel, strategy);
    switch(err) {
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
	PyErr_SetString(PyExc_ValueError, "Invalid initialization option");
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
	return NULL;

    self = newcompobject(&Decomptype);
    if (self == NULL)
	return(NULL);
    self->zst.zalloc = (alloc_func)NULL;
    self->zst.zfree = (free_func)Z_NULL;
    self->zst.next_in = NULL;
    self->zst.avail_in = 0;
    err = inflateInit2(&self->zst, wbits);
    switch(err) {
    case (Z_OK):
	self->is_initialised = 1;
	return (PyObject*)self;
    case(Z_STREAM_ERROR):
	Py_DECREF(self);
	PyErr_SetString(PyExc_ValueError, "Invalid initialization option");
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
    if (self->is_initialised)
	deflateEnd(&self->zst);
    Py_XDECREF(self->unused_data);
    Py_XDECREF(self->unconsumed_tail);
    PyObject_Del(self);
}

static void
Decomp_dealloc(compobject *self)
{
    if (self->is_initialised)
	inflateEnd(&self->zst);
    Py_XDECREF(self->unused_data);
    Py_XDECREF(self->unconsumed_tail);
    PyObject_Del(self);
}

PyDoc_STRVAR(comp_compress__doc__,
"compress(data) -- Return a string containing data compressed.\n"
"\n"
"After calling this function, some of the input data may still\n"
"be stored in internal buffers for later processing.\n"
"Call the flush() method to clear these buffers.");


static PyObject *
PyZlib_objcompress(compobject *self, PyObject *args)
{
    int err, inplen, length = DEFAULTALLOC;
    PyObject *RetVal;
    Byte *input;
    unsigned long start_total_out;

    if (!PyArg_ParseTuple(args, "s#:compress", &input, &inplen))
	return NULL;

    if (!(RetVal = PyBytes_FromStringAndSize(NULL, length)))
	return NULL;

    ENTER_ZLIB

    start_total_out = self->zst.total_out;
    self->zst.avail_in = inplen;
    self->zst.next_in = input;
    self->zst.avail_out = length;
    self->zst.next_out = (unsigned char *)PyBytes_AS_STRING(RetVal);

    Py_BEGIN_ALLOW_THREADS
    err = deflate(&(self->zst), Z_NO_FLUSH);
    Py_END_ALLOW_THREADS

    /* while Z_OK and the output buffer is full, there might be more output,
       so extend the output buffer and try again */
    while (err == Z_OK && self->zst.avail_out == 0) {
	if (PyBytes_Resize(RetVal, length << 1) < 0) {
            Py_DECREF(RetVal);
            RetVal = NULL;
	    goto error;
        }
	self->zst.next_out =
            (unsigned char *)PyBytes_AS_STRING(RetVal) + length;
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

    if (err != Z_OK && err != Z_BUF_ERROR) {
	zlib_error(self->zst, err, "while compressing");
	Py_DECREF(RetVal);
	RetVal = NULL;
	goto error;
    }
    if (PyBytes_Resize(RetVal, self->zst.total_out - start_total_out) < 0) {
        Py_DECREF(RetVal);
        RetVal = NULL;
    }

 error:
    LEAVE_ZLIB
    return RetVal;
}

PyDoc_STRVAR(decomp_decompress__doc__,
"decompress(data, max_length) -- Return a string containing the decompressed\n"
"version of the data.\n"
"\n"
"After calling this function, some of the input data may still be stored in\n"
"internal buffers for later processing.\n"
"Call the flush() method to clear these buffers.\n"
"If the max_length parameter is specified then the return value will be\n"
"no longer than max_length.  Unconsumed input data will be stored in\n"
"the unconsumed_tail attribute.");

static PyObject *
PyZlib_objdecompress(compobject *self, PyObject *args)
{
    int err, inplen, old_length, length = DEFAULTALLOC;
    int max_length = 0;
    PyObject *RetVal;
    Byte *input;
    unsigned long start_total_out;

    if (!PyArg_ParseTuple(args, "s#|i:decompress", &input,
			  &inplen, &max_length))
	return NULL;
    if (max_length < 0) {
	PyErr_SetString(PyExc_ValueError,
			"max_length must be greater than zero");
	return NULL;
    }

    /* limit amount of data allocated to max_length */
    if (max_length && length > max_length)
	length = max_length;
    if (!(RetVal = PyBytes_FromStringAndSize(NULL, length)))
	return NULL;

    ENTER_ZLIB

    start_total_out = self->zst.total_out;
    self->zst.avail_in = inplen;
    self->zst.next_in = input;
    self->zst.avail_out = length;
    self->zst.next_out = (unsigned char *)PyBytes_AS_STRING(RetVal);

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

	if (PyBytes_Resize(RetVal, length) < 0) {
            Py_DECREF(RetVal);
            RetVal = NULL;
	    goto error;
        }
	self->zst.next_out =
            (unsigned char *)PyBytes_AS_STRING(RetVal) + old_length;
	self->zst.avail_out = length - old_length;

	Py_BEGIN_ALLOW_THREADS
	err = inflate(&(self->zst), Z_SYNC_FLUSH);
	Py_END_ALLOW_THREADS
    }

    /* Not all of the compressed data could be accommodated in the output buffer
       of specified size. Return the unconsumed tail in an attribute.*/
    if(max_length) {
	Py_DECREF(self->unconsumed_tail);
	self->unconsumed_tail = PyBytes_FromStringAndSize((char *)self->zst.next_in,
							   self->zst.avail_in);
	if(!self->unconsumed_tail) {
	    Py_DECREF(RetVal);
	    RetVal = NULL;
	    goto error;
	}
    }

    /* The end of the compressed data has been reached, so set the
       unused_data attribute to a string containing the remainder of the
       data in the string.  Note that this is also a logical place to call
       inflateEnd, but the old behaviour of only calling it on flush() is
       preserved.
    */
    if (err == Z_STREAM_END) {
	Py_XDECREF(self->unused_data);  /* Free original empty string */
	self->unused_data = PyBytes_FromStringAndSize(
	    (char *)self->zst.next_in, self->zst.avail_in);
	if (self->unused_data == NULL) {
	    Py_DECREF(RetVal);
	    goto error;
	}
	/* We will only get Z_BUF_ERROR if the output buffer was full
	   but there wasn't more output when we tried again, so it is
	   not an error condition.
	*/
    } else if (err != Z_OK && err != Z_BUF_ERROR) {
	zlib_error(self->zst, err, "while decompressing");
	Py_DECREF(RetVal);
	RetVal = NULL;
	goto error;
    }

    if (PyBytes_Resize(RetVal, self->zst.total_out - start_total_out) < 0) {
        Py_DECREF(RetVal);
        RetVal = NULL;
    }

 error:
    LEAVE_ZLIB

    return RetVal;
}

PyDoc_STRVAR(comp_flush__doc__,
"flush( [mode] ) -- Return a string containing any remaining compressed data.\n"
"\n"
"mode can be one of the constants Z_SYNC_FLUSH, Z_FULL_FLUSH, Z_FINISH; the\n"
"default value used when mode is not specified is Z_FINISH.\n"
"If mode == Z_FINISH, the compressor object can no longer be used after\n"
"calling the flush() method.  Otherwise, more data can still be compressed.");

static PyObject *
PyZlib_flush(compobject *self, PyObject *args)
{
    int err, length = DEFAULTALLOC;
    PyObject *RetVal;
    int flushmode = Z_FINISH;
    unsigned long start_total_out;

    if (!PyArg_ParseTuple(args, "|i:flush", &flushmode))
	return NULL;

    /* Flushing with Z_NO_FLUSH is a no-op, so there's no point in
       doing any work at all; just return an empty string. */
    if (flushmode == Z_NO_FLUSH) {
	return PyBytes_FromStringAndSize(NULL, 0);
    }

    if (!(RetVal = PyBytes_FromStringAndSize(NULL, length)))
	return NULL;

    ENTER_ZLIB

    start_total_out = self->zst.total_out;
    self->zst.avail_in = 0;
    self->zst.avail_out = length;
    self->zst.next_out = (unsigned char *)PyBytes_AS_STRING(RetVal);

    Py_BEGIN_ALLOW_THREADS
    err = deflate(&(self->zst), flushmode);
    Py_END_ALLOW_THREADS

    /* while Z_OK and the output buffer is full, there might be more output,
       so extend the output buffer and try again */
    while (err == Z_OK && self->zst.avail_out == 0) {
	if (PyBytes_Resize(RetVal, length << 1) < 0) {
            Py_DECREF(RetVal);
            RetVal = NULL;
	    goto error;
        }
	self->zst.next_out =
            (unsigned char *)PyBytes_AS_STRING(RetVal) + length;
	self->zst.avail_out = length;
	length = length << 1;

	Py_BEGIN_ALLOW_THREADS
	err = deflate(&(self->zst), flushmode);
	Py_END_ALLOW_THREADS
    }

    /* If flushmode is Z_FINISH, we also have to call deflateEnd() to free
       various data structures. Note we should only get Z_STREAM_END when
       flushmode is Z_FINISH, but checking both for safety*/
    if (err == Z_STREAM_END && flushmode == Z_FINISH) {
	err = deflateEnd(&(self->zst));
	if (err != Z_OK) {
	    zlib_error(self->zst, err, "from deflateEnd()");
	    Py_DECREF(RetVal);
	    RetVal = NULL;
	    goto error;
	}
	else
	    self->is_initialised = 0;

	/* We will only get Z_BUF_ERROR if the output buffer was full
	   but there wasn't more output when we tried again, so it is
	   not an error condition.
	*/
    } else if (err!=Z_OK && err!=Z_BUF_ERROR) {
	zlib_error(self->zst, err, "while flushing");
	Py_DECREF(RetVal);
	RetVal = NULL;
	goto error;
    }

    if (PyBytes_Resize(RetVal, self->zst.total_out - start_total_out) < 0) {
        Py_DECREF(RetVal);
        RetVal = NULL;
    }

 error:
    LEAVE_ZLIB

    return RetVal;
}

#ifdef HAVE_ZLIB_COPY
PyDoc_STRVAR(comp_copy__doc__,
"copy() -- Return a copy of the compression object.");

static PyObject *
PyZlib_copy(compobject *self)
{
    compobject *retval = NULL;
    int err;

    retval = newcompobject(&Comptype);
    if (!retval) return NULL;

    /* Copy the zstream state
     * We use ENTER_ZLIB / LEAVE_ZLIB to make this thread-safe
     */
    ENTER_ZLIB
    err = deflateCopy(&retval->zst, &self->zst);
    switch(err) {
    case(Z_OK):
        break;
    case(Z_STREAM_ERROR):
        PyErr_SetString(PyExc_ValueError, "Inconsistent stream state");
        goto error;
    case(Z_MEM_ERROR):
        PyErr_SetString(PyExc_MemoryError,
                        "Can't allocate memory for compression object");
        goto error;
    default:
        zlib_error(self->zst, err, "while copying compression object");
        goto error;
    }

    Py_INCREF(self->unused_data);
    Py_INCREF(self->unconsumed_tail);
    Py_XDECREF(retval->unused_data);
    Py_XDECREF(retval->unconsumed_tail);
    retval->unused_data = self->unused_data;
    retval->unconsumed_tail = self->unconsumed_tail;

    /* Mark it as being initialized */
    retval->is_initialised = 1;

    LEAVE_ZLIB
    return (PyObject *)retval;

error:
    LEAVE_ZLIB
    Py_XDECREF(retval);
    return NULL;
}

PyDoc_STRVAR(decomp_copy__doc__,
"copy() -- Return a copy of the decompression object.");

static PyObject *
PyZlib_uncopy(compobject *self)
{
    compobject *retval = NULL;
    int err;

    retval = newcompobject(&Decomptype);
    if (!retval) return NULL;

    /* Copy the zstream state
     * We use ENTER_ZLIB / LEAVE_ZLIB to make this thread-safe
     */
    ENTER_ZLIB
    err = inflateCopy(&retval->zst, &self->zst);
    switch(err) {
    case(Z_OK):
        break;
    case(Z_STREAM_ERROR):
        PyErr_SetString(PyExc_ValueError, "Inconsistent stream state");
        goto error;
    case(Z_MEM_ERROR):
        PyErr_SetString(PyExc_MemoryError,
                        "Can't allocate memory for decompression object");
        goto error;
    default:
        zlib_error(self->zst, err, "while copying decompression object");
        goto error;
    }

    Py_INCREF(self->unused_data);
    Py_INCREF(self->unconsumed_tail);
    Py_XDECREF(retval->unused_data);
    Py_XDECREF(retval->unconsumed_tail);
    retval->unused_data = self->unused_data;
    retval->unconsumed_tail = self->unconsumed_tail;

    /* Mark it as being initialized */
    retval->is_initialised = 1;

    LEAVE_ZLIB
    return (PyObject *)retval;

error:
    LEAVE_ZLIB
    Py_XDECREF(retval);
    return NULL;
}
#endif

PyDoc_STRVAR(decomp_flush__doc__,
"flush( [length] ) -- Return a string containing any remaining\n"
"decompressed data. length, if given, is the initial size of the\n"
"output buffer.\n"
"\n"
"The decompressor object can no longer be used after this call.");

static PyObject *
PyZlib_unflush(compobject *self, PyObject *args)
{
    int err, length = DEFAULTALLOC;
    PyObject * retval = NULL;
    unsigned long start_total_out;

    if (!PyArg_ParseTuple(args, "|i:flush", &length))
	return NULL;
    if (!(retval = PyBytes_FromStringAndSize(NULL, length)))
	return NULL;


    ENTER_ZLIB

    start_total_out = self->zst.total_out;
    self->zst.avail_out = length;
    self->zst.next_out = (Byte *)PyBytes_AS_STRING(retval);

    Py_BEGIN_ALLOW_THREADS
    err = inflate(&(self->zst), Z_FINISH);
    Py_END_ALLOW_THREADS

    /* while Z_OK and the output buffer is full, there might be more output,
       so extend the output buffer and try again */
    while ((err == Z_OK || err == Z_BUF_ERROR) && self->zst.avail_out == 0) {
	if (PyBytes_Resize(retval, length << 1) < 0) {
            Py_DECREF(retval);
            retval = NULL;
	    goto error;
        }
	self->zst.next_out = (Byte *)PyBytes_AS_STRING(retval) + length;
	self->zst.avail_out = length;
	length = length << 1;

	Py_BEGIN_ALLOW_THREADS
	err = inflate(&(self->zst), Z_FINISH);
	Py_END_ALLOW_THREADS
    }

    /* If flushmode is Z_FINISH, we also have to call deflateEnd() to free
       various data structures. Note we should only get Z_STREAM_END when
       flushmode is Z_FINISH */
    if (err == Z_STREAM_END) {
	err = inflateEnd(&(self->zst));
        self->is_initialised = 0;
	if (err != Z_OK) {
	    zlib_error(self->zst, err, "from inflateEnd()");
	    Py_DECREF(retval);
	    retval = NULL;
	    goto error;
	}
    }
    if (PyBytes_Resize(retval, self->zst.total_out - start_total_out) < 0) {
        Py_DECREF(retval);
        retval = NULL;
    }

error:

    LEAVE_ZLIB

    return retval;
}

static PyMethodDef comp_methods[] =
{
    {"compress", (binaryfunc)PyZlib_objcompress, METH_VARARGS,
                 comp_compress__doc__},
    {"flush", (binaryfunc)PyZlib_flush, METH_VARARGS,
              comp_flush__doc__},
#ifdef HAVE_ZLIB_COPY
    {"copy",  (PyCFunction)PyZlib_copy, METH_NOARGS,
              comp_copy__doc__},
#endif
    {NULL, NULL}
};

static PyMethodDef Decomp_methods[] =
{
    {"decompress", (binaryfunc)PyZlib_objdecompress, METH_VARARGS,
                   decomp_decompress__doc__},
    {"flush", (binaryfunc)PyZlib_unflush, METH_VARARGS,
              decomp_flush__doc__},
#ifdef HAVE_ZLIB_COPY
    {"copy",  (PyCFunction)PyZlib_uncopy, METH_NOARGS,
              decomp_copy__doc__},
#endif
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

PyDoc_STRVAR(adler32__doc__,
"adler32(string[, start]) -- Compute an Adler-32 checksum of string.\n"
"\n"
"An optional starting value can be specified.  The returned checksum is\n"
"an integer.");

static PyObject *
PyZlib_adler32(PyObject *self, PyObject *args)
{
    uLong adler32val = 1;  /* adler32(0L, Z_NULL, 0) */
    Byte *buf;
    int len;

    if (!PyArg_ParseTuple(args, "s#|I:adler32", &buf, &len, &adler32val))
	return NULL;
    adler32val = adler32(adler32val, buf, len);
    return PyLong_FromUnsignedLong(adler32val & 0xffffffffU);
}

PyDoc_STRVAR(crc32__doc__,
"crc32(string[, start]) -- Compute a CRC-32 checksum of string.\n"
"\n"
"An optional starting value can be specified.  The returned checksum is\n"
"an integer.");

static PyObject *
PyZlib_crc32(PyObject *self, PyObject *args)
{
    uLong crc32val = 0;  /* crc32(0L, Z_NULL, 0) */
    Byte *buf;
    int len;
    if (!PyArg_ParseTuple(args, "s#|I:crc32", &buf, &len, &crc32val))
	return NULL;
    crc32val = crc32(crc32val, buf, len);
    return PyLong_FromUnsignedLong(crc32val & 0xffffffffU);
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

static PyTypeObject Comptype = {
    PyVarObject_HEAD_INIT(0, 0)
    "zlib.Compress",
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

static PyTypeObject Decomptype = {
    PyVarObject_HEAD_INIT(0, 0)
    "zlib.Decompress",
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

PyDoc_STRVAR(zlib_module_documentation,
"The functions in this module allow compression and decompression using the\n"
"zlib library, which is based on GNU zip.\n"
"\n"
"adler32(string[, start]) -- Compute an Adler-32 checksum.\n"
"compress(string[, level]) -- Compress string, with compression level in 1-9.\n"
"compressobj([level]) -- Return a compressor object.\n"
"crc32(string[, start]) -- Compute a CRC-32 checksum.\n"
"decompress(string,[wbits],[bufsize]) -- Decompresses a compressed string.\n"
"decompressobj([wbits]) -- Return a decompressor object.\n"
"\n"
"'wbits' is window buffer size.\n"
"Compressor objects support compress() and flush() methods; decompressor\n"
"objects support decompress() and flush().");

PyMODINIT_FUNC
PyInit_zlib(void)
{
    PyObject *m, *ver;
    Py_TYPE(&Comptype) = &PyType_Type;
    Py_TYPE(&Decomptype) = &PyType_Type;
    m = Py_InitModule4("zlib", zlib_methods,
		       zlib_module_documentation,
		       (PyObject*)NULL,PYTHON_API_VERSION);
    if (m == NULL)
	return;

    ZlibError = PyErr_NewException("zlib.error", NULL, NULL);
    if (ZlibError != NULL) {
        Py_INCREF(ZlibError);
	PyModule_AddObject(m, "error", ZlibError);
    }
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

    ver = PyUnicode_FromString(ZLIB_VERSION);
    if (ver != NULL)
	PyModule_AddObject(m, "ZLIB_VERSION", ver);

    PyModule_AddStringConstant(m, "__version__", "1.0");

#ifdef WITH_THREAD
    zlib_lock = PyThread_allocate_lock();
#endif /* WITH_THREAD */
}
