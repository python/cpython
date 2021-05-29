
/* Tokenizer implementation */

#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include <ctype.h>
#include <assert.h>

#include "tokenizer.h"
#include "errcode.h"

#include "unicodeobject.h"
#include "bytesobject.h"
#include "fileobject.h"
#include "abstract.h"

/* Alternate tab spacing */
#define ALTTABSIZE 1

#define is_potential_identifier_start(c) (\
              (c >= 'a' && c <= 'z')\
               || (c >= 'A' && c <= 'Z')\
               || c == '_'\
               || (c >= 128))

#define is_potential_identifier_char(c) (\
              (c >= 'a' && c <= 'z')\
               || (c >= 'A' && c <= 'Z')\
               || (c >= '0' && c <= '9')\
               || c == '_'\
               || (c >= 128))


/* Don't ever change this -- it would break the portability of Python code */
#define TABSIZE 8

/* Forward */
static struct tok_state *tok_new(void);
static int tok_nextc(struct tok_state *tok);
static void tok_backup(struct tok_state *tok, int c);


/* Spaces in this constant are treated as "zero or more spaces or tabs" when
   tokenizing. */
static const char* type_comment_prefix = "# type: ";

/* Create and initialize a new tok_state structure */

static struct tok_state *
tok_new(void)
{
    struct tok_state *tok = (struct tok_state *)PyMem_Malloc(
                                            sizeof(struct tok_state));
    if (tok == NULL)
        return NULL;
    tok->buf = tok->cur = tok->inp = NULL;
    tok->fp_interactive = 0;
    tok->interactive_src_start = NULL;
    tok->interactive_src_end = NULL;
    tok->start = NULL;
    tok->end = NULL;
    tok->done = E_OK;
    tok->fp = NULL;
    tok->input = NULL;
    tok->tabsize = TABSIZE;
    tok->indent = 0;
    tok->indstack[0] = 0;
    tok->atbol = 1;
    tok->pendin = 0;
    tok->prompt = tok->nextprompt = NULL;
    tok->lineno = 0;
    tok->level = 0;
    tok->altindstack[0] = 0;
    tok->decoding_state = STATE_INIT;
    tok->decoding_erred = 0;
    tok->enc = NULL;
    tok->encoding = NULL;
    tok->cont_line = 0;
    tok->filename = NULL;
    tok->decoding_readline = NULL;
    tok->decoding_buffer = NULL;
    tok->type_comments = 0;
    tok->async_hacks = 0;
    tok->async_def = 0;
    tok->async_def_indent = 0;
    tok->async_def_nl = 0;
    tok->interactive_underflow = IUNDERFLOW_NORMAL;

    return tok;
}

static char *
new_string(const char *s, Py_ssize_t len, struct tok_state *tok)
{
    char* result = (char *)PyMem_Malloc(len + 1);
    if (!result) {
        tok->done = E_NOMEM;
        return NULL;
    }
    memcpy(result, s, len);
    result[len] = '\0';
    return result;
}

static char *
error_ret(struct tok_state *tok) /* XXX */
{
    tok->decoding_erred = 1;
    if (tok->fp != NULL && tok->buf != NULL) /* see PyTokenizer_Free */
        PyMem_Free(tok->buf);
    tok->buf = tok->cur = tok->inp = NULL;
    tok->start = NULL;
    tok->end = NULL;
    tok->done = E_DECODE;
    return NULL;                /* as if it were EOF */
}


static const char *
get_normal_name(const char *s)  /* for utf-8 and latin-1 */
{
    char buf[13];
    int i;
    for (i = 0; i < 12; i++) {
        int c = s[i];
        if (c == '\0')
            break;
        else if (c == '_')
            buf[i] = '-';
        else
            buf[i] = tolower(c);
    }
    buf[i] = '\0';
    if (strcmp(buf, "utf-8") == 0 ||
        strncmp(buf, "utf-8-", 6) == 0)
        return "utf-8";
    else if (strcmp(buf, "latin-1") == 0 ||
             strcmp(buf, "iso-8859-1") == 0 ||
             strcmp(buf, "iso-latin-1") == 0 ||
             strncmp(buf, "latin-1-", 8) == 0 ||
             strncmp(buf, "iso-8859-1-", 11) == 0 ||
             strncmp(buf, "iso-latin-1-", 12) == 0)
        return "iso-8859-1";
    else
        return s;
}

/* Return the coding spec in S, or NULL if none is found.  */

static int
get_coding_spec(const char *s, char **spec, Py_ssize_t size, struct tok_state *tok)
{
    Py_ssize_t i;
    *spec = NULL;
    /* Coding spec must be in a comment, and that comment must be
     * the only statement on the source code line. */
    for (i = 0; i < size - 6; i++) {
        if (s[i] == '#')
            break;
        if (s[i] != ' ' && s[i] != '\t' && s[i] != '\014')
            return 1;
    }
    for (; i < size - 6; i++) { /* XXX inefficient search */
        const char* t = s + i;
        if (memcmp(t, "coding", 6) == 0) {
            const char* begin = NULL;
            t += 6;
            if (t[0] != ':' && t[0] != '=')
                continue;
            do {
                t++;
            } while (t[0] == ' ' || t[0] == '\t');

            begin = t;
            while (Py_ISALNUM(t[0]) ||
                   t[0] == '-' || t[0] == '_' || t[0] == '.')
                t++;

            if (begin < t) {
                char* r = new_string(begin, t - begin, tok);
                const char* q;
                if (!r)
                    return 0;
                q = get_normal_name(r);
                if (r != q) {
                    PyMem_Free(r);
                    r = new_string(q, strlen(q), tok);
                    if (!r)
                        return 0;
                }
                *spec = r;
                break;
            }
        }
    }
    return 1;
}

/* Check whether the line contains a coding spec. If it does,
   invoke the set_readline function for the new encoding.
   This function receives the tok_state and the new encoding.
   Return 1 on success, 0 on failure.  */

