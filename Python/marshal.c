
/* Write Python objects to files and read them back.
   This is intended for writing and reading compiled Python code only;
   a true persistent storage facility would be much harder, since
   it would have to take circular links and sharing into account. */

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "longintrepr.h"
#include "code.h"
#include "marshal.h"

/* High water mark to determine when the marshalled object is dangerously deep
 * and risks coring the interpreter.  When the object stack gets this deep,
 * raise an exception instead of continuing.
 * On Windows debug builds, reduce this value.
 */
#if defined(MS_WINDOWS) && defined(_DEBUG)
#define MAX_MARSHAL_STACK_DEPTH 1500
#else
#define MAX_MARSHAL_STACK_DEPTH 2000
#endif

#define TYPE_NULL		'0'
#define TYPE_NONE		'N'
#define TYPE_FALSE		'F'
#define TYPE_TRUE		'T'
#define TYPE_STOPITER		'S'
#define TYPE_ELLIPSIS   	'.'
#define TYPE_INT		'i'
#define TYPE_INT64		'I'
#define TYPE_FLOAT		'f'
#define TYPE_BINARY_FLOAT	'g'
#define TYPE_COMPLEX		'x'
#define TYPE_BINARY_COMPLEX	'y'
#define TYPE_LONG		'l'
#define TYPE_STRING		's'
#define TYPE_TUPLE		'('
#define TYPE_LIST		'['
#define TYPE_DICT		'{'
#define TYPE_CODE		'c'
#define TYPE_UNICODE		'u'
#define TYPE_UNKNOWN		'?'
#define TYPE_SET		'<'
#define TYPE_FROZENSET  	'>'

typedef struct {
	FILE *fp;
	int error;
	int depth;
	/* If fp == NULL, the following are valid: */
	PyObject *str;
	char *ptr;
	char *end;
	PyObject *strings; /* dict on marshal, list on unmarshal */
	int version;
} WFILE;

#define w_byte(c, p) if (((p)->fp)) putc((c), (p)->fp); \
		      else if ((p)->ptr != (p)->end) *(p)->ptr++ = (c); \
			   else w_more(c, p)

static void
w_more(int c, WFILE *p)
{
	Py_ssize_t size, newsize;
	if (p->str == NULL)
		return; /* An error already occurred */
	size = PyString_Size(p->str);
	newsize = size + 1024;
	if (_PyString_Resize(&p->str, newsize) != 0) {
		p->ptr = p->end = NULL;
	}
	else {
		p->ptr = PyString_AS_STRING((PyStringObject *)p->str) + size;
		p->end =
			PyString_AS_STRING((PyStringObject *)p->str) + newsize;
		*p->ptr++ = Py_SAFE_DOWNCAST(c, int, char);
	}
}

static void
w_string(char *s, int n, WFILE *p)
{
	if (p->fp != NULL) {
		fwrite(s, 1, n, p->fp);
	}
	else {
		while (--n >= 0) {
			w_byte(*s, p);
			s++;
		}
	}
}

static void
w_short(int x, WFILE *p)
{
	w_byte((char)( x      & 0xff), p);
	w_byte((char)((x>> 8) & 0xff), p);
}

static void
w_long(long x, WFILE *p)
{
	w_byte((char)( x      & 0xff), p);
	w_byte((char)((x>> 8) & 0xff), p);
	w_byte((char)((x>>16) & 0xff), p);
	w_byte((char)((x>>24) & 0xff), p);
}

#if SIZEOF_LONG > 4
static void
w_long64(long x, WFILE *p)
{
	w_long(x, p);
	w_long(x>>32, p);
}
#endif

