
/* File object implementation */

#include "Python.h"
#include "structmember.h"

#ifndef DONT_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* DONT_HAVE_SYS_TYPES_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef MS_WIN32
#define fileno _fileno
/* can (almost fully) duplicate with _chsize, see file_truncate */
#define HAVE_FTRUNCATE
#endif

#ifdef macintosh
#ifdef USE_GUSI
#define HAVE_FTRUNCATE
#endif
#endif

#ifdef __MWERKS__
/* Mwerks fopen() doesn't always set errno */
#define NO_FOPEN_ERRNO
#endif

#define BUF(v) PyString_AS_STRING((PyStringObject *)v)

#ifndef DONT_HAVE_ERRNO_H
#include <errno.h>
#endif


typedef struct {
	PyObject_HEAD
	FILE *f_fp;
	PyObject *f_name;
	PyObject *f_mode;
	int (*f_close)(FILE *);
	int f_softspace; /* Flag used by 'print' command */
	int f_binary; /* Flag which indicates whether the file is open
			 open in binary (1) or test (0) mode */
} PyFileObject;

FILE *
PyFile_AsFile(PyObject *f)
{
	if (f == NULL || !PyFile_Check(f))
		return NULL;
	else
		return ((PyFileObject *)f)->f_fp;
}

PyObject *
PyFile_Name(PyObject *f)
{
	if (f == NULL || !PyFile_Check(f))
		return NULL;
	else
		return ((PyFileObject *)f)->f_name;
}

PyObject *
PyFile_FromFile(FILE *fp, char *name, char *mode, int (*close)(FILE *))
{
	PyFileObject *f = PyObject_NEW(PyFileObject, &PyFile_Type);
	if (f == NULL)
		return NULL;
	f->f_fp = NULL;
	f->f_name = PyString_FromString(name);
	f->f_mode = PyString_FromString(mode);
	f->f_close = close;
	f->f_softspace = 0;
	if (strchr(mode,'b') != NULL)
	    f->f_binary = 1;
	else
	    f->f_binary = 0;
	if (f->f_name == NULL || f->f_mode == NULL) {
		Py_DECREF(f);
		return NULL;
	}
	f->f_fp = fp;
	return (PyObject *) f;
}

PyObject *
PyFile_FromString(char *name, char *mode)
{
	extern int fclose(FILE *);
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
		/* Metroworks only, not testable, so unchanged */
		if ( errno == 0 ) {
			PyErr_SetString(PyExc_IOError, "Cannot open file");
			Py_DECREF(f);
			return NULL;
		}
#endif
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, name);
		Py_DECREF(f);
		return NULL;
	}
	return (PyObject *)f;
}

void
PyFile_SetBufSize(PyObject *f, int bufsize)
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
err_closed(void)
{
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed file");
	return NULL;
}

/* Methods */

static void
file_dealloc(PyFileObject *f)
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
	PyObject_DEL(f);
}

static PyObject *
file_repr(PyFileObject *f)
{
	char buf[300];
	sprintf(buf, "<%s file '%.256s', mode '%.10s' at %p>",
		f->f_fp == NULL ? "closed" : "open",
		PyString_AsString(f->f_name),
		PyString_AsString(f->f_mode),
		f);
	return PyString_FromString(buf);
}

static PyObject *
file_close(PyFileObject *f, PyObject *args)
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


/* An 8-byte off_t-like type */
#if defined(HAVE_LARGEFILE_SUPPORT) && SIZEOF_OFF_T < 8 && SIZEOF_FPOS_T >= 8
typedef fpos_t Py_off_t;
#else
typedef off_t Py_off_t;
#endif


/* a portable fseek() function
   return 0 on success, non-zero on failure (with errno set) */
