/*
 * Generated with the GUSIConfig application and then hand-modified by jack.
 */

#define GUSI_SOURCE
#include <GUSIConfig.h>
#include <sys/cdefs.h>
#include <Resources.h>

#include "Python.h"
#include "macglue.h"
#include "pythonresources.h"

static void
PyMac_GUSISpin(bool wait)
{
	static Boolean	inForeground = true;
	int		maxsleep = 6;	/* 6 ticks is "normal" sleeptime */

	if (PyMac_ConsoleIsDead) return;

	if ( !wait )
		maxsleep = 0;

	PyMac_DoYield(maxsleep, 0); /* XXXX or is it safe to call python here? */
}


/* Declarations of Socket Factories */

__BEGIN_DECLS
void GUSIwithInetSockets();
void GUSIwithLocalSockets();
void GUSIwithMTInetSockets();
void GUSIwithMTTcpSockets();
void GUSIwithMTUdpSockets();
void GUSIwithOTInetSockets();
void GUSIwithOTTcpSockets();
void GUSIwithOTUdpSockets();
void GUSIwithPPCSockets();
void GUSISetupFactories();
__END_DECLS

/* Configure Socket Factories */

void GUSISetupFactories()
{
#ifdef GUSISetupFactories_BeginHook
	GUSISetupFactories_BeginHook
#endif
	GUSIwithInetSockets();
#ifdef GUSISetupFactories_EndHook
	GUSISetupFactories_EndHook
#endif
}

/* Declarations of File Devices */

__BEGIN_DECLS
void GUSIwithDConSockets();
void GUSIwithNullSockets();
void GUSISetupDevices();
__END_DECLS

/* Configure File Devices */

void GUSISetupDevices()
{
#ifdef GUSISetupDevices_BeginHook
	GUSISetupDevices_BeginHook
#endif
#ifdef GUSISetupDevices_EndHook
	GUSISetupDevices_EndHook
#endif
}

#ifndef __cplusplus
#error GUSISetupConfig() needs to be written in C++
#endif

GUSIConfiguration::FileSuffix	sSuffices[] = {
	"", '????', '????'
};
extern "C" void GUSISetupConfig()
{
	Handle h;
	short oldrh, prefrh = -1;
	short resource_id = GUSIConfiguration::kNoResource;
	
	oldrh = CurResFile();
	
	/* Try override from the application resource fork */
	UseResFile(PyMac_AppRefNum);
	h = Get1Resource('GU\267I', GUSIOPTIONSOVERRIDE_ID);
	if ( h ) {
		resource_id = GUSIOPTIONSOVERRIDE_ID;
	} else {
		/* Next try normal resource from preference file */
		UseResFile(oldrh);
		prefrh = PyMac_OpenPrefFile();
		h = Get1Resource('GU\267I', GUSIOPTIONS_ID);
		if ( h ) {
			resource_id = GUSIOPTIONS_ID;
		} else {
			/* Finally try normal resource from application */
			if ( prefrh != -1 ) {
				CloseResFile(prefrh);
				prefrh = -1;
			}
			resource_id = GUSIOPTIONS_ID;
		}
	}

	/* Now we have the right resource file topmost and the id. Init GUSI. */
	GUSIConfiguration * config =
		GUSIConfiguration::CreateInstance(resource_id);

	/* Finally restore the old resource file */
   	if ( prefrh != -1) CloseResFile(prefrh);
	UseResFile(oldrh);

	config->ConfigureDefaultTypeCreator('TEXT', 'R*ch');
#if 0
	config->ConfigureSuffices(
		sizeof(sSuffices)/sizeof(GUSIConfiguration::FileSuffix)-1, sSuffices);
#endif
	config->ConfigureAutoInitGraf(false);
	config->ConfigureAutoSpin(false);
	config->ConfigureHandleAppleEvents(false);
	config->ConfigureSigInt(false);
	config->ConfigureSigPipe(true);
	
	GUSISetHook(GUSI_SpinHook, (GUSIHook)PyMac_GUSISpin);

}

/**************** END GUSI CONFIGURATION *************************/
