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

static char posix__doc__ [] =
"This module provides access to operating system functionality that is\n\
standardized by the C Standard and the POSIX standard (a thinly\n\
disguised Unix interface).  Refer to the library manual and\n\
corresponding Unix manual entries for more information on calls.";

#include "Python.h"

#if defined(PYOS_OS2)
#define  INCL_DOS
#define  INCL_DOSERRORS
#define  INCL_DOSPROCESS
#define  INCL_NOPMAPI
#include <os2.h>
#endif

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
/* XXX Gosh I wish these were all moved into config.h */
#if defined(PYCC_VACPP) && defined(PYOS_OS2)
#include <process.h>
#else
#if defined(__WATCOMC__) && !defined(__QNX__)		/* Watcom compiler */
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
#endif  /* ! __WATCOMC__ || __QNX__ */
#endif /* ! __IBMC__ */

#ifndef _MSC_VER

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef NeXT
/* NeXT's <unistd.h> and <utime.h> aren't worth much */
#undef HAVE_UNISTD_H
#undef HAVE_UTIME_H
#define HAVE_WAITPID
/* #undef HAVE_GETCWD */
#endif

#ifdef HAVE_UNISTD_H
/* XXX These are for SunOS4.1.3 but shouldn't hurt elsewhere */
extern int rename();
extern int pclose();
extern int lstat();
extern int symlink();
#else /* !HAVE_UNISTD_H */
#if defined(PYCC_VACPP)
extern int mkdir Py_PROTO((char *));
#else
#if ( defined(__WATCOMC__) || defined(_MSC_VER) ) && !defined(__QNX__)
extern int mkdir Py_PROTO((const char *));
#else
extern int mkdir Py_PROTO((const char *, mode_t));
#endif
#endif
#if defined(__IBMC__) || defined(__IBMCPP__)
extern int chdir Py_PROTO((char *));
extern int rmdir Py_PROTO((char *));
#else
extern int chdir Py_PROTO((const char *));
extern int rmdir Py_PROTO((const char *));
#endif
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
#if defined(__WATCOMC__) && !defined(__QNX__)
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

#if defined(PYCC_VACPP) && defined(PYOS_OS2)
#include <io.h>
#endif /* OS2 */

/* Return a dictionary corresponding to the POSIX environment table */

#if !defined(_MSC_VER) && ( !defined(__WATCOMC__) || defined(__QNX__) )
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
#if defined(PYOS_OS2)
    {
        APIRET rc;
        char   buffer[1024]; /* OS/2 Provides a Documented Max of 1024 Chars */

        rc = DosQueryExtLIBPATH(buffer, BEGIN_LIBPATH);
        if (rc == NO_ERROR) { /* (not a type, envname is NOT 'BEGIN_LIBPATH') */
            PyObject *v = PyString_FromString(buffer);
		    PyDict_SetItemString(d, "BEGINLIBPATH", v);
            Py_DECREF(v);
        }
        rc = DosQueryExtLIBPATH(buffer, END_LIBPATH);
        if (rc == NO_ERROR) { /* (not a typo, envname is NOT 'END_LIBPATH') */
            PyObject *v = PyString_FromString(buffer);
		    PyDict_SetItemString(d, "ENDLIBPATH", v);
            Py_DECREF(v);
        }
    }
#endif
	return d;
}


static PyObject *PosixError; /* Exception posix.error */

/* Set a POSIX-specific error from errno, and return NULL */

static PyObject *
posix_error()
{
	return PyErr_SetFromErrno(PosixError);
}
static PyObject *
posix_error_with_filename(name)
	char* name;
{
	return PyErr_SetFromErrnoWithFilename(PosixError, name);
}


#if defined(PYOS_OS2)
/**********************************************************************
 *         Helper Function to Trim and Format OS/2 Messages
 **********************************************************************/
    static void
os2_formatmsg(char *msgbuf, int msglen, char *reason)
{
    msgbuf[msglen] = '\0'; /* OS/2 Doesn't Guarantee a Terminator */

    if (strlen(msgbuf) > 0) { /* If Non-Empty Msg, Trim CRLF */
        char *lastc = &msgbuf[ strlen(msgbuf)-1 ];

        while (lastc > msgbuf && isspace(*lastc))
            *lastc-- = '\0'; /* Trim Trailing Whitespace (CRLF) */
    }

    /* Add Optional Reason Text */
    if (reason) {
        strcat(msgbuf, " : ");
        strcat(msgbuf, reason);
    }
}

/**********************************************************************
 *             Decode an OS/2 Operating System Error Code
 *
 * A convenience function to lookup an OS/2 error code and return a
 * text message we can use to raise a Python exception.
 *
 * Notes:
 *   The messages for errors returned from the OS/2 kernel reside in
 *   the file OSO001.MSG in the \OS2 directory hierarchy.
 *
 **********************************************************************/
    static char *
os2_strerror(char *msgbuf, int msgbuflen, int errorcode, char *reason)
{
    APIRET rc;
    ULONG  msglen;

    /* Retrieve Kernel-Related Error Message from OSO001.MSG File */
    Py_BEGIN_ALLOW_THREADS
    rc = DosGetMessage(NULL, 0, msgbuf, msgbuflen,
                       errorcode, "oso001.msg", &msglen);
    Py_END_ALLOW_THREADS

    if (rc == NO_ERROR)
        os2_formatmsg(msgbuf, msglen, reason);
    else
        sprintf(msgbuf, "unknown OS error #%d", errorcode);

    return msgbuf;
}

/* Set an OS/2-specific error and return NULL.  OS/2 kernel
   errors are not in a global variable e.g. 'errno' nor are
   they congruent with posix error numbers. */

