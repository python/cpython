
/* ========================== Module _File ========================== */

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
#include <Files.h>
#else
#include <Carbon/Carbon.h>
#endif

/*
** Parse/generate objsect
*/
static PyObject *
PyMac_BuildHFSUniStr255(HFSUniStr255 *itself)
{

	return Py_BuildValue("u#", itself->unicode, itself->length);
}

#if 0
static int
PyMac_GetHFSUniStr255(PyObject *v, HFSUniStr255 *itself)
{
	return PyArg_ParseTuple(v, "O&O&O&O&O&",
		PyMac_GetFixed, &itself->ascent,
		PyMac_GetFixed, &itself->descent,
		PyMac_GetFixed, &itself->leading,
		PyMac_GetFixed, &itself->widMax,
		ResObj_Convert, &itself->wTabHandle);
}
#endif

/*
** Parse/generate objsect
*/
static PyObject *
PyMac_BuildFInfo(FInfo *itself)
{

	return Py_BuildValue("O&O&HO&h",
		PyMac_BuildOSType, itself->fdType,
		PyMac_BuildOSType, itself->fdCreator,
		itself->fdFlags,
		PyMac_BuildPoint, &itself->fdLocation,
		itself->fdFldr);
}

static int
PyMac_GetFInfo(PyObject *v, FInfo *itself)
{
	return PyArg_ParseTuple(v, "O&O&HO&h",
		PyMac_GetOSType, &itself->fdType,
		PyMac_GetOSType, &itself->fdCreator,
		&itself->fdFlags,
		PyMac_GetPoint, &itself->fdLocation,
		&itself->fdFldr);
}


static PyObject *File_Error;

static PyObject *File_UnmountVol(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Str63 volName;
	short vRefNum;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetStr255, volName,
	                      &vRefNum))
		return NULL;
	_err = UnmountVol(volName,
	                  vRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FlushVol(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Str63 volName;
	short vRefNum;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetStr255, volName,
	                      &vRefNum))
		return NULL;
	_err = FlushVol(volName,
	                vRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_HSetVol(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Str63 volName;
	short vRefNum;
	long dirID;
	if (!PyArg_ParseTuple(_args, "O&hl",
	                      PyMac_GetStr255, volName,
	                      &vRefNum,
	                      &dirID))
		return NULL;
	_err = HSetVol(volName,
	               vRefNum,
	               dirID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSClose(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short refNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &refNum))
		return NULL;
	_err = FSClose(refNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_Allocate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short refNum;
	long count;
	if (!PyArg_ParseTuple(_args, "h",
	                      &refNum))
		return NULL;
	_err = Allocate(refNum,
	                &count);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     count);
	return _res;
}

static PyObject *File_GetEOF(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short refNum;
	long logEOF;
	if (!PyArg_ParseTuple(_args, "h",
	                      &refNum))
		return NULL;
	_err = GetEOF(refNum,
	              &logEOF);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     logEOF);
	return _res;
}

static PyObject *File_SetEOF(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short refNum;
	long logEOF;
	if (!PyArg_ParseTuple(_args, "hl",
	                      &refNum,
	                      &logEOF))
		return NULL;
	_err = SetEOF(refNum,
	              logEOF);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_GetFPos(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short refNum;
	long filePos;
	if (!PyArg_ParseTuple(_args, "h",
	                      &refNum))
		return NULL;
	_err = GetFPos(refNum,
	               &filePos);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     filePos);
	return _res;
}

static PyObject *File_SetFPos(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short refNum;
	short posMode;
	long posOff;
	if (!PyArg_ParseTuple(_args, "hhl",
	                      &refNum,
	                      &posMode,
	                      &posOff))
		return NULL;
	_err = SetFPos(refNum,
	               posMode,
	               posOff);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_GetVRefNum(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short fileRefNum;
	short vRefNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &fileRefNum))
		return NULL;
	_err = GetVRefNum(fileRefNum,
	                  &vRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     vRefNum);
	return _res;
}

