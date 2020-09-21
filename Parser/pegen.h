#ifndef PEGEN_H
#define PEGEN_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <token.h>
#include <Python-ast.h>
#include <pyarena.h>

#if 0
#define PyPARSE_YIELD_IS_KEYWORD        0x0001
#endif

#define PyPARSE_DONT_IMPLY_DEDENT       0x0002

#if 0
#define PyPARSE_WITH_IS_KEYWORD         0x0003
#define PyPARSE_PRINT_IS_FUNCTION       0x0004
#define PyPARSE_UNICODE_LITERALS        0x0008
#endif

#define PyPARSE_IGNORE_COOKIE 0x0010
#define PyPARSE_BARRY_AS_BDFL 0x0020
#define PyPARSE_TYPE_COMMENTS 0x0040
#define PyPARSE_ASYNC_HACKS   0x0080

typedef struct _memo {
    int type;
    void *node;
    int mark;
    struct _memo *next;
} Memo;

typedef struct {
    int type;
    PyObject *bytes;
    int lineno, col_offset, end_lineno, end_col_offset;
    Memo *memo;
} Token;

typedef struct {
    char *str;
    int type;
} KeywordToken;


typedef struct {
    struct {
        int lineno;
        char *comment;  // The " <tag>" in "# type: ignore <tag>"
    } *items;
    size_t size;
    size_t num_items;
} growable_comment_array;

typedef struct {
    struct tok_state *tok;
    Token **tokens;
    int mark;
    int fill, size;
    PyArena *arena;
    KeywordToken **keywords;
    int n_keyword_lists;
    int start_rule;
    int *errcode;
    int parsing_started;
    PyObject* normalize;
    int starting_lineno;
    int starting_col_offset;
    int error_indicator;
    int flags;
    int feature_version;
    growable_comment_array type_ignore_comments;
    Token *known_err_token;
    int level;
} Parser;

typedef struct {
    cmpop_ty cmpop;
    expr_ty expr;
} CmpopExprPair;

typedef struct {
    expr_ty key;
    expr_ty value;
} KeyValuePair;

typedef struct {
    arg_ty arg;
    expr_ty value;
} NameDefaultPair;

typedef struct {
    asdl_arg_seq *plain_names;
    asdl_seq *names_with_defaults; // asdl_seq* of NameDefaultsPair's
} SlashWithDefault;

typedef struct {
    arg_ty vararg;
    asdl_seq *kwonlyargs; // asdl_seq* of NameDefaultsPair's
    arg_ty kwarg;
} StarEtc;

typedef struct {
    operator_ty kind;
} AugOperator;

typedef struct {
    void *element;
    int is_keyword;
} KeywordOrStarred;

void _PyPegen_clear_memo_statistics(void);
PyObject *_PyPegen_get_memo_statistics(void);

int _PyPegen_insert_memo(Parser *p, int mark, int type, void *node);
int _PyPegen_update_memo(Parser *p, int mark, int type, void *node);
int _PyPegen_is_memoized(Parser *p, int type, void *pres);

int _PyPegen_lookahead_with_name(int, expr_ty (func)(Parser *), Parser *);
int _PyPegen_lookahead_with_int(int, Token *(func)(Parser *, int), Parser *, int);
int _PyPegen_lookahead_with_string(int , expr_ty (func)(Parser *, const char*), Parser *, const char*);
int _PyPegen_lookahead(int, void *(func)(Parser *), Parser *);

Token *_PyPegen_expect_token(Parser *p, int type);
expr_ty _PyPegen_expect_soft_keyword(Parser *p, const char *keyword);
Token *_PyPegen_get_last_nonnwhitespace_token(Parser *);
int _PyPegen_fill_token(Parser *p);
expr_ty _PyPegen_name_token(Parser *p);
expr_ty _PyPegen_number_token(Parser *p);
void *_PyPegen_string_token(Parser *p);
const char *_PyPegen_get_expr_name(expr_ty);
void *_PyPegen_raise_error(Parser *p, PyObject *errtype, const char *errmsg, ...);
void *_PyPegen_raise_error_known_location(Parser *p, PyObject *errtype,
                                          Py_ssize_t lineno, Py_ssize_t col_offset,
                                          const char *errmsg, va_list va);
void *_PyPegen_dummy_name(Parser *p, ...);

