/* Toy grammar

NOTE: [expr] is right-recursive.

start: stmt* ENDMARKER
stmt: expr NEWLINE
expr: term '+' expr | term
term: factor '*' term | factor
factor: '(' expr ')' | NUMBER | NAME

*/

static const int n_keyword_lists = 0;
static KeywordToken *reserved_keywords[] = {
};

enum {
      R_ROOT,
      R_START,
      R_STMTS,
      R_STMT,
      R_EXPR,
      R_TERM,
      R_FACTOR,
};

enum {
      A_START_0,
      A_STMT_0,
      A_EXPR_0,
      A_EXPR_1,
      A_TERM_0,
      A_TERM_1,
      A_FACTOR_0,
      A_FACTOR_1,
      A_FACTOR_2,
};

static Rule all_rules[] = {

    {"root",
     R_ROOT,
     {0, 3, -1},
     {
      OP_RULE, R_START,
      OP_SUCCESS,

      OP_FAILURE,
     },
    },

    {"start",
     R_START,
     {0, 6, -1},
     {
      OP_RULE, R_STMTS,
      OP_TOKEN, ENDMARKER,
      OP_RETURN, A_START_0,

      OP_FAILURE,
     },
    },

    {"stmt*",
     R_STMTS,
     {0, 3, -1},
     {
      OP_RULE, R_STMT,
      OP_LOOP_ITERATE,

      OP_LOOP_COLLECT,
     },
    },

    {"stmt",
     R_STMT,
     {0, -1},
     {
      OP_RULE, R_EXPR,
      OP_TOKEN, NEWLINE,
      OP_RETURN, A_STMT_0,
     },
    },

    {"expr",
     R_EXPR,
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
     R_TERM,
     {0, 8, -1},
     {
      OP_RULE, R_FACTOR,
      OP_TOKEN, STAR,
      OP_RULE, R_TERM,
      OP_RETURN, A_TERM_0,

      OP_RULE, R_FACTOR,
      OP_RETURN, A_TERM_1,
     },
    },

    {"factor",
     R_FACTOR,
     {0, 8, 11, -1},
     {
      OP_TOKEN, LPAR,
      OP_RULE, R_EXPR,
      OP_TOKEN, RPAR,
      OP_RETURN, A_FACTOR_0,

      OP_NUMBER,
      OP_RETURN, A_FACTOR_1,

      OP_NAME,
      OP_RETURN, A_FACTOR_2,
     },
    },

};

static void *
call_action(Parser *p, Frame *f, int iaction)
{
    Token *t = p->tokens[f->mark];
    int _start_lineno = t->lineno;
    int _start_col_offset = t->col_offset;
    if (p->mark > 0) {
        t = p->tokens[p->mark - 1];
    }
    else {
        // Debug anomaly rather than crashing
        printf("p->mark=%d, p->fill=%d, f->mark=%d\n", p->mark, p->fill, f->mark);
    }
    int _end_lineno = t->end_lineno;
    int _end_col_offset = t->end_col_offset;

    switch (iaction) {
    case A_START_0:
        return  _PyPegen_make_module(p, f->vals[0]);
    case A_STMT_0:
        return _Py_Expr(f->vals[0], EXTRA);
    case A_EXPR_0:
        return _Py_BinOp(f->vals[0], Add, f->vals[2], EXTRA);
    case A_EXPR_1:
        return f->vals[0];
    case A_TERM_0:
        return f->vals[0];
    case A_TERM_1:
        return f->vals[0];
    case A_FACTOR_0:
        return f->vals[1];
    case A_FACTOR_1:
        return f->vals[0];
    case A_FACTOR_2:
        return f->vals[0];
    default:
        assert(0);
    }
}
