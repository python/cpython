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

/* POSIX module implementation */

/* This file is also used for Windows NT.  In that case the module
   actually calls itself 'nt', not 'posix', and a few functions are
   either unimplemented or implemented differently.  The source
   assumes that for Windows NT, the macro 'NT' is defined independent
   of the compiler used.  Different compilers define their own feature
   test macro, e.g. '__BORLANDC__' or '_MSCVER'. */

/* For MS-DOS and Windows 3.x, use ../Dos/dosmodule.c */

#include "allobjects.h"
#include "modsupport.h"
#include "ceval.h"

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mytime.h"		/* For clock_t on some systems */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#ifndef NT
#define HAVE_FORK	1
#endif

#if !defined(NT) || defined(__BORLANDC__)
/* Unix functions that the configure script doesn't check for
   and that aren't easily available under NT except with Borland C */
#define HAVE_GETEGID	1
#define HAVE_GETEUID	1
#define HAVE_GETGID	1
#define HAVE_GETPPID	1
#define HAVE_GETUID	1
#define HAVE_KILL	1
#define HAVE_WAIT	1
#endif

#ifndef NT
#ifdef HAVE_UNISTD_H
#include <unistd.h>
/* XXX These are for SunOS4.1.3 but shouldn't hurt elsewhere */
extern int rename();
extern int pclose();
extern int lstat();
extern int symlink();
#else /* !HAVE_UNISTD_H */
extern int mkdir PROTO((const char *, mode_t));
extern int chdir PROTO((const char *));
extern int rmdir PROTO((const char *));
extern int chmod PROTO((const char *, mode_t));
extern int chown PROTO((const char *, uid_t, gid_t));
extern char *getcwd PROTO((char *, int));
extern char *strerror PROTO((int));
extern int link PROTO((const char *, const char *));
extern int rename PROTO((const char *, const char *));
extern int stat PROTO((const char *, struct stat *));
extern int unlink PROTO((const char *));
extern int pclose PROTO((FILE *));
#ifdef HAVE_SYMLINK
extern int symlink PROTO((const char *, const char *));
#endif /* HAVE_SYMLINK */
#ifdef HAVE_LSTAT
extern int lstat PROTO((const char *, struct stat *));
#endif /* HAVE_LSTAT */
#endif /* !HAVE_UNISTD_H */
#endif /* !NT */

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif /* HAVE_UTIME_H */

#ifdef HAVE_SYS_UTIME_H
#include <sys/utime.h>
#define HAVE_UTIME_H /* pretend we do for the rest of this file */
#endif /* HAVE_SYS_UTIME_H */

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif /* HAVE_SYS_TIMES_H */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif /* HAVE_SYS_UTSNAME_H */

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif /* MAXPATHLEN */

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#ifdef NT
#include <direct.h>
#include <io.h>
#include <process.h>
#include <windows.h>
#define popen   _popen
#define pclose	_pclose
#endif /* NT */

#ifdef OS2
#include <io.h>
#endif /* OS2 */

/* Return a dictionary corresponding to the POSIX environment table */

#ifndef NT
extern char **environ;
#endif /* !NT */

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

static object * posix_error()
{
	return err_errno(PosixError);
}


/* POSIX generic methods */

static object *
posix_1str(args, func)
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
		return posix_error();
	INCREF(None);
	return None;
}

static object *
posix_2str(args, func)
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
		return posix_error();
	INCREF(None);
	return None;
}

static object *
posix_strint(args, func)
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
		return posix_error();
	INCREF(None);
	return None;
}

