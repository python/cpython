
/* ========================= Module _Alias ========================== */

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

static PyObject *Alias_Error;

/* ----------------------- Object type Alias ------------------------ */

PyTypeObject Alias_Type;

#define AliasObj_Check(x) ((x)->ob_type == &Alias_Type)

typedef struct AliasObject {
	PyObject_HEAD
	AliasHandle ob_itself;
	void (*ob_freeit)(AliasHandle ptr);
} AliasObject;

PyObject *AliasObj_New(AliasHandle itself)
{
	AliasObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(AliasObject, &Alias_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = NULL;
	return (PyObject *)it;
}
int AliasObj_Convert(PyObject *v, AliasHandle *p_itself)
{
	if (!AliasObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Alias required");
		return 0;
	}
	*p_itself = ((AliasObject *)v)->ob_itself;
	return 1;
}

static void AliasObj_dealloc(AliasObject *self)
{
	if (self->ob_freeit && self->ob_itself)
	{
		self->ob_freeit(self->ob_itself);
	}
	self->ob_itself = NULL;
	PyObject_Del(self);
}

static PyObject *AliasObj_GetAliasInfo(AliasObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AliasInfoType index;
	Str63 theString;
	if (!PyArg_ParseTuple(_args, "h",
	                      &index))
		return NULL;
	_err = GetAliasInfo(_self->ob_itself,
	                    index,
	                    theString);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildStr255, theString);
	return _res;
}

static PyMethodDef AliasObj_methods[] = {
	{"GetAliasInfo", (PyCFunction)AliasObj_GetAliasInfo, 1,
	 PyDoc_STR("(AliasInfoType index) -> (Str63 theString)")},
	{NULL, NULL, 0}
};

#define AliasObj_getsetlist NULL


#define AliasObj_compare NULL

#define AliasObj_repr NULL

#define AliasObj_hash NULL
#define AliasObj_tp_init 0

#define AliasObj_tp_alloc PyType_GenericAlloc

static PyObject *AliasObj_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *self;
	AliasHandle itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&", kw, AliasObj_Convert, &itself)) return NULL;
	if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((AliasObject *)self)->ob_itself = itself;
	return self;
}

#define AliasObj_tp_free PyObject_Del


PyTypeObject Alias_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Alias.Alias", /*tp_name*/
	sizeof(AliasObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) AliasObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) AliasObj_compare, /*tp_compare*/
	(reprfunc) AliasObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) AliasObj_hash, /*tp_hash*/
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
	AliasObj_methods, /* tp_methods */
	0, /*tp_members*/
	AliasObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	AliasObj_tp_init, /* tp_init */
	AliasObj_tp_alloc, /* tp_alloc */
	AliasObj_tp_new, /* tp_new */
	AliasObj_tp_free, /* tp_free */
};

/* --------------------- End object type Alias ---------------------- */


static PyObject *Alias_NewAlias(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec fromFile;
	FSSpec target;
	AliasHandle alias;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFSSpec, &fromFile,
	                      PyMac_GetFSSpec, &target))
		return NULL;
	_err = NewAlias(&fromFile,
	                &target,
	                &alias);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AliasObj_New, alias);
	return _res;
}

static PyObject *Alias_NewAliasMinimal(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec target;
	AliasHandle alias;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &target))
		return NULL;
	_err = NewAliasMinimal(&target,
	                       &alias);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AliasObj_New, alias);
	return _res;
}

static PyObject *Alias_NewAliasMinimalFromFullPath(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	char *fullPath__in__;
	int fullPath__len__;
	int fullPath__in_len__;
	Str32 zoneName;
	Str31 serverName;
	AliasHandle alias;
	if (!PyArg_ParseTuple(_args, "s#O&O&",
	                      &fullPath__in__, &fullPath__in_len__,
	                      PyMac_GetStr255, zoneName,
	                      PyMac_GetStr255, serverName))
		return NULL;
	fullPath__len__ = fullPath__in_len__;
	_err = NewAliasMinimalFromFullPath(fullPath__len__, fullPath__in__,
	                                   zoneName,
	                                   serverName,
	                                   &alias);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AliasObj_New, alias);
	return _res;
}

static PyObject *Alias_ResolveAlias(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec fromFile;
	AliasHandle alias;
	FSSpec target;
	Boolean wasChanged;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFSSpec, &fromFile,
	                      AliasObj_Convert, &alias))
		return NULL;
	_err = ResolveAlias(&fromFile,
	                    alias,
	                    &target,
	                    &wasChanged);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&b",
	                     PyMac_BuildFSSpec, &target,
	                     wasChanged);
	return _res;
}

