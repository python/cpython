/*********************************************************************
Project	:	GUSI				-	Grand unified socket interface
File		:	GUSISIOUX.cp	-	Interface to Metrowerks SIOUX library
Author	:	Matthias Neeracher
Language	:	MPW C/C++

$Log$
Revision 1.1  1998/08/18 14:52:38  jack
Putting Python-specific GUSI modifications under CVS.

*********************************************************************/

#include <GUSIFile_P.h>
#include <ioctl.h>
#include <console.h>

#include <Events.h>
#include <LowMem.h>

/************************ SIOUXSocket members ************************/

/* This declaration lies about the return type */
extern "C" void SIOUXHandleOneEvent(EventRecord *userevent);

GUSIEvtHandler	GUSISIOUXEvents[]	=	{
	SIOUXHandleOneEvent,		// nullEvent
	
	SIOUXHandleOneEvent,		// mouseDown
	SIOUXHandleOneEvent,		// mouseUp
	nil,							// keyDown
	nil,
	
	nil,							// autoKey
	SIOUXHandleOneEvent,		// updateEvt
	SIOUXHandleOneEvent,		// diskEvt
	SIOUXHandleOneEvent,		// activateEvt
	
	nil,
	nil,
	nil,
	nil,
	
	nil,
	nil,
	SIOUXHandleOneEvent,		// osEvt
	nil,
	
	nil,
	nil,
	nil,
	nil,
	
	nil,
	nil,
	nil,
};

/************************ Declaration of SIOUXSocket ************************/

class SIOUXSocket : public Socket	{		
	friend class SIOUXSocketDomain;	
	
					SIOUXSocket();
					
	virtual 		~SIOUXSocket();
protected:
	int			initialized;
	void			DoInitialize(void);
public:
	virtual int	read(void * buffer, int buflen);
	virtual int write(void * buffer, int buflen);
	virtual int select(Boolean * canRead, Boolean * canWrite, Boolean * exception);
	virtual int	ioctl(unsigned int request, void *argp);
	virtual int	isatty();
};	

class SIOUXSocketDomain : public FileSocketDomain {
	SIOUXSocket *	singleton;
public:
	SIOUXSocketDomain()	:	FileSocketDomain(AF_UNSPEC, true, false), singleton(nil)	{	}
	
	virtual Boolean Yours(const GUSIFileRef & ref, Request request);
	virtual Socket * open(const GUSIFileRef & ref, int oflag);
};

#if GENERATING68K
#pragma segment SIOUX
#endif

/************************ SIOUXSocket members ************************/

void SIOUXSocket::DoInitialize()
{
	if ( initialized ) return;
	initialized++;
	InstallConsole(0);
	GUSISetEvents(GUSISIOUXEvents);
}

SIOUXSocket::SIOUXSocket()
{
	initialized = 0;
	if ( !GUSIConfig.DelayConsole() )
		DoInitialize();
}

SIOUXSocket::~SIOUXSocket()
{
	RemoveConsole();
}

int SIOUXSocket::ioctl(unsigned int request, void *)
{
	if ( !initialized) DoInitialize();
	switch (request)	{
	case FIOINTERACTIVE:
		return 0;
	default:
		return GUSI_error(EOPNOTSUPP);
	}
}

int SIOUXSocket::read(void * buffer, int buflen)
{
	if ( !initialized) DoInitialize();
	fflush(stdout);
	
	return ReadCharsFromConsole((char *) buffer, buflen);
}

int SIOUXSocket::write(void * buffer, int buflen)
{
	if ( !initialized) DoInitialize();
	return WriteCharsToConsole((char *) buffer, buflen);
}

static Boolean input_pending()
{
	QHdrPtr eventQueue = LMGetEventQueue();
	EvQElPtr element = (EvQElPtr)eventQueue->qHead;
	
	// now, count the number of pending keyDown events.
	while (element != nil) {
		if (element->evtQWhat == keyDown || element->evtQWhat == autoKey)
			return true;
		element = (EvQElPtr)element->qLink;
	}
	
	return false;
}

int SIOUXSocket::select(Boolean * canRead, Boolean * canWrite, Boolean * exception)
{
	int		goodies 	= 	0;
		
	if ( !initialized) DoInitialize();
	fflush(stdout);
	
	if (canRead) 
		if (*canRead = input_pending())
			++goodies;
	
	if (canWrite) {
		*canWrite = true;
		++goodies;
	}
	
	if (exception)
		*exception = false;
	
	return goodies;
}

int SIOUXSocket::isatty()
{
	return 1;
}

/********************* SIOUXSocketDomain members **********************/

#ifdef MSLGUSI
#ifndef SFIOGUSI
	extern void GUSISetupMSLSIOUX();
#endif
#endif

extern "C" void GUSIwithSIOUXSockets()
{
	static SIOUXSocketDomain	SIOUXSockets;
	SIOUXSockets.DontStrip();
#ifdef MSLGUSI
#ifndef SFIOGUSI
	GUSISetupMSLSIOUX();
#endif
#endif
}

Boolean SIOUXSocketDomain::Yours(const GUSIFileRef & ref, FileSocketDomain::Request request)
{
	if (ref.spec || (request != willOpen && request != willStat))
		return false;
	
	switch (ref.name[4] | 0x20) {
	case 's':
		if ((ref.name[5] | 0x20) != 't' || (ref.name[6] | 0x20) != 'd')
			return false;
		switch (ref.name[7] | 0x20) {
		case 'i':
			if ((ref.name[8] | 0x20) != 'n' || ref.name[9])
				return false;
			return true;
		case 'o':
			if ((ref.name[8] | 0x20) != 'u' || (ref.name[9] | 0x20) != 't' || ref.name[10])
				return false;
			return true;
		case 'e':
			if ((ref.name[8] | 0x20) != 'r' || (ref.name[9] | 0x20) != 'r' || ref.name[10])
				return false;
			return true;
		default:
			return false;
		}
	case 'c':
		if (	(ref.name[5] | 0x20) != 'o' || (ref.name[6] | 0x20) != 'n'
			|| (ref.name[7] | 0x20) != 's' || (ref.name[8] | 0x20) != 'o'
			|| (ref.name[9] | 0x20) != 'l' || (ref.name[10] | 0x20) != 'e')
			return false;
		switch (ref.name[11]) {
		case 0:
			return true;
		default:
			return false;
		}
	default:
		return false;
	}
}

Socket * SIOUXSocketDomain::open(const GUSIFileRef &, int)
{
	if (!singleton)
		singleton = new SIOUXSocket();
	++*singleton;
	
	return singleton;
}
