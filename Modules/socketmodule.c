/***********************************************************
Copyright 1991, 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Socket module */

/* XXX Ought to fix getStr*arg calls to use getargs(args, "s#", ...) */

/*
This module provides an interface to Berkeley socket IPC.

Limitations:

- only AF_INET and AF_UNIX address families are supported
- no asynchronous I/O (but read polling: avail)
- no read/write operations (use send/recv or makefile instead)
- no flags on sendto/recvfrom operations
- setsockopt() and getsockopt() only support integer options

Interface:

- socket.gethostname() --> host name (string)
- socket.gethostbyname(hostname) --> host IP address (string: 'dd.dd.dd.dd')
- socket.getservbyname(servername, protocolname) --> port number
- socket.socket(family, type [, proto]) --> new socket object
- family and type constants from <socket.h> are accessed as socket.AF_INET etc.
- errors are reported as the exception socket.error
- an Internet socket address is a pair (hostname, port)
  where hostname can be anything recognized by gethostbyname()
  (including the dd.dd.dd.dd notation) and port is in host byte order
- where a hostname is returned, the dd.dd.dd.dd notation is used
- a UNIX domain socket is a string specifying the pathname

Socket methods:

- s.accept() --> new socket object, sockaddr
- s.avail() --> boolean
- s.setsockopt(level, optname, flag) --> None
- s.getsockopt(level, optname) --> flag
- s.bind(sockaddr) --> None
- s.connect(sockaddr) --> None
- s.listen(n) --> None
- s.makefile(mode) --> file object
- s.recv(nbytes) --> string
- s.recvfrom(nbytes) --> string, sockaddr
- s.send(string) --> None
- s.sendto(string, sockaddr) --> None
- s.shutdown(how) --> None
- s.close() --> None

*/

#include "allobjects.h"
#include "modsupport.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h> /* Needed for struct timeval */
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#ifdef _AIX /* I *think* this works */
/* AIX defines fd_set in a separate file.  Sigh... */
#include <sys/select.h>
#endif


/* Global variable holding the exception type for errors detected
   by this module (but not argument type or memory errors, etc.). */

static object *SocketError;


/* Convenience function to raise an error according to errno
   and return a NULL pointer from a function. */

static object *
socket_error()
{
	return err_errno(SocketError);
}


/* The object holding a socket.  It holds some extra information,
   like the address family, which is used to decode socket address
   arguments properly. */

typedef struct {
	OB_HEAD
	int sock_fd;		/* Socket file descriptor */
	int sock_family;	/* Address family, e.g., AF_INET */
	int sock_type;		/* Socket type, e.g., SOCK_STREAM */
	int sock_proto;		/* Protocol type, usually 0 */
} sockobject;


/* A forward reference to the Socktype type object.
   The Socktype variable contains pointers to various functions,
   some of which call newsocobject(), which uses Socktype, so
   there has to be a circular reference. */

extern typeobject Socktype; /* Forward */


/* Create a new socket object.
   This just creates the object and initializes it.
   If the creation fails, return NULL and set an exception (implicit
   in NEWOBJ()). */

