
/* =========================== Module _Qt =========================== */

#include "Python.h"



#ifdef _WIN32
#include "pywintoolbox.h"
#else
#include "macglue.h"
#include "pymactoolbox.h"
#endif

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    	PyErr_SetString(PyExc_NotImplementedError, \
    	"Not available in this shared library/OS version"); \
    	return NULL; \
    }} while(0)


#ifdef WITHOUT_FRAMEWORKS
#include <Movies.h>
#else
/* #include <Carbon/Carbon.h> */
#include <QuickTime/QuickTime.h>
#endif


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
#define GetMediaNextInterestingTimeOnly(media, flags, time, rate, rv) 			GetMediaNextInterestingTime(media, flags, time, rate, rv, NULL)
			
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




static PyObject *Qt_Error;

/* ------------------ Object type MovieController ------------------- */

PyTypeObject MovieController_Type;

#define MovieCtlObj_Check(x) ((x)->ob_type == &MovieController_Type)

typedef struct MovieControllerObject {
	PyObject_HEAD
	MovieController ob_itself;
} MovieControllerObject;

PyObject *MovieCtlObj_New(MovieController itself)
{
	MovieControllerObject *it;
	if (itself == NULL) {
						PyErr_SetString(Qt_Error,"Cannot create null MovieController");
						return NULL;
					}
	it = PyObject_NEW(MovieControllerObject, &MovieController_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
int MovieCtlObj_Convert(PyObject *v, MovieController *p_itself)
{
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
	DisposeMovieController(self->ob_itself);
	PyObject_Del(self);
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
	{NULL, NULL, 0}
};

PyMethodChain MovieCtlObj_chain = { MovieCtlObj_methods, NULL };

static PyObject *MovieCtlObj_getattr(MovieControllerObject *self, char *name)
{
	return Py_FindMethodInChain(&MovieCtlObj_chain, (PyObject *)self, name);
}

#define MovieCtlObj_setattr NULL

#define MovieCtlObj_compare NULL

#define MovieCtlObj_repr NULL

#define MovieCtlObj_hash NULL

PyTypeObject MovieController_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.MovieController", /*tp_name*/
	sizeof(MovieControllerObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) MovieCtlObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) MovieCtlObj_getattr, /*tp_getattr*/
	(setattrfunc) MovieCtlObj_setattr, /*tp_setattr*/
	(cmpfunc) MovieCtlObj_compare, /*tp_compare*/
	(reprfunc) MovieCtlObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) MovieCtlObj_hash, /*tp_hash*/
};

/* ---------------- End object type MovieController ----------------- */


/* ---------------------- Object type TimeBase ---------------------- */

PyTypeObject TimeBase_Type;

#define TimeBaseObj_Check(x) ((x)->ob_type == &TimeBase_Type)

typedef struct TimeBaseObject {
	PyObject_HEAD
	TimeBase ob_itself;
} TimeBaseObject;

PyObject *TimeBaseObj_New(TimeBase itself)
{
	TimeBaseObject *it;
	if (itself == NULL) {
						PyErr_SetString(Qt_Error,"Cannot create null TimeBase");
						return NULL;
					}
	it = PyObject_NEW(TimeBaseObject, &TimeBase_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
int TimeBaseObj_Convert(PyObject *v, TimeBase *p_itself)
{
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
	PyObject_Del(self);
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

PyMethodChain TimeBaseObj_chain = { TimeBaseObj_methods, NULL };

static PyObject *TimeBaseObj_getattr(TimeBaseObject *self, char *name)
{
	return Py_FindMethodInChain(&TimeBaseObj_chain, (PyObject *)self, name);
}

#define TimeBaseObj_setattr NULL

#define TimeBaseObj_compare NULL

#define TimeBaseObj_repr NULL

#define TimeBaseObj_hash NULL

PyTypeObject TimeBase_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.TimeBase", /*tp_name*/
	sizeof(TimeBaseObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) TimeBaseObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) TimeBaseObj_getattr, /*tp_getattr*/
	(setattrfunc) TimeBaseObj_setattr, /*tp_setattr*/
	(cmpfunc) TimeBaseObj_compare, /*tp_compare*/
	(reprfunc) TimeBaseObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) TimeBaseObj_hash, /*tp_hash*/
};

