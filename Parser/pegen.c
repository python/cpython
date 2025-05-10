#include <Python.h>
#include "pycore_ast.h"           // _PyAST_Validate(),
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_parser.h"        // _PYPEGEN_NSTATISTICS
#include "pycore_pyerrors.h"      // PyExc_IncompleteInputError
#include "pycore_runtime.h"     // _PyRuntime
#include "pycore_unicodeobject.h" // _PyUnicode_InternImmortal
#include <errcode.h>

#include "lexer/lexer.h"
#include "tokenizer/tokenizer.h"
#include "pegen.h"

// Internal parser functions

asdl_stmt_seq*
_PyPegen_interactive_exit(Parser *p)
{
    if (p->errcode) {
        *(p->errcode) = E_EOF;
    }
    return NULL;
}

Py_ssize_t
_PyPegen_byte_offset_to_character_offset_line(PyObject *line, Py_ssize_t col_offset, Py_ssize_t end_col_offset)
{
    const unsigned char *data = (const unsigned char*)PyUnicode_AsUTF8(line);

    Py_ssize_t len = 0;
    while (col_offset < end_col_offset) {
        Py_UCS4 ch = data[col_offset];
        if (ch < 0x80) {
            col_offset += 1;
        } else if ((ch & 0xe0) == 0xc0) {
            col_offset += 2;
        } else if ((ch & 0xf0) == 0xe0) {
            col_offset += 3;
        } else if ((ch & 0xf8) == 0xf0) {
            col_offset += 4;
        } else {
            PyErr_SetString(PyExc_ValueError, "Invalid UTF-8 sequence");
            return -1;
        }
        len++;
    }
    return len;
}

Py_ssize_t
_PyPegen_byte_offset_to_character_offset_raw(const char* str, Py_ssize_t col_offset)
{
    Py_ssize_t len = (Py_ssize_t)strlen(str);
    if (col_offset > len + 1) {
        col_offset = len + 1;
    }
    assert(col_offset >= 0);
    PyObject *text = PyUnicode_DecodeUTF8(str, col_offset, "replace");
    if (!text) {
        return -1;
    }
    Py_ssize_t size = PyUnicode_GET_LENGTH(text);
    Py_DECREF(text);
    return size;
}

Py_ssize_t
_PyPegen_byte_offset_to_character_offset(PyObject *line, Py_ssize_t col_offset)
{
    const char *str = PyUnicode_AsUTF8(line);
    if (!str) {
        return -1;
    }
    return _PyPegen_byte_offset_to_character_offset_raw(str, col_offset);
}

// Here, mark is the start of the node, while p->mark is the end.
// If node==NULL, they should be the same.
int
_PyPegen_insert_memo(Parser *p, int mark, int type, void *node)
{
    // Insert in front
    Memo *m = _PyArena_Malloc(p->arena, sizeof(Memo));
    if (m == NULL) {
        return -1;
    }
    m->type = type;
    m->node = node;
    m->mark = p->mark;
    m->next = p->tokens[mark]->memo;
    p->tokens[mark]->memo = m;
    return 0;
}

// Like _PyPegen_insert_memo(), but updates an existing node if found.
int
_PyPegen_update_memo(Parser *p, int mark, int type, void *node)
{
    for (Memo *m = p->tokens[mark]->memo; m != NULL; m = m->next) {
        if (m->type == type) {
            // Update existing node.
            m->node = node;
            m->mark = p->mark;
            return 0;
        }
    }
    // Insert new node.
    return _PyPegen_insert_memo(p, mark, type, node);
}

static int
init_normalization(Parser *p)
{
    if (p->normalize) {
        return 1;
    }
    p->normalize = PyImport_ImportModuleAttrString("unicodedata", "normalize");
    if (!p->normalize)
    {
        return 0;
    }
    return 1;
}

static int
growable_comment_array_init(growable_comment_array *arr, size_t initial_size) {
    assert(initial_size > 0);
    arr->items = PyMem_Malloc(initial_size * sizeof(*arr->items));
    arr->size = initial_size;
    arr->num_items = 0;

    return arr->items != NULL;
}

