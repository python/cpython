/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
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

/* Return a string representing the compiler name */

#include "config.h"

#ifdef THINK_C
#define COMPILER " [THINK C]"
#endif

#ifdef __MWERKS__
#ifdef USE_GUSI
#define HASGUSI " w/GUSI"
#else
#define HASGUSI ""
#endif
#ifdef USE_MSL
#define HASMSL " w/MSL"
#else
#define HASMSL ""
#endif
#ifdef __powerc
#define COMPILER " [CW PPC" HASGUSI HASMSL "]"
#else
#ifdef __CFM68K__
#define COMPILER " [CW CFM68K" HASGUSI HASMSL "]"
#else
#define COMPILER " [CW 68K" HASGUSI HASMSL "]"
#endif
#endif
#endif

#ifdef MPW
#ifdef __SC__
#define COMPILER " [Symantec MPW]"
#else
#define COMPILER " [Apple MPW]"
#endif
#endif

char *
Py_GetCompiler()
{
	return COMPILER;
}
