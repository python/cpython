/* Module definition and import implementation */

#include <stdio.h>
#include "string.h"

#include "PROTO.h"
#include "object.h"
#include "stringobject.h"
#include "listobject.h"
#include "dictobject.h"
#include "moduleobject.h"
#include "node.h"
#include "context.h"
#include "token.h"
#include "graminit.h"
#include "run.h"
#include "support.h"
#include "import.h"
#include "errcode.h"
#include "sysmodule.h"

/* Define pathname separator and delimiter in $PYTHONPATH */

#ifdef THINK_C
#define SEP ':'
#define DELIM ' '
#endif

#ifndef SEP
#define SEP '/'
#endif

#ifndef DELIM
#define DELIM ':'
#endif

void
initimport()
{
	object *v;
	if ((v = newdictobject()) == NULL)
		fatal("no mem for module table");
	if (sysset("modules", v) != 0)
		fatal("can't assign sys.modules");
}

object *
new_module(name)
	char *name;
{
	object *m;
	object *mtab;
	mtab = sysget("modules");
	if (mtab == NULL || !is_dictobject(mtab)) {
		errno = EBADF;
		return NULL;
	}
	m = newmoduleobject(name);
	if (m == NULL)
		return NULL;
	if (dictinsert(mtab, name, m) != 0) {
		DECREF(m);
		return NULL;
	}
	return m;
}

void
define_module(ctx, name)
	context *ctx;
	char *name;
{
	object *m, *d;
	m = new_module(name);
	if (m == NULL) {
		puterrno(ctx);
		return;
	}
	d = getmoduledict(m);
	INCREF(d);
	DECREF(ctx->ctx_locals);
	ctx->ctx_locals = d;
	INCREF(d);
	DECREF(ctx->ctx_globals);
	ctx->ctx_globals = d;
	DECREF(m);
}

object *
parsepath(path, delim)
	char *path;
	int delim;
{
	int i, n;
	char *p;
	object *v, *w;
	
	n = 1;
	p = path;
	while ((p = strchr(p, delim)) != NULL) {
		n++;
		p++;
	}
	v = newlistobject(n);
	if (v == NULL)
		return NULL;
	for (i = 0; ; i++) {
		p = strchr(path, delim);
		if (p == NULL)
			p = strchr(path, '\0'); /* End of string */
		w = newsizedstringobject(path, (int) (p - path));
		if (w == NULL) {
			DECREF(v);
			return NULL;
		}
		setlistitem(v, i, w);
		if (*p == '\0')
			break;
		path = p+1;
	}
	return v;
}

void
setpythonpath(path)
	char *path;
{
	object *v;
	if ((v = parsepath(path, DELIM)) != NULL) {
		if (sysset("path", v) != 0)
			fatal("can't assign sys.path");
		DECREF(v);
	}
}

static FILE *
open_module(name, suffix)
	char *name;
	char *suffix;
{
	object *path;
	char namebuf[256];
	FILE *fp;
	
	path = sysget("path");
	if (path == NULL || !is_listobject(path)) {
		strcpy(namebuf, name);
		strcat(namebuf, suffix);
		fp = fopen(namebuf, "r");
	}
	else {
		int npath = getlistsize(path);
		int i;
		fp = NULL;
		for (i = 0; i < npath; i++) {
			object *v = getlistitem(path, i);
			int len;
			if (!is_stringobject(v))
				continue;
			strcpy(namebuf, getstringvalue(v));
			len = getstringsize(v);
			if (len > 0 && namebuf[len-1] != SEP)
				namebuf[len++] = SEP;
			strcpy(namebuf+len, name); /* XXX check for overflow */
			strcat(namebuf, suffix); /* XXX ditto */
			fp = fopen(namebuf, "r");
			if (fp != NULL)
				break;
		}
	}
	return fp;
}

static object *
load_module(ctx, name)
	context *ctx;
	char *name;
{
	object *m;
	char **p;
	FILE *fp;
	node *n, *mh;
	int err;
	object *mtab;
	object *save_locals, *save_globals;
	
	mtab = sysget("modules");
	if (mtab == NULL || !is_dictobject(mtab)) {
		errno = EBADF;
		return NULL;
	}
	fp = open_module(name, ".py");
	if (fp == NULL) {
		/* XXX Compatibility hack */
		fprintf(stderr,
			"Can't find '%s.py' in sys.path, trying '%s'\n",
			name, name);
		fp = open_module(name, "");
	}
	if (fp == NULL) {
		name_error(ctx, name);
		return NULL;
	}
#ifdef DEBUG
	fprintf(stderr, "Reading module %s from file '%s'\n", name, namebuf);
#endif
	err = parseinput(fp, file_input, &n);
	fclose(fp);
	if (err != E_DONE) {
		input_error(ctx, err);
		return NULL;
	}
	save_locals = ctx->ctx_locals;
	INCREF(save_locals);
	save_globals = ctx->ctx_globals;
	INCREF(save_globals);
	define_module(ctx, name);
	exec_node(ctx, n);
	DECREF(ctx->ctx_locals);
	ctx->ctx_locals = save_locals;
	DECREF(ctx->ctx_globals);
	ctx->ctx_globals = save_globals;
	/* XXX need to free the tree n here; except referenced defs */
	if (ctx->ctx_exception) {
		dictremove(mtab, name); /* Undefine the module */
		return NULL;
	}
	m = dictlookup(mtab, name);
	if (m == NULL) {
		error(ctx, "module not defined after loading");
		return NULL;
	}
	return m;
}

object *
import_module(ctx, name)
	context *ctx;
	char *name;
{
	object *m;
	object *mtab;
	mtab = sysget("modules");
	if (mtab == NULL || !is_dictobject(mtab)) {
		error(ctx, "bad sys.modules");
		return NULL;
	}
	if ((m = dictlookup(mtab, name)) == NULL) {
		m = load_module(ctx, name);
	}
	return m;
}
