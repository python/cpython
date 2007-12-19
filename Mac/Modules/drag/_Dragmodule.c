
/* ========================== Module _Drag ========================== */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
        "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>

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

#define DragObj_Check(x) (Py_TYPE(x) == &DragObj_Type || PyObject_TypeCheck((x), &DragObj_Type))

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

int DragObj_Convert(PyObject *v, DragRef *p_itself)
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
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *DragObj_DisposeDrag(DragObjObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef DisposeDrag
	PyMac_PRECHECK(DisposeDrag);
#endif
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
#ifndef AddDragItemFlavor
	PyMac_PRECHECK(AddDragItemFlavor);
#endif
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
#ifndef SetDragItemFlavorData
	PyMac_PRECHECK(SetDragItemFlavorData);
#endif
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
#ifndef SetDragImage
	PyMac_PRECHECK(SetDragImage);
#endif
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
#ifndef ChangeDragBehaviors
	PyMac_PRECHECK(ChangeDragBehaviors);
#endif
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
#ifndef TrackDrag
	PyMac_PRECHECK(TrackDrag);
#endif
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
#ifndef CountDragItems
	PyMac_PRECHECK(CountDragItems);
#endif
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
#ifndef GetDragItemReferenceNumber
	PyMac_PRECHECK(GetDragItemReferenceNumber);
#endif
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
#ifndef CountDragItemFlavors
	PyMac_PRECHECK(CountDragItemFlavors);
#endif
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
#ifndef GetFlavorType
	PyMac_PRECHECK(GetFlavorType);
#endif
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
#ifndef GetFlavorFlags
	PyMac_PRECHECK(GetFlavorFlags);
#endif
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
#ifndef GetFlavorDataSize
	PyMac_PRECHECK(GetFlavorDataSize);
#endif
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
#ifndef GetFlavorData
	PyMac_PRECHECK(GetFlavorData);
#endif
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
#ifndef GetDragItemBounds
	PyMac_PRECHECK(GetDragItemBounds);
#endif
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
#ifndef SetDragItemBounds
	PyMac_PRECHECK(SetDragItemBounds);
#endif
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
#ifndef GetDropLocation
	PyMac_PRECHECK(GetDropLocation);
#endif
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
#ifndef SetDropLocation
	PyMac_PRECHECK(SetDropLocation);
#endif
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
#ifndef GetDragAttributes
	PyMac_PRECHECK(GetDragAttributes);
#endif
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
#ifndef GetDragMouse
	PyMac_PRECHECK(GetDragMouse);
#endif
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
#ifndef SetDragMouse
	PyMac_PRECHECK(SetDragMouse);
#endif
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
#ifndef GetDragOrigin
	PyMac_PRECHECK(GetDragOrigin);
#endif
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
#ifndef GetDragModifiers
	PyMac_PRECHECK(GetDragModifiers);
#endif
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
#ifndef ShowDragHilite
	PyMac_PRECHECK(ShowDragHilite);
#endif
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
#ifndef HideDragHilite
	PyMac_PRECHECK(HideDragHilite);
#endif
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
#ifndef DragPreScroll
	PyMac_PRECHECK(DragPreScroll);
#endif
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
#ifndef DragPostScroll
	PyMac_PRECHECK(DragPostScroll);
#endif
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
#ifndef UpdateDragHilite
	PyMac_PRECHECK(UpdateDragHilite);
#endif
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
	 PyDoc_STR("() -> None")},
	{"AddDragItemFlavor", (PyCFunction)DragObj_AddDragItemFlavor, 1,
	 PyDoc_STR("(ItemReference theItemRef, FlavorType theType, Buffer dataPtr, FlavorFlags theFlags) -> None")},
	{"SetDragItemFlavorData", (PyCFunction)DragObj_SetDragItemFlavorData, 1,
	 PyDoc_STR("(ItemReference theItemRef, FlavorType theType, Buffer dataPtr, UInt32 dataOffset) -> None")},
	{"SetDragImage", (PyCFunction)DragObj_SetDragImage, 1,
	 PyDoc_STR("(PixMapHandle imagePixMap, RgnHandle imageRgn, Point imageOffsetPt, DragImageFlags theImageFlags) -> None")},
	{"ChangeDragBehaviors", (PyCFunction)DragObj_ChangeDragBehaviors, 1,
	 PyDoc_STR("(DragBehaviors inBehaviorsToSet, DragBehaviors inBehaviorsToClear) -> None")},
	{"TrackDrag", (PyCFunction)DragObj_TrackDrag, 1,
	 PyDoc_STR("(EventRecord theEvent, RgnHandle theRegion) -> None")},
	{"CountDragItems", (PyCFunction)DragObj_CountDragItems, 1,
	 PyDoc_STR("() -> (UInt16 numItems)")},
	{"GetDragItemReferenceNumber", (PyCFunction)DragObj_GetDragItemReferenceNumber, 1,
	 PyDoc_STR("(UInt16 index) -> (ItemReference theItemRef)")},
	{"CountDragItemFlavors", (PyCFunction)DragObj_CountDragItemFlavors, 1,
	 PyDoc_STR("(ItemReference theItemRef) -> (UInt16 numFlavors)")},
	{"GetFlavorType", (PyCFunction)DragObj_GetFlavorType, 1,
	 PyDoc_STR("(ItemReference theItemRef, UInt16 index) -> (FlavorType theType)")},
	{"GetFlavorFlags", (PyCFunction)DragObj_GetFlavorFlags, 1,
	 PyDoc_STR("(ItemReference theItemRef, FlavorType theType) -> (FlavorFlags theFlags)")},
	{"GetFlavorDataSize", (PyCFunction)DragObj_GetFlavorDataSize, 1,
	 PyDoc_STR("(ItemReference theItemRef, FlavorType theType) -> (Size dataSize)")},
	{"GetFlavorData", (PyCFunction)DragObj_GetFlavorData, 1,
	 PyDoc_STR("(ItemReference theItemRef, FlavorType theType, Buffer dataPtr, UInt32 dataOffset) -> (Buffer dataPtr)")},
	{"GetDragItemBounds", (PyCFunction)DragObj_GetDragItemBounds, 1,
	 PyDoc_STR("(ItemReference theItemRef) -> (Rect itemBounds)")},
	{"SetDragItemBounds", (PyCFunction)DragObj_SetDragItemBounds, 1,
	 PyDoc_STR("(ItemReference theItemRef, Rect itemBounds) -> None")},
	{"GetDropLocation", (PyCFunction)DragObj_GetDropLocation, 1,
	 PyDoc_STR("() -> (AEDesc dropLocation)")},
	{"SetDropLocation", (PyCFunction)DragObj_SetDropLocation, 1,
	 PyDoc_STR("(AEDesc dropLocation) -> None")},
	{"GetDragAttributes", (PyCFunction)DragObj_GetDragAttributes, 1,
	 PyDoc_STR("() -> (DragAttributes flags)")},
	{"GetDragMouse", (PyCFunction)DragObj_GetDragMouse, 1,
	 PyDoc_STR("() -> (Point mouse, Point globalPinnedMouse)")},
	{"SetDragMouse", (PyCFunction)DragObj_SetDragMouse, 1,
	 PyDoc_STR("(Point globalPinnedMouse) -> None")},
	{"GetDragOrigin", (PyCFunction)DragObj_GetDragOrigin, 1,
	 PyDoc_STR("() -> (Point globalInitialMouse)")},
	{"GetDragModifiers", (PyCFunction)DragObj_GetDragModifiers, 1,
	 PyDoc_STR("() -> (SInt16 modifiers, SInt16 mouseDownModifiers, SInt16 mouseUpModifiers)")},
	{"ShowDragHilite", (PyCFunction)DragObj_ShowDragHilite, 1,
	 PyDoc_STR("(RgnHandle hiliteFrame, Boolean inside) -> None")},
	{"HideDragHilite", (PyCFunction)DragObj_HideDragHilite, 1,
	 PyDoc_STR("() -> None")},
	{"DragPreScroll", (PyCFunction)DragObj_DragPreScroll, 1,
	 PyDoc_STR("(SInt16 dH, SInt16 dV) -> None")},
	{"DragPostScroll", (PyCFunction)DragObj_DragPostScroll, 1,
	 PyDoc_STR("() -> None")},
	{"UpdateDragHilite", (PyCFunction)DragObj_UpdateDragHilite, 1,
	 PyDoc_STR("(RgnHandle updateRgn) -> None")},
	{NULL, NULL, 0}
};

