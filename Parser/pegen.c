#include <Python.h>
#include "pycore_ast.h"           // _PyAST_Validate(),
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_parser.h"        // _PYPEGEN_NSTATISTICS
#include "pycore_pyerrors.h"      // PyExc_IncompleteInputError
#include "pycore_runtime.h"       // _PyRuntime
#include "pycore_unicodeobject.h" // _PyUnicode_InternImmortal
#include <errcode.h>

#include "lexer/state.h"
#include "tokenizer/tokenizer.h"
#include "pegen.h"

#define IDENTIFIER_CACHE_SIZE 2048  // Must be a power of two.
#define IDENTIFIER_CACHE_MAX_PROBES 8
#define PEGEN_ARRAY_GROWTH_FACTOR 2
#define TYPE_IGNORE_INITIAL_CAPACITY 10

struct _identifier_cache_entry {
    const char *key;   // Borrowed from arena-owned token bytes.
    Py_ssize_t len;
    Py_hash_t hash;
    PyObject *value;   // Borrowed from an arena-owned identifier.
};

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
    if (data == NULL) {
        return -1;
    }

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
    p->tokens[mark]->memo_mask |= 1ULL << (type & 63);
    return 0;
}

// Like _PyPegen_insert_memo(), but returns the inserted Memo so callers
// can update it in place without re-walking the token's memo list.
Memo *
_PyPegen_insert_memo_direct(Parser *p, int mark, int type)
{
    if (_PyPegen_insert_memo(p, mark, type, NULL) < 0) {
        return NULL;
    }
    return p->tokens[mark]->memo;
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

int
_PyPegen_grow_loop_buffer(void ***buffer, Py_ssize_t *capacity)
{
    if ((size_t)*capacity > (size_t)PY_SSIZE_T_MAX /
            (PEGEN_ARRAY_GROWTH_FACTOR * sizeof(**buffer))) {
        return -1;
    }
    Py_ssize_t new_capacity = *capacity == 0
        ? 1 : *capacity * PEGEN_ARRAY_GROWTH_FACTOR;
    void **new_buffer = PyMem_Realloc(*buffer, new_capacity * sizeof(**buffer));
    if (new_buffer == NULL) {
        return -1;
    }
    *buffer = new_buffer;
    *capacity = new_capacity;
    return 0;
}

static int
growable_comment_array_add(growable_comment_array *arr, int lineno, char *comment)
{
    if (arr->num_items >= arr->size) {
        if (arr->size > (size_t)PY_SSIZE_T_MAX /
                (PEGEN_ARRAY_GROWTH_FACTOR * sizeof(*arr->items))) {
            return 0;
        }
        size_t new_size = arr->size == 0
            ? TYPE_IGNORE_INITIAL_CAPACITY
            : arr->size * PEGEN_ARRAY_GROWTH_FACTOR;
        void *new_items_array = PyMem_Realloc(
            arr->items, new_size * sizeof(*arr->items));
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
growable_comment_array_deallocate(growable_comment_array *arr)
{
    for (size_t i = 0; i < arr->num_items; i++) {
        PyMem_Free(arr->items[i].comment);
    }
    PyMem_Free(arr->items);
}

static int
_get_keyword_or_name_type(Parser *p, const char *text, Py_ssize_t length)
{
    assert(length > 0);

    if (length >= p->n_keyword_lists ||
        p->keywords[length] == NULL ||
        p->keywords[length]->type == -1) {
        return NAME;
    }
    for (KeywordToken *k = p->keywords[length]; k != NULL && k->type != -1; k++) {
        if (memcmp(k->str, text, (size_t)length) == 0) {
            return k->type;
        }
    }
    return NAME;
}

// Keep text only where grammar actions or helpers read it. Keyword error
// actions use fixed spellings, so keywords do not need their text.
static inline int
token_needs_text(int type)
{
    switch (type) {
        case NAME:
        case NUMBER:
        case STRING:
        case FSTRING_START:
        case FSTRING_MIDDLE:
        case FSTRING_END:
        case TSTRING_START:
        case TSTRING_MIDDLE:
        case TSTRING_END:
        case TYPE_COMMENT:
        case NOTEQUAL:  // _PyPegen_check_barry_as_flufl() distinguishes != and <>.
            return 1;
        default:
            return 0;
    }
}

static int
initialize_token(Parser *p, Token *parser_token, struct token *new_token, int token_type) {
    assert(parser_token != NULL);

    parser_token->type = token_type;
    parser_token->bytes = NULL;
    if (token_needs_text(token_type) && new_token->span.start >= 0) {
        Py_ssize_t length;
        const char *text = _PyToken_TextView(p->tok, new_token, &length);
        if (token_type == NAME) {
            parser_token->type = _get_keyword_or_name_type(p, text, length);
        }
        // A name reclassified as a keyword does not need its text.
        if (parser_token->type == token_type) {
            parser_token->bytes = PyBytes_FromStringAndSize(text, length);
            if (parser_token->bytes == NULL) {
                return -1;
            }
            if (_PyArena_AddPyObject(p->arena, parser_token->bytes) < 0) {
                Py_DECREF(parser_token->bytes);
                return -1;
            }
        }
    }

    parser_token->metadata = NULL;
    if (new_token->metadata != NULL) {
        if (_PyArena_AddPyObject(p->arena, new_token->metadata) < 0) {
            return -1;
        }
        parser_token->metadata = new_token->metadata;
        new_token->metadata = NULL;
    }

    parser_token->level = new_token->level;
    parser_token->is_raw = new_token->is_raw;
    parser_token->lineno = new_token->start_loc.lineno;
    parser_token->col_offset = new_token->end_loc.lineno == p->starting_lineno
        ? p->starting_col_offset + new_token->start_loc.byte_col
        : new_token->start_loc.byte_col;
    parser_token->end_lineno = new_token->end_loc.lineno;
    parser_token->end_col_offset = new_token->end_loc.lineno == p->starting_lineno
        ? p->starting_col_offset + new_token->end_loc.byte_col
        : new_token->end_loc.byte_col;

    p->fill += 1;

    if (token_type == ERRORTOKEN && _PyTokenizer_GetStatus(p->tok) == E_DECODE) {
        return _Pypegen_raise_decode_error(p);
    }

    return (token_type == ERRORTOKEN ? _Pypegen_tokenizer_error(p) : 0);
}

static int
_resize_tokens_array(Parser *p) {
    if (p->size > INT_MAX / PEGEN_ARRAY_GROWTH_FACTOR ||
            (size_t)p->size > PY_SSIZE_T_MAX /
                (PEGEN_ARRAY_GROWTH_FACTOR * sizeof(*p->tokens))) {
        PyErr_NoMemory();
        return -1;
    }
    int newsize = p->size * PEGEN_ARRAY_GROWTH_FACTOR;
    Token **new_tokens = PyMem_Realloc(p->tokens, (size_t)newsize * sizeof(Token *));
    if (new_tokens == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    p->tokens = new_tokens;

    // Allocate new tokens together without moving existing ones: parser rules
    // may still hold pointers into earlier blocks. Parser_Free frees each block.
    Token *chunk = PyMem_Calloc((size_t)(newsize - p->size), sizeof(Token));
    if (chunk == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    for (int i = p->size; i < newsize; i++) {
        p->tokens[i] = &chunk[i - p->size];
    }
    p->size = newsize;
    return 0;
}

int
_PyPegen_append_token(Parser *p, struct token *token)
{
    int type = token->type;
    // Single input inserts a final newline and may still need dedents. Apply
    // the same normalization when the error pass fills the replay cache.
    if (p->start_rule == Py_single_input && type == ENDMARKER && p->parsing_started) {
        type = NEWLINE;
        p->parsing_started = 0;
        if (!(p->flags & PyPARSE_DONT_IMPLY_DEDENT)) {
            _PyTokenizer_ImplyDedents(p->tok);
        }
    }
    else {
        p->parsing_started = 1;
    }
    if (p->fill == p->size && _resize_tokens_array(p) < 0) {
        return -1;
    }
    return initialize_token(p, p->tokens[p->fill], token, type);
}

int
_PyPegen_fill_token(Parser *p)
{
    if (p->keyword_trial != NULL) {
        // A trial may only consume the cached input, including its ENDMARKER.
        return -1;
    }
    struct token new_token;
    _PyToken_Init(&new_token);
    _PyTokenizer_Get(p->tok, &new_token);

    // Record and skip '# type: ignore' comments
    while (new_token.type == TYPE_IGNORE) {
        Py_ssize_t len;
        const char *text = _PyToken_TextView(p->tok, &new_token, &len);
        char *tag = PyMem_Malloc((size_t)len + 1);
        if (tag == NULL) {
            PyErr_NoMemory();
            goto error;
        }
        memcpy(tag, text, (size_t)len);
        tag[len] = '\0';
        // Ownership of tag passes to the growable array
        if (!growable_comment_array_add(&p->type_ignore_comments, new_token.end_loc.lineno, tag)) {
            PyMem_Free(tag);
            PyErr_NoMemory();
            goto error;
        }
        _PyTokenizer_Get(p->tok, &new_token);
    }

    int type = new_token.type;
    // EOF can finish an ordinary token. Only a boundary token means that
    // the grammar requested input beyond the last physical line.
    if (type == ENDMARKER ||
        (type == DEDENT && (p->flags & PyPARSE_ALLOW_INCOMPLETE_INPUT) &&
         _PyTokenizer_GetStatus(p->tok) == E_EOF)) {
        p->input_exhausted = 1;
    }

    int result = _PyPegen_append_token(p, &new_token);
    _PyToken_Free(&new_token);
    return result;
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

void
_PyPegen_clear_memo_statistics(void)
{
    PyMutex_Lock(&_PyRuntime.parser.mutex);
    for (int i = 0; i < NSTATISTICS; i++) {
        memo_statistics[i] = 0;
    }
    PyMutex_Unlock(&_PyRuntime.parser.mutex);
}

PyObject *
_PyPegen_get_memo_statistics(void)
{
    PyObject *ret = PyList_New(NSTATISTICS);
    if (ret == NULL) {
        return NULL;
    }

    PyMutex_Lock(&_PyRuntime.parser.mutex);
    for (int i = 0; i < NSTATISTICS; i++) {
        PyObject *value = PyLong_FromLong(memo_statistics[i]);
        if (value == NULL) {
            PyMutex_Unlock(&_PyRuntime.parser.mutex);
            Py_DECREF(ret);
            return NULL;
        }
        // PyList_SetItem borrows a reference to value.
        if (PyList_SetItem(ret, i, value) < 0) {
            PyMutex_Unlock(&_PyRuntime.parser.mutex);
            Py_DECREF(ret);
            return NULL;
        }
    }
    PyMutex_Unlock(&_PyRuntime.parser.mutex);
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

    if (!(t->memo_mask & (1ULL << (type & 63)))) {
        return 0;
    }

    for (Memo *m = t->memo; m != NULL; m = m->next) {
        if (m->type == type) {
#if defined(Py_DEBUG)
            if (0 <= type && type < NSTATISTICS) {
                long count = m->mark - p->mark;
                // A memoized negative result counts for one.
                if (count <= 0) {
                    count = 1;
                }
                PyMutex_Lock(&_PyRuntime.parser.mutex);
                memo_statistics[type] += count;
                PyMutex_Unlock(&_PyRuntime.parser.mutex);
            }
#endif
            p->mark = m->mark;
            *(void **)(pres) = m->node;
            return 1;
        }
    }
    return 0;
}

#define LOOKAHEAD1(NAME, RES_TYPE)                                  \
    int                                                             \
    NAME (int positive, RES_TYPE (func)(Parser *), Parser *p)       \
    {                                                               \
        int mark = p->mark;                                         \
        void *res = func(p);                                        \
        p->mark = mark;                                             \
        return (res != NULL) == positive;                           \
    }

LOOKAHEAD1(_PyPegen_lookahead, void *)
LOOKAHEAD1(_PyPegen_lookahead_for_expr, expr_ty)
LOOKAHEAD1(_PyPegen_lookahead_for_stmt, stmt_ty)
#undef LOOKAHEAD1

#define LOOKAHEAD2(NAME, RES_TYPE, T)                                   \
    int                                                                 \
    NAME (int positive, RES_TYPE (func)(Parser *, T), Parser *p, T arg) \
    {                                                                   \
        int mark = p->mark;                                             \
        void *res = func(p, arg);                                       \
        p->mark = mark;                                                 \
        return (res != NULL) == positive;                               \
    }

LOOKAHEAD2(_PyPegen_lookahead_with_int, Token *, int)
LOOKAHEAD2(_PyPegen_lookahead_with_string, expr_ty, const char *)
#undef LOOKAHEAD2

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
        // Trials reuse identifiers already normalized by the original parse.
        // A new one would import or call Python code during speculation.
        if (p->keyword_trial != NULL) {
            Py_DECREF(id);
            goto error;
        }
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

// Return an arena-owned identifier; callers borrow the reference.
static PyObject *
get_cached_identifier(Parser *p, PyObject *bytes)
{
    const char *s = PyBytes_AsString(bytes);
    if (!s) {
        p->error_indicator = 1;
        return NULL;
    }
    // Identifiers repeat constantly; a small span-keyed cache skips the
    // UTF-8 decode + intern for repeated occurrences. Keys point into
    // arena-owned token bytes and values are arena-owned interned strings,
    // so borrowed references are valid for the lifetime of the parse
    // (including the second error pass, which reuses parser and arena).
    Py_ssize_t len = PyBytes_GET_SIZE(bytes);
    Py_hash_t hash = PyObject_Hash(bytes);
    if (hash == -1) {
        p->error_indicator = 1;
        return NULL;
    }
    // Parses without identifiers do not need this cache.
    if (p->identifier_cache == NULL) {
        p->identifier_cache = PyMem_Calloc(
            IDENTIFIER_CACHE_SIZE, sizeof(*p->identifier_cache));
        if (p->identifier_cache == NULL) {
            p->error_indicator = 1;
            return PyErr_NoMemory();
        }
    }
    IdentifierCacheEntry *free_slot = NULL;
    size_t idx = (size_t)hash & (IDENTIFIER_CACHE_SIZE - 1);
    for (int probe = 0; probe < IDENTIFIER_CACHE_MAX_PROBES; probe++) {
        IdentifierCacheEntry *entry = &p->identifier_cache[
            (idx + probe) & (IDENTIFIER_CACHE_SIZE - 1)];
        if (entry->key == NULL) {
            free_slot = entry;
            break;
        }
        if (entry->hash == hash && entry->len == len &&
            memcmp(entry->key, s, len) == 0)
        {
            return entry->value;
        }
    }
    PyObject *id = _PyPegen_new_identifier(p, s);
    if (id == NULL) {
        p->error_indicator = 1;
        return NULL;
    }
    if (free_slot != NULL) {
        free_slot->key = s;
        free_slot->len = len;
        free_slot->hash = hash;
        free_slot->value = id;
    }
    return id;
}

static expr_ty
_PyPegen_name_from_token(Parser *p, Token* t)
{
    if (t == NULL) {
        return NULL;
    }
    // Reuse the AST node when backtracking revisits this token. Memo lookup
    // starts before the token and restores the position after it on a hit.
    // Token kinds can be memo keys: generated grammar rule IDs start at 1000.
    int mark = p->mark - 1;
    p->mark = mark;
    expr_ty cached = NULL;
    if (_PyPegen_is_memoized(p, NAME, &cached)) {
        return cached;
    }
    p->mark = mark + 1;
    PyObject *id = get_cached_identifier(p, t->bytes);
    if (id == NULL) {
        return NULL;
    }
    expr_ty result = _PyAST_Name(id, Load, t->lineno, t->col_offset,
                                t->end_lineno, t->end_col_offset, p->arena);
    if (result != NULL && _PyPegen_insert_memo(p, mark, NAME, result) < 0) {
        p->error_indicator = 1;
        return NULL;
    }
    return result;
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
        if (strlen(*keyword) == (size_t)size &&
            strncmp(*keyword, the_token, (size_t)size) == 0) {
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
    int mark = p->mark;
    Token *t = _PyPegen_expect_token(p, NUMBER);
    if (t == NULL) {
        return NULL;
    }

    // Look up the node at the token's start, as for NAME tokens. A memo hit
    // advances the mark; on a miss, restore the position after the token.
    p->mark = mark;
    expr_ty cached = NULL;
    if (_PyPegen_is_memoized(p, NUMBER, &cached)) {
        return cached;
    }
    p->mark = mark + 1;

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

    expr_ty result = _PyAST_Constant(c, NULL, t->lineno, t->col_offset,
                                    t->end_lineno, t->end_col_offset, p->arena);
    if (result != NULL && _PyPegen_insert_memo(p, mark, NUMBER, result) < 0) {
        p->error_indicator = 1;
        return NULL;
    }
    return result;
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
                    int feature_version, int *errcode, PyArena *arena)
{
    Parser *p = PyMem_Malloc(sizeof(Parser));
    if (p == NULL) {
        return (Parser *) PyErr_NoMemory();
    }
    assert(tok != NULL);
    _PyTokenizer_SetOptions(tok, 0, (flags & PyPARSE_TYPE_COMMENTS) > 0);
    _PyTokenizer_AllowIncompleteInput(tok, flags & PyPARSE_ALLOW_INCOMPLETE_INPUT);
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
    p->type_ignore_comments = (growable_comment_array){0};

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
    p->identifier_cache = NULL;
    p->tstate = PyThreadState_Get();
    // Stack limits are initialized when the thread state is attached.
    assert(((_PyThreadStateImpl *)p->tstate)->c_stack_hard_limit != 0);
    p->stack_soft_limit = ((_PyThreadStateImpl *)p->tstate)->c_stack_soft_limit;
#if _Py_STACK_GROWS_DOWN
    p->stack_soft_limit += _PyOS_STACK_MARGIN_BYTES;
#else
    p->stack_soft_limit -= _PyOS_STACK_MARGIN_BYTES;
#endif
    p->level = 0;
    p->call_invalid_rules = 0;
    p->input_exhausted = 0;
    p->keyword_trial = NULL;
#ifdef Py_DEBUG
    p->debug = _Py_GetConfig()->parser_debug;
#endif
    return p;
}

void
_PyPegen_Parser_Free(Parser *p)
{
    PyMem_Free(p->identifier_cache);
    Py_XDECREF(p->normalize);
    // Resizes allocate blocks starting at indices 1, 2, 4, and so on.
    PyMem_Free(p->tokens[0]);
    for (int i = 1; i < p->size; i *= PEGEN_ARRAY_GROWTH_FACTOR) {
        PyMem_Free(p->tokens[i]);
    }
    PyMem_Free(p->tokens);
    growable_comment_array_deallocate(&p->type_ignore_comments);
    PyMem_Free(p);
}

static void
reset_parser_state_for_error_pass(Parser *p)
{
    for (int i = 0; i < p->fill; i++) {
        p->tokens[i]->memo = NULL;
        p->tokens[i]->memo_mask = 0;
    }
    p->mark = 0;
    p->flags &= ~PyPARSE_ALLOW_INCOMPLETE_INPUT;
    p->call_invalid_rules = 1;
    if (p->keyword_trial == NULL) {
        _PyTokenizer_AllowIncompleteInput(p->tok, 0);
        _PyTokenizer_StopInteractive(p->tok);
    }
}

typedef struct {
    int mark;
    const char *keyword;
    Py_ssize_t cost;
    int is_alias;
} KeywordTypo;

#define MAX_KEYWORD_TYPO_CANDIDATES 32
#define MAX_KEYWORD_TYPO_STEPS 100000

static int
compare_keyword_typos(const void *a, const void *b)
{
    const KeywordTypo *left = a, *right = b;
    if (left->cost != right->cost) {
        return left->cost < right->cost ? -1 : 1;
    }
    int order = strcmp(left->keyword, right->keyword);
    return order ? order : left->mark - right->mark;
}

static int
add_keyword_typo(KeywordTypo *candidates, int *count, int mark,
                 PyObject *name, const char *keyword, Py_ssize_t max_cost)
{
    PyObject *word = PyUnicode_FromString(keyword);
    if (word == NULL) {
        return -1;
    }
    if (PyUnicode_GET_LENGTH(name) <= 2) {
        max_cost = Py_MIN(max_cost, 2);
    }
    Py_ssize_t cost = _Py_UTF8_Edit_Cost(name, word, max_cost);
    Py_DECREF(word);
    if (cost < 0) {
        return -1;
    }
    if (cost == 0 || cost > max_cost) {
        return 0;
    }
    if (*count == MAX_KEYWORD_TYPO_CANDIDATES) {
        return -1;
    }
    candidates[(*count)++] = (KeywordTypo){mark, keyword, cost, 0};
    return 0;
}

static int
try_keyword_typo(Parser *p, KeywordTypo *candidate)
{
    // Token text belongs to the original arena. Token headers and memo entries
    // must be private: even a failed trial can memoize nodes containing its edit.
    Token *tokens = PyMem_Malloc(p->fill * sizeof(*tokens));
    Token **index = PyMem_Malloc(p->fill * sizeof(*index));
    IdentifierCacheEntry *identifiers = p->identifier_cache == NULL ? NULL :
        PyMem_Malloc(IDENTIFIER_CACHE_SIZE * sizeof(*identifiers));
    PyArena *arena = _PyArena_New();
    PyObject *replacement = PyBytes_FromString(candidate->keyword);
    if (tokens == NULL || index == NULL || arena == NULL || replacement == NULL ||
        (p->identifier_cache != NULL && identifiers == NULL))
    {
        PyMem_Free(tokens);
        PyMem_Free(index);
        PyMem_Free(identifiers);
        if (arena != NULL) {
            _PyArena_Free(arena);
        }
        Py_XDECREF(replacement);
        return -1;
    }
    for (int i = 0; i < p->fill; i++) {
        tokens[i] = *p->tokens[i];
        tokens[i].memo = NULL;
        tokens[i].memo_mask = 0;
        index[i] = &tokens[i];
    }
    if (identifiers != NULL) {
        memcpy(identifiers, p->identifier_cache,
               IDENTIFIER_CACHE_SIZE * sizeof(*identifiers));
    }
    Token *edited = &tokens[candidate->mark];
    edited->type = _get_keyword_or_name_type(
        p, candidate->keyword, strlen(candidate->keyword));
    edited->bytes = replacement;

    KeywordTrial state = {.budget = MAX_KEYWORD_TYPO_STEPS};
    Parser trial = *p;
    trial.tokens = index;
    trial.arena = arena;
    trial.size = trial.fill;
    trial.mark = 0;
    trial.level = 0;
    trial.error_indicator = 0;
    trial.call_invalid_rules = 0;
    trial.keyword_trial = &state;
    trial.known_err_token = NULL;
    trial.errcode = NULL;
    trial.identifier_cache = identifiers;
    trial.normalize = NULL;
    int accepted = _PyPegen_parse(&trial) != NULL &&
                   !trial.error_indicator && !PyErr_Occurred();
    for (int i = trial.mark; accepted && i < trial.fill; i++) {
        accepted = tokens[i].type == NEWLINE || tokens[i].type == ENDMARKER;
    }
    // Known foreign keywords may identify a valid header even when its body
    // contains another error (for example, "switch x:\n case:"). Require the
    // ordinary grammar to advance into a later line before accepting that hint.
    if (!accepted && !trial.error_indicator && candidate->is_alias &&
        p->feature_version == PY_MINOR_VERSION)
    {
        for (int i = candidate->mark; i < state.furthest; i++) {
            if (tokens[i].type == NEWLINE) {
                accepted = 1;
                break;
            }
        }
    }
    if (!accepted && !trial.error_indicator && !PyErr_Occurred()) {
        reset_parser_state_for_error_pass(&trial);
        _PyPegen_parse(&trial);
        // Version checks on an enclosing rule can run after its suite. For
        // older grammars, require a complete repair so those checks execute.
        accepted = state.incomplete_suite && p->feature_version == PY_MINOR_VERSION;
    }
    // No Parser_Free: the trial borrows the tokenizer and type-ignore comments,
    // and its token headers are a single allocation.
    PyMem_Free(trial.identifier_cache);
    _PyArena_Free(arena);
    Py_DECREF(replacement);
    PyMem_Free(index);
    PyMem_Free(tokens);
    return PyErr_Occurred() ? -1 : accepted;
}

static PyObject *
keyword_typo_metadata(Parser *p, PySyntaxErrorObject *error)
{
    // A real ENDMARKER is required even for an unfinished suite. A truncated
    // token cache is never evidence that a replacement is valid.
    if ((p->start_rule != Py_file_input && p->start_rule != Py_eval_input &&
         p->start_rule != Py_single_input) ||
        p->fill == 0 || p->fill > MAX_KEYWORD_TYPO_TOKENS ||
        p->tokens[p->fill - 1]->type != ENDMARKER ||
        _PyTokenizer_IsInteractive(p->tok) || !PyUnicode_Check(error->msg))
    {
        return NULL;
    }
    const char *message = PyUnicode_AsUTF8(error->msg);
    if (message == NULL || (strcmp(message, "invalid syntax") != 0 &&
                           strstr(message, "Perhaps you forgot a comma") == NULL))
    {
        return NULL;
    }
    // Prefer an existing missing-comma diagnostic unless the spelling differs
    // by only one edit (the shared edit-cost function charges two per edit).
    Py_ssize_t max_cost = strcmp(message, "invalid syntax") == 0 ? 4 : 2;
    long error_line = PyLong_AsLong(error->lineno);
    if (error_line < 0) {
        return NULL;
    }
    KeywordTypo candidates[MAX_KEYWORD_TYPO_CANDIDATES];
    int count = 0;
    int start = 0;
    for (int i = 0; i < p->fill && p->tokens[i]->lineno < error_line; i++) {
        if (p->tokens[i]->type == NEWLINE) {
            start = i + 1;
        }
    }
    static const struct { const char *name, *keyword; } aliases[] = {
        {"switch", "match"},
        {"delete", "del"},
        {"function", "def"},
        {"func", "def"},
        {"void", "def"},
    };
    for (int i = start; i < p->fill; i++) {
        Token *token = p->tokens[i];
        if (token->type != NAME || token->lineno > error_line ||
            PyBytes_GET_SIZE(token->bytes) < 2 ||
            PyBytes_GET_SIZE(token->bytes) > 10)
        {
            continue;
        }
        const char *alias = NULL;
        for (size_t j = 0; j < Py_ARRAY_LENGTH(aliases); j++) {
            if (strcmp(PyBytes_AS_STRING(token->bytes), aliases[j].name) == 0) {
                alias = aliases[j].keyword;
                break;
            }
        }
        if (alias != NULL) {
            if (count == MAX_KEYWORD_TYPO_CANDIDATES) {
                return NULL;
            }
            candidates[count++] = (KeywordTypo){i, alias, 0, 1};
            continue;
        }
        PyObject *name = PyUnicode_DecodeUTF8(
            PyBytes_AS_STRING(token->bytes), PyBytes_GET_SIZE(token->bytes), NULL);
        if (name == NULL) {
            return NULL;
        }
        int result = 0;
        for (int length = 0; length < p->n_keyword_lists && result == 0; length++) {
            for (KeywordToken *k = p->keywords[length];
                 k != NULL && k->type != -1 && result == 0; k++)
            {
                result = add_keyword_typo(candidates, &count, i, name, k->str, max_cost);
            }
        }
        for (char **k = p->soft_keywords; k != NULL && *k != NULL && result == 0; k++) {
            result = add_keyword_typo(candidates, &count, i, name, *k, max_cost);
        }
        Py_DECREF(name);
        if (result < 0) {
            return NULL;
        }
    }
    qsort(candidates, count, sizeof(*candidates), compare_keyword_typos);
    for (int i = 0; i < count; i++) {
        int accepted = try_keyword_typo(p, &candidates[i]);
        if (accepted < 0) {
            return NULL;
        }
        if (!accepted) {
            continue;
        }
        Token *token = p->tokens[candidates[i].mark];
        PyObject *line;
        if (_PyTokenizer_RetainedSource(p->tok) != NULL) {
            Py_ssize_t size;
            const char *text = _PyTokenizer_LineView(p->tok, token->lineno, &size);
            line = PyUnicode_DecodeUTF8(text, size, "replace");
        }
        else {
            _PyTokenizer_Info info = _PyTokenizer_GetInfo(p->tok);
            line = _PyErr_ProgramDecodedTextObject(
                info.filename, token->lineno, info.encoding);
        }
        if (line == NULL) {
            return NULL;
        }
        Py_ssize_t start = _PyPegen_byte_offset_to_character_offset(line, token->col_offset);
        Py_ssize_t end = _PyPegen_byte_offset_to_character_offset(line, token->end_col_offset);
        PyObject *metadata = NULL;
        if (start >= 0 && end >= 0) {
            metadata = Py_BuildValue("(sOinin)", candidates[i].keyword, line,
                                     token->lineno, start + 1,
                                     token->end_lineno, end + 1);
        }
        Py_DECREF(line);
        return metadata;
    }
    return NULL;
}

static void
_PyPegen_set_syntax_error_metadata(Parser *p) {
    PyObject *exc = PyErr_GetRaisedException();
    if (!exc || !PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_SyntaxError)) {
        PyErr_SetRaisedException(exc);
        return;
    }
    PySyntaxErrorObject *syntax_error = (PySyntaxErrorObject *)exc;
    PyObject *hint = keyword_typo_metadata(p, syntax_error);
    if (hint != NULL) {
        Py_XSETREF(syntax_error->metadata, hint);
        PyErr_SetRaisedException(exc);
        return;
    }
    // Suggestions must never replace the original exception, including when
    // a trial runs out of memory or exhausts its rule budget.
    PyErr_Clear();
    PyErr_SetRaisedException(exc);
}

void *
_PyPegen_run_parser(Parser *p)
{
    void *res = _PyPegen_parse(p);
    assert(p->level == 0);
    if (res != NULL && PyErr_Occurred()) {
        // Discard a result returned with an exception still pending
        // (e.g. a MemoryError from a recovered-from allocation failure).
        return NULL;
    }
    if (res == NULL) {
        if (PyErr_Occurred() &&
            (PyErr_ExceptionMatches(PyExc_IncompleteInputError) ||
             !PyErr_ExceptionMatches(PyExc_SyntaxError))) {
            return NULL;
        }
        if ((p->flags & PyPARSE_ALLOW_INCOMPLETE_INPUT) &&
            p->input_exhausted && !PyErr_Occurred()) {
            return _PyPegen_raise_incomplete_input(p);
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

        // Attach an optional, already-validated keyword hint.
        if (PyErr_ExceptionMatches(PyExc_SyntaxError)) {
            _PyPegen_set_syntax_error_metadata(p);
        }
       return NULL;
    }

    if (p->start_rule == Py_single_input && _PyTokenizer_HasTrailingStatement(p->tok)) {
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
    int parser_flags = compute_parser_flags(flags);
    struct tok_state *tok = _PyTokenizer_FromFile(fp, enc, ps1, ps2);
    if (tok == NULL) {
        if (PyErr_Occurred()) {
            _PyTokenizer_raise_init_error(filename_ob);
        }
        else {
            // The only silent tokenizer init failure is a failed allocation.
            PyErr_NoMemory();
        }
        return NULL;
    }
    tok->barry_as_bdfl = parser_flags & PyPARSE_BARRY_AS_BDFL;

    // From here on we need to clean up even if there's an error
    mod_ty result = NULL;

    PyObject *module = PyUnicode_FromString("__main__");
    if (module == NULL) {
        goto error;
    }
    _PyTokenizer_SetContext(tok, filename_ob, module);
    Py_DECREF(module);

    Parser *p = _PyPegen_Parser_New(tok, start_rule, parser_flags, PY_MINOR_VERSION,
                                    errcode, arena);
    if (p == NULL) {
        goto error;
    }

    result = _PyPegen_run_parser(p);
    _PyPegen_Parser_Free(p);

    const char *source = _PyTokenizer_RetainedSource(tok);
    if (source != NULL && result && interactive_src != NULL) {
        *interactive_src = PyUnicode_FromString(source);
        if (!*interactive_src || _PyArena_AddPyObject(arena, *interactive_src) < 0) {
            Py_XDECREF(*interactive_src);
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
                       PyCompilerFlags *flags, PyArena *arena, PyObject *module,
                       PyObject **incomplete)
{
    _PyTokenizer_Newline newline_mode = start_rule == Py_file_input
        ? _PyTokenizer_IMPLY_NEWLINE : _PyTokenizer_KEEP_NEWLINE;
    int parser_flags = compute_parser_flags(flags);
    if (parser_flags & PyPARSE_ALLOW_INCOMPLETE_INPUT) {
        newline_mode = _PyTokenizer_APPEND_NEWLINE;
    }

    struct tok_state *tok;
    if (flags != NULL && flags->cf_flags & PyCF_IGNORE_COOKIE) {
        tok = _PyTokenizer_FromUTF8(str, newline_mode, 0);
    } else {
        tok = _PyTokenizer_FromString(str, newline_mode, 0);
    }
    if (tok == NULL) {
        if (PyErr_Occurred()) {
            _PyTokenizer_raise_init_error(filename_ob);
        }
        else {
            // The only silent tokenizer init failure is a failed allocation.
            PyErr_NoMemory();
        }
        return NULL;
    }
    tok->barry_as_bdfl = parser_flags & PyPARSE_BARRY_AS_BDFL;

    _PyTokenizer_SetContext(tok, filename_ob, module);

    // We need to clear up from here on
    mod_ty result = NULL;

    int feature_version = flags && (flags->cf_flags & PyCF_ONLY_AST) ?
        flags->cf_feature_version : PY_MINOR_VERSION;
    Parser *p = _PyPegen_Parser_New(tok, start_rule, parser_flags, feature_version,
                                    NULL, arena);
    if (p == NULL) {
        goto error;
    }

    result = _PyPegen_run_parser(p);
    if (result != NULL && (parser_flags & PyPARSE_ALLOW_INCOMPLETE_INPUT)) {
        int needs_newline = _PyTokenizer_GetNewlineRequirements(tok);
        if ((needs_newline & _PyTokenizer_NEWLINE_CONTINUATION) ||
            (start_rule != Py_file_input &&
             (needs_newline & _PyTokenizer_NEWLINE_INDENT)) ||
            (start_rule == Py_single_input && p->input_exhausted &&
             (needs_newline & _PyTokenizer_NEWLINE_STATEMENT))) {
            _PyPegen_raise_incomplete_input(p);
            if (incomplete != NULL && PyErr_ExceptionMatches(PyExc_IncompleteInputError)) {
                *incomplete = PyErr_GetRaisedException();
            }
            else {
                result = NULL;
            }
        }
    }
    _PyPegen_Parser_Free(p);

error:
    _PyTokenizer_Free(tok);
    return result;
}
