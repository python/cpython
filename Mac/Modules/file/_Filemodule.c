
/* ========================== Module _File ========================== */

#include "Python.h"



#include "pymactoolbox.h"

#ifndef HAVE_OSX105_SDK
typedef SInt16  FSIORefNum;
#endif

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
            "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>

#ifdef USE_TOOLBOX_OBJECT_GLUE

#ifndef __LP64__
extern int _PyMac_GetFSSpec(PyObject *v, FSSpec *spec);
extern PyObject *_PyMac_BuildFSSpec(FSSpec *spec);
#define PyMac_BuildFSSpec _PyMac_BuildFSSpec
#endif /* __LP64__*/

extern int _PyMac_GetFSRef(PyObject *v, FSRef *fsr);
extern PyObject *_PyMac_BuildFSRef(FSRef *spec);
#define PyMac_BuildFSRef _PyMac_BuildFSRef
#define PyMac_GetFSSpec _PyMac_GetFSSpec
#define PyMac_GetFSRef _PyMac_GetFSRef

#else   /* !USE_TOOLBOX_OBJECT_GLUE */

#ifndef __LP64__
extern int PyMac_GetFSSpec(PyObject *v, FSSpec *spec);
extern PyObject *PyMac_BuildFSSpec(FSSpec *spec);
#endif /* !__LP64__*/

extern int PyMac_GetFSRef(PyObject *v, FSRef *fsr);
extern PyObject *PyMac_BuildFSRef(FSRef *spec);

#endif  /* !USE_TOOLBOX_OBJECT_GLUE */

/* Forward declarations */
static PyObject *FSRef_New(FSRef *itself);
#ifndef __LP64__
static PyObject *FInfo_New(FInfo *itself);

static PyObject *FSSpec_New(FSSpec *itself);
#define FSSpec_Convert PyMac_GetFSSpec
#endif /* !__LP64__ */

static PyObject *Alias_New(AliasHandle itself);
#ifndef __LP64__
static int FInfo_Convert(PyObject *v, FInfo *p_itself);
#endif /* !__LP64__ */
#define FSRef_Convert PyMac_GetFSRef
static int Alias_Convert(PyObject *v, AliasHandle *p_itself);

/*
** UTCDateTime records
*/
static int
UTCDateTime_Convert(PyObject *v, UTCDateTime *ptr)
{
    return PyArg_Parse(v, "(HlH)", &ptr->highSeconds, &ptr->lowSeconds, &ptr->fraction);
}

static PyObject *
UTCDateTime_New(UTCDateTime *ptr)
{
    return Py_BuildValue("(HlH)", ptr->highSeconds, ptr->lowSeconds, ptr->fraction);
}

/*
** Optional fsspec and fsref pointers. None will pass NULL
*/
#ifndef __LP64__
static int
myPyMac_GetOptFSSpecPtr(PyObject *v, FSSpec **spec)
{
    if (v == Py_None) {
        *spec = NULL;
        return 1;
    }
    return PyMac_GetFSSpec(v, *spec);
}
#endif /* !__LP64__ */

static int
myPyMac_GetOptFSRefPtr(PyObject *v, FSRef **ref)
{
    if (v == Py_None) {
        *ref = NULL;
        return 1;
    }
    return PyMac_GetFSRef(v, *ref);
}

/*
** Parse/generate objsect
*/
static PyObject *
PyMac_BuildHFSUniStr255(HFSUniStr255 *itself)
{

    return Py_BuildValue("u#", itself->unicode, itself->length);
}

#ifndef __LP64__
static OSErr
_PyMac_GetFullPathname(FSSpec *fss, char *path, int len)
{
    FSRef fsr;
    OSErr err;

    *path = '\0';
    err = FSpMakeFSRef(fss, &fsr);
    if (err == fnfErr) {
        /* FSSpecs can point to non-existing files, fsrefs can't. */
        FSSpec fss2;
        int tocopy;

        err = FSMakeFSSpec(fss->vRefNum, fss->parID,
                           (unsigned char*)"", &fss2);
        if (err)
            return err;
        err = FSpMakeFSRef(&fss2, &fsr);
        if (err)
            return err;
        err = (OSErr)FSRefMakePath(&fsr, (unsigned char*)path, len-1);
        if (err)
            return err;
        /* This part is not 100% safe: we append the filename part, but
        ** I'm not sure that we don't run afoul of the various 8bit
        ** encodings here. Will have to look this up at some point...
        */
        strcat(path, "/");
        tocopy = fss->name[0];
        if ((strlen(path) + tocopy) >= len)
            tocopy = len - strlen(path) - 1;
        if (tocopy > 0)
            strncat(path, (char*)fss->name+1, tocopy);
    }
    else {
        if (err)
            return err;
        err = (OSErr)FSRefMakePath(&fsr, (unsigned char*)path, len);
        if (err)
            return err;
    }
    return 0;
}
#endif /* !__LP64__ */


static PyObject *File_Error;

/* ------------------- Object type FSCatalogInfo -------------------- */

static PyTypeObject FSCatalogInfo_Type;

#define FSCatalogInfo_Check(x) ((x)->ob_type == &FSCatalogInfo_Type || PyObject_TypeCheck((x), &FSCatalogInfo_Type))

typedef struct FSCatalogInfoObject {
    PyObject_HEAD
    FSCatalogInfo ob_itself;
} FSCatalogInfoObject;

static PyObject *FSCatalogInfo_New(FSCatalogInfo *itself)
{
    FSCatalogInfoObject *it;
    if (itself == NULL) { Py_INCREF(Py_None); return Py_None; }
    it = PyObject_NEW(FSCatalogInfoObject, &FSCatalogInfo_Type);
    if (it == NULL) return NULL;
    it->ob_itself = *itself;
    return (PyObject *)it;
}

static int FSCatalogInfo_Convert(PyObject *v, FSCatalogInfo *p_itself)
{
    if (!FSCatalogInfo_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "FSCatalogInfo required");
        return 0;
    }
    *p_itself = ((FSCatalogInfoObject *)v)->ob_itself;
    return 1;
}

static void FSCatalogInfo_dealloc(FSCatalogInfoObject *self)
{
    /* Cleanup of self->ob_itself goes here */
    self->ob_type->tp_free((PyObject *)self);
}

static PyMethodDef FSCatalogInfo_methods[] = {
    {NULL, NULL, 0}
};

static PyObject *FSCatalogInfo_get_nodeFlags(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("H", self->ob_itself.nodeFlags);
}

static int FSCatalogInfo_set_nodeFlags(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "H", &self->ob_itself.nodeFlags)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_volume(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("h", self->ob_itself.volume);
}

static int FSCatalogInfo_set_volume(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "h", &self->ob_itself.volume)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_parentDirID(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("l", self->ob_itself.parentDirID);
}

static int FSCatalogInfo_set_parentDirID(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "l", &self->ob_itself.parentDirID)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_nodeID(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("l", self->ob_itself.nodeID);
}

static int FSCatalogInfo_set_nodeID(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "l", &self->ob_itself.nodeID)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_createDate(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("O&", UTCDateTime_New, &self->ob_itself.createDate);
}

static int FSCatalogInfo_set_createDate(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "O&", UTCDateTime_Convert, &self->ob_itself.createDate)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_contentModDate(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("O&", UTCDateTime_New, &self->ob_itself.contentModDate);
}

static int FSCatalogInfo_set_contentModDate(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "O&", UTCDateTime_Convert, &self->ob_itself.contentModDate)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_attributeModDate(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("O&", UTCDateTime_New, &self->ob_itself.attributeModDate);
}

static int FSCatalogInfo_set_attributeModDate(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "O&", UTCDateTime_Convert, &self->ob_itself.attributeModDate)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_accessDate(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("O&", UTCDateTime_New, &self->ob_itself.accessDate);
}

static int FSCatalogInfo_set_accessDate(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "O&", UTCDateTime_Convert, &self->ob_itself.accessDate)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_backupDate(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("O&", UTCDateTime_New, &self->ob_itself.backupDate);
}

static int FSCatalogInfo_set_backupDate(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "O&", UTCDateTime_Convert, &self->ob_itself.backupDate)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_permissions(FSCatalogInfoObject *self, void *closure)
{
    FSPermissionInfo* info = (FSPermissionInfo*)&(self->ob_itself.permissions);
    return Py_BuildValue("(llll)", info->userID, info->groupID, info->userAccess, info->mode);
}

static int FSCatalogInfo_set_permissions(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    long userID;
    long groupID;
    long userAccess;
    long mode;
    int r;

    FSPermissionInfo* info = (FSPermissionInfo*)&(self->ob_itself.permissions);

    r = PyArg_Parse(v, "(llll)", &userID, &groupID, &userAccess, &mode);
    if (!r) {
        return -1;
    }
    info->userID = userID;
    info->groupID = groupID;
    info->userAccess = userAccess;
    info->mode = mode;
    return 0;
}

static PyObject *FSCatalogInfo_get_valence(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("l", self->ob_itself.valence);
}

static int FSCatalogInfo_set_valence(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "l", &self->ob_itself.valence)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_dataLogicalSize(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("l", self->ob_itself.dataLogicalSize);
}

