/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/


/* Cl objects */

#define CLDEBUG

#include <stdarg.h>
#include <cl.h>
#include "Python.h"

typedef struct {
	PyObject_HEAD
	int ob_isCompressor;	/* Compressor or Decompressor */
	CL_Handle ob_compressorHdl;
	int *ob_paramtypes;
	int ob_nparams;
} clobject;

static PyObject *ClError;		/* exception cl.error */

static int error_handler_called = 0;

/*
 * We want to use the function prototypes that are available in the C
 * compiler on the SGI.  Because of that, we need to declare the first
 * argument of the compressor and decompressor methods as "object *",
 * even though they are really "clobject *".  Therefore we cast the
 * argument to the proper type using this macro.
 */
#define SELF	((clobject *) self)

/********************************************************************
			  Utility routines.
********************************************************************/
static void
cl_ErrorHandler(CL_Handle handle, int code, const char *fmt, ...)
{
	va_list ap;
	char errbuf[BUFSIZ];	/* hopefully big enough */
	char *p;

	if (PyErr_Occurred())	/* don't change existing error */
		return;
	error_handler_called = 1;
	va_start(ap, fmt);
	vsprintf(errbuf, fmt, ap);
	va_end(ap);
	p = &errbuf[strlen(errbuf) - 1]; /* swat the line feed */
	if (*p == '\n')
		*p = 0;
	PyErr_SetString(ClError, errbuf);
}

/*
 * This assumes that params are always in the range 0 to some maximum.
 */
static int
param_type_is_float(clobject *self, int param)
{
	int bufferlength;

	if (self->ob_paramtypes == NULL) {
		error_handler_called = 0;
		bufferlength = clQueryParams(self->ob_compressorHdl, 0, 0);
		if (error_handler_called)
			return -1;

		self->ob_paramtypes = PyMem_NEW(int, bufferlength);
		if (self->ob_paramtypes == NULL)
			return -1;
		self->ob_nparams = bufferlength / 2;

		(void) clQueryParams(self->ob_compressorHdl,
				     self->ob_paramtypes, bufferlength);
		if (error_handler_called) {
			PyMem_DEL(self->ob_paramtypes);
			self->ob_paramtypes = NULL;
			return -1;
		}
	}

	if (param < 0 || param >= self->ob_nparams)
		return -1;

	if (self->ob_paramtypes[param*2 + 1] == CL_FLOATING_ENUM_VALUE ||
	    self->ob_paramtypes[param*2 + 1] == CL_FLOATING_RANGE_VALUE)
		return 1;
	else
		return 0;
}

/********************************************************************
	       Single image compression/decompression.
********************************************************************/
static PyObject *
cl_CompressImage(PyObject *self, PyObject *args)
{
	int compressionScheme, width, height, originalFormat;
	float compressionRatio;
	int frameBufferSize, compressedBufferSize;
	char *frameBuffer;
	PyObject *compressedBuffer;

	if (!PyArg_Parse(args, "(iiiifs#)", &compressionScheme,
			 &width, &height,
			 &originalFormat, &compressionRatio, &frameBuffer,
			 &frameBufferSize))
		return NULL;

  retry:
	compressedBuffer = PyString_FromStringAndSize(NULL, frameBufferSize);
	if (compressedBuffer == NULL)
		return NULL;

	compressedBufferSize = frameBufferSize;
	error_handler_called = 0;
	if (clCompressImage(compressionScheme, width, height, originalFormat,
			    compressionRatio, (void *) frameBuffer,
			    &compressedBufferSize,
			    (void *) PyString_AsString(compressedBuffer))
	    == FAILURE) {
		Py_DECREF(compressedBuffer);
		if (!error_handler_called)
			PyErr_SetString(ClError, "clCompressImage failed");
		return NULL;
	}

	if (compressedBufferSize > frameBufferSize) {
		frameBufferSize = compressedBufferSize;
		Py_DECREF(compressedBuffer);
		goto retry;
	}

	if (compressedBufferSize < frameBufferSize)
		if (_PyString_Resize(&compressedBuffer, compressedBufferSize))
			return NULL;

	return compressedBuffer;
}

static PyObject *
cl_DecompressImage(PyObject *self, PyObject *args)
{
	int compressionScheme, width, height, originalFormat;
	char *compressedBuffer;
	int compressedBufferSize, frameBufferSize;
	PyObject *frameBuffer;

	if (!PyArg_Parse(args, "(iiiis#)", &compressionScheme, &width, &height,
			 &originalFormat, &compressedBuffer,
			 &compressedBufferSize))
		return NULL;

	frameBufferSize = width * height * CL_BytesPerPixel(originalFormat);

	frameBuffer = PyString_FromStringAndSize(NULL, frameBufferSize);
	if (frameBuffer == NULL)
		return NULL;

	error_handler_called = 0;
	if (clDecompressImage(compressionScheme, width, height, originalFormat,
			      compressedBufferSize, compressedBuffer,
			      (void *) PyString_AsString(frameBuffer))
	    == FAILURE) {
		Py_DECREF(frameBuffer);
		if (!error_handler_called)
			PyErr_SetString(ClError, "clDecompressImage failed");
		return NULL;
	}

	return frameBuffer;
}

/********************************************************************
		Sequential compression/decompression.
********************************************************************/
#define CheckCompressor(self)	if ((self)->ob_compressorHdl == NULL) { \
	PyErr_SetString(PyExc_RuntimeError, "(de)compressor not active"); \
	return NULL; \
}

