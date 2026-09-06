#include "Python.h"
#include "pycore_fileutils.h"
#include "pycore_pystate.h"

#include "errcode.h"
#include "helpers.h"
#include "reader.h"
#include "reader_internal.h"
#include "../lexer/state.h"

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

static inline int
reader_is_streaming(_PyTok_ReaderKind kind)
{
    return kind == _PYTOK_READER_FILE || kind == _PYTOK_READER_READLINE;
}

void
_PyTok_ReaderFree(struct tok_state *tok)
{
    _PyTok_Reader *reader = tok->reader;
    if (reader == NULL) {
        return;
    }
    Py_XDECREF(reader->readline);
    Py_XDECREF(reader->decoder);
    for (int i = 0;
         i < (int)Py_ARRAY_LENGTH(reader->prefetched_lines); i++) {
        _PyTok_ChunkClear(&reader->prefetched_lines[i]);
    }
    PyMem_Free(reader->file_buffer);
    PyMem_Free(reader->decoded);
    PyMem_Free(reader);
    tok->reader = NULL;
}

static int
reserve_buffer(char **buffer, Py_ssize_t *capacity, Py_ssize_t needed)
{
    if (needed <= *capacity) {
        return 0;
    }
    Py_ssize_t cap = *capacity > 0 ? *capacity : BUFSIZ;
    while (cap < needed) {
        if (cap > PY_SSIZE_T_MAX / 2) {
            cap = needed;
            break;
        }
        cap *= 2;
    }
    char *resized = PyMem_Realloc(*buffer, cap);
    if (resized == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    *buffer = resized;
    *capacity = cap;
    return 0;
}

static int
append_decoded(_PyTok_Reader *reader, const char *data, Py_ssize_t len)
{
    if (reader->decoded_pos > 0) {
        Py_ssize_t remaining = reader->decoded_len - reader->decoded_pos;
        memmove(reader->decoded, reader->decoded + reader->decoded_pos,
                (size_t)remaining);
        reader->decoded_pos = 0;
        reader->decoded_len = remaining;
    }
    if (len < 0 || reader->decoded_len > PY_SSIZE_T_MAX - len - 1 ||
            reserve_buffer(&reader->decoded, &reader->decoded_cap,
                           reader->decoded_len + len + 1) < 0) {
        PyErr_NoMemory();
        return -1;
    }
    memcpy(reader->decoded + reader->decoded_len, data, (size_t)len);
    reader->decoded_len += len;
    reader->decoded[reader->decoded_len] = '\0';
    return 0;
}

static int
append_implicit_newline(_PyTok_Reader *reader)
{
    if (reader->decoded_pos == reader->decoded_len ||
            reader->decoded[reader->decoded_len - 1] == '\n') {
        return 0;
    }
    if (append_decoded(reader, "\n", 1) < 0) {
        return -1;
    }
    reader->decoded_tail_is_implicit = 1;
    return 0;
}

static int
pop_decoded_line(_PyTok_Reader *reader, _PyTok_Chunk *chunk)
{
    assert(reader->decoded_pos >= 0 && reader->decoded_pos <= reader->decoded_len);
    if (reader->decoded_pos == reader->decoded_len) {
        return 0;
    }
    char *start = reader->decoded + reader->decoded_pos;
    char *newline = memchr(start, '\n',
                           reader->decoded_len - reader->decoded_pos);
    if (newline == NULL) {
        return 0;
    }
    Py_ssize_t len = newline - start + 1;
    chunk->data = start;
    chunk->len = len;
    chunk->ownership = _PYTOK_CHUNK_BORROWED;
    reader->decoded_pos += len;
    chunk->implicit_newline = reader->decoded_pos == reader->decoded_len &&
        reader->decoded_tail_is_implicit;
    if (reader->decoded_pos == reader->decoded_len) {
        reader->decoded_pos = reader->decoded_len = 0;
        reader->decoded_tail_is_implicit = 0;
    }
    return 1;
}

static int
chunk_is_line(const _PyTok_Chunk *chunk)
{
    if (chunk->len == 0 || chunk->data[chunk->len - 1] != '\n') {
        return 0;
    }
    return memchr(chunk->data, '\n', chunk->len - 1) == NULL;
}

static _PyTok_ReadResult
next_prepared(struct tok_state *tok, _PyTok_Chunk *chunk)
{
    if (tok->lineno >= tok->source.nlines) {
        return _PYTOK_READ_EOF;
    }
    int lineno = tok->lineno + 1;
    const char *start = _PyLexer_BufferPointer(tok, tok->inp);
    const char *newline = memchr(
        start, '\n', tok->source.bytes + tok->source.len - start);
    _PyTok_Off end = newline != NULL
        ? newline - tok->source.bytes + 1 : tok->source.len;
    chunk->data = (char *)start;
    chunk->len = tok->source.bytes + end - start;
    chunk->ownership = _PYTOK_CHUNK_BORROWED;
    chunk->implicit_newline = _PyTok_SourceLineIsImplicit(
        &tok->source, lineno);
    return _PYTOK_READ_LINE;
}

static _PyTok_ReadResult
read_file_line(struct tok_state *tok, _PyTok_Chunk *chunk)
{
    _PyTok_Reader *reader = tok->reader;
    Py_ssize_t len = 0;
    for (;;) {
        if (len > PY_SSIZE_T_MAX - BUFSIZ ||
                reserve_buffer(&reader->file_buffer, &reader->file_buffer_cap,
                               len + BUFSIZ) < 0) {
            return _PYTOK_READ_ERROR;
        }
        int available = (int)Py_MIN(reader->file_buffer_cap - len, INT_MAX);
        size_t read = 0;
        char *result = _Py_UniversalNewlineFgetsWithSize(
            reader->file_buffer + len, available, tok->fp, NULL, &read);
        if (result == NULL) {
            if (len == 0) {
                return _PYTOK_READ_EOF;
            }
            break;
        }
        len += (Py_ssize_t)read;
        if (len > 0 && reader->file_buffer[len - 1] == '\n') {
            break;
        }
    }
    int implicit = len == 0 || reader->file_buffer[len - 1] != '\n';
    chunk->data = reader->file_buffer;
    chunk->len = len;
    chunk->implicit_newline = implicit;
    chunk->ownership = _PYTOK_CHUNK_BORROWED;
    return _PYTOK_READ_LINE;
}

static int
initialize_file(struct tok_state *tok)
{
    _PyTok_Reader *reader = tok->reader;
    reader->file_initialized = 1;
    if (tok->encoding != NULL) {
        return _PyTok_StartDecoder(tok, "strict");
    }

    _PyTok_ReadResult result = read_file_line(
        tok, &reader->prefetched_lines[0]);
    if (result == _PYTOK_READ_EOF) {
        reader->file_eof = 1;
        return 0;
    }
    if (result != _PYTOK_READ_LINE) {
        return -1;
    }
    Py_ssize_t bom_len;
    _PyTok_EncodingResult detection = _PyTok_DetectEncoding(
        tok, &reader->prefetched_lines[0], NULL, 0, &bom_len);
    if (detection == _PYTOK_ENCODING_ERROR) {
        return -1;
    }
    if (detection == _PYTOK_ENCODING_NEED_SECOND_LINE) {
        char *first = _PyTok_CopyBytes(
            reader->prefetched_lines[0].data,
            reader->prefetched_lines[0].len);
        if (first == NULL) {
            tok->done = E_NOMEM;
            return -1;
        }
        reader->prefetched_lines[0].data = first;
        reader->prefetched_lines[0].ownership = _PYTOK_CHUNK_PYMEM;
        result = read_file_line(tok, &reader->prefetched_lines[1]);
        if (result == _PYTOK_READ_EOF) {
            reader->file_eof = 1;
        }
        else if (result != _PYTOK_READ_LINE) {
            return -1;
        }
        _PyTok_Chunk *second = reader->prefetched_lines[1].data != NULL
            ? &reader->prefetched_lines[1] : NULL;
        detection = _PyTok_DetectEncoding(
            tok, &reader->prefetched_lines[0], second, 1, &bom_len);
        if (detection == _PYTOK_ENCODING_ERROR) {
            return -1;
        }
    }
    if (bom_len != 0) {
        _PyTok_Chunk *first = &reader->prefetched_lines[0];
        if (first->ownership == _PYTOK_CHUNK_PYMEM) {
            memmove(first->data, first->data + bom_len,
                    (size_t)(first->len - bom_len));
            first->data[first->len - bom_len] = '\0';
        }
        else {
            first->data += bom_len;
        }
        first->len -= bom_len;
    }
    if (_PyTok_StartDecoder(tok, "strict") < 0) {
        return -1;
    }
    return 0;
}

static int
finalize_decoding(struct tok_state *tok)
{
    _PyTok_Reader *reader = tok->reader;
    if (reader->decoder_finalized) {
        return 0;
    }
    reader->decoder_finalized = 1;
    if (reader->decoder != NULL) {
        _PyTok_Chunk input = {
            .data = "",
            .ownership = _PYTOK_CHUNK_BORROWED,
        };
        int decoded = _PyTok_DecodeChunk(tok, &input, 1);
        if (decoded == 0 &&
                append_decoded(reader, input.data, input.len) < 0) {
            tok->done = E_NOMEM;
            decoded = -1;
        }
        _PyTok_ChunkClear(&input);
        if (decoded < 0) {
            return -1;
        }
    }
    if (reader->decoded_pos < reader->decoded_len &&
            append_implicit_newline(reader) < 0) {
        tok->done = E_NOMEM;
        return -1;
    }
    return 0;
}

static _PyTok_ReadResult
next_file(struct tok_state *tok, _PyTok_Chunk *chunk)
{
    _PyTok_Reader *reader = tok->reader;
    if (!reader->file_initialized && initialize_file(tok) < 0) {
        return _PYTOK_READ_ERROR;
    }
    for (;;) {
        if (pop_decoded_line(reader, chunk)) {
            return _PYTOK_READ_LINE;
        }
        _PyTok_Chunk input = {0};
        if (reader->prefetched_lines[0].data != NULL) {
            input = reader->prefetched_lines[0];
            reader->prefetched_lines[0] = (_PyTok_Chunk){0};
        }
        else if (reader->prefetched_lines[1].data != NULL) {
            input = reader->prefetched_lines[1];
            reader->prefetched_lines[1] = (_PyTok_Chunk){0};
        }
        else if (!reader->file_eof) {
            _PyTok_ReadResult result = read_file_line(tok, &input);
            if (result == _PYTOK_READ_ERROR) {
                return result;
            }
            if (result == _PYTOK_READ_EOF) {
                reader->file_eof = 1;
            }
        }
        if (input.data != NULL) {
            int implicit = input.implicit_newline;
            if (reader->decoder == NULL && !implicit) {
                *chunk = input;
                return _PYTOK_READ_LINE;
            }
            int decoded = _PyTok_DecodeChunk(tok, &input, 0);
            if (decoded == 0 && chunk_is_line(&input)) {
                *chunk = input;
                return _PYTOK_READ_LINE;
            }
            if (decoded == 0 &&
                    append_decoded(reader, input.data, input.len) < 0) {
                tok->done = E_NOMEM;
                decoded = -1;
            }
            if (decoded == 0 && implicit) {
                reader->decoded_tail_is_implicit = 1;
            }
            _PyTok_ChunkClear(&input);
            if (decoded < 0) {
                return _PYTOK_READ_ERROR;
            }
            continue;
        }
        if (!reader->decoder_finalized) {
            if (finalize_decoding(tok) < 0) {
                return _PYTOK_READ_ERROR;
            }
            continue;
        }
        return _PYTOK_READ_EOF;
    }
}

static _PyTok_ReadResult
next_readline(struct tok_state *tok, _PyTok_Chunk *chunk)
{
    _PyTok_Reader *reader = tok->reader;
    for (;;) {
        if (pop_decoded_line(reader, chunk)) {
            return _PYTOK_READ_LINE;
        }
        if (reader->decoder_finalized) {
            return _PYTOK_READ_EOF;
        }

        PyObject *raw = PyObject_CallNoArgs(reader->readline);
        if (raw == NULL) {
            if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
                PyErr_Clear();
                if (finalize_decoding(tok) < 0) {
                    return _PYTOK_READ_ERROR;
                }
                continue;
            }
            return _PYTOK_READ_ERROR;
        }

        _PyTok_Chunk input = {0};
        if (tok->encoding != NULL) {
            if (!PyBytes_Check(raw)) {
                PyErr_SetString(PyExc_TypeError,
                                "readline() returned a non-bytes object");
                Py_DECREF(raw);
                return _PYTOK_READ_ERROR;
            }
            if (PyBytes_GET_SIZE(raw) == 0) {
                Py_DECREF(raw);
                if (_PyTok_StartDecoder(tok, "replace") < 0) {
                    return _PYTOK_READ_ERROR;
                }
                if (finalize_decoding(tok) < 0) {
                    return _PYTOK_READ_ERROR;
                }
                continue;
            }
            input.owner = raw;
            input.data = PyBytes_AS_STRING(raw);
            input.len = PyBytes_GET_SIZE(raw);
            input.ownership = _PYTOK_CHUNK_PYOBJECT;
            int decoded;
            if (reader->decoder == NULL &&
                    strcmp(tok->encoding, "utf-8") == 0 &&
                    chunk_is_line(&input)) {
                decoded = _PyTok_DecodeOnce(
                    tok, &input, "utf-8", "replace");
            }
            else {
                decoded = _PyTok_StartDecoder(tok, "replace");
                if (decoded == 0) {
                    decoded = _PyTok_DecodeChunk(tok, &input, 0);
                }
            }
            if (decoded < 0) {
                _PyTok_ChunkClear(&input);
                return _PYTOK_READ_ERROR;
            }
        }
        else {
            if (!PyUnicode_Check(raw)) {
                PyErr_SetString(PyExc_TypeError,
                                "readline() returned a non-string object");
                Py_DECREF(raw);
                return _PYTOK_READ_ERROR;
            }
            Py_ssize_t utf8_len;
            const char *utf8 = PyUnicode_AsUTF8AndSize(raw, &utf8_len);
            if (utf8 == NULL) {
                Py_DECREF(raw);
                return _PYTOK_READ_ERROR;
            }
            input.owner = raw;
            input.data = (char *)utf8;
            input.len = utf8_len;
            input.ownership = _PYTOK_CHUNK_PYOBJECT;
            if (input.len == 0) {
                _PyTok_ChunkClear(&input);
                if (finalize_decoding(tok) < 0) {
                    return _PYTOK_READ_ERROR;
                }
                continue;
            }
        }

        if (reader->decoded_pos == reader->decoded_len &&
                chunk_is_line(&input)) {
            *chunk = input;
            return _PYTOK_READ_LINE;
        }

        if (append_decoded(reader, input.data, input.len) < 0) {
            _PyTok_ChunkClear(&input);
            tok->done = E_NOMEM;
            return _PYTOK_READ_ERROR;
        }
        _PyTok_ChunkClear(&input);
        if (reader->decoded_pos < reader->decoded_len &&
                reader->decoded[reader->decoded_len - 1] != '\n') {
            int pending = _PyTok_DecoderHasBufferedInput(tok);
            if (pending < 0) {
                return _PYTOK_READ_ERROR;
            }
            if (!pending && append_implicit_newline(reader) < 0) {
                tok->done = E_NOMEM;
                return _PYTOK_READ_ERROR;
            }
        }
        if (pop_decoded_line(reader, chunk)) {
            return _PYTOK_READ_LINE;
        }
    }
}

