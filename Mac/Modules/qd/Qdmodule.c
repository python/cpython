
/* =========================== Module Qd ============================ */

#include "Python.h"



#define SystemSevenOrLater 1

#include "macglue.h"
#include <Memory.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Controls.h>

extern PyObject *ResObj_New(Handle);
extern int ResObj_Convert(PyObject *, Handle *);

extern PyObject *WinObj_New(WindowPtr);
extern int WinObj_Convert(PyObject *, WindowPtr *);

extern PyObject *DlgObj_New(DialogPtr);
extern int DlgObj_Convert(PyObject *, DialogPtr *);
extern PyTypeObject Dialog_Type;
#define DlgObj_Check(x) ((x)->ob_type == &Dialog_Type)

extern PyObject *MenuObj_New(MenuHandle);
extern int MenuObj_Convert(PyObject *, MenuHandle *);

extern PyObject *CtlObj_New(ControlHandle);
extern int CtlObj_Convert(PyObject *, ControlHandle *);

extern PyObject *WinObj_WhichWindow(WindowPtr);

#include <QuickDraw.h>
#include <Desk.h>

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

static PyObject *Qd_Error;

static PyObject *Qd_OpenPort(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr port;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &port))
		return NULL;
	OpenPort(port);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_InitPort(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr port;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &port))
		return NULL;
	InitPort(port);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ClosePort(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr port;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &port))
		return NULL;
	ClosePort(port);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_SetPort(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr port;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &port))
		return NULL;
	SetPort(port);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_GetPort(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr port;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetPort(&port);
	_res = Py_BuildValue("O&",
	                     WinObj_New, port);
	return _res;
}

static PyObject *Qd_GrafDevice(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short device;
	if (!PyArg_ParseTuple(_args, "h",
	                      &device))
		return NULL;
	GrafDevice(device);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_PortSize(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short width;
	short height;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &width,
	                      &height))
		return NULL;
	PortSize(width,
	         height);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_MovePortTo(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short leftGlobal;
	short topGlobal;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &leftGlobal,
	                      &topGlobal))
		return NULL;
	MovePortTo(leftGlobal,
	           topGlobal);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_SetOrigin(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short h;
	short v;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &h,
	                      &v))
		return NULL;
	SetOrigin(h,
	          v);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_SetClip(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &rgn))
		return NULL;
	SetClip(rgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_GetClip(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &rgn))
		return NULL;
	GetClip(rgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ClipRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	ClipRect(&r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_InitCursor(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	InitCursor();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_HideCursor(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	HideCursor();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ShowCursor(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ShowCursor();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ObscureCursor(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ObscureCursor();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_HidePen(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	HidePen();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ShowPen(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ShowPen();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_GetPen(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point pt;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &pt))
		return NULL;
	GetPen(&pt);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, pt);
	return _res;
}

static PyObject *Qd_PenSize(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short width;
	short height;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &width,
	                      &height))
		return NULL;
	PenSize(width,
	        height);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_PenMode(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short mode;
	if (!PyArg_ParseTuple(_args, "h",
	                      &mode))
		return NULL;
	PenMode(mode);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_PenNormal(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	PenNormal();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_MoveTo(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short h;
	short v;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &h,
	                      &v))
		return NULL;
	MoveTo(h,
	       v);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_Move(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short dh;
	short dv;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &dh,
	                      &dv))
		return NULL;
	Move(dh,
	     dv);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_LineTo(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short h;
	short v;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &h,
	                      &v))
		return NULL;
	LineTo(h,
	       v);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_Line(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short dh;
	short dv;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &dh,
	                      &dv))
		return NULL;
	Line(dh,
	     dv);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ForeColor(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long color;
	if (!PyArg_ParseTuple(_args, "l",
	                      &color))
		return NULL;
	ForeColor(color);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_BackColor(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long color;
	if (!PyArg_ParseTuple(_args, "l",
	                      &color))
		return NULL;
	BackColor(color);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ColorBit(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short whichBit;
	if (!PyArg_ParseTuple(_args, "h",
	                      &whichBit))
		return NULL;
	ColorBit(whichBit);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_SetRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short left;
	short top;
	short right;
	short bottom;
	if (!PyArg_ParseTuple(_args, "hhhh",
	                      &left,
	                      &top,
	                      &right,
	                      &bottom))
		return NULL;
	SetRect(&r,
	        left,
	        top,
	        right,
	        bottom);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &r);
	return _res;
}

static PyObject *Qd_OffsetRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short dh;
	short dv;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &dh,
	                      &dv))
		return NULL;
	OffsetRect(&r,
	           dh,
	           dv);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &r);
	return _res;
}

static PyObject *Qd_InsetRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short dh;
	short dv;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &dh,
	                      &dv))
		return NULL;
	InsetRect(&r,
	          dh,
	          dv);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &r);
	return _res;
}

static PyObject *Qd_SectRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Rect src1;
	Rect src2;
	Rect dstRect;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &src1,
	                      PyMac_GetRect, &src2))
		return NULL;
	_rv = SectRect(&src1,
	               &src2,
	               &dstRect);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     PyMac_BuildRect, &dstRect);
	return _res;
}

static PyObject *Qd_UnionRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect src1;
	Rect src2;
	Rect dstRect;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &src1,
	                      PyMac_GetRect, &src2))
		return NULL;
	UnionRect(&src1,
	          &src2,
	          &dstRect);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &dstRect);
	return _res;
}

