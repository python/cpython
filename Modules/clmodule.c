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

#define func(name, type)	\
		static object *cl_##name(object *self, object *args) \
		{ \
			int x; \
			if (!getargs(args, "i", &x)) return NULL; \
			return new##type##object(CL_##name(x)); \
		}

#define func2(name, type)	\
		static object *cl_##name(object *self, object *args) \
		{ \
			int a1, a2; \
			if (!getargs(args, "(ii)", &a1, &a2)) return NULL; \
			return new##type##object(CL_##name(a1, a2)); \
		}

func(BytesPerSample,int)
func(BytesPerPixel,int)
func(AudioFormatName,string)
func(VideoFormatName,string)
func(AlgorithmNumber,int)
func(AlgorithmType,int)
func2(Algorithm,int)
func(ParamNumber,int)
func(ParamType,int)
func2(ParamID,int)

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
	object *m, *d;

	m = initmodule("cl", cl_methods);
	d = getmoduledict(m);

	ClError = newstringobject("cl.error");
	(void) dictinsert(d, "error", ClError);

	(void) dictinsert(d, "MAX_NUMBER_OF_ORIGINAL_FORMATS",
			  newintobject(CL_MAX_NUMBER_OF_ORIGINAL_FORMATS));
	(void) dictinsert(d, "MONO", newintobject(CL_MONO));
	(void) dictinsert(d, "STEREO_INTERLEAVED",
			  newintobject(CL_STEREO_INTERLEAVED));
	(void) dictinsert(d, "RGB", newintobject(CL_RGB));
	(void) dictinsert(d, "RGBX", newintobject(CL_RGBX));
	(void) dictinsert(d, "RGBA", newintobject(CL_RGBA));
	(void) dictinsert(d, "RGB332", newintobject(CL_RGB332));
	(void) dictinsert(d, "GRAYSCALE", newintobject(CL_GRAYSCALE));
	(void) dictinsert(d, "Y", newintobject(CL_Y));
	(void) dictinsert(d, "YUV", newintobject(CL_YUV));
	(void) dictinsert(d, "YCbCr", newintobject(CL_YCbCr));
	(void) dictinsert(d, "YUV422", newintobject(CL_YUV422));
	(void) dictinsert(d, "YCbCr422", newintobject(CL_YCbCr422));
	(void) dictinsert(d, "YUV422HC", newintobject(CL_YUV422HC));
	(void) dictinsert(d, "YCbCr422HC", newintobject(CL_YCbCr422HC));
	(void) dictinsert(d, "YUV422DC", newintobject(CL_YUV422DC));
	(void) dictinsert(d, "YCbCr422DC", newintobject(CL_YCbCr422DC));
	(void) dictinsert(d, "RGB8", newintobject(CL_RGB8));
	(void) dictinsert(d, "BEST_FIT", newintobject(CL_BEST_FIT));
	(void) dictinsert(d, "MAX_NUMBER_OF_AUDIO_ALGORITHMS",
			  newintobject(CL_MAX_NUMBER_OF_AUDIO_ALGORITHMS));
	(void) dictinsert(d, "MAX_NUMBER_OF_VIDEO_ALGORITHMS",
			  newintobject(CL_MAX_NUMBER_OF_VIDEO_ALGORITHMS));
	(void) dictinsert(d, "AUDIO", newintobject(CL_AUDIO));
	(void) dictinsert(d, "VIDEO", newintobject(CL_VIDEO));
	(void) dictinsert(d, "UNKNOWN_SCHEME",
			  newintobject(CL_UNKNOWN_SCHEME));
	(void) dictinsert(d, "UNCOMPRESSED_AUDIO",
			  newintobject(CL_UNCOMPRESSED_AUDIO));
	(void) dictinsert(d, "G711_ULAW", newintobject(CL_G711_ULAW));
	(void) dictinsert(d, "ULAW", newintobject(CL_ULAW));
	(void) dictinsert(d, "G711_ALAW", newintobject(CL_G711_ALAW));
	(void) dictinsert(d, "ALAW", newintobject(CL_ALAW));
	(void) dictinsert(d, "AWARE_MPEG_AUDIO",
			  newintobject(CL_AWARE_MPEG_AUDIO));
	(void) dictinsert(d, "AWARE_MULTIRATE",
			  newintobject(CL_AWARE_MULTIRATE));
	(void) dictinsert(d, "UNCOMPRESSED", newintobject(CL_UNCOMPRESSED));
	(void) dictinsert(d, "UNCOMPRESSED_VIDEO",
			  newintobject(CL_UNCOMPRESSED_VIDEO));
	(void) dictinsert(d, "RLE", newintobject(CL_RLE));
	(void) dictinsert(d, "JPEG", newintobject(CL_JPEG));