static PyObject * os2_error(int code)
{
    char text[1024];
    PyObject *v;

    os2_strerror(text, sizeof(text), code, "");

    v = Py_BuildValue("(is)", code, text);
    if (v != NULL) {
        PyErr_SetObject(PosixError, v);
        Py_DECREF(v);
    }
    return NULL; /* Signal to Python that an Exception is Pending */
}

#endif /* OS2 */

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
		return posix_error_with_filename(path1);
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
	if (res != 0)
		/* XXX how to report both path1 and path2??? */
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
		return posix_error_with_filename(path);
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
		return posix_error_with_filename(path);
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
		return posix_error_with_filename(path);
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

static char posix_chdir__doc__[] =
"chdir(path) -> None\n\
Change the current working directory to the specified path.";

static PyObject *
posix_chdir(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_1str(args, chdir);
}


static char posix_chmod__doc__[] =
"chmod(path, mode) -> None\n\
Change the access permissions of a file.";

static PyObject *
posix_chmod(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_strint(args, chmod);
}


#ifdef HAVE_CHOWN
static char posix_chown__doc__[] =
"chown(path, uid, gid) -> None\n\
Change the owner and group id of path to the numeric uid and gid.";

static PyObject *
posix_chown(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_strintint(args, chown);
}
#endif /* HAVE_CHOWN */


#ifdef HAVE_GETCWD
static char posix_getcwd__doc__[] =
"getcwd() -> path\n\
Return a string representing the current working directory.";

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
static char posix_link__doc__[] =
"link(src, dst) -> None\n\
Create a hard link to a file.";

static PyObject *
posix_link(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_2str(args, link);
}
#endif /* HAVE_LINK */


static char posix_listdir__doc__[] =
"listdir(path) -> list_of_strings\n\
Return a list containing the names of the entries in the directory.\n\
\n\
	path: path of directory to list\n\
\n\
The list is in arbitrary order.  It does not include the special\n\
entries '.' and '..' even if they are present in the directory.";

static PyObject *
posix_listdir(self, args)
	PyObject *self;
	PyObject *args;
{
	/* XXX Should redo this putting the (now four) versions of opendir
	   in separate files instead of having them all here... */
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
		if (errno == ERROR_FILE_NOT_FOUND)
			return PyList_New(0);
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
#if defined(PYOS_OS2)

#ifndef MAX_PATH
#define MAX_PATH    CCHMAXPATH
#endif
    char *name, *pt;
    int len;
    PyObject *d, *v;
    char namebuf[MAX_PATH+5];
    HDIR  hdir = 1;
    ULONG srchcnt = 1;
    FILEFINDBUF3   ep;
    APIRET rc;

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

    rc = DosFindFirst(namebuf,         /* Wildcard Pattern to Match */
                      &hdir,           /* Handle to Use While Search Directory */
                      FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY,
                      &ep, sizeof(ep), /* Structure to Receive Directory Entry */
                      &srchcnt,        /* Max and Actual Count of Entries Per Iteration */
                      FIL_STANDARD);   /* Format of Entry (EAs or Not) */

    if (rc != NO_ERROR) {
        errno = ENOENT;
        return posix_error();
    }

    if (srchcnt > 0) { /* If Directory is NOT Totally Empty, */
        do {
            if (ep.achName[0] == '.'
            && (ep.achName[1] == '\0' || ep.achName[1] == '.' && ep.achName[2] == '\0'))
                continue; /* Skip Over "." and ".." Names */

            strcpy(namebuf, ep.achName);

            /* Leave Case of Name Alone -- In Native Form */
            /* (Removed Forced Lowercasing Code) */

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
        } while (DosFindNext(hdir, &ep, sizeof(ep), &srchcnt) == NO_ERROR && srchcnt > 0);
    }

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

#endif /* !PYOS_OS2 */
#endif /* !_MSC_VER */
#endif /* !MS_WIN32 */
}

static char posix_mkdir__doc__[] =
"mkdir(path [, mode=0777]) -> None\n\
Create a directory.";

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
#if ( defined(__WATCOMC__) || defined(_MSC_VER) || defined(PYCC_VACPP) ) && !defined(__QNX__)
	res = mkdir(path);
#else
	res = mkdir(path, mode);
#endif
	Py_END_ALLOW_THREADS
	if (res < 0)
		return posix_error_with_filename(path);
	Py_INCREF(Py_None);
	return Py_None;
}


#ifdef HAVE_NICE
static char posix_nice__doc__[] =
"nice(inc) -> new_priority\n\
Decrease the priority of process and return new priority.";

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


static char posix_rename__doc__[] =
"rename(old, new) -> None\n\
Rename a file or directory.";

static PyObject *
posix_rename(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_2str(args, rename);
}


static char posix_rmdir__doc__[] =
"rmdir(path) -> None\n\
Remove a directory.";

static PyObject *
posix_rmdir(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_1str(args, rmdir);
}


static char posix_stat__doc__[] =
"stat(path) -> (mode,ino,dev,nlink,uid,gid,size,atime,mtime,ctime)\n\
Perform a stat system call on the given path.";

static PyObject *
posix_stat(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_do_stat(self, args, stat);
}


#ifdef HAVE_SYSTEM
static char posix_system__doc__[] =
"system(command) -> exit_status\n\
Execute the command (a string) in a subshell.";

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


static char posix_umask__doc__[] =
"umask(new_mask) -> old_mask\n\
Set the current numeric umask and return the previous umask.";

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


static char posix_unlink__doc__[] =
"unlink(path) -> None\n\
Remove a file (same as remove(path)).";

static char posix_remove__doc__[] =
"remove(path) -> None\n\
Remove a file (same as unlink(path)).";

