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
	CL_CompressorHdl ob_compressorHdl;
	long ob_dataMaxSize;
	object *ob_callbackFunc;
	object *ob_callbackID;
	object *ob_data; 
} clobject;

extern typeobject Cltype;	/* Really static, forward */

#define is_clobject(v)		((v)->ob_type == &Cltype)

static object *ClError;		/* exception cl.error */

static int error_handler_called = 0;

static void
cl_ErrorHandler(long errnum, const char *fmt, ...)
{
	va_list ap;
	char errbuf[BUFSIZ];	/* hopefully big enough */

	error_handler_called = 1;
	va_start(ap, fmt);
	vsprintf(errbuf, fmt, ap);
	va_end(ap);
	err_setstr(ClError, errbuf);
}

static object *
cl_Compress(self, args)
	clobject *self;
	object *args;
{
	object *data;
	long frameIndex, numberOfFrames, dataSize;

	if (!getargs(args, "(ii)", &frameIndex, &numberOfFrames))
		return NULL;

	data = newsizedstringobject(NULL,
				    numberOfFrames * self->ob_dataMaxSize);
	if (data == NULL)
		return NULL;

	dataSize = numberOfFrames * self->ob_dataMaxSize;

	error_handler_called = 0;
	if (clCompress(self->ob_compressorHdl, frameIndex, numberOfFrames,
		       &dataSize, (void *) getstringvalue(data)) == FAILURE) {
		DECREF(data);
		if (!error_handler_called)
			err_setstr(ClError, "compress failed");
		return NULL;
	}

	if (dataSize < numberOfFrames * self->ob_dataMaxSize)
		if (resizestring(&data, dataSize))
			return NULL;

	return data;
}	

static object *
cl_Decompress(self, args)
	clobject *self;
	object *args;
{
	object *data;
	long frameIndex, numberOfFrames;

	if (!getargs(args, "(ii)", &frameIndex, &numberOfFrames))
		return NULL;

	data = newsizedstringobject(NULL,
				    numberOfFrames * self->ob_dataMaxSize);
	if (data == NULL)
		return NULL;

	error_handler_called = 0;
	if (clDecompress(self->ob_compressorHdl, frameIndex, numberOfFrames,
			 (void *) getstringvalue(data)) == FAILURE) {
		DECREF(data);
		if (!error_handler_called)
			err_setstr(ClError, "decompress failed");
		return NULL;
	}

	return data;
}	

static object *
cl_GetCompressorInfo(self, args)
	clobject *self;
	object *args;
{
	long result, infoSize;
	void *info;
	object *infoObject, *res;

	if (!getnoarg(args))
		return NULL;

	error_handler_called = 0;
	if (clGetCompressorInfo(self->ob_compressorHdl, &infoSize, &info) == FAILURE) {
		if (!error_handler_called)
			err_setstr(ClError, "getcompressorinfo failed");
		return NULL;
	}

	return newsizedstringobject((char *) info, infoSize);
}

static object *
cl_GetDefault(self, args)
	clobject *self;
	object *args;
{
	long initial, result;

	if (!getargs(args, "i", &initial))
		return NULL;

	error_handler_called = 0;
	result = clGetDefault(self->ob_compressorHdl, initial);
	if (error_handler_called)
		return NULL;

	return newintobject(result);
}

static object *
cl_GetMinMax(self, args)
	clobject *self;
	object *args;
{
	long param, min, max;

	if (!getargs(args, "i", &param))
		return NULL;

	error_handler_called = 0;
	clGetMinMax(self->ob_compressorHdl, param, &min, &max);
	if (error_handler_called)
		return NULL;

	return mkvalue("(ii)", min, max);
}

static object *
cl_GetName(self, args)
	clobject *self;
	object *args;
{
	long descriptor;
	char *name;

	if (!getargs(args, "i", &descriptor))
		return NULL;

	error_handler_called = 0;
	name = clGetName(self->ob_compressorHdl, descriptor);
	if (error_handler_called)
		return NULL;
	if (name == NULL) {
		err_setstr(ClError, "getname failed");
		return NULL;
	}

	return newstringobject(name);
}

