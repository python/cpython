/*
** pymactoolbox.h - globals defined in mactoolboxglue.c
*/
#ifndef Py_PYMACTOOLBOX_H
#define Py_PYMACTOOLBOX_H
#ifdef __cplusplus
	extern "C" {
#endif

#include <Carbon/Carbon.h>

/*
** Issue #27806: Workaround for gcc 4.x which does not have _has_include.
*/
#ifndef __has_include
#define __has_include(x) 0
#endif
/* Workaround */

#if __has_include(<Availability.h>)
#include <Availability.h>
#define APPLE_SUPPORTS_QUICKTIME (__MAC_OS_X_VERSION_MAX_ALLOWED < 101200) && !__LP64__
#else
#define APPLE_SUPPORTS_QUICKTIME !__LP64__
#endif

#if APPLE_SUPPORTS_QUICKTIME
#include <QuickTime/QuickTime.h>
#endif /* APPLE_SUPPORTS_QUICKTIME */

/*
** Helper routines for error codes and such.
*/
char *PyMac_StrError(int);			/* strerror with mac errors */
extern PyObject *PyMac_OSErrException;		/* Exception for OSErr */
PyObject *PyMac_GetOSErrException(void);	/* Initialize & return it */
PyObject *PyErr_Mac(PyObject *, int);		/* Exception with a mac error */
PyObject *PyMac_Error(OSErr);			/* Uses PyMac_GetOSErrException */
#if APPLE_SUPPORTS_QUICKTIME
extern OSErr PyMac_GetFullPathname(FSSpec *, char *, int); /* convert
							      fsspec->path */
#endif /* APPLE_SUPPORTS_QUICKTIME */

/*
** These conversion routines are defined in mactoolboxglue.c itself.
*/
int PyMac_GetOSType(PyObject *, OSType *);	/* argument parser for OSType */
PyObject *PyMac_BuildOSType(OSType);		/* Convert OSType to PyObject */

PyObject *PyMac_BuildNumVersion(NumVersion);/* Convert NumVersion to PyObject */

int PyMac_GetStr255(PyObject *, Str255);	/* argument parser for Str255 */
PyObject *PyMac_BuildStr255(Str255);		/* Convert Str255 to PyObject */
PyObject *PyMac_BuildOptStr255(Str255);		/* Convert Str255 to PyObject,
						   NULL to None */

int PyMac_GetRect(PyObject *, Rect *);		/* argument parser for Rect */
PyObject *PyMac_BuildRect(Rect *);		/* Convert Rect to PyObject */

int PyMac_GetPoint(PyObject *, Point *);	/* argument parser for Point */
PyObject *PyMac_BuildPoint(Point);		/* Convert Point to PyObject */

int PyMac_GetEventRecord(PyObject *, EventRecord *); /* argument parser for
							EventRecord */
PyObject *PyMac_BuildEventRecord(EventRecord *); /* Convert EventRecord to
						    PyObject */

int PyMac_GetFixed(PyObject *, Fixed *);	/* argument parser for Fixed */
PyObject *PyMac_BuildFixed(Fixed);		/* Convert Fixed to PyObject */
int PyMac_Getwide(PyObject *, wide *);		/* argument parser for wide */
PyObject *PyMac_Buildwide(wide *);		/* Convert wide to PyObject */

/*
** The rest of the routines are implemented by extension modules. If they are
** dynamically loaded mactoolboxglue will contain a stub implementation of the
** routine, which imports the module, whereupon the module's init routine will
** communicate the routine pointer back to the stub.
** If USE_TOOLBOX_OBJECT_GLUE is not defined there is no glue code, and the
** extension modules simply declare the routine. This is the case for static
** builds (and could be the case for MacPython CFM builds, because CFM extension
** modules can reference each other without problems).
*/

#ifdef USE_TOOLBOX_OBJECT_GLUE
/*
** These macros are used in the module init code. If we use toolbox object glue
** it sets the function pointer to point to the real function.
*/
#define PyMac_INIT_TOOLBOX_OBJECT_NEW(object, rtn) { \
	extern PyObject *(*PyMacGluePtr_##rtn)(object); \
	PyMacGluePtr_##rtn = _##rtn; \
}
#define PyMac_INIT_TOOLBOX_OBJECT_CONVERT(object, rtn) { \
	extern int (*PyMacGluePtr_##rtn)(PyObject *, object *); \
	PyMacGluePtr_##rtn = _##rtn; \
}
#else
/*
** If we don't use toolbox object glue the init macros are empty. Moreover, we define
** _xxx_New to be the same as xxx_New, and the code in mactoolboxglue isn't included.
*/
#define PyMac_INIT_TOOLBOX_OBJECT_NEW(object, rtn)
#define PyMac_INIT_TOOLBOX_OBJECT_CONVERT(object, rtn)
#endif /* USE_TOOLBOX_OBJECT_GLUE */

/* macfs exports */
#ifndef __LP64__
int PyMac_GetFSSpec(PyObject *, FSSpec *);	/* argument parser for FSSpec */
PyObject *PyMac_BuildFSSpec(FSSpec *);		/* Convert FSSpec to PyObject */
#endif /* !__LP64__ */

int PyMac_GetFSRef(PyObject *, FSRef *);	/* argument parser for FSRef */
PyObject *PyMac_BuildFSRef(FSRef *);		/* Convert FSRef to PyObject */

/* AE exports */
extern PyObject *AEDesc_New(AppleEvent *); /* XXXX Why passed by address?? */
extern PyObject *AEDesc_NewBorrowed(AppleEvent *);
extern int AEDesc_Convert(PyObject *, AppleEvent *);

/* Cm exports */
extern PyObject *CmpObj_New(Component);
extern int CmpObj_Convert(PyObject *, Component *);
extern PyObject *CmpInstObj_New(ComponentInstance);
extern int CmpInstObj_Convert(PyObject *, ComponentInstance *);

/* Ctl exports */
#if APPLE_SUPPORTS_QUICKTIME
extern PyObject *CtlObj_New(ControlHandle);
extern int CtlObj_Convert(PyObject *, ControlHandle *);
#endif /* APPLE_SUPPORTS_QUICKTIME */

/* Dlg exports */
#if APPLE_SUPPORTS_QUICKTIME
extern PyObject *DlgObj_New(DialogPtr);
extern int DlgObj_Convert(PyObject *, DialogPtr *);
extern PyObject *DlgObj_WhichDialog(DialogPtr);
#endif /* APPLE_SUPPORTS_QUICKTIME */

/* Drag exports */
#if APPLE_SUPPORTS_QUICKTIME
extern PyObject *DragObj_New(DragReference);
extern int DragObj_Convert(PyObject *, DragReference *);
#endif /* APPLE_SUPPORTS_QUICKTIME */

/* List exports */
#if APPLE_SUPPORTS_QUICKTIME
extern PyObject *ListObj_New(ListHandle);
extern int ListObj_Convert(PyObject *, ListHandle *);
#endif /* APPLE_SUPPORTS_QUICKTIME */

/* Menu exports */
#if APPLE_SUPPORTS_QUICKTIME
extern PyObject *MenuObj_New(MenuHandle);
extern int MenuObj_Convert(PyObject *, MenuHandle *);
#endif /* APPLE_SUPPORTS_QUICKTIME */

/* Qd exports */
#if APPLE_SUPPORTS_QUICKTIME
extern PyObject *GrafObj_New(GrafPtr);
extern int GrafObj_Convert(PyObject *, GrafPtr *);
extern PyObject *BMObj_New(BitMapPtr);
extern int BMObj_Convert(PyObject *, BitMapPtr *);
extern PyObject *QdRGB_New(RGBColor *);
extern int QdRGB_Convert(PyObject *, RGBColor *);
#endif /* APPLE_SUPPORTS_QUICKTIME */

/* Qdoffs exports */
#if APPLE_SUPPORTS_QUICKTIME
extern PyObject *GWorldObj_New(GWorldPtr);
extern int GWorldObj_Convert(PyObject *, GWorldPtr *);
#endif /* APPLE_SUPPORTS_QUICKTIME */

/* Qt exports */
#if APPLE_SUPPORTS_QUICKTIME
extern PyObject *TrackObj_New(Track);
extern int TrackObj_Convert(PyObject *, Track *);
extern PyObject *MovieObj_New(Movie);
extern int MovieObj_Convert(PyObject *, Movie *);
extern PyObject *MovieCtlObj_New(MovieController);
extern int MovieCtlObj_Convert(PyObject *, MovieController *);
extern PyObject *TimeBaseObj_New(TimeBase);
extern int TimeBaseObj_Convert(PyObject *, TimeBase *);
extern PyObject *UserDataObj_New(UserData);
extern int UserDataObj_Convert(PyObject *, UserData *);
extern PyObject *MediaObj_New(Media);
extern int MediaObj_Convert(PyObject *, Media *);
#endif /* APPLE_SUPPORTS_QUICKTIME */

/* Res exports */
extern PyObject *ResObj_New(Handle);
extern int ResObj_Convert(PyObject *, Handle *);
extern PyObject *OptResObj_New(Handle);
extern int OptResObj_Convert(PyObject *, Handle *);

/* TE exports */
#if APPLE_SUPPORTS_QUICKTIME
extern PyObject *TEObj_New(TEHandle);
extern int TEObj_Convert(PyObject *, TEHandle *);
#endif /* APPLE_SUPPORTS_QUICKTIME */

/* Win exports */
#if APPLE_SUPPORTS_QUICKTIME
extern PyObject *WinObj_New(WindowPtr);
extern int WinObj_Convert(PyObject *, WindowPtr *);
extern PyObject *WinObj_WhichWindow(WindowPtr);
#endif /* APPLE_SUPPORTS_QUICKTIME */

/* CF exports */
extern PyObject *CFObj_New(CFTypeRef);
extern int CFObj_Convert(PyObject *, CFTypeRef *);
extern PyObject *CFTypeRefObj_New(CFTypeRef);
extern int CFTypeRefObj_Convert(PyObject *, CFTypeRef *);
extern PyObject *CFStringRefObj_New(CFStringRef);
extern int CFStringRefObj_Convert(PyObject *, CFStringRef *);
extern PyObject *CFMutableStringRefObj_New(CFMutableStringRef);
extern int CFMutableStringRefObj_Convert(PyObject *, CFMutableStringRef *);
extern PyObject *CFArrayRefObj_New(CFArrayRef);
extern int CFArrayRefObj_Convert(PyObject *, CFArrayRef *);
extern PyObject *CFMutableArrayRefObj_New(CFMutableArrayRef);
extern int CFMutableArrayRefObj_Convert(PyObject *, CFMutableArrayRef *);
extern PyObject *CFDictionaryRefObj_New(CFDictionaryRef);
extern int CFDictionaryRefObj_Convert(PyObject *, CFDictionaryRef *);
extern PyObject *CFMutableDictionaryRefObj_New(CFMutableDictionaryRef);
extern int CFMutableDictionaryRefObj_Convert(PyObject *, CFMutableDictionaryRef *);
extern PyObject *CFURLRefObj_New(CFURLRef);
extern int CFURLRefObj_Convert(PyObject *, CFURLRef *);
extern int OptionalCFURLRefObj_Convert(PyObject *, CFURLRef *);

#ifdef __cplusplus
	}
#endif
#endif
