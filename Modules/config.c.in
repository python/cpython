/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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

/* Configurable Python configuration file */

#include <stdio.h>

#ifdef USE_STDWIN
#include <stdwin.h>

static int use_stdwin;
#endif

/*ARGSUSED*/
void
initargs(p_argc, p_argv)
	int *p_argc;
	char ***p_argv;
{
#ifdef USE_STDWIN
	extern char *getenv();
	char *display;

	/* Ignore an initial argument of '-s', for backward compatibility */
	if (*p_argc > 1 && strcmp((*p_argv)[1], "-s") == 0) {
		(*p_argv)[1] = (*p_argv)[0];
		(*p_argc)--, (*p_argv)++;
	}

	/* Assume we have to initialize stdwin if either of the following
	   conditions holds:
	   - the environment variable $DISPLAY is set
	   - there is an argument "-display" somewhere
	*/
	
	display = getenv("DISPLAY");
	if (display != 0)
		use_stdwin = 1;
	else {
		int i;
		/* Scan through the arguments looking for "-display" */
		for (i = 1; i < *p_argc; i++) {
			if (strcmp((*p_argv)[i], "-display") == 0) {
				use_stdwin = 1;
				break;
			}
		}
	}
	
	if (use_stdwin)
		wargs(p_argc, p_argv);
#endif
}

void
initcalls()
{
}

void
donecalls()
{
#ifdef USE_STDWIN
	if (use_stdwin)
		wdone();
#endif
#ifdef USE_AUDIO
	asa_done();
#endif
}

#ifdef USE_STDWIN
static void
maybeinitstdwin()
{
	if (use_stdwin)
		initstdwin();
	else
		fprintf(stderr,
		 "No $DISPLAY nor -display arg -- stdwin not available\n");
}
#endif

#ifndef PYTHONPATH
#define PYTHONPATH ".:/usr/local/lib/python"
#endif

extern char *getenv();

char *
getpythonpath()
{
	char *path = getenv("PYTHONPATH");
	if (path == 0)
		path = PYTHONPATH;
	return path;
}


/* Table of built-in modules.
   These are initialized when first imported. */

/* Standard modules */
extern void inittime();
extern void initmath();
extern void initregexp();
extern void initposix();
#ifdef USE_AUDIO
extern void initaudio();
#endif
#ifdef USE_AMOEBA
extern void initamoeba();
#endif
#ifdef USE_GL
extern void initgl();
#ifdef USE_PANEL
extern void initpanel();
#endif
#endif
#ifdef USE_STDWIN
extern void maybeinitstdwin();
#endif

struct {
	char *name;
	void (*initfunc)();
} inittab[] = {

	/* Standard modules */

	{"time",	inittime},
	{"math",	initmath},
	{"regexp",	initregexp},
	{"posix",	initposix},


	/* Optional modules */

#ifdef USE_AUDIO
	{"audio",	initaudio},
#endif

#ifdef USE_AMOEBA
	{"amoeba",	initamoeba},
#endif

#ifdef USE_GL
	{"gl",		initgl},
#ifdef USE_PANEL
	{"pnl",		initpanel},
#endif
#endif

#ifdef USE_STDWIN
	{"stdwin",	maybeinitstdwin},
#endif

	{0,		0}		/* Sentinel */
};