static void
w_object(PyObject *v, WFILE *p)
{
	Py_ssize_t i, n;

	p->depth++;

	if (p->depth > MAX_MARSHAL_STACK_DEPTH) {
		p->error = 2;
	}
	else if (v == NULL) {
		w_byte(TYPE_NULL, p);
	}
	else if (v == Py_None) {
		w_byte(TYPE_NONE, p);
	}
	else if (v == PyExc_StopIteration) {
		w_byte(TYPE_STOPITER, p);
	}
	else if (v == Py_Ellipsis) {
	        w_byte(TYPE_ELLIPSIS, p);
	}
	else if (v == Py_False) {
	        w_byte(TYPE_FALSE, p);
	}
	else if (v == Py_True) {
	        w_byte(TYPE_TRUE, p);
	}
	else if (PyLong_Check(v)) {
		long x = PyLong_AsLong(v);
		if ((x == -1)  && PyErr_Occurred()) {
			PyLongObject *ob = (PyLongObject *)v;
			PyErr_Clear();
			w_byte(TYPE_LONG, p);
			n = Py_Size(ob);
			w_long((long)n, p);
			if (n < 0)
				n = -n;
			for (i = 0; i < n; i++)
				w_short(ob->ob_digit[i], p);
		} 
		else {
#if SIZEOF_LONG > 4
			long y = Py_ARITHMETIC_RIGHT_SHIFT(long, x, 31);
			if (y && y != -1) {
				w_byte(TYPE_INT64, p);
				w_long64(x, p);
			}
			else
#endif
			{
				w_byte(TYPE_INT, p);
				w_long(x, p);
			}
		}
	}
	else if (PyFloat_Check(v)) {
		if (p->version > 1) {
			unsigned char buf[8];
			if (_PyFloat_Pack8(PyFloat_AsDouble(v), 
					   buf, 1) < 0) {
				p->error = 1;
				return;
			}
			w_byte(TYPE_BINARY_FLOAT, p);
			w_string((char*)buf, 8, p);
		}
		else {
			char buf[256]; /* Plenty to format any double */
			n = _PyFloat_Repr(PyFloat_AS_DOUBLE(v),
					  buf, sizeof(buf));
			w_byte(TYPE_FLOAT, p);
			w_byte((int)n, p);
			w_string(buf, (int)n, p);
		}
	}
#ifndef WITHOUT_COMPLEX
	else if (PyComplex_Check(v)) {
		if (p->version > 1) {
			unsigned char buf[8];
			if (_PyFloat_Pack8(PyComplex_RealAsDouble(v),
					   buf, 1) < 0) {
				p->error = 1;
				return;
			}
			w_byte(TYPE_BINARY_COMPLEX, p);
			w_string((char*)buf, 8, p);
			if (_PyFloat_Pack8(PyComplex_ImagAsDouble(v), 
					   buf, 1) < 0) {
				p->error = 1;
				return;
			}
			w_string((char*)buf, 8, p);
		}
		else {
			char buf[256]; /* Plenty to format any double */
			w_byte(TYPE_COMPLEX, p);
			n = _PyFloat_Repr(PyComplex_RealAsDouble(v),
					  buf, sizeof(buf));
			n = strlen(buf);
			w_byte((int)n, p);
			w_string(buf, (int)n, p);
			n = _PyFloat_Repr(PyComplex_ImagAsDouble(v),
					  buf, sizeof(buf));
			w_byte((int)n, p);
			w_string(buf, (int)n, p);
		}
	}
#endif
	else if (PyString_Check(v)) {
		w_byte(TYPE_STRING, p);
		n = PyString_GET_SIZE(v);
		if (n > INT_MAX) {
			/* huge strings are not supported */
			p->depth--;
			p->error = 1;
			return;
		}
		w_long((long)n, p);
		w_string(PyString_AS_STRING(v), (int)n, p);
	}
	else if (PyUnicode_Check(v)) {
	        PyObject *utf8;
		utf8 = PyUnicode_AsUTF8String(v);
		if (utf8 == NULL) {
			p->depth--;
			p->error = 1;
			return;
		}
		w_byte(TYPE_UNICODE, p);
		n = PyString_GET_SIZE(utf8);
		if (n > INT_MAX) {
			p->depth--;
			p->error = 1;
			return;
		}
		w_long((long)n, p);
		w_string(PyString_AS_STRING(utf8), (int)n, p);
		Py_DECREF(utf8);
	}
	else if (PyTuple_Check(v)) {
		w_byte(TYPE_TUPLE, p);
		n = PyTuple_Size(v);
		w_long((long)n, p);
		for (i = 0; i < n; i++) {
			w_object(PyTuple_GET_ITEM(v, i), p);
		}
	}
	else if (PyList_Check(v)) {
		w_byte(TYPE_LIST, p);
		n = PyList_GET_SIZE(v);
		w_long((long)n, p);
		for (i = 0; i < n; i++) {
			w_object(PyList_GET_ITEM(v, i), p);
		}
	}
	else if (PyDict_Check(v)) {
		Py_ssize_t pos;
		PyObject *key, *value;
		w_byte(TYPE_DICT, p);
		/* This one is NULL object terminated! */
		pos = 0;
		while (PyDict_Next(v, &pos, &key, &value)) {
			w_object(key, p);
			w_object(value, p);
		}
		w_object((PyObject *)NULL, p);
	}
	else if (PyAnySet_Check(v)) {
		PyObject *value, *it;

		if (PyObject_TypeCheck(v, &PySet_Type))
			w_byte(TYPE_SET, p);
		else
			w_byte(TYPE_FROZENSET, p);
		n = PyObject_Size(v);
		if (n == -1) {
			p->depth--;
			p->error = 1;
			return;
		}
		w_long((long)n, p);
		it = PyObject_GetIter(v);
		if (it == NULL) {
			p->depth--;
			p->error = 1;
			return;
		}
		while ((value = PyIter_Next(it)) != NULL) {
			w_object(value, p);
			Py_DECREF(value);
		}
		Py_DECREF(it);
		if (PyErr_Occurred()) {
			p->depth--;
			p->error = 1;
			return;
		}
	}
	else if (PyCode_Check(v)) {
		PyCodeObject *co = (PyCodeObject *)v;
		w_byte(TYPE_CODE, p);
		w_long(co->co_argcount, p);
		w_long(co->co_kwonlyargcount, p);
		w_long(co->co_nlocals, p);
		w_long(co->co_stacksize, p);
		w_long(co->co_flags, p);
		w_object(co->co_code, p);
		w_object(co->co_consts, p);
		w_object(co->co_names, p);
		w_object(co->co_varnames, p);
		w_object(co->co_freevars, p);
		w_object(co->co_cellvars, p);
		w_object(co->co_filename, p);
		w_object(co->co_name, p);
		w_long(co->co_firstlineno, p);
		w_object(co->co_lnotab, p);
	}
	else if (PyObject_CheckBuffer(v)) {
		/* Write unknown buffer-style objects as a string */
		char *s;
		PyBufferProcs *pb = v->ob_type->tp_as_buffer;
                Py_buffer view;
		if ((*pb->bf_getbuffer)(v, &view, PyBUF_SIMPLE) != 0) {
                        w_byte(TYPE_UNKNOWN, p);
                        p->error = 1;
                }
		w_byte(TYPE_STRING, p);
                n = view.len;
                s = view.buf;                        
		if (n > INT_MAX) {
			p->depth--;
			p->error = 1;
			return;
		}
		w_long((long)n, p);
		w_string(s, (int)n, p);
                if (pb->bf_releasebuffer != NULL)
                        (*pb->bf_releasebuffer)(v, &view);
	}
	else {
		w_byte(TYPE_UNKNOWN, p);
		p->error = 1;
	}
	p->depth--;
}

