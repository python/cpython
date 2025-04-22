#ifndef PEGEN_H
#define PEGEN_H

#include <Python.h>
#include <pycore_ast.h>
#include <pycore_token.h>

#include "lexer/state.h"

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
#define PyPARSE_ALLOW_INCOMPLETE_INPUT 0x0100

#define CURRENT_POS (-5)

#define TOK_GET_MODE(tok) (&(tok->tok_mode_stack[tok->tok_mode_stack_index]))
#define TOK_GET_STRING_PREFIX(tok) (TOK_GET_MODE(tok)->string_kind == TSTRING ? 't' : 'f')

typedef struct _memo {
    int type;
    void *node;
    int mark;
    struct _memo *next;
} Memo;

typedef struct {
    int type;
    PyObject *bytes;
    int level;
    int lineno, col_offset, end_lineno, end_col_offset;
    Memo *memo;
    PyObject *metadata;
} Token;

typedef struct {
    const char *str;
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
    int lineno;
    int col_offset;
    int end_lineno;
    int end_col_offset;
} location;

typedef struct {
    struct tok_state *tok;
    Token **tokens;
    int mark;
    int fill, size;
    PyArena *arena;
    KeywordToken **keywords;
    char **soft_keywords;
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
    int call_invalid_rules;
    int debug;
    location last_stmt_location;
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
    expr_ty key;
    pattern_ty pattern;
} KeyPatternPair;

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

typedef struct { operator_ty kind; } AugOperator;
typedef struct {
    void *element;
    int is_keyword;
} KeywordOrStarred;

typedef struct {
    void *result;
    PyObject *metadata;
} ResultTokenWithMetadata;

// Internal parser functions
#if defined(Py_DEBUG)
void _PyPegen_clear_memo_statistics(void);
PyObject *_PyPegen_get_memo_statistics(void);
#endif

int _PyPegen_insert_memo(Parser *p, int mark, int type, void *node);
int _PyPegen_update_memo(Parser *p, int mark, int type, void *node);
int _PyPegen_is_memoized(Parser *p, int type, void *pres);

int _PyPegen_lookahead_with_name(int, expr_ty (func)(Parser *), Parser *);
int _PyPegen_lookahead_with_int(int, Token *(func)(Parser *, int), Parser *, int);
int _PyPegen_lookahead_with_string(int , expr_ty (func)(Parser *, const char*), Parser *, const char*);
int _PyPegen_lookahead(int, void *(func)(Parser *), Parser *);

Token *_PyPegen_expect_token(Parser *p, int type);
void* _PyPegen_expect_forced_result(Parser *p, void* result, const char* expected);
Token *_PyPegen_expect_forced_token(Parser *p, int type, const char* expected);
expr_ty _PyPegen_expect_soft_keyword(Parser *p, const char *keyword);
expr_ty _PyPegen_soft_keyword_token(Parser *p);
expr_ty _PyPegen_fstring_middle_token(Parser* p);
Token *_PyPegen_get_last_nonnwhitespace_token(Parser *);
int _PyPegen_fill_token(Parser *p);
expr_ty _PyPegen_name_token(Parser *p);
expr_ty _PyPegen_number_token(Parser *p);
void *_PyPegen_string_token(Parser *p);
PyObject *_PyPegen_set_source_in_metadata(Parser *p, Token *t);
Py_ssize_t _PyPegen_byte_offset_to_character_offset_line(PyObject *line, Py_ssize_t col_offset, Py_ssize_t end_col_offset);
Py_ssize_t _PyPegen_byte_offset_to_character_offset(PyObject *line, Py_ssize_t col_offset);
Py_ssize_t _PyPegen_byte_offset_to_character_offset_raw(const char*, Py_ssize_t col_offset);

// Error handling functions and APIs
typedef enum {
    STAR_TARGETS,
    DEL_TARGETS,
    FOR_TARGETS
} TARGETS_TYPE;

