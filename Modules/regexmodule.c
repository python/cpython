/*
XXX support range parameter on search
XXX support mstop parameter on search
*/

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

/* Regular expression objects */
/* This uses Tatu Ylonen's copyleft-free reimplementation of
   GNU regular expressions */

#include "allobjects.h"
#include "modsupport.h"

#include "regexpr.h"
#include "ctype.h"

static object *RegexError;	/* Exception */	

typedef struct {
	OB_HEAD
	struct re_pattern_buffer re_patbuf; /* The compiled expression */
	struct re_registers re_regs; /* The registers from the last match */
	char re_fastmap[256];	/* Storage for fastmap */
	object *re_translate;	/* String object for translate table */
	object *re_lastok;	/* String object last matched/searched */
	object *re_groupindex;	/* Group name to index dictionary */
	object *re_givenpat;	/* Pattern with symbolic groups */
	object *re_realpat;	/* Pattern without symbolic groups */
} regexobject;

/* Regex object methods */

static void
reg_dealloc(re)
	regexobject *re;
{
	XDECREF(re->re_translate);
	XDECREF(re->re_lastok);
	XDECREF(re->re_groupindex);
	XDECREF(re->re_givenpat);
	XDECREF(re->re_realpat);
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
	if (!getargs(args, "i", &i)) {
		object *n;
		err_clear();
		if (!getargs(args, "S", &n))
			return NULL;
		else {
			object *index;
			if (re->re_groupindex == NULL)
				index = NULL;
			else
				index = mappinglookup(re->re_groupindex, n);
			if (index == NULL) {
				err_setstr(RegexError, "group() group name doesn't exist");
				return NULL;
			}
			i = getintvalue(index);
		}
	}
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
	{"match",	(method)reg_match},
	{"search",	(method)reg_search},
	{"group",	(method)reg_group},
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
	if (strcmp(name, "groupindex") == 0) {
		if (re->re_groupindex == NULL) {
			INCREF(None);
			return None;
		}
		INCREF(re->re_groupindex);
		return re->re_groupindex;
	}
	if (strcmp(name, "realpat") == 0) {
		if (re->re_realpat == NULL) {
			INCREF(None);
			return None;
		}
		INCREF(re->re_realpat);
		return re->re_realpat;
	}
	if (strcmp(name, "givenpat") == 0) {
		if (re->re_givenpat == NULL) {
			INCREF(None);
			return None;
		}
		INCREF(re->re_givenpat);
		return re->re_givenpat;
	}
	if (strcmp(name, "__members__") == 0) {
		object *list = newlistobject(6);
		if (list) {
			setlistitem(list, 0, newstringobject("last"));
			setlistitem(list, 1, newstringobject("regs"));
			setlistitem(list, 2, newstringobject("translate"));
			setlistitem(list, 3, newstringobject("groupindex"));
			setlistitem(list, 4, newstringobject("realpat"));
			setlistitem(list, 5, newstringobject("givenpat"));
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
	(destructor)reg_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)reg_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

static object *
newregexobject(pattern, translate, givenpat, groupindex)
	object *pattern;
	object *translate;
	object *givenpat;
	object *groupindex;
{
	regexobject *re;
	char *pat = getstringvalue(pattern);
	int size = getstringsize(pattern);

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
		re->re_groupindex = groupindex;
		INCREF(pattern);
		re->re_realpat = pattern;
		INCREF(givenpat);
		re->re_givenpat = givenpat;
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
	object *pat = NULL;
	object *tran = NULL;
	if (!getargs(args, "S", &pat)) {
		err_clear();
		if (!getargs(args, "(SS)", &pat, &tran))
			return NULL;
	}
	return newregexobject(pat, tran, pat, NULL);
}

static object *
symcomp(pattern, gdict)
	object *pattern;
	object *gdict;
{
	char *opat = getstringvalue(pattern);
	char *oend = opat + getstringsize(pattern);
	int group_count = 0;
	int escaped = 0;
	char *o = opat;
	char *n;
	char name_buf[128];
	char *g;
	object *npattern;
	int require_escape = re_syntax & RE_NO_BK_PARENS ? 0 : 1;

	npattern = newsizedstringobject((char*)NULL, getstringsize(pattern));
	if (npattern == NULL)
		return NULL;
	n = getstringvalue(npattern);

	while (o < oend) {
		if (*o == '(' && escaped == require_escape) {
			char *backtrack;
			escaped = 0;
			++group_count;
			*n++ = *o;
			if (++o >= oend || *o != '<')
				continue;
			/* *o == '<' */
			if (o+1 < oend && *(o+1) == '>')
				continue;
			backtrack = o;
			g = name_buf;
			for (++o; o < oend;) {
				if (*o == '>') {
					object *group_name = NULL;
					object *group_index = NULL;
					*g++ = '\0';
					group_name = newstringobject(name_buf);
					group_index = newintobject(group_count);
					if (group_name == NULL || group_index == NULL
					    || mappinginsert(gdict, group_name, group_index) != 0) {
						XDECREF(group_name);
						XDECREF(group_index);
						XDECREF(npattern);
						return NULL;
					}
					++o; /* eat the '>' */
					break;
				}
				if (!isalnum(*o) && *o != '_') {
					o = backtrack;
					break;
				}
				*g++ = *o++;
			}
		}
		if (*o == '[' && !escaped) {
			*n++ = *o;
			++o;	/* eat the char following '[' */
			*n++ = *o;
			while (o < oend && *o != ']') {
				++o;
				*n++ = *o;
			}
			if (o < oend)
				++o;
		}
		else if (*o == '\\') {
			escaped = 1;
			*n++ = *o;
			++o;
		}
		else {
			escaped = 0;
			*n++ = *o;
			++o;
		}
	}

	if (resizestring(&npattern, n - getstringvalue(npattern)) == 0)
		return npattern;
	else {
		DECREF(npattern);
		return NULL;
	}

}

static object *
regex_symcomp(self, args)
	object *self;
	object *args;
{
	object *pattern;
	object *tran = NULL;
	object *gdict = NULL;
	object *npattern;
	if (!getargs(args, "S", &pattern)) {
		err_clear();
		if (!getargs(args, "(SS)", &pattern, &tran))
			return NULL;
	}
	gdict = newmappingobject();
	if (gdict == NULL
	    || (npattern = symcomp(pattern, gdict)) == NULL) {
		DECREF(gdict);
		DECREF(pattern);
		return NULL;
	}
	return newregexobject(npattern, tran, pattern, gdict);
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
	{"symcomp",	regex_symcomp},
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
