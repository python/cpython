/**********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

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

/* SV module -- interface to the Indigo video board */

#include <sys/time.h>
#include <svideo.h>
#include "allobjects.h"
#include "import.h"
#include "modsupport.h"
#include "compile.h"
#include "ceval.h"
#include "yuv.h"		/* for YUV conversion functions */

typedef struct {
	OB_HEAD
	SV_nodeP ob_svideo;
	svCaptureInfo ob_info;
} svobject;

typedef struct {
	OB_HEAD
	void *ob_capture;
	int ob_mustunlock;
	svCaptureInfo ob_info;
	svobject *ob_svideo;
} captureobject;

static object *SvError;		/* exception sv.error */

static object *newcaptureobject PROTO((svobject *, void *, int));

/* Set a SV-specific error from svideo_errno and return NULL */
static object *
sv_error()
{
	err_setstr(SvError, svStrerror(svideo_errno));
	return NULL;
}

static object *
svc_conversion(self, args, function, factor)
	captureobject *self;
	object *args;
	void (*function)();
	float factor;
{
	object *output;
	int invert;

	if (!getargs(args, "i", &invert))
		return NULL;

	output = newsizedstringobject(NULL, (int) (self->ob_info.width * self->ob_info.height * factor));
	if (output == NULL)
		return NULL;

	(*function)((boolean) invert, self->ob_capture, getstringvalue(output),
		    self->ob_info.width, self->ob_info.height);

	return output;
}

/*
 * 3 functions to convert from Starter Video YUV 4:1:1 format to
 * Compression Library 4:2:2 Duplicate Chroma format.
 */
static object *
svc_YUVtoYUV422DC(self, args)
	captureobject *self;
	object *args;
{
	if (self->ob_info.format != SV_YUV411_FRAMES) {
		err_setstr(SvError, "data has bad format");
		return NULL;
	}
	return svc_conversion(self, args, yuv_sv411_to_cl422dc, 2.0);
}

static object *
svc_YUVtoYUV422DC_quarter(self, args)
	captureobject *self;
	object *args;
{
	if (self->ob_info.format != SV_YUV411_FRAMES) {
		err_setstr(SvError, "data has bad format");
		return NULL;
	}
	return svc_conversion(self, args, yuv_sv411_to_cl422dc_quartersize, 0.5);
}

static object *
svc_YUVtoYUV422DC_sixteenth(self, args)
	captureobject *self;
	object *args;
{
	if (self->ob_info.format != SV_YUV411_FRAMES) {
		err_setstr(SvError, "data has bad format");
		return NULL;
	}
	return svc_conversion(self, args, yuv_sv411_to_cl422dc_sixteenthsize, 0.125);
}

static object *
svc_YUVtoRGB(self, args)
	captureobject *self;
	object *args;
{
	switch (self->ob_info.format) {
	case SV_YUV411_FRAMES:
	case SV_YUV411_FRAMES_AND_BLANKING_BUFFER:
		break;
	default:
		err_setstr(SvError, "data had bad format");
		return NULL;
	}
	return svc_conversion(self, args, svYUVtoRGB, (float) sizeof(long));
}

static object *
svc_RGB8toRGB32(self, args)
	captureobject *self;
	object *args;
{
	if (self->ob_info.format != SV_RGB8_FRAMES) {
		err_setstr(SvError, "data has bad format");
		return NULL;
	}
	return svc_conversion(self, args, svRGB8toRGB32, (float) sizeof(long));
}

static object *
svc_InterleaveFields(self, args)
	captureobject *self;
	object *args;
{
	if (self->ob_info.format != SV_RGB8_FRAMES) {
		err_setstr(SvError, "data has bad format");
		return NULL;
	}
	return svc_conversion(self, args, svInterleaveFields, 1.0);
}

static object *
svc_GetFields(self, args)
	captureobject *self;
	object *args;
{
	object *f1, *f2, *ret;
	int fieldsize;

