
/* New getargs implementation */

/* XXX There are several unchecked sprintf or strcat calls in this file.
   XXX The only way these can become a danger is if some C code in the
   XXX Python source (or in an extension) uses ridiculously long names
   XXX or ridiculously deep nesting in format strings. */

#include "Python.h"

#include <ctype.h>


int PyArg_Parse(PyObject *, char *, ...);
int PyArg_ParseTuple(PyObject *, char *, ...);
int PyArg_VaParse(PyObject *, char *, va_list);

int PyArg_ParseTupleAndKeywords(PyObject *, PyObject *,
				char *, char **, ...);

/* Forward */
static int vgetargs1(PyObject *, char *, va_list *, int);
static void seterror(int, char *, int *, char *, char *);
static char *convertitem(PyObject *, char **, va_list *, int *, char *);
static char *converttuple(PyObject *, char **, va_list *,
			  int *, char *, int);
static char *convertsimple(PyObject *, char **, va_list *, char *);
static char *convertsimple1(PyObject *, char **, va_list *);

static int vgetargskeywords(PyObject *, PyObject *,
			    char *, char **, va_list *);
static char *skipitem(char **, va_list *);

int PyArg_Parse(PyObject *args, char *format, ...)
{
	int retval;
	va_list va;
	
	va_start(va, format);
	retval = vgetargs1(args, format, &va, 1);
	va_end(va);
	return retval;
}


int PyArg_ParseTuple(PyObject *args, char *format, ...)
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
	lva = va;
#endif

	return vgetargs1(args, format, &lva, 0);
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
	char *formatsave = format;
	int i, len;
	char *msg;
	
	assert(compat || (args != (PyObject*)NULL));

	for (;;) {
		int c = *format++;
		if (c == '(' /* ')' */) {
			if (level == 0)
				max++;
			level++;
		}
		else if (/* '(' */ c == ')') {
			if (level == 0)
				Py_FatalError(/* '(' */
				      "excess ')' in getargs format");
			else
				level--;
		}
		else if (c == '\0')
			break;
		else if (c == ':') {
			fname = format;
			break;
		}
		else if (c == ';') {
			message = format;
			break;
		}
		else if (level != 0)
			; /* Pass */
		else if (c == 'e')
			; /* Pass */
		else if (isalpha(c))
			max++;
		else if (c == '|')
			min = max;
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
			sprintf(msgbuf, "%s%s takes no arguments",
				fname==NULL ? "function" : fname,
				fname==NULL ? "" : "()");
			PyErr_SetString(PyExc_TypeError, msgbuf);
			return 0;
		}
		else if (min == 1 && max == 1) {
			if (args == NULL) {
				sprintf(msgbuf,
					"%s%s takes at least one argument",
					fname==NULL ? "function" : fname,
					fname==NULL ? "" : "()");
				PyErr_SetString(PyExc_TypeError, msgbuf);
				return 0;
			}
			msg = convertitem(args, &format, p_va, levels, msgbuf);
			if (msg == NULL)
				return 1;
			seterror(levels[0], msg, levels+1, fname, message);
			return 0;
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
			sprintf(msgbuf,
				"%s%s takes %s %d argument%s (%d given)",
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
				  levels, msgbuf);
		if (msg) {
			seterror(i+1, msg, levels, fname, message);
			return 0;
		}
	}

	if (*format != '\0' && !isalpha((int)(*format)) &&
	    *format != '(' &&
	    *format != '|' && *format != ':' && *format != ';') {
		PyErr_Format(PyExc_SystemError,
			     "bad format string: %.200s", formatsave);
		return 0;
	}
	
	return 1;
}



