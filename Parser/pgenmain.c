/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Parser generator main program */

/* This expects a filename containing the grammar as argv[1] (UNIX)
   or asks the console for such a file name (THINK C).
   It writes its output on two files in the current directory:
   - "graminit.c" gets the grammar as a bunch of initialized data
   - "graminit.h" gets the grammar's non-terminals as #defines.
   Error messages and status info during the generation process are
   written to stdout, or sometimes to stderr. */

/* XXX TO DO:
   - check for duplicate definitions of names (instead of fatal err)
*/

#include "pgenheaders.h"
#include "grammar.h"
#include "node.h"
#include "parsetok.h"
#include "pgen.h"

int Py_DebugFlag;
int Py_VerboseFlag;

/* Forward */
grammar *getgrammar Py_PROTO((char *filename));
#ifdef THINK_C
int main Py_PROTO((int, char **));
char *askfile Py_PROTO((void));
#endif

void
Py_Exit(sts)
	int sts;
{
	exit(sts);
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	grammar *g;
	FILE *fp;
	char *filename;
	
#ifdef THINK_C
	filename = askfile();
#else
	if (argc != 2) {
		fprintf(stderr, "usage: %s grammar\n", argv[0]);
		Py_Exit(2);
	}
	filename = argv[1];
#endif
	g = getgrammar(filename);
	fp = fopen("graminit.c", "w");
	if (fp == NULL) {
		perror("graminit.c");
		Py_Exit(1);
	}
	printf("Writing graminit.c ...\n");
	printgrammar(g, fp);
	fclose(fp);
	fp = fopen("graminit.h", "w");
	if (fp == NULL) {
		perror("graminit.h");
		Py_Exit(1);
	}
	printf("Writing graminit.h ...\n");
	printnonterminals(g, fp);
	fclose(fp);
	Py_Exit(0);
	return 0; /* Make gcc -Wall happy */
}

grammar *
getgrammar(filename)
	char *filename;
{
	FILE *fp;
	node *n;
	grammar *g0, *g;
	perrdetail err;
	
	fp = fopen(filename, "r");
	if (fp == NULL) {
		perror(filename);
		Py_Exit(1);
	}
	g0 = meta_grammar();
	n = PyParser_ParseFile(fp, filename, g0, g0->g_start,
		      (char *)NULL, (char *)NULL, &err);
	fclose(fp);
	if (n == NULL) {
		fprintf(stderr, "Parsing error %d, line %d.\n",
			err.error, err.lineno);
		if (err.text != NULL) {
			int i;
			fprintf(stderr, "%s", err.text);
			i = strlen(err.text);
			if (i == 0 || err.text[i-1] != '\n')
				fprintf(stderr, "\n");
			for (i = 0; i < err.offset; i++) {
				if (err.text[i] == '\t')
					putc('\t', stderr);
				else
					putc(' ', stderr);
			}
			fprintf(stderr, "^\n");
			free(err.text);
		}
		Py_Exit(1);
	}
	g = pgen(n);
	if (g == NULL) {
		printf("Bad grammar.\n");
		Py_Exit(1);
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
		Py_Exit(1);
	}
	/* XXX The (unsigned char *) case is needed by THINK C 3.0 */
	if (sscanf(/*(unsigned char *)*/buf, " %s ", name) != 1) {
		printf("No file\n");
		Py_Exit(1);
	}
	return name;
}
#endif

void
Py_FatalError(msg)
	char *msg;
{
	fprintf(stderr, "pgen: FATAL ERROR: %s\n", msg);
	Py_Exit(1);
}

#ifdef macintosh
/* ARGSUSED */
int
guesstabsize(path)
	char *path;
{
	return 4;
}
#endif

/* No-nonsense my_readline() for tokenizer.c */

char *
PyOS_Readline(prompt)
	char *prompt;
{
	int n = 1000;
	char *p = malloc(n);
	char *q;
	if (p == NULL)
		return NULL;
	fprintf(stderr, "%s", prompt);
	q = fgets(p, n, stdin);
	if (q == NULL) {
		*p = '\0';
		return p;
	}
	n = strlen(p);
	if (n > 0 && p[n-1] != '\n')
		p[n-1] = '\n';
	return realloc(p, n+1);
}

#ifdef HAVE_STDARG_PROTOTYPES
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#ifdef HAVE_STDARG_PROTOTYPES
PySys_WriteStderr(const char *format, ...)
#else
PySys_WriteStderr(va_alist)
	va_dcl
#endif
{
	va_list va;

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(va, format);
#else
	char *format;
	va_start(va);
	format = va_arg(va, char *);
#endif
	vfprintf(stderr, format, va);
	va_end(va);
}
