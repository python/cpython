/* GDBM module, hacked from the still-breathing corpse of the
   DBM module by anthony.baxter@aaii.oz.au. Original copyright
   follows:
*/
/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/
/*****************************************************************
  Modification History:

  Added support for 'gdbm_sync' method.  Roger E. Masse 3/25/97

  *****************************************************************/
/* DBM module using dictionary interface */


#include "Python.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gdbm.h"

typedef struct {
	PyObject_HEAD
	int di_size;	/* -1 means recompute */
	GDBM_FILE di_dbm;
} dbmobject;

staticforward PyTypeObject Dbmtype;

#define is_dbmobject(v) ((v)->ob_type == &Dbmtype)
#define check_dbmobject_open(v) if ((v)->di_dbm == NULL) \
               { PyErr_SetString(DbmError, "GDBM object has already been closed"); \
                 return NULL; }



static PyObject *DbmError;

static PyObject *
newdbmobject(file, flags, mode)
	char *file;
int flags;
int mode;
{
        dbmobject *dp;

	dp = PyObject_NEW(dbmobject, &Dbmtype);
	if (dp == NULL)
		return NULL;
	dp->di_size = -1;
	errno = 0;
	if ( (dp->di_dbm = gdbm_open(file, 0, flags, mode, NULL)) == 0 ) {
		if (errno != 0)
			PyErr_SetFromErrno(DbmError);
		else
			PyErr_SetString(DbmError,
					(char *) gdbm_strerror(gdbm_errno));
		Py_DECREF(dp);
		return NULL;
	}
	return (PyObject *)dp;
}

/* Methods */

static void
dbm_dealloc(dp)
	register dbmobject *dp;
{
        if ( dp->di_dbm )
		gdbm_close(dp->di_dbm);
	PyMem_DEL(dp);
}

static int
dbm_length(dp)
	dbmobject *dp;
{
        if (dp->di_dbm == NULL) {
                 PyErr_SetString(DbmError, "GDBM object has already been closed"); 
                 return -1; 
        }
        if ( dp->di_size < 0 ) {
		datum key,okey;
		int size;
		okey.dsize=0;

		size = 0;
		for ( key=gdbm_firstkey(dp->di_dbm); key.dptr;
		      key = gdbm_nextkey(dp->di_dbm,okey)) {
			size++;
			if(okey.dsize) free(okey.dptr);
			okey=key;
		}
		dp->di_size = size;
	}
	return dp->di_size;
}

static PyObject *
dbm_subscript(dp, key)
	dbmobject *dp;
register PyObject *key;
{
	PyObject *v;
	datum drec, krec;
	
	if (!PyArg_Parse(key, "s#", &krec.dptr, &krec.dsize) )
		return NULL;
	
	drec = gdbm_fetch(dp->di_dbm, krec);
	if ( drec.dptr == 0 ) {
		PyErr_SetString(PyExc_KeyError,
				PyString_AS_STRING((PyStringObject *)key));
		return NULL;
	}
	v = PyString_FromStringAndSize(drec.dptr, drec.dsize);
	free(drec.dptr);
	return v;
}

static int
dbm_ass_sub(dp, v, w)
	dbmobject *dp;
PyObject *v, *w;
{
        datum krec, drec;
	
        if ( !PyArg_Parse(v, "s#", &krec.dptr, &krec.dsize) ) {
		PyErr_SetString(PyExc_TypeError,
				"gdbm mappings have string indices only");
		return -1;
	}
        if (dp->di_dbm == NULL) {
                 PyErr_SetString(DbmError, "GDBM object has already been closed"); 
                 return -1; 
        }
	dp->di_size = -1;
	if (w == NULL) {
		if ( gdbm_delete(dp->di_dbm, krec) < 0 ) {
			PyErr_SetString(PyExc_KeyError,
				      PyString_AS_STRING((PyStringObject *)v));
			return -1;
		}
	} else {
		if ( !PyArg_Parse(w, "s#", &drec.dptr, &drec.dsize) ) {
			PyErr_SetString(PyExc_TypeError,
				    "gdbm mappings have string elements only");
			return -1;
		}
		errno = 0;
		if ( gdbm_store(dp->di_dbm, krec, drec, GDBM_REPLACE) < 0 ) {
			if (errno != 0)
				PyErr_SetFromErrno(DbmError);
			else
				PyErr_SetString(DbmError,
					   (char *) gdbm_strerror(gdbm_errno));
			return -1;
		}
	}
	return 0;
}

static PyMappingMethods dbm_as_mapping = {
	(inquiry)dbm_length,		/*mp_length*/
	(binaryfunc)dbm_subscript,	/*mp_subscript*/
	(objobjargproc)dbm_ass_sub,	/*mp_ass_subscript*/
};

static PyObject *
dbm_close(dp, args)
	register dbmobject *dp;
