
/* ========================= Module _Folder ========================= */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
        "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>


static PyObject *Folder_Error;

static PyObject *Folder_FindFolder(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	OSType folderType;
	Boolean createFolder;
	short foundVRefNum;
	long foundDirID;
	if (!PyArg_ParseTuple(_args, "hO&b",
	                      &vRefNum,
	                      PyMac_GetOSType, &folderType,
	                      &createFolder))
		return NULL;
	_err = FindFolder(vRefNum,
	                  folderType,
	                  createFolder,
	                  &foundVRefNum,
	                  &foundDirID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("hl",
	                     foundVRefNum,
	                     foundDirID);
	return _res;
}

static PyObject *Folder_ReleaseFolder(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	OSType folderType;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &vRefNum,
	                      PyMac_GetOSType, &folderType))
		return NULL;
	_err = ReleaseFolder(vRefNum,
	                     folderType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Folder_FSFindFolder(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	OSType folderType;
	Boolean createFolder;
	FSRef foundRef;
	if (!PyArg_ParseTuple(_args, "hO&b",
	                      &vRefNum,
	                      PyMac_GetOSType, &folderType,
	                      &createFolder))
		return NULL;
	_err = FSFindFolder(vRefNum,
	                    folderType,
	                    createFolder,
	                    &foundRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFSRef, &foundRef);
	return _res;
}

static PyObject *Folder_AddFolderDescriptor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FolderType foldType;
	FolderDescFlags flags;
	FolderClass foldClass;
	FolderLocation foldLocation;
	OSType badgeSignature;
	OSType badgeType;
	Str255 name;
	Boolean replaceFlag;
	if (!PyArg_ParseTuple(_args, "O&lO&O&O&O&O&b",
	                      PyMac_GetOSType, &foldType,
	                      &flags,
	                      PyMac_GetOSType, &foldClass,
	                      PyMac_GetOSType, &foldLocation,
	                      PyMac_GetOSType, &badgeSignature,
	                      PyMac_GetOSType, &badgeType,
	                      PyMac_GetStr255, name,
	                      &replaceFlag))
		return NULL;
	_err = AddFolderDescriptor(foldType,
	                           flags,
	                           foldClass,
	                           foldLocation,
	                           badgeSignature,
	                           badgeType,
	                           name,
	                           replaceFlag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Folder_GetFolderTypes(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	UInt32 requestedTypeCount;
	UInt32 totalTypeCount;
	FolderType theTypes;
	if (!PyArg_ParseTuple(_args, "l",
	                      &requestedTypeCount))
		return NULL;
	_err = GetFolderTypes(requestedTypeCount,
	                      &totalTypeCount,
	                      &theTypes);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("lO&",
	                     totalTypeCount,
	                     PyMac_BuildOSType, theTypes);
	return _res;
}

static PyObject *Folder_RemoveFolderDescriptor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FolderType foldType;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &foldType))
		return NULL;
	_err = RemoveFolderDescriptor(foldType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Folder_GetFolderName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	OSType foldType;
	short foundVRefNum;
	Str255 name;
	if (!PyArg_ParseTuple(_args, "hO&O&",
	                      &vRefNum,
	                      PyMac_GetOSType, &foldType,
	                      PyMac_GetStr255, name))
		return NULL;
	_err = GetFolderName(vRefNum,
	                     foldType,
	                     &foundVRefNum,
	                     name);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     foundVRefNum);
	return _res;
}

static PyObject *Folder_AddFolderRouting(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType fileType;
	FolderType routeFromFolder;
	FolderType routeToFolder;
	RoutingFlags flags;
	Boolean replaceFlag;
	if (!PyArg_ParseTuple(_args, "O&O&O&lb",
	                      PyMac_GetOSType, &fileType,
	                      PyMac_GetOSType, &routeFromFolder,
	                      PyMac_GetOSType, &routeToFolder,
	                      &flags,
	                      &replaceFlag))
		return NULL;
	_err = AddFolderRouting(fileType,
	                        routeFromFolder,
	                        routeToFolder,
	                        flags,
	                        replaceFlag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Folder_RemoveFolderRouting(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType fileType;
	FolderType routeFromFolder;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &fileType,
	                      PyMac_GetOSType, &routeFromFolder))
		return NULL;
	_err = RemoveFolderRouting(fileType,
	                           routeFromFolder);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Folder_FindFolderRouting(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType fileType;
	FolderType routeFromFolder;
	FolderType routeToFolder;
	RoutingFlags flags;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &fileType,
	                      PyMac_GetOSType, &routeFromFolder))
		return NULL;
	_err = FindFolderRouting(fileType,
	                         routeFromFolder,
	                         &routeToFolder,
	                         &flags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&l",
	                     PyMac_BuildOSType, routeToFolder,
	                     flags);
	return _res;
}

