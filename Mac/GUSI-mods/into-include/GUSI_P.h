/*********************************************************************
Project	:	GUSI				-	Grand Unified Socket Interface
File		:	GUSI_P.h			-	Private stuff
Author	:	Matthias Neeracher
Language	:	MPW C/C++

$Log$
Revision 1.1  1998/08/18 14:52:33  jack
Putting Python-specific GUSI modifications under CVS.

Revision 1.3  1994/12/31  01:30:26  neeri
Reorganize filename dispatching.

Revision 1.2  1994/08/10  00:41:05  neeri
Sanitized for universal headers.

Revision 1.1  1994/02/25  02:57:01  neeri
Initial revision

Revision 0.22  1993/07/17  00:00:00  neeri
GUSIRingBuffer::proc -> defproc

Revision 0.21  1993/07/17  00:00:00  neeri
GUSIO_MAX_DOMAIN -> AF_MAX

Revision 0.20  1993/06/27  00:00:00  neeri
Socket::{pre,post}_select

Revision 0.19  1993/06/27  00:00:00  neeri
Socket::ftruncate

Revision 0.18  1993/02/09  00:00:00  neeri
Socket::lurking, Socket::lurkdescr

Revision 0.17  1993/01/31  00:00:00  neeri
GUSIConfiguration::daemon

Revision 0.16  1993/01/17  00:00:00  neeri
Destructors for Socketdomain

Revision 0.15  1993/01/17  00:00:00  neeri
SAFESPIN

Revision 0.14  1993/01/03  00:00:00  neeri
GUSIConfig

Revision 0.13  1992/09/24  00:00:00  neeri
Include GUSIRsrc_P.h

Revision 0.12  1992/09/13  00:00:00  neeri
SPINVOID didn't return

Revision 0.11  1992/08/30  00:00:00  neeri
AppleTalkIdentity()

Revision 0.10  1992/08/03  00:00:00  neeri
RingBuffer

Revision 0.9  1992/07/30  00:00:00  neeri
Initializer Features

Revision 0.8  1992/07/26  00:00:00  neeri
UnixSockets.choose()

Revision 0.7  1992/07/13  00:00:00  neeri
Make AppleTalkSockets global

Revision 0.6  1992/06/27  00:00:00  neeri
choose(), hasNewSF

Revision 0.5  1992/06/07  00:00:00  neeri
Feature

Revision 0.4  1992/05/21  00:00:00  neeri
Implemented select()

Revision 0.3  1992/04/19  00:00:00  neeri
C++ rewrite

Revision 0.2  1992/04/18  00:00:00  neeri
changed read/write/send/recv dispatchers

Revision 0.1  1992/04/18  00:00:00  neeri
ppc Domain

*********************************************************************/

#ifndef __GUSI_P__
#define __GUSI_P__

#define __useAppleExts__

#include <GUSI.h>
#include <GUSIRsrc_P.h>
#include <TFileSpec.h>


#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/socket.h>

extern "C" {

#include <stdio.h>
#include <string.h>

int 		GUSI_error(int err);
void *	GUSI_error_nil(int err);
}

#include <Memory.h>
#include <Gestalt.h>
#include <Traps.h>
#include <AppleEvents.h>
#include <Processes.h>
#include <MixedMode.h>

#if MSLGUSI
using namespace std;
#endif

#if GENERATING68K
#pragma segment GUSI
#endif

#define GUSI_MAX_DOMAIN			AF_MAX
#define DEFAULT_BUFFER_SIZE	4096

/*
 *	In use and shutdown status.
 */
#define	SOCK_STATUS_USED		0x1		/* Used socket table entry */
#define	SOCK_STATUS_NOREAD	0x2		/* No more reading allowed from socket */
#define	SOCK_STATUS_NOWRITE	0x4		/* No more writing allowed to socket */

/*
 *	Socket connection states.
 */
#define	SOCK_STATE_NO_STREAM		0	/* Socket doesn't have a MacTCP stream yet */
#define	SOCK_STATE_UNCONNECTED	1	/* Socket is unconnected. */
#define	SOCK_STATE_LISTENING		2	/* Socket is listening for connection. */
#define	SOCK_STATE_LIS_CON		3	/* Socket is in transition from listen to connected. */
#define	SOCK_STATE_CONNECTING	4	/* Socket is initiating a connection. */
#define	SOCK_STATE_CONNECTED		5	/* Socket is connected. */
#define	SOCK_STATE_CLOSING      6	/* Socket is closing */
#define	SOCK_STATE_LIS_CLOSE    7	/* Socket closed while listening */

#define		min(a,b)				( (a) < (b) ? (a) : (b))
#define		max(a,b)				( (a) > (b) ? (a) : (b))

extern GUSISpinFn GUSISpin;
extern "C" int GUSIDefaultSpin(spin_msg, long);
extern int GUSICheckAlarm();

