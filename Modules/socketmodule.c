/* Socket module */

/*

This module provides an interface to Berkeley socket IPC.

Limitations:

- Only AF_INET, AF_INET6 and AF_UNIX address families are supported in a
  portable manner, though AF_PACKET is supported under Linux.
- No read/write operations (use sendall/recv or makefile instead).
- Additional restrictions apply on some non-Unix platforms (compensated
  for by socket.py).

Module interface:

- socket.error: exception raised for socket specific errors
- socket.gaierror: exception raised for getaddrinfo/getnameinfo errors,
	a subclass of socket.error
- socket.herror: exception raised for gethostby* errors,
	a subclass of socket.error
- socket.fromfd(fd, family, type[, proto]) --> new socket object (created
        from an existing file descriptor)
- socket.gethostbyname(hostname) --> host IP address (string: 'dd.dd.dd.dd')
- socket.gethostbyaddr(IP address) --> (hostname, [alias, ...], [IP addr, ...])
- socket.gethostname() --> host name (string: 'spam' or 'spam.domain.com')
- socket.getprotobyname(protocolname) --> protocol number
- socket.getservbyname(servicename[, protocolname]) --> port number
- socket.getservbyport(portnumber[, protocolname]) --> service name
- socket.socket([family[, type [, proto]]]) --> new socket object
- socket.socketpair([family[, type [, proto]]]) --> (socket, socket)
- socket.ntohs(16 bit value) --> new int object
- socket.ntohl(32 bit value) --> new int object
- socket.htons(16 bit value) --> new int object
- socket.htonl(32 bit value) --> new int object
- socket.getaddrinfo(host, port [, family, socktype, proto, flags])
	--> List of (family, socktype, proto, canonname, sockaddr)
- socket.getnameinfo(sockaddr, flags) --> (host, port)
- socket.AF_INET, socket.SOCK_STREAM, etc.: constants from <socket.h>
- socket.has_ipv6: boolean value indicating if IPv6 is supported
- socket.inet_aton(IP address) -> 32-bit packed IP representation
- socket.inet_ntoa(packed IP) -> IP address string
- socket.getdefaulttimeout() -> None | float
- socket.setdefaulttimeout(None | float)
- an Internet socket address is a pair (hostname, port)
  where hostname can be anything recognized by gethostbyname()
  (including the dd.dd.dd.dd notation) and port is in host byte order
- where a hostname is returned, the dd.dd.dd.dd notation is used
- a UNIX domain socket address is a string specifying the pathname
- an AF_PACKET socket address is a tuple containing a string
  specifying the ethernet interface and an integer specifying
  the Ethernet protocol number to be received. For example:
  ("eth0",0x1234).  Optional 3rd,4th,5th elements in the tuple
  specify packet-type and ha-type/addr.

Local naming conventions:

- names starting with sock_ are socket object methods
- names starting with socket_ are module-level functions
- names starting with PySocket are exported through socketmodule.h

*/

#include "Python.h"

#undef MAX
#define MAX(x, y) ((x) < (y) ? (y) : (x))

/* Socket object documentation */
PyDoc_STRVAR(sock_doc,
"socket([family[, type[, proto]]]) -> socket object\n\
\n\
Open a socket of the given type.  The family argument specifies the\n\
address family; it defaults to AF_INET.  The type argument specifies\n\
whether this is a stream (SOCK_STREAM, this is the default)\n\
or datagram (SOCK_DGRAM) socket.  The protocol argument defaults to 0,\n\
specifying the default protocol.  Keyword arguments are accepted.\n\
\n\
A socket object represents one endpoint of a network connection.\n\
\n\
Methods of socket objects (keyword arguments not allowed):\n\
\n\
accept() -- accept a connection, returning new socket and client address\n\
bind(addr) -- bind the socket to a local address\n\
close() -- close the socket\n\
connect(addr) -- connect the socket to a remote address\n\
connect_ex(addr) -- connect, return an error code instead of an exception\n\
dup() -- return a new socket object identical to the current one [*]\n\
fileno() -- return underlying file descriptor\n\
getpeername() -- return remote address [*]\n\
getsockname() -- return local address\n\
getsockopt(level, optname[, buflen]) -- get socket options\n\
gettimeout() -- return timeout or None\n\
listen(n) -- start listening for incoming connections\n\
makefile([mode, [bufsize]]) -- return a file object for the socket [*]\n\
recv(buflen[, flags]) -- receive data\n\
recvfrom(buflen[, flags]) -- receive data and sender's address\n\
sendall(data[, flags]) -- send all data\n\
send(data[, flags]) -- send data, may not send all of it\n\
sendto(data[, flags], addr) -- send data to a given address\n\
setblocking(0 | 1) -- set or clear the blocking I/O flag\n\
setsockopt(level, optname, value) -- set socket options\n\
settimeout(None | float) -- set or clear the timeout\n\
shutdown(how) -- shut down traffic in one or both directions\n\
\n\
 [*] not available on all platforms!");

/* XXX This is a terrible mess of platform-dependent preprocessor hacks.
   I hope some day someone can clean this up please... */

/* Hacks for gethostbyname_r().  On some non-Linux platforms, the configure
   script doesn't get this right, so we hardcode some platform checks below.
   On the other hand, not all Linux versions agree, so there the settings
   computed by the configure script are needed! */

#ifndef linux
# undef HAVE_GETHOSTBYNAME_R_3_ARG
# undef HAVE_GETHOSTBYNAME_R_5_ARG
# undef HAVE_GETHOSTBYNAME_R_6_ARG
#endif

#ifndef WITH_THREAD
# undef HAVE_GETHOSTBYNAME_R
#endif

#ifdef HAVE_GETHOSTBYNAME_R
# if defined(_AIX) || defined(__osf__)
#  define HAVE_GETHOSTBYNAME_R_3_ARG
# elif defined(__sun) || defined(__sgi)
#  define HAVE_GETHOSTBYNAME_R_5_ARG
# elif defined(linux)
/* Rely on the configure script */
# else
#  undef HAVE_GETHOSTBYNAME_R
# endif
#endif

#if !defined(HAVE_GETHOSTBYNAME_R) && defined(WITH_THREAD) && \
    !defined(MS_WINDOWS)
# define USE_GETHOSTBYNAME_LOCK
#endif

/* On systems on which getaddrinfo() is believed to not be thread-safe,
   (this includes the getaddrinfo emulation) protect access with a lock. */
#if defined(WITH_THREAD) && (defined(__APPLE__) || defined(__FreeBSD__) || \
    defined(__OpenBSD__) || defined(__NetBSD__) || !defined(HAVE_GETADDRINFO))
#define USE_GETADDRINFO_LOCK
#endif

#ifdef USE_GETADDRINFO_LOCK
#define ACQUIRE_GETADDRINFO_LOCK PyThread_acquire_lock(netdb_lock, 1);
#define RELEASE_GETADDRINFO_LOCK PyThread_release_lock(netdb_lock);
#else
#define ACQUIRE_GETADDRINFO_LOCK
#define RELEASE_GETADDRINFO_LOCK
#endif

#if defined(USE_GETHOSTBYNAME_LOCK) || defined(USE_GETADDRINFO_LOCK)
# include "pythread.h"
#endif

#if defined(PYCC_VACPP)
# include <types.h>
# include <io.h>
# include <sys/ioctl.h>
# include <utils.h>
# include <ctype.h>
#endif

#if defined(__VMS)
#if ! defined(_SOCKADDR_LEN)
#   ifdef getaddrinfo
#      undef getaddrinfo
#   endif
#  include "TCPIP_IOCTL_ROUTINE"
#else
#  include <ioctl.h>
#endif
#endif

#if defined(PYOS_OS2)
# define  INCL_DOS
# define  INCL_DOSERRORS
# define  INCL_NOPMAPI
# include <os2.h>
#endif

#if defined(__sgi) && _COMPILER_VERSION>700 && !_SGIAPI
/* make sure that the reentrant (gethostbyaddr_r etc)
   functions are declared correctly if compiling with
   MIPSPro 7.x in ANSI C mode (default) */

/* XXX Using _SGIAPI is the wrong thing, 
   but I don't know what the right thing is. */
#undef _SGIAPI /* to avoid warning */
#define _SGIAPI 1

#undef _XOPEN_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#ifdef _SS_ALIGNSIZE
#define HAVE_GETADDRINFO 1
#define HAVE_GETNAMEINFO 1
#endif

#define HAVE_INET_PTON
#include <netdb.h>
#endif

/* Irix 6.5 fails to define this variable at all. This is needed 
   for both GCC and SGI's compiler. I'd say that the SGI headers 
   are just busted. */
#if defined(__sgi) && !defined(INET_ADDRSTRLEN)
#define INET_ADDRSTRLEN 16
#endif

/* Generic includes */
#include <sys/types.h>

/* Generic socket object definitions and includes */
#define PySocket_BUILDING_SOCKET
#include "socketmodule.h"

/* Addressing includes */

#ifndef MS_WINDOWS

/* Non-MS WINDOWS includes */
# include <netdb.h>

/* Headers needed for inet_ntoa() and inet_addr() */
# ifdef __BEOS__
#  include <net/netdb.h>
# elif defined(PYOS_OS2) && defined(PYCC_VACPP)
#  include <netdb.h>
typedef size_t socklen_t;
# else
#   include <arpa/inet.h>
# endif

# ifndef RISCOS
#  include <fcntl.h>
# else
#  include <sys/ioctl.h>
#  include <socklib.h>
#  define NO_DUP
int h_errno; /* not used */
#  define INET_ADDRSTRLEN 16
# endif

#else

/* MS_WINDOWS includes */
# include <fcntl.h>

#endif

#include <stddef.h>

#ifndef offsetof
# define offsetof(type, member)	((size_t)(&((type *)0)->member))
#endif

#ifndef O_NONBLOCK
# define O_NONBLOCK O_NDELAY
#endif

/* include Python's addrinfo.h unless it causes trouble */
#if defined(__sgi) && _COMPILER_VERSION>700 && defined(_SS_ALIGNSIZE)
  /* Do not include addinfo.h on some newer IRIX versions.
   * _SS_ALIGNSIZE is defined in sys/socket.h by 6.5.21,
   * for example, but not by 6.5.10.
   */
#elif defined(_MSC_VER) && _MSC_VER>1200
  /* Do not include addrinfo.h for MSVC7 or greater. 'addrinfo' and
   * EAI_* constants are defined in (the already included) ws2tcpip.h.
   */
#else
#  include "addrinfo.h"
#endif

#ifndef HAVE_INET_PTON
int inet_pton(int af, const char *src, void *dst);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
#endif

#ifdef __APPLE__
/* On OS X, getaddrinfo returns no error indication of lookup
   failure, so we must use the emulation instead of the libinfo
   implementation. Unfortunately, performing an autoconf test
   for this bug would require DNS access for the machine performing
   the configuration, which is not acceptable. Therefore, we
   determine the bug just by checking for __APPLE__. If this bug
   gets ever fixed, perhaps checking for sys/version.h would be
   appropriate, which is 10/0 on the system with the bug. */
#ifndef HAVE_GETNAMEINFO
/* This bug seems to be fixed in Jaguar. Ths easiest way I could
   Find to check for Jaguar is that it has getnameinfo(), which
   older releases don't have */
#undef HAVE_GETADDRINFO
#endif
#endif

/* I know this is a bad practice, but it is the easiest... */
#if !defined(HAVE_GETADDRINFO)
/* avoid clashes with the C library definition of the symbol. */
#define getaddrinfo fake_getaddrinfo
#define gai_strerror fake_gai_strerror
#define freeaddrinfo fake_freeaddrinfo
#include "getaddrinfo.c"
#endif
#if !defined(HAVE_GETNAMEINFO)
#define getnameinfo fake_getnameinfo
#include "getnameinfo.c"
#endif

#if defined(MS_WINDOWS) || defined(__BEOS__)
/* BeOS suffers from the same socket dichotomy as Win32... - [cjh] */
/* seem to be a few differences in the API */
#define SOCKETCLOSE closesocket
#define NO_DUP /* Actually it exists on NT 3.5, but what the heck... */
#endif

#ifdef MS_WIN32
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#define snprintf _snprintf
#endif

#if defined(PYOS_OS2) && !defined(PYCC_GCC)
#define SOCKETCLOSE soclose
#define NO_DUP /* Sockets are Not Actual File Handles under OS/2 */
#endif

#ifndef SOCKETCLOSE
#define SOCKETCLOSE close
#endif

#ifdef __VMS
/* TCP/IP Services for VMS uses a maximum send/revc buffer length of 65535 */
#define SEGMENT_SIZE 65535
#endif

#if defined(HAVE_BLUETOOTH_H) || defined(HAVE_BLUETOOTH_BLUETOOTH_H)
#define USE_BLUETOOTH 1
#if defined(__FreeBSD__)
#define BTPROTO_L2CAP BLUETOOTH_PROTO_L2CAP
#define BTPROTO_RFCOMM BLUETOOTH_PROTO_RFCOMM
#define sockaddr_l2 sockaddr_l2cap
#define sockaddr_rc sockaddr_rfcomm
#define _BT_SOCKADDR_MEMB(s, proto) &((s)->sock_addr)
#define _BT_L2_MEMB(sa, memb) ((sa)->l2cap_##memb)
#define _BT_RC_MEMB(sa, memb) ((sa)->rfcomm_##memb)
#else
#define _BT_SOCKADDR_MEMB(s, proto) (&((s)->sock_addr).bt_##proto)
#define _BT_L2_MEMB(sa, memb) ((sa)->l2_##memb)
#define _BT_RC_MEMB(sa, memb) ((sa)->rc_##memb)
#define _BT_SCO_MEMB(sa, memb) ((sa)->sco_##memb)
#endif
#endif

/*
 * Constants for getnameinfo()
 */
#if !defined(NI_MAXHOST)
#define NI_MAXHOST 1025
#endif
#if !defined(NI_MAXSERV)
#define NI_MAXSERV 32
#endif

/* XXX There's a problem here: *static* functions are not supposed to have
   a Py prefix (or use CapitalizedWords).  Later... */

/* Global variable holding the exception type for errors detected
   by this module (but not argument type or memory errors, etc.). */
static PyObject *socket_error;
static PyObject *socket_herror;
static PyObject *socket_gaierror;
static PyObject *socket_timeout;

#ifdef RISCOS
/* Global variable which is !=0 if Python is running in a RISC OS taskwindow */
static int taskwindow;
#endif

/* A forward reference to the socket type object.
   The sock_type variable contains pointers to various functions,
   some of which call new_sockobject(), which uses sock_type, so
   there has to be a circular reference. */
static PyTypeObject sock_type;

/* Convenience function to raise an error according to errno
   and return a NULL pointer from a function. */

static PyObject *
set_error(void)
{
#ifdef MS_WINDOWS
	int err_no = WSAGetLastError();
	static struct {
		int no;
		const char *msg;
	} *msgp, msgs[] = {
		{WSAEINTR, "Interrupted system call"},
		{WSAEBADF, "Bad file descriptor"},
		{WSAEACCES, "Permission denied"},
		{WSAEFAULT, "Bad address"},
		{WSAEINVAL, "Invalid argument"},
		{WSAEMFILE, "Too many open files"},
		{WSAEWOULDBLOCK,
		  "The socket operation could not complete "
		  "without blocking"},
		{WSAEINPROGRESS, "Operation now in progress"},
		{WSAEALREADY, "Operation already in progress"},
		{WSAENOTSOCK, "Socket operation on non-socket"},
		{WSAEDESTADDRREQ, "Destination address required"},
		{WSAEMSGSIZE, "Message too long"},
		{WSAEPROTOTYPE, "Protocol wrong type for socket"},
		{WSAENOPROTOOPT, "Protocol not available"},
		{WSAEPROTONOSUPPORT, "Protocol not supported"},
		{WSAESOCKTNOSUPPORT, "Socket type not supported"},
		{WSAEOPNOTSUPP, "Operation not supported"},
		{WSAEPFNOSUPPORT, "Protocol family not supported"},
		{WSAEAFNOSUPPORT, "Address family not supported"},
		{WSAEADDRINUSE, "Address already in use"},
		{WSAEADDRNOTAVAIL, "Can't assign requested address"},
		{WSAENETDOWN, "Network is down"},
		{WSAENETUNREACH, "Network is unreachable"},
		{WSAENETRESET, "Network dropped connection on reset"},
		{WSAECONNABORTED, "Software caused connection abort"},
		{WSAECONNRESET, "Connection reset by peer"},
		{WSAENOBUFS, "No buffer space available"},
		{WSAEISCONN, "Socket is already connected"},
		{WSAENOTCONN, "Socket is not connected"},
		{WSAESHUTDOWN, "Can't send after socket shutdown"},
		{WSAETOOMANYREFS, "Too many references: can't splice"},
		{WSAETIMEDOUT, "Operation timed out"},
		{WSAECONNREFUSED, "Connection refused"},
		{WSAELOOP, "Too many levels of symbolic links"},
		{WSAENAMETOOLONG, "File name too long"},
		{WSAEHOSTDOWN, "Host is down"},
		{WSAEHOSTUNREACH, "No route to host"},
		{WSAENOTEMPTY, "Directory not empty"},
		{WSAEPROCLIM, "Too many processes"},
		{WSAEUSERS, "Too many users"},
		{WSAEDQUOT, "Disc quota exceeded"},
		{WSAESTALE, "Stale NFS file handle"},
		{WSAEREMOTE, "Too many levels of remote in path"},
		{WSASYSNOTREADY, "Network subsystem is unvailable"},
		{WSAVERNOTSUPPORTED, "WinSock version is not supported"},
		{WSANOTINITIALISED,
		  "Successful WSAStartup() not yet performed"},
		{WSAEDISCON, "Graceful shutdown in progress"},
		/* Resolver errors */
		{WSAHOST_NOT_FOUND, "No such host is known"},
		{WSATRY_AGAIN, "Host not found, or server failed"},
		{WSANO_RECOVERY, "Unexpected server error encountered"},
		{WSANO_DATA, "Valid name without requested data"},
		{WSANO_ADDRESS, "No address, look for MX record"},
		{0, NULL}
	};
	if (err_no) {
		PyObject *v;
		const char *msg = "winsock error";

		for (msgp = msgs; msgp->msg; msgp++) {
			if (err_no == msgp->no) {
				msg = msgp->msg;
				break;
			}
		}

		v = Py_BuildValue("(is)", err_no, msg);
		if (v != NULL) {
			PyErr_SetObject(socket_error, v);
			Py_DECREF(v);
		}
		return NULL;
	}
	else
#endif

#if defined(PYOS_OS2) && !defined(PYCC_GCC)
	if (sock_errno() != NO_ERROR) {
		APIRET rc;
		ULONG  msglen;
		char outbuf[100];
		int myerrorcode = sock_errno();

		/* Retrieve socket-related error message from MPTN.MSG file */
		rc = DosGetMessage(NULL, 0, outbuf, sizeof(outbuf),
				   myerrorcode - SOCBASEERR + 26,
				   "mptn.msg",
				   &msglen);
		if (rc == NO_ERROR) {
			PyObject *v;

			/* OS/2 doesn't guarantee a terminator */
			outbuf[msglen] = '\0';
			if (strlen(outbuf) > 0) {
				/* If non-empty msg, trim CRLF */
				char *lastc = &outbuf[ strlen(outbuf)-1 ];
				while (lastc > outbuf && isspace(*lastc)) {
					/* Trim trailing whitespace (CRLF) */
					*lastc-- = '\0';
				}
			}
			v = Py_BuildValue("(is)", myerrorcode, outbuf);
			if (v != NULL) {
				PyErr_SetObject(socket_error, v);
				Py_DECREF(v);
			}
			return NULL;
		}
	}
#endif

#if defined(RISCOS)
	if (_inet_error.errnum != NULL) {
		PyObject *v;
		v = Py_BuildValue("(is)", errno, _inet_err());
		if (v != NULL) {
			PyErr_SetObject(socket_error, v);
			Py_DECREF(v);
		}
		return NULL;
	}
#endif

	return PyErr_SetFromErrno(socket_error);
}


