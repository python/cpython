/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/


/* Cl objects */

#define CLDEBUG

#include <stdarg.h>
#include <cl.h>
#include "allobjects.h"
#include "modsupport.h"		/* For getargs() etc. */
#include "ceval.h"		/* For call_object() */

typedef struct {
	OB_HEAD
	int ob_isCompressor;	/* Compressor or Decompressor */
	CL_Handle ob_compressorHdl;
	int *ob_paramtypes;
	int ob_nparams;
} clobject;

static object *ClError;		/* exception cl.error */

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

	if (err_occurred())	/* don't change existing error */
		return;
	error_handler_called = 1;
	va_start(ap, fmt);
	vsprintf(errbuf, fmt, ap);
	va_end(ap);
	p = &errbuf[strlen(errbuf) - 1]; /* swat the line feed */
	if (*p == '\n')
		*p = 0;
	err_setstr(ClError, errbuf);
}

/*
 * This assumes that params are always in the range 0 to some maximum.
 */
static int
param_type_is_float(clobject *self, int param)
{
	int bufferlength;
	int ret;

	if (self->ob_paramtypes == NULL) {
		error_handler_called = 0;
		bufferlength = clQueryParams(self->ob_compressorHdl, 0, 0);
		if (error_handler_called)
			return -1;

		self->ob_paramtypes = NEW(int, bufferlength);
		if (self->ob_paramtypes == NULL)
			return -1;
		self->ob_nparams = bufferlength / 2;

		(void) clQueryParams(self->ob_compressorHdl, self->ob_paramtypes, bufferlength);
		if (error_handler_called) {
			DEL(self->ob_paramtypes);
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
static object *
cl_CompressImage(object *self, object *args)
{
	int compressionScheme, width, height, originalFormat;
	float compressionRatio;
	int frameBufferSize, compressedBufferSize;
	char *frameBuffer;
	object *compressedBuffer;

	if (!getargs(args, "(iiiifs#)", &compressionScheme, &width, &height,
		     &originalFormat, &compressionRatio, &frameBuffer,
		     &frameBufferSize))
		return NULL;

 retry:
	compressedBuffer = newsizedstringobject(NULL, frameBufferSize);
	if (compressedBuffer == NULL)
		return NULL;

	compressedBufferSize = frameBufferSize;
	error_handler_called = 0;
	if (clCompressImage(compressionScheme, width, height, originalFormat,
			    compressionRatio, (void *) frameBuffer,
			    &compressedBufferSize,
			    (void *) getstringvalue(compressedBuffer))
	    == FAILURE) {
		DECREF(compressedBuffer);
		if (!error_handler_called)
			err_setstr(ClError, "clCompressImage failed");
		return NULL;
	}

	if (compressedBufferSize > frameBufferSize) {
		frameBufferSize = compressedBufferSize;
		DECREF(compressedBuffer);
		goto retry;
	}

	if (compressedBufferSize < frameBufferSize)
		if (resizestring(&compressedBuffer, compressedBufferSize))
			return NULL;

	return compressedBuffer;
}

static object *
cl_DecompressImage(object *self, object *args)
{
	int compressionScheme, width, height, originalFormat;
	char *compressedBuffer;
	int compressedBufferSize, frameBufferSize;
	object *frameBuffer;

	if (!getargs(args, "(iiiis#)", &compressionScheme, &width, &height,
		     &originalFormat, &compressedBuffer,
		     &compressedBufferSize))
		return NULL;

	frameBufferSize = width * height * CL_BytesPerPixel(originalFormat);

	frameBuffer = newsizedstringobject(NULL, frameBufferSize);
	if (frameBuffer == NULL)
		return NULL;

	error_handler_called = 0;
	if (clDecompressImage(compressionScheme, width, height, originalFormat,
			      compressedBufferSize, compressedBuffer,
			      (void *) getstringvalue(frameBuffer)) == FAILURE) {
		DECREF(frameBuffer);
		if (!error_handler_called)
			err_setstr(ClError, "clDecompressImage failed");
		return NULL;
	}

	return frameBuffer;
}

/********************************************************************
		Sequential compression/decompression.
********************************************************************/
#define CheckCompressor(self)	if ((self)->ob_compressorHdl == NULL) { \
					err_setstr(RuntimeError, "(de)compressor not active"); \
					return NULL; \
				}

static object *
doClose(clobject *self, object *args, int (*close_func)(CL_Handle))
{
	CheckCompressor(self);

	if (!getnoarg(args))
		return NULL;

	error_handler_called = 0;
	if ((*close_func)(self->ob_compressorHdl) == FAILURE) {
		if (!error_handler_called)
			err_setstr(ClError, "close failed");
		return NULL;
	}

	self->ob_compressorHdl = NULL;

	if (self->ob_paramtypes)
		DEL(self->ob_paramtypes);
	self->ob_paramtypes = NULL;

	INCREF(None);
	return None;
}

static object *
clm_CloseCompressor(object *self, object *args)
{
	return doClose(SELF, args, clCloseCompressor);
}

static object *
clm_CloseDecompressor(object *self, object *args)
{
	return doClose(SELF, args, clCloseDecompressor);
}

static object *
clm_Compress(object *self, object *args)
{
	int numberOfFrames;
	int frameBufferSize, compressedBufferSize, size;
	char *frameBuffer;
	object *data;

	CheckCompressor(SELF);

	if (!getargs(args, "(is#)", &numberOfFrames, &frameBuffer, &frameBufferSize))
		return NULL;

	error_handler_called = 0;
	size = clGetParam(SELF->ob_compressorHdl, CL_COMPRESSED_BUFFER_SIZE);
	compressedBufferSize = size;
	if (error_handler_called)
		return NULL;

	data = newsizedstringobject(NULL, size);
	if (data == NULL)
		return NULL;

	error_handler_called = 0;
	if (clCompress(SELF->ob_compressorHdl, numberOfFrames,
		       (void *) frameBuffer, &compressedBufferSize,
		       (void *) getstringvalue(data)) == FAILURE) {
		DECREF(data);
		if (!error_handler_called)
			err_setstr(ClError, "compress failed");
		return NULL;
	}

	if (compressedBufferSize < size)
		if (resizestring(&data, compressedBufferSize))
			return NULL;

	if (compressedBufferSize > size) {
		/* we didn't get all "compressed" data */
		DECREF(data);
		err_setstr(ClError, "compressed data is more than fitted");
		return NULL;
	}

	return data;
}

static object *
clm_Decompress(object *self, object *args)
{
	object *data;
	int numberOfFrames;
	char *compressedData;
	int compressedDataSize, dataSize;

	CheckCompressor(SELF);

	if (!getargs(args, "(is#)", &numberOfFrames, &compressedData,
		     &compressedDataSize))
		return NULL;

	error_handler_called = 0;
	dataSize = clGetParam(SELF->ob_compressorHdl, CL_FRAME_BUFFER_SIZE);
	if (error_handler_called)
		return NULL;

	data = newsizedstringobject(NULL, dataSize);
	if (data == NULL)
		return NULL;

	error_handler_called = 0;
	if (clDecompress(SELF->ob_compressorHdl, numberOfFrames,
			 compressedDataSize, (void *) compressedData,
			 (void *) getstringvalue(data)) == FAILURE) {
		DECREF(data);
		if (!error_handler_called)
			err_setstr(ClError, "decompress failed");
		return NULL;
	}

	return data;
}

static object *
doParams(clobject *self, object *args, int (*func)(CL_Handle, int *, int),
	 int modified)
{
	object *list, *v;
	int *PVbuffer;
	int length;
	int i;
	float number;
	
	CheckCompressor(self);

	if (!getargs(args, "O", &list))
		return NULL;
	if (!is_listobject(list)) {
		err_badarg();
		return NULL;
	}
	length = getlistsize(list);
	PVbuffer = NEW(int, length);
	if (PVbuffer == NULL)
		return err_nomem();
	for (i = 0; i < length; i++) {
		v = getlistitem(list, i);
		if (is_floatobject(v)) {
			number = getfloatvalue(v);
			PVbuffer[i] = CL_TypeIsInt(number);
		} else if (is_intobject(v)) {
			PVbuffer[i] = getintvalue(v);
			if ((i & 1) &&
			    param_type_is_float(self, PVbuffer[i-1]) > 0) {
				number = PVbuffer[i];
				PVbuffer[i] = CL_TypeIsInt(number);
			}
		} else {
			DEL(PVbuffer);
			err_badarg();
			return NULL;
		}
	}

	error_handler_called = 0;
	(*func)(self->ob_compressorHdl, PVbuffer, length);
	if (error_handler_called) {
		DEL(PVbuffer);
		return NULL;
	}

	if (modified) {
		for (i = 0; i < length; i++) {
			if ((i & 1) &&
			    param_type_is_float(self, PVbuffer[i-1]) > 0) {
				number = CL_TypeIsFloat(PVbuffer[i]);
				v = newfloatobject(number);
			} else
				v = newintobject(PVbuffer[i]);
			setlistitem(list, i, v);
		}
	}

	DEL(PVbuffer);

	INCREF(None);
	return None;
}

static object *
clm_GetParams(object *self, object *args)
{
	return doParams(SELF, args, clGetParams, 1);
}

static object *
clm_SetParams(object *self, object *args)
{
	return doParams(SELF, args, clSetParams, 0);
}

static object *
do_get(clobject *self, object *args, int (*func)(CL_Handle, int))
{
	int paramID, value;
	float fvalue;

	CheckCompressor(self);

	if (!getargs(args, "i", &paramID))
		return NULL;

	error_handler_called = 0;
	value = (*func)(self->ob_compressorHdl, paramID);
	if (error_handler_called)
		return NULL;

	if (param_type_is_float(self, paramID) > 0) {
		fvalue = CL_TypeIsFloat(value);
		return newfloatobject(fvalue);
	}

	return newintobject(value);
}

static object *
clm_GetParam(object *self, object *args)
{
	return do_get(SELF, args, clGetParam);
}

static object *
clm_GetDefault(object *self, object *args)
{
	return do_get(SELF, args, clGetDefault);
}

static object *
clm_SetParam(object *self, object *args)
{
	int paramID, value;
	float fvalue;

	CheckCompressor(SELF);

	if (!getargs(args, "(ii)", &paramID, &value)) {
		err_clear();
		if (!getargs(args, "(if)", &paramID, &fvalue)) {
			err_clear();
			err_setstr(TypeError, "bad argument list (format '(ii)' or '(if)')");
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
		return newfloatobject(CL_TypeIsFloat(value));
	else
		return newintobject(value);
}

static object *
clm_GetParamID(object *self, object *args)
{
	char *name;
	int value;

	CheckCompressor(SELF);

	if (!getargs(args, "s", &name))
		return NULL;

	error_handler_called = 0;
	value = clGetParamID(SELF->ob_compressorHdl, name);
	if (value == FAILURE) {
		if (!error_handler_called)
			err_setstr(ClError, "getparamid failed");
		return NULL;
	}

	return newintobject(value);
}

static object *
clm_QueryParams(object *self, object *args)
{
	int bufferlength;
	int *PVbuffer;
	object *list;
	int i;

	CheckCompressor(SELF);

	if (!getnoarg(args))
		return NULL;

	error_handler_called = 0;
	bufferlength = clQueryParams(SELF->ob_compressorHdl, 0, 0);
	if (error_handler_called)
		return NULL;

	PVbuffer = NEW(int, bufferlength);
	if (PVbuffer == NULL)
		return err_nomem();

	bufferlength = clQueryParams(SELF->ob_compressorHdl, PVbuffer,
				     bufferlength);
	if (error_handler_called) {
		DEL(PVbuffer);
		return NULL;
	}

	list = newlistobject(bufferlength);
	if (list == NULL) {
		DEL(PVbuffer);
		return NULL;
	}

	for (i = 0; i < bufferlength; i++) {
		if (i & 1)
			setlistitem(list, i, newintobject(PVbuffer[i]));
		else if (PVbuffer[i] == 0) {
			INCREF(None);
			setlistitem(list, i, None);
		} else
			setlistitem(list, i, newstringobject((char *) PVbuffer[i]));
	}

	DEL(PVbuffer);

	return list;
}

static object *
clm_GetMinMax(object *self, object *args)
{
	int param, min, max;
	float fmin, fmax;

	CheckCompressor(SELF);

	if (!getargs(args, "i", &param))
		return NULL;

	clGetMinMax(SELF->ob_compressorHdl, param, &min, &max);

	if (param_type_is_float(SELF, param) > 0) {
		fmin = CL_TypeIsFloat(min);
		fmax = CL_TypeIsFloat(max);
		return mkvalue("(ff)", fmin, fmax);
	}

	return mkvalue("(ii)", min, max);
}

static object *
clm_GetName(object *self, object *args)
{
	int param;
	char *name;

	CheckCompressor(SELF);

	if (!getargs(args, "i", &param))
		return NULL;

	error_handler_called = 0;
	name = clGetName(SELF->ob_compressorHdl, param);
	if (name == NULL || error_handler_called) {
		if (!error_handler_called)
			err_setstr(ClError, "getname failed");
		return NULL;
	}

	return newstringobject(name);
}

static object *
clm_QuerySchemeFromHandle(object *self, object *args)
{
	CheckCompressor(SELF);

	if (!getnoarg(args))
		return NULL;

	return newintobject(clQuerySchemeFromHandle(SELF->ob_compressorHdl));
}

static object *
clm_ReadHeader(object *self, object *args)
{
	char *header;
	int headerSize;

	CheckCompressor(SELF);

	if (!getargs(args, "s#", &header, &headerSize))
		return NULL;

	return newintobject(clReadHeader(SELF->ob_compressorHdl,
					 headerSize, header));
}

static struct methodlist compressor_methods[] = {
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

static struct methodlist decompressor_methods[] = {
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
cl_dealloc(object *self)
{
	if (SELF->ob_compressorHdl) {
		if (SELF->ob_isCompressor)
			clCloseCompressor(SELF->ob_compressorHdl);
		else
			clCloseDecompressor(SELF->ob_compressorHdl);
	}
	DEL(self);
}

static object *
cl_getattr(object *self, char *name)
{
	if (SELF->ob_isCompressor)
		return findmethod(compressor_methods, self, name);
	else
		return findmethod(decompressor_methods, self, name);
}

static typeobject Cltype = {
	OB_HEAD_INIT(&Typetype)
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

static object *
doOpen(object *self, object *args, int (*open_func)(int, CL_Handle *),
       int iscompressor)
{
	int scheme;
	clobject *new;

	if (!getargs(args, "i", &scheme))
		return NULL;

	new = NEWOBJ(clobject, &Cltype);
	if (new == NULL)
		return NULL;

	new->ob_compressorHdl = NULL;
	new->ob_isCompressor = iscompressor;
	new->ob_paramtypes = NULL;

	error_handler_called = 0;
	if ((*open_func)(scheme, &new->ob_compressorHdl) == FAILURE) {
		DECREF(new);
		if (!error_handler_called)
			err_setstr(ClError, "Open(De)Compressor failed");
		return NULL;
	}
	return (object *)new;
}

static object *
cl_OpenCompressor(object *self, object *args)
{
	return doOpen(self, args, clOpenCompressor, 1);
}

static object *
cl_OpenDecompressor(object *self, object *args)
{
	return doOpen(self, args, clOpenDecompressor, 0);
}

static object *
cl_QueryScheme(object *self, object *args)
{
	char *header;
	int headerlen;
	int scheme;

	if (!getargs(args, "s#", &header, &headerlen))
		return NULL;

	scheme = clQueryScheme(header);
	if (scheme < 0) {
		err_setstr(ClError, "unknown compression scheme");
		return NULL;
	}

	return newintobject(scheme);
}

static object *
cl_QueryMaxHeaderSize(object *self, object *args)
{
	int scheme;

	if (!getargs(args, "i", &scheme))
		return NULL;

	return newintobject(clQueryMaxHeaderSize(scheme));
}

static object *
cl_QueryAlgorithms(object *self, object *args)
{
	int algorithmMediaType;
	int bufferlength;
	int *PVbuffer;
	object *list;
	int i;

	if (!getargs(args, "i", &algorithmMediaType))
		return NULL;

	error_handler_called = 0;
	bufferlength = clQueryAlgorithms(algorithmMediaType, 0, 0);
	if (error_handler_called)
		return NULL;

	PVbuffer = NEW(int, bufferlength);
	if (PVbuffer == NULL)
		return err_nomem();

	bufferlength = clQueryAlgorithms(algorithmMediaType, PVbuffer,
				     bufferlength);
	if (error_handler_called) {
		DEL(PVbuffer);
		return NULL;
	}

	list = newlistobject(bufferlength);
	if (list == NULL) {
		DEL(PVbuffer);
		return NULL;
	}

	for (i = 0; i < bufferlength; i++) {
		if (i & 1)
			setlistitem(list, i, newintobject(PVbuffer[i]));
		else if (PVbuffer[i] == 0) {
			INCREF(None);
			setlistitem(list, i, None);
		} else
			setlistitem(list, i, newstringobject((char *) PVbuffer[i]));
	}

	DEL(PVbuffer);

	return list;
}

static object *
cl_QuerySchemeFromName(object *self, object *args)
{
	int algorithmMediaType;
	char *name;
	int scheme;

	if (!getargs(args, "(is)", &algorithmMediaType, &name))
		return NULL;

	error_handler_called = 0;
	scheme = clQuerySchemeFromName(algorithmMediaType, name);
	if (error_handler_called) {
		err_setstr(ClError, "unknown compression scheme");
		return NULL;
	}

	return newintobject(scheme);
}

static object *
cl_GetAlgorithmName(object *self, object *args)
{
	int scheme;
	char *name;

	if (!getargs(args, "i", &scheme))
		return NULL;

	name = clGetAlgorithmName(scheme);
	if (name == 0) {
		err_setstr(ClError, "unknown compression scheme");
		return NULL;
	}

	return newstringobject(name);
}

static object *
do_set(object *self, object *args, int (*func)(int, int, int))
{
	int scheme, paramID, value;
	float fvalue;
	int is_float = 0;

	if (!getargs(args, "(iii)", &scheme, &paramID, &value)) {
		err_clear();
		if (!getargs(args, "(iif)", &scheme, &paramID, &fvalue)) {
			err_clear();
			err_setstr(TypeError, "bad argument list (format '(iii)' or '(iif)')");
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
		return newfloatobject(CL_TypeIsFloat(value));
	else
		return newintobject(value);
}

static object *
cl_SetDefault(object *self, object *args)
{
	return do_set(self, args, clSetDefault);
}

static object *
cl_SetMin(object *self, object *args)
{
	return do_set(self, args, clSetMin);
}

static object *
cl_SetMax(object *self, object *args)
{
	return do_set(self, args, clSetMax);
}

#ifdef CLDEBUG
static object *
cvt_type(object *self, object *args)
{
	int number;
	float fnumber;

	if (getargs(args, "i", &number))
		return newfloatobject(CL_TypeIsFloat(number));
	else {
		err_clear();
		if (getargs(args, "f", &fnumber))
			return newintobject(CL_TypeIsInt(fnumber));
		return NULL;
	}
}
#endif

static struct methodlist cl_methods[] = {
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
#ifdef CLDEBUG
	{"cvt_type",		cvt_type},
#endif
	{NULL,			NULL} /* Sentinel */
};

void
initcl()
{
	object *m, *d;

	m = initmodule("cl", cl_methods);
	d = getmoduledict(m);

	ClError = newstringobject("cl.error");
	if (ClError == NULL || dictinsert(d, "error", ClError) != 0)
		fatal("can't define cl.error");

	(void) clSetErrorHandler(cl_ErrorHandler);
}