static int
_portable_fseek(FILE *fp, Py_off_t offset, int whence)
{
#if defined(HAVE_FSEEKO)
	return fseeko(fp, offset, whence);
#elif defined(HAVE_FSEEK64)
	return fseek64(fp, offset, whence);
#elif defined(__BEOS__)
	return _fseek(fp, offset, whence);
#elif defined(HAVE_LARGEFILE_SUPPORT) && SIZEOF_FPOS_T >= 8
	/* lacking a 64-bit capable fseek(), use a 64-bit capable fsetpos()
	   and fgetpos() to implement fseek()*/
	fpos_t pos;
	switch (whence) {
	case SEEK_END:
		if (fseek(fp, 0, SEEK_END) != 0)
			return -1;
		/* fall through */
	case SEEK_CUR:
		if (fgetpos(fp, &pos) != 0)
			return -1;
		offset += pos;
		break;
	/* case SEEK_SET: break; */
	}
	return fsetpos(fp, &offset);
#else
	return fseek(fp, offset, whence);
#endif
}


/* a portable ftell() function
   Return -1 on failure with errno set appropriately, current file
   position on success */
static Py_off_t
_portable_ftell(FILE* fp)
{
#if SIZEOF_FPOS_T >= 8 && defined(HAVE_LARGEFILE_SUPPORT)
	fpos_t pos;
	if (fgetpos(fp, &pos) != 0)
		return -1;
	return pos;
#elif defined(HAVE_FTELLO) && defined(HAVE_LARGEFILE_SUPPORT)
	return ftello(fp);
#elif defined(HAVE_FTELL64) && defined(HAVE_LARGEFILE_SUPPORT)
	return ftell64(fp);
#else
	return ftell(fp);
#endif
}


static PyObject *
file_seek(PyFileObject *f, PyObject *args)
{
	int whence;
	int ret;
	Py_off_t offset;
	PyObject *offobj;

	if (f->f_fp == NULL)
		return err_closed();
	whence = 0;
	if (!PyArg_ParseTuple(args, "O|i:seek", &offobj, &whence))
		return NULL;
#if !defined(HAVE_LARGEFILE_SUPPORT)
	offset = PyInt_AsLong(offobj);
#else
	offset = PyLong_Check(offobj) ?
		PyLong_AsLongLong(offobj) : PyInt_AsLong(offobj);
#endif
	if (PyErr_Occurred())
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	ret = _portable_fseek(f->f_fp, offset, whence);
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
file_truncate(PyFileObject *f, PyObject *args)
{
	int ret;
	Py_off_t newsize;
	PyObject *newsizeobj;

	if (f->f_fp == NULL)
		return err_closed();
	newsizeobj = NULL;
	if (!PyArg_ParseTuple(args, "|O:truncate", &newsizeobj))
		return NULL;
	if (newsizeobj != NULL) {
#if !defined(HAVE_LARGEFILE_SUPPORT)
		newsize = PyInt_AsLong(newsizeobj);
#else
		newsize = PyLong_Check(newsizeobj) ?
				PyLong_AsLongLong(newsizeobj) :
				PyInt_AsLong(newsizeobj);
#endif
		if (PyErr_Occurred())
			return NULL;
	} else {
		/* Default to current position*/
		Py_BEGIN_ALLOW_THREADS
		errno = 0;
		newsize = _portable_ftell(f->f_fp);
		Py_END_ALLOW_THREADS
		if (newsize == -1) {
		        PyErr_SetFromErrno(PyExc_IOError);
			clearerr(f->f_fp);
			return NULL;
		}
	}
	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	ret = fflush(f->f_fp);
	Py_END_ALLOW_THREADS
	if (ret != 0) goto onioerror;

#ifdef MS_WIN32
	/* can use _chsize; if, however, the newsize overflows 32-bits then
	   _chsize is *not* adequate; in this case, an OverflowError is raised */
	if (newsize > LONG_MAX) {
		PyErr_SetString(PyExc_OverflowError,
			"the new size is too long for _chsize (it is limited to 32-bit values)");
		return NULL;
	} else {
		Py_BEGIN_ALLOW_THREADS
		errno = 0;
		ret = _chsize(fileno(f->f_fp), newsize);
		Py_END_ALLOW_THREADS
		if (ret != 0) goto onioerror;
	}
#else
	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	ret = ftruncate(fileno(f->f_fp), newsize);
	Py_END_ALLOW_THREADS
	if (ret != 0) goto onioerror;
#endif /* !MS_WIN32 */

	Py_INCREF(Py_None);
	return Py_None;

onioerror:
	PyErr_SetFromErrno(PyExc_IOError);
	clearerr(f->f_fp);
	return NULL;
}
#endif /* HAVE_FTRUNCATE */

static PyObject *
file_tell(PyFileObject *f, PyObject *args)
{
	Py_off_t pos;

	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_NoArgs(args))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	pos = _portable_ftell(f->f_fp);
	Py_END_ALLOW_THREADS
	if (pos == -1) {
		PyErr_SetFromErrno(PyExc_IOError);
		clearerr(f->f_fp);
		return NULL;
	}
#if !defined(HAVE_LARGEFILE_SUPPORT)
	return PyInt_FromLong(pos);
#else
	return PyLong_FromLongLong(pos);
#endif
}

