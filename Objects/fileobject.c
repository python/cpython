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

/* File object implementation */

#include "Python.h"
#include "structmember.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef MS_WIN32
#define ftruncate _chsize
#define fileno _fileno
#define HAVE_FTRUNCATE
#endif

#ifdef macintosh
#ifdef USE_GUSI
#define HAVE_FTRUNCATE
#endif
#endif

#ifdef THINK_C
#define HAVE_FOPENRF
#endif
#ifdef __MWERKS__
/* Mwerks fopen() doesn't always set errno */
#define NO_FOPEN_ERRNO
#endif

#define BUF(v) PyString_AS_STRING((PyStringObject *)v)

#include <errno.h>

typedef struct {
	PyObject_HEAD
	FILE *f_fp;
	PyObject *f_name;
	PyObject *f_mode;
	int (*f_close) Py_PROTO((FILE *));
	int f_softspace; /* Flag used by 'print' command */
} PyFileObject;

FILE *
PyFile_AsFile(f)
	PyObject *f;
{
	if (f == NULL || !PyFile_Check(f))
		return NULL;
	else
		return ((PyFileObject *)f)->f_fp;
}

PyObject *
PyFile_Name(f)
	PyObject *f;
{
	if (f == NULL || !PyFile_Check(f))
		return NULL;
	else
		return ((PyFileObject *)f)->f_name;
}

PyObject *
PyFile_FromFile(fp, name, mode, close)
	FILE *fp;
	char *name;
	char *mode;
	int (*close) Py_FPROTO((FILE *));
{
	PyFileObject *f = PyObject_NEW(PyFileObject, &PyFile_Type);
	if (f == NULL)
		return NULL;
	f->f_fp = NULL;
	f->f_name = PyString_FromString(name);
	f->f_mode = PyString_FromString(mode);
	f->f_close = close;
	f->f_softspace = 0;
	if (f->f_name == NULL || f->f_mode == NULL) {
		Py_DECREF(f);
		return NULL;
	}
	f->f_fp = fp;
	return (PyObject *) f;
}

PyObject *
PyFile_FromString(name, mode)
	char *name, *mode;
{
	extern int fclose Py_PROTO((FILE *));
	PyFileObject *f;
	f = (PyFileObject *) PyFile_FromFile((FILE *)NULL, name, mode, fclose);
	if (f == NULL)
		return NULL;
#ifdef HAVE_FOPENRF
	if (*mode == '*') {
		FILE *fopenRF();
		f->f_fp = fopenRF(name, mode+1);
	}
	else
#endif
	{
		Py_BEGIN_ALLOW_THREADS
		f->f_fp = fopen(name, mode);
		Py_END_ALLOW_THREADS
	}
	if (f->f_fp == NULL) {
#ifdef NO_FOPEN_ERRNO
		if ( errno == 0 ) {
			PyErr_SetString(PyExc_IOError, "Cannot open file");
			Py_DECREF(f);
			return NULL;
		}
#endif
		PyErr_SetFromErrno(PyExc_IOError);
		Py_DECREF(f);
		return NULL;
	}
	return (PyObject *)f;
}

void
PyFile_SetBufSize(f, bufsize)
	PyObject *f;
	int bufsize;
{
	if (bufsize >= 0) {
#ifdef HAVE_SETVBUF
		int type;
		switch (bufsize) {
		case 0:
			type = _IONBF;
			break;
		case 1:
			type = _IOLBF;
			bufsize = BUFSIZ;
			break;
		default:
			type = _IOFBF;
		}
		setvbuf(((PyFileObject *)f)->f_fp, (char *)NULL,
			type, bufsize);
#else /* !HAVE_SETVBUF */
		if (bufsize <= 1)
			setbuf(((PyFileObject *)f)->f_fp, (char *)NULL);
#endif /* !HAVE_SETVBUF */
	}
}

static PyObject *
err_closed()
{
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed file");
	return NULL;
}

/* Methods */

static void
file_dealloc(f)
	PyFileObject *f;
{
	if (f->f_fp != NULL && f->f_close != NULL) {
		Py_BEGIN_ALLOW_THREADS
		(*f->f_close)(f->f_fp);
		Py_END_ALLOW_THREADS
	}
	if (f->f_name != NULL) {
		Py_DECREF(f->f_name);
	}
	if (f->f_mode != NULL) {
		Py_DECREF(f->f_mode);
	}
	free((char *)f);
}

