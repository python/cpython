
/* ========================== Module List =========================== */

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

#include <Lists.h>

static PyObject *List_Error;

/* ------------------------ Object type List ------------------------ */

PyTypeObject List_Type;

#define ListObj_Check(x) ((x)->ob_type == &List_Type)

typedef struct ListObject {
	PyObject_HEAD
	ListHandle ob_itself;
} ListObject;

PyObject *ListObj_New(itself)
	ListHandle itself;
{
	ListObject *it;
	if (itself == NULL) {
						PyErr_SetString(List_Error,"Cannot create null List");
						return NULL;
					}
	it = PyObject_NEW(ListObject, &List_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
ListObj_Convert(v, p_itself)
	PyObject *v;
	ListHandle *p_itself;
{
	if (!ListObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "List required");
		return 0;
	}
	*p_itself = ((ListObject *)v)->ob_itself;
	return 1;
}

static void ListObj_dealloc(self)
	ListObject *self;
{
	LDispose(self->ob_itself);
	PyMem_DEL(self);
}

static PyObject *ListObj_LAddColumn(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	short count;
	short colNum;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &count,
	                      &colNum))
		return NULL;
	_rv = LAddColumn(count,
	                 colNum,
	                 _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *ListObj_LAddRow(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	short count;
	short rowNum;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &count,
	                      &rowNum))
		return NULL;
	_rv = LAddRow(count,
	              rowNum,
	              _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *ListObj_LDelColumn(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short count;
	short colNum;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &count,
	                      &colNum))
		return NULL;
	LDelColumn(count,
	           colNum,
	           _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ListObj_LDelRow(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short count;
	short rowNum;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &count,
	                      &rowNum))
		return NULL;
	LDelRow(count,
	        rowNum,
	        _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ListObj_LGetSelect(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Boolean next;
	Point theCell;
	if (!PyArg_ParseTuple(_args, "bO&",
	                      &next,
	                      PyMac_GetPoint, &theCell))
		return NULL;
	_rv = LGetSelect(next,
	                 &theCell,
	                 _self->ob_itself);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     PyMac_BuildPoint, theCell);
	return _res;
}

static PyObject *ListObj_LLastClick(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = LLastClick(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, _rv);
	return _res;
}

static PyObject *ListObj_LNextCell(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Boolean hNext;
	Boolean vNext;
	Point theCell;
	if (!PyArg_ParseTuple(_args, "bbO&",
	                      &hNext,
	                      &vNext,
	                      PyMac_GetPoint, &theCell))
		return NULL;
	_rv = LNextCell(hNext,
	                vNext,
	                &theCell,
	                _self->ob_itself);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     PyMac_BuildPoint, theCell);
	return _res;
}

