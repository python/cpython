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

/* DBM module using dictionary interface */


#include "Python.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ndbm.h>

typedef struct {
	PyObject_HEAD
	int di_size;	/* -1 means recompute */
	DBM *di_dbm;
} dbmobject;

staticforward PyTypeObject Dbmtype;

#define is_dbmobject(v) ((v)->ob_type == &Dbmtype)
#define check_dbmobject_open(v) if ((v)->di_dbm == NULL) \
               { PyErr_SetString(DbmError, "DBM object has already been closed"); \
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
	if ( (dp->di_dbm = dbm_open(file, flags, mode)) == 0 ) {
		PyErr_SetFromErrno(DbmError);
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
		dbm_close(dp->di_dbm);
	PyMem_DEL(dp);
}

static int
dbm_length(dp)
	dbmobject *dp;
{
        if (dp->di_dbm == NULL) {
                 PyErr_SetString(DbmError, "DBM object has already been closed"); 
                 return -1; 
        }
        if ( dp->di_size < 0 ) {
		datum key;
		int size;

		size = 0;
		for ( key=dbm_firstkey(dp->di_dbm); key.dptr;
		      key = dbm_nextkey(dp->di_dbm))
			size++;
		dp->di_size = size;
	}
	return dp->di_size;
}

static PyObject *
dbm_subscript(dp, key)
	dbmobject *dp;
register PyObject *key;
{
	datum drec, krec;
	
	if (!PyArg_Parse(key, "s#", &krec.dptr, &krec.dsize) )
		return NULL;
	
        check_dbmobject_open(dp);
	drec = dbm_fetch(dp->di_dbm, krec);
	if ( drec.dptr == 0 ) {
		PyErr_SetString(PyExc_KeyError,
				PyString_AS_STRING((PyStringObject *)key));
		return NULL;
	}
	if ( dbm_error(dp->di_dbm) ) {
		dbm_clearerr(dp->di_dbm);
		PyErr_SetString(DbmError, "");
		return NULL;
	}
	return PyString_FromStringAndSize(drec.dptr, drec.dsize);
}

static int
dbm_ass_sub(dp, v, w)
	dbmobject *dp;
PyObject *v, *w;
{
        datum krec, drec;
	
        if ( !PyArg_Parse(v, "s#", &krec.dptr, &krec.dsize) ) {
		PyErr_SetString(PyExc_TypeError,
				"dbm mappings have string indices only");
		return -1;
	}
        if (dp->di_dbm == NULL) {
                 PyErr_SetString(DbmError, "DBM object has already been closed"); 
                 return -1;
        }
	dp->di_size = -1;
	if (w == NULL) {
		if ( dbm_delete(dp->di_dbm, krec) < 0 ) {
			dbm_clearerr(dp->di_dbm);
			PyErr_SetString(PyExc_KeyError,
				      PyString_AS_STRING((PyStringObject *)v));
			return -1;
		}
	} else {
		if ( !PyArg_Parse(w, "s#", &drec.dptr, &drec.dsize) ) {
			PyErr_SetString(PyExc_TypeError,
				     "dbm mappings have string elements only");
			return -1;
		}
		if ( dbm_store(dp->di_dbm, krec, drec, DBM_REPLACE) < 0 ) {
			dbm_clearerr(dp->di_dbm);
			PyErr_SetString(DbmError,
					"Cannot add item to database");
			return -1;
		}
	}
	if ( dbm_error(dp->di_dbm) ) {
		dbm_clearerr(dp->di_dbm);
		PyErr_SetString(DbmError, "");
		return -1;
	}
	return 0;
}

static PyMappingMethods dbm_as_mapping = {
	(inquiry)dbm_length,		/*mp_length*/
	(binaryfunc)dbm_subscript,	/*mp_subscript*/
	(objobjargproc)dbm_ass_sub,	/*mp_ass_subscript*/
};

static PyObject *
dbm__close(dp, args)
	register dbmobject *dp;
PyObject *args;
{
	if ( !PyArg_NoArgs(args) )
		return NULL;
        if ( dp->di_dbm )
		dbm_close(dp->di_dbm);
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
	datum key;
	int err;

	if (!PyArg_NoArgs(args))
		return NULL;
        check_dbmobject_open(dp);
	v = PyList_New(0);
	if (v == NULL)
		return NULL;
	for (key = dbm_firstkey(dp->di_dbm); key.dptr;
	     key = dbm_nextkey(dp->di_dbm)) {
		item = PyString_FromStringAndSize(key.dptr, key.dsize);
		if (item == NULL) {
			Py_DECREF(v);
			return NULL;
		}
		err = PyList_Append(v, item);
		Py_DECREF(item);
		if (err != 0) {
			Py_DECREF(v);
			return NULL;
		}
	}
	return v;
}

static PyObject *
dbm_has_key(dp, args)
	register dbmobject *dp;
PyObject *args;
{
	datum key, val;
	
	if (!PyArg_Parse(args, "s#", &key.dptr, &key.dsize))
		return NULL;
        check_dbmobject_open(dp);
	val = dbm_fetch(dp->di_dbm, key);
	return PyInt_FromLong(val.dptr != NULL);
}

static PyMethodDef dbm_methods[] = {
	{"close",	(PyCFunction)dbm__close},
	{"keys",	(PyCFunction)dbm_keys},
	{"has_key",	(PyCFunction)dbm_has_key},
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
	"dbm",
	sizeof(dbmobject),
	0,
	(destructor)dbm_dealloc,  /*tp_dealloc*/
	0,			  /*tp_print*/
	(getattrfunc)dbm_getattr, /*tp_getattr*/
	0,			  /*tp_setattr*/
	0,			  /*tp_compare*/
	0,			  /*tp_repr*/
	0,			  /*tp_as_number*/
	0,			  /*tp_as_sequence*/
	&dbm_as_mapping,	  /*tp_as_mapping*/
};

/* ----------------------------------------------------------------- */

static PyObject *
dbmopen(self, args)
	PyObject *self;
PyObject *args;
{
	char *name;
	char *flags = "r";
	int iflags;
	int mode = 0666;

        if ( !PyArg_ParseTuple(args, "s|si", &name, &flags, &mode) )
		return NULL;
	if ( strcmp(flags, "r") == 0 )
		iflags = O_RDONLY;
	else if ( strcmp(flags, "w") == 0 )
		iflags = O_RDWR;
	else if ( strcmp(flags, "rw") == 0 ) /* B/W compat */
		iflags = O_RDWR|O_CREAT; 
	else if ( strcmp(flags, "c") == 0 )
		iflags = O_RDWR|O_CREAT;
	else if ( strcmp(flags, "n") == 0 )
		iflags = O_RDWR|O_CREAT|O_TRUNC;
	else {
		PyErr_SetString(DbmError,
				"Flags should be one of 'r', 'w', 'c' or 'n'");
		return NULL;
	}
        return newdbmobject(name, iflags, mode);
}

static PyMethodDef dbmmodule_methods[] = {
	{ "open", (PyCFunction)dbmopen, 1 },
	{ 0, 0 },
};

void
initdbm() {
	PyObject *m, *d;

	m = Py_InitModule("dbm", dbmmodule_methods);
	d = PyModule_GetDict(m);
	DbmError = PyErr_NewException("dbm.error", NULL, NULL);
	if (DbmError != NULL)
		PyDict_SetItemString(d, "error", DbmError);
}
