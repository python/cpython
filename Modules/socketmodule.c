/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Socket module */

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef MS_WINDOWS
extern int gethostname(); /* For Solaris, at least */
#endif

#include <sys/types.h>
#include "mytime.h"

#include <signal.h>
#ifndef MS_WINDOWS
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

#ifdef USE_GUSI
/* fdopen() isn't declared in stdio.h (sigh) */
#include <GUSI.h>
#endif


/* Here we have some hacks to choose between K&R or ANSI style function
   definitions.  For NT to build this as an extension module (ie, DLL)
   it must be compiled by the C++ compiler, as it takes the address of
   a static data item exported from the main Python DLL.
*/
#ifdef MS_WINDOWS
/* seem to be a few differences in the API */
#define close closesocket
#define NO_DUP /* Actually it exists on NT 3.5, but what the heck... */
#define FORCE_ANSI_FUNC_DEFS
#endif

#ifdef FORCE_ANSI_FUNC_DEFS
#define BUILD_FUNC_DEF_1( fnname, arg1type, arg1name )	\
fnname( arg1type arg1name )

#define BUILD_FUNC_DEF_2( fnname, arg1type, arg1name, arg2type, arg2name ) \
fnname( arg1type arg1name, arg2type arg2name )

#define BUILD_FUNC_DEF_3( fnname, arg1type, arg1name, arg2type, arg2name , arg3type, arg3name )	\
fnname( arg1type arg1name, arg2type arg2name, arg3type arg3name )

#define BUILD_FUNC_DEF_4( fnname, arg1type, arg1name, arg2type, arg2name , arg3type, arg3name, arg4type, arg4name )	\
fnname( arg1type arg1name, arg2type arg2name, arg3type arg3name, arg4type arg4name )

#else /* !FORCE_ANSI_FN_DEFS */
#define BUILD_FUNC_DEF_1( fnname, arg1type, arg1name )	\
fnname( arg1name )	\
	arg1type arg1name;

#define BUILD_FUNC_DEF_2( fnname, arg1type, arg1name, arg2type, arg2name ) \
fnname( arg1name, arg2name )	\
	arg1type arg1name;	\
	arg2type arg2name;

#define BUILD_FUNC_DEF_3( fnname, arg1type, arg1name, arg2type, arg2name, arg3type, arg3name ) \
fnname( arg1name, arg2name, arg3name )	\
	arg1type arg1name;	\
	arg2type arg2name;	\
	arg3type arg3name;

#define BUILD_FUNC_DEF_4( fnname, arg1type, arg1name, arg2type, arg2name, arg3type, arg3name, arg4type, arg4name ) \
fnname( arg1name, arg2name, arg3name, arg4name )	\
	arg1type arg1name;	\
	arg2type arg2name;	\
	arg3type arg3name;	\
	arg4type arg4name;

#endif /* !FORCE_ANSI_FN_DEFS */

/* Global variable holding the exception type for errors detected
   by this module (but not argument type or memory errors, etc.). */

static PyObject *PySocket_Error;


/* Convenience function to raise an error according to errno
   and return a NULL pointer from a function. */

static PyObject *
PySocket_Err()
{
#ifdef MS_WINDOWS
	if (WSAGetLastError()) {
		PyObject *v;
		v = Py_BuildValue("(is)", WSAGetLastError(), "winsock error");
		if (v != NULL) {
			PyErr_SetObject(PySocket_Error, v);
			Py_DECREF(v);
		}
		return NULL;
	}
	else
#endif
	return PyErr_SetFromErrno(PySocket_Error);
}


/* The object holding a socket.  It holds some extra information,
   like the address family, which is used to decode socket address
   arguments properly. */