	if (self->ob_info.format != SV_RGB8_FRAMES) {
		err_setstr(SvError, "data has bad format");
		return NULL;
	}

	fieldsize = self->ob_info.width * self->ob_info.height / 2;
	f1 = newsizedstringobject((char *) self->ob_capture, fieldsize);
	f2 = newsizedstringobject((char *) self->ob_capture + fieldsize, fieldsize);
	ret = mkvalue("(OO)", f1, f2);
	XDECREF(f1);
	XDECREF(f2);
	return ret;
}
	
static object *
svc_UnlockCaptureData(self, args)
	captureobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;

	if (!self->ob_mustunlock) {
		err_setstr(SvError, "buffer should not be unlocked");
		return NULL;
	}

	if (svUnlockCaptureData(self->ob_svideo->ob_svideo, self->ob_capture))
		return sv_error();

	self->ob_mustunlock = 0;

	INCREF(None);
	return None;
}

#ifdef USE_GL
#include <gl.h>

static object *
svc_lrectwrite(self, args)
	captureobject *self;
	object *args;
{
	Screencoord x1, x2, y1, y2;

	if (!getargs(args, "(hhhh)", &x1, &x2, &y1, &y2))
		return NULL;

	lrectwrite(x1, x2, y1, y2, (unsigned long *) self->ob_capture);

	INCREF(None);
	return None;
}
#endif

static object *
svc_writefile(self, args)
	captureobject *self;
	object *args;
{
	object *file;
	int size;

	if (!getargs(args, "O", &file))
		return NULL;

	if (!is_fileobject(file)) {
		err_setstr(SvError, "not a file object");
		return NULL;
	}

	size = self->ob_info.width * self->ob_info.height;

	if (fwrite(self->ob_capture, sizeof(long), size, getfilefile(file)) != size) {
		err_setstr(SvError, "writing failed");
		return NULL;
	}

	INCREF(None);
	return None;
}

static object *
svc_FindVisibleRegion(self, args)
	captureobject *self;
	object *args;
{
	void *visible;
	int width;

	if (!getnoarg(args))
		return NULL;

	if (svFindVisibleRegion(self->ob_svideo->ob_svideo, self->ob_capture, &visible, self->ob_info.width))
		return sv_error();

	if (visible == NULL) {
		err_setstr(SvError, "data in wrong format");
		return NULL;
	}

	return newcaptureobject(self->ob_svideo, visible, 0);
}

static struct methodlist capture_methods[] = {
	{"YUVtoRGB",		(method)svc_YUVtoRGB},
	{"RGB8toRGB32",		(method)svc_RGB8toRGB32},
	{"InterleaveFields",	(method)svc_InterleaveFields},
	{"UnlockCaptureData",	(method)svc_UnlockCaptureData},
	{"FindVisibleRegion",	(method)svc_FindVisibleRegion},
	{"GetFields",		(method)svc_GetFields},
	{"YUVtoYUV422DC",	(method)svc_YUVtoYUV422DC},
	{"YUVtoYUV422DC_quarter",(method)svc_YUVtoYUV422DC_quarter},
	{"YUVtoYUV422DC_sixteenth",(method)svc_YUVtoYUV422DC_sixteenth},
#ifdef USE_GL
	{"lrectwrite",		(method)svc_lrectwrite},
#endif
	{"writefile",		(method)svc_writefile},
	{NULL,			NULL} 		/* sentinel */
};

static void
capture_dealloc(self)
	captureobject *self;
{
	if (self->ob_capture != NULL) {
		if (self->ob_mustunlock)
			(void) svUnlockCaptureData(self->ob_svideo->ob_svideo, self->ob_capture);
		self->ob_capture = NULL;
		DECREF(self->ob_svideo);
		self->ob_svideo = NULL;
	}
	DEL(self);
}

static object *
capture_getattr(self, name)
	svobject *self;
	char *name;
{
	return findmethod(capture_methods, (object *)self, name);
}

