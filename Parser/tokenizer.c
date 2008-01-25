
/* Tokenizer implementation */

#include "Python.h"
#include "pgenheaders.h"

#include <ctype.h>
#include <assert.h>

#include "tokenizer.h"
#include "errcode.h"

#ifndef PGEN
#include "unicodeobject.h"
#include "stringobject.h"
#include "fileobject.h"
#include "codecs.h"
#include "abstract.h"
#endif /* PGEN */

extern char *PyOS_Readline(FILE *, FILE *, char *);
/* Return malloc'ed string including trailing \n;
   empty malloc'ed string for EOF;
   NULL if interrupted */

/* Don't ever change this -- it would break the portability of Python code */
#define TABSIZE 8

/* Convert a possibly signed character to a nonnegative int */
/* XXX This assumes characters are 8 bits wide */
#ifdef __CHAR_UNSIGNED__
#define Py_CHARMASK(c)		(c)
#else
#define Py_CHARMASK(c)		((c) & 0xff)
#endif

/* Forward */
static struct tok_state *tok_new(void);
static int tok_nextc(struct tok_state *tok);
static void tok_backup(struct tok_state *tok, int c);

/* Token names */

char *_PyParser_TokenNames[] = {
	"ENDMARKER",
	"NAME",
	"NUMBER",
	"STRING",
	"NEWLINE",
	"INDENT",
	"DEDENT",
	"LPAR",
	"RPAR",
	"LSQB",
	"RSQB",
	"COLON",
	"COMMA",
	"SEMI",
	"PLUS",
	"MINUS",
	"STAR",
	"SLASH",
	"VBAR",
	"AMPER",
	"LESS",
	"GREATER",
	"EQUAL",
	"DOT",
	"PERCENT",
	"BACKQUOTE",
	"LBRACE",
	"RBRACE",
	"EQEQUAL",
	"NOTEQUAL",
	"LESSEQUAL",
	"GREATEREQUAL",
	"TILDE",
	"CIRCUMFLEX",
	"LEFTSHIFT",
	"RIGHTSHIFT",
	"DOUBLESTAR",
	"PLUSEQUAL",
	"MINEQUAL",
	"STAREQUAL",
	"SLASHEQUAL",
	"PERCENTEQUAL",
	"AMPEREQUAL",
	"VBAREQUAL",
	"CIRCUMFLEXEQUAL",
	"LEFTSHIFTEQUAL",
	"RIGHTSHIFTEQUAL",
	"DOUBLESTAREQUAL",
	"DOUBLESLASH",
	"DOUBLESLASHEQUAL",
	"AT",
	/* This table must match the #defines in token.h! */
	"OP",
	"<ERRORTOKEN>",
	"<N_TOKENS>"
};


/* Create and initialize a new tok_state structure */

static struct tok_state *
tok_new(void)
{
	struct tok_state *tok = (struct tok_state *)PyMem_MALLOC(
                                                sizeof(struct tok_state));
	if (tok == NULL)
		return NULL;
	tok->buf = tok->cur = tok->end = tok->inp = tok->start = NULL;
	tok->done = E_OK;
	tok->fp = NULL;
	tok->tabsize = TABSIZE;
	tok->indent = 0;
	tok->indstack[0] = 0;
	tok->atbol = 1;
	tok->pendin = 0;
	tok->prompt = tok->nextprompt = NULL;
	tok->lineno = 0;
	tok->level = 0;
	tok->filename = NULL;
	tok->altwarning = 0;
	tok->alterror = 0;
	tok->alttabsize = 1;
	tok->altindstack[0] = 0;
	tok->decoding_state = 0;
	tok->decoding_erred = 0;
	tok->read_coding_spec = 0;
	tok->encoding = NULL;
        tok->cont_line = 0;
#ifndef PGEN
	tok->decoding_readline = NULL;
	tok->decoding_buffer = NULL;
#endif
	return tok;
}

#ifdef PGEN

static char *
decoding_fgets(char *s, int size, struct tok_state *tok)
{
	return fgets(s, size, tok->fp);
}

static int
decoding_feof(struct tok_state *tok)
{
	return feof(tok->fp);
}

static const char *
decode_str(const char *str, struct tok_state *tok)
{
	return str;
}

#else /* PGEN */

static char *
error_ret(struct tok_state *tok) /* XXX */
{
	tok->decoding_erred = 1;
	if (tok->fp != NULL && tok->buf != NULL) /* see PyTokenizer_Free */
		PyMem_FREE(tok->buf);
	tok->buf = NULL;
	return NULL;		/* as if it were EOF */
}

static char *
new_string(const char *s, Py_ssize_t len)
{
	char* result = (char *)PyMem_MALLOC(len + 1);
	if (result != NULL) {
		memcpy(result, s, len);
		result[len] = '\0';
	}
	return result;
}

static char *
get_normal_name(char *s)	/* for utf-8 and latin-1 */
{
	char buf[13];
	int i;
	for (i = 0; i < 12; i++) {
		int c = s[i];
		if (c == '\0') break;
		else if (c == '_') buf[i] = '-';
		else buf[i] = tolower(c);
	}
	buf[i] = '\0';
	if (strcmp(buf, "utf-8") == 0 ||
	    strncmp(buf, "utf-8-", 6) == 0) return "utf-8";
	else if (strcmp(buf, "latin-1") == 0 ||
		 strcmp(buf, "iso-8859-1") == 0 ||
		 strcmp(buf, "iso-latin-1") == 0 ||
		 strncmp(buf, "latin-1-", 8) == 0 ||
		 strncmp(buf, "iso-8859-1-", 11) == 0 ||
		 strncmp(buf, "iso-latin-1-", 12) == 0) return "iso-8859-1";
	else return s;
}

