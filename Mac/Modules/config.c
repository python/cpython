/* -*- C -*- ***********************************************
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

/* Macintosh Python configuration file */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef macintosh
/* The Macintosh main program is in macmain.c */
#define NO_MAIN
#endif

#include <stdio.h>
#include <string.h>

#include "myproto.h"
#include "mymalloc.h"
#include "osdefs.h"
#include "intrcheck.h"


#ifndef NO_MAIN

/* Normally, the main program is called from here (so everything else
   can be in libPython.a).  We save a pointer to argv[0] because it
   may be needed for dynamic loading of modules in import.c.  If you
   have your own main program and want to use non-SunOS dynamic
   loading, you will have to provide your own version of
   getprogramname(). */

static char *argv0;

main(argc, argv)
	int argc;
	char **argv;
{
	argv0 = argv[0];
	realmain(argc, argv);
}

char *
getprogramname()
{
	return argv0;
}

#endif


/* Python version information */

#include "patchlevel.h"

/* Return the version string.  This is constructed from the official
   version number (from patchlevel.h), and the current date (if known
   to the compiler, else a manually inserted date). */

#define VERSION "%s (%s)"

#ifdef __DATE__
#define DATE __DATE__
#else
#define DATE "Aug 17 1994"
#endif

char *
getversion()
{
	static char version[80];
	sprintf(version, VERSION, PATCHLEVEL, DATE);
#ifdef __MWERKS__
#ifdef __powerc
	strcat(version, " [MW PPC compiler]");
#else
	strcat(version, " [MW 68K compiler]");
#endif
#endif
	return version;
}


/* Return the copyright string.  This is updated manually. */

char *
getcopyright()
{
	return "Copyright 1991-1994 Stichting Mathematisch Centrum, Amsterdam";
}


/* Return the initial python search path.  This is called once from
   initsys() to initialize sys.path.
   The environment variable PYTHONPATH is fetched and the default path
   appended.  (The Mac has no environment variables, so there the
   default path is always returned.)  The default path may be passed
   to the preprocessor; if not, a system-dependent default is used. */

#ifndef PYTHONPATH
#ifdef macintosh
/* Mod by Jack: \n is now separator. */
#define PYTHONPATH ":\n:Lib\n:Lib:stdwin\n:Lib:test\n:Lib:mac"
#endif /* macintosh */
#endif /* !PYTHONPATH */

#ifndef PYTHONPATH
#if defined(MSDOS) || defined(NT)
#define PYTHONPATH ".;..\\lib;\\python\\lib"
#endif /* MSDOS || NT */
#endif /* !PYTHONPATH */

#ifndef PYTHONPATH
#define PYTHONPATH ".:/usr/local/lib/python"
#endif /* !PYTHONPATH */

extern char *getenv();

