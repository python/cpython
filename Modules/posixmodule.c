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

/* POSIX module implementation */

#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifdef SYSV
#include <dirent.h>
#define direct dirent
#else
#include <sys/dir.h>
#endif

#include "allobjects.h"
#include "modsupport.h"

extern char *strerror PROTO((int));

#ifdef AMOEBA
#define NO_LSTAT
#endif


/* Return a dictionary corresponding to the POSIX environment table */

extern char **environ;

static object *
convertenviron()
{
	object *d;
	char **e;
	d = newdictobject();
	if (d == NULL)
		return NULL;
	if (environ == NULL)
		return d;
	/* XXX This part ignores errors */
	for (e = environ; *e != NULL; e++) {
		object *v;
		char *p = strchr(*e, '=');
		if (p == NULL)
			continue;
		v = newstringobject(p+1);
		if (v == NULL)
			continue;
		*p = '\0';
		(void) dictinsert(d, *e, v);
		*p = '=';
		DECREF(v);
	}
	return d;
}


static object *PosixError; /* Exception posix.error */

/* Set a POSIX-specific error from errno, and return NULL */

static object *
posix_error()
{
	return err_errno(PosixError);
}


/* POSIX generic methods */

static object *
posix_1str(args, func)
	object *args;
	int (*func) FPROTO((const char *));
{
	object *path1;
	if (!getstrarg(args, &path1))
		return NULL;
	if ((*func)(getstringvalue(path1)) < 0)
		return posix_error();
	INCREF(None);
	return None;
}

static object *
posix_2str(args, func)
	object *args;
	int (*func) FPROTO((const char *, const char *));
{
	object *path1, *path2;
	if (!getstrstrarg(args, &path1, &path2))
		return NULL;
	if ((*func)(getstringvalue(path1), getstringvalue(path2)) < 0)
		return posix_error();
	INCREF(None);
	return None;
}

static object *
posix_strint(args, func)
	object *args;
	int (*func) FPROTO((const char *, int));
{
	object *path1;
	int i;
	if (!getstrintarg(args, &path1, &i))
		return NULL;
	if ((*func)(getstringvalue(path1), i) < 0)
		return posix_error();
	INCREF(None);
	return None;
}

static object *
posix_do_stat(self, args, statfunc)
	object *self;
	object *args;
	int (*statfunc) FPROTO((const char *, struct stat *));
{
	struct stat st;
	object *path;
	object *v;
	if (!getstrarg(args, &path))
		return NULL;
	if ((*statfunc)(getstringvalue(path), &st) != 0)
		return posix_error();
	v = newtupleobject(10);
	if (v == NULL)
		return NULL;
#define SET(i, st_member) settupleitem(v, i, newintobject((long)st.st_member))
	SET(0, st_mode);
	SET(1, st_ino);
	SET(2, st_dev);
	SET(3, st_nlink);
	SET(4, st_uid);
	SET(5, st_gid);
	SET(6, st_size);
	SET(7, st_atime);
	SET(8, st_mtime);
	SET(9, st_ctime);
#undef SET
	if (err_occurred()) {
		DECREF(v);
		return NULL;
	}
	return v;
}


/* POSIX methods */

static object *
posix_chdir(self, args)
	object *self;
	object *args;
{
	extern int chdir PROTO((const char *));
	return posix_1str(args, chdir);
}

static object *
posix_chmod(self, args)
	object *self;
	object *args;
{
	extern int chmod PROTO((const char *, mode_t));
	return posix_strint(args, chmod);
}

static object *
posix_getcwd(self, args)
	object *self;
	object *args;
{
	char buf[1026];
	extern char *getcwd PROTO((char *, int));
	if (!getnoarg(args))
		return NULL;
	if (getcwd(buf, sizeof buf) == NULL)
		return posix_error();
	return newstringobject(buf);
}

static object *
posix_link(self, args)
	object *self;
	object *args;
{
	extern int link PROTO((const char *, const char *));
	return posix_2str(args, link);
}

static object *
posix_listdir(self, args)
	object *self;
	object *args;
{
	object *name, *d, *v;
	DIR *dirp;
	struct direct *ep;
	if (!getstrarg(args, &name))
		return NULL;
	if ((dirp = opendir(getstringvalue(name))) == NULL)
		return posix_error();
	if ((d = newlistobject(0)) == NULL) {
		closedir(dirp);
		return NULL;
	}
	while ((ep = readdir(dirp)) != NULL) {
		v = newstringobject(ep->d_name);
		if (v == NULL) {
			DECREF(d);
			d = NULL;
			break;
		}
		if (addlistitem(d, v) != 0) {
			DECREF(v);
			DECREF(d);
			d = NULL;
			break;
		}
		DECREF(v);
	}
	closedir(dirp);
	return d;
}

