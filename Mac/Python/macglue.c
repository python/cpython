/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

#include "Python.h"

#include "macglue.h"
#include "marshal.h"
#include "import.h"

#include "pythonresources.h"

#include <OSUtils.h> /* for Set(Current)A5 */
#include <Files.h>
#include <StandardFile.h>
#include <Resources.h>
#include <Memory.h>
#include <Events.h>
#include <Windows.h>
#include <Desk.h>
#include <Traps.h>
#include <Processes.h>
#include <Fonts.h>
#include <Menus.h>
#include <TextUtils.h>
#ifdef THINK_C
#include <OSEvents.h> /* For EvQElPtr */
#endif
#ifdef __MWERKS__
#include <SIOUX.h>
#endif

#ifndef HAVE_UNIVERSAL_HEADERS
#define GetResourceSizeOnDisk(x) SizeResource(x)
typedef DlgHookYDProcPtr DlgHookYDUPP;
#define NewDlgHookYDProc(userRoutine) ((DlgHookYDUPP) (userRoutine))
typedef FileFilterYDProcPtr FileFilterYDUPP;
#endif

#include <signal.h>
#include <stdio.h>

/*
** We have to be careful, since we can't handle
** things like updates (and they'll keep coming back if we don't
** handle them). Note that we don't know who has windows open, so
** even handing updates off to SIOUX under MW isn't going to work.
*/
#define MAINLOOP_EVENTMASK (mDownMask|keyDownMask|osMask)

#include <signal.h>

/* XXX We should include Errors.h here, but it has a name conflict
** with the python errors.h. */
#define fnfErr -43

/* Declared in macfsmodule.c: */
extern FSSpec *mfs_GetFSSpecFSSpec();

/* Interrupt code variables: */
static int interrupted;			/* Set to true when cmd-. seen */
static RETSIGTYPE intcatcher Py_PROTO((int));

/*
** We attempt to be a good citizen by giving up the CPU periodically.
** When in the foreground we do this less often and for shorter periods
** than when in the background. At this time we also check for events and
** pass them off to SIOUX, if compiling with mwerks.
** The counts here are in ticks of 1/60th second.
** XXXX The initial values here are not based on anything.
** FG-python gives up the cpu for 1/60th 5 times per second,
** BG-python for .2 second 10 times per second.
*/
static long interval_fg = 12;
static long interval_bg = 6;
static long yield_fg = 1;
static long yield_bg = 12;
static long lastyield;
static int in_foreground;

/* 
** When > 0, do full scanning for events (program is not event aware)
** when == 0, only scan for Command-period
** when < 0, don't do any event scanning 
*/
int PyMac_DoYieldEnabled = 1;

/*
** Some stuff for our GetDirectory and PromptGetFile routines
*/
struct hook_args {
	int selectcur_hit;		/* Set to true when "select current" selected */
	char *prompt;			/* The prompt */
};
static DlgHookYDUPP myhook_upp;
static int upp_inited = 0;


/* Convert C to Pascal string. Returns pointer to static buffer. */
unsigned char *
Pstring(char *str)
{
	static Str255 buf;
	int len;

	len = strlen(str);
	if (len > 255)
		len = 255;
	buf[0] = (unsigned char)len;
	strncpy((char *)buf+1, str, len);
	return buf;
}

/* Like strerror() but for Mac OS error numbers */
char *PyMac_StrError(int err)
{
	static char buf[256];
	Handle h;
	char *str;
	
	h = GetResource('Estr', err);
	if ( h ) {
		HLock(h);
		str = (char *)*h;
		memcpy(buf, str+1, (unsigned char)str[0]);
		buf[(unsigned char)str[0]] = '\0';
		HUnlock(h);
		ReleaseResource(h);
	} else {
		sprintf(buf, "Mac OS error code %d", err);
	}
	return buf;
}

/* Exception object shared by all Mac specific modules for Mac OS errors */
PyObject *PyMac_OSErrException;

