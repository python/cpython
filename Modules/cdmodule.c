/**********************************************************
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

/* CD module -- interface to Mark Callow's and Roger Chickering's */
 /* CD Audio Library (CD). */

#include <sys/types.h>
#include <cdaudio.h>
#include "allobjects.h"
#include "import.h"
#include "modsupport.h"
#include "ceval.h"

#define NCALLBACKS	8

typedef struct {
	OB_HEAD
	CDPLAYER *ob_cdplayer;
} cdplayerobject;

#define CheckPlayer(self)	if ((self)->ob_cdplayer == NULL) { \
					err_setstr(RuntimeError, "no player active"); \
					return NULL; \
				}
#define CheckParser(self)	if ((self)->ob_cdparser == NULL) { \
					err_setstr(RuntimeError, "no parser active"); \
					return NULL; \
				}

static object *CdError;		/* exception cd.error */

static object *
CD_allowremoval(self, args)
	cdplayerobject *self;
	object *args;
{
	CheckPlayer(self);

	if (!getnoarg(args))
		return NULL;

	CDallowremoval(self->ob_cdplayer);

	INCREF(None);
	return None;
}

static object *
CD_preventremoval(self, args)
	cdplayerobject *self;
	object *args;
{
	CheckPlayer(self);

	if (!getnoarg(args))
		return NULL;

	CDpreventremoval(self->ob_cdplayer);

	INCREF(None);
	return None;
}

static object *
CD_bestreadsize(self, args)
	cdplayerobject *self;
	object *args;
{
	CheckPlayer(self);

	if (!getnoarg(args))
		return NULL;

	return newintobject((long) CDbestreadsize(self->ob_cdplayer));
}

static object *
CD_close(self, args)
	cdplayerobject *self;
	object *args;
{
	CheckPlayer(self);

	if (!getnoarg(args))
		return NULL;

	if (!CDclose(self->ob_cdplayer)) {
		err_errno(CdError); /* XXX - ??? */
		return NULL;
	}
	self->ob_cdplayer = NULL;

	INCREF(None);
	return None;
}

static object *
CD_eject(self, args)
	cdplayerobject *self;
	object *args;
{
	CDSTATUS status;

	CheckPlayer(self);

	if (!getnoarg(args))
		return NULL;

	if (!CDeject(self->ob_cdplayer)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			err_setstr(CdError, "no disc in player");
		else
			err_setstr(CdError, "eject failed");
		return NULL;
	}

	INCREF(None);
	return None;
}
	
static object *
CD_getstatus(self, args)
	cdplayerobject *self;
	object *args;
{
	CDSTATUS status;

	CheckPlayer(self);

	if (!getnoarg(args))
		return NULL;

	if (!CDgetstatus(self->ob_cdplayer, &status)) {
		err_errno(CdError); /* XXX - ??? */
		return NULL;
	}

	return mkvalue("(ii(iii)(iii)(iii)iiii(iii))", status.state,
		       status.track, status.min, status.sec, status.frame,
		       status.abs_min, status.abs_sec, status.abs_frame,
		       status.total_min, status.total_sec, status.total_frame,
		       status.first, status.last, status.scsi_audio,
		       status.cur_block, status.polyfilla[0],
		       status.polyfilla[1], status.polyfilla[2]);
}
	
static object *
CD_gettrackinfo(self, args)
	cdplayerobject *self;
	object *args;
{
	int track;
	CDTRACKINFO info;
	CDSTATUS status;

	CheckPlayer(self);

	if (!getargs(args, "i", &track))
		return NULL;

	if (!CDgettrackinfo(self->ob_cdplayer, track, &info)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			err_setstr(CdError, "no disc in player");
		else
			err_setstr(CdError, "gettrackinfo failed");
		return NULL;
	}

	return mkvalue("((iii)(iii))",
		       info.start_min, info.start_sec, info.start_frame,
		       info.total_min, info.total_sec, info.total_frame);
}
	
static object *
CD_msftoblock(self, args)
	cdplayerobject *self;
	object *args;
{
	int min, sec, frame;
	unsigned long block;

	CheckPlayer(self);

	if (!getargs(args, "(iii)", &min, &sec, &frame))
		return NULL;

