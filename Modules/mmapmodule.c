/* -*- Mode: C++; tab-width: 4 -*-
 /	Author: Sam Rushing <rushing@nightmare.com>
 /  Hacked for Unix by A.M. Kuchling <amk1@bigfoot.com> 
 /	$Id$

 / mmapmodule.cpp -- map a view of a file into memory
 /
 / todo: need permission flags, perhaps a 'chsize' analog
 /   not all functions check range yet!!!
 /
 /
 / Note: This module currently only deals with 32-bit file
 /   sizes.
 /
 / The latest version of mmapfile is maintained by Sam at
 / ftp://squirl.nightmare.com/pub/python/python-ext.
*/

#ifndef MS_WIN32
#define UNIX
#endif

#ifdef MS_WIN32
#include <windows.h>
#endif

#ifdef UNIX
#include <unistd.h>
#include <sys/mman.h>
#endif

#include <Python.h>

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
		HFILE		file_handle;
		char *	tagname;
#endif

#ifdef UNIX
  /* No Unix-specific information at this point in time */
#endif
} mmap_object;

static void
mmap_object_dealloc(mmap_object * m_obj)
{
#ifdef MS_WIN32
		UnmapViewOfFile (m_obj->data);
		CloseHandle (m_obj->map_handle);
		CloseHandle ((HANDLE)m_obj->file_handle);
#endif /* MS_WIN32 */

#ifdef UNIX
		if (m_obj->data!=NULL) {
				msync(m_obj->data, m_obj->size, MS_SYNC | MS_INVALIDATE);
				munmap(m_obj->data, m_obj->size);
		}
#endif /* UNIX */

		PyMem_DEL(m_obj);
}

static PyObject *
mmap_close_method (mmap_object * self, PyObject * args)
{
#ifdef MS_WIN32
		UnmapViewOfFile (self->data);
		CloseHandle (self->map_handle);
		CloseHandle ((HANDLE)self->file_handle);
		self->map_handle = (HANDLE) NULL;
#endif /* MS_WIN32 */

#ifdef UNIX
		munmap(self->data, self->size);
		self->data = NULL;
#endif

		Py_INCREF (Py_None);
		return (Py_None);
}

#ifdef MS_WIN32
#define CHECK_VALID(err)  											    \
do {																	\
	if (!self->map_handle) {											 \
		PyErr_SetString (PyExc_ValueError, "mmap closed or invalid");\
		return err;													     \
	}																	 \
} while (0)
#endif /* MS_WIN32 */

#ifdef UNIX
#define CHECK_VALID(err)												\
do {																	\
    if (self->data == NULL) {										    \
    	PyErr_SetString (PyExc_ValueError, "mmap closed or invalid");	\
        return err;														    \
        }																	\
} while (0)
#endif /* UNIX */

static PyObject *
mmap_read_byte_method (mmap_object * self,
						   PyObject * args)
{
		char value;
		char * where = (self->data+self->pos);
		CHECK_VALID(NULL);
		if ((where >= 0) && (where < (self->data+self->size))) {
				value = (char) *(where);
				self->pos += 1;
				return Py_BuildValue("c", (char) *(where));
		} else {
				PyErr_SetString (PyExc_ValueError, "read byte out of range");
				return NULL;
		}
}

static PyObject *
mmap_read_line_method (mmap_object * self,
						   PyObject * args)
{
		char * start;
		char * eof = self->data+self->size;
		char * eol;
		PyObject * result;

		CHECK_VALID(NULL);
		start = self->data+self->pos;

		/* strchr was a bad idea here - there's no way to range
		   check it.  there is no 'strnchr' */
		for (eol = start; (eol < eof) && (*eol != '\n') ; eol++)
		{ /* do nothing */ }

		result = Py_BuildValue("s#", start, (long) (++eol - start));
		self->pos += (eol - start);
		return (result);
}