static void
seterror(int iarg, char *msg, int *levels, char *fname, char *message)
{
	char buf[256];
	int i;
	char *p = buf;

	if (PyErr_Occurred())
		return;
	else if (message == NULL) {
		if (fname != NULL) {
			sprintf(p, "%s() ", fname);
			p += strlen(p);
		}
		if (iarg != 0) {
			sprintf(p, "argument %d", iarg);
			i = 0;
			p += strlen(p);
			while (levels[i] > 0) {
				sprintf(p, ", item %d", levels[i]-1);
				p += strlen(p);
				i++;
			}
		}
		else {
			sprintf(p, "argument");
			p += strlen(p);
		}
		sprintf(p, " %s", msg);
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
	     char *msgbuf, int toplevel)
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
		sprintf(msgbuf,
			toplevel ? "expected %d arguments, not %s" :
				   "must be %d-item sequence, not %s",
			n, arg == Py_None ? "None" : arg->ob_type->tp_name);
		return msgbuf;
	}
	
	if ((i = PySequence_Size(arg)) != n) {
		levels[0] = 0;
		sprintf(msgbuf,
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
		msg = convertitem(item, &format, p_va, levels+1, msgbuf);
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
	    char *msgbuf)
{
	char *msg;
	char *format = *p_format;
	
	if (*format == '(' /* ')' */) {
		format++;
		msg = converttuple(arg, &format, p_va, levels, msgbuf, 0);
		if (msg == NULL)
			format++;
	}
	else {
		msg = convertsimple(arg, &format, p_va, msgbuf);
		if (msg != NULL)
			levels[0] = 0;
	}
	if (msg == NULL)
		*p_format = format;
	return msg;
}


/* Convert a non-tuple argument.  Adds to convertsimple1 functionality
   by formatting messages as "must be <desired type>, not <actual type>". */

static char *
convertsimple(PyObject *arg, char **p_format, va_list *p_va, char *msgbuf)
{
	char *msg = convertsimple1(arg, p_format, p_va);
	if (msg != NULL) {
		sprintf(msgbuf, "must be %.50s, not %.50s", msg,
			arg == Py_None ? "None" : arg->ob_type->tp_name);
		msg = msgbuf;
	}
	return msg;
}


/* Internal API needed by convertsimple1(): */
extern 
PyObject *_PyUnicode_AsDefaultEncodedString(PyObject *unicode,
				  const char *errors);

/* Convert a non-tuple argument.  Return NULL if conversion went OK,
   or a string representing the expected type if the conversion failed.
   When failing, an exception may or may not have been raised.
   Don't call if a tuple is expected. */

static char *
convertsimple1(PyObject *arg, char **p_format, va_list *p_va)
{
	char *format = *p_format;
	char c = *format++;
	
	switch (c) {
	
	case 'b': /* unsigned byte -- very short int */
		{
			char *p = va_arg(*p_va, char *);
			long ival = PyInt_AsLong(arg);
			if (ival == -1 && PyErr_Occurred())
				return "integer<b>";
			else if (ival < 0) {
				PyErr_SetString(PyExc_OverflowError,
			      "unsigned byte integer is less than minimum");
				return "integer<b>";
			}
			else if (ival > UCHAR_MAX) {
				PyErr_SetString(PyExc_OverflowError,
			      "unsigned byte integer is greater than maximum");
				return "integer<b>";
			}
			else
				*p = (unsigned char) ival;
			break;
		}
	
	case 'B': /* byte sized bitfield - both signed and unsigned values allowed */
		{
			char *p = va_arg(*p_va, char *);
			long ival = PyInt_AsLong(arg);
			if (ival == -1 && PyErr_Occurred())
				return "integer<b>";
			else if (ival < SCHAR_MIN) {
				PyErr_SetString(PyExc_OverflowError,
			      "byte-sized integer bitfield is less than minimum");
				return "integer<B>";
			}
			else if (ival > (int)UCHAR_MAX) {
				PyErr_SetString(PyExc_OverflowError,
			      "byte-sized integer bitfield is greater than maximum");
				return "integer<B>";
			}
			else
				*p = (unsigned char) ival;
			break;
		}
	
	case 'h': /* signed short int */
		{
			short *p = va_arg(*p_va, short *);
			long ival = PyInt_AsLong(arg);
			if (ival == -1 && PyErr_Occurred())
				return "integer<h>";
			else if (ival < SHRT_MIN) {
				PyErr_SetString(PyExc_OverflowError,
			      "signed short integer is less than minimum");
				return "integer<h>";
			}
			else if (ival > SHRT_MAX) {
				PyErr_SetString(PyExc_OverflowError,
			      "signed short integer is greater than maximum");
				return "integer<h>";
			}
			else
				*p = (short) ival;
			break;
		}
	
	case 'H': /* short int sized bitfield, both signed and unsigned allowed */
		{
			unsigned short *p = va_arg(*p_va, unsigned short *);
			long ival = PyInt_AsLong(arg);
			if (ival == -1 && PyErr_Occurred())
				return "integer<H>";
			else if (ival < SHRT_MIN) {
				PyErr_SetString(PyExc_OverflowError,
			      "short integer bitfield is less than minimum");
				return "integer<H>";
			}
			else if (ival > USHRT_MAX) {
				PyErr_SetString(PyExc_OverflowError,
			      "short integer bitfield is greater than maximum");
				return "integer<H>";
			}
			else
				*p = (unsigned short) ival;
			break;
		}
	
	case 'i': /* signed int */
		{
			int *p = va_arg(*p_va, int *);
			long ival = PyInt_AsLong(arg);
			if (ival == -1 && PyErr_Occurred())
				return "integer<i>";
			else if (ival > INT_MAX) {
				PyErr_SetString(PyExc_OverflowError,
				    "signed integer is greater than maximum");
				return "integer<i>";
			}
			else if (ival < INT_MIN) {
				PyErr_SetString(PyExc_OverflowError,
				    "signed integer is less than minimum");
				return "integer<i>";
			}
			else
				*p = ival;
			break;
		}
	case 'l': /* long int */
		{
			long *p = va_arg(*p_va, long *);
			long ival = PyInt_AsLong(arg);
			if (ival == -1 && PyErr_Occurred())
				return "integer<l>";
			else
				*p = ival;
			break;
		}
	
#ifdef HAVE_LONG_LONG
	case 'L': /* LONG_LONG */
		{
			LONG_LONG *p = va_arg( *p_va, LONG_LONG * );
			LONG_LONG ival = PyLong_AsLongLong( arg );
			if( ival == (LONG_LONG)-1 && PyErr_Occurred() ) {
				return "long<L>";
			} else {
				*p = ival;
			}
			break;
		}
#endif
	
	case 'f': /* float */
		{
			float *p = va_arg(*p_va, float *);
			double dval = PyFloat_AsDouble(arg);
			if (PyErr_Occurred())
				return "float<f>";
			else
				*p = (float) dval;
			break;
		}
	
	case 'd': /* double */
		{
			double *p = va_arg(*p_va, double *);
			double dval = PyFloat_AsDouble(arg);
			if (PyErr_Occurred())
				return "float<d>";
			else
				*p = dval;
			break;
		}
	
#ifndef WITHOUT_COMPLEX
	case 'D': /* complex double */
		{
			Py_complex *p = va_arg(*p_va, Py_complex *);
			Py_complex cval;
			cval = PyComplex_AsCComplex(arg);
			if (PyErr_Occurred())
				return "complex<D>";
			else
				*p = cval;
			break;
		}
#endif /* WITHOUT_COMPLEX */
	
	case 'c': /* char */
		{
			char *p = va_arg(*p_va, char *);
			if (PyString_Check(arg) && PyString_Size(arg) == 1)
				*p = PyString_AsString(arg)[0];
			else
				return "char";
			break;
		}
	
	case 's': /* string */
		{
			if (*format == '#') {
				void **p = (void **)va_arg(*p_va, char **);
				int *q = va_arg(*p_va, int *);

				if (PyString_Check(arg)) {
				    *p = PyString_AS_STRING(arg);
				    *q = PyString_GET_SIZE(arg);
				}
				else if (PyUnicode_Check(arg)) {
				    arg = _PyUnicode_AsDefaultEncodedString(
							            arg, NULL);
				    if (arg == NULL)
					return "(unicode conversion error)";
				    *p = PyString_AS_STRING(arg);
				    *q = PyString_GET_SIZE(arg);
				}
				else { /* any buffer-like object */
				    PyBufferProcs *pb = arg->ob_type->tp_as_buffer;
				    int count;
				    if ( pb == NULL ||
					 pb->bf_getreadbuffer == NULL ||
					 pb->bf_getsegcount == NULL )
					return "string or read-only buffer";
				    if ( (*pb->bf_getsegcount)(arg, NULL) != 1 )
					return "string or single-segment read-only buffer";
				    if ( (count =
					  (*pb->bf_getreadbuffer)(arg, 0, p)) < 0 )
					return "(unspecified)";
				    *q = count;
				}
				format++;
			} else {
				char **p = va_arg(*p_va, char **);
			
				if (PyString_Check(arg))
				    *p = PyString_AS_STRING(arg);
				else if (PyUnicode_Check(arg)) {
				    arg = _PyUnicode_AsDefaultEncodedString(
							            arg, NULL);
				    if (arg == NULL)
					return "(unicode conversion error)";
				    *p = PyString_AS_STRING(arg);
				}
				else
				  return "string";
				if ((int)strlen(*p) != PyString_Size(arg))
				  return "string without null bytes";
			}
			break;
		}

	case 'z': /* string, may be NULL (None) */
		{
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
				else if (PyUnicode_Check(arg)) {
				    arg = _PyUnicode_AsDefaultEncodedString(
							            arg, NULL);
				    if (arg == NULL)
					return "(unicode conversion error)";
				    *p = PyString_AS_STRING(arg);
				    *q = PyString_GET_SIZE(arg);
				}
				else { /* any buffer-like object */
				    PyBufferProcs *pb = arg->ob_type->tp_as_buffer;
				    int count;
				    if ( pb == NULL ||
					 pb->bf_getreadbuffer == NULL ||
					 pb->bf_getsegcount == NULL )
					return "string or read-only buffer";
				    if ( (*pb->bf_getsegcount)(arg, NULL) != 1 )
					return "string or single-segment read-only buffer";
				    if ( (count =
					  (*pb->bf_getreadbuffer)(arg, 0, p)) < 0 )
					return "(unspecified)";
				    *q = count;
				}
				format++;
			} else {
				char **p = va_arg(*p_va, char **);
			
				if (arg == Py_None)
				  *p = 0;
				else if (PyString_Check(arg))
				  *p = PyString_AsString(arg);
				else if (PyUnicode_Check(arg)) {
				  arg = _PyUnicode_AsDefaultEncodedString(
								  arg, NULL);
				  if (arg == NULL)
				      return "(unicode conversion error)";
				  *p = PyString_AS_STRING(arg);
				}
				else
				  return "string or None";
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
				  return "string without null bytes or None";
			}
			break;
		}
	
	case 'e': /* encoded string */
		{
			char **buffer;
			const char *encoding;
			PyObject *u, *s;
			int size;

			/* Get 'e' parameter: the encoding name */
			encoding = (const char *)va_arg(*p_va, const char *);
			if (encoding == NULL)
			    	encoding = PyUnicode_GetDefaultEncoding();
			
			/* Get 's' parameter: the output buffer to use */
			if (*format != 's')
				return "(unknown parser marker combination)";
			buffer = (char **)va_arg(*p_va, char **);
			format++;
			if (buffer == NULL)
				return "(buffer is NULL)";
			
			/* Convert object to Unicode */
			u = PyUnicode_FromObject(arg);
			if (u == NULL)
				return "string or unicode or text buffer";
			
			/* Encode object; use default error handling */
			s = PyUnicode_AsEncodedString(u,
						      encoding,
						      NULL);
			Py_DECREF(u);
			if (s == NULL)
				return "(encoding failed)";
			if (!PyString_Check(s)) {
				Py_DECREF(s);
				return "(encoder failed to return a string)";
			}
			size = PyString_GET_SIZE(s);

			/* Write output; output is guaranteed to be
			   0-terminated */
			if (*format == '#') { 
				/* Using buffer length parameter '#':

				   - if *buffer is NULL, a new buffer
				   of the needed size is allocated and
				   the data copied into it; *buffer is
				   updated to point to the new buffer;
				   the caller is responsible for
				   PyMem_Free()ing it after usage

				   - if *buffer is not NULL, the data
				   is copied to *buffer; *buffer_len
				   has to be set to the size of the
				   buffer on input; buffer overflow is
				   signalled with an error; buffer has
				   to provide enough room for the
				   encoded string plus the trailing
				   0-byte

				   - in both cases, *buffer_len is
				   updated to the size of the buffer
				   /excluding/ the trailing 0-byte

				*/
				int *buffer_len = va_arg(*p_va, int *);

				format++;
				if (buffer_len == NULL)
					return "(buffer_len is NULL)";
				if (*buffer == NULL) {
					*buffer = PyMem_NEW(char, size + 1);
					if (*buffer == NULL) {
						Py_DECREF(s);
						return "(memory error)";
					}
				} else {
					if (size + 1 > *buffer_len) {
						Py_DECREF(s);
						return "(buffer overflow)";
					}
				}
				memcpy(*buffer,
				       PyString_AS_STRING(s),
				       size + 1);
				*buffer_len = size;
			} else {
				/* Using a 0-terminated buffer:

				   - the encoded string has to be
				   0-terminated for this variant to
				   work; if it is not, an error raised

				   - a new buffer of the needed size
				   is allocated and the data copied
				   into it; *buffer is updated to
				   point to the new buffer; the caller
				   is responsible for PyMem_Free()ing it
				   after usage

				 */
				if ((int)strlen(PyString_AS_STRING(s)) != size)
					return "(encoded string without NULL bytes)";
				*buffer = PyMem_NEW(char, size + 1);
				if (*buffer == NULL) {
					Py_DECREF(s);
					return "(memory error)";
				}
				memcpy(*buffer,
				       PyString_AS_STRING(s),
				       size + 1);
			}
			Py_DECREF(s);
			break;
		}

	case 'u': /* raw unicode buffer (Py_UNICODE *) */
		{
			if (*format == '#') { /* any buffer-like object */
				void **p = (void **)va_arg(*p_va, char **);
				PyBufferProcs *pb = arg->ob_type->tp_as_buffer;
				int *q = va_arg(*p_va, int *);
				int count;

				if ( pb == NULL ||
				     pb->bf_getreadbuffer == NULL ||
				     pb->bf_getsegcount == NULL )
				  return "unicode or read-only buffer";
				if ( (*pb->bf_getsegcount)(arg, NULL) != 1 )
				  return "unicode or single-segment read-only buffer";
				if ( (count =
				      (*pb->bf_getreadbuffer)(arg, 0, p)) < 0 )
				  return "(unspecified)";
				/* buffer interface returns bytes, we want
				   length in characters */
				*q = count/(sizeof(Py_UNICODE)); 
				format++;
			} else {
				Py_UNICODE **p = va_arg(*p_va, Py_UNICODE **);
			
				if (PyUnicode_Check(arg))
				    *p = PyUnicode_AS_UNICODE(arg);
				else
				  return "unicode";
			}
			break;
		}

	case 'S': /* string object */
		{
			PyObject **p = va_arg(*p_va, PyObject **);
			if (PyString_Check(arg))
				*p = arg;
			else
				return "string";
			break;
		}
	
	case 'U': /* Unicode object */
		{
			PyObject **p = va_arg(*p_va, PyObject **);
			if (PyUnicode_Check(arg))
				*p = arg;
			else
				return "unicode";
			break;
		}
	
	case 'O': /* object */
		{
			PyTypeObject *type;
			PyObject **p;
			if (*format == '!') {
				type = va_arg(*p_va, PyTypeObject*);
				p = va_arg(*p_va, PyObject **);
				format++;
				if (arg->ob_type == type)
					*p = arg;
				else
					return type->tp_name;

			}
			else if (*format == '?') {
				inquiry pred = va_arg(*p_va, inquiry);
				p = va_arg(*p_va, PyObject **);
				format++;
				if ((*pred)(arg)) 
					*p = arg;
				else
					return "(unspecified)";
				
			}
			else if (*format == '&') {
				typedef int (*converter)(PyObject *, void *);
				converter convert = va_arg(*p_va, converter);
				void *addr = va_arg(*p_va, void *);
				format++;
				if (! (*convert)(arg, addr))
					return "(unspecified)";
			}
			else {
				p = va_arg(*p_va, PyObject **);
				*p = arg;
			}
			break;
		}
		
		
	case 'w': /* memory buffer, read-write access */
		{
			void **p = va_arg(*p_va, void **);
			PyBufferProcs *pb = arg->ob_type->tp_as_buffer;
			int count;
			
			if ( pb == NULL || pb->bf_getwritebuffer == NULL ||
					pb->bf_getsegcount == NULL )
				return "read-write buffer";
			if ( (*pb->bf_getsegcount)(arg, NULL) != 1 )
				return "single-segment read-write buffer";
			if ( (count = pb->bf_getwritebuffer(arg, 0, p)) < 0 )
				return "(unspecified)";
			if (*format == '#') {
				int *q = va_arg(*p_va, int *);
				
				*q = count;
				format++;
			}
			break;
		}
		
	case 't': /* 8-bit character buffer, read-only access */
		{
			const char **p = va_arg(*p_va, const char **);
			PyBufferProcs *pb = arg->ob_type->tp_as_buffer;
			int count;

			if ( *format++ != '#' )
				return "invalid use of 't' format character";
			if ( !PyType_HasFeature(
				arg->ob_type,
				Py_TPFLAGS_HAVE_GETCHARBUFFER) ||
			     pb == NULL ||
			     pb->bf_getcharbuffer == NULL ||
			     pb->bf_getsegcount == NULL )
				return "string or read-only character buffer";
			if ( (*pb->bf_getsegcount)(arg, NULL) != 1 )
				return "string or single-segment read-only buffer";
			if ( (count = pb->bf_getcharbuffer(arg, 0, p)) < 0 )
				return "(unspecified)";

			*va_arg(*p_va, int *) = count;

			break;
		}
		
	
	default:
		return "impossible<bad format char>";
	
	}
	
	*p_format = format;
	return NULL;
}


