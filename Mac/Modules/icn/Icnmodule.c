
/* =========================== Module Icn =========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <Icons.h>
#else
#include <Carbon/Carbon.h>
#endif


static PyObject *Icn_Error;

static PyObject *Icn_GetCIcon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CIconHandle _rv;
	SInt16 iconID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &iconID))
		return NULL;
	_rv = GetCIcon(iconID);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Icn_PlotCIcon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect theRect;
	CIconHandle theIcon;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &theRect,
	                      ResObj_Convert, &theIcon))
		return NULL;
	PlotCIcon(&theRect,
	          theIcon);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_DisposeCIcon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CIconHandle theIcon;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theIcon))
		return NULL;
	DisposeCIcon(theIcon);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetIcon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Handle _rv;
	SInt16 iconID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &iconID))
		return NULL;
	_rv = GetIcon(iconID);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Icn_PlotIcon(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect theRect;
	Handle theIcon;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &theRect,
	                      ResObj_Convert, &theIcon))
		return NULL;
	PlotIcon(&theRect,
	         theIcon);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_PlotIconID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	SInt16 theResID;
	if (!PyArg_ParseTuple(_args, "O&hhh",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      &theResID))
		return NULL;
	_err = PlotIconID(&theRect,
	                  align,
	                  transform,
	                  theResID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_NewIconSuite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconSuiteRef theIconSuite;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = NewIconSuite(&theIconSuite);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, theIconSuite);
	return _res;
}

static PyObject *Icn_AddIconToSuite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle theIconData;
	IconSuiteRef theSuite;
	ResType theType;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &theIconData,
	                      ResObj_Convert, &theSuite,
	                      PyMac_GetOSType, &theType))
		return NULL;
	_err = AddIconToSuite(theIconData,
	                      theSuite,
	                      theType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetIconFromSuite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle theIconData;
	IconSuiteRef theSuite;
	ResType theType;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &theSuite,
	                      PyMac_GetOSType, &theType))
		return NULL;
	_err = GetIconFromSuite(&theIconData,
	                        theSuite,
	                        theType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, theIconData);
	return _res;
}

static PyObject *Icn_GetIconSuite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconSuiteRef theIconSuite;
	SInt16 theResID;
	IconSelectorValue selector;
	if (!PyArg_ParseTuple(_args, "hl",
	                      &theResID,
	                      &selector))
		return NULL;
	_err = GetIconSuite(&theIconSuite,
	                    theResID,
	                    selector);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, theIconSuite);
	return _res;
}

static PyObject *Icn_DisposeIconSuite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconSuiteRef theIconSuite;
	Boolean disposeData;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      ResObj_Convert, &theIconSuite,
	                      &disposeData))
		return NULL;
	_err = DisposeIconSuite(theIconSuite,
	                        disposeData);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_PlotIconSuite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	IconSuiteRef theIconSuite;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      ResObj_Convert, &theIconSuite))
		return NULL;
	_err = PlotIconSuite(&theRect,
	                     align,
	                     transform,
	                     theIconSuite);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_LoadIconCache(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	IconCacheRef theIconCache;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      ResObj_Convert, &theIconCache))
		return NULL;
	_err = LoadIconCache(&theRect,
	                     align,
	                     transform,
	                     theIconCache);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetLabel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 labelNumber;
	RGBColor labelColor;
	Str255 labelString;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &labelNumber,
	                      PyMac_GetStr255, labelString))
		return NULL;
	_err = GetLabel(labelNumber,
	                &labelColor,
	                labelString);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     QdRGB_New, &labelColor);
	return _res;
}

static PyObject *Icn_PtInIconID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point testPt;
	Rect iconRect;
	IconAlignmentType align;
	SInt16 iconID;
	if (!PyArg_ParseTuple(_args, "O&O&hh",
	                      PyMac_GetPoint, &testPt,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      &iconID))
		return NULL;
	_rv = PtInIconID(testPt,
	                 &iconRect,
	                 align,
	                 iconID);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Icn_PtInIconSuite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point testPt;
	Rect iconRect;
	IconAlignmentType align;
	IconSuiteRef theIconSuite;
	if (!PyArg_ParseTuple(_args, "O&O&hO&",
	                      PyMac_GetPoint, &testPt,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      ResObj_Convert, &theIconSuite))
		return NULL;
	_rv = PtInIconSuite(testPt,
	                    &iconRect,
	                    align,
	                    theIconSuite);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Icn_RectInIconID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Rect testRect;
	Rect iconRect;
	IconAlignmentType align;
	SInt16 iconID;
	if (!PyArg_ParseTuple(_args, "O&O&hh",
	                      PyMac_GetRect, &testRect,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      &iconID))
		return NULL;
	_rv = RectInIconID(&testRect,
	                   &iconRect,
	                   align,
	                   iconID);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Icn_RectInIconSuite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Rect testRect;
	Rect iconRect;
	IconAlignmentType align;
	IconSuiteRef theIconSuite;
	if (!PyArg_ParseTuple(_args, "O&O&hO&",
	                      PyMac_GetRect, &testRect,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      ResObj_Convert, &theIconSuite))
		return NULL;
	_rv = RectInIconSuite(&testRect,
	                      &iconRect,
	                      align,
	                      theIconSuite);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Icn_IconIDToRgn(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	RgnHandle theRgn;
	Rect iconRect;
	IconAlignmentType align;
	SInt16 iconID;
	if (!PyArg_ParseTuple(_args, "O&O&hh",
	                      ResObj_Convert, &theRgn,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      &iconID))
		return NULL;
	_err = IconIDToRgn(theRgn,
	                   &iconRect,
	                   align,
	                   iconID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_IconSuiteToRgn(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	RgnHandle theRgn;
	Rect iconRect;
	IconAlignmentType align;
	IconSuiteRef theIconSuite;
	if (!PyArg_ParseTuple(_args, "O&O&hO&",
	                      ResObj_Convert, &theRgn,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      ResObj_Convert, &theIconSuite))
		return NULL;
	_err = IconSuiteToRgn(theRgn,
	                      &iconRect,
	                      align,
	                      theIconSuite);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_SetSuiteLabel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconSuiteRef theSuite;
	SInt16 theLabel;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      ResObj_Convert, &theSuite,
	                      &theLabel))
		return NULL;
	_err = SetSuiteLabel(theSuite,
	                     theLabel);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetSuiteLabel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	IconSuiteRef theSuite;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theSuite))
		return NULL;
	_rv = GetSuiteLabel(theSuite);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Icn_PlotIconHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	Handle theIcon;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      ResObj_Convert, &theIcon))
		return NULL;
	_err = PlotIconHandle(&theRect,
	                      align,
	                      transform,
	                      theIcon);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_PlotSICNHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	Handle theSICN;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      ResObj_Convert, &theSICN))
		return NULL;
	_err = PlotSICNHandle(&theRect,
	                      align,
	                      transform,
	                      theSICN);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_PlotCIconHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	CIconHandle theCIcon;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      ResObj_Convert, &theCIcon))
		return NULL;
	_err = PlotCIconHandle(&theRect,
	                       align,
	                       transform,
	                       theCIcon);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#if !TARGET_API_MAC_CARBON

static PyObject *Icn_IconServicesTerminate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	IconServicesTerminate();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *Icn_IconRefToIconFamily(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconRef theIconRef;
	IconSelectorValue whichIcons;
	IconFamilyHandle iconFamily;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      ResObj_Convert, &theIconRef,
	                      &whichIcons))
		return NULL;
	_err = IconRefToIconFamily(theIconRef,
	                           whichIcons,
	                           &iconFamily);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, iconFamily);
	return _res;
}

static PyObject *Icn_IconFamilyToIconSuite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconFamilyHandle iconFamily;
	IconSelectorValue whichIcons;
	IconSuiteRef iconSuite;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      ResObj_Convert, &iconFamily,
	                      &whichIcons))
		return NULL;
	_err = IconFamilyToIconSuite(iconFamily,
	                             whichIcons,
	                             &iconSuite);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, iconSuite);
	return _res;
}

static PyObject *Icn_IconSuiteToIconFamily(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconSuiteRef iconSuite;
	IconSelectorValue whichIcons;
	IconFamilyHandle iconFamily;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      ResObj_Convert, &iconSuite,
	                      &whichIcons))
		return NULL;
	_err = IconSuiteToIconFamily(iconSuite,
	                             whichIcons,
	                             &iconFamily);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, iconFamily);
	return _res;
}

static PyObject *Icn_SetIconFamilyData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconFamilyHandle iconFamily;
	OSType iconType;
	Handle h;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &iconFamily,
	                      PyMac_GetOSType, &iconType,
	                      ResObj_Convert, &h))
		return NULL;
	_err = SetIconFamilyData(iconFamily,
	                         iconType,
	                         h);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetIconFamilyData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconFamilyHandle iconFamily;
	OSType iconType;
	Handle h;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &iconFamily,
	                      PyMac_GetOSType, &iconType,
	                      ResObj_Convert, &h))
		return NULL;
	_err = GetIconFamilyData(iconFamily,
	                         iconType,
	                         h);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetIconRefOwners(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconRef theIconRef;
	UInt16 owners;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theIconRef))
		return NULL;
	_err = GetIconRefOwners(theIconRef,
	                        &owners);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("H",
	                     owners);
	return _res;
}

static PyObject *Icn_AcquireIconRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theIconRef))
		return NULL;
	_err = AcquireIconRef(theIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_ReleaseIconRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theIconRef))
		return NULL;
	_err = ReleaseIconRef(theIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetIconRefFromFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec theFile;
	IconRef theIconRef;
	SInt16 theLabel;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &theFile))
		return NULL;
	_err = GetIconRefFromFile(&theFile,
	                          &theIconRef,
	                          &theLabel);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&h",
	                     ResObj_New, theIconRef,
	                     theLabel);
	return _res;
}

static PyObject *Icn_GetIconRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 vRefNum;
	OSType creator;
	OSType iconType;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "hO&O&",
	                      &vRefNum,
	                      PyMac_GetOSType, &creator,
	                      PyMac_GetOSType, &iconType))
		return NULL;
	_err = GetIconRef(vRefNum,
	                  creator,
	                  iconType,
	                  &theIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, theIconRef);
	return _res;
}

static PyObject *Icn_GetIconRefFromFolder(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 vRefNum;
	SInt32 parentFolderID;
	SInt32 folderID;
	SInt8 attributes;
	SInt8 accessPrivileges;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "hllbb",
	                      &vRefNum,
	                      &parentFolderID,
	                      &folderID,
	                      &attributes,
	                      &accessPrivileges))
		return NULL;
	_err = GetIconRefFromFolder(vRefNum,
	                            parentFolderID,
	                            folderID,
	                            attributes,
	                            accessPrivileges,
	                            &theIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, theIconRef);
	return _res;
}

static PyObject *Icn_RegisterIconRefFromIconFamily(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType creator;
	OSType iconType;
	IconFamilyHandle iconFamily;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      PyMac_GetOSType, &creator,
	                      PyMac_GetOSType, &iconType,
	                      ResObj_Convert, &iconFamily))
		return NULL;
	_err = RegisterIconRefFromIconFamily(creator,
	                                     iconType,
	                                     iconFamily,
	                                     &theIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, theIconRef);
	return _res;
}

static PyObject *Icn_RegisterIconRefFromResource(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType creator;
	OSType iconType;
	FSSpec resourceFile;
	SInt16 resourceID;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "O&O&O&h",
	                      PyMac_GetOSType, &creator,
	                      PyMac_GetOSType, &iconType,
	                      PyMac_GetFSSpec, &resourceFile,
	                      &resourceID))
		return NULL;
	_err = RegisterIconRefFromResource(creator,
	                                   iconType,
	                                   &resourceFile,
	                                   resourceID,
	                                   &theIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, theIconRef);
	return _res;
}

static PyObject *Icn_UnregisterIconRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType creator;
	OSType iconType;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &creator,
	                      PyMac_GetOSType, &iconType))
		return NULL;
	_err = UnregisterIconRef(creator,
	                         iconType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_UpdateIconRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theIconRef))
		return NULL;
	_err = UpdateIconRef(theIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_OverrideIconRefFromResource(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconRef theIconRef;
	FSSpec resourceFile;
	SInt16 resourceID;
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      ResObj_Convert, &theIconRef,
	                      PyMac_GetFSSpec, &resourceFile,
	                      &resourceID))
		return NULL;
	_err = OverrideIconRefFromResource(theIconRef,
	                                   &resourceFile,
	                                   resourceID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_OverrideIconRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconRef oldIconRef;
	IconRef newIconRef;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &oldIconRef,
	                      ResObj_Convert, &newIconRef))
		return NULL;
	_err = OverrideIconRef(oldIconRef,
	                       newIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_RemoveIconRefOverride(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theIconRef))
		return NULL;
	_err = RemoveIconRefOverride(theIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_CompositeIconRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconRef backgroundIconRef;
	IconRef foregroundIconRef;
	IconRef compositeIconRef;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &backgroundIconRef,
	                      ResObj_Convert, &foregroundIconRef))
		return NULL;
	_err = CompositeIconRef(backgroundIconRef,
	                        foregroundIconRef,
	                        &compositeIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, compositeIconRef);
	return _res;
}

static PyObject *Icn_IsIconRefComposite(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconRef compositeIconRef;
	IconRef backgroundIconRef;
	IconRef foregroundIconRef;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &compositeIconRef))
		return NULL;
	_err = IsIconRefComposite(compositeIconRef,
	                          &backgroundIconRef,
	                          &foregroundIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     ResObj_New, backgroundIconRef,
	                     ResObj_New, foregroundIconRef);
	return _res;
}

static PyObject *Icn_IsValidIconRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theIconRef))
		return NULL;
	_rv = IsValidIconRef(theIconRef);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Icn_PlotIconRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	IconServicesUsageFlags theIconServicesUsageFlags;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "O&hhlO&",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      &theIconServicesUsageFlags,
	                      ResObj_Convert, &theIconRef))
		return NULL;
	_err = PlotIconRef(&theRect,
	                   align,
	                   transform,
	                   theIconServicesUsageFlags,
	                   theIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_PtInIconRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point testPt;
	Rect iconRect;
	IconAlignmentType align;
	IconServicesUsageFlags theIconServicesUsageFlags;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "O&O&hlO&",
	                      PyMac_GetPoint, &testPt,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      &theIconServicesUsageFlags,
	                      ResObj_Convert, &theIconRef))
		return NULL;
	_rv = PtInIconRef(&testPt,
	                  &iconRect,
	                  align,
	                  theIconServicesUsageFlags,
	                  theIconRef);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Icn_RectInIconRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Rect testRect;
	Rect iconRect;
	IconAlignmentType align;
	IconServicesUsageFlags iconServicesUsageFlags;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "O&O&hlO&",
	                      PyMac_GetRect, &testRect,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      &iconServicesUsageFlags,
	                      ResObj_Convert, &theIconRef))
		return NULL;
	_rv = RectInIconRef(&testRect,
	                    &iconRect,
	                    align,
	                    iconServicesUsageFlags,
	                    theIconRef);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Icn_IconRefToRgn(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	RgnHandle theRgn;
	Rect iconRect;
	IconAlignmentType align;
	IconServicesUsageFlags iconServicesUsageFlags;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "O&O&hlO&",
	                      ResObj_Convert, &theRgn,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      &iconServicesUsageFlags,
	                      ResObj_Convert, &theIconRef))
		return NULL;
	_err = IconRefToRgn(theRgn,
	                    &iconRect,
	                    align,
	                    iconServicesUsageFlags,
	                    theIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetIconSizesFromIconRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconSelectorValue iconSelectorInput;
	IconSelectorValue iconSelectorOutputPtr;
	IconServicesUsageFlags iconServicesUsageFlags;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "llO&",
	                      &iconSelectorInput,
	                      &iconServicesUsageFlags,
	                      ResObj_Convert, &theIconRef))
		return NULL;
	_err = GetIconSizesFromIconRef(iconSelectorInput,
	                               &iconSelectorOutputPtr,
	                               iconServicesUsageFlags,
	                               theIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     iconSelectorOutputPtr);
	return _res;
}

static PyObject *Icn_FlushIconRefs(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType creator;
	OSType iconType;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &creator,
	                      PyMac_GetOSType, &iconType))
		return NULL;
	_err = FlushIconRefs(creator,
	                     iconType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_FlushIconRefsByVolume(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 vRefNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &vRefNum))
		return NULL;
	_err = FlushIconRefsByVolume(vRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_SetCustomIconsEnabled(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 vRefNum;
	Boolean enableCustomIcons;
	if (!PyArg_ParseTuple(_args, "hb",
	                      &vRefNum,
	                      &enableCustomIcons))
		return NULL;
	_err = SetCustomIconsEnabled(vRefNum,
	                             enableCustomIcons);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetCustomIconsEnabled(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 vRefNum;
	Boolean customIconsEnabled;
	if (!PyArg_ParseTuple(_args, "h",
	                      &vRefNum))
		return NULL;
	_err = GetCustomIconsEnabled(vRefNum,
	                             &customIconsEnabled);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("b",
	                     customIconsEnabled);
	return _res;
}

static PyObject *Icn_IsIconRefMaskEmpty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	IconRef iconRef;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &iconRef))
		return NULL;
	_rv = IsIconRefMaskEmpty(iconRef);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

#if TARGET_API_MAC_CARBON

static PyObject *Icn_GetIconRefVariant(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	IconRef _rv;
	IconRef inIconRef;
	OSType inVariant;
	IconTransformType outTransform;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &inIconRef,
	                      PyMac_GetOSType, &inVariant))
		return NULL;
	_rv = GetIconRefVariant(inIconRef,
	                        inVariant,
	                        &outTransform);
	_res = Py_BuildValue("O&h",
	                     ResObj_New, _rv,
	                     outTransform);
	return _res;
}
#endif

#if TARGET_API_MAC_CARBON

static PyObject *Icn_RegisterIconRefFromIconFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType creator;
	OSType iconType;
	FSSpec iconFile;
	IconRef theIconRef;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      PyMac_GetOSType, &creator,
	                      PyMac_GetOSType, &iconType,
	                      PyMac_GetFSSpec, &iconFile))
		return NULL;
	_err = RegisterIconRefFromIconFile(creator,
	                                   iconType,
	                                   &iconFile,
	                                   &theIconRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, theIconRef);
	return _res;
}
#endif

#if TARGET_API_MAC_CARBON

static PyObject *Icn_ReadIconFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec iconFile;
	IconFamilyHandle iconFamily;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &iconFile))
		return NULL;
	_err = ReadIconFile(&iconFile,
	                    &iconFamily);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, iconFamily);
	return _res;
}
#endif

#if TARGET_API_MAC_CARBON

static PyObject *Icn_WriteIconFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconFamilyHandle iconFamily;
	FSSpec iconFile;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &iconFamily,
	                      PyMac_GetFSSpec, &iconFile))
		return NULL;
	_err = WriteIconFile(iconFamily,
	                     &iconFile);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyMethodDef Icn_methods[] = {
	{"GetCIcon", (PyCFunction)Icn_GetCIcon, 1,
	 "(SInt16 iconID) -> (CIconHandle _rv)"},
	{"PlotCIcon", (PyCFunction)Icn_PlotCIcon, 1,
	 "(Rect theRect, CIconHandle theIcon) -> None"},
	{"DisposeCIcon", (PyCFunction)Icn_DisposeCIcon, 1,
	 "(CIconHandle theIcon) -> None"},
	{"GetIcon", (PyCFunction)Icn_GetIcon, 1,
	 "(SInt16 iconID) -> (Handle _rv)"},
	{"PlotIcon", (PyCFunction)Icn_PlotIcon, 1,
	 "(Rect theRect, Handle theIcon) -> None"},
	{"PlotIconID", (PyCFunction)Icn_PlotIconID, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, SInt16 theResID) -> None"},
	{"NewIconSuite", (PyCFunction)Icn_NewIconSuite, 1,
	 "() -> (IconSuiteRef theIconSuite)"},
	{"AddIconToSuite", (PyCFunction)Icn_AddIconToSuite, 1,
	 "(Handle theIconData, IconSuiteRef theSuite, ResType theType) -> None"},
	{"GetIconFromSuite", (PyCFunction)Icn_GetIconFromSuite, 1,
	 "(IconSuiteRef theSuite, ResType theType) -> (Handle theIconData)"},
	{"GetIconSuite", (PyCFunction)Icn_GetIconSuite, 1,
	 "(SInt16 theResID, IconSelectorValue selector) -> (IconSuiteRef theIconSuite)"},
	{"DisposeIconSuite", (PyCFunction)Icn_DisposeIconSuite, 1,
	 "(IconSuiteRef theIconSuite, Boolean disposeData) -> None"},
	{"PlotIconSuite", (PyCFunction)Icn_PlotIconSuite, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, IconSuiteRef theIconSuite) -> None"},
	{"LoadIconCache", (PyCFunction)Icn_LoadIconCache, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, IconCacheRef theIconCache) -> None"},
	{"GetLabel", (PyCFunction)Icn_GetLabel, 1,
	 "(SInt16 labelNumber, Str255 labelString) -> (RGBColor labelColor)"},
	{"PtInIconID", (PyCFunction)Icn_PtInIconID, 1,
	 "(Point testPt, Rect iconRect, IconAlignmentType align, SInt16 iconID) -> (Boolean _rv)"},
	{"PtInIconSuite", (PyCFunction)Icn_PtInIconSuite, 1,
	 "(Point testPt, Rect iconRect, IconAlignmentType align, IconSuiteRef theIconSuite) -> (Boolean _rv)"},
	{"RectInIconID", (PyCFunction)Icn_RectInIconID, 1,
	 "(Rect testRect, Rect iconRect, IconAlignmentType align, SInt16 iconID) -> (Boolean _rv)"},
	{"RectInIconSuite", (PyCFunction)Icn_RectInIconSuite, 1,
	 "(Rect testRect, Rect iconRect, IconAlignmentType align, IconSuiteRef theIconSuite) -> (Boolean _rv)"},
	{"IconIDToRgn", (PyCFunction)Icn_IconIDToRgn, 1,
	 "(RgnHandle theRgn, Rect iconRect, IconAlignmentType align, SInt16 iconID) -> None"},
	{"IconSuiteToRgn", (PyCFunction)Icn_IconSuiteToRgn, 1,
	 "(RgnHandle theRgn, Rect iconRect, IconAlignmentType align, IconSuiteRef theIconSuite) -> None"},
	{"SetSuiteLabel", (PyCFunction)Icn_SetSuiteLabel, 1,
	 "(IconSuiteRef theSuite, SInt16 theLabel) -> None"},
	{"GetSuiteLabel", (PyCFunction)Icn_GetSuiteLabel, 1,
	 "(IconSuiteRef theSuite) -> (SInt16 _rv)"},
	{"PlotIconHandle", (PyCFunction)Icn_PlotIconHandle, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, Handle theIcon) -> None"},
	{"PlotSICNHandle", (PyCFunction)Icn_PlotSICNHandle, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, Handle theSICN) -> None"},
	{"PlotCIconHandle", (PyCFunction)Icn_PlotCIconHandle, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, CIconHandle theCIcon) -> None"},

#if !TARGET_API_MAC_CARBON
	{"IconServicesTerminate", (PyCFunction)Icn_IconServicesTerminate, 1,
	 "() -> None"},
#endif
	{"IconRefToIconFamily", (PyCFunction)Icn_IconRefToIconFamily, 1,
	 "(IconRef theIconRef, IconSelectorValue whichIcons) -> (IconFamilyHandle iconFamily)"},
	{"IconFamilyToIconSuite", (PyCFunction)Icn_IconFamilyToIconSuite, 1,
	 "(IconFamilyHandle iconFamily, IconSelectorValue whichIcons) -> (IconSuiteRef iconSuite)"},
	{"IconSuiteToIconFamily", (PyCFunction)Icn_IconSuiteToIconFamily, 1,
	 "(IconSuiteRef iconSuite, IconSelectorValue whichIcons) -> (IconFamilyHandle iconFamily)"},
	{"SetIconFamilyData", (PyCFunction)Icn_SetIconFamilyData, 1,
	 "(IconFamilyHandle iconFamily, OSType iconType, Handle h) -> None"},
	{"GetIconFamilyData", (PyCFunction)Icn_GetIconFamilyData, 1,
	 "(IconFamilyHandle iconFamily, OSType iconType, Handle h) -> None"},
	{"GetIconRefOwners", (PyCFunction)Icn_GetIconRefOwners, 1,
	 "(IconRef theIconRef) -> (UInt16 owners)"},
	{"AcquireIconRef", (PyCFunction)Icn_AcquireIconRef, 1,
	 "(IconRef theIconRef) -> None"},
	{"ReleaseIconRef", (PyCFunction)Icn_ReleaseIconRef, 1,
	 "(IconRef theIconRef) -> None"},
	{"GetIconRefFromFile", (PyCFunction)Icn_GetIconRefFromFile, 1,
	 "(FSSpec theFile) -> (IconRef theIconRef, SInt16 theLabel)"},
	{"GetIconRef", (PyCFunction)Icn_GetIconRef, 1,
	 "(SInt16 vRefNum, OSType creator, OSType iconType) -> (IconRef theIconRef)"},
	{"GetIconRefFromFolder", (PyCFunction)Icn_GetIconRefFromFolder, 1,
	 "(SInt16 vRefNum, SInt32 parentFolderID, SInt32 folderID, SInt8 attributes, SInt8 accessPrivileges) -> (IconRef theIconRef)"},
	{"RegisterIconRefFromIconFamily", (PyCFunction)Icn_RegisterIconRefFromIconFamily, 1,
	 "(OSType creator, OSType iconType, IconFamilyHandle iconFamily) -> (IconRef theIconRef)"},
	{"RegisterIconRefFromResource", (PyCFunction)Icn_RegisterIconRefFromResource, 1,
	 "(OSType creator, OSType iconType, FSSpec resourceFile, SInt16 resourceID) -> (IconRef theIconRef)"},
	{"UnregisterIconRef", (PyCFunction)Icn_UnregisterIconRef, 1,
	 "(OSType creator, OSType iconType) -> None"},
	{"UpdateIconRef", (PyCFunction)Icn_UpdateIconRef, 1,
	 "(IconRef theIconRef) -> None"},
	{"OverrideIconRefFromResource", (PyCFunction)Icn_OverrideIconRefFromResource, 1,
	 "(IconRef theIconRef, FSSpec resourceFile, SInt16 resourceID) -> None"},
	{"OverrideIconRef", (PyCFunction)Icn_OverrideIconRef, 1,
	 "(IconRef oldIconRef, IconRef newIconRef) -> None"},
	{"RemoveIconRefOverride", (PyCFunction)Icn_RemoveIconRefOverride, 1,
	 "(IconRef theIconRef) -> None"},
	{"CompositeIconRef", (PyCFunction)Icn_CompositeIconRef, 1,
	 "(IconRef backgroundIconRef, IconRef foregroundIconRef) -> (IconRef compositeIconRef)"},
	{"IsIconRefComposite", (PyCFunction)Icn_IsIconRefComposite, 1,
	 "(IconRef compositeIconRef) -> (IconRef backgroundIconRef, IconRef foregroundIconRef)"},
	{"IsValidIconRef", (PyCFunction)Icn_IsValidIconRef, 1,
	 "(IconRef theIconRef) -> (Boolean _rv)"},
	{"PlotIconRef", (PyCFunction)Icn_PlotIconRef, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, IconServicesUsageFlags theIconServicesUsageFlags, IconRef theIconRef) -> None"},
	{"PtInIconRef", (PyCFunction)Icn_PtInIconRef, 1,
	 "(Point testPt, Rect iconRect, IconAlignmentType align, IconServicesUsageFlags theIconServicesUsageFlags, IconRef theIconRef) -> (Boolean _rv)"},
	{"RectInIconRef", (PyCFunction)Icn_RectInIconRef, 1,
	 "(Rect testRect, Rect iconRect, IconAlignmentType align, IconServicesUsageFlags iconServicesUsageFlags, IconRef theIconRef) -> (Boolean _rv)"},
	{"IconRefToRgn", (PyCFunction)Icn_IconRefToRgn, 1,
	 "(RgnHandle theRgn, Rect iconRect, IconAlignmentType align, IconServicesUsageFlags iconServicesUsageFlags, IconRef theIconRef) -> None"},
	{"GetIconSizesFromIconRef", (PyCFunction)Icn_GetIconSizesFromIconRef, 1,
	 "(IconSelectorValue iconSelectorInput, IconServicesUsageFlags iconServicesUsageFlags, IconRef theIconRef) -> (IconSelectorValue iconSelectorOutputPtr)"},
	{"FlushIconRefs", (PyCFunction)Icn_FlushIconRefs, 1,
	 "(OSType creator, OSType iconType) -> None"},
	{"FlushIconRefsByVolume", (PyCFunction)Icn_FlushIconRefsByVolume, 1,
	 "(SInt16 vRefNum) -> None"},
	{"SetCustomIconsEnabled", (PyCFunction)Icn_SetCustomIconsEnabled, 1,
	 "(SInt16 vRefNum, Boolean enableCustomIcons) -> None"},
	{"GetCustomIconsEnabled", (PyCFunction)Icn_GetCustomIconsEnabled, 1,
	 "(SInt16 vRefNum) -> (Boolean customIconsEnabled)"},
	{"IsIconRefMaskEmpty", (PyCFunction)Icn_IsIconRefMaskEmpty, 1,
	 "(IconRef iconRef) -> (Boolean _rv)"},

#if TARGET_API_MAC_CARBON
	{"GetIconRefVariant", (PyCFunction)Icn_GetIconRefVariant, 1,
	 "(IconRef inIconRef, OSType inVariant) -> (IconRef _rv, IconTransformType outTransform)"},
#endif

#if TARGET_API_MAC_CARBON
	{"RegisterIconRefFromIconFile", (PyCFunction)Icn_RegisterIconRefFromIconFile, 1,
	 "(OSType creator, OSType iconType, FSSpec iconFile) -> (IconRef theIconRef)"},
#endif

#if TARGET_API_MAC_CARBON
	{"ReadIconFile", (PyCFunction)Icn_ReadIconFile, 1,
	 "(FSSpec iconFile) -> (IconFamilyHandle iconFamily)"},
#endif

#if TARGET_API_MAC_CARBON
	{"WriteIconFile", (PyCFunction)Icn_WriteIconFile, 1,
	 "(IconFamilyHandle iconFamily, FSSpec iconFile) -> None"},
#endif
	{NULL, NULL, 0}
};




void initIcn(void)
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("Icn", Icn_methods);
	d = PyModule_GetDict(m);
	Icn_Error = PyMac_GetOSErrException();
	if (Icn_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Icn_Error) != 0)
		return;
}

/* ========================= End module Icn ========================= */