typedef struct {
	PyObject_HEAD
	int sock_fd;		/* Socket file descriptor */
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
BUILD_FUNC_DEF_4(PySocketSock_New,int,fd, int,family, int,type, int,proto)
{
	PySocketSockObject *s;
	PySocketSock_Type.ob_type = &PyType_Type;
	s = PyObject_NEW(PySocketSockObject, &PySocketSock_Type);
	if (s != NULL) {
		s->sock_fd = fd;
		s->sock_family = family;
		s->sock_type = type;
		s->sock_proto = proto;
	}
	return s;
}


/* Convert a string specifying a host name or one of a few symbolic
   names to a numeric IP address.  This usually calls gethostbyname()
   to do the work; the names "" and "<broadcast>" are special.
   Return the length (should always be 4 bytes), or negative if
   an error occurred; then an exception is raised. */

static int
BUILD_FUNC_DEF_2(setipaddr, char*,name, struct sockaddr_in *,addr_ret)
{
	struct hostent *hp;
	int d1, d2, d3, d4;
	char ch;
#ifdef HAVE_GETHOSTBYNAME_R
	struct hostent hp_allocated;
	char buf[1001];
	int buf_len = (sizeof buf) - 1;
	int errnop;
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
#ifdef HAVE_GETHOSTBYNAME_R
	Py_BEGIN_ALLOW_THREADS
	hp = gethostbyname_r(name, &hp_allocated, buf, buf_len, &errnop);
	Py_END_ALLOW_THREADS
#else /* not HAVE_GETHOSTBYNAME_R */
	hp = gethostbyname(name);
#endif /* HAVE_GETHOSTBYNAME_R */

	if (hp == NULL) {
#ifdef HAVE_HSTRERROR
	        /* Let's get real error message to return */
	        extern int h_errno;
		PyErr_SetString(PySocket_Error, (char *)hstrerror(h_errno));
#else
		PyErr_SetString(PySocket_Error, "host not found");
#endif
		return -1;
	}
	memcpy((char *) &addr_ret->sin_addr, hp->h_addr, hp->h_length);
	return hp->h_length;
}


/* Create a string object representing an IP address.
   This is always a string of the form 'dd.dd.dd.dd' (with variable
   size numbers). */

static PyObject *
BUILD_FUNC_DEF_1(makeipaddr, struct sockaddr_in *,addr)
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
BUILD_FUNC_DEF_2(makesockaddr,struct sockaddr *,addr, int,addrlen)
{
	if (addrlen == 0) {
		/* No address -- may be recvfrom() from known socket */
		Py_INCREF(Py_None);
		return Py_None;
	}

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
BUILD_FUNC_DEF_4(
getsockaddrarg,PySocketSockObject *,s, PyObject *,args, struct sockaddr **,addr_ret, int *,len_ret)
{
	switch (s->sock_family) {

#ifdef AF_UNIX
	case AF_UNIX:
	{
		struct sockaddr_un* addr;
		char *path;
		int len;
		addr = (struct sockaddr_un* )&(s->sock_addr).un;
		if (!PyArg_Parse(args, "s#", &path, &len))
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
BUILD_FUNC_DEF_2(getsockaddrlen,PySocketSockObject *,s, int *,len_ret)
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
		PyErr_SetString(PySocket_Error, "getsockaddrarg: bad family");
		return 0;

	}
}


/* s.accept() method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_accept,PySocketSockObject *,s, PyObject *,args)
{
	char addrbuf[256];
	int addrlen, newfd;
	PyObject *sock = NULL;
	PyObject *addr = NULL;
	PyObject *res = NULL;

	if (!PyArg_NoArgs(args))
		return NULL;
	if (!getsockaddrlen(s, &addrlen))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	newfd = accept(s->sock_fd, (struct sockaddr *) addrbuf, &addrlen);
	Py_END_ALLOW_THREADS
	if (newfd < 0)
		return PySocket_Err();

	/* Create the new object with unspecified family,
	   to avoid calls to bind() etc. on it. */
	sock = (PyObject *) PySocketSock_New(newfd,
					s->sock_family,
					s->sock_type,
					s->sock_proto);
	if (sock == NULL) {
		close(newfd);
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


/* s.setblocking(1 | 0) method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_setblocking,PySocketSockObject*,s,PyObject*,args)
{
	int block;
#ifndef MS_WINDOWS
	int delay_flag;
#endif
	if (!PyArg_Parse(args, "i", &block))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
#ifndef MS_WINDOWS
	delay_flag = fcntl (s->sock_fd, F_GETFL, 0);
	if (block)
		delay_flag &= (~O_NDELAY);
	else
		delay_flag |= O_NDELAY;
	fcntl (s->sock_fd, F_SETFL, delay_flag);
#else
	block = !block;
	ioctlsocket(s->sock_fd, FIONBIO, (u_long*)&block);
#endif
	Py_END_ALLOW_THREADS

	Py_INCREF(Py_None);
	return Py_None;
}


/* s.setsockopt() method.
   With an integer third argument, sets an integer option.
   With a string third argument, sets an option from a buffer;
   use optional built-in module 'struct' to encode the string. */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_setsockopt,PySocketSockObject *,s, PyObject *,args)
{
	int level;
	int optname;
	int res;
	char *buf;
	int buflen;
	int flag;

	if (PyArg_Parse(args, "(iii)", &level, &optname, &flag)) {
		buf = (char *) &flag;
		buflen = sizeof flag;
	}
	else {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(iis#)", &level, &optname,
				 &buf, &buflen))
			return NULL;
	}
	res = setsockopt(s->sock_fd, level, optname, (ANY *)buf, buflen);
	if (res < 0)
		return PySocket_Err();
	Py_INCREF(Py_None);
	return Py_None;
}


/* s.getsockopt() method.
   With two arguments, retrieves an integer option.
   With a third integer argument, retrieves a string buffer of that size;
   use optional built-in module 'struct' to decode the string. */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_getsockopt,PySocketSockObject *,s, PyObject *,args)
{
	int level;
	int optname;
	int res;
	PyObject *buf;
	int buflen = 0;

	if (!PyArg_ParseTuple(args, "ii|i", &level, &optname, &buflen))
		return NULL;
	
	if (buflen == 0) {
		int flag = 0;
		int flagsize = sizeof flag;
		res = getsockopt(s->sock_fd, level, optname,
				 (ANY *)&flag, &flagsize);
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
			 (ANY *)PyString_AsString(buf), &buflen);
	if (res < 0) {
		Py_DECREF(buf);
		return PySocket_Err();
	}
	_PyString_Resize(&buf, buflen);
	return buf;
}


/* s.bind(sockaddr) method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_bind,PySocketSockObject *,s, PyObject *,args)
{
	struct sockaddr *addr;
	int addrlen;
	int res;
	if (!getsockaddrarg(s, args, &addr, &addrlen))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = bind(s->sock_fd, addr, addrlen);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return PySocket_Err();
	Py_INCREF(Py_None);
	return Py_None;
}


/* s.close() method.
   Set the file descriptor to -1 so operations tried subsequently
   will surely fail. */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_close,PySocketSockObject *,s, PyObject *,args)
{
	if (!PyArg_NoArgs(args))
		return NULL;
	if (s->sock_fd != -1) {
		Py_BEGIN_ALLOW_THREADS
		(void) close(s->sock_fd);
		Py_END_ALLOW_THREADS
	}
	s->sock_fd = -1;
	Py_INCREF(Py_None);
	return Py_None;
}