static int FSCatalogInfo_set_dataLogicalSize(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "l", &self->ob_itself.dataLogicalSize)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_dataPhysicalSize(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("l", self->ob_itself.dataPhysicalSize);
}

static int FSCatalogInfo_set_dataPhysicalSize(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "l", &self->ob_itself.dataPhysicalSize)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_rsrcLogicalSize(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("l", self->ob_itself.rsrcLogicalSize);
}

static int FSCatalogInfo_set_rsrcLogicalSize(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "l", &self->ob_itself.rsrcLogicalSize)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_rsrcPhysicalSize(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("l", self->ob_itself.rsrcPhysicalSize);
}

static int FSCatalogInfo_set_rsrcPhysicalSize(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "l", &self->ob_itself.rsrcPhysicalSize)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_sharingFlags(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("l", self->ob_itself.sharingFlags);
}

static int FSCatalogInfo_set_sharingFlags(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "l", &self->ob_itself.sharingFlags)-1;
    return 0;
}

static PyObject *FSCatalogInfo_get_userPrivileges(FSCatalogInfoObject *self, void *closure)
{
    return Py_BuildValue("b", self->ob_itself.userPrivileges);
}

static int FSCatalogInfo_set_userPrivileges(FSCatalogInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "b", &self->ob_itself.userPrivileges)-1;
    return 0;
}

static PyGetSetDef FSCatalogInfo_getsetlist[] = {
    {"nodeFlags", (getter)FSCatalogInfo_get_nodeFlags, (setter)FSCatalogInfo_set_nodeFlags, NULL},
    {"volume", (getter)FSCatalogInfo_get_volume, (setter)FSCatalogInfo_set_volume, NULL},
    {"parentDirID", (getter)FSCatalogInfo_get_parentDirID, (setter)FSCatalogInfo_set_parentDirID, NULL},
    {"nodeID", (getter)FSCatalogInfo_get_nodeID, (setter)FSCatalogInfo_set_nodeID, NULL},
    {"createDate", (getter)FSCatalogInfo_get_createDate, (setter)FSCatalogInfo_set_createDate, NULL},
    {"contentModDate", (getter)FSCatalogInfo_get_contentModDate, (setter)FSCatalogInfo_set_contentModDate, NULL},
    {"attributeModDate", (getter)FSCatalogInfo_get_attributeModDate, (setter)FSCatalogInfo_set_attributeModDate, NULL},
    {"accessDate", (getter)FSCatalogInfo_get_accessDate, (setter)FSCatalogInfo_set_accessDate, NULL},
    {"backupDate", (getter)FSCatalogInfo_get_backupDate, (setter)FSCatalogInfo_set_backupDate, NULL},
    {"permissions", (getter)FSCatalogInfo_get_permissions, (setter)FSCatalogInfo_set_permissions, NULL},
    {"valence", (getter)FSCatalogInfo_get_valence, (setter)FSCatalogInfo_set_valence, NULL},
    {"dataLogicalSize", (getter)FSCatalogInfo_get_dataLogicalSize, (setter)FSCatalogInfo_set_dataLogicalSize, NULL},
    {"dataPhysicalSize", (getter)FSCatalogInfo_get_dataPhysicalSize, (setter)FSCatalogInfo_set_dataPhysicalSize, NULL},
    {"rsrcLogicalSize", (getter)FSCatalogInfo_get_rsrcLogicalSize, (setter)FSCatalogInfo_set_rsrcLogicalSize, NULL},
    {"rsrcPhysicalSize", (getter)FSCatalogInfo_get_rsrcPhysicalSize, (setter)FSCatalogInfo_set_rsrcPhysicalSize, NULL},
    {"sharingFlags", (getter)FSCatalogInfo_get_sharingFlags, (setter)FSCatalogInfo_set_sharingFlags, NULL},
    {"userPrivileges", (getter)FSCatalogInfo_get_userPrivileges, (setter)FSCatalogInfo_set_userPrivileges, NULL},
    {NULL, NULL, NULL, NULL},
};


#define FSCatalogInfo_compare NULL

#define FSCatalogInfo_repr NULL

#define FSCatalogInfo_hash NULL
static int FSCatalogInfo_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    static char *kw[] = {
                "nodeFlags",
                "volume",
                "parentDirID",
                "nodeID",
                "createDate",
                "contentModDate",
                "atributeModDate",
                "accessDate",
                "backupDate",
                "valence",
                "dataLogicalSize",
                "dataPhysicalSize",
                "rsrcLogicalSize",
                "rsrcPhysicalSize",
                "sharingFlags",
                "userPrivileges"
                , 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "|HhllO&O&O&O&O&llllllb", kw, &((FSCatalogInfoObject *)_self)->ob_itself.nodeFlags,
                &((FSCatalogInfoObject *)_self)->ob_itself.volume,
                &((FSCatalogInfoObject *)_self)->ob_itself.parentDirID,
                &((FSCatalogInfoObject *)_self)->ob_itself.nodeID,
                UTCDateTime_Convert, &((FSCatalogInfoObject *)_self)->ob_itself.createDate,
                UTCDateTime_Convert, &((FSCatalogInfoObject *)_self)->ob_itself.contentModDate,
                UTCDateTime_Convert, &((FSCatalogInfoObject *)_self)->ob_itself.attributeModDate,
                UTCDateTime_Convert, &((FSCatalogInfoObject *)_self)->ob_itself.accessDate,
                UTCDateTime_Convert, &((FSCatalogInfoObject *)_self)->ob_itself.backupDate,
                &((FSCatalogInfoObject *)_self)->ob_itself.valence,
                &((FSCatalogInfoObject *)_self)->ob_itself.dataLogicalSize,
                &((FSCatalogInfoObject *)_self)->ob_itself.dataPhysicalSize,
                &((FSCatalogInfoObject *)_self)->ob_itself.rsrcLogicalSize,
                &((FSCatalogInfoObject *)_self)->ob_itself.rsrcPhysicalSize,
                &((FSCatalogInfoObject *)_self)->ob_itself.sharingFlags,
                &((FSCatalogInfoObject *)_self)->ob_itself.userPrivileges))
    {
        return -1;
    }
    return 0;
}

#define FSCatalogInfo_tp_alloc PyType_GenericAlloc

static PyObject *FSCatalogInfo_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;

    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    memset(&((FSCatalogInfoObject *)self)->ob_itself, 0, sizeof(FSCatalogInfo));
    return self;
}

#define FSCatalogInfo_tp_free PyObject_Del


static PyTypeObject FSCatalogInfo_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "Carbon.File.FSCatalogInfo", /*tp_name*/
    sizeof(FSCatalogInfoObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) FSCatalogInfo_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) FSCatalogInfo_compare, /*tp_compare*/
    (reprfunc) FSCatalogInfo_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) FSCatalogInfo_hash, /*tp_hash*/
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
    FSCatalogInfo_methods, /* tp_methods */
    0, /*tp_members*/
    FSCatalogInfo_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    FSCatalogInfo_tp_init, /* tp_init */
    FSCatalogInfo_tp_alloc, /* tp_alloc */
    FSCatalogInfo_tp_new, /* tp_new */
    FSCatalogInfo_tp_free, /* tp_free */
};

/* ----------------- End object type FSCatalogInfo ------------------ */


/* ----------------------- Object type FInfo ------------------------ */

#ifndef __LP64__

static PyTypeObject FInfo_Type;

#define FInfo_Check(x) ((x)->ob_type == &FInfo_Type || PyObject_TypeCheck((x), &FInfo_Type))

typedef struct FInfoObject {
    PyObject_HEAD
    FInfo ob_itself;
} FInfoObject;

static PyObject *FInfo_New(FInfo *itself)
{
    FInfoObject *it;
    if (itself == NULL) return PyMac_Error(resNotFound);
    it = PyObject_NEW(FInfoObject, &FInfo_Type);
    if (it == NULL) return NULL;
    it->ob_itself = *itself;
    return (PyObject *)it;
}

static int FInfo_Convert(PyObject *v, FInfo *p_itself)
{
    if (!FInfo_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "FInfo required");
        return 0;
    }
    *p_itself = ((FInfoObject *)v)->ob_itself;
    return 1;
}

static void FInfo_dealloc(FInfoObject *self)
{
    /* Cleanup of self->ob_itself goes here */
    self->ob_type->tp_free((PyObject *)self);
}

static PyMethodDef FInfo_methods[] = {
    {NULL, NULL, 0}
};

static PyObject *FInfo_get_Type(FInfoObject *self, void *closure)
{
    return Py_BuildValue("O&", PyMac_BuildOSType, self->ob_itself.fdType);
}

static int FInfo_set_Type(FInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "O&", PyMac_GetOSType, &self->ob_itself.fdType)-1;
    return 0;
}

static PyObject *FInfo_get_Creator(FInfoObject *self, void *closure)
{
    return Py_BuildValue("O&", PyMac_BuildOSType, self->ob_itself.fdCreator);
}