/* -------------------- End object type TimeBase -------------------- */


/* ---------------------- Object type UserData ---------------------- */

PyTypeObject UserData_Type;

#define UserDataObj_Check(x) ((x)->ob_type == &UserData_Type)

typedef struct UserDataObject {
	PyObject_HEAD
	UserData ob_itself;
} UserDataObject;

PyObject *UserDataObj_New(UserData itself)
{
	UserDataObject *it;
	if (itself == NULL) {
						PyErr_SetString(Qt_Error,"Cannot create null UserData");
						return NULL;
					}
	it = PyObject_NEW(UserDataObject, &UserData_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
int UserDataObj_Convert(PyObject *v, UserData *p_itself)
{
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
	DisposeUserData(self->ob_itself);
	PyObject_Del(self);
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
	{NULL, NULL, 0}
};

PyMethodChain UserDataObj_chain = { UserDataObj_methods, NULL };

static PyObject *UserDataObj_getattr(UserDataObject *self, char *name)
{
	return Py_FindMethodInChain(&UserDataObj_chain, (PyObject *)self, name);
}

#define UserDataObj_setattr NULL

#define UserDataObj_compare NULL

#define UserDataObj_repr NULL

#define UserDataObj_hash NULL

PyTypeObject UserData_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.UserData", /*tp_name*/
	sizeof(UserDataObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) UserDataObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) UserDataObj_getattr, /*tp_getattr*/
	(setattrfunc) UserDataObj_setattr, /*tp_setattr*/
	(cmpfunc) UserDataObj_compare, /*tp_compare*/
	(reprfunc) UserDataObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) UserDataObj_hash, /*tp_hash*/
};

/* -------------------- End object type UserData -------------------- */


/* ----------------------- Object type Media ------------------------ */

PyTypeObject Media_Type;

#define MediaObj_Check(x) ((x)->ob_type == &Media_Type)

typedef struct MediaObject {
	PyObject_HEAD
	Media ob_itself;
} MediaObject;

PyObject *MediaObj_New(Media itself)
{
	MediaObject *it;
	if (itself == NULL) {
						PyErr_SetString(Qt_Error,"Cannot create null Media");
						return NULL;
					}
	it = PyObject_NEW(MediaObject, &Media_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
int MediaObj_Convert(PyObject *v, Media *p_itself)
{
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
	DisposeTrackMedia(self->ob_itself);
	PyObject_Del(self);
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

PyMethodChain MediaObj_chain = { MediaObj_methods, NULL };

static PyObject *MediaObj_getattr(MediaObject *self, char *name)
{
	return Py_FindMethodInChain(&MediaObj_chain, (PyObject *)self, name);
}

#define MediaObj_setattr NULL

#define MediaObj_compare NULL

#define MediaObj_repr NULL

#define MediaObj_hash NULL

PyTypeObject Media_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.Media", /*tp_name*/
	sizeof(MediaObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) MediaObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) MediaObj_getattr, /*tp_getattr*/
	(setattrfunc) MediaObj_setattr, /*tp_setattr*/
	(cmpfunc) MediaObj_compare, /*tp_compare*/
	(reprfunc) MediaObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) MediaObj_hash, /*tp_hash*/
};

/* --------------------- End object type Media ---------------------- */


/* ----------------------- Object type Track ------------------------ */

PyTypeObject Track_Type;

#define TrackObj_Check(x) ((x)->ob_type == &Track_Type)

typedef struct TrackObject {
	PyObject_HEAD
	Track ob_itself;
} TrackObject;

