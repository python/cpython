/* strop module */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include <ctype.h>

PyDoc_STRVAR(strop_module__doc__,
"Common string manipulations, optimized for speed.\n"
"\n"
"Always use \"import string\" rather than referencing\n"
"this module directly.");

/* XXX This file assumes that the <ctype.h> is*() functions
   XXX are defined for all 8-bit characters! */

#define WARN if (PyErr_Warn(PyExc_DeprecationWarning, \
		       "strop functions are obsolete; use string methods")) \
	     return NULL

/* The lstrip(), rstrip() and strip() functions are implemented
   in do_strip(), which uses an additional parameter to indicate what
   type of strip should occur. */

#define LEFTSTRIP 0
#define RIGHTSTRIP 1
#define BOTHSTRIP 2


static PyObject *
split_whitespace(char *s, Py_ssize_t len, Py_ssize_t maxsplit)
{
	Py_ssize_t i = 0, j;
	int err;
	Py_ssize_t countsplit = 0;
	PyObject* item;
	PyObject *list = PyList_New(0);

	if (list == NULL)
		return NULL;

	while (i < len) {
		while (i < len && isspace(Py_CHARMASK(s[i]))) {
			i = i+1;
		}
		j = i;
		while (i < len && !isspace(Py_CHARMASK(s[i]))) {
			i = i+1;
		}
		if (j < i) {
			item = PyString_FromStringAndSize(s+j, i-j);
			if (item == NULL)
				goto finally;

			err = PyList_Append(list, item);
			Py_DECREF(item);
			if (err < 0)
				goto finally;

			countsplit++;
			while (i < len && isspace(Py_CHARMASK(s[i]))) {
				i = i+1;
			}
			if (maxsplit && (countsplit >= maxsplit) && i < len) {
				item = PyString_FromStringAndSize(
                                        s+i, len - i);
				if (item == NULL)
					goto finally;

				err = PyList_Append(list, item);
				Py_DECREF(item);
				if (err < 0)
					goto finally;

				i = len;
			}
		}
	}
	return list;
  finally:
	Py_DECREF(list);
	return NULL;
}


PyDoc_STRVAR(splitfields__doc__,
"split(s [,sep [,maxsplit]]) -> list of strings\n"
"splitfields(s [,sep [,maxsplit]]) -> list of strings\n"
"\n"
"Return a list of the words in the string s, using sep as the\n"
"delimiter string.  If maxsplit is nonzero, splits into at most\n"
"maxsplit words.  If sep is not specified, any whitespace string\n"
"is a separator.  Maxsplit defaults to 0.\n"
"\n"
"(split and splitfields are synonymous)");

static PyObject *
strop_splitfields(PyObject *self, PyObject *args)
{
	Py_ssize_t len, n, i, j, err;
	Py_ssize_t splitcount, maxsplit;
	char *s, *sub;
	PyObject *list, *item;

	WARN;
	sub = NULL;
	n = 0;
	splitcount = 0;
	maxsplit = 0;
	if (!PyArg_ParseTuple(args, "t#|z#n:split", &s, &len, &sub, &n, &maxsplit))
		return NULL;
	if (sub == NULL)
		return split_whitespace(s, len, maxsplit);
	if (n == 0) {
		PyErr_SetString(PyExc_ValueError, "empty separator");
		return NULL;
	}

	list = PyList_New(0);
	if (list == NULL)
		return NULL;

	i = j = 0;
	while (i+n <= len) {
		if (s[i] == sub[0] && (n == 1 || memcmp(s+i, sub, n) == 0)) {
			item = PyString_FromStringAndSize(s+j, i-j);
			if (item == NULL)
				goto fail;
			err = PyList_Append(list, item);
			Py_DECREF(item);
			if (err < 0)
				goto fail;
			i = j = i + n;
			splitcount++;
			if (maxsplit && (splitcount >= maxsplit))
				break;
		}
		else
			i++;
	}
	item = PyString_FromStringAndSize(s+j, len-j);
	if (item == NULL)
		goto fail;
	err = PyList_Append(list, item);
	Py_DECREF(item);
	if (err < 0)
		goto fail;

	return list;

 fail:
	Py_DECREF(list);
	return NULL;
}


PyDoc_STRVAR(joinfields__doc__,
"join(list [,sep]) -> string\n"
"joinfields(list [,sep]) -> string\n"
"\n"
"Return a string composed of the words in list, with\n"
"intervening occurrences of sep.  Sep defaults to a single\n"
"space.\n"
"\n"
"(join and joinfields are synonymous)");