static int FInfo_set_Creator(FInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "O&", PyMac_GetOSType, &self->ob_itself.fdCreator)-1;
    return 0;
}

static PyObject *FInfo_get_Flags(FInfoObject *self, void *closure)
{
    return Py_BuildValue("H", self->ob_itself.fdFlags);
}

static int FInfo_set_Flags(FInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "H", &self->ob_itself.fdFlags)-1;
    return 0;
}

static PyObject *FInfo_get_Location(FInfoObject *self, void *closure)
{
    return Py_BuildValue("O&", PyMac_BuildPoint, self->ob_itself.fdLocation);
}

static int FInfo_set_Location(FInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "O&", PyMac_GetPoint, &self->ob_itself.fdLocation)-1;
    return 0;
}

static PyObject *FInfo_get_Fldr(FInfoObject *self, void *closure)
{
    return Py_BuildValue("h", self->ob_itself.fdFldr);
}

static int FInfo_set_Fldr(FInfoObject *self, PyObject *v, void *closure)
{
    return PyArg_Parse(v, "h", &self->ob_itself.fdFldr)-1;
    return 0;
}

static PyGetSetDef FInfo_getsetlist[] = {
    {"Type", (getter)FInfo_get_Type, (setter)FInfo_set_Type, "4-char file type"},
    {"Creator", (getter)FInfo_get_Creator, (setter)FInfo_set_Creator, "4-char file creator"},
    {"Flags", (getter)FInfo_get_Flags, (setter)FInfo_set_Flags, "Finder flag bits"},
    {"Location", (getter)FInfo_get_Location, (setter)FInfo_set_Location, "(x, y) location of the file's icon in its parent finder window"},
    {"Fldr", (getter)FInfo_get_Fldr, (setter)FInfo_set_Fldr, "Original folder, for 'put away'"},
    {NULL, NULL, NULL, NULL},
};


#define FInfo_compare NULL

#define FInfo_repr NULL

#define FInfo_hash NULL
static int FInfo_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    FInfo *itself = NULL;
    static char *kw[] = {"itself", 0};

    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "|O&", kw, FInfo_Convert, &itself))
    {
        if (itself) memcpy(&((FInfoObject *)_self)->ob_itself, itself, sizeof(FInfo));
        return 0;
    }
    return -1;
}

#define FInfo_tp_alloc PyType_GenericAlloc

static PyObject *FInfo_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;

    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    memset(&((FInfoObject *)self)->ob_itself, 0, sizeof(FInfo));
    return self;
}

#define FInfo_tp_free PyObject_Del


static PyTypeObject FInfo_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "Carbon.File.FInfo", /*tp_name*/
    sizeof(FInfoObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) FInfo_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) FInfo_compare, /*tp_compare*/
    (reprfunc) FInfo_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) FInfo_hash, /*tp_hash*/
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
    FInfo_methods, /* tp_methods */
    0, /*tp_members*/
    FInfo_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    FInfo_tp_init, /* tp_init */
    FInfo_tp_alloc, /* tp_alloc */
    FInfo_tp_new, /* tp_new */
    FInfo_tp_free, /* tp_free */
};

#endif /* !__LP64__ */
/* --------------------- End object type FInfo ---------------------- */


/* ----------------------- Object type Alias ------------------------ */

static PyTypeObject Alias_Type;

#define Alias_Check(x) ((x)->ob_type == &Alias_Type || PyObject_TypeCheck((x), &Alias_Type))

typedef struct AliasObject {
    PyObject_HEAD
    AliasHandle ob_itself;
    void (*ob_freeit)(AliasHandle ptr);
} AliasObject;

static PyObject *Alias_New(AliasHandle itself)
{
    AliasObject *it;
    if (itself == NULL) return PyMac_Error(resNotFound);
    it = PyObject_NEW(AliasObject, &Alias_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    it->ob_freeit = NULL;
    return (PyObject *)it;
}

static int Alias_Convert(PyObject *v, AliasHandle *p_itself)
{
    if (!Alias_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "Alias required");
        return 0;
    }
    *p_itself = ((AliasObject *)v)->ob_itself;
    return 1;
}

static void Alias_dealloc(AliasObject *self)
{
    if (self->ob_freeit && self->ob_itself)
    {
        self->ob_freeit(self->ob_itself);
    }
    self->ob_itself = NULL;
    self->ob_type->tp_free((PyObject *)self);
}

#ifndef __LP64__
static PyObject *Alias_ResolveAlias(AliasObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSSpec fromFile__buf__;
    FSSpec *fromFile = &fromFile__buf__;
    FSSpec target;
    Boolean wasChanged;
    if (!PyArg_ParseTuple(_args, "O&",
                          myPyMac_GetOptFSSpecPtr, &fromFile))
        return NULL;
    _err = ResolveAlias(fromFile,
                        _self->ob_itself,
                        &target,
                        &wasChanged);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&b",
                         FSSpec_New, &target,
                         wasChanged);
    return _res;
}

static PyObject *Alias_GetAliasInfo(AliasObject *_self, PyObject *_args)
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

static PyObject *Alias_ResolveAliasWithMountFlags(AliasObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSSpec fromFile__buf__;
    FSSpec *fromFile = &fromFile__buf__;
    FSSpec target;
    Boolean wasChanged;
    unsigned long mountFlags;
    if (!PyArg_ParseTuple(_args, "O&l",
                          myPyMac_GetOptFSSpecPtr, &fromFile,
                          &mountFlags))
        return NULL;
    _err = ResolveAliasWithMountFlags(fromFile,
                                      _self->ob_itself,
                                      &target,
                                      &wasChanged,
                                      mountFlags);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&b",
                         FSSpec_New, &target,
                         wasChanged);
    return _res;
}

static PyObject *Alias_FollowFinderAlias(AliasObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSSpec fromFile__buf__;
    FSSpec *fromFile = &fromFile__buf__;
    Boolean logon;
    FSSpec target;
    Boolean wasChanged;
    if (!PyArg_ParseTuple(_args, "O&b",
                          myPyMac_GetOptFSSpecPtr, &fromFile,
                          &logon))
        return NULL;
    _err = FollowFinderAlias(fromFile,
                             _self->ob_itself,
                             logon,
                             &target,
                             &wasChanged);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&b",
                         FSSpec_New, &target,
                         wasChanged);
    return _res;
}
#endif /* !__LP64__ */

static PyObject *Alias_FSResolveAliasWithMountFlags(AliasObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSRef fromFile__buf__;
    FSRef *fromFile = &fromFile__buf__;
    FSRef target;
    Boolean wasChanged;
    unsigned long mountFlags;
    if (!PyArg_ParseTuple(_args, "O&l",
                          myPyMac_GetOptFSRefPtr, &fromFile,
                          &mountFlags))
        return NULL;
    _err = FSResolveAliasWithMountFlags(fromFile,
                                        _self->ob_itself,
                                        &target,
                                        &wasChanged,
                                        mountFlags);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&b",
                         FSRef_New, &target,
                         wasChanged);
    return _res;
}

static PyObject *Alias_FSResolveAlias(AliasObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSRef fromFile__buf__;
    FSRef *fromFile = &fromFile__buf__;
    FSRef target;
    Boolean wasChanged;
    if (!PyArg_ParseTuple(_args, "O&",
                          myPyMac_GetOptFSRefPtr, &fromFile))
        return NULL;
    _err = FSResolveAlias(fromFile,
                          _self->ob_itself,
                          &target,
                          &wasChanged);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&b",
                         FSRef_New, &target,
                         wasChanged);
    return _res;
}

static PyObject *Alias_FSFollowFinderAlias(AliasObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSRef fromFile;
    Boolean logon;
    FSRef target;
    Boolean wasChanged;
    if (!PyArg_ParseTuple(_args, "b",
                          &logon))
        return NULL;
    _err = FSFollowFinderAlias(&fromFile,
                               _self->ob_itself,
                               logon,
                               &target,
                               &wasChanged);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&O&b",
                         FSRef_New, &fromFile,
                         FSRef_New, &target,
                         wasChanged);
    return _res;
}

static PyMethodDef Alias_methods[] = {
#ifndef __LP64__
    {"ResolveAlias", (PyCFunction)Alias_ResolveAlias, 1,
     PyDoc_STR("(FSSpec fromFile) -> (FSSpec target, Boolean wasChanged)")},
    {"GetAliasInfo", (PyCFunction)Alias_GetAliasInfo, 1,
     PyDoc_STR("(AliasInfoType index) -> (Str63 theString)")},
    {"ResolveAliasWithMountFlags", (PyCFunction)Alias_ResolveAliasWithMountFlags, 1,
     PyDoc_STR("(FSSpec fromFile, unsigned long mountFlags) -> (FSSpec target, Boolean wasChanged)")},
    {"FollowFinderAlias", (PyCFunction)Alias_FollowFinderAlias, 1,
     PyDoc_STR("(FSSpec fromFile, Boolean logon) -> (FSSpec target, Boolean wasChanged)")},
#endif /* !__LP64__ */
    {"FSResolveAliasWithMountFlags", (PyCFunction)Alias_FSResolveAliasWithMountFlags, 1,
     PyDoc_STR("(FSRef fromFile, unsigned long mountFlags) -> (FSRef target, Boolean wasChanged)")},
    {"FSResolveAlias", (PyCFunction)Alias_FSResolveAlias, 1,
     PyDoc_STR("(FSRef fromFile) -> (FSRef target, Boolean wasChanged)")},
    {"FSFollowFinderAlias", (PyCFunction)Alias_FSFollowFinderAlias, 1,
     PyDoc_STR("(Boolean logon) -> (FSRef fromFile, FSRef target, Boolean wasChanged)")},
    {NULL, NULL, 0}
};

