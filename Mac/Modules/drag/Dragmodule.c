
/* ========================== Module Drag =========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <Drag.h>
#else
#include <Carbon/Carbon.h>
#endif

/* Callback glue routines */
DragTrackingHandlerUPP dragglue_TrackingHandlerUPP;
DragReceiveHandlerUPP dragglue_ReceiveHandlerUPP;
DragSendDataUPP dragglue_SendDataUPP;
#if 0
DragInputUPP dragglue_InputUPP;
DragDrawingUPP dragglue_DrawingUPP;
#endif

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_DragObj_New(DragRef);
extern int _DragObj_Convert(PyObject *, DragRef *);

#define DragObj_New _DragObj_New
#define DragObj_Convert _DragObj_Convert
#endif

static PyObject *Drag_Error;

/* ---------------------- Object type DragObj ----------------------- */

PyTypeObject DragObj_Type;

#define DragObj_Check(x) ((x)->ob_type == &DragObj_Type)

typedef struct DragObjObject {
	PyObject_HEAD
	DragRef ob_itself;
	PyObject *sendproc;
} DragObjObject;

PyObject *DragObj_New(DragRef itself)
{
	DragObjObject *it;
	if (itself == NULL) {
						PyErr_SetString(Drag_Error,"Cannot create null Drag");
						return NULL;
					}
	it = PyObject_NEW(DragObjObject, &DragObj_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->sendproc = NULL;
	return (PyObject *)it;
}
DragObj_Convert(PyObject *v, DragRef *p_itself)
{
	if (!DragObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "DragObj required");
		return 0;
	}
	*p_itself = ((DragObjObject *)v)->ob_itself;
	return 1;
}

static void DragObj_dealloc(DragObjObject *self)
{
	Py_XDECREF(self->sendproc);
	PyMem_DEL(self);
}

static PyObject *DragObj_DisposeDrag(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = DisposeDrag(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DragObj_AddDragItemFlavor(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ItemReference theItemRef;
	FlavorType theType;
	char *dataPtr__in__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	FlavorFlags theFlags;
	if (!PyArg_ParseTuple(_args, "lO&z#l",
	                      &theItemRef,
	                      PyMac_GetOSType, &theType,
	                      &dataPtr__in__, &dataPtr__in_len__,
	                      &theFlags))
		return NULL;
	dataPtr__len__ = dataPtr__in_len__;
	_err = AddDragItemFlavor(_self->ob_itself,
	                         theItemRef,
	                         theType,
	                         dataPtr__in__, dataPtr__len__,
	                         theFlags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
 dataPtr__error__: ;
	return _res;
}

static PyObject *DragObj_SetDragItemFlavorData(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ItemReference theItemRef;
	FlavorType theType;
	char *dataPtr__in__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	UInt32 dataOffset;
	if (!PyArg_ParseTuple(_args, "lO&z#l",
	                      &theItemRef,
	                      PyMac_GetOSType, &theType,
	                      &dataPtr__in__, &dataPtr__in_len__,
	                      &dataOffset))
		return NULL;
	dataPtr__len__ = dataPtr__in_len__;
	_err = SetDragItemFlavorData(_self->ob_itself,
	                             theItemRef,
	                             theType,
	                             dataPtr__in__, dataPtr__len__,
	                             dataOffset);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
 dataPtr__error__: ;
	return _res;
}

static PyObject *DragObj_SetDragImage(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	PixMapHandle imagePixMap;
	RgnHandle imageRgn;
	Point imageOffsetPt;
	DragImageFlags theImageFlags;
	if (!PyArg_ParseTuple(_args, "O&O&O&l",
	                      ResObj_Convert, &imagePixMap,
	                      ResObj_Convert, &imageRgn,
	                      PyMac_GetPoint, &imageOffsetPt,
	                      &theImageFlags))
		return NULL;
	_err = SetDragImage(_self->ob_itself,
	                    imagePixMap,
	                    imageRgn,
	                    imageOffsetPt,
	                    theImageFlags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DragObj_ChangeDragBehaviors(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DragBehaviors inBehaviorsToSet;
	DragBehaviors inBehaviorsToClear;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &inBehaviorsToSet,
	                      &inBehaviorsToClear))
		return NULL;
	_err = ChangeDragBehaviors(_self->ob_itself,
	                           inBehaviorsToSet,
	                           inBehaviorsToClear);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DragObj_TrackDrag(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	EventRecord theEvent;
	RgnHandle theRegion;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetEventRecord, &theEvent,
	                      ResObj_Convert, &theRegion))
		return NULL;
	_err = TrackDrag(_self->ob_itself,
	                 &theEvent,
	                 theRegion);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DragObj_CountDragItems(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	UInt16 numItems;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = CountDragItems(_self->ob_itself,
	                      &numItems);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("H",
	                     numItems);
	return _res;
}

static PyObject *DragObj_GetDragItemReferenceNumber(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	UInt16 index;
	ItemReference theItemRef;
	if (!PyArg_ParseTuple(_args, "H",
	                      &index))
		return NULL;
	_err = GetDragItemReferenceNumber(_self->ob_itself,
	                                  index,
	                                  &theItemRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     theItemRef);
	return _res;
}

static PyObject *DragObj_CountDragItemFlavors(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ItemReference theItemRef;
	UInt16 numFlavors;
	if (!PyArg_ParseTuple(_args, "l",
	                      &theItemRef))
		return NULL;
	_err = CountDragItemFlavors(_self->ob_itself,
	                            theItemRef,
	                            &numFlavors);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("H",
	                     numFlavors);
	return _res;
}

static PyObject *DragObj_GetFlavorType(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ItemReference theItemRef;
	UInt16 index;
	FlavorType theType;
	if (!PyArg_ParseTuple(_args, "lH",
	                      &theItemRef,
	                      &index))
		return NULL;
	_err = GetFlavorType(_self->ob_itself,
	                     theItemRef,
	                     index,
	                     &theType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildOSType, theType);
	return _res;
}

static PyObject *DragObj_GetFlavorFlags(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ItemReference theItemRef;
	FlavorType theType;
	FlavorFlags theFlags;
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &theItemRef,
	                      PyMac_GetOSType, &theType))
		return NULL;
	_err = GetFlavorFlags(_self->ob_itself,
	                      theItemRef,
	                      theType,
	                      &theFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     theFlags);
	return _res;
}

static PyObject *DragObj_GetFlavorDataSize(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ItemReference theItemRef;
	FlavorType theType;
	Size dataSize;
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &theItemRef,
	                      PyMac_GetOSType, &theType))
		return NULL;
	_err = GetFlavorDataSize(_self->ob_itself,
	                         theItemRef,
	                         theType,
	                         &dataSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     dataSize);
	return _res;
}

