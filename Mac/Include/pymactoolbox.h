/*
** pymactoolbox.h - global routines exported by the toolbox modules
*/

#ifdef __cplusplus
	extern "C" {
#endif

#ifdef WITHOUT_FRAMEWORKS
#include <Memory.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Controls.h>
#include <Components.h>
#include <Lists.h>
#include <Movies.h>
#include <Errors.h>
#else
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
#endif

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

/* AE exports */
extern PyObject *AEDesc_New(AppleEvent *); /* XXXX Why passed by address?? */
extern int AEDesc_Convert(PyObject *, AppleEvent *);

/* Cm exports */
extern PyObject *CmpObj_New(Component);
extern int CmpObj_Convert(PyObject *, Component *);
extern PyObject *CmpInstObj_New(ComponentInstance);
extern int CmpInstObj_Convert(PyObject *, ComponentInstance *);

/* Ctl exports */
extern PyObject *CtlObj_New(ControlHandle);
extern int CtlObj_Convert(PyObject *, ControlHandle *);

/* Dlg exports */
extern PyObject *DlgObj_New(DialogPtr);
extern int DlgObj_Convert(PyObject *, DialogPtr *);
extern PyObject *DlgObj_WhichDialog(DialogPtr);

/* Drag exports */
extern PyObject *DragObj_New(DragReference);
extern int DragObj_Convert(PyObject *, DragReference *);

/* List exports */
extern PyObject *ListObj_New(ListHandle);
extern int ListObj_Convert(PyObject *, ListHandle *);

/* Menu exports */
extern PyObject *MenuObj_New(MenuHandle);
extern int MenuObj_Convert(PyObject *, MenuHandle *);

/* Qd exports */
extern PyObject *GrafObj_New(GrafPtr);
extern int GrafObj_Convert(PyObject *, GrafPtr *);
extern PyObject *BMObj_New(BitMapPtr);
extern int BMObj_Convert(PyObject *, BitMapPtr *);
extern PyObject *QdRGB_New(RGBColor *);
extern int QdRGB_Convert(PyObject *, RGBColor *);

/* Qdoffs exports */
extern PyObject *GWorldObj_New(GWorldPtr);
extern int GWorldObj_Convert(PyObject *, GWorldPtr *);

/* Qt exports */
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

/* Res exports */
extern PyObject *ResObj_New(Handle);
extern int ResObj_Convert(PyObject *, Handle *);
extern PyObject *OptResObj_New(Handle);
extern int OptResObj_Convert(PyObject *, Handle *);

/* TE exports */
extern PyObject *TEObj_New(TEHandle);
extern int TEObj_Convert(PyObject *, TEHandle *);

/* Win exports */
extern PyObject *WinObj_New(WindowPtr);
extern int WinObj_Convert(PyObject *, WindowPtr *);
extern PyObject *WinObj_WhichWindow(WindowPtr);


#ifdef __cplusplus
	}
#endif