#define DragObj_getsetlist NULL


#define DragObj_compare NULL

#define DragObj_repr NULL

#define DragObj_hash NULL
#define DragObj_tp_init 0

#define DragObj_tp_alloc PyType_GenericAlloc

static PyObject *DragObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	DragRef itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, DragObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((DragObjObject *)_self)->ob_itself = itself;
	return _self;
}

#define DragObj_tp_free PyObject_Del


PyTypeObject DragObj_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_Drag.DragObj", /*tp_name*/
	sizeof(DragObjObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) DragObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) DragObj_compare, /*tp_compare*/
	(reprfunc) DragObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) DragObj_hash, /*tp_hash*/
	0, /*tp_call*/
	0, /*tp_str*/
	PyObject_GenericGetAttr, /*tp_getattro*/
	PyObject_GenericSetAttr, /*tp_setattro */
	0, /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
	0, /*tp_doc*/
	0, /*tp_traverse*/
	0, /*tp_clear*/
	0, /*tp_richcompare*/
	0, /*tp_weaklistoffset*/
	0, /*tp_iter*/
	0, /*tp_iternext*/
	DragObj_methods, /* tp_methods */
	0, /*tp_members*/
	DragObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	DragObj_tp_init, /* tp_init */
	DragObj_tp_alloc, /* tp_alloc */
	DragObj_tp_new, /* tp_new */
	DragObj_tp_free, /* tp_free */
};

/* -------------------- End object type DragObj --------------------- */