static PyObject *Alias_get_data(AliasObject *self, void *closure)
{
    int size;
                        PyObject *rv;

                        size = GetHandleSize((Handle)self->ob_itself);
                        HLock((Handle)self->ob_itself);
                        rv = PyString_FromStringAndSize(*(Handle)self->ob_itself, size);
                        HUnlock((Handle)self->ob_itself);
                        return rv;

}

#define Alias_set_data NULL

static PyGetSetDef Alias_getsetlist[] = {
    {"data", (getter)Alias_get_data, (setter)Alias_set_data, "Raw data of the alias object"},
    {NULL, NULL, NULL, NULL},
};


#define Alias_compare NULL

#define Alias_repr NULL

#define Alias_hash NULL
static int Alias_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    AliasHandle itself = NULL;
    char *rawdata = NULL;
    int rawdatalen = 0;
    Handle h;
    static char *kw[] = {"itself", "rawdata", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "|O&s#", kw, Alias_Convert, &itself, &rawdata, &rawdatalen))
    return -1;
    if (itself && rawdata)
    {
        PyErr_SetString(PyExc_TypeError, "Only one of itself or rawdata may be specified");
        return -1;
    }
    if (!itself && !rawdata)
    {
        PyErr_SetString(PyExc_TypeError, "One of itself or rawdata must be specified");
        return -1;
    }
    if (rawdata)
    {
        if ((h = NewHandle(rawdatalen)) == NULL)
        {
            PyErr_NoMemory();
            return -1;
        }
        HLock(h);
        memcpy((char *)*h, rawdata, rawdatalen);
        HUnlock(h);
        ((AliasObject *)_self)->ob_itself = (AliasHandle)h;
        return 0;
    }
    ((AliasObject *)_self)->ob_itself = itself;
    return 0;
}

#define Alias_tp_alloc PyType_GenericAlloc

static PyObject *Alias_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;

    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((AliasObject *)self)->ob_itself = NULL;
    return self;
}

#define Alias_tp_free PyObject_Del


static PyTypeObject Alias_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "Carbon.File.Alias", /*tp_name*/
    sizeof(AliasObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) Alias_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) Alias_compare, /*tp_compare*/
    (reprfunc) Alias_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) Alias_hash, /*tp_hash*/
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
    Alias_methods, /* tp_methods */
    0, /*tp_members*/
    Alias_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    Alias_tp_init, /* tp_init */
    Alias_tp_alloc, /* tp_alloc */
    Alias_tp_new, /* tp_new */
    Alias_tp_free, /* tp_free */
};

/* --------------------- End object type Alias ---------------------- */


/* ----------------------- Object type FSSpec ----------------------- */
#ifndef __LP64__

static PyTypeObject FSSpec_Type;

#define FSSpec_Check(x) ((x)->ob_type == &FSSpec_Type || PyObject_TypeCheck((x), &FSSpec_Type))

typedef struct FSSpecObject {
    PyObject_HEAD
    FSSpec ob_itself;
} FSSpecObject;

static PyObject *FSSpec_New(FSSpec *itself)
{
    FSSpecObject *it;
    if (itself == NULL) return PyMac_Error(resNotFound);
    it = PyObject_NEW(FSSpecObject, &FSSpec_Type);
    if (it == NULL) return NULL;
    it->ob_itself = *itself;
    return (PyObject *)it;
}

static void FSSpec_dealloc(FSSpecObject *self)
{
    /* Cleanup of self->ob_itself goes here */
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *FSSpec_FSpOpenDF(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    SInt8 permission;
    short refNum;
    if (!PyArg_ParseTuple(_args, "b",
                          &permission))
        return NULL;
    _err = FSpOpenDF(&_self->ob_itself,
                     permission,
                     &refNum);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("h",
                         refNum);
    return _res;
}

static PyObject *FSSpec_FSpOpenRF(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    SInt8 permission;
    short refNum;
    if (!PyArg_ParseTuple(_args, "b",
                          &permission))
        return NULL;
    _err = FSpOpenRF(&_self->ob_itself,
                     permission,
                     &refNum);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("h",
                         refNum);
    return _res;
}

static PyObject *FSSpec_FSpCreate(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    OSType creator;
    OSType fileType;
    ScriptCode scriptTag;
    if (!PyArg_ParseTuple(_args, "O&O&h",
                          PyMac_GetOSType, &creator,
                          PyMac_GetOSType, &fileType,
                          &scriptTag))
        return NULL;
    _err = FSpCreate(&_self->ob_itself,
                     creator,
                     fileType,
                     scriptTag);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSSpec_FSpDirCreate(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    ScriptCode scriptTag;
    long createdDirID;
    if (!PyArg_ParseTuple(_args, "h",
                          &scriptTag))
        return NULL;
    _err = FSpDirCreate(&_self->ob_itself,
                        scriptTag,
                        &createdDirID);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         createdDirID);
    return _res;
}

static PyObject *FSSpec_FSpDelete(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = FSpDelete(&_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSSpec_FSpGetFInfo(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FInfo fndrInfo;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = FSpGetFInfo(&_self->ob_itself,
                       &fndrInfo);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         FInfo_New, &fndrInfo);
    return _res;
}

static PyObject *FSSpec_FSpSetFInfo(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FInfo fndrInfo;
    if (!PyArg_ParseTuple(_args, "O&",
                          FInfo_Convert, &fndrInfo))
        return NULL;
    _err = FSpSetFInfo(&_self->ob_itself,
                       &fndrInfo);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSSpec_FSpSetFLock(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = FSpSetFLock(&_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSSpec_FSpRstFLock(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = FSpRstFLock(&_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSSpec_FSpRename(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    Str255 newName;
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetStr255, newName))
        return NULL;
    _err = FSpRename(&_self->ob_itself,
                     newName);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSSpec_FSpCatMove(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSSpec dest;
    if (!PyArg_ParseTuple(_args, "O&",
                          FSSpec_Convert, &dest))
        return NULL;
    _err = FSpCatMove(&_self->ob_itself,
                      &dest);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSSpec_FSpExchangeFiles(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSSpec dest;
    if (!PyArg_ParseTuple(_args, "O&",
                          FSSpec_Convert, &dest))
        return NULL;
    _err = FSpExchangeFiles(&_self->ob_itself,
                            &dest);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSSpec_FSpMakeFSRef(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSRef newRef;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = FSpMakeFSRef(&_self->ob_itself,
                        &newRef);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         FSRef_New, &newRef);
    return _res;
}

static PyObject *FSSpec_NewAliasMinimal(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    AliasHandle alias;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = NewAliasMinimal(&_self->ob_itself,
                           &alias);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         Alias_New, alias);
    return _res;
}

static PyObject *FSSpec_IsAliasFile(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    Boolean aliasFileFlag;
    Boolean folderFlag;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = IsAliasFile(&_self->ob_itself,
                       &aliasFileFlag,
                       &folderFlag);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("bb",
                         aliasFileFlag,
                         folderFlag);
    return _res;
}

static PyObject *FSSpec_as_pathname(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    char strbuf[1024];
    OSErr err;

    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    err = _PyMac_GetFullPathname(&_self->ob_itself, strbuf, sizeof(strbuf));
    if ( err ) {
        PyMac_Error(err);
        return NULL;
    }
    _res = PyString_FromString(strbuf);
    return _res;

}

static PyObject *FSSpec_as_tuple(FSSpecObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _res = Py_BuildValue("(iis#)", _self->ob_itself.vRefNum, _self->ob_itself.parID,
                                            &_self->ob_itself.name[1], _self->ob_itself.name[0]);
    return _res;

}

static PyMethodDef FSSpec_methods[] = {
    {"FSpOpenDF", (PyCFunction)FSSpec_FSpOpenDF, 1,
     PyDoc_STR("(SInt8 permission) -> (short refNum)")},
    {"FSpOpenRF", (PyCFunction)FSSpec_FSpOpenRF, 1,
     PyDoc_STR("(SInt8 permission) -> (short refNum)")},
    {"FSpCreate", (PyCFunction)FSSpec_FSpCreate, 1,
     PyDoc_STR("(OSType creator, OSType fileType, ScriptCode scriptTag) -> None")},
    {"FSpDirCreate", (PyCFunction)FSSpec_FSpDirCreate, 1,
     PyDoc_STR("(ScriptCode scriptTag) -> (long createdDirID)")},
    {"FSpDelete", (PyCFunction)FSSpec_FSpDelete, 1,
     PyDoc_STR("() -> None")},
    {"FSpGetFInfo", (PyCFunction)FSSpec_FSpGetFInfo, 1,
     PyDoc_STR("() -> (FInfo fndrInfo)")},
    {"FSpSetFInfo", (PyCFunction)FSSpec_FSpSetFInfo, 1,
     PyDoc_STR("(FInfo fndrInfo) -> None")},
    {"FSpSetFLock", (PyCFunction)FSSpec_FSpSetFLock, 1,
     PyDoc_STR("() -> None")},
    {"FSpRstFLock", (PyCFunction)FSSpec_FSpRstFLock, 1,
     PyDoc_STR("() -> None")},
    {"FSpRename", (PyCFunction)FSSpec_FSpRename, 1,
     PyDoc_STR("(Str255 newName) -> None")},
    {"FSpCatMove", (PyCFunction)FSSpec_FSpCatMove, 1,
     PyDoc_STR("(FSSpec dest) -> None")},
    {"FSpExchangeFiles", (PyCFunction)FSSpec_FSpExchangeFiles, 1,
     PyDoc_STR("(FSSpec dest) -> None")},
    {"FSpMakeFSRef", (PyCFunction)FSSpec_FSpMakeFSRef, 1,
     PyDoc_STR("() -> (FSRef newRef)")},
    {"NewAliasMinimal", (PyCFunction)FSSpec_NewAliasMinimal, 1,
     PyDoc_STR("() -> (AliasHandle alias)")},
    {"IsAliasFile", (PyCFunction)FSSpec_IsAliasFile, 1,
     PyDoc_STR("() -> (Boolean aliasFileFlag, Boolean folderFlag)")},
    {"as_pathname", (PyCFunction)FSSpec_as_pathname, 1,
     PyDoc_STR("() -> string")},
    {"as_tuple", (PyCFunction)FSSpec_as_tuple, 1,
     PyDoc_STR("() -> (vRefNum, dirID, name)")},
    {NULL, NULL, 0}
};

static PyObject *FSSpec_get_data(FSSpecObject *self, void *closure)
{
    return PyString_FromStringAndSize((char *)&self->ob_itself, sizeof(self->ob_itself));
}

#define FSSpec_set_data NULL

static PyGetSetDef FSSpec_getsetlist[] = {
    {"data", (getter)FSSpec_get_data, (setter)FSSpec_set_data, "Raw data of the FSSpec object"},
    {NULL, NULL, NULL, NULL},
};


#define FSSpec_compare NULL

static PyObject * FSSpec_repr(FSSpecObject *self)
{
    char buf[512];
    PyOS_snprintf(buf, sizeof(buf), "%s((%d, %ld, '%.*s'))",
        self->ob_type->tp_name,
        self->ob_itself.vRefNum,
        self->ob_itself.parID,
        self->ob_itself.name[0], self->ob_itself.name+1);
    return PyString_FromString(buf);
}

#define FSSpec_hash NULL
static int FSSpec_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    PyObject *v = NULL;
    char *rawdata = NULL;
    int rawdatalen = 0;
    static char *kw[] = {"itself", "rawdata", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "|Os#", kw, &v, &rawdata, &rawdatalen))
    return -1;
    if (v && rawdata)
    {
        PyErr_SetString(PyExc_TypeError, "Only one of itself or rawdata may be specified");
        return -1;
    }
    if (!v && !rawdata)
    {
        PyErr_SetString(PyExc_TypeError, "One of itself or rawdata must be specified");
        return -1;
    }
    if (rawdata)
    {
        if (rawdatalen != sizeof(FSSpec))
        {
            PyErr_SetString(PyExc_TypeError, "FSSpec rawdata incorrect size");
            return -1;
        }
        memcpy(&((FSSpecObject *)_self)->ob_itself, rawdata, rawdatalen);
        return 0;
    }
    if (PyMac_GetFSSpec(v, &((FSSpecObject *)_self)->ob_itself)) return 0;
    return -1;
}

#define FSSpec_tp_alloc PyType_GenericAlloc

static PyObject *FSSpec_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;

    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    memset(&((FSSpecObject *)self)->ob_itself, 0, sizeof(FSSpec));
    return self;
}

#define FSSpec_tp_free PyObject_Del


static PyTypeObject FSSpec_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "Carbon.File.FSSpec", /*tp_name*/
    sizeof(FSSpecObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) FSSpec_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) FSSpec_compare, /*tp_compare*/
    (reprfunc) FSSpec_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) FSSpec_hash, /*tp_hash*/
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
    FSSpec_methods, /* tp_methods */
    0, /*tp_members*/
    FSSpec_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    FSSpec_tp_init, /* tp_init */
    FSSpec_tp_alloc, /* tp_alloc */
    FSSpec_tp_new, /* tp_new */
    FSSpec_tp_free, /* tp_free */
};

