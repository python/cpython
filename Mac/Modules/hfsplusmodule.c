/*
  $Log$
  Revision 1.2  2001/11/06 12:06:39  jackjansen
  First couple of fixes to make it compile with Universal 3.3.2.

  Revision 1.8  2001/10/03 17:29:01  ganatra
  add parent method to FSRef class

  Revision 1.7  2001/04/13 20:54:19  ganatra
  More standard format for MacOSError exceptions

  Revision 1.6  2001/04/11 04:07:40  ganatra
  Add permissions constants and log header..

*/


#include "Python.h"
#ifdef WITHOUT_FRAMEWORKS
#include <Files.h>
#else
#include <CoreServices/CoreServices.h>
#endif

static PyObject *
dict_from_cataloginfo(FSCatalogInfoBitmap bitmap, const FSCatalogInfo *info, HFSUniStr255 *uni);
static
PyObject *macos_error_for_call(OSErr err, const char *name, const char *item);
static
int insert_long(FSCatalogInfoBitmap bitmap, UInt32 bit, PyObject *dict, const char *symbol, UInt32 value);
static
int insert_slong(FSCatalogInfoBitmap bitmap, UInt32 bit, PyObject *dict, const char *symbol, long value);
static
int insert_int(PyObject *d, const char *symbol, int value);
static
Boolean fsref_isdirectory(const FSRef *ref);
static
PyObject *obj_to_hfsunistr(PyObject *in, HFSUniStr255 *uni);

static
int cataloginfo_from_dict(FSCatalogInfoBitmap bitmap, FSCatalogInfo *info, const PyObject *dict);


static PyObject *ErrorObject;

//__________________________________________________________________________________________________
//_______________________________________ FORKREF OBJECT ___________________________________________
//__________________________________________________________________________________________________

typedef struct {
	PyObject_HEAD
	PyObject	*x_attr;
	short		forkref;
} forkRefObject;

staticforward PyTypeObject forkRefObject_Type;

#define forkRefObject_Check(v)	((v)->ob_type == &forkRefObject_Type)

//__________________________________________________________________________________________________

static
forkRefObject *newForkRefObject(PyObject *arg, FSRef *ref, Boolean resourceFork, SInt8 permissions)
{
	OSErr err;
	HFSUniStr255 forkName;

	forkRefObject *self = PyObject_New(forkRefObject, &forkRefObject_Type);
	if (self == NULL)
		return NULL;

	if (resourceFork)
		(void) FSGetResourceForkName(&forkName);
	else
		(void) FSGetDataForkName(&forkName);

	Py_BEGIN_ALLOW_THREADS
	err = FSOpenFork(ref, forkName.length, forkName.unicode, permissions, &self->forkref);
	Py_END_ALLOW_THREADS
	if (err != noErr) {
		Py_DECREF(self);
		return (forkRefObject *) macos_error_for_call(err, "FSOpenFork", NULL);
	}

	return self;
}

//__________________________________________________________________________________________________
// forkRefObject methods
//
static
void forkRefObject_dealloc(forkRefObject *self)
{
	Py_BEGIN_ALLOW_THREADS
	if (self->forkref != -1)
		FSClose(self->forkref);
	Py_END_ALLOW_THREADS
	PyObject_Del(self);
}

//__________________________________________________________________________________________________
static char forkRefObject_read__doc__[] =
"read([bytecount[, posmode[, offset]]]) -> String\n\n\
Read bytes from a fork, optionally passing number of\n\
bytes (default: 128 bytes), posmode (below) and offset:\n\
	0: ignore offset; write at current mark (default)\n\
	1: offset relative to the start of fork\n\
	2: offset relative to the end of fork\n\
	3: offset relative to current fork position\n\
\n\
Returns a string containing the contents of the buffer";

static
PyObject *forkRefObject_read(forkRefObject *self, PyObject *args)
{
	OSErr err;
	ByteCount request = 128, actual;
	unsigned posmode = fsAtMark;
	long tempoffset = 0;
	SInt64 posoffset;
	PyObject *buffer;

	if (!PyArg_ParseTuple(args, "|lil:read", &request, &posmode, &tempoffset)) 
		return NULL;

	posoffset = tempoffset;
	buffer = PyString_FromStringAndSize((char *)NULL, request);
	if (buffer == NULL)
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	err = FSReadFork(self->forkref, posmode, posoffset, request, PyString_AsString(buffer), &actual);
	Py_END_ALLOW_THREADS
	if ((err != noErr) && (err != eofErr)) {
		Py_DECREF(buffer);
		return macos_error_for_call(err, "FSReadFork", NULL);
	}

	if (actual != request)
		_PyString_Resize(&buffer, actual);

	Py_INCREF(buffer);
	return buffer;
}

//__________________________________________________________________________________________________
static char forkRefObject_write__doc__[] =
"write(buffer [bytecount[, posmode[, offset]]]) -> None\n\n\
Write buffer to fork, optionally passing number of bytes, posmode (below) and offset:\n\
	0: ignore offset; write at current mark (default)\n\
	1: offset relative to the start of fork\n\
	2: offset relative to the end of fork\n\
	3: offset relative to current fork position";

static
PyObject *forkRefObject_write(forkRefObject *self, PyObject *args)
{
	OSErr err;
	ByteCount request = -1, actual;
	int size;
	char *buffer;
	unsigned posmode = fsAtMark;
	long tempoffset = 0;
	SInt64 posoffset;
	
	if (!PyArg_ParseTuple(args, "s#|lil:write", &buffer, &size, &request, &posmode, &tempoffset)) 
		return NULL;

	posoffset = tempoffset;
	if (request == -1)
		request = size;

	Py_BEGIN_ALLOW_THREADS
	err = FSWriteFork(self->forkref, posmode, posoffset, request, buffer, &actual);
	Py_END_ALLOW_THREADS
	if (err)
		return macos_error_for_call(err, "FSWriteFork", NULL);

	Py_INCREF(Py_None);
	return Py_None;
}

//__________________________________________________________________________________________________
static char forkRefObject_close__doc__[] =
"close() -> None\n\n\
Close a reference to an open fork.";

static
PyObject *forkRefObject_close(forkRefObject *self, PyObject *args)
{
	OSErr err;

	Py_BEGIN_ALLOW_THREADS
	err = FSClose(self->forkref);
	Py_END_ALLOW_THREADS
	if (err)
		return macos_error_for_call(err, "FSClose", NULL);

	self->forkref = -1;

	Py_INCREF(Py_None);
	return Py_None;
}