static PyObject *
posix_unlink(self, args)
	PyObject *self;
	PyObject *args;
{
	return posix_1str(args, unlink);
}


#ifdef HAVE_UNAME
static char posix_uname__doc__[] =
"uname() -> (sysname, nodename, release, version, machine)\n\
Return a tuple identifying the current operating system.";

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


static char posix_utime__doc__[] =
"utime(path, (atime, utime)) -> None\n\
Set the access and modified time of the file to the given values.";

static PyObject *
posix_utime(self, args)
	PyObject *self;
	PyObject *args;
{
	char *path;
	long atime, mtime;
	int res;

/* XXX should define struct utimbuf instead, above */
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
		return posix_error_with_filename(path);
	Py_INCREF(Py_None);
	return Py_None;
#undef UTIME_ARG
#undef ATIME
#undef MTIME
}


/* Process operations */

static char posix__exit__doc__[] =
"_exit(status)\n\
Exit to the system with specified status, without normal exit processing.";

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
static char posix_execv__doc__[] =
"execv(path, args)\n\
Execute an executable path with arguments, replacing current process.\n\
\n\
	path: path of executable file\n\
	args: tuple or list of strings";

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


static char posix_execve__doc__[] =
"execve(path, args, env)\n\
Execute a path with arguments and environment, replacing current process.\n\
\n\
	path: path of executable file\n\
	args: tuple or list of arguments\n\
	env: dictonary of strings mapping to strings";

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

#if defined(PYOS_OS2)
        /* Omit Pseudo-Env Vars that Would Confuse Programs if Passed On */
        if (stricmp(k, "BEGINLIBPATH") != 0 && stricmp(k, "ENDLIBPATH") != 0) {
#endif
		p = PyMem_NEW(char, PyString_Size(key)+PyString_Size(val) + 2);
		if (p == NULL) {
			PyErr_NoMemory();
			goto fail_2;
		}
		sprintf(p, "%s=%s", k, v);
		envlist[envc++] = p;
#if defined(PYOS_OS2)
    }
#endif
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
static char posix_fork__doc__[] =
"fork() -> pid\n\
Fork a child process.\n\
\n\
Return 0 to child process and PID of child to parent process.";

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
	PyOS_AfterFork();
	return PyInt_FromLong((long)pid);
}
#endif


#ifdef HAVE_GETEGID
static char posix_getegid__doc__[] =
"getegid() -> egid\n\
Return the current process's effective group id.";

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
static char posix_geteuid__doc__[] =
"geteuid() -> euid\n\
Return the current process's effective user id.";

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
static char posix_getgid__doc__[] =
"getgid() -> gid\n\
Return the current process's group id.";

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


static char posix_getpid__doc__[] =
"getpid() -> pid\n\
Return the current process id";

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
static char posix_getpgrp__doc__[] =
"getpgrp() -> pgrp\n\
Return the current process group id.";

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
static char posix_setpgrp__doc__[] =
"setpgrp() -> None\n\
Make this process a session leader.";

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
static char posix_getppid__doc__[] =
"getppid() -> ppid\n\
Return the parent's process id.";

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
static char posix_getuid__doc__[] =
"getuid() -> uid\n\
Return the current process's user id.";

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
static char posix_kill__doc__[] =
"kill(pid, sig) -> None\n\
Kill a process with a signal.";

static PyObject *
posix_kill(self, args)
	PyObject *self;
	PyObject *args;
{
	int pid, sig;
	if (!PyArg_Parse(args, "(ii)", &pid, &sig))
		return NULL;
#if defined(PYOS_OS2)
    if (sig == XCPT_SIGNAL_INTR || sig == XCPT_SIGNAL_BREAK) {
        APIRET rc;
        if ((rc = DosSendSignalException(pid, sig)) != NO_ERROR)
            return os2_error(rc);

    } else if (sig == XCPT_SIGNAL_KILLPROC) {
        APIRET rc;
        if ((rc = DosKillProcess(DKP_PROCESS, pid)) != NO_ERROR)
            return os2_error(rc);

    } else
        return NULL; /* Unrecognized Signal Requested */
#else
	if (kill(pid, sig) == -1)
		return posix_error();
#endif
	Py_INCREF(Py_None);
	return Py_None;
}
#endif

#ifdef HAVE_PLOCK

#ifdef HAVE_SYS_LOCK_H
#include <sys/lock.h>
#endif

static char posix_plock__doc__[] =
"plock(op) -> None\n\
Lock program segments into memory.";

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
static char posix_popen__doc__[] =
"popen(command [, mode='r' [, bufsize]]) -> pipe\n\
Open a pipe to/from a command returning a file object.";

#if defined(PYOS_OS2)
static int
async_system(const char *command)
{
    char        *p, errormsg[256], args[1024];
    RESULTCODES  rcodes;
    APIRET       rc;
    char        *shell = getenv("COMSPEC");
    if (!shell)
        shell = "cmd";

    strcpy(args, shell);
    p = &args[ strlen(args)+1 ];
    strcpy(p, "/c ");
    strcat(p, command);
    p += strlen(p) + 1;
    *p = '\0';

    rc = DosExecPgm(errormsg, sizeof(errormsg),
                    EXEC_ASYNC, /* Execute Async w/o Wait for Results */
                    args,
                    NULL,       /* Inherit Parent's Environment */
                    &rcodes, shell);
    return rc;
}

