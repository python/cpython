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
#include "structseq.h"
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
	if (!PyArg_ParseTuple(args, "s", &path1))
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
	if (!PyArg_ParseTuple(args, "ss", &path1, &path2))
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
	if (!PyArg_ParseTuple(args, "si", &path, &i))
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
	if (!PyArg_ParseTuple(args, "i", &fd))
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
	if (!PyArg_ParseTuple(args, "i", &fd))
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
	if (!PyArg_ParseTuple(args, "is", &fd, &mode))
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
	if (!PyArg_ParseTuple(args, ""))
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
	if (!PyArg_ParseTuple(args, ""))
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
	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;
#ifdef USE_GUSI
	/* Work around a bug in GUSI: if you opendir() a file it will
	** actually opendir() the parent directory.
	*/
	{
		struct stat stb;
		int res;
		
		res = stat(name, &stb);
		if ( res < 0 )
			return mac_error();
		if (!S_ISDIR(stb.st_mode) ) {
			errno = ENOTDIR;
			return mac_error();
		}
	}
#endif
		
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
	if (!PyArg_ParseTuple(args, "iii", &fd, &where, &how))
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
	int perm; /* Accepted but ignored */
	int fd;
	if (!PyArg_ParseTuple(args, "si|i", &path, &mode, &perm))
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
	if (!PyArg_ParseTuple(args, "ii", &fd, &size))
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

static char stat_result__doc__[] = 
"stat_result: Result from stat or lstat.\n\n\
This object may be accessed either as a tuple of\n\
  (mode,ino,dev,nlink,uid,gid,size,atime,mtime,ctime)\n\
or via the attributes st_mode, st_ino, st_dev, st_nlink, st_uid, and so on.\n\
\n\
See os.stat for more information.\n";

#define COMMON_STAT_RESULT_FIELDS \
        { "st_mode",  "protection bits" }, \
        { "st_ino",   "inode" }, \
        { "st_dev",   "device" }, \
        { "st_nlink", "number of hard links" }, \
        { "st_uid",   "user ID of owner" }, \
        { "st_gid",   "group ID of owner" }, \
        { "st_size",  "total size, in bytes" }, \
        { "st_atime", "time of last access" }, \
        { "st_mtime", "time of last modification" }, \
        { "st_ctime", "time of last change" },



static PyStructSequence_Field stat_result_fields[] = {
	COMMON_STAT_RESULT_FIELDS
	{0}
};

static PyStructSequence_Desc stat_result_desc = {
	"stat_result",
	stat_result__doc__,
	stat_result_fields,
	10
};

static PyTypeObject StatResultType;

#ifdef TARGET_API_MAC_OS8
static PyStructSequence_Field xstat_result_fields[] = {
	COMMON_STAT_RESULT_FIELDS
	{ "st_rsize" },
	{ "st_creator" },
	{ "st_type "},
	{0}
};

static PyStructSequence_Desc xstat_result_desc = {
	"xstat_result",
	stat_result__doc__,
	xstat_result_fields,
	13
};

static PyTypeObject XStatResultType;
#endif

static PyObject *
_pystat_from_struct_stat(struct stat st, void* _mst) 
{
	PyObject *v;

#if TARGET_API_MAC_OS8
	struct macstat *mst;

	if (_mst != NULL)
		v = PyStructSequence_New(&XStatResultType);
	else
#endif
		v = PyStructSequence_New(&StatResultType);
	PyStructSequence_SET_ITEM(v, 0, PyInt_FromLong((long)st.st_mode));
	PyStructSequence_SET_ITEM(v, 1, PyInt_FromLong((long)st.st_ino));
	PyStructSequence_SET_ITEM(v, 2, PyInt_FromLong((long)st.st_dev));
	PyStructSequence_SET_ITEM(v, 3, PyInt_FromLong((long)st.st_nlink));
	PyStructSequence_SET_ITEM(v, 4, PyInt_FromLong((long)st.st_uid));
	PyStructSequence_SET_ITEM(v, 5, PyInt_FromLong((long)st.st_gid));
	PyStructSequence_SET_ITEM(v, 6, PyInt_FromLong((long)st.st_size));
	PyStructSequence_SET_ITEM(v, 7, 
				  PyFloat_FromDouble((double)st.st_atime));
	PyStructSequence_SET_ITEM(v, 8, 
				  PyFloat_FromDouble((double)st.st_mtime));
	PyStructSequence_SET_ITEM(v, 9, 
				  PyFloat_FromDouble((double)st.st_ctime));
#if TARGET_API_MAC_OS8
	if (_mst != NULL) {
		mst = (struct macstat *) _mst;
		PyStructSequence_SET_ITEM(v, 10, 
					  PyInt_FromLong((long)mst->st_rsize));
		PyStructSequence_SET_ITEM(v, 11, 
				PyString_FromStringAndSize(mst->st_creator, 
							   4));
		PyStructSequence_SET_ITEM(v, 12, 
				PyString_FromStringAndSize(mst->st_type, 
							   4));
	}
#endif

        if (PyErr_Occurred()) {
                Py_DECREF(v);
                return NULL;
        }

        return v;
}


