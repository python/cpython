/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
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

/*
This module provides an interface to Berkeley socket IPC.

Limitations:

- only AF_INET and AF_UNIX address families
- no asynchronous I/O
- no flags on send/receive operations
- no socket options
- no protocol parameter on socket() system call
- although socket objects have read() and write() methods, they can't be
  used everywhere where file objects can be used (e.g., sys.stdout won't
  work and there is no readline() method)

Interface:

- socket.socket(family, type) returns a new socket object
- family and type constants from <socket.h> are accessed as socket.AF_INET etc.
- socket methods are:
	- s.bind(sockaddr) --> None
	- s.connect(sockaddr) --> None
	- s.accept() --> newsocket, sockaddr
	- s.listen(n) --> None
	- s.read(nbytes) --> string
	- s.write(string) --> nbytes
	- s.recvfrom(nbytes) --> string, sockaddr
	- s.sendto(string, sockaddr) --> nbytes
	- s.shutdown(how) --> None
	- s.close() --> None
- errors are reported as the exception socket.error
- an Internet socket address is a pair (hostname, port)
  where hostname can be anything recognized by gethostbyname()
  (including the dd.dd.dd.dd notation) and port is in host byte order
- where a hostname is returned, the dd.dd.dd.dd notation is used
- a UNIX domain socket is a string specifying the pathname

Bugs:

- On the vax, the port numbers seem to be mixed up (when receiving only???)
*/

#include "allobjects.h"
#include "modsupport.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>

static object *SocketError; /* Exception socket.error */

static object *
socket_error()
{
	return err_errno(SocketError);
}

typedef struct {
	OB_HEAD
	int sock_fd;
	int sock_family;
	int sock_type;
	int sock_proto;
} sockobject;

extern typeobject Socktype; /* Forward */

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

static int setinetaddr PROTO((char *, struct sockaddr_in *));
static int
setinetaddr(name, addr_ret)
	char *name;
	struct sockaddr_in *addr_ret;
{
	struct hostent *hp;

	if (strcmp(name, "<any>") == 0) {
		addr_ret->sin_addr.s_addr = INADDR_ANY;
		return 4;
	}
	if (strcmp(name, "<broadcast>") == 0) {
		addr_ret->sin_addr.s_addr = INADDR_BROADCAST;
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

/*ARGSUSED*/
static object *
makesockaddr(addr, addrlen)
	struct sockaddr *addr;
	int addrlen;
{
	if (addr->sa_family == AF_INET) {
		struct sockaddr_in *a = (struct sockaddr_in *) addr;
		long x = ntohl(a->sin_addr.s_addr);
		char buf[100];
		sprintf(buf, "%d.%d.%d.%d",
			(int) (x>>24) & 0xff, (int) (x>>16) & 0xff,
			(int) (x>> 8) & 0xff, (int) (x>> 0) & 0xff);
		return makepair(newstringobject(buf),
				newintobject((long) ntohs(a->sin_port)));
	}
	if (addr->sa_family == AF_UNIX) {
		struct sockaddr_un *a = (struct sockaddr_un *) addr;
		return newstringobject(a->sun_path);
	}
	err_setstr(SocketError, "returning unknown socket address type");
	return NULL;
}

static int getsockaddrarg PROTO((sockobject *, object *,
				 struct sockaddr **, int *));
static int
getsockaddrarg(s, args, addr_ret, len_ret)
	sockobject *s;
	object *args;
	struct sockaddr **addr_ret;
	int *len_ret;
{
	if (s->sock_family == AF_UNIX) {
		static struct sockaddr_un addr;
		object *path;
		int len;
		if (!getstrarg(args, &path) ||
		    (len = getstringsize(path)) > sizeof addr.sun_path)
			return 0;
		addr.sun_family = AF_UNIX;
		memcpy(addr.sun_path, getstringvalue(path), len);
		*addr_ret = (struct sockaddr *) &addr;
		*len_ret = len + sizeof addr.sun_family;
		return 1;
	}

	if (s->sock_family == AF_INET) {
		static struct sockaddr_in addr;
		object *host;
		int port;
		if (!getstrintarg(args, &host, &port))
			return 0;
		if (setinetaddr(getstringvalue(host), &addr) < 0)
			return 0;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		*addr_ret = (struct sockaddr *) &addr;
		*len_ret = sizeof addr;
		return 1;
	}

	err_setstr(SocketError, "getsockaddrarg: bad family");
	return 0;
}

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
	addrlen = sizeof addrbuf;
	newfd = accept(s->sock_fd, (struct sockaddr *) addrbuf, &addrlen);
	if (newfd < 0)
		return socket_error();
	res = makepair((object *) newsockobject(newfd, AF_UNSPEC, 0, 0),
		       makesockaddr((struct sockaddr *) addrbuf, addrlen));
	if (res == NULL)
		close(newfd);
	return res;
}

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

static object *
sock_read(s, args)
	sockobject *s;
	object *args;
{
	int len, n;
	object *buf;
	if (!getintarg(args, &len))
		return NULL;
	buf = newsizedstringobject((char *) 0, len);
	if (buf == NULL)
		return NULL;
	n = read(s->sock_fd, getstringvalue(buf), len);
	if (n < 0)
		return socket_error();
	if (resizestring(&buf, n) < 0)
		return NULL;
	return buf;
}

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
	return makepair(buf, makesockaddr(addrbuf, addrlen));
}

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
	if (!getstrarg(gettupleitem(args, 0), &buf) ||
	    !getsockaddrarg(s, gettupleitem(args, 1), &addr, &addrlen))
		return NULL;
	len = getstringsize(buf);
	n = sendto(s->sock_fd, getstringvalue(buf), len, 0,
		   addr, addrlen);
	if (n < 0)
		return socket_error();
	return newintobject((long) n);
}

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

static object *
sock_write(s, args)
	sockobject *s;
	object *args;
{
	object *buf;
	int len, n;
	if (!getstrarg(args, &buf))
		return NULL;
	len = getstringsize(buf);
	n = write(s->sock_fd, getstringvalue(buf), len);
	if (n < 0)
		return socket_error();
	return newintobject((long) n);
}

static struct methodlist sock_methods[] = {
	{"accept",	sock_accept},
	{"bind",	sock_bind},
	{"close",	sock_close},
	{"connect",	sock_connect},
	{"listen",	sock_listen},
	{"read",	sock_read},
	{"recvfrom",	sock_recvfrom},
	{"sendto",	sock_sendto},
	{"shutdown",	sock_shutdown},
	{"write",	sock_write},
	{NULL,		NULL}		/* sentinel */
};

static void
sock_dealloc(s)
	sockobject *s;
{
	(void) close(s->sock_fd);
	DEL(s);
}

static object *
sock_getattr(s, name)
	sockobject *s;
	char *name;
{
	return findmethod(sock_methods, (object *) s, name);
}

static typeobject Socktype = {
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

/*ARGSUSED*/
static object *
socket_socket(self, args)
	object *self;
	object *args;
{
	sockobject *s;
	int family, type, proto, fd;
	if (!getintintarg(args, &family, &type))
		return NULL;
	proto = 0;
	fd = socket(family, type, proto);
	if (fd < 0)
		return socket_error();
	s = newsockobject(fd, family, type, proto);
	if (s == NULL)
		close(fd);
	return (object *) s;
}

static struct methodlist socket_methods[] = {
	{"socket",	socket_socket},
	{NULL,		NULL}		 /* Sentinel */
};

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
