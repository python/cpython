/*********************************************************************
Project	:	GUSI				-	Grand Unified Socket Interface
File		:	GUSI.h			-	Socket calls
Author	:	Matthias Neeracher
Language	:	MPW C/C++

$Log$
Revision 1.1  2000/09/12 20:24:50  jack
Moved to Unsupported.

Revision 1.1  1998/08/18 14:52:33  jack
Putting Python-specific GUSI modifications under CVS.

Revision 1.2  1994/12/31  01:45:54  neeri
Fix alignment.

Revision 1.1  1994/02/25  02:56:49  neeri
Initial revision

Revision 0.15  1993/06/27  00:00:00  neeri
f?truncate

Revision 0.14  1993/06/20  00:00:00  neeri
Changed sa_constr_ppc

Revision 0.13  1993/02/14  00:00:00  neeri
AF_PAP

Revision 0.12  1992/12/08  00:00:00  neeri
getcwd()

Revision 0.11  1992/11/15  00:00:00  neeri
remove netdb.h definitions

Revision 0.10  1992/09/26  00:00:00  neeri
Separate dirent and stat

Revision 0.9  1992/09/12  00:00:00  neeri
Hostname stuff

Revision 0.8  1992/09/07  00:00:00  neeri
readlink()

Revision 0.7  1992/08/03  00:00:00  neeri
sa_constr_ppc

Revision 0.6  1992/07/21  00:00:00  neeri
sockaddr_atlk_sym

Revision 0.5  1992/06/26  00:00:00  neeri
choose()

Revision 0.4  1992/05/18  00:00:00  neeri
PPC stuff

Revision 0.3  1992/04/27  00:00:00  neeri
getsockopt()

Revision 0.2  1992/04/19  00:00:00  neeri
C++ compatibility

Revision 0.1  1992/04/17  00:00:00  neeri
bzero()

*********************************************************************/

#ifndef _GUSI_
#define _GUSI_

#include <sys/types.h>

/* Feel free to increase FD_SETSIZE as needed */
#define GUSI_MAX_FD	FD_SETSIZE

#include <sys/cdefs.h>
#include <compat.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <Types.h>
#include <Events.h>
#include <Files.h>
#include <AppleTalk.h>
#include <CTBUtilities.h>
#include <Packages.h>
#include <PPCToolBox.h>
#include <StandardFile.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>
#include <unistd.h>
#include <machine/endian.h>

typedef enum spin_msg {
	SP_MISC,				/* some weird thing, usually just return immediately if you get this */
	SP_SELECT,			/* in a select call */
	SP_NAME,				/* getting a host by name */
	SP_ADDR,				/* getting a host by address */
	SP_STREAM_READ,	/* Stream read call */
	SP_STREAM_WRITE,	/* Stream write call */
	SP_DGRAM_READ,		/* Datagram read call */
	SP_DGRAM_WRITE,	/* Datagram write call */
	SP_SLEEP,			/* sleeping, passes ticks left to sleep */
	SP_AUTO_SPIN		/* Autospin, passes argument to SpinCursor */
} spin_msg;

typedef int (*GUSISpinFn)(spin_msg msg, long param);
typedef void (*GUSIEvtHandler)(EventRecord * ev);
typedef GUSIEvtHandler	GUSIEvtTable[24];

/*
 * Address families, defined in sys/socket.h
 *
 
#define	AF_UNSPEC		 0		// unspecified
#define	AF_UNIX			 1		// local to host (pipes, portals)
#define	AF_INET			 2		// internetwork: UDP, TCP, etc.
#define	AF_CTB			 3		// Apple Comm Toolbox (not yet supported)
#define	AF_FILE			 4		// Normal File I/O (used internally)
#define	AF_PPC			 5		// PPC Toolbox
#define	AF_PAP			 6		// Printer Access Protocol (client only)
#define	AF_APPLETALK	16		// Apple Talk

*/

#define	ATALK_SYMADDR 272		/* Symbolic Address for AppleTalk 			*/

/*
 * Some Implementations of GUSI require you to call GUSISetup for the
 * socket families you'd like to have defined. It's a good idea to call
 * this for *all* implementations.
 *
 * GUSIDefaultSetup() will include all socket families.
 *
 * Never call any of the GUSIwithXXX routines directly.
 */

__BEGIN_DECLS
void GUSIwithAppleTalkSockets();
void GUSIwithInternetSockets();
void GUSIwithPAPSockets();
void GUSIwithPPCSockets();
void GUSIwithUnixSockets();
void GUSIwithSIOUXSockets();
void GUSIwithMPWSockets();

void GUSISetup(void (*socketfamily)());
void GUSIDefaultSetup();
void GUSILoadConfiguration(Handle);
__END_DECLS
/*
 * Types,  defined in sys/socket.h
 *

#define	SOCK_STREAM		 1		// stream socket 
#define	SOCK_DGRAM		 2		// datagram socket

*/

/*
 * Defined in sys/un.h
 *
 
struct sockaddr_un {
	short		sun_family;
	char 		sun_path[108];
};

*/

#ifndef PRAGMA_ALIGN_SUPPORTED
#error Apple had some fun with the conditional macros again
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

struct sockaddr_atlk {
	short			family;
	AddrBlock	addr;
};

struct sockaddr_atlk_sym {
	short			family;
	EntityName	name;
};

struct sockaddr_ppc {
	short					family;
	LocationNameRec	location;
	PPCPortRec			port;
};

/* Definitions for choose() */

#define 	CHOOSE_DEFAULT	1		/*	Use *name as default name						*/
#define	CHOOSE_NEW		2		/* Choose new entity name, not existing one	*/
#define	CHOOSE_DIR		4		/* Choose a directory name, not a file 		*/