static PyObject *
file_fileno(PyFileObject *f, PyObject *args)
{
	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long) fileno(f->f_fp));
}

static PyObject *
file_flush(PyFileObject *f, PyObject *args)
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
file_isatty(PyFileObject *f, PyObject *args)
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


#if BUFSIZ < 8192
#define SMALLCHUNK 8192
#else
#define SMALLCHUNK BUFSIZ
#endif

#if SIZEOF_INT < 4
#define BIGCHUNK  (512 * 32)
#else
#define BIGCHUNK  (512 * 1024)
#endif

static size_t
new_buffersize(PyFileObject *f, size_t currentsize)
{
#ifdef HAVE_FSTAT
	long pos, end;
	struct stat st;
	if (fstat(fileno(f->f_fp), &st) == 0) {
		end = st.st_size;
		/* The following is not a bug: we really need to call lseek()
		   *and* ftell().  The reason is that some stdio libraries
		   mistakenly flush their buffer when ftell() is called and
		   the lseek() call it makes fails, thereby throwing away
		   data that cannot be recovered in any way.  To avoid this,
		   we first test lseek(), and only call ftell() if lseek()
		   works.  We can't use the lseek() value either, because we
		   need to take the amount of buffered data into account.
		   (Yet another reason why stdio stinks. :-) */
		pos = lseek(fileno(f->f_fp), 0L, SEEK_CUR);
		if (pos >= 0)
			pos = ftell(f->f_fp);
		if (pos < 0)
			clearerr(f->f_fp);
		if (end > pos && pos >= 0)
			return currentsize + end - pos + 1;
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
file_read(PyFileObject *f, PyObject *args)
{
	long bytesrequested = -1;
	size_t bytesread, buffersize, chunksize;
	PyObject *v;

	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_ParseTuple(args, "|l:read", &bytesrequested))
		return NULL;
	if (bytesrequested < 0)
		buffersize = new_buffersize(f, (size_t)0);
	else
		buffersize = bytesrequested;
	if (buffersize > INT_MAX) {
		PyErr_SetString(PyExc_OverflowError,
			"requested number of bytes is more than a Python string can hold");
		return NULL;
	}
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
			buffersize = new_buffersize(f, buffersize);
			if (_PyString_Resize(&v, buffersize) < 0)
				return NULL;
		}
	}
	if (bytesread != buffersize)
		_PyString_Resize(&v, bytesread);
	return v;
}

static PyObject *
file_readinto(PyFileObject *f, PyObject *args)
{
	char *ptr;
	size_t ntodo, ndone, nnow;

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
	return PyInt_FromLong((long)ndone);
}