static PyObject *Folder_InvalidateFolderDescriptorCache(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	if (!PyArg_ParseTuple(_args, "hl",
	                      &vRefNum,
	                      &dirID))
		return NULL;
	_err = InvalidateFolderDescriptorCache(vRefNum,
	                                       dirID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Folder_IdentifyFolder(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short vRefNum;
	long dirID;
	FolderType foldType;
	if (!PyArg_ParseTuple(_args, "hl",
	                      &vRefNum,
	                      &dirID))
		return NULL;
	_err = IdentifyFolder(vRefNum,
	                      dirID,
	                      &foldType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildOSType, foldType);
	return _res;
}

static PyMethodDef Folder_methods[] = {
	{"FindFolder", (PyCFunction)Folder_FindFolder, 1,
	 PyDoc_STR("(short vRefNum, OSType folderType, Boolean createFolder) -> (short foundVRefNum, long foundDirID)")},
	{"ReleaseFolder", (PyCFunction)Folder_ReleaseFolder, 1,
	 PyDoc_STR("(short vRefNum, OSType folderType) -> None")},
	{"FSFindFolder", (PyCFunction)Folder_FSFindFolder, 1,
	 PyDoc_STR("(short vRefNum, OSType folderType, Boolean createFolder) -> (FSRef foundRef)")},
	{"AddFolderDescriptor", (PyCFunction)Folder_AddFolderDescriptor, 1,
	 PyDoc_STR("(FolderType foldType, FolderDescFlags flags, FolderClass foldClass, FolderLocation foldLocation, OSType badgeSignature, OSType badgeType, Str255 name, Boolean replaceFlag) -> None")},
	{"GetFolderTypes", (PyCFunction)Folder_GetFolderTypes, 1,
	 PyDoc_STR("(UInt32 requestedTypeCount) -> (UInt32 totalTypeCount, FolderType theTypes)")},
	{"RemoveFolderDescriptor", (PyCFunction)Folder_RemoveFolderDescriptor, 1,
	 PyDoc_STR("(FolderType foldType) -> None")},
	{"GetFolderName", (PyCFunction)Folder_GetFolderName, 1,
	 PyDoc_STR("(short vRefNum, OSType foldType, Str255 name) -> (short foundVRefNum)")},
	{"AddFolderRouting", (PyCFunction)Folder_AddFolderRouting, 1,
	 PyDoc_STR("(OSType fileType, FolderType routeFromFolder, FolderType routeToFolder, RoutingFlags flags, Boolean replaceFlag) -> None")},
	{"RemoveFolderRouting", (PyCFunction)Folder_RemoveFolderRouting, 1,
	 PyDoc_STR("(OSType fileType, FolderType routeFromFolder) -> None")},
	{"FindFolderRouting", (PyCFunction)Folder_FindFolderRouting, 1,
	 PyDoc_STR("(OSType fileType, FolderType routeFromFolder) -> (FolderType routeToFolder, RoutingFlags flags)")},
	{"InvalidateFolderDescriptorCache", (PyCFunction)Folder_InvalidateFolderDescriptorCache, 1,
	 PyDoc_STR("(short vRefNum, long dirID) -> None")},
	{"IdentifyFolder", (PyCFunction)Folder_IdentifyFolder, 1,
	 PyDoc_STR("(short vRefNum, long dirID) -> (FolderType foldType)")},
	{NULL, NULL, 0}
};




void init_Folder(void)
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("_Folder", Folder_methods);
	d = PyModule_GetDict(m);
	Folder_Error = PyMac_GetOSErrException();
	if (Folder_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Folder_Error) != 0)
		return;
}

/* ======================= End module _Folder ======================= */