static PyObject *File_HGetVol(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	StringPtr volName;
	short vRefNum;
	long dirID;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, &volName))
		return NULL;
	_err = HGetVol(volName,
	               &vRefNum,
	               &dirID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("hl",
	                     vRefNum,
	                     dirID);
	return _res;
}

static PyObject *File_HOpen(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	Str255 fileName;
	SInt8 permission;
	short refNum;
	if (!PyArg_ParseTuple(_args, "hlO&b",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, fileName,
	                      &permission))
		return NULL;
	_err = HOpen(vRefNum,
	             dirID,
	             fileName,
	             permission,
	             &refNum);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     refNum);
	return _res;
}

static PyObject *File_HOpenDF(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	Str255 fileName;
	SInt8 permission;
	short refNum;
	if (!PyArg_ParseTuple(_args, "hlO&b",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, fileName,
	                      &permission))
		return NULL;
	_err = HOpenDF(vRefNum,
	               dirID,
	               fileName,
	               permission,
	               &refNum);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     refNum);
	return _res;
}

static PyObject *File_HOpenRF(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	Str255 fileName;
	SInt8 permission;
	short refNum;
	if (!PyArg_ParseTuple(_args, "hlO&b",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, fileName,
	                      &permission))
		return NULL;
	_err = HOpenRF(vRefNum,
	               dirID,
	               fileName,
	               permission,
	               &refNum);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     refNum);
	return _res;
}

static PyObject *File_AllocContig(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short refNum;
	long count;
	if (!PyArg_ParseTuple(_args, "h",
	                      &refNum))
		return NULL;
	_err = AllocContig(refNum,
	                   &count);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     count);
	return _res;
}

static PyObject *File_HCreate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	Str255 fileName;
	OSType creator;
	OSType fileType;
	if (!PyArg_ParseTuple(_args, "hlO&O&O&",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, fileName,
	                      PyMac_GetOSType, &creator,
	                      PyMac_GetOSType, &fileType))
		return NULL;
	_err = HCreate(vRefNum,
	               dirID,
	               fileName,
	               creator,
	               fileType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_DirCreate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long parentDirID;
	Str255 directoryName;
	long createdDirID;
	if (!PyArg_ParseTuple(_args, "hlO&",
	                      &vRefNum,
	                      &parentDirID,
	                      PyMac_GetStr255, directoryName))
		return NULL;
	_err = DirCreate(vRefNum,
	                 parentDirID,
	                 directoryName,
	                 &createdDirID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     createdDirID);
	return _res;
}

static PyObject *File_HDelete(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	Str255 fileName;
	if (!PyArg_ParseTuple(_args, "hlO&",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, fileName))
		return NULL;
	_err = HDelete(vRefNum,
	               dirID,
	               fileName);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_HGetFInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	Str255 fileName;
	FInfo fndrInfo;
	if (!PyArg_ParseTuple(_args, "hlO&",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, fileName))
		return NULL;
	_err = HGetFInfo(vRefNum,
	                 dirID,
	                 fileName,
	                 &fndrInfo);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFInfo, &fndrInfo);
	return _res;
}