static PyObject *
mmap_read_method (mmap_object * self,
					  PyObject * args)
{
		long num_bytes;
		PyObject *result;

		CHECK_VALID(NULL);
		if (!PyArg_ParseTuple (args, "l", &num_bytes))
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
mmap_find_method (mmap_object *self,
					  PyObject *args)
{
		long start = self->pos;
		char * needle;
		int len;

		CHECK_VALID(NULL);
		if (!PyArg_ParseTuple (args, "s#|l", &needle, &len, &start)) {
				return NULL;
		} else {
				char * p = self->data+self->pos;
				char * e = self->data+self->size;
				while (p < e) {
						char * s = p;
						char * n = needle;
						while ((s<e) && (*n) && !(*s-*n)) {
								s++, n++;
						}
						if (!*n) {
								return Py_BuildValue ("l", (long) (p - (self->data + start)));
						}
						p++;
				}
				return Py_BuildValue ("l", (long) -1);
		}
}

static PyObject *
mmap_write_method (mmap_object * self,
					   PyObject * args)
{
		long length;
		char * data;

		CHECK_VALID(NULL);
		if (!PyArg_ParseTuple (args, "s#", &data, &length))
				return(NULL);

		if ((self->pos + length) > self->size) {
				PyErr_SetString (mmap_module_error, "data out of range");
				return NULL;
		}
		memcpy (self->data+self->pos, data, length);
		self->pos = self->pos+length;
		Py_INCREF (Py_None);
		return (Py_None);
}

static PyObject *
mmap_write_byte_method (mmap_object * self,
							PyObject * args)
{
		char value;

		CHECK_VALID(NULL);
		if (!PyArg_ParseTuple (args, "c", &value))
				return(NULL);

		*(self->data+self->pos) = value;
		self->pos += 1;
		Py_INCREF (Py_None);
		return (Py_None);
}

static PyObject *
mmap_size_method (mmap_object * self,
					  PyObject * args)
{
		CHECK_VALID(NULL);

#ifdef MS_WIN32
		if (self->file_handle != (HFILE) 0xFFFFFFFF) {
				return (Py_BuildValue ("l", GetFileSize ((HANDLE)self->file_handle, NULL)));
		} else {
				return (Py_BuildValue ("l", self->size) );
		}
#endif /* MS_WIN32 */

#ifdef UNIX
		return (Py_BuildValue ("l", self->size) );
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
mmap_resize_method (mmap_object * self,
						PyObject * args)
{
		unsigned long new_size;
		CHECK_VALID(NULL);
		if (!PyArg_ParseTuple (args, "l", &new_size)) {
				return NULL;
#ifdef MS_WIN32
		} else { 
				/* First, unmap the file view */
				UnmapViewOfFile (self->data);
				/* Close the mapping object */
				CloseHandle ((HANDLE)self->map_handle);
				/* Move to the desired EOF position */
				SetFilePointer ((HANDLE)self->file_handle, new_size, NULL, FILE_BEGIN);
				/* Change the size of the file */
				SetEndOfFile ((HANDLE)self->file_handle);
				/* Create another mapping object and remap the file view */
				self->map_handle = CreateFileMapping ((HANDLE) self->file_handle,
													  NULL,
													  PAGE_READWRITE,
													  0,
													  new_size,
													  self->tagname);
				char complaint[256];
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
								sprintf (complaint, "MapViewOfFile failed: %ld", GetLastError());
						}
				} else {
						sprintf (complaint, "CreateFileMapping failed: %ld", GetLastError());
				}
				PyErr_SetString (mmap_module_error, complaint);
				return (NULL);
		}
#endif /* MS_WIN32 */

#ifdef UNIX
#ifndef MREMAP_MAYMOVE
} else {
		PyErr_SetString(PyExc_SystemError, "mmap: resizing not available--no mremap()");
		return NULL;
#else
} else {
		void *newmap;

		newmap = mremap(self->data, self->size, new_size, MREMAP_MAYMOVE);
		if (newmap == (void *)-1) 
		{
				PyErr_SetFromErrno(mmap_module_error);
				return NULL;
		}
		self->data = newmap;
		self->size = new_size;
		Py_INCREF(Py_None);
		return Py_None;
#endif /* MREMAP_MAYMOVE */
#endif /* UNIX */
}
}

static PyObject *
mmap_tell_method (mmap_object * self, PyObject * args)
{
		CHECK_VALID(NULL);
		return (Py_BuildValue ("l", self->pos) );
}