static PyObject *DragObj_GetFlavorData(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ItemReference theItemRef;
	FlavorType theType;
	char *dataPtr__out__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	UInt32 dataOffset;
	if (!PyArg_ParseTuple(_args, "lO&il",
	                      &theItemRef,
	                      PyMac_GetOSType, &theType,
	                      &dataPtr__in_len__,
	                      &dataOffset))
		return NULL;
	if ((dataPtr__out__ = malloc(dataPtr__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto dataPtr__error__;
	}
	dataPtr__len__ = dataPtr__in_len__;
	_err = GetFlavorData(_self->ob_itself,
	                     theItemRef,
	                     theType,
	                     dataPtr__out__, &dataPtr__len__,
	                     dataOffset);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("s#",
	                     dataPtr__out__, (int)dataPtr__len__);
	free(dataPtr__out__);
 dataPtr__error__: ;
	return _res;
}

static PyObject *DragObj_GetDragItemBounds(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ItemReference theItemRef;
	Rect itemBounds;
	if (!PyArg_ParseTuple(_args, "l",
	                      &theItemRef))
		return NULL;
	_err = GetDragItemBounds(_self->ob_itself,
	                         theItemRef,
	                         &itemBounds);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &itemBounds);
	return _res;
}

static PyObject *DragObj_SetDragItemBounds(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ItemReference theItemRef;
	Rect itemBounds;
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &theItemRef,
	                      PyMac_GetRect, &itemBounds))
		return NULL;
	_err = SetDragItemBounds(_self->ob_itself,
	                         theItemRef,
	                         &itemBounds);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DragObj_GetDropLocation(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEDesc dropLocation;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetDropLocation(_self->ob_itself,
	                       &dropLocation);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &dropLocation);
	return _res;
}

