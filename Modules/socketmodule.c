/* Socket module */

/*
This module provides an interface to Berkeley socket IPC.

Limitations:

- only AF_INET, AF_INET6 and AF_UNIX address families are supported in a
  portable manner, though AF_PACKET is supported under Linux.
- no read/write operations (use sendall/recv or makefile instead)
- additional restrictions apply on Windows (compensated for by socket.py)

Module interface:

- socket.error: exception raised for socket specific errors
- socket.gaierror: exception raised for getaddrinfo/getnameinfo errors,
	a subclass of socket.error
- socket.herror: exception raised for gethostby* errors,
	a subclass of socket.error
- socket.gethostbyname(hostname) --> host IP address (string: 'dd.dd.dd.dd')
- socket.gethostbyaddr(IP address) --> (hostname, [alias, ...], [IP addr, ...])
- socket.gethostname() --> host name (string: 'spam' or 'spam.domain.com')
- socket.getprotobyname(protocolname) --> protocol number
- socket.getservbyname(servicename, protocolname) --> port number
- socket.socket(family, type [, proto]) --> new socket object
- socket.ntohs(16 bit value) --> new int object
- socket.ntohl(32 bit value) --> new int object
- socket.htons(16 bit value) --> new int object
- socket.htonl(32 bit value) --> new int object
- socket.getaddrinfo(host, port [, family, socktype, proto, flags])
	--> List of (family, socktype, proto, canonname, sockaddr)
- socket.getnameinfo(sockaddr, flags) --> (host, port)
- socket.AF_INET, socket.SOCK_STREAM, etc.: constants from <socket.h>
- socket.inet_aton(IP address) -> 32-bit packed IP representation
- socket.inet_ntoa(packed IP) -> IP address string
- an Internet socket address is a pair (hostname, port)
  where hostname can be anything recognized by gethostbyname()
  (including the dd.dd.dd.dd notation) and port is in host byte order
- where a hostname is returned, the dd.dd.dd.dd notation is used
- a UNIX domain socket address is a string specifying the pathname
- an AF_PACKET socket address is a tuple containing a string
  specifying the ethernet interface and an integer specifying
  the Ethernet protocol number to be received. For example:
  ("eth0",0x1234).  Optional 3rd,4th,5th elements in the tuple
  specify packet-type and ha-type/addr -- these are ignored by
  networking code, but accepted since they are returned by the
  getsockname() method.

Socket methods:

- s.accept() --> new socket object, sockaddr
- s.bind(sockaddr) --> None
- s.close() --> None
- s.connect(sockaddr) --> None
- s.connect_ex(sockaddr) --> 0 or errno (handy for e.g. async connect)
- s.fileno() --> file descriptor
- s.dup() --> same as socket.fromfd(os.dup(s.fileno(), ...)
- s.getpeername() --> sockaddr
- s.getsockname() --> sockaddr
- s.getsockopt(level, optname[, buflen]) --> int or string
- s.listen(backlog) --> None
- s.makefile([mode[, bufsize]]) --> file object
- s.recv(buflen [,flags]) --> string
- s.recvfrom(buflen [,flags]) --> string, sockaddr
- s.send(string [,flags]) --> nbytes
- s.sendall(string [,flags]) # tries to send everything in a loop
- s.sendto(string, [flags,] sockaddr) --> nbytes
- s.setblocking(0 | 1) --> None
- s.setsockopt(level, optname, value) --> None
- s.shutdown(how) --> None
- repr(s) --> "<socket object, fd=%d, family=%d, type=%d, protocol=%d>"

*/

#include "Python.h"

/* XXX This is a terrible mess of of platform-dependent preprocessor hacks.
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

#if !defined(HAVE_GETHOSTBYNAME_R) && defined(WITH_THREAD) && !defined(MS_WINDOWS)
# define USE_GETHOSTBYNAME_LOCK
#endif

#ifdef USE_GETHOSTBYNAME_LOCK
# include "pythread.h"
#endif

#if defined(PYCC_VACPP)
# include <types.h>
# include <io.h>
# include <sys/ioctl.h>
# include <utils.h>
# include <ctype.h>
#endif

#if defined(PYOS_OS2)
# define  INCL_DOS
# define  INCL_DOSERRORS
# define  INCL_NOPMAPI
# include <os2.h>
#endif

/* Generic includes */
#include <sys/types.h>
#include <signal.h>

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
#  include <sys/fcntl.h>
#  define NO_DUP
int h_errno; /* not used */
# endif

#else

/* MS_WINDOWS includes */
# include <fcntl.h>

#endif

#ifdef HAVE_STDDEF_H
# include <stddef.h>
#endif

#ifndef offsetof
# define offsetof(type, member)	((size_t)(&((type *)0)->member))
#endif

#ifndef O_NDELAY
# define O_NDELAY O_NONBLOCK	/* For QNX only? */
#endif

#include "addrinfo.h"

#ifndef HAVE_INET_PTON
int inet_pton (int af, const char *src, void *dst);
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
#undef HAVE_GETADDRINFO
/* avoid clashes with the C library definition of the symbol. */
#define getaddrinfo fake_getaddrinfo
#endif

/* I know this is a bad practice, but it is the easiest... */
#if !defined(HAVE_GETADDRINFO)
#include "getaddrinfo.c"
#endif
#if !defined(HAVE_GETNAMEINFO)
#include "getnameinfo.c"
#endif

#if defined(MS_WINDOWS) || defined(__BEOS__)
/* BeOS suffers from the same socket dichotomy as Win32... - [cjh] */
/* seem to be a few differences in the API */
#define SOCKETCLOSE closesocket
#define NO_DUP /* Actually it exists on NT 3.5, but what the heck... */
#endif

#ifdef MS_WIN32
#	define EAFNOSUPPORT            WSAEAFNOSUPPORT
#	define snprintf _snprintf
#endif

#if defined(PYOS_OS2) && !defined(PYCC_GCC)
#define SOCKETCLOSE soclose
#define NO_DUP /* Sockets are Not Actual File Handles under OS/2 */
#endif

#ifndef SOCKETCLOSE
#define SOCKETCLOSE close
#endif

/* XXX There's a problem here: *static* functions are not supposed to have
   a Py prefix (or use CapitalizedWords).  Later... */

/* Global variable holding the exception type for errors detected
   by this module (but not argument type or memory errors, etc.). */

static PyObject *PySocket_Error;
static PyObject *PyH_Error;
static PyObject *PyGAI_Error;

#ifdef RISCOS
/* Global variable which is !=0 if Python is running in a RISC OS taskwindow */
static int taskwindow;
#endif

/* A forward reference to the socket type object.
   The PySocketSock_Type variable contains pointers to various functions,
   some of which call PySocketSock_New(), which uses PySocketSock_Type, so
   there has to be a circular reference. */
staticforward PyTypeObject PySocketSock_Type;

/* Convenience function to raise an error according to errno
   and return a NULL pointer from a function. */

