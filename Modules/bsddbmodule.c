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

/* Berkeley DB interface.
   Author: Michael McLay
   Hacked: Guido van Rossum
   Btree and Recno additions plus sequence methods: David Ely

   XXX To do:
   - provide interface to the B-tree and record libraries too
   - provide a way to access the various hash functions
   - support more open flags
 */

#include "Python.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <db.h>
/* Please don't include internal header files of the Berkeley db package
   (it messes up the info required in the Setup file) */

typedef struct {
	PyObject_HEAD
	DB *di_bsddb;
	int di_size;	/* -1 means recompute */
} bsddbobject;

staticforward PyTypeObject Bsddbtype;

#define is_bsddbobject(v) ((v)->ob_type == &Bsddbtype)
#define check_bsddbobject_open(v) if ((v)->di_bsddb == NULL) \
               { PyErr_SetString(BsddbError, "BSDDB object has already been closed"); \
                 return NULL; }

static PyObject *BsddbError;

static PyObject *
newdbhashobject(file, flags, mode,
		bsize, ffactor, nelem, cachesize, hash, lorder)
	char *file;
        int flags;
        int mode;
        int bsize;
        int ffactor;
        int nelem;
        int cachesize;
        int hash; /* XXX ignored */
        int lorder;
{
	bsddbobject *dp;
	HASHINFO info;

	if ((dp = PyObject_NEW(bsddbobject, &Bsddbtype)) == NULL)
		return NULL;

	info.bsize = bsize;
	info.ffactor = ffactor;
	info.nelem = nelem;
	info.cachesize = cachesize;
	info.hash = NULL; /* XXX should derive from hash argument */
	info.lorder = lorder;

#ifdef O_BINARY
	flags |= O_BINARY;
#endif
	if ((dp->di_bsddb = dbopen(file, flags,
				   mode, DB_HASH, &info)) == NULL) {
		PyErr_SetFromErrno(BsddbError);
		Py_DECREF(dp);
		return NULL;
	}

	dp->di_size = -1;

	return (PyObject *)dp;
}

static PyObject *
newdbbtobject(file, flags, mode,
	      btflags, cachesize, maxkeypage, minkeypage, psize, lorder)
	char *file;
        int flags;
        int mode;
        int btflags;
        int cachesize;
        int maxkeypage;
        int minkeypage;
        int psize;
        int lorder;
{
	bsddbobject *dp;
	BTREEINFO info;

	if ((dp = PyObject_NEW(bsddbobject, &Bsddbtype)) == NULL)
		return NULL;

	info.flags = btflags;
	info.cachesize = cachesize;
	info.maxkeypage = maxkeypage;
	info.minkeypage = minkeypage;
	info.psize = psize;
	info.lorder = lorder;
	info.compare = 0; /* Use default comparison functions, for now..*/
	info.prefix = 0;

#ifdef O_BINARY
	flags |= O_BINARY;
#endif
	if ((dp->di_bsddb = dbopen(file, flags,
				   mode, DB_BTREE, &info)) == NULL) {
		PyErr_SetFromErrno(BsddbError);
		Py_DECREF(dp);
		return NULL;
	}

	dp->di_size = -1;

	return (PyObject *)dp;
}

static PyObject *
newdbrnobject(file, flags, mode,
	      rnflags, cachesize, psize, lorder, reclen, bval, bfname)
	char *file;
        int flags;
        int mode;
        int rnflags;
        int cachesize;
        int psize;
        int lorder;
        size_t reclen;
        u_char bval;
        char *bfname;
{
	bsddbobject *dp;
	RECNOINFO info;

	if ((dp = PyObject_NEW(bsddbobject, &Bsddbtype)) == NULL)
		return NULL;

	info.flags = rnflags;
	info.cachesize = cachesize;
	info.psize = psize;
	info.lorder = lorder;
	info.reclen = reclen;
	info.bval = bval;
	info.bfname = bfname;

#ifdef O_BINARY
	flags |= O_BINARY;
#endif
	if ((dp->di_bsddb = dbopen(file, flags, mode,
				   DB_RECNO, &info)) == NULL) {
		PyErr_SetFromErrno(BsddbError);
		Py_DECREF(dp);
		return NULL;
	}

	dp->di_size = -1;

	return (PyObject *)dp;
}