static object *
doParams(self, args, func, modified)
	clobject *self;
	object *args;
	void (*func)(CL_CompressorHdl, long *, long);
	int modified;
{
	object *list, *v;
	long *PVbuffer;
	long length;
	int i;
	
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
		if (!is_intobject(v)) {
			DEL(PVbuffer);
			err_badarg();
			return NULL;
		}
		PVbuffer[i] = getintvalue(v);
	}

	error_handler_called = 0;
	(*func)(self->ob_compressorHdl, PVbuffer, length);
	if (error_handler_called)
		return NULL;

	if (modified) {
		for (i = 0; i < length; i++)
			setlistitem(list, i, newintobject(PVbuffer[i]));
	}

	DEL(PVbuffer);

	INCREF(None);
	return None;
}

static object *
cl_GetParams(self, args)
	object *self, *args;
{
	return doParams(self, args, clGetParams, 1);
}

static object *
cl_SetParams(self, args)
	object *self, *args;
{
	return doParams(self, args, clSetParams, 0);
}

static object *
cl_QueryParams(self, args)
	clobject *self;
	object *args;
{
	long bufferlength;
	long *PVbuffer;
	object *list;
	int i;

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
		else
			setlistitem(list, i, newstringobject((char *) PVbuffer[i]));
	}

	DEL(PVbuffer);

	return list;
}

static struct methodlist compressor_methods[] = {
	{"Compress",		cl_Compress},
	{"GetCompressorInfo",	cl_GetCompressorInfo},
	{"GetDefault",		cl_GetDefault},
	{"GetMinMax",		cl_GetMinMax},
	{"GetName",		cl_GetName},
	{"GetParams",		cl_GetParams},
	{"QueryParams",		cl_QueryParams},
	{"SetParams",		cl_SetParams},
	{NULL,			NULL}		/* sentinel */
};

static struct methodlist decompressor_methods[] = {
	{"Decompress",		cl_Decompress},
	{"GetDefault",		cl_GetDefault},
	{"GetMinMax",		cl_GetMinMax},
	{"GetName",		cl_GetName},
	{"GetParams",		cl_GetParams},
	{"QueryParams",		cl_QueryParams},
	{"SetParams",		cl_SetParams},
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
	XDECREF(self->ob_callbackFunc);
	XDECREF(self->ob_callbackID);
	XDECREF(self->ob_data);
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

static long
GetFrame(callbackID, frameIndex, numberOfFrames, data)
	void *callbackID;
	long frameIndex;
	long numberOfFrames;
	void **data;
{
	object *args;
	clobject *self = (clobject *) callbackID;
	object *result;

	args = newtupleobject(3);
	if (args == NULL)
		return FAILURE;

	XINCREF(self->ob_callbackID);
	settupleitem(args, 0, self->ob_callbackID);
	settupleitem(args, 1, newintobject(frameIndex));
	settupleitem(args, 2, newintobject(numberOfFrames));

	if (err_occurred()) {
		XDECREF(self->ob_callbackID);
		return FAILURE;
	}

	result = call_object(self->ob_callbackFunc, args);
	DECREF(args);
	if (result == NULL)
		return FAILURE;

	if (!is_stringobject(result)) {
		DECREF(result);
		return FAILURE;
	}

	XDECREF(self->ob_data);
	self->ob_data = result;
	
	*data = (void *) getstringvalue(result);

	return SUCCESS;
}

static long
GetData(callbackID, frameIndex, numberOfFrames, dataSize, data)
	void *callbackID;
	long frameIndex;
	long numberOfFrames;
	long *dataSize;
	void **data;
{
	object *args, *result;
	clobject *self = (clobject *) callbackID;

	args = newtupleobject(3);
	if (args == NULL)
		return FAILURE;

	XINCREF(self->ob_callbackID);
	settupleitem(args, 0, self->ob_callbackID);
	settupleitem(args, 1, newintobject(frameIndex));
	settupleitem(args, 2, newintobject(numberOfFrames));

	if (err_occurred()) {
		XDECREF(self->ob_callbackID);
		return FAILURE;
	}

	result = call_object(self->ob_callbackFunc, args);
	DECREF(args);
	if (result == NULL)
		return FAILURE;

	if (!is_stringobject(result)) {
		DECREF(result);
		return FAILURE;
	}

	XDECREF(self->ob_data);
	self->ob_data = result;
	
	*dataSize = getstringsize(result);
	*data = (void *) getstringvalue(result);

	return SUCCESS;
}

static object *
cl_OpenCompressor(self, args)
	object *self, *args;
{
	CL_CompressionFormat compressionFormat;
	long qualityFactor;
	object *GetFrameCBPtr;
	object *callbackID;
	clobject *new;