static PyObject *Alias_IsAliasFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec fileFSSpec;
	Boolean aliasFileFlag;
	Boolean folderFlag;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &fileFSSpec))
		return NULL;
	_err = IsAliasFile(&fileFSSpec,
	                   &aliasFileFlag,
	                   &folderFlag);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("bb",
	                     aliasFileFlag,
	                     folderFlag);
	return _res;
}

static PyObject *Alias_ResolveAliasWithMountFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec fromFile;
	AliasHandle alias;
	FSSpec target;
	Boolean wasChanged;
	unsigned long mountFlags;
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      PyMac_GetFSSpec, &fromFile,
	                      AliasObj_Convert, &alias,
	                      &mountFlags))
		return NULL;
	_err = ResolveAliasWithMountFlags(&fromFile,
	                                  alias,
	                                  &target,
	                                  &wasChanged,
	                                  mountFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&b",
	                     PyMac_BuildFSSpec, &target,
	                     wasChanged);
	return _res;
}

static PyObject *Alias_ResolveAliasFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec theSpec;
	Boolean resolveAliasChains;
	Boolean targetIsFolder;
	Boolean wasAliased;
	if (!PyArg_ParseTuple(_args, "b",
	                      &resolveAliasChains))
		return NULL;
	_err = ResolveAliasFile(&theSpec,
	                        resolveAliasChains,
	                        &targetIsFolder,
	                        &wasAliased);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&bb",
	                     PyMac_BuildFSSpec, &theSpec,
	                     targetIsFolder,
	                     wasAliased);
	return _res;
}

static PyObject *Alias_ResolveAliasFileWithMountFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec theSpec;
	Boolean resolveAliasChains;
	Boolean targetIsFolder;
	Boolean wasAliased;
	unsigned long mountFlags;
	if (!PyArg_ParseTuple(_args, "bl",
	                      &resolveAliasChains,
	                      &mountFlags))
		return NULL;
	_err = ResolveAliasFileWithMountFlags(&theSpec,
	                                      resolveAliasChains,
	                                      &targetIsFolder,
	                                      &wasAliased,
	                                      mountFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&bb",
	                     PyMac_BuildFSSpec, &theSpec,
	                     targetIsFolder,
	                     wasAliased);
	return _res;
}

static PyObject *Alias_FollowFinderAlias(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec fromFile;
	AliasHandle alias;
	Boolean logon;
	FSSpec target;
	Boolean wasChanged;
	if (!PyArg_ParseTuple(_args, "O&O&b",
	                      PyMac_GetFSSpec, &fromFile,
	                      AliasObj_Convert, &alias,
	                      &logon))
		return NULL;
	_err = FollowFinderAlias(&fromFile,
	                         alias,
	                         logon,
	                         &target,
	                         &wasChanged);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&b",
	                     PyMac_BuildFSSpec, &target,
	                     wasChanged);
	return _res;
}

static PyObject *Alias_UpdateAlias(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec fromFile;
	FSSpec target;
	AliasHandle alias;
	Boolean wasChanged;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      PyMac_GetFSSpec, &fromFile,
	                      PyMac_GetFSSpec, &target,
	                      AliasObj_Convert, &alias))
		return NULL;
	_err = UpdateAlias(&fromFile,
	                   &target,
	                   alias,
	                   &wasChanged);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("b",
	                     wasChanged);
	return _res;
}

static PyObject *Alias_ResolveAliasFileWithMountFlagsNoUI(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSSpec theSpec;
	Boolean resolveAliasChains;
	Boolean targetIsFolder;
	Boolean wasAliased;
	unsigned long mountFlags;
	if (!PyArg_ParseTuple(_args, "bl",
	                      &resolveAliasChains,
	                      &mountFlags))
		return NULL;
	_err = ResolveAliasFileWithMountFlagsNoUI(&theSpec,
	                                          resolveAliasChains,
	                                          &targetIsFolder,
	                                          &wasAliased,
	                                          mountFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&bb",
	                     PyMac_BuildFSSpec, &theSpec,
	                     targetIsFolder,
	                     wasAliased);
	return _res;
}

static PyObject *Alias_FSNewAlias(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef fromFile;
	FSRef target;
	AliasHandle inAlias;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFSRef, &fromFile,
	                      PyMac_GetFSRef, &target))
		return NULL;
	_err = FSNewAlias(&fromFile,
	                  &target,
	                  &inAlias);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AliasObj_New, inAlias);
	return _res;
}

static PyObject *Alias_FSNewAliasMinimal(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef target;
	AliasHandle inAlias;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSRef, &target))
		return NULL;
	_err = FSNewAliasMinimal(&target,
	                         &inAlias);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AliasObj_New, inAlias);
	return _res;
}