/* s.connect(sockaddr) method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_connect,PySocketSockObject *,s, PyObject *,args)
{
	struct sockaddr *addr;
	int addrlen;
	int res;
	if (!getsockaddrarg(s, args, &addr, &addrlen))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = connect(s->sock_fd, addr, addrlen);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return PySocket_Err();
	Py_INCREF(Py_None);
	return Py_None;
}


/* s.fileno() method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_fileno,PySocketSockObject *,s, PyObject *,args)
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long) s->sock_fd);
}


#ifndef NO_DUP
/* s.dup() method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_dup,PySocketSockObject *,s, PyObject *,args)
{
	int newfd;
	PyObject *sock;
	if (!PyArg_NoArgs(args))
		return NULL;
	newfd = dup(s->sock_fd);
	if (newfd < 0)
		return PySocket_Err();
	sock = (PyObject *) PySocketSock_New(newfd,
					     s->sock_family,
					     s->sock_type,
					     s->sock_proto);
	if (sock == NULL)
		close(newfd);
	return sock;
}
#endif


/* s.getsockname() method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_getsockname,PySocketSockObject *,s, PyObject *,args)
{
	char addrbuf[256];
	int addrlen, res;
	if (!PyArg_NoArgs(args))
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


#ifdef HAVE_GETPEERNAME		/* Cray APP doesn't have this :-( */
/* s.getpeername() method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_getpeername,PySocketSockObject *,s, PyObject *,args)
{
	char addrbuf[256];
	int addrlen, res;
	if (!PyArg_NoArgs(args))
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
#endif /* HAVE_GETPEERNAME */