static FILE *
popen(const char *command, const char *mode, int pipesize, int *err)
{
    HFILE    rhan, whan;
    FILE    *retfd = NULL;
    APIRET   rc = DosCreatePipe(&rhan, &whan, pipesize);

    if (rc != NO_ERROR) {
	*err = rc;
        return NULL; /* ERROR - Unable to Create Anon Pipe */
    }

    if (strchr(mode, 'r') != NULL) { /* Treat Command as a Data Source */
        int oldfd = dup(1);      /* Save STDOUT Handle in Another Handle */

        DosEnterCritSec();      /* Stop Other Threads While Changing Handles */
        close(1);                /* Make STDOUT Available for Reallocation */

        if (dup2(whan, 1) == 0) {      /* Connect STDOUT to Pipe Write Side */
            DosClose(whan);            /* Close Now-Unused Pipe Write Handle */

            if (async_system(command) == NO_ERROR)
                retfd = fdopen(rhan, mode); /* And Return Pipe Read Handle */
        }

        dup2(oldfd, 1);          /* Reconnect STDOUT to Original Handle */
        DosExitCritSec();        /* Now Allow Other Threads to Run */

        close(oldfd);            /* And Close Saved STDOUT Handle */
        return retfd;            /* Return fd of Pipe or NULL if Error */

    } else if (strchr(mode, 'w')) { /* Treat Command as a Data Sink */
        int oldfd = dup(0);      /* Save STDIN Handle in Another Handle */

        DosEnterCritSec();      /* Stop Other Threads While Changing Handles */
        close(0);                /* Make STDIN Available for Reallocation */

        if (dup2(rhan, 0) == 0)     { /* Connect STDIN to Pipe Read Side */
            DosClose(rhan);           /* Close Now-Unused Pipe Read Handle */

            if (async_system(command) == NO_ERROR)
                retfd = fdopen(whan, mode); /* And Return Pipe Write Handle */
        }

        dup2(oldfd, 0);          /* Reconnect STDIN to Original Handle */
        DosExitCritSec();        /* Now Allow Other Threads to Run */

        close(oldfd);            /* And Close Saved STDIN Handle */
        return retfd;            /* Return fd of Pipe or NULL if Error */

    } else {
	*err = ERROR_INVALID_ACCESS;
        return NULL; /* ERROR - Invalid Mode (Neither Read nor Write) */
    }
}

static PyObject *
posix_popen(self, args)
	PyObject *self;
	PyObject *args;
{
	char *name;
	char *mode = "r";
	int   err, bufsize = -1;
	FILE *fp;
	PyObject *f;
	if (!PyArg_ParseTuple(args, "s|si", &name, &mode, &bufsize))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	fp = popen(name, mode, (bufsize > 0) ? bufsize : 4096, &err);
	Py_END_ALLOW_THREADS
	if (fp == NULL)
		return os2_error(err);

	f = PyFile_FromFile(fp, name, mode, fclose);
	if (f != NULL)
		PyFile_SetBufSize(f, bufsize);
	return f;
}

#else
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
#endif

#endif /* HAVE_POPEN */


#ifdef HAVE_SETUID
static char posix_setuid__doc__[] =
"setuid(uid) -> None\n\
Set the current process's user id.";
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
static char posix_setgid__doc__[] =
"setgid(gid) -> None\n\
Set the current process's group id.";

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
static char posix_waitpid__doc__[] =
"waitpid(pid, options) -> (pid, status)\n\
Wait for completion of a give child process.";

static PyObject *
posix_waitpid(self, args)
	PyObject *self;
	PyObject *args;
{
	int pid, options, sts = 0;
	if (!PyArg_Parse(args, "(ii)", &pid, &options))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
#ifdef NeXT
	pid = wait4(pid, (union wait *)&sts, options, NULL);
#else
	pid = waitpid(pid, &sts, options);
#endif
	Py_END_ALLOW_THREADS
	if (pid == -1)
		return posix_error();
	else
		return Py_BuildValue("ii", pid, sts);
}
#endif /* HAVE_WAITPID */


#ifdef HAVE_WAIT
static char posix_wait__doc__[] =
"wait() -> (pid, status)\n\
Wait for completion of a child process.";

static PyObject *
posix_wait(self, args)
	PyObject *self;
	PyObject *args;
{
	int pid, sts;
	Py_BEGIN_ALLOW_THREADS
#ifdef NeXT
	pid = wait((union wait *)&sts);
#else
	pid = wait(&sts);
#endif
	Py_END_ALLOW_THREADS
	if (pid == -1)
		return posix_error();
	else
		return Py_BuildValue("ii", pid, sts);
}
#endif


static char posix_lstat__doc__[] =
"lstat(path) -> (mode,ino,dev,nlink,uid,gid,size,atime,mtime,ctime)\n\
Like stat(path), but do not follow symbolic links.";

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
static char posix_readlink__doc__[] =
"readlink(path) -> path\n\
Return a string representing the path to which the symbolic link points.";

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
		return posix_error_with_filename(path);
	return PyString_FromStringAndSize(buf, n);
}
#endif /* HAVE_READLINK */


#ifdef HAVE_SYMLINK
static char posix_symlink__doc__[] =
"symlink(src, dst) -> None\n\
Create a symbolic link.";

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
	
#if defined(PYCC_VACPP) && defined(PYOS_OS2)
static long
system_uptime()
{
    ULONG     value = 0;

    Py_BEGIN_ALLOW_THREADS
    DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &value, sizeof(value));
    Py_END_ALLOW_THREADS

    return value;
}

static PyObject *
posix_times(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;

    /* Currently Only Uptime is Provided -- Others Later */
	return Py_BuildValue("ddddd",
			     (double)0 /* t.tms_utime / HZ */,
			     (double)0 /* t.tms_stime / HZ */,
			     (double)0 /* t.tms_cutime / HZ */,
			     (double)0 /* t.tms_cstime / HZ */,
			     (double)system_uptime() / 1000);
}
#else /* not OS2 */
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
#endif /* not OS2 */
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