typeobject Capturetype = {
	OB_HEAD_INIT(&Typetype)
	0,				/*ob_size*/
	"capture",			/*tp_name*/
	sizeof(captureobject),		/*tp_size*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)capture_dealloc,	/*tp_dealloc*/
	0,				/*tp_print*/
	(getattrfunc)capture_getattr,	/*tp_getattr*/
	0,				/*tp_setattr*/
	0,				/*tp_compare*/
	0,				/*tp_repr*/
};

static object *
newcaptureobject(self, ptr, mustunlock)
	svobject *self;
	void *ptr;
	int mustunlock;
{
	captureobject *p;

	p = NEWOBJ(captureobject, &Capturetype);
	if (p == NULL)
		return NULL;
	p->ob_svideo = self;
	INCREF(self);
	p->ob_capture = ptr;
	p->ob_mustunlock = mustunlock;
	p->ob_info = self->ob_info;
	return (object *) p;
}

static object *
sv_GetCaptureData(self, args)
	svobject *self;
	object *args;
{
	void *ptr;
	long fieldID;
	object *res, *c;

	if (!getnoarg(args))
		return NULL;

	if (svGetCaptureData(self->ob_svideo, &ptr, &fieldID))
		return sv_error();

	if (ptr == NULL) {
		err_setstr(SvError, "no data available");
		return NULL;
	}

	c = newcaptureobject(self, ptr, 1);
	if (c == NULL)
		return NULL;
	res = mkvalue("(Oi)", c, fieldID);
	DECREF(c);
	return res;
}

static object *
sv_BindGLWindow(self, args)
	svobject *self;
	object *args;
{
	long wid;
	int mode;

	if (!getargs(args, "(ii)", &wid, &mode))
		return NULL;

	if (svBindGLWindow(self->ob_svideo, wid, mode))
		return sv_error();

	INCREF(None);
	return None;
}

static object *
sv_EndContinuousCapture(self, args)
	svobject *self;
	object *args;
{

	if (!getnoarg(args))
		return NULL;

	if (svEndContinuousCapture(self->ob_svideo))
		return sv_error();

	INCREF(None);
	return None;
}

static object *
sv_IsVideoDisplayed(self, args)
	svobject *self;
	object *args;
{
	int v;

	if (!getnoarg(args))
		return NULL;

	v = svIsVideoDisplayed(self->ob_svideo);
	if (v == -1)
		return sv_error();

	return newintobject((long) v);
}

static object *
sv_OutputOffset(self, args)
	svobject *self;
	object *args;
{
	int x_offset;
	int y_offset;

	if (!getargs(args, "(ii)", &x_offset, &y_offset))
		return NULL;

	if (svOutputOffset(self->ob_svideo, x_offset, y_offset))
		return sv_error();

	INCREF(None);
	return None;
}

static object *
sv_PutFrame(self, args)
	svobject *self;
	object *args;
{
	char *buffer;

	if (!getargs(args, "s", &buffer))
		return NULL;

	if (svPutFrame(self->ob_svideo, buffer))
		return sv_error();

	INCREF(None);
	return None;
}

static object *
sv_QuerySize(self, args)
	svobject *self;
	object *args;
{
	int w;
	int h;
	int rw;
	int rh;

	if (!getargs(args, "(ii)", &w, &h))
		return NULL;

	if (svQuerySize(self->ob_svideo, w, h, &rw, &rh))
		return sv_error();

	return mkvalue("(ii)", (long) rw, (long) rh);
}

static object *
sv_SetSize(self, args)
	svobject *self;
	object *args;
{
	int w;
	int h;

	if (!getargs(args, "(ii)", &w, &h))
		return NULL;

	if (svSetSize(self->ob_svideo, w, h))
		return sv_error();

	INCREF(None);
	return None;
}

static object *
sv_SetStdDefaults(self, args)
	svobject *self;
	object *args;
{

