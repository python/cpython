#include <Python.h>
#include <errcode.h>
#include "../tokenizer.h"

#include "pegen.h"
#include "parse_string.h"

PyObject *
_PyPegen_new_type_comment(Parser *p, char *s)
{
    PyObject *res = PyUnicode_DecodeUTF8(s, strlen(s), NULL);
    if (res == NULL) {
        return NULL;
    }
    if (PyArena_AddPyObject(p->arena, res) < 0) {
        Py_DECREF(res);
        return NULL;
    }
    return res;
}

arg_ty
_PyPegen_add_type_comment_to_arg(Parser *p, arg_ty a, Token *tc)
{
    if (tc == NULL) {
        return a;
    }
    char *bytes = PyBytes_AsString(tc->bytes);
    if (bytes == NULL) {
        return NULL;
    }
    PyObject *tco = _PyPegen_new_type_comment(p, bytes);
    if (tco == NULL) {
        return NULL;
    }
    return arg(a->arg, a->annotation, tco,
               a->lineno, a->col_offset, a->end_lineno, a->end_col_offset,
               p->arena);
}

static int
init_normalization(Parser *p)
{
    if (p->normalize) {
        return 1;
    }
    PyObject *m = PyImport_ImportModuleNoBlock("unicodedata");
    if (!m)
    {
        return 0;
    }
    p->normalize = PyObject_GetAttrString(m, "normalize");
    Py_DECREF(m);
    if (!p->normalize)
    {
        return 0;
    }
    return 1;
}

/* Checks if the NOTEQUAL token is valid given the current parser flags
0 indicates success and nonzero indicates failure (an exception may be set) */
int
_PyPegen_check_barry_as_flufl(Parser *p) {
    Token *t = p->tokens[p->fill - 1];
    assert(t->bytes != NULL);
    assert(t->type == NOTEQUAL);

    char* tok_str = PyBytes_AS_STRING(t->bytes);
    if (p->flags & PyPARSE_BARRY_AS_BDFL && strcmp(tok_str, "<>")){
        RAISE_SYNTAX_ERROR("with Barry as BDFL, use '<>' instead of '!='");
        return -1;
    } else if (!(p->flags & PyPARSE_BARRY_AS_BDFL)) {
        return strcmp(tok_str, "!=");
    }
    return 0;
}

