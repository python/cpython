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

/* Compile an expression node to intermediate code */

/* XXX TO DO:
   XXX Compute maximum needed stack sizes while compiling
   XXX Generate simple jump for break/return outside 'try...finally'
*/

#include "allobjects.h"

#include "node.h"
#include "token.h"
#include "graminit.h"
#include "compile.h"
#include "opcode.h"
#include "structmember.h"

#include <ctype.h>
#include <errno.h>

#define OFF(x) offsetof(codeobject, x)

static struct memberlist code_memberlist[] = {
	{"co_code",	T_OBJECT,	OFF(co_code),		READONLY},
	{"co_consts",	T_OBJECT,	OFF(co_consts),		READONLY},
	{"co_names",	T_OBJECT,	OFF(co_names),		READONLY},
	{"co_filename",	T_OBJECT,	OFF(co_filename),	READONLY},
	{"co_name",	T_OBJECT,	OFF(co_name),		READONLY},
	{NULL}	/* Sentinel */
};

static object *
code_getattr(co, name)
	codeobject *co;
	char *name;
{
	return getmember((char *)co, code_memberlist, name);
}

static void
code_dealloc(co)
	codeobject *co;
{
	XDECREF(co->co_code);
	XDECREF(co->co_consts);
	XDECREF(co->co_names);
	XDECREF(co->co_filename);
	XDECREF(co->co_name);
	DEL(co);
}

static object *
code_repr(co)
	codeobject *co;
{
	char buf[500];
	int lineno = -1;
	char *p = GETSTRINGVALUE(co->co_code);
	char *filename = "???";
	char *name = "???";
	if (*p == SET_LINENO)
		lineno = (p[1] & 0xff) | ((p[2] & 0xff) << 8);
	if (co->co_filename && is_stringobject(co->co_filename))
		filename = getstringvalue(co->co_filename);
	if (co->co_name && is_stringobject(co->co_name))
		name = getstringvalue(co->co_name);
	sprintf(buf, "<code object %.100s at %lx, file \"%.300s\", line %d>",
		name, (long)co, filename, lineno);
	return newstringobject(buf);
}

static int
code_compare(co, cp)
	codeobject *co, *cp;
{
	int cmp;
	cmp = cmpobject((object *)co->co_code, (object *)cp->co_code);
	if (cmp) return cmp;
	cmp = cmpobject(co->co_consts, cp->co_consts);
	if (cmp) return cmp;
	cmp = cmpobject(co->co_names, cp->co_names);
	return cmp;
}

static long
code_hash(co)
	codeobject *co;
{
	long h, h1, h2, h3;
	h1 = hashobject((object *)co->co_code);
	if (h1 == -1) return -1;
	h2 = hashobject(co->co_consts);
	if (h2 == -1) return -1;
	h3 = hashobject(co->co_names);
	if (h3 == -1) return -1;
	h = h1 ^ h2 ^ h3;
	if (h == -1) h = -2;
	return h;
}

typeobject Codetype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"code",
	sizeof(codeobject),
	0,
	(destructor)code_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	(getattrfunc)code_getattr, /*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)code_compare, /*tp_compare*/
	(reprfunc)code_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)code_hash, /*tp_hash*/
};

codeobject *
newcodeobject(code, consts, names, filename, name)
	object *code;
	object *consts;
	object *names;
	object *filename;
	object *name;
{
	codeobject *co;
	int i;
	/* Check argument types */
	if (code == NULL || !is_stringobject(code) ||
		consts == NULL ||
		names == NULL ||
		name == NULL || !(is_stringobject(name) || name == None)) {
		err_badcall();
		return NULL;
	}
	/* Allow two lists instead of two tuples */
	if (is_listobject(consts) && is_listobject(names)) {
		consts = listtuple(consts);
		if (consts == NULL)
			return NULL;
		names = listtuple(names);
		if (names == NULL) {
			DECREF(consts);
			return NULL;
		}
	}
	else if (!is_tupleobject(consts) && !is_tupleobject(names)) {
		err_badcall();
		return NULL;
	}
	else {
		INCREF(consts);
		INCREF(names);
	}
	/* Make sure the list of names contains only strings */
	for (i = gettuplesize(names); --i >= 0; ) {
		object *v = gettupleitem(names, i);
		if (v == NULL || !is_stringobject(v)) {
			DECREF(consts);
			DECREF(names);
			err_badcall();
			return NULL;
		}
	}
	co = NEWOBJ(codeobject, &Codetype);
	if (co != NULL) {
		INCREF(code);
		co->co_code = (stringobject *)code;
		co->co_consts = consts;
		co->co_names = names;
		INCREF(filename);
		co->co_filename = filename;
		INCREF(name);
		co->co_name = name;
	}
	else {
		DECREF(consts);
		DECREF(names);
	}
	return co;
}


/* Data structure used internally */

#define MAXBLOCKS 20 /* Max static block nesting within a function */

struct compiling {
	object *c_code;		/* string */
	object *c_consts;	/* list of objects */
	object *c_names;	/* list of strings (names) */
	object *c_globals;	/* dictionary */
	int c_nexti;		/* index into c_code */
	int c_errors;		/* counts errors occurred */
	int c_infunction;	/* set when compiling a function */
	int c_interactive;	/* generating code for interactive command */
	int c_loops;		/* counts nested loops */
	int c_begin;		/* begin of current loop, for 'continue' */
	int c_block[MAXBLOCKS];	/* stack of block types */
	int c_nblocks;		/* current block stack level */
	char *c_filename;	/* filename of current node */
	char *c_name;		/* name of object (e.g. function) */
};


/* Interface to the block stack */

static void
block_push(c, type)
	struct compiling *c;
	int type;
{
	if (c->c_nblocks >= MAXBLOCKS) {
		err_setstr(SystemError, "too many statically nested blocks");
		c->c_errors++;
	}
	else {
		c->c_block[c->c_nblocks++] = type;
	}
}

static void
block_pop(c, type)
	struct compiling *c;
	int type;
{
	if (c->c_nblocks > 0)
		c->c_nblocks--;
	if (c->c_block[c->c_nblocks] != type && c->c_errors == 0) {
		err_setstr(SystemError, "bad block pop");
		c->c_errors++;
	}
}


/* Prototypes */

static int com_init PROTO((struct compiling *, char *));
static void com_free PROTO((struct compiling *));
static void com_done PROTO((struct compiling *));
static void com_node PROTO((struct compiling *, struct _node *));
static void com_addbyte PROTO((struct compiling *, int));
static void com_addint PROTO((struct compiling *, int));
static void com_addoparg PROTO((struct compiling *, int, int));
static void com_addfwref PROTO((struct compiling *, int, int *));
static void com_backpatch PROTO((struct compiling *, int));
static int com_add PROTO((struct compiling *, object *, object *));
static int com_addconst PROTO((struct compiling *, object *));
static int com_addname PROTO((struct compiling *, object *));
static void com_addopname PROTO((struct compiling *, int, node *));
static void com_list PROTO((struct compiling *, node *, int));
static int com_argdefs PROTO((struct compiling *, node *, int *));

static int
com_init(c, filename)
	struct compiling *c;
	char *filename;
{
	if ((c->c_code = newsizedstringobject((char *)NULL, 1000)) == NULL)
		goto fail_3;
	if ((c->c_consts = newlistobject(0)) == NULL)
		goto fail_2;
	if ((c->c_names = newlistobject(0)) == NULL)
		goto fail_1;
	if ((c->c_globals = newdictobject()) == NULL)
		goto fail_0;
	c->c_nexti = 0;
	c->c_errors = 0;
	c->c_infunction = 0;
	c->c_interactive = 0;
	c->c_loops = 0;
	c->c_begin = 0;
	c->c_nblocks = 0;
	c->c_filename = filename;
	c->c_name = "?";
	return 1;
	
  fail_0:
  	DECREF(c->c_names);
  fail_1:
	DECREF(c->c_consts);
  fail_2:
	DECREF(c->c_code);
  fail_3:
 	return 0;
}

static void
com_free(c)
	struct compiling *c;
{
	XDECREF(c->c_code);
	XDECREF(c->c_consts);
	XDECREF(c->c_names);
	XDECREF(c->c_globals);
}

static void
com_done(c)
	struct compiling *c;
{
	if (c->c_code != NULL)
		resizestring(&c->c_code, c->c_nexti);
}

static void
com_addbyte(c, byte)
	struct compiling *c;
	int byte;
{
	int len;
	if (byte < 0 || byte > 255) {
		/*
		fprintf(stderr, "XXX compiling bad byte: %d\n", byte);
		fatal("com_addbyte: byte out of range");
		*/
		err_setstr(SystemError, "com_addbyte: byte out of range");
		c->c_errors++;
	}
	if (c->c_code == NULL)
		return;
	len = getstringsize(c->c_code);
	if (c->c_nexti >= len) {
		if (resizestring(&c->c_code, len+1000) != 0) {
			c->c_errors++;
			return;
		}
	}
	getstringvalue(c->c_code)[c->c_nexti++] = byte;
}

static void
com_addint(c, x)
	struct compiling *c;
	int x;
{
	com_addbyte(c, x & 0xff);
	com_addbyte(c, x >> 8); /* XXX x should be positive */
}