/**************************************************************************
Routine to get next line using platform fgets().

Under MSVC 6:

+ MS threadsafe getc is very slow (multiple layers of function calls before+
  after each character, to lock+unlock the stream).
+ The stream-locking functions are MS-internal -- can't access them from user
  code.
+ There's nothing Tim could find in the MS C or platform SDK libraries that
  can worm around this.
+ MS fgets locks/unlocks only once per line; it's the only hook we have.

So we use fgets for speed(!), despite that it's painful.

MS realloc is also slow.

Reports from other platforms on this method vs getc_unlocked (which MS doesn't
have):
	Linux		a wash
	Solaris		a wash
	Tru64 Unix	getline_via_fgets significantly faster

CAUTION:  The C std isn't clear about this:  in those cases where fgets
writes something into the buffer, can it write into any position beyond the
required trailing null byte?  MSVC 6 fgets does not, and no platform is (yet)
known on which it does; and it would be a strange way to code fgets. Still,
getline_via_fgets may not work correctly if it does.  The std test
test_bufio.py should fail if platform fgets() routinely writes beyond the
trailing null byte.  #define DONT_USE_FGETS_IN_GETLINE to disable this code.
**************************************************************************/

/* Use this routine if told to, or by default on non-get_unlocked()
 * platforms unless told not to.  Yikes!  Let's spell that out:
 * On a platform with getc_unlocked():
 *     By default, use getc_unlocked().
 *     If you want to use fgets() instead, #define USE_FGETS_IN_GETLINE.
 * On a platform without getc_unlocked():
 *     By default, use fgets().
 *     If you don't want to use fgets(), #define DONT_USE_FGETS_IN_GETLINE.
 */
#if !defined(USE_FGETS_IN_GETLINE) && !defined(HAVE_GETC_UNLOCKED)
#define USE_FGETS_IN_GETLINE
#endif

#if defined(DONT_USE_FGETS_IN_GETLINE) && defined(USE_FGETS_IN_GETLINE)
#undef USE_FGETS_IN_GETLINE
#endif

