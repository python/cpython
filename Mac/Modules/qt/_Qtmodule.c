
/* =========================== Module _Qt =========================== */

#include "Python.h"


#ifndef __LP64__

#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
        "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


#include <QuickTime/QuickTime.h>


#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_TrackObj_New(Track);
extern int _TrackObj_Convert(PyObject *, Track *);
extern PyObject *_MovieObj_New(Movie);
extern int _MovieObj_Convert(PyObject *, Movie *);
extern PyObject *_MovieCtlObj_New(MovieController);
extern int _MovieCtlObj_Convert(PyObject *, MovieController *);
extern PyObject *_TimeBaseObj_New(TimeBase);
extern int _TimeBaseObj_Convert(PyObject *, TimeBase *);
extern PyObject *_UserDataObj_New(UserData);
extern int _UserDataObj_Convert(PyObject *, UserData *);
extern PyObject *_MediaObj_New(Media);
extern int _MediaObj_Convert(PyObject *, Media *);

#define TrackObj_New _TrackObj_New
#define TrackObj_Convert _TrackObj_Convert
#define MovieObj_New _MovieObj_New
#define MovieObj_Convert _MovieObj_Convert
#define MovieCtlObj_New _MovieCtlObj_New
#define MovieCtlObj_Convert _MovieCtlObj_Convert
#define TimeBaseObj_New _TimeBaseObj_New
#define TimeBaseObj_Convert _TimeBaseObj_Convert
#define UserDataObj_New _UserDataObj_New
#define UserDataObj_Convert _UserDataObj_Convert
#define MediaObj_New _MediaObj_New
#define MediaObj_Convert _MediaObj_Convert
#endif

/* Macro to allow us to GetNextInterestingTime without duration */
#define GetMediaNextInterestingTimeOnly(media, flags, time, rate, rv)                         GetMediaNextInterestingTime(media, flags, time, rate, rv, NULL)

/*
** Parse/generate time records
*/
static PyObject *
QtTimeRecord_New(TimeRecord *itself)
{
        if (itself->base)
                return Py_BuildValue("O&lO&", PyMac_Buildwide, &itself->value, itself->scale,
                        TimeBaseObj_New, itself->base);
        else
                return  Py_BuildValue("O&lO", PyMac_Buildwide, &itself->value, itself->scale,
                        Py_None);
}

static int
QtTimeRecord_Convert(PyObject *v, TimeRecord *p_itself)
{
        PyObject *base = NULL;
        if( !PyArg_ParseTuple(v, "O&l|O", PyMac_Getwide, &p_itself->value, &p_itself->scale,
                        &base) )
                return 0;
        if ( base == NULL || base == Py_None )
                p_itself->base = NULL;
        else
                if ( !TimeBaseObj_Convert(base, &p_itself->base) )
                        return 0;
        return 1;
}

static int
QtMusicMIDIPacket_Convert(PyObject *v, MusicMIDIPacket *p_itself)
{
        int dummy;

        if( !PyArg_ParseTuple(v, "hls#", &p_itself->length, &p_itself->reserved, p_itself->data, dummy) )
                return 0;
        return 1;
}




static PyObject *Qt_Error;

/* -------------------- Object type IdleManager --------------------- */

PyTypeObject IdleManager_Type;

#define IdleManagerObj_Check(x) ((x)->ob_type == &IdleManager_Type || PyObject_TypeCheck((x), &IdleManager_Type))

typedef struct IdleManagerObject {
	PyObject_HEAD
	IdleManager ob_itself;
} IdleManagerObject;

PyObject *IdleManagerObj_New(IdleManager itself)
{
	IdleManagerObject *it;
	if (itself == NULL) {
	                                PyErr_SetString(Qt_Error,"Cannot create IdleManager from NULL pointer");
	                                return NULL;
	                        }
	it = PyObject_NEW(IdleManagerObject, &IdleManager_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}

int IdleManagerObj_Convert(PyObject *v, IdleManager *p_itself)
{
	if (v == Py_None)
	{
		*p_itself = NULL;
		return 1;
	}
	if (!IdleManagerObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "IdleManager required");
		return 0;
	}
	*p_itself = ((IdleManagerObject *)v)->ob_itself;
	return 1;
}

static void IdleManagerObj_dealloc(IdleManagerObject *self)
{
	/* Cleanup of self->ob_itself goes here */
	self->ob_type->tp_free((PyObject *)self);
}

static PyMethodDef IdleManagerObj_methods[] = {
	{NULL, NULL, 0}
};

#define IdleManagerObj_getsetlist NULL


#define IdleManagerObj_compare NULL

#define IdleManagerObj_repr NULL

#define IdleManagerObj_hash NULL
#define IdleManagerObj_tp_init 0

#define IdleManagerObj_tp_alloc PyType_GenericAlloc

static PyObject *IdleManagerObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	IdleManager itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, IdleManagerObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((IdleManagerObject *)_self)->ob_itself = itself;
	return _self;
}

#define IdleManagerObj_tp_free PyObject_Del


PyTypeObject IdleManager_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.IdleManager", /*tp_name*/
	sizeof(IdleManagerObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) IdleManagerObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) IdleManagerObj_compare, /*tp_compare*/
	(reprfunc) IdleManagerObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) IdleManagerObj_hash, /*tp_hash*/
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
	IdleManagerObj_methods, /* tp_methods */
	0, /*tp_members*/
	IdleManagerObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	IdleManagerObj_tp_init, /* tp_init */
	IdleManagerObj_tp_alloc, /* tp_alloc */
	IdleManagerObj_tp_new, /* tp_new */
	IdleManagerObj_tp_free, /* tp_free */
};

/* ------------------ End object type IdleManager ------------------- */


/* ------------------ Object type MovieController ------------------- */

PyTypeObject MovieController_Type;

#define MovieCtlObj_Check(x) ((x)->ob_type == &MovieController_Type || PyObject_TypeCheck((x), &MovieController_Type))

typedef struct MovieControllerObject {
	PyObject_HEAD
	MovieController ob_itself;
} MovieControllerObject;

PyObject *MovieCtlObj_New(MovieController itself)
{
	MovieControllerObject *it;
	if (itself == NULL) {
	                                PyErr_SetString(Qt_Error,"Cannot create MovieController from NULL pointer");
	                                return NULL;
	                        }
	it = PyObject_NEW(MovieControllerObject, &MovieController_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}

int MovieCtlObj_Convert(PyObject *v, MovieController *p_itself)
{
	if (v == Py_None)
	{
		*p_itself = NULL;
		return 1;
	}
	if (!MovieCtlObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "MovieController required");
		return 0;
	}
	*p_itself = ((MovieControllerObject *)v)->ob_itself;
	return 1;
}

static void MovieCtlObj_dealloc(MovieControllerObject *self)
{
	if (self->ob_itself) DisposeMovieController(self->ob_itself);
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *MovieCtlObj_MCSetMovie(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Movie theMovie;
	WindowPtr movieWindow;
	Point where;
#ifndef MCSetMovie
	PyMac_PRECHECK(MCSetMovie);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      MovieObj_Convert, &theMovie,
	                      WinObj_Convert, &movieWindow,
	                      PyMac_GetPoint, &where))
		return NULL;
	_rv = MCSetMovie(_self->ob_itself,
	                 theMovie,
	                 movieWindow,
	                 where);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCGetIndMovie(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie _rv;
	short index;
#ifndef MCGetIndMovie
	PyMac_PRECHECK(MCGetIndMovie);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &index))
		return NULL;
	_rv = MCGetIndMovie(_self->ob_itself,
	                    index);
	_res = Py_BuildValue("O&",
	                     MovieObj_New, _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCRemoveAllMovies(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
#ifndef MCRemoveAllMovies
	PyMac_PRECHECK(MCRemoveAllMovies);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCRemoveAllMovies(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCRemoveAMovie(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Movie m;
#ifndef MCRemoveAMovie
	PyMac_PRECHECK(MCRemoveAMovie);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      MovieObj_Convert, &m))
		return NULL;
	_rv = MCRemoveAMovie(_self->ob_itself,
	                     m);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCRemoveMovie(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
#ifndef MCRemoveMovie
	PyMac_PRECHECK(MCRemoveMovie);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCRemoveMovie(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCIsPlayerEvent(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	EventRecord e;
#ifndef MCIsPlayerEvent
	PyMac_PRECHECK(MCIsPlayerEvent);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &e))
		return NULL;
	_rv = MCIsPlayerEvent(_self->ob_itself,
	                      &e);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCDoAction(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	short action;
	void * params;
#ifndef MCDoAction
	PyMac_PRECHECK(MCDoAction);
#endif
	if (!PyArg_ParseTuple(_args, "hs",
	                      &action,
	                      &params))
		return NULL;
	_rv = MCDoAction(_self->ob_itself,
	                 action,
	                 params);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCSetControllerAttached(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Boolean attach;
#ifndef MCSetControllerAttached
	PyMac_PRECHECK(MCSetControllerAttached);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &attach))
		return NULL;
	_rv = MCSetControllerAttached(_self->ob_itself,
	                              attach);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCIsControllerAttached(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
#ifndef MCIsControllerAttached
	PyMac_PRECHECK(MCIsControllerAttached);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCIsControllerAttached(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCSetControllerPort(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	CGrafPtr gp;
#ifndef MCSetControllerPort
	PyMac_PRECHECK(MCSetControllerPort);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      GrafObj_Convert, &gp))
		return NULL;
	_rv = MCSetControllerPort(_self->ob_itself,
	                          gp);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCGetControllerPort(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGrafPtr _rv;
#ifndef MCGetControllerPort
	PyMac_PRECHECK(MCGetControllerPort);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCGetControllerPort(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     GrafObj_New, _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCSetVisible(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Boolean visible;
#ifndef MCSetVisible
	PyMac_PRECHECK(MCSetVisible);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &visible))
		return NULL;
	_rv = MCSetVisible(_self->ob_itself,
	                   visible);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCGetVisible(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
#ifndef MCGetVisible
	PyMac_PRECHECK(MCGetVisible);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCGetVisible(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCGetControllerBoundsRect(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Rect bounds;
#ifndef MCGetControllerBoundsRect
	PyMac_PRECHECK(MCGetControllerBoundsRect);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCGetControllerBoundsRect(_self->ob_itself,
	                                &bounds);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &bounds);
	return _res;
}

static PyObject *MovieCtlObj_MCSetControllerBoundsRect(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Rect bounds;
#ifndef MCSetControllerBoundsRect
	PyMac_PRECHECK(MCSetControllerBoundsRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &bounds))
		return NULL;
	_rv = MCSetControllerBoundsRect(_self->ob_itself,
	                                &bounds);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCGetControllerBoundsRgn(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
#ifndef MCGetControllerBoundsRgn
	PyMac_PRECHECK(MCGetControllerBoundsRgn);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCGetControllerBoundsRgn(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCGetWindowRgn(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
	WindowPtr w;
#ifndef MCGetWindowRgn
	PyMac_PRECHECK(MCGetWindowRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &w))
		return NULL;
	_rv = MCGetWindowRgn(_self->ob_itself,
	                     w);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCMovieChanged(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Movie m;
#ifndef MCMovieChanged
	PyMac_PRECHECK(MCMovieChanged);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      MovieObj_Convert, &m))
		return NULL;
	_rv = MCMovieChanged(_self->ob_itself,
	                     m);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCSetDuration(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TimeValue duration;
#ifndef MCSetDuration
	PyMac_PRECHECK(MCSetDuration);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &duration))
		return NULL;
	_rv = MCSetDuration(_self->ob_itself,
	                    duration);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCGetCurrentTime(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue _rv;
	TimeScale scale;
#ifndef MCGetCurrentTime
	PyMac_PRECHECK(MCGetCurrentTime);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCGetCurrentTime(_self->ob_itself,
	                       &scale);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     scale);
	return _res;
}

static PyObject *MovieCtlObj_MCNewAttachedController(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Movie theMovie;
	WindowPtr w;
	Point where;
#ifndef MCNewAttachedController
	PyMac_PRECHECK(MCNewAttachedController);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      MovieObj_Convert, &theMovie,
	                      WinObj_Convert, &w,
	                      PyMac_GetPoint, &where))
		return NULL;
	_rv = MCNewAttachedController(_self->ob_itself,
	                              theMovie,
	                              w,
	                              where);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCDraw(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	WindowPtr w;
#ifndef MCDraw
	PyMac_PRECHECK(MCDraw);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &w))
		return NULL;
	_rv = MCDraw(_self->ob_itself,
	             w);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCActivate(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	WindowPtr w;
	Boolean activate;
#ifndef MCActivate
	PyMac_PRECHECK(MCActivate);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      WinObj_Convert, &w,
	                      &activate))
		return NULL;
	_rv = MCActivate(_self->ob_itself,
	                 w,
	                 activate);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCIdle(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
#ifndef MCIdle
	PyMac_PRECHECK(MCIdle);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCIdle(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCKey(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SInt8 key;
	long modifiers;
#ifndef MCKey
	PyMac_PRECHECK(MCKey);
#endif
	if (!PyArg_ParseTuple(_args, "bl",
	                      &key,
	                      &modifiers))
		return NULL;
	_rv = MCKey(_self->ob_itself,
	            key,
	            modifiers);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCClick(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	WindowPtr w;
	Point where;
	long when;
	long modifiers;
#ifndef MCClick
	PyMac_PRECHECK(MCClick);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&ll",
	                      WinObj_Convert, &w,
	                      PyMac_GetPoint, &where,
	                      &when,
	                      &modifiers))
		return NULL;
	_rv = MCClick(_self->ob_itself,
	              w,
	              where,
	              when,
	              modifiers);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCEnableEditing(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Boolean enabled;
#ifndef MCEnableEditing
	PyMac_PRECHECK(MCEnableEditing);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &enabled))
		return NULL;
	_rv = MCEnableEditing(_self->ob_itself,
	                      enabled);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCIsEditingEnabled(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef MCIsEditingEnabled
	PyMac_PRECHECK(MCIsEditingEnabled);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCIsEditingEnabled(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCCopy(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie _rv;
#ifndef MCCopy
	PyMac_PRECHECK(MCCopy);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCCopy(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     MovieObj_New, _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCCut(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie _rv;
#ifndef MCCut
	PyMac_PRECHECK(MCCut);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCCut(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     MovieObj_New, _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCPaste(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Movie srcMovie;
#ifndef MCPaste
	PyMac_PRECHECK(MCPaste);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      MovieObj_Convert, &srcMovie))
		return NULL;
	_rv = MCPaste(_self->ob_itself,
	              srcMovie);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCClear(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
#ifndef MCClear
	PyMac_PRECHECK(MCClear);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCClear(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCUndo(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
#ifndef MCUndo
	PyMac_PRECHECK(MCUndo);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCUndo(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCPositionController(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Rect movieRect;
	Rect controllerRect;
	long someFlags;
#ifndef MCPositionController
	PyMac_PRECHECK(MCPositionController);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      PyMac_GetRect, &movieRect,
	                      PyMac_GetRect, &controllerRect,
	                      &someFlags))
		return NULL;
	_rv = MCPositionController(_self->ob_itself,
	                           &movieRect,
	                           &controllerRect,
	                           someFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCGetControllerInfo(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	long someFlags;
#ifndef MCGetControllerInfo
	PyMac_PRECHECK(MCGetControllerInfo);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCGetControllerInfo(_self->ob_itself,
	                          &someFlags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     someFlags);
	return _res;
}

static PyObject *MovieCtlObj_MCSetClip(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	RgnHandle theClip;
	RgnHandle movieClip;
#ifndef MCSetClip
	PyMac_PRECHECK(MCSetClip);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &theClip,
	                      ResObj_Convert, &movieClip))
		return NULL;
	_rv = MCSetClip(_self->ob_itself,
	                theClip,
	                movieClip);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCGetClip(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	RgnHandle theClip;
	RgnHandle movieClip;
#ifndef MCGetClip
	PyMac_PRECHECK(MCGetClip);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCGetClip(_self->ob_itself,
	                &theClip,
	                &movieClip);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, theClip,
	                     ResObj_New, movieClip);
	return _res;
}

static PyObject *MovieCtlObj_MCDrawBadge(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	RgnHandle movieRgn;
	RgnHandle badgeRgn;
#ifndef MCDrawBadge
	PyMac_PRECHECK(MCDrawBadge);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &movieRgn))
		return NULL;
	_rv = MCDrawBadge(_self->ob_itself,
	                  movieRgn,
	                  &badgeRgn);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, badgeRgn);
	return _res;
}

static PyObject *MovieCtlObj_MCSetUpEditMenu(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	long modifiers;
	MenuHandle mh;
#ifndef MCSetUpEditMenu
	PyMac_PRECHECK(MCSetUpEditMenu);
#endif
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &modifiers,
	                      MenuObj_Convert, &mh))
		return NULL;
	_rv = MCSetUpEditMenu(_self->ob_itself,
	                      modifiers,
	                      mh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCGetMenuString(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	long modifiers;
	short item;
	Str255 aString;
#ifndef MCGetMenuString
	PyMac_PRECHECK(MCGetMenuString);
#endif
	if (!PyArg_ParseTuple(_args, "lhO&",
	                      &modifiers,
	                      &item,
	                      PyMac_GetStr255, aString))
		return NULL;
	_rv = MCGetMenuString(_self->ob_itself,
	                      modifiers,
	                      item,
	                      aString);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCPtInController(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Point thePt;
	Boolean inController;
#ifndef MCPtInController
	PyMac_PRECHECK(MCPtInController);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &thePt))
		return NULL;
	_rv = MCPtInController(_self->ob_itself,
	                       thePt,
	                       &inController);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     inController);
	return _res;
}

static PyObject *MovieCtlObj_MCInvalidate(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	WindowPtr w;
	RgnHandle invalidRgn;
#ifndef MCInvalidate
	PyMac_PRECHECK(MCInvalidate);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      WinObj_Convert, &w,
	                      ResObj_Convert, &invalidRgn))
		return NULL;
	_rv = MCInvalidate(_self->ob_itself,
	                   w,
	                   invalidRgn);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCAdjustCursor(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	WindowPtr w;
	Point where;
	long modifiers;
#ifndef MCAdjustCursor
	PyMac_PRECHECK(MCAdjustCursor);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      WinObj_Convert, &w,
	                      PyMac_GetPoint, &where,
	                      &modifiers))
		return NULL;
	_rv = MCAdjustCursor(_self->ob_itself,
	                     w,
	                     where,
	                     modifiers);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCGetInterfaceElement(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MCInterfaceElement whichElement;
	void * element;
#ifndef MCGetInterfaceElement
	PyMac_PRECHECK(MCGetInterfaceElement);
#endif
	if (!PyArg_ParseTuple(_args, "ls",
	                      &whichElement,
	                      &element))
		return NULL;
	_rv = MCGetInterfaceElement(_self->ob_itself,
	                            whichElement,
	                            element);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCAddMovieSegment(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Movie srcMovie;
	Boolean scaled;
#ifndef MCAddMovieSegment
	PyMac_PRECHECK(MCAddMovieSegment);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      MovieObj_Convert, &srcMovie,
	                      &scaled))
		return NULL;
	_rv = MCAddMovieSegment(_self->ob_itself,
	                        srcMovie,
	                        scaled);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCTrimMovieSegment(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
#ifndef MCTrimMovieSegment
	PyMac_PRECHECK(MCTrimMovieSegment);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MCTrimMovieSegment(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCSetIdleManager(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	IdleManager im;
#ifndef MCSetIdleManager
	PyMac_PRECHECK(MCSetIdleManager);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      IdleManagerObj_Convert, &im))
		return NULL;
	_rv = MCSetIdleManager(_self->ob_itself,
	                       im);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieCtlObj_MCSetControllerCapabilities(MovieControllerObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	long flags;
	long flagsMask;
#ifndef MCSetControllerCapabilities
	PyMac_PRECHECK(MCSetControllerCapabilities);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &flags,
	                      &flagsMask))
		return NULL;
	_rv = MCSetControllerCapabilities(_self->ob_itself,
	                                  flags,
	                                  flagsMask);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyMethodDef MovieCtlObj_methods[] = {
	{"MCSetMovie", (PyCFunction)MovieCtlObj_MCSetMovie, 1,
	 PyDoc_STR("(Movie theMovie, WindowPtr movieWindow, Point where) -> (ComponentResult _rv)")},
	{"MCGetIndMovie", (PyCFunction)MovieCtlObj_MCGetIndMovie, 1,
	 PyDoc_STR("(short index) -> (Movie _rv)")},
	{"MCRemoveAllMovies", (PyCFunction)MovieCtlObj_MCRemoveAllMovies, 1,
	 PyDoc_STR("() -> (ComponentResult _rv)")},
	{"MCRemoveAMovie", (PyCFunction)MovieCtlObj_MCRemoveAMovie, 1,
	 PyDoc_STR("(Movie m) -> (ComponentResult _rv)")},
	{"MCRemoveMovie", (PyCFunction)MovieCtlObj_MCRemoveMovie, 1,
	 PyDoc_STR("() -> (ComponentResult _rv)")},
	{"MCIsPlayerEvent", (PyCFunction)MovieCtlObj_MCIsPlayerEvent, 1,
	 PyDoc_STR("(EventRecord e) -> (ComponentResult _rv)")},
	{"MCDoAction", (PyCFunction)MovieCtlObj_MCDoAction, 1,
	 PyDoc_STR("(short action, void * params) -> (ComponentResult _rv)")},
	{"MCSetControllerAttached", (PyCFunction)MovieCtlObj_MCSetControllerAttached, 1,
	 PyDoc_STR("(Boolean attach) -> (ComponentResult _rv)")},
	{"MCIsControllerAttached", (PyCFunction)MovieCtlObj_MCIsControllerAttached, 1,
	 PyDoc_STR("() -> (ComponentResult _rv)")},
	{"MCSetControllerPort", (PyCFunction)MovieCtlObj_MCSetControllerPort, 1,
	 PyDoc_STR("(CGrafPtr gp) -> (ComponentResult _rv)")},
	{"MCGetControllerPort", (PyCFunction)MovieCtlObj_MCGetControllerPort, 1,
	 PyDoc_STR("() -> (CGrafPtr _rv)")},
	{"MCSetVisible", (PyCFunction)MovieCtlObj_MCSetVisible, 1,
	 PyDoc_STR("(Boolean visible) -> (ComponentResult _rv)")},
	{"MCGetVisible", (PyCFunction)MovieCtlObj_MCGetVisible, 1,
	 PyDoc_STR("() -> (ComponentResult _rv)")},
	{"MCGetControllerBoundsRect", (PyCFunction)MovieCtlObj_MCGetControllerBoundsRect, 1,
	 PyDoc_STR("() -> (ComponentResult _rv, Rect bounds)")},
	{"MCSetControllerBoundsRect", (PyCFunction)MovieCtlObj_MCSetControllerBoundsRect, 1,
	 PyDoc_STR("(Rect bounds) -> (ComponentResult _rv)")},
	{"MCGetControllerBoundsRgn", (PyCFunction)MovieCtlObj_MCGetControllerBoundsRgn, 1,
	 PyDoc_STR("() -> (RgnHandle _rv)")},
	{"MCGetWindowRgn", (PyCFunction)MovieCtlObj_MCGetWindowRgn, 1,
	 PyDoc_STR("(WindowPtr w) -> (RgnHandle _rv)")},
	{"MCMovieChanged", (PyCFunction)MovieCtlObj_MCMovieChanged, 1,
	 PyDoc_STR("(Movie m) -> (ComponentResult _rv)")},
	{"MCSetDuration", (PyCFunction)MovieCtlObj_MCSetDuration, 1,
	 PyDoc_STR("(TimeValue duration) -> (ComponentResult _rv)")},
	{"MCGetCurrentTime", (PyCFunction)MovieCtlObj_MCGetCurrentTime, 1,
	 PyDoc_STR("() -> (TimeValue _rv, TimeScale scale)")},
	{"MCNewAttachedController", (PyCFunction)MovieCtlObj_MCNewAttachedController, 1,
	 PyDoc_STR("(Movie theMovie, WindowPtr w, Point where) -> (ComponentResult _rv)")},
	{"MCDraw", (PyCFunction)MovieCtlObj_MCDraw, 1,
	 PyDoc_STR("(WindowPtr w) -> (ComponentResult _rv)")},
	{"MCActivate", (PyCFunction)MovieCtlObj_MCActivate, 1,
	 PyDoc_STR("(WindowPtr w, Boolean activate) -> (ComponentResult _rv)")},
	{"MCIdle", (PyCFunction)MovieCtlObj_MCIdle, 1,
	 PyDoc_STR("() -> (ComponentResult _rv)")},
	{"MCKey", (PyCFunction)MovieCtlObj_MCKey, 1,
	 PyDoc_STR("(SInt8 key, long modifiers) -> (ComponentResult _rv)")},
	{"MCClick", (PyCFunction)MovieCtlObj_MCClick, 1,
	 PyDoc_STR("(WindowPtr w, Point where, long when, long modifiers) -> (ComponentResult _rv)")},
	{"MCEnableEditing", (PyCFunction)MovieCtlObj_MCEnableEditing, 1,
	 PyDoc_STR("(Boolean enabled) -> (ComponentResult _rv)")},
	{"MCIsEditingEnabled", (PyCFunction)MovieCtlObj_MCIsEditingEnabled, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"MCCopy", (PyCFunction)MovieCtlObj_MCCopy, 1,
	 PyDoc_STR("() -> (Movie _rv)")},
	{"MCCut", (PyCFunction)MovieCtlObj_MCCut, 1,
	 PyDoc_STR("() -> (Movie _rv)")},
	{"MCPaste", (PyCFunction)MovieCtlObj_MCPaste, 1,
	 PyDoc_STR("(Movie srcMovie) -> (ComponentResult _rv)")},
	{"MCClear", (PyCFunction)MovieCtlObj_MCClear, 1,
	 PyDoc_STR("() -> (ComponentResult _rv)")},
	{"MCUndo", (PyCFunction)MovieCtlObj_MCUndo, 1,
	 PyDoc_STR("() -> (ComponentResult _rv)")},
	{"MCPositionController", (PyCFunction)MovieCtlObj_MCPositionController, 1,
	 PyDoc_STR("(Rect movieRect, Rect controllerRect, long someFlags) -> (ComponentResult _rv)")},
	{"MCGetControllerInfo", (PyCFunction)MovieCtlObj_MCGetControllerInfo, 1,
	 PyDoc_STR("() -> (ComponentResult _rv, long someFlags)")},
	{"MCSetClip", (PyCFunction)MovieCtlObj_MCSetClip, 1,
	 PyDoc_STR("(RgnHandle theClip, RgnHandle movieClip) -> (ComponentResult _rv)")},
	{"MCGetClip", (PyCFunction)MovieCtlObj_MCGetClip, 1,
	 PyDoc_STR("() -> (ComponentResult _rv, RgnHandle theClip, RgnHandle movieClip)")},
	{"MCDrawBadge", (PyCFunction)MovieCtlObj_MCDrawBadge, 1,
	 PyDoc_STR("(RgnHandle movieRgn) -> (ComponentResult _rv, RgnHandle badgeRgn)")},
	{"MCSetUpEditMenu", (PyCFunction)MovieCtlObj_MCSetUpEditMenu, 1,
	 PyDoc_STR("(long modifiers, MenuHandle mh) -> (ComponentResult _rv)")},
	{"MCGetMenuString", (PyCFunction)MovieCtlObj_MCGetMenuString, 1,
	 PyDoc_STR("(long modifiers, short item, Str255 aString) -> (ComponentResult _rv)")},
	{"MCPtInController", (PyCFunction)MovieCtlObj_MCPtInController, 1,
	 PyDoc_STR("(Point thePt) -> (ComponentResult _rv, Boolean inController)")},
	{"MCInvalidate", (PyCFunction)MovieCtlObj_MCInvalidate, 1,
	 PyDoc_STR("(WindowPtr w, RgnHandle invalidRgn) -> (ComponentResult _rv)")},
	{"MCAdjustCursor", (PyCFunction)MovieCtlObj_MCAdjustCursor, 1,
	 PyDoc_STR("(WindowPtr w, Point where, long modifiers) -> (ComponentResult _rv)")},
	{"MCGetInterfaceElement", (PyCFunction)MovieCtlObj_MCGetInterfaceElement, 1,
	 PyDoc_STR("(MCInterfaceElement whichElement, void * element) -> (ComponentResult _rv)")},
	{"MCAddMovieSegment", (PyCFunction)MovieCtlObj_MCAddMovieSegment, 1,
	 PyDoc_STR("(Movie srcMovie, Boolean scaled) -> (ComponentResult _rv)")},
	{"MCTrimMovieSegment", (PyCFunction)MovieCtlObj_MCTrimMovieSegment, 1,
	 PyDoc_STR("() -> (ComponentResult _rv)")},
	{"MCSetIdleManager", (PyCFunction)MovieCtlObj_MCSetIdleManager, 1,
	 PyDoc_STR("(IdleManager im) -> (ComponentResult _rv)")},
	{"MCSetControllerCapabilities", (PyCFunction)MovieCtlObj_MCSetControllerCapabilities, 1,
	 PyDoc_STR("(long flags, long flagsMask) -> (ComponentResult _rv)")},
	{NULL, NULL, 0}
};

#define MovieCtlObj_getsetlist NULL


#define MovieCtlObj_compare NULL

#define MovieCtlObj_repr NULL

#define MovieCtlObj_hash NULL
#define MovieCtlObj_tp_init 0

#define MovieCtlObj_tp_alloc PyType_GenericAlloc

static PyObject *MovieCtlObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	MovieController itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, MovieCtlObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((MovieControllerObject *)_self)->ob_itself = itself;
	return _self;
}

#define MovieCtlObj_tp_free PyObject_Del


PyTypeObject MovieController_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.MovieController", /*tp_name*/
	sizeof(MovieControllerObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) MovieCtlObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) MovieCtlObj_compare, /*tp_compare*/
	(reprfunc) MovieCtlObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) MovieCtlObj_hash, /*tp_hash*/
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
	MovieCtlObj_methods, /* tp_methods */
	0, /*tp_members*/
	MovieCtlObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	MovieCtlObj_tp_init, /* tp_init */
	MovieCtlObj_tp_alloc, /* tp_alloc */
	MovieCtlObj_tp_new, /* tp_new */
	MovieCtlObj_tp_free, /* tp_free */
};

/* ---------------- End object type MovieController ----------------- */


/* ---------------------- Object type TimeBase ---------------------- */

PyTypeObject TimeBase_Type;

#define TimeBaseObj_Check(x) ((x)->ob_type == &TimeBase_Type || PyObject_TypeCheck((x), &TimeBase_Type))

typedef struct TimeBaseObject {
	PyObject_HEAD
	TimeBase ob_itself;
} TimeBaseObject;

PyObject *TimeBaseObj_New(TimeBase itself)
{
	TimeBaseObject *it;
	if (itself == NULL) {
	                                PyErr_SetString(Qt_Error,"Cannot create TimeBase from NULL pointer");
	                                return NULL;
	                        }
	it = PyObject_NEW(TimeBaseObject, &TimeBase_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}

int TimeBaseObj_Convert(PyObject *v, TimeBase *p_itself)
{
	if (v == Py_None)
	{
		*p_itself = NULL;
		return 1;
	}
	if (!TimeBaseObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "TimeBase required");
		return 0;
	}
	*p_itself = ((TimeBaseObject *)v)->ob_itself;
	return 1;
}

static void TimeBaseObj_dealloc(TimeBaseObject *self)
{
	/* Cleanup of self->ob_itself goes here */
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *TimeBaseObj_DisposeTimeBase(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef DisposeTimeBase
	PyMac_PRECHECK(DisposeTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DisposeTimeBase(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TimeBaseObj_GetTimeBaseTime(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue _rv;
	TimeScale s;
	TimeRecord tr;
#ifndef GetTimeBaseTime
	PyMac_PRECHECK(GetTimeBaseTime);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &s))
		return NULL;
	_rv = GetTimeBaseTime(_self->ob_itself,
	                      s,
	                      &tr);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     QtTimeRecord_New, &tr);
	return _res;
}

static PyObject *TimeBaseObj_SetTimeBaseTime(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeRecord tr;
#ifndef SetTimeBaseTime
	PyMac_PRECHECK(SetTimeBaseTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      QtTimeRecord_Convert, &tr))
		return NULL;
	SetTimeBaseTime(_self->ob_itself,
	                &tr);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TimeBaseObj_SetTimeBaseValue(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue t;
	TimeScale s;
#ifndef SetTimeBaseValue
	PyMac_PRECHECK(SetTimeBaseValue);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &t,
	                      &s))
		return NULL;
	SetTimeBaseValue(_self->ob_itself,
	                 t,
	                 s);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TimeBaseObj_GetTimeBaseRate(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
#ifndef GetTimeBaseRate
	PyMac_PRECHECK(GetTimeBaseRate);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTimeBaseRate(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyObject *TimeBaseObj_SetTimeBaseRate(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed r;
#ifndef SetTimeBaseRate
	PyMac_PRECHECK(SetTimeBaseRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFixed, &r))
		return NULL;
	SetTimeBaseRate(_self->ob_itself,
	                r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TimeBaseObj_GetTimeBaseStartTime(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue _rv;
	TimeScale s;
	TimeRecord tr;
#ifndef GetTimeBaseStartTime
	PyMac_PRECHECK(GetTimeBaseStartTime);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &s))
		return NULL;
	_rv = GetTimeBaseStartTime(_self->ob_itself,
	                           s,
	                           &tr);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     QtTimeRecord_New, &tr);
	return _res;
}

static PyObject *TimeBaseObj_SetTimeBaseStartTime(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeRecord tr;
#ifndef SetTimeBaseStartTime
	PyMac_PRECHECK(SetTimeBaseStartTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      QtTimeRecord_Convert, &tr))
		return NULL;
	SetTimeBaseStartTime(_self->ob_itself,
	                     &tr);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TimeBaseObj_GetTimeBaseStopTime(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue _rv;
	TimeScale s;
	TimeRecord tr;
#ifndef GetTimeBaseStopTime
	PyMac_PRECHECK(GetTimeBaseStopTime);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &s))
		return NULL;
	_rv = GetTimeBaseStopTime(_self->ob_itself,
	                          s,
	                          &tr);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     QtTimeRecord_New, &tr);
	return _res;
}

static PyObject *TimeBaseObj_SetTimeBaseStopTime(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeRecord tr;
#ifndef SetTimeBaseStopTime
	PyMac_PRECHECK(SetTimeBaseStopTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      QtTimeRecord_Convert, &tr))
		return NULL;
	SetTimeBaseStopTime(_self->ob_itself,
	                    &tr);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TimeBaseObj_GetTimeBaseFlags(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef GetTimeBaseFlags
	PyMac_PRECHECK(GetTimeBaseFlags);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTimeBaseFlags(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TimeBaseObj_SetTimeBaseFlags(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long timeBaseFlags;
#ifndef SetTimeBaseFlags
	PyMac_PRECHECK(SetTimeBaseFlags);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &timeBaseFlags))
		return NULL;
	SetTimeBaseFlags(_self->ob_itself,
	                 timeBaseFlags);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TimeBaseObj_SetTimeBaseMasterTimeBase(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeBase master;
	TimeRecord slaveZero;
#ifndef SetTimeBaseMasterTimeBase
	PyMac_PRECHECK(SetTimeBaseMasterTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      TimeBaseObj_Convert, &master,
	                      QtTimeRecord_Convert, &slaveZero))
		return NULL;
	SetTimeBaseMasterTimeBase(_self->ob_itself,
	                          master,
	                          &slaveZero);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TimeBaseObj_GetTimeBaseMasterTimeBase(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeBase _rv;
#ifndef GetTimeBaseMasterTimeBase
	PyMac_PRECHECK(GetTimeBaseMasterTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTimeBaseMasterTimeBase(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     TimeBaseObj_New, _rv);
	return _res;
}

static PyObject *TimeBaseObj_SetTimeBaseMasterClock(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Component clockMeister;
	TimeRecord slaveZero;
#ifndef SetTimeBaseMasterClock
	PyMac_PRECHECK(SetTimeBaseMasterClock);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpObj_Convert, &clockMeister,
	                      QtTimeRecord_Convert, &slaveZero))
		return NULL;
	SetTimeBaseMasterClock(_self->ob_itself,
	                       clockMeister,
	                       &slaveZero);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TimeBaseObj_GetTimeBaseMasterClock(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentInstance _rv;
#ifndef GetTimeBaseMasterClock
	PyMac_PRECHECK(GetTimeBaseMasterClock);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTimeBaseMasterClock(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, _rv);
	return _res;
}

static PyObject *TimeBaseObj_GetTimeBaseStatus(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	TimeRecord unpinnedTime;
#ifndef GetTimeBaseStatus
	PyMac_PRECHECK(GetTimeBaseStatus);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTimeBaseStatus(_self->ob_itself,
	                        &unpinnedTime);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     QtTimeRecord_New, &unpinnedTime);
	return _res;
}

static PyObject *TimeBaseObj_SetTimeBaseZero(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeRecord zero;
#ifndef SetTimeBaseZero
	PyMac_PRECHECK(SetTimeBaseZero);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      QtTimeRecord_Convert, &zero))
		return NULL;
	SetTimeBaseZero(_self->ob_itself,
	                &zero);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TimeBaseObj_GetTimeBaseEffectiveRate(TimeBaseObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
#ifndef GetTimeBaseEffectiveRate
	PyMac_PRECHECK(GetTimeBaseEffectiveRate);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTimeBaseEffectiveRate(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyMethodDef TimeBaseObj_methods[] = {
	{"DisposeTimeBase", (PyCFunction)TimeBaseObj_DisposeTimeBase, 1,
	 PyDoc_STR("() -> None")},
	{"GetTimeBaseTime", (PyCFunction)TimeBaseObj_GetTimeBaseTime, 1,
	 PyDoc_STR("(TimeScale s) -> (TimeValue _rv, TimeRecord tr)")},
	{"SetTimeBaseTime", (PyCFunction)TimeBaseObj_SetTimeBaseTime, 1,
	 PyDoc_STR("(TimeRecord tr) -> None")},
	{"SetTimeBaseValue", (PyCFunction)TimeBaseObj_SetTimeBaseValue, 1,
	 PyDoc_STR("(TimeValue t, TimeScale s) -> None")},
	{"GetTimeBaseRate", (PyCFunction)TimeBaseObj_GetTimeBaseRate, 1,
	 PyDoc_STR("() -> (Fixed _rv)")},
	{"SetTimeBaseRate", (PyCFunction)TimeBaseObj_SetTimeBaseRate, 1,
	 PyDoc_STR("(Fixed r) -> None")},
	{"GetTimeBaseStartTime", (PyCFunction)TimeBaseObj_GetTimeBaseStartTime, 1,
	 PyDoc_STR("(TimeScale s) -> (TimeValue _rv, TimeRecord tr)")},
	{"SetTimeBaseStartTime", (PyCFunction)TimeBaseObj_SetTimeBaseStartTime, 1,
	 PyDoc_STR("(TimeRecord tr) -> None")},
	{"GetTimeBaseStopTime", (PyCFunction)TimeBaseObj_GetTimeBaseStopTime, 1,
	 PyDoc_STR("(TimeScale s) -> (TimeValue _rv, TimeRecord tr)")},
	{"SetTimeBaseStopTime", (PyCFunction)TimeBaseObj_SetTimeBaseStopTime, 1,
	 PyDoc_STR("(TimeRecord tr) -> None")},
	{"GetTimeBaseFlags", (PyCFunction)TimeBaseObj_GetTimeBaseFlags, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"SetTimeBaseFlags", (PyCFunction)TimeBaseObj_SetTimeBaseFlags, 1,
	 PyDoc_STR("(long timeBaseFlags) -> None")},
	{"SetTimeBaseMasterTimeBase", (PyCFunction)TimeBaseObj_SetTimeBaseMasterTimeBase, 1,
	 PyDoc_STR("(TimeBase master, TimeRecord slaveZero) -> None")},
	{"GetTimeBaseMasterTimeBase", (PyCFunction)TimeBaseObj_GetTimeBaseMasterTimeBase, 1,
	 PyDoc_STR("() -> (TimeBase _rv)")},
	{"SetTimeBaseMasterClock", (PyCFunction)TimeBaseObj_SetTimeBaseMasterClock, 1,
	 PyDoc_STR("(Component clockMeister, TimeRecord slaveZero) -> None")},
	{"GetTimeBaseMasterClock", (PyCFunction)TimeBaseObj_GetTimeBaseMasterClock, 1,
	 PyDoc_STR("() -> (ComponentInstance _rv)")},
	{"GetTimeBaseStatus", (PyCFunction)TimeBaseObj_GetTimeBaseStatus, 1,
	 PyDoc_STR("() -> (long _rv, TimeRecord unpinnedTime)")},
	{"SetTimeBaseZero", (PyCFunction)TimeBaseObj_SetTimeBaseZero, 1,
	 PyDoc_STR("(TimeRecord zero) -> None")},
	{"GetTimeBaseEffectiveRate", (PyCFunction)TimeBaseObj_GetTimeBaseEffectiveRate, 1,
	 PyDoc_STR("() -> (Fixed _rv)")},
	{NULL, NULL, 0}
};

#define TimeBaseObj_getsetlist NULL


#define TimeBaseObj_compare NULL

#define TimeBaseObj_repr NULL

#define TimeBaseObj_hash NULL
#define TimeBaseObj_tp_init 0

#define TimeBaseObj_tp_alloc PyType_GenericAlloc

static PyObject *TimeBaseObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	TimeBase itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, TimeBaseObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((TimeBaseObject *)_self)->ob_itself = itself;
	return _self;
}

#define TimeBaseObj_tp_free PyObject_Del


PyTypeObject TimeBase_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.TimeBase", /*tp_name*/
	sizeof(TimeBaseObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) TimeBaseObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) TimeBaseObj_compare, /*tp_compare*/
	(reprfunc) TimeBaseObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) TimeBaseObj_hash, /*tp_hash*/
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
	TimeBaseObj_methods, /* tp_methods */
	0, /*tp_members*/
	TimeBaseObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	TimeBaseObj_tp_init, /* tp_init */
	TimeBaseObj_tp_alloc, /* tp_alloc */
	TimeBaseObj_tp_new, /* tp_new */
	TimeBaseObj_tp_free, /* tp_free */
};

/* -------------------- End object type TimeBase -------------------- */


/* ---------------------- Object type UserData ---------------------- */

PyTypeObject UserData_Type;

#define UserDataObj_Check(x) ((x)->ob_type == &UserData_Type || PyObject_TypeCheck((x), &UserData_Type))

typedef struct UserDataObject {
	PyObject_HEAD
	UserData ob_itself;
} UserDataObject;

PyObject *UserDataObj_New(UserData itself)
{
	UserDataObject *it;
	if (itself == NULL) {
	                                PyErr_SetString(Qt_Error,"Cannot create UserData from NULL pointer");
	                                return NULL;
	                        }
	it = PyObject_NEW(UserDataObject, &UserData_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}

int UserDataObj_Convert(PyObject *v, UserData *p_itself)
{
	if (v == Py_None)
	{
		*p_itself = NULL;
		return 1;
	}
	if (!UserDataObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "UserData required");
		return 0;
	}
	*p_itself = ((UserDataObject *)v)->ob_itself;
	return 1;
}

static void UserDataObj_dealloc(UserDataObject *self)
{
	if (self->ob_itself) DisposeUserData(self->ob_itself);
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *UserDataObj_GetUserData(UserDataObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle data;
	OSType udType;
	long index;
#ifndef GetUserData
	PyMac_PRECHECK(GetUserData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      ResObj_Convert, &data,
	                      PyMac_GetOSType, &udType,
	                      &index))
		return NULL;
	_err = GetUserData(_self->ob_itself,
	                   data,
	                   udType,
	                   index);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *UserDataObj_AddUserData(UserDataObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle data;
	OSType udType;
#ifndef AddUserData
	PyMac_PRECHECK(AddUserData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &data,
	                      PyMac_GetOSType, &udType))
		return NULL;
	_err = AddUserData(_self->ob_itself,
	                   data,
	                   udType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *UserDataObj_RemoveUserData(UserDataObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType udType;
	long index;
#ifndef RemoveUserData
	PyMac_PRECHECK(RemoveUserData);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetOSType, &udType,
	                      &index))
		return NULL;
	_err = RemoveUserData(_self->ob_itself,
	                      udType,
	                      index);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *UserDataObj_CountUserDataType(UserDataObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
	OSType udType;
#ifndef CountUserDataType
	PyMac_PRECHECK(CountUserDataType);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &udType))
		return NULL;
	_rv = CountUserDataType(_self->ob_itself,
	                        udType);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *UserDataObj_GetNextUserDataType(UserDataObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	OSType udType;
#ifndef GetNextUserDataType
	PyMac_PRECHECK(GetNextUserDataType);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &udType))
		return NULL;
	_rv = GetNextUserDataType(_self->ob_itself,
	                          udType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *UserDataObj_AddUserDataText(UserDataObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle data;
	OSType udType;
	long index;
	short itlRegionTag;
#ifndef AddUserDataText
	PyMac_PRECHECK(AddUserDataText);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&lh",
	                      ResObj_Convert, &data,
	                      PyMac_GetOSType, &udType,
	                      &index,
	                      &itlRegionTag))
		return NULL;
	_err = AddUserDataText(_self->ob_itself,
	                       data,
	                       udType,
	                       index,
	                       itlRegionTag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *UserDataObj_GetUserDataText(UserDataObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle data;
	OSType udType;
	long index;
	short itlRegionTag;
#ifndef GetUserDataText
	PyMac_PRECHECK(GetUserDataText);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&lh",
	                      ResObj_Convert, &data,
	                      PyMac_GetOSType, &udType,
	                      &index,
	                      &itlRegionTag))
		return NULL;
	_err = GetUserDataText(_self->ob_itself,
	                       data,
	                       udType,
	                       index,
	                       itlRegionTag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *UserDataObj_RemoveUserDataText(UserDataObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType udType;
	long index;
	short itlRegionTag;
#ifndef RemoveUserDataText
	PyMac_PRECHECK(RemoveUserDataText);
#endif
	if (!PyArg_ParseTuple(_args, "O&lh",
	                      PyMac_GetOSType, &udType,
	                      &index,
	                      &itlRegionTag))
		return NULL;
	_err = RemoveUserDataText(_self->ob_itself,
	                          udType,
	                          index,
	                          itlRegionTag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *UserDataObj_PutUserDataIntoHandle(UserDataObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle h;
#ifndef PutUserDataIntoHandle
	PyMac_PRECHECK(PutUserDataIntoHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &h))
		return NULL;
	_err = PutUserDataIntoHandle(_self->ob_itself,
	                             h);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *UserDataObj_CopyUserData(UserDataObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	UserData dstUserData;
	OSType copyRule;
#ifndef CopyUserData
	PyMac_PRECHECK(CopyUserData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      UserDataObj_Convert, &dstUserData,
	                      PyMac_GetOSType, &copyRule))
		return NULL;
	_err = CopyUserData(_self->ob_itself,
	                    dstUserData,
	                    copyRule);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef UserDataObj_methods[] = {
	{"GetUserData", (PyCFunction)UserDataObj_GetUserData, 1,
	 PyDoc_STR("(Handle data, OSType udType, long index) -> None")},
	{"AddUserData", (PyCFunction)UserDataObj_AddUserData, 1,
	 PyDoc_STR("(Handle data, OSType udType) -> None")},
	{"RemoveUserData", (PyCFunction)UserDataObj_RemoveUserData, 1,
	 PyDoc_STR("(OSType udType, long index) -> None")},
	{"CountUserDataType", (PyCFunction)UserDataObj_CountUserDataType, 1,
	 PyDoc_STR("(OSType udType) -> (short _rv)")},
	{"GetNextUserDataType", (PyCFunction)UserDataObj_GetNextUserDataType, 1,
	 PyDoc_STR("(OSType udType) -> (long _rv)")},
	{"AddUserDataText", (PyCFunction)UserDataObj_AddUserDataText, 1,
	 PyDoc_STR("(Handle data, OSType udType, long index, short itlRegionTag) -> None")},
	{"GetUserDataText", (PyCFunction)UserDataObj_GetUserDataText, 1,
	 PyDoc_STR("(Handle data, OSType udType, long index, short itlRegionTag) -> None")},
	{"RemoveUserDataText", (PyCFunction)UserDataObj_RemoveUserDataText, 1,
	 PyDoc_STR("(OSType udType, long index, short itlRegionTag) -> None")},
	{"PutUserDataIntoHandle", (PyCFunction)UserDataObj_PutUserDataIntoHandle, 1,
	 PyDoc_STR("(Handle h) -> None")},
	{"CopyUserData", (PyCFunction)UserDataObj_CopyUserData, 1,
	 PyDoc_STR("(UserData dstUserData, OSType copyRule) -> None")},
	{NULL, NULL, 0}
};

#define UserDataObj_getsetlist NULL


#define UserDataObj_compare NULL

#define UserDataObj_repr NULL

#define UserDataObj_hash NULL
#define UserDataObj_tp_init 0

#define UserDataObj_tp_alloc PyType_GenericAlloc

static PyObject *UserDataObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	UserData itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, UserDataObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((UserDataObject *)_self)->ob_itself = itself;
	return _self;
}

#define UserDataObj_tp_free PyObject_Del


PyTypeObject UserData_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.UserData", /*tp_name*/
	sizeof(UserDataObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) UserDataObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) UserDataObj_compare, /*tp_compare*/
	(reprfunc) UserDataObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) UserDataObj_hash, /*tp_hash*/
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
	UserDataObj_methods, /* tp_methods */
	0, /*tp_members*/
	UserDataObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	UserDataObj_tp_init, /* tp_init */
	UserDataObj_tp_alloc, /* tp_alloc */
	UserDataObj_tp_new, /* tp_new */
	UserDataObj_tp_free, /* tp_free */
};

/* -------------------- End object type UserData -------------------- */


/* ----------------------- Object type Media ------------------------ */

PyTypeObject Media_Type;

#define MediaObj_Check(x) ((x)->ob_type == &Media_Type || PyObject_TypeCheck((x), &Media_Type))

typedef struct MediaObject {
	PyObject_HEAD
	Media ob_itself;
} MediaObject;

PyObject *MediaObj_New(Media itself)
{
	MediaObject *it;
	if (itself == NULL) {
	                                PyErr_SetString(Qt_Error,"Cannot create Media from NULL pointer");
	                                return NULL;
	                        }
	it = PyObject_NEW(MediaObject, &Media_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}

int MediaObj_Convert(PyObject *v, Media *p_itself)
{
	if (v == Py_None)
	{
		*p_itself = NULL;
		return 1;
	}
	if (!MediaObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Media required");
		return 0;
	}
	*p_itself = ((MediaObject *)v)->ob_itself;
	return 1;
}

static void MediaObj_dealloc(MediaObject *self)
{
	if (self->ob_itself) DisposeTrackMedia(self->ob_itself);
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *MediaObj_LoadMediaIntoRam(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue time;
	TimeValue duration;
	long flags;
#ifndef LoadMediaIntoRam
	PyMac_PRECHECK(LoadMediaIntoRam);
#endif
	if (!PyArg_ParseTuple(_args, "lll",
	                      &time,
	                      &duration,
	                      &flags))
		return NULL;
	_err = LoadMediaIntoRam(_self->ob_itself,
	                        time,
	                        duration,
	                        flags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_GetMediaTrack(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Track _rv;
#ifndef GetMediaTrack
	PyMac_PRECHECK(GetMediaTrack);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMediaTrack(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     TrackObj_New, _rv);
	return _res;
}

static PyObject *MediaObj_GetMediaCreationTime(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	unsigned long _rv;
#ifndef GetMediaCreationTime
	PyMac_PRECHECK(GetMediaCreationTime);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMediaCreationTime(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MediaObj_GetMediaModificationTime(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	unsigned long _rv;
#ifndef GetMediaModificationTime
	PyMac_PRECHECK(GetMediaModificationTime);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMediaModificationTime(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MediaObj_GetMediaTimeScale(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeScale _rv;
#ifndef GetMediaTimeScale
	PyMac_PRECHECK(GetMediaTimeScale);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMediaTimeScale(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MediaObj_SetMediaTimeScale(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeScale timeScale;
#ifndef SetMediaTimeScale
	PyMac_PRECHECK(SetMediaTimeScale);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &timeScale))
		return NULL;
	SetMediaTimeScale(_self->ob_itself,
	                  timeScale);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_GetMediaDuration(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue _rv;
#ifndef GetMediaDuration
	PyMac_PRECHECK(GetMediaDuration);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMediaDuration(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MediaObj_GetMediaLanguage(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
#ifndef GetMediaLanguage
	PyMac_PRECHECK(GetMediaLanguage);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMediaLanguage(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *MediaObj_SetMediaLanguage(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short language;
#ifndef SetMediaLanguage
	PyMac_PRECHECK(SetMediaLanguage);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &language))
		return NULL;
	SetMediaLanguage(_self->ob_itself,
	                 language);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_GetMediaQuality(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
#ifndef GetMediaQuality
	PyMac_PRECHECK(GetMediaQuality);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMediaQuality(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *MediaObj_SetMediaQuality(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short quality;
#ifndef SetMediaQuality
	PyMac_PRECHECK(SetMediaQuality);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &quality))
		return NULL;
	SetMediaQuality(_self->ob_itself,
	                quality);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_GetMediaHandlerDescription(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSType mediaType;
	Str255 creatorName;
	OSType creatorManufacturer;
#ifndef GetMediaHandlerDescription
	PyMac_PRECHECK(GetMediaHandlerDescription);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, creatorName))
		return NULL;
	GetMediaHandlerDescription(_self->ob_itself,
	                           &mediaType,
	                           creatorName,
	                           &creatorManufacturer);
	_res = Py_BuildValue("O&O&",
	                     PyMac_BuildOSType, mediaType,
	                     PyMac_BuildOSType, creatorManufacturer);
	return _res;
}

static PyObject *MediaObj_GetMediaUserData(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UserData _rv;
#ifndef GetMediaUserData
	PyMac_PRECHECK(GetMediaUserData);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMediaUserData(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     UserDataObj_New, _rv);
	return _res;
}

static PyObject *MediaObj_GetMediaHandler(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MediaHandler _rv;
#ifndef GetMediaHandler
	PyMac_PRECHECK(GetMediaHandler);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMediaHandler(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, _rv);
	return _res;
}

static PyObject *MediaObj_SetMediaHandler(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	MediaHandlerComponent mH;
#ifndef SetMediaHandler
	PyMac_PRECHECK(SetMediaHandler);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpObj_Convert, &mH))
		return NULL;
	_err = SetMediaHandler(_self->ob_itself,
	                       mH);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_BeginMediaEdits(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef BeginMediaEdits
	PyMac_PRECHECK(BeginMediaEdits);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = BeginMediaEdits(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_EndMediaEdits(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef EndMediaEdits
	PyMac_PRECHECK(EndMediaEdits);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = EndMediaEdits(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_SetMediaDefaultDataRefIndex(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short index;
#ifndef SetMediaDefaultDataRefIndex
	PyMac_PRECHECK(SetMediaDefaultDataRefIndex);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &index))
		return NULL;
	_err = SetMediaDefaultDataRefIndex(_self->ob_itself,
	                                   index);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_GetMediaDataHandlerDescription(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short index;
	OSType dhType;
	Str255 creatorName;
	OSType creatorManufacturer;
#ifndef GetMediaDataHandlerDescription
	PyMac_PRECHECK(GetMediaDataHandlerDescription);
#endif
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &index,
	                      PyMac_GetStr255, creatorName))
		return NULL;
	GetMediaDataHandlerDescription(_self->ob_itself,
	                               index,
	                               &dhType,
	                               creatorName,
	                               &creatorManufacturer);
	_res = Py_BuildValue("O&O&",
	                     PyMac_BuildOSType, dhType,
	                     PyMac_BuildOSType, creatorManufacturer);
	return _res;
}

static PyObject *MediaObj_GetMediaDataHandler(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DataHandler _rv;
	short index;
#ifndef GetMediaDataHandler
	PyMac_PRECHECK(GetMediaDataHandler);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &index))
		return NULL;
	_rv = GetMediaDataHandler(_self->ob_itself,
	                          index);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, _rv);
	return _res;
}

static PyObject *MediaObj_SetMediaDataHandler(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short index;
	DataHandlerComponent dataHandler;
#ifndef SetMediaDataHandler
	PyMac_PRECHECK(SetMediaDataHandler);
#endif
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &index,
	                      CmpObj_Convert, &dataHandler))
		return NULL;
	_err = SetMediaDataHandler(_self->ob_itself,
	                           index,
	                           dataHandler);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_GetMediaSampleDescriptionCount(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef GetMediaSampleDescriptionCount
	PyMac_PRECHECK(GetMediaSampleDescriptionCount);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMediaSampleDescriptionCount(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MediaObj_GetMediaSampleDescription(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long index;
	SampleDescriptionHandle descH;
#ifndef GetMediaSampleDescription
	PyMac_PRECHECK(GetMediaSampleDescription);
#endif
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &index,
	                      ResObj_Convert, &descH))
		return NULL;
	GetMediaSampleDescription(_self->ob_itself,
	                          index,
	                          descH);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_SetMediaSampleDescription(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long index;
	SampleDescriptionHandle descH;
#ifndef SetMediaSampleDescription
	PyMac_PRECHECK(SetMediaSampleDescription);
#endif
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &index,
	                      ResObj_Convert, &descH))
		return NULL;
	_err = SetMediaSampleDescription(_self->ob_itself,
	                                 index,
	                                 descH);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_GetMediaSampleCount(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef GetMediaSampleCount
	PyMac_PRECHECK(GetMediaSampleCount);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMediaSampleCount(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MediaObj_GetMediaSyncSampleCount(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef GetMediaSyncSampleCount
	PyMac_PRECHECK(GetMediaSyncSampleCount);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMediaSyncSampleCount(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MediaObj_SampleNumToMediaTime(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long logicalSampleNum;
	TimeValue sampleTime;
	TimeValue sampleDuration;
#ifndef SampleNumToMediaTime
	PyMac_PRECHECK(SampleNumToMediaTime);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &logicalSampleNum))
		return NULL;
	SampleNumToMediaTime(_self->ob_itself,
	                     logicalSampleNum,
	                     &sampleTime,
	                     &sampleDuration);
	_res = Py_BuildValue("ll",
	                     sampleTime,
	                     sampleDuration);
	return _res;
}

static PyObject *MediaObj_MediaTimeToSampleNum(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue time;
	long sampleNum;
	TimeValue sampleTime;
	TimeValue sampleDuration;
#ifndef MediaTimeToSampleNum
	PyMac_PRECHECK(MediaTimeToSampleNum);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &time))
		return NULL;
	MediaTimeToSampleNum(_self->ob_itself,
	                     time,
	                     &sampleNum,
	                     &sampleTime,
	                     &sampleDuration);
	_res = Py_BuildValue("lll",
	                     sampleNum,
	                     sampleTime,
	                     sampleDuration);
	return _res;
}

static PyObject *MediaObj_AddMediaSample(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataIn;
	long inOffset;
	unsigned long size;
	TimeValue durationPerSample;
	SampleDescriptionHandle sampleDescriptionH;
	long numberOfSamples;
	short sampleFlags;
	TimeValue sampleTime;
#ifndef AddMediaSample
	PyMac_PRECHECK(AddMediaSample);
#endif
	if (!PyArg_ParseTuple(_args, "O&lllO&lh",
	                      ResObj_Convert, &dataIn,
	                      &inOffset,
	                      &size,
	                      &durationPerSample,
	                      ResObj_Convert, &sampleDescriptionH,
	                      &numberOfSamples,
	                      &sampleFlags))
		return NULL;
	_err = AddMediaSample(_self->ob_itself,
	                      dataIn,
	                      inOffset,
	                      size,
	                      durationPerSample,
	                      sampleDescriptionH,
	                      numberOfSamples,
	                      sampleFlags,
	                      &sampleTime);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     sampleTime);
	return _res;
}

static PyObject *MediaObj_AddMediaSampleReference(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long dataOffset;
	unsigned long size;
	TimeValue durationPerSample;
	SampleDescriptionHandle sampleDescriptionH;
	long numberOfSamples;
	short sampleFlags;
	TimeValue sampleTime;
#ifndef AddMediaSampleReference
	PyMac_PRECHECK(AddMediaSampleReference);
#endif
	if (!PyArg_ParseTuple(_args, "lllO&lh",
	                      &dataOffset,
	                      &size,
	                      &durationPerSample,
	                      ResObj_Convert, &sampleDescriptionH,
	                      &numberOfSamples,
	                      &sampleFlags))
		return NULL;
	_err = AddMediaSampleReference(_self->ob_itself,
	                               dataOffset,
	                               size,
	                               durationPerSample,
	                               sampleDescriptionH,
	                               numberOfSamples,
	                               sampleFlags,
	                               &sampleTime);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     sampleTime);
	return _res;
}

static PyObject *MediaObj_GetMediaSample(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataOut;
	long maxSizeToGrow;
	long size;
	TimeValue time;
	TimeValue sampleTime;
	TimeValue durationPerSample;
	SampleDescriptionHandle sampleDescriptionH;
	long sampleDescriptionIndex;
	long maxNumberOfSamples;
	long numberOfSamples;
	short sampleFlags;
#ifndef GetMediaSample
	PyMac_PRECHECK(GetMediaSample);
#endif
	if (!PyArg_ParseTuple(_args, "O&llO&l",
	                      ResObj_Convert, &dataOut,
	                      &maxSizeToGrow,
	                      &time,
	                      ResObj_Convert, &sampleDescriptionH,
	                      &maxNumberOfSamples))
		return NULL;
	_err = GetMediaSample(_self->ob_itself,
	                      dataOut,
	                      maxSizeToGrow,
	                      &size,
	                      time,
	                      &sampleTime,
	                      &durationPerSample,
	                      sampleDescriptionH,
	                      &sampleDescriptionIndex,
	                      maxNumberOfSamples,
	                      &numberOfSamples,
	                      &sampleFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("lllllh",
	                     size,
	                     sampleTime,
	                     durationPerSample,
	                     sampleDescriptionIndex,
	                     numberOfSamples,
	                     sampleFlags);
	return _res;
}

static PyObject *MediaObj_GetMediaSampleReference(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long dataOffset;
	long size;
	TimeValue time;
	TimeValue sampleTime;
	TimeValue durationPerSample;
	SampleDescriptionHandle sampleDescriptionH;
	long sampleDescriptionIndex;
	long maxNumberOfSamples;
	long numberOfSamples;
	short sampleFlags;
#ifndef GetMediaSampleReference
	PyMac_PRECHECK(GetMediaSampleReference);
#endif
	if (!PyArg_ParseTuple(_args, "lO&l",
	                      &time,
	                      ResObj_Convert, &sampleDescriptionH,
	                      &maxNumberOfSamples))
		return NULL;
	_err = GetMediaSampleReference(_self->ob_itself,
	                               &dataOffset,
	                               &size,
	                               time,
	                               &sampleTime,
	                               &durationPerSample,
	                               sampleDescriptionH,
	                               &sampleDescriptionIndex,
	                               maxNumberOfSamples,
	                               &numberOfSamples,
	                               &sampleFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("llllllh",
	                     dataOffset,
	                     size,
	                     sampleTime,
	                     durationPerSample,
	                     sampleDescriptionIndex,
	                     numberOfSamples,
	                     sampleFlags);
	return _res;
}

static PyObject *MediaObj_SetMediaPreferredChunkSize(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long maxChunkSize;
#ifndef SetMediaPreferredChunkSize
	PyMac_PRECHECK(SetMediaPreferredChunkSize);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &maxChunkSize))
		return NULL;
	_err = SetMediaPreferredChunkSize(_self->ob_itself,
	                                  maxChunkSize);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_GetMediaPreferredChunkSize(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long maxChunkSize;
#ifndef GetMediaPreferredChunkSize
	PyMac_PRECHECK(GetMediaPreferredChunkSize);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetMediaPreferredChunkSize(_self->ob_itself,
	                                  &maxChunkSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     maxChunkSize);
	return _res;
}

static PyObject *MediaObj_SetMediaShadowSync(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long frameDiffSampleNum;
	long syncSampleNum;
#ifndef SetMediaShadowSync
	PyMac_PRECHECK(SetMediaShadowSync);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &frameDiffSampleNum,
	                      &syncSampleNum))
		return NULL;
	_err = SetMediaShadowSync(_self->ob_itself,
	                          frameDiffSampleNum,
	                          syncSampleNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_GetMediaShadowSync(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long frameDiffSampleNum;
	long syncSampleNum;
#ifndef GetMediaShadowSync
	PyMac_PRECHECK(GetMediaShadowSync);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &frameDiffSampleNum))
		return NULL;
	_err = GetMediaShadowSync(_self->ob_itself,
	                          frameDiffSampleNum,
	                          &syncSampleNum);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     syncSampleNum);
	return _res;
}

static PyObject *MediaObj_GetMediaDataSize(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	TimeValue startTime;
	TimeValue duration;
#ifndef GetMediaDataSize
	PyMac_PRECHECK(GetMediaDataSize);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &startTime,
	                      &duration))
		return NULL;
	_rv = GetMediaDataSize(_self->ob_itself,
	                       startTime,
	                       duration);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MediaObj_GetMediaDataSize64(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue startTime;
	TimeValue duration;
	wide dataSize;
#ifndef GetMediaDataSize64
	PyMac_PRECHECK(GetMediaDataSize64);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &startTime,
	                      &duration))
		return NULL;
	_err = GetMediaDataSize64(_self->ob_itself,
	                          startTime,
	                          duration,
	                          &dataSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_Buildwide, dataSize);
	return _res;
}

static PyObject *MediaObj_CopyMediaUserData(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Media dstMedia;
	OSType copyRule;
#ifndef CopyMediaUserData
	PyMac_PRECHECK(CopyMediaUserData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      MediaObj_Convert, &dstMedia,
	                      PyMac_GetOSType, &copyRule))
		return NULL;
	_err = CopyMediaUserData(_self->ob_itself,
	                         dstMedia,
	                         copyRule);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_GetMediaNextInterestingTime(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short interestingTimeFlags;
	TimeValue time;
	Fixed rate;
	TimeValue interestingTime;
	TimeValue interestingDuration;
#ifndef GetMediaNextInterestingTime
	PyMac_PRECHECK(GetMediaNextInterestingTime);
#endif
	if (!PyArg_ParseTuple(_args, "hlO&",
	                      &interestingTimeFlags,
	                      &time,
	                      PyMac_GetFixed, &rate))
		return NULL;
	GetMediaNextInterestingTime(_self->ob_itself,
	                            interestingTimeFlags,
	                            time,
	                            rate,
	                            &interestingTime,
	                            &interestingDuration);
	_res = Py_BuildValue("ll",
	                     interestingTime,
	                     interestingDuration);
	return _res;
}

static PyObject *MediaObj_GetMediaDataRef(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short index;
	Handle dataRef;
	OSType dataRefType;
	long dataRefAttributes;
#ifndef GetMediaDataRef
	PyMac_PRECHECK(GetMediaDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &index))
		return NULL;
	_err = GetMediaDataRef(_self->ob_itself,
	                       index,
	                       &dataRef,
	                       &dataRefType,
	                       &dataRefAttributes);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&l",
	                     ResObj_New, dataRef,
	                     PyMac_BuildOSType, dataRefType,
	                     dataRefAttributes);
	return _res;
}

static PyObject *MediaObj_SetMediaDataRef(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short index;
	Handle dataRef;
	OSType dataRefType;
#ifndef SetMediaDataRef
	PyMac_PRECHECK(SetMediaDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "hO&O&",
	                      &index,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_err = SetMediaDataRef(_self->ob_itself,
	                       index,
	                       dataRef,
	                       dataRefType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_SetMediaDataRefAttributes(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short index;
	long dataRefAttributes;
#ifndef SetMediaDataRefAttributes
	PyMac_PRECHECK(SetMediaDataRefAttributes);
#endif
	if (!PyArg_ParseTuple(_args, "hl",
	                      &index,
	                      &dataRefAttributes))
		return NULL;
	_err = SetMediaDataRefAttributes(_self->ob_itself,
	                                 index,
	                                 dataRefAttributes);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_AddMediaDataRef(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short index;
	Handle dataRef;
	OSType dataRefType;
#ifndef AddMediaDataRef
	PyMac_PRECHECK(AddMediaDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_err = AddMediaDataRef(_self->ob_itself,
	                       &index,
	                       dataRef,
	                       dataRefType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     index);
	return _res;
}

static PyObject *MediaObj_GetMediaDataRefCount(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short count;
#ifndef GetMediaDataRefCount
	PyMac_PRECHECK(GetMediaDataRefCount);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetMediaDataRefCount(_self->ob_itself,
	                            &count);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     count);
	return _res;
}

static PyObject *MediaObj_SetMediaPlayHints(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long flags;
	long flagsMask;
#ifndef SetMediaPlayHints
	PyMac_PRECHECK(SetMediaPlayHints);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &flags,
	                      &flagsMask))
		return NULL;
	SetMediaPlayHints(_self->ob_itself,
	                  flags,
	                  flagsMask);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MediaObj_GetMediaPlayHints(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long flags;
#ifndef GetMediaPlayHints
	PyMac_PRECHECK(GetMediaPlayHints);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetMediaPlayHints(_self->ob_itself,
	                  &flags);
	_res = Py_BuildValue("l",
	                     flags);
	return _res;
}

static PyObject *MediaObj_GetMediaNextInterestingTimeOnly(MediaObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short interestingTimeFlags;
	TimeValue time;
	Fixed rate;
	TimeValue interestingTime;
#ifndef GetMediaNextInterestingTimeOnly
	PyMac_PRECHECK(GetMediaNextInterestingTimeOnly);
#endif
	if (!PyArg_ParseTuple(_args, "hlO&",
	                      &interestingTimeFlags,
	                      &time,
	                      PyMac_GetFixed, &rate))
		return NULL;
	GetMediaNextInterestingTimeOnly(_self->ob_itself,
	                                interestingTimeFlags,
	                                time,
	                                rate,
	                                &interestingTime);
	_res = Py_BuildValue("l",
	                     interestingTime);
	return _res;
}

static PyMethodDef MediaObj_methods[] = {
	{"LoadMediaIntoRam", (PyCFunction)MediaObj_LoadMediaIntoRam, 1,
	 PyDoc_STR("(TimeValue time, TimeValue duration, long flags) -> None")},
	{"GetMediaTrack", (PyCFunction)MediaObj_GetMediaTrack, 1,
	 PyDoc_STR("() -> (Track _rv)")},
	{"GetMediaCreationTime", (PyCFunction)MediaObj_GetMediaCreationTime, 1,
	 PyDoc_STR("() -> (unsigned long _rv)")},
	{"GetMediaModificationTime", (PyCFunction)MediaObj_GetMediaModificationTime, 1,
	 PyDoc_STR("() -> (unsigned long _rv)")},
	{"GetMediaTimeScale", (PyCFunction)MediaObj_GetMediaTimeScale, 1,
	 PyDoc_STR("() -> (TimeScale _rv)")},
	{"SetMediaTimeScale", (PyCFunction)MediaObj_SetMediaTimeScale, 1,
	 PyDoc_STR("(TimeScale timeScale) -> None")},
	{"GetMediaDuration", (PyCFunction)MediaObj_GetMediaDuration, 1,
	 PyDoc_STR("() -> (TimeValue _rv)")},
	{"GetMediaLanguage", (PyCFunction)MediaObj_GetMediaLanguage, 1,
	 PyDoc_STR("() -> (short _rv)")},
	{"SetMediaLanguage", (PyCFunction)MediaObj_SetMediaLanguage, 1,
	 PyDoc_STR("(short language) -> None")},
	{"GetMediaQuality", (PyCFunction)MediaObj_GetMediaQuality, 1,
	 PyDoc_STR("() -> (short _rv)")},
	{"SetMediaQuality", (PyCFunction)MediaObj_SetMediaQuality, 1,
	 PyDoc_STR("(short quality) -> None")},
	{"GetMediaHandlerDescription", (PyCFunction)MediaObj_GetMediaHandlerDescription, 1,
	 PyDoc_STR("(Str255 creatorName) -> (OSType mediaType, OSType creatorManufacturer)")},
	{"GetMediaUserData", (PyCFunction)MediaObj_GetMediaUserData, 1,
	 PyDoc_STR("() -> (UserData _rv)")},
	{"GetMediaHandler", (PyCFunction)MediaObj_GetMediaHandler, 1,
	 PyDoc_STR("() -> (MediaHandler _rv)")},
	{"SetMediaHandler", (PyCFunction)MediaObj_SetMediaHandler, 1,
	 PyDoc_STR("(MediaHandlerComponent mH) -> None")},
	{"BeginMediaEdits", (PyCFunction)MediaObj_BeginMediaEdits, 1,
	 PyDoc_STR("() -> None")},
	{"EndMediaEdits", (PyCFunction)MediaObj_EndMediaEdits, 1,
	 PyDoc_STR("() -> None")},
	{"SetMediaDefaultDataRefIndex", (PyCFunction)MediaObj_SetMediaDefaultDataRefIndex, 1,
	 PyDoc_STR("(short index) -> None")},
	{"GetMediaDataHandlerDescription", (PyCFunction)MediaObj_GetMediaDataHandlerDescription, 1,
	 PyDoc_STR("(short index, Str255 creatorName) -> (OSType dhType, OSType creatorManufacturer)")},
	{"GetMediaDataHandler", (PyCFunction)MediaObj_GetMediaDataHandler, 1,
	 PyDoc_STR("(short index) -> (DataHandler _rv)")},
	{"SetMediaDataHandler", (PyCFunction)MediaObj_SetMediaDataHandler, 1,
	 PyDoc_STR("(short index, DataHandlerComponent dataHandler) -> None")},
	{"GetMediaSampleDescriptionCount", (PyCFunction)MediaObj_GetMediaSampleDescriptionCount, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"GetMediaSampleDescription", (PyCFunction)MediaObj_GetMediaSampleDescription, 1,
	 PyDoc_STR("(long index, SampleDescriptionHandle descH) -> None")},
	{"SetMediaSampleDescription", (PyCFunction)MediaObj_SetMediaSampleDescription, 1,
	 PyDoc_STR("(long index, SampleDescriptionHandle descH) -> None")},
	{"GetMediaSampleCount", (PyCFunction)MediaObj_GetMediaSampleCount, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"GetMediaSyncSampleCount", (PyCFunction)MediaObj_GetMediaSyncSampleCount, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"SampleNumToMediaTime", (PyCFunction)MediaObj_SampleNumToMediaTime, 1,
	 PyDoc_STR("(long logicalSampleNum) -> (TimeValue sampleTime, TimeValue sampleDuration)")},
	{"MediaTimeToSampleNum", (PyCFunction)MediaObj_MediaTimeToSampleNum, 1,
	 PyDoc_STR("(TimeValue time) -> (long sampleNum, TimeValue sampleTime, TimeValue sampleDuration)")},
	{"AddMediaSample", (PyCFunction)MediaObj_AddMediaSample, 1,
	 PyDoc_STR("(Handle dataIn, long inOffset, unsigned long size, TimeValue durationPerSample, SampleDescriptionHandle sampleDescriptionH, long numberOfSamples, short sampleFlags) -> (TimeValue sampleTime)")},
	{"AddMediaSampleReference", (PyCFunction)MediaObj_AddMediaSampleReference, 1,
	 PyDoc_STR("(long dataOffset, unsigned long size, TimeValue durationPerSample, SampleDescriptionHandle sampleDescriptionH, long numberOfSamples, short sampleFlags) -> (TimeValue sampleTime)")},
	{"GetMediaSample", (PyCFunction)MediaObj_GetMediaSample, 1,
	 PyDoc_STR("(Handle dataOut, long maxSizeToGrow, TimeValue time, SampleDescriptionHandle sampleDescriptionH, long maxNumberOfSamples) -> (long size, TimeValue sampleTime, TimeValue durationPerSample, long sampleDescriptionIndex, long numberOfSamples, short sampleFlags)")},
	{"GetMediaSampleReference", (PyCFunction)MediaObj_GetMediaSampleReference, 1,
	 PyDoc_STR("(TimeValue time, SampleDescriptionHandle sampleDescriptionH, long maxNumberOfSamples) -> (long dataOffset, long size, TimeValue sampleTime, TimeValue durationPerSample, long sampleDescriptionIndex, long numberOfSamples, short sampleFlags)")},
	{"SetMediaPreferredChunkSize", (PyCFunction)MediaObj_SetMediaPreferredChunkSize, 1,
	 PyDoc_STR("(long maxChunkSize) -> None")},
	{"GetMediaPreferredChunkSize", (PyCFunction)MediaObj_GetMediaPreferredChunkSize, 1,
	 PyDoc_STR("() -> (long maxChunkSize)")},
	{"SetMediaShadowSync", (PyCFunction)MediaObj_SetMediaShadowSync, 1,
	 PyDoc_STR("(long frameDiffSampleNum, long syncSampleNum) -> None")},
	{"GetMediaShadowSync", (PyCFunction)MediaObj_GetMediaShadowSync, 1,
	 PyDoc_STR("(long frameDiffSampleNum) -> (long syncSampleNum)")},
	{"GetMediaDataSize", (PyCFunction)MediaObj_GetMediaDataSize, 1,
	 PyDoc_STR("(TimeValue startTime, TimeValue duration) -> (long _rv)")},
	{"GetMediaDataSize64", (PyCFunction)MediaObj_GetMediaDataSize64, 1,
	 PyDoc_STR("(TimeValue startTime, TimeValue duration) -> (wide dataSize)")},
	{"CopyMediaUserData", (PyCFunction)MediaObj_CopyMediaUserData, 1,
	 PyDoc_STR("(Media dstMedia, OSType copyRule) -> None")},
	{"GetMediaNextInterestingTime", (PyCFunction)MediaObj_GetMediaNextInterestingTime, 1,
	 PyDoc_STR("(short interestingTimeFlags, TimeValue time, Fixed rate) -> (TimeValue interestingTime, TimeValue interestingDuration)")},
	{"GetMediaDataRef", (PyCFunction)MediaObj_GetMediaDataRef, 1,
	 PyDoc_STR("(short index) -> (Handle dataRef, OSType dataRefType, long dataRefAttributes)")},
	{"SetMediaDataRef", (PyCFunction)MediaObj_SetMediaDataRef, 1,
	 PyDoc_STR("(short index, Handle dataRef, OSType dataRefType) -> None")},
	{"SetMediaDataRefAttributes", (PyCFunction)MediaObj_SetMediaDataRefAttributes, 1,
	 PyDoc_STR("(short index, long dataRefAttributes) -> None")},
	{"AddMediaDataRef", (PyCFunction)MediaObj_AddMediaDataRef, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataRefType) -> (short index)")},
	{"GetMediaDataRefCount", (PyCFunction)MediaObj_GetMediaDataRefCount, 1,
	 PyDoc_STR("() -> (short count)")},
	{"SetMediaPlayHints", (PyCFunction)MediaObj_SetMediaPlayHints, 1,
	 PyDoc_STR("(long flags, long flagsMask) -> None")},
	{"GetMediaPlayHints", (PyCFunction)MediaObj_GetMediaPlayHints, 1,
	 PyDoc_STR("() -> (long flags)")},
	{"GetMediaNextInterestingTimeOnly", (PyCFunction)MediaObj_GetMediaNextInterestingTimeOnly, 1,
	 PyDoc_STR("(short interestingTimeFlags, TimeValue time, Fixed rate) -> (TimeValue interestingTime)")},
	{NULL, NULL, 0}
};

#define MediaObj_getsetlist NULL


#define MediaObj_compare NULL

#define MediaObj_repr NULL

#define MediaObj_hash NULL
#define MediaObj_tp_init 0

#define MediaObj_tp_alloc PyType_GenericAlloc

static PyObject *MediaObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	Media itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, MediaObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((MediaObject *)_self)->ob_itself = itself;
	return _self;
}

#define MediaObj_tp_free PyObject_Del


PyTypeObject Media_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.Media", /*tp_name*/
	sizeof(MediaObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) MediaObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) MediaObj_compare, /*tp_compare*/
	(reprfunc) MediaObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) MediaObj_hash, /*tp_hash*/
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
	MediaObj_methods, /* tp_methods */
	0, /*tp_members*/
	MediaObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	MediaObj_tp_init, /* tp_init */
	MediaObj_tp_alloc, /* tp_alloc */
	MediaObj_tp_new, /* tp_new */
	MediaObj_tp_free, /* tp_free */
};

/* --------------------- End object type Media ---------------------- */


/* ----------------------- Object type Track ------------------------ */

PyTypeObject Track_Type;

#define TrackObj_Check(x) ((x)->ob_type == &Track_Type || PyObject_TypeCheck((x), &Track_Type))

typedef struct TrackObject {
	PyObject_HEAD
	Track ob_itself;
} TrackObject;

PyObject *TrackObj_New(Track itself)
{
	TrackObject *it;
	if (itself == NULL) {
	                                PyErr_SetString(Qt_Error,"Cannot create Track from NULL pointer");
	                                return NULL;
	                        }
	it = PyObject_NEW(TrackObject, &Track_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}

int TrackObj_Convert(PyObject *v, Track *p_itself)
{
	if (v == Py_None)
	{
		*p_itself = NULL;
		return 1;
	}
	if (!TrackObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Track required");
		return 0;
	}
	*p_itself = ((TrackObject *)v)->ob_itself;
	return 1;
}

static void TrackObj_dealloc(TrackObject *self)
{
	if (self->ob_itself) DisposeMovieTrack(self->ob_itself);
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *TrackObj_LoadTrackIntoRam(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue time;
	TimeValue duration;
	long flags;
#ifndef LoadTrackIntoRam
	PyMac_PRECHECK(LoadTrackIntoRam);
#endif
	if (!PyArg_ParseTuple(_args, "lll",
	                      &time,
	                      &duration,
	                      &flags))
		return NULL;
	_err = LoadTrackIntoRam(_self->ob_itself,
	                        time,
	                        duration,
	                        flags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackPict(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PicHandle _rv;
	TimeValue time;
#ifndef GetTrackPict
	PyMac_PRECHECK(GetTrackPict);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &time))
		return NULL;
	_rv = GetTrackPict(_self->ob_itself,
	                   time);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackClipRgn(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
#ifndef GetTrackClipRgn
	PyMac_PRECHECK(GetTrackClipRgn);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackClipRgn(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_SetTrackClipRgn(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle theClip;
#ifndef SetTrackClipRgn
	PyMac_PRECHECK(SetTrackClipRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theClip))
		return NULL;
	SetTrackClipRgn(_self->ob_itself,
	                theClip);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackDisplayBoundsRgn(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
#ifndef GetTrackDisplayBoundsRgn
	PyMac_PRECHECK(GetTrackDisplayBoundsRgn);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackDisplayBoundsRgn(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackMovieBoundsRgn(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
#ifndef GetTrackMovieBoundsRgn
	PyMac_PRECHECK(GetTrackMovieBoundsRgn);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackMovieBoundsRgn(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackBoundsRgn(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
#ifndef GetTrackBoundsRgn
	PyMac_PRECHECK(GetTrackBoundsRgn);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackBoundsRgn(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackMatte(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PixMapHandle _rv;
#ifndef GetTrackMatte
	PyMac_PRECHECK(GetTrackMatte);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackMatte(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_SetTrackMatte(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PixMapHandle theMatte;
#ifndef SetTrackMatte
	PyMac_PRECHECK(SetTrackMatte);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theMatte))
		return NULL;
	SetTrackMatte(_self->ob_itself,
	              theMatte);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackID(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef GetTrackID
	PyMac_PRECHECK(GetTrackID);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackID(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackMovie(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie _rv;
#ifndef GetTrackMovie
	PyMac_PRECHECK(GetTrackMovie);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackMovie(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     MovieObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackCreationTime(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	unsigned long _rv;
#ifndef GetTrackCreationTime
	PyMac_PRECHECK(GetTrackCreationTime);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackCreationTime(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackModificationTime(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	unsigned long _rv;
#ifndef GetTrackModificationTime
	PyMac_PRECHECK(GetTrackModificationTime);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackModificationTime(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackEnabled(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef GetTrackEnabled
	PyMac_PRECHECK(GetTrackEnabled);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackEnabled(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_SetTrackEnabled(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean isEnabled;
#ifndef SetTrackEnabled
	PyMac_PRECHECK(SetTrackEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &isEnabled))
		return NULL;
	SetTrackEnabled(_self->ob_itself,
	                isEnabled);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackUsage(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef GetTrackUsage
	PyMac_PRECHECK(GetTrackUsage);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackUsage(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_SetTrackUsage(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long usage;
#ifndef SetTrackUsage
	PyMac_PRECHECK(SetTrackUsage);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &usage))
		return NULL;
	SetTrackUsage(_self->ob_itself,
	              usage);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackDuration(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue _rv;
#ifndef GetTrackDuration
	PyMac_PRECHECK(GetTrackDuration);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackDuration(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackOffset(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue _rv;
#ifndef GetTrackOffset
	PyMac_PRECHECK(GetTrackOffset);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackOffset(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_SetTrackOffset(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue movieOffsetTime;
#ifndef SetTrackOffset
	PyMac_PRECHECK(SetTrackOffset);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &movieOffsetTime))
		return NULL;
	SetTrackOffset(_self->ob_itself,
	               movieOffsetTime);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackLayer(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
#ifndef GetTrackLayer
	PyMac_PRECHECK(GetTrackLayer);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackLayer(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_SetTrackLayer(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short layer;
#ifndef SetTrackLayer
	PyMac_PRECHECK(SetTrackLayer);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &layer))
		return NULL;
	SetTrackLayer(_self->ob_itself,
	              layer);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackAlternate(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Track _rv;
#ifndef GetTrackAlternate
	PyMac_PRECHECK(GetTrackAlternate);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackAlternate(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     TrackObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_SetTrackAlternate(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Track alternateT;
#ifndef SetTrackAlternate
	PyMac_PRECHECK(SetTrackAlternate);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      TrackObj_Convert, &alternateT))
		return NULL;
	SetTrackAlternate(_self->ob_itself,
	                  alternateT);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackVolume(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
#ifndef GetTrackVolume
	PyMac_PRECHECK(GetTrackVolume);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackVolume(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_SetTrackVolume(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short volume;
#ifndef SetTrackVolume
	PyMac_PRECHECK(SetTrackVolume);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &volume))
		return NULL;
	SetTrackVolume(_self->ob_itself,
	               volume);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackDimensions(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed width;
	Fixed height;
#ifndef GetTrackDimensions
	PyMac_PRECHECK(GetTrackDimensions);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetTrackDimensions(_self->ob_itself,
	                   &width,
	                   &height);
	_res = Py_BuildValue("O&O&",
	                     PyMac_BuildFixed, width,
	                     PyMac_BuildFixed, height);
	return _res;
}

static PyObject *TrackObj_SetTrackDimensions(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed width;
	Fixed height;
#ifndef SetTrackDimensions
	PyMac_PRECHECK(SetTrackDimensions);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFixed, &width,
	                      PyMac_GetFixed, &height))
		return NULL;
	SetTrackDimensions(_self->ob_itself,
	                   width,
	                   height);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackUserData(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UserData _rv;
#ifndef GetTrackUserData
	PyMac_PRECHECK(GetTrackUserData);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackUserData(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     UserDataObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackSoundLocalizationSettings(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle settings;
#ifndef GetTrackSoundLocalizationSettings
	PyMac_PRECHECK(GetTrackSoundLocalizationSettings);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetTrackSoundLocalizationSettings(_self->ob_itself,
	                                         &settings);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, settings);
	return _res;
}

static PyObject *TrackObj_SetTrackSoundLocalizationSettings(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle settings;
#ifndef SetTrackSoundLocalizationSettings
	PyMac_PRECHECK(SetTrackSoundLocalizationSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &settings))
		return NULL;
	_err = SetTrackSoundLocalizationSettings(_self->ob_itself,
	                                         settings);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_NewTrackMedia(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Media _rv;
	OSType mediaType;
	TimeScale timeScale;
	Handle dataRef;
	OSType dataRefType;
#ifndef NewTrackMedia
	PyMac_PRECHECK(NewTrackMedia);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&O&",
	                      PyMac_GetOSType, &mediaType,
	                      &timeScale,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_rv = NewTrackMedia(_self->ob_itself,
	                    mediaType,
	                    timeScale,
	                    dataRef,
	                    dataRefType);
	_res = Py_BuildValue("O&",
	                     MediaObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackMedia(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Media _rv;
#ifndef GetTrackMedia
	PyMac_PRECHECK(GetTrackMedia);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackMedia(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     MediaObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_InsertMediaIntoTrack(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue trackStart;
	TimeValue mediaTime;
	TimeValue mediaDuration;
	Fixed mediaRate;
#ifndef InsertMediaIntoTrack
	PyMac_PRECHECK(InsertMediaIntoTrack);
#endif
	if (!PyArg_ParseTuple(_args, "lllO&",
	                      &trackStart,
	                      &mediaTime,
	                      &mediaDuration,
	                      PyMac_GetFixed, &mediaRate))
		return NULL;
	_err = InsertMediaIntoTrack(_self->ob_itself,
	                            trackStart,
	                            mediaTime,
	                            mediaDuration,
	                            mediaRate);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_InsertTrackSegment(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Track dstTrack;
	TimeValue srcIn;
	TimeValue srcDuration;
	TimeValue dstIn;
#ifndef InsertTrackSegment
	PyMac_PRECHECK(InsertTrackSegment);
#endif
	if (!PyArg_ParseTuple(_args, "O&lll",
	                      TrackObj_Convert, &dstTrack,
	                      &srcIn,
	                      &srcDuration,
	                      &dstIn))
		return NULL;
	_err = InsertTrackSegment(_self->ob_itself,
	                          dstTrack,
	                          srcIn,
	                          srcDuration,
	                          dstIn);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_InsertEmptyTrackSegment(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue dstIn;
	TimeValue dstDuration;
#ifndef InsertEmptyTrackSegment
	PyMac_PRECHECK(InsertEmptyTrackSegment);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &dstIn,
	                      &dstDuration))
		return NULL;
	_err = InsertEmptyTrackSegment(_self->ob_itself,
	                               dstIn,
	                               dstDuration);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_DeleteTrackSegment(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue startTime;
	TimeValue duration;
#ifndef DeleteTrackSegment
	PyMac_PRECHECK(DeleteTrackSegment);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &startTime,
	                      &duration))
		return NULL;
	_err = DeleteTrackSegment(_self->ob_itself,
	                          startTime,
	                          duration);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_ScaleTrackSegment(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue startTime;
	TimeValue oldDuration;
	TimeValue newDuration;
#ifndef ScaleTrackSegment
	PyMac_PRECHECK(ScaleTrackSegment);
#endif
	if (!PyArg_ParseTuple(_args, "lll",
	                      &startTime,
	                      &oldDuration,
	                      &newDuration))
		return NULL;
	_err = ScaleTrackSegment(_self->ob_itself,
	                         startTime,
	                         oldDuration,
	                         newDuration);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_IsScrapMovie(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Component _rv;
#ifndef IsScrapMovie
	PyMac_PRECHECK(IsScrapMovie);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsScrapMovie(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CmpObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_CopyTrackSettings(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Track dstTrack;
#ifndef CopyTrackSettings
	PyMac_PRECHECK(CopyTrackSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      TrackObj_Convert, &dstTrack))
		return NULL;
	_err = CopyTrackSettings(_self->ob_itself,
	                         dstTrack);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_AddEmptyTrackToMovie(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Movie dstMovie;
	Handle dataRef;
	OSType dataRefType;
	Track dstTrack;
#ifndef AddEmptyTrackToMovie
	PyMac_PRECHECK(AddEmptyTrackToMovie);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      MovieObj_Convert, &dstMovie,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_err = AddEmptyTrackToMovie(_self->ob_itself,
	                            dstMovie,
	                            dataRef,
	                            dataRefType,
	                            &dstTrack);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     TrackObj_New, dstTrack);
	return _res;
}

static PyObject *TrackObj_AddClonedTrackToMovie(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Movie dstMovie;
	long flags;
	Track dstTrack;
#ifndef AddClonedTrackToMovie
	PyMac_PRECHECK(AddClonedTrackToMovie);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      MovieObj_Convert, &dstMovie,
	                      &flags))
		return NULL;
	_err = AddClonedTrackToMovie(_self->ob_itself,
	                             dstMovie,
	                             flags,
	                             &dstTrack);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     TrackObj_New, dstTrack);
	return _res;
}

static PyObject *TrackObj_AddTrackReference(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Track refTrack;
	OSType refType;
	long addedIndex;
#ifndef AddTrackReference
	PyMac_PRECHECK(AddTrackReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      TrackObj_Convert, &refTrack,
	                      PyMac_GetOSType, &refType))
		return NULL;
	_err = AddTrackReference(_self->ob_itself,
	                         refTrack,
	                         refType,
	                         &addedIndex);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     addedIndex);
	return _res;
}

static PyObject *TrackObj_DeleteTrackReference(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType refType;
	long index;
#ifndef DeleteTrackReference
	PyMac_PRECHECK(DeleteTrackReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetOSType, &refType,
	                      &index))
		return NULL;
	_err = DeleteTrackReference(_self->ob_itself,
	                            refType,
	                            index);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_SetTrackReference(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Track refTrack;
	OSType refType;
	long index;
#ifndef SetTrackReference
	PyMac_PRECHECK(SetTrackReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      TrackObj_Convert, &refTrack,
	                      PyMac_GetOSType, &refType,
	                      &index))
		return NULL;
	_err = SetTrackReference(_self->ob_itself,
	                         refTrack,
	                         refType,
	                         index);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackReference(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Track _rv;
	OSType refType;
	long index;
#ifndef GetTrackReference
	PyMac_PRECHECK(GetTrackReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetOSType, &refType,
	                      &index))
		return NULL;
	_rv = GetTrackReference(_self->ob_itself,
	                        refType,
	                        index);
	_res = Py_BuildValue("O&",
	                     TrackObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_GetNextTrackReferenceType(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSType _rv;
	OSType refType;
#ifndef GetNextTrackReferenceType
	PyMac_PRECHECK(GetNextTrackReferenceType);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &refType))
		return NULL;
	_rv = GetNextTrackReferenceType(_self->ob_itself,
	                                refType);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildOSType, _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackReferenceCount(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	OSType refType;
#ifndef GetTrackReferenceCount
	PyMac_PRECHECK(GetTrackReferenceCount);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &refType))
		return NULL;
	_rv = GetTrackReferenceCount(_self->ob_itself,
	                             refType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackEditRate(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
	TimeValue atTime;
#ifndef GetTrackEditRate
	PyMac_PRECHECK(GetTrackEditRate);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &atTime))
		return NULL;
	_rv = GetTrackEditRate(_self->ob_itself,
	                       atTime);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackDataSize(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	TimeValue startTime;
	TimeValue duration;
#ifndef GetTrackDataSize
	PyMac_PRECHECK(GetTrackDataSize);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &startTime,
	                      &duration))
		return NULL;
	_rv = GetTrackDataSize(_self->ob_itself,
	                       startTime,
	                       duration);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackDataSize64(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue startTime;
	TimeValue duration;
	wide dataSize;
#ifndef GetTrackDataSize64
	PyMac_PRECHECK(GetTrackDataSize64);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &startTime,
	                      &duration))
		return NULL;
	_err = GetTrackDataSize64(_self->ob_itself,
	                          startTime,
	                          duration,
	                          &dataSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_Buildwide, dataSize);
	return _res;
}

static PyObject *TrackObj_PtInTrack(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point pt;
#ifndef PtInTrack
	PyMac_PRECHECK(PtInTrack);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &pt))
		return NULL;
	_rv = PtInTrack(_self->ob_itself,
	                pt);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_CopyTrackUserData(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Track dstTrack;
	OSType copyRule;
#ifndef CopyTrackUserData
	PyMac_PRECHECK(CopyTrackUserData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      TrackObj_Convert, &dstTrack,
	                      PyMac_GetOSType, &copyRule))
		return NULL;
	_err = CopyTrackUserData(_self->ob_itself,
	                         dstTrack,
	                         copyRule);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackNextInterestingTime(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short interestingTimeFlags;
	TimeValue time;
	Fixed rate;
	TimeValue interestingTime;
	TimeValue interestingDuration;
#ifndef GetTrackNextInterestingTime
	PyMac_PRECHECK(GetTrackNextInterestingTime);
#endif
	if (!PyArg_ParseTuple(_args, "hlO&",
	                      &interestingTimeFlags,
	                      &time,
	                      PyMac_GetFixed, &rate))
		return NULL;
	GetTrackNextInterestingTime(_self->ob_itself,
	                            interestingTimeFlags,
	                            time,
	                            rate,
	                            &interestingTime,
	                            &interestingDuration);
	_res = Py_BuildValue("ll",
	                     interestingTime,
	                     interestingDuration);
	return _res;
}

static PyObject *TrackObj_GetTrackSegmentDisplayBoundsRgn(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
	TimeValue time;
	TimeValue duration;
#ifndef GetTrackSegmentDisplayBoundsRgn
	PyMac_PRECHECK(GetTrackSegmentDisplayBoundsRgn);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &time,
	                      &duration))
		return NULL;
	_rv = GetTrackSegmentDisplayBoundsRgn(_self->ob_itself,
	                                      time,
	                                      duration);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *TrackObj_GetTrackStatus(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
#ifndef GetTrackStatus
	PyMac_PRECHECK(GetTrackStatus);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTrackStatus(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TrackObj_SetTrackLoadSettings(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue preloadTime;
	TimeValue preloadDuration;
	long preloadFlags;
	long defaultHints;
#ifndef SetTrackLoadSettings
	PyMac_PRECHECK(SetTrackLoadSettings);
#endif
	if (!PyArg_ParseTuple(_args, "llll",
	                      &preloadTime,
	                      &preloadDuration,
	                      &preloadFlags,
	                      &defaultHints))
		return NULL;
	SetTrackLoadSettings(_self->ob_itself,
	                     preloadTime,
	                     preloadDuration,
	                     preloadFlags,
	                     defaultHints);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TrackObj_GetTrackLoadSettings(TrackObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue preloadTime;
	TimeValue preloadDuration;
	long preloadFlags;
	long defaultHints;
#ifndef GetTrackLoadSettings
	PyMac_PRECHECK(GetTrackLoadSettings);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetTrackLoadSettings(_self->ob_itself,
	                     &preloadTime,
	                     &preloadDuration,
	                     &preloadFlags,
	                     &defaultHints);
	_res = Py_BuildValue("llll",
	                     preloadTime,
	                     preloadDuration,
	                     preloadFlags,
	                     defaultHints);
	return _res;
}

static PyMethodDef TrackObj_methods[] = {
	{"LoadTrackIntoRam", (PyCFunction)TrackObj_LoadTrackIntoRam, 1,
	 PyDoc_STR("(TimeValue time, TimeValue duration, long flags) -> None")},
	{"GetTrackPict", (PyCFunction)TrackObj_GetTrackPict, 1,
	 PyDoc_STR("(TimeValue time) -> (PicHandle _rv)")},
	{"GetTrackClipRgn", (PyCFunction)TrackObj_GetTrackClipRgn, 1,
	 PyDoc_STR("() -> (RgnHandle _rv)")},
	{"SetTrackClipRgn", (PyCFunction)TrackObj_SetTrackClipRgn, 1,
	 PyDoc_STR("(RgnHandle theClip) -> None")},
	{"GetTrackDisplayBoundsRgn", (PyCFunction)TrackObj_GetTrackDisplayBoundsRgn, 1,
	 PyDoc_STR("() -> (RgnHandle _rv)")},
	{"GetTrackMovieBoundsRgn", (PyCFunction)TrackObj_GetTrackMovieBoundsRgn, 1,
	 PyDoc_STR("() -> (RgnHandle _rv)")},
	{"GetTrackBoundsRgn", (PyCFunction)TrackObj_GetTrackBoundsRgn, 1,
	 PyDoc_STR("() -> (RgnHandle _rv)")},
	{"GetTrackMatte", (PyCFunction)TrackObj_GetTrackMatte, 1,
	 PyDoc_STR("() -> (PixMapHandle _rv)")},
	{"SetTrackMatte", (PyCFunction)TrackObj_SetTrackMatte, 1,
	 PyDoc_STR("(PixMapHandle theMatte) -> None")},
	{"GetTrackID", (PyCFunction)TrackObj_GetTrackID, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"GetTrackMovie", (PyCFunction)TrackObj_GetTrackMovie, 1,
	 PyDoc_STR("() -> (Movie _rv)")},
	{"GetTrackCreationTime", (PyCFunction)TrackObj_GetTrackCreationTime, 1,
	 PyDoc_STR("() -> (unsigned long _rv)")},
	{"GetTrackModificationTime", (PyCFunction)TrackObj_GetTrackModificationTime, 1,
	 PyDoc_STR("() -> (unsigned long _rv)")},
	{"GetTrackEnabled", (PyCFunction)TrackObj_GetTrackEnabled, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"SetTrackEnabled", (PyCFunction)TrackObj_SetTrackEnabled, 1,
	 PyDoc_STR("(Boolean isEnabled) -> None")},
	{"GetTrackUsage", (PyCFunction)TrackObj_GetTrackUsage, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"SetTrackUsage", (PyCFunction)TrackObj_SetTrackUsage, 1,
	 PyDoc_STR("(long usage) -> None")},
	{"GetTrackDuration", (PyCFunction)TrackObj_GetTrackDuration, 1,
	 PyDoc_STR("() -> (TimeValue _rv)")},
	{"GetTrackOffset", (PyCFunction)TrackObj_GetTrackOffset, 1,
	 PyDoc_STR("() -> (TimeValue _rv)")},
	{"SetTrackOffset", (PyCFunction)TrackObj_SetTrackOffset, 1,
	 PyDoc_STR("(TimeValue movieOffsetTime) -> None")},
	{"GetTrackLayer", (PyCFunction)TrackObj_GetTrackLayer, 1,
	 PyDoc_STR("() -> (short _rv)")},
	{"SetTrackLayer", (PyCFunction)TrackObj_SetTrackLayer, 1,
	 PyDoc_STR("(short layer) -> None")},
	{"GetTrackAlternate", (PyCFunction)TrackObj_GetTrackAlternate, 1,
	 PyDoc_STR("() -> (Track _rv)")},
	{"SetTrackAlternate", (PyCFunction)TrackObj_SetTrackAlternate, 1,
	 PyDoc_STR("(Track alternateT) -> None")},
	{"GetTrackVolume", (PyCFunction)TrackObj_GetTrackVolume, 1,
	 PyDoc_STR("() -> (short _rv)")},
	{"SetTrackVolume", (PyCFunction)TrackObj_SetTrackVolume, 1,
	 PyDoc_STR("(short volume) -> None")},
	{"GetTrackDimensions", (PyCFunction)TrackObj_GetTrackDimensions, 1,
	 PyDoc_STR("() -> (Fixed width, Fixed height)")},
	{"SetTrackDimensions", (PyCFunction)TrackObj_SetTrackDimensions, 1,
	 PyDoc_STR("(Fixed width, Fixed height) -> None")},
	{"GetTrackUserData", (PyCFunction)TrackObj_GetTrackUserData, 1,
	 PyDoc_STR("() -> (UserData _rv)")},
	{"GetTrackSoundLocalizationSettings", (PyCFunction)TrackObj_GetTrackSoundLocalizationSettings, 1,
	 PyDoc_STR("() -> (Handle settings)")},
	{"SetTrackSoundLocalizationSettings", (PyCFunction)TrackObj_SetTrackSoundLocalizationSettings, 1,
	 PyDoc_STR("(Handle settings) -> None")},
	{"NewTrackMedia", (PyCFunction)TrackObj_NewTrackMedia, 1,
	 PyDoc_STR("(OSType mediaType, TimeScale timeScale, Handle dataRef, OSType dataRefType) -> (Media _rv)")},
	{"GetTrackMedia", (PyCFunction)TrackObj_GetTrackMedia, 1,
	 PyDoc_STR("() -> (Media _rv)")},
	{"InsertMediaIntoTrack", (PyCFunction)TrackObj_InsertMediaIntoTrack, 1,
	 PyDoc_STR("(TimeValue trackStart, TimeValue mediaTime, TimeValue mediaDuration, Fixed mediaRate) -> None")},
	{"InsertTrackSegment", (PyCFunction)TrackObj_InsertTrackSegment, 1,
	 PyDoc_STR("(Track dstTrack, TimeValue srcIn, TimeValue srcDuration, TimeValue dstIn) -> None")},
	{"InsertEmptyTrackSegment", (PyCFunction)TrackObj_InsertEmptyTrackSegment, 1,
	 PyDoc_STR("(TimeValue dstIn, TimeValue dstDuration) -> None")},
	{"DeleteTrackSegment", (PyCFunction)TrackObj_DeleteTrackSegment, 1,
	 PyDoc_STR("(TimeValue startTime, TimeValue duration) -> None")},
	{"ScaleTrackSegment", (PyCFunction)TrackObj_ScaleTrackSegment, 1,
	 PyDoc_STR("(TimeValue startTime, TimeValue oldDuration, TimeValue newDuration) -> None")},
	{"IsScrapMovie", (PyCFunction)TrackObj_IsScrapMovie, 1,
	 PyDoc_STR("() -> (Component _rv)")},
	{"CopyTrackSettings", (PyCFunction)TrackObj_CopyTrackSettings, 1,
	 PyDoc_STR("(Track dstTrack) -> None")},
	{"AddEmptyTrackToMovie", (PyCFunction)TrackObj_AddEmptyTrackToMovie, 1,
	 PyDoc_STR("(Movie dstMovie, Handle dataRef, OSType dataRefType) -> (Track dstTrack)")},
	{"AddClonedTrackToMovie", (PyCFunction)TrackObj_AddClonedTrackToMovie, 1,
	 PyDoc_STR("(Movie dstMovie, long flags) -> (Track dstTrack)")},
	{"AddTrackReference", (PyCFunction)TrackObj_AddTrackReference, 1,
	 PyDoc_STR("(Track refTrack, OSType refType) -> (long addedIndex)")},
	{"DeleteTrackReference", (PyCFunction)TrackObj_DeleteTrackReference, 1,
	 PyDoc_STR("(OSType refType, long index) -> None")},
	{"SetTrackReference", (PyCFunction)TrackObj_SetTrackReference, 1,
	 PyDoc_STR("(Track refTrack, OSType refType, long index) -> None")},
	{"GetTrackReference", (PyCFunction)TrackObj_GetTrackReference, 1,
	 PyDoc_STR("(OSType refType, long index) -> (Track _rv)")},
	{"GetNextTrackReferenceType", (PyCFunction)TrackObj_GetNextTrackReferenceType, 1,
	 PyDoc_STR("(OSType refType) -> (OSType _rv)")},
	{"GetTrackReferenceCount", (PyCFunction)TrackObj_GetTrackReferenceCount, 1,
	 PyDoc_STR("(OSType refType) -> (long _rv)")},
	{"GetTrackEditRate", (PyCFunction)TrackObj_GetTrackEditRate, 1,
	 PyDoc_STR("(TimeValue atTime) -> (Fixed _rv)")},
	{"GetTrackDataSize", (PyCFunction)TrackObj_GetTrackDataSize, 1,
	 PyDoc_STR("(TimeValue startTime, TimeValue duration) -> (long _rv)")},
	{"GetTrackDataSize64", (PyCFunction)TrackObj_GetTrackDataSize64, 1,
	 PyDoc_STR("(TimeValue startTime, TimeValue duration) -> (wide dataSize)")},
	{"PtInTrack", (PyCFunction)TrackObj_PtInTrack, 1,
	 PyDoc_STR("(Point pt) -> (Boolean _rv)")},
	{"CopyTrackUserData", (PyCFunction)TrackObj_CopyTrackUserData, 1,
	 PyDoc_STR("(Track dstTrack, OSType copyRule) -> None")},
	{"GetTrackNextInterestingTime", (PyCFunction)TrackObj_GetTrackNextInterestingTime, 1,
	 PyDoc_STR("(short interestingTimeFlags, TimeValue time, Fixed rate) -> (TimeValue interestingTime, TimeValue interestingDuration)")},
	{"GetTrackSegmentDisplayBoundsRgn", (PyCFunction)TrackObj_GetTrackSegmentDisplayBoundsRgn, 1,
	 PyDoc_STR("(TimeValue time, TimeValue duration) -> (RgnHandle _rv)")},
	{"GetTrackStatus", (PyCFunction)TrackObj_GetTrackStatus, 1,
	 PyDoc_STR("() -> (ComponentResult _rv)")},
	{"SetTrackLoadSettings", (PyCFunction)TrackObj_SetTrackLoadSettings, 1,
	 PyDoc_STR("(TimeValue preloadTime, TimeValue preloadDuration, long preloadFlags, long defaultHints) -> None")},
	{"GetTrackLoadSettings", (PyCFunction)TrackObj_GetTrackLoadSettings, 1,
	 PyDoc_STR("() -> (TimeValue preloadTime, TimeValue preloadDuration, long preloadFlags, long defaultHints)")},
	{NULL, NULL, 0}
};

#define TrackObj_getsetlist NULL


#define TrackObj_compare NULL

#define TrackObj_repr NULL

#define TrackObj_hash NULL
#define TrackObj_tp_init 0

#define TrackObj_tp_alloc PyType_GenericAlloc

static PyObject *TrackObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	Track itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, TrackObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((TrackObject *)_self)->ob_itself = itself;
	return _self;
}

#define TrackObj_tp_free PyObject_Del


PyTypeObject Track_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.Track", /*tp_name*/
	sizeof(TrackObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) TrackObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) TrackObj_compare, /*tp_compare*/
	(reprfunc) TrackObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) TrackObj_hash, /*tp_hash*/
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
	TrackObj_methods, /* tp_methods */
	0, /*tp_members*/
	TrackObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	TrackObj_tp_init, /* tp_init */
	TrackObj_tp_alloc, /* tp_alloc */
	TrackObj_tp_new, /* tp_new */
	TrackObj_tp_free, /* tp_free */
};

/* --------------------- End object type Track ---------------------- */


/* ----------------------- Object type Movie ------------------------ */

PyTypeObject Movie_Type;

#define MovieObj_Check(x) ((x)->ob_type == &Movie_Type || PyObject_TypeCheck((x), &Movie_Type))

typedef struct MovieObject {
	PyObject_HEAD
	Movie ob_itself;
} MovieObject;

PyObject *MovieObj_New(Movie itself)
{
	MovieObject *it;
	if (itself == NULL) {
	                                PyErr_SetString(Qt_Error,"Cannot create Movie from NULL pointer");
	                                return NULL;
	                        }
	it = PyObject_NEW(MovieObject, &Movie_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}

int MovieObj_Convert(PyObject *v, Movie *p_itself)
{
	if (v == Py_None)
	{
		*p_itself = NULL;
		return 1;
	}
	if (!MovieObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Movie required");
		return 0;
	}
	*p_itself = ((MovieObject *)v)->ob_itself;
	return 1;
}

static void MovieObj_dealloc(MovieObject *self)
{
	if (self->ob_itself) DisposeMovie(self->ob_itself);
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *MovieObj_MoviesTask(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long maxMilliSecToUse;
#ifndef MoviesTask
	PyMac_PRECHECK(MoviesTask);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &maxMilliSecToUse))
		return NULL;
	MoviesTask(_self->ob_itself,
	           maxMilliSecToUse);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_PrerollMovie(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue time;
	Fixed Rate;
#ifndef PrerollMovie
	PyMac_PRECHECK(PrerollMovie);
#endif
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &time,
	                      PyMac_GetFixed, &Rate))
		return NULL;
	_err = PrerollMovie(_self->ob_itself,
	                    time,
	                    Rate);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_AbortPrePrerollMovie(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr err;
#ifndef AbortPrePrerollMovie
	PyMac_PRECHECK(AbortPrePrerollMovie);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &err))
		return NULL;
	AbortPrePrerollMovie(_self->ob_itself,
	                     err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_LoadMovieIntoRam(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue time;
	TimeValue duration;
	long flags;
#ifndef LoadMovieIntoRam
	PyMac_PRECHECK(LoadMovieIntoRam);
#endif
	if (!PyArg_ParseTuple(_args, "lll",
	                      &time,
	                      &duration,
	                      &flags))
		return NULL;
	_err = LoadMovieIntoRam(_self->ob_itself,
	                        time,
	                        duration,
	                        flags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_SetMovieActive(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean active;
#ifndef SetMovieActive
	PyMac_PRECHECK(SetMovieActive);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &active))
		return NULL;
	SetMovieActive(_self->ob_itself,
	               active);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieActive(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef GetMovieActive
	PyMac_PRECHECK(GetMovieActive);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieActive(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_StartMovie(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef StartMovie
	PyMac_PRECHECK(StartMovie);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	StartMovie(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_StopMovie(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef StopMovie
	PyMac_PRECHECK(StopMovie);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	StopMovie(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GoToBeginningOfMovie(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef GoToBeginningOfMovie
	PyMac_PRECHECK(GoToBeginningOfMovie);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GoToBeginningOfMovie(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GoToEndOfMovie(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef GoToEndOfMovie
	PyMac_PRECHECK(GoToEndOfMovie);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GoToEndOfMovie(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_IsMovieDone(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef IsMovieDone
	PyMac_PRECHECK(IsMovieDone);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsMovieDone(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_GetMoviePreviewMode(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef GetMoviePreviewMode
	PyMac_PRECHECK(GetMoviePreviewMode);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMoviePreviewMode(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_SetMoviePreviewMode(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean usePreview;
#ifndef SetMoviePreviewMode
	PyMac_PRECHECK(SetMoviePreviewMode);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &usePreview))
		return NULL;
	SetMoviePreviewMode(_self->ob_itself,
	                    usePreview);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_ShowMoviePoster(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef ShowMoviePoster
	PyMac_PRECHECK(ShowMoviePoster);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ShowMoviePoster(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieTimeBase(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeBase _rv;
#ifndef GetMovieTimeBase
	PyMac_PRECHECK(GetMovieTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieTimeBase(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     TimeBaseObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_SetMovieMasterTimeBase(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeBase tb;
	TimeRecord slaveZero;
#ifndef SetMovieMasterTimeBase
	PyMac_PRECHECK(SetMovieMasterTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      TimeBaseObj_Convert, &tb,
	                      QtTimeRecord_Convert, &slaveZero))
		return NULL;
	SetMovieMasterTimeBase(_self->ob_itself,
	                       tb,
	                       &slaveZero);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_SetMovieMasterClock(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Component clockMeister;
	TimeRecord slaveZero;
#ifndef SetMovieMasterClock
	PyMac_PRECHECK(SetMovieMasterClock);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpObj_Convert, &clockMeister,
	                      QtTimeRecord_Convert, &slaveZero))
		return NULL;
	SetMovieMasterClock(_self->ob_itself,
	                    clockMeister,
	                    &slaveZero);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_ChooseMovieClock(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long flags;
#ifndef ChooseMovieClock
	PyMac_PRECHECK(ChooseMovieClock);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &flags))
		return NULL;
	ChooseMovieClock(_self->ob_itself,
	                 flags);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieGWorld(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGrafPtr port;
	GDHandle gdh;
#ifndef GetMovieGWorld
	PyMac_PRECHECK(GetMovieGWorld);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetMovieGWorld(_self->ob_itself,
	               &port,
	               &gdh);
	_res = Py_BuildValue("O&O&",
	                     GrafObj_New, port,
	                     OptResObj_New, gdh);
	return _res;
}

static PyObject *MovieObj_SetMovieGWorld(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGrafPtr port;
	GDHandle gdh;
#ifndef SetMovieGWorld
	PyMac_PRECHECK(SetMovieGWorld);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      GrafObj_Convert, &port,
	                      OptResObj_Convert, &gdh))
		return NULL;
	SetMovieGWorld(_self->ob_itself,
	               port,
	               gdh);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieNaturalBoundsRect(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect naturalBounds;
#ifndef GetMovieNaturalBoundsRect
	PyMac_PRECHECK(GetMovieNaturalBoundsRect);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetMovieNaturalBoundsRect(_self->ob_itself,
	                          &naturalBounds);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &naturalBounds);
	return _res;
}

static PyObject *MovieObj_GetNextTrackForCompositing(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Track _rv;
	Track theTrack;
#ifndef GetNextTrackForCompositing
	PyMac_PRECHECK(GetNextTrackForCompositing);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      TrackObj_Convert, &theTrack))
		return NULL;
	_rv = GetNextTrackForCompositing(_self->ob_itself,
	                                 theTrack);
	_res = Py_BuildValue("O&",
	                     TrackObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_GetPrevTrackForCompositing(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Track _rv;
	Track theTrack;
#ifndef GetPrevTrackForCompositing
	PyMac_PRECHECK(GetPrevTrackForCompositing);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      TrackObj_Convert, &theTrack))
		return NULL;
	_rv = GetPrevTrackForCompositing(_self->ob_itself,
	                                 theTrack);
	_res = Py_BuildValue("O&",
	                     TrackObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_GetMoviePict(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PicHandle _rv;
	TimeValue time;
#ifndef GetMoviePict
	PyMac_PRECHECK(GetMoviePict);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &time))
		return NULL;
	_rv = GetMoviePict(_self->ob_itself,
	                   time);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_GetMoviePosterPict(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PicHandle _rv;
#ifndef GetMoviePosterPict
	PyMac_PRECHECK(GetMoviePosterPict);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMoviePosterPict(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_UpdateMovie(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef UpdateMovie
	PyMac_PRECHECK(UpdateMovie);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = UpdateMovie(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_InvalidateMovieRegion(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	RgnHandle invalidRgn;
#ifndef InvalidateMovieRegion
	PyMac_PRECHECK(InvalidateMovieRegion);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &invalidRgn))
		return NULL;
	_err = InvalidateMovieRegion(_self->ob_itself,
	                             invalidRgn);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieBox(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect boxRect;
#ifndef GetMovieBox
	PyMac_PRECHECK(GetMovieBox);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetMovieBox(_self->ob_itself,
	            &boxRect);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &boxRect);
	return _res;
}

static PyObject *MovieObj_SetMovieBox(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect boxRect;
#ifndef SetMovieBox
	PyMac_PRECHECK(SetMovieBox);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &boxRect))
		return NULL;
	SetMovieBox(_self->ob_itself,
	            &boxRect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieDisplayClipRgn(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
#ifndef GetMovieDisplayClipRgn
	PyMac_PRECHECK(GetMovieDisplayClipRgn);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieDisplayClipRgn(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_SetMovieDisplayClipRgn(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle theClip;
#ifndef SetMovieDisplayClipRgn
	PyMac_PRECHECK(SetMovieDisplayClipRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theClip))
		return NULL;
	SetMovieDisplayClipRgn(_self->ob_itself,
	                       theClip);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieClipRgn(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
#ifndef GetMovieClipRgn
	PyMac_PRECHECK(GetMovieClipRgn);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieClipRgn(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_SetMovieClipRgn(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle theClip;
#ifndef SetMovieClipRgn
	PyMac_PRECHECK(SetMovieClipRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theClip))
		return NULL;
	SetMovieClipRgn(_self->ob_itself,
	                theClip);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieDisplayBoundsRgn(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
#ifndef GetMovieDisplayBoundsRgn
	PyMac_PRECHECK(GetMovieDisplayBoundsRgn);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieDisplayBoundsRgn(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_GetMovieBoundsRgn(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
#ifndef GetMovieBoundsRgn
	PyMac_PRECHECK(GetMovieBoundsRgn);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieBoundsRgn(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_SetMovieVideoOutput(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentInstance vout;
#ifndef SetMovieVideoOutput
	PyMac_PRECHECK(SetMovieVideoOutput);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &vout))
		return NULL;
	SetMovieVideoOutput(_self->ob_itself,
	                    vout);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_PutMovieIntoHandle(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle publicMovie;
#ifndef PutMovieIntoHandle
	PyMac_PRECHECK(PutMovieIntoHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &publicMovie))
		return NULL;
	_err = PutMovieIntoHandle(_self->ob_itself,
	                          publicMovie);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_PutMovieIntoDataFork(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short fRefNum;
	long offset;
	long maxSize;
#ifndef PutMovieIntoDataFork
	PyMac_PRECHECK(PutMovieIntoDataFork);
#endif
	if (!PyArg_ParseTuple(_args, "hll",
	                      &fRefNum,
	                      &offset,
	                      &maxSize))
		return NULL;
	_err = PutMovieIntoDataFork(_self->ob_itself,
	                            fRefNum,
	                            offset,
	                            maxSize);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_PutMovieIntoDataFork64(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long fRefNum;
	wide offset;
	unsigned long maxSize;
#ifndef PutMovieIntoDataFork64
	PyMac_PRECHECK(PutMovieIntoDataFork64);
#endif
	if (!PyArg_ParseTuple(_args, "lO&l",
	                      &fRefNum,
	                      PyMac_Getwide, &offset,
	                      &maxSize))
		return NULL;
	_err = PutMovieIntoDataFork64(_self->ob_itself,
	                              fRefNum,
	                              &offset,
	                              maxSize);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_PutMovieIntoStorage(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DataHandler dh;
	wide offset;
	unsigned long maxSize;
#ifndef PutMovieIntoStorage
	PyMac_PRECHECK(PutMovieIntoStorage);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      CmpInstObj_Convert, &dh,
	                      PyMac_Getwide, &offset,
	                      &maxSize))
		return NULL;
	_err = PutMovieIntoStorage(_self->ob_itself,
	                           dh,
	                           &offset,
	                           maxSize);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_PutMovieForDataRefIntoHandle(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataRef;
	OSType dataRefType;
	Handle publicMovie;
#ifndef PutMovieForDataRefIntoHandle
	PyMac_PRECHECK(PutMovieForDataRefIntoHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      ResObj_Convert, &publicMovie))
		return NULL;
	_err = PutMovieForDataRefIntoHandle(_self->ob_itself,
	                                    dataRef,
	                                    dataRefType,
	                                    publicMovie);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieCreationTime(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	unsigned long _rv;
#ifndef GetMovieCreationTime
	PyMac_PRECHECK(GetMovieCreationTime);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieCreationTime(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_GetMovieModificationTime(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	unsigned long _rv;
#ifndef GetMovieModificationTime
	PyMac_PRECHECK(GetMovieModificationTime);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieModificationTime(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_GetMovieTimeScale(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeScale _rv;
#ifndef GetMovieTimeScale
	PyMac_PRECHECK(GetMovieTimeScale);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieTimeScale(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_SetMovieTimeScale(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeScale timeScale;
#ifndef SetMovieTimeScale
	PyMac_PRECHECK(SetMovieTimeScale);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &timeScale))
		return NULL;
	SetMovieTimeScale(_self->ob_itself,
	                  timeScale);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieDuration(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue _rv;
#ifndef GetMovieDuration
	PyMac_PRECHECK(GetMovieDuration);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieDuration(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_GetMovieRate(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
#ifndef GetMovieRate
	PyMac_PRECHECK(GetMovieRate);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieRate(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyObject *MovieObj_SetMovieRate(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed rate;
#ifndef SetMovieRate
	PyMac_PRECHECK(SetMovieRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFixed, &rate))
		return NULL;
	SetMovieRate(_self->ob_itself,
	             rate);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMoviePreferredRate(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
#ifndef GetMoviePreferredRate
	PyMac_PRECHECK(GetMoviePreferredRate);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMoviePreferredRate(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyObject *MovieObj_SetMoviePreferredRate(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed rate;
#ifndef SetMoviePreferredRate
	PyMac_PRECHECK(SetMoviePreferredRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFixed, &rate))
		return NULL;
	SetMoviePreferredRate(_self->ob_itself,
	                      rate);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMoviePreferredVolume(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
#ifndef GetMoviePreferredVolume
	PyMac_PRECHECK(GetMoviePreferredVolume);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMoviePreferredVolume(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_SetMoviePreferredVolume(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short volume;
#ifndef SetMoviePreferredVolume
	PyMac_PRECHECK(SetMoviePreferredVolume);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &volume))
		return NULL;
	SetMoviePreferredVolume(_self->ob_itself,
	                        volume);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieVolume(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
#ifndef GetMovieVolume
	PyMac_PRECHECK(GetMovieVolume);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieVolume(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_SetMovieVolume(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short volume;
#ifndef SetMovieVolume
	PyMac_PRECHECK(SetMovieVolume);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &volume))
		return NULL;
	SetMovieVolume(_self->ob_itself,
	               volume);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMoviePreviewTime(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue previewTime;
	TimeValue previewDuration;
#ifndef GetMoviePreviewTime
	PyMac_PRECHECK(GetMoviePreviewTime);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetMoviePreviewTime(_self->ob_itself,
	                    &previewTime,
	                    &previewDuration);
	_res = Py_BuildValue("ll",
	                     previewTime,
	                     previewDuration);
	return _res;
}

static PyObject *MovieObj_SetMoviePreviewTime(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue previewTime;
	TimeValue previewDuration;
#ifndef SetMoviePreviewTime
	PyMac_PRECHECK(SetMoviePreviewTime);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &previewTime,
	                      &previewDuration))
		return NULL;
	SetMoviePreviewTime(_self->ob_itself,
	                    previewTime,
	                    previewDuration);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMoviePosterTime(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue _rv;
#ifndef GetMoviePosterTime
	PyMac_PRECHECK(GetMoviePosterTime);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMoviePosterTime(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_SetMoviePosterTime(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue posterTime;
#ifndef SetMoviePosterTime
	PyMac_PRECHECK(SetMoviePosterTime);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &posterTime))
		return NULL;
	SetMoviePosterTime(_self->ob_itself,
	                   posterTime);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieSelection(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue selectionTime;
	TimeValue selectionDuration;
#ifndef GetMovieSelection
	PyMac_PRECHECK(GetMovieSelection);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetMovieSelection(_self->ob_itself,
	                  &selectionTime,
	                  &selectionDuration);
	_res = Py_BuildValue("ll",
	                     selectionTime,
	                     selectionDuration);
	return _res;
}

static PyObject *MovieObj_SetMovieSelection(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue selectionTime;
	TimeValue selectionDuration;
#ifndef SetMovieSelection
	PyMac_PRECHECK(SetMovieSelection);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &selectionTime,
	                      &selectionDuration))
		return NULL;
	SetMovieSelection(_self->ob_itself,
	                  selectionTime,
	                  selectionDuration);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_SetMovieActiveSegment(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue startTime;
	TimeValue duration;
#ifndef SetMovieActiveSegment
	PyMac_PRECHECK(SetMovieActiveSegment);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &startTime,
	                      &duration))
		return NULL;
	SetMovieActiveSegment(_self->ob_itself,
	                      startTime,
	                      duration);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieActiveSegment(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue startTime;
	TimeValue duration;
#ifndef GetMovieActiveSegment
	PyMac_PRECHECK(GetMovieActiveSegment);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetMovieActiveSegment(_self->ob_itself,
	                      &startTime,
	                      &duration);
	_res = Py_BuildValue("ll",
	                     startTime,
	                     duration);
	return _res;
}

static PyObject *MovieObj_GetMovieTime(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue _rv;
	TimeRecord currentTime;
#ifndef GetMovieTime
	PyMac_PRECHECK(GetMovieTime);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieTime(_self->ob_itself,
	                   &currentTime);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     QtTimeRecord_New, &currentTime);
	return _res;
}

static PyObject *MovieObj_SetMovieTime(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeRecord newtime;
#ifndef SetMovieTime
	PyMac_PRECHECK(SetMovieTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      QtTimeRecord_Convert, &newtime))
		return NULL;
	SetMovieTime(_self->ob_itself,
	             &newtime);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_SetMovieTimeValue(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue newtime;
#ifndef SetMovieTimeValue
	PyMac_PRECHECK(SetMovieTimeValue);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &newtime))
		return NULL;
	SetMovieTimeValue(_self->ob_itself,
	                  newtime);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieUserData(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UserData _rv;
#ifndef GetMovieUserData
	PyMac_PRECHECK(GetMovieUserData);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieUserData(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     UserDataObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_GetMovieTrackCount(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef GetMovieTrackCount
	PyMac_PRECHECK(GetMovieTrackCount);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieTrackCount(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_GetMovieTrack(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Track _rv;
	long trackID;
#ifndef GetMovieTrack
	PyMac_PRECHECK(GetMovieTrack);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &trackID))
		return NULL;
	_rv = GetMovieTrack(_self->ob_itself,
	                    trackID);
	_res = Py_BuildValue("O&",
	                     TrackObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_GetMovieIndTrack(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Track _rv;
	long index;
#ifndef GetMovieIndTrack
	PyMac_PRECHECK(GetMovieIndTrack);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &index))
		return NULL;
	_rv = GetMovieIndTrack(_self->ob_itself,
	                       index);
	_res = Py_BuildValue("O&",
	                     TrackObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_GetMovieIndTrackType(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Track _rv;
	long index;
	OSType trackType;
	long flags;
#ifndef GetMovieIndTrackType
	PyMac_PRECHECK(GetMovieIndTrackType);
#endif
	if (!PyArg_ParseTuple(_args, "lO&l",
	                      &index,
	                      PyMac_GetOSType, &trackType,
	                      &flags))
		return NULL;
	_rv = GetMovieIndTrackType(_self->ob_itself,
	                           index,
	                           trackType,
	                           flags);
	_res = Py_BuildValue("O&",
	                     TrackObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_NewMovieTrack(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Track _rv;
	Fixed width;
	Fixed height;
	short trackVolume;
#ifndef NewMovieTrack
	PyMac_PRECHECK(NewMovieTrack);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      PyMac_GetFixed, &width,
	                      PyMac_GetFixed, &height,
	                      &trackVolume))
		return NULL;
	_rv = NewMovieTrack(_self->ob_itself,
	                    width,
	                    height,
	                    trackVolume);
	_res = Py_BuildValue("O&",
	                     TrackObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_SetAutoTrackAlternatesEnabled(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean enable;
#ifndef SetAutoTrackAlternatesEnabled
	PyMac_PRECHECK(SetAutoTrackAlternatesEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &enable))
		return NULL;
	SetAutoTrackAlternatesEnabled(_self->ob_itself,
	                              enable);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_SelectMovieAlternates(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef SelectMovieAlternates
	PyMac_PRECHECK(SelectMovieAlternates);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	SelectMovieAlternates(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_InsertMovieSegment(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Movie dstMovie;
	TimeValue srcIn;
	TimeValue srcDuration;
	TimeValue dstIn;
#ifndef InsertMovieSegment
	PyMac_PRECHECK(InsertMovieSegment);
#endif
	if (!PyArg_ParseTuple(_args, "O&lll",
	                      MovieObj_Convert, &dstMovie,
	                      &srcIn,
	                      &srcDuration,
	                      &dstIn))
		return NULL;
	_err = InsertMovieSegment(_self->ob_itself,
	                          dstMovie,
	                          srcIn,
	                          srcDuration,
	                          dstIn);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_InsertEmptyMovieSegment(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue dstIn;
	TimeValue dstDuration;
#ifndef InsertEmptyMovieSegment
	PyMac_PRECHECK(InsertEmptyMovieSegment);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &dstIn,
	                      &dstDuration))
		return NULL;
	_err = InsertEmptyMovieSegment(_self->ob_itself,
	                               dstIn,
	                               dstDuration);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_DeleteMovieSegment(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue startTime;
	TimeValue duration;
#ifndef DeleteMovieSegment
	PyMac_PRECHECK(DeleteMovieSegment);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &startTime,
	                      &duration))
		return NULL;
	_err = DeleteMovieSegment(_self->ob_itself,
	                          startTime,
	                          duration);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_ScaleMovieSegment(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue startTime;
	TimeValue oldDuration;
	TimeValue newDuration;
#ifndef ScaleMovieSegment
	PyMac_PRECHECK(ScaleMovieSegment);
#endif
	if (!PyArg_ParseTuple(_args, "lll",
	                      &startTime,
	                      &oldDuration,
	                      &newDuration))
		return NULL;
	_err = ScaleMovieSegment(_self->ob_itself,
	                         startTime,
	                         oldDuration,
	                         newDuration);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_CutMovieSelection(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie _rv;
#ifndef CutMovieSelection
	PyMac_PRECHECK(CutMovieSelection);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CutMovieSelection(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     MovieObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_CopyMovieSelection(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie _rv;
#ifndef CopyMovieSelection
	PyMac_PRECHECK(CopyMovieSelection);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CopyMovieSelection(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     MovieObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_PasteMovieSelection(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie src;
#ifndef PasteMovieSelection
	PyMac_PRECHECK(PasteMovieSelection);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      MovieObj_Convert, &src))
		return NULL;
	PasteMovieSelection(_self->ob_itself,
	                    src);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_AddMovieSelection(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie src;
#ifndef AddMovieSelection
	PyMac_PRECHECK(AddMovieSelection);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      MovieObj_Convert, &src))
		return NULL;
	AddMovieSelection(_self->ob_itself,
	                  src);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_ClearMovieSelection(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef ClearMovieSelection
	PyMac_PRECHECK(ClearMovieSelection);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ClearMovieSelection(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_PutMovieIntoTypedHandle(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Track targetTrack;
	OSType handleType;
	Handle publicMovie;
	TimeValue start;
	TimeValue dur;
	long flags;
	ComponentInstance userComp;
#ifndef PutMovieIntoTypedHandle
	PyMac_PRECHECK(PutMovieIntoTypedHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&lllO&",
	                      TrackObj_Convert, &targetTrack,
	                      PyMac_GetOSType, &handleType,
	                      ResObj_Convert, &publicMovie,
	                      &start,
	                      &dur,
	                      &flags,
	                      CmpInstObj_Convert, &userComp))
		return NULL;
	_err = PutMovieIntoTypedHandle(_self->ob_itself,
	                               targetTrack,
	                               handleType,
	                               publicMovie,
	                               start,
	                               dur,
	                               flags,
	                               userComp);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_CopyMovieSettings(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Movie dstMovie;
#ifndef CopyMovieSettings
	PyMac_PRECHECK(CopyMovieSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      MovieObj_Convert, &dstMovie))
		return NULL;
	_err = CopyMovieSettings(_self->ob_itself,
	                         dstMovie);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_ConvertMovieToFile(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Track onlyTrack;
	FSSpec outputFile;
	OSType fileType;
	OSType creator;
	ScriptCode scriptTag;
	short resID;
	long flags;
	ComponentInstance userComp;
#ifndef ConvertMovieToFile
	PyMac_PRECHECK(ConvertMovieToFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&hlO&",
	                      TrackObj_Convert, &onlyTrack,
	                      PyMac_GetFSSpec, &outputFile,
	                      PyMac_GetOSType, &fileType,
	                      PyMac_GetOSType, &creator,
	                      &scriptTag,
	                      &flags,
	                      CmpInstObj_Convert, &userComp))
		return NULL;
	_err = ConvertMovieToFile(_self->ob_itself,
	                          onlyTrack,
	                          &outputFile,
	                          fileType,
	                          creator,
	                          scriptTag,
	                          &resID,
	                          flags,
	                          userComp);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     resID);
	return _res;
}

static PyObject *MovieObj_GetMovieDataSize(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	TimeValue startTime;
	TimeValue duration;
#ifndef GetMovieDataSize
	PyMac_PRECHECK(GetMovieDataSize);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &startTime,
	                      &duration))
		return NULL;
	_rv = GetMovieDataSize(_self->ob_itself,
	                       startTime,
	                       duration);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_GetMovieDataSize64(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue startTime;
	TimeValue duration;
	wide dataSize;
#ifndef GetMovieDataSize64
	PyMac_PRECHECK(GetMovieDataSize64);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &startTime,
	                      &duration))
		return NULL;
	_err = GetMovieDataSize64(_self->ob_itself,
	                          startTime,
	                          duration,
	                          &dataSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_Buildwide, dataSize);
	return _res;
}

static PyObject *MovieObj_PtInMovie(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point pt;
#ifndef PtInMovie
	PyMac_PRECHECK(PtInMovie);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &pt))
		return NULL;
	_rv = PtInMovie(_self->ob_itself,
	                pt);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_SetMovieLanguage(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long language;
#ifndef SetMovieLanguage
	PyMac_PRECHECK(SetMovieLanguage);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &language))
		return NULL;
	SetMovieLanguage(_self->ob_itself,
	                 language);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_CopyMovieUserData(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Movie dstMovie;
	OSType copyRule;
#ifndef CopyMovieUserData
	PyMac_PRECHECK(CopyMovieUserData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      MovieObj_Convert, &dstMovie,
	                      PyMac_GetOSType, &copyRule))
		return NULL;
	_err = CopyMovieUserData(_self->ob_itself,
	                         dstMovie,
	                         copyRule);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieNextInterestingTime(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short interestingTimeFlags;
	short numMediaTypes;
	OSType whichMediaTypes;
	TimeValue time;
	Fixed rate;
	TimeValue interestingTime;
	TimeValue interestingDuration;
#ifndef GetMovieNextInterestingTime
	PyMac_PRECHECK(GetMovieNextInterestingTime);
#endif
	if (!PyArg_ParseTuple(_args, "hhO&lO&",
	                      &interestingTimeFlags,
	                      &numMediaTypes,
	                      PyMac_GetOSType, &whichMediaTypes,
	                      &time,
	                      PyMac_GetFixed, &rate))
		return NULL;
	GetMovieNextInterestingTime(_self->ob_itself,
	                            interestingTimeFlags,
	                            numMediaTypes,
	                            &whichMediaTypes,
	                            time,
	                            rate,
	                            &interestingTime,
	                            &interestingDuration);
	_res = Py_BuildValue("ll",
	                     interestingTime,
	                     interestingDuration);
	return _res;
}

static PyObject *MovieObj_AddMovieResource(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short resRefNum;
	short resId;
	Str255 resName;
#ifndef AddMovieResource
	PyMac_PRECHECK(AddMovieResource);
#endif
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &resRefNum,
	                      PyMac_GetStr255, resName))
		return NULL;
	_err = AddMovieResource(_self->ob_itself,
	                        resRefNum,
	                        &resId,
	                        resName);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     resId);
	return _res;
}

static PyObject *MovieObj_UpdateMovieResource(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short resRefNum;
	short resId;
	Str255 resName;
#ifndef UpdateMovieResource
	PyMac_PRECHECK(UpdateMovieResource);
#endif
	if (!PyArg_ParseTuple(_args, "hhO&",
	                      &resRefNum,
	                      &resId,
	                      PyMac_GetStr255, resName))
		return NULL;
	_err = UpdateMovieResource(_self->ob_itself,
	                           resRefNum,
	                           resId,
	                           resName);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_AddMovieToStorage(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DataHandler dh;
#ifndef AddMovieToStorage
	PyMac_PRECHECK(AddMovieToStorage);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_err = AddMovieToStorage(_self->ob_itself,
	                         dh);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_UpdateMovieInStorage(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DataHandler dh;
#ifndef UpdateMovieInStorage
	PyMac_PRECHECK(UpdateMovieInStorage);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_err = UpdateMovieInStorage(_self->ob_itself,
	                            dh);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_HasMovieChanged(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef HasMovieChanged
	PyMac_PRECHECK(HasMovieChanged);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = HasMovieChanged(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MovieObj_ClearMovieChanged(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef ClearMovieChanged
	PyMac_PRECHECK(ClearMovieChanged);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ClearMovieChanged(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_SetMovieDefaultDataRef(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataRef;
	OSType dataRefType;
#ifndef SetMovieDefaultDataRef
	PyMac_PRECHECK(SetMovieDefaultDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_err = SetMovieDefaultDataRef(_self->ob_itself,
	                              dataRef,
	                              dataRefType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieDefaultDataRef(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataRef;
	OSType dataRefType;
#ifndef GetMovieDefaultDataRef
	PyMac_PRECHECK(GetMovieDefaultDataRef);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetMovieDefaultDataRef(_self->ob_itself,
	                              &dataRef,
	                              &dataRefType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     ResObj_New, dataRef,
	                     PyMac_BuildOSType, dataRefType);
	return _res;
}

static PyObject *MovieObj_SetMovieColorTable(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	CTabHandle ctab;
#ifndef SetMovieColorTable
	PyMac_PRECHECK(SetMovieColorTable);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &ctab))
		return NULL;
	_err = SetMovieColorTable(_self->ob_itself,
	                          ctab);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieColorTable(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	CTabHandle ctab;
#ifndef GetMovieColorTable
	PyMac_PRECHECK(GetMovieColorTable);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetMovieColorTable(_self->ob_itself,
	                          &ctab);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, ctab);
	return _res;
}

static PyObject *MovieObj_FlattenMovie(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long movieFlattenFlags;
	FSSpec theFile;
	OSType creator;
	ScriptCode scriptTag;
	long createMovieFileFlags;
	short resId;
	Str255 resName;
#ifndef FlattenMovie
	PyMac_PRECHECK(FlattenMovie);
#endif
	if (!PyArg_ParseTuple(_args, "lO&O&hlO&",
	                      &movieFlattenFlags,
	                      PyMac_GetFSSpec, &theFile,
	                      PyMac_GetOSType, &creator,
	                      &scriptTag,
	                      &createMovieFileFlags,
	                      PyMac_GetStr255, resName))
		return NULL;
	FlattenMovie(_self->ob_itself,
	             movieFlattenFlags,
	             &theFile,
	             creator,
	             scriptTag,
	             createMovieFileFlags,
	             &resId,
	             resName);
	_res = Py_BuildValue("h",
	                     resId);
	return _res;
}

static PyObject *MovieObj_FlattenMovieData(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie _rv;
	long movieFlattenFlags;
	FSSpec theFile;
	OSType creator;
	ScriptCode scriptTag;
	long createMovieFileFlags;
#ifndef FlattenMovieData
	PyMac_PRECHECK(FlattenMovieData);
#endif
	if (!PyArg_ParseTuple(_args, "lO&O&hl",
	                      &movieFlattenFlags,
	                      PyMac_GetFSSpec, &theFile,
	                      PyMac_GetOSType, &creator,
	                      &scriptTag,
	                      &createMovieFileFlags))
		return NULL;
	_rv = FlattenMovieData(_self->ob_itself,
	                       movieFlattenFlags,
	                       &theFile,
	                       creator,
	                       scriptTag,
	                       createMovieFileFlags);
	_res = Py_BuildValue("O&",
	                     MovieObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_FlattenMovieDataToDataRef(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie _rv;
	long movieFlattenFlags;
	Handle dataRef;
	OSType dataRefType;
	OSType creator;
	ScriptCode scriptTag;
	long createMovieFileFlags;
#ifndef FlattenMovieDataToDataRef
	PyMac_PRECHECK(FlattenMovieDataToDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "lO&O&O&hl",
	                      &movieFlattenFlags,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      PyMac_GetOSType, &creator,
	                      &scriptTag,
	                      &createMovieFileFlags))
		return NULL;
	_rv = FlattenMovieDataToDataRef(_self->ob_itself,
	                                movieFlattenFlags,
	                                dataRef,
	                                dataRefType,
	                                creator,
	                                scriptTag,
	                                createMovieFileFlags);
	_res = Py_BuildValue("O&",
	                     MovieObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_MovieSearchText(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Ptr text;
	long size;
	long searchFlags;
	Track searchTrack;
	TimeValue searchTime;
	long searchOffset;
#ifndef MovieSearchText
	PyMac_PRECHECK(MovieSearchText);
#endif
	if (!PyArg_ParseTuple(_args, "sll",
	                      &text,
	                      &size,
	                      &searchFlags))
		return NULL;
	_err = MovieSearchText(_self->ob_itself,
	                       text,
	                       size,
	                       searchFlags,
	                       &searchTrack,
	                       &searchTime,
	                       &searchOffset);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&ll",
	                     TrackObj_New, searchTrack,
	                     searchTime,
	                     searchOffset);
	return _res;
}

static PyObject *MovieObj_GetPosterBox(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect boxRect;
#ifndef GetPosterBox
	PyMac_PRECHECK(GetPosterBox);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetPosterBox(_self->ob_itself,
	             &boxRect);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &boxRect);
	return _res;
}

static PyObject *MovieObj_SetPosterBox(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect boxRect;
#ifndef SetPosterBox
	PyMac_PRECHECK(SetPosterBox);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &boxRect))
		return NULL;
	SetPosterBox(_self->ob_itself,
	             &boxRect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMovieSegmentDisplayBoundsRgn(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
	TimeValue time;
	TimeValue duration;
#ifndef GetMovieSegmentDisplayBoundsRgn
	PyMac_PRECHECK(GetMovieSegmentDisplayBoundsRgn);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &time,
	                      &duration))
		return NULL;
	_rv = GetMovieSegmentDisplayBoundsRgn(_self->ob_itself,
	                                      time,
	                                      duration);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_GetMovieStatus(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	Track firstProblemTrack;
#ifndef GetMovieStatus
	PyMac_PRECHECK(GetMovieStatus);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieStatus(_self->ob_itself,
	                     &firstProblemTrack);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     TrackObj_New, firstProblemTrack);
	return _res;
}

static PyObject *MovieObj_NewMovieController(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MovieController _rv;
	Rect movieRect;
	long someFlags;
#ifndef NewMovieController
	PyMac_PRECHECK(NewMovieController);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetRect, &movieRect,
	                      &someFlags))
		return NULL;
	_rv = NewMovieController(_self->ob_itself,
	                         &movieRect,
	                         someFlags);
	_res = Py_BuildValue("O&",
	                     MovieCtlObj_New, _rv);
	return _res;
}

static PyObject *MovieObj_PutMovieOnScrap(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long movieScrapFlags;
#ifndef PutMovieOnScrap
	PyMac_PRECHECK(PutMovieOnScrap);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &movieScrapFlags))
		return NULL;
	_err = PutMovieOnScrap(_self->ob_itself,
	                       movieScrapFlags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_SetMoviePlayHints(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long flags;
	long flagsMask;
#ifndef SetMoviePlayHints
	PyMac_PRECHECK(SetMoviePlayHints);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &flags,
	                      &flagsMask))
		return NULL;
	SetMoviePlayHints(_self->ob_itself,
	                  flags,
	                  flagsMask);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MovieObj_GetMaxLoadedTimeInMovie(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeValue time;
#ifndef GetMaxLoadedTimeInMovie
	PyMac_PRECHECK(GetMaxLoadedTimeInMovie);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetMaxLoadedTimeInMovie(_self->ob_itself,
	                               &time);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     time);
	return _res;
}

static PyObject *MovieObj_QTMovieNeedsTimeTable(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Boolean needsTimeTable;
#ifndef QTMovieNeedsTimeTable
	PyMac_PRECHECK(QTMovieNeedsTimeTable);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = QTMovieNeedsTimeTable(_self->ob_itself,
	                             &needsTimeTable);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("b",
	                     needsTimeTable);
	return _res;
}

static PyObject *MovieObj_QTGetDataRefMaxFileOffset(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType dataRefType;
	Handle dataRef;
	long offset;
#ifndef QTGetDataRefMaxFileOffset
	PyMac_PRECHECK(QTGetDataRefMaxFileOffset);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &dataRefType,
	                      ResObj_Convert, &dataRef))
		return NULL;
	_err = QTGetDataRefMaxFileOffset(_self->ob_itself,
	                                 dataRefType,
	                                 dataRef,
	                                 &offset);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     offset);
	return _res;
}

static PyMethodDef MovieObj_methods[] = {
	{"MoviesTask", (PyCFunction)MovieObj_MoviesTask, 1,
	 PyDoc_STR("(long maxMilliSecToUse) -> None")},
	{"PrerollMovie", (PyCFunction)MovieObj_PrerollMovie, 1,
	 PyDoc_STR("(TimeValue time, Fixed Rate) -> None")},
	{"AbortPrePrerollMovie", (PyCFunction)MovieObj_AbortPrePrerollMovie, 1,
	 PyDoc_STR("(OSErr err) -> None")},
	{"LoadMovieIntoRam", (PyCFunction)MovieObj_LoadMovieIntoRam, 1,
	 PyDoc_STR("(TimeValue time, TimeValue duration, long flags) -> None")},
	{"SetMovieActive", (PyCFunction)MovieObj_SetMovieActive, 1,
	 PyDoc_STR("(Boolean active) -> None")},
	{"GetMovieActive", (PyCFunction)MovieObj_GetMovieActive, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"StartMovie", (PyCFunction)MovieObj_StartMovie, 1,
	 PyDoc_STR("() -> None")},
	{"StopMovie", (PyCFunction)MovieObj_StopMovie, 1,
	 PyDoc_STR("() -> None")},
	{"GoToBeginningOfMovie", (PyCFunction)MovieObj_GoToBeginningOfMovie, 1,
	 PyDoc_STR("() -> None")},
	{"GoToEndOfMovie", (PyCFunction)MovieObj_GoToEndOfMovie, 1,
	 PyDoc_STR("() -> None")},
	{"IsMovieDone", (PyCFunction)MovieObj_IsMovieDone, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"GetMoviePreviewMode", (PyCFunction)MovieObj_GetMoviePreviewMode, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"SetMoviePreviewMode", (PyCFunction)MovieObj_SetMoviePreviewMode, 1,
	 PyDoc_STR("(Boolean usePreview) -> None")},
	{"ShowMoviePoster", (PyCFunction)MovieObj_ShowMoviePoster, 1,
	 PyDoc_STR("() -> None")},
	{"GetMovieTimeBase", (PyCFunction)MovieObj_GetMovieTimeBase, 1,
	 PyDoc_STR("() -> (TimeBase _rv)")},
	{"SetMovieMasterTimeBase", (PyCFunction)MovieObj_SetMovieMasterTimeBase, 1,
	 PyDoc_STR("(TimeBase tb, TimeRecord slaveZero) -> None")},
	{"SetMovieMasterClock", (PyCFunction)MovieObj_SetMovieMasterClock, 1,
	 PyDoc_STR("(Component clockMeister, TimeRecord slaveZero) -> None")},
	{"ChooseMovieClock", (PyCFunction)MovieObj_ChooseMovieClock, 1,
	 PyDoc_STR("(long flags) -> None")},
	{"GetMovieGWorld", (PyCFunction)MovieObj_GetMovieGWorld, 1,
	 PyDoc_STR("() -> (CGrafPtr port, GDHandle gdh)")},
	{"SetMovieGWorld", (PyCFunction)MovieObj_SetMovieGWorld, 1,
	 PyDoc_STR("(CGrafPtr port, GDHandle gdh) -> None")},
	{"GetMovieNaturalBoundsRect", (PyCFunction)MovieObj_GetMovieNaturalBoundsRect, 1,
	 PyDoc_STR("() -> (Rect naturalBounds)")},
	{"GetNextTrackForCompositing", (PyCFunction)MovieObj_GetNextTrackForCompositing, 1,
	 PyDoc_STR("(Track theTrack) -> (Track _rv)")},
	{"GetPrevTrackForCompositing", (PyCFunction)MovieObj_GetPrevTrackForCompositing, 1,
	 PyDoc_STR("(Track theTrack) -> (Track _rv)")},
	{"GetMoviePict", (PyCFunction)MovieObj_GetMoviePict, 1,
	 PyDoc_STR("(TimeValue time) -> (PicHandle _rv)")},
	{"GetMoviePosterPict", (PyCFunction)MovieObj_GetMoviePosterPict, 1,
	 PyDoc_STR("() -> (PicHandle _rv)")},
	{"UpdateMovie", (PyCFunction)MovieObj_UpdateMovie, 1,
	 PyDoc_STR("() -> None")},
	{"InvalidateMovieRegion", (PyCFunction)MovieObj_InvalidateMovieRegion, 1,
	 PyDoc_STR("(RgnHandle invalidRgn) -> None")},
	{"GetMovieBox", (PyCFunction)MovieObj_GetMovieBox, 1,
	 PyDoc_STR("() -> (Rect boxRect)")},
	{"SetMovieBox", (PyCFunction)MovieObj_SetMovieBox, 1,
	 PyDoc_STR("(Rect boxRect) -> None")},
	{"GetMovieDisplayClipRgn", (PyCFunction)MovieObj_GetMovieDisplayClipRgn, 1,
	 PyDoc_STR("() -> (RgnHandle _rv)")},
	{"SetMovieDisplayClipRgn", (PyCFunction)MovieObj_SetMovieDisplayClipRgn, 1,
	 PyDoc_STR("(RgnHandle theClip) -> None")},
	{"GetMovieClipRgn", (PyCFunction)MovieObj_GetMovieClipRgn, 1,
	 PyDoc_STR("() -> (RgnHandle _rv)")},
	{"SetMovieClipRgn", (PyCFunction)MovieObj_SetMovieClipRgn, 1,
	 PyDoc_STR("(RgnHandle theClip) -> None")},
	{"GetMovieDisplayBoundsRgn", (PyCFunction)MovieObj_GetMovieDisplayBoundsRgn, 1,
	 PyDoc_STR("() -> (RgnHandle _rv)")},
	{"GetMovieBoundsRgn", (PyCFunction)MovieObj_GetMovieBoundsRgn, 1,
	 PyDoc_STR("() -> (RgnHandle _rv)")},
	{"SetMovieVideoOutput", (PyCFunction)MovieObj_SetMovieVideoOutput, 1,
	 PyDoc_STR("(ComponentInstance vout) -> None")},
	{"PutMovieIntoHandle", (PyCFunction)MovieObj_PutMovieIntoHandle, 1,
	 PyDoc_STR("(Handle publicMovie) -> None")},
	{"PutMovieIntoDataFork", (PyCFunction)MovieObj_PutMovieIntoDataFork, 1,
	 PyDoc_STR("(short fRefNum, long offset, long maxSize) -> None")},
	{"PutMovieIntoDataFork64", (PyCFunction)MovieObj_PutMovieIntoDataFork64, 1,
	 PyDoc_STR("(long fRefNum, wide offset, unsigned long maxSize) -> None")},
	{"PutMovieIntoStorage", (PyCFunction)MovieObj_PutMovieIntoStorage, 1,
	 PyDoc_STR("(DataHandler dh, wide offset, unsigned long maxSize) -> None")},
	{"PutMovieForDataRefIntoHandle", (PyCFunction)MovieObj_PutMovieForDataRefIntoHandle, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataRefType, Handle publicMovie) -> None")},
	{"GetMovieCreationTime", (PyCFunction)MovieObj_GetMovieCreationTime, 1,
	 PyDoc_STR("() -> (unsigned long _rv)")},
	{"GetMovieModificationTime", (PyCFunction)MovieObj_GetMovieModificationTime, 1,
	 PyDoc_STR("() -> (unsigned long _rv)")},
	{"GetMovieTimeScale", (PyCFunction)MovieObj_GetMovieTimeScale, 1,
	 PyDoc_STR("() -> (TimeScale _rv)")},
	{"SetMovieTimeScale", (PyCFunction)MovieObj_SetMovieTimeScale, 1,
	 PyDoc_STR("(TimeScale timeScale) -> None")},
	{"GetMovieDuration", (PyCFunction)MovieObj_GetMovieDuration, 1,
	 PyDoc_STR("() -> (TimeValue _rv)")},
	{"GetMovieRate", (PyCFunction)MovieObj_GetMovieRate, 1,
	 PyDoc_STR("() -> (Fixed _rv)")},
	{"SetMovieRate", (PyCFunction)MovieObj_SetMovieRate, 1,
	 PyDoc_STR("(Fixed rate) -> None")},
	{"GetMoviePreferredRate", (PyCFunction)MovieObj_GetMoviePreferredRate, 1,
	 PyDoc_STR("() -> (Fixed _rv)")},
	{"SetMoviePreferredRate", (PyCFunction)MovieObj_SetMoviePreferredRate, 1,
	 PyDoc_STR("(Fixed rate) -> None")},
	{"GetMoviePreferredVolume", (PyCFunction)MovieObj_GetMoviePreferredVolume, 1,
	 PyDoc_STR("() -> (short _rv)")},
	{"SetMoviePreferredVolume", (PyCFunction)MovieObj_SetMoviePreferredVolume, 1,
	 PyDoc_STR("(short volume) -> None")},
	{"GetMovieVolume", (PyCFunction)MovieObj_GetMovieVolume, 1,
	 PyDoc_STR("() -> (short _rv)")},
	{"SetMovieVolume", (PyCFunction)MovieObj_SetMovieVolume, 1,
	 PyDoc_STR("(short volume) -> None")},
	{"GetMoviePreviewTime", (PyCFunction)MovieObj_GetMoviePreviewTime, 1,
	 PyDoc_STR("() -> (TimeValue previewTime, TimeValue previewDuration)")},
	{"SetMoviePreviewTime", (PyCFunction)MovieObj_SetMoviePreviewTime, 1,
	 PyDoc_STR("(TimeValue previewTime, TimeValue previewDuration) -> None")},
	{"GetMoviePosterTime", (PyCFunction)MovieObj_GetMoviePosterTime, 1,
	 PyDoc_STR("() -> (TimeValue _rv)")},
	{"SetMoviePosterTime", (PyCFunction)MovieObj_SetMoviePosterTime, 1,
	 PyDoc_STR("(TimeValue posterTime) -> None")},
	{"GetMovieSelection", (PyCFunction)MovieObj_GetMovieSelection, 1,
	 PyDoc_STR("() -> (TimeValue selectionTime, TimeValue selectionDuration)")},
	{"SetMovieSelection", (PyCFunction)MovieObj_SetMovieSelection, 1,
	 PyDoc_STR("(TimeValue selectionTime, TimeValue selectionDuration) -> None")},
	{"SetMovieActiveSegment", (PyCFunction)MovieObj_SetMovieActiveSegment, 1,
	 PyDoc_STR("(TimeValue startTime, TimeValue duration) -> None")},
	{"GetMovieActiveSegment", (PyCFunction)MovieObj_GetMovieActiveSegment, 1,
	 PyDoc_STR("() -> (TimeValue startTime, TimeValue duration)")},
	{"GetMovieTime", (PyCFunction)MovieObj_GetMovieTime, 1,
	 PyDoc_STR("() -> (TimeValue _rv, TimeRecord currentTime)")},
	{"SetMovieTime", (PyCFunction)MovieObj_SetMovieTime, 1,
	 PyDoc_STR("(TimeRecord newtime) -> None")},
	{"SetMovieTimeValue", (PyCFunction)MovieObj_SetMovieTimeValue, 1,
	 PyDoc_STR("(TimeValue newtime) -> None")},
	{"GetMovieUserData", (PyCFunction)MovieObj_GetMovieUserData, 1,
	 PyDoc_STR("() -> (UserData _rv)")},
	{"GetMovieTrackCount", (PyCFunction)MovieObj_GetMovieTrackCount, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"GetMovieTrack", (PyCFunction)MovieObj_GetMovieTrack, 1,
	 PyDoc_STR("(long trackID) -> (Track _rv)")},
	{"GetMovieIndTrack", (PyCFunction)MovieObj_GetMovieIndTrack, 1,
	 PyDoc_STR("(long index) -> (Track _rv)")},
	{"GetMovieIndTrackType", (PyCFunction)MovieObj_GetMovieIndTrackType, 1,
	 PyDoc_STR("(long index, OSType trackType, long flags) -> (Track _rv)")},
	{"NewMovieTrack", (PyCFunction)MovieObj_NewMovieTrack, 1,
	 PyDoc_STR("(Fixed width, Fixed height, short trackVolume) -> (Track _rv)")},
	{"SetAutoTrackAlternatesEnabled", (PyCFunction)MovieObj_SetAutoTrackAlternatesEnabled, 1,
	 PyDoc_STR("(Boolean enable) -> None")},
	{"SelectMovieAlternates", (PyCFunction)MovieObj_SelectMovieAlternates, 1,
	 PyDoc_STR("() -> None")},
	{"InsertMovieSegment", (PyCFunction)MovieObj_InsertMovieSegment, 1,
	 PyDoc_STR("(Movie dstMovie, TimeValue srcIn, TimeValue srcDuration, TimeValue dstIn) -> None")},
	{"InsertEmptyMovieSegment", (PyCFunction)MovieObj_InsertEmptyMovieSegment, 1,
	 PyDoc_STR("(TimeValue dstIn, TimeValue dstDuration) -> None")},
	{"DeleteMovieSegment", (PyCFunction)MovieObj_DeleteMovieSegment, 1,
	 PyDoc_STR("(TimeValue startTime, TimeValue duration) -> None")},
	{"ScaleMovieSegment", (PyCFunction)MovieObj_ScaleMovieSegment, 1,
	 PyDoc_STR("(TimeValue startTime, TimeValue oldDuration, TimeValue newDuration) -> None")},
	{"CutMovieSelection", (PyCFunction)MovieObj_CutMovieSelection, 1,
	 PyDoc_STR("() -> (Movie _rv)")},
	{"CopyMovieSelection", (PyCFunction)MovieObj_CopyMovieSelection, 1,
	 PyDoc_STR("() -> (Movie _rv)")},
	{"PasteMovieSelection", (PyCFunction)MovieObj_PasteMovieSelection, 1,
	 PyDoc_STR("(Movie src) -> None")},
	{"AddMovieSelection", (PyCFunction)MovieObj_AddMovieSelection, 1,
	 PyDoc_STR("(Movie src) -> None")},
	{"ClearMovieSelection", (PyCFunction)MovieObj_ClearMovieSelection, 1,
	 PyDoc_STR("() -> None")},
	{"PutMovieIntoTypedHandle", (PyCFunction)MovieObj_PutMovieIntoTypedHandle, 1,
	 PyDoc_STR("(Track targetTrack, OSType handleType, Handle publicMovie, TimeValue start, TimeValue dur, long flags, ComponentInstance userComp) -> None")},
	{"CopyMovieSettings", (PyCFunction)MovieObj_CopyMovieSettings, 1,
	 PyDoc_STR("(Movie dstMovie) -> None")},
	{"ConvertMovieToFile", (PyCFunction)MovieObj_ConvertMovieToFile, 1,
	 PyDoc_STR("(Track onlyTrack, FSSpec outputFile, OSType fileType, OSType creator, ScriptCode scriptTag, long flags, ComponentInstance userComp) -> (short resID)")},
	{"GetMovieDataSize", (PyCFunction)MovieObj_GetMovieDataSize, 1,
	 PyDoc_STR("(TimeValue startTime, TimeValue duration) -> (long _rv)")},
	{"GetMovieDataSize64", (PyCFunction)MovieObj_GetMovieDataSize64, 1,
	 PyDoc_STR("(TimeValue startTime, TimeValue duration) -> (wide dataSize)")},
	{"PtInMovie", (PyCFunction)MovieObj_PtInMovie, 1,
	 PyDoc_STR("(Point pt) -> (Boolean _rv)")},
	{"SetMovieLanguage", (PyCFunction)MovieObj_SetMovieLanguage, 1,
	 PyDoc_STR("(long language) -> None")},
	{"CopyMovieUserData", (PyCFunction)MovieObj_CopyMovieUserData, 1,
	 PyDoc_STR("(Movie dstMovie, OSType copyRule) -> None")},
	{"GetMovieNextInterestingTime", (PyCFunction)MovieObj_GetMovieNextInterestingTime, 1,
	 PyDoc_STR("(short interestingTimeFlags, short numMediaTypes, OSType whichMediaTypes, TimeValue time, Fixed rate) -> (TimeValue interestingTime, TimeValue interestingDuration)")},
	{"AddMovieResource", (PyCFunction)MovieObj_AddMovieResource, 1,
	 PyDoc_STR("(short resRefNum, Str255 resName) -> (short resId)")},
	{"UpdateMovieResource", (PyCFunction)MovieObj_UpdateMovieResource, 1,
	 PyDoc_STR("(short resRefNum, short resId, Str255 resName) -> None")},
	{"AddMovieToStorage", (PyCFunction)MovieObj_AddMovieToStorage, 1,
	 PyDoc_STR("(DataHandler dh) -> None")},
	{"UpdateMovieInStorage", (PyCFunction)MovieObj_UpdateMovieInStorage, 1,
	 PyDoc_STR("(DataHandler dh) -> None")},
	{"HasMovieChanged", (PyCFunction)MovieObj_HasMovieChanged, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"ClearMovieChanged", (PyCFunction)MovieObj_ClearMovieChanged, 1,
	 PyDoc_STR("() -> None")},
	{"SetMovieDefaultDataRef", (PyCFunction)MovieObj_SetMovieDefaultDataRef, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataRefType) -> None")},
	{"GetMovieDefaultDataRef", (PyCFunction)MovieObj_GetMovieDefaultDataRef, 1,
	 PyDoc_STR("() -> (Handle dataRef, OSType dataRefType)")},
	{"SetMovieColorTable", (PyCFunction)MovieObj_SetMovieColorTable, 1,
	 PyDoc_STR("(CTabHandle ctab) -> None")},
	{"GetMovieColorTable", (PyCFunction)MovieObj_GetMovieColorTable, 1,
	 PyDoc_STR("() -> (CTabHandle ctab)")},
	{"FlattenMovie", (PyCFunction)MovieObj_FlattenMovie, 1,
	 PyDoc_STR("(long movieFlattenFlags, FSSpec theFile, OSType creator, ScriptCode scriptTag, long createMovieFileFlags, Str255 resName) -> (short resId)")},
	{"FlattenMovieData", (PyCFunction)MovieObj_FlattenMovieData, 1,
	 PyDoc_STR("(long movieFlattenFlags, FSSpec theFile, OSType creator, ScriptCode scriptTag, long createMovieFileFlags) -> (Movie _rv)")},
	{"FlattenMovieDataToDataRef", (PyCFunction)MovieObj_FlattenMovieDataToDataRef, 1,
	 PyDoc_STR("(long movieFlattenFlags, Handle dataRef, OSType dataRefType, OSType creator, ScriptCode scriptTag, long createMovieFileFlags) -> (Movie _rv)")},
	{"MovieSearchText", (PyCFunction)MovieObj_MovieSearchText, 1,
	 PyDoc_STR("(Ptr text, long size, long searchFlags) -> (Track searchTrack, TimeValue searchTime, long searchOffset)")},
	{"GetPosterBox", (PyCFunction)MovieObj_GetPosterBox, 1,
	 PyDoc_STR("() -> (Rect boxRect)")},
	{"SetPosterBox", (PyCFunction)MovieObj_SetPosterBox, 1,
	 PyDoc_STR("(Rect boxRect) -> None")},
	{"GetMovieSegmentDisplayBoundsRgn", (PyCFunction)MovieObj_GetMovieSegmentDisplayBoundsRgn, 1,
	 PyDoc_STR("(TimeValue time, TimeValue duration) -> (RgnHandle _rv)")},
	{"GetMovieStatus", (PyCFunction)MovieObj_GetMovieStatus, 1,
	 PyDoc_STR("() -> (ComponentResult _rv, Track firstProblemTrack)")},
	{"NewMovieController", (PyCFunction)MovieObj_NewMovieController, 1,
	 PyDoc_STR("(Rect movieRect, long someFlags) -> (MovieController _rv)")},
	{"PutMovieOnScrap", (PyCFunction)MovieObj_PutMovieOnScrap, 1,
	 PyDoc_STR("(long movieScrapFlags) -> None")},
	{"SetMoviePlayHints", (PyCFunction)MovieObj_SetMoviePlayHints, 1,
	 PyDoc_STR("(long flags, long flagsMask) -> None")},
	{"GetMaxLoadedTimeInMovie", (PyCFunction)MovieObj_GetMaxLoadedTimeInMovie, 1,
	 PyDoc_STR("() -> (TimeValue time)")},
	{"QTMovieNeedsTimeTable", (PyCFunction)MovieObj_QTMovieNeedsTimeTable, 1,
	 PyDoc_STR("() -> (Boolean needsTimeTable)")},
	{"QTGetDataRefMaxFileOffset", (PyCFunction)MovieObj_QTGetDataRefMaxFileOffset, 1,
	 PyDoc_STR("(OSType dataRefType, Handle dataRef) -> (long offset)")},
	{NULL, NULL, 0}
};

#define MovieObj_getsetlist NULL


#define MovieObj_compare NULL

#define MovieObj_repr NULL

#define MovieObj_hash NULL
#define MovieObj_tp_init 0

#define MovieObj_tp_alloc PyType_GenericAlloc

static PyObject *MovieObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	Movie itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, MovieObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((MovieObject *)_self)->ob_itself = itself;
	return _self;
}

#define MovieObj_tp_free PyObject_Del


PyTypeObject Movie_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.Movie", /*tp_name*/
	sizeof(MovieObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) MovieObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) MovieObj_compare, /*tp_compare*/
	(reprfunc) MovieObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) MovieObj_hash, /*tp_hash*/
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
	MovieObj_methods, /* tp_methods */
	0, /*tp_members*/
	MovieObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	MovieObj_tp_init, /* tp_init */
	MovieObj_tp_alloc, /* tp_alloc */
	MovieObj_tp_new, /* tp_new */
	MovieObj_tp_free, /* tp_free */
};

/* --------------------- End object type Movie ---------------------- */


/* ---------------------- Object type SGOutput ---------------------- */

PyTypeObject SGOutput_Type;

#define SGOutputObj_Check(x) ((x)->ob_type == &SGOutput_Type || PyObject_TypeCheck((x), &SGOutput_Type))

typedef struct SGOutputObject {
	PyObject_HEAD
	SGOutput ob_itself;
} SGOutputObject;

PyObject *SGOutputObj_New(SGOutput itself)
{
	SGOutputObject *it;
	if (itself == NULL) {
	                                PyErr_SetString(Qt_Error,"Cannot create SGOutput from NULL pointer");
	                                return NULL;
	                        }
	it = PyObject_NEW(SGOutputObject, &SGOutput_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}

int SGOutputObj_Convert(PyObject *v, SGOutput *p_itself)
{
	if (v == Py_None)
	{
		*p_itself = NULL;
		return 1;
	}
	if (!SGOutputObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "SGOutput required");
		return 0;
	}
	*p_itself = ((SGOutputObject *)v)->ob_itself;
	return 1;
}

static void SGOutputObj_dealloc(SGOutputObject *self)
{
	/* Cleanup of self->ob_itself goes here */
	self->ob_type->tp_free((PyObject *)self);
}

static PyMethodDef SGOutputObj_methods[] = {
	{NULL, NULL, 0}
};

#define SGOutputObj_getsetlist NULL


#define SGOutputObj_compare NULL

#define SGOutputObj_repr NULL

#define SGOutputObj_hash NULL
#define SGOutputObj_tp_init 0

#define SGOutputObj_tp_alloc PyType_GenericAlloc

static PyObject *SGOutputObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	SGOutput itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, SGOutputObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((SGOutputObject *)_self)->ob_itself = itself;
	return _self;
}

#define SGOutputObj_tp_free PyObject_Del


PyTypeObject SGOutput_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.SGOutput", /*tp_name*/
	sizeof(SGOutputObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) SGOutputObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) SGOutputObj_compare, /*tp_compare*/
	(reprfunc) SGOutputObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) SGOutputObj_hash, /*tp_hash*/
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
	SGOutputObj_methods, /* tp_methods */
	0, /*tp_members*/
	SGOutputObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	SGOutputObj_tp_init, /* tp_init */
	SGOutputObj_tp_alloc, /* tp_alloc */
	SGOutputObj_tp_new, /* tp_new */
	SGOutputObj_tp_free, /* tp_free */
};

/* -------------------- End object type SGOutput -------------------- */


static PyObject *Qt_EnterMovies(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef EnterMovies
	PyMac_PRECHECK(EnterMovies);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = EnterMovies();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_ExitMovies(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef ExitMovies
	PyMac_PRECHECK(ExitMovies);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ExitMovies();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_GetMoviesError(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef GetMoviesError
	PyMac_PRECHECK(GetMoviesError);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetMoviesError();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_ClearMoviesStickyError(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef ClearMoviesStickyError
	PyMac_PRECHECK(ClearMoviesStickyError);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ClearMoviesStickyError();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_GetMoviesStickyError(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef GetMoviesStickyError
	PyMac_PRECHECK(GetMoviesStickyError);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetMoviesStickyError();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_QTGetWallClockTimeBase(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TimeBase wallClockTimeBase;
#ifndef QTGetWallClockTimeBase
	PyMac_PRECHECK(QTGetWallClockTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = QTGetWallClockTimeBase(&wallClockTimeBase);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     TimeBaseObj_New, wallClockTimeBase);
	return _res;
}

static PyObject *Qt_QTIdleManagerOpen(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	IdleManager _rv;
#ifndef QTIdleManagerOpen
	PyMac_PRECHECK(QTIdleManagerOpen);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = QTIdleManagerOpen();
	_res = Py_BuildValue("O&",
	                     IdleManagerObj_New, _rv);
	return _res;
}

static PyObject *Qt_CreateMovieControl(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	WindowPtr theWindow;
	Rect localRect;
	Movie theMovie;
	UInt32 options;
	ControlHandle returnedControl;
#ifndef CreateMovieControl
	PyMac_PRECHECK(CreateMovieControl);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      WinObj_Convert, &theWindow,
	                      MovieObj_Convert, &theMovie,
	                      &options))
		return NULL;
	_err = CreateMovieControl(theWindow,
	                          &localRect,
	                          theMovie,
	                          options,
	                          &returnedControl);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     PyMac_BuildRect, &localRect,
	                     CtlObj_New, returnedControl);
	return _res;
}

static PyObject *Qt_DisposeMatte(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PixMapHandle theMatte;
#ifndef DisposeMatte
	PyMac_PRECHECK(DisposeMatte);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theMatte))
		return NULL;
	DisposeMatte(theMatte);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_NewMovie(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie _rv;
	long flags;
#ifndef NewMovie
	PyMac_PRECHECK(NewMovie);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &flags))
		return NULL;
	_rv = NewMovie(flags);
	_res = Py_BuildValue("O&",
	                     MovieObj_New, _rv);
	return _res;
}

static PyObject *Qt_QTGetTimeUntilNextTask(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long duration;
	long scale;
#ifndef QTGetTimeUntilNextTask
	PyMac_PRECHECK(QTGetTimeUntilNextTask);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &scale))
		return NULL;
	_err = QTGetTimeUntilNextTask(&duration,
	                              scale);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     duration);
	return _res;
}

static PyObject *Qt_GetDataHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Component _rv;
	Handle dataRef;
	OSType dataHandlerSubType;
	long flags;
#ifndef GetDataHandler
	PyMac_PRECHECK(GetDataHandler);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataHandlerSubType,
	                      &flags))
		return NULL;
	_rv = GetDataHandler(dataRef,
	                     dataHandlerSubType,
	                     flags);
	_res = Py_BuildValue("O&",
	                     CmpObj_New, _rv);
	return _res;
}

static PyObject *Qt_PasteHandleIntoMovie(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle h;
	OSType handleType;
	Movie theMovie;
	long flags;
	ComponentInstance userComp;
#ifndef PasteHandleIntoMovie
	PyMac_PRECHECK(PasteHandleIntoMovie);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&lO&",
	                      ResObj_Convert, &h,
	                      PyMac_GetOSType, &handleType,
	                      MovieObj_Convert, &theMovie,
	                      &flags,
	                      CmpInstObj_Convert, &userComp))
		return NULL;
	_err = PasteHandleIntoMovie(h,
	                            handleType,
	                            theMovie,
	                            flags,
	                            userComp);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_GetMovieImporterForDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType dataRefType;
	Handle dataRef;
	long flags;
	Component importer;
#ifndef GetMovieImporterForDataRef
	PyMac_PRECHECK(GetMovieImporterForDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      PyMac_GetOSType, &dataRefType,
	                      ResObj_Convert, &dataRef,
	                      &flags))
		return NULL;
	_err = GetMovieImporterForDataRef(dataRefType,
	                                  dataRef,
	                                  flags,
	                                  &importer);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CmpObj_New, importer);
	return _res;
}

static PyObject *Qt_QTGetMIMETypeInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	char* mimeStringStart;
	short mimeStringLength;
	OSType infoSelector;
	void * infoDataPtr;
	long infoDataSize;
#ifndef QTGetMIMETypeInfo
	PyMac_PRECHECK(QTGetMIMETypeInfo);
#endif
	if (!PyArg_ParseTuple(_args, "shO&s",
	                      &mimeStringStart,
	                      &mimeStringLength,
	                      PyMac_GetOSType, &infoSelector,
	                      &infoDataPtr))
		return NULL;
	_err = QTGetMIMETypeInfo(mimeStringStart,
	                         mimeStringLength,
	                         infoSelector,
	                         infoDataPtr,
	                         &infoDataSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     infoDataSize);
	return _res;
}

static PyObject *Qt_TrackTimeToMediaTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeValue _rv;
	TimeValue value;
	Track theTrack;
#ifndef TrackTimeToMediaTime
	PyMac_PRECHECK(TrackTimeToMediaTime);
#endif
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &value,
	                      TrackObj_Convert, &theTrack))
		return NULL;
	_rv = TrackTimeToMediaTime(value,
	                           theTrack);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_NewUserData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	UserData theUserData;
#ifndef NewUserData
	PyMac_PRECHECK(NewUserData);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = NewUserData(&theUserData);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     UserDataObj_New, theUserData);
	return _res;
}

static PyObject *Qt_NewUserDataFromHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle h;
	UserData theUserData;
#ifndef NewUserDataFromHandle
	PyMac_PRECHECK(NewUserDataFromHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &h))
		return NULL;
	_err = NewUserDataFromHandle(h,
	                             &theUserData);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     UserDataObj_New, theUserData);
	return _res;
}

static PyObject *Qt_CreateMovieFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec fileSpec;
	OSType creator;
	ScriptCode scriptTag;
	long createMovieFileFlags;
	short resRefNum;
	Movie newmovie;
#ifndef CreateMovieFile
	PyMac_PRECHECK(CreateMovieFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&hl",
	                      PyMac_GetFSSpec, &fileSpec,
	                      PyMac_GetOSType, &creator,
	                      &scriptTag,
	                      &createMovieFileFlags))
		return NULL;
	_err = CreateMovieFile(&fileSpec,
	                       creator,
	                       scriptTag,
	                       createMovieFileFlags,
	                       &resRefNum,
	                       &newmovie);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("hO&",
	                     resRefNum,
	                     MovieObj_New, newmovie);
	return _res;
}

static PyObject *Qt_OpenMovieFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec fileSpec;
	short resRefNum;
	SInt8 permission;
#ifndef OpenMovieFile
	PyMac_PRECHECK(OpenMovieFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      PyMac_GetFSSpec, &fileSpec,
	                      &permission))
		return NULL;
	_err = OpenMovieFile(&fileSpec,
	                     &resRefNum,
	                     permission);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     resRefNum);
	return _res;
}

static PyObject *Qt_CloseMovieFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short resRefNum;
#ifndef CloseMovieFile
	PyMac_PRECHECK(CloseMovieFile);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &resRefNum))
		return NULL;
	_err = CloseMovieFile(resRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_DeleteMovieFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec fileSpec;
#ifndef DeleteMovieFile
	PyMac_PRECHECK(DeleteMovieFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &fileSpec))
		return NULL;
	_err = DeleteMovieFile(&fileSpec);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_NewMovieFromFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Movie theMovie;
	short resRefNum;
	short resId;
	short newMovieFlags;
	Boolean dataRefWasChanged;
#ifndef NewMovieFromFile
	PyMac_PRECHECK(NewMovieFromFile);
#endif
	if (!PyArg_ParseTuple(_args, "hhh",
	                      &resRefNum,
	                      &resId,
	                      &newMovieFlags))
		return NULL;
	_err = NewMovieFromFile(&theMovie,
	                        resRefNum,
	                        &resId,
	                        (StringPtr)0,
	                        newMovieFlags,
	                        &dataRefWasChanged);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&hb",
	                     MovieObj_New, theMovie,
	                     resId,
	                     dataRefWasChanged);
	return _res;
}

static PyObject *Qt_NewMovieFromHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Movie theMovie;
	Handle h;
	short newMovieFlags;
	Boolean dataRefWasChanged;
#ifndef NewMovieFromHandle
	PyMac_PRECHECK(NewMovieFromHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      ResObj_Convert, &h,
	                      &newMovieFlags))
		return NULL;
	_err = NewMovieFromHandle(&theMovie,
	                          h,
	                          newMovieFlags,
	                          &dataRefWasChanged);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&b",
	                     MovieObj_New, theMovie,
	                     dataRefWasChanged);
	return _res;
}

static PyObject *Qt_NewMovieFromDataFork(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Movie theMovie;
	short fRefNum;
	long fileOffset;
	short newMovieFlags;
	Boolean dataRefWasChanged;
#ifndef NewMovieFromDataFork
	PyMac_PRECHECK(NewMovieFromDataFork);
#endif
	if (!PyArg_ParseTuple(_args, "hlh",
	                      &fRefNum,
	                      &fileOffset,
	                      &newMovieFlags))
		return NULL;
	_err = NewMovieFromDataFork(&theMovie,
	                            fRefNum,
	                            fileOffset,
	                            newMovieFlags,
	                            &dataRefWasChanged);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&b",
	                     MovieObj_New, theMovie,
	                     dataRefWasChanged);
	return _res;
}

static PyObject *Qt_NewMovieFromDataFork64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Movie theMovie;
	long fRefNum;
	wide fileOffset;
	short newMovieFlags;
	Boolean dataRefWasChanged;
#ifndef NewMovieFromDataFork64
	PyMac_PRECHECK(NewMovieFromDataFork64);
#endif
	if (!PyArg_ParseTuple(_args, "lO&h",
	                      &fRefNum,
	                      PyMac_Getwide, &fileOffset,
	                      &newMovieFlags))
		return NULL;
	_err = NewMovieFromDataFork64(&theMovie,
	                              fRefNum,
	                              &fileOffset,
	                              newMovieFlags,
	                              &dataRefWasChanged);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&b",
	                     MovieObj_New, theMovie,
	                     dataRefWasChanged);
	return _res;
}

static PyObject *Qt_NewMovieFromDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Movie m;
	short flags;
	short id;
	Handle dataRef;
	OSType dtaRefType;
#ifndef NewMovieFromDataRef
	PyMac_PRECHECK(NewMovieFromDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "hO&O&",
	                      &flags,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dtaRefType))
		return NULL;
	_err = NewMovieFromDataRef(&m,
	                           flags,
	                           &id,
	                           dataRef,
	                           dtaRefType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&h",
	                     MovieObj_New, m,
	                     id);
	return _res;
}

static PyObject *Qt_NewMovieFromStorageOffset(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Movie theMovie;
	DataHandler dh;
	wide fileOffset;
	short newMovieFlags;
	Boolean dataRefWasCataRefType;
#ifndef NewMovieFromStorageOffset
	PyMac_PRECHECK(NewMovieFromStorageOffset);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      CmpInstObj_Convert, &dh,
	                      PyMac_Getwide, &fileOffset,
	                      &newMovieFlags))
		return NULL;
	_err = NewMovieFromStorageOffset(&theMovie,
	                                 dh,
	                                 &fileOffset,
	                                 newMovieFlags,
	                                 &dataRefWasCataRefType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&b",
	                     MovieObj_New, theMovie,
	                     dataRefWasCataRefType);
	return _res;
}

static PyObject *Qt_NewMovieForDataRefFromHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Movie theMovie;
	Handle h;
	short newMovieFlags;
	Boolean dataRefWasChanged;
	Handle dataRef;
	OSType dataRefType;
#ifndef NewMovieForDataRefFromHandle
	PyMac_PRECHECK(NewMovieForDataRefFromHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&hO&O&",
	                      ResObj_Convert, &h,
	                      &newMovieFlags,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_err = NewMovieForDataRefFromHandle(&theMovie,
	                                    h,
	                                    newMovieFlags,
	                                    &dataRefWasChanged,
	                                    dataRef,
	                                    dataRefType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&b",
	                     MovieObj_New, theMovie,
	                     dataRefWasChanged);
	return _res;
}

static PyObject *Qt_RemoveMovieResource(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short resRefNum;
	short resId;
#ifndef RemoveMovieResource
	PyMac_PRECHECK(RemoveMovieResource);
#endif
	if (!PyArg_ParseTuple(_args, "hh",
	                      &resRefNum,
	                      &resId))
		return NULL;
	_err = RemoveMovieResource(resRefNum,
	                           resId);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_CreateMovieStorage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataRef;
	OSType dataRefType;
	OSType creator;
	ScriptCode scriptTag;
	long createMovieFileFlags;
	DataHandler outDataHandler;
	Movie newmovie;
#ifndef CreateMovieStorage
	PyMac_PRECHECK(CreateMovieStorage);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&hl",
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      PyMac_GetOSType, &creator,
	                      &scriptTag,
	                      &createMovieFileFlags))
		return NULL;
	_err = CreateMovieStorage(dataRef,
	                          dataRefType,
	                          creator,
	                          scriptTag,
	                          createMovieFileFlags,
	                          &outDataHandler,
	                          &newmovie);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     CmpInstObj_New, outDataHandler,
	                     MovieObj_New, newmovie);
	return _res;
}

static PyObject *Qt_OpenMovieStorage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataRef;
	OSType dataRefType;
	long flags;
	DataHandler outDataHandler;
#ifndef OpenMovieStorage
	PyMac_PRECHECK(OpenMovieStorage);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      &flags))
		return NULL;
	_err = OpenMovieStorage(dataRef,
	                        dataRefType,
	                        flags,
	                        &outDataHandler);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, outDataHandler);
	return _res;
}

static PyObject *Qt_CloseMovieStorage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DataHandler dh;
#ifndef CloseMovieStorage
	PyMac_PRECHECK(CloseMovieStorage);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_err = CloseMovieStorage(dh);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_DeleteMovieStorage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataRef;
	OSType dataRefType;
#ifndef DeleteMovieStorage
	PyMac_PRECHECK(DeleteMovieStorage);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_err = DeleteMovieStorage(dataRef,
	                          dataRefType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_CreateShortcutMovieFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec fileSpec;
	OSType creator;
	ScriptCode scriptTag;
	long createMovieFileFlags;
	Handle targetDataRef;
	OSType targetDataRefType;
#ifndef CreateShortcutMovieFile
	PyMac_PRECHECK(CreateShortcutMovieFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&hlO&O&",
	                      PyMac_GetFSSpec, &fileSpec,
	                      PyMac_GetOSType, &creator,
	                      &scriptTag,
	                      &createMovieFileFlags,
	                      ResObj_Convert, &targetDataRef,
	                      PyMac_GetOSType, &targetDataRefType))
		return NULL;
	_err = CreateShortcutMovieFile(&fileSpec,
	                               creator,
	                               scriptTag,
	                               createMovieFileFlags,
	                               targetDataRef,
	                               targetDataRefType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_CanQuickTimeOpenFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec fileSpec;
	OSType fileType;
	OSType fileNameExtension;
	Boolean outCanOpenWithGraphicsImporter;
	Boolean outCanOpenAsMovie;
	Boolean outPreferGraphicsImporter;
	UInt32 inFlags;
#ifndef CanQuickTimeOpenFile
	PyMac_PRECHECK(CanQuickTimeOpenFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&l",
	                      PyMac_GetFSSpec, &fileSpec,
	                      PyMac_GetOSType, &fileType,
	                      PyMac_GetOSType, &fileNameExtension,
	                      &inFlags))
		return NULL;
	_err = CanQuickTimeOpenFile(&fileSpec,
	                            fileType,
	                            fileNameExtension,
	                            &outCanOpenWithGraphicsImporter,
	                            &outCanOpenAsMovie,
	                            &outPreferGraphicsImporter,
	                            inFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("bbb",
	                     outCanOpenWithGraphicsImporter,
	                     outCanOpenAsMovie,
	                     outPreferGraphicsImporter);
	return _res;
}

static PyObject *Qt_CanQuickTimeOpenDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataRef;
	OSType dataRefType;
	Boolean outCanOpenWithGraphicsImporter;
	Boolean outCanOpenAsMovie;
	Boolean outPreferGraphicsImporter;
	UInt32 inFlags;
#ifndef CanQuickTimeOpenDataRef
	PyMac_PRECHECK(CanQuickTimeOpenDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      &inFlags))
		return NULL;
	_err = CanQuickTimeOpenDataRef(dataRef,
	                               dataRefType,
	                               &outCanOpenWithGraphicsImporter,
	                               &outCanOpenAsMovie,
	                               &outPreferGraphicsImporter,
	                               inFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("bbb",
	                     outCanOpenWithGraphicsImporter,
	                     outCanOpenAsMovie,
	                     outPreferGraphicsImporter);
	return _res;
}

static PyObject *Qt_NewMovieFromScrap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie _rv;
	long newMovieFlags;
#ifndef NewMovieFromScrap
	PyMac_PRECHECK(NewMovieFromScrap);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &newMovieFlags))
		return NULL;
	_rv = NewMovieFromScrap(newMovieFlags);
	_res = Py_BuildValue("O&",
	                     MovieObj_New, _rv);
	return _res;
}

static PyObject *Qt_QTNewAlias(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec fss;
	AliasHandle alias;
	Boolean minimal;
#ifndef QTNewAlias
	PyMac_PRECHECK(QTNewAlias);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      PyMac_GetFSSpec, &fss,
	                      &minimal))
		return NULL;
	_err = QTNewAlias(&fss,
	                  &alias,
	                  minimal);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, alias);
	return _res;
}

static PyObject *Qt_EndFullScreen(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Ptr fullState;
	long flags;
#ifndef EndFullScreen
	PyMac_PRECHECK(EndFullScreen);
#endif
	if (!PyArg_ParseTuple(_args, "sl",
	                      &fullState,
	                      &flags))
		return NULL;
	_err = EndFullScreen(fullState,
	                     flags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_AddSoundDescriptionExtension(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SoundDescriptionHandle desc;
	Handle extension;
	OSType idType;
#ifndef AddSoundDescriptionExtension
	PyMac_PRECHECK(AddSoundDescriptionExtension);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &desc,
	                      ResObj_Convert, &extension,
	                      PyMac_GetOSType, &idType))
		return NULL;
	_err = AddSoundDescriptionExtension(desc,
	                                    extension,
	                                    idType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_GetSoundDescriptionExtension(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SoundDescriptionHandle desc;
	Handle extension;
	OSType idType;
#ifndef GetSoundDescriptionExtension
	PyMac_PRECHECK(GetSoundDescriptionExtension);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &desc,
	                      PyMac_GetOSType, &idType))
		return NULL;
	_err = GetSoundDescriptionExtension(desc,
	                                    &extension,
	                                    idType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, extension);
	return _res;
}

static PyObject *Qt_RemoveSoundDescriptionExtension(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SoundDescriptionHandle desc;
	OSType idType;
#ifndef RemoveSoundDescriptionExtension
	PyMac_PRECHECK(RemoveSoundDescriptionExtension);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &desc,
	                      PyMac_GetOSType, &idType))
		return NULL;
	_err = RemoveSoundDescriptionExtension(desc,
	                                       idType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_QTIsStandardParameterDialogEvent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	EventRecord pEvent;
	QTParameterDialog createdDialog;
#ifndef QTIsStandardParameterDialogEvent
	PyMac_PRECHECK(QTIsStandardParameterDialogEvent);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &createdDialog))
		return NULL;
	_err = QTIsStandardParameterDialogEvent(&pEvent,
	                                        createdDialog);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildEventRecord, &pEvent);
	return _res;
}

static PyObject *Qt_QTDismissStandardParameterDialog(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	QTParameterDialog createdDialog;
#ifndef QTDismissStandardParameterDialog
	PyMac_PRECHECK(QTDismissStandardParameterDialog);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &createdDialog))
		return NULL;
	_err = QTDismissStandardParameterDialog(createdDialog);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_QTStandardParameterDialogDoAction(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	QTParameterDialog createdDialog;
	long action;
	void * params;
#ifndef QTStandardParameterDialogDoAction
	PyMac_PRECHECK(QTStandardParameterDialogDoAction);
#endif
	if (!PyArg_ParseTuple(_args, "lls",
	                      &createdDialog,
	                      &action,
	                      &params))
		return NULL;
	_err = QTStandardParameterDialogDoAction(createdDialog,
	                                         action,
	                                         params);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_QTRegisterAccessKey(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Str255 accessKeyType;
	long flags;
	Handle accessKey;
#ifndef QTRegisterAccessKey
	PyMac_PRECHECK(QTRegisterAccessKey);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      PyMac_GetStr255, accessKeyType,
	                      &flags,
	                      ResObj_Convert, &accessKey))
		return NULL;
	_err = QTRegisterAccessKey(accessKeyType,
	                           flags,
	                           accessKey);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_QTUnregisterAccessKey(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Str255 accessKeyType;
	long flags;
	Handle accessKey;
#ifndef QTUnregisterAccessKey
	PyMac_PRECHECK(QTUnregisterAccessKey);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      PyMac_GetStr255, accessKeyType,
	                      &flags,
	                      ResObj_Convert, &accessKey))
		return NULL;
	_err = QTUnregisterAccessKey(accessKeyType,
	                             flags,
	                             accessKey);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_QTGetSupportedRestrictions(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType inRestrictionClass;
	UInt32 outRestrictionIDs;
#ifndef QTGetSupportedRestrictions
	PyMac_PRECHECK(QTGetSupportedRestrictions);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &inRestrictionClass))
		return NULL;
	_err = QTGetSupportedRestrictions(inRestrictionClass,
	                                  &outRestrictionIDs);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outRestrictionIDs);
	return _res;
}

static PyObject *Qt_QTTextToNativeText(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle theText;
	long encoding;
	long flags;
#ifndef QTTextToNativeText
	PyMac_PRECHECK(QTTextToNativeText);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      ResObj_Convert, &theText,
	                      &encoding,
	                      &flags))
		return NULL;
	_err = QTTextToNativeText(theText,
	                          encoding,
	                          flags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_VideoMediaResetStatistics(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
#ifndef VideoMediaResetStatistics
	PyMac_PRECHECK(VideoMediaResetStatistics);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = VideoMediaResetStatistics(mh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VideoMediaGetStatistics(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
#ifndef VideoMediaGetStatistics
	PyMac_PRECHECK(VideoMediaGetStatistics);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = VideoMediaGetStatistics(mh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VideoMediaGetStallCount(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	unsigned long stalls;
#ifndef VideoMediaGetStallCount
	PyMac_PRECHECK(VideoMediaGetStallCount);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = VideoMediaGetStallCount(mh,
	                              &stalls);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     stalls);
	return _res;
}

static PyObject *Qt_VideoMediaSetCodecParameter(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	CodecType cType;
	OSType parameterID;
	long parameterChangeSeed;
	void * dataPtr;
	long dataSize;
#ifndef VideoMediaSetCodecParameter
	PyMac_PRECHECK(VideoMediaSetCodecParameter);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&lsl",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetOSType, &cType,
	                      PyMac_GetOSType, &parameterID,
	                      &parameterChangeSeed,
	                      &dataPtr,
	                      &dataSize))
		return NULL;
	_rv = VideoMediaSetCodecParameter(mh,
	                                  cType,
	                                  parameterID,
	                                  parameterChangeSeed,
	                                  dataPtr,
	                                  dataSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VideoMediaGetCodecParameter(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	CodecType cType;
	OSType parameterID;
	Handle outParameterData;
#ifndef VideoMediaGetCodecParameter
	PyMac_PRECHECK(VideoMediaGetCodecParameter);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetOSType, &cType,
	                      PyMac_GetOSType, &parameterID,
	                      ResObj_Convert, &outParameterData))
		return NULL;
	_rv = VideoMediaGetCodecParameter(mh,
	                                  cType,
	                                  parameterID,
	                                  outParameterData);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TextMediaAddTextSample(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Ptr text;
	unsigned long size;
	short fontNumber;
	short fontSize;
	Style textFace;
	RGBColor textColor;
	RGBColor backColor;
	short textJustification;
	Rect textBox;
	long displayFlags;
	TimeValue scrollDelay;
	short hiliteStart;
	short hiliteEnd;
	RGBColor rgbHiliteColor;
	TimeValue duration;
	TimeValue sampleTime;
#ifndef TextMediaAddTextSample
	PyMac_PRECHECK(TextMediaAddTextSample);
#endif
	if (!PyArg_ParseTuple(_args, "O&slhhbhllhhl",
	                      CmpInstObj_Convert, &mh,
	                      &text,
	                      &size,
	                      &fontNumber,
	                      &fontSize,
	                      &textFace,
	                      &textJustification,
	                      &displayFlags,
	                      &scrollDelay,
	                      &hiliteStart,
	                      &hiliteEnd,
	                      &duration))
		return NULL;
	_rv = TextMediaAddTextSample(mh,
	                             text,
	                             size,
	                             fontNumber,
	                             fontSize,
	                             textFace,
	                             &textColor,
	                             &backColor,
	                             textJustification,
	                             &textBox,
	                             displayFlags,
	                             scrollDelay,
	                             hiliteStart,
	                             hiliteEnd,
	                             &rgbHiliteColor,
	                             duration,
	                             &sampleTime);
	_res = Py_BuildValue("lO&O&O&O&l",
	                     _rv,
	                     QdRGB_New, &textColor,
	                     QdRGB_New, &backColor,
	                     PyMac_BuildRect, &textBox,
	                     QdRGB_New, &rgbHiliteColor,
	                     sampleTime);
	return _res;
}

static PyObject *Qt_TextMediaAddTESample(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	TEHandle hTE;
	RGBColor backColor;
	short textJustification;
	Rect textBox;
	long displayFlags;
	TimeValue scrollDelay;
	short hiliteStart;
	short hiliteEnd;
	RGBColor rgbHiliteColor;
	TimeValue duration;
	TimeValue sampleTime;
#ifndef TextMediaAddTESample
	PyMac_PRECHECK(TextMediaAddTESample);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&hllhhl",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &hTE,
	                      &textJustification,
	                      &displayFlags,
	                      &scrollDelay,
	                      &hiliteStart,
	                      &hiliteEnd,
	                      &duration))
		return NULL;
	_rv = TextMediaAddTESample(mh,
	                           hTE,
	                           &backColor,
	                           textJustification,
	                           &textBox,
	                           displayFlags,
	                           scrollDelay,
	                           hiliteStart,
	                           hiliteEnd,
	                           &rgbHiliteColor,
	                           duration,
	                           &sampleTime);
	_res = Py_BuildValue("lO&O&O&l",
	                     _rv,
	                     QdRGB_New, &backColor,
	                     PyMac_BuildRect, &textBox,
	                     QdRGB_New, &rgbHiliteColor,
	                     sampleTime);
	return _res;
}

static PyObject *Qt_TextMediaAddHiliteSample(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short hiliteStart;
	short hiliteEnd;
	RGBColor rgbHiliteColor;
	TimeValue duration;
	TimeValue sampleTime;
#ifndef TextMediaAddHiliteSample
	PyMac_PRECHECK(TextMediaAddHiliteSample);
#endif
	if (!PyArg_ParseTuple(_args, "O&hhl",
	                      CmpInstObj_Convert, &mh,
	                      &hiliteStart,
	                      &hiliteEnd,
	                      &duration))
		return NULL;
	_rv = TextMediaAddHiliteSample(mh,
	                               hiliteStart,
	                               hiliteEnd,
	                               &rgbHiliteColor,
	                               duration,
	                               &sampleTime);
	_res = Py_BuildValue("lO&l",
	                     _rv,
	                     QdRGB_New, &rgbHiliteColor,
	                     sampleTime);
	return _res;
}

static PyObject *Qt_TextMediaDrawRaw(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	GWorldPtr gw;
	GDHandle gd;
	void * data;
	long dataSize;
	TextDescriptionHandle tdh;
#ifndef TextMediaDrawRaw
	PyMac_PRECHECK(TextMediaDrawRaw);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&slO&",
	                      CmpInstObj_Convert, &mh,
	                      GWorldObj_Convert, &gw,
	                      OptResObj_Convert, &gd,
	                      &data,
	                      &dataSize,
	                      ResObj_Convert, &tdh))
		return NULL;
	_rv = TextMediaDrawRaw(mh,
	                       gw,
	                       gd,
	                       data,
	                       dataSize,
	                       tdh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TextMediaSetTextProperty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	TimeValue atMediaTime;
	long propertyType;
	void * data;
	long dataSize;
#ifndef TextMediaSetTextProperty
	PyMac_PRECHECK(TextMediaSetTextProperty);
#endif
	if (!PyArg_ParseTuple(_args, "O&llsl",
	                      CmpInstObj_Convert, &mh,
	                      &atMediaTime,
	                      &propertyType,
	                      &data,
	                      &dataSize))
		return NULL;
	_rv = TextMediaSetTextProperty(mh,
	                               atMediaTime,
	                               propertyType,
	                               data,
	                               dataSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TextMediaRawSetup(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	GWorldPtr gw;
	GDHandle gd;
	void * data;
	long dataSize;
	TextDescriptionHandle tdh;
	TimeValue sampleDuration;
#ifndef TextMediaRawSetup
	PyMac_PRECHECK(TextMediaRawSetup);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&slO&l",
	                      CmpInstObj_Convert, &mh,
	                      GWorldObj_Convert, &gw,
	                      OptResObj_Convert, &gd,
	                      &data,
	                      &dataSize,
	                      ResObj_Convert, &tdh,
	                      &sampleDuration))
		return NULL;
	_rv = TextMediaRawSetup(mh,
	                        gw,
	                        gd,
	                        data,
	                        dataSize,
	                        tdh,
	                        sampleDuration);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TextMediaRawIdle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	GWorldPtr gw;
	GDHandle gd;
	TimeValue sampleTime;
	long flagsIn;
	long flagsOut;
#ifndef TextMediaRawIdle
	PyMac_PRECHECK(TextMediaRawIdle);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&ll",
	                      CmpInstObj_Convert, &mh,
	                      GWorldObj_Convert, &gw,
	                      OptResObj_Convert, &gd,
	                      &sampleTime,
	                      &flagsIn))
		return NULL;
	_rv = TextMediaRawIdle(mh,
	                       gw,
	                       gd,
	                       sampleTime,
	                       flagsIn,
	                       &flagsOut);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     flagsOut);
	return _res;
}

static PyObject *Qt_TextMediaGetTextProperty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	TimeValue atMediaTime;
	long propertyType;
	void * data;
	long dataSize;
#ifndef TextMediaGetTextProperty
	PyMac_PRECHECK(TextMediaGetTextProperty);
#endif
	if (!PyArg_ParseTuple(_args, "O&llsl",
	                      CmpInstObj_Convert, &mh,
	                      &atMediaTime,
	                      &propertyType,
	                      &data,
	                      &dataSize))
		return NULL;
	_rv = TextMediaGetTextProperty(mh,
	                               atMediaTime,
	                               propertyType,
	                               data,
	                               dataSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TextMediaFindNextText(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Ptr text;
	long size;
	short findFlags;
	TimeValue startTime;
	TimeValue foundTime;
	TimeValue foundDuration;
	long offset;
#ifndef TextMediaFindNextText
	PyMac_PRECHECK(TextMediaFindNextText);
#endif
	if (!PyArg_ParseTuple(_args, "O&slhl",
	                      CmpInstObj_Convert, &mh,
	                      &text,
	                      &size,
	                      &findFlags,
	                      &startTime))
		return NULL;
	_rv = TextMediaFindNextText(mh,
	                            text,
	                            size,
	                            findFlags,
	                            startTime,
	                            &foundTime,
	                            &foundDuration,
	                            &offset);
	_res = Py_BuildValue("llll",
	                     _rv,
	                     foundTime,
	                     foundDuration,
	                     offset);
	return _res;
}

static PyObject *Qt_TextMediaHiliteTextSample(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	TimeValue sampleTime;
	short hiliteStart;
	short hiliteEnd;
	RGBColor rgbHiliteColor;
#ifndef TextMediaHiliteTextSample
	PyMac_PRECHECK(TextMediaHiliteTextSample);
#endif
	if (!PyArg_ParseTuple(_args, "O&lhh",
	                      CmpInstObj_Convert, &mh,
	                      &sampleTime,
	                      &hiliteStart,
	                      &hiliteEnd))
		return NULL;
	_rv = TextMediaHiliteTextSample(mh,
	                                sampleTime,
	                                hiliteStart,
	                                hiliteEnd,
	                                &rgbHiliteColor);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     QdRGB_New, &rgbHiliteColor);
	return _res;
}

static PyObject *Qt_TextMediaSetTextSampleData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	void * data;
	OSType dataType;
#ifndef TextMediaSetTextSampleData
	PyMac_PRECHECK(TextMediaSetTextSampleData);
#endif
	if (!PyArg_ParseTuple(_args, "O&sO&",
	                      CmpInstObj_Convert, &mh,
	                      &data,
	                      PyMac_GetOSType, &dataType))
		return NULL;
	_rv = TextMediaSetTextSampleData(mh,
	                                 data,
	                                 dataType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaSetProperty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short spriteIndex;
	long propertyType;
	void * propertyValue;
#ifndef SpriteMediaSetProperty
	PyMac_PRECHECK(SpriteMediaSetProperty);
#endif
	if (!PyArg_ParseTuple(_args, "O&hls",
	                      CmpInstObj_Convert, &mh,
	                      &spriteIndex,
	                      &propertyType,
	                      &propertyValue))
		return NULL;
	_rv = SpriteMediaSetProperty(mh,
	                             spriteIndex,
	                             propertyType,
	                             propertyValue);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaGetProperty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short spriteIndex;
	long propertyType;
	void * propertyValue;
#ifndef SpriteMediaGetProperty
	PyMac_PRECHECK(SpriteMediaGetProperty);
#endif
	if (!PyArg_ParseTuple(_args, "O&hls",
	                      CmpInstObj_Convert, &mh,
	                      &spriteIndex,
	                      &propertyType,
	                      &propertyValue))
		return NULL;
	_rv = SpriteMediaGetProperty(mh,
	                             spriteIndex,
	                             propertyType,
	                             propertyValue);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaHitTestSprites(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long flags;
	Point loc;
	short spriteHitIndex;
#ifndef SpriteMediaHitTestSprites
	PyMac_PRECHECK(SpriteMediaHitTestSprites);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      CmpInstObj_Convert, &mh,
	                      &flags,
	                      PyMac_GetPoint, &loc))
		return NULL;
	_rv = SpriteMediaHitTestSprites(mh,
	                                flags,
	                                loc,
	                                &spriteHitIndex);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     spriteHitIndex);
	return _res;
}

static PyObject *Qt_SpriteMediaCountSprites(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short numSprites;
#ifndef SpriteMediaCountSprites
	PyMac_PRECHECK(SpriteMediaCountSprites);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = SpriteMediaCountSprites(mh,
	                              &numSprites);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     numSprites);
	return _res;
}

static PyObject *Qt_SpriteMediaCountImages(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short numImages;
#ifndef SpriteMediaCountImages
	PyMac_PRECHECK(SpriteMediaCountImages);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = SpriteMediaCountImages(mh,
	                             &numImages);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     numImages);
	return _res;
}

static PyObject *Qt_SpriteMediaGetIndImageDescription(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short imageIndex;
	ImageDescriptionHandle imageDescription;
#ifndef SpriteMediaGetIndImageDescription
	PyMac_PRECHECK(SpriteMediaGetIndImageDescription);
#endif
	if (!PyArg_ParseTuple(_args, "O&hO&",
	                      CmpInstObj_Convert, &mh,
	                      &imageIndex,
	                      ResObj_Convert, &imageDescription))
		return NULL;
	_rv = SpriteMediaGetIndImageDescription(mh,
	                                        imageIndex,
	                                        imageDescription);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaGetDisplayedSampleNumber(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long sampleNum;
#ifndef SpriteMediaGetDisplayedSampleNumber
	PyMac_PRECHECK(SpriteMediaGetDisplayedSampleNumber);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = SpriteMediaGetDisplayedSampleNumber(mh,
	                                          &sampleNum);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     sampleNum);
	return _res;
}

static PyObject *Qt_SpriteMediaGetSpriteName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID spriteID;
	Str255 spriteName;
#ifndef SpriteMediaGetSpriteName
	PyMac_PRECHECK(SpriteMediaGetSpriteName);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      CmpInstObj_Convert, &mh,
	                      &spriteID,
	                      PyMac_GetStr255, spriteName))
		return NULL;
	_rv = SpriteMediaGetSpriteName(mh,
	                               spriteID,
	                               spriteName);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaGetImageName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short imageIndex;
	Str255 imageName;
#ifndef SpriteMediaGetImageName
	PyMac_PRECHECK(SpriteMediaGetImageName);
#endif
	if (!PyArg_ParseTuple(_args, "O&hO&",
	                      CmpInstObj_Convert, &mh,
	                      &imageIndex,
	                      PyMac_GetStr255, imageName))
		return NULL;
	_rv = SpriteMediaGetImageName(mh,
	                              imageIndex,
	                              imageName);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaSetSpriteProperty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID spriteID;
	long propertyType;
	void * propertyValue;
#ifndef SpriteMediaSetSpriteProperty
	PyMac_PRECHECK(SpriteMediaSetSpriteProperty);
#endif
	if (!PyArg_ParseTuple(_args, "O&lls",
	                      CmpInstObj_Convert, &mh,
	                      &spriteID,
	                      &propertyType,
	                      &propertyValue))
		return NULL;
	_rv = SpriteMediaSetSpriteProperty(mh,
	                                   spriteID,
	                                   propertyType,
	                                   propertyValue);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaGetSpriteProperty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID spriteID;
	long propertyType;
	void * propertyValue;
#ifndef SpriteMediaGetSpriteProperty
	PyMac_PRECHECK(SpriteMediaGetSpriteProperty);
#endif
	if (!PyArg_ParseTuple(_args, "O&lls",
	                      CmpInstObj_Convert, &mh,
	                      &spriteID,
	                      &propertyType,
	                      &propertyValue))
		return NULL;
	_rv = SpriteMediaGetSpriteProperty(mh,
	                                   spriteID,
	                                   propertyType,
	                                   propertyValue);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaHitTestAllSprites(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long flags;
	Point loc;
	QTAtomID spriteHitID;
#ifndef SpriteMediaHitTestAllSprites
	PyMac_PRECHECK(SpriteMediaHitTestAllSprites);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      CmpInstObj_Convert, &mh,
	                      &flags,
	                      PyMac_GetPoint, &loc))
		return NULL;
	_rv = SpriteMediaHitTestAllSprites(mh,
	                                   flags,
	                                   loc,
	                                   &spriteHitID);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     spriteHitID);
	return _res;
}

static PyObject *Qt_SpriteMediaHitTestOneSprite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID spriteID;
	long flags;
	Point loc;
	Boolean wasHit;
#ifndef SpriteMediaHitTestOneSprite
	PyMac_PRECHECK(SpriteMediaHitTestOneSprite);
#endif
	if (!PyArg_ParseTuple(_args, "O&llO&",
	                      CmpInstObj_Convert, &mh,
	                      &spriteID,
	                      &flags,
	                      PyMac_GetPoint, &loc))
		return NULL;
	_rv = SpriteMediaHitTestOneSprite(mh,
	                                  spriteID,
	                                  flags,
	                                  loc,
	                                  &wasHit);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     wasHit);
	return _res;
}

static PyObject *Qt_SpriteMediaSpriteIndexToID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short spriteIndex;
	QTAtomID spriteID;
#ifndef SpriteMediaSpriteIndexToID
	PyMac_PRECHECK(SpriteMediaSpriteIndexToID);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &mh,
	                      &spriteIndex))
		return NULL;
	_rv = SpriteMediaSpriteIndexToID(mh,
	                                 spriteIndex,
	                                 &spriteID);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     spriteID);
	return _res;
}

static PyObject *Qt_SpriteMediaSpriteIDToIndex(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID spriteID;
	short spriteIndex;
#ifndef SpriteMediaSpriteIDToIndex
	PyMac_PRECHECK(SpriteMediaSpriteIDToIndex);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &spriteID))
		return NULL;
	_rv = SpriteMediaSpriteIDToIndex(mh,
	                                 spriteID,
	                                 &spriteIndex);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     spriteIndex);
	return _res;
}

static PyObject *Qt_SpriteMediaSetActionVariable(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID variableID;
	float value;
#ifndef SpriteMediaSetActionVariable
	PyMac_PRECHECK(SpriteMediaSetActionVariable);
#endif
	if (!PyArg_ParseTuple(_args, "O&lf",
	                      CmpInstObj_Convert, &mh,
	                      &variableID,
	                      &value))
		return NULL;
	_rv = SpriteMediaSetActionVariable(mh,
	                                   variableID,
	                                   &value);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaGetActionVariable(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID variableID;
	float value;
#ifndef SpriteMediaGetActionVariable
	PyMac_PRECHECK(SpriteMediaGetActionVariable);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &variableID))
		return NULL;
	_rv = SpriteMediaGetActionVariable(mh,
	                                   variableID,
	                                   &value);
	_res = Py_BuildValue("lf",
	                     _rv,
	                     value);
	return _res;
}

static PyObject *Qt_SpriteMediaDisposeSprite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID spriteID;
#ifndef SpriteMediaDisposeSprite
	PyMac_PRECHECK(SpriteMediaDisposeSprite);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &spriteID))
		return NULL;
	_rv = SpriteMediaDisposeSprite(mh,
	                               spriteID);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaSetActionVariableToString(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID variableID;
	Ptr theCString;
#ifndef SpriteMediaSetActionVariableToString
	PyMac_PRECHECK(SpriteMediaSetActionVariableToString);
#endif
	if (!PyArg_ParseTuple(_args, "O&ls",
	                      CmpInstObj_Convert, &mh,
	                      &variableID,
	                      &theCString))
		return NULL;
	_rv = SpriteMediaSetActionVariableToString(mh,
	                                           variableID,
	                                           theCString);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaGetActionVariableAsString(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID variableID;
	Handle theCString;
#ifndef SpriteMediaGetActionVariableAsString
	PyMac_PRECHECK(SpriteMediaGetActionVariableAsString);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &variableID))
		return NULL;
	_rv = SpriteMediaGetActionVariableAsString(mh,
	                                           variableID,
	                                           &theCString);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, theCString);
	return _res;
}

static PyObject *Qt_SpriteMediaNewImage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Handle dataRef;
	OSType dataRefType;
	QTAtomID desiredID;
#ifndef SpriteMediaNewImage
	PyMac_PRECHECK(SpriteMediaNewImage);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&l",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      &desiredID))
		return NULL;
	_rv = SpriteMediaNewImage(mh,
	                          dataRef,
	                          dataRefType,
	                          desiredID);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaDisposeImage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short imageIndex;
#ifndef SpriteMediaDisposeImage
	PyMac_PRECHECK(SpriteMediaDisposeImage);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &mh,
	                      &imageIndex))
		return NULL;
	_rv = SpriteMediaDisposeImage(mh,
	                              imageIndex);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SpriteMediaImageIndexToID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short imageIndex;
	QTAtomID imageID;
#ifndef SpriteMediaImageIndexToID
	PyMac_PRECHECK(SpriteMediaImageIndexToID);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &mh,
	                      &imageIndex))
		return NULL;
	_rv = SpriteMediaImageIndexToID(mh,
	                                imageIndex,
	                                &imageID);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     imageID);
	return _res;
}

static PyObject *Qt_SpriteMediaImageIDToIndex(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID imageID;
	short imageIndex;
#ifndef SpriteMediaImageIDToIndex
	PyMac_PRECHECK(SpriteMediaImageIDToIndex);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &imageID))
		return NULL;
	_rv = SpriteMediaImageIDToIndex(mh,
	                                imageID,
	                                &imageIndex);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     imageIndex);
	return _res;
}

static PyObject *Qt_FlashMediaSetPan(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short xPercent;
	short yPercent;
#ifndef FlashMediaSetPan
	PyMac_PRECHECK(FlashMediaSetPan);
#endif
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      CmpInstObj_Convert, &mh,
	                      &xPercent,
	                      &yPercent))
		return NULL;
	_rv = FlashMediaSetPan(mh,
	                       xPercent,
	                       yPercent);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_FlashMediaSetZoom(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short factor;
#ifndef FlashMediaSetZoom
	PyMac_PRECHECK(FlashMediaSetZoom);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &mh,
	                      &factor))
		return NULL;
	_rv = FlashMediaSetZoom(mh,
	                        factor);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_FlashMediaSetZoomRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long left;
	long top;
	long right;
	long bottom;
#ifndef FlashMediaSetZoomRect
	PyMac_PRECHECK(FlashMediaSetZoomRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&llll",
	                      CmpInstObj_Convert, &mh,
	                      &left,
	                      &top,
	                      &right,
	                      &bottom))
		return NULL;
	_rv = FlashMediaSetZoomRect(mh,
	                            left,
	                            top,
	                            right,
	                            bottom);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_FlashMediaGetRefConBounds(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long refCon;
	long left;
	long top;
	long right;
	long bottom;
#ifndef FlashMediaGetRefConBounds
	PyMac_PRECHECK(FlashMediaGetRefConBounds);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &refCon))
		return NULL;
	_rv = FlashMediaGetRefConBounds(mh,
	                                refCon,
	                                &left,
	                                &top,
	                                &right,
	                                &bottom);
	_res = Py_BuildValue("lllll",
	                     _rv,
	                     left,
	                     top,
	                     right,
	                     bottom);
	return _res;
}

static PyObject *Qt_FlashMediaGetRefConID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long refCon;
	long refConID;
#ifndef FlashMediaGetRefConID
	PyMac_PRECHECK(FlashMediaGetRefConID);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &refCon))
		return NULL;
	_rv = FlashMediaGetRefConID(mh,
	                            refCon,
	                            &refConID);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     refConID);
	return _res;
}

static PyObject *Qt_FlashMediaIDToRefCon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long refConID;
	long refCon;
#ifndef FlashMediaIDToRefCon
	PyMac_PRECHECK(FlashMediaIDToRefCon);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &refConID))
		return NULL;
	_rv = FlashMediaIDToRefCon(mh,
	                           refConID,
	                           &refCon);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     refCon);
	return _res;
}

static PyObject *Qt_FlashMediaGetDisplayedFrameNumber(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long flashFrameNumber;
#ifndef FlashMediaGetDisplayedFrameNumber
	PyMac_PRECHECK(FlashMediaGetDisplayedFrameNumber);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = FlashMediaGetDisplayedFrameNumber(mh,
	                                        &flashFrameNumber);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     flashFrameNumber);
	return _res;
}

static PyObject *Qt_FlashMediaFrameNumberToMovieTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long flashFrameNumber;
	TimeValue movieTime;
#ifndef FlashMediaFrameNumberToMovieTime
	PyMac_PRECHECK(FlashMediaFrameNumberToMovieTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &flashFrameNumber))
		return NULL;
	_rv = FlashMediaFrameNumberToMovieTime(mh,
	                                       flashFrameNumber,
	                                       &movieTime);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     movieTime);
	return _res;
}

static PyObject *Qt_FlashMediaFrameLabelToMovieTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Ptr theLabel;
	TimeValue movieTime;
#ifndef FlashMediaFrameLabelToMovieTime
	PyMac_PRECHECK(FlashMediaFrameLabelToMovieTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &mh,
	                      &theLabel))
		return NULL;
	_rv = FlashMediaFrameLabelToMovieTime(mh,
	                                      theLabel,
	                                      &movieTime);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     movieTime);
	return _res;
}

static PyObject *Qt_FlashMediaGetFlashVariable(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	char path;
	char name;
	Handle theVariableCStringOut;
#ifndef FlashMediaGetFlashVariable
	PyMac_PRECHECK(FlashMediaGetFlashVariable);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = FlashMediaGetFlashVariable(mh,
	                                 &path,
	                                 &name,
	                                 &theVariableCStringOut);
	_res = Py_BuildValue("lccO&",
	                     _rv,
	                     path,
	                     name,
	                     ResObj_New, theVariableCStringOut);
	return _res;
}

static PyObject *Qt_FlashMediaSetFlashVariable(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	char path;
	char name;
	char value;
	Boolean updateFocus;
#ifndef FlashMediaSetFlashVariable
	PyMac_PRECHECK(FlashMediaSetFlashVariable);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &mh,
	                      &updateFocus))
		return NULL;
	_rv = FlashMediaSetFlashVariable(mh,
	                                 &path,
	                                 &name,
	                                 &value,
	                                 updateFocus);
	_res = Py_BuildValue("lccc",
	                     _rv,
	                     path,
	                     name,
	                     value);
	return _res;
}

static PyObject *Qt_FlashMediaDoButtonActions(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	char path;
	long buttonID;
	long transition;
#ifndef FlashMediaDoButtonActions
	PyMac_PRECHECK(FlashMediaDoButtonActions);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mh,
	                      &buttonID,
	                      &transition))
		return NULL;
	_rv = FlashMediaDoButtonActions(mh,
	                                &path,
	                                buttonID,
	                                transition);
	_res = Py_BuildValue("lc",
	                     _rv,
	                     path);
	return _res;
}

static PyObject *Qt_FlashMediaGetSupportedSwfVersion(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	UInt8 swfVersion;
#ifndef FlashMediaGetSupportedSwfVersion
	PyMac_PRECHECK(FlashMediaGetSupportedSwfVersion);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = FlashMediaGetSupportedSwfVersion(mh,
	                                       &swfVersion);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     swfVersion);
	return _res;
}

static PyObject *Qt_Media3DGetCurrentGroup(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	void * group;
#ifndef Media3DGetCurrentGroup
	PyMac_PRECHECK(Media3DGetCurrentGroup);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &mh,
	                      &group))
		return NULL;
	_rv = Media3DGetCurrentGroup(mh,
	                             group);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_Media3DTranslateNamedObjectTo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	char objectName;
	Fixed x;
	Fixed y;
	Fixed z;
#ifndef Media3DTranslateNamedObjectTo
	PyMac_PRECHECK(Media3DTranslateNamedObjectTo);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetFixed, &x,
	                      PyMac_GetFixed, &y,
	                      PyMac_GetFixed, &z))
		return NULL;
	_rv = Media3DTranslateNamedObjectTo(mh,
	                                    &objectName,
	                                    x,
	                                    y,
	                                    z);
	_res = Py_BuildValue("lc",
	                     _rv,
	                     objectName);
	return _res;
}

static PyObject *Qt_Media3DScaleNamedObjectTo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	char objectName;
	Fixed xScale;
	Fixed yScale;
	Fixed zScale;
#ifndef Media3DScaleNamedObjectTo
	PyMac_PRECHECK(Media3DScaleNamedObjectTo);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetFixed, &xScale,
	                      PyMac_GetFixed, &yScale,
	                      PyMac_GetFixed, &zScale))
		return NULL;
	_rv = Media3DScaleNamedObjectTo(mh,
	                                &objectName,
	                                xScale,
	                                yScale,
	                                zScale);
	_res = Py_BuildValue("lc",
	                     _rv,
	                     objectName);
	return _res;
}

static PyObject *Qt_Media3DRotateNamedObjectTo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	char objectName;
	Fixed xDegrees;
	Fixed yDegrees;
	Fixed zDegrees;
#ifndef Media3DRotateNamedObjectTo
	PyMac_PRECHECK(Media3DRotateNamedObjectTo);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetFixed, &xDegrees,
	                      PyMac_GetFixed, &yDegrees,
	                      PyMac_GetFixed, &zDegrees))
		return NULL;
	_rv = Media3DRotateNamedObjectTo(mh,
	                                 &objectName,
	                                 xDegrees,
	                                 yDegrees,
	                                 zDegrees);
	_res = Py_BuildValue("lc",
	                     _rv,
	                     objectName);
	return _res;
}

static PyObject *Qt_Media3DSetCameraData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	void * cameraData;
#ifndef Media3DSetCameraData
	PyMac_PRECHECK(Media3DSetCameraData);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &mh,
	                      &cameraData))
		return NULL;
	_rv = Media3DSetCameraData(mh,
	                           cameraData);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_Media3DGetCameraData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	void * cameraData;
#ifndef Media3DGetCameraData
	PyMac_PRECHECK(Media3DGetCameraData);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &mh,
	                      &cameraData))
		return NULL;
	_rv = Media3DGetCameraData(mh,
	                           cameraData);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_Media3DSetCameraAngleAspect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTFloatSingle fov;
	QTFloatSingle aspectRatioXToY;
#ifndef Media3DSetCameraAngleAspect
	PyMac_PRECHECK(Media3DSetCameraAngleAspect);
#endif
	if (!PyArg_ParseTuple(_args, "O&ff",
	                      CmpInstObj_Convert, &mh,
	                      &fov,
	                      &aspectRatioXToY))
		return NULL;
	_rv = Media3DSetCameraAngleAspect(mh,
	                                  fov,
	                                  aspectRatioXToY);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_Media3DGetCameraAngleAspect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTFloatSingle fov;
	QTFloatSingle aspectRatioXToY;
#ifndef Media3DGetCameraAngleAspect
	PyMac_PRECHECK(Media3DGetCameraAngleAspect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = Media3DGetCameraAngleAspect(mh,
	                                  &fov,
	                                  &aspectRatioXToY);
	_res = Py_BuildValue("lff",
	                     _rv,
	                     fov,
	                     aspectRatioXToY);
	return _res;
}

static PyObject *Qt_Media3DSetCameraRange(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	void * tQ3CameraRange;
#ifndef Media3DSetCameraRange
	PyMac_PRECHECK(Media3DSetCameraRange);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &mh,
	                      &tQ3CameraRange))
		return NULL;
	_rv = Media3DSetCameraRange(mh,
	                            tQ3CameraRange);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_Media3DGetCameraRange(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	void * tQ3CameraRange;
#ifndef Media3DGetCameraRange
	PyMac_PRECHECK(Media3DGetCameraRange);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &mh,
	                      &tQ3CameraRange))
		return NULL;
	_rv = Media3DGetCameraRange(mh,
	                            tQ3CameraRange);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_NewTimeBase(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeBase _rv;
#ifndef NewTimeBase
	PyMac_PRECHECK(NewTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = NewTimeBase();
	_res = Py_BuildValue("O&",
	                     TimeBaseObj_New, _rv);
	return _res;
}

static PyObject *Qt_ConvertTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeRecord theTime;
	TimeBase newBase;
#ifndef ConvertTime
	PyMac_PRECHECK(ConvertTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      QtTimeRecord_Convert, &theTime,
	                      TimeBaseObj_Convert, &newBase))
		return NULL;
	ConvertTime(&theTime,
	            newBase);
	_res = Py_BuildValue("O&",
	                     QtTimeRecord_New, &theTime);
	return _res;
}

static PyObject *Qt_ConvertTimeScale(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeRecord theTime;
	TimeScale newScale;
#ifndef ConvertTimeScale
	PyMac_PRECHECK(ConvertTimeScale);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      QtTimeRecord_Convert, &theTime,
	                      &newScale))
		return NULL;
	ConvertTimeScale(&theTime,
	                 newScale);
	_res = Py_BuildValue("O&",
	                     QtTimeRecord_New, &theTime);
	return _res;
}

static PyObject *Qt_AddTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeRecord dst;
	TimeRecord src;
#ifndef AddTime
	PyMac_PRECHECK(AddTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      QtTimeRecord_Convert, &dst,
	                      QtTimeRecord_Convert, &src))
		return NULL;
	AddTime(&dst,
	        &src);
	_res = Py_BuildValue("O&",
	                     QtTimeRecord_New, &dst);
	return _res;
}

static PyObject *Qt_SubtractTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TimeRecord dst;
	TimeRecord src;
#ifndef SubtractTime
	PyMac_PRECHECK(SubtractTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      QtTimeRecord_Convert, &dst,
	                      QtTimeRecord_Convert, &src))
		return NULL;
	SubtractTime(&dst,
	             &src);
	_res = Py_BuildValue("O&",
	                     QtTimeRecord_New, &dst);
	return _res;
}

static PyObject *Qt_MusicMediaGetIndexedTunePlayer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ti;
	long sampleDescIndex;
	ComponentInstance tp;
#ifndef MusicMediaGetIndexedTunePlayer
	PyMac_PRECHECK(MusicMediaGetIndexedTunePlayer);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ti,
	                      &sampleDescIndex))
		return NULL;
	_rv = MusicMediaGetIndexedTunePlayer(ti,
	                                     sampleDescIndex,
	                                     &tp);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     CmpInstObj_New, tp);
	return _res;
}

static PyObject *Qt_CodecManagerVersion(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long version;
#ifndef CodecManagerVersion
	PyMac_PRECHECK(CodecManagerVersion);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = CodecManagerVersion(&version);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     version);
	return _res;
}

static PyObject *Qt_GetMaxCompressionSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	PixMapHandle src;
	Rect srcRect;
	short colorDepth;
	CodecQ quality;
	CodecType cType;
	CompressorComponent codec;
	long size;
#ifndef GetMaxCompressionSize
	PyMac_PRECHECK(GetMaxCompressionSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&hlO&O&",
	                      ResObj_Convert, &src,
	                      PyMac_GetRect, &srcRect,
	                      &colorDepth,
	                      &quality,
	                      PyMac_GetOSType, &cType,
	                      CmpObj_Convert, &codec))
		return NULL;
	_err = GetMaxCompressionSize(src,
	                             &srcRect,
	                             colorDepth,
	                             quality,
	                             cType,
	                             codec,
	                             &size);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     size);
	return _res;
}

static PyObject *Qt_GetCompressionTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	PixMapHandle src;
	Rect srcRect;
	short colorDepth;
	CodecType cType;
	CompressorComponent codec;
	CodecQ spatialQuality;
	CodecQ temporalQuality;
	unsigned long compressTime;
#ifndef GetCompressionTime
	PyMac_PRECHECK(GetCompressionTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&hO&O&",
	                      ResObj_Convert, &src,
	                      PyMac_GetRect, &srcRect,
	                      &colorDepth,
	                      PyMac_GetOSType, &cType,
	                      CmpObj_Convert, &codec))
		return NULL;
	_err = GetCompressionTime(src,
	                          &srcRect,
	                          colorDepth,
	                          cType,
	                          codec,
	                          &spatialQuality,
	                          &temporalQuality,
	                          &compressTime);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("lll",
	                     spatialQuality,
	                     temporalQuality,
	                     compressTime);
	return _res;
}

static PyObject *Qt_CompressImage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	PixMapHandle src;
	Rect srcRect;
	CodecQ quality;
	CodecType cType;
	ImageDescriptionHandle desc;
	Ptr data;
#ifndef CompressImage
	PyMac_PRECHECK(CompressImage);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&lO&O&s",
	                      ResObj_Convert, &src,
	                      PyMac_GetRect, &srcRect,
	                      &quality,
	                      PyMac_GetOSType, &cType,
	                      ResObj_Convert, &desc,
	                      &data))
		return NULL;
	_err = CompressImage(src,
	                     &srcRect,
	                     quality,
	                     cType,
	                     desc,
	                     data);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_DecompressImage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Ptr data;
	ImageDescriptionHandle desc;
	PixMapHandle dst;
	Rect srcRect;
	Rect dstRect;
	short mode;
	RgnHandle mask;
#ifndef DecompressImage
	PyMac_PRECHECK(DecompressImage);
#endif
	if (!PyArg_ParseTuple(_args, "sO&O&O&O&hO&",
	                      &data,
	                      ResObj_Convert, &desc,
	                      ResObj_Convert, &dst,
	                      PyMac_GetRect, &srcRect,
	                      PyMac_GetRect, &dstRect,
	                      &mode,
	                      ResObj_Convert, &mask))
		return NULL;
	_err = DecompressImage(data,
	                       desc,
	                       dst,
	                       &srcRect,
	                       &dstRect,
	                       mode,
	                       mask);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_GetSimilarity(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	PixMapHandle src;
	Rect srcRect;
	ImageDescriptionHandle desc;
	Ptr data;
	Fixed similarity;
#ifndef GetSimilarity
	PyMac_PRECHECK(GetSimilarity);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&s",
	                      ResObj_Convert, &src,
	                      PyMac_GetRect, &srcRect,
	                      ResObj_Convert, &desc,
	                      &data))
		return NULL;
	_err = GetSimilarity(src,
	                     &srcRect,
	                     desc,
	                     data,
	                     &similarity);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, similarity);
	return _res;
}

static PyObject *Qt_GetImageDescriptionCTable(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ImageDescriptionHandle desc;
	CTabHandle ctable;
#ifndef GetImageDescriptionCTable
	PyMac_PRECHECK(GetImageDescriptionCTable);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &desc))
		return NULL;
	_err = GetImageDescriptionCTable(desc,
	                                 &ctable);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, ctable);
	return _res;
}

static PyObject *Qt_SetImageDescriptionCTable(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ImageDescriptionHandle desc;
	CTabHandle ctable;
#ifndef SetImageDescriptionCTable
	PyMac_PRECHECK(SetImageDescriptionCTable);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &desc,
	                      ResObj_Convert, &ctable))
		return NULL;
	_err = SetImageDescriptionCTable(desc,
	                                 ctable);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_GetImageDescriptionExtension(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ImageDescriptionHandle desc;
	Handle extension;
	long idType;
	long index;
#ifndef GetImageDescriptionExtension
	PyMac_PRECHECK(GetImageDescriptionExtension);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      ResObj_Convert, &desc,
	                      &idType,
	                      &index))
		return NULL;
	_err = GetImageDescriptionExtension(desc,
	                                    &extension,
	                                    idType,
	                                    index);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, extension);
	return _res;
}

static PyObject *Qt_AddImageDescriptionExtension(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ImageDescriptionHandle desc;
	Handle extension;
	long idType;
#ifndef AddImageDescriptionExtension
	PyMac_PRECHECK(AddImageDescriptionExtension);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      ResObj_Convert, &desc,
	                      ResObj_Convert, &extension,
	                      &idType))
		return NULL;
	_err = AddImageDescriptionExtension(desc,
	                                    extension,
	                                    idType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_RemoveImageDescriptionExtension(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ImageDescriptionHandle desc;
	long idType;
	long index;
#ifndef RemoveImageDescriptionExtension
	PyMac_PRECHECK(RemoveImageDescriptionExtension);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      ResObj_Convert, &desc,
	                      &idType,
	                      &index))
		return NULL;
	_err = RemoveImageDescriptionExtension(desc,
	                                       idType,
	                                       index);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_CountImageDescriptionExtensionType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ImageDescriptionHandle desc;
	long idType;
	long count;
#ifndef CountImageDescriptionExtensionType
	PyMac_PRECHECK(CountImageDescriptionExtensionType);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      ResObj_Convert, &desc,
	                      &idType))
		return NULL;
	_err = CountImageDescriptionExtensionType(desc,
	                                          idType,
	                                          &count);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     count);
	return _res;
}

static PyObject *Qt_GetNextImageDescriptionExtensionType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ImageDescriptionHandle desc;
	long idType;
#ifndef GetNextImageDescriptionExtensionType
	PyMac_PRECHECK(GetNextImageDescriptionExtensionType);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &desc))
		return NULL;
	_err = GetNextImageDescriptionExtensionType(desc,
	                                            &idType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     idType);
	return _res;
}

static PyObject *Qt_FindCodec(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	CodecType cType;
	CodecComponent specCodec;
	CompressorComponent compressor;
	DecompressorComponent decompressor;
#ifndef FindCodec
	PyMac_PRECHECK(FindCodec);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &cType,
	                      CmpObj_Convert, &specCodec))
		return NULL;
	_err = FindCodec(cType,
	                 specCodec,
	                 &compressor,
	                 &decompressor);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     CmpObj_New, compressor,
	                     CmpObj_New, decompressor);
	return _res;
}

static PyObject *Qt_CompressPicture(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	PicHandle srcPicture;
	PicHandle dstPicture;
	CodecQ quality;
	CodecType cType;
#ifndef CompressPicture
	PyMac_PRECHECK(CompressPicture);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&lO&",
	                      ResObj_Convert, &srcPicture,
	                      ResObj_Convert, &dstPicture,
	                      &quality,
	                      PyMac_GetOSType, &cType))
		return NULL;
	_err = CompressPicture(srcPicture,
	                       dstPicture,
	                       quality,
	                       cType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_CompressPictureFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short srcRefNum;
	short dstRefNum;
	CodecQ quality;
	CodecType cType;
#ifndef CompressPictureFile
	PyMac_PRECHECK(CompressPictureFile);
#endif
	if (!PyArg_ParseTuple(_args, "hhlO&",
	                      &srcRefNum,
	                      &dstRefNum,
	                      &quality,
	                      PyMac_GetOSType, &cType))
		return NULL;
	_err = CompressPictureFile(srcRefNum,
	                           dstRefNum,
	                           quality,
	                           cType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_ConvertImage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ImageDescriptionHandle srcDD;
	Ptr srcData;
	short colorDepth;
	CTabHandle ctable;
	CodecQ accuracy;
	CodecQ quality;
	CodecType cType;
	CodecComponent codec;
	ImageDescriptionHandle dstDD;
	Ptr dstData;
#ifndef ConvertImage
	PyMac_PRECHECK(ConvertImage);
#endif
	if (!PyArg_ParseTuple(_args, "O&shO&llO&O&O&s",
	                      ResObj_Convert, &srcDD,
	                      &srcData,
	                      &colorDepth,
	                      ResObj_Convert, &ctable,
	                      &accuracy,
	                      &quality,
	                      PyMac_GetOSType, &cType,
	                      CmpObj_Convert, &codec,
	                      ResObj_Convert, &dstDD,
	                      &dstData))
		return NULL;
	_err = ConvertImage(srcDD,
	                    srcData,
	                    colorDepth,
	                    ctable,
	                    accuracy,
	                    quality,
	                    cType,
	                    codec,
	                    dstDD,
	                    dstData);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_AddFilePreview(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short resRefNum;
	OSType previewType;
	Handle previewData;
#ifndef AddFilePreview
	PyMac_PRECHECK(AddFilePreview);
#endif
	if (!PyArg_ParseTuple(_args, "hO&O&",
	                      &resRefNum,
	                      PyMac_GetOSType, &previewType,
	                      ResObj_Convert, &previewData))
		return NULL;
	_err = AddFilePreview(resRefNum,
	                      previewType,
	                      previewData);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_GetBestDeviceRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	GDHandle gdh;
	Rect rp;
#ifndef GetBestDeviceRect
	PyMac_PRECHECK(GetBestDeviceRect);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetBestDeviceRect(&gdh,
	                         &rp);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     OptResObj_New, gdh,
	                     PyMac_BuildRect, &rp);
	return _res;
}

static PyObject *Qt_GDHasScale(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	GDHandle gdh;
	short depth;
	Fixed scale;
#ifndef GDHasScale
	PyMac_PRECHECK(GDHasScale);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      OptResObj_Convert, &gdh,
	                      &depth))
		return NULL;
	_err = GDHasScale(gdh,
	                  depth,
	                  &scale);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, scale);
	return _res;
}

static PyObject *Qt_GDGetScale(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	GDHandle gdh;
	Fixed scale;
	short flags;
#ifndef GDGetScale
	PyMac_PRECHECK(GDGetScale);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      OptResObj_Convert, &gdh))
		return NULL;
	_err = GDGetScale(gdh,
	                  &scale,
	                  &flags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&h",
	                     PyMac_BuildFixed, scale,
	                     flags);
	return _res;
}

static PyObject *Qt_GDSetScale(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	GDHandle gdh;
	Fixed scale;
	short flags;
#ifndef GDSetScale
	PyMac_PRECHECK(GDSetScale);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      OptResObj_Convert, &gdh,
	                      PyMac_GetFixed, &scale,
	                      &flags))
		return NULL;
	_err = GDSetScale(gdh,
	                  scale,
	                  flags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_GetGraphicsImporterForFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec theFile;
	ComponentInstance gi;
#ifndef GetGraphicsImporterForFile
	PyMac_PRECHECK(GetGraphicsImporterForFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &theFile))
		return NULL;
	_err = GetGraphicsImporterForFile(&theFile,
	                                  &gi);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, gi);
	return _res;
}

static PyObject *Qt_GetGraphicsImporterForDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataRef;
	OSType dataRefType;
	ComponentInstance gi;
#ifndef GetGraphicsImporterForDataRef
	PyMac_PRECHECK(GetGraphicsImporterForDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_err = GetGraphicsImporterForDataRef(dataRef,
	                                     dataRefType,
	                                     &gi);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, gi);
	return _res;
}

static PyObject *Qt_GetGraphicsImporterForFileWithFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec theFile;
	ComponentInstance gi;
	long flags;
#ifndef GetGraphicsImporterForFileWithFlags
	PyMac_PRECHECK(GetGraphicsImporterForFileWithFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetFSSpec, &theFile,
	                      &flags))
		return NULL;
	_err = GetGraphicsImporterForFileWithFlags(&theFile,
	                                           &gi,
	                                           flags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, gi);
	return _res;
}

static PyObject *Qt_GetGraphicsImporterForDataRefWithFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataRef;
	OSType dataRefType;
	ComponentInstance gi;
	long flags;
#ifndef GetGraphicsImporterForDataRefWithFlags
	PyMac_PRECHECK(GetGraphicsImporterForDataRefWithFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      &flags))
		return NULL;
	_err = GetGraphicsImporterForDataRefWithFlags(dataRef,
	                                              dataRefType,
	                                              &gi,
	                                              flags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, gi);
	return _res;
}

static PyObject *Qt_MakeImageDescriptionForPixMap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	PixMapHandle pixmap;
	ImageDescriptionHandle idh;
#ifndef MakeImageDescriptionForPixMap
	PyMac_PRECHECK(MakeImageDescriptionForPixMap);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pixmap))
		return NULL;
	_err = MakeImageDescriptionForPixMap(pixmap,
	                                     &idh);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, idh);
	return _res;
}

static PyObject *Qt_MakeImageDescriptionForEffect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType effectType;
	ImageDescriptionHandle idh;
#ifndef MakeImageDescriptionForEffect
	PyMac_PRECHECK(MakeImageDescriptionForEffect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &effectType))
		return NULL;
	_err = MakeImageDescriptionForEffect(effectType,
	                                     &idh);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, idh);
	return _res;
}

static PyObject *Qt_QTGetPixelSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
	OSType PixelFormat;
#ifndef QTGetPixelSize
	PyMac_PRECHECK(QTGetPixelSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &PixelFormat))
		return NULL;
	_rv = QTGetPixelSize(PixelFormat);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTGetPixelFormatDepthForImageDescription(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
	OSType PixelFormat;
#ifndef QTGetPixelFormatDepthForImageDescription
	PyMac_PRECHECK(QTGetPixelFormatDepthForImageDescription);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &PixelFormat))
		return NULL;
	_rv = QTGetPixelFormatDepthForImageDescription(PixelFormat);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTGetPixMapHandleRowBytes(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	PixMapHandle pm;
#ifndef QTGetPixMapHandleRowBytes
	PyMac_PRECHECK(QTGetPixMapHandleRowBytes);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pm))
		return NULL;
	_rv = QTGetPixMapHandleRowBytes(pm);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTSetPixMapHandleRowBytes(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	PixMapHandle pm;
	long rowBytes;
#ifndef QTSetPixMapHandleRowBytes
	PyMac_PRECHECK(QTSetPixMapHandleRowBytes);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      ResObj_Convert, &pm,
	                      &rowBytes))
		return NULL;
	_err = QTSetPixMapHandleRowBytes(pm,
	                                 rowBytes);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_QTGetPixMapHandleGammaLevel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
	PixMapHandle pm;
#ifndef QTGetPixMapHandleGammaLevel
	PyMac_PRECHECK(QTGetPixMapHandleGammaLevel);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pm))
		return NULL;
	_rv = QTGetPixMapHandleGammaLevel(pm);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyObject *Qt_QTSetPixMapHandleGammaLevel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	PixMapHandle pm;
	Fixed gammaLevel;
#ifndef QTSetPixMapHandleGammaLevel
	PyMac_PRECHECK(QTSetPixMapHandleGammaLevel);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &pm,
	                      PyMac_GetFixed, &gammaLevel))
		return NULL;
	_err = QTSetPixMapHandleGammaLevel(pm,
	                                   gammaLevel);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_QTGetPixMapHandleRequestedGammaLevel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
	PixMapHandle pm;
#ifndef QTGetPixMapHandleRequestedGammaLevel
	PyMac_PRECHECK(QTGetPixMapHandleRequestedGammaLevel);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pm))
		return NULL;
	_rv = QTGetPixMapHandleRequestedGammaLevel(pm);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyObject *Qt_QTSetPixMapHandleRequestedGammaLevel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	PixMapHandle pm;
	Fixed requestedGammaLevel;
#ifndef QTSetPixMapHandleRequestedGammaLevel
	PyMac_PRECHECK(QTSetPixMapHandleRequestedGammaLevel);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &pm,
	                      PyMac_GetFixed, &requestedGammaLevel))
		return NULL;
	_err = QTSetPixMapHandleRequestedGammaLevel(pm,
	                                            requestedGammaLevel);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_CompAdd(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	wide src;
	wide dst;
#ifndef CompAdd
	PyMac_PRECHECK(CompAdd);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CompAdd(&src,
	        &dst);
	_res = Py_BuildValue("O&O&",
	                     PyMac_Buildwide, src,
	                     PyMac_Buildwide, dst);
	return _res;
}

static PyObject *Qt_CompSub(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	wide src;
	wide dst;
#ifndef CompSub
	PyMac_PRECHECK(CompSub);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CompSub(&src,
	        &dst);
	_res = Py_BuildValue("O&O&",
	                     PyMac_Buildwide, src,
	                     PyMac_Buildwide, dst);
	return _res;
}

static PyObject *Qt_CompNeg(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	wide dst;
#ifndef CompNeg
	PyMac_PRECHECK(CompNeg);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CompNeg(&dst);
	_res = Py_BuildValue("O&",
	                     PyMac_Buildwide, dst);
	return _res;
}

static PyObject *Qt_CompShift(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	wide src;
	short shift;
#ifndef CompShift
	PyMac_PRECHECK(CompShift);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &shift))
		return NULL;
	CompShift(&src,
	          shift);
	_res = Py_BuildValue("O&",
	                     PyMac_Buildwide, src);
	return _res;
}

static PyObject *Qt_CompMul(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long src1;
	long src2;
	wide dst;
#ifndef CompMul
	PyMac_PRECHECK(CompMul);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &src1,
	                      &src2))
		return NULL;
	CompMul(src1,
	        src2,
	        &dst);
	_res = Py_BuildValue("O&",
	                     PyMac_Buildwide, dst);
	return _res;
}

static PyObject *Qt_CompDiv(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	wide numerator;
	long denominator;
	long remainder;
#ifndef CompDiv
	PyMac_PRECHECK(CompDiv);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &denominator))
		return NULL;
	_rv = CompDiv(&numerator,
	              denominator,
	              &remainder);
	_res = Py_BuildValue("lO&l",
	                     _rv,
	                     PyMac_Buildwide, numerator,
	                     remainder);
	return _res;
}

static PyObject *Qt_CompFixMul(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	wide compSrc;
	Fixed fixSrc;
	wide compDst;
#ifndef CompFixMul
	PyMac_PRECHECK(CompFixMul);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFixed, &fixSrc))
		return NULL;
	CompFixMul(&compSrc,
	           fixSrc,
	           &compDst);
	_res = Py_BuildValue("O&O&",
	                     PyMac_Buildwide, compSrc,
	                     PyMac_Buildwide, compDst);
	return _res;
}

static PyObject *Qt_CompMulDiv(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	wide co;
	long mul;
	long divisor;
#ifndef CompMulDiv
	PyMac_PRECHECK(CompMulDiv);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &mul,
	                      &divisor))
		return NULL;
	CompMulDiv(&co,
	           mul,
	           divisor);
	_res = Py_BuildValue("O&",
	                     PyMac_Buildwide, co);
	return _res;
}

static PyObject *Qt_CompMulDivTrunc(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	wide co;
	long mul;
	long divisor;
	long remainder;
#ifndef CompMulDivTrunc
	PyMac_PRECHECK(CompMulDivTrunc);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &mul,
	                      &divisor))
		return NULL;
	CompMulDivTrunc(&co,
	                mul,
	                divisor,
	                &remainder);
	_res = Py_BuildValue("O&l",
	                     PyMac_Buildwide, co,
	                     remainder);
	return _res;
}

static PyObject *Qt_CompCompare(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	wide a;
	wide minusb;
#ifndef CompCompare
	PyMac_PRECHECK(CompCompare);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_Getwide, &a,
	                      PyMac_Getwide, &minusb))
		return NULL;
	_rv = CompCompare(&a,
	                  &minusb);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_CompSquareRoot(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	unsigned long _rv;
	wide src;
#ifndef CompSquareRoot
	PyMac_PRECHECK(CompSquareRoot);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_Getwide, &src))
		return NULL;
	_rv = CompSquareRoot(&src);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_FixMulDiv(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
	Fixed src;
	Fixed mul;
	Fixed divisor;
#ifndef FixMulDiv
	PyMac_PRECHECK(FixMulDiv);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      PyMac_GetFixed, &src,
	                      PyMac_GetFixed, &mul,
	                      PyMac_GetFixed, &divisor))
		return NULL;
	_rv = FixMulDiv(src,
	                mul,
	                divisor);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyObject *Qt_UnsignedFixMulDiv(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
	Fixed src;
	Fixed mul;
	Fixed divisor;
#ifndef UnsignedFixMulDiv
	PyMac_PRECHECK(UnsignedFixMulDiv);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      PyMac_GetFixed, &src,
	                      PyMac_GetFixed, &mul,
	                      PyMac_GetFixed, &divisor))
		return NULL;
	_rv = UnsignedFixMulDiv(src,
	                        mul,
	                        divisor);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyObject *Qt_FixExp2(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
	Fixed src;
#ifndef FixExp2
	PyMac_PRECHECK(FixExp2);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFixed, &src))
		return NULL;
	_rv = FixExp2(src);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyObject *Qt_FixLog2(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
	Fixed src;
#ifndef FixLog2
	PyMac_PRECHECK(FixLog2);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFixed, &src))
		return NULL;
	_rv = FixLog2(src);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyObject *Qt_FixPow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
	Fixed base;
	Fixed exp;
#ifndef FixPow
	PyMac_PRECHECK(FixPow);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFixed, &base,
	                      PyMac_GetFixed, &exp))
		return NULL;
	_rv = FixPow(base,
	             exp);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportSetDataReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Handle dataRef;
	OSType dataReType;
#ifndef GraphicsImportSetDataReference
	PyMac_PRECHECK(GraphicsImportSetDataReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataReType))
		return NULL;
	_rv = GraphicsImportSetDataReference(ci,
	                                     dataRef,
	                                     dataReType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetDataReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Handle dataRef;
	OSType dataReType;
#ifndef GraphicsImportGetDataReference
	PyMac_PRECHECK(GraphicsImportGetDataReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetDataReference(ci,
	                                     &dataRef,
	                                     &dataReType);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, dataRef,
	                     PyMac_BuildOSType, dataReType);
	return _res;
}

static PyObject *Qt_GraphicsImportSetDataFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	FSSpec theFile;
#ifndef GraphicsImportSetDataFile
	PyMac_PRECHECK(GraphicsImportSetDataFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFSSpec, &theFile))
		return NULL;
	_rv = GraphicsImportSetDataFile(ci,
	                                &theFile);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetDataFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	FSSpec theFile;
#ifndef GraphicsImportGetDataFile
	PyMac_PRECHECK(GraphicsImportGetDataFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFSSpec, &theFile))
		return NULL;
	_rv = GraphicsImportGetDataFile(ci,
	                                &theFile);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportSetDataHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Handle h;
#ifndef GraphicsImportSetDataHandle
	PyMac_PRECHECK(GraphicsImportSetDataHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &h))
		return NULL;
	_rv = GraphicsImportSetDataHandle(ci,
	                                  h);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetDataHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Handle h;
#ifndef GraphicsImportGetDataHandle
	PyMac_PRECHECK(GraphicsImportGetDataHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetDataHandle(ci,
	                                  &h);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, h);
	return _res;
}

static PyObject *Qt_GraphicsImportGetImageDescription(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	ImageDescriptionHandle desc;
#ifndef GraphicsImportGetImageDescription
	PyMac_PRECHECK(GraphicsImportGetImageDescription);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetImageDescription(ci,
	                                        &desc);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, desc);
	return _res;
}

static PyObject *Qt_GraphicsImportGetDataOffsetAndSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	unsigned long offset;
	unsigned long size;
#ifndef GraphicsImportGetDataOffsetAndSize
	PyMac_PRECHECK(GraphicsImportGetDataOffsetAndSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetDataOffsetAndSize(ci,
	                                         &offset,
	                                         &size);
	_res = Py_BuildValue("lll",
	                     _rv,
	                     offset,
	                     size);
	return _res;
}

static PyObject *Qt_GraphicsImportReadData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	void * dataPtr;
	unsigned long dataOffset;
	unsigned long dataSize;
#ifndef GraphicsImportReadData
	PyMac_PRECHECK(GraphicsImportReadData);
#endif
	if (!PyArg_ParseTuple(_args, "O&sll",
	                      CmpInstObj_Convert, &ci,
	                      &dataPtr,
	                      &dataOffset,
	                      &dataSize))
		return NULL;
	_rv = GraphicsImportReadData(ci,
	                             dataPtr,
	                             dataOffset,
	                             dataSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportSetClip(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	RgnHandle clipRgn;
#ifndef GraphicsImportSetClip
	PyMac_PRECHECK(GraphicsImportSetClip);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &clipRgn))
		return NULL;
	_rv = GraphicsImportSetClip(ci,
	                            clipRgn);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetClip(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	RgnHandle clipRgn;
#ifndef GraphicsImportGetClip
	PyMac_PRECHECK(GraphicsImportGetClip);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetClip(ci,
	                            &clipRgn);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, clipRgn);
	return _res;
}

static PyObject *Qt_GraphicsImportSetSourceRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Rect sourceRect;
#ifndef GraphicsImportSetSourceRect
	PyMac_PRECHECK(GraphicsImportSetSourceRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetRect, &sourceRect))
		return NULL;
	_rv = GraphicsImportSetSourceRect(ci,
	                                  &sourceRect);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetSourceRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Rect sourceRect;
#ifndef GraphicsImportGetSourceRect
	PyMac_PRECHECK(GraphicsImportGetSourceRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetSourceRect(ci,
	                                  &sourceRect);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &sourceRect);
	return _res;
}

static PyObject *Qt_GraphicsImportGetNaturalBounds(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Rect naturalBounds;
#ifndef GraphicsImportGetNaturalBounds
	PyMac_PRECHECK(GraphicsImportGetNaturalBounds);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetNaturalBounds(ci,
	                                     &naturalBounds);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &naturalBounds);
	return _res;
}

static PyObject *Qt_GraphicsImportDraw(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
#ifndef GraphicsImportDraw
	PyMac_PRECHECK(GraphicsImportDraw);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportDraw(ci);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportSetGWorld(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	CGrafPtr port;
	GDHandle gd;
#ifndef GraphicsImportSetGWorld
	PyMac_PRECHECK(GraphicsImportSetGWorld);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      GrafObj_Convert, &port,
	                      OptResObj_Convert, &gd))
		return NULL;
	_rv = GraphicsImportSetGWorld(ci,
	                              port,
	                              gd);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetGWorld(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	CGrafPtr port;
	GDHandle gd;
#ifndef GraphicsImportGetGWorld
	PyMac_PRECHECK(GraphicsImportGetGWorld);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetGWorld(ci,
	                              &port,
	                              &gd);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     GrafObj_New, port,
	                     OptResObj_New, gd);
	return _res;
}

static PyObject *Qt_GraphicsImportSetBoundsRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Rect bounds;
#ifndef GraphicsImportSetBoundsRect
	PyMac_PRECHECK(GraphicsImportSetBoundsRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetRect, &bounds))
		return NULL;
	_rv = GraphicsImportSetBoundsRect(ci,
	                                  &bounds);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetBoundsRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Rect bounds;
#ifndef GraphicsImportGetBoundsRect
	PyMac_PRECHECK(GraphicsImportGetBoundsRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetBoundsRect(ci,
	                                  &bounds);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &bounds);
	return _res;
}

static PyObject *Qt_GraphicsImportSaveAsPicture(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	FSSpec fss;
	ScriptCode scriptTag;
#ifndef GraphicsImportSaveAsPicture
	PyMac_PRECHECK(GraphicsImportSaveAsPicture);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFSSpec, &fss,
	                      &scriptTag))
		return NULL;
	_rv = GraphicsImportSaveAsPicture(ci,
	                                  &fss,
	                                  scriptTag);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportSetGraphicsMode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	long graphicsMode;
	RGBColor opColor;
#ifndef GraphicsImportSetGraphicsMode
	PyMac_PRECHECK(GraphicsImportSetGraphicsMode);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      CmpInstObj_Convert, &ci,
	                      &graphicsMode,
	                      QdRGB_Convert, &opColor))
		return NULL;
	_rv = GraphicsImportSetGraphicsMode(ci,
	                                    graphicsMode,
	                                    &opColor);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetGraphicsMode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	long graphicsMode;
	RGBColor opColor;
#ifndef GraphicsImportGetGraphicsMode
	PyMac_PRECHECK(GraphicsImportGetGraphicsMode);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetGraphicsMode(ci,
	                                    &graphicsMode,
	                                    &opColor);
	_res = Py_BuildValue("llO&",
	                     _rv,
	                     graphicsMode,
	                     QdRGB_New, &opColor);
	return _res;
}

static PyObject *Qt_GraphicsImportSetQuality(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	CodecQ quality;
#ifndef GraphicsImportSetQuality
	PyMac_PRECHECK(GraphicsImportSetQuality);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &quality))
		return NULL;
	_rv = GraphicsImportSetQuality(ci,
	                               quality);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetQuality(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	CodecQ quality;
#ifndef GraphicsImportGetQuality
	PyMac_PRECHECK(GraphicsImportGetQuality);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetQuality(ci,
	                               &quality);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     quality);
	return _res;
}

static PyObject *Qt_GraphicsImportSaveAsQuickTimeImageFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	FSSpec fss;
	ScriptCode scriptTag;
#ifndef GraphicsImportSaveAsQuickTimeImageFile
	PyMac_PRECHECK(GraphicsImportSaveAsQuickTimeImageFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFSSpec, &fss,
	                      &scriptTag))
		return NULL;
	_rv = GraphicsImportSaveAsQuickTimeImageFile(ci,
	                                             &fss,
	                                             scriptTag);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportSetDataReferenceOffsetAndLimit(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	unsigned long offset;
	unsigned long limit;
#ifndef GraphicsImportSetDataReferenceOffsetAndLimit
	PyMac_PRECHECK(GraphicsImportSetDataReferenceOffsetAndLimit);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &ci,
	                      &offset,
	                      &limit))
		return NULL;
	_rv = GraphicsImportSetDataReferenceOffsetAndLimit(ci,
	                                                   offset,
	                                                   limit);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetDataReferenceOffsetAndLimit(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	unsigned long offset;
	unsigned long limit;
#ifndef GraphicsImportGetDataReferenceOffsetAndLimit
	PyMac_PRECHECK(GraphicsImportGetDataReferenceOffsetAndLimit);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetDataReferenceOffsetAndLimit(ci,
	                                                   &offset,
	                                                   &limit);
	_res = Py_BuildValue("lll",
	                     _rv,
	                     offset,
	                     limit);
	return _res;
}

static PyObject *Qt_GraphicsImportGetAliasedDataReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Handle dataRef;
	OSType dataRefType;
#ifndef GraphicsImportGetAliasedDataReference
	PyMac_PRECHECK(GraphicsImportGetAliasedDataReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetAliasedDataReference(ci,
	                                            &dataRef,
	                                            &dataRefType);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, dataRef,
	                     PyMac_BuildOSType, dataRefType);
	return _res;
}

static PyObject *Qt_GraphicsImportValidate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Boolean valid;
#ifndef GraphicsImportValidate
	PyMac_PRECHECK(GraphicsImportValidate);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportValidate(ci,
	                             &valid);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     valid);
	return _res;
}

static PyObject *Qt_GraphicsImportGetMetaData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	void * userData;
#ifndef GraphicsImportGetMetaData
	PyMac_PRECHECK(GraphicsImportGetMetaData);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &ci,
	                      &userData))
		return NULL;
	_rv = GraphicsImportGetMetaData(ci,
	                                userData);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetMIMETypeList(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	void * qtAtomContainerPtr;
#ifndef GraphicsImportGetMIMETypeList
	PyMac_PRECHECK(GraphicsImportGetMIMETypeList);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &ci,
	                      &qtAtomContainerPtr))
		return NULL;
	_rv = GraphicsImportGetMIMETypeList(ci,
	                                    qtAtomContainerPtr);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportDoesDrawAllPixels(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	short drawsAllPixels;
#ifndef GraphicsImportDoesDrawAllPixels
	PyMac_PRECHECK(GraphicsImportDoesDrawAllPixels);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportDoesDrawAllPixels(ci,
	                                      &drawsAllPixels);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     drawsAllPixels);
	return _res;
}

static PyObject *Qt_GraphicsImportGetAsPicture(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	PicHandle picture;
#ifndef GraphicsImportGetAsPicture
	PyMac_PRECHECK(GraphicsImportGetAsPicture);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetAsPicture(ci,
	                                 &picture);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, picture);
	return _res;
}

static PyObject *Qt_GraphicsImportExportImageFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	OSType fileType;
	OSType fileCreator;
	FSSpec fss;
	ScriptCode scriptTag;
#ifndef GraphicsImportExportImageFile
	PyMac_PRECHECK(GraphicsImportExportImageFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&h",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetOSType, &fileType,
	                      PyMac_GetOSType, &fileCreator,
	                      PyMac_GetFSSpec, &fss,
	                      &scriptTag))
		return NULL;
	_rv = GraphicsImportExportImageFile(ci,
	                                    fileType,
	                                    fileCreator,
	                                    &fss,
	                                    scriptTag);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetExportImageTypeList(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	void * qtAtomContainerPtr;
#ifndef GraphicsImportGetExportImageTypeList
	PyMac_PRECHECK(GraphicsImportGetExportImageTypeList);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &ci,
	                      &qtAtomContainerPtr))
		return NULL;
	_rv = GraphicsImportGetExportImageTypeList(ci,
	                                           qtAtomContainerPtr);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetExportSettingsAsAtomContainer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	void * qtAtomContainerPtr;
#ifndef GraphicsImportGetExportSettingsAsAtomContainer
	PyMac_PRECHECK(GraphicsImportGetExportSettingsAsAtomContainer);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &ci,
	                      &qtAtomContainerPtr))
		return NULL;
	_rv = GraphicsImportGetExportSettingsAsAtomContainer(ci,
	                                                     qtAtomContainerPtr);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportSetExportSettingsFromAtomContainer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	void * qtAtomContainer;
#ifndef GraphicsImportSetExportSettingsFromAtomContainer
	PyMac_PRECHECK(GraphicsImportSetExportSettingsFromAtomContainer);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &ci,
	                      &qtAtomContainer))
		return NULL;
	_rv = GraphicsImportSetExportSettingsFromAtomContainer(ci,
	                                                       qtAtomContainer);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetImageCount(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	unsigned long imageCount;
#ifndef GraphicsImportGetImageCount
	PyMac_PRECHECK(GraphicsImportGetImageCount);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetImageCount(ci,
	                                  &imageCount);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     imageCount);
	return _res;
}

static PyObject *Qt_GraphicsImportSetImageIndex(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	unsigned long imageIndex;
#ifndef GraphicsImportSetImageIndex
	PyMac_PRECHECK(GraphicsImportSetImageIndex);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &imageIndex))
		return NULL;
	_rv = GraphicsImportSetImageIndex(ci,
	                                  imageIndex);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetImageIndex(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	unsigned long imageIndex;
#ifndef GraphicsImportGetImageIndex
	PyMac_PRECHECK(GraphicsImportGetImageIndex);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetImageIndex(ci,
	                                  &imageIndex);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     imageIndex);
	return _res;
}

static PyObject *Qt_GraphicsImportGetDataOffsetAndSize64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	wide offset;
	wide size;
#ifndef GraphicsImportGetDataOffsetAndSize64
	PyMac_PRECHECK(GraphicsImportGetDataOffsetAndSize64);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetDataOffsetAndSize64(ci,
	                                           &offset,
	                                           &size);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     PyMac_Buildwide, offset,
	                     PyMac_Buildwide, size);
	return _res;
}

static PyObject *Qt_GraphicsImportReadData64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	void * dataPtr;
	wide dataOffset;
	unsigned long dataSize;
#ifndef GraphicsImportReadData64
	PyMac_PRECHECK(GraphicsImportReadData64);
#endif
	if (!PyArg_ParseTuple(_args, "O&sO&l",
	                      CmpInstObj_Convert, &ci,
	                      &dataPtr,
	                      PyMac_Getwide, &dataOffset,
	                      &dataSize))
		return NULL;
	_rv = GraphicsImportReadData64(ci,
	                               dataPtr,
	                               &dataOffset,
	                               dataSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportSetDataReferenceOffsetAndLimit64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	wide offset;
	wide limit;
#ifndef GraphicsImportSetDataReferenceOffsetAndLimit64
	PyMac_PRECHECK(GraphicsImportSetDataReferenceOffsetAndLimit64);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_Getwide, &offset,
	                      PyMac_Getwide, &limit))
		return NULL;
	_rv = GraphicsImportSetDataReferenceOffsetAndLimit64(ci,
	                                                     &offset,
	                                                     &limit);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetDataReferenceOffsetAndLimit64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	wide offset;
	wide limit;
#ifndef GraphicsImportGetDataReferenceOffsetAndLimit64
	PyMac_PRECHECK(GraphicsImportGetDataReferenceOffsetAndLimit64);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetDataReferenceOffsetAndLimit64(ci,
	                                                     &offset,
	                                                     &limit);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     PyMac_Buildwide, offset,
	                     PyMac_Buildwide, limit);
	return _res;
}

static PyObject *Qt_GraphicsImportGetDefaultClip(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	RgnHandle defaultRgn;
#ifndef GraphicsImportGetDefaultClip
	PyMac_PRECHECK(GraphicsImportGetDefaultClip);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetDefaultClip(ci,
	                                   &defaultRgn);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, defaultRgn);
	return _res;
}

static PyObject *Qt_GraphicsImportGetDefaultGraphicsMode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	long defaultGraphicsMode;
	RGBColor defaultOpColor;
#ifndef GraphicsImportGetDefaultGraphicsMode
	PyMac_PRECHECK(GraphicsImportGetDefaultGraphicsMode);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetDefaultGraphicsMode(ci,
	                                           &defaultGraphicsMode,
	                                           &defaultOpColor);
	_res = Py_BuildValue("llO&",
	                     _rv,
	                     defaultGraphicsMode,
	                     QdRGB_New, &defaultOpColor);
	return _res;
}

static PyObject *Qt_GraphicsImportGetDefaultSourceRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Rect defaultSourceRect;
#ifndef GraphicsImportGetDefaultSourceRect
	PyMac_PRECHECK(GraphicsImportGetDefaultSourceRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetDefaultSourceRect(ci,
	                                         &defaultSourceRect);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &defaultSourceRect);
	return _res;
}

static PyObject *Qt_GraphicsImportGetColorSyncProfile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Handle profile;
#ifndef GraphicsImportGetColorSyncProfile
	PyMac_PRECHECK(GraphicsImportGetColorSyncProfile);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetColorSyncProfile(ci,
	                                        &profile);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, profile);
	return _res;
}

static PyObject *Qt_GraphicsImportSetDestRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Rect destRect;
#ifndef GraphicsImportSetDestRect
	PyMac_PRECHECK(GraphicsImportSetDestRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetRect, &destRect))
		return NULL;
	_rv = GraphicsImportSetDestRect(ci,
	                                &destRect);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetDestRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	Rect destRect;
#ifndef GraphicsImportGetDestRect
	PyMac_PRECHECK(GraphicsImportGetDestRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetDestRect(ci,
	                                &destRect);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &destRect);
	return _res;
}

static PyObject *Qt_GraphicsImportSetFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	long flags;
#ifndef GraphicsImportSetFlags
	PyMac_PRECHECK(GraphicsImportSetFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &flags))
		return NULL;
	_rv = GraphicsImportSetFlags(ci,
	                             flags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImportGetFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	long flags;
#ifndef GraphicsImportGetFlags
	PyMac_PRECHECK(GraphicsImportGetFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetFlags(ci,
	                             &flags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     flags);
	return _res;
}

static PyObject *Qt_GraphicsImportGetBaseDataOffsetAndSize64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
	wide offset;
	wide size;
#ifndef GraphicsImportGetBaseDataOffsetAndSize64
	PyMac_PRECHECK(GraphicsImportGetBaseDataOffsetAndSize64);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportGetBaseDataOffsetAndSize64(ci,
	                                               &offset,
	                                               &size);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     PyMac_Buildwide, offset,
	                     PyMac_Buildwide, size);
	return _res;
}

static PyObject *Qt_GraphicsImportSetImageIndexToThumbnail(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsImportComponent ci;
#ifndef GraphicsImportSetImageIndexToThumbnail
	PyMac_PRECHECK(GraphicsImportSetImageIndexToThumbnail);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImportSetImageIndexToThumbnail(ci);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportDoExport(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	unsigned long actualSizeWritten;
#ifndef GraphicsExportDoExport
	PyMac_PRECHECK(GraphicsExportDoExport);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportDoExport(ci,
	                             &actualSizeWritten);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     actualSizeWritten);
	return _res;
}

static PyObject *Qt_GraphicsExportCanTranscode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Boolean canTranscode;
#ifndef GraphicsExportCanTranscode
	PyMac_PRECHECK(GraphicsExportCanTranscode);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportCanTranscode(ci,
	                                 &canTranscode);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     canTranscode);
	return _res;
}

static PyObject *Qt_GraphicsExportDoTranscode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
#ifndef GraphicsExportDoTranscode
	PyMac_PRECHECK(GraphicsExportDoTranscode);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportDoTranscode(ci);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportCanUseCompressor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Boolean canUseCompressor;
	void * codecSettingsAtomContainerPtr;
#ifndef GraphicsExportCanUseCompressor
	PyMac_PRECHECK(GraphicsExportCanUseCompressor);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &ci,
	                      &codecSettingsAtomContainerPtr))
		return NULL;
	_rv = GraphicsExportCanUseCompressor(ci,
	                                     &canUseCompressor,
	                                     codecSettingsAtomContainerPtr);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     canUseCompressor);
	return _res;
}

static PyObject *Qt_GraphicsExportDoUseCompressor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	void * codecSettingsAtomContainer;
	ImageDescriptionHandle outDesc;
#ifndef GraphicsExportDoUseCompressor
	PyMac_PRECHECK(GraphicsExportDoUseCompressor);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &ci,
	                      &codecSettingsAtomContainer))
		return NULL;
	_rv = GraphicsExportDoUseCompressor(ci,
	                                    codecSettingsAtomContainer,
	                                    &outDesc);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, outDesc);
	return _res;
}

static PyObject *Qt_GraphicsExportDoStandaloneExport(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
#ifndef GraphicsExportDoStandaloneExport
	PyMac_PRECHECK(GraphicsExportDoStandaloneExport);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportDoStandaloneExport(ci);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetDefaultFileTypeAndCreator(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	OSType fileType;
	OSType fileCreator;
#ifndef GraphicsExportGetDefaultFileTypeAndCreator
	PyMac_PRECHECK(GraphicsExportGetDefaultFileTypeAndCreator);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetDefaultFileTypeAndCreator(ci,
	                                                 &fileType,
	                                                 &fileCreator);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     PyMac_BuildOSType, fileType,
	                     PyMac_BuildOSType, fileCreator);
	return _res;
}

static PyObject *Qt_GraphicsExportGetDefaultFileNameExtension(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	OSType fileNameExtension;
#ifndef GraphicsExportGetDefaultFileNameExtension
	PyMac_PRECHECK(GraphicsExportGetDefaultFileNameExtension);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetDefaultFileNameExtension(ci,
	                                                &fileNameExtension);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildOSType, fileNameExtension);
	return _res;
}

static PyObject *Qt_GraphicsExportGetMIMETypeList(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	void * qtAtomContainerPtr;
#ifndef GraphicsExportGetMIMETypeList
	PyMac_PRECHECK(GraphicsExportGetMIMETypeList);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &ci,
	                      &qtAtomContainerPtr))
		return NULL;
	_rv = GraphicsExportGetMIMETypeList(ci,
	                                    qtAtomContainerPtr);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportSetSettingsFromAtomContainer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	void * qtAtomContainer;
#ifndef GraphicsExportSetSettingsFromAtomContainer
	PyMac_PRECHECK(GraphicsExportSetSettingsFromAtomContainer);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &ci,
	                      &qtAtomContainer))
		return NULL;
	_rv = GraphicsExportSetSettingsFromAtomContainer(ci,
	                                                 qtAtomContainer);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetSettingsAsAtomContainer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	void * qtAtomContainerPtr;
#ifndef GraphicsExportGetSettingsAsAtomContainer
	PyMac_PRECHECK(GraphicsExportGetSettingsAsAtomContainer);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &ci,
	                      &qtAtomContainerPtr))
		return NULL;
	_rv = GraphicsExportGetSettingsAsAtomContainer(ci,
	                                               qtAtomContainerPtr);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetSettingsAsText(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Handle theText;
#ifndef GraphicsExportGetSettingsAsText
	PyMac_PRECHECK(GraphicsExportGetSettingsAsText);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetSettingsAsText(ci,
	                                      &theText);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, theText);
	return _res;
}

static PyObject *Qt_GraphicsExportSetDontRecompress(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Boolean dontRecompress;
#ifndef GraphicsExportSetDontRecompress
	PyMac_PRECHECK(GraphicsExportSetDontRecompress);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &ci,
	                      &dontRecompress))
		return NULL;
	_rv = GraphicsExportSetDontRecompress(ci,
	                                      dontRecompress);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetDontRecompress(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Boolean dontRecompress;
#ifndef GraphicsExportGetDontRecompress
	PyMac_PRECHECK(GraphicsExportGetDontRecompress);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetDontRecompress(ci,
	                                      &dontRecompress);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     dontRecompress);
	return _res;
}

static PyObject *Qt_GraphicsExportSetInterlaceStyle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	unsigned long interlaceStyle;
#ifndef GraphicsExportSetInterlaceStyle
	PyMac_PRECHECK(GraphicsExportSetInterlaceStyle);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &interlaceStyle))
		return NULL;
	_rv = GraphicsExportSetInterlaceStyle(ci,
	                                      interlaceStyle);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInterlaceStyle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	unsigned long interlaceStyle;
#ifndef GraphicsExportGetInterlaceStyle
	PyMac_PRECHECK(GraphicsExportGetInterlaceStyle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetInterlaceStyle(ci,
	                                      &interlaceStyle);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     interlaceStyle);
	return _res;
}

static PyObject *Qt_GraphicsExportSetMetaData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	void * userData;
#ifndef GraphicsExportSetMetaData
	PyMac_PRECHECK(GraphicsExportSetMetaData);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &ci,
	                      &userData))
		return NULL;
	_rv = GraphicsExportSetMetaData(ci,
	                                userData);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetMetaData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	void * userData;
#ifndef GraphicsExportGetMetaData
	PyMac_PRECHECK(GraphicsExportGetMetaData);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &ci,
	                      &userData))
		return NULL;
	_rv = GraphicsExportGetMetaData(ci,
	                                userData);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportSetTargetDataSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	unsigned long targetDataSize;
#ifndef GraphicsExportSetTargetDataSize
	PyMac_PRECHECK(GraphicsExportSetTargetDataSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &targetDataSize))
		return NULL;
	_rv = GraphicsExportSetTargetDataSize(ci,
	                                      targetDataSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetTargetDataSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	unsigned long targetDataSize;
#ifndef GraphicsExportGetTargetDataSize
	PyMac_PRECHECK(GraphicsExportGetTargetDataSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetTargetDataSize(ci,
	                                      &targetDataSize);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     targetDataSize);
	return _res;
}

static PyObject *Qt_GraphicsExportSetCompressionMethod(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	long compressionMethod;
#ifndef GraphicsExportSetCompressionMethod
	PyMac_PRECHECK(GraphicsExportSetCompressionMethod);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &compressionMethod))
		return NULL;
	_rv = GraphicsExportSetCompressionMethod(ci,
	                                         compressionMethod);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetCompressionMethod(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	long compressionMethod;
#ifndef GraphicsExportGetCompressionMethod
	PyMac_PRECHECK(GraphicsExportGetCompressionMethod);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetCompressionMethod(ci,
	                                         &compressionMethod);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     compressionMethod);
	return _res;
}

static PyObject *Qt_GraphicsExportSetCompressionQuality(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	CodecQ spatialQuality;
#ifndef GraphicsExportSetCompressionQuality
	PyMac_PRECHECK(GraphicsExportSetCompressionQuality);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &spatialQuality))
		return NULL;
	_rv = GraphicsExportSetCompressionQuality(ci,
	                                          spatialQuality);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetCompressionQuality(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	CodecQ spatialQuality;
#ifndef GraphicsExportGetCompressionQuality
	PyMac_PRECHECK(GraphicsExportGetCompressionQuality);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetCompressionQuality(ci,
	                                          &spatialQuality);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     spatialQuality);
	return _res;
}

static PyObject *Qt_GraphicsExportSetResolution(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Fixed horizontalResolution;
	Fixed verticalResolution;
#ifndef GraphicsExportSetResolution
	PyMac_PRECHECK(GraphicsExportSetResolution);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFixed, &horizontalResolution,
	                      PyMac_GetFixed, &verticalResolution))
		return NULL;
	_rv = GraphicsExportSetResolution(ci,
	                                  horizontalResolution,
	                                  verticalResolution);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetResolution(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Fixed horizontalResolution;
	Fixed verticalResolution;
#ifndef GraphicsExportGetResolution
	PyMac_PRECHECK(GraphicsExportGetResolution);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetResolution(ci,
	                                  &horizontalResolution,
	                                  &verticalResolution);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     PyMac_BuildFixed, horizontalResolution,
	                     PyMac_BuildFixed, verticalResolution);
	return _res;
}

static PyObject *Qt_GraphicsExportSetDepth(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	long depth;
#ifndef GraphicsExportSetDepth
	PyMac_PRECHECK(GraphicsExportSetDepth);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &depth))
		return NULL;
	_rv = GraphicsExportSetDepth(ci,
	                             depth);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetDepth(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	long depth;
#ifndef GraphicsExportGetDepth
	PyMac_PRECHECK(GraphicsExportGetDepth);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetDepth(ci,
	                             &depth);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     depth);
	return _res;
}

static PyObject *Qt_GraphicsExportSetColorSyncProfile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Handle colorSyncProfile;
#ifndef GraphicsExportSetColorSyncProfile
	PyMac_PRECHECK(GraphicsExportSetColorSyncProfile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &colorSyncProfile))
		return NULL;
	_rv = GraphicsExportSetColorSyncProfile(ci,
	                                        colorSyncProfile);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetColorSyncProfile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Handle colorSyncProfile;
#ifndef GraphicsExportGetColorSyncProfile
	PyMac_PRECHECK(GraphicsExportGetColorSyncProfile);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetColorSyncProfile(ci,
	                                        &colorSyncProfile);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, colorSyncProfile);
	return _res;
}

static PyObject *Qt_GraphicsExportSetInputDataReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Handle dataRef;
	OSType dataRefType;
	ImageDescriptionHandle desc;
#ifndef GraphicsExportSetInputDataReference
	PyMac_PRECHECK(GraphicsExportSetInputDataReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      ResObj_Convert, &desc))
		return NULL;
	_rv = GraphicsExportSetInputDataReference(ci,
	                                          dataRef,
	                                          dataRefType,
	                                          desc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInputDataReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Handle dataRef;
	OSType dataRefType;
#ifndef GraphicsExportGetInputDataReference
	PyMac_PRECHECK(GraphicsExportGetInputDataReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetInputDataReference(ci,
	                                          &dataRef,
	                                          &dataRefType);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, dataRef,
	                     PyMac_BuildOSType, dataRefType);
	return _res;
}

static PyObject *Qt_GraphicsExportSetInputFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	FSSpec theFile;
	ImageDescriptionHandle desc;
#ifndef GraphicsExportSetInputFile
	PyMac_PRECHECK(GraphicsExportSetInputFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFSSpec, &theFile,
	                      ResObj_Convert, &desc))
		return NULL;
	_rv = GraphicsExportSetInputFile(ci,
	                                 &theFile,
	                                 desc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInputFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	FSSpec theFile;
#ifndef GraphicsExportGetInputFile
	PyMac_PRECHECK(GraphicsExportGetInputFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFSSpec, &theFile))
		return NULL;
	_rv = GraphicsExportGetInputFile(ci,
	                                 &theFile);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportSetInputHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Handle h;
	ImageDescriptionHandle desc;
#ifndef GraphicsExportSetInputHandle
	PyMac_PRECHECK(GraphicsExportSetInputHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &h,
	                      ResObj_Convert, &desc))
		return NULL;
	_rv = GraphicsExportSetInputHandle(ci,
	                                   h,
	                                   desc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInputHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Handle h;
#ifndef GraphicsExportGetInputHandle
	PyMac_PRECHECK(GraphicsExportGetInputHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetInputHandle(ci,
	                                   &h);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, h);
	return _res;
}

static PyObject *Qt_GraphicsExportSetInputPtr(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Ptr p;
	unsigned long size;
	ImageDescriptionHandle desc;
#ifndef GraphicsExportSetInputPtr
	PyMac_PRECHECK(GraphicsExportSetInputPtr);
#endif
	if (!PyArg_ParseTuple(_args, "O&slO&",
	                      CmpInstObj_Convert, &ci,
	                      &p,
	                      &size,
	                      ResObj_Convert, &desc))
		return NULL;
	_rv = GraphicsExportSetInputPtr(ci,
	                                p,
	                                size,
	                                desc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportSetInputGraphicsImporter(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	GraphicsImportComponent grip;
#ifndef GraphicsExportSetInputGraphicsImporter
	PyMac_PRECHECK(GraphicsExportSetInputGraphicsImporter);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      CmpInstObj_Convert, &grip))
		return NULL;
	_rv = GraphicsExportSetInputGraphicsImporter(ci,
	                                             grip);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInputGraphicsImporter(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	GraphicsImportComponent grip;
#ifndef GraphicsExportGetInputGraphicsImporter
	PyMac_PRECHECK(GraphicsExportGetInputGraphicsImporter);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetInputGraphicsImporter(ci,
	                                             &grip);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     CmpInstObj_New, grip);
	return _res;
}

static PyObject *Qt_GraphicsExportSetInputPicture(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	PicHandle picture;
#ifndef GraphicsExportSetInputPicture
	PyMac_PRECHECK(GraphicsExportSetInputPicture);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &picture))
		return NULL;
	_rv = GraphicsExportSetInputPicture(ci,
	                                    picture);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInputPicture(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	PicHandle picture;
#ifndef GraphicsExportGetInputPicture
	PyMac_PRECHECK(GraphicsExportGetInputPicture);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetInputPicture(ci,
	                                    &picture);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, picture);
	return _res;
}

static PyObject *Qt_GraphicsExportSetInputGWorld(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	GWorldPtr gworld;
#ifndef GraphicsExportSetInputGWorld
	PyMac_PRECHECK(GraphicsExportSetInputGWorld);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      GWorldObj_Convert, &gworld))
		return NULL;
	_rv = GraphicsExportSetInputGWorld(ci,
	                                   gworld);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInputGWorld(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	GWorldPtr gworld;
#ifndef GraphicsExportGetInputGWorld
	PyMac_PRECHECK(GraphicsExportGetInputGWorld);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetInputGWorld(ci,
	                                   &gworld);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     GWorldObj_New, gworld);
	return _res;
}

static PyObject *Qt_GraphicsExportSetInputPixmap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	PixMapHandle pixmap;
#ifndef GraphicsExportSetInputPixmap
	PyMac_PRECHECK(GraphicsExportSetInputPixmap);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &pixmap))
		return NULL;
	_rv = GraphicsExportSetInputPixmap(ci,
	                                   pixmap);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInputPixmap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	PixMapHandle pixmap;
#ifndef GraphicsExportGetInputPixmap
	PyMac_PRECHECK(GraphicsExportGetInputPixmap);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetInputPixmap(ci,
	                                   &pixmap);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, pixmap);
	return _res;
}

static PyObject *Qt_GraphicsExportSetInputOffsetAndLimit(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	unsigned long offset;
	unsigned long limit;
#ifndef GraphicsExportSetInputOffsetAndLimit
	PyMac_PRECHECK(GraphicsExportSetInputOffsetAndLimit);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &ci,
	                      &offset,
	                      &limit))
		return NULL;
	_rv = GraphicsExportSetInputOffsetAndLimit(ci,
	                                           offset,
	                                           limit);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInputOffsetAndLimit(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	unsigned long offset;
	unsigned long limit;
#ifndef GraphicsExportGetInputOffsetAndLimit
	PyMac_PRECHECK(GraphicsExportGetInputOffsetAndLimit);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetInputOffsetAndLimit(ci,
	                                           &offset,
	                                           &limit);
	_res = Py_BuildValue("lll",
	                     _rv,
	                     offset,
	                     limit);
	return _res;
}

static PyObject *Qt_GraphicsExportMayExporterReadInputData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Boolean mayReadInputData;
#ifndef GraphicsExportMayExporterReadInputData
	PyMac_PRECHECK(GraphicsExportMayExporterReadInputData);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportMayExporterReadInputData(ci,
	                                             &mayReadInputData);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     mayReadInputData);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInputDataSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	unsigned long size;
#ifndef GraphicsExportGetInputDataSize
	PyMac_PRECHECK(GraphicsExportGetInputDataSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetInputDataSize(ci,
	                                     &size);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     size);
	return _res;
}

static PyObject *Qt_GraphicsExportReadInputData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	void * dataPtr;
	unsigned long dataOffset;
	unsigned long dataSize;
#ifndef GraphicsExportReadInputData
	PyMac_PRECHECK(GraphicsExportReadInputData);
#endif
	if (!PyArg_ParseTuple(_args, "O&sll",
	                      CmpInstObj_Convert, &ci,
	                      &dataPtr,
	                      &dataOffset,
	                      &dataSize))
		return NULL;
	_rv = GraphicsExportReadInputData(ci,
	                                  dataPtr,
	                                  dataOffset,
	                                  dataSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInputImageDescription(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	ImageDescriptionHandle desc;
#ifndef GraphicsExportGetInputImageDescription
	PyMac_PRECHECK(GraphicsExportGetInputImageDescription);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetInputImageDescription(ci,
	                                             &desc);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, desc);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInputImageDimensions(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Rect dimensions;
#ifndef GraphicsExportGetInputImageDimensions
	PyMac_PRECHECK(GraphicsExportGetInputImageDimensions);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetInputImageDimensions(ci,
	                                            &dimensions);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &dimensions);
	return _res;
}

static PyObject *Qt_GraphicsExportGetInputImageDepth(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	long inputDepth;
#ifndef GraphicsExportGetInputImageDepth
	PyMac_PRECHECK(GraphicsExportGetInputImageDepth);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetInputImageDepth(ci,
	                                       &inputDepth);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     inputDepth);
	return _res;
}

static PyObject *Qt_GraphicsExportDrawInputImage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	CGrafPtr gw;
	GDHandle gd;
	Rect srcRect;
	Rect dstRect;
#ifndef GraphicsExportDrawInputImage
	PyMac_PRECHECK(GraphicsExportDrawInputImage);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      GrafObj_Convert, &gw,
	                      OptResObj_Convert, &gd,
	                      PyMac_GetRect, &srcRect,
	                      PyMac_GetRect, &dstRect))
		return NULL;
	_rv = GraphicsExportDrawInputImage(ci,
	                                   gw,
	                                   gd,
	                                   &srcRect,
	                                   &dstRect);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportSetOutputDataReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Handle dataRef;
	OSType dataRefType;
#ifndef GraphicsExportSetOutputDataReference
	PyMac_PRECHECK(GraphicsExportSetOutputDataReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_rv = GraphicsExportSetOutputDataReference(ci,
	                                           dataRef,
	                                           dataRefType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetOutputDataReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Handle dataRef;
	OSType dataRefType;
#ifndef GraphicsExportGetOutputDataReference
	PyMac_PRECHECK(GraphicsExportGetOutputDataReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetOutputDataReference(ci,
	                                           &dataRef,
	                                           &dataRefType);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, dataRef,
	                     PyMac_BuildOSType, dataRefType);
	return _res;
}

static PyObject *Qt_GraphicsExportSetOutputFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	FSSpec theFile;
#ifndef GraphicsExportSetOutputFile
	PyMac_PRECHECK(GraphicsExportSetOutputFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFSSpec, &theFile))
		return NULL;
	_rv = GraphicsExportSetOutputFile(ci,
	                                  &theFile);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetOutputFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	FSSpec theFile;
#ifndef GraphicsExportGetOutputFile
	PyMac_PRECHECK(GraphicsExportGetOutputFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFSSpec, &theFile))
		return NULL;
	_rv = GraphicsExportGetOutputFile(ci,
	                                  &theFile);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportSetOutputHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Handle h;
#ifndef GraphicsExportSetOutputHandle
	PyMac_PRECHECK(GraphicsExportSetOutputHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &h))
		return NULL;
	_rv = GraphicsExportSetOutputHandle(ci,
	                                    h);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetOutputHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Handle h;
#ifndef GraphicsExportGetOutputHandle
	PyMac_PRECHECK(GraphicsExportGetOutputHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetOutputHandle(ci,
	                                    &h);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, h);
	return _res;
}

static PyObject *Qt_GraphicsExportSetOutputOffsetAndMaxSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	unsigned long offset;
	unsigned long maxSize;
	Boolean truncateFile;
#ifndef GraphicsExportSetOutputOffsetAndMaxSize
	PyMac_PRECHECK(GraphicsExportSetOutputOffsetAndMaxSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&llb",
	                      CmpInstObj_Convert, &ci,
	                      &offset,
	                      &maxSize,
	                      &truncateFile))
		return NULL;
	_rv = GraphicsExportSetOutputOffsetAndMaxSize(ci,
	                                              offset,
	                                              maxSize,
	                                              truncateFile);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetOutputOffsetAndMaxSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	unsigned long offset;
	unsigned long maxSize;
	Boolean truncateFile;
#ifndef GraphicsExportGetOutputOffsetAndMaxSize
	PyMac_PRECHECK(GraphicsExportGetOutputOffsetAndMaxSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetOutputOffsetAndMaxSize(ci,
	                                              &offset,
	                                              &maxSize,
	                                              &truncateFile);
	_res = Py_BuildValue("lllb",
	                     _rv,
	                     offset,
	                     maxSize,
	                     truncateFile);
	return _res;
}

static PyObject *Qt_GraphicsExportSetOutputFileTypeAndCreator(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	OSType fileType;
	OSType fileCreator;
#ifndef GraphicsExportSetOutputFileTypeAndCreator
	PyMac_PRECHECK(GraphicsExportSetOutputFileTypeAndCreator);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetOSType, &fileType,
	                      PyMac_GetOSType, &fileCreator))
		return NULL;
	_rv = GraphicsExportSetOutputFileTypeAndCreator(ci,
	                                                fileType,
	                                                fileCreator);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetOutputFileTypeAndCreator(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	OSType fileType;
	OSType fileCreator;
#ifndef GraphicsExportGetOutputFileTypeAndCreator
	PyMac_PRECHECK(GraphicsExportGetOutputFileTypeAndCreator);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetOutputFileTypeAndCreator(ci,
	                                                &fileType,
	                                                &fileCreator);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     PyMac_BuildOSType, fileType,
	                     PyMac_BuildOSType, fileCreator);
	return _res;
}

static PyObject *Qt_GraphicsExportSetOutputMark(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	unsigned long mark;
#ifndef GraphicsExportSetOutputMark
	PyMac_PRECHECK(GraphicsExportSetOutputMark);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &mark))
		return NULL;
	_rv = GraphicsExportSetOutputMark(ci,
	                                  mark);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetOutputMark(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	unsigned long mark;
#ifndef GraphicsExportGetOutputMark
	PyMac_PRECHECK(GraphicsExportGetOutputMark);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetOutputMark(ci,
	                                  &mark);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     mark);
	return _res;
}

static PyObject *Qt_GraphicsExportReadOutputData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	void * dataPtr;
	unsigned long dataOffset;
	unsigned long dataSize;
#ifndef GraphicsExportReadOutputData
	PyMac_PRECHECK(GraphicsExportReadOutputData);
#endif
	if (!PyArg_ParseTuple(_args, "O&sll",
	                      CmpInstObj_Convert, &ci,
	                      &dataPtr,
	                      &dataOffset,
	                      &dataSize))
		return NULL;
	_rv = GraphicsExportReadOutputData(ci,
	                                   dataPtr,
	                                   dataOffset,
	                                   dataSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportSetThumbnailEnabled(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Boolean enableThumbnail;
	long maxThumbnailWidth;
	long maxThumbnailHeight;
#ifndef GraphicsExportSetThumbnailEnabled
	PyMac_PRECHECK(GraphicsExportSetThumbnailEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "O&bll",
	                      CmpInstObj_Convert, &ci,
	                      &enableThumbnail,
	                      &maxThumbnailWidth,
	                      &maxThumbnailHeight))
		return NULL;
	_rv = GraphicsExportSetThumbnailEnabled(ci,
	                                        enableThumbnail,
	                                        maxThumbnailWidth,
	                                        maxThumbnailHeight);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetThumbnailEnabled(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Boolean thumbnailEnabled;
	long maxThumbnailWidth;
	long maxThumbnailHeight;
#ifndef GraphicsExportGetThumbnailEnabled
	PyMac_PRECHECK(GraphicsExportGetThumbnailEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetThumbnailEnabled(ci,
	                                        &thumbnailEnabled,
	                                        &maxThumbnailWidth,
	                                        &maxThumbnailHeight);
	_res = Py_BuildValue("lbll",
	                     _rv,
	                     thumbnailEnabled,
	                     maxThumbnailWidth,
	                     maxThumbnailHeight);
	return _res;
}

static PyObject *Qt_GraphicsExportSetExifEnabled(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Boolean enableExif;
#ifndef GraphicsExportSetExifEnabled
	PyMac_PRECHECK(GraphicsExportSetExifEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &ci,
	                      &enableExif))
		return NULL;
	_rv = GraphicsExportSetExifEnabled(ci,
	                                   enableExif);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsExportGetExifEnabled(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicsExportComponent ci;
	Boolean exifEnabled;
#ifndef GraphicsExportGetExifEnabled
	PyMac_PRECHECK(GraphicsExportGetExifEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsExportGetExifEnabled(ci,
	                                   &exifEnabled);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     exifEnabled);
	return _res;
}

static PyObject *Qt_ImageTranscoderBeginSequence(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ImageTranscoderComponent itc;
	ImageDescriptionHandle srcDesc;
	ImageDescriptionHandle dstDesc;
	void * data;
	long dataSize;
#ifndef ImageTranscoderBeginSequence
	PyMac_PRECHECK(ImageTranscoderBeginSequence);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&sl",
	                      CmpInstObj_Convert, &itc,
	                      ResObj_Convert, &srcDesc,
	                      &data,
	                      &dataSize))
		return NULL;
	_rv = ImageTranscoderBeginSequence(itc,
	                                   srcDesc,
	                                   &dstDesc,
	                                   data,
	                                   dataSize);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, dstDesc);
	return _res;
}

static PyObject *Qt_ImageTranscoderDisposeData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ImageTranscoderComponent itc;
	void * dstData;
#ifndef ImageTranscoderDisposeData
	PyMac_PRECHECK(ImageTranscoderDisposeData);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &itc,
	                      &dstData))
		return NULL;
	_rv = ImageTranscoderDisposeData(itc,
	                                 dstData);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_ImageTranscoderEndSequence(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ImageTranscoderComponent itc;
#ifndef ImageTranscoderEndSequence
	PyMac_PRECHECK(ImageTranscoderEndSequence);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &itc))
		return NULL;
	_rv = ImageTranscoderEndSequence(itc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_ClockGetTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance aClock;
	TimeRecord out;
#ifndef ClockGetTime
	PyMac_PRECHECK(ClockGetTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &aClock))
		return NULL;
	_rv = ClockGetTime(aClock,
	                   &out);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     QtTimeRecord_New, &out);
	return _res;
}

static PyObject *Qt_ClockSetTimeBase(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance aClock;
	TimeBase tb;
#ifndef ClockSetTimeBase
	PyMac_PRECHECK(ClockSetTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &aClock,
	                      TimeBaseObj_Convert, &tb))
		return NULL;
	_rv = ClockSetTimeBase(aClock,
	                       tb);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_ClockGetRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance aClock;
	Fixed rate;
#ifndef ClockGetRate
	PyMac_PRECHECK(ClockGetRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &aClock))
		return NULL;
	_rv = ClockGetRate(aClock,
	                   &rate);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildFixed, rate);
	return _res;
}

static PyObject *Qt_SCPositionRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	Rect rp;
	Point where;
#ifndef SCPositionRect
	PyMac_PRECHECK(SCPositionRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = SCPositionRect(ci,
	                     &rp,
	                     &where);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     PyMac_BuildRect, &rp,
	                     PyMac_BuildPoint, where);
	return _res;
}

static PyObject *Qt_SCPositionDialog(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	short id;
	Point where;
#ifndef SCPositionDialog
	PyMac_PRECHECK(SCPositionDialog);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &id))
		return NULL;
	_rv = SCPositionDialog(ci,
	                       id,
	                       &where);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildPoint, where);
	return _res;
}

static PyObject *Qt_SCSetTestImagePictHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	PicHandle testPict;
	Rect testRect;
	short testFlags;
#ifndef SCSetTestImagePictHandle
	PyMac_PRECHECK(SCSetTestImagePictHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &testPict,
	                      &testFlags))
		return NULL;
	_rv = SCSetTestImagePictHandle(ci,
	                               testPict,
	                               &testRect,
	                               testFlags);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &testRect);
	return _res;
}

static PyObject *Qt_SCSetTestImagePictFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	short testFileRef;
	Rect testRect;
	short testFlags;
#ifndef SCSetTestImagePictFile
	PyMac_PRECHECK(SCSetTestImagePictFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      CmpInstObj_Convert, &ci,
	                      &testFileRef,
	                      &testFlags))
		return NULL;
	_rv = SCSetTestImagePictFile(ci,
	                             testFileRef,
	                             &testRect,
	                             testFlags);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &testRect);
	return _res;
}

static PyObject *Qt_SCSetTestImagePixMap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	PixMapHandle testPixMap;
	Rect testRect;
	short testFlags;
#ifndef SCSetTestImagePixMap
	PyMac_PRECHECK(SCSetTestImagePixMap);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &testPixMap,
	                      &testFlags))
		return NULL;
	_rv = SCSetTestImagePixMap(ci,
	                           testPixMap,
	                           &testRect,
	                           testFlags);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &testRect);
	return _res;
}

static PyObject *Qt_SCGetBestDeviceRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	Rect r;
#ifndef SCGetBestDeviceRect
	PyMac_PRECHECK(SCGetBestDeviceRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = SCGetBestDeviceRect(ci,
	                          &r);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &r);
	return _res;
}

static PyObject *Qt_SCRequestImageSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
#ifndef SCRequestImageSettings
	PyMac_PRECHECK(SCRequestImageSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = SCRequestImageSettings(ci);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SCCompressImage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	PixMapHandle src;
	Rect srcRect;
	ImageDescriptionHandle desc;
	Handle data;
#ifndef SCCompressImage
	PyMac_PRECHECK(SCCompressImage);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &src,
	                      PyMac_GetRect, &srcRect))
		return NULL;
	_rv = SCCompressImage(ci,
	                      src,
	                      &srcRect,
	                      &desc,
	                      &data);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, desc,
	                     ResObj_New, data);
	return _res;
}

static PyObject *Qt_SCCompressPicture(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	PicHandle srcPicture;
	PicHandle dstPicture;
#ifndef SCCompressPicture
	PyMac_PRECHECK(SCCompressPicture);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &srcPicture,
	                      ResObj_Convert, &dstPicture))
		return NULL;
	_rv = SCCompressPicture(ci,
	                        srcPicture,
	                        dstPicture);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SCCompressPictureFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	short srcRefNum;
	short dstRefNum;
#ifndef SCCompressPictureFile
	PyMac_PRECHECK(SCCompressPictureFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      CmpInstObj_Convert, &ci,
	                      &srcRefNum,
	                      &dstRefNum))
		return NULL;
	_rv = SCCompressPictureFile(ci,
	                            srcRefNum,
	                            dstRefNum);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SCRequestSequenceSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
#ifndef SCRequestSequenceSettings
	PyMac_PRECHECK(SCRequestSequenceSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = SCRequestSequenceSettings(ci);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SCCompressSequenceBegin(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	PixMapHandle src;
	Rect srcRect;
	ImageDescriptionHandle desc;
#ifndef SCCompressSequenceBegin
	PyMac_PRECHECK(SCCompressSequenceBegin);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &src,
	                      PyMac_GetRect, &srcRect))
		return NULL;
	_rv = SCCompressSequenceBegin(ci,
	                              src,
	                              &srcRect,
	                              &desc);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, desc);
	return _res;
}

static PyObject *Qt_SCCompressSequenceFrame(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	PixMapHandle src;
	Rect srcRect;
	Handle data;
	long dataSize;
	short notSyncFlag;
#ifndef SCCompressSequenceFrame
	PyMac_PRECHECK(SCCompressSequenceFrame);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &src,
	                      PyMac_GetRect, &srcRect))
		return NULL;
	_rv = SCCompressSequenceFrame(ci,
	                              src,
	                              &srcRect,
	                              &data,
	                              &dataSize,
	                              &notSyncFlag);
	_res = Py_BuildValue("lO&lh",
	                     _rv,
	                     ResObj_New, data,
	                     dataSize,
	                     notSyncFlag);
	return _res;
}

static PyObject *Qt_SCCompressSequenceEnd(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
#ifndef SCCompressSequenceEnd
	PyMac_PRECHECK(SCCompressSequenceEnd);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = SCCompressSequenceEnd(ci);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SCDefaultPictHandleSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	PicHandle srcPicture;
	short motion;
#ifndef SCDefaultPictHandleSettings
	PyMac_PRECHECK(SCDefaultPictHandleSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &srcPicture,
	                      &motion))
		return NULL;
	_rv = SCDefaultPictHandleSettings(ci,
	                                  srcPicture,
	                                  motion);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SCDefaultPictFileSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	short srcRef;
	short motion;
#ifndef SCDefaultPictFileSettings
	PyMac_PRECHECK(SCDefaultPictFileSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      CmpInstObj_Convert, &ci,
	                      &srcRef,
	                      &motion))
		return NULL;
	_rv = SCDefaultPictFileSettings(ci,
	                                srcRef,
	                                motion);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SCDefaultPixMapSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	PixMapHandle src;
	short motion;
#ifndef SCDefaultPixMapSettings
	PyMac_PRECHECK(SCDefaultPixMapSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &src,
	                      &motion))
		return NULL;
	_rv = SCDefaultPixMapSettings(ci,
	                              src,
	                              motion);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SCGetInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	OSType infoType;
	void * info;
#ifndef SCGetInfo
	PyMac_PRECHECK(SCGetInfo);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&s",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetOSType, &infoType,
	                      &info))
		return NULL;
	_rv = SCGetInfo(ci,
	                infoType,
	                info);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SCSetInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	OSType infoType;
	void * info;
#ifndef SCSetInfo
	PyMac_PRECHECK(SCSetInfo);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&s",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetOSType, &infoType,
	                      &info))
		return NULL;
	_rv = SCSetInfo(ci,
	                infoType,
	                info);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SCSetCompressFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	long flags;
#ifndef SCSetCompressFlags
	PyMac_PRECHECK(SCSetCompressFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &flags))
		return NULL;
	_rv = SCSetCompressFlags(ci,
	                         flags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SCGetCompressFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	long flags;
#ifndef SCGetCompressFlags
	PyMac_PRECHECK(SCGetCompressFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = SCGetCompressFlags(ci,
	                         &flags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     flags);
	return _res;
}

static PyObject *Qt_SCGetSettingsAsText(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
	Handle text;
#ifndef SCGetSettingsAsText
	PyMac_PRECHECK(SCGetSettingsAsText);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = SCGetSettingsAsText(ci,
	                          &text);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, text);
	return _res;
}

static PyObject *Qt_SCAsyncIdle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance ci;
#ifndef SCAsyncIdle
	PyMac_PRECHECK(SCAsyncIdle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = SCAsyncIdle(ci);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TweenerReset(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TweenerComponent tc;
#ifndef TweenerReset
	PyMac_PRECHECK(TweenerReset);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &tc))
		return NULL;
	_rv = TweenerReset(tc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TCGetSourceRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	HandlerError _rv;
	MediaHandler mh;
	TimeCodeDescriptionHandle tcdH;
	UserData srefH;
#ifndef TCGetSourceRef
	PyMac_PRECHECK(TCGetSourceRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &tcdH))
		return NULL;
	_rv = TCGetSourceRef(mh,
	                     tcdH,
	                     &srefH);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     UserDataObj_New, srefH);
	return _res;
}

static PyObject *Qt_TCSetSourceRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	HandlerError _rv;
	MediaHandler mh;
	TimeCodeDescriptionHandle tcdH;
	UserData srefH;
#ifndef TCSetSourceRef
	PyMac_PRECHECK(TCSetSourceRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &tcdH,
	                      UserDataObj_Convert, &srefH))
		return NULL;
	_rv = TCSetSourceRef(mh,
	                     tcdH,
	                     srefH);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TCSetTimeCodeFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	HandlerError _rv;
	MediaHandler mh;
	long flags;
	long flagsMask;
#ifndef TCSetTimeCodeFlags
	PyMac_PRECHECK(TCSetTimeCodeFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mh,
	                      &flags,
	                      &flagsMask))
		return NULL;
	_rv = TCSetTimeCodeFlags(mh,
	                         flags,
	                         flagsMask);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TCGetTimeCodeFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	HandlerError _rv;
	MediaHandler mh;
	long flags;
#ifndef TCGetTimeCodeFlags
	PyMac_PRECHECK(TCGetTimeCodeFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = TCGetTimeCodeFlags(mh,
	                         &flags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     flags);
	return _res;
}

static PyObject *Qt_MovieImportHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	Handle dataH;
	Movie theMovie;
	Track targetTrack;
	Track usedTrack;
	TimeValue atTime;
	TimeValue addedDuration;
	long inFlags;
	long outFlags;
#ifndef MovieImportHandle
	PyMac_PRECHECK(MovieImportHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&ll",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &dataH,
	                      MovieObj_Convert, &theMovie,
	                      TrackObj_Convert, &targetTrack,
	                      &atTime,
	                      &inFlags))
		return NULL;
	_rv = MovieImportHandle(ci,
	                        dataH,
	                        theMovie,
	                        targetTrack,
	                        &usedTrack,
	                        atTime,
	                        &addedDuration,
	                        inFlags,
	                        &outFlags);
	_res = Py_BuildValue("lO&ll",
	                     _rv,
	                     TrackObj_New, usedTrack,
	                     addedDuration,
	                     outFlags);
	return _res;
}

static PyObject *Qt_MovieImportFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	FSSpec theFile;
	Movie theMovie;
	Track targetTrack;
	Track usedTrack;
	TimeValue atTime;
	TimeValue addedDuration;
	long inFlags;
	long outFlags;
#ifndef MovieImportFile
	PyMac_PRECHECK(MovieImportFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&ll",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFSSpec, &theFile,
	                      MovieObj_Convert, &theMovie,
	                      TrackObj_Convert, &targetTrack,
	                      &atTime,
	                      &inFlags))
		return NULL;
	_rv = MovieImportFile(ci,
	                      &theFile,
	                      theMovie,
	                      targetTrack,
	                      &usedTrack,
	                      atTime,
	                      &addedDuration,
	                      inFlags,
	                      &outFlags);
	_res = Py_BuildValue("lO&ll",
	                     _rv,
	                     TrackObj_New, usedTrack,
	                     addedDuration,
	                     outFlags);
	return _res;
}

static PyObject *Qt_MovieImportSetSampleDuration(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	TimeValue duration;
	TimeScale scale;
#ifndef MovieImportSetSampleDuration
	PyMac_PRECHECK(MovieImportSetSampleDuration);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &ci,
	                      &duration,
	                      &scale))
		return NULL;
	_rv = MovieImportSetSampleDuration(ci,
	                                   duration,
	                                   scale);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportSetSampleDescription(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	SampleDescriptionHandle desc;
	OSType mediaType;
#ifndef MovieImportSetSampleDescription
	PyMac_PRECHECK(MovieImportSetSampleDescription);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &desc,
	                      PyMac_GetOSType, &mediaType))
		return NULL;
	_rv = MovieImportSetSampleDescription(ci,
	                                      desc,
	                                      mediaType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportSetMediaFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	AliasHandle alias;
#ifndef MovieImportSetMediaFile
	PyMac_PRECHECK(MovieImportSetMediaFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &alias))
		return NULL;
	_rv = MovieImportSetMediaFile(ci,
	                              alias);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportSetDimensions(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	Fixed width;
	Fixed height;
#ifndef MovieImportSetDimensions
	PyMac_PRECHECK(MovieImportSetDimensions);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFixed, &width,
	                      PyMac_GetFixed, &height))
		return NULL;
	_rv = MovieImportSetDimensions(ci,
	                               width,
	                               height);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportSetChunkSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	long chunkSize;
#ifndef MovieImportSetChunkSize
	PyMac_PRECHECK(MovieImportSetChunkSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &chunkSize))
		return NULL;
	_rv = MovieImportSetChunkSize(ci,
	                              chunkSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportSetAuxiliaryData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	Handle data;
	OSType handleType;
#ifndef MovieImportSetAuxiliaryData
	PyMac_PRECHECK(MovieImportSetAuxiliaryData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &data,
	                      PyMac_GetOSType, &handleType))
		return NULL;
	_rv = MovieImportSetAuxiliaryData(ci,
	                                  data,
	                                  handleType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportSetFromScrap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	Boolean fromScrap;
#ifndef MovieImportSetFromScrap
	PyMac_PRECHECK(MovieImportSetFromScrap);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &ci,
	                      &fromScrap))
		return NULL;
	_rv = MovieImportSetFromScrap(ci,
	                              fromScrap);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportDoUserDialog(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	FSSpec theFile;
	Handle theData;
	Boolean canceled;
#ifndef MovieImportDoUserDialog
	PyMac_PRECHECK(MovieImportDoUserDialog);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFSSpec, &theFile,
	                      ResObj_Convert, &theData))
		return NULL;
	_rv = MovieImportDoUserDialog(ci,
	                              &theFile,
	                              theData,
	                              &canceled);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     canceled);
	return _res;
}

static PyObject *Qt_MovieImportSetDuration(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	TimeValue duration;
#ifndef MovieImportSetDuration
	PyMac_PRECHECK(MovieImportSetDuration);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &duration))
		return NULL;
	_rv = MovieImportSetDuration(ci,
	                             duration);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportGetAuxiliaryDataType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	OSType auxType;
#ifndef MovieImportGetAuxiliaryDataType
	PyMac_PRECHECK(MovieImportGetAuxiliaryDataType);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = MovieImportGetAuxiliaryDataType(ci,
	                                      &auxType);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildOSType, auxType);
	return _res;
}

static PyObject *Qt_MovieImportValidate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	FSSpec theFile;
	Handle theData;
	Boolean valid;
#ifndef MovieImportValidate
	PyMac_PRECHECK(MovieImportValidate);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFSSpec, &theFile,
	                      ResObj_Convert, &theData))
		return NULL;
	_rv = MovieImportValidate(ci,
	                          &theFile,
	                          theData,
	                          &valid);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     valid);
	return _res;
}

static PyObject *Qt_MovieImportGetFileType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	OSType fileType;
#ifndef MovieImportGetFileType
	PyMac_PRECHECK(MovieImportGetFileType);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = MovieImportGetFileType(ci,
	                             &fileType);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildOSType, fileType);
	return _res;
}

static PyObject *Qt_MovieImportDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	Handle dataRef;
	OSType dataRefType;
	Movie theMovie;
	Track targetTrack;
	Track usedTrack;
	TimeValue atTime;
	TimeValue addedDuration;
	long inFlags;
	long outFlags;
#ifndef MovieImportDataRef
	PyMac_PRECHECK(MovieImportDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&O&ll",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      MovieObj_Convert, &theMovie,
	                      TrackObj_Convert, &targetTrack,
	                      &atTime,
	                      &inFlags))
		return NULL;
	_rv = MovieImportDataRef(ci,
	                         dataRef,
	                         dataRefType,
	                         theMovie,
	                         targetTrack,
	                         &usedTrack,
	                         atTime,
	                         &addedDuration,
	                         inFlags,
	                         &outFlags);
	_res = Py_BuildValue("lO&ll",
	                     _rv,
	                     TrackObj_New, usedTrack,
	                     addedDuration,
	                     outFlags);
	return _res;
}

static PyObject *Qt_MovieImportGetSampleDescription(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	SampleDescriptionHandle desc;
	OSType mediaType;
#ifndef MovieImportGetSampleDescription
	PyMac_PRECHECK(MovieImportGetSampleDescription);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = MovieImportGetSampleDescription(ci,
	                                      &desc,
	                                      &mediaType);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, desc,
	                     PyMac_BuildOSType, mediaType);
	return _res;
}

static PyObject *Qt_MovieImportSetOffsetAndLimit(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	unsigned long offset;
	unsigned long limit;
#ifndef MovieImportSetOffsetAndLimit
	PyMac_PRECHECK(MovieImportSetOffsetAndLimit);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &ci,
	                      &offset,
	                      &limit))
		return NULL;
	_rv = MovieImportSetOffsetAndLimit(ci,
	                                   offset,
	                                   limit);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportSetOffsetAndLimit64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	wide offset;
	wide limit;
#ifndef MovieImportSetOffsetAndLimit64
	PyMac_PRECHECK(MovieImportSetOffsetAndLimit64);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_Getwide, &offset,
	                      PyMac_Getwide, &limit))
		return NULL;
	_rv = MovieImportSetOffsetAndLimit64(ci,
	                                     &offset,
	                                     &limit);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportIdle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	long inFlags;
	long outFlags;
#ifndef MovieImportIdle
	PyMac_PRECHECK(MovieImportIdle);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &inFlags))
		return NULL;
	_rv = MovieImportIdle(ci,
	                      inFlags,
	                      &outFlags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     outFlags);
	return _res;
}

static PyObject *Qt_MovieImportValidateDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	Handle dataRef;
	OSType dataRefType;
	UInt8 valid;
#ifndef MovieImportValidateDataRef
	PyMac_PRECHECK(MovieImportValidateDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_rv = MovieImportValidateDataRef(ci,
	                                 dataRef,
	                                 dataRefType,
	                                 &valid);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     valid);
	return _res;
}

static PyObject *Qt_MovieImportGetLoadState(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	long importerLoadState;
#ifndef MovieImportGetLoadState
	PyMac_PRECHECK(MovieImportGetLoadState);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = MovieImportGetLoadState(ci,
	                              &importerLoadState);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     importerLoadState);
	return _res;
}

static PyObject *Qt_MovieImportGetMaxLoadedTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	TimeValue time;
#ifndef MovieImportGetMaxLoadedTime
	PyMac_PRECHECK(MovieImportGetMaxLoadedTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = MovieImportGetMaxLoadedTime(ci,
	                                  &time);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     time);
	return _res;
}

static PyObject *Qt_MovieImportEstimateCompletionTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	TimeRecord time;
#ifndef MovieImportEstimateCompletionTime
	PyMac_PRECHECK(MovieImportEstimateCompletionTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = MovieImportEstimateCompletionTime(ci,
	                                        &time);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     QtTimeRecord_New, &time);
	return _res;
}

static PyObject *Qt_MovieImportSetDontBlock(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	Boolean dontBlock;
#ifndef MovieImportSetDontBlock
	PyMac_PRECHECK(MovieImportSetDontBlock);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &ci,
	                      &dontBlock))
		return NULL;
	_rv = MovieImportSetDontBlock(ci,
	                              dontBlock);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportGetDontBlock(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	Boolean willBlock;
#ifndef MovieImportGetDontBlock
	PyMac_PRECHECK(MovieImportGetDontBlock);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = MovieImportGetDontBlock(ci,
	                              &willBlock);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     willBlock);
	return _res;
}

static PyObject *Qt_MovieImportSetIdleManager(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	IdleManager im;
#ifndef MovieImportSetIdleManager
	PyMac_PRECHECK(MovieImportSetIdleManager);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      IdleManagerObj_Convert, &im))
		return NULL;
	_rv = MovieImportSetIdleManager(ci,
	                                im);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportSetNewMovieFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	long newMovieFlags;
#ifndef MovieImportSetNewMovieFlags
	PyMac_PRECHECK(MovieImportSetNewMovieFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &newMovieFlags))
		return NULL;
	_rv = MovieImportSetNewMovieFlags(ci,
	                                  newMovieFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieImportGetDestinationMediaType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieImportComponent ci;
	OSType mediaType;
#ifndef MovieImportGetDestinationMediaType
	PyMac_PRECHECK(MovieImportGetDestinationMediaType);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = MovieImportGetDestinationMediaType(ci,
	                                         &mediaType);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildOSType, mediaType);
	return _res;
}

static PyObject *Qt_MovieExportToHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieExportComponent ci;
	Handle dataH;
	Movie theMovie;
	Track onlyThisTrack;
	TimeValue startTime;
	TimeValue duration;
#ifndef MovieExportToHandle
	PyMac_PRECHECK(MovieExportToHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&ll",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &dataH,
	                      MovieObj_Convert, &theMovie,
	                      TrackObj_Convert, &onlyThisTrack,
	                      &startTime,
	                      &duration))
		return NULL;
	_rv = MovieExportToHandle(ci,
	                          dataH,
	                          theMovie,
	                          onlyThisTrack,
	                          startTime,
	                          duration);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieExportToFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieExportComponent ci;
	FSSpec theFile;
	Movie theMovie;
	Track onlyThisTrack;
	TimeValue startTime;
	TimeValue duration;
#ifndef MovieExportToFile
	PyMac_PRECHECK(MovieExportToFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&ll",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFSSpec, &theFile,
	                      MovieObj_Convert, &theMovie,
	                      TrackObj_Convert, &onlyThisTrack,
	                      &startTime,
	                      &duration))
		return NULL;
	_rv = MovieExportToFile(ci,
	                        &theFile,
	                        theMovie,
	                        onlyThisTrack,
	                        startTime,
	                        duration);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieExportGetAuxiliaryData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieExportComponent ci;
	Handle dataH;
	OSType handleType;
#ifndef MovieExportGetAuxiliaryData
	PyMac_PRECHECK(MovieExportGetAuxiliaryData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &dataH))
		return NULL;
	_rv = MovieExportGetAuxiliaryData(ci,
	                                  dataH,
	                                  &handleType);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildOSType, handleType);
	return _res;
}

static PyObject *Qt_MovieExportSetSampleDescription(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieExportComponent ci;
	SampleDescriptionHandle desc;
	OSType mediaType;
#ifndef MovieExportSetSampleDescription
	PyMac_PRECHECK(MovieExportSetSampleDescription);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &desc,
	                      PyMac_GetOSType, &mediaType))
		return NULL;
	_rv = MovieExportSetSampleDescription(ci,
	                                      desc,
	                                      mediaType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieExportDoUserDialog(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieExportComponent ci;
	Movie theMovie;
	Track onlyThisTrack;
	TimeValue startTime;
	TimeValue duration;
	Boolean canceled;
#ifndef MovieExportDoUserDialog
	PyMac_PRECHECK(MovieExportDoUserDialog);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&ll",
	                      CmpInstObj_Convert, &ci,
	                      MovieObj_Convert, &theMovie,
	                      TrackObj_Convert, &onlyThisTrack,
	                      &startTime,
	                      &duration))
		return NULL;
	_rv = MovieExportDoUserDialog(ci,
	                              theMovie,
	                              onlyThisTrack,
	                              startTime,
	                              duration,
	                              &canceled);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     canceled);
	return _res;
}

static PyObject *Qt_MovieExportGetCreatorType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieExportComponent ci;
	OSType creator;
#ifndef MovieExportGetCreatorType
	PyMac_PRECHECK(MovieExportGetCreatorType);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = MovieExportGetCreatorType(ci,
	                                &creator);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildOSType, creator);
	return _res;
}

static PyObject *Qt_MovieExportToDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieExportComponent ci;
	Handle dataRef;
	OSType dataRefType;
	Movie theMovie;
	Track onlyThisTrack;
	TimeValue startTime;
	TimeValue duration;
#ifndef MovieExportToDataRef
	PyMac_PRECHECK(MovieExportToDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&O&ll",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      MovieObj_Convert, &theMovie,
	                      TrackObj_Convert, &onlyThisTrack,
	                      &startTime,
	                      &duration))
		return NULL;
	_rv = MovieExportToDataRef(ci,
	                           dataRef,
	                           dataRefType,
	                           theMovie,
	                           onlyThisTrack,
	                           startTime,
	                           duration);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieExportFromProceduresToDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieExportComponent ci;
	Handle dataRef;
	OSType dataRefType;
#ifndef MovieExportFromProceduresToDataRef
	PyMac_PRECHECK(MovieExportFromProceduresToDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_rv = MovieExportFromProceduresToDataRef(ci,
	                                         dataRef,
	                                         dataRefType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieExportValidate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieExportComponent ci;
	Movie theMovie;
	Track onlyThisTrack;
	Boolean valid;
#ifndef MovieExportValidate
	PyMac_PRECHECK(MovieExportValidate);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      MovieObj_Convert, &theMovie,
	                      TrackObj_Convert, &onlyThisTrack))
		return NULL;
	_rv = MovieExportValidate(ci,
	                          theMovie,
	                          onlyThisTrack,
	                          &valid);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     valid);
	return _res;
}

static PyObject *Qt_MovieExportGetFileNameExtension(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieExportComponent ci;
	OSType extension;
#ifndef MovieExportGetFileNameExtension
	PyMac_PRECHECK(MovieExportGetFileNameExtension);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = MovieExportGetFileNameExtension(ci,
	                                      &extension);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildOSType, extension);
	return _res;
}

static PyObject *Qt_MovieExportGetShortFileTypeString(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieExportComponent ci;
	Str255 typeString;
#ifndef MovieExportGetShortFileTypeString
	PyMac_PRECHECK(MovieExportGetShortFileTypeString);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetStr255, typeString))
		return NULL;
	_rv = MovieExportGetShortFileTypeString(ci,
	                                        typeString);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MovieExportGetSourceMediaType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MovieExportComponent ci;
	OSType mediaType;
#ifndef MovieExportGetSourceMediaType
	PyMac_PRECHECK(MovieExportGetSourceMediaType);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = MovieExportGetSourceMediaType(ci,
	                                    &mediaType);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildOSType, mediaType);
	return _res;
}

static PyObject *Qt_TextExportGetTimeFraction(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TextExportComponent ci;
	long movieTimeFraction;
#ifndef TextExportGetTimeFraction
	PyMac_PRECHECK(TextExportGetTimeFraction);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = TextExportGetTimeFraction(ci,
	                                &movieTimeFraction);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     movieTimeFraction);
	return _res;
}

static PyObject *Qt_TextExportSetTimeFraction(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TextExportComponent ci;
	long movieTimeFraction;
#ifndef TextExportSetTimeFraction
	PyMac_PRECHECK(TextExportSetTimeFraction);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &movieTimeFraction))
		return NULL;
	_rv = TextExportSetTimeFraction(ci,
	                                movieTimeFraction);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TextExportGetSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TextExportComponent ci;
	long setting;
#ifndef TextExportGetSettings
	PyMac_PRECHECK(TextExportGetSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = TextExportGetSettings(ci,
	                            &setting);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     setting);
	return _res;
}

static PyObject *Qt_TextExportSetSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TextExportComponent ci;
	long setting;
#ifndef TextExportSetSettings
	PyMac_PRECHECK(TextExportSetSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &setting))
		return NULL;
	_rv = TextExportSetSettings(ci,
	                            setting);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MIDIImportGetSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TextExportComponent ci;
	long setting;
#ifndef MIDIImportGetSettings
	PyMac_PRECHECK(MIDIImportGetSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = MIDIImportGetSettings(ci,
	                            &setting);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     setting);
	return _res;
}

static PyObject *Qt_MIDIImportSetSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TextExportComponent ci;
	long setting;
#ifndef MIDIImportSetSettings
	PyMac_PRECHECK(MIDIImportSetSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &setting))
		return NULL;
	_rv = MIDIImportSetSettings(ci,
	                            setting);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImageImportSetSequenceEnabled(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicImageMovieImportComponent ci;
	Boolean enable;
#ifndef GraphicsImageImportSetSequenceEnabled
	PyMac_PRECHECK(GraphicsImageImportSetSequenceEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &ci,
	                      &enable))
		return NULL;
	_rv = GraphicsImageImportSetSequenceEnabled(ci,
	                                            enable);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_GraphicsImageImportGetSequenceEnabled(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	GraphicImageMovieImportComponent ci;
	Boolean enable;
#ifndef GraphicsImageImportGetSequenceEnabled
	PyMac_PRECHECK(GraphicsImageImportGetSequenceEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = GraphicsImageImportGetSequenceEnabled(ci,
	                                            &enable);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     enable);
	return _res;
}

static PyObject *Qt_PreviewShowData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	pnotComponent p;
	OSType dataType;
	Handle data;
	Rect inHere;
#ifndef PreviewShowData
	PyMac_PRECHECK(PreviewShowData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&",
	                      CmpInstObj_Convert, &p,
	                      PyMac_GetOSType, &dataType,
	                      ResObj_Convert, &data,
	                      PyMac_GetRect, &inHere))
		return NULL;
	_rv = PreviewShowData(p,
	                      dataType,
	                      data,
	                      &inHere);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_PreviewMakePreviewReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	pnotComponent p;
	OSType previewType;
	short resID;
	FSSpec sourceFile;
#ifndef PreviewMakePreviewReference
	PyMac_PRECHECK(PreviewMakePreviewReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &p,
	                      PyMac_GetFSSpec, &sourceFile))
		return NULL;
	_rv = PreviewMakePreviewReference(p,
	                                  &previewType,
	                                  &resID,
	                                  &sourceFile);
	_res = Py_BuildValue("lO&h",
	                     _rv,
	                     PyMac_BuildOSType, previewType,
	                     resID);
	return _res;
}

static PyObject *Qt_PreviewEvent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	pnotComponent p;
	EventRecord e;
	Boolean handledEvent;
#ifndef PreviewEvent
	PyMac_PRECHECK(PreviewEvent);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &p))
		return NULL;
	_rv = PreviewEvent(p,
	                   &e,
	                   &handledEvent);
	_res = Py_BuildValue("lO&b",
	                     _rv,
	                     PyMac_BuildEventRecord, &e,
	                     handledEvent);
	return _res;
}

static PyObject *Qt_DataCodecDecompress(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataCodecComponent dc;
	void * srcData;
	UInt32 srcSize;
	void * dstData;
	UInt32 dstBufferSize;
#ifndef DataCodecDecompress
	PyMac_PRECHECK(DataCodecDecompress);
#endif
	if (!PyArg_ParseTuple(_args, "O&slsl",
	                      CmpInstObj_Convert, &dc,
	                      &srcData,
	                      &srcSize,
	                      &dstData,
	                      &dstBufferSize))
		return NULL;
	_rv = DataCodecDecompress(dc,
	                          srcData,
	                          srcSize,
	                          dstData,
	                          dstBufferSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataCodecGetCompressBufferSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataCodecComponent dc;
	UInt32 srcSize;
	UInt32 dstSize;
#ifndef DataCodecGetCompressBufferSize
	PyMac_PRECHECK(DataCodecGetCompressBufferSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &dc,
	                      &srcSize))
		return NULL;
	_rv = DataCodecGetCompressBufferSize(dc,
	                                     srcSize,
	                                     &dstSize);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     dstSize);
	return _res;
}

static PyObject *Qt_DataCodecCompress(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataCodecComponent dc;
	void * srcData;
	UInt32 srcSize;
	void * dstData;
	UInt32 dstBufferSize;
	UInt32 actualDstSize;
	UInt32 decompressSlop;
#ifndef DataCodecCompress
	PyMac_PRECHECK(DataCodecCompress);
#endif
	if (!PyArg_ParseTuple(_args, "O&slsl",
	                      CmpInstObj_Convert, &dc,
	                      &srcData,
	                      &srcSize,
	                      &dstData,
	                      &dstBufferSize))
		return NULL;
	_rv = DataCodecCompress(dc,
	                        srcData,
	                        srcSize,
	                        dstData,
	                        dstBufferSize,
	                        &actualDstSize,
	                        &decompressSlop);
	_res = Py_BuildValue("lll",
	                     _rv,
	                     actualDstSize,
	                     decompressSlop);
	return _res;
}

static PyObject *Qt_DataCodecBeginInterruptSafe(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataCodecComponent dc;
	unsigned long maxSrcSize;
#ifndef DataCodecBeginInterruptSafe
	PyMac_PRECHECK(DataCodecBeginInterruptSafe);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &dc,
	                      &maxSrcSize))
		return NULL;
	_rv = DataCodecBeginInterruptSafe(dc,
	                                  maxSrcSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataCodecEndInterruptSafe(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataCodecComponent dc;
#ifndef DataCodecEndInterruptSafe
	PyMac_PRECHECK(DataCodecEndInterruptSafe);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dc))
		return NULL;
	_rv = DataCodecEndInterruptSafe(dc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Handle h;
	long hOffset;
	long offset;
	long size;
#ifndef DataHGetData
	PyMac_PRECHECK(DataHGetData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&lll",
	                      CmpInstObj_Convert, &dh,
	                      ResObj_Convert, &h,
	                      &hOffset,
	                      &offset,
	                      &size))
		return NULL;
	_rv = DataHGetData(dh,
	                   h,
	                   hOffset,
	                   offset,
	                   size);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHPutData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Handle h;
	long hOffset;
	long offset;
	long size;
#ifndef DataHPutData
	PyMac_PRECHECK(DataHPutData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&ll",
	                      CmpInstObj_Convert, &dh,
	                      ResObj_Convert, &h,
	                      &hOffset,
	                      &size))
		return NULL;
	_rv = DataHPutData(dh,
	                   h,
	                   hOffset,
	                   &offset,
	                   size);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     offset);
	return _res;
}

static PyObject *Qt_DataHFlushData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
#ifndef DataHFlushData
	PyMac_PRECHECK(DataHFlushData);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHFlushData(dh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHOpenForWrite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
#ifndef DataHOpenForWrite
	PyMac_PRECHECK(DataHOpenForWrite);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHOpenForWrite(dh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHCloseForWrite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
#ifndef DataHCloseForWrite
	PyMac_PRECHECK(DataHCloseForWrite);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHCloseForWrite(dh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHOpenForRead(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
#ifndef DataHOpenForRead
	PyMac_PRECHECK(DataHOpenForRead);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHOpenForRead(dh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHCloseForRead(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
#ifndef DataHCloseForRead
	PyMac_PRECHECK(DataHCloseForRead);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHCloseForRead(dh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHSetDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Handle dataRef;
#ifndef DataHSetDataRef
	PyMac_PRECHECK(DataHSetDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      ResObj_Convert, &dataRef))
		return NULL;
	_rv = DataHSetDataRef(dh,
	                      dataRef);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Handle dataRef;
#ifndef DataHGetDataRef
	PyMac_PRECHECK(DataHGetDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetDataRef(dh,
	                      &dataRef);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, dataRef);
	return _res;
}

static PyObject *Qt_DataHCompareDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Handle dataRef;
	Boolean equal;
#ifndef DataHCompareDataRef
	PyMac_PRECHECK(DataHCompareDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      ResObj_Convert, &dataRef))
		return NULL;
	_rv = DataHCompareDataRef(dh,
	                          dataRef,
	                          &equal);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     equal);
	return _res;
}

static PyObject *Qt_DataHTask(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
#ifndef DataHTask
	PyMac_PRECHECK(DataHTask);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHTask(dh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHFinishData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Ptr PlaceToPutDataPtr;
	Boolean Cancel;
#ifndef DataHFinishData
	PyMac_PRECHECK(DataHFinishData);
#endif
	if (!PyArg_ParseTuple(_args, "O&sb",
	                      CmpInstObj_Convert, &dh,
	                      &PlaceToPutDataPtr,
	                      &Cancel))
		return NULL;
	_rv = DataHFinishData(dh,
	                      PlaceToPutDataPtr,
	                      Cancel);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHFlushCache(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
#ifndef DataHFlushCache
	PyMac_PRECHECK(DataHFlushCache);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHFlushCache(dh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHResolveDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Handle theDataRef;
	Boolean wasChanged;
	Boolean userInterfaceAllowed;
#ifndef DataHResolveDataRef
	PyMac_PRECHECK(DataHResolveDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&b",
	                      CmpInstObj_Convert, &dh,
	                      ResObj_Convert, &theDataRef,
	                      &userInterfaceAllowed))
		return NULL;
	_rv = DataHResolveDataRef(dh,
	                          theDataRef,
	                          &wasChanged,
	                          userInterfaceAllowed);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     wasChanged);
	return _res;
}

static PyObject *Qt_DataHGetFileSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long fileSize;
#ifndef DataHGetFileSize
	PyMac_PRECHECK(DataHGetFileSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetFileSize(dh,
	                       &fileSize);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     fileSize);
	return _res;
}

static PyObject *Qt_DataHCanUseDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Handle dataRef;
	long useFlags;
#ifndef DataHCanUseDataRef
	PyMac_PRECHECK(DataHCanUseDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      ResObj_Convert, &dataRef))
		return NULL;
	_rv = DataHCanUseDataRef(dh,
	                         dataRef,
	                         &useFlags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     useFlags);
	return _res;
}

static PyObject *Qt_DataHPreextend(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	unsigned long maxToAdd;
	unsigned long spaceAdded;
#ifndef DataHPreextend
	PyMac_PRECHECK(DataHPreextend);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &dh,
	                      &maxToAdd))
		return NULL;
	_rv = DataHPreextend(dh,
	                     maxToAdd,
	                     &spaceAdded);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     spaceAdded);
	return _res;
}

static PyObject *Qt_DataHSetFileSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long fileSize;
#ifndef DataHSetFileSize
	PyMac_PRECHECK(DataHSetFileSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &dh,
	                      &fileSize))
		return NULL;
	_rv = DataHSetFileSize(dh,
	                       fileSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetFreeSpace(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	unsigned long freeSize;
#ifndef DataHGetFreeSpace
	PyMac_PRECHECK(DataHGetFreeSpace);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetFreeSpace(dh,
	                        &freeSize);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     freeSize);
	return _res;
}

static PyObject *Qt_DataHCreateFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	OSType creator;
	Boolean deleteExisting;
#ifndef DataHCreateFile
	PyMac_PRECHECK(DataHCreateFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&b",
	                      CmpInstObj_Convert, &dh,
	                      PyMac_GetOSType, &creator,
	                      &deleteExisting))
		return NULL;
	_rv = DataHCreateFile(dh,
	                      creator,
	                      deleteExisting);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetPreferredBlockSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long blockSize;
#ifndef DataHGetPreferredBlockSize
	PyMac_PRECHECK(DataHGetPreferredBlockSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetPreferredBlockSize(dh,
	                                 &blockSize);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     blockSize);
	return _res;
}

static PyObject *Qt_DataHGetDeviceIndex(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long deviceIndex;
#ifndef DataHGetDeviceIndex
	PyMac_PRECHECK(DataHGetDeviceIndex);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetDeviceIndex(dh,
	                          &deviceIndex);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     deviceIndex);
	return _res;
}

static PyObject *Qt_DataHIsStreamingDataHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Boolean yes;
#ifndef DataHIsStreamingDataHandler
	PyMac_PRECHECK(DataHIsStreamingDataHandler);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHIsStreamingDataHandler(dh,
	                                  &yes);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     yes);
	return _res;
}

static PyObject *Qt_DataHGetDataInBuffer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long startOffset;
	long size;
#ifndef DataHGetDataInBuffer
	PyMac_PRECHECK(DataHGetDataInBuffer);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &dh,
	                      &startOffset))
		return NULL;
	_rv = DataHGetDataInBuffer(dh,
	                           startOffset,
	                           &size);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     size);
	return _res;
}

static PyObject *Qt_DataHGetScheduleAheadTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long millisecs;
#ifndef DataHGetScheduleAheadTime
	PyMac_PRECHECK(DataHGetScheduleAheadTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetScheduleAheadTime(dh,
	                                &millisecs);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     millisecs);
	return _res;
}

static PyObject *Qt_DataHSetCacheSizeLimit(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Size cacheSizeLimit;
#ifndef DataHSetCacheSizeLimit
	PyMac_PRECHECK(DataHSetCacheSizeLimit);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &dh,
	                      &cacheSizeLimit))
		return NULL;
	_rv = DataHSetCacheSizeLimit(dh,
	                             cacheSizeLimit);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetCacheSizeLimit(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Size cacheSizeLimit;
#ifndef DataHGetCacheSizeLimit
	PyMac_PRECHECK(DataHGetCacheSizeLimit);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetCacheSizeLimit(dh,
	                             &cacheSizeLimit);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     cacheSizeLimit);
	return _res;
}

static PyObject *Qt_DataHGetMovie(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Movie theMovie;
	short id;
#ifndef DataHGetMovie
	PyMac_PRECHECK(DataHGetMovie);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetMovie(dh,
	                    &theMovie,
	                    &id);
	_res = Py_BuildValue("lO&h",
	                     _rv,
	                     MovieObj_New, theMovie,
	                     id);
	return _res;
}

static PyObject *Qt_DataHAddMovie(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Movie theMovie;
	short id;
#ifndef DataHAddMovie
	PyMac_PRECHECK(DataHAddMovie);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      MovieObj_Convert, &theMovie))
		return NULL;
	_rv = DataHAddMovie(dh,
	                    theMovie,
	                    &id);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     id);
	return _res;
}

static PyObject *Qt_DataHUpdateMovie(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Movie theMovie;
	short id;
#ifndef DataHUpdateMovie
	PyMac_PRECHECK(DataHUpdateMovie);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      CmpInstObj_Convert, &dh,
	                      MovieObj_Convert, &theMovie,
	                      &id))
		return NULL;
	_rv = DataHUpdateMovie(dh,
	                       theMovie,
	                       id);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHDoesBuffer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Boolean buffersReads;
	Boolean buffersWrites;
#ifndef DataHDoesBuffer
	PyMac_PRECHECK(DataHDoesBuffer);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHDoesBuffer(dh,
	                      &buffersReads,
	                      &buffersWrites);
	_res = Py_BuildValue("lbb",
	                     _rv,
	                     buffersReads,
	                     buffersWrites);
	return _res;
}

static PyObject *Qt_DataHGetFileName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Str255 str;
#ifndef DataHGetFileName
	PyMac_PRECHECK(DataHGetFileName);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      PyMac_GetStr255, str))
		return NULL;
	_rv = DataHGetFileName(dh,
	                       str);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetAvailableFileSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long fileSize;
#ifndef DataHGetAvailableFileSize
	PyMac_PRECHECK(DataHGetAvailableFileSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetAvailableFileSize(dh,
	                                &fileSize);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     fileSize);
	return _res;
}

static PyObject *Qt_DataHGetMacOSFileType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	OSType fileType;
#ifndef DataHGetMacOSFileType
	PyMac_PRECHECK(DataHGetMacOSFileType);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetMacOSFileType(dh,
	                            &fileType);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildOSType, fileType);
	return _res;
}

static PyObject *Qt_DataHGetMIMEType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Str255 mimeType;
#ifndef DataHGetMIMEType
	PyMac_PRECHECK(DataHGetMIMEType);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      PyMac_GetStr255, mimeType))
		return NULL;
	_rv = DataHGetMIMEType(dh,
	                       mimeType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHSetDataRefWithAnchor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Handle anchorDataRef;
	OSType dataRefType;
	Handle dataRef;
#ifndef DataHSetDataRefWithAnchor
	PyMac_PRECHECK(DataHSetDataRefWithAnchor);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&",
	                      CmpInstObj_Convert, &dh,
	                      ResObj_Convert, &anchorDataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      ResObj_Convert, &dataRef))
		return NULL;
	_rv = DataHSetDataRefWithAnchor(dh,
	                                anchorDataRef,
	                                dataRefType,
	                                dataRef);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetDataRefWithAnchor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Handle anchorDataRef;
	OSType dataRefType;
	Handle dataRef;
#ifndef DataHGetDataRefWithAnchor
	PyMac_PRECHECK(DataHGetDataRefWithAnchor);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &dh,
	                      ResObj_Convert, &anchorDataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_rv = DataHGetDataRefWithAnchor(dh,
	                                anchorDataRef,
	                                dataRefType,
	                                &dataRef);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, dataRef);
	return _res;
}

static PyObject *Qt_DataHSetMacOSFileType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	OSType fileType;
#ifndef DataHSetMacOSFileType
	PyMac_PRECHECK(DataHSetMacOSFileType);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      PyMac_GetOSType, &fileType))
		return NULL;
	_rv = DataHSetMacOSFileType(dh,
	                            fileType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHSetTimeBase(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	TimeBase tb;
#ifndef DataHSetTimeBase
	PyMac_PRECHECK(DataHSetTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      TimeBaseObj_Convert, &tb))
		return NULL;
	_rv = DataHSetTimeBase(dh,
	                       tb);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetInfoFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	UInt32 flags;
#ifndef DataHGetInfoFlags
	PyMac_PRECHECK(DataHGetInfoFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetInfoFlags(dh,
	                        &flags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     flags);
	return _res;
}

static PyObject *Qt_DataHGetFileSize64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	wide fileSize;
#ifndef DataHGetFileSize64
	PyMac_PRECHECK(DataHGetFileSize64);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetFileSize64(dh,
	                         &fileSize);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_Buildwide, fileSize);
	return _res;
}

static PyObject *Qt_DataHPreextend64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	wide maxToAdd;
	wide spaceAdded;
#ifndef DataHPreextend64
	PyMac_PRECHECK(DataHPreextend64);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      PyMac_Getwide, &maxToAdd))
		return NULL;
	_rv = DataHPreextend64(dh,
	                       &maxToAdd,
	                       &spaceAdded);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_Buildwide, spaceAdded);
	return _res;
}

static PyObject *Qt_DataHSetFileSize64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	wide fileSize;
#ifndef DataHSetFileSize64
	PyMac_PRECHECK(DataHSetFileSize64);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      PyMac_Getwide, &fileSize))
		return NULL;
	_rv = DataHSetFileSize64(dh,
	                         &fileSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetFreeSpace64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	wide freeSize;
#ifndef DataHGetFreeSpace64
	PyMac_PRECHECK(DataHGetFreeSpace64);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetFreeSpace64(dh,
	                          &freeSize);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_Buildwide, freeSize);
	return _res;
}

static PyObject *Qt_DataHAppend64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	void * data;
	wide fileOffset;
	unsigned long size;
#ifndef DataHAppend64
	PyMac_PRECHECK(DataHAppend64);
#endif
	if (!PyArg_ParseTuple(_args, "O&sl",
	                      CmpInstObj_Convert, &dh,
	                      &data,
	                      &size))
		return NULL;
	_rv = DataHAppend64(dh,
	                    data,
	                    &fileOffset,
	                    size);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_Buildwide, fileOffset);
	return _res;
}

static PyObject *Qt_DataHPollRead(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	void * dataPtr;
	UInt32 dataSizeSoFar;
#ifndef DataHPollRead
	PyMac_PRECHECK(DataHPollRead);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &dh,
	                      &dataPtr))
		return NULL;
	_rv = DataHPollRead(dh,
	                    dataPtr,
	                    &dataSizeSoFar);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     dataSizeSoFar);
	return _res;
}

static PyObject *Qt_DataHGetDataAvailability(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long offset;
	long len;
	long missing_offset;
	long missing_len;
#ifndef DataHGetDataAvailability
	PyMac_PRECHECK(DataHGetDataAvailability);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &dh,
	                      &offset,
	                      &len))
		return NULL;
	_rv = DataHGetDataAvailability(dh,
	                               offset,
	                               len,
	                               &missing_offset,
	                               &missing_len);
	_res = Py_BuildValue("lll",
	                     _rv,
	                     missing_offset,
	                     missing_len);
	return _res;
}

static PyObject *Qt_DataHGetDataRefAsType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	OSType requestedType;
	Handle dataRef;
#ifndef DataHGetDataRefAsType
	PyMac_PRECHECK(DataHGetDataRefAsType);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      PyMac_GetOSType, &requestedType))
		return NULL;
	_rv = DataHGetDataRefAsType(dh,
	                            requestedType,
	                            &dataRef);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, dataRef);
	return _res;
}

static PyObject *Qt_DataHSetDataRefExtension(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Handle extension;
	OSType idType;
#ifndef DataHSetDataRefExtension
	PyMac_PRECHECK(DataHSetDataRefExtension);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &dh,
	                      ResObj_Convert, &extension,
	                      PyMac_GetOSType, &idType))
		return NULL;
	_rv = DataHSetDataRefExtension(dh,
	                               extension,
	                               idType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetDataRefExtension(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Handle extension;
	OSType idType;
#ifndef DataHGetDataRefExtension
	PyMac_PRECHECK(DataHGetDataRefExtension);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      PyMac_GetOSType, &idType))
		return NULL;
	_rv = DataHGetDataRefExtension(dh,
	                               &extension,
	                               idType);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, extension);
	return _res;
}

static PyObject *Qt_DataHGetMovieWithFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Movie theMovie;
	short id;
	short flags;
#ifndef DataHGetMovieWithFlags
	PyMac_PRECHECK(DataHGetMovieWithFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &dh,
	                      &flags))
		return NULL;
	_rv = DataHGetMovieWithFlags(dh,
	                             &theMovie,
	                             &id,
	                             flags);
	_res = Py_BuildValue("lO&h",
	                     _rv,
	                     MovieObj_New, theMovie,
	                     id);
	return _res;
}

static PyObject *Qt_DataHGetFileTypeOrdering(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	DataHFileTypeOrderingHandle orderingListHandle;
#ifndef DataHGetFileTypeOrdering
	PyMac_PRECHECK(DataHGetFileTypeOrdering);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetFileTypeOrdering(dh,
	                               &orderingListHandle);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, orderingListHandle);
	return _res;
}

static PyObject *Qt_DataHCreateFileWithFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	OSType creator;
	Boolean deleteExisting;
	UInt32 flags;
#ifndef DataHCreateFileWithFlags
	PyMac_PRECHECK(DataHCreateFileWithFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&bl",
	                      CmpInstObj_Convert, &dh,
	                      PyMac_GetOSType, &creator,
	                      &deleteExisting,
	                      &flags))
		return NULL;
	_rv = DataHCreateFileWithFlags(dh,
	                               creator,
	                               deleteExisting,
	                               flags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	OSType what;
	void * info;
#ifndef DataHGetInfo
	PyMac_PRECHECK(DataHGetInfo);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&s",
	                      CmpInstObj_Convert, &dh,
	                      PyMac_GetOSType, &what,
	                      &info))
		return NULL;
	_rv = DataHGetInfo(dh,
	                   what,
	                   info);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHSetIdleManager(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	IdleManager im;
#ifndef DataHSetIdleManager
	PyMac_PRECHECK(DataHSetIdleManager);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      IdleManagerObj_Convert, &im))
		return NULL;
	_rv = DataHSetIdleManager(dh,
	                          im);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHDeleteFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
#ifndef DataHDeleteFile
	PyMac_PRECHECK(DataHDeleteFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHDeleteFile(dh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHSetMovieUsageFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long flags;
#ifndef DataHSetMovieUsageFlags
	PyMac_PRECHECK(DataHSetMovieUsageFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &dh,
	                      &flags))
		return NULL;
	_rv = DataHSetMovieUsageFlags(dh,
	                              flags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHUseTemporaryDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long inFlags;
#ifndef DataHUseTemporaryDataRef
	PyMac_PRECHECK(DataHUseTemporaryDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &dh,
	                      &inFlags))
		return NULL;
	_rv = DataHUseTemporaryDataRef(dh,
	                               inFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetTemporaryDataRefCapabilities(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long outUnderstoodFlags;
#ifndef DataHGetTemporaryDataRefCapabilities
	PyMac_PRECHECK(DataHGetTemporaryDataRefCapabilities);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &dh))
		return NULL;
	_rv = DataHGetTemporaryDataRefCapabilities(dh,
	                                           &outUnderstoodFlags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     outUnderstoodFlags);
	return _res;
}

static PyObject *Qt_DataHRenameFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	Handle newDataRef;
#ifndef DataHRenameFile
	PyMac_PRECHECK(DataHRenameFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &dh,
	                      ResObj_Convert, &newDataRef))
		return NULL;
	_rv = DataHRenameFile(dh,
	                      newDataRef);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHPlaybackHints(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long flags;
	unsigned long minFileOffset;
	unsigned long maxFileOffset;
	long bytesPerSecond;
#ifndef DataHPlaybackHints
	PyMac_PRECHECK(DataHPlaybackHints);
#endif
	if (!PyArg_ParseTuple(_args, "O&llll",
	                      CmpInstObj_Convert, &dh,
	                      &flags,
	                      &minFileOffset,
	                      &maxFileOffset,
	                      &bytesPerSecond))
		return NULL;
	_rv = DataHPlaybackHints(dh,
	                         flags,
	                         minFileOffset,
	                         maxFileOffset,
	                         bytesPerSecond);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHPlaybackHints64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long flags;
	wide minFileOffset;
	wide maxFileOffset;
	long bytesPerSecond;
#ifndef DataHPlaybackHints64
	PyMac_PRECHECK(DataHPlaybackHints64);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&O&l",
	                      CmpInstObj_Convert, &dh,
	                      &flags,
	                      PyMac_Getwide, &minFileOffset,
	                      PyMac_Getwide, &maxFileOffset,
	                      &bytesPerSecond))
		return NULL;
	_rv = DataHPlaybackHints64(dh,
	                           flags,
	                           &minFileOffset,
	                           &maxFileOffset,
	                           bytesPerSecond);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_DataHGetDataRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long flags;
	long bytesPerSecond;
#ifndef DataHGetDataRate
	PyMac_PRECHECK(DataHGetDataRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &dh,
	                      &flags))
		return NULL;
	_rv = DataHGetDataRate(dh,
	                       flags,
	                       &bytesPerSecond);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     bytesPerSecond);
	return _res;
}

static PyObject *Qt_DataHSetTimeHints(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	DataHandler dh;
	long flags;
	long bandwidthPriority;
	TimeScale scale;
	TimeValue minTime;
	TimeValue maxTime;
#ifndef DataHSetTimeHints
	PyMac_PRECHECK(DataHSetTimeHints);
#endif
	if (!PyArg_ParseTuple(_args, "O&lllll",
	                      CmpInstObj_Convert, &dh,
	                      &flags,
	                      &bandwidthPriority,
	                      &scale,
	                      &minTime,
	                      &maxTime))
		return NULL;
	_rv = DataHSetTimeHints(dh,
	                        flags,
	                        bandwidthPriority,
	                        scale,
	                        minTime,
	                        maxTime);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetMaxSrcRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short inputStd;
	Rect maxSrcRect;
#ifndef VDGetMaxSrcRect
	PyMac_PRECHECK(VDGetMaxSrcRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &inputStd))
		return NULL;
	_rv = VDGetMaxSrcRect(ci,
	                      inputStd,
	                      &maxSrcRect);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &maxSrcRect);
	return _res;
}

static PyObject *Qt_VDGetActiveSrcRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short inputStd;
	Rect activeSrcRect;
#ifndef VDGetActiveSrcRect
	PyMac_PRECHECK(VDGetActiveSrcRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &inputStd))
		return NULL;
	_rv = VDGetActiveSrcRect(ci,
	                         inputStd,
	                         &activeSrcRect);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &activeSrcRect);
	return _res;
}

static PyObject *Qt_VDSetDigitizerRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	Rect digitizerRect;
#ifndef VDSetDigitizerRect
	PyMac_PRECHECK(VDSetDigitizerRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDSetDigitizerRect(ci,
	                         &digitizerRect);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &digitizerRect);
	return _res;
}

static PyObject *Qt_VDGetDigitizerRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	Rect digitizerRect;
#ifndef VDGetDigitizerRect
	PyMac_PRECHECK(VDGetDigitizerRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetDigitizerRect(ci,
	                         &digitizerRect);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &digitizerRect);
	return _res;
}

static PyObject *Qt_VDGetVBlankRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short inputStd;
	Rect vBlankRect;
#ifndef VDGetVBlankRect
	PyMac_PRECHECK(VDGetVBlankRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &inputStd))
		return NULL;
	_rv = VDGetVBlankRect(ci,
	                      inputStd,
	                      &vBlankRect);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &vBlankRect);
	return _res;
}

static PyObject *Qt_VDGetMaskPixMap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	PixMapHandle maskPixMap;
#ifndef VDGetMaskPixMap
	PyMac_PRECHECK(VDGetMaskPixMap);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &maskPixMap))
		return NULL;
	_rv = VDGetMaskPixMap(ci,
	                      maskPixMap);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDUseThisCLUT(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	CTabHandle colorTableHandle;
#ifndef VDUseThisCLUT
	PyMac_PRECHECK(VDUseThisCLUT);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &colorTableHandle))
		return NULL;
	_rv = VDUseThisCLUT(ci,
	                    colorTableHandle);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDSetInputGammaValue(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	Fixed channel1;
	Fixed channel2;
	Fixed channel3;
#ifndef VDSetInputGammaValue
	PyMac_PRECHECK(VDSetInputGammaValue);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFixed, &channel1,
	                      PyMac_GetFixed, &channel2,
	                      PyMac_GetFixed, &channel3))
		return NULL;
	_rv = VDSetInputGammaValue(ci,
	                           channel1,
	                           channel2,
	                           channel3);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetInputGammaValue(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	Fixed channel1;
	Fixed channel2;
	Fixed channel3;
#ifndef VDGetInputGammaValue
	PyMac_PRECHECK(VDGetInputGammaValue);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetInputGammaValue(ci,
	                           &channel1,
	                           &channel2,
	                           &channel3);
	_res = Py_BuildValue("lO&O&O&",
	                     _rv,
	                     PyMac_BuildFixed, channel1,
	                     PyMac_BuildFixed, channel2,
	                     PyMac_BuildFixed, channel3);
	return _res;
}

static PyObject *Qt_VDSetBrightness(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short brightness;
#ifndef VDSetBrightness
	PyMac_PRECHECK(VDSetBrightness);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDSetBrightness(ci,
	                      &brightness);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     brightness);
	return _res;
}

static PyObject *Qt_VDGetBrightness(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short brightness;
#ifndef VDGetBrightness
	PyMac_PRECHECK(VDGetBrightness);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetBrightness(ci,
	                      &brightness);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     brightness);
	return _res;
}

static PyObject *Qt_VDSetContrast(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short contrast;
#ifndef VDSetContrast
	PyMac_PRECHECK(VDSetContrast);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDSetContrast(ci,
	                    &contrast);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     contrast);
	return _res;
}

static PyObject *Qt_VDSetHue(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short hue;
#ifndef VDSetHue
	PyMac_PRECHECK(VDSetHue);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDSetHue(ci,
	               &hue);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     hue);
	return _res;
}

static PyObject *Qt_VDSetSharpness(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short sharpness;
#ifndef VDSetSharpness
	PyMac_PRECHECK(VDSetSharpness);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDSetSharpness(ci,
	                     &sharpness);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     sharpness);
	return _res;
}

static PyObject *Qt_VDSetSaturation(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short saturation;
#ifndef VDSetSaturation
	PyMac_PRECHECK(VDSetSaturation);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDSetSaturation(ci,
	                      &saturation);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     saturation);
	return _res;
}

static PyObject *Qt_VDGetContrast(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short contrast;
#ifndef VDGetContrast
	PyMac_PRECHECK(VDGetContrast);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetContrast(ci,
	                    &contrast);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     contrast);
	return _res;
}

static PyObject *Qt_VDGetHue(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short hue;
#ifndef VDGetHue
	PyMac_PRECHECK(VDGetHue);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetHue(ci,
	               &hue);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     hue);
	return _res;
}

static PyObject *Qt_VDGetSharpness(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short sharpness;
#ifndef VDGetSharpness
	PyMac_PRECHECK(VDGetSharpness);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetSharpness(ci,
	                     &sharpness);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     sharpness);
	return _res;
}

static PyObject *Qt_VDGetSaturation(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short saturation;
#ifndef VDGetSaturation
	PyMac_PRECHECK(VDGetSaturation);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetSaturation(ci,
	                      &saturation);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     saturation);
	return _res;
}

static PyObject *Qt_VDGrabOneFrame(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
#ifndef VDGrabOneFrame
	PyMac_PRECHECK(VDGrabOneFrame);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGrabOneFrame(ci);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetMaxAuxBuffer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	PixMapHandle pm;
	Rect r;
#ifndef VDGetMaxAuxBuffer
	PyMac_PRECHECK(VDGetMaxAuxBuffer);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetMaxAuxBuffer(ci,
	                        &pm,
	                        &r);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, pm,
	                     PyMac_BuildRect, &r);
	return _res;
}

static PyObject *Qt_VDGetCurrentFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long inputCurrentFlag;
	long outputCurrentFlag;
#ifndef VDGetCurrentFlags
	PyMac_PRECHECK(VDGetCurrentFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetCurrentFlags(ci,
	                        &inputCurrentFlag,
	                        &outputCurrentFlag);
	_res = Py_BuildValue("lll",
	                     _rv,
	                     inputCurrentFlag,
	                     outputCurrentFlag);
	return _res;
}

static PyObject *Qt_VDSetKeyColor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long index;
#ifndef VDSetKeyColor
	PyMac_PRECHECK(VDSetKeyColor);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &index))
		return NULL;
	_rv = VDSetKeyColor(ci,
	                    index);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetKeyColor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long index;
#ifndef VDGetKeyColor
	PyMac_PRECHECK(VDGetKeyColor);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetKeyColor(ci,
	                    &index);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     index);
	return _res;
}

static PyObject *Qt_VDAddKeyColor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long index;
#ifndef VDAddKeyColor
	PyMac_PRECHECK(VDAddKeyColor);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDAddKeyColor(ci,
	                    &index);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     index);
	return _res;
}

static PyObject *Qt_VDGetNextKeyColor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long index;
#ifndef VDGetNextKeyColor
	PyMac_PRECHECK(VDGetNextKeyColor);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &index))
		return NULL;
	_rv = VDGetNextKeyColor(ci,
	                        index);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDSetKeyColorRange(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	RGBColor minRGB;
	RGBColor maxRGB;
#ifndef VDSetKeyColorRange
	PyMac_PRECHECK(VDSetKeyColorRange);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDSetKeyColorRange(ci,
	                         &minRGB,
	                         &maxRGB);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     QdRGB_New, &minRGB,
	                     QdRGB_New, &maxRGB);
	return _res;
}

static PyObject *Qt_VDGetKeyColorRange(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	RGBColor minRGB;
	RGBColor maxRGB;
#ifndef VDGetKeyColorRange
	PyMac_PRECHECK(VDGetKeyColorRange);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetKeyColorRange(ci,
	                         &minRGB,
	                         &maxRGB);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     QdRGB_New, &minRGB,
	                     QdRGB_New, &maxRGB);
	return _res;
}

static PyObject *Qt_VDSetInputColorSpaceMode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short colorSpaceMode;
#ifndef VDSetInputColorSpaceMode
	PyMac_PRECHECK(VDSetInputColorSpaceMode);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &colorSpaceMode))
		return NULL;
	_rv = VDSetInputColorSpaceMode(ci,
	                               colorSpaceMode);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetInputColorSpaceMode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short colorSpaceMode;
#ifndef VDGetInputColorSpaceMode
	PyMac_PRECHECK(VDGetInputColorSpaceMode);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetInputColorSpaceMode(ci,
	                               &colorSpaceMode);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     colorSpaceMode);
	return _res;
}

static PyObject *Qt_VDSetClipState(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short clipEnable;
#ifndef VDSetClipState
	PyMac_PRECHECK(VDSetClipState);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &clipEnable))
		return NULL;
	_rv = VDSetClipState(ci,
	                     clipEnable);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetClipState(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short clipEnable;
#ifndef VDGetClipState
	PyMac_PRECHECK(VDGetClipState);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetClipState(ci,
	                     &clipEnable);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     clipEnable);
	return _res;
}

static PyObject *Qt_VDSetClipRgn(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	RgnHandle clipRegion;
#ifndef VDSetClipRgn
	PyMac_PRECHECK(VDSetClipRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &clipRegion))
		return NULL;
	_rv = VDSetClipRgn(ci,
	                   clipRegion);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDClearClipRgn(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	RgnHandle clipRegion;
#ifndef VDClearClipRgn
	PyMac_PRECHECK(VDClearClipRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &clipRegion))
		return NULL;
	_rv = VDClearClipRgn(ci,
	                     clipRegion);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetCLUTInUse(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	CTabHandle colorTableHandle;
#ifndef VDGetCLUTInUse
	PyMac_PRECHECK(VDGetCLUTInUse);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetCLUTInUse(ci,
	                     &colorTableHandle);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, colorTableHandle);
	return _res;
}

static PyObject *Qt_VDSetPLLFilterType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short pllType;
#ifndef VDSetPLLFilterType
	PyMac_PRECHECK(VDSetPLLFilterType);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &pllType))
		return NULL;
	_rv = VDSetPLLFilterType(ci,
	                         pllType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetPLLFilterType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short pllType;
#ifndef VDGetPLLFilterType
	PyMac_PRECHECK(VDGetPLLFilterType);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetPLLFilterType(ci,
	                         &pllType);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     pllType);
	return _res;
}

static PyObject *Qt_VDGetMaskandValue(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short blendLevel;
	long mask;
	long value;
#ifndef VDGetMaskandValue
	PyMac_PRECHECK(VDGetMaskandValue);
#endif
	if (!PyArg_ParseTuple(_args, "O&H",
	                      CmpInstObj_Convert, &ci,
	                      &blendLevel))
		return NULL;
	_rv = VDGetMaskandValue(ci,
	                        blendLevel,
	                        &mask,
	                        &value);
	_res = Py_BuildValue("lll",
	                     _rv,
	                     mask,
	                     value);
	return _res;
}

static PyObject *Qt_VDSetMasterBlendLevel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short blendLevel;
#ifndef VDSetMasterBlendLevel
	PyMac_PRECHECK(VDSetMasterBlendLevel);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDSetMasterBlendLevel(ci,
	                            &blendLevel);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     blendLevel);
	return _res;
}

static PyObject *Qt_VDSetPlayThruOnOff(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short state;
#ifndef VDSetPlayThruOnOff
	PyMac_PRECHECK(VDSetPlayThruOnOff);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &state))
		return NULL;
	_rv = VDSetPlayThruOnOff(ci,
	                         state);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDSetFieldPreference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short fieldFlag;
#ifndef VDSetFieldPreference
	PyMac_PRECHECK(VDSetFieldPreference);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &fieldFlag))
		return NULL;
	_rv = VDSetFieldPreference(ci,
	                           fieldFlag);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetFieldPreference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short fieldFlag;
#ifndef VDGetFieldPreference
	PyMac_PRECHECK(VDGetFieldPreference);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetFieldPreference(ci,
	                           &fieldFlag);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     fieldFlag);
	return _res;
}

static PyObject *Qt_VDPreflightGlobalRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	GrafPtr theWindow;
	Rect globalRect;
#ifndef VDPreflightGlobalRect
	PyMac_PRECHECK(VDPreflightGlobalRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      GrafObj_Convert, &theWindow))
		return NULL;
	_rv = VDPreflightGlobalRect(ci,
	                            theWindow,
	                            &globalRect);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &globalRect);
	return _res;
}

static PyObject *Qt_VDSetPlayThruGlobalRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	GrafPtr theWindow;
	Rect globalRect;
#ifndef VDSetPlayThruGlobalRect
	PyMac_PRECHECK(VDSetPlayThruGlobalRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      GrafObj_Convert, &theWindow))
		return NULL;
	_rv = VDSetPlayThruGlobalRect(ci,
	                              theWindow,
	                              &globalRect);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &globalRect);
	return _res;
}

static PyObject *Qt_VDSetBlackLevelValue(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short blackLevel;
#ifndef VDSetBlackLevelValue
	PyMac_PRECHECK(VDSetBlackLevelValue);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDSetBlackLevelValue(ci,
	                           &blackLevel);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     blackLevel);
	return _res;
}

static PyObject *Qt_VDGetBlackLevelValue(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short blackLevel;
#ifndef VDGetBlackLevelValue
	PyMac_PRECHECK(VDGetBlackLevelValue);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetBlackLevelValue(ci,
	                           &blackLevel);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     blackLevel);
	return _res;
}

static PyObject *Qt_VDSetWhiteLevelValue(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short whiteLevel;
#ifndef VDSetWhiteLevelValue
	PyMac_PRECHECK(VDSetWhiteLevelValue);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDSetWhiteLevelValue(ci,
	                           &whiteLevel);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     whiteLevel);
	return _res;
}

static PyObject *Qt_VDGetWhiteLevelValue(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short whiteLevel;
#ifndef VDGetWhiteLevelValue
	PyMac_PRECHECK(VDGetWhiteLevelValue);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetWhiteLevelValue(ci,
	                           &whiteLevel);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     whiteLevel);
	return _res;
}

static PyObject *Qt_VDGetVideoDefaults(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	unsigned short blackLevel;
	unsigned short whiteLevel;
	unsigned short brightness;
	unsigned short hue;
	unsigned short saturation;
	unsigned short contrast;
	unsigned short sharpness;
#ifndef VDGetVideoDefaults
	PyMac_PRECHECK(VDGetVideoDefaults);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetVideoDefaults(ci,
	                         &blackLevel,
	                         &whiteLevel,
	                         &brightness,
	                         &hue,
	                         &saturation,
	                         &contrast,
	                         &sharpness);
	_res = Py_BuildValue("lHHHHHHH",
	                     _rv,
	                     blackLevel,
	                     whiteLevel,
	                     brightness,
	                     hue,
	                     saturation,
	                     contrast,
	                     sharpness);
	return _res;
}

static PyObject *Qt_VDGetNumberOfInputs(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short inputs;
#ifndef VDGetNumberOfInputs
	PyMac_PRECHECK(VDGetNumberOfInputs);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetNumberOfInputs(ci,
	                          &inputs);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     inputs);
	return _res;
}

static PyObject *Qt_VDGetInputFormat(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short input;
	short format;
#ifndef VDGetInputFormat
	PyMac_PRECHECK(VDGetInputFormat);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &input))
		return NULL;
	_rv = VDGetInputFormat(ci,
	                       input,
	                       &format);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     format);
	return _res;
}

static PyObject *Qt_VDSetInput(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short input;
#ifndef VDSetInput
	PyMac_PRECHECK(VDSetInput);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &input))
		return NULL;
	_rv = VDSetInput(ci,
	                 input);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetInput(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short input;
#ifndef VDGetInput
	PyMac_PRECHECK(VDGetInput);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetInput(ci,
	                 &input);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     input);
	return _res;
}

static PyObject *Qt_VDSetInputStandard(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short inputStandard;
#ifndef VDSetInputStandard
	PyMac_PRECHECK(VDSetInputStandard);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &inputStandard))
		return NULL;
	_rv = VDSetInputStandard(ci,
	                         inputStandard);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDSetupBuffers(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	VdigBufferRecListHandle bufferList;
#ifndef VDSetupBuffers
	PyMac_PRECHECK(VDSetupBuffers);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &bufferList))
		return NULL;
	_rv = VDSetupBuffers(ci,
	                     bufferList);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGrabOneFrameAsync(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short buffer;
#ifndef VDGrabOneFrameAsync
	PyMac_PRECHECK(VDGrabOneFrameAsync);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &buffer))
		return NULL;
	_rv = VDGrabOneFrameAsync(ci,
	                          buffer);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDDone(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	short buffer;
#ifndef VDDone
	PyMac_PRECHECK(VDDone);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &ci,
	                      &buffer))
		return NULL;
	_rv = VDDone(ci,
	             buffer);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDSetCompression(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	OSType compressType;
	short depth;
	Rect bounds;
	CodecQ spatialQuality;
	CodecQ temporalQuality;
	long keyFrameRate;
#ifndef VDSetCompression
	PyMac_PRECHECK(VDSetCompression);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&hlll",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetOSType, &compressType,
	                      &depth,
	                      &spatialQuality,
	                      &temporalQuality,
	                      &keyFrameRate))
		return NULL;
	_rv = VDSetCompression(ci,
	                       compressType,
	                       depth,
	                       &bounds,
	                       spatialQuality,
	                       temporalQuality,
	                       keyFrameRate);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &bounds);
	return _res;
}

static PyObject *Qt_VDCompressOneFrameAsync(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
#ifndef VDCompressOneFrameAsync
	PyMac_PRECHECK(VDCompressOneFrameAsync);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDCompressOneFrameAsync(ci);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetImageDescription(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	ImageDescriptionHandle desc;
#ifndef VDGetImageDescription
	PyMac_PRECHECK(VDGetImageDescription);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &desc))
		return NULL;
	_rv = VDGetImageDescription(ci,
	                            desc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDResetCompressSequence(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
#ifndef VDResetCompressSequence
	PyMac_PRECHECK(VDResetCompressSequence);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDResetCompressSequence(ci);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDSetCompressionOnOff(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	Boolean state;
#ifndef VDSetCompressionOnOff
	PyMac_PRECHECK(VDSetCompressionOnOff);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &ci,
	                      &state))
		return NULL;
	_rv = VDSetCompressionOnOff(ci,
	                            state);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetCompressionTypes(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	VDCompressionListHandle h;
#ifndef VDGetCompressionTypes
	PyMac_PRECHECK(VDGetCompressionTypes);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      ResObj_Convert, &h))
		return NULL;
	_rv = VDGetCompressionTypes(ci,
	                            h);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDSetTimeBase(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	TimeBase t;
#ifndef VDSetTimeBase
	PyMac_PRECHECK(VDSetTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      TimeBaseObj_Convert, &t))
		return NULL;
	_rv = VDSetTimeBase(ci,
	                    t);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDSetFrameRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	Fixed framesPerSecond;
#ifndef VDSetFrameRate
	PyMac_PRECHECK(VDSetFrameRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetFixed, &framesPerSecond))
		return NULL;
	_rv = VDSetFrameRate(ci,
	                     framesPerSecond);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetDataRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long milliSecPerFrame;
	Fixed framesPerSecond;
	long bytesPerSecond;
#ifndef VDGetDataRate
	PyMac_PRECHECK(VDGetDataRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetDataRate(ci,
	                    &milliSecPerFrame,
	                    &framesPerSecond,
	                    &bytesPerSecond);
	_res = Py_BuildValue("llO&l",
	                     _rv,
	                     milliSecPerFrame,
	                     PyMac_BuildFixed, framesPerSecond,
	                     bytesPerSecond);
	return _res;
}

static PyObject *Qt_VDGetSoundInputDriver(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	Str255 soundDriverName;
#ifndef VDGetSoundInputDriver
	PyMac_PRECHECK(VDGetSoundInputDriver);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetStr255, soundDriverName))
		return NULL;
	_rv = VDGetSoundInputDriver(ci,
	                            soundDriverName);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetDMADepths(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long depthArray;
	long preferredDepth;
#ifndef VDGetDMADepths
	PyMac_PRECHECK(VDGetDMADepths);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetDMADepths(ci,
	                     &depthArray,
	                     &preferredDepth);
	_res = Py_BuildValue("lll",
	                     _rv,
	                     depthArray,
	                     preferredDepth);
	return _res;
}

static PyObject *Qt_VDGetPreferredTimeScale(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	TimeScale preferred;
#ifndef VDGetPreferredTimeScale
	PyMac_PRECHECK(VDGetPreferredTimeScale);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetPreferredTimeScale(ci,
	                              &preferred);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     preferred);
	return _res;
}

static PyObject *Qt_VDReleaseAsyncBuffers(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
#ifndef VDReleaseAsyncBuffers
	PyMac_PRECHECK(VDReleaseAsyncBuffers);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDReleaseAsyncBuffers(ci);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDSetDataRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long bytesPerSecond;
#ifndef VDSetDataRate
	PyMac_PRECHECK(VDSetDataRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &bytesPerSecond))
		return NULL;
	_rv = VDSetDataRate(ci,
	                    bytesPerSecond);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetTimeCode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	TimeRecord atTime;
	void * timeCodeFormat;
	void * timeCodeTime;
#ifndef VDGetTimeCode
	PyMac_PRECHECK(VDGetTimeCode);
#endif
	if (!PyArg_ParseTuple(_args, "O&ss",
	                      CmpInstObj_Convert, &ci,
	                      &timeCodeFormat,
	                      &timeCodeTime))
		return NULL;
	_rv = VDGetTimeCode(ci,
	                    &atTime,
	                    timeCodeFormat,
	                    timeCodeTime);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     QtTimeRecord_New, &atTime);
	return _res;
}

static PyObject *Qt_VDUseSafeBuffers(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	Boolean useSafeBuffers;
#ifndef VDUseSafeBuffers
	PyMac_PRECHECK(VDUseSafeBuffers);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &ci,
	                      &useSafeBuffers))
		return NULL;
	_rv = VDUseSafeBuffers(ci,
	                       useSafeBuffers);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetSoundInputSource(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long videoInput;
	long soundInput;
#ifndef VDGetSoundInputSource
	PyMac_PRECHECK(VDGetSoundInputSource);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &videoInput))
		return NULL;
	_rv = VDGetSoundInputSource(ci,
	                            videoInput,
	                            &soundInput);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     soundInput);
	return _res;
}

static PyObject *Qt_VDGetCompressionTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	OSType compressionType;
	short depth;
	Rect srcRect;
	CodecQ spatialQuality;
	CodecQ temporalQuality;
	unsigned long compressTime;
#ifndef VDGetCompressionTime
	PyMac_PRECHECK(VDGetCompressionTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetOSType, &compressionType,
	                      &depth))
		return NULL;
	_rv = VDGetCompressionTime(ci,
	                           compressionType,
	                           depth,
	                           &srcRect,
	                           &spatialQuality,
	                           &temporalQuality,
	                           &compressTime);
	_res = Py_BuildValue("lO&lll",
	                     _rv,
	                     PyMac_BuildRect, &srcRect,
	                     spatialQuality,
	                     temporalQuality,
	                     compressTime);
	return _res;
}

static PyObject *Qt_VDSetPreferredPacketSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long preferredPacketSizeInBytes;
#ifndef VDSetPreferredPacketSize
	PyMac_PRECHECK(VDSetPreferredPacketSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &preferredPacketSizeInBytes))
		return NULL;
	_rv = VDSetPreferredPacketSize(ci,
	                               preferredPacketSizeInBytes);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDSetPreferredImageDimensions(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long width;
	long height;
#ifndef VDSetPreferredImageDimensions
	PyMac_PRECHECK(VDSetPreferredImageDimensions);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &ci,
	                      &width,
	                      &height))
		return NULL;
	_rv = VDSetPreferredImageDimensions(ci,
	                                    width,
	                                    height);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetPreferredImageDimensions(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long width;
	long height;
#ifndef VDGetPreferredImageDimensions
	PyMac_PRECHECK(VDGetPreferredImageDimensions);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = VDGetPreferredImageDimensions(ci,
	                                    &width,
	                                    &height);
	_res = Py_BuildValue("lll",
	                     _rv,
	                     width,
	                     height);
	return _res;
}

static PyObject *Qt_VDGetInputName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	long videoInput;
	Str255 name;
#ifndef VDGetInputName
	PyMac_PRECHECK(VDGetInputName);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      CmpInstObj_Convert, &ci,
	                      &videoInput,
	                      PyMac_GetStr255, name))
		return NULL;
	_rv = VDGetInputName(ci,
	                     videoInput,
	                     name);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDSetDestinationPort(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	CGrafPtr destPort;
#ifndef VDSetDestinationPort
	PyMac_PRECHECK(VDSetDestinationPort);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      GrafObj_Convert, &destPort))
		return NULL;
	_rv = VDSetDestinationPort(ci,
	                           destPort);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_VDGetDeviceNameAndFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	Str255 outName;
	UInt32 outNameFlags;
#ifndef VDGetDeviceNameAndFlags
	PyMac_PRECHECK(VDGetDeviceNameAndFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &ci,
	                      PyMac_GetStr255, outName))
		return NULL;
	_rv = VDGetDeviceNameAndFlags(ci,
	                              outName,
	                              &outNameFlags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     outNameFlags);
	return _res;
}

static PyObject *Qt_VDCaptureStateChanging(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	VideoDigitizerComponent ci;
	UInt32 inStateFlags;
#ifndef VDCaptureStateChanging
	PyMac_PRECHECK(VDCaptureStateChanging);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &ci,
	                      &inStateFlags))
		return NULL;
	_rv = VDCaptureStateChanging(ci,
	                             inStateFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_XMLParseGetDetailedParseError(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance aParser;
	long errorLine;
	StringPtr errDesc;
#ifndef XMLParseGetDetailedParseError
	PyMac_PRECHECK(XMLParseGetDetailedParseError);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &aParser,
	                      &errDesc))
		return NULL;
	_rv = XMLParseGetDetailedParseError(aParser,
	                                    &errorLine,
	                                    errDesc);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     errorLine);
	return _res;
}

static PyObject *Qt_XMLParseAddElement(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance aParser;
	char elementName;
	UInt32 nameSpaceID;
	UInt32 elementID;
	long elementFlags;
#ifndef XMLParseAddElement
	PyMac_PRECHECK(XMLParseAddElement);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &aParser,
	                      &nameSpaceID,
	                      &elementFlags))
		return NULL;
	_rv = XMLParseAddElement(aParser,
	                         &elementName,
	                         nameSpaceID,
	                         &elementID,
	                         elementFlags);
	_res = Py_BuildValue("lcl",
	                     _rv,
	                     elementName,
	                     elementID);
	return _res;
}

static PyObject *Qt_XMLParseAddAttribute(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance aParser;
	UInt32 elementID;
	UInt32 nameSpaceID;
	char attributeName;
	UInt32 attributeID;
#ifndef XMLParseAddAttribute
	PyMac_PRECHECK(XMLParseAddAttribute);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &aParser,
	                      &elementID,
	                      &nameSpaceID))
		return NULL;
	_rv = XMLParseAddAttribute(aParser,
	                           elementID,
	                           nameSpaceID,
	                           &attributeName,
	                           &attributeID);
	_res = Py_BuildValue("lcl",
	                     _rv,
	                     attributeName,
	                     attributeID);
	return _res;
}

static PyObject *Qt_XMLParseAddMultipleAttributes(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance aParser;
	UInt32 elementID;
	UInt32 nameSpaceIDs;
	char attributeNames;
	UInt32 attributeIDs;
#ifndef XMLParseAddMultipleAttributes
	PyMac_PRECHECK(XMLParseAddMultipleAttributes);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &aParser,
	                      &elementID))
		return NULL;
	_rv = XMLParseAddMultipleAttributes(aParser,
	                                    elementID,
	                                    &nameSpaceIDs,
	                                    &attributeNames,
	                                    &attributeIDs);
	_res = Py_BuildValue("llcl",
	                     _rv,
	                     nameSpaceIDs,
	                     attributeNames,
	                     attributeIDs);
	return _res;
}

static PyObject *Qt_XMLParseAddAttributeAndValue(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance aParser;
	UInt32 elementID;
	UInt32 nameSpaceID;
	char attributeName;
	UInt32 attributeID;
	UInt32 attributeValueKind;
	void * attributeValueKindInfo;
#ifndef XMLParseAddAttributeAndValue
	PyMac_PRECHECK(XMLParseAddAttributeAndValue);
#endif
	if (!PyArg_ParseTuple(_args, "O&llls",
	                      CmpInstObj_Convert, &aParser,
	                      &elementID,
	                      &nameSpaceID,
	                      &attributeValueKind,
	                      &attributeValueKindInfo))
		return NULL;
	_rv = XMLParseAddAttributeAndValue(aParser,
	                                   elementID,
	                                   nameSpaceID,
	                                   &attributeName,
	                                   &attributeID,
	                                   attributeValueKind,
	                                   attributeValueKindInfo);
	_res = Py_BuildValue("lcl",
	                     _rv,
	                     attributeName,
	                     attributeID);
	return _res;
}

static PyObject *Qt_XMLParseAddAttributeValueKind(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance aParser;
	UInt32 elementID;
	UInt32 attributeID;
	UInt32 attributeValueKind;
	void * attributeValueKindInfo;
#ifndef XMLParseAddAttributeValueKind
	PyMac_PRECHECK(XMLParseAddAttributeValueKind);
#endif
	if (!PyArg_ParseTuple(_args, "O&llls",
	                      CmpInstObj_Convert, &aParser,
	                      &elementID,
	                      &attributeID,
	                      &attributeValueKind,
	                      &attributeValueKindInfo))
		return NULL;
	_rv = XMLParseAddAttributeValueKind(aParser,
	                                    elementID,
	                                    attributeID,
	                                    attributeValueKind,
	                                    attributeValueKindInfo);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_XMLParseAddNameSpace(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance aParser;
	char nameSpaceURL;
	UInt32 nameSpaceID;
#ifndef XMLParseAddNameSpace
	PyMac_PRECHECK(XMLParseAddNameSpace);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &aParser))
		return NULL;
	_rv = XMLParseAddNameSpace(aParser,
	                           &nameSpaceURL,
	                           &nameSpaceID);
	_res = Py_BuildValue("lcl",
	                     _rv,
	                     nameSpaceURL,
	                     nameSpaceID);
	return _res;
}

static PyObject *Qt_XMLParseSetOffsetAndLimit(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance aParser;
	UInt32 offset;
	UInt32 limit;
#ifndef XMLParseSetOffsetAndLimit
	PyMac_PRECHECK(XMLParseSetOffsetAndLimit);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &aParser,
	                      &offset,
	                      &limit))
		return NULL;
	_rv = XMLParseSetOffsetAndLimit(aParser,
	                                offset,
	                                limit);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_XMLParseSetEventParseRefCon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	ComponentInstance aParser;
	long refcon;
#ifndef XMLParseSetEventParseRefCon
	PyMac_PRECHECK(XMLParseSetEventParseRefCon);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &aParser,
	                      &refcon))
		return NULL;
	_rv = XMLParseSetEventParseRefCon(aParser,
	                                  refcon);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGInitialize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
#ifndef SGInitialize
	PyMac_PRECHECK(SGInitialize);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGInitialize(s);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetDataOutput(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	FSSpec movieFile;
	long whereFlags;
#ifndef SGSetDataOutput
	PyMac_PRECHECK(SGSetDataOutput);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      CmpInstObj_Convert, &s,
	                      PyMac_GetFSSpec, &movieFile,
	                      &whereFlags))
		return NULL;
	_rv = SGSetDataOutput(s,
	                      &movieFile,
	                      whereFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetDataOutput(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	FSSpec movieFile;
	long whereFlags;
#ifndef SGGetDataOutput
	PyMac_PRECHECK(SGGetDataOutput);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      PyMac_GetFSSpec, &movieFile))
		return NULL;
	_rv = SGGetDataOutput(s,
	                      &movieFile,
	                      &whereFlags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     whereFlags);
	return _res;
}

static PyObject *Qt_SGSetGWorld(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	CGrafPtr gp;
	GDHandle gd;
#ifndef SGSetGWorld
	PyMac_PRECHECK(SGSetGWorld);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &s,
	                      GrafObj_Convert, &gp,
	                      OptResObj_Convert, &gd))
		return NULL;
	_rv = SGSetGWorld(s,
	                  gp,
	                  gd);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetGWorld(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	CGrafPtr gp;
	GDHandle gd;
#ifndef SGGetGWorld
	PyMac_PRECHECK(SGGetGWorld);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGGetGWorld(s,
	                  &gp,
	                  &gd);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     GrafObj_New, gp,
	                     OptResObj_New, gd);
	return _res;
}

static PyObject *Qt_SGNewChannel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	OSType channelType;
	SGChannel ref;
#ifndef SGNewChannel
	PyMac_PRECHECK(SGNewChannel);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      PyMac_GetOSType, &channelType))
		return NULL;
	_rv = SGNewChannel(s,
	                   channelType,
	                   &ref);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     CmpInstObj_New, ref);
	return _res;
}

static PyObject *Qt_SGDisposeChannel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
#ifndef SGDisposeChannel
	PyMac_PRECHECK(SGDisposeChannel);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGDisposeChannel(s,
	                       c);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGStartPreview(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
#ifndef SGStartPreview
	PyMac_PRECHECK(SGStartPreview);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGStartPreview(s);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGStartRecord(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
#ifndef SGStartRecord
	PyMac_PRECHECK(SGStartRecord);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGStartRecord(s);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGIdle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
#ifndef SGIdle
	PyMac_PRECHECK(SGIdle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGIdle(s);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGStop(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
#ifndef SGStop
	PyMac_PRECHECK(SGStop);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGStop(s);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGPause(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	Boolean pause;
#ifndef SGPause
	PyMac_PRECHECK(SGPause);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &s,
	                      &pause))
		return NULL;
	_rv = SGPause(s,
	              pause);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGPrepare(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	Boolean prepareForPreview;
	Boolean prepareForRecord;
#ifndef SGPrepare
	PyMac_PRECHECK(SGPrepare);
#endif
	if (!PyArg_ParseTuple(_args, "O&bb",
	                      CmpInstObj_Convert, &s,
	                      &prepareForPreview,
	                      &prepareForRecord))
		return NULL;
	_rv = SGPrepare(s,
	                prepareForPreview,
	                prepareForRecord);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGRelease(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
#ifndef SGRelease
	PyMac_PRECHECK(SGRelease);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGRelease(s);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetMovie(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Movie _rv;
	SeqGrabComponent s;
#ifndef SGGetMovie
	PyMac_PRECHECK(SGGetMovie);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGGetMovie(s);
	_res = Py_BuildValue("O&",
	                     MovieObj_New, _rv);
	return _res;
}

static PyObject *Qt_SGSetMaximumRecordTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	unsigned long ticks;
#ifndef SGSetMaximumRecordTime
	PyMac_PRECHECK(SGSetMaximumRecordTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &s,
	                      &ticks))
		return NULL;
	_rv = SGSetMaximumRecordTime(s,
	                             ticks);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetMaximumRecordTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	unsigned long ticks;
#ifndef SGGetMaximumRecordTime
	PyMac_PRECHECK(SGGetMaximumRecordTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGGetMaximumRecordTime(s,
	                             &ticks);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     ticks);
	return _res;
}

static PyObject *Qt_SGGetStorageSpaceRemaining(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	unsigned long bytes;
#ifndef SGGetStorageSpaceRemaining
	PyMac_PRECHECK(SGGetStorageSpaceRemaining);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGGetStorageSpaceRemaining(s,
	                                 &bytes);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     bytes);
	return _res;
}

static PyObject *Qt_SGGetTimeRemaining(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	long ticksLeft;
#ifndef SGGetTimeRemaining
	PyMac_PRECHECK(SGGetTimeRemaining);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGGetTimeRemaining(s,
	                         &ticksLeft);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     ticksLeft);
	return _res;
}

static PyObject *Qt_SGGrabPict(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	PicHandle p;
	Rect bounds;
	short offscreenDepth;
	long grabPictFlags;
#ifndef SGGrabPict
	PyMac_PRECHECK(SGGrabPict);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&hl",
	                      CmpInstObj_Convert, &s,
	                      PyMac_GetRect, &bounds,
	                      &offscreenDepth,
	                      &grabPictFlags))
		return NULL;
	_rv = SGGrabPict(s,
	                 &p,
	                 &bounds,
	                 offscreenDepth,
	                 grabPictFlags);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, p);
	return _res;
}

static PyObject *Qt_SGGetLastMovieResID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	short resID;
#ifndef SGGetLastMovieResID
	PyMac_PRECHECK(SGGetLastMovieResID);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGGetLastMovieResID(s,
	                          &resID);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     resID);
	return _res;
}

static PyObject *Qt_SGSetFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	long sgFlags;
#ifndef SGSetFlags
	PyMac_PRECHECK(SGSetFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &s,
	                      &sgFlags))
		return NULL;
	_rv = SGSetFlags(s,
	                 sgFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	long sgFlags;
#ifndef SGGetFlags
	PyMac_PRECHECK(SGGetFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGGetFlags(s,
	                 &sgFlags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     sgFlags);
	return _res;
}

static PyObject *Qt_SGNewChannelFromComponent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel newChannel;
	Component sgChannelComponent;
#ifndef SGNewChannelFromComponent
	PyMac_PRECHECK(SGNewChannelFromComponent);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      CmpObj_Convert, &sgChannelComponent))
		return NULL;
	_rv = SGNewChannelFromComponent(s,
	                                &newChannel,
	                                sgChannelComponent);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     CmpInstObj_New, newChannel);
	return _res;
}

static PyObject *Qt_SGSetSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	UserData ud;
	long flags;
#ifndef SGSetSettings
	PyMac_PRECHECK(SGSetSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      CmpInstObj_Convert, &s,
	                      UserDataObj_Convert, &ud,
	                      &flags))
		return NULL;
	_rv = SGSetSettings(s,
	                    ud,
	                    flags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	UserData ud;
	long flags;
#ifndef SGGetSettings
	PyMac_PRECHECK(SGGetSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &s,
	                      &flags))
		return NULL;
	_rv = SGGetSettings(s,
	                    &ud,
	                    flags);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     UserDataObj_New, ud);
	return _res;
}

static PyObject *Qt_SGGetIndChannel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	short index;
	SGChannel ref;
	OSType chanType;
#ifndef SGGetIndChannel
	PyMac_PRECHECK(SGGetIndChannel);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &s,
	                      &index))
		return NULL;
	_rv = SGGetIndChannel(s,
	                      index,
	                      &ref,
	                      &chanType);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     CmpInstObj_New, ref,
	                     PyMac_BuildOSType, chanType);
	return _res;
}

static PyObject *Qt_SGUpdate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	RgnHandle updateRgn;
#ifndef SGUpdate
	PyMac_PRECHECK(SGUpdate);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      ResObj_Convert, &updateRgn))
		return NULL;
	_rv = SGUpdate(s,
	               updateRgn);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetPause(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	Boolean paused;
#ifndef SGGetPause
	PyMac_PRECHECK(SGGetPause);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGGetPause(s,
	                 &paused);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     paused);
	return _res;
}

static PyObject *Qt_SGSetChannelSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	UserData ud;
	long flags;
#ifndef SGSetChannelSettings
	PyMac_PRECHECK(SGSetChannelSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&l",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      UserDataObj_Convert, &ud,
	                      &flags))
		return NULL;
	_rv = SGSetChannelSettings(s,
	                           c,
	                           ud,
	                           flags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetChannelSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	UserData ud;
	long flags;
#ifndef SGGetChannelSettings
	PyMac_PRECHECK(SGGetChannelSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      &flags))
		return NULL;
	_rv = SGGetChannelSettings(s,
	                           c,
	                           &ud,
	                           flags);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     UserDataObj_New, ud);
	return _res;
}

static PyObject *Qt_SGGetMode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	Boolean previewMode;
	Boolean recordMode;
#ifndef SGGetMode
	PyMac_PRECHECK(SGGetMode);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGGetMode(s,
	                &previewMode,
	                &recordMode);
	_res = Py_BuildValue("lbb",
	                     _rv,
	                     previewMode,
	                     recordMode);
	return _res;
}

static PyObject *Qt_SGSetDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	Handle dataRef;
	OSType dataRefType;
	long whereFlags;
#ifndef SGSetDataRef
	PyMac_PRECHECK(SGSetDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&l",
	                      CmpInstObj_Convert, &s,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      &whereFlags))
		return NULL;
	_rv = SGSetDataRef(s,
	                   dataRef,
	                   dataRefType,
	                   whereFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetDataRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	Handle dataRef;
	OSType dataRefType;
	long whereFlags;
#ifndef SGGetDataRef
	PyMac_PRECHECK(SGGetDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGGetDataRef(s,
	                   &dataRef,
	                   &dataRefType,
	                   &whereFlags);
	_res = Py_BuildValue("lO&O&l",
	                     _rv,
	                     ResObj_New, dataRef,
	                     PyMac_BuildOSType, dataRefType,
	                     whereFlags);
	return _res;
}

static PyObject *Qt_SGNewOutput(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	Handle dataRef;
	OSType dataRefType;
	long whereFlags;
	SGOutput sgOut;
#ifndef SGNewOutput
	PyMac_PRECHECK(SGNewOutput);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&l",
	                      CmpInstObj_Convert, &s,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType,
	                      &whereFlags))
		return NULL;
	_rv = SGNewOutput(s,
	                  dataRef,
	                  dataRefType,
	                  whereFlags,
	                  &sgOut);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     SGOutputObj_New, sgOut);
	return _res;
}

static PyObject *Qt_SGDisposeOutput(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGOutput sgOut;
#ifndef SGDisposeOutput
	PyMac_PRECHECK(SGDisposeOutput);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      SGOutputObj_Convert, &sgOut))
		return NULL;
	_rv = SGDisposeOutput(s,
	                      sgOut);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetOutputFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGOutput sgOut;
	long whereFlags;
#ifndef SGSetOutputFlags
	PyMac_PRECHECK(SGSetOutputFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      CmpInstObj_Convert, &s,
	                      SGOutputObj_Convert, &sgOut,
	                      &whereFlags))
		return NULL;
	_rv = SGSetOutputFlags(s,
	                       sgOut,
	                       whereFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetChannelOutput(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	SGOutput sgOut;
#ifndef SGSetChannelOutput
	PyMac_PRECHECK(SGSetChannelOutput);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      SGOutputObj_Convert, &sgOut))
		return NULL;
	_rv = SGSetChannelOutput(s,
	                         c,
	                         sgOut);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetDataOutputStorageSpaceRemaining(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGOutput sgOut;
	unsigned long space;
#ifndef SGGetDataOutputStorageSpaceRemaining
	PyMac_PRECHECK(SGGetDataOutputStorageSpaceRemaining);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      SGOutputObj_Convert, &sgOut))
		return NULL;
	_rv = SGGetDataOutputStorageSpaceRemaining(s,
	                                           sgOut,
	                                           &space);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     space);
	return _res;
}

static PyObject *Qt_SGHandleUpdateEvent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	EventRecord event;
	Boolean handled;
#ifndef SGHandleUpdateEvent
	PyMac_PRECHECK(SGHandleUpdateEvent);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      PyMac_GetEventRecord, &event))
		return NULL;
	_rv = SGHandleUpdateEvent(s,
	                          &event,
	                          &handled);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     handled);
	return _res;
}

static PyObject *Qt_SGSetOutputNextOutput(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGOutput sgOut;
	SGOutput nextOut;
#ifndef SGSetOutputNextOutput
	PyMac_PRECHECK(SGSetOutputNextOutput);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &s,
	                      SGOutputObj_Convert, &sgOut,
	                      SGOutputObj_Convert, &nextOut))
		return NULL;
	_rv = SGSetOutputNextOutput(s,
	                            sgOut,
	                            nextOut);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetOutputNextOutput(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGOutput sgOut;
	SGOutput nextOut;
#ifndef SGGetOutputNextOutput
	PyMac_PRECHECK(SGGetOutputNextOutput);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      SGOutputObj_Convert, &sgOut))
		return NULL;
	_rv = SGGetOutputNextOutput(s,
	                            sgOut,
	                            &nextOut);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     SGOutputObj_New, nextOut);
	return _res;
}

static PyObject *Qt_SGSetOutputMaximumOffset(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGOutput sgOut;
	wide maxOffset;
#ifndef SGSetOutputMaximumOffset
	PyMac_PRECHECK(SGSetOutputMaximumOffset);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &s,
	                      SGOutputObj_Convert, &sgOut,
	                      PyMac_Getwide, &maxOffset))
		return NULL;
	_rv = SGSetOutputMaximumOffset(s,
	                               sgOut,
	                               &maxOffset);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetOutputMaximumOffset(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGOutput sgOut;
	wide maxOffset;
#ifndef SGGetOutputMaximumOffset
	PyMac_PRECHECK(SGGetOutputMaximumOffset);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      SGOutputObj_Convert, &sgOut))
		return NULL;
	_rv = SGGetOutputMaximumOffset(s,
	                               sgOut,
	                               &maxOffset);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_Buildwide, maxOffset);
	return _res;
}

static PyObject *Qt_SGGetOutputDataReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGOutput sgOut;
	Handle dataRef;
	OSType dataRefType;
#ifndef SGGetOutputDataReference
	PyMac_PRECHECK(SGGetOutputDataReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      SGOutputObj_Convert, &sgOut))
		return NULL;
	_rv = SGGetOutputDataReference(s,
	                               sgOut,
	                               &dataRef,
	                               &dataRefType);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, dataRef,
	                     PyMac_BuildOSType, dataRefType);
	return _res;
}

static PyObject *Qt_SGWriteExtendedMovieData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	Ptr p;
	long len;
	wide offset;
	SGOutput sgOut;
#ifndef SGWriteExtendedMovieData
	PyMac_PRECHECK(SGWriteExtendedMovieData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&sl",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      &p,
	                      &len))
		return NULL;
	_rv = SGWriteExtendedMovieData(s,
	                               c,
	                               p,
	                               len,
	                               &offset,
	                               &sgOut);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     PyMac_Buildwide, offset,
	                     SGOutputObj_New, sgOut);
	return _res;
}

static PyObject *Qt_SGGetStorageSpaceRemaining64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	wide bytes;
#ifndef SGGetStorageSpaceRemaining64
	PyMac_PRECHECK(SGGetStorageSpaceRemaining64);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGGetStorageSpaceRemaining64(s,
	                                   &bytes);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_Buildwide, bytes);
	return _res;
}

static PyObject *Qt_SGGetDataOutputStorageSpaceRemaining64(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGOutput sgOut;
	wide space;
#ifndef SGGetDataOutputStorageSpaceRemaining64
	PyMac_PRECHECK(SGGetDataOutputStorageSpaceRemaining64);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      SGOutputObj_Convert, &sgOut))
		return NULL;
	_rv = SGGetDataOutputStorageSpaceRemaining64(s,
	                                             sgOut,
	                                             &space);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_Buildwide, space);
	return _res;
}

static PyObject *Qt_SGWriteMovieData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	Ptr p;
	long len;
	long offset;
#ifndef SGWriteMovieData
	PyMac_PRECHECK(SGWriteMovieData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&sl",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      &p,
	                      &len))
		return NULL;
	_rv = SGWriteMovieData(s,
	                       c,
	                       p,
	                       len,
	                       &offset);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     offset);
	return _res;
}

static PyObject *Qt_SGGetTimeBase(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	TimeBase tb;
#ifndef SGGetTimeBase
	PyMac_PRECHECK(SGGetTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGGetTimeBase(s,
	                    &tb);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     TimeBaseObj_New, tb);
	return _res;
}

static PyObject *Qt_SGAddMovieData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	Ptr p;
	long len;
	long offset;
	long chRefCon;
	TimeValue time;
	short writeType;
#ifndef SGAddMovieData
	PyMac_PRECHECK(SGAddMovieData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&slllh",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      &p,
	                      &len,
	                      &chRefCon,
	                      &time,
	                      &writeType))
		return NULL;
	_rv = SGAddMovieData(s,
	                     c,
	                     p,
	                     len,
	                     &offset,
	                     chRefCon,
	                     time,
	                     writeType);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     offset);
	return _res;
}

static PyObject *Qt_SGChangedSource(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
#ifndef SGChangedSource
	PyMac_PRECHECK(SGChangedSource);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGChangedSource(s,
	                      c);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGAddExtendedMovieData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	Ptr p;
	long len;
	wide offset;
	long chRefCon;
	TimeValue time;
	short writeType;
	SGOutput whichOutput;
#ifndef SGAddExtendedMovieData
	PyMac_PRECHECK(SGAddExtendedMovieData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&slllh",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      &p,
	                      &len,
	                      &chRefCon,
	                      &time,
	                      &writeType))
		return NULL;
	_rv = SGAddExtendedMovieData(s,
	                             c,
	                             p,
	                             len,
	                             &offset,
	                             chRefCon,
	                             time,
	                             writeType,
	                             &whichOutput);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     PyMac_Buildwide, offset,
	                     SGOutputObj_New, whichOutput);
	return _res;
}

static PyObject *Qt_SGAddOutputDataRefToMedia(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGOutput sgOut;
	Media theMedia;
	SampleDescriptionHandle desc;
#ifndef SGAddOutputDataRefToMedia
	PyMac_PRECHECK(SGAddOutputDataRefToMedia);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&",
	                      CmpInstObj_Convert, &s,
	                      SGOutputObj_Convert, &sgOut,
	                      MediaObj_Convert, &theMedia,
	                      ResObj_Convert, &desc))
		return NULL;
	_rv = SGAddOutputDataRefToMedia(s,
	                                sgOut,
	                                theMedia,
	                                desc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetSettingsSummary(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	Handle summaryText;
#ifndef SGSetSettingsSummary
	PyMac_PRECHECK(SGSetSettingsSummary);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      ResObj_Convert, &summaryText))
		return NULL;
	_rv = SGSetSettingsSummary(s,
	                           summaryText);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetChannelUsage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long usage;
#ifndef SGSetChannelUsage
	PyMac_PRECHECK(SGSetChannelUsage);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &c,
	                      &usage))
		return NULL;
	_rv = SGSetChannelUsage(c,
	                        usage);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetChannelUsage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long usage;
#ifndef SGGetChannelUsage
	PyMac_PRECHECK(SGGetChannelUsage);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetChannelUsage(c,
	                        &usage);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     usage);
	return _res;
}

static PyObject *Qt_SGSetChannelBounds(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Rect bounds;
#ifndef SGSetChannelBounds
	PyMac_PRECHECK(SGSetChannelBounds);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      PyMac_GetRect, &bounds))
		return NULL;
	_rv = SGSetChannelBounds(c,
	                         &bounds);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetChannelBounds(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Rect bounds;
#ifndef SGGetChannelBounds
	PyMac_PRECHECK(SGGetChannelBounds);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetChannelBounds(c,
	                         &bounds);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &bounds);
	return _res;
}

static PyObject *Qt_SGSetChannelVolume(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short volume;
#ifndef SGSetChannelVolume
	PyMac_PRECHECK(SGSetChannelVolume);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &c,
	                      &volume))
		return NULL;
	_rv = SGSetChannelVolume(c,
	                         volume);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetChannelVolume(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short volume;
#ifndef SGGetChannelVolume
	PyMac_PRECHECK(SGGetChannelVolume);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetChannelVolume(c,
	                         &volume);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     volume);
	return _res;
}

static PyObject *Qt_SGGetChannelInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long channelInfo;
#ifndef SGGetChannelInfo
	PyMac_PRECHECK(SGGetChannelInfo);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetChannelInfo(c,
	                       &channelInfo);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     channelInfo);
	return _res;
}

static PyObject *Qt_SGSetChannelPlayFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long playFlags;
#ifndef SGSetChannelPlayFlags
	PyMac_PRECHECK(SGSetChannelPlayFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &c,
	                      &playFlags))
		return NULL;
	_rv = SGSetChannelPlayFlags(c,
	                            playFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetChannelPlayFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long playFlags;
#ifndef SGGetChannelPlayFlags
	PyMac_PRECHECK(SGGetChannelPlayFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetChannelPlayFlags(c,
	                            &playFlags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     playFlags);
	return _res;
}

static PyObject *Qt_SGSetChannelMaxFrames(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long frameCount;
#ifndef SGSetChannelMaxFrames
	PyMac_PRECHECK(SGSetChannelMaxFrames);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &c,
	                      &frameCount))
		return NULL;
	_rv = SGSetChannelMaxFrames(c,
	                            frameCount);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetChannelMaxFrames(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long frameCount;
#ifndef SGGetChannelMaxFrames
	PyMac_PRECHECK(SGGetChannelMaxFrames);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetChannelMaxFrames(c,
	                            &frameCount);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     frameCount);
	return _res;
}

static PyObject *Qt_SGSetChannelRefCon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long refCon;
#ifndef SGSetChannelRefCon
	PyMac_PRECHECK(SGSetChannelRefCon);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &c,
	                      &refCon))
		return NULL;
	_rv = SGSetChannelRefCon(c,
	                         refCon);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetChannelClip(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	RgnHandle theClip;
#ifndef SGSetChannelClip
	PyMac_PRECHECK(SGSetChannelClip);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      ResObj_Convert, &theClip))
		return NULL;
	_rv = SGSetChannelClip(c,
	                       theClip);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetChannelClip(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	RgnHandle theClip;
#ifndef SGGetChannelClip
	PyMac_PRECHECK(SGGetChannelClip);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetChannelClip(c,
	                       &theClip);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, theClip);
	return _res;
}

static PyObject *Qt_SGGetChannelSampleDescription(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Handle sampleDesc;
#ifndef SGGetChannelSampleDescription
	PyMac_PRECHECK(SGGetChannelSampleDescription);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      ResObj_Convert, &sampleDesc))
		return NULL;
	_rv = SGGetChannelSampleDescription(c,
	                                    sampleDesc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetChannelDevice(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	StringPtr name;
#ifndef SGSetChannelDevice
	PyMac_PRECHECK(SGSetChannelDevice);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &c,
	                      &name))
		return NULL;
	_rv = SGSetChannelDevice(c,
	                         name);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetChannelTimeScale(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	TimeScale scale;
#ifndef SGGetChannelTimeScale
	PyMac_PRECHECK(SGGetChannelTimeScale);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetChannelTimeScale(c,
	                            &scale);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     scale);
	return _res;
}

static PyObject *Qt_SGChannelPutPicture(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
#ifndef SGChannelPutPicture
	PyMac_PRECHECK(SGChannelPutPicture);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGChannelPutPicture(c);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGChannelSetRequestedDataRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long bytesPerSecond;
#ifndef SGChannelSetRequestedDataRate
	PyMac_PRECHECK(SGChannelSetRequestedDataRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &c,
	                      &bytesPerSecond))
		return NULL;
	_rv = SGChannelSetRequestedDataRate(c,
	                                    bytesPerSecond);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGChannelGetRequestedDataRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long bytesPerSecond;
#ifndef SGChannelGetRequestedDataRate
	PyMac_PRECHECK(SGChannelGetRequestedDataRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGChannelGetRequestedDataRate(c,
	                                    &bytesPerSecond);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     bytesPerSecond);
	return _res;
}

static PyObject *Qt_SGChannelSetDataSourceName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Str255 name;
	ScriptCode scriptTag;
#ifndef SGChannelSetDataSourceName
	PyMac_PRECHECK(SGChannelSetDataSourceName);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      CmpInstObj_Convert, &c,
	                      PyMac_GetStr255, name,
	                      &scriptTag))
		return NULL;
	_rv = SGChannelSetDataSourceName(c,
	                                 name,
	                                 scriptTag);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGChannelGetDataSourceName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Str255 name;
	ScriptCode scriptTag;
#ifndef SGChannelGetDataSourceName
	PyMac_PRECHECK(SGChannelGetDataSourceName);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      PyMac_GetStr255, name))
		return NULL;
	_rv = SGChannelGetDataSourceName(c,
	                                 name,
	                                 &scriptTag);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     scriptTag);
	return _res;
}

static PyObject *Qt_SGChannelSetCodecSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Handle settings;
#ifndef SGChannelSetCodecSettings
	PyMac_PRECHECK(SGChannelSetCodecSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      ResObj_Convert, &settings))
		return NULL;
	_rv = SGChannelSetCodecSettings(c,
	                                settings);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGChannelGetCodecSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Handle settings;
#ifndef SGChannelGetCodecSettings
	PyMac_PRECHECK(SGChannelGetCodecSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGChannelGetCodecSettings(c,
	                                &settings);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, settings);
	return _res;
}

static PyObject *Qt_SGGetChannelTimeBase(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	TimeBase tb;
#ifndef SGGetChannelTimeBase
	PyMac_PRECHECK(SGGetChannelTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetChannelTimeBase(c,
	                           &tb);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     TimeBaseObj_New, tb);
	return _res;
}

static PyObject *Qt_SGGetChannelRefCon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long refCon;
#ifndef SGGetChannelRefCon
	PyMac_PRECHECK(SGGetChannelRefCon);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetChannelRefCon(c,
	                         &refCon);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     refCon);
	return _res;
}

static PyObject *Qt_SGGetChannelDeviceAndInputNames(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Str255 outDeviceName;
	Str255 outInputName;
	short outInputNumber;
#ifndef SGGetChannelDeviceAndInputNames
	PyMac_PRECHECK(SGGetChannelDeviceAndInputNames);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &c,
	                      PyMac_GetStr255, outDeviceName,
	                      PyMac_GetStr255, outInputName))
		return NULL;
	_rv = SGGetChannelDeviceAndInputNames(c,
	                                      outDeviceName,
	                                      outInputName,
	                                      &outInputNumber);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     outInputNumber);
	return _res;
}

static PyObject *Qt_SGSetChannelDeviceInput(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short inInputNumber;
#ifndef SGSetChannelDeviceInput
	PyMac_PRECHECK(SGSetChannelDeviceInput);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &c,
	                      &inInputNumber))
		return NULL;
	_rv = SGSetChannelDeviceInput(c,
	                              inInputNumber);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetChannelSettingsStateChanging(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	UInt32 inFlags;
#ifndef SGSetChannelSettingsStateChanging
	PyMac_PRECHECK(SGSetChannelSettingsStateChanging);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &c,
	                      &inFlags))
		return NULL;
	_rv = SGSetChannelSettingsStateChanging(c,
	                                        inFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGInitChannel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	SeqGrabComponent owner;
#ifndef SGInitChannel
	PyMac_PRECHECK(SGInitChannel);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      CmpInstObj_Convert, &owner))
		return NULL;
	_rv = SGInitChannel(c,
	                    owner);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGWriteSamples(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Movie m;
	AliasHandle theFile;
#ifndef SGWriteSamples
	PyMac_PRECHECK(SGWriteSamples);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &c,
	                      MovieObj_Convert, &m,
	                      ResObj_Convert, &theFile))
		return NULL;
	_rv = SGWriteSamples(c,
	                     m,
	                     theFile);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetDataRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long bytesPerSecond;
#ifndef SGGetDataRate
	PyMac_PRECHECK(SGGetDataRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetDataRate(c,
	                    &bytesPerSecond);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     bytesPerSecond);
	return _res;
}

static PyObject *Qt_SGAlignChannelRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Rect r;
#ifndef SGAlignChannelRect
	PyMac_PRECHECK(SGAlignChannelRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGAlignChannelRect(c,
	                         &r);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &r);
	return _res;
}

static PyObject *Qt_SGPanelGetDitl(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	Handle ditl;
#ifndef SGPanelGetDitl
	PyMac_PRECHECK(SGPanelGetDitl);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGPanelGetDitl(s,
	                     &ditl);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, ditl);
	return _res;
}

static PyObject *Qt_SGPanelGetTitle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	Str255 title;
#ifndef SGPanelGetTitle
	PyMac_PRECHECK(SGPanelGetTitle);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      PyMac_GetStr255, title))
		return NULL;
	_rv = SGPanelGetTitle(s,
	                      title);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGPanelCanRun(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
#ifndef SGPanelCanRun
	PyMac_PRECHECK(SGPanelCanRun);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGPanelCanRun(s,
	                    c);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGPanelInstall(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	DialogPtr d;
	short itemOffset;
#ifndef SGPanelInstall
	PyMac_PRECHECK(SGPanelInstall);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&h",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      DlgObj_Convert, &d,
	                      &itemOffset))
		return NULL;
	_rv = SGPanelInstall(s,
	                     c,
	                     d,
	                     itemOffset);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGPanelEvent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	DialogPtr d;
	short itemOffset;
	EventRecord theEvent;
	short itemHit;
	Boolean handled;
#ifndef SGPanelEvent
	PyMac_PRECHECK(SGPanelEvent);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&hO&",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      DlgObj_Convert, &d,
	                      &itemOffset,
	                      PyMac_GetEventRecord, &theEvent))
		return NULL;
	_rv = SGPanelEvent(s,
	                   c,
	                   d,
	                   itemOffset,
	                   &theEvent,
	                   &itemHit,
	                   &handled);
	_res = Py_BuildValue("lhb",
	                     _rv,
	                     itemHit,
	                     handled);
	return _res;
}

static PyObject *Qt_SGPanelItem(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	DialogPtr d;
	short itemOffset;
	short itemNum;
#ifndef SGPanelItem
	PyMac_PRECHECK(SGPanelItem);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&hh",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      DlgObj_Convert, &d,
	                      &itemOffset,
	                      &itemNum))
		return NULL;
	_rv = SGPanelItem(s,
	                  c,
	                  d,
	                  itemOffset,
	                  itemNum);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGPanelRemove(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	DialogPtr d;
	short itemOffset;
#ifndef SGPanelRemove
	PyMac_PRECHECK(SGPanelRemove);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&h",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      DlgObj_Convert, &d,
	                      &itemOffset))
		return NULL;
	_rv = SGPanelRemove(s,
	                    c,
	                    d,
	                    itemOffset);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGPanelSetGrabber(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SeqGrabComponent sg;
#ifndef SGPanelSetGrabber
	PyMac_PRECHECK(SGPanelSetGrabber);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &sg))
		return NULL;
	_rv = SGPanelSetGrabber(s,
	                        sg);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGPanelSetResFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	short resRef;
#ifndef SGPanelSetResFile
	PyMac_PRECHECK(SGPanelSetResFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &s,
	                      &resRef))
		return NULL;
	_rv = SGPanelSetResFile(s,
	                        resRef);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGPanelGetSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	UserData ud;
	long flags;
#ifndef SGPanelGetSettings
	PyMac_PRECHECK(SGPanelGetSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      &flags))
		return NULL;
	_rv = SGPanelGetSettings(s,
	                         c,
	                         &ud,
	                         flags);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     UserDataObj_New, ud);
	return _res;
}

static PyObject *Qt_SGPanelSetSettings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	SGChannel c;
	UserData ud;
	long flags;
#ifndef SGPanelSetSettings
	PyMac_PRECHECK(SGPanelSetSettings);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&l",
	                      CmpInstObj_Convert, &s,
	                      CmpInstObj_Convert, &c,
	                      UserDataObj_Convert, &ud,
	                      &flags))
		return NULL;
	_rv = SGPanelSetSettings(s,
	                         c,
	                         ud,
	                         flags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGPanelValidateInput(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	Boolean ok;
#ifndef SGPanelValidateInput
	PyMac_PRECHECK(SGPanelValidateInput);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGPanelValidateInput(s,
	                           &ok);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     ok);
	return _res;
}

static PyObject *Qt_SGPanelGetDITLForSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SeqGrabComponent s;
	Handle ditl;
	Point requestedSize;
#ifndef SGPanelGetDITLForSize
	PyMac_PRECHECK(SGPanelGetDITLForSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &s))
		return NULL;
	_rv = SGPanelGetDITLForSize(s,
	                            &ditl,
	                            &requestedSize);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, ditl,
	                     PyMac_BuildPoint, requestedSize);
	return _res;
}

static PyObject *Qt_SGGetSrcVideoBounds(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Rect r;
#ifndef SGGetSrcVideoBounds
	PyMac_PRECHECK(SGGetSrcVideoBounds);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetSrcVideoBounds(c,
	                          &r);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &r);
	return _res;
}

static PyObject *Qt_SGSetVideoRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Rect r;
#ifndef SGSetVideoRect
	PyMac_PRECHECK(SGSetVideoRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      PyMac_GetRect, &r))
		return NULL;
	_rv = SGSetVideoRect(c,
	                     &r);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetVideoRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Rect r;
#ifndef SGGetVideoRect
	PyMac_PRECHECK(SGGetVideoRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetVideoRect(c,
	                     &r);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &r);
	return _res;
}

static PyObject *Qt_SGGetVideoCompressorType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	OSType compressorType;
#ifndef SGGetVideoCompressorType
	PyMac_PRECHECK(SGGetVideoCompressorType);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetVideoCompressorType(c,
	                               &compressorType);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildOSType, compressorType);
	return _res;
}

static PyObject *Qt_SGSetVideoCompressorType(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	OSType compressorType;
#ifndef SGSetVideoCompressorType
	PyMac_PRECHECK(SGSetVideoCompressorType);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      PyMac_GetOSType, &compressorType))
		return NULL;
	_rv = SGSetVideoCompressorType(c,
	                               compressorType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetVideoCompressor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short depth;
	CompressorComponent compressor;
	CodecQ spatialQuality;
	CodecQ temporalQuality;
	long keyFrameRate;
#ifndef SGSetVideoCompressor
	PyMac_PRECHECK(SGSetVideoCompressor);
#endif
	if (!PyArg_ParseTuple(_args, "O&hO&lll",
	                      CmpInstObj_Convert, &c,
	                      &depth,
	                      CmpObj_Convert, &compressor,
	                      &spatialQuality,
	                      &temporalQuality,
	                      &keyFrameRate))
		return NULL;
	_rv = SGSetVideoCompressor(c,
	                           depth,
	                           compressor,
	                           spatialQuality,
	                           temporalQuality,
	                           keyFrameRate);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetVideoCompressor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short depth;
	CompressorComponent compressor;
	CodecQ spatialQuality;
	CodecQ temporalQuality;
	long keyFrameRate;
#ifndef SGGetVideoCompressor
	PyMac_PRECHECK(SGGetVideoCompressor);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetVideoCompressor(c,
	                           &depth,
	                           &compressor,
	                           &spatialQuality,
	                           &temporalQuality,
	                           &keyFrameRate);
	_res = Py_BuildValue("lhO&lll",
	                     _rv,
	                     depth,
	                     CmpObj_New, compressor,
	                     spatialQuality,
	                     temporalQuality,
	                     keyFrameRate);
	return _res;
}

static PyObject *Qt_SGGetVideoDigitizerComponent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentInstance _rv;
	SGChannel c;
#ifndef SGGetVideoDigitizerComponent
	PyMac_PRECHECK(SGGetVideoDigitizerComponent);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetVideoDigitizerComponent(c);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, _rv);
	return _res;
}

static PyObject *Qt_SGSetVideoDigitizerComponent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	ComponentInstance vdig;
#ifndef SGSetVideoDigitizerComponent
	PyMac_PRECHECK(SGSetVideoDigitizerComponent);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      CmpInstObj_Convert, &vdig))
		return NULL;
	_rv = SGSetVideoDigitizerComponent(c,
	                                   vdig);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGVideoDigitizerChanged(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
#ifndef SGVideoDigitizerChanged
	PyMac_PRECHECK(SGVideoDigitizerChanged);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGVideoDigitizerChanged(c);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGrabFrame(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short bufferNum;
#ifndef SGGrabFrame
	PyMac_PRECHECK(SGGrabFrame);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &c,
	                      &bufferNum))
		return NULL;
	_rv = SGGrabFrame(c,
	                  bufferNum);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGrabFrameComplete(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short bufferNum;
	Boolean done;
#ifndef SGGrabFrameComplete
	PyMac_PRECHECK(SGGrabFrameComplete);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &c,
	                      &bufferNum))
		return NULL;
	_rv = SGGrabFrameComplete(c,
	                          bufferNum,
	                          &done);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     done);
	return _res;
}

static PyObject *Qt_SGCompressFrame(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short bufferNum;
#ifndef SGCompressFrame
	PyMac_PRECHECK(SGCompressFrame);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &c,
	                      &bufferNum))
		return NULL;
	_rv = SGCompressFrame(c,
	                      bufferNum);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetCompressBuffer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short depth;
	Rect compressSize;
#ifndef SGSetCompressBuffer
	PyMac_PRECHECK(SGSetCompressBuffer);
#endif
	if (!PyArg_ParseTuple(_args, "O&hO&",
	                      CmpInstObj_Convert, &c,
	                      &depth,
	                      PyMac_GetRect, &compressSize))
		return NULL;
	_rv = SGSetCompressBuffer(c,
	                          depth,
	                          &compressSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetCompressBuffer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short depth;
	Rect compressSize;
#ifndef SGGetCompressBuffer
	PyMac_PRECHECK(SGGetCompressBuffer);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetCompressBuffer(c,
	                          &depth,
	                          &compressSize);
	_res = Py_BuildValue("lhO&",
	                     _rv,
	                     depth,
	                     PyMac_BuildRect, &compressSize);
	return _res;
}

static PyObject *Qt_SGGetBufferInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short bufferNum;
	PixMapHandle bufferPM;
	Rect bufferRect;
	GWorldPtr compressBuffer;
	Rect compressBufferRect;
#ifndef SGGetBufferInfo
	PyMac_PRECHECK(SGGetBufferInfo);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &c,
	                      &bufferNum))
		return NULL;
	_rv = SGGetBufferInfo(c,
	                      bufferNum,
	                      &bufferPM,
	                      &bufferRect,
	                      &compressBuffer,
	                      &compressBufferRect);
	_res = Py_BuildValue("lO&O&O&O&",
	                     _rv,
	                     ResObj_New, bufferPM,
	                     PyMac_BuildRect, &bufferRect,
	                     GWorldObj_New, compressBuffer,
	                     PyMac_BuildRect, &compressBufferRect);
	return _res;
}

static PyObject *Qt_SGSetUseScreenBuffer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Boolean useScreenBuffer;
#ifndef SGSetUseScreenBuffer
	PyMac_PRECHECK(SGSetUseScreenBuffer);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &c,
	                      &useScreenBuffer))
		return NULL;
	_rv = SGSetUseScreenBuffer(c,
	                           useScreenBuffer);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetUseScreenBuffer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Boolean useScreenBuffer;
#ifndef SGGetUseScreenBuffer
	PyMac_PRECHECK(SGGetUseScreenBuffer);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetUseScreenBuffer(c,
	                           &useScreenBuffer);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     useScreenBuffer);
	return _res;
}

static PyObject *Qt_SGSetFrameRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Fixed frameRate;
#ifndef SGSetFrameRate
	PyMac_PRECHECK(SGSetFrameRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      PyMac_GetFixed, &frameRate))
		return NULL;
	_rv = SGSetFrameRate(c,
	                     frameRate);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetFrameRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Fixed frameRate;
#ifndef SGGetFrameRate
	PyMac_PRECHECK(SGGetFrameRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetFrameRate(c,
	                     &frameRate);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildFixed, frameRate);
	return _res;
}

static PyObject *Qt_SGSetPreferredPacketSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long preferredPacketSizeInBytes;
#ifndef SGSetPreferredPacketSize
	PyMac_PRECHECK(SGSetPreferredPacketSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &c,
	                      &preferredPacketSizeInBytes))
		return NULL;
	_rv = SGSetPreferredPacketSize(c,
	                               preferredPacketSizeInBytes);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetPreferredPacketSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long preferredPacketSizeInBytes;
#ifndef SGGetPreferredPacketSize
	PyMac_PRECHECK(SGGetPreferredPacketSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetPreferredPacketSize(c,
	                               &preferredPacketSizeInBytes);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     preferredPacketSizeInBytes);
	return _res;
}

static PyObject *Qt_SGSetUserVideoCompressorList(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Handle compressorTypes;
#ifndef SGSetUserVideoCompressorList
	PyMac_PRECHECK(SGSetUserVideoCompressorList);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      ResObj_Convert, &compressorTypes))
		return NULL;
	_rv = SGSetUserVideoCompressorList(c,
	                                   compressorTypes);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetUserVideoCompressorList(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Handle compressorTypes;
#ifndef SGGetUserVideoCompressorList
	PyMac_PRECHECK(SGGetUserVideoCompressorList);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetUserVideoCompressorList(c,
	                                   &compressorTypes);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, compressorTypes);
	return _res;
}

static PyObject *Qt_SGSetSoundInputDriver(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Str255 driverName;
#ifndef SGSetSoundInputDriver
	PyMac_PRECHECK(SGSetSoundInputDriver);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      PyMac_GetStr255, driverName))
		return NULL;
	_rv = SGSetSoundInputDriver(c,
	                            driverName);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetSoundInputDriver(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	SGChannel c;
#ifndef SGGetSoundInputDriver
	PyMac_PRECHECK(SGGetSoundInputDriver);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetSoundInputDriver(c);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSoundInputDriverChanged(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
#ifndef SGSoundInputDriverChanged
	PyMac_PRECHECK(SGSoundInputDriverChanged);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGSoundInputDriverChanged(c);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetSoundRecordChunkSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	long seconds;
#ifndef SGSetSoundRecordChunkSize
	PyMac_PRECHECK(SGSetSoundRecordChunkSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &c,
	                      &seconds))
		return NULL;
	_rv = SGSetSoundRecordChunkSize(c,
	                                seconds);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetSoundRecordChunkSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	SGChannel c;
#ifndef SGGetSoundRecordChunkSize
	PyMac_PRECHECK(SGGetSoundRecordChunkSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetSoundRecordChunkSize(c);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetSoundInputRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Fixed rate;
#ifndef SGSetSoundInputRate
	PyMac_PRECHECK(SGSetSoundInputRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      PyMac_GetFixed, &rate))
		return NULL;
	_rv = SGSetSoundInputRate(c,
	                          rate);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetSoundInputRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Fixed _rv;
	SGChannel c;
#ifndef SGGetSoundInputRate
	PyMac_PRECHECK(SGGetSoundInputRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetSoundInputRate(c);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFixed, _rv);
	return _res;
}

static PyObject *Qt_SGSetSoundInputParameters(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short sampleSize;
	short numChannels;
	OSType compressionType;
#ifndef SGSetSoundInputParameters
	PyMac_PRECHECK(SGSetSoundInputParameters);
#endif
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      CmpInstObj_Convert, &c,
	                      &sampleSize,
	                      &numChannels,
	                      PyMac_GetOSType, &compressionType))
		return NULL;
	_rv = SGSetSoundInputParameters(c,
	                                sampleSize,
	                                numChannels,
	                                compressionType);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetSoundInputParameters(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short sampleSize;
	short numChannels;
	OSType compressionType;
#ifndef SGGetSoundInputParameters
	PyMac_PRECHECK(SGGetSoundInputParameters);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetSoundInputParameters(c,
	                                &sampleSize,
	                                &numChannels,
	                                &compressionType);
	_res = Py_BuildValue("lhhO&",
	                     _rv,
	                     sampleSize,
	                     numChannels,
	                     PyMac_BuildOSType, compressionType);
	return _res;
}

static PyObject *Qt_SGSetAdditionalSoundRates(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Handle rates;
#ifndef SGSetAdditionalSoundRates
	PyMac_PRECHECK(SGSetAdditionalSoundRates);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &c,
	                      ResObj_Convert, &rates))
		return NULL;
	_rv = SGSetAdditionalSoundRates(c,
	                                rates);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetAdditionalSoundRates(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	Handle rates;
#ifndef SGGetAdditionalSoundRates
	PyMac_PRECHECK(SGGetAdditionalSoundRates);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetAdditionalSoundRates(c,
	                                &rates);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, rates);
	return _res;
}

static PyObject *Qt_SGSetFontName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	StringPtr pstr;
#ifndef SGSetFontName
	PyMac_PRECHECK(SGSetFontName);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &c,
	                      &pstr))
		return NULL;
	_rv = SGSetFontName(c,
	                    pstr);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetFontSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short fontSize;
#ifndef SGSetFontSize
	PyMac_PRECHECK(SGSetFontSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &c,
	                      &fontSize))
		return NULL;
	_rv = SGSetFontSize(c,
	                    fontSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGSetTextForeColor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	RGBColor theColor;
#ifndef SGSetTextForeColor
	PyMac_PRECHECK(SGSetTextForeColor);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGSetTextForeColor(c,
	                         &theColor);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     QdRGB_New, &theColor);
	return _res;
}

static PyObject *Qt_SGSetTextBackColor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	RGBColor theColor;
#ifndef SGSetTextBackColor
	PyMac_PRECHECK(SGSetTextBackColor);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGSetTextBackColor(c,
	                         &theColor);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     QdRGB_New, &theColor);
	return _res;
}

static PyObject *Qt_SGSetJustification(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short just;
#ifndef SGSetJustification
	PyMac_PRECHECK(SGSetJustification);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &c,
	                      &just))
		return NULL;
	_rv = SGSetJustification(c,
	                         just);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_SGGetTextReturnToSpaceValue(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short rettospace;
#ifndef SGGetTextReturnToSpaceValue
	PyMac_PRECHECK(SGGetTextReturnToSpaceValue);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &c))
		return NULL;
	_rv = SGGetTextReturnToSpaceValue(c,
	                                  &rettospace);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     rettospace);
	return _res;
}

static PyObject *Qt_SGSetTextReturnToSpaceValue(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	SGChannel c;
	short rettospace;
#ifndef SGSetTextReturnToSpaceValue
	PyMac_PRECHECK(SGSetTextReturnToSpaceValue);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &c,
	                      &rettospace))
		return NULL;
	_rv = SGSetTextReturnToSpaceValue(c,
	                                  rettospace);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTVideoOutputGetCurrentClientName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
	Str255 str;
#ifndef QTVideoOutputGetCurrentClientName
	PyMac_PRECHECK(QTVideoOutputGetCurrentClientName);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &vo,
	                      PyMac_GetStr255, str))
		return NULL;
	_rv = QTVideoOutputGetCurrentClientName(vo,
	                                        str);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTVideoOutputSetClientName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
	Str255 str;
#ifndef QTVideoOutputSetClientName
	PyMac_PRECHECK(QTVideoOutputSetClientName);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &vo,
	                      PyMac_GetStr255, str))
		return NULL;
	_rv = QTVideoOutputSetClientName(vo,
	                                 str);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTVideoOutputGetClientName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
	Str255 str;
#ifndef QTVideoOutputGetClientName
	PyMac_PRECHECK(QTVideoOutputGetClientName);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &vo,
	                      PyMac_GetStr255, str))
		return NULL;
	_rv = QTVideoOutputGetClientName(vo,
	                                 str);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTVideoOutputBegin(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
#ifndef QTVideoOutputBegin
	PyMac_PRECHECK(QTVideoOutputBegin);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &vo))
		return NULL;
	_rv = QTVideoOutputBegin(vo);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTVideoOutputEnd(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
#ifndef QTVideoOutputEnd
	PyMac_PRECHECK(QTVideoOutputEnd);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &vo))
		return NULL;
	_rv = QTVideoOutputEnd(vo);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTVideoOutputSetDisplayMode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
	long displayModeID;
#ifndef QTVideoOutputSetDisplayMode
	PyMac_PRECHECK(QTVideoOutputSetDisplayMode);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &vo,
	                      &displayModeID))
		return NULL;
	_rv = QTVideoOutputSetDisplayMode(vo,
	                                  displayModeID);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTVideoOutputGetDisplayMode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
	long displayModeID;
#ifndef QTVideoOutputGetDisplayMode
	PyMac_PRECHECK(QTVideoOutputGetDisplayMode);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &vo))
		return NULL;
	_rv = QTVideoOutputGetDisplayMode(vo,
	                                  &displayModeID);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     displayModeID);
	return _res;
}

static PyObject *Qt_QTVideoOutputGetGWorld(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
	GWorldPtr gw;
#ifndef QTVideoOutputGetGWorld
	PyMac_PRECHECK(QTVideoOutputGetGWorld);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &vo))
		return NULL;
	_rv = QTVideoOutputGetGWorld(vo,
	                             &gw);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     GWorldObj_New, gw);
	return _res;
}

static PyObject *Qt_QTVideoOutputGetIndSoundOutput(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
	long index;
	Component outputComponent;
#ifndef QTVideoOutputGetIndSoundOutput
	PyMac_PRECHECK(QTVideoOutputGetIndSoundOutput);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &vo,
	                      &index))
		return NULL;
	_rv = QTVideoOutputGetIndSoundOutput(vo,
	                                     index,
	                                     &outputComponent);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     CmpObj_New, outputComponent);
	return _res;
}

static PyObject *Qt_QTVideoOutputGetClock(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
	ComponentInstance clock;
#ifndef QTVideoOutputGetClock
	PyMac_PRECHECK(QTVideoOutputGetClock);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &vo))
		return NULL;
	_rv = QTVideoOutputGetClock(vo,
	                            &clock);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     CmpInstObj_New, clock);
	return _res;
}

static PyObject *Qt_QTVideoOutputSetEchoPort(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
	CGrafPtr echoPort;
#ifndef QTVideoOutputSetEchoPort
	PyMac_PRECHECK(QTVideoOutputSetEchoPort);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &vo,
	                      GrafObj_Convert, &echoPort))
		return NULL;
	_rv = QTVideoOutputSetEchoPort(vo,
	                               echoPort);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTVideoOutputGetIndImageDecompressor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
	long index;
	Component codec;
#ifndef QTVideoOutputGetIndImageDecompressor
	PyMac_PRECHECK(QTVideoOutputGetIndImageDecompressor);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &vo,
	                      &index))
		return NULL;
	_rv = QTVideoOutputGetIndImageDecompressor(vo,
	                                           index,
	                                           &codec);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     CmpObj_New, codec);
	return _res;
}

static PyObject *Qt_QTVideoOutputBaseSetEchoPort(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTVideoOutputComponent vo;
	CGrafPtr echoPort;
#ifndef QTVideoOutputBaseSetEchoPort
	PyMac_PRECHECK(QTVideoOutputBaseSetEchoPort);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &vo,
	                      GrafObj_Convert, &echoPort))
		return NULL;
	_rv = QTVideoOutputBaseSetEchoPort(vo,
	                                   echoPort);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSetChunkManagementFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	UInt32 flags;
	UInt32 flagsMask;
#ifndef MediaSetChunkManagementFlags
	PyMac_PRECHECK(MediaSetChunkManagementFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mh,
	                      &flags,
	                      &flagsMask))
		return NULL;
	_rv = MediaSetChunkManagementFlags(mh,
	                                   flags,
	                                   flagsMask);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetChunkManagementFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	UInt32 flags;
#ifndef MediaGetChunkManagementFlags
	PyMac_PRECHECK(MediaGetChunkManagementFlags);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetChunkManagementFlags(mh,
	                                   &flags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     flags);
	return _res;
}

static PyObject *Qt_MediaSetPurgeableChunkMemoryAllowance(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Size allowance;
#ifndef MediaSetPurgeableChunkMemoryAllowance
	PyMac_PRECHECK(MediaSetPurgeableChunkMemoryAllowance);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &allowance))
		return NULL;
	_rv = MediaSetPurgeableChunkMemoryAllowance(mh,
	                                            allowance);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetPurgeableChunkMemoryAllowance(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Size allowance;
#ifndef MediaGetPurgeableChunkMemoryAllowance
	PyMac_PRECHECK(MediaGetPurgeableChunkMemoryAllowance);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetPurgeableChunkMemoryAllowance(mh,
	                                            &allowance);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     allowance);
	return _res;
}

static PyObject *Qt_MediaEmptyAllPurgeableChunks(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
#ifndef MediaEmptyAllPurgeableChunks
	PyMac_PRECHECK(MediaEmptyAllPurgeableChunks);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaEmptyAllPurgeableChunks(mh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSetHandlerCapabilities(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long flags;
	long flagsMask;
#ifndef MediaSetHandlerCapabilities
	PyMac_PRECHECK(MediaSetHandlerCapabilities);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mh,
	                      &flags,
	                      &flagsMask))
		return NULL;
	_rv = MediaSetHandlerCapabilities(mh,
	                                  flags,
	                                  flagsMask);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaIdle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	TimeValue atMediaTime;
	long flagsIn;
	long flagsOut;
	TimeRecord movieTime;
#ifndef MediaIdle
	PyMac_PRECHECK(MediaIdle);
#endif
	if (!PyArg_ParseTuple(_args, "O&llO&",
	                      CmpInstObj_Convert, &mh,
	                      &atMediaTime,
	                      &flagsIn,
	                      QtTimeRecord_Convert, &movieTime))
		return NULL;
	_rv = MediaIdle(mh,
	                atMediaTime,
	                flagsIn,
	                &flagsOut,
	                &movieTime);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     flagsOut);
	return _res;
}

static PyObject *Qt_MediaGetMediaInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Handle h;
#ifndef MediaGetMediaInfo
	PyMac_PRECHECK(MediaGetMediaInfo);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &h))
		return NULL;
	_rv = MediaGetMediaInfo(mh,
	                        h);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaPutMediaInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Handle h;
#ifndef MediaPutMediaInfo
	PyMac_PRECHECK(MediaPutMediaInfo);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &h))
		return NULL;
	_rv = MediaPutMediaInfo(mh,
	                        h);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSetActive(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Boolean enableMedia;
#ifndef MediaSetActive
	PyMac_PRECHECK(MediaSetActive);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &mh,
	                      &enableMedia))
		return NULL;
	_rv = MediaSetActive(mh,
	                     enableMedia);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSetRate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Fixed rate;
#ifndef MediaSetRate
	PyMac_PRECHECK(MediaSetRate);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetFixed, &rate))
		return NULL;
	_rv = MediaSetRate(mh,
	                   rate);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGGetStatus(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	ComponentResult statusErr;
#ifndef MediaGGetStatus
	PyMac_PRECHECK(MediaGGetStatus);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGGetStatus(mh,
	                      &statusErr);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     statusErr);
	return _res;
}

static PyObject *Qt_MediaTrackEdited(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
#ifndef MediaTrackEdited
	PyMac_PRECHECK(MediaTrackEdited);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaTrackEdited(mh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSetMediaTimeScale(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	TimeScale newTimeScale;
#ifndef MediaSetMediaTimeScale
	PyMac_PRECHECK(MediaSetMediaTimeScale);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &newTimeScale))
		return NULL;
	_rv = MediaSetMediaTimeScale(mh,
	                             newTimeScale);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSetMovieTimeScale(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	TimeScale newTimeScale;
#ifndef MediaSetMovieTimeScale
	PyMac_PRECHECK(MediaSetMovieTimeScale);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &newTimeScale))
		return NULL;
	_rv = MediaSetMovieTimeScale(mh,
	                             newTimeScale);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSetGWorld(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	CGrafPtr aPort;
	GDHandle aGD;
#ifndef MediaSetGWorld
	PyMac_PRECHECK(MediaSetGWorld);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &mh,
	                      GrafObj_Convert, &aPort,
	                      OptResObj_Convert, &aGD))
		return NULL;
	_rv = MediaSetGWorld(mh,
	                     aPort,
	                     aGD);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSetDimensions(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Fixed width;
	Fixed height;
#ifndef MediaSetDimensions
	PyMac_PRECHECK(MediaSetDimensions);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetFixed, &width,
	                      PyMac_GetFixed, &height))
		return NULL;
	_rv = MediaSetDimensions(mh,
	                         width,
	                         height);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSetClip(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	RgnHandle theClip;
#ifndef MediaSetClip
	PyMac_PRECHECK(MediaSetClip);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &theClip))
		return NULL;
	_rv = MediaSetClip(mh,
	                   theClip);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetTrackOpaque(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Boolean trackIsOpaque;
#ifndef MediaGetTrackOpaque
	PyMac_PRECHECK(MediaGetTrackOpaque);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetTrackOpaque(mh,
	                          &trackIsOpaque);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     trackIsOpaque);
	return _res;
}

static PyObject *Qt_MediaSetGraphicsMode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long mode;
	RGBColor opColor;
#ifndef MediaSetGraphicsMode
	PyMac_PRECHECK(MediaSetGraphicsMode);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      CmpInstObj_Convert, &mh,
	                      &mode,
	                      QdRGB_Convert, &opColor))
		return NULL;
	_rv = MediaSetGraphicsMode(mh,
	                           mode,
	                           &opColor);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetGraphicsMode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long mode;
	RGBColor opColor;
#ifndef MediaGetGraphicsMode
	PyMac_PRECHECK(MediaGetGraphicsMode);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetGraphicsMode(mh,
	                           &mode,
	                           &opColor);
	_res = Py_BuildValue("llO&",
	                     _rv,
	                     mode,
	                     QdRGB_New, &opColor);
	return _res;
}

static PyObject *Qt_MediaGSetVolume(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short volume;
#ifndef MediaGSetVolume
	PyMac_PRECHECK(MediaGSetVolume);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &mh,
	                      &volume))
		return NULL;
	_rv = MediaGSetVolume(mh,
	                      volume);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSetSoundBalance(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short balance;
#ifndef MediaSetSoundBalance
	PyMac_PRECHECK(MediaSetSoundBalance);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &mh,
	                      &balance))
		return NULL;
	_rv = MediaSetSoundBalance(mh,
	                           balance);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetSoundBalance(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short balance;
#ifndef MediaGetSoundBalance
	PyMac_PRECHECK(MediaGetSoundBalance);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetSoundBalance(mh,
	                           &balance);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     balance);
	return _res;
}

static PyObject *Qt_MediaGetNextBoundsChange(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	TimeValue when;
#ifndef MediaGetNextBoundsChange
	PyMac_PRECHECK(MediaGetNextBoundsChange);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetNextBoundsChange(mh,
	                               &when);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     when);
	return _res;
}

static PyObject *Qt_MediaGetSrcRgn(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	RgnHandle rgn;
	TimeValue atMediaTime;
#ifndef MediaGetSrcRgn
	PyMac_PRECHECK(MediaGetSrcRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &rgn,
	                      &atMediaTime))
		return NULL;
	_rv = MediaGetSrcRgn(mh,
	                     rgn,
	                     atMediaTime);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaPreroll(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	TimeValue time;
	Fixed rate;
#ifndef MediaPreroll
	PyMac_PRECHECK(MediaPreroll);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      CmpInstObj_Convert, &mh,
	                      &time,
	                      PyMac_GetFixed, &rate))
		return NULL;
	_rv = MediaPreroll(mh,
	                   time,
	                   rate);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSampleDescriptionChanged(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long index;
#ifndef MediaSampleDescriptionChanged
	PyMac_PRECHECK(MediaSampleDescriptionChanged);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &index))
		return NULL;
	_rv = MediaSampleDescriptionChanged(mh,
	                                    index);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaHasCharacteristic(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	OSType characteristic;
	Boolean hasIt;
#ifndef MediaHasCharacteristic
	PyMac_PRECHECK(MediaHasCharacteristic);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetOSType, &characteristic))
		return NULL;
	_rv = MediaHasCharacteristic(mh,
	                             characteristic,
	                             &hasIt);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     hasIt);
	return _res;
}

static PyObject *Qt_MediaGetOffscreenBufferSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Rect bounds;
	short depth;
	CTabHandle ctab;
#ifndef MediaGetOffscreenBufferSize
	PyMac_PRECHECK(MediaGetOffscreenBufferSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&hO&",
	                      CmpInstObj_Convert, &mh,
	                      &depth,
	                      ResObj_Convert, &ctab))
		return NULL;
	_rv = MediaGetOffscreenBufferSize(mh,
	                                  &bounds,
	                                  depth,
	                                  ctab);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     PyMac_BuildRect, &bounds);
	return _res;
}

static PyObject *Qt_MediaSetHints(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long hints;
#ifndef MediaSetHints
	PyMac_PRECHECK(MediaSetHints);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &hints))
		return NULL;
	_rv = MediaSetHints(mh,
	                    hints);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Str255 name;
	long requestedLanguage;
	long actualLanguage;
#ifndef MediaGetName
	PyMac_PRECHECK(MediaGetName);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetStr255, name,
	                      &requestedLanguage))
		return NULL;
	_rv = MediaGetName(mh,
	                   name,
	                   requestedLanguage,
	                   &actualLanguage);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     actualLanguage);
	return _res;
}

static PyObject *Qt_MediaForceUpdate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long forceUpdateFlags;
#ifndef MediaForceUpdate
	PyMac_PRECHECK(MediaForceUpdate);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &forceUpdateFlags))
		return NULL;
	_rv = MediaForceUpdate(mh,
	                       forceUpdateFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetDrawingRgn(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	RgnHandle partialRgn;
#ifndef MediaGetDrawingRgn
	PyMac_PRECHECK(MediaGetDrawingRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetDrawingRgn(mh,
	                         &partialRgn);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, partialRgn);
	return _res;
}

static PyObject *Qt_MediaGSetActiveSegment(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	TimeValue activeStart;
	TimeValue activeDuration;
#ifndef MediaGSetActiveSegment
	PyMac_PRECHECK(MediaGSetActiveSegment);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mh,
	                      &activeStart,
	                      &activeDuration))
		return NULL;
	_rv = MediaGSetActiveSegment(mh,
	                             activeStart,
	                             activeDuration);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaInvalidateRegion(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	RgnHandle invalRgn;
#ifndef MediaInvalidateRegion
	PyMac_PRECHECK(MediaInvalidateRegion);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &invalRgn))
		return NULL;
	_rv = MediaInvalidateRegion(mh,
	                            invalRgn);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetNextStepTime(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short flags;
	TimeValue mediaTimeIn;
	TimeValue mediaTimeOut;
	Fixed rate;
#ifndef MediaGetNextStepTime
	PyMac_PRECHECK(MediaGetNextStepTime);
#endif
	if (!PyArg_ParseTuple(_args, "O&hlO&",
	                      CmpInstObj_Convert, &mh,
	                      &flags,
	                      &mediaTimeIn,
	                      PyMac_GetFixed, &rate))
		return NULL;
	_rv = MediaGetNextStepTime(mh,
	                           flags,
	                           mediaTimeIn,
	                           &mediaTimeOut,
	                           rate);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     mediaTimeOut);
	return _res;
}

static PyObject *Qt_MediaChangedNonPrimarySource(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long inputIndex;
#ifndef MediaChangedNonPrimarySource
	PyMac_PRECHECK(MediaChangedNonPrimarySource);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &inputIndex))
		return NULL;
	_rv = MediaChangedNonPrimarySource(mh,
	                                   inputIndex);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaTrackReferencesChanged(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
#ifndef MediaTrackReferencesChanged
	PyMac_PRECHECK(MediaTrackReferencesChanged);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaTrackReferencesChanged(mh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaReleaseSampleDataPointer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long sampleNum;
#ifndef MediaReleaseSampleDataPointer
	PyMac_PRECHECK(MediaReleaseSampleDataPointer);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &sampleNum))
		return NULL;
	_rv = MediaReleaseSampleDataPointer(mh,
	                                    sampleNum);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaTrackPropertyAtomChanged(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
#ifndef MediaTrackPropertyAtomChanged
	PyMac_PRECHECK(MediaTrackPropertyAtomChanged);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaTrackPropertyAtomChanged(mh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSetVideoParam(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long whichParam;
	unsigned short value;
#ifndef MediaSetVideoParam
	PyMac_PRECHECK(MediaSetVideoParam);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &whichParam))
		return NULL;
	_rv = MediaSetVideoParam(mh,
	                         whichParam,
	                         &value);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     value);
	return _res;
}

static PyObject *Qt_MediaGetVideoParam(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long whichParam;
	unsigned short value;
#ifndef MediaGetVideoParam
	PyMac_PRECHECK(MediaGetVideoParam);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &whichParam))
		return NULL;
	_rv = MediaGetVideoParam(mh,
	                         whichParam,
	                         &value);
	_res = Py_BuildValue("lH",
	                     _rv,
	                     value);
	return _res;
}

static PyObject *Qt_MediaCompare(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Boolean isOK;
	Media srcMedia;
	ComponentInstance srcMediaComponent;
#ifndef MediaCompare
	PyMac_PRECHECK(MediaCompare);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      CmpInstObj_Convert, &mh,
	                      MediaObj_Convert, &srcMedia,
	                      CmpInstObj_Convert, &srcMediaComponent))
		return NULL;
	_rv = MediaCompare(mh,
	                   &isOK,
	                   srcMedia,
	                   srcMediaComponent);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     isOK);
	return _res;
}

static PyObject *Qt_MediaGetClock(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	ComponentInstance clock;
#ifndef MediaGetClock
	PyMac_PRECHECK(MediaGetClock);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetClock(mh,
	                    &clock);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     CmpInstObj_New, clock);
	return _res;
}

static PyObject *Qt_MediaSetSoundOutputComponent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Component outputComponent;
#ifndef MediaSetSoundOutputComponent
	PyMac_PRECHECK(MediaSetSoundOutputComponent);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      CmpObj_Convert, &outputComponent))
		return NULL;
	_rv = MediaSetSoundOutputComponent(mh,
	                                   outputComponent);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetSoundOutputComponent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Component outputComponent;
#ifndef MediaGetSoundOutputComponent
	PyMac_PRECHECK(MediaGetSoundOutputComponent);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetSoundOutputComponent(mh,
	                                   &outputComponent);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     CmpObj_New, outputComponent);
	return _res;
}

static PyObject *Qt_MediaSetSoundLocalizationData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Handle data;
#ifndef MediaSetSoundLocalizationData
	PyMac_PRECHECK(MediaSetSoundLocalizationData);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &data))
		return NULL;
	_rv = MediaSetSoundLocalizationData(mh,
	                                    data);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetInvalidRegion(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	RgnHandle rgn;
#ifndef MediaGetInvalidRegion
	PyMac_PRECHECK(MediaGetInvalidRegion);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &rgn))
		return NULL;
	_rv = MediaGetInvalidRegion(mh,
	                            rgn);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSampleDescriptionB2N(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	SampleDescriptionHandle sampleDescriptionH;
#ifndef MediaSampleDescriptionB2N
	PyMac_PRECHECK(MediaSampleDescriptionB2N);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &sampleDescriptionH))
		return NULL;
	_rv = MediaSampleDescriptionB2N(mh,
	                                sampleDescriptionH);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSampleDescriptionN2B(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	SampleDescriptionHandle sampleDescriptionH;
#ifndef MediaSampleDescriptionN2B
	PyMac_PRECHECK(MediaSampleDescriptionN2B);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      ResObj_Convert, &sampleDescriptionH))
		return NULL;
	_rv = MediaSampleDescriptionN2B(mh,
	                                sampleDescriptionH);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaFlushNonPrimarySourceData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long inputIndex;
#ifndef MediaFlushNonPrimarySourceData
	PyMac_PRECHECK(MediaFlushNonPrimarySourceData);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &inputIndex))
		return NULL;
	_rv = MediaFlushNonPrimarySourceData(mh,
	                                     inputIndex);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetURLLink(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Point displayWhere;
	Handle urlLink;
#ifndef MediaGetURLLink
	PyMac_PRECHECK(MediaGetURLLink);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetPoint, &displayWhere))
		return NULL;
	_rv = MediaGetURLLink(mh,
	                      displayWhere,
	                      &urlLink);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, urlLink);
	return _res;
}

static PyObject *Qt_MediaHitTestForTargetRefCon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long flags;
	Point loc;
	long targetRefCon;
#ifndef MediaHitTestForTargetRefCon
	PyMac_PRECHECK(MediaHitTestForTargetRefCon);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      CmpInstObj_Convert, &mh,
	                      &flags,
	                      PyMac_GetPoint, &loc))
		return NULL;
	_rv = MediaHitTestForTargetRefCon(mh,
	                                  flags,
	                                  loc,
	                                  &targetRefCon);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     targetRefCon);
	return _res;
}

static PyObject *Qt_MediaHitTestTargetRefCon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long targetRefCon;
	long flags;
	Point loc;
	Boolean wasHit;
#ifndef MediaHitTestTargetRefCon
	PyMac_PRECHECK(MediaHitTestTargetRefCon);
#endif
	if (!PyArg_ParseTuple(_args, "O&llO&",
	                      CmpInstObj_Convert, &mh,
	                      &targetRefCon,
	                      &flags,
	                      PyMac_GetPoint, &loc))
		return NULL;
	_rv = MediaHitTestTargetRefCon(mh,
	                               targetRefCon,
	                               flags,
	                               loc,
	                               &wasHit);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     wasHit);
	return _res;
}

static PyObject *Qt_MediaDisposeTargetRefCon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long targetRefCon;
#ifndef MediaDisposeTargetRefCon
	PyMac_PRECHECK(MediaDisposeTargetRefCon);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &targetRefCon))
		return NULL;
	_rv = MediaDisposeTargetRefCon(mh,
	                               targetRefCon);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaTargetRefConsEqual(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long firstRefCon;
	long secondRefCon;
	Boolean equal;
#ifndef MediaTargetRefConsEqual
	PyMac_PRECHECK(MediaTargetRefConsEqual);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mh,
	                      &firstRefCon,
	                      &secondRefCon))
		return NULL;
	_rv = MediaTargetRefConsEqual(mh,
	                              firstRefCon,
	                              secondRefCon,
	                              &equal);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     equal);
	return _res;
}

static PyObject *Qt_MediaPrePrerollCancel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	void * refcon;
#ifndef MediaPrePrerollCancel
	PyMac_PRECHECK(MediaPrePrerollCancel);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &mh,
	                      &refcon))
		return NULL;
	_rv = MediaPrePrerollCancel(mh,
	                            refcon);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaEnterEmptyEdit(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
#ifndef MediaEnterEmptyEdit
	PyMac_PRECHECK(MediaEnterEmptyEdit);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaEnterEmptyEdit(mh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaCurrentMediaQueuedData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long milliSecs;
#ifndef MediaCurrentMediaQueuedData
	PyMac_PRECHECK(MediaCurrentMediaQueuedData);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaCurrentMediaQueuedData(mh,
	                                  &milliSecs);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     milliSecs);
	return _res;
}

static PyObject *Qt_MediaGetEffectiveVolume(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short volume;
#ifndef MediaGetEffectiveVolume
	PyMac_PRECHECK(MediaGetEffectiveVolume);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetEffectiveVolume(mh,
	                              &volume);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     volume);
	return _res;
}

static PyObject *Qt_MediaGetSoundLevelMeteringEnabled(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Boolean enabled;
#ifndef MediaGetSoundLevelMeteringEnabled
	PyMac_PRECHECK(MediaGetSoundLevelMeteringEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetSoundLevelMeteringEnabled(mh,
	                                        &enabled);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     enabled);
	return _res;
}

static PyObject *Qt_MediaSetSoundLevelMeteringEnabled(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Boolean enable;
#ifndef MediaSetSoundLevelMeteringEnabled
	PyMac_PRECHECK(MediaSetSoundLevelMeteringEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &mh,
	                      &enable))
		return NULL;
	_rv = MediaSetSoundLevelMeteringEnabled(mh,
	                                        enable);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetEffectiveSoundBalance(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short balance;
#ifndef MediaGetEffectiveSoundBalance
	PyMac_PRECHECK(MediaGetEffectiveSoundBalance);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetEffectiveSoundBalance(mh,
	                                    &balance);
	_res = Py_BuildValue("lh",
	                     _rv,
	                     balance);
	return _res;
}

static PyObject *Qt_MediaSetScreenLock(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	Boolean lockIt;
#ifndef MediaSetScreenLock
	PyMac_PRECHECK(MediaSetScreenLock);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      CmpInstObj_Convert, &mh,
	                      &lockIt))
		return NULL;
	_rv = MediaSetScreenLock(mh,
	                         lockIt);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetErrorString(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	ComponentResult theError;
	Str255 errorString;
#ifndef MediaGetErrorString
	PyMac_PRECHECK(MediaGetErrorString);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      CmpInstObj_Convert, &mh,
	                      &theError,
	                      PyMac_GetStr255, errorString))
		return NULL;
	_rv = MediaGetErrorString(mh,
	                          theError,
	                          errorString);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetSoundEqualizerBandLevels(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	UInt8 bandLevels;
#ifndef MediaGetSoundEqualizerBandLevels
	PyMac_PRECHECK(MediaGetSoundEqualizerBandLevels);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetSoundEqualizerBandLevels(mh,
	                                       &bandLevels);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     bandLevels);
	return _res;
}

static PyObject *Qt_MediaDoIdleActions(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
#ifndef MediaDoIdleActions
	PyMac_PRECHECK(MediaDoIdleActions);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaDoIdleActions(mh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaSetSoundBassAndTreble(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short bass;
	short treble;
#ifndef MediaSetSoundBassAndTreble
	PyMac_PRECHECK(MediaSetSoundBassAndTreble);
#endif
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      CmpInstObj_Convert, &mh,
	                      &bass,
	                      &treble))
		return NULL;
	_rv = MediaSetSoundBassAndTreble(mh,
	                                 bass,
	                                 treble);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetSoundBassAndTreble(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short bass;
	short treble;
#ifndef MediaGetSoundBassAndTreble
	PyMac_PRECHECK(MediaGetSoundBassAndTreble);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetSoundBassAndTreble(mh,
	                                 &bass,
	                                 &treble);
	_res = Py_BuildValue("lhh",
	                     _rv,
	                     bass,
	                     treble);
	return _res;
}

static PyObject *Qt_MediaTimeBaseChanged(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
#ifndef MediaTimeBaseChanged
	PyMac_PRECHECK(MediaTimeBaseChanged);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaTimeBaseChanged(mh);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaMCIsPlayerEvent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	EventRecord e;
	Boolean handledIt;
#ifndef MediaMCIsPlayerEvent
	PyMac_PRECHECK(MediaMCIsPlayerEvent);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetEventRecord, &e))
		return NULL;
	_rv = MediaMCIsPlayerEvent(mh,
	                           &e,
	                           &handledIt);
	_res = Py_BuildValue("lb",
	                     _rv,
	                     handledIt);
	return _res;
}

static PyObject *Qt_MediaGetMediaLoadState(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long mediaLoadState;
#ifndef MediaGetMediaLoadState
	PyMac_PRECHECK(MediaGetMediaLoadState);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGetMediaLoadState(mh,
	                             &mediaLoadState);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     mediaLoadState);
	return _res;
}

static PyObject *Qt_MediaVideoOutputChanged(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	ComponentInstance vout;
#ifndef MediaVideoOutputChanged
	PyMac_PRECHECK(MediaVideoOutputChanged);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      CmpInstObj_Convert, &vout))
		return NULL;
	_rv = MediaVideoOutputChanged(mh,
	                              vout);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaEmptySampleCache(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long sampleNum;
	long sampleCount;
#ifndef MediaEmptySampleCache
	PyMac_PRECHECK(MediaEmptySampleCache);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mh,
	                      &sampleNum,
	                      &sampleCount))
		return NULL;
	_rv = MediaEmptySampleCache(mh,
	                            sampleNum,
	                            sampleCount);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaGetPublicInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	OSType infoSelector;
	void * infoDataPtr;
	Size ioDataSize;
#ifndef MediaGetPublicInfo
	PyMac_PRECHECK(MediaGetPublicInfo);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&s",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetOSType, &infoSelector,
	                      &infoDataPtr))
		return NULL;
	_rv = MediaGetPublicInfo(mh,
	                         infoSelector,
	                         infoDataPtr,
	                         &ioDataSize);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     ioDataSize);
	return _res;
}

static PyObject *Qt_MediaSetPublicInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	OSType infoSelector;
	void * infoDataPtr;
	Size dataSize;
#ifndef MediaSetPublicInfo
	PyMac_PRECHECK(MediaSetPublicInfo);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&sl",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetOSType, &infoSelector,
	                      &infoDataPtr,
	                      &dataSize))
		return NULL;
	_rv = MediaSetPublicInfo(mh,
	                         infoSelector,
	                         infoDataPtr,
	                         dataSize);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaRefConSetProperty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long refCon;
	long propertyType;
	void * propertyValue;
#ifndef MediaRefConSetProperty
	PyMac_PRECHECK(MediaRefConSetProperty);
#endif
	if (!PyArg_ParseTuple(_args, "O&lls",
	                      CmpInstObj_Convert, &mh,
	                      &refCon,
	                      &propertyType,
	                      &propertyValue))
		return NULL;
	_rv = MediaRefConSetProperty(mh,
	                             refCon,
	                             propertyType,
	                             propertyValue);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaRefConGetProperty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long refCon;
	long propertyType;
	void * propertyValue;
#ifndef MediaRefConGetProperty
	PyMac_PRECHECK(MediaRefConGetProperty);
#endif
	if (!PyArg_ParseTuple(_args, "O&lls",
	                      CmpInstObj_Convert, &mh,
	                      &refCon,
	                      &propertyType,
	                      &propertyValue))
		return NULL;
	_rv = MediaRefConGetProperty(mh,
	                             refCon,
	                             propertyType,
	                             propertyValue);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MediaNavigateTargetRefCon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long navigation;
	long refCon;
#ifndef MediaNavigateTargetRefCon
	PyMac_PRECHECK(MediaNavigateTargetRefCon);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &navigation))
		return NULL;
	_rv = MediaNavigateTargetRefCon(mh,
	                                navigation,
	                                &refCon);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     refCon);
	return _res;
}

static PyObject *Qt_MediaGGetIdleManager(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	IdleManager pim;
#ifndef MediaGGetIdleManager
	PyMac_PRECHECK(MediaGGetIdleManager);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mh))
		return NULL;
	_rv = MediaGGetIdleManager(mh,
	                           &pim);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     IdleManagerObj_New, pim);
	return _res;
}

static PyObject *Qt_MediaGSetIdleManager(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	IdleManager im;
#ifndef MediaGSetIdleManager
	PyMac_PRECHECK(MediaGSetIdleManager);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mh,
	                      IdleManagerObj_Convert, &im))
		return NULL;
	_rv = MediaGSetIdleManager(mh,
	                           im);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTMIDIGetMIDIPorts(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTMIDIComponent ci;
	QTMIDIPortListHandle inputPorts;
	QTMIDIPortListHandle outputPorts;
#ifndef QTMIDIGetMIDIPorts
	PyMac_PRECHECK(QTMIDIGetMIDIPorts);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &ci))
		return NULL;
	_rv = QTMIDIGetMIDIPorts(ci,
	                         &inputPorts,
	                         &outputPorts);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, inputPorts,
	                     ResObj_New, outputPorts);
	return _res;
}

static PyObject *Qt_QTMIDIUseSendPort(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTMIDIComponent ci;
	long portIndex;
	long inUse;
#ifndef QTMIDIUseSendPort
	PyMac_PRECHECK(QTMIDIUseSendPort);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &ci,
	                      &portIndex,
	                      &inUse))
		return NULL;
	_rv = QTMIDIUseSendPort(ci,
	                        portIndex,
	                        inUse);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_QTMIDISendMIDI(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	QTMIDIComponent ci;
	long portIndex;
	MusicMIDIPacket mp;
#ifndef QTMIDISendMIDI
	PyMac_PRECHECK(QTMIDISendMIDI);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      CmpInstObj_Convert, &ci,
	                      &portIndex,
	                      QtMusicMIDIPacket_Convert, &mp))
		return NULL;
	_rv = QTMIDISendMIDI(ci,
	                     portIndex,
	                     &mp);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGetPart(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	long midiChannel;
	long polyphony;
#ifndef MusicGetPart
	PyMac_PRECHECK(MusicGetPart);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mc,
	                      &part))
		return NULL;
	_rv = MusicGetPart(mc,
	                   part,
	                   &midiChannel,
	                   &polyphony);
	_res = Py_BuildValue("lll",
	                     _rv,
	                     midiChannel,
	                     polyphony);
	return _res;
}

static PyObject *Qt_MusicSetPart(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	long midiChannel;
	long polyphony;
#ifndef MusicSetPart
	PyMac_PRECHECK(MusicSetPart);
#endif
	if (!PyArg_ParseTuple(_args, "O&lll",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &midiChannel,
	                      &polyphony))
		return NULL;
	_rv = MusicSetPart(mc,
	                   part,
	                   midiChannel,
	                   polyphony);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicSetPartInstrumentNumber(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	long instrumentNumber;
#ifndef MusicSetPartInstrumentNumber
	PyMac_PRECHECK(MusicSetPartInstrumentNumber);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &instrumentNumber))
		return NULL;
	_rv = MusicSetPartInstrumentNumber(mc,
	                                   part,
	                                   instrumentNumber);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGetPartInstrumentNumber(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
#ifndef MusicGetPartInstrumentNumber
	PyMac_PRECHECK(MusicGetPartInstrumentNumber);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mc,
	                      &part))
		return NULL;
	_rv = MusicGetPartInstrumentNumber(mc,
	                                   part);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicStorePartInstrument(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	long instrumentNumber;
#ifndef MusicStorePartInstrument
	PyMac_PRECHECK(MusicStorePartInstrument);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &instrumentNumber))
		return NULL;
	_rv = MusicStorePartInstrument(mc,
	                               part,
	                               instrumentNumber);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGetPartAtomicInstrument(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	AtomicInstrument ai;
	long flags;
#ifndef MusicGetPartAtomicInstrument
	PyMac_PRECHECK(MusicGetPartAtomicInstrument);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &flags))
		return NULL;
	_rv = MusicGetPartAtomicInstrument(mc,
	                                   part,
	                                   &ai,
	                                   flags);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, ai);
	return _res;
}

static PyObject *Qt_MusicSetPartAtomicInstrument(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	AtomicInstrumentPtr aiP;
	long flags;
#ifndef MusicSetPartAtomicInstrument
	PyMac_PRECHECK(MusicSetPartAtomicInstrument);
#endif
	if (!PyArg_ParseTuple(_args, "O&lsl",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &aiP,
	                      &flags))
		return NULL;
	_rv = MusicSetPartAtomicInstrument(mc,
	                                   part,
	                                   aiP,
	                                   flags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGetPartKnob(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	long knobID;
#ifndef MusicGetPartKnob
	PyMac_PRECHECK(MusicGetPartKnob);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &knobID))
		return NULL;
	_rv = MusicGetPartKnob(mc,
	                       part,
	                       knobID);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicSetPartKnob(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	long knobID;
	long knobValue;
#ifndef MusicSetPartKnob
	PyMac_PRECHECK(MusicSetPartKnob);
#endif
	if (!PyArg_ParseTuple(_args, "O&lll",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &knobID,
	                      &knobValue))
		return NULL;
	_rv = MusicSetPartKnob(mc,
	                       part,
	                       knobID,
	                       knobValue);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGetKnob(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long knobID;
#ifndef MusicGetKnob
	PyMac_PRECHECK(MusicGetKnob);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mc,
	                      &knobID))
		return NULL;
	_rv = MusicGetKnob(mc,
	                   knobID);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicSetKnob(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long knobID;
	long knobValue;
#ifndef MusicSetKnob
	PyMac_PRECHECK(MusicSetKnob);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mc,
	                      &knobID,
	                      &knobValue))
		return NULL;
	_rv = MusicSetKnob(mc,
	                   knobID,
	                   knobValue);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGetPartName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	StringPtr name;
#ifndef MusicGetPartName
	PyMac_PRECHECK(MusicGetPartName);
#endif
	if (!PyArg_ParseTuple(_args, "O&ls",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &name))
		return NULL;
	_rv = MusicGetPartName(mc,
	                       part,
	                       name);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicSetPartName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	StringPtr name;
#ifndef MusicSetPartName
	PyMac_PRECHECK(MusicSetPartName);
#endif
	if (!PyArg_ParseTuple(_args, "O&ls",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &name))
		return NULL;
	_rv = MusicSetPartName(mc,
	                       part,
	                       name);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicPlayNote(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	long pitch;
	long velocity;
#ifndef MusicPlayNote
	PyMac_PRECHECK(MusicPlayNote);
#endif
	if (!PyArg_ParseTuple(_args, "O&lll",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &pitch,
	                      &velocity))
		return NULL;
	_rv = MusicPlayNote(mc,
	                    part,
	                    pitch,
	                    velocity);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicResetPart(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
#ifndef MusicResetPart
	PyMac_PRECHECK(MusicResetPart);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mc,
	                      &part))
		return NULL;
	_rv = MusicResetPart(mc,
	                     part);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicSetPartController(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	MusicController controllerNumber;
	long controllerValue;
#ifndef MusicSetPartController
	PyMac_PRECHECK(MusicSetPartController);
#endif
	if (!PyArg_ParseTuple(_args, "O&lll",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &controllerNumber,
	                      &controllerValue))
		return NULL;
	_rv = MusicSetPartController(mc,
	                             part,
	                             controllerNumber,
	                             controllerValue);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGetPartController(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	MusicController controllerNumber;
#ifndef MusicGetPartController
	PyMac_PRECHECK(MusicGetPartController);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &controllerNumber))
		return NULL;
	_rv = MusicGetPartController(mc,
	                             part,
	                             controllerNumber);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGetInstrumentNames(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long modifiableInstruments;
	Handle instrumentNames;
	Handle instrumentCategoryLasts;
	Handle instrumentCategoryNames;
#ifndef MusicGetInstrumentNames
	PyMac_PRECHECK(MusicGetInstrumentNames);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mc,
	                      &modifiableInstruments))
		return NULL;
	_rv = MusicGetInstrumentNames(mc,
	                              modifiableInstruments,
	                              &instrumentNames,
	                              &instrumentCategoryLasts,
	                              &instrumentCategoryNames);
	_res = Py_BuildValue("lO&O&O&",
	                     _rv,
	                     ResObj_New, instrumentNames,
	                     ResObj_New, instrumentCategoryLasts,
	                     ResObj_New, instrumentCategoryNames);
	return _res;
}

static PyObject *Qt_MusicGetDrumNames(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long modifiableInstruments;
	Handle instrumentNumbers;
	Handle instrumentNames;
#ifndef MusicGetDrumNames
	PyMac_PRECHECK(MusicGetDrumNames);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mc,
	                      &modifiableInstruments))
		return NULL;
	_rv = MusicGetDrumNames(mc,
	                        modifiableInstruments,
	                        &instrumentNumbers,
	                        &instrumentNames);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, instrumentNumbers,
	                     ResObj_New, instrumentNames);
	return _res;
}

static PyObject *Qt_MusicGetMasterTune(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
#ifndef MusicGetMasterTune
	PyMac_PRECHECK(MusicGetMasterTune);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mc))
		return NULL;
	_rv = MusicGetMasterTune(mc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicSetMasterTune(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long masterTune;
#ifndef MusicSetMasterTune
	PyMac_PRECHECK(MusicSetMasterTune);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mc,
	                      &masterTune))
		return NULL;
	_rv = MusicSetMasterTune(mc,
	                         masterTune);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGetDeviceConnection(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long index;
	long id1;
	long id2;
#ifndef MusicGetDeviceConnection
	PyMac_PRECHECK(MusicGetDeviceConnection);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mc,
	                      &index))
		return NULL;
	_rv = MusicGetDeviceConnection(mc,
	                               index,
	                               &id1,
	                               &id2);
	_res = Py_BuildValue("lll",
	                     _rv,
	                     id1,
	                     id2);
	return _res;
}

static PyObject *Qt_MusicUseDeviceConnection(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long id1;
	long id2;
#ifndef MusicUseDeviceConnection
	PyMac_PRECHECK(MusicUseDeviceConnection);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mc,
	                      &id1,
	                      &id2))
		return NULL;
	_rv = MusicUseDeviceConnection(mc,
	                               id1,
	                               id2);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGetKnobSettingStrings(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long knobIndex;
	long isGlobal;
	Handle settingsNames;
	Handle settingsCategoryLasts;
	Handle settingsCategoryNames;
#ifndef MusicGetKnobSettingStrings
	PyMac_PRECHECK(MusicGetKnobSettingStrings);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mc,
	                      &knobIndex,
	                      &isGlobal))
		return NULL;
	_rv = MusicGetKnobSettingStrings(mc,
	                                 knobIndex,
	                                 isGlobal,
	                                 &settingsNames,
	                                 &settingsCategoryLasts,
	                                 &settingsCategoryNames);
	_res = Py_BuildValue("lO&O&O&",
	                     _rv,
	                     ResObj_New, settingsNames,
	                     ResObj_New, settingsCategoryLasts,
	                     ResObj_New, settingsCategoryNames);
	return _res;
}

static PyObject *Qt_MusicGetMIDIPorts(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long inputPortCount;
	long outputPortCount;
#ifndef MusicGetMIDIPorts
	PyMac_PRECHECK(MusicGetMIDIPorts);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mc))
		return NULL;
	_rv = MusicGetMIDIPorts(mc,
	                        &inputPortCount,
	                        &outputPortCount);
	_res = Py_BuildValue("lll",
	                     _rv,
	                     inputPortCount,
	                     outputPortCount);
	return _res;
}

static PyObject *Qt_MusicSendMIDI(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long portIndex;
	MusicMIDIPacket mp;
#ifndef MusicSendMIDI
	PyMac_PRECHECK(MusicSendMIDI);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      CmpInstObj_Convert, &mc,
	                      &portIndex,
	                      QtMusicMIDIPacket_Convert, &mp))
		return NULL;
	_rv = MusicSendMIDI(mc,
	                    portIndex,
	                    &mp);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicSetOfflineTimeTo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long newTimeStamp;
#ifndef MusicSetOfflineTimeTo
	PyMac_PRECHECK(MusicSetOfflineTimeTo);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mc,
	                      &newTimeStamp))
		return NULL;
	_rv = MusicSetOfflineTimeTo(mc,
	                            newTimeStamp);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGetInfoText(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long selector;
	Handle textH;
	Handle styleH;
#ifndef MusicGetInfoText
	PyMac_PRECHECK(MusicGetInfoText);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mc,
	                      &selector))
		return NULL;
	_rv = MusicGetInfoText(mc,
	                       selector,
	                       &textH,
	                       &styleH);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, textH,
	                     ResObj_New, styleH);
	return _res;
}

static PyObject *Qt_MusicGetInstrumentInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long getInstrumentInfoFlags;
	InstrumentInfoListHandle infoListH;
#ifndef MusicGetInstrumentInfo
	PyMac_PRECHECK(MusicGetInstrumentInfo);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mc,
	                      &getInstrumentInfoFlags))
		return NULL;
	_rv = MusicGetInstrumentInfo(mc,
	                             getInstrumentInfoFlags,
	                             &infoListH);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, infoListH);
	return _res;
}

static PyObject *Qt_MusicTask(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
#ifndef MusicTask
	PyMac_PRECHECK(MusicTask);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mc))
		return NULL;
	_rv = MusicTask(mc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicSetPartInstrumentNumberInterruptSafe(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	long instrumentNumber;
#ifndef MusicSetPartInstrumentNumberInterruptSafe
	PyMac_PRECHECK(MusicSetPartInstrumentNumberInterruptSafe);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      &instrumentNumber))
		return NULL;
	_rv = MusicSetPartInstrumentNumberInterruptSafe(mc,
	                                                part,
	                                                instrumentNumber);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicSetPartSoundLocalization(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long part;
	Handle data;
#ifndef MusicSetPartSoundLocalization
	PyMac_PRECHECK(MusicSetPartSoundLocalization);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&",
	                      CmpInstObj_Convert, &mc,
	                      &part,
	                      ResObj_Convert, &data))
		return NULL;
	_rv = MusicSetPartSoundLocalization(mc,
	                                    part,
	                                    data);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGenericConfigure(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long mode;
	long flags;
	long baseResID;
#ifndef MusicGenericConfigure
	PyMac_PRECHECK(MusicGenericConfigure);
#endif
	if (!PyArg_ParseTuple(_args, "O&lll",
	                      CmpInstObj_Convert, &mc,
	                      &mode,
	                      &flags,
	                      &baseResID))
		return NULL;
	_rv = MusicGenericConfigure(mc,
	                            mode,
	                            flags,
	                            baseResID);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicGenericGetKnobList(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	long knobType;
	GenericKnobDescriptionListHandle gkdlH;
#ifndef MusicGenericGetKnobList
	PyMac_PRECHECK(MusicGenericGetKnobList);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mc,
	                      &knobType))
		return NULL;
	_rv = MusicGenericGetKnobList(mc,
	                              knobType,
	                              &gkdlH);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     ResObj_New, gkdlH);
	return _res;
}

static PyObject *Qt_MusicGenericSetResourceNumbers(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	Handle resourceIDH;
#ifndef MusicGenericSetResourceNumbers
	PyMac_PRECHECK(MusicGenericSetResourceNumbers);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mc,
	                      ResObj_Convert, &resourceIDH))
		return NULL;
	_rv = MusicGenericSetResourceNumbers(mc,
	                                     resourceIDH);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicDerivedMIDISend(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	MusicMIDIPacket packet;
#ifndef MusicDerivedMIDISend
	PyMac_PRECHECK(MusicDerivedMIDISend);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &mc,
	                      QtMusicMIDIPacket_Convert, &packet))
		return NULL;
	_rv = MusicDerivedMIDISend(mc,
	                           &packet);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicDerivedOpenResFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
#ifndef MusicDerivedOpenResFile
	PyMac_PRECHECK(MusicDerivedOpenResFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &mc))
		return NULL;
	_rv = MusicDerivedOpenResFile(mc);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_MusicDerivedCloseResFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MusicComponent mc;
	short resRefNum;
#ifndef MusicDerivedCloseResFile
	PyMac_PRECHECK(MusicDerivedCloseResFile);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      CmpInstObj_Convert, &mc,
	                      &resRefNum))
		return NULL;
	_rv = MusicDerivedCloseResFile(mc,
	                               resRefNum);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_NAUnregisterMusicDevice(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	NoteAllocator na;
	long index;
#ifndef NAUnregisterMusicDevice
	PyMac_PRECHECK(NAUnregisterMusicDevice);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &na,
	                      &index))
		return NULL;
	_rv = NAUnregisterMusicDevice(na,
	                              index);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_NASaveMusicConfiguration(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	NoteAllocator na;
#ifndef NASaveMusicConfiguration
	PyMac_PRECHECK(NASaveMusicConfiguration);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &na))
		return NULL;
	_rv = NASaveMusicConfiguration(na);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_NAGetMIDIPorts(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	NoteAllocator na;
	QTMIDIPortListHandle inputPorts;
	QTMIDIPortListHandle outputPorts;
#ifndef NAGetMIDIPorts
	PyMac_PRECHECK(NAGetMIDIPorts);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &na))
		return NULL;
	_rv = NAGetMIDIPorts(na,
	                     &inputPorts,
	                     &outputPorts);
	_res = Py_BuildValue("lO&O&",
	                     _rv,
	                     ResObj_New, inputPorts,
	                     ResObj_New, outputPorts);
	return _res;
}

static PyObject *Qt_NATask(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	NoteAllocator na;
#ifndef NATask
	PyMac_PRECHECK(NATask);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &na))
		return NULL;
	_rv = NATask(na);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneSetHeader(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	unsigned long * header;
#ifndef TuneSetHeader
	PyMac_PRECHECK(TuneSetHeader);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &tp,
	                      &header))
		return NULL;
	_rv = TuneSetHeader(tp,
	                    header);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneGetTimeBase(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	TimeBase tb;
#ifndef TuneGetTimeBase
	PyMac_PRECHECK(TuneGetTimeBase);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &tp))
		return NULL;
	_rv = TuneGetTimeBase(tp,
	                      &tb);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     TimeBaseObj_New, tb);
	return _res;
}

static PyObject *Qt_TuneSetTimeScale(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	TimeScale scale;
#ifndef TuneSetTimeScale
	PyMac_PRECHECK(TuneSetTimeScale);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &tp,
	                      &scale))
		return NULL;
	_rv = TuneSetTimeScale(tp,
	                       scale);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneGetTimeScale(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	TimeScale scale;
#ifndef TuneGetTimeScale
	PyMac_PRECHECK(TuneGetTimeScale);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &tp))
		return NULL;
	_rv = TuneGetTimeScale(tp,
	                       &scale);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     scale);
	return _res;
}

static PyObject *Qt_TuneInstant(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	unsigned long tune;
	unsigned long tunePosition;
#ifndef TuneInstant
	PyMac_PRECHECK(TuneInstant);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &tp,
	                      &tunePosition))
		return NULL;
	_rv = TuneInstant(tp,
	                  &tune,
	                  tunePosition);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     tune);
	return _res;
}

static PyObject *Qt_TuneStop(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	long stopFlags;
#ifndef TuneStop
	PyMac_PRECHECK(TuneStop);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &tp,
	                      &stopFlags))
		return NULL;
	_rv = TuneStop(tp,
	               stopFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneSetVolume(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	Fixed volume;
#ifndef TuneSetVolume
	PyMac_PRECHECK(TuneSetVolume);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &tp,
	                      PyMac_GetFixed, &volume))
		return NULL;
	_rv = TuneSetVolume(tp,
	                    volume);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneGetVolume(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
#ifndef TuneGetVolume
	PyMac_PRECHECK(TuneGetVolume);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &tp))
		return NULL;
	_rv = TuneGetVolume(tp);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TunePreroll(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
#ifndef TunePreroll
	PyMac_PRECHECK(TunePreroll);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &tp))
		return NULL;
	_rv = TunePreroll(tp);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneUnroll(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
#ifndef TuneUnroll
	PyMac_PRECHECK(TuneUnroll);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &tp))
		return NULL;
	_rv = TuneUnroll(tp);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneSetPartTranspose(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	unsigned long part;
	long transpose;
	long velocityShift;
#ifndef TuneSetPartTranspose
	PyMac_PRECHECK(TuneSetPartTranspose);
#endif
	if (!PyArg_ParseTuple(_args, "O&lll",
	                      CmpInstObj_Convert, &tp,
	                      &part,
	                      &transpose,
	                      &velocityShift))
		return NULL;
	_rv = TuneSetPartTranspose(tp,
	                           part,
	                           transpose,
	                           velocityShift);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneGetNoteAllocator(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	NoteAllocator _rv;
	TunePlayer tp;
#ifndef TuneGetNoteAllocator
	PyMac_PRECHECK(TuneGetNoteAllocator);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &tp))
		return NULL;
	_rv = TuneGetNoteAllocator(tp);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, _rv);
	return _res;
}

static PyObject *Qt_TuneSetSofter(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	long softer;
#ifndef TuneSetSofter
	PyMac_PRECHECK(TuneSetSofter);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &tp,
	                      &softer))
		return NULL;
	_rv = TuneSetSofter(tp,
	                    softer);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneTask(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
#ifndef TuneTask
	PyMac_PRECHECK(TuneTask);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &tp))
		return NULL;
	_rv = TuneTask(tp);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneSetBalance(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	long balance;
#ifndef TuneSetBalance
	PyMac_PRECHECK(TuneSetBalance);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &tp,
	                      &balance))
		return NULL;
	_rv = TuneSetBalance(tp,
	                     balance);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneSetSoundLocalization(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	Handle data;
#ifndef TuneSetSoundLocalization
	PyMac_PRECHECK(TuneSetSoundLocalization);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpInstObj_Convert, &tp,
	                      ResObj_Convert, &data))
		return NULL;
	_rv = TuneSetSoundLocalization(tp,
	                               data);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneSetHeaderWithSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	unsigned long * header;
	unsigned long size;
#ifndef TuneSetHeaderWithSize
	PyMac_PRECHECK(TuneSetHeaderWithSize);
#endif
	if (!PyArg_ParseTuple(_args, "O&sl",
	                      CmpInstObj_Convert, &tp,
	                      &header,
	                      &size))
		return NULL;
	_rv = TuneSetHeaderWithSize(tp,
	                            header,
	                            size);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneSetPartMix(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	unsigned long partNumber;
	long volume;
	long balance;
	long mixFlags;
#ifndef TuneSetPartMix
	PyMac_PRECHECK(TuneSetPartMix);
#endif
	if (!PyArg_ParseTuple(_args, "O&llll",
	                      CmpInstObj_Convert, &tp,
	                      &partNumber,
	                      &volume,
	                      &balance,
	                      &mixFlags))
		return NULL;
	_rv = TuneSetPartMix(tp,
	                     partNumber,
	                     volume,
	                     balance,
	                     mixFlags);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qt_TuneGetPartMix(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	TunePlayer tp;
	unsigned long partNumber;
	long volumeOut;
	long balanceOut;
	long mixFlagsOut;
#ifndef TuneGetPartMix
	PyMac_PRECHECK(TuneGetPartMix);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &tp,
	                      &partNumber))
		return NULL;
	_rv = TuneGetPartMix(tp,
	                     partNumber,
	                     &volumeOut,
	                     &balanceOut,
	                     &mixFlagsOut);
	_res = Py_BuildValue("llll",
	                     _rv,
	                     volumeOut,
	                     balanceOut,
	                     mixFlagsOut);
	return _res;
}

static PyObject *Qt_AlignWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr wp;
	Boolean front;
#ifndef AlignWindow
	PyMac_PRECHECK(AlignWindow);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      WinObj_Convert, &wp,
	                      &front))
		return NULL;
	AlignWindow(wp,
	            front,
	            (Rect *)0,
	            (ICMAlignmentProcRecordPtr)0);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_DragAlignedWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr wp;
	Point startPt;
	Rect boundsRect;
#ifndef DragAlignedWindow
	PyMac_PRECHECK(DragAlignedWindow);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      WinObj_Convert, &wp,
	                      PyMac_GetPoint, &startPt,
	                      PyMac_GetRect, &boundsRect))
		return NULL;
	DragAlignedWindow(wp,
	                  startPt,
	                  &boundsRect,
	                  (Rect *)0,
	                  (ICMAlignmentProcRecordPtr)0);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qt_MoviesTask(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long maxMilliSecToUse;
#ifndef MoviesTask
	PyMac_PRECHECK(MoviesTask);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &maxMilliSecToUse))
		return NULL;
	MoviesTask((Movie)0,
	           maxMilliSecToUse);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif /* __LP64__ */

static PyMethodDef Qt_methods[] = {
#ifndef __LP64__
	{"EnterMovies", (PyCFunction)Qt_EnterMovies, 1,
	 PyDoc_STR("() -> None")},
	{"ExitMovies", (PyCFunction)Qt_ExitMovies, 1,
	 PyDoc_STR("() -> None")},
	{"GetMoviesError", (PyCFunction)Qt_GetMoviesError, 1,
	 PyDoc_STR("() -> None")},
	{"ClearMoviesStickyError", (PyCFunction)Qt_ClearMoviesStickyError, 1,
	 PyDoc_STR("() -> None")},
	{"GetMoviesStickyError", (PyCFunction)Qt_GetMoviesStickyError, 1,
	 PyDoc_STR("() -> None")},
	{"QTGetWallClockTimeBase", (PyCFunction)Qt_QTGetWallClockTimeBase, 1,
	 PyDoc_STR("() -> (TimeBase wallClockTimeBase)")},
	{"QTIdleManagerOpen", (PyCFunction)Qt_QTIdleManagerOpen, 1,
	 PyDoc_STR("() -> (IdleManager _rv)")},
	{"CreateMovieControl", (PyCFunction)Qt_CreateMovieControl, 1,
	 PyDoc_STR("(WindowPtr theWindow, Movie theMovie, UInt32 options) -> (Rect localRect, ControlHandle returnedControl)")},
	{"DisposeMatte", (PyCFunction)Qt_DisposeMatte, 1,
	 PyDoc_STR("(PixMapHandle theMatte) -> None")},
	{"NewMovie", (PyCFunction)Qt_NewMovie, 1,
	 PyDoc_STR("(long flags) -> (Movie _rv)")},
	{"QTGetTimeUntilNextTask", (PyCFunction)Qt_QTGetTimeUntilNextTask, 1,
	 PyDoc_STR("(long scale) -> (long duration)")},
	{"GetDataHandler", (PyCFunction)Qt_GetDataHandler, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataHandlerSubType, long flags) -> (Component _rv)")},
	{"PasteHandleIntoMovie", (PyCFunction)Qt_PasteHandleIntoMovie, 1,
	 PyDoc_STR("(Handle h, OSType handleType, Movie theMovie, long flags, ComponentInstance userComp) -> None")},
	{"GetMovieImporterForDataRef", (PyCFunction)Qt_GetMovieImporterForDataRef, 1,
	 PyDoc_STR("(OSType dataRefType, Handle dataRef, long flags) -> (Component importer)")},
	{"QTGetMIMETypeInfo", (PyCFunction)Qt_QTGetMIMETypeInfo, 1,
	 PyDoc_STR("(char* mimeStringStart, short mimeStringLength, OSType infoSelector, void * infoDataPtr) -> (long infoDataSize)")},
	{"TrackTimeToMediaTime", (PyCFunction)Qt_TrackTimeToMediaTime, 1,
	 PyDoc_STR("(TimeValue value, Track theTrack) -> (TimeValue _rv)")},
	{"NewUserData", (PyCFunction)Qt_NewUserData, 1,
	 PyDoc_STR("() -> (UserData theUserData)")},
	{"NewUserDataFromHandle", (PyCFunction)Qt_NewUserDataFromHandle, 1,
	 PyDoc_STR("(Handle h) -> (UserData theUserData)")},
	{"CreateMovieFile", (PyCFunction)Qt_CreateMovieFile, 1,
	 PyDoc_STR("(FSSpec fileSpec, OSType creator, ScriptCode scriptTag, long createMovieFileFlags) -> (short resRefNum, Movie newmovie)")},
	{"OpenMovieFile", (PyCFunction)Qt_OpenMovieFile, 1,
	 PyDoc_STR("(FSSpec fileSpec, SInt8 permission) -> (short resRefNum)")},
	{"CloseMovieFile", (PyCFunction)Qt_CloseMovieFile, 1,
	 PyDoc_STR("(short resRefNum) -> None")},
	{"DeleteMovieFile", (PyCFunction)Qt_DeleteMovieFile, 1,
	 PyDoc_STR("(FSSpec fileSpec) -> None")},
	{"NewMovieFromFile", (PyCFunction)Qt_NewMovieFromFile, 1,
	 PyDoc_STR("(short resRefNum, short resId, short newMovieFlags) -> (Movie theMovie, short resId, Boolean dataRefWasChanged)")},
	{"NewMovieFromHandle", (PyCFunction)Qt_NewMovieFromHandle, 1,
	 PyDoc_STR("(Handle h, short newMovieFlags) -> (Movie theMovie, Boolean dataRefWasChanged)")},
	{"NewMovieFromDataFork", (PyCFunction)Qt_NewMovieFromDataFork, 1,
	 PyDoc_STR("(short fRefNum, long fileOffset, short newMovieFlags) -> (Movie theMovie, Boolean dataRefWasChanged)")},
	{"NewMovieFromDataFork64", (PyCFunction)Qt_NewMovieFromDataFork64, 1,
	 PyDoc_STR("(long fRefNum, wide fileOffset, short newMovieFlags) -> (Movie theMovie, Boolean dataRefWasChanged)")},
	{"NewMovieFromDataRef", (PyCFunction)Qt_NewMovieFromDataRef, 1,
	 PyDoc_STR("(short flags, Handle dataRef, OSType dtaRefType) -> (Movie m, short id)")},
	{"NewMovieFromStorageOffset", (PyCFunction)Qt_NewMovieFromStorageOffset, 1,
	 PyDoc_STR("(DataHandler dh, wide fileOffset, short newMovieFlags) -> (Movie theMovie, Boolean dataRefWasCataRefType)")},
	{"NewMovieForDataRefFromHandle", (PyCFunction)Qt_NewMovieForDataRefFromHandle, 1,
	 PyDoc_STR("(Handle h, short newMovieFlags, Handle dataRef, OSType dataRefType) -> (Movie theMovie, Boolean dataRefWasChanged)")},
	{"RemoveMovieResource", (PyCFunction)Qt_RemoveMovieResource, 1,
	 PyDoc_STR("(short resRefNum, short resId) -> None")},
	{"CreateMovieStorage", (PyCFunction)Qt_CreateMovieStorage, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataRefType, OSType creator, ScriptCode scriptTag, long createMovieFileFlags) -> (DataHandler outDataHandler, Movie newmovie)")},
	{"OpenMovieStorage", (PyCFunction)Qt_OpenMovieStorage, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataRefType, long flags) -> (DataHandler outDataHandler)")},
	{"CloseMovieStorage", (PyCFunction)Qt_CloseMovieStorage, 1,
	 PyDoc_STR("(DataHandler dh) -> None")},
	{"DeleteMovieStorage", (PyCFunction)Qt_DeleteMovieStorage, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataRefType) -> None")},
	{"CreateShortcutMovieFile", (PyCFunction)Qt_CreateShortcutMovieFile, 1,
	 PyDoc_STR("(FSSpec fileSpec, OSType creator, ScriptCode scriptTag, long createMovieFileFlags, Handle targetDataRef, OSType targetDataRefType) -> None")},
	{"CanQuickTimeOpenFile", (PyCFunction)Qt_CanQuickTimeOpenFile, 1,
	 PyDoc_STR("(FSSpec fileSpec, OSType fileType, OSType fileNameExtension, UInt32 inFlags) -> (Boolean outCanOpenWithGraphicsImporter, Boolean outCanOpenAsMovie, Boolean outPreferGraphicsImporter)")},
	{"CanQuickTimeOpenDataRef", (PyCFunction)Qt_CanQuickTimeOpenDataRef, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataRefType, UInt32 inFlags) -> (Boolean outCanOpenWithGraphicsImporter, Boolean outCanOpenAsMovie, Boolean outPreferGraphicsImporter)")},
	{"NewMovieFromScrap", (PyCFunction)Qt_NewMovieFromScrap, 1,
	 PyDoc_STR("(long newMovieFlags) -> (Movie _rv)")},
	{"QTNewAlias", (PyCFunction)Qt_QTNewAlias, 1,
	 PyDoc_STR("(FSSpec fss, Boolean minimal) -> (AliasHandle alias)")},
	{"EndFullScreen", (PyCFunction)Qt_EndFullScreen, 1,
	 PyDoc_STR("(Ptr fullState, long flags) -> None")},
	{"AddSoundDescriptionExtension", (PyCFunction)Qt_AddSoundDescriptionExtension, 1,
	 PyDoc_STR("(SoundDescriptionHandle desc, Handle extension, OSType idType) -> None")},
	{"GetSoundDescriptionExtension", (PyCFunction)Qt_GetSoundDescriptionExtension, 1,
	 PyDoc_STR("(SoundDescriptionHandle desc, OSType idType) -> (Handle extension)")},
	{"RemoveSoundDescriptionExtension", (PyCFunction)Qt_RemoveSoundDescriptionExtension, 1,
	 PyDoc_STR("(SoundDescriptionHandle desc, OSType idType) -> None")},
	{"QTIsStandardParameterDialogEvent", (PyCFunction)Qt_QTIsStandardParameterDialogEvent, 1,
	 PyDoc_STR("(QTParameterDialog createdDialog) -> (EventRecord pEvent)")},
	{"QTDismissStandardParameterDialog", (PyCFunction)Qt_QTDismissStandardParameterDialog, 1,
	 PyDoc_STR("(QTParameterDialog createdDialog) -> None")},
	{"QTStandardParameterDialogDoAction", (PyCFunction)Qt_QTStandardParameterDialogDoAction, 1,
	 PyDoc_STR("(QTParameterDialog createdDialog, long action, void * params) -> None")},
	{"QTRegisterAccessKey", (PyCFunction)Qt_QTRegisterAccessKey, 1,
	 PyDoc_STR("(Str255 accessKeyType, long flags, Handle accessKey) -> None")},
	{"QTUnregisterAccessKey", (PyCFunction)Qt_QTUnregisterAccessKey, 1,
	 PyDoc_STR("(Str255 accessKeyType, long flags, Handle accessKey) -> None")},
	{"QTGetSupportedRestrictions", (PyCFunction)Qt_QTGetSupportedRestrictions, 1,
	 PyDoc_STR("(OSType inRestrictionClass) -> (UInt32 outRestrictionIDs)")},
	{"QTTextToNativeText", (PyCFunction)Qt_QTTextToNativeText, 1,
	 PyDoc_STR("(Handle theText, long encoding, long flags) -> None")},
	{"VideoMediaResetStatistics", (PyCFunction)Qt_VideoMediaResetStatistics, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv)")},
	{"VideoMediaGetStatistics", (PyCFunction)Qt_VideoMediaGetStatistics, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv)")},
	{"VideoMediaGetStallCount", (PyCFunction)Qt_VideoMediaGetStallCount, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, unsigned long stalls)")},
	{"VideoMediaSetCodecParameter", (PyCFunction)Qt_VideoMediaSetCodecParameter, 1,
	 PyDoc_STR("(MediaHandler mh, CodecType cType, OSType parameterID, long parameterChangeSeed, void * dataPtr, long dataSize) -> (ComponentResult _rv)")},
	{"VideoMediaGetCodecParameter", (PyCFunction)Qt_VideoMediaGetCodecParameter, 1,
	 PyDoc_STR("(MediaHandler mh, CodecType cType, OSType parameterID, Handle outParameterData) -> (ComponentResult _rv)")},
	{"TextMediaAddTextSample", (PyCFunction)Qt_TextMediaAddTextSample, 1,
	 PyDoc_STR("(MediaHandler mh, Ptr text, unsigned long size, short fontNumber, short fontSize, Style textFace, short textJustification, long displayFlags, TimeValue scrollDelay, short hiliteStart, short hiliteEnd, TimeValue duration) -> (ComponentResult _rv, RGBColor textColor, RGBColor backColor, Rect textBox, RGBColor rgbHiliteColor, TimeValue sampleTime)")},
	{"TextMediaAddTESample", (PyCFunction)Qt_TextMediaAddTESample, 1,
	 PyDoc_STR("(MediaHandler mh, TEHandle hTE, short textJustification, long displayFlags, TimeValue scrollDelay, short hiliteStart, short hiliteEnd, TimeValue duration) -> (ComponentResult _rv, RGBColor backColor, Rect textBox, RGBColor rgbHiliteColor, TimeValue sampleTime)")},
	{"TextMediaAddHiliteSample", (PyCFunction)Qt_TextMediaAddHiliteSample, 1,
	 PyDoc_STR("(MediaHandler mh, short hiliteStart, short hiliteEnd, TimeValue duration) -> (ComponentResult _rv, RGBColor rgbHiliteColor, TimeValue sampleTime)")},
	{"TextMediaDrawRaw", (PyCFunction)Qt_TextMediaDrawRaw, 1,
	 PyDoc_STR("(MediaHandler mh, GWorldPtr gw, GDHandle gd, void * data, long dataSize, TextDescriptionHandle tdh) -> (ComponentResult _rv)")},
	{"TextMediaSetTextProperty", (PyCFunction)Qt_TextMediaSetTextProperty, 1,
	 PyDoc_STR("(MediaHandler mh, TimeValue atMediaTime, long propertyType, void * data, long dataSize) -> (ComponentResult _rv)")},
	{"TextMediaRawSetup", (PyCFunction)Qt_TextMediaRawSetup, 1,
	 PyDoc_STR("(MediaHandler mh, GWorldPtr gw, GDHandle gd, void * data, long dataSize, TextDescriptionHandle tdh, TimeValue sampleDuration) -> (ComponentResult _rv)")},
	{"TextMediaRawIdle", (PyCFunction)Qt_TextMediaRawIdle, 1,
	 PyDoc_STR("(MediaHandler mh, GWorldPtr gw, GDHandle gd, TimeValue sampleTime, long flagsIn) -> (ComponentResult _rv, long flagsOut)")},
	{"TextMediaGetTextProperty", (PyCFunction)Qt_TextMediaGetTextProperty, 1,
	 PyDoc_STR("(MediaHandler mh, TimeValue atMediaTime, long propertyType, void * data, long dataSize) -> (ComponentResult _rv)")},
	{"TextMediaFindNextText", (PyCFunction)Qt_TextMediaFindNextText, 1,
	 PyDoc_STR("(MediaHandler mh, Ptr text, long size, short findFlags, TimeValue startTime) -> (ComponentResult _rv, TimeValue foundTime, TimeValue foundDuration, long offset)")},
	{"TextMediaHiliteTextSample", (PyCFunction)Qt_TextMediaHiliteTextSample, 1,
	 PyDoc_STR("(MediaHandler mh, TimeValue sampleTime, short hiliteStart, short hiliteEnd) -> (ComponentResult _rv, RGBColor rgbHiliteColor)")},
	{"TextMediaSetTextSampleData", (PyCFunction)Qt_TextMediaSetTextSampleData, 1,
	 PyDoc_STR("(MediaHandler mh, void * data, OSType dataType) -> (ComponentResult _rv)")},
	{"SpriteMediaSetProperty", (PyCFunction)Qt_SpriteMediaSetProperty, 1,
	 PyDoc_STR("(MediaHandler mh, short spriteIndex, long propertyType, void * propertyValue) -> (ComponentResult _rv)")},
	{"SpriteMediaGetProperty", (PyCFunction)Qt_SpriteMediaGetProperty, 1,
	 PyDoc_STR("(MediaHandler mh, short spriteIndex, long propertyType, void * propertyValue) -> (ComponentResult _rv)")},
	{"SpriteMediaHitTestSprites", (PyCFunction)Qt_SpriteMediaHitTestSprites, 1,
	 PyDoc_STR("(MediaHandler mh, long flags, Point loc) -> (ComponentResult _rv, short spriteHitIndex)")},
	{"SpriteMediaCountSprites", (PyCFunction)Qt_SpriteMediaCountSprites, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, short numSprites)")},
	{"SpriteMediaCountImages", (PyCFunction)Qt_SpriteMediaCountImages, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, short numImages)")},
	{"SpriteMediaGetIndImageDescription", (PyCFunction)Qt_SpriteMediaGetIndImageDescription, 1,
	 PyDoc_STR("(MediaHandler mh, short imageIndex, ImageDescriptionHandle imageDescription) -> (ComponentResult _rv)")},
	{"SpriteMediaGetDisplayedSampleNumber", (PyCFunction)Qt_SpriteMediaGetDisplayedSampleNumber, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, long sampleNum)")},
	{"SpriteMediaGetSpriteName", (PyCFunction)Qt_SpriteMediaGetSpriteName, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID spriteID, Str255 spriteName) -> (ComponentResult _rv)")},
	{"SpriteMediaGetImageName", (PyCFunction)Qt_SpriteMediaGetImageName, 1,
	 PyDoc_STR("(MediaHandler mh, short imageIndex, Str255 imageName) -> (ComponentResult _rv)")},
	{"SpriteMediaSetSpriteProperty", (PyCFunction)Qt_SpriteMediaSetSpriteProperty, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID spriteID, long propertyType, void * propertyValue) -> (ComponentResult _rv)")},
	{"SpriteMediaGetSpriteProperty", (PyCFunction)Qt_SpriteMediaGetSpriteProperty, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID spriteID, long propertyType, void * propertyValue) -> (ComponentResult _rv)")},
	{"SpriteMediaHitTestAllSprites", (PyCFunction)Qt_SpriteMediaHitTestAllSprites, 1,
	 PyDoc_STR("(MediaHandler mh, long flags, Point loc) -> (ComponentResult _rv, QTAtomID spriteHitID)")},
	{"SpriteMediaHitTestOneSprite", (PyCFunction)Qt_SpriteMediaHitTestOneSprite, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID spriteID, long flags, Point loc) -> (ComponentResult _rv, Boolean wasHit)")},
	{"SpriteMediaSpriteIndexToID", (PyCFunction)Qt_SpriteMediaSpriteIndexToID, 1,
	 PyDoc_STR("(MediaHandler mh, short spriteIndex) -> (ComponentResult _rv, QTAtomID spriteID)")},
	{"SpriteMediaSpriteIDToIndex", (PyCFunction)Qt_SpriteMediaSpriteIDToIndex, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID spriteID) -> (ComponentResult _rv, short spriteIndex)")},
	{"SpriteMediaSetActionVariable", (PyCFunction)Qt_SpriteMediaSetActionVariable, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID variableID, float value) -> (ComponentResult _rv)")},
	{"SpriteMediaGetActionVariable", (PyCFunction)Qt_SpriteMediaGetActionVariable, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID variableID) -> (ComponentResult _rv, float value)")},
	{"SpriteMediaDisposeSprite", (PyCFunction)Qt_SpriteMediaDisposeSprite, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID spriteID) -> (ComponentResult _rv)")},
	{"SpriteMediaSetActionVariableToString", (PyCFunction)Qt_SpriteMediaSetActionVariableToString, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID variableID, Ptr theCString) -> (ComponentResult _rv)")},
	{"SpriteMediaGetActionVariableAsString", (PyCFunction)Qt_SpriteMediaGetActionVariableAsString, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID variableID) -> (ComponentResult _rv, Handle theCString)")},
	{"SpriteMediaNewImage", (PyCFunction)Qt_SpriteMediaNewImage, 1,
	 PyDoc_STR("(MediaHandler mh, Handle dataRef, OSType dataRefType, QTAtomID desiredID) -> (ComponentResult _rv)")},
	{"SpriteMediaDisposeImage", (PyCFunction)Qt_SpriteMediaDisposeImage, 1,
	 PyDoc_STR("(MediaHandler mh, short imageIndex) -> (ComponentResult _rv)")},
	{"SpriteMediaImageIndexToID", (PyCFunction)Qt_SpriteMediaImageIndexToID, 1,
	 PyDoc_STR("(MediaHandler mh, short imageIndex) -> (ComponentResult _rv, QTAtomID imageID)")},
	{"SpriteMediaImageIDToIndex", (PyCFunction)Qt_SpriteMediaImageIDToIndex, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID imageID) -> (ComponentResult _rv, short imageIndex)")},
	{"FlashMediaSetPan", (PyCFunction)Qt_FlashMediaSetPan, 1,
	 PyDoc_STR("(MediaHandler mh, short xPercent, short yPercent) -> (ComponentResult _rv)")},
	{"FlashMediaSetZoom", (PyCFunction)Qt_FlashMediaSetZoom, 1,
	 PyDoc_STR("(MediaHandler mh, short factor) -> (ComponentResult _rv)")},
	{"FlashMediaSetZoomRect", (PyCFunction)Qt_FlashMediaSetZoomRect, 1,
	 PyDoc_STR("(MediaHandler mh, long left, long top, long right, long bottom) -> (ComponentResult _rv)")},
	{"FlashMediaGetRefConBounds", (PyCFunction)Qt_FlashMediaGetRefConBounds, 1,
	 PyDoc_STR("(MediaHandler mh, long refCon) -> (ComponentResult _rv, long left, long top, long right, long bottom)")},
	{"FlashMediaGetRefConID", (PyCFunction)Qt_FlashMediaGetRefConID, 1,
	 PyDoc_STR("(MediaHandler mh, long refCon) -> (ComponentResult _rv, long refConID)")},
	{"FlashMediaIDToRefCon", (PyCFunction)Qt_FlashMediaIDToRefCon, 1,
	 PyDoc_STR("(MediaHandler mh, long refConID) -> (ComponentResult _rv, long refCon)")},
	{"FlashMediaGetDisplayedFrameNumber", (PyCFunction)Qt_FlashMediaGetDisplayedFrameNumber, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, long flashFrameNumber)")},
	{"FlashMediaFrameNumberToMovieTime", (PyCFunction)Qt_FlashMediaFrameNumberToMovieTime, 1,
	 PyDoc_STR("(MediaHandler mh, long flashFrameNumber) -> (ComponentResult _rv, TimeValue movieTime)")},
	{"FlashMediaFrameLabelToMovieTime", (PyCFunction)Qt_FlashMediaFrameLabelToMovieTime, 1,
	 PyDoc_STR("(MediaHandler mh, Ptr theLabel) -> (ComponentResult _rv, TimeValue movieTime)")},
	{"FlashMediaGetFlashVariable", (PyCFunction)Qt_FlashMediaGetFlashVariable, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, char path, char name, Handle theVariableCStringOut)")},
	{"FlashMediaSetFlashVariable", (PyCFunction)Qt_FlashMediaSetFlashVariable, 1,
	 PyDoc_STR("(MediaHandler mh, Boolean updateFocus) -> (ComponentResult _rv, char path, char name, char value)")},
	{"FlashMediaDoButtonActions", (PyCFunction)Qt_FlashMediaDoButtonActions, 1,
	 PyDoc_STR("(MediaHandler mh, long buttonID, long transition) -> (ComponentResult _rv, char path)")},
	{"FlashMediaGetSupportedSwfVersion", (PyCFunction)Qt_FlashMediaGetSupportedSwfVersion, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, UInt8 swfVersion)")},
	{"Media3DGetCurrentGroup", (PyCFunction)Qt_Media3DGetCurrentGroup, 1,
	 PyDoc_STR("(MediaHandler mh, void * group) -> (ComponentResult _rv)")},
	{"Media3DTranslateNamedObjectTo", (PyCFunction)Qt_Media3DTranslateNamedObjectTo, 1,
	 PyDoc_STR("(MediaHandler mh, Fixed x, Fixed y, Fixed z) -> (ComponentResult _rv, char objectName)")},
	{"Media3DScaleNamedObjectTo", (PyCFunction)Qt_Media3DScaleNamedObjectTo, 1,
	 PyDoc_STR("(MediaHandler mh, Fixed xScale, Fixed yScale, Fixed zScale) -> (ComponentResult _rv, char objectName)")},
	{"Media3DRotateNamedObjectTo", (PyCFunction)Qt_Media3DRotateNamedObjectTo, 1,
	 PyDoc_STR("(MediaHandler mh, Fixed xDegrees, Fixed yDegrees, Fixed zDegrees) -> (ComponentResult _rv, char objectName)")},
	{"Media3DSetCameraData", (PyCFunction)Qt_Media3DSetCameraData, 1,
	 PyDoc_STR("(MediaHandler mh, void * cameraData) -> (ComponentResult _rv)")},
	{"Media3DGetCameraData", (PyCFunction)Qt_Media3DGetCameraData, 1,
	 PyDoc_STR("(MediaHandler mh, void * cameraData) -> (ComponentResult _rv)")},
	{"Media3DSetCameraAngleAspect", (PyCFunction)Qt_Media3DSetCameraAngleAspect, 1,
	 PyDoc_STR("(MediaHandler mh, QTFloatSingle fov, QTFloatSingle aspectRatioXToY) -> (ComponentResult _rv)")},
	{"Media3DGetCameraAngleAspect", (PyCFunction)Qt_Media3DGetCameraAngleAspect, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, QTFloatSingle fov, QTFloatSingle aspectRatioXToY)")},
	{"Media3DSetCameraRange", (PyCFunction)Qt_Media3DSetCameraRange, 1,
	 PyDoc_STR("(MediaHandler mh, void * tQ3CameraRange) -> (ComponentResult _rv)")},
	{"Media3DGetCameraRange", (PyCFunction)Qt_Media3DGetCameraRange, 1,
	 PyDoc_STR("(MediaHandler mh, void * tQ3CameraRange) -> (ComponentResult _rv)")},
	{"NewTimeBase", (PyCFunction)Qt_NewTimeBase, 1,
	 PyDoc_STR("() -> (TimeBase _rv)")},
	{"ConvertTime", (PyCFunction)Qt_ConvertTime, 1,
	 PyDoc_STR("(TimeRecord theTime, TimeBase newBase) -> (TimeRecord theTime)")},
	{"ConvertTimeScale", (PyCFunction)Qt_ConvertTimeScale, 1,
	 PyDoc_STR("(TimeRecord theTime, TimeScale newScale) -> (TimeRecord theTime)")},
	{"AddTime", (PyCFunction)Qt_AddTime, 1,
	 PyDoc_STR("(TimeRecord dst, TimeRecord src) -> (TimeRecord dst)")},
	{"SubtractTime", (PyCFunction)Qt_SubtractTime, 1,
	 PyDoc_STR("(TimeRecord dst, TimeRecord src) -> (TimeRecord dst)")},
	{"MusicMediaGetIndexedTunePlayer", (PyCFunction)Qt_MusicMediaGetIndexedTunePlayer, 1,
	 PyDoc_STR("(ComponentInstance ti, long sampleDescIndex) -> (ComponentResult _rv, ComponentInstance tp)")},
	{"CodecManagerVersion", (PyCFunction)Qt_CodecManagerVersion, 1,
	 PyDoc_STR("() -> (long version)")},
	{"GetMaxCompressionSize", (PyCFunction)Qt_GetMaxCompressionSize, 1,
	 PyDoc_STR("(PixMapHandle src, Rect srcRect, short colorDepth, CodecQ quality, CodecType cType, CompressorComponent codec) -> (long size)")},
	{"GetCompressionTime", (PyCFunction)Qt_GetCompressionTime, 1,
	 PyDoc_STR("(PixMapHandle src, Rect srcRect, short colorDepth, CodecType cType, CompressorComponent codec) -> (CodecQ spatialQuality, CodecQ temporalQuality, unsigned long compressTime)")},
	{"CompressImage", (PyCFunction)Qt_CompressImage, 1,
	 PyDoc_STR("(PixMapHandle src, Rect srcRect, CodecQ quality, CodecType cType, ImageDescriptionHandle desc, Ptr data) -> None")},
	{"DecompressImage", (PyCFunction)Qt_DecompressImage, 1,
	 PyDoc_STR("(Ptr data, ImageDescriptionHandle desc, PixMapHandle dst, Rect srcRect, Rect dstRect, short mode, RgnHandle mask) -> None")},
	{"GetSimilarity", (PyCFunction)Qt_GetSimilarity, 1,
	 PyDoc_STR("(PixMapHandle src, Rect srcRect, ImageDescriptionHandle desc, Ptr data) -> (Fixed similarity)")},
	{"GetImageDescriptionCTable", (PyCFunction)Qt_GetImageDescriptionCTable, 1,
	 PyDoc_STR("(ImageDescriptionHandle desc) -> (CTabHandle ctable)")},
	{"SetImageDescriptionCTable", (PyCFunction)Qt_SetImageDescriptionCTable, 1,
	 PyDoc_STR("(ImageDescriptionHandle desc, CTabHandle ctable) -> None")},
	{"GetImageDescriptionExtension", (PyCFunction)Qt_GetImageDescriptionExtension, 1,
	 PyDoc_STR("(ImageDescriptionHandle desc, long idType, long index) -> (Handle extension)")},
	{"AddImageDescriptionExtension", (PyCFunction)Qt_AddImageDescriptionExtension, 1,
	 PyDoc_STR("(ImageDescriptionHandle desc, Handle extension, long idType) -> None")},
	{"RemoveImageDescriptionExtension", (PyCFunction)Qt_RemoveImageDescriptionExtension, 1,
	 PyDoc_STR("(ImageDescriptionHandle desc, long idType, long index) -> None")},
	{"CountImageDescriptionExtensionType", (PyCFunction)Qt_CountImageDescriptionExtensionType, 1,
	 PyDoc_STR("(ImageDescriptionHandle desc, long idType) -> (long count)")},
	{"GetNextImageDescriptionExtensionType", (PyCFunction)Qt_GetNextImageDescriptionExtensionType, 1,
	 PyDoc_STR("(ImageDescriptionHandle desc) -> (long idType)")},
	{"FindCodec", (PyCFunction)Qt_FindCodec, 1,
	 PyDoc_STR("(CodecType cType, CodecComponent specCodec) -> (CompressorComponent compressor, DecompressorComponent decompressor)")},
	{"CompressPicture", (PyCFunction)Qt_CompressPicture, 1,
	 PyDoc_STR("(PicHandle srcPicture, PicHandle dstPicture, CodecQ quality, CodecType cType) -> None")},
	{"CompressPictureFile", (PyCFunction)Qt_CompressPictureFile, 1,
	 PyDoc_STR("(short srcRefNum, short dstRefNum, CodecQ quality, CodecType cType) -> None")},
	{"ConvertImage", (PyCFunction)Qt_ConvertImage, 1,
	 PyDoc_STR("(ImageDescriptionHandle srcDD, Ptr srcData, short colorDepth, CTabHandle ctable, CodecQ accuracy, CodecQ quality, CodecType cType, CodecComponent codec, ImageDescriptionHandle dstDD, Ptr dstData) -> None")},
	{"AddFilePreview", (PyCFunction)Qt_AddFilePreview, 1,
	 PyDoc_STR("(short resRefNum, OSType previewType, Handle previewData) -> None")},
	{"GetBestDeviceRect", (PyCFunction)Qt_GetBestDeviceRect, 1,
	 PyDoc_STR("() -> (GDHandle gdh, Rect rp)")},
	{"GDHasScale", (PyCFunction)Qt_GDHasScale, 1,
	 PyDoc_STR("(GDHandle gdh, short depth) -> (Fixed scale)")},
	{"GDGetScale", (PyCFunction)Qt_GDGetScale, 1,
	 PyDoc_STR("(GDHandle gdh) -> (Fixed scale, short flags)")},
	{"GDSetScale", (PyCFunction)Qt_GDSetScale, 1,
	 PyDoc_STR("(GDHandle gdh, Fixed scale, short flags) -> None")},
	{"GetGraphicsImporterForFile", (PyCFunction)Qt_GetGraphicsImporterForFile, 1,
	 PyDoc_STR("(FSSpec theFile) -> (ComponentInstance gi)")},
	{"GetGraphicsImporterForDataRef", (PyCFunction)Qt_GetGraphicsImporterForDataRef, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataRefType) -> (ComponentInstance gi)")},
	{"GetGraphicsImporterForFileWithFlags", (PyCFunction)Qt_GetGraphicsImporterForFileWithFlags, 1,
	 PyDoc_STR("(FSSpec theFile, long flags) -> (ComponentInstance gi)")},
	{"GetGraphicsImporterForDataRefWithFlags", (PyCFunction)Qt_GetGraphicsImporterForDataRefWithFlags, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataRefType, long flags) -> (ComponentInstance gi)")},
	{"MakeImageDescriptionForPixMap", (PyCFunction)Qt_MakeImageDescriptionForPixMap, 1,
	 PyDoc_STR("(PixMapHandle pixmap) -> (ImageDescriptionHandle idh)")},
	{"MakeImageDescriptionForEffect", (PyCFunction)Qt_MakeImageDescriptionForEffect, 1,
	 PyDoc_STR("(OSType effectType) -> (ImageDescriptionHandle idh)")},
	{"QTGetPixelSize", (PyCFunction)Qt_QTGetPixelSize, 1,
	 PyDoc_STR("(OSType PixelFormat) -> (short _rv)")},
	{"QTGetPixelFormatDepthForImageDescription", (PyCFunction)Qt_QTGetPixelFormatDepthForImageDescription, 1,
	 PyDoc_STR("(OSType PixelFormat) -> (short _rv)")},
	{"QTGetPixMapHandleRowBytes", (PyCFunction)Qt_QTGetPixMapHandleRowBytes, 1,
	 PyDoc_STR("(PixMapHandle pm) -> (long _rv)")},
	{"QTSetPixMapHandleRowBytes", (PyCFunction)Qt_QTSetPixMapHandleRowBytes, 1,
	 PyDoc_STR("(PixMapHandle pm, long rowBytes) -> None")},
	{"QTGetPixMapHandleGammaLevel", (PyCFunction)Qt_QTGetPixMapHandleGammaLevel, 1,
	 PyDoc_STR("(PixMapHandle pm) -> (Fixed _rv)")},
	{"QTSetPixMapHandleGammaLevel", (PyCFunction)Qt_QTSetPixMapHandleGammaLevel, 1,
	 PyDoc_STR("(PixMapHandle pm, Fixed gammaLevel) -> None")},
	{"QTGetPixMapHandleRequestedGammaLevel", (PyCFunction)Qt_QTGetPixMapHandleRequestedGammaLevel, 1,
	 PyDoc_STR("(PixMapHandle pm) -> (Fixed _rv)")},
	{"QTSetPixMapHandleRequestedGammaLevel", (PyCFunction)Qt_QTSetPixMapHandleRequestedGammaLevel, 1,
	 PyDoc_STR("(PixMapHandle pm, Fixed requestedGammaLevel) -> None")},
	{"CompAdd", (PyCFunction)Qt_CompAdd, 1,
	 PyDoc_STR("() -> (wide src, wide dst)")},
	{"CompSub", (PyCFunction)Qt_CompSub, 1,
	 PyDoc_STR("() -> (wide src, wide dst)")},
	{"CompNeg", (PyCFunction)Qt_CompNeg, 1,
	 PyDoc_STR("() -> (wide dst)")},
	{"CompShift", (PyCFunction)Qt_CompShift, 1,
	 PyDoc_STR("(short shift) -> (wide src)")},
	{"CompMul", (PyCFunction)Qt_CompMul, 1,
	 PyDoc_STR("(long src1, long src2) -> (wide dst)")},
	{"CompDiv", (PyCFunction)Qt_CompDiv, 1,
	 PyDoc_STR("(long denominator) -> (long _rv, wide numerator, long remainder)")},
	{"CompFixMul", (PyCFunction)Qt_CompFixMul, 1,
	 PyDoc_STR("(Fixed fixSrc) -> (wide compSrc, wide compDst)")},
	{"CompMulDiv", (PyCFunction)Qt_CompMulDiv, 1,
	 PyDoc_STR("(long mul, long divisor) -> (wide co)")},
	{"CompMulDivTrunc", (PyCFunction)Qt_CompMulDivTrunc, 1,
	 PyDoc_STR("(long mul, long divisor) -> (wide co, long remainder)")},
	{"CompCompare", (PyCFunction)Qt_CompCompare, 1,
	 PyDoc_STR("(wide a, wide minusb) -> (long _rv)")},
	{"CompSquareRoot", (PyCFunction)Qt_CompSquareRoot, 1,
	 PyDoc_STR("(wide src) -> (unsigned long _rv)")},
	{"FixMulDiv", (PyCFunction)Qt_FixMulDiv, 1,
	 PyDoc_STR("(Fixed src, Fixed mul, Fixed divisor) -> (Fixed _rv)")},
	{"UnsignedFixMulDiv", (PyCFunction)Qt_UnsignedFixMulDiv, 1,
	 PyDoc_STR("(Fixed src, Fixed mul, Fixed divisor) -> (Fixed _rv)")},
	{"FixExp2", (PyCFunction)Qt_FixExp2, 1,
	 PyDoc_STR("(Fixed src) -> (Fixed _rv)")},
	{"FixLog2", (PyCFunction)Qt_FixLog2, 1,
	 PyDoc_STR("(Fixed src) -> (Fixed _rv)")},
	{"FixPow", (PyCFunction)Qt_FixPow, 1,
	 PyDoc_STR("(Fixed base, Fixed exp) -> (Fixed _rv)")},
	{"GraphicsImportSetDataReference", (PyCFunction)Qt_GraphicsImportSetDataReference, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, Handle dataRef, OSType dataReType) -> (ComponentResult _rv)")},
	{"GraphicsImportGetDataReference", (PyCFunction)Qt_GraphicsImportGetDataReference, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, Handle dataRef, OSType dataReType)")},
	{"GraphicsImportSetDataFile", (PyCFunction)Qt_GraphicsImportSetDataFile, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, FSSpec theFile) -> (ComponentResult _rv)")},
	{"GraphicsImportGetDataFile", (PyCFunction)Qt_GraphicsImportGetDataFile, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, FSSpec theFile) -> (ComponentResult _rv)")},
	{"GraphicsImportSetDataHandle", (PyCFunction)Qt_GraphicsImportSetDataHandle, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, Handle h) -> (ComponentResult _rv)")},
	{"GraphicsImportGetDataHandle", (PyCFunction)Qt_GraphicsImportGetDataHandle, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, Handle h)")},
	{"GraphicsImportGetImageDescription", (PyCFunction)Qt_GraphicsImportGetImageDescription, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, ImageDescriptionHandle desc)")},
	{"GraphicsImportGetDataOffsetAndSize", (PyCFunction)Qt_GraphicsImportGetDataOffsetAndSize, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, unsigned long offset, unsigned long size)")},
	{"GraphicsImportReadData", (PyCFunction)Qt_GraphicsImportReadData, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, void * dataPtr, unsigned long dataOffset, unsigned long dataSize) -> (ComponentResult _rv)")},
	{"GraphicsImportSetClip", (PyCFunction)Qt_GraphicsImportSetClip, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, RgnHandle clipRgn) -> (ComponentResult _rv)")},
	{"GraphicsImportGetClip", (PyCFunction)Qt_GraphicsImportGetClip, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, RgnHandle clipRgn)")},
	{"GraphicsImportSetSourceRect", (PyCFunction)Qt_GraphicsImportSetSourceRect, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, Rect sourceRect) -> (ComponentResult _rv)")},
	{"GraphicsImportGetSourceRect", (PyCFunction)Qt_GraphicsImportGetSourceRect, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, Rect sourceRect)")},
	{"GraphicsImportGetNaturalBounds", (PyCFunction)Qt_GraphicsImportGetNaturalBounds, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, Rect naturalBounds)")},
	{"GraphicsImportDraw", (PyCFunction)Qt_GraphicsImportDraw, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv)")},
	{"GraphicsImportSetGWorld", (PyCFunction)Qt_GraphicsImportSetGWorld, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, CGrafPtr port, GDHandle gd) -> (ComponentResult _rv)")},
	{"GraphicsImportGetGWorld", (PyCFunction)Qt_GraphicsImportGetGWorld, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, CGrafPtr port, GDHandle gd)")},
	{"GraphicsImportSetBoundsRect", (PyCFunction)Qt_GraphicsImportSetBoundsRect, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, Rect bounds) -> (ComponentResult _rv)")},
	{"GraphicsImportGetBoundsRect", (PyCFunction)Qt_GraphicsImportGetBoundsRect, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, Rect bounds)")},
	{"GraphicsImportSaveAsPicture", (PyCFunction)Qt_GraphicsImportSaveAsPicture, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, FSSpec fss, ScriptCode scriptTag) -> (ComponentResult _rv)")},
	{"GraphicsImportSetGraphicsMode", (PyCFunction)Qt_GraphicsImportSetGraphicsMode, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, long graphicsMode, RGBColor opColor) -> (ComponentResult _rv)")},
	{"GraphicsImportGetGraphicsMode", (PyCFunction)Qt_GraphicsImportGetGraphicsMode, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, long graphicsMode, RGBColor opColor)")},
	{"GraphicsImportSetQuality", (PyCFunction)Qt_GraphicsImportSetQuality, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, CodecQ quality) -> (ComponentResult _rv)")},
	{"GraphicsImportGetQuality", (PyCFunction)Qt_GraphicsImportGetQuality, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, CodecQ quality)")},
	{"GraphicsImportSaveAsQuickTimeImageFile", (PyCFunction)Qt_GraphicsImportSaveAsQuickTimeImageFile, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, FSSpec fss, ScriptCode scriptTag) -> (ComponentResult _rv)")},
	{"GraphicsImportSetDataReferenceOffsetAndLimit", (PyCFunction)Qt_GraphicsImportSetDataReferenceOffsetAndLimit, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, unsigned long offset, unsigned long limit) -> (ComponentResult _rv)")},
	{"GraphicsImportGetDataReferenceOffsetAndLimit", (PyCFunction)Qt_GraphicsImportGetDataReferenceOffsetAndLimit, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, unsigned long offset, unsigned long limit)")},
	{"GraphicsImportGetAliasedDataReference", (PyCFunction)Qt_GraphicsImportGetAliasedDataReference, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, Handle dataRef, OSType dataRefType)")},
	{"GraphicsImportValidate", (PyCFunction)Qt_GraphicsImportValidate, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, Boolean valid)")},
	{"GraphicsImportGetMetaData", (PyCFunction)Qt_GraphicsImportGetMetaData, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, void * userData) -> (ComponentResult _rv)")},
	{"GraphicsImportGetMIMETypeList", (PyCFunction)Qt_GraphicsImportGetMIMETypeList, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, void * qtAtomContainerPtr) -> (ComponentResult _rv)")},
	{"GraphicsImportDoesDrawAllPixels", (PyCFunction)Qt_GraphicsImportDoesDrawAllPixels, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, short drawsAllPixels)")},
	{"GraphicsImportGetAsPicture", (PyCFunction)Qt_GraphicsImportGetAsPicture, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, PicHandle picture)")},
	{"GraphicsImportExportImageFile", (PyCFunction)Qt_GraphicsImportExportImageFile, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, OSType fileType, OSType fileCreator, FSSpec fss, ScriptCode scriptTag) -> (ComponentResult _rv)")},
	{"GraphicsImportGetExportImageTypeList", (PyCFunction)Qt_GraphicsImportGetExportImageTypeList, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, void * qtAtomContainerPtr) -> (ComponentResult _rv)")},
	{"GraphicsImportGetExportSettingsAsAtomContainer", (PyCFunction)Qt_GraphicsImportGetExportSettingsAsAtomContainer, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, void * qtAtomContainerPtr) -> (ComponentResult _rv)")},
	{"GraphicsImportSetExportSettingsFromAtomContainer", (PyCFunction)Qt_GraphicsImportSetExportSettingsFromAtomContainer, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, void * qtAtomContainer) -> (ComponentResult _rv)")},
	{"GraphicsImportGetImageCount", (PyCFunction)Qt_GraphicsImportGetImageCount, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, unsigned long imageCount)")},
	{"GraphicsImportSetImageIndex", (PyCFunction)Qt_GraphicsImportSetImageIndex, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, unsigned long imageIndex) -> (ComponentResult _rv)")},
	{"GraphicsImportGetImageIndex", (PyCFunction)Qt_GraphicsImportGetImageIndex, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, unsigned long imageIndex)")},
	{"GraphicsImportGetDataOffsetAndSize64", (PyCFunction)Qt_GraphicsImportGetDataOffsetAndSize64, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, wide offset, wide size)")},
	{"GraphicsImportReadData64", (PyCFunction)Qt_GraphicsImportReadData64, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, void * dataPtr, wide dataOffset, unsigned long dataSize) -> (ComponentResult _rv)")},
	{"GraphicsImportSetDataReferenceOffsetAndLimit64", (PyCFunction)Qt_GraphicsImportSetDataReferenceOffsetAndLimit64, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, wide offset, wide limit) -> (ComponentResult _rv)")},
	{"GraphicsImportGetDataReferenceOffsetAndLimit64", (PyCFunction)Qt_GraphicsImportGetDataReferenceOffsetAndLimit64, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, wide offset, wide limit)")},
	{"GraphicsImportGetDefaultClip", (PyCFunction)Qt_GraphicsImportGetDefaultClip, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, RgnHandle defaultRgn)")},
	{"GraphicsImportGetDefaultGraphicsMode", (PyCFunction)Qt_GraphicsImportGetDefaultGraphicsMode, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, long defaultGraphicsMode, RGBColor defaultOpColor)")},
	{"GraphicsImportGetDefaultSourceRect", (PyCFunction)Qt_GraphicsImportGetDefaultSourceRect, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, Rect defaultSourceRect)")},
	{"GraphicsImportGetColorSyncProfile", (PyCFunction)Qt_GraphicsImportGetColorSyncProfile, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, Handle profile)")},
	{"GraphicsImportSetDestRect", (PyCFunction)Qt_GraphicsImportSetDestRect, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, Rect destRect) -> (ComponentResult _rv)")},
	{"GraphicsImportGetDestRect", (PyCFunction)Qt_GraphicsImportGetDestRect, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, Rect destRect)")},
	{"GraphicsImportSetFlags", (PyCFunction)Qt_GraphicsImportSetFlags, 1,
	 PyDoc_STR("(GraphicsImportComponent ci, long flags) -> (ComponentResult _rv)")},
	{"GraphicsImportGetFlags", (PyCFunction)Qt_GraphicsImportGetFlags, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, long flags)")},
	{"GraphicsImportGetBaseDataOffsetAndSize64", (PyCFunction)Qt_GraphicsImportGetBaseDataOffsetAndSize64, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv, wide offset, wide size)")},
	{"GraphicsImportSetImageIndexToThumbnail", (PyCFunction)Qt_GraphicsImportSetImageIndexToThumbnail, 1,
	 PyDoc_STR("(GraphicsImportComponent ci) -> (ComponentResult _rv)")},
	{"GraphicsExportDoExport", (PyCFunction)Qt_GraphicsExportDoExport, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, unsigned long actualSizeWritten)")},
	{"GraphicsExportCanTranscode", (PyCFunction)Qt_GraphicsExportCanTranscode, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Boolean canTranscode)")},
	{"GraphicsExportDoTranscode", (PyCFunction)Qt_GraphicsExportDoTranscode, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv)")},
	{"GraphicsExportCanUseCompressor", (PyCFunction)Qt_GraphicsExportCanUseCompressor, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, void * codecSettingsAtomContainerPtr) -> (ComponentResult _rv, Boolean canUseCompressor)")},
	{"GraphicsExportDoUseCompressor", (PyCFunction)Qt_GraphicsExportDoUseCompressor, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, void * codecSettingsAtomContainer) -> (ComponentResult _rv, ImageDescriptionHandle outDesc)")},
	{"GraphicsExportDoStandaloneExport", (PyCFunction)Qt_GraphicsExportDoStandaloneExport, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv)")},
	{"GraphicsExportGetDefaultFileTypeAndCreator", (PyCFunction)Qt_GraphicsExportGetDefaultFileTypeAndCreator, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, OSType fileType, OSType fileCreator)")},
	{"GraphicsExportGetDefaultFileNameExtension", (PyCFunction)Qt_GraphicsExportGetDefaultFileNameExtension, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, OSType fileNameExtension)")},
	{"GraphicsExportGetMIMETypeList", (PyCFunction)Qt_GraphicsExportGetMIMETypeList, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, void * qtAtomContainerPtr) -> (ComponentResult _rv)")},
	{"GraphicsExportSetSettingsFromAtomContainer", (PyCFunction)Qt_GraphicsExportSetSettingsFromAtomContainer, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, void * qtAtomContainer) -> (ComponentResult _rv)")},
	{"GraphicsExportGetSettingsAsAtomContainer", (PyCFunction)Qt_GraphicsExportGetSettingsAsAtomContainer, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, void * qtAtomContainerPtr) -> (ComponentResult _rv)")},
	{"GraphicsExportGetSettingsAsText", (PyCFunction)Qt_GraphicsExportGetSettingsAsText, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Handle theText)")},
	{"GraphicsExportSetDontRecompress", (PyCFunction)Qt_GraphicsExportSetDontRecompress, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, Boolean dontRecompress) -> (ComponentResult _rv)")},
	{"GraphicsExportGetDontRecompress", (PyCFunction)Qt_GraphicsExportGetDontRecompress, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Boolean dontRecompress)")},
	{"GraphicsExportSetInterlaceStyle", (PyCFunction)Qt_GraphicsExportSetInterlaceStyle, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, unsigned long interlaceStyle) -> (ComponentResult _rv)")},
	{"GraphicsExportGetInterlaceStyle", (PyCFunction)Qt_GraphicsExportGetInterlaceStyle, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, unsigned long interlaceStyle)")},
	{"GraphicsExportSetMetaData", (PyCFunction)Qt_GraphicsExportSetMetaData, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, void * userData) -> (ComponentResult _rv)")},
	{"GraphicsExportGetMetaData", (PyCFunction)Qt_GraphicsExportGetMetaData, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, void * userData) -> (ComponentResult _rv)")},
	{"GraphicsExportSetTargetDataSize", (PyCFunction)Qt_GraphicsExportSetTargetDataSize, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, unsigned long targetDataSize) -> (ComponentResult _rv)")},
	{"GraphicsExportGetTargetDataSize", (PyCFunction)Qt_GraphicsExportGetTargetDataSize, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, unsigned long targetDataSize)")},
	{"GraphicsExportSetCompressionMethod", (PyCFunction)Qt_GraphicsExportSetCompressionMethod, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, long compressionMethod) -> (ComponentResult _rv)")},
	{"GraphicsExportGetCompressionMethod", (PyCFunction)Qt_GraphicsExportGetCompressionMethod, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, long compressionMethod)")},
	{"GraphicsExportSetCompressionQuality", (PyCFunction)Qt_GraphicsExportSetCompressionQuality, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, CodecQ spatialQuality) -> (ComponentResult _rv)")},
	{"GraphicsExportGetCompressionQuality", (PyCFunction)Qt_GraphicsExportGetCompressionQuality, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, CodecQ spatialQuality)")},
	{"GraphicsExportSetResolution", (PyCFunction)Qt_GraphicsExportSetResolution, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, Fixed horizontalResolution, Fixed verticalResolution) -> (ComponentResult _rv)")},
	{"GraphicsExportGetResolution", (PyCFunction)Qt_GraphicsExportGetResolution, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Fixed horizontalResolution, Fixed verticalResolution)")},
	{"GraphicsExportSetDepth", (PyCFunction)Qt_GraphicsExportSetDepth, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, long depth) -> (ComponentResult _rv)")},
	{"GraphicsExportGetDepth", (PyCFunction)Qt_GraphicsExportGetDepth, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, long depth)")},
	{"GraphicsExportSetColorSyncProfile", (PyCFunction)Qt_GraphicsExportSetColorSyncProfile, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, Handle colorSyncProfile) -> (ComponentResult _rv)")},
	{"GraphicsExportGetColorSyncProfile", (PyCFunction)Qt_GraphicsExportGetColorSyncProfile, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Handle colorSyncProfile)")},
	{"GraphicsExportSetInputDataReference", (PyCFunction)Qt_GraphicsExportSetInputDataReference, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, Handle dataRef, OSType dataRefType, ImageDescriptionHandle desc) -> (ComponentResult _rv)")},
	{"GraphicsExportGetInputDataReference", (PyCFunction)Qt_GraphicsExportGetInputDataReference, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Handle dataRef, OSType dataRefType)")},
	{"GraphicsExportSetInputFile", (PyCFunction)Qt_GraphicsExportSetInputFile, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, FSSpec theFile, ImageDescriptionHandle desc) -> (ComponentResult _rv)")},
	{"GraphicsExportGetInputFile", (PyCFunction)Qt_GraphicsExportGetInputFile, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, FSSpec theFile) -> (ComponentResult _rv)")},
	{"GraphicsExportSetInputHandle", (PyCFunction)Qt_GraphicsExportSetInputHandle, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, Handle h, ImageDescriptionHandle desc) -> (ComponentResult _rv)")},
	{"GraphicsExportGetInputHandle", (PyCFunction)Qt_GraphicsExportGetInputHandle, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Handle h)")},
	{"GraphicsExportSetInputPtr", (PyCFunction)Qt_GraphicsExportSetInputPtr, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, Ptr p, unsigned long size, ImageDescriptionHandle desc) -> (ComponentResult _rv)")},
	{"GraphicsExportSetInputGraphicsImporter", (PyCFunction)Qt_GraphicsExportSetInputGraphicsImporter, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, GraphicsImportComponent grip) -> (ComponentResult _rv)")},
	{"GraphicsExportGetInputGraphicsImporter", (PyCFunction)Qt_GraphicsExportGetInputGraphicsImporter, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, GraphicsImportComponent grip)")},
	{"GraphicsExportSetInputPicture", (PyCFunction)Qt_GraphicsExportSetInputPicture, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, PicHandle picture) -> (ComponentResult _rv)")},
	{"GraphicsExportGetInputPicture", (PyCFunction)Qt_GraphicsExportGetInputPicture, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, PicHandle picture)")},
	{"GraphicsExportSetInputGWorld", (PyCFunction)Qt_GraphicsExportSetInputGWorld, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, GWorldPtr gworld) -> (ComponentResult _rv)")},
	{"GraphicsExportGetInputGWorld", (PyCFunction)Qt_GraphicsExportGetInputGWorld, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, GWorldPtr gworld)")},
	{"GraphicsExportSetInputPixmap", (PyCFunction)Qt_GraphicsExportSetInputPixmap, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, PixMapHandle pixmap) -> (ComponentResult _rv)")},
	{"GraphicsExportGetInputPixmap", (PyCFunction)Qt_GraphicsExportGetInputPixmap, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, PixMapHandle pixmap)")},
	{"GraphicsExportSetInputOffsetAndLimit", (PyCFunction)Qt_GraphicsExportSetInputOffsetAndLimit, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, unsigned long offset, unsigned long limit) -> (ComponentResult _rv)")},
	{"GraphicsExportGetInputOffsetAndLimit", (PyCFunction)Qt_GraphicsExportGetInputOffsetAndLimit, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, unsigned long offset, unsigned long limit)")},
	{"GraphicsExportMayExporterReadInputData", (PyCFunction)Qt_GraphicsExportMayExporterReadInputData, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Boolean mayReadInputData)")},
	{"GraphicsExportGetInputDataSize", (PyCFunction)Qt_GraphicsExportGetInputDataSize, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, unsigned long size)")},
	{"GraphicsExportReadInputData", (PyCFunction)Qt_GraphicsExportReadInputData, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, void * dataPtr, unsigned long dataOffset, unsigned long dataSize) -> (ComponentResult _rv)")},
	{"GraphicsExportGetInputImageDescription", (PyCFunction)Qt_GraphicsExportGetInputImageDescription, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, ImageDescriptionHandle desc)")},
	{"GraphicsExportGetInputImageDimensions", (PyCFunction)Qt_GraphicsExportGetInputImageDimensions, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Rect dimensions)")},
	{"GraphicsExportGetInputImageDepth", (PyCFunction)Qt_GraphicsExportGetInputImageDepth, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, long inputDepth)")},
	{"GraphicsExportDrawInputImage", (PyCFunction)Qt_GraphicsExportDrawInputImage, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, CGrafPtr gw, GDHandle gd, Rect srcRect, Rect dstRect) -> (ComponentResult _rv)")},
	{"GraphicsExportSetOutputDataReference", (PyCFunction)Qt_GraphicsExportSetOutputDataReference, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, Handle dataRef, OSType dataRefType) -> (ComponentResult _rv)")},
	{"GraphicsExportGetOutputDataReference", (PyCFunction)Qt_GraphicsExportGetOutputDataReference, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Handle dataRef, OSType dataRefType)")},
	{"GraphicsExportSetOutputFile", (PyCFunction)Qt_GraphicsExportSetOutputFile, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, FSSpec theFile) -> (ComponentResult _rv)")},
	{"GraphicsExportGetOutputFile", (PyCFunction)Qt_GraphicsExportGetOutputFile, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, FSSpec theFile) -> (ComponentResult _rv)")},
	{"GraphicsExportSetOutputHandle", (PyCFunction)Qt_GraphicsExportSetOutputHandle, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, Handle h) -> (ComponentResult _rv)")},
	{"GraphicsExportGetOutputHandle", (PyCFunction)Qt_GraphicsExportGetOutputHandle, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Handle h)")},
	{"GraphicsExportSetOutputOffsetAndMaxSize", (PyCFunction)Qt_GraphicsExportSetOutputOffsetAndMaxSize, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, unsigned long offset, unsigned long maxSize, Boolean truncateFile) -> (ComponentResult _rv)")},
	{"GraphicsExportGetOutputOffsetAndMaxSize", (PyCFunction)Qt_GraphicsExportGetOutputOffsetAndMaxSize, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, unsigned long offset, unsigned long maxSize, Boolean truncateFile)")},
	{"GraphicsExportSetOutputFileTypeAndCreator", (PyCFunction)Qt_GraphicsExportSetOutputFileTypeAndCreator, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, OSType fileType, OSType fileCreator) -> (ComponentResult _rv)")},
	{"GraphicsExportGetOutputFileTypeAndCreator", (PyCFunction)Qt_GraphicsExportGetOutputFileTypeAndCreator, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, OSType fileType, OSType fileCreator)")},
	{"GraphicsExportSetOutputMark", (PyCFunction)Qt_GraphicsExportSetOutputMark, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, unsigned long mark) -> (ComponentResult _rv)")},
	{"GraphicsExportGetOutputMark", (PyCFunction)Qt_GraphicsExportGetOutputMark, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, unsigned long mark)")},
	{"GraphicsExportReadOutputData", (PyCFunction)Qt_GraphicsExportReadOutputData, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, void * dataPtr, unsigned long dataOffset, unsigned long dataSize) -> (ComponentResult _rv)")},
	{"GraphicsExportSetThumbnailEnabled", (PyCFunction)Qt_GraphicsExportSetThumbnailEnabled, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, Boolean enableThumbnail, long maxThumbnailWidth, long maxThumbnailHeight) -> (ComponentResult _rv)")},
	{"GraphicsExportGetThumbnailEnabled", (PyCFunction)Qt_GraphicsExportGetThumbnailEnabled, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Boolean thumbnailEnabled, long maxThumbnailWidth, long maxThumbnailHeight)")},
	{"GraphicsExportSetExifEnabled", (PyCFunction)Qt_GraphicsExportSetExifEnabled, 1,
	 PyDoc_STR("(GraphicsExportComponent ci, Boolean enableExif) -> (ComponentResult _rv)")},
	{"GraphicsExportGetExifEnabled", (PyCFunction)Qt_GraphicsExportGetExifEnabled, 1,
	 PyDoc_STR("(GraphicsExportComponent ci) -> (ComponentResult _rv, Boolean exifEnabled)")},
	{"ImageTranscoderBeginSequence", (PyCFunction)Qt_ImageTranscoderBeginSequence, 1,
	 PyDoc_STR("(ImageTranscoderComponent itc, ImageDescriptionHandle srcDesc, void * data, long dataSize) -> (ComponentResult _rv, ImageDescriptionHandle dstDesc)")},
	{"ImageTranscoderDisposeData", (PyCFunction)Qt_ImageTranscoderDisposeData, 1,
	 PyDoc_STR("(ImageTranscoderComponent itc, void * dstData) -> (ComponentResult _rv)")},
	{"ImageTranscoderEndSequence", (PyCFunction)Qt_ImageTranscoderEndSequence, 1,
	 PyDoc_STR("(ImageTranscoderComponent itc) -> (ComponentResult _rv)")},
	{"ClockGetTime", (PyCFunction)Qt_ClockGetTime, 1,
	 PyDoc_STR("(ComponentInstance aClock) -> (ComponentResult _rv, TimeRecord out)")},
	{"ClockSetTimeBase", (PyCFunction)Qt_ClockSetTimeBase, 1,
	 PyDoc_STR("(ComponentInstance aClock, TimeBase tb) -> (ComponentResult _rv)")},
	{"ClockGetRate", (PyCFunction)Qt_ClockGetRate, 1,
	 PyDoc_STR("(ComponentInstance aClock) -> (ComponentResult _rv, Fixed rate)")},
	{"SCPositionRect", (PyCFunction)Qt_SCPositionRect, 1,
	 PyDoc_STR("(ComponentInstance ci) -> (ComponentResult _rv, Rect rp, Point where)")},
	{"SCPositionDialog", (PyCFunction)Qt_SCPositionDialog, 1,
	 PyDoc_STR("(ComponentInstance ci, short id) -> (ComponentResult _rv, Point where)")},
	{"SCSetTestImagePictHandle", (PyCFunction)Qt_SCSetTestImagePictHandle, 1,
	 PyDoc_STR("(ComponentInstance ci, PicHandle testPict, short testFlags) -> (ComponentResult _rv, Rect testRect)")},
	{"SCSetTestImagePictFile", (PyCFunction)Qt_SCSetTestImagePictFile, 1,
	 PyDoc_STR("(ComponentInstance ci, short testFileRef, short testFlags) -> (ComponentResult _rv, Rect testRect)")},
	{"SCSetTestImagePixMap", (PyCFunction)Qt_SCSetTestImagePixMap, 1,
	 PyDoc_STR("(ComponentInstance ci, PixMapHandle testPixMap, short testFlags) -> (ComponentResult _rv, Rect testRect)")},
	{"SCGetBestDeviceRect", (PyCFunction)Qt_SCGetBestDeviceRect, 1,
	 PyDoc_STR("(ComponentInstance ci) -> (ComponentResult _rv, Rect r)")},
	{"SCRequestImageSettings", (PyCFunction)Qt_SCRequestImageSettings, 1,
	 PyDoc_STR("(ComponentInstance ci) -> (ComponentResult _rv)")},
	{"SCCompressImage", (PyCFunction)Qt_SCCompressImage, 1,
	 PyDoc_STR("(ComponentInstance ci, PixMapHandle src, Rect srcRect) -> (ComponentResult _rv, ImageDescriptionHandle desc, Handle data)")},
	{"SCCompressPicture", (PyCFunction)Qt_SCCompressPicture, 1,
	 PyDoc_STR("(ComponentInstance ci, PicHandle srcPicture, PicHandle dstPicture) -> (ComponentResult _rv)")},
	{"SCCompressPictureFile", (PyCFunction)Qt_SCCompressPictureFile, 1,
	 PyDoc_STR("(ComponentInstance ci, short srcRefNum, short dstRefNum) -> (ComponentResult _rv)")},
	{"SCRequestSequenceSettings", (PyCFunction)Qt_SCRequestSequenceSettings, 1,
	 PyDoc_STR("(ComponentInstance ci) -> (ComponentResult _rv)")},
	{"SCCompressSequenceBegin", (PyCFunction)Qt_SCCompressSequenceBegin, 1,
	 PyDoc_STR("(ComponentInstance ci, PixMapHandle src, Rect srcRect) -> (ComponentResult _rv, ImageDescriptionHandle desc)")},
	{"SCCompressSequenceFrame", (PyCFunction)Qt_SCCompressSequenceFrame, 1,
	 PyDoc_STR("(ComponentInstance ci, PixMapHandle src, Rect srcRect) -> (ComponentResult _rv, Handle data, long dataSize, short notSyncFlag)")},
	{"SCCompressSequenceEnd", (PyCFunction)Qt_SCCompressSequenceEnd, 1,
	 PyDoc_STR("(ComponentInstance ci) -> (ComponentResult _rv)")},
	{"SCDefaultPictHandleSettings", (PyCFunction)Qt_SCDefaultPictHandleSettings, 1,
	 PyDoc_STR("(ComponentInstance ci, PicHandle srcPicture, short motion) -> (ComponentResult _rv)")},
	{"SCDefaultPictFileSettings", (PyCFunction)Qt_SCDefaultPictFileSettings, 1,
	 PyDoc_STR("(ComponentInstance ci, short srcRef, short motion) -> (ComponentResult _rv)")},
	{"SCDefaultPixMapSettings", (PyCFunction)Qt_SCDefaultPixMapSettings, 1,
	 PyDoc_STR("(ComponentInstance ci, PixMapHandle src, short motion) -> (ComponentResult _rv)")},
	{"SCGetInfo", (PyCFunction)Qt_SCGetInfo, 1,
	 PyDoc_STR("(ComponentInstance ci, OSType infoType, void * info) -> (ComponentResult _rv)")},
	{"SCSetInfo", (PyCFunction)Qt_SCSetInfo, 1,
	 PyDoc_STR("(ComponentInstance ci, OSType infoType, void * info) -> (ComponentResult _rv)")},
	{"SCSetCompressFlags", (PyCFunction)Qt_SCSetCompressFlags, 1,
	 PyDoc_STR("(ComponentInstance ci, long flags) -> (ComponentResult _rv)")},
	{"SCGetCompressFlags", (PyCFunction)Qt_SCGetCompressFlags, 1,
	 PyDoc_STR("(ComponentInstance ci) -> (ComponentResult _rv, long flags)")},
	{"SCGetSettingsAsText", (PyCFunction)Qt_SCGetSettingsAsText, 1,
	 PyDoc_STR("(ComponentInstance ci) -> (ComponentResult _rv, Handle text)")},
	{"SCAsyncIdle", (PyCFunction)Qt_SCAsyncIdle, 1,
	 PyDoc_STR("(ComponentInstance ci) -> (ComponentResult _rv)")},
	{"TweenerReset", (PyCFunction)Qt_TweenerReset, 1,
	 PyDoc_STR("(TweenerComponent tc) -> (ComponentResult _rv)")},
	{"TCGetSourceRef", (PyCFunction)Qt_TCGetSourceRef, 1,
	 PyDoc_STR("(MediaHandler mh, TimeCodeDescriptionHandle tcdH) -> (HandlerError _rv, UserData srefH)")},
	{"TCSetSourceRef", (PyCFunction)Qt_TCSetSourceRef, 1,
	 PyDoc_STR("(MediaHandler mh, TimeCodeDescriptionHandle tcdH, UserData srefH) -> (HandlerError _rv)")},
	{"TCSetTimeCodeFlags", (PyCFunction)Qt_TCSetTimeCodeFlags, 1,
	 PyDoc_STR("(MediaHandler mh, long flags, long flagsMask) -> (HandlerError _rv)")},
	{"TCGetTimeCodeFlags", (PyCFunction)Qt_TCGetTimeCodeFlags, 1,
	 PyDoc_STR("(MediaHandler mh) -> (HandlerError _rv, long flags)")},
	{"MovieImportHandle", (PyCFunction)Qt_MovieImportHandle, 1,
	 PyDoc_STR("(MovieImportComponent ci, Handle dataH, Movie theMovie, Track targetTrack, TimeValue atTime, long inFlags) -> (ComponentResult _rv, Track usedTrack, TimeValue addedDuration, long outFlags)")},
	{"MovieImportFile", (PyCFunction)Qt_MovieImportFile, 1,
	 PyDoc_STR("(MovieImportComponent ci, FSSpec theFile, Movie theMovie, Track targetTrack, TimeValue atTime, long inFlags) -> (ComponentResult _rv, Track usedTrack, TimeValue addedDuration, long outFlags)")},
	{"MovieImportSetSampleDuration", (PyCFunction)Qt_MovieImportSetSampleDuration, 1,
	 PyDoc_STR("(MovieImportComponent ci, TimeValue duration, TimeScale scale) -> (ComponentResult _rv)")},
	{"MovieImportSetSampleDescription", (PyCFunction)Qt_MovieImportSetSampleDescription, 1,
	 PyDoc_STR("(MovieImportComponent ci, SampleDescriptionHandle desc, OSType mediaType) -> (ComponentResult _rv)")},
	{"MovieImportSetMediaFile", (PyCFunction)Qt_MovieImportSetMediaFile, 1,
	 PyDoc_STR("(MovieImportComponent ci, AliasHandle alias) -> (ComponentResult _rv)")},
	{"MovieImportSetDimensions", (PyCFunction)Qt_MovieImportSetDimensions, 1,
	 PyDoc_STR("(MovieImportComponent ci, Fixed width, Fixed height) -> (ComponentResult _rv)")},
	{"MovieImportSetChunkSize", (PyCFunction)Qt_MovieImportSetChunkSize, 1,
	 PyDoc_STR("(MovieImportComponent ci, long chunkSize) -> (ComponentResult _rv)")},
	{"MovieImportSetAuxiliaryData", (PyCFunction)Qt_MovieImportSetAuxiliaryData, 1,
	 PyDoc_STR("(MovieImportComponent ci, Handle data, OSType handleType) -> (ComponentResult _rv)")},
	{"MovieImportSetFromScrap", (PyCFunction)Qt_MovieImportSetFromScrap, 1,
	 PyDoc_STR("(MovieImportComponent ci, Boolean fromScrap) -> (ComponentResult _rv)")},
	{"MovieImportDoUserDialog", (PyCFunction)Qt_MovieImportDoUserDialog, 1,
	 PyDoc_STR("(MovieImportComponent ci, FSSpec theFile, Handle theData) -> (ComponentResult _rv, Boolean canceled)")},
	{"MovieImportSetDuration", (PyCFunction)Qt_MovieImportSetDuration, 1,
	 PyDoc_STR("(MovieImportComponent ci, TimeValue duration) -> (ComponentResult _rv)")},
	{"MovieImportGetAuxiliaryDataType", (PyCFunction)Qt_MovieImportGetAuxiliaryDataType, 1,
	 PyDoc_STR("(MovieImportComponent ci) -> (ComponentResult _rv, OSType auxType)")},
	{"MovieImportValidate", (PyCFunction)Qt_MovieImportValidate, 1,
	 PyDoc_STR("(MovieImportComponent ci, FSSpec theFile, Handle theData) -> (ComponentResult _rv, Boolean valid)")},
	{"MovieImportGetFileType", (PyCFunction)Qt_MovieImportGetFileType, 1,
	 PyDoc_STR("(MovieImportComponent ci) -> (ComponentResult _rv, OSType fileType)")},
	{"MovieImportDataRef", (PyCFunction)Qt_MovieImportDataRef, 1,
	 PyDoc_STR("(MovieImportComponent ci, Handle dataRef, OSType dataRefType, Movie theMovie, Track targetTrack, TimeValue atTime, long inFlags) -> (ComponentResult _rv, Track usedTrack, TimeValue addedDuration, long outFlags)")},
	{"MovieImportGetSampleDescription", (PyCFunction)Qt_MovieImportGetSampleDescription, 1,
	 PyDoc_STR("(MovieImportComponent ci) -> (ComponentResult _rv, SampleDescriptionHandle desc, OSType mediaType)")},
	{"MovieImportSetOffsetAndLimit", (PyCFunction)Qt_MovieImportSetOffsetAndLimit, 1,
	 PyDoc_STR("(MovieImportComponent ci, unsigned long offset, unsigned long limit) -> (ComponentResult _rv)")},
	{"MovieImportSetOffsetAndLimit64", (PyCFunction)Qt_MovieImportSetOffsetAndLimit64, 1,
	 PyDoc_STR("(MovieImportComponent ci, wide offset, wide limit) -> (ComponentResult _rv)")},
	{"MovieImportIdle", (PyCFunction)Qt_MovieImportIdle, 1,
	 PyDoc_STR("(MovieImportComponent ci, long inFlags) -> (ComponentResult _rv, long outFlags)")},
	{"MovieImportValidateDataRef", (PyCFunction)Qt_MovieImportValidateDataRef, 1,
	 PyDoc_STR("(MovieImportComponent ci, Handle dataRef, OSType dataRefType) -> (ComponentResult _rv, UInt8 valid)")},
	{"MovieImportGetLoadState", (PyCFunction)Qt_MovieImportGetLoadState, 1,
	 PyDoc_STR("(MovieImportComponent ci) -> (ComponentResult _rv, long importerLoadState)")},
	{"MovieImportGetMaxLoadedTime", (PyCFunction)Qt_MovieImportGetMaxLoadedTime, 1,
	 PyDoc_STR("(MovieImportComponent ci) -> (ComponentResult _rv, TimeValue time)")},
	{"MovieImportEstimateCompletionTime", (PyCFunction)Qt_MovieImportEstimateCompletionTime, 1,
	 PyDoc_STR("(MovieImportComponent ci) -> (ComponentResult _rv, TimeRecord time)")},
	{"MovieImportSetDontBlock", (PyCFunction)Qt_MovieImportSetDontBlock, 1,
	 PyDoc_STR("(MovieImportComponent ci, Boolean dontBlock) -> (ComponentResult _rv)")},
	{"MovieImportGetDontBlock", (PyCFunction)Qt_MovieImportGetDontBlock, 1,
	 PyDoc_STR("(MovieImportComponent ci) -> (ComponentResult _rv, Boolean willBlock)")},
	{"MovieImportSetIdleManager", (PyCFunction)Qt_MovieImportSetIdleManager, 1,
	 PyDoc_STR("(MovieImportComponent ci, IdleManager im) -> (ComponentResult _rv)")},
	{"MovieImportSetNewMovieFlags", (PyCFunction)Qt_MovieImportSetNewMovieFlags, 1,
	 PyDoc_STR("(MovieImportComponent ci, long newMovieFlags) -> (ComponentResult _rv)")},
	{"MovieImportGetDestinationMediaType", (PyCFunction)Qt_MovieImportGetDestinationMediaType, 1,
	 PyDoc_STR("(MovieImportComponent ci) -> (ComponentResult _rv, OSType mediaType)")},
	{"MovieExportToHandle", (PyCFunction)Qt_MovieExportToHandle, 1,
	 PyDoc_STR("(MovieExportComponent ci, Handle dataH, Movie theMovie, Track onlyThisTrack, TimeValue startTime, TimeValue duration) -> (ComponentResult _rv)")},
	{"MovieExportToFile", (PyCFunction)Qt_MovieExportToFile, 1,
	 PyDoc_STR("(MovieExportComponent ci, FSSpec theFile, Movie theMovie, Track onlyThisTrack, TimeValue startTime, TimeValue duration) -> (ComponentResult _rv)")},
	{"MovieExportGetAuxiliaryData", (PyCFunction)Qt_MovieExportGetAuxiliaryData, 1,
	 PyDoc_STR("(MovieExportComponent ci, Handle dataH) -> (ComponentResult _rv, OSType handleType)")},
	{"MovieExportSetSampleDescription", (PyCFunction)Qt_MovieExportSetSampleDescription, 1,
	 PyDoc_STR("(MovieExportComponent ci, SampleDescriptionHandle desc, OSType mediaType) -> (ComponentResult _rv)")},
	{"MovieExportDoUserDialog", (PyCFunction)Qt_MovieExportDoUserDialog, 1,
	 PyDoc_STR("(MovieExportComponent ci, Movie theMovie, Track onlyThisTrack, TimeValue startTime, TimeValue duration) -> (ComponentResult _rv, Boolean canceled)")},
	{"MovieExportGetCreatorType", (PyCFunction)Qt_MovieExportGetCreatorType, 1,
	 PyDoc_STR("(MovieExportComponent ci) -> (ComponentResult _rv, OSType creator)")},
	{"MovieExportToDataRef", (PyCFunction)Qt_MovieExportToDataRef, 1,
	 PyDoc_STR("(MovieExportComponent ci, Handle dataRef, OSType dataRefType, Movie theMovie, Track onlyThisTrack, TimeValue startTime, TimeValue duration) -> (ComponentResult _rv)")},
	{"MovieExportFromProceduresToDataRef", (PyCFunction)Qt_MovieExportFromProceduresToDataRef, 1,
	 PyDoc_STR("(MovieExportComponent ci, Handle dataRef, OSType dataRefType) -> (ComponentResult _rv)")},
	{"MovieExportValidate", (PyCFunction)Qt_MovieExportValidate, 1,
	 PyDoc_STR("(MovieExportComponent ci, Movie theMovie, Track onlyThisTrack) -> (ComponentResult _rv, Boolean valid)")},
	{"MovieExportGetFileNameExtension", (PyCFunction)Qt_MovieExportGetFileNameExtension, 1,
	 PyDoc_STR("(MovieExportComponent ci) -> (ComponentResult _rv, OSType extension)")},
	{"MovieExportGetShortFileTypeString", (PyCFunction)Qt_MovieExportGetShortFileTypeString, 1,
	 PyDoc_STR("(MovieExportComponent ci, Str255 typeString) -> (ComponentResult _rv)")},
	{"MovieExportGetSourceMediaType", (PyCFunction)Qt_MovieExportGetSourceMediaType, 1,
	 PyDoc_STR("(MovieExportComponent ci) -> (ComponentResult _rv, OSType mediaType)")},
	{"TextExportGetTimeFraction", (PyCFunction)Qt_TextExportGetTimeFraction, 1,
	 PyDoc_STR("(TextExportComponent ci) -> (ComponentResult _rv, long movieTimeFraction)")},
	{"TextExportSetTimeFraction", (PyCFunction)Qt_TextExportSetTimeFraction, 1,
	 PyDoc_STR("(TextExportComponent ci, long movieTimeFraction) -> (ComponentResult _rv)")},
	{"TextExportGetSettings", (PyCFunction)Qt_TextExportGetSettings, 1,
	 PyDoc_STR("(TextExportComponent ci) -> (ComponentResult _rv, long setting)")},
	{"TextExportSetSettings", (PyCFunction)Qt_TextExportSetSettings, 1,
	 PyDoc_STR("(TextExportComponent ci, long setting) -> (ComponentResult _rv)")},
	{"MIDIImportGetSettings", (PyCFunction)Qt_MIDIImportGetSettings, 1,
	 PyDoc_STR("(TextExportComponent ci) -> (ComponentResult _rv, long setting)")},
	{"MIDIImportSetSettings", (PyCFunction)Qt_MIDIImportSetSettings, 1,
	 PyDoc_STR("(TextExportComponent ci, long setting) -> (ComponentResult _rv)")},
	{"GraphicsImageImportSetSequenceEnabled", (PyCFunction)Qt_GraphicsImageImportSetSequenceEnabled, 1,
	 PyDoc_STR("(GraphicImageMovieImportComponent ci, Boolean enable) -> (ComponentResult _rv)")},
	{"GraphicsImageImportGetSequenceEnabled", (PyCFunction)Qt_GraphicsImageImportGetSequenceEnabled, 1,
	 PyDoc_STR("(GraphicImageMovieImportComponent ci) -> (ComponentResult _rv, Boolean enable)")},
	{"PreviewShowData", (PyCFunction)Qt_PreviewShowData, 1,
	 PyDoc_STR("(pnotComponent p, OSType dataType, Handle data, Rect inHere) -> (ComponentResult _rv)")},
	{"PreviewMakePreviewReference", (PyCFunction)Qt_PreviewMakePreviewReference, 1,
	 PyDoc_STR("(pnotComponent p, FSSpec sourceFile) -> (ComponentResult _rv, OSType previewType, short resID)")},
	{"PreviewEvent", (PyCFunction)Qt_PreviewEvent, 1,
	 PyDoc_STR("(pnotComponent p) -> (ComponentResult _rv, EventRecord e, Boolean handledEvent)")},
	{"DataCodecDecompress", (PyCFunction)Qt_DataCodecDecompress, 1,
	 PyDoc_STR("(DataCodecComponent dc, void * srcData, UInt32 srcSize, void * dstData, UInt32 dstBufferSize) -> (ComponentResult _rv)")},
	{"DataCodecGetCompressBufferSize", (PyCFunction)Qt_DataCodecGetCompressBufferSize, 1,
	 PyDoc_STR("(DataCodecComponent dc, UInt32 srcSize) -> (ComponentResult _rv, UInt32 dstSize)")},
	{"DataCodecCompress", (PyCFunction)Qt_DataCodecCompress, 1,
	 PyDoc_STR("(DataCodecComponent dc, void * srcData, UInt32 srcSize, void * dstData, UInt32 dstBufferSize) -> (ComponentResult _rv, UInt32 actualDstSize, UInt32 decompressSlop)")},
	{"DataCodecBeginInterruptSafe", (PyCFunction)Qt_DataCodecBeginInterruptSafe, 1,
	 PyDoc_STR("(DataCodecComponent dc, unsigned long maxSrcSize) -> (ComponentResult _rv)")},
	{"DataCodecEndInterruptSafe", (PyCFunction)Qt_DataCodecEndInterruptSafe, 1,
	 PyDoc_STR("(DataCodecComponent dc) -> (ComponentResult _rv)")},
	{"DataHGetData", (PyCFunction)Qt_DataHGetData, 1,
	 PyDoc_STR("(DataHandler dh, Handle h, long hOffset, long offset, long size) -> (ComponentResult _rv)")},
	{"DataHPutData", (PyCFunction)Qt_DataHPutData, 1,
	 PyDoc_STR("(DataHandler dh, Handle h, long hOffset, long size) -> (ComponentResult _rv, long offset)")},
	{"DataHFlushData", (PyCFunction)Qt_DataHFlushData, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv)")},
	{"DataHOpenForWrite", (PyCFunction)Qt_DataHOpenForWrite, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv)")},
	{"DataHCloseForWrite", (PyCFunction)Qt_DataHCloseForWrite, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv)")},
	{"DataHOpenForRead", (PyCFunction)Qt_DataHOpenForRead, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv)")},
	{"DataHCloseForRead", (PyCFunction)Qt_DataHCloseForRead, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv)")},
	{"DataHSetDataRef", (PyCFunction)Qt_DataHSetDataRef, 1,
	 PyDoc_STR("(DataHandler dh, Handle dataRef) -> (ComponentResult _rv)")},
	{"DataHGetDataRef", (PyCFunction)Qt_DataHGetDataRef, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, Handle dataRef)")},
	{"DataHCompareDataRef", (PyCFunction)Qt_DataHCompareDataRef, 1,
	 PyDoc_STR("(DataHandler dh, Handle dataRef) -> (ComponentResult _rv, Boolean equal)")},
	{"DataHTask", (PyCFunction)Qt_DataHTask, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv)")},
	{"DataHFinishData", (PyCFunction)Qt_DataHFinishData, 1,
	 PyDoc_STR("(DataHandler dh, Ptr PlaceToPutDataPtr, Boolean Cancel) -> (ComponentResult _rv)")},
	{"DataHFlushCache", (PyCFunction)Qt_DataHFlushCache, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv)")},
	{"DataHResolveDataRef", (PyCFunction)Qt_DataHResolveDataRef, 1,
	 PyDoc_STR("(DataHandler dh, Handle theDataRef, Boolean userInterfaceAllowed) -> (ComponentResult _rv, Boolean wasChanged)")},
	{"DataHGetFileSize", (PyCFunction)Qt_DataHGetFileSize, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, long fileSize)")},
	{"DataHCanUseDataRef", (PyCFunction)Qt_DataHCanUseDataRef, 1,
	 PyDoc_STR("(DataHandler dh, Handle dataRef) -> (ComponentResult _rv, long useFlags)")},
	{"DataHPreextend", (PyCFunction)Qt_DataHPreextend, 1,
	 PyDoc_STR("(DataHandler dh, unsigned long maxToAdd) -> (ComponentResult _rv, unsigned long spaceAdded)")},
	{"DataHSetFileSize", (PyCFunction)Qt_DataHSetFileSize, 1,
	 PyDoc_STR("(DataHandler dh, long fileSize) -> (ComponentResult _rv)")},
	{"DataHGetFreeSpace", (PyCFunction)Qt_DataHGetFreeSpace, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, unsigned long freeSize)")},
	{"DataHCreateFile", (PyCFunction)Qt_DataHCreateFile, 1,
	 PyDoc_STR("(DataHandler dh, OSType creator, Boolean deleteExisting) -> (ComponentResult _rv)")},
	{"DataHGetPreferredBlockSize", (PyCFunction)Qt_DataHGetPreferredBlockSize, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, long blockSize)")},
	{"DataHGetDeviceIndex", (PyCFunction)Qt_DataHGetDeviceIndex, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, long deviceIndex)")},
	{"DataHIsStreamingDataHandler", (PyCFunction)Qt_DataHIsStreamingDataHandler, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, Boolean yes)")},
	{"DataHGetDataInBuffer", (PyCFunction)Qt_DataHGetDataInBuffer, 1,
	 PyDoc_STR("(DataHandler dh, long startOffset) -> (ComponentResult _rv, long size)")},
	{"DataHGetScheduleAheadTime", (PyCFunction)Qt_DataHGetScheduleAheadTime, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, long millisecs)")},
	{"DataHSetCacheSizeLimit", (PyCFunction)Qt_DataHSetCacheSizeLimit, 1,
	 PyDoc_STR("(DataHandler dh, Size cacheSizeLimit) -> (ComponentResult _rv)")},
	{"DataHGetCacheSizeLimit", (PyCFunction)Qt_DataHGetCacheSizeLimit, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, Size cacheSizeLimit)")},
	{"DataHGetMovie", (PyCFunction)Qt_DataHGetMovie, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, Movie theMovie, short id)")},
	{"DataHAddMovie", (PyCFunction)Qt_DataHAddMovie, 1,
	 PyDoc_STR("(DataHandler dh, Movie theMovie) -> (ComponentResult _rv, short id)")},
	{"DataHUpdateMovie", (PyCFunction)Qt_DataHUpdateMovie, 1,
	 PyDoc_STR("(DataHandler dh, Movie theMovie, short id) -> (ComponentResult _rv)")},
	{"DataHDoesBuffer", (PyCFunction)Qt_DataHDoesBuffer, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, Boolean buffersReads, Boolean buffersWrites)")},
	{"DataHGetFileName", (PyCFunction)Qt_DataHGetFileName, 1,
	 PyDoc_STR("(DataHandler dh, Str255 str) -> (ComponentResult _rv)")},
	{"DataHGetAvailableFileSize", (PyCFunction)Qt_DataHGetAvailableFileSize, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, long fileSize)")},
	{"DataHGetMacOSFileType", (PyCFunction)Qt_DataHGetMacOSFileType, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, OSType fileType)")},
	{"DataHGetMIMEType", (PyCFunction)Qt_DataHGetMIMEType, 1,
	 PyDoc_STR("(DataHandler dh, Str255 mimeType) -> (ComponentResult _rv)")},
	{"DataHSetDataRefWithAnchor", (PyCFunction)Qt_DataHSetDataRefWithAnchor, 1,
	 PyDoc_STR("(DataHandler dh, Handle anchorDataRef, OSType dataRefType, Handle dataRef) -> (ComponentResult _rv)")},
	{"DataHGetDataRefWithAnchor", (PyCFunction)Qt_DataHGetDataRefWithAnchor, 1,
	 PyDoc_STR("(DataHandler dh, Handle anchorDataRef, OSType dataRefType) -> (ComponentResult _rv, Handle dataRef)")},
	{"DataHSetMacOSFileType", (PyCFunction)Qt_DataHSetMacOSFileType, 1,
	 PyDoc_STR("(DataHandler dh, OSType fileType) -> (ComponentResult _rv)")},
	{"DataHSetTimeBase", (PyCFunction)Qt_DataHSetTimeBase, 1,
	 PyDoc_STR("(DataHandler dh, TimeBase tb) -> (ComponentResult _rv)")},
	{"DataHGetInfoFlags", (PyCFunction)Qt_DataHGetInfoFlags, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, UInt32 flags)")},
	{"DataHGetFileSize64", (PyCFunction)Qt_DataHGetFileSize64, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, wide fileSize)")},
	{"DataHPreextend64", (PyCFunction)Qt_DataHPreextend64, 1,
	 PyDoc_STR("(DataHandler dh, wide maxToAdd) -> (ComponentResult _rv, wide spaceAdded)")},
	{"DataHSetFileSize64", (PyCFunction)Qt_DataHSetFileSize64, 1,
	 PyDoc_STR("(DataHandler dh, wide fileSize) -> (ComponentResult _rv)")},
	{"DataHGetFreeSpace64", (PyCFunction)Qt_DataHGetFreeSpace64, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, wide freeSize)")},
	{"DataHAppend64", (PyCFunction)Qt_DataHAppend64, 1,
	 PyDoc_STR("(DataHandler dh, void * data, unsigned long size) -> (ComponentResult _rv, wide fileOffset)")},
	{"DataHPollRead", (PyCFunction)Qt_DataHPollRead, 1,
	 PyDoc_STR("(DataHandler dh, void * dataPtr) -> (ComponentResult _rv, UInt32 dataSizeSoFar)")},
	{"DataHGetDataAvailability", (PyCFunction)Qt_DataHGetDataAvailability, 1,
	 PyDoc_STR("(DataHandler dh, long offset, long len) -> (ComponentResult _rv, long missing_offset, long missing_len)")},
	{"DataHGetDataRefAsType", (PyCFunction)Qt_DataHGetDataRefAsType, 1,
	 PyDoc_STR("(DataHandler dh, OSType requestedType) -> (ComponentResult _rv, Handle dataRef)")},
	{"DataHSetDataRefExtension", (PyCFunction)Qt_DataHSetDataRefExtension, 1,
	 PyDoc_STR("(DataHandler dh, Handle extension, OSType idType) -> (ComponentResult _rv)")},
	{"DataHGetDataRefExtension", (PyCFunction)Qt_DataHGetDataRefExtension, 1,
	 PyDoc_STR("(DataHandler dh, OSType idType) -> (ComponentResult _rv, Handle extension)")},
	{"DataHGetMovieWithFlags", (PyCFunction)Qt_DataHGetMovieWithFlags, 1,
	 PyDoc_STR("(DataHandler dh, short flags) -> (ComponentResult _rv, Movie theMovie, short id)")},
	{"DataHGetFileTypeOrdering", (PyCFunction)Qt_DataHGetFileTypeOrdering, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, DataHFileTypeOrderingHandle orderingListHandle)")},
	{"DataHCreateFileWithFlags", (PyCFunction)Qt_DataHCreateFileWithFlags, 1,
	 PyDoc_STR("(DataHandler dh, OSType creator, Boolean deleteExisting, UInt32 flags) -> (ComponentResult _rv)")},
	{"DataHGetInfo", (PyCFunction)Qt_DataHGetInfo, 1,
	 PyDoc_STR("(DataHandler dh, OSType what, void * info) -> (ComponentResult _rv)")},
	{"DataHSetIdleManager", (PyCFunction)Qt_DataHSetIdleManager, 1,
	 PyDoc_STR("(DataHandler dh, IdleManager im) -> (ComponentResult _rv)")},
	{"DataHDeleteFile", (PyCFunction)Qt_DataHDeleteFile, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv)")},
	{"DataHSetMovieUsageFlags", (PyCFunction)Qt_DataHSetMovieUsageFlags, 1,
	 PyDoc_STR("(DataHandler dh, long flags) -> (ComponentResult _rv)")},
	{"DataHUseTemporaryDataRef", (PyCFunction)Qt_DataHUseTemporaryDataRef, 1,
	 PyDoc_STR("(DataHandler dh, long inFlags) -> (ComponentResult _rv)")},
	{"DataHGetTemporaryDataRefCapabilities", (PyCFunction)Qt_DataHGetTemporaryDataRefCapabilities, 1,
	 PyDoc_STR("(DataHandler dh) -> (ComponentResult _rv, long outUnderstoodFlags)")},
	{"DataHRenameFile", (PyCFunction)Qt_DataHRenameFile, 1,
	 PyDoc_STR("(DataHandler dh, Handle newDataRef) -> (ComponentResult _rv)")},
	{"DataHPlaybackHints", (PyCFunction)Qt_DataHPlaybackHints, 1,
	 PyDoc_STR("(DataHandler dh, long flags, unsigned long minFileOffset, unsigned long maxFileOffset, long bytesPerSecond) -> (ComponentResult _rv)")},
	{"DataHPlaybackHints64", (PyCFunction)Qt_DataHPlaybackHints64, 1,
	 PyDoc_STR("(DataHandler dh, long flags, wide minFileOffset, wide maxFileOffset, long bytesPerSecond) -> (ComponentResult _rv)")},
	{"DataHGetDataRate", (PyCFunction)Qt_DataHGetDataRate, 1,
	 PyDoc_STR("(DataHandler dh, long flags) -> (ComponentResult _rv, long bytesPerSecond)")},
	{"DataHSetTimeHints", (PyCFunction)Qt_DataHSetTimeHints, 1,
	 PyDoc_STR("(DataHandler dh, long flags, long bandwidthPriority, TimeScale scale, TimeValue minTime, TimeValue maxTime) -> (ComponentResult _rv)")},
	{"VDGetMaxSrcRect", (PyCFunction)Qt_VDGetMaxSrcRect, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short inputStd) -> (ComponentResult _rv, Rect maxSrcRect)")},
	{"VDGetActiveSrcRect", (PyCFunction)Qt_VDGetActiveSrcRect, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short inputStd) -> (ComponentResult _rv, Rect activeSrcRect)")},
	{"VDSetDigitizerRect", (PyCFunction)Qt_VDSetDigitizerRect, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, Rect digitizerRect)")},
	{"VDGetDigitizerRect", (PyCFunction)Qt_VDGetDigitizerRect, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, Rect digitizerRect)")},
	{"VDGetVBlankRect", (PyCFunction)Qt_VDGetVBlankRect, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short inputStd) -> (ComponentResult _rv, Rect vBlankRect)")},
	{"VDGetMaskPixMap", (PyCFunction)Qt_VDGetMaskPixMap, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, PixMapHandle maskPixMap) -> (ComponentResult _rv)")},
	{"VDUseThisCLUT", (PyCFunction)Qt_VDUseThisCLUT, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, CTabHandle colorTableHandle) -> (ComponentResult _rv)")},
	{"VDSetInputGammaValue", (PyCFunction)Qt_VDSetInputGammaValue, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, Fixed channel1, Fixed channel2, Fixed channel3) -> (ComponentResult _rv)")},
	{"VDGetInputGammaValue", (PyCFunction)Qt_VDGetInputGammaValue, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, Fixed channel1, Fixed channel2, Fixed channel3)")},
	{"VDSetBrightness", (PyCFunction)Qt_VDSetBrightness, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short brightness)")},
	{"VDGetBrightness", (PyCFunction)Qt_VDGetBrightness, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short brightness)")},
	{"VDSetContrast", (PyCFunction)Qt_VDSetContrast, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short contrast)")},
	{"VDSetHue", (PyCFunction)Qt_VDSetHue, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short hue)")},
	{"VDSetSharpness", (PyCFunction)Qt_VDSetSharpness, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short sharpness)")},
	{"VDSetSaturation", (PyCFunction)Qt_VDSetSaturation, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short saturation)")},
	{"VDGetContrast", (PyCFunction)Qt_VDGetContrast, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short contrast)")},
	{"VDGetHue", (PyCFunction)Qt_VDGetHue, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short hue)")},
	{"VDGetSharpness", (PyCFunction)Qt_VDGetSharpness, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short sharpness)")},
	{"VDGetSaturation", (PyCFunction)Qt_VDGetSaturation, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short saturation)")},
	{"VDGrabOneFrame", (PyCFunction)Qt_VDGrabOneFrame, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv)")},
	{"VDGetMaxAuxBuffer", (PyCFunction)Qt_VDGetMaxAuxBuffer, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, PixMapHandle pm, Rect r)")},
	{"VDGetCurrentFlags", (PyCFunction)Qt_VDGetCurrentFlags, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, long inputCurrentFlag, long outputCurrentFlag)")},
	{"VDSetKeyColor", (PyCFunction)Qt_VDSetKeyColor, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, long index) -> (ComponentResult _rv)")},
	{"VDGetKeyColor", (PyCFunction)Qt_VDGetKeyColor, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, long index)")},
	{"VDAddKeyColor", (PyCFunction)Qt_VDAddKeyColor, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, long index)")},
	{"VDGetNextKeyColor", (PyCFunction)Qt_VDGetNextKeyColor, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, long index) -> (ComponentResult _rv)")},
	{"VDSetKeyColorRange", (PyCFunction)Qt_VDSetKeyColorRange, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, RGBColor minRGB, RGBColor maxRGB)")},
	{"VDGetKeyColorRange", (PyCFunction)Qt_VDGetKeyColorRange, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, RGBColor minRGB, RGBColor maxRGB)")},
	{"VDSetInputColorSpaceMode", (PyCFunction)Qt_VDSetInputColorSpaceMode, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short colorSpaceMode) -> (ComponentResult _rv)")},
	{"VDGetInputColorSpaceMode", (PyCFunction)Qt_VDGetInputColorSpaceMode, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, short colorSpaceMode)")},
	{"VDSetClipState", (PyCFunction)Qt_VDSetClipState, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short clipEnable) -> (ComponentResult _rv)")},
	{"VDGetClipState", (PyCFunction)Qt_VDGetClipState, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, short clipEnable)")},
	{"VDSetClipRgn", (PyCFunction)Qt_VDSetClipRgn, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, RgnHandle clipRegion) -> (ComponentResult _rv)")},
	{"VDClearClipRgn", (PyCFunction)Qt_VDClearClipRgn, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, RgnHandle clipRegion) -> (ComponentResult _rv)")},
	{"VDGetCLUTInUse", (PyCFunction)Qt_VDGetCLUTInUse, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, CTabHandle colorTableHandle)")},
	{"VDSetPLLFilterType", (PyCFunction)Qt_VDSetPLLFilterType, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short pllType) -> (ComponentResult _rv)")},
	{"VDGetPLLFilterType", (PyCFunction)Qt_VDGetPLLFilterType, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, short pllType)")},
	{"VDGetMaskandValue", (PyCFunction)Qt_VDGetMaskandValue, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, unsigned short blendLevel) -> (ComponentResult _rv, long mask, long value)")},
	{"VDSetMasterBlendLevel", (PyCFunction)Qt_VDSetMasterBlendLevel, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short blendLevel)")},
	{"VDSetPlayThruOnOff", (PyCFunction)Qt_VDSetPlayThruOnOff, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short state) -> (ComponentResult _rv)")},
	{"VDSetFieldPreference", (PyCFunction)Qt_VDSetFieldPreference, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short fieldFlag) -> (ComponentResult _rv)")},
	{"VDGetFieldPreference", (PyCFunction)Qt_VDGetFieldPreference, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, short fieldFlag)")},
	{"VDPreflightGlobalRect", (PyCFunction)Qt_VDPreflightGlobalRect, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, GrafPtr theWindow) -> (ComponentResult _rv, Rect globalRect)")},
	{"VDSetPlayThruGlobalRect", (PyCFunction)Qt_VDSetPlayThruGlobalRect, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, GrafPtr theWindow) -> (ComponentResult _rv, Rect globalRect)")},
	{"VDSetBlackLevelValue", (PyCFunction)Qt_VDSetBlackLevelValue, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short blackLevel)")},
	{"VDGetBlackLevelValue", (PyCFunction)Qt_VDGetBlackLevelValue, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short blackLevel)")},
	{"VDSetWhiteLevelValue", (PyCFunction)Qt_VDSetWhiteLevelValue, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short whiteLevel)")},
	{"VDGetWhiteLevelValue", (PyCFunction)Qt_VDGetWhiteLevelValue, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short whiteLevel)")},
	{"VDGetVideoDefaults", (PyCFunction)Qt_VDGetVideoDefaults, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, unsigned short blackLevel, unsigned short whiteLevel, unsigned short brightness, unsigned short hue, unsigned short saturation, unsigned short contrast, unsigned short sharpness)")},
	{"VDGetNumberOfInputs", (PyCFunction)Qt_VDGetNumberOfInputs, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, short inputs)")},
	{"VDGetInputFormat", (PyCFunction)Qt_VDGetInputFormat, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short input) -> (ComponentResult _rv, short format)")},
	{"VDSetInput", (PyCFunction)Qt_VDSetInput, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short input) -> (ComponentResult _rv)")},
	{"VDGetInput", (PyCFunction)Qt_VDGetInput, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, short input)")},
	{"VDSetInputStandard", (PyCFunction)Qt_VDSetInputStandard, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short inputStandard) -> (ComponentResult _rv)")},
	{"VDSetupBuffers", (PyCFunction)Qt_VDSetupBuffers, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, VdigBufferRecListHandle bufferList) -> (ComponentResult _rv)")},
	{"VDGrabOneFrameAsync", (PyCFunction)Qt_VDGrabOneFrameAsync, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short buffer) -> (ComponentResult _rv)")},
	{"VDDone", (PyCFunction)Qt_VDDone, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, short buffer) -> (ComponentResult _rv)")},
	{"VDSetCompression", (PyCFunction)Qt_VDSetCompression, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, OSType compressType, short depth, CodecQ spatialQuality, CodecQ temporalQuality, long keyFrameRate) -> (ComponentResult _rv, Rect bounds)")},
	{"VDCompressOneFrameAsync", (PyCFunction)Qt_VDCompressOneFrameAsync, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv)")},
	{"VDGetImageDescription", (PyCFunction)Qt_VDGetImageDescription, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, ImageDescriptionHandle desc) -> (ComponentResult _rv)")},
	{"VDResetCompressSequence", (PyCFunction)Qt_VDResetCompressSequence, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv)")},
	{"VDSetCompressionOnOff", (PyCFunction)Qt_VDSetCompressionOnOff, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, Boolean state) -> (ComponentResult _rv)")},
	{"VDGetCompressionTypes", (PyCFunction)Qt_VDGetCompressionTypes, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, VDCompressionListHandle h) -> (ComponentResult _rv)")},
	{"VDSetTimeBase", (PyCFunction)Qt_VDSetTimeBase, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, TimeBase t) -> (ComponentResult _rv)")},
	{"VDSetFrameRate", (PyCFunction)Qt_VDSetFrameRate, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, Fixed framesPerSecond) -> (ComponentResult _rv)")},
	{"VDGetDataRate", (PyCFunction)Qt_VDGetDataRate, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, long milliSecPerFrame, Fixed framesPerSecond, long bytesPerSecond)")},
	{"VDGetSoundInputDriver", (PyCFunction)Qt_VDGetSoundInputDriver, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, Str255 soundDriverName) -> (ComponentResult _rv)")},
	{"VDGetDMADepths", (PyCFunction)Qt_VDGetDMADepths, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, long depthArray, long preferredDepth)")},
	{"VDGetPreferredTimeScale", (PyCFunction)Qt_VDGetPreferredTimeScale, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, TimeScale preferred)")},
	{"VDReleaseAsyncBuffers", (PyCFunction)Qt_VDReleaseAsyncBuffers, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv)")},
	{"VDSetDataRate", (PyCFunction)Qt_VDSetDataRate, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, long bytesPerSecond) -> (ComponentResult _rv)")},
	{"VDGetTimeCode", (PyCFunction)Qt_VDGetTimeCode, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, void * timeCodeFormat, void * timeCodeTime) -> (ComponentResult _rv, TimeRecord atTime)")},
	{"VDUseSafeBuffers", (PyCFunction)Qt_VDUseSafeBuffers, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, Boolean useSafeBuffers) -> (ComponentResult _rv)")},
	{"VDGetSoundInputSource", (PyCFunction)Qt_VDGetSoundInputSource, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, long videoInput) -> (ComponentResult _rv, long soundInput)")},
	{"VDGetCompressionTime", (PyCFunction)Qt_VDGetCompressionTime, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, OSType compressionType, short depth) -> (ComponentResult _rv, Rect srcRect, CodecQ spatialQuality, CodecQ temporalQuality, unsigned long compressTime)")},
	{"VDSetPreferredPacketSize", (PyCFunction)Qt_VDSetPreferredPacketSize, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, long preferredPacketSizeInBytes) -> (ComponentResult _rv)")},
	{"VDSetPreferredImageDimensions", (PyCFunction)Qt_VDSetPreferredImageDimensions, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, long width, long height) -> (ComponentResult _rv)")},
	{"VDGetPreferredImageDimensions", (PyCFunction)Qt_VDGetPreferredImageDimensions, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci) -> (ComponentResult _rv, long width, long height)")},
	{"VDGetInputName", (PyCFunction)Qt_VDGetInputName, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, long videoInput, Str255 name) -> (ComponentResult _rv)")},
	{"VDSetDestinationPort", (PyCFunction)Qt_VDSetDestinationPort, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, CGrafPtr destPort) -> (ComponentResult _rv)")},
	{"VDGetDeviceNameAndFlags", (PyCFunction)Qt_VDGetDeviceNameAndFlags, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, Str255 outName) -> (ComponentResult _rv, UInt32 outNameFlags)")},
	{"VDCaptureStateChanging", (PyCFunction)Qt_VDCaptureStateChanging, 1,
	 PyDoc_STR("(VideoDigitizerComponent ci, UInt32 inStateFlags) -> (ComponentResult _rv)")},
	{"XMLParseGetDetailedParseError", (PyCFunction)Qt_XMLParseGetDetailedParseError, 1,
	 PyDoc_STR("(ComponentInstance aParser, StringPtr errDesc) -> (ComponentResult _rv, long errorLine)")},
	{"XMLParseAddElement", (PyCFunction)Qt_XMLParseAddElement, 1,
	 PyDoc_STR("(ComponentInstance aParser, UInt32 nameSpaceID, long elementFlags) -> (ComponentResult _rv, char elementName, UInt32 elementID)")},
	{"XMLParseAddAttribute", (PyCFunction)Qt_XMLParseAddAttribute, 1,
	 PyDoc_STR("(ComponentInstance aParser, UInt32 elementID, UInt32 nameSpaceID) -> (ComponentResult _rv, char attributeName, UInt32 attributeID)")},
	{"XMLParseAddMultipleAttributes", (PyCFunction)Qt_XMLParseAddMultipleAttributes, 1,
	 PyDoc_STR("(ComponentInstance aParser, UInt32 elementID) -> (ComponentResult _rv, UInt32 nameSpaceIDs, char attributeNames, UInt32 attributeIDs)")},
	{"XMLParseAddAttributeAndValue", (PyCFunction)Qt_XMLParseAddAttributeAndValue, 1,
	 PyDoc_STR("(ComponentInstance aParser, UInt32 elementID, UInt32 nameSpaceID, UInt32 attributeValueKind, void * attributeValueKindInfo) -> (ComponentResult _rv, char attributeName, UInt32 attributeID)")},
	{"XMLParseAddAttributeValueKind", (PyCFunction)Qt_XMLParseAddAttributeValueKind, 1,
	 PyDoc_STR("(ComponentInstance aParser, UInt32 elementID, UInt32 attributeID, UInt32 attributeValueKind, void * attributeValueKindInfo) -> (ComponentResult _rv)")},
	{"XMLParseAddNameSpace", (PyCFunction)Qt_XMLParseAddNameSpace, 1,
	 PyDoc_STR("(ComponentInstance aParser) -> (ComponentResult _rv, char nameSpaceURL, UInt32 nameSpaceID)")},
	{"XMLParseSetOffsetAndLimit", (PyCFunction)Qt_XMLParseSetOffsetAndLimit, 1,
	 PyDoc_STR("(ComponentInstance aParser, UInt32 offset, UInt32 limit) -> (ComponentResult _rv)")},
	{"XMLParseSetEventParseRefCon", (PyCFunction)Qt_XMLParseSetEventParseRefCon, 1,
	 PyDoc_STR("(ComponentInstance aParser, long refcon) -> (ComponentResult _rv)")},
	{"SGInitialize", (PyCFunction)Qt_SGInitialize, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv)")},
	{"SGSetDataOutput", (PyCFunction)Qt_SGSetDataOutput, 1,
	 PyDoc_STR("(SeqGrabComponent s, FSSpec movieFile, long whereFlags) -> (ComponentResult _rv)")},
	{"SGGetDataOutput", (PyCFunction)Qt_SGGetDataOutput, 1,
	 PyDoc_STR("(SeqGrabComponent s, FSSpec movieFile) -> (ComponentResult _rv, long whereFlags)")},
	{"SGSetGWorld", (PyCFunction)Qt_SGSetGWorld, 1,
	 PyDoc_STR("(SeqGrabComponent s, CGrafPtr gp, GDHandle gd) -> (ComponentResult _rv)")},
	{"SGGetGWorld", (PyCFunction)Qt_SGGetGWorld, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, CGrafPtr gp, GDHandle gd)")},
	{"SGNewChannel", (PyCFunction)Qt_SGNewChannel, 1,
	 PyDoc_STR("(SeqGrabComponent s, OSType channelType) -> (ComponentResult _rv, SGChannel ref)")},
	{"SGDisposeChannel", (PyCFunction)Qt_SGDisposeChannel, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c) -> (ComponentResult _rv)")},
	{"SGStartPreview", (PyCFunction)Qt_SGStartPreview, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv)")},
	{"SGStartRecord", (PyCFunction)Qt_SGStartRecord, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv)")},
	{"SGIdle", (PyCFunction)Qt_SGIdle, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv)")},
	{"SGStop", (PyCFunction)Qt_SGStop, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv)")},
	{"SGPause", (PyCFunction)Qt_SGPause, 1,
	 PyDoc_STR("(SeqGrabComponent s, Boolean pause) -> (ComponentResult _rv)")},
	{"SGPrepare", (PyCFunction)Qt_SGPrepare, 1,
	 PyDoc_STR("(SeqGrabComponent s, Boolean prepareForPreview, Boolean prepareForRecord) -> (ComponentResult _rv)")},
	{"SGRelease", (PyCFunction)Qt_SGRelease, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv)")},
	{"SGGetMovie", (PyCFunction)Qt_SGGetMovie, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (Movie _rv)")},
	{"SGSetMaximumRecordTime", (PyCFunction)Qt_SGSetMaximumRecordTime, 1,
	 PyDoc_STR("(SeqGrabComponent s, unsigned long ticks) -> (ComponentResult _rv)")},
	{"SGGetMaximumRecordTime", (PyCFunction)Qt_SGGetMaximumRecordTime, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, unsigned long ticks)")},
	{"SGGetStorageSpaceRemaining", (PyCFunction)Qt_SGGetStorageSpaceRemaining, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, unsigned long bytes)")},
	{"SGGetTimeRemaining", (PyCFunction)Qt_SGGetTimeRemaining, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, long ticksLeft)")},
	{"SGGrabPict", (PyCFunction)Qt_SGGrabPict, 1,
	 PyDoc_STR("(SeqGrabComponent s, Rect bounds, short offscreenDepth, long grabPictFlags) -> (ComponentResult _rv, PicHandle p)")},
	{"SGGetLastMovieResID", (PyCFunction)Qt_SGGetLastMovieResID, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, short resID)")},
	{"SGSetFlags", (PyCFunction)Qt_SGSetFlags, 1,
	 PyDoc_STR("(SeqGrabComponent s, long sgFlags) -> (ComponentResult _rv)")},
	{"SGGetFlags", (PyCFunction)Qt_SGGetFlags, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, long sgFlags)")},
	{"SGNewChannelFromComponent", (PyCFunction)Qt_SGNewChannelFromComponent, 1,
	 PyDoc_STR("(SeqGrabComponent s, Component sgChannelComponent) -> (ComponentResult _rv, SGChannel newChannel)")},
	{"SGSetSettings", (PyCFunction)Qt_SGSetSettings, 1,
	 PyDoc_STR("(SeqGrabComponent s, UserData ud, long flags) -> (ComponentResult _rv)")},
	{"SGGetSettings", (PyCFunction)Qt_SGGetSettings, 1,
	 PyDoc_STR("(SeqGrabComponent s, long flags) -> (ComponentResult _rv, UserData ud)")},
	{"SGGetIndChannel", (PyCFunction)Qt_SGGetIndChannel, 1,
	 PyDoc_STR("(SeqGrabComponent s, short index) -> (ComponentResult _rv, SGChannel ref, OSType chanType)")},
	{"SGUpdate", (PyCFunction)Qt_SGUpdate, 1,
	 PyDoc_STR("(SeqGrabComponent s, RgnHandle updateRgn) -> (ComponentResult _rv)")},
	{"SGGetPause", (PyCFunction)Qt_SGGetPause, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, Boolean paused)")},
	{"SGSetChannelSettings", (PyCFunction)Qt_SGSetChannelSettings, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, UserData ud, long flags) -> (ComponentResult _rv)")},
	{"SGGetChannelSettings", (PyCFunction)Qt_SGGetChannelSettings, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, long flags) -> (ComponentResult _rv, UserData ud)")},
	{"SGGetMode", (PyCFunction)Qt_SGGetMode, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, Boolean previewMode, Boolean recordMode)")},
	{"SGSetDataRef", (PyCFunction)Qt_SGSetDataRef, 1,
	 PyDoc_STR("(SeqGrabComponent s, Handle dataRef, OSType dataRefType, long whereFlags) -> (ComponentResult _rv)")},
	{"SGGetDataRef", (PyCFunction)Qt_SGGetDataRef, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, Handle dataRef, OSType dataRefType, long whereFlags)")},
	{"SGNewOutput", (PyCFunction)Qt_SGNewOutput, 1,
	 PyDoc_STR("(SeqGrabComponent s, Handle dataRef, OSType dataRefType, long whereFlags) -> (ComponentResult _rv, SGOutput sgOut)")},
	{"SGDisposeOutput", (PyCFunction)Qt_SGDisposeOutput, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGOutput sgOut) -> (ComponentResult _rv)")},
	{"SGSetOutputFlags", (PyCFunction)Qt_SGSetOutputFlags, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGOutput sgOut, long whereFlags) -> (ComponentResult _rv)")},
	{"SGSetChannelOutput", (PyCFunction)Qt_SGSetChannelOutput, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, SGOutput sgOut) -> (ComponentResult _rv)")},
	{"SGGetDataOutputStorageSpaceRemaining", (PyCFunction)Qt_SGGetDataOutputStorageSpaceRemaining, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGOutput sgOut) -> (ComponentResult _rv, unsigned long space)")},
	{"SGHandleUpdateEvent", (PyCFunction)Qt_SGHandleUpdateEvent, 1,
	 PyDoc_STR("(SeqGrabComponent s, EventRecord event) -> (ComponentResult _rv, Boolean handled)")},
	{"SGSetOutputNextOutput", (PyCFunction)Qt_SGSetOutputNextOutput, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGOutput sgOut, SGOutput nextOut) -> (ComponentResult _rv)")},
	{"SGGetOutputNextOutput", (PyCFunction)Qt_SGGetOutputNextOutput, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGOutput sgOut) -> (ComponentResult _rv, SGOutput nextOut)")},
	{"SGSetOutputMaximumOffset", (PyCFunction)Qt_SGSetOutputMaximumOffset, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGOutput sgOut, wide maxOffset) -> (ComponentResult _rv)")},
	{"SGGetOutputMaximumOffset", (PyCFunction)Qt_SGGetOutputMaximumOffset, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGOutput sgOut) -> (ComponentResult _rv, wide maxOffset)")},
	{"SGGetOutputDataReference", (PyCFunction)Qt_SGGetOutputDataReference, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGOutput sgOut) -> (ComponentResult _rv, Handle dataRef, OSType dataRefType)")},
	{"SGWriteExtendedMovieData", (PyCFunction)Qt_SGWriteExtendedMovieData, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, Ptr p, long len) -> (ComponentResult _rv, wide offset, SGOutput sgOut)")},
	{"SGGetStorageSpaceRemaining64", (PyCFunction)Qt_SGGetStorageSpaceRemaining64, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, wide bytes)")},
	{"SGGetDataOutputStorageSpaceRemaining64", (PyCFunction)Qt_SGGetDataOutputStorageSpaceRemaining64, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGOutput sgOut) -> (ComponentResult _rv, wide space)")},
	{"SGWriteMovieData", (PyCFunction)Qt_SGWriteMovieData, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, Ptr p, long len) -> (ComponentResult _rv, long offset)")},
	{"SGGetTimeBase", (PyCFunction)Qt_SGGetTimeBase, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, TimeBase tb)")},
	{"SGAddMovieData", (PyCFunction)Qt_SGAddMovieData, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, Ptr p, long len, long chRefCon, TimeValue time, short writeType) -> (ComponentResult _rv, long offset)")},
	{"SGChangedSource", (PyCFunction)Qt_SGChangedSource, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c) -> (ComponentResult _rv)")},
	{"SGAddExtendedMovieData", (PyCFunction)Qt_SGAddExtendedMovieData, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, Ptr p, long len, long chRefCon, TimeValue time, short writeType) -> (ComponentResult _rv, wide offset, SGOutput whichOutput)")},
	{"SGAddOutputDataRefToMedia", (PyCFunction)Qt_SGAddOutputDataRefToMedia, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGOutput sgOut, Media theMedia, SampleDescriptionHandle desc) -> (ComponentResult _rv)")},
	{"SGSetSettingsSummary", (PyCFunction)Qt_SGSetSettingsSummary, 1,
	 PyDoc_STR("(SeqGrabComponent s, Handle summaryText) -> (ComponentResult _rv)")},
	{"SGSetChannelUsage", (PyCFunction)Qt_SGSetChannelUsage, 1,
	 PyDoc_STR("(SGChannel c, long usage) -> (ComponentResult _rv)")},
	{"SGGetChannelUsage", (PyCFunction)Qt_SGGetChannelUsage, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, long usage)")},
	{"SGSetChannelBounds", (PyCFunction)Qt_SGSetChannelBounds, 1,
	 PyDoc_STR("(SGChannel c, Rect bounds) -> (ComponentResult _rv)")},
	{"SGGetChannelBounds", (PyCFunction)Qt_SGGetChannelBounds, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, Rect bounds)")},
	{"SGSetChannelVolume", (PyCFunction)Qt_SGSetChannelVolume, 1,
	 PyDoc_STR("(SGChannel c, short volume) -> (ComponentResult _rv)")},
	{"SGGetChannelVolume", (PyCFunction)Qt_SGGetChannelVolume, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, short volume)")},
	{"SGGetChannelInfo", (PyCFunction)Qt_SGGetChannelInfo, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, long channelInfo)")},
	{"SGSetChannelPlayFlags", (PyCFunction)Qt_SGSetChannelPlayFlags, 1,
	 PyDoc_STR("(SGChannel c, long playFlags) -> (ComponentResult _rv)")},
	{"SGGetChannelPlayFlags", (PyCFunction)Qt_SGGetChannelPlayFlags, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, long playFlags)")},
	{"SGSetChannelMaxFrames", (PyCFunction)Qt_SGSetChannelMaxFrames, 1,
	 PyDoc_STR("(SGChannel c, long frameCount) -> (ComponentResult _rv)")},
	{"SGGetChannelMaxFrames", (PyCFunction)Qt_SGGetChannelMaxFrames, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, long frameCount)")},
	{"SGSetChannelRefCon", (PyCFunction)Qt_SGSetChannelRefCon, 1,
	 PyDoc_STR("(SGChannel c, long refCon) -> (ComponentResult _rv)")},
	{"SGSetChannelClip", (PyCFunction)Qt_SGSetChannelClip, 1,
	 PyDoc_STR("(SGChannel c, RgnHandle theClip) -> (ComponentResult _rv)")},
	{"SGGetChannelClip", (PyCFunction)Qt_SGGetChannelClip, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, RgnHandle theClip)")},
	{"SGGetChannelSampleDescription", (PyCFunction)Qt_SGGetChannelSampleDescription, 1,
	 PyDoc_STR("(SGChannel c, Handle sampleDesc) -> (ComponentResult _rv)")},
	{"SGSetChannelDevice", (PyCFunction)Qt_SGSetChannelDevice, 1,
	 PyDoc_STR("(SGChannel c, StringPtr name) -> (ComponentResult _rv)")},
	{"SGGetChannelTimeScale", (PyCFunction)Qt_SGGetChannelTimeScale, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, TimeScale scale)")},
	{"SGChannelPutPicture", (PyCFunction)Qt_SGChannelPutPicture, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv)")},
	{"SGChannelSetRequestedDataRate", (PyCFunction)Qt_SGChannelSetRequestedDataRate, 1,
	 PyDoc_STR("(SGChannel c, long bytesPerSecond) -> (ComponentResult _rv)")},
	{"SGChannelGetRequestedDataRate", (PyCFunction)Qt_SGChannelGetRequestedDataRate, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, long bytesPerSecond)")},
	{"SGChannelSetDataSourceName", (PyCFunction)Qt_SGChannelSetDataSourceName, 1,
	 PyDoc_STR("(SGChannel c, Str255 name, ScriptCode scriptTag) -> (ComponentResult _rv)")},
	{"SGChannelGetDataSourceName", (PyCFunction)Qt_SGChannelGetDataSourceName, 1,
	 PyDoc_STR("(SGChannel c, Str255 name) -> (ComponentResult _rv, ScriptCode scriptTag)")},
	{"SGChannelSetCodecSettings", (PyCFunction)Qt_SGChannelSetCodecSettings, 1,
	 PyDoc_STR("(SGChannel c, Handle settings) -> (ComponentResult _rv)")},
	{"SGChannelGetCodecSettings", (PyCFunction)Qt_SGChannelGetCodecSettings, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, Handle settings)")},
	{"SGGetChannelTimeBase", (PyCFunction)Qt_SGGetChannelTimeBase, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, TimeBase tb)")},
	{"SGGetChannelRefCon", (PyCFunction)Qt_SGGetChannelRefCon, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, long refCon)")},
	{"SGGetChannelDeviceAndInputNames", (PyCFunction)Qt_SGGetChannelDeviceAndInputNames, 1,
	 PyDoc_STR("(SGChannel c, Str255 outDeviceName, Str255 outInputName) -> (ComponentResult _rv, short outInputNumber)")},
	{"SGSetChannelDeviceInput", (PyCFunction)Qt_SGSetChannelDeviceInput, 1,
	 PyDoc_STR("(SGChannel c, short inInputNumber) -> (ComponentResult _rv)")},
	{"SGSetChannelSettingsStateChanging", (PyCFunction)Qt_SGSetChannelSettingsStateChanging, 1,
	 PyDoc_STR("(SGChannel c, UInt32 inFlags) -> (ComponentResult _rv)")},
	{"SGInitChannel", (PyCFunction)Qt_SGInitChannel, 1,
	 PyDoc_STR("(SGChannel c, SeqGrabComponent owner) -> (ComponentResult _rv)")},
	{"SGWriteSamples", (PyCFunction)Qt_SGWriteSamples, 1,
	 PyDoc_STR("(SGChannel c, Movie m, AliasHandle theFile) -> (ComponentResult _rv)")},
	{"SGGetDataRate", (PyCFunction)Qt_SGGetDataRate, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, long bytesPerSecond)")},
	{"SGAlignChannelRect", (PyCFunction)Qt_SGAlignChannelRect, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, Rect r)")},
	{"SGPanelGetDitl", (PyCFunction)Qt_SGPanelGetDitl, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, Handle ditl)")},
	{"SGPanelGetTitle", (PyCFunction)Qt_SGPanelGetTitle, 1,
	 PyDoc_STR("(SeqGrabComponent s, Str255 title) -> (ComponentResult _rv)")},
	{"SGPanelCanRun", (PyCFunction)Qt_SGPanelCanRun, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c) -> (ComponentResult _rv)")},
	{"SGPanelInstall", (PyCFunction)Qt_SGPanelInstall, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, DialogPtr d, short itemOffset) -> (ComponentResult _rv)")},
	{"SGPanelEvent", (PyCFunction)Qt_SGPanelEvent, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, DialogPtr d, short itemOffset, EventRecord theEvent) -> (ComponentResult _rv, short itemHit, Boolean handled)")},
	{"SGPanelItem", (PyCFunction)Qt_SGPanelItem, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, DialogPtr d, short itemOffset, short itemNum) -> (ComponentResult _rv)")},
	{"SGPanelRemove", (PyCFunction)Qt_SGPanelRemove, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, DialogPtr d, short itemOffset) -> (ComponentResult _rv)")},
	{"SGPanelSetGrabber", (PyCFunction)Qt_SGPanelSetGrabber, 1,
	 PyDoc_STR("(SeqGrabComponent s, SeqGrabComponent sg) -> (ComponentResult _rv)")},
	{"SGPanelSetResFile", (PyCFunction)Qt_SGPanelSetResFile, 1,
	 PyDoc_STR("(SeqGrabComponent s, short resRef) -> (ComponentResult _rv)")},
	{"SGPanelGetSettings", (PyCFunction)Qt_SGPanelGetSettings, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, long flags) -> (ComponentResult _rv, UserData ud)")},
	{"SGPanelSetSettings", (PyCFunction)Qt_SGPanelSetSettings, 1,
	 PyDoc_STR("(SeqGrabComponent s, SGChannel c, UserData ud, long flags) -> (ComponentResult _rv)")},
	{"SGPanelValidateInput", (PyCFunction)Qt_SGPanelValidateInput, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, Boolean ok)")},
	{"SGPanelGetDITLForSize", (PyCFunction)Qt_SGPanelGetDITLForSize, 1,
	 PyDoc_STR("(SeqGrabComponent s) -> (ComponentResult _rv, Handle ditl, Point requestedSize)")},
	{"SGGetSrcVideoBounds", (PyCFunction)Qt_SGGetSrcVideoBounds, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, Rect r)")},
	{"SGSetVideoRect", (PyCFunction)Qt_SGSetVideoRect, 1,
	 PyDoc_STR("(SGChannel c, Rect r) -> (ComponentResult _rv)")},
	{"SGGetVideoRect", (PyCFunction)Qt_SGGetVideoRect, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, Rect r)")},
	{"SGGetVideoCompressorType", (PyCFunction)Qt_SGGetVideoCompressorType, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, OSType compressorType)")},
	{"SGSetVideoCompressorType", (PyCFunction)Qt_SGSetVideoCompressorType, 1,
	 PyDoc_STR("(SGChannel c, OSType compressorType) -> (ComponentResult _rv)")},
	{"SGSetVideoCompressor", (PyCFunction)Qt_SGSetVideoCompressor, 1,
	 PyDoc_STR("(SGChannel c, short depth, CompressorComponent compressor, CodecQ spatialQuality, CodecQ temporalQuality, long keyFrameRate) -> (ComponentResult _rv)")},
	{"SGGetVideoCompressor", (PyCFunction)Qt_SGGetVideoCompressor, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, short depth, CompressorComponent compressor, CodecQ spatialQuality, CodecQ temporalQuality, long keyFrameRate)")},
	{"SGGetVideoDigitizerComponent", (PyCFunction)Qt_SGGetVideoDigitizerComponent, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentInstance _rv)")},
	{"SGSetVideoDigitizerComponent", (PyCFunction)Qt_SGSetVideoDigitizerComponent, 1,
	 PyDoc_STR("(SGChannel c, ComponentInstance vdig) -> (ComponentResult _rv)")},
	{"SGVideoDigitizerChanged", (PyCFunction)Qt_SGVideoDigitizerChanged, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv)")},
	{"SGGrabFrame", (PyCFunction)Qt_SGGrabFrame, 1,
	 PyDoc_STR("(SGChannel c, short bufferNum) -> (ComponentResult _rv)")},
	{"SGGrabFrameComplete", (PyCFunction)Qt_SGGrabFrameComplete, 1,
	 PyDoc_STR("(SGChannel c, short bufferNum) -> (ComponentResult _rv, Boolean done)")},
	{"SGCompressFrame", (PyCFunction)Qt_SGCompressFrame, 1,
	 PyDoc_STR("(SGChannel c, short bufferNum) -> (ComponentResult _rv)")},
	{"SGSetCompressBuffer", (PyCFunction)Qt_SGSetCompressBuffer, 1,
	 PyDoc_STR("(SGChannel c, short depth, Rect compressSize) -> (ComponentResult _rv)")},
	{"SGGetCompressBuffer", (PyCFunction)Qt_SGGetCompressBuffer, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, short depth, Rect compressSize)")},
	{"SGGetBufferInfo", (PyCFunction)Qt_SGGetBufferInfo, 1,
	 PyDoc_STR("(SGChannel c, short bufferNum) -> (ComponentResult _rv, PixMapHandle bufferPM, Rect bufferRect, GWorldPtr compressBuffer, Rect compressBufferRect)")},
	{"SGSetUseScreenBuffer", (PyCFunction)Qt_SGSetUseScreenBuffer, 1,
	 PyDoc_STR("(SGChannel c, Boolean useScreenBuffer) -> (ComponentResult _rv)")},
	{"SGGetUseScreenBuffer", (PyCFunction)Qt_SGGetUseScreenBuffer, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, Boolean useScreenBuffer)")},
	{"SGSetFrameRate", (PyCFunction)Qt_SGSetFrameRate, 1,
	 PyDoc_STR("(SGChannel c, Fixed frameRate) -> (ComponentResult _rv)")},
	{"SGGetFrameRate", (PyCFunction)Qt_SGGetFrameRate, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, Fixed frameRate)")},
	{"SGSetPreferredPacketSize", (PyCFunction)Qt_SGSetPreferredPacketSize, 1,
	 PyDoc_STR("(SGChannel c, long preferredPacketSizeInBytes) -> (ComponentResult _rv)")},
	{"SGGetPreferredPacketSize", (PyCFunction)Qt_SGGetPreferredPacketSize, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, long preferredPacketSizeInBytes)")},
	{"SGSetUserVideoCompressorList", (PyCFunction)Qt_SGSetUserVideoCompressorList, 1,
	 PyDoc_STR("(SGChannel c, Handle compressorTypes) -> (ComponentResult _rv)")},
	{"SGGetUserVideoCompressorList", (PyCFunction)Qt_SGGetUserVideoCompressorList, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, Handle compressorTypes)")},
	{"SGSetSoundInputDriver", (PyCFunction)Qt_SGSetSoundInputDriver, 1,
	 PyDoc_STR("(SGChannel c, Str255 driverName) -> (ComponentResult _rv)")},
	{"SGGetSoundInputDriver", (PyCFunction)Qt_SGGetSoundInputDriver, 1,
	 PyDoc_STR("(SGChannel c) -> (long _rv)")},
	{"SGSoundInputDriverChanged", (PyCFunction)Qt_SGSoundInputDriverChanged, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv)")},
	{"SGSetSoundRecordChunkSize", (PyCFunction)Qt_SGSetSoundRecordChunkSize, 1,
	 PyDoc_STR("(SGChannel c, long seconds) -> (ComponentResult _rv)")},
	{"SGGetSoundRecordChunkSize", (PyCFunction)Qt_SGGetSoundRecordChunkSize, 1,
	 PyDoc_STR("(SGChannel c) -> (long _rv)")},
	{"SGSetSoundInputRate", (PyCFunction)Qt_SGSetSoundInputRate, 1,
	 PyDoc_STR("(SGChannel c, Fixed rate) -> (ComponentResult _rv)")},
	{"SGGetSoundInputRate", (PyCFunction)Qt_SGGetSoundInputRate, 1,
	 PyDoc_STR("(SGChannel c) -> (Fixed _rv)")},
	{"SGSetSoundInputParameters", (PyCFunction)Qt_SGSetSoundInputParameters, 1,
	 PyDoc_STR("(SGChannel c, short sampleSize, short numChannels, OSType compressionType) -> (ComponentResult _rv)")},
	{"SGGetSoundInputParameters", (PyCFunction)Qt_SGGetSoundInputParameters, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, short sampleSize, short numChannels, OSType compressionType)")},
	{"SGSetAdditionalSoundRates", (PyCFunction)Qt_SGSetAdditionalSoundRates, 1,
	 PyDoc_STR("(SGChannel c, Handle rates) -> (ComponentResult _rv)")},
	{"SGGetAdditionalSoundRates", (PyCFunction)Qt_SGGetAdditionalSoundRates, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, Handle rates)")},
	{"SGSetFontName", (PyCFunction)Qt_SGSetFontName, 1,
	 PyDoc_STR("(SGChannel c, StringPtr pstr) -> (ComponentResult _rv)")},
	{"SGSetFontSize", (PyCFunction)Qt_SGSetFontSize, 1,
	 PyDoc_STR("(SGChannel c, short fontSize) -> (ComponentResult _rv)")},
	{"SGSetTextForeColor", (PyCFunction)Qt_SGSetTextForeColor, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, RGBColor theColor)")},
	{"SGSetTextBackColor", (PyCFunction)Qt_SGSetTextBackColor, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, RGBColor theColor)")},
	{"SGSetJustification", (PyCFunction)Qt_SGSetJustification, 1,
	 PyDoc_STR("(SGChannel c, short just) -> (ComponentResult _rv)")},
	{"SGGetTextReturnToSpaceValue", (PyCFunction)Qt_SGGetTextReturnToSpaceValue, 1,
	 PyDoc_STR("(SGChannel c) -> (ComponentResult _rv, short rettospace)")},
	{"SGSetTextReturnToSpaceValue", (PyCFunction)Qt_SGSetTextReturnToSpaceValue, 1,
	 PyDoc_STR("(SGChannel c, short rettospace) -> (ComponentResult _rv)")},
	{"QTVideoOutputGetCurrentClientName", (PyCFunction)Qt_QTVideoOutputGetCurrentClientName, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo, Str255 str) -> (ComponentResult _rv)")},
	{"QTVideoOutputSetClientName", (PyCFunction)Qt_QTVideoOutputSetClientName, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo, Str255 str) -> (ComponentResult _rv)")},
	{"QTVideoOutputGetClientName", (PyCFunction)Qt_QTVideoOutputGetClientName, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo, Str255 str) -> (ComponentResult _rv)")},
	{"QTVideoOutputBegin", (PyCFunction)Qt_QTVideoOutputBegin, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo) -> (ComponentResult _rv)")},
	{"QTVideoOutputEnd", (PyCFunction)Qt_QTVideoOutputEnd, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo) -> (ComponentResult _rv)")},
	{"QTVideoOutputSetDisplayMode", (PyCFunction)Qt_QTVideoOutputSetDisplayMode, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo, long displayModeID) -> (ComponentResult _rv)")},
	{"QTVideoOutputGetDisplayMode", (PyCFunction)Qt_QTVideoOutputGetDisplayMode, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo) -> (ComponentResult _rv, long displayModeID)")},
	{"QTVideoOutputGetGWorld", (PyCFunction)Qt_QTVideoOutputGetGWorld, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo) -> (ComponentResult _rv, GWorldPtr gw)")},
	{"QTVideoOutputGetIndSoundOutput", (PyCFunction)Qt_QTVideoOutputGetIndSoundOutput, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo, long index) -> (ComponentResult _rv, Component outputComponent)")},
	{"QTVideoOutputGetClock", (PyCFunction)Qt_QTVideoOutputGetClock, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo) -> (ComponentResult _rv, ComponentInstance clock)")},
	{"QTVideoOutputSetEchoPort", (PyCFunction)Qt_QTVideoOutputSetEchoPort, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo, CGrafPtr echoPort) -> (ComponentResult _rv)")},
	{"QTVideoOutputGetIndImageDecompressor", (PyCFunction)Qt_QTVideoOutputGetIndImageDecompressor, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo, long index) -> (ComponentResult _rv, Component codec)")},
	{"QTVideoOutputBaseSetEchoPort", (PyCFunction)Qt_QTVideoOutputBaseSetEchoPort, 1,
	 PyDoc_STR("(QTVideoOutputComponent vo, CGrafPtr echoPort) -> (ComponentResult _rv)")},
	{"MediaSetChunkManagementFlags", (PyCFunction)Qt_MediaSetChunkManagementFlags, 1,
	 PyDoc_STR("(MediaHandler mh, UInt32 flags, UInt32 flagsMask) -> (ComponentResult _rv)")},
	{"MediaGetChunkManagementFlags", (PyCFunction)Qt_MediaGetChunkManagementFlags, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, UInt32 flags)")},
	{"MediaSetPurgeableChunkMemoryAllowance", (PyCFunction)Qt_MediaSetPurgeableChunkMemoryAllowance, 1,
	 PyDoc_STR("(MediaHandler mh, Size allowance) -> (ComponentResult _rv)")},
	{"MediaGetPurgeableChunkMemoryAllowance", (PyCFunction)Qt_MediaGetPurgeableChunkMemoryAllowance, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, Size allowance)")},
	{"MediaEmptyAllPurgeableChunks", (PyCFunction)Qt_MediaEmptyAllPurgeableChunks, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv)")},
	{"MediaSetHandlerCapabilities", (PyCFunction)Qt_MediaSetHandlerCapabilities, 1,
	 PyDoc_STR("(MediaHandler mh, long flags, long flagsMask) -> (ComponentResult _rv)")},
	{"MediaIdle", (PyCFunction)Qt_MediaIdle, 1,
	 PyDoc_STR("(MediaHandler mh, TimeValue atMediaTime, long flagsIn, TimeRecord movieTime) -> (ComponentResult _rv, long flagsOut)")},
	{"MediaGetMediaInfo", (PyCFunction)Qt_MediaGetMediaInfo, 1,
	 PyDoc_STR("(MediaHandler mh, Handle h) -> (ComponentResult _rv)")},
	{"MediaPutMediaInfo", (PyCFunction)Qt_MediaPutMediaInfo, 1,
	 PyDoc_STR("(MediaHandler mh, Handle h) -> (ComponentResult _rv)")},
	{"MediaSetActive", (PyCFunction)Qt_MediaSetActive, 1,
	 PyDoc_STR("(MediaHandler mh, Boolean enableMedia) -> (ComponentResult _rv)")},
	{"MediaSetRate", (PyCFunction)Qt_MediaSetRate, 1,
	 PyDoc_STR("(MediaHandler mh, Fixed rate) -> (ComponentResult _rv)")},
	{"MediaGGetStatus", (PyCFunction)Qt_MediaGGetStatus, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, ComponentResult statusErr)")},
	{"MediaTrackEdited", (PyCFunction)Qt_MediaTrackEdited, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv)")},
	{"MediaSetMediaTimeScale", (PyCFunction)Qt_MediaSetMediaTimeScale, 1,
	 PyDoc_STR("(MediaHandler mh, TimeScale newTimeScale) -> (ComponentResult _rv)")},
	{"MediaSetMovieTimeScale", (PyCFunction)Qt_MediaSetMovieTimeScale, 1,
	 PyDoc_STR("(MediaHandler mh, TimeScale newTimeScale) -> (ComponentResult _rv)")},
	{"MediaSetGWorld", (PyCFunction)Qt_MediaSetGWorld, 1,
	 PyDoc_STR("(MediaHandler mh, CGrafPtr aPort, GDHandle aGD) -> (ComponentResult _rv)")},
	{"MediaSetDimensions", (PyCFunction)Qt_MediaSetDimensions, 1,
	 PyDoc_STR("(MediaHandler mh, Fixed width, Fixed height) -> (ComponentResult _rv)")},
	{"MediaSetClip", (PyCFunction)Qt_MediaSetClip, 1,
	 PyDoc_STR("(MediaHandler mh, RgnHandle theClip) -> (ComponentResult _rv)")},
	{"MediaGetTrackOpaque", (PyCFunction)Qt_MediaGetTrackOpaque, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, Boolean trackIsOpaque)")},
	{"MediaSetGraphicsMode", (PyCFunction)Qt_MediaSetGraphicsMode, 1,
	 PyDoc_STR("(MediaHandler mh, long mode, RGBColor opColor) -> (ComponentResult _rv)")},
	{"MediaGetGraphicsMode", (PyCFunction)Qt_MediaGetGraphicsMode, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, long mode, RGBColor opColor)")},
	{"MediaGSetVolume", (PyCFunction)Qt_MediaGSetVolume, 1,
	 PyDoc_STR("(MediaHandler mh, short volume) -> (ComponentResult _rv)")},
	{"MediaSetSoundBalance", (PyCFunction)Qt_MediaSetSoundBalance, 1,
	 PyDoc_STR("(MediaHandler mh, short balance) -> (ComponentResult _rv)")},
	{"MediaGetSoundBalance", (PyCFunction)Qt_MediaGetSoundBalance, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, short balance)")},
	{"MediaGetNextBoundsChange", (PyCFunction)Qt_MediaGetNextBoundsChange, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, TimeValue when)")},
	{"MediaGetSrcRgn", (PyCFunction)Qt_MediaGetSrcRgn, 1,
	 PyDoc_STR("(MediaHandler mh, RgnHandle rgn, TimeValue atMediaTime) -> (ComponentResult _rv)")},
	{"MediaPreroll", (PyCFunction)Qt_MediaPreroll, 1,
	 PyDoc_STR("(MediaHandler mh, TimeValue time, Fixed rate) -> (ComponentResult _rv)")},
	{"MediaSampleDescriptionChanged", (PyCFunction)Qt_MediaSampleDescriptionChanged, 1,
	 PyDoc_STR("(MediaHandler mh, long index) -> (ComponentResult _rv)")},
	{"MediaHasCharacteristic", (PyCFunction)Qt_MediaHasCharacteristic, 1,
	 PyDoc_STR("(MediaHandler mh, OSType characteristic) -> (ComponentResult _rv, Boolean hasIt)")},
	{"MediaGetOffscreenBufferSize", (PyCFunction)Qt_MediaGetOffscreenBufferSize, 1,
	 PyDoc_STR("(MediaHandler mh, short depth, CTabHandle ctab) -> (ComponentResult _rv, Rect bounds)")},
	{"MediaSetHints", (PyCFunction)Qt_MediaSetHints, 1,
	 PyDoc_STR("(MediaHandler mh, long hints) -> (ComponentResult _rv)")},
	{"MediaGetName", (PyCFunction)Qt_MediaGetName, 1,
	 PyDoc_STR("(MediaHandler mh, Str255 name, long requestedLanguage) -> (ComponentResult _rv, long actualLanguage)")},
	{"MediaForceUpdate", (PyCFunction)Qt_MediaForceUpdate, 1,
	 PyDoc_STR("(MediaHandler mh, long forceUpdateFlags) -> (ComponentResult _rv)")},
	{"MediaGetDrawingRgn", (PyCFunction)Qt_MediaGetDrawingRgn, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, RgnHandle partialRgn)")},
	{"MediaGSetActiveSegment", (PyCFunction)Qt_MediaGSetActiveSegment, 1,
	 PyDoc_STR("(MediaHandler mh, TimeValue activeStart, TimeValue activeDuration) -> (ComponentResult _rv)")},
	{"MediaInvalidateRegion", (PyCFunction)Qt_MediaInvalidateRegion, 1,
	 PyDoc_STR("(MediaHandler mh, RgnHandle invalRgn) -> (ComponentResult _rv)")},
	{"MediaGetNextStepTime", (PyCFunction)Qt_MediaGetNextStepTime, 1,
	 PyDoc_STR("(MediaHandler mh, short flags, TimeValue mediaTimeIn, Fixed rate) -> (ComponentResult _rv, TimeValue mediaTimeOut)")},
	{"MediaChangedNonPrimarySource", (PyCFunction)Qt_MediaChangedNonPrimarySource, 1,
	 PyDoc_STR("(MediaHandler mh, long inputIndex) -> (ComponentResult _rv)")},
	{"MediaTrackReferencesChanged", (PyCFunction)Qt_MediaTrackReferencesChanged, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv)")},
	{"MediaReleaseSampleDataPointer", (PyCFunction)Qt_MediaReleaseSampleDataPointer, 1,
	 PyDoc_STR("(MediaHandler mh, long sampleNum) -> (ComponentResult _rv)")},
	{"MediaTrackPropertyAtomChanged", (PyCFunction)Qt_MediaTrackPropertyAtomChanged, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv)")},
	{"MediaSetVideoParam", (PyCFunction)Qt_MediaSetVideoParam, 1,
	 PyDoc_STR("(MediaHandler mh, long whichParam) -> (ComponentResult _rv, unsigned short value)")},
	{"MediaGetVideoParam", (PyCFunction)Qt_MediaGetVideoParam, 1,
	 PyDoc_STR("(MediaHandler mh, long whichParam) -> (ComponentResult _rv, unsigned short value)")},
	{"MediaCompare", (PyCFunction)Qt_MediaCompare, 1,
	 PyDoc_STR("(MediaHandler mh, Media srcMedia, ComponentInstance srcMediaComponent) -> (ComponentResult _rv, Boolean isOK)")},
	{"MediaGetClock", (PyCFunction)Qt_MediaGetClock, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, ComponentInstance clock)")},
	{"MediaSetSoundOutputComponent", (PyCFunction)Qt_MediaSetSoundOutputComponent, 1,
	 PyDoc_STR("(MediaHandler mh, Component outputComponent) -> (ComponentResult _rv)")},
	{"MediaGetSoundOutputComponent", (PyCFunction)Qt_MediaGetSoundOutputComponent, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, Component outputComponent)")},
	{"MediaSetSoundLocalizationData", (PyCFunction)Qt_MediaSetSoundLocalizationData, 1,
	 PyDoc_STR("(MediaHandler mh, Handle data) -> (ComponentResult _rv)")},
	{"MediaGetInvalidRegion", (PyCFunction)Qt_MediaGetInvalidRegion, 1,
	 PyDoc_STR("(MediaHandler mh, RgnHandle rgn) -> (ComponentResult _rv)")},
	{"MediaSampleDescriptionB2N", (PyCFunction)Qt_MediaSampleDescriptionB2N, 1,
	 PyDoc_STR("(MediaHandler mh, SampleDescriptionHandle sampleDescriptionH) -> (ComponentResult _rv)")},
	{"MediaSampleDescriptionN2B", (PyCFunction)Qt_MediaSampleDescriptionN2B, 1,
	 PyDoc_STR("(MediaHandler mh, SampleDescriptionHandle sampleDescriptionH) -> (ComponentResult _rv)")},
	{"MediaFlushNonPrimarySourceData", (PyCFunction)Qt_MediaFlushNonPrimarySourceData, 1,
	 PyDoc_STR("(MediaHandler mh, long inputIndex) -> (ComponentResult _rv)")},
	{"MediaGetURLLink", (PyCFunction)Qt_MediaGetURLLink, 1,
	 PyDoc_STR("(MediaHandler mh, Point displayWhere) -> (ComponentResult _rv, Handle urlLink)")},
	{"MediaHitTestForTargetRefCon", (PyCFunction)Qt_MediaHitTestForTargetRefCon, 1,
	 PyDoc_STR("(MediaHandler mh, long flags, Point loc) -> (ComponentResult _rv, long targetRefCon)")},
	{"MediaHitTestTargetRefCon", (PyCFunction)Qt_MediaHitTestTargetRefCon, 1,
	 PyDoc_STR("(MediaHandler mh, long targetRefCon, long flags, Point loc) -> (ComponentResult _rv, Boolean wasHit)")},
	{"MediaDisposeTargetRefCon", (PyCFunction)Qt_MediaDisposeTargetRefCon, 1,
	 PyDoc_STR("(MediaHandler mh, long targetRefCon) -> (ComponentResult _rv)")},
	{"MediaTargetRefConsEqual", (PyCFunction)Qt_MediaTargetRefConsEqual, 1,
	 PyDoc_STR("(MediaHandler mh, long firstRefCon, long secondRefCon) -> (ComponentResult _rv, Boolean equal)")},
	{"MediaPrePrerollCancel", (PyCFunction)Qt_MediaPrePrerollCancel, 1,
	 PyDoc_STR("(MediaHandler mh, void * refcon) -> (ComponentResult _rv)")},
	{"MediaEnterEmptyEdit", (PyCFunction)Qt_MediaEnterEmptyEdit, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv)")},
	{"MediaCurrentMediaQueuedData", (PyCFunction)Qt_MediaCurrentMediaQueuedData, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, long milliSecs)")},
	{"MediaGetEffectiveVolume", (PyCFunction)Qt_MediaGetEffectiveVolume, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, short volume)")},
	{"MediaGetSoundLevelMeteringEnabled", (PyCFunction)Qt_MediaGetSoundLevelMeteringEnabled, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, Boolean enabled)")},
	{"MediaSetSoundLevelMeteringEnabled", (PyCFunction)Qt_MediaSetSoundLevelMeteringEnabled, 1,
	 PyDoc_STR("(MediaHandler mh, Boolean enable) -> (ComponentResult _rv)")},
	{"MediaGetEffectiveSoundBalance", (PyCFunction)Qt_MediaGetEffectiveSoundBalance, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, short balance)")},
	{"MediaSetScreenLock", (PyCFunction)Qt_MediaSetScreenLock, 1,
	 PyDoc_STR("(MediaHandler mh, Boolean lockIt) -> (ComponentResult _rv)")},
	{"MediaGetErrorString", (PyCFunction)Qt_MediaGetErrorString, 1,
	 PyDoc_STR("(MediaHandler mh, ComponentResult theError, Str255 errorString) -> (ComponentResult _rv)")},
	{"MediaGetSoundEqualizerBandLevels", (PyCFunction)Qt_MediaGetSoundEqualizerBandLevels, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, UInt8 bandLevels)")},
	{"MediaDoIdleActions", (PyCFunction)Qt_MediaDoIdleActions, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv)")},
	{"MediaSetSoundBassAndTreble", (PyCFunction)Qt_MediaSetSoundBassAndTreble, 1,
	 PyDoc_STR("(MediaHandler mh, short bass, short treble) -> (ComponentResult _rv)")},
	{"MediaGetSoundBassAndTreble", (PyCFunction)Qt_MediaGetSoundBassAndTreble, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, short bass, short treble)")},
	{"MediaTimeBaseChanged", (PyCFunction)Qt_MediaTimeBaseChanged, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv)")},
	{"MediaMCIsPlayerEvent", (PyCFunction)Qt_MediaMCIsPlayerEvent, 1,
	 PyDoc_STR("(MediaHandler mh, EventRecord e) -> (ComponentResult _rv, Boolean handledIt)")},
	{"MediaGetMediaLoadState", (PyCFunction)Qt_MediaGetMediaLoadState, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, long mediaLoadState)")},
	{"MediaVideoOutputChanged", (PyCFunction)Qt_MediaVideoOutputChanged, 1,
	 PyDoc_STR("(MediaHandler mh, ComponentInstance vout) -> (ComponentResult _rv)")},
	{"MediaEmptySampleCache", (PyCFunction)Qt_MediaEmptySampleCache, 1,
	 PyDoc_STR("(MediaHandler mh, long sampleNum, long sampleCount) -> (ComponentResult _rv)")},
	{"MediaGetPublicInfo", (PyCFunction)Qt_MediaGetPublicInfo, 1,
	 PyDoc_STR("(MediaHandler mh, OSType infoSelector, void * infoDataPtr) -> (ComponentResult _rv, Size ioDataSize)")},
	{"MediaSetPublicInfo", (PyCFunction)Qt_MediaSetPublicInfo, 1,
	 PyDoc_STR("(MediaHandler mh, OSType infoSelector, void * infoDataPtr, Size dataSize) -> (ComponentResult _rv)")},
	{"MediaRefConSetProperty", (PyCFunction)Qt_MediaRefConSetProperty, 1,
	 PyDoc_STR("(MediaHandler mh, long refCon, long propertyType, void * propertyValue) -> (ComponentResult _rv)")},
	{"MediaRefConGetProperty", (PyCFunction)Qt_MediaRefConGetProperty, 1,
	 PyDoc_STR("(MediaHandler mh, long refCon, long propertyType, void * propertyValue) -> (ComponentResult _rv)")},
	{"MediaNavigateTargetRefCon", (PyCFunction)Qt_MediaNavigateTargetRefCon, 1,
	 PyDoc_STR("(MediaHandler mh, long navigation) -> (ComponentResult _rv, long refCon)")},
	{"MediaGGetIdleManager", (PyCFunction)Qt_MediaGGetIdleManager, 1,
	 PyDoc_STR("(MediaHandler mh) -> (ComponentResult _rv, IdleManager pim)")},
	{"MediaGSetIdleManager", (PyCFunction)Qt_MediaGSetIdleManager, 1,
	 PyDoc_STR("(MediaHandler mh, IdleManager im) -> (ComponentResult _rv)")},
	{"QTMIDIGetMIDIPorts", (PyCFunction)Qt_QTMIDIGetMIDIPorts, 1,
	 PyDoc_STR("(QTMIDIComponent ci) -> (ComponentResult _rv, QTMIDIPortListHandle inputPorts, QTMIDIPortListHandle outputPorts)")},
	{"QTMIDIUseSendPort", (PyCFunction)Qt_QTMIDIUseSendPort, 1,
	 PyDoc_STR("(QTMIDIComponent ci, long portIndex, long inUse) -> (ComponentResult _rv)")},
	{"QTMIDISendMIDI", (PyCFunction)Qt_QTMIDISendMIDI, 1,
	 PyDoc_STR("(QTMIDIComponent ci, long portIndex, MusicMIDIPacket mp) -> (ComponentResult _rv)")},
	{"MusicGetPart", (PyCFunction)Qt_MusicGetPart, 1,
	 PyDoc_STR("(MusicComponent mc, long part) -> (ComponentResult _rv, long midiChannel, long polyphony)")},
	{"MusicSetPart", (PyCFunction)Qt_MusicSetPart, 1,
	 PyDoc_STR("(MusicComponent mc, long part, long midiChannel, long polyphony) -> (ComponentResult _rv)")},
	{"MusicSetPartInstrumentNumber", (PyCFunction)Qt_MusicSetPartInstrumentNumber, 1,
	 PyDoc_STR("(MusicComponent mc, long part, long instrumentNumber) -> (ComponentResult _rv)")},
	{"MusicGetPartInstrumentNumber", (PyCFunction)Qt_MusicGetPartInstrumentNumber, 1,
	 PyDoc_STR("(MusicComponent mc, long part) -> (ComponentResult _rv)")},
	{"MusicStorePartInstrument", (PyCFunction)Qt_MusicStorePartInstrument, 1,
	 PyDoc_STR("(MusicComponent mc, long part, long instrumentNumber) -> (ComponentResult _rv)")},
	{"MusicGetPartAtomicInstrument", (PyCFunction)Qt_MusicGetPartAtomicInstrument, 1,
	 PyDoc_STR("(MusicComponent mc, long part, long flags) -> (ComponentResult _rv, AtomicInstrument ai)")},
	{"MusicSetPartAtomicInstrument", (PyCFunction)Qt_MusicSetPartAtomicInstrument, 1,
	 PyDoc_STR("(MusicComponent mc, long part, AtomicInstrumentPtr aiP, long flags) -> (ComponentResult _rv)")},
	{"MusicGetPartKnob", (PyCFunction)Qt_MusicGetPartKnob, 1,
	 PyDoc_STR("(MusicComponent mc, long part, long knobID) -> (ComponentResult _rv)")},
	{"MusicSetPartKnob", (PyCFunction)Qt_MusicSetPartKnob, 1,
	 PyDoc_STR("(MusicComponent mc, long part, long knobID, long knobValue) -> (ComponentResult _rv)")},
	{"MusicGetKnob", (PyCFunction)Qt_MusicGetKnob, 1,
	 PyDoc_STR("(MusicComponent mc, long knobID) -> (ComponentResult _rv)")},
	{"MusicSetKnob", (PyCFunction)Qt_MusicSetKnob, 1,
	 PyDoc_STR("(MusicComponent mc, long knobID, long knobValue) -> (ComponentResult _rv)")},
	{"MusicGetPartName", (PyCFunction)Qt_MusicGetPartName, 1,
	 PyDoc_STR("(MusicComponent mc, long part, StringPtr name) -> (ComponentResult _rv)")},
	{"MusicSetPartName", (PyCFunction)Qt_MusicSetPartName, 1,
	 PyDoc_STR("(MusicComponent mc, long part, StringPtr name) -> (ComponentResult _rv)")},
	{"MusicPlayNote", (PyCFunction)Qt_MusicPlayNote, 1,
	 PyDoc_STR("(MusicComponent mc, long part, long pitch, long velocity) -> (ComponentResult _rv)")},
	{"MusicResetPart", (PyCFunction)Qt_MusicResetPart, 1,
	 PyDoc_STR("(MusicComponent mc, long part) -> (ComponentResult _rv)")},
	{"MusicSetPartController", (PyCFunction)Qt_MusicSetPartController, 1,
	 PyDoc_STR("(MusicComponent mc, long part, MusicController controllerNumber, long controllerValue) -> (ComponentResult _rv)")},
	{"MusicGetPartController", (PyCFunction)Qt_MusicGetPartController, 1,
	 PyDoc_STR("(MusicComponent mc, long part, MusicController controllerNumber) -> (ComponentResult _rv)")},
	{"MusicGetInstrumentNames", (PyCFunction)Qt_MusicGetInstrumentNames, 1,
	 PyDoc_STR("(MusicComponent mc, long modifiableInstruments) -> (ComponentResult _rv, Handle instrumentNames, Handle instrumentCategoryLasts, Handle instrumentCategoryNames)")},
	{"MusicGetDrumNames", (PyCFunction)Qt_MusicGetDrumNames, 1,
	 PyDoc_STR("(MusicComponent mc, long modifiableInstruments) -> (ComponentResult _rv, Handle instrumentNumbers, Handle instrumentNames)")},
	{"MusicGetMasterTune", (PyCFunction)Qt_MusicGetMasterTune, 1,
	 PyDoc_STR("(MusicComponent mc) -> (ComponentResult _rv)")},
	{"MusicSetMasterTune", (PyCFunction)Qt_MusicSetMasterTune, 1,
	 PyDoc_STR("(MusicComponent mc, long masterTune) -> (ComponentResult _rv)")},
	{"MusicGetDeviceConnection", (PyCFunction)Qt_MusicGetDeviceConnection, 1,
	 PyDoc_STR("(MusicComponent mc, long index) -> (ComponentResult _rv, long id1, long id2)")},
	{"MusicUseDeviceConnection", (PyCFunction)Qt_MusicUseDeviceConnection, 1,
	 PyDoc_STR("(MusicComponent mc, long id1, long id2) -> (ComponentResult _rv)")},
	{"MusicGetKnobSettingStrings", (PyCFunction)Qt_MusicGetKnobSettingStrings, 1,
	 PyDoc_STR("(MusicComponent mc, long knobIndex, long isGlobal) -> (ComponentResult _rv, Handle settingsNames, Handle settingsCategoryLasts, Handle settingsCategoryNames)")},
	{"MusicGetMIDIPorts", (PyCFunction)Qt_MusicGetMIDIPorts, 1,
	 PyDoc_STR("(MusicComponent mc) -> (ComponentResult _rv, long inputPortCount, long outputPortCount)")},
	{"MusicSendMIDI", (PyCFunction)Qt_MusicSendMIDI, 1,
	 PyDoc_STR("(MusicComponent mc, long portIndex, MusicMIDIPacket mp) -> (ComponentResult _rv)")},
	{"MusicSetOfflineTimeTo", (PyCFunction)Qt_MusicSetOfflineTimeTo, 1,
	 PyDoc_STR("(MusicComponent mc, long newTimeStamp) -> (ComponentResult _rv)")},
	{"MusicGetInfoText", (PyCFunction)Qt_MusicGetInfoText, 1,
	 PyDoc_STR("(MusicComponent mc, long selector) -> (ComponentResult _rv, Handle textH, Handle styleH)")},
	{"MusicGetInstrumentInfo", (PyCFunction)Qt_MusicGetInstrumentInfo, 1,
	 PyDoc_STR("(MusicComponent mc, long getInstrumentInfoFlags) -> (ComponentResult _rv, InstrumentInfoListHandle infoListH)")},
	{"MusicTask", (PyCFunction)Qt_MusicTask, 1,
	 PyDoc_STR("(MusicComponent mc) -> (ComponentResult _rv)")},
	{"MusicSetPartInstrumentNumberInterruptSafe", (PyCFunction)Qt_MusicSetPartInstrumentNumberInterruptSafe, 1,
	 PyDoc_STR("(MusicComponent mc, long part, long instrumentNumber) -> (ComponentResult _rv)")},
	{"MusicSetPartSoundLocalization", (PyCFunction)Qt_MusicSetPartSoundLocalization, 1,
	 PyDoc_STR("(MusicComponent mc, long part, Handle data) -> (ComponentResult _rv)")},
	{"MusicGenericConfigure", (PyCFunction)Qt_MusicGenericConfigure, 1,
	 PyDoc_STR("(MusicComponent mc, long mode, long flags, long baseResID) -> (ComponentResult _rv)")},
	{"MusicGenericGetKnobList", (PyCFunction)Qt_MusicGenericGetKnobList, 1,
	 PyDoc_STR("(MusicComponent mc, long knobType) -> (ComponentResult _rv, GenericKnobDescriptionListHandle gkdlH)")},
	{"MusicGenericSetResourceNumbers", (PyCFunction)Qt_MusicGenericSetResourceNumbers, 1,
	 PyDoc_STR("(MusicComponent mc, Handle resourceIDH) -> (ComponentResult _rv)")},
	{"MusicDerivedMIDISend", (PyCFunction)Qt_MusicDerivedMIDISend, 1,
	 PyDoc_STR("(MusicComponent mc, MusicMIDIPacket packet) -> (ComponentResult _rv)")},
	{"MusicDerivedOpenResFile", (PyCFunction)Qt_MusicDerivedOpenResFile, 1,
	 PyDoc_STR("(MusicComponent mc) -> (ComponentResult _rv)")},
	{"MusicDerivedCloseResFile", (PyCFunction)Qt_MusicDerivedCloseResFile, 1,
	 PyDoc_STR("(MusicComponent mc, short resRefNum) -> (ComponentResult _rv)")},
	{"NAUnregisterMusicDevice", (PyCFunction)Qt_NAUnregisterMusicDevice, 1,
	 PyDoc_STR("(NoteAllocator na, long index) -> (ComponentResult _rv)")},
	{"NASaveMusicConfiguration", (PyCFunction)Qt_NASaveMusicConfiguration, 1,
	 PyDoc_STR("(NoteAllocator na) -> (ComponentResult _rv)")},
	{"NAGetMIDIPorts", (PyCFunction)Qt_NAGetMIDIPorts, 1,
	 PyDoc_STR("(NoteAllocator na) -> (ComponentResult _rv, QTMIDIPortListHandle inputPorts, QTMIDIPortListHandle outputPorts)")},
	{"NATask", (PyCFunction)Qt_NATask, 1,
	 PyDoc_STR("(NoteAllocator na) -> (ComponentResult _rv)")},
	{"TuneSetHeader", (PyCFunction)Qt_TuneSetHeader, 1,
	 PyDoc_STR("(TunePlayer tp, unsigned long * header) -> (ComponentResult _rv)")},
	{"TuneGetTimeBase", (PyCFunction)Qt_TuneGetTimeBase, 1,
	 PyDoc_STR("(TunePlayer tp) -> (ComponentResult _rv, TimeBase tb)")},
	{"TuneSetTimeScale", (PyCFunction)Qt_TuneSetTimeScale, 1,
	 PyDoc_STR("(TunePlayer tp, TimeScale scale) -> (ComponentResult _rv)")},
	{"TuneGetTimeScale", (PyCFunction)Qt_TuneGetTimeScale, 1,
	 PyDoc_STR("(TunePlayer tp) -> (ComponentResult _rv, TimeScale scale)")},
	{"TuneInstant", (PyCFunction)Qt_TuneInstant, 1,
	 PyDoc_STR("(TunePlayer tp, unsigned long tunePosition) -> (ComponentResult _rv, unsigned long tune)")},
	{"TuneStop", (PyCFunction)Qt_TuneStop, 1,
	 PyDoc_STR("(TunePlayer tp, long stopFlags) -> (ComponentResult _rv)")},
	{"TuneSetVolume", (PyCFunction)Qt_TuneSetVolume, 1,
	 PyDoc_STR("(TunePlayer tp, Fixed volume) -> (ComponentResult _rv)")},
	{"TuneGetVolume", (PyCFunction)Qt_TuneGetVolume, 1,
	 PyDoc_STR("(TunePlayer tp) -> (ComponentResult _rv)")},
	{"TunePreroll", (PyCFunction)Qt_TunePreroll, 1,
	 PyDoc_STR("(TunePlayer tp) -> (ComponentResult _rv)")},
	{"TuneUnroll", (PyCFunction)Qt_TuneUnroll, 1,
	 PyDoc_STR("(TunePlayer tp) -> (ComponentResult _rv)")},
	{"TuneSetPartTranspose", (PyCFunction)Qt_TuneSetPartTranspose, 1,
	 PyDoc_STR("(TunePlayer tp, unsigned long part, long transpose, long velocityShift) -> (ComponentResult _rv)")},
	{"TuneGetNoteAllocator", (PyCFunction)Qt_TuneGetNoteAllocator, 1,
	 PyDoc_STR("(TunePlayer tp) -> (NoteAllocator _rv)")},
	{"TuneSetSofter", (PyCFunction)Qt_TuneSetSofter, 1,
	 PyDoc_STR("(TunePlayer tp, long softer) -> (ComponentResult _rv)")},
	{"TuneTask", (PyCFunction)Qt_TuneTask, 1,
	 PyDoc_STR("(TunePlayer tp) -> (ComponentResult _rv)")},
	{"TuneSetBalance", (PyCFunction)Qt_TuneSetBalance, 1,
	 PyDoc_STR("(TunePlayer tp, long balance) -> (ComponentResult _rv)")},
	{"TuneSetSoundLocalization", (PyCFunction)Qt_TuneSetSoundLocalization, 1,
	 PyDoc_STR("(TunePlayer tp, Handle data) -> (ComponentResult _rv)")},
	{"TuneSetHeaderWithSize", (PyCFunction)Qt_TuneSetHeaderWithSize, 1,
	 PyDoc_STR("(TunePlayer tp, unsigned long * header, unsigned long size) -> (ComponentResult _rv)")},
	{"TuneSetPartMix", (PyCFunction)Qt_TuneSetPartMix, 1,
	 PyDoc_STR("(TunePlayer tp, unsigned long partNumber, long volume, long balance, long mixFlags) -> (ComponentResult _rv)")},
	{"TuneGetPartMix", (PyCFunction)Qt_TuneGetPartMix, 1,
	 PyDoc_STR("(TunePlayer tp, unsigned long partNumber) -> (ComponentResult _rv, long volumeOut, long balanceOut, long mixFlagsOut)")},
	{"AlignWindow", (PyCFunction)Qt_AlignWindow, 1,
	 PyDoc_STR("(WindowPtr wp, Boolean front) -> None")},
	{"DragAlignedWindow", (PyCFunction)Qt_DragAlignedWindow, 1,
	 PyDoc_STR("(WindowPtr wp, Point startPt, Rect boundsRect) -> None")},
	{"MoviesTask", (PyCFunction)Qt_MoviesTask, 1,
	 PyDoc_STR("(long maxMilliSecToUse) -> None")},
#endif /* __LP64__ */
	{NULL, NULL, 0}
};




void init_Qt(void)
{
	PyObject *m;
#ifndef __LP64__
	PyObject *d;



	        PyMac_INIT_TOOLBOX_OBJECT_NEW(Track, TrackObj_New);
	        PyMac_INIT_TOOLBOX_OBJECT_CONVERT(Track, TrackObj_Convert);
	        PyMac_INIT_TOOLBOX_OBJECT_NEW(Movie, MovieObj_New);
	        PyMac_INIT_TOOLBOX_OBJECT_CONVERT(Movie, MovieObj_Convert);
	        PyMac_INIT_TOOLBOX_OBJECT_NEW(MovieController, MovieCtlObj_New);
	        PyMac_INIT_TOOLBOX_OBJECT_CONVERT(MovieController, MovieCtlObj_Convert);
	        PyMac_INIT_TOOLBOX_OBJECT_NEW(TimeBase, TimeBaseObj_New);
	        PyMac_INIT_TOOLBOX_OBJECT_CONVERT(TimeBase, TimeBaseObj_Convert);
	        PyMac_INIT_TOOLBOX_OBJECT_NEW(UserData, UserDataObj_New);
	        PyMac_INIT_TOOLBOX_OBJECT_CONVERT(UserData, UserDataObj_Convert);
	        PyMac_INIT_TOOLBOX_OBJECT_NEW(Media, MediaObj_New);
	        PyMac_INIT_TOOLBOX_OBJECT_CONVERT(Media, MediaObj_Convert);
#endif /* __LP64__ */


	m = Py_InitModule("_Qt", Qt_methods);
#ifndef __LP64__
	d = PyModule_GetDict(m);
	Qt_Error = PyMac_GetOSErrException();
	if (Qt_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Qt_Error) != 0)
		return;
	IdleManager_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&IdleManager_Type) < 0) return;
	Py_INCREF(&IdleManager_Type);
	PyModule_AddObject(m, "IdleManager", (PyObject *)&IdleManager_Type);
	/* Backward-compatible name */
	Py_INCREF(&IdleManager_Type);
	PyModule_AddObject(m, "IdleManagerType", (PyObject *)&IdleManager_Type);
	MovieController_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&MovieController_Type) < 0) return;
	Py_INCREF(&MovieController_Type);
	PyModule_AddObject(m, "MovieController", (PyObject *)&MovieController_Type);
	/* Backward-compatible name */
	Py_INCREF(&MovieController_Type);
	PyModule_AddObject(m, "MovieControllerType", (PyObject *)&MovieController_Type);
	TimeBase_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&TimeBase_Type) < 0) return;
	Py_INCREF(&TimeBase_Type);
	PyModule_AddObject(m, "TimeBase", (PyObject *)&TimeBase_Type);
	/* Backward-compatible name */
	Py_INCREF(&TimeBase_Type);
	PyModule_AddObject(m, "TimeBaseType", (PyObject *)&TimeBase_Type);
	UserData_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&UserData_Type) < 0) return;
	Py_INCREF(&UserData_Type);
	PyModule_AddObject(m, "UserData", (PyObject *)&UserData_Type);
	/* Backward-compatible name */
	Py_INCREF(&UserData_Type);
	PyModule_AddObject(m, "UserDataType", (PyObject *)&UserData_Type);
	Media_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&Media_Type) < 0) return;
	Py_INCREF(&Media_Type);
	PyModule_AddObject(m, "Media", (PyObject *)&Media_Type);
	/* Backward-compatible name */
	Py_INCREF(&Media_Type);
	PyModule_AddObject(m, "MediaType", (PyObject *)&Media_Type);
	Track_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&Track_Type) < 0) return;
	Py_INCREF(&Track_Type);
	PyModule_AddObject(m, "Track", (PyObject *)&Track_Type);
	/* Backward-compatible name */
	Py_INCREF(&Track_Type);
	PyModule_AddObject(m, "TrackType", (PyObject *)&Track_Type);
	Movie_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&Movie_Type) < 0) return;
	Py_INCREF(&Movie_Type);
	PyModule_AddObject(m, "Movie", (PyObject *)&Movie_Type);
	/* Backward-compatible name */
	Py_INCREF(&Movie_Type);
	PyModule_AddObject(m, "MovieType", (PyObject *)&Movie_Type);
	SGOutput_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&SGOutput_Type) < 0) return;
	Py_INCREF(&SGOutput_Type);
	PyModule_AddObject(m, "SGOutput", (PyObject *)&SGOutput_Type);
	/* Backward-compatible name */
	Py_INCREF(&SGOutput_Type);
	PyModule_AddObject(m, "SGOutputType", (PyObject *)&SGOutput_Type);
#endif /* __LP64__ */
}

/* ========================= End module _Qt ========================= */