/* Initialize and return PyMac_OSErrException */
PyObject *
PyMac_GetOSErrException()
{
	if (PyMac_OSErrException == NULL)
		PyMac_OSErrException = PyString_FromString("Mac OS Error");
	return PyMac_OSErrException;
}

/* Set a MAC-specific error from errno, and return NULL; return None if no error */
PyObject * 
PyErr_Mac(PyObject *eobj, int err)
{
	char *msg;
	PyObject *v;
	
	if (err == 0 && !PyErr_Occurred()) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (err == -1 && PyErr_Occurred())
		return NULL;
	msg = PyMac_StrError(err);
	v = Py_BuildValue("(is)", err, msg);
	PyErr_SetObject(eobj, v);
	Py_DECREF(v);
	return NULL;
}

/* Call PyErr_Mac with PyMac_OSErrException */
PyObject *
PyMac_Error(OSErr err)
{
	return PyErr_Mac(PyMac_GetOSErrException(), err);
}

/* The catcher routine (which may not be used for all compilers) */
static RETSIGTYPE
intcatcher(sig)
	int sig;
{
	interrupted = 1;
	signal(SIGINT, intcatcher);
}

void
PyOS_InitInterrupts()
{
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, intcatcher);
}

/*
** This routine scans the event queue looking for cmd-.
** This is the only way to get an interrupt under THINK (since it
** doesn't do SIGINT handling), but is also used under MW, when
** the full-fledged event loop is disabled. This way, we can at least
** interrupt a runaway python program.
*/
static void
scan_event_queue(flush)
	int flush;
{
#if defined(__MWERKS__) && defined(__CFM68K__)
	return; /* No GetEvQHdr yet */
#else
	register EvQElPtr q;
	
	q = (EvQElPtr) GetEvQHdr()->qHead;
	
	for (; q; q = (EvQElPtr)q->qLink) {
		if (q->evtQWhat == keyDown &&
				(char)q->evtQMessage == '.' &&
				(q->evtQModifiers & cmdKey) != 0) {
			if ( flush )
				FlushEvents(keyDownMask, 0);
			interrupted = 1;
			break;
		}
	}
#endif
}

int
PyOS_InterruptOccurred()
{
	if (PyMac_DoYieldEnabled < 0)
		return 0;
#ifdef THINK_C
	scan_event_queue(1);
#endif
	PyMac_Yield();
	if (interrupted) {
		interrupted = 0;
		return 1;
	}
	return 0;
}

/* intrpeek() is like intrcheck(), but it doesn't flush the events. The
** idea is that you call intrpeek() somewhere in a busy-wait loop, and return
** None as soon as it returns 1. The mainloop will then pick up the cmd-. soon
** thereafter.
*/
static int
intrpeek()
{
#ifdef THINK_C
	scan_event_queue(0);
#endif
	return interrupted;
}

/* Check whether we are in the foreground */
int
PyMac_InForeground()
{
	static ProcessSerialNumber ours;
	static inited;
	ProcessSerialNumber curfg;
	Boolean eq;
	
	if ( inited == 0 )
		(void)GetCurrentProcess(&ours);
	inited = 1;
	if ( GetFrontProcess(&curfg) < 0 )
		eq = 1;
	else if ( SameProcess(&ours, &curfg, &eq) < 0 )
		eq = 1;
	return (int)eq;

}

/*
** Set yield timeouts
*/
void
PyMac_SetYield(long fgi, long fgy, long bgi, long bgy)
{
	interval_fg = fgi;
	yield_fg = fgy;
	interval_bg = bgi;
	yield_bg = bgy;
}

/*
** Handle an event, either one found in the mainloop eventhandler or
** one passed back from the python program.
*/
void
PyMac_HandleEvent(evp)
	EventRecord *evp;
{
	WindowPtr wp;
			
#ifdef __MWERKS__
	/* If SIOUX wants it we're done */
	(void)SIOUXHandleOneEvent(evp);
#else
	/* Other compilers are just unlucky: we only weed out clicks in other applications */
	if ( evp->what == mouseDown ) {
		if ( FindWindow(evp->where, &wp) == inSysWindow )
			SystemClick(evp, wp);
	}
#endif /* !__MWERKS__ */
}