static PyObject *Qd_EqualRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Rect rect1;
	Rect rect2;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &rect1,
	                      PyMac_GetRect, &rect2))
		return NULL;
	_rv = EqualRect(&rect1,
	                &rect2);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Qd_EmptyRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	_rv = EmptyRect(&r);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Qd_FrameRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	FrameRect(&r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_PaintRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	PaintRect(&r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_EraseRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	EraseRect(&r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_InvertRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	InvertRect(&r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_FrameOval(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	FrameOval(&r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_PaintOval(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	PaintOval(&r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_EraseOval(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	EraseOval(&r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_InvertOval(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	InvertOval(&r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_FrameRoundRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short ovalWidth;
	short ovalHeight;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      PyMac_GetRect, &r,
	                      &ovalWidth,
	                      &ovalHeight))
		return NULL;
	FrameRoundRect(&r,
	               ovalWidth,
	               ovalHeight);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_PaintRoundRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short ovalWidth;
	short ovalHeight;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      PyMac_GetRect, &r,
	                      &ovalWidth,
	                      &ovalHeight))
		return NULL;
	PaintRoundRect(&r,
	               ovalWidth,
	               ovalHeight);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_EraseRoundRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short ovalWidth;
	short ovalHeight;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      PyMac_GetRect, &r,
	                      &ovalWidth,
	                      &ovalHeight))
		return NULL;
	EraseRoundRect(&r,
	               ovalWidth,
	               ovalHeight);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_InvertRoundRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short ovalWidth;
	short ovalHeight;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      PyMac_GetRect, &r,
	                      &ovalWidth,
	                      &ovalHeight))
		return NULL;
	InvertRoundRect(&r,
	                ovalWidth,
	                ovalHeight);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_FrameArc(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short startAngle;
	short arcAngle;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      PyMac_GetRect, &r,
	                      &startAngle,
	                      &arcAngle))
		return NULL;
	FrameArc(&r,
	         startAngle,
	         arcAngle);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_PaintArc(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short startAngle;
	short arcAngle;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      PyMac_GetRect, &r,
	                      &startAngle,
	                      &arcAngle))
		return NULL;
	PaintArc(&r,
	         startAngle,
	         arcAngle);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_EraseArc(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short startAngle;
	short arcAngle;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      PyMac_GetRect, &r,
	                      &startAngle,
	                      &arcAngle))
		return NULL;
	EraseArc(&r,
	         startAngle,
	         arcAngle);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_InvertArc(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short startAngle;
	short arcAngle;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      PyMac_GetRect, &r,
	                      &startAngle,
	                      &arcAngle))
		return NULL;
	InvertArc(&r,
	          startAngle,
	          arcAngle);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_NewRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = NewRgn();
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Qd_OpenRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	OpenRgn();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_CloseRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle dstRgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &dstRgn))
		return NULL;
	CloseRgn(dstRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_DisposeRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &rgn))
		return NULL;
	DisposeRgn(rgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_CopyRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle srcRgn;
	RgnHandle dstRgn;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &srcRgn,
	                      ResObj_Convert, &dstRgn))
		return NULL;
	CopyRgn(srcRgn,
	        dstRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_SetEmptyRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &rgn))
		return NULL;
	SetEmptyRgn(rgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_SetRectRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	short left;
	short top;
	short right;
	short bottom;
	if (!PyArg_ParseTuple(_args, "O&hhhh",
	                      ResObj_Convert, &rgn,
	                      &left,
	                      &top,
	                      &right,
	                      &bottom))
		return NULL;
	SetRectRgn(rgn,
	           left,
	           top,
	           right,
	           bottom);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_RectRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &rgn,
	                      PyMac_GetRect, &r))
		return NULL;
	RectRgn(rgn,
	        &r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_OffsetRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	short dh;
	short dv;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      ResObj_Convert, &rgn,
	                      &dh,
	                      &dv))
		return NULL;
	OffsetRgn(rgn,
	          dh,
	          dv);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_InsetRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	short dh;
	short dv;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      ResObj_Convert, &rgn,
	                      &dh,
	                      &dv))
		return NULL;
	InsetRgn(rgn,
	         dh,
	         dv);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_SectRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle srcRgnA;
	RgnHandle srcRgnB;
	RgnHandle dstRgn;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &srcRgnA,
	                      ResObj_Convert, &srcRgnB,
	                      ResObj_Convert, &dstRgn))
		return NULL;
	SectRgn(srcRgnA,
	        srcRgnB,
	        dstRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_UnionRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle srcRgnA;
	RgnHandle srcRgnB;
	RgnHandle dstRgn;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &srcRgnA,
	                      ResObj_Convert, &srcRgnB,
	                      ResObj_Convert, &dstRgn))
		return NULL;
	UnionRgn(srcRgnA,
	         srcRgnB,
	         dstRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_DiffRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle srcRgnA;
	RgnHandle srcRgnB;
	RgnHandle dstRgn;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &srcRgnA,
	                      ResObj_Convert, &srcRgnB,
	                      ResObj_Convert, &dstRgn))
		return NULL;
	DiffRgn(srcRgnA,
	        srcRgnB,
	        dstRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_XorRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle srcRgnA;
	RgnHandle srcRgnB;
	RgnHandle dstRgn;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &srcRgnA,
	                      ResObj_Convert, &srcRgnB,
	                      ResObj_Convert, &dstRgn))
		return NULL;
	XorRgn(srcRgnA,
	       srcRgnB,
	       dstRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_RectInRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Rect r;
	RgnHandle rgn;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &r,
	                      ResObj_Convert, &rgn))
		return NULL;
	_rv = RectInRgn(&r,
	                rgn);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Qd_EqualRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	RgnHandle rgnA;
	RgnHandle rgnB;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &rgnA,
	                      ResObj_Convert, &rgnB))
		return NULL;
	_rv = EqualRgn(rgnA,
	               rgnB);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Qd_EmptyRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	RgnHandle rgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &rgn))
		return NULL;
	_rv = EmptyRgn(rgn);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Qd_FrameRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &rgn))
		return NULL;
	FrameRgn(rgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_PaintRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &rgn))
		return NULL;
	PaintRgn(rgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_EraseRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &rgn))
		return NULL;
	EraseRgn(rgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_InvertRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &rgn))
		return NULL;
	InvertRgn(rgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ScrollRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short dh;
	short dv;
	RgnHandle updateRgn;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &r,
	                      &dh,
	                      &dv,
	                      ResObj_Convert, &updateRgn))
		return NULL;
	ScrollRect(&r,
	           dh,
	           dv,
	           updateRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_OpenPicture(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PicHandle _rv;
	Rect picFrame;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &picFrame))
		return NULL;
	_rv = OpenPicture(&picFrame);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Qd_PicComment(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short kind;
	short dataSize;
	Handle dataHandle;
	if (!PyArg_ParseTuple(_args, "hhO&",
	                      &kind,
	                      &dataSize,
	                      ResObj_Convert, &dataHandle))
		return NULL;
	PicComment(kind,
	           dataSize,
	           dataHandle);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ClosePicture(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ClosePicture();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_DrawPicture(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PicHandle myPicture;
	Rect dstRect;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &myPicture,
	                      PyMac_GetRect, &dstRect))
		return NULL;
	DrawPicture(myPicture,
	            &dstRect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_KillPicture(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PicHandle myPicture;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &myPicture))
		return NULL;
	KillPicture(myPicture);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_OpenPoly(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PolyHandle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = OpenPoly();
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Qd_ClosePoly(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ClosePoly();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_KillPoly(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PolyHandle poly;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &poly))
		return NULL;
	KillPoly(poly);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_OffsetPoly(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PolyHandle poly;
	short dh;
	short dv;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      ResObj_Convert, &poly,
	                      &dh,
	                      &dv))
		return NULL;
	OffsetPoly(poly,
	           dh,
	           dv);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_FramePoly(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PolyHandle poly;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &poly))
		return NULL;
	FramePoly(poly);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_PaintPoly(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PolyHandle poly;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &poly))
		return NULL;
	PaintPoly(poly);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ErasePoly(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PolyHandle poly;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &poly))
		return NULL;
	ErasePoly(poly);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_InvertPoly(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PolyHandle poly;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &poly))
		return NULL;
	InvertPoly(poly);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_SetPt(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point pt;
	short h;
	short v;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      PyMac_GetPoint, &pt,
	                      &h,
	                      &v))
		return NULL;
	SetPt(&pt,
	      h,
	      v);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, pt);
	return _res;
}