//__________________________________________________________________________________________________
static char forkRefObject_seek__doc__[] =
"seek(offset [,posmode]) -> None\n\n\
Set the current position in the fork with an optional posmode:\n\
	1: offset relative to the start of fork (default)\n\
	2: offset relative to the end of fork\n\
	3: offset relative to current fork position";

static
PyObject *forkRefObject_seek(forkRefObject *self, PyObject *args)
{
	OSErr err;
	unsigned posmode = fsFromStart;
	long tempoffset = 0;
	SInt64 posoffset;

	if (!PyArg_ParseTuple(args, "l|l:seek", &tempoffset, &posmode)) 
		return NULL;

	posoffset = tempoffset;
	Py_BEGIN_ALLOW_THREADS
	err = FSSetForkPosition(self->forkref, posmode, posoffset);
	Py_END_ALLOW_THREADS

	if (err)
		return macos_error_for_call(err, "FSSetForkPosition", NULL);

	Py_INCREF(Py_None);
	return Py_None;
}

//__________________________________________________________________________________________________
static char forkRefObject_resize__doc__[] =
"resize(offset [,posmode]) -> None\n\n\
Set the fork size with an optional posmode:\n\
	1: offset relative to the start of fork (default)\n\
	2: offset relative to the end of fork\n\
	3: offset relative to current fork position";

static
PyObject *forkRefObject_resize(forkRefObject *self, PyObject *args)
{
	OSErr err;
	unsigned posmode = fsFromStart;
	long tempoffset = 0;
	SInt64 posoffset;

	if (!PyArg_ParseTuple(args, "l|l:resize", &tempoffset, &posmode)) 
		return NULL;

	posoffset = tempoffset;
	Py_BEGIN_ALLOW_THREADS
	err = FSSetForkSize(self->forkref, posmode, posoffset);
	Py_END_ALLOW_THREADS

	if (err)
		return macos_error_for_call(err, "FSSetForkSize", NULL);

	Py_INCREF(Py_None);
	return Py_None;
}

//__________________________________________________________________________________________________
static char forkRefObject_tell__doc__[] =
"tell() -> current position (Long)\n\n\
Return the current position in the fork.";

static
PyObject *forkRefObject_tell(forkRefObject *self, PyObject *args)
{
	OSErr	err;
	SInt64	position;

	Py_BEGIN_ALLOW_THREADS
	err = FSGetForkPosition(self->forkref, &position);
	Py_END_ALLOW_THREADS
	if (err)
		return macos_error_for_call(err, "FSGetForkPosition", NULL);

	return PyLong_FromLongLong(position);
}

//__________________________________________________________________________________________________
static char forkRefObject_length__doc__[] =
"length() -> fork length (Long)\n\n\
Return the logical length of the fork.";

static
PyObject *forkRefObject_length(forkRefObject *self, PyObject *args)
{
	OSErr	err;
	SInt64	size;

	Py_BEGIN_ALLOW_THREADS
	err = FSGetForkSize(self->forkref, &size);
	Py_END_ALLOW_THREADS
	if (err)
		return macos_error_for_call(err, "FSGetForkSize", NULL);

	return PyLong_FromLongLong(size);
}

//__________________________________________________________________________________________________

static PyMethodDef forkRefObject_methods[] = {
	{"read",	(PyCFunction)forkRefObject_read,METH_VARARGS,		forkRefObject_read__doc__},
	{"write",	(PyCFunction)forkRefObject_write,METH_VARARGS,		forkRefObject_write__doc__},
	{"close",	(PyCFunction)forkRefObject_close,METH_VARARGS,		forkRefObject_close__doc__},
	{"seek",	(PyCFunction)forkRefObject_seek,METH_VARARGS,		forkRefObject_seek__doc__},
	{"tell",	(PyCFunction)forkRefObject_tell,METH_VARARGS,		forkRefObject_tell__doc__},
	{"length",	(PyCFunction)forkRefObject_length,METH_VARARGS,		forkRefObject_length__doc__},
	{"resize",	(PyCFunction)forkRefObject_resize,METH_VARARGS,		forkRefObject_resize__doc__},
	{NULL,		NULL}
};

//__________________________________________________________________________________________________

static
PyObject *forkRefObject_getattr(forkRefObject *self, char *name)
{
	return Py_FindMethod(forkRefObject_methods, (PyObject *)self, name);
}

//__________________________________________________________________________________________________

static int
forkRefObject_print(forkRefObject *self, FILE *fp, int flags)
{
	fprintf(fp, "%d", self->forkref);
	return 0;
}

//__________________________________________________________________________________________________

statichere PyTypeObject forkRefObject_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"openfile",			/*tp_name*/
	sizeof(forkRefObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)forkRefObject_dealloc,	/*tp_dealloc*/
	(printfunc)forkRefObject_print,		/*tp_print*/
	(getattrfunc)forkRefObject_getattr, /*tp_getattr*/
	0, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};



//__________________________________________________________________________________________________
//_______________________________________ ITERATOR OBJECT __________________________________________
//__________________________________________________________________________________________________

typedef struct {
	PyObject_HEAD
	PyObject	*x_attr;
	FSIterator	iterator;
} iteratorObject;

staticforward PyTypeObject iteratorObject_Type;

#define iteratorObject_Check(v)	((v)->ob_type == &iteratorObject_Type)

static
iteratorObject *newIteratorObject(PyObject *arg, FSRef *ref)
{
	OSErr err;
	iteratorObject *self = PyObject_New(iteratorObject, &iteratorObject_Type);
	if (self == NULL)
		return NULL;

	self->x_attr = NULL;
	Py_BEGIN_ALLOW_THREADS
	err = FSOpenIterator(ref, kFSIterateFlat, &self->iterator);
	Py_END_ALLOW_THREADS
	if (err != noErr) {
		Py_DECREF(self);
		return NULL;
	}

	return self;
}

//__________________________________________________________________________________________________
// iteratorObject methods
//
static
void iteratorObject_dealloc(iteratorObject *self)
{
	Py_XDECREF(self->x_attr);
	FSCloseIterator(self->iterator);
	PyObject_Del(self);
}

//__________________________________________________________________________________________________
static char iteratorObject_listdir__doc__[] =
"listdir([itemcount [, bitmap]]) -> Dict\n\n\
Returns a dictionary of items and their attributes\n\
for the given iterator and an optional bitmap describing\n\
the attributes to be fetched (see CarbonCore/Files.h for\n\
details of the bit definitions and key definitions).";

