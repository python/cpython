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

#include <windows.h>
#include <direct.h>

#include "Python.h"
#include "importdl.h"

const struct filedescr _PyImport_DynLoadFiletab[] = {
#ifdef _DEBUG
	{"_d.pyd", "rb", C_EXTENSION},
	{"_d.dll", "rb", C_EXTENSION},
#else
	{".pyd", "rb", C_EXTENSION},
	{".dll", "rb", C_EXTENSION},
#endif
	{0, 0}
};


dl_funcptr _PyImport_GetDynLoadFunc(const char *fqname, const char *shortname,
				    const char *pathname, FILE *fp)
{
	dl_funcptr p;
	char funcname[258];

	sprintf(funcname, "init%.200s", shortname);

#ifdef MS_WIN32
	{
		HINSTANCE hDLL;
		char pathbuf[260];
		if (strchr(pathname, '\\') == NULL &&
		    strchr(pathname, '/') == NULL)
		{
			/* Prefix bare filename with ".\" */
			char *p = pathbuf;
			*p = '\0';
			_getcwd(pathbuf, sizeof pathbuf);
			if (*p != '\0' && p[1] == ':')
				p += 2;
			sprintf(p, ".\\%-.255s", pathname);
			pathname = pathbuf;
		}
		/* Look for dependent DLLs in directory of pathname first */
		/* XXX This call doesn't exist in Windows CE */
		hDLL = LoadLibraryEx(pathname, NULL,
				     LOAD_WITH_ALTERED_SEARCH_PATH);
		if (hDLL==NULL){
			char errBuf[256];
			unsigned int errorCode;

			/* Get an error string from Win32 error code */
			char theInfo[256]; /* Pointer to error text
					      from system */
			int theLength; /* Length of error text */

			errorCode = GetLastError();

			theLength = FormatMessage(
				FORMAT_MESSAGE_FROM_SYSTEM, /* flags */
				NULL, /* message source */
				errorCode, /* the message (error) ID */
				0, /* default language environment */
				(LPTSTR) theInfo, /* the buffer */
				sizeof(theInfo), /* the buffer size */
				NULL); /* no additional format args. */

			/* Problem: could not get the error message.
			   This should not happen if called correctly. */
			if (theLength == 0) {
				sprintf(errBuf,
					"DLL load failed with error code %d",
					errorCode);
			} else {
				int len;
				/* For some reason a \r\n
				   is appended to the text */
				if (theLength >= 2 &&
				    theInfo[theLength-2] == '\r' &&
				    theInfo[theLength-1] == '\n') {
					theLength -= 2;
					theInfo[theLength] = '\0';
				}
				strcpy(errBuf, "DLL load failed: ");
				len = strlen(errBuf);
				strncpy(errBuf+len, theInfo,
					sizeof(errBuf)-len);
				errBuf[sizeof(errBuf)-1] = '\0';
			}
			PyErr_SetString(PyExc_ImportError, errBuf);
		return NULL;
		}
		p = GetProcAddress(hDLL, funcname);
	}
#endif /* MS_WIN32 */
#ifdef MS_WIN16
	{
		HINSTANCE hDLL;
		char pathbuf[16];
		if (strchr(pathname, '\\') == NULL &&
		    strchr(pathname, '/') == NULL)
		{
			/* Prefix bare filename with ".\" */
			sprintf(pathbuf, ".\\%-.13s", pathname);
			pathname = pathbuf;
		}
		hDLL = LoadLibrary(pathname);
		if (hDLL < HINSTANCE_ERROR){
			char errBuf[256];
			sprintf(errBuf,
				"DLL load failed with error code %d", hDLL);
			PyErr_SetString(PyExc_ImportError, errBuf);
			return NULL;
		}
		p = GetProcAddress(hDLL, funcname);
	}
#endif /* MS_WIN16 */

	return p;
}