PyObject *
_PyPegen_new_identifier(Parser *p, char *n)
{
    PyObject *id = PyUnicode_DecodeUTF8(n, strlen(n), NULL);
    if (!id) {
        goto error;
    }
    /* PyUnicode_DecodeUTF8 should always return a ready string. */
    assert(PyUnicode_IS_READY(id));
    /* Check whether there are non-ASCII characters in the
       identifier; if so, normalize to NFKC. */
    if (!PyUnicode_IS_ASCII(id))
    {
        PyObject *id2;
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
        id2 = _PyObject_FastCall(p->normalize, args, 2);
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
    PyUnicode_InternInPlace(&id);
    if (PyArena_AddPyObject(p->arena, id) < 0)
    {
        Py_DECREF(id);
        goto error;
    }
    return id;

error:
    p->error_indicator = 1;
    return NULL;
}

static PyObject *
_create_dummy_identifier(Parser *p)
{
    return _PyPegen_new_identifier(p, "");
}

static inline Py_ssize_t
byte_offset_to_character_offset(PyObject *line, int col_offset)
{
    const char *str = PyUnicode_AsUTF8(line);
    if (!str) {
        return 0;
    }
    PyObject *text = PyUnicode_DecodeUTF8(str, col_offset, "replace");
    if (!text) {
        return 0;
    }
    Py_ssize_t size = PyUnicode_GET_LENGTH(text);
    str = PyUnicode_AsUTF8(text);
    if (str != NULL && (int)strlen(str) == col_offset) {
        size = strlen(str);
    }
    Py_DECREF(text);
    return size;
}

const char *
_PyPegen_get_expr_name(expr_ty e)
{
    switch (e->kind) {
        case Attribute_kind:
            return "attribute";
        case Subscript_kind:
            return "subscript";
        case Starred_kind:
            return "starred";
        case Name_kind:
            return "name";
        case List_kind:
            return "list";
        case Tuple_kind:
            return "tuple";
        case Lambda_kind:
            return "lambda";
        case Call_kind:
            return "function call";
        case BoolOp_kind:
        case BinOp_kind:
        case UnaryOp_kind:
            return "operator";
        case GeneratorExp_kind:
            return "generator expression";
        case Yield_kind:
        case YieldFrom_kind:
            return "yield expression";
        case Await_kind:
            return "await expression";
        case ListComp_kind:
            return "list comprehension";
        case SetComp_kind:
            return "set comprehension";
        case DictComp_kind:
            return "dict comprehension";
        case Dict_kind:
            return "dict display";
        case Set_kind:
            return "set display";
        case JoinedStr_kind:
        case FormattedValue_kind:
            return "f-string expression";
        case Constant_kind: {
            PyObject *value = e->v.Constant.value;
            if (value == Py_None) {
                return "None";
            }
            if (value == Py_False) {
                return "False";
            }
            if (value == Py_True) {
                return "True";
            }
            if (value == Py_Ellipsis) {
                return "Ellipsis";
            }
            return "literal";
        }
        case Compare_kind:
            return "comparison";
        case IfExp_kind:
            return "conditional expression";
        case NamedExpr_kind:
            return "named expression";
        default:
            PyErr_Format(PyExc_SystemError,
                         "unexpected expression in assignment %d (line %d)",
                         e->kind, e->lineno);
            return NULL;
    }
}

static int
raise_decode_error(Parser *p)
{
    assert(PyErr_Occurred());
    const char *errtype = NULL;
    if (PyErr_ExceptionMatches(PyExc_UnicodeError)) {
        errtype = "unicode error";
    }
    else if (PyErr_ExceptionMatches(PyExc_ValueError)) {
        errtype = "value error";
    }
    if (errtype) {
        PyObject *type, *value, *tback, *errstr;
        PyErr_Fetch(&type, &value, &tback);
        errstr = PyObject_Str(value);
        if (errstr) {
            RAISE_SYNTAX_ERROR("(%s) %U", errtype, errstr);
            Py_DECREF(errstr);
        }
        else {
            PyErr_Clear();
            RAISE_SYNTAX_ERROR("(%s) unknown error", errtype);
        }
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(tback);
    }

    return -1;
}

static void
raise_tokenizer_init_error(PyObject *filename)
{
    if (!(PyErr_ExceptionMatches(PyExc_LookupError)
          || PyErr_ExceptionMatches(PyExc_ValueError)
          || PyErr_ExceptionMatches(PyExc_UnicodeDecodeError))) {
        return;
    }
    PyObject *errstr = NULL;
    PyObject *tuple = NULL;
    PyObject *type, *value, *tback;
    PyErr_Fetch(&type, &value, &tback);
    errstr = PyObject_Str(value);
    if (!errstr) {
        goto error;
    }

    PyObject *tmp = Py_BuildValue("(OiiO)", filename, 0, -1, Py_None);
    if (!tmp) {
        goto error;
    }

    tuple = PyTuple_Pack(2, errstr, tmp);
    Py_DECREF(tmp);
    if (!value) {
        goto error;
    }
    PyErr_SetObject(PyExc_SyntaxError, tuple);

error:
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(tback);
    Py_XDECREF(errstr);
    Py_XDECREF(tuple);
}

static int
tokenizer_error(Parser *p)
{
    if (PyErr_Occurred()) {
        return -1;
    }

    const char *msg = NULL;
    PyObject* errtype = PyExc_SyntaxError;
    switch (p->tok->done) {
        case E_TOKEN:
            msg = "invalid token";
            break;
        case E_EOFS:
            RAISE_SYNTAX_ERROR("EOF while scanning triple-quoted string literal");
            return -1;
        case E_EOLS:
            RAISE_SYNTAX_ERROR("EOL while scanning string literal");
            return -1;
        case E_EOF:
            RAISE_SYNTAX_ERROR("unexpected EOF while parsing");
            return -1;
        case E_DEDENT:
            RAISE_INDENTATION_ERROR("unindent does not match any outer indentation level");
            return -1;
        case E_INTR:
            if (!PyErr_Occurred()) {
                PyErr_SetNone(PyExc_KeyboardInterrupt);
            }
            return -1;
        case E_NOMEM:
            PyErr_NoMemory();
            return -1;
        case E_TABSPACE:
            errtype = PyExc_TabError;
            msg = "inconsistent use of tabs and spaces in indentation";
            break;
        case E_TOODEEP:
            errtype = PyExc_IndentationError;
            msg = "too many levels of indentation";
            break;
        case E_LINECONT:
            msg = "unexpected character after line continuation character";
            break;
        default:
            msg = "unknown parsing error";
    }

    PyErr_Format(errtype, msg);
    // There is no reliable column information for this error
    PyErr_SyntaxLocationObject(p->tok->filename, p->tok->lineno, 0);

    return -1;
}

void *
_PyPegen_raise_error(Parser *p, PyObject *errtype, const char *errmsg, ...)
{
    Token *t = p->known_err_token != NULL ? p->known_err_token : p->tokens[p->fill - 1];
    int col_offset;
    if (t->col_offset == -1) {
        col_offset = Py_SAFE_DOWNCAST(p->tok->cur - p->tok->buf,
                                      intptr_t, int);
    } else {
        col_offset = t->col_offset + 1;
    }

    va_list va;
    va_start(va, errmsg);
    _PyPegen_raise_error_known_location(p, errtype, t->lineno,
                                        col_offset, errmsg, va);
    va_end(va);

    return NULL;
}


void *
_PyPegen_raise_error_known_location(Parser *p, PyObject *errtype,
                                    int lineno, int col_offset,
                                    const char *errmsg, va_list va)
{
    PyObject *value = NULL;
    PyObject *errstr = NULL;
    PyObject *error_line = NULL;
    PyObject *tmp = NULL;
    p->error_indicator = 1;

    errstr = PyUnicode_FromFormatV(errmsg, va);
    if (!errstr) {
        goto error;
    }

    if (p->start_rule == Py_file_input) {
        error_line = PyErr_ProgramTextObject(p->tok->filename, lineno);
    }

    if (!error_line) {
        Py_ssize_t size = p->tok->inp - p->tok->buf;
        if (size && p->tok->buf[size-1] == '\n') {
            size--;
        }
        error_line = PyUnicode_DecodeUTF8(p->tok->buf, size, "replace");
        if (!error_line) {
            goto error;
        }
    }

    Py_ssize_t col_number = byte_offset_to_character_offset(error_line, col_offset);

    tmp = Py_BuildValue("(OiiN)", p->tok->filename, lineno, col_number, error_line);
    if (!tmp) {
        goto error;
    }
    value = PyTuple_Pack(2, errstr, tmp);
    Py_DECREF(tmp);
    if (!value) {
        goto error;
    }
    PyErr_SetObject(errtype, value);

    Py_DECREF(errstr);
    Py_DECREF(value);
    return NULL;

error:
    Py_XDECREF(errstr);
    Py_XDECREF(error_line);
    return NULL;
}

#if 0
static const char *
token_name(int type)
{
    if (0 <= type && type <= N_TOKENS) {
        return _PyParser_TokenNames[type];
    }
    return "<Huh?>";
}
#endif

// Here, mark is the start of the node, while p->mark is the end.
// If node==NULL, they should be the same.
int
_PyPegen_insert_memo(Parser *p, int mark, int type, void *node)
{
    // Insert in front
    Memo *m = PyArena_Malloc(p->arena, sizeof(Memo));
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

// Return dummy NAME.
void *
_PyPegen_dummy_name(Parser *p, ...)
{
    static void *cache = NULL;

    if (cache != NULL) {
        return cache;
    }

    PyObject *id = _create_dummy_identifier(p);
    if (!id) {
        return NULL;
    }
    cache = Name(id, Load, 1, 0, 1, 0, p->arena);
    return cache;
}

static int
_get_keyword_or_name_type(Parser *p, const char *name, int name_len)
{
    if (name_len >= p->n_keyword_lists || p->keywords[name_len] == NULL) {
        return NAME;
    }
    for (KeywordToken *k = p->keywords[name_len]; k->type != -1; k++) {
        if (strncmp(k->str, name, name_len) == 0) {
            return k->type;
        }
    }
    return NAME;
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

int
_PyPegen_fill_token(Parser *p)
{
    const char *start, *end;
    int type = PyTokenizer_Get(p->tok, &start, &end);

    // Record and skip '# type: ignore' comments
    while (type == TYPE_IGNORE) {
        Py_ssize_t len = end - start;
        char *tag = PyMem_Malloc(len + 1);
        if (tag == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        strncpy(tag, start, len);
        tag[len] = '\0';
        // Ownership of tag passes to the growable array
        if (!growable_comment_array_add(&p->type_ignore_comments, p->tok->lineno, tag)) {
            PyErr_NoMemory();
            return -1;
        }
        type = PyTokenizer_Get(p->tok, &start, &end);
    }

    if (type == ENDMARKER && p->start_rule == Py_single_input && p->parsing_started) {
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

    if (p->fill == p->size) {
        int newsize = p->size * 2;
        Token **new_tokens = PyMem_Realloc(p->tokens, newsize * sizeof(Token *));
        if (new_tokens == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        else {
            p->tokens = new_tokens;
        }
        for (int i = p->size; i < newsize; i++) {
            p->tokens[i] = PyMem_Malloc(sizeof(Token));
            if (p->tokens[i] == NULL) {
                p->size = i; // Needed, in order to cleanup correctly after parser fails
                PyErr_NoMemory();
                return -1;
            }
            memset(p->tokens[i], '\0', sizeof(Token));
        }
        p->size = newsize;
    }

    Token *t = p->tokens[p->fill];
    t->type = (type == NAME) ? _get_keyword_or_name_type(p, start, (int)(end - start)) : type;
    t->bytes = PyBytes_FromStringAndSize(start, end - start);
    if (t->bytes == NULL) {
        return -1;
    }
    PyArena_AddPyObject(p->arena, t->bytes);

    int lineno = type == STRING ? p->tok->first_lineno : p->tok->lineno;
    const char *line_start = type == STRING ? p->tok->multi_line_start : p->tok->line_start;
    int end_lineno = p->tok->lineno;
    int col_offset = -1, end_col_offset = -1;
    if (start != NULL && start >= line_start) {
        col_offset = (int)(start - line_start);
    }
    if (end != NULL && end >= p->tok->line_start) {
        end_col_offset = (int)(end - p->tok->line_start);
    }

    t->lineno = p->starting_lineno + lineno;
    t->col_offset = p->tok->lineno == 1 ? p->starting_col_offset + col_offset : col_offset;
    t->end_lineno = p->starting_lineno + end_lineno;
    t->end_col_offset = p->tok->lineno == 1 ? p->starting_col_offset + end_col_offset : end_col_offset;

    p->fill += 1;

    if (type == ERRORTOKEN) {
        if (p->tok->done == E_DECODE) {
            return raise_decode_error(p);
        }
        else {
            return tokenizer_error(p);
        }
    }

    return 0;
}

// Instrumentation to count the effectiveness of memoization.
// The array counts the number of tokens skipped by memoization,
// indexed by type.

#define NSTATISTICS 2000
static long memo_statistics[NSTATISTICS];

void
_PyPegen_clear_memo_statistics()
{
    for (int i = 0; i < NSTATISTICS; i++) {
        memo_statistics[i] = 0;
    }
}

PyObject *
_PyPegen_get_memo_statistics()
{
    PyObject *ret = PyList_New(NSTATISTICS);
    if (ret == NULL) {
        return NULL;
    }
    for (int i = 0; i < NSTATISTICS; i++) {
        PyObject *value = PyLong_FromLong(memo_statistics[i]);
        if (value == NULL) {
            Py_DECREF(ret);
            return NULL;
        }
        // PyList_SetItem borrows a reference to value.
        if (PyList_SetItem(ret, i, value) < 0) {
            Py_DECREF(ret);
            return NULL;
        }
    }
    return ret;
}

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
            if (0 <= type && type < NSTATISTICS) {
                long count = m->mark - p->mark;
                // A memoized negative result counts for one.
                if (count <= 0) {
                    count = 1;
                }
                memo_statistics[type] += count;
            }
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
_PyPegen_lookahead_with_int(int positive, Token *(func)(Parser *, int), Parser *p, int arg)
{
    int mark = p->mark;
    void *res = func(p, arg);
    p->mark = mark;
    return (res != NULL) == positive;
}

int
_PyPegen_lookahead(int positive, void *(func)(Parser *), Parser *p)
{
    int mark = p->mark;
    void *res = (void*)func(p);
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

expr_ty
_PyPegen_name_token(Parser *p)
{
    Token *t = _PyPegen_expect_token(p, NAME);
    if (t == NULL) {
        return NULL;
    }
    char* s = PyBytes_AsString(t->bytes);
    if (!s) {
        return NULL;
    }
    PyObject *id = _PyPegen_new_identifier(p, s);
    if (id == NULL) {
        return NULL;
    }
    return Name(id, Load, t->lineno, t->col_offset, t->end_lineno, t->end_col_offset,
                p->arena);
}

void *
_PyPegen_string_token(Parser *p)
{
    return _PyPegen_expect_token(p, STRING);
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
    else
        x = PyOS_strtol(s, (char **)&end, 0);
    if (*end == '\0') {
        if (errno != 0)
            return PyLong_FromString(s, (char **)0, 0);
        return PyLong_FromLong(x);
    }
    /* XXX Huge floats may silently fail */
    if (imflag) {
        compl.real = 0.;
        compl.imag = PyOS_string_to_double(s, (char **)&end, NULL);
        if (compl.imag == -1.0 && PyErr_Occurred())
            return NULL;
        return PyComplex_FromCComplex(compl);
    }
    else {
        dx = PyOS_string_to_double(s, NULL, NULL);
        if (dx == -1.0 && PyErr_Occurred())
            return NULL;
        return PyFloat_FromDouble(dx);
    }
}

static PyObject *
parsenumber(const char *s)
{
    char *dup, *end;
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

    char *num_raw = PyBytes_AsString(t->bytes);
    if (num_raw == NULL) {
        return NULL;
    }

    if (p->feature_version < 6 && strchr(num_raw, '_') != NULL) {
        p->error_indicator = 1;
        return RAISE_SYNTAX_ERROR("Underscores in numeric literals are only supported "
                                  "in Python 3.6 and greater");
    }

    PyObject *c = parsenumber(num_raw);

    if (c == NULL) {
        return NULL;
    }

    if (PyArena_AddPyObject(p->arena, c) < 0) {
        Py_DECREF(c);
        return NULL;
    }

    return Constant(c, NULL, t->lineno, t->col_offset, t->end_lineno, t->end_col_offset,
                    p->arena);
}

static int // bool
newline_in_string(Parser *p, const char *cur)
{
    for (char c = *cur; cur >= p->tok->buf; c = *--cur) {
        if (c == '\'' || c == '"') {
            return 1;
        }
    }
    return 0;
}

/* Check that the source for a single input statement really is a single
   statement by looking at what is left in the buffer after parsing.
   Trailing whitespace and comments are OK. */
static int // bool
bad_single_statement(Parser *p)
{
    const char *cur = strchr(p->tok->buf, '\n');

    /* Newlines are allowed if preceded by a line continuation character
       or if they appear inside a string. */
    if (!cur || *(cur - 1) == '\\' || newline_in_string(p, cur)) {
        return 0;
    }
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
    if (flags->cf_feature_version < 7) {
        parser_flags |= PyPARSE_ASYNC_HACKS;
    }
    return parser_flags;
}

Parser *
_PyPegen_Parser_New(struct tok_state *tok, int start_rule, int flags,
                    int feature_version, int *errcode, PyArena *arena)
{
    Parser *p = PyMem_Malloc(sizeof(Parser));
    if (p == NULL) {
        return (Parser *) PyErr_NoMemory();
    }
    assert(tok != NULL);
    tok->type_comments = (flags & PyPARSE_TYPE_COMMENTS) > 0;
    tok->async_hacks = (flags & PyPARSE_ASYNC_HACKS) > 0;
    p->tok = tok;
    p->keywords = NULL;
    p->n_keyword_lists = -1;
    p->tokens = PyMem_Malloc(sizeof(Token *));
    if (!p->tokens) {
        PyMem_Free(p);
        return (Parser *) PyErr_NoMemory();
    }
    p->tokens[0] = PyMem_Calloc(1, sizeof(Token));
    if (!p->tokens) {
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

    return p;
}

void *
_PyPegen_run_parser(Parser *p)
{
    void *res = _PyPegen_parse(p);
    if (res == NULL) {
        if (PyErr_Occurred()) {
            return NULL;
        }
        if (p->fill == 0) {
            RAISE_SYNTAX_ERROR("error at start before reading any input");
        }
        else if (p->tok->done == E_EOF) {
            RAISE_SYNTAX_ERROR("unexpected EOF while parsing");
        }
        else {
            if (p->tokens[p->fill-1]->type == INDENT) {
                RAISE_INDENTATION_ERROR("unexpected indent");
            }
            else if (p->tokens[p->fill-1]->type == DEDENT) {
                RAISE_INDENTATION_ERROR("unexpected unindent");
            }
            else {
                RAISE_SYNTAX_ERROR("invalid syntax");
            }
        }
        return NULL;
    }

    if (p->start_rule == Py_single_input && bad_single_statement(p)) {
        p->tok->done = E_BADSINGLE; // This is not necessary for now, but might be in the future
        return RAISE_SYNTAX_ERROR("multiple statements found while compiling a single statement");
    }

    return res;
}

mod_ty
_PyPegen_run_parser_from_file_pointer(FILE *fp, int start_rule, PyObject *filename_ob,
                             const char *enc, const char *ps1, const char *ps2,
                             PyCompilerFlags *flags, int *errcode, PyArena *arena)
{
    struct tok_state *tok = PyTokenizer_FromFile(fp, enc, ps1, ps2);
    if (tok == NULL) {
        if (PyErr_Occurred()) {
            raise_tokenizer_init_error(filename_ob);
            return NULL;
        }
        return NULL;
    }
    // This transfers the ownership to the tokenizer
    tok->filename = filename_ob;
    Py_INCREF(filename_ob);

    // From here on we need to clean up even if there's an error
    mod_ty result = NULL;

    int parser_flags = compute_parser_flags(flags);
    Parser *p = _PyPegen_Parser_New(tok, start_rule, parser_flags, PY_MINOR_VERSION,
                                    errcode, arena);
    if (p == NULL) {
        goto error;
    }

    result = _PyPegen_run_parser(p);
    _PyPegen_Parser_Free(p);

error:
    PyTokenizer_Free(tok);
    return result;
}

mod_ty
_PyPegen_run_parser_from_file(const char *filename, int start_rule,
                     PyObject *filename_ob, PyCompilerFlags *flags, PyArena *arena)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename);
        return NULL;
    }

    mod_ty result = _PyPegen_run_parser_from_file_pointer(fp, start_rule, filename_ob,
                                                 NULL, NULL, NULL, flags, NULL, arena);

    fclose(fp);
    return result;
}

mod_ty
_PyPegen_run_parser_from_string(const char *str, int start_rule, PyObject *filename_ob,
                       PyCompilerFlags *flags, PyArena *arena)
{
    int exec_input = start_rule == Py_file_input;

    struct tok_state *tok;
    if (flags == NULL || flags->cf_flags & PyCF_IGNORE_COOKIE) {
        tok = PyTokenizer_FromUTF8(str, exec_input);
    } else {
        tok = PyTokenizer_FromString(str, exec_input);
    }
    if (tok == NULL) {
        if (PyErr_Occurred()) {
            raise_tokenizer_init_error(filename_ob);
        }
        return NULL;
    }
    // This transfers the ownership to the tokenizer
    tok->filename = filename_ob;
    Py_INCREF(filename_ob);

    // We need to clear up from here on
    mod_ty result = NULL;

    int parser_flags = compute_parser_flags(flags);
    int feature_version = flags ? flags->cf_feature_version : PY_MINOR_VERSION;
    Parser *p = _PyPegen_Parser_New(tok, start_rule, parser_flags, feature_version,
                                    NULL, arena);
    if (p == NULL) {
        goto error;
    }

    result = _PyPegen_run_parser(p);
    _PyPegen_Parser_Free(p);

error:
    PyTokenizer_Free(tok);
    return result;
}

void *
_PyPegen_interactive_exit(Parser *p)
{
    if (p->errcode) {
        *(p->errcode) = E_EOF;
    }
    return NULL;
}

/* Creates a single-element asdl_seq* that contains a */
asdl_seq *
_PyPegen_singleton_seq(Parser *p, void *a)
{
    assert(a != NULL);
    asdl_seq *seq = _Py_asdl_seq_new(1, p->arena);
    if (!seq) {
        return NULL;
    }
    asdl_seq_SET(seq, 0, a);
    return seq;
}

/* Creates a copy of seq and prepends a to it */
asdl_seq *
_PyPegen_seq_insert_in_front(Parser *p, void *a, asdl_seq *seq)
{
    assert(a != NULL);
    if (!seq) {
        return _PyPegen_singleton_seq(p, a);
    }

    asdl_seq *new_seq = _Py_asdl_seq_new(asdl_seq_LEN(seq) + 1, p->arena);
    if (!new_seq) {
        return NULL;
    }

    asdl_seq_SET(new_seq, 0, a);
    for (Py_ssize_t i = 1, l = asdl_seq_LEN(new_seq); i < l; i++) {
        asdl_seq_SET(new_seq, i, asdl_seq_GET(seq, i - 1));
    }
    return new_seq;
}

/* Creates a copy of seq and appends a to it */
asdl_seq *
_PyPegen_seq_append_to_end(Parser *p, asdl_seq *seq, void *a)
{
    assert(a != NULL);
    if (!seq) {
        return _PyPegen_singleton_seq(p, a);
    }

    asdl_seq *new_seq = _Py_asdl_seq_new(asdl_seq_LEN(seq) + 1, p->arena);
    if (!new_seq) {
        return NULL;
    }

    for (Py_ssize_t i = 0, l = asdl_seq_LEN(new_seq); i + 1 < l; i++) {
        asdl_seq_SET(new_seq, i, asdl_seq_GET(seq, i));
    }
    asdl_seq_SET(new_seq, asdl_seq_LEN(new_seq) - 1, a);
    return new_seq;
}

static Py_ssize_t
_get_flattened_seq_size(asdl_seq *seqs)
{
    Py_ssize_t size = 0;
    for (Py_ssize_t i = 0, l = asdl_seq_LEN(seqs); i < l; i++) {
        asdl_seq *inner_seq = asdl_seq_GET(seqs, i);
        size += asdl_seq_LEN(inner_seq);
    }
    return size;
}

/* Flattens an asdl_seq* of asdl_seq*s */
asdl_seq *
_PyPegen_seq_flatten(Parser *p, asdl_seq *seqs)
{
    Py_ssize_t flattened_seq_size = _get_flattened_seq_size(seqs);
    assert(flattened_seq_size > 0);

    asdl_seq *flattened_seq = _Py_asdl_seq_new(flattened_seq_size, p->arena);
    if (!flattened_seq) {
        return NULL;
    }

    int flattened_seq_idx = 0;
    for (Py_ssize_t i = 0, l = asdl_seq_LEN(seqs); i < l; i++) {
        asdl_seq *inner_seq = asdl_seq_GET(seqs, i);
        for (Py_ssize_t j = 0, li = asdl_seq_LEN(inner_seq); j < li; j++) {
            asdl_seq_SET(flattened_seq, flattened_seq_idx++, asdl_seq_GET(inner_seq, j));
        }
    }
    assert(flattened_seq_idx == flattened_seq_size);

    return flattened_seq;
}

/* Creates a new name of the form <first_name>.<second_name> */
expr_ty
_PyPegen_join_names_with_dot(Parser *p, expr_ty first_name, expr_ty second_name)
{
    assert(first_name != NULL && second_name != NULL);
    PyObject *first_identifier = first_name->v.Name.id;
    PyObject *second_identifier = second_name->v.Name.id;

    if (PyUnicode_READY(first_identifier) == -1) {
        return NULL;
    }
    if (PyUnicode_READY(second_identifier) == -1) {
        return NULL;
    }
    const char *first_str = PyUnicode_AsUTF8(first_identifier);
    if (!first_str) {
        return NULL;
    }
    const char *second_str = PyUnicode_AsUTF8(second_identifier);
    if (!second_str) {
        return NULL;
    }
    Py_ssize_t len = strlen(first_str) + strlen(second_str) + 1;  // +1 for the dot

    PyObject *str = PyBytes_FromStringAndSize(NULL, len);
    if (!str) {
        return NULL;
    }

    char *s = PyBytes_AS_STRING(str);
    if (!s) {
        return NULL;
    }

    strcpy(s, first_str);
    s += strlen(first_str);
    *s++ = '.';
    strcpy(s, second_str);
    s += strlen(second_str);
    *s = '\0';

    PyObject *uni = PyUnicode_DecodeUTF8(PyBytes_AS_STRING(str), PyBytes_GET_SIZE(str), NULL);
    Py_DECREF(str);
    if (!uni) {
        return NULL;
    }
    PyUnicode_InternInPlace(&uni);
    if (PyArena_AddPyObject(p->arena, uni) < 0) {
        Py_DECREF(uni);
        return NULL;
    }

    return _Py_Name(uni, Load, EXTRA_EXPR(first_name, second_name));
}

/* Counts the total number of dots in seq's tokens */
int
_PyPegen_seq_count_dots(asdl_seq *seq)
{
    int number_of_dots = 0;
    for (Py_ssize_t i = 0, l = asdl_seq_LEN(seq); i < l; i++) {
        Token *current_expr = asdl_seq_GET(seq, i);
        switch (current_expr->type) {
            case ELLIPSIS:
                number_of_dots += 3;
                break;
            case DOT:
                number_of_dots += 1;
                break;
            default:
                Py_UNREACHABLE();
        }
    }

    return number_of_dots;
}

/* Creates an alias with '*' as the identifier name */
alias_ty
_PyPegen_alias_for_star(Parser *p)
{
    PyObject *str = PyUnicode_InternFromString("*");
    if (!str) {
        return NULL;
    }
    if (PyArena_AddPyObject(p->arena, str) < 0) {
        Py_DECREF(str);
        return NULL;
    }
    return alias(str, NULL, p->arena);
}

/* Creates a new asdl_seq* with the identifiers of all the names in seq */
asdl_seq *
_PyPegen_map_names_to_ids(Parser *p, asdl_seq *seq)
{
    Py_ssize_t len = asdl_seq_LEN(seq);
    assert(len > 0);

    asdl_seq *new_seq = _Py_asdl_seq_new(len, p->arena);
    if (!new_seq) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        expr_ty e = asdl_seq_GET(seq, i);
        asdl_seq_SET(new_seq, i, e->v.Name.id);
    }
    return new_seq;
}

/* Constructs a CmpopExprPair */
CmpopExprPair *
_PyPegen_cmpop_expr_pair(Parser *p, cmpop_ty cmpop, expr_ty expr)
{
    assert(expr != NULL);
    CmpopExprPair *a = PyArena_Malloc(p->arena, sizeof(CmpopExprPair));
    if (!a) {
        return NULL;
    }
    a->cmpop = cmpop;
    a->expr = expr;
    return a;
}

asdl_int_seq *
_PyPegen_get_cmpops(Parser *p, asdl_seq *seq)
{
    Py_ssize_t len = asdl_seq_LEN(seq);
    assert(len > 0);

    asdl_int_seq *new_seq = _Py_asdl_int_seq_new(len, p->arena);
    if (!new_seq) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        CmpopExprPair *pair = asdl_seq_GET(seq, i);
        asdl_seq_SET(new_seq, i, pair->cmpop);
    }
    return new_seq;
}

asdl_seq *
_PyPegen_get_exprs(Parser *p, asdl_seq *seq)
{
    Py_ssize_t len = asdl_seq_LEN(seq);
    assert(len > 0);

    asdl_seq *new_seq = _Py_asdl_seq_new(len, p->arena);
    if (!new_seq) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        CmpopExprPair *pair = asdl_seq_GET(seq, i);
        asdl_seq_SET(new_seq, i, pair->expr);
    }
    return new_seq;
}

/* Creates an asdl_seq* where all the elements have been changed to have ctx as context */
static asdl_seq *
_set_seq_context(Parser *p, asdl_seq *seq, expr_context_ty ctx)
{
    Py_ssize_t len = asdl_seq_LEN(seq);
    if (len == 0) {
        return NULL;
    }

    asdl_seq *new_seq = _Py_asdl_seq_new(len, p->arena);
    if (!new_seq) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        expr_ty e = asdl_seq_GET(seq, i);
        asdl_seq_SET(new_seq, i, _PyPegen_set_expr_context(p, e, ctx));
    }
    return new_seq;
}

static expr_ty
_set_name_context(Parser *p, expr_ty e, expr_context_ty ctx)
{
    return _Py_Name(e->v.Name.id, ctx, EXTRA_EXPR(e, e));
}

static expr_ty
_set_tuple_context(Parser *p, expr_ty e, expr_context_ty ctx)
{
    return _Py_Tuple(_set_seq_context(p, e->v.Tuple.elts, ctx), ctx, EXTRA_EXPR(e, e));
}

static expr_ty
_set_list_context(Parser *p, expr_ty e, expr_context_ty ctx)
{
    return _Py_List(_set_seq_context(p, e->v.List.elts, ctx), ctx, EXTRA_EXPR(e, e));
}

static expr_ty
_set_subscript_context(Parser *p, expr_ty e, expr_context_ty ctx)
{
    return _Py_Subscript(e->v.Subscript.value, e->v.Subscript.slice, ctx, EXTRA_EXPR(e, e));
}

static expr_ty
_set_attribute_context(Parser *p, expr_ty e, expr_context_ty ctx)
{
    return _Py_Attribute(e->v.Attribute.value, e->v.Attribute.attr, ctx, EXTRA_EXPR(e, e));
}

static expr_ty
_set_starred_context(Parser *p, expr_ty e, expr_context_ty ctx)
{
    return _Py_Starred(_PyPegen_set_expr_context(p, e->v.Starred.value, ctx), ctx, EXTRA_EXPR(e, e));
}

/* Creates an `expr_ty` equivalent to `expr` but with `ctx` as context */
expr_ty
_PyPegen_set_expr_context(Parser *p, expr_ty expr, expr_context_ty ctx)
{
    assert(expr != NULL);

    expr_ty new = NULL;
    switch (expr->kind) {
        case Name_kind:
            new = _set_name_context(p, expr, ctx);
            break;
        case Tuple_kind:
            new = _set_tuple_context(p, expr, ctx);
            break;
        case List_kind:
            new = _set_list_context(p, expr, ctx);
            break;
        case Subscript_kind:
            new = _set_subscript_context(p, expr, ctx);
            break;
        case Attribute_kind:
            new = _set_attribute_context(p, expr, ctx);
            break;
        case Starred_kind:
            new = _set_starred_context(p, expr, ctx);
            break;
        default:
            new = expr;
    }
    return new;
}

/* Constructs a KeyValuePair that is used when parsing a dict's key value pairs */
KeyValuePair *
_PyPegen_key_value_pair(Parser *p, expr_ty key, expr_ty value)
{
    KeyValuePair *a = PyArena_Malloc(p->arena, sizeof(KeyValuePair));
    if (!a) {
        return NULL;
    }
    a->key = key;
    a->value = value;
    return a;
}

/* Extracts all keys from an asdl_seq* of KeyValuePair*'s */
asdl_seq *
_PyPegen_get_keys(Parser *p, asdl_seq *seq)
{
    Py_ssize_t len = asdl_seq_LEN(seq);
    asdl_seq *new_seq = _Py_asdl_seq_new(len, p->arena);
    if (!new_seq) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        KeyValuePair *pair = asdl_seq_GET(seq, i);
        asdl_seq_SET(new_seq, i, pair->key);
    }
    return new_seq;
}

