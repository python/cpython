/*
 /  Author: Sam Rushing <rushing@nightmare.com>
 /  Hacked for Unix by A.M. Kuchling <amk1@bigfoot.com> 
 /  $Id$

 / mmapmodule.cpp -- map a view of a file into memory
 /
 / todo: need permission flags, perhaps a 'chsize' analog
 /   not all functions check range yet!!!
 /
 /
 / Note: This module currently only deals with 32-bit file
 /   sizes.
 /
 / This version of mmapmodule.c has been changed significantly
 / from the original mmapfile.c on which it was based.
 / The original version of mmapfile is maintained by Sam at
 / ftp://squirl.nightmare.com/pub/python/python-ext.
*/

#include <Python.h>

#ifndef MS_WIN32
#define UNIX
#endif

#ifdef MS_WIN32
#include <windows.h>
static int
my_getpagesize(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}
#endif

#ifdef UNIX
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#ifndef MS_SYNC
/* This is missing e.g. on SunOS 4.1.4 */
#define MS_SYNC 0
#endif

#if defined(HAVE_SYSCONF) && defined(_SC_PAGESIZE)
static int
my_getpagesize(void)
{
    return sysconf(_SC_PAGESIZE);
}
#else
#define my_getpagesize getpagesize
#endif

#endif /* UNIX */

#include <string.h>
#include <sys/types.h>

static PyObject *mmap_module_error;

typedef struct {
	PyObject_HEAD
	char *	data;
	size_t	size;
	size_t	pos;

#ifdef MS_WIN32
	HANDLE	map_handle;
	HANDLE	file_handle;
	char *	tagname;
#endif

#ifdef UNIX
        int fd;
#endif
} mmap_object;

static void
mmap_object_dealloc(mmap_object *m_obj)
{
#ifdef MS_WIN32
	if (m_obj->data != NULL)
		UnmapViewOfFile (m_obj->data);
	if (m_obj->map_handle != INVALID_HANDLE_VALUE)
		CloseHandle (m_obj->map_handle);
	if (m_obj->file_handle != INVALID_HANDLE_VALUE)
		CloseHandle (m_obj->file_handle);
	if (m_obj->tagname)
		PyMem_Free(m_obj->tagname);
#endif /* MS_WIN32 */

#ifdef UNIX
	if (m_obj->data!=NULL) {
		msync(m_obj->data, m_obj->size, MS_SYNC);
		munmap(m_obj->data, m_obj->size);
	}
#endif /* UNIX */

	PyObject_Del(m_obj);
}

static PyObject *
mmap_close_method(mmap_object *self, PyObject *args)
{
        if (!PyArg_ParseTuple(args, ":close"))
		return NULL;
#ifdef MS_WIN32
	/* For each resource we maintain, we need to check
	   the value is valid, and if so, free the resource 
	   and set the member value to an invalid value so
	   the dealloc does not attempt to resource clearing
	   again.
	   TODO - should we check for errors in the close operations???
	*/
	if (self->data != NULL) {
		UnmapViewOfFile (self->data);
		self->data = NULL;
	}
	if (self->map_handle != INVALID_HANDLE_VALUE) {
		CloseHandle (self->map_handle);
		self->map_handle = INVALID_HANDLE_VALUE;
	}
	if (self->file_handle != INVALID_HANDLE_VALUE) {
		CloseHandle (self->file_handle);
		self->file_handle = INVALID_HANDLE_VALUE;
	}
#endif /* MS_WIN32 */

#ifdef UNIX
	munmap(self->data, self->size);
	self->data = NULL;
#endif

	Py_INCREF (Py_None);
	return (Py_None);
}

#ifdef MS_WIN32
#define CHECK_VALID(err)						\
do {									\
    if (!self->map_handle) {						\
	PyErr_SetString (PyExc_ValueError, "mmap closed or invalid");	\
	return err;							\
    }									\
} while (0)
#endif /* MS_WIN32 */

#ifdef UNIX
#define CHECK_VALID(err)						\
do {									\
    if (self->data == NULL) {						\
	PyErr_SetString (PyExc_ValueError, "mmap closed or invalid");	\
	return err;							\
	}								\
} while (0)
#endif /* UNIX */

