
/* DBM module using dictionary interface */


#include "Python.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Some Linux systems install gdbm/ndbm.h, but not ndbm.h.  This supports
 * whichever configure was able to locate.
 */
#if defined(HAVE_NDBM_H)
#include <ndbm.h>
#if defined(PYOS_OS2) && !defined(PYCC_GCC)
static char *which_dbm = "ndbm";
#else
static char *which_dbm = "GNU gdbm";  /* EMX port of GDBM */
#endif
#elif defined(HAVE_GDBM_NDBM_H)
#include <gdbm/ndbm.h>
static char *which_dbm = "GNU gdbm";
#elif defined(HAVE_BERKDB_H)
#include <db.h>
static char *which_dbm = "Berkeley DB";
#else
#error "No ndbm.h available!"
#endif

typedef struct {
	PyObject_HEAD
	int di_size;	/* -1 means recompute */
	DBM *di_dbm;
} dbmobject;

static PyTypeObject Dbmtype;

#define is_dbmobject(v) (Py_Type(v) == &Dbmtype)
#define check_dbmobject_open(v) if ((v)->di_dbm == NULL) \
               { PyErr_SetString(DbmError, "DBM object has already been closed"); \
                 return NULL; }

static PyObject *DbmError;

static PyObject *
newdbmobject(char *file, int flags, int mode)
{
        dbmobject *dp;

	dp = PyObject_New(dbmobject, &Dbmtype);
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
dbm_dealloc(register dbmobject *dp)
{
        if ( dp->di_dbm )
		dbm_close(dp->di_dbm);
	PyObject_Del(dp);
}

static Py_ssize_t
dbm_length(dbmobject *dp)
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
dbm_subscript(dbmobject *dp, register PyObject *key)
{
	datum drec, krec;
	int tmp_size;
	
	if (!PyArg_Parse(key, "s#", &krec.dptr, &tmp_size) )
		return NULL;
	
	krec.dsize = tmp_size;
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
dbm_ass_sub(dbmobject *dp, PyObject *v, PyObject *w)
{
        datum krec, drec;
	int tmp_size;
	
        if ( !PyArg_Parse(v, "s#", &krec.dptr, &tmp_size) ) {
		PyErr_SetString(PyExc_TypeError,
				"dbm mappings have string indices only");
		return -1;
	}
	krec.dsize = tmp_size;
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
		if ( !PyArg_Parse(w, "s#", &drec.dptr, &tmp_size) ) {
			PyErr_SetString(PyExc_TypeError,
				     "dbm mappings have string elements only");
			return -1;
		}
		drec.dsize = tmp_size;
		if ( dbm_store(dp->di_dbm, krec, drec, DBM_REPLACE) < 0 ) {
			dbm_clearerr(dp->di_dbm);
			PyErr_SetString(DbmError,
					"cannot add item to database");
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
	(lenfunc)dbm_length,		/*mp_length*/
	(binaryfunc)dbm_subscript,	/*mp_subscript*/
	(objobjargproc)dbm_ass_sub,	/*mp_ass_subscript*/
};

static PyObject *
dbm__close(register dbmobject *dp, PyObject *unused)
{
        if (dp->di_dbm)
		dbm_close(dp->di_dbm);
	dp->di_dbm = NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
dbm_keys(register dbmobject *dp, PyObject *unused)
{
	register PyObject *v, *item;
	datum key;
	int err;

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
dbm_has_key(register dbmobject *dp, PyObject *args)
{
	char *tmp_ptr;
	datum key, val;
	int tmp_size;
	
	if (!PyArg_ParseTuple(args, "s#:has_key", &tmp_ptr, &tmp_size))
		return NULL;
	key.dptr = tmp_ptr;
	key.dsize = tmp_size;
        check_dbmobject_open(dp);
	val = dbm_fetch(dp->di_dbm, key);
	return PyInt_FromLong(val.dptr != NULL);
}

static PyObject *
dbm_get(register dbmobject *dp, PyObject *args)
{
	datum key, val;
	PyObject *defvalue = Py_None;
	char *tmp_ptr;
	int tmp_size;

	if (!PyArg_ParseTuple(args, "s#|O:get",
                              &tmp_ptr, &tmp_size, &defvalue))
		return NULL;
	key.dptr = tmp_ptr;
	key.dsize = tmp_size;
        check_dbmobject_open(dp);
	val = dbm_fetch(dp->di_dbm, key);
	if (val.dptr != NULL)
		return PyString_FromStringAndSize(val.dptr, val.dsize);
	else {
		Py_INCREF(defvalue);
		return defvalue;
	}
}

static PyObject *
dbm_setdefault(register dbmobject *dp, PyObject *args)
{
	datum key, val;
	PyObject *defvalue = NULL;
	char *tmp_ptr;
	int tmp_size;

	if (!PyArg_ParseTuple(args, "s#|S:setdefault",
                              &tmp_ptr, &tmp_size, &defvalue))
		return NULL;
	key.dptr = tmp_ptr;
	key.dsize = tmp_size;
        check_dbmobject_open(dp);
	val = dbm_fetch(dp->di_dbm, key);
	if (val.dptr != NULL)
		return PyString_FromStringAndSize(val.dptr, val.dsize);
	if (defvalue == NULL) {
		defvalue = PyString_FromStringAndSize(NULL, 0);
		if (defvalue == NULL)
			return NULL;
	}
	else
		Py_INCREF(defvalue);
	val.dptr = PyString_AS_STRING(defvalue);
	val.dsize = PyString_GET_SIZE(defvalue);
	if (dbm_store(dp->di_dbm, key, val, DBM_INSERT) < 0) {
		dbm_clearerr(dp->di_dbm);
		PyErr_SetString(DbmError, "cannot add item to database");
		return NULL;
	}
	return defvalue;
}

static PyMethodDef dbm_methods[] = {
	{"close",	(PyCFunction)dbm__close,	METH_NOARGS,
	 "close()\nClose the database."},
	{"keys",	(PyCFunction)dbm_keys,		METH_NOARGS,
	 "keys() -> list\nReturn a list of all keys in the database."},
	{"has_key",	(PyCFunction)dbm_has_key,	METH_VARARGS,
	 "has_key(key} -> boolean\nReturn true iff key is in the database."},
	{"get",		(PyCFunction)dbm_get,		METH_VARARGS,
	 "get(key[, default]) -> value\n"
	 "Return the value for key if present, otherwise default."},
	{"setdefault",	(PyCFunction)dbm_setdefault,	METH_VARARGS,
	 "setdefault(key[, default]) -> value\n"
	 "Return the value for key if present, otherwise default.  If key\n"
	 "is not in the database, it is inserted with default as the value."},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
dbm_getattr(dbmobject *dp, char *name)
{
	return Py_FindMethod(dbm_methods, (PyObject *)dp, name);
}

static PyTypeObject Dbmtype = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"dbm.dbm",
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
dbmopen(PyObject *self, PyObject *args)
{
	char *name;
	char *flags = "r";
	int iflags;
	int mode = 0666;

        if ( !PyArg_ParseTuple(args, "s|si:open", &name, &flags, &mode) )
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
				"arg 2 to open should be 'r', 'w', 'c', or 'n'");
		return NULL;
	}
        return newdbmobject(name, iflags, mode);
}

static PyMethodDef dbmmodule_methods[] = {
	{ "open", (PyCFunction)dbmopen, METH_VARARGS,
	  "open(path[, flag[, mode]]) -> mapping\n"
	  "Return a database object."},
	{ 0, 0 },
};

PyMODINIT_FUNC
initdbm(void) {
	PyObject *m, *d, *s;

	Dbmtype.ob_type = &PyType_Type;
	m = Py_InitModule("dbm", dbmmodule_methods);
	if (m == NULL)
		return;
	d = PyModule_GetDict(m);
	if (DbmError == NULL)
		DbmError = PyErr_NewException("dbm.error", NULL, NULL);
	s = PyString_FromString(which_dbm);
	if (s != NULL) {
		PyDict_SetItemString(d, "library", s);
		Py_DECREF(s);
	}
	if (DbmError != NULL)
		PyDict_SetItemString(d, "error", DbmError);
}
