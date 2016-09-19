
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

#define PGEN

#include "Python.h"
#include "pgenheaders.h"
#include "grammar.h"
#include "node.h"
#include "parsetok.h"
#include "pgen.h"

int Py_DebugFlag;
int Py_VerboseFlag;
int Py_IgnoreEnvironmentFlag;

/* Forward */
grammar *getgrammar(const char *filename);

void Py_Exit(int) _Py_NO_RETURN;

void
Py_Exit(int sts)
{
    exit(sts);
}

#ifdef WITH_THREAD
/* Needed by obmalloc.c */
int PyGILState_Check(void)
{ return 1; }
#endif

void _PyMem_DumpTraceback(int fd, const void *ptr)
{}

int
main(int argc, char **argv)
{
    grammar *g;
    FILE *fp;
    char *filename, *graminit_h, *graminit_c;

    if (argc != 4) {
        fprintf(stderr,
            "usage: %s grammar graminit.h graminit.c\n", argv[0]);
        Py_Exit(2);
    }
    filename = argv[1];
    graminit_h = argv[2];
    graminit_c = argv[3];
    g = getgrammar(filename);
    fp = fopen(graminit_c, "w");
    if (fp == NULL) {
        perror(graminit_c);
        Py_Exit(1);
    }
    if (Py_DebugFlag)
        printf("Writing %s ...\n", graminit_c);
    printgrammar(g, fp);
    fclose(fp);
    fp = fopen(graminit_h, "w");
    if (fp == NULL) {
        perror(graminit_h);
        Py_Exit(1);
    }
    if (Py_DebugFlag)
        printf("Writing %s ...\n", graminit_h);
    printnonterminals(g, fp);
    fclose(fp);
    freegrammar(g);
    Py_Exit(0);
    return 0; /* Make gcc -Wall happy */
}

grammar *
getgrammar(const char *filename)
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
            size_t len;
            int i;
            fprintf(stderr, "%s", err.text);
            len = strlen(err.text);
            if (len == 0 || err.text[len-1] != '\n')
                fprintf(stderr, "\n");
            for (i = 0; i < err.offset; i++) {
                if (err.text[i] == '\t')
                    putc('\t', stderr);
                else
                    putc(' ', stderr);
            }
            fprintf(stderr, "^\n");
            PyObject_FREE(err.text);
        }
        Py_Exit(1);
    }
    g = pgen(n);
    PyNode_Free(n);
    if (g == NULL) {
        printf("Bad grammar.\n");
        Py_Exit(1);
    }
    return g;
}

/* Can't happen in pgen */
PyObject*
PyErr_Occurred()
{
    return 0;
}

void
Py_FatalError(const char *msg)
{
    fprintf(stderr, "pgen: FATAL ERROR: %s\n", msg);
    Py_Exit(1);
}

/* No-nonsense my_readline() for tokenizer.c */

char *
PyOS_Readline(FILE *sys_stdin, FILE *sys_stdout, const char *prompt)
{
    size_t n = 1000;
    char *p = (char *)PyMem_MALLOC(n);
    char *q;
    if (p == NULL)
        return NULL;
    fprintf(stderr, "%s", prompt);
    q = fgets(p, n, sys_stdin);
    if (q == NULL) {
        *p = '\0';
        return p;
    }
    n = strlen(p);
    if (n > 0 && p[n-1] != '\n')
        p[n-1] = '\n';
    return (char *)PyMem_REALLOC(p, n+1);
}

/* No-nonsense fgets */
char *
Py_UniversalNewlineFgets(char *buf, int n, FILE *stream, PyObject *fobj)
{
    return fgets(buf, n, stream);
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