static object *
posix_mkdir(self, args)
	object *self;
	object *args;
{
	extern int mkdir PROTO((const char *, mode_t));
	return posix_strint(args, mkdir);
}

static object *
posix_rename(self, args)
	object *self;
	object *args;
{
	extern int rename PROTO((const char *, const char *));
	return posix_2str(args, rename);
}

static object *
posix_rmdir(self, args)
	object *self;
	object *args;
{
	extern int rmdir PROTO((const char *));
	return posix_1str(args, rmdir);
}

static object *
posix_stat(self, args)
	object *self;
	object *args;
{
	extern int stat PROTO((const char *, struct stat *));
	return posix_do_stat(self, args, stat);
}

static object *
posix_system(self, args)
	object *self;
	object *args;
{
	object *command;
	int sts;
	if (!getstrarg(args, &command))
		return NULL;
	sts = system(getstringvalue(command));
	return newintobject((long)sts);
}

static object *
posix_umask(self, args)
	object *self;
	object *args;
{
	int i;
	if (!getintarg(args, &i))
		return NULL;
	i = umask(i);
	if (i < 0)
		return posix_error();
	return newintobject((long)i);
}

static object *
posix_unlink(self, args)
	object *self;
	object *args;
{
	extern int unlink PROTO((const char *));
	return posix_1str(args, unlink);
}

static object *
posix_utimes(self, args)
	object *self;
	object *args;
{
	object *path;
	struct timeval tv[2];
	if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 2) {
		err_badarg();
		return NULL;
	}
	if (!getstrarg(gettupleitem(args, 0), &path) ||
				!getlonglongargs(gettupleitem(args, 1),
					&tv[0].tv_sec, &tv[1].tv_sec))
		return NULL;
	tv[0].tv_usec = tv[1].tv_usec = 0;
	if (utimes(getstringvalue(path), tv) < 0)
		return posix_error();
	INCREF(None);
	return None;
}


#ifndef NO_LSTAT

static object *
posix_lstat(self, args)
	object *self;
	object *args;
{
	extern int lstat PROTO((const char *, struct stat *));
	return posix_do_stat(self, args, lstat);
}

static object *
posix_readlink(self, args)
	object *self;
	object *args;
{
	char buf[1024]; /* XXX Should use MAXPATHLEN */
	object *path;
	int n;
	if (!getstrarg(args, &path))
		return NULL;
	n = readlink(getstringvalue(path), buf, sizeof buf);
	if (n < 0)
		return posix_error();
	return newsizedstringobject(buf, n);
}

static object *
posix_symlink(self, args)
	object *self;
	object *args;
{
	extern int symlink PROTO((const char *, const char *));
	return posix_2str(args, symlink);
}

#endif /* NO_LSTAT */


static struct methodlist posix_methods[] = {
	{"chdir",	posix_chdir},
	{"chmod",	posix_chmod},
	{"getcwd",	posix_getcwd},
	{"link",	posix_link},
	{"listdir",	posix_listdir},
	{"mkdir",	posix_mkdir},
	{"rename",	posix_rename},
	{"rmdir",	posix_rmdir},
	{"stat",	posix_stat},
	{"system",	posix_system},
	{"umask",	posix_umask},
	{"unlink",	posix_unlink},
	{"utimes",	posix_utimes},
#ifndef NO_LSTAT
	{"lstat",	posix_lstat},
	{"readlink",	posix_readlink},
	{"symlink",	posix_symlink},
#endif
	{NULL,		NULL}		 /* Sentinel */
};


void
initposix()
{
	object *m, *d, *v;
	
	m = initmodule("posix", posix_methods);
	d = getmoduledict(m);
	
	/* Initialize posix.environ dictionary */
	v = convertenviron();
	if (v == NULL || dictinsert(d, "environ", v) != 0)
		fatal("can't define posix.environ");
	DECREF(v);
	
	/* Initialize posix.error exception */
	PosixError = newstringobject("posix.error");
	if (PosixError == NULL || dictinsert(d, "error", PosixError) != 0)
		fatal("can't define posix.error");
}