static int
check_coding_spec(const char* line, Py_ssize_t size, struct tok_state *tok,
                  int set_readline(struct tok_state *, const char *))
{
    char *cs;
    if (tok->cont_line) {
        /* It's a continuation line, so it can't be a coding spec. */
        tok->decoding_state = STATE_NORMAL;
        return 1;
    }
    if (!get_coding_spec(line, &cs, size, tok)) {
        return 0;
    }
    if (!cs) {
        Py_ssize_t i;
        for (i = 0; i < size; i++) {
            if (line[i] == '#' || line[i] == '\n' || line[i] == '\r')
                break;
            if (line[i] != ' ' && line[i] != '\t' && line[i] != '\014') {
                /* Stop checking coding spec after a line containing
                 * anything except a comment. */
                tok->decoding_state = STATE_NORMAL;
                break;
            }
        }
        return 1;
    }
    tok->decoding_state = STATE_NORMAL;
    if (tok->encoding == NULL) {
        assert(tok->decoding_readline == NULL);
        if (strcmp(cs, "utf-8") != 0 && !set_readline(tok, cs)) {
            error_ret(tok);
            PyErr_Format(PyExc_SyntaxError, "encoding problem: %s", cs);
            PyMem_Free(cs);
            return 0;
        }
        tok->encoding = cs;
    } else {                /* then, compare cs with BOM */
        if (strcmp(tok->encoding, cs) != 0) {
            error_ret(tok);
            PyErr_Format(PyExc_SyntaxError,
                         "encoding problem: %s with BOM", cs);
            PyMem_Free(cs);
            return 0;
        }
        PyMem_Free(cs);
    }
    return 1;
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
    int ch1, ch2, ch3;
    ch1 = get_char(tok);
    tok->decoding_state = STATE_SEEK_CODING;
    if (ch1 == EOF) {
        return 1;
    } else if (ch1 == 0xEF) {
        ch2 = get_char(tok);
        if (ch2 != 0xBB) {
            unget_char(ch2, tok);
            unget_char(ch1, tok);
            return 1;
        }
        ch3 = get_char(tok);
        if (ch3 != 0xBF) {
            unget_char(ch3, tok);
            unget_char(ch2, tok);
            unget_char(ch1, tok);
            return 1;
        }
#if 0
    /* Disable support for UTF-16 BOMs until a decision
       is made whether this needs to be supported.  */
    } else if (ch1 == 0xFE) {
        ch2 = get_char(tok);
        if (ch2 != 0xFF) {
            unget_char(ch2, tok);
            unget_char(ch1, tok);
            return 1;
        }
        if (!set_readline(tok, "utf-16-be"))
            return 0;
        tok->decoding_state = STATE_NORMAL;
    } else if (ch1 == 0xFF) {
        ch2 = get_char(tok);
        if (ch2 != 0xFE) {
            unget_char(ch2, tok);
            unget_char(ch1, tok);
            return 1;
        }
        if (!set_readline(tok, "utf-16-le"))
            return 0;
        tok->decoding_state = STATE_NORMAL;
#endif
    } else {
        unget_char(ch1, tok);
        return 1;
    }
    if (tok->encoding != NULL)
        PyMem_Free(tok->encoding);
    tok->encoding = new_string("utf-8", 5, tok);
    if (!tok->encoding)
        return 0;
    /* No need to set_readline: input is already utf-8 */
    return 1;
}

static int
tok_concatenate_interactive_new_line(struct tok_state *tok, const char *line) {
    assert(tok->fp_interactive);

    if (!line) {
        return 0;
    }

    Py_ssize_t current_size = tok->interactive_src_end - tok->interactive_src_start;
    Py_ssize_t line_size = strlen(line);
    char* new_str = tok->interactive_src_start;

    new_str = PyMem_Realloc(new_str, current_size + line_size + 1);
    if (!new_str) {
        if (tok->interactive_src_start) {
            PyMem_Free(tok->interactive_src_start);
        }
        tok->interactive_src_start = NULL;
        tok->interactive_src_end = NULL;
        tok->done = E_NOMEM;
        return -1;
    }
    strcpy(new_str + current_size, line);

    tok->interactive_src_start = new_str;
    tok->interactive_src_end = new_str + current_size + line_size;
    return 0;
}


/* Read a line of text from TOK into S, using the stream in TOK.
   Return NULL on failure, else S.

   On entry, tok->decoding_buffer will be one of:
     1) NULL: need to call tok->decoding_readline to get a new line
     2) PyUnicodeObject *: decoding_feof has called tok->decoding_readline and
       stored the result in tok->decoding_buffer
     3) PyByteArrayObject *: previous call to tok_readline_recode did not have enough room
       (in the s buffer) to copy entire contents of the line read
       by tok->decoding_readline.  tok->decoding_buffer has the overflow.
       In this case, tok_readline_recode is called in a loop (with an expanded buffer)
       until the buffer ends with a '\n' (or until the end of the file is
       reached): see tok_nextc and its calls to tok_reserve_buf.
*/

static int
tok_reserve_buf(struct tok_state *tok, Py_ssize_t size)
{
    Py_ssize_t cur = tok->cur - tok->buf;
    Py_ssize_t oldsize = tok->inp - tok->buf;
    Py_ssize_t newsize = oldsize + Py_MAX(size, oldsize >> 1);
    if (newsize > tok->end - tok->buf) {
        char *newbuf = tok->buf;
        Py_ssize_t start = tok->start == NULL ? -1 : tok->start - tok->buf;
        newbuf = (char *)PyMem_Realloc(newbuf, newsize);
        if (newbuf == NULL) {
            tok->done = E_NOMEM;
            return 0;
        }
        tok->buf = newbuf;
        tok->cur = tok->buf + cur;
        tok->inp = tok->buf + oldsize;
        tok->end = tok->buf + newsize;
        tok->start = start < 0 ? NULL : tok->buf + start;
    }
    return 1;
}

static int
tok_readline_recode(struct tok_state *tok) {
    PyObject *line;
    const  char *buf;
    Py_ssize_t buflen;
    line = tok->decoding_buffer;
    if (line == NULL) {
        line = PyObject_CallNoArgs(tok->decoding_readline);
        if (line == NULL) {
            error_ret(tok);
            goto error;
        }
    }
    else {
        tok->decoding_buffer = NULL;
    }
    buf = PyUnicode_AsUTF8AndSize(line, &buflen);
    if (buf == NULL) {
        error_ret(tok);
        goto error;
    }
    if (!tok_reserve_buf(tok, buflen + 1)) {
        goto error;
    }
    memcpy(tok->inp, buf, buflen);
    tok->inp += buflen;
    *tok->inp = '\0';
    if (tok->fp_interactive &&
        tok_concatenate_interactive_new_line(tok, buf) == -1) {
        goto error;
    }
    Py_DECREF(line);
    return 1;
error:
    Py_XDECREF(line);
    return 0;
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
    PyObject *readline, *io, *stream;
    _Py_IDENTIFIER(open);
    _Py_IDENTIFIER(readline);
    int fd;
    long pos;

    fd = fileno(tok->fp);
    /* Due to buffering the file offset for fd can be different from the file
     * position of tok->fp.  If tok->fp was opened in text mode on Windows,
     * its file position counts CRLF as one char and can't be directly mapped
     * to the file offset for fd.  Instead we step back one byte and read to
     * the end of line.*/
    pos = ftell(tok->fp);
    if (pos == -1 ||
        lseek(fd, (off_t)(pos > 0 ? pos - 1 : pos), SEEK_SET) == (off_t)-1) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, NULL);
        return 0;
    }

    io = PyImport_ImportModuleNoBlock("io");
    if (io == NULL)
        return 0;

    stream = _PyObject_CallMethodId(io, &PyId_open, "isisOOO",
                    fd, "r", -1, enc, Py_None, Py_None, Py_False);
    Py_DECREF(io);
    if (stream == NULL)
        return 0;

    readline = _PyObject_GetAttrId(stream, &PyId_readline);
    Py_DECREF(stream);
    if (readline == NULL)
        return 0;
    Py_XSETREF(tok->decoding_readline, readline);

    if (pos > 0) {
        PyObject *bufobj = _PyObject_CallNoArg(readline);
        if (bufobj == NULL)
            return 0;
        Py_DECREF(bufobj);
    }

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