	if (!getnoarg(args))
		return NULL;

	if (svSetStdDefaults(self->ob_svideo))
		return sv_error();

	INCREF(None);
	return None;
}

static object *
sv_UseExclusive(self, args)
	svobject *self;
	object *args;
{
	boolean onoff;
	int mode;

	if (!getargs(args, "(ii)", &onoff, &mode))
		return NULL;

	if (svUseExclusive(self->ob_svideo, onoff, mode))
		return sv_error();

	INCREF(None);
	return None;
}

static object *
sv_WindowOffset(self, args)
	svobject *self;
	object *args;
{
	int x_offset;
	int y_offset;

	if (!getargs(args, "(ii)", &x_offset, &y_offset))
		return NULL;

	if (svWindowOffset(self->ob_svideo, x_offset, y_offset))
		return sv_error();

	INCREF(None);
	return None;
}

static object *
sv_CaptureBurst(self, args)
	svobject *self;
	object *args;
{
	int bytes, i;
	svCaptureInfo info;
	void *bitvector = NULL;
	object *videodata, *bitvecobj, *res;
	static object *evenitem, *odditem;

	if (!getargs(args, "(iiiii)", &info.format, &info.width, &info.height,
		     &info.size, &info.samplingrate))
		return NULL;

	switch (info.format) {
	case SV_RGB8_FRAMES:
		bitvector = malloc(SV_BITVEC_SIZE(info.size));
		break;
	case SV_YUV411_FRAMES_AND_BLANKING_BUFFER:
		break;
	default:
		err_setstr(SvError, "illegal format specified");
		return NULL;
	}

	if (svQueryCaptureBufferSize(self->ob_svideo, &info, &bytes)) {
		if (bitvector)
			free(bitvector);
		return sv_error();
	}

	videodata = newsizedstringobject(NULL, bytes);
	if (videodata == NULL) {
		if (bitvector)
			free(bitvector);
		return NULL;
	}

	/* XXX -- need to do something about the bitvector */
	if (svCaptureBurst(self->ob_svideo, &info, getstringvalue(videodata),
			   bitvector)) {
		if (bitvector)
			free(bitvector);
		DECREF(videodata);
		return sv_error();
	}

	if (bitvector) {
		if (evenitem == NULL) {
			evenitem = newintobject(0);
			if (evenitem == NULL) {
				free(bitvector);
				DECREF(videodata);
				return NULL;
			}
		}
		if (odditem == NULL) {
			odditem = newintobject(1);
			if (odditem == NULL) {
				free(bitvector);
				DECREF(videodata);
				return NULL;
			}
		}
		bitvecobj = newtupleobject(2 * info.size);
		if (bitvecobj == NULL) {
			free(bitvecobj);
			DECREF(videodata);
			return NULL;
		}
		for (i = 0; i < 2 * info.size; i++) {
			if (SV_GET_FIELD(bitvector, i) == SV_EVEN_FIELD) {
				INCREF(evenitem);
				settupleitem(bitvecobj, i, evenitem);
			} else {
				INCREF(odditem);
				settupleitem(bitvecobj, i, odditem);
			}
		}
		free(bitvector);
	} else {
		bitvecobj = None;
		INCREF(None);
	}

	res = mkvalue("((iiiii)OO)", info.format, info.width, info.height,
		       info.size, info.samplingrate, videodata, bitvecobj);
	DECREF(videodata);
	DECREF(bitvecobj);
	return res;
}

static object *
sv_CaptureOneFrame(self, args)
	svobject *self;
	object *args;
{
	svCaptureInfo info;
	int format, width, height;
	int bytes;
	object *videodata, *res;
	
