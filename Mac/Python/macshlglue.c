/*
** Mac shared lib glue.
*/

#ifdef __powerc
#include <CPlusLibPPC.h>
#endif
#include <Quickdraw.h>
#include <SegLoad.h>
#include <FragLoad.h>
#include <Files.h>
#include <Resources.h>

#ifdef __MWERKS__
/*
** This part is copied from MW Startup.c, which is
**	Copyright © 1993 metrowerks inc. All Rights Reserved.
*/
#include <setjmp.h>
#include <stdio.h>

DestructorChain	*__local_destructor_chain;	/*	chain of local objects that need destruction	*/

	/*	public data		*/

QDGlobals qd;						/*	define the Quickdraw globals here!	*/
jmp_buf __program_exit;				/*	exit() does a longjmp() to here		*/
void (*__atexit_hook)(void);		/*	atexit()  sets this up if it is ever called	*/
void (*___atexit_hook)(void);		/*	_atexit() sets this up if it is ever called	*/
int __aborting;						/*	abort() sets this and longjmps to __program_exit	*/
#endif

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
__sinit(InitBlockPtr data)
{
	if ( data == nil ) return noErr;
	if ( data->fragLocator.where == kOnDiskFlat ) {
		library_fss = *data->fragLocator.u.onDisk.fileSpec;
		library_fss_valid = 1;
	} else if ( data->fragLocator.where == kOnDiskSegmented ) {
		library_fss = *data->fragLocator.u.inSegs.fileSpec;
		library_fss_valid = 1;
	}
	return noErr;
}

/*
** Insert the library resources into the search path. Put them after
** the resources from the application (which we assume is the current
** resource file). Again, we ignore errors.
*/
void
PyMac_AddLibResources()
{
	OSErr err;
	short fd, curfd;
	
	if ( !library_fss_valid ) 
		return;
	fd = FSpOpenResFile(&library_fss, fsRdPerm);
}