#define GUSI_INTERRUPT(mesg,param)	(GUSICheckAlarm() || (GUSISpin && (*GUSISpin)(mesg,param)))

/* SPIN returns a -1 on user cancel for fn returning integers */
#define		SPIN(cond,mesg,param)							\
					do {												\
						if (GUSI_INTERRUPT(mesg,param))		\
							return GUSI_error(EINTR);			\
					} while(cond)

/* SPINP returns a NULL on user cancel, for fn returning pointers */				
#define		SPINP(cond,mesg,param)							\
					do {												\
						if (GUSI_INTERRUPT(mesg,param)) {	\
							GUSI_error(EINTR);					\
							return NULL;							\
						}												\
					} while(cond)

/* SPINVOID just returns on user cancel, for fn returning void */				
#define		SPINVOID(cond,mesg,param)						\
					do {												\
						if (GUSI_INTERRUPT(mesg,param)) {	\
								GUSI_error(EINTR);				\
								return;								\
							}											\
					} while(cond)
					
/* SAFESPIN doesn't return, you have to check errno */				
#define		SAFESPIN(cond,mesg,param)						\
					do {												\
						if (GUSI_INTERRUPT(mesg,param)) {	\
							GUSI_error(EINTR);					\
							break;									\
						} else										\
							errno = 0;								\
					} while(cond)

//
// Library functions are never allowed to clear errno, so we have to save
//
class ErrnoSaver {
public:
	ErrnoSaver()  { fSavedErrno = ::errno; ::errno = 0; 	}
	~ErrnoSaver() { if (!::errno) ::errno = fSavedErrno;  }
private:
	int fSavedErrno;
};

#define SAVE_AND_CLEAR_ERRNO	ErrnoSaver saveErrno
			
class SocketTable;

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

class Socket {
	friend class SocketTable;
	
	short			refCount;
protected:
					Socket();
public:
	virtual int	bind(void * name, int namelen);
	virtual int connect(void * address, int addrlen);
	virtual int listen(int qlen);
	virtual Socket * accept(void * address, int * addrlen);
	virtual int	read(void * buffer, int buflen);
	virtual int write(void * buffer, int buflen);
	virtual int recvfrom(void * buffer, int buflen, int flags, void * from, int * fromlen);
	virtual int sendto(void * buffer, int buflen, int flags, void * to, int tolen);
	virtual int getsockname(void * name, int * namelen);
	virtual int getpeername(void * name, int * namelen);
	virtual int getsockopt(int level, int optname, void *optval, int * optlen);
	virtual int setsockopt(int level, int optname, void *optval, int optlen);
	virtual int	fcntl(unsigned int cmd, int arg);
	virtual int	ioctl(unsigned int request, void *argp);
	virtual int	fstat(struct stat * buf);
	virtual long lseek(long offset, int whence);
	virtual int ftruncate(long offset);
	virtual int	isatty();
	virtual int shutdown(int how);
	virtual void pre_select(Boolean wantRead, Boolean wantWrite, Boolean wantExcept);
	virtual int select(Boolean * canRead, Boolean * canWrite, Boolean * exception);
	virtual void post_select(Boolean wantRead, Boolean wantWrite, Boolean wantExcept);
	virtual 		~Socket();
	
	void operator++()	{	++refCount;							}
	void operator--()	{	if (!--refCount) delete this;	}
};


#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

class SocketDomain {
	static SocketDomain *		domains[GUSI_MAX_DOMAIN];
	static ProcessSerialNumber	process;
protected:
	SocketDomain(int domain);
	virtual ~SocketDomain();
public:
	inline static SocketDomain *	Domain(int domain);
	static void Ready();
	
	// Optionally override the following
	
	virtual Socket * socket(int type, short protocol);
	
	// Optionally override the following
	
	virtual int socketpair(int type, short protocol, Socket * sockets[]);
	
	// Optionally define the following
	
	virtual int choose(
						int 		type, 
						char * 	prompt, 
						void * 	constraint,		
						int 		flags,
 						void * 	name, 
						int * 	namelen);
	
	// Never override the following
	
	void DontStrip();
};

class SocketTable {
	Socket *	sockets[GUSI_MAX_FD];
	Boolean	needsConsole;
public:
	SocketTable();
	~SocketTable();
	
	void		InitConsole();
	int		Install(Socket * sock, int start = 0);
	int		Remove(int fd);
	Socket * operator[](int fd);
};

struct GUSISuffix {
	char 		suffix[4];
	OSType	suffType;
	OSType	suffCreator;
};

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

//
// I learned the hard way not to rely on bit field alignments
//

struct GUSIConfigRsrc {
	OSType			defaultType;
	OSType			defaultCreator;
	
