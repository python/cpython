static const int n_keyword_lists = 3;
static KeywordToken *reserved_keywords[] = {
    NULL,
    NULL,
    (KeywordToken[]) {
        {"if", 500},
        {NULL, -1},
    },
};

enum {
    R_START,
    R_STMT,
    R_IF_STMT,
    R_EXPR,
    R_TERM,
    R_FACTOR,
    R_ROOT,
    R__LOOP0_1,
};

enum {
    A_START_0,
    A_STMT_0,
    A_STMT_1,
    A_IF_STMT_0,
    A_EXPR_0,
    A_EXPR_1,
    A_TERM_0,
    A_TERM_1,
    A_FACTOR_0,
    A_FACTOR_1,
    A_FACTOR_2,
};

static Rule all_rules[] = {
    {"start",
     R_START,
     {0, -1},
     {
        OP_RULE, R__LOOP0_1, OP_TOKEN, ENDMARKER, OP_RETURN, A_START_0,
     },
    },
    {"stmt",
     R_STMT,
     {0, 6, -1},
     {
        OP_RULE, R_EXPR, OP_TOKEN, NEWLINE, OP_RETURN, A_STMT_0,
        OP_RULE, R_IF_STMT, OP_TOKEN, NEWLINE, OP_RETURN, A_STMT_1,
     },
    },
    {"if_stmt",
     R_IF_STMT,
     {0, -1},
     {
        OP_TOKEN, 500, OP_NAME, OP_TOKEN, 11, OP_RULE, R_EXPR, OP_RETURN, A_IF_STMT_0,
     },
    },
    {"expr",
     R_EXPR,
     {0, 8, -1},
     {
        OP_RULE, R_TERM, OP_TOKEN, 14, OP_RULE, R_EXPR, OP_RETURN, A_EXPR_0,
        OP_RULE, R_TERM, OP_RETURN, A_EXPR_1,
     },
    },
    {"term",
     R_TERM,
     {0, 8, -1},
     {
        OP_RULE, R_FACTOR, OP_TOKEN, 16, OP_RULE, R_TERM, OP_RETURN, A_TERM_0,
        OP_RULE, R_FACTOR, OP_RETURN, A_TERM_1,
     },
    },
    {"factor",
     R_FACTOR,
     {0, 8, 11, -1},
     {
        OP_TOKEN, 7, OP_RULE, R_EXPR, OP_TOKEN, 8, OP_RETURN, A_FACTOR_0,
        OP_NUMBER, OP_RETURN, A_FACTOR_1,
        OP_NAME, OP_RETURN, A_FACTOR_2,
     },
    },
    {"root",
     R_ROOT,
     {0, 3, -1},
     {
        OP_RULE, R_START, OP_SUCCESS,
        OP_FAILURE,
     },
    },
    {"_loop0_1",
     R__LOOP0_1,
     {0, 3, -1},
     {
        OP_RULE, R_STMT, OP_LOOP_ITERATE,
        OP_LOOP_COLLECT,
     },
    },
};

static void *
call_action(Parser *p, Frame *f, int iaction)
{
    assert(p->mark > 0);
    Token *t = p->tokens[f->mark];
    int _start_lineno = t->lineno;
    int _start_col_offset = t->col_offset;
    t = p->tokens[p->mark - 1];
    int _end_lineno = t->end_lineno;
    int _end_col_offset = t->end_col_offset;

    switch (iaction) {
    case A_START_0:
        return _PyPegen_make_module ( p , f->vals[0] );
    case A_STMT_0:
        return _Py_Expr ( f->vals[0] , EXTRA );
    case A_STMT_1:
    case A_EXPR_1:
    case A_TERM_1:
    case A_FACTOR_1:
    case A_FACTOR_2:
        return f->vals[0];
    case A_IF_STMT_0:
        return _Py_If ( f->vals[1] , f->vals[3] , NULL , EXTRA );
    case A_EXPR_0:
        return _Py_BinOp ( f->vals[0] , Add , f->vals[2] , EXTRA );
    case A_TERM_0:
        return _Py_BinOp ( f->vals[0] , Mult , f->vals[2] , EXTRA );
    case A_FACTOR_0:
        return f->vals[1];
    default:
        assert(0);
    }
}