static PyObject *DragObj_SetDropLocation(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEDesc dropLocation;
	if (!PyArg_ParseTuple(_args, "O&",
	                      AEDesc_Convert, &dropLocation))
		return NULL;
	_err = SetDropLocation(_self->ob_itself,
	                       &dropLocation);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DragObj_GetDragAttributes(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DragAttributes flags;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetDragAttributes(_self->ob_itself,
	                         &flags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     flags);
	return _res;
}

static PyObject *DragObj_GetDragMouse(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Point mouse;
	Point globalPinnedMouse;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetDragMouse(_self->ob_itself,
	                    &mouse,
	                    &globalPinnedMouse);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     PyMac_BuildPoint, mouse,
	                     PyMac_BuildPoint, globalPinnedMouse);
	return _res;
}

static PyObject *DragObj_SetDragMouse(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Point globalPinnedMouse;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &globalPinnedMouse))
		return NULL;
	_err = SetDragMouse(_self->ob_itself,
	                    globalPinnedMouse);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DragObj_GetDragOrigin(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Point globalInitialMouse;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetDragOrigin(_self->ob_itself,
	                     &globalInitialMouse);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, globalInitialMouse);
	return _res;
}

static PyObject *DragObj_GetDragModifiers(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 modifiers;
	SInt16 mouseDownModifiers;
	SInt16 mouseUpModifiers;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetDragModifiers(_self->ob_itself,
	                        &modifiers,
	                        &mouseDownModifiers,
	                        &mouseUpModifiers);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("hhh",
	                     modifiers,
	                     mouseDownModifiers,
	                     mouseUpModifiers);
	return _res;
}