static
PyObject *iteratorObject_listdir(iteratorObject *self, PyObject *args)
{
	OSErr err;
	int count = 500;
	UInt32 actual;
	FSCatalogInfoPtr infos;
	HFSUniStr255 *unis;
	PyObject *items;
	unsigned i;
	FSCatalogInfoBitmap	bitmap = kFSCatInfoGettableInfo;

	if (!PyArg_ParseTuple(args, "|il:listdir", &count, &bitmap)) 
		return NULL;

	items = PyList_New(0);
	if (items == NULL)
		return NULL;

	infos = malloc(sizeof(FSCatalogInfo) * count);
	if (infos == NULL)
		return PyErr_NoMemory();

	unis = malloc(sizeof(HFSUniStr255) * count);
	if (unis == NULL) {
		free(infos);
		return PyErr_NoMemory();
	}

	err = FSGetCatalogInfoBulk(self->iterator, count, &actual, NULL, bitmap, infos, NULL, NULL, unis);

	if (err == noErr || err == errFSNoMoreItems) {
		for (i = 0; i < actual; i ++) {
			PyObject *item;
	
			item = dict_from_cataloginfo(bitmap, &infos[i], &unis[i]);
			if (item == NULL)
				continue;
	
			if (PyList_Append(items, item) != 0) {
				printf("ack! (PyList_Append)\n");
				continue;
			}
			Py_DECREF(item);
		}
	}

	free(infos);
	free(unis);

	Py_INCREF(items);
	return items;
}

//__________________________________________________________________________________________________

static PyMethodDef iteratorObject_methods[] = {
	{"listdir",	(PyCFunction)iteratorObject_listdir,METH_VARARGS,	iteratorObject_listdir__doc__},
	{NULL,		NULL}
};

//__________________________________________________________________________________________________

static
PyObject *iteratorObject_getattr(iteratorObject *self, char *name)
{
	if (self->x_attr != NULL) {
		PyObject *v = PyDict_GetItemString(self->x_attr, name);
		if (v != NULL) {
			Py_INCREF(v);
			return v;
		}
	}
	return Py_FindMethod(iteratorObject_methods, (PyObject *)self, name);
}

//__________________________________________________________________________________________________

static
int iteratorObject_setattr(iteratorObject *self, char *name, PyObject *v)
{
	if (self->x_attr == NULL) {
		self->x_attr = PyDict_New();
		if (self->x_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = PyDict_DelItemString(self->x_attr, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError, "delete non-existing iteratorObject attribute");
		return rv;
	}
	else
		return PyDict_SetItemString(self->x_attr, name, v);
}

//__________________________________________________________________________________________________

statichere PyTypeObject iteratorObject_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"iterator",			/*tp_name*/
	sizeof(iteratorObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)iteratorObject_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)iteratorObject_getattr, /*tp_getattr*/
	(setattrfunc)iteratorObject_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};


//__________________________________________________________________________________________________
//_________________________________________ FSREF OBJECT ___________________________________________
//__________________________________________________________________________________________________

typedef struct {
	PyObject_HEAD
	PyObject	*x_attr;
	FSRef		ref;
	int			typeknown;
	Boolean		directory;
} fsRefObject;

staticforward PyTypeObject fsRefObject_Type;

#define fsRefObject_Check(v)	((v)->ob_type == &fsRefObject_Type)

static
fsRefObject *newFSRefObject(PyObject *arg, FSRef *ref, Boolean typeknown, Boolean directory)
{
	fsRefObject *self = PyObject_New(fsRefObject, &fsRefObject_Type);
	if (self == NULL)
		return NULL;

	self->x_attr = PyDict_New();
	if (self->x_attr == NULL) {
		Py_DECREF(self);
		return NULL;
	}

	if (PyDict_SetItemString(self->x_attr, "directory", directory ? Py_True : Py_False) < 0) {
		Py_XDECREF(self->x_attr);
		Py_DECREF(self);
		return NULL;
	}

	self->ref = *ref;
	self->typeknown = typeknown;
	self->directory = directory;
	return self;
}

//__________________________________________________________________________________________________

Boolean fsRefObject_isdir(fsRefObject *self)
{
	if (self->typeknown == false) {
		self->directory = fsref_isdirectory(&self->ref);
		self->typeknown = true;
	}
	return self->directory;
}

//__________________________________________________________________________________________________
// fsRefObject methods
//
static
void fsRefObject_dealloc(fsRefObject *self)
{
	Py_XDECREF(self->x_attr);
	PyObject_Del(self);
}

//__________________________________________________________________________________________________
static char fsRefObject_opendir__doc__[] =
"opendir() -> iterator\n\n\
Return an iterator";

static
PyObject *fsRefObject_opendir(fsRefObject *self, PyObject *args)
{
	iteratorObject *obj;

	obj = newIteratorObject(args, &self->ref);
	if (obj == NULL)
	    return NULL;
	return (PyObject *) obj;
}

//__________________________________________________________________________________________________
static char fsRefObject_namedfsref__doc__[] =
"namedfsref(String) -> FSref\n\n\
Return an FSRef for the named item in the container FSRef";

static
PyObject *fsRefObject_namedfsref(fsRefObject *self, PyObject *args)
{
	OSErr err;
	fsRefObject *newObject;
	FSRef newref;
	HFSUniStr255 uni;
	PyObject *namearg, *nameobj;

	if (!PyArg_ParseTuple(args, "O|l:namedfsref", &namearg))
		return NULL;

	nameobj = obj_to_hfsunistr(namearg, &uni);
	if (nameobj == NULL)
		return NULL;
	Py_DECREF(nameobj);

	Py_BEGIN_ALLOW_THREADS
	err = FSMakeFSRefUnicode(&self->ref, uni.length, uni.unicode, kTextEncodingUnknown, &newref);
	Py_END_ALLOW_THREADS
	if (err != noErr)
		return macos_error_for_call(err, "FSMakeFSRefUnicode", NULL);

	newObject = newFSRefObject(args, &newref, false, false);

	Py_INCREF(newObject);
	return (PyObject *) newObject;
}

//__________________________________________________________________________________________________
static char fsRefObject_parent__doc__[] =
"parent() -> FSref\n\n\
Return an FSRef for the parent of this FSRef, or NULL (if this \n\
is the root.)";

static
PyObject *fsRefObject_parent(fsRefObject *self, PyObject *args)
{
	OSErr err;
	fsRefObject *newObject;
	FSRef parentref;
	FSCatalogInfo info;

	Py_BEGIN_ALLOW_THREADS
	err = FSGetCatalogInfo(&self->ref, kFSCatInfoParentDirID, &info, NULL, NULL, &parentref);
	Py_END_ALLOW_THREADS
	if (err != noErr)
		return macos_error_for_call(err, "FSGetCatalogInfo", NULL);

	if (info.parentDirID == fsRtParID)
		return NULL;

	newObject = newFSRefObject(args, &parentref, false, false);
	Py_INCREF(newObject);
	return (PyObject *) newObject;
}