	block = CDmsftoblock(self->ob_cdplayer, min, sec, frame);
	return newintobject((long) block);
}
	
static object *
CD_play(self, args)
	cdplayerobject *self;
	object *args;
{
	int start, play;
	CDSTATUS status;

	CheckPlayer(self);

	if (!getargs(args, "(ii)", &start, &play))
		return NULL;

	if (!CDplay(self->ob_cdplayer, start, play)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			err_setstr(CdError, "no disc in player");
		else
			err_setstr(CdError, "play failed");
		return NULL;
	}

	INCREF(None);
	return None;
}
	
static object *
CD_playabs(self, args)
	cdplayerobject *self;
	object *args;
{
	int min, sec, frame, play;
	CDSTATUS status;

	CheckPlayer(self);

	if (!getargs(args, "(iiii)", &min, &sec, &frame, &play))
		return NULL;

	if (!CDplayabs(self->ob_cdplayer, min, sec, frame, play)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			err_setstr(CdError, "no disc in player");
		else
			err_setstr(CdError, "playabs failed");
		return NULL;
	}

	INCREF(None);
	return None;
}
	
static object *
CD_playtrack(self, args)
	cdplayerobject *self;
	object *args;
{
	int start, play;
	CDSTATUS status;

	CheckPlayer(self);

	if (!getargs(args, "(ii)", &start, &play))
		return NULL;

	if (!CDplaytrack(self->ob_cdplayer, start, play)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			err_setstr(CdError, "no disc in player");
		else
			err_setstr(CdError, "playtrack failed");
		return NULL;
	}

	INCREF(None);
	return None;
}
	
static object *
CD_playtrackabs(self, args)
	cdplayerobject *self;
	object *args;
{
	int track, min, sec, frame, play;
	CDSTATUS status;

	CheckPlayer(self);

	if (!getargs(args, "(iiiii)", &track, &min, &sec, &frame, &play))
		return NULL;

	if (!CDplaytrackabs(self->ob_cdplayer, track, min, sec, frame, play)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			err_setstr(CdError, "no disc in player");
		else
			err_setstr(CdError, "playtrackabs failed");
		return NULL;
	}

	INCREF(None);
	return None;
}
	
static object *
CD_readda(self, args)
	cdplayerobject *self;
	object *args;
{
	int numframes, n;
	object *result;

	CheckPlayer(self);

	if (!getargs(args, "i", &numframes))
		return NULL;

	result = newsizedstringobject(NULL, numframes * sizeof(CDFRAME));
	if (result == NULL)
		return NULL;

	n = CDreadda(self->ob_cdplayer, (CDFRAME *) getstringvalue(result), numframes);
	if (n == -1) {
		DECREF(result);
		err_errno(CdError);
		return NULL;
	}
	if (n < numframes)
		if (resizestring(&result, n * sizeof(CDFRAME)))
			return NULL;

	return result;
}

static object *
CD_seek(self, args)
	cdplayerobject *self;
	object *args;
{
	int min, sec, frame;
	long block;

	CheckPlayer(self);

	if (!getargs(args, "(iii)", &min, &sec, &frame))
		return NULL;

	block = CDseek(self->ob_cdplayer, min, sec, frame);
	if (block == -1) {
		err_errno(CdError);
		return NULL;
	}

	return newintobject(block);
}
	
static object *
CD_seektrack(self, args)
	cdplayerobject *self;
	object *args;
{
	int track;
	long block;

	CheckPlayer(self);

	if (!getargs(args, "i", &track))
		return NULL;

	block = CDseektrack(self->ob_cdplayer, track);
	if (block == -1) {
		err_errno(CdError);
		return NULL;
	}

	return newintobject(block);
}
	
static object *
CD_stop(self, args)
	cdplayerobject *self;
	object *args;
{
	CDSTATUS status;

	CheckPlayer(self);

	if (!getnoarg(args))
		return NULL;

	if (!CDstop(self->ob_cdplayer)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			err_setstr(CdError, "no disc in player");
		else
			err_setstr(CdError, "stop failed");
		return NULL;
	}

	INCREF(None);
	return None;
}
	
