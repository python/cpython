
/* ======================= Module HtmlRender ======================== */

#include "Python.h"



#define SystemSevenOrLater 1

#include "macglue.h"
#include <Memory.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Controls.h>

extern PyObject *ResObj_New(Handle);
extern int ResObj_Convert(PyObject *, Handle *);
extern PyObject *OptResObj_New(Handle);
extern int OptResObj_Convert(PyObject *, Handle *);

extern PyObject *WinObj_New(WindowPtr);
extern int WinObj_Convert(PyObject *, WindowPtr *);
extern PyTypeObject Window_Type;
#define WinObj_Check(x) ((x)->ob_type == &Window_Type)

extern PyObject *DlgObj_New(DialogPtr);
extern int DlgObj_Convert(PyObject *, DialogPtr *);
extern PyTypeObject Dialog_Type;
#define DlgObj_Check(x) ((x)->ob_type == &Dialog_Type)

extern PyObject *MenuObj_New(MenuHandle);
extern int MenuObj_Convert(PyObject *, MenuHandle *);

extern PyObject *CtlObj_New(ControlHandle);
extern int CtlObj_Convert(PyObject *, ControlHandle *);

extern PyObject *GrafObj_New(GrafPtr);
extern int GrafObj_Convert(PyObject *, GrafPtr *);

extern PyObject *BMObj_New(BitMapPtr);
extern int BMObj_Convert(PyObject *, BitMapPtr *);

extern PyObject *WinObj_WhichWindow(WindowPtr);

#include <HTMLRendering.h>

static PyObject *Html_Error;

/* --------------------- Object type HtmlObject --------------------- */

PyTypeObject HtmlObject_Type;

#define HtmlObj_Check(x) ((x)->ob_type == &HtmlObject_Type)

typedef struct HtmlObjectObject {
	PyObject_HEAD
	HRReference ob_itself;
} HtmlObjectObject;

