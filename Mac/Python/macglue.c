/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
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
#include "importdl.h"
#include "pymactoolbox.h"

#include "pythonresources.h"

#ifdef WITHOUT_FRAMEWORKS
#include <OSUtils.h> /* for Set(Current)A5 */
#include <Files.h>
#include <StandardFile.h>
#include <Resources.h>
#include <Memory.h>
#include <Windows.h>
#include <Traps.h>
#include <Processes.h>
#include <Fonts.h>
#include <Menus.h>
#include <TextUtils.h>
#include <LowMem.h>
#include <Events.h>
#else
#include <Carbon/Carbon.h>
#endif

#if !TARGET_API_MAC_OS8
/* Unfortunately this call is probably slower... */
#define LMGetTicks() TickCount()
#endif

#ifdef __MWERKS__
#include <SIOUX.h>
extern void SIOUXSetupMenus(void);
extern void SIOUXDoAboutBox(void);
#endif
#ifdef USE_GUSI
/* Functions we redefine because they're in obscure libraries */
extern void SpinCursor(short x);
extern void RotateCursor(short x);
extern pascal void PLstrcpy(unsigned char *, unsigned char *);
extern pascal int PLstrcmp(unsigned char *, unsigned char *);
extern pascal unsigned char *PLstrrchr(unsigned char *, unsigned char);

#endif

#ifdef USE_GUSI1
#include <TFileSpec.h> /* For Path2FSSpec */
#include <GUSI.h>
#endif

/* The ID of the Sioux apple menu */
#define SIOUX_APPLEID	32000

#include <signal.h>
#include <stdio.h>

/*
** When less than this amount of stackspace is left we
** raise a MemoryError.
*/
#ifndef MINIMUM_STACK_SIZE
#define MINIMUM_STACK_SIZE 8192
#endif

#if TARGET_API_MAC_CARBON
/*
** On MacOSX StackSpace() lies: it gives the distance from heap end to stack pointer,
** but the stack cannot grow that far due to rlimit values. We cannot get at this value
** from Carbon, so we set a maximum to the stack here that is based on the default
** stack limit of 512K.
*/
#define MAXIMUM_STACK_SIZE (256*1024)
#endif

/*
** We have to be careful, since we can't handle
** things like updates (and they'll keep coming back if we don't
** handle them). Note that we don't know who has windows open, so
** even handing updates off to SIOUX under MW isn't going to work.
*/
#define MAINLOOP_EVENTMASK (mDownMask|keyDownMask|osMask|activMask)

#include <signal.h>

/* XXX We should include Errors.h here, but it has a name conflict
** with the python errors.h. */
#define fnfErr -43

/* Interrupt code variables: */
static int interrupted;			/* Set to true when cmd-. seen */
static RETSIGTYPE intcatcher(int);

#if !TARGET_API_MAC_OSX
static int PyMac_Yield(void);
#endif

/*
** These are the real scheduling parameters that control what we check
** in the event loop, and how often we check. The values are initialized
** from pyMac_SchedParamStruct.
*/

struct real_sched_param_struct {
	int		check_interrupt;	/* if true check for command-dot */
	int		process_events;		/* if nonzero enable evt processing, this mask */
	int		besocial;		/* if nonzero be a little social with CPU */
	unsigned long	check_interval;		/* how often to check, in ticks */
	unsigned long	bg_yield;		/* yield so long when in background */
	/* these are computed from previous and clock and such */
	int		enabled;		/* check_interrupt OR process_event OR yield */
	unsigned long	next_check;		/* when to check/yield next, in ticks */
};

static struct real_sched_param_struct schedparams =
	{ 1, MAINLOOP_EVENTMASK, 1, 15, 15, 1, 0};

/*
** Workaround for sioux/gusi combo: set when we are exiting
*/
int PyMac_ConsoleIsDead;

/*
** Sioux menu bar, saved early so we can restore it
*/
static MenuBarHandle sioux_mbar;

/*
** Some stuff for our GetDirectory and PromptGetFile routines
*/
struct hook_args {
	int selectcur_hit;		/* Set to true when "select current" selected */
	char *prompt;			/* The prompt */
};
#if !TARGET_API_MAC_OS8
/* The StandardFile hooks don't exist in Carbon. This breaks GetDirectory,
** but the macfsn code will replace it by a NavServices version anyway.
*/
#define myhook_upp NULL
#else
static DlgHookYDUPP myhook_upp;
static int upp_inited = 0;
#endif