static PyObject *Alias_FSIsAliasFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef fileRef;
	Boolean aliasFileFlag;
	Boolean folderFlag;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSRef, &fileRef))
		return NULL;
	_err = FSIsAliasFile(&fileRef,
	                     &aliasFileFlag,
	                     &folderFlag);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("bb",
	                     aliasFileFlag,
	                     folderFlag);
	return _res;
}

static PyObject *Alias_FSResolveAliasWithMountFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef fromFile;
	AliasHandle inAlias;
	FSRef target;
	Boolean wasChanged;
	unsigned long mountFlags;
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      PyMac_GetFSRef, &fromFile,
	                      AliasObj_Convert, &inAlias,
	                      &mountFlags))
		return NULL;
	_err = FSResolveAliasWithMountFlags(&fromFile,
	                                    inAlias,
	                                    &target,
	                                    &wasChanged,
	                                    mountFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&b",
	                     PyMac_BuildFSRef, &target,
	                     wasChanged);
	return _res;
}

static PyObject *Alias_FSResolveAlias(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef fromFile;
	AliasHandle alias;
	FSRef target;
	Boolean wasChanged;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetFSRef, &fromFile,
	                      AliasObj_Convert, &alias))
		return NULL;
	_err = FSResolveAlias(&fromFile,
	                      alias,
	                      &target,
	                      &wasChanged);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&b",
	                     PyMac_BuildFSRef, &target,
	                     wasChanged);
	return _res;
}

static PyObject *Alias_FSResolveAliasFileWithMountFlags(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef theRef;
	Boolean resolveAliasChains;
	Boolean targetIsFolder;
	Boolean wasAliased;
	unsigned long mountFlags;
	if (!PyArg_ParseTuple(_args, "bl",
	                      &resolveAliasChains,
	                      &mountFlags))
		return NULL;
	_err = FSResolveAliasFileWithMountFlags(&theRef,
	                                        resolveAliasChains,
	                                        &targetIsFolder,
	                                        &wasAliased,
	                                        mountFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&bb",
	                     PyMac_BuildFSRef, &theRef,
	                     targetIsFolder,
	                     wasAliased);
	return _res;
}

static PyObject *Alias_FSResolveAliasFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef theRef;
	Boolean resolveAliasChains;
	Boolean targetIsFolder;
	Boolean wasAliased;
	if (!PyArg_ParseTuple(_args, "b",
	                      &resolveAliasChains))
		return NULL;
	_err = FSResolveAliasFile(&theRef,
	                          resolveAliasChains,
	                          &targetIsFolder,
	                          &wasAliased);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&bb",
	                     PyMac_BuildFSRef, &theRef,
	                     targetIsFolder,
	                     wasAliased);
	return _res;
}

static PyObject *Alias_FSFollowFinderAlias(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef fromFile;
	AliasHandle alias;
	Boolean logon;
	FSRef target;
	Boolean wasChanged;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      AliasObj_Convert, &alias,
	                      &logon))
		return NULL;
	_err = FSFollowFinderAlias(&fromFile,
	                           alias,
	                           logon,
	                           &target,
	                           &wasChanged);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&b",
	                     PyMac_BuildFSRef, &fromFile,
	                     PyMac_BuildFSRef, &target,
	                     wasChanged);
	return _res;
}

static PyObject *Alias_FSUpdateAlias(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	FSRef fromFile;
	FSRef target;
	AliasHandle alias;
	Boolean wasChanged;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      PyMac_GetFSRef, &fromFile,
	                      PyMac_GetFSRef, &target,
	                      AliasObj_Convert, &alias))
		return NULL;
	_err = FSUpdateAlias(&fromFile,
	                     &target,
	                     alias,
	                     &wasChanged);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("b",
	                     wasChanged);
	return _res;
}