char *
getpythonpath()
{
#ifdef macintosh
	/* Modified by Jack to do something a bit more sensible:
	** - Prepend the current directory (which is presumably where python lives)
	** - Add :
	** - Chdir to where the source file (if any) lives
	*/
	static char *pythonpath;
	extern char *fileargument;
	char curwd[256];
	char *p, *endp;
	int newlen;
	
	if ( pythonpath ) return pythonpath;
	if (getwd(curwd) < 0 ) {
		return PYTHONPATH;
	}
	p = PYTHONPATH;
	endp = p;
	pythonpath = malloc(2);
	if ( pythonpath == NULL ) return PYTHONPATH;
	strcpy(pythonpath, ":");
	while (*endp) {
		endp = strchr(p, '\n');
		if ( endp == NULL )
			endp = p + strlen(p);
		newlen = strlen(pythonpath) + 1 + strlen(curwd) + (endp-p);
		pythonpath = realloc(pythonpath, newlen+1);
		if ( pythonpath == NULL ) return PYTHONPATH;
		strcat(pythonpath, "\n");
		if ( *p == ':' ) {
			p++;
			strcat(pythonpath, curwd);
			strncat(pythonpath, p, (endp-p));
			newlen--;   /* Ok, ok, we've allocated one byte too much */
		} else {
			/* We've allocated too much in this case */
			newlen -= strlen(curwd);
			pythonpath = realloc(pythonpath, newlen+1);
			if ( pythonpath == NULL ) return PYTHONPATH;
			strncat(pythonpath, p, (endp-p));
		}
		pythonpath[newlen] = '\0';
		p = endp + 1;
	}
	if ( fileargument ) {
		strcpy(curwd, fileargument);
		endp = strrchr(curwd, ':');
		if ( endp && endp > curwd ) {
			*endp = '\0';
			chdir(curwd);
		}
	}
	return pythonpath;
#else /* !macintosh */
	char *path = getenv("PYTHONPATH");
	char *defpath = PYTHONPATH;
	char *buf;
	char *p;
	int n;

	if (path == 0 || *path == '\0')
		return defpath;
	n = strlen(path) + strlen(defpath) + 2;
	buf = malloc(n);
	if (buf == NULL)
		return path; /* XXX too bad -- but not likely */
	strcpy(buf, path);
	p = buf + strlen(buf);
	*p++ = DELIM;
	strcpy(p, defpath);
	return buf;
#endif /* !macintosh */
}


/* Table of built-in modules.
   These are initialized when first imported.
   Note: selection of optional extensions is now generally done by the
   makesetup script. */

extern void initarray();
extern void initmath();
extern void initparser();
extern void initmac();
extern void MacOS_Init();
extern void initregex();
extern void initstrop();
extern void initstruct();
extern void inittime();
extern void initdbm();
extern void initfcntl();
extern void initnis();
extern void initpwd();
extern void initgrp();
extern void initcrypt();
extern void initselect();
extern void initsocket();
extern void initaudioop();
extern void initimageop();
extern void initrgbimg();
extern void initstdwin();
extern void initmd5();
extern void initmpz();
extern void initrotor();
extern void inital();
extern void initcd();
extern void initcl();
extern void initfm();
extern void initgl();
extern void initimgfile();
extern void initimgformat();
extern void initsgi();
extern void initsv();
extern void initfl();
extern void initthread();
extern void inittiming();
extern void initsignal();
extern void initnew();
extern void initdl();
extern void initsyslog();
extern void initgestalt();
#ifdef THINK
extern void initmacconsole();
#endif
extern void initctb();
extern void initmacspeech();
extern void initmacdnr();
extern void initmactcp();

/* -- ADDMODULE MARKER 1 -- */

extern void initmarshal();

struct {
	char *name;
	void (*initfunc)();
} inittab[] = {

	{"array", initarray},
	{"math", initmath},
	{"parser", initparser},
	{"mac", initmac},
	{"MacOS", MacOS_Init},
	{"regex", initregex},
	{"strop", initstrop},
	{"struct", initstruct},
	{"time", inittime},
	{"audioop", initaudioop},
	{"imageop", initimageop},
	{"rgbimg", initrgbimg},
#ifdef USE_STDWIN
	{"stdwin", initstdwin},
#endif
	{"md5", initmd5},
	{"rotor", initrotor},
	{"new", initnew},
	{"gestalt", initgestalt},
#ifdef THINK
	{"macconsole", initmacconsole},
#endif
	{"ctb", initctb},
	{"macspeech", initmacspeech},
	{"imgformat", initimgformat},
	{"macdnr", initmacdnr},
	{"mactcp", initmactcp},

/* -- ADDMODULE MARKER 2 -- */

	/* This module "lives in" with marshal.c */
	{"marshal", initmarshal},

	/* These entries are here for sys.builtin_module_names */
	{"__main__", NULL},
	{"__builtin__", NULL},
	{"sys", NULL},

	/* Sentinel */
	{0, 0}
};

#ifdef USE_FROZEN
#include "frozen.c"
#else
struct frozen {
	char *name;
	char *code;
	int size;
} frozen_modules[] = {
	{0, 0, 0}
};
#endif