static PyObject *File_HSetFInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	Str255 fileName;
	FInfo fndrInfo;
	if (!PyArg_ParseTuple(_args, "hlO&O&",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, fileName,
	                      PyMac_GetFInfo, &fndrInfo))
		return NULL;
	_err = HSetFInfo(vRefNum,
	                 dirID,
	                 fileName,
	                 &fndrInfo);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_HSetFLock(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	Str255 fileName;
	if (!PyArg_ParseTuple(_args, "hlO&",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, fileName))
		return NULL;
	_err = HSetFLock(vRefNum,
	                 dirID,
	                 fileName);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_HRstFLock(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	Str255 fileName;
	if (!PyArg_ParseTuple(_args, "hlO&",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, fileName))
		return NULL;
	_err = HRstFLock(vRefNum,
	                 dirID,
	                 fileName);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_HRename(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	Str255 oldName;
	Str255 newName;
	if (!PyArg_ParseTuple(_args, "hlO&O&",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, oldName,
	                      PyMac_GetStr255, newName))
		return NULL;
	_err = HRename(vRefNum,
	               dirID,
	               oldName,
	               newName);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_CatMove(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	Str255 oldName;
	long newDirID;
	Str255 newName;
	if (!PyArg_ParseTuple(_args, "hlO&lO&",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, oldName,
	                      &newDirID,
	                      PyMac_GetStr255, newName))
		return NULL;
	_err = CatMove(vRefNum,
	               dirID,
	               oldName,
	               newDirID,
	               newName);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSMakeFSSpec(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	Str255 fileName;
	FSSpec spec;
	if (!PyArg_ParseTuple(_args, "hlO&",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, fileName))
		return NULL;
	_err = FSMakeFSSpec(vRefNum,
	                    dirID,
	                    fileName,
	                    &spec);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFSSpec, &spec);
	return _res;
}

static PyObject *File_FSpOpenDF(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec spec;
	SInt8 permission;
	short refNum;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      PyMac_GetFSSpec, &spec,
	                      &permission))
		return NULL;
	_err = FSpOpenDF(&spec,
	                 permission,
	                 &refNum);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     refNum);
	return _res;
}

static PyObject *File_FSpOpenRF(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec spec;
	SInt8 permission;
	short refNum;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      PyMac_GetFSSpec, &spec,
	                      &permission))
		return NULL;
	_err = FSpOpenRF(&spec,
	                 permission,
	                 &refNum);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     refNum);
	return _res;
}

static PyObject *File_FSpCreate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec spec;
	OSType creator;
	OSType fileType;
	ScriptCode scriptTag;
	if (!PyArg_ParseTuple(_args, "O&O&O&h",
	                      PyMac_GetFSSpec, &spec,
	                      PyMac_GetOSType, &creator,
	                      PyMac_GetOSType, &fileType,
	                      &scriptTag))
		return NULL;
	_err = FSpCreate(&spec,
	                 creator,
	                 fileType,
	                 scriptTag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSpDirCreate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec spec;
	ScriptCode scriptTag;
	long createdDirID;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetFSSpec, &spec,
	                      &scriptTag))
		return NULL;
	_err = FSpDirCreate(&spec,
	                    scriptTag,
	                    &createdDirID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     createdDirID);
	return _res;
}

static PyObject *File_FSpDelete(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec spec;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &spec))
		return NULL;
	_err = FSpDelete(&spec);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSpGetFInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec spec;
	FInfo fndrInfo;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &spec))
		return NULL;
	_err = FSpGetFInfo(&spec,
	                   &fndrInfo);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFInfo, &fndrInfo);
	return _res;
}

static PyObject *File_FSpSetFInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec spec;
	FInfo fndrInfo;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFSSpec, &spec,
	                      PyMac_GetFInfo, &fndrInfo))
		return NULL;
	_err = FSpSetFInfo(&spec,
	                   &fndrInfo);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSpSetFLock(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec spec;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &spec))
		return NULL;
	_err = FSpSetFLock(&spec);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSpRstFLock(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec spec;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &spec))
		return NULL;
	_err = FSpRstFLock(&spec);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSpRename(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec spec;
	Str255 newName;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFSSpec, &spec,
	                      PyMac_GetStr255, newName))
		return NULL;
	_err = FSpRename(&spec,
	                 newName);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSpCatMove(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec source;
	FSSpec dest;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFSSpec, &source,
	                      PyMac_GetFSSpec, &dest))
		return NULL;
	_err = FSpCatMove(&source,
	                  &dest);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSpExchangeFiles(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec source;
	FSSpec dest;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFSSpec, &source,
	                      PyMac_GetFSSpec, &dest))
		return NULL;
	_err = FSpExchangeFiles(&source,
	                        &dest);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSpMakeFSRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec source;
	FSRef newRef;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &source))
		return NULL;
	_err = FSpMakeFSRef(&source,
	                    &newRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFSRef, &newRef);
	return _res;
}