/* version currently has no effect for writing longs. */
void
PyMarshal_WriteLongToFile(long x, FILE *fp, int version)
{
	WFILE wf;
	wf.fp = fp;
	wf.error = 0;
	wf.depth = 0;
	wf.strings = NULL;
	wf.version = version;
	w_long(x, &wf);
}

void
PyMarshal_WriteObjectToFile(PyObject *x, FILE *fp, int version)
{
	WFILE wf;
	wf.fp = fp;
	wf.error = 0;
	wf.depth = 0;
	wf.strings = (version > 0) ? PyDict_New() : NULL;
	wf.version = version;
	w_object(x, &wf);
	Py_XDECREF(wf.strings);
}

typedef WFILE RFILE; /* Same struct with different invariants */

#define rs_byte(p) (((p)->ptr < (p)->end) ? (unsigned char)*(p)->ptr++ : EOF)

#define r_byte(p) ((p)->fp ? getc((p)->fp) : rs_byte(p))

static int
r_string(char *s, int n, RFILE *p)
{
	if (p->fp != NULL)
		/* The result fits into int because it must be <=n. */
		return (int)fread(s, 1, n, p->fp);
	if (p->end - p->ptr < n)
		n = (int)(p->end - p->ptr);
	memcpy(s, p->ptr, n);
	p->ptr += n;
	return n;
}

static int
r_short(RFILE *p)
{
	register short x;
	x = r_byte(p);
	x |= r_byte(p) << 8;
	/* Sign-extension, in case short greater than 16 bits */
	x |= -(x & 0x8000);
	return x;
}