/*
** The python-code event handler
*/
static PyObject *python_event_handler;

/*
** Set to true if we're appearance-compliant
*/
int PyMac_AppearanceCompliant;

/* Given an FSSpec, return the FSSpec of the parent folder */

static OSErr
get_folder_parent (FSSpec * fss, FSSpec * parent)
{
	CInfoPBRec rec;
	short err;

        * parent = * fss;
        rec.hFileInfo.ioNamePtr = parent->name;
        rec.hFileInfo.ioVRefNum = parent->vRefNum;
        rec.hFileInfo.ioDirID = parent->parID;
		rec.hFileInfo.ioFDirIndex = -1;
        rec.hFileInfo.ioFVersNum = 0;
        if (err = PBGetCatInfoSync (& rec))
        	return err;
        parent->parID = rec.dirInfo.ioDrParID;
/*	parent->name[0] = 0; */
        return 0;
}

/* Given an FSSpec return a full, colon-separated pathname */

OSErr
PyMac_GetFullPathname (FSSpec *fss, char *buf, int length)
{
	short err;
	FSSpec fss_parent, fss_current;
	char tmpbuf[1024];
	int plen;

	fss_current = *fss;
	plen = fss_current.name[0];
	if ( plen+2 > length ) {
		*buf = 0;
		return errFSNameTooLong;
	}
	memcpy(buf, &fss_current.name[1], plen);
	buf[plen] = 0;
	/* Special case for disk names */
	if ( fss_current.parID <= 1 ) {
		buf[plen++] = ':';
		buf[plen] = 0;
		return 0;
	}
	while (fss_current.parID > 1) {
    		/* Get parent folder name */
                if (err = get_folder_parent(&fss_current, &fss_parent)) {
                	*buf = 0;
             		return err;
             	}
                fss_current = fss_parent;
                /* Prepend path component just found to buf */
    			plen = fss_current.name[0];
    			if (strlen(buf) + plen + 1 > 1024) {
    				/* Oops... Not enough space (shouldn't happen) */
    				*buf = 0;
    				return errFSNameTooLong;
    			}
    			memcpy(tmpbuf, &fss_current.name[1], plen);
    			tmpbuf[plen] = ':';
    			strcpy(&tmpbuf[plen+1], buf);
    			if ( strlen(tmpbuf) > length ) {
    				*buf = 0;
    				return errFSNameTooLong;
    			}
    			strcpy(buf, tmpbuf);
        }
        return 0;
}

#ifdef USE_GUSI1
/*
** GUSI (1.6.0 and earlier, at the least) do not set the MacOS idea of
** the working directory. Hence, we call this routine after each call
** to chdir() to rectify things.
*/
void
PyMac_FixGUSIcd()
{
	WDPBRec pb;
	FSSpec curdirfss;
	
	if ( Path2FSSpec(":x", &curdirfss) != noErr ) 
		return;
	
	/* Set MacOS "working directory" */
	pb.ioNamePtr= "\p";
	pb.ioVRefNum= curdirfss.vRefNum;
	pb.ioWDDirID= curdirfss.parID;
	if (PBHSetVolSync(&pb) != noErr)
		return;
}
#endif

#ifdef USE_GUSI
/*
** SpinCursor (needed by GUSI) drags in heaps of stuff, so we
** provide a dummy here.
*/
void SpinCursor(short x) { /* Dummy */ }
void RotateCursor(short x) { /* Dummy */ }

/*
** Replacement GUSI Spin function
*/
#ifdef USE_GUSI1
static int
PyMac_GUSISpin(spin_msg msg, long arg)
{
	static Boolean	inForeground = true;
	int		maxsleep = 6;	/* 6 ticks is "normal" sleeptime */

	if (PyMac_ConsoleIsDead) return 0;
#if 0
	if (inForeground)
		SpinCursor(msg == SP_AUTO_SPIN ? short(arg) : 1);
#endif


	if ( msg == SP_AUTO_SPIN )
		maxsleep = 0;
	if ( msg==SP_SLEEP||msg==SP_SELECT ) {
		maxsleep = arg;
		/*
		** We force-scan for interrupts. Not pretty, but otherwise
		** a program may hang in select or sleep forever.
		*/
		scan_event_queue(1);
	}
	if (interrupted) {
		interrupted = 0;
		return -1;
	}

	PyMac_DoYield(maxsleep, 0); /* XXXX or is it safe to call python here? */

	return 0;
}