PyObject *TrackObj_New(Track itself)
{
	TrackObject *it;
	if (itself == NULL) {
						PyErr_SetString(Qt_Error,"Cannot create null Track");
						return NULL;
					}
	it = PyObject_NEW(TrackObject, &Track_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
int TrackObj_Convert(PyObject *v, Track *p_itself)
{
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
	DisposeMovieTrack(self->ob_itself);
	PyObject_Del(self);
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

PyMethodChain TrackObj_chain = { TrackObj_methods, NULL };

static PyObject *TrackObj_getattr(TrackObject *self, char *name)
{
	return Py_FindMethodInChain(&TrackObj_chain, (PyObject *)self, name);
}

#define TrackObj_setattr NULL

#define TrackObj_compare NULL

#define TrackObj_repr NULL

#define TrackObj_hash NULL

PyTypeObject Track_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.Track", /*tp_name*/
	sizeof(TrackObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) TrackObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) TrackObj_getattr, /*tp_getattr*/
	(setattrfunc) TrackObj_setattr, /*tp_setattr*/
	(cmpfunc) TrackObj_compare, /*tp_compare*/
	(reprfunc) TrackObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) TrackObj_hash, /*tp_hash*/
};

/* --------------------- End object type Track ---------------------- */


/* ----------------------- Object type Movie ------------------------ */

PyTypeObject Movie_Type;

#define MovieObj_Check(x) ((x)->ob_type == &Movie_Type)

typedef struct MovieObject {
	PyObject_HEAD
	Movie ob_itself;
} MovieObject;