static sockobject *
newsockobject(fd, family, type, proto)
	int fd, family, type, proto;
{
	sockobject *s;
	s = NEWOBJ(sockobject, &Socktype);
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
setipaddr(name, addr_ret)
	char *name;
	struct sockaddr_in *addr_ret;
{
	struct hostent *hp;
	int d1, d2, d3, d4;
	char ch;

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
	hp = gethostbyname(name);
	if (hp == NULL) {
		err_setstr(SocketError, "host not found");
		return -1;
	}
	memcpy((char *) &addr_ret->sin_addr, hp->h_addr, hp->h_length);
	return hp->h_length;
}


/* Generally useful convenience function to create a tuple from two
   objects.  This eats references to the objects; if either is NULL
   it destroys the other and returns NULL without raising an exception
   (assuming the function that was called to create the argument must
   have raised an exception and returned NULL). */

static object *
makepair(a, b)
	object *a, *b;
{
	object *pair = NULL;
	if (a == NULL || b == NULL || (pair = newtupleobject(2)) == NULL) {
		XDECREF(a);
		XDECREF(b);
		return NULL;
	}
	settupleitem(pair, 0, a);
	settupleitem(pair, 1, b);
	return pair;
}


/* Create a string object representing an IP address.
   This is always a string of the form 'dd.dd.dd.dd' (with variable
   size numbers). */

static object *
makeipaddr(addr)
	struct sockaddr_in *addr;
{
	long x = ntohl(addr->sin_addr.s_addr);
	char buf[100];
	sprintf(buf, "%d.%d.%d.%d",
		(int) (x>>24) & 0xff, (int) (x>>16) & 0xff,
		(int) (x>> 8) & 0xff, (int) (x>> 0) & 0xff);
	return newstringobject(buf);
}


/* Create an object representing the given socket address,
   suitable for passing it back to bind(), connect() etc.
   The family field of the sockaddr structure is inspected
   to determine what kind of address it really is. */

/*ARGSUSED*/
static object *
makesockaddr(addr, addrlen)
	struct sockaddr *addr;
	int addrlen;
{
	switch (addr->sa_family) {

	case AF_INET:
	{
		struct sockaddr_in *a = (struct sockaddr_in *) addr;
		return makepair(makeipaddr(a),
				newintobject((long) ntohs(a->sin_port)));
	}

	case AF_UNIX:
	{
		struct sockaddr_un *a = (struct sockaddr_un *) addr;
		return newstringobject(a->sun_path);
	}

	/* More cases here... */

	default:
		err_setstr(SocketError, "return unknown socket address type");
		return NULL;
	}
}


/* Parse a socket address argument according to the socket object's
   address family.  Return 1 if the address was in the proper format,
   0 of not.  The address is returned through addr_ret, its length
   through len_ret. */

static int
getsockaddrarg(s, args, addr_ret, len_ret)
	sockobject *s;
	object *args;
	struct sockaddr **addr_ret;
	int *len_ret;
{
	switch (s->sock_family) {

	case AF_UNIX:
	{
		static struct sockaddr_un addr;
		object *path;
		int len;
		if (!getStrarg(args, &path))
			return 0;
		if ((len = getstringsize(path)) > sizeof addr.sun_path) {
			err_setstr(SocketError, "AF_UNIX path too long");
			return 0;
		}
		addr.sun_family = AF_UNIX;
		memcpy(addr.sun_path, getstringvalue(path), len);
		*addr_ret = (struct sockaddr *) &addr;
		*len_ret = len + sizeof addr.sun_family;
		return 1;
	}

	case AF_INET:
	{
		static struct sockaddr_in addr;
		object *host;
		int port;
		if (!getStrintarg(args, &host, &port))
			return 0;
		if (setipaddr(getstringvalue(host), &addr) < 0)
			return 0;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		*addr_ret = (struct sockaddr *) &addr;
		*len_ret = sizeof addr;
		return 1;
	}

	/* More cases here... */

	default:
		err_setstr(SocketError, "getsockaddrarg: bad family");
		return 0;

	}
}


/* Get the address length according to the socket object's address family. 
   Return 1 if the family is known, 0 otherwise.  The length is returned
   through len_ret. */

static int
getsockaddrlen(s, len_ret)
	sockobject *s;
	int *len_ret;
{
	switch (s->sock_family) {

	case AF_UNIX:
	{
		*len_ret = sizeof (struct sockaddr_un);
		return 1;
	}

	case AF_INET:
	{
		*len_ret = sizeof (struct sockaddr_in);
		return 1;
	}

	/* More cases here... */

	default:
		err_setstr(SocketError, "getsockaddrarg: bad family");
		return 0;

	}
}


/* s.accept() method */

static object *
sock_accept(s, args)
	sockobject *s;
	object *args;
{
	char addrbuf[256];
	int addrlen, newfd;
	object *res;
	if (!getnoarg(args))
		return NULL;
	if (!getsockaddrlen(s, &addrlen))
		return NULL;
	newfd = accept(s->sock_fd, (struct sockaddr *) addrbuf, &addrlen);
	if (newfd < 0)
		return socket_error();
	/* Create the new object with unspecified family,
	   to avoid calls to bind() etc. on it. */
	res = makepair((object *) newsockobject(newfd,
						s->sock_family,
						s->sock_type,
						s->sock_proto),
		       makesockaddr((struct sockaddr *) addrbuf, addrlen));
	if (res == NULL)
		close(newfd);
	return res;
}


/* s.allowbroadcast() method */
/* XXX obsolete -- will disappear in next release */

static object *
sock_allowbroadcast(s, args)
	sockobject *s;
	object *args;
{
	int flag;
	int res;
	if (!getintarg(args, &flag))
		return NULL;
	res = setsockopt(s->sock_fd, SOL_SOCKET, SO_BROADCAST,
			 &flag, sizeof flag);
	if (res < 0)
		return socket_error();
	INCREF(None);
	return None;
}


/* s.setsockopt() method */
/* XXX this works for integer flags only */

static object *
sock_setsockopt(s, args)
	sockobject *s;
	object *args;
{
	int level;
	int optname;
	int flag;
	int res;

	if (!getargs(args, "(iii)", &level, &optname, &flag))
		return NULL;
	res = setsockopt(s->sock_fd, level, optname, &flag, sizeof flag);
	if (res < 0)
		return socket_error();
	INCREF(None);
	return None;
}


/* s.getsockopt() method */
/* XXX this works for integer flags only */

static object *
sock_getsockopt(s, args)
	sockobject *s;
	object *args;
{
	int level;
	int optname;
	int flag;
	int flagsize;
	int res;

	if (!getargs(args, "(ii)", &level, &optname))
		return NULL;
	flagsize = sizeof flag;
	flag = 0;
	res = getsockopt(s->sock_fd, level, optname, &flag, &flagsize);
	if (res < 0)
		return socket_error();
	return newintobject(flag);
}


/* s.avail() method */

static object *
sock_avail(s, args)
	sockobject *s;
	object *args;
{
	struct timeval timeout;
	fd_set readers;
	int n;
	if (!getnoarg(args))
		return NULL;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	FD_ZERO(&readers);
	FD_SET(s->sock_fd, &readers);
	n = select(s->sock_fd+1, &readers, (fd_set *)0, (fd_set *)0, &timeout);
	if (n < 0)
		return socket_error();
	return newintobject((long) (n != 0));
}


/* s.bind(sockaddr) method */

static object *
sock_bind(s, args)
	sockobject *s;
	object *args;
{
	struct sockaddr *addr;
	int addrlen;
	if (!getsockaddrarg(s, args, &addr, &addrlen))
		return NULL;
	if (bind(s->sock_fd, addr, addrlen) < 0)
		return socket_error();
	INCREF(None);
	return None;
}


/* s.close() method.
   Set the file descriptor to -1 so operations tried subsequently
   will surely fail. */

static object *
sock_close(s, args)
	sockobject *s;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	(void) close(s->sock_fd);
	s->sock_fd = -1;
	INCREF(None);
	return None;
}