static PyObject *Drag_NewDrag(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DragRef theDrag;
#ifndef NewDrag
	PyMac_PRECHECK(NewDrag);
#endif
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
#ifndef GetDragHiliteColor
	PyMac_PRECHECK(GetDragHiliteColor);
#endif
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
#ifndef WaitMouseMoved
	PyMac_PRECHECK(WaitMouseMoved);
#endif
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
#ifndef ZoomRects
	PyMac_PRECHECK(ZoomRects);
#endif
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
#ifndef ZoomRegion
	PyMac_PRECHECK(ZoomRegion);
#endif
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
	    Py_INCREF(callback);        /* Cannot decref later, too bad */
	    _err = InstallTrackingHandler(dragglue_TrackingHandlerUPP, theWindow, (void *)callback);
	        if (_err != noErr) return PyMac_Error(_err);
	        Py_INCREF(Py_None);
	        _res = Py_None;
	        return _res;

}

static PyObject *Drag_InstallReceiveHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	    PyObject *callback;
	    WindowPtr theWindow = NULL;
	    OSErr _err;

	    if ( !PyArg_ParseTuple(_args, "O|O&", &callback, WinObj_Convert, &theWindow) )
	        return NULL;
	    Py_INCREF(callback);        /* Cannot decref later, too bad */
	    _err = InstallReceiveHandler(dragglue_ReceiveHandlerUPP, theWindow, (void *)callback);
	        if (_err != noErr) return PyMac_Error(_err);
	        Py_INCREF(Py_None);
	        _res = Py_None;
	        return _res;

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
	        _res = Py_None;
	        return _res;

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
	        _res = Py_None;
	        return _res;

}

static PyMethodDef Drag_methods[] = {
	{"NewDrag", (PyCFunction)Drag_NewDrag, 1,
	 PyDoc_STR("() -> (DragRef theDrag)")},
	{"GetDragHiliteColor", (PyCFunction)Drag_GetDragHiliteColor, 1,
	 PyDoc_STR("(WindowPtr window) -> (RGBColor color)")},
	{"WaitMouseMoved", (PyCFunction)Drag_WaitMouseMoved, 1,
	 PyDoc_STR("(Point initialMouse) -> (Boolean _rv)")},
	{"ZoomRects", (PyCFunction)Drag_ZoomRects, 1,
	 PyDoc_STR("(Rect fromRect, Rect toRect, SInt16 zoomSteps, ZoomAcceleration acceleration) -> None")},
	{"ZoomRegion", (PyCFunction)Drag_ZoomRegion, 1,
	 PyDoc_STR("(RgnHandle region, Point zoomDistance, SInt16 zoomSteps, ZoomAcceleration acceleration) -> None")},
	{"InstallTrackingHandler", (PyCFunction)Drag_InstallTrackingHandler, 1,
	 PyDoc_STR(NULL)},
	{"InstallReceiveHandler", (PyCFunction)Drag_InstallReceiveHandler, 1,
	 PyDoc_STR(NULL)},
	{"RemoveTrackingHandler", (PyCFunction)Drag_RemoveTrackingHandler, 1,
	 PyDoc_STR(NULL)},
	{"RemoveReceiveHandler", (PyCFunction)Drag_RemoveReceiveHandler, 1,
	 PyDoc_STR(NULL)},
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
                PySys_WriteStderr("Drag: Exception in TrackingHandler\n");
                PyErr_Print();
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
                PySys_WriteStderr("Drag: Exception in ReceiveHandler\n");
                PyErr_Print();
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
                PySys_WriteStderr("Drag: Exception in SendDataHandler\n");
                PyErr_Print();
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



void init_Drag(void)
{
	PyObject *m;
	PyObject *d;



	        PyMac_INIT_TOOLBOX_OBJECT_NEW(DragRef, DragObj_New);
	        PyMac_INIT_TOOLBOX_OBJECT_CONVERT(DragRef, DragObj_Convert);


	m = Py_InitModule("_Drag", Drag_methods);
	d = PyModule_GetDict(m);
	Drag_Error = PyMac_GetOSErrException();
	if (Drag_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Drag_Error) != 0)
		return;
	Py_TYPE(&DragObj_Type) = &PyType_Type;
	if (PyType_Ready(&DragObj_Type) < 0) return;
	Py_INCREF(&DragObj_Type);
	PyModule_AddObject(m, "DragObj", (PyObject *)&DragObj_Type);
	/* Backward-compatible name */
	Py_INCREF(&DragObj_Type);
	PyModule_AddObject(m, "DragObjType", (PyObject *)&DragObj_Type);

	dragglue_TrackingHandlerUPP = NewDragTrackingHandlerUPP(dragglue_TrackingHandler);
	dragglue_ReceiveHandlerUPP = NewDragReceiveHandlerUPP(dragglue_ReceiveHandler);
	dragglue_SendDataUPP = NewDragSendDataUPP(dragglue_SendData);
#if 0
	dragglue_InputUPP = NewDragInputUPP(dragglue_Input);
	dragglue_DrawingUPP = NewDragDrawingUPP(dragglue_Drawing);
#endif


}

/* ======================== End module _Drag ======================== */