static long
r_long(RFILE *p)
{
	register long x;
	register FILE *fp = p->fp;
	if (fp) {
		x = getc(fp);
		x |= (long)getc(fp) << 8;
		x |= (long)getc(fp) << 16;
		x |= (long)getc(fp) << 24;
	}
	else {
		x = rs_byte(p);
		x |= (long)rs_byte(p) << 8;
		x |= (long)rs_byte(p) << 16;
		x |= (long)rs_byte(p) << 24;
	}
#if SIZEOF_LONG > 4
	/* Sign extension for 64-bit machines */
	x |= -(x & 0x80000000L);
#endif
	return x;
}

/* r_long64 deals with the TYPE_INT64 code.  On a machine with
   sizeof(long) > 4, it returns a Python int object, else a Python long
   object.  Note that w_long64 writes out TYPE_INT if 32 bits is enough,
   so there's no inefficiency here in returning a PyLong on 32-bit boxes
   for everything written via TYPE_INT64 (i.e., if an int is written via
   TYPE_INT64, it *needs* more than 32 bits).
*/
static PyObject *
r_long64(RFILE *p)
{
	long lo4 = r_long(p);
	long hi4 = r_long(p);
#if SIZEOF_LONG > 4
	long x = (hi4 << 32) | (lo4 & 0xFFFFFFFFL);
	return PyInt_FromLong(x);
#else
	unsigned char buf[8];
	int one = 1;
	int is_little_endian = (int)*(char*)&one;
	if (is_little_endian) {
		memcpy(buf, &lo4, 4);
		memcpy(buf+4, &hi4, 4);
	}
	else {
		memcpy(buf, &hi4, 4);
		memcpy(buf+4, &lo4, 4);
	}
	return _PyLong_FromByteArray(buf, 8, is_little_endian, 1);
#endif
}