//__________________________________________________________________________________________________
static char fsRefObject_openfork__doc__[] =
"openfork([resourcefork [,perms]]) -> forkRef\n\n\
Return a forkRef object for reading/writing/etc. Optionally,\n\
pass 1 for the resourcefork param to open the resource fork,\n\
and permissions to open read-write or something else:\n\
0: fsCurPerm\n\
1: fsRdPerm (default)\n\
2: fsWrPerm\n\
3: fsRdWrPerm\n\
4: fsRdWrShPerm";

static
PyObject *fsRefObject_openfork(fsRefObject *self, PyObject *args)
{
	forkRefObject *obj;
	int resfork = 0, perms = fsRdPerm;

	if (!PyArg_ParseTuple(args, "|ii:openfork", &resfork, &perms))
		return NULL;

	obj = newForkRefObject(args, &self->ref, resfork, perms);
	if (obj == NULL)
	    return NULL;
	return (PyObject *) obj;
}

//__________________________________________________________________________________________________

static
PyObject *fsRefObject_createcommon(Boolean createdir, fsRefObject *self, PyObject *args)
{
	OSErr err;
	PyObject *nameobj, *namearg, *attrs = NULL;
	FSRef newref;
	fsRefObject *newObject;
	HFSUniStr255 uni;
	FSCatalogInfoBitmap bitmap = kFSCatInfoSettableInfo;
	FSCatalogInfo info;

	if (!PyArg_ParseTuple(args, "O|Ol", &namearg, &attrs, &bitmap))
		return NULL;

	nameobj = obj_to_hfsunistr(namearg, &uni);
	if (nameobj == NULL)
		return NULL;
	Py_DECREF(nameobj);

	if (attrs) {
		if (!cataloginfo_from_dict(bitmap, &info, attrs))
			return NULL;
	} else {
		bitmap = 0L;
	}

	if (createdir) {
		Py_BEGIN_ALLOW_THREADS
		err = FSCreateDirectoryUnicode(&self->ref, uni.length, uni.unicode, bitmap, &info, &newref, NULL, NULL);
		Py_END_ALLOW_THREADS
		if (err != noErr)
			return macos_error_for_call(err, "FSCreateDirectoryUnicode", NULL);
	} else {
		Py_BEGIN_ALLOW_THREADS
		err = FSCreateFileUnicode(&self->ref, uni.length, uni.unicode, bitmap, &info, &newref, NULL);
		Py_END_ALLOW_THREADS
		if (err != noErr)
			return macos_error_for_call(err, "FSCreateFileUnicode", NULL);
	}

	newObject = newFSRefObject(args, &newref, true, createdir);
	if (newObject == NULL)
	    return NULL;

	Py_INCREF(newObject);
	return (PyObject *) newObject;
}

//__________________________________________________________________________________________________
static char fsRefObject_create__doc__[] =
"create(name [,dict]) -> FSRef\n\n\
Create a file in the specified directory with the given name\n\
and return an FSRef object of the newly created item";

static
PyObject *fsRefObject_create(fsRefObject *self, PyObject *args)
{
	return fsRefObject_createcommon(false, self, args);
}

//__________________________________________________________________________________________________
static char fsRefObject_mkdir__doc__[] =
"mkdir(name [,dict]) -> FSRef\n\n\
Create a directory in the specified directory with the given name\n\
and return an FSRef object of the newly created item";

static
PyObject *fsRefObject_mkdir(fsRefObject *self, PyObject *args)
{
	return fsRefObject_createcommon(true, self, args);
}

//__________________________________________________________________________________________________
static char fsRefObject_getcatinfo__doc__[] =
"getcatinfo([bitmap]) -> Dict\n\n\
Returns a dictionary of attributes for the given item\n\
and an optional bitmap describing the attributes to be\n\
fetched (see CarbonCore/Files.h for details of the bit\n\
definitions and key definitions).";

static
PyObject *fsRefObject_getcatinfo(fsRefObject *self, PyObject *args)
{
	PyObject *dict;
	OSErr err;
	FSCatalogInfo info = {0};
	HFSUniStr255 uni;
	FSCatalogInfoBitmap	bitmap = kFSCatInfoGettableInfo;

	if (!PyArg_ParseTuple(args, "|l:getcatinfo", &bitmap))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	err = FSGetCatalogInfo(&self->ref, bitmap, &info, &uni, NULL, NULL);
	Py_END_ALLOW_THREADS
	if (err != noErr) 
		return macos_error_for_call(err, "FSGetCatalogInfo", NULL);

	dict = dict_from_cataloginfo(bitmap, &info, &uni);
	if (dict == NULL)
		return NULL;

	Py_INCREF(dict);
	return dict;
}

//__________________________________________________________________________________________________
static char fsRefObject_setcatinfo__doc__[] =
"setcatinfo(Dict [,bitmap]) -> None\n\n\
Sets attributes for the given item.  An optional\n\
bitmap describing the attributes to be set can be\n\
given as well (see CarbonCore/Files.h for details\n\
of the bit definitions and key definitions).";

static
PyObject *fsRefObject_setcatinfo(fsRefObject *self, PyObject *args)
{
	PyObject *dict;
	OSErr err;
	FSCatalogInfo info = {0};
	FSCatalogInfoBitmap	bitmap = kFSCatInfoSettableInfo;

	if (!PyArg_ParseTuple(args, "O|l:setcatinfo", &dict, &bitmap))
		return NULL;

	if (!cataloginfo_from_dict(bitmap, &info, dict)) return NULL;

	Py_BEGIN_ALLOW_THREADS
	err = FSSetCatalogInfo(&self->ref, bitmap, &info);
	Py_END_ALLOW_THREADS
	if (err != noErr) 
		return macos_error_for_call(err, "FSSetCatalogInfo", NULL);

	Py_INCREF(Py_None);
	return Py_None;
}

//__________________________________________________________________________________________________
static char fsRefObject_delete__doc__[] =
"delete() -> None\n\n\
Delete the item";

static
PyObject *fsRefObject_delete(fsRefObject *self, PyObject *args)
{
	OSErr err;
	
	Py_BEGIN_ALLOW_THREADS
	err = FSDeleteObject(&self->ref);
	Py_END_ALLOW_THREADS
	if (err != noErr) 
		return macos_error_for_call(err, "FSDeleteObject", NULL);

	Py_INCREF(Py_None);
	return Py_None;
}

//__________________________________________________________________________________________________
static char fsRefObject_exchange__doc__[] =
"exchange(FSRef) -> None\n\n\
Exchange this file with another.";

