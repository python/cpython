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

/* Mac module implementation */

#include "allobjects.h"
#include "modsupport.h"
#include "ceval.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef THINK_C
#include "unix.h"
#undef S_IFMT
#undef S_IFDIR
#undef S_IFCHR
#undef S_IFBLK
#undef S_IFREG
#undef S_ISDIR
#undef S_ISREG
#endif

#include "macstat.h"
#ifdef USE_GUSI
/* Remove defines from macstat.h */
#undef S_IFMT
#undef S_IFDIR
#undef S_IFREG
#undef S_IREAD
#undef S_IWRITE
#undef S_IEXEC

#include <GUSI.h>
#include <sys/types.h>
#include <stat.h>
#else
#define stat macstat
#endif

#ifdef __MWERKS__
#include <unix.h>
#else
#include <fcntl.h>
#endif

/* Optional routines, for some compiler/runtime combinations */
#if defined(USE_GUSI) || !defined(__MWERKS__)
#define WEHAVE_FDOPEN
#endif
#if defined(MPW) || defined(USE_GUSI)
#define WEHAVE_DUP
#endif

#include "macdefs.h"
#ifdef USE_GUSI
#include <dirent.h>
#else
#include "dirent.h"
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/* Prototypes for Unix simulation on Mac */

#ifndef USE_GUSI

int chdir PROTO((const char *path));
int mkdir PROTO((const char *path, int mode));
DIR * opendir PROTO((char *));
void closedir PROTO((DIR *));
struct dirent * readdir PROTO((DIR *));
int rmdir PROTO((const char *path));
int sync PROTO((void));

#if defined(THINK_C) || defined(__SC__)
int unlink PROTO((char *));
#else
int unlink PROTO((const char *));
#endif

#endif /* USE_GUSI */

char *getwd PROTO((char *));
char *getbootvol PROTO((void));


static object *MacError; /* Exception mac.error */

/* Set a MAC-specific error from errno, and return NULL */

static object * 
mac_error() 
{
	return err_errno(MacError);
}

/* MAC generic methods */

static object *
mac_1str(args, func)
	object *args;
	int (*func) FPROTO((const char *));
{
	char *path1;
	int res;
	if (!getargs(args, "s", &path1))
		return NULL;
	BGN_SAVE
	res = (*func)(path1);
	END_SAVE
	if (res < 0)
		return mac_error();
	INCREF(None);
	return None;
}

static object *
mac_2str(args, func)
	object *args;
	int (*func) FPROTO((const char *, const char *));
{
	char *path1, *path2;
	int res;
	if (!getargs(args, "(ss)", &path1, &path2))
		return NULL;
	BGN_SAVE
	res = (*func)(path1, path2);
	END_SAVE
	if (res < 0)
		return mac_error();
	INCREF(None);
	return None;
}

static object *
mac_strint(args, func)
	object *args;
	int (*func) FPROTO((const char *, int));
{
	char *path;
	int i;
	int res;
	if (!getargs(args, "(si)", &path, &i))
		return NULL;
	BGN_SAVE
	res = (*func)(path, i);
	END_SAVE
	if (res < 0)
		return mac_error();
	INCREF(None);
	return None;
}

static object *
mac_chdir(self, args)
	object *self;
	object *args;
{
#ifdef USE_GUSI
	object *rv;
	
	/* Change MacOS's idea of wd too */
	rv = mac_1str(args, chdir);
	PyMac_FixGUSIcd();
	return rv;
#else
	return mac_1str(args, chdir);
#endif

}

static object *
mac_close(self, args)
	object *self;
	object *args;
{
	int fd, res;
	if (!getargs(args, "i", &fd))
		return NULL;
	BGN_SAVE
	res = close(fd);
	END_SAVE
#ifndef USE_GUSI
	/* GUSI gives surious errors here? */
	if (res < 0)
		return mac_error();
#endif
	INCREF(None);
	return None;
}

#ifdef WEHAVE_DUP

static object *
mac_dup(self, args)
	object *self;
	object *args;
{
	int fd;
	if (!getargs(args, "i", &fd))
		return NULL;
	BGN_SAVE
	fd = dup(fd);
	END_SAVE
	if (fd < 0)
		return mac_error();
	return newintobject((long)fd);
}

#endif

