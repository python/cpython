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

#include <AddressXlation.h>
#include <Desk.h>

#ifndef __MWERKS__
#define ResultUPP ResultProcPtr
#define NewResultProc(x) (x)
/* The '2' has move in this name... */
#define Result2UPP ResultProc2Ptr
#define NewResult2Proc(x) (x)
#endif

static object *ErrorObject;

/* ----------------------------------------------------- */
/* Declarations for objects of type MacTCP DNR Result */

/* Types of records we have */
#define DNR_ADDR 0
#define DNR_HINFO 1
#define DNR_MX 2

typedef struct {
	OB_HEAD
	int type;		/* DNR_XXX */
	int waiting;	/* True while completion proc not called */
	struct returnRec hinfo;
} dnrrobject;

staticforward typeobject Dnrrtype;

#define is_dnrrobject(v)		((v)->ob_type == &Dnrrtype)

/* ---------------------------------------------------------------- */

static pascal void
dnrr_done(rrp, udp)
	struct hostInfo *rrp;	/* Unused */
	dnrrobject *udp;
{
	if ( !udp->waiting ) {
		printf("macdnr: dnrr_done: spurious completion call!\n");
		return;
	}
	udp->waiting = 0;
	DECREF(udp);
}

static int dnrwait(self)
	dnrrobject *self;
{
	while ( self->waiting ) {
		if ( !PyMac_Idle() )
			return 0;
	}
	return 1;
}

static object *
dnrr_wait(self, args)
	dnrrobject *self;
	object *args;
{
	if (!newgetargs(args, ""))
		return NULL;
	if ( !dnrwait(self) ) {
		INCREF(None);
		return None;
	}
	if ( self->hinfo.rtnCode ) {
		PyErr_Mac(ErrorObject, self->hinfo.rtnCode);
		return NULL;
	}
	INCREF(None);
	return None;
}

static object *
dnrr_isdone(self, args)
	dnrrobject *self;
	object *args;
{
	if (!newgetargs(args, ""))
		return NULL;
	return newintobject(!self->waiting);
}

static struct methodlist dnrr_methods[] = {
	{"wait",	(method)dnrr_wait,	1},
	{"isdone",	(method)dnrr_isdone,	1},
 
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */

static dnrrobject *
newdnrrobject(tp)
	int tp;
{
	dnrrobject *self;
	
	self = NEWOBJ(dnrrobject, &Dnrrtype);
	if (self == NULL)
		return NULL;
	self->type = tp;
	self->waiting = 0;
	memset(&self->hinfo, 0, sizeof(self->hinfo));
	return self;
}

static void
dnrr_dealloc(self)
	dnrrobject *self;
{
	self->waiting = 0;  /* Not really needed, since we incref for completion */
	DEL(self);
}

/* Code to access structure members by accessing attributes */

#include "structmember.h"

#define OFF(x) offsetof(struct returnRec, x)

static struct memberlist dnrr_memberlist_addr[] = {
	{ "rtnCode", T_INT, OFF(rtnCode), RO},
	{ "cname", T_STRING_INPLACE, OFF(cname), RO},
	{ "ip0", T_UINT, OFF(rdata.addr[0]), RO},
	{ "ip1", T_UINT, OFF(rdata.addr[1]), RO},
	{ "ip2", T_UINT, OFF(rdata.addr[2]), RO},
	{ "ip3", T_UINT, OFF(rdata.addr[3]), RO},
	{NULL}	/* Sentinel */
};

static struct memberlist dnrr_memberlist_hinfo[] = {
	{ "rtnCode", T_INT, OFF(rtnCode), RO},
	{ "cname", T_STRING_INPLACE, OFF(cname), RO},
	{ "cpuType", T_STRING_INPLACE, OFF(rdata.hinfo.cpuType), RO},
	{ "osType", T_STRING_INPLACE, OFF(rdata.hinfo.osType), RO},
	{NULL}	/* Sentinel */
};

static struct memberlist dnrr_memberlist_mx[] = {
	{ "rtnCode", T_INT, OFF(rtnCode), RO},
	{ "cname", T_STRING_INPLACE, OFF(cname), RO},
	{ "preference", T_USHORT, OFF(rdata.mx.preference), RO},
	{ "exchange", T_STRING_INPLACE, OFF(rdata.mx.exchange), RO},
	{NULL}	/* Sentinel */
};

static struct memberlist *dnrr_mlists[3] = {
	dnrr_memberlist_addr,
	dnrr_memberlist_hinfo,
	dnrr_memberlist_mx
};

static object *
dnrr_getattr(self, name)
	dnrrobject *self;
	char *name;
{
	object *rv;
	int tp;
	
	rv = findmethod(dnrr_methods, (object *)self, name);
	if ( rv ) return rv;
	err_clear();
	if ( self->waiting )
		if ( !dnrwait(self) ) {
			err_setstr(ErrorObject, "Resolver busy");
			return NULL;
		}
	tp = self->type;
	return getmember((char *)&self->hinfo, dnrr_mlists[tp], name);
}


static typeobject Dnrrtype = {
	OB_HEAD_INIT(&Typetype)
	0,				/*ob_size*/
	"MacTCP DNR Result",			/*tp_name*/
	sizeof(dnrrobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)dnrr_dealloc,	/*tp_dealloc*/
	(printfunc)0,		/*tp_print*/
	(getattrfunc)dnrr_getattr,	/*tp_getattr*/
	(setattrfunc)0,	/*tp_setattr*/
	(cmpfunc)0,		/*tp_compare*/
	(reprfunc)0,		/*tp_repr*/
	0,			/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)0,		/*tp_hash*/
};

/* End of code for MacTCP DNR Result objects */
/* -------------------------------------------------------- */