static PyObject *
file_repr(f)
	PyFileObject *f;
{
	char buf[300];
	sprintf(buf, "<%s file '%.256s', mode '%.10s' at %lx>",
		f->f_fp == NULL ? "closed" : "open",
		PyString_AsString(f->f_name),
		PyString_AsString(f->f_mode),
		(long)f);
	return PyString_FromString(buf);
}

static PyObject *
file_close(f, args)
	PyFileObject *f;
	PyObject *args;
{
	int sts = 0;
	if (!PyArg_NoArgs(args))
		return NULL;
	if (f->f_fp != NULL) {
		if (f->f_close != NULL) {
			Py_BEGIN_ALLOW_THREADS
			errno = 0;
			sts = (*f->f_close)(f->f_fp);
			Py_END_ALLOW_THREADS
		}
		f->f_fp = NULL;
	}
	if (sts == EOF)
		return PyErr_SetFromErrno(PyExc_IOError);
	if (sts != 0)
		return PyInt_FromLong((long)sts);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
file_seek(f, args)
	PyFileObject *f;
	PyObject *args;
{
	long offset;
	int whence;
	int ret;
	
	if (f->f_fp == NULL)
		return err_closed();
	whence = 0;
	if (!PyArg_Parse(args, "l", &offset)) {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(li)", &offset, &whence))
			return NULL;
	}
	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	ret = fseek(f->f_fp, offset, whence);
	Py_END_ALLOW_THREADS
	if (ret != 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef HAVE_FTRUNCATE
static PyObject *
file_truncate(f, args)
	PyFileObject *f;
	PyObject *args;
{
	long newsize;
	int ret;
	
	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_Parse(args, "l", &newsize)) {
		PyErr_Clear();
		if (!PyArg_NoArgs(args))
		        return NULL;
		Py_BEGIN_ALLOW_THREADS
		errno = 0;
		newsize =  ftell(f->f_fp); /* default to current position*/
		Py_END_ALLOW_THREADS
		if (newsize == -1L) {
		        PyErr_SetFromErrno(PyExc_IOError);
			clearerr(f->f_fp);
			return NULL;
		}
	}
	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	ret = fflush(f->f_fp);
	Py_END_ALLOW_THREADS
	if (ret == 0) {
	        Py_BEGIN_ALLOW_THREADS
		errno = 0;
		ret = ftruncate(fileno(f->f_fp), newsize);
		Py_END_ALLOW_THREADS
	}
	if (ret != 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* HAVE_FTRUNCATE */

static PyObject *
file_tell(f, args)
	PyFileObject *f;
	PyObject *args;
{
	long offset;
	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_NoArgs(args))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	offset = ftell(f->f_fp);
	Py_END_ALLOW_THREADS
	if (offset == -1L) {
		PyErr_SetFromErrno(PyExc_IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	return PyInt_FromLong(offset);
}

static PyObject *
file_fileno(f, args)
	PyFileObject *f;
	PyObject *args;
{
	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long) fileno(f->f_fp));
}

static PyObject *
file_flush(f, args)
	PyFileObject *f;
	PyObject *args;
{
	int res;
	
	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_NoArgs(args))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	res = fflush(f->f_fp);
	Py_END_ALLOW_THREADS
	if (res != 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
file_isatty(f, args)
	PyFileObject *f;
	PyObject *args;
{
	long res;
	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_NoArgs(args))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = isatty((int)fileno(f->f_fp));
	Py_END_ALLOW_THREADS
	return PyInt_FromLong(res);
}

/* We expect that fstat exists on most systems.
   It's confirmed on Unix, Mac and Windows.
   If you don't have it, add #define DONT_HAVE_FSTAT to your config.h. */
#ifndef DONT_HAVE_FSTAT
#define HAVE_FSTAT

#include <sys/types.h>
#include <sys/stat.h>

#endif

#if BUFSIZ < 8192
#define SMALLCHUNK 8192
#else
#define SMALLCHUNK BUFSIZ
#endif

#define BIGCHUNK (512*1024)

static size_t
new_buffersize(f, currentsize)
	PyFileObject *f;
	size_t currentsize;
{
#ifdef HAVE_FSTAT
	long pos, end;
	struct stat st;
	if (fstat(fileno(f->f_fp), &st) == 0) {
		end = st.st_size;
		pos = lseek(fileno(f->f_fp), 0L, SEEK_CUR);
		if (pos >= 0)
			pos = ftell(f->f_fp);
		if (pos < 0)
			clearerr(f->f_fp);
		if (end > pos && pos >= 0)
			return end - pos + 1;
		/* Add 1 so if the file were to grow we'd notice. */
	}
#endif
	if (currentsize > SMALLCHUNK) {
		/* Keep doubling until we reach BIGCHUNK;
		   then keep adding BIGCHUNK. */
		if (currentsize <= BIGCHUNK)
			return currentsize + currentsize;
		else
			return currentsize + BIGCHUNK;
	}
	return currentsize + SMALLCHUNK;
}

static PyObject *
file_read(f, args)
	PyFileObject *f;
	PyObject *args;
{
	long bytesrequested = -1;
	size_t bytesread, buffersize, chunksize;
	PyObject *v;
	
	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_ParseTuple(args, "|l", &bytesrequested))
		return NULL;
	if (bytesrequested < 0)
		buffersize = new_buffersize(f, 0);
	else
		buffersize = bytesrequested;
	v = PyString_FromStringAndSize((char *)NULL, buffersize);
	if (v == NULL)
		return NULL;
	bytesread = 0;
	for (;;) {
		Py_BEGIN_ALLOW_THREADS
		errno = 0;
		chunksize = fread(BUF(v) + bytesread, 1,
				  buffersize - bytesread, f->f_fp);
		Py_END_ALLOW_THREADS
		if (chunksize == 0) {
			if (!ferror(f->f_fp))
				break;
			PyErr_SetFromErrno(PyExc_IOError);
			clearerr(f->f_fp);
			Py_DECREF(v);
			return NULL;
		}
		bytesread += chunksize;
		if (bytesread < buffersize)
			break;
		if (bytesrequested < 0) {
			buffersize = bytesread + new_buffersize(f, buffersize);
			if (_PyString_Resize(&v, buffersize) < 0)
				return NULL;
		}
	}
	if (bytesread != buffersize)
		_PyString_Resize(&v, bytesread);
	return v;
}

static PyObject *
file_readinto(f, args)
	PyFileObject *f;
	PyObject *args;
{
	char *ptr;
	int ntodo, ndone, nnow;
	
	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_Parse(args, "w#", &ptr, &ntodo))
		return NULL;
	ndone = 0;
	while (ntodo > 0) {
		Py_BEGIN_ALLOW_THREADS
		errno = 0;
		nnow = fread(ptr+ndone, 1, ntodo, f->f_fp);
		Py_END_ALLOW_THREADS
		if (nnow == 0) {
			if (!ferror(f->f_fp))
				break;
			PyErr_SetFromErrno(PyExc_IOError);
			clearerr(f->f_fp);
			return NULL;
		}
		ndone += nnow;
		ntodo -= nnow;
	}
	return PyInt_FromLong(ndone);
}


/* Internal routine to get a line.
   Size argument interpretation:
   > 0: max length;
   = 0: read arbitrary line;
   < 0: strip trailing '\n', raise EOFError if EOF reached immediately
*/

static PyObject *
getline(f, n)
	PyFileObject *f;
	int n;
{
	register FILE *fp;
	register int c;
	register char *buf, *end;
	int n1, n2;
	PyObject *v;

	fp = f->f_fp;
	n2 = n > 0 ? n : 100;
	v = PyString_FromStringAndSize((char *)NULL, n2);
	if (v == NULL)
		return NULL;
	buf = BUF(v);
	end = buf + n2;

	Py_BEGIN_ALLOW_THREADS
	for (;;) {
		if ((c = getc(fp)) == EOF) {
			clearerr(fp);
			Py_BLOCK_THREADS
			if (PyErr_CheckSignals()) {
				Py_DECREF(v);
				return NULL;
			}
			if (n < 0 && buf == BUF(v)) {
				Py_DECREF(v);
				PyErr_SetString(PyExc_EOFError,
					   "EOF when reading a line");
				return NULL;
			}
			Py_UNBLOCK_THREADS
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
			Py_BLOCK_THREADS
			if (_PyString_Resize(&v, n2) < 0)
				return NULL;
			Py_UNBLOCK_THREADS
			buf = BUF(v) + n1;
			end = BUF(v) + n2;
		}
	}
	Py_END_ALLOW_THREADS

	n1 = buf - BUF(v);
	if (n1 != n2)
		_PyString_Resize(&v, n1);
	return v;
}

/* External C interface */

PyObject *
PyFile_GetLine(f, n)
	PyObject *f;
	int n;
{
	if (f == NULL) {
		PyErr_BadInternalCall();
		return NULL;
	}
	if (!PyFile_Check(f)) {
		PyObject *reader;
		PyObject *args;
		PyObject *result;
		reader = PyObject_GetAttrString(f, "readline");
		if (reader == NULL)
			return NULL;
		if (n <= 0)
			args = Py_BuildValue("()");
		else
			args = Py_BuildValue("(i)", n);
		if (args == NULL) {
			Py_DECREF(reader);
			return NULL;
		}
		result = PyEval_CallObject(reader, args);
		Py_DECREF(reader);
		Py_DECREF(args);
		if (result != NULL && !PyString_Check(result)) {
			Py_DECREF(result);
			result = NULL;
			PyErr_SetString(PyExc_TypeError,
				   "object.readline() returned non-string");
		}
		if (n < 0 && result != NULL) {
			char *s = PyString_AsString(result);
			int len = PyString_Size(result);
			if (len == 0) {
				Py_DECREF(result);
				result = NULL;
				PyErr_SetString(PyExc_EOFError,
					   "EOF when reading a line");
			}
			else if (s[len-1] == '\n') {
				if (result->ob_refcnt == 1)
					_PyString_Resize(&result, len-1);
				else {
					PyObject *v;
					v = PyString_FromStringAndSize(s,
								       len-1);
					Py_DECREF(result);
					result = v;
				}
			}
		}
		return result;
	}
	if (((PyFileObject*)f)->f_fp == NULL)
		return err_closed();
	return getline((PyFileObject *)f, n);
}

/* Python method */

static PyObject *
file_readline(f, args)
	PyFileObject *f;
	PyObject *args;
{
	int n = -1;

	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_ParseTuple(args, "|i", &n))
		return NULL;
	if (n == 0)
		return PyString_FromString("");
	if (n < 0)
		n = 0;
	return getline(f, n);
}