static void
com_addoparg(c, op, arg)
	struct compiling *c;
	int op;
	int arg;
{
	com_addbyte(c, op);
	com_addint(c, arg);
}

static void
com_addfwref(c, op, p_anchor)
	struct compiling *c;
	int op;
	int *p_anchor;
{
	/* Compile a forward reference for backpatching */
	int here;
	int anchor;
	com_addbyte(c, op);
	here = c->c_nexti;
	anchor = *p_anchor;
	*p_anchor = here;
	com_addint(c, anchor == 0 ? 0 : here - anchor);
}

static void
com_backpatch(c, anchor)
	struct compiling *c;
	int anchor; /* Must be nonzero */
{
	unsigned char *code = (unsigned char *) getstringvalue(c->c_code);
	int target = c->c_nexti;
	int dist;
	int prev;
	for (;;) {
		/* Make the JUMP instruction at anchor point to target */
		prev = code[anchor] + (code[anchor+1] << 8);
		dist = target - (anchor+2);
		code[anchor] = dist & 0xff;
		code[anchor+1] = dist >> 8;
		if (!prev)
			break;
		anchor -= prev;
	}
}

/* Handle literals and names uniformly */

static int
com_add(c, list, v)
	struct compiling *c;
	object *list;
	object *v;
{
	int n = getlistsize(list);
	int i;
	for (i = n; --i >= 0; ) {
		object *w = getlistitem(list, i);
		if (v->ob_type == w->ob_type && cmpobject(v, w) == 0)
			return i;
	}
	if (addlistitem(list, v) != 0)
		c->c_errors++;
	return n;
}

static int
com_addconst(c, v)
	struct compiling *c;
	object *v;
{
	return com_add(c, c->c_consts, v);
}

static int
com_addname(c, v)
	struct compiling *c;
	object *v;
{
	return com_add(c, c->c_names, v);
}

static void
com_addopnamestr(c, op, name)
	struct compiling *c;
	int op;
	char *name;
{
	object *v;
	int i;
	if (name == NULL || (v = newstringobject(name)) == NULL) {
		c->c_errors++;
		i = 255;
	}
	else {
		i = com_addname(c, v);
		DECREF(v);
	}
	/* Hack to replace *_NAME opcodes by *_GLOBAL if necessary */
	switch (op) {
	case LOAD_NAME:
	case STORE_NAME:
	case DELETE_NAME:
		if (dictlookup(c->c_globals, name) != NULL) {
			switch (op) {
			case LOAD_NAME:   op = LOAD_GLOBAL;   break;
			case STORE_NAME:  op = STORE_GLOBAL;  break;
			case DELETE_NAME: op = DELETE_GLOBAL; break;
			}
		}
	}
	com_addoparg(c, op, i);
}

static void
com_addopname(c, op, n)
	struct compiling *c;
	int op;
	node *n;
{
	char *name;
	char buffer[1000];
	/* XXX it is possible to write this code without the 1000
	   chars on the total length of dotted names, I just can't be
	   bothered right now */
	if (TYPE(n) == STAR)
		name = "*";
	else if (TYPE(n) == dotted_name) {
		char *p = buffer;
		int i;
		name = buffer;
		for (i = 0; i < NCH(n); i += 2) {
			char *s = STR(CHILD(n, i));
			if (p + strlen(s) > buffer + (sizeof buffer) - 2) {
				err_setstr(MemoryError,
					   "dotted_name too long");
				name = NULL;
				break;
			}
			if (p != buffer)
				*p++ = '.';
			strcpy(p, s);
			p = strchr(p, '\0');
		}
	}
	else {
		REQ(n, NAME);
		name = STR(n);
	}
	com_addopnamestr(c, op, name);
}

static object *
parsenumber(s)
	char *s;
{
	extern long mystrtol PROTO((const char *, char **, int));
	extern unsigned long mystrtoul PROTO((const char *, char **, int));
	extern double atof PROTO((const char *));
	char *end;
	long x;
	errno = 0;
	end = s + strlen(s) - 1;
	if (*end == 'l' || *end == 'L')
		return long_scan(s, 0);
	if (s[0] == '0')
		x = (long) mystrtoul(s, &end, 0);
	else
		x = mystrtol(s, &end, 0);
	if (*end == '\0') {
		if (errno != 0) {
			err_setstr(OverflowError,
				   "integer literal too large");
			return NULL;
		}
		return newintobject(x);
	}
	/* XXX Huge floats may silently fail */
	return newfloatobject(atof(s));
}

static object *
parsestr(s)
	char *s;
{
	object *v;
	int len;
	char *buf;
	char *p;
	char *end;
	int c;
	int quote = *s;
	if (quote != '\'' && quote != '\"') {
		err_badcall();
		return NULL;
	}
	s++;
	len = strlen(s);
	if (s[--len] != quote) {
		err_badcall();
		return NULL;
	}
	if (len >= 4 && s[0] == quote && s[1] == quote) {
		s += 2;
		len -= 2;
		if (s[--len] != quote || s[--len] != quote) {
			err_badcall();
			return NULL;
		}
	}
	if (strchr(s, '\\') == NULL)
		return newsizedstringobject(s, len);
	v = newsizedstringobject((char *)NULL, len);
	p = buf = getstringvalue(v);
	end = s + len;
	while (s < end) {
		if (*s != '\\') {
			*p++ = *s++;
			continue;
		}
		s++;
		switch (*s++) {
		/* XXX This assumes ASCII! */
		case '\n': break;
		case '\\': *p++ = '\\'; break;
		case '\'': *p++ = '\''; break;
		case '\"': *p++ = '\"'; break;
		case 'b': *p++ = '\b'; break;
		case 'f': *p++ = '\014'; break; /* FF */
		case 't': *p++ = '\t'; break;
		case 'n': *p++ = '\n'; break;
		case 'r': *p++ = '\r'; break;
		case 'v': *p++ = '\013'; break; /* VT */
		case 'a': *p++ = '\007'; break; /* BEL, not classic C */
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			c = s[-1] - '0';
			if ('0' <= *s && *s <= '7') {
				c = (c<<3) + *s++ - '0';
				if ('0' <= *s && *s <= '7')
					c = (c<<3) + *s++ - '0';
			}
			*p++ = c;
			break;
		case 'x':
			if (isxdigit(Py_CHARMASK(*s))) {
				sscanf(s, "%x", &c);
				*p++ = c;
				do {
					s++;
				} while (isxdigit(Py_CHARMASK(*s)));
				break;
			}
		/* FALLTHROUGH */
		default: *p++ = '\\'; *p++ = s[-1]; break;
		}
	}
	resizestring(&v, (int)(p - buf));
	return v;
}

static object *
parsestrplus(n)
	node *n;
{
	object *v;
	int i;
	REQ(CHILD(n, 0), STRING);
	if ((v = parsestr(STR(CHILD(n, 0)))) != NULL) {
		/* String literal concatenation */
		for (i = 1; i < NCH(n) && v != NULL; i++) {
			joinstring_decref(&v, parsestr(STR(CHILD(n, i))));
		}
	}
	return v;
}

static void
com_list_constructor(c, n)
	struct compiling *c;
	node *n;
{
	int len;
	int i;
	if (TYPE(n) != testlist)
		REQ(n, exprlist);
	/* exprlist: expr (',' expr)* [',']; likewise for testlist */
	len = (NCH(n) + 1) / 2;
	for (i = 0; i < NCH(n); i += 2)
		com_node(c, CHILD(n, i));
	com_addoparg(c, BUILD_LIST, len);
}

static void
com_dictmaker(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	/* dictmaker: test ':' test (',' test ':' value)* [','] */
	for (i = 0; i+2 < NCH(n); i += 4) {
		/* We must arrange things just right for STORE_SUBSCR.
		   It wants the stack to look like (value) (dict) (key) */
		com_addbyte(c, DUP_TOP);
		com_node(c, CHILD(n, i+2)); /* value */
		com_addbyte(c, ROT_TWO);
		com_node(c, CHILD(n, i)); /* key */
		com_addbyte(c, STORE_SUBSCR);
	}
}

static void
com_atom(c, n)
	struct compiling *c;
	node *n;
{
	node *ch;
	object *v;
	int i;
	REQ(n, atom);
	ch = CHILD(n, 0);
	switch (TYPE(ch)) {
	case LPAR:
		if (TYPE(CHILD(n, 1)) == RPAR)
			com_addoparg(c, BUILD_TUPLE, 0);
		else
			com_node(c, CHILD(n, 1));
		break;
	case LSQB:
		if (TYPE(CHILD(n, 1)) == RSQB)
			com_addoparg(c, BUILD_LIST, 0);
		else
			com_list_constructor(c, CHILD(n, 1));
		break;
	case LBRACE: /* '{' [dictmaker] '}' */
		com_addoparg(c, BUILD_MAP, 0);
		if (TYPE(CHILD(n, 1)) != RBRACE)
			com_dictmaker(c, CHILD(n, 1));
		break;
	case BACKQUOTE:
		com_node(c, CHILD(n, 1));
		com_addbyte(c, UNARY_CONVERT);
		break;
	case NUMBER:
		if ((v = parsenumber(STR(ch))) == NULL) {
			c->c_errors++;
			i = 255;
		}
		else {
			i = com_addconst(c, v);
			DECREF(v);
		}
		com_addoparg(c, LOAD_CONST, i);
		break;
	case STRING:
		v = parsestrplus(n);
		if (v == NULL) {
			c->c_errors++;
			i = 255;
		}
		else {
			i = com_addconst(c, v);
			DECREF(v);
		}
		com_addoparg(c, LOAD_CONST, i);
		break;
	case NAME:
		com_addopname(c, LOAD_NAME, ch);
		break;
	default:
		fprintf(stderr, "node type %d\n", TYPE(ch));
		err_setstr(SystemError, "com_atom: unexpected node type");
		c->c_errors++;
	}
}

