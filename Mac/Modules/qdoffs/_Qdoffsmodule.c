
/* ========================= Module _Qdoffs ========================= */

#include "Python.h"


#ifndef __LP64__

#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    PyErr_SetString(PyExc_NotImplementedError, \
    "Not available in this shared library/OS version"); \
    return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_GWorldObj_New(GWorldPtr);
extern int _GWorldObj_Convert(PyObject *, GWorldPtr *);

#define GWorldObj_New _GWorldObj_New
#define GWorldObj_Convert _GWorldObj_Convert
#endif

#define as_GrafPtr(gworld) ((GrafPtr)(gworld))


static PyObject *Qdoffs_Error;

/* ----------------------- Object type GWorld ----------------------- */

PyTypeObject GWorld_Type;

#define GWorldObj_Check(x) ((x)->ob_type == &GWorld_Type || PyObject_TypeCheck((x), &GWorld_Type))

typedef struct GWorldObject {
    PyObject_HEAD
    GWorldPtr ob_itself;
} GWorldObject;

PyObject *GWorldObj_New(GWorldPtr itself)
{
    GWorldObject *it;
    if (itself == NULL) return PyMac_Error(resNotFound);
    it = PyObject_NEW(GWorldObject, &GWorld_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    return (PyObject *)it;
}

int GWorldObj_Convert(PyObject *v, GWorldPtr *p_itself)
{
    if (!GWorldObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "GWorld required");
        return 0;
    }
    *p_itself = ((GWorldObject *)v)->ob_itself;
    return 1;
}

static void GWorldObj_dealloc(GWorldObject *self)
{
    DisposeGWorld(self->ob_itself);
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *GWorldObj_GetGWorldDevice(GWorldObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    GDHandle _rv;
#ifndef GetGWorldDevice
    PyMac_PRECHECK(GetGWorldDevice);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetGWorldDevice(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         ResObj_New, _rv);
    return _res;
}

static PyObject *GWorldObj_GetGWorldPixMap(GWorldObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    PixMapHandle _rv;
#ifndef GetGWorldPixMap
    PyMac_PRECHECK(GetGWorldPixMap);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetGWorldPixMap(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         ResObj_New, _rv);
    return _res;
}

static PyObject *GWorldObj_as_GrafPtr(GWorldObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    GrafPtr _rv;
#ifndef as_GrafPtr
    PyMac_PRECHECK(as_GrafPtr);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = as_GrafPtr(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         GrafObj_New, _rv);
    return _res;
}

static PyMethodDef GWorldObj_methods[] = {
    {"GetGWorldDevice", (PyCFunction)GWorldObj_GetGWorldDevice, 1,
     PyDoc_STR("() -> (GDHandle _rv)")},
    {"GetGWorldPixMap", (PyCFunction)GWorldObj_GetGWorldPixMap, 1,
     PyDoc_STR("() -> (PixMapHandle _rv)")},
    {"as_GrafPtr", (PyCFunction)GWorldObj_as_GrafPtr, 1,
     PyDoc_STR("() -> (GrafPtr _rv)")},
    {NULL, NULL, 0}
};

#define GWorldObj_getsetlist NULL


#define GWorldObj_compare NULL

#define GWorldObj_repr NULL

#define GWorldObj_hash NULL
#define GWorldObj_tp_init 0

#define GWorldObj_tp_alloc PyType_GenericAlloc

static PyObject *GWorldObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *_self;
    GWorldPtr itself;
    char *kw[] = {"itself", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, GWorldObj_Convert, &itself)) return NULL;
    if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((GWorldObject *)_self)->ob_itself = itself;
    return _self;
}

#define GWorldObj_tp_free PyObject_Del


PyTypeObject GWorld_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_Qdoffs.GWorld", /*tp_name*/
    sizeof(GWorldObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) GWorldObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) GWorldObj_compare, /*tp_compare*/
    (reprfunc) GWorldObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) GWorldObj_hash, /*tp_hash*/
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
    GWorldObj_methods, /* tp_methods */
    0, /*tp_members*/
    GWorldObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    GWorldObj_tp_init, /* tp_init */
    GWorldObj_tp_alloc, /* tp_alloc */
    GWorldObj_tp_new, /* tp_new */
    GWorldObj_tp_free, /* tp_free */
};

/* --------------------- End object type GWorld --------------------- */


