/*
XXX support translate table
XXX support range parameter on search
XXX support mstop parameter on search
*/

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

/* Regular expression objects */
/* This uses GNU regex.c, from subdirectory regex !!! */

#include "allobjects.h"
#include "modsupport.h"

#include "regex.h"

static object *RegexError;	/* Exception */	

typedef struct {
	OB_HEAD
	object *re_string;	/* The string (for printing) */
	struct re_pattern_buffer re_patbuf; /* The compiled expression */
	struct re_registers re_regs; /* The registers from the last match */
	int re_regs_valid;	/* Nonzero if the registers are valid */
	char re_fastmap[256];	/* Storage for fastmap */
} regexobject;

/* Regex object methods */

static void
reg_dealloc(re)
	regexobject *re;
{
	XDECREF(re->re_string);
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
			object *w, *u;
			if (	(w = newtupleobject(2)) == NULL ||
				(u = newintobject(regs->start[i])) == NULL ||
				settupleitem(w, 0, u) != 0 ||
				(u = newintobject(regs->end[i])) == NULL ||
				settupleitem(w, 1, u) != 0) {
				XDECREF(w);
				DECREF(v);
				return NULL;
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
	object *v;
	char *buffer;
	int offset;
	int result;
	if (args != NULL && is_stringobject(args)) {
		v = args;
		offset = 0;
	}
	else if (!getstrintarg(args, &v, &offset))
		return NULL;
	buffer = getstringvalue(v);
	re->re_regs_valid = 0;
	result = re_match(&re->re_patbuf, buffer, getstringsize(v),
			  offset, &re->re_regs);
	if (result < -1) {
		/* Failure like stack overflow */
		err_setstr(RegexError, "match failure");
		return NULL;
	}
	re->re_regs_valid = result >= 0;
	return newintobject((long)result); /* Length of the match or -1 */
}
static object *
reg_search(re, args)
	regexobject *re;
	object *args;
{
	object *v;
	char *buffer;
	int size;
	int offset;
	int range;
	int result;
	if (args != NULL && is_stringobject(args)) {
		v = args;
		offset = 0;
	}
	else if (!getstrintarg(args, &v, &offset))
		return NULL;
	buffer = getstringvalue(v);
	size = getstringsize(v);
	if (offset < 0 || offset > size) {
		err_setstr(RegexError, "search offset out of range");
		return NULL;
	}
	range = size - offset + 1;
	re->re_regs_valid = 0;
	result = re_search(&re->re_patbuf, buffer, size, offset, range,
			   &re->re_regs);
	if (result < -1) {
		/* Failure like stack overflow */
		err_setstr(RegexError, "match failure");
		return NULL;
	}
	re->re_regs_valid = result >= 0;
	return newintobject((long)result); /* Position of the match or -1 */
}

static struct methodlist reg_methods[] = {
	{"match",	reg_match},
	{"search",	reg_search},
	{NULL,		NULL}		/* sentinel */
};

static object *
reg_getattr(re, name)
	regexobject *re;
	char *name;
{
	if (strcmp(name, "regs") == 0) {
		if (!re->re_regs_valid) {
			err_setstr(RegexError,
			  "regs only valid after successful match/search");
			return NULL;
		}
		return makeresult(&re->re_regs);
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

static regexobject *
newregexobject(string)
	object *string;
{
	regexobject *re;
	re = NEWOBJ(regexobject, &Regextype);
	if (re != NULL) {
		char *error;
		INCREF(string);
		re->re_string = string;
		re->re_patbuf.buffer = NULL;
		re->re_patbuf.allocated = 0;
		re->re_patbuf.fastmap = re->re_fastmap;
		re->re_patbuf.translate = NULL;
		re->re_regs_valid = 0;
		error = re_compile_pattern(getstringvalue(string),
					   getstringsize(string),
					   &re->re_patbuf);
		if (error != NULL) {
			err_setstr(RegexError, error);
			DECREF(re);
			re = NULL;
		}
	}
	return re;
}

static object *
regex_compile(self, args)
	object *self;
	object *args;
{
	object *string;
	if (!getstrarg(args, &string))
		return NULL;
	return (object *)newregexobject(string);
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
	if (!getstrstrarg(args, &pat, &string))
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
	if (!getstrstrarg(args, &pat, &string))
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
	object *m, *d;
	
	m = initmodule("regex", regex_global_methods);
	d = getmoduledict(m);
	
	/* Initialize regex.error exception */
	RegexError = newstringobject("regex.error");
	if (RegexError == NULL || dictinsert(d, "error", RegexError) != 0)
		fatal("can't define regex.error");
}
