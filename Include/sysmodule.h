#ifndef Py_SYSMODULE_H
#define Py_SYSMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

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

/* System module interface */

DL_IMPORT(PyObject *) PySys_GetObject Py_PROTO((char *));
DL_IMPORT(int) PySys_SetObject Py_PROTO((char *, PyObject *));
DL_IMPORT(FILE *) PySys_GetFile Py_PROTO((char *, FILE *));
DL_IMPORT(void) PySys_SetArgv Py_PROTO((int, char **));
DL_IMPORT(void) PySys_SetPath Py_PROTO((char *));

#ifdef HAVE_STDARG_PROTOTYPES
DL_IMPORT(void) PySys_WriteStdout(const char *format, ...);
DL_IMPORT(void) PySys_WriteStderr(const char *format, ...);
#else
/* Better to have no prototypes at all for varargs functions in this case */
DL_IMPORT(void) PySys_WriteStdout();
DL_IMPORT(void) PySys_WriteStderr();
#endif

extern DL_IMPORT(PyObject *) _PySys_TraceFunc, *_PySys_ProfileFunc;
extern DL_IMPORT(int) _PySys_CheckInterval;

#ifdef __cplusplus
}
#endif
#endif /* !Py_SYSMODULE_H */
