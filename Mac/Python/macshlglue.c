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

/*
** Shared library initialization code.
**
** This code calls the MetroWerks shared-library initialization code
** and performs one extra step: it remembers the FSSpec of the file
** we are loaded from, so we can later call PyMac_AddLibResources to
** add the file to our resource file chain.
**
** This file is needed for PythonCore and for any dynamically loaded
** module that has interesting resources in its .slb file.
** Use by replacing __initialize in the "CFM preferences" init field
** by __initialize_with_resources.
*/

#include <Types.h>
#include <Quickdraw.h>
#include <SegLoad.h>
#include <CodeFragments.h>
#include <Files.h>
#include <Resources.h>

/*
** Variables passed from shared lib initialization to PyMac_AddLibResources.
*/
static int library_fss_valid;
static FSSpec library_fss;

/*
** Routine called upon fragment load. We attempt to save the FSSpec from which we're
** loaded. We always return noErr (we just continue without the resources).
*/
OSErr pascal
__initialize_with_resources(CFragInitBlockPtr data)
{
	/* Call the MW runtime's initialization routine */
/* #ifdef __CFM68K__ */
#if 1
	__initialize();
#else
	__sinit();
#endif
	
	if ( data == nil ) return noErr;
	if ( data->fragLocator.where == kDataForkCFragLocator ) {
		library_fss = *data->fragLocator.u.onDisk.fileSpec;
		library_fss_valid = 1;
	} else if ( data->fragLocator.where == kResourceCFragLocator ) {
		library_fss = *data->fragLocator.u.inSegs.fileSpec;
		library_fss_valid = 1;
	}
	return noErr;
}

/*
** compare two FSSpecs, return true if equal, false if different
** XXX where could this function live? (jvr)
*/

static int
FSpCompare(FSSpec *fss1, FSSpec *fss2) {
	if (fss1->vRefNum != fss2->vRefNum)
		return 0;
	if (fss1->parID != fss2->parID)
		return 0;
	return !PLstrcmp(fss1->name, fss2->name);
}

/* XXX can't include "macglue.h" somehow (jvr) */
extern FSSpec PyMac_ApplicationFSSpec;		/* Application location (from macargv.c) */

/*
** Insert the library resources into the search path. Put them after
** the resources from the application (which we assume is the current
** resource file). Again, we ignore errors.
*/
void
PyMac_AddLibResources()
{
	if ( !library_fss_valid || FSpCompare(&library_fss, &PyMac_ApplicationFSSpec))
		return;
	(void)FSpOpenResFile(&library_fss, fsRdPerm);
}

/*
** Dummy main() program to keep linker happy: we want to
** use the MW AppRuntime in our shared library (better than building
** custom runtime libraries as we did before) but AppRuntime
** expects a main program. Note that it 
*/

#pragma export off
int
main(int argc, char **argv) {
	DebugStr("\pCannot happen: PythonCore dummy main called!");
}
#pragma export reset
