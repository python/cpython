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

/* UNIX group file access module */

#include "Python.h"

#include <sys/types.h>
#include <grp.h>

static PyObject *mkgrent(p)
	struct group *p;
{
	PyObject *v, *w;
	char **member;
	if ((w = PyList_New(0)) == NULL) {
		return NULL;
	}
	for (member = p->gr_mem; *member != NULL; member++) {
		PyObject *x = PyString_FromString(*member);
		if (x == NULL || PyList_Append(w, x) != 0) {
			Py_XDECREF(x);
			Py_DECREF(w);
			return NULL;
		}
		Py_DECREF(x);
	}
	v = Py_BuildValue("(sslO)",
		       p->gr_name,
		       p->gr_passwd,
#if defined(NeXT) && defined(_POSIX_SOURCE) && defined(__LITTLE_ENDIAN__)
/* Correct a bug present on Intel machines in NextStep 3.2 and 3.3;
   for later versions you may have to remove this */
		       (long)p->gr_short_pad, /* ugh-NeXT broke the padding */
#else
		       (long)p->gr_gid,
#endif
		       w);
	Py_DECREF(w);
	return v;
}

static PyObject *grp_getgrgid(self, args)
	PyObject *self, *args;
{
	int gid;
	struct group *p;
	if (!PyArg_Parse((args),"i",(&gid)))
		return NULL;
	if ((p = getgrgid(gid)) == NULL) {
		PyErr_SetString(PyExc_KeyError, "getgrgid(): gid not found");
		return NULL;
	}
	return mkgrent(p);
}

static PyObject *grp_getgrnam(self, args)
	PyObject *self, *args;
{
	char *name;
	struct group *p;
	if (!PyArg_Parse((args),"s",(&name)))
		return NULL;
	if ((p = getgrnam(name)) == NULL) {
		PyErr_SetString(PyExc_KeyError, "getgrnam(): name not found");
		return NULL;
	}
	return mkgrent(p);
}

static PyObject *grp_getgrall(self, args)
	PyObject *self, *args;
{
	PyObject *d;
	struct group *p;
	if (!PyArg_NoArgs(args))
		return NULL;
	if ((d = PyList_New(0)) == NULL)
		return NULL;
	setgrent();
	while ((p = getgrent()) != NULL) {
		PyObject *v = mkgrent(p);
		if (v == NULL || PyList_Append(d, v) != 0) {
			Py_XDECREF(v);
			Py_DECREF(d);
			return NULL;
		}
		Py_DECREF(v);
	}
	return d;
}

static PyMethodDef grp_methods[] = {
	{"getgrgid",	grp_getgrgid},
	{"getgrnam",	grp_getgrnam},
	{"getgrall",	grp_getgrall},
	{NULL,		NULL}		/* sentinel */
};

DL_EXPORT(void)
initgrp()
{
	Py_InitModule("grp", grp_methods);
}
