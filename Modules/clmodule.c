/***********************************************************
Copyright 1991, 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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

#include <stdarg.h>
#include <cl.h>
#include "allobjects.h"
#include "modsupport.h"		/* For getargs() etc. */
#include "ceval.h"		/* For call_object() */

typedef struct {
	OB_HEAD
	int ob_isCompressor;	/* Compressor or Decompressor */
	CL_Handle ob_compressorHdl;
} clobject;

static object *ClError;		/* exception cl.error */

static int error_handler_called = 0;

/********************************************************************
			  Utility routines.
********************************************************************/
static void
cl_ErrorHandler(long errnum, const char *fmt, ...)
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
 * This is not very efficient.
 */
static int
param_type_is_float(CL_Handle comp, long param)
{
	long bufferlength;
	long *PVbuffer;
	int ret;

	error_handler_called = 0;
	bufferlength = clQueryParams(comp, 0, 0);
	if (error_handler_called)
		return -1;

	if (param < 0 || param >= bufferlength / 2)
		return -1;

	PVbuffer = NEW(long, bufferlength);
	if (PVbuffer == NULL)
		return -1;

	bufferlength = clQueryParams(comp, PVbuffer, bufferlength);
	if (error_handler_called) {
		DEL(PVbuffer);
		return -1;
	}

	if (PVbuffer[param*2 + 1] == CL_FLOATING_ENUM_VALUE ||
	    PVbuffer[param*2 + 1] == CL_FLOATING_RANGE_VALUE)
		ret = 1;
	else
		ret = 0;

	DEL(PVbuffer);

	return ret;
}

/********************************************************************
	       Single image compression/decompression.
********************************************************************/
static object *
cl_CompressImage(self, args)
	object *self, *args;
{
	long compressionScheme, width, height, originalFormat;
	float compressionRatio;
	long frameBufferSize, compressedBufferSize;
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
cl_DecompressImage(self, args)
	object *self, *args;
{
	long compressionScheme, width, height, originalFormat;
	char *compressedBuffer;
	long compressedBufferSize, frameBufferSize;
	object *frameBuffer;

	if (!getargs(args, "(iiiis#i)", &compressionScheme, &width, &height,
		     &originalFormat, &compressedBuffer, &compressedBufferSize,
		     &frameBufferSize))
		return NULL;

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
extern typeobject Cltype;	/* Really static, forward */

#define CheckCompressor(self)	if ((self)->ob_compressorHdl == NULL) { \
					err_setstr(RuntimeError, "(de)compressor not active"); \
					return NULL; \
				}

static object *
doClose(self, args, close_func)
	clobject *self;
	object *args;
	long (*close_func) PROTO((CL_Handle));
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

	INCREF(None);
	return None;
}

static object *
clm_CloseCompressor(self, args)
	clobject *self;
	object *args;
{
	return doClose(self, args, clCloseCompressor);
}

static object *
clm_CloseDecompressor(self, args)
	clobject *self;
	object *args;
{
	return doClose(self, args, clCloseDecompressor);
}

static object *
clm_Compress(self, args)
	clobject *self;
	object *args;
{
	long numberOfFrames;
	long frameBufferSize, compressedBufferSize;
	char *frameBuffer;
	long PVbuf[2];
	object *data;

	CheckCompressor(self);

	if (!getargs(args, "(is#)", &numberOfFrames, &frameBuffer, &frameBufferSize))
		return NULL;

	PVbuf[0] = CL_COMPRESSED_BUFFER_SIZE;
	PVbuf[1] = 0;
	error_handler_called = 0;
	clGetParams(self->ob_compressorHdl, PVbuf, 2L);
	if (error_handler_called)
		return NULL;

	data = newsizedstringobject(NULL, PVbuf[1]);
	if (data == NULL)
		return NULL;

	compressedBufferSize = PVbuf[1];

	error_handler_called = 0;
	if (clCompress(self->ob_compressorHdl, numberOfFrames,
		       (void *) frameBuffer, &compressedBufferSize,
		       (void *) getstringvalue(data)) == FAILURE) {
		DECREF(data);
		if (!error_handler_called)
			err_setstr(ClError, "compress failed");
		return NULL;
	}