static PyObject *
mmap_read_byte_method(mmap_object *self,
		      PyObject *args)
{
	char value;
	char *where;
	CHECK_VALID(NULL);
        if (!PyArg_ParseTuple(args, ":read_byte"))
		return NULL;
	if (self->pos < self->size) {
	        where = self->data + self->pos;
		value = (char) *(where);
		self->pos += 1;
		return Py_BuildValue("c", (char) *(where));
	} else {
		PyErr_SetString (PyExc_ValueError, "read byte out of range");
		return NULL;
	}
}

static PyObject *
mmap_read_line_method(mmap_object *self,
		     PyObject *args)
{
	char *start = self->data+self->pos;
	char *eof = self->data+self->size;
	char *eol;
	PyObject *result;

	CHECK_VALID(NULL);
        if (!PyArg_ParseTuple(args, ":readline"))
		return NULL;

	eol = memchr(start, '\n', self->size - self->pos);
	if (!eol)
		eol = eof;
	else
		++eol;		/* we're interested in the position after the
				   newline. */
	result = PyString_FromStringAndSize(start, (eol - start));
	self->pos += (eol - start);
	return (result);
}

static PyObject *
mmap_read_method(mmap_object *self,
		 PyObject *args)
{
	long num_bytes;
	PyObject *result;

	CHECK_VALID(NULL);
	if (!PyArg_ParseTuple(args, "l:read", &num_bytes))
		return(NULL);

	/* silently 'adjust' out-of-range requests */
	if ((self->pos + num_bytes) > self->size) {
		num_bytes -= (self->pos+num_bytes) - self->size;
	}
	result = Py_BuildValue("s#", self->data+self->pos, num_bytes);
	self->pos += num_bytes;	 
	return (result);
}

static PyObject *
mmap_find_method(mmap_object *self,
		 PyObject *args)
{
	int start = self->pos;
	char *needle;
	int len;

	CHECK_VALID(NULL);
	if (!PyArg_ParseTuple (args, "s#|i:find", &needle, &len, &start)) {
		return NULL;
	} else {
		char *p = self->data+self->pos;
		char *e = self->data+self->size;
		while (p < e) {
			char *s = p;
			char *n = needle;
			while ((s<e) && (*n) && !(*s-*n)) {
				s++, n++;
			}
			if (!*n) {
				return Py_BuildValue (
					"i",
					(int) (p - (self->data + start)));
			}
			p++;
		}
		return Py_BuildValue ("l", (long) -1);
	}
}

static PyObject *
mmap_write_method(mmap_object *self,
		  PyObject *args)
{
	long length;
	char *data;

	CHECK_VALID(NULL);
	if (!PyArg_ParseTuple (args, "s#:write", &data, &length))
		return(NULL);

	if ((self->pos + length) > self->size) {
		PyErr_SetString (PyExc_ValueError, "data out of range");
		return NULL;
	}
	memcpy (self->data+self->pos, data, length);
	self->pos = self->pos+length;
	Py_INCREF (Py_None);
	return (Py_None);
}

static PyObject *
mmap_write_byte_method(mmap_object *self,
		       PyObject *args)
{
	char value;

	CHECK_VALID(NULL);
	if (!PyArg_ParseTuple (args, "c:write_byte", &value))
		return(NULL);

	*(self->data+self->pos) = value;
	self->pos += 1;
	Py_INCREF (Py_None);
	return (Py_None);
}

static PyObject *
mmap_size_method(mmap_object *self,
		 PyObject *args)
{
	CHECK_VALID(NULL);
        if (!PyArg_ParseTuple(args, ":size"))
		return NULL;

#ifdef MS_WIN32
	if (self->file_handle != INVALID_HANDLE_VALUE) {
		return (Py_BuildValue (
			"l", (long)
			GetFileSize (self->file_handle, NULL)));
	} else {
		return (Py_BuildValue ("l", (long) self->size) );
	}
#endif /* MS_WIN32 */

#ifdef UNIX
	{
		struct stat buf;
		if (-1 == fstat(self->fd, &buf)) {
			PyErr_SetFromErrno(mmap_module_error);
			return NULL;
		}
		return (Py_BuildValue ("l", (long) buf.st_size) );
	}
#endif /* UNIX */
}