static void
com_slice(c, n, op)
	struct compiling *c;
	node *n;
	int op;
{
	if (NCH(n) == 1) {
		com_addbyte(c, op);
	}
	else if (NCH(n) == 2) {
		if (TYPE(CHILD(n, 0)) != COLON) {
			com_node(c, CHILD(n, 0));
			com_addbyte(c, op+1);
		}
		else {
			com_node(c, CHILD(n, 1));
			com_addbyte(c, op+2);
		}
	}
	else {
		com_node(c, CHILD(n, 0));
		com_node(c, CHILD(n, 2));
		com_addbyte(c, op+3);
	}
}

static void
com_apply_subscript(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, subscript);
	if (NCH(n) == 1 && TYPE(CHILD(n, 0)) != COLON) {
		/* It's a single subscript */
		com_node(c, CHILD(n, 0));
		com_addbyte(c, BINARY_SUBSCR);
	}
	else {
		/* It's a slice: [expr] ':' [expr] */
		com_slice(c, n, SLICE);
	}
}

static int
com_argument(c, n, inkeywords)
	struct compiling *c;
	node *n; /* argument */
	int inkeywords;
{
	node *m;
	REQ(n, argument); /* [test '='] test; really [ keyword '='] keyword */
	if (NCH(n) == 1) {
		if (inkeywords) {
			err_setstr(SyntaxError,
				   "non-keyword arg after keyword arg");
			c->c_errors++;
		}
		else {
			com_node(c, CHILD(n, 0));
		}
		return 0;
	}
	m = n;
	do {
		m = CHILD(m, 0);
	} while (NCH(m) == 1);
	if (TYPE(m) != NAME) {
		err_setstr(SyntaxError, "keyword can't be an expression");
		c->c_errors++;
	}
	else {
		object *v = newstringobject(STR(m));
		if (v == NULL)
			c->c_errors++;
		else {
			com_addoparg(c, LOAD_CONST, com_addconst(c, v));
			DECREF(v);
		}
	}
	com_node(c, CHILD(n, 2));
	return 1;
}

static void
com_call_function(c, n)
	struct compiling *c;
	node *n; /* EITHER arglist OR ')' */
{
	if (TYPE(n) == RPAR) {
		com_addoparg(c, CALL_FUNCTION, 0);
	}
	else {
		int inkeywords, i, na, nk;
		REQ(n, arglist);
		inkeywords = 0;
		na = 0;
		nk = 0;
		for (i = 0; i < NCH(n); i += 2) {
			inkeywords = com_argument(c, CHILD(n, i), inkeywords);
			if (!inkeywords)
				na++;
			else
				nk++;
		}
		if (na > 255 || nk > 255) {
			err_setstr(SyntaxError, "more than 255 arguments");
			c->c_errors++;
		}
		com_addoparg(c, CALL_FUNCTION, na | (nk << 8));
	}
}

static void
com_select_member(c, n)
	struct compiling *c;
	node *n;
{
	com_addopname(c, LOAD_ATTR, n);
}

static void
com_apply_trailer(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, trailer);
	switch (TYPE(CHILD(n, 0))) {
	case LPAR:
		com_call_function(c, CHILD(n, 1));
		break;
	case DOT:
		com_select_member(c, CHILD(n, 1));
		break;
	case LSQB:
		com_apply_subscript(c, CHILD(n, 1));
		break;
	default:
		err_setstr(SystemError,
			"com_apply_trailer: unknown trailer type");
		c->c_errors++;
	}
}

static void
com_factor(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	REQ(n, factor);
	if (TYPE(CHILD(n, 0)) == PLUS) {
		com_factor(c, CHILD(n, 1));
		com_addbyte(c, UNARY_POSITIVE);
	}
	else if (TYPE(CHILD(n, 0)) == MINUS) {
		com_factor(c, CHILD(n, 1));
		com_addbyte(c, UNARY_NEGATIVE);
	}
	else if (TYPE(CHILD(n, 0)) == TILDE) {
		com_factor(c, CHILD(n, 1));
		com_addbyte(c, UNARY_INVERT);
	}
	else {
		com_atom(c, CHILD(n, 0));
		for (i = 1; i < NCH(n); i++)
			com_apply_trailer(c, CHILD(n, i));
	}
}

static void
com_term(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int op;
	REQ(n, term);
	com_factor(c, CHILD(n, 0));
	for (i = 2; i < NCH(n); i += 2) {
		com_factor(c, CHILD(n, i));
		switch (TYPE(CHILD(n, i-1))) {
		case STAR:
			op = BINARY_MULTIPLY;
			break;
		case SLASH:
			op = BINARY_DIVIDE;
			break;
		case PERCENT:
			op = BINARY_MODULO;
			break;
		default:
			err_setstr(SystemError,
				"com_term: operator not *, / or %");
			c->c_errors++;
			op = 255;
		}
		com_addbyte(c, op);
	}
}

static void
com_arith_expr(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int op;
	REQ(n, arith_expr);
	com_term(c, CHILD(n, 0));
	for (i = 2; i < NCH(n); i += 2) {
		com_term(c, CHILD(n, i));
		switch (TYPE(CHILD(n, i-1))) {
		case PLUS:
			op = BINARY_ADD;
			break;
		case MINUS:
			op = BINARY_SUBTRACT;
			break;
		default:
			err_setstr(SystemError,
				"com_arith_expr: operator not + or -");
			c->c_errors++;
			op = 255;
		}
		com_addbyte(c, op);
	}
}

static void
com_shift_expr(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int op;
	REQ(n, shift_expr);
	com_arith_expr(c, CHILD(n, 0));
	for (i = 2; i < NCH(n); i += 2) {
		com_arith_expr(c, CHILD(n, i));
		switch (TYPE(CHILD(n, i-1))) {
		case LEFTSHIFT:
			op = BINARY_LSHIFT;
			break;
		case RIGHTSHIFT:
			op = BINARY_RSHIFT;
			break;
		default:
			err_setstr(SystemError,
				"com_shift_expr: operator not << or >>");
			c->c_errors++;
			op = 255;
		}
		com_addbyte(c, op);
	}
}

static void
com_and_expr(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int op;
	REQ(n, and_expr);
	com_shift_expr(c, CHILD(n, 0));
	for (i = 2; i < NCH(n); i += 2) {
		com_shift_expr(c, CHILD(n, i));
		if (TYPE(CHILD(n, i-1)) == AMPER) {
			op = BINARY_AND;
		}
		else {
			err_setstr(SystemError,
				"com_and_expr: operator not &");
			c->c_errors++;
			op = 255;
		}
		com_addbyte(c, op);
	}
}

static void
com_xor_expr(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int op;
	REQ(n, xor_expr);
	com_and_expr(c, CHILD(n, 0));
	for (i = 2; i < NCH(n); i += 2) {
		com_and_expr(c, CHILD(n, i));
		if (TYPE(CHILD(n, i-1)) == CIRCUMFLEX) {
			op = BINARY_XOR;
		}
		else {
			err_setstr(SystemError,
				"com_xor_expr: operator not ^");
			c->c_errors++;
			op = 255;
		}
		com_addbyte(c, op);
	}
}

static void
com_expr(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int op;
	REQ(n, expr);
	com_xor_expr(c, CHILD(n, 0));
	for (i = 2; i < NCH(n); i += 2) {
		com_xor_expr(c, CHILD(n, i));
		if (TYPE(CHILD(n, i-1)) == VBAR) {
			op = BINARY_OR;
		}
		else {
			err_setstr(SystemError,
				"com_expr: expr operator not |");
			c->c_errors++;
			op = 255;
		}
		com_addbyte(c, op);
	}
}

static enum cmp_op
cmp_type(n)
	node *n;
{
	REQ(n, comp_op);
	/* comp_op: '<' | '>' | '=' | '>=' | '<=' | '<>' | '!=' | '=='
	          | 'in' | 'not' 'in' | 'is' | 'is' not' */
	if (NCH(n) == 1) {
		n = CHILD(n, 0);
		switch (TYPE(n)) {
		case LESS:	return LT;
		case GREATER:	return GT;
		case EQEQUAL:			/* == */
		case EQUAL:	return EQ;
		case LESSEQUAL:	return LE;
		case GREATEREQUAL: return GE;
		case NOTEQUAL:	return NE;	/* <> or != */
		case NAME:	if (strcmp(STR(n), "in") == 0) return IN;
				if (strcmp(STR(n), "is") == 0) return IS;
		}
	}
	else if (NCH(n) == 2) {
		switch (TYPE(CHILD(n, 0))) {
		case NAME:	if (strcmp(STR(CHILD(n, 1)), "in") == 0)
					return NOT_IN;
				if (strcmp(STR(CHILD(n, 0)), "is") == 0)
					return IS_NOT;
		}
	}
	return BAD;
}