#ifdef USE_FGETS_IN_GETLINE
static PyObject*
getline_via_fgets(FILE *fp)
{
/* INITBUFSIZE is the maximum line length that lets us get away with the fast
 * no-realloc, one-fgets()-call path.  Boosting it isn't free, because we have
 * to fill this much of the buffer with a known value in order to figure out
 * how much of the buffer fgets() overwrites.  So if INITBUFSIZE is larger
 * than "most" lines, we waste time filling unused buffer slots.  100 is
 * surely adequate for most peoples' email archives, chewing over source code,
 * etc -- "regular old text files".
 * MAXBUFSIZE is the maximum line length that lets us get away with the less
 * fast (but still zippy) no-realloc, two-fgets()-call path.  See above for
 * cautions about boosting that.  300 was chosen because the worst real-life
 * text-crunching job reported on Python-Dev was a mail-log crawler where over
 * half the lines were 254 chars.
 * INCBUFSIZE is the amount by which we grow the buffer, if MAXBUFSIZE isn't
 * enough.  It doesn't much matter what this is set to: we only get here for
 * absurdly long lines anyway.
 */
#define INITBUFSIZE 100
#define MAXBUFSIZE 300
#define INCBUFSIZE 1000
	char* p;	/* temp */
	char buf[MAXBUFSIZE];
	PyObject* v;	/* the string object result */
	char* pvfree;	/* address of next free slot */
	char* pvend;    /* address one beyond last free slot */
	size_t nfree;	/* # of free buffer slots; pvend-pvfree */
	size_t total_v_size;  /* total # of slots in buffer */

	/* Optimize for normal case:  avoid _PyString_Resize if at all
	 * possible via first reading into stack buffer "buf".
	 */
	total_v_size = INITBUFSIZE;	/* start small and pray */
	pvfree = buf;
	for (;;) {
		Py_BEGIN_ALLOW_THREADS
		pvend = buf + total_v_size;
		nfree = pvend - pvfree;
		memset(pvfree, '\n', nfree);
		p = fgets(pvfree, nfree, fp);
		Py_END_ALLOW_THREADS

		if (p == NULL) {
			clearerr(fp);
			if (PyErr_CheckSignals())
				return NULL;
			v = PyString_FromStringAndSize(buf, pvfree - buf);
			return v;
		}
		/* fgets read *something* */
		p = memchr(pvfree, '\n', nfree);
		if (p != NULL) {
			/* Did the \n come from fgets or from us?
			 * Since fgets stops at the first \n, and then writes
			 * \0, if it's from fgets a \0 must be next.  But if
			 * that's so, it could not have come from us, since
			 * the \n's we filled the buffer with have only more
			 * \n's to the right.
			 */
			if (p+1 < pvend && *(p+1) == '\0') {
				/* It's from fgets:  we win!  In particular,
				 * we haven't done any mallocs yet, and can
				 * build the final result on the first try.
				 */
				++p;	/* include \n from fgets */
			}
			else {
				/* Must be from us:  fgets didn't fill the
				 * buffer and didn't find a newline, so it
				 * must be the last and newline-free line of
				 * the file.
				 */
				assert(p > pvfree && *(p-1) == '\0');
				--p;	/* don't include \0 from fgets */
			}
			v = PyString_FromStringAndSize(buf, p - buf);
			return v;
		}
		/* yuck:  fgets overwrote all the newlines, i.e. the entire
		 * buffer.  So this line isn't over yet, or maybe it is but
		 * we're exactly at EOF.  If we haven't already, try using the
		 * rest of the stack buffer.
		 */
		assert(*(pvend-1) == '\0');
		if (pvfree == buf) {
			pvfree = pvend - 1;	/* overwrite trailing null */
			total_v_size = MAXBUFSIZE;
		}
		else
			break;
	}

	/* The stack buffer isn't big enough; malloc a string object and read
	 * into its buffer.
	 */
	total_v_size = MAXBUFSIZE + INCBUFSIZE;
	v = PyString_FromStringAndSize((char*)NULL, (int)total_v_size);
	if (v == NULL)
		return v;
	/* copy over everything except the last null byte */
	memcpy(BUF(v), buf, MAXBUFSIZE-1);
	pvfree = BUF(v) + MAXBUFSIZE - 1;

	/* Keep reading stuff into v; if it ever ends successfully, break
	 * after setting p one beyond the end of the line.  The code here is
	 * very much like the code above, except reads into v's buffer; see
	 * the code above for detailed comments about the logic.
	 */
	for (;;) {
		Py_BEGIN_ALLOW_THREADS
		pvend = BUF(v) + total_v_size;
		nfree = pvend - pvfree;
		memset(pvfree, '\n', nfree);
		p = fgets(pvfree, nfree, fp);
		Py_END_ALLOW_THREADS

		if (p == NULL) {
			clearerr(fp);
			if (PyErr_CheckSignals()) {
				Py_DECREF(v);
				return NULL;
			}
			p = pvfree;
			break;
		}
		p = memchr(pvfree, '\n', nfree);
		if (p != NULL) {
			if (p+1 < pvend && *(p+1) == '\0') {
				/* \n came from fgets */
				++p;
				break;
			}
			/* \n came from us; last line of file, no newline */
			assert(p > pvfree && *(p-1) == '\0');
			--p;
			break;
		}
		/* expand buffer and try again */
		assert(*(pvend-1) == '\0');
		total_v_size += INCBUFSIZE;
		if (total_v_size > INT_MAX) {
			PyErr_SetString(PyExc_OverflowError,
			    "line is longer than a Python string can hold");
			Py_DECREF(v);
			return NULL;
		}
		if (_PyString_Resize(&v, (int)total_v_size) < 0)
			return NULL;
		/* overwrite the trailing null byte */
		pvfree = BUF(v) + (total_v_size - INCBUFSIZE - 1);
	}
	if (BUF(v) + total_v_size != p)
		_PyString_Resize(&v, p - BUF(v));
	return v;
#undef INITBUFSIZE
#undef MAXBUFSIZE
#undef INCBUFSIZE
}
#endif	/* ifdef USE_FGETS_IN_GETLINE */