/*
** Yield the CPU to other tasks.
*/
static
PyMac_DoYield()
{
	EventRecord ev;
	long yield;
	static int no_waitnextevent = -1;
	int gotone;
	
	if ( no_waitnextevent < 0 ) {
		no_waitnextevent = (NGetTrapAddress(_WaitNextEvent, ToolTrap) ==
							NGetTrapAddress(_Unimplemented, ToolTrap));
	}

	if ( !PyMac_DoYieldEnabled ) {
#ifndef THINK_C
		/* Under think this has been done before in intrcheck() or intrpeek() */
		scan_event_queue(0);
#endif
		return;
	}
		
	in_foreground = PyMac_InForeground();
	if ( in_foreground )
		yield = yield_fg;
	else
		yield = yield_bg;
	while ( 1 ) {
		if ( no_waitnextevent ) {
			SystemTask();
			gotone = GetNextEvent(MAINLOOP_EVENTMASK, &ev);
		} else {
			gotone = WaitNextEvent(MAINLOOP_EVENTMASK, &ev, yield, NULL);
		}	
		/* Get out quickly if nothing interesting is happening */
		if ( !gotone || ev.what == nullEvent )
			break;
		PyMac_HandleEvent(&ev);
	}
	lastyield = TickCount();
}

/*
** Yield the CPU to other tasks if opportune
*/
void
PyMac_Yield() {
	long iv;
	
	if ( in_foreground )
		iv = interval_fg;
	else
		iv = interval_bg;
	if ( TickCount() > lastyield + iv )
		PyMac_DoYield();
}

/*
** Idle routine for busy-wait loops.
** Gives up CPU, handles events and returns true if an interrupt is pending
** (but not actually handled yet).
*/
int
PyMac_Idle()
{
	PyMac_DoYield();
	return intrpeek();
}
/*
** Returns true if the argument has a resource fork, and it contains
** a 'PYC ' resource of the correct name
*/
int
PyMac_FindResourceModule(module, filename)
char *module;
char *filename;
{
	FSSpec fss;
	FInfo finfo;
	short oldrh, filerh;
	int ok;
	Handle h;
	
	if ( FSMakeFSSpec(0, 0, Pstring(filename), &fss) != noErr )
		return 0;			/* It doesn't exist */
	if ( FSpGetFInfo(&fss, &finfo) != noErr )
		return 0;			/* shouldn't happen, I guess */
	oldrh = CurResFile();
	filerh = FSpOpenResFile(&fss, fsRdPerm);
	if ( filerh == -1 )
		return 0;
	UseResFile(filerh);
	SetResLoad(0);
	h = Get1NamedResource('PYC ', Pstring(module));
	SetResLoad(1);
	ok = (h != NULL);
	CloseResFile(filerh);
	UseResFile(oldrh);
	return ok;
}