static PyObject *
PySocket_Err(void)
{
#ifdef MS_WINDOWS
	int err_no = WSAGetLastError();
	if (err_no) {
		static struct { int no; const char *msg; } *msgp, msgs[] = {
			{ WSAEINTR, "Interrupted system call" },
			{ WSAEBADF, "Bad file descriptor" },
			{ WSAEACCES, "Permission denied" },
			{ WSAEFAULT, "Bad address" },
			{ WSAEINVAL, "Invalid argument" },
			{ WSAEMFILE, "Too many open files" },
			{ WSAEWOULDBLOCK,
				"The socket operation could not complete "
				"without blocking" },
			{ WSAEINPROGRESS, "Operation now in progress" },
			{ WSAEALREADY, "Operation already in progress" },
			{ WSAENOTSOCK, "Socket operation on non-socket" },
			{ WSAEDESTADDRREQ, "Destination address required" },
			{ WSAEMSGSIZE, "Message too long" },
			{ WSAEPROTOTYPE, "Protocol wrong type for socket" },
			{ WSAENOPROTOOPT, "Protocol not available" },
			{ WSAEPROTONOSUPPORT, "Protocol not supported" },
			{ WSAESOCKTNOSUPPORT, "Socket type not supported" },
			{ WSAEOPNOTSUPP, "Operation not supported" },
			{ WSAEPFNOSUPPORT, "Protocol family not supported" },
			{ WSAEAFNOSUPPORT, "Address family not supported" },
			{ WSAEADDRINUSE, "Address already in use" },
			{ WSAEADDRNOTAVAIL,
				"Can't assign requested address" },
			{ WSAENETDOWN, "Network is down" },
			{ WSAENETUNREACH, "Network is unreachable" },
			{ WSAENETRESET,
				"Network dropped connection on reset" },
			{ WSAECONNABORTED,
				"Software caused connection abort" },
			{ WSAECONNRESET, "Connection reset by peer" },
			{ WSAENOBUFS, "No buffer space available" },
			{ WSAEISCONN, "Socket is already connected" },
			{ WSAENOTCONN, "Socket is not connected" },
			{ WSAESHUTDOWN, "Can't send after socket shutdown" },
			{ WSAETOOMANYREFS,
				"Too many references: can't splice" },
			{ WSAETIMEDOUT, "Operation timed out" },
			{ WSAECONNREFUSED, "Connection refused" },
			{ WSAELOOP, "Too many levels of symbolic links" },
			{ WSAENAMETOOLONG, "File name too long" },
			{ WSAEHOSTDOWN, "Host is down" },
			{ WSAEHOSTUNREACH, "No route to host" },
			{ WSAENOTEMPTY, "Directory not empty" },
			{ WSAEPROCLIM, "Too many processes" },
			{ WSAEUSERS, "Too many users" },
			{ WSAEDQUOT, "Disc quota exceeded" },
			{ WSAESTALE, "Stale NFS file handle" },
			{ WSAEREMOTE, "Too many levels of remote in path" },
			{ WSASYSNOTREADY,
				"Network subsystem is unvailable" },
			{ WSAVERNOTSUPPORTED,
				"WinSock version is not supported" },
			{ WSANOTINITIALISED,
				"Successful WSAStartup() not yet performed" },
			{ WSAEDISCON, "Graceful shutdown in progress" },
			/* Resolver errors */
			{ WSAHOST_NOT_FOUND, "No such host is known" },
			{ WSATRY_AGAIN, "Host not found, or server failed" },
			{ WSANO_RECOVERY,
				"Unexpected server error encountered" },
			{ WSANO_DATA, "Valid name without requested data" },
			{ WSANO_ADDRESS, "No address, look for MX record" },
			{ 0, NULL }
		};
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
			PyErr_SetObject(PySocket_Error, v);
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
        char   outbuf[100];
        int    myerrorcode = sock_errno();

        /* Retrieve Socket-Related Error Message from MPTN.MSG File */
        rc = DosGetMessage(NULL, 0, outbuf, sizeof(outbuf),
                           myerrorcode - SOCBASEERR + 26, "mptn.msg", &msglen);
        if (rc == NO_ERROR) {
            PyObject *v;

            outbuf[msglen] = '\0'; /* OS/2 Doesn't Guarantee a Terminator */
            if (strlen(outbuf) > 0) { /* If Non-Empty Msg, Trim CRLF */
                char *lastc = &outbuf[ strlen(outbuf)-1 ];
                while (lastc > outbuf && isspace(*lastc))
                    *lastc-- = '\0'; /* Trim Trailing Whitespace (CRLF) */
            }
            v = Py_BuildValue("(is)", myerrorcode, outbuf);
            if (v != NULL) {
                PyErr_SetObject(PySocket_Error, v);
                Py_DECREF(v);
            }
            return NULL;
        }
    }
#endif

	return PyErr_SetFromErrno(PySocket_Error);
}


static PyObject *
PyH_Err(int h_error)
{
	PyObject *v;

#ifdef HAVE_HSTRERROR
	v = Py_BuildValue("(is)", h_error, (char *)hstrerror(h_error));
#else
	v = Py_BuildValue("(is)", h_error, "host not found");
#endif
	if (v != NULL) {
		PyErr_SetObject(PyH_Error, v);
		Py_DECREF(v);
	}

	return NULL;
}


static PyObject *
PyGAI_Err(int error)
{
	PyObject *v;

#ifdef EAI_SYSTEM
	/* EAI_SYSTEM is not available on Windows XP. */
	if (error == EAI_SYSTEM)
		return PySocket_Err();
#endif

#ifdef HAVE_GAI_STRERROR
	v = Py_BuildValue("(is)", error, gai_strerror(error));
#else
	v = Py_BuildValue("(is)", error, "getaddrinfo failed");
#endif
	if (v != NULL) {
		PyErr_SetObject(PyGAI_Error, v);
		Py_DECREF(v);
	}

	return NULL;
}

/* Initialize a new socket object. */

static void
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
	s->errorhandler = &PySocket_Err;
#ifdef RISCOS
	if(taskwindow) {
		socketioctl(s->sock_fd, 0x80046679, (u_long*)&block);
	}
#endif
}


/* Create a new socket object.
   This just creates the object and initializes it.
   If the creation fails, return NULL and set an exception (implicit
   in NEWOBJ()). */

static PySocketSockObject *
PySocketSock_New(SOCKET_T fd, int family, int type, int proto)
{
	PySocketSockObject *s;
	s = (PySocketSockObject *)
		PyType_GenericNew(&PySocketSock_Type, NULL, NULL);
	if (s != NULL)
		init_sockobject(s, fd, family, type, proto);
	return s;
}


/* Lock to allow python interpreter to continue, but only allow one
   thread to be in gethostbyname */
#ifdef USE_GETHOSTBYNAME_LOCK
PyThread_type_lock gethostbyname_lock;
#endif


/* Convert a string specifying a host name or one of a few symbolic
   names to a numeric IP address.  This usually calls gethostbyname()
   to do the work; the names "" and "<broadcast>" are special.
   Return the length (IPv4 should be 4 bytes), or negative if
   an error occurred; then an exception is raised. */

