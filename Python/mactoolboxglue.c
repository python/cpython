/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/


#include "Python.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <Script.h>
#include <Resources.h>
#endif

/*
** Find out what the current script is.
** Donated by Fredrik Lund.
*/
char *PyMac_getscript()
{
#if TARGET_API_MAC_OSX
    /* We cannot use GetSysFont because it requires the window manager
    ** There are other APIs to query the default 8 bit encoding, but
    ** I don't know about them (yet).
    */
    return "ascii";
#else
   int font, script, lang;
    font = 0;
    font = GetSysFont();
    script = FontToScript(font);
    switch (script) {
    case smRoman:
        lang = GetScriptVariable(script, smScriptLang);
        if (lang == langIcelandic)
            return "mac-iceland";
        else if (lang == langTurkish)
            return "mac-turkish";
        else if (lang == langGreek)
            return "mac-greek";
        else
            return "mac-roman";
        break;
#if 0
    /* We don't have a codec for this, so don't return it */
    case smJapanese:
        return "mac-japan";
#endif
    case smGreek:
        return "mac-greek";
    case smCyrillic:
        return "mac-cyrillic";
    default:
        return "ascii"; /* better than nothing */
    }
#endif /* TARGET_API_MAC_OSX */
}

/* Like strerror() but for Mac OS error numbers */
char *PyMac_StrError(int err)
{
	static char buf[256];
	Handle h;
	char *str;
	
	h = GetResource('Estr', err);
	if ( h ) {
		HLock(h);
		str = (char *)*h;
		memcpy(buf, str+1, (unsigned char)str[0]);
		buf[(unsigned char)str[0]] = '\0';
		HUnlock(h);
		ReleaseResource(h);
	} else {
		PyOS_snprintf(buf, sizeof(buf), "Mac OS error code %d", err);
	}
	return buf;
}

/* Exception object shared by all Mac specific modules for Mac OS errors */
PyObject *PyMac_OSErrException;

/* Initialize and return PyMac_OSErrException */
PyObject *
PyMac_GetOSErrException(void)
{
	if (PyMac_OSErrException == NULL)
		PyMac_OSErrException = PyString_FromString("MacOS.Error");
	return PyMac_OSErrException;
}

/* Set a MAC-specific error from errno, and return NULL; return None if no error */
PyObject * 
PyErr_Mac(PyObject *eobj, int err)
{
	char *msg;
	PyObject *v;
	
	if (err == 0 && !PyErr_Occurred()) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (err == -1 && PyErr_Occurred())
		return NULL;
	msg = PyMac_StrError(err);
	v = Py_BuildValue("(is)", err, msg);
	PyErr_SetObject(eobj, v);
	Py_DECREF(v);
	return NULL;
}

/* Call PyErr_Mac with PyMac_OSErrException */
PyObject *
PyMac_Error(OSErr err)
{
	return PyErr_Mac(PyMac_GetOSErrException(), err);
}


#if TARGET_API_MAC_OSX
OSErr
PyMac_GetFullPathname(FSSpec *fss, char *path, int len)
{
	FSRef fsr;
	OSErr err;
	
	*path = '\0';
	err = FSpMakeFSRef(fss, &fsr);
	if ( err == fnfErr ) {
		/* FSSpecs can point to non-existing files, fsrefs can't. */
		FSSpec fss2;
		int tocopy;
		
		err = FSMakeFSSpec(fss->vRefNum, fss->parID, "", &fss2);
		if ( err ) return err;
		err = FSpMakeFSRef(&fss2, &fsr);
		if ( err ) return err;
		err = (OSErr)FSRefMakePath(&fsr, path, len-1);
		if ( err ) return err;
		/* This part is not 100% safe: we append the filename part, but
		** I'm not sure that we don't run afoul of the various 8bit
		** encodings here. Will have to look this up at some point...
		*/
		strcat(path, "/");
		tocopy = fss->name[0];
		if ( strlen(path) + tocopy >= len )
			tocopy = len - strlen(path) - 1;
		if ( tocopy > 0 )
			strncat(path, fss->name+1, tocopy);
	} else {
		if ( err ) return err;
		err = (OSErr)FSRefMakePath(&fsr, path, len);
		if ( err ) return err;
	}
	return 0;
}