static PyObject *
mmap_flush_method (mmap_object * self, PyObject * args)
{
		size_t offset	= 0;
		size_t size	= self->size;
		CHECK_VALID(NULL);
		if (!PyArg_ParseTuple (args, "|ll", &offset, &size)) {
				return NULL;
		} else if ((offset + size) > self->size) {
				PyErr_SetString (PyExc_ValueError, "flush values out of range");
				return NULL;
		} else {
#ifdef MS_WIN32
				return (Py_BuildValue ("l", FlushViewOfFile (self->data+offset, size)));
#endif /* MS_WIN32 */
#ifdef UNIX
                /* XXX semantics of return value? */
                /* XXX flags for msync? */
				if (-1 == msync(self->data + offset, size, MS_SYNC | MS_INVALIDATE))
				{
						PyErr_SetFromErrno(mmap_module_error);
						return NULL;
				}
				return Py_BuildValue ("l", 0);	
#endif /* UNIX */	
		}
}

static PyObject *
mmap_seek_method (mmap_object * self, PyObject * args)
{
		/* ptrdiff_t dist; */
		long dist;
		int how=0;
		CHECK_VALID(NULL);
		if (!PyArg_ParseTuple (args, "l|i", &dist, &how)) {
				return(NULL);
		} else {
				unsigned long where;
				switch (how) {
				case 0:
						where = dist;
						break;
				case 1:
						where = self->pos + dist;
						break;
				case 2:
						where = self->size - dist;
						break;
				default:
						PyErr_SetString (PyExc_ValueError, "unknown seek type");
						return NULL;
				}
				if ((where >= 0) && (where < (self->size))) {
						self->pos = where;
						Py_INCREF (Py_None);
						return (Py_None);
				} else {
						PyErr_SetString (PyExc_ValueError, "seek out of range");
						return NULL;
				}
		}
}