int _Pypegen_raise_decode_error(Parser *p);
void _PyPegen_raise_tokenizer_init_error(PyObject *filename);
int _Pypegen_tokenizer_error(Parser *p);
void *_PyPegen_raise_error(Parser *p, PyObject *errtype, int use_mark, const char *errmsg, ...);
void *_PyPegen_raise_error_known_location(Parser *p, PyObject *errtype,
                                          Py_ssize_t lineno, Py_ssize_t col_offset,
                                          Py_ssize_t end_lineno, Py_ssize_t end_col_offset,
                                          const char *errmsg, va_list va);
void _Pypegen_set_syntax_error(Parser* p, Token* last_token);
void _Pypegen_stack_overflow(Parser *p);

static inline void *
RAISE_ERROR_KNOWN_LOCATION(Parser *p, PyObject *errtype,
                           Py_ssize_t lineno, Py_ssize_t col_offset,
                           Py_ssize_t end_lineno, Py_ssize_t end_col_offset,
                           const char *errmsg, ...)
{
    va_list va;
    va_start(va, errmsg);
    Py_ssize_t _col_offset = (col_offset == CURRENT_POS ? CURRENT_POS : col_offset + 1);
    Py_ssize_t _end_col_offset = (end_col_offset == CURRENT_POS ? CURRENT_POS : end_col_offset + 1);
    _PyPegen_raise_error_known_location(p, errtype, lineno, _col_offset, end_lineno, _end_col_offset, errmsg, va);
    va_end(va);
    return NULL;
}
#define RAISE_SYNTAX_ERROR(msg, ...) _PyPegen_raise_error(p, PyExc_SyntaxError, 0, msg, ##__VA_ARGS__)
#define RAISE_INDENTATION_ERROR(msg, ...) _PyPegen_raise_error(p, PyExc_IndentationError, 0, msg, ##__VA_ARGS__)
#define RAISE_SYNTAX_ERROR_ON_NEXT_TOKEN(msg, ...) _PyPegen_raise_error(p, PyExc_SyntaxError, 1, msg, ##__VA_ARGS__)
#define RAISE_SYNTAX_ERROR_KNOWN_RANGE(a, b, msg, ...) \
    RAISE_ERROR_KNOWN_LOCATION(p, PyExc_SyntaxError, (a)->lineno, (a)->col_offset, (b)->end_lineno, (b)->end_col_offset, msg, ##__VA_ARGS__)
#define RAISE_SYNTAX_ERROR_KNOWN_LOCATION(a, msg, ...) \
    RAISE_ERROR_KNOWN_LOCATION(p, PyExc_SyntaxError, (a)->lineno, (a)->col_offset, (a)->end_lineno, (a)->end_col_offset, msg, ##__VA_ARGS__)
#define RAISE_SYNTAX_ERROR_STARTING_FROM(a, msg, ...) \
    RAISE_ERROR_KNOWN_LOCATION(p, PyExc_SyntaxError, (a)->lineno, (a)->col_offset, CURRENT_POS, CURRENT_POS, msg, ##__VA_ARGS__)
#define RAISE_SYNTAX_ERROR_INVALID_TARGET(type, e) _RAISE_SYNTAX_ERROR_INVALID_TARGET(p, type, e)

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

#define CHECK(type, result) ((type) CHECK_CALL(p, result))
#define CHECK_NULL_ALLOWED(type, result) ((type) CHECK_CALL_NULL_ALLOWED(p, result))

expr_ty _PyPegen_get_invalid_target(expr_ty e, TARGETS_TYPE targets_type);
const char *_PyPegen_get_expr_name(expr_ty);
Py_LOCAL_INLINE(void *)
_RAISE_SYNTAX_ERROR_INVALID_TARGET(Parser *p, TARGETS_TYPE type, void *e)
{
    expr_ty invalid_target = CHECK_NULL_ALLOWED(expr_ty, _PyPegen_get_invalid_target(e, type));
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
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION(invalid_target, "invalid syntax");
    }
    return NULL;
}

// Action utility functions

void *_PyPegen_dummy_name(Parser *p, ...);
void * _PyPegen_seq_last_item(asdl_seq *seq);
#define PyPegen_last_item(seq, type) ((type)_PyPegen_seq_last_item((asdl_seq*)seq))
void * _PyPegen_seq_first_item(asdl_seq *seq);
#define PyPegen_first_item(seq, type) ((type)_PyPegen_seq_first_item((asdl_seq*)seq))
#define UNUSED(expr) do { (void)(expr); } while (0)
#define EXTRA_EXPR(head, tail) head->lineno, (head)->col_offset, (tail)->end_lineno, (tail)->end_col_offset, p->arena
#define EXTRA _start_lineno, _start_col_offset, _end_lineno, _end_col_offset, p->arena
PyObject *_PyPegen_new_type_comment(Parser *, const char *);

Py_LOCAL_INLINE(PyObject *)
NEW_TYPE_COMMENT(Parser *p, Token *tc)
{
    if (tc == NULL) {
        return NULL;
    }
    const char *bytes = PyBytes_AsString(tc->bytes);
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

#define CHECK_VERSION(type, version, msg, node) ((type) INVALID_VERSION_CHECK(p, version, msg, node))

arg_ty _PyPegen_add_type_comment_to_arg(Parser *, arg_ty, Token *);
PyObject *_PyPegen_new_identifier(Parser *, const char *);
asdl_seq *_PyPegen_singleton_seq(Parser *, void *);
asdl_seq *_PyPegen_seq_insert_in_front(Parser *, void *, asdl_seq *);
asdl_seq *_PyPegen_seq_append_to_end(Parser *, asdl_seq *, void *);
asdl_seq *_PyPegen_seq_flatten(Parser *, asdl_seq *);
expr_ty _PyPegen_join_names_with_dot(Parser *, expr_ty, expr_ty);
int _PyPegen_seq_count_dots(asdl_seq *);
alias_ty _PyPegen_alias_for_star(Parser *, int, int, int, int, PyArena *);
asdl_identifier_seq *_PyPegen_map_names_to_ids(Parser *, asdl_expr_seq *);
CmpopExprPair *_PyPegen_cmpop_expr_pair(Parser *, cmpop_ty, expr_ty);
asdl_int_seq *_PyPegen_get_cmpops(Parser *p, asdl_seq *);
asdl_expr_seq *_PyPegen_get_exprs(Parser *, asdl_seq *);
expr_ty _PyPegen_set_expr_context(Parser *, expr_ty, expr_context_ty);
KeyValuePair *_PyPegen_key_value_pair(Parser *, expr_ty, expr_ty);
asdl_expr_seq *_PyPegen_get_keys(Parser *, asdl_seq *);
asdl_expr_seq *_PyPegen_get_values(Parser *, asdl_seq *);
KeyPatternPair *_PyPegen_key_pattern_pair(Parser *, expr_ty, pattern_ty);
asdl_expr_seq *_PyPegen_get_pattern_keys(Parser *, asdl_seq *);
asdl_pattern_seq *_PyPegen_get_patterns(Parser *, asdl_seq *);
NameDefaultPair *_PyPegen_name_default_pair(Parser *, arg_ty, expr_ty, Token *);
SlashWithDefault *_PyPegen_slash_with_default(Parser *, asdl_arg_seq *, asdl_seq *);
StarEtc *_PyPegen_star_etc(Parser *, arg_ty, asdl_seq *, arg_ty);
arguments_ty _PyPegen_make_arguments(Parser *, asdl_arg_seq *, SlashWithDefault *,
                                     asdl_arg_seq *, asdl_seq *, StarEtc *);
arguments_ty _PyPegen_empty_arguments(Parser *);
expr_ty _PyPegen_template_str(Parser *p, Token *a, asdl_expr_seq *raw_expressions, Token *b);
expr_ty _PyPegen_joined_str(Parser *p, Token *a, asdl_expr_seq *raw_expressions, Token *b);
expr_ty _PyPegen_interpolation(Parser *, expr_ty, Token *, ResultTokenWithMetadata *, ResultTokenWithMetadata *, Token *,
                                 int, int, int, int, PyArena *);
expr_ty _PyPegen_formatted_value(Parser *, expr_ty, Token *, ResultTokenWithMetadata *, ResultTokenWithMetadata *, Token *,
                                 int, int, int, int, PyArena *);
AugOperator *_PyPegen_augoperator(Parser*, operator_ty type);
stmt_ty _PyPegen_function_def_decorators(Parser *, asdl_expr_seq *, stmt_ty);
stmt_ty _PyPegen_class_def_decorators(Parser *, asdl_expr_seq *, stmt_ty);
KeywordOrStarred *_PyPegen_keyword_or_starred(Parser *, void *, int);
asdl_expr_seq *_PyPegen_seq_extract_starred_exprs(Parser *, asdl_seq *);
asdl_keyword_seq *_PyPegen_seq_delete_starred_exprs(Parser *, asdl_seq *);
expr_ty _PyPegen_collect_call_seqs(Parser *, asdl_expr_seq *, asdl_seq *,
                     int lineno, int col_offset, int end_lineno,
                     int end_col_offset, PyArena *arena);
expr_ty _PyPegen_constant_from_token(Parser* p, Token* tok);
expr_ty _PyPegen_decoded_constant_from_token(Parser* p, Token* tok);
expr_ty _PyPegen_constant_from_string(Parser* p, Token* tok);
expr_ty _PyPegen_concatenate_strings(Parser *p, asdl_expr_seq *, int, int, int, int, PyArena *);
expr_ty _PyPegen_FetchRawForm(Parser *p, int, int, int, int);
expr_ty _PyPegen_ensure_imaginary(Parser *p, expr_ty);
expr_ty _PyPegen_ensure_real(Parser *p, expr_ty);
asdl_seq *_PyPegen_join_sequences(Parser *, asdl_seq *, asdl_seq *);
int _PyPegen_check_barry_as_flufl(Parser *, Token *);
int _PyPegen_check_legacy_stmt(Parser *p, expr_ty t);
ResultTokenWithMetadata *_PyPegen_check_fstring_conversion(Parser *p, Token *, expr_ty t);
ResultTokenWithMetadata *_PyPegen_setup_full_format_spec(Parser *, Token *, asdl_expr_seq *, int, int,
                                                         int, int, PyArena *);
mod_ty _PyPegen_make_module(Parser *, asdl_stmt_seq *);
void *_PyPegen_arguments_parsing_error(Parser *, expr_ty);
expr_ty _PyPegen_get_last_comprehension_item(comprehension_ty comprehension);
void *_PyPegen_nonparen_genexp_in_call(Parser *p, expr_ty args, asdl_comprehension_seq *comprehensions);
stmt_ty _PyPegen_checked_future_import(Parser *p, identifier module, asdl_alias_seq *,
                                       int , int, int , int , int , PyArena *);
asdl_stmt_seq* _PyPegen_register_stmts(Parser *p, asdl_stmt_seq* stmts);
stmt_ty _PyPegen_register_stmt(Parser *p, stmt_ty s);

// Parser API

Parser *_PyPegen_Parser_New(struct tok_state *, int, int, int, int *, const char*, PyArena *);
void _PyPegen_Parser_Free(Parser *);
mod_ty _PyPegen_run_parser_from_file_pointer(FILE *, int, PyObject *, const char *,
                                    const char *, const char *, PyCompilerFlags *, int *, PyObject **,
                                    PyArena *);
void *_PyPegen_run_parser(Parser *);
mod_ty _PyPegen_run_parser_from_string(const char *, int, PyObject *, PyCompilerFlags *, PyArena *);
asdl_stmt_seq *_PyPegen_interactive_exit(Parser *);

// Generated function in parse.c - function definition in python.gram
void *_PyPegen_parse(Parser *);

#endif