/* Extracts all values from an asdl_seq* of KeyValuePair*'s */
asdl_seq *
_PyPegen_get_values(Parser *p, asdl_seq *seq)
{
    Py_ssize_t len = asdl_seq_LEN(seq);
    asdl_seq *new_seq = _Py_asdl_seq_new(len, p->arena);
    if (!new_seq) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        KeyValuePair *pair = asdl_seq_GET(seq, i);
        asdl_seq_SET(new_seq, i, pair->value);
    }
    return new_seq;
}

/* Constructs a NameDefaultPair */
NameDefaultPair *
_PyPegen_name_default_pair(Parser *p, arg_ty arg, expr_ty value, Token *tc)
{
    NameDefaultPair *a = PyArena_Malloc(p->arena, sizeof(NameDefaultPair));
    if (!a) {
        return NULL;
    }
    a->arg = _PyPegen_add_type_comment_to_arg(p, arg, tc);
    a->value = value;
    return a;
}

/* Constructs a SlashWithDefault */
SlashWithDefault *
_PyPegen_slash_with_default(Parser *p, asdl_seq *plain_names, asdl_seq *names_with_defaults)
{
    SlashWithDefault *a = PyArena_Malloc(p->arena, sizeof(SlashWithDefault));
    if (!a) {
        return NULL;
    }
    a->plain_names = plain_names;
    a->names_with_defaults = names_with_defaults;
    return a;
}

