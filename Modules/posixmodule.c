/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* POSIX module implementation */

/* This file is also used for Windows NT and MS-Win.  In that case the module
   actually calls itself 'nt', not 'posix', and a few functions are
   either unimplemented or implemented differently.  The source
   assumes that for Windows NT, the macro 'MS_WIN32' is defined independent
   of the compiler used.  Different compilers define their own feature
   test macro, e.g. '__BORLANDC__' or '_MSC_VER'. */

/* See also ../Dos/dosmodule.c */

#include "Python.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>		/* For WNOHANG */
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "mytime.h"		/* For clock_t on some systems */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

/* Various compilers have only certain posix functions */
#ifdef __WATCOMC__		/* Watcom compiler */
#define HAVE_GETCWD     1
#define HAVE_OPENDIR    1
#define HAVE_SYSTEM	1
#if defined(__OS2__)
#define HAVE_EXECV      1
#define HAVE_WAIT       1
#endif
#include <process.h>
#else
#ifdef __BORLANDC__		/* Borland compiler */
#define HAVE_EXECV      1
#define HAVE_GETCWD     1
#define HAVE_GETEGID    1
#define HAVE_GETEUID    1
#define HAVE_GETGID     1
#define HAVE_GETPPID    1
#define HAVE_GETUID     1
#define HAVE_KILL       1
#define HAVE_OPENDIR    1
#define HAVE_PIPE       1
#define HAVE_POPEN      1
#define HAVE_SYSTEM	1
#define HAVE_WAIT       1
#else
#ifdef _MSC_VER		/* Microsoft compiler */
#define HAVE_GETCWD     1
#ifdef MS_WIN32
#define HAVE_EXECV      1
#define HAVE_PIPE       1
#define HAVE_POPEN      1
#define HAVE_SYSTEM	1
#else /* 16-bit Windows */
#endif /* !MS_WIN32 */
#else			/* all other compilers */
/* Unix functions that the configure script doesn't check for */
#define HAVE_EXECV      1
#define HAVE_FORK       1
#define HAVE_GETCWD     1
#define HAVE_GETEGID    1
#define HAVE_GETEUID    1
#define HAVE_GETGID     1
#define HAVE_GETPPID    1
#define HAVE_GETUID     1
#define HAVE_KILL       1
#define HAVE_OPENDIR    1
#define HAVE_PIPE       1
#define HAVE_POPEN      1
#define HAVE_SYSTEM	1
#define HAVE_WAIT       1
#endif  /* _MSC_VER */
#endif  /* __BORLANDC__ */
#endif  /* ! __WATCOMC__ */

#ifndef _MSC_VER

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef NeXT
/* NeXT's <unistd.h> and <utime.h> aren't worth much */
#undef HAVE_UNISTD_H
#undef HAVE_UTIME_H
/* #undef HAVE_GETCWD */
#endif

#ifdef HAVE_UNISTD_H
/* XXX These are for SunOS4.1.3 but shouldn't hurt elsewhere */
extern int rename();
extern int pclose();
extern int lstat();
extern int symlink();
#else /* !HAVE_UNISTD_H */
#if defined(__WATCOMC__) || defined(_MSC_VER)
extern int mkdir Py_PROTO((const char *));
#else
extern int mkdir Py_PROTO((const char *, mode_t));
#endif
extern int chdir Py_PROTO((const char *));
extern int rmdir Py_PROTO((const char *));
extern int chmod Py_PROTO((const char *, mode_t));
extern int chown Py_PROTO((const char *, uid_t, gid_t));
extern char *getcwd Py_PROTO((char *, int));
extern char *strerror Py_PROTO((int));
extern int link Py_PROTO((const char *, const char *));
extern int rename Py_PROTO((const char *, const char *));
extern int stat Py_PROTO((const char *, struct stat *));
extern int unlink Py_PROTO((const char *));
extern int pclose Py_PROTO((FILE *));
#ifdef HAVE_SYMLINK
extern int symlink Py_PROTO((const char *, const char *));
#endif /* HAVE_SYMLINK */
#ifdef HAVE_LSTAT
extern int lstat Py_PROTO((const char *, struct stat *));
#endif /* HAVE_LSTAT */
#endif /* !HAVE_UNISTD_H */

#endif /* !_MSC_VER */

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
#ifdef __WATCOMC__
#include <direct.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#endif
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

#ifdef _MSC_VER
#include <direct.h>
#include <io.h>
#include <process.h>
#include <windows.h>
#ifdef MS_WIN32
#define popen	_popen
#define pclose	_pclose
#else /* 16-bit Windows */
#include <dos.h>
#include <ctype.h>
#endif /* MS_WIN32 */
#endif /* _MSC_VER */

#ifdef OS2
#include <io.h>
#endif /* OS2 */

/* Return a dictionary corresponding to the POSIX environment table */

#if !defined(_MSC_VER) && !defined(__WATCOMC__)
extern char **environ;
#endif /* !_MSC_VER */