static _PyTok_ReadResult
next_interactive(struct tok_state *tok, _PyTok_Chunk *chunk)
{
    _PyTok_Reader *reader = tok->reader;
    if (reader->stop_interactive) {
        return _PYTOK_READ_STOPPED;
    }
    char *input = PyOS_Readline(
        tok->fp != NULL ? tok->fp : stdin, stdout, reader->prompt);
    if (reader->nextprompt != NULL) {
        reader->prompt = reader->nextprompt;
    }
    if (input == NULL) {
        return _PYTOK_READ_INTERRUPT;
    }
    Py_ssize_t len = strlen(input);
    if (len == 0) {
        PyMem_Free(input);
        return _PYTOK_READ_EOF;
    }
    _PyTok_Chunk decoded = {
        .data = input,
        .len = len,
        .ownership = _PYTOK_CHUNK_PYMEM,
    };
    if (tok->encoding != NULL &&
            _PyTok_DecodeOnce(
                tok, &decoded, tok->encoding, NULL) < 0) {
        _PyTok_ChunkClear(&decoded);
        return _PYTOK_READ_ERROR;
    }
    chunk->data = _PyTok_NormalizeNewlines(
        decoded.data, decoded.len, 0, 0,
        &chunk->len, NULL);
    _PyTok_ChunkClear(&decoded);
    if (chunk->data == NULL) {
        PyErr_NoMemory();
        tok->done = E_NOMEM;
        return _PYTOK_READ_ERROR;
    }
    chunk->ownership = _PYTOK_CHUNK_PYMEM;
    return _PYTOK_READ_LINE;
}

