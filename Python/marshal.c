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

/* Write Python objects to files and read them back.
   This is intended for writing and reading compiled Python code only;
   a true persistent storage facility would be much harder, since
   it would have to take circular links and sharing into account. */

#include "allobjects.h"
#include "longintrepr.h"
#include "compile.h"
#include "marshal.h"

#include <errno.h>
extern int errno;

#define TYPE_NULL	'0'
#define TYPE_NONE	'N'
#define TYPE_INT	'i'
#define TYPE_FLOAT	'f'
#define TYPE_LONG	'l'
#define TYPE_STRING	's'
#define TYPE_TUPLE	'('
#define TYPE_LIST	'['
#define TYPE_DICT	'{'
#define TYPE_CODE	'C'
#define TYPE_UNKNOWN	'?'

#define wr_byte(c, fp) putc((c), (fp))

void
wr_short(x, fp)
	int x;
	FILE *fp;
{
	wr_byte( x      & 0xff, fp);
	wr_byte((x>> 8) & 0xff, fp);
}

void
wr_long(x, fp)
	long x;
	FILE *fp;
{
	wr_byte((int)( x      & 0xff), fp);
	wr_byte((int)((x>> 8) & 0xff), fp);
	wr_byte((int)((x>>16) & 0xff), fp);
	wr_byte((int)((x>>24) & 0xff), fp);
}

void
wr_object(v, fp)
	object *v;
	FILE *fp;
{
	long i, n;
	
	if (v == NULL)
		wr_byte(TYPE_NULL, fp);
	else if (v == None)
		wr_byte(TYPE_NONE, fp);
	else if (is_intobject(v)) {
		wr_byte(TYPE_INT, fp);
		wr_long(getintvalue(v), fp);
	}
	else if (is_longobject(v)) {
		longobject *ob = (longobject *)v;
		wr_byte(TYPE_LONG, fp);
		n = ob->ob_size;
		wr_long((long)n, fp);
		if (n < 0)
			n = -n;
		for (i = 0; i < n; i++)
			wr_short(ob->ob_digit[i], fp);
	}
	else if (is_floatobject(v)) {
		extern void float_buf_repr PROTO((char *, floatobject *));
		char buf[256]; /* Plenty to format any double */
		float_buf_repr(buf, (floatobject *)v);
		n = strlen(buf);
		wr_byte(TYPE_FLOAT, fp);
		wr_byte((int)n, fp);
		fwrite(buf, 1, (int)n, fp);
	}
	else if (is_stringobject(v)) {
		wr_byte(TYPE_STRING, fp);
		n = getstringsize(v);
		wr_long(n, fp);
		fwrite(getstringvalue(v), 1, (int)n, fp);
	}
	else if (is_tupleobject(v)) {
		wr_byte(TYPE_TUPLE, fp);
		n = gettuplesize(v);
		wr_long(n, fp);
		for (i = 0; i < n; i++) {
			wr_object(gettupleitem(v, (int)i), fp);
		}
	}
	else if (is_listobject(v)) {
		wr_byte(TYPE_LIST, fp);
		n = getlistsize(v);
		wr_long(n, fp);
		for (i = 0; i < n; i++) {
			wr_object(getlistitem(v, (int)i), fp);
		}
	}
	else if (is_dictobject(v)) {
		wr_byte(TYPE_DICT, fp);
		/* This one is NULL object terminated! */
		n = getdictsize(v);
		for (i = 0; i < n; i++) {
			object *key, *val;
			extern object *getdict2key();
			key = getdict2key(v, (int)i);
			if (key != NULL) {
				val = dictlookup(v, getstringvalue(key));
				wr_object(key, fp);
				wr_object(val, fp);
			}
		}
		wr_object((object *)NULL, fp);
	}
	else if (is_codeobject(v)) {
		codeobject *co = (codeobject *)v;
		wr_byte(TYPE_CODE, fp);
		wr_object((object *)co->co_code, fp);
		wr_object(co->co_consts, fp);
		wr_object(co->co_names, fp);
		wr_object(co->co_filename, fp);
	}
	else {
		wr_byte(TYPE_UNKNOWN, fp);
	}
}

#define rd_byte(fp) getc(fp)

int
rd_short(fp)
	FILE *fp;
{
	register short x;
	x = rd_byte(fp);
	x |= rd_byte(fp) << 8;
	/* XXX If your short is > 16 bits, add sign-extension here!!! */
	return x;
}