#endif /* !__LP64__ */
/* --------------------- End object type FSSpec --------------------- */


/* ----------------------- Object type FSRef ------------------------ */

static PyTypeObject FSRef_Type;

#define FSRef_Check(x) ((x)->ob_type == &FSRef_Type || PyObject_TypeCheck((x), &FSRef_Type))

typedef struct FSRefObject {
    PyObject_HEAD
    FSRef ob_itself;
} FSRefObject;

static PyObject *FSRef_New(FSRef *itself)
{
    FSRefObject *it;
    if (itself == NULL) return PyMac_Error(resNotFound);
    it = PyObject_NEW(FSRefObject, &FSRef_Type);
    if (it == NULL) return NULL;
    it->ob_itself = *itself;
    return (PyObject *)it;
}

static void FSRef_dealloc(FSRefObject *self)
{
    /* Cleanup of self->ob_itself goes here */
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *FSRef_FSMakeFSRefUnicode(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    UniChar *nameLength__in__;
    UniCharCount nameLength__len__;
    int nameLength__in_len__;
    TextEncoding textEncodingHint;
    FSRef newRef;
    if (!PyArg_ParseTuple(_args, "u#l",
                          &nameLength__in__, &nameLength__in_len__,
                          &textEncodingHint))
        return NULL;
    nameLength__len__ = nameLength__in_len__;
    _err = FSMakeFSRefUnicode(&_self->ob_itself,
                              nameLength__len__, nameLength__in__,
                              textEncodingHint,
                              &newRef);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         FSRef_New, &newRef);
    return _res;
}

