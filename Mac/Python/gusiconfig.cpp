/*
 * Generated with the GUSIConfig application and then hand-modified by jack.
 */

#define GUSI_SOURCE
#include <GUSIConfig.h>
#include <sys/cdefs.h>

#include "Python.h"
#include "macglue.h"

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
	GUSIConfiguration * config =
		GUSIConfiguration::CreateInstance(GUSIConfiguration::kNoResource);

	config->ConfigureDefaultTypeCreator('TEXT', 'TEXT');
	config->ConfigureSuffices(
		sizeof(sSuffices)/sizeof(GUSIConfiguration::FileSuffix)-1, sSuffices);
	config->ConfigureAutoInitGraf(false);
	config->ConfigureAutoSpin(false);
	config->ConfigureHandleAppleEvents(false);
	config->ConfigureSigInt(false);
	config->ConfigureSigPipe(true);
	
	GUSISetHook(GUSI_SpinHook, (GUSIHook)PyMac_GUSISpin);

}

/**************** END GUSI CONFIGURATION *************************/