/*
** Load the specified module from a resource
*/
PyObject *
PyMac_LoadResourceModule(module, filename)
char *module;
char *filename;
{
	FSSpec fss;
	FInfo finfo;
	short oldrh, filerh;
	int ok;
	Handle h;
	OSErr err;
	PyObject *m, *co;
	long num, size;
	
	if ( (err=FSMakeFSSpec(0, 0, Pstring(filename), &fss)) != noErr )
		goto error;
	if ( (err=FSpGetFInfo(&fss, &finfo)) != noErr )
		goto error;
	oldrh = CurResFile();
	filerh = FSpOpenResFile(&fss, fsRdPerm);
	if ( filerh == -1 ) {
		err = ResError();
		goto error;
	}
	UseResFile(filerh);
	h = Get1NamedResource('PYC ', Pstring(module));
	if ( h == NULL ) {
		err = ResError();
		goto error;
	}
	HLock(h);
	/*
	** XXXX The next few lines are intimately tied to the format of pyc
	** files. I'm not sure whether this code should be here or in import.c -- Jack
	*/
	size = GetHandleSize(h);
	if ( size < 8 ) {
		PyErr_SetString(PyExc_ImportError, "Resource too small");
		co = NULL;
	} else {
		num = (*h)[0] & 0xff;
		num = num | (((*h)[1] & 0xff) << 8);
		num = num | (((*h)[2] & 0xff) << 16);
		num = num | (((*h)[3] & 0xff) << 24);
		if ( num != PyImport_GetMagicNumber() ) {
			PyErr_SetString(PyExc_ImportError, "Bad MAGIC in resource");
			co = NULL;
		} else {
			co = PyMarshal_ReadObjectFromString((*h)+8, size-8);
		}
	}
	HUnlock(h);
	CloseResFile(filerh);
	UseResFile(oldrh);
	if ( co ) {
		m = PyImport_ExecCodeModule(module, co);
		Py_DECREF(co);
	} else {
		m = NULL;
	}
	return m;
error:
	{
		char buf[512];
		
		sprintf(buf, "%s: %s", filename, PyMac_StrError(err));
		PyErr_SetString(PyExc_ImportError, buf);
		return NULL;
	}
}

/*
** Helper routine for GetDirectory
*/
static pascal short
myhook_proc(short item, DialogPtr theDialog, struct hook_args *dataptr)
{
	if ( item == sfHookFirstCall && dataptr->prompt) {
		Handle prompth;
		short type;
		Rect rect;
		
		GetDialogItem(theDialog, PROMPT_ITEM, &type, &prompth, &rect);
		if ( prompth )
			SetDialogItemText(prompth, (unsigned char *)dataptr->prompt);
	} else
	if ( item == SELECTCUR_ITEM ) {
		item = sfItemCancelButton;
		dataptr->selectcur_hit = 1;
	}
	return item;
}	

/*
** Ask the user for a directory. I still can't understand
** why Apple doesn't provide a standard solution for this...
*/
int
PyMac_GetDirectory(dirfss, prompt)
	FSSpec *dirfss;
	char *prompt;
{
	static SFTypeList list = {'fldr', 0, 0, 0};
	static Point where = {-1, -1};
	StandardFileReply reply;
	struct hook_args hook_args;
	
	if ( !upp_inited ) {
		myhook_upp = NewDlgHookYDProc(myhook_proc);
		upp_inited = 1;
	}
	if ( prompt && *prompt )
		hook_args.prompt = (char *)Pstring(prompt);
	else
		hook_args.prompt = NULL;
	hook_args.selectcur_hit = 0;
	CustomGetFile((FileFilterYDUPP)0, 1, list, &reply, GETDIR_ID, where, myhook_upp,
				NULL, NULL, NULL, (void *)&hook_args);
				
	reply.sfFile.name[0] = 0;
	if( FSMakeFSSpec(reply.sfFile.vRefNum, reply.sfFile.parID, reply.sfFile.name, dirfss) )
		return 0;
	return hook_args.selectcur_hit;
}

/*
** Slightly extended StandardGetFile: accepts a prompt */
void PyMac_PromptGetFile(short numTypes, ConstSFTypeListPtr typeList, 
		StandardFileReply *reply, char *prompt)
{
	static Point where = {-1, -1};
	struct hook_args hook_args;
	
	if ( !upp_inited ) {
		myhook_upp = NewDlgHookYDProc(myhook_proc);
		upp_inited = 1;
	}
	if ( prompt && *prompt )
		hook_args.prompt = (char *)Pstring(prompt);
	else
		hook_args.prompt = NULL;
	hook_args.selectcur_hit = 0;
	CustomGetFile((FileFilterYDUPP)0, numTypes, typeList, reply, GETFILEPROMPT_ID, where,
				myhook_upp, NULL, NULL, NULL, (void *)&hook_args);
}

/* Convert a 4-char string object argument to an OSType value */
int
PyMac_GetOSType(PyObject *v, OSType *pr)
{
	if (!PyString_Check(v) || PyString_Size(v) != 4) {
		PyErr_SetString(PyExc_TypeError,
			"OSType arg must be string of 4 chars");
		return 0;
	}
	memcpy((char *)pr, PyString_AsString(v), 4);
	return 1;
}