	if (compressedBufferSize < PVbuf[1])
		if (resizestring(&data, compressedBufferSize))
			return NULL;

	if (compressedBufferSize > PVbuf[1]) {
		/* we didn't get all "compressed" data */
		DECREF(data);
		err_setstr(ClError, "compressed data is more than fitted");
		return NULL;
	}

	return data;
}

static object *
clm_Decompress(self, args)
	clobject *self;
	object *args;
{
	long PVbuf[2];
	object *data;
	long numberOfFrames;
	char *compressedData;
	long compressedDataSize;

	CheckCompressor(self);

	if (!getargs(args, "(is#)", &numberOfFrames, &compressedData,
		     &compressedDataSize))
		return NULL;

	PVbuf[0] = CL_FRAME_BUFFER_SIZE;
	PVbuf[1] = 0;
	error_handler_called = 0;
	clGetParams(self->ob_compressorHdl, PVbuf, 2L);
	if (error_handler_called)
		return NULL;

	data = newsizedstringobject(NULL, PVbuf[1]);
	if (data == NULL)
		return NULL;

	error_handler_called = 0;
	if (clDecompress(self->ob_compressorHdl, numberOfFrames,
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
doParams(self, args, func, modified)
	clobject *self;
	object *args;
	void (*func)(CL_Handle, long *, long);
	int modified;
{
	object *list, *v;
	long *PVbuffer;
	long length;
	int i;
	
	CheckCompressor(self);

	if (!getargs(args, "O", &list))
		return NULL;
	if (!is_listobject(list)) {
		err_badarg();
		return NULL;
	}
	length = getlistsize(list);
	PVbuffer = NEW(long, length);
	if (PVbuffer == NULL)
		return err_nomem();
	for (i = 0; i < length; i++) {
		v = getlistitem(list, i);
		if (is_floatobject(v))
			PVbuffer[i] = clFloatToRatio(getfloatvalue(v));
		else if (is_intobject(v))
			PVbuffer[i] = getintvalue(v);
		else {
			DEL(PVbuffer);
			err_badarg();
			return NULL;
		}
	}

	error_handler_called = 0;
	(*func)(self->ob_compressorHdl, PVbuffer, length);
	if (error_handler_called)
		return NULL;

	if (modified) {
		for (i = 0; i < length; i++) {
			v = getlistitem(list, i);
			if (is_floatobject(v))
				v = newfloatobject(clRatioToFloat(PVbuffer[i]));
			else
				v = newintobject(PVbuffer[i]);
			setlistitem(list, i, v);
		}
	}

	DEL(PVbuffer);

	INCREF(None);
	return None;
}

static object *
clm_GetParams(self, args)
	clobject *self;
	object *args;
{
	return doParams(self, args, clGetParams, 1);
}

static object *
clm_SetParams(self, args)
	clobject *self;
	object *args;
{
	return doParams(self, args, clSetParams, 1);
}

static object *
clm_GetParamID(self, args)
	clobject *self;
	object *args;
{
	char *name;
	long value;

	CheckCompressor(self);

	if (!getargs(args, "s", &name))
		return NULL;

	error_handler_called = 0;
	value = clGetParamID(self->ob_compressorHdl, name);
	if (value == FAILURE) {
		if (!error_handler_called)
			err_setstr(ClError, "getparamid failed");
		return NULL;
	}

	return newintobject(value);
}

static object *
clm_QueryParams(self, args)
	clobject *self;
	object *args;
{
	long bufferlength;
	long *PVbuffer;
	object *list;
	int i;

	CheckCompressor(self);

	if (!getnoarg(args))
		return NULL;

	error_handler_called = 0;
	bufferlength = clQueryParams(self->ob_compressorHdl, 0, 0);
	if (error_handler_called)
		return NULL;

	PVbuffer = NEW(long, bufferlength);
	if (PVbuffer == NULL)
		return err_nomem();

	bufferlength = clQueryParams(self->ob_compressorHdl, PVbuffer,
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
clm_GetMinMax(self, args)
	clobject *self;
	object *args;
{
	long param, min, max;
	double fmin, fmax;

	CheckCompressor(self);

	if (!getargs(args, "i", &param))
		return NULL;

	clGetMinMax(self->ob_compressorHdl, param, &min, &max);

	if (param_type_is_float(self->ob_compressorHdl, param) > 0) {
		fmin = clRatioToFloat(min);
		fmax = clRatioToFloat(max);
		return mkvalue("(ff)", fmin, fmax);
	}

	return mkvalue("(ii)", min, max);
}

static object *
clm_GetName(self, args)
	clobject *self;
	object *args;
{
	long param;
	char *name;

	CheckCompressor(self);

	if (!getargs(args, "i", &param))
		return NULL;

	error_handler_called = 0;
	name = clGetName(self->ob_compressorHdl, param);
	if (name == NULL || error_handler_called) {
		if (!error_handler_called)
			err_setstr(ClError, "getname failed");
		return NULL;
	}

	return newstringobject(name);
}

static object *
clm_GetDefault(self, args)
	clobject *self;
	object *args;
{
	long param, value;
	double fvalue;

	CheckCompressor(self);

	if (!getargs(args, "i", &param))
		return NULL;

	error_handler_called = 0;
	value = clGetDefault(self->ob_compressorHdl, param);
	if (error_handler_called)
		return NULL;

	if (param_type_is_float(self->ob_compressorHdl, param) > 0) {
		fvalue = clRatioToFloat(value);
		return newfloatobject(fvalue);
	}

	return newintobject(value);
}

static struct methodlist compressor_methods[] = {
	{"close",		clm_CloseCompressor}, /* alias */
	{"CloseCompressor",	clm_CloseCompressor},
	{"Compress",		clm_Compress},
	{"GetDefault",		clm_GetDefault},
	{"GetMinMax",		clm_GetMinMax},
	{"GetName",		clm_GetName},
	{"GetParamID",		clm_GetParamID},
	{"GetParams",		clm_GetParams},
	{"QueryParams",		clm_QueryParams},
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
	{"GetParamID",		clm_GetParamID},
	{"GetParams",		clm_GetParams},
	{"QueryParams",		clm_QueryParams},
	{"SetParams",		clm_SetParams},
	{NULL,			NULL}		/* sentinel */
};

static void
cl_dealloc(self)
	clobject *self;
{
	if (self->ob_compressorHdl) {
		if (self->ob_isCompressor)
			clCloseCompressor(self->ob_compressorHdl);
		else
			clCloseDecompressor(self->ob_compressorHdl);
	}
	DEL(self);
}

static object *
cl_getattr(self, name)
	clobject *self;
	char *name;
{
	if (self->ob_isCompressor)
		return findmethod(compressor_methods, (object *)self, name);
	else
		return findmethod(decompressor_methods, (object *) self, name);
}

static typeobject Cltype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"cl",			/*tp_name*/
	sizeof(clobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	cl_dealloc,		/*tp_dealloc*/
	0,			/*tp_print*/
	cl_getattr,		/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
};

static object *
doOpen(self, args, open_func, iscompressor)
	object *self, *args;
	long (*open_func) PROTO((long, CL_Handle *));
	int iscompressor;
{
	long scheme;
	clobject *new;

	if (!getargs(args, "i", &scheme))
		return NULL;

	new = NEWOBJ(clobject, &Cltype);
	if (new == NULL)
		return NULL;

	new->ob_compressorHdl = NULL;
	new->ob_isCompressor = iscompressor;

	error_handler_called = 0;
	if ((*open_func)(scheme, &new->ob_compressorHdl) == FAILURE) {
		DECREF(new);
		if (!error_handler_called)
			err_setstr(ClError, "Open(De)Compressor failed");
		return NULL;
	}
	return new;
}

static object *
cl_OpenCompressor(self, args)
	object *self, *args;
{
	return doOpen(self, args, clOpenCompressor, 1);
}

static object *
cl_OpenDecompressor(self, args)
	object *self, *args;
{
	return doOpen(self, args, clOpenDecompressor, 0);
}

static struct methodlist cl_methods[] = {
	{"CompressImage",	cl_CompressImage},
	{"DecompressImage",	cl_DecompressImage},
	{"OpenCompressor",	cl_OpenCompressor},
	{"OpenDecompressor",	cl_OpenDecompressor},
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
