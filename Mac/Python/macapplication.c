/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
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

/* Macintosh Python main program for both applets and interpreter */

#include <Resources.h>
#include <CodeFragments.h>

#ifdef __CFM68K__
#pragma lib_export on
#endif

extern void PyMac_InitApplication(void);
#ifdef USE_MAC_APPLET_SUPPORT
extern void PyMac_InitApplet(void);
#endif /* USE_MAC_APPLET_SUPPORT */

/* From the MSL runtime: */
extern void __initialize(void);

/*
** Alternative initialization entry point for some very special cases.
** Use this in stead of __initialize in the PEF settings to remember (and
** re-open as resource file) the application. This is needed if we link against
** a dynamic library that, in its own __initialize routine, opens a resource
** file. This would mess up our finding of override preferences.
** Only set this entrypoint in your apps if you notice sys.path or some such is
** messed up.
*/
static int application_fss_valid;
static FSSpec application_fss;

OSErr pascal
__initialize_remember_app_fsspec(CFragInitBlockPtr data)
{
	/* Call the MW runtime's initialization routine */
	__initialize();
	if ( data == nil ) return noErr;
	if ( data->fragLocator.where == kDataForkCFragLocator ) {
		application_fss = *data->fragLocator.u.onDisk.fileSpec;
		application_fss_valid = 1;
	} else if ( data->fragLocator.where == kResourceCFragLocator ) {
		application_fss = *data->fragLocator.u.inSegs.fileSpec;
		application_fss_valid = 1;
	}
	return noErr;
}

void
main() {
	if ( application_fss_valid )
			(void)FSpOpenResFile(&application_fss, fsRdPerm);
#ifdef USE_MAC_APPLET_SUPPORT
	{
        Handle mainpyc;

        mainpyc = Get1NamedResource('PYC ', "\p__main__");
        if (mainpyc != NULL)
                PyMac_InitApplet();
        else
                PyMac_InitApplication();
    }
#else
	PyMac_InitApplication();
#endif /* USE_MAC_APPLET_SUPPORT */
}
