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

/* Return the initial module search path. */

#include "Python.h"
#include "osdefs.h"


#ifndef PYTHONPATH
#define PYTHONPATH "/usr/local/lib/python"
#endif

#ifndef PREFIX
#define PREFIX "/usr/local"
#endif

#ifndef EXEC_PREFIX
#define EXEC_PREFIX PREFIX
#endif


/* This is called once from pythonrun to initialize sys.path.  The
   environment variable PYTHONPATH is fetched and the default path
   appended.  The default path may be passed to the preprocessor; if
   not, a hardcoded default is used, which only makes (some) sense on
   Unix. */

char *
Py_GetPath()
{
	char *path = getenv("PYTHONPATH");
	char *defpath = PYTHONPATH;
	static char *buf = NULL;
	char *p;
	int n;

	if (path == NULL)
		path = "";
	n = strlen(path) + strlen(defpath) + 2;
	if (buf != NULL) {
		free(buf);
		buf = NULL;
	}
	buf = malloc(n);
	if (buf == NULL)
		Py_FatalError("not enough memory to copy module search path");
	strcpy(buf, path);
	p = buf + strlen(buf);
	if (p != buf)
		*p++ = DELIM;
	strcpy(p, defpath);
	return buf;
}


/* Similar for Makefile variables $prefix and $exec_prefix */

char *
Py_GetPrefix()
{
	return PREFIX;
}

char *
Py_GetExecPrefix()
{
	return EXEC_PREFIX;
}