static object *
posix_strintint(args, func)
	object *args;
	int (*func) FPROTO((const char *, int, int));
{
	char *path;
	int i,i2;
	int res;
	if (!getargs(args, "(sii)", &path, &i, &i2))
		return NULL;
	BGN_SAVE
	res = (*func)(path, i, i2);
	END_SAVE
	if (res < 0)
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
	char *path;
	int res;
	if (!getargs(args, "s", &path))
		return NULL;
	BGN_SAVE
	res = (*statfunc)(path, &st);
	END_SAVE
	if (res != 0)
		return posix_error();
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


/* POSIX methods */

static object *
posix_chdir(self, args)
	object *self;
	object *args;
{
	return posix_1str(args, chdir);
}

static object *
posix_chmod(self, args)
	object *self;
	object *args;
{
	return posix_strint(args, chmod);
}

#ifdef HAVE_CHOWN
static object *
posix_chown(self, args)
	object *self;
	object *args;
{
	return posix_strintint(args, chown);
}
#endif /* HAVE_CHOWN */

static object *
posix_getcwd(self, args)
	object *self;
	object *args;
{
	char buf[1026];
	char *res;
	if (!getnoarg(args))
		return NULL;
	BGN_SAVE
	res = getcwd(buf, sizeof buf);
	END_SAVE
	if (res == NULL)
		return posix_error();
	return newstringobject(buf);
}

#ifdef HAVE_LINK
static object *
posix_link(self, args)
	object *self;
	object *args;
{
	return posix_2str(args, link);
}
#endif /* HAVE_LINK */

static object *
posix_listdir(self, args)
	object *self;
	object *args;
{
#ifdef NT

	char *name;
	int len;
	object *d, *v;
	HANDLE hFindFile;
	WIN32_FIND_DATA FileData;
	char namebuf[MAX_PATH+5];

	if (!getargs(args, "s#", &name, &len))
		return NULL;
	if (len >= MAX_PATH) {
		err_setstr(ValueError, "path too long");
		return NULL;
	}
	strcpy(namebuf, name);
	if (namebuf[len-1] != '/' && namebuf[len-1] != '\\')
		namebuf[len++] = '/';
	strcpy(namebuf + len, "*.*");

	if ((d = newlistobject(0)) == NULL)
		return NULL;

	hFindFile = FindFirstFile(namebuf, &FileData);
	if (hFindFile == INVALID_HANDLE_VALUE) {
		errno = GetLastError();
		return posix_error();
	}
	do {
		v = newstringobject(FileData.cFileName);
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
	} while (FindNextFile(hFindFile, &FileData) == TRUE);

	if (FindClose(hFindFile) == FALSE) {
		errno = GetLastError();
		return posix_error();
	}

	return d;

#else /* !NT */

	char *name;
	object *d, *v;
	DIR *dirp;
	struct dirent *ep;
	if (!getargs(args, "s", &name))
		return NULL;
	BGN_SAVE
	if ((dirp = opendir(name)) == NULL) {
		RET_SAVE
		return posix_error();
	}
	if ((d = newlistobject(0)) == NULL) {
		closedir(dirp);
		RET_SAVE
		return NULL;
	}
	while ((ep = readdir(dirp)) != NULL) {
		v = newsizedstringobject(ep->d_name, NAMLEN(ep));
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

#endif /* !NT */
}

static object *
posix_mkdir(self, args)
	object *self;
	object *args;
{
	return posix_strint(args, mkdir);
}

#ifdef HAVE_NICE
static object *
posix_nice(self, args)
	object *self;
	object *args;
{
	int increment, value;

	if (!getargs(args, "i", &increment))
		return NULL;
	value = nice(increment);
	if (value == -1)
		return posix_error();
	return newintobject((long) value);
}
#endif /* HAVE_NICE */

static object *
posix_rename(self, args)
	object *self;
	object *args;
{
	return posix_2str(args, rename);
}

static object *
posix_rmdir(self, args)
	object *self;
	object *args;
{
	return posix_1str(args, rmdir);
}

static object *
posix_stat(self, args)
	object *self;
	object *args;
{
	return posix_do_stat(self, args, stat);
}

static object *
posix_system(self, args)
	object *self;
	object *args;
{
	char *command;
	long sts;
	if (!getargs(args, "s", &command))
		return NULL;
	BGN_SAVE
	sts = system(command);
	END_SAVE
	return newintobject(sts);
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
	return posix_1str(args, unlink);
}

#ifdef HAVE_UNAME
static object *
posix_uname(self, args)
	object *self;
	object *args;
{
	struct utsname u;
	object *v;
	int res;
	if (!getnoarg(args))
		return NULL;
	BGN_SAVE
	res = uname(&u);
	END_SAVE
	if (res < 0)
		return posix_error();
	return mkvalue("(sssss)",
		       u.sysname,
		       u.nodename,
		       u.release,
		       u.version,
		       u.machine);
}
#endif /* HAVE_UNAME */

static object *
posix_utime(self, args)
	object *self;
	object *args;
{
	char *path;
	long atime, mtime;
	int res;

#ifdef HAVE_UTIME_H
	struct utimbuf buf;
#define ATIME buf.actime
#define MTIME buf.modtime
#define UTIME_ARG &buf
#else /* HAVE_UTIME_H */
	time_t buf[2];
#define ATIME buf[0]
#define MTIME buf[1]
#define UTIME_ARG buf
#endif /* HAVE_UTIME_H */

	if (!getargs(args, "(s(ll))", &path, &atime, &mtime))
		return NULL;
	ATIME = atime;
	MTIME = mtime;
	BGN_SAVE
	res = utime(path, UTIME_ARG);
	END_SAVE
	if (res < 0)
		return posix_error();
	INCREF(None);
	return None;
#undef UTIME_ARG
#undef ATIME
#undef MTIME
}


/* Process operations */

static object *
posix__exit(self, args)
	object *self;
	object *args;
{
	int sts;
	if (!getintarg(args, &sts))
		return NULL;
	_exit(sts);
	/* NOTREACHED */
}

static object *
posix_execv(self, args)
	object *self;
	object *args;
{
	char *path;
	object *argv;
	char **argvlist;
	int i, argc;
	object *(*getitem) PROTO((object *, int));

	/* execv has two arguments: (path, argv), where
	   argv is a list or tuple of strings. */

	if (!getargs(args, "(sO)", &path, &argv))
		return NULL;
	if (is_listobject(argv)) {
		argc = getlistsize(argv);
		getitem = getlistitem;
	}
	else if (is_tupleobject(argv)) {
		argc = gettuplesize(argv);
		getitem = gettupleitem;
	}
	else {
 badarg:
		err_badarg();
		return NULL;
	}

	argvlist = NEW(char *, argc+1);
	if (argvlist == NULL)
		return NULL;
	for (i = 0; i < argc; i++) {
		if (!getargs((*getitem)(argv, i), "s", &argvlist[i])) {
			DEL(argvlist);
			goto badarg;
		}
	}
	argvlist[argc] = NULL;

#ifdef BAD_EXEC_PROTOTYPES
	execv(path, (const char **) argvlist);
#else /* BAD_EXEC_PROTOTYPES */
	execv(path, argvlist);
#endif /* BAD_EXEC_PROTOTYPES */

	/* If we get here it's definitely an error */

	DEL(argvlist);
	return posix_error();
}

static object *
posix_execve(self, args)
	object *self;
	object *args;
{
	char *path;
	object *argv, *env;
	char **argvlist;
	char **envlist;
	object *key, *val;
	int i, pos, argc, envc;
	object *(*getitem) PROTO((object *, int));

	/* execve has three arguments: (path, argv, env), where
	   argv is a list or tuple of strings and env is a dictionary
	   like posix.environ. */

	if (!getargs(args, "(sOO)", &path, &argv, &env))
		return NULL;
	if (is_listobject(argv)) {
		argc = getlistsize(argv);
		getitem = getlistitem;
	}
	else if (is_tupleobject(argv)) {
		argc = gettuplesize(argv);
		getitem = gettupleitem;
	}
	else {
		err_setstr(TypeError, "argv must be tuple or list");
		return NULL;
	}
	if (!is_dictobject(env)) {
		err_setstr(TypeError, "env must be dictionary");
		return NULL;
	}

	argvlist = NEW(char *, argc+1);
	if (argvlist == NULL) {
		err_nomem();
		return NULL;
	}
	for (i = 0; i < argc; i++) {
		if (!getargs((*getitem)(argv, i),
			     "s;argv must be list of strings",
			     &argvlist[i])) {
			goto fail_1;
		}
	}
	argvlist[argc] = NULL;

	i = getmappingsize(env);
	envlist = NEW(char *, i + 1);
	if (envlist == NULL) {
		err_nomem();
		goto fail_1;
	}
	pos = 0;
	envc = 0;
	while (mappinggetnext(env, &pos, &key, &val)) {
		char *p, *k, *v;
		if (!getargs(key, "s;non-string key in env", &k) ||
		    !getargs(val, "s;non-string value in env", &v)) {
			goto fail_2;
		}
		p = NEW(char, getstringsize(key) + getstringsize(val) + 2);
		if (p == NULL) {
			err_nomem();
			goto fail_2;
		}
		sprintf(p, "%s=%s", k, v);
		envlist[envc++] = p;
	}
	envlist[envc] = 0;


#ifdef BAD_EXEC_PROTOTYPES
	execve(path, (const char **)argvlist, envlist);
#else /* BAD_EXEC_PROTOTYPES */
	execve(path, argvlist, envlist);
#endif /* BAD_EXEC_PROTOTYPES */
	
	/* If we get here it's definitely an error */

	(void) posix_error();

 fail_2:
	while (--envc >= 0)
		DEL(envlist[envc]);
	DEL(envlist);
 fail_1:
	DEL(argvlist);

	return NULL;
}

#ifdef HAVE_FORK
static object *
posix_fork(self, args)
	object *self;
	object *args;
{
	int pid;
	if (!getnoarg(args))
		return NULL;
	pid = fork();
	if (pid == -1)
		return posix_error();
	return newintobject((long)pid);
}
#endif

#ifdef HAVE_GETEGID
static object *
posix_getegid(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newintobject((long)getegid());
}
#endif

#ifdef HAVE_GETEUID
static object *
posix_geteuid(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newintobject((long)geteuid());
}
#endif

#ifdef HAVE_GETGID
static object *
posix_getgid(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newintobject((long)getgid());
}
#endif

static object *
posix_getpid(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newintobject((long)getpid());
}

#ifdef HAVE_GETPGRP
static object *
posix_getpgrp(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
#ifdef GETPGRP_HAVE_ARG
	return newintobject((long)getpgrp(0));
#else /* GETPGRP_HAVE_ARG */
	return newintobject((long)getpgrp());
#endif /* GETPGRP_HAVE_ARG */
}
#endif /* HAVE_GETPGRP */

#ifdef HAVE_SETPGRP
static object *
posix_setpgrp(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
#ifdef SETPGRP_HAVE_ARG
	if (setpgrp(0, 0) < 0)
#else /* SETPGRP_HAVE_ARG */
	if (setpgrp() < 0)
#endif /* SETPGRP_HAVE_ARG */
		return posix_error();
	INCREF(None);
	return None;
}

#endif /* HAVE_SETPGRP */

#ifdef HAVE_GETPPID
static object *
posix_getppid(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newintobject((long)getppid());
}
#endif

#ifdef HAVE_GETUID
static object *
posix_getuid(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newintobject((long)getuid());
}
#endif

#ifdef HAVE_KILL
static object *
posix_kill(self, args)
	object *self;
	object *args;
{
	int pid, sig;
	if (!getargs(args, "(ii)", &pid, &sig))
		return NULL;
	if (kill(pid, sig) == -1)
		return posix_error();
	INCREF(None);
	return None;
}
#endif

static object *
posix_popen(self, args)
	object *self;
	object *args;
{
	char *name;
	char *mode = "r";
	int bufsize = -1;
	FILE *fp;
	object *f;
	if (!newgetargs(args, "s|si", &name, &mode, &bufsize))
		return NULL;
	BGN_SAVE
	fp = popen(name, mode);
	END_SAVE
	if (fp == NULL)
		return posix_error();
	f = newopenfileobject(fp, name, mode, pclose);
	if (f != NULL)
		setfilebufsize(f, bufsize);
	return f;
}

#ifdef HAVE_SETUID
static object *
posix_setuid(self, args)
	object *self;
	object *args;
{
	int uid;
	if (!getargs(args, "i", &uid))
		return NULL;
	if (setuid(uid) < 0)
		return posix_error();
	INCREF(None);
	return None;
}
#endif /* HAVE_SETUID */

#ifdef HAVE_SETGID
static object *
posix_setgid(self, args)
	object *self;
	object *args;
{
	int gid;
	if (!getargs(args, "i", &gid))
		return NULL;
	if (setgid(gid) < 0)
		return posix_error();
	INCREF(None);
	return None;
}
#endif /* HAVE_SETGID */

#ifdef HAVE_WAITPID
static object *
posix_waitpid(self, args)
	object *self;
	object *args;
{
	int pid, options, sts;
	if (!getargs(args, "(ii)", &pid, &options))
		return NULL;
	BGN_SAVE
	pid = waitpid(pid, &sts, options);
	END_SAVE
	if (pid == -1)
		return posix_error();
	else
		return mkvalue("ii", pid, sts);
}
#endif /* HAVE_WAITPID */

#ifdef HAVE_WAIT
static object *
posix_wait(self, args)
	object *self;
	object *args;
{
	int pid, sts;
	BGN_SAVE
	pid = wait(&sts);
	END_SAVE
	if (pid == -1)
		return posix_error();
	else
		return mkvalue("ii", pid, sts);
}
#endif

static object *
posix_lstat(self, args)
	object *self;
	object *args;
{
#ifdef HAVE_LSTAT
	return posix_do_stat(self, args, lstat);
#else /* !HAVE_LSTAT */
	return posix_do_stat(self, args, stat);
#endif /* !HAVE_LSTAT */
}

#ifdef HAVE_READLINK
static object *
posix_readlink(self, args)
	object *self;
	object *args;
{
	char buf[MAXPATHLEN];
	char *path;
	int n;
	if (!getargs(args, "s", &path))
		return NULL;
	BGN_SAVE
	n = readlink(path, buf, (int) sizeof buf);
	END_SAVE
	if (n < 0)
		return posix_error();
	return newsizedstringobject(buf, n);
}
#endif /* HAVE_READLINK */

#ifdef HAVE_SYMLINK
static object *
posix_symlink(self, args)
	object *self;
	object *args;
{
	return posix_2str(args, symlink);
}
#endif /* HAVE_SYMLINK */

#ifdef HAVE_TIMES
#ifndef HZ
#define HZ 60 /* Universal constant :-) */
#endif /* HZ */
static object *
posix_times(self, args)
	object *self;
	object *args;
{
	struct tms t;
	clock_t c;
	if (!getnoarg(args))
		return NULL;
	errno = 0;
	c = times(&t);
	if (c == (clock_t) -1)
		return posix_error();
	return mkvalue("dddd",
		       (double)t.tms_utime / HZ,
		       (double)t.tms_stime / HZ,
		       (double)t.tms_cutime / HZ,
		       (double)t.tms_cstime / HZ);
}
#endif /* HAVE_TIMES */
#ifdef NT
#define HAVE_TIMES	/* so the method table will pick it up */
static object *
posix_times(self, args)
	object *self;
	object *args;
{
	FILETIME create, exit, kernel, user;
	HANDLE hProc;
	if (!getnoarg(args))
		return NULL;
	hProc = GetCurrentProcess();
	GetProcessTimes(hProc,&create, &exit, &kernel, &user);
	return mkvalue("dddd",
		       (double)(kernel.dwHighDateTime*2E32+kernel.dwLowDateTime) / 2E6,
		       (double)(user.dwHighDateTime*2E32+user.dwLowDateTime) / 2E6,
		       (double)0,
		       (double)0);
}
#endif /* NT */

#ifdef HAVE_SETSID
static object *
posix_setsid(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	if (setsid() < 0)
		return posix_error();
	INCREF(None);
	return None;
}
#endif /* HAVE_SETSID */

#ifdef HAVE_SETPGID
static object *
posix_setpgid(self, args)
	object *self;
	object *args;
{
	int pid, pgrp;
	if (!getargs(args, "(ii)", &pid, &pgrp))
		return NULL;
	if (setpgid(pid, pgrp) < 0)
		return posix_error();
	INCREF(None);
	return None;
}
#endif /* HAVE_SETPGID */

#ifdef HAVE_TCGETPGRP
static object *
posix_tcgetpgrp(self, args)
	object *self;
	object *args;
{
	int fd, pgid;
	if (!getargs(args, "i", &fd))
		return NULL;
	pgid = tcgetpgrp(fd);
	if (pgid < 0)
		return posix_error();
	return newintobject((long)pgid);
}
#endif /* HAVE_TCGETPGRP */

#ifdef HAVE_TCSETPGRP
static object *
posix_tcsetpgrp(self, args)
	object *self;
	object *args;
{
	int fd, pgid;
	if (!getargs(args, "(ii)", &fd, &pgid))
		return NULL;
	if (tcsetpgrp(fd, pgid) < 0)
		return posix_error();
       INCREF(None);
	return None;
}
#endif /* HAVE_TCSETPGRP */

/* Functions acting on file descriptors */

static object *
posix_open(self, args)
	object *self;
	object *args;
{
	char *file;
	int flag;
	int mode = 0777;
	int fd;
	if (!getargs(args, "(si)", &file, &flag)) {
		err_clear();
		if (!getargs(args, "(sii)", &file, &flag, &mode))
			return NULL;
	}
	BGN_SAVE
	fd = open(file, flag, mode);
	END_SAVE
	if (fd < 0)
		return posix_error();
	return newintobject((long)fd);
}

static object *
posix_close(self, args)
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
		return posix_error();
	INCREF(None);
	return None;
}

static object *
posix_dup(self, args)
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
		return posix_error();
	return newintobject((long)fd);
}