int
_PyTok_ReaderIsInteractive(const struct tok_state *tok)
{
    return tok->reader->kind == _PYTOK_READER_INTERACTIVE;
}

void
_PyTok_ReaderStopInteractive(struct tok_state *tok)
{
    if (_PyTok_ReaderIsInteractive(tok)) {
        tok->reader->stop_interactive = 1;
    }
}

static _PyTok_ReadResult
reader_next(struct tok_state *tok, _PyTok_Chunk *chunk)
{
    *chunk = (_PyTok_Chunk){0};
    switch (tok->reader->kind) {
        case _PYTOK_READER_PREPARED:
            return next_prepared(tok, chunk);
        case _PYTOK_READER_FILE:
            return next_file(tok, chunk);
        case _PYTOK_READER_READLINE:
            return next_readline(tok, chunk);
        case _PYTOK_READER_INTERACTIVE:
            return next_interactive(tok, chunk);
    }
    Py_UNREACHABLE();
}

static void
reset_streaming_buffer(struct tok_state *tok)
{
    _PyTok_SourceDiscard(&tok->source);
    tok->buf_offset = tok->source.base_offset;
    tok->cur = tok->inp = tok->line_start = tok->buf_offset;
}

int
_PyTok_ReaderUnderflow(struct tok_state *tok)
{
    assert(tok->cur >= tok->buf_offset && tok->cur <= tok->inp);
    _PyTok_ReaderKind kind = tok->reader->kind;
    int prepared = kind == _PYTOK_READER_PREPARED;
    int streaming = reader_is_streaming(kind);
    int reset_buffer = !prepared && tok->start < 0 &&
        _PyLexer_CurrentFTString(tok) == NULL;

    _PyTok_Chunk chunk;
    _PyTok_ReadResult result = reader_next(tok, &chunk);
    if (result != _PYTOK_READ_LINE) {
        if (reset_buffer && streaming) {
            reset_streaming_buffer(tok);
        }
        if (result == _PYTOK_READ_EOF) {
            tok->done = E_EOF;
        }
        else if (result == _PYTOK_READ_STOPPED) {
            tok->done = E_INTERACT_STOP;
        }
        else if (result == _PYTOK_READ_INTERRUPT) {
            tok->done = E_INTR;
        }
        else {
            if (tok->done == E_OK) {
                tok->done = PyErr_ExceptionMatches(PyExc_MemoryError)
                    ? E_NOMEM : E_ERROR;
            }
        }
        if (kind == _PYTOK_READER_INTERACTIVE &&
                result != _PYTOK_READ_STOPPED) {
            PySys_WriteStderr("\n");
        }
        return 0;
    }
    assert(chunk.data != NULL && chunk.len > 0);
    if (tok->lineno == INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "too many tokenizer source lines");
        tok->done = E_ERROR;
        _PyTok_ChunkClear(&chunk);
        return 0;
    }

    Py_ssize_t scan_len = chunk.len;
    if (kind == _PYTOK_READER_INTERACTIVE &&
            chunk.implicit_newline) {
        scan_len--;
    }
    if (!prepared) {
        if (streaming && reset_buffer) {
            reset_streaming_buffer(tok);
        }
        _PyTok_Off source_start = _PyTok_SourceAppendLine(
            &tok->source, chunk.data, chunk.len,
            chunk.implicit_newline);
        if (source_start < 0) {
            _PyTok_ChunkClear(&chunk);
            tok->done = PyErr_ExceptionMatches(PyExc_MemoryError)
                ? E_NOMEM : E_ERROR;
            return 0;
        }
        if (reset_buffer) {
            tok->cur = source_start;
            tok->buf_offset = source_start;
            tok->line_start = tok->buf_offset;
            tok->start = -1;
        }
        tok->inp = source_start + scan_len;
    }
    if (prepared) {
        if (tok->start < 0 && _PyLexer_CurrentFTString(tok) == NULL) {
            tok->buf_offset = tok->cur;
        }
        tok->inp = _PyLexer_BufferOffset(tok, chunk.data) + chunk.len;
    }
    tok->implicit_newline = chunk.implicit_newline;

    tok->lineno++;
    if (kind == _PYTOK_READER_FILE &&
            (tok->encoding == NULL || strcmp(tok->encoding, "utf-8") == 0) &&
            !_PyTokenizer_ensure_utf8(_PyLexer_BufferPointer(tok, tok->cur), tok, tok->lineno)) {
        _PyTok_ChunkClear(&chunk);
        return 0;
    }
    _PyTok_ChunkClear(&chunk);
    return 1;
}

