
/* Berkeley DB interface.
   Author: Michael McLay
   Hacked: Guido van Rossum
   Btree and Recno additions plus sequence methods: David Ely

   XXX To do:
   - provide interface to the B-tree and record libraries too
   - provide a way to access the various hash functions
   - support more open flags

   The windows port of the Berkeley DB code is hard to find on the web:
   www.nightmare.com/software.html
*/

#include "Python.h"
#ifdef WITH_THREAD
#include "pythread.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_DB_185_H
#include <db_185.h>
#else
#include <db.h>
#endif
/* Please don't include internal header files of the Berkeley db package
   (it messes up the info required in the Setup file) */

typedef struct {
	PyObject_HEAD
	DB *di_bsddb;
	int di_size;	/* -1 means recompute */
#ifdef WITH_THREAD
	PyThread_type_lock di_lock;
#endif
} bsddbobject;

staticforward PyTypeObject Bsddbtype;

#define is_bsddbobject(v) ((v)->ob_type == &Bsddbtype)
#define check_bsddbobject_open(v) if ((v)->di_bsddb == NULL) \
               { PyErr_SetString(BsddbError, "BSDDB object has already been closed"); \
                 return NULL; }

static PyObject *BsddbError;

static PyObject *
newdbhashobject(char *file, int flags, int mode,
		int bsize, int ffactor, int nelem, int cachesize, int hash, int lorder)
{
	bsddbobject *dp;
	HASHINFO info;

	if ((dp = PyObject_New(bsddbobject, &Bsddbtype)) == NULL)
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
	Py_BEGIN_ALLOW_THREADS
	dp->di_bsddb = dbopen(file, flags, mode, DB_HASH, &info);
	Py_END_ALLOW_THREADS
	if (dp->di_bsddb == NULL) {
		PyErr_SetFromErrno(BsddbError);
#ifdef WITH_THREAD
		dp->di_lock = NULL;
#endif
		Py_DECREF(dp);
		return NULL;
	}

	dp->di_size = -1;
#ifdef WITH_THREAD
	dp->di_lock = PyThread_allocate_lock();
	if (dp->di_lock == NULL) {
		PyErr_SetString(BsddbError, "can't allocate lock");
		Py_DECREF(dp);
		return NULL;
	}
#endif

	return (PyObject *)dp;
}

static PyObject *
newdbbtobject(char *file, int flags, int mode,
	      int btflags, int cachesize, int maxkeypage, int minkeypage, int psize, int lorder)
{
	bsddbobject *dp;
	BTREEINFO info;

	if ((dp = PyObject_New(bsddbobject, &Bsddbtype)) == NULL)
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
	Py_BEGIN_ALLOW_THREADS
	dp->di_bsddb = dbopen(file, flags, mode, DB_BTREE, &info);
	Py_END_ALLOW_THREADS
	if (dp->di_bsddb == NULL) {
		PyErr_SetFromErrno(BsddbError);
#ifdef WITH_THREAD
		dp->di_lock = NULL;
#endif
		Py_DECREF(dp);
		return NULL;
	}

	dp->di_size = -1;
#ifdef WITH_THREAD
	dp->di_lock = PyThread_allocate_lock();
	if (dp->di_lock == NULL) {
		PyErr_SetString(BsddbError, "can't allocate lock");
		Py_DECREF(dp);
		return NULL;
	}
#endif

	return (PyObject *)dp;
}

static PyObject *
newdbrnobject(char *file, int flags, int mode,
	      int rnflags, int cachesize, int psize, int lorder, size_t reclen, u_char bval, char *bfname)
{
	bsddbobject *dp;
	RECNOINFO info;

	if ((dp = PyObject_New(bsddbobject, &Bsddbtype)) == NULL)
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
	Py_BEGIN_ALLOW_THREADS
	dp->di_bsddb = dbopen(file, flags, mode, DB_RECNO, &info);
	Py_END_ALLOW_THREADS
	if (dp->di_bsddb == NULL) {
		PyErr_SetFromErrno(BsddbError);
#ifdef WITH_THREAD
		dp->di_lock = NULL;
#endif
		Py_DECREF(dp);
		return NULL;
	}

	dp->di_size = -1;
#ifdef WITH_THREAD
	dp->di_lock = PyThread_allocate_lock();
	if (dp->di_lock == NULL) {
		PyErr_SetString(BsddbError, "can't allocate lock");
		Py_DECREF(dp);
		return NULL;
	}
#endif

	return (PyObject *)dp;
}