static
PyObject *fsRefObject_exchange(fsRefObject *self, PyObject *args)
{
	OSErr err;
	fsRefObject *fsrefobj;

	if (!PyArg_ParseTuple(args, "O|:exchange", &fsrefobj))
		return NULL;

	if (fsRefObject_Check(fsrefobj) == false)
		return NULL;

	Py_BEGIN_ALLOW_THREADS	
	err = FSExchangeObjects(&self->ref, &fsrefobj->ref);
	Py_END_ALLOW_THREADS
	if (err != noErr)
		return macos_error_for_call(err, "FSExchangeObjects", NULL);

	Py_INCREF(Py_None);
	return Py_None;
}

//__________________________________________________________________________________________________
static char fsRefObject_move__doc__[] =
"move(FSRef) -> FSRef\n\n\
Move the item to the container and return an FSRef to\n\
the new item.";

static
PyObject *fsRefObject_move(fsRefObject *self, PyObject *args)
{
	OSErr err;
	fsRefObject *newObject;
	FSRef newref;
	fsRefObject *fsrefobj;

	if (!PyArg_ParseTuple(args, "O|:move", &fsrefobj))
		return NULL;

	if (fsRefObject_Check(fsrefobj) == false)
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	err = FSMoveObject(&self->ref, &fsrefobj->ref, &newref);
	Py_END_ALLOW_THREADS
	if (err != noErr)
		return macos_error_for_call(err, "FSMoveObject", NULL);

	newObject = newFSRefObject(args, &newref, false, false);
	if (newObject == NULL)
	    return NULL;

	Py_INCREF(newObject);
	return (PyObject *) newObject;
}

//__________________________________________________________________________________________________
static char fsRefObject_rename__doc__[] =
"rename(String [,encoding]) -> FSRef\n\n\
Rename the item to the new name and return an FSRef to\n\
the new item.";

static
PyObject *fsRefObject_rename(fsRefObject *self, PyObject *args)
{
	OSErr err;
	fsRefObject *newObject;
	FSRef newref;
	TextEncoding hint = kTextEncodingUnknown;
	HFSUniStr255 uni;
	PyObject *namearg, *nameobj;

	if (!PyArg_ParseTuple(args, "O|l:rename", &namearg, &hint))
		return NULL;

	nameobj = obj_to_hfsunistr(namearg, &uni);
	if (nameobj == NULL)
		return NULL;
	Py_DECREF(nameobj);

	Py_BEGIN_ALLOW_THREADS
	err = FSRenameUnicode(&self->ref, uni.length, uni.unicode, hint, &newref);
	Py_END_ALLOW_THREADS
	if (err != noErr)
		return macos_error_for_call(err, "FSRenameUnicode", NULL);

	newObject = newFSRefObject(args, &newref, false, false);
	if (newObject == NULL)
	    return NULL;

	Py_INCREF(newObject);
	return (PyObject *) newObject;
}


//__________________________________________________________________________________________________

static PyMethodDef fsRefObject_methods[] = {
	{"getcatinfo",	(PyCFunction)fsRefObject_getcatinfo,METH_VARARGS,	fsRefObject_getcatinfo__doc__},
	{"setcatinfo",	(PyCFunction)fsRefObject_setcatinfo,METH_VARARGS,	fsRefObject_setcatinfo__doc__},
	{"create",		(PyCFunction)fsRefObject_create,	METH_VARARGS,	fsRefObject_create__doc__},
	{"mkdir",		(PyCFunction)fsRefObject_mkdir,		METH_VARARGS,	fsRefObject_mkdir__doc__},
	{"namedfsref",	(PyCFunction)fsRefObject_namedfsref,METH_VARARGS,	fsRefObject_namedfsref__doc__},
	{"parent",		(PyCFunction)fsRefObject_parent,	METH_VARARGS,	fsRefObject_parent__doc__},
	{"delete",		(PyCFunction)fsRefObject_delete,	METH_VARARGS,	fsRefObject_delete__doc__},
	{"rename",		(PyCFunction)fsRefObject_rename,	METH_VARARGS,	fsRefObject_rename__doc__},
	{"move",		(PyCFunction)fsRefObject_move,		METH_VARARGS,	fsRefObject_move__doc__},
	{"exchange",	(PyCFunction)fsRefObject_exchange,	METH_VARARGS,	fsRefObject_exchange__doc__},

	{"opendir",		(PyCFunction)fsRefObject_opendir,	METH_VARARGS,	fsRefObject_opendir__doc__},
	{"openfork", 	(PyCFunction)fsRefObject_openfork,	METH_VARARGS,	fsRefObject_openfork__doc__},

	{NULL,		NULL}
};

//__________________________________________________________________________________________________

static
PyObject *fsRefObject_getattr(fsRefObject *self, char *name)
{
	if (self->x_attr != NULL) {
		PyObject *v = PyDict_GetItemString(self->x_attr, name);
		if (v != NULL) {
			Py_INCREF(v);
			return v;
		}
	}
	return Py_FindMethod(fsRefObject_methods, (PyObject *)self, name);
}

//__________________________________________________________________________________________________

static
int fsRefObject_setattr(fsRefObject *self, char *name, PyObject *v)
{
	if (self->x_attr == NULL) {
		self->x_attr = PyDict_New();
		if (self->x_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = PyDict_DelItemString(self->x_attr, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError, "delete non-existing fsRefObject attribute");
		return rv;
	}
	else
		return PyDict_SetItemString(self->x_attr, name, v);
}

//__________________________________________________________________________________________________

statichere PyTypeObject fsRefObject_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"fsref",			/*tp_name*/
	sizeof(fsRefObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)fsRefObject_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)fsRefObject_getattr, /*tp_getattr*/
	(setattrfunc)fsRefObject_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};



//__________________________________________________________________________________________________
//____________________________________ MODULE FUNCTIONS ____________________________________________
//__________________________________________________________________________________________________
static char fmgrmodule_getcatinfo__doc__[] =
"getcatinfo(path[,bitmap]) -> Dict\n\n\
Returns a dictionary of attributes for the given item\n\
and an optional bitmap describing the attributes to be\n\
fetched (see CarbonCore/Files.h for details of the bit\n\
definitions and key definitions).";