#ifdef HAVE_TIMES
static char posix_times__doc__[] =
"times() -> (utime, stime, cutime, cstime, elapsed_time)\n\
Return a tuple of floating point numbers indicating process times.";
#endif


#ifdef HAVE_SETSID
static char posix_setsid__doc__[] =
"setsid() -> None\n\
Call the system call setsid().";

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
static char posix_setpgid__doc__[] =
"setpgid(pid, pgrp) -> None\n\
Call the system call setpgid().";

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
static char posix_tcgetpgrp__doc__[] =
"tcgetpgrp(fd) -> pgid\n\
Return the process group associated with the terminal given by a fd.";

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
static char posix_tcsetpgrp__doc__[] =
"tcsetpgrp(fd, pgid) -> None\n\
Set the process group associated with the terminal given by a fd.";

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

static char posix_open__doc__[] =
"open(filename, flag [, mode=0777]) -> fd\n\
Open a file (for low level IO).";

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
		return posix_error_with_filename(file);
	return PyInt_FromLong((long)fd);
}


static char posix_close__doc__[] =
"close(fd) -> None\n\
Close a file descriptor (for low level IO).";

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


static char posix_dup__doc__[] =
"dup(fd) -> fd2\n\
Return a duplicate of a file descriptor.";

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


static char posix_dup2__doc__[] =
"dup2(fd, fd2) -> None\n\
Duplicate file descriptor.";

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


static char posix_lseek__doc__[] =
"lseek(fd, pos, how) -> newpos\n\
Set the current position of a file descriptor.";

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


static char posix_read__doc__[] =
"read(fd, buffersize) -> string\n\
Read a file descriptor.";

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


static char posix_write__doc__[] =
"write(fd, string) -> byteswritten\n\
Write a string to a file descriptor.";

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


static char posix_fstat__doc__[]=
"fstat(fd) -> (mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime)\n\
Like stat(), but for an open file descriptor.";

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


static char posix_fdopen__doc__[] =
"fdopen(fd, [, mode='r' [, bufsize]]) -> file_object\n\
Return an open file object connected to a file descriptor.";

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
static char posix_pipe__doc__[] =
"pipe() -> (read_end, write_end)\n\
Create a pipe.";

static PyObject *
posix_pipe(self, args)
	PyObject *self;
	PyObject *args;
{
#if defined(PYOS_OS2)
    HFILE read, write;
    APIRET rc;

    if (!PyArg_Parse(args, ""))
        return NULL;

	Py_BEGIN_ALLOW_THREADS
    rc = DosCreatePipe( &read, &write, 4096);
	Py_END_ALLOW_THREADS
    if (rc != NO_ERROR)
        return os2_error(rc);

    return Py_BuildValue("(ii)", read, write);
#else
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
	int read_fd, write_fd;
	BOOL ok;
	if (!PyArg_Parse(args, ""))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	ok = CreatePipe(&read, &write, NULL, 0);
	Py_END_ALLOW_THREADS
	if (!ok)
		return posix_error();
	read_fd = _open_osfhandle((long)read, 0);
	write_fd = _open_osfhandle((long)write, 1);
	return Py_BuildValue("(ii)", read_fd, write_fd);
#endif /* MS_WIN32 */
#endif
}
#endif  /* HAVE_PIPE */


#ifdef HAVE_MKFIFO
static char posix_mkfifo__doc__[] =
"mkfifo(file, [, mode=0666]) -> None\n\
Create a FIFO (a POSIX named pipe).";

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
static char posix_ftruncate__doc__[] =
"ftruncate(fd, length) -> None\n\
Truncate a file to a specified length.";

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

#ifdef NeXT
#define HAVE_PUTENV
/* Steve Spicklemire got this putenv from NeXTAnswers */
static int
putenv(char *newval)
{
	extern char **environ;

	static int firstTime = 1;
	char **ep;
	char *cp;
	int esiz;
	char *np;

	if (!(np = strchr(newval, '=')))
		return 1;
	*np = '\0';

	/* look it up */
	for (ep=environ ; *ep ; ep++)
	{
		/* this should always be true... */
		if (cp = strchr(*ep, '='))
		{
			*cp = '\0';
			if (!strcmp(*ep, newval))
			{
				/* got it! */
				*cp = '=';
				break;
			}
			*cp = '=';
		}
		else
		{
			*np = '=';
			return 1;
		}
	}

	*np = '=';
	if (*ep)
	{
		/* the string was already there:
		   just replace it with the new one */
		*ep = newval;
		return 0;
	}

	/* expand environ by one */
	for (esiz=2, ep=environ ; *ep ; ep++)
		esiz++;
	if (firstTime)
	{
		char **epp;
		char **newenv;
		if (!(newenv = malloc(esiz * sizeof(char *))))
			return 1;
   
		for (ep=environ, epp=newenv ; *ep ;)
			*epp++ = *ep++;
		*epp++ = newval;
		*epp = (char *) 0;
		environ = newenv;
	}
	else
	{
		if (!(environ = realloc(environ, esiz * sizeof(char *))))
			return 1;
		environ[esiz - 2] = newval;
		environ[esiz - 1] = (char *) 0;
		firstTime = 0;
	}

	return 0;
}
#endif /* NeXT */


#ifdef HAVE_PUTENV
static char posix_putenv__doc__[] =
"putenv(key, value) -> None\n\
Change or add an environment variable.";

#ifdef __BEOS__
/* We have putenv(), but not in the headers (as of PR2). - [cjh] */
int putenv( const char *str );
#endif