/* s.connect(sockaddr) method */

static object *
sock_connect(s, args)
	sockobject *s;
	object *args;
{
	struct sockaddr *addr;
	int addrlen;
	if (!getsockaddrarg(s, args, &addr, &addrlen))
		return NULL;
	if (connect(s->sock_fd, addr, addrlen) < 0)
		return socket_error();
	INCREF(None);
	return None;
}


/* s.listen(n) method */

static object *
sock_listen(s, args)
	sockobject *s;
	object *args;
{
	int backlog;
	if (!getintarg(args, &backlog))
		return NULL;
	if (listen(s->sock_fd, backlog) < 0)
		return socket_error();
	INCREF(None);
	return None;
}


/* s.makefile(mode) method.
   Create a new open file object referring to a dupped version of
   the socket's file descriptor.  (The dup() call is necessary so
   that the open file and socket objects may be closed independent
   of each other.)
   The mode argument specifies 'r' or 'w' passed to fdopen(). */

static object *
sock_makefile(s, args)
	sockobject *s;
	object *args;
{
	extern int fclose PROTO((FILE *));
	object *mode;
	int fd;
	FILE *fp;
	if (!getStrarg(args, &mode))
		return NULL;
	if ((fd = dup(s->sock_fd)) < 0 ||
	    (fp = fdopen(fd, getstringvalue(mode))) == NULL)
		return socket_error();
	return newopenfileobject(fp, "<socket>", getstringvalue(mode), fclose);
}