static PyObject *Qdoffs_NewGWorld(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    QDErr _err;
    GWorldPtr offscreenGWorld;
    short PixelDepth;
    Rect boundsRect;
    CTabHandle cTable;
    GDHandle aGDevice;
    GWorldFlags flags;
#ifndef NewGWorld
    PyMac_PRECHECK(NewGWorld);
#endif
    if (!PyArg_ParseTuple(_args, "hO&O&O&l",
                          &PixelDepth,
                          PyMac_GetRect, &boundsRect,
                          OptResObj_Convert, &cTable,
                          OptResObj_Convert, &aGDevice,
                          &flags))
        return NULL;
    _err = NewGWorld(&offscreenGWorld,
                     PixelDepth,
                     &boundsRect,
                     cTable,
                     aGDevice,
                     flags);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         GWorldObj_New, offscreenGWorld);
    return _res;
}

static PyObject *Qdoffs_LockPixels(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    PixMapHandle pm;
#ifndef LockPixels
    PyMac_PRECHECK(LockPixels);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &pm))
        return NULL;
    _rv = LockPixels(pm);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *Qdoffs_UnlockPixels(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    PixMapHandle pm;
#ifndef UnlockPixels
    PyMac_PRECHECK(UnlockPixels);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &pm))
        return NULL;
    UnlockPixels(pm);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Qdoffs_UpdateGWorld(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    GWorldFlags _rv;
    GWorldPtr offscreenGWorld;
    short pixelDepth;
    Rect boundsRect;
    CTabHandle cTable;
    GDHandle aGDevice;
    GWorldFlags flags;
#ifndef UpdateGWorld
    PyMac_PRECHECK(UpdateGWorld);
#endif
    if (!PyArg_ParseTuple(_args, "hO&O&O&l",
                          &pixelDepth,
                          PyMac_GetRect, &boundsRect,
                          OptResObj_Convert, &cTable,
                          OptResObj_Convert, &aGDevice,
                          &flags))
        return NULL;
    _rv = UpdateGWorld(&offscreenGWorld,
                       pixelDepth,
                       &boundsRect,
                       cTable,
                       aGDevice,
                       flags);
    _res = Py_BuildValue("lO&",
                         _rv,
                         GWorldObj_New, offscreenGWorld);
    return _res;
}

static PyObject *Qdoffs_GetGWorld(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CGrafPtr port;
    GDHandle gdh;
#ifndef GetGWorld
    PyMac_PRECHECK(GetGWorld);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    GetGWorld(&port,
              &gdh);
    _res = Py_BuildValue("O&O&",
                         GrafObj_New, port,
                         ResObj_New, gdh);
    return _res;
}

static PyObject *Qdoffs_SetGWorld(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CGrafPtr port;
    GDHandle gdh;
#ifndef SetGWorld
    PyMac_PRECHECK(SetGWorld);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          GrafObj_Convert, &port,
                          OptResObj_Convert, &gdh))
        return NULL;
    SetGWorld(port,
              gdh);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Qdoffs_CTabChanged(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CTabHandle ctab;
#ifndef CTabChanged
    PyMac_PRECHECK(CTabChanged);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          OptResObj_Convert, &ctab))
        return NULL;
    CTabChanged(ctab);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Qdoffs_PixPatChanged(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    PixPatHandle ppat;
#ifndef PixPatChanged
    PyMac_PRECHECK(PixPatChanged);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &ppat))
        return NULL;
    PixPatChanged(ppat);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Qdoffs_PortChanged(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    GrafPtr port;
#ifndef PortChanged
    PyMac_PRECHECK(PortChanged);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          GrafObj_Convert, &port))
        return NULL;
    PortChanged(port);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Qdoffs_GDeviceChanged(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    GDHandle gdh;
#ifndef GDeviceChanged
    PyMac_PRECHECK(GDeviceChanged);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          OptResObj_Convert, &gdh))
        return NULL;
    GDeviceChanged(gdh);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Qdoffs_AllowPurgePixels(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    PixMapHandle pm;
#ifndef AllowPurgePixels
    PyMac_PRECHECK(AllowPurgePixels);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &pm))
        return NULL;
    AllowPurgePixels(pm);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Qdoffs_NoPurgePixels(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    PixMapHandle pm;
#ifndef NoPurgePixels
    PyMac_PRECHECK(NoPurgePixels);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &pm))
        return NULL;
    NoPurgePixels(pm);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Qdoffs_GetPixelsState(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    GWorldFlags _rv;
    PixMapHandle pm;
#ifndef GetPixelsState
    PyMac_PRECHECK(GetPixelsState);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &pm))
        return NULL;
    _rv = GetPixelsState(pm);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *Qdoffs_SetPixelsState(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    PixMapHandle pm;
    GWorldFlags state;
#ifndef SetPixelsState
    PyMac_PRECHECK(SetPixelsState);
#endif
    if (!PyArg_ParseTuple(_args, "O&l",
                          ResObj_Convert, &pm,
                          &state))
        return NULL;
    SetPixelsState(pm,
                   state);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Qdoffs_GetPixRowBytes(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    long _rv;
    PixMapHandle pm;
#ifndef GetPixRowBytes
    PyMac_PRECHECK(GetPixRowBytes);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &pm))
        return NULL;
    _rv = GetPixRowBytes(pm);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *Qdoffs_NewScreenBuffer(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    QDErr _err;
    Rect globalRect;
    Boolean purgeable;
    GDHandle gdh;
    PixMapHandle offscreenPixMap;
#ifndef NewScreenBuffer
    PyMac_PRECHECK(NewScreenBuffer);
#endif
    if (!PyArg_ParseTuple(_args, "O&b",
                          PyMac_GetRect, &globalRect,
                          &purgeable))
        return NULL;
    _err = NewScreenBuffer(&globalRect,
                           purgeable,
                           &gdh,
                           &offscreenPixMap);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&O&",
                         ResObj_New, gdh,
                         ResObj_New, offscreenPixMap);
    return _res;
}

