/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
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

/* Objective-C interface for NeXTStep */
/* Tested with NeXTStep 3.3 on Intel and Sparc architectures */

/* Original author: Jon M. Kutemeier */
/* Revamped and maintained by: Guido van Rossum */

/* XXX To do:
   - bug??? x.send('name', []) gives weird error
   - rename functions from objc_* to ObjC_*
   - change send(sel, [a, b, c]) to send(self, a, b, c)
   - call back to Python from Objective-C
 */

/* Python header file */
#include "Python.h"

/* NeXT headers */
#include <sys/param.h>
#include <mach-o/rld.h>
#include <objc/objc.h>
#include <objc/objc-runtime.h>
#import <remote/NXProxy.h>

/* Distinguish between ObjC classes and instances */
typedef enum {
    OBJC_CLASS,
    OBJC_INSTANCE,
} ObjC_Typecode;

/* Exception raised for ObjC specific errors */
static PyObject *ObjC_Error;

/* Python wrapper about ObjC id (instance or class) */
typedef struct {
    PyObject_HEAD
    id             obj;
    ObjC_Typecode  type;
    int            owned;
} ObjCObject;

/* Corresponding Python type object */
staticforward PyTypeObject ObjC_Type;

/* Corresponding Python type check macro */
#define ObjC_Check(o) ((o)->ob_type == &ObjC_Type)

/* Create a new ObjCObject */
static ObjCObject *
newObjCObject(obj, type, owned)
    id obj;
    ObjC_Typecode type;
    int owned;
{
    ObjCObject *self;

    self = PyObject_NEW(ObjCObject, &ObjC_Type);
    if (self == NULL)
	return NULL;

    self->obj = obj;
    self->type = type;
    self->owned = owned;

    return self;
}

static void
objc_sendfree(self)
    ObjCObject *self;
{
    if (self->obj)
	self->obj = (id)objc_msgSend(self->obj, SELUID("free"));
}

/* Deallocate an ObjCObject */
static void
objc_dealloc(self)
    ObjCObject *self;
{
    if (self->owned)
	objc_sendfree(self);
    PyMem_DEL(self);
}

/* Return a string representation of an ObjCObject */
static PyObject *
objc_repr(self)
    ObjCObject *self;
{
    char buffer[512];
    char *p = buffer;
    if (self->obj == nil)
	p = "<Objective-C nil>";
    else {
	char *t;
	switch (self->type) {
	case OBJC_CLASS: t = "class"; break;
	case OBJC_INSTANCE: t = "instance"; break;
	default: t = "???"; break;
	}
	sprintf(buffer, "<Objective-C %s %s at %lx>",
		NAMEOF(self->obj), t, (long)(self->obj));
    }
    return PyString_FromString(p);
}

/*** ObjCObject methods ***/

/* Call an object's free method */
static PyObject *
objc_free(self, args)
    ObjCObject *self;
    PyObject *args;
{
    if (!PyArg_ParseTuple(args, ""))
	return NULL;
    objc_sendfree(self);
}

/* Send a message to an ObjCObject.
   The Python call looks like e.g. obj.send('moveTo::', [arg1, arg2])
   which translates into Objective-C as [obj moveTo: arg1 : arg2] */
