/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

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

#include "allobjects.h"
#include "modsupport.h"		/* For getargs() etc. */

#include "macglue.h"
#include "tcpglue.h"

#include <Desk.h>

/* State of a tcp stream, in the connectionState field */
#define STATE_CLOSED	0
#define STATE_LISTEN	2
#define STATE_ESTAB		8
#define STATE_CWAIT		18

/* Python code has an additional reason for asr call: open done */
#define MY_OPEN_DONE	32766

static object *ErrorObject;

TCPIOCompletionUPP	upp_tcp_done;
TCPNotifyUPP		upp_tcp_asr;
#if 0
UDPIOCompletionUPP	upp_udp_done;
#endif
UDPNotifyUPP		upp_udp_asr;

/* ----------------------------------------------------- */
/* Declarations for objects of type MacTCP connection status */

typedef struct {
	OB_HEAD
	TCPStatusPB status;
} tcpcsobject;

staticforward typeobject Tcpcstype;

#define is_tcpcsobject(v)		((v)->ob_type == &Tcpcstype)

/* ---------------------------------------------------------------- */
/* Declarations for objects of type MacTCP global status */

#ifdef TCP_GS
typedef struct {
	OB_HEAD
	TCPParam *ptr;
} tcpgsobject;

staticforward typeobject Tcpgstype;

#define is_tcpgsobject(v)		((v)->ob_type == &Tcpgstype)
#endif /* TCP_GS */

/* ---------------------------------------------------------------- */
/* Declarations for objects of type MacTCP TCP stream */

typedef struct {
	OB_HEAD
	TCPiopb iop;
	long localhost;			/* Our IP address */
	short localport;		/* Our port number */
	object *asr;			/* Optional async notification routine */
	int asr_ec;				/* error code parameter to asr */
	int asr_reason;			/* detail for some errors */
	int async_busy;			/* True when completion routine pending */
	int async_err;			/* the error for the async call */
} tcpsobject;

staticforward typeobject Tcpstype;

#define is_tcpsobject(v)		((v)->ob_type == &Tcpstype)

/* ---------------------------------------------------------------- */
/* Declarations for objects of type MacTCP UDP stream */

typedef struct {
	OB_HEAD
	UDPiopb iop;
	object *asr;
	int asr_ec;				/* error code parameter to asr */
	ip_port port;
} udpsobject;

staticforward typeobject Udpstype;

#define is_udpsobject(v)		((v)->ob_type == &Udpstype)

/* ---------------------------------------------------------------- */

static tcpcsobject *
newtcpcsobject(ptr)
	TCPStatusPB *ptr;
{
	tcpcsobject *self;
	
	self = NEWOBJ(tcpcsobject, &Tcpcstype);
	if (self == NULL)
		return NULL;
	self->status = *ptr;
	return self;
}

static void
tcpcs_dealloc(self)
	tcpcsobject *self;
{
	DEL(self);
}
/* Code to access structure members by accessing attributes */

#include "structmember.h"

#define OFF(x) offsetof(TCPStatusPB, x)

static struct memberlist tcpcs_memberlist[] = {
	{"remoteHost", 		T_ULONG, 	OFF(remoteHost), 		RO},
	{"remotePort", 		T_USHORT, 	OFF(remotePort), 		RO},
	{"localHost", 		T_UINT, 	OFF(localHost), 		RO},
	{"localPort", 		T_USHORT, 	OFF(localPort), 		RO},
	{"tosFlags", 		T_BYTE, 	OFF(tosFlags), 			RO},
#if 0  /* Bug in header file: cannot access precedence */
	{"precedence" 		T_BYTE, 	OFF(precedence), 		RO},
#endif
	{"connectionState", T_BYTE, 	OFF(connectionState), 	RO},
	{"sendWindow", 		T_USHORT, 	OFF(sendWindow), 		RO},
	{"rcvWindow", 		T_USHORT, 	OFF(rcvWindow), 		RO},
	{"amtUnackedData", 	T_USHORT, 	OFF(amtUnackedData), 	RO},
	{"amtUnreadData", 	T_USHORT, 	OFF(amtUnreadData), 	RO},
	{"sendUnacked",		T_UINT, 	OFF(sendUnacked), 		RO},
	{"sendNext", 		T_UINT, 	OFF(sendNext), 			RO},
	{"congestionWindow", T_UINT, 	OFF(congestionWindow), 	RO},
	{"rcvNext", 		T_UINT, 	OFF(rcvNext), 			RO},
	{"srtt", 			T_UINT, 	OFF(srtt), 				RO},
	{"lastRTT",			T_UINT,		OFF(lastRTT),			RO},
	{"sendMaxSegSize",	T_UINT,		OFF(sendMaxSegSize),	RO},
	{NULL}	/* Sentinel */
};