static int
setipaddr(char* name, struct sockaddr * addr_ret, int af)
{
	struct addrinfo hints, *res;
	int error;

	memset((void *) addr_ret, '\0', sizeof(*addr_ret));
	if (name[0] == '\0') {
		int siz;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = af;
		hints.ai_socktype = SOCK_DGRAM;	/*dummy*/
		hints.ai_flags = AI_PASSIVE;
		error = getaddrinfo(NULL, "0", &hints, &res);
		if (error) {
			PyGAI_Err(error);
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
			PyErr_SetString(PySocket_Error,
				"unsupported address family");
			return -1;
		}
		if (res->ai_next) {
			freeaddrinfo(res);
			PyErr_SetString(PySocket_Error,
				"wildcard resolved to multiple address");
			return -1;
		}
		memcpy(addr_ret, res->ai_addr, res->ai_addrlen);
		freeaddrinfo(res);
		return siz;
	}
	if (name[0] == '<' && strcmp(name, "<broadcast>") == 0) {
		struct sockaddr_in *sin;
		if (af != PF_INET && af != PF_UNSPEC) {
			PyErr_SetString(PySocket_Error,
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
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = af;
	error = getaddrinfo(name, NULL, &hints, &res);
#if defined(__digital__) && defined(__unix__)
        if (error == EAI_NONAME && af == AF_UNSPEC) {
          /* On Tru64 V5.1, numeric-to-addr conversion
             fails if no address family is given. Assume IPv4 for now.*/
          hints.ai_family = AF_INET;
          error = getaddrinfo(name, NULL, &hints, &res);
        }
#endif
	if (error) {
		PyGAI_Err(error);
		return -1;
	}
	memcpy((char *) addr_ret, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);
	switch (addr_ret->sa_family) {
	case AF_INET:
		return 4;
#ifdef ENABLE_IPV6
	case AF_INET6:
		return 16;
#endif
	default:
		PyErr_SetString(PySocket_Error, "unknown address family");
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
		PyGAI_Err(error);
		return NULL;
	}
	return PyString_FromString(buf);
}


/* Create an object representing the given socket address,
   suitable for passing it back to bind(), connect() etc.
   The family field of the sockaddr structure is inspected
   to determine what kind of address it really is. */

/*ARGSUSED*/
static PyObject *
makesockaddr(int sockfd, struct sockaddr *addr, int addrlen)
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

#ifdef AF_UNIX
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
			ret = Py_BuildValue("Oiii", addrobj, ntohs(a->sin6_port),
				a->sin6_flowinfo, a->sin6_scope_id);
			Py_DECREF(addrobj);
		}
		return ret;
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
		return Py_BuildValue("shbhs#", ifname, ntohs(a->sll_protocol),
				     a->sll_pkttype, a->sll_hatype,
				     a->sll_addr, a->sll_halen);
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

#ifdef AF_UNIX
	case AF_UNIX:
	{
		struct sockaddr_un* addr;
		char *path;
		int len;
		addr = (struct sockaddr_un* )&(s->sock_addr).un;
		if (!PyArg_Parse(args, "t#", &path, &len))
			return 0;
		if (len > sizeof addr->sun_path) {
			PyErr_SetString(PySocket_Error,
					"AF_UNIX path too long");
			return 0;
		}
		addr->sun_family = s->sock_family;
		memcpy(addr->sun_path, path, len);
		addr->sun_path[len] = 0;
		*addr_ret = (struct sockaddr *) addr;
		*len_ret = len + sizeof(*addr) - sizeof(addr->sun_path);
		return 1;
	}
#endif /* AF_UNIX */

	case AF_INET:
	{
		struct sockaddr_in* addr;
		char *host;
		int port;
 		addr=(struct sockaddr_in*)&(s->sock_addr).in;
		if (!PyTuple_Check(args)) {
			PyErr_Format(PyExc_TypeError,
		  "getsockaddrarg: AF_INET address must be tuple, not %.500s",
				     args->ob_type->tp_name);
			return 0;
		}
		if (!PyArg_ParseTuple(args, "si:getsockaddrarg", &host, &port))
			return 0;
		if (setipaddr(host, (struct sockaddr *)addr, AF_INET) < 0)
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
		int port, flowinfo, scope_id;
 		addr = (struct sockaddr_in6*)&(s->sock_addr).in6;
		flowinfo = scope_id = 0;
		if (!PyArg_ParseTuple(args, "si|ii", &host, &port, &flowinfo,
				&scope_id)) {
			return 0;
		}
		if (setipaddr(host, (struct sockaddr *)addr, AF_INET6) < 0)
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

#ifdef HAVE_NETPACKET_PACKET_H
	case AF_PACKET:
	{
		struct sockaddr_ll* addr;
		struct ifreq ifr;
		char *interfaceName;
		int protoNumber;
		int hatype = 0;
		int pkttype = 0;
		char *haddr;

		if (!PyArg_ParseTuple(args, "si|iis", &interfaceName,
				      &protoNumber, &pkttype, &hatype, &haddr))
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
		*addr_ret = (struct sockaddr *) addr;
		*len_ret = sizeof *addr;
		return 1;
	}
#endif

	/* More cases here... */

	default:
		PyErr_SetString(PySocket_Error, "getsockaddrarg: bad family");
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

#ifdef AF_UNIX
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

#ifdef HAVE_NETPACKET_PACKET_H
	case AF_PACKET:
	{
		*len_ret = sizeof (struct sockaddr_ll);
		return 1;
	}
#endif

	/* More cases here... */

	default:
		PyErr_SetString(PySocket_Error, "getsockaddrlen: bad family");
		return 0;

	}
}


/* s.accept() method */

static PyObject *
PySocketSock_accept(PySocketSockObject *s)
{
	char addrbuf[256];
	SOCKET_T newfd;
	socklen_t addrlen;
	PyObject *sock = NULL;
	PyObject *addr = NULL;
	PyObject *res = NULL;

	if (!getsockaddrlen(s, &addrlen))
		return NULL;
	memset(addrbuf, 0, addrlen);
	Py_BEGIN_ALLOW_THREADS
	newfd = accept(s->sock_fd, (struct sockaddr *) addrbuf, &addrlen);
	Py_END_ALLOW_THREADS
#ifdef MS_WINDOWS
	if (newfd == INVALID_SOCKET)
#else
	if (newfd < 0)
#endif
		return s->errorhandler();

	/* Create the new object with unspecified family,
	   to avoid calls to bind() etc. on it. */
	sock = (PyObject *) PySocketSock_New(newfd,
					s->sock_family,
					s->sock_type,
					s->sock_proto);
	if (sock == NULL) {
		SOCKETCLOSE(newfd);
		goto finally;
	}
	addr = makesockaddr(s->sock_fd, (struct sockaddr *)addrbuf,
			    addrlen);
	if (addr == NULL)
		goto finally;

	res = Py_BuildValue("OO", sock, addr);

  finally:
	Py_XDECREF(sock);
	Py_XDECREF(addr);
	return res;
}

static char accept_doc[] =
"accept() -> (socket object, address info)\n\
\n\
Wait for an incoming connection.  Return a new socket representing the\n\
connection, and the address of the client.  For IP sockets, the address\n\
info is a pair (hostaddr, port).";


/* s.setblocking(1 | 0) method */

static PyObject *
PySocketSock_setblocking(PySocketSockObject *s, PyObject *arg)
{
	int block;
#ifndef RISCOS
#ifndef MS_WINDOWS
	int delay_flag;
#endif
#endif
	block = PyInt_AsLong(arg);
	if (block == -1 && PyErr_Occurred())
		return NULL;
	Py_BEGIN_ALLOW_THREADS
#ifdef __BEOS__
	block = !block;
	setsockopt( s->sock_fd, SOL_SOCKET, SO_NONBLOCK,
				(void *)(&block), sizeof( int ) );
#else
#ifndef RISCOS
#ifndef MS_WINDOWS
#if defined(PYOS_OS2) && !defined(PYCC_GCC)
	block = !block;
	ioctl(s->sock_fd, FIONBIO, (caddr_t)&block, sizeof(block));
#else /* !PYOS_OS2 */
	delay_flag = fcntl (s->sock_fd, F_GETFL, 0);
	if (block)
		delay_flag &= (~O_NDELAY);
	else
		delay_flag |= O_NDELAY;
	fcntl (s->sock_fd, F_SETFL, delay_flag);
#endif /* !PYOS_OS2 */
#else /* MS_WINDOWS */
	block = !block;
	ioctlsocket(s->sock_fd, FIONBIO, (u_long*)&block);
#endif /* MS_WINDOWS */
#endif /* __BEOS__ */
#endif /* RISCOS */
	Py_END_ALLOW_THREADS

	Py_INCREF(Py_None);
	return Py_None;
}

static char setblocking_doc[] =
"setblocking(flag)\n\
\n\
Set the socket to blocking (flag is true) or non-blocking (false).\n\
This uses the FIONBIO ioctl with the O_NDELAY flag.";


#ifdef RISCOS
/* s.sleeptaskw(1 | 0) method */

static PyObject *
PySocketSock_sleeptaskw(PySocketSockObject *s,PyObject *args)
{
 int block;
 int delay_flag;
 if (!PyArg_Parse(args, "i", &block))
  return NULL;
 Py_BEGIN_ALLOW_THREADS
  socketioctl(s->sock_fd, 0x80046679, (u_long*)&block);
 Py_END_ALLOW_THREADS

 Py_INCREF(Py_None);
 return Py_None;
}
static char sleeptaskw_doc[] =
"sleeptaskw(flag)\n\
\n\
Allow sleeps in taskwindows.";
#endif


/* s.setsockopt() method.
   With an integer third argument, sets an integer option.
   With a string third argument, sets an option from a buffer;
   use optional built-in module 'struct' to encode the string. */

static PyObject *
PySocketSock_setsockopt(PySocketSockObject *s, PyObject *args)
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

static char setsockopt_doc[] =
"setsockopt(level, option, value)\n\
\n\
Set a socket option.  See the Unix manual for level and option.\n\
The value argument can either be an integer or a string.";


/* s.getsockopt() method.
   With two arguments, retrieves an integer option.
   With a third integer argument, retrieves a string buffer of that size;
   use optional built-in module 'struct' to decode the string. */

static PyObject *
PySocketSock_getsockopt(PySocketSockObject *s, PyObject *args)
{
	int level;
	int optname;
	int res;
	PyObject *buf;
	socklen_t buflen = 0;

#ifdef __BEOS__
	/* We have incomplete socket support. */
	PyErr_SetString(PySocket_Error, "getsockopt not supported");
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
	if (buflen <= 0 || buflen > 1024) {
		PyErr_SetString(PySocket_Error,
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

static char getsockopt_doc[] =
"getsockopt(level, option[, buffersize]) -> value\n\
\n\
Get a socket option.  See the Unix manual for level and option.\n\
If a nonzero buffersize argument is given, the return value is a\n\
string of that length; otherwise it is an integer.";


/* s.bind(sockaddr) method */

static PyObject *
PySocketSock_bind(PySocketSockObject *s, PyObject *addro)
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

static char bind_doc[] =
"bind(address)\n\
\n\
Bind the socket to a local address.  For IP sockets, the address is a\n\
pair (host, port); the host must refer to the local host. For raw packet\n\
sockets the address is a tuple (ifname, proto [,pkttype [,hatype]])";


/* s.close() method.
   Set the file descriptor to -1 so operations tried subsequently
   will surely fail. */

static PyObject *
PySocketSock_close(PySocketSockObject *s)
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

static char close_doc[] =
"close()\n\
\n\
Close the socket.  It cannot be used after this call.";


/* s.connect(sockaddr) method */

static PyObject *
PySocketSock_connect(PySocketSockObject *s, PyObject *addro)
{
	struct sockaddr *addr;
	int addrlen;
	int res;

	if (!getsockaddrarg(s, addro, &addr, &addrlen))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = connect(s->sock_fd, addr, addrlen);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return s->errorhandler();
	Py_INCREF(Py_None);
	return Py_None;
}

static char connect_doc[] =
"connect(address)\n\
\n\
Connect the socket to a remote address.  For IP sockets, the address\n\
is a pair (host, port).";


/* s.connect_ex(sockaddr) method */

static PyObject *
PySocketSock_connect_ex(PySocketSockObject *s, PyObject *addro)
{
	struct sockaddr *addr;
	int addrlen;
	int res;

	if (!getsockaddrarg(s, addro, &addr, &addrlen))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = connect(s->sock_fd, addr, addrlen);
	Py_END_ALLOW_THREADS
	if (res != 0) {
#ifdef MS_WINDOWS
		res = WSAGetLastError();
#else
		res = errno;
#endif
	}
	return PyInt_FromLong((long) res);
}

static char connect_ex_doc[] =
"connect_ex(address)\n\
\n\
This is like connect(address), but returns an error code (the errno value)\n\
instead of raising an exception when an error occurs.";


/* s.fileno() method */

static PyObject *
PySocketSock_fileno(PySocketSockObject *s)
{
#if SIZEOF_SOCKET_T <= SIZEOF_LONG
	return PyInt_FromLong((long) s->sock_fd);
#else
	return PyLong_FromLongLong((LONG_LONG)s->sock_fd);
#endif
}

static char fileno_doc[] =
"fileno() -> integer\n\
\n\
Return the integer file descriptor of the socket.";


#ifndef NO_DUP
/* s.dup() method */

static PyObject *
PySocketSock_dup(PySocketSockObject *s)
{
	SOCKET_T newfd;
	PyObject *sock;

	newfd = dup(s->sock_fd);
	if (newfd < 0)
		return s->errorhandler();
	sock = (PyObject *) PySocketSock_New(newfd,
					     s->sock_family,
					     s->sock_type,
					     s->sock_proto);
	if (sock == NULL)
		SOCKETCLOSE(newfd);
	return sock;
}

static char dup_doc[] =
"dup() -> socket object\n\
\n\
Return a new socket object connected to the same system resource.";

#endif


/* s.getsockname() method */

static PyObject *
PySocketSock_getsockname(PySocketSockObject *s)
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
	return makesockaddr(s->sock_fd, (struct sockaddr *) addrbuf, addrlen);
}

static char getsockname_doc[] =
"getsockname() -> address info\n\
\n\
Return the address of the local endpoint.  For IP sockets, the address\n\
info is a pair (hostaddr, port).";


#ifdef HAVE_GETPEERNAME		/* Cray APP doesn't have this :-( */
/* s.getpeername() method */

static PyObject *
PySocketSock_getpeername(PySocketSockObject *s)
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
	return makesockaddr(s->sock_fd, (struct sockaddr *) addrbuf, addrlen);
}

static char getpeername_doc[] =
"getpeername() -> address info\n\
\n\
Return the address of the remote endpoint.  For IP sockets, the address\n\
info is a pair (hostaddr, port).";

#endif /* HAVE_GETPEERNAME */


/* s.listen(n) method */

static PyObject *
PySocketSock_listen(PySocketSockObject *s, PyObject *arg)
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

static char listen_doc[] =
"listen(backlog)\n\
\n\
Enable a server to accept connections.  The backlog argument must be at\n\
least 1; it specifies the number of unaccepted connection that the system\n\
will allow before refusing new connections.";


#ifndef NO_DUP
/* s.makefile(mode) method.
   Create a new open file object referring to a dupped version of
   the socket's file descriptor.  (The dup() call is necessary so
   that the open file and socket objects may be closed independent
   of each other.)
   The mode argument specifies 'r' or 'w' passed to fdopen(). */

static PyObject *
PySocketSock_makefile(PySocketSockObject *s, PyObject *args)
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

	if (!PyArg_ParseTuple(args, "|si:makefile", &mode, &bufsize))
		return NULL;
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
#ifdef USE_GUSI2
	/* Workaround for bug in Metrowerks MSL vs. GUSI I/O library */
	if (strchr(mode, 'b') != NULL )
		bufsize = 0;
#endif
	f = PyFile_FromFile(fp, "<socket>", mode, fclose);
	if (f != NULL)
		PyFile_SetBufSize(f, bufsize);
	return f;
}

static char makefile_doc[] =
"makefile([mode[, buffersize]]) -> file object\n\
\n\
Return a regular file object corresponding to the socket.\n\
The mode and buffersize arguments are as for the built-in open() function.";

#endif /* NO_DUP */


/* s.recv(nbytes [,flags]) method */

static PyObject *
PySocketSock_recv(PySocketSockObject *s, PyObject *args)
{
	int len, n, flags = 0;
	PyObject *buf;
	if (!PyArg_ParseTuple(args, "i|i:recv", &len, &flags))
		return NULL;
        if (len < 0) {
		PyErr_SetString(PyExc_ValueError,
				"negative buffersize in connect");
		return NULL;
	}
	buf = PyString_FromStringAndSize((char *) 0, len);
	if (buf == NULL)
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	n = recv(s->sock_fd, PyString_AS_STRING(buf), len, flags);
	Py_END_ALLOW_THREADS
	if (n < 0) {
		Py_DECREF(buf);
		return s->errorhandler();
	}
	if (n != len && _PyString_Resize(&buf, n) < 0)
		return NULL;
	return buf;
}

static char recv_doc[] =
"recv(buffersize[, flags]) -> data\n\
\n\
Receive up to buffersize bytes from the socket.  For the optional flags\n\
argument, see the Unix manual.  When no data is available, block until\n\
at least one byte is available or until the remote end is closed.  When\n\
the remote end is closed and all data is read, return the empty string.";


/* s.recvfrom(nbytes [,flags]) method */

static PyObject *
PySocketSock_recvfrom(PySocketSockObject *s, PyObject *args)
{
	char addrbuf[256];
	PyObject *buf = NULL;
	PyObject *addr = NULL;
	PyObject *ret = NULL;

	int len, n, flags = 0;
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
	if (n < 0) {
		Py_DECREF(buf);
		return s->errorhandler();
	}
	if (n != len && _PyString_Resize(&buf, n) < 0)
		return NULL;

	if (!(addr = makesockaddr(s->sock_fd, (struct sockaddr *)addrbuf, addrlen)))
		goto finally;

	ret = Py_BuildValue("OO", buf, addr);
  finally:
	Py_XDECREF(addr);
	Py_XDECREF(buf);
	return ret;
}

static char recvfrom_doc[] =
"recvfrom(buffersize[, flags]) -> (data, address info)\n\
\n\
Like recv(buffersize, flags) but also return the sender's address info.";


/* s.send(data [,flags]) method */

static PyObject *
PySocketSock_send(PySocketSockObject *s, PyObject *args)
{
	char *buf;
	int len, n, flags = 0;
	if (!PyArg_ParseTuple(args, "s#|i:send", &buf, &len, &flags))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	n = send(s->sock_fd, buf, len, flags);
	Py_END_ALLOW_THREADS
	if (n < 0)
		return s->errorhandler();
	return PyInt_FromLong((long)n);
}

static char send_doc[] =
"send(data[, flags]) -> count\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.  Return the number of bytes\n\
sent; this may be less than len(data) if the network is busy.";


/* s.sendall(data [,flags]) method */

static PyObject *
PySocketSock_sendall(PySocketSockObject *s, PyObject *args)
{
	char *buf;
	int len, n, flags = 0;
	if (!PyArg_ParseTuple(args, "s#|i:sendall", &buf, &len, &flags))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	do {
		n = send(s->sock_fd, buf, len, flags);
		if (n < 0)
			break;
		buf += n;
		len -= n;
	} while (len > 0);
	Py_END_ALLOW_THREADS
	if (n < 0)
		return s->errorhandler();
	Py_INCREF(Py_None);
	return Py_None;
}

static char sendall_doc[] =
"sendall(data[, flags])\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.  This calls send() repeatedly\n\
until all data is sent.  If an error occurs, it's impossible\n\
to tell how much data has been sent.";


/* s.sendto(data, [flags,] sockaddr) method */

static PyObject *
PySocketSock_sendto(PySocketSockObject *s, PyObject *args)
{
	PyObject *addro;
	char *buf;
	struct sockaddr *addr;
	int addrlen, len, n, flags;
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
	n = sendto(s->sock_fd, buf, len, flags, addr, addrlen);
	Py_END_ALLOW_THREADS
	if (n < 0)
		return s->errorhandler();
	return PyInt_FromLong((long)n);
}

static char sendto_doc[] =
"sendto(data[, flags], address)\n\
\n\
Like send(data, flags) but allows specifying the destination address.\n\
For IP sockets, the address is a pair (hostaddr, port).";


/* s.shutdown(how) method */

static PyObject *
PySocketSock_shutdown(PySocketSockObject *s, PyObject *arg)
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

static char shutdown_doc[] =
"shutdown(flag)\n\
\n\
Shut down the reading side of the socket (flag == 0), the writing side\n\
of the socket (flag == 1), or both ends (flag == 2).";


/* List of methods for socket objects */

static PyMethodDef PySocketSock_methods[] = {
	{"accept",	(PyCFunction)PySocketSock_accept, METH_NOARGS,
			accept_doc},
	{"bind",	(PyCFunction)PySocketSock_bind, METH_O,
			bind_doc},
	{"close",	(PyCFunction)PySocketSock_close, METH_NOARGS,
			close_doc},
	{"connect",	(PyCFunction)PySocketSock_connect, METH_O,
			connect_doc},
	{"connect_ex",	(PyCFunction)PySocketSock_connect_ex, METH_O,
			connect_ex_doc},
#ifndef NO_DUP
	{"dup",		(PyCFunction)PySocketSock_dup, METH_NOARGS,
			dup_doc},
#endif
	{"fileno",	(PyCFunction)PySocketSock_fileno, METH_NOARGS,
			fileno_doc},
#ifdef HAVE_GETPEERNAME
	{"getpeername",	(PyCFunction)PySocketSock_getpeername, 
	                METH_NOARGS, getpeername_doc},
#endif
	{"getsockname",	(PyCFunction)PySocketSock_getsockname,
	                METH_NOARGS, getsockname_doc},
	{"getsockopt",	(PyCFunction)PySocketSock_getsockopt, METH_VARARGS,
			getsockopt_doc},
	{"listen",	(PyCFunction)PySocketSock_listen, METH_O,
			listen_doc},
#ifndef NO_DUP
	{"makefile",	(PyCFunction)PySocketSock_makefile, METH_VARARGS,
			makefile_doc},
#endif
	{"recv",	(PyCFunction)PySocketSock_recv, METH_VARARGS,
			recv_doc},
	{"recvfrom",	(PyCFunction)PySocketSock_recvfrom, METH_VARARGS,
			recvfrom_doc},
	{"send",	(PyCFunction)PySocketSock_send, METH_VARARGS,
			send_doc},
	{"sendall",	(PyCFunction)PySocketSock_sendall, METH_VARARGS,
			sendall_doc},
	{"sendto",	(PyCFunction)PySocketSock_sendto, METH_VARARGS,
			sendto_doc},
	{"setblocking",	(PyCFunction)PySocketSock_setblocking, METH_O,
			setblocking_doc},
	{"setsockopt",	(PyCFunction)PySocketSock_setsockopt, METH_VARARGS,
			setsockopt_doc},
	{"shutdown",	(PyCFunction)PySocketSock_shutdown, METH_O,
			shutdown_doc},
#ifdef RISCOS
	{"sleeptaskw",	(PyCFunction)PySocketSock_sleeptaskw, METH_VARARGS,
	 		sleeptaskw_doc},
#endif
	{NULL,			NULL}		/* sentinel */
};


/* Deallocate a socket object in response to the last Py_DECREF().
   First close the file description. */

static void
PySocketSock_dealloc(PySocketSockObject *s)
{
	if (s->sock_fd != -1)
		(void) SOCKETCLOSE(s->sock_fd);
	s->ob_type->tp_free((PyObject *)s);
}


static PyObject *
PySocketSock_repr(PySocketSockObject *s)
{
	char buf[512];
#if SIZEOF_SOCKET_T > SIZEOF_LONG
	if (s->sock_fd > LONG_MAX) {
		/* this can occur on Win64, and actually there is a special
		   ugly printf formatter for decimal pointer length integer
		   printing, only bother if necessary*/
		PyErr_SetString(PyExc_OverflowError,
			"no printf formatter to display the socket descriptor in decimal");
		return NULL;
	}
#endif
	PyOS_snprintf(buf, sizeof(buf),
		      "<socket object, fd=%ld, family=%d, type=%d, protocol=%d>",
		      (long)s->sock_fd, s->sock_family,
		      s->sock_type,
		      s->sock_proto);
	return PyString_FromString(buf);
}


/* Create a new, uninitialized socket object. */

static PyObject *
PySocketSock_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *new;

	new = type->tp_alloc(type, 0);
	if (new != NULL)
		((PySocketSockObject *)new)->sock_fd = -1;
	return new;
}