static object *
posix_dup2(self, args)
	object *self;
	object *args;
{
	int fd, fd2, res;
	if (!getargs(args, "(ii)", &fd, &fd2))
		return NULL;
	BGN_SAVE
	res = dup2(fd, fd2);
	END_SAVE
	if (res < 0)
		return posix_error();
	INCREF(None);
	return None;
}

static object *
posix_lseek(self, args)
	object *self;
	object *args;
{
	int fd, how;
	long pos, res;
	if (!getargs(args, "(ili)", &fd, &pos, &how))
		return NULL;
#ifdef SEEK_SET
	/* Turn 0, 1, 2 into SEEK_{SET,CUR,END} */
	switch (how) {
	case 0: how = SEEK_SET; break;
	case 1: how = SEEK_CUR; break;
	case 2: how = SEEK_END; break;
	}
#endif /* SEEK_END */
	BGN_SAVE
	res = lseek(fd, pos, how);
	END_SAVE
	if (res < 0)
		return posix_error();
	return newintobject(res);
}

static object *
posix_read(self, args)
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
		return posix_error();
	}
	resizestring(&buffer, size);
	return buffer;
}

static object *
posix_write(self, args)
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
		return posix_error();
	return newintobject((long)size);
}

static object *
posix_fstat(self, args)
	object *self;
	object *args;
{
	int fd;
	struct stat st;
	int res;
	if (!getargs(args, "i", &fd))
		return NULL;
	BGN_SAVE
	res = fstat(fd, &st);
	END_SAVE
	if (res != 0)
		return posix_error();
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
posix_fdopen(self, args)
	object *self;
	object *args;
{
	extern int fclose PROTO((FILE *));
	int fd;
	char *mode = "r";
	int bufsize = -1;
	FILE *fp;
	object *f;
	if (!newgetargs(args, "i|si", &fd, &mode, &bufsize))
		return NULL;
	BGN_SAVE
	fp = fdopen(fd, mode);
	END_SAVE
	if (fp == NULL)
		return posix_error();
	f = newopenfileobject(fp, "(fdopen)", mode, fclose);
	if (f != NULL)
		setfilebufsize(f, bufsize);
	return f;
}

static object *
posix_pipe(self, args)
	object *self;
	object *args;
{
#ifndef NT
	int fds[2];
	int res;
	if (!getargs(args, ""))
		return NULL;
	BGN_SAVE
	res = pipe(fds);
	END_SAVE
	if (res != 0)
		return posix_error();
	return mkvalue("(ii)", fds[0], fds[1]);
#else /* NT */
	HANDLE read, write;
	BOOL ok;
	if (!getargs(args, ""))
		return NULL;
	BGN_SAVE
	ok = CreatePipe( &read, &write, NULL, 0);
	END_SAVE
	if (!ok)
		return posix_error();
	return mkvalue("(ii)", read, write);
#endif /* NT */
}

static struct methodlist posix_methods[] = {
	{"chdir",	posix_chdir},
	{"chmod",	posix_chmod},
#ifdef HAVE_CHOWN
	{"chown",	posix_chown},
#endif /* HAVE_CHOWN */
	{"getcwd",	posix_getcwd},
#ifdef HAVE_LINK
	{"link",	posix_link},
#endif /* HAVE_LINK */
	{"listdir",	posix_listdir},
	{"lstat",	posix_lstat},
	{"mkdir",	posix_mkdir},
#ifdef HAVE_NICE
	{"nice",	posix_nice},
#endif /* HAVE_NICE */
#ifdef HAVE_READLINK
	{"readlink",	posix_readlink},
#endif /* HAVE_READLINK */
	{"rename",	posix_rename},
	{"rmdir",	posix_rmdir},
	{"stat",	posix_stat},
#ifdef HAVE_SYMLINK
	{"symlink",	posix_symlink},
#endif /* HAVE_SYMLINK */
	{"system",	posix_system},
	{"umask",	posix_umask},
#ifdef HAVE_UNAME
	{"uname",	posix_uname},
#endif /* HAVE_UNAME */
	{"unlink",	posix_unlink},
	{"utime",	posix_utime},
#ifdef HAVE_TIMES
	{"times",	posix_times},
#endif /* HAVE_TIMES */
	{"_exit",	posix__exit},
	{"execv",	posix_execv},
	{"execve",	posix_execve},
#ifdef HAVE_FORK
	{"fork",	posix_fork},
#endif /* HAVE_FORK */
#ifdef HAVE_GETEGID
	{"getegid",	posix_getegid},
#endif /* HAVE_GETEGID */
#ifdef HAVE_GETEUID
	{"geteuid",	posix_geteuid},
#endif /* HAVE_GETEUID */
#ifdef HAVE_GETGID
	{"getgid",	posix_getgid},
#endif /* HAVE_GETGID */
	{"getpid",	posix_getpid},
#ifdef HAVE_GETPGRP
	{"getpgrp",	posix_getpgrp},
#endif /* HAVE_GETPGRP */
#ifdef HAVE_GETPPID
	{"getppid",	posix_getppid},
#endif /* HAVE_GETPPID */
#ifdef HAVE_GETUID
	{"getuid",	posix_getuid},
#endif /* HAVE_GETUID */
#ifdef HAVE_KILL
	{"kill",	posix_kill},
#endif /* HAVE_KILL */
	{"popen",	posix_popen,	1},
#ifdef HAVE_SETUID
	{"setuid",	posix_setuid},
#endif /* HAVE_SETUID */
#ifdef HAVE_SETGID
	{"setgid",	posix_setgid},
#endif /* HAVE_SETGID */
#ifdef HAVE_SETPGRP
	{"setpgrp",	posix_setpgrp},
#endif /* HAVE_SETPGRP */
#ifdef HAVE_WAIT
	{"wait",	posix_wait},
#endif /* HAVE_WAIT */
#ifdef HAVE_WAITPID
	{"waitpid",	posix_waitpid},
#endif /* HAVE_WAITPID */
#ifdef HAVE_SETSID
	{"setsid",	posix_setsid},
#endif /* HAVE_SETSID */
#ifdef HAVE_SETPGID
	{"setpgid",	posix_setpgid},
#endif /* HAVE_SETPGID */
#ifdef HAVE_TCGETPGRP
	{"tcgetpgrp",	posix_tcgetpgrp},
#endif /* HAVE_TCGETPGRP */
#ifdef HAVE_TCSETPGRP
	{"tcsetpgrp",	posix_tcsetpgrp},
#endif /* HAVE_TCSETPGRP */
	{"open",	posix_open},
	{"close",	posix_close},
	{"dup",		posix_dup},
	{"dup2",	posix_dup2},
	{"lseek",	posix_lseek},
	{"read",	posix_read},
	{"write",	posix_write},
	{"fstat",	posix_fstat},
	{"fdopen",	posix_fdopen,	1},
	{"pipe",	posix_pipe},
	{NULL,		NULL}		 /* Sentinel */
};


#ifdef NT
void
initnt()
{
	object *m, *d, *v;
	
	m = initmodule("nt", posix_methods);
	d = getmoduledict(m);
	
	/* Initialize nt.environ dictionary */
	v = convertenviron();
	if (v == NULL || dictinsert(d, "environ", v) != 0)
		fatal("can't define nt.environ");
	DECREF(v);
	
	/* Initialize nt.error exception */
	PosixError = newstringobject("nt.error");
	if (PosixError == NULL || dictinsert(d, "error", PosixError) != 0)
		fatal("can't define nt.error");
}
#else /* !NT */
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
#endif /* !NT */
