/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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

/* Note: This file is partially converted to the new naming standard */
/* ctbcm objects */


#include "allobjects.h"

#include "macglue.h"

#include <CommResources.h>
#include <Connections.h>
#include <ToolUtils.h>
#include <OSUtils.h>

#ifndef HAVE_UNIVERSAL_HEADERS
#define ConnectionCompletionUPP ProcPtr
#define ConnectionChooseIdleUPP ProcPtr
#define NewConnectionCompletionProc(x) (x)
#define NewConnectionChooseIdleProc(x) (x)
#endif

#define 	_UnimplementedToolTrap	0xA89F
#define 	_CommToolboxTrap		0x8B
#define 	_UnimplementedOSTrap	0x9F

extern object *PyErr_Mac(object *,int);

static object *ErrorObject;

typedef struct {
	OB_HEAD
	ConnHandle hdl;		/* The handle to the connection */
	object *callback;	/* Python callback routine */
	int has_callback;	/* True if callback not None */
	int err;			/* Error to pass to the callback */
} ctbcmobject;

staticforward typeobject ctbcmtype;

#define is_ctbcmobject(v)		((v)->ob_type == &ctbcmtype)

static
TrapAvailable(short tNumber, TrapType tType)
{
	short unImplemented;
	
	if (tType == OSTrap)
		unImplemented = _UnimplementedOSTrap;
	else
		unImplemented = _UnimplementedToolTrap;

	return NGetTrapAddress(tNumber, tType) != NGetTrapAddress(unImplemented, tType);
}

static
initialize_ctb()
{
	OSErr err;
	static initialized = -1;
	
	if ( initialized >= 0 )
		return initialized;
	initialized = 0;
	
	if ( !TrapAvailable(_CommToolboxTrap, OSTrap) ) {
		err_setstr(ErrorObject, "CTB not available");
		return 0;
	}
	if ( (err=InitCTBUtilities()) ) {
		PyErr_Mac(ErrorObject, (int)err);
		return 0;
	}
	if ( (err=InitCRM()) ) {
		PyErr_Mac(ErrorObject, (int)err);
		return 0;
	}
	if ( (err=InitCM()) ) {
		PyErr_Mac(ErrorObject, (int)err);
		return 0;
	}
	initialized = 1;
	return 1;
}

static int
ctbcm_pycallback(arg)
	void *arg;
{
	ctbcmobject *self = (ctbcmobject *)arg;
	object *args, *rv;
	
	if ( !self->has_callback )    /* It could have been removed in the meantime */
		return 0;
	args = mkvalue("(i)", self->err);
	rv = call_object(self->callback, args);
	DECREF(args);
	if( rv == NULL )
		return -1;
	DECREF(rv);
	return 0;
}

/*DBG*/int ncallback;
static pascal void
ctbcm_ctbcallback(hconn)
	ConnHandle hconn;
{
	ctbcmobject *self;
	
	/* XXXX Do I have to do the A5 mumbo-jumbo? */
	ncallback++; /*DBG*/
	self = (ctbcmobject *)CMGetUserData(hconn);
	self->err = (int)((*hconn)->errCode);
	Py_AddPendingCall(ctbcm_pycallback, (void *)self);
}

static ctbcmobject *
newctbcmobject(arg)
	object *arg;
{
	ctbcmobject *self;
	self = NEWOBJ(ctbcmobject, &ctbcmtype);
	if (self == NULL)
		return NULL;
	self->hdl = NULL;
	INCREF(None);
	self->callback = None;
	self->has_callback = 0;
	return self;
}

/* ctbcm methods */

static void
ctbcm_dealloc(self)
	ctbcmobject *self;
{
	if ( self->hdl ) {
		(void)CMClose(self->hdl, 0, (ConnectionCompletionUPP)0, 0, 1);
		/*XXXX Is this safe? */
		CMDispose(self->hdl);
		self->hdl = NULL;
	}
	DEL(self);
}

static object *
ctbcm_open(self, args)
	ctbcmobject *self;
	object *args;
{
	long timeout;
	OSErr err;
	ConnectionCompletionUPP cb_upp = NewConnectionCompletionProc(ctbcm_ctbcallback);
	