static object *
tcpcs_getattr(self, name)
	tcpcsobject *self;
	char *name;
{
	object *rv;
	
	return getmember((char *)&self->status, tcpcs_memberlist, name);
}


static typeobject Tcpcstype = {
	OB_HEAD_INIT(&Typetype)
	0,							/*ob_size*/
	"MacTCP connection status",	/*tp_name*/
	sizeof(tcpcsobject),		/*tp_basicsize*/
	0,							/*tp_itemsize*/
	/* methods */
	(destructor)tcpcs_dealloc,	/*tp_dealloc*/
	(printfunc)0,				/*tp_print*/
	(getattrfunc)tcpcs_getattr,	/*tp_getattr*/
	(setattrfunc)0,				/*tp_setattr*/
	(cmpfunc)0,					/*tp_compare*/
	(reprfunc)0,				/*tp_repr*/
	0,							/*tp_as_number*/
	0,							/*tp_as_sequence*/
	0,							/*tp_as_mapping*/
	(hashfunc)0,				/*tp_hash*/
};

/* End of code for MacTCP connection status objects */
/* -------------------------------------------------------- */

#ifdef TCP_GS
static tcpgsobject *
newtcpgsobject(ptr)
	TCPParam *ptr;
{
	tcpgsobject *self;
	
	self = NEWOBJ(tcpgsobject, &Tcpgstype);
	if (self == NULL)
		return NULL;
	self->ptr = ptr;
	return self;
}

static void
tcpgs_dealloc(self)
	tcpgsobject *self;
{
	DEL(self);
}
/* Code to access structure members by accessing attributes */
#undef OFF
#define OFF(x) offsetof(TCPParam, x)

static struct memberlist tcpgs_memberlist[] = {
	{"RtoA",		T_UINT,	OFF(tcpRtoA),		RO},
	{"RtoMin",		T_UINT,	OFF(tcpRtoMin),		RO},
	{"RtoMax",		T_UINT,	OFF(tcpRtoMax),		RO},
	{"MaxSegSize",	T_UINT,	OFF(tcpMaxSegSize),	RO},
	{"MaxConn",		T_UINT,	OFF(tcpMaxConn),	RO},
	{"MaxWindow",	T_UINT,	OFF(tcpMaxWindow),	RO},
	{NULL}	/* Sentinel */
};

static object *
tcpgs_getattr(self, name)
	tcpgsobject *self;
	char *name;
{
	object *rv;
	
	return getmember((char *)self->ptr, tcpgs_memberlist, name);
}

static typeobject Tcpgstype = {
	OB_HEAD_INIT(&Typetype)
	0,				/*ob_size*/
	"MacTCP global status",			/*tp_name*/
	sizeof(tcpgsobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)tcpgs_dealloc,	/*tp_dealloc*/
	(printfunc)0,		/*tp_print*/
	(getattrfunc)tcpgs_getattr,	/*tp_getattr*/
	(setattrfunc)0,	/*tp_setattr*/
	(cmpfunc)0,		/*tp_compare*/
	(reprfunc)0,		/*tp_repr*/
	0,			/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)0,		/*tp_hash*/
};
#endif /* TCP_GS */

/* End of code for MacTCP global status objects */
/* -------------------------------------------------------- */

static int
tcps_checkstate(self, state, state2)
	tcpsobject *self;
	int state, state2;
{
	OSErr err;
	TCPStatusPB *pb;
	char buf[80];
	
	if ( self->async_busy ) {
		err_setstr(ErrorObject, "Operation not allowed, PassiveOpen in progress");
		return -1;
	}
	if ( state < 0 && state2 < 0 )
		return 0;
	err = xTCPStatus(&self->iop, &pb);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return -1;
	}
	if ( state == pb->connectionState ||
		 state2 == pb->connectionState )
		 return 0;
	sprintf(buf, "Operation not allowed, connection state=%d", pb->connectionState);
	err_setstr(ErrorObject, buf);
	return -1;
}