static void
bsddb_dealloc(dp)
	bsddbobject *dp;
{
	if (dp->di_bsddb != NULL) {
		if ((dp->di_bsddb->close)(dp->di_bsddb) != 0)
			fprintf(stderr,
				"Python bsddb: close errno %d in dealloc\n",
				errno);
	}
	PyMem_DEL(dp);
}

static int
bsddb_length(dp)
	bsddbobject *dp;
{
        if (dp->di_bsddb == NULL) {
                 PyErr_SetString(BsddbError, "BSDDB object has already been closed"); 
                 return -1; 
        }
	if (dp->di_size < 0) {
		DBT krec, drec;
		int status;
		int size = 0;
		for (status = (dp->di_bsddb->seq)(dp->di_bsddb,
						  &krec, &drec,R_FIRST);
		     status == 0;
		     status = (dp->di_bsddb->seq)(dp->di_bsddb,
						  &krec, &drec, R_NEXT))
			size++;
		if (status < 0) {
			PyErr_SetFromErrno(BsddbError);
			return -1;
		}
		dp->di_size = size;
	}
	return dp->di_size;
}

static PyObject *
bsddb_subscript(dp, key)
	bsddbobject *dp;
        PyObject *key;
{
	int status;
	DBT krec, drec;
	char *data;
	int size;

	if (!PyArg_Parse(key, "s#", &data, &size))
		return NULL;
        check_bsddbobject_open(dp);
	
	krec.data = data;
	krec.size = size;

	status = (dp->di_bsddb->get)(dp->di_bsddb, &krec, &drec, 0);
	if (status != 0) {
		if (status < 0)
			PyErr_SetFromErrno(BsddbError);
		else
			PyErr_SetObject(PyExc_KeyError, key);
		return NULL;
	}

	return PyString_FromStringAndSize((char *)drec.data, (int)drec.size);
}

static int
bsddb_ass_sub(dp, key, value)
	bsddbobject *dp;
        PyObject *key, *value;
{
	int status;
	DBT krec, drec;
	char *data;
	int size;

	if (!PyArg_Parse(key, "s#", &data, &size)) {
		PyErr_SetString(PyExc_TypeError,
				"bsddb key type must be string");
		return -1;
	}
        if (dp->di_bsddb == NULL) {
                 PyErr_SetString(BsddbError, "BSDDB object has already been closed"); 
                 return -1; 
        }
	krec.data = data;
	krec.size = size;
	dp->di_size = -1;
	if (value == NULL) {
		status = (dp->di_bsddb->del)(dp->di_bsddb, &krec, 0);
	}
	else {
		if (!PyArg_Parse(value, "s#", &data, &size)) {
			PyErr_SetString(PyExc_TypeError,
					"bsddb value type must be string");
			return -1;
		}
		drec.data = data;
		drec.size = size;
#if 0
		/* For RECNO, put fails with 'No space left on device'
		   after a few short records are added??  Looks fine
		   to this point... linked with 1.85 on Solaris Intel
		   Roger E. Masse 1/16/97
		 */
		printf("before put data: '%s', size: %d\n",
		       drec.data, drec.size);
		printf("before put key= '%s', size= %d\n",
		       krec.data, krec.size);
#endif
		status = (dp->di_bsddb->put)(dp->di_bsddb, &krec, &drec, 0);
	}
	if (status != 0) {
		if (status < 0)
			PyErr_SetFromErrno(BsddbError);
		else
			PyErr_SetObject(PyExc_KeyError, key);
		return -1;
	}
	return 0;
}

static PyMappingMethods bsddb_as_mapping = {
	(inquiry)bsddb_length,		/*mp_length*/
	(binaryfunc)bsddb_subscript,	/*mp_subscript*/
	(objobjargproc)bsddb_ass_sub,	/*mp_ass_subscript*/
};

