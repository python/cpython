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

/* Sad objects */

#include "allobjects.h"
#include "modsupport.h"
#include "structmember.h"

#include <sys/ioctl.h>
#include <sun/audioio.h>

/* #define offsetof(str,mem) ((int)(((str *)0)->mem)) */

typedef struct {
	OB_HEAD
	int	x_fd;		/* The open file */
	int	x_icount;	/* # samples read */
	int	x_ocount;	/* # samples written */
	int	x_isctl;	/* True if control device */
	
} sadobject;

typedef struct {
	OB_HEAD
	audio_info_t ai;
} sadstatusobject;

extern typeobject Sadtype;		/* Really static, forward */
extern typeobject Sadstatustype;	/* Really static, forward */
extern sadstatusobject *sads_alloc();	/* Forward */

static object *SunAudioError;

static int dummy_for_dl;

#define is_sadobject(v)		((v)->ob_type == &Sadtype)
#define is_sadstatusobject(v)	((v)->ob_type == &Sadstatustype)

static sadobject *
newsadobject(arg)
	object *arg;
{
	sadobject *xp;
	int fd;
	char *mode;
	int imode;

	/* Check arg for r/w/rw */
	if ( !getargs(arg, "s", &mode) )
	  return 0;
	if ( strcmp(mode, "r") == 0 )
	  imode = 0;
	else if ( strcmp(mode, "w") == 0 )
	  imode = 1;
	else if ( strcmp(mode, "rw") == 0 )
	  imode = 2;
	else if ( strcmp(mode, "control") == 0 )
	  imode = -1;
	else {
	    err_setstr(SunAudioError,
		       "Mode should be one of 'r', 'w', 'rw' or 'control'");
	    return 0;
	}
	
	/* Open the correct device */
	if ( imode < 0 )
	  fd = open("/dev/audioctl", 2); /* XXXX Chaeck that this works */
	else
	  fd = open("/dev/audio", imode);
	if ( fd < 0 ) {
	    err_errno(SunAudioError);
	    return NULL;
	}

	/* Create and initialize the object */
	xp = NEWOBJ(sadobject, &Sadtype);
	if (xp == NULL)
		return NULL;
	xp->x_fd = fd;
	xp->x_icount = xp->x_ocount = 0;
	xp->x_isctl = (imode < 0);
	
	return xp;
}

/* Sad methods */

static void
sad_dealloc(xp)
	sadobject *xp;
{
        close(xp->x_fd);
	DEL(xp);
}

static object *
sad_read(self, args)
        sadobject *self;
        object *args;
{
        int size, count;
	char *cp;
	object *rv;
	
        if ( !getargs(args, "i", &size) )
	  return 0;
	rv = newsizedstringobject(NULL, size);
	if ( rv == NULL )
	  return 0;

	cp = getstringvalue(rv);

	count = read(self->x_fd, cp, size);
	if ( count < 0 ) {
	    DECREF(rv);
	    err_errno(SunAudioError);
	    return NULL;
	}
	if ( count != size )
	  printf("sunaudio: funny read rv %d wtd %d\n", count, size);
	self->x_icount += count;
	return rv;
}

static object *
sad_write(self, args)
        sadobject *self;
        object *args;
{
        char *cp;
	int count, size;
	
        if ( !getargs(args, "s#", &cp, &size) )
	  return 0;

	count = write(self->x_fd, cp, size);
	if ( count < 0 ) {
	    err_errno(SunAudioError);
	    return NULL;
	}
	if ( count != size )
	  printf("sunaudio: funny write rv %d wanted %d\n", count, size);
	self->x_ocount += count;
	
	INCREF(None);
	return None;
}

static object *
sad_getinfo(self, args)
    sadobject *self;
    object *args;
{
	sadstatusobject *rv;

	if ( !getargs(args, "") )
	  return NULL;
	rv = sads_alloc();
	if ( ioctl(self->x_fd, AUDIO_GETINFO, &rv->ai) < 0 ) {
		err_errno(SunAudioError);
		DECREF(rv);
		return NULL;
	}
	return (object *)rv;
}

static object *
sad_setinfo(self, arg)
    sadobject *self;
    sadstatusobject *arg;
{
	if ( !is_sadstatusobject(arg) ) {
		err_setstr(TypeError, "Must be sun audio status object");
		return NULL;
	}
	if ( ioctl(self->x_fd, AUDIO_SETINFO, &arg->ai) < 0 ) {
		err_errno(SunAudioError);
		return NULL;
	}
	INCREF(None);
	return None;
}

static object *
sad_ibufcount(self, args)
    sadobject *self;
    object *args;
{
    audio_info_t ai;
    object *rv;
    
    if ( !getargs(args, "") )
      return 0;
    if ( ioctl(self->x_fd, AUDIO_GETINFO, &ai) < 0 ) {
	err_errno(SunAudioError);
	return NULL;
    }
    rv = newintobject(ai.record.samples - self->x_icount);
    return rv;
}

static object *
sad_obufcount(self, args)
    sadobject *self;
    object *args;
{
    audio_info_t ai;
    object *rv;
    
    if ( !getargs(args, "") )
      return 0;
    if ( ioctl(self->x_fd, AUDIO_GETINFO, &ai) < 0 ) {
	err_errno(SunAudioError);
	return NULL;
    }
    rv = newintobject(self->x_ocount - ai.play.samples);
    return rv;
}