static PyObject *
set_herror(int h_error)
{
	PyObject *v;

#ifdef HAVE_HSTRERROR
	v = Py_BuildValue("(is)", h_error, (char *)hstrerror(h_error));
#else
	v = Py_BuildValue("(is)", h_error, "host not found");
#endif
	if (v != NULL) {
		PyErr_SetObject(socket_herror, v);
		Py_DECREF(v);
	}

	return NULL;
}


static PyObject *
set_gaierror(int error)
{
	PyObject *v;

#ifdef EAI_SYSTEM
	/* EAI_SYSTEM is not available on Windows XP. */
	if (error == EAI_SYSTEM)
		return set_error();
#endif

#ifdef HAVE_GAI_STRERROR
	v = Py_BuildValue("(is)", error, gai_strerror(error));
#else
	v = Py_BuildValue("(is)", error, "getaddrinfo failed");
#endif
	if (v != NULL) {
		PyErr_SetObject(socket_gaierror, v);
		Py_DECREF(v);
	}

	return NULL;
}

/* Function to perform the setting of socket blocking mode
   internally. block = (1 | 0). */
static int
internal_setblocking(PySocketSockObject *s, int block)
{
#ifndef RISCOS
#ifndef MS_WINDOWS
	int delay_flag;
#endif
#endif

	Py_BEGIN_ALLOW_THREADS
#ifdef __BEOS__
	block = !block;
	setsockopt(s->sock_fd, SOL_SOCKET, SO_NONBLOCK,
		   (void *)(&block), sizeof(int));
#else
#ifndef RISCOS
#ifndef MS_WINDOWS
#if defined(PYOS_OS2) && !defined(PYCC_GCC)
	block = !block;
	ioctl(s->sock_fd, FIONBIO, (caddr_t)&block, sizeof(block));
#elif defined(__VMS)
	block = !block;
	ioctl(s->sock_fd, FIONBIO, (char *)&block);
#else  /* !PYOS_OS2 && !_VMS */
	delay_flag = fcntl(s->sock_fd, F_GETFL, 0);
	if (block)
		delay_flag &= (~O_NONBLOCK);
	else
		delay_flag |= O_NONBLOCK;
	fcntl(s->sock_fd, F_SETFL, delay_flag);
#endif /* !PYOS_OS2 */
#else /* MS_WINDOWS */
	block = !block;
	ioctlsocket(s->sock_fd, FIONBIO, (u_long*)&block);
#endif /* MS_WINDOWS */
#else /* RISCOS */
	block = !block;
	socketioctl(s->sock_fd, FIONBIO, (u_long*)&block);
#endif /* RISCOS */
#endif /* __BEOS__ */
	Py_END_ALLOW_THREADS

	/* Since these don't return anything */
	return 1;
}

/* Do a select() on the socket, if necessary (sock_timeout > 0).
   The argument writing indicates the direction.
   This does not raise an exception; we'll let our caller do that
   after they've reacquired the interpreter lock.
   Returns 1 on timeout, 0 otherwise. */
static int
internal_select(PySocketSockObject *s, int writing)
{
	fd_set fds;
	struct timeval tv;
	int n;

	/* Nothing to do unless we're in timeout mode (not non-blocking) */
	if (s->sock_timeout <= 0.0)
		return 0;

	/* Guard against closed socket */
	if (s->sock_fd < 0)
		return 0;

	/* Construct the arguments to select */
	tv.tv_sec = (int)s->sock_timeout;
	tv.tv_usec = (int)((s->sock_timeout - tv.tv_sec) * 1e6);
	FD_ZERO(&fds);
	FD_SET(s->sock_fd, &fds);

	/* See if the socket is ready */
	if (writing)
		n = select(s->sock_fd+1, NULL, &fds, NULL, &tv);
	else
		n = select(s->sock_fd+1, &fds, NULL, NULL, &tv);
	if (n == 0)
		return 1;
	return 0;
}

/* Initialize a new socket object. */

static double defaulttimeout = -1.0; /* Default timeout for new sockets */

PyMODINIT_FUNC
init_sockobject(PySocketSockObject *s,
		SOCKET_T fd, int family, int type, int proto)
{
#ifdef RISCOS
	int block = 1;
#endif
	s->sock_fd = fd;
	s->sock_family = family;
	s->sock_type = type;
	s->sock_proto = proto;
	s->sock_timeout = defaulttimeout;

	s->errorhandler = &set_error;

	if (defaulttimeout >= 0.0)
		internal_setblocking(s, 0);

#ifdef RISCOS
	if (taskwindow)
		socketioctl(s->sock_fd, 0x80046679, (u_long*)&block);
#endif
}


/* Create a new socket object.
   This just creates the object and initializes it.
   If the creation fails, return NULL and set an exception (implicit
   in NEWOBJ()). */

static PySocketSockObject *
new_sockobject(SOCKET_T fd, int family, int type, int proto)
{
	PySocketSockObject *s;
	s = (PySocketSockObject *)
		PyType_GenericNew(&sock_type, NULL, NULL);
	if (s != NULL)
		init_sockobject(s, fd, family, type, proto);
	return s;
}


/* Lock to allow python interpreter to continue, but only allow one
   thread to be in gethostbyname or getaddrinfo */
#if defined(USE_GETHOSTBYNAME_LOCK) || defined(USE_GETADDRINFO_LOCK)
PyThread_type_lock netdb_lock;
#endif


/* Convert a string specifying a host name or one of a few symbolic
   names to a numeric IP address.  This usually calls gethostbyname()
   to do the work; the names "" and "<broadcast>" are special.
   Return the length (IPv4 should be 4 bytes), or negative if
   an error occurred; then an exception is raised. */

static int
setipaddr(char *name, struct sockaddr *addr_ret, size_t addr_ret_size, int af)
{
	struct addrinfo hints, *res;
	int error;
	int d1, d2, d3, d4;
	char ch;

	memset((void *) addr_ret, '\0', sizeof(*addr_ret));
	if (name[0] == '\0') {
		int siz;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = af;
		hints.ai_socktype = SOCK_DGRAM;	/*dummy*/
		hints.ai_flags = AI_PASSIVE;
		Py_BEGIN_ALLOW_THREADS
		ACQUIRE_GETADDRINFO_LOCK
		error = getaddrinfo(NULL, "0", &hints, &res);
		Py_END_ALLOW_THREADS
		/* We assume that those thread-unsafe getaddrinfo() versions
		   *are* safe regarding their return value, ie. that a
		   subsequent call to getaddrinfo() does not destroy the
		   outcome of the first call. */
		RELEASE_GETADDRINFO_LOCK
		if (error) {
			set_gaierror(error);
			return -1;
		}
		switch (res->ai_family) {
		case AF_INET:
			siz = 4;
			break;
#ifdef ENABLE_IPV6
		case AF_INET6:
			siz = 16;
			break;
#endif
		default:
			freeaddrinfo(res);
			PyErr_SetString(socket_error,
				"unsupported address family");
			return -1;
		}
		if (res->ai_next) {
			freeaddrinfo(res);
			PyErr_SetString(socket_error,
				"wildcard resolved to multiple address");
			return -1;
		}
		if (res->ai_addrlen < addr_ret_size)
			addr_ret_size = res->ai_addrlen;
		memcpy(addr_ret, res->ai_addr, addr_ret_size);
		freeaddrinfo(res);
		return siz;
	}
	if (name[0] == '<' && strcmp(name, "<broadcast>") == 0) {
		struct sockaddr_in *sin;
		if (af != AF_INET && af != AF_UNSPEC) {
			PyErr_SetString(socket_error,
				"address family mismatched");
			return -1;
		}
		sin = (struct sockaddr_in *)addr_ret;
		memset((void *) sin, '\0', sizeof(*sin));
		sin->sin_family = AF_INET;
#ifdef HAVE_SOCKADDR_SA_LEN
		sin->sin_len = sizeof(*sin);
#endif
		sin->sin_addr.s_addr = INADDR_BROADCAST;
		return sizeof(sin->sin_addr);
	}
	if (sscanf(name, "%d.%d.%d.%d%c", &d1, &d2, &d3, &d4, &ch) == 4 &&
	    0 <= d1 && d1 <= 255 && 0 <= d2 && d2 <= 255 &&
	    0 <= d3 && d3 <= 255 && 0 <= d4 && d4 <= 255) {
		struct sockaddr_in *sin;
		sin = (struct sockaddr_in *)addr_ret;
		sin->sin_addr.s_addr = htonl(
			((long) d1 << 24) | ((long) d2 << 16) |
			((long) d3 << 8) | ((long) d4 << 0));
		sin->sin_family = AF_INET;
#ifdef HAVE_SOCKADDR_SA_LEN
		sin->sin_len = sizeof(*sin);
#endif
		return 4;
	}
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = af;
	Py_BEGIN_ALLOW_THREADS
	ACQUIRE_GETADDRINFO_LOCK
	error = getaddrinfo(name, NULL, &hints, &res);
#if defined(__digital__) && defined(__unix__)
	if (error == EAI_NONAME && af == AF_UNSPEC) {
		/* On Tru64 V5.1, numeric-to-addr conversion fails
		   if no address family is given. Assume IPv4 for now.*/
		hints.ai_family = AF_INET;
		error = getaddrinfo(name, NULL, &hints, &res);
	}
#endif
	Py_END_ALLOW_THREADS
	RELEASE_GETADDRINFO_LOCK  /* see comment in setipaddr() */
	if (error) {
		set_gaierror(error);
		return -1;
	}
	if (res->ai_addrlen < addr_ret_size)
		addr_ret_size = res->ai_addrlen;
	memcpy((char *) addr_ret, res->ai_addr, addr_ret_size);
	freeaddrinfo(res);
	switch (addr_ret->sa_family) {
	case AF_INET:
		return 4;
#ifdef ENABLE_IPV6
	case AF_INET6:
		return 16;
#endif
	default:
		PyErr_SetString(socket_error, "unknown address family");
		return -1;
	}
}


/* Create a string object representing an IP address.
   This is always a string of the form 'dd.dd.dd.dd' (with variable
   size numbers). */

static PyObject *
makeipaddr(struct sockaddr *addr, int addrlen)
{
	char buf[NI_MAXHOST];
	int error;

	error = getnameinfo(addr, addrlen, buf, sizeof(buf), NULL, 0,
		NI_NUMERICHOST);
	if (error) {
		set_gaierror(error);
		return NULL;
	}
	return PyString_FromString(buf);
}


#ifdef USE_BLUETOOTH
/* Convert a string representation of a Bluetooth address into a numeric
   address.  Returns the length (6), or raises an exception and returns -1 if
   an error occurred. */

static int
setbdaddr(char *name, bdaddr_t *bdaddr)
{
	unsigned int b0, b1, b2, b3, b4, b5;
	char ch;
	int n;

	n = sscanf(name, "%X:%X:%X:%X:%X:%X%c",
		   &b5, &b4, &b3, &b2, &b1, &b0, &ch);
	if (n == 6 && (b0 | b1 | b2 | b3 | b4 | b5) < 256) {
		bdaddr->b[0] = b0;
		bdaddr->b[1] = b1;
		bdaddr->b[2] = b2;
		bdaddr->b[3] = b3;
		bdaddr->b[4] = b4;
		bdaddr->b[5] = b5;
		return 6;
	} else {
		PyErr_SetString(socket_error, "bad bluetooth address");
		return -1;
	}
}

/* Create a string representation of the Bluetooth address.  This is always a
   string of the form 'XX:XX:XX:XX:XX:XX' where XX is a two digit hexadecimal
   value (zero padded if necessary). */

static PyObject *
makebdaddr(bdaddr_t *bdaddr)
{
	char buf[(6 * 2) + 5 + 1];

	sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
		bdaddr->b[5], bdaddr->b[4], bdaddr->b[3],
		bdaddr->b[2], bdaddr->b[1], bdaddr->b[0]);
	return PyString_FromString(buf);
}
#endif


/* Create an object representing the given socket address,
   suitable for passing it back to bind(), connect() etc.
   The family field of the sockaddr structure is inspected
   to determine what kind of address it really is. */

/*ARGSUSED*/
static PyObject *
makesockaddr(int sockfd, struct sockaddr *addr, int addrlen, int proto)
{
	if (addrlen == 0) {
		/* No address -- may be recvfrom() from known socket */
		Py_INCREF(Py_None);
		return Py_None;
	}

#ifdef __BEOS__
	/* XXX: BeOS version of accept() doesn't set family correctly */
	addr->sa_family = AF_INET;
#endif

	switch (addr->sa_family) {

	case AF_INET:
	{
		struct sockaddr_in *a;
		PyObject *addrobj = makeipaddr(addr, sizeof(*a));
		PyObject *ret = NULL;
		if (addrobj) {
			a = (struct sockaddr_in *)addr;
			ret = Py_BuildValue("Oi", addrobj, ntohs(a->sin_port));
			Py_DECREF(addrobj);
		}
		return ret;
	}

#if defined(AF_UNIX)
	case AF_UNIX:
	{
		struct sockaddr_un *a = (struct sockaddr_un *) addr;
		return PyString_FromString(a->sun_path);
	}
#endif /* AF_UNIX */

#ifdef ENABLE_IPV6
	case AF_INET6:
	{
		struct sockaddr_in6 *a;
		PyObject *addrobj = makeipaddr(addr, sizeof(*a));
		PyObject *ret = NULL;
		if (addrobj) {
			a = (struct sockaddr_in6 *)addr;
			ret = Py_BuildValue("Oiii",
					    addrobj,
					    ntohs(a->sin6_port),
					    a->sin6_flowinfo,
					    a->sin6_scope_id);
			Py_DECREF(addrobj);
		}
		return ret;
	}
#endif

#ifdef USE_BLUETOOTH
	case AF_BLUETOOTH:
		switch (proto) {

		case BTPROTO_L2CAP:
		{
			struct sockaddr_l2 *a = (struct sockaddr_l2 *) addr;
			PyObject *addrobj = makebdaddr(&_BT_L2_MEMB(a, bdaddr));
			PyObject *ret = NULL;
			if (addrobj) {
				ret = Py_BuildValue("Oi",
						    addrobj,
						    _BT_L2_MEMB(a, psm));
				Py_DECREF(addrobj);
			}
			return ret;
		}

		case BTPROTO_RFCOMM:
		{
			struct sockaddr_rc *a = (struct sockaddr_rc *) addr;
			PyObject *addrobj = makebdaddr(&_BT_RC_MEMB(a, bdaddr));
			PyObject *ret = NULL;
			if (addrobj) {
				ret = Py_BuildValue("Oi",
						    addrobj,
						    _BT_RC_MEMB(a, channel));
				Py_DECREF(addrobj);
			}
			return ret;
		}

#if !defined(__FreeBSD__)
		case BTPROTO_SCO:
		{
			struct sockaddr_sco *a = (struct sockaddr_sco *) addr;
			return makebdaddr(&_BT_SCO_MEMB(a, bdaddr));
		}
#endif

		}
#endif

#ifdef HAVE_NETPACKET_PACKET_H
	case AF_PACKET:
	{
		struct sockaddr_ll *a = (struct sockaddr_ll *)addr;
		char *ifname = "";
		struct ifreq ifr;
		/* need to look up interface name give index */
		if (a->sll_ifindex) {
			ifr.ifr_ifindex = a->sll_ifindex;
			if (ioctl(sockfd, SIOCGIFNAME, &ifr) == 0)
				ifname = ifr.ifr_name;
		}
		return Py_BuildValue("shbhs#",
				     ifname,
				     ntohs(a->sll_protocol),
				     a->sll_pkttype,
				     a->sll_hatype,
				     a->sll_addr,
				     a->sll_halen);
	}
#endif

	/* More cases here... */

	default:
		/* If we don't know the address family, don't raise an
		   exception -- return it as a tuple. */
		return Py_BuildValue("is#",
				     addr->sa_family,
				     addr->sa_data,
				     sizeof(addr->sa_data));

	}
}


/* Parse a socket address argument according to the socket object's
   address family.  Return 1 if the address was in the proper format,
   0 of not.  The address is returned through addr_ret, its length
   through len_ret. */