/* Check whether the characters at s start a valid
   UTF-8 sequence. Return the number of characters forming
   the sequence if yes, 0 if not.  */
static int valid_utf8(const unsigned char* s)
{
    int expected = 0;
    int length;
    if (*s < 0x80)
        /* single-byte code */
        return 1;
    if (*s < 0xc0)
        /* following byte */
        return 0;
    if (*s < 0xE0)
        expected = 1;
    else if (*s < 0xF0)
        expected = 2;
    else if (*s < 0xF8)
        expected = 3;
    else
        return 0;
    length = expected + 1;
    for (; expected; expected--)
        if (s[expected] < 0x80 || s[expected] >= 0xC0)
            return 0;
    return length;
}

static int
ensure_utf8(char *line, struct tok_state *tok)
{
    int badchar = 0;
    unsigned char *c;
    int length;
    for (c = (unsigned char *)line; *c; c += length) {
        if (!(length = valid_utf8(c))) {
            badchar = *c;
            break;
        }
    }
    if (badchar) {
        /* Need to add 1 to the line number, since this line
       has not been counted, yet.  */
        PyErr_Format(PyExc_SyntaxError,
                     "Non-UTF-8 code starting with '\\x%.2x' "
                     "in file %U on line %i, "
                     "but no encoding declared; "
                     "see http://python.org/dev/peps/pep-0263/ for details",
                     badchar, tok->filename, tok->lineno + 1);
        return 0;
    }
    return 1;
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
    assert(Py_CHARMASK(*tok->str) == c);        /* tok->cur may point to read-only segment */
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


static char *
translate_newlines(const char *s, int exec_input, struct tok_state *tok) {
    int skip_next_lf = 0;
    size_t needed_length = strlen(s) + 2, final_length;
    char *buf, *current;
    char c = '\0';
    buf = PyMem_Malloc(needed_length);
    if (buf == NULL) {
        tok->done = E_NOMEM;
        return NULL;
    }
    for (current = buf; *s; s++, current++) {
        c = *s;
        if (skip_next_lf) {
            skip_next_lf = 0;
            if (c == '\n') {
                c = *++s;
                if (!c)
                    break;
            }
        }
        if (c == '\r') {
            skip_next_lf = 1;
            c = '\n';
        }
        *current = c;
    }
    /* If this is exec input, add a newline to the end of the string if
       there isn't one already. */
    if (exec_input && c != '\n') {
        *current = '\n';
        current++;
    }
    *current = '\0';
    final_length = current - buf + 1;
    if (final_length < needed_length && final_length) {
        /* should never fail */
        char* result = PyMem_Realloc(buf, final_length);
        if (result == NULL) {
            PyMem_Free(buf);
        }
        buf = result;
    }
    return buf;
}

/* Decode a byte string STR for use as the buffer of TOK.
   Look for encoding declarations inside STR, and record them
   inside TOK.  */

static char *
decode_str(const char *input, int single, struct tok_state *tok)
{
    PyObject* utf8 = NULL;
    char *str;
    const char *s;
    const char *newl[2] = {NULL, NULL};
    int lineno = 0;
    tok->input = str = translate_newlines(input, single, tok);
    if (str == NULL)
        return NULL;
    tok->enc = NULL;
    tok->str = str;
    if (!check_bom(buf_getc, buf_ungetc, buf_setreadl, tok))
        return error_ret(tok);
    str = tok->str;             /* string after BOM if any */
    assert(str);
    if (tok->enc != NULL) {
        utf8 = translate_into_utf8(str, tok->enc);
        if (utf8 == NULL)
            return error_ret(tok);
        str = PyBytes_AsString(utf8);
    }
    for (s = str;; s++) {
        if (*s == '\0') break;
        else if (*s == '\n') {
            assert(lineno < 2);
            newl[lineno] = s;
            lineno++;
            if (lineno == 2) break;
        }
    }
    tok->enc = NULL;
    /* need to check line 1 and 2 separately since check_coding_spec
       assumes a single line as input */
    if (newl[0]) {
        if (!check_coding_spec(str, newl[0] - str, tok, buf_setreadl)) {
            return NULL;
        }
        if (tok->enc == NULL && tok->decoding_state != STATE_NORMAL && newl[1]) {
            if (!check_coding_spec(newl[0]+1, newl[1] - newl[0],
                                   tok, buf_setreadl))
                return NULL;
        }
    }
    if (tok->enc != NULL) {
        assert(utf8 == NULL);
        utf8 = translate_into_utf8(str, tok->enc);
        if (utf8 == NULL)
            return error_ret(tok);
        str = PyBytes_AS_STRING(utf8);
    }
    assert(tok->decoding_buffer == NULL);
    tok->decoding_buffer = utf8; /* CAUTION */
    return str;
}

/* Set up tokenizer for string */

struct tok_state *
PyTokenizer_FromString(const char *str, int exec_input)
{
    struct tok_state *tok = tok_new();
    char *decoded;

    if (tok == NULL)
        return NULL;
    decoded = decode_str(str, exec_input, tok);
    if (decoded == NULL) {
        PyTokenizer_Free(tok);
        return NULL;
    }

    tok->buf = tok->cur = tok->inp = decoded;
    tok->end = decoded;
    return tok;
}

/* Set up tokenizer for UTF-8 string */

struct tok_state *
PyTokenizer_FromUTF8(const char *str, int exec_input)
{
    struct tok_state *tok = tok_new();
    char *translated;
    if (tok == NULL)
        return NULL;
    tok->input = translated = translate_newlines(str, exec_input, tok);
    if (translated == NULL) {
        PyTokenizer_Free(tok);
        return NULL;
    }
    tok->decoding_state = STATE_NORMAL;
    tok->enc = NULL;
    tok->str = translated;
    tok->encoding = new_string("utf-8", 5, tok);
    if (!tok->encoding) {
        PyTokenizer_Free(tok);
        return NULL;
    }

    tok->buf = tok->cur = tok->inp = translated;
    tok->end = translated;
    return tok;
}

/* Set up tokenizer for file */

struct tok_state *
PyTokenizer_FromFile(FILE *fp, const char* enc,
                     const char *ps1, const char *ps2)
{
    struct tok_state *tok = tok_new();
    if (tok == NULL)
        return NULL;
    if ((tok->buf = (char *)PyMem_Malloc(BUFSIZ)) == NULL) {
        PyTokenizer_Free(tok);
        return NULL;
    }
    tok->cur = tok->inp = tok->buf;
    tok->end = tok->buf + BUFSIZ;
    tok->fp = fp;
    tok->prompt = ps1;
    tok->nextprompt = ps2;
    if (enc != NULL) {
        /* Must copy encoding declaration since it
           gets copied into the parse tree. */
        tok->encoding = new_string(enc, strlen(enc), tok);
        if (!tok->encoding) {
            PyTokenizer_Free(tok);
            return NULL;
        }
        tok->decoding_state = STATE_NORMAL;
    }
    return tok;
}

/* Free a tok_state structure */

void
PyTokenizer_Free(struct tok_state *tok)
{
    if (tok->encoding != NULL) {
        PyMem_Free(tok->encoding);
    }
    Py_XDECREF(tok->decoding_readline);
    Py_XDECREF(tok->decoding_buffer);
    Py_XDECREF(tok->filename);
    if (tok->fp != NULL && tok->buf != NULL) {
        PyMem_Free(tok->buf);
    }
    if (tok->input) {
        PyMem_Free(tok->input);
    }
    if (tok->interactive_src_start != NULL) {
        PyMem_Free(tok->interactive_src_start);
    }
    PyMem_Free(tok);
}

static int
tok_readline_raw(struct tok_state *tok)
{
    do {
        if (!tok_reserve_buf(tok, BUFSIZ)) {
            return 0;
        }
        char *line = Py_UniversalNewlineFgets(tok->inp,
                                              (int)(tok->end - tok->inp),
                                              tok->fp, NULL);
        if (line == NULL) {
            return 1;
        }
        if (tok->fp_interactive &&
            tok_concatenate_interactive_new_line(tok, line) == -1) {
            return 0;
        }
        if (*tok->inp == '\0') {
            return 0;
        }
        tok->inp = strchr(tok->inp, '\0');
    } while (tok->inp[-1] != '\n');
    return 1;
}

static int
tok_underflow_string(struct tok_state *tok) {
    char *end = strchr(tok->inp, '\n');
    if (end != NULL) {
        end++;
    }
    else {
        end = strchr(tok->inp, '\0');
        if (end == tok->inp) {
            tok->done = E_EOF;
            return 0;
        }
    }
    if (tok->start == NULL) {
        tok->buf = tok->cur;
    }
    tok->line_start = tok->cur;
    tok->lineno++;
    tok->inp = end;
    return 1;
}

static int
tok_underflow_interactive(struct tok_state *tok) {
    if (tok->interactive_underflow == IUNDERFLOW_STOP) {
        tok->done = E_INTERACT_STOP;
        return 1;
    }
    char *newtok = PyOS_Readline(stdin, stdout, tok->prompt);
    if (newtok != NULL) {
        char *translated = translate_newlines(newtok, 0, tok);
        PyMem_Free(newtok);
        if (translated == NULL) {
            return 0;
        }
        newtok = translated;
    }
    if (tok->encoding && newtok && *newtok) {
        /* Recode to UTF-8 */
        Py_ssize_t buflen;
        const char* buf;
        PyObject *u = translate_into_utf8(newtok, tok->encoding);
        PyMem_Free(newtok);
        if (u == NULL) {
            tok->done = E_DECODE;
            return 0;
        }
        buflen = PyBytes_GET_SIZE(u);
        buf = PyBytes_AS_STRING(u);
        newtok = PyMem_Malloc(buflen+1);
        if (newtok == NULL) {
            Py_DECREF(u);
            tok->done = E_NOMEM;
            return 0;
        }
        strcpy(newtok, buf);
        Py_DECREF(u);
    }
    if (tok->fp_interactive &&
        tok_concatenate_interactive_new_line(tok, newtok) == -1) {
        PyMem_Free(newtok);
        return 0;
    }
    if (tok->nextprompt != NULL) {
        tok->prompt = tok->nextprompt;
    }
    if (newtok == NULL) {
        tok->done = E_INTR;
    }
    else if (*newtok == '\0') {
        PyMem_Free(newtok);
        tok->done = E_EOF;
    }
    else if (tok->start != NULL) {
        Py_ssize_t cur_multi_line_start = tok->multi_line_start - tok->buf;
        size_t size = strlen(newtok);
        tok->lineno++;
        if (!tok_reserve_buf(tok, size + 1)) {
            PyMem_Free(tok->buf);
            tok->buf = NULL;
            PyMem_Free(newtok);
            return 0;
        }
        memcpy(tok->cur, newtok, size + 1);
        PyMem_Free(newtok);
        tok->inp += size;
        tok->multi_line_start = tok->buf + cur_multi_line_start;
    }
    else {
        tok->lineno++;
        PyMem_Free(tok->buf);
        tok->buf = newtok;
        tok->cur = tok->buf;
        tok->line_start = tok->buf;
        tok->inp = strchr(tok->buf, '\0');
        tok->end = tok->inp + 1;
    }
    if (tok->done != E_OK) {
        if (tok->prompt != NULL) {
            PySys_WriteStderr("\n");
        }
        return 0;
    }
    return 1;
}

static int
tok_underflow_file(struct tok_state *tok) {
    if (tok->start == NULL) {
        tok->cur = tok->inp = tok->buf;
    }
    if (tok->decoding_state == STATE_INIT) {
        /* We have not yet determined the encoding.
           If an encoding is found, use the file-pointer
           reader functions from now on. */
        if (!check_bom(fp_getc, fp_ungetc, fp_setreadl, tok)) {
            error_ret(tok);
            return 0;
        }
        assert(tok->decoding_state != STATE_INIT);
    }
    /* Read until '\n' or EOF */
    if (tok->decoding_readline != NULL) {
        /* We already have a codec associated with this input. */
        if (!tok_readline_recode(tok)) {
            return 0;
        }
    }
    else {
        /* We want a 'raw' read. */
        if (!tok_readline_raw(tok)) {
            return 0;
        }
    }
    if (tok->inp == tok->cur) {
        tok->done = E_EOF;
        return 0;
    }
    if (tok->inp[-1] != '\n') {
        /* Last line does not end in \n, fake one */
        *tok->inp++ = '\n';
        *tok->inp = '\0';
    }

    tok->lineno++;
    if (tok->decoding_state != STATE_NORMAL) {
        if (tok->lineno > 2) {
            tok->decoding_state = STATE_NORMAL;
        }
        else if (!check_coding_spec(tok->cur, strlen(tok->cur),
                                    tok, fp_setreadl))
        {
            return 0;
        }
    }
    /* The default encoding is UTF-8, so make sure we don't have any
       non-UTF-8 sequences in it. */
    if (!tok->encoding
        && (tok->decoding_state != STATE_NORMAL || tok->lineno >= 2)) {
        if (!ensure_utf8(tok->cur, tok)) {
            error_ret(tok);
            return 0;
        }
    }
    assert(tok->done == E_OK);
    return tok->done == E_OK;
}

static void
print_escape(FILE *f, const char *s, Py_ssize_t size)
{
    if (s == NULL) {
        fputs("NULL", f);
        return;
    }
    putc('"', f);
    while (size-- > 0) {
        unsigned char c = *s++;
        switch (c) {
            case '\n': fputs("\\n", f); break;
            case '\r': fputs("\\r", f); break;
            case '\t': fputs("\\t", f); break;
            case '\f': fputs("\\f", f); break;
            case '\'': fputs("\\'", f); break;
            case '"': fputs("\\\"", f); break;
            default:
                if (0x20 <= c && c <= 0x7f)
                    putc(c, f);
                else
                    fprintf(f, "\\x%02x", c);
        }
    }
    putc('"', f);
}

/* Get next char, updating state; error code goes into tok->done */

static int
tok_nextc(struct tok_state *tok)
{
    int rc;
    for (;;) {
        if (tok->cur != tok->inp) {
            return Py_CHARMASK(*tok->cur++); /* Fast path */
        }
        if (tok->done != E_OK)
            return EOF;
        if (tok->fp == NULL) {
            rc = tok_underflow_string(tok);
        }
        else if (tok->prompt != NULL) {
            rc = tok_underflow_interactive(tok);
        }
        else {
            rc = tok_underflow_file(tok);
        }
        if (Py_DebugFlag) {
            printf("line[%d] = ", tok->lineno);
            print_escape(stdout, tok->cur, tok->inp - tok->cur);
            printf("  tok->done = %d\n", tok->done);
        }
        if (!rc) {
            tok->cur = tok->inp;
            return EOF;
        }
        tok->line_start = tok->cur;
    }
    Py_UNREACHABLE();
}

/* Back-up one character */

static void
tok_backup(struct tok_state *tok, int c)
{
    if (c != EOF) {
        if (--tok->cur < tok->buf) {
            Py_FatalError("tokenizer beginning of buffer");
        }
        if ((int)(unsigned char)*tok->cur != c) {
            Py_FatalError("tok_backup: wrong character");
        }
    }
}


static int
syntaxerror(struct tok_state *tok, const char *format, ...)
{
    PyObject *errmsg, *errtext, *args;
    va_list vargs;
#ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, format);
#else
    va_start(vargs);
#endif
    errmsg = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    if (!errmsg) {
        goto error;
    }

    errtext = PyUnicode_DecodeUTF8(tok->line_start, tok->cur - tok->line_start,
                                   "replace");
    if (!errtext) {
        goto error;
    }
    int offset = (int)PyUnicode_GET_LENGTH(errtext);
    Py_ssize_t line_len = strcspn(tok->line_start, "\n");
    if (line_len != tok->cur - tok->line_start) {
        Py_DECREF(errtext);
        errtext = PyUnicode_DecodeUTF8(tok->line_start, line_len,
                                       "replace");
    }
    if (!errtext) {
        goto error;
    }

    args = Py_BuildValue("(O(OiiN))", errmsg,
                         tok->filename, tok->lineno, offset, errtext);
    if (args) {
        PyErr_SetObject(PyExc_SyntaxError, args);
        Py_DECREF(args);
    }

error:
    Py_XDECREF(errmsg);
    tok->done = E_ERROR;
    return ERRORTOKEN;
}

static int
indenterror(struct tok_state *tok)
{
    tok->done = E_TABSPACE;
    tok->cur = tok->inp;
    return ERRORTOKEN;
}

/* Verify that the identifier follows PEP 3131.
   All identifier strings are guaranteed to be "ready" unicode objects.
 */
static int
verify_identifier(struct tok_state *tok)
{
    PyObject *s;
    if (tok->decoding_erred)
        return 0;
    s = PyUnicode_DecodeUTF8(tok->start, tok->cur - tok->start, NULL);
    if (s == NULL) {
        if (PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)) {
            tok->done = E_DECODE;
        }
        else {
            tok->done = E_ERROR;
        }
        return 0;
    }
    Py_ssize_t invalid = _PyUnicode_ScanIdentifier(s);
    if (invalid < 0) {
        Py_DECREF(s);
        tok->done = E_ERROR;
        return 0;
    }
    assert(PyUnicode_GET_LENGTH(s) > 0);
    if (invalid < PyUnicode_GET_LENGTH(s)) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(s, invalid);
        if (invalid + 1 < PyUnicode_GET_LENGTH(s)) {
            /* Determine the offset in UTF-8 encoded input */
            Py_SETREF(s, PyUnicode_Substring(s, 0, invalid + 1));
            if (s != NULL) {
                Py_SETREF(s, PyUnicode_AsUTF8String(s));
            }
            if (s == NULL) {
                tok->done = E_ERROR;
                return 0;
            }
            tok->cur = (char *)tok->start + PyBytes_GET_SIZE(s);
        }
        Py_DECREF(s);
        // PyUnicode_FromFormatV() does not support %X
        char hex[9];
        (void)PyOS_snprintf(hex, sizeof(hex), "%04X", ch);
        if (Py_UNICODE_ISPRINTABLE(ch)) {
            syntaxerror(tok, "invalid character '%c' (U+%s)", ch, hex);
        }
        else {
            syntaxerror(tok, "invalid non-printable character U+%s", hex);
        }
        return 0;
    }
    Py_DECREF(s);
    return 1;
}