static PyObject * 
posix_putenv(self, args)
	PyObject *self;
	PyObject *args;
{
        char *s1, *s2;
        char *new;

	if (!PyArg_ParseTuple(args, "ss", &s1, &s2))
		return NULL;

#if defined(PYOS_OS2)
    if (stricmp(s1, "BEGINLIBPATH") == 0) {
        APIRET rc;

        if (strlen(s2) == 0)  /* If New Value is an Empty String */
            s2 = NULL;        /* Then OS/2 API Wants a NULL to Undefine It */

        rc = DosSetExtLIBPATH(s2, BEGIN_LIBPATH);
        if (rc != NO_ERROR)
            return os2_error(rc);

    } else if (stricmp(s1, "ENDLIBPATH") == 0) {
        APIRET rc;

        if (strlen(s2) == 0)  /* If New Value is an Empty String */
            s2 = NULL;        /* Then OS/2 API Wants a NULL to Undefine It */

        rc = DosSetExtLIBPATH(s2, END_LIBPATH);
        if (rc != NO_ERROR)
            return os2_error(rc);
    } else {
#endif

	/* XXX This leaks memory -- not easy to fix :-( */
	if ((new = malloc(strlen(s1) + strlen(s2) + 2)) == NULL)
                return PyErr_NoMemory();
	(void) sprintf(new, "%s=%s", s1, s2);
	if (putenv(new)) {
                posix_error();
                return NULL;
	}

#if defined(PYOS_OS2)
    }
#endif
	Py_INCREF(Py_None);
        return Py_None;
}
#endif /* putenv */

#ifdef HAVE_STRERROR
static char posix_strerror__doc__[] =
"strerror(code) -> string\n\
Translate an error code to a message string.";

PyObject *
posix_strerror(self, args)
	PyObject *self;
	PyObject *args;
{
	int code;
	char *message;
	if (!PyArg_ParseTuple(args, "i", &code))
		return NULL;
	message = strerror(code);
	if (message == NULL) {
		PyErr_SetString(PyExc_ValueError,
				"strerror code out of range");
		return NULL;
	}
	return PyString_FromString(message);
}
#endif /* strerror */


#ifdef HAVE_SYS_WAIT_H

#ifdef WIFSTOPPED
static char posix_WIFSTOPPED__doc__[] =
"WIFSTOPPED(status) -> Boolean\n\
See Unix documentation.";

static PyObject *
posix_WIFSTOPPED(self, args)
	PyObject *self;
	PyObject *args;
{
	int status = 0;
   
	if (!PyArg_Parse(args, "i", &status))
	{
		return NULL;
	}
   
	return Py_BuildValue("i", WIFSTOPPED(status));
}
#endif /* WIFSTOPPED */

#ifdef WIFSIGNALED
static char posix_WIFSIGNALED__doc__[] =
"WIFSIGNALED(status) -> Boolean\n\
See Unix documentation.";

static PyObject *
posix_WIFSIGNALED(self, args)
	PyObject *self;
	PyObject *args;
{
	int status = 0;
   
	if (!PyArg_Parse(args, "i", &status))
	{
		return NULL;
	}
   
	return Py_BuildValue("i", WIFSIGNALED(status));
}
#endif /* WIFSIGNALED */

#ifdef WIFEXITED
static char posix_WIFEXITED__doc__[] =
"WIFEXITED(status) -> Boolean\n\
See Unix documentation.";

static PyObject *
posix_WIFEXITED(self, args)
	PyObject *self;
	PyObject *args;
{
	int status = 0;
   
	if (!PyArg_Parse(args, "i", &status))
	{
		return NULL;
	}
   
	return Py_BuildValue("i", WIFEXITED(status));
}
#endif /* WIFEXITED */

#ifdef WIFSTOPPED
static char posix_WEXITSTATUS__doc__[] =
"WEXITSTATUS(status) -> integer\n\
See Unix documentation.";

static PyObject *
posix_WEXITSTATUS(self, args)
	PyObject *self;
	PyObject *args;
{
	int status = 0;
   
	if (!PyArg_Parse(args, "i", &status))
	{
		return NULL;
	}
   
	return Py_BuildValue("i", WEXITSTATUS(status));
}
#endif /* WEXITSTATUS */

#ifdef WTERMSIG
static char posix_WTERMSIG__doc__[] =
"WTERMSIG(status) -> integer\n\
See Unix documentation.";

static PyObject *
posix_WTERMSIG(self, args)
	PyObject *self;
	PyObject *args;
{
	int status = 0;
   
	if (!PyArg_Parse(args, "i", &status))
	{
		return NULL;
	}
   
	return Py_BuildValue("i", WTERMSIG(status));
}
#endif /* WTERMSIG */

#ifdef WSTOPSIG
static char posix_WSTOPSIG__doc__[] =
"WSTOPSIG(status) -> integer\n\
See Unix documentation.";

static PyObject *
posix_WSTOPSIG(self, args)
	PyObject *self;
	PyObject *args;
{
	int status = 0;
   
	if (!PyArg_Parse(args, "i", &status))
	{
		return NULL;
	}
   
	return Py_BuildValue("i", WSTOPSIG(status));
}
#endif /* WSTOPSIG */

#endif /* HAVE_SYS_WAIT_H */


