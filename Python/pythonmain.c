/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
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

/* Python interpreter main program */

#include "allobjects.h"

extern int debugging; /* Needed in parser.c, declared in pythonrun.c */
extern int verbose; /* Needed in import.c, declared in pythonrun.c */
extern int suppress_print; /* Needed in ceval.c, declared in pythonrun.c */

/* Interface to getopt(): */
extern int optind;
extern char *optarg;
extern int getopt(); /* PROTO((int, char **, char *)); -- not standardized */

extern char *getenv();

extern char *getversion();
extern char *getcopyright();

int
realmain(argc, argv)
	int argc;
	char **argv;
{
	int c;
	int sts;
	char *command = NULL;
	char *filename = NULL;
	FILE *fp = stdin;
	char *p;
	int inspect = 0;
	int unbuffered = 0;

	if ((p = getenv("PYTHONDEBUG")) && *p != '\0')
		debugging = 1;
	if ((p = getenv("PYTHONSUPPRESS")) && *p != '\0')
		suppress_print = 1;
	if ((p = getenv("PYTHONVERBOSE")) && *p != '\0')
		verbose = 1;
	if ((p = getenv("PYTHONINSPECT")) && *p != '\0')
		inspect = 1;
	if ((p = getenv("PYTHONUNBUFFERED")) && *p != '\0')
		unbuffered = 1;

	while ((c = getopt(argc, argv, "c:disuv")) != EOF) {
		if (c == 'c') {
			/* -c is the last option; following arguments
			   that look like options are left for the
			   the command to interpret. */
			command = malloc(strlen(optarg) + 2);
			if (command == NULL)
				fatal("not enough memory to copy -c argument");
			strcpy(command, optarg);
			strcat(command, "\n");
			break;
		}
		
		switch (c) {

		case 'd':
			debugging++;
			break;

		case 'i':
			inspect++;
			break;

		case 's':
			suppress_print++;
			break;

		case 'u':
			unbuffered++;
			break;

		case 'v':
			verbose++;
			break;

		/* This space reserved for other options */

		default:
			fprintf(stderr,
"usage: %s [-d] [-i] [-s] [-u ] [-v] [-c cmd | file | -] [arg] ...\n",
				argv[0]);
			fprintf(stderr, "\
\n\
Options and arguments (and corresponding environment variables):\n\
-d     : debug output from parser (also PYTHONDEBUG=x)\n\
-i     : inspect interactively after running script (also PYTHONINSPECT=x)\n\
-s     : suppress the printing of top level expressions (also PYTHONSUPPRESS=x)\n\
-u     : unbuffered stdout and stderr (also PYTHONUNBUFFERED=x)\n\
-v     : verbose (trace import statements) (also PYTHONVERBOSE=x)\n\
-c cmd : program passed in as string (terminates option list)\n\
");
			/* ANSI does not allow strings > 512 chars
			   and MPW doesn't like it either -- so split it! */
			fprintf(stderr, "\
file   : program read from script file\n\
-      : program read from stdin (default; interactive mode if a tty)\n\
arg ...: arguments passed to program in sys.argv[1:]\n\
\n\
Other environment variables:\n\
PYTHONSTARTUP: file executed on interactive startup (no default)\n\
PYTHONPATH   : colon-separated list of directories prefixed to the\n\
               default module search path.  The result is sys.path.\n\
");
			exit(2);
			/*NOTREACHED*/

		}
	}

	if (unbuffered) {
#ifndef MPW
		setbuf(stdout, (char *)NULL);
		setbuf(stderr, (char *)NULL);
#else
		/* On MPW (3.2) unbuffered seems to hang */
		setvbuf(stdout, (char *)NULL, _IOLBF, BUFSIZ);
		setvbuf(stderr, (char *)NULL, _IOLBF, BUFSIZ);
#endif
	}

	if (command == NULL && optind < argc && strcmp(argv[optind], "-") != 0)
		filename = argv[optind];

	if (verbose ||
	    command == NULL && filename == NULL && isatty((int)fileno(fp)))
		fprintf(stderr, "Python %s\n%s\n",
			getversion(), getcopyright());
	
	if (filename != NULL) {
		if ((fp = fopen(filename, "r")) == NULL) {
			fprintf(stderr, "%s: can't open file '%s'\n",
				argv[0], filename);
			exit(2);
		}
	}
	
	initall();
	
	if (command != NULL) {
		/* Backup optind and force sys.argv[0] = '-c' */
		optind--;
		argv[optind] = "-c";
	}

	setpythonargv(argc-optind, argv+optind);

	if (command) {
		sts = run_command(command) != 0;
	}
	else {
		if (filename == NULL && isatty((int)fileno(fp))) {
			char *startup = getenv("PYTHONSTARTUP");
#ifdef macintosh
			if (startup == NULL)
				startup = "PythonStartup";
#endif
			if (startup != NULL && startup[0] != '\0') {
				FILE *fp = fopen(startup, "r");
				if (fp != NULL) {
					(void) run_script(fp, startup);
					err_clear();
					fclose(fp);
				}
			}
		}
		sts = run(fp, filename == NULL ? "<stdin>" : filename) != 0;
		if (filename != NULL)
			fclose(fp);
	}

	if (inspect && isatty((int)fileno(stdin)) &&
	    (filename != NULL || command != NULL))
		sts = run(stdin, "<stdin>") != 0;

	goaway(sts);
	/*NOTREACHED*/
}