void
PyMac_SetGUSISpin() {
	GUSISetHook(GUSI_SpinHook, (GUSIHook)PyMac_GUSISpin);
}
#endif

/* Called at exit() time thru atexit(), to stop event processing */
void
PyMac_StopGUSISpin() {
	PyMac_ConsoleIsDead = 1;
}

#if TARGET_API_MAC_OS8
/*
** Replacement routines for the PLstr... functions so we don't need
** StdCLib.
*/
pascal void
PLstrcpy(unsigned char *to, unsigned char *fr)
{
	memcpy(to, fr, fr[0]+1);
}

pascal int
PLstrcmp(unsigned char *s1, unsigned char *s2)
{
	int res;
	int l = s1[0] < s2[0] ? s1[0] : s2[0];
	
	res = memcmp(s1+1, s2+1, l);
	if ( res != 0 )
		return res;
	
	if ( s1[0] < s2[0] )
		return -1;
	else if ( s1[0] > s2[0] )
		return 1;
	else
		return 0;
}

pascal unsigned char *
PLstrrchr(unsigned char *str, unsigned char chr)
{
	unsigned char *ptr = 0;
	unsigned char *p;
	
	for(p=str+1; p<str+str[0]; p++)
		if ( *p == chr )
			ptr = p;
	return ptr;
}
	
#endif /* TARGET_API_MAC_OS8 */
#endif /* USE_GUSI */


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

#if TARGET_API_MAC_OS8
void
c2pstrcpy(unsigned char *dst, const char *src)
{
	int len;
	
	len = strlen(src);
	if ( len > 255 ) len = 255;
	strncpy((char *)dst+1, src, len);
	dst[0] = len;
}
#endif /* TARGET_API_MAC_OS8 */

#ifdef USE_STACKCHECK
/* Check for stack overflow */
int
PyOS_CheckStack()
{
	char here;
	static char *sentinel = 0;
	static PyThreadState *thread_for_sentinel = 0;
	
	if ( sentinel == 0 ) {
		unsigned long stackspace = StackSpace();
		
#ifdef MAXIMUM_STACK_SIZE
	/* See the comment at the definition */
	if ( stackspace > MAXIMUM_STACK_SIZE )
		stackspace = MAXIMUM_STACK_SIZE;
#endif	
		sentinel = &here - stackspace + MINIMUM_STACK_SIZE;
	}
	if ( thread_for_sentinel == 0 ) {
		thread_for_sentinel = PyThreadState_Get();
	}
	if ( &here < sentinel ) {
		if (thread_for_sentinel == PyThreadState_Get()) {
			return -1;
#if 0
		} else {
			/* Else we are unsure... */
			fprintf(stderr, "Stackcheck in other thread (was %x now %x)\n", thread_for_sentinel,PyThreadState_Get()); 
#endif
		}
	}
	return 0;
}
#endif /* USE_STACKCHECK */

#if !TARGET_API_MAC_OSX
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

void
PyOS_FiniInterrupts()
{
}

/* Check whether we are in the foreground */
static int
PyMac_InForeground(void)
{
	static ProcessSerialNumber ours;
	static inited;
	ProcessSerialNumber curfg;
	Boolean eq;
	
	if ( inited == 0 ) {
		(void)GetCurrentProcess(&ours);
		inited = 1;
	}
	if ( GetFrontProcess(&curfg) < 0 )
		eq = 1;
	else if ( SameProcess(&ours, &curfg, &eq) < 0 )
		eq = 1;
	return (int)eq;
}

