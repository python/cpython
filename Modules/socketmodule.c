/* Socket module */

/* SSL support based on patches by Brian E Gallew and Laszlo Kovacs */

/*
This module provides an interface to Berkeley socket IPC.

Limitations:

- only AF_INET and AF_UNIX address families are supported
- no read/write operations (use send/recv or makefile instead)
- additional restrictions apply on Windows

Module interface:

- socket.error: exception raised for socket specific errors
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
- socket.AF_INET, socket.SOCK_STREAM, etc.: constants from <socket.h>
- socket.inet_aton(IP address) -> 32-bit packed IP representation
- socket.inet_ntoa(packed IP) -> IP address string
- socket.ssl(socket, keyfile, certfile) -> new ssl object
- an Internet socket address is a pair (hostname, port)
  where hostname can be anything recognized by gethostbyname()
  (including the dd.dd.dd.dd notation) and port is in host byte order
- where a hostname is returned, the dd.dd.dd.dd notation is used
- a UNIX domain socket address is a string specifying the pathname

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
- s.sendto(string, [flags,] sockaddr) --> nbytes
- s.setblocking(0 | 1) --> None
- s.setsockopt(level, optname, value) --> None
- s.shutdown(how) --> None
- repr(s) --> "<socket object, fd=%d, family=%d, type=%d, protocol=%d>"

*/

#include "Python.h"

/* Hacks for gethostbyname_r().  On some non-Linux platforms, the configure
   script doesn't get this right, so we hardcode some platform checks below.
   On the other hand, not all Linux versions agree, so there the settings
   computed by the configure script are needed! */

#ifndef linux
#undef HAVE_GETHOSTBYNAME_R_3_ARG
#undef HAVE_GETHOSTBYNAME_R_5_ARG
#undef HAVE_GETHOSTBYNAME_R_6_ARG
#endif

#ifndef WITH_THREAD
#undef HAVE_GETHOSTBYNAME_R
#endif

#ifdef HAVE_GETHOSTBYNAME_R
#if defined(_AIX) || defined(__osf__)
#define HAVE_GETHOSTBYNAME_R_3_ARG
#elif defined(__sun__) || defined(__sgi)
#define HAVE_GETHOSTBYNAME_R_5_ARG
#elif defined(linux)
/* Rely on the configure script */
#else
#undef HAVE_GETHOSTBYNAME_R
#endif
#endif

#if !defined(HAVE_GETHOSTBYNAME_R) && defined(WITH_THREAD) && !defined(MS_WINDOWS)
#define USE_GETHOSTBYNAME_LOCK
#endif

#ifdef USE_GETHOSTBYNAME_LOCK
#include "pythread.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(PYCC_VACPP)
#include <types.h>
#include <io.h>
#include <sys/ioctl.h>
#include <utils.h>
#include <ctype.h>
#endif

#if defined(PYOS_OS2)
#define  INCL_DOS
#define  INCL_DOSERRORS
#define  INCL_NOPMAPI
#include <os2.h>
#endif

#include <sys/types.h>

#include <signal.h>
#ifndef MS_WINDOWS
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#if !(defined(__BEOS__) || defined(__CYGWIN__))
#include <netinet/tcp.h>
#endif

/* Headers needed for inet_ntoa() and inet_addr() */
#ifdef __BEOS__
#include <net/netdb.h>
#else
#ifndef USE_GUSI1
#include <arpa/inet.h>
#endif
#endif

#include <fcntl.h>
#else
#include <winsock.h>
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#else
#undef AF_UNIX
#endif

#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK	/* For QNX only? */
#endif

#ifdef USE_GUSI1
/* fdopen() isn't declared in stdio.h (sigh) */
#include <GUSI.h>
#endif

#ifdef USE_SSL
#include "openssl/rsa.h"
#include "openssl/crypto.h"
#include "openssl/x509.h"
#include "openssl/pem.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#endif /* USE_SSL */

#if defined(MS_WINDOWS) || defined(__BEOS__)
/* BeOS suffers from the same socket dichotomy as Win32... - [cjh] */
/* seem to be a few differences in the API */
#define SOCKETCLOSE closesocket
#define NO_DUP /* Actually it exists on NT 3.5, but what the heck... */
#endif

/* abstract the socket file descriptor type */
#ifdef MS_WINDOWS
typedef SOCKET SOCKET_T;
#	ifdef MS_WIN64
#		define SIZEOF_SOCKET_T 8
#	else
#		define SIZEOF_SOCKET_T 4
#	endif
#else
typedef int SOCKET_T;
#	define SIZEOF_SOCKET_T SIZEOF_INT
#endif


#if defined(PYOS_OS2)
#define SOCKETCLOSE soclose
#define NO_DUP /* Sockets are Not Actual File Handles under OS/2 */
#endif

#ifndef SOCKETCLOSE
#define SOCKETCLOSE close
#endif

/* Global variable holding the exception type for errors detected
   by this module (but not argument type or memory errors, etc.). */

static PyObject *PySocket_Error;

#ifdef USE_SSL
static PyObject *SSLErrorObject;
#endif /* USE_SSL */


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

#if defined(PYOS_OS2)
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


/* The object holding a socket.  It holds some extra information,
   like the address family, which is used to decode socket address
   arguments properly. */

typedef struct {
	PyObject_HEAD
	SOCKET_T sock_fd;		/* Socket file descriptor */
	int sock_family;	/* Address family, e.g., AF_INET */
	int sock_type;		/* Socket type, e.g., SOCK_STREAM */
	int sock_proto;		/* Protocol type, usually 0 */
	union sock_addr {
		struct sockaddr_in in;
#ifdef AF_UNIX
		struct sockaddr_un un;
#endif
	} sock_addr;
} PySocketSockObject;

#ifdef USE_SSL

typedef struct {
	PyObject_HEAD
	PySocketSockObject *Socket;	/* Socket on which we're layered */
	PyObject 	*x_attr;	/* Attributes dictionary */
	SSL_CTX* 	ctx;
	SSL*     	ssl;
	X509*    	server_cert;
	BIO*		sbio;
	char    	server[256];
	char		issuer[256];

} SSLObject;

staticforward PyTypeObject SSL_Type;
staticforward PyObject *SSL_SSLwrite(SSLObject *self, PyObject *args);
staticforward PyObject *SSL_SSLread(SSLObject *self, PyObject *args);

#define SSLObject_Check(v)	((v)->ob_type == &SSL_Type)

#endif /* USE_SSL */
 
/* A forward reference to the Socktype type object.
   The Socktype variable contains pointers to various functions,
   some of which call newsockobject(), which uses Socktype, so
   there has to be a circular reference. */