static int
tcps_asr_safe(arg)
	void *arg;
{
	tcpsobject *self = (tcpsobject *)arg;
	object *args, *rv;
	
	if ( self->asr == None )
		return;
	args = mkvalue("(ii)", self->asr_ec, self->asr_reason);
	rv = call_object(self->asr, args);
	DECREF(args);
	if ( rv ) {
		DECREF(rv);
		return 0;
	}
	return -1;
}

static pascal void
tcps_asr(str, ec, self, reason, icmp)
	StreamPtr str;
	unsigned short ec;
	tcpsobject *self;
	unsigned short reason;
	struct ICMPReport icmp;
{
	if ( self->asr == None )
		return;
	self->asr_ec = ec;
	self->asr_reason = reason;
	Py_AddPendingCall(tcps_asr_safe, (void *)self);
}

static void
tcps_done(pb)
	TCPiopb *pb;
{
	tcpsobject *self = (tcpsobject *)pb->csParam.open.userDataPtr;
	
	if ( pb != &self->iop || !self->async_busy ) {
		/* Oops... problems */
		printf("tcps_done: unexpected call\n");
		return;
	}
	self->async_busy = 0;
	self->async_err = pb->ioResult;
	/* Extension of mactcp semantics: also call asr on open complete */
	if ( self->asr == None )
		return;
	self->asr_ec = MY_OPEN_DONE;
	self->asr_reason = 0;
	Py_AddPendingCall(tcps_asr_safe, (void *)self);
}

static object *
tcps_isdone(self, args)
	tcpsobject *self;
	object *args;
{
	if (!newgetargs(args, ""))
		return NULL;
	return newintobject(!self->async_busy);
}

static object *
tcps_wait(self, args)
	tcpsobject *self;
	object *args;
{
	if (!newgetargs(args, ""))
		return NULL;
	while ( self->async_busy ) {
		if ( !PyMac_Idle() ) {
			INCREF(None);
			return None;
		}
	}
	if ( self->async_err ) {
		PyErr_Mac(ErrorObject, self->async_err);
		self->async_err = 0;
		return NULL;
	}
	INCREF(None);
	return None;
}


static object *
tcps_PassiveOpen(self, args)
	tcpsobject *self;
	object *args;
{
	short port;
	OSErr err;
	
	if (!newgetargs(args, "h", &port))
		return NULL;
	if ( tcps_checkstate(self, -1, -1) < 0 )
		return NULL;
	self->async_busy = 1;
	self->async_err = 0;
	err = xTCPPassiveOpen(&self->iop, port, upp_tcp_done,
						 (void *)self);
	if ( err ) {
		self->async_busy = 0;
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	self->localhost = self->iop.csParam.open.localHost;
	self->localport = self->iop.csParam.open.localPort;
	INCREF(None);
	return None;
}

static object *
tcps_ActiveOpen(self, args)
	tcpsobject *self;
	object *args;
{
	short lport, rport;
	long rhost;
	OSErr err;
	
	if (!newgetargs(args, "hlh", &lport, &rhost, &rport))
		return NULL;
	if ( tcps_checkstate(self, -1, -1) < 0 )
		return NULL;
	err = xTCPActiveOpen(&self->iop, lport, rhost, rport, (TCPIOCompletionUPP)0);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}	
	self->localhost = self->iop.csParam.open.localHost;
	self->localport = self->iop.csParam.open.localPort;
	INCREF(None);
	return None;
}

static object *
tcps_Send(self, args)
	tcpsobject *self;
	object *args;
{
	char *buf;
	int bufsize;
	int push = 0, urgent = 0;
	OSErr err;
	miniwds wds;
	
	if (!newgetargs(args, "s#|ii", &buf, &bufsize, &push, &urgent))
		return NULL;
	if ( tcps_checkstate(self, STATE_ESTAB, STATE_CWAIT) < 0 )
		return NULL;
	wds.length = bufsize;
	wds.ptr = buf;
	wds.terminus = 0;
	err = xTCPSend(&self->iop, (wdsEntry *)&wds, (Boolean)push, (Boolean)urgent,
					(TCPIOCompletionUPP)0);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	INCREF(None);
	return None;
}

static object *
tcps_Rcv(self, args)
	tcpsobject *self;
	object *args;
{
	int length;
	int timeout;
	rdsEntry rds[2];
	OSErr err;
	object *rv;
	int urgent, mark;
	
