/*
** macglue - A couple of mac-specific routines often needed.
**
** Jack Jansen, CWI, 1994.
** Some routines by Guido, moved here from macosmodule.c
** (since they are useable by other modules as well).
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Python.h"
#include "macglue.h"

#include <OSUtils.h> /* for Set(Current)A5 */
#include <Resources.h>
#include <Memory.h>
#include <Events.h>
#include <Windows.h>
#include <Desk.h>

/* We should include Errors.h here, but it has a name conflict
** with the python errors.h. */
#define fnfErr -43

/* Convert C to Pascal string. Returns pointer to static buffer. */
unsigned char *
Pstring(char *str)
{
	static Str255 buf;
	int len;

	len = strlen(str);
	buf[0] = (unsigned char)len;
	strncpy((char *)buf+1, str, len);
	return buf;
}

/* Replace strerror with something that might work */
char *macstrerror(int err)
{
	static char buf[256];
	Handle h;
	char *str;
	
	h = GetResource('Estr', err);
	if ( h ) {
		HLock(h);
		str = (char *)*h;
		memcpy(buf, str+1, (unsigned char)str[0]);
		HUnlock(h);
		ReleaseResource(h);
	} else {
		sprintf(buf, "Mac OS error code %d", err);
	}
	return buf;
}

/* Set a MAC-specific error from errno, and return NULL; return None if no error */
PyObject * 
PyErr_Mac(PyObject *eobj, int err)
{
	char *msg;
	PyObject *v;
	
	if (err == 0) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	msg = macstrerror(err);
	v = Py_BuildValue("(is)", err, msg);
	PyErr_SetObject(eobj, v);
	Py_DECREF(v);
	return NULL;
}

/*
** Idle routine for busy-wait loops.
** This is rather tricky: if we see an event we check whether it is
** for somebody else (i.e. a click outside our windows) and, if so,
** we pass the event on (so the user can switch processes). However,
** by doing this we loose events meant for our windows. Too bad, I guess...
*/
int
PyMac_Idle()
{
	EventRecord ev;
	WindowPtr wp;

	SystemTask();
	if ( intrpeek() )
		return 0;
	if ( GetNextEvent(0xffff, &ev) ) {
		if ( ev.what == mouseDown ) {
			if ( FindWindow(ev.where, &wp) == inSysWindow )
				SystemClick(&ev, wp);
		}
	}
	return 1;
}


/* Convert a ResType argument */
int
GetOSType(PyObject *v, ResType *pr)
{
	if (!PyString_Check(v) || PyString_Size(v) != 4) {
		PyErr_SetString(PyExc_TypeError,
			"OSType arg must be string of 4 chars");
		return 0;
	}
	memcpy((char *)pr, PyString_AsString(v), 4);
	return 1;
}

/* Convert a Str255 argument */
int
GetStr255(PyObject *v, Str255 pbuf)
{
	int len;
	if (!PyString_Check(v) || (len = PyString_Size(v)) > 255) {
		PyErr_SetString(PyExc_TypeError,
			"Str255 arg must be string of at most 255 chars");
		return 0;
	}
	pbuf[0] = len;
	memcpy((char *)(pbuf+1), PyString_AsString(v), len);
	return 1;
}

/*
** Convert anything resembling an FSSpec argument
** NOTE: This routine will fail on pre-sys7 machines. 
** The caller is responsible for not calling this routine
** in those cases (which is fine, since everyone calling
** this is probably sys7 dependent anyway).
*/
int
GetFSSpec(PyObject *v, FSSpec *fs)
{
	Str255 path;
	short refnum;
	long parid;
	OSErr err;

	if ( PyString_Check(v) ) {
		/* It's a pathname */
		if( !PyArg_Parse(v, "O&", GetStr255, &path) )
			return 0;
		refnum = 0; /* XXXX Should get CurWD here... */
		parid = 0;
	} else {
		PyErr_Clear();
		if( !PyArg_Parse(v, "(hlO&); FSSpec should be fullpath or (int,int,string)",
							&refnum, &parid, GetStr255, &path))
			return 0;
	}
	err = FSMakeFSSpec(refnum, parid, path, fs);
	if ( err && err != fnfErr ) {
		PyErr_SetString(PyExc_TypeError,
			"FSMakeFSSpec error");
		return 0;
	}
	return 1;
}

/*
** Return a python object that describes an FSSpec
*/
PyObject *
PyMac_BuildFSSpec(FSSpec *fs)
{
	return Py_BuildValue("(iis#)", fs->vRefNum, fs->parID, &fs->name[1], fs->name[0]);
}