/* Return the coding spec in S, or NULL if none is found.  */

static char *
get_coding_spec(const char *s, Py_ssize_t size)
{
	Py_ssize_t i;
	/* Coding spec must be in a comment, and that comment must be
         * the only statement on the source code line. */
        for (i = 0; i < size - 6; i++) {
		if (s[i] == '#')
			break;
		if (s[i] != ' ' && s[i] != '\t' && s[i] != '\014')
			return NULL;
	}
	for (; i < size - 6; i++) { /* XXX inefficient search */
		const char* t = s + i;
		if (strncmp(t, "coding", 6) == 0) {
			const char* begin = NULL;
			t += 6;
			if (t[0] != ':' && t[0] != '=')
				continue;
			do {
				t++;
			} while (t[0] == '\x20' || t[0] == '\t');

			begin = t;
			while (isalnum(Py_CHARMASK(t[0])) ||
			       t[0] == '-' || t[0] == '_' || t[0] == '.')
				t++;

			if (begin < t) {
				char* r = new_string(begin, t - begin);
				char* q = get_normal_name(r);
				if (r != q) {
					PyMem_FREE(r);
					r = new_string(q, strlen(q));
				}
				return r;
			}
		}
	}
	return NULL;
}

/* Check whether the line contains a coding spec. If it does,
   invoke the set_readline function for the new encoding.
   This function receives the tok_state and the new encoding.
   Return 1 on success, 0 on failure.  */

static int
check_coding_spec(const char* line, Py_ssize_t size, struct tok_state *tok,
		  int set_readline(struct tok_state *, const char *))
{
	char * cs;
	int r = 1;

        if (tok->cont_line)
		/* It's a continuation line, so it can't be a coding spec. */
		return 1;
	cs = get_coding_spec(line, size);
	if (cs != NULL) {
		tok->read_coding_spec = 1;
		if (tok->encoding == NULL) {
			assert(tok->decoding_state == 1); /* raw */
			if (strcmp(cs, "utf-8") == 0 ||
			    strcmp(cs, "iso-8859-1") == 0) {
				tok->encoding = cs;
			} else {
#ifdef Py_USING_UNICODE
				r = set_readline(tok, cs);
				if (r) {
					tok->encoding = cs;
					tok->decoding_state = -1;
				}
				else
					PyMem_FREE(cs);
#else
                                /* Without Unicode support, we cannot
                                   process the coding spec. Since there
                                   won't be any Unicode literals, that
                                   won't matter. */
				PyMem_FREE(cs);
#endif
			}
		} else {	/* then, compare cs with BOM */
			r = (strcmp(tok->encoding, cs) == 0);
			PyMem_FREE(cs);
		}
	}
	if (!r) {
		cs = tok->encoding;
		if (!cs)
			cs = "with BOM";
		PyErr_Format(PyExc_SyntaxError, "encoding problem: %s", cs);
	}
	return r;
}

/* See whether the file starts with a BOM. If it does,
   invoke the set_readline function with the new encoding.
   Return 1 on success, 0 on failure.  */

static int
check_bom(int get_char(struct tok_state *),
	  void unget_char(int, struct tok_state *),
	  int set_readline(struct tok_state *, const char *),
	  struct tok_state *tok)
{
	int ch = get_char(tok);
	tok->decoding_state = 1;
	if (ch == EOF) {
		return 1;
	} else if (ch == 0xEF) {
		ch = get_char(tok); if (ch != 0xBB) goto NON_BOM;
		ch = get_char(tok); if (ch != 0xBF) goto NON_BOM;
#if 0
	/* Disable support for UTF-16 BOMs until a decision
	   is made whether this needs to be supported.  */
	} else if (ch == 0xFE) {
		ch = get_char(tok); if (ch != 0xFF) goto NON_BOM;
		if (!set_readline(tok, "utf-16-be")) return 0;
		tok->decoding_state = -1;
	} else if (ch == 0xFF) {
		ch = get_char(tok); if (ch != 0xFE) goto NON_BOM;
		if (!set_readline(tok, "utf-16-le")) return 0;
		tok->decoding_state = -1;
#endif
	} else {
		unget_char(ch, tok);
		return 1;
	}
	if (tok->encoding != NULL)
		PyMem_FREE(tok->encoding);
	tok->encoding = new_string("utf-8", 5);	/* resulting is in utf-8 */
	return 1;
  NON_BOM:
	/* any token beginning with '\xEF', '\xFE', '\xFF' is a bad token */
	unget_char(0xFF, tok);	/* XXX this will cause a syntax error */
	return 1;
}

/* Read a line of text from TOK into S, using the stream in TOK.
   Return NULL on failure, else S.

   On entry, tok->decoding_buffer will be one of:
     1) NULL: need to call tok->decoding_readline to get a new line
     2) PyUnicodeObject *: decoding_feof has called tok->decoding_readline and
           stored the result in tok->decoding_buffer
     3) PyStringObject *: previous call to fp_readl did not have enough room
           (in the s buffer) to copy entire contents of the line read
           by tok->decoding_readline.  tok->decoding_buffer has the overflow.
           In this case, fp_readl is called in a loop (with an expanded buffer)
           until the buffer ends with a '\n' (or until the end of the file is
           reached): see tok_nextc and its calls to decoding_fgets.
*/