	if (!getargs(args, "(iii)", &format, &width, &height))
		return NULL;
	info.format = format;
	info.width = width;
	info.height = height;
	info.size = 0;
	info.samplingrate = 0;
	if (svQueryCaptureBufferSize(self->ob_svideo, &info, &bytes))
		return sv_error();
	videodata = newsizedstringobject(NULL, bytes);
	if (videodata == NULL)
		return NULL;
	if (svCaptureOneFrame(self->ob_svideo, format, &width, &height,
			      getstringvalue(videodata))) {
		DECREF(videodata);
		return sv_error();
	}

	res = mkvalue("(iiO)", width, height, videodata);
	DECREF(videodata);
	return res;
}

static object *
sv_InitContinuousCapture(self, args)
	svobject *self;
	object *args;
{
	svCaptureInfo info;

	if (!getargs(args, "(iiiii)", &info.format, &info.width, &info.height,
		     &info.size, &info.samplingrate))
		return NULL;

	if (svInitContinuousCapture(self->ob_svideo, &info))
		return sv_error();

	self->ob_info = info;

	return mkvalue("(iiiii)", info.format, info.width, info.height,
		       info.size, info.samplingrate);
}

static object *
sv_LoadMap(self, args)
	svobject *self;
	object *args;
{
	object *rgb, *v, *cell;
	rgb_tuple *mapp;
	int maptype;
	int i, j;		/* indices */

	if (!getargs(args, "(iO)", &maptype, &rgb))
		return NULL;
	if (!is_listobject(rgb) || getlistsize(rgb) != 256) {
		err_badarg();
		return NULL;
	}
	mapp = NEW(rgb_tuple, 256);
	if (mapp == NULL)
		return err_nomem();
	for (i = 0; i < 256; i++) {
		v = getlistitem(rgb, i);
		if (!is_tupleobject(v) || gettuplesize(v) != 3) {
			DEL(mapp);
			err_badarg();
			return NULL;
		}
		for (j = 0; j < 3; j++) {
			cell = gettupleitem(v, j);
			if (!is_intobject(cell)) {
				DEL(mapp);
				err_badarg();
				return NULL;
			}
			switch (j) {
			case 0: mapp[i].red = getintvalue(cell); break;
			case 1: mapp[i].blue = getintvalue(cell); break;
			case 2: mapp[i].green = getintvalue(cell); break;
			}
		}
	}

	if (svLoadMap(self->ob_svideo, maptype, mapp)) {
		DEL(mapp);
		return sv_error();
	}

	DEL(mapp);

	INCREF(None);
	return None;
}
		
static object *
sv_CloseVideo(self, args)
	svobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;

	if (svCloseVideo(self->ob_svideo))
		return sv_error();
	self->ob_svideo = NULL;

	INCREF(None);
	return None;
}

static object *
doParams(self, args, func, modified)
	svobject *self;
	object *args;
	int (*func)(SV_nodeP, long *, int);
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

	if ((*func)(self->ob_svideo, PVbuffer, length)) {
		DEL(PVbuffer);
		return sv_error();
	}

	if (modified) {
		for (i = 0; i < length; i++)
			setlistitem(list, i, newintobject(PVbuffer[i]));
	}

	DEL(PVbuffer);

	INCREF(None);
	return None;
}

static object *
sv_GetParam(self, args)
	object *self, *args;
{
	return doParams(self, args, svGetParam, 1);
}

static object *
sv_GetParamRange(self, args)
	object *self, *args;
{
	return doParams(self, args, svGetParamRange, 1);
}

static object *
sv_SetParam(self, args)
	object *self, *args;
{
	return doParams(self, args, svSetParam, 0);
}