	if (!newgetargs(args, "i", &timeout))
		return NULL;
	if ( tcps_checkstate(self, -1, -1) < 0 )
		return NULL;
	memset((char *)&rds, 0, sizeof(rds));
	err = xTCPNoCopyRcv(&self->iop, rds, 1, timeout, (TCPIOCompletionUPP)0);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	urgent = self->iop.csParam.receive.urgentFlag;
	mark = self->iop.csParam.receive.markFlag;
	rv = newsizedstringobject((char *)rds[0].ptr, rds[0].length);
	err = xTCPBufReturn(&self->iop, rds, (TCPIOCompletionUPP)0);
	if ( err ) {
		/* Should not happen */printf("mactcp module: BufReturn failed?\n");
		PyErr_Mac(ErrorObject, err);
		DECREF(rv);
		return NULL;
	}
	return mkvalue("(Oii)", rv, urgent, mark);
}

static object *
tcps_Close(self, args)
	tcpsobject *self;
	object *args;
{
	OSErr err;
	
	if (!newgetargs(args, ""))
		return NULL;
	err = xTCPClose(&self->iop, (TCPIOCompletionUPP)0);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	INCREF(None);
	return None;
}

static object *
tcps_Abort(self, args)
	tcpsobject *self;
	object *args;
{
	OSErr err;
	
	if (!newgetargs(args, ""))
		return NULL;
	err = xTCPAbort(&self->iop);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	INCREF(None);
	return None;
}

static object *
tcps_Status(self, args)
	tcpsobject *self;
	object *args;
{
	OSErr err;
	TCPStatusPB *pb;
	
	if (!newgetargs(args, ""))
		return NULL;
	if ( tcps_checkstate(self, -1, -1) < 0 )
		return NULL;
	err = xTCPStatus(&self->iop, &pb);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (object *)newtcpcsobject(pb);
}

static object *
tcps_GetSockName(self, args)
	tcpsobject *self;
	object *args;
{
	/* This routine is needed so we can get at the local port even when
	** a PassiveOpen is in progress (when we can't do a Status call).
	** This is needed for socket listen(); getsockname(); accept() emulation
	** as used by ftp and the like.
	*/	
	if (!newgetargs(args, ""))
		return NULL;
	return mkvalue("(lh)", self->localhost, self->localport);
}

