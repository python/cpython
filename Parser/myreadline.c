
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
#ifdef MS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif /* MS_WINDOWS */

int (*PyOS_InputHook)(void) = NULL;

#ifdef RISCOS
int Py_RISCOSWimpFlag;
#endif

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
#ifdef MS_WINDOWS
		/* In the case of a Ctrl+C or some other external event 
		   interrupting the operation:
		   Win2k/NT: ERROR_OPERATION_ABORTED is the most recent Win32 
		   error code (and feof() returns TRUE).
		   Win9x: Ctrl+C seems to have no effect on fgets() returning
		   early - the signal handler is called, but the fgets()
		   only returns "normally" (ie, when Enter hit or feof())
		*/
		if (GetLastError()==ERROR_OPERATION_ABORTED) {
			/* Signals come asynchronously, so we sleep a brief 
			   moment before checking if the handler has been 
			   triggered (we cant just return 1 before the 
			   signal handler has been called, as the later 
			   signal may be treated as a separate interrupt).
			*/
			Sleep(1);
			if (PyOS_InterruptOccurred()) {
				return 1; /* Interrupt */
			}
			/* Either the sleep wasn't long enough (need a
			   short loop retrying?) or not interrupted at all
			   (in which case we should revisit the whole thing!)
			   Logging some warning would be nice.  assert is not
			   viable as under the debugger, the various dialogs
			   mean the condition is not true.
			*/
		}
#endif /* MS_WINDOWS */
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
#ifndef RISCOS
	if (prompt)
		fprintf(stderr, "%s", prompt);
#else
	if (prompt) {
		if(Py_RISCOSWimpFlag)
			fprintf(stderr, "\x0cr%s\x0c", prompt);
		else
			fprintf(stderr, "%s", prompt);
	}
#endif
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