/* s.listen(n) method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_listen,PySocketSockObject *,s, PyObject *,args)
{
	int backlog;
	int res;
	if (!PyArg_Parse(args, "i", &backlog))
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

#ifndef NO_DUP
/* s.makefile(mode) method.
   Create a new open file object referring to a dupped version of
   the socket's file descriptor.  (The dup() call is necessary so
   that the open file and socket objects may be closed independent
   of each other.)
   The mode argument specifies 'r' or 'w' passed to fdopen(). */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_makefile,PySocketSockObject *,s, PyObject *,args)
{
	extern int fclose Py_PROTO((FILE *));
	char *mode = "r";
	int bufsize = -1;
	int fd;
	FILE *fp;
	PyObject *f;

	if (!PyArg_ParseTuple(args, "|si", &mode, &bufsize))
		return NULL;
#ifdef MS_WIN32
   if ( ((fd =  _open_osfhandle( s->sock_fd, _O_BINARY )) < 0) ||
        ((fd = dup(fd)) < 0) || ((fp = fdopen(fd, mode)) == NULL)) {
#else
	if ((fd = dup(s->sock_fd)) < 0 ||
	    (fp = fdopen(fd, mode)) == NULL) {
#endif
		if (fd >= 0)
			close(fd);
		return PySocket_Err();
	}
	f = PyFile_FromFile(fp, "<socket>", mode, fclose);
	if (f != NULL)
		PyFile_SetBufSize(f, bufsize);
	return f;
}
#endif /* NO_DUP */

/* s.recv(nbytes [,flags]) method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_recv,PySocketSockObject *,s, PyObject *,args)
{
	int len, n, flags = 0;
	PyObject *buf;
	if (!PyArg_ParseTuple(args, "i|i", &len, &flags))
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


/* s.recvfrom(nbytes [,flags]) method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_recvfrom,PySocketSockObject *,s, PyObject *,args)
{
	char addrbuf[256];
	PyObject *buf = NULL;
	PyObject *addr = NULL;
	PyObject *ret = NULL;

	int addrlen, len, n, flags = 0;
	if (!PyArg_ParseTuple(args, "i|i", &len, &flags))
		return NULL;
	if (!getsockaddrlen(s, &addrlen))
		return NULL;
	buf = PyString_FromStringAndSize((char *) 0, len);
	if (buf == NULL)
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	n = recvfrom(s->sock_fd, PyString_AsString(buf), len, flags,
#ifndef MS_WINDOWS
		     (ANY *)addrbuf, &addrlen
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


/* s.send(data [,flags]) method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_send,PySocketSockObject *,s, PyObject *,args)
{
	char *buf;
	int len, n, flags = 0;
	if (!PyArg_ParseTuple(args, "s#|i", &buf, &len, &flags))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	n = send(s->sock_fd, buf, len, flags);
	Py_END_ALLOW_THREADS
	if (n < 0)
		return PySocket_Err();
	return PyInt_FromLong((long)n);
}


/* s.sendto(data, [flags,] sockaddr) method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_sendto,PySocketSockObject *,s, PyObject *,args)
{
	PyObject *addro;
	char *buf;
	struct sockaddr *addr;
	int addrlen, len, n, flags;
	flags = 0;
	if (!PyArg_Parse(args, "(s#O)", &buf, &len, &addro)) {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(s#iO)", &buf, &len, &flags, &addro))
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


/* s.shutdown(how) method */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_shutdown,PySocketSockObject *,s, PyObject *,args)
{
	int how;
	int res;
	if (!PyArg_Parse(args, "i", &how))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = shutdown(s->sock_fd, how);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return PySocket_Err();
	Py_INCREF(Py_None);
	return Py_None;
}


