
/* New getargs implementation */

#include "Python.h"

#include <ctype.h>


int PyArg_Parse(PyObject *, char *, ...);
int PyArg_ParseTuple(PyObject *, char *, ...);
int PyArg_VaParse(PyObject *, char *, va_list);

int PyArg_ParseTupleAndKeywords(PyObject *, PyObject *,
				char *, char **, ...);
int PyArg_VaParseTupleAndKeywords(PyObject *, PyObject *,
				char *, char **, va_list);


/* Forward */
static int vgetargs1(PyObject *, char *, va_list *, int);
static void seterror(int, char *, int *, char *, char *);
static char *convertitem(PyObject *, char **, va_list *, int *, char *, 
			 size_t, PyObject **);
static char *converttuple(PyObject *, char **, va_list *,
			  int *, char *, size_t, int, PyObject **);
static char *convertsimple(PyObject *, char **, va_list *, char *,
			   size_t, PyObject **);
static int convertbuffer(PyObject *, void **p, char **);

static int vgetargskeywords(PyObject *, PyObject *,
			    char *, char **, va_list *);
static char *skipitem(char **, va_list *);

int
PyArg_Parse(PyObject *args, char *format, ...)
{
	int retval;
	va_list va;
	
	va_start(va, format);
	retval = vgetargs1(args, format, &va, 1);
	va_end(va);
	return retval;
}


int
PyArg_ParseTuple(PyObject *args, char *format, ...)
{
	int retval;
	va_list va;
	
	va_start(va, format);
	retval = vgetargs1(args, format, &va, 0);
	va_end(va);
	return retval;
}


int
PyArg_VaParse(PyObject *args, char *format, va_list va)
{
	va_list lva;

#ifdef VA_LIST_IS_ARRAY
	memcpy(lva, va, sizeof(va_list));
#else
#ifdef __va_copy
	__va_copy(lva, va);
#else
	lva = va;
#endif
#endif

	return vgetargs1(args, format, &lva, 0);
}


/* Handle cleanup of allocated memory in case of exception */

static int
addcleanup(void *ptr, PyObject **freelist)
{
	PyObject *cobj;
	if (!*freelist) {
		*freelist = PyList_New(0);
		if (!*freelist) {
			PyMem_FREE(ptr);
			return -1;
		}
	}
	cobj = PyCObject_FromVoidPtr(ptr, NULL);
	if (!cobj) {
		PyMem_FREE(ptr);
		return -1;
	}
	if(PyList_Append(*freelist, cobj)) {
                PyMem_FREE(ptr);
		Py_DECREF(cobj);
		return -1;
	}
        Py_DECREF(cobj);
	return 0;
}

static int
cleanreturn(int retval, PyObject *freelist)
{
	if(freelist) {
		if((retval) == 0) {
			int len = PyList_GET_SIZE(freelist), i;
			for (i = 0; i < len; i++)
                                PyMem_FREE(PyCObject_AsVoidPtr(
                                		PyList_GET_ITEM(freelist, i)));
		}
		Py_DECREF(freelist);
	}
	return retval;
}


static int
vgetargs1(PyObject *args, char *format, va_list *p_va, int compat)
{
	char msgbuf[256];
	int levels[32];
	char *fname = NULL;
	char *message = NULL;
	int min = -1;
	int max = 0;
	int level = 0;
	int endfmt = 0;
	char *formatsave = format;
	int i, len;
	char *msg;
	PyObject *freelist = NULL;
	
	assert(compat || (args != (PyObject*)NULL));

	while (endfmt == 0) {
		int c = *format++;
		switch (c) {
		case '(':
			if (level == 0)
				max++;
			level++;
			break;
		case ')':
			if (level == 0)
				Py_FatalError("excess ')' in getargs format");
			else
				level--;
			break;
		case '\0':
			endfmt = 1;
			break;
		case ':':
			fname = format;
			endfmt = 1;
			break;
		case ';':
			message = format;
			endfmt = 1;
			break;
		default:
			if (level == 0) {
				if (c == 'O')
					max++;
				else if (isalpha(c)) {
					if (c != 'e') /* skip encoded */
						max++;
				} else if (c == '|')
					min = max;
			}
			break;
		}
	}
	
	if (level != 0)
		Py_FatalError(/* '(' */ "missing ')' in getargs format");
	
	if (min < 0)
		min = max;
	
	format = formatsave;
	
	if (compat) {
		if (max == 0) {
			if (args == NULL)
				return 1;
			PyOS_snprintf(msgbuf, sizeof(msgbuf),
				      "%.200s%s takes no arguments",
				      fname==NULL ? "function" : fname,
				      fname==NULL ? "" : "()");
			PyErr_SetString(PyExc_TypeError, msgbuf);
			return 0;
		}
		else if (min == 1 && max == 1) {
			if (args == NULL) {
				PyOS_snprintf(msgbuf, sizeof(msgbuf),
				      "%.200s%s takes at least one argument",
					      fname==NULL ? "function" : fname,
					      fname==NULL ? "" : "()");
				PyErr_SetString(PyExc_TypeError, msgbuf);
				return 0;
			}
			msg = convertitem(args, &format, p_va, levels, msgbuf,
					  sizeof(msgbuf), &freelist);
			if (msg == NULL)
				return cleanreturn(1, freelist);
			seterror(levels[0], msg, levels+1, fname, message);
			return cleanreturn(0, freelist);
		}
		else {
			PyErr_SetString(PyExc_SystemError,
			    "old style getargs format uses new features");
			return 0;
		}
	}
	
	if (!PyTuple_Check(args)) {
		PyErr_SetString(PyExc_SystemError,
		    "new style getargs format but argument is not a tuple");
		return 0;
	}
	
	len = PyTuple_GET_SIZE(args);
	
	if (len < min || max < len) {
		if (message == NULL) {
			PyOS_snprintf(msgbuf, sizeof(msgbuf),
				      "%.150s%s takes %s %d argument%s "
				      "(%d given)",
				      fname==NULL ? "function" : fname,
				      fname==NULL ? "" : "()",
				      min==max ? "exactly"
				      : len < min ? "at least" : "at most",
				      len < min ? min : max,
				      (len < min ? min : max) == 1 ? "" : "s",
				      len);
			message = msgbuf;
		}
		PyErr_SetString(PyExc_TypeError, message);
		return 0;
	}
	
	for (i = 0; i < len; i++) {
		if (*format == '|')
			format++;
		msg = convertitem(PyTuple_GET_ITEM(args, i), &format, p_va,
				  levels, msgbuf, sizeof(msgbuf), &freelist);
		if (msg) {
			seterror(i+1, msg, levels, fname, message);
			return cleanreturn(0, freelist);
		}
	}

	if (*format != '\0' && !isalpha((int)(*format)) &&
	    *format != '(' &&
	    *format != '|' && *format != ':' && *format != ';') {
		PyErr_Format(PyExc_SystemError,
			     "bad format string: %.200s", formatsave);
		return cleanreturn(0, freelist);
	}
	
	return cleanreturn(1, freelist);
}