/* This assumes that you want the entire file mapped,
 / and when recreating the map will make the new file
 / have the new size
 /
 / Is this really necessary?  This could easily be done
 / from python by just closing and re-opening with the
 / new size?
 */

static PyObject *
mmap_resize_method(mmap_object *self,
		   PyObject *args)
{
	unsigned long new_size;
	CHECK_VALID(NULL);
	if (!PyArg_ParseTuple (args, "l:resize", &new_size)) {
		return NULL;
#ifdef MS_WIN32
	} else { 
		DWORD dwErrCode = 0;
		/* First, unmap the file view */
		UnmapViewOfFile (self->data);
		/* Close the mapping object */
		CloseHandle (self->map_handle);
		/* Move to the desired EOF position */
		SetFilePointer (self->file_handle,
				new_size, NULL, FILE_BEGIN);
		/* Change the size of the file */
		SetEndOfFile (self->file_handle);
		/* Create another mapping object and remap the file view */
		self->map_handle = CreateFileMapping (
			self->file_handle,
			NULL,
			PAGE_READWRITE,
			0,
			new_size,
			self->tagname);
		if (self->map_handle != NULL) {
			self->data = (char *) MapViewOfFile (self->map_handle,
							     FILE_MAP_WRITE,
							     0,
							     0,
							     0);
			if (self->data != NULL) {
				self->size = new_size;
				Py_INCREF (Py_None);
				return Py_None;
			} else {
				dwErrCode = GetLastError();
			}
		} else {
			dwErrCode = GetLastError();
		}
		PyErr_SetFromWindowsErr(dwErrCode);
		return (NULL);
#endif /* MS_WIN32 */

#ifdef UNIX
#ifndef HAVE_MREMAP 
} else {
	PyErr_SetString(PyExc_SystemError,
			"mmap: resizing not available--no mremap()");
	return NULL;
#else
} else {
	void *newmap;

#ifdef MREMAP_MAYMOVE
	newmap = mremap(self->data, self->size, new_size, MREMAP_MAYMOVE);
#else
	newmap = mremap(self->data, self->size, new_size, 0);
#endif
	if (newmap == (void *)-1) 
	{
		PyErr_SetFromErrno(mmap_module_error);
		return NULL;
	}
	self->data = newmap;
	self->size = new_size;
	Py_INCREF(Py_None);
	return Py_None;
#endif /* HAVE_MREMAP */
#endif /* UNIX */
}
}

static PyObject *
mmap_tell_method(mmap_object *self, PyObject *args)
{
	CHECK_VALID(NULL);
        if (!PyArg_ParseTuple(args, ":tell"))
		return NULL;
	return (Py_BuildValue ("l", (long) self->pos) );
}

static PyObject *
mmap_flush_method(mmap_object *self, PyObject *args)
{
	size_t offset	= 0;
	size_t size = self->size;
	CHECK_VALID(NULL);
	if (!PyArg_ParseTuple (args, "|ll:flush", &offset, &size)) {
		return NULL;
	} else if ((offset + size) > self->size) {
		PyErr_SetString (PyExc_ValueError,
				 "flush values out of range");
		return NULL;
	} else {
#ifdef MS_WIN32
		return (Py_BuildValue("l", (long)
                                      FlushViewOfFile(self->data+offset, size)));
#endif /* MS_WIN32 */
#ifdef UNIX
		/* XXX semantics of return value? */
		/* XXX flags for msync? */
		if (-1 == msync(self->data + offset, size,
				MS_SYNC))
		{
			PyErr_SetFromErrno(mmap_module_error);
			return NULL;
		}
		return Py_BuildValue ("l", (long) 0);	
#endif /* UNIX */   
	}
}

static PyObject *
mmap_seek_method(mmap_object *self, PyObject *args)
{
	int dist;
	int how=0;
	CHECK_VALID(NULL);
	if (!PyArg_ParseTuple (args, "i|i:seek", &dist, &how)) {
		return(NULL);
	} else {
		size_t where;
		switch (how) {
		case 0: /* relative to start */
			if (dist < 0)
				goto onoutofrange;
			where = dist;
			break;
		case 1: /* relative to current position */
			if ((int)self->pos + dist < 0)
				goto onoutofrange;
			where = self->pos + dist;
			break;
		case 2: /* relative to end */
			if ((int)self->size + dist < 0)
				goto onoutofrange;
			where = self->size + dist;
			break;
		default:
			PyErr_SetString (PyExc_ValueError,
					 "unknown seek type");
			return NULL;
		}
		if (where > self->size)
			goto onoutofrange;
		self->pos = where;
		Py_INCREF (Py_None);
		return (Py_None);
	}

onoutofrange:
	PyErr_SetString (PyExc_ValueError, "seek out of range");
	return NULL;
}

