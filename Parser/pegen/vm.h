typedef enum _opcodes {
    OP_NOOP,
    OP_NAME,
    OP_NUMBER,
    OP_STRING,
    OP_CUT,
    OP_OPTIONAL,
    OP_LOOP_START,
    OP_LOOP_COLLECT,
    OP_FAILURE,
    // The rest have an argument
    OP_TOKEN,
    OP_RULE,
    OP_RETURN,
    OP_LOOP_CONTINUE,
    OP_SUCCESS,
} Opcode;

char *opcode_names[] = {
    "OP_NOOP",
    "OP_NAME",
    "OP_NUMBER",
    "OP_STRING",
    "OP_CUT",
    "OP_OPTIONAL",
    "OP_LOOP_START",
    "OP_LOOP_COLLECT",
    "OP_FAILURE",
    // The rest have an argument
    "OP_TOKEN",
    "OP_RULE",
    "OP_RETURN",
    "OP_LOOP_CONTINUE",
    "OP_SUCCESS",
};

typedef struct _rule {
    char *name;
    int alts[10];
    int opcodes[100];
} Rule;

typedef struct _frame {
    Rule *rule;
    int ialt;
    int iop;
    int cut;
    int mark;
    int ival;
    void *vals[10];
} Frame;

typedef struct _stack {
    Parser *p;
    int top;
    Frame frames[100];
} Stack;

/* Toy grammar

NOTE: [expr] is optional to test OP_RULE_OPTIONAL.

start: stmt ENDMARKER
stmt: expr NEWLINE
expr: term '+' expr | term
term: NAME | NUMBER

*/

static const int n_keyword_lists = 0;
static KeywordToken *reserved_keywords[] = {
};

enum {
      R_START,
      R_STMT,
      R_EXPR,
      R_TERM,
};

enum {
      A_START_0,
      A_STMT_0,
      A_EXPR_0,
      A_EXPR_1,
      A_TERM_0,
      A_TERM_1,
};

static Rule all_rules[] = {

    {"start",
     {0, 4, -1},
     {
      OP_RULE, R_STMT,
      OP_SUCCESS, A_START_0,

      OP_FAILURE,
     },
    },

    {"stmt",
     {0, -1},
     {
      OP_RULE, R_EXPR,
      OP_RETURN, A_STMT_0,
     },
    },

    {"expr",
     {0, 8, -1},
     {
      OP_RULE, R_TERM,
      OP_TOKEN, PLUS,
      OP_RULE, R_EXPR,
      OP_RETURN, A_EXPR_0,

      OP_RULE, R_TERM,
      OP_RETURN, A_EXPR_1,
     },
    },

    {"term",
     {0, 3, -1},
     {
      OP_NAME,
      OP_RETURN, A_TERM_0,

      OP_NUMBER,
      OP_RETURN, A_TERM_1,
     },
    },

};

static void *
call_action(Parser *p, Frame *f, int iaction)
{
    Token *t = p->tokens[f->mark];
    int _start_lineno = t->lineno;
    int _start_col_offset = t->col_offset;
    t = p->tokens[p->mark - 1];
    int _end_lineno = t->end_lineno;
    int _end_col_offset = t->end_col_offset;

    switch (iaction) {
    case A_START_0:
        return  _PyPegen_make_module(p, f->vals[0]);
    case A_STMT_0:
        return _PyPegen_singleton_seq(p, _Py_Expr(f->vals[0], EXTRA));
    case A_EXPR_0:
        if (f->vals[2])
            return _Py_BinOp(f->vals[0], Add, f->vals[2], EXTRA);
        else
            return f->vals[0];
    case A_EXPR_1:
        return f->vals[0];
    case A_TERM_0:
        return f->vals[0];
    case A_TERM_1:
        return f->vals[0];
    default:
        assert(0);
    }
}