static PyObject *
doClose(clobject *self, PyObject *args, int (*close_func)(CL_Handle))
{
	CheckCompressor(self);

	if (!PyArg_NoArgs(args))
		return NULL;

	error_handler_called = 0;
	if ((*close_func)(self->ob_compressorHdl) == FAILURE) {
		if (!error_handler_called)
			PyErr_SetString(ClError, "close failed");
		return NULL;
	}

	self->ob_compressorHdl = NULL;

	if (self->ob_paramtypes)
		PyMem_DEL(self->ob_paramtypes);
	self->ob_paramtypes = NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
clm_CloseCompressor(PyObject *self, PyObject *args)
{
	return doClose(SELF, args, clCloseCompressor);
}

static PyObject *
clm_CloseDecompressor(PyObject *self, PyObject *args)
{
	return doClose(SELF, args, clCloseDecompressor);
}

static PyObject *
clm_Compress(PyObject *self, PyObject *args)
{
	int numberOfFrames;
	int frameBufferSize, compressedBufferSize, size;
	char *frameBuffer;
	PyObject *data;

	CheckCompressor(SELF);

	if (!PyArg_Parse(args, "(is#)", &numberOfFrames,
			 &frameBuffer, &frameBufferSize))
		return NULL;

	error_handler_called = 0;
	size = clGetParam(SELF->ob_compressorHdl, CL_COMPRESSED_BUFFER_SIZE);
	compressedBufferSize = size;
	if (error_handler_called)
		return NULL;

	data = PyString_FromStringAndSize(NULL, size);
	if (data == NULL)
		return NULL;

	error_handler_called = 0;
	if (clCompress(SELF->ob_compressorHdl, numberOfFrames,
		       (void *) frameBuffer, &compressedBufferSize,
		       (void *) PyString_AsString(data)) == FAILURE) {
		Py_DECREF(data);
		if (!error_handler_called)
			PyErr_SetString(ClError, "compress failed");
		return NULL;
	}

	if (compressedBufferSize < size)
		if (_PyString_Resize(&data, compressedBufferSize))
			return NULL;

	if (compressedBufferSize > size) {
		/* we didn't get all "compressed" data */
		Py_DECREF(data);
		PyErr_SetString(ClError,
				"compressed data is more than fitted");
		return NULL;
	}

	return data;
}

static PyObject *
clm_Decompress(PyObject *self, PyObject *args)
{
	PyObject *data;
	int numberOfFrames;
	char *compressedData;
	int compressedDataSize, dataSize;

	CheckCompressor(SELF);

	if (!PyArg_Parse(args, "(is#)", &numberOfFrames, &compressedData,
			 &compressedDataSize))
		return NULL;

	error_handler_called = 0;
	dataSize = clGetParam(SELF->ob_compressorHdl, CL_FRAME_BUFFER_SIZE);
	if (error_handler_called)
		return NULL;

	data = PyString_FromStringAndSize(NULL, dataSize);
	if (data == NULL)
		return NULL;

	error_handler_called = 0;
	if (clDecompress(SELF->ob_compressorHdl, numberOfFrames,
			 compressedDataSize, (void *) compressedData,
			 (void *) PyString_AsString(data)) == FAILURE) {
		Py_DECREF(data);
		if (!error_handler_called)
			PyErr_SetString(ClError, "decompress failed");
		return NULL;
	}

	return data;
}

static PyObject *
doParams(clobject *self, PyObject *args, int (*func)(CL_Handle, int *, int),
	 int modified)
{
	PyObject *list, *v;
	int *PVbuffer;
	int length;
	int i;
	float number;
	
	CheckCompressor(self);

	if (!PyArg_Parse(args, "O", &list))
		return NULL;
	if (!PyList_Check(list)) {
		PyErr_BadArgument();
		return NULL;
	}
	length = PyList_Size(list);
	PVbuffer = PyMem_NEW(int, length);
	if (PVbuffer == NULL)
		return PyErr_NoMemory();
	for (i = 0; i < length; i++) {
		v = PyList_GetItem(list, i);
		if (PyFloat_Check(v)) {
			number = PyFloat_AsDouble(v);
			PVbuffer[i] = CL_TypeIsInt(number);
		} else if (PyInt_Check(v)) {
			PVbuffer[i] = PyInt_AsLong(v);
			if ((i & 1) &&
			    param_type_is_float(self, PVbuffer[i-1]) > 0) {
				number = PVbuffer[i];
				PVbuffer[i] = CL_TypeIsInt(number);
			}
		} else {
			PyMem_DEL(PVbuffer);
			PyErr_BadArgument();
			return NULL;
		}
	}

	error_handler_called = 0;
	(*func)(self->ob_compressorHdl, PVbuffer, length);
	if (error_handler_called) {
		PyMem_DEL(PVbuffer);
		return NULL;
	}

	if (modified) {
		for (i = 0; i < length; i++) {
			if ((i & 1) &&
			    param_type_is_float(self, PVbuffer[i-1]) > 0) {
				number = CL_TypeIsFloat(PVbuffer[i]);
				v = PyFloat_FromDouble(number);
			} else
				v = PyInt_FromLong(PVbuffer[i]);
			PyList_SetItem(list, i, v);
		}
	}

	PyMem_DEL(PVbuffer);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
clm_GetParams(PyObject *self, PyObject *args)
{
	return doParams(SELF, args, clGetParams, 1);
}

static PyObject *
clm_SetParams(PyObject *self, PyObject *args)
{
	return doParams(SELF, args, clSetParams, 0);
}

static PyObject *
do_get(clobject *self, PyObject *args, int (*func)(CL_Handle, int))
{
	int paramID, value;
	float fvalue;

	CheckCompressor(self);

	if (!PyArg_Parse(args, "i", &paramID))
		return NULL;

	error_handler_called = 0;
	value = (*func)(self->ob_compressorHdl, paramID);
	if (error_handler_called)
		return NULL;

	if (param_type_is_float(self, paramID) > 0) {
		fvalue = CL_TypeIsFloat(value);
		return PyFloat_FromDouble(fvalue);
	}

	return PyInt_FromLong(value);
}

static PyObject *
clm_GetParam(PyObject *self, PyObject *args)
{
	return do_get(SELF, args, clGetParam);
}

static PyObject *
clm_GetDefault(PyObject *self, PyObject *args)
{
	return do_get(SELF, args, clGetDefault);
}

static PyObject *
clm_SetParam(PyObject *self, PyObject *args)
{
	int paramID, value;
	float fvalue;

	CheckCompressor(SELF);

	if (!PyArg_Parse(args, "(ii)", &paramID, &value)) {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(if)", &paramID, &fvalue)) {
			PyErr_Clear();
			PyErr_SetString(PyExc_TypeError,
			       "bad argument list (format '(ii)' or '(if)')");
			return NULL;
		}
		value = CL_TypeIsInt(fvalue);
	} else {
		if (param_type_is_float(SELF, paramID) > 0) {
			fvalue = value;
			value = CL_TypeIsInt(fvalue);
		}
	}

 	error_handler_called = 0;
	value = clSetParam(SELF->ob_compressorHdl, paramID, value);
	if (error_handler_called)
		return NULL;

	if (param_type_is_float(SELF, paramID) > 0)
		return PyFloat_FromDouble(CL_TypeIsFloat(value));
	else
		return PyInt_FromLong(value);
}

static PyObject *
clm_GetParamID(PyObject *self, PyObject *args)
{
	char *name;
	int value;

	CheckCompressor(SELF);

	if (!PyArg_Parse(args, "s", &name))
		return NULL;

	error_handler_called = 0;
	value = clGetParamID(SELF->ob_compressorHdl, name);
	if (value == FAILURE) {
		if (!error_handler_called)
			PyErr_SetString(ClError, "getparamid failed");
		return NULL;
	}

	return PyInt_FromLong(value);
}

static PyObject *
clm_QueryParams(PyObject *self, PyObject *args)
{
	int bufferlength;
	int *PVbuffer;
	PyObject *list;
	int i;

	CheckCompressor(SELF);

	if (!PyArg_NoArgs(args))
		return NULL;

	error_handler_called = 0;
	bufferlength = clQueryParams(SELF->ob_compressorHdl, 0, 0);
	if (error_handler_called)
		return NULL;

	PVbuffer = PyMem_NEW(int, bufferlength);
	if (PVbuffer == NULL)
		return PyErr_NoMemory();

	bufferlength = clQueryParams(SELF->ob_compressorHdl, PVbuffer,
				     bufferlength);
	if (error_handler_called) {
		PyMem_DEL(PVbuffer);
		return NULL;
	}

	list = PyList_New(bufferlength);
	if (list == NULL) {
		PyMem_DEL(PVbuffer);
		return NULL;
	}

	for (i = 0; i < bufferlength; i++) {
		if (i & 1)
			PyList_SetItem(list, i, PyInt_FromLong(PVbuffer[i]));
		else if (PVbuffer[i] == 0) {
			Py_INCREF(Py_None);
			PyList_SetItem(list, i, Py_None);
		} else
			PyList_SetItem(list, i,
				   PyString_FromString((char *) PVbuffer[i]));
	}

	PyMem_DEL(PVbuffer);

	return list;
}

static PyObject *
clm_GetMinMax(PyObject *self, PyObject *args)
{
	int param, min, max;
	float fmin, fmax;

	CheckCompressor(SELF);

	if (!PyArg_Parse(args, "i", &param))
		return NULL;

	clGetMinMax(SELF->ob_compressorHdl, param, &min, &max);

	if (param_type_is_float(SELF, param) > 0) {
		fmin = CL_TypeIsFloat(min);
		fmax = CL_TypeIsFloat(max);
		return Py_BuildValue("(ff)", fmin, fmax);
	}

	return Py_BuildValue("(ii)", min, max);
}

static PyObject *
clm_GetName(PyObject *self, PyObject *args)
{
	int param;
	char *name;

	CheckCompressor(SELF);

	if (!PyArg_Parse(args, "i", &param))
		return NULL;

	error_handler_called = 0;
	name = clGetName(SELF->ob_compressorHdl, param);
	if (name == NULL || error_handler_called) {
		if (!error_handler_called)
			PyErr_SetString(ClError, "getname failed");
		return NULL;
	}

	return PyString_FromString(name);
}

static PyObject *
clm_QuerySchemeFromHandle(PyObject *self, PyObject *args)
{
	CheckCompressor(SELF);

	if (!PyArg_NoArgs(args))
		return NULL;

	return PyInt_FromLong(clQuerySchemeFromHandle(SELF->ob_compressorHdl));
}

static PyObject *
clm_ReadHeader(PyObject *self, PyObject *args)
{
	char *header;
	int headerSize;

	CheckCompressor(SELF);

	if (!PyArg_Parse(args, "s#", &header, &headerSize))
		return NULL;

	return PyInt_FromLong(clReadHeader(SELF->ob_compressorHdl,
					   headerSize, header));
}

static PyMethodDef compressor_methods[] = {
	{"close",		clm_CloseCompressor}, /* alias */
	{"CloseCompressor",	clm_CloseCompressor},
	{"Compress",		clm_Compress},
	{"GetDefault",		clm_GetDefault},
	{"GetMinMax",		clm_GetMinMax},
	{"GetName",		clm_GetName},
	{"GetParam",		clm_GetParam},
	{"GetParamID",		clm_GetParamID},
	{"GetParams",		clm_GetParams},
	{"QueryParams",		clm_QueryParams},
	{"QuerySchemeFromHandle",clm_QuerySchemeFromHandle},
	{"SetParam",		clm_SetParam},
	{"SetParams",		clm_SetParams},
	{NULL,			NULL}		/* sentinel */
};

static PyMethodDef decompressor_methods[] = {
	{"close",		clm_CloseDecompressor},	/* alias */
	{"CloseDecompressor",	clm_CloseDecompressor},
	{"Decompress",		clm_Decompress},
	{"GetDefault",		clm_GetDefault},
	{"GetMinMax",		clm_GetMinMax},
	{"GetName",		clm_GetName},
	{"GetParam",		clm_GetParam},
	{"GetParamID",		clm_GetParamID},
	{"GetParams",		clm_GetParams},
	{"ReadHeader",		clm_ReadHeader},
	{"QueryParams",		clm_QueryParams},
	{"QuerySchemeFromHandle",clm_QuerySchemeFromHandle},
	{"SetParam",		clm_SetParam},
	{"SetParams",		clm_SetParams},
	{NULL,			NULL}		/* sentinel */
};

static void
cl_dealloc(PyObject *self)
{
	if (SELF->ob_compressorHdl) {
		if (SELF->ob_isCompressor)
			clCloseCompressor(SELF->ob_compressorHdl);
		else
			clCloseDecompressor(SELF->ob_compressorHdl);
	}
	PyMem_DEL(self);
}

static PyObject *
cl_getattr(PyObject *self, char *name)
{
	if (SELF->ob_isCompressor)
		return Py_FindMethod(compressor_methods, self, name);
	else
		return Py_FindMethod(decompressor_methods, self, name);
}

static PyTypeObject Cltype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"cl",			/*tp_name*/
	sizeof(clobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)cl_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)cl_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
};

