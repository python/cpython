/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* select - Module containing unix select(2) call.
   Under Unix, the file descriptors are small integers.
   Under Win32, select only exists for sockets, and sockets may
   have any value except INVALID_SOCKET.
   Under BeOS, we suffer the same dichotomy as Win32; sockets can be anything
   >= 0.
*/

#include "Python.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef __sgi
/* This is missing from unistd.h */
extern void bzero(void *, int);
#endif

#ifndef DONT_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if defined(PYOS_OS2)
#include <sys/time.h>
#include <utils.h>
#endif

#ifdef MS_WINDOWS
#include <winsock.h>
#else
#ifdef __BEOS__
#include <net/socket.h>
#define SOCKET int
#else
#include "myselect.h" /* Also includes mytime.h */
#define SOCKET int
#endif
#endif

static PyObject *SelectError;

/* list of Python objects and their file descriptor */
typedef struct {
	PyObject *obj;			     /* owned reference */
	SOCKET fd;
	int sentinel;			     /* -1 == sentinel */
} pylist;

static void
reap_obj(pylist fd2obj[FD_SETSIZE + 3])
{
	int i;
	for (i = 0; i < FD_SETSIZE + 3 && fd2obj[i].sentinel >= 0; i++) {
		Py_XDECREF(fd2obj[i].obj);
		fd2obj[i].obj = NULL;
	}
	fd2obj[0].sentinel = -1;
}


/* returns -1 and sets the Python exception if an error occurred, otherwise
   returns a number >= 0
*/
static int
list2set(PyObject *list, fd_set *set, pylist fd2obj[FD_SETSIZE + 3])
{
	int i;
	int max = -1;
	int index = 0;
	int len = PyList_Size(list);
	PyObject* o = NULL;

	fd2obj[0].obj = (PyObject*)0;	     /* set list to zero size */
	FD_ZERO(set);

	for (i = 0; i < len; i++)  {
		SOCKET v;

		/* any intervening fileno() calls could decr this refcnt */
		if (!(o = PyList_GetItem(list, i)))
                    return -1;

		Py_INCREF(o);
		v = PyObject_AsFileDescriptor( o );
		if (v == -1) goto finally;

#if defined(_MSC_VER)
		max = 0;		     /* not used for Win32 */
#else  /* !_MSC_VER */
		if (v < 0 || v >= FD_SETSIZE) {
			PyErr_SetString(PyExc_ValueError,
				    "filedescriptor out of range in select()");
			goto finally;
		}
		if (v > max)
			max = v;
#endif /* _MSC_VER */
		FD_SET(v, set);

		/* add object and its file descriptor to the list */
		if (index >= FD_SETSIZE) {
			PyErr_SetString(PyExc_ValueError,
				      "too many file descriptors in select()");
			goto finally;
		}
		fd2obj[index].obj = o;
		fd2obj[index].fd = v;
		fd2obj[index].sentinel = 0;
		fd2obj[++index].sentinel = -1;
	}
	return max+1;

  finally:
	Py_XDECREF(o);
	return -1;
}