PyObject *MovieObj_New(Movie itself)
{
	MovieObject *it;
	if (itself == NULL) {
						PyErr_SetString(Qt_Error,"Cannot create null Movie");
						return NULL;
					}
	it = PyObject_NEW(MovieObject, &Movie_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
int MovieObj_Convert(PyObject *v, Movie *p_itself)
{
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
	DisposeMovie(self->ob_itself);
	PyObject_Del(self);
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

#if !TARGET_API_MAC_CARBON

static PyObject *MovieObj_SetMovieAnchorDataRef(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataRef;
	OSType dataRefType;
#ifndef SetMovieAnchorDataRef
	PyMac_PRECHECK(SetMovieAnchorDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_err = SetMovieAnchorDataRef(_self->ob_itself,
	                             dataRef,
	                             dataRefType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *MovieObj_GetMovieAnchorDataRef(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataRef;
	OSType dataRefType;
	long outFlags;
#ifndef GetMovieAnchorDataRef
	PyMac_PRECHECK(GetMovieAnchorDataRef);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetMovieAnchorDataRef(_self->ob_itself,
	                             &dataRef,
	                             &dataRefType,
	                             &outFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&l",
	                     ResObj_New, dataRef,
	                     PyMac_BuildOSType, dataRefType,
	                     outFlags);
	return _res;
}
#endif

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

#if !TARGET_API_MAC_CARBON

static PyObject *MovieObj_GetMovieLoadState(MovieObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef GetMovieLoadState
	PyMac_PRECHECK(GetMovieLoadState);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMovieLoadState(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}
#endif

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
	{"GetMovieNextInterestingTime", (PyCFunction)MovieObj_GetMovieNextInterestingTime, 1,
	 PyDoc_STR("(short interestingTimeFlags, short numMediaTypes, OSType whichMediaTypes, TimeValue time, Fixed rate) -> (TimeValue interestingTime, TimeValue interestingDuration)")},
	{"AddMovieResource", (PyCFunction)MovieObj_AddMovieResource, 1,
	 PyDoc_STR("(short resRefNum, Str255 resName) -> (short resId)")},
	{"UpdateMovieResource", (PyCFunction)MovieObj_UpdateMovieResource, 1,
	 PyDoc_STR("(short resRefNum, short resId, Str255 resName) -> None")},
	{"HasMovieChanged", (PyCFunction)MovieObj_HasMovieChanged, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"ClearMovieChanged", (PyCFunction)MovieObj_ClearMovieChanged, 1,
	 PyDoc_STR("() -> None")},
	{"SetMovieDefaultDataRef", (PyCFunction)MovieObj_SetMovieDefaultDataRef, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataRefType) -> None")},
	{"GetMovieDefaultDataRef", (PyCFunction)MovieObj_GetMovieDefaultDataRef, 1,
	 PyDoc_STR("() -> (Handle dataRef, OSType dataRefType)")},

#if !TARGET_API_MAC_CARBON
	{"SetMovieAnchorDataRef", (PyCFunction)MovieObj_SetMovieAnchorDataRef, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataRefType) -> None")},
#endif

#if !TARGET_API_MAC_CARBON
	{"GetMovieAnchorDataRef", (PyCFunction)MovieObj_GetMovieAnchorDataRef, 1,
	 PyDoc_STR("() -> (Handle dataRef, OSType dataRefType, long outFlags)")},
#endif
	{"SetMovieColorTable", (PyCFunction)MovieObj_SetMovieColorTable, 1,
	 PyDoc_STR("(CTabHandle ctab) -> None")},
	{"GetMovieColorTable", (PyCFunction)MovieObj_GetMovieColorTable, 1,
	 PyDoc_STR("() -> (CTabHandle ctab)")},
	{"FlattenMovie", (PyCFunction)MovieObj_FlattenMovie, 1,
	 PyDoc_STR("(long movieFlattenFlags, FSSpec theFile, OSType creator, ScriptCode scriptTag, long createMovieFileFlags, Str255 resName) -> (short resId)")},
	{"FlattenMovieData", (PyCFunction)MovieObj_FlattenMovieData, 1,
	 PyDoc_STR("(long movieFlattenFlags, FSSpec theFile, OSType creator, ScriptCode scriptTag, long createMovieFileFlags) -> (Movie _rv)")},
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

#if !TARGET_API_MAC_CARBON
	{"GetMovieLoadState", (PyCFunction)MovieObj_GetMovieLoadState, 1,
	 PyDoc_STR("() -> (long _rv)")},
#endif
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

PyMethodChain MovieObj_chain = { MovieObj_methods, NULL };

static PyObject *MovieObj_getattr(MovieObject *self, char *name)
{
	return Py_FindMethodInChain(&MovieObj_chain, (PyObject *)self, name);
}

#define MovieObj_setattr NULL

#define MovieObj_compare NULL

#define MovieObj_repr NULL

#define MovieObj_hash NULL

PyTypeObject Movie_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Qt.Movie", /*tp_name*/
	sizeof(MovieObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) MovieObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) MovieObj_getattr, /*tp_getattr*/
	(setattrfunc) MovieObj_setattr, /*tp_setattr*/
	(cmpfunc) MovieObj_compare, /*tp_compare*/
	(reprfunc) MovieObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) MovieObj_hash, /*tp_hash*/
};

/* --------------------- End object type Movie ---------------------- */


#if !TARGET_API_MAC_CARBON

static PyObject *Qt_CheckQuickTimeRegistration(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	void * registrationKey;
	long flags;
#ifndef CheckQuickTimeRegistration
	PyMac_PRECHECK(CheckQuickTimeRegistration);
#endif
	if (!PyArg_ParseTuple(_args, "sl",
	                      &registrationKey,
	                      &flags))
		return NULL;
	CheckQuickTimeRegistration(registrationKey,
	                           flags);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

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

#if !TARGET_API_MAC_CARBON

static PyObject *Qt_OpenADataHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle dataRef;
	OSType dataHandlerSubType;
	Handle anchorDataRef;
	OSType anchorDataRefType;
	TimeBase tb;
	long flags;
	ComponentInstance dh;
#ifndef OpenADataHandler
	PyMac_PRECHECK(OpenADataHandler);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&O&O&l",
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataHandlerSubType,
	                      ResObj_Convert, &anchorDataRef,
	                      PyMac_GetOSType, &anchorDataRefType,
	                      TimeBaseObj_Convert, &tb,
	                      &flags))
		return NULL;
	_err = OpenADataHandler(dataRef,
	                        dataHandlerSubType,
	                        anchorDataRef,
	                        anchorDataRefType,
	                        tb,
	                        flags,
	                        &dh);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, dh);
	return _res;
}
#endif

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
	OSType dataRefType;
#ifndef NewMovieFromDataRef
	PyMac_PRECHECK(NewMovieFromDataRef);
#endif
	if (!PyArg_ParseTuple(_args, "hO&O&",
	                      &flags,
	                      ResObj_Convert, &dataRef,
	                      PyMac_GetOSType, &dataRefType))
		return NULL;
	_err = NewMovieFromDataRef(&m,
	                           flags,
	                           &id,
	                           dataRef,
	                           dataRefType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&h",
	                     MovieObj_New, m,
	                     id);
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

#if !TARGET_API_MAC_CARBON

static PyObject *Qt_SpriteMediaGetIndImageProperty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	short imageIndex;
	long imagePropertyType;
	void * imagePropertyValue;
#ifndef SpriteMediaGetIndImageProperty
	PyMac_PRECHECK(SpriteMediaGetIndImageProperty);
#endif
	if (!PyArg_ParseTuple(_args, "O&hls",
	                      CmpInstObj_Convert, &mh,
	                      &imageIndex,
	                      &imagePropertyType,
	                      &imagePropertyValue))
		return NULL;
	_rv = SpriteMediaGetIndImageProperty(mh,
	                                     imageIndex,
	                                     imagePropertyType,
	                                     imagePropertyValue);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}