/* List of methods for socket objects */

static PyMethodDef PySocketSock_methods[] = {
	{"accept",		(PyCFunction)PySocketSock_accept},
	{"setblocking",		(PyCFunction)PySocketSock_setblocking},
	{"setsockopt",		(PyCFunction)PySocketSock_setsockopt},
	{"getsockopt",		(PyCFunction)PySocketSock_getsockopt, 1},
	{"bind",		(PyCFunction)PySocketSock_bind},
	{"close",		(PyCFunction)PySocketSock_close},
	{"connect",		(PyCFunction)PySocketSock_connect},
	{"fileno",		(PyCFunction)PySocketSock_fileno},
#ifndef NO_DUP
	{"dup",			(PyCFunction)PySocketSock_dup},
#endif
	{"getsockname",		(PyCFunction)PySocketSock_getsockname},
#ifdef HAVE_GETPEERNAME
	{"getpeername",		(PyCFunction)PySocketSock_getpeername},
#endif
	{"listen",		(PyCFunction)PySocketSock_listen},
#ifndef NO_DUP
	{"makefile",		(PyCFunction)PySocketSock_makefile, 1},
#endif
	{"recv",		(PyCFunction)PySocketSock_recv, 1},
	{"recvfrom",		(PyCFunction)PySocketSock_recvfrom, 1},
	{"send",		(PyCFunction)PySocketSock_send, 1},
	{"sendto",		(PyCFunction)PySocketSock_sendto},
	{"shutdown",		(PyCFunction)PySocketSock_shutdown},
	{NULL,			NULL}		/* sentinel */
};


/* Deallocate a socket object in response to the last Py_DECREF().
   First close the file description. */

static void
BUILD_FUNC_DEF_1(PySocketSock_dealloc,PySocketSockObject *,s)
{
	(void) close(s->sock_fd);
	PyMem_DEL(s);
}


/* Return a socket object's named attribute. */

static PyObject *
BUILD_FUNC_DEF_2(PySocketSock_getattr,PySocketSockObject *,s, char *,name)
{
	return Py_FindMethod(PySocketSock_methods, (PyObject *) s, name);
}


