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

/* Python version information */

/* Return the version string.  This is constructed from the official
   version number, the patch level, and the current date (if known to
   the compiler, else a manually inserted date). */

#define VERSION "1.0.%d ALPHA (%s)"

#ifdef __DATE__
#define DATE __DATE__
#else
#define DATE ">= 21 Dec 1993"
#endif

char *
getversion()
{
	static char version[80];
	sprintf(version, VERSION, PATCHLEVEL, DATE);
	return version;
}


/* Return the copyright string.  This is updated manually. */

char *
getcopyright()
{
	return
"Copyright 1990, 1991, 1992, 1993 Stichting Mathematisch Centrum, Amsterdam";
}


/* Return the initial python search path.  This is called once from
   initsys() to initialize sys.path.
   The environment variable PYTHONPATH is fetched and the default path
   appended.  (The Mac has no environment variables, so there the
   default path is always returned.)  The default path may be passed
   to the preprocessor; if not, a system-dependent default is used. */

#ifndef PYTHONPATH
#ifdef macintosh
#define PYTHONPATH ": :lib :demo"
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
   mkext.py script in ../Extensions, but for non-UNIX systems most
   well-known extensions are still listed here. */

/* Standard modules */

#ifdef USE_AL
extern void inital();
#endif
#ifdef USE_AMOEBA
extern void initamoeba();
#endif
#ifdef USE_AUDIO
extern void initaudio();
#endif
#ifdef USE_AUDIOOP
extern void initaudioop();
#endif
#ifdef USE_CD
extern void initcd();
#endif
#ifdef USE_CL
extern void initcl();
#endif
#ifdef USE_DBM
extern void initdbm();
#endif
#ifdef USE_FCNTL
extern void initfcntl();
#endif
#ifdef USE_FL
extern void initfl();
#endif
#ifdef USE_FM
extern void initfm();
#endif
#ifdef USE_GL
extern void initgl();
#endif
#ifdef USE_GRP
extern void initgrp();
#endif
#ifdef USE_IMGFILE
extern void initimgfile();
#endif
#ifdef USE_JPEG
extern void initjpeg();
#endif
#ifdef USE_MAC
extern void initmac();
#endif
#ifdef USE_MARSHAL
extern void initmarshal();
#endif
#ifdef USE_MATH
extern void initmath();
#endif
#ifdef USE_NIS
extern void initnis();
#endif
#ifdef USE_PANEL
extern void initpanel();
#endif
#ifdef USE_POSIX
extern void initposix();
#endif
#ifdef USE_PWD
extern void initpwd();
#endif
#ifdef USE_REGEX
extern void initregex();
#endif
#ifdef USE_ROTOR
extern void initrotor();
#endif
#ifdef USE_SELECT
extern void initselect();
#endif
#ifdef USE_SGI
extern void initsgi();
#endif
#ifdef USE_SOCKET
extern void initsocket();
#endif
#ifdef USE_STDWIN
extern void initstdwin();
#endif
#ifdef USE_STROP
extern void initstrop();
#endif
#ifdef USE_STRUCT
extern void initstruct();
#endif
#ifdef USE_SUNAUDIODEV
extern void initsunaudiodev();
#endif
#ifdef USE_SV
extern void initsv();
#endif
#ifdef USE_TIME
extern void inittime();
#endif
#ifdef USE_IMAGEOP
extern void initimageop();
#endif
#ifdef USE_MPZ
extern void initmpz();
#endif
#ifdef USE_MD5
extern void initmd5();
#endif
#ifdef USE_ARRAY
extern void initarray();
#endif
#ifdef USE_XT
extern void initXt();
#endif
#ifdef USE_XAW
extern void initXaw();
#endif
#ifdef USE_XM
extern void initXm();
#endif
#ifdef USE_GLX
extern void initGlx();
#endif
#ifdef USE_HTML
extern void initHTML();
#endif
#ifdef USE_XLIB
extern void initXlib();
#endif
#ifdef USE_PARSER
extern void initparser();
#endif
/* -- ADDMODULE MARKER 1 -- */

struct {
	char *name;
	void (*initfunc)();
} inittab[] = {

#ifdef USE_AL
	{"al",		inital},
#endif

#ifdef USE_AMOEBA
	{"amoeba",	initamoeba},
#endif

#ifdef USE_AUDIO
	{"audio",	initaudio},
#endif

#ifdef USE_AUDIOOP
	{"audioop",	initaudioop},
#endif

#ifdef USE_CD
	{"cd",		initcd},
#endif

#ifdef USE_CL
	{"cl",		initcl},
#endif

#ifdef USE_DBM
	{"dbm",		initdbm},
#endif

#ifdef USE_FCNTL
	{"fcntl",	initfcntl},
#endif

#ifdef USE_FL
	{"fl",		initfl},
#endif

#ifdef USE_FM
	{"fm",		initfm},
#endif

#ifdef USE_GL
	{"gl",		initgl},
#endif

#ifdef USE_GRP
	{"grp",		initgrp},
#endif

#ifdef USE_IMGFILE
	{"imgfile",	initimgfile},
#endif

#ifdef USE_JPEG
	{"jpeg",	initjpeg},
#endif

#ifdef USE_MAC
	{"mac",	initmac},
#endif

#ifdef USE_MARSHAL
	{"marshal",	initmarshal},
#endif

#ifdef USE_MATH
	{"math",	initmath},
#endif

#ifdef USE_NIS
	{"nis",		initnis},
#endif

#ifdef USE_PANEL
	{"pnl",		initpanel},
#endif

#ifdef USE_POSIX
	{"posix",	initposix},
#endif

#ifdef USE_PWD
	{"pwd",		initpwd},
#endif

#ifdef USE_REGEX
	{"regex",	initregex},
#endif

#ifdef USE_ROTOR
	{"rotor",	initrotor},
#endif

#ifdef USE_SELECT
	{"select",	initselect},
#endif

#ifdef USE_SGI
	{"sgi",		initsgi},
#endif

#ifdef USE_SOCKET
	{"socket",	initsocket},
#endif

#ifdef USE_STDWIN
	{"stdwin",	initstdwin},
#endif

#ifdef USE_STROP
	{"strop",	initstrop},
#endif

#ifdef USE_STRUCT
	{"struct",	initstruct},
#endif

#ifdef USE_SUNAUDIODEV
	{"sunaudiodev",	initsunaudiodev},
#endif

#ifdef USE_SV
	{"sv",		initsv},
#endif

#ifdef USE_TIME
	{"time",	inittime},
#endif

#ifdef USE_IMAGEOP
       {"imageop", initimageop},
#endif

#ifdef USE_MPZ
       {"mpz", initmpz},
#endif

#ifdef USE_MD5
       {"md5", initmd5},
#endif

#ifdef USE_ARRAY
       {"array", initarray},
#endif
