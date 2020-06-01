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
    SK___PEG_PARSER__,
};

static const char *soft_keywords[] = {
    "__peg_parser__",
};

enum {
    R_START,
    R_STMT,
    R_IF_STMT,
    R_EXPR,
    R_TERM,
    R_FACTOR,
    R_ROOT,
    R__LOOP1_1,
    R__GATHER_2,
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
    A_FACTOR_3,
    A_FACTOR_4,
    A__GATHER_2_0,
    A__GATHER_2_1,
};

static Rule all_rules[] = {
    {"start",
     R_START,
     {0, -1},
     {
        OP_RULE, R__LOOP1_1, OP_TOKEN, ENDMARKER, OP_RETURN, A_START_0,
     },
    },
    {"stmt",
     R_STMT,
     {0, 10, -1},
     {
        OP_SAVE_MARK, OP_TOKEN, 500, OP_NEG_LOOKAHEAD, OP_RULE, R_EXPR, OP_TOKEN, NEWLINE, OP_RETURN, A_STMT_0,
        OP_SAVE_MARK, OP_TOKEN, 500, OP_POS_LOOKAHEAD, OP_RULE, R_IF_STMT, OP_RETURN, A_STMT_1,
     },
    },
    {"if_stmt",
     R_IF_STMT,
     {0, -1},
     {
        OP_TOKEN, 500, OP_NAME, OP_TOKEN, COLON, OP_RULE, R_STMT, OP_RETURN, A_IF_STMT_0,
     },
    },
    {"expr",
     R_EXPR,
     {0, 8, -1},
     {
        OP_RULE, R_TERM, OP_TOKEN, PLUS, OP_RULE, R_EXPR, OP_RETURN, A_EXPR_0,
        OP_RULE, R_TERM, OP_RETURN, A_EXPR_1,
     },
    },
    {"term",
     R_TERM,
     {0, 8, -1},
     {
        OP_RULE, R_FACTOR, OP_TOKEN, STAR, OP_RULE, R_TERM, OP_RETURN, A_TERM_0,
        OP_RULE, R_FACTOR, OP_RETURN, A_TERM_1,
     },
    },
    {"factor",
     R_FACTOR,
     {0, 8, 16, 19, 23, -1},
     {
        OP_TOKEN, LPAR, OP_RULE, R_EXPR, OP_TOKEN, RPAR, OP_RETURN, A_FACTOR_0,
        OP_TOKEN, LSQB, OP_RULE, R__GATHER_2, OP_TOKEN, RSQB, OP_RETURN, A_FACTOR_1,
        OP_NUMBER, OP_RETURN, A_FACTOR_2,
        OP_SOFT_KEYWORD, SK___PEG_PARSER__, OP_RETURN, A_FACTOR_3,
        OP_NAME, OP_RETURN, A_FACTOR_4,
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
    {"_loop1_1",
     R__LOOP1_1,
     {0, 3, -1},
     {
        OP_RULE, R_STMT, OP_LOOP_ITERATE,
        OP_LOOP_COLLECT_NONEMPTY,
     },
    },
    {"_gather_2",
     R__GATHER_2,
     {0, 5, -1},
     {
        OP_RULE, R_EXPR, OP_TOKEN, COMMA, OP_LOOP_ITERATE,
        OP_RULE, R_EXPR, OP_LOOP_COLLECT_DELIMITED,
     },
    },
};

static void *
call_action(Parser *p, Frame *_f, int _iaction)
{
    assert(p->mark > 0);
    Token *_t = p->tokens[_f->mark];
    int _start_lineno = _t->lineno;
    int _start_col_offset = _t->col_offset;
    _t = p->tokens[p->mark - 1];
    int _end_lineno = _t->end_lineno;
    int _end_col_offset = _t->end_col_offset;

    switch (_iaction) {
    case A_START_0:
        return _PyPegen_make_module ( p , _f->vals[0] );
    case A_STMT_0:
        return _Py_Expr ( _f->vals[0] , EXTRA );
    case A_STMT_1:
    case A_EXPR_1:
    case A_TERM_1:
    case A_FACTOR_2:
    case A_FACTOR_4:
    case A__GATHER_2_0:
    case A__GATHER_2_1:
        return _f->vals[0];
    case A_IF_STMT_0:
        return _Py_If ( _f->vals[1] , CHECK ( _PyPegen_singleton_seq ( p , _f->vals[3] ) ) , NULL , EXTRA );
    case A_EXPR_0:
        return _Py_BinOp ( _f->vals[0] , Add , _f->vals[2] , EXTRA );
    case A_TERM_0:
        return _Py_BinOp ( _f->vals[0] , Mult , _f->vals[2] , EXTRA );
    case A_FACTOR_0:
        return _f->vals[1];
    case A_FACTOR_1:
        return _Py_List ( _f->vals[1] , Load , EXTRA );
    case A_FACTOR_3:
        return RAISE_SYNTAX_ERROR ( "You found it!" );
    default:
        assert(0);
    }
}
