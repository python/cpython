/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

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

/* Universal Python configuration file */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "myproto.h"
#include "mymalloc.h"
#include "osdefs.h"


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
#ifdef macintosh

#ifndef MPW /* XXX RJW undefined in MPW */
	wargs(&argc, &argv);
#endif

#ifndef MPW_3 /* XXX RJW doesn't seem to work with MPW C 3.0 */
	extern int std_open_hook();
	set_open_hook (std_open_hook);
#endif
#endif

	argv0 = argv[0];
	realmain(argc, argv);
}

char *
getprogramname()
{
	return argv0;
}

#endif


/* Return the initial python search path.  This is called once from
   initsys() to initialize sys.path.
   The environment variable PYTHONPATH is fetched and the default path
   appended.  (The Mac has no environment variables, so there the
   default path is always returned.)  The default path may be passed
   to the preprocessor; if not, a system-dependent default is used. */

#ifndef PYTHONPATH
#ifdef macintosh
#define PYTHONPATH ": :Lib :Lib:stdwin :Demo"
#endif /* macintosh */
#endif /* !PYTHONPATH */

#ifndef PYTHONPATH
#ifdef MSDOS
#define PYTHONPATH ".;..\\lib;\\python\\lib"
#endif /* MSDOS */
#endif /* !PYTHONPATH */

#ifndef PYTHONPATH
#define PYTHONPATH ".:/usr/local/lib/python"
#endif /* !PYTHONPATH */

extern char *getenv();

char *
getpythonpath()
{
#ifdef macintosh
	return PYTHONPATH;
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
extern void initregex();
extern void initstrop();
extern void initstruct();
extern void inittime();
extern void initdbm();
extern void initfcntl();
extern void initnis();
extern void initpwd();
extern void initgrp();
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
extern void initsgi();
extern void initsv();
extern void initfl();
extern void initthread();
extern void inittiming();

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
	{"regex", initregex},
	{"strop", initstrop},
	{"struct", initstruct},
	{"time", inittime},
	{"audioop", initaudioop},
	{"imageop", initimageop},
	{"rgbimg", initrgbimg},
	{"stdwin", initstdwin},
	{"md5", initmd5},
	{"rotor", initrotor},

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