/* Convert an OSType value to a 4-char string object */
PyObject *
PyMac_BuildOSType(OSType t)
{
	return PyString_FromStringAndSize((char *)&t, 4);
}


/* Convert a Python string object to a Str255 */
int
PyMac_GetStr255(PyObject *v, Str255 pbuf)
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

/* Convert a Str255 to a Python string object */
PyObject *
PyMac_BuildStr255(Str255 s)
{
	return PyString_FromStringAndSize((char *)&s[1], (int)s[0]);
}


/*
** Convert a Python object to an FSSpec.
** The object may either be a full pathname or a triple
** (vrefnum, dirid, path).
** NOTE: This routine will fail on pre-sys7 machines. 
** The caller is responsible for not calling this routine
** in those cases (which is fine, since everyone calling
** this is probably sys7 dependent anyway).
*/
int
PyMac_GetFSSpec(PyObject *v, FSSpec *fs)
{
	Str255 path;
	short refnum;
	long parid;
	OSErr err;
	FSSpec *fs2;

	/* first check whether it already is an FSSpec */
	fs2 = mfs_GetFSSpecFSSpec(v);
	if ( fs2 ) {
		(void)FSMakeFSSpec(fs2->vRefNum, fs2->parID, fs2->name, fs);
		return 1;
	}
	if ( PyString_Check(v) ) {
		/* It's a pathname */
		if( !PyArg_Parse(v, "O&", PyMac_GetStr255, &path) )
			return 0;
		refnum = 0; /* XXXX Should get CurWD here... */
		parid = 0;
	} else {
		if( !PyArg_Parse(v, "(hlO&); FSSpec should be fullpath or (vrefnum,dirid,path)",
							&refnum, &parid, PyMac_GetStr255, &path)) {
			return 0;
		}
	}
	err = FSMakeFSSpec(refnum, parid, path, fs);
	if ( err && err != fnfErr ) {
		PyErr_Mac(PyExc_ValueError, err);
		return 0;
	}
	return 1;
}



/* Convert a Python object to a Rect.
   The object must be a (left, top, right, bottom) tuple.
   (This differs from the order in the struct but is consistent with
   the arguments to SetRect(), and also with STDWIN). */
int
PyMac_GetRect(PyObject *v, Rect *r)
{
	return PyArg_Parse(v, "(hhhh)", &r->left, &r->top, &r->right, &r->bottom);
}

/* Convert a Rect to a Python object */
PyObject *
PyMac_BuildRect(Rect *r)
{
	return Py_BuildValue("(hhhh)", r->left, r->top, r->right, r->bottom);
}


/* Convert a Python object to a Point.
   The object must be a (h, v) tuple.
   (This differs from the order in the struct but is consistent with
   the arguments to SetPoint(), and also with STDWIN). */
int
PyMac_GetPoint(PyObject *v, Point *p)
{
	return PyArg_Parse(v, "(hh)", &p->h, &p->v);
}

/* Convert a Point to a Python object */
PyObject *
PyMac_BuildPoint(Point p)
{
	return Py_BuildValue("(hh)", p.h, p.v);
}


/* Convert a Python object to an EventRecord.
   The object must be a (what, message, when, (v, h), modifiers) tuple. */
int
PyMac_GetEventRecord(PyObject *v, EventRecord *e)
{
	return PyArg_Parse(v, "(hll(hh)h)",
	                   &e->what,
	                   &e->message,
	                   &e->when,
	                   &e->where.h,
	                   &e->where.v,                   
	                   &e->modifiers);
}

/* Convert a Rect to an EventRecord object */
PyObject *
PyMac_BuildEventRecord(EventRecord *e)
{
	return Py_BuildValue("(hll(hh)h)",
	                     e->what,
	                     e->message,
	                     e->when,
	                     e->where.h,
	                     e->where.v,
	                     e->modifiers);
}
