
/* ========================== Module Drag =========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#include <Drag.h>

/* Callback glue routines */
DragTrackingHandlerUPP dragglue_TrackingHandlerUPP;
DragReceiveHandlerUPP dragglue_ReceiveHandlerUPP;
DragSendDataUPP dragglue_SendDataUPP;
#if 0
DragInputUPP dragglue_InputUPP;
DragDrawingUPP dragglue_DrawingUPP;
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

PyObject *DragObj_New(itself)
	DragRef itself;
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
DragObj_Convert(v, p_itself)
	PyObject *v;
	DragRef *p_itself;
{
	if (!DragObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "DragObj required");
		return 0;
	}
	*p_itself = ((DragObjObject *)v)->ob_itself;
	return 1;
}

static void DragObj_dealloc(self)
	DragObjObject *self;
{
	Py_XDECREF(self->sendproc);
	PyMem_DEL(self);
}

static PyMethodDef DragObj_methods[] = {
	{NULL, NULL, 0}
};

PyMethodChain DragObj_chain = { DragObj_methods, NULL };

static PyObject *DragObj_getattr(self, name)
	DragObjObject *self;
	char *name;
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


static PyObject *Drag_NewDrag(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Drag_DisposeDrag(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	if (!PyArg_ParseTuple(_args, "O&",
	                      DragObj_Convert, &theDrag))
		return NULL;
	_err = DisposeDrag(theDrag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_AddDragItemFlavor(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	ItemReference theItemRef;
	FlavorType theType;
	char *dataPtr__in__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	FlavorFlags theFlags;
	if (!PyArg_ParseTuple(_args, "O&lO&z#l",
	                      DragObj_Convert, &theDrag,
	                      &theItemRef,
	                      PyMac_GetOSType, &theType,
	                      &dataPtr__in__, &dataPtr__in_len__,
	                      &theFlags))
		return NULL;
	dataPtr__len__ = dataPtr__in_len__;
	_err = AddDragItemFlavor(theDrag,
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

static PyObject *Drag_SetDragItemFlavorData(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	ItemReference theItemRef;
	FlavorType theType;
	char *dataPtr__in__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	UInt32 dataOffset;
	if (!PyArg_ParseTuple(_args, "O&lO&z#l",
	                      DragObj_Convert, &theDrag,
	                      &theItemRef,
	                      PyMac_GetOSType, &theType,
	                      &dataPtr__in__, &dataPtr__in_len__,
	                      &dataOffset))
		return NULL;
	dataPtr__len__ = dataPtr__in_len__;
	_err = SetDragItemFlavorData(theDrag,
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

static PyObject *Drag_SetDragImage(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	PixMapHandle imagePixMap;
	RgnHandle imageRgn;
	Point imageOffsetPt;
	DragImageFlags theImageFlags;
	if (!PyArg_ParseTuple(_args, "O&O&O&O&l",
	                      DragObj_Convert, &theDrag,
	                      ResObj_Convert, &imagePixMap,
	                      ResObj_Convert, &imageRgn,
	                      PyMac_GetPoint, &imageOffsetPt,
	                      &theImageFlags))
		return NULL;
	_err = SetDragImage(theDrag,
	                    imagePixMap,
	                    imageRgn,
	                    imageOffsetPt,
	                    theImageFlags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_ChangeDragBehaviors(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	DragBehaviors inBehaviorsToSet;
	DragBehaviors inBehaviorsToClear;
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      DragObj_Convert, &theDrag,
	                      &inBehaviorsToSet,
	                      &inBehaviorsToClear))
		return NULL;
	_err = ChangeDragBehaviors(theDrag,
	                           inBehaviorsToSet,
	                           inBehaviorsToClear);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_TrackDrag(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	EventRecord theEvent;
	RgnHandle theRegion;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      DragObj_Convert, &theDrag,
	                      PyMac_GetEventRecord, &theEvent,
	                      ResObj_Convert, &theRegion))
		return NULL;
	_err = TrackDrag(theDrag,
	                 &theEvent,
	                 theRegion);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_CountDragItems(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	UInt16 numItems;
	if (!PyArg_ParseTuple(_args, "O&",
	                      DragObj_Convert, &theDrag))
		return NULL;
	_err = CountDragItems(theDrag,
	                      &numItems);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("H",
	                     numItems);
	return _res;
}

static PyObject *Drag_GetDragItemReferenceNumber(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	UInt16 index;
	ItemReference theItemRef;
	if (!PyArg_ParseTuple(_args, "O&H",
	                      DragObj_Convert, &theDrag,
	                      &index))
		return NULL;
	_err = GetDragItemReferenceNumber(theDrag,
	                                  index,
	                                  &theItemRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     theItemRef);
	return _res;
}

static PyObject *Drag_CountDragItemFlavors(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	ItemReference theItemRef;
	UInt16 numFlavors;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      DragObj_Convert, &theDrag,
	                      &theItemRef))
		return NULL;
	_err = CountDragItemFlavors(theDrag,
	                            theItemRef,
	                            &numFlavors);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("H",
	                     numFlavors);
	return _res;
}

static PyObject *Drag_GetFlavorType(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	ItemReference theItemRef;
	UInt16 index;
	FlavorType theType;
	if (!PyArg_ParseTuple(_args, "O&lH",
	                      DragObj_Convert, &theDrag,
	                      &theItemRef,
	                      &index))
		return NULL;
	_err = GetFlavorType(theDrag,
	                     theItemRef,
	                     index,
	                     &theType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildOSType, theType);
	return _res;
}

static PyObject *Drag_GetFlavorFlags(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	ItemReference theItemRef;
	FlavorType theType;
	FlavorFlags theFlags;
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      DragObj_Convert, &theDrag,
	                      &theItemRef,
	                      PyMac_GetOSType, &theType))
		return NULL;
	_err = GetFlavorFlags(theDrag,
	                      theItemRef,
	                      theType,
	                      &theFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     theFlags);
	return _res;
}

static PyObject *Drag_GetFlavorDataSize(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	ItemReference theItemRef;
	FlavorType theType;
	Size dataSize;
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      DragObj_Convert, &theDrag,
	                      &theItemRef,
	                      PyMac_GetOSType, &theType))
		return NULL;
	_err = GetFlavorDataSize(theDrag,
	                         theItemRef,
	                         theType,
	                         &dataSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     dataSize);
	return _res;
}

static PyObject *Drag_GetFlavorData(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	ItemReference theItemRef;
	FlavorType theType;
	char *dataPtr__out__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	UInt32 dataOffset;
	if (!PyArg_ParseTuple(_args, "O&lO&il",
	                      DragObj_Convert, &theDrag,
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
	_err = GetFlavorData(theDrag,
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

static PyObject *Drag_GetDragItemBounds(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	ItemReference theItemRef;
	Rect itemBounds;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      DragObj_Convert, &theDrag,
	                      &theItemRef))
		return NULL;
	_err = GetDragItemBounds(theDrag,
	                         theItemRef,
	                         &itemBounds);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &itemBounds);
	return _res;
}

static PyObject *Drag_SetDragItemBounds(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	ItemReference theItemRef;
	Rect itemBounds;
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      DragObj_Convert, &theDrag,
	                      &theItemRef,
	                      PyMac_GetRect, &itemBounds))
		return NULL;
	_err = SetDragItemBounds(theDrag,
	                         theItemRef,
	                         &itemBounds);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_GetDropLocation(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	AEDesc dropLocation;
	if (!PyArg_ParseTuple(_args, "O&",
	                      DragObj_Convert, &theDrag))
		return NULL;
	_err = GetDropLocation(theDrag,
	                       &dropLocation);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &dropLocation);
	return _res;
}