static PyObject *DragObj_ShowDragHilite(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	RgnHandle hiliteFrame;
	Boolean inside;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      ResObj_Convert, &hiliteFrame,
	                      &inside))
		return NULL;
	_err = ShowDragHilite(_self->ob_itself,
	                      hiliteFrame,
	                      inside);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DragObj_HideDragHilite(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HideDragHilite(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DragObj_DragPreScroll(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 dH;
	SInt16 dV;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &dH,
	                      &dV))
		return NULL;
	_err = DragPreScroll(_self->ob_itself,
	                     dH,
	                     dV);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DragObj_DragPostScroll(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = DragPostScroll(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DragObj_UpdateDragHilite(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	RgnHandle updateRgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &updateRgn))
		return NULL;
	_err = UpdateDragHilite(_self->ob_itself,
	                        updateRgn);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef DragObj_methods[] = {
	{"DisposeDrag", (PyCFunction)DragObj_DisposeDrag, 1,
	 "() -> None"},
	{"AddDragItemFlavor", (PyCFunction)DragObj_AddDragItemFlavor, 1,
	 "(ItemReference theItemRef, FlavorType theType, Buffer dataPtr, FlavorFlags theFlags) -> None"},
	{"SetDragItemFlavorData", (PyCFunction)DragObj_SetDragItemFlavorData, 1,
	 "(ItemReference theItemRef, FlavorType theType, Buffer dataPtr, UInt32 dataOffset) -> None"},
	{"SetDragImage", (PyCFunction)DragObj_SetDragImage, 1,
	 "(PixMapHandle imagePixMap, RgnHandle imageRgn, Point imageOffsetPt, DragImageFlags theImageFlags) -> None"},
	{"ChangeDragBehaviors", (PyCFunction)DragObj_ChangeDragBehaviors, 1,
	 "(DragBehaviors inBehaviorsToSet, DragBehaviors inBehaviorsToClear) -> None"},
	{"TrackDrag", (PyCFunction)DragObj_TrackDrag, 1,
	 "(EventRecord theEvent, RgnHandle theRegion) -> None"},
	{"CountDragItems", (PyCFunction)DragObj_CountDragItems, 1,
	 "() -> (UInt16 numItems)"},
	{"GetDragItemReferenceNumber", (PyCFunction)DragObj_GetDragItemReferenceNumber, 1,
	 "(UInt16 index) -> (ItemReference theItemRef)"},
	{"CountDragItemFlavors", (PyCFunction)DragObj_CountDragItemFlavors, 1,
	 "(ItemReference theItemRef) -> (UInt16 numFlavors)"},
	{"GetFlavorType", (PyCFunction)DragObj_GetFlavorType, 1,
	 "(ItemReference theItemRef, UInt16 index) -> (FlavorType theType)"},
	{"GetFlavorFlags", (PyCFunction)DragObj_GetFlavorFlags, 1,
	 "(ItemReference theItemRef, FlavorType theType) -> (FlavorFlags theFlags)"},
	{"GetFlavorDataSize", (PyCFunction)DragObj_GetFlavorDataSize, 1,
	 "(ItemReference theItemRef, FlavorType theType) -> (Size dataSize)"},
	{"GetFlavorData", (PyCFunction)DragObj_GetFlavorData, 1,
	 "(ItemReference theItemRef, FlavorType theType, Buffer dataPtr, UInt32 dataOffset) -> (Buffer dataPtr)"},
	{"GetDragItemBounds", (PyCFunction)DragObj_GetDragItemBounds, 1,
	 "(ItemReference theItemRef) -> (Rect itemBounds)"},
	{"SetDragItemBounds", (PyCFunction)DragObj_SetDragItemBounds, 1,
	 "(ItemReference theItemRef, Rect itemBounds) -> None"},
	{"GetDropLocation", (PyCFunction)DragObj_GetDropLocation, 1,
	 "() -> (AEDesc dropLocation)"},
	{"SetDropLocation", (PyCFunction)DragObj_SetDropLocation, 1,
	 "(AEDesc dropLocation) -> None"},
	{"GetDragAttributes", (PyCFunction)DragObj_GetDragAttributes, 1,
	 "() -> (DragAttributes flags)"},
	{"GetDragMouse", (PyCFunction)DragObj_GetDragMouse, 1,
	 "() -> (Point mouse, Point globalPinnedMouse)"},
	{"SetDragMouse", (PyCFunction)DragObj_SetDragMouse, 1,
	 "(Point globalPinnedMouse) -> None"},
	{"GetDragOrigin", (PyCFunction)DragObj_GetDragOrigin, 1,
	 "() -> (Point globalInitialMouse)"},
	{"GetDragModifiers", (PyCFunction)DragObj_GetDragModifiers, 1,
	 "() -> (SInt16 modifiers, SInt16 mouseDownModifiers, SInt16 mouseUpModifiers)"},
	{"ShowDragHilite", (PyCFunction)DragObj_ShowDragHilite, 1,
	 "(RgnHandle hiliteFrame, Boolean inside) -> None"},
	{"HideDragHilite", (PyCFunction)DragObj_HideDragHilite, 1,
	 "() -> None"},
	{"DragPreScroll", (PyCFunction)DragObj_DragPreScroll, 1,
	 "(SInt16 dH, SInt16 dV) -> None"},
	{"DragPostScroll", (PyCFunction)DragObj_DragPostScroll, 1,
	 "() -> None"},
	{"UpdateDragHilite", (PyCFunction)DragObj_UpdateDragHilite, 1,
	 "(RgnHandle updateRgn) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain DragObj_chain = { DragObj_methods, NULL };

static PyObject *DragObj_getattr(DragObjObject *self, char *name)
{
	return Py_FindMethodInChain(&DragObj_chain, (PyObject *)self, name);
}

#define DragObj_setattr NULL

#define DragObj_compare NULL

#define DragObj_repr NULL

#define DragObj_hash NULL

PyTypeObject DragObj_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"DragObj", /*tp_name*/
	sizeof(DragObjObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) DragObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) DragObj_getattr, /*tp_getattr*/
	(setattrfunc) DragObj_setattr, /*tp_setattr*/
	(cmpfunc) DragObj_compare, /*tp_compare*/
	(reprfunc) DragObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) DragObj_hash, /*tp_hash*/
};

/* -------------------- End object type DragObj --------------------- */


static PyObject *Drag_NewDrag(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = NewDrag(&theDrag);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     DragObj_New, theDrag);
	return _res;
}

static PyObject *Drag_GetDragHiliteColor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	WindowPtr window;
	RGBColor color;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &window))
		return NULL;
	_err = GetDragHiliteColor(window,
	                          &color);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     QdRGB_New, &color);
	return _res;
}