static PyObject *
strop_joinfields(PyObject *self, PyObject *args)
{
	PyObject *seq;
	char *sep = NULL;
	Py_ssize_t seqlen, seplen = 0;
	Py_ssize_t i, reslen = 0, slen = 0, sz = 100;
	PyObject *res = NULL;
	char* p = NULL;
	ssizeargfunc getitemfunc;

	WARN;
	if (!PyArg_ParseTuple(args, "O|t#:join", &seq, &sep, &seplen))
		return NULL;
	if (sep == NULL) {
		sep = " ";
		seplen = 1;
	}

	seqlen = PySequence_Size(seq);
	if (seqlen < 0 && PyErr_Occurred())
		return NULL;

	if (seqlen == 1) {
		/* Optimization if there's only one item */
		PyObject *item = PySequence_GetItem(seq, 0);
		if (item && !PyString_Check(item)) {
			PyErr_SetString(PyExc_TypeError,
				 "first argument must be sequence of strings");
			Py_DECREF(item);
			return NULL;
		}
		return item;
	}

	if (!(res = PyString_FromStringAndSize((char*)NULL, sz)))
		return NULL;
	p = PyString_AsString(res);

	/* optimize for lists, since it's the most common case.  all others
	 * (tuples and arbitrary sequences) just use the sequence abstract
	 * interface.
	 */
	if (PyList_Check(seq)) {
		for (i = 0; i < seqlen; i++) {
			PyObject *item = PyList_GET_ITEM(seq, i);
			if (!PyString_Check(item)) {
				PyErr_SetString(PyExc_TypeError,
				"first argument must be sequence of strings");
				Py_DECREF(res);
				return NULL;
			}
			slen = PyString_GET_SIZE(item);
			while (reslen + slen + seplen >= sz) {
				if (_PyString_Resize(&res, sz * 2) < 0)
					return NULL;
				sz *= 2;
				p = PyString_AsString(res) + reslen;
			}
			if (i > 0) {
				memcpy(p, sep, seplen);
				p += seplen;
				reslen += seplen;
			}
			memcpy(p, PyString_AS_STRING(item), slen);
			p += slen;
			reslen += slen;
		}
		_PyString_Resize(&res, reslen);
		return res;
	}

	if (seq->ob_type->tp_as_sequence == NULL ||
		 (getitemfunc = seq->ob_type->tp_as_sequence->sq_item) == NULL)
	{
		PyErr_SetString(PyExc_TypeError,
				"first argument must be a sequence");
		return NULL;
	}
	/* This is now type safe */
	for (i = 0; i < seqlen; i++) {
		PyObject *item = getitemfunc(seq, i);
		if (!item || !PyString_Check(item)) {
			PyErr_SetString(PyExc_TypeError,
				 "first argument must be sequence of strings");
			Py_DECREF(res);
			Py_XDECREF(item);
			return NULL;
		}
		slen = PyString_GET_SIZE(item);
		while (reslen + slen + seplen >= sz) {
			if (_PyString_Resize(&res, sz * 2) < 0) {
				Py_DECREF(item);
				return NULL;
			}
			sz *= 2;
			p = PyString_AsString(res) + reslen;
		}
		if (i > 0) {
			memcpy(p, sep, seplen);
			p += seplen;
			reslen += seplen;
		}
		memcpy(p, PyString_AS_STRING(item), slen);
		p += slen;
		reslen += slen;
		Py_DECREF(item);
	}
	_PyString_Resize(&res, reslen);
	return res;
}


PyDoc_STRVAR(find__doc__,
"find(s, sub [,start [,end]]) -> in\n"
"\n"
"Return the lowest index in s where substring sub is found,\n"
"such that sub is contained within s[start,end].  Optional\n"
"arguments start and end are interpreted as in slice notation.\n"
"\n"
"Return -1 on failure.");

static PyObject *
strop_find(PyObject *self, PyObject *args)
{
	char *s, *sub;
	Py_ssize_t len, n, i = 0, last = PY_SSIZE_T_MAX;

	WARN;
	if (!PyArg_ParseTuple(args, "t#t#|nn:find", &s, &len, &sub, &n, &i, &last))
		return NULL;

	if (last > len)
		last = len;
	if (last < 0)
		last += len;
	if (last < 0)
		last = 0;
	if (i < 0)
		i += len;
	if (i < 0)
		i = 0;

	if (n == 0 && i <= last)
		return PyInt_FromLong((long)i);

	last -= n;
	for (; i <= last; ++i)
		if (s[i] == sub[0] &&
		    (n == 1 || memcmp(&s[i+1], &sub[1], n-1) == 0))
			return PyInt_FromLong((long)i);

	return PyInt_FromLong(-1L);
}