/* Internal routine to get a line.
   Size argument interpretation:
   > 0: max length;
   <= 0: read arbitrary line
*/

#ifdef HAVE_GETC_UNLOCKED
#define GETC(f) getc_unlocked(f)
#define FLOCKFILE(f) flockfile(f)
#define FUNLOCKFILE(f) funlockfile(f)
#else
#define GETC(f) getc(f)
#define FLOCKFILE(f)
#define FUNLOCKFILE(f)
#endif

static PyObject *
get_line(PyFileObject *f, int n)
{
	FILE *fp = f->f_fp;
	int c;
	char *buf, *end;
	size_t n1, n2;
	PyObject *v;

#ifdef USE_FGETS_IN_GETLINE
	if (n <= 0)
		return getline_via_fgets(fp);
#endif
	n2 = n > 0 ? n : 100;
	v = PyString_FromStringAndSize((char *)NULL, n2);
	if (v == NULL)
		return NULL;
	buf = BUF(v);
	end = buf + n2;

	for (;;) {
		Py_BEGIN_ALLOW_THREADS
		FLOCKFILE(fp);
		while ((c = GETC(fp)) != EOF &&
		       (*buf++ = c) != '\n' &&
			buf != end)
			;
		FUNLOCKFILE(fp);
		Py_END_ALLOW_THREADS
		if (c == '\n')
			break;
		if (c == EOF) {
			clearerr(fp);
			if (PyErr_CheckSignals()) {
				Py_DECREF(v);
				return NULL;
			}
			break;
		}
		/* Must be because buf == end */
		if (n > 0)
			break;
		n1 = n2;
		n2 += 1000;
		if (n2 > INT_MAX) {
			PyErr_SetString(PyExc_OverflowError,
			    "line is longer than a Python string can hold");
			Py_DECREF(v);
			return NULL;
		}
		if (_PyString_Resize(&v, n2) < 0)
			return NULL;
		buf = BUF(v) + n1;
		end = BUF(v) + n2;
	}

	n1 = buf - BUF(v);
	if (n1 != n2)
		_PyString_Resize(&v, n1);
	return v;
}

/* External C interface */

PyObject *
PyFile_GetLine(PyObject *f, int n)
{
	PyObject *result;

	if (f == NULL) {
		PyErr_BadInternalCall();
		return NULL;
	}

	if (PyFile_Check(f)) {
		if (((PyFileObject*)f)->f_fp == NULL)
			return err_closed();
		result = get_line((PyFileObject *)f, n);
	}
	else {
		PyObject *reader;
		PyObject *args;

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
	}

	if (n < 0 && result != NULL && PyString_Check(result)) {
		char *s = PyString_AS_STRING(result);
		int len = PyString_GET_SIZE(result);
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
				v = PyString_FromStringAndSize(s, len-1);
				Py_DECREF(result);
				result = v;
			}
		}
	}
	return result;
}

/* Python method */

static PyObject *
file_readline(PyFileObject *f, PyObject *args)
{
	int n = -1;

	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_ParseTuple(args, "|i:readline", &n))
		return NULL;
	if (n == 0)
		return PyString_FromString("");
	if (n < 0)
		n = 0;
	return get_line(f, n);
}

static PyObject *
file_xreadlines(PyFileObject *f, PyObject *args)
{
	static PyObject* xreadlines_function = NULL;

	if (!PyArg_ParseTuple(args, ":xreadlines"))
		return NULL;

	if (!xreadlines_function) {
		PyObject *xreadlines_module =
			PyImport_ImportModule("xreadlines");
		if(!xreadlines_module)
			return NULL;

		xreadlines_function = PyObject_GetAttrString(xreadlines_module,
							     "xreadlines");
		Py_DECREF(xreadlines_module);
		if(!xreadlines_function)
			return NULL;
	}
	return PyObject_CallFunction(xreadlines_function, "(O)", f);
}