static PyObject *File_FSMakeFSRefUnicode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef parentRef;
	UniChar *nameLength__in__;
	UniCharCount nameLength__len__;
	int nameLength__in_len__;
	TextEncoding textEncodingHint;
	FSRef newRef;
	if (!PyArg_ParseTuple(_args, "O&u#l",
	                      PyMac_GetFSRef, &parentRef,
	                      &nameLength__in__, &nameLength__in_len__,
	                      &textEncodingHint))
		return NULL;
	nameLength__len__ = nameLength__in_len__;
	_err = FSMakeFSRefUnicode(&parentRef,
	                          nameLength__len__, nameLength__in__,
	                          textEncodingHint,
	                          &newRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFSRef, &newRef);
	return _res;
}

static PyObject *File_FSCompareFSRefs(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef ref1;
	FSRef ref2;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFSRef, &ref1,
	                      PyMac_GetFSRef, &ref2))
		return NULL;
	_err = FSCompareFSRefs(&ref1,
	                       &ref2);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSDeleteObject(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef ref;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSRef, &ref))
		return NULL;
	_err = FSDeleteObject(&ref);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSMoveObject(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef ref;
	FSRef destDirectory;
	FSRef newRef;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFSRef, &ref,
	                      PyMac_GetFSRef, &destDirectory))
		return NULL;
	_err = FSMoveObject(&ref,
	                    &destDirectory,
	                    &newRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFSRef, &newRef);
	return _res;
}

static PyObject *File_FSExchangeObjects(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef ref;
	FSRef destRef;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFSRef, &ref,
	                      PyMac_GetFSRef, &destRef))
		return NULL;
	_err = FSExchangeObjects(&ref,
	                         &destRef);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSRenameUnicode(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef ref;
	UniChar *nameLength__in__;
	UniCharCount nameLength__len__;
	int nameLength__in_len__;
	TextEncoding textEncodingHint;
	FSRef newRef;
	if (!PyArg_ParseTuple(_args, "O&u#l",
	                      PyMac_GetFSRef, &ref,
	                      &nameLength__in__, &nameLength__in_len__,
	                      &textEncodingHint))
		return NULL;
	nameLength__len__ = nameLength__in_len__;
	_err = FSRenameUnicode(&ref,
	                       nameLength__len__, nameLength__in__,
	                       textEncodingHint,
	                       &newRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFSRef, &newRef);
	return _res;
}