static PyObject *
bsddb_close(dp, args)
	bsddbobject *dp;
        PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	if (dp->di_bsddb != NULL) {
		if ((dp->di_bsddb->close)(dp->di_bsddb) != 0) {
			dp->di_bsddb = NULL;
			PyErr_SetFromErrno(BsddbError);
			return NULL;
		}
	}
	dp->di_bsddb = NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
bsddb_keys(dp, args)
	bsddbobject *dp;
        PyObject *args;
{
	PyObject *list, *item;
	DBT krec, drec;
	int status;
	int err;

	if (!PyArg_NoArgs(args))
		return NULL;
	check_bsddbobject_open(dp);
	list = PyList_New(0);
	if (list == NULL)
		return NULL;
	for (status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec, &drec, R_FIRST);
	     status == 0;
	     status = (dp->di_bsddb->seq)(dp->di_bsddb,
					  &krec, &drec, R_NEXT)) {
		item = PyString_FromStringAndSize((char *)krec.data,
						  (int)krec.size);
		if (item == NULL) {
			Py_DECREF(list);
			return NULL;
		}
		err = PyList_Append(list, item);
		Py_DECREF(item);
		if (err != 0) {
			Py_DECREF(list);
			return NULL;
		}
	}
	if (status < 0) {
		PyErr_SetFromErrno(BsddbError);
		Py_DECREF(list);
		return NULL;
	}
	if (dp->di_size < 0)
		dp->di_size = PyList_Size(list); /* We just did the work */
	return list;
}

static PyObject *
bsddb_has_key(dp, args)
	bsddbobject *dp;
        PyObject *args;
{
	DBT krec, drec;
	int status;
	char *data;
	int size;

	if (!PyArg_Parse(args, "s#", &data, &size))
		return NULL;
	check_bsddbobject_open(dp);
	krec.data = data;
	krec.size = size;

	status = (dp->di_bsddb->get)(dp->di_bsddb, &krec, &drec, 0);
	if (status < 0) {
		PyErr_SetFromErrno(BsddbError);
		return NULL;
	}

	return PyInt_FromLong(status == 0);
}

static PyObject *
bsddb_set_location(dp, key)
	bsddbobject *dp;
        PyObject *key;
{
	int status;
	DBT krec, drec;
	char *data;
	int size;

	if (!PyArg_Parse(key, "s#", &data, &size))
		return NULL;
	check_bsddbobject_open(dp);
	krec.data = data;
	krec.size = size;

	status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec, &drec, R_CURSOR);
	if (status != 0) {
		if (status < 0)
			PyErr_SetFromErrno(BsddbError);
		else
			PyErr_SetObject(PyExc_KeyError, key);
		return NULL;
	}

	return Py_BuildValue("s#s#", krec.data, krec.size,
			     drec.data, drec.size);
}

static PyObject *
bsddb_seq(dp, args, sequence_request)
	bsddbobject *dp;
        PyObject *args;
        int sequence_request;
{
	int status;
	DBT krec, drec;

	if (!PyArg_NoArgs(args))
		return NULL;

	check_bsddbobject_open(dp);
	krec.data = 0;
	krec.size = 0;

	status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec,
				     &drec, sequence_request);
	if (status != 0) {
		if (status < 0)
			PyErr_SetFromErrno(BsddbError);
		else
			PyErr_SetObject(PyExc_KeyError, args);
		return NULL;
	}

	return Py_BuildValue("s#s#", krec.data, krec.size,
			     drec.data, drec.size);
}

