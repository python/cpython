/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Macintosh Applet Python main program */

#include "Python.h"
#include "marshal.h"
#include "import.h"

#include <Memory.h>
#include <Resources.h>
#include <QuickDraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <Menus.h>

static void
init_mac_world()
{
	MaxApplZone();
	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	TEInit();
	InitDialogs((long)0);
	InitMenus();
	InitCursor();
}

static int
run_main_resource()
{
	Handle h;
	long size;
	PyObject *code;
	PyObject *result;
	
	h = GetNamedResource('PYC ', "\p__main__");
	if (h == NULL) {
		fprintf(stderr, "No 'PYC ' resource named __main__ found\n");
		return 1;
	}
	size = GetResourceSizeOnDisk(h);
	HLock(h);
	code = PyMarshal_ReadObjectFromString(*h + 8, (int)(size - 8));
	HUnlock(h);
	ReleaseResource(h);
	if (code == NULL) {
		PyErr_Print();
		return 1;
	}
	result = PyImport_ExecCodeModule("__main__", code);
	Py_DECREF(code);
	if (result == NULL) {
		PyErr_Print();
		return 1;
	}
	Py_DECREF(result);
	return 0;
}

main()
{
	static char *argv[] = {"__main__", NULL};
	
	init_mac_world();
	Py_Initialize();
	PySys_SetArgv((sizeof argv / sizeof argv[0]) - 1, argv);
	run_main_resource();
	fflush(stderr);
	fflush(stdout);
	/* XXX Should we bother to Py_Exit(sts)? */
}
