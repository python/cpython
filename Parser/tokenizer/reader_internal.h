#ifndef Py_TOKENIZER_READER_INTERNAL_H
#define Py_TOKENIZER_READER_INTERNAL_H

#include "Python.h"

typedef enum {
    _PYTOK_READER_PREPARED,
    _PYTOK_READER_FILE,
    _PYTOK_READER_READLINE,
    _PYTOK_READER_INTERACTIVE,
} _PyTok_ReaderKind;

typedef enum {
    _PYTOK_READ_LINE,
    _PYTOK_READ_EOF,
    _PYTOK_READ_STOPPED,
    _PYTOK_READ_INTERRUPT,
    _PYTOK_READ_ERROR,
} _PyTok_ReadResult;

typedef enum {
    _PYTOK_ENCODING_ERROR = -1,
    _PYTOK_ENCODING_DONE,
    _PYTOK_ENCODING_NEED_SECOND_LINE,
} _PyTok_EncodingResult;

typedef enum {
    _PYTOK_CHUNK_BORROWED,
    _PYTOK_CHUNK_PYMEM,
    _PYTOK_CHUNK_PYOBJECT,
} _PyTok_ChunkOwnership;

typedef struct {
    char *data;
    PyObject *owner;
    Py_ssize_t len;
    _PyTok_ChunkOwnership ownership;
    unsigned char implicit_newline;
} _PyTok_Chunk;

typedef struct _PyTok_Reader {
    PyObject *readline;
    PyObject *decoder;
    const char *prompt;
    const char *nextprompt;


    char *file_buffer;
    Py_ssize_t file_buffer_cap;
    _PyTok_Chunk prefetched_lines[2];

    char *decoded;
    Py_ssize_t decoded_pos;
    Py_ssize_t decoded_len;
    Py_ssize_t decoded_cap;
    _PyTok_ReaderKind kind;
    unsigned char decoded_tail_is_implicit;
    unsigned char file_initialized;
    unsigned char file_eof;
    unsigned char decoder_finalized;
    unsigned char stop_interactive;
} _PyTok_Reader;

struct tok_state;

char *_PyTok_CopyBytes(const char *, Py_ssize_t);
int _PyTok_DecodeOnce(
    struct tok_state *, _PyTok_Chunk *, const char *, const char *);
char *_PyTok_NormalizeNewlines(
    const char *, Py_ssize_t, int, int, Py_ssize_t *, int *);
void _PyTok_ChunkClear(_PyTok_Chunk *);
int _PyTok_SetEncoding(struct tok_state *, const char *);
_PyTok_EncodingResult _PyTok_DetectEncoding(
    struct tok_state *, const _PyTok_Chunk *, const _PyTok_Chunk *, int,
    Py_ssize_t *);
int _PyTok_PrepareString(struct tok_state *, const char *, int, int, int);
int _PyTok_StartDecoder(struct tok_state *, const char *);
int _PyTok_DecodeChunk(struct tok_state *, _PyTok_Chunk *, int);
int _PyTok_DecoderHasBufferedInput(struct tok_state *);

#endif