static PyObject *
doOpen(PyObject *self, PyObject *args, int (*open_func)(int, CL_Handle *),
       int iscompressor)
{
	int scheme;
	clobject *new;

	if (!PyArg_Parse(args, "i", &scheme))
		return NULL;

	new = PyObject_NEW(clobject, &Cltype);
	if (new == NULL)
		return NULL;

	new->ob_compressorHdl = NULL;
	new->ob_isCompressor = iscompressor;
	new->ob_paramtypes = NULL;

	error_handler_called = 0;
	if ((*open_func)(scheme, &new->ob_compressorHdl) == FAILURE) {
		Py_DECREF(new);
		if (!error_handler_called)
			PyErr_SetString(ClError, "Open(De)Compressor failed");
		return NULL;
	}
	return (PyObject *)new;
}

static PyObject *
cl_OpenCompressor(PyObject *self, PyObject *args)
{
	return doOpen(self, args, clOpenCompressor, 1);
}

static PyObject *
cl_OpenDecompressor(PyObject *self, PyObject *args)
{
	return doOpen(self, args, clOpenDecompressor, 0);
}

static PyObject *
cl_QueryScheme(PyObject *self, PyObject *args)
{
	char *header;
	int headerlen;
	int scheme;

	if (!PyArg_Parse(args, "s#", &header, &headerlen))
		return NULL;

	scheme = clQueryScheme(header);
	if (scheme < 0) {
		PyErr_SetString(ClError, "unknown compression scheme");
		return NULL;
	}

	return PyInt_FromLong(scheme);
}

