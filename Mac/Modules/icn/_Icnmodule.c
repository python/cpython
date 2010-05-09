
/* ========================== Module _Icn =========================== */

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


static PyObject *Icn_Error;

static PyObject *Icn_GetCIcon(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CIconHandle _rv;
    SInt16 iconID;
#ifndef GetCIcon
    PyMac_PRECHECK(GetCIcon);
#endif
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
#ifndef PlotCIcon
    PyMac_PRECHECK(PlotCIcon);
#endif
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
#ifndef DisposeCIcon
    PyMac_PRECHECK(DisposeCIcon);
#endif
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
#ifndef GetIcon
    PyMac_PRECHECK(GetIcon);
#endif
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
#ifndef PlotIcon
    PyMac_PRECHECK(PlotIcon);
#endif
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
#ifndef PlotIconID
    PyMac_PRECHECK(PlotIconID);
#endif
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
#ifndef NewIconSuite
    PyMac_PRECHECK(NewIconSuite);
#endif
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
#ifndef AddIconToSuite
    PyMac_PRECHECK(AddIconToSuite);
#endif
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
#ifndef GetIconFromSuite
    PyMac_PRECHECK(GetIconFromSuite);
#endif
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
#ifndef GetIconSuite
    PyMac_PRECHECK(GetIconSuite);
#endif
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
#ifndef DisposeIconSuite
    PyMac_PRECHECK(DisposeIconSuite);
#endif
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
#ifndef PlotIconSuite
    PyMac_PRECHECK(PlotIconSuite);
#endif
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
#ifndef LoadIconCache
    PyMac_PRECHECK(LoadIconCache);
#endif
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
#ifndef GetLabel
    PyMac_PRECHECK(GetLabel);
#endif
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
#ifndef PtInIconID
    PyMac_PRECHECK(PtInIconID);
#endif
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
#ifndef PtInIconSuite
    PyMac_PRECHECK(PtInIconSuite);
#endif
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
#ifndef RectInIconID
    PyMac_PRECHECK(RectInIconID);
#endif
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
#ifndef RectInIconSuite
    PyMac_PRECHECK(RectInIconSuite);
#endif
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
#ifndef IconIDToRgn
    PyMac_PRECHECK(IconIDToRgn);
#endif
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
#ifndef IconSuiteToRgn
    PyMac_PRECHECK(IconSuiteToRgn);
#endif
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
#ifndef SetSuiteLabel
    PyMac_PRECHECK(SetSuiteLabel);
#endif
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
#ifndef GetSuiteLabel
    PyMac_PRECHECK(GetSuiteLabel);
#endif
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
#ifndef PlotIconHandle
    PyMac_PRECHECK(PlotIconHandle);
#endif
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
#ifndef PlotSICNHandle
    PyMac_PRECHECK(PlotSICNHandle);
#endif
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
#ifndef PlotCIconHandle
    PyMac_PRECHECK(PlotCIconHandle);
#endif
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

static PyObject *Icn_IconRefToIconFamily(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    IconRef theIconRef;
    IconSelectorValue whichIcons;
    IconFamilyHandle iconFamily;
#ifndef IconRefToIconFamily
    PyMac_PRECHECK(IconRefToIconFamily);
#endif
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
#ifndef IconFamilyToIconSuite
    PyMac_PRECHECK(IconFamilyToIconSuite);
#endif
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
#ifndef IconSuiteToIconFamily
    PyMac_PRECHECK(IconSuiteToIconFamily);
#endif
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
#ifndef SetIconFamilyData
    PyMac_PRECHECK(SetIconFamilyData);
#endif
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
#ifndef GetIconFamilyData
    PyMac_PRECHECK(GetIconFamilyData);
#endif
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
#ifndef GetIconRefOwners
    PyMac_PRECHECK(GetIconRefOwners);
#endif
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
#ifndef AcquireIconRef
    PyMac_PRECHECK(AcquireIconRef);
#endif
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
#ifndef ReleaseIconRef
    PyMac_PRECHECK(ReleaseIconRef);
#endif
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
#ifndef GetIconRefFromFile
    PyMac_PRECHECK(GetIconRefFromFile);
#endif
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
#ifndef GetIconRef
    PyMac_PRECHECK(GetIconRef);
#endif
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
#ifndef GetIconRefFromFolder
    PyMac_PRECHECK(GetIconRefFromFolder);
#endif
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
#ifndef RegisterIconRefFromIconFamily
    PyMac_PRECHECK(RegisterIconRefFromIconFamily);
#endif
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
#ifndef RegisterIconRefFromResource
    PyMac_PRECHECK(RegisterIconRefFromResource);
#endif
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

static PyObject *Icn_RegisterIconRefFromFSRef(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    OSType creator;
    OSType iconType;
    FSRef iconFile;
    IconRef theIconRef;
#ifndef RegisterIconRefFromFSRef
    PyMac_PRECHECK(RegisterIconRefFromFSRef);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&",
                          PyMac_GetOSType, &creator,
                          PyMac_GetOSType, &iconType,
                          PyMac_GetFSRef, &iconFile))
        return NULL;
    _err = RegisterIconRefFromFSRef(creator,
                                    iconType,
                                    &iconFile,
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
#ifndef UnregisterIconRef
    PyMac_PRECHECK(UnregisterIconRef);
#endif
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
#ifndef UpdateIconRef
    PyMac_PRECHECK(UpdateIconRef);
#endif
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
#ifndef OverrideIconRefFromResource
    PyMac_PRECHECK(OverrideIconRefFromResource);
#endif
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
#ifndef OverrideIconRef
    PyMac_PRECHECK(OverrideIconRef);
#endif
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
#ifndef RemoveIconRefOverride
    PyMac_PRECHECK(RemoveIconRefOverride);
#endif
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
#ifndef CompositeIconRef
    PyMac_PRECHECK(CompositeIconRef);
#endif
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
#ifndef IsIconRefComposite
    PyMac_PRECHECK(IsIconRefComposite);
#endif
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
#ifndef IsValidIconRef
    PyMac_PRECHECK(IsValidIconRef);
#endif
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
#ifndef PlotIconRef
    PyMac_PRECHECK(PlotIconRef);
#endif
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
#ifndef PtInIconRef
    PyMac_PRECHECK(PtInIconRef);
#endif
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
#ifndef RectInIconRef
    PyMac_PRECHECK(RectInIconRef);
#endif
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
#ifndef IconRefToRgn
    PyMac_PRECHECK(IconRefToRgn);
#endif
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
#ifndef GetIconSizesFromIconRef
    PyMac_PRECHECK(GetIconSizesFromIconRef);
#endif
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
#ifndef FlushIconRefs
    PyMac_PRECHECK(FlushIconRefs);
#endif
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
#ifndef FlushIconRefsByVolume
    PyMac_PRECHECK(FlushIconRefsByVolume);
#endif
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
#ifndef SetCustomIconsEnabled
    PyMac_PRECHECK(SetCustomIconsEnabled);
#endif
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
#ifndef GetCustomIconsEnabled
    PyMac_PRECHECK(GetCustomIconsEnabled);
#endif
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
#ifndef IsIconRefMaskEmpty
    PyMac_PRECHECK(IsIconRefMaskEmpty);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &iconRef))
        return NULL;
    _rv = IsIconRefMaskEmpty(iconRef);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *Icn_GetIconRefVariant(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    IconRef _rv;
    IconRef inIconRef;
    OSType inVariant;
    IconTransformType outTransform;
#ifndef GetIconRefVariant
    PyMac_PRECHECK(GetIconRefVariant);
#endif
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

static PyObject *Icn_RegisterIconRefFromIconFile(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    OSType creator;
    OSType iconType;
    FSSpec iconFile;
    IconRef theIconRef;
#ifndef RegisterIconRefFromIconFile
    PyMac_PRECHECK(RegisterIconRefFromIconFile);
#endif
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

static PyObject *Icn_ReadIconFile(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSSpec iconFile;
    IconFamilyHandle iconFamily;
#ifndef ReadIconFile
    PyMac_PRECHECK(ReadIconFile);
#endif
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

static PyObject *Icn_ReadIconFromFSRef(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    FSRef ref;
    IconFamilyHandle iconFamily;
#ifndef ReadIconFromFSRef
    PyMac_PRECHECK(ReadIconFromFSRef);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetFSRef, &ref))
        return NULL;
    _err = ReadIconFromFSRef(&ref,
                             &iconFamily);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         ResObj_New, iconFamily);
    return _res;
}

