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

/* File object implementation */

#include "allobjects.h"
#include "modsupport.h"
#include "ceval.h"

#define BUF(v) GETSTRINGVALUE((stringobject *)v)

#include <errno.h>

typedef struct {
	OB_HEAD
	FILE *f_fp;
	object *f_name;
	object *f_mode;
	int (*f_close) PROTO((FILE *));
	int f_softspace; /* Flag used by 'print' command */
} fileobject;

FILE *
getfilefile(f)
	object *f;
{
	if (f == NULL || !is_fileobject(f))
		return NULL;
	else
		return ((fileobject *)f)->f_fp;
}

object *
getfilename(f)
	object *f;
{
	if (f == NULL || !is_fileobject(f))
		return NULL;
	else
		return ((fileobject *)f)->f_name;
}

object *
newopenfileobject(fp, name, mode, close)
	FILE *fp;
	char *name;
	char *mode;
	int (*close) FPROTO((FILE *));
{
	fileobject *f = NEWOBJ(fileobject, &Filetype);
	if (f == NULL)
		return NULL;
	f->f_fp = NULL;
	f->f_name = newstringobject(name);
	f->f_mode = newstringobject(mode);
	f->f_close = close;
	f->f_softspace = 0;
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
	extern int fclose PROTO((FILE *));
	fileobject *f;
	f = (fileobject *) newopenfileobject((FILE *)NULL, name, mode, fclose);
	if (f == NULL)
		return NULL;
#ifdef USE_FOPENRF
	if (*mode == '*') {
		FILE *fopenRF();
		f->f_fp = fopenRF(name, mode+1);
	}
	else
#endif
	{
		BGN_SAVE
		f->f_fp = fopen(name, mode);
		END_SAVE
	}
	if (f->f_fp == NULL) {
		err_errno(IOError);
		DECREF(f);
		return NULL;
	}
	return (object *)f;
}

static object *
err_closed()
{
	err_setstr(ValueError, "I/O operation on closed file");
	return NULL;
}

/* Methods */

static void
file_dealloc(f)
	fileobject *f;
{
	if (f->f_fp != NULL && f->f_close != NULL) {
		BGN_SAVE
		(*f->f_close)(f->f_fp);
		END_SAVE
	}
	if (f->f_name != NULL)
		DECREF(f->f_name);
	if (f->f_mode != NULL)
		DECREF(f->f_mode);
	free((char *)f);
}

static object *
file_repr(f)
	fileobject *f;
{
	char buf[300];
	sprintf(buf, "<%s file '%.256s', mode '%.10s' at %lx>",
		f->f_fp == NULL ? "closed" : "open",
		getstringvalue(f->f_name),
		getstringvalue(f->f_mode),
		(long)f);
	return newstringobject(buf);
}

static object *
file_close(f, args)
	fileobject *f;
	object *args;
{
	int sts = 0;
	if (!getnoarg(args))
		return NULL;
	if (f->f_fp != NULL) {
		if (f->f_close != NULL) {
			BGN_SAVE
			errno = 0;
			sts = (*f->f_close)(f->f_fp);
			END_SAVE
		}
		f->f_fp = NULL;
	}
	if (sts == EOF)
		return err_errno(IOError);
	if (sts != 0)
		return newintobject((long)sts);
	INCREF(None);
	return None;
}

static object *
file_seek(f, args)
	fileobject *f;
	object *args;
{
	long offset;
	int whence;
	int ret;
	