static void
seterror(int iarg, char *msg, int *levels, char *fname, char *message)
{
	char buf[512];
	int i;
	char *p = buf;

	if (PyErr_Occurred())
		return;
	else if (message == NULL) {
		if (fname != NULL) {
			PyOS_snprintf(p, sizeof(buf), "%.200s() ", fname);
			p += strlen(p);
		}
		if (iarg != 0) {
			PyOS_snprintf(p, sizeof(buf) - (p - buf),
				      "argument %d", iarg);
			i = 0;
			p += strlen(p);
			while (levels[i] > 0 && (int)(p-buf) < 220) {
				PyOS_snprintf(p, sizeof(buf) - (buf - p),
					      ", item %d", levels[i]-1);
				p += strlen(p);
				i++;
			}
		}
		else {
			PyOS_snprintf(p, sizeof(buf) - (p - buf), "argument");
			p += strlen(p);
		}
		PyOS_snprintf(p, sizeof(buf) - (p - buf), " %.256s", msg);
		message = buf;
	}
	PyErr_SetString(PyExc_TypeError, message);
}


/* Convert a tuple argument.
   On entry, *p_format points to the character _after_ the opening '('.
   On successful exit, *p_format points to the closing ')'.
   If successful:
      *p_format and *p_va are updated,
      *levels and *msgbuf are untouched,
      and NULL is returned.
   If the argument is invalid:
      *p_format is unchanged,
      *p_va is undefined,
      *levels is a 0-terminated list of item numbers,
      *msgbuf contains an error message, whose format is:
         "must be <typename1>, not <typename2>", where:
            <typename1> is the name of the expected type, and
            <typename2> is the name of the actual type,
      and msgbuf is returned.
*/

static char *
converttuple(PyObject *arg, char **p_format, va_list *p_va, int *levels,
	     char *msgbuf, size_t bufsize, int toplevel, PyObject **freelist)
{
	int level = 0;
	int n = 0;
	char *format = *p_format;
	int i;
	
	for (;;) {
		int c = *format++;
		if (c == '(') {
			if (level == 0)
				n++;
			level++;
		}
		else if (c == ')') {
			if (level == 0)
				break;
			level--;
		}
		else if (c == ':' || c == ';' || c == '\0')
			break;
		else if (level == 0 && isalpha(c))
			n++;
	}
	
	if (!PySequence_Check(arg) || PyString_Check(arg)) {
		levels[0] = 0;
		PyOS_snprintf(msgbuf, bufsize,
			      toplevel ? "expected %d arguments, not %.50s" :
			              "must be %d-item sequence, not %.50s",
			      n, 
			      arg == Py_None ? "None" : arg->ob_type->tp_name);
		return msgbuf;
	}
	
	if ((i = PySequence_Size(arg)) != n) {
		levels[0] = 0;
		PyOS_snprintf(msgbuf, bufsize,
			      toplevel ? "expected %d arguments, not %d" :
			             "must be sequence of length %d, not %d",
			      n, i);
		return msgbuf;
	}

	format = *p_format;
	for (i = 0; i < n; i++) {
		char *msg;
		PyObject *item;
		item = PySequence_GetItem(arg, i);
		msg = convertitem(item, &format, p_va, levels+1, msgbuf,
				  bufsize, freelist);
		/* PySequence_GetItem calls tp->sq_item, which INCREFs */
		Py_XDECREF(item);
		if (msg != NULL) {
			levels[0] = i+1;
			return msg;
		}
	}

	*p_format = format;
	return NULL;
}


/* Convert a single item. */

static char *
convertitem(PyObject *arg, char **p_format, va_list *p_va, int *levels,
	    char *msgbuf, size_t bufsize, PyObject **freelist)
{
	char *msg;
	char *format = *p_format;
	
	if (*format == '(' /* ')' */) {
		format++;
		msg = converttuple(arg, &format, p_va, levels, msgbuf, 
				   bufsize, 0, freelist);
		if (msg == NULL)
			format++;
	}
	else {
		msg = convertsimple(arg, &format, p_va, msgbuf, bufsize,
				    freelist);
		if (msg != NULL)
			levels[0] = 0;
	}
	if (msg == NULL)
		*p_format = format;
	return msg;
}



#define UNICODE_DEFAULT_ENCODING(arg) \
        _PyUnicode_AsDefaultEncodedString(arg, NULL)

/* Format an error message generated by convertsimple(). */