long
rd_long(fp)
	FILE *fp;
{
	register long x;
	x = rd_byte(fp);
	x |= (long)rd_byte(fp) << 8;
	x |= (long)rd_byte(fp) << 16;
	x |= (long)rd_byte(fp) << 24;
	/* XXX If your long is > 32 bits, add sign-extension here!!! */
	return x;
}

object *
rd_object(fp)
	FILE *fp;
{
	object *v;
	long i, n;
	int type = rd_byte(fp);
	
	switch (type) {
	
	case EOF:
		err_setstr(RuntimeError, "EOF read where object expected");
		return NULL;
	
	case TYPE_NULL:
		return NULL;
	
	case TYPE_NONE:
		INCREF(None);
		return None;
	
	case TYPE_INT:
		return newintobject(rd_long(fp));
	
	case TYPE_LONG:
		{
			int size;
			longobject *ob;
			n = rd_long(fp);
			size = n<0 ? -n : n;
			ob = alloclongobject(size);
			if (ob == NULL)
				return NULL;
			ob->ob_size = n;
			for (i = 0; i < size; i++)
				ob->ob_digit[i] = rd_short(fp);
			return (object *)ob;
		}
	
	case TYPE_FLOAT:
		{
			extern double strtod();
			char buf[256];
			double res;
			char *end;
			n = rd_byte(fp);
			if (fread(buf, 1, (int)n, fp) != n) {
				err_setstr(RuntimeError,
					"EOF read where object expected");
				return NULL;
			}
			buf[n] = '\0';
			errno = 0;
			res = strtod(buf, &end);
			if (*end != '\0') {
				err_setstr(RuntimeError, "bad float syntax");
				return NULL;
			}
			if (errno != 0) {
				err_setstr(RuntimeError,
					"float constant too large");
				return NULL;
			}
			return newfloatobject(res);
		}
	
	case TYPE_STRING:
		n = rd_long(fp);
		v = newsizedstringobject((char *)NULL, n);
		if (v != NULL) {
			if (fread(getstringvalue(v), 1, (int)n, fp) != n) {
				DECREF(v);
				v = NULL;
				err_setstr(RuntimeError,
					"EOF read where object expected");
			}
		}
		return v;
	
	case TYPE_TUPLE:
		n = rd_long(fp);
		v = newtupleobject((int)n);
		if (v == NULL)
			return v;
		for (i = 0; i < n; i++)
			settupleitem(v, (int)i, rd_object(fp));
		return v;
	
	case TYPE_LIST:
		n = rd_long(fp);
		v = newlistobject((int)n);
		if (v == NULL)
			return v;
		for (i = 0; i < n; i++)
			setlistitem(v, (int)i, rd_object(fp));
		return v;
	
	case TYPE_DICT:
		v = newdictobject();
		if (v == NULL)
			return NULL;
		for (;;) {
			object *key, *val;
			key = rd_object(fp);
			if (key == NULL)
				break;
			val = rd_object(fp);
			dict2insert(v, key, val);
			DECREF(key);
			XDECREF(val);
		}
		return v;
	
	case TYPE_CODE:
		{
			object *code = rd_object(fp);
			object *consts = rd_object(fp);
			object *names = rd_object(fp);
			object *filename = rd_object(fp);
			if (!err_occurred()) {
				v = (object *) newcodeobject(code,
						consts, names, filename);
			}
			else
				v = NULL;
			XDECREF(code);
			XDECREF(consts);
			XDECREF(names);
			XDECREF(filename);

		}
		return v;
	
	default:
		err_setstr(RuntimeError, "read unknown object");
		return NULL;
	
	}
}

/* And an interface for Python programs... */

static object *
dump(self, args)
	object *self;
	object *args;
{
	object *f;
	if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 2) {
		err_badarg();
		return NULL;
	}
	f = gettupleitem(args, 1);
	if (f == NULL || !is_fileobject(f)) {
		err_badarg();
		return NULL;
	}
	wr_object(gettupleitem(args, 0), getfilefile(f));
	INCREF(None);
	return None;
}

static object *
load(self, f)
	object *self;
	object *f;
{
	object *v;
	if (f == NULL || !is_fileobject(f)) {
		err_badarg();
		return NULL;
	}
	v = rd_object(getfilefile(f));
	if (err_occurred()) {
		XDECREF(v);
		v = NULL;
	}
	return v;
}

static struct methodlist marshal_methods[] = {
	{"dump",	dump},
	{"load",	load},
	{NULL,		NULL}		/* sentinel */
};

void
initmarshal()
{
	(void) initmodule("marshal", marshal_methods);
}