static PyObject *
mmap_move_method(mmap_object *self, PyObject *args)
{
	unsigned long dest, src, count;
	CHECK_VALID(NULL);
	if (!PyArg_ParseTuple (args, "iii:move", &dest, &src, &count)) {
		return NULL;
	} else {
		/* bounds check the values */
		if (/* end of source after end of data?? */
			((src+count) > self->size)
			/* dest will fit? */
			|| (dest+count > self->size)) {
			PyErr_SetString (PyExc_ValueError,
					 "source or destination out of range");
			return NULL;
		} else {
			memmove (self->data+dest, self->data+src, count);
			Py_INCREF (Py_None);
			return Py_None;
		}
	}
}

static struct PyMethodDef mmap_object_methods[] = {
	{"close",	(PyCFunction) mmap_close_method,	1},
	{"find",	(PyCFunction) mmap_find_method,		1},
	{"flush",	(PyCFunction) mmap_flush_method,	1},
	{"move",	(PyCFunction) mmap_move_method,		1},
	{"read",	(PyCFunction) mmap_read_method,		1},
	{"read_byte",	(PyCFunction) mmap_read_byte_method,  	1},
	{"readline",	(PyCFunction) mmap_read_line_method,	1},
	{"resize",	(PyCFunction) mmap_resize_method,	1},
	{"seek",	(PyCFunction) mmap_seek_method,		1},
	{"size",	(PyCFunction) mmap_size_method,		1},
	{"tell",	(PyCFunction) mmap_tell_method,		1},
	{"write",	(PyCFunction) mmap_write_method,	1},
	{"write_byte",	(PyCFunction) mmap_write_byte_method,	1},
	{NULL,	   NULL}       /* sentinel */
};

/* Functions for treating an mmap'ed file as a buffer */

static int
mmap_buffer_getreadbuf(mmap_object *self, int index, const void **ptr)
{
	CHECK_VALID(-1);
	if ( index != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"Accessing non-existent mmap segment");
		return -1;
	}
	*ptr = self->data;
	return self->size;
}

static int
mmap_buffer_getwritebuf(mmap_object *self, int index, const void **ptr)
{  
	CHECK_VALID(-1);
	if ( index != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"Accessing non-existent mmap segment");
		return -1;
	}
	*ptr = self->data;
	return self->size;
}

static int
mmap_buffer_getsegcount(mmap_object *self, int *lenp)
{
	CHECK_VALID(-1);
	if (lenp) 
		*lenp = self->size;
	return 1;
}

static int
mmap_buffer_getcharbuffer(mmap_object *self, int index, const void **ptr)
{
	if ( index != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"accessing non-existent buffer segment");
		return -1;
	}
	*ptr = (const char *)self->data;
	return self->size;
}

static PyObject *
mmap_object_getattr(mmap_object *self, char *name)
{
	return Py_FindMethod (mmap_object_methods, (PyObject *)self, name);
}

static int
mmap_length(mmap_object *self)
{
	CHECK_VALID(-1);
	return self->size;
}

static PyObject *
mmap_item(mmap_object *self, int i)
{
	CHECK_VALID(NULL);
	if (i < 0 || (size_t)i >= self->size) {
		PyErr_SetString(PyExc_IndexError, "mmap index out of range");
		return NULL;
	}
	return PyString_FromStringAndSize(self->data + i, 1);
}

static PyObject *
mmap_slice(mmap_object *self, int ilow, int ihigh)
{
	CHECK_VALID(NULL);
	if (ilow < 0)
		ilow = 0;
	else if ((size_t)ilow > self->size)
		ilow = self->size;
	if (ihigh < 0)
		ihigh = 0;
	if (ihigh < ilow)
		ihigh = ilow;
	else if ((size_t)ihigh > self->size)
		ihigh = self->size;
    
	return PyString_FromStringAndSize(self->data + ilow, ihigh-ilow);
}