static char *
fp_readl(char *s, int size, struct tok_state *tok)
{
#ifndef Py_USING_UNICODE
	/* In a non-Unicode built, this should never be called. */
	Py_FatalError("fp_readl should not be called in this build.");
	return NULL; /* Keep compiler happy (not reachable) */
#else
	PyObject* utf8 = NULL;
	PyObject* buf = tok->decoding_buffer;
	char *str;
	Py_ssize_t utf8len;

	/* Ask for one less byte so we can terminate it */
	assert(size > 0);
	size--;

	if (buf == NULL) {
		buf = PyObject_CallObject(tok->decoding_readline, NULL);
		if (buf == NULL)
			return error_ret(tok);
	} else {
		tok->decoding_buffer = NULL;
		if (PyString_CheckExact(buf))
			utf8 = buf;
	}
	if (utf8 == NULL) {
		utf8 = PyUnicode_AsUTF8String(buf);
		Py_DECREF(buf);
		if (utf8 == NULL)
			return error_ret(tok);
	}
	str = PyString_AsString(utf8);
	utf8len = PyString_GET_SIZE(utf8);
	if (utf8len > size) {
		tok->decoding_buffer = PyString_FromStringAndSize(str+size, utf8len-size);
		if (tok->decoding_buffer == NULL) {
			Py_DECREF(utf8);
			return error_ret(tok);
		}
		utf8len = size;
	}
	memcpy(s, str, utf8len);
	s[utf8len] = '\0';
	Py_DECREF(utf8);
	if (utf8len == 0) return NULL; /* EOF */
	return s;
#endif
}

/* Set the readline function for TOK to a StreamReader's
   readline function. The StreamReader is named ENC.

   This function is called from check_bom and check_coding_spec.

   ENC is usually identical to the future value of tok->encoding,
   except for the (currently unsupported) case of UTF-16.

   Return 1 on success, 0 on failure. */

static int
fp_setreadl(struct tok_state *tok, const char* enc)
{
	PyObject *reader, *stream, *readline;

	/* XXX: constify filename argument. */
	stream = PyFile_FromFile(tok->fp, (char*)tok->filename, "rb", NULL);
	if (stream == NULL)
		return 0;

	reader = PyCodec_StreamReader(enc, stream, NULL);
	Py_DECREF(stream);
	if (reader == NULL)
		return 0;

	readline = PyObject_GetAttrString(reader, "readline");
	Py_DECREF(reader);
	if (readline == NULL)
		return 0;

	tok->decoding_readline = readline;
	return 1;
}

/* Fetch the next byte from TOK. */

static int fp_getc(struct tok_state *tok) {
	return getc(tok->fp);
}

/* Unfetch the last byte back into TOK.  */

static void fp_ungetc(int c, struct tok_state *tok) {
	ungetc(c, tok->fp);
}

/* Read a line of input from TOK. Determine encoding
   if necessary.  */

static char *
decoding_fgets(char *s, int size, struct tok_state *tok)
{
	char *line = NULL;
	int badchar = 0;
	for (;;) {
		if (tok->decoding_state < 0) {
			/* We already have a codec associated with
			   this input. */
			line = fp_readl(s, size, tok);
			break;
		} else if (tok->decoding_state > 0) {
			/* We want a 'raw' read. */
			line = Py_UniversalNewlineFgets(s, size,
							tok->fp, NULL);
			break;
		} else {
			/* We have not yet determined the encoding.
			   If an encoding is found, use the file-pointer
			   reader functions from now on. */
			if (!check_bom(fp_getc, fp_ungetc, fp_setreadl, tok))
				return error_ret(tok);
			assert(tok->decoding_state != 0);
		}
	}
	if (line != NULL && tok->lineno < 2 && !tok->read_coding_spec) {
		if (!check_coding_spec(line, strlen(line), tok, fp_setreadl)) {
			return error_ret(tok);
		}
	}
#ifndef PGEN
	/* The default encoding is ASCII, so make sure we don't have any
           non-ASCII bytes in it. */
	if (line && !tok->encoding) {
		unsigned char *c;
		for (c = (unsigned char *)line; *c; c++)
			if (*c > 127) {
				badchar = *c;
				break;
			}
	}
	if (badchar) {
		char buf[500];
		/* Need to add 1 to the line number, since this line
		   has not been counted, yet.  */
		sprintf(buf,
			"Non-ASCII character '\\x%.2x' "
			"in file %.200s on line %i, "
			"but no encoding declared; "
			"see http://www.python.org/peps/pep-0263.html for details",
			badchar, tok->filename, tok->lineno + 1);
		PyErr_SetString(PyExc_SyntaxError, buf);
		return error_ret(tok);
	}
#endif
	return line;
}

static int
decoding_feof(struct tok_state *tok)
{
	if (tok->decoding_state >= 0) {
		return feof(tok->fp);
	} else {
		PyObject* buf = tok->decoding_buffer;
		if (buf == NULL) {
			buf = PyObject_CallObject(tok->decoding_readline, NULL);
			if (buf == NULL) {
				error_ret(tok);
				return 1;
			} else {
				tok->decoding_buffer = buf;
			}
		}
		return PyObject_Length(buf) == 0;
	}
}

/* Fetch a byte from TOK, using the string buffer. */

static int
buf_getc(struct tok_state *tok) {
	return Py_CHARMASK(*tok->str++);
}

/* Unfetch a byte from TOK, using the string buffer. */

static void
buf_ungetc(int c, struct tok_state *tok) {
	tok->str--;
	assert(Py_CHARMASK(*tok->str) == c);	/* tok->cur may point to read-only segment */
}

/* Set the readline function for TOK to ENC. For the string-based
   tokenizer, this means to just record the encoding. */

static int
buf_setreadl(struct tok_state *tok, const char* enc) {
	tok->enc = enc;
	return 1;
}

/* Return a UTF-8 encoding Python string object from the
   C byte string STR, which is encoded with ENC. */