static int
growable_comment_array_add(growable_comment_array *arr, int lineno, char *comment) {
    if (arr->num_items >= arr->size) {
        size_t new_size = arr->size * 2;
        void *new_items_array = PyMem_Realloc(arr->items, new_size * sizeof(*arr->items));
        if (!new_items_array) {
            return 0;
        }
        arr->items = new_items_array;
        arr->size = new_size;
    }

    arr->items[arr->num_items].lineno = lineno;
    arr->items[arr->num_items].comment = comment;  // Take ownership
    arr->num_items++;
    return 1;
}

static void
growable_comment_array_deallocate(growable_comment_array *arr) {
    for (unsigned i = 0; i < arr->num_items; i++) {
        PyMem_Free(arr->items[i].comment);
    }
    PyMem_Free(arr->items);
}

static int
_get_keyword_or_name_type(Parser *p, struct token *new_token)
{
    Py_ssize_t name_len = new_token->end_col_offset - new_token->col_offset;
    assert(name_len > 0);

    if (name_len >= p->n_keyword_lists ||
        p->keywords[name_len] == NULL ||
        p->keywords[name_len]->type == -1) {
        return NAME;
    }
    for (KeywordToken *k = p->keywords[name_len]; k != NULL && k->type != -1; k++) {
        if (strncmp(k->str, new_token->start, (size_t)name_len) == 0) {
            return k->type;
        }
    }
    return NAME;
}

static int
initialize_token(Parser *p, Token *parser_token, struct token *new_token, int token_type) {
    assert(parser_token != NULL);

    parser_token->type = (token_type == NAME) ? _get_keyword_or_name_type(p, new_token) : token_type;
    parser_token->bytes = PyBytes_FromStringAndSize(new_token->start, new_token->end - new_token->start);
    if (parser_token->bytes == NULL) {
        return -1;
    }
    if (_PyArena_AddPyObject(p->arena, parser_token->bytes) < 0) {
        Py_DECREF(parser_token->bytes);
        return -1;
    }

    parser_token->metadata = NULL;
    if (new_token->metadata != NULL) {
        if (_PyArena_AddPyObject(p->arena, new_token->metadata) < 0) {
            Py_DECREF(new_token->metadata);
            return -1;
        }
        parser_token->metadata = new_token->metadata;
        new_token->metadata = NULL;
    }

    parser_token->level = new_token->level;
    parser_token->lineno = new_token->lineno;
    parser_token->col_offset = p->tok->lineno == p->starting_lineno ? p->starting_col_offset + new_token->col_offset
                                                                    : new_token->col_offset;
    parser_token->end_lineno = new_token->end_lineno;
    parser_token->end_col_offset = p->tok->lineno == p->starting_lineno ? p->starting_col_offset + new_token->end_col_offset
                                                                 : new_token->end_col_offset;

    p->fill += 1;

    if (token_type == ERRORTOKEN && p->tok->done == E_DECODE) {
        return _Pypegen_raise_decode_error(p);
    }

    return (token_type == ERRORTOKEN ? _Pypegen_tokenizer_error(p) : 0);
}