static PyMethodDef posix_methods[] = {
	{"chdir",	posix_chdir, 0, posix_chdir__doc__},
	{"chmod",	posix_chmod, 0, posix_chmod__doc__},
#ifdef HAVE_CHOWN
	{"chown",	posix_chown, 0, posix_chown__doc__},
#endif /* HAVE_CHOWN */
#ifdef HAVE_GETCWD
	{"getcwd",	posix_getcwd, 0, posix_getcwd__doc__},
#endif
#ifdef HAVE_LINK
	{"link",	posix_link, 0, posix_link__doc__},
#endif /* HAVE_LINK */
	{"listdir",	posix_listdir, 0, posix_listdir__doc__},
	{"lstat",	posix_lstat, 0, posix_lstat__doc__},
	{"mkdir",	posix_mkdir, 1, posix_mkdir__doc__},
#ifdef HAVE_NICE
	{"nice",	posix_nice, 0, posix_nice__doc__},
#endif /* HAVE_NICE */
#ifdef HAVE_READLINK
	{"readlink",	posix_readlink, 0, posix_readlink__doc__},
#endif /* HAVE_READLINK */
	{"rename",	posix_rename, 0, posix_rename__doc__},
	{"rmdir",	posix_rmdir, 0, posix_rmdir__doc__},
	{"stat",	posix_stat, 0, posix_stat__doc__},
#ifdef HAVE_SYMLINK
	{"symlink",	posix_symlink, 0, posix_symlink__doc__},
#endif /* HAVE_SYMLINK */
#ifdef HAVE_SYSTEM
	{"system",	posix_system, 0, posix_system__doc__},
#endif
	{"umask",	posix_umask, 0, posix_umask__doc__},
#ifdef HAVE_UNAME
	{"uname",	posix_uname, 0, posix_uname__doc__},
#endif /* HAVE_UNAME */
	{"unlink",	posix_unlink, 0, posix_unlink__doc__},
	{"remove",	posix_unlink, 0, posix_remove__doc__},
	{"utime",	posix_utime, 0, posix_utime__doc__},
#ifdef HAVE_TIMES
	{"times",	posix_times, 0, posix_times__doc__},
#endif /* HAVE_TIMES */
	{"_exit",	posix__exit, 0, posix__exit__doc__},
#ifdef HAVE_EXECV
	{"execv",	posix_execv, 0, posix_execv__doc__},
	{"execve",	posix_execve, 0, posix_execve__doc__},
#endif /* HAVE_EXECV */
#ifdef HAVE_FORK
	{"fork",	posix_fork, 0, posix_fork__doc__},
#endif /* HAVE_FORK */
#ifdef HAVE_GETEGID
	{"getegid",	posix_getegid, 0, posix_getegid__doc__},
#endif /* HAVE_GETEGID */
#ifdef HAVE_GETEUID
	{"geteuid",	posix_geteuid, 0, posix_geteuid__doc__},
#endif /* HAVE_GETEUID */
#ifdef HAVE_GETGID
	{"getgid",	posix_getgid, 0, posix_getgid__doc__},
#endif /* HAVE_GETGID */
	{"getpid",	posix_getpid, 0, posix_getpid__doc__},
#ifdef HAVE_GETPGRP
	{"getpgrp",	posix_getpgrp, 0, posix_getpgrp__doc__},
#endif /* HAVE_GETPGRP */
#ifdef HAVE_GETPPID
	{"getppid",	posix_getppid, 0, posix_getppid__doc__},
#endif /* HAVE_GETPPID */
#ifdef HAVE_GETUID
	{"getuid",	posix_getuid, 0, posix_getuid__doc__},
#endif /* HAVE_GETUID */
#ifdef HAVE_KILL
	{"kill",	posix_kill, 0, posix_kill__doc__},
#endif /* HAVE_KILL */
#ifdef HAVE_PLOCK
	{"plock",	posix_plock, 0, posix_plock__doc__},
#endif /* HAVE_PLOCK */
#ifdef HAVE_POPEN
	{"popen",	posix_popen, 1, posix_popen__doc__},
#endif /* HAVE_POPEN */
#ifdef HAVE_SETUID
	{"setuid",	posix_setuid, 0, posix_setuid__doc__},
#endif /* HAVE_SETUID */
#ifdef HAVE_SETGID
	{"setgid",	posix_setgid, 0, posix_setgid__doc__},
#endif /* HAVE_SETGID */
#ifdef HAVE_SETPGRP
	{"setpgrp",	posix_setpgrp, 0, posix_setpgrp__doc__},
#endif /* HAVE_SETPGRP */
#ifdef HAVE_WAIT
	{"wait",	posix_wait, 0, posix_wait__doc__},
#endif /* HAVE_WAIT */
#ifdef HAVE_WAITPID
	{"waitpid",	posix_waitpid, 0, posix_waitpid__doc__},
#endif /* HAVE_WAITPID */
#ifdef HAVE_SETSID
	{"setsid",	posix_setsid, 0, posix_setsid__doc__},
#endif /* HAVE_SETSID */
#ifdef HAVE_SETPGID
	{"setpgid",	posix_setpgid, 0, posix_setpgid__doc__},
#endif /* HAVE_SETPGID */
#ifdef HAVE_TCGETPGRP
	{"tcgetpgrp",	posix_tcgetpgrp, 0, posix_tcgetpgrp__doc__},
#endif /* HAVE_TCGETPGRP */
#ifdef HAVE_TCSETPGRP
	{"tcsetpgrp",	posix_tcsetpgrp, 0, posix_tcsetpgrp__doc__},
#endif /* HAVE_TCSETPGRP */
	{"open",	posix_open, 1, posix_open__doc__},
	{"close",	posix_close, 0, posix_close__doc__},
	{"dup",		posix_dup, 0, posix_dup__doc__},
	{"dup2",	posix_dup2, 0, posix_dup2__doc__},
	{"lseek",	posix_lseek, 0, posix_lseek__doc__},
	{"read",	posix_read, 0, posix_read__doc__},
	{"write",	posix_write, 0, posix_write__doc__},
	{"fstat",	posix_fstat, 0, posix_fstat__doc__},
	{"fdopen",	posix_fdopen,	1, posix_fdopen__doc__},
#ifdef HAVE_PIPE
	{"pipe",	posix_pipe, 0, posix_pipe__doc__},
#endif
#ifdef HAVE_MKFIFO
	{"mkfifo",	posix_mkfifo, 1, posix_mkfifo__doc__},
#endif
#ifdef HAVE_FTRUNCATE
	{"ftruncate",	posix_ftruncate, 1, posix_ftruncate__doc__},
#endif
#ifdef HAVE_PUTENV
	{"putenv",	posix_putenv, 1, posix_putenv__doc__},
#endif
#ifdef HAVE_STRERROR
	{"strerror",	posix_strerror, 1, posix_strerror__doc__},
#endif
#ifdef HAVE_SYS_WAIT_H
#ifdef WIFSTOPPED
        {"WIFSTOPPED",	posix_WIFSTOPPED, 0, posix_WIFSTOPPED__doc__},
#endif /* WIFSTOPPED */
#ifdef WIFSIGNALED
        {"WIFSIGNALED",	posix_WIFSIGNALED, 0, posix_WIFSIGNALED__doc__},
#endif /* WIFSIGNALED */
#ifdef WIFEXITED
        {"WIFEXITED",	posix_WIFEXITED, 0, posix_WIFEXITED__doc__},
#endif /* WIFEXITED */
#ifdef WEXITSTATUS
        {"WEXITSTATUS",	posix_WEXITSTATUS, 0, posix_WEXITSTATUS__doc__},
#endif /* WEXITSTATUS */
#ifdef WTERMSIG
        {"WTERMSIG",	posix_WTERMSIG, 0, posix_WTERMSIG__doc__},
#endif /* WTERMSIG */
#ifdef WSTOPSIG
        {"WSTOPSIG",	posix_WSTOPSIG, 0, posix_WSTOPSIG__doc__},
#endif /* WSTOPSIG */
#endif /* HAVE_SYS_WAIT_H */
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

#if defined(PYOS_OS2)
/* Insert Platform-Specific Constant Values (Strings & Numbers) of Common Use */
static int insertvalues(PyObject *d)
{
    APIRET    rc;
    ULONG     values[QSV_MAX+1];
    PyObject *v;
    char     *ver, tmp[10];

    Py_BEGIN_ALLOW_THREADS
    rc = DosQuerySysInfo(1, QSV_MAX, &values[1], sizeof(values));
    Py_END_ALLOW_THREADS

    if (rc != NO_ERROR) {
        os2_error(rc);
        return -1;
    }

    if (ins(d, "meminstalled", values[QSV_TOTPHYSMEM])) return -1;
    if (ins(d, "memkernel",    values[QSV_TOTRESMEM])) return -1;
    if (ins(d, "memvirtual",   values[QSV_TOTAVAILMEM])) return -1;
    if (ins(d, "maxpathlen",   values[QSV_MAX_PATH_LENGTH])) return -1;
    if (ins(d, "maxnamelen",   values[QSV_MAX_COMP_LENGTH])) return -1;
    if (ins(d, "revision",     values[QSV_VERSION_REVISION])) return -1;
    if (ins(d, "timeslice",    values[QSV_MIN_SLICE])) return -1;

    switch (values[QSV_VERSION_MINOR]) {
    case 0:  ver = "2.00"; break;
    case 10: ver = "2.10"; break;
    case 11: ver = "2.11"; break;
    case 30: ver = "3.00"; break;
    case 40: ver = "4.00"; break;
    case 50: ver = "5.00"; break;
    default:
        sprintf(tmp, "%d-%d", values[QSV_VERSION_MAJOR],
                              values[QSV_VERSION_MINOR]);
        ver = &tmp[0];
    }

    /* Add Indicator of the Version of the Operating System */
    v = PyString_FromString(ver);
    if (!v || PyDict_SetItemString(d, "version", v) < 0)
        return -1;
    Py_DECREF(v);

    /* Add Indicator of Which Drive was Used to Boot the System */
    tmp[0] = 'A' + values[QSV_BOOT_DRIVE] - 1;
    tmp[1] = ':';
    tmp[2] = '\0';

    v = PyString_FromString(tmp);
    if (!v || PyDict_SetItemString(d, "bootdrive", v) < 0)
        return -1;
    Py_DECREF(v);

    return 0;
}
#endif

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
#ifdef O_BINARY
        if (ins(d, "O_BINARY", (long)O_BINARY)) return -1;
#endif
#ifdef O_TEXT
        if (ins(d, "O_TEXT", (long)O_TEXT)) return -1;
#endif

#if defined(PYOS_OS2)
        if (insertvalues(d)) return -1;
#endif
        return 0;
}


#if ( defined(_MSC_VER) || defined(__WATCOMC__) ) && !defined(__QNX__)
#define INITFUNC initnt
#define MODNAME "nt"
#else
#if defined(PYOS_OS2)
#define INITFUNC initos2
#define MODNAME "os2"
#else
#define INITFUNC initposix
#define MODNAME "posix"
#endif
#endif

void
INITFUNC()
{
	PyObject *m, *d, *v;
	
	m = Py_InitModule4(MODNAME,
			   posix_methods,
			   posix__doc__,
			   (PyObject *)NULL,
			   PYTHON_API_VERSION);
	d = PyModule_GetDict(m);
	
	/* Initialize environ dictionary */
	v = convertenviron();
	if (v == NULL || PyDict_SetItemString(d, "environ", v) != 0)
		return;
	Py_DECREF(v);
	
        if (all_ins(d))
                return;

	Py_INCREF(PyExc_OSError);
	PosixError = PyExc_OSError;
	PyDict_SetItemString(d, "error", PosixError);
}
