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

/* xx module */

#include "allobjects.h"
#include "modsupport.h"

#include <GestaltEqu.h>
#include "pascal.h"
#include "Speech.h"

/* Somehow the Apple Fix2X and X2Fix don't do what I expect */
#define fixed2double(x) (((double)(x))/32768.0)
#define double2fixed(x) ((Fixed)((x)*32768.0))

char *CurrentSpeech;
object *ms_error_object;

/* General error handler */
static object *
ms_error(num)
	OSErr num;
{
	char buf[40];
	
	sprintf(buf, "Mac Speech Mgr error #%d", num);
	err_setstr(ms_error_object, buf);
	return (object *)NULL;
}

/* -------------
**
** Part one - the speech channel object
*/
typedef struct {
	OB_HEAD
	object	*x_attr;	/* Attributes dictionary */
	SpeechChannel chan;
	object *curtext;	/* If non-NULL current text being spoken */
} scobject;

staticforward typeobject sctype;

#define is_scobject(v)		((v)->ob_type == &sctype)

static scobject *
newscobject(arg)
	VoiceSpec *arg;
{
	scobject *xp;
	OSErr err;
	
	xp = NEWOBJ(scobject, &sctype);
	if (xp == NULL)
		return NULL;
	xp->x_attr = NULL;
	if ( (err=NewSpeechChannel(arg, &xp->chan)) != 0) {
		DECREF(xp);
		return (scobject *)ms_error(err);
	}
	xp->curtext = NULL;
	return xp;
}

/* sc methods */

static void
sc_dealloc(xp)
	scobject *xp;
{
	DisposeSpeechChannel(xp->chan);
	XDECREF(xp->x_attr);
	DEL(xp);
}

static object *
sc_Stop(self, args)
	scobject *self;
	object *args;
{
	OSErr err;
	
	if (!getnoarg(args))
		return NULL;
	if ((err=StopSpeech(self->chan)) != 0)
		return ms_error(err);
	if ( self->curtext ) {
		DECREF(self->curtext);
		self->curtext = NULL;
	}
	INCREF(None);
	return None;
}

static object *
sc_SpeakText(self, args)
	scobject *self;
	object *args;
{
	OSErr err;
	char *str;
	int len;
	
	if (!getargs(args, "s#", &str, &len))
		return NULL;
	if ( self->curtext ) {
		StopSpeech(self->chan);
		DECREF(self->curtext);
		self->curtext = NULL;
	}
	if ((err=SpeakText(self->chan, (Ptr)str, (long)len)) != 0)
		return ms_error(err);
	(void)getargs(args, "O", &self->curtext);	/* Or should I check this? */
	INCREF(self->curtext);
	INCREF(None);
	return None;
}

static object *
sc_GetRate(self, args)
	scobject *self;
	object *args;
{
	OSErr err;
	Fixed farg;
	
	if (!getnoarg(args))
		return NULL;
	if ((err=GetSpeechRate(self->chan, &farg)) != 0)
		return ms_error(err);
	return newfloatobject(fixed2double(farg));
}

static object *
sc_GetPitch(self, args)
	scobject *self;
	object *args;
{
	OSErr err;
	Fixed farg;
	
	if (!getnoarg(args))
		return NULL;
	if ((err=GetSpeechPitch(self->chan, &farg)) != 0)
		return ms_error(err);
	return newfloatobject(fixed2double(farg));
}

static object *
sc_SetRate(self, args)
	scobject *self;
	object *args;
{
	OSErr err;
	double darg;
	
	if (!getargs(args, "d", &darg))
		return NULL;
	if ((err=SetSpeechRate(self->chan, double2fixed(darg))) != 0)
		return ms_error(err);
	INCREF(None);
	return None;
}

static object *
sc_SetPitch(self, args)
	scobject *self;
	object *args;
{
	OSErr err;
	double darg;
	
	if (!getargs(args, "d", &darg))
		return NULL;
	if ((err=SetSpeechPitch(self->chan, double2fixed(darg))) != 0)
		return ms_error(err);
	INCREF(None);
	return None;
}

static struct methodlist sc_methods[] = {
	{"Stop",		(method)sc_Stop},
	{"SetRate",		(method)sc_SetRate},
	{"GetRate",		(method)sc_GetRate},
	{"SetPitch",	(method)sc_SetPitch},
	{"GetPitch",	(method)sc_GetPitch},
	{"SpeakText",	(method)sc_SpeakText},
	{NULL,			NULL}		/* sentinel */
};

static object *
sc_getattr(xp, name)
	scobject *xp;
	char *name;
{
	if (xp->x_attr != NULL) {
		object *v = dictlookup(xp->x_attr, name);
		if (v != NULL) {
			INCREF(v);
			return v;
		}
	}
	return findmethod(sc_methods, (object *)xp, name);
}

static int
sc_setattr(xp, name, v)
	scobject *xp;
	char *name;
	object *v;
{
	if (xp->x_attr == NULL) {
		xp->x_attr = newdictobject();
		if (xp->x_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = dictremove(xp->x_attr, name);
		if (rv < 0)
			err_setstr(AttributeError,
			        "delete non-existing sc attribute");
		return rv;
	}
	else
		return dictinsert(xp->x_attr, name, v);
}

static typeobject sctype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"MacSpeechChannel",			/*tp_name*/
	sizeof(scobject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)sc_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)sc_getattr, /*tp_getattr*/
	(setattrfunc)sc_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

/* -------------
**
** Part two - the voice object
*/
typedef struct {
	OB_HEAD
	object	*x_attr;	/* Attributes dictionary */
	int		initialized;
	VoiceSpec	vs;
	VoiceDescription vd;
} mvobject;