static PyObject *
mmap_concat(mmap_object *self, PyObject *bb)
{
	CHECK_VALID(NULL);
	PyErr_SetString(PyExc_SystemError,
			"mmaps don't support concatenation");
	return NULL;
}

static PyObject *
mmap_repeat(mmap_object *self, int n)
{
	CHECK_VALID(NULL);
	PyErr_SetString(PyExc_SystemError,
			"mmaps don't support repeat operation");
	return NULL;
}

static int
mmap_ass_slice(mmap_object *self, int ilow, int ihigh, PyObject *v)
{
	const char *buf;

	CHECK_VALID(-1);
	if (ilow < 0)
		ilow = 0;
	else if ((size_t)ilow > self->size)
		ilow = self->size;
	if (ihigh < 0)
		ihigh = 0;
	if (ihigh < ilow)
		ihigh = ilow;
	else if ((size_t)ihigh > self->size)
		ihigh = self->size;
    
	if (! (PyString_Check(v)) ) {
		PyErr_SetString(PyExc_IndexError, 
				"mmap slice assignment must be a string");
		return -1;
	}
	if ( PyString_Size(v) != (ihigh - ilow) ) {
		PyErr_SetString(PyExc_IndexError, 
				"mmap slice assignment is wrong size");
		return -1;
	}
	buf = PyString_AsString(v);
	memcpy(self->data + ilow, buf, ihigh-ilow);
	return 0;
}

static int
mmap_ass_item(mmap_object *self, int i, PyObject *v)
{
	const char *buf;
 
	CHECK_VALID(-1);
	if (i < 0 || (size_t)i >= self->size) {
		PyErr_SetString(PyExc_IndexError, "mmap index out of range");
		return -1;
	}
	if (! (PyString_Check(v) && PyString_Size(v)==1) ) {
		PyErr_SetString(PyExc_IndexError, 
			"mmap assignment must be single-character string");
		return -1;
	}
	buf = PyString_AsString(v);
	self->data[i] = buf[0];
	return 0;
}

static PySequenceMethods mmap_as_sequence = {
	(inquiry)mmap_length,		       /*sq_length*/
	(binaryfunc)mmap_concat,	       /*sq_concat*/
	(intargfunc)mmap_repeat,	       /*sq_repeat*/
	(intargfunc)mmap_item,		       /*sq_item*/
	(intintargfunc)mmap_slice,	       /*sq_slice*/
	(intobjargproc)mmap_ass_item,	       /*sq_ass_item*/
	(intintobjargproc)mmap_ass_slice,      /*sq_ass_slice*/
};

static PyBufferProcs mmap_as_buffer = {
	(getreadbufferproc)mmap_buffer_getreadbuf,
	(getwritebufferproc)mmap_buffer_getwritebuf,
	(getsegcountproc)mmap_buffer_getsegcount,
	(getcharbufferproc)mmap_buffer_getcharbuffer,
};

static PyTypeObject mmap_object_type = {
	PyObject_HEAD_INIT(0) /* patched in module init */
	0,					/* ob_size */
	"mmap",					/* tp_name */
	sizeof(mmap_object),			/* tp_size */
	0,					/* tp_itemsize */
	/* methods */
	(destructor) mmap_object_dealloc,	/* tp_dealloc */
	0,					/* tp_print */
	(getattrfunc) mmap_object_getattr,	/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	&mmap_as_sequence,			/*tp_as_sequence*/
	0,					/*tp_as_mapping*/
	0,					/*tp_hash*/
	0,					/*tp_call*/
	0,					/*tp_str*/
	0,					/*tp_getattro*/
	0,					/*tp_setattro*/
	&mmap_as_buffer,			/*tp_as_buffer*/
	Py_TPFLAGS_HAVE_GETCHARBUFFER,		/*tp_flags*/
	0,					/*tp_doc*/
};


/* extract the map size from the given PyObject

   The map size is restricted to [0, INT_MAX] because this is the current
   Python limitation on object sizes. Although the mmap object *could* handle
   a larger map size, there is no point because all the useful operations
   (len(), slicing(), sequence indexing) are limited by a C int.

   Returns -1 on error, with an appropriate Python exception raised. On
   success, the map size is returned. */
