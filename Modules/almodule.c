/**********************************************************
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

/* AL module -- interface to Mark Calows' Auido Library (AL). */

#include "audio.h"

#include "allobjects.h"
#include "import.h"
#include "modsupport.h"
#include "structmember.h"


/* Config objects */

typedef struct {
	OB_HEAD
	ALconfig ob_config;
} configobject;

extern typeobject Configtype; /* Forward */

#define is_configobject(v) ((v)->ob_type == &Configtype)

static object *
setConfig (self, args, func)
	configobject *self;
	object *args;
	void (*func)(ALconfig, long);
{
	long par;

	if (!getlongarg(args, &par)) return NULL;

	(*func) (self-> ob_config, par);

	INCREF (None);
	return None;
}

static object *
getConfig (self, args, func)
	configobject *self;
	object *args;
	long (*func)(ALconfig);
{	
	long par;

	if (!getnoarg(args)) return NULL;
	
	par = (*func) (self-> ob_config);

	return newintobject (par);
}

static object *
al_setqueuesize (self, args)
	configobject *self;
	object *args;
{
	return (setConfig (self, args, ALsetqueuesize));
}

static object *
al_getqueuesize (self, args)
	configobject *self;
	object *args;
{
	return (getConfig (self, args, ALgetqueuesize));
}

static object *
al_setwidth (self, args)
	configobject *self;
	object *args;
{
	return (setConfig (self, args, ALsetwidth));
}

static object *
al_getwidth (self, args)
	configobject *self;
	object *args;
{
	return (getConfig (self, args, ALgetwidth));	
}

static object *
al_getchannels (self, args)
	configobject *self;
	object *args;
{
	return (getConfig (self, args, ALgetchannels));	
}

static object *
al_setchannels (self, args)
	configobject *self;
	object *args;
{
	return (setConfig (self, args, ALsetchannels));
}

static struct methodlist config_methods[] = {
	{"getqueuesize",	al_getqueuesize},
	{"setqueuesize",	al_setqueuesize},
	{"getwidth",		al_getwidth},
	{"setwidth",		al_setwidth},
	{"getchannels",		al_getchannels},
	{"setchannels",		al_setchannels},
	{NULL,			NULL}		/* sentinel */
};

static void
config_dealloc(self)
	configobject *self;
{
	ALfreeconfig(self->ob_config);
	DEL(self);
}

static object *
config_getattr(self, name)
	configobject *self;
	char *name;
{
	return findmethod(config_methods, (object *)self, name);
}