static void
com_comparison(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	enum cmp_op op;
	int anchor;
	REQ(n, comparison); /* comparison: expr (comp_op expr)* */
	com_expr(c, CHILD(n, 0));
	if (NCH(n) == 1)
		return;
	
	/****************************************************************
	   The following code is generated for all but the last
	   comparison in a chain:
	   
	   label:	on stack:	opcode:		jump to:
	   
			a		<code to load b>
			a, b		DUP_TOP
			a, b, b		ROT_THREE
			b, a, b		COMPARE_OP
			b, 0-or-1	JUMP_IF_FALSE	L1
			b, 1		POP_TOP
			b		
	
	   We are now ready to repeat this sequence for the next
	   comparison in the chain.
	   
	   For the last we generate:
	   
	   		b		<code to load c>
	   		b, c		COMPARE_OP
	   		0-or-1		
	   
	   If there were any jumps to L1 (i.e., there was more than one
	   comparison), we generate:
	   
	   		0-or-1		JUMP_FORWARD	L2
	   L1:		b, 0		ROT_TWO
	   		0, b		POP_TOP
	   		0
	   L2:
	****************************************************************/
	
	anchor = 0;
	
	for (i = 2; i < NCH(n); i += 2) {
		com_expr(c, CHILD(n, i));
		if (i+2 < NCH(n)) {
			com_addbyte(c, DUP_TOP);
			com_addbyte(c, ROT_THREE);
		}
		op = cmp_type(CHILD(n, i-1));
		if (op == BAD) {
			err_setstr(SystemError,
				"com_comparison: unknown comparison op");
			c->c_errors++;
		}
		com_addoparg(c, COMPARE_OP, op);
		if (i+2 < NCH(n)) {
			com_addfwref(c, JUMP_IF_FALSE, &anchor);
			com_addbyte(c, POP_TOP);
		}
	}
	
	if (anchor) {
		int anchor2 = 0;
		com_addfwref(c, JUMP_FORWARD, &anchor2);
		com_backpatch(c, anchor);
		com_addbyte(c, ROT_TWO);
		com_addbyte(c, POP_TOP);
		com_backpatch(c, anchor2);
	}
}

static void
com_not_test(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, not_test); /* 'not' not_test | comparison */
	if (NCH(n) == 1) {
		com_comparison(c, CHILD(n, 0));
	}
	else {
		com_not_test(c, CHILD(n, 1));
		com_addbyte(c, UNARY_NOT);
	}
}

static void
com_and_test(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int anchor;
	REQ(n, and_test); /* not_test ('and' not_test)* */
	anchor = 0;
	i = 0;
	for (;;) {
		com_not_test(c, CHILD(n, i));
		if ((i += 2) >= NCH(n))
			break;
		com_addfwref(c, JUMP_IF_FALSE, &anchor);
		com_addbyte(c, POP_TOP);
	}
	if (anchor)
		com_backpatch(c, anchor);
}

static void
com_test(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, test); /* and_test ('and' and_test)* | lambdef */
	if (NCH(n) == 1 && TYPE(CHILD(n, 0)) == lambdef) {
		object *v;
		int i;
		int argcount;
		int ndefs = com_argdefs(c, CHILD(n, 0), &argcount);
		v = (object *) compile(CHILD(n, 0), c->c_filename);
		if (v == NULL) {
			c->c_errors++;
			i = 255;
		}
		else {
			i = com_addconst(c, v);
			DECREF(v);
		}
		com_addoparg(c, LOAD_CONST, i);
		com_addbyte(c, BUILD_FUNCTION);
		if (ndefs > 0)
			com_addoparg(c, SET_FUNC_ARGS, argcount);
	}
	else {
		int anchor = 0;
		int i = 0;
		for (;;) {
			com_and_test(c, CHILD(n, i));
			if ((i += 2) >= NCH(n))
				break;
			com_addfwref(c, JUMP_IF_TRUE, &anchor);
			com_addbyte(c, POP_TOP);
		}
		if (anchor)
			com_backpatch(c, anchor);
	}
}

static void
com_list(c, n, toplevel)
	struct compiling *c;
	node *n;
	int toplevel; /* If nonzero, *always* build a tuple */
{
	/* exprlist: expr (',' expr)* [',']; likewise for testlist */
	if (NCH(n) == 1 && !toplevel) {
		com_node(c, CHILD(n, 0));
	}
	else {
		int i;
		int len;
		len = (NCH(n) + 1) / 2;
		for (i = 0; i < NCH(n); i += 2)
			com_node(c, CHILD(n, i));
		com_addoparg(c, BUILD_TUPLE, len);
	}
}


/* Begin of assignment compilation */

static void com_assign_name PROTO((struct compiling *, node *, int));
static void com_assign PROTO((struct compiling *, node *, int));

static void
com_assign_attr(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	com_addopname(c, assigning ? STORE_ATTR : DELETE_ATTR, n);
}

static void
com_assign_slice(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	com_slice(c, n, assigning ? STORE_SLICE : DELETE_SLICE);
}

static void
com_assign_subscript(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	com_node(c, n);
	com_addbyte(c, assigning ? STORE_SUBSCR : DELETE_SUBSCR);
}

static void
com_assign_trailer(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	REQ(n, trailer);
	switch (TYPE(CHILD(n, 0))) {
	case LPAR: /* '(' [exprlist] ')' */
		err_setstr(SyntaxError, "can't assign to function call");
		c->c_errors++;
		break;
	case DOT: /* '.' NAME */
		com_assign_attr(c, CHILD(n, 1), assigning);
		break;
	case LSQB: /* '[' subscript ']' */
		n = CHILD(n, 1);
		REQ(n, subscript); /* subscript: expr | [expr] ':' [expr] */
		if (NCH(n) > 1 || TYPE(CHILD(n, 0)) == COLON)
			com_assign_slice(c, n, assigning);
		else
			com_assign_subscript(c, CHILD(n, 0), assigning);
		break;
	default:
		err_setstr(SystemError, "unknown trailer type");
		c->c_errors++;
	}
}

static void
com_assign_tuple(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	int i;
	if (TYPE(n) != testlist)
		REQ(n, exprlist);
	if (assigning)
		com_addoparg(c, UNPACK_TUPLE, (NCH(n)+1)/2);
	for (i = 0; i < NCH(n); i += 2)
		com_assign(c, CHILD(n, i), assigning);
}

static void
com_assign_list(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	int i;
	if (assigning)
		com_addoparg(c, UNPACK_LIST, (NCH(n)+1)/2);
	for (i = 0; i < NCH(n); i += 2)
		com_assign(c, CHILD(n, i), assigning);
}

static void
com_assign_name(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	REQ(n, NAME);
	com_addopname(c, assigning ? STORE_NAME : DELETE_NAME, n);
}

static void
com_assign(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	/* Loop to avoid trivial recursion */
	for (;;) {
		switch (TYPE(n)) {
		
		case exprlist:
		case testlist:
			if (NCH(n) > 1) {
				com_assign_tuple(c, n, assigning);
				return;
			}
			n = CHILD(n, 0);
			break;
		
		case test:
		case and_test:
		case not_test:
		case comparison:
		case expr:
		case xor_expr:
		case and_expr:
		case shift_expr:
		case arith_expr:
		case term:
			if (NCH(n) > 1) {
				err_setstr(SyntaxError,
					"can't assign to operator");
				c->c_errors++;
				return;
			}
			n = CHILD(n, 0);
			break;
		
		case factor: /* ('+'|'-'|'~') factor | atom trailer* */
			if (TYPE(CHILD(n, 0)) != atom) { /* '+'|'-'|'~' */
				err_setstr(SyntaxError,
					"can't assign to operator");
				c->c_errors++;
				return;
			}
			if (NCH(n) > 1) { /* trailer present */
				int i;
				com_node(c, CHILD(n, 0));
				for (i = 1; i+1 < NCH(n); i++) {
					com_apply_trailer(c, CHILD(n, i));
				} /* NB i is still alive */
				com_assign_trailer(c,
						CHILD(n, i), assigning);
				return;
			}
			n = CHILD(n, 0);
			break;
		
		case atom:
			switch (TYPE(CHILD(n, 0))) {
			case LPAR:
				n = CHILD(n, 1);
				if (TYPE(n) == RPAR) {
					/* XXX Should allow () = () ??? */
					err_setstr(SyntaxError,
						"can't assign to ()");
					c->c_errors++;
					return;
				}
				break;
			case LSQB:
				n = CHILD(n, 1);
				if (TYPE(n) == RSQB) {
					err_setstr(SyntaxError,
						"can't assign to []");
					c->c_errors++;
					return;
				}
				com_assign_list(c, n, assigning);
				return;
			case NAME:
				com_assign_name(c, CHILD(n, 0), assigning);
				return;
			default:
				err_setstr(SyntaxError,
						"can't assign to literal");
				c->c_errors++;
				return;
			}
			break;
		
		default:
			fprintf(stderr, "node type %d\n", TYPE(n));
			err_setstr(SystemError, "com_assign: bad node");
			c->c_errors++;
			return;
		
		}
	}
}

