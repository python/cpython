/* SV module -- interface to the Indigo video board */

/* WARNING! This module is for hardware that we don't have any more,
   so it hasn't been tested.  It has been converted to the new coding
   style, and it is possible that this conversion has broken something
   -- user beware! */

#include <sys/time.h>
#include <svideo.h>
#include "Python.h"
#include "compile.h"
#include "yuv.h"		/* for YUV conversion functions */

typedef struct {
	PyObject_HEAD
	SV_nodeP ob_svideo;
	svCaptureInfo ob_info;
} svobject;

typedef struct {
	PyObject_HEAD
	void *ob_capture;
	int ob_mustunlock;
	svCaptureInfo ob_info;
	svobject *ob_svideo;
} captureobject;

static PyObject *SvError;		/* exception sv.error */

static PyObject *newcaptureobject(svobject *, void *, int);

/* Set a SV-specific error from svideo_errno and return NULL */
static PyObject *
sv_error(void)
{
	PyErr_SetString(SvError, svStrerror(svideo_errno));
	return NULL;
}

static PyObject *
svc_conversion(captureobject *self, PyObject *args, void (*function)(),	float factor)
{
	PyObject *output;
	int invert;
	char* outstr;

	if (!PyArg_Parse(args, "i", &invert))
		return NULL;

	if (!(output = PyString_FromStringAndSize(
		NULL,
		(int)(self->ob_info.width * self->ob_info.height * factor))))
	{
		return NULL;
	}
	if (!(outstr = PyString_AsString(output))) {
		Py_DECREF(output);
		return NULL;
	}

	(*function)((boolean)invert, self->ob_capture,
		    outstr,
		    self->ob_info.width, self->ob_info.height);

	return output;
}

/*
 * 3 functions to convert from Starter Video YUV 4:1:1 format to
 * Compression Library 4:2:2 Duplicate Chroma format.
 */
static PyObject *
svc_YUVtoYUV422DC(captureobject *self, PyObject *args)
{
	if (self->ob_info.format != SV_YUV411_FRAMES) {
		PyErr_SetString(SvError, "data has bad format");
		return NULL;
	}
	return svc_conversion(self, args, yuv_sv411_to_cl422dc, 2.0);
}

static PyObject *
svc_YUVtoYUV422DC_quarter(captureobject *self, PyObject *args)
{
	if (self->ob_info.format != SV_YUV411_FRAMES) {
		PyErr_SetString(SvError, "data has bad format");
		return NULL;
	}
	return svc_conversion(self, args,
			      yuv_sv411_to_cl422dc_quartersize, 0.5);
}

static PyObject *
svc_YUVtoYUV422DC_sixteenth(captureobject *self, PyObject *args)
{
	if (self->ob_info.format != SV_YUV411_FRAMES) {
		PyErr_SetString(SvError, "data has bad format");
		return NULL;
	}
	return svc_conversion(self, args,
			      yuv_sv411_to_cl422dc_sixteenthsize, 0.125);
}

static PyObject *
svc_YUVtoRGB(captureobject *self, PyObject *args)
{
	switch (self->ob_info.format) {
	case SV_YUV411_FRAMES:
	case SV_YUV411_FRAMES_AND_BLANKING_BUFFER:
		break;
	default:
		PyErr_SetString(SvError, "data had bad format");
		return NULL;
	}
	return svc_conversion(self, args, svYUVtoRGB, (float) sizeof(long));
}

static PyObject *
svc_RGB8toRGB32(captureobject *self, PyObject *args)
{
	if (self->ob_info.format != SV_RGB8_FRAMES) {
		PyErr_SetString(SvError, "data has bad format");
		return NULL;
	}
	return svc_conversion(self, args, svRGB8toRGB32, (float) sizeof(long));
}

static PyObject *
svc_InterleaveFields(captureobject *self, PyObject *args)
{
	if (self->ob_info.format != SV_RGB8_FRAMES) {
		PyErr_SetString(SvError, "data has bad format");
		return NULL;
	}
	return svc_conversion(self, args, svInterleaveFields, 1.0);
}