static PyObject *
mac_stat(self, args)
	PyObject *self;
	PyObject *args;
{
	struct stat st;
	char *path;
	int res;
	if (!PyArg_ParseTuple(args, "s", &path))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = stat(path, &st);
	Py_END_ALLOW_THREADS
	if (res != 0)
		return mac_error();

	return _pystat_from_struct_stat(st, NULL);
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
	if (!PyArg_ParseTuple(args, "l", &fd))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = fstat((int)fd, &st);
	Py_END_ALLOW_THREADS
	if (res != 0)
		return mac_error();

	return _pystat_from_struct_stat(st, NULL);
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
	if (!PyArg_ParseTuple(args, "s", &path))
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

	return _pystat_from_struct_stat(st, (void*) &mst);
}
#endif

static PyObject *
mac_sync(self, args)
	PyObject *self;
	PyObject *args;
{
	int res;
	if (!PyArg_ParseTuple(args, ""))
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
	if (!PyArg_ParseTuple(args, "is#", &fd, &buffer, &size))
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
	{"chdir",	mac_chdir, 1},
	{"close",	mac_close, 1},
#ifdef WEHAVE_DUP
	{"dup",		mac_dup, 1},
#endif
#ifdef WEHAVE_FDOPEN
	{"fdopen",	mac_fdopen, 1},
#endif
#ifdef WEHAVE_FSTAT
	{"fstat",	mac_fstat, 1},
#endif
#if TARGET_API_MAC_OS8
	{"getbootvol",	mac_getbootvol, 1}, /* non-standard */
#endif
	{"getcwd",	mac_getcwd, 1},
	{"listdir",	mac_listdir, 1},
	{"lseek",	mac_lseek, 1},
	{"mkdir",	mac_mkdir, 1},
	{"open",	mac_open, 1},
	{"read",	mac_read, 1},
	{"rename",	mac_rename, 1},
	{"rmdir",	mac_rmdir, 1},
	{"stat",	mac_stat, 1},
#if TARGET_API_MAC_OS8
	{"xstat",	mac_xstat, 1},
#endif
	{"sync",	mac_sync, 1},
	{"remove",	mac_unlink, 1},
	{"unlink",	mac_unlink, 1},
	{"write",	mac_write, 1},
#ifdef USE_MALLOC_DEBUG
	{"mstats",	mac_mstats, 1},
#endif

	{NULL,		NULL}		 /* Sentinel */
};

static int
ins(PyObject *d, char *symbol, long value)
{
        PyObject* v = PyInt_FromLong(value);
        if (!v || PyDict_SetItemString(d, symbol, v) < 0)
                return -1;                   /* triggers fatal error */

        Py_DECREF(v);
        return 0;
}

static int
all_ins(PyObject *d)
{
#ifdef F_OK
        if (ins(d, "F_OK", (long)F_OK)) return -1;
#endif        
#ifdef R_OK
        if (ins(d, "R_OK", (long)R_OK)) return -1;
#endif        
#ifdef W_OK
        if (ins(d, "W_OK", (long)W_OK)) return -1;
#endif        
#ifdef X_OK
        if (ins(d, "X_OK", (long)X_OK)) return -1;
#endif        
#ifdef NGROUPS_MAX
        if (ins(d, "NGROUPS_MAX", (long)NGROUPS_MAX)) return -1;
#endif
#ifdef TMP_MAX
        if (ins(d, "TMP_MAX", (long)TMP_MAX)) return -1;
#endif
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
#ifdef O_BINARY
        if (ins(d, "O_BINARY", (long)O_BINARY)) return -1;
#endif
#ifdef O_TEXT
        if (ins(d, "O_TEXT", (long)O_TEXT)) return -1;
#endif

#ifdef HAVE_SPAWNV
        if (ins(d, "P_WAIT", (long)_P_WAIT)) return -1;
        if (ins(d, "P_NOWAIT", (long)_P_NOWAIT)) return -1;
        if (ins(d, "P_OVERLAY", (long)_OLD_P_OVERLAY)) return -1;
        if (ins(d, "P_NOWAITO", (long)_P_NOWAITO)) return -1;
        if (ins(d, "P_DETACH", (long)_P_DETACH)) return -1;
#endif

#if defined(PYOS_OS2)
        if (insertvalues(d)) return -1;
#endif
        return 0;
}


void
initmac()
{
	PyObject *m, *d;
	
	m = Py_InitModule("mac", mac_methods);
	d = PyModule_GetDict(m);
	
        if (all_ins(d))
                return;

	/* Initialize mac.error exception */
	MacError = PyErr_NewException("mac.error", NULL, NULL);
	PyDict_SetItemString(d, "error", MacError);

	PyStructSequence_InitType(&StatResultType, &stat_result_desc);
	PyDict_SetItemString(d, "stat_result", (PyObject*) &StatResultType);

#if TARGET_API_MAC_OS8
	PyStructSequence_InitType(&XStatResultType, &xstat_result_desc);
	PyDict_SetItemString(d, "xstat_result", (PyObject*) &XStatResultType);
#endif
}