static struct methodlist svideo_methods[] = {
	{"BindGLWindow",	(method)sv_BindGLWindow},
	{"EndContinuousCapture",(method)sv_EndContinuousCapture},
	{"IsVideoDisplayed",	(method)sv_IsVideoDisplayed},
	{"OutputOffset",	(method)sv_OutputOffset},
	{"PutFrame",		(method)sv_PutFrame},
	{"QuerySize",		(method)sv_QuerySize},
	{"SetSize",		(method)sv_SetSize},
	{"SetStdDefaults",	(method)sv_SetStdDefaults},
	{"UseExclusive",	(method)sv_UseExclusive},
	{"WindowOffset",	(method)sv_WindowOffset},
	{"InitContinuousCapture",(method)sv_InitContinuousCapture},
	{"CaptureBurst",	(method)sv_CaptureBurst},
	{"CaptureOneFrame",	(method)sv_CaptureOneFrame},
	{"GetCaptureData",	(method)sv_GetCaptureData},
	{"CloseVideo",		(method)sv_CloseVideo},
	{"LoadMap",		(method)sv_LoadMap},
	{"GetParam",		(method)sv_GetParam},
	{"GetParamRange",	(method)sv_GetParamRange},
	{"SetParam",		(method)sv_SetParam},
	{NULL,			NULL} 		/* sentinel */
};

static object *
sv_conversion(self, args, function, inputfactor, factor)
	object *self, *args;
	void (*function)();
	int inputfactor;
	float factor;
{
	int invert, width, height, inputlength;
	char *input;
	object *output;

	if (!getargs(args, "(is#ii)", &invert, &input, &inputlength, &width, &height))
		return NULL;

	if (width * height * inputfactor > inputlength) {
		err_setstr(SvError, "input buffer not long enough");
		return NULL;
	}

	output = newsizedstringobject(NULL, (int) (width * height * factor));
	if (output == NULL)
		return NULL;

	(*function)(invert, input, getstringvalue(output), width, height);

	return output;
}

static object *
sv_InterleaveFields(self, args)
	object *self, *args;
{
	return sv_conversion(self, args, svInterleaveFields, 1, 1.0);
}

static object *
sv_RGB8toRGB32(self, args)
	object *self, *args;
{
	return sv_conversion(self, args, svRGB8toRGB32, 1, (float) sizeof(long));
}

static object *
sv_YUVtoRGB(self, args)
	object *self, *args;
{
	return sv_conversion(self, args, svYUVtoRGB, 2, (float) sizeof(long));
}

static void
svideo_dealloc(self)
	svobject *self;
{
	if (self->ob_svideo != NULL)
		(void) svCloseVideo(self->ob_svideo);
	DEL(self);
}

static object *
svideo_getattr(self, name)
	svobject *self;
	char *name;
{
	return findmethod(svideo_methods, (object *)self, name);
}

typeobject Svtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"sv",			/*tp_name*/
	sizeof(svobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)svideo_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)svideo_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

static object *
newsvobject(svp)
	SV_nodeP svp;
{
	svobject *p;

	p = NEWOBJ(svobject, &Svtype);
	if (p == NULL)
		return NULL;
	p->ob_svideo = svp;
	p->ob_info.format = 0;
	p->ob_info.size = 0;
	p->ob_info.width = 0;
	p->ob_info.height = 0;
	p->ob_info.samplingrate = 0;
	return (object *) p;
}

static object *
sv_OpenVideo(self, args)
	object *self, *args;
{
	SV_nodeP svp;

	if (!getnoarg(args))
		return NULL;

	svp = svOpenVideo();
	if (svp == NULL)
		return sv_error();

	return newsvobject(svp);
}

static struct methodlist sv_methods[] = {
	{"InterleaveFields",	(method)sv_InterleaveFields},
	{"RGB8toRGB32",		(method)sv_RGB8toRGB32},
	{"YUVtoRGB",		(method)sv_YUVtoRGB},
	{"OpenVideo",		(method)sv_OpenVideo},
	{NULL,			NULL}	/* Sentinel */
};

void
initsv()
{
	object *m, *d;

	m = initmodule("sv", sv_methods);
	d = getmoduledict(m);

	SvError = newstringobject("sv.error");
	if (SvError == NULL || dictinsert(d, "error", SvError) != 0)
		fatal("can't define sv.error");
}