typeobject Configtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"config",		/*tp_name*/
	sizeof(configobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	config_dealloc,		/*tp_dealloc*/
	0,			/*tp_print*/
	config_getattr,		/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

static object *
newconfigobject(config)
	ALconfig config;
{
	configobject *p;
	
	p = NEWOBJ(configobject, &Configtype);
	if (p == NULL)
		return NULL;
	p->ob_config = config;
	return (object *)p;
}

/* Port objects */

typedef struct {
	OB_HEAD
	ALport ob_port;
} portobject;

extern typeobject Porttype; /* Forward */

#define is_portobject(v) ((v)->ob_type == &Porttype)

static object *
al_closeport (self, args)
	portobject *self;
	object *args;
{
	if (!getnoarg(args)) return NULL;

	if (self->ob_port != NULL) {
		ALcloseport (self-> ob_port);
		self->ob_port = NULL;
		/* XXX Using a closed port may dump core! */
	}

	INCREF (None);
	return None;
}

static object *
al_getfd (self, args)
	portobject *self;
	object *args;
{
	int fd;

	if (!getnoarg(args)) return NULL;

	fd = ALgetfd (self-> ob_port);

	return newintobject (fd);
}

static object *
al_getfilled (self, args)
	portobject *self;
	object *args;
{
	long count;

	if (!getnoarg(args)) return NULL;
	
	count = ALgetfilled (self-> ob_port);

	return newintobject (count);
}

static object *
al_getfillable (self, args)
	portobject *self;
	object *args;
{
	long count;

	if (!getnoarg(args)) return NULL;
	
	count = ALgetfillable (self-> ob_port);

	return newintobject (count);
}

static object *
al_readsamps (self, args)
	portobject *self;
	object *args;
{
	long count;
	object *v;
	ALconfig c;
	int width;

	if (!getlongarg (args, &count)) return NULL;

	if (count <= 0)
	{
		err_setstr (RuntimeError, "al.readsamps : arg <= 0");
		return NULL;
	}

	c = ALgetconfig(self->ob_port);
	width = ALgetwidth(c);
	ALfreeconfig(c);
	v = newsizedstringobject ((char *)NULL, width * count);
	if (v == NULL) return NULL;

	ALreadsamps (self-> ob_port, (void *) getstringvalue(v), count);

	return (v);
}

static object *
al_writesamps (self, args)
	portobject *self;
	object *args;
{
	long count;
	object *v;
	ALconfig c;
	int width;

	if (!getstrarg (args, &v)) return NULL;

	c = ALgetconfig(self->ob_port);
	width = ALgetwidth(c);
	ALfreeconfig(c);
	ALwritesamps (self-> ob_port, (void *) getstringvalue(v),
		      getstringsize(v) / width);

	INCREF (None);
	return None;
}

static object *
al_getfillpoint (self, args)
	portobject *self;
	object *args;
{
	long count;

	if (!getnoarg(args)) return NULL;
	
	count = ALgetfillpoint (self-> ob_port);

	return newintobject (count);
}

static object *
al_setfillpoint (self, args)
	portobject *self;
	object *args;
{
	long count;

	if (!getlongarg(args, &count)) return NULL;
	
	ALsetfillpoint (self-> ob_port, count);

	INCREF (None);
	return (None);
}

static object *
al_setconfig (self, args)
	portobject *self;
	object *args;
{
	ALconfig config;

	if (!getconfigarg(args, &config)) return NULL;
	
	ALsetconfig (self-> ob_port, config);

	INCREF (None);
	return (None);
}

static object *
al_getconfig (self, args)
	portobject *self;
	object *args;
{
	ALconfig config;

	if (!getnoarg(args)) return NULL;
	
	config = ALgetconfig (self-> ob_port);

	return newconfigobject (config);
}

static struct methodlist port_methods[] = {
	{"closeport",		al_closeport},
	{"getfd",		al_getfd},
	{"getfilled",		al_getfilled},
	{"getfillable",		al_getfillable},
	{"readsamps",		al_readsamps},
	{"writesamps",		al_writesamps},
	{"setfillpoint",	al_setfillpoint},
	{"getfillpoint",	al_getfillpoint},
	{"setconfig",		al_setconfig},
	{"getconfig",		al_getconfig},
	{NULL,			NULL}		/* sentinel */
};

static void
port_dealloc(p)
	portobject *p;
{
	if (p->ob_port != NULL)
		ALcloseport(p->ob_port);
	DEL(p);
}

static object *
port_getattr(p, name)
	portobject *p;
	char *name;
{
	return findmethod(port_methods, (object *)p, name);
}

typeobject Porttype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"port",			/*tp_name*/
	sizeof(portobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	port_dealloc,		/*tp_dealloc*/
	0,			/*tp_print*/
	port_getattr,		/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

static object *
newportobject(port)
	ALport port;
{
	portobject *p;
	
	p = NEWOBJ(portobject, &Porttype);
	if (p == NULL)
		return NULL;
	p->ob_port = port;
	return (object *)p;
}

/* the module al */

static object *
al_openport (self, args)
	object *self, *args;
{
	object *name, *dir;
	ALport port;
	ALconfig config = NULL;
	int size = gettuplesize(args);

	if (size == 2) {
		if (!getstrstrarg (args, &name, &dir))
			return NULL;
	}
	else if (size == 3) {
		if (!getstrstrconfigarg (args, &name, &dir, &config))
			return NULL;
	}
	else {
		err_badarg();
		return NULL;
	}

	port = ALopenport(getstringvalue(name), getstringvalue(dir), config);

	return newportobject (port);
}

static object *
al_newconfig (self, args)
	object *self, *args;
{
	ALconfig config;

	if (!getnoarg (args)) return NULL;

	config = ALnewconfig ();

	return newconfigobject (config);
}

static object *
al_queryparams(self, args)
	object *self, *args;
{
	long device;
	long length;
	long *PVbuffer;
	long PVdummy[2];
	object *v;
	object *w;

	if (!getlongarg(args, &device))
		return NULL;
	length = ALqueryparams(device, PVdummy, 2L);
	PVbuffer = NEW(long, length);
	if (PVbuffer == NULL)
		return err_nomem();
	(void) ALqueryparams(device, PVbuffer, length);
	v = newlistobject((int)length);
	if (v != NULL) {
		int i;
		for (i = 0; i < length; i++)
			setlistitem(v, i, newintobject(PVbuffer[i]));
	}
	DEL(PVbuffer);
	return v;
}

static object *
doParams(args, func, modified)
	object *args;
	void (*func)(long, long *, long);
	int modified;
{
	long device;
	object *list, *v;
	long *PVbuffer;
	long length;
	int i;
	
	if (!getlongobjectarg(args, &device, &list))
		return NULL;
	if (!is_listobject(list)) {
		err_badarg();
		return NULL;
	}
	length = getlistsize(list);
	PVbuffer = NEW(long, length);
	if (PVbuffer == NULL)
		return err_nomem();
	for (i = 0; i < length; i++) {
		v = getlistitem(list, i);
		if (!is_intobject(v)) {
			DEL(PVbuffer);
			err_badarg();
			return NULL;
		}
		PVbuffer[i] = getintvalue(v);
	}

	ALgetparams(device, PVbuffer, length);

	if (modified) {
		for (i = 0; i < length; i++)
			setlistitem(list, i, newintobject(PVbuffer[i]));
	}

	INCREF(None);
	return None;
}

static object *
al_getparams(self, args)
	object *self, *args;
{
	return doParams(args, ALgetparams, 1);
}

static object *
al_setparams(self, args)
	object *self, *args;
{
	return doParams(args, ALsetparams, 0);
}

static struct methodlist al_methods[] = {
	{"openport",		al_openport},
	{"newconfig",		al_newconfig},
	{"queryparams",		al_queryparams},
	{"getparams",		al_getparams},
	{"setparams",		al_setparams},
	{NULL,			NULL}		/* sentinel */
};

void
inital()
{
	initmodule("al", al_methods);
}

int
getconfigarg (o, conf)
	configobject *o;
	ALconfig *conf;
{
	if (o == NULL || !is_configobject(o))
		return err_badarg ();
	
	*conf = o-> ob_config;
	
	return 1;
}

int
getstrstrconfigarg(v, a, b, c)
	object *v;
	object **a;
	object **b;
	ALconfig *c;
{
	if (v == NULL || !is_tupleobject(v) || gettuplesize(v) != 3) {
		return err_badarg();
	}
	
	return getstrarg(gettupleitem(v, 0), a) &&
		getstrarg(gettupleitem(v, 1), b) &&
		getconfigarg (gettupleitem (v, 2), c);
}