static char *
converterr(char *expected, PyObject *arg, char *msgbuf, size_t bufsize)
{
	assert(expected != NULL);
	assert(arg != NULL); 
	PyOS_snprintf(msgbuf, bufsize,
		      "must be %.50s, not %.50s", expected,
		      arg == Py_None ? "None" : arg->ob_type->tp_name);
	return msgbuf;
}

#define CONV_UNICODE "(unicode conversion error)"

/* explicitly check for float arguments when integers are expected.  For now
 * signal a warning.  Returns true if an exception was raised. */
static int
float_argument_error(PyObject *arg)
{
	if (PyFloat_Check(arg) &&
	    PyErr_Warn(PyExc_DeprecationWarning,
		       "integer argument expected, got float" ))
		return 1;
	else
		return 0;
}

/* Convert a non-tuple argument.  Return NULL if conversion went OK,
   or a string with a message describing the failure.  The message is
   formatted as "must be <desired type>, not <actual type>".
   When failing, an exception may or may not have been raised.
   Don't call if a tuple is expected. 
*/

static char *
convertsimple(PyObject *arg, char **p_format, va_list *p_va, char *msgbuf,
	      size_t bufsize, PyObject **freelist)
{
	char *format = *p_format;
	char c = *format++;
#ifdef Py_USING_UNICODE
	PyObject *uarg;
#endif
	
	switch (c) {
	
	case 'b': { /* unsigned byte -- very short int */
		char *p = va_arg(*p_va, char *);
		long ival;
		if (float_argument_error(arg))
			return converterr("integer<b>", arg, msgbuf, bufsize);
		ival = PyInt_AsLong(arg);
		if (ival == -1 && PyErr_Occurred())
			return converterr("integer<b>", arg, msgbuf, bufsize);
		else if (ival < 0) {
			PyErr_SetString(PyExc_OverflowError,
			"unsigned byte integer is less than minimum");
			return converterr("integer<b>", arg, msgbuf, bufsize);
		}
		else if (ival > UCHAR_MAX) {
			PyErr_SetString(PyExc_OverflowError,
			"unsigned byte integer is greater than maximum");
			return converterr("integer<b>", arg, msgbuf, bufsize);
		}
		else
			*p = (unsigned char) ival;
		break;
	}
	
	case 'B': {/* byte sized bitfield - both signed and unsigned
		      values allowed */  
		char *p = va_arg(*p_va, char *);
		long ival;
		if (float_argument_error(arg))
			return converterr("integer<B>", arg, msgbuf, bufsize);
		ival = PyInt_AsUnsignedLongMask(arg);
		if (ival == -1 && PyErr_Occurred())
			return converterr("integer<B>", arg, msgbuf, bufsize);
		else
			*p = (unsigned char) ival;
		break;
	}
	
	case 'h': {/* signed short int */
		short *p = va_arg(*p_va, short *);
		long ival;
		if (float_argument_error(arg))
			return converterr("integer<h>", arg, msgbuf, bufsize);
		ival = PyInt_AsLong(arg);
		if (ival == -1 && PyErr_Occurred())
			return converterr("integer<h>", arg, msgbuf, bufsize);
		else if (ival < SHRT_MIN) {
			PyErr_SetString(PyExc_OverflowError,
			"signed short integer is less than minimum");
			return converterr("integer<h>", arg, msgbuf, bufsize);
		}
		else if (ival > SHRT_MAX) {
			PyErr_SetString(PyExc_OverflowError,
			"signed short integer is greater than maximum");
			return converterr("integer<h>", arg, msgbuf, bufsize);
		}
		else
			*p = (short) ival;
		break;
	}
	
	case 'H': { /* short int sized bitfield, both signed and
		       unsigned allowed */ 
		unsigned short *p = va_arg(*p_va, unsigned short *);
		long ival;
		if (float_argument_error(arg))
			return converterr("integer<H>", arg, msgbuf, bufsize);
		ival = PyInt_AsUnsignedLongMask(arg);
		if (ival == -1 && PyErr_Occurred())
			return converterr("integer<H>", arg, msgbuf, bufsize);
		else
			*p = (unsigned short) ival;
		break;
	}
	
	case 'i': {/* signed int */
		int *p = va_arg(*p_va, int *);
		long ival;
		if (float_argument_error(arg))
			return converterr("integer<i>", arg, msgbuf, bufsize);
		ival = PyInt_AsLong(arg);
		if (ival == -1 && PyErr_Occurred())
			return converterr("integer<i>", arg, msgbuf, bufsize);
		else if (ival > INT_MAX) {
			PyErr_SetString(PyExc_OverflowError,
				"signed integer is greater than maximum");
			return converterr("integer<i>", arg, msgbuf, bufsize);
		}
		else if (ival < INT_MIN) {
			PyErr_SetString(PyExc_OverflowError,
				"signed integer is less than minimum");
			return converterr("integer<i>", arg, msgbuf, bufsize);
		}
		else
			*p = ival;
		break;
	}

	case 'I': { /* int sized bitfield, both signed and
		       unsigned allowed */ 
		unsigned int *p = va_arg(*p_va, unsigned int *);
		unsigned int ival;
		if (float_argument_error(arg))
			return converterr("integer<I>", arg, msgbuf, bufsize);
		ival = PyInt_AsUnsignedLongMask(arg);
		if (ival == -1 && PyErr_Occurred())
			return converterr("integer<I>", arg, msgbuf, bufsize);
		else
			*p = ival;
		break;
	}
	
	case 'l': {/* long int */
		long *p = va_arg(*p_va, long *);
		long ival;
		if (float_argument_error(arg))
			return converterr("integer<l>", arg, msgbuf, bufsize);
		ival = PyInt_AsLong(arg);
		if (ival == -1 && PyErr_Occurred())
			return converterr("integer<l>", arg, msgbuf, bufsize);
		else
			*p = ival;
		break;
	}

	case 'k': { /* long sized bitfield */
		unsigned long *p = va_arg(*p_va, unsigned long *);
		unsigned long ival;
		if (PyInt_Check(arg))
			ival = PyInt_AsUnsignedLongMask(arg);
		else if (PyLong_Check(arg))
			ival = PyLong_AsUnsignedLongMask(arg);
		else
			return converterr("integer<k>", arg, msgbuf, bufsize);
		*p = ival;
		break;
	}
	
#ifdef HAVE_LONG_LONG
	case 'L': {/* PY_LONG_LONG */
		PY_LONG_LONG *p = va_arg( *p_va, PY_LONG_LONG * );
		PY_LONG_LONG ival = PyLong_AsLongLong( arg );
		if( ival == (PY_LONG_LONG)-1 && PyErr_Occurred() ) {
			return converterr("long<L>", arg, msgbuf, bufsize);
		} else {
			*p = ival;
		}
		break;
	}

	case 'K': { /* long long sized bitfield */
		unsigned PY_LONG_LONG *p = va_arg(*p_va, unsigned PY_LONG_LONG *);
		unsigned PY_LONG_LONG ival;
		if (PyInt_Check(arg))
			ival = PyInt_AsUnsignedLongMask(arg);
		else if (PyLong_Check(arg))
			ival = PyLong_AsUnsignedLongLongMask(arg);
		else
			return converterr("integer<K>", arg, msgbuf, bufsize);
		*p = ival;
		break;
	}
#endif
	
	case 'f': {/* float */
		float *p = va_arg(*p_va, float *);
		double dval = PyFloat_AsDouble(arg);
		if (PyErr_Occurred())
			return converterr("float<f>", arg, msgbuf, bufsize);
		else
			*p = (float) dval;
		break;
	}
	
	case 'd': {/* double */
		double *p = va_arg(*p_va, double *);
		double dval = PyFloat_AsDouble(arg);
		if (PyErr_Occurred())
			return converterr("float<d>", arg, msgbuf, bufsize);
		else
			*p = dval;
		break;
	}
	
#ifndef WITHOUT_COMPLEX
	case 'D': {/* complex double */
		Py_complex *p = va_arg(*p_va, Py_complex *);
		Py_complex cval;
		cval = PyComplex_AsCComplex(arg);
		if (PyErr_Occurred())
			return converterr("complex<D>", arg, msgbuf, bufsize);
		else
			*p = cval;
		break;
	}
#endif /* WITHOUT_COMPLEX */
	
	case 'c': {/* char */
		char *p = va_arg(*p_va, char *);
		if (PyString_Check(arg) && PyString_Size(arg) == 1)
			*p = PyString_AS_STRING(arg)[0];
		else
			return converterr("char", arg, msgbuf, bufsize);
		break;
	}
	
	case 's': {/* string */
		if (*format == '#') {
			void **p = (void **)va_arg(*p_va, char **);
			int *q = va_arg(*p_va, int *);
			
			if (PyString_Check(arg)) {
				*p = PyString_AS_STRING(arg);
				*q = PyString_GET_SIZE(arg);
			}
#ifdef Py_USING_UNICODE
			else if (PyUnicode_Check(arg)) {
				uarg = UNICODE_DEFAULT_ENCODING(arg);
				if (uarg == NULL)
					return converterr(CONV_UNICODE,
							  arg, msgbuf, bufsize);
				*p = PyString_AS_STRING(uarg);
				*q = PyString_GET_SIZE(uarg);
			}
#endif
			else { /* any buffer-like object */
				char *buf;
				int count = convertbuffer(arg, p, &buf);
				if (count < 0)
					return converterr(buf, arg, msgbuf, bufsize);
				*q = count;
			}
			format++;
		} else {
			char **p = va_arg(*p_va, char **);
			
			if (PyString_Check(arg))
				*p = PyString_AS_STRING(arg);
#ifdef Py_USING_UNICODE
			else if (PyUnicode_Check(arg)) {
				uarg = UNICODE_DEFAULT_ENCODING(arg);
				if (uarg == NULL)
					return converterr(CONV_UNICODE,
							  arg, msgbuf, bufsize);
				*p = PyString_AS_STRING(uarg);
			}
#endif
			else
				return converterr("string", arg, msgbuf, bufsize);
			if ((int)strlen(*p) != PyString_Size(arg))
				return converterr("string without null bytes",
						  arg, msgbuf, bufsize);
		}
		break;
	}

	case 'z': {/* string, may be NULL (None) */
		if (*format == '#') { /* any buffer-like object */
			void **p = (void **)va_arg(*p_va, char **);
			int *q = va_arg(*p_va, int *);
			
			if (arg == Py_None) {
				*p = 0;
				*q = 0;
			}
			else if (PyString_Check(arg)) {
				*p = PyString_AS_STRING(arg);
				*q = PyString_GET_SIZE(arg);
			}
#ifdef Py_USING_UNICODE
			else if (PyUnicode_Check(arg)) {
				uarg = UNICODE_DEFAULT_ENCODING(arg);
				if (uarg == NULL)
					return converterr(CONV_UNICODE,
							  arg, msgbuf, bufsize);
				*p = PyString_AS_STRING(uarg);
				*q = PyString_GET_SIZE(uarg);
			}
#endif
			else { /* any buffer-like object */
				char *buf;
				int count = convertbuffer(arg, p, &buf);
				if (count < 0)
					return converterr(buf, arg, msgbuf, bufsize);
				*q = count;
			}
			format++;
		} else {
			char **p = va_arg(*p_va, char **);
			
			if (arg == Py_None)
				*p = 0;
			else if (PyString_Check(arg))
				*p = PyString_AS_STRING(arg);
#ifdef Py_USING_UNICODE
			else if (PyUnicode_Check(arg)) {
				uarg = UNICODE_DEFAULT_ENCODING(arg);
				if (uarg == NULL)
					return converterr(CONV_UNICODE,
							  arg, msgbuf, bufsize);
				*p = PyString_AS_STRING(uarg);
			}
#endif
			else
				return converterr("string or None", 
						  arg, msgbuf, bufsize);
			if (*format == '#') {
				int *q = va_arg(*p_va, int *);
				if (arg == Py_None)
					*q = 0;
				else
					*q = PyString_Size(arg);
				format++;
			}
			else if (*p != NULL &&
				 (int)strlen(*p) != PyString_Size(arg))
				return converterr(
					"string without null bytes or None", 
					arg, msgbuf, bufsize);
		}
		break;
	}
	
	case 'e': {/* encoded string */
		char **buffer;
		const char *encoding;
		PyObject *s;
		int size, recode_strings;

		/* Get 'e' parameter: the encoding name */
		encoding = (const char *)va_arg(*p_va, const char *);
#ifdef Py_USING_UNICODE
		if (encoding == NULL)
			encoding = PyUnicode_GetDefaultEncoding();
#endif
			
		/* Get output buffer parameter:
		   's' (recode all objects via Unicode) or
		   't' (only recode non-string objects) 
		*/
		if (*format == 's')
			recode_strings = 1;
		else if (*format == 't')
			recode_strings = 0;
		else
			return converterr(
				"(unknown parser marker combination)",
				arg, msgbuf, bufsize);
		buffer = (char **)va_arg(*p_va, char **);
		format++;
		if (buffer == NULL)
			return converterr("(buffer is NULL)", 
					  arg, msgbuf, bufsize);
			
		/* Encode object */
		if (!recode_strings && PyString_Check(arg)) {
			s = arg;
			Py_INCREF(s);
		}
		else {
#ifdef Py_USING_UNICODE
		    	PyObject *u;

			/* Convert object to Unicode */
			u = PyUnicode_FromObject(arg);
			if (u == NULL)
				return converterr(
					"string or unicode or text buffer", 
					arg, msgbuf, bufsize);
			
			/* Encode object; use default error handling */
			s = PyUnicode_AsEncodedString(u,
						      encoding,
						      NULL);
			Py_DECREF(u);
			if (s == NULL)
				return converterr("(encoding failed)",
						  arg, msgbuf, bufsize);
			if (!PyString_Check(s)) {
				Py_DECREF(s);
				return converterr(
					"(encoder failed to return a string)",
					arg, msgbuf, bufsize);
			}
#else
			return converterr("string<e>", arg, msgbuf, bufsize);
#endif
		}
		size = PyString_GET_SIZE(s);

		/* Write output; output is guaranteed to be 0-terminated */
		if (*format == '#') { 
			/* Using buffer length parameter '#':
				   
			   - if *buffer is NULL, a new buffer of the
			   needed size is allocated and the data
			   copied into it; *buffer is updated to point
			   to the new buffer; the caller is
			   responsible for PyMem_Free()ing it after
			   usage 

			   - if *buffer is not NULL, the data is
			   copied to *buffer; *buffer_len has to be
			   set to the size of the buffer on input;
			   buffer overflow is signalled with an error;
			   buffer has to provide enough room for the
			   encoded string plus the trailing 0-byte
			   
			   - in both cases, *buffer_len is updated to
			   the size of the buffer /excluding/ the
			   trailing 0-byte 
			   
			*/
			int *buffer_len = va_arg(*p_va, int *);

			format++;
			if (buffer_len == NULL) {
				Py_DECREF(s);
				return converterr(
					"(buffer_len is NULL)",
					arg, msgbuf, bufsize);
			}
			if (*buffer == NULL) {
				*buffer = PyMem_NEW(char, size + 1);
				if (*buffer == NULL) {
					Py_DECREF(s);
					return converterr(
						"(memory error)",
						arg, msgbuf, bufsize);
				}
				if(addcleanup(*buffer, freelist)) {
					Py_DECREF(s);
					return converterr(
						"(cleanup problem)",
						arg, msgbuf, bufsize);
				}
			} else {
				if (size + 1 > *buffer_len) {
					Py_DECREF(s);
					return converterr(
						"(buffer overflow)", 
						arg, msgbuf, bufsize);
				}
			}
			memcpy(*buffer,
			       PyString_AS_STRING(s),
			       size + 1);
			*buffer_len = size;
		} else {
			/* Using a 0-terminated buffer:
				   
			   - the encoded string has to be 0-terminated
			   for this variant to work; if it is not, an
			   error raised 

			   - a new buffer of the needed size is
			   allocated and the data copied into it;
			   *buffer is updated to point to the new
			   buffer; the caller is responsible for
			   PyMem_Free()ing it after usage

			*/
			if ((int)strlen(PyString_AS_STRING(s)) != size) {
				Py_DECREF(s);
				return converterr(
					"(encoded string without NULL bytes)",
					arg, msgbuf, bufsize);
			}
			*buffer = PyMem_NEW(char, size + 1);
			if (*buffer == NULL) {
				Py_DECREF(s);
				return converterr("(memory error)",
						  arg, msgbuf, bufsize);
			}
			if(addcleanup(*buffer, freelist)) {
				Py_DECREF(s);
				return converterr("(cleanup problem)",
						arg, msgbuf, bufsize);
			}
			memcpy(*buffer,
			       PyString_AS_STRING(s),
			       size + 1);
		}
		Py_DECREF(s);
		break;
	}

#ifdef Py_USING_UNICODE
	case 'u': {/* raw unicode buffer (Py_UNICODE *) */
		if (*format == '#') { /* any buffer-like object */
			void **p = (void **)va_arg(*p_va, char **);
			int *q = va_arg(*p_va, int *);
			if (PyUnicode_Check(arg)) {
			    	*p = PyUnicode_AS_UNICODE(arg);
				*q = PyUnicode_GET_SIZE(arg);
			}
			else {
			char *buf;
			int count = convertbuffer(arg, p, &buf);
			if (count < 0)
				return converterr(buf, arg, msgbuf, bufsize);
			*q = count/(sizeof(Py_UNICODE)); 
			}
			format++;
		} else {
			Py_UNICODE **p = va_arg(*p_va, Py_UNICODE **);
			if (PyUnicode_Check(arg))
				*p = PyUnicode_AS_UNICODE(arg);
			else
				return converterr("unicode", arg, msgbuf, bufsize);
		}
		break;
	}
#endif

	case 'S': { /* string object */
		PyObject **p = va_arg(*p_va, PyObject **);
		if (PyString_Check(arg))
			*p = arg;
		else
			return converterr("string", arg, msgbuf, bufsize);
		break;
	}
	
#ifdef Py_USING_UNICODE
	case 'U': { /* Unicode object */
		PyObject **p = va_arg(*p_va, PyObject **);
		if (PyUnicode_Check(arg))
			*p = arg;
		else
			return converterr("unicode", arg, msgbuf, bufsize);
		break;
	}
#endif
	
	case 'O': { /* object */
		PyTypeObject *type;
		PyObject **p;
		if (*format == '!') {
			type = va_arg(*p_va, PyTypeObject*);
			p = va_arg(*p_va, PyObject **);
			format++;
			if (PyType_IsSubtype(arg->ob_type, type))
				*p = arg;
			else
				return converterr(type->tp_name, arg, msgbuf, bufsize);

		}
		else if (*format == '?') {
			inquiry pred = va_arg(*p_va, inquiry);
			p = va_arg(*p_va, PyObject **);
			format++;
			if ((*pred)(arg)) 
				*p = arg;
			else
				return converterr("(unspecified)", 
						  arg, msgbuf, bufsize);
				
		}
		else if (*format == '&') {
			typedef int (*converter)(PyObject *, void *);
			converter convert = va_arg(*p_va, converter);
			void *addr = va_arg(*p_va, void *);
			format++;
			if (! (*convert)(arg, addr))
				return converterr("(unspecified)", 
						  arg, msgbuf, bufsize);
		}
		else {
			p = va_arg(*p_va, PyObject **);
			*p = arg;
		}
		break;
	}
		
		
	case 'w': { /* memory buffer, read-write access */
		void **p = va_arg(*p_va, void **);
		PyBufferProcs *pb = arg->ob_type->tp_as_buffer;
		int count;
			
		if (pb == NULL || 
		    pb->bf_getwritebuffer == NULL ||
		    pb->bf_getsegcount == NULL)
			return converterr("read-write buffer", arg, msgbuf, bufsize);
		if ((*pb->bf_getsegcount)(arg, NULL) != 1)
			return converterr("single-segment read-write buffer", 
					  arg, msgbuf, bufsize);
		if ((count = pb->bf_getwritebuffer(arg, 0, p)) < 0)
			return converterr("(unspecified)", arg, msgbuf, bufsize);
		if (*format == '#') {
			int *q = va_arg(*p_va, int *);
			
			*q = count;
			format++;
		}
		break;
	}
		
	case 't': { /* 8-bit character buffer, read-only access */
		const char **p = va_arg(*p_va, const char **);
		PyBufferProcs *pb = arg->ob_type->tp_as_buffer;
		int count;
		
		if (*format++ != '#')
			return converterr(
				"invalid use of 't' format character", 
				arg, msgbuf, bufsize);
		if (!PyType_HasFeature(arg->ob_type,
				       Py_TPFLAGS_HAVE_GETCHARBUFFER) ||
		    pb == NULL || pb->bf_getcharbuffer == NULL ||
		    pb->bf_getsegcount == NULL)
			return converterr(
				"string or read-only character buffer",
				arg, msgbuf, bufsize);

		if (pb->bf_getsegcount(arg, NULL) != 1)
			return converterr(
				"string or single-segment read-only buffer",
				arg, msgbuf, bufsize);

		count = pb->bf_getcharbuffer(arg, 0, p);
		if (count < 0)
			return converterr("(unspecified)", arg, msgbuf, bufsize);
		*va_arg(*p_va, int *) = count;
		break;
	}

	default:
		return converterr("impossible<bad format char>", arg, msgbuf, bufsize);
	
	}
	
	*p_format = format;
	return NULL;
}