static PyObject *Qdoffs_DisposeScreenBuffer(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    PixMapHandle offscreenPixMap;
#ifndef DisposeScreenBuffer
    PyMac_PRECHECK(DisposeScreenBuffer);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &offscreenPixMap))
        return NULL;
    DisposeScreenBuffer(offscreenPixMap);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Qdoffs_QDDone(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    GrafPtr port;
#ifndef QDDone
    PyMac_PRECHECK(QDDone);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          GrafObj_Convert, &port))
        return NULL;
    _rv = QDDone(port);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *Qdoffs_OffscreenVersion(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    long _rv;
#ifndef OffscreenVersion
    PyMac_PRECHECK(OffscreenVersion);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = OffscreenVersion();
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *Qdoffs_NewTempScreenBuffer(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    QDErr _err;
    Rect globalRect;
    Boolean purgeable;
    GDHandle gdh;
    PixMapHandle offscreenPixMap;
#ifndef NewTempScreenBuffer
    PyMac_PRECHECK(NewTempScreenBuffer);
#endif
    if (!PyArg_ParseTuple(_args, "O&b",
                          PyMac_GetRect, &globalRect,
                          &purgeable))
        return NULL;
    _err = NewTempScreenBuffer(&globalRect,
                               purgeable,
                               &gdh,
                               &offscreenPixMap);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&O&",
                         ResObj_New, gdh,
                         ResObj_New, offscreenPixMap);
    return _res;
}

static PyObject *Qdoffs_PixMap32Bit(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    PixMapHandle pmHandle;
#ifndef PixMap32Bit
    PyMac_PRECHECK(PixMap32Bit);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &pmHandle))
        return NULL;
    _rv = PixMap32Bit(pmHandle);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *Qdoffs_GetPixMapBytes(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    PixMapHandle pm;
    int from, length;
    char *cp;

    if ( !PyArg_ParseTuple(_args, "O&ii", ResObj_Convert, &pm, &from, &length) )
        return NULL;
    cp = GetPixBaseAddr(pm)+from;
    _res = PyString_FromStringAndSize(cp, length);
    return _res;

}

static PyObject *Qdoffs_PutPixMapBytes(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    PixMapHandle pm;
    int from, length;
    char *cp, *icp;

    if ( !PyArg_ParseTuple(_args, "O&is#", ResObj_Convert, &pm, &from, &icp, &length) )
        return NULL;
    cp = GetPixBaseAddr(pm)+from;
    memcpy(cp, icp, length);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;

}
#endif /* __LP64__ */