staticforward typeobject mvtype;

#define is_mvobject(v)		((v)->ob_type == &mvtype)

static mvobject *
newmvobject()
{
	mvobject *xp;
	xp = NEWOBJ(mvobject, &mvtype);
	if (xp == NULL)
		return NULL;
	xp->x_attr = NULL;
	xp->initialized = 0;
	return xp;
}

static int
initmvobject(self, ind)
	mvobject *self;
	int ind;
{
	OSErr err;
	
	if ( (err=GetIndVoice((short)ind, &self->vs)) != 0 ) {
		ms_error(err);
		return 0;
	}
	if ( (err=GetVoiceDescription(&self->vs, &self->vd, sizeof self->vd)) != 0) {
		ms_error(err);
		return 0;
	}
	self->initialized = 1;
	return 1;
} 
/* mv methods */

static void
mv_dealloc(xp)
	mvobject *xp;
{
	XDECREF(xp->x_attr);
	DEL(xp);
}

static object *
mv_getgender(self, args)
	mvobject *self;
	object *args;
{
	object *rv;
	
	if (!getnoarg(args))
		return NULL;
	if (!self->initialized) {
		err_setstr(ms_error_object, "Uninitialized voice");
		return NULL;
	}
	rv = newintobject(self->vd.gender);
	return rv;
}

static object *
mv_newchannel(self, args)
	mvobject *self;
	object *args;
{	
	if (!getnoarg(args))
		return NULL;
	if (!self->initialized) {
		err_setstr(ms_error_object, "Uninitialized voice");
		return NULL;
	}
	return (object *)newscobject(&self->vs);
}

static struct methodlist mv_methods[] = {
	{"GetGender",	(method)mv_getgender},
	{"NewChannel",	(method)mv_newchannel},
	{NULL,		NULL}		/* sentinel */
};

static object *
mv_getattr(xp, name)
	mvobject *xp;
	char *name;
{
	if (xp->x_attr != NULL) {
		object *v = dictlookup(xp->x_attr, name);
		if (v != NULL) {
			INCREF(v);
			return v;
		}
	}
	return findmethod(mv_methods, (object *)xp, name);
}

static int
mv_setattr(xp, name, v)
	mvobject *xp;
	char *name;
	object *v;
{
	if (xp->x_attr == NULL) {
		xp->x_attr = newdictobject();
		if (xp->x_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = dictremove(xp->x_attr, name);
		if (rv < 0)
			err_setstr(AttributeError,
			        "delete non-existing MacVoice attribute");
		return rv;
	}
	else
		return dictinsert(xp->x_attr, name, v);
}

static typeobject mvtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"MacVoice",			/*tp_name*/
	sizeof(mvobject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)mv_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)mv_getattr, /*tp_getattr*/
	(setattrfunc)mv_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};


/* -------------
**
** Part three - The module interface
*/

/* See if Speech manager available */

static object *
ms_Available(self, args)
	object *self; /* Not used */
	object *args;
{
	OSErr err;
	long result;
	
	if (!getnoarg(args))
		return NULL;
	err = Gestalt(gestaltSpeechAttr, &result);
	if ( err == noErr && (result & (1<<gestaltSpeechMgrPresent)))
		result = 1;
	else
		result = 0;
	return newintobject(result);
}

/* Count number of busy speeches */

static object *
ms_Busy(self, args)
	object *self; /* Not used */
	object *args;
{
	OSErr err;
	short result;
	
	if (!getnoarg(args))
		return NULL;
	result = SpeechBusy();
	return newintobject(result);
}

/* Say something */

static object *
ms_SpeakString(self, args)
	object *self; /* Not used */
	object *args;
{
	OSErr err;
	short result;
	char *str;
	int len;
	
	if (!getstrarg(args, &str))
		return NULL;
	if (CurrentSpeech) {
		/* Free the old speech, after killing it off
		** (note that speach is async and c2pstr works inplace)
		*/
		SpeakString("\p");
		free(CurrentSpeech);
	}
	len = strlen(str);
	CurrentSpeech = malloc(len+1);
	strcpy(CurrentSpeech, str);
	err = SpeakString(c2pstr(CurrentSpeech));
	if ( err )
		return ms_error(err);
	INCREF(None);
	return None;
}


/* Count number of available voices */

static object *
ms_CountVoices(self, args)
	object *self; /* Not used */
	object *args;
{
	OSErr err;
	short result;
	
	if (!getnoarg(args))
		return NULL;
	CountVoices(&result);
	return newintobject(result);
}

static object *
ms_GetIndVoice(self, args)
	object *self; /* Not used */
	object *args;
{
	OSErr err;
	mvobject *rv;
	long ind;
	
	if( !getargs(args, "i", &ind))
		return 0;
	rv = newmvobject();
	if ( !initmvobject(rv, ind) ) {
		DECREF(rv);
		return NULL;
	}
	return (object *)rv;
}



/* List of functions defined in the module */

static struct methodlist ms_methods[] = {
	{"Available",	ms_Available},
	{"CountVoices",	ms_CountVoices},
	{"Busy",		ms_Busy},
	{"SpeakString",	ms_SpeakString},
	{"GetIndVoice", ms_GetIndVoice},
	{NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initmacspeech) */

void
initmacspeech()
{
	object *m, *d;

	/* Create the module and add the functions */
	m = initmodule("macspeech", ms_methods);

	/* Add some symbolic constants to the module */
	d = getmoduledict(m);
	ms_error_object = newstringobject("macspeech.error");
	dictinsert(d, "error", ms_error_object);

	/* Check for errors */
	if (err_occurred())
		fatal("can't initialize module macspeech");
}