/* Constructs a StarEtc */
StarEtc *
_PyPegen_star_etc(Parser *p, arg_ty vararg, asdl_seq *kwonlyargs, arg_ty kwarg)
{
    StarEtc *a = PyArena_Malloc(p->arena, sizeof(StarEtc));
    if (!a) {
        return NULL;
    }
    a->vararg = vararg;
    a->kwonlyargs = kwonlyargs;
    a->kwarg = kwarg;
    return a;
}

asdl_seq *
_PyPegen_join_sequences(Parser *p, asdl_seq *a, asdl_seq *b)
{
    Py_ssize_t first_len = asdl_seq_LEN(a);
    Py_ssize_t second_len = asdl_seq_LEN(b);
    asdl_seq *new_seq = _Py_asdl_seq_new(first_len + second_len, p->arena);
    if (!new_seq) {
        return NULL;
    }

    int k = 0;
    for (Py_ssize_t i = 0; i < first_len; i++) {
        asdl_seq_SET(new_seq, k++, asdl_seq_GET(a, i));
    }
    for (Py_ssize_t i = 0; i < second_len; i++) {
        asdl_seq_SET(new_seq, k++, asdl_seq_GET(b, i));
    }

    return new_seq;
}

static asdl_seq *
_get_names(Parser *p, asdl_seq *names_with_defaults)
{
    Py_ssize_t len = asdl_seq_LEN(names_with_defaults);
    asdl_seq *seq = _Py_asdl_seq_new(len, p->arena);
    if (!seq) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        NameDefaultPair *pair = asdl_seq_GET(names_with_defaults, i);
        asdl_seq_SET(seq, i, pair->arg);
    }
    return seq;
}

