/*
** macglue - A couple of mac-specific routines often needed.
**
** Jack Jansen, CWI, 1994.
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
	Handle h;
	
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

#if 0
	SystemTask();
	if ( intrcheck() )
		return 0;
	if ( GetNextEvent(0xffff, &ev) ) {
		if ( ev.what == mouseDown ) {
			if ( FindWindow(ev.where, &wp) == inSysWindow )
				SystemClick(&ev, wp);
		}
	}
#endif
	return 1;
}

