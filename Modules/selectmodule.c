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

/* select - Module containing unix select(2) call.
   Under Unix, the file descriptors are small integers.
   Under Win32, select only exists for sockets, and sockets may
   have any value except INVALID_SOCKET.
*/

#include "Python.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef __sgi
/* This is missing from unistd.h */
extern void bzero();
#endif

#include <sys/types.h>

#ifdef MS_WINDOWS
#include <winsock.h>
#else
#include "myselect.h" /* Also includes mytime.h */
#define SOCKET int
#endif

static PyObject *SelectError;

typedef struct {	/* list of Python objects and their file descriptor */
	PyObject *obj;
	SOCKET fd;
} pylist;

/* returns -1 and sets the Python exception if an error occurred, otherwise
   returns a number >= 0
*/
static int
list2set(list, set, fd2obj)
	PyObject *list;
	fd_set *set;
	pylist fd2obj[FD_SETSIZE + 3];
{
	int i, len, index, max = -1;
	PyObject *o, *fno;
	PyObject *meth;
	SOCKET v;

	index = 0;
	fd2obj[0].obj = (PyObject*)0;	     /* set list to zero size */
    
	FD_ZERO(set);
	len = PyList_Size(list);
	for (i=0; i<len; i++)  {
		o = PyList_GetItem(list, i);
		if (PyInt_Check(o)) {
			v = PyInt_AsLong(o);
		}
		else if ((meth = PyObject_GetAttrString(o, "fileno")) != NULL)
		{
			fno = PyEval_CallObject(meth, NULL);
			Py_DECREF(meth);
			if (fno == NULL)
				return -1;
                        if (!PyInt_Check(fno)) {
                            PyErr_SetString(PyExc_TypeError,
                                       "fileno method returned a non-integer");
                            Py_DECREF(fno);
                            return -1;
                        }
                        v = PyInt_AsLong(fno);
			Py_DECREF(fno);
		}
		else {
			PyErr_SetString(PyExc_TypeError,
			  "argument must be an int, or have a fileno() method."
				);
			return -1;
		}
#ifdef _MSC_VER
		max = 0;	/* not used for Win32 */
#else
		if (v < 0 || v >= FD_SETSIZE) {
			PyErr_SetString(
				PyExc_ValueError,
				"filedescriptor out of range in select()");
			return -1;
		}
		if (v > max)
			max = v;
#endif
		FD_SET(v, set);
		/* add object and its file descriptor to the list */
		if (index >= FD_SETSIZE) {
			PyErr_SetString(
				PyExc_ValueError,
				"too many file descriptors in select()");
			return -1;
		}
		fd2obj[index].obj = o;
		fd2obj[index].fd = v;
		fd2obj[++index].obj = (PyObject *)0; /* sentinel */
	}
	return max+1;
}

/* returns NULL and sets the Python exception if an error occurred */
static PyObject *
set2list(set, fd2obj)
	fd_set *set;
	pylist fd2obj[FD_SETSIZE + 3];
{
	int j, num=0;
	PyObject *list, *o;
	SOCKET fd;

	for (j=0; fd2obj[j].obj; j++)
		if (FD_ISSET(fd2obj[j].fd, set))
			num++;

	list = PyList_New(num);
	if (!list)
		return NULL;

	num = 0;
	for (j=0; fd2obj[j].obj; j++) {
		fd = fd2obj[j].fd;
		if (FD_ISSET(fd, set)) {
#ifndef _MSC_VER
			if (fd > FD_SETSIZE) {
				PyErr_SetString(PyExc_SystemError,
			   "filedescriptor out of range returned in select()");
				return NULL;
			}
#endif
			o = fd2obj[j].obj;
			Py_INCREF(o);
			if (PyList_SetItem(list, num, o) < 0) {
				Py_DECREF(list);
				return NULL;
			}
			num++;
		}
	}
	return list;
}
    
static PyObject *
select_select(self, args)
	PyObject *self;
	PyObject *args;
{
	pylist rfd2obj[FD_SETSIZE + 3];
	pylist wfd2obj[FD_SETSIZE + 3];
	pylist efd2obj[FD_SETSIZE + 3];
	PyObject *ifdlist, *ofdlist, *efdlist;
	PyObject *ret;
	PyObject *tout = Py_None;
	fd_set ifdset, ofdset, efdset;
	double timeout;
	struct timeval tv, *tvp;
	int seconds;
	int imax, omax, emax, max;
	int n;

	/* convert arguments */
	if (!PyArg_ParseTuple(args, "OOO|O",
			      &ifdlist, &ofdlist, &efdlist, &tout))
		return NULL;

	if (tout == Py_None)
		tvp = (struct timeval *)0;
	else if (!PyArg_Parse(tout, "d;timeout must be float or None",
			      &timeout))
		return NULL;
	else {
		seconds = (int)timeout;
		timeout = timeout - (double)seconds;
		tv.tv_sec = seconds;
		tv.tv_usec = (int)(timeout*1000000.0);
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

	/* Convert lists to fd_sets, and get maximum fd number
	 * propagates the Python exception set in list2set()
	 */
	if ((imax=list2set(ifdlist, &ifdset, rfd2obj)) < 0) 
		return NULL;
	if ((omax=list2set(ofdlist, &ofdset, wfd2obj)) < 0) 
		return NULL;
	if ((emax=list2set(efdlist, &efdset, efd2obj)) < 0) 
		return NULL;
	max = imax;
	if (omax > max) max = omax;
	if (emax > max) max = emax;

	Py_BEGIN_ALLOW_THREADS
	n = select(max, &ifdset, &ofdset, &efdset, tvp);
	Py_END_ALLOW_THREADS

	if (n < 0) {
		PyErr_SetFromErrno(SelectError);
		return NULL;
	}

	if (n == 0) {			     /* Speedup hack */
		ifdlist = PyList_New(0);
		if (!ifdlist)
			return NULL;
		ret = Py_BuildValue("OOO", ifdlist, ifdlist, ifdlist);
		Py_XDECREF(ifdlist);
		return ret;
	}

	/* any of these three calls can raise an exception.  it's more
	   convenient to test for this after all three calls... but is that
	   acceptable?
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
	return ret;
}


static PyMethodDef select_methods[] = {
    {"select",	select_select, 1},
    {0,  	0},			     /* sentinel */
};


void
initselect()
{
	PyObject *m, *d;
	m = Py_InitModule("select", select_methods);
	d = PyModule_GetDict(m);
	SelectError = PyString_FromString("select.error");
	PyDict_SetItemString(d, "error", SelectError);
	if (PyErr_Occurred())
		Py_FatalError("Cannot initialize select module");
}