static asdl_seq *
_get_defaults(Parser *p, asdl_seq *names_with_defaults)
{
    Py_ssize_t len = asdl_seq_LEN(names_with_defaults);
    asdl_seq *seq = _Py_asdl_seq_new(len, p->arena);
    if (!seq) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        NameDefaultPair *pair = asdl_seq_GET(names_with_defaults, i);
        asdl_seq_SET(seq, i, pair->value);
    }
    return seq;
}

/* Constructs an arguments_ty object out of all the parsed constructs in the parameters rule */
arguments_ty
_PyPegen_make_arguments(Parser *p, asdl_seq *slash_without_default,
                        SlashWithDefault *slash_with_default, asdl_seq *plain_names,
                        asdl_seq *names_with_default, StarEtc *star_etc)
{
    asdl_seq *posonlyargs;
    if (slash_without_default != NULL) {
        posonlyargs = slash_without_default;
    }
    else if (slash_with_default != NULL) {
        asdl_seq *slash_with_default_names =
            _get_names(p, slash_with_default->names_with_defaults);
        if (!slash_with_default_names) {
            return NULL;
        }
        posonlyargs = _PyPegen_join_sequences(p, slash_with_default->plain_names, slash_with_default_names);
        if (!posonlyargs) {
            return NULL;
        }
    }
    else {
        posonlyargs = _Py_asdl_seq_new(0, p->arena);
        if (!posonlyargs) {
            return NULL;
        }
    }

    asdl_seq *posargs;
    if (plain_names != NULL && names_with_default != NULL) {
        asdl_seq *names_with_default_names = _get_names(p, names_with_default);
        if (!names_with_default_names) {
            return NULL;
        }
        posargs = _PyPegen_join_sequences(p, plain_names, names_with_default_names);
        if (!posargs) {
            return NULL;
        }
    }
    else if (plain_names == NULL && names_with_default != NULL) {
        posargs = _get_names(p, names_with_default);
        if (!posargs) {
            return NULL;
        }
    }
    else if (plain_names != NULL && names_with_default == NULL) {
        posargs = plain_names;
    }
    else {
        posargs = _Py_asdl_seq_new(0, p->arena);
        if (!posargs) {
            return NULL;
        }
    }

    asdl_seq *posdefaults;
    if (slash_with_default != NULL && names_with_default != NULL) {
        asdl_seq *slash_with_default_values =
            _get_defaults(p, slash_with_default->names_with_defaults);
        if (!slash_with_default_values) {
            return NULL;
        }
        asdl_seq *names_with_default_values = _get_defaults(p, names_with_default);
        if (!names_with_default_values) {
            return NULL;
        }
        posdefaults = _PyPegen_join_sequences(p, slash_with_default_values, names_with_default_values);
        if (!posdefaults) {
            return NULL;
        }
    }
    else if (slash_with_default == NULL && names_with_default != NULL) {
        posdefaults = _get_defaults(p, names_with_default);
        if (!posdefaults) {
            return NULL;
        }
    }
    else if (slash_with_default != NULL && names_with_default == NULL) {
        posdefaults = _get_defaults(p, slash_with_default->names_with_defaults);
        if (!posdefaults) {
            return NULL;
        }
    }
    else {
        posdefaults = _Py_asdl_seq_new(0, p->arena);
        if (!posdefaults) {
            return NULL;
        }
    }

    arg_ty vararg = NULL;
    if (star_etc != NULL && star_etc->vararg != NULL) {
        vararg = star_etc->vararg;
    }

    asdl_seq *kwonlyargs;
    if (star_etc != NULL && star_etc->kwonlyargs != NULL) {
        kwonlyargs = _get_names(p, star_etc->kwonlyargs);
        if (!kwonlyargs) {
            return NULL;
        }
    }
    else {
        kwonlyargs = _Py_asdl_seq_new(0, p->arena);
        if (!kwonlyargs) {
            return NULL;
        }
    }

    asdl_seq *kwdefaults;
    if (star_etc != NULL && star_etc->kwonlyargs != NULL) {
        kwdefaults = _get_defaults(p, star_etc->kwonlyargs);
        if (!kwdefaults) {
            return NULL;
        }
    }
    else {
        kwdefaults = _Py_asdl_seq_new(0, p->arena);
        if (!kwdefaults) {
            return NULL;
        }
    }

    arg_ty kwarg = NULL;
    if (star_etc != NULL && star_etc->kwarg != NULL) {
        kwarg = star_etc->kwarg;
    }

    return _Py_arguments(posonlyargs, posargs, vararg, kwonlyargs, kwdefaults, kwarg,
                         posdefaults, p->arena);
}