	if (!getargs(args, "((iiiiiiiiii)iOO)",
		     &compressionFormat.width,
		     &compressionFormat.height,
		     &compressionFormat.frameSize,
		     &compressionFormat.dataMaxSize,
		     &compressionFormat.originalFormat,
		     &compressionFormat.components,
		     &compressionFormat.bitsPerComponent,
		     &compressionFormat.frameRate,
		     &compressionFormat.numberOfFrames,
		     &compressionFormat.compressionScheme,
		     &qualityFactor, &GetFrameCBPtr, &callbackID))
		return NULL;

	new = NEWOBJ(clobject, &Cltype);
	if (new == 0)
		return NULL;

	new->ob_compressorHdl = NULL;
	new->ob_callbackFunc = NULL;
	new->ob_callbackID = NULL;
	new->ob_data = NULL;

	error_handler_called = 0;
	if (clOpenCompressor(&compressionFormat, qualityFactor, GetFrame,
			     (void *) new, &new->ob_compressorHdl) == FAILURE) {
		DECREF(new);
		if (!error_handler_called)
			err_setstr(ClError, "opencompressor failed");
		return NULL;
	}

	new->ob_isCompressor = 1;
	new->ob_callbackFunc = GetFrameCBPtr;
	XINCREF(new->ob_callbackFunc);
	if (callbackID == NULL)
		callbackID = None;
	new->ob_callbackID = callbackID;
	INCREF(new->ob_callbackID);
	new->ob_data = NULL;
	new->ob_dataMaxSize = compressionFormat.dataMaxSize;

	return new;
}

static object *
cl_OpenDecompressor(self, args)
	object *self, *args;
{
	CL_CompressionFormat compressionFormat;
	long infoSize;
	void *info;
	object *GetDataCBPtr;
	object *callbackID;
	clobject *new;
	object *res;

	if (!getargs(args, "(s#OO)", &info, &infoSize, &GetDataCBPtr,
		     &callbackID))
		return NULL;

	new = NEWOBJ(clobject, &Cltype);
	if (new == 0)
		return NULL;

	new->ob_compressorHdl = NULL;
	new->ob_callbackFunc = NULL;
	new->ob_callbackID = NULL;
	new->ob_data = NULL;

	error_handler_called = 0;
	if (clOpenDecompressor(&compressionFormat, infoSize, info, GetData,
			       (void *) new, &new->ob_compressorHdl) == FAILURE) {
		DECREF(new);
		if (!error_handler_called)
			err_setstr(ClError, "opendecompressor failed");
		return NULL;
	}

	new->ob_isCompressor = 0;
	new->ob_callbackFunc = GetDataCBPtr;
	XINCREF(new->ob_callbackFunc);
	if (callbackID == NULL)
		callbackID = None;
	new->ob_callbackID = callbackID;
	XINCREF(new->ob_callbackID);
	new->ob_data = NULL;

	res = mkvalue("(O(iiiiiiiiii))", new, 
		     compressionFormat.width,
		     compressionFormat.height,
		     compressionFormat.frameSize,
		     compressionFormat.dataMaxSize,
		     compressionFormat.originalFormat,
		     compressionFormat.components,
		     compressionFormat.bitsPerComponent,
		     compressionFormat.frameRate,
		     compressionFormat.numberOfFrames,
		     compressionFormat.compressionScheme);

	DECREF(new);
	return res;
}

static object *
cl_AddParam(self, args)
	object *self, *args;
{
	char *name;
	long type, min, max, initial, paramID;

	if (!getargs(args, "(siiii)", &name, &type, &min, &max, &initial))
		return NULL;

	error_handler_called = 0;
	if (clAddParam(name, type, min, max, initial, &paramID) == FAILURE) {
		if (!error_handler_called)
			err_setstr(ClError, "addparam failed");
		return NULL;
	}

	return newintobject(paramID);
}

static struct methodlist cl_methods[] = {
	{"AddParam",		cl_AddParam},
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