static void
com_expr_stmt(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, expr_stmt); /* testlist ('=' testlist)* */
	com_node(c, CHILD(n, NCH(n)-1));
	if (NCH(n) == 1) {
		if (c->c_interactive)
			com_addbyte(c, PRINT_EXPR);
		else
			com_addbyte(c, POP_TOP);
	}
	else {
		int i;
		for (i = 0; i < NCH(n)-2; i+=2) {
			if (i+2 < NCH(n)-2)
				com_addbyte(c, DUP_TOP);
			com_assign(c, CHILD(n, i), 1/*assign*/);
		}
	}
}

static void
com_print_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	REQ(n, print_stmt); /* 'print' (test ',')* [test] */
	for (i = 1; i < NCH(n); i += 2) {
		com_node(c, CHILD(n, i));
		com_addbyte(c, PRINT_ITEM);
	}
	if (TYPE(CHILD(n, NCH(n)-1)) != COMMA)
		com_addbyte(c, PRINT_NEWLINE);
		/* XXX Alternatively, LOAD_CONST '\n' and then PRINT_ITEM */
}

static void
com_return_stmt(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, return_stmt); /* 'return' [testlist] */
	if (!c->c_infunction) {
		err_setstr(SyntaxError, "'return' outside function");
		c->c_errors++;
	}
	if (NCH(n) < 2)
		com_addoparg(c, LOAD_CONST, com_addconst(c, None));
	else
		com_node(c, CHILD(n, 1));
	com_addbyte(c, RETURN_VALUE);
}

static void
com_raise_stmt(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, raise_stmt); /* 'raise' test [',' test [',' test]] */
	com_node(c, CHILD(n, 1));
	if (NCH(n) > 3)
		com_node(c, CHILD(n, 3));
	else
		com_addoparg(c, LOAD_CONST, com_addconst(c, None));
	if (NCH(n) > 5) {
		com_node(c, CHILD(n, 5));
		com_addoparg(c, RAISE_VARARGS, 3);
	}
	else
		com_addbyte(c, RAISE_EXCEPTION);
}

static void
com_import_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	REQ(n, import_stmt);
	/* 'import' dotted_name (',' dotted_name)* |
	   'from' dotted_name 'import' ('*' | NAME (',' NAME)*) */
	if (STR(CHILD(n, 0))[0] == 'f') {
		/* 'from' dotted_name 'import' ... */
		REQ(CHILD(n, 1), dotted_name);
		com_addopname(c, IMPORT_NAME, CHILD(n, 1));
		for (i = 3; i < NCH(n); i += 2)
			com_addopname(c, IMPORT_FROM, CHILD(n, i));
		com_addbyte(c, POP_TOP);
	}
	else {
		/* 'import' ... */
		for (i = 1; i < NCH(n); i += 2) {
			REQ(CHILD(n, i), dotted_name);
			com_addopname(c, IMPORT_NAME, CHILD(n, i));
			com_addopname(c, STORE_NAME, CHILD(CHILD(n, i), 0));
		}
	}
}

static void
com_global_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	REQ(n, global_stmt);
	/* 'global' NAME (',' NAME)* */
	for (i = 1; i < NCH(n); i += 2) {
		if (dictinsert(c->c_globals, STR(CHILD(n, i)), None) != 0)
			c->c_errors++;
	}
}

#define strequ(a, b) (strcmp((a), (b)) == 0)

static void
com_access_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int i, j, k, mode, imode;
	object *vmode;
	REQ(n, access_stmt);
	/* 'access' NAME (',' NAME)* ':' accesstype (',' accesstype)*
	   accesstype: NAME+ */

	/* Find where the colon is */
	i = 1;
	while (TYPE(CHILD(n,i-1)) != COLON)
		i += 1;

	/* Calculate the mode mask */
	mode = 0;
	for (j = i; j < NCH(n); j += 2) {
		int r = 0, w = 0, p = 0;
		for (k = 0; k < NCH(CHILD(n,j)); k++) {
			if (strequ(STR(CHILD(CHILD(n,j),k)), "public"))
				p = 0;
			else if (strequ(STR(CHILD(CHILD(n,j),k)), "protected"))
				p = 1;
			else if (strequ(STR(CHILD(CHILD(n,j),k)), "private"))
				p = 2;
			else if (strequ(STR(CHILD(CHILD(n,j),k)), "read"))
				r = 1;
			else if (strequ(STR(CHILD(CHILD(n,j),k)), "write"))
				w = 1;
			else /* XXX should make this an exception */
				fprintf(stderr, "bad access type %s\n",
					STR(CHILD(CHILD(n,j),k)));
		}
		if (r == 0 && w == 0)
			r = w = 1;
		if (p == 0) {
			if (r == 1) mode |= AC_R_PUBLIC;
			if (w == 1) mode |= AC_W_PUBLIC;
		} else if (p == 1) {
			if (r == 1) mode |= AC_R_PROTECTED;
			if (w == 1) mode |= AC_W_PROTECTED;
		} else {
			if (r == 1) mode |= AC_R_PRIVATE;
			if (w == 1) mode |= AC_W_PRIVATE;
		}
	}
	vmode = newintobject((long)mode);
	imode = com_addconst(c, vmode);
	XDECREF(vmode);
	for (i = 1; TYPE(CHILD(n,i-1)) != COLON; i+=2) {
		com_addoparg(c, LOAD_CONST, imode);
		com_addopname(c, ACCESS_MODE, CHILD(n, i));
	}
}

static void
com_exec_stmt(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, exec_stmt);
	/* exec_stmt: 'exec' expr ['in' expr [',' expr]] */
	com_node(c, CHILD(n, 1));
	if (NCH(n) >= 4)
		com_node(c, CHILD(n, 3));
	else
		com_addoparg(c, LOAD_CONST, com_addconst(c, None));
	if (NCH(n) >= 6)
		com_node(c, CHILD(n, 5));
	else
		com_addbyte(c, DUP_TOP);
	com_addbyte(c, EXEC_STMT);
}

static void
com_if_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int anchor = 0;
	REQ(n, if_stmt);
	/*'if' test ':' suite ('elif' test ':' suite)* ['else' ':' suite] */
	for (i = 0; i+3 < NCH(n); i+=4) {
		int a = 0;
		node *ch = CHILD(n, i+1);
		if (i > 0)
			com_addoparg(c, SET_LINENO, ch->n_lineno);
		com_node(c, CHILD(n, i+1));
		com_addfwref(c, JUMP_IF_FALSE, &a);
		com_addbyte(c, POP_TOP);
		com_node(c, CHILD(n, i+3));
		com_addfwref(c, JUMP_FORWARD, &anchor);
		com_backpatch(c, a);
		com_addbyte(c, POP_TOP);
	}
	if (i+2 < NCH(n))
		com_node(c, CHILD(n, i+2));
	com_backpatch(c, anchor);
}

static void
com_while_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int break_anchor = 0;
	int anchor = 0;
	int save_begin = c->c_begin;
	REQ(n, while_stmt); /* 'while' test ':' suite ['else' ':' suite] */
	com_addfwref(c, SETUP_LOOP, &break_anchor);
	block_push(c, SETUP_LOOP);
	c->c_begin = c->c_nexti;
	com_addoparg(c, SET_LINENO, n->n_lineno);
	com_node(c, CHILD(n, 1));
	com_addfwref(c, JUMP_IF_FALSE, &anchor);
	com_addbyte(c, POP_TOP);
	c->c_loops++;
	com_node(c, CHILD(n, 3));
	c->c_loops--;
	com_addoparg(c, JUMP_ABSOLUTE, c->c_begin);
	c->c_begin = save_begin;
	com_backpatch(c, anchor);
	com_addbyte(c, POP_TOP);
	com_addbyte(c, POP_BLOCK);
	block_pop(c, SETUP_LOOP);
	if (NCH(n) > 4)
		com_node(c, CHILD(n, 6));
	com_backpatch(c, break_anchor);
}

static void
com_for_stmt(c, n)
	struct compiling *c;
	node *n;
{
	object *v;
	int break_anchor = 0;
	int anchor = 0;
	int save_begin = c->c_begin;
	REQ(n, for_stmt);
	/* 'for' exprlist 'in' exprlist ':' suite ['else' ':' suite] */
	com_addfwref(c, SETUP_LOOP, &break_anchor);
	block_push(c, SETUP_LOOP);
	com_node(c, CHILD(n, 3));
	v = newintobject(0L);
	if (v == NULL)
		c->c_errors++;
	com_addoparg(c, LOAD_CONST, com_addconst(c, v));
	XDECREF(v);
	c->c_begin = c->c_nexti;
	com_addoparg(c, SET_LINENO, n->n_lineno);
	com_addfwref(c, FOR_LOOP, &anchor);
	com_assign(c, CHILD(n, 1), 1/*assigning*/);
	c->c_loops++;
	com_node(c, CHILD(n, 5));
	c->c_loops--;
	com_addoparg(c, JUMP_ABSOLUTE, c->c_begin);
	c->c_begin = save_begin;
	com_backpatch(c, anchor);
	com_addbyte(c, POP_BLOCK);
	block_pop(c, SETUP_LOOP);
	if (NCH(n) > 8)
		com_node(c, CHILD(n, 8));
	com_backpatch(c, break_anchor);
}