static int
convertbuffer(PyObject *arg, void **p, char **errmsg)
{
	PyBufferProcs *pb = arg->ob_type->tp_as_buffer;
	int count;
	if (pb == NULL ||
	    pb->bf_getreadbuffer == NULL ||
	    pb->bf_getsegcount == NULL) {
		*errmsg = "string or read-only buffer";
		return -1;
	}
	if ((*pb->bf_getsegcount)(arg, NULL) != 1) {
		*errmsg = "string or single-segment read-only buffer";
		return -1;
	}
	if ((count = (*pb->bf_getreadbuffer)(arg, 0, p)) < 0) {
		*errmsg = "(unspecified)";
	}
	return count;
}

/* Support for keyword arguments donated by
   Geoff Philbrick <philbric@delphi.hks.com> */

/* Return false (0) for error, else true. */
int
PyArg_ParseTupleAndKeywords(PyObject *args,
			    PyObject *keywords,
			    char *format, 
			    char **kwlist, ...)
{
	int retval;
	va_list va;

	if ((args == NULL || !PyTuple_Check(args)) ||
	    (keywords != NULL && !PyDict_Check(keywords)) ||
	    format == NULL ||
	    kwlist == NULL)
	{
		PyErr_BadInternalCall();
		return 0;
	}

	va_start(va, kwlist);
	retval = vgetargskeywords(args, keywords, format, kwlist, &va);	
	va_end(va);
	return retval;
}


