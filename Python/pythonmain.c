/* Python interpreter main program */

#include <stdio.h>
#include <ctype.h>
#include "string.h"

extern char *getpythonpath();

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

int debugging;

main(argc, argv)
	int argc;
	char **argv;
{
	char *filename = NULL;
	FILE *fp = stdin;
	int ret;

	initargs(&argc, &argv);
	
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
	/* XXX exceptions are initialized by initrun but this
	   may be too late */
	
	initsys(argc-1, argv+1);
	inittime();
	initmath();
	
	setpythonpath(getpythonpath());
	
	initrun();
	initcalls();
	
	if (!isatty(fileno(fp))) {
		ret = runfile(fp, file_input, (char *)NULL, (char *)NULL);
	}
	else {
		object *v, *w;
		sysset("ps1", v = newstringobject(">>> "));
		sysset("ps2", w = newstringobject("... "));
		DECREF(v);
		DECREF(w);
		for (;;) {
			char *ps1 = NULL, *ps2 = NULL;
			v = sysget("ps1");
			w = sysget("ps2");
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
#ifdef THINK_C
	if (sts == 0)
		Click_On(0);
#endif
	closerun();
	donecalls();
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

#ifdef THINK_C

/* Check for file descriptor connected to interactive device.
   Pretend that stdin is always interactive, other files never. */

int
isatty(fd)
	int fd;
{
	return fd == fileno(stdin);
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
