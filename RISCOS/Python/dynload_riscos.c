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

/* This module provides the necessary stubs for when dynamic loading is
   not present. */

#include "Python.h"
#include "importdl.h"

#include "dlk.h"


const struct filedescr _PyImport_DynLoadFiletab[] = {
	{"/pyd", "rb", C_EXTENSION},
	{0, 0}
};

void dynload_init_dummy()
{
}

dl_funcptr _PyImport_GetDynLoadFunc(const char *fqname, const char *shortname,
				    char *pathname, FILE *fp)
{
	int err;
	char errstr[256];
	void (*init_function)(void);

	err = dlk_load_no_init(pathname, &init_function);
	if (err) {
	    PyOS_snprintf(errstr, sizeof(errstr), "dlk failure %d", err);
	    PyErr_SetString(PyExc_ImportError, errstr);
	}
	return init_function;
}