/* Initialize a new socket object. */

/*ARGSUSED*/
static int
PySocketSock_init(PyObject *self, PyObject *args, PyObject *kwds)
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
		PySocket_Err();
		return -1;
	}
	init_sockobject(s, fd, family, type, proto);
	/* From now on, ignore SIGPIPE and let the error checking
	   do the work. */
#ifdef SIGPIPE
	(void) signal(SIGPIPE, SIG_IGN);
#endif
	return 0;
}


/* Type object for socket objects. */

static char socket_doc[] =
"socket([family[, type[, proto]]]) -> socket object\n\
\n\
Open a socket of the given type.  The family argument specifies the\n\
address family; it defaults to AF_INET.  The type argument specifies\n\
whether this is a stream (SOCK_STREAM, this is the default)\n\
or datagram (SOCK_DGRAM) socket.  The protocol argument defaults to 0,\n\
specifying the default protocol.\n\
\n\
A socket represents one endpoint of a network connection.\n\
\n\
Methods:\n\
\n\
accept() -- accept a connection, returning new socket and client address\n\
bind() -- bind the socket to a local address\n\
close() -- close the socket\n\
connect() -- connect the socket to a remote address\n\
connect_ex() -- connect, return an error code instead of an exception \n\
dup() -- return a new socket object identical to the current one (*)\n\
fileno() -- return underlying file descriptor\n\
getpeername() -- return remote address (*)\n\
getsockname() -- return local address\n\
getsockopt() -- get socket options\n\
listen() -- start listening for incoming connections\n\
makefile() -- return a file object corresponding to the socket (*)\n\
recv() -- receive data\n\
recvfrom() -- receive data and sender's address\n\
send() -- send data, may not send all of it\n\
sendall() -- send all data\n\
sendto() -- send data to a given address\n\
setblocking() -- set or clear the blocking I/O flag\n\
setsockopt() -- set socket options\n\
shutdown() -- shut down traffic in one or both directions\n\
\n\
(*) not available on all platforms!)";