static PyObject *File_FSCreateFork(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef ref;
	UniChar *forkNameLength__in__;
	UniCharCount forkNameLength__len__;
	int forkNameLength__in_len__;
	if (!PyArg_ParseTuple(_args, "O&u#",
	                      PyMac_GetFSRef, &ref,
	                      &forkNameLength__in__, &forkNameLength__in_len__))
		return NULL;
	forkNameLength__len__ = forkNameLength__in_len__;
	_err = FSCreateFork(&ref,
	                    forkNameLength__len__, forkNameLength__in__);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSDeleteFork(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef ref;
	UniChar *forkNameLength__in__;
	UniCharCount forkNameLength__len__;
	int forkNameLength__in_len__;
	if (!PyArg_ParseTuple(_args, "O&u#",
	                      PyMac_GetFSRef, &ref,
	                      &forkNameLength__in__, &forkNameLength__in_len__))
		return NULL;
	forkNameLength__len__ = forkNameLength__in_len__;
	_err = FSDeleteFork(&ref,
	                    forkNameLength__len__, forkNameLength__in__);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSOpenFork(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef ref;
	UniChar *forkNameLength__in__;
	UniCharCount forkNameLength__len__;
	int forkNameLength__in_len__;
	SInt8 permissions;
	SInt16 forkRefNum;
	if (!PyArg_ParseTuple(_args, "O&u#b",
	                      PyMac_GetFSRef, &ref,
	                      &forkNameLength__in__, &forkNameLength__in_len__,
	                      &permissions))
		return NULL;
	forkNameLength__len__ = forkNameLength__in_len__;
	_err = FSOpenFork(&ref,
	                  forkNameLength__len__, forkNameLength__in__,
	                  permissions,
	                  &forkRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     forkRefNum);
	return _res;
}

static PyObject *File_FSGetForkPosition(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 forkRefNum;
	SInt64 position;
	if (!PyArg_ParseTuple(_args, "h",
	                      &forkRefNum))
		return NULL;
	_err = FSGetForkPosition(forkRefNum,
	                         &position);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("L",
	                     position);
	return _res;
}

static PyObject *File_FSSetForkPosition(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 forkRefNum;
	UInt16 positionMode;
	SInt64 positionOffset;
	if (!PyArg_ParseTuple(_args, "hHL",
	                      &forkRefNum,
	                      &positionMode,
	                      &positionOffset))
		return NULL;
	_err = FSSetForkPosition(forkRefNum,
	                         positionMode,
	                         positionOffset);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSGetForkSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 forkRefNum;
	SInt64 forkSize;
	if (!PyArg_ParseTuple(_args, "h",
	                      &forkRefNum))
		return NULL;
	_err = FSGetForkSize(forkRefNum,
	                     &forkSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("L",
	                     forkSize);
	return _res;
}

static PyObject *File_FSSetForkSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 forkRefNum;
	UInt16 positionMode;
	SInt64 positionOffset;
	if (!PyArg_ParseTuple(_args, "hHL",
	                      &forkRefNum,
	                      &positionMode,
	                      &positionOffset))
		return NULL;
	_err = FSSetForkSize(forkRefNum,
	                     positionMode,
	                     positionOffset);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSAllocateFork(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 forkRefNum;
	FSAllocationFlags flags;
	UInt16 positionMode;
	SInt64 positionOffset;
	UInt64 requestCount;
	UInt64 actualCount;
	if (!PyArg_ParseTuple(_args, "hHHLL",
	                      &forkRefNum,
	                      &flags,
	                      &positionMode,
	                      &positionOffset,
	                      &requestCount))
		return NULL;
	_err = FSAllocateFork(forkRefNum,
	                      flags,
	                      positionMode,
	                      positionOffset,
	                      requestCount,
	                      &actualCount);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("L",
	                     actualCount);
	return _res;
}

static PyObject *File_FSFlushFork(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 forkRefNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &forkRefNum))
		return NULL;
	_err = FSFlushFork(forkRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSCloseFork(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 forkRefNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &forkRefNum))
		return NULL;
	_err = FSCloseFork(forkRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSGetDataForkName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	HFSUniStr255 dataForkName;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = FSGetDataForkName(&dataForkName);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildHFSUniStr255, &dataForkName);
	return _res;
}

static PyObject *File_FSGetResourceForkName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	HFSUniStr255 resourceForkName;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = FSGetResourceForkName(&resourceForkName);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildHFSUniStr255, &resourceForkName);
	return _res;
}

static PyObject *File_FSPathMakeRef(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	char* path;
	FSRef ref;
	Boolean isDirectory;
	if (!PyArg_ParseTuple(_args, "s",
	                      &path))
		return NULL;
	_err = FSPathMakeRef(path,
	                     &ref,
	                     &isDirectory);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&b",
	                     PyMac_BuildFSRef, &ref,
	                     isDirectory);
	return _res;
}