static PyObject *
mmap_move_method (mmap_object * self, PyObject * args)
{
		unsigned long dest, src, count;
		CHECK_VALID(NULL);
		if (!PyArg_ParseTuple (args, "iii", &dest, &src, &count)) {
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
		{"close",		(PyCFunction) mmap_close_method,		1},
		{"find",		(PyCFunction) mmap_find_method,			1},
		{"flush",		(PyCFunction) mmap_flush_method,		1},
		{"move",		(PyCFunction) mmap_move_method,			1},
		{"read",		(PyCFunction) mmap_read_method,			1},
		{"read_byte", (PyCFunction) mmap_read_byte_method,	1},
		{"readline",	(PyCFunction) mmap_read_line_method,	1},
		{"resize",	(PyCFunction) mmap_resize_method,		1},
		{"seek",		(PyCFunction) mmap_seek_method,			1},
		{"size",		(PyCFunction) mmap_size_method,			1},
		{"tell",		(PyCFunction) mmap_tell_method,			1},
		{"write",		(PyCFunction) mmap_write_method,		1},
		{"write_byte",(PyCFunction) mmap_write_byte_method,	1},
		{NULL,	   NULL}	   /* sentinel */
};

/* Functions for treating an mmap'ed file as a buffer */

static int
mmap_buffer_getreadbuf(self, index, ptr)
        mmap_object *self;
int index;
const void **ptr;
{
		CHECK_VALID(-1);
		if ( index != 0 ) {
				PyErr_SetString(PyExc_SystemError, "Accessing non-existent mmap segment");
				return -1;
		}
		*ptr = self->data;
		return self->size;
}

static int
mmap_buffer_getwritebuf(self, index, ptr)
        mmap_object *self;
int index;
const void **ptr;
{  
		CHECK_VALID(-1);
		if ( index != 0 ) {
				PyErr_SetString(PyExc_SystemError, "Accessing non-existent mmap segment");
				return -1;
		}
		*ptr = self->data;
		return self->size;
}

static int
mmap_buffer_getsegcount(self, lenp)
        mmap_object *self;
int *lenp;
{
		CHECK_VALID(-1);
		if (lenp) 
				*lenp = self->size;
		return 1;
}

static int
mmap_buffer_getcharbuffer(self, index, ptr)
        mmap_object *self;
int index;
const void **ptr;
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
mmap_object_getattr(mmap_object * self, char * name)
{
		return Py_FindMethod (mmap_object_methods, (PyObject *)self, name);
}

static int
mmap_length(self)
        mmap_object *self;
{
		CHECK_VALID(-1);
		return self->size;
}

static PyObject *
mmap_item(self, i)
        mmap_object *self;
int i;
{
		CHECK_VALID(NULL);
		if (i < 0 || i >= self->size) {
				PyErr_SetString(PyExc_IndexError, "mmap index out of range");
				return NULL;
		}
		return PyString_FromStringAndSize(self->data + i, 1);
}

static PyObject *
mmap_slice(self, ilow, ihigh)
        mmap_object *self;
int ilow, ihigh;
{
		CHECK_VALID(NULL);
		if (ilow < 0)
				ilow = 0;
		else if (ilow > self->size)
				ilow = self->size;
		if (ihigh < 0)
				ihigh = 0;
		if (ihigh < ilow)
				ihigh = ilow;
		else if (ihigh > self->size)
				ihigh = self->size;
	
		return PyString_FromStringAndSize(self->data + ilow, ihigh-ilow);
}

static PyObject *
mmap_concat(self, bb)
        mmap_object *self;
PyObject *bb;
{
		CHECK_VALID(NULL);
		PyErr_SetString(PyExc_SystemError, "mmaps don't support concatenation");
		return NULL;
}

static PyObject *
mmap_repeat(self, n)
        mmap_object *self;
int n;
{
		CHECK_VALID(NULL);
		PyErr_SetString(PyExc_SystemError, "mmaps don't support repeat operation");
		return NULL;
}

static int
mmap_ass_slice(self, ilow, ihigh, v)
        mmap_object *self;
int ilow, ihigh;
PyObject *v;
{
		unsigned char *buf;

		CHECK_VALID(-1);
		if (ilow < 0)
				ilow = 0;
		else if (ilow > self->size)
				ilow = self->size;
		if (ihigh < 0)
				ihigh = 0;
		if (ihigh < ilow)
				ihigh = ilow;
		else if (ihigh > self->size)
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
mmap_ass_item(self, i, v)
        mmap_object *self;
int i;
PyObject *v;
{
		unsigned char *buf;
 
		CHECK_VALID(-1);
		if (i < 0 || i >= self->size) {
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
        (inquiry)mmap_length,                  /*sq_length*/
        (binaryfunc)mmap_concat,               /*sq_concat*/
        (intargfunc)mmap_repeat,               /*sq_repeat*/
        (intargfunc)mmap_item,                 /*sq_item*/
        (intintargfunc)mmap_slice,             /*sq_slice*/
        (intobjargproc)mmap_ass_item,          /*sq_ass_item*/
        (intintobjargproc)mmap_ass_slice,      /*sq_ass_slice*/
};

static PyBufferProcs mmap_as_buffer = {
        (getreadbufferproc)mmap_buffer_getreadbuf,
        (getwritebufferproc)mmap_buffer_getwritebuf,
        (getsegcountproc)mmap_buffer_getsegcount,
        (getcharbufferproc)mmap_buffer_getcharbuffer,
};

static PyTypeObject mmap_object_type = {
		PyObject_HEAD_INIT(&PyType_Type)
		0,									/* ob_size */
		"mmap",							/* tp_name */
		sizeof(mmap_object),				/* tp_size */
		0,									/* tp_itemsize */
		/* methods */
		(destructor) mmap_object_dealloc, /* tp_dealloc */
		0,									/* tp_print */
		(getattrfunc) mmap_object_getattr,/* tp_getattr */
		0,									/* tp_setattr */
		0,									/* tp_compare */
		0,									/* tp_repr */
		0,									/* tp_as_number */
		&mmap_as_sequence,                /*tp_as_sequence*/
		0,                                    /*tp_as_mapping*/
		0,                                    /*tp_hash*/
		0,                                    /*tp_call*/
		0,                                    /*tp_str*/
		0,                                    /*tp_getattro*/
		0,                                    /*tp_setattro*/
		&mmap_as_buffer,                  /*tp_as_buffer*/
		Py_TPFLAGS_HAVE_GETCHARBUFFER,        /*tp_flags*/
		0,                                    /*tp_doc*/
};

#ifdef UNIX 
static PyObject *
new_mmap_object (PyObject * self, PyObject * args, PyObject *kwdict)
{
		mmap_object * m_obj;
		unsigned long map_size;
		int fd, flags = MAP_SHARED, prot = PROT_WRITE | PROT_READ;
		char * filename;
		int namelen;
		char *keywords[] = {"file", "size", "flags", "prot", NULL};

		if (!PyArg_ParseTupleAndKeywords(args, kwdict, 
										 "il|ii", keywords, 
										 &fd, &map_size, &flags, &prot)
				)
				return NULL;
  
		m_obj = PyObject_NEW (mmap_object, &mmap_object_type);
		if (m_obj == NULL) {return NULL;}
		m_obj->size = (size_t) map_size;
		m_obj->pos = (size_t) 0;
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
new_mmap_object (PyObject * self, PyObject * args)
{
		mmap_object * m_obj;
		unsigned long map_size;
		char * filename;
		int namelen;
		char * tagname;

		char complaint[256];
		HFILE fh = 0;
		OFSTRUCT file_info;

		if (!PyArg_Parse (args,
						  "(s#zl)",
						  &filename,
						  &namelen,
						  &tagname,
						  &map_size)
				)
				return NULL;
  
		/* if an actual filename has been specified */
		if (namelen != 0) {
				fh = OpenFile (filename, &file_info, OF_READWRITE);
				if (fh == HFILE_ERROR) {
						sprintf (complaint, "OpenFile failed: %ld", GetLastError());
						PyErr_SetString(mmap_module_error, complaint);
						return NULL;
				}
		}

		m_obj = PyObject_NEW (mmap_object, &mmap_object_type);
	
		if (fh) {
				m_obj->file_handle = fh;
				if (!map_size) {
						m_obj->size = GetFileSize ((HANDLE)fh, NULL);
				} else {
						m_obj->size = map_size;
				}
		}
		else {
				m_obj->file_handle = (HFILE) 0xFFFFFFFF;
				m_obj->size = map_size;
		}

		/* set the initial position */
		m_obj->pos = (size_t) 0;

		m_obj->map_handle = CreateFileMapping ((HANDLE) m_obj->file_handle,
											   NULL,
											   PAGE_READWRITE,
											   0,
											   m_obj->size,
											   tagname);
		if (m_obj->map_handle != NULL) {
				m_obj->data = (char *) MapViewOfFile (m_obj->map_handle,
													  FILE_MAP_WRITE,
													  0,
													  0,
													  0);
				if (m_obj->data != NULL) {
						return ((PyObject *) m_obj);
				} else {
						sprintf (complaint, "MapViewOfFile failed: %ld", GetLastError());
				}
		} else {
				sprintf (complaint, "CreateFileMapping failed: %ld", GetLastError());
		}
		PyErr_SetString (mmap_module_error, complaint);
		return (NULL);
}
#endif /* MS_WIN32 */

/* List of functions exported by this module */
static struct PyMethodDef mmap_functions[] = {
#ifdef MS_WIN32
		{"mmap",		(PyCFunction) new_mmap_object},
#endif /* MS_WIN32 */

#ifdef UNIX
		{"mmap",		(PyCFunction) new_mmap_object, 
		 METH_VARARGS|METH_KEYWORDS},
#endif /* UNIX */
		{NULL,			NULL}		 /* Sentinel */
};

#ifdef MS_WIN32
extern"C" __declspec(dllexport) void
#endif /* MS_WIN32 */
#ifdef UNIX
extern void
#endif

initmmap(void)
{
		PyObject *dict, *module;
		module = Py_InitModule ("mmap", mmap_functions);
		dict = PyModule_GetDict (module);
		mmap_module_error = PyString_FromString ("mmap error");
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
		PyDict_SetItemString (dict, "MAP_PRIVATE", PyInt_FromLong(MAP_PRIVATE) );
#endif
#ifdef MAP_DENYWRITE
		PyDict_SetItemString (dict, "MAP_DENYWRITE", PyInt_FromLong(MAP_DENYWRITE) );
#endif
#ifdef MAP_EXECUTABLE
		PyDict_SetItemString (dict, "MAP_EXECUTABLE", PyInt_FromLong(MAP_EXECUTABLE) );
#endif
#ifdef MAP_ANON
		PyDict_SetItemString (dict, "MAP_ANON", PyInt_FromLong(MAP_ANON) );
		PyDict_SetItemString (dict, "MAP_ANONYMOUS", PyInt_FromLong(MAP_ANON) );
#endif

#ifdef UNIX
		PyDict_SetItemString (dict, "PAGESIZE", PyInt_FromLong( (long)getpagesize() ) );
#endif

}