static int
_resize_tokens_array(Parser *p) {
    int newsize = p->size * 2;
    Token **new_tokens = PyMem_Realloc(p->tokens, (size_t)newsize * sizeof(Token *));
    if (new_tokens == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    p->tokens = new_tokens;

    for (int i = p->size; i < newsize; i++) {
        p->tokens[i] = PyMem_Calloc(1, sizeof(Token));
        if (p->tokens[i] == NULL) {
            p->size = i; // Needed, in order to cleanup correctly after parser fails
            PyErr_NoMemory();
            return -1;
        }
    }
    p->size = newsize;
    return 0;
}

int
_PyPegen_fill_token(Parser *p)
{
    struct token new_token;
    _PyToken_Init(&new_token);
    int type = _PyTokenizer_Get(p->tok, &new_token);

    // Record and skip '# type: ignore' comments
    while (type == TYPE_IGNORE) {
        Py_ssize_t len = new_token.end_col_offset - new_token.col_offset;
        char *tag = PyMem_Malloc((size_t)len + 1);
        if (tag == NULL) {
            PyErr_NoMemory();
            goto error;
        }
        strncpy(tag, new_token.start, (size_t)len);
        tag[len] = '\0';
        // Ownership of tag passes to the growable array
        if (!growable_comment_array_add(&p->type_ignore_comments, p->tok->lineno, tag)) {
            PyErr_NoMemory();
            goto error;
        }
        type = _PyTokenizer_Get(p->tok, &new_token);
    }

    // If we have reached the end and we are in single input mode we need to insert a newline and reset the parsing
    if (p->start_rule == Py_single_input && type == ENDMARKER && p->parsing_started) {
        type = NEWLINE; /* Add an extra newline */
        p->parsing_started = 0;

        if (p->tok->indent && !(p->flags & PyPARSE_DONT_IMPLY_DEDENT)) {
            p->tok->pendin = -p->tok->indent;
            p->tok->indent = 0;
        }
    }
    else {
        p->parsing_started = 1;
    }

    // Check if we are at the limit of the token array capacity and resize if needed
    if ((p->fill == p->size) && (_resize_tokens_array(p) != 0)) {
        goto error;
    }

    Token *t = p->tokens[p->fill];
    return initialize_token(p, t, &new_token, type);
error:
    _PyToken_Free(&new_token);
    return -1;
}

#if defined(Py_DEBUG)
// Instrumentation to count the effectiveness of memoization.
// The array counts the number of tokens skipped by memoization,
// indexed by type.

#define NSTATISTICS _PYPEGEN_NSTATISTICS
#define memo_statistics _PyRuntime.parser.memo_statistics

#ifdef Py_GIL_DISABLED
#define MUTEX_LOCK() PyMutex_Lock(&_PyRuntime.parser.mutex)
#define MUTEX_UNLOCK() PyMutex_Unlock(&_PyRuntime.parser.mutex)
#else
#define MUTEX_LOCK()
#define MUTEX_UNLOCK()
#endif

void
_PyPegen_clear_memo_statistics(void)
{
    MUTEX_LOCK();
    for (int i = 0; i < NSTATISTICS; i++) {
        memo_statistics[i] = 0;
    }
    MUTEX_UNLOCK();
}

PyObject *
_PyPegen_get_memo_statistics(void)
{
    PyObject *ret = PyList_New(NSTATISTICS);
    if (ret == NULL) {
        return NULL;
    }

    MUTEX_LOCK();
    for (int i = 0; i < NSTATISTICS; i++) {
        PyObject *value = PyLong_FromLong(memo_statistics[i]);
        if (value == NULL) {
            MUTEX_UNLOCK();
            Py_DECREF(ret);
            return NULL;
        }
        // PyList_SetItem borrows a reference to value.
        if (PyList_SetItem(ret, i, value) < 0) {
            MUTEX_UNLOCK();
            Py_DECREF(ret);
            return NULL;
        }
    }
    MUTEX_UNLOCK();
    return ret;
}
#endif

int  // bool
_PyPegen_is_memoized(Parser *p, int type, void *pres)
{
    if (p->mark == p->fill) {
        if (_PyPegen_fill_token(p) < 0) {
            p->error_indicator = 1;
            return -1;
        }
    }

    Token *t = p->tokens[p->mark];

    for (Memo *m = t->memo; m != NULL; m = m->next) {
        if (m->type == type) {
#if defined(Py_DEBUG)
            if (0 <= type && type < NSTATISTICS) {
                long count = m->mark - p->mark;
                // A memoized negative result counts for one.
                if (count <= 0) {
                    count = 1;
                }
                MUTEX_LOCK();
                memo_statistics[type] += count;
                MUTEX_UNLOCK();
            }
#endif
            p->mark = m->mark;
            *(void **)(pres) = m->node;
            return 1;
        }
    }
    return 0;
}

int
_PyPegen_lookahead_with_name(int positive, expr_ty (func)(Parser *), Parser *p)
{
    int mark = p->mark;
    void *res = func(p);
    p->mark = mark;
    return (res != NULL) == positive;
}

int
_PyPegen_lookahead_with_string(int positive, expr_ty (func)(Parser *, const char*), Parser *p, const char* arg)
{
    int mark = p->mark;
    void *res = func(p, arg);
    p->mark = mark;
    return (res != NULL) == positive;
}

int
_PyPegen_lookahead_with_int(int positive, Token *(func)(Parser *, int), Parser *p, int arg)
{
    int mark = p->mark;
    void *res = func(p, arg);
    p->mark = mark;
    return (res != NULL) == positive;
}

// gh-111178: Use _Py_NO_SANITIZE_UNDEFINED to disable sanitizer checks on
// undefined behavior (UBsan) in this function, rather than changing 'func'
// callback API.
int _Py_NO_SANITIZE_UNDEFINED
_PyPegen_lookahead(int positive, void *(func)(Parser *), Parser *p)
{
    int mark = p->mark;
    void *res = func(p);
    p->mark = mark;
    return (res != NULL) == positive;
}

Token *
_PyPegen_expect_token(Parser *p, int type)
{
    if (p->mark == p->fill) {
        if (_PyPegen_fill_token(p) < 0) {
            p->error_indicator = 1;
            return NULL;
        }
    }
    Token *t = p->tokens[p->mark];
    if (t->type != type) {
       return NULL;
    }
    p->mark += 1;
    return t;
}

void*
_PyPegen_expect_forced_result(Parser *p, void* result, const char* expected) {

    if (p->error_indicator == 1) {
        return NULL;
    }
    if (result == NULL) {
        RAISE_SYNTAX_ERROR("expected (%s)", expected);
        return NULL;
    }
    return result;
}

Token *
_PyPegen_expect_forced_token(Parser *p, int type, const char* expected) {

    if (p->error_indicator == 1) {
        return NULL;
    }

    if (p->mark == p->fill) {
        if (_PyPegen_fill_token(p) < 0) {
            p->error_indicator = 1;
            return NULL;
        }
    }
    Token *t = p->tokens[p->mark];
    if (t->type != type) {
        RAISE_SYNTAX_ERROR_KNOWN_LOCATION(t, "expected '%s'", expected);
        return NULL;
    }
    p->mark += 1;
    return t;
}

expr_ty
_PyPegen_expect_soft_keyword(Parser *p, const char *keyword)
{
    if (p->mark == p->fill) {
        if (_PyPegen_fill_token(p) < 0) {
            p->error_indicator = 1;
            return NULL;
        }
    }
    Token *t = p->tokens[p->mark];
    if (t->type != NAME) {
        return NULL;
    }
    const char *s = PyBytes_AsString(t->bytes);
    if (!s) {
        p->error_indicator = 1;
        return NULL;
    }
    if (strcmp(s, keyword) != 0) {
        return NULL;
    }
    return _PyPegen_name_token(p);
}

Token *
_PyPegen_get_last_nonnwhitespace_token(Parser *p)
{
    assert(p->mark >= 0);
    Token *token = NULL;
    for (int m = p->mark - 1; m >= 0; m--) {
        token = p->tokens[m];
        if (token->type != ENDMARKER && (token->type < NEWLINE || token->type > DEDENT)) {
            break;
        }
    }
    return token;
}

PyObject *
_PyPegen_new_identifier(Parser *p, const char *n)
{
    PyObject *id = PyUnicode_DecodeUTF8(n, (Py_ssize_t)strlen(n), NULL);
    if (!id) {
        goto error;
    }
    /* Check whether there are non-ASCII characters in the
       identifier; if so, normalize to NFKC. */
    if (!PyUnicode_IS_ASCII(id))
    {
        if (!init_normalization(p))
        {
            Py_DECREF(id);
            goto error;
        }
        PyObject *form = PyUnicode_InternFromString("NFKC");
        if (form == NULL)
        {
            Py_DECREF(id);
            goto error;
        }
        PyObject *args[2] = {form, id};
        PyObject *id2 = PyObject_Vectorcall(p->normalize, args, 2, NULL);
        Py_DECREF(id);
        Py_DECREF(form);
        if (!id2) {
            goto error;
        }

        if (!PyUnicode_Check(id2))
        {
            PyErr_Format(PyExc_TypeError,
                         "unicodedata.normalize() must return a string, not "
                         "%.200s",
                         _PyType_Name(Py_TYPE(id2)));
            Py_DECREF(id2);
            goto error;
        }
        id = id2;
    }
    static const char * const forbidden[] = {
        "None",
        "True",
        "False",
        NULL
    };
    for (int i = 0; forbidden[i] != NULL; i++) {
        if (_PyUnicode_EqualToASCIIString(id, forbidden[i])) {
            PyErr_Format(PyExc_ValueError,
                         "identifier field can't represent '%s' constant",
                         forbidden[i]);
            Py_DECREF(id);
            goto error;
        }
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyUnicode_InternImmortal(interp, &id);
    if (_PyArena_AddPyObject(p->arena, id) < 0)
    {
        Py_DECREF(id);
        goto error;
    }
    return id;

error:
    p->error_indicator = 1;
    return NULL;
}

static expr_ty
_PyPegen_name_from_token(Parser *p, Token* t)
{
    if (t == NULL) {
        return NULL;
    }
    const char *s = PyBytes_AsString(t->bytes);
    if (!s) {
        p->error_indicator = 1;
        return NULL;
    }
    PyObject *id = _PyPegen_new_identifier(p, s);
    if (id == NULL) {
        p->error_indicator = 1;
        return NULL;
    }
    return _PyAST_Name(id, Load, t->lineno, t->col_offset, t->end_lineno,
                       t->end_col_offset, p->arena);
}

expr_ty
_PyPegen_name_token(Parser *p)
{
    Token *t = _PyPegen_expect_token(p, NAME);
    return _PyPegen_name_from_token(p, t);
}

void *
_PyPegen_string_token(Parser *p)
{
    return _PyPegen_expect_token(p, STRING);
}

expr_ty _PyPegen_soft_keyword_token(Parser *p) {
    Token *t = _PyPegen_expect_token(p, NAME);
    if (t == NULL) {
        return NULL;
    }
    char *the_token;
    Py_ssize_t size;
    PyBytes_AsStringAndSize(t->bytes, &the_token, &size);
    for (char **keyword = p->soft_keywords; *keyword != NULL; keyword++) {
        if (strncmp(*keyword, the_token, (size_t)size) == 0) {
            return _PyPegen_name_from_token(p, t);
        }
    }
    return NULL;
}

static PyObject *
parsenumber_raw(const char *s)
{
    const char *end;
    long x;
    double dx;
    Py_complex compl;
    int imflag;

    assert(s != NULL);
    errno = 0;
    end = s + strlen(s) - 1;
    imflag = *end == 'j' || *end == 'J';
    if (s[0] == '0') {
        x = (long)PyOS_strtoul(s, (char **)&end, 0);
        if (x < 0 && errno == 0) {
            return PyLong_FromString(s, (char **)0, 0);
        }
    }
    else {
        x = PyOS_strtol(s, (char **)&end, 0);
    }
    if (*end == '\0') {
        if (errno != 0) {
            return PyLong_FromString(s, (char **)0, 0);
        }
        return PyLong_FromLong(x);
    }
    /* XXX Huge floats may silently fail */
    if (imflag) {
        compl.real = 0.;
        compl.imag = PyOS_string_to_double(s, (char **)&end, NULL);
        if (compl.imag == -1.0 && PyErr_Occurred()) {
            return NULL;
        }
        return PyComplex_FromCComplex(compl);
    }
    dx = PyOS_string_to_double(s, NULL, NULL);
    if (dx == -1.0 && PyErr_Occurred()) {
        return NULL;
    }
    return PyFloat_FromDouble(dx);
}

static PyObject *
parsenumber(const char *s)
{
    char *dup;
    char *end;
    PyObject *res = NULL;

    assert(s != NULL);

    if (strchr(s, '_') == NULL) {
        return parsenumber_raw(s);
    }
    /* Create a duplicate without underscores. */
    dup = PyMem_Malloc(strlen(s) + 1);
    if (dup == NULL) {
        return PyErr_NoMemory();
    }
    end = dup;
    for (; *s; s++) {
        if (*s != '_') {
            *end++ = *s;
        }
    }
    *end = '\0';
    res = parsenumber_raw(dup);
    PyMem_Free(dup);
    return res;
}

expr_ty
_PyPegen_number_token(Parser *p)
{
    Token *t = _PyPegen_expect_token(p, NUMBER);
    if (t == NULL) {
        return NULL;
    }

    const char *num_raw = PyBytes_AsString(t->bytes);
    if (num_raw == NULL) {
        p->error_indicator = 1;
        return NULL;
    }

    if (p->feature_version < 6 && strchr(num_raw, '_') != NULL) {
        p->error_indicator = 1;
        return RAISE_SYNTAX_ERROR("Underscores in numeric literals are only supported "
                                  "in Python 3.6 and greater");
    }

    PyObject *c = parsenumber(num_raw);

    if (c == NULL) {
        p->error_indicator = 1;
        PyThreadState *tstate = _PyThreadState_GET();
        // The only way a ValueError should happen in _this_ code is via
        // PyLong_FromString hitting a length limit.
        if (tstate->current_exception != NULL &&
            Py_TYPE(tstate->current_exception) == (PyTypeObject *)PyExc_ValueError
        ) {
            PyObject *exc = PyErr_GetRaisedException();
            /* Intentionally omitting columns to avoid a wall of 1000s of '^'s
             * on the error message. Nobody is going to overlook their huge
             * numeric literal once given the line. */
            RAISE_ERROR_KNOWN_LOCATION(
                p, PyExc_SyntaxError,
                t->lineno, -1 /* col_offset */,
                t->end_lineno, -1 /* end_col_offset */,
                "%S - Consider hexadecimal for huge integer literals "
                "to avoid decimal conversion limits.",
                exc);
            Py_DECREF(exc);
        }
        return NULL;
    }

    if (_PyArena_AddPyObject(p->arena, c) < 0) {
        Py_DECREF(c);
        p->error_indicator = 1;
        return NULL;
    }

    return _PyAST_Constant(c, NULL, t->lineno, t->col_offset, t->end_lineno,
                           t->end_col_offset, p->arena);
}

/* Check that the source for a single input statement really is a single
   statement by looking at what is left in the buffer after parsing.
   Trailing whitespace and comments are OK. */
static int // bool
bad_single_statement(Parser *p)
{
    char *cur = p->tok->cur;
    char c = *cur;

    for (;;) {
        while (c == ' ' || c == '\t' || c == '\n' || c == '\014') {
            c = *++cur;
        }

        if (!c) {
            return 0;
        }

        if (c != '#') {
            return 1;
        }

        /* Suck up comment. */
        while (c && c != '\n') {
            c = *++cur;
        }
    }
}

static int
compute_parser_flags(PyCompilerFlags *flags)
{
    int parser_flags = 0;
    if (!flags) {
        return 0;
    }
    if (flags->cf_flags & PyCF_DONT_IMPLY_DEDENT) {
        parser_flags |= PyPARSE_DONT_IMPLY_DEDENT;
    }
    if (flags->cf_flags & PyCF_IGNORE_COOKIE) {
        parser_flags |= PyPARSE_IGNORE_COOKIE;
    }
    if (flags->cf_flags & CO_FUTURE_BARRY_AS_BDFL) {
        parser_flags |= PyPARSE_BARRY_AS_BDFL;
    }
    if (flags->cf_flags & PyCF_TYPE_COMMENTS) {
        parser_flags |= PyPARSE_TYPE_COMMENTS;
    }
    if (flags->cf_flags & PyCF_ALLOW_INCOMPLETE_INPUT) {
        parser_flags |= PyPARSE_ALLOW_INCOMPLETE_INPUT;
    }
    return parser_flags;
}

// Parser API

Parser *
_PyPegen_Parser_New(struct tok_state *tok, int start_rule, int flags,
                    int feature_version, int *errcode, const char* source, PyArena *arena)
{
    Parser *p = PyMem_Malloc(sizeof(Parser));
    if (p == NULL) {
        return (Parser *) PyErr_NoMemory();
    }
    assert(tok != NULL);
    tok->type_comments = (flags & PyPARSE_TYPE_COMMENTS) > 0;
    p->tok = tok;
    p->keywords = NULL;
    p->n_keyword_lists = -1;
    p->soft_keywords = NULL;
    p->tokens = PyMem_Malloc(sizeof(Token *));
    if (!p->tokens) {
        PyMem_Free(p);
        return (Parser *) PyErr_NoMemory();
    }
    p->tokens[0] = PyMem_Calloc(1, sizeof(Token));
    if (!p->tokens[0]) {
        PyMem_Free(p->tokens);
        PyMem_Free(p);
        return (Parser *) PyErr_NoMemory();
    }
    if (!growable_comment_array_init(&p->type_ignore_comments, 10)) {
        PyMem_Free(p->tokens[0]);
        PyMem_Free(p->tokens);
        PyMem_Free(p);
        return (Parser *) PyErr_NoMemory();
    }

    p->mark = 0;
    p->fill = 0;
    p->size = 1;

    p->errcode = errcode;
    p->arena = arena;
    p->start_rule = start_rule;
    p->parsing_started = 0;
    p->normalize = NULL;
    p->error_indicator = 0;

    p->starting_lineno = 0;
    p->starting_col_offset = 0;
    p->flags = flags;
    p->feature_version = feature_version;
    p->known_err_token = NULL;
    p->level = 0;
    p->call_invalid_rules = 0;
    p->last_stmt_location.lineno = 0;
    p->last_stmt_location.col_offset = 0;
    p->last_stmt_location.end_lineno = 0;
    p->last_stmt_location.end_col_offset = 0;
#ifdef Py_DEBUG
    p->debug = _Py_GetConfig()->parser_debug;
#endif
    return p;
}

void
_PyPegen_Parser_Free(Parser *p)
{
    Py_XDECREF(p->normalize);
    for (int i = 0; i < p->size; i++) {
        PyMem_Free(p->tokens[i]);
    }
    PyMem_Free(p->tokens);
    growable_comment_array_deallocate(&p->type_ignore_comments);
    PyMem_Free(p);
}

static void
reset_parser_state_for_error_pass(Parser *p)
{
    p->last_stmt_location.lineno = 0;
    p->last_stmt_location.col_offset = 0;
    p->last_stmt_location.end_lineno = 0;
    p->last_stmt_location.end_col_offset = 0;
    for (int i = 0; i < p->fill; i++) {
        p->tokens[i]->memo = NULL;
    }
    p->mark = 0;
    p->call_invalid_rules = 1;
    // Don't try to get extra tokens in interactive mode when trying to
    // raise specialized errors in the second pass.
    p->tok->interactive_underflow = IUNDERFLOW_STOP;
}

static inline int
_is_end_of_source(Parser *p) {
    int err = p->tok->done;
    return err == E_EOF || err == E_EOFS || err == E_EOLS;
}

static void
_PyPegen_set_syntax_error_metadata(Parser *p) {
    PyObject *exc = PyErr_GetRaisedException();
    if (!exc || !PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_SyntaxError)) {
        PyErr_SetRaisedException(exc);
        return;
    }
    const char *source = NULL;
    if (p->tok->str != NULL) {
        source = p->tok->str;
    }
    if (!source && p->tok->fp_interactive && p->tok->interactive_src_start) {
        source = p->tok->interactive_src_start;
    }
    PyObject* the_source = NULL;
    if (source) {
        if (p->tok->encoding == NULL) {
            the_source = PyUnicode_FromString(source);
        } else {
            the_source = PyUnicode_Decode(source, strlen(source), p->tok->encoding, NULL);
        }
    }
    if (!the_source) {
        PyErr_Clear();
        the_source = Py_None;
        Py_INCREF(the_source);
    }
    PyObject* metadata = Py_BuildValue(
        "(iiN)",
        p->last_stmt_location.lineno,
        p->last_stmt_location.col_offset,
        the_source // N gives ownership to metadata
    );
    if (!metadata) {
        Py_DECREF(the_source);
        PyErr_Clear();
        return;
    }
    PySyntaxErrorObject *syntax_error = (PySyntaxErrorObject *)exc;

    Py_XDECREF(syntax_error->metadata);
    syntax_error->metadata = metadata;
    PyErr_SetRaisedException(exc);
}

void *
_PyPegen_run_parser(Parser *p)
{
    void *res = _PyPegen_parse(p);
    assert(p->level == 0);
    if (res == NULL) {
        if ((p->flags & PyPARSE_ALLOW_INCOMPLETE_INPUT) &&  _is_end_of_source(p)) {
            PyErr_Clear();
            return _PyPegen_raise_error(p, PyExc_IncompleteInputError, 0, "incomplete input");
        }
        if (PyErr_Occurred() && !PyErr_ExceptionMatches(PyExc_SyntaxError)) {
            return NULL;
        }
       // Make a second parser pass. In this pass we activate heavier and slower checks
        // to produce better error messages and more complete diagnostics. Extra "invalid_*"
        // rules will be active during parsing.
        Token *last_token = p->tokens[p->fill - 1];
        reset_parser_state_for_error_pass(p);
        _PyPegen_parse(p);

        // Set SyntaxErrors accordingly depending on the parser/tokenizer status at the failure
        // point.
        _Pypegen_set_syntax_error(p, last_token);

        // Set the metadata in the exception from p->last_stmt_location
        if (PyErr_ExceptionMatches(PyExc_SyntaxError)) {
            _PyPegen_set_syntax_error_metadata(p);
        }
       return NULL;
    }

    if (p->start_rule == Py_single_input && bad_single_statement(p)) {
        p->tok->done = E_BADSINGLE; // This is not necessary for now, but might be in the future
        return RAISE_SYNTAX_ERROR("multiple statements found while compiling a single statement");
    }

    // test_peg_generator defines _Py_TEST_PEGEN to not call PyAST_Validate()
#if defined(Py_DEBUG) && !defined(_Py_TEST_PEGEN)
    if (p->start_rule == Py_single_input ||
        p->start_rule == Py_file_input ||
        p->start_rule == Py_eval_input)
    {
        if (!_PyAST_Validate(res)) {
            return NULL;
        }
    }
#endif
    return res;
}

mod_ty
_PyPegen_run_parser_from_file_pointer(FILE *fp, int start_rule, PyObject *filename_ob,
                             const char *enc, const char *ps1, const char *ps2,
                             PyCompilerFlags *flags, int *errcode,
                             PyObject **interactive_src, PyArena *arena)
{
    struct tok_state *tok = _PyTokenizer_FromFile(fp, enc, ps1, ps2);
    if (tok == NULL) {
        if (PyErr_Occurred()) {
            _PyPegen_raise_tokenizer_init_error(filename_ob);
            return NULL;
        }
        return NULL;
    }
    if (!tok->fp || ps1 != NULL || ps2 != NULL ||
        PyUnicode_CompareWithASCIIString(filename_ob, "<stdin>") == 0) {
        tok->fp_interactive = 1;
    }
    // This transfers the ownership to the tokenizer
    tok->filename = Py_NewRef(filename_ob);

    // From here on we need to clean up even if there's an error
    mod_ty result = NULL;

    int parser_flags = compute_parser_flags(flags);
    Parser *p = _PyPegen_Parser_New(tok, start_rule, parser_flags, PY_MINOR_VERSION,
                                    errcode, NULL, arena);
    if (p == NULL) {
        goto error;
    }

    result = _PyPegen_run_parser(p);
    _PyPegen_Parser_Free(p);

    if (tok->fp_interactive && tok->interactive_src_start && result && interactive_src != NULL) {
        *interactive_src = PyUnicode_FromString(tok->interactive_src_start);
        if (!interactive_src || _PyArena_AddPyObject(arena, *interactive_src) < 0) {
            Py_XDECREF(interactive_src);
            result = NULL;
            goto error;
        }
    }

error:
    _PyTokenizer_Free(tok);
    return result;
}

mod_ty
_PyPegen_run_parser_from_string(const char *str, int start_rule, PyObject *filename_ob,
                       PyCompilerFlags *flags, PyArena *arena)
{
    int exec_input = start_rule == Py_file_input;

    struct tok_state *tok;
    if (flags != NULL && flags->cf_flags & PyCF_IGNORE_COOKIE) {
        tok = _PyTokenizer_FromUTF8(str, exec_input, 0);
    } else {
        tok = _PyTokenizer_FromString(str, exec_input, 0);
    }
    if (tok == NULL) {
        if (PyErr_Occurred()) {
            _PyPegen_raise_tokenizer_init_error(filename_ob);
        }
        return NULL;
    }
    // This transfers the ownership to the tokenizer
    tok->filename = Py_NewRef(filename_ob);

    // We need to clear up from here on
    mod_ty result = NULL;

    int parser_flags = compute_parser_flags(flags);
    int feature_version = flags && (flags->cf_flags & PyCF_ONLY_AST) ?
        flags->cf_feature_version : PY_MINOR_VERSION;
    Parser *p = _PyPegen_Parser_New(tok, start_rule, parser_flags, feature_version,
                                    NULL, str, arena);
    if (p == NULL) {
        goto error;
    }

    result = _PyPegen_run_parser(p);
    _PyPegen_Parser_Free(p);

error:
    _PyTokenizer_Free(tok);
    return result;
}