/* Code generated for "try: S finally: Sf" is as follows:
   
		SETUP_FINALLY	L
		<code for S>
		POP_BLOCK
		LOAD_CONST	<nil>
	L:	<code for Sf>
		END_FINALLY
   
   The special instructions use the block stack.  Each block
   stack entry contains the instruction that created it (here
   SETUP_FINALLY), the level of the value stack at the time the
   block stack entry was created, and a label (here L).
   
   SETUP_FINALLY:
	Pushes the current value stack level and the label
	onto the block stack.
   POP_BLOCK:
	Pops en entry from the block stack, and pops the value
	stack until its level is the same as indicated on the
	block stack.  (The label is ignored.)
   END_FINALLY:
	Pops a variable number of entries from the *value* stack
	and re-raises the exception they specify.  The number of
	entries popped depends on the (pseudo) exception type.
   
   The block stack is unwound when an exception is raised:
   when a SETUP_FINALLY entry is found, the exception is pushed
   onto the value stack (and the exception condition is cleared),
   and the interpreter jumps to the label gotten from the block
   stack.
   
   Code generated for "try: S except E1, V1: S1 except E2, V2: S2 ...":
   (The contents of the value stack is shown in [], with the top
   at the right; 'tb' is trace-back info, 'val' the exception's
   associated value, and 'exc' the exception.)
   
   Value stack		Label	Instruction	Argument
   []				SETUP_EXCEPT	L1
   []				<code for S>
   []				POP_BLOCK
   []				JUMP_FORWARD	L0
   
   [tb, val, exc]	L1:	DUP				)
   [tb, val, exc, exc]		<evaluate E1>			)
   [tb, val, exc, exc, E1]	COMPARE_OP	EXC_MATCH	) only if E1
   [tb, val, exc, 1-or-0]	JUMP_IF_FALSE	L2		)
   [tb, val, exc, 1]		POP				)
   [tb, val, exc]		POP
   [tb, val]			<assign to V1>	(or POP if no V1)
   [tb]				POP
   []				<code for S1>
   				JUMP_FORWARD	L0
   
   [tb, val, exc, 0]	L2:	POP
   [tb, val, exc]		DUP
   .............................etc.......................

   [tb, val, exc, 0]	Ln+1:	POP
   [tb, val, exc]	   	END_FINALLY	# re-raise exception
   
   []			L0:	<next statement>
   
   Of course, parts are not generated if Vi or Ei is not present.
*/

static void
com_try_except(c, n)
	struct compiling *c;
	node *n;
{
	int except_anchor = 0;
	int end_anchor = 0;
	int else_anchor = 0;
	int i;
	node *ch;

	com_addfwref(c, SETUP_EXCEPT, &except_anchor);
	block_push(c, SETUP_EXCEPT);
	com_node(c, CHILD(n, 2));
	com_addbyte(c, POP_BLOCK);
	block_pop(c, SETUP_EXCEPT);
	com_addfwref(c, JUMP_FORWARD, &else_anchor);
	com_backpatch(c, except_anchor);
	for (i = 3;
	     i < NCH(n) && TYPE(ch = CHILD(n, i)) == except_clause;
	     i += 3) {
		/* except_clause: 'except' [expr [',' expr]] */
		if (except_anchor == 0) {
			err_setstr(SyntaxError,
				"default 'except:' must be last");
			c->c_errors++;
			break;
		}
		except_anchor = 0;
		com_addoparg(c, SET_LINENO, ch->n_lineno);
		if (NCH(ch) > 1) {
			com_addbyte(c, DUP_TOP);
			com_node(c, CHILD(ch, 1));
			com_addoparg(c, COMPARE_OP, EXC_MATCH);
			com_addfwref(c, JUMP_IF_FALSE, &except_anchor);
			com_addbyte(c, POP_TOP);
		}
		com_addbyte(c, POP_TOP);
		if (NCH(ch) > 3)
			com_assign(c, CHILD(ch, 3), 1/*assigning*/);
		else
			com_addbyte(c, POP_TOP);
		com_addbyte(c, POP_TOP);
		com_node(c, CHILD(n, i+2));
		com_addfwref(c, JUMP_FORWARD, &end_anchor);
		if (except_anchor) {
			com_backpatch(c, except_anchor);
			com_addbyte(c, POP_TOP);
		}
	}
	com_addbyte(c, END_FINALLY);
	com_backpatch(c, else_anchor);
	if (i < NCH(n))
		com_node(c, CHILD(n, i+2));
	com_backpatch(c, end_anchor);
}

static void
com_try_finally(c, n)
	struct compiling *c;
	node *n;
{
	int finally_anchor = 0;
	node *ch;

	com_addfwref(c, SETUP_FINALLY, &finally_anchor);
	block_push(c, SETUP_FINALLY);
	com_node(c, CHILD(n, 2));
	com_addbyte(c, POP_BLOCK);
	block_pop(c, SETUP_FINALLY);
	block_push(c, END_FINALLY);
	com_addoparg(c, LOAD_CONST, com_addconst(c, None));
	com_backpatch(c, finally_anchor);
	ch = CHILD(n, NCH(n)-1);
	com_addoparg(c, SET_LINENO, ch->n_lineno);
	com_node(c, ch);
	com_addbyte(c, END_FINALLY);
	block_pop(c, END_FINALLY);
}

static void
com_try_stmt(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, try_stmt);
	/* 'try' ':' suite (except_clause ':' suite)+ ['else' ':' suite]
	 | 'try' ':' suite 'finally' ':' suite */
	if (TYPE(CHILD(n, 3)) != except_clause)
		com_try_finally(c, n);
	else
		com_try_except(c, n);
}

static object *
get_docstring(n)
	node *n;
{
	int i;

	switch (TYPE(n)) {

	case suite:
		if (NCH(n) == 1)
			return get_docstring(CHILD(n, 0));
		else {
			for (i = 0; i < NCH(n); i++) {
				node *ch = CHILD(n, i);
				if (TYPE(ch) == stmt)
					return get_docstring(ch);
			}
		}
		break;

	case file_input:
		for (i = 0; i < NCH(n); i++) {
			node *ch = CHILD(n, i);
			if (TYPE(ch) == stmt)
				return get_docstring(ch);
		}
		break;

	case stmt:
	case simple_stmt:
	case small_stmt:
		return get_docstring(CHILD(n, 0));

	case expr_stmt:
	case testlist:
	case test:
	case and_test:
	case not_test:
	case comparison:
	case expr:
	case xor_expr:
	case and_expr:
	case shift_expr:
	case arith_expr:
	case term:
	case factor:
		if (NCH(n) == 1)
			return get_docstring(CHILD(n, 0));
		break;

	case atom:
		if (TYPE(CHILD(n, 0)) == STRING)
			return parsestrplus(n);
		break;

	}
	return NULL;
}

static void
com_suite(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, suite);
	/* simple_stmt | NEWLINE INDENT NEWLINE* (stmt NEWLINE*)+ DEDENT */
	if (NCH(n) == 1) {
		com_node(c, CHILD(n, 0));
	}
	else {
		int i;
		for (i = 0; i < NCH(n); i++) {
			node *ch = CHILD(n, i);
			if (TYPE(ch) == stmt)
				com_node(c, ch);
		}
	}
}

/* ARGSUSED */
static void
com_continue_stmt(c, n)
	struct compiling *c;
	node *n; /* Not used, but passed for consistency */
{
	int i = c->c_nblocks;
	if (i-- > 0 && c->c_block[i] == SETUP_LOOP) {
		com_addoparg(c, JUMP_ABSOLUTE, c->c_begin);
	}
	else {
		err_setstr(SyntaxError, "'continue' not properly in loop");
		c->c_errors++;
	}
	/* XXX Could allow it inside a 'finally' clause
	   XXX if we could pop the exception still on the stack */
}

static int
com_argdefs(c, n, argcount_return)
	struct compiling *c;
	node *n;
	int *argcount_return;
{
	int i, nch, nargs, ndefs, star;
	if (TYPE(n) == lambdef) {
		/* lambdef: 'lambda' [varargslist] ':' test */
		n = CHILD(n, 1);
	}
	else {
		REQ(n, funcdef); /* funcdef: 'def' NAME parameters ... */
		n = CHILD(n, 2);
		REQ(n, parameters); /* parameters: '(' [varargslist] ')' */
		n = CHILD(n, 1);
	}
	if (TYPE(n) != varargslist)
		    return -1;
	/* varargslist:
		(fpdef ['=' test] ',')* '*' NAME ....... |
		fpdef ['=' test] (',' fpdef ['=' test])* [','] */
	nch = NCH(n);
	nargs = 0;
	ndefs = 0;
	star = 0;
	for (i = 0; i < nch; i++) {
		int t;
		if (TYPE(CHILD(n, i)) == STAR)
			break;
		nargs++;
		i++;
		if (i >= nch)
			break;
		t = TYPE(CHILD(n, i));
		if (t == EQUAL) {
			i++;
			ndefs++;
			com_node(c, CHILD(n, i));
			i++;
			if (i >= nch)
				break;
			t = TYPE(CHILD(n, i));
		}
		else {
			/* Treat "(a=1, b)" as "(a=1, b=None)" */
			if (ndefs) {
				com_addoparg(c, LOAD_CONST,
					     com_addconst(c, None));
				ndefs++;
			}
		}
		if (t != COMMA)
			break;
	}
	if (star)
		nargs ^= 0x4000;
	*argcount_return = nargs;
	if (ndefs > 0)
		com_addoparg(c, BUILD_TUPLE, ndefs);
	return ndefs;
}