static PyObject *
file_readlines(f, args)
	PyFileObject *f;
	PyObject *args;
{
	long sizehint = 0;
	PyObject *list;
	PyObject *line;
	char small_buffer[SMALLCHUNK];
	char *buffer = small_buffer;
	size_t buffersize = SMALLCHUNK;
	PyObject *big_buffer = NULL;
	size_t nfilled = 0;
	size_t nread;
	size_t totalread = 0;
	char *p, *q, *end;
	int err;

	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_ParseTuple(args, "|l", &sizehint))
		return NULL;
	if ((list = PyList_New(0)) == NULL)
		return NULL;
	for (;;) {
		Py_BEGIN_ALLOW_THREADS
		errno = 0;
		nread = fread(buffer+nfilled, 1, buffersize-nfilled, f->f_fp);
		Py_END_ALLOW_THREADS
		if (nread == 0) {
			sizehint = 0;
			if (!ferror(f->f_fp))
				break;
			PyErr_SetFromErrno(PyExc_IOError);
			clearerr(f->f_fp);
		  error:
			Py_DECREF(list);
			list = NULL;
			goto cleanup;
		}
		totalread += nread;
		p = memchr(buffer+nfilled, '\n', nread);
		if (p == NULL) {
			/* Need a larger buffer to fit this line */
			nfilled += nread;
			buffersize *= 2;
			if (big_buffer == NULL) {
				/* Create the big buffer */
				big_buffer = PyString_FromStringAndSize(
					NULL, buffersize);
				if (big_buffer == NULL)
					goto error;
				buffer = PyString_AS_STRING(big_buffer);
				memcpy(buffer, small_buffer, nfilled);
			}
			else {
				/* Grow the big buffer */
				_PyString_Resize(&big_buffer, buffersize);
				buffer = PyString_AS_STRING(big_buffer);
			}
			continue;
		}
		end = buffer+nfilled+nread;
		q = buffer;
		do {
			/* Process complete lines */
			p++;
			line = PyString_FromStringAndSize(q, p-q);
			if (line == NULL)
				goto error;
			err = PyList_Append(list, line);
			Py_DECREF(line);
			if (err != 0)
				goto error;
			q = p;
			p = memchr(q, '\n', end-q);
		} while (p != NULL);
		/* Move the remaining incomplete line to the start */
		nfilled = end-q;
		memmove(buffer, q, nfilled);
		if (sizehint > 0)
			if (totalread >= (size_t)sizehint)
				break;
	}
	if (nfilled != 0) {
		/* Partial last line */
		line = PyString_FromStringAndSize(buffer, nfilled);
		if (line == NULL)
			goto error;
		if (sizehint > 0) {
			/* Need to complete the last line */
			PyObject *rest = getline(f, 0);
			if (rest == NULL) {
				Py_DECREF(line);
				goto error;
			}
			PyString_Concat(&line, rest);
			Py_DECREF(rest);
			if (line == NULL)
				goto error;
		}
		err = PyList_Append(list, line);
		Py_DECREF(line);
		if (err != 0)
			goto error;
	}
  cleanup:
	if (big_buffer) {
		Py_DECREF(big_buffer);
	}
	return list;
}