static PyObject *Qd_LocalToGlobal(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point pt;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &pt))
		return NULL;
	LocalToGlobal(&pt);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, pt);
	return _res;
}

static PyObject *Qd_GlobalToLocal(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point pt;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &pt))
		return NULL;
	GlobalToLocal(&pt);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, pt);
	return _res;
}

static PyObject *Qd_Random(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = Random();
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Qd_GetPixel(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	short h;
	short v;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &h,
	                      &v))
		return NULL;
	_rv = GetPixel(h,
	               v);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Qd_ScalePt(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point pt;
	Rect srcRect;
	Rect dstRect;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      PyMac_GetPoint, &pt,
	                      PyMac_GetRect, &srcRect,
	                      PyMac_GetRect, &dstRect))
		return NULL;
	ScalePt(&pt,
	        &srcRect,
	        &dstRect);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, pt);
	return _res;
}

static PyObject *Qd_MapPt(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point pt;
	Rect srcRect;
	Rect dstRect;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      PyMac_GetPoint, &pt,
	                      PyMac_GetRect, &srcRect,
	                      PyMac_GetRect, &dstRect))
		return NULL;
	MapPt(&pt,
	      &srcRect,
	      &dstRect);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, pt);
	return _res;
}

static PyObject *Qd_MapRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	Rect srcRect;
	Rect dstRect;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &srcRect,
	                      PyMac_GetRect, &dstRect))
		return NULL;
	MapRect(&r,
	        &srcRect,
	        &dstRect);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &r);
	return _res;
}

static PyObject *Qd_MapRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	Rect srcRect;
	Rect dstRect;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &rgn,
	                      PyMac_GetRect, &srcRect,
	                      PyMac_GetRect, &dstRect))
		return NULL;
	MapRgn(rgn,
	       &srcRect,
	       &dstRect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_MapPoly(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PolyHandle poly;
	Rect srcRect;
	Rect dstRect;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &poly,
	                      PyMac_GetRect, &srcRect,
	                      PyMac_GetRect, &dstRect))
		return NULL;
	MapPoly(poly,
	        &srcRect,
	        &dstRect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_AddPt(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point src;
	Point dst;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &src,
	                      PyMac_GetPoint, &dst))
		return NULL;
	AddPt(src,
	      &dst);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, dst);
	return _res;
}