static PyObject *Drag_SetDropLocation(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	AEDesc dropLocation;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      DragObj_Convert, &theDrag,
	                      AEDesc_Convert, &dropLocation))
		return NULL;
	_err = SetDropLocation(theDrag,
	                       &dropLocation);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_GetDragAttributes(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	DragAttributes flags;
	if (!PyArg_ParseTuple(_args, "O&",
	                      DragObj_Convert, &theDrag))
		return NULL;
	_err = GetDragAttributes(theDrag,
	                         &flags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     flags);
	return _res;
}

static PyObject *Drag_GetDragMouse(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	Point mouse;
	Point globalPinnedMouse;
	if (!PyArg_ParseTuple(_args, "O&",
	                      DragObj_Convert, &theDrag))
		return NULL;
	_err = GetDragMouse(theDrag,
	                    &mouse,
	                    &globalPinnedMouse);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     PyMac_BuildPoint, mouse,
	                     PyMac_BuildPoint, globalPinnedMouse);
	return _res;
}

static PyObject *Drag_SetDragMouse(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	Point globalPinnedMouse;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      DragObj_Convert, &theDrag,
	                      PyMac_GetPoint, &globalPinnedMouse))
		return NULL;
	_err = SetDragMouse(theDrag,
	                    globalPinnedMouse);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_GetDragOrigin(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	Point globalInitialMouse;
	if (!PyArg_ParseTuple(_args, "O&",
	                      DragObj_Convert, &theDrag))
		return NULL;
	_err = GetDragOrigin(theDrag,
	                     &globalInitialMouse);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, globalInitialMouse);
	return _res;
}