static PyObject *
objc_send(self, args)
    ObjCObject *self;
    PyObject *args;
{
    char *methodname;
    char *margBuff = NULL;
    PyObject *retobject = NULL;
    PyObject *arglist;
    id receiver, obj;
    char *type;
    SEL sel;
    Method meth;
    unsigned int margCount, margSize;
    int offset, i;

    if (!PyArg_ParseTuple(args, "sO!", &methodname, &PyList_Type, &arglist))
	return NULL;

    /* Get the method descriptor from the object */

    receiver = self->obj;
    sel = SELUID(methodname);

    switch(self->type) {
    case OBJC_CLASS:
	meth = class_getClassMethod(receiver->isa, sel);
	break;
    case OBJC_INSTANCE:
	meth = class_getInstanceMethod(receiver->isa, sel);
	break;
    default:
	PyErr_SetString(ObjC_Error,
			"receiver's type is neither instance not class!?!?");
	return NULL;
    }

    if (!meth) {
	PyErr_SetString(ObjC_Error, "receiver has no method by that name");
	return NULL;
    }

    /* Fill in the argument list, type-checking the arguments */

    margCount = method_getNumberOfArguments(meth);

    if (PyList_Size(arglist) + 2 != margCount) {
	PyErr_SetString(ObjC_Error,
			"wrong number of arguments for this method");
	    return NULL;
    }

    margSize = method_getSizeOfArguments(meth);
    margBuff = PyMem_NEW(char, margSize+1);
    if (margBuff == NULL)
	return PyErr_NoMemory();

    method_getArgumentInfo(meth, 0, &type, &offset);
    marg_setValue(margBuff, offset, id, receiver);

    method_getArgumentInfo(meth, 1, &type, &offset);
    marg_setValue(margBuff, offset, SEL, sel);

    for (i = 2; i < margCount; i++) {
	PyObject *argument;
	method_getArgumentInfo(meth, i, &type, &offset);

	argument = PyList_GetItem(arglist, i-2);

	/* scan past protocol-type modifiers */
	while (strchr("rnNoOV", *type) != 0)
	    type++;

	/* common type checks */
	switch(*type) {

	    /* XXX The errors here should point out which argument */

	case 'c':
	case '*':
	case 'C':
	    if (!PyString_Check(argument)) {
		PyErr_SetString(ObjC_Error, "string argument expected");
		goto error;
	    }
	    break;

	case 'i':
	case 's':
	case 'I':
	case 'S':
	case 'l':
	case 'L':
	case '^':
	    if (!PyInt_Check(argument)) {
		PyErr_SetString(ObjC_Error, "integer argument expected");
		goto error;
	    }
	    break;

	case 'f':
	case 'd':
	    if (!PyFloat_Check(argument)) {
		PyErr_SetString(ObjC_Error, "float argument expected");
		goto error;
	    }
	    break;

	}

	/* convert and store the argument */
	switch (*type) {

	case 'c': /* char */
	    marg_setValue(margBuff, offset, char,
			  PyString_AsString(argument)[0]);
	    break;

	case 'C': /* unsigned char */
	    marg_setValue(margBuff, offset, unsigned char,
			  PyString_AsString(argument)[0]);
	    break;

	case '*': /* string */
	    marg_setValue(margBuff, offset, char *,
			  PyString_AsString(argument));
	    break;

	case 'i': /* int */
	    marg_setValue(margBuff, offset, int,
			  PyInt_AsLong(argument));
	    break;

	case 'I': /* unsigned int */
	    marg_setValue(margBuff, offset, unsigned int,
			  PyInt_AsLong(argument));
	    break;

	case 's': /* short */
	    marg_setValue(margBuff, offset, short,
			  PyInt_AsLong(argument));
	    break;

	case 'S': /* unsigned short */
	    marg_setValue(margBuff, offset, unsigned short,
			  PyInt_AsLong(argument));
	    break;

	case 'l': /* long */
	    marg_setValue(margBuff, offset, long,
			  PyInt_AsLong(argument));
	    break;

	case 'L': /* unsigned long */
	    marg_setValue(margBuff, offset, unsigned long,
			  PyInt_AsLong(argument));
	    break;

	case 'f': /* float */
	    marg_setValue(margBuff, offset, float,
			  (float)PyFloat_AsDouble(argument));
	    break;

	case 'd': /* double */
	    marg_setValue(margBuff, offset, double,
			  PyFloat_AsDouble(argument));
	    break;

	case '@': /* id (or None) */
	    if (ObjC_Check(argument))
		marg_setValue(margBuff, offset, id,
			      ((ObjCObject *)(argument))->obj);
	    else if (argument == Py_None)
		marg_setValue(margBuff, offset, id, nil);
	    else {
		PyErr_SetString(ObjC_Error, "id or None argument expected");
		goto error;
	    }
	    break;

	case '^': /* void * (use int) */
	    marg_setValue(margBuff, offset, void *,
			  (void *)PyInt_AsLong(argument));
	    break;

	case ':': /* SEL (use string or int) */
	    if (PyInt_Check(argument))
		marg_setValue(margBuff, offset, SEL,
			      (SEL)PyInt_AsLong(argument));
	    else if (PyString_Check(argument))
		marg_setValue(margBuff, offset, SEL,
			      SELUID(PyString_AsString(argument)));
	    else {
		PyErr_SetString(ObjC_Error,
				"selector string or int argument expected");
		goto error;
	    }
	    break;

	case '#': /* Class (may also use int) */
	    if (ObjC_Check(argument) &&
		((ObjCObject *)argument)->type == OBJC_INSTANCE)
		marg_setValue(margBuff, offset, Class *,
			      (Class *)((ObjCObject *)argument)->obj);
	    else if (PyInt_Check(argument))
		marg_setValue(margBuff, offset, Class *,
			      (Class *)PyInt_AsLong(argument));
	    else {
		PyErr_SetString(ObjC_Error,
				"ObjC class object required");
		goto error;
	    }
	    break;

	default:
	    PyErr_SetString(ObjC_Error, "unknown argument type");
	    goto error;

	}
    }

    /* Call the method and set the return value */

    type = meth->method_types;

    while (strchr("rnNoOV", *type))
	type++;

    switch(*type) {

/* Cast objc_msgSendv to a function returning the right thing */
#define MS_CAST(type) ((type (*)())objc_msgSendv)

    case 'c':
    case '*':
    case 'C':
	retobject = (PyObject *)PyString_FromString(
	    MS_CAST(char *)(receiver, sel, margSize, margBuff));
	break;

    case 'i':
    case 's':
    case 'I':
    case 'S':
	retobject = (PyObject *)PyInt_FromLong(
	    MS_CAST(int)(receiver, sel, margSize, margBuff));
	break;

    case 'l':
    case 'L':
    case '^':
	retobject = (PyObject *)PyInt_FromLong(
	    MS_CAST(long)(receiver, sel, margSize, margBuff));
	break;

    case 'f':
	retobject = (PyObject *)PyFloat_FromDouble(
	    MS_CAST(float)(receiver, sel, margSize, margBuff));
	break;

    case 'd':
	retobject = (PyObject *)PyFloat_FromDouble(
	    MS_CAST(double)(receiver, sel, margSize, margBuff));
	break;

    case '@':
	obj = MS_CAST(id)(receiver, sel, margSize, margBuff);
	if (obj == nil) {
	    retobject = Py_None;
	    Py_INCREF(retobject);
	}
	else if (obj != receiver)
	    retobject = (PyObject *)newObjCObject(obj, OBJC_INSTANCE, 0);
	else {
	    retobject = (PyObject *)self;
	    Py_INCREF(retobject);
	}
	break;

    case ':':
	retobject = (PyObject *)PyInt_FromLong(
	    (long)MS_CAST(SEL)(receiver, sel, margSize, margBuff));
	break;

    case '#':
	retobject = (PyObject *)PyInt_FromLong(
	    (long)MS_CAST(Class *)(receiver, sel, margSize, margBuff));
	break;

#undef MS_CAST

    }

  error:
    PyMem_XDEL(margBuff);
    return retobject;
}