	if (f->f_fp == NULL)
		return err_closed();
	whence = 0;
	if (!getargs(args, "l", &offset)) {
		err_clear();
		if (!getargs(args, "(li)", &offset, &whence))
			return NULL;
	}
	BGN_SAVE
	errno = 0;
	ret = fseek(f->f_fp, offset, whence);
	END_SAVE
	if (ret != 0) {
		err_errno(IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	INCREF(None);
	return None;
}

static object *
file_tell(f, args)
	fileobject *f;
	object *args;
{
	long offset;
	if (f->f_fp == NULL)
		return err_closed();
	if (!getnoarg(args))
		return NULL;
	BGN_SAVE
	errno = 0;
	offset = ftell(f->f_fp);
	END_SAVE
	if (offset == -1L) {
		err_errno(IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	return newintobject(offset);
}

static object *
file_fileno(f, args)
	fileobject *f;
	object *args;
{
	if (f->f_fp == NULL)
		return err_closed();
	if (!getnoarg(args))
		return NULL;
	return newintobject((long) fileno(f->f_fp));
}

static object *
file_flush(f, args)
	fileobject *f;
	object *args;
{
	int res;
	
	if (f->f_fp == NULL)
		return err_closed();
	if (!getnoarg(args))
		return NULL;
	BGN_SAVE
	errno = 0;
	res = fflush(f->f_fp);
	END_SAVE
	if (res != 0) {
		err_errno(IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	INCREF(None);
	return None;
}

static object *
file_isatty(f, args)
	fileobject *f;
	object *args;
{
	long res;
	if (f->f_fp == NULL)
		return err_closed();
	if (!getnoarg(args))
		return NULL;
	BGN_SAVE
	res = isatty((int)fileno(f->f_fp));
	END_SAVE
	return newintobject(res);
}

static object *
file_read(f, args)
	fileobject *f;
	object *args;
{
	int n, n1, n2, n3;
	object *v;
	
	if (f->f_fp == NULL)
		return err_closed();
	if (args == NULL) {
		n = 0;
		if (n < 0) {
			err_setstr(ValueError, "negative read count");
			return NULL;
		}
	}
	else if (!getargs(args, "i", &n))
		return NULL;
	
	n2 = n != 0 ? n : BUFSIZ;
	v = newsizedstringobject((char *)NULL, n2);
	if (v == NULL)
		return NULL;
	n1 = 0;
	BGN_SAVE
	for (;;) {
		n3 = fread(BUF(v)+n1, 1, n2-n1, f->f_fp);
		/* XXX Error check? */
		if (n3 == 0)
			break;
		n1 += n3;
		if (n1 == n)
			break;
		if (n == 0) {
			n2 = n1 + BUFSIZ;
			RET_SAVE
			if (resizestring(&v, n2) < 0)
				return NULL;
			RES_SAVE
		}
	}
	END_SAVE
	if (n1 != n2)
		resizestring(&v, n1);
	return v;
}

/* Internal routine to get a line.
   Size argument interpretation:
   > 0: max length;
   = 0: read arbitrary line;
   < 0: strip trailing '\n', raise EOFError if EOF reached immediately
*/

static object *
getline(f, n)
	fileobject *f;
	int n;
{
	register FILE *fp;
	register int c;
	register char *buf, *end;
	int n1, n2;
	object *v;

	fp = f->f_fp;
	n2 = n > 0 ? n : 100;
	v = newsizedstringobject((char *)NULL, n2);
	if (v == NULL)
		return NULL;
	buf = BUF(v);
	end = buf + n2;

	BGN_SAVE
	for (;;) {
		if ((c = getc(fp)) == EOF) {
			clearerr(fp);
			if (intrcheck()) {
				RET_SAVE
				DECREF(v);
				err_set(KeyboardInterrupt);
				return NULL;
			}
			if (n < 0 && buf == BUF(v)) {
				RET_SAVE
				DECREF(v);
				err_setstr(EOFError,
					   "EOF when reading a line");
				return NULL;
			}
			break;
		}
		if ((*buf++ = c) == '\n') {
			if (n < 0)
				buf--;
			break;
		}
		if (buf == end) {
			if (n > 0)
				break;
			n1 = n2;
			n2 += 1000;
			RET_SAVE
			if (resizestring(&v, n2) < 0)
				return NULL;
			RES_SAVE
			buf = BUF(v) + n1;
			end = BUF(v) + n2;
		}
	}
	END_SAVE

	n1 = buf - BUF(v);
	if (n1 != n2)
		resizestring(&v, n1);
	return v;
}

/* External C interface */

object *
filegetline(f, n)
	object *f;
	int n;
{
	if (f == NULL) {
		err_badcall();
		return NULL;
	}
	if (!is_fileobject(f)) {
		object *reader;
		object *args;
		object *result;
		reader = getattr(f, "readline");
		if (reader == NULL)
			return NULL;
		if (n <= 0)
			args = mkvalue("()");
		else
			args = mkvalue("(i)", n);
		if (args == NULL) {
			DECREF(reader);
			return NULL;
		}
		result = call_object(reader, args);
		DECREF(reader);
		DECREF(args);
		if (result != NULL && !is_stringobject(result)) {
			DECREF(result);
			result = NULL;
			err_setstr(TypeError,
				   "object.readline() returned non-string");
		}
		if (n < 0 && result != NULL) {
			char *s = getstringvalue(result);
			int len = getstringsize(result);
			if (len == 0) {
				DECREF(result);
				result = NULL;
				err_setstr(EOFError,
					   "EOF when reading a line");
			}
			else if (s[len-1] == '\n') {
				if (result->ob_refcnt == 1)
					resizestring(&result, len-1);
				else {
					object *v;
					v = newsizedstringobject(s, len-1);
					DECREF(result);
					result = v;
				}
			}
		}
		return result;
	}
	if (((fileobject*)f)->f_fp == NULL)
		return err_closed();
	return getline((fileobject *)f, n);
}

/* Python method */

static object *
file_readline(f, args)
	fileobject *f;
	object *args;
{
	int n;

	if (f->f_fp == NULL)
		return err_closed();
	if (args == NULL)
		n = 0; /* Unlimited */
	else {
		if (!getintarg(args, &n))
			return NULL;
		if (n < 0) {
			err_setstr(ValueError, "negative readline count");
			return NULL;
		}
	}

	return getline(f, n);
}

static object *
file_readlines(f, args)
	fileobject *f;
	object *args;
{
	object *list;
	object *line;

	if (f->f_fp == NULL)
		return err_closed();
	if (!getnoarg(args))
		return NULL;
	if ((list = newlistobject(0)) == NULL)
		return NULL;
	for (;;) {
		line = getline(f, 0);
		if (line != NULL && getstringsize(line) == 0) {
			DECREF(line);
			break;
		}
		if (line == NULL || addlistitem(list, line) != 0) {
			DECREF(list);
			XDECREF(line);
			return NULL;
		}
		DECREF(line);
	}
	return list;
}

static object *
file_write(f, args)
	fileobject *f;
	object *args;
{
	char *s;
	int n, n2;
	if (f->f_fp == NULL)
		return err_closed();
	if (!getargs(args, "s#", &s, &n))
		return NULL;
	f->f_softspace = 0;
	BGN_SAVE
	errno = 0;
	n2 = fwrite(s, 1, n, f->f_fp);
	END_SAVE
	if (n2 != n) {
		err_errno(IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	INCREF(None);
	return None;
}

static object *
file_writelines(f, args)
	fileobject *f;
	object *args;
{
	int i, n;
	if (f->f_fp == NULL)
		return err_closed();
	if (args == NULL || !is_listobject(args)) {
		err_setstr(TypeError,
			   "writelines() requires list of strings");
		return NULL;
	}
	n = getlistsize(args);
	f->f_softspace = 0;
	BGN_SAVE
	errno = 0;
	for (i = 0; i < n; i++) {
		object *line = getlistitem(args, i);
		int len;
		int nwritten;
		if (!is_stringobject(line)) {
			RET_SAVE
			err_setstr(TypeError,
				   "writelines() requires list of strings");
			return NULL;
		}
		len = getstringsize(line);
		nwritten = fwrite(getstringvalue(line), 1, len, f->f_fp);
		if (nwritten != len) {
			RET_SAVE
			err_errno(IOError);
			clearerr(f->f_fp);
			return NULL;
		}
	}
	END_SAVE
	INCREF(None);
	return None;
}

static struct methodlist file_methods[] = {
	{"close",	file_close},
	{"flush",	file_flush},
	{"fileno",	file_fileno},
	{"isatty",	file_isatty},
	{"read",	file_read},
	{"readline",	file_readline},
	{"readlines",	file_readlines},
	{"seek",	file_seek},
	{"tell",	file_tell},
	{"write",	file_write},
	{"writelines",	file_writelines},
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
	0,		/*tp_print*/
	file_getattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	file_repr,	/*tp_repr*/
};

/* Interface for the 'soft space' between print items. */

int
softspace(f, newflag)
	object *f;
	int newflag;
{
	int oldflag = 0;
	if (f == NULL) {
		/* Do nothing */
	}
	else if (is_fileobject(f)) {
		oldflag = ((fileobject *)f)->f_softspace;
		((fileobject *)f)->f_softspace = newflag;
	}
	else {
		object *v;
		v = getattr(f, "softspace");
		if (v == NULL)
			err_clear();
		else {
			if (is_intobject(v))
				oldflag = getintvalue(v);
			DECREF(v);
		}
		v = newintobject((long)newflag);
		if (v == NULL)
			err_clear();
		else {
			if (setattr(f, "softspace", v) != 0)
				err_clear();
			DECREF(v);
		}
	}
	return oldflag;
}

/* Interfaces to write objects/strings to file-like objects */

int
writeobject(v, f, flags)
	object *v;
	object *f;
	int flags;
{
	object *writer, *value, *result;
	if (f == NULL) {
		err_setstr(TypeError, "writeobject with NULL file");
		return -1;
	}
	else if (is_fileobject(f)) {
		FILE *fp = getfilefile(f);
		if (fp == NULL) {
			err_closed();
			return -1;
		}
		return printobject(v, fp, flags);
	}
	writer = getattr(f, "write");
	if (writer == NULL)
		return -1;
	if (flags & PRINT_RAW)
		value = strobject(v);
	else
		value = reprobject(v);
	if (value == NULL) {
		DECREF(writer);
		return -1;
	}
	result = call_object(writer, value);
	DECREF(writer);
	DECREF(value);
	if (result == NULL)
		return -1;
	DECREF(result);
	return 0;
}

void
writestring(s, f)
	char *s;
	object *f;
{
	if (f == NULL) {
		/* Do nothing */
	}
	else if (is_fileobject(f)) {
		FILE *fp = getfilefile(f);
		if (fp != NULL)
			fputs(s, fp);
	}
	else {
		object *v = newstringobject(s);
		if (v == NULL) {
			err_clear();
		}
		else {
			if (writeobject(v, f, PRINT_RAW) != 0)
				err_clear();
			DECREF(v);
		}
	}
}