static struct tok_state *
tokenizer_new_with_reader(_PyTok_ReaderKind kind)
{
    struct tok_state *tok = PyMem_Calloc(1, sizeof(*tok));
    if (tok == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    tok->done = E_OK;
    tok->start = -1;
    tok->atbol = 1;
    tok->start_loc = (_PyTok_Loc){-1, -1};
    tok->ftstring_stack = tok->ftstring_stack_inline;
    tok->ftstring_capacity = FTSTRING_STACK_INLINE_CAPACITY;
#ifdef Py_DEBUG
    tok->debug = _Py_GetConfig()->parser_debug;
#endif
    tok->reader = PyMem_Calloc(1, sizeof(*tok->reader));
    if (tok->reader == NULL) {
        PyErr_NoMemory();
        _PyTokenizer_Free(tok);
        return NULL;
    }
    tok->reader->kind = kind;
    return tok;
}

static struct tok_state *
tokenizer_from_string(const char *input, int utf8_only, int exec_input,
                      int preserve_crlf)
{
    struct tok_state *tok = tokenizer_new_with_reader(_PYTOK_READER_PREPARED);
    if (tok == NULL) {
        return NULL;
    }
    if (_PyTok_PrepareString(
            tok, input, utf8_only, exec_input, preserve_crlf) < 0) {
        _PyTokenizer_Free(tok);
        return NULL;
    }
    return tok;
}

struct tok_state *
_PyTokenizer_FromString(const char *input, int exec_input, int preserve_crlf)
{
    return tokenizer_from_string(input, 0, exec_input, preserve_crlf);
}

struct tok_state *
_PyTokenizer_FromUTF8(const char *input, int exec_input, int preserve_crlf)
{
    return tokenizer_from_string(input, 1, exec_input, preserve_crlf);
}

struct tok_state *
_PyTokenizer_FromReadline(PyObject *readline, const char *encoding)
{
    struct tok_state *tok = tokenizer_new_with_reader(_PYTOK_READER_READLINE);
    if (tok == NULL) {
        return NULL;
    }
    if (encoding != NULL && _PyTok_SetEncoding(tok, encoding) < 0) {
        _PyTokenizer_Free(tok);
        return NULL;
    }
    tok->reader->readline = Py_NewRef(readline);
    return tok;
}

struct tok_state *
_PyTokenizer_FromFile(FILE *fp, const char *encoding,
                      const char *ps1, const char *ps2)
{
    _PyTok_ReaderKind kind = ps1 != NULL || ps2 != NULL
        ? _PYTOK_READER_INTERACTIVE : _PYTOK_READER_FILE;
    struct tok_state *tok = tokenizer_new_with_reader(kind);
    if (tok == NULL) {
        return NULL;
    }
    if (encoding != NULL && _PyTok_SetEncoding(tok, encoding) < 0) {
        _PyTokenizer_Free(tok);
        return NULL;
    }
    tok->fp = fp;
    tok->reader->prompt = ps1;
    tok->reader->nextprompt = ps2;
    return tok;
}

#if defined(__wasi__) || (defined(__EMSCRIPTEN__) && (__EMSCRIPTEN_major__ >= 3))
/* WASI has no dup(), and Emscripten's emulation is slow. */
typedef union {
    void *cookie;
    int fd;
} borrowed_fd;

static ssize_t
borrow_read(void *cookie, char *buffer, size_t size)
{
    borrowed_fd borrowed = {.cookie = cookie};
    return read(borrowed.fd, buffer, size);
}

static FILE *
fdopen_borrow(int fd)
{
    cookie_io_functions_t callbacks = {borrow_read, NULL, NULL, NULL};
    borrowed_fd borrowed = {.fd = fd};
    return fopencookie(borrowed.cookie, "r", callbacks);
}
#else
static FILE *
fdopen_borrow(int fd)
{
    int copy = _Py_dup(fd);
    return copy < 0 ? NULL : fdopen(copy, "r");
}
#endif

char *
_PyTokenizer_FindEncodingFilename(int fd, PyObject *filename)
{
    FILE *fp = fdopen_borrow(fd);
    if (fp == NULL) {
        return NULL;
    }
    struct tok_state *tok = _PyTokenizer_FromFile(fp, NULL, NULL, NULL);
    if (tok == NULL) {
        fclose(fp);
        return NULL;
    }
    tok->filename = filename != NULL
        ? Py_NewRef(filename) : PyUnicode_FromString("<string>");
    if (tok->filename == NULL) {
        fclose(fp);
        _PyTokenizer_Free(tok);
        return NULL;
    }
    if (initialize_file(tok) < 0) {
        fclose(fp);
        _PyTokenizer_Free(tok);
        return NULL;
    }
    fclose(fp);
    char *encoding = tok->encoding == NULL
        ? NULL : _PyTok_CopyBytes(tok->encoding, strlen(tok->encoding));
    _PyTokenizer_Free(tok);
    return encoding;
}