#endif

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

#if !TARGET_API_MAC_CARBON

static PyObject *Qt_MovieMediaGetCurrentMovieProperty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	OSType whichProperty;
	void * value;
#ifndef MovieMediaGetCurrentMovieProperty
	PyMac_PRECHECK(MovieMediaGetCurrentMovieProperty);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&s",
	                      CmpInstObj_Convert, &mh,
	                      PyMac_GetOSType, &whichProperty,
	                      &value))
		return NULL;
	_rv = MovieMediaGetCurrentMovieProperty(mh,
	                                        whichProperty,
	                                        value);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Qt_MovieMediaGetCurrentTrackProperty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	long trackID;
	OSType whichProperty;
	void * value;
#ifndef MovieMediaGetCurrentTrackProperty
	PyMac_PRECHECK(MovieMediaGetCurrentTrackProperty);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&s",
	                      CmpInstObj_Convert, &mh,
	                      &trackID,
	                      PyMac_GetOSType, &whichProperty,
	                      &value))
		return NULL;
	_rv = MovieMediaGetCurrentTrackProperty(mh,
	                                        trackID,
	                                        whichProperty,
	                                        value);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Qt_MovieMediaGetChildMovieDataReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID dataRefID;
	short dataRefIndex;
	OSType dataRefType;
	Handle dataRef;
	QTAtomID dataRefIDOut;
	short dataRefIndexOut;
#ifndef MovieMediaGetChildMovieDataReference
	PyMac_PRECHECK(MovieMediaGetChildMovieDataReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&lh",
	                      CmpInstObj_Convert, &mh,
	                      &dataRefID,
	                      &dataRefIndex))
		return NULL;
	_rv = MovieMediaGetChildMovieDataReference(mh,
	                                           dataRefID,
	                                           dataRefIndex,
	                                           &dataRefType,
	                                           &dataRef,
	                                           &dataRefIDOut,
	                                           &dataRefIndexOut);
	_res = Py_BuildValue("lO&O&lh",
	                     _rv,
	                     PyMac_BuildOSType, dataRefType,
	                     ResObj_New, dataRef,
	                     dataRefIDOut,
	                     dataRefIndexOut);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Qt_MovieMediaSetChildMovieDataReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID dataRefID;
	OSType dataRefType;
	Handle dataRef;
#ifndef MovieMediaSetChildMovieDataReference
	PyMac_PRECHECK(MovieMediaSetChildMovieDataReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&O&",
	                      CmpInstObj_Convert, &mh,
	                      &dataRefID,
	                      PyMac_GetOSType, &dataRefType,
	                      ResObj_Convert, &dataRef))
		return NULL;
	_rv = MovieMediaSetChildMovieDataReference(mh,
	                                           dataRefID,
	                                           dataRefType,
	                                           dataRef);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Qt_MovieMediaLoadChildMovieFromDataReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	QTAtomID dataRefID;
#ifndef MovieMediaLoadChildMovieFromDataReference
	PyMac_PRECHECK(MovieMediaLoadChildMovieFromDataReference);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CmpInstObj_Convert, &mh,
	                      &dataRefID))
		return NULL;
	_rv = MovieMediaLoadChildMovieFromDataReference(mh,
	                                                dataRefID);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}
#endif

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

#if !TARGET_API_MAC_CARBON