static object *
sad_drain(self, args)
    sadobject *self;
    object *args;
{
    
    if ( !getargs(args, "") )
      return 0;
    if ( ioctl(self->x_fd, AUDIO_DRAIN, 0) < 0 ) {
	err_errno(SunAudioError);
	return NULL;
    }
    INCREF(None);
    return None;
}

static struct methodlist sad_methods[] = {
        { "read",	sad_read },
        { "write",	sad_write },
        { "ibufcount",	sad_ibufcount },
        { "obufcount",	sad_obufcount },
#define CTL_METHODS 4
        { "getinfo",	sad_getinfo },
        { "setinfo",	sad_setinfo },
        { "drain",	sad_drain },
	{NULL,		NULL}		/* sentinel */
};

static object *
sad_getattr(xp, name)
	sadobject *xp;
	char *name;
{
	if ( xp->x_isctl )
	  return findmethod(sad_methods+CTL_METHODS, (object *)xp, name);
	else
	  return findmethod(sad_methods, (object *)xp, name);
}

/* ----------------------------------------------------------------- */

static sadstatusobject *
sads_alloc() {
	sadstatusobject *rv;

	rv = NEWOBJ(sadstatusobject, &Sadstatustype);
	return rv;
}

static void
sads_dealloc(xp)
    sadstatusobject *xp;
{
	DEL(xp);
}

#define OFF(x) offsetof(audio_info_t,x)
static struct memberlist sads_ml[] = {
	{ "i_sample_rate",	T_UINT,		OFF(record.sample_rate) },
	{ "i_channels",		T_UINT,		OFF(record.channels) },
	{ "i_precision",	T_UINT,		OFF(record.precision) },
	{ "i_encoding",		T_UINT,		OFF(record.encoding) },
	{ "i_gain",		T_UINT,		OFF(record.gain) },
	{ "i_port",		T_UINT,		OFF(record.port) },
	{ "i_samples",		T_UINT,		OFF(record.samples) },
	{ "i_eof",		T_UINT,		OFF(record.eof) },
	{ "i_pause",		T_UBYTE,	OFF(record.pause) },
	{ "i_error",		T_UBYTE,	OFF(record.error) },
	{ "i_waiting",		T_UBYTE,	OFF(record.waiting) },
	{ "i_open",		T_UBYTE,	OFF(record.open) ,	 RO},
	{ "i_active",		T_UBYTE,	OFF(record.active) ,	 RO},

	{ "o_sample_rate",	T_UINT,		OFF(play.sample_rate) },
	{ "o_channels",		T_UINT,		OFF(play.channels) },
	{ "o_precision",	T_UINT,		OFF(play.precision) },
	{ "o_encoding",		T_UINT,		OFF(play.encoding) },
	{ "o_gain",		T_UINT,		OFF(play.gain) },
	{ "o_port",		T_UINT,		OFF(play.port) },
	{ "o_samples",		T_UINT,		OFF(play.samples) },
	{ "o_eof",		T_UINT,		OFF(play.eof) },
	{ "o_pause",		T_UBYTE,	OFF(play.pause) },
	{ "o_error",		T_UBYTE,	OFF(play.error) },
	{ "o_waiting",		T_UBYTE,	OFF(play.waiting) },
	{ "o_open",		T_UBYTE,	OFF(play.open) ,	 RO},
	{ "o_active",		T_UBYTE,	OFF(play.active) ,	 RO},

	{ "monitor_gain",	T_UINT,		OFF(monitor_gain) },
        { NULL,                 0,              0},
};

static object *
sads_getattr(xp, name)
    sadstatusobject *xp;
    char *name;
{
	return getmember((char *)&xp->ai, sads_ml, name);
}

static int
sads_setattr(xp, name, v)
    sadstatusobject *xp;
    char *name;
    object *v;
{

	if (v == NULL) {
		err_setstr(TypeError,
			   "can't delete sun audio status attributes");
		return NULL;
	}
	return setmember((char *)&xp->ai, sads_ml, name, v);
}

/* ------------------------------------------------------------------- */


static typeobject Sadtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"sun_audio_device",	/*tp_name*/
	sizeof(sadobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	sad_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	sad_getattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
};

static typeobject Sadstatustype = {
	OB_HEAD_INIT(&Typetype)
	0,				/*ob_size*/
	"sun_audio_device_status",	/*tp_name*/
	sizeof(sadstatusobject),	/*tp_size*/
	0,				/*tp_itemsize*/
	/* methods */
	sads_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	sads_getattr,	/*tp_getattr*/
	sads_setattr,	/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
};
/* ------------------------------------------------------------------- */

static object *
sadopen(self, args)
    object *self;
    object *args;
{
    object *rv;
    
    rv = (object *)newsadobject(args);
    return rv;
}
    
static struct methodlist sunaudiodev_methods[] = {
    { "open", sadopen },
    { 0, 0 },
};

void
initsunaudiodev() {
    object *m, *d;

    m = initmodule("sunaudiodev", sunaudiodev_methods);
    d = getmoduledict(m);
    SunAudioError = newstringobject("sunaudiodev.error");
    if ( SunAudioError == NULL || dictinsert(d, "error", SunAudioError) )
      fatal("can't define sunaudiodev.error");
}