static void
com_funcdef(c, n)
	struct compiling *c;
	node *n;
{
	object *v;
	REQ(n, funcdef); /* funcdef: 'def' NAME parameters ':' suite */
	v = (object *)compile(n, c->c_filename);
	if (v == NULL)
		c->c_errors++;
	else {
		int i = com_addconst(c, v);
		int argcount;
		int ndefs = com_argdefs(c, n, &argcount);
		com_addoparg(c, LOAD_CONST, i);
		com_addbyte(c, BUILD_FUNCTION);
		if (ndefs > 0)
			com_addoparg(c, SET_FUNC_ARGS, argcount);
		com_addopname(c, STORE_NAME, CHILD(n, 1));
		DECREF(v);
	}
}

static void
com_bases(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	REQ(n, testlist);
	/* testlist: test (',' test)* [','] */
	for (i = 0; i < NCH(n); i += 2)
		com_node(c, CHILD(n, i));
	com_addoparg(c, BUILD_TUPLE, (NCH(n)+1) / 2);
}

static void
com_classdef(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	object *v;
	REQ(n, classdef);
	/* classdef: class NAME ['(' testlist ')'] ':' suite */
	if ((v = newstringobject(STR(CHILD(n, 1)))) == NULL) {
		c->c_errors++;
		return;
	}
	/* Push the class name on the stack */
	i = com_addconst(c, v);
	com_addoparg(c, LOAD_CONST, i);
	DECREF(v);
	/* Push the tuple of base classes on the stack */
	if (TYPE(CHILD(n, 2)) != LPAR)
		com_addoparg(c, BUILD_TUPLE, 0);
	else
		com_bases(c, CHILD(n, 3));
	v = (object *)compile(n, c->c_filename);
	if (v == NULL)
		c->c_errors++;
	else {
		i = com_addconst(c, v);
		com_addoparg(c, LOAD_CONST, i);
		com_addbyte(c, BUILD_FUNCTION);
		com_addbyte(c, UNARY_CALL);
		com_addbyte(c, BUILD_CLASS);
		com_addopname(c, STORE_NAME, CHILD(n, 1));
		DECREF(v);
	}
}

static void
com_node(c, n)
	struct compiling *c;
	node *n;
{
	switch (TYPE(n)) {
	
	/* Definition nodes */
	
	case funcdef:
		com_funcdef(c, n);
		break;
	case classdef:
		com_classdef(c, n);
		break;
	
	/* Trivial parse tree nodes */
	
	case stmt:
	case small_stmt:
	case flow_stmt:
		com_node(c, CHILD(n, 0));
		break;

	case simple_stmt:
		/* small_stmt (';' small_stmt)* [';'] NEWLINE */
		com_addoparg(c, SET_LINENO, n->n_lineno);
		{
			int i;
			for (i = 0; i < NCH(n)-1; i += 2)
				com_node(c, CHILD(n, i));
		}
		break;
	
	case compound_stmt:
		com_addoparg(c, SET_LINENO, n->n_lineno);
		com_node(c, CHILD(n, 0));
		break;

	/* Statement nodes */
	
	case expr_stmt:
		com_expr_stmt(c, n);
		break;
	case print_stmt:
		com_print_stmt(c, n);
		break;
	case del_stmt: /* 'del' exprlist */
		com_assign(c, CHILD(n, 1), 0/*delete*/);
		break;
	case pass_stmt:
		break;
	case break_stmt:
		if (c->c_loops == 0) {
			err_setstr(SyntaxError, "'break' outside loop");
			c->c_errors++;
		}
		com_addbyte(c, BREAK_LOOP);
		break;
	case continue_stmt:
		com_continue_stmt(c, n);
		break;
	case return_stmt:
		com_return_stmt(c, n);
		break;
	case raise_stmt:
		com_raise_stmt(c, n);
		break;
	case import_stmt:
		com_import_stmt(c, n);
		break;
	case global_stmt:
		com_global_stmt(c, n);
		break;
	case access_stmt:
		com_access_stmt(c, n);
		break;
	case exec_stmt:
		com_exec_stmt(c, n);
		break;
	case if_stmt:
		com_if_stmt(c, n);
		break;
	case while_stmt:
		com_while_stmt(c, n);
		break;
	case for_stmt:
		com_for_stmt(c, n);
		break;
	case try_stmt:
		com_try_stmt(c, n);
		break;
	case suite:
		com_suite(c, n);
		break;
	
	/* Expression nodes */
	
	case testlist:
		com_list(c, n, 0);
		break;
	case test:
		com_test(c, n);
		break;
	case and_test:
		com_and_test(c, n);
		break;
	case not_test:
		com_not_test(c, n);
		break;
	case comparison:
		com_comparison(c, n);
		break;
	case exprlist:
		com_list(c, n, 0);
		break;
	case expr:
		com_expr(c, n);
		break;
	case xor_expr:
		com_xor_expr(c, n);
		break;
	case and_expr:
		com_and_expr(c, n);
		break;
	case shift_expr:
		com_shift_expr(c, n);
		break;
	case arith_expr:
		com_arith_expr(c, n);
		break;
	case term:
		com_term(c, n);
		break;
	case factor:
		com_factor(c, n);
		break;
	case atom:
		com_atom(c, n);
		break;
	
	default:
		fprintf(stderr, "node type %d\n", TYPE(n));
		err_setstr(SystemError, "com_node: unexpected node type");
		c->c_errors++;
	}
}

static void com_fplist PROTO((struct compiling *, node *));

static void
com_fpdef(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, fpdef); /* fpdef: NAME | '(' fplist ')' */
	if (TYPE(CHILD(n, 0)) == LPAR)
		com_fplist(c, CHILD(n, 1));
	else
		com_addopname(c, STORE_NAME, CHILD(n, 0));
}

static void
com_fplist(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, fplist); /* fplist: fpdef (',' fpdef)* [','] */
	if (NCH(n) == 1) {
		com_fpdef(c, CHILD(n, 0));
	}
	else {
		int i;
		com_addoparg(c, UNPACK_TUPLE, (NCH(n)+1)/2);
		for (i = 0; i < NCH(n); i += 2)
			com_fpdef(c, CHILD(n, i));
	}
}

static void
com_arglist(c, n)
	struct compiling *c;
	node *n;
{
	int nch, op, nargs, i, t;
	REQ(n, varargslist);
	/* varargslist:
		(fpdef ['=' test] ',')* '*' NAME ..... |
		fpdef ['=' test] (',' fpdef ['=' test])* [','] */
	nch = NCH(n);
	op = UNPACK_ARG;
	nargs = 0;
	for (i = 0; i < nch; i++) {
		if (TYPE(CHILD(n, i)) == STAR) {
			nch = i;
			if (TYPE(CHILD(n, i+1)) != STAR)
				op = UNPACK_VARARG;
			break;
		}
		nargs++;
		i++;
		if (i >= nch)
			break;
		t = TYPE(CHILD(n, i));
		if (t == EQUAL) {
			i += 2;
			if (i >= nch)
				break;
			t = TYPE(CHILD(n, i));
		}
		if (t != COMMA)
			break;
	}
	com_addoparg(c, op, nargs);
	for (i = 0; i < nch; i++) {
		com_fpdef(c, CHILD(n, i));
		i++;
		if (i >= nch)
			break;
		t = TYPE(CHILD(n, i));
		if (t == EQUAL) {
			i += 2;
			if (i >= nch)
				break;
			t = TYPE(CHILD(n, i));
		}
		if (t != COMMA)
			break;
	}
	if (op == UNPACK_VARARG)
		com_addopname(c, STORE_NAME, CHILD(n, nch+1));
}

static void
com_file_input(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	object *doc;
	REQ(n, file_input); /* (NEWLINE | stmt)* ENDMARKER */
	doc = get_docstring(n);
	if (doc != NULL) {
		int i = com_addconst(c, doc);
		DECREF(doc);
		com_addoparg(c, LOAD_CONST, i);
		com_addopnamestr(c, STORE_NAME, "__doc__");
	}
	for (i = 0; i < NCH(n); i++) {
		node *ch = CHILD(n, i);
		if (TYPE(ch) != ENDMARKER && TYPE(ch) != NEWLINE)
			com_node(c, ch);
	}
}

/* Top-level compile-node interface */