int
PyArg_VaParseTupleAndKeywords(PyObject *args,
			    PyObject *keywords,
			    char *format, 
			    char **kwlist, va_list va)
{
	int retval;
	va_list lva;

	if ((args == NULL || !PyTuple_Check(args)) ||
	    (keywords != NULL && !PyDict_Check(keywords)) ||
	    format == NULL ||
	    kwlist == NULL)
	{
		PyErr_BadInternalCall();
		return 0;
	}

#ifdef VA_LIST_IS_ARRAY
	memcpy(lva, va, sizeof(va_list));
#else
#ifdef __va_copy
	__va_copy(lva, va);
#else
	lva = va;
#endif
#endif

	retval = vgetargskeywords(args, keywords, format, kwlist, &lva);	
	return retval;
}


static int
vgetargskeywords(PyObject *args, PyObject *keywords, char *format,
	         char **kwlist, va_list *p_va)
{
	char msgbuf[512];
	int levels[32];
	char *fname, *message;
	int min, max;
	char *formatsave;
	int i, len, nargs, nkeywords;
	char *msg, **p;
	PyObject *freelist = NULL;

	assert(args != NULL && PyTuple_Check(args));
	assert(keywords == NULL || PyDict_Check(keywords));
	assert(format != NULL);
	assert(kwlist != NULL);
	assert(p_va != NULL);

	/* Search the format:
	   message <- error msg, if any (else NULL).
	   fname <- routine name, if any (else NULL).
	   min <- # of required arguments, or -1 if all are required.
	   max <- most arguments (required + optional).
	   Check that kwlist has a non-NULL entry for each arg.
	   Raise error if a tuple arg spec is found.
	*/
	fname = message = NULL;
	formatsave = format;
	p = kwlist;
	min = -1;
	max = 0;
	while ((i = *format++) != '\0') {
		if (isalpha(i) && i != 'e') {
			max++;
			if (*p == NULL) {
				PyErr_SetString(PyExc_RuntimeError,
					"more argument specifiers than "
					"keyword list entries");
				return 0;
			}
			p++;
		}
		else if (i == '|')
			min = max;
		else if (i == ':') {
			fname = format;
			break;
		}
		else if (i == ';') {
			message = format;
			break;
		}
		else if (i == '(') {
			PyErr_SetString(PyExc_RuntimeError,
				"tuple found in format when using keyword "
				"arguments");
			return 0;
		}
	}
	format = formatsave;
	if (*p != NULL) {
		PyErr_SetString(PyExc_RuntimeError,
			"more keyword list entries than "
			"argument specifiers");
		return 0;
	}
	if (min < 0) {
		/* All arguments are required. */
		min = max;
	}

	nargs = PyTuple_GET_SIZE(args);
	nkeywords = keywords == NULL ? 0 : PyDict_Size(keywords);

	/* make sure there are no duplicate values for an argument;
	   its not clear when to use the term "keyword argument vs. 
	   keyword parameter in messages */
	if (nkeywords > 0) {
		for (i = 0; i < nargs; i++) {
			char *thiskw = kwlist[i];
			if (thiskw == NULL)
				break;
			if (PyDict_GetItemString(keywords, thiskw)) {
				PyErr_Format(PyExc_TypeError,
					"keyword parameter '%s' was given "
					"by position and by name",
					thiskw);
				return 0;
			}
			else if (PyErr_Occurred())
				return 0;
		}
	}

	/* required arguments missing from args can be supplied by keyword 
	   arguments; set len to the number of posiitional arguments, and,
	   if that's less than the minimum required, add in the number of
	   required arguments that are supplied by keywords */
	len = nargs;
	if (nkeywords > 0 && nargs < min) {
		for (i = nargs; i < min; i++) {
			if (PyDict_GetItemString(keywords, kwlist[i]))
				len++;
			else if (PyErr_Occurred())
				return 0;
		}
	}

	/* make sure we got an acceptable number of arguments; the message
	   is a little confusing with keywords since keyword arguments
	   which are supplied, but don't match the required arguments
	   are not included in the "%d given" part of the message */
	if (len < min || max < len) {
		if (message == NULL) {
			PyOS_snprintf(msgbuf, sizeof(msgbuf),
				      "%.200s%s takes %s %d argument%s "
				      "(%d given)",
				      fname==NULL ? "function" : fname,
				      fname==NULL ? "" : "()",
				      min==max ? "exactly"
			              : len < min ? "at least" : "at most",
				      len < min ? min : max,
				      (len < min ? min : max) == 1 ? "" : "s",
				      len);
			message = msgbuf;
		}
		PyErr_SetString(PyExc_TypeError, message);
		return 0;
	}

	/* convert the positional arguments */
	for (i = 0; i < nargs; i++) {
		if (*format == '|')
			format++;
		msg = convertitem(PyTuple_GET_ITEM(args, i), &format, p_va,
				  levels, msgbuf, sizeof(msgbuf), &freelist);
		if (msg) {
			seterror(i+1, msg, levels, fname, message);
			return cleanreturn(0, freelist);
		}
	}

	/* handle no keyword parameters in call */	
	if (nkeywords == 0)
		return cleanreturn(1, freelist);

	/* convert the keyword arguments; this uses the format 
	   string where it was left after processing args */
	for (i = nargs; i < max; i++) {
		PyObject *item;
		if (*format == '|')
			format++;
		item = PyDict_GetItemString(keywords, kwlist[i]);
		if (item != NULL) {
			Py_INCREF(item);
			msg = convertitem(item, &format, p_va, levels, msgbuf,
					  sizeof(msgbuf), &freelist);
			Py_DECREF(item);
			if (msg) {
				seterror(i+1, msg, levels, fname, message);
				return cleanreturn(0, freelist);
			}
			--nkeywords;
			if (nkeywords == 0)
				break;
		}
		else if (PyErr_Occurred())
			return cleanreturn(0, freelist);
		else {
			msg = skipitem(&format, p_va);
			if (msg) {
				seterror(i+1, msg, levels, fname, message);
				return cleanreturn(0, freelist);
			}
		}
	}

	/* make sure there are no extraneous keyword arguments */
	if (nkeywords > 0) {
		PyObject *key, *value;
		int pos = 0;
		while (PyDict_Next(keywords, &pos, &key, &value)) {
			int match = 0;
			char *ks;
			if (!PyString_Check(key)) {
				PyErr_SetString(PyExc_TypeError, 
					        "keywords must be strings");
				return cleanreturn(0, freelist);
			}
			ks = PyString_AsString(key);
			for (i = 0; i < max; i++) {
				if (!strcmp(ks, kwlist[i])) {
					match = 1;
					break;
				}
			}
			if (!match) {
				PyErr_Format(PyExc_TypeError,
					     "'%s' is an invalid keyword "
					     "argument for this function",
					     ks);
				return cleanreturn(0, freelist);
			}
		}
	}

	return cleanreturn(1, freelist);
}