/* List of methods for ObjCObject */
static PyMethodDef objc_methods[] = {
    {"send",	(PyCFunction)objc_send, 1},
    {"free",	(PyCFunction)objc_free, 1},
    {NULL,	NULL}		/* sentinel */
};

/* Get an attribute of an ObjCObject */
static PyObject *
objc_getattr(self, name)
    ObjCObject *self;
    char *name;
{
    PyObject *method;

    /* Try a function method */
    method = Py_FindMethod(objc_methods, (PyObject *)self, name);
    if (method != NULL)
	return method;
    PyErr_Clear();

    /* Try an instance variable */
    if (strcmp(name, "obj") == 0)
	return PyInt_FromLong((long)self->obj);
    if (strcmp(name, "type") == 0)
	return PyInt_FromLong((long)self->type);
    if (strcmp(name, "owned") == 0)
	return PyInt_FromLong((long)self->owned);
    if (strcmp(name, "name") == 0)
	return PyString_FromString(NAMEOF(self->obj));
    if (strcmp(name, "__members__") == 0)
	return Py_BuildValue("[sss]", "name", "obj", "owned", "type");

    PyErr_SetString(PyExc_AttributeError, name);
    return NULL;
}

/* The type object */
static PyTypeObject ObjC_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                                  /*ob_size*/
    "objc",                             /*tp_name*/
    sizeof(ObjCObject),                 /*tp_basicsize*/
    0,                                  /*tp_itemsize*/
    /* methods */
    (destructor)objc_dealloc,           /*tp_dealloc*/
    0,                         		/*tp_print*/
    (getattrfunc)objc_getattr,          /*tp_getattr*/
    0,                       		/*tp_setattr*/
    0,                         		/*tp_compare*/
    (reprfunc)objc_repr,		/*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    0,     		                /*tp_hash*/
    0,					/*tp_call*/
    0,					/*tp_str*/
    0, 0, 0, 0,				/*xxx1-4*/
    "Objective-C id wrapper",		/*tp_doc*/
};



