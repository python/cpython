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
#ifndef Py_MACGLUE_H
#define Py_MACGLUE_H
#include <Carbon/Carbon.h>

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

extern short PyMac_AppRefNum;			/* RefNum of application rsrcfork (from macmain.c) */
extern FSSpec PyMac_ApplicationFSSpec;		/* Application location (from macargv.c) */
extern char PyMac_ApplicationPath[];		/* Application location (from macargv.c) */
extern OSErr PyMac_init_application_location(void);	/* Init the above */
extern int PyMac_GetArgv(char ***, int);	/* Get argc, argv (from macargv.c) */

extern PyObject *PyMac_OSErrException;		/* Exception for OSErr */
PyObject *PyMac_GetOSErrException(void);	/* Initialize & return it */

int PyMac_HandleEvent(EventRecord *);	/* Handle one event, possibly in Python */
void PyMac_HandleEventIntern(EventRecord *); /* Handle one event internal only */
int PyMac_SetEventHandler(PyObject *);	/* set python-coded event handler */

int PyMac_FindResourceModule(PyStringObject *, char *, char *); /* Test for 'PYC ' resource in a file */
PyObject * PyMac_LoadResourceModule(char *, char *); /* Load 'PYC ' resource from file */
int PyMac_FindCodeResourceModule(PyStringObject *, char *, char *); /* Test for 'PYD ' resource in a file */
PyObject * PyMac_LoadCodeResourceModule(char *, char *); /* Load 'PYD ' resource from file */
struct filedescr *PyMac_FindModuleExtension(char *, size_t *, char *); /* Look for module in single folder */

void PyMac_InitApplet(void);			/* Initialize and run an Applet */
void PyMac_Initialize(void);			/* Initialize function for embedding Python */


/* From macfiletype.c: */

long PyMac_getfiletype(char *);			/* Get file type */
int PyMac_setfiletype(char *, long, long);		/* Set file creator and type */

/* from macmain.c: */
void PyMac_Exit(int);
void PyMac_InitApplication(void);
void PyMac_OutputSeen(void);
void PyMac_OutputNotSeen(void);
int PyMac_GetDelayConsoleFlag(void);

/* from macgetargv: */
OSErr PyMac_init_process_location(void);
char *	strdup(const char *str);

#ifdef __cplusplus
	}
#endif
#endif
