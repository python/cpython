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

#ifdef WITHOUT_FRAMEWORKS
#include <Types.h>
#include <Files.h>
#include <Events.h>
#include <StandardFile.h>
#else
#include <Carbon/Carbon.h>
#endif

#include "pymactoolbox.h"

#ifdef __cplusplus
	extern "C" {
#endif

/* Scheduler parameters */
typedef struct {
	int		check_interrupt;	/* if true check for command-dot */
	int		process_events;		/* if nonzero enable evt processing, this mask */
	int		besocial;		/* Be social, give up CPU now and again */
	double		check_interval;		/* how often to check */
	double		bg_yield;		/* yield at most so long when in background */
} PyMacSchedParams;

unsigned char *Pstring(char *str);		/* Convert c-string to pascal-string in static buffer */

#ifdef USE_GUSI
extern int PyMac_ConsoleIsDead;			/* True when exiting */
extern void PyMac_StopGUSISpin(void);		/* Stop eventprocessing during exit() */
#endif

extern short PyMac_AppRefNum;			/* RefNum of application rsrcfork (from macmain.c) */
extern FSSpec PyMac_ApplicationFSSpec;		/* Application location (from macargv.c) */
extern char PyMac_ApplicationPath[];		/* Application location (from macargv.c) */
extern OSErr PyMac_init_application_location(void);	/* Init the above */
extern int PyMac_GetArgv(char ***, int);	/* Get argc, argv (from macargv.c) */

extern PyObject *PyMac_OSErrException;		/* Exception for OSErr */
PyObject *PyMac_GetOSErrException(void);	/* Initialize & return it */

#if !TARGET_API_MAC_OSX
void PyMac_GetSchedParams(PyMacSchedParams *);	/* Get schedulers params */
void PyMac_SetSchedParams(PyMacSchedParams *);	/* Set schedulers params */
int PyMac_DoYield(int, int);	/* Yield cpu. First arg is maxtime, second ok to call python */
#endif
int PyMac_HandleEvent(EventRecord *);	/* Handle one event, possibly in Python */
void PyMac_HandleEventIntern(EventRecord *); /* Handle one event internal only */
int PyMac_SetEventHandler(PyObject *);	/* set python-coded event handler */

#if !TARGET_API_MAC_OSX
void PyMac_InitMenuBar(void);			/* Setup menu bar as we want it */
void PyMac_RestoreMenuBar(void);		/* Restore menu bar for ease of exiting */
void PyMac_RaiseConsoleWindow();		/* Bring console window to front, if it exists */
#endif
int PyMac_FindResourceModule(PyStringObject *, char *, char *); /* Test for 'PYC ' resource in a file */
PyObject * PyMac_LoadResourceModule(char *, char *); /* Load 'PYC ' resource from file */
int PyMac_FindCodeResourceModule(PyStringObject *, char *, char *); /* Test for 'PYD ' resource in a file */
PyObject * PyMac_LoadCodeResourceModule(char *, char *); /* Load 'PYD ' resource from file */
struct filedescr *PyMac_FindModuleExtension(char *, size_t *, char *); /* Look for module in single folder */

#if TARGET_API_MAC_OS8
int PyMac_GetDirectory(FSSpec *dirfss, char *prompt);		/* Ask user for a directory */
void PyMac_PromptGetFile(short numTypes, ConstSFTypeListPtr typeList, 
	StandardFileReply *reply, char *prompt);	/* Ask user for file, with prompt */
#endif /* TARGET_API_MAC_OS8 */

void PyMac_InitApplet(void);			/* Initialize and run an Applet */
void PyMac_Initialize(void);			/* Initialize function for embedding Python */

#ifdef USE_GUSI2
short PyMac_OpenPrefFile(void);			/* From macgetpath.c, open and return preference file */
#endif


/* From macfiletype.c: */

long PyMac_getfiletype(char *);			/* Get file type */
int PyMac_setfiletype(char *, long, long);		/* Set file creator and type */

/* from macmain.c: */
void PyMac_Exit(int);
void PyMac_InitApplication(void);
void PyMac_OutputSeen(void);
void PyMac_OutputNotSeen(void);
int PyMac_GetDelayConsoleFlag(void);
#ifdef USE_MAC_APPLET_SUPPORT
void PyMac_InitApplet(void);
#endif

/* from macgetargv: */
OSErr PyMac_init_process_location(void);
char *	strdup(const char *str);

#ifdef USE_GUSI2
/* from pyGUSISIOUX.cp */
typedef long (*PyWriteHandler)(char *buffer, long n);
typedef long (*PyReadHandler)(char *buffer, long n);

/* Override routines that normally reads and writes to the
** SIOUX console window. Intended for embedding applications
** that want to forestall a Python console window ever showing up.
*/
void PyMac_SetConsoleHandler(PyReadHandler stdinH, PyWriteHandler stdoutH,
			     PyWriteHandler stderrH);

/* Courtesy console handlers that drop all output and return
** 0 on reads.
*/
long PyMac_DummyReadHandler(char *, long);
long PyMac_DummyWriteHandler(char *, long);
#endif /* USE_GUSI2 */

#ifdef __cplusplus
	}
#endif