#ifdef Py_USING_UNICODE
static PyObject *
translate_into_utf8(const char* str, const char* enc) {
	PyObject *utf8;
	PyObject* buf = PyUnicode_Decode(str, strlen(str), enc, NULL);
	if (buf == NULL)
		return NULL;
	utf8 = PyUnicode_AsUTF8String(buf);
	Py_DECREF(buf);
	return utf8;
}
#endif

/* Decode a byte string STR for use as the buffer of TOK.
   Look for encoding declarations inside STR, and record them
   inside TOK.  */

static const char *
decode_str(const char *str, struct tok_state *tok)
{
	PyObject* utf8 = NULL;
	const char *s;
	const char *newl[2] = {NULL, NULL};
	int lineno = 0;
	tok->enc = NULL;
	tok->str = str;
	if (!check_bom(buf_getc, buf_ungetc, buf_setreadl, tok))
		return error_ret(tok);
	str = tok->str;		/* string after BOM if any */
	assert(str);
#ifdef Py_USING_UNICODE
	if (tok->enc != NULL) {
		utf8 = translate_into_utf8(str, tok->enc);
		if (utf8 == NULL)
			return error_ret(tok);
		str = PyString_AsString(utf8);
	}
#endif
	for (s = str;; s++) {
		if (*s == '\0') break;
		else if (*s == '\n') {
			newl[lineno] = s;
			lineno++;
			if (lineno == 2) break;
		}
	}
	tok->enc = NULL;
	/* need to check line 1 and 2 separately since check_coding_spec
	   assumes a single line as input */
	if (newl[0]) {
		if (!check_coding_spec(str, newl[0] - str, tok, buf_setreadl))
			return error_ret(tok);
		if (tok->enc == NULL && newl[1]) {
			if (!check_coding_spec(newl[0]+1, newl[1] - newl[0],
					       tok, buf_setreadl))
				return error_ret(tok);
		}
	}
#ifdef Py_USING_UNICODE
	if (tok->enc != NULL) {
		assert(utf8 == NULL);
		utf8 = translate_into_utf8(str, tok->enc);
		if (utf8 == NULL) {
			PyErr_Format(PyExc_SyntaxError,
				"unknown encoding: %s", tok->enc);
			return error_ret(tok);
		}
		str = PyString_AsString(utf8);
	}
#endif
	assert(tok->decoding_buffer == NULL);
	tok->decoding_buffer = utf8; /* CAUTION */
	return str;
}

#endif /* PGEN */

/* Set up tokenizer for string */

struct tok_state *
PyTokenizer_FromString(const char *str)
{
	struct tok_state *tok = tok_new();
	if (tok == NULL)
		return NULL;
	str = (char *)decode_str(str, tok);
	if (str == NULL) {
		PyTokenizer_Free(tok);
		return NULL;
	}

	/* XXX: constify members. */
	tok->buf = tok->cur = tok->end = tok->inp = (char*)str;
	return tok;
}


/* Set up tokenizer for file */

struct tok_state *
PyTokenizer_FromFile(FILE *fp, char *ps1, char *ps2)
{
	struct tok_state *tok = tok_new();
	if (tok == NULL)
		return NULL;
	if ((tok->buf = (char *)PyMem_MALLOC(BUFSIZ)) == NULL) {
		PyTokenizer_Free(tok);
		return NULL;
	}
	tok->cur = tok->inp = tok->buf;
	tok->end = tok->buf + BUFSIZ;
	tok->fp = fp;
	tok->prompt = ps1;
	tok->nextprompt = ps2;
	return tok;
}


/* Free a tok_state structure */

void
PyTokenizer_Free(struct tok_state *tok)
{
	if (tok->encoding != NULL)
		PyMem_FREE(tok->encoding);
#ifndef PGEN
	Py_XDECREF(tok->decoding_readline);
	Py_XDECREF(tok->decoding_buffer);
#endif
	if (tok->fp != NULL && tok->buf != NULL)
		PyMem_FREE(tok->buf);
	PyMem_FREE(tok);
}

#if !defined(PGEN) && defined(Py_USING_UNICODE)
static int
tok_stdin_decode(struct tok_state *tok, char **inp)
{
	PyObject *enc, *sysstdin, *decoded, *utf8;
	const char *encoding;
	char *converted;

	if (PySys_GetFile((char *)"stdin", NULL) != stdin)
		return 0;
	sysstdin = PySys_GetObject("stdin");
	if (sysstdin == NULL || !PyFile_Check(sysstdin))
		return 0;

	enc = ((PyFileObject *)sysstdin)->f_encoding;
	if (enc == NULL || !PyString_Check(enc))
		return 0;
	Py_INCREF(enc);

	encoding = PyString_AsString(enc);
	decoded = PyUnicode_Decode(*inp, strlen(*inp), encoding, NULL);
	if (decoded == NULL)
		goto error_clear;

	utf8 = PyUnicode_AsEncodedString(decoded, "utf-8", NULL);
	Py_DECREF(decoded);
	if (utf8 == NULL)
		goto error_clear;

	assert(PyString_Check(utf8));
	converted = new_string(PyString_AS_STRING(utf8),
			       PyString_GET_SIZE(utf8));
	Py_DECREF(utf8);
	if (converted == NULL)
		goto error_nomem;

	PyMem_FREE(*inp);
	*inp = converted;
	if (tok->encoding != NULL)
		PyMem_FREE(tok->encoding);
	tok->encoding = new_string(encoding, strlen(encoding));
	if (tok->encoding == NULL)
		goto error_nomem;

	Py_DECREF(enc);
	return 0;

error_nomem:
	Py_DECREF(enc);
	tok->done = E_NOMEM;
	return -1;

error_clear:
	/* Fallback to iso-8859-1: for backward compatibility */
	Py_DECREF(enc);
	PyErr_Clear();
	return 0;
}
#endif