/* returns NULL and sets the Python exception if an error occurred */
static PyObject *
set2list(fd_set *set, pylist fd2obj[FD_SETSIZE + 3])
{
	int i, j, count=0;
	PyObject *list, *o;
	SOCKET fd;

	for (j = 0; fd2obj[j].sentinel >= 0; j++) {
		if (FD_ISSET(fd2obj[j].fd, set))
			count++;
	}
	list = PyList_New(count);
	if (!list)
		return NULL;

	i = 0;
	for (j = 0; fd2obj[j].sentinel >= 0; j++) {
		fd = fd2obj[j].fd;
		if (FD_ISSET(fd, set)) {
#ifndef _MSC_VER
			if (fd > FD_SETSIZE) {
				PyErr_SetString(PyExc_SystemError,
			   "filedescriptor out of range returned in select()");
				goto finally;
			}
#endif
			o = fd2obj[j].obj;
			fd2obj[j].obj = NULL;
			/* transfer ownership */
			if (PyList_SetItem(list, i, o) < 0)
				goto finally;

			i++;
		}
	}
	return list;
  finally:
	Py_DECREF(list);
	return NULL;
}

    
static PyObject *
select_select(PyObject *self, PyObject *args)
{
#ifdef MS_WINDOWS
	/* This would be an awful lot of stack space on Windows! */
	pylist *rfd2obj, *wfd2obj, *efd2obj;
#else
	pylist rfd2obj[FD_SETSIZE + 3];
	pylist wfd2obj[FD_SETSIZE + 3];
	pylist efd2obj[FD_SETSIZE + 3];
#endif
	PyObject *ifdlist, *ofdlist, *efdlist;
	PyObject *ret = NULL;
	PyObject *tout = Py_None;
	fd_set ifdset, ofdset, efdset;
	double timeout;
	struct timeval tv, *tvp;
	long seconds;
	int imax, omax, emax, max;
	int n;

	/* convert arguments */
	if (!PyArg_ParseTuple(args, "OOO|O:select",
			      &ifdlist, &ofdlist, &efdlist, &tout))
		return NULL;

	if (tout == Py_None)
		tvp = (struct timeval *)0;
	else if (!PyArg_Parse(tout, "d", &timeout)) {
		PyErr_SetString(PyExc_TypeError,
				"timeout must be a float or None");
		return NULL;
	}
	else {
		if (timeout > (double)LONG_MAX) {
			PyErr_SetString(PyExc_OverflowError, "timeout period too long");
			return NULL;
		}
		seconds = (long)timeout;
		timeout = timeout - (double)seconds;
		tv.tv_sec = seconds;
		tv.tv_usec = (long)(timeout*1000000.0);
		tvp = &tv;
	}

	/* sanity check first three arguments */
	if (!PyList_Check(ifdlist) ||
	    !PyList_Check(ofdlist) ||
	    !PyList_Check(efdlist))
	{
		PyErr_SetString(PyExc_TypeError,
				"arguments 1-3 must be lists");
		return NULL;
	}

#ifdef MS_WINDOWS
	/* Allocate memory for the lists */
	rfd2obj = PyMem_NEW(pylist, FD_SETSIZE + 3);
	wfd2obj = PyMem_NEW(pylist, FD_SETSIZE + 3);
	efd2obj = PyMem_NEW(pylist, FD_SETSIZE + 3);
	if (rfd2obj == NULL || wfd2obj == NULL || efd2obj == NULL) {
		if (rfd2obj) PyMem_DEL(rfd2obj);
		if (wfd2obj) PyMem_DEL(wfd2obj);
		if (efd2obj) PyMem_DEL(efd2obj);
		return NULL;
	}
#endif
	/* Convert lists to fd_sets, and get maximum fd number
	 * propagates the Python exception set in list2set()
	 */
	rfd2obj[0].sentinel = -1;
	wfd2obj[0].sentinel = -1;
	efd2obj[0].sentinel = -1;
	if ((imax=list2set(ifdlist, &ifdset, rfd2obj)) < 0) 
		goto finally;
	if ((omax=list2set(ofdlist, &ofdset, wfd2obj)) < 0) 
		goto finally;
	if ((emax=list2set(efdlist, &efdset, efd2obj)) < 0) 
		goto finally;
	max = imax;
	if (omax > max) max = omax;
	if (emax > max) max = emax;

	Py_BEGIN_ALLOW_THREADS
	n = select(max, &ifdset, &ofdset, &efdset, tvp);
	Py_END_ALLOW_THREADS

	if (n < 0) {
		PyErr_SetFromErrno(SelectError);
	}
	else if (n == 0) {
                /* optimization */
		ifdlist = PyList_New(0);
		if (ifdlist) {
			ret = Py_BuildValue("OOO", ifdlist, ifdlist, ifdlist);
			Py_DECREF(ifdlist);
		}
	}
	else {
		/* any of these three calls can raise an exception.  it's more
		   convenient to test for this after all three calls... but
		   is that acceptable?
		*/
		ifdlist = set2list(&ifdset, rfd2obj);
		ofdlist = set2list(&ofdset, wfd2obj);
		efdlist = set2list(&efdset, efd2obj);
		if (PyErr_Occurred())
			ret = NULL;
		else
			ret = Py_BuildValue("OOO", ifdlist, ofdlist, efdlist);

		Py_DECREF(ifdlist);
		Py_DECREF(ofdlist);
		Py_DECREF(efdlist);
	}
	
  finally:
	reap_obj(rfd2obj);
	reap_obj(wfd2obj);
	reap_obj(efd2obj);
#ifdef MS_WINDOWS
	PyMem_DEL(rfd2obj);
	PyMem_DEL(wfd2obj);
	PyMem_DEL(efd2obj);
#endif
	return ret;
}

static char select_doc[] =
"select(rlist, wlist, xlist[, timeout]) -> (rlist, wlist, xlist)\n\
\n\
Wait until one or more file descriptors are ready for some kind of I/O.\n\
The first three arguments are lists of file descriptors to be waited for:\n\
rlist -- wait until ready for reading\n\
wlist -- wait until ready for writing\n\
xlist -- wait for an ``exceptional condition''\n\
If only one kind of condition is required, pass [] for the other lists.\n\
A file descriptor is either a socket or file object, or a small integer\n\
gotten from a fileno() method call on one of those.\n\
\n\
The optional 4th argument specifies a timeout in seconds; it may be\n\
a floating point number to specify fractions of seconds.  If it is absent\n\
or None, the call will never time out.\n\
\n\
The return value is a tuple of three lists corresponding to the first three\n\
arguments; each contains the subset of the corresponding file descriptors\n\
that are ready.\n\
\n\
*** IMPORTANT NOTICE ***\n\
On Windows, only sockets are supported; on Unix, all file descriptors.";


static PyMethodDef select_methods[] = {
    {"select",	select_select, 1, select_doc},
    {0,  	0},			     /* sentinel */
};

static char module_doc[] =
"This module supports asynchronous I/O on multiple file descriptors.\n\
\n\
*** IMPORTANT NOTICE ***\n\
On Windows, only sockets are supported; on Unix, all file descriptors.";

DL_EXPORT(void)
initselect(void)
{
	PyObject *m, *d;
	m = Py_InitModule3("select", select_methods, module_doc);
	d = PyModule_GetDict(m);
	SelectError = PyErr_NewException("select.error", NULL, NULL);
	PyDict_SetItemString(d, "error", SelectError);
}