#endif /* TARGET_API_MAC_OSX */

#ifdef WITH_NEXT_FRAMEWORK
/*
** In a bundle, find a file "resourceName" of type "resourceType". Return the
** full pathname in "resourceURLCstr".
*/
static int
locateResourcePy(CFStringRef resourceType, CFStringRef resourceName, char *resourceURLCStr, int length)
{
    CFBundleRef mainBundle = NULL;
    CFURLRef URL, absoluteURL;
    CFStringRef filenameString, filepathString;
    CFIndex size, i;
    CFArrayRef arrayRef = NULL;
    int success = 0;
    
#if TARGET_API_MAC_OSX
	CFURLPathStyle thePathStyle = kCFURLPOSIXPathStyle;
#else
	CFURLPathStyle thePathStyle = kCFURLHFSPathStyle;
#endif

    /* Get a reference to our main bundle */
    mainBundle = CFBundleGetMainBundle();

	/* If we are running inside a bundle, look through it. Otherwise, do nothing. */
	if (mainBundle) {

	    /* Look for py files in the main bundle by type */
	    arrayRef = CFBundleCopyResourceURLsOfType( mainBundle, 
	            resourceType, 
	           NULL );

	    /* See if there are any filename matches */
	    size = CFArrayGetCount(arrayRef);
	    for (i = 0; i < size; i++) {
	        URL = CFArrayGetValueAtIndex(arrayRef, i);
	        filenameString = CFURLCopyLastPathComponent(URL);
	        if (CFStringCompare(filenameString, resourceName, 0) == kCFCompareEqualTo) {
	            /* We found a match, get the file's full path */
	            absoluteURL = CFURLCopyAbsoluteURL(URL);
	            filepathString = CFURLCopyFileSystemPath(absoluteURL, thePathStyle);
	            CFRelease(absoluteURL);

	            /* Copy the full path into the caller's character buffer */
	            success = CFStringGetCString(filepathString, resourceURLCStr, length,
	                                        kCFStringEncodingMacRoman);

	            CFRelease(filepathString);
	        }
	        CFRelease(filenameString);
	    }
		CFRelease(arrayRef);
	}
    return success;
}

/*
** iff we are running in a .app framework then we could be
** the main program for an applet. In that case, return the
** script filename for the applet.
** Otherwise return NULL.
*/
char *
PyMac_GetAppletScriptFile(void)
{
    static char scriptpath[1024];

	/* First we see whether we have __rawmain__.py and run that if it
	** is there. This is used for applets that want sys.argv to be
	** unix-like: __rawmain__ will construct it (from the initial appleevent)
	** and then call __main__.py.
	*/
	if (locateResourcePy(CFSTR("py"), CFSTR("__rawmain__.py"), scriptpath, 1024)) {
		return scriptpath;
	} else if (locateResourcePy(CFSTR("pyc"), CFSTR("__rawmain__.pyc"), scriptpath, 1024)) {
		return scriptpath;
	} else if (locateResourcePy(CFSTR("py"), CFSTR("__main__.py"), scriptpath, 1024)) {
		return scriptpath;
	} else if (locateResourcePy(CFSTR("pyc"), CFSTR("__main__.pyc"), scriptpath, 1024)) {
		return scriptpath;
	}
	return NULL;
}

#endif


/* Convert a 4-char string object argument to an OSType value */
int
PyMac_GetOSType(PyObject *v, OSType *pr)
{
	if (!PyString_Check(v) || PyString_Size(v) != 4) {
		PyErr_SetString(PyExc_TypeError,
			"OSType arg must be string of 4 chars");
		return 0;
	}
	memcpy((char *)pr, PyString_AsString(v), 4);
	return 1;
}

/* Convert an OSType value to a 4-char string object */
PyObject *
PyMac_BuildOSType(OSType t)
{
	return PyString_FromStringAndSize((char *)&t, 4);
}

/* Convert an NumVersion value to a 4-element tuple */
PyObject *
PyMac_BuildNumVersion(NumVersion t)
{
	return Py_BuildValue("(hhhh)", t.majorRev, t.minorAndBugRev, t.stage, t.nonRelRev);
}