static char *
skipitem(char **p_format, va_list *p_va)
{
	char *format = *p_format;
	char c = *format++;
	
	switch (c) {
	
	case 'b': /* byte -- very short int */
	case 'B': /* byte as bitfield */
		{
			(void) va_arg(*p_va, char *);
			break;
		}
	
	case 'h': /* short int */
		{
			(void) va_arg(*p_va, short *);
			break;
		}
	
	case 'H': /* short int as bitfield */
		{
			(void) va_arg(*p_va, unsigned short *);
			break;
		}
	
	case 'i': /* int */
		{
			(void) va_arg(*p_va, int *);
			break;
		}
	
	case 'l': /* long int */
		{
			(void) va_arg(*p_va, long *);
			break;
		}
	
#ifdef HAVE_LONG_LONG
	case 'L': /* PY_LONG_LONG int */
		{
			(void) va_arg(*p_va, PY_LONG_LONG *);
			break;
		}
#endif
	
	case 'f': /* float */
		{
			(void) va_arg(*p_va, float *);
			break;
		}
	
	case 'd': /* double */
		{
			(void) va_arg(*p_va, double *);
			break;
		}
	
#ifndef WITHOUT_COMPLEX
	case 'D': /* complex double */
		{
			(void) va_arg(*p_va, Py_complex *);
			break;
		}
#endif /* WITHOUT_COMPLEX */
	
	case 'c': /* char */
		{
			(void) va_arg(*p_va, char *);
			break;
		}
	
	case 's': /* string */
		{
			(void) va_arg(*p_va, char **);
			if (*format == '#') {
				(void) va_arg(*p_va, int *);
				format++;
			}
			break;
		}
	
	case 'z': /* string */
		{
			(void) va_arg(*p_va, char **);
			if (*format == '#') {
				(void) va_arg(*p_va, int *);
				format++;
			}
			break;
		}
	
	case 'S': /* string object */
		{
			(void) va_arg(*p_va, PyObject **);
			break;
		}
	
	case 'O': /* object */
		{
			if (*format == '!') {
				format++;
				(void) va_arg(*p_va, PyTypeObject*);
				(void) va_arg(*p_va, PyObject **);
			}
#if 0
/* I don't know what this is for */
			else if (*format == '?') {
				inquiry pred = va_arg(*p_va, inquiry);
				format++;
				if ((*pred)(arg)) {
					(void) va_arg(*p_va, PyObject **);
				}
			}
#endif
			else if (*format == '&') {
				typedef int (*converter)(PyObject *, void *);
				(void) va_arg(*p_va, converter);
				(void) va_arg(*p_va, void *);
				format++;
			}
			else {
				(void) va_arg(*p_va, PyObject **);
			}
			break;
		}
	
	default:
		return "impossible<bad format char>";
	
	}
	
	*p_format = format;
	return NULL;
}