static int
_GetMapSize(PyObject *o)
{
	if (PyInt_Check(o)) {
		long i = PyInt_AsLong(o);
		if (PyErr_Occurred())
			return -1;
		if (i < 0)
			goto onnegoverflow;
		if (i > INT_MAX)
			goto onposoverflow;
		return (int)i;
	}
	else if (PyLong_Check(o)) {
		long i = PyLong_AsLong(o);
		if (PyErr_Occurred()) {
			/* yes negative overflow is mistaken for positive overflow
			   but not worth the trouble to check sign of 'i' */
			if (PyErr_ExceptionMatches(PyExc_OverflowError))
				goto onposoverflow;
			else
				return -1;
		}
		if (i < 0)
			goto onnegoverflow;
		if (i > INT_MAX)
			goto onposoverflow;
		return (int)i;
	}
	else {
		PyErr_SetString(PyExc_TypeError,
			"map size must be an integral value");
		return -1;
	}

onnegoverflow:
	PyErr_SetString(PyExc_OverflowError,
		"memory mapped size must be positive");
	return -1;

onposoverflow:
	PyErr_SetString(PyExc_OverflowError,
		"memory mapped size is too large (limited by C int)");
	return -1;
}

#ifdef UNIX 
static PyObject *
new_mmap_object(PyObject *self, PyObject *args, PyObject *kwdict)
{
	mmap_object *m_obj;
	PyObject *map_size_obj = NULL;
	int map_size;
	int fd, flags = MAP_SHARED, prot = PROT_WRITE | PROT_READ;
	char *keywords[] = {"file", "size", "flags", "prot", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwdict, 
					 "iO|ii", keywords, 
					 &fd, &map_size_obj, &flags, &prot)
		)
		return NULL;
	map_size = _GetMapSize(map_size_obj);
	if (map_size < 0)
		return NULL;
	
	m_obj = PyObject_New (mmap_object, &mmap_object_type);
	if (m_obj == NULL) {return NULL;}
	m_obj->size = (size_t) map_size;
	m_obj->pos = (size_t) 0;
	m_obj->fd = fd;
	m_obj->data = mmap(NULL, map_size, 
			   prot, flags,
			   fd, 0);
	if (m_obj->data == (void *)-1)
	{
		Py_DECREF(m_obj);
		PyErr_SetFromErrno(mmap_module_error);
		return NULL;
	}
	return (PyObject *)m_obj;
}
#endif /* UNIX */