/* Constructs an empty arguments_ty object, that gets used when a function accepts no
 * arguments. */
arguments_ty
_PyPegen_empty_arguments(Parser *p)
{
    asdl_seq *posonlyargs = _Py_asdl_seq_new(0, p->arena);
    if (!posonlyargs) {
        return NULL;
    }
    asdl_seq *posargs = _Py_asdl_seq_new(0, p->arena);
    if (!posargs) {
        return NULL;
    }
    asdl_seq *posdefaults = _Py_asdl_seq_new(0, p->arena);
    if (!posdefaults) {
        return NULL;
    }
    asdl_seq *kwonlyargs = _Py_asdl_seq_new(0, p->arena);
    if (!kwonlyargs) {
        return NULL;
    }
    asdl_seq *kwdefaults = _Py_asdl_seq_new(0, p->arena);
    if (!kwdefaults) {
        return NULL;
    }

    return _Py_arguments(posonlyargs, posargs, NULL, kwonlyargs, kwdefaults, NULL, kwdefaults,
                         p->arena);
}

/* Encapsulates the value of an operator_ty into an AugOperator struct */
AugOperator *
_PyPegen_augoperator(Parser *p, operator_ty kind)
{
    AugOperator *a = PyArena_Malloc(p->arena, sizeof(AugOperator));
    if (!a) {
        return NULL;
    }
    a->kind = kind;
    return a;
}

