#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_fileutils.h"     // _Py_UniversalNewlineFgetsWithSize()
#include "pycore_runtime.h"       // _Py_ID()

#include "errcode.h"              // E_NOMEM

#ifdef HAVE_UNISTD_H
#  include <unistd.h>             // lseek(), read()
#endif

#include "helpers.h"
#include "../lexer/state.h"
#include "../lexer/lexer.h"
#include "../lexer/buffer.h"


static int
tok_concatenate_interactive_new_line(struct tok_state *tok, const char *line) {
    assert(tok->fp_interactive);

    if (!line) {
        return 0;
    }

    Py_ssize_t current_size = tok->interactive_src_end - tok->interactive_src_start;
    Py_ssize_t line_size = strlen(line);
    char last_char = line[line_size > 0 ? line_size - 1 : line_size];
    if (last_char != '\n') {
        line_size += 1;
    }
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
    tok->implicit_newline = 0;
    if (last_char != '\n') {
        /* Last line does not end in \n, fake one */
        new_str[current_size + line_size - 1] = '\n';
        new_str[current_size + line_size] = '\0';
        tok->implicit_newline = 1;
    }
    tok->interactive_src_start = new_str;
    tok->interactive_src_end = new_str + current_size + line_size;
    return 0;
}

static int
tok_readline_raw(struct tok_state *tok)
{
    do {
        if (!_PyLexer_tok_reserve_buf(tok, BUFSIZ)) {
            return 0;
        }
        int n_chars = (int)(tok->end - tok->inp);
        size_t line_size = 0;
        char *line = _Py_UniversalNewlineFgetsWithSize(tok->inp, n_chars, tok->fp, NULL, &line_size);
        if (line == NULL) {
            return 1;
        }
        if (tok->fp_interactive &&
            tok_concatenate_interactive_new_line(tok, line) == -1) {
            return 0;
        }
        tok->inp += line_size;
        if (tok->inp == tok->buf) {
            return 0;
        }
    } while (tok->inp[-1] != '\n');
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
            _PyTokenizer_error_ret(tok);
            goto error;
        }
    }
    else {
        tok->decoding_buffer = NULL;
    }
    buf = PyUnicode_AsUTF8AndSize(line, &buflen);
    if (buf == NULL) {
        _PyTokenizer_error_ret(tok);
        goto error;
    }
    // Make room for the null terminator *and* potentially
    // an extra newline character that we may need to artificially
    // add.
    size_t buffer_size = buflen + 2;
    if (!_PyLexer_tok_reserve_buf(tok, buffer_size)) {
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

/* Fetch the next byte from TOK. */
static int fp_getc(struct tok_state *tok) {
    return getc(tok->fp);
}

/* Unfetch the last byte back into TOK.  */
static void fp_ungetc(int c, struct tok_state *tok) {
    ungetc(c, tok->fp);
}

/* Set the readline function for TOK to a StreamReader's
   readline function. The StreamReader is named ENC.

   This function is called from _PyTokenizer_check_bom and _PyTokenizer_check_coding_spec.

   ENC is usually identical to the future value of tok->encoding,
   except for the (currently unsupported) case of UTF-16.

   Return 1 on success, 0 on failure. */
static int
fp_setreadl(struct tok_state *tok, const char* enc)
{
    PyObject *readline, *open, *stream;
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

    open = PyImport_ImportModuleAttrString("io", "open");
    if (open == NULL) {
        return 0;
    }
    stream = PyObject_CallFunction(open, "isisOOO",
                    fd, "r", -1, enc, Py_None, Py_None, Py_False);
    Py_DECREF(open);
    if (stream == NULL) {
        return 0;
    }

    readline = PyObject_GetAttr(stream, &_Py_ID(readline));
    Py_DECREF(stream);
    if (readline == NULL) {
        return 0;
    }
    Py_XSETREF(tok->decoding_readline, readline);

    if (pos > 0) {
        PyObject *bufobj = _PyObject_CallNoArgs(readline);
        if (bufobj == NULL) {
            return 0;
        }
        Py_DECREF(bufobj);
    }

    return 1;
}

static int
tok_underflow_interactive(struct tok_state *tok) {
    if (tok->interactive_underflow == IUNDERFLOW_STOP) {
        tok->done = E_INTERACT_STOP;
        return 1;
    }
    char *newtok = PyOS_Readline(tok->fp ? tok->fp : stdin, stdout, tok->prompt);
    if (newtok != NULL) {
        char *translated = _PyTokenizer_translate_newlines(newtok, 0, 0, tok);
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
        PyObject *u = _PyTokenizer_translate_into_utf8(newtok, tok->encoding);
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
        _PyLexer_remember_fstring_buffers(tok);
        size_t size = strlen(newtok);
        ADVANCE_LINENO();
        if (!_PyLexer_tok_reserve_buf(tok, size + 1)) {
            PyMem_Free(tok->buf);
            tok->buf = NULL;
            PyMem_Free(newtok);
            return 0;
        }
        memcpy(tok->cur, newtok, size + 1);
        PyMem_Free(newtok);
        tok->inp += size;
        tok->multi_line_start = tok->buf + cur_multi_line_start;
        _PyLexer_restore_fstring_buffers(tok);
    }
    else {
        _PyLexer_remember_fstring_buffers(tok);
        ADVANCE_LINENO();
        PyMem_Free(tok->buf);
        tok->buf = newtok;
        tok->cur = tok->buf;
        tok->line_start = tok->buf;
        tok->inp = strchr(tok->buf, '\0');
        tok->end = tok->inp + 1;
        _PyLexer_restore_fstring_buffers(tok);
    }
    if (tok->done != E_OK) {
        if (tok->prompt != NULL) {
            PySys_WriteStderr("\n");
        }
        return 0;
    }

    if (tok->tok_mode_stack_index && !_PyLexer_update_ftstring_expr(tok, 0)) {
        return 0;
    }
    return 1;
}

static int
tok_underflow_file(struct tok_state *tok) {
    if (tok->start == NULL && !INSIDE_FSTRING(tok)) {
        tok->cur = tok->inp = tok->buf;
    }
    if (tok->decoding_state == STATE_INIT) {
        /* We have not yet determined the encoding.
           If an encoding is found, use the file-pointer
           reader functions from now on. */
        if (!_PyTokenizer_check_bom(fp_getc, fp_ungetc, fp_setreadl, tok)) {
            _PyTokenizer_error_ret(tok);
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
    tok->implicit_newline = 0;
    if (tok->inp[-1] != '\n') {
        assert(tok->inp + 1 < tok->end);
        /* Last line does not end in \n, fake one */
        *tok->inp++ = '\n';
        *tok->inp = '\0';
        tok->implicit_newline = 1;
    }

    if (tok->tok_mode_stack_index && !_PyLexer_update_ftstring_expr(tok, 0)) {
        return 0;
    }

    ADVANCE_LINENO();
    if (tok->decoding_state != STATE_NORMAL) {
        if (tok->lineno > 2) {
            tok->decoding_state = STATE_NORMAL;
        }
        else if (!_PyTokenizer_check_coding_spec(tok->cur, strlen(tok->cur),
                                    tok, fp_setreadl))
        {
            return 0;
        }
    }
    /* The default encoding is UTF-8, so make sure we don't have any
       non-UTF-8 sequences in it. */
    if (!tok->encoding && !_PyTokenizer_ensure_utf8(tok->cur, tok)) {
        _PyTokenizer_error_ret(tok);
        return 0;
    }
    assert(tok->done == E_OK);
    return tok->done == E_OK;
}

/* Set up tokenizer for file */
struct tok_state *
_PyTokenizer_FromFile(FILE *fp, const char* enc,
                      const char *ps1, const char *ps2)
{
    struct tok_state *tok = _PyTokenizer_tok_new();
    if (tok == NULL)
        return NULL;
    if ((tok->buf = (char *)PyMem_Malloc(BUFSIZ)) == NULL) {
        _PyTokenizer_Free(tok);
        return NULL;
    }
    tok->cur = tok->inp = tok->buf;
    tok->end = tok->buf + BUFSIZ;
    tok->fp = fp;
    tok->prompt = ps1;
    tok->nextprompt = ps2;
    if (ps1 || ps2) {
        tok->underflow = &tok_underflow_interactive;
    } else {
        tok->underflow = &tok_underflow_file;
    }
    if (enc != NULL) {
        /* Must copy encoding declaration since it
           gets copied into the parse tree. */
        tok->encoding = _PyTokenizer_new_string(enc, strlen(enc), tok);
        if (!tok->encoding) {
            _PyTokenizer_Free(tok);
            return NULL;
        }
        tok->decoding_state = STATE_NORMAL;
    }
    return tok;
}

#if defined(__wasi__) || (defined(__EMSCRIPTEN__) && (__EMSCRIPTEN_major__ >= 3))
// fdopen() with borrowed fd. WASI does not provide dup() and Emscripten's
// dup() emulation with open() is slow.
typedef union {
    void *cookie;
    int fd;
} borrowed;

static ssize_t
borrow_read(void *cookie, char *buf, size_t size)
{
    borrowed b = {.cookie = cookie};
    return read(b.fd, (void *)buf, size);
}

static FILE *
fdopen_borrow(int fd) {
    // supports only reading. seek fails. close and write are no-ops.
    cookie_io_functions_t io_cb = {borrow_read, NULL, NULL, NULL};
    borrowed b = {.fd = fd};
    return fopencookie(b.cookie, "r", io_cb);
}
#else
static FILE *
fdopen_borrow(int fd) {
    fd = _Py_dup(fd);
    if (fd < 0) {
        return NULL;
    }
    return fdopen(fd, "r");
}
#endif

/* Get the encoding of a Python file. Check for the coding cookie and check if
   the file starts with a BOM.

   _PyTokenizer_FindEncodingFilename() returns NULL when it can't find the
   encoding in the first or second line of the file (in which case the encoding
   should be assumed to be UTF-8).

   The char* returned is malloc'ed via PyMem_Malloc() and thus must be freed
   by the caller. */
char *
_PyTokenizer_FindEncodingFilename(int fd, PyObject *filename)
{
    struct tok_state *tok;
    FILE *fp;
    char *encoding = NULL;

    fp = fdopen_borrow(fd);
    if (fp == NULL) {
        return NULL;
    }
    tok = _PyTokenizer_FromFile(fp, NULL, NULL, NULL);
    if (tok == NULL) {
        fclose(fp);
        return NULL;
    }
    if (filename != NULL) {
        tok->filename = Py_NewRef(filename);
    }
    else {
        tok->filename = PyUnicode_FromString("<string>");
        if (tok->filename == NULL) {
            fclose(fp);
            _PyTokenizer_Free(tok);
            return encoding;
        }
    }
    struct token token;
    // We don't want to report warnings here because it could cause infinite recursion
    // if fetching the encoding shows a warning.
    tok->report_warnings = 0;
    while (tok->lineno < 2 && tok->done == E_OK) {
        _PyToken_Init(&token);
        _PyTokenizer_Get(tok, &token);
        _PyToken_Free(&token);
    }
    fclose(fp);
    if (tok->encoding) {
        encoding = (char *)PyMem_Malloc(strlen(tok->encoding) + 1);
        if (encoding) {
            strcpy(encoding, tok->encoding);
        }
    }
    _PyTokenizer_Free(tok);
    return encoding;
}
