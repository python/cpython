
/* Readline interface for tokenizer.c and [raw_]input() in bltinmodule.c.
   By default, or when stdin is not a tty device, we have a super
   simple my_readline function using fgets.
   Optionally, we can use the GNU readline library.
   my_readline() has a different return value from GNU readline():
   - NULL if an interrupt occurred or if an error occurred
   - a malloc'ed empty string if EOF was read
   - a malloc'ed string ending in \n normally
*/

#include "Python.h"

int (*PyOS_InputHook)(void) = NULL;

/* This function restarts a fgets() after an EINTR error occurred
   except if PyOS_InterruptOccurred() returns true. */

static int
my_fgets(char *buf, int len, FILE *fp)
{
	char *p;
	for (;;) {
		if (PyOS_InputHook != NULL)
			(void)(PyOS_InputHook)();
		errno = 0;
		p = fgets(buf, len, fp);
		if (p != NULL)
			return 0; /* No error */
		if (feof(fp)) {
			return -1; /* EOF */
		}
#ifdef EINTR
		if (errno == EINTR) {
			if (PyOS_InterruptOccurred()) {
				return 1; /* Interrupt */
			}
			continue;
		}
#endif
		if (PyOS_InterruptOccurred()) {
			return 1; /* Interrupt */
		}
		return -2; /* Error */
	}
	/* NOTREACHED */
}


/* Readline implementation using fgets() */

char *
PyOS_StdioReadline(char *prompt)
{
	size_t n;
	char *p;
	n = 100;
	if ((p = PyMem_MALLOC(n)) == NULL)
		return NULL;
	fflush(stdout);
	if (prompt)
		fprintf(stderr, "%s", prompt);
	fflush(stderr);
	switch (my_fgets(p, (int)n, stdin)) {
	case 0: /* Normal case */
		break;
	case 1: /* Interrupt */
		PyMem_FREE(p);
		return NULL;
	case -1: /* EOF */
	case -2: /* Error */
	default: /* Shouldn't happen */
		*p = '\0';
		break;
	}
#ifdef MPW
	/* Hack for MPW C where the prompt comes right back in the input */
	/* XXX (Actually this would be rather nice on most systems...) */
	n = strlen(prompt);
	if (strncmp(p, prompt, n) == 0)
		memmove(p, p + n, strlen(p) - n + 1);
#endif
	n = strlen(p);
	while (n > 0 && p[n-1] != '\n') {
		size_t incr = n+2;
		p = PyMem_REALLOC(p, n + incr);
		if (p == NULL)
			return NULL;
		if (incr > INT_MAX) {
			PyErr_SetString(PyExc_OverflowError, "input line too long");
		}
		if (my_fgets(p+n, (int)incr, stdin) != 0)
			break;
		n += strlen(p+n);
	}
	return PyMem_REALLOC(p, n+1);
}


/* By initializing this function pointer, systems embedding Python can
   override the readline function.

   Note: Python expects in return a buffer allocated with PyMem_Malloc. */

char *(*PyOS_ReadlineFunctionPointer)(char *);


/* Interface used by tokenizer.c and bltinmodule.c */

char *
PyOS_Readline(char *prompt)
{
	char *rv;
	if (PyOS_ReadlineFunctionPointer == NULL) {
			PyOS_ReadlineFunctionPointer = PyOS_StdioReadline;
	}
	Py_BEGIN_ALLOW_THREADS
	rv = (*PyOS_ReadlineFunctionPointer)(prompt);
	Py_END_ALLOW_THREADS
	return rv;
}