int dnr_is_open;

static int
opendnr(fn)
	char *fn;
{
	OSErr err;
	
	if ( dnr_is_open ) return 1;
	if ( (err=OpenResolver(fn)) ) {
		PyErr_Mac(ErrorObject, err);
		return 0;
	}
	dnr_is_open = 1;
	return 1;
}
	
static object *
dnr_Open(self, args)
	object *self;	/* Not used */
	object *args;
{
	char *fn = NULL;

	if (!newgetargs(args, "|s", &fn))
		return NULL;
	if ( dnr_is_open ) {
		err_setstr(ErrorObject, "DNR already open");
		return NULL;
	}
	if ( !opendnr(fn) )
		return NULL;
	INCREF(None);
	return None;
}

static object *
dnr_Close(self, args)
	object *self;	/* Not used */
	object *args;
{
	OSErr err;

	if (!newgetargs(args, ""))
		return NULL;
	dnr_is_open = 0;
	if ( (err=CloseResolver()) ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	INCREF(None);
	return None;
}

static object *
dnr_StrToAddr(self, args)
	object *self;	/* Not used */
	object *args;
{
	OSErr err;
	char *hostname;
	dnrrobject *rv;
	ResultUPP cb_upp = NewResultProc(dnrr_done);

	if (!newgetargs(args, "s", &hostname))
		return NULL;
	if ( !opendnr(NULL) )
		return NULL;
	if ( (rv=newdnrrobject(DNR_ADDR)) == NULL )
		return NULL;
	err = StrToAddr(hostname, (struct hostInfo *)&rv->hinfo, cb_upp, (char *)rv);
	if ( err == cacheFault ) {
		rv->waiting++;
		INCREF(rv);
	} else if ( err ) {
		DECREF(rv);
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (object *)rv;
}

static object *
dnr_AddrToName(self, args)
	object *self;	/* Not used */
	object *args;
{
	OSErr err;
	unsigned long ipaddr;
	dnrrobject *rv;
	ResultUPP cb_upp = NewResultProc(dnrr_done);

	if (!newgetargs(args, "l", &ipaddr))
		return NULL;
	if ( !opendnr(NULL) )
		return NULL;
	if ( (rv=newdnrrobject(DNR_ADDR)) == NULL )
		return NULL;
	err = AddrToName(ipaddr, (struct hostInfo *)&rv->hinfo, cb_upp, (char *)rv);
	if ( err == cacheFault ) {
		rv->waiting++;
		INCREF(rv);
	} else if ( err ) {
		DECREF(rv);
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (object *)rv;
}

static object *
dnr_AddrToStr(self, args)
	object *self;	/* Not used */
	object *args;
{
	OSErr err;
	unsigned long ipaddr;
	char ipname[16];

	if (!newgetargs(args, "l", &ipaddr))
		return NULL;
	if ( !opendnr(NULL) )
		return NULL;
	if ( (err=AddrToStr(ipaddr, ipname)) ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return newstringobject(ipname);
}

static object *
dnr_HInfo(self, args)
	object *self;	/* Not used */
	object *args;
{
	OSErr err;
	char *hostname;
	dnrrobject *rv;
	Result2UPP cb_upp = NewResult2Proc(dnrr_done);

	if (!newgetargs(args, "s", &hostname))
		return NULL;
	if ( !opendnr(NULL) )
		return NULL;
	if ( (rv=newdnrrobject(DNR_HINFO)) == NULL )
		return NULL;
	err = HInfo(hostname, &rv->hinfo, cb_upp, (char *)rv);
	if ( err == cacheFault ) {
		rv->waiting++;
		INCREF(rv);
	} else if ( err ) {
		DECREF(rv);
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (object *)rv;

	if (!newgetargs(args, ""))
		return NULL;
	INCREF(None);
	return None;
}

static object *
dnr_MXInfo(self, args)
	object *self;	/* Not used */
	object *args;
{
	OSErr err;
	char *hostname;
	dnrrobject *rv;
	Result2UPP cb_upp = NewResult2Proc(dnrr_done);

	if (!newgetargs(args, "s", &hostname))
		return NULL;
	if ( !opendnr(NULL) )
		return NULL;
	if ( (rv=newdnrrobject(DNR_MX)) == NULL )
		return NULL;
	err = MXInfo(hostname, &rv->hinfo, cb_upp, (char *)rv);
	if ( err == cacheFault ) {
		rv->waiting++;
		INCREF(rv);
	} else if ( err ) {
		DECREF(rv);
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (object *)rv;
}

/* List of methods defined in the module */

static struct methodlist dnr_methods[] = {
	{"Open",	dnr_Open,	1},
	{"Close",	dnr_Close,	1},
	{"StrToAddr",	dnr_StrToAddr,	1},
	{"AddrToStr",	dnr_AddrToStr,	1},
	{"AddrToName",	dnr_AddrToName,	1},
	{"HInfo",	dnr_HInfo,	1},
	{"MXInfo",	dnr_MXInfo,	1},
 
	{NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initmacdnr) */

void
initmacdnr()
{
	object *m, *d;

	/* Create the module and add the functions */
	m = initmodule("macdnr", dnr_methods);

	/* Add some symbolic constants to the module */
	d = getmoduledict(m);
	ErrorObject = newstringobject("macdnr.error");
	dictinsert(d, "error", ErrorObject);
#if 0
/* Not needed, after all */
#define CONST(name, value) o = newintobject(value); dictinsert(d, name, o);
	CONST("ADDR", DNR_ADDR);
	CONST("HINFO", DNR_HINFO);
	CONST("MX", DNR_MX);
#endif
	/* Check for errors */
	if (err_occurred())
		fatal("can't initialize module macdnr");
}
