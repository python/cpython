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

/* fcntl module */

#include "allobjects.h"
#include "modsupport.h"

#include <fcntl.h>


/* fcntl(fd, opt, [arg]) */

static object *
fcntl_fcntl(self, args)
	object *self; /* Not used */
	object *args;
{
	int fd;
	int code;
	int arg;
	int ret;
	char *str;
	int len;
	char buf[1024];

	if (getargs(args, "(iis#)", &fd, &code, &str, &len)) {
		if (len > sizeof buf) {
			err_setstr(ValueError, "fcntl string arg too long");
			return NULL;
		}
		memcpy(buf, str, len);
		BGN_SAVE
		ret = fcntl(fd, code, buf);
		END_SAVE
		if (ret < 0) {
			err_errno(IOError);
			return NULL;
		}
		return newsizedstringobject(buf, len);
	}

	err_clear();
	if (getargs(args, "(ii)", &fd, &code))
		arg = 0;
	else {
		err_clear();
		if (!getargs(args, "(iii)", &fd, &code, &arg))
			return NULL;
	}
	BGN_SAVE
	ret = fcntl(fd, code, arg);
	END_SAVE
	if (ret < 0) {
		err_errno(IOError);
		return NULL;
	}
	return newintobject((long)ret);
}


/* ioctl(fd, opt, [arg]) */

static object *
fcntl_ioctl(self, args)
	object *self; /* Not used */
	object *args;
{
	int fd;
	int code;
	int arg;
	int ret;
	char *str;
	int len;
	char buf[1024];

	if (getargs(args, "(iis#)", &fd, &code, &str, &len)) {
		if (len > sizeof buf) {
			err_setstr(ValueError, "ioctl string arg too long");
			return NULL;
		}
		memcpy(buf, str, len);
		BGN_SAVE
		ret = ioctl(fd, code, buf);
		END_SAVE
		if (ret < 0) {
			err_errno(IOError);
			return NULL;
		}
		return newsizedstringobject(buf, len);
	}

	err_clear();
	if (getargs(args, "(ii)", &fd, &code))
		arg = 0;
	else {
		err_clear();
		if (!getargs(args, "(iii)", &fd, &code, &arg))
			return NULL;
	}
	BGN_SAVE
	ret = ioctl(fd, code, arg);
	END_SAVE
	if (ret < 0) {
		err_errno(IOError);
		return NULL;
	}
	return newintobject((long)ret);
}


/* flock(fd, operation) */

static object *
fcntl_flock(self, args)
	object *self; /* Not used */
	object *args;
{
	int fd;
	int code;
	int ret;
	FILE *f;

	if (!getargs(args, "(ii)", &fd, &code))
		return NULL;

	BGN_SAVE
#ifdef HAVE_FLOCK
	ret = flock(fd, code);
#else

#ifndef LOCK_SH
#define LOCK_SH		1	/* shared lock */
#define LOCK_EX		2	/* exclusive lock */
#define LOCK_NB		4	/* don't block when locking */
#define LOCK_UN		8	/* unlock */
#endif
	{
		struct flock l;
		if (code == LOCK_UN)
			l.l_type = F_UNLCK;
		else if (code & LOCK_SH)
			l.l_type = F_RDLCK;
		else if (code & LOCK_EX)
			l.l_type = F_WRLCK;
		else {
			err_setstr(ValueError, "unrecognized flock argument");
			return NULL;
		}
		l.l_whence = l.l_start = l.l_len = 0;
		ret = fcntl(fd, (code & LOCK_NB) ? F_SETLK : F_SETLKW, &l);
	}
#endif /* HAVE_FLOCK */
	END_SAVE
	if (ret < 0) {
		err_errno(IOError);
		return NULL;
	}
	INCREF(None);
	return None;
}



/* List of functions */

static struct methodlist fcntl_methods[] = {
	{"fcntl",	fcntl_fcntl},
	{"ioctl",	fcntl_ioctl},
	{"flock",	fcntl_flock},
	{NULL,		NULL}		/* sentinel */
};


/* Module initialisation */

void
initfcntl()
{
	object *m, *d;

	/* Create the module and add the functions */
	m = initmodule("fcntl", fcntl_methods);

	/* Add some symbolic constants to the module */
	d = getmoduledict(m);

	/* Check for errors */
	if (err_occurred())
		fatal("can't initialize module fcntl");
}