static
PyObject *fmgrmodule_getcatinfo(PyObject *self, PyObject *args)
{
	char *path;
	PyObject *dict;
	FSRef ref;
	OSErr err;
	FSCatalogInfo info = {0};
	HFSUniStr255 uni;
	FSCatalogInfoBitmap	bitmap = kFSCatInfoGettableInfo;

	if (!PyArg_ParseTuple(args, "s|l", &path, &bitmap))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	err = FSPathMakeRef((UInt8 *)path, &ref, NULL);
	Py_END_ALLOW_THREADS
	if (err != noErr) 
		return macos_error_for_call(err, "FSPathMakeRef", path);

	Py_BEGIN_ALLOW_THREADS
	err = FSGetCatalogInfo(&ref, bitmap, &info, &uni, NULL, NULL);
	Py_END_ALLOW_THREADS
	if (err != noErr) 
		return macos_error_for_call(err, "FSGetCatalogInfo", path);

	dict = dict_from_cataloginfo(bitmap, &info, &uni);
	if (dict == NULL)
		return NULL;

	Py_INCREF(dict);
	return dict;
}

//__________________________________________________________________________________________________
static char fmgrmodule_opendir__doc__[] =
"opendir(path) -> iterator\n\n\
Return an iterator for listdir.";

static
PyObject *fmgrmodule_opendir(PyObject *self, PyObject *args)
{
	char *path;
	iteratorObject *rv;
	FSRef ref;
	OSErr err;
	Boolean	isdir;

	if (!PyArg_ParseTuple(args, "s", &path))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	err = FSPathMakeRef((UInt8 *)path, &ref, &isdir);
	Py_END_ALLOW_THREADS

	if (err != noErr)
		return macos_error_for_call(err, "FSPathMakeRef", path);
	else if (isdir == false)
		return PyErr_Format(PyExc_SyntaxError, "requires a directory");

	rv = newIteratorObject(args, &ref);
	if (rv == NULL)
	    return NULL;
	return (PyObject *) rv;
}

//__________________________________________________________________________________________________
static char fmgrmodule_fsref__doc__[] =
"fsref(path) -> FSRef\n\n\
Return an FSRef object.";

static
PyObject *fmgrmodule_fsref(PyObject *self, PyObject *args)
{
	char *path;
	fsRefObject *obj;
	FSRef ref;
	OSErr err;
	Boolean isdir;

	if (!PyArg_ParseTuple(args, "s", &path))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	err = FSPathMakeRef((UInt8 *)path, &ref, &isdir);
	Py_END_ALLOW_THREADS

	if (err != noErr)
		return macos_error_for_call(err, "FSPathMakeRef", path);

	obj = newFSRefObject(args, &ref, true, isdir);
	if (obj == NULL)
	    return NULL;
	return (PyObject *) obj;
}

//__________________________________________________________________________________________________
static char fmgrmodule_openfork__doc__[] =
"openfork(path[,resourcefork[,perms]]) -> forkRef\n\n\
Return a forkRef object for reading/writing/etc. Optionally,\n\
pass 1 for the resourcefork param to open the resource fork,\n\
and permissions to open read-write or something else:\n\
0: fsCurPerm\n\
1: fsRdPerm\n\
2: fsWrPerm\n\
3: fsRdWrPerm\n\
4: fsRdWrShPerm";

static
PyObject *fmgrmodule_openfork(PyObject *self, PyObject *args)
{
	char *path;
	forkRefObject *rv;
	FSRef ref;
	OSErr err;
	Boolean	isdir;
	int resfork = 0, perms = fsRdPerm;

	if (!PyArg_ParseTuple(args, "s|ii", &path, &resfork, &perms))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	err = FSPathMakeRef((UInt8 *)path, &ref, &isdir);
	Py_END_ALLOW_THREADS

	if (err != noErr) {
		return macos_error_for_call(err, "FSPathMakeRef", path);
	} else if (isdir == true) {
		return PyErr_Format(PyExc_SyntaxError, "requires a file");
	}

	rv = newForkRefObject(args, &ref, resfork, perms);
	if (rv == NULL)
	    return NULL;
	return (PyObject *) rv;
}

//__________________________________________________________________________________________________
// List of functions defined in the module
//
static PyMethodDef fmgrmodule_methods[] = {
	{"getcatinfo",	fmgrmodule_getcatinfo,	METH_VARARGS,	fmgrmodule_getcatinfo__doc__},
	{"opendir",	fmgrmodule_opendir,	METH_VARARGS,			fmgrmodule_opendir__doc__},
	{"openfork", fmgrmodule_openfork,	METH_VARARGS,		fmgrmodule_openfork__doc__},
	{"fsref", fmgrmodule_fsref,	METH_VARARGS,				fmgrmodule_fsref__doc__},
	{NULL,		NULL}
};