static object *
CD_togglepause(self, args)
	cdplayerobject *self;
	object *args;
{
	CDSTATUS status;

	CheckPlayer(self);

	if (!getnoarg(args))
		return NULL;

	if (!CDtogglepause(self->ob_cdplayer)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			err_setstr(CdError, "no disc in player");
		else
			err_setstr(CdError, "togglepause failed");
		return NULL;
	}

	INCREF(None);
	return None;
}
	
static struct methodlist cdplayer_methods[] = {
	{"allowremoval",	CD_allowremoval},
	{"bestreadsize",	CD_bestreadsize},
	{"close",		CD_close},
	{"eject",		CD_eject},
	{"getstatus",		CD_getstatus},
	{"gettrackinfo",	CD_gettrackinfo},
	{"msftoblock",		CD_msftoblock},
	{"play",		CD_play},
	{"playabs",		CD_playabs},
	{"playtrack",		CD_playtrack},
	{"playtrackabs",	CD_playtrackabs},
	{"preventremoval",	CD_preventremoval},
	{"readda",		CD_readda},
	{"seek",		CD_seek},
	{"seektrack",		CD_seektrack},
	{"stop",		CD_stop},
	{"togglepause",		CD_togglepause},
	{NULL,			NULL} 		/* sentinel */
};

static void
cdplayer_dealloc(self)
	cdplayerobject *self;
{
	if (self->ob_cdplayer != NULL)
		CDclose(self->ob_cdplayer);
	DEL(self);
}

static object *
cdplayer_getattr(cdp, name)
	cdplayerobject *cdp;
	char *name;
{
	return findmethod(cdplayer_methods, (object *)cdp, name);
}