static PyObject *
r_object(RFILE *p)
{
	/* NULL is a valid return value, it does not necessarily means that
	   an exception is set. */
	PyObject *v, *v2, *v3;
	long i, n;
	int type = r_byte(p);
	PyObject *retval;

	p->depth++;

	if (p->depth > MAX_MARSHAL_STACK_DEPTH) {
		p->depth--;
		PyErr_SetString(PyExc_ValueError, "recursion limit exceeded");
		return NULL;
	}

	switch (type) {

	case EOF:
		PyErr_SetString(PyExc_EOFError,
				"EOF read where object expected");
		retval = NULL;
		break;

	case TYPE_NULL:
		retval = NULL;
		break;

	case TYPE_NONE:
		Py_INCREF(Py_None);
		retval = Py_None;
		break;

	case TYPE_STOPITER:
		Py_INCREF(PyExc_StopIteration);
		retval = PyExc_StopIteration;
		break;

	case TYPE_ELLIPSIS:
		Py_INCREF(Py_Ellipsis);
		retval = Py_Ellipsis;
		break;

	case TYPE_FALSE:
		Py_INCREF(Py_False);
		retval = Py_False;
		break;

	case TYPE_TRUE:
		Py_INCREF(Py_True);
		retval = Py_True;
		break;

	case TYPE_INT:
		retval = PyInt_FromLong(r_long(p));
		break;

	case TYPE_INT64:
		retval = r_long64(p);
		break;

	case TYPE_LONG:
		{
			int size;
			PyLongObject *ob;
			n = r_long(p);
			if (n < -INT_MAX || n > INT_MAX) {
				PyErr_SetString(PyExc_ValueError,
						"bad marshal data");
				retval = NULL;
				break;
			}
			size = n<0 ? -n : n;
			ob = _PyLong_New(size);
			if (ob == NULL) {
				retval = NULL;
				break;
			}
			Py_Size(ob) = n;
			for (i = 0; i < size; i++) {
				int digit = r_short(p);
				if (digit < 0) {
					Py_DECREF(ob);
					PyErr_SetString(PyExc_ValueError,
							"bad marshal data");
					ob = NULL;
					break;
				}
				if (ob != NULL)
					ob->ob_digit[i] = digit;
			}
			retval = (PyObject *)ob;
			break;
		}

	case TYPE_FLOAT:
		{
			char buf[256];
			double dx;
			n = r_byte(p);
			if (n == EOF || r_string(buf, (int)n, p) != n) {
				PyErr_SetString(PyExc_EOFError,
					"EOF read where object expected");
				retval = NULL;
				break;
			}
			buf[n] = '\0';
			retval = NULL;
			PyFPE_START_PROTECT("atof", break)
			dx = PyOS_ascii_atof(buf);
			PyFPE_END_PROTECT(dx)
			retval = PyFloat_FromDouble(dx);
			break;
		}

	case TYPE_BINARY_FLOAT:
		{
			unsigned char buf[8];
			double x;
			if (r_string((char*)buf, 8, p) != 8) {
				PyErr_SetString(PyExc_EOFError,
					"EOF read where object expected");
				retval = NULL;
				break;
			}
			x = _PyFloat_Unpack8(buf, 1);
			if (x == -1.0 && PyErr_Occurred()) {
				retval = NULL;
				break;
			}
			retval = PyFloat_FromDouble(x);
			break;
		}

#ifndef WITHOUT_COMPLEX
	case TYPE_COMPLEX:
		{
			char buf[256];
			Py_complex c;
			n = r_byte(p);
			if (n == EOF || r_string(buf, (int)n, p) != n) {
				PyErr_SetString(PyExc_EOFError,
					"EOF read where object expected");
				retval = NULL;
				break;
			}
			buf[n] = '\0';
			retval = NULL;
			PyFPE_START_PROTECT("atof", break;)
			c.real = PyOS_ascii_atof(buf);
			PyFPE_END_PROTECT(c)
			n = r_byte(p);
			if (n == EOF || r_string(buf, (int)n, p) != n) {
				PyErr_SetString(PyExc_EOFError,
					"EOF read where object expected");
				retval = NULL;
				break;
			}
			buf[n] = '\0';
			PyFPE_START_PROTECT("atof", break)
			c.imag = PyOS_ascii_atof(buf);
			PyFPE_END_PROTECT(c)
			retval = PyComplex_FromCComplex(c);
			break;
		}

	case TYPE_BINARY_COMPLEX:
		{
			unsigned char buf[8];
			Py_complex c;
			if (r_string((char*)buf, 8, p) != 8) {
				PyErr_SetString(PyExc_EOFError,
					"EOF read where object expected");
				retval = NULL;
				break;
			}
			c.real = _PyFloat_Unpack8(buf, 1);
			if (c.real == -1.0 && PyErr_Occurred()) {
				retval = NULL;
				break;
			}
			if (r_string((char*)buf, 8, p) != 8) {
				PyErr_SetString(PyExc_EOFError,
					"EOF read where object expected");
				retval = NULL;
				break;
			}
			c.imag = _PyFloat_Unpack8(buf, 1);
			if (c.imag == -1.0 && PyErr_Occurred()) {
				retval = NULL;
				break;
			}
			retval = PyComplex_FromCComplex(c);
			break;
		}
#endif

	case TYPE_STRING:
		n = r_long(p);
		if (n < 0 || n > INT_MAX) {
			PyErr_SetString(PyExc_ValueError, "bad marshal data");
			retval = NULL;
			break;
		}
		v = PyString_FromStringAndSize((char *)NULL, n);
		if (v == NULL) {
			retval = NULL;
			break;
		}
		if (r_string(PyString_AS_STRING(v), (int)n, p) != n) {
			Py_DECREF(v);
			PyErr_SetString(PyExc_EOFError,
					"EOF read where object expected");
			retval = NULL;
			break;
		}
		retval = v;
		break;

	case TYPE_UNICODE:
	    {
		char *buffer;

		n = r_long(p);
		if (n < 0 || n > INT_MAX) {
			PyErr_SetString(PyExc_ValueError, "bad marshal data");
			retval = NULL;
			break;
		}
		buffer = PyMem_NEW(char, n);
		if (buffer == NULL) {
			retval = PyErr_NoMemory();
			break;
		}
		if (r_string(buffer, (int)n, p) != n) {
			PyMem_DEL(buffer);
			PyErr_SetString(PyExc_EOFError,
				"EOF read where object expected");
			retval = NULL;
			break;
		}
		v = PyUnicode_DecodeUTF8(buffer, n, NULL);
		PyMem_DEL(buffer);
		retval = v;
		break;
	    }

	case TYPE_TUPLE:
		n = r_long(p);
		if (n < 0 || n > INT_MAX) {
			PyErr_SetString(PyExc_ValueError, "bad marshal data");
			retval = NULL;
			break;
		}
		v = PyTuple_New((int)n);
		if (v == NULL) {
			retval = NULL;
			break;
		}
		for (i = 0; i < n; i++) {
			v2 = r_object(p);
			if ( v2 == NULL ) {
				if (!PyErr_Occurred())
					PyErr_SetString(PyExc_TypeError,
						"NULL object in marshal data");
				Py_DECREF(v);
				v = NULL;
				break;
			}
			PyTuple_SET_ITEM(v, (int)i, v2);
		}
		retval = v;
		break;

	case TYPE_LIST:
		n = r_long(p);
		if (n < 0 || n > INT_MAX) {
			PyErr_SetString(PyExc_ValueError, "bad marshal data");
			retval = NULL;
			break;
		}
		v = PyList_New((int)n);
		if (v == NULL) {
			retval = NULL;
			break;
		}
		for (i = 0; i < n; i++) {
			v2 = r_object(p);
			if ( v2 == NULL ) {
				if (!PyErr_Occurred())
					PyErr_SetString(PyExc_TypeError,
						"NULL object in marshal data");
				Py_DECREF(v);
				v = NULL;
				break;
			}
			PyList_SET_ITEM(v, (int)i, v2);
		}
		retval = v;
		break;

	case TYPE_DICT:
		v = PyDict_New();
		if (v == NULL) {
			retval = NULL;
			break;
		}
		for (;;) {
			PyObject *key, *val;
			key = r_object(p);
			if (key == NULL)
				break;
			val = r_object(p);
			if (val != NULL)
				PyDict_SetItem(v, key, val);
			Py_DECREF(key);
			Py_XDECREF(val);
		}
		if (PyErr_Occurred()) {
			Py_DECREF(v);
			v = NULL;
		}
		retval = v;
		break;

	case TYPE_SET:
	case TYPE_FROZENSET:
		n = r_long(p);
		if (n < 0 || n > INT_MAX) {
			PyErr_SetString(PyExc_ValueError, "bad marshal data");
			retval = NULL;
			break;
		}
		v = PyTuple_New((int)n);
		if (v == NULL) {
			retval = NULL;
			break;
		}
		for (i = 0; i < n; i++) {
			v2 = r_object(p);
			if ( v2 == NULL ) {
				if (!PyErr_Occurred())
					PyErr_SetString(PyExc_TypeError,
						"NULL object in marshal data");
				Py_DECREF(v);
				v = NULL;
				break;
			}
			PyTuple_SET_ITEM(v, (int)i, v2);
		}
		if (v == NULL) {
			retval = NULL;
			break;
		}
		if (type == TYPE_SET)
			v3 = PySet_New(v);
		else
			v3 = PyFrozenSet_New(v);
		Py_DECREF(v);
		retval = v3;
		break;

	case TYPE_CODE:
		{
			int argcount;
			int kwonlyargcount;
			int nlocals;
			int stacksize;
			int flags;
			PyObject *code = NULL;
			PyObject *consts = NULL;
			PyObject *names = NULL;
			PyObject *varnames = NULL;
			PyObject *freevars = NULL;
			PyObject *cellvars = NULL;
			PyObject *filename = NULL;
			PyObject *name = NULL;
			int firstlineno;
			PyObject *lnotab = NULL;
			
			v = NULL;

			/* XXX ignore long->int overflows for now */
			argcount = (int)r_long(p);
			kwonlyargcount = (int)r_long(p);
			nlocals = (int)r_long(p);
			stacksize = (int)r_long(p);
			flags = (int)r_long(p);
			code = r_object(p);
			if (code == NULL)
				goto code_error;
			consts = r_object(p);
			if (consts == NULL)
				goto code_error;
			names = r_object(p);
			if (names == NULL)
				goto code_error;
			varnames = r_object(p);
			if (varnames == NULL)
				goto code_error;
			freevars = r_object(p);
			if (freevars == NULL)
				goto code_error;
			cellvars = r_object(p);
			if (cellvars == NULL)
				goto code_error;
			filename = r_object(p);
			if (filename == NULL)
				goto code_error;
			name = r_object(p);
			if (name == NULL)
				goto code_error;
			firstlineno = (int)r_long(p);
			lnotab = r_object(p);
			if (lnotab == NULL)
				goto code_error;

			v = (PyObject *) PyCode_New(
					argcount, kwonlyargcount,
					nlocals, stacksize, flags,
					code, consts, names, varnames,
					freevars, cellvars, filename, name,
					firstlineno, lnotab);

		  code_error:
			Py_XDECREF(code);
			Py_XDECREF(consts);
			Py_XDECREF(names);
			Py_XDECREF(varnames);
			Py_XDECREF(freevars);
			Py_XDECREF(cellvars);
			Py_XDECREF(filename);
			Py_XDECREF(name);
			Py_XDECREF(lnotab);

			return v;
		}
		retval = v;
		break;

	default:
		/* Bogus data got written, which isn't ideal.
		   This will let you keep working and recover. */
		PyErr_SetString(PyExc_ValueError, "bad marshal data");
		retval = NULL;
		break;

	}
	p->depth--;
	return retval;
}