static PyObject *
bsddb_next(dp, key)
	bsddbobject *dp;
        PyObject *key;
{
	return bsddb_seq(dp, key, R_NEXT);
}
static PyObject *
bsddb_previous(dp, key)
	bsddbobject *dp;
        PyObject *key;
{
	return bsddb_seq(dp, key, R_PREV);
}
static PyObject *
bsddb_first(dp, key)
	bsddbobject *dp;
        PyObject *key;
{
	return bsddb_seq(dp, key, R_FIRST);
}
static PyObject *
bsddb_last(dp, key)
	bsddbobject *dp;
        PyObject *key;
{
	return bsddb_seq(dp, key, R_LAST);
}
static PyObject *
bsddb_sync(dp, args)
	bsddbobject *dp;
        PyObject *args;
{
	int status;

	if (!PyArg_NoArgs(args))
		return NULL;
	check_bsddbobject_open(dp);
	status = (dp->di_bsddb->sync)(dp->di_bsddb, 0);
	if (status != 0) {
		PyErr_SetFromErrno(BsddbError);
		return NULL;
	}
	return PyInt_FromLong(status = 0);
}
static PyMethodDef bsddb_methods[] = {
	{"close",		(PyCFunction)bsddb_close},
	{"keys",		(PyCFunction)bsddb_keys},
	{"has_key",		(PyCFunction)bsddb_has_key},
	{"set_location",	(PyCFunction)bsddb_set_location},
	{"next",		(PyCFunction)bsddb_next},
	{"previous",	(PyCFunction)bsddb_previous},
	{"first",		(PyCFunction)bsddb_first},
	{"last",		(PyCFunction)bsddb_last},
	{"sync",		(PyCFunction)bsddb_sync},
	{NULL,	       	NULL}		/* sentinel */
};

static PyObject *
bsddb_getattr(dp, name)
	PyObject *dp;
        char *name;
{
	return Py_FindMethod(bsddb_methods, dp, name);
}

static PyTypeObject Bsddbtype = {
	PyObject_HEAD_INIT(NULL)
	0,
	"bsddb",
	sizeof(bsddbobject),
	0,
	(destructor)bsddb_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)bsddb_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	&bsddb_as_mapping,	/*tp_as_mapping*/
};

static PyObject *
bsdhashopen(self, args)
	PyObject *self;
        PyObject *args;
{
	char *file;
	char *flag = NULL;
	int flags = O_RDONLY;
	int mode = 0666;
	int bsize = 0;
	int ffactor = 0;
	int nelem = 0;
	int cachesize = 0;
	int hash = 0; /* XXX currently ignored */
	int lorder = 0;

	if (!PyArg_ParseTuple(args, "s|siiiiiii",
			      &file, &flag, &mode,
			      &bsize, &ffactor, &nelem, &cachesize,
			      &hash, &lorder))
		return NULL;
	if (flag != NULL) {
		/* XXX need to pass O_EXCL, O_EXLOCK, O_NONBLOCK, O_SHLOCK */
		if (flag[0] == 'r')
			flags = O_RDONLY;
		else if (flag[0] == 'w')
			flags = O_RDWR;
		else if (flag[0] == 'c')
			flags = O_RDWR|O_CREAT;
		else if (flag[0] == 'n')
			flags = O_RDWR|O_CREAT|O_TRUNC;
		else {
			PyErr_SetString(BsddbError,
				"Flag should begin with 'r', 'w', 'c' or 'n'");
			return NULL;
		}
		if (flag[1] == 'l') {
#if defined(O_EXLOCK) && defined(O_SHLOCK)
			if (flag[0] == 'r')
				flags |= O_SHLOCK;
			else
				flags |= O_EXLOCK;
#else
			PyErr_SetString(BsddbError,
				     "locking not supported on this platform");
			return NULL;
#endif
		}
	}
	return newdbhashobject(file, flags, mode,
			       bsize, ffactor, nelem, cachesize, hash, lorder);
}