static PyObject *
file_write(f, args)
	PyFileObject *f;
	PyObject *args;
{
	char *s;
	int n, n2;
	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_Parse(args, "s#", &s, &n))
		return NULL;
	f->f_softspace = 0;
	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	n2 = fwrite(s, 1, n, f->f_fp);
	Py_END_ALLOW_THREADS
	if (n2 != n) {
		PyErr_SetFromErrno(PyExc_IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
file_writelines(f, args)
	PyFileObject *f;
	PyObject *args;
{
	int i, n;
	if (f->f_fp == NULL)
		return err_closed();
	if (args == NULL || !PyList_Check(args)) {
		PyErr_SetString(PyExc_TypeError,
			   "writelines() requires list of strings");
		return NULL;
	}
	n = PyList_Size(args);
	f->f_softspace = 0;
	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	for (i = 0; i < n; i++) {
		PyObject *line = PyList_GetItem(args, i);
		int len;
		int nwritten;
		if (!PyString_Check(line)) {
			Py_BLOCK_THREADS
			PyErr_SetString(PyExc_TypeError,
				   "writelines() requires list of strings");
			return NULL;
		}
		len = PyString_Size(line);
		nwritten = fwrite(PyString_AsString(line), 1, len, f->f_fp);
		if (nwritten != len) {
			Py_BLOCK_THREADS
			PyErr_SetFromErrno(PyExc_IOError);
			clearerr(f->f_fp);
			return NULL;
		}
	}
	Py_END_ALLOW_THREADS
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef file_methods[] = {
	{"readline",	(PyCFunction)file_readline, 1},
	{"read",	(PyCFunction)file_read, 1},
	{"write",	(PyCFunction)file_write, 0},
	{"fileno",	(PyCFunction)file_fileno, 0},
	{"seek",	(PyCFunction)file_seek, 0},
#ifdef HAVE_FTRUNCATE
	{"truncate",	(PyCFunction)file_truncate, 0},
#endif
	{"tell",	(PyCFunction)file_tell, 0},
	{"readinto",	(PyCFunction)file_readinto, 0},
	{"readlines",	(PyCFunction)file_readlines, 1},
	{"writelines",	(PyCFunction)file_writelines, 0},
	{"flush",	(PyCFunction)file_flush, 0},
	{"close",	(PyCFunction)file_close, 0},
	{"isatty",	(PyCFunction)file_isatty, 0},
	{NULL,		NULL}		/* sentinel */
};

#define OFF(x) offsetof(PyFileObject, x)

static struct memberlist file_memberlist[] = {
	{"softspace",	T_INT,		OFF(f_softspace)},
	{"mode",	T_OBJECT,	OFF(f_mode),	RO},
	{"name",	T_OBJECT,	OFF(f_name),	RO},
	/* getattr(f, "closed") is implemented without this table */
	{"closed",	T_INT,		0,		RO},
	{NULL}	/* Sentinel */
};

static PyObject *
file_getattr(f, name)
	PyFileObject *f;
	char *name;
{
	PyObject *res;

	res = Py_FindMethod(file_methods, (PyObject *)f, name);
	if (res != NULL)
		return res;
	PyErr_Clear();
	if (strcmp(name, "closed") == 0)
		return PyInt_FromLong((long)(f->f_fp == 0));
	return PyMember_Get((char *)f, file_memberlist, name);
}

static int
file_setattr(f, name, v)
	PyFileObject *f;
	char *name;
	PyObject *v;
{
	if (v == NULL) {
		PyErr_SetString(PyExc_AttributeError,
				"can't delete file attributes");
		return -1;
	}
	return PyMember_Set((char *)f, file_memberlist, name, v);
}

PyTypeObject PyFile_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"file",
	sizeof(PyFileObject),
	0,
	(destructor)file_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	(getattrfunc)file_getattr, /*tp_getattr*/
	(setattrfunc)file_setattr, /*tp_setattr*/
	0,		/*tp_compare*/
	(reprfunc)file_repr, /*tp_repr*/
};

/* Interface for the 'soft space' between print items. */

int
PyFile_SoftSpace(f, newflag)
	PyObject *f;
	int newflag;
{
	int oldflag = 0;
	if (f == NULL) {
		/* Do nothing */
	}
	else if (PyFile_Check(f)) {
		oldflag = ((PyFileObject *)f)->f_softspace;
		((PyFileObject *)f)->f_softspace = newflag;
	}
	else {
		PyObject *v;
		v = PyObject_GetAttrString(f, "softspace");
		if (v == NULL)
			PyErr_Clear();
		else {
			if (PyInt_Check(v))
				oldflag = PyInt_AsLong(v);
			Py_DECREF(v);
		}
		v = PyInt_FromLong((long)newflag);
		if (v == NULL)
			PyErr_Clear();
		else {
			if (PyObject_SetAttrString(f, "softspace", v) != 0)
				PyErr_Clear();
			Py_DECREF(v);
		}
	}
	return oldflag;
}

/* Interfaces to write objects/strings to file-like objects */

int
PyFile_WriteObject(v, f, flags)
	PyObject *v;
	PyObject *f;
	int flags;
{
	PyObject *writer, *value, *args, *result;
	if (f == NULL) {
		PyErr_SetString(PyExc_TypeError, "writeobject with NULL file");
		return -1;
	}
	else if (PyFile_Check(f)) {
		FILE *fp = PyFile_AsFile(f);
		if (fp == NULL) {
			err_closed();
			return -1;
		}
		return PyObject_Print(v, fp, flags);
	}
	writer = PyObject_GetAttrString(f, "write");
	if (writer == NULL)
		return -1;
	if (flags & Py_PRINT_RAW)
		value = PyObject_Str(v);
	else
		value = PyObject_Repr(v);
	if (value == NULL) {
		Py_DECREF(writer);
		return -1;
	}
	args = Py_BuildValue("(O)", value);
	if (args == NULL) {
		Py_DECREF(value);
		Py_DECREF(writer);
		return -1;
	}
	result = PyEval_CallObject(writer, args);
	Py_DECREF(args);
	Py_DECREF(value);
	Py_DECREF(writer);
	if (result == NULL)
		return -1;
	Py_DECREF(result);
	return 0;
}

int
PyFile_WriteString(s, f)
	char *s;
	PyObject *f;
{
	if (f == NULL) {
		/* Should be caused by a pre-existing error */
		if(!PyErr_Occurred())
			PyErr_SetString(PyExc_SystemError,
					"null file for PyFile_WriteString");
		return -1;
	}
	else if (PyFile_Check(f)) {
		FILE *fp = PyFile_AsFile(f);
		if (fp == NULL) {
			err_closed();
			return -1;
		}
		fputs(s, fp);
		return 0;
	}
	else if (!PyErr_Occurred()) {
		PyObject *v = PyString_FromString(s);
		int err;
		if (v == NULL)
			return -1;
		err = PyFile_WriteObject(v, f, Py_PRINT_RAW);
		Py_DECREF(v);
		return err;
	}
	else
		return -1;
}