/* Get next char, updating state; error code goes into tok->done */

static int
tok_nextc(register struct tok_state *tok)
{
	for (;;) {
		if (tok->cur != tok->inp) {
			return Py_CHARMASK(*tok->cur++); /* Fast path */
		}
		if (tok->done != E_OK)
			return EOF;
		if (tok->fp == NULL) {
			char *end = strchr(tok->inp, '\n');
			if (end != NULL)
				end++;
			else {
				end = strchr(tok->inp, '\0');
				if (end == tok->inp) {
					tok->done = E_EOF;
					return EOF;
				}
			}
			if (tok->start == NULL)
				tok->buf = tok->cur;
			tok->line_start = tok->cur;
			tok->lineno++;
			tok->inp = end;
			return Py_CHARMASK(*tok->cur++);
		}
		if (tok->prompt != NULL) {
			char *newtok = PyOS_Readline(stdin, stdout, tok->prompt);
			if (tok->nextprompt != NULL)
				tok->prompt = tok->nextprompt;
			if (newtok == NULL)
				tok->done = E_INTR;
			else if (*newtok == '\0') {
				PyMem_FREE(newtok);
				tok->done = E_EOF;
			}
#if !defined(PGEN) && defined(Py_USING_UNICODE)
			else if (tok_stdin_decode(tok, &newtok) != 0)
				PyMem_FREE(newtok);
#endif
			else if (tok->start != NULL) {
				size_t start = tok->start - tok->buf;
				size_t oldlen = tok->cur - tok->buf;
				size_t newlen = oldlen + strlen(newtok);
				char *buf = tok->buf;
				buf = (char *)PyMem_REALLOC(buf, newlen+1);
				tok->lineno++;
				if (buf == NULL) {
					PyMem_FREE(tok->buf);
					tok->buf = NULL;
					PyMem_FREE(newtok);
					tok->done = E_NOMEM;
					return EOF;
				}
				tok->buf = buf;
				tok->cur = tok->buf + oldlen;
				tok->line_start = tok->cur;
				strcpy(tok->buf + oldlen, newtok);
				PyMem_FREE(newtok);
				tok->inp = tok->buf + newlen;
				tok->end = tok->inp + 1;
				tok->start = tok->buf + start;
			}
			else {
				tok->lineno++;
				if (tok->buf != NULL)
					PyMem_FREE(tok->buf);
				tok->buf = newtok;
				tok->line_start = tok->buf;
				tok->cur = tok->buf;
				tok->line_start = tok->buf;
				tok->inp = strchr(tok->buf, '\0');
				tok->end = tok->inp + 1;
			}
		}
		else {
			int done = 0;
			Py_ssize_t cur = 0;
			char *pt;
			if (tok->start == NULL) {
				if (tok->buf == NULL) {
					tok->buf = (char *)
						PyMem_MALLOC(BUFSIZ);
					if (tok->buf == NULL) {
						tok->done = E_NOMEM;
						return EOF;
					}
					tok->end = tok->buf + BUFSIZ;
				}
				if (decoding_fgets(tok->buf, (int)(tok->end - tok->buf),
					  tok) == NULL) {
					tok->done = E_EOF;
					done = 1;
				}
				else {
					tok->done = E_OK;
					tok->inp = strchr(tok->buf, '\0');
					done = tok->inp[-1] == '\n';
				}
			}
			else {
				cur = tok->cur - tok->buf;
				if (decoding_feof(tok)) {
					tok->done = E_EOF;
					done = 1;
				}
				else
					tok->done = E_OK;
			}
			tok->lineno++;
			/* Read until '\n' or EOF */
			while (!done) {
				Py_ssize_t curstart = tok->start == NULL ? -1 :
					          tok->start - tok->buf;
				Py_ssize_t curvalid = tok->inp - tok->buf;
				Py_ssize_t newsize = curvalid + BUFSIZ;
				char *newbuf = tok->buf;
				newbuf = (char *)PyMem_REALLOC(newbuf,
							       newsize);
				if (newbuf == NULL) {
					tok->done = E_NOMEM;
					tok->cur = tok->inp;
					return EOF;
				}
				tok->buf = newbuf;
				tok->inp = tok->buf + curvalid;
				tok->end = tok->buf + newsize;
				tok->start = curstart < 0 ? NULL :
					     tok->buf + curstart;
				if (decoding_fgets(tok->inp,
					       (int)(tok->end - tok->inp),
					       tok) == NULL) {
					/* Break out early on decoding
					   errors, as tok->buf will be NULL
					 */
					if (tok->decoding_erred)
						return EOF;
					/* Last line does not end in \n,
					   fake one */
					strcpy(tok->inp, "\n");
				}
				tok->inp = strchr(tok->inp, '\0');
				done = tok->inp[-1] == '\n';
			}
			if (tok->buf != NULL) {
				tok->cur = tok->buf + cur;
				tok->line_start = tok->cur;
				/* replace "\r\n" with "\n" */
				/* For Mac leave the \r, giving syntax error */
				pt = tok->inp - 2;
				if (pt >= tok->buf && *pt == '\r') {
					*pt++ = '\n';
					*pt = '\0';
					tok->inp = pt;
				}
			}
		}
		if (tok->done != E_OK) {
			if (tok->prompt != NULL)
				PySys_WriteStderr("\n");
			tok->cur = tok->inp;
			return EOF;
		}
	}
	/*NOTREACHED*/
}


/* Back-up one character */

static void
tok_backup(register struct tok_state *tok, register int c)
{
	if (c != EOF) {
		if (--tok->cur < tok->buf)
			Py_FatalError("tok_backup: begin of buffer");
		if (*tok->cur != c)
			*tok->cur = c;
	}
}