static PyObject *
bsdbtopen(self, args)
	PyObject *self;
        PyObject *args;
{
	char *file;
	char *flag = NULL;
	int flags = O_RDONLY;
	int mode = 0666;
	int cachesize = 0;
	int maxkeypage = 0;
	int minkeypage = 0;
	int btflags = 0;
	unsigned int psize = 0;
	int lorder = 0;

	if (!PyArg_ParseTuple(args, "s|siiiiiii",
			      &file, &flag, &mode,
			      &btflags, &cachesize, &maxkeypage, &minkeypage,
			      &psize, &lorder))
		return NULL;
	if (flag != NULL) {
		/* XXX need to pass O_EXCL, O_EXLOCK, O_NONBLOCK, O_SHLOCK */
		if (flag[0] == 'r')
			flags = O_RDONLY;
		else if (flag[0] == 'w')
			flags = O_RDWR;
		else if (flag[0] == 'c')
			flags = O_RDWR|O_CREAT;
		else if (flag[0] == 'n')
			flags = O_RDWR|O_CREAT|O_TRUNC;
		else {
			PyErr_SetString(BsddbError,
			       "Flag should begin with 'r', 'w', 'c' or 'n'");
			return NULL;
		}
		if (flag[1] == 'l') {
#if defined(O_EXLOCK) && defined(O_SHLOCK)
			if (flag[0] == 'r')
				flags |= O_SHLOCK;
			else
				flags |= O_EXLOCK;
#else
			PyErr_SetString(BsddbError,
				    "locking not supported on this platform");
			return NULL;
#endif
		}
	}
	return newdbbtobject(file, flags, mode,
			     btflags, cachesize, maxkeypage, minkeypage,
			     psize, lorder);
}

static PyObject *
bsdrnopen(self, args)
	PyObject *self;
        PyObject *args;
{
	char *file;
	char *flag = NULL;
	int flags = O_RDONLY;
	int mode = 0666;
	int cachesize = 0;
	int rnflags = 0;
	unsigned int psize = 0;
	int lorder = 0;
	size_t reclen = 0;
	char  *bval = "";
	char *bfname = NULL;

	if (!PyArg_ParseTuple(args, "s|siiiiiiss",
			      &file, &flag, &mode,
			      &rnflags, &cachesize, &psize, &lorder,
			      &reclen, &bval, &bfname))
		return NULL;

# if 0
	printf("file: %s\n", file);
	printf("flag: %s\n", flag);
	printf("mode: %d\n", mode);
	printf("rnflags: 0x%x\n", rnflags);
	printf("cachesize: %d\n", cachesize);
	printf("psize: %d\n", psize);
	printf("lorder: %d\n", 0);
	printf("reclen: %d\n", reclen);
	printf("bval: %c\n", bval[0]);
	printf("bfname %s\n", bfname);
#endif
	
	if (flag != NULL) {
		/* XXX need to pass O_EXCL, O_EXLOCK, O_NONBLOCK, O_SHLOCK */
		if (flag[0] == 'r')
			flags = O_RDONLY;
		else if (flag[0] == 'w')
			flags = O_RDWR;
		else if (flag[0] == 'c')
			flags = O_RDWR|O_CREAT;
		else if (flag[0] == 'n')
			flags = O_RDWR|O_CREAT|O_TRUNC;
		else {
			PyErr_SetString(BsddbError,
			       "Flag should begin with 'r', 'w', 'c' or 'n'");
			return NULL;
		}
		if (flag[1] == 'l') {
#if defined(O_EXLOCK) && defined(O_SHLOCK)
			if (flag[0] == 'r')
				flags |= O_SHLOCK;
			else
				flags |= O_EXLOCK;
#else
			PyErr_SetString(BsddbError,
				    "locking not supported on this platform");
			return NULL;
#endif
		}
		else if (flag[1] != '\0') {
			PyErr_SetString(BsddbError,
				       "Flag char 2 should be 'l' or absent");
			return NULL;
		}
	}
	return newdbrnobject(file, flags, mode, rnflags, cachesize,
			     psize, lorder, reclen, bval[0], bfname);
}

static PyMethodDef bsddbmodule_methods[] = {
	{"hashopen",	(PyCFunction)bsdhashopen, 1},
	{"btopen",	(PyCFunction)bsdbtopen, 1},
	{"rnopen",	(PyCFunction)bsdrnopen, 1},
	{0,		0},
};

void
initbsddb() {
	PyObject *m, *d;

	Bsddbtype.ob_type = &PyType_Type;
	m = Py_InitModule("bsddb", bsddbmodule_methods);
	d = PyModule_GetDict(m);
	BsddbError = PyErr_NewException("bsddb.error", NULL, NULL);
	if (BsddbError != NULL)
		PyDict_SetItemString(d, "error", BsddbError);
}