PyObject *args;
{
	if ( !PyArg_NoArgs(args) )
		return NULL;
        if ( dp->di_dbm )
		gdbm_close(dp->di_dbm);
	dp->di_dbm = NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
dbm_keys(dp, args)
	register dbmobject *dp;
PyObject *args;
{
	register PyObject *v, *item;
	datum key, nextkey;
	int err;

	if (dp == NULL || !is_dbmobject(dp)) {
		PyErr_BadInternalCall();
		return NULL;
	}

	if (!PyArg_NoArgs(args))
		return NULL;

	check_dbmobject_open(dp);

	v = PyList_New(0);
	if (v == NULL)
		return NULL;

	key = gdbm_firstkey(dp->di_dbm);
	while (key.dptr) {
		item = PyString_FromStringAndSize(key.dptr, key.dsize);
		if (item == NULL) {
			free(key.dptr);
			Py_DECREF(v);
			return NULL;
		}
		err = PyList_Append(v, item);
		Py_DECREF(item);
		if (err != 0) {
			free(key.dptr);
			Py_DECREF(v);
			return NULL;
		}
		nextkey = gdbm_nextkey(dp->di_dbm, key);
		free(key.dptr);
		key = nextkey;
	}

	return v;
}

static PyObject *
dbm_has_key(dp, args)
	register dbmobject *dp;
PyObject *args;
{
	datum key;
	
	if (!PyArg_Parse(args, "s#", &key.dptr, &key.dsize))
		return NULL;
	check_dbmobject_open(dp);
	return PyInt_FromLong((long) gdbm_exists(dp->di_dbm, key));
}

static PyObject *
dbm_firstkey(dp, args)
	register dbmobject *dp;
PyObject *args;
{
	register PyObject *v;
	datum key;

	if (!PyArg_NoArgs(args))
		return NULL;
	check_dbmobject_open(dp);
	key = gdbm_firstkey(dp->di_dbm);
	if (key.dptr) {
		v = PyString_FromStringAndSize(key.dptr, key.dsize);
		free(key.dptr);
		return v;
	} else {
		Py_INCREF(Py_None);
		return Py_None;
	}
}

static PyObject *
dbm_nextkey(dp, args)
	register dbmobject *dp;
PyObject *args;
{
	register PyObject *v;
	datum key, nextkey;

	if (!PyArg_Parse(args, "s#", &key.dptr, &key.dsize))
		return NULL;
	check_dbmobject_open(dp);
	nextkey = gdbm_nextkey(dp->di_dbm, key);
	if (nextkey.dptr) {
		v = PyString_FromStringAndSize(nextkey.dptr, nextkey.dsize);
		free(nextkey.dptr);
		return v;
	} else {
		Py_INCREF(Py_None);
		return Py_None;
	}
}

static PyObject *
dbm_reorganize(dp, args)
	register dbmobject *dp;
PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	check_dbmobject_open(dp);
	errno = 0;
	if (gdbm_reorganize(dp->di_dbm) < 0) {
		if (errno != 0)
			PyErr_SetFromErrno(DbmError);
		else
			PyErr_SetString(DbmError,
					(char *) gdbm_strerror(gdbm_errno));
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
dbm_sync(dp, args)
	register dbmobject *dp;
                PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	check_dbmobject_open(dp);
	gdbm_sync(dp->di_dbm);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef dbm_methods[] = {
	{"close",	(PyCFunction)dbm_close},
	{"keys",	(PyCFunction)dbm_keys},
	{"has_key",	(PyCFunction)dbm_has_key},
	{"firstkey",	(PyCFunction)dbm_firstkey},
	{"nextkey",	(PyCFunction)dbm_nextkey},
	{"reorganize",	(PyCFunction)dbm_reorganize},
	{"sync",                    (PyCFunction)dbm_sync},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
dbm_getattr(dp, name)
	dbmobject *dp;
char *name;
{
	return Py_FindMethod(dbm_methods, (PyObject *)dp, name);
}

static PyTypeObject Dbmtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"gdbm",
	sizeof(dbmobject),
	0,
	(destructor)dbm_dealloc,   /*tp_dealloc*/
	0,			   /*tp_print*/
	(getattrfunc)dbm_getattr,  /*tp_getattr*/
	0,			   /*tp_setattr*/
	0,			   /*tp_compare*/
	0,			   /*tp_repr*/
	0,			   /*tp_as_number*/
	0,			   /*tp_as_sequence*/
	&dbm_as_mapping,	   /*tp_as_mapping*/
};

/* ----------------------------------------------------------------- */

static PyObject *
dbmopen(self, args)
	PyObject *self;
PyObject *args;
{
	char *name;
	char *flags = "r ";
	int iflags;
	int mode = 0666;

/* XXXX add other flags. 2nd character can be "f" meaning open in fast mode. */
	if ( !PyArg_ParseTuple(args, "s|si", &name, &flags, &mode) )
		return NULL;
	switch (flags[0]) {
	case 'r':
		iflags = GDBM_READER;
		break;
	case 'w':
		iflags = GDBM_WRITER;
		break;
	case 'c':
		iflags = GDBM_WRCREAT;
		break;
	case 'n':
		iflags = GDBM_NEWDB;
		break;
	default:
		PyErr_SetString(DbmError,
				"Flags should be one of 'r', 'w', 'c' or 'n'");
		return NULL;
	}
	if (flags[1] == 'f')
		iflags |= GDBM_FAST;
	return newdbmobject(name, iflags, mode);
}

static PyMethodDef dbmmodule_methods[] = {
	{ "open", (PyCFunction)dbmopen, 1 },
	{ 0, 0 },
};

void
initgdbm() {
	PyObject *m, *d;

	m = Py_InitModule("gdbm", dbmmodule_methods);
	d = PyModule_GetDict(m);
	DbmError = PyString_FromString("gdbm.error");
	if ( DbmError == NULL || PyDict_SetItemString(d, "error", DbmError) )
		Py_FatalError("can't define gdbm.error");
}