static PyObject *
read_object(RFILE *p)
{
	PyObject *v;
	if (PyErr_Occurred()) {
		fprintf(stderr, "XXX readobject called with exception set\n");
		return NULL;
	}
	v = r_object(p);
	if (v == NULL && !PyErr_Occurred())
		PyErr_SetString(PyExc_TypeError, "NULL object in marshal data");
	return v;
}

int
PyMarshal_ReadShortFromFile(FILE *fp)
{
	RFILE rf;
	assert(fp);
	rf.fp = fp;
	rf.strings = NULL;
	rf.end = rf.ptr = NULL;
	return r_short(&rf);
}

long
PyMarshal_ReadLongFromFile(FILE *fp)
{
	RFILE rf;
	rf.fp = fp;
	rf.strings = NULL;
	rf.ptr = rf.end = NULL;
	return r_long(&rf);
}

#ifdef HAVE_FSTAT
/* Return size of file in bytes; < 0 if unknown. */
static off_t
getfilesize(FILE *fp)
{
	struct stat st;
	if (fstat(fileno(fp), &st) != 0)
		return -1;
	else
		return st.st_size;
}
#endif

/* If we can get the size of the file up-front, and it's reasonably small,
 * read it in one gulp and delegate to ...FromString() instead.  Much quicker
 * than reading a byte at a time from file; speeds .pyc imports.
 * CAUTION:  since this may read the entire remainder of the file, don't
 * call it unless you know you're done with the file.
 */