PyDoc_STRVAR(rfind__doc__,
"rfind(s, sub [,start [,end]]) -> int\n"
"\n"
"Return the highest index in s where substring sub is found,\n"
"such that sub is contained within s[start,end].  Optional\n"
"arguments start and end are interpreted as in slice notation.\n"
"\n"
"Return -1 on failure.");

static PyObject *
strop_rfind(PyObject *self, PyObject *args)
{
	char *s, *sub;
	Py_ssize_t len, n, j;
	Py_ssize_t i = 0, last = PY_SSIZE_T_MAX;

	WARN;
	if (!PyArg_ParseTuple(args, "t#t#|nn:rfind", &s, &len, &sub, &n, &i, &last))
		return NULL;

	if (last > len)
		last = len;
	if (last < 0)
		last += len;
	if (last < 0)
		last = 0;
	if (i < 0)
		i += len;
	if (i < 0)
		i = 0;

	if (n == 0 && i <= last)
		return PyInt_FromLong((long)last);

	for (j = last-n; j >= i; --j)
		if (s[j] == sub[0] &&
		    (n == 1 || memcmp(&s[j+1], &sub[1], n-1) == 0))
			return PyInt_FromLong((long)j);

	return PyInt_FromLong(-1L);
}


static PyObject *
do_strip(PyObject *args, int striptype)
{
	char *s;
	Py_ssize_t len, i, j;


	if (PyString_AsStringAndSize(args, &s, &len))
		return NULL;

	i = 0;
	if (striptype != RIGHTSTRIP) {
		while (i < len && isspace(Py_CHARMASK(s[i]))) {
			i++;
		}
	}

	j = len;
	if (striptype != LEFTSTRIP) {
		do {
			j--;
		} while (j >= i && isspace(Py_CHARMASK(s[j])));
		j++;
	}

	if (i == 0 && j == len) {
		Py_INCREF(args);
		return args;
	}
	else
		return PyString_FromStringAndSize(s+i, j-i);
}


PyDoc_STRVAR(strip__doc__,
"strip(s) -> string\n"
"\n"
"Return a copy of the string s with leading and trailing\n"
"whitespace removed.");

static PyObject *
strop_strip(PyObject *self, PyObject *args)
{
	WARN;
	return do_strip(args, BOTHSTRIP);
}


PyDoc_STRVAR(lstrip__doc__,
"lstrip(s) -> string\n"
"\n"
"Return a copy of the string s with leading whitespace removed.");

static PyObject *
strop_lstrip(PyObject *self, PyObject *args)
{
	WARN;
	return do_strip(args, LEFTSTRIP);
}


PyDoc_STRVAR(rstrip__doc__,
"rstrip(s) -> string\n"
"\n"
"Return a copy of the string s with trailing whitespace removed.");

static PyObject *
strop_rstrip(PyObject *self, PyObject *args)
{
	WARN;
	return do_strip(args, RIGHTSTRIP);
}


PyDoc_STRVAR(lower__doc__,
"lower(s) -> string\n"
"\n"
"Return a copy of the string s converted to lowercase.");

static PyObject *
strop_lower(PyObject *self, PyObject *args)
{
	char *s, *s_new;
	Py_ssize_t i, n;
	PyObject *newstr;
	int changed;

	WARN;
	if (PyString_AsStringAndSize(args, &s, &n))
		return NULL;
	newstr = PyString_FromStringAndSize(NULL, n);
	if (newstr == NULL)
		return NULL;
	s_new = PyString_AsString(newstr);
	changed = 0;
	for (i = 0; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (isupper(c)) {
			changed = 1;
			*s_new = tolower(c);
		} else
			*s_new = c;
		s_new++;
	}
	if (!changed) {
		Py_DECREF(newstr);
		Py_INCREF(args);
		return args;
	}
	return newstr;
}


PyDoc_STRVAR(upper__doc__,
"upper(s) -> string\n"
"\n"
"Return a copy of the string s converted to uppercase.");