static PyMethodDef Qdoffs_methods[] = {
#ifndef __LP64__
    {"NewGWorld", (PyCFunction)Qdoffs_NewGWorld, 1,
     PyDoc_STR("(short PixelDepth, Rect boundsRect, CTabHandle cTable, GDHandle aGDevice, GWorldFlags flags) -> (GWorldPtr offscreenGWorld)")},
    {"LockPixels", (PyCFunction)Qdoffs_LockPixels, 1,
     PyDoc_STR("(PixMapHandle pm) -> (Boolean _rv)")},
    {"UnlockPixels", (PyCFunction)Qdoffs_UnlockPixels, 1,
     PyDoc_STR("(PixMapHandle pm) -> None")},
    {"UpdateGWorld", (PyCFunction)Qdoffs_UpdateGWorld, 1,
     PyDoc_STR("(short pixelDepth, Rect boundsRect, CTabHandle cTable, GDHandle aGDevice, GWorldFlags flags) -> (GWorldFlags _rv, GWorldPtr offscreenGWorld)")},
    {"GetGWorld", (PyCFunction)Qdoffs_GetGWorld, 1,
     PyDoc_STR("() -> (CGrafPtr port, GDHandle gdh)")},
    {"SetGWorld", (PyCFunction)Qdoffs_SetGWorld, 1,
     PyDoc_STR("(CGrafPtr port, GDHandle gdh) -> None")},
    {"CTabChanged", (PyCFunction)Qdoffs_CTabChanged, 1,
     PyDoc_STR("(CTabHandle ctab) -> None")},
    {"PixPatChanged", (PyCFunction)Qdoffs_PixPatChanged, 1,
     PyDoc_STR("(PixPatHandle ppat) -> None")},
    {"PortChanged", (PyCFunction)Qdoffs_PortChanged, 1,
     PyDoc_STR("(GrafPtr port) -> None")},
    {"GDeviceChanged", (PyCFunction)Qdoffs_GDeviceChanged, 1,
     PyDoc_STR("(GDHandle gdh) -> None")},
    {"AllowPurgePixels", (PyCFunction)Qdoffs_AllowPurgePixels, 1,
     PyDoc_STR("(PixMapHandle pm) -> None")},
    {"NoPurgePixels", (PyCFunction)Qdoffs_NoPurgePixels, 1,
     PyDoc_STR("(PixMapHandle pm) -> None")},
    {"GetPixelsState", (PyCFunction)Qdoffs_GetPixelsState, 1,
     PyDoc_STR("(PixMapHandle pm) -> (GWorldFlags _rv)")},
    {"SetPixelsState", (PyCFunction)Qdoffs_SetPixelsState, 1,
     PyDoc_STR("(PixMapHandle pm, GWorldFlags state) -> None")},
    {"GetPixRowBytes", (PyCFunction)Qdoffs_GetPixRowBytes, 1,
     PyDoc_STR("(PixMapHandle pm) -> (long _rv)")},
    {"NewScreenBuffer", (PyCFunction)Qdoffs_NewScreenBuffer, 1,
     PyDoc_STR("(Rect globalRect, Boolean purgeable) -> (GDHandle gdh, PixMapHandle offscreenPixMap)")},
    {"DisposeScreenBuffer", (PyCFunction)Qdoffs_DisposeScreenBuffer, 1,
     PyDoc_STR("(PixMapHandle offscreenPixMap) -> None")},
    {"QDDone", (PyCFunction)Qdoffs_QDDone, 1,
     PyDoc_STR("(GrafPtr port) -> (Boolean _rv)")},
    {"OffscreenVersion", (PyCFunction)Qdoffs_OffscreenVersion, 1,
     PyDoc_STR("() -> (long _rv)")},
    {"NewTempScreenBuffer", (PyCFunction)Qdoffs_NewTempScreenBuffer, 1,
     PyDoc_STR("(Rect globalRect, Boolean purgeable) -> (GDHandle gdh, PixMapHandle offscreenPixMap)")},
    {"PixMap32Bit", (PyCFunction)Qdoffs_PixMap32Bit, 1,
     PyDoc_STR("(PixMapHandle pmHandle) -> (Boolean _rv)")},
    {"GetPixMapBytes", (PyCFunction)Qdoffs_GetPixMapBytes, 1,
     PyDoc_STR("(pixmap, int start, int size) -> string. Return bytes from the pixmap")},
    {"PutPixMapBytes", (PyCFunction)Qdoffs_PutPixMapBytes, 1,
     PyDoc_STR("(pixmap, int start, string data). Store bytes into the pixmap")},
#endif /* __LP64__ */
    {NULL, NULL, 0}
};




void init_Qdoffs(void)
{
    PyObject *m;
#ifndef __LP64__
    PyObject *d;



        PyMac_INIT_TOOLBOX_OBJECT_NEW(GWorldPtr, GWorldObj_New);
        PyMac_INIT_TOOLBOX_OBJECT_CONVERT(GWorldPtr, GWorldObj_Convert);

#endif /* __LP64__ */

    m = Py_InitModule("_Qdoffs", Qdoffs_methods);
#ifndef __LP64__
    d = PyModule_GetDict(m);
    Qdoffs_Error = PyMac_GetOSErrException();
    if (Qdoffs_Error == NULL ||
        PyDict_SetItemString(d, "Error", Qdoffs_Error) != 0)
        return;
    GWorld_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&GWorld_Type) < 0) return;
    Py_INCREF(&GWorld_Type);
    PyModule_AddObject(m, "GWorld", (PyObject *)&GWorld_Type);
    /* Backward-compatible name */
    Py_INCREF(&GWorld_Type);
    PyModule_AddObject(m, "GWorldType", (PyObject *)&GWorld_Type);
#endif /* __LP64__ */
}

/* ======================= End module _Qdoffs ======================= */

