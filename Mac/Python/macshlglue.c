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

#include <Quickdraw.h>
#include <SegLoad.h>
#include <FragLoad.h>
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
__initialize_with_resources(InitBlockPtr data)
{
	/* Call the MW runtime's initialization routine */
/* #ifdef __CFM68K__ */
#if 1
	__initialize();
#else
	__sinit();
#endif
	
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
	if ( !library_fss_valid ) 
		return;
	(void)FSpOpenResFile(&library_fss, fsRdPerm);
}