#ifdef MS_WIN32
static PyObject *
new_mmap_object(PyObject *self, PyObject *args)
{
	mmap_object *m_obj;
	PyObject *map_size_obj = NULL;
	int map_size;
	char *tagname = "";

	DWORD dwErr = 0;
	int fileno;
	HANDLE fh = 0;

	if (!PyArg_ParseTuple(args,
			  "iO|z",
			  &fileno,
			  &map_size_obj,
			  &tagname)
		)
		return NULL;
  
	map_size = _GetMapSize(map_size_obj);
	if (map_size < 0)
		return NULL;
	
	/* if an actual filename has been specified */
	if (fileno != 0) {
		fh = (HANDLE)_get_osfhandle(fileno);
		if (fh==(HANDLE)-1) {
		    PyErr_SetFromErrno(mmap_module_error);
		    return NULL;
		}
		/* Win9x appears to need us seeked to zero */
		fseek(&_iob[fileno], 0, SEEK_SET);
	}

	m_obj = PyObject_New (mmap_object, &mmap_object_type);
	if (m_obj==NULL)
		return NULL;
	/* Set every field to an invalid marker, so we can safely
	   destruct the object in the face of failure */
	m_obj->data = NULL;
	m_obj->file_handle = INVALID_HANDLE_VALUE;
	m_obj->map_handle = INVALID_HANDLE_VALUE;
	m_obj->tagname = NULL;

	if (fh) {
		/* It is necessary to duplicate the handle, so the
		   Python code can close it on us */
		if (!DuplicateHandle(
			    GetCurrentProcess(), /* source process handle */
			    fh, /* handle to be duplicated */
			    GetCurrentProcess(), /* target proc handle */
			    (LPHANDLE)&m_obj->file_handle, /* result */
			    0, /* access - ignored due to options value */
			    FALSE, /* inherited by child processes? */
			    DUPLICATE_SAME_ACCESS)) { /* options */
			dwErr = GetLastError();
			Py_DECREF(m_obj);
			PyErr_SetFromWindowsErr(dwErr);
			return NULL;
		}
		if (!map_size) {
			m_obj->size = GetFileSize (fh, NULL);
		} else {
			m_obj->size = map_size;
		}
	}
	else {
		m_obj->size = map_size;
	}

	/* set the initial position */
	m_obj->pos = (size_t) 0;

	/* set the tag name */
	if (tagname != NULL && *tagname != '\0') {
		m_obj->tagname = PyMem_Malloc(strlen(tagname)+1);
		if (m_obj->tagname == NULL) {
			PyErr_NoMemory();
			Py_DECREF(m_obj);
			return NULL;
		}
		strcpy(m_obj->tagname, tagname);
	}
	else
		m_obj->tagname = NULL;

	m_obj->map_handle = CreateFileMapping (m_obj->file_handle,
					       NULL,
					       PAGE_READWRITE,
					       0,
					       m_obj->size,
					       m_obj->tagname);
	if (m_obj->map_handle != NULL) {
		m_obj->data = (char *) MapViewOfFile (m_obj->map_handle,
						      FILE_MAP_WRITE,
						      0,
						      0,
						      0);
		if (m_obj->data != NULL) {
			return ((PyObject *) m_obj);
		} else {
		    dwErr = GetLastError();
		}
	} else {
		dwErr = GetLastError();
	}
	Py_DECREF(m_obj);
	PyErr_SetFromWindowsErr(dwErr);
	return (NULL);
}
#endif /* MS_WIN32 */

/* List of functions exported by this module */
static struct PyMethodDef mmap_functions[] = {
	{"mmap",	(PyCFunction) new_mmap_object, 
	 METH_VARARGS|METH_KEYWORDS},
	{NULL,		NULL}	     /* Sentinel */
};

#ifdef MS_WIN32
__declspec(dllexport) void
#endif /* MS_WIN32 */
#ifdef UNIX
extern void
#endif

initmmap(void)
{
	PyObject *dict, *module;

	/* Patch the object type */
	mmap_object_type.ob_type = &PyType_Type;

	module = Py_InitModule ("mmap", mmap_functions);
	dict = PyModule_GetDict (module);
	mmap_module_error = PyExc_EnvironmentError;
	Py_INCREF(mmap_module_error);
	PyDict_SetItemString (dict, "error", mmap_module_error);
#ifdef PROT_EXEC
	PyDict_SetItemString (dict, "PROT_EXEC", PyInt_FromLong(PROT_EXEC) );
#endif
#ifdef PROT_READ
	PyDict_SetItemString (dict, "PROT_READ", PyInt_FromLong(PROT_READ) );
#endif
#ifdef PROT_WRITE
	PyDict_SetItemString (dict, "PROT_WRITE", PyInt_FromLong(PROT_WRITE) );
#endif

#ifdef MAP_SHARED
	PyDict_SetItemString (dict, "MAP_SHARED", PyInt_FromLong(MAP_SHARED) );
#endif
#ifdef MAP_PRIVATE
	PyDict_SetItemString (dict, "MAP_PRIVATE",
			      PyInt_FromLong(MAP_PRIVATE) );
#endif
#ifdef MAP_DENYWRITE
	PyDict_SetItemString (dict, "MAP_DENYWRITE",
			      PyInt_FromLong(MAP_DENYWRITE) );
#endif
#ifdef MAP_EXECUTABLE
	PyDict_SetItemString (dict, "MAP_EXECUTABLE",
			      PyInt_FromLong(MAP_EXECUTABLE) );
#endif
#ifdef MAP_ANON
	PyDict_SetItemString (dict, "MAP_ANON", PyInt_FromLong(MAP_ANON) );
	PyDict_SetItemString (dict, "MAP_ANONYMOUS",
			      PyInt_FromLong(MAP_ANON) );
#endif

	PyDict_SetItemString (dict, "PAGESIZE",
			      PyInt_FromLong( (long)my_getpagesize() ) );
}