static PyMethodDef Alias_methods[] = {
	{"NewAlias", (PyCFunction)Alias_NewAlias, 1,
	 PyDoc_STR("(FSSpec fromFile, FSSpec target) -> (AliasHandle alias)")},
	{"NewAliasMinimal", (PyCFunction)Alias_NewAliasMinimal, 1,
	 PyDoc_STR("(FSSpec target) -> (AliasHandle alias)")},
	{"NewAliasMinimalFromFullPath", (PyCFunction)Alias_NewAliasMinimalFromFullPath, 1,
	 PyDoc_STR("(Buffer fullPath, Str32 zoneName, Str31 serverName) -> (AliasHandle alias)")},
	{"ResolveAlias", (PyCFunction)Alias_ResolveAlias, 1,
	 PyDoc_STR("(FSSpec fromFile, AliasHandle alias) -> (FSSpec target, Boolean wasChanged)")},
	{"IsAliasFile", (PyCFunction)Alias_IsAliasFile, 1,
	 PyDoc_STR("(FSSpec fileFSSpec) -> (Boolean aliasFileFlag, Boolean folderFlag)")},
	{"ResolveAliasWithMountFlags", (PyCFunction)Alias_ResolveAliasWithMountFlags, 1,
	 PyDoc_STR("(FSSpec fromFile, AliasHandle alias, unsigned long mountFlags) -> (FSSpec target, Boolean wasChanged)")},
	{"ResolveAliasFile", (PyCFunction)Alias_ResolveAliasFile, 1,
	 PyDoc_STR("(Boolean resolveAliasChains) -> (FSSpec theSpec, Boolean targetIsFolder, Boolean wasAliased)")},
	{"ResolveAliasFileWithMountFlags", (PyCFunction)Alias_ResolveAliasFileWithMountFlags, 1,
	 PyDoc_STR("(Boolean resolveAliasChains, unsigned long mountFlags) -> (FSSpec theSpec, Boolean targetIsFolder, Boolean wasAliased)")},
	{"FollowFinderAlias", (PyCFunction)Alias_FollowFinderAlias, 1,
	 PyDoc_STR("(FSSpec fromFile, AliasHandle alias, Boolean logon) -> (FSSpec target, Boolean wasChanged)")},
	{"UpdateAlias", (PyCFunction)Alias_UpdateAlias, 1,
	 PyDoc_STR("(FSSpec fromFile, FSSpec target, AliasHandle alias) -> (Boolean wasChanged)")},
	{"ResolveAliasFileWithMountFlagsNoUI", (PyCFunction)Alias_ResolveAliasFileWithMountFlagsNoUI, 1,
	 PyDoc_STR("(Boolean resolveAliasChains, unsigned long mountFlags) -> (FSSpec theSpec, Boolean targetIsFolder, Boolean wasAliased)")},
	{"FSNewAlias", (PyCFunction)Alias_FSNewAlias, 1,
	 PyDoc_STR("(FSRef fromFile, FSRef target) -> (AliasHandle inAlias)")},
	{"FSNewAliasMinimal", (PyCFunction)Alias_FSNewAliasMinimal, 1,
	 PyDoc_STR("(FSRef target) -> (AliasHandle inAlias)")},
	{"FSIsAliasFile", (PyCFunction)Alias_FSIsAliasFile, 1,
	 PyDoc_STR("(FSRef fileRef) -> (Boolean aliasFileFlag, Boolean folderFlag)")},
	{"FSResolveAliasWithMountFlags", (PyCFunction)Alias_FSResolveAliasWithMountFlags, 1,
	 PyDoc_STR("(FSRef fromFile, AliasHandle inAlias, unsigned long mountFlags) -> (FSRef target, Boolean wasChanged)")},
	{"FSResolveAlias", (PyCFunction)Alias_FSResolveAlias, 1,
	 PyDoc_STR("(FSRef fromFile, AliasHandle alias) -> (FSRef target, Boolean wasChanged)")},
	{"FSResolveAliasFileWithMountFlags", (PyCFunction)Alias_FSResolveAliasFileWithMountFlags, 1,
	 PyDoc_STR("(Boolean resolveAliasChains, unsigned long mountFlags) -> (FSRef theRef, Boolean targetIsFolder, Boolean wasAliased)")},
	{"FSResolveAliasFile", (PyCFunction)Alias_FSResolveAliasFile, 1,
	 PyDoc_STR("(Boolean resolveAliasChains) -> (FSRef theRef, Boolean targetIsFolder, Boolean wasAliased)")},
	{"FSFollowFinderAlias", (PyCFunction)Alias_FSFollowFinderAlias, 1,
	 PyDoc_STR("(AliasHandle alias, Boolean logon) -> (FSRef fromFile, FSRef target, Boolean wasChanged)")},
	{"FSUpdateAlias", (PyCFunction)Alias_FSUpdateAlias, 1,
	 PyDoc_STR("(FSRef fromFile, FSRef target, AliasHandle alias) -> (Boolean wasChanged)")},
	{NULL, NULL, 0}
};




void init_Alias(void)
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("_Alias", Alias_methods);
	d = PyModule_GetDict(m);
	Alias_Error = PyMac_GetOSErrException();
	if (Alias_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Alias_Error) != 0)
		return;
	Alias_Type.ob_type = &PyType_Type;
	Py_INCREF(&Alias_Type);
	PyModule_AddObject(m, "Alias", (PyObject *)&Alias_Type);
	/* Backward-compatible name */
	Py_INCREF(&Alias_Type);
	PyModule_AddObject(m, "AliasType", (PyObject *)&Alias_Type);
}

/* ======================= End module _Alias ======================== */

