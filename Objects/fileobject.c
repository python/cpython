/* File object implementation */

#include <stdio.h>

#include "PROTO.h"
#include "object.h"
#include "stringobject.h"
#include "intobject.h"
#include "fileobject.h"
#include "methodobject.h"
#include "objimpl.h"

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
		errno = EBADF;
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
		errno = ENOMEM;
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
	if ((f->f_fp = fopen(name, mode)) == NULL) {
		DECREF(f);
		return NULL;
	}
	return (object *)f;
}

/* Methods */

static void
filedealloc(f)
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
fileprint(f, fp, flags)
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
filerepr(f)
	fileobject *f;
{
	char buf[300];
	/* XXX This differs from fileprint if the filename contains
	   quotes or other funny characters. */
	sprintf(buf, "<%s file '%.256s', mode '%.10s'>",
		f->f_fp == NULL ? "closed" : "open",
		getstringvalue(f->f_name),
		getstringvalue(f->f_mode));
	return newstringobject(buf);
}

static object *
fileclose(f, args)
	fileobject *f;
	object *args;
{
	if (args != NULL) {
		errno = EINVAL;
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
fileread(f, args)
	fileobject *f;
	object *args;
{
	int n;
	object *v;
	if (f->f_fp == NULL) {
		errno = EBADF;
		return NULL;
	}
	if (args == NULL || !is_intobject(args)) {
		errno = EINVAL;
		return NULL;
	}
	n = getintvalue(args);
	if (n <= 0 /* || n > 0x7fff /*XXX*/ ) {
		errno = EDOM;
		return NULL;
	}
	v = newsizedstringobject((char *)NULL, n);
	if (v == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	n = fread(getstringvalue(v), 1, n, f->f_fp);
	/* EOF is reported as an empty string */
	/* XXX should detect real I/O errors? */
	resizestring(&v, n);
	return v;
}

/* XXX Should this be unified with raw_input()? */

static object *
filereadline(f, args)
	fileobject *f;
	object *args;
{
	int n;
	object *v;
	if (f->f_fp == NULL) {
		errno = EBADF;
		return NULL;
	}
	if (args == NULL) {
		n = 10000; /* XXX should really be unlimited */
	}
	else if (is_intobject(args)) {
		n = getintvalue(args);
		if (n < 0 || n > 0x7fff /*XXX*/ ) {
			errno = EDOM;
			return NULL;
		}
	}
	else {
		errno = EINVAL;
		return NULL;
	}
	v = newsizedstringobject((char *)NULL, n);
	if (v == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	if (fgets(getstringvalue(v), n+1, f->f_fp) == NULL) {
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
filewrite(f, args)
	fileobject *f;
	object *args;
{
	int n, n2;
	if (f->f_fp == NULL) {
		errno = EBADF;
		return NULL;
	}
	if (args == NULL || !is_stringobject(args)) {
		errno = EINVAL;
		return NULL;
	}
	errno = 0;
	n2 = fwrite(getstringvalue(args), 1, n = getstringsize(args), f->f_fp);
	if (n2 != n) {
		if (errno == 0)
			errno = EIO;
		return NULL;
	}
	INCREF(None);
	return None;
}

static struct methodlist {
	char *ml_name;
	method ml_meth;
} filemethods[] = {
	{"write",	filewrite},
	{"read",	fileread},
	{"readline",	filereadline},
	{"close",	fileclose},
	{NULL,		NULL}		/* sentinel */
};

static object *
filegetattr(f, name)
	fileobject *f;
	char *name;
{
	struct methodlist *ml = filemethods;
	for (; ml->ml_name != NULL; ml++) {
		if (strcmp(name, ml->ml_name) == 0)
			return newmethodobject(ml->ml_name, ml->ml_meth,
				(object *)f);
	}
	errno = ESRCH;
	return NULL;
}

typeobject Filetype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"file",
	sizeof(fileobject),
	0,
	filedealloc,	/*tp_dealloc*/
	fileprint,	/*tp_print*/
	filegetattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	filerepr,	/*tp_repr*/
};
