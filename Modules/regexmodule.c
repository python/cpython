/*
XXX support range parameter on search
XXX support mstop parameter on search
*/

/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
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

/* Regular expression objects */
/* This uses Tatu Ylonen's copyleft-free reimplementation of
   GNU regular expressions */

#include "allobjects.h"
#include "modsupport.h"

#include "regexpr.h"

static object *RegexError;	/* Exception */	

typedef struct {
	OB_HEAD
	struct re_pattern_buffer re_patbuf; /* The compiled expression */
	struct re_registers re_regs; /* The registers from the last match */
	char re_fastmap[256];	/* Storage for fastmap */
	object *re_translate;	/* String object for translate table */
	object *re_lastok;	/* String object last matched/searched */
} regexobject;

/* Regex object methods */

static void
reg_dealloc(re)
	regexobject *re;
{
	XDECREF(re->re_translate);
	XDECREF(re->re_lastok);
	XDEL(re->re_patbuf.buffer);
	XDEL(re->re_patbuf.translate);
	DEL(re);
}

static object *
makeresult(regs)
	struct re_registers *regs;
{
	object *v = newtupleobject(RE_NREGS);
	if (v != NULL) {
		int i;
		for (i = 0; i < RE_NREGS; i++) {
			object *w;
			w = mkvalue("(ii)", regs->start[i], regs->end[i]);
			if (w == NULL) {
				XDECREF(v);
				v = NULL;
				break;
			}
			settupleitem(v, i, w);
		}
	}
	return v;
}

static object *
reg_match(re, args)
	regexobject *re;
	object *args;
{
	object *argstring;
	char *buffer;
	int size;
	int offset;
	int result;
	if (getargs(args, "S", &argstring)) {
		offset = 0;
	}
	else {
		err_clear();
		if (!getargs(args, "(Si)", &argstring, &offset))
			return NULL;
	}
	buffer = getstringvalue(argstring);
	size = getstringsize(argstring);
	if (offset < 0 || offset > size) {
		err_setstr(RegexError, "match offset out of range");
		return NULL;
	}
	XDECREF(re->re_lastok);
	re->re_lastok = NULL;
	result = re_match(&re->re_patbuf, buffer, size, offset, &re->re_regs);
	if (result < -1) {
		/* Failure like stack overflow */
		err_setstr(RegexError, "match failure");
		return NULL;
	}
	if (result >= 0) {
		INCREF(argstring);
		re->re_lastok = argstring;
	}
	return newintobject((long)result); /* Length of the match or -1 */
}

static object *
reg_search(re, args)
	regexobject *re;
	object *args;
{
	object *argstring;
	char *buffer;
	int size;
	int offset;
	int range;
	int result;
	
	if (getargs(args, "S", &argstring)) {
		offset = 0;
	}
	else {
		err_clear();
		if (!getargs(args, "(Si)", &argstring, &offset))
			return NULL;
	}
	buffer = getstringvalue(argstring);
	size = getstringsize(argstring);
	if (offset < 0 || offset > size) {
		err_setstr(RegexError, "search offset out of range");
		return NULL;
	}
	/* NB: In Emacs 18.57, the documentation for re_search[_2] and
	   the implementation don't match: the documentation states that
	   |range| positions are tried, while the code tries |range|+1
	   positions.  It seems more productive to believe the code! */
	range = size - offset;
	XDECREF(re->re_lastok);
	re->re_lastok = NULL;
	result = re_search(&re->re_patbuf, buffer, size, offset, range,
			   &re->re_regs);
	if (result < -1) {
		/* Failure like stack overflow */
		err_setstr(RegexError, "match failure");
		return NULL;
	}
	if (result >= 0) {
		INCREF(argstring);
		re->re_lastok = argstring;
	}
	return newintobject((long)result); /* Position of the match or -1 */
}

static object *
reg_group(re, args)
	regexobject *re;
	object *args;
{
	int i, a, b;
	if (args != NULL && is_tupleobject(args)) {
		int n = gettuplesize(args);
		object *res = newtupleobject(n);
		if (res == NULL)
			return NULL;
		for (i = 0; i < n; i++) {
			object *v = reg_group(re, gettupleitem(args, i));
			if (v == NULL) {
				DECREF(res);
				return NULL;
			}
			settupleitem(res, i, v);
		}
		return res;
	}
	if (!getargs(args, "i", &i))
		return NULL;
	if (i < 0 || i >= RE_NREGS) {
		err_setstr(RegexError, "group() index out of range");
		return NULL;
	}
	if (re->re_lastok == NULL) {
		err_setstr(RegexError,
		    "group() only valid after successful match/search");
		return NULL;
	}
	a = re->re_regs.start[i];
	b = re->re_regs.end[i];
	if (a < 0 || b < 0) {
		INCREF(None);
		return None;
	}
	return newsizedstringobject(getstringvalue(re->re_lastok)+a, b-a);
}

static struct methodlist reg_methods[] = {
	{"match",	reg_match},
	{"search",	reg_search},
	{"group",	reg_group},
	{NULL,		NULL}		/* sentinel */
};