/* Return the token corresponding to a single character */

int
PyToken_OneChar(int c)
{
	switch (c) {
	case '(':	return LPAR;
	case ')':	return RPAR;
	case '[':	return LSQB;
	case ']':	return RSQB;
	case ':':	return COLON;
	case ',':	return COMMA;
	case ';':	return SEMI;
	case '+':	return PLUS;
	case '-':	return MINUS;
	case '*':	return STAR;
	case '/':	return SLASH;
	case '|':	return VBAR;
	case '&':	return AMPER;
	case '<':	return LESS;
	case '>':	return GREATER;
	case '=':	return EQUAL;
	case '.':	return DOT;
	case '%':	return PERCENT;
	case '`':	return BACKQUOTE;
	case '{':	return LBRACE;
	case '}':	return RBRACE;
	case '^':	return CIRCUMFLEX;
	case '~':	return TILDE;
	case '@':       return AT;
	default:	return OP;
	}
}


int
PyToken_TwoChars(int c1, int c2)
{
	switch (c1) {
	case '=':
		switch (c2) {
		case '=':	return EQEQUAL;
		}
		break;
	case '!':
		switch (c2) {
		case '=':	return NOTEQUAL;
		}
		break;
	case '<':
		switch (c2) {
		case '>':	return NOTEQUAL;
		case '=':	return LESSEQUAL;
		case '<':	return LEFTSHIFT;
		}
		break;
	case '>':
		switch (c2) {
		case '=':	return GREATEREQUAL;
		case '>':	return RIGHTSHIFT;
		}
		break;
	case '+':
		switch (c2) {
		case '=':	return PLUSEQUAL;
		}
		break;
	case '-':
		switch (c2) {
		case '=':	return MINEQUAL;
		}
		break;
	case '*':
		switch (c2) {
		case '*':	return DOUBLESTAR;
		case '=':	return STAREQUAL;
		}
		break;
	case '/':
		switch (c2) {
		case '/':	return DOUBLESLASH;
		case '=':	return SLASHEQUAL;
		}
		break;
	case '|':
		switch (c2) {
		case '=':	return VBAREQUAL;
		}
		break;
	case '%':
		switch (c2) {
		case '=':	return PERCENTEQUAL;
		}
		break;
	case '&':
		switch (c2) {
		case '=':	return AMPEREQUAL;
		}
		break;
	case '^':
		switch (c2) {
		case '=':	return CIRCUMFLEXEQUAL;
		}
		break;
	}
	return OP;
}

int
PyToken_ThreeChars(int c1, int c2, int c3)
{
	switch (c1) {
	case '<':
		switch (c2) {
		case '<':
			switch (c3) {
			case '=':
				return LEFTSHIFTEQUAL;
			}
			break;
		}
		break;
	case '>':
		switch (c2) {
		case '>':
			switch (c3) {
			case '=':
				return RIGHTSHIFTEQUAL;
			}
			break;
		}
		break;
	case '*':
		switch (c2) {
		case '*':
			switch (c3) {
			case '=':
				return DOUBLESTAREQUAL;
			}
			break;
		}
		break;
	case '/':
		switch (c2) {
		case '/':
			switch (c3) {
			case '=':
				return DOUBLESLASHEQUAL;
			}
			break;
		}
		break;
	}
	return OP;
}

static int
indenterror(struct tok_state *tok)
{
	if (tok->alterror) {
		tok->done = E_TABSPACE;
		tok->cur = tok->inp;
		return 1;
	}
	if (tok->altwarning) {
		PySys_WriteStderr("%s: inconsistent use of tabs and spaces "
                                  "in indentation\n", tok->filename);
		tok->altwarning = 0;
	}
	return 0;
}


/* Get next token, after space stripping etc. */

