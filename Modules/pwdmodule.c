
/* UNIX password file access module */

#include "Python.h"

#include <sys/types.h>
#include <pwd.h>

static char pwd__doc__ [] = "\
This module provides access to the Unix password database.\n\
It is available on all Unix versions.\n\
\n\
Password database entries are reported as 7-tuples containing the following\n\
items from the password database (see `<pwd.h>'), in order:\n\
pw_name, pw_passwd, pw_uid, pw_gid, pw_gecos, pw_dir, pw_shell.\n\
The uid and gid items are integers, all others are strings. An\n\
exception is raised if the entry asked for cannot be found.";

      
static PyObject *
mkpwent(struct passwd *p)
{
	return Py_BuildValue(
		"(ssllsss)",
		p->pw_name,
		p->pw_passwd,
#if defined(NeXT) && defined(_POSIX_SOURCE) && defined(__LITTLE_ENDIAN__)
/* Correct a bug present on Intel machines in NextStep 3.2 and 3.3;
   for later versions you may have to remove this */
		(long)p->pw_short_pad1,	     /* ugh-NeXT broke the padding */
		(long)p->pw_short_pad2,
#else
		(long)p->pw_uid,
		(long)p->pw_gid,
#endif
		p->pw_gecos,
		p->pw_dir,
		p->pw_shell);
}

static char pwd_getpwuid__doc__[] = "\
getpwuid(uid) -> entry\n\
Return the password database entry for the given numeric user ID.\n\
See pwd.__doc__ for more on password database entries.";

static PyObject *
pwd_getpwuid(PyObject *self, PyObject *args)
{
	int uid;
	struct passwd *p;
	if (!PyArg_Parse(args, "i", &uid))
		return NULL;
	if ((p = getpwuid(uid)) == NULL) {
		PyErr_SetString(PyExc_KeyError, "getpwuid(): uid not found");
		return NULL;
	}
	return mkpwent(p);
}

static char pwd_getpwnam__doc__[] = "\
getpwnam(name) -> entry\n\
Return the password database entry for the given user name.\n\
See pwd.__doc__ for more on password database entries.";

static PyObject *
pwd_getpwnam(PyObject *self, PyObject *args)
{
	char *name;
	struct passwd *p;
	if (!PyArg_Parse(args, "s", &name))
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
pwd_getpwall(PyObject *self, PyObject *args)
{
	PyObject *d;
	struct passwd *p;
	if (!PyArg_NoArgs(args))
		return NULL;
	if ((d = PyList_New(0)) == NULL)
		return NULL;
	setpwent();
	while ((p = getpwent()) != NULL) {
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
	{"getpwuid",	pwd_getpwuid, METH_OLDARGS, pwd_getpwuid__doc__},
	{"getpwnam",	pwd_getpwnam, METH_OLDARGS, pwd_getpwnam__doc__},
#ifdef HAVE_GETPWENT
	{"getpwall",	pwd_getpwall, METH_OLDARGS, pwd_getpwall__doc__},
#endif
	{NULL,		NULL}		/* sentinel */
};

DL_EXPORT(void)
initpwd(void)
{
	Py_InitModule4("pwd", pwd_methods, pwd__doc__,
                       (PyObject *)NULL, PYTHON_API_VERSION);
}