static PyObject *
strop_upper(PyObject *self, PyObject *args)
{
	char *s, *s_new;
	Py_ssize_t i, n;
	PyObject *newstr;
	int changed;

	WARN;
	if (PyString_AsStringAndSize(args, &s, &n))
		return NULL;
	newstr = PyString_FromStringAndSize(NULL, n);
	if (newstr == NULL)
		return NULL;
	s_new = PyString_AsString(newstr);
	changed = 0;
	for (i = 0; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (islower(c)) {
			changed = 1;
			*s_new = toupper(c);
		} else
			*s_new = c;
		s_new++;
	}
	if (!changed) {
		Py_DECREF(newstr);
		Py_INCREF(args);
		return args;
	}
	return newstr;
}


PyDoc_STRVAR(capitalize__doc__,
"capitalize(s) -> string\n"
"\n"
"Return a copy of the string s with only its first character\n"
"capitalized.");

static PyObject *
strop_capitalize(PyObject *self, PyObject *args)
{
	char *s, *s_new;
	Py_ssize_t i, n;
	PyObject *newstr;
	int changed;

	WARN;
	if (PyString_AsStringAndSize(args, &s, &n))
		return NULL;
	newstr = PyString_FromStringAndSize(NULL, n);
	if (newstr == NULL)
		return NULL;
	s_new = PyString_AsString(newstr);
	changed = 0;
	if (0 < n) {
		int c = Py_CHARMASK(*s++);
		if (islower(c)) {
			changed = 1;
			*s_new = toupper(c);
		} else
			*s_new = c;
		s_new++;
	}
	for (i = 1; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (isupper(c)) {
			changed = 1;
			*s_new = tolower(c);
		} else
			*s_new = c;
		s_new++;
	}
	if (!changed) {
		Py_DECREF(newstr);
		Py_INCREF(args);
		return args;
	}
	return newstr;
}


PyDoc_STRVAR(expandtabs__doc__,
"expandtabs(string, [tabsize]) -> string\n"
"\n"
"Expand tabs in a string, i.e. replace them by one or more spaces,\n"
"depending on the current column and the given tab size (default 8).\n"
"The column number is reset to zero after each newline occurring in the\n"
"string.  This doesn't understand other non-printing characters.");

static PyObject *
strop_expandtabs(PyObject *self, PyObject *args)
{
	/* Original by Fredrik Lundh */
	char* e;
	char* p;
	char* q;
	Py_ssize_t i, j, old_j;
	PyObject* out;
	char* string;
	Py_ssize_t stringlen;
	int tabsize = 8;

	WARN;
	/* Get arguments */
	if (!PyArg_ParseTuple(args, "s#|i:expandtabs", &string, &stringlen, &tabsize))
		return NULL;
	if (tabsize < 1) {
		PyErr_SetString(PyExc_ValueError,
				"tabsize must be at least 1");
		return NULL;
	}

	/* First pass: determine size of output string */
	i = j = old_j = 0; /* j: current column; i: total of previous lines */
	e = string + stringlen;
	for (p = string; p < e; p++) {
		if (*p == '\t') {
			j += tabsize - (j%tabsize);
			if (old_j > j) {
				PyErr_SetString(PyExc_OverflowError,
						"new string is too long");
				return NULL;
			}
			old_j = j;
		} else {
			j++;
			if (*p == '\n') {
				i += j;
				j = 0;
			}
		}
	}

	if ((i + j) < 0) {
		PyErr_SetString(PyExc_OverflowError, "new string is too long");
		return NULL;
	}

	/* Second pass: create output string and fill it */
	out = PyString_FromStringAndSize(NULL, i+j);
	if (out == NULL)
		return NULL;

	i = 0;
	q = PyString_AS_STRING(out);

	for (p = string; p < e; p++) {
		if (*p == '\t') {
			j = tabsize - (i%tabsize);
			i += j;
			while (j-- > 0)
				*q++ = ' ';
		} else {
			*q++ = *p;
			i++;
			if (*p == '\n')
				i = 0;
		}
	}

	return out;
}


PyDoc_STRVAR(count__doc__,
"count(s, sub[, start[, end]]) -> int\n"
"\n"
"Return the number of occurrences of substring sub in string\n"
"s[start:end].  Optional arguments start and end are\n"
"interpreted as in slice notation.");