static PyObject *Drag_GetDragModifiers(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	SInt16 modifiers;
	SInt16 mouseDownModifiers;
	SInt16 mouseUpModifiers;
	if (!PyArg_ParseTuple(_args, "O&",
	                      DragObj_Convert, &theDrag))
		return NULL;
	_err = GetDragModifiers(theDrag,
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

static PyObject *Drag_ShowDragHilite(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	RgnHandle hiliteFrame;
	Boolean inside;
	if (!PyArg_ParseTuple(_args, "O&O&b",
	                      DragObj_Convert, &theDrag,
	                      ResObj_Convert, &hiliteFrame,
	                      &inside))
		return NULL;
	_err = ShowDragHilite(theDrag,
	                      hiliteFrame,
	                      inside);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_HideDragHilite(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	if (!PyArg_ParseTuple(_args, "O&",
	                      DragObj_Convert, &theDrag))
		return NULL;
	_err = HideDragHilite(theDrag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_DragPreScroll(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	SInt16 dH;
	SInt16 dV;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      DragObj_Convert, &theDrag,
	                      &dH,
	                      &dV))
		return NULL;
	_err = DragPreScroll(theDrag,
	                     dH,
	                     dV);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_DragPostScroll(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	if (!PyArg_ParseTuple(_args, "O&",
	                      DragObj_Convert, &theDrag))
		return NULL;
	_err = DragPostScroll(theDrag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_UpdateDragHilite(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
	RgnHandle updateRgn;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      DragObj_Convert, &theDrag,
	                      ResObj_Convert, &updateRgn))
		return NULL;
	_err = UpdateDragHilite(theDrag,
	                        updateRgn);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Drag_GetDragHiliteColor(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Drag_WaitMouseMoved(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Drag_ZoomRects(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Drag_ZoomRegion(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Drag_InstallTrackingHandler(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Drag_InstallReceiveHandler(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Drag_RemoveTrackingHandler(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Drag_RemoveReceiveHandler(_self, _args)
	PyObject *_self;
	PyObject *_args;
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
	{"DisposeDrag", (PyCFunction)Drag_DisposeDrag, 1,
	 "(DragRef theDrag) -> None"},
	{"AddDragItemFlavor", (PyCFunction)Drag_AddDragItemFlavor, 1,
	 "(DragRef theDrag, ItemReference theItemRef, FlavorType theType, Buffer dataPtr, FlavorFlags theFlags) -> None"},
	{"SetDragItemFlavorData", (PyCFunction)Drag_SetDragItemFlavorData, 1,
	 "(DragRef theDrag, ItemReference theItemRef, FlavorType theType, Buffer dataPtr, UInt32 dataOffset) -> None"},
	{"SetDragImage", (PyCFunction)Drag_SetDragImage, 1,
	 "(DragRef theDrag, PixMapHandle imagePixMap, RgnHandle imageRgn, Point imageOffsetPt, DragImageFlags theImageFlags) -> None"},
	{"ChangeDragBehaviors", (PyCFunction)Drag_ChangeDragBehaviors, 1,
	 "(DragRef theDrag, DragBehaviors inBehaviorsToSet, DragBehaviors inBehaviorsToClear) -> None"},
	{"TrackDrag", (PyCFunction)Drag_TrackDrag, 1,
	 "(DragRef theDrag, EventRecord theEvent, RgnHandle theRegion) -> None"},
	{"CountDragItems", (PyCFunction)Drag_CountDragItems, 1,
	 "(DragRef theDrag) -> (UInt16 numItems)"},
	{"GetDragItemReferenceNumber", (PyCFunction)Drag_GetDragItemReferenceNumber, 1,
	 "(DragRef theDrag, UInt16 index) -> (ItemReference theItemRef)"},
	{"CountDragItemFlavors", (PyCFunction)Drag_CountDragItemFlavors, 1,
	 "(DragRef theDrag, ItemReference theItemRef) -> (UInt16 numFlavors)"},
	{"GetFlavorType", (PyCFunction)Drag_GetFlavorType, 1,
	 "(DragRef theDrag, ItemReference theItemRef, UInt16 index) -> (FlavorType theType)"},
	{"GetFlavorFlags", (PyCFunction)Drag_GetFlavorFlags, 1,
	 "(DragRef theDrag, ItemReference theItemRef, FlavorType theType) -> (FlavorFlags theFlags)"},
	{"GetFlavorDataSize", (PyCFunction)Drag_GetFlavorDataSize, 1,
	 "(DragRef theDrag, ItemReference theItemRef, FlavorType theType) -> (Size dataSize)"},
	{"GetFlavorData", (PyCFunction)Drag_GetFlavorData, 1,
	 "(DragRef theDrag, ItemReference theItemRef, FlavorType theType, Buffer dataPtr, UInt32 dataOffset) -> (Buffer dataPtr)"},
	{"GetDragItemBounds", (PyCFunction)Drag_GetDragItemBounds, 1,
	 "(DragRef theDrag, ItemReference theItemRef) -> (Rect itemBounds)"},
	{"SetDragItemBounds", (PyCFunction)Drag_SetDragItemBounds, 1,
	 "(DragRef theDrag, ItemReference theItemRef, Rect itemBounds) -> None"},
	{"GetDropLocation", (PyCFunction)Drag_GetDropLocation, 1,
	 "(DragRef theDrag) -> (AEDesc dropLocation)"},
	{"SetDropLocation", (PyCFunction)Drag_SetDropLocation, 1,
	 "(DragRef theDrag, AEDesc dropLocation) -> None"},
	{"GetDragAttributes", (PyCFunction)Drag_GetDragAttributes, 1,
	 "(DragRef theDrag) -> (DragAttributes flags)"},
	{"GetDragMouse", (PyCFunction)Drag_GetDragMouse, 1,
	 "(DragRef theDrag) -> (Point mouse, Point globalPinnedMouse)"},
	{"SetDragMouse", (PyCFunction)Drag_SetDragMouse, 1,
	 "(DragRef theDrag, Point globalPinnedMouse) -> None"},
	{"GetDragOrigin", (PyCFunction)Drag_GetDragOrigin, 1,
	 "(DragRef theDrag) -> (Point globalInitialMouse)"},
	{"GetDragModifiers", (PyCFunction)Drag_GetDragModifiers, 1,
	 "(DragRef theDrag) -> (SInt16 modifiers, SInt16 mouseDownModifiers, SInt16 mouseUpModifiers)"},
	{"ShowDragHilite", (PyCFunction)Drag_ShowDragHilite, 1,
	 "(DragRef theDrag, RgnHandle hiliteFrame, Boolean inside) -> None"},
	{"HideDragHilite", (PyCFunction)Drag_HideDragHilite, 1,
	 "(DragRef theDrag) -> None"},
	{"DragPreScroll", (PyCFunction)Drag_DragPreScroll, 1,
	 "(DragRef theDrag, SInt16 dH, SInt16 dV) -> None"},
	{"DragPostScroll", (PyCFunction)Drag_DragPostScroll, 1,
	 "(DragRef theDrag) -> None"},
	{"UpdateDragHilite", (PyCFunction)Drag_UpdateDragHilite, 1,
	 "(DragRef theDrag, RgnHandle updateRgn) -> None"},
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



void initDrag()
{
	PyObject *m;
	PyObject *d;




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

	dragglue_TrackingHandlerUPP = NewDragTrackingHandlerProc(dragglue_TrackingHandler);
	dragglue_ReceiveHandlerUPP = NewDragReceiveHandlerProc(dragglue_ReceiveHandler);
	dragglue_SendDataUPP = NewDragSendDataProc(dragglue_SendData);
#if 0
	dragglue_InputUPP = NewDragInputProc(dragglue_Input);
	dragglue_DrawingUPP = NewDragDrawingProc(dragglue_Drawing);
#endif


}

/* ======================== End module Drag ========================= */