static PyObject *FSRef_FSCompareFSRefs(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSRef ref2;
    if (!PyArg_ParseTuple(_args, "O&",
                          FSRef_Convert, &ref2))
        return NULL;
    _err = FSCompareFSRefs(&_self->ob_itself,
                           &ref2);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSRef_FSCreateFileUnicode(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    UniChar *nameLength__in__;
    UniCharCount nameLength__len__;
    int nameLength__in_len__;
    FSCatalogInfoBitmap whichInfo;
    FSCatalogInfo catalogInfo;
    FSRef newRef;
#ifndef __LP64__
    FSSpec newSpec;
#endif
    if (!PyArg_ParseTuple(_args, "u#lO&",
                          &nameLength__in__, &nameLength__in_len__,
                          &whichInfo,
                          FSCatalogInfo_Convert, &catalogInfo))
        return NULL;
    nameLength__len__ = nameLength__in_len__;
    _err = FSCreateFileUnicode(&_self->ob_itself,
                               nameLength__len__, nameLength__in__,
                               whichInfo,
                               &catalogInfo,
                               &newRef,
#ifndef __LP64__
                               &newSpec
#else   /* __LP64__ */
                               NULL
#endif /* __LP64__*/
                              );
    if (_err != noErr) return PyMac_Error(_err);

#ifndef __LP64__
    _res = Py_BuildValue("O&O&",
                         FSRef_New, &newRef,
                         FSSpec_New, &newSpec);
#else /* __LP64__ */
    _res = Py_BuildValue("O&O", FSRef_New, &newRef, Py_None);
#endif /* __LP64__ */

    return _res;
}

static PyObject *FSRef_FSCreateDirectoryUnicode(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    UniChar *nameLength__in__;
    UniCharCount nameLength__len__;
    int nameLength__in_len__;
    FSCatalogInfoBitmap whichInfo;
    FSCatalogInfo catalogInfo;
    FSRef newRef;
#ifndef __LP64__
    FSSpec newSpec;
#endif /* !__LP64__ */
    UInt32 newDirID;
    if (!PyArg_ParseTuple(_args, "u#lO&",
                          &nameLength__in__, &nameLength__in_len__,
                          &whichInfo,
                          FSCatalogInfo_Convert, &catalogInfo))
        return NULL;
    nameLength__len__ = nameLength__in_len__;
    _err = FSCreateDirectoryUnicode(&_self->ob_itself,
                                    nameLength__len__, nameLength__in__,
                                    whichInfo,
                                    &catalogInfo,
                                    &newRef,
#ifndef __LP64__
                                    &newSpec,
#else /* !__LP64__ */
                                    NULL,
#endif /* !__LP64__ */
                                    &newDirID);
    if (_err != noErr) return PyMac_Error(_err);

#ifndef __LP64__
    _res = Py_BuildValue("O&O&l",
                         FSRef_New, &newRef,
                         FSSpec_New, &newSpec,
                         newDirID);
#else   /* __LP64__ */
    _res = Py_BuildValue("O&Ol",
                         FSRef_New, &newRef,
                         Py_None,
                         newDirID);
#endif /* __LP64__ */
    return _res;
}

static PyObject *FSRef_FSDeleteObject(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = FSDeleteObject(&_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSRef_FSMoveObject(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSRef destDirectory;
    FSRef newRef;
    if (!PyArg_ParseTuple(_args, "O&",
                          FSRef_Convert, &destDirectory))
        return NULL;
    _err = FSMoveObject(&_self->ob_itself,
                        &destDirectory,
                        &newRef);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         FSRef_New, &newRef);
    return _res;
}

static PyObject *FSRef_FSExchangeObjects(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSRef destRef;
    if (!PyArg_ParseTuple(_args, "O&",
                          FSRef_Convert, &destRef))
        return NULL;
    _err = FSExchangeObjects(&_self->ob_itself,
                             &destRef);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSRef_FSRenameUnicode(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    UniChar *nameLength__in__;
    UniCharCount nameLength__len__;
    int nameLength__in_len__;
    TextEncoding textEncodingHint;
    FSRef newRef;
    if (!PyArg_ParseTuple(_args, "u#l",
                          &nameLength__in__, &nameLength__in_len__,
                          &textEncodingHint))
        return NULL;
    nameLength__len__ = nameLength__in_len__;
    _err = FSRenameUnicode(&_self->ob_itself,
                           nameLength__len__, nameLength__in__,
                           textEncodingHint,
                           &newRef);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         FSRef_New, &newRef);
    return _res;
}

static PyObject *FSRef_FSGetCatalogInfo(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSCatalogInfoBitmap whichInfo;
    FSCatalogInfo catalogInfo;
    HFSUniStr255 outName;
#ifndef __LP64__
    FSSpec fsSpec;
#endif /* !__LP64__ */
    FSRef parentRef;
    if (!PyArg_ParseTuple(_args, "l",
                          &whichInfo))
        return NULL;
    _err = FSGetCatalogInfo(&_self->ob_itself,
                            whichInfo,
                            &catalogInfo,
                            &outName,
#ifndef __LP64__
                            &fsSpec,
#else   /* __LP64__ */
                            NULL,
#endif /* __LP64__ */
                            &parentRef);
    if (_err != noErr) return PyMac_Error(_err);

#ifndef __LP64__
    _res = Py_BuildValue("O&O&O&O&",
                         FSCatalogInfo_New, &catalogInfo,
                         PyMac_BuildHFSUniStr255, &outName,
                         FSSpec_New, &fsSpec,
                         FSRef_New, &parentRef);
#else   /* __LP64__ */
    _res = Py_BuildValue("O&O&OO&",
                         FSCatalogInfo_New, &catalogInfo,
                         PyMac_BuildHFSUniStr255, &outName,
                         Py_None,
                         FSRef_New, &parentRef);
#endif /* __LP64__ */
    return _res;
}

static PyObject *FSRef_FSSetCatalogInfo(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSCatalogInfoBitmap whichInfo;
    FSCatalogInfo catalogInfo;
    if (!PyArg_ParseTuple(_args, "lO&",
                          &whichInfo,
                          FSCatalogInfo_Convert, &catalogInfo))
        return NULL;
    _err = FSSetCatalogInfo(&_self->ob_itself,
                            whichInfo,
                            &catalogInfo);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSRef_FSCreateFork(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    UniChar *forkNameLength__in__;
    UniCharCount forkNameLength__len__;
    int forkNameLength__in_len__;
    if (!PyArg_ParseTuple(_args, "u#",
                          &forkNameLength__in__, &forkNameLength__in_len__))
        return NULL;
    forkNameLength__len__ = forkNameLength__in_len__;
    _err = FSCreateFork(&_self->ob_itself,
                        forkNameLength__len__, forkNameLength__in__);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSRef_FSDeleteFork(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    UniChar *forkNameLength__in__;
    UniCharCount forkNameLength__len__;
    int forkNameLength__in_len__;
    if (!PyArg_ParseTuple(_args, "u#",
                          &forkNameLength__in__, &forkNameLength__in_len__))
        return NULL;
    forkNameLength__len__ = forkNameLength__in_len__;
    _err = FSDeleteFork(&_self->ob_itself,
                        forkNameLength__len__, forkNameLength__in__);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSRef_FSOpenFork(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    UniChar *forkNameLength__in__;
    UniCharCount forkNameLength__len__;
    int forkNameLength__in_len__;
    SInt8 permissions;
    FSIORefNum forkRefNum;
    if (!PyArg_ParseTuple(_args, "u#b",
                          &forkNameLength__in__, &forkNameLength__in_len__,
                          &permissions))
        return NULL;
    forkNameLength__len__ = forkNameLength__in_len__;
    _err = FSOpenFork(&_self->ob_itself,
                      forkNameLength__len__, forkNameLength__in__,
                      permissions,
                      &forkRefNum);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("h",
                         forkRefNum);
    return _res;
}

static PyObject *FSRef_FNNotify(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    FNMessage message;
    OptionBits flags;
    if (!PyArg_ParseTuple(_args, "ll",
                          &message,
                          &flags))
        return NULL;
    _err = FNNotify(&_self->ob_itself,
                    message,
                    flags);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *FSRef_FSNewAliasMinimal(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    AliasHandle inAlias;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = FSNewAliasMinimal(&_self->ob_itself,
                             &inAlias);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         Alias_New, inAlias);
    return _res;
}

static PyObject *FSRef_FSIsAliasFile(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    Boolean aliasFileFlag;
    Boolean folderFlag;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = FSIsAliasFile(&_self->ob_itself,
                         &aliasFileFlag,
                         &folderFlag);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("bb",
                         aliasFileFlag,
                         folderFlag);
    return _res;
}

static PyObject *FSRef_FSRefMakePath(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    OSStatus _err;
#define MAXPATHNAME 1024
    UInt8 path[MAXPATHNAME];
    UInt32 maxPathSize = MAXPATHNAME;

    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = FSRefMakePath(&_self->ob_itself,
                                             path,
                                             maxPathSize);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("s", path);
    return _res;

}

static PyObject *FSRef_as_pathname(FSRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _res = FSRef_FSRefMakePath(_self, _args);
    return _res;

}

static PyMethodDef FSRef_methods[] = {
    {"FSMakeFSRefUnicode", (PyCFunction)FSRef_FSMakeFSRefUnicode, 1,
     PyDoc_STR("(Buffer nameLength, TextEncoding textEncodingHint) -> (FSRef newRef)")},
    {"FSCompareFSRefs", (PyCFunction)FSRef_FSCompareFSRefs, 1,
     PyDoc_STR("(FSRef ref2) -> None")},
    {"FSCreateFileUnicode", (PyCFunction)FSRef_FSCreateFileUnicode, 1,
     PyDoc_STR("(Buffer nameLength, FSCatalogInfoBitmap whichInfo, FSCatalogInfo catalogInfo) -> (FSRef newRef, FSSpec newSpec)")},
    {"FSCreateDirectoryUnicode", (PyCFunction)FSRef_FSCreateDirectoryUnicode, 1,
     PyDoc_STR("(Buffer nameLength, FSCatalogInfoBitmap whichInfo, FSCatalogInfo catalogInfo) -> (FSRef newRef, FSSpec newSpec, UInt32 newDirID)")},
    {"FSDeleteObject", (PyCFunction)FSRef_FSDeleteObject, 1,
     PyDoc_STR("() -> None")},
    {"FSMoveObject", (PyCFunction)FSRef_FSMoveObject, 1,
     PyDoc_STR("(FSRef destDirectory) -> (FSRef newRef)")},
    {"FSExchangeObjects", (PyCFunction)FSRef_FSExchangeObjects, 1,
     PyDoc_STR("(FSRef destRef) -> None")},
    {"FSRenameUnicode", (PyCFunction)FSRef_FSRenameUnicode, 1,
     PyDoc_STR("(Buffer nameLength, TextEncoding textEncodingHint) -> (FSRef newRef)")},
    {"FSGetCatalogInfo", (PyCFunction)FSRef_FSGetCatalogInfo, 1,
     PyDoc_STR("(FSCatalogInfoBitmap whichInfo) -> (FSCatalogInfo catalogInfo, HFSUniStr255 outName, FSSpec fsSpec, FSRef parentRef)")},
    {"FSSetCatalogInfo", (PyCFunction)FSRef_FSSetCatalogInfo, 1,
     PyDoc_STR("(FSCatalogInfoBitmap whichInfo, FSCatalogInfo catalogInfo) -> None")},
    {"FSCreateFork", (PyCFunction)FSRef_FSCreateFork, 1,
     PyDoc_STR("(Buffer forkNameLength) -> None")},
    {"FSDeleteFork", (PyCFunction)FSRef_FSDeleteFork, 1,
     PyDoc_STR("(Buffer forkNameLength) -> None")},
    {"FSOpenFork", (PyCFunction)FSRef_FSOpenFork, 1,
     PyDoc_STR("(Buffer forkNameLength, SInt8 permissions) -> (SInt16 forkRefNum)")},
    {"FNNotify", (PyCFunction)FSRef_FNNotify, 1,
     PyDoc_STR("(FNMessage message, OptionBits flags) -> None")},
    {"FSNewAliasMinimal", (PyCFunction)FSRef_FSNewAliasMinimal, 1,
     PyDoc_STR("() -> (AliasHandle inAlias)")},
    {"FSIsAliasFile", (PyCFunction)FSRef_FSIsAliasFile, 1,
     PyDoc_STR("() -> (Boolean aliasFileFlag, Boolean folderFlag)")},
    {"FSRefMakePath", (PyCFunction)FSRef_FSRefMakePath, 1,
     PyDoc_STR("() -> string")},
    {"as_pathname", (PyCFunction)FSRef_as_pathname, 1,
     PyDoc_STR("() -> string")},
    {NULL, NULL, 0}
};

static PyObject *FSRef_get_data(FSRefObject *self, void *closure)
{
    return PyString_FromStringAndSize((char *)&self->ob_itself, sizeof(self->ob_itself));
}

#define FSRef_set_data NULL

static PyGetSetDef FSRef_getsetlist[] = {
    {"data", (getter)FSRef_get_data, (setter)FSRef_set_data, "Raw data of the FSRef object"},
    {NULL, NULL, NULL, NULL},
};


#define FSRef_compare NULL

#define FSRef_repr NULL

#define FSRef_hash NULL
static int FSRef_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    PyObject *v = NULL;
    char *rawdata = NULL;
    int rawdatalen = 0;
    static char *kw[] = {"itself", "rawdata", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "|Os#", kw, &v, &rawdata, &rawdatalen))
    return -1;
    if (v && rawdata)
    {
        PyErr_SetString(PyExc_TypeError, "Only one of itself or rawdata may be specified");
        return -1;
    }
    if (!v && !rawdata)
    {
        PyErr_SetString(PyExc_TypeError, "One of itself or rawdata must be specified");
        return -1;
    }
    if (rawdata)
    {
        if (rawdatalen != sizeof(FSRef))
        {
            PyErr_SetString(PyExc_TypeError, "FSRef rawdata incorrect size");
            return -1;
        }
        memcpy(&((FSRefObject *)_self)->ob_itself, rawdata, rawdatalen);
        return 0;
    }
    if (PyMac_GetFSRef(v, &((FSRefObject *)_self)->ob_itself)) return 0;
    return -1;
}

#define FSRef_tp_alloc PyType_GenericAlloc

static PyObject *FSRef_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;

    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    memset(&((FSRefObject *)self)->ob_itself, 0, sizeof(FSRef));
    return self;
}

#define FSRef_tp_free PyObject_Del


static PyTypeObject FSRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "Carbon.File.FSRef", /*tp_name*/
    sizeof(FSRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) FSRef_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) FSRef_compare, /*tp_compare*/
    (reprfunc) FSRef_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) FSRef_hash, /*tp_hash*/
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
    FSRef_methods, /* tp_methods */
    0, /*tp_members*/
    FSRef_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    FSRef_tp_init, /* tp_init */
    FSRef_tp_alloc, /* tp_alloc */
    FSRef_tp_new, /* tp_new */
    FSRef_tp_free, /* tp_free */
};

