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

/* File object implementation */

/* XXX This should become a built-in module 'io'.  It should support more
   functionality, better exception handling for invalid calls, etc.
   (Especially reading on a write-only file or vice versa!)
   It should also cooperate with posix to support popen(), which should
   share most code but have a special close function. */

#include "allobjects.h"

#include "errno.h"
#ifndef errno
extern int errno;
#endif

typedef struct {
	OB_HEAD
	FILE *f_fp;
	object *f_name;
	object *f_mode;
	/* XXX Should move the 'need space' on printing flag here */
} fileobject;

FILE *
getfilefile(f)
	object *f;
{
	if (!is_fileobject(f)) {
		err_badcall();
		return NULL;
	}
	return ((fileobject *)f)->f_fp;
}

object *
newopenfileobject(fp, name, mode)
	FILE *fp;
	char *name;
	char *mode;
{
	fileobject *f = NEWOBJ(fileobject, &Filetype);
	if (f == NULL)
		return NULL;
	f->f_fp = NULL;
	f->f_name = newstringobject(name);
	f->f_mode = newstringobject(mode);
	if (f->f_name == NULL || f->f_mode == NULL) {
		DECREF(f);
		return NULL;
	}
	f->f_fp = fp;
	return (object *) f;
}

object *
newfileobject(name, mode)
	char *name, *mode;
{
	fileobject *f;
	FILE *fp;
	f = (fileobject *) newopenfileobject((FILE *)NULL, name, mode);
	if (f == NULL)
		return NULL;
#ifdef THINK_C
	if (*mode == '*') {
		FILE *fopenRF();
		f->f_fp = fopenRF(name, mode+1);
	}
	else
#endif
	f->f_fp = fopen(name, mode);
	if (f->f_fp == NULL) {
		err_errno(RuntimeError);
		DECREF(f);
		return NULL;
	}
	return (object *)f;
}

/* Methods */

static void
file_dealloc(f)
	fileobject *f;
{
	if (f->f_fp != NULL)
		fclose(f->f_fp);
	if (f->f_name != NULL)
		DECREF(f->f_name);
	if (f->f_mode != NULL)
		DECREF(f->f_mode);
	free((char *)f);
}

static void
file_print(f, fp, flags)
	fileobject *f;
	FILE *fp;
	int flags;
{
	fprintf(fp, "<%s file ", f->f_fp == NULL ? "closed" : "open");
	printobject(f->f_name, fp, flags);
	fprintf(fp, ", mode ");
	printobject(f->f_mode, fp, flags);
	fprintf(fp, ">");
}

static object *
file_repr(f)
	fileobject *f;
{
	char buf[300];
	/* XXX This differs from file_print if the filename contains
	   quotes or other funny characters. */
	sprintf(buf, "<%s file '%.256s', mode '%.10s'>",
		f->f_fp == NULL ? "closed" : "open",
		getstringvalue(f->f_name),
		getstringvalue(f->f_mode));
	return newstringobject(buf);
}

static object *
file_close(f, args)
	fileobject *f;
	object *args;
{
	if (args != NULL) {
		err_badarg();
		return NULL;
	}
	if (f->f_fp != NULL) {
		fclose(f->f_fp);
		f->f_fp = NULL;
	}
	INCREF(None);
	return None;
}

static object *
file_read(f, args)
	fileobject *f;
	object *args;
{
	int n;
	object *v;
	if (f->f_fp == NULL) {
		err_badarg();
		return NULL;
	}
	if (args == NULL || !is_intobject(args)) {
		err_badarg();
		return NULL;
	}
	n = getintvalue(args);
	if (n < 0) {
		err_badarg();
		return NULL;
	}
	v = newsizedstringobject((char *)NULL, n);
	if (v == NULL)
		return NULL;
	n = fread(getstringvalue(v), 1, n, f->f_fp);
	/* EOF is reported as an empty string */
	/* XXX should detect real I/O errors? */
	resizestring(&v, n);
	return v;
}

/* XXX Should this be unified with raw_input()? */

static object *
file_readline(f, args)
	fileobject *f;
	object *args;
{
	int n;
	object *v;
	if (f->f_fp == NULL) {
		err_badarg();
		return NULL;
	}
	if (args == NULL) {
		n = 10000; /* XXX should really be unlimited */
	}
	else if (is_intobject(args)) {
		n = getintvalue(args);
		if (n < 0) {
			err_badarg();
			return NULL;
		}
	}
	else {
		err_badarg();
		return NULL;
	}
	v = newsizedstringobject((char *)NULL, n);
	if (v == NULL)
		return NULL;
#ifndef THINK_C_3_0
	/* XXX Think C 3.0 wrongly reads up to n characters... */
	n = n+1;
#endif
	if (fgets(getstringvalue(v), n, f->f_fp) == NULL) {
		/* EOF is reported as an empty string */
		/* XXX should detect real I/O errors? */
		n = 0;
	}
	else {
		n = strlen(getstringvalue(v));
	}
	resizestring(&v, n);
	return v;
}

static object *
file_write(f, args)
	fileobject *f;
	object *args;
{
	int n, n2;
	if (f->f_fp == NULL) {
		err_badarg();
		return NULL;
	}
	if (args == NULL || !is_stringobject(args)) {
		err_badarg();
		return NULL;
	}
	errno = 0;
	n2 = fwrite(getstringvalue(args), 1, n = getstringsize(args), f->f_fp);
	if (n2 != n) {
		if (errno == 0)
			errno = EIO;
		err_errno(RuntimeError);
		return NULL;
	}
	INCREF(None);
	return None;
}

static struct methodlist file_methods[] = {
	{"write",	file_write},
	{"read",	file_read},
	{"readline",	file_readline},
	{"close",	file_close},
	{NULL,		NULL}		/* sentinel */
};

static object *
file_getattr(f, name)
	fileobject *f;
	char *name;
{
	return findmethod(file_methods, (object *)f, name);
}

typeobject Filetype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"file",
	sizeof(fileobject),
	0,
	file_dealloc,	/*tp_dealloc*/
	file_print,	/*tp_print*/
	file_getattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	file_repr,	/*tp_repr*/
};
