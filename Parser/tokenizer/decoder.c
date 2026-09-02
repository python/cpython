#include "Python.h"
#include "pycore_codecs.h"
#include "pycore_global_strings.h"
#include "pycore_runtime.h"
#include "errcode.h"

#include "reader_internal.h"
#include "helpers.h"
#include "../lexer/state.h"

char *
_PyTok_CopyBytes(const char *data, Py_ssize_t len)
{
    if (len < 0 || len == PY_SSIZE_T_MAX) {
        PyErr_NoMemory();
        return NULL;
    }
    char *copy = PyMem_Malloc((size_t)len + 1);
    if (copy == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memcpy(copy, data, (size_t)len);
    copy[len] = '\0';
    return copy;
}

static void
chunk_release_data(_PyTok_Chunk *chunk)
{
    switch (chunk->ownership) {
        case _PYTOK_CHUNK_BORROWED:
            break;
        case _PYTOK_CHUNK_PYMEM:
            PyMem_Free(chunk->data);
            break;
        case _PYTOK_CHUNK_PYOBJECT:
            Py_DECREF(chunk->owner);
            break;
    }
}

void
_PyTok_ChunkClear(_PyTok_Chunk *chunk)
{
    chunk_release_data(chunk);
    *chunk = (_PyTok_Chunk){0};
}

static int
chunk_set_unicode(struct tok_state *tok, _PyTok_Chunk *chunk,
                  PyObject *unicode, int strip_bom)
{
    Py_ssize_t utf8_len;
    const char *utf8 = PyUnicode_AsUTF8AndSize(unicode, &utf8_len);
    if (utf8 == NULL) {
        Py_DECREF(unicode);
        tok->done = PyErr_ExceptionMatches(PyExc_MemoryError)
            ? E_NOMEM : E_DECODE;
        return -1;
    }
    if (strip_bom && PyUnicode_GET_LENGTH(unicode) > 0 &&
            PyUnicode_ReadChar(unicode, 0) == 0xFEFF) {
        utf8 += 3;
        utf8_len -= 3;
    }
    chunk_release_data(chunk);
    chunk->owner = unicode;
    chunk->data = (char *)utf8;
    chunk->len = utf8_len;
    chunk->ownership = _PYTOK_CHUNK_PYOBJECT;
    return 0;
}

char *
_PyTok_NormalizeNewlines(const char *data, Py_ssize_t len, int preserve_crlf,
                         int add_final_newline, Py_ssize_t *out_len,
                         int *implicit_newline)
{
    if (len > PY_SSIZE_T_MAX - 2) {
        PyErr_NoMemory();
        return NULL;
    }
    char *result = PyMem_Malloc((size_t)len + 2);
    if (result == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    Py_ssize_t write = 0;
    for (Py_ssize_t read = 0; read < len; read++) {
        char c = data[read];
        if (!preserve_crlf && c == '\r') {
            if (read + 1 < len && data[read + 1] == '\n') {
                read++;
            }
            c = '\n';
        }
        result[write++] = c;
    }
    int implicit = add_final_newline && write > 0 && result[write - 1] != '\n';
    if (implicit) {
        result[write++] = '\n';
    }
    result[write] = '\0';
    *out_len = write;
    *implicit_newline = implicit;
    return result;
}

int
_PyTok_SetEncoding(struct tok_state *tok, const char *encoding)
{
    char *copy = _PyTok_CopyBytes(encoding, strlen(encoding));
    if (copy == NULL) {
        tok->done = E_NOMEM;
        return -1;
    }
    PyMem_Free(tok->encoding);
    tok->encoding = copy;
    return 0;
}

static int
find_cookie(const char *line, Py_ssize_t len, char **encoding, int *scan_next)
{
    Py_ssize_t i = 0;
    *encoding = NULL;
    *scan_next = 1;
    for (; i < len; i++) {
        if (line[i] == '#') {
            break;
        }
        if (line[i] == '\n' || line[i] == '\r') {
            return 0;
        }
        if (line[i] != ' ' && line[i] != '\t' && line[i] != '\f') {
            *scan_next = 0;
            return 0;
        }
    }
    for (; i + 6 < len; i++) {
        if (memcmp(line + i, "coding", 6) != 0) {
            continue;
        }
        const char *cursor = line + i + 6;
        if (*cursor != ':' && *cursor != '=') {
            continue;
        }
        do {
            cursor++;
        } while (cursor < line + len &&
                 (*cursor == ' ' || *cursor == '\t'));
        const char *start = cursor;
        while (cursor < line + len &&
                (Py_ISALNUM(*cursor) || *cursor == '-' ||
                 *cursor == '_' || *cursor == '.')) {
            cursor++;
        }
        if (cursor == start) {
            continue;
        }
        char *found = _PyTok_CopyBytes(start, cursor - start);
        if (found == NULL) {
            return -1;
        }
        char normalized[13];
        int n;
        for (n = 0; n < 12 && found[n] != '\0'; n++) {
            normalized[n] = found[n] == '_' ? '-' : Py_TOLOWER(found[n]);
        }
        normalized[n] = '\0';
        const char *canonical = found;
        if (strcmp(normalized, "utf-8") == 0 ||
                strncmp(normalized, "utf-8-", 6) == 0) {
            canonical = "utf-8";
        }
        else if (strcmp(normalized, "latin-1") == 0 ||
                 strcmp(normalized, "iso-8859-1") == 0 ||
                 strcmp(normalized, "iso-latin-1") == 0 ||
                 strncmp(normalized, "latin-1-", 8) == 0 ||
                 strncmp(normalized, "iso-8859-1-", 11) == 0 ||
                 strncmp(normalized, "iso-latin-1-", 12) == 0) {
            canonical = "iso-8859-1";
        }
        if (canonical != found) {
            PyMem_Free(found);
            found = _PyTok_CopyBytes(canonical, strlen(canonical));
            if (found == NULL) {
                return -1;
            }
        }
        *encoding = found;
        *scan_next = 0;
        return 0;
    }
    return 0;
}

_PyTok_EncodingResult
_PyTok_DetectEncoding(struct tok_state *tok, const _PyTok_Chunk *first,
                      const _PyTok_Chunk *second, int final,
                      Py_ssize_t *bom_len)
{
    int bom = first->len >= 3 &&
        (unsigned char)first->data[0] == 0xEF &&
        (unsigned char)first->data[1] == 0xBB &&
        (unsigned char)first->data[2] == 0xBF;
    *bom_len = bom ? 3 : 0;

    char *cookie = NULL;
    int scan_next = 0;
    int cookie_line = 1;
    const char *first_data = first->data + (bom ? 3 : 0);
    Py_ssize_t first_len = first->len - (bom ? 3 : 0);
    if (find_cookie(first_data, first_len, &cookie, &scan_next) < 0) {
        return _PYTOK_ENCODING_ERROR;
    }
    if (cookie == NULL && scan_next && second != NULL) {
        if (find_cookie(second->data, second->len, &cookie, &scan_next) < 0) {
            return _PYTOK_ENCODING_ERROR;
        }
        cookie_line = 2;
    }
    else if (cookie == NULL && scan_next && !final) {
        return _PYTOK_ENCODING_NEED_SECOND_LINE;
    }

    if (bom) {
        if (_PyTok_SetEncoding(tok, "utf-8") < 0) {
            PyMem_Free(cookie);
            return _PYTOK_ENCODING_ERROR;
        }
    }
    if (cookie == NULL) {
        return _PYTOK_ENCODING_DONE;
    }
    if (bom && strcmp(cookie, "utf-8") != 0) {
        const _PyTok_Chunk *line = cookie_line == 2 ? second : first;
        const char *line_data = line->data + (cookie_line == 1 ? 3 : 0);
        Py_ssize_t line_len = line->len - (cookie_line == 1 ? 3 : 0);
        const char *saved_line_start = tok->line_start;
        char *saved_cur = tok->cur;
        int saved_lineno = tok->lineno;
        tok->line_start = line_data;
        tok->cur = (char *)line_data;
        tok->lineno = cookie_line;
        int end_col = (int)Py_MIN(line_len, INT_MAX);
        if (end_col > 0 && (line_data[end_col - 1] == '\n' ||
                            line_data[end_col - 1] == '\r')) {
            end_col--;
        }
        _PyTokenizer_syntaxerror_known_range(
            tok, 0, end_col, "encoding problem: %s with BOM", cookie);
        tok->line_start = saved_line_start;
        tok->cur = saved_cur;
        tok->lineno = saved_lineno;
        PyMem_Free(cookie);
        return _PYTOK_ENCODING_ERROR;
    }
    if (!bom && _PyTok_SetEncoding(tok, cookie) < 0) {
        PyMem_Free(cookie);
        return _PYTOK_ENCODING_ERROR;
    }
    PyMem_Free(cookie);
    return _PYTOK_ENCODING_DONE;
}

int
_PyTok_DecodeOnce(struct tok_state *tok, _PyTok_Chunk *chunk,
                  const char *encoding, const char *errors)
{
    PyObject *unicode = PyUnicode_Decode(
        chunk->data, chunk->len, encoding, errors);
    if (unicode == NULL) {
        tok->done = PyErr_ExceptionMatches(PyExc_MemoryError)
            ? E_NOMEM : E_DECODE;
        return -1;
    }
    return chunk_set_unicode(tok, chunk, unicode, 0);
}

static Py_ssize_t
raw_line_length(const char *data, Py_ssize_t len)
{
    for (Py_ssize_t i = 0; i < len; i++) {
        if (data[i] == '\n') {
            return i + 1;
        }
        if (data[i] == '\r') {
            return i + 1 < len && data[i + 1] == '\n' ? i + 2 : i + 1;
        }
    }
    return len;
}

static int
store_prepared_source(struct tok_state *tok, const char *data, Py_ssize_t len,
                      int preserve_crlf, int add_final_newline)
{
    Py_ssize_t pos = 0;
    while (pos < len) {
        Py_ssize_t raw_line_len;
        if (preserve_crlf) {
            const char *newline = memchr(data + pos, '\n', len - pos);
            raw_line_len = newline == NULL
                ? len - pos : newline - data - pos + 1;
        }
        else {
            raw_line_len = raw_line_length(data + pos, len - pos);
        }
        int terminated = preserve_crlf
            ? data[pos + raw_line_len - 1] == '\n'
            : data[pos + raw_line_len - 1] == '\n' ||
              data[pos + raw_line_len - 1] == '\r';
        int add_newline = add_final_newline &&
            pos + raw_line_len == len && !terminated;
        int normalize = add_newline ||
            (!preserve_crlf &&
             memchr(data + pos, '\r', raw_line_len) != NULL);

        const char *line = data + pos;
        Py_ssize_t line_len = raw_line_len;
        char *normalized = NULL;
        int implicit = 0;
        if (normalize) {
            normalized = _PyTok_NormalizeNewlines(
                line, line_len, preserve_crlf, add_newline,
                &line_len, &implicit);
            if (normalized == NULL) {
                tok->done = E_NOMEM;
                return -1;
            }
            line = normalized;
        }
        _PyTok_Off appended = _PyTok_SourceAppendLine(
            &tok->source, line, line_len, implicit);
        PyMem_Free(normalized);
        if (appended < 0) {
            tok->done = PyErr_ExceptionMatches(PyExc_MemoryError)
                ? E_NOMEM : E_ERROR;
            return -1;
        }
        pos += raw_line_len;
    }
    return 0;
}

int
_PyTok_PrepareString(struct tok_state *tok, const char *input, int utf8_only,
                     int exec_input, int preserve_crlf)
{
    Py_ssize_t raw_len = strlen(input);
    char *raw = (char *)input;

    if (utf8_only) {
        if (_PyTok_SetEncoding(tok, "utf-8") < 0) {
            return -1;
        }
    }
    else {
        Py_ssize_t first_original_len = raw_line_length(raw, raw_len);
        _PyTok_Chunk first = {
            .data = raw,
            .len = first_original_len,
            .ownership = _PYTOK_CHUNK_BORROWED,
        };
        _PyTok_Chunk second = {0};
        int have_second = first_original_len < raw_len;
        if (have_second) {
            second.data = raw + first_original_len;
            second.len = raw_line_length(second.data,
                                         raw_len - first_original_len);
        }
        Py_ssize_t bom_len;
        _PyTok_EncodingResult detection = _PyTok_DetectEncoding(
            tok, &first, have_second ? &second : NULL, 1, &bom_len);
        if (detection == _PYTOK_ENCODING_ERROR) {
            return -1;
        }
        raw += bom_len;
        raw_len -= bom_len;
    }

    _PyTok_Chunk decoded = {
        .data = raw,
        .len = raw_len,
        .ownership = _PYTOK_CHUNK_BORROWED,
    };
    if (tok->encoding != NULL && strcmp(tok->encoding, "utf-8") != 0) {
        if (_PyTok_DecodeOnce(
                tok, &decoded, tok->encoding, NULL) < 0) {
            return -1;
        }
    }

    int stored = store_prepared_source(
        tok, decoded.data, decoded.len, preserve_crlf, exec_input);
    _PyTok_ChunkClear(&decoded);
    if (stored < 0) {
        return -1;
    }
    tok->str = tok->source.bytes != NULL ? tok->source.bytes : (char *)"";
    if (!utf8_only &&
            (tok->encoding == NULL || strcmp(tok->encoding, "utf-8") == 0) &&
            !_PyTokenizer_ensure_utf8(tok->str, tok, 1)) {
        return -1;
    }
    return 0;
}

int
_PyTok_StartDecoder(struct tok_state *tok, const char *errors)
{
    _PyTok_Reader *reader = tok->reader;
    if (tok->encoding == NULL || reader->decoder != NULL) {
        return 0;
    }
    if (reader->kind == _PYTOK_READER_FILE &&
            strcmp(tok->encoding, "utf-8") == 0) {
        return 0;
    }

    PyObject *codec = _PyCodec_LookupTextEncoding(tok->encoding, NULL);
    if (codec != NULL) {
        PyObject *factory = PyObject_GetAttrString(codec, "incrementaldecoder");
        Py_DECREF(codec);
        if (factory != NULL) {
            reader->decoder = PyObject_CallFunction(factory, "s", errors);
            Py_DECREF(factory);
        }
    }
    if (reader->decoder == NULL) {
        tok->done = PyErr_ExceptionMatches(PyExc_MemoryError)
            ? E_NOMEM : E_DECODE;
        if (reader->kind == _PYTOK_READER_FILE) {
            _PyTokenizer_raise_init_error(
                tok->filename != NULL ? tok->filename : Py_None);
        }
        return -1;
    }
    return 0;
}

int
_PyTok_DecodeChunk(struct tok_state *tok, _PyTok_Chunk *chunk, int final)
{
    _PyTok_Reader *reader = tok->reader;
    if (reader->decoder == NULL) {
        return 0;
    }
    int strip_bom = reader->kind == _PYTOK_READER_READLINE &&
        chunk->len >= 2 &&
        (((unsigned char)chunk->data[0] == 0xFF &&
          (unsigned char)chunk->data[1] == 0xFE) ||
         ((unsigned char)chunk->data[0] == 0xFE &&
          (unsigned char)chunk->data[1] == 0xFF));
    PyObject *input;
    if (chunk->ownership == _PYTOK_CHUNK_PYOBJECT &&
            PyBytes_Check(chunk->owner) &&
            chunk->data == PyBytes_AS_STRING(chunk->owner)) {
        input = Py_NewRef(chunk->owner);
    }
    else {
        input = PyBytes_FromStringAndSize(chunk->data, chunk->len);
    }
    if (input == NULL) {
        tok->done = E_NOMEM;
        return -1;
    }
    PyObject *unicode = PyObject_CallMethodObjArgs(
        reader->decoder, &_Py_ID(decode), input,
        final ? Py_True : Py_False, NULL);
    Py_DECREF(input);
    if (unicode == NULL) {
        tok->done = PyErr_ExceptionMatches(PyExc_MemoryError)
            ? E_NOMEM : E_DECODE;
        if (reader->kind == _PYTOK_READER_FILE) {
            _PyTokenizer_raise_init_error(
                tok->filename != NULL ? tok->filename : Py_None);
        }
        return -1;
    }
    if (!PyUnicode_Check(unicode)) {
        PyErr_Format(PyExc_TypeError,
                     "decoder should return a string result, not '%.200s'",
                     Py_TYPE(unicode)->tp_name);
        Py_DECREF(unicode);
        tok->done = E_DECODE;
        return -1;
    }
    return chunk_set_unicode(tok, chunk, unicode, strip_bom);
}

int
_PyTok_DecoderHasBufferedInput(struct tok_state *tok)
{
    if (tok->reader->decoder == NULL) {
        return 0;
    }
    PyObject *state = PyObject_CallMethodNoArgs(
        tok->reader->decoder, &_Py_ID(getstate));
    if (state == NULL) {
        tok->done = PyErr_ExceptionMatches(PyExc_MemoryError)
            ? E_NOMEM : E_DECODE;
        return -1;
    }
    if (!PyTuple_Check(state) || PyTuple_GET_SIZE(state) != 2 ||
            !PyBytes_Check(PyTuple_GET_ITEM(state, 0)) ||
            !PyLong_Check(PyTuple_GET_ITEM(state, 1))) {
        Py_DECREF(state);
        PyErr_SetString(PyExc_TypeError,
                        "incremental decoder getstate() must return (bytes, int)");
        tok->done = E_DECODE;
        return -1;
    }
    int pending = PyBytes_GET_SIZE(PyTuple_GET_ITEM(state, 0)) != 0;
    Py_DECREF(state);
    return pending;
}