static PyObject *Icn_WriteIconFile(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    IconFamilyHandle iconFamily;
    FSSpec iconFile;
#ifndef WriteIconFile
    PyMac_PRECHECK(WriteIconFile);
#endif
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
#endif /* __LP64__ */

static PyMethodDef Icn_methods[] = {
#ifndef __LP64__
    {"GetCIcon", (PyCFunction)Icn_GetCIcon, 1,
     PyDoc_STR("(SInt16 iconID) -> (CIconHandle _rv)")},
    {"PlotCIcon", (PyCFunction)Icn_PlotCIcon, 1,
     PyDoc_STR("(Rect theRect, CIconHandle theIcon) -> None")},
    {"DisposeCIcon", (PyCFunction)Icn_DisposeCIcon, 1,
     PyDoc_STR("(CIconHandle theIcon) -> None")},
    {"GetIcon", (PyCFunction)Icn_GetIcon, 1,
     PyDoc_STR("(SInt16 iconID) -> (Handle _rv)")},
    {"PlotIcon", (PyCFunction)Icn_PlotIcon, 1,
     PyDoc_STR("(Rect theRect, Handle theIcon) -> None")},
    {"PlotIconID", (PyCFunction)Icn_PlotIconID, 1,
     PyDoc_STR("(Rect theRect, IconAlignmentType align, IconTransformType transform, SInt16 theResID) -> None")},
    {"NewIconSuite", (PyCFunction)Icn_NewIconSuite, 1,
     PyDoc_STR("() -> (IconSuiteRef theIconSuite)")},
    {"AddIconToSuite", (PyCFunction)Icn_AddIconToSuite, 1,
     PyDoc_STR("(Handle theIconData, IconSuiteRef theSuite, ResType theType) -> None")},
    {"GetIconFromSuite", (PyCFunction)Icn_GetIconFromSuite, 1,
     PyDoc_STR("(IconSuiteRef theSuite, ResType theType) -> (Handle theIconData)")},
    {"GetIconSuite", (PyCFunction)Icn_GetIconSuite, 1,
     PyDoc_STR("(SInt16 theResID, IconSelectorValue selector) -> (IconSuiteRef theIconSuite)")},
    {"DisposeIconSuite", (PyCFunction)Icn_DisposeIconSuite, 1,
     PyDoc_STR("(IconSuiteRef theIconSuite, Boolean disposeData) -> None")},
    {"PlotIconSuite", (PyCFunction)Icn_PlotIconSuite, 1,
     PyDoc_STR("(Rect theRect, IconAlignmentType align, IconTransformType transform, IconSuiteRef theIconSuite) -> None")},
    {"LoadIconCache", (PyCFunction)Icn_LoadIconCache, 1,
     PyDoc_STR("(Rect theRect, IconAlignmentType align, IconTransformType transform, IconCacheRef theIconCache) -> None")},
    {"GetLabel", (PyCFunction)Icn_GetLabel, 1,
     PyDoc_STR("(SInt16 labelNumber, Str255 labelString) -> (RGBColor labelColor)")},
    {"PtInIconID", (PyCFunction)Icn_PtInIconID, 1,
     PyDoc_STR("(Point testPt, Rect iconRect, IconAlignmentType align, SInt16 iconID) -> (Boolean _rv)")},
    {"PtInIconSuite", (PyCFunction)Icn_PtInIconSuite, 1,
     PyDoc_STR("(Point testPt, Rect iconRect, IconAlignmentType align, IconSuiteRef theIconSuite) -> (Boolean _rv)")},
    {"RectInIconID", (PyCFunction)Icn_RectInIconID, 1,
     PyDoc_STR("(Rect testRect, Rect iconRect, IconAlignmentType align, SInt16 iconID) -> (Boolean _rv)")},
    {"RectInIconSuite", (PyCFunction)Icn_RectInIconSuite, 1,
     PyDoc_STR("(Rect testRect, Rect iconRect, IconAlignmentType align, IconSuiteRef theIconSuite) -> (Boolean _rv)")},
    {"IconIDToRgn", (PyCFunction)Icn_IconIDToRgn, 1,
     PyDoc_STR("(RgnHandle theRgn, Rect iconRect, IconAlignmentType align, SInt16 iconID) -> None")},
    {"IconSuiteToRgn", (PyCFunction)Icn_IconSuiteToRgn, 1,
     PyDoc_STR("(RgnHandle theRgn, Rect iconRect, IconAlignmentType align, IconSuiteRef theIconSuite) -> None")},
    {"SetSuiteLabel", (PyCFunction)Icn_SetSuiteLabel, 1,
     PyDoc_STR("(IconSuiteRef theSuite, SInt16 theLabel) -> None")},
    {"GetSuiteLabel", (PyCFunction)Icn_GetSuiteLabel, 1,
     PyDoc_STR("(IconSuiteRef theSuite) -> (SInt16 _rv)")},
    {"PlotIconHandle", (PyCFunction)Icn_PlotIconHandle, 1,
     PyDoc_STR("(Rect theRect, IconAlignmentType align, IconTransformType transform, Handle theIcon) -> None")},
    {"PlotSICNHandle", (PyCFunction)Icn_PlotSICNHandle, 1,
     PyDoc_STR("(Rect theRect, IconAlignmentType align, IconTransformType transform, Handle theSICN) -> None")},
    {"PlotCIconHandle", (PyCFunction)Icn_PlotCIconHandle, 1,
     PyDoc_STR("(Rect theRect, IconAlignmentType align, IconTransformType transform, CIconHandle theCIcon) -> None")},
    {"IconRefToIconFamily", (PyCFunction)Icn_IconRefToIconFamily, 1,
     PyDoc_STR("(IconRef theIconRef, IconSelectorValue whichIcons) -> (IconFamilyHandle iconFamily)")},
    {"IconFamilyToIconSuite", (PyCFunction)Icn_IconFamilyToIconSuite, 1,
     PyDoc_STR("(IconFamilyHandle iconFamily, IconSelectorValue whichIcons) -> (IconSuiteRef iconSuite)")},
    {"IconSuiteToIconFamily", (PyCFunction)Icn_IconSuiteToIconFamily, 1,
     PyDoc_STR("(IconSuiteRef iconSuite, IconSelectorValue whichIcons) -> (IconFamilyHandle iconFamily)")},
    {"SetIconFamilyData", (PyCFunction)Icn_SetIconFamilyData, 1,
     PyDoc_STR("(IconFamilyHandle iconFamily, OSType iconType, Handle h) -> None")},
    {"GetIconFamilyData", (PyCFunction)Icn_GetIconFamilyData, 1,
     PyDoc_STR("(IconFamilyHandle iconFamily, OSType iconType, Handle h) -> None")},
    {"GetIconRefOwners", (PyCFunction)Icn_GetIconRefOwners, 1,
     PyDoc_STR("(IconRef theIconRef) -> (UInt16 owners)")},
    {"AcquireIconRef", (PyCFunction)Icn_AcquireIconRef, 1,
     PyDoc_STR("(IconRef theIconRef) -> None")},
    {"ReleaseIconRef", (PyCFunction)Icn_ReleaseIconRef, 1,
     PyDoc_STR("(IconRef theIconRef) -> None")},
    {"GetIconRefFromFile", (PyCFunction)Icn_GetIconRefFromFile, 1,
     PyDoc_STR("(FSSpec theFile) -> (IconRef theIconRef, SInt16 theLabel)")},
    {"GetIconRef", (PyCFunction)Icn_GetIconRef, 1,
     PyDoc_STR("(SInt16 vRefNum, OSType creator, OSType iconType) -> (IconRef theIconRef)")},
    {"GetIconRefFromFolder", (PyCFunction)Icn_GetIconRefFromFolder, 1,
     PyDoc_STR("(SInt16 vRefNum, SInt32 parentFolderID, SInt32 folderID, SInt8 attributes, SInt8 accessPrivileges) -> (IconRef theIconRef)")},
    {"RegisterIconRefFromIconFamily", (PyCFunction)Icn_RegisterIconRefFromIconFamily, 1,
     PyDoc_STR("(OSType creator, OSType iconType, IconFamilyHandle iconFamily) -> (IconRef theIconRef)")},
    {"RegisterIconRefFromResource", (PyCFunction)Icn_RegisterIconRefFromResource, 1,
     PyDoc_STR("(OSType creator, OSType iconType, FSSpec resourceFile, SInt16 resourceID) -> (IconRef theIconRef)")},
    {"RegisterIconRefFromFSRef", (PyCFunction)Icn_RegisterIconRefFromFSRef, 1,
     PyDoc_STR("(OSType creator, OSType iconType, FSRef iconFile) -> (IconRef theIconRef)")},
    {"UnregisterIconRef", (PyCFunction)Icn_UnregisterIconRef, 1,
     PyDoc_STR("(OSType creator, OSType iconType) -> None")},
    {"UpdateIconRef", (PyCFunction)Icn_UpdateIconRef, 1,
     PyDoc_STR("(IconRef theIconRef) -> None")},
    {"OverrideIconRefFromResource", (PyCFunction)Icn_OverrideIconRefFromResource, 1,
     PyDoc_STR("(IconRef theIconRef, FSSpec resourceFile, SInt16 resourceID) -> None")},
    {"OverrideIconRef", (PyCFunction)Icn_OverrideIconRef, 1,
     PyDoc_STR("(IconRef oldIconRef, IconRef newIconRef) -> None")},
    {"RemoveIconRefOverride", (PyCFunction)Icn_RemoveIconRefOverride, 1,
     PyDoc_STR("(IconRef theIconRef) -> None")},
    {"CompositeIconRef", (PyCFunction)Icn_CompositeIconRef, 1,
     PyDoc_STR("(IconRef backgroundIconRef, IconRef foregroundIconRef) -> (IconRef compositeIconRef)")},
    {"IsIconRefComposite", (PyCFunction)Icn_IsIconRefComposite, 1,
     PyDoc_STR("(IconRef compositeIconRef) -> (IconRef backgroundIconRef, IconRef foregroundIconRef)")},
    {"IsValidIconRef", (PyCFunction)Icn_IsValidIconRef, 1,
     PyDoc_STR("(IconRef theIconRef) -> (Boolean _rv)")},
    {"PlotIconRef", (PyCFunction)Icn_PlotIconRef, 1,
     PyDoc_STR("(Rect theRect, IconAlignmentType align, IconTransformType transform, IconServicesUsageFlags theIconServicesUsageFlags, IconRef theIconRef) -> None")},
    {"PtInIconRef", (PyCFunction)Icn_PtInIconRef, 1,
     PyDoc_STR("(Point testPt, Rect iconRect, IconAlignmentType align, IconServicesUsageFlags theIconServicesUsageFlags, IconRef theIconRef) -> (Boolean _rv)")},
    {"RectInIconRef", (PyCFunction)Icn_RectInIconRef, 1,
     PyDoc_STR("(Rect testRect, Rect iconRect, IconAlignmentType align, IconServicesUsageFlags iconServicesUsageFlags, IconRef theIconRef) -> (Boolean _rv)")},
    {"IconRefToRgn", (PyCFunction)Icn_IconRefToRgn, 1,
     PyDoc_STR("(RgnHandle theRgn, Rect iconRect, IconAlignmentType align, IconServicesUsageFlags iconServicesUsageFlags, IconRef theIconRef) -> None")},
    {"GetIconSizesFromIconRef", (PyCFunction)Icn_GetIconSizesFromIconRef, 1,
     PyDoc_STR("(IconSelectorValue iconSelectorInput, IconServicesUsageFlags iconServicesUsageFlags, IconRef theIconRef) -> (IconSelectorValue iconSelectorOutputPtr)")},
    {"FlushIconRefs", (PyCFunction)Icn_FlushIconRefs, 1,
     PyDoc_STR("(OSType creator, OSType iconType) -> None")},
    {"FlushIconRefsByVolume", (PyCFunction)Icn_FlushIconRefsByVolume, 1,
     PyDoc_STR("(SInt16 vRefNum) -> None")},
    {"SetCustomIconsEnabled", (PyCFunction)Icn_SetCustomIconsEnabled, 1,
     PyDoc_STR("(SInt16 vRefNum, Boolean enableCustomIcons) -> None")},
    {"GetCustomIconsEnabled", (PyCFunction)Icn_GetCustomIconsEnabled, 1,
     PyDoc_STR("(SInt16 vRefNum) -> (Boolean customIconsEnabled)")},
    {"IsIconRefMaskEmpty", (PyCFunction)Icn_IsIconRefMaskEmpty, 1,
     PyDoc_STR("(IconRef iconRef) -> (Boolean _rv)")},
    {"GetIconRefVariant", (PyCFunction)Icn_GetIconRefVariant, 1,
     PyDoc_STR("(IconRef inIconRef, OSType inVariant) -> (IconRef _rv, IconTransformType outTransform)")},
    {"RegisterIconRefFromIconFile", (PyCFunction)Icn_RegisterIconRefFromIconFile, 1,
     PyDoc_STR("(OSType creator, OSType iconType, FSSpec iconFile) -> (IconRef theIconRef)")},
    {"ReadIconFile", (PyCFunction)Icn_ReadIconFile, 1,
     PyDoc_STR("(FSSpec iconFile) -> (IconFamilyHandle iconFamily)")},
    {"ReadIconFromFSRef", (PyCFunction)Icn_ReadIconFromFSRef, 1,
     PyDoc_STR("(FSRef ref) -> (IconFamilyHandle iconFamily)")},
    {"WriteIconFile", (PyCFunction)Icn_WriteIconFile, 1,
     PyDoc_STR("(IconFamilyHandle iconFamily, FSSpec iconFile) -> None")},
#endif /* __LP64__ */
    {NULL, NULL, 0}
};




void init_Icn(void)
{
    PyObject *m;
#ifndef __LP64__
    PyObject *d;
#endif /* __LP64__ */




    m = Py_InitModule("_Icn", Icn_methods);
#ifndef __LP64__
    d = PyModule_GetDict(m);
    Icn_Error = PyMac_GetOSErrException();
    if (Icn_Error == NULL ||
        PyDict_SetItemString(d, "Error", Icn_Error) != 0)
        return;
#endif /* __LP64__ */
}

/* ======================== End module _Icn ========================= */