/* Construct a FunctionDef equivalent to function_def, but with decorators */
stmt_ty
_PyPegen_function_def_decorators(Parser *p, asdl_seq *decorators, stmt_ty function_def)
{
    assert(function_def != NULL);
    if (function_def->kind == AsyncFunctionDef_kind) {
        return _Py_AsyncFunctionDef(
            function_def->v.FunctionDef.name, function_def->v.FunctionDef.args,
            function_def->v.FunctionDef.body, decorators, function_def->v.FunctionDef.returns,
            function_def->v.FunctionDef.type_comment, function_def->lineno,
            function_def->col_offset, function_def->end_lineno, function_def->end_col_offset,
            p->arena);
    }

    return _Py_FunctionDef(function_def->v.FunctionDef.name, function_def->v.FunctionDef.args,
                           function_def->v.FunctionDef.body, decorators,
                           function_def->v.FunctionDef.returns,
                           function_def->v.FunctionDef.type_comment, function_def->lineno,
                           function_def->col_offset, function_def->end_lineno,
                           function_def->end_col_offset, p->arena);
}

/* Construct a ClassDef equivalent to class_def, but with decorators */
stmt_ty
_PyPegen_class_def_decorators(Parser *p, asdl_seq *decorators, stmt_ty class_def)
{
    assert(class_def != NULL);
    return _Py_ClassDef(class_def->v.ClassDef.name, class_def->v.ClassDef.bases,
                        class_def->v.ClassDef.keywords, class_def->v.ClassDef.body, decorators,
                        class_def->lineno, class_def->col_offset, class_def->end_lineno,
                        class_def->end_col_offset, p->arena);
}

/* Construct a KeywordOrStarred */
KeywordOrStarred *
_PyPegen_keyword_or_starred(Parser *p, void *element, int is_keyword)
{
    KeywordOrStarred *a = PyArena_Malloc(p->arena, sizeof(KeywordOrStarred));
    if (!a) {
        return NULL;
    }
    a->element = element;
    a->is_keyword = is_keyword;
    return a;
}

/* Get the number of starred expressions in an asdl_seq* of KeywordOrStarred*s */
static int
_seq_number_of_starred_exprs(asdl_seq *seq)
{
    int n = 0;
    for (Py_ssize_t i = 0, l = asdl_seq_LEN(seq); i < l; i++) {
        KeywordOrStarred *k = asdl_seq_GET(seq, i);
        if (!k->is_keyword) {
            n++;
        }
    }
    return n;
}

/* Extract the starred expressions of an asdl_seq* of KeywordOrStarred*s */
asdl_seq *
_PyPegen_seq_extract_starred_exprs(Parser *p, asdl_seq *kwargs)
{
    int new_len = _seq_number_of_starred_exprs(kwargs);
    if (new_len == 0) {
        return NULL;
    }
    asdl_seq *new_seq = _Py_asdl_seq_new(new_len, p->arena);
    if (!new_seq) {
        return NULL;
    }

    int idx = 0;
    for (Py_ssize_t i = 0, len = asdl_seq_LEN(kwargs); i < len; i++) {
        KeywordOrStarred *k = asdl_seq_GET(kwargs, i);
        if (!k->is_keyword) {
            asdl_seq_SET(new_seq, idx++, k->element);
        }
    }
    return new_seq;
}