static PyObject *
convertenviron()
{
	PyObject *d;
	char **e;
	d = PyDict_New();
	if (d == NULL)
		return NULL;
	if (environ == NULL)
		return d;
	/* XXX This part ignores errors */
	for (e = environ; *e != NULL; e++) {
		PyObject *v;
		char *p = strchr(*e, '=');
		if (p == NULL)
			continue;
		v = PyString_FromString(p+1);
		if (v == NULL)
			continue;
		*p = '\0';
		(void) PyDict_SetItemString(d, *e, v);
		*p = '=';
		Py_DECREF(v);
	}
	return d;
}


static PyObject *PosixError; /* Exception posix.error */

/* Set a POSIX-specific error from errno, and return NULL */

static PyObject * posix_error()
{
	return PyErr_SetFromErrno(PosixError);
}


/* POSIX generic methods */

static PyObject *
posix_1str(args, func)
	PyObject *args;
	int (*func) Py_FPROTO((const char *));
{
	char *path1;
	int res;
	if (!PyArg_Parse(args, "s", &path1))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = (*func)(path1);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
posix_2str(args, func)
	PyObject *args;
	int (*func) Py_FPROTO((const char *, const char *));
{
	char *path1, *path2;
	int res;
	if (!PyArg_Parse(args, "(ss)", &path1, &path2))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = (*func)(path1, path2);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
posix_strint(args, func)
	PyObject *args;
	int (*func) Py_FPROTO((const char *, int));
{
	char *path;
	int i;
	int res;
	if (!PyArg_Parse(args, "(si)", &path, &i))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = (*func)(path, i);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
posix_strintint(args, func)
	PyObject *args;
	int (*func) Py_FPROTO((const char *, int, int));
{
	char *path;
	int i,i2;
	int res;
	if (!PyArg_Parse(args, "(sii)", &path, &i, &i2))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = (*func)(path, i, i2);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
posix_do_stat(self, args, statfunc)
	PyObject *self;
	PyObject *args;
	int (*statfunc) Py_FPROTO((const char *, struct stat *));
{
	struct stat st;
	char *path;
	int res;
	if (!PyArg_Parse(args, "s", &path))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = (*statfunc)(path, &st);
	Py_END_ALLOW_THREADS
	if (res != 0)
		return posix_error();
	return Py_BuildValue("(llllllllll)",
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

static PyObject *
posix_chdir(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_1str(args, chdir);
}

static PyObject *
posix_chmod(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_strint(args, chmod);
}

#ifdef HAVE_CHOWN
static PyObject *
posix_chown(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_strintint(args, chown);
}
#endif /* HAVE_CHOWN */

#ifdef HAVE_GETCWD
static PyObject *
posix_getcwd(self, args)
	PyObject *self;
	PyObject *args;
{
	char buf[1026];
	char *res;
	if (!PyArg_NoArgs(args))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = getcwd(buf, sizeof buf);
	Py_END_ALLOW_THREADS
	if (res == NULL)
		return posix_error();
	return PyString_FromString(buf);
}
#endif

#ifdef HAVE_LINK
static PyObject *
posix_link(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_2str(args, link);
}
#endif /* HAVE_LINK */

static PyObject *
posix_listdir(self, args)
	PyObject *self;
	PyObject *args;
{
#if defined(MS_WIN32) && !defined(HAVE_OPENDIR)

	char *name;
	int len;
	PyObject *d, *v;
	HANDLE hFindFile;
	WIN32_FIND_DATA FileData;
	char namebuf[MAX_PATH+5];

	if (!PyArg_Parse(args, "s#", &name, &len))
		return NULL;
	if (len >= MAX_PATH) {
		PyErr_SetString(PyExc_ValueError, "path too long");
		return NULL;
	}
	strcpy(namebuf, name);
	if (namebuf[len-1] != '/' && namebuf[len-1] != '\\')
		namebuf[len++] = '/';
	strcpy(namebuf + len, "*.*");

	if ((d = PyList_New(0)) == NULL)
		return NULL;

	hFindFile = FindFirstFile(namebuf, &FileData);
	if (hFindFile == INVALID_HANDLE_VALUE) {
		errno = GetLastError();
		return posix_error();
	}
	do {
		if (FileData.cFileName[0] == '.' &&
		    (FileData.cFileName[1] == '\0' ||
		     FileData.cFileName[1] == '.' &&
		     FileData.cFileName[2] == '\0'))
			continue;
		v = PyString_FromString(FileData.cFileName);
		if (v == NULL) {
			Py_DECREF(d);
			d = NULL;
			break;
		}
		if (PyList_Append(d, v) != 0) {
			Py_DECREF(v);
			Py_DECREF(d);
			d = NULL;
			break;
		}
		Py_DECREF(v);
	} while (FindNextFile(hFindFile, &FileData) == TRUE);

	if (FindClose(hFindFile) == FALSE) {
		errno = GetLastError();
		return posix_error();
	}

	return d;

#else /* !MS_WIN32 */
#ifdef _MSC_VER /* 16-bit Windows */

#ifndef MAX_PATH
#define MAX_PATH	250
#endif
	char *name, *pt;
	int len;
	PyObject *d, *v;
	char namebuf[MAX_PATH+5];
	struct _find_t ep;

	if (!PyArg_Parse(args, "s#", &name, &len))
		return NULL;
	if (len >= MAX_PATH) {
		PyErr_SetString(PyExc_ValueError, "path too long");
		return NULL;
	}
	strcpy(namebuf, name);
	for (pt = namebuf; *pt; pt++)
		if (*pt == '/')
			*pt = '\\';
	if (namebuf[len-1] != '\\')
		namebuf[len++] = '\\';
	strcpy(namebuf + len, "*.*");

	if ((d = PyList_New(0)) == NULL)
		return NULL;

	if (_dos_findfirst(namebuf, _A_RDONLY |
			   _A_HIDDEN | _A_SYSTEM | _A_SUBDIR, &ep) != 0)
        {
		errno = ENOENT;
		return posix_error();
	}
	do {
		if (ep.name[0] == '.' &&
		    (ep.name[1] == '\0' ||
		     ep.name[1] == '.' &&
		     ep.name[2] == '\0'))
			continue;
		strcpy(namebuf, ep.name);
		for (pt = namebuf; *pt; pt++)
			if (isupper(*pt))
				*pt = tolower(*pt);
		v = PyString_FromString(namebuf);
		if (v == NULL) {
			Py_DECREF(d);
			d = NULL;
			break;
		}
		if (PyList_Append(d, v) != 0) {
			Py_DECREF(v);
			Py_DECREF(d);
			d = NULL;
			break;
		}
		Py_DECREF(v);
	} while (_dos_findnext(&ep) == 0);

	return d;

#else

	char *name;
	PyObject *d, *v;
	DIR *dirp;
	struct dirent *ep;
	if (!PyArg_Parse(args, "s", &name))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	if ((dirp = opendir(name)) == NULL) {
		Py_BLOCK_THREADS
		return posix_error();
	}
	if ((d = PyList_New(0)) == NULL) {
		closedir(dirp);
		Py_BLOCK_THREADS
		return NULL;
	}
	while ((ep = readdir(dirp)) != NULL) {
		if (ep->d_name[0] == '.' &&
		    (NAMLEN(ep) == 1 ||
		     (ep->d_name[1] == '.' && NAMLEN(ep) == 2)))
			continue;
		v = PyString_FromStringAndSize(ep->d_name, NAMLEN(ep));
		if (v == NULL) {
			Py_DECREF(d);
			d = NULL;
			break;
		}
		if (PyList_Append(d, v) != 0) {
			Py_DECREF(v);
			Py_DECREF(d);
			d = NULL;
			break;
		}
		Py_DECREF(v);
	}
	closedir(dirp);
	Py_END_ALLOW_THREADS

	return d;

#endif /* !_MSC_VER */
#endif /* !MS_WIN32 */
}

static PyObject *
posix_mkdir(self, args)
	PyObject *self;
	PyObject *args;
{
	int res;
	char *path;
	int mode = 0777;
	if (!PyArg_ParseTuple(args, "s|i", &path, &mode))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
#if defined(__WATCOMC__) || defined(_MSC_VER)
	res = mkdir(path);
#else
	res = mkdir(path, mode);
#endif
	Py_END_ALLOW_THREADS
	if (res < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef HAVE_NICE
static PyObject *
posix_nice(self, args)
	PyObject *self;
	PyObject *args;
{
	int increment, value;

	if (!PyArg_Parse(args, "i", &increment))
		return NULL;
	value = nice(increment);
	if (value == -1)
		return posix_error();
	return PyInt_FromLong((long) value);
}
#endif /* HAVE_NICE */

static PyObject *
posix_rename(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_2str(args, rename);
}

static PyObject *
posix_rmdir(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_1str(args, rmdir);
}

static PyObject *
posix_stat(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_do_stat(self, args, stat);
}

#ifdef HAVE_SYSTEM
static PyObject *
posix_system(self, args)
	PyObject *self;
	PyObject *args;
{
	char *command;
	long sts;
	if (!PyArg_Parse(args, "s", &command))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	sts = system(command);
	Py_END_ALLOW_THREADS
	return PyInt_FromLong(sts);
}
#endif

static PyObject *
posix_umask(self, args)
	PyObject *self;
	PyObject *args;
{
	int i;
	if (!PyArg_Parse(args, "i", &i))
		return NULL;
	i = umask(i);
	if (i < 0)
		return posix_error();
	return PyInt_FromLong((long)i);
}

static PyObject *
posix_unlink(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_1str(args, unlink);
}

#ifdef HAVE_UNAME
static PyObject *
posix_uname(self, args)
	PyObject *self;
	PyObject *args;
{
	struct utsname u;
	int res;
	if (!PyArg_NoArgs(args))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = uname(&u);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return posix_error();
	return Py_BuildValue("(sssss)",
			     u.sysname,
			     u.nodename,
			     u.release,
			     u.version,
			     u.machine);
}
#endif /* HAVE_UNAME */

static PyObject *
posix_utime(self, args)
	PyObject *self;
	PyObject *args;
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

	if (!PyArg_Parse(args, "(s(ll))", &path, &atime, &mtime))
		return NULL;
	ATIME = atime;
	MTIME = mtime;
	Py_BEGIN_ALLOW_THREADS
	res = utime(path, UTIME_ARG);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
#undef UTIME_ARG
#undef ATIME
#undef MTIME
}


/* Process operations */

static PyObject *
posix__exit(self, args)
	PyObject *self;
	PyObject *args;
{
	int sts;
	if (!PyArg_Parse(args, "i", &sts))
		return NULL;
	_exit(sts);
	return NULL; /* Make gcc -Wall happy */
}

#ifdef HAVE_EXECV
static PyObject *
posix_execv(self, args)
	PyObject *self;
	PyObject *args;
{
	char *path;
	PyObject *argv;
	char **argvlist;
	int i, argc;
	PyObject *(*getitem) Py_PROTO((PyObject *, int));

	/* execv has two arguments: (path, argv), where
	   argv is a list or tuple of strings. */

	if (!PyArg_Parse(args, "(sO)", &path, &argv))
		return NULL;
	if (PyList_Check(argv)) {
		argc = PyList_Size(argv);
		getitem = PyList_GetItem;
	}
	else if (PyTuple_Check(argv)) {
		argc = PyTuple_Size(argv);
		getitem = PyTuple_GetItem;
	}
	else {
 badarg:
		PyErr_BadArgument();
		return NULL;
	}

	argvlist = PyMem_NEW(char *, argc+1);
	if (argvlist == NULL)
		return NULL;
	for (i = 0; i < argc; i++) {
		if (!PyArg_Parse((*getitem)(argv, i), "s", &argvlist[i])) {
			PyMem_DEL(argvlist);
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

	PyMem_DEL(argvlist);
	return posix_error();
}

static PyObject *
posix_execve(self, args)
	PyObject *self;
	PyObject *args;
{
	char *path;
	PyObject *argv, *env;
	char **argvlist;
	char **envlist;
	PyObject *key, *val, *keys=NULL, *vals=NULL;
	int i, pos, argc, envc;
	PyObject *(*getitem) Py_PROTO((PyObject *, int));

	/* execve has three arguments: (path, argv, env), where
	   argv is a list or tuple of strings and env is a dictionary
	   like posix.environ. */

	if (!PyArg_Parse(args, "(sOO)", &path, &argv, &env))
		return NULL;
	if (PyList_Check(argv)) {
		argc = PyList_Size(argv);
		getitem = PyList_GetItem;
	}
	else if (PyTuple_Check(argv)) {
		argc = PyTuple_Size(argv);
		getitem = PyTuple_GetItem;
	}
	else {
		PyErr_SetString(PyExc_TypeError, "argv must be tuple or list");
		return NULL;
	}
	if (!PyMapping_Check(env)) {
		PyErr_SetString(PyExc_TypeError, "env must be mapping object");
		return NULL;
	}

	argvlist = PyMem_NEW(char *, argc+1);
	if (argvlist == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	for (i = 0; i < argc; i++) {
		if (!PyArg_Parse((*getitem)(argv, i),
				 "s;argv must be list of strings",
				 &argvlist[i]))
		{
			goto fail_1;
		}
	}
	argvlist[argc] = NULL;

	i = PyMapping_Length(env);
	envlist = PyMem_NEW(char *, i + 1);
	if (envlist == NULL) {
		PyErr_NoMemory();
		goto fail_1;
	}
	envc = 0;
	keys = PyMapping_Keys(env);
	vals = PyMapping_Values(env);
	if (!keys || !vals)
		goto fail_2;
	
	for (pos = 0; pos < i; pos++) {
		char *p, *k, *v;

		key = PyList_GetItem(keys, pos);
		val = PyList_GetItem(vals, pos);
		if (!key || !val)
			goto fail_2;
		
		if (!PyArg_Parse(key, "s;non-string key in env", &k) ||
		    !PyArg_Parse(val, "s;non-string value in env", &v))
		{
			goto fail_2;
		}
		p = PyMem_NEW(char, PyString_Size(key)+PyString_Size(val) + 2);
		if (p == NULL) {
			PyErr_NoMemory();
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
		PyMem_DEL(envlist[envc]);
	PyMem_DEL(envlist);
 fail_1:
	PyMem_DEL(argvlist);
	Py_XDECREF(vals);
	Py_XDECREF(keys);
	return NULL;
}
#endif /* HAVE_EXECV */

#ifdef HAVE_FORK
static PyObject *
posix_fork(self, args)
	PyObject *self;
	PyObject *args;
{
	int pid;
	if (!PyArg_NoArgs(args))
		return NULL;
	pid = fork();
	if (pid == -1)
		return posix_error();
	return PyInt_FromLong((long)pid);
}
#endif

#ifdef HAVE_GETEGID
static PyObject *
posix_getegid(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long)getegid());
}
#endif

#ifdef HAVE_GETEUID
static PyObject *
posix_geteuid(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long)geteuid());
}
#endif

#ifdef HAVE_GETGID
static PyObject *
posix_getgid(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long)getgid());
}
#endif

static PyObject *
posix_getpid(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long)getpid());
}

#ifdef HAVE_GETPGRP
static PyObject *
posix_getpgrp(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
#ifdef GETPGRP_HAVE_ARG
	return PyInt_FromLong((long)getpgrp(0));
#else /* GETPGRP_HAVE_ARG */
	return PyInt_FromLong((long)getpgrp());
#endif /* GETPGRP_HAVE_ARG */
}
#endif /* HAVE_GETPGRP */

#ifdef HAVE_SETPGRP
static PyObject *
posix_setpgrp(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
#ifdef SETPGRP_HAVE_ARG
	if (setpgrp(0, 0) < 0)
#else /* SETPGRP_HAVE_ARG */
	if (setpgrp() < 0)
#endif /* SETPGRP_HAVE_ARG */
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}

#endif /* HAVE_SETPGRP */

#ifdef HAVE_GETPPID
static PyObject *
posix_getppid(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long)getppid());
}
#endif

#ifdef HAVE_GETUID
static PyObject *
posix_getuid(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long)getuid());
}
#endif