static PyObject *Qt_Media3DGetViewObject(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentResult _rv;
	MediaHandler mh;
	void * tq3viewObject;
#ifndef Media3DGetViewObject
	PyMac_PRECHECK(Media3DGetViewObject);
#endif
	if (!PyArg_ParseTuple(_args, "O&s",
	                      CmpInstObj_Convert, &mh,
	                      &tq3viewObject))
		return NULL;
	_rv = Media3DGetViewObject(mh,
	                           tq3viewObject);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}
#endif

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
	if (!PyArg_ParseTuple(_args, "O&",
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
	if (!PyArg_ParseTuple(_args, "l",
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

static PyMethodDef Qt_methods[] = {

#if !TARGET_API_MAC_CARBON
	{"CheckQuickTimeRegistration", (PyCFunction)Qt_CheckQuickTimeRegistration, 1,
	 PyDoc_STR("(void * registrationKey, long flags) -> None")},
#endif
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
	{"DisposeMatte", (PyCFunction)Qt_DisposeMatte, 1,
	 PyDoc_STR("(PixMapHandle theMatte) -> None")},
	{"NewMovie", (PyCFunction)Qt_NewMovie, 1,
	 PyDoc_STR("(long flags) -> (Movie _rv)")},
	{"GetDataHandler", (PyCFunction)Qt_GetDataHandler, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataHandlerSubType, long flags) -> (Component _rv)")},

#if !TARGET_API_MAC_CARBON
	{"OpenADataHandler", (PyCFunction)Qt_OpenADataHandler, 1,
	 PyDoc_STR("(Handle dataRef, OSType dataHandlerSubType, Handle anchorDataRef, OSType anchorDataRefType, TimeBase tb, long flags) -> (ComponentInstance dh)")},
#endif
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
	 PyDoc_STR("(short flags, Handle dataRef, OSType dataRefType) -> (Movie m, short id)")},
	{"RemoveMovieResource", (PyCFunction)Qt_RemoveMovieResource, 1,
	 PyDoc_STR("(short resRefNum, short resId) -> None")},
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

#if !TARGET_API_MAC_CARBON
	{"SpriteMediaGetIndImageProperty", (PyCFunction)Qt_SpriteMediaGetIndImageProperty, 1,
	 PyDoc_STR("(MediaHandler mh, short imageIndex, long imagePropertyType, void * imagePropertyValue) -> (ComponentResult _rv)")},
#endif
	{"SpriteMediaDisposeSprite", (PyCFunction)Qt_SpriteMediaDisposeSprite, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID spriteID) -> (ComponentResult _rv)")},
	{"SpriteMediaSetActionVariableToString", (PyCFunction)Qt_SpriteMediaSetActionVariableToString, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID variableID, Ptr theCString) -> (ComponentResult _rv)")},
	{"SpriteMediaGetActionVariableAsString", (PyCFunction)Qt_SpriteMediaGetActionVariableAsString, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID variableID) -> (ComponentResult _rv, Handle theCString)")},
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

#if !TARGET_API_MAC_CARBON
	{"MovieMediaGetCurrentMovieProperty", (PyCFunction)Qt_MovieMediaGetCurrentMovieProperty, 1,
	 PyDoc_STR("(MediaHandler mh, OSType whichProperty, void * value) -> (ComponentResult _rv)")},
#endif

#if !TARGET_API_MAC_CARBON
	{"MovieMediaGetCurrentTrackProperty", (PyCFunction)Qt_MovieMediaGetCurrentTrackProperty, 1,
	 PyDoc_STR("(MediaHandler mh, long trackID, OSType whichProperty, void * value) -> (ComponentResult _rv)")},
#endif

#if !TARGET_API_MAC_CARBON
	{"MovieMediaGetChildMovieDataReference", (PyCFunction)Qt_MovieMediaGetChildMovieDataReference, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID dataRefID, short dataRefIndex) -> (ComponentResult _rv, OSType dataRefType, Handle dataRef, QTAtomID dataRefIDOut, short dataRefIndexOut)")},
#endif

#if !TARGET_API_MAC_CARBON
	{"MovieMediaSetChildMovieDataReference", (PyCFunction)Qt_MovieMediaSetChildMovieDataReference, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID dataRefID, OSType dataRefType, Handle dataRef) -> (ComponentResult _rv)")},
#endif

#if !TARGET_API_MAC_CARBON
	{"MovieMediaLoadChildMovieFromDataReference", (PyCFunction)Qt_MovieMediaLoadChildMovieFromDataReference, 1,
	 PyDoc_STR("(MediaHandler mh, QTAtomID dataRefID) -> (ComponentResult _rv)")},
#endif
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

#if !TARGET_API_MAC_CARBON
	{"Media3DGetViewObject", (PyCFunction)Qt_Media3DGetViewObject, 1,
	 PyDoc_STR("(MediaHandler mh, void * tq3viewObject) -> (ComponentResult _rv)")},
#endif
	{"NewTimeBase", (PyCFunction)Qt_NewTimeBase, 1,
	 PyDoc_STR("() -> (TimeBase _rv)")},
	{"ConvertTime", (PyCFunction)Qt_ConvertTime, 1,
	 PyDoc_STR("(TimeBase newBase) -> (TimeRecord theTime)")},
	{"ConvertTimeScale", (PyCFunction)Qt_ConvertTimeScale, 1,
	 PyDoc_STR("(TimeScale newScale) -> (TimeRecord theTime)")},
	{"AddTime", (PyCFunction)Qt_AddTime, 1,
	 PyDoc_STR("(TimeRecord dst, TimeRecord src) -> (TimeRecord dst)")},
	{"SubtractTime", (PyCFunction)Qt_SubtractTime, 1,
	 PyDoc_STR("(TimeRecord dst, TimeRecord src) -> (TimeRecord dst)")},
	{"MusicMediaGetIndexedTunePlayer", (PyCFunction)Qt_MusicMediaGetIndexedTunePlayer, 1,
	 PyDoc_STR("(ComponentInstance ti, long sampleDescIndex) -> (ComponentResult _rv, ComponentInstance tp)")},
	{"AlignWindow", (PyCFunction)Qt_AlignWindow, 1,
	 PyDoc_STR("(WindowPtr wp, Boolean front) -> None")},
	{"DragAlignedWindow", (PyCFunction)Qt_DragAlignedWindow, 1,
	 PyDoc_STR("(WindowPtr wp, Point startPt, Rect boundsRect) -> None")},
	{"MoviesTask", (PyCFunction)Qt_MoviesTask, 1,
	 PyDoc_STR("(long maxMilliSecToUse) -> None")},
	{NULL, NULL, 0}
};




void init_Qt(void)
{
	PyObject *m;
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


	m = Py_InitModule("_Qt", Qt_methods);
	d = PyModule_GetDict(m);
	Qt_Error = PyMac_GetOSErrException();
	if (Qt_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Qt_Error) != 0)
		return;
	MovieController_Type.ob_type = &PyType_Type;
	Py_INCREF(&MovieController_Type);
	if (PyDict_SetItemString(d, "MovieControllerType", (PyObject *)&MovieController_Type) != 0)
		Py_FatalError("can't initialize MovieControllerType");
	TimeBase_Type.ob_type = &PyType_Type;
	Py_INCREF(&TimeBase_Type);
	if (PyDict_SetItemString(d, "TimeBaseType", (PyObject *)&TimeBase_Type) != 0)
		Py_FatalError("can't initialize TimeBaseType");
	UserData_Type.ob_type = &PyType_Type;
	Py_INCREF(&UserData_Type);
	if (PyDict_SetItemString(d, "UserDataType", (PyObject *)&UserData_Type) != 0)
		Py_FatalError("can't initialize UserDataType");
	Media_Type.ob_type = &PyType_Type;
	Py_INCREF(&Media_Type);
	if (PyDict_SetItemString(d, "MediaType", (PyObject *)&Media_Type) != 0)
		Py_FatalError("can't initialize MediaType");
	Track_Type.ob_type = &PyType_Type;
	Py_INCREF(&Track_Type);
	if (PyDict_SetItemString(d, "TrackType", (PyObject *)&Track_Type) != 0)
		Py_FatalError("can't initialize TrackType");
	Movie_Type.ob_type = &PyType_Type;
	Py_INCREF(&Movie_Type);
	if (PyDict_SetItemString(d, "MovieType", (PyObject *)&Movie_Type) != 0)
		Py_FatalError("can't initialize MovieType");
}

/* ========================= End module _Qt ========================= */