/*** Top-level functions ***/

/* Max #files passed to loadobjectfile() */
#define MAXRLD 128

/* Load a list of object files */
static PyObject *
objc_loadobjectfiles(self, args)
PyObject *self;   /* Not used */
PyObject *args;
{
    NXStream *errorStream;
    struct mach_header *new_header;
    const char *filenames[MAXRLD+1];
    long ret;
    char *streamBuf;
    PyObject *filelist, *file;
    int listsize, len, maxLen, i;

    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &filelist))
	return NULL;

    listsize = PyList_Size(filelist);

    if (listsize > MAXRLD) {
	PyErr_SetString(ObjC_Error, "more than 128 files in list");
	return NULL;
    }

    errorStream = NXOpenMemory(NULL, 0, NX_WRITEONLY);

    for (i = 0; i < listsize; i++) {
	file = PyList_GetItem(filelist, i);

	if (!PyString_Check(file))
	    {
		PyErr_SetString(ObjC_Error,
				"all list items must be strings");
		return NULL;
	    }

	filenames[i] = PyString_AsString(file);
    }

    filenames[listsize] = NULL;

    ret = objc_loadModules(filenames, errorStream, NULL, &new_header, NULL);

    /* extract the error messages for the exception */

    if(ret) {
	NXPutc(errorStream, (char)0);

	NXGetMemoryBuffer(errorStream, &streamBuf, &len, &maxLen);
	PyErr_SetString(ObjC_Error, streamBuf);
    }

    NXCloseMemory(errorStream, NX_FREEBUFFER);

    if(ret)
	return NULL;

    Py_XINCREF(Py_None);
    return Py_None;
}

static PyObject *
objc_lookupclass(self, args)
PyObject *self;   /* Not used */
PyObject *args;
{
    char *classname;
    id    class;

    if (!PyArg_ParseTuple(args, "s", &classname))
	return NULL;

    if (!(class = objc_lookUpClass(classname)))
	{
	    PyErr_SetString(ObjC_Error, "unknown ObjC class");
	    return NULL;
	}

    return (PyObject *)newObjCObject(class, OBJC_CLASS, 0);
}

/* List all classes */
static PyObject *
objc_listclasses(self, args)
    ObjCObject *self;
    PyObject *args;
{
    NXHashTable *class_hash = objc_getClasses();
    NXHashState state = NXInitHashState(class_hash);
    Class classid;
    PyObject *list;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    list = PyList_New(0);
    if (list == NULL)
	return NULL;
  
    while (NXNextHashState(class_hash, &state, (void**)&classid)) {
	ObjCObject *item = newObjCObject(classid, OBJC_CLASS, 0);
	if (item == NULL || PyList_Append(list, (PyObject *)item) < 0) {
	    Py_XDECREF(item);
	    Py_DECREF(list);
	    return NULL;
	}
	Py_INCREF(item);
    }

    return list;
}

/* List of top-level functions */
static PyMethodDef objc_class_methods[] = {
    {"loadobjectfiles",	objc_loadobjectfiles, 1},
    {"lookupclass", 	objc_lookupclass, 1},
    {"listclasses",	objc_listclasses, 1},
    {NULL, 		NULL}		/* sentinel */
};

/* Initialize for the module */
void
initobjc()
{
    PyObject *m, *d;

    m = Py_InitModule("objc", objc_class_methods);
    d = PyModule_GetDict(m);

    ObjC_Error = PyString_FromString("objc.error");
    PyDict_SetItemString(d, "error", ObjC_Error);

    if (PyErr_Occurred())
	Py_FatalError("can't initialize module objc");

#ifdef WITH_THREAD
    objc_setMultithreaded(1);
#endif
}