static PyObject *Drag_WaitMouseMoved(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point initialMouse;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &initialMouse))
		return NULL;
	_rv = WaitMouseMoved(initialMouse);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Drag_ZoomRects(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect fromRect;
	Rect toRect;
	SInt16 zoomSteps;
	ZoomAcceleration acceleration;
	if (!PyArg_ParseTuple(_args, "O&O&hh",
	                      PyMac_GetRect, &fromRect,
	                      PyMac_GetRect, &toRect,
	                      &zoomSteps,
	                      &acceleration))
		return NULL;
	_err = ZoomRects(&fromRect,
	                 &toRect,
	                 zoomSteps,
	                 acceleration);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_ZoomRegion(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	RgnHandle region;
	Point zoomDistance;
	SInt16 zoomSteps;
	ZoomAcceleration acceleration;
	if (!PyArg_ParseTuple(_args, "O&O&hh",
	                      ResObj_Convert, &region,
	                      PyMac_GetPoint, &zoomDistance,
	                      &zoomSteps,
	                      &acceleration))
		return NULL;
	_err = ZoomRegion(region,
	                  zoomDistance,
	                  zoomSteps,
	                  acceleration);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_InstallTrackingHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	    PyObject *callback;
	    WindowPtr theWindow = NULL;
	    OSErr _err;
	    
	    if ( !PyArg_ParseTuple(_args, "O|O&", &callback, WinObj_Convert, &theWindow) )
	    	return NULL;
	    Py_INCREF(callback);	/* Cannot decref later, too bad */
	    _err = InstallTrackingHandler(dragglue_TrackingHandlerUPP, theWindow, (void *)callback);
		if (_err != noErr) return PyMac_Error(_err);
		Py_INCREF(Py_None);
		return Py_None;

}

static PyObject *Drag_InstallReceiveHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	    PyObject *callback;
	    WindowPtr theWindow = NULL;
	    OSErr _err;
	    
	    if ( !PyArg_ParseTuple(_args, "O|O&", &callback, WinObj_Convert, &theWindow) )
	    	return NULL;
	    Py_INCREF(callback);	/* Cannot decref later, too bad */
	    _err = InstallReceiveHandler(dragglue_ReceiveHandlerUPP, theWindow, (void *)callback);
		if (_err != noErr) return PyMac_Error(_err);
		Py_INCREF(Py_None);
		return Py_None;

}

static PyObject *Drag_RemoveTrackingHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	    WindowPtr theWindow = NULL;
	    OSErr _err;
	    
	    if ( !PyArg_ParseTuple(_args, "|O&", WinObj_Convert, &theWindow) )
	    	return NULL;
	    _err = RemoveTrackingHandler(dragglue_TrackingHandlerUPP, theWindow);
		if (_err != noErr) return PyMac_Error(_err);
		Py_INCREF(Py_None);
		return Py_None;

}

static PyObject *Drag_RemoveReceiveHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	    WindowPtr theWindow = NULL;
	    OSErr _err;
	    
	    if ( !PyArg_ParseTuple(_args, "|O&", WinObj_Convert, &theWindow) )
	    	return NULL;
	    _err = RemoveReceiveHandler(dragglue_ReceiveHandlerUPP, theWindow);
		if (_err != noErr) return PyMac_Error(_err);
		Py_INCREF(Py_None);
		return Py_None;

}

static PyMethodDef Drag_methods[] = {
	{"NewDrag", (PyCFunction)Drag_NewDrag, 1,
	 "() -> (DragRef theDrag)"},
	{"GetDragHiliteColor", (PyCFunction)Drag_GetDragHiliteColor, 1,
	 "(WindowPtr window) -> (RGBColor color)"},
	{"WaitMouseMoved", (PyCFunction)Drag_WaitMouseMoved, 1,
	 "(Point initialMouse) -> (Boolean _rv)"},
	{"ZoomRects", (PyCFunction)Drag_ZoomRects, 1,
	 "(Rect fromRect, Rect toRect, SInt16 zoomSteps, ZoomAcceleration acceleration) -> None"},
	{"ZoomRegion", (PyCFunction)Drag_ZoomRegion, 1,
	 "(RgnHandle region, Point zoomDistance, SInt16 zoomSteps, ZoomAcceleration acceleration) -> None"},
	{"InstallTrackingHandler", (PyCFunction)Drag_InstallTrackingHandler, 1,
	 NULL},
	{"InstallReceiveHandler", (PyCFunction)Drag_InstallReceiveHandler, 1,
	 NULL},
	{"RemoveTrackingHandler", (PyCFunction)Drag_RemoveTrackingHandler, 1,
	 NULL},
	{"RemoveReceiveHandler", (PyCFunction)Drag_RemoveReceiveHandler, 1,
	 NULL},
	{NULL, NULL, 0}
};



