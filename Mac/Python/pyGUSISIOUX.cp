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

static PyReadHandler sInConsole = 0L;
static PyWriteHandler sOutConsole = 0L;
static PyWriteHandler sErrConsole = 0L;

inline bool hasCustomConsole(void) { return sInConsole != 0L; }

class GUSISIOUXSocket : public GUSISocket {
public:
	~GUSISIOUXSocket();
	
	
ssize_t	read(const GUSIScatterer & buffer);
ssize_t write(const GUSIGatherer & buffer);
virtual int	ioctl(unsigned int request, va_list arg);
virtual int	fstat(struct stat * buf);
virtual int	isatty();
bool select(bool * canRead, bool * canWrite, bool *);

	static GUSISIOUXSocket *	Instance(int fd);
private:	
	GUSISIOUXSocket(int fd);
	static bool initialized;
	static void Initialize();
	int fFd;
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

GUSISIOUXSocket * GUSISIOUXSocket::Instance(int fd)
{
	return new GUSISIOUXSocket(fd);
}
// This declaration lies about the return type
extern "C" void SIOUXHandleOneEvent(EventRecord *userevent);

bool GUSISIOUXSocket::initialized = false;

GUSISIOUXSocket::GUSISIOUXSocket(int fd) : fFd(fd) 
{
	if (!hasCustomConsole()) {
		if (!PyMac_GetDelayConsoleFlag() && !initialized)
			Initialize();
		/* Tell the upper layers there's no unseen output */
		PyMac_OutputSeen();
	}
}

void
GUSISIOUXSocket::Initialize()
{
	if(!initialized && !hasCustomConsole())
	{
		initialized = true;
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
}
GUSISIOUXSocket::~GUSISIOUXSocket()
{
	if ( !initialized || hasCustomConsole() )
		return;
	
	initialized = false;
	RemoveConsole();
}
ssize_t GUSISIOUXSocket::read(const GUSIScatterer & buffer)
{
	if(hasCustomConsole())
	{
		if(fFd == 0)
			return buffer.SetLength(
				sInConsole((char *) buffer.Buffer(), (int)buffer.Length()));
		
		return 0;
	}
	
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
	if(hasCustomConsole())
	{
		if(fFd == 1)
			return sOutConsole((char *) buffer.Buffer(), (int)buffer.Length());
		else if(fFd == 2)
			return sErrConsole((char *) buffer.Buffer(), (int)buffer.Length());
		
		return 0;
	}
	
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
	return GUSISIOUXSocket::Instance(1);
}
void GUSISetupConsoleDescriptors()
{
	GUSIDescriptorTable * table = GUSIDescriptorTable::Instance();
	
	table->InstallSocket(GUSISIOUXSocket::Instance(0));
	table->InstallSocket(GUSISIOUXSocket::Instance(1));
	table->InstallSocket(GUSISIOUXSocket::Instance(2));
}

void PyMac_SetConsoleHandler(PyReadHandler stdinH, PyWriteHandler stdoutH, PyWriteHandler stderrH)
{
	if(stdinH && stdoutH && stderrH)
	{
		sInConsole = stdinH;
		sOutConsole = stdoutH;
		sErrConsole = stderrH;
	}
}

long PyMac_DummyReadHandler(char *buffer, long n)
{
	return 0;
}

long PyMac_DummyWriteHandler(char *buffer, long n)
{
	return 0;
}