/* s.recv(nbytes) method */

static object *
sock_recv(s, args)
	sockobject *s;
	object *args;
{
	int len, n, flags;
	object *buf;
	if (!getintintarg(args, &len, &flags)) {
		err_clear();
		if (!getintarg(args, &len))
			return NULL;
		flags = 0;
	}
	buf = newsizedstringobject((char *) 0, len);
	if (buf == NULL)
		return NULL;
	n = recv(s->sock_fd, getstringvalue(buf), len, flags);
	if (n < 0)
		return socket_error();
	if (resizestring(&buf, n) < 0)
		return NULL;
	return buf;
}


/* s.recvfrom(nbytes) method */

static object *
sock_recvfrom(s, args)
	sockobject *s;
	object *args;
{
	char addrbuf[256];
	object *buf;
	int addrlen, len, n;
	if (!getintarg(args, &len))
		return NULL;
	buf = newsizedstringobject((char *) 0, len);
	addrlen = sizeof addrbuf;
	n = recvfrom(s->sock_fd, getstringvalue(buf), len, 0,
		     addrbuf, &addrlen);
	if (n < 0)
		return socket_error();
	if (resizestring(&buf, n) < 0)
		return NULL;
	return makepair(buf,
			makesockaddr((struct sockaddr *)addrbuf, addrlen));
}


/* s.send(data) method */

static object *
sock_send(s, args)
	sockobject *s;
	object *args;
{
	object *buf;
	int len, n, flags;
	if (!getStrintarg(args, &buf, &flags)) {
		err_clear();
		if (!getStrarg(args, &buf))
			return NULL;
		flags = 0;
	}
	len = getstringsize(buf);
	n = send(s->sock_fd, getstringvalue(buf), len, flags);
	if (n < 0)
		return socket_error();
	INCREF(None);
	return None;
}


/* s.sendto(data, sockaddr) method */

static object *
sock_sendto(s, args)
	sockobject *s;
	object *args;
{
	object *buf;
	struct sockaddr *addr;
	int addrlen, len, n;
	if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 2) {
		err_badarg();
		return NULL;
	}
	if (!getStrarg(gettupleitem(args, 0), &buf) ||
	    !getsockaddrarg(s, gettupleitem(args, 1), &addr, &addrlen))
		return NULL;
	len = getstringsize(buf);
	n = sendto(s->sock_fd, getstringvalue(buf), len, 0,
		   addr, addrlen);
	if (n < 0)
		return socket_error();
	INCREF(None);
	return None;
}


/* s.shutdown(how) method */

static object *
sock_shutdown(s, args)
	sockobject *s;
	object *args;
{
	int how;
	if (!getintarg(args, &how))
		return NULL;
	if (shutdown(s->sock_fd, how) < 0)
		return socket_error();
	INCREF(None);
	return None;
}


/* List of methods for socket objects */

static struct methodlist sock_methods[] = {
	{"accept",	sock_accept},
	{"avail",	sock_avail},
	{"allowbroadcast",	sock_allowbroadcast},
	{"setsockopt",	sock_setsockopt},
	{"getsockopt",	sock_getsockopt},
	{"bind",	sock_bind},
	{"close",	sock_close},
	{"connect",	sock_connect},
	{"listen",	sock_listen},
	{"makefile",	sock_makefile},
	{"recv",	sock_recv},
	{"recvfrom",	sock_recvfrom},
	{"send",	sock_send},
	{"sendto",	sock_sendto},
	{"shutdown",	sock_shutdown},
	{NULL,		NULL}		/* sentinel */
};


/* Deallocate a socket object in response to the last DECREF().
   First close the file description. */

static void
sock_dealloc(s)
	sockobject *s;
{
	(void) close(s->sock_fd);
	DEL(s);
}


/* Return a socket object's named attribute. */

static object *
sock_getattr(s, name)
	sockobject *s;
	char *name;
{
	return findmethod(sock_methods, (object *) s, name);
}


/* Type object for socket objects.
   XXX This should be static, but some compilers don't grok the
   XXX forward reference to it in that case... */