static int
getsockaddrarg(PySocketSockObject *s, PyObject *args,
	       struct sockaddr **addr_ret, int *len_ret)
{
	switch (s->sock_family) {

#if defined(AF_UNIX)
	case AF_UNIX:
	{
		struct sockaddr_un* addr;
		char *path;
		int len;
		addr = (struct sockaddr_un*)&(s->sock_addr).un;
		if (!PyArg_Parse(args, "t#", &path, &len))
			return 0;
		if (len > sizeof addr->sun_path) {
			PyErr_SetString(socket_error,
					"AF_UNIX path too long");
			return 0;
		}
		addr->sun_family = s->sock_family;
		memcpy(addr->sun_path, path, len);
		addr->sun_path[len] = 0;
		*addr_ret = (struct sockaddr *) addr;
#if defined(PYOS_OS2)
		*len_ret = sizeof(*addr);
#else
		*len_ret = len + sizeof(*addr) - sizeof(addr->sun_path);
#endif
		return 1;
	}
#endif /* AF_UNIX */

	case AF_INET:
	{
		struct sockaddr_in* addr;
		char *host;
		int port, result;
 		addr=(struct sockaddr_in*)&(s->sock_addr).in;
		if (!PyTuple_Check(args)) {
			PyErr_Format(
				PyExc_TypeError,
				"getsockaddrarg: "
				"AF_INET address must be tuple, not %.500s",
				args->ob_type->tp_name);
			return 0;
		}
		if (!PyArg_ParseTuple(args, "eti:getsockaddrarg", 
				      "idna", &host, &port))
			return 0;
                result = setipaddr(host, (struct sockaddr *)addr, 
                                   sizeof(*addr),  AF_INET);
                PyMem_Free(host);
                if (result < 0)
			return 0;
		addr->sin_family = AF_INET;
		addr->sin_port = htons((short)port);
		*addr_ret = (struct sockaddr *) addr;
		*len_ret = sizeof *addr;
		return 1;
	}

#ifdef ENABLE_IPV6
	case AF_INET6:
	{
		struct sockaddr_in6* addr;
		char *host;
		int port, flowinfo, scope_id, result;
 		addr = (struct sockaddr_in6*)&(s->sock_addr).in6;
		flowinfo = scope_id = 0;
		if (!PyArg_ParseTuple(args, "eti|ii", 
				      "idna", &host, &port, &flowinfo,
				      &scope_id)) {
			return 0;
		}
                result = setipaddr(host, (struct sockaddr *)addr,  
                                   sizeof(*addr), AF_INET6);
                PyMem_Free(host);
                if (result < 0)
			return 0;
		addr->sin6_family = s->sock_family;
		addr->sin6_port = htons((short)port);
		addr->sin6_flowinfo = flowinfo;
		addr->sin6_scope_id = scope_id;
		*addr_ret = (struct sockaddr *) addr;
		*len_ret = sizeof *addr;
		return 1;
	}
#endif

#ifdef USE_BLUETOOTH
	case AF_BLUETOOTH:
	{
		switch (s->sock_proto) {
		case BTPROTO_L2CAP:
		{
			struct sockaddr_l2 *addr = (struct sockaddr_l2 *) _BT_SOCKADDR_MEMB(s, l2);
			char *straddr;

			_BT_L2_MEMB(addr, family) = AF_BLUETOOTH;
			if (!PyArg_ParseTuple(args, "si", &straddr,
					      &_BT_L2_MEMB(addr, psm))) {
				PyErr_SetString(socket_error, "getsockaddrarg: "
						"wrong format");
				return 0;
			}
			if (setbdaddr(straddr, &_BT_L2_MEMB(addr, bdaddr)) < 0)
				return 0;

			*addr_ret = (struct sockaddr *) addr;
			*len_ret = sizeof *addr;
			return 1;
		}
		case BTPROTO_RFCOMM:
		{
			struct sockaddr_rc *addr = (struct sockaddr_rc *) _BT_SOCKADDR_MEMB(s, rc);
			char *straddr;

			_BT_RC_MEMB(addr, family) = AF_BLUETOOTH;
			if (!PyArg_ParseTuple(args, "si", &straddr,
					      &_BT_RC_MEMB(addr, channel))) {
				PyErr_SetString(socket_error, "getsockaddrarg: "
						"wrong format");
				return 0;
			}
			if (setbdaddr(straddr, &_BT_RC_MEMB(addr, bdaddr)) < 0)
				return 0;

			*addr_ret = (struct sockaddr *) addr;
			*len_ret = sizeof *addr;
			return 1;
		}
#if !defined(__FreeBSD__)
		case BTPROTO_SCO:
		{
			struct sockaddr_sco *addr = (struct sockaddr_sco *) _BT_SOCKADDR_MEMB(s, sco);
			char *straddr;

			_BT_SCO_MEMB(addr, family) = AF_BLUETOOTH;
			straddr = PyString_AsString(args);
			if (straddr == NULL) {
				PyErr_SetString(socket_error, "getsockaddrarg: "
						"wrong format");
				return 0;
			}
			if (setbdaddr(straddr, &_BT_SCO_MEMB(addr, bdaddr)) < 0)
				return 0;

			*addr_ret = (struct sockaddr *) addr;
			*len_ret = sizeof *addr;
			return 1;
		}
#endif
		default:
			PyErr_SetString(socket_error, "getsockaddrarg: unknown Bluetooth protocol");
			return 0;
		}
	}
#endif

#ifdef HAVE_NETPACKET_PACKET_H
	case AF_PACKET:
	{
		struct sockaddr_ll* addr;
		struct ifreq ifr;
		char *interfaceName;
		int protoNumber;
		int hatype = 0;
		int pkttype = 0;
		char *haddr = NULL;
		unsigned int halen = 0;

		if (!PyArg_ParseTuple(args, "si|iis#", &interfaceName,
				      &protoNumber, &pkttype, &hatype,
				      &haddr, &halen))
			return 0;
		strncpy(ifr.ifr_name, interfaceName, sizeof(ifr.ifr_name));
		ifr.ifr_name[(sizeof(ifr.ifr_name))-1] = '\0';
		if (ioctl(s->sock_fd, SIOCGIFINDEX, &ifr) < 0) {
		        s->errorhandler();
			return 0;
		}
		addr = &(s->sock_addr.ll);
		addr->sll_family = AF_PACKET;
		addr->sll_protocol = htons((short)protoNumber);
		addr->sll_ifindex = ifr.ifr_ifindex;
		addr->sll_pkttype = pkttype;
		addr->sll_hatype = hatype;
		if (halen > 8) {
		  PyErr_SetString(PyExc_ValueError,
				  "Hardware address must be 8 bytes or less");
		  return 0;
		}
		if (halen != 0) {
		  memcpy(&addr->sll_addr, haddr, halen);
		}
		addr->sll_halen = halen;
		*addr_ret = (struct sockaddr *) addr;
		*len_ret = sizeof *addr;
		return 1;
	}
#endif

	/* More cases here... */

	default:
		PyErr_SetString(socket_error, "getsockaddrarg: bad family");
		return 0;

	}
}


/* Get the address length according to the socket object's address family.
   Return 1 if the family is known, 0 otherwise.  The length is returned
   through len_ret. */

static int
getsockaddrlen(PySocketSockObject *s, socklen_t *len_ret)
{
	switch (s->sock_family) {

#if defined(AF_UNIX)
	case AF_UNIX:
	{
		*len_ret = sizeof (struct sockaddr_un);
		return 1;
	}
#endif /* AF_UNIX */

	case AF_INET:
	{
		*len_ret = sizeof (struct sockaddr_in);
		return 1;
	}

#ifdef ENABLE_IPV6
	case AF_INET6:
	{
		*len_ret = sizeof (struct sockaddr_in6);
		return 1;
	}
#endif

#ifdef USE_BLUETOOTH
	case AF_BLUETOOTH:
	{
		switch(s->sock_proto)
		{

		case BTPROTO_L2CAP:
			*len_ret = sizeof (struct sockaddr_l2);
			return 1;
		case BTPROTO_RFCOMM:
			*len_ret = sizeof (struct sockaddr_rc);
			return 1;
#if !defined(__FreeBSD__)
		case BTPROTO_SCO:
			*len_ret = sizeof (struct sockaddr_sco);
			return 1;
#endif
		default:
			PyErr_SetString(socket_error, "getsockaddrlen: "
					"unknown BT protocol");
			return 0;

		}
	}
#endif

#ifdef HAVE_NETPACKET_PACKET_H
	case AF_PACKET:
	{
		*len_ret = sizeof (struct sockaddr_ll);
		return 1;
	}
#endif

	/* More cases here... */

	default:
		PyErr_SetString(socket_error, "getsockaddrlen: bad family");
		return 0;

	}
}


/* s.accept() method */

static PyObject *
sock_accept(PySocketSockObject *s)
{
	char addrbuf[256];
	SOCKET_T newfd;
	socklen_t addrlen;
	PyObject *sock = NULL;
	PyObject *addr = NULL;
	PyObject *res = NULL;
	int timeout;

	if (!getsockaddrlen(s, &addrlen))
		return NULL;
	memset(addrbuf, 0, addrlen);

#ifdef MS_WINDOWS
	newfd = INVALID_SOCKET;
#else
	newfd = -1;
#endif

	Py_BEGIN_ALLOW_THREADS
	timeout = internal_select(s, 0);
	if (!timeout)
		newfd = accept(s->sock_fd, (struct sockaddr *) addrbuf,
			       &addrlen);
	Py_END_ALLOW_THREADS

	if (timeout) {
		PyErr_SetString(socket_timeout, "timed out");
		return NULL;
	}

#ifdef MS_WINDOWS
	if (newfd == INVALID_SOCKET)
#else
	if (newfd < 0)
#endif
		return s->errorhandler();

	/* Create the new object with unspecified family,
	   to avoid calls to bind() etc. on it. */
	sock = (PyObject *) new_sockobject(newfd,
					   s->sock_family,
					   s->sock_type,
					   s->sock_proto);

	if (sock == NULL) {
		SOCKETCLOSE(newfd);
		goto finally;
	}
	addr = makesockaddr(s->sock_fd, (struct sockaddr *)addrbuf,
			    addrlen, s->sock_proto);
	if (addr == NULL)
		goto finally;

	res = PyTuple_Pack(2, sock, addr);

finally:
	Py_XDECREF(sock);
	Py_XDECREF(addr);
	return res;
}

PyDoc_STRVAR(accept_doc,
"accept() -> (socket object, address info)\n\
\n\
Wait for an incoming connection.  Return a new socket representing the\n\
connection, and the address of the client.  For IP sockets, the address\n\
info is a pair (hostaddr, port).");

/* s.setblocking(flag) method.  Argument:
   False -- non-blocking mode; same as settimeout(0)
   True -- blocking mode; same as settimeout(None)
*/