#ifdef IRIX_5_3_LIBRARY
	(void) dictinsert(d, "JPEG_SOFTWARE", newintobject(CL_JPEG_SOFTWARE));
#endif
	(void) dictinsert(d, "MPEG_VIDEO", newintobject(CL_MPEG_VIDEO));
	(void) dictinsert(d, "MVC1", newintobject(CL_MVC1));
	(void) dictinsert(d, "RTR", newintobject(CL_RTR));
	(void) dictinsert(d, "RTR1", newintobject(CL_RTR1));
	(void) dictinsert(d, "HDCC", newintobject(CL_HDCC));
	(void) dictinsert(d, "MVC2", newintobject(CL_MVC2));
	(void) dictinsert(d, "RLE24", newintobject(CL_RLE24));
	(void) dictinsert(d, "MAX_NUMBER_OF_PARAMS",
			  newintobject(CL_MAX_NUMBER_OF_PARAMS));
	(void) dictinsert(d, "IMAGE_WIDTH", newintobject(CL_IMAGE_WIDTH));
	(void) dictinsert(d, "IMAGE_HEIGHT", newintobject(CL_IMAGE_HEIGHT));
	(void) dictinsert(d, "ORIGINAL_FORMAT",
			  newintobject(CL_ORIGINAL_FORMAT));
	(void) dictinsert(d, "INTERNAL_FORMAT",
			  newintobject(CL_INTERNAL_FORMAT));
	(void) dictinsert(d, "COMPONENTS", newintobject(CL_COMPONENTS));
	(void) dictinsert(d, "BITS_PER_COMPONENT",
			  newintobject(CL_BITS_PER_COMPONENT));
	(void) dictinsert(d, "FRAME_RATE", newintobject(CL_FRAME_RATE));
	(void) dictinsert(d, "COMPRESSION_RATIO",
			  newintobject(CL_COMPRESSION_RATIO));
	(void) dictinsert(d, "EXACT_COMPRESSION_RATIO",
			  newintobject(CL_EXACT_COMPRESSION_RATIO));
	(void) dictinsert(d, "FRAME_BUFFER_SIZE",
			  newintobject(CL_FRAME_BUFFER_SIZE));
	(void) dictinsert(d, "COMPRESSED_BUFFER_SIZE",
			  newintobject(CL_COMPRESSED_BUFFER_SIZE));
	(void) dictinsert(d, "BLOCK_SIZE", newintobject(CL_BLOCK_SIZE));
	(void) dictinsert(d, "PREROLL", newintobject(CL_PREROLL));
	(void) dictinsert(d, "FRAME_TYPE", newintobject(CL_FRAME_TYPE));
	(void) dictinsert(d, "ALGORITHM_ID", newintobject(CL_ALGORITHM_ID));
	(void) dictinsert(d, "ALGORITHM_VERSION",
			  newintobject(CL_ALGORITHM_VERSION));
	(void) dictinsert(d, "ORIENTATION", newintobject(CL_ORIENTATION));
	(void) dictinsert(d, "NUMBER_OF_FRAMES",
			  newintobject(CL_NUMBER_OF_FRAMES));
	(void) dictinsert(d, "SPEED", newintobject(CL_SPEED));
	(void) dictinsert(d, "LAST_FRAME_INDEX",
			  newintobject(CL_LAST_FRAME_INDEX));
#ifdef IRIX_5_3_LIBRARY
	(void) dictinsert(d, "ENABLE_IMAGEINFO",
			  newintobject(CL_ENABLE_IMAGEINFO));
	(void) dictinsert(d, "INTERNAL_IMAGE_WIDTH",
			  newintobject(CL_INTERNAL_IMAGE_WIDTH));
	(void) dictinsert(d, "INTERNAL_IMAGE_HEIGHT",
			  newintobject(CL_INTERNAL_IMAGE_HEIGHT));
#endif
	(void) dictinsert(d, "NUMBER_OF_PARAMS",
			  newintobject(CL_NUMBER_OF_PARAMS));