static struct methodlist tcps_methods[] = {
	{"isdone",	(method)tcps_isdone,	1},
	{"wait",	(method)tcps_wait,		1},
	{"PassiveOpen",	(method)tcps_PassiveOpen,	1},
	{"ActiveOpen",	(method)tcps_ActiveOpen,	1},
	{"Send",	(method)tcps_Send,	1},
	{"Rcv",	(method)tcps_Rcv,	1},
	{"Close",	(method)tcps_Close,	1},
	{"Abort",	(method)tcps_Abort,	1},
	{"Status",	(method)tcps_Status,	1},
	{"GetSockName", (method)tcps_GetSockName, 1},
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */

static object *
tcps_getattr(self, name)
	tcpsobject *self;
	char *name;
{
	if ( strcmp(name, "asr") == 0 ) {
		INCREF(self->asr);
		return self->asr;
	}
	return findmethod(tcps_methods, (object *)self, name);
}

static int
tcps_setattr(self, name, value)
	tcpsobject *self;
	char *name;
	object *value;
{
	if ( strcmp(name, "asr") != 0 || value == NULL )
		return -1;
	self->asr = value;	/* XXXX Assuming I don't have to incref */
	return 0;
}

static tcpsobject *
newtcpsobject(bufsize)
	int bufsize;
{
	tcpsobject *self;
	OSErr err;
	
	self = NEWOBJ(tcpsobject, &Tcpstype);
	if (self == NULL)
		return NULL;
	memset((char *)&self->iop, 0, sizeof(self->iop));
	err= xTCPCreate(bufsize, upp_tcp_asr, (void *)self, &self->iop);
	if ( err ) {
		DEL(self);
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	INCREF(None);
	self->asr = None;
	self->async_busy = 0;
	self->async_err = 0;
	return self;
}

static void
tcps_dealloc(self)
	tcpsobject *self;
{
	if ( self->async_busy ) {
		printf("mactcp module: error: dealloc with async busy\n");
		return;
	}
	xTCPRelease(&self->iop);
	DEL(self);
}

static typeobject Tcpstype = {
	OB_HEAD_INIT(&Typetype)
	0,							/*ob_size*/
	"MacTCP TCP stream",		/*tp_name*/
	sizeof(tcpsobject),			/*tp_basicsize*/
	0,							/*tp_itemsize*/
	/* methods */
	(destructor)tcps_dealloc,	/*tp_dealloc*/
	(printfunc)0,				/*tp_print*/
	(getattrfunc)tcps_getattr,	/*tp_getattr*/
	(setattrfunc)tcps_setattr,	/*tp_setattr*/
	(cmpfunc)0,					/*tp_compare*/
	(reprfunc)0,				/*tp_repr*/
	0,							/*tp_as_number*/
	0,							/*tp_as_sequence*/
	0,							/*tp_as_mapping*/
	(hashfunc)0,				/*tp_hash*/
};

/* End of code for MacTCP TCP stream objects */
/* -------------------------------------------------------- */

static int
udps_asr_safe(arg)
	void *arg;
{
	udpsobject *self = (udpsobject *)arg;
	object *args, *rv;
	
	if ( self->asr == None )
		return;
	args = mkvalue("(i)", self->asr_ec);
	rv = call_object(self->asr, args);
	DECREF(args);
	if ( rv ) {
		DECREF(rv);
		return 0;
	}
	return -1;
}

static pascal void
udps_asr(str, ec, self, icmp)
	StreamPtr str;
	unsigned short ec;
	udpsobject *self;
	struct ICMPReport icmp;
{
	if ( self->asr == None )
		return;
	self->asr_ec = ec;
	Py_AddPendingCall(udps_asr_safe, (void *)self);
}


static object *
udps_Read(self, args)
	udpsobject *self;
	object *args;
{
	OSErr err;
	object *rv;
	int timeout;
	
	if (!newgetargs(args, "i", &timeout))
		return NULL;
	err = xUDPRead(&self->iop, timeout, (UDPIOCompletionUPP)0);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	rv = newsizedstringobject((char *)self->iop.csParam.receive.rcvBuff,
								self->iop.csParam.receive.rcvBuffLen);
	err = xUDPBfrReturn(&self->iop, self->iop.csParam.receive.rcvBuff);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		DECREF(rv);
		return NULL;
	}
	return rv;
}

static object *
udps_Write(self, args)
	udpsobject *self;
	object *args;
{
	unsigned long host;
	unsigned short port;
	char *buf;
	int bufsize;
	OSErr err;
	miniwds wds;
	