typeobject CdPlayertype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"cdplayer",		/*tp_name*/
	sizeof(cdplayerobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	cdplayer_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	cdplayer_getattr,	/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

static object *
newcdplayerobject(cdp)
	CDPLAYER *cdp;
{
	cdplayerobject *p;

	p = NEWOBJ(cdplayerobject, &CdPlayertype);
	if (p == NULL)
		return NULL;
	p->ob_cdplayer = cdp;
	return (object *) p;
}

static object *
CD_open(self, args)
	object *self, *args;
{
	char *dev, *direction;
	CDPLAYER *cdp;

	/*
	 * Variable number of args.
	 * First defaults to "None", second defaults to "r".
	 */
	dev = NULL;
	direction = "r";
	if (!getnoarg(args)) {
		err_clear();
		if (!getargs(args, "z", &dev)) {
			err_clear();
			if (!getargs(args, "(zs)", &dev, &direction))
				return NULL;
		}
	}

	cdp = CDopen(dev, direction);
	if (cdp == NULL) {
		err_errno(CdError);
		return NULL;
	}

	return newcdplayerobject(cdp);
}

typedef struct {
	OB_HEAD
	CDPARSER *ob_cdparser;
	struct {
		object *ob_cdcallback;
		object *ob_cdcallbackarg;
	} ob_cdcallbacks[NCALLBACKS];
} cdparserobject;

static void
CD_callback(arg, type, data)
	void *arg;
	CDDATATYPES type;
	void *data;
{
	object *result, *args, *v;
	char *p;
	int i;
	cdparserobject *self;

	self = (cdparserobject *) arg;
	args = newtupleobject(3);
	if (args == NULL)
		return;
	INCREF(self->ob_cdcallbacks[type].ob_cdcallbackarg);
	settupleitem(args, 0, self->ob_cdcallbacks[type].ob_cdcallbackarg);
	settupleitem(args, 1, newintobject((long) type));
	switch (type) {
	case cd_audio:
		v = newsizedstringobject(data, CDDA_DATASIZE);
		break;
	case cd_pnum:
	case cd_index:
		v = newintobject(((CDPROGNUM *) data)->value);
		break;
	case cd_ptime:
	case cd_atime:
#define ptr ((struct cdtimecode *) data)
		v = mkvalue("(iii)", ptr->mhi * 10 + ptr->mlo,
			    ptr->shi * 10 + ptr->slo,
			    ptr->fhi * 10 + ptr->flo);
#undef ptr
		break;
	case cd_catalog:
		v = newsizedstringobject(NULL, 13);
		p = getstringvalue(v);
		for (i = 0; i < 13; i++)
			*p++ = ((char *) data)[i] + '0';
		break;
	case cd_ident:
		v = newsizedstringobject(NULL, 12);
		p = getstringvalue(v);
		CDsbtoa(p, ((struct cdident *) data)->country, 2);
		p += 2;
		CDsbtoa(p, ((struct cdident *) data)->owner, 3);
		p += 3;
		*p++ = ((struct cdident *) data)->year[0] + '0';
		*p++ = ((struct cdident *) data)->year[1] + '0';
		*p++ = ((struct cdident *) data)->serial[0] + '0';
		*p++ = ((struct cdident *) data)->serial[1] + '0';
		*p++ = ((struct cdident *) data)->serial[2] + '0';
		*p++ = ((struct cdident *) data)->serial[3] + '0';
		*p++ = ((struct cdident *) data)->serial[4] + '0';
		break;
	case cd_control:
		v = newintobject((long) *((unchar *) data));
		break;
	}
	settupleitem(args, 2, v);
	if (err_occurred()) {
		DECREF(args);
		return;
	}
	
	result = call_object(self->ob_cdcallbacks[type].ob_cdcallback, args);
	DECREF(args);
	XDECREF(result);
}

static object *
CD_deleteparser(self, args)
	cdparserobject *self;
	object *args;
{
	int i;

	CheckParser(self);

	if (!getnoarg(args))
		return NULL;

	CDdeleteparser(self->ob_cdparser);
	self->ob_cdparser = NULL;

	/* no sense in keeping the callbacks, so remove them */
	for (i = 0; i < NCALLBACKS; i++) {
		XDECREF(self->ob_cdcallbacks[i].ob_cdcallback);
		self->ob_cdcallbacks[i].ob_cdcallback = NULL;
		XDECREF(self->ob_cdcallbacks[i].ob_cdcallbackarg);
		self->ob_cdcallbacks[i].ob_cdcallbackarg = NULL;
	}

	INCREF(None);
	return None;
}

static object *
CD_parseframe(self, args)
	cdparserobject *self;
	object *args;
{
	char *cdfp;
	int length;
	CDFRAME *p;

	CheckParser(self);

	if (!getargs(args, "s#", &cdfp, &length))
		return NULL;

	if (length % sizeof(CDFRAME) != 0) {
		err_setstr(TypeError, "bad length");
		return NULL;
	}

	p = (CDFRAME *) cdfp;
	while (length > 0) {
		CDparseframe(self->ob_cdparser, p);
		length -= sizeof(CDFRAME);
		p++;
		if (err_occurred())
			return NULL;
	}

	INCREF(None);
	return None;
}

static object *
CD_removecallback(self, args)
	cdparserobject *self;
	object *args;
{
	int type;

	CheckParser(self);

	if (!getargs(args, "i", &type))
		return NULL;

	if (type < 0 || type >= NCALLBACKS) {
		err_setstr(TypeError, "bad type");
		return NULL;
	}

	CDremovecallback(self->ob_cdparser, (CDDATATYPES) type);

	XDECREF(self->ob_cdcallbacks[type].ob_cdcallback);
	self->ob_cdcallbacks[type].ob_cdcallback = NULL;

	XDECREF(self->ob_cdcallbacks[type].ob_cdcallbackarg);
	self->ob_cdcallbacks[type].ob_cdcallbackarg = NULL;

	INCREF(None);
	return None;
}

static object *
CD_resetparser(self, args)
	cdparserobject *self;
	object *args;
{
	CheckParser(self);

	if (!getnoarg(args))
		return NULL;

	CDresetparser(self->ob_cdparser);

	INCREF(None);
	return None;
}

static object *
CD_addcallback(self, args)
	cdparserobject *self;
	object *args;
{
	int type;
	object *funcobject, *funcargobject;

	CheckParser(self);

	/* XXX - more work here */
	if (!getargs(args, "(iOO)", &type, &funcobject, &funcargobject))
		return NULL;

	if (type < 0 || type >= NCALLBACKS) {
		err_setstr(TypeError, "argument out of range");
		return NULL;
	}

#ifdef IRIX_405
	CDaddcallback(self->ob_cdparser, (CDDATATYPES) type, CD_callback, (void *) self);
#else
	CDsetcallback(self->ob_cdparser, (CDDATATYPES) type, CD_callback, (void *) self);
#endif
	XDECREF(self->ob_cdcallbacks[type].ob_cdcallback);
	INCREF(funcobject);
	self->ob_cdcallbacks[type].ob_cdcallback = funcobject;
	XDECREF(self->ob_cdcallbacks[type].ob_cdcallbackarg);
	INCREF(funcargobject);
	self->ob_cdcallbacks[type].ob_cdcallbackarg = funcargobject;

	INCREF(None);
	return None;
}

static struct methodlist cdparser_methods[] = {
	{"addcallback",		CD_addcallback},
	{"deleteparser",	CD_deleteparser},
	{"parseframe",		CD_parseframe},
	{"removecallback",	CD_removecallback},
	{"resetparser",		CD_resetparser},
	{"setcallback",		CD_addcallback}, /* backward compatibility */
	{NULL,			NULL} 		/* sentinel */
};

static void
cdparser_dealloc(self)
	cdparserobject *self;
{
	int i;

	for (i = 0; i < NCALLBACKS; i++) {
		XDECREF(self->ob_cdcallbacks[i].ob_cdcallback);
		self->ob_cdcallbacks[i].ob_cdcallback = NULL;
		XDECREF(self->ob_cdcallbacks[i].ob_cdcallbackarg);
		self->ob_cdcallbacks[i].ob_cdcallbackarg = NULL;
	}
	CDdeleteparser(self->ob_cdparser);
	DEL(self);
}

static object *
cdparser_getattr(cdp, name)
	cdparserobject *cdp;
	char *name;
{
	return findmethod(cdparser_methods, (object *)cdp, name);
}

typeobject CdParsertype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"cdparser",		/*tp_name*/
	sizeof(cdparserobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	cdparser_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	cdparser_getattr,	/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

static object *
newcdparserobject(cdp)
	CDPARSER *cdp;
{
	cdparserobject *p;
	int i;

	p = NEWOBJ(cdparserobject, &CdParsertype);
	if (p == NULL)
		return NULL;
	p->ob_cdparser = cdp;
	for (i = 0; i < NCALLBACKS; i++) {
		p->ob_cdcallbacks[i].ob_cdcallback = NULL;
		p->ob_cdcallbacks[i].ob_cdcallbackarg = NULL;
	}
	return (object *) p;
}

static object *
CD_createparser(self, args)
	object *self, *args;
{
	CDPARSER *cdp;

	if (!getnoarg(args))
		return NULL;
	cdp = CDcreateparser();
	if (cdp == NULL) {
		err_setstr(CdError, "createparser failed");
		return NULL;
	}

	return newcdparserobject(cdp);
}

static object *
CD_sbtoa(self, args)
	object *self;
	object *args;
{
	char *sb;
	int length;
	object *result;

	if (!getargs(args, "s#", &sb, &length))
		return NULL;
	result = newsizedstringobject(NULL, length);
	CDsbtoa(getstringvalue(result), (unchar *) sb, length);
	return result;
}

static object *
CD_timetoa(self, args)
	object *self;
	object *args;
{
	char *tc;
	int length;
	object *result;

	if (!getargs(args, "s#", &tc, &length))
		return NULL;

	if (length != sizeof(struct cdtimecode)) {
		err_setstr(TypeError, "bad length");
		return NULL;
	}

	result = newsizedstringobject(NULL, 8);
	CDtimetoa(getstringvalue(result), (struct cdtimecode *) tc);
	return result;
}

static struct methodlist CD_methods[] = {
	{"sbtoa",	CD_sbtoa},
	{"open",	CD_open},
	{"createparser",CD_createparser},
	{"timetoa",	CD_timetoa},
	{NULL,		NULL}	/* Sentinel */
};

void
initcd()
{
	object *m, *d;

	m = initmodule("cd", CD_methods);
	d = getmoduledict(m);

	CdError = newstringobject("cd.error");
	if (CdError == NULL || dictinsert(d, "error", CdError) != 0)
		fatal("can't define cd.error");
}