static PyObject *
strop_count(PyObject *self, PyObject *args)
{
	char *s, *sub;
	Py_ssize_t len, n;
	Py_ssize_t i = 0, last = PY_SSIZE_T_MAX;
	Py_ssize_t m, r;

	WARN;
	if (!PyArg_ParseTuple(args, "t#t#|nn:count", &s, &len, &sub, &n, &i, &last))
		return NULL;
	if (last > len)
		last = len;
	if (last < 0)
		last += len;
	if (last < 0)
		last = 0;
	if (i < 0)
		i += len;
	if (i < 0)
		i = 0;
	m = last + 1 - n;
	if (n == 0)
		return PyInt_FromLong((long) (m-i));

	r = 0;
	while (i < m) {
		if (!memcmp(s+i, sub, n)) {
			r++;
			i += n;
		} else {
			i++;
		}
	}
	return PyInt_FromLong((long) r);
}


PyDoc_STRVAR(swapcase__doc__,
"swapcase(s) -> string\n"
"\n"
"Return a copy of the string s with upper case characters\n"
"converted to lowercase and vice versa.");

static PyObject *
strop_swapcase(PyObject *self, PyObject *args)
{
	char *s, *s_new;
	Py_ssize_t i, n;
	PyObject *newstr;
	int changed;

	WARN;
	if (PyString_AsStringAndSize(args, &s, &n))
		return NULL;
	newstr = PyString_FromStringAndSize(NULL, n);
	if (newstr == NULL)
		return NULL;
	s_new = PyString_AsString(newstr);
	changed = 0;
	for (i = 0; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (islower(c)) {
			changed = 1;
			*s_new = toupper(c);
		}
		else if (isupper(c)) {
			changed = 1;
			*s_new = tolower(c);
		}
		else
			*s_new = c;
		s_new++;
	}
	if (!changed) {
		Py_DECREF(newstr);
		Py_INCREF(args);
		return args;
	}
	return newstr;
}


PyDoc_STRVAR(atoi__doc__,
"atoi(s [,base]) -> int\n"
"\n"
"Return the integer represented by the string s in the given\n"
"base, which defaults to 10.  The string s must consist of one\n"
"or more digits, possibly preceded by a sign.  If base is 0, it\n"
"is chosen from the leading characters of s, 0 for octal, 0x or\n"
"0X for hexadecimal.  If base is 16, a preceding 0x or 0X is\n"
"accepted.");