/* Convert a Python string object to a Str255 */
int
PyMac_GetStr255(PyObject *v, Str255 pbuf)
{
	int len;
	if (!PyString_Check(v) || (len = PyString_Size(v)) > 255) {
		PyErr_SetString(PyExc_TypeError,
			"Str255 arg must be string of at most 255 chars");
		return 0;
	}
	pbuf[0] = len;
	memcpy((char *)(pbuf+1), PyString_AsString(v), len);
	return 1;
}

/* Convert a Str255 to a Python string object */
PyObject *
PyMac_BuildStr255(Str255 s)
{
	if ( s == NULL ) {
		PyErr_SetString(PyExc_SystemError, "Str255 pointer is NULL");
		return NULL;
	}
	return PyString_FromStringAndSize((char *)&s[1], (int)s[0]);
}

PyObject *
PyMac_BuildOptStr255(Str255 s)
{
	if ( s == NULL ) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromStringAndSize((char *)&s[1], (int)s[0]);
}



/* Convert a Python object to a Rect.
   The object must be a (left, top, right, bottom) tuple.
   (This differs from the order in the struct but is consistent with
   the arguments to SetRect(), and also with STDWIN). */
int
PyMac_GetRect(PyObject *v, Rect *r)
{
	return PyArg_Parse(v, "(hhhh)", &r->left, &r->top, &r->right, &r->bottom);
}

/* Convert a Rect to a Python object */
PyObject *
PyMac_BuildRect(Rect *r)
{
	return Py_BuildValue("(hhhh)", r->left, r->top, r->right, r->bottom);
}


/* Convert a Python object to a Point.
   The object must be a (h, v) tuple.
   (This differs from the order in the struct but is consistent with
   the arguments to SetPoint(), and also with STDWIN). */
int
PyMac_GetPoint(PyObject *v, Point *p)
{
	return PyArg_Parse(v, "(hh)", &p->h, &p->v);
}

/* Convert a Point to a Python object */
PyObject *
PyMac_BuildPoint(Point p)
{
	return Py_BuildValue("(hh)", p.h, p.v);
}


/* Convert a Python object to an EventRecord.
   The object must be a (what, message, when, (v, h), modifiers) tuple. */
int
PyMac_GetEventRecord(PyObject *v, EventRecord *e)
{
	return PyArg_Parse(v, "(Hll(hh)H)",
	                   &e->what,
	                   &e->message,
	                   &e->when,
	                   &e->where.h,
	                   &e->where.v,                   
	                   &e->modifiers);
}

/* Convert a Rect to an EventRecord object */
PyObject *
PyMac_BuildEventRecord(EventRecord *e)
{
	return Py_BuildValue("(hll(hh)h)",
	                     e->what,
	                     e->message,
	                     e->when,
	                     e->where.h,
	                     e->where.v,
	                     e->modifiers);
}

/* Convert Python object to Fixed */
int
PyMac_GetFixed(PyObject *v, Fixed *f)
{
	double d;
	
	if( !PyArg_Parse(v, "d", &d))
		return 0;
	*f = (Fixed)(d * 0x10000);
	return 1;
}

/* Convert a Fixed to a Python object */
PyObject *
PyMac_BuildFixed(Fixed f)
{
	double d;
	
	d = f;
	d = d / 0x10000;
	return Py_BuildValue("d", d);
}

/* Convert wide to/from Python int or (hi, lo) tuple. XXXX Should use Python longs */
int
PyMac_Getwide(PyObject *v, wide *rv)
{
	if (PyInt_Check(v)) {
		rv->hi = 0;
		rv->lo = PyInt_AsLong(v);
		if( rv->lo & 0x80000000 )
			rv->hi = -1;
		return 1;
	}
	return PyArg_Parse(v, "(ll)", &rv->hi, &rv->lo);
}


PyObject *
PyMac_Buildwide(wide *w)
{
	if ( (w->hi == 0 && (w->lo & 0x80000000) == 0) ||
	     (w->hi == -1 && (w->lo & 0x80000000) ) )
		return PyInt_FromLong(w->lo);
	return Py_BuildValue("(ll)", w->hi, w->lo);
}

