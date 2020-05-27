typedef enum _opcodes {
    OP_NAME,
    OP_NUMBER,
    OP_STRING,
    OP_CUT,
    OP_SUCCESS,
    OP_FAILURE,
    // The rest have an argument
    OP_TOKEN,
    OP_TOKEN_OPTIONAL,
    OP_RULE,
    OP_RULE_OPTIONAL,
    OP_RETURN,
} Opcode;

char *opcode_names[] = {
    "OP_NAME",
    "OP_NUMBER",
    "OP_STRING",
    "OP_CUT",
    "OP_SUCCESS",
    "OP_FAILURE",
    // The rest have an argument
    "OP_TOKEN",
    "OP_TOKEN_OPTIONAL",
    "OP_RULE",
    "OP_RULE_OPTIONAL",
    "OP_RETURN",
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

/* Toy grammar:

start: expr ENDMARKER
expr: term '+' expr | term
term: NAME | NUMBER

*/

static const int n_keyword_lists = 0;
static KeywordToken *reserved_keywords[] = {
};

static Rule all_rules[] = {

    {"start",
     {0, 6, -1},
     {
      OP_RULE, 1, OP_TOKEN, NEWLINE, OP_SUCCESS, 0,
      OP_FAILURE,
     },
    },

    {"expr",
     {0, 8, -1},
     {
      OP_RULE, 2, OP_TOKEN_OPTIONAL, PLUS, OP_RULE, 1, OP_RETURN, 1,
      OP_RULE, 2, OP_RETURN, 2,
     },
    },

    {"term",
     {0, 3, -1},
     {
      OP_NAME, OP_RETURN, 3,
      OP_NUMBER, OP_RETURN, 4,
     },
    },

};

static void *
call_action(Parser *p, Frame *f, int iaction)
{
    // TODO: compute lineno and col offset (put in frame?)
    int _start_lineno = 1;
    int _start_col_offset = 1;
    int _end_lineno = 99;
    int _end_col_offset = 99;

    switch (iaction) {
    case 0:
        return  _PyPegen_make_module(p, _PyPegen_singleton_seq(p, _Py_Expr(f->vals[0], EXTRA)));
    case 1:
        return _Py_BinOp(f->vals[0], Add, f->vals[2], EXTRA);
    case 2:
        return f->vals[0];
    case 3:
        return f->vals[0];
    case 4:
        return f->vals[0];
    default:
        assert(0);
    }
}