/* --------------------- End object type FSRef ---------------------- */

#ifndef __LP64__
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
                         FInfo_New, &fndrInfo);
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
                          FInfo_Convert, &fndrInfo))
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
                         FSSpec_New, &spec);
    return _res;
}
#endif /* !__LP64__ */

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
    UInt8 * path;
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
                         FSRef_New, &ref,
                         isDirectory);
    return _res;
}

static PyObject *File_FNNotifyByPath(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt8 * path;
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

#ifndef __LP64__
static PyObject *File_NewAlias(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSSpec fromFile__buf__;
    FSSpec *fromFile = &fromFile__buf__;
    FSSpec target;
    AliasHandle alias;
    if (!PyArg_ParseTuple(_args, "O&O&",
                          myPyMac_GetOptFSSpecPtr, &fromFile,
                          FSSpec_Convert, &target))
        return NULL;
    _err = NewAlias(fromFile,
                    &target,
                    &alias);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         Alias_New, alias);
    return _res;
}

static PyObject *File_NewAliasMinimalFromFullPath(PyObject *_self, PyObject *_args)
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
                         Alias_New, alias);
    return _res;
}

static PyObject *File_ResolveAliasFile(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSSpec theSpec;
    Boolean resolveAliasChains;
    Boolean targetIsFolder;
    Boolean wasAliased;
    if (!PyArg_ParseTuple(_args, "O&b",
                          FSSpec_Convert, &theSpec,
                          &resolveAliasChains))
        return NULL;
    _err = ResolveAliasFile(&theSpec,
                            resolveAliasChains,
                            &targetIsFolder,
                            &wasAliased);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&bb",
                         FSSpec_New, &theSpec,
                         targetIsFolder,
                         wasAliased);
    return _res;
}

static PyObject *File_ResolveAliasFileWithMountFlags(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSSpec theSpec;
    Boolean resolveAliasChains;
    Boolean targetIsFolder;
    Boolean wasAliased;
    unsigned long mountFlags;
    if (!PyArg_ParseTuple(_args, "O&bl",
                          FSSpec_Convert, &theSpec,
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
                         FSSpec_New, &theSpec,
                         targetIsFolder,
                         wasAliased);
    return _res;
}

static PyObject *File_UpdateAlias(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSSpec fromFile__buf__;
    FSSpec *fromFile = &fromFile__buf__;
    FSSpec target;
    AliasHandle alias;
    Boolean wasChanged;
    if (!PyArg_ParseTuple(_args, "O&O&O&",
                          myPyMac_GetOptFSSpecPtr, &fromFile,
                          FSSpec_Convert, &target,
                          Alias_Convert, &alias))
        return NULL;
    _err = UpdateAlias(fromFile,
                       &target,
                       alias,
                       &wasChanged);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("b",
                         wasChanged);
    return _res;
}

static PyObject *File_ResolveAliasFileWithMountFlagsNoUI(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSSpec theSpec;
    Boolean resolveAliasChains;
    Boolean targetIsFolder;
    Boolean wasAliased;
    unsigned long mountFlags;
    if (!PyArg_ParseTuple(_args, "O&bl",
                          FSSpec_Convert, &theSpec,
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
                         FSSpec_New, &theSpec,
                         targetIsFolder,
                         wasAliased);
    return _res;
}
#endif /* !__LP64__ */

static PyObject *File_FSNewAlias(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSRef fromFile__buf__;
    FSRef *fromFile = &fromFile__buf__;
    FSRef target;
    AliasHandle inAlias;
    if (!PyArg_ParseTuple(_args, "O&O&",
                          myPyMac_GetOptFSRefPtr, &fromFile,
                          FSRef_Convert, &target))
        return NULL;
    _err = FSNewAlias(fromFile,
                      &target,
                      &inAlias);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         Alias_New, inAlias);
    return _res;
}

static PyObject *File_FSResolveAliasFileWithMountFlags(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSRef theRef;
    Boolean resolveAliasChains;
    Boolean targetIsFolder;
    Boolean wasAliased;
    unsigned long mountFlags;
    if (!PyArg_ParseTuple(_args, "O&bl",
                          FSRef_Convert, &theRef,
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
                         FSRef_New, &theRef,
                         targetIsFolder,
                         wasAliased);
    return _res;
}

static PyObject *File_FSResolveAliasFile(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSRef theRef;
    Boolean resolveAliasChains;
    Boolean targetIsFolder;
    Boolean wasAliased;
    if (!PyArg_ParseTuple(_args, "O&b",
                          FSRef_Convert, &theRef,
                          &resolveAliasChains))
        return NULL;
    _err = FSResolveAliasFile(&theRef,
                              resolveAliasChains,
                              &targetIsFolder,
                              &wasAliased);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&bb",
                         FSRef_New, &theRef,
                         targetIsFolder,
                         wasAliased);
    return _res;
}

static PyObject *File_FSUpdateAlias(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    FSRef fromFile__buf__;
    FSRef *fromFile = &fromFile__buf__;
    FSRef target;
    AliasHandle alias;
    Boolean wasChanged;
    if (!PyArg_ParseTuple(_args, "O&O&O&",
                          myPyMac_GetOptFSRefPtr, &fromFile,
                          FSRef_Convert, &target,
                          Alias_Convert, &alias))
        return NULL;
    _err = FSUpdateAlias(fromFile,
                         &target,
                         alias,
                         &wasChanged);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("b",
                         wasChanged);
    return _res;
}

static PyObject *File_pathname(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    PyObject *obj;

    if (!PyArg_ParseTuple(_args, "O", &obj))
        return NULL;
    if (PyString_Check(obj)) {
        Py_INCREF(obj);
        return obj;
    }
    if (PyUnicode_Check(obj))
        return PyUnicode_AsEncodedString(obj, "utf8", "strict");
    _res = PyObject_CallMethod(obj, "as_pathname", NULL);
    return _res;

}

static PyMethodDef File_methods[] = {
#ifndef __LP64__
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
#endif /* !__LP64__*/
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
     PyDoc_STR("(UInt8 * path) -> (FSRef ref, Boolean isDirectory)")},
    {"FNNotifyByPath", (PyCFunction)File_FNNotifyByPath, 1,
     PyDoc_STR("(UInt8 * path, FNMessage message, OptionBits flags) -> None")},
    {"FNNotifyAll", (PyCFunction)File_FNNotifyAll, 1,
     PyDoc_STR("(FNMessage message, OptionBits flags) -> None")},
#ifndef  __LP64__
    {"NewAlias", (PyCFunction)File_NewAlias, 1,
     PyDoc_STR("(FSSpec fromFile, FSSpec target) -> (AliasHandle alias)")},
    {"NewAliasMinimalFromFullPath", (PyCFunction)File_NewAliasMinimalFromFullPath, 1,
     PyDoc_STR("(Buffer fullPath, Str32 zoneName, Str31 serverName) -> (AliasHandle alias)")},
    {"ResolveAliasFile", (PyCFunction)File_ResolveAliasFile, 1,
     PyDoc_STR("(FSSpec theSpec, Boolean resolveAliasChains) -> (FSSpec theSpec, Boolean targetIsFolder, Boolean wasAliased)")},
    {"ResolveAliasFileWithMountFlags", (PyCFunction)File_ResolveAliasFileWithMountFlags, 1,
     PyDoc_STR("(FSSpec theSpec, Boolean resolveAliasChains, unsigned long mountFlags) -> (FSSpec theSpec, Boolean targetIsFolder, Boolean wasAliased)")},
    {"UpdateAlias", (PyCFunction)File_UpdateAlias, 1,
     PyDoc_STR("(FSSpec fromFile, FSSpec target, AliasHandle alias) -> (Boolean wasChanged)")},
    {"ResolveAliasFileWithMountFlagsNoUI", (PyCFunction)File_ResolveAliasFileWithMountFlagsNoUI, 1,
     PyDoc_STR("(FSSpec theSpec, Boolean resolveAliasChains, unsigned long mountFlags) -> (FSSpec theSpec, Boolean targetIsFolder, Boolean wasAliased)")},
#endif /* !__LP64__ */
    {"FSNewAlias", (PyCFunction)File_FSNewAlias, 1,
     PyDoc_STR("(FSRef fromFile, FSRef target) -> (AliasHandle inAlias)")},
    {"FSResolveAliasFileWithMountFlags", (PyCFunction)File_FSResolveAliasFileWithMountFlags, 1,
     PyDoc_STR("(FSRef theRef, Boolean resolveAliasChains, unsigned long mountFlags) -> (FSRef theRef, Boolean targetIsFolder, Boolean wasAliased)")},
    {"FSResolveAliasFile", (PyCFunction)File_FSResolveAliasFile, 1,
     PyDoc_STR("(FSRef theRef, Boolean resolveAliasChains) -> (FSRef theRef, Boolean targetIsFolder, Boolean wasAliased)")},
    {"FSUpdateAlias", (PyCFunction)File_FSUpdateAlias, 1,
     PyDoc_STR("(FSRef fromFile, FSRef target, AliasHandle alias) -> (Boolean wasChanged)")},
    {"pathname", (PyCFunction)File_pathname, 1,
     PyDoc_STR("(str|unicode|FSSpec|FSref) -> pathname")},
    {NULL, NULL, 0}
};