static PyObject *File_FNNotify(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSRef ref;
	FNMessage message;
	OptionBits flags;
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      PyMac_GetFSRef, &ref,
	                      &message,
	                      &flags))
		return NULL;
	_err = FNNotify(&ref,
	                message,
	                flags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FNNotifyByPath(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	char* path;
	FNMessage message;
	OptionBits flags;
	if (!PyArg_ParseTuple(_args, "sll",
	                      &path,
	                      &message,
	                      &flags))
		return NULL;
	_err = FNNotifyByPath(path,
	                      message,
	                      flags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FNNotifyAll(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	FNMessage message;
	OptionBits flags;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &message,
	                      &flags))
		return NULL;
	_err = FNNotifyAll(message,
	                   flags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *File_FSRefMakePath(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	OSStatus _err;
	FSRef ref;
#define MAXPATHNAME 1024
	UInt8 path[MAXPATHNAME];
	UInt32 maxPathSize = MAXPATHNAME;

	if (!PyArg_ParseTuple(_args, "O&",
						  PyMac_GetFSRef, &ref))
		return NULL;
	_err = FSRefMakePath(&ref,
						 path,
						 maxPathSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("s", path);
	return _res;

}

static PyMethodDef File_methods[] = {
	{"UnmountVol", (PyCFunction)File_UnmountVol, 1,
	 PyDoc_STR("(Str63 volName, short vRefNum) -> None")},
	{"FlushVol", (PyCFunction)File_FlushVol, 1,
	 PyDoc_STR("(Str63 volName, short vRefNum) -> None")},
	{"HSetVol", (PyCFunction)File_HSetVol, 1,
	 PyDoc_STR("(Str63 volName, short vRefNum, long dirID) -> None")},
	{"FSClose", (PyCFunction)File_FSClose, 1,
	 PyDoc_STR("(short refNum) -> None")},
	{"Allocate", (PyCFunction)File_Allocate, 1,
	 PyDoc_STR("(short refNum) -> (long count)")},
	{"GetEOF", (PyCFunction)File_GetEOF, 1,
	 PyDoc_STR("(short refNum) -> (long logEOF)")},
	{"SetEOF", (PyCFunction)File_SetEOF, 1,
	 PyDoc_STR("(short refNum, long logEOF) -> None")},
	{"GetFPos", (PyCFunction)File_GetFPos, 1,
	 PyDoc_STR("(short refNum) -> (long filePos)")},
	{"SetFPos", (PyCFunction)File_SetFPos, 1,
	 PyDoc_STR("(short refNum, short posMode, long posOff) -> None")},
	{"GetVRefNum", (PyCFunction)File_GetVRefNum, 1,
	 PyDoc_STR("(short fileRefNum) -> (short vRefNum)")},
	{"HGetVol", (PyCFunction)File_HGetVol, 1,
	 PyDoc_STR("(StringPtr volName) -> (short vRefNum, long dirID)")},
	{"HOpen", (PyCFunction)File_HOpen, 1,
	 PyDoc_STR("(short vRefNum, long dirID, Str255 fileName, SInt8 permission) -> (short refNum)")},
	{"HOpenDF", (PyCFunction)File_HOpenDF, 1,
	 PyDoc_STR("(short vRefNum, long dirID, Str255 fileName, SInt8 permission) -> (short refNum)")},
	{"HOpenRF", (PyCFunction)File_HOpenRF, 1,
	 PyDoc_STR("(short vRefNum, long dirID, Str255 fileName, SInt8 permission) -> (short refNum)")},
	{"AllocContig", (PyCFunction)File_AllocContig, 1,
	 PyDoc_STR("(short refNum) -> (long count)")},
	{"HCreate", (PyCFunction)File_HCreate, 1,
	 PyDoc_STR("(short vRefNum, long dirID, Str255 fileName, OSType creator, OSType fileType) -> None")},
	{"DirCreate", (PyCFunction)File_DirCreate, 1,
	 PyDoc_STR("(short vRefNum, long parentDirID, Str255 directoryName) -> (long createdDirID)")},
	{"HDelete", (PyCFunction)File_HDelete, 1,
	 PyDoc_STR("(short vRefNum, long dirID, Str255 fileName) -> None")},
	{"HGetFInfo", (PyCFunction)File_HGetFInfo, 1,
	 PyDoc_STR("(short vRefNum, long dirID, Str255 fileName) -> (FInfo fndrInfo)")},
	{"HSetFInfo", (PyCFunction)File_HSetFInfo, 1,
	 PyDoc_STR("(short vRefNum, long dirID, Str255 fileName, FInfo fndrInfo) -> None")},
	{"HSetFLock", (PyCFunction)File_HSetFLock, 1,
	 PyDoc_STR("(short vRefNum, long dirID, Str255 fileName) -> None")},
	{"HRstFLock", (PyCFunction)File_HRstFLock, 1,
	 PyDoc_STR("(short vRefNum, long dirID, Str255 fileName) -> None")},
	{"HRename", (PyCFunction)File_HRename, 1,
	 PyDoc_STR("(short vRefNum, long dirID, Str255 oldName, Str255 newName) -> None")},
	{"CatMove", (PyCFunction)File_CatMove, 1,
	 PyDoc_STR("(short vRefNum, long dirID, Str255 oldName, long newDirID, Str255 newName) -> None")},
	{"FSMakeFSSpec", (PyCFunction)File_FSMakeFSSpec, 1,
	 PyDoc_STR("(short vRefNum, long dirID, Str255 fileName) -> (FSSpec spec)")},
	{"FSpOpenDF", (PyCFunction)File_FSpOpenDF, 1,
	 PyDoc_STR("(FSSpec spec, SInt8 permission) -> (short refNum)")},
	{"FSpOpenRF", (PyCFunction)File_FSpOpenRF, 1,
	 PyDoc_STR("(FSSpec spec, SInt8 permission) -> (short refNum)")},
	{"FSpCreate", (PyCFunction)File_FSpCreate, 1,
	 PyDoc_STR("(FSSpec spec, OSType creator, OSType fileType, ScriptCode scriptTag) -> None")},
	{"FSpDirCreate", (PyCFunction)File_FSpDirCreate, 1,
	 PyDoc_STR("(FSSpec spec, ScriptCode scriptTag) -> (long createdDirID)")},
	{"FSpDelete", (PyCFunction)File_FSpDelete, 1,
	 PyDoc_STR("(FSSpec spec) -> None")},
	{"FSpGetFInfo", (PyCFunction)File_FSpGetFInfo, 1,
	 PyDoc_STR("(FSSpec spec) -> (FInfo fndrInfo)")},
	{"FSpSetFInfo", (PyCFunction)File_FSpSetFInfo, 1,
	 PyDoc_STR("(FSSpec spec, FInfo fndrInfo) -> None")},
	{"FSpSetFLock", (PyCFunction)File_FSpSetFLock, 1,
	 PyDoc_STR("(FSSpec spec) -> None")},
	{"FSpRstFLock", (PyCFunction)File_FSpRstFLock, 1,
	 PyDoc_STR("(FSSpec spec) -> None")},
	{"FSpRename", (PyCFunction)File_FSpRename, 1,
	 PyDoc_STR("(FSSpec spec, Str255 newName) -> None")},
	{"FSpCatMove", (PyCFunction)File_FSpCatMove, 1,
	 PyDoc_STR("(FSSpec source, FSSpec dest) -> None")},
	{"FSpExchangeFiles", (PyCFunction)File_FSpExchangeFiles, 1,
	 PyDoc_STR("(FSSpec source, FSSpec dest) -> None")},
	{"FSpMakeFSRef", (PyCFunction)File_FSpMakeFSRef, 1,
	 PyDoc_STR("(FSSpec source) -> (FSRef newRef)")},
	{"FSMakeFSRefUnicode", (PyCFunction)File_FSMakeFSRefUnicode, 1,
	 PyDoc_STR("(FSRef parentRef, Buffer nameLength, TextEncoding textEncodingHint) -> (FSRef newRef)")},
	{"FSCompareFSRefs", (PyCFunction)File_FSCompareFSRefs, 1,
	 PyDoc_STR("(FSRef ref1, FSRef ref2) -> None")},
	{"FSDeleteObject", (PyCFunction)File_FSDeleteObject, 1,
	 PyDoc_STR("(FSRef ref) -> None")},
	{"FSMoveObject", (PyCFunction)File_FSMoveObject, 1,
	 PyDoc_STR("(FSRef ref, FSRef destDirectory) -> (FSRef newRef)")},
	{"FSExchangeObjects", (PyCFunction)File_FSExchangeObjects, 1,
	 PyDoc_STR("(FSRef ref, FSRef destRef) -> None")},
	{"FSRenameUnicode", (PyCFunction)File_FSRenameUnicode, 1,
	 PyDoc_STR("(FSRef ref, Buffer nameLength, TextEncoding textEncodingHint) -> (FSRef newRef)")},
	{"FSCreateFork", (PyCFunction)File_FSCreateFork, 1,
	 PyDoc_STR("(FSRef ref, Buffer forkNameLength) -> None")},
	{"FSDeleteFork", (PyCFunction)File_FSDeleteFork, 1,
	 PyDoc_STR("(FSRef ref, Buffer forkNameLength) -> None")},
	{"FSOpenFork", (PyCFunction)File_FSOpenFork, 1,
	 PyDoc_STR("(FSRef ref, Buffer forkNameLength, SInt8 permissions) -> (SInt16 forkRefNum)")},
	{"FSGetForkPosition", (PyCFunction)File_FSGetForkPosition, 1,
	 PyDoc_STR("(SInt16 forkRefNum) -> (SInt64 position)")},
	{"FSSetForkPosition", (PyCFunction)File_FSSetForkPosition, 1,
	 PyDoc_STR("(SInt16 forkRefNum, UInt16 positionMode, SInt64 positionOffset) -> None")},
	{"FSGetForkSize", (PyCFunction)File_FSGetForkSize, 1,
	 PyDoc_STR("(SInt16 forkRefNum) -> (SInt64 forkSize)")},
	{"FSSetForkSize", (PyCFunction)File_FSSetForkSize, 1,
	 PyDoc_STR("(SInt16 forkRefNum, UInt16 positionMode, SInt64 positionOffset) -> None")},
	{"FSAllocateFork", (PyCFunction)File_FSAllocateFork, 1,
	 PyDoc_STR("(SInt16 forkRefNum, FSAllocationFlags flags, UInt16 positionMode, SInt64 positionOffset, UInt64 requestCount) -> (UInt64 actualCount)")},
	{"FSFlushFork", (PyCFunction)File_FSFlushFork, 1,
	 PyDoc_STR("(SInt16 forkRefNum) -> None")},
	{"FSCloseFork", (PyCFunction)File_FSCloseFork, 1,
	 PyDoc_STR("(SInt16 forkRefNum) -> None")},
	{"FSGetDataForkName", (PyCFunction)File_FSGetDataForkName, 1,
	 PyDoc_STR("() -> (HFSUniStr255 dataForkName)")},
	{"FSGetResourceForkName", (PyCFunction)File_FSGetResourceForkName, 1,
	 PyDoc_STR("() -> (HFSUniStr255 resourceForkName)")},
	{"FSPathMakeRef", (PyCFunction)File_FSPathMakeRef, 1,
	 PyDoc_STR("(char* path) -> (FSRef ref, Boolean isDirectory)")},
	{"FNNotify", (PyCFunction)File_FNNotify, 1,
	 PyDoc_STR("(FSRef ref, FNMessage message, OptionBits flags) -> None")},
	{"FNNotifyByPath", (PyCFunction)File_FNNotifyByPath, 1,
	 PyDoc_STR("(char* path, FNMessage message, OptionBits flags) -> None")},
	{"FNNotifyAll", (PyCFunction)File_FNNotifyAll, 1,
	 PyDoc_STR("(FNMessage message, OptionBits flags) -> None")},
	{"FSRefMakePath", (PyCFunction)File_FSRefMakePath, 1,
	 PyDoc_STR("(FSRef) -> string")},
	{NULL, NULL, 0}
};




void init_File(void)
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("_File", File_methods);
	d = PyModule_GetDict(m);
	File_Error = PyMac_GetOSErrException();
	if (File_Error == NULL ||
	    PyDict_SetItemString(d, "Error", File_Error) != 0)
		return;
}

/* ======================== End module _File ======================== */