PyObject *
PyMarshal_ReadLastObjectFromFile(FILE *fp)
{
/* 75% of 2.1's .pyc files can exploit SMALL_FILE_LIMIT.
 * REASONABLE_FILE_LIMIT is by defn something big enough for Tkinter.pyc.
 */
#define SMALL_FILE_LIMIT (1L << 14)
#define REASONABLE_FILE_LIMIT (1L << 18)
#ifdef HAVE_FSTAT
	off_t filesize;
#endif
#ifdef HAVE_FSTAT
	filesize = getfilesize(fp);
	if (filesize > 0) {
		char buf[SMALL_FILE_LIMIT];
		char* pBuf = NULL;
		if (filesize <= SMALL_FILE_LIMIT)
			pBuf = buf;
		else if (filesize <= REASONABLE_FILE_LIMIT)
			pBuf = (char *)PyMem_MALLOC(filesize);
		if (pBuf != NULL) {
			PyObject* v;
			size_t n;
			/* filesize must fit into an int, because it
			   is smaller than REASONABLE_FILE_LIMIT */
			n = fread(pBuf, 1, (int)filesize, fp);
			v = PyMarshal_ReadObjectFromString(pBuf, n);
			if (pBuf != buf)
				PyMem_FREE(pBuf);
			return v;
		}

	}
#endif
	/* We don't have fstat, or we do but the file is larger than
	 * REASONABLE_FILE_LIMIT or malloc failed -- read a byte at a time.
	 */
	return PyMarshal_ReadObjectFromFile(fp);

#undef SMALL_FILE_LIMIT
#undef REASONABLE_FILE_LIMIT
}

PyObject *
PyMarshal_ReadObjectFromFile(FILE *fp)
{
	RFILE rf;
	PyObject *result;
	rf.fp = fp;
	rf.strings = PyList_New(0);
	rf.depth = 0;
	rf.ptr = rf.end = NULL;
	result = r_object(&rf);
	Py_DECREF(rf.strings);
	return result;
}

PyObject *
PyMarshal_ReadObjectFromString(char *str, Py_ssize_t len)
{
	RFILE rf;
	PyObject *result;
	rf.fp = NULL;
	rf.ptr = str;
	rf.end = str + len;
	rf.strings = PyList_New(0);
	rf.depth = 0;
	result = r_object(&rf);
	Py_DECREF(rf.strings);
	return result;
}