typedef struct {
	short			numTypes;
	SFTypeList	types;
} sa_constr_file;

typedef struct {
	short			numTypes;
	NLType		types;
} sa_constr_atlk;

/* Definitions for sa_constr_ppc */

#define PPC_CON_NEWSTYLE		0x8000	/* Required */
#define PPC_CON_MATCH_NAME		0x0001	/* Match name */
#define PPC_CON_MATCH_TYPE 	0x0002 	/* Match port type */
#define PPC_CON_MATCH_NBP		0x0004	/* Match NBP type */

typedef struct	{
	short			flags;
	Str32			nbpType;
	PPCPortRec	match;
} sa_constr_ppc;

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

__BEGIN_DECLS
/* 
 * IO/Socket stuff, defined elsewhere (unistd.h, sys/socket.h
 *

int socket(int domain, int type, short protocol);
int bind(int s, void *name, int namelen);
int connect(int s, void *addr, int addrlen);
int listen(int s, int qlen);
int accept(int s, void *addr, int *addrlen);
int close(int s);
int read(int s, char *buffer, unsigned buflen);
int readv(int s, struct iovec *iov, int count);
int recv(int s, void *buffer, int buflen, int flags);
int recvfrom(int s, void *buffer, int buflen, int flags, void *from, int *fromlen);
int recvmsg(int s,struct msghdr *msg,int flags);
int write(int s, const char *buffer, unsigned buflen);
int writev(int s, struct iovec *iov, int count);
int send(int s, void *buffer, int buflen, int flags);
int sendto (int s, void *buffer, int buflen, int flags, void *to, int tolen);
int sendmsg(int s,struct msghdr *msg,int flags);
int select(int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int getdtablesize(void);
int getsockname(int s, void *name, int *namelen);
int getpeername(int s, struct sockaddr *name, int *namelen);
int shutdown(int s, int how);
int fcntl(int s, unsigned int cmd, int arg);
int dup(int s);
int dup2(int s, int s1);
int ioctl(int d, unsigned int request, long *argp);
int getsockopt(int s, int level, int optname, char *optval, int * optlen);
int setsockopt(int s, int level, int optname, char *optval, int optlen);
int isatty(int);
int remove(const char *filename);
int rename(const char *oldname, const char *newname);
int creat(const char*);
int faccess(char*, unsigned int, long*);
long lseek(int, long, int);
int open(const char*, int);
int unlink(char*);
int symlink(char* linkto, char* linkname);
int readlink(char* path, char* buf, int bufsiz);
int truncate(char *path, long length);
int ftruncate(int fd, long length);
int chdir(char * path);
int mkdir(char * path);
int rmdir(char * path);
char * getcwd(char * buf, int size);
*/

/* 
 * Defined in stdio.h
 */
 
#ifdef __MWERKS__
void fsetfileinfo (char *filename, unsigned long newcreator, unsigned long newtype);
#endif

void fgetfileinfo (char *filename, unsigned long * creator, unsigned long * type);

#ifdef __MWERKS__
FILE *fdopen(int fd, const char *mode);
int fwalk(int (*func)(FILE * stream));
#endif

int choose(
		int 		domain,
		int 		type,
		char * 	prompt,
		void * 	constraint,
		int 		flags,
		void * 	name,
		int * 	namelen);

/* 
 * Hostname routines, defined in netdb.h
 *
 
struct hostent * gethostbyname(char *name);
struct hostent * gethostbyaddr(struct in_addr *addrP, int, int);
int gethostname(char *machname, long buflen);
struct servent * getservbyname (char * name, char * proto);
struct protoent * getprotobyname(char * name);

*/

char * inet_ntoa(struct in_addr inaddr);
struct in_addr inet_addr(char *address);

/* 
 * GUSI supports a number of hooks. Every one of them has a different prototype, but needs
 * to be passed as a GUSIHook
 */

typedef enum {
	GUSI_SpinHook,	/* A GUSISpinFn, to be called when a call blocks */
	GUSI_ExecHook, /* Boolean (*hook)(const GUSIFileRef & ref), decides if file is executable */
	GUSI_FTypeHook,/* Boolean (*hook)(const FSSpec & spec) sets a default file type */
	GUSI_SpeedHook /* A long integer, to be added to the cursor spin variable */
} GUSIHookCode;

typedef void (*GUSIHook)(void);
void GUSISetHook(GUSIHookCode code, GUSIHook hook);
GUSIHook GUSIGetHook(GUSIHookCode code);

/* 
 * What to do when a routine blocks
 */

/* Defined for compatibility */
#define GUSISetSpin(routine)	GUSISetHook(GUSI_SpinHook, (GUSIHook)routine)
#define GUSIGetSpin()			(GUSISpinFn) GUSIGetHook(GUSI_SpinHook)

int GUSISetEvents(GUSIEvtTable table);
GUSIEvtHandler * GUSIGetEvents(void);

extern GUSIEvtHandler	GUSISIOWEvents[];

#define SIGPIPE	13
#define SIGALRM	14

/* 
 * BSD memory routines, defined in compat.h
 *

#define index(a, b)						strchr(a, b)
#define rindex(a, b)						strrchr(a, b)
#define bzero(from, len) 				memset(from, 0, len)
#define bcopy(from, to, len)			memcpy(to, from, len)
#define bcmp(s1, s2, len)				memcmp(s1, s2, len)
#define bfill(from, len, x)			memset(from, x, len)

 */

__END_DECLS

#endif /* !_GUSI_ */