static PyObject *
file_readlines(PyFileObject *f, PyObject *args)
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
	if (!PyArg_ParseTuple(args, "|l:readlines", &sizehint))
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
			if (buffersize > INT_MAX) {
				PyErr_SetString(PyExc_OverflowError,
			    "line is longer than a Python string can hold");
				goto error;
			}
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
			PyObject *rest = get_line(f, 0);
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
file_write(PyFileObject *f, PyObject *args)
{
	char *s;
	int n, n2;
	if (f->f_fp == NULL)
		return err_closed();
	if (!PyArg_Parse(args, f->f_binary ? "s#" : "t#", &s, &n))
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
file_writelines(PyFileObject *f, PyObject *args)
{
#define CHUNKSIZE 1000
	PyObject *list, *line;
	PyObject *result;
	int i, j, index, len, nwritten, islist;

	if (f->f_fp == NULL)
		return err_closed();
	if (args == NULL || !PySequence_Check(args)) {
		PyErr_SetString(PyExc_TypeError,
			   "writelines() argument must be a sequence of strings");
		return NULL;
	}
	islist = PyList_Check(args);

	/* Strategy: slurp CHUNKSIZE lines into a private list,
	   checking that they are all strings, then write that list
	   without holding the interpreter lock, then come back for more. */
	index = 0;
	if (islist)
		list = NULL;
	else {
		list = PyList_New(CHUNKSIZE);
		if (list == NULL)
			return NULL;
	}
	result = NULL;

	for (;;) {
		if (islist) {
			Py_XDECREF(list);
			list = PyList_GetSlice(args, index, index+CHUNKSIZE);
			if (list == NULL)
				return NULL;
			j = PyList_GET_SIZE(list);
		}
		else {
			for (j = 0; j < CHUNKSIZE; j++) {
				line = PySequence_GetItem(args, index+j);
				if (line == NULL) {
					if (PyErr_ExceptionMatches(
						PyExc_IndexError)) {
						PyErr_Clear();
						break;
					}
					/* Some other error occurred.
					   XXX We may lose some output. */
					goto error;
				}
				PyList_SetItem(list, j, line);
			}
		}
		if (j == 0)
			break;

		/* Check that all entries are indeed strings. If not,
		   apply the same rules as for file.write() and
		   convert the results to strings. This is slow, but
		   seems to be the only way since all conversion APIs
		   could potentially execute Python code. */
		for (i = 0; i < j; i++) {
			PyObject *v = PyList_GET_ITEM(list, i);
			if (!PyString_Check(v)) {
			    	const char *buffer;
			    	int len;
				if (((f->f_binary &&
				      PyObject_AsReadBuffer(v,
					      (const void**)&buffer,
							    &len)) ||
				     PyObject_AsCharBuffer(v,
							   &buffer,
							   &len))) {
					PyErr_SetString(PyExc_TypeError,
				"writelines() argument must be a sequence of strings");
					goto error;
				}
				line = PyString_FromStringAndSize(buffer,
								  len);
				if (line == NULL)
					goto error;
				Py_DECREF(v);
				PyList_SET_ITEM(list, i, line);
			}
		}

		/* Since we are releasing the global lock, the
		   following code may *not* execute Python code. */
		Py_BEGIN_ALLOW_THREADS
		f->f_softspace = 0;
		errno = 0;
		for (i = 0; i < j; i++) {
		    	line = PyList_GET_ITEM(list, i);
			len = PyString_GET_SIZE(line);
			nwritten = fwrite(PyString_AS_STRING(line),
					  1, len, f->f_fp);
			if (nwritten != len) {
				Py_BLOCK_THREADS
				PyErr_SetFromErrno(PyExc_IOError);
				clearerr(f->f_fp);
				goto error;
			}
		}
		Py_END_ALLOW_THREADS

		if (j < CHUNKSIZE)
			break;
		index += CHUNKSIZE;
	}

	Py_INCREF(Py_None);
	result = Py_None;
  error:
	Py_XDECREF(list);
	return result;
}

static PyMethodDef file_methods[] = {
	{"readline",	(PyCFunction)file_readline, 1},
	{"read",	(PyCFunction)file_read, 1},
	{"write",	(PyCFunction)file_write, 0},
	{"fileno",	(PyCFunction)file_fileno, 0},
	{"seek",	(PyCFunction)file_seek, 1},
#ifdef HAVE_FTRUNCATE
	{"truncate",	(PyCFunction)file_truncate, 1},
#endif
	{"tell",	(PyCFunction)file_tell, 0},
	{"readinto",	(PyCFunction)file_readinto, 0},
	{"readlines",	(PyCFunction)file_readlines, 1},
	{"xreadlines",	(PyCFunction)file_xreadlines, 1},
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
file_getattr(PyFileObject *f, char *name)
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
file_setattr(PyFileObject *f, char *name, PyObject *v)
{
	if (v == NULL) {
		PyErr_SetString(PyExc_AttributeError,
				"can't delete file attributes");
		return -1;
	}
	return PyMember_Set((char *)f, file_memberlist, name, v);
}

static PyObject *
file_getiter(PyObject *f)
{
	return PyObject_CallMethod(f, "xreadlines", "");
}

PyTypeObject PyFile_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"file",
	sizeof(PyFileObject),
	0,
	(destructor)file_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	(getattrfunc)file_getattr,		/* tp_getattr */
	(setattrfunc)file_setattr,		/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)file_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
 	0,					/* tp_doc */
 	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	file_getiter,				/* tp_iter */
	0,					/* tp_iternext */
};