PyObject *HtmlObj_New(itself)
	HRReference itself;
{
	HtmlObjectObject *it;
	it = PyObject_NEW(HtmlObjectObject, &HtmlObject_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
HtmlObj_Convert(v, p_itself)
	PyObject *v;
	HRReference *p_itself;
{
	if (!HtmlObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "HtmlObject required");
		return 0;
	}
	*p_itself = ((HtmlObjectObject *)v)->ob_itself;
	return 1;
}

static void HtmlObj_dealloc(self)
	HtmlObjectObject *self;
{
	/* Cleanup of self->ob_itself goes here */
	PyObject_DEL(self);
}

static PyObject *HtmlObj_HRDisposeReference(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HRDisposeReference(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRSetGrafPtr(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	GrafPtr grafPtr;
	if (!PyArg_ParseTuple(_args, "O&",
	                      GrafObj_Convert, &grafPtr))
		return NULL;
	_err = HRSetGrafPtr(_self->ob_itself,
	                    grafPtr);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRActivate(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HRActivate(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRDeactivate(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HRDeactivate(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRDraw(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	RgnHandle updateRgnH;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &updateRgnH))
		return NULL;
	_err = HRDraw(_self->ob_itself,
	              updateRgnH);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRSetRenderingRect(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect renderingRect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &renderingRect))
		return NULL;
	_err = HRSetRenderingRect(_self->ob_itself,
	                          &renderingRect);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRGetRenderedImageSize(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Point renderingSize;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HRGetRenderedImageSize(_self->ob_itself,
	                              &renderingSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, renderingSize);
	return _res;
}

static PyObject *HtmlObj_HRScrollToLocation(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Point location;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HRScrollToLocation(_self->ob_itself,
	                          &location);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, location);
	return _res;
}

static PyObject *HtmlObj_HRForceQuickdraw(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean forceQuickdraw;
	if (!PyArg_ParseTuple(_args, "b",
	                      &forceQuickdraw))
		return NULL;
	_err = HRForceQuickdraw(_self->ob_itself,
	                        forceQuickdraw);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRSetScrollbarState(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	HRScrollbarState hScrollbarState;
	HRScrollbarState vScrollbarState;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &hScrollbarState,
	                      &vScrollbarState))
		return NULL;
	_err = HRSetScrollbarState(_self->ob_itself,
	                           hScrollbarState,
	                           vScrollbarState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRSetDrawBorder(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean drawBorder;
	if (!PyArg_ParseTuple(_args, "b",
	                      &drawBorder))
		return NULL;
	_err = HRSetDrawBorder(_self->ob_itself,
	                       drawBorder);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRSetGrowboxCutout(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean allowCutout;
	if (!PyArg_ParseTuple(_args, "b",
	                      &allowCutout))
		return NULL;
	_err = HRSetGrowboxCutout(_self->ob_itself,
	                          allowCutout);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRGoToFile(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSSpec fsspec;
	Boolean addToHistory;
	Boolean forceRefresh;
	if (!PyArg_ParseTuple(_args, "O&bb",
	                      PyMac_GetFSSpec, &fsspec,
	                      &addToHistory,
	                      &forceRefresh))
		return NULL;
	_err = HRGoToFile(_self->ob_itself,
	                  &fsspec,
	                  addToHistory,
	                  forceRefresh);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRGoToURL(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	char * url;
	Boolean addToHistory;
	Boolean forceRefresh;
	if (!PyArg_ParseTuple(_args, "sbb",
	                      &url,
	                      &addToHistory,
	                      &forceRefresh))
		return NULL;
	_err = HRGoToURL(_self->ob_itself,
	                 url,
	                 addToHistory,
	                 forceRefresh);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRGoToAnchor(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	char * anchorName;
	if (!PyArg_ParseTuple(_args, "s",
	                      &anchorName))
		return NULL;
	_err = HRGoToAnchor(_self->ob_itself,
	                    anchorName);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRGoToPtr(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	char *buffer__in__;
	long buffer__len__;
	int buffer__in_len__;
	Boolean addToHistory;
	Boolean forceRefresh;
	if (!PyArg_ParseTuple(_args, "s#bb",
	                      &buffer__in__, &buffer__in_len__,
	                      &addToHistory,
	                      &forceRefresh))
		return NULL;
	buffer__len__ = buffer__in_len__;
	_err = HRGoToPtr(_self->ob_itself,
	                 buffer__in__, buffer__len__,
	                 addToHistory,
	                 forceRefresh);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
 buffer__error__: ;
	return _res;
}

static PyObject *HtmlObj_HRGetRootURL(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Handle rootURLH;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &rootURLH))
		return NULL;
	_err = HRGetRootURL(_self->ob_itself,
	                    rootURLH);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRGetBaseURL(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Handle baseURLH;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &baseURLH))
		return NULL;
	_err = HRGetBaseURL(_self->ob_itself,
	                    baseURLH);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRGetHTMLURL(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Handle HTMLURLH;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &HTMLURLH))
		return NULL;
	_err = HRGetHTMLURL(_self->ob_itself,
	                    HTMLURLH);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRGetTitle(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	StringPtr title;
	if (!PyArg_ParseTuple(_args, "s",
	                      &title))
		return NULL;
	_err = HRGetTitle(_self->ob_itself,
	                  title);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRGetHTMLFile(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSSpec fsspec;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HRGetHTMLFile(_self->ob_itself,
	                     &fsspec);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFSSpec, fsspec);
	return _res;
}

static PyObject *HtmlObj_HRUnregisterWasURLVisitedUPP(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	HRUnregisterWasURLVisitedUPP(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRUnregisterNewURLUPP(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	HRUnregisterNewURLUPP(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *HtmlObj_HRUnregisterURLToFSSpecUPP(_self, _args)
	HtmlObjectObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	HRUnregisterURLToFSSpecUPP(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef HtmlObj_methods[] = {
	{"HRDisposeReference", (PyCFunction)HtmlObj_HRDisposeReference, 1,
	 "() -> None"},
	{"HRSetGrafPtr", (PyCFunction)HtmlObj_HRSetGrafPtr, 1,
	 "(GrafPtr grafPtr) -> None"},
	{"HRActivate", (PyCFunction)HtmlObj_HRActivate, 1,
	 "() -> None"},
	{"HRDeactivate", (PyCFunction)HtmlObj_HRDeactivate, 1,
	 "() -> None"},
	{"HRDraw", (PyCFunction)HtmlObj_HRDraw, 1,
	 "(RgnHandle updateRgnH) -> None"},
	{"HRSetRenderingRect", (PyCFunction)HtmlObj_HRSetRenderingRect, 1,
	 "(Rect renderingRect) -> None"},
	{"HRGetRenderedImageSize", (PyCFunction)HtmlObj_HRGetRenderedImageSize, 1,
	 "() -> (Point renderingSize)"},
	{"HRScrollToLocation", (PyCFunction)HtmlObj_HRScrollToLocation, 1,
	 "() -> (Point location)"},
	{"HRForceQuickdraw", (PyCFunction)HtmlObj_HRForceQuickdraw, 1,
	 "(Boolean forceQuickdraw) -> None"},
	{"HRSetScrollbarState", (PyCFunction)HtmlObj_HRSetScrollbarState, 1,
	 "(HRScrollbarState hScrollbarState, HRScrollbarState vScrollbarState) -> None"},
	{"HRSetDrawBorder", (PyCFunction)HtmlObj_HRSetDrawBorder, 1,
	 "(Boolean drawBorder) -> None"},
	{"HRSetGrowboxCutout", (PyCFunction)HtmlObj_HRSetGrowboxCutout, 1,
	 "(Boolean allowCutout) -> None"},
	{"HRGoToFile", (PyCFunction)HtmlObj_HRGoToFile, 1,
	 "(FSSpec fsspec, Boolean addToHistory, Boolean forceRefresh) -> None"},
	{"HRGoToURL", (PyCFunction)HtmlObj_HRGoToURL, 1,
	 "(char * url, Boolean addToHistory, Boolean forceRefresh) -> None"},
	{"HRGoToAnchor", (PyCFunction)HtmlObj_HRGoToAnchor, 1,
	 "(char * anchorName) -> None"},
	{"HRGoToPtr", (PyCFunction)HtmlObj_HRGoToPtr, 1,
	 "(Buffer buffer, Boolean addToHistory, Boolean forceRefresh) -> None"},
	{"HRGetRootURL", (PyCFunction)HtmlObj_HRGetRootURL, 1,
	 "(Handle rootURLH) -> None"},
	{"HRGetBaseURL", (PyCFunction)HtmlObj_HRGetBaseURL, 1,
	 "(Handle baseURLH) -> None"},
	{"HRGetHTMLURL", (PyCFunction)HtmlObj_HRGetHTMLURL, 1,
	 "(Handle HTMLURLH) -> None"},
	{"HRGetTitle", (PyCFunction)HtmlObj_HRGetTitle, 1,
	 "(StringPtr title) -> None"},
	{"HRGetHTMLFile", (PyCFunction)HtmlObj_HRGetHTMLFile, 1,
	 "() -> (FSSpec fsspec)"},
	{"HRUnregisterWasURLVisitedUPP", (PyCFunction)HtmlObj_HRUnregisterWasURLVisitedUPP, 1,
	 "() -> None"},
	{"HRUnregisterNewURLUPP", (PyCFunction)HtmlObj_HRUnregisterNewURLUPP, 1,
	 "() -> None"},
	{"HRUnregisterURLToFSSpecUPP", (PyCFunction)HtmlObj_HRUnregisterURLToFSSpecUPP, 1,
	 "() -> None"},
	{NULL, NULL, 0}
};

PyMethodChain HtmlObj_chain = { HtmlObj_methods, NULL };

static PyObject *HtmlObj_getattr(self, name)
	HtmlObjectObject *self;
	char *name;
{
	return Py_FindMethodInChain(&HtmlObj_chain, (PyObject *)self, name);
}

#define HtmlObj_setattr NULL

#define HtmlObj_compare NULL

#define HtmlObj_repr NULL

#define HtmlObj_hash NULL

PyTypeObject HtmlObject_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"HtmlRender.HtmlObject", /*tp_name*/
	sizeof(HtmlObjectObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) HtmlObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) HtmlObj_getattr, /*tp_getattr*/
	(setattrfunc) HtmlObj_setattr, /*tp_setattr*/
	(cmpfunc) HtmlObj_compare, /*tp_compare*/
	(reprfunc) HtmlObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) HtmlObj_hash, /*tp_hash*/
};

/* ------------------- End object type HtmlObject ------------------- */


static PyObject *Html_HRGetHTMLRenderingLibVersion(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	NumVersion returnVers;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HRGetHTMLRenderingLibVersion(&returnVers);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildNumVersion, returnVers);
	return _res;
}

static PyObject *Html_HRNewReference(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	HRReference hrRef;
	OSType rendererType;
	GrafPtr grafPtr;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &rendererType,
	                      GrafObj_Convert, &grafPtr))
		return NULL;
	_err = HRNewReference(&hrRef,
	                      rendererType,
	                      grafPtr);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     HtmlObj_New, hrRef);
	return _res;
}

static PyObject *Html_HRFreeMemory(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt32 _rv;
	Size inBytesNeeded;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inBytesNeeded))
		return NULL;
	_rv = HRFreeMemory(inBytesNeeded);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Html_HRScreenConfigurationChanged(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	HRScreenConfigurationChanged();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Html_HRIsHREvent(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord eventRecord;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &eventRecord))
		return NULL;
	_rv = HRIsHREvent(&eventRecord);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Html_HRUtilCreateFullURL(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	char * rootURL;
	char * linkURL;
	Handle fullURLH;
	if (!PyArg_ParseTuple(_args, "ssO&",
	                      &rootURL,
	                      &linkURL,
	                      ResObj_Convert, &fullURLH))
		return NULL;
	_err = HRUtilCreateFullURL(rootURL,
	                           linkURL,
	                           fullURLH);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Html_HRUtilGetFSSpecFromURL(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	char * rootURL;
	char * linkURL;
	FSSpec destSpec;
	if (!PyArg_ParseTuple(_args, "ss",
	                      &rootURL,
	                      &linkURL))
		return NULL;
	_err = HRUtilGetFSSpecFromURL(rootURL,
	                              linkURL,
	                              &destSpec);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFSSpec, destSpec);
	return _res;
}

static PyObject *Html_HRUtilGetURLFromFSSpec(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSSpec fsspec;
	Handle urlHandle;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFSSpec, &fsspec,
	                      ResObj_Convert, &urlHandle))
		return NULL;
	_err = HRUtilGetURLFromFSSpec(&fsspec,
	                              urlHandle);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Html_HRHTMLRenderingLibAvailable(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	int _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = HRHTMLRenderingLibAvailable();
	_res = Py_BuildValue("i",
	                     _rv);
	return _res;
}

static PyMethodDef Html_methods[] = {
	{"HRGetHTMLRenderingLibVersion", (PyCFunction)Html_HRGetHTMLRenderingLibVersion, 1,
	 "() -> (NumVersion returnVers)"},
	{"HRNewReference", (PyCFunction)Html_HRNewReference, 1,
	 "(OSType rendererType, GrafPtr grafPtr) -> (HRReference hrRef)"},
	{"HRFreeMemory", (PyCFunction)Html_HRFreeMemory, 1,
	 "(Size inBytesNeeded) -> (SInt32 _rv)"},
	{"HRScreenConfigurationChanged", (PyCFunction)Html_HRScreenConfigurationChanged, 1,
	 "() -> None"},
	{"HRIsHREvent", (PyCFunction)Html_HRIsHREvent, 1,
	 "(EventRecord eventRecord) -> (Boolean _rv)"},
	{"HRUtilCreateFullURL", (PyCFunction)Html_HRUtilCreateFullURL, 1,
	 "(char * rootURL, char * linkURL, Handle fullURLH) -> None"},
	{"HRUtilGetFSSpecFromURL", (PyCFunction)Html_HRUtilGetFSSpecFromURL, 1,
	 "(char * rootURL, char * linkURL) -> (FSSpec destSpec)"},
	{"HRUtilGetURLFromFSSpec", (PyCFunction)Html_HRUtilGetURLFromFSSpec, 1,
	 "(FSSpec fsspec, Handle urlHandle) -> None"},
	{"HRHTMLRenderingLibAvailable", (PyCFunction)Html_HRHTMLRenderingLibAvailable, 1,
	 "() -> (int _rv)"},
	{NULL, NULL, 0}
};




void initHtmlRender()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("HtmlRender", Html_methods);
	d = PyModule_GetDict(m);
	Html_Error = PyMac_GetOSErrException();
	if (Html_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Html_Error) != 0)
		Py_FatalError("can't initialize HtmlRender.Error");
	HtmlObject_Type.ob_type = &PyType_Type;
	Py_INCREF(&HtmlObject_Type);
	if (PyDict_SetItemString(d, "HtmlObjectType", (PyObject *)&HtmlObject_Type) != 0)
		Py_FatalError("can't initialize HtmlObjectType");
}

/* ===================== End module HtmlRender ====================== */