int
PyArg_UnpackTuple(PyObject *args, char *name, int min, int max, ...)
{
	int i, l;
	PyObject **o;
	va_list vargs;

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(vargs, max);
#else
	va_start(vargs);
#endif

	assert(min >= 0);
	assert(min <= max);
	if (!PyTuple_Check(args)) {
		PyErr_SetString(PyExc_SystemError,
		    "PyArg_UnpackTuple() argument list is not a tuple");
		return 0;
	}	
	l = PyTuple_GET_SIZE(args);
	if (l < min) {
		if (name != NULL)
			PyErr_Format(
			    PyExc_TypeError,
			    "%s expected %s%d arguments, got %d", 
			    name, (min == max ? "" : "at least "), min, l);
		else
			PyErr_Format(
			    PyExc_TypeError,
			    "unpacked tuple should have %s%d elements,"
			    " but has %d", 
			    (min == max ? "" : "at least "), min, l);
		va_end(vargs);
		return 0;
	}
	if (l > max) {
		if (name != NULL)
			PyErr_Format(
			    PyExc_TypeError,
			    "%s expected %s%d arguments, got %d", 
			    name, (min == max ? "" : "at most "), max, l);
		else
			PyErr_Format(
			    PyExc_TypeError,
			    "unpacked tuple should have %s%d elements,"
			    " but has %d", 
			    (min == max ? "" : "at most "), max, l);
		va_end(vargs);
		return 0;
	}
	for (i = 0; i < l; i++) {
		o = va_arg(vargs, PyObject **);
		*o = PyTuple_GET_ITEM(args, i);
	}
	va_end(vargs);
	return 1;
}