	if (!newgetargs(args, "lhs#", &host, &port, &buf, &bufsize))
		return NULL;
	wds.length = bufsize;
	wds.ptr = buf;
	wds.terminus = 0;
	err = xUDPWrite(&self->iop, host, port, &wds, (UDPIOCompletionUPP)0);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	INCREF(None);
	return None;
}
static struct methodlist udps_methods[] = {
	{"Read",	(method)udps_Read,	1},
	{"Write",	(method)udps_Write,	1},
 
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */

static object *
udps_getattr(self, name)
	udpsobject *self;
	char *name;
{
	if ( strcmp(name, "asr") == 0 ) {
		INCREF(self->asr);
		return self->asr;
	}
	if ( strcmp(name, "port") == 0 ) 
		return newintobject((int)self->port);
	return findmethod(udps_methods, (object *)self, name);
}

static int
udps_setattr(self, name, value)
	udpsobject *self;
	char *name;
	object *value;
{
	if ( strcmp(name, "asr") != 0 || value == NULL )
		return -1;
	self->asr = value;	/* XXXX Assuming I don't have to incref */
	return 0;
}

static udpsobject *
newudpsobject(bufsize, port)
	int bufsize;
	int port;
{
	udpsobject *self;
	OSErr err;
	
	self = NEWOBJ(udpsobject, &Udpstype);
	if (self == NULL)
		return NULL;
	memset((char *)&self->iop, 0, sizeof(self->iop));
	self->port = port;
	err= xUDPCreate(&self->iop, bufsize, &self->port, upp_udp_asr,
					 (void *)self);
	if ( err ) {
		DEL(self);
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	INCREF(None);
	self->asr = None;
	return self;
}

static void
udps_dealloc(self)
	udpsobject *self;
{
	xUDPRelease(&self->iop);
	DEL(self);
}

static typeobject Udpstype = {
	OB_HEAD_INIT(&Typetype)
	0,							/*ob_size*/
	"MacTCP UDP stream",		/*tp_name*/
	sizeof(udpsobject),			/*tp_basicsize*/
	0,							/*tp_itemsize*/
	/* methods */
	(destructor)udps_dealloc,	/*tp_dealloc*/
	(printfunc)0,				/*tp_print*/
	(getattrfunc)udps_getattr,	/*tp_getattr*/
	(setattrfunc)udps_setattr,	/*tp_setattr*/
	(cmpfunc)0,					/*tp_compare*/
	(reprfunc)0,				/*tp_repr*/
	0,							/*tp_as_number*/
	0,							/*tp_as_sequence*/
	0,							/*tp_as_mapping*/
	(hashfunc)0,				/*tp_hash*/
};

/* End of code for MacTCP UDP stream objects */
/* -------------------------------------------------------- */

static object *
mactcp_TCPCreate(self, args)
	object *self;	/* Not used */
	object *args;
{
	OSErr err;
	object *rv;
	int bufsize;

	if (!newgetargs(args, "i", &bufsize))
		return NULL;
	if ( (err = xOpenDriver()) != noErr ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	rv = (object *)newtcpsobject(bufsize);
	return rv;
}

static object *
mactcp_UDPCreate(self, args)
	object *self;	/* Not used */
	object *args;
{
	OSErr err;
	object *rv;
	int bufsize, port;

	if (!newgetargs(args, "ii", &bufsize, &port))
		return NULL;
	if ( (err = xOpenDriver()) != noErr ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	rv = (object *)newudpsobject(bufsize, port);
	return rv;
}

static object *
mactcp_MTU(self, args)
	object *self;	/* Not used */
	object *args;
{
	OSErr err;
	unsigned short mtu;

	if (!newgetargs(args, ""))
		return NULL;
	if ( (err = xOpenDriver()) != noErr ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	mtu = xMaxMTU();
	return newintobject((int)mtu);
}

static object *
mactcp_IPAddr(self, args)
	object *self;	/* Not used */
	object *args;
{
	OSErr err;
	unsigned long rv;

	if (!newgetargs(args, ""))
		return NULL;
	if ( (err = xOpenDriver()) != noErr ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	rv = xIPAddr();
	return newintobject((int)rv);
}

static object *
mactcp_NetMask(self, args)
	object *self;	/* Not used */
	object *args;
{
	OSErr err;
	unsigned long rv;

	if (!newgetargs(args, ""))
		return NULL;
	if ( (err = xOpenDriver()) != noErr ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	rv = xNetMask();
	return newintobject((int)rv);
}

#ifdef TCP_GS
static object *
mactcp_GlobalInfo(self, args)
	object *self;	/* Not used */
	object *args;
{
	OSErr err;

	if (!newgetargs(args, ""))
		return NULL;
	if ( (err = xOpenDriver()) != noErr ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	/* XXXX Allocate, fill */
	INCREF(None);
	return None;
}
#endif /* TCP_GS */

/* List of methods defined in the module */

static struct methodlist mactcp_methods[] = {
	{"TCPCreate",	mactcp_TCPCreate,	1},
	{"UDPCreate",	mactcp_UDPCreate,	1},
	{"MTU",			mactcp_MTU,	1},
	{"IPAddr",		mactcp_IPAddr,	1},
	{"NetMask",		mactcp_NetMask,	1},
#ifdef TCP_GS
	{"GlobalInfo",	mactcp_GlobalInfo,	1},
#endif
 
	{NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initmactcp) */

void
initmactcp()
{
	object *m, *d;

	/* Create the module and add the functions */
	m = initmodule("mactcp", mactcp_methods);

	/* Add some symbolic constants to the module */
	d = getmoduledict(m);
	ErrorObject = newstringobject("mactcp.error");
	dictinsert(d, "error", ErrorObject);
	
	upp_tcp_done = NewTCPIOCompletionProc(tcps_done);
	upp_tcp_asr = NewTCPNotifyProc(tcps_asr);
#if 0
	upp_udp_done = NewUDPIOCompletionProc(udps_done);
#endif
	upp_udp_asr = NewUDPNotifyProc(udps_asr);

	/* XXXX Add constants here */
	
	/* Check for errors */
	if (err_occurred())
		fatal("can't initialize module mactcp");
}
