/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
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

#include "Python.h"
#include "ceval.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#if TARGET_API_MAC_OS8
/* Skip for Carbon */
#include "macstat.h"
#endif

#ifdef USE_GUSI
/* Remove defines from macstat.h */
#undef S_IFMT
#undef S_IFDIR
#undef S_IFREG
#undef S_IREAD
#undef S_IWRITE
#undef S_IEXEC

#ifdef USE_GUSI1
#include <GUSI.h>
#endif /* USE_GUSI1 */
#include <sys/types.h>
#include <sys/stat.h>
#else /* USE_GUSI */
#if TARGET_API_MAC_OS8
#define stat macstat
#endif
#endif /* USE_GUSI */

#ifdef USE_GUSI2
#define sync bad_sync
#include <unistd.h>
#include <fcntl.h>
#undef sync
int sync(void);
#else
#define mode_t int
#include <fcntl.h>
#ifdef _POSIX
#include <unistd.h>
#include <stat.h>
#endif
#endif

/* Optional routines, for some compiler/runtime combinations */
#if defined(USE_GUSI) || !defined(__MWERKS__)
#define WEHAVE_FDOPEN
#endif
#if defined(MPW) || defined(USE_GUSI)
#define WEHAVE_DUP
#endif
#if defined(USE_GUSI)
#define WEHAVE_FSTAT
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

int chdir(const char *path);
int mkdir(const char *path, int mode);
DIR * opendir(char *);
void closedir(DIR *);
struct dirent * readdir(DIR *);
int rmdir(const char *path);
int sync(void);

int unlink(const char *);

#endif /* USE_GUSI */

char *getwd(char *);
char *getbootvol(void);


static PyObject *MacError; /* Exception mac.error */

/* Set a MAC-specific error from errno, and return NULL */

static PyObject * 
mac_error() 
{
	return PyErr_SetFromErrno(MacError);
}

/* MAC generic methods */