static PyObject *
strop_atoi(PyObject *self, PyObject *args)
{
	char *s, *end;
	int base = 10;
	long x;
	char buffer[256]; /* For errors */

	WARN;
	if (!PyArg_ParseTuple(args, "s|i:atoi", &s, &base))
		return NULL;

	if ((base != 0 && base < 2) || base > 36) {
		PyErr_SetString(PyExc_ValueError, "invalid base for atoi()");
		return NULL;
	}

	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	errno = 0;
	if (base == 0 && s[0] == '0')
		x = (long) PyOS_strtoul(s, &end, base);
	else
		x = PyOS_strtol(s, &end, base);
	if (end == s || !isalnum(Py_CHARMASK(end[-1])))
		goto bad;
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
  bad:
		PyOS_snprintf(buffer, sizeof(buffer),
			      "invalid literal for atoi(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	else if (errno != 0) {
		PyOS_snprintf(buffer, sizeof(buffer), 
			      "atoi() literal too large: %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	return PyInt_FromLong(x);
}


PyDoc_STRVAR(atol__doc__,
"atol(s [,base]) -> long\n"
"\n"
"Return the long integer represented by the string s in the\n"
"given base, which defaults to 10.  The string s must consist\n"
"of one or more digits, possibly preceded by a sign.  If base\n"
"is 0, it is chosen from the leading characters of s, 0 for\n"
"octal, 0x or 0X for hexadecimal.  If base is 16, a preceding\n"
"0x or 0X is accepted.  A trailing L or l is not accepted,\n"
"unless base is 0.");

static PyObject *
strop_atol(PyObject *self, PyObject *args)
{
	char *s, *end;
	int base = 10;
	PyObject *x;
	char buffer[256]; /* For errors */

	WARN;
	if (!PyArg_ParseTuple(args, "s|i:atol", &s, &base))
		return NULL;

	if ((base != 0 && base < 2) || base > 36) {
		PyErr_SetString(PyExc_ValueError, "invalid base for atol()");
		return NULL;
	}

	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (s[0] == '\0') {
		PyErr_SetString(PyExc_ValueError, "empty string for atol()");
		return NULL;
	}
	x = PyLong_FromString(s, &end, base);
	if (x == NULL)
		return NULL;
	if (base == 0 && (*end == 'l' || *end == 'L'))
		end++;
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		PyOS_snprintf(buffer, sizeof(buffer),
			      "invalid literal for atol(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		Py_DECREF(x);
		return NULL;
	}
	return x;
}


PyDoc_STRVAR(atof__doc__,
"atof(s) -> float\n"
"\n"
"Return the floating point number represented by the string s.");

static PyObject *
strop_atof(PyObject *self, PyObject *args)
{
	char *s, *end;
	double x;
	char buffer[256]; /* For errors */

	WARN;
	if (!PyArg_ParseTuple(args, "s:atof", &s))
		return NULL;
	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (s[0] == '\0') {
		PyErr_SetString(PyExc_ValueError, "empty string for atof()");
		return NULL;
	}
	errno = 0;
	PyFPE_START_PROTECT("strop_atof", return 0)
	x = PyOS_ascii_strtod(s, &end);
	PyFPE_END_PROTECT(x)
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		PyOS_snprintf(buffer, sizeof(buffer),
			      "invalid literal for atof(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	else if (errno != 0) {
		PyOS_snprintf(buffer, sizeof(buffer), 
			      "atof() literal too large: %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	return PyFloat_FromDouble(x);
}


PyDoc_STRVAR(maketrans__doc__,
"maketrans(frm, to) -> string\n"
"\n"
"Return a translation table (a string of 256 bytes long)\n"
"suitable for use in string.translate.  The strings frm and to\n"
"must be of the same length.");

static PyObject *
strop_maketrans(PyObject *self, PyObject *args)
{
	unsigned char *c, *from=NULL, *to=NULL;
	Py_ssize_t i, fromlen=0, tolen=0;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "t#t#:maketrans", &from, &fromlen, &to, &tolen))
		return NULL;

	if (fromlen != tolen) {
		PyErr_SetString(PyExc_ValueError,
				"maketrans arguments must have same length");
		return NULL;
	}

	result = PyString_FromStringAndSize((char *)NULL, 256);
	if (result == NULL)
		return NULL;
	c = (unsigned char *) PyString_AS_STRING((PyStringObject *)result);
	for (i = 0; i < 256; i++)
		c[i]=(unsigned char)i;
	for (i = 0; i < fromlen; i++)
		c[from[i]]=to[i];

	return result;
}


PyDoc_STRVAR(translate__doc__,
"translate(s,table [,deletechars]) -> string\n"
"\n"
"Return a copy of the string s, where all characters occurring\n"
"in the optional argument deletechars are removed, and the\n"
"remaining characters have been mapped through the given\n"
"translation table, which must be a string of length 256.");

static PyObject *
strop_translate(PyObject *self, PyObject *args)
{
	register char *input, *table, *output;
	Py_ssize_t i; 
	int c, changed = 0;
	PyObject *input_obj;
	char *table1, *output_start, *del_table=NULL;
	Py_ssize_t inlen, tablen, dellen = 0;
	PyObject *result;
	int trans_table[256];

	WARN;
	if (!PyArg_ParseTuple(args, "St#|t#:translate", &input_obj,
			      &table1, &tablen, &del_table, &dellen))
		return NULL;
	if (tablen != 256) {
		PyErr_SetString(PyExc_ValueError,
			      "translation table must be 256 characters long");
		return NULL;
	}

	table = table1;
	inlen = PyString_GET_SIZE(input_obj);
	result = PyString_FromStringAndSize((char *)NULL, inlen);
	if (result == NULL)
		return NULL;
	output_start = output = PyString_AsString(result);
	input = PyString_AsString(input_obj);

	if (dellen == 0) {
		/* If no deletions are required, use faster code */
		for (i = inlen; --i >= 0; ) {
			c = Py_CHARMASK(*input++);
			if (Py_CHARMASK((*output++ = table[c])) != c)
				changed = 1;
		}
		if (changed)
			return result;
		Py_DECREF(result);
		Py_INCREF(input_obj);
		return input_obj;
	}

	for (i = 0; i < 256; i++)
		trans_table[i] = Py_CHARMASK(table[i]);

	for (i = 0; i < dellen; i++)
		trans_table[(int) Py_CHARMASK(del_table[i])] = -1;

	for (i = inlen; --i >= 0; ) {
		c = Py_CHARMASK(*input++);
		if (trans_table[c] != -1)
			if (Py_CHARMASK(*output++ = (char)trans_table[c]) == c)
				continue;
		changed = 1;
	}
	if (!changed) {
		Py_DECREF(result);
		Py_INCREF(input_obj);
		return input_obj;
	}
	/* Fix the size of the resulting string */
	if (inlen > 0)
		_PyString_Resize(&result, output - output_start);
	return result;
}


/* What follows is used for implementing replace().  Perry Stoll. */

/*
  mymemfind

  strstr replacement for arbitrary blocks of memory.

  Locates the first occurrence in the memory pointed to by MEM of the
  contents of memory pointed to by PAT.  Returns the index into MEM if
  found, or -1 if not found.  If len of PAT is greater than length of
  MEM, the function returns -1.
*/
static Py_ssize_t 
mymemfind(const char *mem, Py_ssize_t len, const char *pat, Py_ssize_t pat_len)
{
	register Py_ssize_t ii;

	/* pattern can not occur in the last pat_len-1 chars */
	len -= pat_len;

	for (ii = 0; ii <= len; ii++) {
		if (mem[ii] == pat[0] &&
		    (pat_len == 1 ||
		     memcmp(&mem[ii+1], &pat[1], pat_len-1) == 0)) {
			return ii;
		}
	}
	return -1;
}

/*
  mymemcnt

   Return the number of distinct times PAT is found in MEM.
   meaning mem=1111 and pat==11 returns 2.
           mem=11111 and pat==11 also return 2.
 */
static Py_ssize_t 
mymemcnt(const char *mem, Py_ssize_t len, const char *pat, Py_ssize_t pat_len)
{
	register Py_ssize_t offset = 0;
	Py_ssize_t nfound = 0;

	while (len >= 0) {
		offset = mymemfind(mem, len, pat, pat_len);
		if (offset == -1)
			break;
		mem += offset + pat_len;
		len -= offset + pat_len;
		nfound++;
	}
	return nfound;
}

/*
   mymemreplace

   Return a string in which all occurrences of PAT in memory STR are
   replaced with SUB.

   If length of PAT is less than length of STR or there are no occurrences
   of PAT in STR, then the original string is returned. Otherwise, a new
   string is allocated here and returned.

   on return, out_len is:
       the length of output string, or
       -1 if the input string is returned, or
       unchanged if an error occurs (no memory).

   return value is:
       the new string allocated locally, or
       NULL if an error occurred.
*/
static char *
mymemreplace(const char *str, Py_ssize_t len,		/* input string */
             const char *pat, Py_ssize_t pat_len,	/* pattern string to find */
             const char *sub, Py_ssize_t sub_len,	/* substitution string */
             Py_ssize_t count,				/* number of replacements */
	     Py_ssize_t *out_len)
{
	char *out_s;
	char *new_s;
	Py_ssize_t nfound, offset, new_len;

	if (len == 0 || pat_len > len)
		goto return_same;

	/* find length of output string */
	nfound = mymemcnt(str, len, pat, pat_len);
	if (count < 0)
		count = PY_SSIZE_T_MAX;
	else if (nfound > count)
		nfound = count;
	if (nfound == 0)
		goto return_same;

	new_len = len + nfound*(sub_len - pat_len);
	if (new_len == 0) {
		/* Have to allocate something for the caller to free(). */
		out_s = (char *)PyMem_MALLOC(1);
		if (out_s == NULL)
			return NULL;
		out_s[0] = '\0';
	}
	else {
		assert(new_len > 0);
		new_s = (char *)PyMem_MALLOC(new_len);
		if (new_s == NULL)
			return NULL;
		out_s = new_s;

		for (; count > 0 && len > 0; --count) {
			/* find index of next instance of pattern */
			offset = mymemfind(str, len, pat, pat_len);
			if (offset == -1)
				break;

			/* copy non matching part of input string */
			memcpy(new_s, str, offset);
			str += offset + pat_len;
			len -= offset + pat_len;

			/* copy substitute into the output string */
			new_s += offset;
			memcpy(new_s, sub, sub_len);
			new_s += sub_len;
		}
		/* copy any remaining values into output string */
		if (len > 0)
			memcpy(new_s, str, len);
	}
	*out_len = new_len;
	return out_s;

  return_same:
	*out_len = -1;
	return (char *)str; /* cast away const */
}


PyDoc_STRVAR(replace__doc__,
"replace (str, old, new[, maxsplit]) -> string\n"
"\n"
"Return a copy of string str with all occurrences of substring\n"
"old replaced by new. If the optional argument maxsplit is\n"
"given, only the first maxsplit occurrences are replaced.");

static PyObject *
strop_replace(PyObject *self, PyObject *args)
{
	char *str, *pat,*sub,*new_s;
	Py_ssize_t len,pat_len,sub_len,out_len;
	Py_ssize_t count = -1;
	PyObject *newstr;

	WARN;
	if (!PyArg_ParseTuple(args, "t#t#t#|n:replace",
			      &str, &len, &pat, &pat_len, &sub, &sub_len,
			      &count))
		return NULL;
	if (pat_len <= 0) {
		PyErr_SetString(PyExc_ValueError, "empty pattern string");
		return NULL;
	}
	/* CAUTION:  strop treats a replace count of 0 as infinity, unlke
	 * current (2.1) string.py and string methods.  Preserve this for
	 * ... well, hard to say for what <wink>.
	 */
	if (count == 0)
		count = -1;
	new_s = mymemreplace(str,len,pat,pat_len,sub,sub_len,count,&out_len);
	if (new_s == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	if (out_len == -1) {
		/* we're returning another reference to the input string */
		newstr = PyTuple_GetItem(args, 0);
		Py_XINCREF(newstr);
	}
	else {
		newstr = PyString_FromStringAndSize(new_s, out_len);
		PyMem_FREE(new_s);
	}
	return newstr;
}


/* List of functions defined in the module */

static PyMethodDef
strop_methods[] = {
	{"atof",	strop_atof,	   METH_VARARGS, atof__doc__},
	{"atoi",	strop_atoi,	   METH_VARARGS, atoi__doc__},
	{"atol",	strop_atol,	   METH_VARARGS, atol__doc__},
	{"capitalize",	strop_capitalize,  METH_O,       capitalize__doc__},
	{"count",	strop_count,	   METH_VARARGS, count__doc__},
	{"expandtabs",	strop_expandtabs,  METH_VARARGS, expandtabs__doc__},
	{"find",	strop_find,	   METH_VARARGS, find__doc__},
	{"join",	strop_joinfields,  METH_VARARGS, joinfields__doc__},
	{"joinfields",	strop_joinfields,  METH_VARARGS, joinfields__doc__},
	{"lstrip",	strop_lstrip,	   METH_O,       lstrip__doc__},
	{"lower",	strop_lower,	   METH_O,       lower__doc__},
	{"maketrans",	strop_maketrans,   METH_VARARGS, maketrans__doc__},
	{"replace",	strop_replace,	   METH_VARARGS, replace__doc__},
	{"rfind",	strop_rfind,	   METH_VARARGS, rfind__doc__},
	{"rstrip",	strop_rstrip,	   METH_O,       rstrip__doc__},
	{"split",	strop_splitfields, METH_VARARGS, splitfields__doc__},
	{"splitfields",	strop_splitfields, METH_VARARGS, splitfields__doc__},
	{"strip",	strop_strip,	   METH_O,       strip__doc__},
	{"swapcase",	strop_swapcase,    METH_O,       swapcase__doc__},
	{"translate",	strop_translate,   METH_VARARGS, translate__doc__},
	{"upper",	strop_upper,	   METH_O,       upper__doc__},
	{NULL,		NULL}	/* sentinel */
};


PyMODINIT_FUNC
initstrop(void)
{
	PyObject *m, *s;
	char buf[256];
	int c, n;
	m = Py_InitModule4("strop", strop_methods, strop_module__doc__,
			   (PyObject*)NULL, PYTHON_API_VERSION);
	if (m == NULL)
		return;

	/* Create 'whitespace' object */
	n = 0;
	for (c = 0; c < 256; c++) {
		if (isspace(c))
			buf[n++] = c;
	}
	s = PyString_FromStringAndSize(buf, n);
	if (s)
		PyModule_AddObject(m, "whitespace", s);

	/* Create 'lowercase' object */
	n = 0;
	for (c = 0; c < 256; c++) {
		if (islower(c))
			buf[n++] = c;
	}
	s = PyString_FromStringAndSize(buf, n);
	if (s)
		PyModule_AddObject(m, "lowercase", s);

	/* Create 'uppercase' object */
	n = 0;
	for (c = 0; c < 256; c++) {
		if (isupper(c))
			buf[n++] = c;
	}
	s = PyString_FromStringAndSize(buf, n);
	if (s)
		PyModule_AddObject(m, "uppercase", s);
}
