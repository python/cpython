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

/* Python interpreter main program */

#include "allobjects.h"

extern int debugging; /* Needed by parser.c */
extern int verbose; /* Needed by import.c */

/* Interface to getopt(): */
extern int optind;
extern char *optarg;
extern int getopt PROTO((int, char **, char *));

main(argc, argv)
	int argc;
	char **argv;
{
	int c;
	int sts;
	char *command = NULL;
	char *filename = NULL;
	FILE *fp = stdin;
	
	initargs(&argc, &argv); /* Defined in config*.c */

	while ((c = getopt(argc, argv, "c:dv")) != EOF) {
		if (c == 'c') {
			/* -c is the last option; following arguments
			   that look like options are left for the
			   the command to interpret. */
			command = malloc(strlen(optarg) + 2);
			/* Ignore malloc errors this early... */
			strcpy(command, optarg);
			strcat(command, "\n");
			break;
		}
		
		switch (c) {

		case 'd':
			debugging++;
			break;

		case 'v':
			verbose++;
			break;

		/* This space reserved for other options */

		default:
			fprintf(stderr,
				"usage: %s [-c cmd | file | -] [arg] ...\n",
				argv[0]);
			exit(2);
			/*NOTREACHED*/

		}
	}
	
	if (command == NULL && optind < argc && strcmp(argv[optind], "-") != 0)
		filename = argv[optind];
	
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
			if (startup != NULL && startup[0] != '\0') {
				FILE *fp = fopen(startup, "r");
				if (fp != NULL) {
					(void) run_script(fp, startup);
					err_clear();
				}
			}
		}
		sts = run(fp, filename == NULL ? "<stdin>" : filename) != 0;
	}

	goaway(sts);
	/*NOTREACHED*/
}
