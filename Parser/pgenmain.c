
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
grammar *getgrammar(char *filename);
#ifdef THINK_C
int main(int, char **);
char *askfile(void);
#endif

void
Py_Exit(int sts)
{
	exit(sts);
}

int
main(int argc, char **argv)
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
getgrammar(char *filename)
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
			size_t i;
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
			PyMem_DEL(err.text);
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
askfile(void)
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
Py_FatalError(char *msg)
{
	fprintf(stderr, "pgen: FATAL ERROR: %s\n", msg);
	Py_Exit(1);
}

#ifdef macintosh
/* ARGSUSED */
int
guesstabsize(char *path)
{
	return 4;
}
#endif

/* No-nonsense my_readline() for tokenizer.c */

char *
PyOS_Readline(char *prompt)
{
	size_t n = 1000;
	char *p = PyMem_MALLOC(n);
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
	return PyMem_REALLOC(p, n+1);
}

#include <stdarg.h>

void
PySys_WriteStderr(const char *format, ...)
{
	va_list va;

	va_start(va, format);
	vfprintf(stderr, format, va);
	va_end(va);
}