PyObject *
PyMarshal_WriteObjectToString(PyObject *x, int version)
{
	WFILE wf;
	PyObject *res = NULL;

	wf.fp = NULL;
	wf.str = PyString_FromStringAndSize((char *)NULL, 50);
	if (wf.str == NULL)
		return NULL;
	wf.ptr = PyString_AS_STRING((PyStringObject *)wf.str);
	wf.end = wf.ptr + PyString_Size(wf.str);
	wf.error = 0;
	wf.depth = 0;
	wf.version = version;
	wf.strings = (version > 0) ? PyDict_New() : NULL;
	w_object(x, &wf);
	Py_XDECREF(wf.strings);
	if (wf.str != NULL) {
		char *base = PyString_AS_STRING((PyStringObject *)wf.str);
		if (wf.ptr - base > PY_SSIZE_T_MAX) {
			Py_DECREF(wf.str);
			PyErr_SetString(PyExc_OverflowError,
					"too much marshal data for a string");
			return NULL;
		}
		if (_PyString_Resize(&wf.str, (Py_ssize_t)(wf.ptr - base)) < 0)
			return NULL;
	}
	if (wf.error) {
		Py_XDECREF(wf.str);
		PyErr_SetString(PyExc_ValueError,
				(wf.error==1)?"unmarshallable object"
				:"object too deeply nested to marshal");
		return NULL;
	}
	if (wf.str != NULL) {
		/* XXX Quick hack -- need to do this differently */
		res = PyBytes_FromObject(wf.str);
		Py_DECREF(wf.str);
	}
	return res;
}

/* And an interface for Python programs... */

static PyObject *
marshal_dump(PyObject *self, PyObject *args)
{
	/* XXX Quick hack -- need to do this differently */
	PyObject *x;
	PyObject *f;
	int version = Py_MARSHAL_VERSION;
	PyObject *s;
	PyObject *res;
	if (!PyArg_ParseTuple(args, "OO|i:dump", &x, &f, &version))
		return NULL;
	s = PyMarshal_WriteObjectToString(x, version);
	if (s == NULL)
		return NULL;
	res = PyObject_CallMethod(f, "write", "O", s);
	Py_DECREF(s);
	return res;
}

static PyObject *
marshal_load(PyObject *self, PyObject *f)
{
	/* XXX Quick hack -- need to do this differently */
	PyObject *data, *result;
	RFILE rf;
	data = PyObject_CallMethod(f, "read", "");
	if (data == NULL)
		return NULL;
	rf.fp = NULL;
	if (PyString_Check(data)) {
		rf.ptr = PyString_AS_STRING(data);
		rf.end = rf.ptr + PyString_GET_SIZE(data);
	}
	else if (PyBytes_Check(data)) {
		rf.ptr = PyBytes_AS_STRING(data);
		rf.end = rf.ptr + PyBytes_GET_SIZE(data);
	}
	else {
		PyErr_Format(PyExc_TypeError,
			     "f.read() returned neither string "
			     "nor bytes but %.100s",
			     data->ob_type->tp_name);
		Py_DECREF(data);
		return NULL;
	}
	rf.strings = PyList_New(0);
	rf.depth = 0;
	result = read_object(&rf);
	Py_DECREF(rf.strings);
	Py_DECREF(data);
	return result;
}

static PyObject *
marshal_dumps(PyObject *self, PyObject *args)
{
	PyObject *x;
	int version = Py_MARSHAL_VERSION;
	if (!PyArg_ParseTuple(args, "O|i:dumps", &x, &version))
		return NULL;
	return PyMarshal_WriteObjectToString(x, version);
}

static PyObject *
marshal_loads(PyObject *self, PyObject *args)
{
	RFILE rf;
	char *s;
	Py_ssize_t n;
	PyObject* result;
	if (!PyArg_ParseTuple(args, "s#:loads", &s, &n))
		return NULL;
	rf.fp = NULL;
	rf.ptr = s;
	rf.end = s + n;
	rf.strings = PyList_New(0);
	rf.depth = 0;
	result = read_object(&rf);
	Py_DECREF(rf.strings);
	return result;
}

static PyMethodDef marshal_methods[] = {
	{"dump",	marshal_dump,	METH_VARARGS},
	{"load",	marshal_load,	METH_O},
	{"dumps",	marshal_dumps,	METH_VARARGS},
	{"loads",	marshal_loads,	METH_VARARGS},
	{NULL,		NULL}		/* sentinel */
};

PyMODINIT_FUNC
PyMarshal_Init(void)
{
	PyObject *mod = Py_InitModule("marshal", marshal_methods);
	if (mod == NULL)
		return;
	PyModule_AddIntConstant(mod, "version", Py_MARSHAL_VERSION);
}
