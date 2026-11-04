#ifndef Py_TOKENIZER_H
#define Py_TOKENIZER_H

#include "Python.h"
#include "types.h"

struct tok_state;

typedef enum {
    _PyTokenizer_KEEP_NEWLINE,   // Preserve the original line terminator.
    _PyTokenizer_IMPLY_NEWLINE,  // Add a missing line terminator.
    _PyTokenizer_APPEND_NEWLINE, // Also append a blank line to terminated input.
} _PyTokenizer_Newline;

/* Reasons that the appended newline can change the token stream. */
enum {
    _PyTokenizer_NEWLINE_STATEMENT = 1,
    _PyTokenizer_NEWLINE_INDENT = 2,
    _PyTokenizer_NEWLINE_CONTINUATION = 4,
};

/* Initialize before use. metadata owns a reference released by _PyToken_Free;
   a consumer taking that reference must set metadata to NULL. */
struct token {
    int type;
    int level;
    unsigned char is_raw;
    unsigned char is_incomplete;
    _PyTok_Span span;
    _PyTok_Loc start_loc;
    _PyTok_Loc end_loc;
    PyObject *metadata;
};

typedef struct {
    const char *text;
    Py_ssize_t length;
    _PyTok_Span line_span;
    _PyTok_Off end_line_start;
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
    unsigned char implicit_newline;  // _PyTokenizer_Newline
    _PyTokenizer_Diagnostic diagnostic;
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

/* Get replaces the initialized token, releasing its previous metadata.
   Errors are returned as ERRORTOKEN, with or without a Python exception.
   In incomplete-input mode, an unfinished string retains its token type,
   sets is_incomplete and has E_INCOMPLETE status. Only f-string and t-string
   middle tokens retain their text span. No Python exception is set. */
void _PyTokenizer_Get(struct tok_state *, struct token *);
void _PyTokenizer_Free(struct tok_state *);
int _PyTokenizer_Traverse(struct tok_state *, visitproc, void *);
void _PyTokenizer_raise_init_error(PyObject *filename);
void _PyToken_Init(struct token *);
static inline void
_PyToken_Free(struct token *token)
{
    Py_CLEAR(token->metadata);
}

/* Views and borrowed snapshot references remain valid until the tokenizer is
   mutated or freed. Source spans may be discarded when reading more input. */
_PyTokenizer_Info _PyTokenizer_GetInfo(const struct tok_state *);
int _PyTokenizer_GetStatus(const struct tok_state *);
int _PyTokenizer_GetNewlineRequirements(const struct tok_state *);
/* An absent token span has a nonnull empty text view. */
const char *_PyToken_TextView(
    const struct tok_state *, const struct token *, Py_ssize_t *);
/* Use the token from the most recent Get. An absent span has nonnull empty text.
   line_span covers the token's complete physical line range. end_line_start
   is the logical offset of its final physical line. */
void _PyToken_GetView(
    const struct tok_state *tok, const struct token *token,
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
void _PyTokenizer_AllowIncompleteInput(struct tok_state *, int);
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

#endif /* !Py_TOKENIZER_H */
