#ifndef PEGEN_H
#define PEGEN_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <token.h>
#include <Python-ast.h>
#include <pyarena.h>
#include <setjmp.h>

enum INPUT_MODE {
    FILE_INPUT,
    STRING_INPUT,
};
typedef enum INPUT_MODE INPUT_MODE;

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
    struct tok_state *tok;
    Token **tokens;
    int mark;
    int fill, size;
    PyArena *arena;
    KeywordToken **keywords;
    int n_keyword_lists;
    void *start_rule_func;
    INPUT_MODE input_mode;
    jmp_buf error_env;
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
    asdl_seq *plain_names;
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

int insert_memo(Parser *p, int mark, int type, void *node);
int update_memo(Parser *p, int mark, int type, void *node);
int is_memoized(Parser *p, int type, void *pres);

int lookahead_with_string(int, void *(func)(Parser *, const char *), Parser *, const char *);
int lookahead_with_int(int, Token *(func)(Parser *, int), Parser *, int);
int lookahead(int, void *(func)(Parser *), Parser *);

Token *expect_token(Parser *p, int type);
Token *get_last_nonnwhitespace_token(Parser *);
int fill_token(Parser *p);
void *async_token(Parser *p);
void *await_token(Parser *p);
void *endmarker_token(Parser *p);
expr_ty name_token(Parser *p);
void *newline_token(Parser *p);
void *indent_token(Parser *p);
void *dedent_token(Parser *p);
expr_ty number_token(Parser *p);
void *string_token(Parser *p);
int raise_syntax_error(Parser *p, const char *errmsg, ...);
void *CONSTRUCTOR(Parser *p, ...);

#define UNUSED(expr) do { (void)(expr); } while (0)
#define EXTRA_EXPR(head, tail) head->lineno, head->col_offset, tail->end_lineno, tail->end_col_offset, p->arena
#define EXTRA start_lineno, start_col_offset, end_lineno, end_col_offset, p->arena
#define CHECK(result) CHECK_CALL(p, result)
#define CHECK_NULL_ALLOWED(result) CHECK_CALL_NULL_ALLOWED(p, result)

inline void *
CHECK_CALL(Parser *p, void *result)
{
    if (result == NULL) {
        assert(PyErr_Occurred());
        longjmp(p->error_env, 1);
    }
    return result;
}

/* This is needed for helper functions that are allowed to
   return NULL without an error. Example: seq_extract_starred_exprs */
inline void *
CHECK_CALL_NULL_ALLOWED(Parser *p, void *result)
{
    if (result == NULL && PyErr_Occurred()) {
        longjmp(p->error_env, 1);
    }
    return result;
}

PyObject *new_identifier(Parser *, char *);
PyObject *run_parser_from_file(const char *filename,
                               void *(start_rule_func)(Parser *),
                               int mode,
                               KeywordToken **keywords_list,
                               int n_keyword_lists);
PyObject *run_parser_from_string(const char *str,
                                 void *(start_rule_func)(Parser *),
                                 int mode,
                                 KeywordToken **keywords_list,
                                 int n_keyword_lists);
asdl_seq *singleton_seq(Parser *, void *);
asdl_seq *seq_insert_in_front(Parser *, void *, asdl_seq *);
asdl_seq *seq_flatten(Parser *, asdl_seq *);
expr_ty join_names_with_dot(Parser *, expr_ty, expr_ty);
int seq_count_dots(asdl_seq *);
alias_ty alias_for_star(Parser *);
asdl_seq *map_names_to_ids(Parser *, asdl_seq *);
CmpopExprPair *cmpop_expr_pair(Parser *, cmpop_ty, expr_ty);
asdl_int_seq *get_cmpops(Parser *p, asdl_seq *);
asdl_seq *get_exprs(Parser *, asdl_seq *);
expr_ty set_expr_context(Parser *, expr_ty, expr_context_ty);
KeyValuePair *key_value_pair(Parser *, expr_ty, expr_ty);
asdl_seq *get_keys(Parser *, asdl_seq *);
asdl_seq *get_values(Parser *, asdl_seq *);
NameDefaultPair *name_default_pair(Parser *, arg_ty, expr_ty);
SlashWithDefault *slash_with_default(Parser *, asdl_seq *, asdl_seq *);
StarEtc *star_etc(Parser *, arg_ty, asdl_seq *, arg_ty);
arguments_ty make_arguments(Parser *, asdl_seq *, SlashWithDefault *,
                            asdl_seq *, asdl_seq *, StarEtc *);
arguments_ty empty_arguments(Parser *);
AugOperator *augoperator(Parser*, operator_ty type);
stmt_ty function_def_decorators(Parser *, asdl_seq *, stmt_ty);
stmt_ty class_def_decorators(Parser *, asdl_seq *, stmt_ty);
KeywordOrStarred *keyword_or_starred(Parser *, void *, int);
asdl_seq *seq_extract_starred_exprs(Parser *, asdl_seq *);
asdl_seq *seq_delete_starred_exprs(Parser *, asdl_seq *);
expr_ty concatenate_strings(Parser *p, asdl_seq *);

#endif