staticforward PyTypeObject PySocketSock_Type;


/* Create a new socket object.
   This just creates the object and initializes it.
   If the creation fails, return NULL and set an exception (implicit
   in NEWOBJ()). */

static PySocketSockObject *
PySocketSock_New(SOCKET_T fd, int family, int type, int proto)
{
	PySocketSockObject *s;
	PySocketSock_Type.ob_type = &PyType_Type;
	s = PyObject_New(PySocketSockObject, &PySocketSock_Type);
	if (s != NULL) {
		s->sock_fd = fd;
		s->sock_family = family;
		s->sock_type = type;
		s->sock_proto = proto;
	}
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
   Return the length (should always be 4 bytes), or negative if
   an error occurred; then an exception is raised. */

static int
setipaddr(char* name, struct sockaddr_in * addr_ret)
{
	struct hostent *hp;
	int d1, d2, d3, d4;
	int h_length;
	char ch;
#ifdef HAVE_GETHOSTBYNAME_R
	struct hostent hp_allocated;
#ifdef HAVE_GETHOSTBYNAME_R_3_ARG
	struct hostent_data data;
#else
	char buf[1001];
	int buf_len = (sizeof buf) - 1;
	int errnop;
#endif
#if defined(HAVE_GETHOSTBYNAME_R_3_ARG) || defined(HAVE_GETHOSTBYNAME_R_6_ARG)
	int result;
#endif
#endif /* HAVE_GETHOSTBYNAME_R */

	memset((void *) addr_ret, '\0', sizeof(*addr_ret));
	if (name[0] == '\0') {
		addr_ret->sin_addr.s_addr = INADDR_ANY;
		return 4;
	}
	if (name[0] == '<' && strcmp(name, "<broadcast>") == 0) {
		addr_ret->sin_addr.s_addr = INADDR_BROADCAST;
		return 4;
	}
	if (sscanf(name, "%d.%d.%d.%d%c", &d1, &d2, &d3, &d4, &ch) == 4 &&
	    0 <= d1 && d1 <= 255 && 0 <= d2 && d2 <= 255 &&
	    0 <= d3 && d3 <= 255 && 0 <= d4 && d4 <= 255) {
		addr_ret->sin_addr.s_addr = htonl(
			((long) d1 << 24) | ((long) d2 << 16) |
			((long) d3 << 8) | ((long) d4 << 0));
		return 4;
	}
	Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_GETHOSTBYNAME_R
#if    defined(HAVE_GETHOSTBYNAME_R_6_ARG)
	result = gethostbyname_r(name, &hp_allocated, buf, buf_len, &hp, &errnop);
#elif  defined(HAVE_GETHOSTBYNAME_R_5_ARG)
	hp = gethostbyname_r(name, &hp_allocated, buf, buf_len, &errnop);
#else  /* HAVE_GETHOSTBYNAME_R_3_ARG */
	memset((void *) &data, '\0', sizeof(data));
	result = gethostbyname_r(name, &hp_allocated, &data);
	hp = (result != 0) ? NULL : &hp_allocated;
#endif
#else /* not HAVE_GETHOSTBYNAME_R */
#ifdef USE_GETHOSTBYNAME_LOCK
	PyThread_acquire_lock(gethostbyname_lock, 1);
#endif
	hp = gethostbyname(name);
#endif /* HAVE_GETHOSTBYNAME_R */
	Py_END_ALLOW_THREADS

	if (hp == NULL) {
#ifdef HAVE_HSTRERROR
	        /* Let's get real error message to return */
	        extern int h_errno;
		PyErr_SetString(PySocket_Error, (char *)hstrerror(h_errno));
#else
		PyErr_SetString(PySocket_Error, "host not found");
#endif
#ifdef USE_GETHOSTBYNAME_LOCK
		PyThread_release_lock(gethostbyname_lock);
#endif
		return -1;
	}
	memcpy((char *) &addr_ret->sin_addr, hp->h_addr, hp->h_length);
	h_length = hp->h_length;
#ifdef USE_GETHOSTBYNAME_LOCK
	PyThread_release_lock(gethostbyname_lock);
#endif
	return h_length;
}


/* Create a string object representing an IP address.
   This is always a string of the form 'dd.dd.dd.dd' (with variable
   size numbers). */

static PyObject *
makeipaddr(struct sockaddr_in *addr)
{
	long x = ntohl(addr->sin_addr.s_addr);
	char buf[100];
	sprintf(buf, "%d.%d.%d.%d",
		(int) (x>>24) & 0xff, (int) (x>>16) & 0xff,
		(int) (x>> 8) & 0xff, (int) (x>> 0) & 0xff);
	return PyString_FromString(buf);
}


/* Create an object representing the given socket address,
   suitable for passing it back to bind(), connect() etc.
   The family field of the sockaddr structure is inspected
   to determine what kind of address it really is. */

/*ARGSUSED*/
static PyObject *
makesockaddr(struct sockaddr *addr, int addrlen)
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
		struct sockaddr_in *a = (struct sockaddr_in *) addr;
		PyObject *addrobj = makeipaddr(a);
		PyObject *ret = NULL;
		if (addrobj) {
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
getsockaddrarg(PySocketSockObject *s, PyObject *args, struct sockaddr **addr_ret, int *len_ret)
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
		addr->sun_family = AF_UNIX;
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
		if (!PyArg_Parse(args, "(si)", &host, &port))
			return 0;
		if (setipaddr(host, addr) < 0)
			return 0;
		addr->sin_family = AF_INET;
		addr->sin_port = htons((short)port);
		*addr_ret = (struct sockaddr *) addr;
		*len_ret = sizeof *addr;
		return 1;
	}

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

	/* More cases here... */

	default:
		PyErr_SetString(PySocket_Error, "getsockaddrlen: bad family");
		return 0;

	}
}


/* s.accept() method */