/* Interface for the 'soft space' between print items. */

int
PyFile_SoftSpace(PyObject *f, int newflag)
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
PyFile_WriteObject(PyObject *v, PyObject *f, int flags)
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
PyFile_WriteString(char *s, PyObject *f)
{
	if (f == NULL) {
		/* Should be caused by a pre-existing error */
		if (!PyErr_Occurred())
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

/* Try to get a file-descriptor from a Python object.  If the object
   is an integer or long integer, its value is returned.  If not, the
   object's fileno() method is called if it exists; the method must return
   an integer or long integer, which is returned as the file descriptor value.
   -1 is returned on failure.
*/

int PyObject_AsFileDescriptor(PyObject *o)
{
	int fd;
	PyObject *meth;

	if (PyInt_Check(o)) {
		fd = PyInt_AsLong(o);
	}
	else if (PyLong_Check(o)) {
		fd = PyLong_AsLong(o);
	}
	else if ((meth = PyObject_GetAttrString(o, "fileno")) != NULL)
	{
		PyObject *fno = PyEval_CallObject(meth, NULL);
		Py_DECREF(meth);
		if (fno == NULL)
			return -1;

		if (PyInt_Check(fno)) {
			fd = PyInt_AsLong(fno);
			Py_DECREF(fno);
		}
		else if (PyLong_Check(fno)) {
			fd = PyLong_AsLong(fno);
			Py_DECREF(fno);
		}
		else {
			PyErr_SetString(PyExc_TypeError,
					"fileno() returned a non-integer");
			Py_DECREF(fno);
			return -1;
		}
	}
	else {
		PyErr_SetString(PyExc_TypeError,
				"argument must be an int, or have a fileno() method.");
		return -1;
	}

	if (fd < 0) {
		PyErr_Format(PyExc_ValueError,
			     "file descriptor cannot be a negative integer (%i)",
			     fd);
		return -1;
	}
	return fd;
}