static void
bsddb_dealloc(bsddbobject *dp)
{
#ifdef WITH_THREAD
	if (dp->di_lock) {
		PyThread_acquire_lock(dp->di_lock, 0);
		PyThread_release_lock(dp->di_lock);
		PyThread_free_lock(dp->di_lock);
		dp->di_lock = NULL;
	}
#endif
	if (dp->di_bsddb != NULL) {
		int status;
		Py_BEGIN_ALLOW_THREADS
		status = (dp->di_bsddb->close)(dp->di_bsddb);
		Py_END_ALLOW_THREADS
		if (status != 0)
			fprintf(stderr,
				"Python bsddb: close errno %d in dealloc\n",
				errno);
	}
	PyObject_Del(dp);
}

#ifdef WITH_THREAD
#define BSDDB_BGN_SAVE(_dp) Py_BEGIN_ALLOW_THREADS PyThread_acquire_lock(_dp->di_lock,1);
#define BSDDB_END_SAVE(_dp) PyThread_release_lock(_dp->di_lock); Py_END_ALLOW_THREADS
#else
#define BSDDB_BGN_SAVE(_dp) Py_BEGIN_ALLOW_THREADS 
#define BSDDB_END_SAVE(_dp) Py_END_ALLOW_THREADS
#endif

static int
bsddb_length(bsddbobject *dp)
{
        if (dp->di_bsddb == NULL) {
                 PyErr_SetString(BsddbError, "BSDDB object has already been closed"); 
                 return -1; 
        }
	if (dp->di_size < 0) {
		DBT krec, drec;
		int status;
		int size = 0;
		BSDDB_BGN_SAVE(dp)
		for (status = (dp->di_bsddb->seq)(dp->di_bsddb,
						  &krec, &drec,R_FIRST);
		     status == 0;
		     status = (dp->di_bsddb->seq)(dp->di_bsddb,
						  &krec, &drec, R_NEXT))
			size++;
		BSDDB_END_SAVE(dp)
		if (status < 0) {
			PyErr_SetFromErrno(BsddbError);
			return -1;
		}
		dp->di_size = size;
	}
	return dp->di_size;
}

static PyObject *
bsddb_subscript(bsddbobject *dp, PyObject *key)
{
	int status;
	DBT krec, drec;
	char *data,buf[4096];
	int size;
	PyObject *result;

	if (!PyArg_Parse(key, "s#", &data, &size))
		return NULL;
        check_bsddbobject_open(dp);
	
	krec.data = data;
	krec.size = size;

	BSDDB_BGN_SAVE(dp)
	status = (dp->di_bsddb->get)(dp->di_bsddb, &krec, &drec, 0);
	if (status == 0) {
		if (drec.size > sizeof(buf)) data = malloc(drec.size);
		else data = buf;
		memcpy(data,drec.data,drec.size);
	}
	BSDDB_END_SAVE(dp)
	if (status != 0) {
		if (status < 0)
			PyErr_SetFromErrno(BsddbError);
		else
			PyErr_SetObject(PyExc_KeyError, key);
		return NULL;
	}

	result = PyString_FromStringAndSize(data, (int)drec.size);
	if (data != buf) free(data);
	return result;
}

