/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
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

#ifndef __MWERKS__
#include <fcntl.h>
#endif

#include "macdefs.h"
#include "dirent.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/* Prototypes for Unix simulation on Mac */

int chdir PROTO((const char *path));
char *getbootvol PROTO((void));
char *getwd PROTO((char *));
int mkdir PROTO((const char *path, int mode));
DIR * opendir PROTO((char *));
void closedir PROTO((DIR *));
struct dirent * readdir PROTO((DIR *));
int rmdir PROTO((const char *path));
int sync PROTO((void));
#ifdef THINK_C
int unlink PROTO((char *));
#else
int unlink PROTO((const char *));
#endif



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
	return mac_1str(args, chdir);
}

#ifndef __MWERKS__
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
	if (res < 0)
		return mac_error();
	INCREF(None);
	return None;
}
#endif /* !__MWERKS__ */

#ifdef MPW

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

#endif /* MPW */

#ifndef __MWERKS__
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
	res = getwd(path);
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

#ifndef __MWERKS__
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
#endif /* !__MWERKS__ */

static object *
mac_mkdir(self, args)
	object *self;
	object *args;
{
	return mac_strint(args, mkdir);
}

#ifndef __MWERKS__
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
#endif /* !__MWERKS */

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
	struct macstat st;
	char *path;
	int res;
	if (!getargs(args, "s", &path))
		return NULL;
	BGN_SAVE
	res = macstat(path, &st);
	END_SAVE
	if (res != 0)
		return mac_error();
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

#ifndef __MWERKS__
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
#endif /* !__MWERKS__ */

static struct methodlist mac_methods[] = {
	{"chdir",	mac_chdir},
#ifndef __MWERKS__
	{"close",	mac_close},
#endif
#ifdef MPW
	{"dup",		mac_dup},
#endif
#ifndef __MWERKS__
	{"fdopen",	mac_fdopen},
#endif
	{"getbootvol",	mac_getbootvol}, /* non-standard */
	{"getcwd",	mac_getcwd},
	{"listdir",	mac_listdir},
#ifndef __MWERKS__
	{"lseek",	mac_lseek},
#endif
	{"mkdir",	mac_mkdir},
#ifndef __MWERKS__
	{"open",	mac_open},
	{"read",	mac_read},
#endif
	{"rename",	mac_rename},
	{"rmdir",	mac_rmdir},
	{"stat",	mac_stat},
	{"sync",	mac_sync},
	{"unlink",	mac_unlink},
#ifndef __MWERKS__
	{"write",	mac_write},
#endif

	{NULL,		NULL}		 /* Sentinel */
};


void
initmac()
{
	object *m, *d, *v;
	
	m = initmodule("mac", mac_methods);
	d = getmoduledict(m);
	
	/* Initialize mac.error exception */
	MacError = newstringobject("mac.error");
	if (MacError == NULL || dictinsert(d, "error", MacError) != 0)
		fatal("can't define mac.error");
}