#ifdef HAVE_KILL
static PyObject *
posix_kill(self, args)
	PyObject *self;
	PyObject *args;
{
	int pid, sig;
	if (!PyArg_Parse(args, "(ii)", &pid, &sig))
		return NULL;
	if (kill(pid, sig) == -1)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}
#endif

#ifdef HAVE_PLOCK

#ifdef HAVE_SYS_LOCK_H
#include <sys/lock.h>
#endif

static PyObject *
posix_plock(self, args)
	PyObject *self;
	PyObject *args;
{
	int op;
	if (!PyArg_Parse(args, "i", &op))
		return NULL;
	if (plock(op) == -1)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}
#endif

#ifdef HAVE_POPEN
static PyObject *
posix_popen(self, args)
	PyObject *self;
	PyObject *args;
{
	char *name;
	char *mode = "r";
	int bufsize = -1;
	FILE *fp;
	PyObject *f;
	if (!PyArg_ParseTuple(args, "s|si", &name, &mode, &bufsize))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	fp = popen(name, mode);
	Py_END_ALLOW_THREADS
	if (fp == NULL)
		return posix_error();
	f = PyFile_FromFile(fp, name, mode, pclose);
	if (f != NULL)
		PyFile_SetBufSize(f, bufsize);
	return f;
}
#endif /* HAVE_POPEN */