static PyTypeObject PySocketSock_Type = {
	PyObject_HEAD_INIT(0)	/* Must fill in type value later */
	0,					/* ob_size */
	"_socket.socket",			/* tp_name */
	sizeof(PySocketSockObject),		/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)PySocketSock_dealloc,	/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)PySocketSock_repr,		/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,	/* set below */			/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	socket_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	PySocketSock_methods,			/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	PySocketSock_init,			/* tp_init */
	0,	/* set below */			/* tp_alloc */
	PySocketSock_new,			/* tp_new */
	0,	/* set below */			/* tp_free */
};


/* Python interface to gethostname(). */

/*ARGSUSED*/
static PyObject *
PySocket_gethostname(PyObject *self, PyObject *args)
{
	char buf[1024];
	int res;
	if (!PyArg_ParseTuple(args, ":gethostname"))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = gethostname(buf, (int) sizeof buf - 1);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return PySocket_Err();
	buf[sizeof buf - 1] = '\0';
	return PyString_FromString(buf);
}

static char gethostname_doc[] =
"gethostname() -> string\n\
\n\
Return the current host name.";


/* Python interface to gethostbyname(name). */

/*ARGSUSED*/
static PyObject *
PySocket_gethostbyname(PyObject *self, PyObject *args)
{
	char *name;
	struct sockaddr_storage addrbuf;

	if (!PyArg_ParseTuple(args, "s:gethostbyname", &name))
		return NULL;
	if (setipaddr(name, (struct sockaddr *)&addrbuf, AF_INET) < 0)
		return NULL;
	return makeipaddr((struct sockaddr *)&addrbuf,
		sizeof(struct sockaddr_in));
}