static PyObject *
cl_QueryMaxHeaderSize(PyObject *self, PyObject *args)
{
	int scheme;

	if (!PyArg_Parse(args, "i", &scheme))
		return NULL;

	return PyInt_FromLong(clQueryMaxHeaderSize(scheme));
}

static PyObject *
cl_QueryAlgorithms(PyObject *self, PyObject *args)
{
	int algorithmMediaType;
	int bufferlength;
	int *PVbuffer;
	PyObject *list;
	int i;

	if (!PyArg_Parse(args, "i", &algorithmMediaType))
		return NULL;

	error_handler_called = 0;
	bufferlength = clQueryAlgorithms(algorithmMediaType, 0, 0);
	if (error_handler_called)
		return NULL;

	PVbuffer = PyMem_NEW(int, bufferlength);
	if (PVbuffer == NULL)
		return PyErr_NoMemory();

	bufferlength = clQueryAlgorithms(algorithmMediaType, PVbuffer,
					 bufferlength);
	if (error_handler_called) {
		PyMem_DEL(PVbuffer);
		return NULL;
	}

	list = PyList_New(bufferlength);
	if (list == NULL) {
		PyMem_DEL(PVbuffer);
		return NULL;
	}

	for (i = 0; i < bufferlength; i++) {
		if (i & 1)
			PyList_SetItem(list, i, PyInt_FromLong(PVbuffer[i]));
		else if (PVbuffer[i] == 0) {
			Py_INCREF(Py_None);
			PyList_SetItem(list, i, Py_None);
		} else
			PyList_SetItem(list, i,
				   PyString_FromString((char *) PVbuffer[i]));
	}

	PyMem_DEL(PVbuffer);

	return list;
}

static PyObject *
cl_QuerySchemeFromName(PyObject *self, PyObject *args)
{
	int algorithmMediaType;
	char *name;
	int scheme;

	if (!PyArg_Parse(args, "(is)", &algorithmMediaType, &name))
		return NULL;

	error_handler_called = 0;
	scheme = clQuerySchemeFromName(algorithmMediaType, name);
	if (error_handler_called) {
		PyErr_SetString(ClError, "unknown compression scheme");
		return NULL;
	}

	return PyInt_FromLong(scheme);
}

static PyObject *
cl_GetAlgorithmName(PyObject *self, PyObject *args)
{
	int scheme;
	char *name;

	if (!PyArg_Parse(args, "i", &scheme))
		return NULL;

	name = clGetAlgorithmName(scheme);
	if (name == 0) {
		PyErr_SetString(ClError, "unknown compression scheme");
		return NULL;
	}

	return PyString_FromString(name);
}

static PyObject *
do_set(PyObject *self, PyObject *args, int (*func)(int, int, int))
{
	int scheme, paramID, value;
	float fvalue;
	int is_float = 0;

	if (!PyArg_Parse(args, "(iii)", &scheme, &paramID, &value)) {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(iif)", &scheme, &paramID, &fvalue)) {
			PyErr_Clear();
			PyErr_SetString(PyExc_TypeError,
			     "bad argument list (format '(iii)' or '(iif)')");
			return NULL;
		}
		value = CL_TypeIsInt(fvalue);
		is_float = 1;
	} else {
		/* check some parameters which we know to be floats */
		switch (scheme) {
		case CL_COMPRESSION_RATIO:
		case CL_SPEED:
			fvalue = value;
			value = CL_TypeIsInt(fvalue);
			is_float = 1;
			break;
		}
	}

 	error_handler_called = 0;
	value = (*func)(scheme, paramID, value);
	if (error_handler_called)
		return NULL;

	if (is_float)
		return PyFloat_FromDouble(CL_TypeIsFloat(value));
	else
		return PyInt_FromLong(value);
}

static PyObject *
cl_SetDefault(PyObject *self, PyObject *args)
{
	return do_set(self, args, clSetDefault);
}

static PyObject *
cl_SetMin(PyObject *self, PyObject *args)
{
	return do_set(self, args, clSetMin);
}

static PyObject *
cl_SetMax(PyObject *self, PyObject *args)
{
	return do_set(self, args, clSetMax);
}

#define func(name, handler)	\
static PyObject *cl_##name(PyObject *self, PyObject *args) \
{ \
	  int x; \
	  if (!PyArg_Parse(args, "i", &x)) return NULL; \
	  return Py##handler(CL_##name(x)); \
}

#define func2(name, handler)	\
static PyObject *cl_##name(PyObject *self, PyObject *args) \
{ \
	  int a1, a2; \
	  if (!PyArg_Parse(args, "(ii)", &a1, &a2)) return NULL; \
	  return Py##handler(CL_##name(a1, a2)); \
}

func(BytesPerSample, Int_FromLong)
func(BytesPerPixel, Int_FromLong)
func(AudioFormatName, String_FromString)
func(VideoFormatName, String_FromString)
func(AlgorithmNumber, Int_FromLong)
func(AlgorithmType, Int_FromLong)
func2(Algorithm, Int_FromLong)
func(ParamNumber, Int_FromLong)
func(ParamType, Int_FromLong)
func2(ParamID, Int_FromLong)

#ifdef CLDEBUG
	static PyObject *
cvt_type(PyObject *self, PyObject *args)
{
	int number;
	float fnumber;

	if (PyArg_Parse(args, "i", &number))
		return PyFloat_FromDouble(CL_TypeIsFloat(number));
	else {
		PyErr_Clear();
		if (PyArg_Parse(args, "f", &fnumber))
			return PyInt_FromLong(CL_TypeIsInt(fnumber));
		return NULL;
	}
}
#endif

static PyMethodDef cl_methods[] = {
	{"CompressImage",	cl_CompressImage},
	{"DecompressImage",	cl_DecompressImage},
	{"GetAlgorithmName",	cl_GetAlgorithmName},
	{"OpenCompressor",	cl_OpenCompressor},
	{"OpenDecompressor",	cl_OpenDecompressor},
	{"QueryAlgorithms",	cl_QueryAlgorithms},
	{"QueryMaxHeaderSize",	cl_QueryMaxHeaderSize},
	{"QueryScheme",		cl_QueryScheme},
	{"QuerySchemeFromName",	cl_QuerySchemeFromName},
	{"SetDefault",		cl_SetDefault},
	{"SetMax",		cl_SetMax},
	{"SetMin",		cl_SetMin},
	{"BytesPerSample",	cl_BytesPerSample},
	{"BytesPerPixel",	cl_BytesPerPixel},
	{"AudioFormatName",	cl_AudioFormatName},
	{"VideoFormatName",	cl_VideoFormatName},
	{"AlgorithmNumber",	cl_AlgorithmNumber},
	{"AlgorithmType",	cl_AlgorithmType},
	{"Algorithm",		cl_Algorithm},
	{"ParamNumber",		cl_ParamNumber},
	{"ParamType",		cl_ParamType},
	{"ParamID",		cl_ParamID},
#ifdef CLDEBUG
	{"cvt_type",		cvt_type},
#endif
	{NULL,			NULL} /* Sentinel */
};