static void
compile_funcdef(c, n)
	struct compiling *c;
	node *n;
{
	object *doc;
	node *ch;
	REQ(n, funcdef); /* funcdef: 'def' NAME parameters ':' suite */
	c->c_name = STR(CHILD(n, 1));
	doc = get_docstring(CHILD(n, 4));
	if (doc != NULL) {
		(void) com_addconst(c, doc);
		DECREF(doc);
	}
	com_addoparg(c, RESERVE_FAST, com_addconst(c, None)); /* Patched! */
	ch = CHILD(n, 2); /* parameters: '(' [varargslist] ')' */
	ch = CHILD(ch, 1); /* ')' | varargslist */
	if (TYPE(ch) == RPAR)
		com_addoparg(c, UNPACK_ARG, 0);
	else
		com_arglist(c, ch);
	c->c_infunction = 1;
	com_node(c, CHILD(n, 4));
	c->c_infunction = 0;
	com_addoparg(c, LOAD_CONST, com_addconst(c, None));
	com_addbyte(c, RETURN_VALUE);
}

static void
compile_lambdef(c, n)
	struct compiling *c;
	node *n;
{
	node *ch;
	REQ(n, lambdef); /* lambdef: 'lambda' [parameters] ':' test */
	c->c_name = "<lambda>";

	ch = CHILD(n, 1);
	(void) com_addconst(c, None);
	if (TYPE(ch) == COLON) {
		com_addoparg(c, UNPACK_ARG, 0);
		com_node(c, CHILD(n, 2));
	}
	else {
		com_addoparg(c, RESERVE_FAST, com_addconst(c, None));
		com_arglist(c, ch);
		com_node(c, CHILD(n, 3));
	}

	com_addbyte(c, RETURN_VALUE);
}

static void
compile_classdef(c, n)
	struct compiling *c;
	node *n;
{
	node *ch;
	object *doc;
	REQ(n, classdef);
	/* classdef: 'class' NAME ['(' testlist ')'] ':' suite */
	c->c_name = STR(CHILD(n, 1));
	ch = CHILD(n, NCH(n)-1); /* The suite */
	doc = get_docstring(ch);
	if (doc != NULL) {
		int i = com_addconst(c, doc);
		DECREF(doc);
		com_addoparg(c, LOAD_CONST, i);
		com_addopnamestr(c, STORE_NAME, "__doc__");
	}
	else
		(void) com_addconst(c, None);
	com_node(c, ch);
	com_addbyte(c, LOAD_LOCALS);
	com_addbyte(c, RETURN_VALUE);
}

static void
compile_node(c, n)
	struct compiling *c;
	node *n;
{
	com_addoparg(c, SET_LINENO, n->n_lineno);
	
	switch (TYPE(n)) {
	
	case single_input: /* One interactive command */
		/* NEWLINE | simple_stmt | compound_stmt NEWLINE */
		c->c_interactive++;
		n = CHILD(n, 0);
		if (TYPE(n) != NEWLINE)
			com_node(c, n);
		com_addoparg(c, LOAD_CONST, com_addconst(c, None));
		com_addbyte(c, RETURN_VALUE);
		c->c_interactive--;
		break;
	
	case file_input: /* A whole file, or built-in function exec() */
		com_file_input(c, n);
		com_addoparg(c, LOAD_CONST, com_addconst(c, None));
		com_addbyte(c, RETURN_VALUE);
		break;
	
	case eval_input: /* Built-in function input() */
		com_node(c, CHILD(n, 0));
		com_addbyte(c, RETURN_VALUE);
		break;
	
	case lambdef: /* anonymous function definition */
		compile_lambdef(c, n);
		break;

	case funcdef: /* A function definition */
		compile_funcdef(c, n);
		break;
	
	case classdef: /* A class definition */
		compile_classdef(c, n);
		break;
	
	default:
		fprintf(stderr, "node type %d\n", TYPE(n));
		err_setstr(SystemError, "compile_node: unexpected node type");
		c->c_errors++;
	}
}

/* Optimization for local variables in functions (and *only* functions).

   This replaces all LOAD_NAME, STORE_NAME and DELETE_NAME
   instructions that refer to local variables with LOAD_FAST etc.
   The latter instructions are much faster because they don't need to
   look up the variable name in a dictionary.

   To find all local variables, we check all STORE_NAME, IMPORT_FROM and
   DELETE_NAME instructions.  This yields all local variables, including
   arguments, function definitions, class definitions and import
   statements.

   All remaining LOAD_NAME instructions must refer to non-local (global
   or builtin) variables, so are replaced by LOAD_GLOBAL.

   There are two problems:  'from foo import *' and 'exec' may introduce
   local variables that we can't know while compiling.  If this is the
   case, we don't optimize at all (this rarely happens, since exec is
   rare, & this form of import statement is mostly used at the module
   level).

   NB: this modifies the string object co->co_code!
*/

static void
optimize(c)
	struct compiling *c;
{
	unsigned char *next_instr, *cur_instr;
	object *locals;
	int nlocals;
	int opcode;
	int oparg;
	object *name;
	int fast_reserved;
	object *error_type, *error_value, *error_traceback;
	
#define NEXTOP()	(*next_instr++)
#define NEXTARG()	(next_instr += 2, (next_instr[-1]<<8) + next_instr[-2])
#define GETITEM(v, i)	(getlistitem((v), (i)))
#define GETNAMEOBJ(i)	(GETITEM(c->c_names, (i)))
	
	locals = newdictobject();
	if (locals == NULL) {
		c->c_errors++;
		return;
	}
	nlocals = 0;

	err_fetch(&error_type, &error_value, &error_traceback);
	
	next_instr = (unsigned char *) getstringvalue(c->c_code);
	for (;;) {
		opcode = NEXTOP();
		if (opcode == STOP_CODE)
			break;
		if (opcode == EXEC_STMT)
			goto end; /* Don't optimize if exec present */
		if (HAS_ARG(opcode))
			oparg = NEXTARG();
		if (opcode == STORE_NAME || opcode == DELETE_NAME ||
		    opcode == IMPORT_FROM) {
			object *v;
			name = GETNAMEOBJ(oparg);
			if (dict2lookup(locals, name) != NULL)
				continue;
			err_clear();
			v = newintobject(nlocals);
			if (v == NULL) {
				c->c_errors++;
				goto err;
			}
			nlocals++;
			if (dict2insert(locals, name, v) != 0) {
				DECREF(v);
				c->c_errors++;
				goto err;
			}
			DECREF(v);
		}
	}
	
	if (dictlookup(locals, "*") != NULL) {
		/* Don't optimize anything */
		goto end;
	}
	
	next_instr = (unsigned char *) getstringvalue(c->c_code);
	fast_reserved = 0;
	for (;;) {
		cur_instr = next_instr;
		opcode = NEXTOP();
		if (opcode == STOP_CODE)
			break;
		if (HAS_ARG(opcode))
			oparg = NEXTARG();
		if (opcode == RESERVE_FAST) {
			int i;
			object *localmap = newtupleobject(nlocals);
			int pos;
			object *key, *value;
			if (localmap == NULL) { /* XXX mask error */
				err_clear();
				continue;
			}
			pos = 0;
			while (mappinggetnext(locals, &pos, &key, &value)) {
				int j;
				if (!is_intobject(value))
					continue;
				j = getintvalue(value);
				if (0 <= j && j < nlocals) {
					INCREF(key);
					settupleitem(localmap, j, key);
				}
			}
			i = com_addconst(c, localmap);
			cur_instr[1] = i & 0xff;
			cur_instr[2] = (i>>8) & 0xff;
			fast_reserved = 1;
			DECREF(localmap);
			continue;
		}
		if (!fast_reserved)
			continue;
		if (opcode == LOAD_NAME ||
		    opcode == STORE_NAME ||
		    opcode == DELETE_NAME) {
			object *v;
			int i;
			name = GETNAMEOBJ(oparg);
			v = dict2lookup(locals, name);
			if (v == NULL) {
				err_clear();
				if (opcode == LOAD_NAME)
					cur_instr[0] = LOAD_GLOBAL;
				continue;
			}
			i = getintvalue(v);
			switch (opcode) {
			case LOAD_NAME: cur_instr[0] = LOAD_FAST; break;
			case STORE_NAME: cur_instr[0] = STORE_FAST; break;
			case DELETE_NAME: cur_instr[0] = DELETE_FAST; break;
			}
			cur_instr[1] = i & 0xff;
			cur_instr[2] = (i>>8) & 0xff;
		}
	}

 end:
	err_restore(error_type, error_value, error_traceback);
 err:
	DECREF(locals);
}

codeobject *
compile(n, filename)
	node *n;
	char *filename;
{
	struct compiling sc;
	codeobject *co;
	if (!com_init(&sc, filename))
		return NULL;
	compile_node(&sc, n);
	com_done(&sc);
	if ((TYPE(n) == funcdef || TYPE(n) == lambdef) && sc.c_errors == 0)
		optimize(&sc);
	co = NULL;
	if (sc.c_errors == 0) {
		object *v, *w;
		v = newstringobject(sc.c_filename);
		w = newstringobject(sc.c_name);
		if (v != NULL && w != NULL)
			co = newcodeobject(sc.c_code, sc.c_consts,
					   sc.c_names, v, w);
		XDECREF(v);
		XDECREF(w);
	}
	com_free(&sc);
	return co;
}