static pascal OSErr
dragglue_TrackingHandler(DragTrackingMessage theMessage, WindowPtr theWindow,
                         void *handlerRefCon, DragReference theDrag)
{
	PyObject *args, *rv;
	int i;
	
	args = Py_BuildValue("hO&O&", theMessage, DragObj_New, theDrag, WinObj_WhichWindow, theWindow);
	if ( args == NULL )
		return -1;
	rv = PyEval_CallObject((PyObject *)handlerRefCon, args);
	Py_DECREF(args);
	if ( rv == NULL ) {
		fprintf(stderr, "Drag: Exception in TrackingHandler\n");
		return -1;
	}
	i = -1;
	if ( rv == Py_None )
		i = 0;
	else
		PyArg_Parse(rv, "l", &i);
	Py_DECREF(rv);
	return i;
}

static pascal OSErr
dragglue_ReceiveHandler(WindowPtr theWindow, void *handlerRefCon,
                        DragReference theDrag)
{
	PyObject *args, *rv;
	int i;
	
	args = Py_BuildValue("O&O&", DragObj_New, theDrag, WinObj_WhichWindow, theWindow);
	if ( args == NULL )
		return -1;
	rv = PyEval_CallObject((PyObject *)handlerRefCon, args);
	Py_DECREF(args);
	if ( rv == NULL ) {
		fprintf(stderr, "Drag: Exception in ReceiveHandler\n");
		return -1;
	}
	i = -1;
	if ( rv == Py_None )
		i = 0;
	else
		PyArg_Parse(rv, "l", &i);
	Py_DECREF(rv);
	return i;
}

static pascal OSErr
dragglue_SendData(FlavorType theType, void *dragSendRefCon,
                      ItemReference theItem, DragReference theDrag)
{
	DragObjObject *self = (DragObjObject *)dragSendRefCon;
	PyObject *args, *rv;
	int i;
	
	if ( self->sendproc == NULL )
		return -1;
	args = Py_BuildValue("O&l", PyMac_BuildOSType, theType, theItem);
	if ( args == NULL )
		return -1;
	rv = PyEval_CallObject(self->sendproc, args);
	Py_DECREF(args);
	if ( rv == NULL ) {
		fprintf(stderr, "Drag: Exception in SendDataHandler\n");
		return -1;
	}
	i = -1;
	if ( rv == Py_None )
		i = 0;
	else
		PyArg_Parse(rv, "l", &i);
	Py_DECREF(rv);
	return i;
}

#if 0
static pascal OSErr
dragglue_Input(Point *mouse, short *modifiers,
                   void *dragSendRefCon, DragReference theDrag)
{
    return 0;
}

static pascal OSErr
dragglue_Drawing(xxxx
                   void *dragSendRefCon, DragReference theDrag)
{
    return 0;
}
#endif



void initDrag(void)
{
	PyObject *m;
	PyObject *d;



		PyMac_INIT_TOOLBOX_OBJECT_NEW(DragRef, DragObj_New);
		PyMac_INIT_TOOLBOX_OBJECT_CONVERT(DragRef, DragObj_Convert);


	m = Py_InitModule("Drag", Drag_methods);
	d = PyModule_GetDict(m);
	Drag_Error = PyMac_GetOSErrException();
	if (Drag_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Drag_Error) != 0)
		return;
	DragObj_Type.ob_type = &PyType_Type;
	Py_INCREF(&DragObj_Type);
	if (PyDict_SetItemString(d, "DragObjType", (PyObject *)&DragObj_Type) != 0)
		Py_FatalError("can't initialize DragObjType");

	dragglue_TrackingHandlerUPP = NewDragTrackingHandlerUPP(dragglue_TrackingHandler);
	dragglue_ReceiveHandlerUPP = NewDragReceiveHandlerUPP(dragglue_ReceiveHandler);
	dragglue_SendDataUPP = NewDragSendDataUPP(dragglue_SendData);
#if 0
	dragglue_InputUPP = NewDragInputUPP(dragglue_Input);
	dragglue_DrawingUPP = NewDragDrawingUPP(dragglue_Drawing);
#endif


}

/* ======================== End module Drag ========================= */