static PyObject *
svc_GetFields(captureobject *self, PyObject *args)
{
	PyObject *f1 = NULL;
	PyObject *f2 = NULL;
	PyObject *ret = NULL;
	int fieldsize;
	char* obcapture;

	if (self->ob_info.format != SV_RGB8_FRAMES) {
		PyErr_SetString(SvError, "data has bad format");
		return NULL;
	}

	fieldsize = self->ob_info.width * self->ob_info.height / 2;
	obcapture = (char*)self->ob_capture;
	
	if (!(f1 = PyString_FromStringAndSize(obcapture, fieldsize)))
		goto finally;
	if (!(f2 = PyString_FromStringAndSize(obcapture + fieldsize,
					      fieldsize)))
		goto finally;
	ret = Py_BuildValue("(OO)", f1, f2);

  finally:
	Py_XDECREF(f1);
	Py_XDECREF(f2);
	return ret;
}
	
static PyObject *
svc_UnlockCaptureData(captureobject *self, PyObject *args)
{
	if (!PyArg_Parse(args, ""))
		return NULL;

	if (!self->ob_mustunlock) {
		PyErr_SetString(SvError, "buffer should not be unlocked");
		return NULL;
	}

	if (svUnlockCaptureData(self->ob_svideo->ob_svideo, self->ob_capture))
		return sv_error();

	self->ob_mustunlock = 0;

	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef USE_GL
#include <gl.h>

static PyObject *
svc_lrectwrite(captureobject *self, PyObject *args)
{
	Screencoord x1, x2, y1, y2;

	if (!PyArg_Parse(args, "(hhhh)", &x1, &x2, &y1, &y2))
		return NULL;

	lrectwrite(x1, x2, y1, y2, (unsigned long *) self->ob_capture);

	Py_INCREF(Py_None);
	return Py_None;
}
#endif

static PyObject *
svc_writefile(captureobject *self, PyObject *args)
{
	PyObject *file;
	int size;
	FILE* fp;

	if (!PyArg_Parse(args, "O", &file))
		return NULL;

	if (!PyFile_Check(file)) {
		PyErr_SetString(SvError, "not a file object");
		return NULL;
	}

	if (!(fp = PyFile_AsFile(file)))
		return NULL;

	size = self->ob_info.width * self->ob_info.height;

	if (fwrite(self->ob_capture, sizeof(long), size, fp) != size) {
		PyErr_SetString(SvError, "writing failed");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
svc_FindVisibleRegion(captureobject *self, PyObject *args)
{
	void *visible;
	int width;

	if (!PyArg_Parse(args, ""))
		return NULL;

	if (svFindVisibleRegion(self->ob_svideo->ob_svideo,
				self->ob_capture, &visible,
				self->ob_info.width))
		return sv_error();

	if (visible == NULL) {
		PyErr_SetString(SvError, "data in wrong format");
		return NULL;
	}

	return newcaptureobject(self->ob_svideo, visible, 0);
}

static PyMethodDef capture_methods[] = {
	{"YUVtoRGB",		(PyCFunction)svc_YUVtoRGB},
	{"RGB8toRGB32",		(PyCFunction)svc_RGB8toRGB32},
	{"InterleaveFields",	(PyCFunction)svc_InterleaveFields},
	{"UnlockCaptureData",	(PyCFunction)svc_UnlockCaptureData},
	{"FindVisibleRegion",	(PyCFunction)svc_FindVisibleRegion},
	{"GetFields",		(PyCFunction)svc_GetFields},
	{"YUVtoYUV422DC",	(PyCFunction)svc_YUVtoYUV422DC},
	{"YUVtoYUV422DC_quarter",(PyCFunction)svc_YUVtoYUV422DC_quarter},
	{"YUVtoYUV422DC_sixteenth",(PyCFunction)svc_YUVtoYUV422DC_sixteenth},
#ifdef USE_GL
	{"lrectwrite",		(PyCFunction)svc_lrectwrite},
#endif
	{"writefile",		(PyCFunction)svc_writefile},
	{NULL,			NULL} 		/* sentinel */
};

static void
capture_dealloc(captureobject *self)
{
	if (self->ob_capture != NULL) {
		if (self->ob_mustunlock)
			(void)svUnlockCaptureData(self->ob_svideo->ob_svideo,
						  self->ob_capture);
		self->ob_capture = NULL;
		Py_DECREF(self->ob_svideo);
		self->ob_svideo = NULL;
	}
	PyObject_Del(self);
}

static PyObject *
capture_getattr(svobject *self, char *name)
{
	return Py_FindMethod(capture_methods, (PyObject *)self, name);
}

PyTypeObject Capturetype = {
	PyObject_HEAD_INIT(&PyType_Type)
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

static PyObject *
newcaptureobject(svobject *self, void *ptr, int mustunlock)
{
	captureobject *p;

	p = PyObject_New(captureobject, &Capturetype);
	if (p == NULL)
		return NULL;
	p->ob_svideo = self;
	Py_INCREF(self);
	p->ob_capture = ptr;
	p->ob_mustunlock = mustunlock;
	p->ob_info = self->ob_info;
	return (PyObject *) p;
}

static PyObject *
sv_GetCaptureData(svobject *self, PyObject *args)
{
	void *ptr;
	long fieldID;
	PyObject *res, *c;

	if (!PyArg_Parse(args, ""))
		return NULL;

	if (svGetCaptureData(self->ob_svideo, &ptr, &fieldID))
		return sv_error();

	if (ptr == NULL) {
		PyErr_SetString(SvError, "no data available");
		return NULL;
	}

	c = newcaptureobject(self, ptr, 1);
	if (c == NULL)
		return NULL;
	res = Py_BuildValue("(Oi)", c, fieldID);
	Py_DECREF(c);
	return res;
}

static PyObject *
sv_BindGLWindow(svobject *self, PyObject *args)
{
	long wid;
	int mode;

	if (!PyArg_Parse(args, "(ii)", &wid, &mode))
		return NULL;

	if (svBindGLWindow(self->ob_svideo, wid, mode))
		return sv_error();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sv_EndContinuousCapture(svobject *self, PyObject *args)
{

	if (!PyArg_Parse(args, ""))
		return NULL;

	if (svEndContinuousCapture(self->ob_svideo))
		return sv_error();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sv_IsVideoDisplayed(svobject *self, PyObject *args)
{
	int v;

	if (!PyArg_Parse(args, ""))
		return NULL;

	v = svIsVideoDisplayed(self->ob_svideo);
	if (v == -1)
		return sv_error();

	return PyInt_FromLong((long) v);
}

static PyObject *
sv_OutputOffset(svobject *self, PyObject *args)
{
	int x_offset;
	int y_offset;

	if (!PyArg_Parse(args, "(ii)", &x_offset, &y_offset))
		return NULL;

	if (svOutputOffset(self->ob_svideo, x_offset, y_offset))
		return sv_error();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sv_PutFrame(svobject *self, PyObject *args)
{
	char *buffer;

	if (!PyArg_Parse(args, "s", &buffer))
		return NULL;

	if (svPutFrame(self->ob_svideo, buffer))
		return sv_error();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sv_QuerySize(svobject *self, PyObject *args)
{
	int w;
	int h;
	int rw;
	int rh;

	if (!PyArg_Parse(args, "(ii)", &w, &h))
		return NULL;

	if (svQuerySize(self->ob_svideo, w, h, &rw, &rh))
		return sv_error();

	return Py_BuildValue("(ii)", (long) rw, (long) rh);
}

static PyObject *
sv_SetSize(svobject *self, PyObject *args)
{
	int w;
	int h;

	if (!PyArg_Parse(args, "(ii)", &w, &h))
		return NULL;

	if (svSetSize(self->ob_svideo, w, h))
		return sv_error();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sv_SetStdDefaults(svobject *self, PyObject *args)
{

	if (!PyArg_Parse(args, ""))
		return NULL;

	if (svSetStdDefaults(self->ob_svideo))
		return sv_error();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sv_UseExclusive(svobject *self, PyObject *args)
{
	boolean onoff;
	int mode;

	if (!PyArg_Parse(args, "(ii)", &onoff, &mode))
		return NULL;

	if (svUseExclusive(self->ob_svideo, onoff, mode))
		return sv_error();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sv_WindowOffset(svobject *self, PyObject *args)
{
	int x_offset;
	int y_offset;

	if (!PyArg_Parse(args, "(ii)", &x_offset, &y_offset))
		return NULL;

	if (svWindowOffset(self->ob_svideo, x_offset, y_offset))
		return sv_error();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sv_CaptureBurst(svobject *self, PyObject *args)
{
	int bytes, i;
	svCaptureInfo info;
	void *bitvector = NULL;
	PyObject *videodata = NULL;
	PyObject *bitvecobj = NULL;
	PyObject *res = NULL;
	static PyObject *evenitem, *odditem;

	if (!PyArg_Parse(args, "(iiiii)", &info.format,
			 &info.width, &info.height,
			 &info.size, &info.samplingrate))
		return NULL;

	switch (info.format) {
	case SV_RGB8_FRAMES:
		bitvector = malloc(SV_BITVEC_SIZE(info.size));
		break;
	case SV_YUV411_FRAMES_AND_BLANKING_BUFFER:
		break;
	default:
		PyErr_SetString(SvError, "illegal format specified");
		return NULL;
	}

	if (svQueryCaptureBufferSize(self->ob_svideo, &info, &bytes)) {
		res = sv_error();
		goto finally;
	}

	if (!(videodata = PyString_FromStringAndSize(NULL, bytes)))
		goto finally;

	/* XXX -- need to do something about the bitvector */
	{
		char* str = PyString_AsString(videodata);
		if (!str)
			goto finally;
		
		if (svCaptureBurst(self->ob_svideo, &info, str, bitvector)) {
			res = sv_error();
			goto finally;
		}
	}

	if (bitvector) {
		if (evenitem == NULL) {
			if (!(evenitem = PyInt_FromLong(0)))
				goto finally;
		}
		if (odditem == NULL) {
			if (!(odditem = PyInt_FromLong(1)))
				goto finally;
		}
		if (!(bitvecobj = PyTuple_New(2 * info.size)))
			goto finally;

		for (i = 0; i < 2 * info.size; i++) {
			int sts;

			if (SV_GET_FIELD(bitvector, i) == SV_EVEN_FIELD) {
				Py_INCREF(evenitem);
				sts = PyTuple_SetItem(bitvecobj, i, evenitem);
			} else {
				Py_INCREF(odditem);
				sts = PyTuple_SetItem(bitvecobj, i, odditem);
			}
			if (sts < 0)
				goto finally;
		}
	} else {
		bitvecobj = Py_None;
		Py_INCREF(Py_None);
	}

	res = Py_BuildValue("((iiiii)OO)", info.format,
			    info.width, info.height,
			    info.size, info.samplingrate,
			    videodata, bitvecobj);

  finally:
	if (bitvector)
		free(bitvector);

	Py_XDECREF(videodata);
	Py_XDECREF(bitvecobj);
	return res;
}

static PyObject *
sv_CaptureOneFrame(svobject *self, PyObject *args)
{
	svCaptureInfo info;
	int format, width, height;
	int bytes;
	PyObject *videodata = NULL;
	PyObject *res = NULL;
	char *str;
	
	if (!PyArg_Parse(args, "(iii)", &format, &width, &height))
		return NULL;

	info.format = format;
	info.width = width;
	info.height = height;
	info.size = 0;
	info.samplingrate = 0;
	if (svQueryCaptureBufferSize(self->ob_svideo, &info, &bytes))
		return sv_error();

	if (!(videodata = PyString_FromStringAndSize(NULL, bytes)))
		return NULL;
	
	str = PyString_AsString(videodata);
	if (!str)
		goto finally;

	if (svCaptureOneFrame(self->ob_svideo, format, &width, &height, str)) {
		res = sv_error();
		goto finally;
	}

	res = Py_BuildValue("(iiO)", width, height, videodata);

  finally:
	Py_XDECREF(videodata);
	return res;
}

static PyObject *
sv_InitContinuousCapture(svobject *self, PyObject *args)
{
	svCaptureInfo info;

	if (!PyArg_Parse(args, "(iiiii)", &info.format,
			 &info.width, &info.height,
			 &info.size, &info.samplingrate))
		return NULL;

	if (svInitContinuousCapture(self->ob_svideo, &info))
		return sv_error();

	self->ob_info = info;

	return Py_BuildValue("(iiiii)", info.format, info.width, info.height,
			     info.size, info.samplingrate);
}

static PyObject *
sv_LoadMap(svobject *self, PyObject *args)
{
	PyObject *rgb;
	PyObject *res = NULL;
	rgb_tuple *mapp = NULL;
	int maptype;
	int i, j;			     /* indices */

	if (!PyArg_Parse(args, "(iO)", &maptype, &rgb))
		return NULL;

	if (!PyList_Check(rgb) || PyList_Size(rgb) != 256) {
		PyErr_BadArgument();
		return NULL;
	}

	if (!(mapp = PyMem_NEW(rgb_tuple, 256)))
		return PyErr_NoMemory();

	for (i = 0; i < 256; i++) {
		PyObject* v = PyList_GetItem(rgb, i);
		if (!v)
			goto finally;

		if (!PyTuple_Check(v) || PyTuple_Size(v) != 3) {
			PyErr_BadArgument();
			goto finally;
		}
		for (j = 0; j < 3; j++) {
			PyObject* cell = PyTuple_GetItem(v, j);
			if (!cell)
				goto finally;

			if (!PyInt_Check(cell)) {
				PyErr_BadArgument();
				goto finally;
			}
			switch (j) {
			case 0: mapp[i].red = PyInt_AsLong(cell); break;
			case 1: mapp[i].blue = PyInt_AsLong(cell); break;
			case 2: mapp[i].green = PyInt_AsLong(cell); break;
			}
			if (PyErr_Occurred())
				goto finally;
		}
	}

	if (svLoadMap(self->ob_svideo, maptype, mapp)) {
		res = sv_error();
		goto finally;
	}

	Py_INCREF(Py_None);
	res = Py_None;

  finally:
	PyMem_DEL(mapp);
	return res;
}
		
static PyObject *
sv_CloseVideo(svobject *self, PyObject *args)
{
	if (!PyArg_Parse(args, ""))
		return NULL;

	if (svCloseVideo(self->ob_svideo))
		return sv_error();

	self->ob_svideo = NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
doParams(svobject *self, PyObject *args,
         int (*func)(SV_nodeP, long *, int), int modified)
{
	PyObject *list;
	PyObject *res = NULL;
	long *PVbuffer = NULL;
	long length;
	int i;
	
	if (!PyArg_Parse(args, "O", &list))
		return NULL;

	if (!PyList_Check(list)) {
		PyErr_BadArgument();
		return NULL;
	}

	if ((length = PyList_Size(list)) < 0)
		return NULL;

	PVbuffer = PyMem_NEW(long, length);
	if (PVbuffer == NULL)
		return PyErr_NoMemory();

	for (i = 0; i < length; i++) {
		PyObject *v = PyList_GetItem(list, i);
		if (!v)
			goto finally;

		if (!PyInt_Check(v)) {
			PyErr_BadArgument();
			goto finally;
		}
		PVbuffer[i] = PyInt_AsLong(v);
		/* can't just test the return value, because what if the
		   value was -1?!
		*/
		if (PVbuffer[i] == -1 && PyErr_Occurred())
			goto finally;
	}

	if ((*func)(self->ob_svideo, PVbuffer, length)) {
		res = sv_error();
		goto finally;
	}

	if (modified) {
		for (i = 0; i < length; i++) {
			PyObject* v = PyInt_FromLong(PVbuffer[i]);
			if (!v || PyList_SetItem(list, i, v) < 0)
				goto finally;
		}
	}

	Py_INCREF(Py_None);
	res = Py_None;

  finally:
	PyMem_DEL(PVbuffer);
	return res;
}

static PyObject *
sv_GetParam(PyObject *self, PyObject *args)
{
	return doParams(self, args, svGetParam, 1);
}

static PyObject *
sv_GetParamRange(PyObject *self, PyObject *args)
{
	return doParams(self, args, svGetParamRange, 1);
}

static PyObject *
sv_SetParam(PyObject *self, PyObject *args)
{
	return doParams(self, args, svSetParam, 0);
}

static PyMethodDef svideo_methods[] = {
	{"BindGLWindow",	(PyCFunction)sv_BindGLWindow},
	{"EndContinuousCapture",(PyCFunction)sv_EndContinuousCapture},
	{"IsVideoDisplayed",	(PyCFunction)sv_IsVideoDisplayed},
	{"OutputOffset",	(PyCFunction)sv_OutputOffset},
	{"PutFrame",		(PyCFunction)sv_PutFrame},
	{"QuerySize",		(PyCFunction)sv_QuerySize},
	{"SetSize",		(PyCFunction)sv_SetSize},
	{"SetStdDefaults",	(PyCFunction)sv_SetStdDefaults},
	{"UseExclusive",	(PyCFunction)sv_UseExclusive},
	{"WindowOffset",	(PyCFunction)sv_WindowOffset},
	{"InitContinuousCapture",(PyCFunction)sv_InitContinuousCapture},
	{"CaptureBurst",	(PyCFunction)sv_CaptureBurst},
	{"CaptureOneFrame",	(PyCFunction)sv_CaptureOneFrame},
	{"GetCaptureData",	(PyCFunction)sv_GetCaptureData},
	{"CloseVideo",		(PyCFunction)sv_CloseVideo},
	{"LoadMap",		(PyCFunction)sv_LoadMap},
	{"GetParam",		(PyCFunction)sv_GetParam},
	{"GetParamRange",	(PyCFunction)sv_GetParamRange},
	{"SetParam",		(PyCFunction)sv_SetParam},
	{NULL,			NULL} 		/* sentinel */
};

static PyObject *
sv_conversion(PyObject *self, PyObject *args, void (*function)(),
              int inputfactor, float factor)
{
	int invert, width, height, inputlength;
	char *input, *str;
	PyObject *output;

	if (!PyArg_Parse(args, "(is#ii)", &invert,
			 &input, &inputlength, &width, &height))
		return NULL;

	if (width * height * inputfactor > inputlength) {
		PyErr_SetString(SvError, "input buffer not long enough");
		return NULL;
	}

	if (!(output = PyString_FromStringAndSize(NULL,
					      (int)(width * height * factor))))
		return NULL;

	str = PyString_AsString(output);
	if (!str) {
		Py_DECREF(output);
		return NULL;
	}
	(*function)(invert, input, str, width, height);

	return output;
}

static PyObject *
sv_InterleaveFields(PyObject *self, PyObject *args)
{
	return sv_conversion(self, args, svInterleaveFields, 1, 1.0);
}

static PyObject *
sv_RGB8toRGB32(PyObject *self, PyObject *args)
{
	return sv_conversion(self, args, svRGB8toRGB32, 1, (float) sizeof(long));
}

static PyObject *
sv_YUVtoRGB(PyObject *self, PyObject *args)
{
	return sv_conversion(self, args, svYUVtoRGB, 2, (float) sizeof(long));
}

static void
svideo_dealloc(svobject *self)
{
	if (self->ob_svideo != NULL)
		(void) svCloseVideo(self->ob_svideo);
	PyObject_Del(self);
}

static PyObject *
svideo_getattr(svobject *self, char *name)
{
	return Py_FindMethod(svideo_methods, (PyObject *)self, name);
}

PyTypeObject Svtype = {
	PyObject_HEAD_INIT(&PyType_Type)
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

static PyObject *
newsvobject(SV_nodeP svp)
{
	svobject *p;

	p = PyObject_New(svobject, &Svtype);
	if (p == NULL)
		return NULL;
	p->ob_svideo = svp;
	p->ob_info.format = 0;
	p->ob_info.size = 0;
	p->ob_info.width = 0;
	p->ob_info.height = 0;
	p->ob_info.samplingrate = 0;
	return (PyObject *) p;
}

static PyObject *
sv_OpenVideo(PyObject *self, PyObject *args)
{
	SV_nodeP svp;

	if (!PyArg_Parse(args, ""))
		return NULL;

	svp = svOpenVideo();
	if (svp == NULL)
		return sv_error();

	return newsvobject(svp);
}

static PyMethodDef sv_methods[] = {
	{"InterleaveFields",	(PyCFunction)sv_InterleaveFields},
	{"RGB8toRGB32",		(PyCFunction)sv_RGB8toRGB32},
	{"YUVtoRGB",		(PyCFunction)sv_YUVtoRGB},
	{"OpenVideo",		(PyCFunction)sv_OpenVideo},
	{NULL,			NULL}	/* Sentinel */
};

void
initsv(void)
{
	PyObject *m, *d;

	m = Py_InitModule("sv", sv_methods);
	d = PyModule_GetDict(m);

	SvError = PyErr_NewException("sv.error", NULL, NULL);
	if (SvError == NULL || PyDict_SetItemString(d, "error", SvError) != 0)
		return;
}
