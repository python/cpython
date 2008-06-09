

/* Cl objects */

#define CLDEBUG

#include <stdarg.h>
#include <cl.h>
#if defined(CL_JPEG_SOFTWARE) && !defined(CL_JPEG_COSMO)
#include <dmedia/cl_cosmo.h>
#endif
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

	if (!PyArg_ParseTuple(args, "iiiifs#", &compressionScheme,
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
	    == FAILURE || error_handler_called) {
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
		_PyString_Resize(&compressedBuffer, compressedBufferSize);

	return compressedBuffer;
}

static PyObject *
cl_DecompressImage(PyObject *self, PyObject *args)
{
	int compressionScheme, width, height, originalFormat;
	char *compressedBuffer;
	int compressedBufferSize, frameBufferSize;
	PyObject *frameBuffer;

	if (!PyArg_ParseTuple(args, "iiiis#", &compressionScheme, &width, &height,
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
	    == FAILURE || error_handler_called) {
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
doClose(clobject *self, int (*close_func)(CL_Handle))
{
	CheckCompressor(self);

	error_handler_called = 0;
	if ((*close_func)(self->ob_compressorHdl) == FAILURE ||
	    error_handler_called) {
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
clm_CloseCompressor(PyObject *self)
{
	return doClose(SELF, clCloseCompressor);
}

static PyObject *
clm_CloseDecompressor(PyObject *self)
{
	return doClose(SELF, clCloseDecompressor);
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
		       (void *) PyString_AsString(data)) == FAILURE ||
	    error_handler_called) {
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
			 (void *) PyString_AsString(data)) == FAILURE ||
	    error_handler_called) {
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
	if (value == FAILURE || error_handler_called) {
		if (!error_handler_called)
			PyErr_SetString(ClError, "getparamid failed");
		return NULL;
	}

	return PyInt_FromLong(value);
}

static PyObject *
clm_QueryParams(PyObject *self)
{
	int bufferlength;
	int *PVbuffer;
	PyObject *list;
	int i;

	CheckCompressor(SELF);

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
clm_QuerySchemeFromHandle(PyObject *self)
{
	CheckCompressor(SELF);
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
	{"close",		clm_CloseCompressor, METH_NOARGS}, /* alias */
	{"CloseCompressor",	clm_CloseCompressor, METH_NOARGS},
	{"Compress",		clm_Compress, METH_OLDARGS},
	{"GetDefault",		clm_GetDefault, METH_OLDARGS},
	{"GetMinMax",		clm_GetMinMax, METH_OLDARGS},
	{"GetName",		clm_GetName, METH_OLDARGS},
	{"GetParam",		clm_GetParam, METH_OLDARGS},
	{"GetParamID",		clm_GetParamID, METH_OLDARGS},
	{"GetParams",		clm_GetParams, METH_OLDARGS},
	{"QueryParams",		clm_QueryParams, METH_NOARGS},
	{"QuerySchemeFromHandle",clm_QuerySchemeFromHandle, METH_NOARGS},
	{"SetParam",		clm_SetParam, METH_OLDARGS},
	{"SetParams",		clm_SetParams, METH_OLDARGS},
	{NULL,			NULL}		/* sentinel */
};

static PyMethodDef decompressor_methods[] = {
	{"close",		clm_CloseDecompressor, METH_NOARGS},	/* alias */
	{"CloseDecompressor",	clm_CloseDecompressor, METH_NOARGS},
	{"Decompress",		clm_Decompress, METH_OLDARGS},
	{"GetDefault",		clm_GetDefault, METH_OLDARGS},
	{"GetMinMax",		clm_GetMinMax, METH_OLDARGS},
	{"GetName",		clm_GetName, METH_OLDARGS},
	{"GetParam",		clm_GetParam, METH_OLDARGS},
	{"GetParamID",		clm_GetParamID, METH_OLDARGS},
	{"GetParams",		clm_GetParams, METH_OLDARGS},
	{"ReadHeader",		clm_ReadHeader, METH_OLDARGS},
	{"QueryParams",		clm_QueryParams, METH_NOARGS},
	{"QuerySchemeFromHandle",clm_QuerySchemeFromHandle, METH_NOARGS},
	{"SetParam",		clm_SetParam, METH_OLDARGS},
	{"SetParams",		clm_SetParams, METH_OLDARGS},
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
	PyObject_Del(self);
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
	"cl.cl",		/*tp_name*/
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

	if (!PyArg_ParseTuple(args, "i", &scheme))
		return NULL;

	new = PyObject_New(clobject, &Cltype);
	if (new == NULL)
		return NULL;

	new->ob_compressorHdl = NULL;
	new->ob_isCompressor = iscompressor;
	new->ob_paramtypes = NULL;

	error_handler_called = 0;
	if ((*open_func)(scheme, &new->ob_compressorHdl) == FAILURE ||
	    error_handler_called) {
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

	if (!PyArg_ParseTuple(args, "s#", &header, &headerlen))
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

	if (!PyArg_ParseTuple(args, "i", &scheme))
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

	if (!PyArg_ParseTuple(args, "i", &algorithmMediaType))
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

	if (!PyArg_ParseTuple(args, "is", &algorithmMediaType, &name))
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

	if (!PyArg_ParseTuple(args, "i", &scheme))
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

	if (!PyArg_ParseTuple(args, "iii", &scheme, &paramID, &value)) {
		PyErr_Clear();
		if (!PyArg_ParseTuple(args, "iif", &scheme, &paramID, &fvalue)) {
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
	  if (!PyArg_ParseTuple(args, "i", &x)) return NULL; \
	  return Py##handler(CL_##name(x)); \
}

#define func2(name, handler)	\
static PyObject *cl_##name(PyObject *self, PyObject *args) \
{ \
	  int a1, a2; \
	  if (!PyArg_ParseTuple(args, "ii", &a1, &a2)) return NULL; \
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
	{"CompressImage",	cl_CompressImage, METH_VARARGS},
	{"DecompressImage",	cl_DecompressImage, METH_VARARGS},
	{"GetAlgorithmName",	cl_GetAlgorithmName, METH_VARARGS},
	{"OpenCompressor",	cl_OpenCompressor, METH_VARARGS},
	{"OpenDecompressor",	cl_OpenDecompressor, METH_VARARGS},
	{"QueryAlgorithms",	cl_QueryAlgorithms, METH_VARARGS},
	{"QueryMaxHeaderSize",	cl_QueryMaxHeaderSize, METH_VARARGS},
	{"QueryScheme",		cl_QueryScheme, METH_VARARGS},
	{"QuerySchemeFromName",	cl_QuerySchemeFromName, METH_VARARGS},
	{"SetDefault",		cl_SetDefault, METH_VARARGS},
	{"SetMax",		cl_SetMax, METH_VARARGS},
	{"SetMin",		cl_SetMin, METH_VARARGS},
	{"BytesPerSample",	cl_BytesPerSample, METH_VARARGS},
	{"BytesPerPixel",	cl_BytesPerPixel, METH_VARARGS},
	{"AudioFormatName",	cl_AudioFormatName, METH_VARARGS},
	{"VideoFormatName",	cl_VideoFormatName, METH_VARARGS},
	{"AlgorithmNumber",	cl_AlgorithmNumber, METH_VARARGS},
	{"AlgorithmType",	cl_AlgorithmType, METH_VARARGS},
	{"Algorithm",		cl_Algorithm, METH_VARARGS},
	{"ParamNumber",		cl_ParamNumber, METH_VARARGS},
	{"ParamType",		cl_ParamType, METH_VARARGS},
	{"ParamID",		cl_ParamID, METH_VARARGS},
#ifdef CLDEBUG
	{"cvt_type",		cvt_type, METH_VARARGS},
#endif
	{NULL,			NULL} /* Sentinel */
};

#ifdef CL_JPEG_SOFTWARE
#define IRIX_5_3_LIBRARY
#endif

void
initcl(void)
{
	PyObject *m, *d, *x;
    
    if (PyErr_WarnPy3k("the cl module has been removed in "
                       "Python 3.0", 2) < 0)
        return;
    
	m = Py_InitModule("cl", cl_methods);
	if (m == NULL)
		return;
	d = PyModule_GetDict(m);

	ClError = PyErr_NewException("cl.error", NULL, NULL);
	(void) PyDict_SetItemString(d, "error", ClError);

#ifdef CL_ADDED_ALGORITHM_ERROR
	x = PyInt_FromLong(CL_ADDED_ALGORITHM_ERROR);
	if (x == NULL || PyDict_SetItemString(d, "ADDED_ALGORITHM_ERROR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_ALAW
	x = PyInt_FromLong(CL_ALAW);
	if (x == NULL || PyDict_SetItemString(d, "ALAW", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_ALGORITHM_ID
	x = PyInt_FromLong(CL_ALGORITHM_ID);
	if (x == NULL || PyDict_SetItemString(d, "ALGORITHM_ID", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_ALGORITHM_TABLE_FULL
	x = PyInt_FromLong(CL_ALGORITHM_TABLE_FULL);
	if (x == NULL || PyDict_SetItemString(d, "ALGORITHM_TABLE_FULL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_ALGORITHM_VERSION
	x = PyInt_FromLong(CL_ALGORITHM_VERSION);
	if (x == NULL || PyDict_SetItemString(d, "ALGORITHM_VERSION", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_ALG_AUDIO
	x = PyInt_FromLong(CL_ALG_AUDIO);
	if (x == NULL || PyDict_SetItemString(d, "ALG_AUDIO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_ALG_VIDEO
	x = PyInt_FromLong(CL_ALG_VIDEO);
	if (x == NULL || PyDict_SetItemString(d, "ALG_VIDEO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AUDIO
	x = PyInt_FromLong(CL_AUDIO);
	if (x == NULL || PyDict_SetItemString(d, "AUDIO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_BITRATE_POLICY
	x = PyInt_FromLong(CL_AWARE_BITRATE_POLICY);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_BITRATE_POLICY", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_BITRATE_TARGET
	x = PyInt_FromLong(CL_AWARE_BITRATE_TARGET);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_BITRATE_TARGET", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_CHANNEL_POLICY
	x = PyInt_FromLong(CL_AWARE_CHANNEL_POLICY);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_CHANNEL_POLICY", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_CONST_QUAL
	x = PyInt_FromLong(CL_AWARE_CONST_QUAL);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_CONST_QUAL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_ERROR
	x = PyInt_FromLong(CL_AWARE_ERROR);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_ERROR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_FIXED_RATE
	x = PyInt_FromLong(CL_AWARE_FIXED_RATE);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_FIXED_RATE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_INDEPENDENT
	x = PyInt_FromLong(CL_AWARE_INDEPENDENT);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_INDEPENDENT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_JOINT_STEREO
	x = PyInt_FromLong(CL_AWARE_JOINT_STEREO);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_JOINT_STEREO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_LAYER
	x = PyInt_FromLong(CL_AWARE_LAYER);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_LAYER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_LOSSLESS
	x = PyInt_FromLong(CL_AWARE_LOSSLESS);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_LOSSLESS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_MPEG_AUDIO
	x = PyInt_FromLong(CL_AWARE_MPEG_AUDIO);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_MPEG_AUDIO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_MPEG_LAYER_I
	x = PyInt_FromLong(CL_AWARE_MPEG_LAYER_I);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_MPEG_LAYER_I", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_MPEG_LAYER_II
	x = PyInt_FromLong(CL_AWARE_MPEG_LAYER_II);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_MPEG_LAYER_II", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_MULTIRATE
	x = PyInt_FromLong(CL_AWARE_MULTIRATE);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_MULTIRATE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_NOISE_MARGIN
	x = PyInt_FromLong(CL_AWARE_NOISE_MARGIN);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_NOISE_MARGIN", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_AWARE_STEREO
	x = PyInt_FromLong(CL_AWARE_STEREO);
	if (x == NULL || PyDict_SetItemString(d, "AWARE_STEREO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_ALGORITHM_NAME
	x = PyInt_FromLong(CL_BAD_ALGORITHM_NAME);
	if (x == NULL || PyDict_SetItemString(d, "BAD_ALGORITHM_NAME", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_ALGORITHM_TYPE
	x = PyInt_FromLong(CL_BAD_ALGORITHM_TYPE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_ALGORITHM_TYPE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_BLOCK_SIZE
	x = PyInt_FromLong(CL_BAD_BLOCK_SIZE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_BLOCK_SIZE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_BOARD
	x = PyInt_FromLong(CL_BAD_BOARD);
	if (x == NULL || PyDict_SetItemString(d, "BAD_BOARD", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_BUFFERING
	x = PyInt_FromLong(CL_BAD_BUFFERING);
	if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFERING", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_BUFFERLENGTH_NEG
	x = PyInt_FromLong(CL_BAD_BUFFERLENGTH_NEG);
	if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFERLENGTH_NEG", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_BUFFERLENGTH_ODD
	x = PyInt_FromLong(CL_BAD_BUFFERLENGTH_ODD);
	if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFERLENGTH_ODD", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_BUFFER_EXISTS
	x = PyInt_FromLong(CL_BAD_BUFFER_EXISTS);
	if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFER_EXISTS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_BUFFER_HANDLE
	x = PyInt_FromLong(CL_BAD_BUFFER_HANDLE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFER_HANDLE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_BUFFER_POINTER
	x = PyInt_FromLong(CL_BAD_BUFFER_POINTER);
	if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFER_POINTER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_BUFFER_QUERY_SIZE
	x = PyInt_FromLong(CL_BAD_BUFFER_QUERY_SIZE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFER_QUERY_SIZE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_BUFFER_SIZE
	x = PyInt_FromLong(CL_BAD_BUFFER_SIZE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFER_SIZE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_BUFFER_SIZE_POINTER
	x = PyInt_FromLong(CL_BAD_BUFFER_SIZE_POINTER);
	if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFER_SIZE_POINTER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_BUFFER_TYPE
	x = PyInt_FromLong(CL_BAD_BUFFER_TYPE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFER_TYPE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_COMPRESSION_SCHEME
	x = PyInt_FromLong(CL_BAD_COMPRESSION_SCHEME);
	if (x == NULL || PyDict_SetItemString(d, "BAD_COMPRESSION_SCHEME", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_COMPRESSOR_HANDLE
	x = PyInt_FromLong(CL_BAD_COMPRESSOR_HANDLE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_COMPRESSOR_HANDLE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_COMPRESSOR_HANDLE_POINTER
	x = PyInt_FromLong(CL_BAD_COMPRESSOR_HANDLE_POINTER);
	if (x == NULL || PyDict_SetItemString(d, "BAD_COMPRESSOR_HANDLE_POINTER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_FRAME_SIZE
	x = PyInt_FromLong(CL_BAD_FRAME_SIZE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_FRAME_SIZE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_FUNCTIONALITY
	x = PyInt_FromLong(CL_BAD_FUNCTIONALITY);
	if (x == NULL || PyDict_SetItemString(d, "BAD_FUNCTIONALITY", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_FUNCTION_POINTER
	x = PyInt_FromLong(CL_BAD_FUNCTION_POINTER);
	if (x == NULL || PyDict_SetItemString(d, "BAD_FUNCTION_POINTER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_HEADER_SIZE
	x = PyInt_FromLong(CL_BAD_HEADER_SIZE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_HEADER_SIZE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_INITIAL_VALUE
	x = PyInt_FromLong(CL_BAD_INITIAL_VALUE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_INITIAL_VALUE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_INTERNAL_FORMAT
	x = PyInt_FromLong(CL_BAD_INTERNAL_FORMAT);
	if (x == NULL || PyDict_SetItemString(d, "BAD_INTERNAL_FORMAT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_LICENSE
	x = PyInt_FromLong(CL_BAD_LICENSE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_LICENSE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_MIN_GT_MAX
	x = PyInt_FromLong(CL_BAD_MIN_GT_MAX);
	if (x == NULL || PyDict_SetItemString(d, "BAD_MIN_GT_MAX", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_NO_BUFFERSPACE
	x = PyInt_FromLong(CL_BAD_NO_BUFFERSPACE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_NO_BUFFERSPACE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_NUMBER_OF_BLOCKS
	x = PyInt_FromLong(CL_BAD_NUMBER_OF_BLOCKS);
	if (x == NULL || PyDict_SetItemString(d, "BAD_NUMBER_OF_BLOCKS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_PARAM
	x = PyInt_FromLong(CL_BAD_PARAM);
	if (x == NULL || PyDict_SetItemString(d, "BAD_PARAM", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_PARAM_ID_POINTER
	x = PyInt_FromLong(CL_BAD_PARAM_ID_POINTER);
	if (x == NULL || PyDict_SetItemString(d, "BAD_PARAM_ID_POINTER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_PARAM_TYPE
	x = PyInt_FromLong(CL_BAD_PARAM_TYPE);
	if (x == NULL || PyDict_SetItemString(d, "BAD_PARAM_TYPE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_POINTER
	x = PyInt_FromLong(CL_BAD_POINTER);
	if (x == NULL || PyDict_SetItemString(d, "BAD_POINTER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_PVBUFFER
	x = PyInt_FromLong(CL_BAD_PVBUFFER);
	if (x == NULL || PyDict_SetItemString(d, "BAD_PVBUFFER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_SCHEME_POINTER
	x = PyInt_FromLong(CL_BAD_SCHEME_POINTER);
	if (x == NULL || PyDict_SetItemString(d, "BAD_SCHEME_POINTER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_STREAM_HEADER
	x = PyInt_FromLong(CL_BAD_STREAM_HEADER);
	if (x == NULL || PyDict_SetItemString(d, "BAD_STREAM_HEADER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_STRING_POINTER
	x = PyInt_FromLong(CL_BAD_STRING_POINTER);
	if (x == NULL || PyDict_SetItemString(d, "BAD_STRING_POINTER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BAD_TEXT_STRING_PTR
	x = PyInt_FromLong(CL_BAD_TEXT_STRING_PTR);
	if (x == NULL || PyDict_SetItemString(d, "BAD_TEXT_STRING_PTR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BEST_FIT
	x = PyInt_FromLong(CL_BEST_FIT);
	if (x == NULL || PyDict_SetItemString(d, "BEST_FIT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BIDIRECTIONAL
	x = PyInt_FromLong(CL_BIDIRECTIONAL);
	if (x == NULL || PyDict_SetItemString(d, "BIDIRECTIONAL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BITRATE
	x = PyInt_FromLong(CL_BITRATE);
	if (x == NULL || PyDict_SetItemString(d, "BITRATE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BITRATE_POLICY
	x = PyInt_FromLong(CL_BITRATE_POLICY);
	if (x == NULL || PyDict_SetItemString(d, "BITRATE_POLICY", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BITRATE_TARGET
	x = PyInt_FromLong(CL_BITRATE_TARGET);
	if (x == NULL || PyDict_SetItemString(d, "BITRATE_TARGET", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BITS_PER_COMPONENT
	x = PyInt_FromLong(CL_BITS_PER_COMPONENT);
	if (x == NULL || PyDict_SetItemString(d, "BITS_PER_COMPONENT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BLENDING
	x = PyInt_FromLong(CL_BLENDING);
	if (x == NULL || PyDict_SetItemString(d, "BLENDING", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BLOCK_SIZE
	x = PyInt_FromLong(CL_BLOCK_SIZE);
	if (x == NULL || PyDict_SetItemString(d, "BLOCK_SIZE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BOTTOM_UP
	x = PyInt_FromLong(CL_BOTTOM_UP);
	if (x == NULL || PyDict_SetItemString(d, "BOTTOM_UP", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BUFFER_NOT_CREATED
	x = PyInt_FromLong(CL_BUFFER_NOT_CREATED);
	if (x == NULL || PyDict_SetItemString(d, "BUFFER_NOT_CREATED", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BUF_COMPRESSED
	x = PyInt_FromLong(CL_BUF_COMPRESSED);
	if (x == NULL || PyDict_SetItemString(d, "BUF_COMPRESSED", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BUF_DATA
	x = PyInt_FromLong(CL_BUF_DATA);
	if (x == NULL || PyDict_SetItemString(d, "BUF_DATA", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_BUF_FRAME
	x = PyInt_FromLong(CL_BUF_FRAME);
	if (x == NULL || PyDict_SetItemString(d, "BUF_FRAME", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_CHANNEL_POLICY
	x = PyInt_FromLong(CL_CHANNEL_POLICY);
	if (x == NULL || PyDict_SetItemString(d, "CHANNEL_POLICY", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_CHROMA_THRESHOLD
	x = PyInt_FromLong(CL_CHROMA_THRESHOLD);
	if (x == NULL || PyDict_SetItemString(d, "CHROMA_THRESHOLD", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_CODEC
	x = PyInt_FromLong(CL_CODEC);
	if (x == NULL || PyDict_SetItemString(d, "CODEC", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_COMPONENTS
	x = PyInt_FromLong(CL_COMPONENTS);
	if (x == NULL || PyDict_SetItemString(d, "COMPONENTS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_COMPRESSED_BUFFER_SIZE
	x = PyInt_FromLong(CL_COMPRESSED_BUFFER_SIZE);
	if (x == NULL || PyDict_SetItemString(d, "COMPRESSED_BUFFER_SIZE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_COMPRESSION_RATIO
	x = PyInt_FromLong(CL_COMPRESSION_RATIO);
	if (x == NULL || PyDict_SetItemString(d, "COMPRESSION_RATIO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_COMPRESSOR
	x = PyInt_FromLong(CL_COMPRESSOR);
	if (x == NULL || PyDict_SetItemString(d, "COMPRESSOR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_CONTINUOUS_BLOCK
	x = PyInt_FromLong(CL_CONTINUOUS_BLOCK);
	if (x == NULL || PyDict_SetItemString(d, "CONTINUOUS_BLOCK", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_CONTINUOUS_NONBLOCK
	x = PyInt_FromLong(CL_CONTINUOUS_NONBLOCK);
	if (x == NULL || PyDict_SetItemString(d, "CONTINUOUS_NONBLOCK", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_COSMO_CODEC_CONTROL
	x = PyInt_FromLong(CL_COSMO_CODEC_CONTROL);
	if (x == NULL || PyDict_SetItemString(d, "COSMO_CODEC_CONTROL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_COSMO_NUM_PARAMS
	x = PyInt_FromLong(CL_COSMO_NUM_PARAMS);
	if (x == NULL || PyDict_SetItemString(d, "COSMO_NUM_PARAMS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_COSMO_VALUE_BASE
	x = PyInt_FromLong(CL_COSMO_VALUE_BASE);
	if (x == NULL || PyDict_SetItemString(d, "COSMO_VALUE_BASE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_COSMO_VIDEO_MANUAL_CONTROL
	x = PyInt_FromLong(CL_COSMO_VIDEO_MANUAL_CONTROL);
	if (x == NULL || PyDict_SetItemString(d, "COSMO_VIDEO_MANUAL_CONTROL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_COSMO_VIDEO_TRANSFER_MODE
	x = PyInt_FromLong(CL_COSMO_VIDEO_TRANSFER_MODE);
	if (x == NULL || PyDict_SetItemString(d, "COSMO_VIDEO_TRANSFER_MODE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_DATA
	x = PyInt_FromLong(CL_DATA);
	if (x == NULL || PyDict_SetItemString(d, "DATA", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_DECOMPRESSOR
	x = PyInt_FromLong(CL_DECOMPRESSOR);
	if (x == NULL || PyDict_SetItemString(d, "DECOMPRESSOR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_DSO_ERROR
	x = PyInt_FromLong(CL_DSO_ERROR);
	if (x == NULL || PyDict_SetItemString(d, "DSO_ERROR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_EDGE_THRESHOLD
	x = PyInt_FromLong(CL_EDGE_THRESHOLD);
	if (x == NULL || PyDict_SetItemString(d, "EDGE_THRESHOLD", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_ENABLE_IMAGEINFO
	x = PyInt_FromLong(CL_ENABLE_IMAGEINFO);
	if (x == NULL || PyDict_SetItemString(d, "ENABLE_IMAGEINFO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_END_OF_SEQUENCE
	x = PyInt_FromLong(CL_END_OF_SEQUENCE);
	if (x == NULL || PyDict_SetItemString(d, "END_OF_SEQUENCE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_ENUM_VALUE
	x = PyInt_FromLong(CL_ENUM_VALUE);
	if (x == NULL || PyDict_SetItemString(d, "ENUM_VALUE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_EXACT_COMPRESSION_RATIO
	x = PyInt_FromLong(CL_EXACT_COMPRESSION_RATIO);
	if (x == NULL || PyDict_SetItemString(d, "EXACT_COMPRESSION_RATIO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_EXTERNAL_DEVICE
	x = PyInt_FromLong((long) CL_EXTERNAL_DEVICE);
	if (x == NULL || PyDict_SetItemString(d, "EXTERNAL_DEVICE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FLOATING_ENUM_VALUE
	x = PyInt_FromLong(CL_FLOATING_ENUM_VALUE);
	if (x == NULL || PyDict_SetItemString(d, "FLOATING_ENUM_VALUE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FLOATING_RANGE_VALUE
	x = PyInt_FromLong(CL_FLOATING_RANGE_VALUE);
	if (x == NULL || PyDict_SetItemString(d, "FLOATING_RANGE_VALUE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FORMAT
	x = PyInt_FromLong(CL_FORMAT);
	if (x == NULL || PyDict_SetItemString(d, "FORMAT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FORMAT_ABGR
	x = PyInt_FromLong(CL_FORMAT_ABGR);
	if (x == NULL || PyDict_SetItemString(d, "FORMAT_ABGR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FORMAT_BGR
	x = PyInt_FromLong(CL_FORMAT_BGR);
	if (x == NULL || PyDict_SetItemString(d, "FORMAT_BGR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FORMAT_BGR233
	x = PyInt_FromLong(CL_FORMAT_BGR233);
	if (x == NULL || PyDict_SetItemString(d, "FORMAT_BGR233", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FORMAT_GRAYSCALE
	x = PyInt_FromLong(CL_FORMAT_GRAYSCALE);
	if (x == NULL || PyDict_SetItemString(d, "FORMAT_GRAYSCALE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FORMAT_MONO
	x = PyInt_FromLong(CL_FORMAT_MONO);
	if (x == NULL || PyDict_SetItemString(d, "FORMAT_MONO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FORMAT_RBG323
	x = PyInt_FromLong(CL_FORMAT_RBG323);
	if (x == NULL || PyDict_SetItemString(d, "FORMAT_RBG323", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FORMAT_STEREO_INTERLEAVED
	x = PyInt_FromLong(CL_FORMAT_STEREO_INTERLEAVED);
	if (x == NULL || PyDict_SetItemString(d, "FORMAT_STEREO_INTERLEAVED", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FORMAT_XBGR
	x = PyInt_FromLong(CL_FORMAT_XBGR);
	if (x == NULL || PyDict_SetItemString(d, "FORMAT_XBGR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FORMAT_YCbCr
	x = PyInt_FromLong(CL_FORMAT_YCbCr);
	if (x == NULL || PyDict_SetItemString(d, "FORMAT_YCbCr", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FORMAT_YCbCr422
	x = PyInt_FromLong(CL_FORMAT_YCbCr422);
	if (x == NULL || PyDict_SetItemString(d, "FORMAT_YCbCr422", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FORMAT_YCbCr422DC
	x = PyInt_FromLong(CL_FORMAT_YCbCr422DC);
	if (x == NULL || PyDict_SetItemString(d, "FORMAT_YCbCr422DC", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FRAME
	x = PyInt_FromLong(CL_FRAME);
	if (x == NULL || PyDict_SetItemString(d, "FRAME", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FRAMES_PER_CHUNK
	x = PyInt_FromLong(CL_FRAMES_PER_CHUNK);
	if (x == NULL || PyDict_SetItemString(d, "FRAMES_PER_CHUNK", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FRAME_BUFFER_SIZE
	x = PyInt_FromLong(CL_FRAME_BUFFER_SIZE);
	if (x == NULL || PyDict_SetItemString(d, "FRAME_BUFFER_SIZE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FRAME_BUFFER_SIZE_ZERO
	x = PyInt_FromLong(CL_FRAME_BUFFER_SIZE_ZERO);
	if (x == NULL || PyDict_SetItemString(d, "FRAME_BUFFER_SIZE_ZERO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FRAME_INDEX
	x = PyInt_FromLong(CL_FRAME_INDEX);
	if (x == NULL || PyDict_SetItemString(d, "FRAME_INDEX", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FRAME_RATE
	x = PyInt_FromLong(CL_FRAME_RATE);
	if (x == NULL || PyDict_SetItemString(d, "FRAME_RATE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FRAME_SIZE
	x = PyInt_FromLong(CL_FRAME_SIZE);
	if (x == NULL || PyDict_SetItemString(d, "FRAME_SIZE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_FRAME_TYPE
	x = PyInt_FromLong(CL_FRAME_TYPE);
	if (x == NULL || PyDict_SetItemString(d, "FRAME_TYPE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_G711_ALAW
	x = PyInt_FromLong(CL_G711_ALAW);
	if (x == NULL || PyDict_SetItemString(d, "G711_ALAW", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_G711_ALAW_SOFTWARE
	x = PyInt_FromLong(CL_G711_ALAW_SOFTWARE);
	if (x == NULL || PyDict_SetItemString(d, "G711_ALAW_SOFTWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_G711_ULAW
	x = PyInt_FromLong(CL_G711_ULAW);
	if (x == NULL || PyDict_SetItemString(d, "G711_ULAW", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_G711_ULAW_SOFTWARE
	x = PyInt_FromLong(CL_G711_ULAW_SOFTWARE);
	if (x == NULL || PyDict_SetItemString(d, "G711_ULAW_SOFTWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_GRAYSCALE
	x = PyInt_FromLong(CL_GRAYSCALE);
	if (x == NULL || PyDict_SetItemString(d, "GRAYSCALE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_HDCC
	x = PyInt_FromLong(CL_HDCC);
	if (x == NULL || PyDict_SetItemString(d, "HDCC", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_HDCC_SAMPLES_PER_TILE
	x = PyInt_FromLong(CL_HDCC_SAMPLES_PER_TILE);
	if (x == NULL || PyDict_SetItemString(d, "HDCC_SAMPLES_PER_TILE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_HDCC_SOFTWARE
	x = PyInt_FromLong(CL_HDCC_SOFTWARE);
	if (x == NULL || PyDict_SetItemString(d, "HDCC_SOFTWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_HDCC_TILE_THRESHOLD
	x = PyInt_FromLong(CL_HDCC_TILE_THRESHOLD);
	if (x == NULL || PyDict_SetItemString(d, "HDCC_TILE_THRESHOLD", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_HEADER_START_CODE
	x = PyInt_FromLong(CL_HEADER_START_CODE);
	if (x == NULL || PyDict_SetItemString(d, "HEADER_START_CODE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_IMAGEINFO_FIELDMASK
	x = PyInt_FromLong(CL_IMAGEINFO_FIELDMASK);
	if (x == NULL || PyDict_SetItemString(d, "IMAGEINFO_FIELDMASK", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_IMAGE_CROP_BOTTOM
	x = PyInt_FromLong(CL_IMAGE_CROP_BOTTOM);
	if (x == NULL || PyDict_SetItemString(d, "IMAGE_CROP_BOTTOM", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_IMAGE_CROP_LEFT
	x = PyInt_FromLong(CL_IMAGE_CROP_LEFT);
	if (x == NULL || PyDict_SetItemString(d, "IMAGE_CROP_LEFT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_IMAGE_CROP_RIGHT
	x = PyInt_FromLong(CL_IMAGE_CROP_RIGHT);
	if (x == NULL || PyDict_SetItemString(d, "IMAGE_CROP_RIGHT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_IMAGE_CROP_TOP
	x = PyInt_FromLong(CL_IMAGE_CROP_TOP);
	if (x == NULL || PyDict_SetItemString(d, "IMAGE_CROP_TOP", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_IMAGE_HEIGHT
	x = PyInt_FromLong(CL_IMAGE_HEIGHT);
	if (x == NULL || PyDict_SetItemString(d, "IMAGE_HEIGHT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_IMAGE_WIDTH
	x = PyInt_FromLong(CL_IMAGE_WIDTH);
	if (x == NULL || PyDict_SetItemString(d, "IMAGE_WIDTH", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_IMPACT_CODEC_CONTROL
	x = PyInt_FromLong(CL_IMPACT_CODEC_CONTROL);
	if (x == NULL || PyDict_SetItemString(d, "IMPACT_CODEC_CONTROL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_IMPACT_FRAME_INTERLEAVE
	x = PyInt_FromLong(CL_IMPACT_FRAME_INTERLEAVE);
	if (x == NULL || PyDict_SetItemString(d, "IMPACT_FRAME_INTERLEAVE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_IMPACT_NUM_PARAMS
	x = PyInt_FromLong(CL_IMPACT_NUM_PARAMS);
	if (x == NULL || PyDict_SetItemString(d, "IMPACT_NUM_PARAMS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_INTERNAL_FORMAT
	x = PyInt_FromLong(CL_INTERNAL_FORMAT);
	if (x == NULL || PyDict_SetItemString(d, "INTERNAL_FORMAT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_INTERNAL_IMAGE_HEIGHT
	x = PyInt_FromLong(CL_INTERNAL_IMAGE_HEIGHT);
	if (x == NULL || PyDict_SetItemString(d, "INTERNAL_IMAGE_HEIGHT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_INTERNAL_IMAGE_WIDTH
	x = PyInt_FromLong(CL_INTERNAL_IMAGE_WIDTH);
	if (x == NULL || PyDict_SetItemString(d, "INTERNAL_IMAGE_WIDTH", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_INTRA
	x = PyInt_FromLong(CL_INTRA);
	if (x == NULL || PyDict_SetItemString(d, "INTRA", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_JPEG
	x = PyInt_FromLong(CL_JPEG);
	if (x == NULL || PyDict_SetItemString(d, "JPEG", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_JPEG_COSMO
	x = PyInt_FromLong(CL_JPEG_COSMO);
	if (x == NULL || PyDict_SetItemString(d, "JPEG_COSMO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_JPEG_ERROR
	x = PyInt_FromLong(CL_JPEG_ERROR);
	if (x == NULL || PyDict_SetItemString(d, "JPEG_ERROR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_JPEG_IMPACT
	x = PyInt_FromLong(CL_JPEG_IMPACT);
	if (x == NULL || PyDict_SetItemString(d, "JPEG_IMPACT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_JPEG_NUM_PARAMS
	x = PyInt_FromLong(CL_JPEG_NUM_PARAMS);
	if (x == NULL || PyDict_SetItemString(d, "JPEG_NUM_PARAMS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_JPEG_QUALITY_FACTOR
	x = PyInt_FromLong(CL_JPEG_QUALITY_FACTOR);
	if (x == NULL || PyDict_SetItemString(d, "JPEG_QUALITY_FACTOR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_JPEG_QUANTIZATION_TABLES
	x = PyInt_FromLong(CL_JPEG_QUANTIZATION_TABLES);
	if (x == NULL || PyDict_SetItemString(d, "JPEG_QUANTIZATION_TABLES", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_JPEG_SOFTWARE
	x = PyInt_FromLong(CL_JPEG_SOFTWARE);
	if (x == NULL || PyDict_SetItemString(d, "JPEG_SOFTWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_JPEG_STREAM_HEADERS
	x = PyInt_FromLong(CL_JPEG_STREAM_HEADERS);
	if (x == NULL || PyDict_SetItemString(d, "JPEG_STREAM_HEADERS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_KEYFRAME
	x = PyInt_FromLong(CL_KEYFRAME);
	if (x == NULL || PyDict_SetItemString(d, "KEYFRAME", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_KEYFRAME_DISTANCE
	x = PyInt_FromLong(CL_KEYFRAME_DISTANCE);
	if (x == NULL || PyDict_SetItemString(d, "KEYFRAME_DISTANCE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_LAST_FRAME_INDEX
	x = PyInt_FromLong(CL_LAST_FRAME_INDEX);
	if (x == NULL || PyDict_SetItemString(d, "LAST_FRAME_INDEX", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_LAYER
	x = PyInt_FromLong(CL_LAYER);
	if (x == NULL || PyDict_SetItemString(d, "LAYER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_LUMA_THRESHOLD
	x = PyInt_FromLong(CL_LUMA_THRESHOLD);
	if (x == NULL || PyDict_SetItemString(d, "LUMA_THRESHOLD", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MAX_NUMBER_OF_AUDIO_ALGORITHMS
	x = PyInt_FromLong(CL_MAX_NUMBER_OF_AUDIO_ALGORITHMS);
	if (x == NULL || PyDict_SetItemString(d, "MAX_NUMBER_OF_AUDIO_ALGORITHMS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MAX_NUMBER_OF_FORMATS
	x = PyInt_FromLong(CL_MAX_NUMBER_OF_FORMATS);
	if (x == NULL || PyDict_SetItemString(d, "MAX_NUMBER_OF_FORMATS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MAX_NUMBER_OF_ORIGINAL_FORMATS
	x = PyInt_FromLong(CL_MAX_NUMBER_OF_ORIGINAL_FORMATS);
	if (x == NULL || PyDict_SetItemString(d, "MAX_NUMBER_OF_ORIGINAL_FORMATS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MAX_NUMBER_OF_PARAMS
	x = PyInt_FromLong(CL_MAX_NUMBER_OF_PARAMS);
	if (x == NULL || PyDict_SetItemString(d, "MAX_NUMBER_OF_PARAMS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MAX_NUMBER_OF_VIDEO_ALGORITHMS
	x = PyInt_FromLong(CL_MAX_NUMBER_OF_VIDEO_ALGORITHMS);
	if (x == NULL || PyDict_SetItemString(d, "MAX_NUMBER_OF_VIDEO_ALGORITHMS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MONO
	x = PyInt_FromLong(CL_MONO);
	if (x == NULL || PyDict_SetItemString(d, "MONO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_AUDIO_AWARE
	x = PyInt_FromLong(CL_MPEG1_AUDIO_AWARE);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_AUDIO_AWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_AUDIO_LAYER
	x = PyInt_FromLong(CL_MPEG1_AUDIO_LAYER);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_AUDIO_LAYER", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_AUDIO_LAYER_I
	x = PyInt_FromLong(CL_MPEG1_AUDIO_LAYER_I);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_AUDIO_LAYER_I", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_AUDIO_LAYER_II
	x = PyInt_FromLong(CL_MPEG1_AUDIO_LAYER_II);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_AUDIO_LAYER_II", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_AUDIO_MODE
	x = PyInt_FromLong(CL_MPEG1_AUDIO_MODE);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_AUDIO_MODE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_AUDIO_MODE_DUAL
	x = PyInt_FromLong(CL_MPEG1_AUDIO_MODE_DUAL);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_AUDIO_MODE_DUAL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_AUDIO_MODE_JOINT
	x = PyInt_FromLong(CL_MPEG1_AUDIO_MODE_JOINT);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_AUDIO_MODE_JOINT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_AUDIO_MODE_SINGLE
	x = PyInt_FromLong(CL_MPEG1_AUDIO_MODE_SINGLE);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_AUDIO_MODE_SINGLE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_AUDIO_MODE_STEREO
	x = PyInt_FromLong(CL_MPEG1_AUDIO_MODE_STEREO);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_AUDIO_MODE_STEREO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_AUDIO_SOFTWARE
	x = PyInt_FromLong(CL_MPEG1_AUDIO_SOFTWARE);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_AUDIO_SOFTWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_END_OF_STREAM
	x = PyInt_FromLong(CL_MPEG1_END_OF_STREAM);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_END_OF_STREAM", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_ERROR
	x = PyInt_FromLong(CL_MPEG1_ERROR);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_ERROR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_NUM_PARAMS
	x = PyInt_FromLong(CL_MPEG1_NUM_PARAMS);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_NUM_PARAMS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_VIDEO_M
	x = PyInt_FromLong(CL_MPEG1_VIDEO_M);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_VIDEO_M", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_VIDEO_MAX_MOTION_VECTOR_LENGTH_B_X
	x = PyInt_FromLong(CL_MPEG1_VIDEO_MAX_MOTION_VECTOR_LENGTH_B_X);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_VIDEO_MAX_MOTION_VECTOR_LENGTH_B_X", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_VIDEO_MAX_MOTION_VECTOR_LENGTH_B_Y
	x = PyInt_FromLong(CL_MPEG1_VIDEO_MAX_MOTION_VECTOR_LENGTH_B_Y);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_VIDEO_MAX_MOTION_VECTOR_LENGTH_B_Y", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_VIDEO_MAX_MOTION_VECTOR_LENGTH_P_X
	x = PyInt_FromLong(CL_MPEG1_VIDEO_MAX_MOTION_VECTOR_LENGTH_P_X);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_VIDEO_MAX_MOTION_VECTOR_LENGTH_P_X", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_VIDEO_MAX_MOTION_VECTOR_LENGTH_P_Y
	x = PyInt_FromLong(CL_MPEG1_VIDEO_MAX_MOTION_VECTOR_LENGTH_P_Y);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_VIDEO_MAX_MOTION_VECTOR_LENGTH_P_Y", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_VIDEO_N
	x = PyInt_FromLong(CL_MPEG1_VIDEO_N);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_VIDEO_N", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_VIDEO_SOFTNESS
	x = PyInt_FromLong(CL_MPEG1_VIDEO_SOFTNESS);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_VIDEO_SOFTNESS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_VIDEO_SOFTNESS_MAXIMUM
	x = PyInt_FromLong(CL_MPEG1_VIDEO_SOFTNESS_MAXIMUM);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_VIDEO_SOFTNESS_MAXIMUM", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_VIDEO_SOFTNESS_MEDIUM
	x = PyInt_FromLong(CL_MPEG1_VIDEO_SOFTNESS_MEDIUM);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_VIDEO_SOFTNESS_MEDIUM", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_VIDEO_SOFTNESS_NONE
	x = PyInt_FromLong(CL_MPEG1_VIDEO_SOFTNESS_NONE);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_VIDEO_SOFTNESS_NONE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG1_VIDEO_SOFTWARE
	x = PyInt_FromLong(CL_MPEG1_VIDEO_SOFTWARE);
	if (x == NULL || PyDict_SetItemString(d, "MPEG1_VIDEO_SOFTWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MPEG_VIDEO
	x = PyInt_FromLong(CL_MPEG_VIDEO);
	if (x == NULL || PyDict_SetItemString(d, "MPEG_VIDEO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MULTIRATE_AWARE
	x = PyInt_FromLong(CL_MULTIRATE_AWARE);
	if (x == NULL || PyDict_SetItemString(d, "MULTIRATE_AWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC1
	x = PyInt_FromLong(CL_MVC1);
	if (x == NULL || PyDict_SetItemString(d, "MVC1", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC1_SOFTWARE
	x = PyInt_FromLong(CL_MVC1_SOFTWARE);
	if (x == NULL || PyDict_SetItemString(d, "MVC1_SOFTWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC2
	x = PyInt_FromLong(CL_MVC2);
	if (x == NULL || PyDict_SetItemString(d, "MVC2", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC2_BLENDING
	x = PyInt_FromLong(CL_MVC2_BLENDING);
	if (x == NULL || PyDict_SetItemString(d, "MVC2_BLENDING", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC2_BLENDING_OFF
	x = PyInt_FromLong(CL_MVC2_BLENDING_OFF);
	if (x == NULL || PyDict_SetItemString(d, "MVC2_BLENDING_OFF", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC2_BLENDING_ON
	x = PyInt_FromLong(CL_MVC2_BLENDING_ON);
	if (x == NULL || PyDict_SetItemString(d, "MVC2_BLENDING_ON", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC2_CHROMA_THRESHOLD
	x = PyInt_FromLong(CL_MVC2_CHROMA_THRESHOLD);
	if (x == NULL || PyDict_SetItemString(d, "MVC2_CHROMA_THRESHOLD", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC2_EDGE_THRESHOLD
	x = PyInt_FromLong(CL_MVC2_EDGE_THRESHOLD);
	if (x == NULL || PyDict_SetItemString(d, "MVC2_EDGE_THRESHOLD", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC2_ERROR
	x = PyInt_FromLong(CL_MVC2_ERROR);
	if (x == NULL || PyDict_SetItemString(d, "MVC2_ERROR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC2_LUMA_THRESHOLD
	x = PyInt_FromLong(CL_MVC2_LUMA_THRESHOLD);
	if (x == NULL || PyDict_SetItemString(d, "MVC2_LUMA_THRESHOLD", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC2_SOFTWARE
	x = PyInt_FromLong(CL_MVC2_SOFTWARE);
	if (x == NULL || PyDict_SetItemString(d, "MVC2_SOFTWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC3_QUALITY_LEVEL
	x = PyInt_FromLong(CL_MVC3_QUALITY_LEVEL);
	if (x == NULL || PyDict_SetItemString(d, "MVC3_QUALITY_LEVEL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_MVC3_SOFTWARE
	x = PyInt_FromLong(CL_MVC3_SOFTWARE);
	if (x == NULL || PyDict_SetItemString(d, "MVC3_SOFTWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_NEXT_NOT_AVAILABLE
	x = PyInt_FromLong(CL_NEXT_NOT_AVAILABLE);
	if (x == NULL || PyDict_SetItemString(d, "NEXT_NOT_AVAILABLE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_NOISE_MARGIN
	x = PyInt_FromLong(CL_NOISE_MARGIN);
	if (x == NULL || PyDict_SetItemString(d, "NOISE_MARGIN", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_NONE
	x = PyInt_FromLong(CL_NONE);
	if (x == NULL || PyDict_SetItemString(d, "NONE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_NUMBER_OF_FORMATS
	x = PyInt_FromLong(CL_NUMBER_OF_FORMATS);
	if (x == NULL || PyDict_SetItemString(d, "NUMBER_OF_FORMATS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_NUMBER_OF_FRAMES
	x = PyInt_FromLong(CL_NUMBER_OF_FRAMES);
	if (x == NULL || PyDict_SetItemString(d, "NUMBER_OF_FRAMES", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_NUMBER_OF_PARAMS
	x = PyInt_FromLong(CL_NUMBER_OF_PARAMS);
	if (x == NULL || PyDict_SetItemString(d, "NUMBER_OF_PARAMS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_NUMBER_OF_PARAMS_FREEZE
	x = PyInt_FromLong(CL_NUMBER_OF_PARAMS_FREEZE);
	if (x == NULL || PyDict_SetItemString(d, "NUMBER_OF_PARAMS_FREEZE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_NUMBER_OF_VIDEO_FORMATS
	x = PyInt_FromLong(CL_NUMBER_OF_VIDEO_FORMATS);
	if (x == NULL || PyDict_SetItemString(d, "NUMBER_OF_VIDEO_FORMATS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_ORIENTATION
	x = PyInt_FromLong(CL_ORIENTATION);
	if (x == NULL || PyDict_SetItemString(d, "ORIENTATION", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_ORIGINAL_FORMAT
	x = PyInt_FromLong(CL_ORIGINAL_FORMAT);
	if (x == NULL || PyDict_SetItemString(d, "ORIGINAL_FORMAT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_PARAM_OUT_OF_RANGE
	x = PyInt_FromLong(CL_PARAM_OUT_OF_RANGE);
	if (x == NULL || PyDict_SetItemString(d, "PARAM_OUT_OF_RANGE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_PIXEL_ASPECT
	x = PyInt_FromLong(CL_PIXEL_ASPECT);
	if (x == NULL || PyDict_SetItemString(d, "PIXEL_ASPECT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_PREDICTED
	x = PyInt_FromLong(CL_PREDICTED);
	if (x == NULL || PyDict_SetItemString(d, "PREDICTED", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_PREROLL
	x = PyInt_FromLong(CL_PREROLL);
	if (x == NULL || PyDict_SetItemString(d, "PREROLL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_QUALITY_FACTOR
	x = PyInt_FromLong(CL_QUALITY_FACTOR);
	if (x == NULL || PyDict_SetItemString(d, "QUALITY_FACTOR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_QUALITY_LEVEL
	x = PyInt_FromLong(CL_QUALITY_LEVEL);
	if (x == NULL || PyDict_SetItemString(d, "QUALITY_LEVEL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_QUALITY_SPATIAL
	x = PyInt_FromLong(CL_QUALITY_SPATIAL);
	if (x == NULL || PyDict_SetItemString(d, "QUALITY_SPATIAL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_QUALITY_TEMPORAL
	x = PyInt_FromLong(CL_QUALITY_TEMPORAL);
	if (x == NULL || PyDict_SetItemString(d, "QUALITY_TEMPORAL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_QUANTIZATION_TABLES
	x = PyInt_FromLong(CL_QUANTIZATION_TABLES);
	if (x == NULL || PyDict_SetItemString(d, "QUANTIZATION_TABLES", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RANGE_VALUE
	x = PyInt_FromLong(CL_RANGE_VALUE);
	if (x == NULL || PyDict_SetItemString(d, "RANGE_VALUE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RGB
	x = PyInt_FromLong(CL_RGB);
	if (x == NULL || PyDict_SetItemString(d, "RGB", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RGB332
	x = PyInt_FromLong(CL_RGB332);
	if (x == NULL || PyDict_SetItemString(d, "RGB332", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RGB8
	x = PyInt_FromLong(CL_RGB8);
	if (x == NULL || PyDict_SetItemString(d, "RGB8", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RGBA
	x = PyInt_FromLong(CL_RGBA);
	if (x == NULL || PyDict_SetItemString(d, "RGBA", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RGBX
	x = PyInt_FromLong(CL_RGBX);
	if (x == NULL || PyDict_SetItemString(d, "RGBX", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RLE
	x = PyInt_FromLong(CL_RLE);
	if (x == NULL || PyDict_SetItemString(d, "RLE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RLE24
	x = PyInt_FromLong(CL_RLE24);
	if (x == NULL || PyDict_SetItemString(d, "RLE24", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RLE24_SOFTWARE
	x = PyInt_FromLong(CL_RLE24_SOFTWARE);
	if (x == NULL || PyDict_SetItemString(d, "RLE24_SOFTWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RLE_SOFTWARE
	x = PyInt_FromLong(CL_RLE_SOFTWARE);
	if (x == NULL || PyDict_SetItemString(d, "RLE_SOFTWARE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RTR
	x = PyInt_FromLong(CL_RTR);
	if (x == NULL || PyDict_SetItemString(d, "RTR", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RTR1
	x = PyInt_FromLong(CL_RTR1);
	if (x == NULL || PyDict_SetItemString(d, "RTR1", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_RTR_QUALITY_LEVEL
	x = PyInt_FromLong(CL_RTR_QUALITY_LEVEL);
	if (x == NULL || PyDict_SetItemString(d, "RTR_QUALITY_LEVEL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_SAMPLES_PER_TILE
	x = PyInt_FromLong(CL_SAMPLES_PER_TILE);
	if (x == NULL || PyDict_SetItemString(d, "SAMPLES_PER_TILE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_SCHEME_BUSY
	x = PyInt_FromLong(CL_SCHEME_BUSY);
	if (x == NULL || PyDict_SetItemString(d, "SCHEME_BUSY", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_SCHEME_NOT_AVAILABLE
	x = PyInt_FromLong(CL_SCHEME_NOT_AVAILABLE);
	if (x == NULL || PyDict_SetItemString(d, "SCHEME_NOT_AVAILABLE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_SPEED
	x = PyInt_FromLong(CL_SPEED);
	if (x == NULL || PyDict_SetItemString(d, "SPEED", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_STEREO_INTERLEAVED
	x = PyInt_FromLong(CL_STEREO_INTERLEAVED);
	if (x == NULL || PyDict_SetItemString(d, "STEREO_INTERLEAVED", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_STREAM_HEADERS
	x = PyInt_FromLong(CL_STREAM_HEADERS);
	if (x == NULL || PyDict_SetItemString(d, "STREAM_HEADERS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_TILE_THRESHOLD
	x = PyInt_FromLong(CL_TILE_THRESHOLD);
	if (x == NULL || PyDict_SetItemString(d, "TILE_THRESHOLD", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_TOP_DOWN
	x = PyInt_FromLong(CL_TOP_DOWN);
	if (x == NULL || PyDict_SetItemString(d, "TOP_DOWN", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_ULAW
	x = PyInt_FromLong(CL_ULAW);
	if (x == NULL || PyDict_SetItemString(d, "ULAW", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_UNCOMPRESSED
	x = PyInt_FromLong(CL_UNCOMPRESSED);
	if (x == NULL || PyDict_SetItemString(d, "UNCOMPRESSED", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_UNCOMPRESSED_AUDIO
	x = PyInt_FromLong(CL_UNCOMPRESSED_AUDIO);
	if (x == NULL || PyDict_SetItemString(d, "UNCOMPRESSED_AUDIO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_UNCOMPRESSED_VIDEO
	x = PyInt_FromLong(CL_UNCOMPRESSED_VIDEO);
	if (x == NULL || PyDict_SetItemString(d, "UNCOMPRESSED_VIDEO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_UNKNOWN_SCHEME
	x = PyInt_FromLong(CL_UNKNOWN_SCHEME);
	if (x == NULL || PyDict_SetItemString(d, "UNKNOWN_SCHEME", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_VIDEO
	x = PyInt_FromLong(CL_VIDEO);
	if (x == NULL || PyDict_SetItemString(d, "VIDEO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_Y
	x = PyInt_FromLong(CL_Y);
	if (x == NULL || PyDict_SetItemString(d, "Y", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_YCbCr
	x = PyInt_FromLong(CL_YCbCr);
	if (x == NULL || PyDict_SetItemString(d, "YCbCr", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_YCbCr422
	x = PyInt_FromLong(CL_YCbCr422);
	if (x == NULL || PyDict_SetItemString(d, "YCbCr422", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_YCbCr422DC
	x = PyInt_FromLong(CL_YCbCr422DC);
	if (x == NULL || PyDict_SetItemString(d, "YCbCr422DC", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_YCbCr422HC
	x = PyInt_FromLong(CL_YCbCr422HC);
	if (x == NULL || PyDict_SetItemString(d, "YCbCr422HC", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_YUV
	x = PyInt_FromLong(CL_YUV);
	if (x == NULL || PyDict_SetItemString(d, "YUV", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_YUV422
	x = PyInt_FromLong(CL_YUV422);
	if (x == NULL || PyDict_SetItemString(d, "YUV422", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_YUV422DC
	x = PyInt_FromLong(CL_YUV422DC);
	if (x == NULL || PyDict_SetItemString(d, "YUV422DC", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef CL_YUV422HC
	x = PyInt_FromLong(CL_YUV422HC);
	if (x == NULL || PyDict_SetItemString(d, "YUV422HC", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef AWCMP_STEREO
	x = PyInt_FromLong(AWCMP_STEREO);
	if (x == NULL || PyDict_SetItemString(d, "AWCMP_STEREO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef AWCMP_JOINT_STEREO
	x = PyInt_FromLong(AWCMP_JOINT_STEREO);
	if (x == NULL || PyDict_SetItemString(d, "AWCMP_JOINT_STEREO", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef AWCMP_INDEPENDENT
	x = PyInt_FromLong(AWCMP_INDEPENDENT);
	if (x == NULL || PyDict_SetItemString(d, "AWCMP_INDEPENDENT", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef AWCMP_FIXED_RATE
	x = PyInt_FromLong(AWCMP_FIXED_RATE);
	if (x == NULL || PyDict_SetItemString(d, "AWCMP_FIXED_RATE", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef AWCMP_CONST_QUAL
	x = PyInt_FromLong(AWCMP_CONST_QUAL);
	if (x == NULL || PyDict_SetItemString(d, "AWCMP_CONST_QUAL", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef AWCMP_LOSSLESS
	x = PyInt_FromLong(AWCMP_LOSSLESS);
	if (x == NULL || PyDict_SetItemString(d, "AWCMP_LOSSLESS", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef AWCMP_MPEG_LAYER_I
	x = PyInt_FromLong(AWCMP_MPEG_LAYER_I);
	if (x == NULL || PyDict_SetItemString(d, "AWCMP_MPEG_LAYER_I", x) < 0)
		return;
	Py_DECREF(x);
#endif
#ifdef AWCMP_MPEG_LAYER_II
	x = PyInt_FromLong(AWCMP_MPEG_LAYER_II);
	if (x == NULL || PyDict_SetItemString(d, "AWCMP_MPEG_LAYER_II", x) < 0)
		return;
	Py_DECREF(x);
#endif

	(void) clSetErrorHandler(cl_ErrorHandler);
}