	char				autoSpin;
	unsigned char	flags;
	
	OSType			version;
	short				numSuffices;
	GUSISuffix 		suffices[1];
};

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

struct GUSIConfiguration {
	OSType			defaultType;
	OSType			defaultCreator;
	
	char				autoSpin;
	
	Boolean	 		noChdir;		// Set current directory without chdir()
	Boolean 			accurStat;	// Return # of subdirectories + 2 in st_nlink
	Boolean	 		hasConsole;	// Do we have our own console ?
	Boolean			noAutoInitGraf;	// Never automatically do InitGraf
	Boolean			sharedOpen;	// Open files with shared permissions
	Boolean			sigPipe;		// raise SIGPIPE on write to closed socket
	Boolean			noAppleEvents; // Don't solicit AppleEvents for MPW tools
	Boolean			delayConsole;	// Do not open console until needed
	
	OSType			version;
	short				numSuffices;
	GUSISuffix *	suffices;
	
	GUSIConfiguration();
	void GUSILoadConfiguration(Handle config);
	
	void SetDefaultFType(const TFileSpec & name) const;
	void DoAutoSpin() const;
	void AutoInitGraf()	const {	if (!noAutoInitGraf) DoAutoInitGraf();	}
	void DoAutoInitGraf() const;
	Boolean DelayConsole() const;
private:
	static Boolean firstTime;
	static short	we;
};

extern GUSIConfiguration	GUSIConfig;
extern SocketTable					Sockets;

typedef pascal OSErr (*OSErrInitializer)();
typedef pascal void  (*voidInitializer)();

class Feature {
	Boolean	good;
public:
	Feature(unsigned short trapNum, TrapType tTyp);
	Feature(OSType type, long value);
	Feature(OSType type, long mask, long value);
	Feature(const Feature & precondition, OSErrInitializer init);
	Feature(OSErrInitializer init);
	Feature(const Feature & precondition, voidInitializer init);
	Feature(voidInitializer init);
	Feature(const Feature & cond1, const Feature & cond2);

	operator void*() const {	return (void *) good;	}
};

extern Feature hasMakeFSSpec;
extern Feature hasAlias;
extern Feature hasNewSF;
extern Feature hasProcessMgr;
extern Feature hasCRM;
extern Feature hasCTB;
extern Feature hasStdNBP;
extern Feature hasCM;
extern Feature hasFT;
extern Feature hasTM;
extern Feature	hasPPC;
extern Feature hasRevisedTimeMgr;

class ScattGath	{
	Handle			scratch;
protected:
	void *			buf;
	int						len;
	int						count;
	const struct iovec *	io;

	ScattGath(const struct iovec *iov, int cnt);
	virtual ~ScattGath();
public:
	void *			buffer()			{	return buf;			}
	int				buflen()			{	return len;			}
	int				length(int l)	{	return len = l;	}
	operator void *()					{	return buf;			}
};

class Scatterer : public ScattGath {
public:
	Scatterer(const struct iovec *iov, int count);
	virtual ~Scatterer();
};

class Gatherer : public ScattGath {
public:
	Gatherer(const struct iovec *iov, int count);
	virtual ~Gatherer();
};

typedef pascal void (*Deferred)(void *);

class RingBuffer {
	// Valid bytes are between consume and produce
	// Free bytes are between produce and consume
	// bytes between endbuf-spare and endbuf are neither
	Ptr		buffer;
	Ptr		endbuf;
	Ptr 		consume;
	Ptr		produce;
	u_short	free;
	u_short	valid;
	u_short	spare;
	Boolean	lock;
	Deferred	defproc;
	void *	arg;
	
public:
				RingBuffer(u_short bufsiz);
				~RingBuffer();
	
	Ptr		Producer(long & len);			//	Find continuous memory for producer
	Ptr		Consumer(long & len);			//	Find continuous memory for consumer
	void		Validate(long len);				// Validate this, unallocate rest
	void 		Invalidate(long len);
	void		Produce(Ptr from, long & len);//	Allocate, copy & validate
	void		Consume(Ptr to, long & len);	// Copy & invalidate
	
	long		Free()								{ return free;									}		
	long		Valid()								{ return valid;								}
	
	void 		Defer()								{ lock = true;									}
	void 		Undefer()							{ lock = false; if (defproc) defproc(arg);}
	Boolean	Locked()								{ return lock;									}
	void		Later(Deferred def, void * ar){ defproc = def; arg = ar;					}
	
	operator void *()								{ return buffer;								}
};

Boolean GUSIInterrupt();

Boolean CopyIconFamily(short srcResFile, short srcID, short dstResFile, short dstID);

pascal OSErr PPCInit_P();

OSErr AppleTalkIdentity(short & net, short & node);

void CopyC2PStr(const char * cstr, StringPtr pstr);

#endif