Py_LOCAL_INLINE(void *)
RAISE_ERROR_KNOWN_LOCATION(Parser *p, PyObject *errtype, int lineno,
                           int col_offset, const char *errmsg, ...)
{
    va_list va;
    va_start(va, errmsg);
    _PyPegen_raise_error_known_location(p, errtype, lineno, col_offset + 1,
                                        errmsg, va);
    va_end(va);
    return NULL;
}


#define UNUSED(expr) do { (void)(expr); } while (0)
#define EXTRA_EXPR(head, tail) head->lineno, head->col_offset, tail->end_lineno, tail->end_col_offset, p->arena
#define EXTRA _start_lineno, _start_col_offset, _end_lineno, _end_col_offset, p->arena
#define RAISE_SYNTAX_ERROR(msg, ...) _PyPegen_raise_error(p, PyExc_SyntaxError, msg, ##__VA_ARGS__)
#define RAISE_INDENTATION_ERROR(msg, ...) _PyPegen_raise_error(p, PyExc_IndentationError, msg, ##__VA_ARGS__)
#define RAISE_SYNTAX_ERROR_KNOWN_LOCATION(a, msg, ...) \
    RAISE_ERROR_KNOWN_LOCATION(p, PyExc_SyntaxError, (a)->lineno, (a)->col_offset, msg, ##__VA_ARGS__)

Py_LOCAL_INLINE(void *)
CHECK_CALL(Parser *p, void *result)
{
    if (result == NULL) {
        assert(PyErr_Occurred());
        p->error_indicator = 1;
    }
    return result;
}

/* This is needed for helper functions that are allowed to
   return NULL without an error. Example: _PyPegen_seq_extract_starred_exprs */
Py_LOCAL_INLINE(void *)
CHECK_CALL_NULL_ALLOWED(Parser *p, void *result)
{
    if (result == NULL && PyErr_Occurred()) {
        p->error_indicator = 1;
    }
    return result;
}

#define CHECK(result) CHECK_CALL(p, result)
#define CHECK_NULL_ALLOWED(result) CHECK_CALL_NULL_ALLOWED(p, result)

PyObject *_PyPegen_new_type_comment(Parser *, char *);

Py_LOCAL_INLINE(PyObject *)
NEW_TYPE_COMMENT(Parser *p, Token *tc)
{
    if (tc == NULL) {
        return NULL;
    }
    char *bytes = PyBytes_AsString(tc->bytes);
    if (bytes == NULL) {
        goto error;
    }
    PyObject *tco = _PyPegen_new_type_comment(p, bytes);
    if (tco == NULL) {
        goto error;
    }
    return tco;
 error:
    p->error_indicator = 1;  // Inline CHECK_CALL
    return NULL;
}

Py_LOCAL_INLINE(void *)
INVALID_VERSION_CHECK(Parser *p, int version, char *msg, void *node)
{
    if (node == NULL) {
        p->error_indicator = 1;  // Inline CHECK_CALL
        return NULL;
    }
    if (p->feature_version < version) {
        p->error_indicator = 1;
        return RAISE_SYNTAX_ERROR("%s only supported in Python 3.%i and greater",
                                  msg, version);
    }
    return node;
}

#define CHECK_VERSION(version, msg, node) INVALID_VERSION_CHECK(p, version, msg, node)

arg_ty _PyPegen_add_type_comment_to_arg(Parser *, arg_ty, Token *);
PyObject *_PyPegen_new_identifier(Parser *, char *);
Parser *_PyPegen_Parser_New(struct tok_state *, int, int, int, int *, PyArena *);
void _PyPegen_Parser_Free(Parser *);
mod_ty _PyPegen_run_parser_from_file_pointer(FILE *, int, PyObject *, const char *,
                                    const char *, const char *, PyCompilerFlags *, int *, PyArena *);
void *_PyPegen_run_parser(Parser *);
mod_ty _PyPegen_run_parser_from_file(const char *, int, PyObject *, PyCompilerFlags *, PyArena *);
mod_ty _PyPegen_run_parser_from_string(const char *, int, PyObject *, PyCompilerFlags *, PyArena *);
asdl_stmt_seq *_PyPegen_interactive_exit(Parser *);
asdl_seq *_PyPegen_singleton_seq(Parser *, void *);
asdl_seq *_PyPegen_seq_insert_in_front(Parser *, void *, asdl_seq *);
asdl_seq *_PyPegen_seq_append_to_end(Parser *, asdl_seq *, void *);
asdl_seq *_PyPegen_seq_flatten(Parser *, asdl_seq *);
expr_ty _PyPegen_join_names_with_dot(Parser *, expr_ty, expr_ty);
int _PyPegen_seq_count_dots(asdl_seq *);
alias_ty _PyPegen_alias_for_star(Parser *);
asdl_identifier_seq *_PyPegen_map_names_to_ids(Parser *, asdl_expr_seq *);
CmpopExprPair *_PyPegen_cmpop_expr_pair(Parser *, cmpop_ty, expr_ty);
asdl_int_seq *_PyPegen_get_cmpops(Parser *p, asdl_seq *);
asdl_expr_seq *_PyPegen_get_exprs(Parser *, asdl_seq *);
expr_ty _PyPegen_set_expr_context(Parser *, expr_ty, expr_context_ty);
KeyValuePair *_PyPegen_key_value_pair(Parser *, expr_ty, expr_ty);
asdl_expr_seq *_PyPegen_get_keys(Parser *, asdl_seq *);
asdl_expr_seq *_PyPegen_get_values(Parser *, asdl_seq *);
NameDefaultPair *_PyPegen_name_default_pair(Parser *, arg_ty, expr_ty, Token *);
SlashWithDefault *_PyPegen_slash_with_default(Parser *, asdl_arg_seq *, asdl_seq *);
StarEtc *_PyPegen_star_etc(Parser *, arg_ty, asdl_seq *, arg_ty);
arguments_ty _PyPegen_make_arguments(Parser *, asdl_arg_seq *, SlashWithDefault *,
                                     asdl_arg_seq *, asdl_seq *, StarEtc *);
arguments_ty _PyPegen_empty_arguments(Parser *);
AugOperator *_PyPegen_augoperator(Parser*, operator_ty type);
stmt_ty _PyPegen_function_def_decorators(Parser *, asdl_expr_seq *, stmt_ty);
stmt_ty _PyPegen_class_def_decorators(Parser *, asdl_expr_seq *, stmt_ty);
KeywordOrStarred *_PyPegen_keyword_or_starred(Parser *, void *, int);
asdl_expr_seq *_PyPegen_seq_extract_starred_exprs(Parser *, asdl_seq *);
asdl_keyword_seq *_PyPegen_seq_delete_starred_exprs(Parser *, asdl_seq *);
expr_ty _PyPegen_collect_call_seqs(Parser *, asdl_expr_seq *, asdl_seq *,
                     int lineno, int col_offset, int end_lineno,
                     int end_col_offset, PyArena *arena);
expr_ty _PyPegen_concatenate_strings(Parser *p, asdl_seq *);
asdl_seq *_PyPegen_join_sequences(Parser *, asdl_seq *, asdl_seq *);
int _PyPegen_check_barry_as_flufl(Parser *);
mod_ty _PyPegen_make_module(Parser *, asdl_stmt_seq *);

// Error reporting helpers
typedef enum {
    STAR_TARGETS,
    DEL_TARGETS,
    FOR_TARGETS
} TARGETS_TYPE;
expr_ty _PyPegen_get_invalid_target(expr_ty e, TARGETS_TYPE targets_type);
#define RAISE_SYNTAX_ERROR_INVALID_TARGET(type, e) _RAISE_SYNTAX_ERROR_INVALID_TARGET(p, type, e)

Py_LOCAL_INLINE(void *)
_RAISE_SYNTAX_ERROR_INVALID_TARGET(Parser *p, TARGETS_TYPE type, void *e)
{
    expr_ty invalid_target = CHECK_NULL_ALLOWED(_PyPegen_get_invalid_target(e, type));
    if (invalid_target != NULL) {
        const char *msg;
        if (type == STAR_TARGETS || type == FOR_TARGETS) {
            msg = "cannot assign to %s";
        }
        else {
            msg = "cannot delete %s";
        }
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION(
            invalid_target,
            msg,
            _PyPegen_get_expr_name(invalid_target)
        );
    }
    return RAISE_SYNTAX_ERROR("invalid syntax");
}

void *_PyPegen_arguments_parsing_error(Parser *, expr_ty);
void *_PyPegen_nonparen_genexp_in_call(Parser *p, expr_ty args);


// Generated function in parse.c - function definition in python.gram
void *_PyPegen_parse(Parser *);

#endif
