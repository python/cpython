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

/* 
** Written by Jack Jansen, October 1994, initially only to allow him to
** test the ctb module:-)
*/

#include "allobjects.h"
#include "modsupport.h"		/* For getargs() etc. */
#include "structmember.h"

#include <console.h>

static object *ErrorObject;

#define OFF(x) offsetof(struct __copt, x)

static struct memberlist copt_memberlist[] = {
	{"top", T_SHORT, OFF(top)},
	{"left", T_SHORT, OFF(left)},
	{"title", T_PSTRING, OFF(title)},
	{"procID", T_SHORT, OFF(procID), RO},
	{"txFont", T_SHORT, OFF(txFont)},
	{"txSize", T_SHORT, OFF(txSize)},
	{"txFace", T_SHORT, OFF(txFace)},
	{"nrows", T_SHORT, OFF(nrows)},
	{"ncols", T_SHORT, OFF(ncols)},
	{"pause_atexit", T_SHORT, OFF(pause_atexit)},
	{NULL}
};

static unsigned char mytitle[256];
typedef struct {
	OB_HEAD
} coptobject;

staticforward typeobject Xxotype;

#define is_coptobject(v)		((v)->ob_type == &Xxotype)

static coptobject *
newcoptobject()
{
	coptobject *self;
	self = NEWOBJ(coptobject, &Xxotype);
	return self;
}

/* Xxo methods */

static void
copt_dealloc(self)
	coptobject *self;
{
	DEL(self);
}

static object *
copt_getattr(self, name)
	coptobject *self;
	char *name;
{
	return getmember((char *)&console_options, copt_memberlist, name);
}

static int
copt_setattr(self, name, v)
	coptobject *self;
	char *name;
	object *v;
{
	char *str;
	int len;
	
	if ( strcmp(name, "title") == 0 ) {
		if ( !v || !is_stringobject(v)) {
			err_setstr(ErrorObject, "title must be a string");
			return -1;
		}
		str = getstringvalue(v);
		len = strlen(str);
		mytitle[0] = (unsigned char)len;
		strncpy((char *)mytitle+1, str, mytitle[0]);
		console_options.title = mytitle;
		return 0;
	}
	return setmember((char *)&console_options, copt_memberlist, name, v);
}

static typeobject Xxotype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"console options",			/*tp_name*/
	sizeof(coptobject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)copt_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)copt_getattr, /*tp_getattr*/
	(setattrfunc)copt_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

/* ------------------------------------------- */

typedef struct {
	OB_HEAD
	FILE *fp;
	object *file;
} cons_object;

staticforward typeobject constype;

#define is_cons_object(v)		((v)->ob_type == &constype)

static cons_object *
newcons_object(fp, file)
	FILE *fp;
	object *file;
{
	cons_object *self;
	self = NEWOBJ(cons_object, &constype);
	if (self == NULL)
		return NULL;
	self->fp = fp;
	self->file = file;
	return self;
}

/* cons methods */

static void
cons_dealloc(self)
	cons_object *self;
{
	DECREF(self->file);
	DEL(self);
}

static object *
cons_setmode(self, args)
	cons_object *self;
	object *args;
{
	int mode;
	
	if (!getargs(args, "i", &mode))
		return NULL;
	csetmode(mode, self->fp);
	INCREF(None);
	return None;
}

static object *
cons_cleos(self, args)
	cons_object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	ccleos(self->fp);
	INCREF(None);
	return None;
}

static object *
cons_cleol(self, args)
	cons_object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	ccleol(self->fp);
	INCREF(None);
	return None;
}

static object *
cons_show(self, args)
	cons_object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	cshow(self->fp);
	INCREF(None);
	return None;
}

static object *
cons_hide(self, args)
	cons_object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	chide(self->fp);
	INCREF(None);
	return None;
}

static object *
cons_echo2printer(self, args)
	cons_object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	cecho2printer(self->fp);
	INCREF(None);
	return None;
}

static object *
cons_gotoxy(self, args)
	cons_object *self;
	object *args;
{
	int x, y;
	
	if (!getargs(args, "(ii)", &x, &y))
		return NULL;
	cgotoxy(x, y, self->fp);
	INCREF(None);
	return None;
}