static int
tok_decimal_tail(struct tok_state *tok)
{
    int c;

    while (1) {
        do {
            c = tok_nextc(tok);
        } while (isdigit(c));
        if (c != '_') {
            break;
        }
        c = tok_nextc(tok);
        if (!isdigit(c)) {
            tok_backup(tok, c);
            syntaxerror(tok, "invalid decimal literal");
            return 0;
        }
    }
    return c;
}

/* Get next token, after space stripping etc. */

static int
tok_get(struct tok_state *tok, const char **p_start, const char **p_end)
{
    int c;
    int blankline, nonascii;

    *p_start = *p_end = NULL;
  nextline:
    tok->start = NULL;
    blankline = 0;

    /* Get indentation level */
    if (tok->atbol) {
        int col = 0;
        int altcol = 0;
        tok->atbol = 0;
        for (;;) {
            c = tok_nextc(tok);
            if (c == ' ') {
                col++, altcol++;
            }
            else if (c == '\t') {
                col = (col / tok->tabsize + 1) * tok->tabsize;
                altcol = (altcol / ALTTABSIZE + 1) * ALTTABSIZE;
            }
            else if (c == '\014')  {/* Control-L (formfeed) */
                col = altcol = 0; /* For Emacs users */
            }
            else {
                break;
            }
        }
        tok_backup(tok, c);
        if (c == '#' || c == '\n' || c == '\\') {
            /* Lines with only whitespace and/or comments
               and/or a line continuation character
               shouldn't affect the indentation and are
               not passed to the parser as NEWLINE tokens,
               except *totally* empty lines in interactive
               mode, which signal the end of a command group. */
            if (col == 0 && c == '\n' && tok->prompt != NULL) {
                blankline = 0; /* Let it through */
            }
            else if (tok->prompt != NULL && tok->lineno == 1) {
                /* In interactive mode, if the first line contains
                   only spaces and/or a comment, let it through. */
                blankline = 0;
                col = altcol = 0;
            }
            else {
                blankline = 1; /* Ignore completely */
            }
            /* We can't jump back right here since we still
               may need to skip to the end of a comment */
        }
        if (!blankline && tok->level == 0) {
            if (col == tok->indstack[tok->indent]) {
                /* No change */
                if (altcol != tok->altindstack[tok->indent]) {
                    return indenterror(tok);
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
                    return indenterror(tok);
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
                    return indenterror(tok);
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

    /* Peek ahead at the next character */
    c = tok_nextc(tok);
    tok_backup(tok, c);
    /* Check if we are closing an async function */
    if (tok->async_def
        && !blankline
        /* Due to some implementation artifacts of type comments,
         * a TYPE_COMMENT at the start of a function won't set an
         * indentation level and it will produce a NEWLINE after it.
         * To avoid spuriously ending an async function due to this,
         * wait until we have some non-newline char in front of us. */
        && c != '\n'
        && tok->level == 0
        /* There was a NEWLINE after ASYNC DEF,
           so we're past the signature. */
        && tok->async_def_nl
        /* Current indentation level is less than where
           the async function was defined */
        && tok->async_def_indent >= tok->indent)
    {
        tok->async_def = 0;
        tok->async_def_indent = 0;
        tok->async_def_nl = 0;
    }

 again:
    tok->start = NULL;
    /* Skip spaces */
    do {
        c = tok_nextc(tok);
    } while (c == ' ' || c == '\t' || c == '\014');

    /* Set start of current token */
    tok->start = tok->cur - 1;

    /* Skip comment, unless it's a type comment */
    if (c == '#') {
        const char *prefix, *p, *type_start;

        while (c != EOF && c != '\n') {
            c = tok_nextc(tok);
        }

        if (tok->type_comments) {
            p = tok->start;
            prefix = type_comment_prefix;
            while (*prefix && p < tok->cur) {
                if (*prefix == ' ') {
                    while (*p == ' ' || *p == '\t') {
                        p++;
                    }
                } else if (*prefix == *p) {
                    p++;
                } else {
                    break;
                }

                prefix++;
            }

            /* This is a type comment if we matched all of type_comment_prefix. */
            if (!*prefix) {
                int is_type_ignore = 1;
                const char *ignore_end = p + 6;
                tok_backup(tok, c);  /* don't eat the newline or EOF */

                type_start = p;

                /* A TYPE_IGNORE is "type: ignore" followed by the end of the token
                 * or anything ASCII and non-alphanumeric. */
                is_type_ignore = (
                    tok->cur >= ignore_end && memcmp(p, "ignore", 6) == 0
                    && !(tok->cur > ignore_end
                         && ((unsigned char)ignore_end[0] >= 128 || Py_ISALNUM(ignore_end[0]))));

                if (is_type_ignore) {
                    *p_start = ignore_end;
                    *p_end = tok->cur;

                    /* If this type ignore is the only thing on the line, consume the newline also. */
                    if (blankline) {
                        tok_nextc(tok);
                        tok->atbol = 1;
                    }
                    return TYPE_IGNORE;
                } else {
                    *p_start = type_start;  /* after type_comment_prefix */
                    *p_end = tok->cur;
                    return TYPE_COMMENT;
                }
            }
        }
    }

    if (tok->done == E_INTERACT_STOP) {
        return ENDMARKER;
    }

    /* Check for EOF and errors now */
    if (c == EOF) {
        if (tok->level) {
            return ERRORTOKEN;
        }
        return tok->done == E_EOF ? ENDMARKER : ERRORTOKEN;
    }

    /* Identifier (most frequent token!) */
    nonascii = 0;
    if (is_potential_identifier_start(c)) {
        /* Process the various legal combinations of b"", r"", u"", and f"". */
        int saw_b = 0, saw_r = 0, saw_u = 0, saw_f = 0;
        while (1) {
            if (!(saw_b || saw_u || saw_f) && (c == 'b' || c == 'B'))
                saw_b = 1;
            /* Since this is a backwards compatibility support literal we don't
               want to support it in arbitrary order like byte literals. */
            else if (!(saw_b || saw_u || saw_r || saw_f)
                     && (c == 'u'|| c == 'U')) {
                saw_u = 1;
            }
            /* ur"" and ru"" are not supported */
            else if (!(saw_r || saw_u) && (c == 'r' || c == 'R')) {
                saw_r = 1;
            }
            else if (!(saw_f || saw_b || saw_u) && (c == 'f' || c == 'F')) {
                saw_f = 1;
            }
            else {
                break;
            }
            c = tok_nextc(tok);
            if (c == '"' || c == '\'') {
                goto letter_quote;
            }
        }
        while (is_potential_identifier_char(c)) {
            if (c >= 128) {
                nonascii = 1;
            }
            c = tok_nextc(tok);
        }
        tok_backup(tok, c);
        if (nonascii && !verify_identifier(tok)) {
            return ERRORTOKEN;
        }

        *p_start = tok->start;
        *p_end = tok->cur;

        /* async/await parsing block. */
        if (tok->cur - tok->start == 5 && tok->start[0] == 'a') {
            /* May be an 'async' or 'await' token.  For Python 3.7 or
               later we recognize them unconditionally.  For Python
               3.5 or 3.6 we recognize 'async' in front of 'def', and
               either one inside of 'async def'.  (Technically we
               shouldn't recognize these at all for 3.4 or earlier,
               but there's no *valid* Python 3.4 code that would be
               rejected, and async functions will be rejected in a
               later phase.) */
            if (!tok->async_hacks || tok->async_def) {
                /* Always recognize the keywords. */
                if (memcmp(tok->start, "async", 5) == 0) {
                    return ASYNC;
                }
                if (memcmp(tok->start, "await", 5) == 0) {
                    return AWAIT;
                }
            }
            else if (memcmp(tok->start, "async", 5) == 0) {
                /* The current token is 'async'.
                   Look ahead one token to see if that is 'def'. */

                struct tok_state ahead_tok;
                const char *ahead_tok_start = NULL;
                const char *ahead_tok_end = NULL;
                int ahead_tok_kind;

                memcpy(&ahead_tok, tok, sizeof(ahead_tok));
                ahead_tok_kind = tok_get(&ahead_tok, &ahead_tok_start,
                                         &ahead_tok_end);

                if (ahead_tok_kind == NAME
                    && ahead_tok.cur - ahead_tok.start == 3
                    && memcmp(ahead_tok.start, "def", 3) == 0)
                {
                    /* The next token is going to be 'def', so instead of
                       returning a plain NAME token, return ASYNC. */
                    tok->async_def_indent = tok->indent;
                    tok->async_def = 1;
                    return ASYNC;
                }
            }
        }

        return NAME;
    }

    /* Newline */
    if (c == '\n') {
        tok->atbol = 1;
        if (blankline || tok->level > 0) {
            goto nextline;
        }
        *p_start = tok->start;
        *p_end = tok->cur - 1; /* Leave '\n' out of the string */
        tok->cont_line = 0;
        if (tok->async_def) {
            /* We're somewhere inside an 'async def' function, and
               we've encountered a NEWLINE after its signature. */
            tok->async_def_nl = 1;
        }
        return NEWLINE;
    }

    /* Period or number starting with period? */
    if (c == '.') {
        c = tok_nextc(tok);
        if (isdigit(c)) {
            goto fraction;
        } else if (c == '.') {
            c = tok_nextc(tok);
            if (c == '.') {
                *p_start = tok->start;
                *p_end = tok->cur;
                return ELLIPSIS;
            }
            else {
                tok_backup(tok, c);
            }
            tok_backup(tok, '.');
        }
        else {
            tok_backup(tok, c);
        }
        *p_start = tok->start;
        *p_end = tok->cur;
        return DOT;
    }

    /* Number */
    if (isdigit(c)) {
        if (c == '0') {
            /* Hex, octal or binary -- maybe. */
            c = tok_nextc(tok);
            if (c == 'x' || c == 'X') {
                /* Hex */
                c = tok_nextc(tok);
                do {
                    if (c == '_') {
                        c = tok_nextc(tok);
                    }
                    if (!isxdigit(c)) {
                        tok_backup(tok, c);
                        return syntaxerror(tok, "invalid hexadecimal literal");
                    }
                    do {
                        c = tok_nextc(tok);
                    } while (isxdigit(c));
                } while (c == '_');
            }
            else if (c == 'o' || c == 'O') {
                /* Octal */
                c = tok_nextc(tok);
                do {
                    if (c == '_') {
                        c = tok_nextc(tok);
                    }
                    if (c < '0' || c >= '8') {
                        tok_backup(tok, c);
                        if (isdigit(c)) {
                            return syntaxerror(tok,
                                    "invalid digit '%c' in octal literal", c);
                        }
                        else {
                            return syntaxerror(tok, "invalid octal literal");
                        }
                    }
                    do {
                        c = tok_nextc(tok);
                    } while ('0' <= c && c < '8');
                } while (c == '_');
                if (isdigit(c)) {
                    return syntaxerror(tok,
                            "invalid digit '%c' in octal literal", c);
                }
            }
            else if (c == 'b' || c == 'B') {
                /* Binary */
                c = tok_nextc(tok);
                do {
                    if (c == '_') {
                        c = tok_nextc(tok);
                    }
                    if (c != '0' && c != '1') {
                        tok_backup(tok, c);
                        if (isdigit(c)) {
                            return syntaxerror(tok,
                                    "invalid digit '%c' in binary literal", c);
                        }
                        else {
                            return syntaxerror(tok, "invalid binary literal");
                        }
                    }
                    do {
                        c = tok_nextc(tok);
                    } while (c == '0' || c == '1');
                } while (c == '_');
                if (isdigit(c)) {
                    return syntaxerror(tok,
                            "invalid digit '%c' in binary literal", c);
                }
            }
            else {
                int nonzero = 0;
                /* maybe old-style octal; c is first char of it */
                /* in any case, allow '0' as a literal */
                while (1) {
                    if (c == '_') {
                        c = tok_nextc(tok);
                        if (!isdigit(c)) {
                            tok_backup(tok, c);
                            return syntaxerror(tok, "invalid decimal literal");
                        }
                    }
                    if (c != '0') {
                        break;
                    }
                    c = tok_nextc(tok);
                }
                if (isdigit(c)) {
                    nonzero = 1;
                    c = tok_decimal_tail(tok);
                    if (c == 0) {
                        return ERRORTOKEN;
                    }
                }
                if (c == '.') {
                    c = tok_nextc(tok);
                    goto fraction;
                }
                else if (c == 'e' || c == 'E') {
                    goto exponent;
                }
                else if (c == 'j' || c == 'J') {
                    goto imaginary;
                }
                else if (nonzero) {
                    /* Old-style octal: now disallowed. */
                    tok_backup(tok, c);
                    return syntaxerror(tok,
                                       "leading zeros in decimal integer "
                                       "literals are not permitted; "
                                       "use an 0o prefix for octal integers");
                }
            }
        }
        else {
            /* Decimal */
            c = tok_decimal_tail(tok);
            if (c == 0) {
                return ERRORTOKEN;
            }
            {
                /* Accept floating point numbers. */
                if (c == '.') {
                    c = tok_nextc(tok);
        fraction:
                    /* Fraction */
                    if (isdigit(c)) {
                        c = tok_decimal_tail(tok);
                        if (c == 0) {
                            return ERRORTOKEN;
                        }
                    }
                }
                if (c == 'e' || c == 'E') {
                    int e;
                  exponent:
                    e = c;
                    /* Exponent part */
                    c = tok_nextc(tok);
                    if (c == '+' || c == '-') {
                        c = tok_nextc(tok);
                        if (!isdigit(c)) {
                            tok_backup(tok, c);
                            return syntaxerror(tok, "invalid decimal literal");
                        }
                    } else if (!isdigit(c)) {
                        tok_backup(tok, c);
                        tok_backup(tok, e);
                        *p_start = tok->start;
                        *p_end = tok->cur;
                        return NUMBER;
                    }
                    c = tok_decimal_tail(tok);
                    if (c == 0) {
                        return ERRORTOKEN;
                    }
                }
                if (c == 'j' || c == 'J') {
                    /* Imaginary part */
        imaginary:
                    c = tok_nextc(tok);
                }
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
        int quote = c;
        int quote_size = 1;             /* 1 or 3 */
        int end_quote_size = 0;

        /* Nodes of type STRING, especially multi line strings
           must be handled differently in order to get both
           the starting line number and the column offset right.
           (cf. issue 16806) */
        tok->first_lineno = tok->lineno;
        tok->multi_line_start = tok->line_start;

        /* Find the quote size and start of string */
        c = tok_nextc(tok);
        if (c == quote) {
            c = tok_nextc(tok);
            if (c == quote) {
                quote_size = 3;
            }
            else {
                end_quote_size = 1;     /* empty string found */
            }
        }
        if (c != quote) {
            tok_backup(tok, c);
        }

        /* Get rest of string */
        while (end_quote_size != quote_size) {
            c = tok_nextc(tok);
            if (c == EOF || (quote_size == 1 && c == '\n')) {
                // shift the tok_state's location into
                // the start of string, and report the error
                // from the initial quote character
                tok->cur = (char *)tok->start;
                tok->cur++;
                tok->line_start = tok->multi_line_start;
                int start = tok->lineno;
                tok->lineno = tok->first_lineno;

                if (quote_size == 3) {
                    return syntaxerror(tok,
                                       "unterminated triple-quoted string literal"
                                       " (detected at line %d)", start);
                }
                else {
                    return syntaxerror(tok,
                                       "unterminated string literal (detected at"
                                       " line %d)", start);
                }
            }
            if (c == quote) {
                end_quote_size += 1;
            }
            else {
                end_quote_size = 0;
                if (c == '\\') {
                    tok_nextc(tok);  /* skip escaped char */
                }
            }
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
        c = tok_nextc(tok);
        if (c == EOF) {
            tok->done = E_EOF;
            tok->cur = tok->inp;
            return ERRORTOKEN;
        } else {
            tok_backup(tok, c);
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
            }
            else {
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
        if (tok->level >= MAXLEVEL) {
            return syntaxerror(tok, "too many nested parentheses");
        }
        tok->parenstack[tok->level] = c;
        tok->parenlinenostack[tok->level] = tok->lineno;
        tok->parencolstack[tok->level] = (int)(tok->start - tok->line_start);
        tok->level++;
        break;
    case ')':
    case ']':
    case '}':
        if (!tok->level) {
            return syntaxerror(tok, "unmatched '%c'", c);
        }
        tok->level--;
        int opening = tok->parenstack[tok->level];
        if (!((opening == '(' && c == ')') ||
              (opening == '[' && c == ']') ||
              (opening == '{' && c == '}')))
        {
            if (tok->parenlinenostack[tok->level] != tok->lineno) {
                return syntaxerror(tok,
                        "closing parenthesis '%c' does not match "
                        "opening parenthesis '%c' on line %d",
                        c, opening, tok->parenlinenostack[tok->level]);
            }
            else {
                return syntaxerror(tok,
                        "closing parenthesis '%c' does not match "
                        "opening parenthesis '%c'",
                        c, opening);
            }
        }
        break;
    }

    /* Punctuation character */
    *p_start = tok->start;
    *p_end = tok->cur;
    return PyToken_OneChar(c);
}

int
PyTokenizer_Get(struct tok_state *tok, const char **p_start, const char **p_end)
{
    int result = tok_get(tok, p_start, p_end);
    if (tok->decoding_erred) {
        result = ERRORTOKEN;
        tok->done = E_DECODE;
    }
    return result;
}

/* Get the encoding of a Python file. Check for the coding cookie and check if
   the file starts with a BOM.

   PyTokenizer_FindEncodingFilename() returns NULL when it can't find the
   encoding in the first or second line of the file (in which case the encoding
   should be assumed to be UTF-8).

   The char* returned is malloc'ed via PyMem_Malloc() and thus must be freed
   by the caller. */

char *
PyTokenizer_FindEncodingFilename(int fd, PyObject *filename)
{
    struct tok_state *tok;
    FILE *fp;
    const char *p_start = NULL;
    const char *p_end = NULL;
    char *encoding = NULL;

    fd = _Py_dup(fd);
    if (fd < 0) {
        return NULL;
    }

    fp = fdopen(fd, "r");
    if (fp == NULL) {
        return NULL;
    }
    tok = PyTokenizer_FromFile(fp, NULL, NULL, NULL);
    if (tok == NULL) {
        fclose(fp);
        return NULL;
    }
    if (filename != NULL) {
        Py_INCREF(filename);
        tok->filename = filename;
    }
    else {
        tok->filename = PyUnicode_FromString("<string>");
        if (tok->filename == NULL) {
            fclose(fp);
            PyTokenizer_Free(tok);
            return encoding;
        }
    }
    while (tok->lineno < 2 && tok->done == E_OK) {
        PyTokenizer_Get(tok, &p_start, &p_end);
    }
    fclose(fp);
    if (tok->encoding) {
        encoding = (char *)PyMem_Malloc(strlen(tok->encoding) + 1);
        if (encoding) {
            strcpy(encoding, tok->encoding);
        }
    }
    PyTokenizer_Free(tok);
    return encoding;
}

char *
PyTokenizer_FindEncoding(int fd)
{
    return PyTokenizer_FindEncodingFilename(fd, NULL);
}

#ifdef Py_DEBUG

void
tok_dump(int type, char *start, char *end)
{
    printf("%s", _PyParser_TokenNames[type]);
    if (type == NAME || type == NUMBER || type == STRING || type == OP)
        printf("(%.*s)", (int)(end - start), start);
}

#endif