static PyObject *Qd_EqualPt(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point pt1;
	Point pt2;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &pt1,
	                      PyMac_GetPoint, &pt2))
		return NULL;
	_rv = EqualPt(pt1,
	              pt2);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Qd_PtInRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point pt;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &pt,
	                      PyMac_GetRect, &r))
		return NULL;
	_rv = PtInRect(pt,
	               &r);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Qd_Pt2Rect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point pt1;
	Point pt2;
	Rect dstRect;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &pt1,
	                      PyMac_GetPoint, &pt2))
		return NULL;
	Pt2Rect(pt1,
	        pt2,
	        &dstRect);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &dstRect);
	return _res;
}

static PyObject *Qd_PtToAngle(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	Point pt;
	short angle;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &r,
	                      PyMac_GetPoint, &pt))
		return NULL;
	PtToAngle(&r,
	          pt,
	          &angle);
	_res = Py_BuildValue("h",
	                     angle);
	return _res;
}

static PyObject *Qd_SubPt(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point src;
	Point dst;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &src,
	                      PyMac_GetPoint, &dst))
		return NULL;
	SubPt(src,
	      &dst);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, dst);
	return _res;
}

static PyObject *Qd_PtInRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point pt;
	RgnHandle rgn;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &pt,
	                      ResObj_Convert, &rgn))
		return NULL;
	_rv = PtInRgn(pt,
	              rgn);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Qd_NewPixMap(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PixMapHandle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = NewPixMap();
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Qd_DisposePixMap(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PixMapHandle pm;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pm))
		return NULL;
	DisposePixMap(pm);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_CopyPixMap(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PixMapHandle srcPM;
	PixMapHandle dstPM;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &srcPM,
	                      ResObj_Convert, &dstPM))
		return NULL;
	CopyPixMap(srcPM,
	           dstPM);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_NewPixPat(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PixPatHandle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = NewPixPat();
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Qd_DisposePixPat(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PixPatHandle pp;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pp))
		return NULL;
	DisposePixPat(pp);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_CopyPixPat(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PixPatHandle srcPP;
	PixPatHandle dstPP;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &srcPP,
	                      ResObj_Convert, &dstPP))
		return NULL;
	CopyPixPat(srcPP,
	           dstPP);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_PenPixPat(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PixPatHandle pp;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pp))
		return NULL;
	PenPixPat(pp);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_BackPixPat(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PixPatHandle pp;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pp))
		return NULL;
	BackPixPat(pp);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_GetPixPat(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PixPatHandle _rv;
	short patID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &patID))
		return NULL;
	_rv = GetPixPat(patID);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Qd_FillCRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	PixPatHandle pp;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &r,
	                      ResObj_Convert, &pp))
		return NULL;
	FillCRect(&r,
	          pp);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_FillCOval(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	PixPatHandle pp;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &r,
	                      ResObj_Convert, &pp))
		return NULL;
	FillCOval(&r,
	          pp);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_FillCRoundRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short ovalWidth;
	short ovalHeight;
	PixPatHandle pp;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &r,
	                      &ovalWidth,
	                      &ovalHeight,
	                      ResObj_Convert, &pp))
		return NULL;
	FillCRoundRect(&r,
	               ovalWidth,
	               ovalHeight,
	               pp);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_FillCArc(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	short startAngle;
	short arcAngle;
	PixPatHandle pp;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &r,
	                      &startAngle,
	                      &arcAngle,
	                      ResObj_Convert, &pp))
		return NULL;
	FillCArc(&r,
	         startAngle,
	         arcAngle,
	         pp);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_FillCRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle rgn;
	PixPatHandle pp;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &rgn,
	                      ResObj_Convert, &pp))
		return NULL;
	FillCRgn(rgn,
	         pp);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_FillCPoly(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PolyHandle poly;
	PixPatHandle pp;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &poly,
	                      ResObj_Convert, &pp))
		return NULL;
	FillCPoly(poly,
	          pp);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_SetPortPix(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PixMapHandle pm;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pm))
		return NULL;
	SetPortPix(pm);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_AllocCursor(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	AllocCursor();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_GetCTSeed(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetCTSeed();
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qd_SetClientID(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short id;
	if (!PyArg_ParseTuple(_args, "h",
	                      &id))
		return NULL;
	SetClientID(id);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ProtectEntry(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short index;
	Boolean protect;
	if (!PyArg_ParseTuple(_args, "hb",
	                      &index,
	                      &protect))
		return NULL;
	ProtectEntry(index,
	             protect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_ReserveEntry(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short index;
	Boolean reserve;
	if (!PyArg_ParseTuple(_args, "hb",
	                      &index,
	                      &reserve))
		return NULL;
	ReserveEntry(index,
	             reserve);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_QDError(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = QDError();
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Qd_TextFont(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short font;
	if (!PyArg_ParseTuple(_args, "h",
	                      &font))
		return NULL;
	TextFont(font);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_TextFace(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short face;
	if (!PyArg_ParseTuple(_args, "h",
	                      &face))
		return NULL;
	TextFace(face);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_TextMode(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short mode;
	if (!PyArg_ParseTuple(_args, "h",
	                      &mode))
		return NULL;
	TextMode(mode);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_TextSize(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short size;
	if (!PyArg_ParseTuple(_args, "h",
	                      &size))
		return NULL;
	TextSize(size);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_SpaceExtra(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long extra;
	if (!PyArg_ParseTuple(_args, "l",
	                      &extra))
		return NULL;
	SpaceExtra(extra);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_DrawChar(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short ch;
	if (!PyArg_ParseTuple(_args, "h",
	                      &ch))
		return NULL;
	DrawChar(ch);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_DrawString(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 s;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, s))
		return NULL;
	DrawString(s);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qd_DrawText(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	char *textBuf__in__;
	int textBuf__len__;
	int textBuf__in_len__;
	short firstByte;
	short byteCount;
	if (!PyArg_ParseTuple(_args, "s#hh",
	                      &textBuf__in__, &textBuf__in_len__,
	                      &firstByte,
	                      &byteCount))
		return NULL;
	DrawText(textBuf__in__,
	         firstByte,
	         byteCount);
	Py_INCREF(Py_None);
	_res = Py_None;
 textBuf__error__: ;
	return _res;
}

static PyObject *Qd_CharWidth(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	short ch;
	if (!PyArg_ParseTuple(_args, "h",
	                      &ch))
		return NULL;
	_rv = CharWidth(ch);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Qd_StringWidth(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	Str255 s;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, s))
		return NULL;
	_rv = StringWidth(s);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Qd_TextWidth(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	char *textBuf__in__;
	int textBuf__len__;
	int textBuf__in_len__;
	short firstByte;
	short byteCount;
	if (!PyArg_ParseTuple(_args, "s#hh",
	                      &textBuf__in__, &textBuf__in_len__,
	                      &firstByte,
	                      &byteCount))
		return NULL;
	_rv = TextWidth(textBuf__in__,
	                firstByte,
	                byteCount);
	_res = Py_BuildValue("h",
	                     _rv);
 textBuf__error__: ;
	return _res;
}

static PyObject *Qd_CharExtra(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long extra;
	if (!PyArg_ParseTuple(_args, "l",
	                      &extra))
		return NULL;
	CharExtra(extra);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef Qd_methods[] = {
	{"OpenPort", (PyCFunction)Qd_OpenPort, 1,
	 "(WindowPtr port) -> None"},
	{"InitPort", (PyCFunction)Qd_InitPort, 1,
	 "(WindowPtr port) -> None"},
	{"ClosePort", (PyCFunction)Qd_ClosePort, 1,
	 "(WindowPtr port) -> None"},
	{"SetPort", (PyCFunction)Qd_SetPort, 1,
	 "(WindowPtr port) -> None"},
	{"GetPort", (PyCFunction)Qd_GetPort, 1,
	 "() -> (WindowPtr port)"},
	{"GrafDevice", (PyCFunction)Qd_GrafDevice, 1,
	 "(short device) -> None"},
	{"PortSize", (PyCFunction)Qd_PortSize, 1,
	 "(short width, short height) -> None"},
	{"MovePortTo", (PyCFunction)Qd_MovePortTo, 1,
	 "(short leftGlobal, short topGlobal) -> None"},
	{"SetOrigin", (PyCFunction)Qd_SetOrigin, 1,
	 "(short h, short v) -> None"},
	{"SetClip", (PyCFunction)Qd_SetClip, 1,
	 "(RgnHandle rgn) -> None"},
	{"GetClip", (PyCFunction)Qd_GetClip, 1,
	 "(RgnHandle rgn) -> None"},
	{"ClipRect", (PyCFunction)Qd_ClipRect, 1,
	 "(Rect r) -> None"},
	{"InitCursor", (PyCFunction)Qd_InitCursor, 1,
	 "() -> None"},
	{"HideCursor", (PyCFunction)Qd_HideCursor, 1,
	 "() -> None"},
	{"ShowCursor", (PyCFunction)Qd_ShowCursor, 1,
	 "() -> None"},
	{"ObscureCursor", (PyCFunction)Qd_ObscureCursor, 1,
	 "() -> None"},
	{"HidePen", (PyCFunction)Qd_HidePen, 1,
	 "() -> None"},
	{"ShowPen", (PyCFunction)Qd_ShowPen, 1,
	 "() -> None"},
	{"GetPen", (PyCFunction)Qd_GetPen, 1,
	 "(Point pt) -> (Point pt)"},
	{"PenSize", (PyCFunction)Qd_PenSize, 1,
	 "(short width, short height) -> None"},
	{"PenMode", (PyCFunction)Qd_PenMode, 1,
	 "(short mode) -> None"},
	{"PenNormal", (PyCFunction)Qd_PenNormal, 1,
	 "() -> None"},
	{"MoveTo", (PyCFunction)Qd_MoveTo, 1,
	 "(short h, short v) -> None"},
	{"Move", (PyCFunction)Qd_Move, 1,
	 "(short dh, short dv) -> None"},
	{"LineTo", (PyCFunction)Qd_LineTo, 1,
	 "(short h, short v) -> None"},
	{"Line", (PyCFunction)Qd_Line, 1,
	 "(short dh, short dv) -> None"},
	{"ForeColor", (PyCFunction)Qd_ForeColor, 1,
	 "(long color) -> None"},
	{"BackColor", (PyCFunction)Qd_BackColor, 1,
	 "(long color) -> None"},
	{"ColorBit", (PyCFunction)Qd_ColorBit, 1,
	 "(short whichBit) -> None"},
	{"SetRect", (PyCFunction)Qd_SetRect, 1,
	 "(short left, short top, short right, short bottom) -> (Rect r)"},
	{"OffsetRect", (PyCFunction)Qd_OffsetRect, 1,
	 "(short dh, short dv) -> (Rect r)"},
	{"InsetRect", (PyCFunction)Qd_InsetRect, 1,
	 "(short dh, short dv) -> (Rect r)"},
	{"SectRect", (PyCFunction)Qd_SectRect, 1,
	 "(Rect src1, Rect src2) -> (Boolean _rv, Rect dstRect)"},
	{"UnionRect", (PyCFunction)Qd_UnionRect, 1,
	 "(Rect src1, Rect src2) -> (Rect dstRect)"},
	{"EqualRect", (PyCFunction)Qd_EqualRect, 1,
	 "(Rect rect1, Rect rect2) -> (Boolean _rv)"},
	{"EmptyRect", (PyCFunction)Qd_EmptyRect, 1,
	 "(Rect r) -> (Boolean _rv)"},
	{"FrameRect", (PyCFunction)Qd_FrameRect, 1,
	 "(Rect r) -> None"},
	{"PaintRect", (PyCFunction)Qd_PaintRect, 1,
	 "(Rect r) -> None"},
	{"EraseRect", (PyCFunction)Qd_EraseRect, 1,
	 "(Rect r) -> None"},
	{"InvertRect", (PyCFunction)Qd_InvertRect, 1,
	 "(Rect r) -> None"},
	{"FrameOval", (PyCFunction)Qd_FrameOval, 1,
	 "(Rect r) -> None"},
	{"PaintOval", (PyCFunction)Qd_PaintOval, 1,
	 "(Rect r) -> None"},
	{"EraseOval", (PyCFunction)Qd_EraseOval, 1,
	 "(Rect r) -> None"},
	{"InvertOval", (PyCFunction)Qd_InvertOval, 1,
	 "(Rect r) -> None"},
	{"FrameRoundRect", (PyCFunction)Qd_FrameRoundRect, 1,
	 "(Rect r, short ovalWidth, short ovalHeight) -> None"},
	{"PaintRoundRect", (PyCFunction)Qd_PaintRoundRect, 1,
	 "(Rect r, short ovalWidth, short ovalHeight) -> None"},
	{"EraseRoundRect", (PyCFunction)Qd_EraseRoundRect, 1,
	 "(Rect r, short ovalWidth, short ovalHeight) -> None"},
	{"InvertRoundRect", (PyCFunction)Qd_InvertRoundRect, 1,
	 "(Rect r, short ovalWidth, short ovalHeight) -> None"},
	{"FrameArc", (PyCFunction)Qd_FrameArc, 1,
	 "(Rect r, short startAngle, short arcAngle) -> None"},
	{"PaintArc", (PyCFunction)Qd_PaintArc, 1,
	 "(Rect r, short startAngle, short arcAngle) -> None"},
	{"EraseArc", (PyCFunction)Qd_EraseArc, 1,
	 "(Rect r, short startAngle, short arcAngle) -> None"},
	{"InvertArc", (PyCFunction)Qd_InvertArc, 1,
	 "(Rect r, short startAngle, short arcAngle) -> None"},
	{"NewRgn", (PyCFunction)Qd_NewRgn, 1,
	 "() -> (RgnHandle _rv)"},
	{"OpenRgn", (PyCFunction)Qd_OpenRgn, 1,
	 "() -> None"},
	{"CloseRgn", (PyCFunction)Qd_CloseRgn, 1,
	 "(RgnHandle dstRgn) -> None"},
	{"DisposeRgn", (PyCFunction)Qd_DisposeRgn, 1,
	 "(RgnHandle rgn) -> None"},
	{"CopyRgn", (PyCFunction)Qd_CopyRgn, 1,
	 "(RgnHandle srcRgn, RgnHandle dstRgn) -> None"},
	{"SetEmptyRgn", (PyCFunction)Qd_SetEmptyRgn, 1,
	 "(RgnHandle rgn) -> None"},
	{"SetRectRgn", (PyCFunction)Qd_SetRectRgn, 1,
	 "(RgnHandle rgn, short left, short top, short right, short bottom) -> None"},
	{"RectRgn", (PyCFunction)Qd_RectRgn, 1,
	 "(RgnHandle rgn, Rect r) -> None"},
	{"OffsetRgn", (PyCFunction)Qd_OffsetRgn, 1,
	 "(RgnHandle rgn, short dh, short dv) -> None"},
	{"InsetRgn", (PyCFunction)Qd_InsetRgn, 1,
	 "(RgnHandle rgn, short dh, short dv) -> None"},
	{"SectRgn", (PyCFunction)Qd_SectRgn, 1,
	 "(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn) -> None"},
	{"UnionRgn", (PyCFunction)Qd_UnionRgn, 1,
	 "(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn) -> None"},
	{"DiffRgn", (PyCFunction)Qd_DiffRgn, 1,
	 "(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn) -> None"},
	{"XorRgn", (PyCFunction)Qd_XorRgn, 1,
	 "(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn) -> None"},
	{"RectInRgn", (PyCFunction)Qd_RectInRgn, 1,
	 "(Rect r, RgnHandle rgn) -> (Boolean _rv)"},
	{"EqualRgn", (PyCFunction)Qd_EqualRgn, 1,
	 "(RgnHandle rgnA, RgnHandle rgnB) -> (Boolean _rv)"},
	{"EmptyRgn", (PyCFunction)Qd_EmptyRgn, 1,
	 "(RgnHandle rgn) -> (Boolean _rv)"},
	{"FrameRgn", (PyCFunction)Qd_FrameRgn, 1,
	 "(RgnHandle rgn) -> None"},
	{"PaintRgn", (PyCFunction)Qd_PaintRgn, 1,
	 "(RgnHandle rgn) -> None"},
	{"EraseRgn", (PyCFunction)Qd_EraseRgn, 1,
	 "(RgnHandle rgn) -> None"},
	{"InvertRgn", (PyCFunction)Qd_InvertRgn, 1,
	 "(RgnHandle rgn) -> None"},
	{"ScrollRect", (PyCFunction)Qd_ScrollRect, 1,
	 "(Rect r, short dh, short dv, RgnHandle updateRgn) -> None"},
	{"OpenPicture", (PyCFunction)Qd_OpenPicture, 1,
	 "(Rect picFrame) -> (PicHandle _rv)"},
	{"PicComment", (PyCFunction)Qd_PicComment, 1,
	 "(short kind, short dataSize, Handle dataHandle) -> None"},
	{"ClosePicture", (PyCFunction)Qd_ClosePicture, 1,
	 "() -> None"},
	{"DrawPicture", (PyCFunction)Qd_DrawPicture, 1,
	 "(PicHandle myPicture, Rect dstRect) -> None"},
	{"KillPicture", (PyCFunction)Qd_KillPicture, 1,
	 "(PicHandle myPicture) -> None"},
	{"OpenPoly", (PyCFunction)Qd_OpenPoly, 1,
	 "() -> (PolyHandle _rv)"},
	{"ClosePoly", (PyCFunction)Qd_ClosePoly, 1,
	 "() -> None"},
	{"KillPoly", (PyCFunction)Qd_KillPoly, 1,
	 "(PolyHandle poly) -> None"},
	{"OffsetPoly", (PyCFunction)Qd_OffsetPoly, 1,
	 "(PolyHandle poly, short dh, short dv) -> None"},
	{"FramePoly", (PyCFunction)Qd_FramePoly, 1,
	 "(PolyHandle poly) -> None"},
	{"PaintPoly", (PyCFunction)Qd_PaintPoly, 1,
	 "(PolyHandle poly) -> None"},
	{"ErasePoly", (PyCFunction)Qd_ErasePoly, 1,
	 "(PolyHandle poly) -> None"},
	{"InvertPoly", (PyCFunction)Qd_InvertPoly, 1,
	 "(PolyHandle poly) -> None"},
	{"SetPt", (PyCFunction)Qd_SetPt, 1,
	 "(Point pt, short h, short v) -> (Point pt)"},
	{"LocalToGlobal", (PyCFunction)Qd_LocalToGlobal, 1,
	 "(Point pt) -> (Point pt)"},
	{"GlobalToLocal", (PyCFunction)Qd_GlobalToLocal, 1,
	 "(Point pt) -> (Point pt)"},
	{"Random", (PyCFunction)Qd_Random, 1,
	 "() -> (short _rv)"},
	{"GetPixel", (PyCFunction)Qd_GetPixel, 1,
	 "(short h, short v) -> (Boolean _rv)"},
	{"ScalePt", (PyCFunction)Qd_ScalePt, 1,
	 "(Point pt, Rect srcRect, Rect dstRect) -> (Point pt)"},
	{"MapPt", (PyCFunction)Qd_MapPt, 1,
	 "(Point pt, Rect srcRect, Rect dstRect) -> (Point pt)"},
	{"MapRect", (PyCFunction)Qd_MapRect, 1,
	 "(Rect srcRect, Rect dstRect) -> (Rect r)"},
	{"MapRgn", (PyCFunction)Qd_MapRgn, 1,
	 "(RgnHandle rgn, Rect srcRect, Rect dstRect) -> None"},
	{"MapPoly", (PyCFunction)Qd_MapPoly, 1,
	 "(PolyHandle poly, Rect srcRect, Rect dstRect) -> None"},
	{"AddPt", (PyCFunction)Qd_AddPt, 1,
	 "(Point src, Point dst) -> (Point dst)"},
	{"EqualPt", (PyCFunction)Qd_EqualPt, 1,
	 "(Point pt1, Point pt2) -> (Boolean _rv)"},
	{"PtInRect", (PyCFunction)Qd_PtInRect, 1,
	 "(Point pt, Rect r) -> (Boolean _rv)"},
	{"Pt2Rect", (PyCFunction)Qd_Pt2Rect, 1,
	 "(Point pt1, Point pt2) -> (Rect dstRect)"},
	{"PtToAngle", (PyCFunction)Qd_PtToAngle, 1,
	 "(Rect r, Point pt) -> (short angle)"},
	{"SubPt", (PyCFunction)Qd_SubPt, 1,
	 "(Point src, Point dst) -> (Point dst)"},
	{"PtInRgn", (PyCFunction)Qd_PtInRgn, 1,
	 "(Point pt, RgnHandle rgn) -> (Boolean _rv)"},
	{"NewPixMap", (PyCFunction)Qd_NewPixMap, 1,
	 "() -> (PixMapHandle _rv)"},
	{"DisposePixMap", (PyCFunction)Qd_DisposePixMap, 1,
	 "(PixMapHandle pm) -> None"},
	{"CopyPixMap", (PyCFunction)Qd_CopyPixMap, 1,
	 "(PixMapHandle srcPM, PixMapHandle dstPM) -> None"},
	{"NewPixPat", (PyCFunction)Qd_NewPixPat, 1,
	 "() -> (PixPatHandle _rv)"},
	{"DisposePixPat", (PyCFunction)Qd_DisposePixPat, 1,
	 "(PixPatHandle pp) -> None"},
	{"CopyPixPat", (PyCFunction)Qd_CopyPixPat, 1,
	 "(PixPatHandle srcPP, PixPatHandle dstPP) -> None"},
	{"PenPixPat", (PyCFunction)Qd_PenPixPat, 1,
	 "(PixPatHandle pp) -> None"},
	{"BackPixPat", (PyCFunction)Qd_BackPixPat, 1,
	 "(PixPatHandle pp) -> None"},
	{"GetPixPat", (PyCFunction)Qd_GetPixPat, 1,
	 "(short patID) -> (PixPatHandle _rv)"},
	{"FillCRect", (PyCFunction)Qd_FillCRect, 1,
	 "(Rect r, PixPatHandle pp) -> None"},
	{"FillCOval", (PyCFunction)Qd_FillCOval, 1,
	 "(Rect r, PixPatHandle pp) -> None"},
	{"FillCRoundRect", (PyCFunction)Qd_FillCRoundRect, 1,
	 "(Rect r, short ovalWidth, short ovalHeight, PixPatHandle pp) -> None"},
	{"FillCArc", (PyCFunction)Qd_FillCArc, 1,
	 "(Rect r, short startAngle, short arcAngle, PixPatHandle pp) -> None"},
	{"FillCRgn", (PyCFunction)Qd_FillCRgn, 1,
	 "(RgnHandle rgn, PixPatHandle pp) -> None"},
	{"FillCPoly", (PyCFunction)Qd_FillCPoly, 1,
	 "(PolyHandle poly, PixPatHandle pp) -> None"},
	{"SetPortPix", (PyCFunction)Qd_SetPortPix, 1,
	 "(PixMapHandle pm) -> None"},
	{"AllocCursor", (PyCFunction)Qd_AllocCursor, 1,
	 "() -> None"},
	{"GetCTSeed", (PyCFunction)Qd_GetCTSeed, 1,
	 "() -> (long _rv)"},
	{"SetClientID", (PyCFunction)Qd_SetClientID, 1,
	 "(short id) -> None"},
	{"ProtectEntry", (PyCFunction)Qd_ProtectEntry, 1,
	 "(short index, Boolean protect) -> None"},
	{"ReserveEntry", (PyCFunction)Qd_ReserveEntry, 1,
	 "(short index, Boolean reserve) -> None"},
	{"QDError", (PyCFunction)Qd_QDError, 1,
	 "() -> (short _rv)"},
	{"TextFont", (PyCFunction)Qd_TextFont, 1,
	 "(short font) -> None"},
	{"TextFace", (PyCFunction)Qd_TextFace, 1,
	 "(short face) -> None"},
	{"TextMode", (PyCFunction)Qd_TextMode, 1,
	 "(short mode) -> None"},
	{"TextSize", (PyCFunction)Qd_TextSize, 1,
	 "(short size) -> None"},
	{"SpaceExtra", (PyCFunction)Qd_SpaceExtra, 1,
	 "(long extra) -> None"},
	{"DrawChar", (PyCFunction)Qd_DrawChar, 1,
	 "(short ch) -> None"},
	{"DrawString", (PyCFunction)Qd_DrawString, 1,
	 "(Str255 s) -> None"},
	{"DrawText", (PyCFunction)Qd_DrawText, 1,
	 "(Buffer textBuf, short firstByte, short byteCount) -> None"},
	{"CharWidth", (PyCFunction)Qd_CharWidth, 1,
	 "(short ch) -> (short _rv)"},
	{"StringWidth", (PyCFunction)Qd_StringWidth, 1,
	 "(Str255 s) -> (short _rv)"},
	{"TextWidth", (PyCFunction)Qd_TextWidth, 1,
	 "(Buffer textBuf, short firstByte, short byteCount) -> (short _rv)"},
	{"CharExtra", (PyCFunction)Qd_CharExtra, 1,
	 "(long extra) -> None"},
	{NULL, NULL, 0}
};




void initQd()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("Qd", Qd_methods);
	d = PyModule_GetDict(m);
	Qd_Error = PyMac_GetOSErrException();
	if (Qd_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Qd_Error) != 0)
		Py_FatalError("can't initialize Qd.Error");
}

/* ========================= End module Qd ========================== */