static PyObject *ListObj_LSize(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short listWidth;
	short listHeight;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &listWidth,
	                      &listHeight))
		return NULL;
	LSize(listWidth,
	      listHeight,
	      _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ListObj_LSetDrawingMode(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean drawIt;
	if (!PyArg_ParseTuple(_args, "b",
	                      &drawIt))
		return NULL;
	LSetDrawingMode(drawIt,
	                _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ListObj_LScroll(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short dCols;
	short dRows;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &dCols,
	                      &dRows))
		return NULL;
	LScroll(dCols,
	        dRows,
	        _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ListObj_LAutoScroll(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	LAutoScroll(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ListObj_LUpdate(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle theRgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theRgn))
		return NULL;
	LUpdate(theRgn,
	        _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ListObj_LActivate(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean act;
	if (!PyArg_ParseTuple(_args, "b",
	                      &act))
		return NULL;
	LActivate(act,
	          _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ListObj_LCellSize(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point cSize;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &cSize))
		return NULL;
	LCellSize(cSize,
	          _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ListObj_LClick(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point pt;
	short modifiers;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetPoint, &pt,
	                      &modifiers))
		return NULL;
	_rv = LClick(pt,
	             modifiers,
	             _self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *ListObj_LAddToCell(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	char *dataPtr__in__;
	short dataPtr__len__;
	int dataPtr__in_len__;
	Point theCell;
	if (!PyArg_ParseTuple(_args, "s#O&",
	                      &dataPtr__in__, &dataPtr__in_len__,
	                      PyMac_GetPoint, &theCell))
		return NULL;
	dataPtr__len__ = dataPtr__in_len__;
	LAddToCell(dataPtr__in__, dataPtr__len__,
	           theCell,
	           _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
 dataPtr__error__: ;
	return _res;
}

static PyObject *ListObj_LClrCell(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point theCell;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &theCell))
		return NULL;
	LClrCell(theCell,
	         _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ListObj_LGetCell(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	char *dataPtr__out__;
	short dataPtr__len__;
	int dataPtr__in_len__;
	Point theCell;
	if (!PyArg_ParseTuple(_args, "iO&",
	                      &dataPtr__in_len__,
	                      PyMac_GetPoint, &theCell))
		return NULL;
	if ((dataPtr__out__ = malloc(dataPtr__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto dataPtr__error__;
	}
	dataPtr__len__ = dataPtr__in_len__;
	LGetCell(dataPtr__out__, &dataPtr__len__,
	         theCell,
	         _self->ob_itself);
	_res = Py_BuildValue("s#",
	                     dataPtr__out__, (int)dataPtr__len__);
	free(dataPtr__out__);
 dataPtr__error__: ;
	return _res;
}

static PyObject *ListObj_LRect(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect cellRect;
	Point theCell;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &theCell))
		return NULL;
	LRect(&cellRect,
	      theCell,
	      _self->ob_itself);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &cellRect);
	return _res;
}

static PyObject *ListObj_LSetCell(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	char *dataPtr__in__;
	short dataPtr__len__;
	int dataPtr__in_len__;
	Point theCell;
	if (!PyArg_ParseTuple(_args, "s#O&",
	                      &dataPtr__in__, &dataPtr__in_len__,
	                      PyMac_GetPoint, &theCell))
		return NULL;
	dataPtr__len__ = dataPtr__in_len__;
	LSetCell(dataPtr__in__, dataPtr__len__,
	         theCell,
	         _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
 dataPtr__error__: ;
	return _res;
}

static PyObject *ListObj_LSetSelect(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean setIt;
	Point theCell;
	if (!PyArg_ParseTuple(_args, "bO&",
	                      &setIt,
	                      PyMac_GetPoint, &theCell))
		return NULL;
	LSetSelect(setIt,
	           theCell,
	           _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ListObj_LDraw(_self, _args)
	ListObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point theCell;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &theCell))
		return NULL;
	LDraw(theCell,
	      _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef ListObj_methods[] = {
	{"LAddColumn", (PyCFunction)ListObj_LAddColumn, 1,
	 "(short count, short colNum) -> (short _rv)"},
	{"LAddRow", (PyCFunction)ListObj_LAddRow, 1,
	 "(short count, short rowNum) -> (short _rv)"},
	{"LDelColumn", (PyCFunction)ListObj_LDelColumn, 1,
	 "(short count, short colNum) -> None"},
	{"LDelRow", (PyCFunction)ListObj_LDelRow, 1,
	 "(short count, short rowNum) -> None"},
	{"LGetSelect", (PyCFunction)ListObj_LGetSelect, 1,
	 "(Boolean next, Point theCell) -> (Boolean _rv, Point theCell)"},
	{"LLastClick", (PyCFunction)ListObj_LLastClick, 1,
	 "() -> (Point _rv)"},
	{"LNextCell", (PyCFunction)ListObj_LNextCell, 1,
	 "(Boolean hNext, Boolean vNext, Point theCell) -> (Boolean _rv, Point theCell)"},
	{"LSize", (PyCFunction)ListObj_LSize, 1,
	 "(short listWidth, short listHeight) -> None"},
	{"LSetDrawingMode", (PyCFunction)ListObj_LSetDrawingMode, 1,
	 "(Boolean drawIt) -> None"},
	{"LScroll", (PyCFunction)ListObj_LScroll, 1,
	 "(short dCols, short dRows) -> None"},
	{"LAutoScroll", (PyCFunction)ListObj_LAutoScroll, 1,
	 "() -> None"},
	{"LUpdate", (PyCFunction)ListObj_LUpdate, 1,
	 "(RgnHandle theRgn) -> None"},
	{"LActivate", (PyCFunction)ListObj_LActivate, 1,
	 "(Boolean act) -> None"},
	{"LCellSize", (PyCFunction)ListObj_LCellSize, 1,
	 "(Point cSize) -> None"},
	{"LClick", (PyCFunction)ListObj_LClick, 1,
	 "(Point pt, short modifiers) -> (Boolean _rv)"},
	{"LAddToCell", (PyCFunction)ListObj_LAddToCell, 1,
	 "(Buffer dataPtr, Point theCell) -> None"},
	{"LClrCell", (PyCFunction)ListObj_LClrCell, 1,
	 "(Point theCell) -> None"},
	{"LGetCell", (PyCFunction)ListObj_LGetCell, 1,
	 "(Buffer dataPtr, Point theCell) -> (Buffer dataPtr)"},
	{"LRect", (PyCFunction)ListObj_LRect, 1,
	 "(Point theCell) -> (Rect cellRect)"},
	{"LSetCell", (PyCFunction)ListObj_LSetCell, 1,
	 "(Buffer dataPtr, Point theCell) -> None"},
	{"LSetSelect", (PyCFunction)ListObj_LSetSelect, 1,
	 "(Boolean setIt, Point theCell) -> None"},
	{"LDraw", (PyCFunction)ListObj_LDraw, 1,
	 "(Point theCell) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain ListObj_chain = { ListObj_methods, NULL };

static PyObject *ListObj_getattr(self, name)
	ListObject *self;
	char *name;
{
	{
		/* XXXX Should we HLock() here?? */
		if ( strcmp(name, "listFlags") == 0 )
			return Py_BuildValue("l", (long)(*self->ob_itself)->listFlags & 0xff);
		if ( strcmp(name, "selFlags") == 0 )
			return Py_BuildValue("l", (long)(*self->ob_itself)->selFlags & 0xff);
	}
	return Py_FindMethodInChain(&ListObj_chain, (PyObject *)self, name);
}

static int
ListObj_setattr(self, name, value)
	ListObject *self;
	char *name;
	PyObject *value;
{
	long intval;
		
	if ( value == NULL || !PyInt_Check(value) )
		return -1;
	intval = PyInt_AsLong(value);
	if (strcmp(name, "listFlags") == 0 ) {
		/* XXXX Should we HLock the handle here?? */
		(*self->ob_itself)->listFlags = intval;
		return 0;
	}
	if (strcmp(name, "selFlags") == 0 ) {
		(*self->ob_itself)->selFlags = intval;
		return 0;
	}
	return -1;
}


PyTypeObject List_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"List", /*tp_name*/
	sizeof(ListObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) ListObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) ListObj_getattr, /*tp_getattr*/
	(setattrfunc) ListObj_setattr, /*tp_setattr*/
};

/* ---------------------- End object type List ---------------------- */


static PyObject *List_LNew(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ListHandle _rv;
	Rect rView;
	Rect dataBounds;
	Point cSize;
	short theProc;
	WindowPtr theWindow;
	Boolean drawIt;
	Boolean hasGrow;
	Boolean scrollHoriz;
	Boolean scrollVert;
	if (!PyArg_ParseTuple(_args, "O&O&O&hO&bbbb",
	                      PyMac_GetRect, &rView,
	                      PyMac_GetRect, &dataBounds,
	                      PyMac_GetPoint, &cSize,
	                      &theProc,
	                      WinObj_Convert, &theWindow,
	                      &drawIt,
	                      &hasGrow,
	                      &scrollHoriz,
	                      &scrollVert))
		return NULL;
	_rv = LNew(&rView,
	           &dataBounds,
	           cSize,
	           theProc,
	           theWindow,
	           drawIt,
	           hasGrow,
	           scrollHoriz,
	           scrollVert);
	_res = Py_BuildValue("O&",
	                     ListObj_New, _rv);
	return _res;
}

static PyMethodDef List_methods[] = {
	{"LNew", (PyCFunction)List_LNew, 1,
	 "(Rect rView, Rect dataBounds, Point cSize, short theProc, WindowPtr theWindow, Boolean drawIt, Boolean hasGrow, Boolean scrollHoriz, Boolean scrollVert) -> (ListHandle _rv)"},
	{NULL, NULL, 0}
};




void initList()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("List", List_methods);
	d = PyModule_GetDict(m);
	List_Error = PyMac_GetOSErrException();
	if (List_Error == NULL ||
	    PyDict_SetItemString(d, "Error", List_Error) != 0)
		Py_FatalError("can't initialize List.Error");
	List_Type.ob_type = &PyType_Type;
	Py_INCREF(&List_Type);
	if (PyDict_SetItemString(d, "ListType", (PyObject *)&List_Type) != 0)
		Py_FatalError("can't initialize ListType");
}

/* ======================== End module List ========================= */

