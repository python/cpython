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

#include "pyconfig.h"
#include "Python.h"

#ifdef __MWERKS__
#ifdef USE_GUSI2
#define HASGUSI " GUSI2"
#else
#define HASGUSI ""
#endif

#ifdef WITH_THREAD
#define HASTHREAD " THREADS"
#else
#define HASTHREAD ""
#endif

#if TARGET_API_MAC_CARBON
#define TARGET_API " CARBON"
#else
#define TARGET_API " PPC"
#endif

#ifdef WITH_CYCLE_GC
#define HASGC " GC"
#else
#define HASGC ""
#endif

#define COMPILER " [CW" TARGET_API HASGUSI HASTHREAD HASGC"]"
#endif

#ifdef MPW
#define COMPILER " [Apple MPW]"
#endif

const char *
Py_GetCompiler(void)
{
	return COMPILER;
}