static PyObject *
sock_setblocking(PySocketSockObject *s, PyObject *arg)
{
	int block;

	block = PyInt_AsLong(arg);
	if (block == -1 && PyErr_Occurred())
		return NULL;

	s->sock_timeout = block ? -1.0 : 0.0;
	internal_setblocking(s, block);

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(setblocking_doc,
"setblocking(flag)\n\
\n\
Set the socket to blocking (flag is true) or non-blocking (false).\n\
setblocking(True) is equivalent to settimeout(None);\n\
setblocking(False) is equivalent to settimeout(0.0).");

/* s.settimeout(timeout) method.  Argument:
   None -- no timeout, blocking mode; same as setblocking(True)
   0.0  -- non-blocking mode; same as setblocking(False)
   > 0  -- timeout mode; operations time out after timeout seconds
   < 0  -- illegal; raises an exception
*/
static PyObject *
sock_settimeout(PySocketSockObject *s, PyObject *arg)
{
	double timeout;

	if (arg == Py_None)
		timeout = -1.0;
	else {
		timeout = PyFloat_AsDouble(arg);
		if (timeout < 0.0) {
			if (!PyErr_Occurred())
				PyErr_SetString(PyExc_ValueError,
						"Timeout value out of range");
			return NULL;
		}
	}

	s->sock_timeout = timeout;
	internal_setblocking(s, timeout < 0.0);

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(settimeout_doc,
"settimeout(timeout)\n\
\n\
Set a timeout on socket operations.  'timeout' can be a float,\n\
giving in seconds, or None.  Setting a timeout of None disables\n\
the timeout feature and is equivalent to setblocking(1).\n\
Setting a timeout of zero is the same as setblocking(0).");

/* s.gettimeout() method.
   Returns the timeout associated with a socket. */
static PyObject *
sock_gettimeout(PySocketSockObject *s)
{
	if (s->sock_timeout < 0.0) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	else
		return PyFloat_FromDouble(s->sock_timeout);
}

PyDoc_STRVAR(gettimeout_doc,
"gettimeout() -> timeout\n\
\n\
Returns the timeout in floating seconds associated with socket \n\
operations. A timeout of None indicates that timeouts on socket \n\
operations are disabled.");

#ifdef RISCOS
/* s.sleeptaskw(1 | 0) method */

static PyObject *
sock_sleeptaskw(PySocketSockObject *s,PyObject *arg)
{
	int block;
	block = PyInt_AsLong(arg);
	if (block == -1 && PyErr_Occurred())
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	socketioctl(s->sock_fd, 0x80046679, (u_long*)&block);
	Py_END_ALLOW_THREADS

	Py_INCREF(Py_None);
	return Py_None;
}
PyDoc_STRVAR(sleeptaskw_doc,
"sleeptaskw(flag)\n\
\n\
Allow sleeps in taskwindows.");
#endif


/* s.setsockopt() method.
   With an integer third argument, sets an integer option.
   With a string third argument, sets an option from a buffer;
   use optional built-in module 'struct' to encode the string. */

static PyObject *
sock_setsockopt(PySocketSockObject *s, PyObject *args)
{
	int level;
	int optname;
	int res;
	char *buf;
	int buflen;
	int flag;

	if (PyArg_ParseTuple(args, "iii:setsockopt",
			     &level, &optname, &flag)) {
		buf = (char *) &flag;
		buflen = sizeof flag;
	}
	else {
		PyErr_Clear();
		if (!PyArg_ParseTuple(args, "iis#:setsockopt",
				      &level, &optname, &buf, &buflen))
			return NULL;
	}
	res = setsockopt(s->sock_fd, level, optname, (void *)buf, buflen);
	if (res < 0)
		return s->errorhandler();
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(setsockopt_doc,
"setsockopt(level, option, value)\n\
\n\
Set a socket option.  See the Unix manual for level and option.\n\
The value argument can either be an integer or a string.");


/* s.getsockopt() method.
   With two arguments, retrieves an integer option.
   With a third integer argument, retrieves a string buffer of that size;
   use optional built-in module 'struct' to decode the string. */

static PyObject *
sock_getsockopt(PySocketSockObject *s, PyObject *args)
{
	int level;
	int optname;
	int res;
	PyObject *buf;
	socklen_t buflen = 0;

#ifdef __BEOS__
	/* We have incomplete socket support. */
	PyErr_SetString(socket_error, "getsockopt not supported");
	return NULL;
#else

	if (!PyArg_ParseTuple(args, "ii|i:getsockopt",
			      &level, &optname, &buflen))
		return NULL;

	if (buflen == 0) {
		int flag = 0;
		socklen_t flagsize = sizeof flag;
		res = getsockopt(s->sock_fd, level, optname,
				 (void *)&flag, &flagsize);
		if (res < 0)
			return s->errorhandler();
		return PyInt_FromLong(flag);
	}
#ifdef __VMS
	if (buflen > 1024) {
#else
	if (buflen <= 0 || buflen > 1024) {
#endif
		PyErr_SetString(socket_error,
				"getsockopt buflen out of range");
		return NULL;
	}
	buf = PyString_FromStringAndSize((char *)NULL, buflen);
	if (buf == NULL)
		return NULL;
	res = getsockopt(s->sock_fd, level, optname,
			 (void *)PyString_AS_STRING(buf), &buflen);
	if (res < 0) {
		Py_DECREF(buf);
		return s->errorhandler();
	}
	_PyString_Resize(&buf, buflen);
	return buf;
#endif /* __BEOS__ */
}

PyDoc_STRVAR(getsockopt_doc,
"getsockopt(level, option[, buffersize]) -> value\n\
\n\
Get a socket option.  See the Unix manual for level and option.\n\
If a nonzero buffersize argument is given, the return value is a\n\
string of that length; otherwise it is an integer.");


/* s.bind(sockaddr) method */

static PyObject *
sock_bind(PySocketSockObject *s, PyObject *addro)
{
	struct sockaddr *addr;
	int addrlen;
	int res;

	if (!getsockaddrarg(s, addro, &addr, &addrlen))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = bind(s->sock_fd, addr, addrlen);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return s->errorhandler();
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(bind_doc,
"bind(address)\n\
\n\
Bind the socket to a local address.  For IP sockets, the address is a\n\
pair (host, port); the host must refer to the local host. For raw packet\n\
sockets the address is a tuple (ifname, proto [,pkttype [,hatype]])");


/* s.close() method.
   Set the file descriptor to -1 so operations tried subsequently
   will surely fail. */

static PyObject *
sock_close(PySocketSockObject *s)
{
	SOCKET_T fd;

	if ((fd = s->sock_fd) != -1) {
		s->sock_fd = -1;
		Py_BEGIN_ALLOW_THREADS
		(void) SOCKETCLOSE(fd);
		Py_END_ALLOW_THREADS
	}
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(close_doc,
"close()\n\
\n\
Close the socket.  It cannot be used after this call.");

static int
internal_connect(PySocketSockObject *s, struct sockaddr *addr, int addrlen,
		 int *timeoutp)
{
	int res, timeout;

	timeout = 0;
	res = connect(s->sock_fd, addr, addrlen);

#ifdef MS_WINDOWS

	if (s->sock_timeout > 0.0) {
		if (res < 0 && WSAGetLastError() == WSAEWOULDBLOCK) {
			/* This is a mess.  Best solution: trust select */
			fd_set fds;
			fd_set fds_exc;
			struct timeval tv;
			tv.tv_sec = (int)s->sock_timeout;
			tv.tv_usec = (int)((s->sock_timeout - tv.tv_sec) * 1e6);
			FD_ZERO(&fds);
			FD_SET(s->sock_fd, &fds);
			FD_ZERO(&fds_exc);
			FD_SET(s->sock_fd, &fds_exc);
			res = select(s->sock_fd+1, NULL, &fds, &fds_exc, &tv);
			if (res == 0) {
				res = WSAEWOULDBLOCK;
				timeout = 1;
			} else if (res > 0) {
				if (FD_ISSET(s->sock_fd, &fds))
					/* The socket is in the writeable set - this
					   means connected */
					res = 0;
				else {
					/* As per MS docs, we need to call getsockopt()
					   to get the underlying error */
					int res_size = sizeof res;
					/* It must be in the exception set */
					assert(FD_ISSET(s->sock_fd, &fds_exc));
					if (0 == getsockopt(s->sock_fd, SOL_SOCKET, SO_ERROR, 
					                    (char *)&res, &res_size))
						/* getsockopt also clears WSAGetLastError,
						   so reset it back. */
						WSASetLastError(res);
					else
						res = WSAGetLastError();
				}
			}
			/* else if (res < 0) an error occurred */
		}
	}

	if (res < 0)
		res = WSAGetLastError();

#else

	if (s->sock_timeout > 0.0) {
		if (res < 0 && errno == EINPROGRESS) {
			timeout = internal_select(s, 1);
			res = connect(s->sock_fd, addr, addrlen);
			if (res < 0 && errno == EISCONN)
				res = 0;
		}
	}

	if (res < 0)
		res = errno;

#endif
	*timeoutp = timeout;

	return res;
}

/* s.connect(sockaddr) method */

static PyObject *
sock_connect(PySocketSockObject *s, PyObject *addro)
{
	struct sockaddr *addr;
	int addrlen;
	int res;
	int timeout;

	if (!getsockaddrarg(s, addro, &addr, &addrlen))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	res = internal_connect(s, addr, addrlen, &timeout);
	Py_END_ALLOW_THREADS

	if (timeout) {
		PyErr_SetString(socket_timeout, "timed out");
		return NULL;
	}
	if (res != 0)
		return s->errorhandler();
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(connect_doc,
"connect(address)\n\
\n\
Connect the socket to a remote address.  For IP sockets, the address\n\
is a pair (host, port).");


/* s.connect_ex(sockaddr) method */

static PyObject *
sock_connect_ex(PySocketSockObject *s, PyObject *addro)
{
	struct sockaddr *addr;
	int addrlen;
	int res;
	int timeout;

	if (!getsockaddrarg(s, addro, &addr, &addrlen))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	res = internal_connect(s, addr, addrlen, &timeout);
	Py_END_ALLOW_THREADS

	return PyInt_FromLong((long) res);
}

PyDoc_STRVAR(connect_ex_doc,
"connect_ex(address) -> errno\n\
\n\
This is like connect(address), but returns an error code (the errno value)\n\
instead of raising an exception when an error occurs.");


/* s.fileno() method */

static PyObject *
sock_fileno(PySocketSockObject *s)
{
#if SIZEOF_SOCKET_T <= SIZEOF_LONG
	return PyInt_FromLong((long) s->sock_fd);
#else
	return PyLong_FromLongLong((PY_LONG_LONG)s->sock_fd);
#endif
}

PyDoc_STRVAR(fileno_doc,
"fileno() -> integer\n\
\n\
Return the integer file descriptor of the socket.");


#ifndef NO_DUP
/* s.dup() method */

static PyObject *
sock_dup(PySocketSockObject *s)
{
	SOCKET_T newfd;
	PyObject *sock;

	newfd = dup(s->sock_fd);
	if (newfd < 0)
		return s->errorhandler();
	sock = (PyObject *) new_sockobject(newfd,
					   s->sock_family,
					   s->sock_type,
					   s->sock_proto);
	if (sock == NULL)
		SOCKETCLOSE(newfd);
	return sock;
}

PyDoc_STRVAR(dup_doc,
"dup() -> socket object\n\
\n\
Return a new socket object connected to the same system resource.");

#endif


/* s.getsockname() method */

static PyObject *
sock_getsockname(PySocketSockObject *s)
{
	char addrbuf[256];
	int res;
	socklen_t addrlen;

	if (!getsockaddrlen(s, &addrlen))
		return NULL;
	memset(addrbuf, 0, addrlen);
	Py_BEGIN_ALLOW_THREADS
	res = getsockname(s->sock_fd, (struct sockaddr *) addrbuf, &addrlen);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return s->errorhandler();
	return makesockaddr(s->sock_fd, (struct sockaddr *) addrbuf, addrlen,
			    s->sock_proto);
}

PyDoc_STRVAR(getsockname_doc,
"getsockname() -> address info\n\
\n\
Return the address of the local endpoint.  For IP sockets, the address\n\
info is a pair (hostaddr, port).");


#ifdef HAVE_GETPEERNAME		/* Cray APP doesn't have this :-( */
/* s.getpeername() method */

static PyObject *
sock_getpeername(PySocketSockObject *s)
{
	char addrbuf[256];
	int res;
	socklen_t addrlen;

	if (!getsockaddrlen(s, &addrlen))
		return NULL;
	memset(addrbuf, 0, addrlen);
	Py_BEGIN_ALLOW_THREADS
	res = getpeername(s->sock_fd, (struct sockaddr *) addrbuf, &addrlen);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return s->errorhandler();
	return makesockaddr(s->sock_fd, (struct sockaddr *) addrbuf, addrlen,
			    s->sock_proto);
}

PyDoc_STRVAR(getpeername_doc,
"getpeername() -> address info\n\
\n\
Return the address of the remote endpoint.  For IP sockets, the address\n\
info is a pair (hostaddr, port).");

#endif /* HAVE_GETPEERNAME */


/* s.listen(n) method */

static PyObject *
sock_listen(PySocketSockObject *s, PyObject *arg)
{
	int backlog;
	int res;

	backlog = PyInt_AsLong(arg);
	if (backlog == -1 && PyErr_Occurred())
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	if (backlog < 1)
		backlog = 1;
	res = listen(s->sock_fd, backlog);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return s->errorhandler();
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(listen_doc,
"listen(backlog)\n\
\n\
Enable a server to accept connections.  The backlog argument must be at\n\
least 1; it specifies the number of unaccepted connection that the system\n\
will allow before refusing new connections.");


#ifndef NO_DUP
/* s.makefile(mode) method.
   Create a new open file object referring to a dupped version of
   the socket's file descriptor.  (The dup() call is necessary so
   that the open file and socket objects may be closed independent
   of each other.)
   The mode argument specifies 'r' or 'w' passed to fdopen(). */

static PyObject *
sock_makefile(PySocketSockObject *s, PyObject *args)
{
	extern int fclose(FILE *);
	char *mode = "r";
	int bufsize = -1;
#ifdef MS_WIN32
	Py_intptr_t fd;
#else
	int fd;
#endif
	FILE *fp;
	PyObject *f;
#ifdef __VMS
	char *mode_r = "r";
	char *mode_w = "w";
#endif

	if (!PyArg_ParseTuple(args, "|si:makefile", &mode, &bufsize))
		return NULL;
#ifdef __VMS
	if (strcmp(mode,"rb") == 0) {
	    mode = mode_r;
	}
	else {
		if (strcmp(mode,"wb") == 0) {
			mode = mode_w;
		}
	}
#endif
#ifdef MS_WIN32
	if (((fd = _open_osfhandle(s->sock_fd, _O_BINARY)) < 0) ||
	    ((fd = dup(fd)) < 0) || ((fp = fdopen(fd, mode)) == NULL))
#else
	if ((fd = dup(s->sock_fd)) < 0 || (fp = fdopen(fd, mode)) == NULL)
#endif
	{
		if (fd >= 0)
			SOCKETCLOSE(fd);
		return s->errorhandler();
	}
	f = PyFile_FromFile(fp, "<socket>", mode, fclose);
	if (f != NULL)
		PyFile_SetBufSize(f, bufsize);
	return f;
}

PyDoc_STRVAR(makefile_doc,
"makefile([mode[, buffersize]]) -> file object\n\
\n\
Return a regular file object corresponding to the socket.\n\
The mode and buffersize arguments are as for the built-in open() function.");

#endif /* NO_DUP */


/* s.recv(nbytes [,flags]) method */

static PyObject *
sock_recv(PySocketSockObject *s, PyObject *args)
{
	int len, n = 0, flags = 0, timeout;
	PyObject *buf;
#ifdef __VMS
	int read_length;
	char *read_buf;
#endif

	if (!PyArg_ParseTuple(args, "i|i:recv", &len, &flags))
		return NULL;

	if (len < 0) {
		PyErr_SetString(PyExc_ValueError,
				"negative buffersize in recv");
		return NULL;
	}

	buf = PyString_FromStringAndSize((char *) 0, len);
	if (buf == NULL)
		return NULL;

#ifndef __VMS
	Py_BEGIN_ALLOW_THREADS
	timeout = internal_select(s, 0);
	if (!timeout)
		n = recv(s->sock_fd, PyString_AS_STRING(buf), len, flags);
	Py_END_ALLOW_THREADS

	if (timeout) {
		Py_DECREF(buf);
		PyErr_SetString(socket_timeout, "timed out");
		return NULL;
	}
	if (n < 0) {
		Py_DECREF(buf);
		return s->errorhandler();
	}
	if (n != len)
		_PyString_Resize(&buf, n);
#else
	read_buf = PyString_AsString(buf);
	read_length = len;
	while (read_length != 0) {
		unsigned int segment;

		segment = read_length /SEGMENT_SIZE;
		if (segment != 0) {
			segment = SEGMENT_SIZE;
		}
		else {
			segment = read_length;
		}

		Py_BEGIN_ALLOW_THREADS
		timeout = internal_select(s, 0);
		if (!timeout)
			n = recv(s->sock_fd, read_buf, segment, flags);
		Py_END_ALLOW_THREADS

		if (timeout) {
			Py_DECREF(buf);
			PyErr_SetString(socket_timeout, "timed out");
			return NULL;
		}
		if (n < 0) {
			Py_DECREF(buf);
			return s->errorhandler();
		}
		if (n != read_length) {
			read_buf += n;
			break;
		}

		read_length -= segment;
		read_buf += segment;
	}
	if (_PyString_Resize(&buf, (read_buf - PyString_AsString(buf))) < 0)
	{
	    return NULL;
	}
#endif /* !__VMS */
	return buf;
}

PyDoc_STRVAR(recv_doc,
"recv(buffersize[, flags]) -> data\n\
\n\
Receive up to buffersize bytes from the socket.  For the optional flags\n\
argument, see the Unix manual.  When no data is available, block until\n\
at least one byte is available or until the remote end is closed.  When\n\
the remote end is closed and all data is read, return the empty string.");


/* s.recvfrom(nbytes [,flags]) method */

static PyObject *
sock_recvfrom(PySocketSockObject *s, PyObject *args)
{
	char addrbuf[256];
	PyObject *buf = NULL;
	PyObject *addr = NULL;
	PyObject *ret = NULL;
	int len, n = 0, flags = 0, timeout;
	socklen_t addrlen;

	if (!PyArg_ParseTuple(args, "i|i:recvfrom", &len, &flags))
		return NULL;

	if (!getsockaddrlen(s, &addrlen))
		return NULL;
	buf = PyString_FromStringAndSize((char *) 0, len);
	if (buf == NULL)
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	memset(addrbuf, 0, addrlen);
	timeout = internal_select(s, 0);
	if (!timeout)
		n = recvfrom(s->sock_fd, PyString_AS_STRING(buf), len, flags,
#ifndef MS_WINDOWS
#if defined(PYOS_OS2) && !defined(PYCC_GCC)
			     (struct sockaddr *)addrbuf, &addrlen
#else
			     (void *)addrbuf, &addrlen
#endif
#else
			     (struct sockaddr *)addrbuf, &addrlen
#endif
			);
	Py_END_ALLOW_THREADS

	if (timeout) {
		Py_DECREF(buf);
		PyErr_SetString(socket_timeout, "timed out");
		return NULL;
	}
	if (n < 0) {
		Py_DECREF(buf);
		return s->errorhandler();
	}

	if (n != len && _PyString_Resize(&buf, n) < 0)
		return NULL;

	if (!(addr = makesockaddr(s->sock_fd, (struct sockaddr *)addrbuf,
				  addrlen, s->sock_proto)))
		goto finally;

	ret = PyTuple_Pack(2, buf, addr);

finally:
	Py_XDECREF(addr);
	Py_XDECREF(buf);
	return ret;
}

PyDoc_STRVAR(recvfrom_doc,
"recvfrom(buffersize[, flags]) -> (data, address info)\n\
\n\
Like recv(buffersize, flags) but also return the sender's address info.");

/* s.send(data [,flags]) method */

static PyObject *
sock_send(PySocketSockObject *s, PyObject *args)
{
	char *buf;
	int len, n = 0, flags = 0, timeout;
#ifdef __VMS
	int send_length;
#endif

	if (!PyArg_ParseTuple(args, "s#|i:send", &buf, &len, &flags))
		return NULL;

#ifndef __VMS
	Py_BEGIN_ALLOW_THREADS
	timeout = internal_select(s, 1);
	if (!timeout)
		n = send(s->sock_fd, buf, len, flags);
	Py_END_ALLOW_THREADS

	if (timeout) {
		PyErr_SetString(socket_timeout, "timed out");
		return NULL;
	}
	if (n < 0)
		return s->errorhandler();
#else
	/* Divide packet into smaller segments for	*/
	/*  TCP/IP Services for OpenVMS			*/
	send_length = len;
	while (send_length != 0) {
		unsigned int segment;

		segment = send_length / SEGMENT_SIZE;
		if (segment != 0) {
			segment = SEGMENT_SIZE;
		}
		else {
			segment = send_length;
		}
		Py_BEGIN_ALLOW_THREADS
		timeout = internal_select(s, 1);
		if (!timeout)
			n = send(s->sock_fd, buf, segment, flags);
		Py_END_ALLOW_THREADS
		if (timeout) {
			PyErr_SetString(socket_timeout, "timed out");
			return NULL;
		}
		if (n < 0) {
			return s->errorhandler();
		}
		send_length -= segment;
		buf += segment;
	} /* end while */
#endif /* !__VMS */
	return PyInt_FromLong((long)n);
}

PyDoc_STRVAR(send_doc,
"send(data[, flags]) -> count\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.  Return the number of bytes\n\
sent; this may be less than len(data) if the network is busy.");


/* s.sendall(data [,flags]) method */

static PyObject *
sock_sendall(PySocketSockObject *s, PyObject *args)
{
	char *buf;
	int len, n = 0, flags = 0, timeout;

	if (!PyArg_ParseTuple(args, "s#|i:sendall", &buf, &len, &flags))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	do {
		timeout = internal_select(s, 1);
		if (timeout)
			break;
		n = send(s->sock_fd, buf, len, flags);
		if (n < 0)
			break;
		buf += n;
		len -= n;
	} while (len > 0);
	Py_END_ALLOW_THREADS

	if (timeout) {
		PyErr_SetString(socket_timeout, "timed out");
		return NULL;
	}
	if (n < 0)
		return s->errorhandler();

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(sendall_doc,
"sendall(data[, flags])\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.  This calls send() repeatedly\n\
until all data is sent.  If an error occurs, it's impossible\n\
to tell how much data has been sent.");


/* s.sendto(data, [flags,] sockaddr) method */

static PyObject *
sock_sendto(PySocketSockObject *s, PyObject *args)
{
	PyObject *addro;
	char *buf;
	struct sockaddr *addr;
	int addrlen, len, n = 0, flags, timeout;

	flags = 0;
	if (!PyArg_ParseTuple(args, "s#O:sendto", &buf, &len, &addro)) {
		PyErr_Clear();
		if (!PyArg_ParseTuple(args, "s#iO:sendto",
				      &buf, &len, &flags, &addro))
			return NULL;
	}

	if (!getsockaddrarg(s, addro, &addr, &addrlen))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	timeout = internal_select(s, 1);
	if (!timeout)
		n = sendto(s->sock_fd, buf, len, flags, addr, addrlen);
	Py_END_ALLOW_THREADS

	if (timeout) {
		PyErr_SetString(socket_timeout, "timed out");
		return NULL;
	}
	if (n < 0)
		return s->errorhandler();
	return PyInt_FromLong((long)n);
}

PyDoc_STRVAR(sendto_doc,
"sendto(data[, flags], address) -> count\n\
\n\
Like send(data, flags) but allows specifying the destination address.\n\
For IP sockets, the address is a pair (hostaddr, port).");


/* s.shutdown(how) method */

static PyObject *
sock_shutdown(PySocketSockObject *s, PyObject *arg)
{
	int how;
	int res;

	how = PyInt_AsLong(arg);
	if (how == -1 && PyErr_Occurred())
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = shutdown(s->sock_fd, how);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return s->errorhandler();
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(shutdown_doc,
"shutdown(flag)\n\
\n\
Shut down the reading side of the socket (flag == SHUT_RD), the writing side\n\
of the socket (flag == SHUT_WR), or both ends (flag == SHUT_RDWR).");


/* List of methods for socket objects */

static PyMethodDef sock_methods[] = {
	{"accept",	(PyCFunction)sock_accept, METH_NOARGS,
			accept_doc},
	{"bind",	(PyCFunction)sock_bind, METH_O,
			bind_doc},
	{"close",	(PyCFunction)sock_close, METH_NOARGS,
			close_doc},
	{"connect",	(PyCFunction)sock_connect, METH_O,
			connect_doc},
	{"connect_ex",	(PyCFunction)sock_connect_ex, METH_O,
			connect_ex_doc},
#ifndef NO_DUP
	{"dup",		(PyCFunction)sock_dup, METH_NOARGS,
			dup_doc},
#endif
	{"fileno",	(PyCFunction)sock_fileno, METH_NOARGS,
			fileno_doc},
#ifdef HAVE_GETPEERNAME
	{"getpeername",	(PyCFunction)sock_getpeername,
			METH_NOARGS, getpeername_doc},
#endif
	{"getsockname",	(PyCFunction)sock_getsockname,
			METH_NOARGS, getsockname_doc},
	{"getsockopt",	(PyCFunction)sock_getsockopt, METH_VARARGS,
			getsockopt_doc},
	{"listen",	(PyCFunction)sock_listen, METH_O,
			listen_doc},
#ifndef NO_DUP
	{"makefile",	(PyCFunction)sock_makefile, METH_VARARGS,
			makefile_doc},
#endif
	{"recv",	(PyCFunction)sock_recv, METH_VARARGS,
			recv_doc},
	{"recvfrom",	(PyCFunction)sock_recvfrom, METH_VARARGS,
			recvfrom_doc},
	{"send",	(PyCFunction)sock_send, METH_VARARGS,
			send_doc},
	{"sendall",	(PyCFunction)sock_sendall, METH_VARARGS,
			sendall_doc},
	{"sendto",	(PyCFunction)sock_sendto, METH_VARARGS,
			sendto_doc},
	{"setblocking",	(PyCFunction)sock_setblocking, METH_O,
			setblocking_doc},
	{"settimeout", (PyCFunction)sock_settimeout, METH_O,
			settimeout_doc},
	{"gettimeout", (PyCFunction)sock_gettimeout, METH_NOARGS,
			gettimeout_doc},
	{"setsockopt",	(PyCFunction)sock_setsockopt, METH_VARARGS,
			setsockopt_doc},
	{"shutdown",	(PyCFunction)sock_shutdown, METH_O,
			shutdown_doc},
#ifdef RISCOS
	{"sleeptaskw",	(PyCFunction)sock_sleeptaskw, METH_O,
	 		sleeptaskw_doc},
#endif
	{NULL,			NULL}		/* sentinel */
};


/* Deallocate a socket object in response to the last Py_DECREF().
   First close the file description. */

static void
sock_dealloc(PySocketSockObject *s)
{
	if (s->sock_fd != -1)
		(void) SOCKETCLOSE(s->sock_fd);
	s->ob_type->tp_free((PyObject *)s);
}


static PyObject *
sock_repr(PySocketSockObject *s)
{
	char buf[512];
#if SIZEOF_SOCKET_T > SIZEOF_LONG
	if (s->sock_fd > LONG_MAX) {
		/* this can occur on Win64, and actually there is a special
		   ugly printf formatter for decimal pointer length integer
		   printing, only bother if necessary*/
		PyErr_SetString(PyExc_OverflowError,
				"no printf formatter to display "
				"the socket descriptor in decimal");
		return NULL;
	}
#endif
	PyOS_snprintf(
		buf, sizeof(buf),
		"<socket object, fd=%ld, family=%d, type=%d, protocol=%d>",
		(long)s->sock_fd, s->sock_family,
		s->sock_type,
		s->sock_proto);
	return PyString_FromString(buf);
}


/* Create a new, uninitialized socket object. */

static PyObject *
sock_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *new;

	new = type->tp_alloc(type, 0);
	if (new != NULL) {
		((PySocketSockObject *)new)->sock_fd = -1;
		((PySocketSockObject *)new)->sock_timeout = -1.0;
		((PySocketSockObject *)new)->errorhandler = &set_error;
	}
	return new;
}


/* Initialize a new socket object. */

/*ARGSUSED*/
static int
sock_initobj(PyObject *self, PyObject *args, PyObject *kwds)
{
	PySocketSockObject *s = (PySocketSockObject *)self;
	SOCKET_T fd;
	int family = AF_INET, type = SOCK_STREAM, proto = 0;
	static char *keywords[] = {"family", "type", "proto", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
					 "|iii:socket", keywords,
					 &family, &type, &proto))
		return -1;

	Py_BEGIN_ALLOW_THREADS
	fd = socket(family, type, proto);
	Py_END_ALLOW_THREADS

#ifdef MS_WINDOWS
	if (fd == INVALID_SOCKET)
#else
	if (fd < 0)
#endif
	{
		set_error();
		return -1;
	}
	init_sockobject(s, fd, family, type, proto);

	return 0;

}


/* Type object for socket objects. */

static PyTypeObject sock_type = {
	PyObject_HEAD_INIT(0)	/* Must fill in type value later */
	0,					/* ob_size */
	"_socket.socket",			/* tp_name */
	sizeof(PySocketSockObject),		/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)sock_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)sock_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	sock_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	sock_methods,				/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	sock_initobj,				/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	sock_new,				/* tp_new */
	PyObject_Del,				/* tp_free */
};


/* Python interface to gethostname(). */

/*ARGSUSED*/
static PyObject *
socket_gethostname(PyObject *self, PyObject *args)
{
	char buf[1024];
	int res;
	if (!PyArg_ParseTuple(args, ":gethostname"))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = gethostname(buf, (int) sizeof buf - 1);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return set_error();
	buf[sizeof buf - 1] = '\0';
	return PyString_FromString(buf);
}

PyDoc_STRVAR(gethostname_doc,
"gethostname() -> string\n\
\n\
Return the current host name.");


/* Python interface to gethostbyname(name). */

/*ARGSUSED*/
static PyObject *
socket_gethostbyname(PyObject *self, PyObject *args)
{
	char *name;
#ifdef ENABLE_IPV6
	struct sockaddr_storage addrbuf;
#else
        struct sockaddr_in addrbuf;
#endif

	if (!PyArg_ParseTuple(args, "s:gethostbyname", &name))
		return NULL;
	if (setipaddr(name, (struct sockaddr *)&addrbuf,  sizeof(addrbuf), AF_INET) < 0)
		return NULL;
	return makeipaddr((struct sockaddr *)&addrbuf,
		sizeof(struct sockaddr_in));
}

PyDoc_STRVAR(gethostbyname_doc,
"gethostbyname(host) -> address\n\
\n\
Return the IP address (a string of the form '255.255.255.255') for a host.");


/* Convenience function common to gethostbyname_ex and gethostbyaddr */

static PyObject *
gethost_common(struct hostent *h, struct sockaddr *addr, int alen, int af)
{
	char **pch;
	PyObject *rtn_tuple = (PyObject *)NULL;
	PyObject *name_list = (PyObject *)NULL;
	PyObject *addr_list = (PyObject *)NULL;
	PyObject *tmp;

	if (h == NULL) {
		/* Let's get real error message to return */
#ifndef RISCOS
		set_herror(h_errno);
#else
		PyErr_SetString(socket_error, "host not found");
#endif
		return NULL;
	}

	if (h->h_addrtype != af) {
#ifdef HAVE_STRERROR
		/* Let's get real error message to return */
		PyErr_SetString(socket_error,
				(char *)strerror(EAFNOSUPPORT));
#else
		PyErr_SetString(
			socket_error,
			"Address family not supported by protocol family");
#endif
		return NULL;
	}

	switch (af) {

	case AF_INET:
		if (alen < sizeof(struct sockaddr_in))
			return NULL;
		break;

#ifdef ENABLE_IPV6
	case AF_INET6:
		if (alen < sizeof(struct sockaddr_in6))
			return NULL;
		break;
#endif

	}

	if ((name_list = PyList_New(0)) == NULL)
		goto err;

	if ((addr_list = PyList_New(0)) == NULL)
		goto err;

	for (pch = h->h_aliases; *pch != NULL; pch++) {
		int status;
		tmp = PyString_FromString(*pch);
		if (tmp == NULL)
			goto err;

		status = PyList_Append(name_list, tmp);
		Py_DECREF(tmp);

		if (status)
			goto err;
	}

	for (pch = h->h_addr_list; *pch != NULL; pch++) {
		int status;

		switch (af) {

		case AF_INET:
		    {
			struct sockaddr_in sin;
			memset(&sin, 0, sizeof(sin));
			sin.sin_family = af;
#ifdef HAVE_SOCKADDR_SA_LEN
			sin.sin_len = sizeof(sin);
#endif
			memcpy(&sin.sin_addr, *pch, sizeof(sin.sin_addr));
			tmp = makeipaddr((struct sockaddr *)&sin, sizeof(sin));

			if (pch == h->h_addr_list && alen >= sizeof(sin))
				memcpy((char *) addr, &sin, sizeof(sin));
			break;
		    }

#ifdef ENABLE_IPV6
		case AF_INET6:
		    {
			struct sockaddr_in6 sin6;
			memset(&sin6, 0, sizeof(sin6));
			sin6.sin6_family = af;
#ifdef HAVE_SOCKADDR_SA_LEN
			sin6.sin6_len = sizeof(sin6);
#endif
			memcpy(&sin6.sin6_addr, *pch, sizeof(sin6.sin6_addr));
			tmp = makeipaddr((struct sockaddr *)&sin6,
				sizeof(sin6));

			if (pch == h->h_addr_list && alen >= sizeof(sin6))
				memcpy((char *) addr, &sin6, sizeof(sin6));
			break;
		    }
#endif

		default:	/* can't happen */
			PyErr_SetString(socket_error,
					"unsupported address family");
			return NULL;
		}

		if (tmp == NULL)
			goto err;

		status = PyList_Append(addr_list, tmp);
		Py_DECREF(tmp);

		if (status)
			goto err;
	}

	rtn_tuple = Py_BuildValue("sOO", h->h_name, name_list, addr_list);

 err:
	Py_XDECREF(name_list);
	Py_XDECREF(addr_list);
	return rtn_tuple;
}


/* Python interface to gethostbyname_ex(name). */

/*ARGSUSED*/
static PyObject *
socket_gethostbyname_ex(PyObject *self, PyObject *args)
{
	char *name;
	struct hostent *h;
#ifdef ENABLE_IPV6
        struct sockaddr_storage addr;
#else
        struct sockaddr_in addr;
#endif
	struct sockaddr *sa;
	PyObject *ret;
#ifdef HAVE_GETHOSTBYNAME_R
	struct hostent hp_allocated;
#ifdef HAVE_GETHOSTBYNAME_R_3_ARG
	struct hostent_data data;
#else
	char buf[16384];
	int buf_len = (sizeof buf) - 1;
	int errnop;
#endif
#if defined(HAVE_GETHOSTBYNAME_R_3_ARG) || defined(HAVE_GETHOSTBYNAME_R_6_ARG)
	int result;
#endif
#endif /* HAVE_GETHOSTBYNAME_R */

	if (!PyArg_ParseTuple(args, "s:gethostbyname_ex", &name))
		return NULL;
	if (setipaddr(name, (struct sockaddr *)&addr, sizeof(addr), AF_INET) < 0)
		return NULL;
	Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_GETHOSTBYNAME_R
#if   defined(HAVE_GETHOSTBYNAME_R_6_ARG)
	result = gethostbyname_r(name, &hp_allocated, buf, buf_len,
				 &h, &errnop);
#elif defined(HAVE_GETHOSTBYNAME_R_5_ARG)
	h = gethostbyname_r(name, &hp_allocated, buf, buf_len, &errnop);
#else /* HAVE_GETHOSTBYNAME_R_3_ARG */
	memset((void *) &data, '\0', sizeof(data));
	result = gethostbyname_r(name, &hp_allocated, &data);
	h = (result != 0) ? NULL : &hp_allocated;
#endif
#else /* not HAVE_GETHOSTBYNAME_R */
#ifdef USE_GETHOSTBYNAME_LOCK
	PyThread_acquire_lock(netdb_lock, 1);
#endif
	h = gethostbyname(name);
#endif /* HAVE_GETHOSTBYNAME_R */
	Py_END_ALLOW_THREADS
	/* Some C libraries would require addr.__ss_family instead of
	   addr.ss_family.
	   Therefore, we cast the sockaddr_storage into sockaddr to
	   access sa_family. */
	sa = (struct sockaddr*)&addr;
	ret = gethost_common(h, (struct sockaddr *)&addr, sizeof(addr),
			     sa->sa_family);
#ifdef USE_GETHOSTBYNAME_LOCK
	PyThread_release_lock(netdb_lock);
#endif
	return ret;
}

PyDoc_STRVAR(ghbn_ex_doc,
"gethostbyname_ex(host) -> (name, aliaslist, addresslist)\n\
\n\
Return the true host name, a list of aliases, and a list of IP addresses,\n\
for a host.  The host argument is a string giving a host name or IP number.");


/* Python interface to gethostbyaddr(IP). */

/*ARGSUSED*/
static PyObject *
socket_gethostbyaddr(PyObject *self, PyObject *args)
{
#ifdef ENABLE_IPV6
	struct sockaddr_storage addr;
#else
	struct sockaddr_in addr;
#endif
	struct sockaddr *sa = (struct sockaddr *)&addr;
	char *ip_num;
	struct hostent *h;
	PyObject *ret;
#ifdef HAVE_GETHOSTBYNAME_R
	struct hostent hp_allocated;
#ifdef HAVE_GETHOSTBYNAME_R_3_ARG
	struct hostent_data data;
#else
	char buf[16384];
	int buf_len = (sizeof buf) - 1;
	int errnop;
#endif
#if defined(HAVE_GETHOSTBYNAME_R_3_ARG) || defined(HAVE_GETHOSTBYNAME_R_6_ARG)
	int result;
#endif
#endif /* HAVE_GETHOSTBYNAME_R */
	char *ap;
	int al;
	int af;

	if (!PyArg_ParseTuple(args, "s:gethostbyaddr", &ip_num))
		return NULL;
	af = AF_UNSPEC;
	if (setipaddr(ip_num, sa, sizeof(addr), af) < 0)
		return NULL;
	af = sa->sa_family;
	ap = NULL;
	al = 0;
	switch (af) {
	case AF_INET:
		ap = (char *)&((struct sockaddr_in *)sa)->sin_addr;
		al = sizeof(((struct sockaddr_in *)sa)->sin_addr);
		break;
#ifdef ENABLE_IPV6
	case AF_INET6:
		ap = (char *)&((struct sockaddr_in6 *)sa)->sin6_addr;
		al = sizeof(((struct sockaddr_in6 *)sa)->sin6_addr);
		break;
#endif
	default:
		PyErr_SetString(socket_error, "unsupported address family");
		return NULL;
	}
	Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_GETHOSTBYNAME_R
#if   defined(HAVE_GETHOSTBYNAME_R_6_ARG)
	result = gethostbyaddr_r(ap, al, af,
		&hp_allocated, buf, buf_len,
		&h, &errnop);
#elif defined(HAVE_GETHOSTBYNAME_R_5_ARG)
	h = gethostbyaddr_r(ap, al, af,
			    &hp_allocated, buf, buf_len, &errnop);
#else /* HAVE_GETHOSTBYNAME_R_3_ARG */
	memset((void *) &data, '\0', sizeof(data));
	result = gethostbyaddr_r(ap, al, af, &hp_allocated, &data);
	h = (result != 0) ? NULL : &hp_allocated;
#endif
#else /* not HAVE_GETHOSTBYNAME_R */
#ifdef USE_GETHOSTBYNAME_LOCK
	PyThread_acquire_lock(netdb_lock, 1);
#endif
	h = gethostbyaddr(ap, al, af);
#endif /* HAVE_GETHOSTBYNAME_R */
	Py_END_ALLOW_THREADS
	ret = gethost_common(h, (struct sockaddr *)&addr, sizeof(addr), af);
#ifdef USE_GETHOSTBYNAME_LOCK
	PyThread_release_lock(netdb_lock);
#endif
	return ret;
}

PyDoc_STRVAR(gethostbyaddr_doc,
"gethostbyaddr(host) -> (name, aliaslist, addresslist)\n\
\n\
Return the true host name, a list of aliases, and a list of IP addresses,\n\
for a host.  The host argument is a string giving a host name or IP number.");


/* Python interface to getservbyname(name).
   This only returns the port number, since the other info is already
   known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
socket_getservbyname(PyObject *self, PyObject *args)
{
	char *name, *proto=NULL;
	struct servent *sp;
	if (!PyArg_ParseTuple(args, "s|s:getservbyname", &name, &proto))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	sp = getservbyname(name, proto);
	Py_END_ALLOW_THREADS
	if (sp == NULL) {
		PyErr_SetString(socket_error, "service/proto not found");
		return NULL;
	}
	return PyInt_FromLong((long) ntohs(sp->s_port));
}

PyDoc_STRVAR(getservbyname_doc,
"getservbyname(servicename[, protocolname]) -> integer\n\
\n\
Return a port number from a service name and protocol name.\n\
The optional protocol name, if given, should be 'tcp' or 'udp',\n\
otherwise any protocol will match.");


/* Python interface to getservbyport(port).
   This only returns the service name, since the other info is already
   known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
socket_getservbyport(PyObject *self, PyObject *args)
{
	unsigned short port;
	char *proto=NULL;
	struct servent *sp;
	if (!PyArg_ParseTuple(args, "H|s:getservbyport", &port, &proto))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	sp = getservbyport(htons(port), proto);
	Py_END_ALLOW_THREADS
	if (sp == NULL) {
		PyErr_SetString(socket_error, "port/proto not found");
		return NULL;
	}
	return PyString_FromString(sp->s_name);
}

PyDoc_STRVAR(getservbyport_doc,
"getservbyport(port[, protocolname]) -> string\n\
\n\
Return the service name from a port number and protocol name.\n\
The optional protocol name, if given, should be 'tcp' or 'udp',\n\
otherwise any protocol will match.");

/* Python interface to getprotobyname(name).
   This only returns the protocol number, since the other info is
   already known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
socket_getprotobyname(PyObject *self, PyObject *args)
{
	char *name;
	struct protoent *sp;
#ifdef __BEOS__
/* Not available in BeOS yet. - [cjh] */
	PyErr_SetString(socket_error, "getprotobyname not supported");
	return NULL;
#else
	if (!PyArg_ParseTuple(args, "s:getprotobyname", &name))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	sp = getprotobyname(name);
	Py_END_ALLOW_THREADS
	if (sp == NULL) {
		PyErr_SetString(socket_error, "protocol not found");
		return NULL;
	}
	return PyInt_FromLong((long) sp->p_proto);
#endif
}

PyDoc_STRVAR(getprotobyname_doc,
"getprotobyname(name) -> integer\n\
\n\
Return the protocol number for the named protocol.  (Rarely used.)");


#ifdef HAVE_SOCKETPAIR
/* Create a pair of sockets using the socketpair() function.
   Arguments as for socket() except the default family is AF_UNIX if
   defined on the platform; otherwise, the default is AF_INET. */

/*ARGSUSED*/
static PyObject *
socket_socketpair(PyObject *self, PyObject *args)
{
	PySocketSockObject *s0 = NULL, *s1 = NULL;
	SOCKET_T sv[2];
	int family, type = SOCK_STREAM, proto = 0;
	PyObject *res = NULL;

#if defined(AF_UNIX)
	family = AF_UNIX;
#else
	family = AF_INET;
#endif
	if (!PyArg_ParseTuple(args, "|iii:socketpair",
			      &family, &type, &proto))
		return NULL;
	/* Create a pair of socket fds */
	if (socketpair(family, type, proto, sv) < 0)
		return set_error();
	s0 = new_sockobject(sv[0], family, type, proto);
	if (s0 == NULL)
		goto finally;
	s1 = new_sockobject(sv[1], family, type, proto);
	if (s1 == NULL)
		goto finally;
	res = PyTuple_Pack(2, s0, s1);

finally:
	if (res == NULL) {
		if (s0 == NULL)
			SOCKETCLOSE(sv[0]);
		if (s1 == NULL)
			SOCKETCLOSE(sv[1]);
	}
	Py_XDECREF(s0);
	Py_XDECREF(s1);
	return res;
}

PyDoc_STRVAR(socketpair_doc,
"socketpair([family[, type[, proto]]]) -> (socket object, socket object)\n\
\n\
Create a pair of socket objects from the sockets returned by the platform\n\
socketpair() function.\n\
The arguments are the same as for socket() except the default family is\n\
AF_UNIX if defined on the platform; otherwise, the default is AF_INET.");

#endif /* HAVE_SOCKETPAIR */


#ifndef NO_DUP
/* Create a socket object from a numeric file description.
   Useful e.g. if stdin is a socket.
   Additional arguments as for socket(). */

/*ARGSUSED*/
static PyObject *
socket_fromfd(PyObject *self, PyObject *args)
{
	PySocketSockObject *s;
	SOCKET_T fd;
	int family, type, proto = 0;
	if (!PyArg_ParseTuple(args, "iii|i:fromfd",
			      &fd, &family, &type, &proto))
		return NULL;
	/* Dup the fd so it and the socket can be closed independently */
	fd = dup(fd);
	if (fd < 0)
		return set_error();
	s = new_sockobject(fd, family, type, proto);
	return (PyObject *) s;
}

PyDoc_STRVAR(fromfd_doc,
"fromfd(fd, family, type[, proto]) -> socket object\n\
\n\
Create a socket object from the given file descriptor.\n\
The remaining arguments are the same as for socket().");

#endif /* NO_DUP */


static PyObject *
socket_ntohs(PyObject *self, PyObject *args)
{
	int x1, x2;

	if (!PyArg_ParseTuple(args, "i:ntohs", &x1)) {
		return NULL;
	}
	x2 = (int)ntohs((short)x1);
	return PyInt_FromLong(x2);
}

PyDoc_STRVAR(ntohs_doc,
"ntohs(integer) -> integer\n\
\n\
Convert a 16-bit integer from network to host byte order.");


static PyObject *
socket_ntohl(PyObject *self, PyObject *arg)
{
	unsigned long x;

	if (PyInt_Check(arg)) {
		x = PyInt_AS_LONG(arg);
		if (x == (unsigned long) -1 && PyErr_Occurred())
			return NULL;
	}
	else if (PyLong_Check(arg)) {
		x = PyLong_AsUnsignedLong(arg);
		if (x == (unsigned long) -1 && PyErr_Occurred())
			return NULL;
#if SIZEOF_LONG > 4
		{
			unsigned long y;
			/* only want the trailing 32 bits */
			y = x & 0xFFFFFFFFUL;
			if (y ^ x)
				return PyErr_Format(PyExc_OverflowError,
					    "long int larger than 32 bits");
			x = y;
		}
#endif
	}
	else
		return PyErr_Format(PyExc_TypeError,
				    "expected int/long, %s found",
				    arg->ob_type->tp_name);
	if (x == (unsigned long) -1 && PyErr_Occurred())
		return NULL;
	return PyInt_FromLong(ntohl(x));
}

PyDoc_STRVAR(ntohl_doc,
"ntohl(integer) -> integer\n\
\n\
Convert a 32-bit integer from network to host byte order.");


static PyObject *
socket_htons(PyObject *self, PyObject *args)
{
	int x1, x2;

	if (!PyArg_ParseTuple(args, "i:htons", &x1)) {
		return NULL;
	}
	x2 = (int)htons((short)x1);
	return PyInt_FromLong(x2);
}

PyDoc_STRVAR(htons_doc,
"htons(integer) -> integer\n\
\n\
Convert a 16-bit integer from host to network byte order.");


static PyObject *
socket_htonl(PyObject *self, PyObject *arg)
{
	unsigned long x;

	if (PyInt_Check(arg)) {
		x = PyInt_AS_LONG(arg);
		if (x == (unsigned long) -1 && PyErr_Occurred())
			return NULL;
	}
	else if (PyLong_Check(arg)) {
		x = PyLong_AsUnsignedLong(arg);
		if (x == (unsigned long) -1 && PyErr_Occurred())
			return NULL;
#if SIZEOF_LONG > 4
		{
			unsigned long y;
			/* only want the trailing 32 bits */
			y = x & 0xFFFFFFFFUL;
			if (y ^ x)
				return PyErr_Format(PyExc_OverflowError,
					    "long int larger than 32 bits");
			x = y;
		}
#endif
	}
	else
		return PyErr_Format(PyExc_TypeError,
				    "expected int/long, %s found",
				    arg->ob_type->tp_name);
	return PyInt_FromLong(htonl(x));
}

PyDoc_STRVAR(htonl_doc,
"htonl(integer) -> integer\n\
\n\
Convert a 32-bit integer from host to network byte order.");

/* socket.inet_aton() and socket.inet_ntoa() functions. */

PyDoc_STRVAR(inet_aton_doc,
"inet_aton(string) -> packed 32-bit IP representation\n\
\n\
Convert an IP address in string format (123.45.67.89) to the 32-bit packed\n\
binary format used in low-level network functions.");

static PyObject*
socket_inet_aton(PyObject *self, PyObject *args)
{
#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif
#ifdef HAVE_INET_ATON
	struct in_addr buf;
#else
	/* Have to use inet_addr() instead */
	unsigned long packed_addr;
#endif
	char *ip_addr;

	if (!PyArg_ParseTuple(args, "s:inet_aton", &ip_addr))
		return NULL;


#ifdef HAVE_INET_ATON
	if (inet_aton(ip_addr, &buf))
		return PyString_FromStringAndSize((char *)(&buf),
						  sizeof(buf));

	PyErr_SetString(socket_error,
			"illegal IP address string passed to inet_aton");
	return NULL;

#else /* ! HAVE_INET_ATON */
	/* XXX Problem here: inet_aton('255.255.255.255') raises
	   an exception while it should be a valid address. */
	packed_addr = inet_addr(ip_addr);

	if (packed_addr == INADDR_NONE) {	/* invalid address */
		PyErr_SetString(socket_error,
			"illegal IP address string passed to inet_aton");
		return NULL;
	}
	return PyString_FromStringAndSize((char *) &packed_addr,
					  sizeof(packed_addr));
#endif
}

PyDoc_STRVAR(inet_ntoa_doc,
"inet_ntoa(packed_ip) -> ip_address_string\n\
\n\
Convert an IP address from 32-bit packed binary format to string format");

static PyObject*
socket_inet_ntoa(PyObject *self, PyObject *args)
{
	char *packed_str;
	int addr_len;
	struct in_addr packed_addr;

	if (!PyArg_ParseTuple(args, "s#:inet_ntoa", &packed_str, &addr_len)) {
		return NULL;
	}

	if (addr_len != sizeof(packed_addr)) {
		PyErr_SetString(socket_error,
			"packed IP wrong length for inet_ntoa");
		return NULL;
	}

	memcpy(&packed_addr, packed_str, addr_len);

	return PyString_FromString(inet_ntoa(packed_addr));
}

#ifdef HAVE_INET_PTON

PyDoc_STRVAR(inet_pton_doc,
"inet_pton(af, ip) -> packed IP address string\n\
\n\
Convert an IP address from string format to a packed string suitable\n\
for use with low-level network functions.");

static PyObject *
socket_inet_pton(PyObject *self, PyObject *args)
{
	int af;
	char* ip;
	int retval;
#ifdef ENABLE_IPV6
	char packed[MAX(sizeof(struct in_addr), sizeof(struct in6_addr))];
#else
	char packed[sizeof(struct in_addr)];
#endif
	if (!PyArg_ParseTuple(args, "is:inet_pton", &af, &ip)) {
		return NULL;
	}

#if !defined(ENABLE_IPV6) && defined(AF_INET6)
	if(af == AF_INET6) {
		PyErr_SetString(socket_error,
				"can't use AF_INET6, IPv6 is disabled");
		return NULL;
	}
#endif 

	retval = inet_pton(af, ip, packed);
	if (retval < 0) {
		PyErr_SetFromErrno(socket_error);
		return NULL;
	} else if (retval == 0) {
		PyErr_SetString(socket_error,
			"illegal IP address string passed to inet_pton");
		return NULL;
	} else if (af == AF_INET) {
		return PyString_FromStringAndSize(packed,
			sizeof(struct in_addr));
#ifdef ENABLE_IPV6
	} else if (af == AF_INET6) {
		return PyString_FromStringAndSize(packed,
			sizeof(struct in6_addr));
#endif
	} else {
		PyErr_SetString(socket_error, "unknown address family");
		return NULL;
	}
}
	
PyDoc_STRVAR(inet_ntop_doc,
"inet_ntop(af, packed_ip) -> string formatted IP address\n\
\n\
Convert a packed IP address of the given family to string format.");

static PyObject *
socket_inet_ntop(PyObject *self, PyObject *args)
{
	int af;
	char* packed;
	int len;
	const char* retval;
#ifdef ENABLE_IPV6
	char ip[MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN) + 1];
#else
	char ip[INET_ADDRSTRLEN + 1];
#endif
	
	/* Guarantee NUL-termination for PyString_FromString() below */
	memset((void *) &ip[0], '\0', sizeof(ip));

	if (!PyArg_ParseTuple(args, "is#:inet_ntop", &af, &packed, &len)) {
		return NULL;
	}

	if (af == AF_INET) {
		if (len != sizeof(struct in_addr)) {
			PyErr_SetString(PyExc_ValueError,
				"invalid length of packed IP address string");
			return NULL;
		}
#ifdef ENABLE_IPV6
	} else if (af == AF_INET6) {
		if (len != sizeof(struct in6_addr)) {
			PyErr_SetString(PyExc_ValueError,
				"invalid length of packed IP address string");
			return NULL;
		}
#endif
	} else {
		PyErr_Format(PyExc_ValueError,
			"unknown address family %d", af);
		return NULL;
	}

	retval = inet_ntop(af, packed, ip, sizeof(ip));
	if (!retval) {
		PyErr_SetFromErrno(socket_error);
		return NULL;
	} else {
		return PyString_FromString(retval);
	}

	/* NOTREACHED */
	PyErr_SetString(PyExc_RuntimeError, "invalid handling of inet_ntop");
	return NULL;
}

#endif /* HAVE_INET_PTON */

/* Python interface to getaddrinfo(host, port). */

/*ARGSUSED*/
static PyObject *
socket_getaddrinfo(PyObject *self, PyObject *args)
{
	struct addrinfo hints, *res;
	struct addrinfo *res0 = NULL;
	PyObject *hobj = NULL;
	PyObject *pobj = (PyObject *)NULL;
	char pbuf[30];
	char *hptr, *pptr;
	int family, socktype, protocol, flags;
	int error;
	PyObject *all = (PyObject *)NULL;
	PyObject *single = (PyObject *)NULL;
	PyObject *idna = NULL;

	family = socktype = protocol = flags = 0;
	family = AF_UNSPEC;
	if (!PyArg_ParseTuple(args, "OO|iiii:getaddrinfo",
			      &hobj, &pobj, &family, &socktype,
			      &protocol, &flags)) {
		return NULL;
	}
	if (hobj == Py_None) {
		hptr = NULL;
	} else if (PyUnicode_Check(hobj)) {
		idna = PyObject_CallMethod(hobj, "encode", "s", "idna");
		if (!idna)
			return NULL;
		hptr = PyString_AsString(idna);
	} else if (PyString_Check(hobj)) {
		hptr = PyString_AsString(hobj);
	} else {
		PyErr_SetString(PyExc_TypeError, 
				"getaddrinfo() argument 1 must be string or None");
		return NULL;
	}
	if (PyInt_Check(pobj)) {
		PyOS_snprintf(pbuf, sizeof(pbuf), "%ld", PyInt_AsLong(pobj));
		pptr = pbuf;
	} else if (PyString_Check(pobj)) {
		pptr = PyString_AsString(pobj);
	} else if (pobj == Py_None) {
		pptr = (char *)NULL;
	} else {
		PyErr_SetString(socket_error, "Int or String expected");
                goto err;
	}
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = socktype;
	hints.ai_protocol = protocol;
	hints.ai_flags = flags;
	Py_BEGIN_ALLOW_THREADS
	ACQUIRE_GETADDRINFO_LOCK
	error = getaddrinfo(hptr, pptr, &hints, &res0);
	Py_END_ALLOW_THREADS
	RELEASE_GETADDRINFO_LOCK  /* see comment in setipaddr() */
	if (error) {
		set_gaierror(error);
		goto err;
	}

	if ((all = PyList_New(0)) == NULL)
		goto err;
	for (res = res0; res; res = res->ai_next) {
		PyObject *addr =
			makesockaddr(-1, res->ai_addr, res->ai_addrlen, protocol);
		if (addr == NULL)
			goto err;
		single = Py_BuildValue("iiisO", res->ai_family,
			res->ai_socktype, res->ai_protocol,
			res->ai_canonname ? res->ai_canonname : "",
			addr);
		Py_DECREF(addr);
		if (single == NULL)
			goto err;

		if (PyList_Append(all, single))
			goto err;
		Py_XDECREF(single);
	}
	Py_XDECREF(idna);
	if (res0)
		freeaddrinfo(res0);
	return all;
 err:
	Py_XDECREF(single);
	Py_XDECREF(all);
	Py_XDECREF(idna);
	if (res0)
		freeaddrinfo(res0);
	return (PyObject *)NULL;
}

PyDoc_STRVAR(getaddrinfo_doc,
"getaddrinfo(host, port [, family, socktype, proto, flags])\n\
    -> list of (family, socktype, proto, canonname, sockaddr)\n\
\n\
Resolve host and port into addrinfo struct.");

/* Python interface to getnameinfo(sa, flags). */

/*ARGSUSED*/
static PyObject *
socket_getnameinfo(PyObject *self, PyObject *args)
{
	PyObject *sa = (PyObject *)NULL;
	int flags;
	char *hostp;
	int port, flowinfo, scope_id;
	char hbuf[NI_MAXHOST], pbuf[NI_MAXSERV];
	struct addrinfo hints, *res = NULL;
	int error;
	PyObject *ret = (PyObject *)NULL;

	flags = flowinfo = scope_id = 0;
	if (!PyArg_ParseTuple(args, "Oi:getnameinfo", &sa, &flags))
		return NULL;
	if  (!PyArg_ParseTuple(sa, "si|ii",
			       &hostp, &port, &flowinfo, &scope_id))
		return NULL;
	PyOS_snprintf(pbuf, sizeof(pbuf), "%d", port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;	/* make numeric port happy */
	Py_BEGIN_ALLOW_THREADS
	ACQUIRE_GETADDRINFO_LOCK
	error = getaddrinfo(hostp, pbuf, &hints, &res);
	Py_END_ALLOW_THREADS
	RELEASE_GETADDRINFO_LOCK  /* see comment in setipaddr() */
	if (error) {
		set_gaierror(error);
		goto fail;
	}
	if (res->ai_next) {
		PyErr_SetString(socket_error,
			"sockaddr resolved to multiple addresses");
		goto fail;
	}
	switch (res->ai_family) {
	case AF_INET:
	    {
		char *t1;
		int t2;
		if (PyArg_ParseTuple(sa, "si", &t1, &t2) == 0) {
			PyErr_SetString(socket_error,
				"IPv4 sockaddr must be 2 tuple");
			goto fail;
		}
		break;
	    }
#ifdef ENABLE_IPV6
	case AF_INET6:
	    {
		struct sockaddr_in6 *sin6;
		sin6 = (struct sockaddr_in6 *)res->ai_addr;
		sin6->sin6_flowinfo = flowinfo;
		sin6->sin6_scope_id = scope_id;
		break;
	    }
#endif
	}
	error = getnameinfo(res->ai_addr, res->ai_addrlen,
			hbuf, sizeof(hbuf), pbuf, sizeof(pbuf), flags);
	if (error) {
		set_gaierror(error);
		goto fail;
	}
	ret = Py_BuildValue("ss", hbuf, pbuf);

fail:
	if (res)
		freeaddrinfo(res);
	return ret;
}

PyDoc_STRVAR(getnameinfo_doc,
"getnameinfo(sockaddr, flags) --> (host, port)\n\
\n\
Get host and port for a sockaddr.");


/* Python API to getting and setting the default timeout value. */

static PyObject *
socket_getdefaulttimeout(PyObject *self)
{
	if (defaulttimeout < 0.0) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	else
		return PyFloat_FromDouble(defaulttimeout);
}

PyDoc_STRVAR(getdefaulttimeout_doc,
"getdefaulttimeout() -> timeout\n\
\n\
Returns the default timeout in floating seconds for new socket objects.\n\
A value of None indicates that new socket objects have no timeout.\n\
When the socket module is first imported, the default is None.");

static PyObject *
socket_setdefaulttimeout(PyObject *self, PyObject *arg)
{
	double timeout;

	if (arg == Py_None)
		timeout = -1.0;
	else {
		timeout = PyFloat_AsDouble(arg);
		if (timeout < 0.0) {
			if (!PyErr_Occurred())
				PyErr_SetString(PyExc_ValueError,
						"Timeout value out of range");
			return NULL;
		}
	}

	defaulttimeout = timeout;

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(setdefaulttimeout_doc,
"setdefaulttimeout(timeout)\n\
\n\
Set the default timeout in floating seconds for new socket objects.\n\
A value of None indicates that new socket objects have no timeout.\n\
When the socket module is first imported, the default is None.");


/* List of functions exported by this module. */

static PyMethodDef socket_methods[] = {
	{"gethostbyname",	socket_gethostbyname,
	 METH_VARARGS, gethostbyname_doc},
	{"gethostbyname_ex",	socket_gethostbyname_ex,
	 METH_VARARGS, ghbn_ex_doc},
	{"gethostbyaddr",	socket_gethostbyaddr,
	 METH_VARARGS, gethostbyaddr_doc},
	{"gethostname",		socket_gethostname,
	 METH_VARARGS, gethostname_doc},
	{"getservbyname",	socket_getservbyname,
	 METH_VARARGS, getservbyname_doc},
	{"getservbyport",	socket_getservbyport,
	 METH_VARARGS, getservbyport_doc},
	{"getprotobyname",	socket_getprotobyname,
	 METH_VARARGS,getprotobyname_doc},
#ifndef NO_DUP
	{"fromfd",		socket_fromfd,
	 METH_VARARGS, fromfd_doc},
#endif
#ifdef HAVE_SOCKETPAIR
	{"socketpair",		socket_socketpair,
	 METH_VARARGS, socketpair_doc},
#endif
	{"ntohs",		socket_ntohs,
	 METH_VARARGS, ntohs_doc},
	{"ntohl",		socket_ntohl,
	 METH_O, ntohl_doc},
	{"htons",		socket_htons,
	 METH_VARARGS, htons_doc},
	{"htonl",		socket_htonl,
	 METH_O, htonl_doc},
	{"inet_aton",		socket_inet_aton,
	 METH_VARARGS, inet_aton_doc},
	{"inet_ntoa",		socket_inet_ntoa,
	 METH_VARARGS, inet_ntoa_doc},
#ifdef HAVE_INET_PTON
	{"inet_pton",		socket_inet_pton,
	 METH_VARARGS, inet_pton_doc},
	{"inet_ntop",		socket_inet_ntop,
	 METH_VARARGS, inet_ntop_doc},
#endif
	{"getaddrinfo",		socket_getaddrinfo,
	 METH_VARARGS, getaddrinfo_doc},
	{"getnameinfo",		socket_getnameinfo,
	 METH_VARARGS, getnameinfo_doc},
	{"getdefaulttimeout",	(PyCFunction)socket_getdefaulttimeout,
	 METH_NOARGS, getdefaulttimeout_doc},
	{"setdefaulttimeout",	socket_setdefaulttimeout,
	 METH_O, setdefaulttimeout_doc},
	{NULL,			NULL}		 /* Sentinel */
};


#ifdef RISCOS
#define OS_INIT_DEFINED

static int
os_init(void)
{
	_kernel_swi_regs r;

	r.r[0] = 0;
	_kernel_swi(0x43380, &r, &r);
	taskwindow = r.r[0];

	return 1;
}

#endif /* RISCOS */


#ifdef MS_WINDOWS
#define OS_INIT_DEFINED

/* Additional initialization and cleanup for Windows */

static void
os_cleanup(void)
{
	WSACleanup();
}

static int
os_init(void)
{
	WSADATA WSAData;
	int ret;
	char buf[100];
	ret = WSAStartup(0x0101, &WSAData);
	switch (ret) {
	case 0:	/* No error */
		Py_AtExit(os_cleanup);
		return 1; /* Success */
	case WSASYSNOTREADY:
		PyErr_SetString(PyExc_ImportError,
				"WSAStartup failed: network not ready");
		break;
	case WSAVERNOTSUPPORTED:
	case WSAEINVAL:
		PyErr_SetString(
			PyExc_ImportError,
			"WSAStartup failed: requested version not supported");
		break;
	default:
		PyOS_snprintf(buf, sizeof(buf),
			      "WSAStartup failed: error code %d", ret);
		PyErr_SetString(PyExc_ImportError, buf);
		break;
	}
	return 0; /* Failure */
}

#endif /* MS_WINDOWS */


#ifdef PYOS_OS2
#define OS_INIT_DEFINED

/* Additional initialization for OS/2 */

static int
os_init(void)
{
#ifndef PYCC_GCC
	char reason[64];
	int rc = sock_init();

	if (rc == 0) {
		return 1; /* Success */
	}

	PyOS_snprintf(reason, sizeof(reason),
		      "OS/2 TCP/IP Error# %d", sock_errno());
	PyErr_SetString(PyExc_ImportError, reason);

	return 0;  /* Failure */
#else
	/* No need to initialise sockets with GCC/EMX */
	return 1; /* Success */
#endif
}

#endif /* PYOS_OS2 */


#ifndef OS_INIT_DEFINED
static int
os_init(void)
{
	return 1; /* Success */
}
#endif


/* C API table - always add new things to the end for binary
   compatibility. */
static
PySocketModule_APIObject PySocketModuleAPI =
{
	&sock_type,
        NULL
};


/* Initialize the _socket module.

   This module is actually called "_socket", and there's a wrapper
   "socket.py" which implements some additional functionality.  On some
   platforms (e.g. Windows and OS/2), socket.py also implements a
   wrapper for the socket type that provides missing functionality such
   as makefile(), dup() and fromfd().  The import of "_socket" may fail
   with an ImportError exception if os-specific initialization fails.
   On Windows, this does WINSOCK initialization.  When WINSOCK is
   initialized succesfully, a call to WSACleanup() is scheduled to be
   made at exit time.
*/

PyDoc_STRVAR(socket_doc,
"Implementation module for socket operations.\n\
\n\
See the socket module for documentation.");

PyMODINIT_FUNC
init_socket(void)
{
	PyObject *m, *has_ipv6;

	if (!os_init())
		return;

	sock_type.ob_type = &PyType_Type;
	m = Py_InitModule3(PySocket_MODULE_NAME,
			   socket_methods,
			   socket_doc);

	socket_error = PyErr_NewException("socket.error", NULL, NULL);
	if (socket_error == NULL)
		return;
        PySocketModuleAPI.error = socket_error;
	Py_INCREF(socket_error);
	PyModule_AddObject(m, "error", socket_error);
	socket_herror = PyErr_NewException("socket.herror",
					   socket_error, NULL);
	if (socket_herror == NULL)
		return;
	Py_INCREF(socket_herror);
	PyModule_AddObject(m, "herror", socket_herror);
	socket_gaierror = PyErr_NewException("socket.gaierror", socket_error,
	    NULL);
	if (socket_gaierror == NULL)
		return;
	Py_INCREF(socket_gaierror);
	PyModule_AddObject(m, "gaierror", socket_gaierror);
	socket_timeout = PyErr_NewException("socket.timeout",
					    socket_error, NULL);
	if (socket_timeout == NULL)
		return;
	Py_INCREF(socket_timeout);
	PyModule_AddObject(m, "timeout", socket_timeout);
	Py_INCREF((PyObject *)&sock_type);
	if (PyModule_AddObject(m, "SocketType",
			       (PyObject *)&sock_type) != 0)
		return;
	Py_INCREF((PyObject *)&sock_type);
	if (PyModule_AddObject(m, "socket",
			       (PyObject *)&sock_type) != 0)
		return;

#ifdef ENABLE_IPV6
	has_ipv6 = Py_True;
#else
	has_ipv6 = Py_False;
#endif
	Py_INCREF(has_ipv6);
	PyModule_AddObject(m, "has_ipv6", has_ipv6);

	/* Export C API */
	if (PyModule_AddObject(m, PySocket_CAPI_NAME,
	       PyCObject_FromVoidPtr((void *)&PySocketModuleAPI, NULL)
				 ) != 0)
		return;

	/* Address families (we only support AF_INET and AF_UNIX) */
#ifdef AF_UNSPEC
	PyModule_AddIntConstant(m, "AF_UNSPEC", AF_UNSPEC);
#endif
	PyModule_AddIntConstant(m, "AF_INET", AF_INET);
#ifdef AF_INET6
	PyModule_AddIntConstant(m, "AF_INET6", AF_INET6);
#endif /* AF_INET6 */
#if defined(AF_UNIX)
	PyModule_AddIntConstant(m, "AF_UNIX", AF_UNIX);
#endif /* AF_UNIX */
#ifdef AF_AX25
	/* Amateur Radio AX.25 */
	PyModule_AddIntConstant(m, "AF_AX25", AF_AX25);
#endif
#ifdef AF_IPX
	PyModule_AddIntConstant(m, "AF_IPX", AF_IPX); /* Novell IPX */
#endif
#ifdef AF_APPLETALK
	/* Appletalk DDP */
	PyModule_AddIntConstant(m, "AF_APPLETALK", AF_APPLETALK);
#endif
#ifdef AF_NETROM
	/* Amateur radio NetROM */
	PyModule_AddIntConstant(m, "AF_NETROM", AF_NETROM);
#endif
#ifdef AF_BRIDGE
	/* Multiprotocol bridge */
	PyModule_AddIntConstant(m, "AF_BRIDGE", AF_BRIDGE);
#endif
#ifdef AF_ATMPVC
	/* ATM PVCs */
	PyModule_AddIntConstant(m, "AF_ATMPVC", AF_ATMPVC);
#endif
#ifdef AF_AAL5
	/* Reserved for Werner's ATM */
	PyModule_AddIntConstant(m, "AF_AAL5", AF_AAL5);
#endif
#ifdef AF_X25
	/* Reserved for X.25 project */
	PyModule_AddIntConstant(m, "AF_X25", AF_X25);
#endif
#ifdef AF_INET6
	PyModule_AddIntConstant(m, "AF_INET6", AF_INET6); /* IP version 6 */
#endif
#ifdef AF_ROSE
	/* Amateur Radio X.25 PLP */
	PyModule_AddIntConstant(m, "AF_ROSE", AF_ROSE);
#endif
#ifdef AF_DECnet
	/* Reserved for DECnet project */
	PyModule_AddIntConstant(m, "AF_DECnet", AF_DECnet);
#endif
#ifdef AF_NETBEUI
	/* Reserved for 802.2LLC project */
	PyModule_AddIntConstant(m, "AF_NETBEUI", AF_NETBEUI);
#endif
#ifdef AF_SECURITY
	/* Security callback pseudo AF */
	PyModule_AddIntConstant(m, "AF_SECURITY", AF_SECURITY);
#endif
#ifdef AF_KEY
	/* PF_KEY key management API */
	PyModule_AddIntConstant(m, "AF_KEY", AF_KEY);
#endif
#ifdef AF_NETLINK
	/*  */
	PyModule_AddIntConstant(m, "AF_NETLINK", AF_NETLINK);
#endif
#ifdef AF_ROUTE
	/* Alias to emulate 4.4BSD */
	PyModule_AddIntConstant(m, "AF_ROUTE", AF_ROUTE);
#endif
#ifdef AF_ASH
	/* Ash */
	PyModule_AddIntConstant(m, "AF_ASH", AF_ASH);
#endif
#ifdef AF_ECONET
	/* Acorn Econet */
	PyModule_AddIntConstant(m, "AF_ECONET", AF_ECONET);
#endif
#ifdef AF_ATMSVC
	/* ATM SVCs */
	PyModule_AddIntConstant(m, "AF_ATMSVC", AF_ATMSVC);
#endif
#ifdef AF_SNA
	/* Linux SNA Project (nutters!) */
	PyModule_AddIntConstant(m, "AF_SNA", AF_SNA);
#endif
#ifdef AF_IRDA
	/* IRDA sockets */
	PyModule_AddIntConstant(m, "AF_IRDA", AF_IRDA);
#endif
#ifdef AF_PPPOX
	/* PPPoX sockets */
	PyModule_AddIntConstant(m, "AF_PPPOX", AF_PPPOX);
#endif
#ifdef AF_WANPIPE
	/* Wanpipe API Sockets */
	PyModule_AddIntConstant(m, "AF_WANPIPE", AF_WANPIPE);
#endif
#ifdef AF_LLC
	/* Linux LLC */
	PyModule_AddIntConstant(m, "AF_LLC", AF_LLC);
#endif

#ifdef USE_BLUETOOTH
	PyModule_AddIntConstant(m, "AF_BLUETOOTH", AF_BLUETOOTH);
	PyModule_AddIntConstant(m, "BTPROTO_L2CAP", BTPROTO_L2CAP);
#if !defined(__FreeBSD__)
	PyModule_AddIntConstant(m, "BTPROTO_SCO", BTPROTO_SCO);
#endif
	PyModule_AddIntConstant(m, "BTPROTO_RFCOMM", BTPROTO_RFCOMM);
	PyModule_AddObject(m, "BDADDR_ANY", Py_BuildValue("s", "00:00:00:00:00:00"));
	PyModule_AddObject(m, "BDADDR_LOCAL", Py_BuildValue("s", "00:00:00:FF:FF:FF"));
#endif

#ifdef HAVE_NETPACKET_PACKET_H
	PyModule_AddIntConstant(m, "AF_PACKET", AF_PACKET);
	PyModule_AddIntConstant(m, "PF_PACKET", PF_PACKET);
	PyModule_AddIntConstant(m, "PACKET_HOST", PACKET_HOST);
	PyModule_AddIntConstant(m, "PACKET_BROADCAST", PACKET_BROADCAST);
	PyModule_AddIntConstant(m, "PACKET_MULTICAST", PACKET_MULTICAST);
	PyModule_AddIntConstant(m, "PACKET_OTHERHOST", PACKET_OTHERHOST);
	PyModule_AddIntConstant(m, "PACKET_OUTGOING", PACKET_OUTGOING);
	PyModule_AddIntConstant(m, "PACKET_LOOPBACK", PACKET_LOOPBACK);
	PyModule_AddIntConstant(m, "PACKET_FASTROUTE", PACKET_FASTROUTE);
#endif

	/* Socket types */
	PyModule_AddIntConstant(m, "SOCK_STREAM", SOCK_STREAM);
	PyModule_AddIntConstant(m, "SOCK_DGRAM", SOCK_DGRAM);
#ifndef __BEOS__
/* We have incomplete socket support. */
	PyModule_AddIntConstant(m, "SOCK_RAW", SOCK_RAW);
	PyModule_AddIntConstant(m, "SOCK_SEQPACKET", SOCK_SEQPACKET);
#if defined(SOCK_RDM)
	PyModule_AddIntConstant(m, "SOCK_RDM", SOCK_RDM);
#endif
#endif

#ifdef	SO_DEBUG
	PyModule_AddIntConstant(m, "SO_DEBUG", SO_DEBUG);
#endif
#ifdef	SO_ACCEPTCONN
	PyModule_AddIntConstant(m, "SO_ACCEPTCONN", SO_ACCEPTCONN);
#endif
#ifdef	SO_REUSEADDR
	PyModule_AddIntConstant(m, "SO_REUSEADDR", SO_REUSEADDR);
#endif
#ifdef SO_EXCLUSIVEADDRUSE
	PyModule_AddIntConstant(m, "SO_EXCLUSIVEADDRUSE", SO_EXCLUSIVEADDRUSE);
#endif

#ifdef	SO_KEEPALIVE
	PyModule_AddIntConstant(m, "SO_KEEPALIVE", SO_KEEPALIVE);
#endif
#ifdef	SO_DONTROUTE
	PyModule_AddIntConstant(m, "SO_DONTROUTE", SO_DONTROUTE);
#endif
#ifdef	SO_BROADCAST
	PyModule_AddIntConstant(m, "SO_BROADCAST", SO_BROADCAST);
#endif
#ifdef	SO_USELOOPBACK
	PyModule_AddIntConstant(m, "SO_USELOOPBACK", SO_USELOOPBACK);
#endif
#ifdef	SO_LINGER
	PyModule_AddIntConstant(m, "SO_LINGER", SO_LINGER);
#endif
#ifdef	SO_OOBINLINE
	PyModule_AddIntConstant(m, "SO_OOBINLINE", SO_OOBINLINE);
#endif
#ifdef	SO_REUSEPORT
	PyModule_AddIntConstant(m, "SO_REUSEPORT", SO_REUSEPORT);
#endif
#ifdef	SO_SNDBUF
	PyModule_AddIntConstant(m, "SO_SNDBUF", SO_SNDBUF);
#endif
#ifdef	SO_RCVBUF
	PyModule_AddIntConstant(m, "SO_RCVBUF", SO_RCVBUF);
#endif
#ifdef	SO_SNDLOWAT
	PyModule_AddIntConstant(m, "SO_SNDLOWAT", SO_SNDLOWAT);
#endif
#ifdef	SO_RCVLOWAT
	PyModule_AddIntConstant(m, "SO_RCVLOWAT", SO_RCVLOWAT);
#endif
#ifdef	SO_SNDTIMEO
	PyModule_AddIntConstant(m, "SO_SNDTIMEO", SO_SNDTIMEO);
#endif
#ifdef	SO_RCVTIMEO
	PyModule_AddIntConstant(m, "SO_RCVTIMEO", SO_RCVTIMEO);
#endif
#ifdef	SO_ERROR
	PyModule_AddIntConstant(m, "SO_ERROR", SO_ERROR);
#endif
#ifdef	SO_TYPE
	PyModule_AddIntConstant(m, "SO_TYPE", SO_TYPE);
#endif

	/* Maximum number of connections for "listen" */
#ifdef	SOMAXCONN
	PyModule_AddIntConstant(m, "SOMAXCONN", SOMAXCONN);
#else
	PyModule_AddIntConstant(m, "SOMAXCONN", 5); /* Common value */
#endif

	/* Flags for send, recv */
#ifdef	MSG_OOB
	PyModule_AddIntConstant(m, "MSG_OOB", MSG_OOB);
#endif
#ifdef	MSG_PEEK
	PyModule_AddIntConstant(m, "MSG_PEEK", MSG_PEEK);
#endif
#ifdef	MSG_DONTROUTE
	PyModule_AddIntConstant(m, "MSG_DONTROUTE", MSG_DONTROUTE);
#endif
#ifdef	MSG_DONTWAIT
	PyModule_AddIntConstant(m, "MSG_DONTWAIT", MSG_DONTWAIT);
#endif
#ifdef	MSG_EOR
	PyModule_AddIntConstant(m, "MSG_EOR", MSG_EOR);
#endif
#ifdef	MSG_TRUNC
	PyModule_AddIntConstant(m, "MSG_TRUNC", MSG_TRUNC);
#endif
#ifdef	MSG_CTRUNC
	PyModule_AddIntConstant(m, "MSG_CTRUNC", MSG_CTRUNC);
#endif
#ifdef	MSG_WAITALL
	PyModule_AddIntConstant(m, "MSG_WAITALL", MSG_WAITALL);
#endif
#ifdef	MSG_BTAG
	PyModule_AddIntConstant(m, "MSG_BTAG", MSG_BTAG);
#endif
#ifdef	MSG_ETAG
	PyModule_AddIntConstant(m, "MSG_ETAG", MSG_ETAG);
#endif

	/* Protocol level and numbers, usable for [gs]etsockopt */
#ifdef	SOL_SOCKET
	PyModule_AddIntConstant(m, "SOL_SOCKET", SOL_SOCKET);
#endif
#ifdef	SOL_IP
	PyModule_AddIntConstant(m, "SOL_IP", SOL_IP);
#else
	PyModule_AddIntConstant(m, "SOL_IP", 0);
#endif
#ifdef	SOL_IPX
	PyModule_AddIntConstant(m, "SOL_IPX", SOL_IPX);
#endif
#ifdef	SOL_AX25
	PyModule_AddIntConstant(m, "SOL_AX25", SOL_AX25);
#endif
#ifdef	SOL_ATALK
	PyModule_AddIntConstant(m, "SOL_ATALK", SOL_ATALK);
#endif
#ifdef	SOL_NETROM
	PyModule_AddIntConstant(m, "SOL_NETROM", SOL_NETROM);
#endif
#ifdef	SOL_ROSE
	PyModule_AddIntConstant(m, "SOL_ROSE", SOL_ROSE);
#endif
#ifdef	SOL_TCP
	PyModule_AddIntConstant(m, "SOL_TCP", SOL_TCP);
#else
	PyModule_AddIntConstant(m, "SOL_TCP", 6);
#endif
#ifdef	SOL_UDP
	PyModule_AddIntConstant(m, "SOL_UDP", SOL_UDP);
#else
	PyModule_AddIntConstant(m, "SOL_UDP", 17);
#endif
#ifdef	IPPROTO_IP
	PyModule_AddIntConstant(m, "IPPROTO_IP", IPPROTO_IP);
#else
	PyModule_AddIntConstant(m, "IPPROTO_IP", 0);
#endif
#ifdef	IPPROTO_HOPOPTS
	PyModule_AddIntConstant(m, "IPPROTO_HOPOPTS", IPPROTO_HOPOPTS);
#endif
#ifdef	IPPROTO_ICMP
	PyModule_AddIntConstant(m, "IPPROTO_ICMP", IPPROTO_ICMP);
#else
	PyModule_AddIntConstant(m, "IPPROTO_ICMP", 1);
#endif
#ifdef	IPPROTO_IGMP
	PyModule_AddIntConstant(m, "IPPROTO_IGMP", IPPROTO_IGMP);
#endif
#ifdef	IPPROTO_GGP
	PyModule_AddIntConstant(m, "IPPROTO_GGP", IPPROTO_GGP);
#endif
#ifdef	IPPROTO_IPV4
	PyModule_AddIntConstant(m, "IPPROTO_IPV4", IPPROTO_IPV4);
#endif
#ifdef	IPPROTO_IPV6
	PyModule_AddIntConstant(m, "IPPROTO_IPV6", IPPROTO_IPV6);
#endif
#ifdef	IPPROTO_IPIP
	PyModule_AddIntConstant(m, "IPPROTO_IPIP", IPPROTO_IPIP);
#endif
#ifdef	IPPROTO_TCP
	PyModule_AddIntConstant(m, "IPPROTO_TCP", IPPROTO_TCP);
#else
	PyModule_AddIntConstant(m, "IPPROTO_TCP", 6);
#endif
#ifdef	IPPROTO_EGP
	PyModule_AddIntConstant(m, "IPPROTO_EGP", IPPROTO_EGP);
#endif
#ifdef	IPPROTO_PUP
	PyModule_AddIntConstant(m, "IPPROTO_PUP", IPPROTO_PUP);
#endif
#ifdef	IPPROTO_UDP
	PyModule_AddIntConstant(m, "IPPROTO_UDP", IPPROTO_UDP);
#else
	PyModule_AddIntConstant(m, "IPPROTO_UDP", 17);
#endif
#ifdef	IPPROTO_IDP
	PyModule_AddIntConstant(m, "IPPROTO_IDP", IPPROTO_IDP);
#endif
#ifdef	IPPROTO_HELLO
	PyModule_AddIntConstant(m, "IPPROTO_HELLO", IPPROTO_HELLO);
#endif
#ifdef	IPPROTO_ND
	PyModule_AddIntConstant(m, "IPPROTO_ND", IPPROTO_ND);
#endif
#ifdef	IPPROTO_TP
	PyModule_AddIntConstant(m, "IPPROTO_TP", IPPROTO_TP);
#endif
#ifdef	IPPROTO_IPV6
	PyModule_AddIntConstant(m, "IPPROTO_IPV6", IPPROTO_IPV6);
#endif
#ifdef	IPPROTO_ROUTING
	PyModule_AddIntConstant(m, "IPPROTO_ROUTING", IPPROTO_ROUTING);
#endif
#ifdef	IPPROTO_FRAGMENT
	PyModule_AddIntConstant(m, "IPPROTO_FRAGMENT", IPPROTO_FRAGMENT);
#endif
#ifdef	IPPROTO_RSVP
	PyModule_AddIntConstant(m, "IPPROTO_RSVP", IPPROTO_RSVP);
#endif
#ifdef	IPPROTO_GRE
	PyModule_AddIntConstant(m, "IPPROTO_GRE", IPPROTO_GRE);
#endif
#ifdef	IPPROTO_ESP
	PyModule_AddIntConstant(m, "IPPROTO_ESP", IPPROTO_ESP);
#endif
#ifdef	IPPROTO_AH
	PyModule_AddIntConstant(m, "IPPROTO_AH", IPPROTO_AH);
#endif
#ifdef	IPPROTO_MOBILE
	PyModule_AddIntConstant(m, "IPPROTO_MOBILE", IPPROTO_MOBILE);
#endif
#ifdef	IPPROTO_ICMPV6
	PyModule_AddIntConstant(m, "IPPROTO_ICMPV6", IPPROTO_ICMPV6);
#endif
#ifdef	IPPROTO_NONE
	PyModule_AddIntConstant(m, "IPPROTO_NONE", IPPROTO_NONE);
#endif
#ifdef	IPPROTO_DSTOPTS
	PyModule_AddIntConstant(m, "IPPROTO_DSTOPTS", IPPROTO_DSTOPTS);
#endif
#ifdef	IPPROTO_XTP
	PyModule_AddIntConstant(m, "IPPROTO_XTP", IPPROTO_XTP);
#endif
#ifdef	IPPROTO_EON
	PyModule_AddIntConstant(m, "IPPROTO_EON", IPPROTO_EON);
#endif
#ifdef	IPPROTO_PIM
	PyModule_AddIntConstant(m, "IPPROTO_PIM", IPPROTO_PIM);
#endif
#ifdef	IPPROTO_IPCOMP
	PyModule_AddIntConstant(m, "IPPROTO_IPCOMP", IPPROTO_IPCOMP);
#endif
#ifdef	IPPROTO_VRRP
	PyModule_AddIntConstant(m, "IPPROTO_VRRP", IPPROTO_VRRP);
#endif
#ifdef	IPPROTO_BIP
	PyModule_AddIntConstant(m, "IPPROTO_BIP", IPPROTO_BIP);
#endif
/**/
#ifdef	IPPROTO_RAW
	PyModule_AddIntConstant(m, "IPPROTO_RAW", IPPROTO_RAW);
#else
	PyModule_AddIntConstant(m, "IPPROTO_RAW", 255);
#endif
#ifdef	IPPROTO_MAX
	PyModule_AddIntConstant(m, "IPPROTO_MAX", IPPROTO_MAX);
#endif

	/* Some port configuration */
#ifdef	IPPORT_RESERVED
	PyModule_AddIntConstant(m, "IPPORT_RESERVED", IPPORT_RESERVED);
#else
	PyModule_AddIntConstant(m, "IPPORT_RESERVED", 1024);
#endif
#ifdef	IPPORT_USERRESERVED
	PyModule_AddIntConstant(m, "IPPORT_USERRESERVED", IPPORT_USERRESERVED);
#else
	PyModule_AddIntConstant(m, "IPPORT_USERRESERVED", 5000);
#endif

	/* Some reserved IP v.4 addresses */
#ifdef	INADDR_ANY
	PyModule_AddIntConstant(m, "INADDR_ANY", INADDR_ANY);
#else
	PyModule_AddIntConstant(m, "INADDR_ANY", 0x00000000);
#endif
#ifdef	INADDR_BROADCAST
	PyModule_AddIntConstant(m, "INADDR_BROADCAST", INADDR_BROADCAST);
#else
	PyModule_AddIntConstant(m, "INADDR_BROADCAST", 0xffffffff);
#endif
#ifdef	INADDR_LOOPBACK
	PyModule_AddIntConstant(m, "INADDR_LOOPBACK", INADDR_LOOPBACK);
#else
	PyModule_AddIntConstant(m, "INADDR_LOOPBACK", 0x7F000001);
#endif
#ifdef	INADDR_UNSPEC_GROUP
	PyModule_AddIntConstant(m, "INADDR_UNSPEC_GROUP", INADDR_UNSPEC_GROUP);
#else
	PyModule_AddIntConstant(m, "INADDR_UNSPEC_GROUP", 0xe0000000);
#endif
#ifdef	INADDR_ALLHOSTS_GROUP
	PyModule_AddIntConstant(m, "INADDR_ALLHOSTS_GROUP",
				INADDR_ALLHOSTS_GROUP);
#else
	PyModule_AddIntConstant(m, "INADDR_ALLHOSTS_GROUP", 0xe0000001);
#endif
#ifdef	INADDR_MAX_LOCAL_GROUP
	PyModule_AddIntConstant(m, "INADDR_MAX_LOCAL_GROUP",
				INADDR_MAX_LOCAL_GROUP);
#else
	PyModule_AddIntConstant(m, "INADDR_MAX_LOCAL_GROUP", 0xe00000ff);
#endif
#ifdef	INADDR_NONE
	PyModule_AddIntConstant(m, "INADDR_NONE", INADDR_NONE);
#else
	PyModule_AddIntConstant(m, "INADDR_NONE", 0xffffffff);
#endif

	/* IPv4 [gs]etsockopt options */
#ifdef	IP_OPTIONS
	PyModule_AddIntConstant(m, "IP_OPTIONS", IP_OPTIONS);
#endif
#ifdef	IP_HDRINCL
	PyModule_AddIntConstant(m, "IP_HDRINCL", IP_HDRINCL);
#endif
#ifdef	IP_TOS
	PyModule_AddIntConstant(m, "IP_TOS", IP_TOS);
#endif
#ifdef	IP_TTL
	PyModule_AddIntConstant(m, "IP_TTL", IP_TTL);
#endif
#ifdef	IP_RECVOPTS
	PyModule_AddIntConstant(m, "IP_RECVOPTS", IP_RECVOPTS);
#endif
#ifdef	IP_RECVRETOPTS
	PyModule_AddIntConstant(m, "IP_RECVRETOPTS", IP_RECVRETOPTS);
#endif
#ifdef	IP_RECVDSTADDR
	PyModule_AddIntConstant(m, "IP_RECVDSTADDR", IP_RECVDSTADDR);
#endif
#ifdef	IP_RETOPTS
	PyModule_AddIntConstant(m, "IP_RETOPTS", IP_RETOPTS);
#endif
#ifdef	IP_MULTICAST_IF
	PyModule_AddIntConstant(m, "IP_MULTICAST_IF", IP_MULTICAST_IF);
#endif
#ifdef	IP_MULTICAST_TTL
	PyModule_AddIntConstant(m, "IP_MULTICAST_TTL", IP_MULTICAST_TTL);
#endif
#ifdef	IP_MULTICAST_LOOP
	PyModule_AddIntConstant(m, "IP_MULTICAST_LOOP", IP_MULTICAST_LOOP);
#endif
#ifdef	IP_ADD_MEMBERSHIP
	PyModule_AddIntConstant(m, "IP_ADD_MEMBERSHIP", IP_ADD_MEMBERSHIP);
#endif
#ifdef	IP_DROP_MEMBERSHIP
	PyModule_AddIntConstant(m, "IP_DROP_MEMBERSHIP", IP_DROP_MEMBERSHIP);
#endif
#ifdef	IP_DEFAULT_MULTICAST_TTL
	PyModule_AddIntConstant(m, "IP_DEFAULT_MULTICAST_TTL",
				IP_DEFAULT_MULTICAST_TTL);
#endif
#ifdef	IP_DEFAULT_MULTICAST_LOOP
	PyModule_AddIntConstant(m, "IP_DEFAULT_MULTICAST_LOOP",
				IP_DEFAULT_MULTICAST_LOOP);
#endif
#ifdef	IP_MAX_MEMBERSHIPS
	PyModule_AddIntConstant(m, "IP_MAX_MEMBERSHIPS", IP_MAX_MEMBERSHIPS);
#endif

	/* IPv6 [gs]etsockopt options, defined in RFC2553 */
#ifdef	IPV6_JOIN_GROUP
	PyModule_AddIntConstant(m, "IPV6_JOIN_GROUP", IPV6_JOIN_GROUP);
#endif
#ifdef	IPV6_LEAVE_GROUP
	PyModule_AddIntConstant(m, "IPV6_LEAVE_GROUP", IPV6_LEAVE_GROUP);
#endif
#ifdef	IPV6_MULTICAST_HOPS
	PyModule_AddIntConstant(m, "IPV6_MULTICAST_HOPS", IPV6_MULTICAST_HOPS);
#endif
#ifdef	IPV6_MULTICAST_IF
	PyModule_AddIntConstant(m, "IPV6_MULTICAST_IF", IPV6_MULTICAST_IF);
#endif
#ifdef	IPV6_MULTICAST_LOOP
	PyModule_AddIntConstant(m, "IPV6_MULTICAST_LOOP", IPV6_MULTICAST_LOOP);
#endif
#ifdef	IPV6_UNICAST_HOPS
	PyModule_AddIntConstant(m, "IPV6_UNICAST_HOPS", IPV6_UNICAST_HOPS);
#endif
        /* Additional IPV6 socket options, defined in RFC 3493 */
#ifdef IPV6_V6ONLY
	PyModule_AddIntConstant(m, "IPV6_V6ONLY", IPV6_V6ONLY);
#endif
	/* Advanced IPV6 socket options, from RFC 3542 */
#ifdef IPV6_CHECKSUM
	PyModule_AddIntConstant(m, "IPV6_CHECKSUM", IPV6_CHECKSUM);
#endif
#ifdef IPV6_DONTFRAG
	PyModule_AddIntConstant(m, "IPV6_DONTFRAG", IPV6_DONTFRAG);
#endif
#ifdef IPV6_DSTOPTS
	PyModule_AddIntConstant(m, "IPV6_DSTOPTS", IPV6_DSTOPTS);
#endif
#ifdef IPV6_HOPLIMIT
	PyModule_AddIntConstant(m, "IPV6_HOPLIMIT", IPV6_HOPLIMIT);
#endif
#ifdef IPV6_HOPOPTS
	PyModule_AddIntConstant(m, "IPV6_HOPOPTS", IPV6_HOPOPTS);
#endif
#ifdef IPV6_NEXTHOP
	PyModule_AddIntConstant(m, "IPV6_NEXTHOP", IPV6_NEXTHOP);
#endif
#ifdef IPV6_PATHMTU
	PyModule_AddIntConstant(m, "IPV6_PATHMTU", IPV6_PATHMTU);
#endif
#ifdef IPV6_PKTINFO
	PyModule_AddIntConstant(m, "IPV6_PKTINFO", IPV6_PKTINFO);
#endif
#ifdef IPV6_RECVDSTOPTS
	PyModule_AddIntConstant(m, "IPV6_RECVDSTOPTS", IPV6_RECVDSTOPTS);
#endif
#ifdef IPV6_RECVHOPLIMIT
	PyModule_AddIntConstant(m, "IPV6_RECVHOPLIMIT", IPV6_RECVHOPLIMIT);
#endif
#ifdef IPV6_RECVHOPOPTS
	PyModule_AddIntConstant(m, "IPV6_RECVHOPOPTS", IPV6_RECVHOPOPTS);
#endif
#ifdef IPV6_RECVPKTINFO
	PyModule_AddIntConstant(m, "IPV6_RECVPKTINFO", IPV6_RECVPKTINFO);
#endif
#ifdef IPV6_RECVRTHDR
	PyModule_AddIntConstant(m, "IPV6_RECVRTHDR", IPV6_RECVRTHDR);
#endif
#ifdef IPV6_RECVTCLASS
	PyModule_AddIntConstant(m, "IPV6_RECVTCLASS", IPV6_RECVTCLASS);
#endif
#ifdef IPV6_RTHDR
	PyModule_AddIntConstant(m, "IPV6_RTHDR", IPV6_RTHDR);
#endif
#ifdef IPV6_RTHDRDSTOPTS
	PyModule_AddIntConstant(m, "IPV6_RTHDRDSTOPTS", IPV6_RTHDRDSTOPTS);
#endif
#ifdef IPV6_RTHDR_TYPE_0
	PyModule_AddIntConstant(m, "IPV6_RTHDR_TYPE_0", IPV6_RTHDR_TYPE_0);
#endif
#ifdef IPV6_RECVPATHMTU
	PyModule_AddIntConstant(m, "IPV6_RECVPATHMTU", IPV6_RECVPATHMTU);
#endif
#ifdef IPV6_TCLASS
	PyModule_AddIntConstant(m, "IPV6_TCLASS", IPV6_TCLASS);
#endif
#ifdef IPV6_USE_MIN_MTU
	PyModule_AddIntConstant(m, "IPV6_USE_MIN_MTU", IPV6_USE_MIN_MTU);
#endif

	/* TCP options */
#ifdef	TCP_NODELAY
	PyModule_AddIntConstant(m, "TCP_NODELAY", TCP_NODELAY);
#endif
#ifdef	TCP_MAXSEG
	PyModule_AddIntConstant(m, "TCP_MAXSEG", TCP_MAXSEG);
#endif
#ifdef	TCP_CORK
	PyModule_AddIntConstant(m, "TCP_CORK", TCP_CORK);
#endif
#ifdef	TCP_KEEPIDLE
	PyModule_AddIntConstant(m, "TCP_KEEPIDLE", TCP_KEEPIDLE);
#endif
#ifdef	TCP_KEEPINTVL
	PyModule_AddIntConstant(m, "TCP_KEEPINTVL", TCP_KEEPINTVL);
#endif
#ifdef	TCP_KEEPCNT
	PyModule_AddIntConstant(m, "TCP_KEEPCNT", TCP_KEEPCNT);
#endif
#ifdef	TCP_SYNCNT
	PyModule_AddIntConstant(m, "TCP_SYNCNT", TCP_SYNCNT);
#endif
#ifdef	TCP_LINGER2
	PyModule_AddIntConstant(m, "TCP_LINGER2", TCP_LINGER2);
#endif
#ifdef	TCP_DEFER_ACCEPT
	PyModule_AddIntConstant(m, "TCP_DEFER_ACCEPT", TCP_DEFER_ACCEPT);
#endif
#ifdef	TCP_WINDOW_CLAMP
	PyModule_AddIntConstant(m, "TCP_WINDOW_CLAMP", TCP_WINDOW_CLAMP);
#endif
#ifdef	TCP_INFO
	PyModule_AddIntConstant(m, "TCP_INFO", TCP_INFO);
#endif
#ifdef	TCP_QUICKACK
	PyModule_AddIntConstant(m, "TCP_QUICKACK", TCP_QUICKACK);
#endif


	/* IPX options */
#ifdef	IPX_TYPE
	PyModule_AddIntConstant(m, "IPX_TYPE", IPX_TYPE);
#endif

	/* get{addr,name}info parameters */
#ifdef EAI_ADDRFAMILY
	PyModule_AddIntConstant(m, "EAI_ADDRFAMILY", EAI_ADDRFAMILY);
#endif
#ifdef EAI_AGAIN
	PyModule_AddIntConstant(m, "EAI_AGAIN", EAI_AGAIN);
#endif
#ifdef EAI_BADFLAGS
	PyModule_AddIntConstant(m, "EAI_BADFLAGS", EAI_BADFLAGS);
#endif
#ifdef EAI_FAIL
	PyModule_AddIntConstant(m, "EAI_FAIL", EAI_FAIL);
#endif
#ifdef EAI_FAMILY
	PyModule_AddIntConstant(m, "EAI_FAMILY", EAI_FAMILY);
#endif
#ifdef EAI_MEMORY
	PyModule_AddIntConstant(m, "EAI_MEMORY", EAI_MEMORY);
#endif
#ifdef EAI_NODATA
	PyModule_AddIntConstant(m, "EAI_NODATA", EAI_NODATA);
#endif
#ifdef EAI_NONAME
	PyModule_AddIntConstant(m, "EAI_NONAME", EAI_NONAME);
#endif
#ifdef EAI_OVERFLOW
	PyModule_AddIntConstant(m, "EAI_OVERFLOW", EAI_OVERFLOW);
#endif
#ifdef EAI_SERVICE
	PyModule_AddIntConstant(m, "EAI_SERVICE", EAI_SERVICE);
#endif
#ifdef EAI_SOCKTYPE
	PyModule_AddIntConstant(m, "EAI_SOCKTYPE", EAI_SOCKTYPE);
#endif
#ifdef EAI_SYSTEM
	PyModule_AddIntConstant(m, "EAI_SYSTEM", EAI_SYSTEM);
#endif
#ifdef EAI_BADHINTS
	PyModule_AddIntConstant(m, "EAI_BADHINTS", EAI_BADHINTS);
#endif
#ifdef EAI_PROTOCOL
	PyModule_AddIntConstant(m, "EAI_PROTOCOL", EAI_PROTOCOL);
#endif
#ifdef EAI_MAX
	PyModule_AddIntConstant(m, "EAI_MAX", EAI_MAX);
#endif
#ifdef AI_PASSIVE
	PyModule_AddIntConstant(m, "AI_PASSIVE", AI_PASSIVE);
#endif
#ifdef AI_CANONNAME
	PyModule_AddIntConstant(m, "AI_CANONNAME", AI_CANONNAME);
#endif
#ifdef AI_NUMERICHOST
	PyModule_AddIntConstant(m, "AI_NUMERICHOST", AI_NUMERICHOST);
#endif
#ifdef AI_NUMERICSERV
	PyModule_AddIntConstant(m, "AI_NUMERICSERV", AI_NUMERICSERV);
#endif
#ifdef AI_MASK
	PyModule_AddIntConstant(m, "AI_MASK", AI_MASK);
#endif
#ifdef AI_ALL
	PyModule_AddIntConstant(m, "AI_ALL", AI_ALL);
#endif
#ifdef AI_V4MAPPED_CFG
	PyModule_AddIntConstant(m, "AI_V4MAPPED_CFG", AI_V4MAPPED_CFG);
#endif
#ifdef AI_ADDRCONFIG
	PyModule_AddIntConstant(m, "AI_ADDRCONFIG", AI_ADDRCONFIG);
#endif
#ifdef AI_V4MAPPED
	PyModule_AddIntConstant(m, "AI_V4MAPPED", AI_V4MAPPED);
#endif
#ifdef AI_DEFAULT
	PyModule_AddIntConstant(m, "AI_DEFAULT", AI_DEFAULT);
#endif
#ifdef NI_MAXHOST
	PyModule_AddIntConstant(m, "NI_MAXHOST", NI_MAXHOST);
#endif
#ifdef NI_MAXSERV
	PyModule_AddIntConstant(m, "NI_MAXSERV", NI_MAXSERV);
#endif
#ifdef NI_NOFQDN
	PyModule_AddIntConstant(m, "NI_NOFQDN", NI_NOFQDN);
#endif
#ifdef NI_NUMERICHOST
	PyModule_AddIntConstant(m, "NI_NUMERICHOST", NI_NUMERICHOST);
#endif
#ifdef NI_NAMEREQD
	PyModule_AddIntConstant(m, "NI_NAMEREQD", NI_NAMEREQD);
#endif
#ifdef NI_NUMERICSERV
	PyModule_AddIntConstant(m, "NI_NUMERICSERV", NI_NUMERICSERV);
#endif
#ifdef NI_DGRAM
	PyModule_AddIntConstant(m, "NI_DGRAM", NI_DGRAM);
#endif

	/* shutdown() parameters */
#ifdef SHUT_RD
	PyModule_AddIntConstant(m, "SHUT_RD", SHUT_RD);
#elif defined(SD_RECEIVE)
	PyModule_AddIntConstant(m, "SHUT_RD", SD_RECEIVE);
#else
	PyModule_AddIntConstant(m, "SHUT_RD", 0);
#endif
#ifdef SHUT_WR
	PyModule_AddIntConstant(m, "SHUT_WR", SHUT_WR);
#elif defined(SD_SEND)
	PyModule_AddIntConstant(m, "SHUT_WR", SD_SEND);
#else
	PyModule_AddIntConstant(m, "SHUT_WR", 1);
#endif
#ifdef SHUT_RDWR
	PyModule_AddIntConstant(m, "SHUT_RDWR", SHUT_RDWR);
#elif defined(SD_BOTH)
	PyModule_AddIntConstant(m, "SHUT_RDWR", SD_BOTH);
#else
	PyModule_AddIntConstant(m, "SHUT_RDWR", 2);
#endif

	/* Initialize gethostbyname lock */
#if defined(USE_GETHOSTBYNAME_LOCK) || defined(USE_GETADDRINFO_LOCK)
	netdb_lock = PyThread_allocate_lock();
#endif
}


#ifndef HAVE_INET_PTON

/* Simplistic emulation code for inet_pton that only works for IPv4 */
/* These are not exposed because they do not set errno properly */

int
inet_pton(int af, const char *src, void *dst)
{
	if (af == AF_INET) {
		long packed_addr;
		packed_addr = inet_addr(src);
		if (packed_addr == INADDR_NONE)
			return 0;
		memcpy(dst, &packed_addr, 4);
		return 1;
	}
	/* Should set errno to EAFNOSUPPORT */
	return -1;
}

const char *
inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
	if (af == AF_INET) {
		struct in_addr packed_addr;
		if (size < 16)
			/* Should set errno to ENOSPC. */
			return NULL;
		memcpy(&packed_addr, src, sizeof(packed_addr));
		return strncpy(dst, inet_ntoa(packed_addr), size);
	}
	/* Should set errno to EAFNOSUPPORT */
	return NULL;
}

#endif