/* Return a new asdl_seq* with only the keywords in kwargs */
asdl_seq *
_PyPegen_seq_delete_starred_exprs(Parser *p, asdl_seq *kwargs)
{
    Py_ssize_t len = asdl_seq_LEN(kwargs);
    Py_ssize_t new_len = len - _seq_number_of_starred_exprs(kwargs);
    if (new_len == 0) {
        return NULL;
    }
    asdl_seq *new_seq = _Py_asdl_seq_new(new_len, p->arena);
    if (!new_seq) {
        return NULL;
    }

    int idx = 0;
    for (Py_ssize_t i = 0; i < len; i++) {
        KeywordOrStarred *k = asdl_seq_GET(kwargs, i);
        if (k->is_keyword) {
            asdl_seq_SET(new_seq, idx++, k->element);
        }
    }
    return new_seq;
}

expr_ty
_PyPegen_concatenate_strings(Parser *p, asdl_seq *strings)
{
    Py_ssize_t len = asdl_seq_LEN(strings);
    assert(len > 0);

    Token *first = asdl_seq_GET(strings, 0);
    Token *last = asdl_seq_GET(strings, len - 1);

    int bytesmode = 0;
    PyObject *bytes_str = NULL;

    FstringParser state;
    _PyPegen_FstringParser_Init(&state);

    for (Py_ssize_t i = 0; i < len; i++) {
        Token *t = asdl_seq_GET(strings, i);

        int this_bytesmode;
        int this_rawmode;
        PyObject *s;
        const char *fstr;
        Py_ssize_t fstrlen = -1;

        if (_PyPegen_parsestr(p, &this_bytesmode, &this_rawmode, &s, &fstr, &fstrlen, t) != 0) {
            goto error;
        }

        /* Check that we are not mixing bytes with unicode. */
        if (i != 0 && bytesmode != this_bytesmode) {
            RAISE_SYNTAX_ERROR("cannot mix bytes and nonbytes literals");
            Py_XDECREF(s);
            goto error;
        }
        bytesmode = this_bytesmode;

        if (fstr != NULL) {
            assert(s == NULL && !bytesmode);

            int result = _PyPegen_FstringParser_ConcatFstring(p, &state, &fstr, fstr + fstrlen,
                                                     this_rawmode, 0, first, t, last);
            if (result < 0) {
                goto error;
            }
        }
        else {
            /* String or byte string. */
            assert(s != NULL && fstr == NULL);
            assert(bytesmode ? PyBytes_CheckExact(s) : PyUnicode_CheckExact(s));

            if (bytesmode) {
                if (i == 0) {
                    bytes_str = s;
                }
                else {
                    PyBytes_ConcatAndDel(&bytes_str, s);
                    if (!bytes_str) {
                        goto error;
                    }
                }
            }
            else {
                /* This is a regular string. Concatenate it. */
                if (_PyPegen_FstringParser_ConcatAndDel(&state, s) < 0) {
                    goto error;
                }
            }
        }
    }

    if (bytesmode) {
        if (PyArena_AddPyObject(p->arena, bytes_str) < 0) {
            goto error;
        }
        return Constant(bytes_str, NULL, first->lineno, first->col_offset, last->end_lineno,
                        last->end_col_offset, p->arena);
    }

    return _PyPegen_FstringParser_Finish(p, &state, first, last);

error:
    Py_XDECREF(bytes_str);
    _PyPegen_FstringParser_Dealloc(&state);
    if (PyErr_Occurred()) {
        raise_decode_error(p);
    }
    return NULL;
}

mod_ty
_PyPegen_make_module(Parser *p, asdl_seq *a) {
    asdl_seq *type_ignores = NULL;
    Py_ssize_t num = p->type_ignore_comments.num_items;
    if (num > 0) {
        // Turn the raw (comment, lineno) pairs into TypeIgnore objects in the arena
        type_ignores = _Py_asdl_seq_new(num, p->arena);
        if (type_ignores == NULL) {
            return NULL;
        }
        for (int i = 0; i < num; i++) {
            PyObject *tag = _PyPegen_new_type_comment(p, p->type_ignore_comments.items[i].comment);
            if (tag == NULL) {
                return NULL;
            }
            type_ignore_ty ti = TypeIgnore(p->type_ignore_comments.items[i].lineno, tag, p->arena);
            if (ti == NULL) {
                return NULL;
            }
            asdl_seq_SET(type_ignores, i, ti);
        }
    }
    return Module(a, type_ignores, p->arena);
}

// Error reporting helpers

expr_ty
_PyPegen_get_invalid_target(expr_ty e)
{
    if (e == NULL) {
        return NULL;
    }

#define VISIT_CONTAINER(CONTAINER, TYPE) do { \
        Py_ssize_t len = asdl_seq_LEN(CONTAINER->v.TYPE.elts);\
        for (Py_ssize_t i = 0; i < len; i++) {\
            expr_ty other = asdl_seq_GET(CONTAINER->v.TYPE.elts, i);\
            expr_ty child = _PyPegen_get_invalid_target(other);\
            if (child != NULL) {\
                return child;\
            }\
        }\
    } while (0)

    // We only need to visit List and Tuple nodes recursively as those
    // are the only ones that can contain valid names in targets when
    // they are parsed as expressions. Any other kind of expression
    // that is a container (like Sets or Dicts) is directly invalid and
    // we don't need to visit it recursively.

    switch (e->kind) {
        case List_kind: {
            VISIT_CONTAINER(e, List);
            return NULL;
        }
        case Tuple_kind: {
            VISIT_CONTAINER(e, Tuple);
            return NULL;
        }
        case Starred_kind:
            return _PyPegen_get_invalid_target(e->v.Starred.value);
        case Name_kind:
        case Subscript_kind:
        case Attribute_kind:
            return NULL;
        default:
            return e;
    }
}

void *_PyPegen_arguments_parsing_error(Parser *p, expr_ty e) {
    int kwarg_unpacking = 0;
    for (Py_ssize_t i = 0, l = asdl_seq_LEN(e->v.Call.keywords); i < l; i++) {
        keyword_ty keyword = asdl_seq_GET(e->v.Call.keywords, i);
        if (!keyword->arg) {
            kwarg_unpacking = 1;
        }
    }

    const char *msg = NULL;
    if (kwarg_unpacking) {
        msg = "positional argument follows keyword argument unpacking";
    } else {
        msg = "positional argument follows keyword argument";
    }

    return RAISE_SYNTAX_ERROR(msg);
}

void *
_PyPegen_nonparen_genexp_in_call(Parser *p, expr_ty args)
{
    /* The rule that calls this function is 'args for_if_clauses'.
       For the input f(L, x for x in y), L and x are in args and
       the for is parsed as a for_if_clause. We have to check if
       len <= 1, so that input like dict((a, b) for a, b in x)
       gets successfully parsed and then we pass the last
       argument (x in the above example) as the location of the
       error */
    Py_ssize_t len = asdl_seq_LEN(args->v.Call.args);
    if (len <= 1) {
        return NULL;
    }

    return RAISE_SYNTAX_ERROR_KNOWN_LOCATION(
        (expr_ty) asdl_seq_GET(args->v.Call.args, len - 1),
        "Generator expression must be parenthesized"
    );
}
