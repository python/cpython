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

/* Parser generator main program */

/* This expects a filename containing the grammar as argv[1] (UNIX)
   or asks the console for such a file name (THINK C).
   It writes its output on two files in the current directory:
   - "graminit.c" gets the grammar as a bunch of initialized data
   - "graminit.h" gets the grammar's non-terminals as #defines.
   Error messages and status info during the generation process are
   written to stdout, or sometimes to stderr. */

#include "pgenheaders.h"
#include "grammar.h"
#include "node.h"
#include "parsetok.h"
#include "pgen.h"

int debugging;

/* Forward */
grammar *getgrammar PROTO((char *filename));
#ifdef THINK_C
int main PROTO((int, char **));
char *askfile PROTO((void));
#endif

int
main(argc, argv)
	int argc;
	char **argv;
{
	grammar *g;
	node *n;
	FILE *fp;
	char *filename;
	
#ifdef THINK_C
	filename = askfile();
#else
	if (argc != 2) {
		fprintf(stderr, "usage: %s grammar\n", argv[0]);
		exit(2);
	}
	filename = argv[1];
#endif
	g = getgrammar(filename);
	fp = fopen("graminit.c", "w");
	if (fp == NULL) {
		perror("graminit.c");
		exit(1);
	}
	printf("Writing graminit.c ...\n");
	printgrammar(g, fp);
	fclose(fp);
	fp = fopen("graminit.h", "w");
	if (fp == NULL) {
		perror("graminit.h");
		exit(1);
	}
	printf("Writing graminit.h ...\n");
	printnonterminals(g, fp);
	fclose(fp);
	exit(0);
}

grammar *
getgrammar(filename)
	char *filename;
{
	FILE *fp;
	node *n;
	grammar *g0, *g;
	
	fp = fopen(filename, "r");
	if (fp == NULL) {
		perror(filename);
		exit(1);
	}
	g0 = meta_grammar();
	n = NULL;
	parsefile(fp, filename, g0, g0->g_start, (char *)NULL, (char *)NULL, &n);
	fclose(fp);
	if (n == NULL) {
		fprintf(stderr, "Parsing error.\n");
		exit(1);
	}
	g = pgen(n);
	if (g == NULL) {
		printf("Bad grammar.\n");
		exit(1);
	}
	return g;
}

#ifdef THINK_C
char *
askfile()
{
	char buf[256];
	static char name[256];
	printf("Input file name: ");
	if (fgets(buf, sizeof buf, stdin) == NULL) {
		printf("EOF\n");
		exit(1);
	}
	/* XXX The (unsigned char *) case is needed by THINK C 3.0 */
	if (sscanf(/*(unsigned char *)*/buf, " %s ", name) != 1) {
		printf("No file\n");
		exit(1);
	}
	return name;
}
#endif

void
fatal(msg)
	char *msg;
{
	fprintf(stderr, "pgen: FATAL ERROR: %s\n", msg);
	exit(1);
}

/* XXX TO DO:
   - check for duplicate definitions of names (instead of fatal err)
*/