static PyObject *
BUILD_FUNC_DEF_1(PySocketSock_repr,PySocketSockObject *,s)
{
	char buf[512];
	sprintf(buf, 
		"<socket object, fd=%d, family=%d, type=%d, protocol=%d>", 
		s->sock_fd, s->sock_family, s->sock_type, s->sock_proto);
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
BUILD_FUNC_DEF_2(PySocket_gethostname,PyObject *,self, PyObject *,args)
{
	char buf[1024];
	int res;
	if (!PyArg_NoArgs(args))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = gethostname(buf, (int) sizeof buf - 1);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return PySocket_Err();
	buf[sizeof buf - 1] = '\0';
	return PyString_FromString(buf);
}


/* Python interface to gethostbyname(name). */

/*ARGSUSED*/
static PyObject *
BUILD_FUNC_DEF_2(PySocket_gethostbyname,PyObject *,self, PyObject *,args)
{
	char *name;
	struct sockaddr_in addrbuf;
	if (!PyArg_Parse(args, "s", &name))
		return NULL;
	if (setipaddr(name, &addrbuf) < 0)
		return NULL;
	return makeipaddr(&addrbuf);
}

/* Python interface to gethostbyaddr(IP). */

/*ARGSUSED*/
static PyObject *
BUILD_FUNC_DEF_2(PySocket_gethostbyaddr,PyObject *,self, PyObject *, args)
{
        struct sockaddr_in addr;
	char *ip_num;
	struct hostent *h;
	char **pch;
	PyObject *rtn_tuple = (PyObject *)NULL;
	PyObject *name_list = (PyObject *)NULL;
	PyObject *addr_list = (PyObject *)NULL;
	PyObject *tmp;

	if (!PyArg_Parse(args, "s", &ip_num))
		return NULL;
	if (setipaddr(ip_num, &addr) < 0)
		return NULL;
	h = gethostbyaddr((char *)&addr.sin_addr,
			  sizeof(addr.sin_addr),
			  AF_INET);
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
		memcpy((char *) &addr.sin_addr, *pch, h->h_length);
		tmp = makeipaddr(&addr);
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


/* Python interface to getservbyname(name).
   This only returns the port number, since the other info is already
   known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
BUILD_FUNC_DEF_2(PySocket_getservbyname,PyObject *,self, PyObject *,args)
{
	char *name, *proto;
	struct servent *sp;
	if (!PyArg_Parse(args, "(ss)", &name, &proto))
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


/* Python interface to getprotobyname(name).
   This only returns the protocol number, since the other info is
   already known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
BUILD_FUNC_DEF_2(PySocket_getprotobyname,PyObject *,self, PyObject *,args)
{
	char *name;
	struct protoent *sp;
	if (!PyArg_Parse(args, "s", &name))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	sp = getprotobyname(name);
	Py_END_ALLOW_THREADS
	if (sp == NULL) {
		PyErr_SetString(PySocket_Error, "protocol not found");
		return NULL;
	}
	return PyInt_FromLong((long) sp->p_proto);
}


/* Python interface to socket(family, type, proto).
   The third (protocol) argument is optional.
   Return a new socket object. */

/*ARGSUSED*/
static PyObject *
BUILD_FUNC_DEF_2(PySocket_socket,PyObject *,self, PyObject *,args)
{
	PySocketSockObject *s;
#ifdef MS_WINDOWS
	SOCKET fd;
#else
	int fd;
#endif
	int family, type, proto = 0;
	if (!PyArg_ParseTuple(args, "ii|i", &family, &type, &proto))
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
		(void) close(fd);
	/* From now on, ignore SIGPIPE and let the error checking
	   do the work. */
#ifdef SIGPIPE      
	(void) signal(SIGPIPE, SIG_IGN);
#endif   
	return (PyObject *) s;
}

#ifndef NO_DUP
/* Create a socket object from a numeric file description.
   Useful e.g. if stdin is a socket.
   Additional arguments as for socket(). */

/*ARGSUSED*/
static PyObject *
BUILD_FUNC_DEF_2(PySocket_fromfd,PyObject *,self, PyObject *,args)
{
	PySocketSockObject *s;
	int fd, family, type, proto = 0;
	if (!PyArg_ParseTuple(args, "iii|i", &fd, &family, &type, &proto))
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
#endif /* NO_DUP */

static PyObject *
BUILD_FUNC_DEF_2(PySocket_ntohs, PyObject *, self, PyObject *, args)
{
	int x1, x2;

	if (!PyArg_Parse(args, "i", &x1)) {
		return NULL;
	}
	x2 = (int)ntohs((short)x1);
	return PyInt_FromLong(x2);
}

static PyObject *
BUILD_FUNC_DEF_2(PySocket_ntohl, PyObject *, self, PyObject *, args)
{
	int x1, x2;

	if (!PyArg_Parse(args, "i", &x1)) {
		return NULL;
	}
	x2 = ntohl(x1);
	return PyInt_FromLong(x2);
}

static PyObject *
BUILD_FUNC_DEF_2(PySocket_htons, PyObject *, self, PyObject *, args)
{
	int x1, x2;

	if (!PyArg_Parse(args, "i", &x1)) {
		return NULL;
	}
	x2 = (int)htons((short)x1);
	return PyInt_FromLong(x2);
}

static PyObject *
BUILD_FUNC_DEF_2(PySocket_htonl, PyObject *, self, PyObject *, args)
{
	int x1, x2;

	if (!PyArg_Parse(args, "i", &x1)) {
		return NULL;
	}
	x2 = htonl(x1);
	return PyInt_FromLong(x2);
}

/* List of functions exported by this module. */

static PyMethodDef PySocket_methods[] = {
	{"gethostbyname",	PySocket_gethostbyname},
	{"gethostbyaddr",	PySocket_gethostbyaddr},
	{"gethostname",		PySocket_gethostname},
	{"getservbyname",	PySocket_getservbyname},
	{"getprotobyname",	PySocket_getprotobyname},
	{"socket",		PySocket_socket, 1},
#ifndef NO_DUP
	{"fromfd",		PySocket_fromfd, 1},
#endif
	{"ntohs",		PySocket_ntohs},
	{"ntohl",		PySocket_ntohl},
	{"htons",		PySocket_htons},
	{"htonl",		PySocket_htonl},
	{NULL,			NULL}		 /* Sentinel */
};


/* Convenience routine to export an integer value.
 *
 * Since this function is called only from initsocket/init_socket(), any
 * errors trigger a fatal exception.
 */
static void
BUILD_FUNC_DEF_3(insint,PyObject *,d, char *,name, int,value)
{
	PyObject *v = PyInt_FromLong((long) value);
	if (!v || PyDict_SetItemString(d, name, v))
		Py_FatalError("can't initialize socket module");

	Py_DECREF(v);
}


#ifdef MS_WINDOWS

/* Additional initialization and cleanup for NT/Windows */

static void
NTcleanup()
{
	WSACleanup();
}

static int
NTinit()
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


/* Initialize this module.
   This is called when the first 'import socket' is done,
   via a table in config.c, if config.c is compiled with USE_SOCKET
   defined.

   For MS_WINDOWS (which means any Windows variant), this module
   is actually called "_socket", and there's a wrapper "socket.py"
   which implements some missing functionality (such as makefile(),
   dup() and fromfd()).  The import of "_socket" may fail with an
   ImportError exception if initialization of WINSOCK fails.  When
   WINSOCK is initialized succesfully, a call to WSACleanup() is
   scheduled to be made at exit time.  */

void
#ifdef MS_WINDOWS
init_socket()
#else
initsocket()
#endif
{
	PyObject *m, *d;
#ifdef MS_WINDOWS
	if (!NTinit())
		return;
	m = Py_InitModule("_socket", PySocket_methods);
#else
	m = Py_InitModule("socket", PySocket_methods);
#endif
	d = PyModule_GetDict(m);
	PySocket_Error = PyString_FromString("socket.error");
	if (PySocket_Error == NULL || 
	    PyDict_SetItemString(d, "error", PySocket_Error) != 0)
		Py_FatalError("can't define socket.error");
	PySocketSock_Type.ob_type = &PyType_Type;
	Py_INCREF(&PySocketSock_Type);
	if (PyDict_SetItemString(d, "SocketType",
				 (PyObject *)&PySocketSock_Type) != 0)
		Py_FatalError("can't define socket.SocketType");
	insint(d, "AF_INET", AF_INET);
#ifdef AF_UNIX
	insint(d, "AF_UNIX", AF_UNIX);
#endif /* AF_UNIX */
	insint(d, "SOCK_STREAM", SOCK_STREAM);
	insint(d, "SOCK_DGRAM", SOCK_DGRAM);
	insint(d, "SOCK_RAW", SOCK_RAW);
	insint(d, "SOCK_SEQPACKET", SOCK_SEQPACKET);
	insint(d, "SOCK_RDM", SOCK_RDM);

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
#ifdef	IPPROTO_IP
	insint(d, "IPPROTO_IP", IPPROTO_IP);
#endif
#ifdef	IPPROTO_ICMP
	insint(d, "IPPROTO_ICMP", IPPROTO_ICMP);
#endif
#ifdef	IPPROTO_IGMP
	insint(d, "IPPROTO_IGMP", IPPROTO_IGMP);
#endif
#ifdef	IPPROTO_GGP
	insint(d, "IPPROTO_GGP", IPPROTO_GGP);
#endif
#ifdef	IPPROTO_TCP
	insint(d, "IPPROTO_TCP", IPPROTO_TCP);
#endif
#ifdef	IPPROTO_EGP
	insint(d, "IPPROTO_EGP", IPPROTO_EGP);
#endif
#ifdef	IPPROTO_PUP
	insint(d, "IPPROTO_PUP", IPPROTO_PUP);
#endif
#ifdef	IPPROTO_UDP
	insint(d, "IPPROTO_UDP", IPPROTO_UDP);
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
}
