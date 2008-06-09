#include "Python.h"
#include "bytes_methods.h"

/* Our own locale-independent ctype.h-like macros */

const unsigned int _Py_ctype_table[256] = {
    0, /* 0x0 '\x00' */
    0, /* 0x1 '\x01' */
    0, /* 0x2 '\x02' */
    0, /* 0x3 '\x03' */
    0, /* 0x4 '\x04' */
    0, /* 0x5 '\x05' */
    0, /* 0x6 '\x06' */
    0, /* 0x7 '\x07' */
    0, /* 0x8 '\x08' */
    FLAG_SPACE, /* 0x9 '\t' */
    FLAG_SPACE, /* 0xa '\n' */
    FLAG_SPACE, /* 0xb '\v' */
    FLAG_SPACE, /* 0xc '\f' */
    FLAG_SPACE, /* 0xd '\r' */
    0, /* 0xe '\x0e' */
    0, /* 0xf '\x0f' */
    0, /* 0x10 '\x10' */
    0, /* 0x11 '\x11' */
    0, /* 0x12 '\x12' */
    0, /* 0x13 '\x13' */
    0, /* 0x14 '\x14' */
    0, /* 0x15 '\x15' */
    0, /* 0x16 '\x16' */
    0, /* 0x17 '\x17' */
    0, /* 0x18 '\x18' */
    0, /* 0x19 '\x19' */
    0, /* 0x1a '\x1a' */
    0, /* 0x1b '\x1b' */
    0, /* 0x1c '\x1c' */
    0, /* 0x1d '\x1d' */
    0, /* 0x1e '\x1e' */
    0, /* 0x1f '\x1f' */
    FLAG_SPACE, /* 0x20 ' ' */
    0, /* 0x21 '!' */
    0, /* 0x22 '"' */
    0, /* 0x23 '#' */
    0, /* 0x24 '$' */
    0, /* 0x25 '%' */
    0, /* 0x26 '&' */
    0, /* 0x27 "'" */
    0, /* 0x28 '(' */
    0, /* 0x29 ')' */
    0, /* 0x2a '*' */
    0, /* 0x2b '+' */
    0, /* 0x2c ',' */
    0, /* 0x2d '-' */
    0, /* 0x2e '.' */
    0, /* 0x2f '/' */
    FLAG_DIGIT|FLAG_XDIGIT, /* 0x30 '0' */
    FLAG_DIGIT|FLAG_XDIGIT, /* 0x31 '1' */
    FLAG_DIGIT|FLAG_XDIGIT, /* 0x32 '2' */
    FLAG_DIGIT|FLAG_XDIGIT, /* 0x33 '3' */
    FLAG_DIGIT|FLAG_XDIGIT, /* 0x34 '4' */
    FLAG_DIGIT|FLAG_XDIGIT, /* 0x35 '5' */
    FLAG_DIGIT|FLAG_XDIGIT, /* 0x36 '6' */
    FLAG_DIGIT|FLAG_XDIGIT, /* 0x37 '7' */
    FLAG_DIGIT|FLAG_XDIGIT, /* 0x38 '8' */
    FLAG_DIGIT|FLAG_XDIGIT, /* 0x39 '9' */
    0, /* 0x3a ':' */
    0, /* 0x3b ';' */
    0, /* 0x3c '<' */
    0, /* 0x3d '=' */
    0, /* 0x3e '>' */
    0, /* 0x3f '?' */
    0, /* 0x40 '@' */
    FLAG_UPPER|FLAG_XDIGIT, /* 0x41 'A' */
    FLAG_UPPER|FLAG_XDIGIT, /* 0x42 'B' */
    FLAG_UPPER|FLAG_XDIGIT, /* 0x43 'C' */
    FLAG_UPPER|FLAG_XDIGIT, /* 0x44 'D' */
    FLAG_UPPER|FLAG_XDIGIT, /* 0x45 'E' */
    FLAG_UPPER|FLAG_XDIGIT, /* 0x46 'F' */
    FLAG_UPPER, /* 0x47 'G' */
    FLAG_UPPER, /* 0x48 'H' */
    FLAG_UPPER, /* 0x49 'I' */
    FLAG_UPPER, /* 0x4a 'J' */
    FLAG_UPPER, /* 0x4b 'K' */
    FLAG_UPPER, /* 0x4c 'L' */
    FLAG_UPPER, /* 0x4d 'M' */
    FLAG_UPPER, /* 0x4e 'N' */
    FLAG_UPPER, /* 0x4f 'O' */
    FLAG_UPPER, /* 0x50 'P' */
    FLAG_UPPER, /* 0x51 'Q' */
    FLAG_UPPER, /* 0x52 'R' */
    FLAG_UPPER, /* 0x53 'S' */
    FLAG_UPPER, /* 0x54 'T' */
    FLAG_UPPER, /* 0x55 'U' */
    FLAG_UPPER, /* 0x56 'V' */
    FLAG_UPPER, /* 0x57 'W' */
    FLAG_UPPER, /* 0x58 'X' */
    FLAG_UPPER, /* 0x59 'Y' */
    FLAG_UPPER, /* 0x5a 'Z' */
    0, /* 0x5b '[' */
    0, /* 0x5c '\\' */
    0, /* 0x5d ']' */
    0, /* 0x5e '^' */
    0, /* 0x5f '_' */
    0, /* 0x60 '`' */
    FLAG_LOWER|FLAG_XDIGIT, /* 0x61 'a' */
    FLAG_LOWER|FLAG_XDIGIT, /* 0x62 'b' */
    FLAG_LOWER|FLAG_XDIGIT, /* 0x63 'c' */
    FLAG_LOWER|FLAG_XDIGIT, /* 0x64 'd' */
    FLAG_LOWER|FLAG_XDIGIT, /* 0x65 'e' */
    FLAG_LOWER|FLAG_XDIGIT, /* 0x66 'f' */
    FLAG_LOWER, /* 0x67 'g' */
    FLAG_LOWER, /* 0x68 'h' */
    FLAG_LOWER, /* 0x69 'i' */
    FLAG_LOWER, /* 0x6a 'j' */
    FLAG_LOWER, /* 0x6b 'k' */
    FLAG_LOWER, /* 0x6c 'l' */
    FLAG_LOWER, /* 0x6d 'm' */
    FLAG_LOWER, /* 0x6e 'n' */
    FLAG_LOWER, /* 0x6f 'o' */
    FLAG_LOWER, /* 0x70 'p' */
    FLAG_LOWER, /* 0x71 'q' */
    FLAG_LOWER, /* 0x72 'r' */
    FLAG_LOWER, /* 0x73 's' */
    FLAG_LOWER, /* 0x74 't' */
    FLAG_LOWER, /* 0x75 'u' */
    FLAG_LOWER, /* 0x76 'v' */
    FLAG_LOWER, /* 0x77 'w' */
    FLAG_LOWER, /* 0x78 'x' */
    FLAG_LOWER, /* 0x79 'y' */
    FLAG_LOWER, /* 0x7a 'z' */
    0, /* 0x7b '{' */
    0, /* 0x7c '|' */
    0, /* 0x7d '}' */
    0, /* 0x7e '~' */
    0, /* 0x7f '\x7f' */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


const unsigned char _Py_ctype_tolower[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};

const unsigned char _Py_ctype_toupper[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};


PyDoc_STRVAR_shared(_Py_isspace__doc__,
"B.isspace() -> bool\n\
\n\
Return True if all characters in B are whitespace\n\
and there is at least one character in B, False otherwise.");

PyObject*
_Py_bytes_isspace(const char *cptr, Py_ssize_t len)
{
    register const unsigned char *p
        = (unsigned char *) cptr;
    register const unsigned char *e;

    /* Shortcut for single character strings */
    if (len == 1 && ISSPACE(*p))
        Py_RETURN_TRUE;

    /* Special case for empty strings */
    if (len == 0)
        Py_RETURN_FALSE;

    e = p + len;
    for (; p < e; p++) {
	if (!ISSPACE(*p))
            Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}


PyDoc_STRVAR_shared(_Py_isalpha__doc__,
"B.isalpha() -> bool\n\
\n\
Return True if all characters in B are alphabetic\n\
and there is at least one character in B, False otherwise.");

PyObject*
_Py_bytes_isalpha(const char *cptr, Py_ssize_t len)
{
    register const unsigned char *p
        = (unsigned char *) cptr;
    register const unsigned char *e;

    /* Shortcut for single character strings */
    if (len == 1 && ISALPHA(*p))
	Py_RETURN_TRUE;

    /* Special case for empty strings */
    if (len == 0)
	Py_RETURN_FALSE;

    e = p + len;
    for (; p < e; p++) {
	if (!ISALPHA(*p))
	    Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}


PyDoc_STRVAR_shared(_Py_isalnum__doc__,
"B.isalnum() -> bool\n\
\n\
Return True if all characters in B are alphanumeric\n\
and there is at least one character in B, False otherwise.");

PyObject*
_Py_bytes_isalnum(const char *cptr, Py_ssize_t len)
{
    register const unsigned char *p
        = (unsigned char *) cptr;
    register const unsigned char *e;

    /* Shortcut for single character strings */
    if (len == 1 && ISALNUM(*p))
	Py_RETURN_TRUE;

    /* Special case for empty strings */
    if (len == 0)
	Py_RETURN_FALSE;

    e = p + len;
    for (; p < e; p++) {
	if (!ISALNUM(*p))
	    Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}


PyDoc_STRVAR_shared(_Py_isdigit__doc__,
"B.isdigit() -> bool\n\
\n\
Return True if all characters in B are digits\n\
and there is at least one character in B, False otherwise.");

PyObject*
_Py_bytes_isdigit(const char *cptr, Py_ssize_t len)
{
    register const unsigned char *p
        = (unsigned char *) cptr;
    register const unsigned char *e;

    /* Shortcut for single character strings */
    if (len == 1 && ISDIGIT(*p))
	Py_RETURN_TRUE;

    /* Special case for empty strings */
    if (len == 0)
	Py_RETURN_FALSE;

    e = p + len;
    for (; p < e; p++) {
	if (!ISDIGIT(*p))
	    Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}


PyDoc_STRVAR_shared(_Py_islower__doc__,
"B.islower() -> bool\n\
\n\
Return True if all cased characters in B are lowercase and there is\n\
at least one cased character in B, False otherwise.");

PyObject*
_Py_bytes_islower(const char *cptr, Py_ssize_t len)
{
    register const unsigned char *p
        = (unsigned char *) cptr;
    register const unsigned char *e;
    int cased;

    /* Shortcut for single character strings */
    if (len == 1)
	return PyBool_FromLong(ISLOWER(*p));

    /* Special case for empty strings */
    if (len == 0)
	Py_RETURN_FALSE;

    e = p + len;
    cased = 0;
    for (; p < e; p++) {
	if (ISUPPER(*p))
	    Py_RETURN_FALSE;
	else if (!cased && ISLOWER(*p))
	    cased = 1;
    }
    return PyBool_FromLong(cased);
}


PyDoc_STRVAR_shared(_Py_isupper__doc__,
"B.isupper() -> bool\n\
\n\
Return True if all cased characters in B are uppercase and there is\n\
at least one cased character in B, False otherwise.");

PyObject*
_Py_bytes_isupper(const char *cptr, Py_ssize_t len)
{
    register const unsigned char *p
        = (unsigned char *) cptr;
    register const unsigned char *e;
    int cased;

    /* Shortcut for single character strings */
    if (len == 1)
	return PyBool_FromLong(ISUPPER(*p));

    /* Special case for empty strings */
    if (len == 0)
	Py_RETURN_FALSE;

    e = p + len;
    cased = 0;
    for (; p < e; p++) {
	if (ISLOWER(*p))
	    Py_RETURN_FALSE;
	else if (!cased && ISUPPER(*p))
	    cased = 1;
    }
    return PyBool_FromLong(cased);
}


PyDoc_STRVAR_shared(_Py_istitle__doc__,
"B.istitle() -> bool\n\
\n\
Return True if B is a titlecased string and there is at least one\n\
character in B, i.e. uppercase characters may only follow uncased\n\
characters and lowercase characters only cased ones. Return False\n\
otherwise.");

PyObject*
_Py_bytes_istitle(const char *cptr, Py_ssize_t len)
{
    register const unsigned char *p
        = (unsigned char *) cptr;
    register const unsigned char *e;
    int cased, previous_is_cased;

    /* Shortcut for single character strings */
    if (len == 1)
	return PyBool_FromLong(ISUPPER(*p));

    /* Special case for empty strings */
    if (len == 0)
	Py_RETURN_FALSE;

    e = p + len;
    cased = 0;
    previous_is_cased = 0;
    for (; p < e; p++) {
	register const unsigned char ch = *p;

	if (ISUPPER(ch)) {
	    if (previous_is_cased)
		Py_RETURN_FALSE;
	    previous_is_cased = 1;
	    cased = 1;
	}
	else if (ISLOWER(ch)) {
	    if (!previous_is_cased)
		Py_RETURN_FALSE;
	    previous_is_cased = 1;
	    cased = 1;
	}
	else
	    previous_is_cased = 0;
    }
    return PyBool_FromLong(cased);
}


PyDoc_STRVAR_shared(_Py_lower__doc__,
"B.lower() -> copy of B\n\
\n\
Return a copy of B with all ASCII characters converted to lowercase.");

void
_Py_bytes_lower(char *result, const char *cptr, Py_ssize_t len)
{
	Py_ssize_t i;

        /*
	newobj = PyString_FromStringAndSize(NULL, len);
	if (!newobj)
		return NULL;

	s = PyString_AS_STRING(newobj);
        */

	Py_MEMCPY(result, cptr, len);

	for (i = 0; i < len; i++) {
		int c = Py_CHARMASK(result[i]);
		if (ISUPPER(c))
			result[i] = TOLOWER(c);
	}
}


PyDoc_STRVAR_shared(_Py_upper__doc__,
"B.upper() -> copy of B\n\
\n\
Return a copy of B with all ASCII characters converted to uppercase.");

void
_Py_bytes_upper(char *result, const char *cptr, Py_ssize_t len)
{
	Py_ssize_t i;

        /*
	newobj = PyString_FromStringAndSize(NULL, len);
	if (!newobj)
		return NULL;

	s = PyString_AS_STRING(newobj);
        */

	Py_MEMCPY(result, cptr, len);

	for (i = 0; i < len; i++) {
		int c = Py_CHARMASK(result[i]);
		if (ISLOWER(c))
			result[i] = TOUPPER(c);
	}
}


PyDoc_STRVAR_shared(_Py_title__doc__,
"B.title() -> copy of B\n\
\n\
Return a titlecased version of B, i.e. ASCII words start with uppercase\n\
characters, all remaining cased characters have lowercase.");

void
_Py_bytes_title(char *result, char *s, Py_ssize_t len)
{
	Py_ssize_t i;
	int previous_is_cased = 0;

        /*
	newobj = PyString_FromStringAndSize(NULL, len);
	if (newobj == NULL)
		return NULL;
	s_new = PyString_AsString(newobj);
        */
	for (i = 0; i < len; i++) {
		int c = Py_CHARMASK(*s++);
		if (ISLOWER(c)) {
			if (!previous_is_cased)
			    c = TOUPPER(c);
			previous_is_cased = 1;
		} else if (ISUPPER(c)) {
			if (previous_is_cased)
			    c = TOLOWER(c);
			previous_is_cased = 1;
		} else
			previous_is_cased = 0;
		*result++ = c;
	}
}


PyDoc_STRVAR_shared(_Py_capitalize__doc__,
"B.capitalize() -> copy of B\n\
\n\
Return a copy of B with only its first character capitalized (ASCII).");

void
_Py_bytes_capitalize(char *result, char *s, Py_ssize_t len)
{
	Py_ssize_t i;

        /*
	newobj = PyString_FromStringAndSize(NULL, len);
	if (newobj == NULL)
		return NULL;
	s_new = PyString_AsString(newobj);
        */
	if (0 < len) {
		int c = Py_CHARMASK(*s++);
		if (ISLOWER(c))
			*result = TOUPPER(c);
		else
			*result = c;
		result++;
	}
	for (i = 1; i < len; i++) {
		int c = Py_CHARMASK(*s++);
		if (ISUPPER(c))
			*result = TOLOWER(c);
		else
			*result = c;
		result++;
	}
}


PyDoc_STRVAR_shared(_Py_swapcase__doc__,
"B.swapcase() -> copy of B\n\
\n\
Return a copy of B with uppercase ASCII characters converted\n\
to lowercase ASCII and vice versa.");

void
_Py_bytes_swapcase(char *result, char *s, Py_ssize_t len)
{
	Py_ssize_t i;

        /*
	newobj = PyString_FromStringAndSize(NULL, len);
	if (newobj == NULL)
		return NULL;
	s_new = PyString_AsString(newobj);
        */
	for (i = 0; i < len; i++) {
		int c = Py_CHARMASK(*s++);
		if (ISLOWER(c)) {
			*result = TOUPPER(c);
		}
		else if (ISUPPER(c)) {
			*result = TOLOWER(c);
		}
		else
			*result = c;
		result++;
	}
}