#ifdef CL_JPEG_SOFTWARE
#define IRIX_5_3_LIBRARY
#endif

void
initcl()
{
	PyObject *m, *d;

	m = Py_InitModule("cl", cl_methods);
	d = PyModule_GetDict(m);

	ClError = PyString_FromString("cl.error");
	(void) PyDict_SetItemString(d, "error", ClError);

	(void) PyDict_SetItemString(d, "MAX_NUMBER_OF_ORIGINAL_FORMATS",
			   PyInt_FromLong(CL_MAX_NUMBER_OF_ORIGINAL_FORMATS));
	(void) PyDict_SetItemString(d, "MONO", PyInt_FromLong(CL_MONO));
	(void) PyDict_SetItemString(d, "STEREO_INTERLEAVED",
				    PyInt_FromLong(CL_STEREO_INTERLEAVED));
	(void) PyDict_SetItemString(d, "RGB", PyInt_FromLong(CL_RGB));
	(void) PyDict_SetItemString(d, "RGBX", PyInt_FromLong(CL_RGBX));
	(void) PyDict_SetItemString(d, "RGBA", PyInt_FromLong(CL_RGBA));
	(void) PyDict_SetItemString(d, "RGB332", PyInt_FromLong(CL_RGB332));
	(void) PyDict_SetItemString(d, "GRAYSCALE",
				    PyInt_FromLong(CL_GRAYSCALE));
	(void) PyDict_SetItemString(d, "Y", PyInt_FromLong(CL_Y));
	(void) PyDict_SetItemString(d, "YUV", PyInt_FromLong(CL_YUV));
	(void) PyDict_SetItemString(d, "YCbCr", PyInt_FromLong(CL_YCbCr));
	(void) PyDict_SetItemString(d, "YUV422", PyInt_FromLong(CL_YUV422));
	(void) PyDict_SetItemString(d, "YCbCr422",
				    PyInt_FromLong(CL_YCbCr422));
	(void) PyDict_SetItemString(d, "YUV422HC",
				    PyInt_FromLong(CL_YUV422HC));
	(void) PyDict_SetItemString(d, "YCbCr422HC",
				    PyInt_FromLong(CL_YCbCr422HC));
	(void) PyDict_SetItemString(d, "YUV422DC",
				    PyInt_FromLong(CL_YUV422DC));
	(void) PyDict_SetItemString(d, "YCbCr422DC",
				    PyInt_FromLong(CL_YCbCr422DC));
	(void) PyDict_SetItemString(d, "RGB8", PyInt_FromLong(CL_RGB8));
	(void) PyDict_SetItemString(d, "BEST_FIT",
				    PyInt_FromLong(CL_BEST_FIT));
	(void) PyDict_SetItemString(d, "MAX_NUMBER_OF_AUDIO_ALGORITHMS",
			   PyInt_FromLong(CL_MAX_NUMBER_OF_AUDIO_ALGORITHMS));
	(void) PyDict_SetItemString(d, "MAX_NUMBER_OF_VIDEO_ALGORITHMS",
			   PyInt_FromLong(CL_MAX_NUMBER_OF_VIDEO_ALGORITHMS));
	(void) PyDict_SetItemString(d, "AUDIO", PyInt_FromLong(CL_AUDIO));
	(void) PyDict_SetItemString(d, "VIDEO", PyInt_FromLong(CL_VIDEO));
	(void) PyDict_SetItemString(d, "UNKNOWN_SCHEME",
				    PyInt_FromLong(CL_UNKNOWN_SCHEME));
	(void) PyDict_SetItemString(d, "UNCOMPRESSED_AUDIO",
				    PyInt_FromLong(CL_UNCOMPRESSED_AUDIO));
	(void) PyDict_SetItemString(d, "G711_ULAW",
				    PyInt_FromLong(CL_G711_ULAW));
	(void) PyDict_SetItemString(d, "ULAW", PyInt_FromLong(CL_ULAW));
	(void) PyDict_SetItemString(d, "G711_ALAW",
				    PyInt_FromLong(CL_G711_ALAW));
	(void) PyDict_SetItemString(d, "ALAW", PyInt_FromLong(CL_ALAW));
	(void) PyDict_SetItemString(d, "AWARE_MPEG_AUDIO",
				    PyInt_FromLong(CL_AWARE_MPEG_AUDIO));
	(void) PyDict_SetItemString(d, "AWARE_MULTIRATE",
				    PyInt_FromLong(CL_AWARE_MULTIRATE));
	(void) PyDict_SetItemString(d, "UNCOMPRESSED",
				    PyInt_FromLong(CL_UNCOMPRESSED));
	(void) PyDict_SetItemString(d, "UNCOMPRESSED_VIDEO",
				    PyInt_FromLong(CL_UNCOMPRESSED_VIDEO));
	(void) PyDict_SetItemString(d, "RLE", PyInt_FromLong(CL_RLE));
	(void) PyDict_SetItemString(d, "JPEG", PyInt_FromLong(CL_JPEG));
#ifdef IRIX_5_3_LIBRARY
	(void) PyDict_SetItemString(d, "JPEG_SOFTWARE",
				    PyInt_FromLong(CL_JPEG_SOFTWARE));
#endif
	(void) PyDict_SetItemString(d, "MPEG_VIDEO",
				    PyInt_FromLong(CL_MPEG_VIDEO));
	(void) PyDict_SetItemString(d, "MVC1", PyInt_FromLong(CL_MVC1));
	(void) PyDict_SetItemString(d, "RTR", PyInt_FromLong(CL_RTR));
	(void) PyDict_SetItemString(d, "RTR1", PyInt_FromLong(CL_RTR1));
	(void) PyDict_SetItemString(d, "HDCC", PyInt_FromLong(CL_HDCC));
	(void) PyDict_SetItemString(d, "MVC2", PyInt_FromLong(CL_MVC2));
	(void) PyDict_SetItemString(d, "RLE24", PyInt_FromLong(CL_RLE24));
	(void) PyDict_SetItemString(d, "MAX_NUMBER_OF_PARAMS",
				    PyInt_FromLong(CL_MAX_NUMBER_OF_PARAMS));
	(void) PyDict_SetItemString(d, "IMAGE_WIDTH",
				    PyInt_FromLong(CL_IMAGE_WIDTH));
	(void) PyDict_SetItemString(d, "IMAGE_HEIGHT",
				    PyInt_FromLong(CL_IMAGE_HEIGHT));
	(void) PyDict_SetItemString(d, "ORIGINAL_FORMAT",
				    PyInt_FromLong(CL_ORIGINAL_FORMAT));
	(void) PyDict_SetItemString(d, "INTERNAL_FORMAT",
				    PyInt_FromLong(CL_INTERNAL_FORMAT));
	(void) PyDict_SetItemString(d, "COMPONENTS",
				    PyInt_FromLong(CL_COMPONENTS));
	(void) PyDict_SetItemString(d, "BITS_PER_COMPONENT",
				    PyInt_FromLong(CL_BITS_PER_COMPONENT));
	(void) PyDict_SetItemString(d, "FRAME_RATE",
				    PyInt_FromLong(CL_FRAME_RATE));
	(void) PyDict_SetItemString(d, "COMPRESSION_RATIO",
				    PyInt_FromLong(CL_COMPRESSION_RATIO));
	(void) PyDict_SetItemString(d, "EXACT_COMPRESSION_RATIO",
				  PyInt_FromLong(CL_EXACT_COMPRESSION_RATIO));
	(void) PyDict_SetItemString(d, "FRAME_BUFFER_SIZE",
				    PyInt_FromLong(CL_FRAME_BUFFER_SIZE));
	(void) PyDict_SetItemString(d, "COMPRESSED_BUFFER_SIZE",
				    PyInt_FromLong(CL_COMPRESSED_BUFFER_SIZE));
	(void) PyDict_SetItemString(d, "BLOCK_SIZE",
				    PyInt_FromLong(CL_BLOCK_SIZE));
	(void) PyDict_SetItemString(d, "PREROLL", PyInt_FromLong(CL_PREROLL));
	(void) PyDict_SetItemString(d, "FRAME_TYPE",
				    PyInt_FromLong(CL_FRAME_TYPE));
	(void) PyDict_SetItemString(d, "ALGORITHM_ID",
				    PyInt_FromLong(CL_ALGORITHM_ID));
	(void) PyDict_SetItemString(d, "ALGORITHM_VERSION",
				    PyInt_FromLong(CL_ALGORITHM_VERSION));
	(void) PyDict_SetItemString(d, "ORIENTATION",
				    PyInt_FromLong(CL_ORIENTATION));
	(void) PyDict_SetItemString(d, "NUMBER_OF_FRAMES",
				    PyInt_FromLong(CL_NUMBER_OF_FRAMES));
	(void) PyDict_SetItemString(d, "SPEED", PyInt_FromLong(CL_SPEED));
	(void) PyDict_SetItemString(d, "LAST_FRAME_INDEX",
				    PyInt_FromLong(CL_LAST_FRAME_INDEX));
#ifdef IRIX_5_3_LIBRARY
	(void) PyDict_SetItemString(d, "ENABLE_IMAGEINFO",
				    PyInt_FromLong(CL_ENABLE_IMAGEINFO));
	(void) PyDict_SetItemString(d, "INTERNAL_IMAGE_WIDTH",
				    PyInt_FromLong(CL_INTERNAL_IMAGE_WIDTH));
	(void) PyDict_SetItemString(d, "INTERNAL_IMAGE_HEIGHT",
				    PyInt_FromLong(CL_INTERNAL_IMAGE_HEIGHT));
#endif
	(void) PyDict_SetItemString(d, "NUMBER_OF_PARAMS",
				    PyInt_FromLong(CL_NUMBER_OF_PARAMS));
#ifdef IRIX_5_3_LIBRARY
	(void) PyDict_SetItemString(d, "MVC2_LUMA_THRESHOLD",
				    PyInt_FromLong(CL_MVC2_LUMA_THRESHOLD));
	(void) PyDict_SetItemString(d, "MVC2_CHROMA_THRESHOLD",
				    PyInt_FromLong(CL_MVC2_CHROMA_THRESHOLD));
	(void) PyDict_SetItemString(d, "MVC2_EDGE_THRESHOLD",
				    PyInt_FromLong(CL_MVC2_EDGE_THRESHOLD));
	(void) PyDict_SetItemString(d, "MVC2_BLENDING",
				    PyInt_FromLong(CL_MVC2_BLENDING));
	(void) PyDict_SetItemString(d, "MVC2_BLENDING_OFF",
				    PyInt_FromLong(CL_MVC2_BLENDING_OFF));
	(void) PyDict_SetItemString(d, "MVC2_BLENDING_ON",
				    PyInt_FromLong(CL_MVC2_BLENDING_ON));
	(void) PyDict_SetItemString(d, "JPEG_QUALITY_FACTOR",
				    PyInt_FromLong(CL_JPEG_QUALITY_FACTOR));
	(void) PyDict_SetItemString(d, "JPEG_STREAM_HEADERS",
				    PyInt_FromLong(CL_JPEG_STREAM_HEADERS));
	(void) PyDict_SetItemString(d, "JPEG_QUANTIZATION_TABLES",
				 PyInt_FromLong(CL_JPEG_QUANTIZATION_TABLES));
	(void) PyDict_SetItemString(d, "JPEG_NUM_PARAMS",
				    PyInt_FromLong(CL_JPEG_NUM_PARAMS));
	(void) PyDict_SetItemString(d, "RTR_QUALITY_LEVEL",
				    PyInt_FromLong(CL_RTR_QUALITY_LEVEL));
	(void) PyDict_SetItemString(d, "HDCC_TILE_THRESHOLD",
				    PyInt_FromLong(CL_HDCC_TILE_THRESHOLD));
	(void) PyDict_SetItemString(d, "HDCC_SAMPLES_PER_TILE",
				    PyInt_FromLong(CL_HDCC_SAMPLES_PER_TILE));
#endif
	(void) PyDict_SetItemString(d, "END_OF_SEQUENCE",
				    PyInt_FromLong(CL_END_OF_SEQUENCE));
	(void) PyDict_SetItemString(d, "CHANNEL_POLICY",
				    PyInt_FromLong(CL_CHANNEL_POLICY));
	(void) PyDict_SetItemString(d, "NOISE_MARGIN",
				    PyInt_FromLong(CL_NOISE_MARGIN));
	(void) PyDict_SetItemString(d, "BITRATE_POLICY",
				    PyInt_FromLong(CL_BITRATE_POLICY));
	(void) PyDict_SetItemString(d, "BITRATE_TARGET",
				    PyInt_FromLong(CL_BITRATE_TARGET));
	(void) PyDict_SetItemString(d, "LAYER", PyInt_FromLong(CL_LAYER));
	(void) PyDict_SetItemString(d, "ENUM_VALUE",
				    PyInt_FromLong(CL_ENUM_VALUE));
	(void) PyDict_SetItemString(d, "RANGE_VALUE",
				    PyInt_FromLong(CL_RANGE_VALUE));
	(void) PyDict_SetItemString(d, "FLOATING_ENUM_VALUE",
				    PyInt_FromLong(CL_FLOATING_ENUM_VALUE));
	(void) PyDict_SetItemString(d, "FLOATING_RANGE_VALUE",
				    PyInt_FromLong(CL_FLOATING_RANGE_VALUE));
	(void) PyDict_SetItemString(d, "DECOMPRESSOR",
				    PyInt_FromLong(CL_DECOMPRESSOR));
	(void) PyDict_SetItemString(d, "COMPRESSOR",
				    PyInt_FromLong(CL_COMPRESSOR));
	(void) PyDict_SetItemString(d, "CODEC", PyInt_FromLong(CL_CODEC));
	(void) PyDict_SetItemString(d, "NONE", PyInt_FromLong(CL_NONE));
#ifdef IRIX_5_3_LIBRARY
	(void) PyDict_SetItemString(d, "BUF_FRAME",
				    PyInt_FromLong(CL_BUF_FRAME));
	(void) PyDict_SetItemString(d, "BUF_DATA",
				    PyInt_FromLong(CL_BUF_DATA));
#endif
#ifdef CL_FRAME
	(void) PyDict_SetItemString(d, "FRAME", PyInt_FromLong(CL_FRAME));
	(void) PyDict_SetItemString(d, "DATA", PyInt_FromLong(CL_DATA));
#endif
	(void) PyDict_SetItemString(d, "NONE", PyInt_FromLong(CL_NONE));
	(void) PyDict_SetItemString(d, "KEYFRAME",
				    PyInt_FromLong(CL_KEYFRAME));
	(void) PyDict_SetItemString(d, "INTRA", PyInt_FromLong(CL_INTRA));
	(void) PyDict_SetItemString(d, "PREDICTED",
				    PyInt_FromLong(CL_PREDICTED));
	(void) PyDict_SetItemString(d, "BIDIRECTIONAL",
				    PyInt_FromLong(CL_BIDIRECTIONAL));
	(void) PyDict_SetItemString(d, "TOP_DOWN",
				    PyInt_FromLong(CL_TOP_DOWN));
	(void) PyDict_SetItemString(d, "BOTTOM_UP",
				    PyInt_FromLong(CL_BOTTOM_UP));
#ifdef IRIX_5_3_LIBRARY
	(void) PyDict_SetItemString(d, "CONTINUOUS_BLOCK",
				    PyInt_FromLong(CL_CONTINUOUS_BLOCK));
	(void) PyDict_SetItemString(d, "CONTINUOUS_NONBLOCK",
				    PyInt_FromLong(CL_CONTINUOUS_NONBLOCK));
	(void) PyDict_SetItemString(d, "EXTERNAL_DEVICE",
				    PyInt_FromLong((long)CL_EXTERNAL_DEVICE));
#endif
	(void) PyDict_SetItemString(d, "AWCMP_STEREO",
				    PyInt_FromLong(AWCMP_STEREO));
	(void) PyDict_SetItemString(d, "AWCMP_JOINT_STEREO",
				    PyInt_FromLong(AWCMP_JOINT_STEREO));
	(void) PyDict_SetItemString(d, "AWCMP_INDEPENDENT",
				    PyInt_FromLong(AWCMP_INDEPENDENT));
	(void) PyDict_SetItemString(d, "AWCMP_FIXED_RATE",
				    PyInt_FromLong(AWCMP_FIXED_RATE));
	(void) PyDict_SetItemString(d, "AWCMP_CONST_QUAL",
				    PyInt_FromLong(AWCMP_CONST_QUAL));
	(void) PyDict_SetItemString(d, "AWCMP_LOSSLESS",
				    PyInt_FromLong(AWCMP_LOSSLESS));
	(void) PyDict_SetItemString(d, "AWCMP_MPEG_LAYER_I",
				    PyInt_FromLong(AWCMP_MPEG_LAYER_I));
	(void) PyDict_SetItemString(d, "AWCMP_MPEG_LAYER_II",
				    PyInt_FromLong(AWCMP_MPEG_LAYER_II));
	(void) PyDict_SetItemString(d, "HEADER_START_CODE",
				    PyInt_FromLong(CL_HEADER_START_CODE));
	(void) PyDict_SetItemString(d, "BAD_NO_BUFFERSPACE",
				    PyInt_FromLong(CL_BAD_NO_BUFFERSPACE));
	(void) PyDict_SetItemString(d, "BAD_PVBUFFER",
				    PyInt_FromLong(CL_BAD_PVBUFFER));
	(void) PyDict_SetItemString(d, "BAD_BUFFERLENGTH_NEG",
				    PyInt_FromLong(CL_BAD_BUFFERLENGTH_NEG));
	(void) PyDict_SetItemString(d, "BAD_BUFFERLENGTH_ODD",
				    PyInt_FromLong(CL_BAD_BUFFERLENGTH_ODD));
	(void) PyDict_SetItemString(d, "BAD_PARAM",
				    PyInt_FromLong(CL_BAD_PARAM));
	(void) PyDict_SetItemString(d, "BAD_COMPRESSION_SCHEME",
				    PyInt_FromLong(CL_BAD_COMPRESSION_SCHEME));
	(void) PyDict_SetItemString(d, "BAD_COMPRESSOR_HANDLE",
				    PyInt_FromLong(CL_BAD_COMPRESSOR_HANDLE));
	(void) PyDict_SetItemString(d, "BAD_COMPRESSOR_HANDLE_POINTER",
			    PyInt_FromLong(CL_BAD_COMPRESSOR_HANDLE_POINTER));
	(void) PyDict_SetItemString(d, "BAD_BUFFER_HANDLE",
				    PyInt_FromLong(CL_BAD_BUFFER_HANDLE));
	(void) PyDict_SetItemString(d, "BAD_BUFFER_QUERY_SIZE",
				    PyInt_FromLong(CL_BAD_BUFFER_QUERY_SIZE));
	(void) PyDict_SetItemString(d, "JPEG_ERROR",
				    PyInt_FromLong(CL_JPEG_ERROR));
	(void) PyDict_SetItemString(d, "BAD_FRAME_SIZE",
				    PyInt_FromLong(CL_BAD_FRAME_SIZE));
	(void) PyDict_SetItemString(d, "PARAM_OUT_OF_RANGE",
				    PyInt_FromLong(CL_PARAM_OUT_OF_RANGE));
	(void) PyDict_SetItemString(d, "ADDED_ALGORITHM_ERROR",
				    PyInt_FromLong(CL_ADDED_ALGORITHM_ERROR));
	(void) PyDict_SetItemString(d, "BAD_ALGORITHM_TYPE",
				    PyInt_FromLong(CL_BAD_ALGORITHM_TYPE));
	(void) PyDict_SetItemString(d, "BAD_ALGORITHM_NAME",
				    PyInt_FromLong(CL_BAD_ALGORITHM_NAME));
	(void) PyDict_SetItemString(d, "BAD_BUFFERING",
				    PyInt_FromLong(CL_BAD_BUFFERING));
	(void) PyDict_SetItemString(d, "BUFFER_NOT_CREATED",
				    PyInt_FromLong(CL_BUFFER_NOT_CREATED));
	(void) PyDict_SetItemString(d, "BAD_BUFFER_EXISTS",
				    PyInt_FromLong(CL_BAD_BUFFER_EXISTS));
	(void) PyDict_SetItemString(d, "BAD_INTERNAL_FORMAT",
				    PyInt_FromLong(CL_BAD_INTERNAL_FORMAT));
	(void) PyDict_SetItemString(d, "BAD_BUFFER_POINTER",
				    PyInt_FromLong(CL_BAD_BUFFER_POINTER));
	(void) PyDict_SetItemString(d, "FRAME_BUFFER_SIZE_ZERO",
				    PyInt_FromLong(CL_FRAME_BUFFER_SIZE_ZERO));
	(void) PyDict_SetItemString(d, "BAD_STREAM_HEADER",
				    PyInt_FromLong(CL_BAD_STREAM_HEADER));
	(void) PyDict_SetItemString(d, "BAD_LICENSE",
				    PyInt_FromLong(CL_BAD_LICENSE));
	(void) PyDict_SetItemString(d, "AWARE_ERROR",
				    PyInt_FromLong(CL_AWARE_ERROR));
	(void) PyDict_SetItemString(d, "BAD_BUFFER_SIZE_POINTER",
				 PyInt_FromLong(CL_BAD_BUFFER_SIZE_POINTER));
	(void) PyDict_SetItemString(d, "BAD_BUFFER_SIZE",
				    PyInt_FromLong(CL_BAD_BUFFER_SIZE));
	(void) PyDict_SetItemString(d, "BAD_BUFFER_TYPE",
				    PyInt_FromLong(CL_BAD_BUFFER_TYPE));
	(void) PyDict_SetItemString(d, "BAD_HEADER_SIZE",
				    PyInt_FromLong(CL_BAD_HEADER_SIZE));
	(void) PyDict_SetItemString(d, "BAD_FUNCTION_POINTER",
				    PyInt_FromLong(CL_BAD_FUNCTION_POINTER));
	(void) PyDict_SetItemString(d, "BAD_SCHEME_POINTER",
				    PyInt_FromLong(CL_BAD_SCHEME_POINTER));
	(void) PyDict_SetItemString(d, "BAD_STRING_POINTER",
				    PyInt_FromLong(CL_BAD_STRING_POINTER));
	(void) PyDict_SetItemString(d, "BAD_MIN_GT_MAX",
				    PyInt_FromLong(CL_BAD_MIN_GT_MAX));
	(void) PyDict_SetItemString(d, "BAD_INITIAL_VALUE",
				    PyInt_FromLong(CL_BAD_INITIAL_VALUE));
	(void) PyDict_SetItemString(d, "BAD_PARAM_ID_POINTER",
				    PyInt_FromLong(CL_BAD_PARAM_ID_POINTER));
	(void) PyDict_SetItemString(d, "BAD_PARAM_TYPE",
				    PyInt_FromLong(CL_BAD_PARAM_TYPE));
	(void) PyDict_SetItemString(d, "BAD_TEXT_STRING_PTR",
				    PyInt_FromLong(CL_BAD_TEXT_STRING_PTR));
	(void) PyDict_SetItemString(d, "BAD_FUNCTIONALITY",
				    PyInt_FromLong(CL_BAD_FUNCTIONALITY));
	(void) PyDict_SetItemString(d, "BAD_NUMBER_OF_BLOCKS",
				    PyInt_FromLong(CL_BAD_NUMBER_OF_BLOCKS));
	(void) PyDict_SetItemString(d, "BAD_BLOCK_SIZE",
				    PyInt_FromLong(CL_BAD_BLOCK_SIZE));
	(void) PyDict_SetItemString(d, "BAD_POINTER",
				    PyInt_FromLong(CL_BAD_POINTER));
	(void) PyDict_SetItemString(d, "BAD_BOARD",
				    PyInt_FromLong(CL_BAD_BOARD));
	(void) PyDict_SetItemString(d, "MVC2_ERROR",
				    PyInt_FromLong(CL_MVC2_ERROR));
#ifdef IRIX_5_3_LIBRARY
	(void) PyDict_SetItemString(d, "NEXT_NOT_AVAILABLE",
				    PyInt_FromLong(CL_NEXT_NOT_AVAILABLE));
	(void) PyDict_SetItemString(d, "SCHEME_BUSY",
				    PyInt_FromLong(CL_SCHEME_BUSY));
	(void) PyDict_SetItemString(d, "SCHEME_NOT_AVAILABLE",
				    PyInt_FromLong(CL_SCHEME_NOT_AVAILABLE));
#endif
#ifdef CL_LUMA_THRESHOLD
	/* backward compatibility */
	(void) PyDict_SetItemString(d, "LUMA_THRESHOLD",
				    PyInt_FromLong(CL_LUMA_THRESHOLD));
	(void) PyDict_SetItemString(d, "CHROMA_THRESHOLD",
				    PyInt_FromLong(CL_CHROMA_THRESHOLD));
	(void) PyDict_SetItemString(d, "EDGE_THRESHOLD",
				    PyInt_FromLong(CL_EDGE_THRESHOLD));
	(void) PyDict_SetItemString(d, "BLENDING",
				    PyInt_FromLong(CL_BLENDING));
	(void) PyDict_SetItemString(d, "QUALITY_FACTOR",
				    PyInt_FromLong(CL_QUALITY_FACTOR));
	(void) PyDict_SetItemString(d, "STREAM_HEADERS",
				    PyInt_FromLong(CL_STREAM_HEADERS));
	(void) PyDict_SetItemString(d, "QUALITY_LEVEL",
				    PyInt_FromLong(CL_QUALITY_LEVEL));
	(void) PyDict_SetItemString(d, "TILE_THRESHOLD",
				    PyInt_FromLong(CL_TILE_THRESHOLD));
	(void) PyDict_SetItemString(d, "SAMPLES_PER_TILE",
				    PyInt_FromLong(CL_SAMPLES_PER_TILE));
#endif

	if (PyErr_Occurred())
		Py_FatalError("can't initialize module cl");

	(void) clSetErrorHandler(cl_ErrorHandler);
}