#ifdef HAVE_SETUID
static PyObject *
posix_setuid(self, args)
	PyObject *self;
	PyObject *args;
{
	int uid;
	if (!PyArg_Parse(args, "i", &uid))
		return NULL;
	if (setuid(uid) < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* HAVE_SETUID */

#ifdef HAVE_SETGID
static PyObject *
posix_setgid(self, args)
	PyObject *self;
	PyObject *args;
{
	int gid;
	if (!PyArg_Parse(args, "i", &gid))
		return NULL;
	if (setgid(gid) < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* HAVE_SETGID */

#ifdef HAVE_WAITPID
static PyObject *
posix_waitpid(self, args)
	PyObject *self;
	PyObject *args;
{
	int pid, options, sts = 0;
	if (!PyArg_Parse(args, "(ii)", &pid, &options))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	pid = waitpid(pid, &sts, options);
	Py_END_ALLOW_THREADS
	if (pid == -1)
		return posix_error();
	else
		return Py_BuildValue("ii", pid, sts);
}
#endif /* HAVE_WAITPID */

#ifdef HAVE_WAIT
static PyObject *
posix_wait(self, args)
	PyObject *self;
	PyObject *args;
{
	int pid, sts;
	Py_BEGIN_ALLOW_THREADS
	pid = wait(&sts);
	Py_END_ALLOW_THREADS
	if (pid == -1)
		return posix_error();
	else
		return Py_BuildValue("ii", pid, sts);
}
#endif

static PyObject *
posix_lstat(self, args)
	PyObject *self;
	PyObject *args;
{
#ifdef HAVE_LSTAT
	return posix_do_stat(self, args, lstat);
#else /* !HAVE_LSTAT */
	return posix_do_stat(self, args, stat);
#endif /* !HAVE_LSTAT */
}

#ifdef HAVE_READLINK
static PyObject *
posix_readlink(self, args)
	PyObject *self;
	PyObject *args;
{
	char buf[MAXPATHLEN];
	char *path;
	int n;
	if (!PyArg_Parse(args, "s", &path))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	n = readlink(path, buf, (int) sizeof buf);
	Py_END_ALLOW_THREADS
	if (n < 0)
		return posix_error();
	return PyString_FromStringAndSize(buf, n);
}
#endif /* HAVE_READLINK */

#ifdef HAVE_SYMLINK
static PyObject *
posix_symlink(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_2str(args, symlink);
}
#endif /* HAVE_SYMLINK */

#ifdef HAVE_TIMES
#ifndef HZ
#define HZ 60 /* Universal constant :-) */
#endif /* HZ */
static PyObject *
posix_times(self, args)
	PyObject *self;
	PyObject *args;
{
	struct tms t;
	clock_t c;
	if (!PyArg_NoArgs(args))
		return NULL;
	errno = 0;
	c = times(&t);
	if (c == (clock_t) -1)
		return posix_error();
	return Py_BuildValue("ddddd",
			     (double)t.tms_utime / HZ,
			     (double)t.tms_stime / HZ,
			     (double)t.tms_cutime / HZ,
			     (double)t.tms_cstime / HZ,
			     (double)c / HZ);
}
#endif /* HAVE_TIMES */
#ifdef MS_WIN32
#define HAVE_TIMES	/* so the method table will pick it up */
static PyObject *
posix_times(self, args)
	PyObject *self;
	PyObject *args;
{
	FILETIME create, exit, kernel, user;
	HANDLE hProc;
	if (!PyArg_NoArgs(args))
		return NULL;
	hProc = GetCurrentProcess();
	GetProcessTimes(hProc,&create, &exit, &kernel, &user);
	return Py_BuildValue(
		"ddddd",
		(double)(kernel.dwHighDateTime*2E32+kernel.dwLowDateTime)/2E6,
		(double)(user.dwHighDateTime*2E32+user.dwLowDateTime) / 2E6,
		(double)0,
		(double)0,
		(double)0);
}
#endif /* MS_WIN32 */

#ifdef HAVE_SETSID
static PyObject *
posix_setsid(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	if (setsid() < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* HAVE_SETSID */

#ifdef HAVE_SETPGID
static PyObject *
posix_setpgid(self, args)
	PyObject *self;
	PyObject *args;
{
	int pid, pgrp;
	if (!PyArg_Parse(args, "(ii)", &pid, &pgrp))
		return NULL;
	if (setpgid(pid, pgrp) < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* HAVE_SETPGID */

#ifdef HAVE_TCGETPGRP
static PyObject *
posix_tcgetpgrp(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd, pgid;
	if (!PyArg_Parse(args, "i", &fd))
		return NULL;
	pgid = tcgetpgrp(fd);
	if (pgid < 0)
		return posix_error();
	return PyInt_FromLong((long)pgid);
}
#endif /* HAVE_TCGETPGRP */

#ifdef HAVE_TCSETPGRP
static PyObject *
posix_tcsetpgrp(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd, pgid;
	if (!PyArg_Parse(args, "(ii)", &fd, &pgid))
		return NULL;
	if (tcsetpgrp(fd, pgid) < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* HAVE_TCSETPGRP */

/* Functions acting on file descriptors */

static PyObject *
posix_open(self, args)
	PyObject *self;
	PyObject *args;
{
	char *file;
	int flag;
	int mode = 0777;
	int fd;
	if (!PyArg_ParseTuple(args, "si|i", &file, &flag, &mode))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	fd = open(file, flag, mode);
	Py_END_ALLOW_THREADS
	if (fd < 0)
		return posix_error();
	return PyInt_FromLong((long)fd);
}

static PyObject *
posix_close(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd, res;
	if (!PyArg_Parse(args, "i", &fd))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = close(fd);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
posix_dup(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd;
	if (!PyArg_Parse(args, "i", &fd))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	fd = dup(fd);
	Py_END_ALLOW_THREADS
	if (fd < 0)
		return posix_error();
	return PyInt_FromLong((long)fd);
}

static PyObject *
posix_dup2(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd, fd2, res;
	if (!PyArg_Parse(args, "(ii)", &fd, &fd2))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = dup2(fd, fd2);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
posix_lseek(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd, how;
	long pos, res;
	if (!PyArg_Parse(args, "(ili)", &fd, &pos, &how))
		return NULL;
#ifdef SEEK_SET
	/* Turn 0, 1, 2 into SEEK_{SET,CUR,END} */
	switch (how) {
	case 0: how = SEEK_SET; break;
	case 1: how = SEEK_CUR; break;
	case 2: how = SEEK_END; break;
	}
#endif /* SEEK_END */
	Py_BEGIN_ALLOW_THREADS
	res = lseek(fd, pos, how);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return posix_error();
	return PyInt_FromLong(res);
}

static PyObject *
posix_read(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd, size, n;
	PyObject *buffer;
	if (!PyArg_Parse(args, "(ii)", &fd, &size))
		return NULL;
	buffer = PyString_FromStringAndSize((char *)NULL, size);
	if (buffer == NULL)
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	n = read(fd, PyString_AsString(buffer), size);
	Py_END_ALLOW_THREADS
	if (n < 0) {
		Py_DECREF(buffer);
		return posix_error();
	}
	if (n != size)
		_PyString_Resize(&buffer, n);
	return buffer;
}

static PyObject *
posix_write(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd, size;
	char *buffer;
	if (!PyArg_Parse(args, "(is#)", &fd, &buffer, &size))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	size = write(fd, buffer, size);
	Py_END_ALLOW_THREADS
	if (size < 0)
		return posix_error();
	return PyInt_FromLong((long)size);
}

static PyObject *
posix_fstat(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd;
	struct stat st;
	int res;
	if (!PyArg_Parse(args, "i", &fd))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = fstat(fd, &st);
	Py_END_ALLOW_THREADS
	if (res != 0)
		return posix_error();
	return Py_BuildValue("(llllllllll)",
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

static PyObject *
posix_fdopen(self, args)
	PyObject *self;
	PyObject *args;
{
	extern int fclose Py_PROTO((FILE *));
	int fd;
	char *mode = "r";
	int bufsize = -1;
	FILE *fp;
	PyObject *f;
	if (!PyArg_ParseTuple(args, "i|si", &fd, &mode, &bufsize))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	fp = fdopen(fd, mode);
	Py_END_ALLOW_THREADS
	if (fp == NULL)
		return posix_error();
	f = PyFile_FromFile(fp, "(fdopen)", mode, fclose);
	if (f != NULL)
		PyFile_SetBufSize(f, bufsize);
	return f;
}

#ifdef HAVE_PIPE
static PyObject *
posix_pipe(self, args)
	PyObject *self;
	PyObject *args;
{
#if !defined(MS_WIN32)
	int fds[2];
	int res;
	if (!PyArg_Parse(args, ""))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = pipe(fds);
	Py_END_ALLOW_THREADS
	if (res != 0)
		return posix_error();
	return Py_BuildValue("(ii)", fds[0], fds[1]);
#else /* MS_WIN32 */
	HANDLE read, write;
	BOOL ok;
	if (!PyArg_Parse(args, ""))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	ok = CreatePipe( &read, &write, NULL, 0);
	Py_END_ALLOW_THREADS
	if (!ok)
		return posix_error();
	return Py_BuildValue("(ii)", read, write);
#endif /* MS_WIN32 */
}
#endif  /* HAVE_PIPE */

#ifdef HAVE_MKFIFO
static PyObject *
posix_mkfifo(self, args)
	PyObject *self;
	PyObject *args;
{
	char *file;
	int mode = 0666;
	int res;
	if (!PyArg_ParseTuple(args, "s|i", &file, &mode))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = mkfifo(file, mode);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return posix_error();
	Py_INCREF(Py_None);
	return Py_None;
}
#endif

#ifdef HAVE_FTRUNCATE
static PyObject *
posix_ftruncate(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	int fd;
	long length;
	int res;

	if (!PyArg_Parse(args, "(il)", &fd, &length))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	res = ftruncate(fd, length);
	Py_END_ALLOW_THREADS
	if (res < 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}
#endif

#ifdef HAVE_PUTENV
static PyObject * 
posix_putenv(self,args)
	PyObject *self;
	PyObject *args;
{
        char *s1, *s2;
        char *new;

	if (!PyArg_ParseTuple(args, "ss", &s1, &s2))
		return NULL;
	/* XXX This leaks memory -- not easy to fix :-( */
	if ((new = malloc(strlen(s1) + strlen(s2) + 2)) == NULL)
                return PyErr_NoMemory();
	(void) sprintf(new, "%s=%s", s1, s2);
	if (putenv(new)) {
                posix_error();
                return NULL;
	}
	Py_INCREF(Py_None);
        return Py_None;
}
#endif

static PyMethodDef posix_methods[] = {
	{"chdir",	posix_chdir},
	{"chmod",	posix_chmod},
#ifdef HAVE_CHOWN
	{"chown",	posix_chown},
#endif /* HAVE_CHOWN */
#ifdef HAVE_GETCWD
	{"getcwd",	posix_getcwd},
#endif
#ifdef HAVE_LINK
	{"link",	posix_link},
#endif /* HAVE_LINK */
	{"listdir",	posix_listdir},
	{"lstat",	posix_lstat},
	{"mkdir",	posix_mkdir, 1},
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
#ifdef HAVE_SYSTEM
	{"system",	posix_system},
#endif
	{"umask",	posix_umask},
#ifdef HAVE_UNAME
	{"uname",	posix_uname},
#endif /* HAVE_UNAME */
	{"unlink",	posix_unlink},
	{"remove",	posix_unlink},
	{"utime",	posix_utime},
#ifdef HAVE_TIMES
	{"times",	posix_times},
#endif /* HAVE_TIMES */
	{"_exit",	posix__exit},
#ifdef HAVE_EXECV
	{"execv",	posix_execv},
	{"execve",	posix_execve},
#endif /* HAVE_EXECV */
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
#ifdef HAVE_PLOCK
	{"plock",	posix_plock},
#endif /* HAVE_PLOCK */
#ifdef HAVE_POPEN
	{"popen",	posix_popen,	1},
#endif /* HAVE_POPEN */
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
	{"open",	posix_open, 1},
	{"close",	posix_close},
	{"dup",		posix_dup},
	{"dup2",	posix_dup2},
	{"lseek",	posix_lseek},
	{"read",	posix_read},
	{"write",	posix_write},
	{"fstat",	posix_fstat},
	{"fdopen",	posix_fdopen,	1},
#ifdef HAVE_PIPE
	{"pipe",	posix_pipe},
#endif
#ifdef HAVE_MKFIFO
	{"mkfifo",	posix_mkfifo, 1},
#endif
#ifdef HAVE_FTRUNCATE
	{"ftruncate",	posix_ftruncate, 1},
#endif
#ifdef HAVE_PUTENV
	{"putenv",	posix_putenv, 1},
#endif
	{NULL,		NULL}		 /* Sentinel */
};


static int
ins(d, symbol, value)
        PyObject* d;
        char* symbol;
        long value;
{
        PyObject* v = PyInt_FromLong(value);
        if (!v || PyDict_SetItemString(d, symbol, v) < 0)
                return -1;                   /* triggers fatal error */

        Py_DECREF(v);
        return 0;
}

static int
all_ins(d)
        PyObject* d;
{
#ifdef WNOHANG
        if (ins(d, "WNOHANG", (long)WNOHANG)) return -1;
#endif        
#ifdef O_RDONLY
        if (ins(d, "O_RDONLY", (long)O_RDONLY)) return -1;
#endif
#ifdef O_WRONLY
        if (ins(d, "O_WRONLY", (long)O_WRONLY)) return -1;
#endif
#ifdef O_RDWR
        if (ins(d, "O_RDWR", (long)O_RDWR)) return -1;
#endif
#ifdef O_NDELAY
        if (ins(d, "O_NDELAY", (long)O_NDELAY)) return -1;
#endif
#ifdef O_NONBLOCK
        if (ins(d, "O_NONBLOCK", (long)O_NONBLOCK)) return -1;
#endif
#ifdef O_APPEND
        if (ins(d, "O_APPEND", (long)O_APPEND)) return -1;
#endif
#ifdef O_DSYNC
        if (ins(d, "O_DSYNC", (long)O_DSYNC)) return -1;
#endif
#ifdef O_RSYNC
        if (ins(d, "O_RSYNC", (long)O_RSYNC)) return -1;
#endif
#ifdef O_SYNC
        if (ins(d, "O_SYNC", (long)O_SYNC)) return -1;
#endif
#ifdef O_NOCTTY
        if (ins(d, "O_NOCTTY", (long)O_NOCTTY)) return -1;
#endif
#ifdef O_CREAT
        if (ins(d, "O_CREAT", (long)O_CREAT)) return -1;
#endif
#ifdef O_EXCL
        if (ins(d, "O_EXCL", (long)O_EXCL)) return -1;
#endif
#ifdef O_TRUNC
        if (ins(d, "O_TRUNC", (long)O_TRUNC)) return -1;
#endif
        return 0;
}




#if defined(_MSC_VER) || defined(__WATCOMC__)
void
initnt()
{
	PyObject *m, *d, *v;
	
	m = Py_InitModule("nt", posix_methods);
	d = PyModule_GetDict(m);
	
	/* Initialize nt.environ dictionary */
	v = convertenviron();
	if (v == NULL || PyDict_SetItemString(d, "environ", v) != 0)
		goto finally;
	Py_DECREF(v);
	
        if (all_ins(d))
                goto finally;

	/* Initialize nt.error exception */
	PosixError = PyString_FromString("nt.error");
	PyDict_SetItemString(d, "error", PosixError);

        if (!PyErr_Occurred())
                return;

  finally:
        Py_FatalError("can't initialize NT posixmodule");
}
#else /* not a PC port */
void
initposix()
{
	PyObject *m, *d, *v;
	
	m = Py_InitModule("posix", posix_methods);
	d = PyModule_GetDict(m);
	
	/* Initialize posix.environ dictionary */
	v = convertenviron();
	if (v == NULL || PyDict_SetItemString(d, "environ", v) != 0)
                goto finally;
	Py_DECREF(v);
	
        if (all_ins(d))
                goto finally;

	/* Initialize posix.error exception */
	PosixError = PyString_FromString("posix.error");
	PyDict_SetItemString(d, "error", PosixError);

        if (!PyErr_Occurred())
                return;

  finally:
        Py_FatalError("can't initialize posix module");
}
#endif /* !_MSC_VER */