#ifdef IRIX_5_3_LIBRARY
	(void) dictinsert(d, "MVC2_LUMA_THRESHOLD",
			  newintobject(CL_MVC2_LUMA_THRESHOLD));
	(void) dictinsert(d, "MVC2_CHROMA_THRESHOLD",
			  newintobject(CL_MVC2_CHROMA_THRESHOLD));
	(void) dictinsert(d, "MVC2_EDGE_THRESHOLD",
			  newintobject(CL_MVC2_EDGE_THRESHOLD));
	(void) dictinsert(d, "MVC2_BLENDING", newintobject(CL_MVC2_BLENDING));
	(void) dictinsert(d, "MVC2_BLENDING_OFF",
			  newintobject(CL_MVC2_BLENDING_OFF));
	(void) dictinsert(d, "MVC2_BLENDING_ON",
			  newintobject(CL_MVC2_BLENDING_ON));
	(void) dictinsert(d, "JPEG_QUALITY_FACTOR",
			  newintobject(CL_JPEG_QUALITY_FACTOR));
	(void) dictinsert(d, "JPEG_STREAM_HEADERS",
			  newintobject(CL_JPEG_STREAM_HEADERS));
	(void) dictinsert(d, "JPEG_QUANTIZATION_TABLES",
			  newintobject(CL_JPEG_QUANTIZATION_TABLES));
	(void) dictinsert(d, "JPEG_NUM_PARAMS",
			  newintobject(CL_JPEG_NUM_PARAMS));
	(void) dictinsert(d, "RTR_QUALITY_LEVEL",
			  newintobject(CL_RTR_QUALITY_LEVEL));
	(void) dictinsert(d, "HDCC_TILE_THRESHOLD",
			  newintobject(CL_HDCC_TILE_THRESHOLD));
	(void) dictinsert(d, "HDCC_SAMPLES_PER_TILE",
			  newintobject(CL_HDCC_SAMPLES_PER_TILE));
#endif
	(void) dictinsert(d, "END_OF_SEQUENCE",
			  newintobject(CL_END_OF_SEQUENCE));
	(void) dictinsert(d, "CHANNEL_POLICY",
			  newintobject(CL_CHANNEL_POLICY));
	(void) dictinsert(d, "NOISE_MARGIN", newintobject(CL_NOISE_MARGIN));
	(void) dictinsert(d, "BITRATE_POLICY",
			  newintobject(CL_BITRATE_POLICY));
	(void) dictinsert(d, "BITRATE_TARGET",
			  newintobject(CL_BITRATE_TARGET));
	(void) dictinsert(d, "LAYER", newintobject(CL_LAYER));
	(void) dictinsert(d, "ENUM_VALUE", newintobject(CL_ENUM_VALUE));
	(void) dictinsert(d, "RANGE_VALUE", newintobject(CL_RANGE_VALUE));
	(void) dictinsert(d, "FLOATING_ENUM_VALUE",
			  newintobject(CL_FLOATING_ENUM_VALUE));
	(void) dictinsert(d, "FLOATING_RANGE_VALUE",
			  newintobject(CL_FLOATING_RANGE_VALUE));
	(void) dictinsert(d, "DECOMPRESSOR", newintobject(CL_DECOMPRESSOR));
	(void) dictinsert(d, "COMPRESSOR", newintobject(CL_COMPRESSOR));
	(void) dictinsert(d, "CODEC", newintobject(CL_CODEC));
	(void) dictinsert(d, "NONE", newintobject(CL_NONE));
#ifdef IRIX_5_3_LIBRARY
	(void) dictinsert(d, "BUF_FRAME", newintobject(CL_BUF_FRAME));
	(void) dictinsert(d, "BUF_DATA", newintobject(CL_BUF_DATA));
#endif
#ifdef CL_FRAME
	(void) dictinsert(d, "FRAME", newintobject(CL_FRAME));
	(void) dictinsert(d, "DATA", newintobject(CL_DATA));
#endif
	(void) dictinsert(d, "NONE", newintobject(CL_NONE));
	(void) dictinsert(d, "KEYFRAME", newintobject(CL_KEYFRAME));
	(void) dictinsert(d, "INTRA", newintobject(CL_INTRA));
	(void) dictinsert(d, "PREDICTED", newintobject(CL_PREDICTED));
	(void) dictinsert(d, "BIDIRECTIONAL", newintobject(CL_BIDIRECTIONAL));
	(void) dictinsert(d, "TOP_DOWN", newintobject(CL_TOP_DOWN));
	(void) dictinsert(d, "BOTTOM_UP", newintobject(CL_BOTTOM_UP));
#ifdef IRIX_5_3_LIBRARY
	(void) dictinsert(d, "CONTINUOUS_BLOCK",
			  newintobject(CL_CONTINUOUS_BLOCK));
	(void) dictinsert(d, "CONTINUOUS_NONBLOCK",
			  newintobject(CL_CONTINUOUS_NONBLOCK));
	(void) dictinsert(d, "EXTERNAL_DEVICE",
			  newintobject((long)CL_EXTERNAL_DEVICE));