static int
tok_get(register struct tok_state *tok, char **p_start, char **p_end)
{
	register int c;
	int blankline;

	*p_start = *p_end = NULL;
  nextline:
	tok->start = NULL;
	blankline = 0;

	/* Get indentation level */
	if (tok->atbol) {
		register int col = 0;
		register int altcol = 0;
		tok->atbol = 0;
		for (;;) {
			c = tok_nextc(tok);
			if (c == ' ')
				col++, altcol++;
			else if (c == '\t') {
				col = (col/tok->tabsize + 1) * tok->tabsize;
				altcol = (altcol/tok->alttabsize + 1)
					* tok->alttabsize;
			}
			else if (c == '\014') /* Control-L (formfeed) */
				col = altcol = 0; /* For Emacs users */
			else
				break;
		}
		tok_backup(tok, c);
		if (c == '#' || c == '\n') {
			/* Lines with only whitespace and/or comments
			   shouldn't affect the indentation and are
			   not passed to the parser as NEWLINE tokens,
			   except *totally* empty lines in interactive
			   mode, which signal the end of a command group. */
			if (col == 0 && c == '\n' && tok->prompt != NULL)
				blankline = 0; /* Let it through */
			else
				blankline = 1; /* Ignore completely */
			/* We can't jump back right here since we still
			   may need to skip to the end of a comment */
		}
		if (!blankline && tok->level == 0) {
			if (col == tok->indstack[tok->indent]) {
				/* No change */
				if (altcol != tok->altindstack[tok->indent]) {
					if (indenterror(tok))
						return ERRORTOKEN;
				}
			}
			else if (col > tok->indstack[tok->indent]) {
				/* Indent -- always one */
				if (tok->indent+1 >= MAXINDENT) {
					tok->done = E_TOODEEP;
					tok->cur = tok->inp;
					return ERRORTOKEN;
				}
				if (altcol <= tok->altindstack[tok->indent]) {
					if (indenterror(tok))
						return ERRORTOKEN;
				}
				tok->pendin++;
				tok->indstack[++tok->indent] = col;
				tok->altindstack[tok->indent] = altcol;
			}
			else /* col < tok->indstack[tok->indent] */ {
				/* Dedent -- any number, must be consistent */
				while (tok->indent > 0 &&
					col < tok->indstack[tok->indent]) {
					tok->pendin--;
					tok->indent--;
				}
				if (col != tok->indstack[tok->indent]) {
					tok->done = E_DEDENT;
					tok->cur = tok->inp;
					return ERRORTOKEN;
				}
				if (altcol != tok->altindstack[tok->indent]) {
					if (indenterror(tok))
						return ERRORTOKEN;
				}
			}
		}
	}

	tok->start = tok->cur;

	/* Return pending indents/dedents */
	if (tok->pendin != 0) {
		if (tok->pendin < 0) {
			tok->pendin++;
			return DEDENT;
		}
		else {
			tok->pendin--;
			return INDENT;
		}
	}

 again:
	tok->start = NULL;
	/* Skip spaces */
	do {
		c = tok_nextc(tok);
	} while (c == ' ' || c == '\t' || c == '\014');

	/* Set start of current token */
	tok->start = tok->cur - 1;

	/* Skip comment, while looking for tab-setting magic */
	if (c == '#') {
		static char *tabforms[] = {
			"tab-width:",		/* Emacs */
			":tabstop=",		/* vim, full form */
			":ts=",			/* vim, abbreviated form */
			"set tabsize=",		/* will vi never die? */
		/* more templates can be added here to support other editors */
		};
		char cbuf[80];
		char *tp, **cp;
		tp = cbuf;
		do {
			*tp++ = c = tok_nextc(tok);
		} while (c != EOF && c != '\n' &&
			 (size_t)(tp - cbuf + 1) < sizeof(cbuf));
		*tp = '\0';
		for (cp = tabforms;
		     cp < tabforms + sizeof(tabforms)/sizeof(tabforms[0]);
		     cp++) {
			if ((tp = strstr(cbuf, *cp))) {
				int newsize = atoi(tp + strlen(*cp));

				if (newsize >= 1 && newsize <= 40) {
					tok->tabsize = newsize;
					if (Py_VerboseFlag)
					    PySys_WriteStderr(
						"Tab size set to %d\n",
						newsize);
				}
			}
		}
		while (c != EOF && c != '\n')
			c = tok_nextc(tok);
	}

	/* Check for EOF and errors now */
	if (c == EOF) {
		return tok->done == E_EOF ? ENDMARKER : ERRORTOKEN;
	}

	/* Identifier (most frequent token!) */
	if (isalpha(c) || c == '_') {
		/* Process r"", u"" and ur"" */
		switch (c) {
		case 'r':
		case 'R':
			c = tok_nextc(tok);
			if (c == '"' || c == '\'')
				goto letter_quote;
			break;
		case 'u':
		case 'U':
			c = tok_nextc(tok);
			if (c == 'r' || c == 'R')
				c = tok_nextc(tok);
			if (c == '"' || c == '\'')
				goto letter_quote;
			break;
		}
		while (isalnum(c) || c == '_') {
			c = tok_nextc(tok);
		}
		tok_backup(tok, c);
		*p_start = tok->start;
		*p_end = tok->cur;
		return NAME;
	}

	/* Newline */
	if (c == '\n') {
		tok->atbol = 1;
		if (blankline || tok->level > 0)
			goto nextline;
		*p_start = tok->start;
		*p_end = tok->cur - 1; /* Leave '\n' out of the string */
                tok->cont_line = 0;
		return NEWLINE;
	}

	/* Period or number starting with period? */
	if (c == '.') {
		c = tok_nextc(tok);
		if (isdigit(c)) {
			goto fraction;
		}
		else {
			tok_backup(tok, c);
			*p_start = tok->start;
			*p_end = tok->cur;
			return DOT;
		}
	}

	/* Number */
	if (isdigit(c)) {
		if (c == '0') {
			/* Hex or octal -- maybe. */
			c = tok_nextc(tok);
			if (c == '.')
				goto fraction;
#ifndef WITHOUT_COMPLEX
			if (c == 'j' || c == 'J')
				goto imaginary;
#endif
			if (c == 'x' || c == 'X') {
				/* Hex */
				do {
					c = tok_nextc(tok);
				} while (isxdigit(c));
			}
			else {
				int found_decimal = 0;
				/* Octal; c is first char of it */
				/* There's no 'isoctdigit' macro, sigh */
				while ('0' <= c && c < '8') {
					c = tok_nextc(tok);
				}
				if (isdigit(c)) {
					found_decimal = 1;
					do {
						c = tok_nextc(tok);
					} while (isdigit(c));
				}
				if (c == '.')
					goto fraction;
				else if (c == 'e' || c == 'E')
					goto exponent;
#ifndef WITHOUT_COMPLEX
				else if (c == 'j' || c == 'J')
					goto imaginary;
#endif
				else if (found_decimal) {
					tok->done = E_TOKEN;
					tok_backup(tok, c);
					return ERRORTOKEN;
				}
			}
			if (c == 'l' || c == 'L')
				c = tok_nextc(tok);
		}
		else {
			/* Decimal */
			do {
				c = tok_nextc(tok);
			} while (isdigit(c));
			if (c == 'l' || c == 'L')
				c = tok_nextc(tok);
			else {
				/* Accept floating point numbers. */
				if (c == '.') {
		fraction:
					/* Fraction */
					do {
						c = tok_nextc(tok);
					} while (isdigit(c));
				}
				if (c == 'e' || c == 'E') {
		exponent:
					/* Exponent part */
					c = tok_nextc(tok);
					if (c == '+' || c == '-')
						c = tok_nextc(tok);
					if (!isdigit(c)) {
						tok->done = E_TOKEN;
						tok_backup(tok, c);
						return ERRORTOKEN;
					}
					do {
						c = tok_nextc(tok);
					} while (isdigit(c));
				}
#ifndef WITHOUT_COMPLEX
				if (c == 'j' || c == 'J')
					/* Imaginary part */
		imaginary:
					c = tok_nextc(tok);
#endif
			}
		}
		tok_backup(tok, c);
		*p_start = tok->start;
		*p_end = tok->cur;
		return NUMBER;
	}

  letter_quote:
	/* String */
	if (c == '\'' || c == '"') {
		Py_ssize_t quote2 = tok->cur - tok->start + 1;
		int quote = c;
		int triple = 0;
		int tripcount = 0;
		for (;;) {
			c = tok_nextc(tok);
			if (c == '\n') {
				if (!triple) {
					tok->done = E_EOLS;
					tok_backup(tok, c);
					return ERRORTOKEN;
				}
				tripcount = 0;
                                tok->cont_line = 1; /* multiline string. */
			}
			else if (c == EOF) {
				if (triple)
					tok->done = E_EOFS;
				else
					tok->done = E_EOLS;
				tok->cur = tok->inp;
				return ERRORTOKEN;
			}
			else if (c == quote) {
				tripcount++;
				if (tok->cur - tok->start == quote2) {
					c = tok_nextc(tok);
					if (c == quote) {
						triple = 1;
						tripcount = 0;
						continue;
					}
					tok_backup(tok, c);
				}
				if (!triple || tripcount == 3)
					break;
			}
			else if (c == '\\') {
				tripcount = 0;
				c = tok_nextc(tok);
				if (c == EOF) {
					tok->done = E_EOLS;
					tok->cur = tok->inp;
					return ERRORTOKEN;
				}
			}
			else
				tripcount = 0;
		}
		*p_start = tok->start;
		*p_end = tok->cur;
		return STRING;
	}

	/* Line continuation */
	if (c == '\\') {
		c = tok_nextc(tok);
		if (c != '\n') {
			tok->done = E_LINECONT;
			tok->cur = tok->inp;
			return ERRORTOKEN;
		}
                tok->cont_line = 1;
		goto again; /* Read next line */
	}

	/* Check for two-character token */
	{
		int c2 = tok_nextc(tok);
		int token = PyToken_TwoChars(c, c2);
		if (token != OP) {
			int c3 = tok_nextc(tok);
			int token3 = PyToken_ThreeChars(c, c2, c3);
			if (token3 != OP) {
				token = token3;
			} else {
				tok_backup(tok, c3);
			}
			*p_start = tok->start;
			*p_end = tok->cur;
			return token;
		}
		tok_backup(tok, c2);
	}

	/* Keep track of parentheses nesting level */
	switch (c) {
	case '(':
	case '[':
	case '{':
		tok->level++;
		break;
	case ')':
	case ']':
	case '}':
		tok->level--;
		break;
	}

	/* Punctuation character */
	*p_start = tok->start;
	*p_end = tok->cur;
	return PyToken_OneChar(c);
}

