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

/* Support for dynamic loading of extension modules */

#define  INCL_DOSERRORS
#define  INCL_DOSMODULEMGR
#include <os2.h>

#include "Python.h"
#include "importdl.h"


const struct filedescr _PyImport_DynLoadFiletab[] = {
	{".pyd", "rb", C_EXTENSION},
	{".dll", "rb", C_EXTENSION},
	{0, 0}
};

dl_funcptr _PyImport_GetDynLoadFunc(const char *name, const char *funcname,
				    const char *pathname, FILE *fp)
{
	dl_funcptr p;
	APIRET  rc;
	HMODULE hDLL;
	char failreason[256];

	rc = DosLoadModule(failreason,
			   sizeof(failreason),
			   pathname,
			   &hDLL);

	if (rc != NO_ERROR) {
		char errBuf[256];
		sprintf(errBuf,
			"DLL load failed, rc = %d: %s",
			rc, failreason);
		PyErr_SetString(PyExc_ImportError, errBuf);
		return NULL;
	}

	rc = DosQueryProcAddr(hDLL, 0L, funcname, &p);
	if (rc != NO_ERROR)
		p = NULL; /* Signify Failure to Acquire Entrypoint */
	return p;
}