#ifdef WEHAVE_FDOPEN
static object *
mac_fdopen(self, args)
	object *self;
	object *args;
{
	extern int fclose PROTO((FILE *));
	int fd;
	char *mode;
	FILE *fp;
	if (!getargs(args, "(is)", &fd, &mode))
		return NULL;
	BGN_SAVE
	fp = fdopen(fd, mode);
	END_SAVE
	if (fp == NULL)
		return mac_error();
	return newopenfileobject(fp, "(fdopen)", mode, fclose);
}
#endif

static object *
mac_getbootvol(self, args)
	object *self;
	object *args;
{
	char *res;
	if (!getnoarg(args))
		return NULL;
	BGN_SAVE
	res = getbootvol();
	END_SAVE
	if (res == NULL)
		return mac_error();
	return newstringobject(res);
}

static object *
mac_getcwd(self, args)
	object *self;
	object *args;
{
	char path[MAXPATHLEN];
	char *res;
	if (!getnoarg(args))
		return NULL;
	BGN_SAVE
#ifdef USE_GUSI
	res = getcwd(path, sizeof path);
#else
	res = getwd(path);
#endif
	END_SAVE
	if (res == NULL) {
		err_setstr(MacError, path);
		return NULL;
	}
	return newstringobject(res);
}

static object *
mac_listdir(self, args)
	object *self;
	object *args;
{
	char *name;
	object *d, *v;
	DIR *dirp;
	struct dirent *ep;
	if (!getargs(args, "s", &name))
		return NULL;
	BGN_SAVE
	if ((dirp = opendir(name)) == NULL) {
		RET_SAVE
		return mac_error();
	}
	if ((d = newlistobject(0)) == NULL) {
		closedir(dirp);
		RET_SAVE
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
	END_SAVE

	return d;
}

static object *
mac_lseek(self, args)
	object *self;
	object *args;
{
	int fd;
	int where;
	int how;
	long res;
	if (!getargs(args, "(iii)", &fd, &where, &how))
		return NULL;
	BGN_SAVE
	res = lseek(fd, (long)where, how);
	END_SAVE
	if (res < 0)
		return mac_error();
	return newintobject(res);
}

static object *
mac_mkdir(self, args)
	object *self;
	object *args;
{
	int res;
	char *path;
	int mode = 0777; /* Unused */
	if (!newgetargs(args, "s|i", &path, &mode))
		return NULL;
	BGN_SAVE
#ifdef USE_GUSI
	res = mkdir(path);
#else
	res = mkdir(path, mode);
#endif
	END_SAVE
	if (res < 0)
		return mac_error();
	INCREF(None);
	return None;
}

static object *
mac_open(self, args)
	object *self;
	object *args;
{
	char *path;
	int mode;
	int fd;
	if (!getargs(args, "(si)", &path, &mode))
		return NULL;
	BGN_SAVE
	fd = open(path, mode);
	END_SAVE
	if (fd < 0)
		return mac_error();
	return newintobject((long)fd);
}

static object *
mac_read(self, args)
	object *self;
	object *args;
{
	int fd, size;
	object *buffer;
	if (!getargs(args, "(ii)", &fd, &size))
		return NULL;
	buffer = newsizedstringobject((char *)NULL, size);
	if (buffer == NULL)
		return NULL;
	BGN_SAVE
	size = read(fd, getstringvalue(buffer), size);
	END_SAVE
	if (size < 0) {
		DECREF(buffer);
		return mac_error();
	}
	resizestring(&buffer, size);
	return buffer;
}

static object *
mac_rename(self, args)
	object *self;
	object *args;
{
	return mac_2str(args, rename);
}

static object *
mac_rmdir(self, args)
	object *self;
	object *args;
{
	return mac_1str(args, rmdir);
}

static object *
mac_stat(self, args)
	object *self;
	object *args;
{
	struct stat st;
	char *path;
	int res;
	if (!getargs(args, "s", &path))
		return NULL;
	BGN_SAVE
	res = stat(path, &st);
	END_SAVE
	if (res != 0)
		return mac_error();
#if 1
	return mkvalue("(lllllllddd)",
		    (long)st.st_mode,
		    (long)st.st_ino,
		    (long)st.st_dev,
		    (long)st.st_nlink,
		    (long)st.st_uid,
		    (long)st.st_gid,
		    (long)st.st_size,
		    (double)st.st_atime,
		    (double)st.st_mtime,
		    (double)st.st_ctime);
#else
	return mkvalue("(llllllllll)",
		    (long)st.st_mode,
		    (long)st.st_ino,
		    (long)st.st_dev,
		    (long)st.st_nlink,
		    (long)st.st_uid,
		    (long)st.st_gid,
		    (long)st.st_size,
		    (long)st.st_atime,
		    (long)st.st_mtime,
		    (long)st.st_ctime);
#endif
}

static object *
mac_xstat(self, args)
	object *self;
	object *args;
{
	struct macstat mst;
	struct stat st;
	char *path;
	int res;
	if (!getargs(args, "s", &path))
		return NULL;
	/*
	** Convoluted: we want stat() and xstat() to agree, so we call both
	** stat and macstat, and use the latter only for values not provided by
	** the former.
	*/
	BGN_SAVE
	res = macstat(path, &mst);
	END_SAVE
	if (res != 0)
		return mac_error();
	BGN_SAVE
	res = stat(path, &st);
	END_SAVE
	if (res != 0)
		return mac_error();
#if 1
	return mkvalue("(llllllldddls#s#)",
		    (long)st.st_mode,
		    (long)st.st_ino,
		    (long)st.st_dev,
		    (long)st.st_nlink,
		    (long)st.st_uid,
		    (long)st.st_gid,
		    (long)st.st_size,
		    (double)st.st_atime,
		    (double)st.st_mtime,
		    (double)st.st_ctime,
		    (long)mst.st_rsize,
		    mst.st_creator, 4,
		    mst.st_type, 4);
#else
	return mkvalue("(llllllllllls#s#)",
		    (long)st.st_mode,
		    (long)st.st_ino,
		    (long)st.st_dev,
		    (long)st.st_nlink,
		    (long)st.st_uid,
		    (long)st.st_gid,
		    (long)st.st_size,
		    (long)st.st_atime,
		    (long)st.st_mtime,
		    (long)st.st_ctime,
		    (long)mst.st_rsize,
		    mst.st_creator, 4,
		    mst.st_type, 4);
#endif
}

static object *
mac_sync(self, args)
	object *self;
	object *args;
{
	int res;
	if (!getnoarg(args))
		return NULL;
	BGN_SAVE
	res = sync();
	END_SAVE
	if (res != 0)
		return mac_error();
	INCREF(None);
	return None;
}

static object *
mac_unlink(self, args)
	object *self;
	object *args;
{
	return mac_1str(args, (int (*)(const char *))unlink);
}

static object *
mac_write(self, args)
	object *self;
	object *args;
{
	int fd, size;
	char *buffer;
	if (!getargs(args, "(is#)", &fd, &buffer, &size))
		return NULL;
	BGN_SAVE
	size = write(fd, buffer, size);
	END_SAVE
	if (size < 0)
		return mac_error();
	return newintobject((long)size);
}

#ifdef USE_MALLOC_DEBUG
static object *
mac_mstats(self, args)
	object*self;
	object *args;
{
	mstats("python");
	INCREF(None);
	return None;
}
#endif USE_MALLOC_DEBUG

static struct methodlist mac_methods[] = {
	{"chdir",	mac_chdir},
	{"close",	mac_close},
#ifdef WEHAVE_DUP
	{"dup",		mac_dup},
#endif
#ifdef WEHAVE_FDOPEN
	{"fdopen",	mac_fdopen},
#endif
	{"getbootvol",	mac_getbootvol}, /* non-standard */
	{"getcwd",	mac_getcwd},
	{"listdir",	mac_listdir, 0},
	{"lseek",	mac_lseek},
	{"mkdir",	mac_mkdir, 1},
	{"open",	mac_open},
	{"read",	mac_read},
	{"rename",	mac_rename},
	{"rmdir",	mac_rmdir},
	{"stat",	mac_stat},
	{"xstat",	mac_xstat},
	{"sync",	mac_sync},
	{"remove",	mac_unlink},
	{"unlink",	mac_unlink},
	{"write",	mac_write},
#ifdef USE_MALLOC_DEBUG
	{"mstats",	mac_mstats},
#endif

	{NULL,		NULL}		 /* Sentinel */
};


void
initmac()
{
	object *m, *d;
	
	m = initmodule("mac", mac_methods);
	d = getmoduledict(m);
	
	/* Initialize mac.error exception */
	MacError = newstringobject("mac.error");
	if (MacError == NULL || dictinsert(d, "error", MacError) != 0)
		fatal("can't define mac.error");
}