#endif
	(void) dictinsert(d, "AWCMP_STEREO", newintobject(AWCMP_STEREO));
	(void) dictinsert(d, "AWCMP_JOINT_STEREO",
			  newintobject(AWCMP_JOINT_STEREO));
	(void) dictinsert(d, "AWCMP_INDEPENDENT",
			  newintobject(AWCMP_INDEPENDENT));
	(void) dictinsert(d, "AWCMP_FIXED_RATE",
			  newintobject(AWCMP_FIXED_RATE));
	(void) dictinsert(d, "AWCMP_CONST_QUAL",
			  newintobject(AWCMP_CONST_QUAL));
	(void) dictinsert(d, "AWCMP_LOSSLESS", newintobject(AWCMP_LOSSLESS));
	(void) dictinsert(d, "AWCMP_MPEG_LAYER_I",
			  newintobject(AWCMP_MPEG_LAYER_I));
	(void) dictinsert(d, "AWCMP_MPEG_LAYER_II",
			  newintobject(AWCMP_MPEG_LAYER_II));
	(void) dictinsert(d, "HEADER_START_CODE",
			  newintobject(CL_HEADER_START_CODE));
	(void) dictinsert(d, "BAD_NO_BUFFERSPACE",
			  newintobject(CL_BAD_NO_BUFFERSPACE));
	(void) dictinsert(d, "BAD_PVBUFFER", newintobject(CL_BAD_PVBUFFER));
	(void) dictinsert(d, "BAD_BUFFERLENGTH_NEG",
			  newintobject(CL_BAD_BUFFERLENGTH_NEG));
	(void) dictinsert(d, "BAD_BUFFERLENGTH_ODD",
			  newintobject(CL_BAD_BUFFERLENGTH_ODD));
	(void) dictinsert(d, "BAD_PARAM", newintobject(CL_BAD_PARAM));
	(void) dictinsert(d, "BAD_COMPRESSION_SCHEME",
			  newintobject(CL_BAD_COMPRESSION_SCHEME));
	(void) dictinsert(d, "BAD_COMPRESSOR_HANDLE",
			  newintobject(CL_BAD_COMPRESSOR_HANDLE));
	(void) dictinsert(d, "BAD_COMPRESSOR_HANDLE_POINTER",
			  newintobject(CL_BAD_COMPRESSOR_HANDLE_POINTER));
	(void) dictinsert(d, "BAD_BUFFER_HANDLE",
			  newintobject(CL_BAD_BUFFER_HANDLE));
	(void) dictinsert(d, "BAD_BUFFER_QUERY_SIZE",
			  newintobject(CL_BAD_BUFFER_QUERY_SIZE));
	(void) dictinsert(d, "JPEG_ERROR", newintobject(CL_JPEG_ERROR));
	(void) dictinsert(d, "BAD_FRAME_SIZE",
			  newintobject(CL_BAD_FRAME_SIZE));
	(void) dictinsert(d, "PARAM_OUT_OF_RANGE",
			  newintobject(CL_PARAM_OUT_OF_RANGE));
	(void) dictinsert(d, "ADDED_ALGORITHM_ERROR",
			  newintobject(CL_ADDED_ALGORITHM_ERROR));
	(void) dictinsert(d, "BAD_ALGORITHM_TYPE",
			  newintobject(CL_BAD_ALGORITHM_TYPE));
	(void) dictinsert(d, "BAD_ALGORITHM_NAME",
			  newintobject(CL_BAD_ALGORITHM_NAME));
	(void) dictinsert(d, "BAD_BUFFERING", newintobject(CL_BAD_BUFFERING));
	(void) dictinsert(d, "BUFFER_NOT_CREATED",
			  newintobject(CL_BUFFER_NOT_CREATED));
	(void) dictinsert(d, "BAD_BUFFER_EXISTS",
			  newintobject(CL_BAD_BUFFER_EXISTS));
	(void) dictinsert(d, "BAD_INTERNAL_FORMAT",
			  newintobject(CL_BAD_INTERNAL_FORMAT));
	(void) dictinsert(d, "BAD_BUFFER_POINTER",
			  newintobject(CL_BAD_BUFFER_POINTER));
	(void) dictinsert(d, "FRAME_BUFFER_SIZE_ZERO",
			  newintobject(CL_FRAME_BUFFER_SIZE_ZERO));
	(void) dictinsert(d, "BAD_STREAM_HEADER",
			  newintobject(CL_BAD_STREAM_HEADER));
	(void) dictinsert(d, "BAD_LICENSE", newintobject(CL_BAD_LICENSE));
	(void) dictinsert(d, "AWARE_ERROR", newintobject(CL_AWARE_ERROR));
	(void) dictinsert(d, "BAD_BUFFER_SIZE_POINTER",
			  newintobject(CL_BAD_BUFFER_SIZE_POINTER));
	(void) dictinsert(d, "BAD_BUFFER_SIZE",
			  newintobject(CL_BAD_BUFFER_SIZE));
	(void) dictinsert(d, "BAD_BUFFER_TYPE",
			  newintobject(CL_BAD_BUFFER_TYPE));
	(void) dictinsert(d, "BAD_HEADER_SIZE",
			  newintobject(CL_BAD_HEADER_SIZE));
	(void) dictinsert(d, "BAD_FUNCTION_POINTER",
			  newintobject(CL_BAD_FUNCTION_POINTER));
	(void) dictinsert(d, "BAD_SCHEME_POINTER",
			  newintobject(CL_BAD_SCHEME_POINTER));
	(void) dictinsert(d, "BAD_STRING_POINTER",
			  newintobject(CL_BAD_STRING_POINTER));
	(void) dictinsert(d, "BAD_MIN_GT_MAX",
			  newintobject(CL_BAD_MIN_GT_MAX));
	(void) dictinsert(d, "BAD_INITIAL_VALUE",
			  newintobject(CL_BAD_INITIAL_VALUE));
	(void) dictinsert(d, "BAD_PARAM_ID_POINTER",
			  newintobject(CL_BAD_PARAM_ID_POINTER));
	(void) dictinsert(d, "BAD_PARAM_TYPE",
			  newintobject(CL_BAD_PARAM_TYPE));
	(void) dictinsert(d, "BAD_TEXT_STRING_PTR",
			  newintobject(CL_BAD_TEXT_STRING_PTR));
	(void) dictinsert(d, "BAD_FUNCTIONALITY",
			  newintobject(CL_BAD_FUNCTIONALITY));
	(void) dictinsert(d, "BAD_NUMBER_OF_BLOCKS",
			  newintobject(CL_BAD_NUMBER_OF_BLOCKS));
	(void) dictinsert(d, "BAD_BLOCK_SIZE",
			  newintobject(CL_BAD_BLOCK_SIZE));
	(void) dictinsert(d, "BAD_POINTER", newintobject(CL_BAD_POINTER));
	(void) dictinsert(d, "BAD_BOARD", newintobject(CL_BAD_BOARD));
	(void) dictinsert(d, "MVC2_ERROR", newintobject(CL_MVC2_ERROR));