/*
** This routine scans the event queue looking for cmd-.
*/
static void
scan_event_queue(force)
	int force;
{
#if !TARGET_API_MAC_OS8
	if ( interrupted || (!schedparams.check_interrupt && !force) )
		return;
	if ( CheckEventQueueForUserCancel() )
		interrupted = 1;
#else
	register EvQElPtr q;
	
	if ( interrupted || (!schedparams.check_interrupt && !force) || !PyMac_InForeground() )
		return;
	q = (EvQElPtr) LMGetEventQueue()->qHead;
	
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
PyErr_CheckSignals()
{
	int xxx, xxx_old;
	
	if (schedparams.enabled) {
		if ( interrupted || (unsigned long)LMGetTicks() > schedparams.next_check ) {
			scan_event_queue(0);
			if (interrupted) {
				interrupted = 0;
				PyErr_SetNone(PyExc_KeyboardInterrupt);
				return -1;
			}
			if ( PyMac_Yield() < 0)
				return -1;
			xxx = LMGetTicks();
			xxx_old = schedparams.next_check;
			schedparams.next_check = (unsigned long)LMGetTicks()
					 + schedparams.check_interval;
		}
	}
	return 0;
}

int
PyOS_InterruptOccurred()
{
	scan_event_queue(0);
	if ( !interrupted )
		return 0;
	interrupted = 0;
	return 1;
}
#endif

int
PyMac_SetEventHandler(PyObject *evh)
{
	if ( evh && python_event_handler ) {
		PyErr_SetString(PyExc_RuntimeError, "Python event handler already set");
		return 0;
	}
	if ( python_event_handler )
		Py_DECREF(python_event_handler);
	if ( evh )
		Py_INCREF(evh);
	python_event_handler = evh;
	return 1;
}

/*
** Handle an event, either one found in the mainloop eventhandler or
** one passed back from the python program.
*/
void
PyMac_HandleEventIntern(evp)
	EventRecord *evp;
{
#if TARGET_API_MAC_OS8
	if ( evp->what == mouseDown ) {
		WindowPtr wp;
		
		if ( FindWindow(evp->where, &wp) == inSysWindow ) {
			SystemClick(evp, wp);
			return;
		}
	}
#endif
#ifdef __MWERKS__
	{
		int siouxdidit;

		/* If SIOUX wants it we're done */
		siouxdidit = SIOUXHandleOneEvent(evp);
		if ( siouxdidit )
			return;
	}
#else
	/* Other compilers are just unlucky... */
#endif /* !__MWERKS__ */
}

/*
** Handle an event, either through HandleEvent or by passing it to the Python
** event handler.
*/
int
PyMac_HandleEvent(evp)
	EventRecord *evp;
{
	PyObject *rv;
	
	if ( python_event_handler ) {
		rv = PyObject_CallFunction(python_event_handler, "(O&)", 
			PyMac_BuildEventRecord, evp);
		if ( rv )
			Py_DECREF(rv);
		else
			return -1;	/* Propagate exception */
	} else {
		PyMac_HandleEventIntern(evp);
	}
	return 0;
}

#if !TARGET_API_MAC_OSX
/*
** Yield the CPU to other tasks without processing events.
*/
int
PyMac_DoYield(int maxsleep, int maycallpython)
{
	EventRecord ev;
	int gotone;
	long latest_time_ready;
	static int in_here = 0;
	
	in_here++;
		
	/*
	** Check which of the eventloop cases we have:
	** - process events
	** - don't process events but do yield
	** - do neither
	*/
	if( in_here > 1 || !schedparams.process_events || 
	    (python_event_handler && !maycallpython) ) {
		if ( maxsleep >= 0 ) {
#if TARGET_API_MAC_OS8
			SystemTask();
#else
			int xxx = 0;
#endif
		}
	} else {
		latest_time_ready = LMGetTicks() + maxsleep;
		do {
			/* XXXX Hack by Jack.
			** In time.sleep() you can click to another application
			** once only. If you come back to Python you cannot get away
			** again.
			**/
			gotone = WaitNextEvent(schedparams.process_events, &ev, maxsleep, NULL);	
			/* Get out quickly if nothing interesting is happening */
			if ( !gotone || ev.what == nullEvent )
				break;
			if ( PyMac_HandleEvent(&ev) < 0 ) {
				in_here--;
				return -1;
			}
			maxsleep = latest_time_ready - LMGetTicks();
		} while ( maxsleep > 0 );
	}
	in_here--;
	return 0;
}

/*
** Process events and/or yield the CPU to other tasks if opportune
*/
int
PyMac_Yield() {
	unsigned long maxsleep;
	
	if( PyMac_InForeground() )
		maxsleep = 0;
	else
		maxsleep = schedparams.bg_yield;

	return PyMac_DoYield(maxsleep, 1);
}

/*
** Return current scheduler parameters
*/
void
PyMac_GetSchedParams(PyMacSchedParams *sp)
{
	sp->check_interrupt = schedparams.check_interrupt;
	sp->process_events = schedparams.process_events;
	sp->besocial = schedparams.besocial;
	sp->check_interval = schedparams.check_interval / 60.0;
	sp->bg_yield = schedparams.bg_yield / 60.0;
}

/*
** Set current scheduler parameters
*/
void
PyMac_SetSchedParams(PyMacSchedParams *sp)
{
	schedparams.check_interrupt = sp->check_interrupt;
	schedparams.process_events = sp->process_events;
	schedparams.besocial = sp->besocial;
	schedparams.check_interval = (unsigned long)(sp->check_interval*60);
	schedparams.bg_yield = (unsigned long)(sp->bg_yield*60);
	if ( schedparams.check_interrupt || schedparams.process_events ||
	     schedparams.besocial )
	     	schedparams.enabled = 1;
	else
		schedparams.enabled = 0;
	schedparams.next_check = 0;	/* Check immedeately */
}

/*
** Install our menu bar.
*/
void
PyMac_InitMenuBar()
{
	MenuHandle applemenu;
	
	if ( sioux_mbar ) return;
#if 0
	/* This code does not seem to work anymore: apparently
	** we now always have a menubar (since MacOS9?).
	** So we simply always setup the Sioux menus here.
	*/
	if ( (sioux_mbar=GetMenuBar()) == NULL )  {
#else
	{
#endif
		/* Sioux menu not installed yet. Do so */
		SIOUXSetupMenus();
		if ( (sioux_mbar=GetMenuBar()) == NULL )
			return;
	}
	if ( (applemenu=GetMenuHandle(SIOUX_APPLEID)) == NULL ) return;
	SetMenuItemText(applemenu, 1, "\pAbout Python...");
}

/*
** Restore sioux menu bar
*/
void
PyMac_RestoreMenuBar()
{
#if 1
	/* This doesn't seem to work anymore? Or only for Carbon? */
	MenuBarHandle curmenubar;
	
	curmenubar = GetMenuBar();
	if ( sioux_mbar ) {
		SetMenuBar(sioux_mbar);
		DrawMenuBar();
	} else {
		PyMac_InitMenuBar();
		DrawMenuBar();
	}
#endif
}

void
PyMac_RaiseConsoleWindow()
{
	/* Note: this is a hack. SIOUXTextWindow is SIOUX's internal structure
	** and we happen to know that the first entry is the window pointer.
	*/
	extern WindowRef *SIOUXTextWindow;

	if ( SIOUXTextWindow == NULL || *SIOUXTextWindow == NULL )
		return;
	if ( FrontWindow() != *SIOUXTextWindow )
		BringToFront(*SIOUXTextWindow);
}

/*
** Our replacement about box
*/

#include "patchlevel.h"

void
SIOUXDoAboutBox(void)
{
	DialogPtr theDialog;
	WindowPtr theWindow;
	short item;
	short fontID;
	
	if( (theDialog = GetNewDialog(ABOUT_ID, NULL, (WindowPtr)-1)) == NULL )
		return;
	theWindow = GetDialogWindow(theDialog);
	SetPortWindowPort(theWindow);
	GetFNum("\pPython-Sans", &fontID);
	if (fontID == 0)
		fontID = kFontIDGeneva;
	TextFont(fontID);
	TextSize(9);
	ParamText(Pstring(PY_VERSION), "\p", "\p", "\p");
	ShowWindow(theWindow);
	ModalDialog(NULL, &item);
	DisposeDialog(theDialog);
}

#endif /* !TARGET_API_MAC_OSX */

#if TARGET_API_MAC_OS8
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
#endif /* TARGET_API_MAC_OS8 */


