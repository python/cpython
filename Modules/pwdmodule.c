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
	{"getpwuid",	pwd_getpwuid},
	{"getpwnam",	pwd_getpwnam},
#ifdef HAVE_GETPWENT
	{"getpwall",	pwd_getpwall},
#endif
	{NULL,		NULL}		/* sentinel */
};

void
initpwd()
{
	Py_InitModule("pwd", pwd_methods);
}
