
/* UNIX password file access module */

#include "Python.h"
#include "structseq.h"

#include <sys/types.h>
#include <pwd.h>

static PyStructSequence_Field struct_pwd_type_fields[] = {
	{"pw_name", "user name"},
	{"pw_passwd", "password"},
	{"pw_uid", "user id"},
	{"pw_gid", "group id"}, 
	{"pw_gecos", "real name"}, 
	{"pw_dir", "home directory"},
	{"pw_shell", "shell program"},
	{0}
};

static char struct_passwd__doc__[] =
"pwd.struct_passwd: Results from getpw*() routines.\n\n\
This object may be accessed either as a tuple of\n\
  (pw_name,pw_passwd,pw_uid,pw_gid,pw_gecos,pw_dir,pw_shell)\n\
or via the object attributes as named in the above tuple.\n";

static PyStructSequence_Desc struct_pwd_type_desc = {
	"pwd.struct_passwd",
	struct_passwd__doc__,
	struct_pwd_type_fields,
	7,
};

static char pwd__doc__ [] = "\
This module provides access to the Unix password database.\n\
It is available on all Unix versions.\n\
\n\
Password database entries are reported as 7-tuples containing the following\n\
items from the password database (see `<pwd.h>'), in order:\n\
pw_name, pw_passwd, pw_uid, pw_gid, pw_gecos, pw_dir, pw_shell.\n\
The uid and gid items are integers, all others are strings. An\n\
exception is raised if the entry asked for cannot be found.";

      
static PyTypeObject StructPwdType;

static PyObject *
mkpwent(struct passwd *p)
{
	int setIndex = 0;
	PyObject *v = PyStructSequence_New(&StructPwdType);
	if (v == NULL)
		return NULL;

#define SETI(i,val) PyStructSequence_SET_ITEM(v, i, PyInt_FromLong((long) val))
#define SETS(i,val) PyStructSequence_SET_ITEM(v, i, PyString_FromString(val))

	SETS(setIndex++, p->pw_name);
	SETS(setIndex++, p->pw_passwd);
	SETI(setIndex++, p->pw_uid);
	SETI(setIndex++, p->pw_gid);
	SETS(setIndex++, p->pw_gecos);
	SETS(setIndex++, p->pw_dir);
	SETS(setIndex++, p->pw_shell);

#undef SETS
#undef SETI

	if (PyErr_Occurred()) {
		Py_XDECREF(v);
		return NULL;
	}

	return v;
}

static char pwd_getpwuid__doc__[] = "\
getpwuid(uid) -> (pw_name,pw_passwd,pw_uid,pw_gid,pw_gecos,pw_dir,pw_shell)\n\
Return the password database entry for the given numeric user ID.\n\
See pwd.__doc__ for more on password database entries.";

static PyObject *
pwd_getpwuid(PyObject *self, PyObject *args)
{
	int uid;
	struct passwd *p;
	if (!PyArg_ParseTuple(args, "i:getpwuid", &uid))
		return NULL;
	if ((p = getpwuid(uid)) == NULL) {
		PyErr_SetString(PyExc_KeyError, "getpwuid(): uid not found");
		return NULL;
	}
	return mkpwent(p);
}

static char pwd_getpwnam__doc__[] = "\
getpwnam(name) -> (pw_name,pw_passwd,pw_uid,pw_gid,pw_gecos,pw_dir,pw_shell)\n\
Return the password database entry for the given user name.\n\
See pwd.__doc__ for more on password database entries.";

static PyObject *
pwd_getpwnam(PyObject *self, PyObject *args)
{
	char *name;
	struct passwd *p;
	if (!PyArg_ParseTuple(args, "s:getpwnam", &name))
		return NULL;
	if ((p = getpwnam(name)) == NULL) {
		PyErr_SetString(PyExc_KeyError, "getpwnam(): name not found");
		return NULL;
	}
	return mkpwent(p);
}

#ifdef HAVE_GETPWENT
static char pwd_getpwall__doc__[] = "\
getpwall() -> list_of_entries\n\
Return a list of all available password database entries, \
in arbitrary order.\n\
See pwd.__doc__ for more on password database entries.";

static PyObject *
pwd_getpwall(PyObject *self)
{
	PyObject *d;
	struct passwd *p;
	if ((d = PyList_New(0)) == NULL)
		return NULL;
#if defined(PYOS_OS2) && defined(PYCC_GCC)
	if ((p = getpwuid(0)) != NULL) {
#else
	setpwent();
	while ((p = getpwent()) != NULL) {
#endif
		PyObject *v = mkpwent(p);
		if (v == NULL || PyList_Append(d, v) != 0) {
			Py_XDECREF(v);
			Py_DECREF(d);
			return NULL;
		}
		Py_DECREF(v);
	}
	endpwent();
	return d;
}
#endif

static PyMethodDef pwd_methods[] = {
	{"getpwuid",	pwd_getpwuid, METH_VARARGS, pwd_getpwuid__doc__},
	{"getpwnam",	pwd_getpwnam, METH_VARARGS, pwd_getpwnam__doc__},
#ifdef HAVE_GETPWENT
	{"getpwall",	(PyCFunction)pwd_getpwall,
		METH_NOARGS,  pwd_getpwall__doc__},
#endif
	{NULL,		NULL}		/* sentinel */
};

DL_EXPORT(void)
initpwd(void)
{
	PyObject *m;
	m = Py_InitModule3("pwd", pwd_methods, pwd__doc__);

	PyStructSequence_InitType(&StructPwdType, &struct_pwd_type_desc);
	Py_INCREF((PyObject *) &StructPwdType);
	PyModule_AddObject(m, "struct_pwent", (PyObject *) &StructPwdType);
}