/* Support for keyword arguments donated by
   Geoff Philbrick <philbric@delphi.hks.com> */

int PyArg_ParseTupleAndKeywords(PyObject *args,
				PyObject *keywords,
				char *format, 
				char **kwlist, ...)
{
	int retval;
	va_list va;
	
	va_start(va, kwlist);
	retval = vgetargskeywords(args, keywords, format, kwlist, &va);	
	va_end(va);
	return retval;
}


static int
vgetargskeywords(PyObject *args, PyObject *keywords, char *format,
	         char **kwlist, va_list *p_va)
{
	char msgbuf[256];
	int levels[32];
	char *fname = NULL;
	char *message = NULL;
	int min = -1;
	int max = 0;
	char *formatsave = format;
	int i, len, tplen, kwlen;
	char *msg, *ks, **p;
	int nkwds, pos, match, converted;
	PyObject *key, *value;
	
	/* nested tuples cannot be parsed when using keyword arguments */
	
	for (;;) {
		int c = *format++;
		if (c == '(') {
			PyErr_SetString(PyExc_SystemError,
		      "tuple found in format when using keyword arguments");
			return 0;
		}
		else if (c == '\0')
			break;
		else if (c == ':') {
			fname = format;
			break;
		} else if (c == ';') {
			message = format;
			break;
		} else if (c == 'e')
			; /* Pass */
		else if (isalpha(c))
			max++;
		else if (c == '|')
			min = max;
	}	
	
	if (min < 0)
		min = max;
	
	format = formatsave;
	
	if (!PyTuple_Check(args)) {
		PyErr_SetString(PyExc_SystemError,
		    "new style getargs format but argument is not a tuple");
		return 0;
	}	
	
	tplen = PyTuple_GET_SIZE(args);
	
	/* do a cursory check of the keywords just to see how many we got */
	   
	if (keywords) { 	
		if (!PyDict_Check(keywords)) {
			if (keywords == NULL)
				PyErr_SetString(PyExc_SystemError,
		     "NULL received when keyword dictionary expected");
			else
				PyErr_Format(PyExc_SystemError,
		     "%s received when keyword dictionary expected",
					     keywords->ob_type->tp_name);
			return 0;
		}	
		kwlen = PyDict_Size(keywords);
	}
	else {
		kwlen = 0;
	}
			
	/* make sure there are no duplicate values for an argument;
	   its not clear when to use the term "keyword argument vs. 
	   keyword parameter in messages */
	
	if (keywords) {
		for (i = 0; i < tplen; i++) {
			if (PyMapping_HasKeyString(keywords, kwlist[i])) {
				sprintf(msgbuf,
					"keyword parameter %s redefined",
					kwlist[i]);
				PyErr_SetString(PyExc_TypeError, msgbuf);
				return 0;
			}
		}
	}
	PyErr_Clear(); /* I'm not which Py functions set the error string */
		
	/* required arguments missing from args can be supplied by keyword 
	   arguments */
	
	len = tplen;
	if (keywords && tplen < min) {
		for (i = tplen; i < min; i++) {
		  if (PyMapping_HasKeyString(keywords, kwlist[i])) {
				len++;
		  }
		}
	}
	PyErr_Clear();	
	
	/* make sure we got an acceptable number of arguments; the message
	   is a little confusing with keywords since keyword arguments
	   which are supplied, but don't match the required arguments
	   are not included in the "%d given" part of the message */

	if (len < min || max < len) {
		if (message == NULL) {
			sprintf(msgbuf,
				"%s%s takes %s %d argument%s (%d given)",
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
	
	for (i = 0; i < tplen; i++) {
		if (*format == '|')
			format++;
		msg = convertitem(PyTuple_GET_ITEM(args, i), &format, p_va,
				 levels, msgbuf);
		if (msg) {
			seterror(i+1, msg, levels, fname, message);
			return 0;
		}
	}

	/* handle no keyword parameters in call  */	
	   	   
	if (!keywords) return 1; 
		
	/* make sure the number of keywords in the keyword list matches the 
	   number of items in the format string */
	  
	nkwds = 0;
	p =  kwlist;
	for (;;) {
		if (!*(p++)) break;
		nkwds++;
	}

	if (nkwds != max) {
		PyErr_SetString(PyExc_SystemError,
	  "number of items in format string and keyword list do not match");
		return 0;
	}	  	  
			
	/* convert the keyword arguments; this uses the format 
	   string where it was left after processing args */
	
	converted = 0;
	for (i = tplen; i < nkwds; i++) {
		PyObject *item;
		if (*format == '|')
			format++;
		item = PyMapping_GetItemString(keywords, kwlist[i]);
		if (item != NULL) {
			msg = convertitem(item, &format, p_va, levels, msgbuf);
			if (msg) {
				seterror(i+1, msg, levels, fname, message);
				return 0;
			}
			converted++;
			Py_DECREF(item);
		}
		else {
			PyErr_Clear();
			msg = skipitem(&format, p_va);
			if (msg) {
				seterror(i+1, msg, levels, fname, message);
				return 0;
			}
		}
	}
	
	/* make sure there are no extraneous keyword arguments */
	
	pos = 0;
	if (converted < kwlen) {
		while (PyDict_Next(keywords, &pos, &key, &value)) {
			match = 0;
			ks = PyString_AsString(key);
			for (i = 0; i < nkwds; i++) {
				if (!strcmp(ks, kwlist[i])) {
					match = 1;
					break;
				}
			}
			if (!match) {
				sprintf(msgbuf,
			"%s is an invalid keyword argument for this function",
					ks);
				PyErr_SetString(PyExc_TypeError, msgbuf);
				return 0;
			}
		}
	}
	
	return 1;
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
	case 'L': /* LONG_LONG int */
		{
			(void) va_arg(*p_va, LONG_LONG *);
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