	if (!getargs(args, "l", &timeout))
		return NULL;
	if ( (err=CMOpen(self->hdl, self->has_callback, cb_upp, timeout)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	INCREF(None);
	return None;
}

static object *
ctbcm_listen(self, args)
	ctbcmobject *self;
	object *args;
{
	long timeout;
	OSErr err;
	ConnectionCompletionUPP cb_upp = NewConnectionCompletionProc(ctbcm_ctbcallback);
	
	if (!getargs(args, "l", &timeout))
		return NULL;
	if ( (err=CMListen(self->hdl,self->has_callback, cb_upp, timeout)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	INCREF(None);
	return None;
}

static object *
ctbcm_accept(self, args)
	ctbcmobject *self;
	object *args;
{
	int accept;
	OSErr err;
	
	if (!getargs(args, "i", &accept))
		return NULL;
	if ( (err=CMAccept(self->hdl, accept)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	INCREF(None);
	return None;
}

static object *
ctbcm_close(self, args)
	ctbcmobject *self;
	object *args;
{
	int now;
	long timeout;
	OSErr err;
	ConnectionCompletionUPP cb_upp = NewConnectionCompletionProc(ctbcm_ctbcallback);
	
	if (!getargs(args, "(li)", &timeout, &now))
		return NULL;
	if ( (err=CMClose(self->hdl, self->has_callback, cb_upp, timeout, now)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	INCREF(None);
	return None;
}

static object *
ctbcm_read(self, args)
	ctbcmobject *self;
	object *args;
{
	long timeout, len;
	int chan;
	CMFlags flags;
	OSErr err;
	object *rv;
	ConnectionCompletionUPP cb_upp = NewConnectionCompletionProc(ctbcm_ctbcallback);
	
	if (!getargs(args, "(lil)", &len, &chan, &timeout))
		return NULL;
	if ((rv=newsizedstringobject(NULL, len)) == NULL)
		return NULL;
	if ((err=CMRead(self->hdl, (Ptr)getstringvalue(rv), &len, (CMChannel)chan,
				self->has_callback, cb_upp, timeout, &flags)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	resizestring(&rv, len);
	return mkvalue("(Oi)", rv, (int)flags);
}

static object *
ctbcm_write(self, args)
	ctbcmobject *self;
	object *args;
{
	long timeout, len;
	int chan, ilen, flags;
	OSErr err;
	char *buf;
	ConnectionCompletionUPP cb_upp = NewConnectionCompletionProc(ctbcm_ctbcallback);
	
	if (!getargs(args, "(s#ili)", &buf, &ilen, &chan, &timeout, &flags))
		return NULL;
	len = ilen;
	if ((err=CMWrite(self->hdl, (Ptr)buf, &len, (CMChannel)chan,
				self->has_callback, cb_upp, timeout, (CMFlags)flags)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	return newintobject((int)len);
}

static object *
ctbcm_status(self, args)
	ctbcmobject *self;
	object *args;
{
	CMBufferSizes sizes;
	CMStatFlags flags;
	OSErr err;
	object *rv;
	
	if (!getnoarg(args))
		return NULL;
	if ((err=CMStatus(self->hdl, sizes, &flags)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	rv = mkvalue("(llllll)", sizes[0], sizes[1], sizes[2], sizes[3], sizes[4], sizes[5]);
	if ( rv == NULL )
		return NULL;
	return mkvalue("(Ol)", rv, (long)flags);
}

static object *
ctbcm_getconfig(self, args)
	ctbcmobject *self;
	object *args;
{
	char *rv;

	if (!getnoarg(args))
		return NULL;
	if ((rv=(char *)CMGetConfig(self->hdl)) == NULL ) {
		err_setstr(ErrorObject, "CMGetConfig failed");
		return NULL;
	}
	return newstringobject(rv);
}

static object *
ctbcm_setconfig(self, args)
	ctbcmobject *self;
	object *args;
{
	char *cfg;
	OSErr err;
	
	if (!getargs(args, "s", &cfg))
		return NULL;
	if ((err=CMSetConfig(self->hdl, (Ptr)cfg)) < 0)
		return PyErr_Mac(ErrorObject, err);
	return newintobject((int)err);
}

static object *
ctbcm_choose(self, args)
	ctbcmobject *self;
	object *args;
{
	int rv;
	Point pt;

	if (!getnoarg(args))
		return NULL;
	pt.v = 40;
	pt.h = 40;
	rv=CMChoose(&self->hdl, pt, (ConnectionChooseIdleUPP)0);
	return newintobject(rv);
}

static object *
ctbcm_idle(self, args)
	ctbcmobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	CMIdle(self->hdl);
	INCREF(None);
	return None;
}

static object *
ctbcm_abort(self, args)
	ctbcmobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	CMAbort(self->hdl);
	INCREF(None);
	return None;
}

static object *
ctbcm_reset(self, args)
	ctbcmobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	CMReset(self->hdl);
	INCREF(None);
	return None;
}

static object *
ctbcm_break(self, args)
	ctbcmobject *self;
	object *args;
{
	long duration;
	ConnectionCompletionUPP cb_upp = NewConnectionCompletionProc(ctbcm_ctbcallback);
	
	if (!getargs(args, "l", &duration))
		return NULL;
	CMBreak(self->hdl, duration,self->has_callback, cb_upp);
	INCREF(None);
	return None;
}

static struct methodlist ctbcm_methods[] = {
	{"Open",		(method)ctbcm_open},
	{"Close",		(method)ctbcm_close},
	{"Read",		(method)ctbcm_read},
	{"Write",		(method)ctbcm_write},
	{"Status",		(method)ctbcm_status},
	{"GetConfig",	(method)ctbcm_getconfig},
	{"SetConfig",	(method)ctbcm_setconfig},
	{"Choose",		(method)ctbcm_choose},
	{"Idle",		(method)ctbcm_idle},
	{"Listen",		(method)ctbcm_listen},
	{"Accept",		(method)ctbcm_accept},
	{"Abort",		(method)ctbcm_abort},
	{"Reset",		(method)ctbcm_reset},
	{"Break",		(method)ctbcm_break},
	{NULL,		NULL}		/* sentinel */
};

static object *
ctbcm_getattr(self, name)
	ctbcmobject *self;
	char *name;
{
	if ( strcmp(name, "callback") == 0 ) {
		INCREF(self->callback);
		return self->callback;
	}
	return findmethod(ctbcm_methods, (object *)self, name);
}

static int
ctbcm_setattr(self, name, v)
	ctbcmobject *self;
	char *name;
	object *v;
{
	if ( strcmp(name, "callback") != 0 ) {
		err_setstr(AttributeError, "ctbcm objects have callback attr only");
		return -1;
	}
	if ( v == NULL ) {
		v = None;
	}
	INCREF(v);	/* XXXX Must I do this? */
	self->callback = v;
	self->has_callback = (v != None);
	return 0;
}

statichere typeobject ctbcmtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"ctbcm",			/*tp_name*/
	sizeof(ctbcmobject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)ctbcm_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)ctbcm_getattr, /*tp_getattr*/
	(setattrfunc)ctbcm_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};
/* --------------------------------------------------------------------- */

/* Function of no arguments returning new ctbcm object */

static object *
ctb_cmnew(self, args)
	object *self; /* Not used */
	object *args;
{
	int strlen;
	object *sizes_obj;
	char *c_str;
	unsigned char p_str[255];
	CMBufferSizes sizes;
	short procid;
	ConnHandle hdl;
	ctbcmobject *rv;
	
	if (!getargs(args, "(s#O)", &c_str, &strlen, &sizes_obj))
		return NULL;
	strncpy((char *)p_str+1, c_str, strlen);
	p_str[0] = strlen;
	if (!initialize_ctb())
		return NULL;
	if ( sizes_obj == None ) {
		memset(sizes, '\0', sizeof sizes);
	} else {
		if ( !getargs(sizes_obj, "(llllll)", &sizes[0], &sizes[1], &sizes[2],
						&sizes[3], &sizes[4], &sizes[5]))
			return NULL;
	}
	if ( (procid=CMGetProcID(p_str)) < 0 )
		return PyErr_Mac(ErrorObject, procid);
	hdl = CMNew(procid, cmNoMenus|cmQuiet, sizes, 0, 0);
	if ( hdl == NULL ) {
		err_setstr(ErrorObject, "CMNew failed");
		return NULL;
	}
	rv = newctbcmobject(args);
	if ( rv == NULL )
	    return NULL;   /* XXXX Should dispose of hdl */
	rv->hdl = hdl;
	CMSetUserData(hdl, (long)rv);
	return (object *)rv;
}

static object *
ctb_available(self, args)
	object *self;
	object *args;
{
	int ok;
	
	if (!getnoarg(args))
		return NULL;
	ok = initialize_ctb();
	err_clear();
	return newintobject(ok);
}

/* List of functions defined in the module */

static struct methodlist ctb_methods[] = {
	{"CMNew",		ctb_cmnew},
	{"available",	ctb_available},
	{NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initctb) */

void
initctb()
{
	object *m, *d, *o;

	/* Create the module and add the functions */
	m = initmodule("ctb", ctb_methods);

	/* Add some symbolic constants to the module */
	d = getmoduledict(m);
	
#define CMCONST(name, value) o = newintobject(value); dictinsert(d, name, o)

	CMCONST("cmData", 1);
	CMCONST("cmCntl", 2);
	CMCONST("cmAttn", 3);
	
	CMCONST("cmFlagsEOM", 1);
	
	CMCONST("chooseDisaster", -2);
	CMCONST("chooseFailed", -1);
	CMCONST("chooseAborted", 0);
	CMCONST("chooseOKMinor", 1);
	CMCONST("chooseOKMajor", 2);
	CMCONST("chooseCancel", 3);
	
	CMCONST("cmStatusOpening", 1);
	CMCONST("cmStatusOpen", 2);
	CMCONST("cmStatusClosing", 4);
	CMCONST("cmStatusDataAvail", 8);
	CMCONST("cmStatusCntlAvail", 0x10);
	CMCONST("cmStatusAttnAvail", 0x20);
	CMCONST("cmStatusDRPend", 0x40);
	CMCONST("cmStatusDWPend", 0x80);
	CMCONST("cmStatusCWPend", 0x100);
	CMCONST("cmStatusCWPend", 0x200);
	CMCONST("cmStatusARPend", 0x400);
	CMCONST("cmStatusAWPend", 0x800);
	CMCONST("cmStatusBreakPending", 0x1000);
	CMCONST("cmStatusListenPend", 0x2000);
	CMCONST("cmStatusIncomingCallPresent", 0x4000);
	
	ErrorObject = newstringobject("ctb.error");
	dictinsert(d, "error", ErrorObject);

	/* Check for errors */
	if (err_occurred())
		fatal("can't initialize module ctb");
}
