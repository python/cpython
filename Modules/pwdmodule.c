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
mkpwent(p)
	struct passwd *p;
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
pwd_getpwuid(self, args)
	PyObject *self;
	PyObject *args;
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
pwd_getpwnam(self, args)
	PyObject *self;
	PyObject *args;
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
pwd_getpwall(self, args)
	PyObject *self;
	PyObject *args;
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
	return d;
}
#endif

static PyMethodDef pwd_methods[] = {
	{"getpwuid",	pwd_getpwuid, 0, pwd_getpwuid__doc__},
	{"getpwnam",	pwd_getpwnam, 0, pwd_getpwnam__doc__},
#ifdef HAVE_GETPWENT
	{"getpwall",	pwd_getpwall, 0, pwd_getpwall__doc__},
#endif
	{NULL,		NULL}		/* sentinel */
};

void
initpwd()
{
	Py_InitModule4("pwd", pwd_methods, pwd__doc__,
                       (PyObject *)NULL, PYTHON_API_VERSION);
}