#ifdef IRIX_5_3_LIBRARY
	(void) dictinsert(d, "NEXT_NOT_AVAILABLE",
			  newintobject(CL_NEXT_NOT_AVAILABLE));
	(void) dictinsert(d, "SCHEME_BUSY", newintobject(CL_SCHEME_BUSY));
	(void) dictinsert(d, "SCHEME_NOT_AVAILABLE",
			  newintobject(CL_SCHEME_NOT_AVAILABLE));
#endif
#ifdef CL_LUMA_THRESHOLD
	/* backward compatibility */
	(void) dictinsert(d, "LUMA_THRESHOLD",
			  newintobject(CL_LUMA_THRESHOLD));
	(void) dictinsert(d, "CHROMA_THRESHOLD",
			  newintobject(CL_CHROMA_THRESHOLD));
	(void) dictinsert(d, "EDGE_THRESHOLD",
			  newintobject(CL_EDGE_THRESHOLD));
	(void) dictinsert(d, "BLENDING", newintobject(CL_BLENDING));
	(void) dictinsert(d, "QUALITY_FACTOR",
			  newintobject(CL_QUALITY_FACTOR));
	(void) dictinsert(d, "STREAM_HEADERS",
			  newintobject(CL_STREAM_HEADERS));
	(void) dictinsert(d, "QUALITY_LEVEL", newintobject(CL_QUALITY_LEVEL));
	(void) dictinsert(d, "TILE_THRESHOLD",
			  newintobject(CL_TILE_THRESHOLD));
	(void) dictinsert(d, "SAMPLES_PER_TILE",
			  newintobject(CL_SAMPLES_PER_TILE));
#endif

	if (err_occurred())
		fatal("can't initialize module cl");

	(void) clSetErrorHandler(cl_ErrorHandler);
}