//__________________________________________________________________________________________________
// Initialization function for the module (*must* be called inithfsplus)
//
DL_EXPORT(void)
inithfsplus(void)
{
	PyObject *m, *d;

	/* Initialize the type of the new type object here; doing it here
	 * is required for portability to Windows without requiring C++. */
	iteratorObject_Type.ob_type = &PyType_Type;
	forkRefObject_Type.ob_type = &PyType_Type;
	fsRefObject_Type.ob_type = &PyType_Type;

	/* Create the module and add the functions */
	m = Py_InitModule("fmgr", fmgrmodule_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	insert_int(d, "fsCurPerm", fsCurPerm);
	insert_int(d, "fsRdPerm", fsRdPerm);
	insert_int(d, "fsWrPerm", fsWrPerm);
	insert_int(d, "fsRdWrPerm", fsRdWrPerm);
	insert_int(d, "fsRdWrShPerm", fsRdWrShPerm);
	insert_int(d, "fsAtMark", fsAtMark);
	insert_int(d, "fsFromStart", fsFromStart);
	insert_int(d, "fsFromLEOF", fsFromLEOF);
	insert_int(d, "fsFromMark", fsFromMark);
	insert_int(d, "noCacheMask", noCacheMask);
	ErrorObject = PyErr_NewException("fmgr.error", NULL, NULL);
	PyDict_SetItemString(d, "error", ErrorObject);
}

//__________________________________________________________________________________________________
//_________________________________________ UTILITIES ______________________________________________
//__________________________________________________________________________________________________

static const char *_kFSCatInfoPrintableName = "pname";
static const char *_kFSCatInfoUnicodeName = "uname";

static const char *_kFSCatInfoVolume = "volume";
static const char *_kFSCatInfoNodeID = "nodeid";
static const char *_kFSCatInfoParentDirID = "parid";
static const char *_kFSCatInfoTextEncoding = "encoding";
static const char *_kFSCatInfoNodeFlags = "flags";

static const char *_kFSCatInfoDataLogical = "datalogicalsize";
static const char *_kFSCatInfoRsrcLogical = "rsrclogicalsize";
static const char *_kFSCatInfoDataPhysical = "dataphysicalsize";
static const char *_kFSCatInfoRsrcPhysical = "rsrcphysicalsize";

static const char *_kFSCatInfoUserID = "uid";
static const char *_kFSCatInfoGroupID = "gid";
static const char *_kFSCatInfoUserAccess = "useraccess";
static const char *_kFSCatInfoMode = "mode";

static const char *_kFSCatInfoFinderInfo = "finfo";
static const char *_kFSCatInfoFinderXInfo = "fxinfo";

static const char *_kFSCatInfoCreateDate = "crdate";
static const char *_kFSCatInfoContentMod = "mddate";
static const char *_kFSCatInfoAttrMod = "amdate";
static const char *_kFSCatInfoAccessDate = "acdate";
static const char *_kFSCatInfoBackupDate = "bkdate";

//__________________________________________________________________________________________________

static
PyObject *macos_error_for_call(OSErr err, const char *name, const char *item)
{
	PyObject *v;
	char buffer[1024];
	
	if (item)
		sprintf(buffer, "mac error calling %s on %s", name, item);
	else
		sprintf(buffer, "mac error calling %s", name);

	v = Py_BuildValue("(is)", err, buffer);
	if (v != NULL) {
		PyErr_SetObject(PyExc_OSError, v);
		Py_DECREF(v);
	}
	return NULL;
}
//__________________________________________________________________________________________________

static
int insert_slong(FSCatalogInfoBitmap bitmap, UInt32 bit, PyObject *d, const char *symbol, long value)
{
	if (bitmap & bit) {
		PyObject* v = PyLong_FromLong(value);
		if (!v || PyDict_SetItemString(d, (char *) symbol, v) < 0)
			return -1;
		Py_DECREF(v);
	}
	return 0;
}

//__________________________________________________________________________________________________

static
int insert_long(FSCatalogInfoBitmap bitmap, UInt32 bit, PyObject *d, const char *symbol, UInt32 value)
{
	if (bitmap & bit) {
		PyObject* v = PyLong_FromUnsignedLong(value);
		if (!v || PyDict_SetItemString(d, (char *) symbol, v) < 0)
			return -1;
		Py_DECREF(v);
	}
	return 0;
}

//__________________________________________________________________________________________________

static
int insert_int(PyObject *d, const char *symbol, int value)
{
	PyObject* v = PyInt_FromLong(value);
	if (!v || PyDict_SetItemString(d, (char *) symbol, v) < 0)
		return -1;

	Py_DECREF(v);
	return 0;
}

//__________________________________________________________________________________________________

static
int insert_longlong(FSCatalogInfoBitmap bitmap, UInt32 bit, PyObject *d, const char *symbol, UInt64 value)
{
	if (bitmap & bit) {
		PyObject* v = PyLong_FromLongLong(value);
		if (!v || PyDict_SetItemString(d, (char *) symbol, v) < 0)
			return -1;
		Py_DECREF(v);
	}
	return 0;
}

//__________________________________________________________________________________________________

static
int insert_utcdatetime(FSCatalogInfoBitmap bitmap, UInt32 bit, PyObject *d, const char *symbol, const UTCDateTime *utc)
{
	if (bitmap & bit) {
		PyObject* tuple = Py_BuildValue("ili", utc->highSeconds, utc->lowSeconds, utc->fraction);
		if (!tuple || PyDict_SetItemString(d, (char *) symbol, tuple) < 0)
			return -1;
		Py_DECREF(tuple);
	}
	return 0;
}

//__________________________________________________________________________________________________

static
int fetch_long(FSCatalogInfoBitmap bitmap, UInt32 bit, const PyObject *d, const char *symbol, UInt32 *value)
{
	if (bitmap & bit) {
		PyObject* v = PyDict_GetItemString((PyObject *) d, (char *) symbol);
		if (v == NULL)
			return -1;

		*value = PyLong_AsUnsignedLong(v);
	}
	return 0;
}

//__________________________________________________________________________________________________

static
int fetch_utcdatetime(FSCatalogInfoBitmap bitmap, UInt32 bit, const PyObject *d, const char *symbol, UTCDateTime *utc)
{
	if (bitmap & bit) {
		PyObject* tuple = PyDict_GetItemString((PyObject *) d, (char *) symbol);
		if (tuple == NULL)
			return -1;

		if (!PyArg_ParseTuple(tuple, "ili", &utc->highSeconds, &utc->lowSeconds, &utc->fraction))
			return -1;
	}

	return 0;
}

//__________________________________________________________________________________________________

static
void printableUniStr(const HFSUniStr255 *uni, char *buffer)
{
	int		i;
	char	localbuf[32];

	buffer[0] = 0;
	for (i = 0; i < uni->length; i ++) {
		UniChar		uch = uni->unicode[i];

		if ((uch & 0x7f) == uch) {
			sprintf(localbuf, "%c", uch);
		} else {
			sprintf(localbuf, "\\u%04x", uch);
		}
		strcat(buffer, localbuf);
	}
}

//__________________________________________________________________________________________________

static
int cataloginfo_from_dict(FSCatalogInfoBitmap bitmap, FSCatalogInfo *info, const PyObject *dict)
{
	UInt32 storage;
#if UNIVERSAL_INTERFACES_VERSION > 0x0332
	FSPermissionInfo *permissions;
#endif
	
	// Dates
	if (fetch_utcdatetime(bitmap, kFSCatInfoCreateDate, dict, _kFSCatInfoCreateDate, &info->createDate)) return NULL;
	if (fetch_utcdatetime(bitmap, kFSCatInfoContentMod, dict, _kFSCatInfoContentMod, &info->contentModDate)) return NULL;
	if (fetch_utcdatetime(bitmap, kFSCatInfoAttrMod, dict, _kFSCatInfoAttrMod, &info->attributeModDate)) return NULL;
	if (fetch_utcdatetime(bitmap, kFSCatInfoAccessDate, dict, _kFSCatInfoAccessDate, &info->accessDate)) return NULL;
	if (fetch_utcdatetime(bitmap, kFSCatInfoBackupDate, dict, _kFSCatInfoBackupDate, &info->backupDate)) return NULL;

#if UNIVERSAL_INTERFACES_VERSION > 0x0332
	// Permissions
	permissions = (FSPermissionInfo *) info->permissions;
	if (fetch_long(bitmap, kFSCatInfoPermissions, dict, _kFSCatInfoUserID, &permissions->userID)) return NULL;
	if (fetch_long(bitmap, kFSCatInfoPermissions, dict, _kFSCatInfoGroupID, &permissions->groupID)) return NULL;
	if (fetch_long(bitmap, kFSCatInfoPermissions, dict, _kFSCatInfoMode, &storage)) return NULL;
	permissions->mode = (UInt16) storage;
	if (fetch_long(bitmap, kFSCatInfoPermissions, dict, _kFSCatInfoUserAccess, &storage)) return NULL;
	permissions->userAccess = (UInt8) storage;
#endif
	// IDs
	if (fetch_long(bitmap, kFSCatInfoTextEncoding, dict, _kFSCatInfoTextEncoding, &info->textEncodingHint)) return NULL;
	if (fetch_long(bitmap, kFSCatInfoNodeFlags, dict, _kFSCatInfoNodeFlags, &storage)) return NULL;
	info->nodeFlags = (UInt16) storage;

	// FinderInfo
	if (bitmap & kFSCatInfoFinderInfo) {
		PyObject *obj = PyDict_GetItemString((PyObject *) dict, (char *) _kFSCatInfoFinderInfo);
		if (obj == NULL)
			return NULL;
		BlockMoveData(PyString_AsString(obj), info->finderInfo, sizeof(FInfo));
	}

	if (bitmap & kFSCatInfoFinderXInfo) {
		PyObject *obj = PyDict_GetItemString((PyObject *) dict, (char *) _kFSCatInfoFinderXInfo);
		if (obj == NULL)
			return NULL;
		BlockMoveData(PyString_AsString(obj), info->extFinderInfo, sizeof(FXInfo));
	}

	return 1;
}

//__________________________________________________________________________________________________

static
PyObject *dict_from_cataloginfo(FSCatalogInfoBitmap bitmap, const FSCatalogInfo *info, HFSUniStr255 *uni)
{
	PyObject *dict;
	PyObject *id;
#if UNIVERSAL_INTERFACES_VERSION > 0x0332
	FSPermissionInfo *permissions;
#endif
	char buffer[1024];

	dict = PyDict_New();
	if (dict == NULL)
		return NULL;

	// Name
	if (uni) {
		id = PyUnicode_FromUnicode(uni->unicode, uni->length);	
		PyDict_SetItemString(dict, (char*) _kFSCatInfoUnicodeName, id); Py_XDECREF(id);
		
		printableUniStr(uni, buffer);
		id = PyString_FromString(buffer);
		PyDict_SetItemString(dict, (char*) _kFSCatInfoPrintableName, id); Py_XDECREF(id);
	}

	// IDs
	if (insert_slong(bitmap, kFSCatInfoVolume, dict, _kFSCatInfoVolume, info->volume)) return NULL;
	if (insert_long(bitmap, kFSCatInfoNodeID, dict, _kFSCatInfoNodeID, info->nodeID)) return NULL;
	if (insert_long(bitmap, kFSCatInfoParentDirID, dict, _kFSCatInfoParentDirID, info->parentDirID)) return NULL;
	if (insert_long(bitmap, kFSCatInfoTextEncoding, dict, _kFSCatInfoTextEncoding, info->textEncodingHint)) return NULL;
	if (insert_long(bitmap, kFSCatInfoNodeFlags, dict, _kFSCatInfoNodeFlags, info->nodeFlags)) return NULL;

	// Sizes
	if (insert_longlong(bitmap, kFSCatInfoDataSizes, dict, _kFSCatInfoDataLogical, info->dataLogicalSize)) return NULL;
	if (insert_longlong(bitmap, kFSCatInfoDataSizes, dict, _kFSCatInfoDataPhysical, info->dataPhysicalSize)) return NULL;
	if (insert_longlong(bitmap, kFSCatInfoRsrcSizes, dict, _kFSCatInfoRsrcLogical, info->rsrcLogicalSize)) return NULL;
	if (insert_longlong(bitmap, kFSCatInfoRsrcSizes, dict, _kFSCatInfoRsrcPhysical, info->rsrcPhysicalSize)) return NULL;

#if UNIVERSAL_INTERFACES_VERSION > 0x0332
	// Permissions
	permissions = (FSPermissionInfo *) info->permissions;
	if (insert_long(bitmap, kFSCatInfoPermissions, dict, _kFSCatInfoUserID, permissions->userID)) return NULL;
	if (insert_long(bitmap, kFSCatInfoPermissions, dict, _kFSCatInfoGroupID, permissions->groupID)) return NULL;
	if (insert_long(bitmap, kFSCatInfoPermissions, dict, _kFSCatInfoUserAccess, permissions->userAccess)) return NULL;
	if (insert_long(bitmap, kFSCatInfoPermissions, dict, _kFSCatInfoMode, permissions->mode)) return NULL;
#endif

	// Dates
	if (insert_utcdatetime(bitmap, kFSCatInfoCreateDate, dict, _kFSCatInfoCreateDate, &info->createDate)) return NULL;
	if (insert_utcdatetime(bitmap, kFSCatInfoContentMod, dict, _kFSCatInfoContentMod, &info->contentModDate)) return NULL;
	if (insert_utcdatetime(bitmap, kFSCatInfoAttrMod, dict, _kFSCatInfoAttrMod, &info->attributeModDate)) return NULL;
	if (insert_utcdatetime(bitmap, kFSCatInfoAccessDate, dict, _kFSCatInfoAccessDate, &info->accessDate)) return NULL;
	if (insert_utcdatetime(bitmap, kFSCatInfoBackupDate, dict, _kFSCatInfoBackupDate, &info->backupDate)) return NULL;
	
	// FinderInfo
	if (bitmap & kFSCatInfoFinderInfo) {
		id = Py_BuildValue("s#", (char *) info->finderInfo, sizeof(FInfo));
		PyDict_SetItemString(dict, (char*) _kFSCatInfoFinderInfo, id); Py_XDECREF(id);
	}
	if (bitmap & kFSCatInfoFinderXInfo) {
		id = Py_BuildValue("s#", (char *) info->extFinderInfo, sizeof(FXInfo));
		PyDict_SetItemString(dict, (char*) _kFSCatInfoFinderXInfo, id); Py_XDECREF(id);
	}

	return dict;
}

//__________________________________________________________________________________________________

static
Boolean fsref_isdirectory(const FSRef *ref)
{
	Boolean	isdir = false;
	OSErr	err;
	FSCatalogInfo info;

	err = FSGetCatalogInfo(ref, kFSCatInfoNodeFlags, &info, NULL, NULL, NULL);

	if (err == noErr)
		isdir = (info.nodeFlags & kioFlAttribDirMask);

	return isdir;
}

//__________________________________________________________________________________________________

static
PyObject *obj_to_hfsunistr(PyObject *in, HFSUniStr255 *uni)
{
	PyObject *out;

	out = PyUnicode_FromObject(in);
	if (out == NULL)
		return NULL;

	BlockMoveData(PyUnicode_AS_UNICODE(out), uni->unicode, PyUnicode_GET_DATA_SIZE(out));
	uni->length = PyUnicode_GET_SIZE(out);

	return out;
}


