/* Python interpreter main program */

/* XXX This is still a mess */

#ifdef THINK_C
#define USE_STDWIN
#endif

#include <stdio.h>
#include <ctype.h>
#include "string.h"

#ifdef USE_STDWIN
#include "stdwin.h"
int use_stdwin;
#endif

#ifdef USE_AUDIO
#include "asa.h"
#endif

extern char *getenv();

#include "PROTO.h"
#include "grammar.h"
#include "node.h"
#include "parsetok.h"
#include "graminit.h"
#include "errcode.h"
#include "object.h"
#include "stringobject.h"
#include "sysmodule.h"

extern grammar gram; /* From graminit.c */

#ifndef PYTHONPATH

#ifdef THINK_C

#define PYTHONPATH ": :mod"

#else /* !THINK_C */

#ifdef AMOEBA
#define PYTHONPATH ".:/profile/module/python"
#else /* !AMOEBA */
#define PYTHONPATH ".:/usr/local/lib/python"
#endif /* !AMOEBA */

#endif /* !THINK_C */

#endif /* !PYTHONPATH */

int debugging;

main(argc, argv)
	int argc;
	char **argv;
{
	char *path;
	char *filename = NULL;
	FILE *fp = stdin;
	int ret;
	
#ifdef USE_STDWIN
#ifdef THINK_C
	wsetstdio(1);
#else THINK_C
	/* Must use "python -s" now to get stdwin support */
	if (argc > 1 && strcmp(argv[1], "-s") == 0)
		argv[1] = argv[0],
		argc--, argv++,
#endif /* !THINK_C */
		use_stdwin = 1;
	if (use_stdwin)
		winitargs(&argc, &argv);
#endif /* USE_STDWIN */
	
#ifdef THINK_C_not_today
	printf("argc = %d, argv[0] = '%s'\n", argc, argv[0]);
	if (argc <= 1)
		askargs(&argc, &argv);
#endif
	
	initintr(); /* For intrcheck() */
	
	if (argc > 1 && strcmp(argv[1], "-") != 0)
		filename = argv[1];

	if (filename != NULL) {
		if ((fp = fopen(filename, "r")) == NULL) {
			fprintf(stderr, "python: can't open file '%s'\n",
				filename);
			exit(2);
		}
	}
	
	/* XXX what is the ideal initialization order? */
	
	initsys(argc-1, argv+1);
	inittime();
	initmath();
	
#ifndef THINK_C
	path = getenv("PYTHONPATH");
	if (path == NULL)
#endif
		path = PYTHONPATH;
	setpythonpath(path);
	
	initrun();
	
#ifdef USE_POSIX
	initposix();
#endif

#ifdef THINK_C
	initmac();
#endif

#ifdef USE_AUDIO
	initaudio();
#endif
	
#ifdef USE_AMOEBA
	initamoeba();
#endif
	
#ifdef USE_STDWIN
	if (use_stdwin)
		initstdwin();
#endif
	
#ifdef USE_GL
	initgl();
#endif
	
#ifdef USE_PANEL
	initpanel();
#endif
	
	if (!isatty(fileno(fp))) {
		ret = runfile(fp, file_input, (char *)NULL, (char *)NULL);
	}
	else {
		sysset("ps1", newstringobject(">>> "));
		sysset("ps2", newstringobject("... "));
		for (;;) {
			object *v = sysget("ps1"), *w = sysget("ps2");
			char *ps1 = NULL, *ps2 = NULL;
			if (v != NULL && is_stringobject(v)) {
				INCREF(v);
				ps1 = getstringvalue(v);
			}
			else
				v = NULL;
			if (w != NULL && is_stringobject(w)) {
				INCREF(w);
				ps2 = getstringvalue(w);
			}
			else
				w = NULL;
			ret = runfile(fp, single_input, ps1, ps2);
			if (v != NULL)
				DECREF(v);
			if (w != NULL)
				DECREF(w);
			if (ret == E_EOF || ret == E_NOMEM)
				break;
		}
	}
	goaway(ret == E_DONE || ret == E_EOF ? 0 : 1);
	/*NOTREACHED*/
}

goaway(sts)
	int sts;
{
	closerun();
#ifdef USE_STDWIN
	if (use_stdwin)
		wdone();
#endif
#ifdef USE_AUDIO
	asa_done();
#endif
#ifdef THINK_C
#ifndef TRACE_REFS
	/* Avoid 'click mouse to continue' in Lightspeed C */
	if (sts == 0)
		Click_On(0);
#endif
#endif	
	exit(sts);
	/*NOTREACHED*/
}

/* Parse input from a file and execute it */

static int
runfile(fp, start, ps1, ps2)
	FILE *fp;
	int start;
	char *ps1, *ps2;
{
	node *n;
	int ret;
	ret = parsefile(fp, &gram, start, ps1, ps2, &n);
	if (ret != E_DONE)
		return ret;
	return execute(n) == 0 ? E_DONE : E_ERROR;
}

#ifdef THINK_C

/* Ask a yes/no question */

int
askyesno(prompt)
	char *prompt;
{
	char buf[256];
	
	printf("%s [ny] ", prompt);
	if (fgets(buf, sizeof buf, stdin) == NULL)
		return 0;
	return buf[0] == 'y' || buf[0] == 'Y';
}

/* Check for file descriptor connected to interactive device.
   Pretend that stdin is always interactive, other files never. */

int
isatty(fd)
	int fd;
{
	return fd == fileno(stdin);
}

/* Kludge to get arguments on the Mac */

#define MAXARGS 20

static char *
nextarg(pnext)
	char **pnext;
{
	char *ret;
	char *p = *pnext;
	while (isspace(*p))
		p++;
	if (*p == '\0')
		return NULL;
	ret = p;
	while (!isspace(*p))
		p++;
	if (*p != '\0')
		*p++ = '\0';
	*pnext = p;
	return ret;
}

static
askargs(pargc, pargv)
	int *pargc;
	char ***pargv; /* sic */
{
	static char buf[256];
	static char *argv[MAXARGS];
	int argc;
	char *p, *next;
	fprintf(stderr, "Args: ");
	if (fgets(buf, sizeof buf, stdin) == NULL)
		return;
	next = buf;
	if ((p = nextarg(&next)) == NULL)
		return;
	if (*pargc > 0)
		argv[0] = (*pargv)[0];
	else
		argv[0] = "PYTHON";
	argc = 1;
	argv[argc++] = p;
	while (argc+1 < MAXARGS && (p = nextarg(&next)) != NULL)
		argv[argc++] = p;
	argv[argc] = NULL;
	*pargc = argc;
	*pargv = argv;
}

#endif

/*	WISH LIST

	- improved per-module error handling; different use of errno
	- possible new types:
		- iterator (for range, keys, ...)
	- improve interpreter error handling, e.g., true tracebacks
	- release parse trees when no longer needed (make them objects?)
	- faster parser (for long modules)
	- save precompiled modules on file?
	- fork threads, locking
	- allow syntax extensions
*/