static PyObject *
PySocketSock_accept(PySocketSockObject *s, PyObject *args)
{
	char addrbuf[256];
	SOCKET_T newfd;
	socklen_t addrlen;
	PyObject *sock = NULL;
	PyObject *addr = NULL;
	PyObject *res = NULL;

	if (!PyArg_ParseTuple(args, ":accept"))
		return NULL;
	if (!getsockaddrlen(s, &addrlen))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	newfd = accept(s->sock_fd, (struct sockaddr *) addrbuf, &addrlen);
	Py_END_ALLOW_THREADS
#ifdef MS_WINDOWS
	if (newfd == INVALID_SOCKET)
#else
	if (newfd < 0)
#endif
		return PySocket_Err();

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
	if (!(addr = makesockaddr((struct sockaddr *) addrbuf, addrlen)))
		goto finally;

	if (!(res = Py_BuildValue("OO", sock, addr)))
		goto finally;

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
PySocketSock_setblocking(PySocketSockObject *s, PyObject *args)
{
	int block;
#ifndef MS_WINDOWS
	int delay_flag;
#endif
	if (!PyArg_ParseTuple(args, "i:setblocking", &block))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
#ifdef __BEOS__
	block = !block;
	setsockopt( s->sock_fd, SOL_SOCKET, SO_NONBLOCK,
				(void *)(&block), sizeof( int ) );
#else
#ifndef MS_WINDOWS
#ifdef PYOS_OS2
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
	Py_END_ALLOW_THREADS

	Py_INCREF(Py_None);
	return Py_None;
}

static char setblocking_doc[] =
"setblocking(flag)\n\
\n\
Set the socket to blocking (flag is true) or non-blocking (false).\n\
This uses the FIONBIO ioctl with the O_NDELAY flag.";


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
		return PySocket_Err();
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
	PyErr_SetString( PySocket_Error, "getsockopt not supported" );
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
			return PySocket_Err();
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
			 (void *)PyString_AsString(buf), &buflen);
	if (res < 0) {
		Py_DECREF(buf);
		return PySocket_Err();
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
PySocketSock_bind(PySocketSockObject *s, PyObject *args)
{
	struct sockaddr *addr;
	int addrlen;
	int res;
	PyObject *addro;
	if (!PyArg_ParseTuple(args, "O:bind", &addro))
		return NULL;
	if (!getsockaddrarg(s, addro, &addr, &addrlen))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = bind(s->sock_fd, addr, addrlen);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return PySocket_Err();
	Py_INCREF(Py_None);
	return Py_None;
}

static char bind_doc[] =
"bind(address)\n\
\n\
Bind the socket to a local address.  For IP sockets, the address is a\n\
pair (host, port); the host must refer to the local host.";


/* s.close() method.
   Set the file descriptor to -1 so operations tried subsequently
   will surely fail. */

static PyObject *
PySocketSock_close(PySocketSockObject *s, PyObject *args)
{
	SOCKET_T fd;
	if (!PyArg_ParseTuple(args, ":close"))
		return NULL;
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
PySocketSock_connect(PySocketSockObject *s, PyObject *args)
{
	struct sockaddr *addr;
	int addrlen;
	int res;
	PyObject *addro;
	if (!PyArg_ParseTuple(args, "O:connect", &addro))
		return NULL;
	if (!getsockaddrarg(s, addro, &addr, &addrlen))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = connect(s->sock_fd, addr, addrlen);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return PySocket_Err();
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
PySocketSock_connect_ex(PySocketSockObject *s, PyObject *args)
{
	struct sockaddr *addr;
	int addrlen;
	int res;
	PyObject *addro;
	if (!PyArg_ParseTuple(args, "O:connect_ex", &addro))
		return NULL;
	if (!getsockaddrarg(s, addro, &addr, &addrlen))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = connect(s->sock_fd, addr, addrlen);
	Py_END_ALLOW_THREADS
	if (res != 0)
		res = errno;
	return PyInt_FromLong((long) res);
}

static char connect_ex_doc[] =
"connect_ex(address)\n\
\n\
This is like connect(address), but returns an error code (the errno value)\n\
instead of raising an exception when an error occurs.";


/* s.fileno() method */

static PyObject *
PySocketSock_fileno(PySocketSockObject *s, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":fileno"))
		return NULL;
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
PySocketSock_dup(PySocketSockObject *s, PyObject *args)
{
	SOCKET_T newfd;
	PyObject *sock;
	if (!PyArg_ParseTuple(args, ":dup"))
		return NULL;
	newfd = dup(s->sock_fd);
	if (newfd < 0)
		return PySocket_Err();
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
PySocketSock_getsockname(PySocketSockObject *s, PyObject *args)
{
	char addrbuf[256];
	int res;
	socklen_t addrlen;

	if (!PyArg_ParseTuple(args, ":getsockname"))
		return NULL;
	if (!getsockaddrlen(s, &addrlen))
		return NULL;
	memset(addrbuf, 0, addrlen);
	Py_BEGIN_ALLOW_THREADS
	res = getsockname(s->sock_fd, (struct sockaddr *) addrbuf, &addrlen);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return PySocket_Err();
	return makesockaddr((struct sockaddr *) addrbuf, addrlen);
}

static char getsockname_doc[] =
"getsockname() -> address info\n\
\n\
Return the address of the local endpoint.  For IP sockets, the address\n\
info is a pair (hostaddr, port).";


#ifdef HAVE_GETPEERNAME		/* Cray APP doesn't have this :-( */
/* s.getpeername() method */

static PyObject *
PySocketSock_getpeername(PySocketSockObject *s, PyObject *args)
{
	char addrbuf[256];
	int res;
	socklen_t addrlen;

	if (!PyArg_ParseTuple(args, ":getpeername"))
		return NULL;
	if (!getsockaddrlen(s, &addrlen))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = getpeername(s->sock_fd, (struct sockaddr *) addrbuf, &addrlen);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return PySocket_Err();
	return makesockaddr((struct sockaddr *) addrbuf, addrlen);
}

static char getpeername_doc[] =
"getpeername() -> address info\n\
\n\
Return the address of the remote endpoint.  For IP sockets, the address\n\
info is a pair (hostaddr, port).";

#endif /* HAVE_GETPEERNAME */


/* s.listen(n) method */

static PyObject *
PySocketSock_listen(PySocketSockObject *s, PyObject *args)
{
	int backlog;
	int res;
	if (!PyArg_ParseTuple(args, "i:listen", &backlog))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	if (backlog < 1)
		backlog = 1;
	res = listen(s->sock_fd, backlog);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return PySocket_Err();
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
	intptr_t fd;
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
		return PySocket_Err();
	}
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
	buf = PyString_FromStringAndSize((char *) 0, len);
	if (buf == NULL)
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	n = recv(s->sock_fd, PyString_AsString(buf), len, flags);
	Py_END_ALLOW_THREADS
	if (n < 0) {
		Py_DECREF(buf);
		return PySocket_Err();
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
	n = recvfrom(s->sock_fd, PyString_AsString(buf), len, flags,
#ifndef MS_WINDOWS
#if defined(PYOS_OS2)
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
		return PySocket_Err();
	}
	if (n != len && _PyString_Resize(&buf, n) < 0)
		return NULL;
		
	if (!(addr = makesockaddr((struct sockaddr *)addrbuf, addrlen)))
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
		return PySocket_Err();
	return PyInt_FromLong((long)n);
}

static char send_doc[] =
"send(data[, flags])\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.";


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
		return PySocket_Err();
	return PyInt_FromLong((long)n);
}

static char sendto_doc[] =
"sendto(data[, flags], address)\n\
\n\
Like send(data, flags) but allows specifying the destination address.\n\
For IP sockets, the address is a pair (hostaddr, port).";


/* s.shutdown(how) method */

static PyObject *
PySocketSock_shutdown(PySocketSockObject *s, PyObject *args)
{
	int how;
	int res;
	if (!PyArg_ParseTuple(args, "i:shutdown", &how))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = shutdown(s->sock_fd, how);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return PySocket_Err();
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
	{"accept",		(PyCFunction)PySocketSock_accept, METH_VARARGS,
				accept_doc},
	{"bind",		(PyCFunction)PySocketSock_bind, METH_VARARGS,
				bind_doc},
	{"close",		(PyCFunction)PySocketSock_close, METH_VARARGS,
				close_doc},
	{"connect",		(PyCFunction)PySocketSock_connect, METH_VARARGS,
				connect_doc},
	{"connect_ex",		(PyCFunction)PySocketSock_connect_ex, METH_VARARGS,
				connect_ex_doc},
#ifndef NO_DUP
	{"dup",			(PyCFunction)PySocketSock_dup, METH_VARARGS,
				dup_doc},
#endif
	{"fileno",		(PyCFunction)PySocketSock_fileno, METH_VARARGS,
				fileno_doc},
#ifdef HAVE_GETPEERNAME
	{"getpeername",		(PyCFunction)PySocketSock_getpeername, METH_VARARGS,
				getpeername_doc},
#endif
	{"getsockname",		(PyCFunction)PySocketSock_getsockname, METH_VARARGS,
				getsockname_doc},
	{"getsockopt",		(PyCFunction)PySocketSock_getsockopt, METH_VARARGS,
				getsockopt_doc},
	{"listen",		(PyCFunction)PySocketSock_listen, METH_VARARGS,
				listen_doc},
#ifndef NO_DUP
	{"makefile",		(PyCFunction)PySocketSock_makefile, METH_VARARGS,
				makefile_doc},
#endif
	{"recv",		(PyCFunction)PySocketSock_recv, METH_VARARGS,
				recv_doc},
	{"recvfrom",		(PyCFunction)PySocketSock_recvfrom, METH_VARARGS,
				recvfrom_doc},
	{"send",		(PyCFunction)PySocketSock_send, METH_VARARGS,
				send_doc},
	{"sendto",		(PyCFunction)PySocketSock_sendto, METH_VARARGS,
				sendto_doc},
	{"setblocking",		(PyCFunction)PySocketSock_setblocking, METH_VARARGS,
				setblocking_doc},
	{"setsockopt",		(PyCFunction)PySocketSock_setsockopt, METH_VARARGS,
				setsockopt_doc},
	{"shutdown",		(PyCFunction)PySocketSock_shutdown, METH_VARARGS,
				shutdown_doc},
	{NULL,			NULL}		/* sentinel */
};


/* Deallocate a socket object in response to the last Py_DECREF().
   First close the file description. */

static void
PySocketSock_dealloc(PySocketSockObject *s)
{
	if (s->sock_fd != -1)
		(void) SOCKETCLOSE(s->sock_fd);
	PyObject_Del(s);
}


/* Return a socket object's named attribute. */

static PyObject *
PySocketSock_getattr(PySocketSockObject *s, char *name)
{
	return Py_FindMethod(PySocketSock_methods, (PyObject *) s, name);
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
	sprintf(buf, 
		"<socket object, fd=%ld, family=%d, type=%d, protocol=%d>", 
		(long)s->sock_fd, s->sock_family, s->sock_type, s->sock_proto);
	return PyString_FromString(buf);
}


/* Type object for socket objects. */

static PyTypeObject PySocketSock_Type = {
	PyObject_HEAD_INIT(0)	/* Must fill in type value later */
	0,
	"socket",
	sizeof(PySocketSockObject),
	0,
	(destructor)PySocketSock_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	(getattrfunc)PySocketSock_getattr, /*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	(reprfunc)PySocketSock_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
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
	struct sockaddr_in addrbuf;
	if (!PyArg_ParseTuple(args, "s:gethostbyname", &name))
		return NULL;
	if (setipaddr(name, &addrbuf) < 0)
		return NULL;
	return makeipaddr(&addrbuf);
}

static char gethostbyname_doc[] =
"gethostbyname(host) -> address\n\
\n\
Return the IP address (a string of the form '255.255.255.255') for a host.";


/* Convenience function common to gethostbyname_ex and gethostbyaddr */

static PyObject *
gethost_common(struct hostent *h, struct sockaddr_in *addr)
{
	char **pch;
	PyObject *rtn_tuple = (PyObject *)NULL;
	PyObject *name_list = (PyObject *)NULL;
	PyObject *addr_list = (PyObject *)NULL;
	PyObject *tmp;
	if (h == NULL) {
#ifdef HAVE_HSTRERROR
	        /* Let's get real error message to return */
	        extern int h_errno;
		PyErr_SetString(PySocket_Error, (char *)hstrerror(h_errno));
#else
		PyErr_SetString(PySocket_Error, "host not found");
#endif
		return NULL;
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
		memcpy((char *) &addr->sin_addr, *pch, h->h_length);
		tmp = makeipaddr(addr);
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
	struct sockaddr_in addr;
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
	if (setipaddr(name, &addr) < 0)
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
	ret = gethost_common(h, &addr);
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
        struct sockaddr_in addr;
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

	if (!PyArg_ParseTuple(args, "s:gethostbyaddr", &ip_num))
		return NULL;
	if (setipaddr(ip_num, &addr) < 0)
		return NULL;
	Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_GETHOSTBYNAME_R
#if   defined(HAVE_GETHOSTBYNAME_R_6_ARG)
	result = gethostbyaddr_r((char *)&addr.sin_addr,
		sizeof(addr.sin_addr),
		AF_INET, &hp_allocated, buf, buf_len,
		&h, &errnop);
#elif defined(HAVE_GETHOSTBYNAME_R_5_ARG)
	h = gethostbyaddr_r((char *)&addr.sin_addr,
			    sizeof(addr.sin_addr),
			    AF_INET, 
			    &hp_allocated, buf, buf_len, &errnop);
#else /* HAVE_GETHOSTBYNAME_R_3_ARG */
	memset((void *) &data, '\0', sizeof(data));
	result = gethostbyaddr_r((char *)&addr.sin_addr,
		sizeof(addr.sin_addr),
		AF_INET, &hp_allocated, &data);
	h = (result != 0) ? NULL : &hp_allocated;
#endif
#else /* not HAVE_GETHOSTBYNAME_R */
#ifdef USE_GETHOSTBYNAME_LOCK
	PyThread_acquire_lock(gethostbyname_lock, 1);
#endif
	h = gethostbyaddr((char *)&addr.sin_addr,
			  sizeof(addr.sin_addr),
			  AF_INET);
#endif /* HAVE_GETHOSTBYNAME_R */
	Py_END_ALLOW_THREADS
	ret = gethost_common(h, &addr);
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


/* Python interface to socket(family, type, proto).
   The third (protocol) argument is optional.
   Return a new socket object. */

/*ARGSUSED*/
static PyObject *
PySocket_socket(PyObject *self, PyObject *args)
{
	PySocketSockObject *s;
	SOCKET_T fd;
	int family, type, proto = 0;
	if (!PyArg_ParseTuple(args, "ii|i:socket", &family, &type, &proto))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	fd = socket(family, type, proto);
	Py_END_ALLOW_THREADS
#ifdef MS_WINDOWS
	if (fd == INVALID_SOCKET)
#else
	if (fd < 0)
#endif
		return PySocket_Err();
	s = PySocketSock_New(fd, family, type, proto);
	/* If the object can't be created, don't forget to close the
	   file descriptor again! */
	if (s == NULL)
		(void) SOCKETCLOSE(fd);
	/* From now on, ignore SIGPIPE and let the error checking
	   do the work. */
#ifdef SIGPIPE      
	(void) signal(SIGPIPE, SIG_IGN);
#endif   
	return (PyObject *) s;
}

static char socket_doc[] =
"socket(family, type[, proto]) -> socket object\n\
\n\
Open a socket of the given type.  The family argument specifies the\n\
address family; it is normally AF_INET, sometimes AF_UNIX.\n\
The type argument specifies whether this is a stream (SOCK_STREAM)\n\
or datagram (SOCK_DGRAM) socket.  The protocol argument defaults to 0,\n\
specifying the default protocol.";


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
	long packed_addr;

	if (!PyArg_ParseTuple(args, "s:inet_aton", &ip_addr)) {
		return NULL;
	}
#ifdef USE_GUSI1
	packed_addr = (long)inet_addr(ip_addr).s_addr;
#else
	packed_addr = inet_addr(ip_addr);
#endif

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


#ifdef USE_SSL

/* This is a C function to be called for new object initialization */
static SSLObject *
newSSLObject(PySocketSockObject *Sock, char *key_file, char *cert_file)
{
	SSLObject *self;

	self = PyObject_New(SSLObject, &SSL_Type); /* Create new object */
	if (self == NULL){
		PyErr_SetObject(SSLErrorObject,
				PyString_FromString("newSSLObject error"));
		return NULL;
	}
	memset(self->server, '\0', sizeof(char) * 256);
	memset(self->issuer, '\0', sizeof(char) * 256);  
  
	self->x_attr = PyDict_New();
	self->ctx = SSL_CTX_new(SSLv23_method()); /* Set up context */
	if (self->ctx == NULL) {
		PyErr_SetObject(SSLErrorObject,
				PyString_FromString("SSL_CTX_new error"));
		PyObject_Del(self);
		return NULL;
	}

	if ( (key_file && !cert_file) || (!key_file && cert_file) )
	{
		PyErr_SetObject(SSLErrorObject,
		      PyString_FromString(
			"Both the key & certificate files must be specified"));
		PyObject_Del(self);
		return NULL;
	}

	if (key_file && cert_file)
	{
		if (SSL_CTX_use_PrivateKey_file(self->ctx, key_file,
						SSL_FILETYPE_PEM) < 1)
		{
			PyErr_SetObject(SSLErrorObject,
				PyString_FromString(
				  "SSL_CTX_use_PrivateKey_file error"));
			PyObject_Del(self);
			return NULL;
		}

		if (SSL_CTX_use_certificate_chain_file(self->ctx,
						       cert_file) < 1)
		{
			PyErr_SetObject(SSLErrorObject,
				PyString_FromString(
				  "SSL_CTX_use_certificate_chain_file error"));
			PyObject_Del(self);
			return NULL;
		}
	}

	SSL_CTX_set_verify(self->ctx,
			   SSL_VERIFY_NONE, NULL); /* set verify lvl */
	self->ssl = SSL_new(self->ctx); /* New ssl struct */
	SSL_set_fd(self->ssl, Sock->sock_fd);	/* Set the socket for SSL */
	SSL_set_connect_state(self->ssl);

	if ((SSL_connect(self->ssl)) == -1) {
		/* Actually negotiate SSL connection */
		PyErr_SetObject(SSLErrorObject,
				PyString_FromString("SSL_connect error"));
		PyObject_Del(self);
		return NULL;
	}
	self->ssl->debug = 1;

	if ((self->server_cert = SSL_get_peer_certificate(self->ssl))) {
		X509_NAME_oneline(X509_get_subject_name(self->server_cert),
				  self->server, 256);
		X509_NAME_oneline(X509_get_issuer_name(self->server_cert),
				  self->issuer, 256);
	}
	self->x_attr = NULL;
	self->Socket = Sock;
	Py_INCREF(self->Socket);
	return self;
}

/* This is the Python function called for new object initialization */
static PyObject *
PySocket_ssl(PyObject *self, PyObject *args)
{
	SSLObject *rv;
	PySocketSockObject *Sock;
	char *key_file;
	char *cert_file;
  
	if (!PyArg_ParseTuple(args, "O!zz:ssl",
			      &PySocketSock_Type, (PyObject*)&Sock,
			      &key_file, &cert_file) )
		return NULL;
  
	rv = newSSLObject(Sock, key_file, cert_file);
	if ( rv == NULL )
		return NULL;
	return (PyObject *)rv;
}

static char ssl_doc[] =
"ssl(socket, keyfile, certfile) -> sslobject";

static PyObject *
SSL_server(SSLObject *self, PyObject *args)
{
	return PyString_FromString(self->server);
}

static PyObject *
SSL_issuer(SSLObject *self, PyObject *args)
{
	return PyString_FromString(self->issuer);
}


/* SSL object methods */

static PyMethodDef SSLMethods[] = {
	{ "write", (PyCFunction)SSL_SSLwrite, 1 },
	{ "read", (PyCFunction)SSL_SSLread, 1 },
	{ "server", (PyCFunction)SSL_server, 1 },
	{ "issuer", (PyCFunction)SSL_issuer, 1 },
	{ NULL, NULL}
};

static void SSL_dealloc(SSLObject *self)
{
	if (self->server_cert)	/* Possible not to have one? */
		X509_free (self->server_cert);
	SSL_CTX_free(self->ctx);
	SSL_free(self->ssl);
	Py_XDECREF(self->x_attr);
	Py_XDECREF(self->Socket);
	PyObject_Del(self);
}

static PyObject *SSL_getattr(SSLObject *self, char *name)
{
	return Py_FindMethod(SSLMethods, (PyObject *)self, name);
}

staticforward PyTypeObject SSL_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"SSL",			/*tp_name*/
	sizeof(SSLObject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)SSL_dealloc,	/*tp_dealloc*/
	0,				/*tp_print*/
	(getattrfunc)SSL_getattr,	/*tp_getattr*/
	0,				/*tp_setattr*/
	0,				/*tp_compare*/
	0,				/*tp_repr*/
	0,				/*tp_as_number*/
	0,				/*tp_as_sequence*/
	0,				/*tp_as_mapping*/
	0,				/*tp_hash*/
};



static PyObject *SSL_SSLwrite(SSLObject *self, PyObject *args)
{
	char *data;
	size_t len = 0;
  
	if (!PyArg_ParseTuple(args, "s#|i:write", &data, &len))
		return NULL;
  
	if (!len)
		len = strlen(data);
  
	len = SSL_write(self->ssl, data, len);
	return PyInt_FromLong((long)len);
}

static PyObject *SSL_SSLread(SSLObject *self, PyObject *args)
{
	PyObject *buf;
	int count = 0;
	int len = 1024;
	int res;
  
	PyArg_ParseTuple(args, "|i:read", &len);
  
	if (!(buf = PyString_FromStringAndSize((char *) 0, len)))
		return NULL;	/* Error object should already be set */
  
	count = SSL_read(self->ssl, PyString_AsString(buf), len);
	res = SSL_get_error(self->ssl, count);

	switch (res) {
	case 0:			/* Good return value! */
		break;
	case 6:
		PyErr_SetString(SSLErrorObject, "EOF");
		Py_DECREF(buf);
		return NULL;
		break;
	case 5:
	default:
		return PyErr_SetFromErrno(SSLErrorObject);
		break;
	}
  
	fflush(stderr);
	  
	if (count < 0) {
		Py_DECREF(buf);
		return PyErr_SetFromErrno(SSLErrorObject);
	}
  
	if (count != len && _PyString_Resize(&buf, count) < 0)
		return NULL;
	return buf;
}

#endif /* USE_SSL */


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
	{"socket",		PySocket_socket, 
	 METH_VARARGS, socket_doc},
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
#ifdef USE_SSL
	{"ssl",			PySocket_ssl, 
	 METH_VARARGS, ssl_doc},
#endif /* USE_SSL */
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
		sprintf(buf, "WSAStartup failed: error code %d", ret);
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
    char reason[64];
    int rc = sock_init();

    if (rc == 0) {
	    atexit(OS2cleanup);
	    return 1; /* Indicate Success */
    }

    sprintf(reason, "OS/2 TCP/IP Error# %d", sock_errno());
    PyErr_SetString(PyExc_ImportError, reason);

    return 0;  /* Indicate Failure */
}

#endif /* PYOS_OS2 */

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

static char sockettype_doc[] =
"A socket represents one endpoint of a network connection.\n\
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
makefile() -- return a file object corresponding tot the socket (*)\n\
recv() -- receive data\n\
recvfrom() -- receive data and sender's address\n\
send() -- send data\n\
sendto() -- send data to a given address\n\
setblocking() -- set or clear the blocking I/O flag\n\
setsockopt() -- set socket options\n\
shutdown() -- shut down traffic in one or both directions\n\
\n\
(*) not available on all platforms!)";

DL_EXPORT(void)
init_socket(void)
{
	PyObject *m, *d;
#ifdef MS_WINDOWS
	if (!NTinit())
		return;
#else
#if defined(__TOS_OS2__)
	if (!OS2init())
		return;
#endif /* __TOS_OS2__ */
#endif /* MS_WINDOWS */
	m = Py_InitModule3("_socket", PySocket_methods, module_doc);
	d = PyModule_GetDict(m);
	PySocket_Error = PyErr_NewException("socket.error", NULL, NULL);
	if (PySocket_Error == NULL)
		return;
#ifdef USE_SSL
	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
	SSLErrorObject = PyErr_NewException("socket.sslerror", NULL, NULL);
	if (SSLErrorObject == NULL)
		return;
	PyDict_SetItemString(d, "sslerror", SSLErrorObject);
	Py_INCREF(&SSL_Type);
	if (PyDict_SetItemString(d, "SSLType",
				 (PyObject *)&SSL_Type) != 0)
		return;
#endif /* USE_SSL */
	PyDict_SetItemString(d, "error", PySocket_Error);
	PySocketSock_Type.ob_type = &PyType_Type;
	PySocketSock_Type.tp_doc = sockettype_doc;
	Py_INCREF(&PySocketSock_Type);
	if (PyDict_SetItemString(d, "SocketType",
				 (PyObject *)&PySocketSock_Type) != 0)
		return;

	/* Address families (we only support AF_INET and AF_UNIX) */
#ifdef AF_UNSPEC
	insint(d, "AF_UNSPEC", AF_UNSPEC);
#endif
	insint(d, "AF_INET", AF_INET);
#ifdef AF_UNIX
	insint(d, "AF_UNIX", AF_UNIX);
#endif /* AF_UNIX */
#ifdef AF_AX25
	insint(d, "AF_AX25", AF_AX25); /* Amateur Radio AX.25 */
#endif
#ifdef AF_IPX
	insint(d, "AF_IPX", AF_IPX); /* Novell IPX */
#endif
#ifdef AF_APPLETALK
	insint(d, "AF_APPLETALK", AF_APPLETALK); /* Appletalk DDP */
#endif
#ifdef AF_NETROM
	insint(d, "AF_NETROM", AF_NETROM); /* Amateur radio NetROM */
#endif
#ifdef AF_BRIDGE
	insint(d, "AF_BRIDGE", AF_BRIDGE); /* Multiprotocol bridge */
#endif
#ifdef AF_AAL5
	insint(d, "AF_AAL5", AF_AAL5); /* Reserved for Werner's ATM */
#endif
#ifdef AF_X25
	insint(d, "AF_X25", AF_X25); /* Reserved for X.25 project */
#endif
#ifdef AF_INET6
	insint(d, "AF_INET6", AF_INET6); /* IP version 6 */
#endif
#ifdef AF_ROSE
	insint(d, "AF_ROSE", AF_ROSE); /* Amateur Radio X.25 PLP */
#endif

	/* Socket types */
	insint(d, "SOCK_STREAM", SOCK_STREAM);
	insint(d, "SOCK_DGRAM", SOCK_DGRAM);
#ifndef __BEOS__
/* We have incomplete socket support. */
	insint(d, "SOCK_RAW", SOCK_RAW);
	insint(d, "SOCK_SEQPACKET", SOCK_SEQPACKET);
	insint(d, "SOCK_RDM", SOCK_RDM);
#endif

#ifdef	SO_DEBUG
	insint(d, "SO_DEBUG", SO_DEBUG);
#endif
#ifdef	SO_ACCEPTCONN
	insint(d, "SO_ACCEPTCONN", SO_ACCEPTCONN);
#endif
#ifdef	SO_REUSEADDR
	insint(d, "SO_REUSEADDR", SO_REUSEADDR);
#endif
#ifdef	SO_KEEPALIVE
	insint(d, "SO_KEEPALIVE", SO_KEEPALIVE);
#endif
#ifdef	SO_DONTROUTE
	insint(d, "SO_DONTROUTE", SO_DONTROUTE);
#endif
#ifdef	SO_BROADCAST
	insint(d, "SO_BROADCAST", SO_BROADCAST);
#endif
#ifdef	SO_USELOOPBACK
	insint(d, "SO_USELOOPBACK", SO_USELOOPBACK);
#endif
#ifdef	SO_LINGER
	insint(d, "SO_LINGER", SO_LINGER);
#endif
#ifdef	SO_OOBINLINE
	insint(d, "SO_OOBINLINE", SO_OOBINLINE);
#endif
#ifdef	SO_REUSEPORT
	insint(d, "SO_REUSEPORT", SO_REUSEPORT);
#endif
#ifdef	SO_SNDBUF
	insint(d, "SO_SNDBUF", SO_SNDBUF);
#endif
#ifdef	SO_RCVBUF
	insint(d, "SO_RCVBUF", SO_RCVBUF);
#endif
#ifdef	SO_SNDLOWAT
	insint(d, "SO_SNDLOWAT", SO_SNDLOWAT);
#endif
#ifdef	SO_RCVLOWAT
	insint(d, "SO_RCVLOWAT", SO_RCVLOWAT);
#endif
#ifdef	SO_SNDTIMEO
	insint(d, "SO_SNDTIMEO", SO_SNDTIMEO);
#endif
#ifdef	SO_RCVTIMEO
	insint(d, "SO_RCVTIMEO", SO_RCVTIMEO);
#endif
#ifdef	SO_ERROR
	insint(d, "SO_ERROR", SO_ERROR);
#endif
#ifdef	SO_TYPE
	insint(d, "SO_TYPE", SO_TYPE);
#endif

	/* Maximum number of connections for "listen" */
#ifdef	SOMAXCONN
	insint(d, "SOMAXCONN", SOMAXCONN);
#else
	insint(d, "SOMAXCONN", 5);	/* Common value */
#endif

	/* Flags for send, recv */
#ifdef	MSG_OOB
	insint(d, "MSG_OOB", MSG_OOB);
#endif
#ifdef	MSG_PEEK
	insint(d, "MSG_PEEK", MSG_PEEK);
#endif
#ifdef	MSG_DONTROUTE
	insint(d, "MSG_DONTROUTE", MSG_DONTROUTE);
#endif
#ifdef	MSG_DONTWAIT
	insint(d, "MSG_DONTWAIT", MSG_DONTWAIT);
#endif
#ifdef	MSG_EOR
	insint(d, "MSG_EOR", MSG_EOR);
#endif
#ifdef	MSG_TRUNC
	insint(d, "MSG_TRUNC", MSG_TRUNC);
#endif
#ifdef	MSG_CTRUNC
	insint(d, "MSG_CTRUNC", MSG_CTRUNC);
#endif
#ifdef	MSG_WAITALL
	insint(d, "MSG_WAITALL", MSG_WAITALL);
#endif
#ifdef	MSG_BTAG
	insint(d, "MSG_BTAG", MSG_BTAG);
#endif
#ifdef	MSG_ETAG
	insint(d, "MSG_ETAG", MSG_ETAG);
#endif

	/* Protocol level and numbers, usable for [gs]etsockopt */
#ifdef	SOL_SOCKET
	insint(d, "SOL_SOCKET", SOL_SOCKET);
#endif
#ifdef	SOL_IP
	insint(d, "SOL_IP", SOL_IP);
#else
	insint(d, "SOL_IP", 0);
#endif
#ifdef	SOL_IPX
	insint(d, "SOL_IPX", SOL_IPX);
#endif
#ifdef	SOL_AX25
	insint(d, "SOL_AX25", SOL_AX25);
#endif
#ifdef	SOL_ATALK
	insint(d, "SOL_ATALK", SOL_ATALK);
#endif
#ifdef	SOL_NETROM
	insint(d, "SOL_NETROM", SOL_NETROM);
#endif
#ifdef	SOL_ROSE
	insint(d, "SOL_ROSE", SOL_ROSE);
#endif
#ifdef	SOL_TCP
	insint(d, "SOL_TCP", SOL_TCP);
#else
	insint(d, "SOL_TCP", 6);
#endif
#ifdef	SOL_UDP
	insint(d, "SOL_UDP", SOL_UDP);
#else
	insint(d, "SOL_UDP", 17);
#endif
#ifdef	IPPROTO_IP
	insint(d, "IPPROTO_IP", IPPROTO_IP);
#else
	insint(d, "IPPROTO_IP", 0);
#endif
#ifdef	IPPROTO_ICMP
	insint(d, "IPPROTO_ICMP", IPPROTO_ICMP);
#else
	insint(d, "IPPROTO_ICMP", 1);
#endif
#ifdef	IPPROTO_IGMP
	insint(d, "IPPROTO_IGMP", IPPROTO_IGMP);
#endif
#ifdef	IPPROTO_GGP
	insint(d, "IPPROTO_GGP", IPPROTO_GGP);
#endif
#ifdef	IPPROTO_TCP
	insint(d, "IPPROTO_TCP", IPPROTO_TCP);
#else
	insint(d, "IPPROTO_TCP", 6);
#endif
#ifdef	IPPROTO_EGP
	insint(d, "IPPROTO_EGP", IPPROTO_EGP);
#endif
#ifdef	IPPROTO_PUP
	insint(d, "IPPROTO_PUP", IPPROTO_PUP);
#endif
#ifdef	IPPROTO_UDP
	insint(d, "IPPROTO_UDP", IPPROTO_UDP);
#else
	insint(d, "IPPROTO_UDP", 17);
#endif
#ifdef	IPPROTO_IDP
	insint(d, "IPPROTO_IDP", IPPROTO_IDP);
#endif
#ifdef	IPPROTO_HELLO
	insint(d, "IPPROTO_HELLO", IPPROTO_HELLO);
#endif
#ifdef	IPPROTO_ND
	insint(d, "IPPROTO_ND", IPPROTO_ND);
#endif
#ifdef	IPPROTO_TP
	insint(d, "IPPROTO_TP", IPPROTO_TP);
#endif
#ifdef	IPPROTO_XTP
	insint(d, "IPPROTO_XTP", IPPROTO_XTP);
#endif
#ifdef	IPPROTO_EON
	insint(d, "IPPROTO_EON", IPPROTO_EON);
#endif
#ifdef	IPPROTO_BIP
	insint(d, "IPPROTO_BIP", IPPROTO_BIP);
#endif
/**/
#ifdef	IPPROTO_RAW
	insint(d, "IPPROTO_RAW", IPPROTO_RAW);
#else
	insint(d, "IPPROTO_RAW", 255);
#endif
#ifdef	IPPROTO_MAX
	insint(d, "IPPROTO_MAX", IPPROTO_MAX);
#endif

	/* Some port configuration */
#ifdef	IPPORT_RESERVED
	insint(d, "IPPORT_RESERVED", IPPORT_RESERVED);
#else
	insint(d, "IPPORT_RESERVED", 1024);
#endif
#ifdef	IPPORT_USERRESERVED
	insint(d, "IPPORT_USERRESERVED", IPPORT_USERRESERVED);
#else
	insint(d, "IPPORT_USERRESERVED", 5000);
#endif

	/* Some reserved IP v.4 addresses */
#ifdef	INADDR_ANY
	insint(d, "INADDR_ANY", INADDR_ANY);
#else
	insint(d, "INADDR_ANY", 0x00000000);
#endif
#ifdef	INADDR_BROADCAST
	insint(d, "INADDR_BROADCAST", INADDR_BROADCAST);
#else
	insint(d, "INADDR_BROADCAST", 0xffffffff);
#endif
#ifdef	INADDR_LOOPBACK
	insint(d, "INADDR_LOOPBACK", INADDR_LOOPBACK);
#else
	insint(d, "INADDR_LOOPBACK", 0x7F000001);
#endif
#ifdef	INADDR_UNSPEC_GROUP
	insint(d, "INADDR_UNSPEC_GROUP", INADDR_UNSPEC_GROUP);
#else
	insint(d, "INADDR_UNSPEC_GROUP", 0xe0000000);
#endif
#ifdef	INADDR_ALLHOSTS_GROUP
	insint(d, "INADDR_ALLHOSTS_GROUP", INADDR_ALLHOSTS_GROUP);
#else
	insint(d, "INADDR_ALLHOSTS_GROUP", 0xe0000001);
#endif
#ifdef	INADDR_MAX_LOCAL_GROUP
	insint(d, "INADDR_MAX_LOCAL_GROUP", INADDR_MAX_LOCAL_GROUP);
#else
	insint(d, "INADDR_MAX_LOCAL_GROUP", 0xe00000ff);
#endif
#ifdef	INADDR_NONE
	insint(d, "INADDR_NONE", INADDR_NONE);
#else
	insint(d, "INADDR_NONE", 0xffffffff);
#endif

	/* IP [gs]etsockopt options */
#ifdef	IP_OPTIONS
	insint(d, "IP_OPTIONS", IP_OPTIONS);
#endif
#ifdef	IP_HDRINCL
	insint(d, "IP_HDRINCL", IP_HDRINCL);
#endif
#ifdef	IP_TOS
	insint(d, "IP_TOS", IP_TOS);
#endif
#ifdef	IP_TTL
	insint(d, "IP_TTL", IP_TTL);
#endif
#ifdef	IP_RECVOPTS
	insint(d, "IP_RECVOPTS", IP_RECVOPTS);
#endif
#ifdef	IP_RECVRETOPTS
	insint(d, "IP_RECVRETOPTS", IP_RECVRETOPTS);
#endif
#ifdef	IP_RECVDSTADDR
	insint(d, "IP_RECVDSTADDR", IP_RECVDSTADDR);
#endif
#ifdef	IP_RETOPTS
	insint(d, "IP_RETOPTS", IP_RETOPTS);
#endif
#ifdef	IP_MULTICAST_IF
	insint(d, "IP_MULTICAST_IF", IP_MULTICAST_IF);
#endif
#ifdef	IP_MULTICAST_TTL
	insint(d, "IP_MULTICAST_TTL", IP_MULTICAST_TTL);
#endif
#ifdef	IP_MULTICAST_LOOP
	insint(d, "IP_MULTICAST_LOOP", IP_MULTICAST_LOOP);
#endif
#ifdef	IP_ADD_MEMBERSHIP
	insint(d, "IP_ADD_MEMBERSHIP", IP_ADD_MEMBERSHIP);
#endif
#ifdef	IP_DROP_MEMBERSHIP
	insint(d, "IP_DROP_MEMBERSHIP", IP_DROP_MEMBERSHIP);
#endif
#ifdef	IP_DEFAULT_MULTICAST_TTL
	insint(d, "IP_DEFAULT_MULTICAST_TTL", IP_DEFAULT_MULTICAST_TTL);
#endif
#ifdef	IP_DEFAULT_MULTICAST_LOOP
	insint(d, "IP_DEFAULT_MULTICAST_LOOP", IP_DEFAULT_MULTICAST_LOOP);
#endif
#ifdef	IP_MAX_MEMBERSHIPS
	insint(d, "IP_MAX_MEMBERSHIPS", IP_MAX_MEMBERSHIPS);
#endif

	/* TCP options */
#ifdef	TCP_NODELAY
	insint(d, "TCP_NODELAY", TCP_NODELAY);
#endif
#ifdef	TCP_MAXSEG
	insint(d, "TCP_MAXSEG", TCP_MAXSEG);
#endif

	/* IPX options */
#ifdef	IPX_TYPE
	insint(d, "IPX_TYPE", IPX_TYPE);
#endif

	/* Initialize gethostbyname lock */
#ifdef USE_GETHOSTBYNAME_LOCK
	gethostbyname_lock = PyThread_allocate_lock();
#endif
}
