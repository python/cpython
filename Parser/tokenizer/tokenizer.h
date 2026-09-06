#ifndef Py_TOKENIZER_H
#define Py_TOKENIZER_H

#include "Python.h"
#include "types.h"

struct tok_state;

/* Initialize before use. metadata owns a reference released by _PyToken_Free;
   a consumer taking that reference must set metadata to NULL. */
struct token {
    int level;
    int is_raw;
    _PyTok_Span span;
    _PyTok_Loc start_loc;
    _PyTok_Loc end_loc;
    PyObject *metadata;
};

typedef struct {
    const char *text;
    Py_ssize_t length;
    const char *line;
    Py_ssize_t line_length;
    const char *end_line;
    int implicit_newline;
    int at_eof;
} _PyToken_View;

/* Supplemental source context for a terminal error. location is the reporting
   cursor, independent of the scanner cursor; lineno == 0 means absent.
   The text span may cover multiple physical lines. */
typedef struct {
    _PyTok_Loc location;
    _PyTok_Span text_span;
} _PyTokenizer_Diagnostic;

typedef struct {
    int status;
    _PyTok_Loc location;
    _PyTok_Off cursor;
    _PyTok_Span input_span;
    _PyTok_Span line_span;
    int level;
    char delimiter;
    _PyTok_Loc delimiter_loc;
    int in_formatted_string;
    int is_interactive;
    int is_file;
    PyObject *filename;
    PyObject *module;
    const char *encoding;
} _PyTokenizer_Info;

int _PyTokenizer_Get(struct tok_state *, struct token *);
void _PyTokenizer_Free(struct tok_state *);
void _PyTokenizer_raise_init_error(PyObject *filename);
void _PyToken_Init(struct token *);
void _PyToken_Free(struct token *);

/* Views and borrowed snapshot references remain valid until the tokenizer is
   mutated or freed. Source spans may be discarded when reading more input. */
_PyTokenizer_Info _PyTokenizer_GetInfo(const struct tok_state *);
_PyTokenizer_Diagnostic _PyTokenizer_GetDiagnostic(const struct tok_state *);
/* An absent token span has a nonnull empty text view. */
const char *_PyToken_TextView(
    const struct tok_state *, const struct token *, Py_ssize_t *);
/* Pair the token with the type returned by the most recent Get. text is NULL
   for an absent span; line includes the token's complete physical line range. */
void _PyToken_GetView(
    const struct tok_state *tok, const struct token *token, int type,
    _PyToken_View *view);
const char *_PyTokenizer_SpanView(
    const struct tok_state *, _PyTok_Span, Py_ssize_t *);
/* For retained input: 1-based lines clamp to the first or final line.
   The view excludes the newline and need not be NUL-terminated. */
const char *_PyTokenizer_LineView(
    const struct tok_state *, Py_ssize_t, Py_ssize_t *);
/* Return NUL-terminated retained input, or NULL without setting an exception
   for streaming input or interactive input before its first line. */
const char *_PyTokenizer_RetainedSource(const struct tok_state *);
void _PyTokenizer_SetContext(
    struct tok_state *tok, PyObject *filename, PyObject *module);
void _PyTokenizer_SetOptions(
    struct tok_state *tok, int extra_tokens, int type_comments);
void _PyTokenizer_ImplyDedents(struct tok_state *);
int _PyTokenizer_HasTrailingStatement(const struct tok_state *);
int _PyTokenizer_IsInteractive(const struct tok_state *);
void _PyTokenizer_StopInteractive(struct tok_state *);

struct tok_state *_PyTokenizer_FromString(
    const char *input, int exec_input, int preserve_crlf);
struct tok_state *_PyTokenizer_FromUTF8(
    const char *input, int exec_input, int preserve_crlf);
struct tok_state *_PyTokenizer_FromReadline(
    PyObject *readline, const char *encoding);
struct tok_state *_PyTokenizer_FromFile(
    FILE *fp, const char *encoding, const char *ps1, const char *ps2);
/* Return the declared encoding in PyMem-allocated storage, or NULL.
   An exception is set on error. */
char *_PyTokenizer_FindEncodingFilename(int, PyObject *);

#define tok_dump _Py_tok_dump

#endif /* !Py_TOKENIZER_H */