static object *
cons_getxy(self, args)
	cons_object *self;
	object *args;
{
	int x, y;
	
	if (!getnoarg(args))
		return NULL;
	cgetxy(&x, &y, self->fp);
	return mkvalue("(ii)", x, y);
}

static object *
cons_settabs(self, args)
	cons_object *self;
	object *args;
{
	int arg;
	
	if (!getargs(args, "i", &arg))
		return NULL;
	csettabs(arg, self->fp);
	INCREF(None);
	return None;
}

static object *
cons_inverse(self, args)
	cons_object *self;
	object *args;
{
	int arg;
	
	if (!getargs(args, "i", &arg))
		return NULL;
	cinverse(arg, self->fp);
	INCREF(None);
	return None;
}

static struct methodlist cons_methods[] = {
	{"setmode",	(method)cons_setmode},
	{"gotoxy", 	(method)cons_gotoxy},
	{"getxy",	(method)cons_getxy},
	{"cleos",	(method)cons_cleos},
	{"cleol",	(method)cons_cleol},
	{"settabs",	(method)cons_settabs},
	{"inverse",	(method)cons_inverse},
	{"show",	(method)cons_show},
	{"hide",	(method)cons_hide},
	{"echo2printer",	(method)cons_echo2printer},
	{NULL,		NULL}		/* sentinel */
};

static object *
cons_getattr(self, name)
	cons_object *self;
	char *name;
{
	if ( strcmp(name, "file") == 0 ) {
		INCREF(self->file);
		return self->file;
	}
	return findmethod(cons_methods, (object *)self, name);
}

static typeobject constype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"cons",			/*tp_name*/
	sizeof(cons_object),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)cons_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)cons_getattr,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};
/* --------------------------------------------------------------------- */

/* Return a new console */

static object *
maccons_copen(self, args)
	object *self; /* Not used */
	object *args;
{
	FILE *fp;
	object *file;
	cons_object *rv;
	char *name;
	unsigned char nbuf[256];
	int len;
	
	name = NULL;
	if (!getnoarg(args))
		return NULL;
	if ( (fp=fopenc()) == NULL ) {
		err_errno(ErrorObject);
		return NULL;
	}
	if ( (file=newopenfileobject(fp, "<a console>", "r+", fclose)) == NULL)
		return NULL;
	rv = newcons_object(fp, file);
	if ( rv == NULL ) {
		fclose(fp);
	    return NULL;
	}
	return (object *)rv;
}

/* Return an open file as a console */

static object *
maccons_fopen(self, args)
	object *self; /* Not used */
	object *args;
{
	cons_object *rv;
	object *file;
	FILE *fp;
	
	if (!newgetargs(args, "O", &file))
		return NULL;
	if ( !is_fileobject(file) ) {
		err_badarg();
		return NULL;
	}
	fp = getfilefile(file);
	if ( !isatty(fileno(fp)) ) {
		err_setstr(ErrorObject, "File is not a console");
		return NULL;
	}
	rv = newcons_object(fp, file);
	if ( rv == NULL ) {
	    return NULL;
	}
	INCREF(file);
	return (object *)rv;
}

/* List of functions defined in the module */

static struct methodlist maccons_methods[] = {
	{"fopen",		(method)maccons_fopen, 1},
	{"copen",		(method)maccons_copen, 0},
	{NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initmacconsole) */

void
initmacconsole()
{
	object *m, *d, *o;

	/* Create the module and add the functions */
	m = initmodule("macconsole", maccons_methods);

	/* Add some symbolic constants to the module */
#define INTATTR(name, value) o = newintobject(value); dictinsert(d, name, o);
	d = getmoduledict(m);
	ErrorObject = newstringobject("macconsole.error");
	dictinsert(d, "error", ErrorObject);
	o = (object *)newcoptobject();
	dictinsert(d, "options", o);
	INTATTR("C_RAW", C_RAW);
	INTATTR("C_CBREAK", C_CBREAK);
	INTATTR("C_ECHO", C_ECHO);
	INTATTR("C_NOECHO", C_NOECHO);

	/* Check for errors */
	if (err_occurred())
		fatal("can't initialize module macconsole");
}