static char gethostbyname_doc[] =
"gethostbyname(host) -> address\n\
\n\
Return the IP address (a string of the form '255.255.255.255') for a host.";


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
		PyH_Err(h_errno);
#else
		PyErr_SetString(PySocket_Error, "host not found");
#endif
		return NULL;
	}
	if (h->h_addrtype != af) {
#ifdef HAVE_STRERROR
	        /* Let's get real error message to return */
		PyErr_SetString(PySocket_Error, (char *)strerror(EAFNOSUPPORT));
#else
		PyErr_SetString(PySocket_Error,
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
			PyErr_SetString(PySocket_Error,
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
PySocket_gethostbyname_ex(PyObject *self, PyObject *args)
{
	char *name;
	struct hostent *h;
	struct sockaddr_storage addr;
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
	if (setipaddr(name, (struct sockaddr *)&addr, PF_INET) < 0)
		return NULL;
	Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_GETHOSTBYNAME_R
#if   defined(HAVE_GETHOSTBYNAME_R_6_ARG)
	result = gethostbyname_r(name, &hp_allocated, buf, buf_len, &h, &errnop);
#elif defined(HAVE_GETHOSTBYNAME_R_5_ARG)
	h = gethostbyname_r(name, &hp_allocated, buf, buf_len, &errnop);
#else /* HAVE_GETHOSTBYNAME_R_3_ARG */
	memset((void *) &data, '\0', sizeof(data));
	result = gethostbyname_r(name, &hp_allocated, &data);
	h = (result != 0) ? NULL : &hp_allocated;
#endif
#else /* not HAVE_GETHOSTBYNAME_R */
#ifdef USE_GETHOSTBYNAME_LOCK
	PyThread_acquire_lock(gethostbyname_lock, 1);
#endif
	h = gethostbyname(name);
#endif /* HAVE_GETHOSTBYNAME_R */
	Py_END_ALLOW_THREADS
	/* Some C libraries would require addr.__ss_family instead of addr.ss_family.
           Therefore, we cast the sockaddr_storage into sockaddr to access sa_family. */
	sa = (struct sockaddr*)&addr;
	ret = gethost_common(h, (struct sockaddr *)&addr, sizeof(addr), sa->sa_family);
#ifdef USE_GETHOSTBYNAME_LOCK
	PyThread_release_lock(gethostbyname_lock);
#endif
	return ret;
}

static char ghbn_ex_doc[] =
"gethostbyname_ex(host) -> (name, aliaslist, addresslist)\n\
\n\
Return the true host name, a list of aliases, and a list of IP addresses,\n\
for a host.  The host argument is a string giving a host name or IP number.";


/* Python interface to gethostbyaddr(IP). */

/*ARGSUSED*/
static PyObject *
PySocket_gethostbyaddr(PyObject *self, PyObject *args)
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
	af = PF_UNSPEC;
	if (setipaddr(ip_num, sa, af) < 0)
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
		PyErr_SetString(PySocket_Error, "unsupported address family");
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
	PyThread_acquire_lock(gethostbyname_lock, 1);
#endif
	h = gethostbyaddr(ap, al, af);
#endif /* HAVE_GETHOSTBYNAME_R */
	Py_END_ALLOW_THREADS
	ret = gethost_common(h, (struct sockaddr *)&addr, sizeof(addr), af);
#ifdef USE_GETHOSTBYNAME_LOCK
	PyThread_release_lock(gethostbyname_lock);
#endif
	return ret;
}

static char gethostbyaddr_doc[] =
"gethostbyaddr(host) -> (name, aliaslist, addresslist)\n\
\n\
Return the true host name, a list of aliases, and a list of IP addresses,\n\
for a host.  The host argument is a string giving a host name or IP number.";


/* Python interface to getservbyname(name).
   This only returns the port number, since the other info is already
   known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
PySocket_getservbyname(PyObject *self, PyObject *args)
{
	char *name, *proto;
	struct servent *sp;
	if (!PyArg_ParseTuple(args, "ss:getservbyname", &name, &proto))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	sp = getservbyname(name, proto);
	Py_END_ALLOW_THREADS
	if (sp == NULL) {
		PyErr_SetString(PySocket_Error, "service/proto not found");
		return NULL;
	}
	return PyInt_FromLong((long) ntohs(sp->s_port));
}

static char getservbyname_doc[] =
"getservbyname(servicename, protocolname) -> integer\n\
\n\
Return a port number from a service name and protocol name.\n\
The protocol name should be 'tcp' or 'udp'.";


/* Python interface to getprotobyname(name).
   This only returns the protocol number, since the other info is
   already known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
PySocket_getprotobyname(PyObject *self, PyObject *args)
{
	char *name;
	struct protoent *sp;
#ifdef __BEOS__
/* Not available in BeOS yet. - [cjh] */
	PyErr_SetString( PySocket_Error, "getprotobyname not supported" );
	return NULL;
#else
	if (!PyArg_ParseTuple(args, "s:getprotobyname", &name))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	sp = getprotobyname(name);
	Py_END_ALLOW_THREADS
	if (sp == NULL) {
		PyErr_SetString(PySocket_Error, "protocol not found");
		return NULL;
	}
	return PyInt_FromLong((long) sp->p_proto);
#endif
}

static char getprotobyname_doc[] =
"getprotobyname(name) -> integer\n\
\n\
Return the protocol number for the named protocol.  (Rarely used.)";


#ifndef NO_DUP
/* Create a socket object from a numeric file description.
   Useful e.g. if stdin is a socket.
   Additional arguments as for socket(). */

/*ARGSUSED*/
static PyObject *
PySocket_fromfd(PyObject *self, PyObject *args)
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
		return PySocket_Err();
	s = PySocketSock_New(fd, family, type, proto);
	/* From now on, ignore SIGPIPE and let the error checking
	   do the work. */
#ifdef SIGPIPE
	(void) signal(SIGPIPE, SIG_IGN);
#endif
	return (PyObject *) s;
}