static PyObject *
mac_1str(args, func)
	PyObject *args;
	int (*func)(const char *);
{
	char *path1;
	int res;
	if (!PyArg_Parse(args, "s", &path1))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = (*func)(path1);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return mac_error();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
mac_2str(args, func)
	PyObject *args;
	int (*func)(const char *, const char *);
{
	char *path1, *path2;
	int res;
	if (!PyArg_Parse(args, "(ss)", &path1, &path2))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = (*func)(path1, path2);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return mac_error();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
mac_strint(args, func)
	PyObject *args;
	int (*func)(const char *, int);
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
		return mac_error();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
mac_chdir(self, args)
	PyObject *self;
	PyObject *args;
{
#ifdef USE_GUSI1
	PyObject *rv;
	
	/* Change MacOS's idea of wd too */
	rv = mac_1str(args, chdir);
	PyMac_FixGUSIcd();
	return rv;
#else
	return mac_1str(args, chdir);
#endif

}

static PyObject *
mac_close(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd, res;
	if (!PyArg_Parse(args, "i", &fd))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = close(fd);
	Py_END_ALLOW_THREADS
#ifndef USE_GUSI1
	/* GUSI gives surious errors here? */
	if (res < 0)
		return mac_error();
#endif
	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef WEHAVE_DUP

static PyObject *
mac_dup(self, args)
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
		return mac_error();
	return PyInt_FromLong((long)fd);
}

#endif

#ifdef WEHAVE_FDOPEN
static PyObject *
mac_fdopen(self, args)
	PyObject *self;
	PyObject *args;
{
	extern int fclose(FILE *);
	int fd;
	char *mode;
	FILE *fp;
	if (!PyArg_Parse(args, "(is)", &fd, &mode))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	fp = fdopen(fd, mode);
	Py_END_ALLOW_THREADS
	if (fp == NULL)
		return mac_error();
	return PyFile_FromFile(fp, "(fdopen)", mode, fclose);
}
#endif

#if TARGET_API_MAC_OS8
static PyObject *
mac_getbootvol(self, args)
	PyObject *self;
	PyObject *args;
{
	char *res;
	if (!PyArg_NoArgs(args))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = getbootvol();
	Py_END_ALLOW_THREADS
	if (res == NULL)
		return mac_error();
	return PyString_FromString(res);
}
#endif

static PyObject *
mac_getcwd(self, args)
	PyObject *self;
	PyObject *args;
{
	char path[MAXPATHLEN];
	char *res;
	if (!PyArg_NoArgs(args))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
#ifdef USE_GUSI
	res = getcwd(path, sizeof path);
#else
	res = getwd(path);
#endif
	Py_END_ALLOW_THREADS
	if (res == NULL) {
		PyErr_SetString(MacError, path);
		return NULL;
	}
	return PyString_FromString(res);
}

static PyObject *
mac_listdir(self, args)
	PyObject *self;
	PyObject *args;
{
	char *name;
	PyObject *d, *v;
	DIR *dirp;
	struct dirent *ep;
	if (!PyArg_Parse(args, "s", &name))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	if ((dirp = opendir(name)) == NULL) {
		Py_BLOCK_THREADS
		return mac_error();
	}
	if ((d = PyList_New(0)) == NULL) {
		closedir(dirp);
		Py_BLOCK_THREADS
		return NULL;
	}
	while ((ep = readdir(dirp)) != NULL) {
		v = PyString_FromString(ep->d_name);
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
}

static PyObject *
mac_lseek(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd;
	int where;
	int how;
	long res;
	if (!PyArg_Parse(args, "(iii)", &fd, &where, &how))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = lseek(fd, (long)where, how);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return mac_error();
	return PyInt_FromLong(res);
}

static PyObject *
mac_mkdir(self, args)
	PyObject *self;
	PyObject *args;
{
	int res;
	char *path;
	int mode = 0777; /* Unused */
	if (!PyArg_ParseTuple(args, "s|i", &path, &mode))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
#ifdef USE_GUSI1
	res = mkdir(path);
#else
	res = mkdir(path, mode);
#endif
	Py_END_ALLOW_THREADS
	if (res < 0)
		return mac_error();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
mac_open(self, args)
	PyObject *self;
	PyObject *args;
{
	char *path;
	int mode;
	int fd;
	if (!PyArg_Parse(args, "(si)", &path, &mode))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	fd = open(path, mode);
	Py_END_ALLOW_THREADS
	if (fd < 0)
		return mac_error();
	return PyInt_FromLong((long)fd);
}

static PyObject *
mac_read(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd, size;
	PyObject *buffer;
	if (!PyArg_Parse(args, "(ii)", &fd, &size))
		return NULL;
	buffer = PyString_FromStringAndSize((char *)NULL, size);
	if (buffer == NULL)
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	size = read(fd, PyString_AsString(buffer), size);
	Py_END_ALLOW_THREADS
	if (size < 0) {
		Py_DECREF(buffer);
		return mac_error();
	}
	_PyString_Resize(&buffer, size);
	return buffer;
}

static PyObject *
mac_rename(self, args)
	PyObject *self;
	PyObject *args;
{
	return mac_2str(args, rename);
}

static PyObject *
mac_rmdir(self, args)
	PyObject *self;
	PyObject *args;
{
	return mac_1str(args, rmdir);
}

static PyObject *
mac_stat(self, args)
	PyObject *self;
	PyObject *args;
{
	struct stat st;
	char *path;
	int res;
	if (!PyArg_Parse(args, "s", &path))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = stat(path, &st);
	Py_END_ALLOW_THREADS
	if (res != 0)
		return mac_error();
	return Py_BuildValue("(lllllllddd)",
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
}

#ifdef WEHAVE_FSTAT
static PyObject *
mac_fstat(self, args)
	PyObject *self;
	PyObject *args;
{
	struct stat st;
	long fd;
	int res;
	if (!PyArg_Parse(args, "l", &fd))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = fstat((int)fd, &st);
	Py_END_ALLOW_THREADS
	if (res != 0)
		return mac_error();
	return Py_BuildValue("(lllllllddd)",
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
}
#endif /* WEHAVE_FSTAT */

#if TARGET_API_MAC_OS8
static PyObject *
mac_xstat(self, args)
	PyObject *self;
	PyObject *args;
{
	struct macstat mst;
	struct stat st;
	char *path;
	int res;
	if (!PyArg_Parse(args, "s", &path))
		return NULL;
	/*
	** Convoluted: we want stat() and xstat() to agree, so we call both
	** stat and macstat, and use the latter only for values not provided by
	** the former.
	*/
	Py_BEGIN_ALLOW_THREADS
	res = macstat(path, &mst);
	Py_END_ALLOW_THREADS
	if (res != 0)
		return mac_error();
	Py_BEGIN_ALLOW_THREADS
	res = stat(path, &st);
	Py_END_ALLOW_THREADS
	if (res != 0)
		return mac_error();
	return Py_BuildValue("(llllllldddls#s#)",
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
}
#endif

static PyObject *
mac_sync(self, args)
	PyObject *self;
	PyObject *args;
{
	int res;
	if (!PyArg_NoArgs(args))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = sync();
	Py_END_ALLOW_THREADS
	if (res != 0)
		return mac_error();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
mac_unlink(self, args)
	PyObject *self;
	PyObject *args;
{
	return mac_1str(args, (int (*)(const char *))unlink);
}

static PyObject *
mac_write(self, args)
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
		return mac_error();
	return PyInt_FromLong((long)size);
}

#ifdef USE_MALLOC_DEBUG
void *mstats(char *);

static PyObject *
mac_mstats(self, args)
	PyObject*self;
	PyObject *args;
{
	mstats("python");
	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* USE_MALLOC_DEBUG */

static struct PyMethodDef mac_methods[] = {
	{"chdir",	mac_chdir},
	{"close",	mac_close},
#ifdef WEHAVE_DUP
	{"dup",		mac_dup},
#endif
#ifdef WEHAVE_FDOPEN
	{"fdopen",	mac_fdopen},
#endif
#ifdef WEHAVE_FSTAT
	{"fstat",	mac_fstat},
#endif
#if TARGET_API_MAC_OS8
	{"getbootvol",	mac_getbootvol}, /* non-standard */
#endif
	{"getcwd",	mac_getcwd},
	{"listdir",	mac_listdir, 0},
	{"lseek",	mac_lseek},
	{"mkdir",	mac_mkdir, 1},
	{"open",	mac_open},
	{"read",	mac_read},
	{"rename",	mac_rename},
	{"rmdir",	mac_rmdir},
	{"stat",	mac_stat},
#if TARGET_API_MAC_OS8
	{"xstat",	mac_xstat},
#endif
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
	PyObject *m, *d;
	
	m = Py_InitModule("mac", mac_methods);
	d = PyModule_GetDict(m);
	
	/* Initialize mac.error exception */
	MacError = PyErr_NewException("mac.error", NULL, NULL);
	PyDict_SetItemString(d, "error", MacError);
}