#ifdef USE_TOOLBOX_OBJECT_GLUE
/*
** Glue together the toolbox objects.
**
** Because toolbox modules interdepend on each other, they use each others
** object types, on MacOSX/MachO this leads to the situation that they
** cannot be dynamically loaded (or they would all have to be lumped into
** a single .so, but this would be bad for extensibility).
**
** This file defines wrappers for all the _New and _Convert functions,
** which are the Py_BuildValue and PyArg_ParseTuple helpers. The wrappers
** check an indirection function pointer, and if it isn't filled in yet
** they import the appropriate module, whose init routine should fill in
** the pointer.
*/

#define GLUE_NEW(object, routinename, module) \
PyObject *(*PyMacGluePtr_##routinename)(object); \
\
PyObject *routinename(object cobj) { \
    if (!PyMacGluePtr_##routinename) { \
       if (!PyImport_ImportModule(module)) return NULL; \
       if (!PyMacGluePtr_##routinename) { \
           PyErr_SetString(PyExc_ImportError, "Module did not provide routine: " module ": " #routinename); \
           return NULL; \
       } \
    } \
    return (*PyMacGluePtr_##routinename)(cobj); \
}

#define GLUE_CONVERT(object, routinename, module) \
int (*PyMacGluePtr_##routinename)(PyObject *, object *); \
\
int routinename(PyObject *pyobj, object *cobj) { \
    if (!PyMacGluePtr_##routinename) { \
       if (!PyImport_ImportModule(module)) return NULL; \
       if (!PyMacGluePtr_##routinename) { \
           PyErr_SetString(PyExc_ImportError, "Module did not provide routine: " module ": " #routinename); \
           return NULL; \
       } \
    } \
    return (*PyMacGluePtr_##routinename)(pyobj, cobj); \
}

GLUE_NEW(FSSpec *, PyMac_BuildFSSpec, "macfs")
GLUE_CONVERT(FSSpec, PyMac_GetFSSpec, "macfs")
GLUE_NEW(FSRef *, PyMac_BuildFSRef, "macfs")
GLUE_CONVERT(FSRef, PyMac_GetFSRef, "macfs")

GLUE_NEW(AppleEvent *, AEDesc_New, "Carbon.AE") /* XXXX Why by address? */
GLUE_CONVERT(AppleEvent, AEDesc_Convert, "Carbon.AE")

GLUE_NEW(Component, CmpObj_New, "Carbon.Cm")
GLUE_CONVERT(Component, CmpObj_Convert, "Carbon.Cm")
GLUE_NEW(ComponentInstance, CmpInstObj_New, "Carbon.Cm")
GLUE_CONVERT(ComponentInstance, CmpInstObj_Convert, "Carbon.Cm")

GLUE_NEW(ControlHandle, CtlObj_New, "Carbon.Ctl")
GLUE_CONVERT(ControlHandle, CtlObj_Convert, "Carbon.Ctl")

GLUE_NEW(DialogPtr, DlgObj_New, "Carbon.Dlg")
GLUE_CONVERT(DialogPtr, DlgObj_Convert, "Carbon.Dlg")
GLUE_NEW(DialogPtr, DlgObj_WhichDialog, "Carbon.Dlg")

GLUE_NEW(DragReference, DragObj_New, "Carbon.Drag")
GLUE_CONVERT(DragReference, DragObj_Convert, "Carbon.Drag")

GLUE_NEW(ListHandle, ListObj_New, "Carbon.List")
GLUE_CONVERT(ListHandle, ListObj_Convert, "Carbon.List")

GLUE_NEW(MenuHandle, MenuObj_New, "Carbon.Menu")
GLUE_CONVERT(MenuHandle, MenuObj_Convert, "Carbon.Menu")

GLUE_NEW(GrafPtr, GrafObj_New, "Carbon.Qd")
GLUE_CONVERT(GrafPtr, GrafObj_Convert, "Carbon.Qd")
GLUE_NEW(BitMapPtr, BMObj_New, "Carbon.Qd")
GLUE_CONVERT(BitMapPtr, BMObj_Convert, "Carbon.Qd")
GLUE_NEW(RGBColor *, QdRGB_New, "Carbon.Qd") /* XXXX Why? */
GLUE_CONVERT(RGBColor, QdRGB_Convert, "Carbon.Qd")

GLUE_NEW(GWorldPtr, GWorldObj_New, "Carbon.Qdoffs")
GLUE_CONVERT(GWorldPtr, GWorldObj_Convert, "Carbon.Qdoffs")

GLUE_NEW(Track, TrackObj_New, "Carbon.Qt")
GLUE_CONVERT(Track, TrackObj_Convert, "Carbon.Qt")
GLUE_NEW(Movie, MovieObj_New, "Carbon.Qt")
GLUE_CONVERT(Movie, MovieObj_Convert, "Carbon.Qt")
GLUE_NEW(MovieController, MovieCtlObj_New, "Carbon.Qt")
GLUE_CONVERT(MovieController, MovieCtlObj_Convert, "Carbon.Qt")
GLUE_NEW(TimeBase, TimeBaseObj_New, "Carbon.Qt")
GLUE_CONVERT(TimeBase, TimeBaseObj_Convert, "Carbon.Qt")
GLUE_NEW(UserData, UserDataObj_New, "Carbon.Qt")
GLUE_CONVERT(UserData, UserDataObj_Convert, "Carbon.Qt")
GLUE_NEW(Media, MediaObj_New, "Carbon.Qt")
GLUE_CONVERT(Media, MediaObj_Convert, "Carbon.Qt")

GLUE_NEW(Handle, ResObj_New, "Carbon.Res")
GLUE_CONVERT(Handle, ResObj_Convert, "Carbon.Res")
GLUE_NEW(Handle, OptResObj_New, "Carbon.Res")
GLUE_CONVERT(Handle, OptResObj_Convert, "Carbon.Res")

GLUE_NEW(TEHandle, TEObj_New, "Carbon.TE")
GLUE_CONVERT(TEHandle, TEObj_Convert, "Carbon.TE")

GLUE_NEW(WindowPtr, WinObj_New, "Carbon.Win")
GLUE_CONVERT(WindowPtr, WinObj_Convert, "Carbon.Win")
GLUE_NEW(WindowPtr, WinObj_WhichWindow, "Carbon.Win")

GLUE_CONVERT(CFTypeRef, CFTypeRefObj_Convert, "Carbon.CF")
GLUE_NEW(CFTypeRef, CFTypeRefObj_New, "Carbon.CF")

GLUE_CONVERT(CFStringRef, CFStringRefObj_Convert, "Carbon.CF")
GLUE_NEW(CFStringRef, CFStringRefObj_New, "Carbon.CF")
GLUE_CONVERT(CFMutableStringRef, CFMutableStringRefObj_Convert, "Carbon.CF")
GLUE_NEW(CFMutableStringRef, CFMutableStringRefObj_New, "Carbon.CF")

GLUE_CONVERT(CFArrayRef, CFArrayRefObj_Convert, "Carbon.CF")
GLUE_NEW(CFArrayRef, CFArrayRefObj_New, "Carbon.CF")
GLUE_CONVERT(CFMutableArrayRef, CFMutableArrayRefObj_Convert, "Carbon.CF")
GLUE_NEW(CFMutableArrayRef, CFMutableArrayRefObj_New, "Carbon.CF")

GLUE_CONVERT(CFDictionaryRef, CFDictionaryRefObj_Convert, "Carbon.CF")
GLUE_NEW(CFDictionaryRef, CFDictionaryRefObj_New, "Carbon.CF")
GLUE_CONVERT(CFMutableDictionaryRef, CFMutableDictionaryRefObj_Convert, "Carbon.CF")
GLUE_NEW(CFMutableDictionaryRef, CFMutableDictionaryRefObj_New, "Carbon.CF")

GLUE_CONVERT(CFURLRef, CFURLRefObj_Convert, "Carbon.CF")
GLUE_CONVERT(CFURLRef, OptionalCFURLRefObj_Convert, "Carbon.CF")
GLUE_NEW(CFURLRef, CFURLRefObj_New, "Carbon.CF")

#endif /* USE_TOOLBOX_OBJECT_GLUE */