static char fromfd_doc[] =
"fromfd(fd, family, type[, proto]) -> socket object\n\
\n\
Create a socket object from the given file descriptor.\n\
The remaining arguments are the same as for socket().";

#endif /* NO_DUP */


static PyObject *
PySocket_ntohs(PyObject *self, PyObject *args)
{
	int x1, x2;

	if (!PyArg_ParseTuple(args, "i:ntohs", &x1)) {
		return NULL;
	}
	x2 = (int)ntohs((short)x1);
	return PyInt_FromLong(x2);
}

static char ntohs_doc[] =
"ntohs(integer) -> integer\n\
\n\
Convert a 16-bit integer from network to host byte order.";


static PyObject *
PySocket_ntohl(PyObject *self, PyObject *args)
{
	int x1, x2;

	if (!PyArg_ParseTuple(args, "i:ntohl", &x1)) {
		return NULL;
	}
	x2 = ntohl(x1);
	return PyInt_FromLong(x2);
}

static char ntohl_doc[] =
"ntohl(integer) -> integer\n\
\n\
Convert a 32-bit integer from network to host byte order.";


static PyObject *
PySocket_htons(PyObject *self, PyObject *args)
{
	int x1, x2;

	if (!PyArg_ParseTuple(args, "i:htons", &x1)) {
		return NULL;
	}
	x2 = (int)htons((short)x1);
	return PyInt_FromLong(x2);
}

static char htons_doc[] =
"htons(integer) -> integer\n\
\n\
Convert a 16-bit integer from host to network byte order.";


static PyObject *
PySocket_htonl(PyObject *self, PyObject *args)
{
	int x1, x2;

	if (!PyArg_ParseTuple(args, "i:htonl", &x1)) {
		return NULL;
	}
	x2 = htonl(x1);
	return PyInt_FromLong(x2);
}

static char htonl_doc[] =
"htonl(integer) -> integer\n\
\n\
Convert a 32-bit integer from host to network byte order.";

/*
 * socket.inet_aton() and socket.inet_ntoa() functions
 *
 * written 20 Aug 1999 by Ben Gertzfield <che@debian.org> <- blame him!
 *
 */

static char inet_aton_doc[] =
"inet_aton(string) -> packed 32-bit IP representation\n\
\n\
Convert an IP address in string format (123.45.67.89) to the 32-bit packed\n\
binary format used in low-level network functions.";

static PyObject*
PySocket_inet_aton(PyObject *self, PyObject *args)
{
#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif

	/* Have to use inet_addr() instead */
	char *ip_addr;
	unsigned long packed_addr;

	if (!PyArg_ParseTuple(args, "s:inet_aton", &ip_addr)) {
		return NULL;
	}
	packed_addr = inet_addr(ip_addr);

	if (packed_addr == INADDR_NONE) {	/* invalid address */
		PyErr_SetString(PySocket_Error,
			"illegal IP address string passed to inet_aton");
		return NULL;
	}

	return PyString_FromStringAndSize((char *) &packed_addr,
					  sizeof(packed_addr));
}

static char inet_ntoa_doc[] =
"inet_ntoa(packed_ip) -> ip_address_string\n\
\n\
Convert an IP address from 32-bit packed binary format to string format";

static PyObject*
PySocket_inet_ntoa(PyObject *self, PyObject *args)
{
	char *packed_str;
	int addr_len;
	struct in_addr packed_addr;

	if (!PyArg_ParseTuple(args, "s#:inet_ntoa", &packed_str, &addr_len)) {
		return NULL;
	}

	if (addr_len != sizeof(packed_addr)) {
		PyErr_SetString(PySocket_Error,
			"packed IP wrong length for inet_ntoa");
		return NULL;
	}

	memcpy(&packed_addr, packed_str, addr_len);

	return PyString_FromString(inet_ntoa(packed_addr));
}

/* Python interface to getaddrinfo(host, port). */