typeobject Socktype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"socket",
	sizeof(sockobject),
	0,
	sock_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	sock_getattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};


/* Python interface to gethostname(). */

/*ARGSUSED*/
static object *
socket_gethostname(self, args)
	object *self;
	object *args;
{
	char buf[1024];
	if (!getnoarg(args))
		return NULL;
	if (gethostname(buf, (int) sizeof buf - 1) < 0)
		return socket_error();
	buf[sizeof buf - 1] = '\0';
	return newstringobject(buf);
}
/* Python interface to gethostbyname(name). */

/*ARGSUSED*/
static object *
socket_gethostbyname(self, args)
	object *self;
	object *args;
{
	object *name;
	struct sockaddr_in addrbuf;
	if (!getStrarg(args, &name))
		return NULL;
	if (setipaddr(getstringvalue(name), &addrbuf) < 0)
		return NULL;
	return makeipaddr(&addrbuf);
}


/* Python interface to getservbyname(name).
   This only returns the port number, since the other info is already
   known or not useful (like the list of aliases). */

/*ARGSUSED*/
static object *
socket_getservbyname(self, args)
	object *self;
	object *args;
{
	object *name, *proto;
	struct servent *sp;
	if (!getStrStrarg(args, &name, &proto))
		return NULL;
	sp = getservbyname(getstringvalue(name), getstringvalue(proto));
	if (sp == NULL) {
		err_setstr(SocketError, "service/proto not found");
		return NULL;
	}
	return newintobject((long) ntohs(sp->s_port));
}


/* Python interface to socket(family, type, proto).
   The third (protocol) argument is optional.
   Return a new socket object. */

/*ARGSUSED*/
static object *
socket_socket(self, args)
	object *self;
	object *args;
{
	sockobject *s;
	int family, type, proto, fd;
	if (args != NULL && is_tupleobject(args) && gettuplesize(args) == 3) {
		if (!getintintintarg(args, &family, &type, &proto))
			return NULL;
	}
	else {
		if (!getintintarg(args, &family, &type))
			return NULL;
		proto = 0;
	}
	fd = socket(family, type, proto);
	if (fd < 0)
		return socket_error();
	s = newsockobject(fd, family, type, proto);
	/* If the object can't be created, don't forget to close the
	   file descriptor again! */
	if (s == NULL)
		(void) close(fd);
	/* From now on, ignore SIGPIPE and let the error checking
	   do the work. */
	(void) signal(SIGPIPE, SIG_IGN);
	return (object *) s;
}


/* List of functions exported by this module. */

static struct methodlist socket_methods[] = {
	{"gethostbyname",	socket_gethostbyname},
	{"gethostname",		socket_gethostname},
	{"getservbyname",	socket_getservbyname},
	{"socket",		socket_socket},
	{NULL,			NULL}		 /* Sentinel */
};


/* Convenience routine to export an integer value.
   For simplicity, errors (which are unlikely anyway) are ignored. */

static void
insint(d, name, value)
	object *d;
	char *name;
	int value;
{
	object *v = newintobject((long) value);
	if (v == NULL) {
		/* Don't bother reporting this error */
		err_clear();
	}
	else {
		dictinsert(d, name, v);
		DECREF(v);
	}
}


/* Initialize this module.
   This is called when the first 'import socket' is done,
   via a table in config.c, if config.c is compiled with USE_SOCKET
   defined. */

void
initsocket()
{
	object *m, *d;

	m = initmodule("socket", socket_methods);
	d = getmoduledict(m);
	SocketError = newstringobject("socket.error");
	if (SocketError == NULL || dictinsert(d, "error", SocketError) != 0)
		fatal("can't define socket.error");
	insint(d, "AF_INET", AF_INET);
	insint(d, "AF_UNIX", AF_UNIX);
	insint(d, "SOCK_STREAM", SOCK_STREAM);
	insint(d, "SOCK_DGRAM", SOCK_DGRAM);
	insint(d, "SOCK_RAW", SOCK_RAW);
	insint(d, "SOCK_SEQPACKET", SOCK_SEQPACKET);
	insint(d, "SOCK_RDM", SOCK_RDM);
}