int
PyTokenizer_Get(struct tok_state *tok, char **p_start, char **p_end)
{
	int result = tok_get(tok, p_start, p_end);
	if (tok->decoding_erred) {
		result = ERRORTOKEN;
		tok->done = E_DECODE;
	}
	return result;
}

/* This function is only called from parsetok. However, it cannot live
   there, as it must be empty for PGEN, and we can check for PGEN only
   in this file. */

#if defined(PGEN) || !defined(Py_USING_UNICODE)
char*
PyTokenizer_RestoreEncoding(struct tok_state* tok, int len, int* offset)
{
	return NULL;
}
#else
static PyObject *
dec_utf8(const char *enc, const char *text, size_t len) {
	PyObject *ret = NULL;	
	PyObject *unicode_text = PyUnicode_DecodeUTF8(text, len, "replace");
	if (unicode_text) {
		ret = PyUnicode_AsEncodedString(unicode_text, enc, "replace");
		Py_DECREF(unicode_text);
	}
	if (!ret) {
		PyErr_Clear();
	}
	return ret;
}
char *
PyTokenizer_RestoreEncoding(struct tok_state* tok, int len, int *offset)
{
	char *text = NULL;
	if (tok->encoding) {
		/* convert source to original encondig */
		PyObject *lineobj = dec_utf8(tok->encoding, tok->buf, len);
		if (lineobj != NULL) {
			int linelen = PyString_Size(lineobj);
			const char *line = PyString_AsString(lineobj);
			text = PyObject_MALLOC(linelen + 1);
			if (text != NULL && line != NULL) {
				if (linelen)
					strncpy(text, line, linelen);
				text[linelen] = '\0';
			}
			Py_DECREF(lineobj);
					
			/* adjust error offset */
			if (*offset > 1) {
				PyObject *offsetobj = dec_utf8(tok->encoding, 
							       tok->buf, *offset-1);
				if (offsetobj) {
					*offset = PyString_Size(offsetobj) + 1;
					Py_DECREF(offsetobj);
				}
			}
			
		}
	}
	return text;

}
#endif

			   

#ifdef Py_DEBUG

void
tok_dump(int type, char *start, char *end)
{
	printf("%s", _PyParser_TokenNames[type]);
	if (type == NAME || type == NUMBER || type == STRING || type == OP)
		printf("(%.*s)", (int)(end - start), start);
}

#endif