/*ARGSUSED*/
static PyObject *
PySocket_getaddrinfo(PyObject *self, PyObject *args)
{
	struct addrinfo hints, *res;
	struct addrinfo *res0 = NULL;
	PyObject *pobj = (PyObject *)NULL;
	char pbuf[30];
	char *hptr, *pptr;
	int family, socktype, protocol, flags;
	int error;
	PyObject *all = (PyObject *)NULL;
	PyObject *single = (PyObject *)NULL;

	family = socktype = protocol = flags = 0;
	family = PF_UNSPEC;
	if (!PyArg_ParseTuple(args, "zO|iiii:getaddrinfo",
	    &hptr, &pobj, &family, &socktype,
			&protocol, &flags)) {
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
		PyErr_SetString(PySocket_Error, "Int or String expected");
		return NULL;
	}
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = socktype;
	hints.ai_protocol = protocol;
	hints.ai_flags = flags;
	error = getaddrinfo(hptr, pptr, &hints, &res0);
	if (error) {
		PyGAI_Err(error);
		return NULL;
	}

	if ((all = PyList_New(0)) == NULL)
		goto err;
	for (res = res0; res; res = res->ai_next) {
		PyObject *addr =
			makesockaddr(-1, res->ai_addr, res->ai_addrlen);
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
	return all;
 err:
	Py_XDECREF(single);
	Py_XDECREF(all);
	if (res0)
		freeaddrinfo(res0);
	return (PyObject *)NULL;
}

static char getaddrinfo_doc[] =
"socket.getaddrinfo(host, port [, family, socktype, proto, flags])\n\
	--> List of (family, socktype, proto, canonname, sockaddr)\n\
\n\
Resolve host and port into addrinfo struct.";

/* Python interface to getnameinfo(sa, flags). */

/*ARGSUSED*/
static PyObject *
PySocket_getnameinfo(PyObject *self, PyObject *args)
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
	if  (!PyArg_ParseTuple(sa, "si|ii", &hostp, &port, &flowinfo, &scope_id))
		return NULL;
	PyOS_snprintf(pbuf, sizeof(pbuf), "%d", port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;	/* make numeric port happy */
	error = getaddrinfo(hostp, pbuf, &hints, &res);
	if (error) {
		PyGAI_Err(error);
		goto fail;
	}
	if (res->ai_next) {
		PyErr_SetString(PySocket_Error,
			"sockaddr resolved to multiple addresses");
		goto fail;
	}
	switch (res->ai_family) {
	case AF_INET:
	    {
		char *t1;
		int t2;
		if (PyArg_ParseTuple(sa, "si", &t1, &t2) == 0) {
			PyErr_SetString(PySocket_Error,
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
		PyGAI_Err(error);
		goto fail;
	}
	ret = Py_BuildValue("ss", hbuf, pbuf);

fail:
	if (res)
		freeaddrinfo(res);
	return ret;
}

static char getnameinfo_doc[] =
"socket.getnameinfo(sockaddr, flags) --> (host, port)\n\
\n\
Get host and port for a sockaddr.";

/* List of functions exported by this module. */

static PyMethodDef PySocket_methods[] = {
	{"gethostbyname",	PySocket_gethostbyname,
	 METH_VARARGS, gethostbyname_doc},
	{"gethostbyname_ex",	PySocket_gethostbyname_ex,
	 METH_VARARGS, ghbn_ex_doc},
	{"gethostbyaddr",	PySocket_gethostbyaddr,
	 METH_VARARGS, gethostbyaddr_doc},
	{"gethostname",		PySocket_gethostname,
	 METH_VARARGS, gethostname_doc},
	{"getservbyname",	PySocket_getservbyname,
	 METH_VARARGS, getservbyname_doc},
	{"getprotobyname",	PySocket_getprotobyname,
	 METH_VARARGS,getprotobyname_doc},
#ifndef NO_DUP
	{"fromfd",		PySocket_fromfd,
	 METH_VARARGS, fromfd_doc},
#endif
	{"ntohs",		PySocket_ntohs,
	 METH_VARARGS, ntohs_doc},
	{"ntohl",		PySocket_ntohl,
	 METH_VARARGS, ntohl_doc},
	{"htons",		PySocket_htons,
	 METH_VARARGS, htons_doc},
	{"htonl",		PySocket_htonl,
	 METH_VARARGS, htonl_doc},
	{"inet_aton",		PySocket_inet_aton,
	 METH_VARARGS, inet_aton_doc},
	{"inet_ntoa",		PySocket_inet_ntoa,
	 METH_VARARGS, inet_ntoa_doc},
	{"getaddrinfo",		PySocket_getaddrinfo,
	 METH_VARARGS, getaddrinfo_doc},
	{"getnameinfo",		PySocket_getnameinfo,
	 METH_VARARGS, getnameinfo_doc},
	{NULL,			NULL}		 /* Sentinel */
};


/* Convenience routine to export an integer value.
 *
 * Errors are silently ignored, for better or for worse...
 */
static void
insint(PyObject *d, char *name, int value)
{
	PyObject *v = PyInt_FromLong((long) value);
	if (!v || PyDict_SetItemString(d, name, v))
		PyErr_Clear();

	Py_XDECREF(v);
}


#ifdef MS_WINDOWS

/* Additional initialization and cleanup for NT/Windows */

static void
NTcleanup(void)
{
	WSACleanup();
}

static int
NTinit(void)
{
	WSADATA WSAData;
	int ret;
	char buf[100];
	ret = WSAStartup(0x0101, &WSAData);
	switch (ret) {
	case 0:	/* no error */
		atexit(NTcleanup);
		return 1;
	case WSASYSNOTREADY:
		PyErr_SetString(PyExc_ImportError,
				"WSAStartup failed: network not ready");
		break;
	case WSAVERNOTSUPPORTED:
	case WSAEINVAL:
		PyErr_SetString(PyExc_ImportError,
		    "WSAStartup failed: requested version not supported");
		break;
	default:
		PyOS_snprintf(buf, sizeof(buf),
			      "WSAStartup failed: error code %d", ret);
		PyErr_SetString(PyExc_ImportError, buf);
		break;
	}
	return 0;
}

#endif /* MS_WINDOWS */

#if defined(PYOS_OS2)

/* Additional initialization and cleanup for OS/2 */

static void
OS2cleanup(void)
{
    /* No cleanup is necessary for OS/2 Sockets */
}

static int
OS2init(void)
{
#if !defined(PYCC_GCC)
    char reason[64];
    int rc = sock_init();

    if (rc == 0) {
	    atexit(OS2cleanup);
	    return 1; /* Indicate Success */
    }

    PyOS_snprintf(reason, sizeof(reason),
		  "OS/2 TCP/IP Error# %d", sock_errno());
    PyErr_SetString(PyExc_ImportError, reason);

    return 0;  /* Indicate Failure */
#else
    /* no need to initialise sockets with GCC/EMX */
    return 1;
#endif
}

#endif /* PYOS_OS2 */

/* C API table - always add new things to the end for binary
   compatibility. */
static
PySocketModule_APIObject PySocketModuleAPI =
{
    &PySocketSock_Type,
};

/* Initialize this module.
 *   This is called when the first 'import socket' is done,
 *   via a table in config.c, if config.c is compiled with USE_SOCKET
 *   defined.
 *
 *   For MS_WINDOWS (which means any Windows variant), this module
 *   is actually called "_socket", and there's a wrapper "socket.py"
 *   which implements some missing functionality (such as makefile(),
 *   dup() and fromfd()).  The import of "_socket" may fail with an
 *   ImportError exception if initialization of WINSOCK fails.  When
 *   WINSOCK is initialized succesfully, a call to WSACleanup() is
 *   scheduled to be made at exit time.
 *
 *   For OS/2, this module is also called "_socket" and uses a wrapper
 *   "socket.py" which implements that functionality that is missing
 *   when PC operating systems don't put socket descriptors in the
 *   operating system's filesystem layer.
 */

static char module_doc[] =
"Implementation module for socket operations.  See the socket module\n\
for documentation.";

DL_EXPORT(void)
init_socket(void)
{
	PyObject *m;
#ifdef RISCOS
	_kernel_swi_regs r;
	r.r[0]=0;
	_kernel_swi(0x43380, &r, &r);
	taskwindow = r.r[0];
#else
#ifdef MS_WINDOWS
	if (!NTinit())
		return;
#else
#if defined(PYOS_OS2)
	if (!OS2init())
		return;
#endif /* PYOS_OS2 */
#endif /* MS_WINDOWS */
#endif /* RISCOS */
	PySocketSock_Type.ob_type = &PyType_Type;
	PySocketSock_Type.tp_getattro = PyObject_GenericGetAttr;
	PySocketSock_Type.tp_alloc = PyType_GenericAlloc;
	PySocketSock_Type.tp_free = PyObject_Del;
	m = Py_InitModule3(PySocket_MODULE_NAME, 
			   PySocket_methods, 
			   module_doc);

	PySocket_Error = PyErr_NewException("socket.error", NULL, NULL);
	if (PySocket_Error == NULL)
		return;
	Py_INCREF(PySocket_Error);
	PyModule_AddObject(m, "error", PySocket_Error);
	PyH_Error = PyErr_NewException("socket.herror", PySocket_Error, NULL);
	if (PyH_Error == NULL)
		return;
	Py_INCREF(PyH_Error);
	PyModule_AddObject(m, "herror", PyH_Error);
	PyGAI_Error = PyErr_NewException("socket.gaierror", PySocket_Error,
	    NULL);
	if (PyGAI_Error == NULL)
		return;
	Py_INCREF(PyGAI_Error);
	PyModule_AddObject(m, "gaierror", PyGAI_Error);
	Py_INCREF((PyObject *)&PySocketSock_Type);
	if (PyModule_AddObject(m, "SocketType",
			       (PyObject *)&PySocketSock_Type) != 0)
		return;
	Py_INCREF((PyObject *)&PySocketSock_Type);
	if (PyModule_AddObject(m, "socket",
			       (PyObject *)&PySocketSock_Type) != 0)
		return;

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
#ifdef AF_UNIX
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
	PyModule_AddIntConstant(m, "SOCK_RDM", SOCK_RDM);
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
	PyModule_AddIntConstant(m, "SOMAXCONN", 5);	/* Common value */
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

	/* Initialize gethostbyname lock */
#ifdef USE_GETHOSTBYNAME_LOCK
	gethostbyname_lock = PyThread_allocate_lock();
#endif
}

/* Simplistic emulation code for inet_pton that only works for IPv4 */
#ifndef HAVE_INET_PTON
int 
inet_pton (int af, const char *src, void *dst)
{
	if(af == AF_INET){
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