static object *
reg_getattr(re, name)
	regexobject *re;
	char *name;
{
	if (strcmp(name, "regs") == 0) {
		if (re->re_lastok == NULL) {
			INCREF(None);
			return None;
		}
		return makeresult(&re->re_regs);
	}
	if (strcmp(name, "last") == 0) {
		if (re->re_lastok == NULL) {
			INCREF(None);
			return None;
		}
		INCREF(re->re_lastok);
		return re->re_lastok;
	}
	if (strcmp(name, "translate") == 0) {
		if (re->re_translate == NULL) {
			INCREF(None);
			return None;
		}
		INCREF(re->re_translate);
		return re->re_translate;
	}
	if (strcmp(name, "__members__") == 0) {
		object *list = newlistobject(3);
		if (list) {
			setlistitem(list, 0, newstringobject("last"));
			setlistitem(list, 1, newstringobject("regs"));
			setlistitem(list, 2, newstringobject("translate"));
			if (err_occurred()) {
				DECREF(list);
				list = NULL;
			}
		}
		return list;
	}
	return findmethod(reg_methods, (object *)re, name);
}

static typeobject Regextype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"regex",		/*tp_name*/
	sizeof(regexobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	reg_dealloc,		/*tp_dealloc*/
	0,			/*tp_print*/
	reg_getattr,		/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

static object *
newregexobject(pat, size, translate)
	char *pat;
	int size;
	object *translate;
{
	regexobject *re;
	if (translate != NULL && getstringsize(translate) != 256) {
		err_setstr(RegexError,
			   "translation table must be 256 bytes");
		return NULL;
	}
	re = NEWOBJ(regexobject, &Regextype);
	if (re != NULL) {
		char *error;
		re->re_patbuf.buffer = NULL;
		re->re_patbuf.allocated = 0;
		re->re_patbuf.fastmap = re->re_fastmap;
		if (translate)
			re->re_patbuf.translate = getstringvalue(translate);
		else
			re->re_patbuf.translate = NULL;
		XINCREF(translate);
		re->re_translate = translate;
		re->re_lastok = NULL;
		error = re_compile_pattern(pat, size, &re->re_patbuf);
		if (error != NULL) {
			err_setstr(RegexError, error);
			DECREF(re);
			re = NULL;
		}
	}
	return (object *)re;
}

static object *
regex_compile(self, args)
	object *self;
	object *args;
{
	char *pat;
	int size;
	object *tran = NULL;
	if (!getargs(args, "s#", &pat, &size)) {
		err_clear();
		if (!getargs(args, "(s#S)", &pat, &size, &tran))
			return NULL;
	}
	return newregexobject(pat, size, tran);
}

static object *cache_pat;
static object *cache_prog;

static int
update_cache(pat)
	object *pat;
{
	if (pat != cache_pat) {
		XDECREF(cache_pat);
		cache_pat = NULL;
		XDECREF(cache_prog);
		cache_prog = regex_compile((object *)NULL, pat);
		if (cache_prog == NULL)
			return -1;
		cache_pat = pat;
		INCREF(cache_pat);
	}
	return 0;
}

static object *
regex_match(self, args)
	object *self;
	object *args;
{
	object *pat, *string;
	if (!getargs(args, "(SS)", &pat, &string))
		return NULL;
	if (update_cache(pat) < 0)
		return NULL;
	return reg_match((regexobject *)cache_prog, string);
}

static object *
regex_search(self, args)
	object *self;
	object *args;
{
	object *pat, *string;
	if (!getargs(args, "(SS)", &pat, &string))
		return NULL;
	if (update_cache(pat) < 0)
		return NULL;
	return reg_search((regexobject *)cache_prog, string);
}

static object *
regex_set_syntax(self, args)
	object *self, *args;
{
	int syntax;
	if (!getintarg(args, &syntax))
		return NULL;
	syntax = re_set_syntax(syntax);
	return newintobject((long)syntax);
}

static struct methodlist regex_global_methods[] = {
	{"compile",	regex_compile},
	{"match",	regex_match},
	{"search",	regex_search},
	{"set_syntax",	regex_set_syntax},
	{NULL,		NULL}		/* sentinel */
};

initregex()
{
	object *m, *d, *v;
	
	m = initmodule("regex", regex_global_methods);
	d = getmoduledict(m);
	
	/* Initialize regex.error exception */
	RegexError = newstringobject("regex.error");
	if (RegexError == NULL || dictinsert(d, "error", RegexError) != 0)
		fatal("can't define regex.error");

	/* Initialize regex.casefold constant */
	v = newsizedstringobject((char *)NULL, 256);
	if (v != NULL) {
		int i;
		char *s = getstringvalue(v);
		for (i = 0; i < 256; i++) {
			if (isupper(i))
				s[i] = tolower(i);
			else
				s[i] = i;
		}
		dictinsert(d, "casefold", v);
		DECREF(v);
	}
}
