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

/* Font Manager module */

#include "allobjects.h"

#include "modsupport.h"

#include <gl.h>
#include <device.h>
#include <fmclient.h>


/* Font Handle object implementation */

typedef struct {
	OB_HEAD
	fmfonthandle fh_fh;
} fhobject;

extern typeobject Fhtype;	/* Really static, forward */

#define is_fhobject(v)		((v)->ob_type == &Fhtype)

static object *
newfhobject(fh)
	fmfonthandle fh;
{
	fhobject *fhp;
	if (fh == NULL) {
		err_setstr(RuntimeError, "error creating new font handle");
		return NULL;
	}
	fhp = NEWOBJ(fhobject, &Fhtype);
	if (fhp == NULL)
		return NULL;
	fhp->fh_fh = fh;
	return (object *)fhp;
}

/* Font Handle methods */

static object *
fh_scalefont(self, args)
	fhobject *self;
	object *args;
{
	double size;
	if (!getdoublearg(args, &size))
		return NULL;
	return newfhobject(fmscalefont(self->fh_fh, size));
}

/* XXX fmmakefont */

static object *
fh_setfont(self, args)
	fhobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	fmsetfont(self->fh_fh);
	INCREF(None);
	return None;
}

static object *
fh_getfontname(self, args)
	fhobject *self;
	object *args;
{
	char fontname[256];
	int len;
	if (!getnoarg(args))
		return NULL;
	len = fmgetfontname(self->fh_fh, sizeof fontname, fontname);
	if (len < 0) {
		err_setstr(RuntimeError, "error in fmgetfontname");
		return NULL;
	}
	return newsizedstringobject(fontname, len);
}

static object *
fh_getcomment(self, args)
	fhobject *self;
	object *args;
{
	char comment[256];
	int len;
	if (!getnoarg(args))
		return NULL;
	len = fmgetcomment(self->fh_fh, sizeof comment, comment);
	if (len < 0) {
		err_setstr(RuntimeError, "error in fmgetcomment");
		return NULL;
	}
	return newsizedstringobject(comment, len);
}

static object *
fh_getfontinfo(self, args)
	fhobject *self;
	object *args;
{
	fmfontinfo info;
	object *v;
	if (!getnoarg(args))
		return NULL;
	if (fmgetfontinfo(self->fh_fh, &info) < 0) {
		err_setstr(RuntimeError, "error in fmgetfontinfo");
		return NULL;
	}
	v = newtupleobject(8);
	if (v == NULL)
		return NULL;
#define SET(i, member) settupleitem(v, i, newintobject(info.member))
	SET(0, printermatched);
	SET(1, fixed_width);
	SET(2, xorig);
	SET(3, yorig);
	SET(4, xsize);
	SET(5, ysize);
	SET(6, height);
	SET(7, nglyphs);
#undef SET
	if (err_occurred())
		return NULL;
	return v;
}

#if 0
static object *
fh_getwholemetrics(self, args)
	fhobject *self;
	object *args;
{
}
#endif

static object *
fh_getstrwidth(self, args)
	fhobject *self;
	object *args;
{
	char *str;
	if (!getstrarg(args, &str))
		return NULL;
	return newintobject(fmgetstrwidth(self->fh_fh, str));
}

static struct methodlist fh_methods[] = {
	{"scalefont",	fh_scalefont},
	{"setfont",	fh_setfont},
	{"getfontname",	fh_getfontname},
	{"getcomment",	fh_getcomment},
	{"getfontinfo",	fh_getfontinfo},
#if 0
	{"getwholemetrics",	fh_getwholemetrics},
#endif
	{"getstrwidth",	fh_getstrwidth},
	{NULL,		NULL}		/* sentinel */
};

static object *
fh_getattr(fhp, name)
	fhobject *fhp;
	char *name;
{
	return findmethod(fh_methods, (object *)fhp, name);
}

static void
fh_dealloc(fhp)
	fhobject *fhp;
{
	fmfreefont(fhp->fh_fh);
	DEL(fhp);
}

static typeobject Fhtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"font handle",		/*tp_name*/
	sizeof(fhobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	fh_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	fh_getattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
};


/* Font Manager functions */

static object *
fm_init(self, args)
	object *self, *args;
{
	if (!getnoarg(args))
		return NULL;
	fminit();
	INCREF(None);
	return None;
}

static object *
fm_findfont(self, args)
	object *self, *args;
{
	char *str;
	if (!getstrarg(args, &str))
		return NULL;
	return newfhobject(fmfindfont(str));
}

static object *
fm_prstr(self, args)
	object *self, *args;
{
	char *str;
	if (!getstrarg(args, &str))
		return NULL;
	fmprstr(str);
	INCREF(None);
	return None;
}

/* XXX This uses a global variable as temporary! Not re-entrant! */

static object *fontlist;

static void
clientproc(fontname)
	char *fontname;
{
	int err;
	object *v;
	if (fontlist == NULL)
		return;
	v = newstringobject(fontname);
	if (v == NULL)
		err = -1;
	else {
		err = addlistitem(fontlist, v);
		DECREF(v);
	}
	if (err != 0) {
		DECREF(fontlist);
		fontlist = NULL;
	}
}

static object *
fm_enumerate(self, args)
	object *self, *args;
{
	object *res;
	if (!getnoarg(args))
		return NULL;
	fontlist = newlistobject(0);
	if (fontlist == NULL)
		return NULL;
	fmenumerate(clientproc);
	res = fontlist;
	fontlist = NULL;
	return res;
}

static object *
fm_setpath(self, args)
	object *self, *args;
{
	char *str;
	if (!getstrarg(args, &str))
		return NULL;
	fmsetpath(str);
	INCREF(None);
	return None;
}

static object *
fm_fontpath(self, args)
	object *self, *args;
{
	if (!getnoarg(args))
		return NULL;
	return newstringobject(fmfontpath());
}

static struct methodlist fm_methods[] = {
	{"init",	fm_init},
	{"findfont",	fm_findfont},
	{"enumerate",	fm_enumerate},
	{"prstr",	fm_prstr},
	{"setpath",	fm_setpath},
	{"fontpath",	fm_fontpath},
	{NULL,		NULL}		/* sentinel */
};


void
initfm()
{
	initmodule("fm", fm_methods);
	fminit();
}