static int
bsddb_ass_sub(bsddbobject *dp, PyObject *key, PyObject *value)
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
		BSDDB_BGN_SAVE(dp)
		status = (dp->di_bsddb->del)(dp->di_bsddb, &krec, 0);
		BSDDB_END_SAVE(dp)
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
		BSDDB_BGN_SAVE(dp)
		status = (dp->di_bsddb->put)(dp->di_bsddb, &krec, &drec, 0);
		BSDDB_END_SAVE(dp)
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
bsddb_close(bsddbobject *dp, PyObject *args)
{
	if (!PyArg_NoArgs(args))
		return NULL;
	if (dp->di_bsddb != NULL) {
		int status;
		BSDDB_BGN_SAVE(dp)
		status = (dp->di_bsddb->close)(dp->di_bsddb);
		BSDDB_END_SAVE(dp)
		if (status != 0) {
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
bsddb_keys(bsddbobject *dp, PyObject *args)
{
	PyObject *list, *item;
	DBT krec, drec;
	char *data=NULL,buf[4096];
	int status;
	int err;

	if (!PyArg_NoArgs(args))
		return NULL;
	check_bsddbobject_open(dp);
	list = PyList_New(0);
	if (list == NULL)
		return NULL;
	BSDDB_BGN_SAVE(dp)
	status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec, &drec, R_FIRST);
	if (status == 0) {
		if (krec.size > sizeof(buf)) data = malloc(krec.size);
		else data = buf;
		memcpy(data,krec.data,krec.size);
	}
	BSDDB_END_SAVE(dp)
	while (status == 0) {
		item = PyString_FromStringAndSize(data, (int)krec.size);
		if (data != buf) free(data);
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
		BSDDB_BGN_SAVE(dp)
		status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec, &drec, R_NEXT);
		if (status == 0) {
			if (krec.size > sizeof(buf)) data = malloc(krec.size);
			else data = buf;
			memcpy(data,krec.data,krec.size);
		}
		BSDDB_END_SAVE(dp)
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
bsddb_has_key(bsddbobject *dp, PyObject *args)
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

	BSDDB_BGN_SAVE(dp)
	status = (dp->di_bsddb->get)(dp->di_bsddb, &krec, &drec, 0);
	BSDDB_END_SAVE(dp)
	if (status < 0) {
		PyErr_SetFromErrno(BsddbError);
		return NULL;
	}

	return PyInt_FromLong(status == 0);
}

static PyObject *
bsddb_set_location(bsddbobject *dp, PyObject *key)
{
	int status;
	DBT krec, drec;
	char *data,buf[4096];
	int size;
	PyObject *result;

	if (!PyArg_Parse(key, "s#", &data, &size))
		return NULL;
	check_bsddbobject_open(dp);
	krec.data = data;
	krec.size = size;

	BSDDB_BGN_SAVE(dp)
	status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec, &drec, R_CURSOR);
	if (status == 0) {
		if (drec.size > sizeof(buf)) data = malloc(drec.size);
		else data = buf;
		memcpy(data,drec.data,drec.size);
	}
	BSDDB_END_SAVE(dp)
	if (status != 0) {
		if (status < 0)
			PyErr_SetFromErrno(BsddbError);
		else
			PyErr_SetObject(PyExc_KeyError, key);
		return NULL;
	}

	result = Py_BuildValue("s#s#", krec.data, krec.size, data, drec.size);
	if (data != buf) free(data);
	return result;
}

static PyObject *
bsddb_seq(bsddbobject *dp, PyObject *args, int sequence_request)
{
	int status;
	DBT krec, drec;
	char *kdata=NULL,kbuf[4096];
	char *ddata=NULL,dbuf[4096];
	PyObject *result;

	if (!PyArg_NoArgs(args))
		return NULL;

	check_bsddbobject_open(dp);
	krec.data = 0;
	krec.size = 0;

	BSDDB_BGN_SAVE(dp)
	status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec,
				     &drec, sequence_request);
	if (status == 0) {
		if (krec.size > sizeof(kbuf)) kdata = malloc(krec.size);
		else kdata = kbuf;
		memcpy(kdata,krec.data,krec.size);
		if (drec.size > sizeof(dbuf)) ddata = malloc(drec.size);
		else ddata = dbuf;
		memcpy(ddata,drec.data,drec.size);
	}
	BSDDB_END_SAVE(dp)
	if (status != 0) {
		if (status < 0)
			PyErr_SetFromErrno(BsddbError);
		else
			PyErr_SetObject(PyExc_KeyError, args);
		return NULL;
	}

	result = Py_BuildValue("s#s#", kdata, krec.size, ddata, drec.size);
	if (kdata != kbuf) free(kdata);
	if (ddata != dbuf) free(ddata);
	return result;
}

static PyObject *
bsddb_next(bsddbobject *dp, PyObject *key)
{
	return bsddb_seq(dp, key, R_NEXT);
}
static PyObject *
bsddb_previous(bsddbobject *dp, PyObject *key)
{
	return bsddb_seq(dp, key, R_PREV);
}
static PyObject *
bsddb_first(bsddbobject *dp, PyObject *key)
{
	return bsddb_seq(dp, key, R_FIRST);
}
static PyObject *
bsddb_last(bsddbobject *dp, PyObject *key)
{
	return bsddb_seq(dp, key, R_LAST);
}
static PyObject *
bsddb_sync(bsddbobject *dp, PyObject *args)
{
	int status;

	if (!PyArg_NoArgs(args))
		return NULL;
	check_bsddbobject_open(dp);
	BSDDB_BGN_SAVE(dp)
	status = (dp->di_bsddb->sync)(dp->di_bsddb, 0);
	BSDDB_END_SAVE(dp)
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
bsddb_getattr(PyObject *dp, char *name)
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
bsdhashopen(PyObject *self, PyObject *args)
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

	if (!PyArg_ParseTuple(args, "s|siiiiiii:hashopen",
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
bsdbtopen(PyObject *self, PyObject *args)
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

	if (!PyArg_ParseTuple(args, "s|siiiiiii:btopen",
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
bsdrnopen(PyObject *self, PyObject *args)
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

	if (!PyArg_ParseTuple(args, "s|siiiiiiss:rnopen",
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

DL_EXPORT(void)
initbsddb(void) {
	PyObject *m, *d;

	Bsddbtype.ob_type = &PyType_Type;
	m = Py_InitModule("bsddb", bsddbmodule_methods);
	d = PyModule_GetDict(m);
	BsddbError = PyErr_NewException("bsddb.error", NULL, NULL);
	if (BsddbError != NULL)
		PyDict_SetItemString(d, "error", BsddbError);
}