#ifndef __LP64__
int
PyMac_GetFSSpec(PyObject *v, FSSpec *spec)
{
    Str255 path;
    short refnum;
    long parid;
    OSErr err;
    FSRef fsr;

    if (FSSpec_Check(v)) {
        *spec = ((FSSpecObject *)v)->ob_itself;
        return 1;
    }

    if (PyArg_Parse(v, "(hlO&)",
                                            &refnum, &parid, PyMac_GetStr255, &path)) {
        err = FSMakeFSSpec(refnum, parid, path, spec);
        if ( err && err != fnfErr ) {
            PyMac_Error(err);
            return 0;
        }
        return 1;
    }
    PyErr_Clear();
    /* Otherwise we try to go via an FSRef. On OSX we go all the way,
    ** on OS9 we accept only a real FSRef object
    */
    if ( PyMac_GetFSRef(v, &fsr) ) {
        err = FSGetCatalogInfo(&fsr, kFSCatInfoNone, NULL, NULL, spec, NULL);
        if (err != noErr) {
            PyMac_Error(err);
            return 0;
        }
        return 1;
    }
    return 0;
}
#endif /* !__LP64__ */

int
PyMac_GetFSRef(PyObject *v, FSRef *fsr)
{
    OSStatus err;
#ifndef __LP64__
    FSSpec fss;
#endif /* !__LP64__ */

    if (FSRef_Check(v)) {
        *fsr = ((FSRefObject *)v)->ob_itself;
        return 1;
    }

    /* On OSX we now try a pathname */
    if ( PyString_Check(v) || PyUnicode_Check(v)) {
        char *path = NULL;
        if (!PyArg_Parse(v, "et", Py_FileSystemDefaultEncoding, &path))
            return 0;
        if ( (err=FSPathMakeRef((unsigned char*)path, fsr, NULL)) )
            PyMac_Error(err);
        PyMem_Free(path);
        return !err;
    }
    /* XXXX Should try unicode here too */

#ifndef __LP64__
    /* Otherwise we try to go via an FSSpec */
    if (FSSpec_Check(v)) {
        fss = ((FSSpecObject *)v)->ob_itself;
        if ((err=FSpMakeFSRef(&fss, fsr)) == 0)
            return 1;
        PyMac_Error(err);
        return 0;
    }
#endif /* !__LP64__ */

    PyErr_SetString(PyExc_TypeError, "FSRef, FSSpec or pathname required");
    return 0;
}

#ifndef __LP64__
extern PyObject *
PyMac_BuildFSSpec(FSSpec *spec)
{
    return FSSpec_New(spec);
}
#endif /* !__LP64__ */

extern PyObject *
PyMac_BuildFSRef(FSRef *spec)
{
    return FSRef_New(spec);
}


void init_File(void)
{
    PyObject *m;
    PyObject *d;


#ifndef __LP64__
    PyMac_INIT_TOOLBOX_OBJECT_NEW(FSSpec *, PyMac_BuildFSSpec);
    PyMac_INIT_TOOLBOX_OBJECT_CONVERT(FSSpec, PyMac_GetFSSpec);
#endif /* !__LP64__ */

    PyMac_INIT_TOOLBOX_OBJECT_NEW(FSRef *, PyMac_BuildFSRef);
    PyMac_INIT_TOOLBOX_OBJECT_CONVERT(FSRef, PyMac_GetFSRef);


    m = Py_InitModule("_File", File_methods);
    d = PyModule_GetDict(m);
    File_Error = PyMac_GetOSErrException();
    if (File_Error == NULL ||
        PyDict_SetItemString(d, "Error", File_Error) != 0)
        return;
    FSCatalogInfo_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&FSCatalogInfo_Type) < 0) return;
    Py_INCREF(&FSCatalogInfo_Type);
    PyModule_AddObject(m, "FSCatalogInfo", (PyObject *)&FSCatalogInfo_Type);
    /* Backward-compatible name */
    Py_INCREF(&FSCatalogInfo_Type);
    PyModule_AddObject(m, "FSCatalogInfoType", (PyObject *)&FSCatalogInfo_Type);

#ifndef __LP64__
    FInfo_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&FInfo_Type) < 0) return;
    Py_INCREF(&FInfo_Type);
    PyModule_AddObject(m, "FInfo", (PyObject *)&FInfo_Type);
    /* Backward-compatible name */
    Py_INCREF(&FInfo_Type);
    PyModule_AddObject(m, "FInfoType", (PyObject *)&FInfo_Type);
#endif /* !__LP64__ */
    Alias_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&Alias_Type) < 0) return;
    Py_INCREF(&Alias_Type);
    PyModule_AddObject(m, "Alias", (PyObject *)&Alias_Type);
    /* Backward-compatible name */
    Py_INCREF(&Alias_Type);
    PyModule_AddObject(m, "AliasType", (PyObject *)&Alias_Type);

#ifndef __LP64__
    FSSpec_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&FSSpec_Type) < 0) return;
    Py_INCREF(&FSSpec_Type);
    PyModule_AddObject(m, "FSSpec", (PyObject *)&FSSpec_Type);
    /* Backward-compatible name */
    Py_INCREF(&FSSpec_Type);
    PyModule_AddObject(m, "FSSpecType", (PyObject *)&FSSpec_Type);
#endif /* !__LP64__ */
    FSRef_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&FSRef_Type) < 0) return;
    Py_INCREF(&FSRef_Type);
    PyModule_AddObject(m, "FSRef", (PyObject *)&FSRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&FSRef_Type);
    PyModule_AddObject(m, "FSRefType", (PyObject *)&FSRef_Type);
}

/* ======================== End module _File ======================== */

