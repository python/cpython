/*
** Modified version of GUSISIOUX.cp especially for Python.
** Changes (by Jack):
** - Optionally delay the console window until something is written to it.
** - Tell the upper layers whether the last command was a read or a write.
** - Tell SIOUX not to use WaitNextEvent (both Python and SIOUX trying to be
**   nice to background apps means we're yielding almost 100% of the time).
** - Make sure signals are processed when returning from read/write.
*/
#define GUSI_SOURCE
#include "GUSIInternal.h"
#include "GUSISIOUX.h"
#include "GUSIDevice.h"
#include "GUSIDescriptor.h"
#include "GUSIBasics.h"
#include "GUSIDiag.h"
//#ifndef WITHOUT_JACK_MODS
//#include "GUSIConfig.h"
//#endif

#include <LowMem.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <console.h>

#include "Python.h"
#include "macglue.h"
extern Boolean SIOUXUseWaitNextEvent;

class GUSISIOUXSocket : public GUSISocket {
public:
	~GUSISIOUXSocket();
	
	
ssize_t	read(const GUSIScatterer & buffer);
ssize_t write(const GUSIGatherer & buffer);
virtual int	ioctl(unsigned int request, va_list arg);
virtual int	fstat(struct stat * buf);
virtual int	isatty();
bool select(bool * canRead, bool * canWrite, bool *);

	static GUSISIOUXSocket *	Instance();
private:
	static GUSISIOUXSocket *	sInstance;
	
	GUSISIOUXSocket();
	bool initialized;
	void Initialize();
	bool fDelayConsole;
};
class GUSISIOUXDevice : public GUSIDevice {
public:
	static GUSISIOUXDevice *	Instance();

	
virtual bool Want(GUSIFileToken & file);
virtual GUSISocket * open(GUSIFileToken &, int flags);
private:
	GUSISIOUXDevice() 								{}
	
	static GUSISIOUXDevice *	sInstance;
};
GUSISIOUXSocket * GUSISIOUXSocket::sInstance;

GUSISIOUXSocket * GUSISIOUXSocket::Instance()
{
	if (!sInstance)
		if (sInstance = new GUSISIOUXSocket)
			sInstance->AddReference();

	return sInstance;
}
// This declaration lies about the return type
extern "C" void SIOUXHandleOneEvent(EventRecord *userevent);

GUSISIOUXSocket::GUSISIOUXSocket() 
{
	if (PyMac_GetDelayConsoleFlag())
		fDelayConsole = true;
	else
		fDelayConsole = false;
	if ( fDelayConsole )
		initialized = 0;
	else
		Initialize();
	/* Tell the upper layers there's no unseen output */
	PyMac_OutputSeen();
}

void
GUSISIOUXSocket::Initialize()
{
	initialized = 1;
	InstallConsole(0);
	GUSISetHook(GUSI_EventHook+nullEvent, 	(GUSIHook)SIOUXHandleOneEvent);
	GUSISetHook(GUSI_EventHook+mouseDown, 	(GUSIHook)SIOUXHandleOneEvent);
	GUSISetHook(GUSI_EventHook+mouseUp, 	(GUSIHook)SIOUXHandleOneEvent);
	GUSISetHook(GUSI_EventHook+updateEvt, 	(GUSIHook)SIOUXHandleOneEvent);
	GUSISetHook(GUSI_EventHook+diskEvt, 	(GUSIHook)SIOUXHandleOneEvent);
	GUSISetHook(GUSI_EventHook+activateEvt, (GUSIHook)SIOUXHandleOneEvent);
	GUSISetHook(GUSI_EventHook+osEvt, 		(GUSIHook)SIOUXHandleOneEvent);
	PyMac_InitMenuBar();
}
GUSISIOUXSocket::~GUSISIOUXSocket()
{
	if ( !initialized ) return;
	RemoveConsole();
}
ssize_t GUSISIOUXSocket::read(const GUSIScatterer & buffer)
{
	if ( !initialized ) Initialize();
	GUSIStdioFlush();
	PyMac_OutputSeen();
	PyMac_RaiseConsoleWindow();
	return buffer.SetLength(
		ReadCharsFromConsole((char *) buffer.Buffer(), (int)buffer.Length()));
	GUSIContext::Yield(kGUSIPoll);
}
ssize_t GUSISIOUXSocket::write(const GUSIGatherer & buffer)
{
	ssize_t rv;
			
	if ( !initialized ) Initialize();
	PyMac_OutputNotSeen();
	SIOUXUseWaitNextEvent = false;
	rv = WriteCharsToConsole((char *) buffer.Buffer(), (int)buffer.Length());
	GUSIContext::Yield(kGUSIPoll);
	return rv;
}
int GUSISIOUXSocket::ioctl(unsigned int request, va_list)
{
	switch (request)	{
	case FIOINTERACTIVE:
		return 0;
	default:
		return GUSISetPosixError(EOPNOTSUPP);
	}
}
int	GUSISIOUXSocket::fstat(struct stat * buf)
{
	GUSISocket::fstat(buf);
	buf->st_mode =	S_IFCHR | 0666;
	
	return 0;
}
int GUSISIOUXSocket::isatty()
{ 
	return 1;
}
static bool input_pending()
{
#if !TARGET_API_MAC_CARBON
	// Jack thinks that completely removing this code is a bit
	// too much...
	QHdrPtr eventQueue = LMGetEventQueue();
	EvQElPtr element = (EvQElPtr)eventQueue->qHead;
	
	// now, count the number of pending keyDown events.
	while (element != nil) {
		if (element->evtQWhat == keyDown || element->evtQWhat == autoKey)
			return true;
		element = (EvQElPtr)element->qLink;
	}
#endif
	return false;
}

bool GUSISIOUXSocket::select(bool * canRead, bool * canWrite, bool *)
{
	if ( !initialized ) Initialize();
	bool cond = false;
	if (canRead)
		if (*canRead = input_pending())
			cond = true;
	if (canWrite)
		cond = *canWrite = true;
		
	return cond;
}
GUSISIOUXDevice * GUSISIOUXDevice::sInstance;
GUSISIOUXDevice * GUSISIOUXDevice::Instance()
{
	if (!sInstance)
		sInstance = new GUSISIOUXDevice();
	return sInstance;
}
bool GUSISIOUXDevice::Want(GUSIFileToken & file)
{
	switch (file.WhichRequest()) {
	case GUSIFileToken::kWillOpen:
		return file.IsDevice() && (file.StrStdStream(file.Path()) > -1);
	default:
		return false;
	}
}
GUSISocket * GUSISIOUXDevice::open(GUSIFileToken &, int)
{
	return GUSISIOUXSocket::Instance();
}
void GUSISetupConsoleDescriptors()
{
	GUSIDescriptorTable * table = GUSIDescriptorTable::Instance();
	GUSISIOUXSocket *     SIOUX = GUSISIOUXSocket::Instance();
	
	table->InstallSocket(SIOUX);
	table->InstallSocket(SIOUX);
	table->InstallSocket(SIOUX);
}
