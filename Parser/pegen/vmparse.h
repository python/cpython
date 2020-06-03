static const int n_keyword_lists = 15;
static KeywordToken *reserved_keywords[] = {
    NULL,
    NULL,
    (KeywordToken[]) {
        {"if", 510},
        {"in", 518},
        {"is", 526},
        {"as", 531},
        {"or", 532},
        {NULL, -1},
    },
    (KeywordToken[]) {
        {"del", 503},
        {"try", 511},
        {"for", 517},
        {"def", 522},
        {"not", 525},
        {"and", 533},
        {NULL, -1},
    },
    (KeywordToken[]) {
        {"pass", 502},
        {"from", 514},
        {"elif", 515},
        {"else", 516},
        {"with", 519},
        {"True", 527},
        {"None", 529},
        {NULL, -1},
    },
    (KeywordToken[]) {
        {"raise", 501},
        {"yield", 504},
        {"break", 506},
        {"while", 512},
        {"class", 523},
        {"False", 528},
        {NULL, -1},
    },
    (KeywordToken[]) {
        {"return", 500},
        {"assert", 505},
        {"global", 508},
        {"import", 513},
        {"except", 520},
        {"lambda", 524},
        {NULL, -1},
    },
    (KeywordToken[]) {
        {"finally", 521},
        {NULL, -1},
    },
    (KeywordToken[]) {
        {"continue", 507},
        {"nonlocal", 509},
        {NULL, -1},
    },
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (KeywordToken[]) {
        {"__new_parser__", 530},
        {NULL, -1},
    },
};

static const char *soft_keywords[] = {
};

enum {
    R_FILE,
    R_INTERACTIVE,
    R_EVAL,
    R_FUNC_TYPE,
    R_FSTRING,
    R_TYPE_EXPRESSIONS,
    R_STATEMENTS,
    R_STATEMENT,
    R_STATEMENT_NEWLINE,
    R_SIMPLE_STMT,
    R_SMALL_STMT,
    R_COMPOUND_STMT,
    R_ASSIGNMENT,
    R_AUGASSIGN,
    R_GLOBAL_STMT,
    R_NONLOCAL_STMT,
    R_YIELD_STMT,
    R_ASSERT_STMT,
    R_DEL_STMT,
    R_IMPORT_STMT,
    R_IMPORT_NAME,
    R_IMPORT_FROM,
    R_IMPORT_FROM_TARGETS,
    R_IMPORT_FROM_AS_NAMES,
    R_IMPORT_FROM_AS_NAME,
    R_DOTTED_AS_NAMES,
    R_DOTTED_AS_NAME,
    R_DOTTED_NAME,
    R_IF_STMT,
    R_ELIF_STMT,
    R_ELSE_BLOCK,
    R_WHILE_STMT,
    R_FOR_STMT,
    R_WITH_STMT,
    R_WITH_ITEM,
    R_TRY_STMT,
    R_EXCEPT_BLOCK,
    R_FINALLY_BLOCK,
    R_RETURN_STMT,
    R_RAISE_STMT,
    R_FUNCTION_DEF,
    R_FUNCTION_DEF_RAW,
    R_FUNC_TYPE_COMMENT,
    R_PARAMS,
    R_PARAMETERS,
    R_SLASH_NO_DEFAULT,
    R_SLASH_WITH_DEFAULT,
    R_STAR_ETC,
    R_KWDS,
    R_PARAM_NO_DEFAULT,
    R_PARAM_WITH_DEFAULT,
    R_PARAM_MAYBE_DEFAULT,
    R_PARAM,
    R_ANNOTATION,
    R_DEFAULT,
    R_DECORATORS,
    R_CLASS_DEF,
    R_CLASS_DEF_RAW,
    R_BLOCK,
    R_EXPRESSIONS_LIST,
    R_STAR_EXPRESSIONS,
    R_STAR_EXPRESSION,
    R_STAR_NAMED_EXPRESSIONS,
    R_STAR_NAMED_EXPRESSION,
    R_NAMED_EXPRESSION,
    R_ANNOTATED_RHS,
    R_EXPRESSIONS,
    R_EXPRESSION,
    R_LAMBDEF,
    R_LAMBDA_PARAMETERS,
    R_LAMBDA_SLASH_NO_DEFAULT,
    R_LAMBDA_SLASH_WITH_DEFAULT,
    R_LAMBDA_STAR_ETC,
    R_LAMBDA_KWDS,
    R_LAMBDA_PARAM_NO_DEFAULT,
    R_LAMBDA_PARAM_WITH_DEFAULT,
    R_LAMBDA_PARAM_MAYBE_DEFAULT,
    R_LAMBDA_PARAM,
    R_DISJUNCTION,
    R_CONJUNCTION,
    R_INVERSION,
    R_COMPARISON,
    R_COMPARE_OP_BITWISE_OR_PAIR,
    R_EQ_BITWISE_OR,
    R_NOTEQ_BITWISE_OR,
    R_LTE_BITWISE_OR,
    R_LT_BITWISE_OR,
    R_GTE_BITWISE_OR,
    R_GT_BITWISE_OR,
    R_NOTIN_BITWISE_OR,
    R_IN_BITWISE_OR,
    R_ISNOT_BITWISE_OR,
    R_IS_BITWISE_OR,
    R_BITWISE_OR,
    R_BITWISE_XOR,
    R_BITWISE_AND,
    R_SHIFT_EXPR,
    R_SUM,
    R_TERM,
    R_FACTOR,
    R_POWER,
    R_AWAIT_PRIMARY,
    R_PRIMARY,
    R_SLICES,
    R_SLICE,
    R_ATOM,
    R_STRINGS,
    R_LIST,
    R_LISTCOMP,
    R_TUPLE,
    R_GROUP,
    R_GENEXP,
    R_SET,
    R_SETCOMP,
    R_DICT,
    R_DICTCOMP,
    R_DOUBLE_STARRED_KVPAIRS,
    R_DOUBLE_STARRED_KVPAIR,
    R_KVPAIR,
    R_FOR_IF_CLAUSES,
    R_FOR_IF_CLAUSE,
    R_YIELD_EXPR,
    R_ARGUMENTS,
    R_ARGS,
    R_KWARGS,
    R_STARRED_EXPRESSION,
    R_KWARG_OR_STARRED,
    R_KWARG_OR_DOUBLE_STARRED,
    R_STAR_TARGETS,
    R_STAR_TARGETS_SEQ,
    R_STAR_TARGET,
    R_STAR_ATOM,
    R_SINGLE_TARGET,
    R_SINGLE_SUBSCRIPT_ATTRIBUTE_TARGET,
    R_DEL_TARGETS,
    R_DEL_TARGET,
    R_DEL_T_ATOM,
    R_DEL_TARGET_END,
    R_TARGETS,
    R_TARGET,
    R_T_PRIMARY,
    R_T_LOOKAHEAD,
    R_T_ATOM,
    R_INCORRECT_ARGUMENTS,
    R_INVALID_KWARG,
    R_INVALID_NAMED_EXPRESSION,
    R_INVALID_ASSIGNMENT,
    R_INVALID_BLOCK,
    R_INVALID_COMPREHENSION,
    R_INVALID_DICT_COMPREHENSION,
    R_INVALID_PARAMETERS,
    R_INVALID_STAR_ETC,
    R_INVALID_LAMBDA_STAR_ETC,
    R_INVALID_DOUBLE_TYPE_COMMENTS,
    R_INVALID_DEL_TARGET,
    R_INVALID_IMPORT_FROM_TARGETS,
    R_ROOT,
    R__LOOP0_1,
    R__LOOP0_2,
    R__GATHER_3,
    R__GATHER_4,
    R__GATHER_5,
    R__GATHER_6,
    R__LOOP1_7,
    R__GATHER_8,
    R__TMP_9,
    R__TMP_10,
    R__TMP_11,
    R__TMP_12,
    R__TMP_13,
    R__TMP_14,
    R__TMP_15,
    R__TMP_16,
    R__LOOP1_17,
    R__TMP_18,
    R__TMP_19,
    R__GATHER_20,
    R__GATHER_21,
    R__TMP_22,
    R__LOOP0_23,
    R__LOOP1_24,
    R__GATHER_25,
    R__TMP_26,
    R__GATHER_27,
    R__TMP_28,
    R__GATHER_29,
    R__GATHER_30,
    R__GATHER_31,
    R__GATHER_32,
    R__TMP_33,
    R__LOOP1_34,
    R__TMP_35,
    R__TMP_36,
    R__TMP_37,
    R__TMP_38,
    R__TMP_39,
    R__LOOP0_40,
    R__LOOP0_41,
    R__LOOP0_42,
    R__LOOP1_43,
    R__LOOP0_44,
    R__LOOP1_45,
    R__LOOP1_46,
    R__LOOP1_47,
    R__LOOP0_48,
    R__LOOP1_49,
    R__LOOP0_50,
    R__LOOP1_51,
    R__LOOP0_52,
    R__LOOP1_53,
    R__LOOP1_54,
    R__TMP_55,
    R__GATHER_56,
    R__LOOP1_57,
    R__GATHER_58,
    R__LOOP1_59,
    R__LOOP0_60,
    R__LOOP0_61,
    R__LOOP0_62,
    R__LOOP1_63,
    R__LOOP0_64,
    R__LOOP1_65,
    R__LOOP1_66,
    R__LOOP1_67,
    R__LOOP0_68,
    R__LOOP1_69,
    R__LOOP0_70,
    R__LOOP1_71,
    R__LOOP0_72,
    R__LOOP1_73,
    R__LOOP1_74,
    R__LOOP1_75,
    R__LOOP1_76,
    R__TMP_77,
    R__GATHER_78,
    R__TMP_79,
    R__TMP_80,
    R__TMP_81,
    R__TMP_82,
    R__LOOP1_83,
    R__TMP_84,
    R__TMP_85,
    R__GATHER_86,
    R__LOOP1_87,
    R__LOOP0_88,
    R__LOOP0_89,
    R__TMP_90,
    R__TMP_91,
    R__GATHER_92,
    R__GATHER_93,
    R__GATHER_94,
    R__GATHER_95,
    R__LOOP0_96,
    R__GATHER_97,
    R__TMP_98,
    R__GATHER_99,
    R__GATHER_100,
    R__TMP_101,
    R__LOOP0_102,
    R__TMP_103,
    R__TMP_104,
    R__TMP_105,
    R__TMP_106,
    R__LOOP0_107,
    R__TMP_108,
    R__TMP_109,
    R__TMP_110,
    R__TMP_111,
    R__TMP_112,
    R__TMP_113,
    R__TMP_114,
    R__TMP_115,
    R__TMP_116,
    R__TMP_117,
    R__TMP_118,
    R__TMP_119,
    R__TMP_120,
    R__TMP_121,
    R__LOOP1_122,
    R__TMP_123,
    R__TMP_124,
};

enum {
    A_FILE_0,
    A_INTERACTIVE_0,
    A_EVAL_0,
    A_FUNC_TYPE_0,
    A_FSTRING_0,
    A_TYPE_EXPRESSIONS_0,
    A_TYPE_EXPRESSIONS_1,
    A_TYPE_EXPRESSIONS_2,
    A_TYPE_EXPRESSIONS_3,
    A_TYPE_EXPRESSIONS_4,
    A_TYPE_EXPRESSIONS_5,
    A_TYPE_EXPRESSIONS_6,
    A_STATEMENTS_0,
    A_STATEMENT_0,
    A_STATEMENT_1,
    A_STATEMENT_NEWLINE_0,
    A_STATEMENT_NEWLINE_1,
    A_STATEMENT_NEWLINE_2,
    A_STATEMENT_NEWLINE_3,
    A_SIMPLE_STMT_0,
    A_SIMPLE_STMT_1,
    A_SMALL_STMT_0,
    A_SMALL_STMT_1,
    A_SMALL_STMT_2,
    A_SMALL_STMT_3,
    A_SMALL_STMT_4,
    A_SMALL_STMT_5,
    A_SMALL_STMT_6,
    A_SMALL_STMT_7,
    A_SMALL_STMT_8,
    A_SMALL_STMT_9,
    A_SMALL_STMT_10,
    A_SMALL_STMT_11,
    A_SMALL_STMT_12,
    A_COMPOUND_STMT_0,
    A_COMPOUND_STMT_1,
    A_COMPOUND_STMT_2,
    A_COMPOUND_STMT_3,
    A_COMPOUND_STMT_4,
    A_COMPOUND_STMT_5,
    A_COMPOUND_STMT_6,
    A_ASSIGNMENT_0,
    A_ASSIGNMENT_1,
    A_ASSIGNMENT_2,
    A_ASSIGNMENT_3,
    A_ASSIGNMENT_4,
    A_AUGASSIGN_0,
    A_AUGASSIGN_1,
    A_AUGASSIGN_2,
    A_AUGASSIGN_3,
    A_AUGASSIGN_4,
    A_AUGASSIGN_5,
    A_AUGASSIGN_6,
    A_AUGASSIGN_7,
    A_AUGASSIGN_8,
    A_AUGASSIGN_9,
    A_AUGASSIGN_10,
    A_AUGASSIGN_11,
    A_AUGASSIGN_12,
    A_GLOBAL_STMT_0,
    A_NONLOCAL_STMT_0,
    A_YIELD_STMT_0,
    A_ASSERT_STMT_0,
    A_DEL_STMT_0,
    A_IMPORT_STMT_0,
    A_IMPORT_STMT_1,
    A_IMPORT_NAME_0,
    A_IMPORT_FROM_0,
    A_IMPORT_FROM_1,
    A_IMPORT_FROM_TARGETS_0,
    A_IMPORT_FROM_TARGETS_1,
    A_IMPORT_FROM_TARGETS_2,
    A_IMPORT_FROM_TARGETS_3,
    A_IMPORT_FROM_AS_NAMES_0,
    A_IMPORT_FROM_AS_NAME_0,
    A_DOTTED_AS_NAMES_0,
    A_DOTTED_AS_NAME_0,
    A_DOTTED_NAME_0,
    A_DOTTED_NAME_1,
    A_IF_STMT_0,
    A_IF_STMT_1,
    A_ELIF_STMT_0,
    A_ELIF_STMT_1,
    A_ELSE_BLOCK_0,
    A_WHILE_STMT_0,
    A_FOR_STMT_0,
    A_FOR_STMT_1,
    A_WITH_STMT_0,
    A_WITH_STMT_1,
    A_WITH_STMT_2,
    A_WITH_STMT_3,
    A_WITH_ITEM_0,
    A_TRY_STMT_0,
    A_TRY_STMT_1,
    A_EXCEPT_BLOCK_0,
    A_EXCEPT_BLOCK_1,
    A_FINALLY_BLOCK_0,
    A_RETURN_STMT_0,
    A_RAISE_STMT_0,
    A_RAISE_STMT_1,
    A_FUNCTION_DEF_0,
    A_FUNCTION_DEF_1,
    A_FUNCTION_DEF_RAW_0,
    A_FUNCTION_DEF_RAW_1,
    A_FUNC_TYPE_COMMENT_0,
    A_FUNC_TYPE_COMMENT_1,
    A_FUNC_TYPE_COMMENT_2,
    A_PARAMS_0,
    A_PARAMS_1,
    A_PARAMETERS_0,
    A_PARAMETERS_1,
    A_PARAMETERS_2,
    A_PARAMETERS_3,
    A_PARAMETERS_4,
    A_SLASH_NO_DEFAULT_0,
    A_SLASH_NO_DEFAULT_1,
    A_SLASH_WITH_DEFAULT_0,
    A_SLASH_WITH_DEFAULT_1,
    A_STAR_ETC_0,
    A_STAR_ETC_1,
    A_STAR_ETC_2,
    A_STAR_ETC_3,
    A_KWDS_0,
    A_PARAM_NO_DEFAULT_0,
    A_PARAM_NO_DEFAULT_1,
    A_PARAM_WITH_DEFAULT_0,
    A_PARAM_WITH_DEFAULT_1,
    A_PARAM_MAYBE_DEFAULT_0,
    A_PARAM_MAYBE_DEFAULT_1,
    A_PARAM_0,
    A_ANNOTATION_0,
    A_DEFAULT_0,
    A_DECORATORS_0,
    A_CLASS_DEF_0,
    A_CLASS_DEF_1,
    A_CLASS_DEF_RAW_0,
    A_BLOCK_0,
    A_BLOCK_1,
    A_BLOCK_2,
    A_EXPRESSIONS_LIST_0,
    A_STAR_EXPRESSIONS_0,
    A_STAR_EXPRESSIONS_1,
    A_STAR_EXPRESSIONS_2,
    A_STAR_EXPRESSION_0,
    A_STAR_EXPRESSION_1,
    A_STAR_NAMED_EXPRESSIONS_0,
    A_STAR_NAMED_EXPRESSION_0,
    A_STAR_NAMED_EXPRESSION_1,
    A_NAMED_EXPRESSION_0,
    A_NAMED_EXPRESSION_1,
    A_NAMED_EXPRESSION_2,
    A_ANNOTATED_RHS_0,
    A_ANNOTATED_RHS_1,
    A_EXPRESSIONS_0,
    A_EXPRESSIONS_1,
    A_EXPRESSIONS_2,
    A_EXPRESSION_0,
    A_EXPRESSION_1,
    A_EXPRESSION_2,
    A_LAMBDEF_0,
    A_LAMBDA_PARAMETERS_0,
    A_LAMBDA_PARAMETERS_1,
    A_LAMBDA_PARAMETERS_2,
    A_LAMBDA_PARAMETERS_3,
    A_LAMBDA_PARAMETERS_4,
    A_LAMBDA_SLASH_NO_DEFAULT_0,
    A_LAMBDA_SLASH_NO_DEFAULT_1,
    A_LAMBDA_SLASH_WITH_DEFAULT_0,
    A_LAMBDA_SLASH_WITH_DEFAULT_1,
    A_LAMBDA_STAR_ETC_0,
    A_LAMBDA_STAR_ETC_1,
    A_LAMBDA_STAR_ETC_2,
    A_LAMBDA_STAR_ETC_3,
    A_LAMBDA_KWDS_0,
    A_LAMBDA_PARAM_NO_DEFAULT_0,
    A_LAMBDA_PARAM_NO_DEFAULT_1,
    A_LAMBDA_PARAM_WITH_DEFAULT_0,
    A_LAMBDA_PARAM_WITH_DEFAULT_1,
    A_LAMBDA_PARAM_MAYBE_DEFAULT_0,
    A_LAMBDA_PARAM_MAYBE_DEFAULT_1,
    A_LAMBDA_PARAM_0,
    A_DISJUNCTION_0,
    A_DISJUNCTION_1,
    A_CONJUNCTION_0,
    A_CONJUNCTION_1,
    A_INVERSION_0,
    A_INVERSION_1,
    A_COMPARISON_0,
    A_COMPARISON_1,
    A_COMPARE_OP_BITWISE_OR_PAIR_0,
    A_COMPARE_OP_BITWISE_OR_PAIR_1,
    A_COMPARE_OP_BITWISE_OR_PAIR_2,
    A_COMPARE_OP_BITWISE_OR_PAIR_3,
    A_COMPARE_OP_BITWISE_OR_PAIR_4,
    A_COMPARE_OP_BITWISE_OR_PAIR_5,
    A_COMPARE_OP_BITWISE_OR_PAIR_6,
    A_COMPARE_OP_BITWISE_OR_PAIR_7,
    A_COMPARE_OP_BITWISE_OR_PAIR_8,
    A_COMPARE_OP_BITWISE_OR_PAIR_9,
    A_EQ_BITWISE_OR_0,
    A_NOTEQ_BITWISE_OR_0,
    A_LTE_BITWISE_OR_0,
    A_LT_BITWISE_OR_0,
    A_GTE_BITWISE_OR_0,
    A_GT_BITWISE_OR_0,
    A_NOTIN_BITWISE_OR_0,
    A_IN_BITWISE_OR_0,
    A_ISNOT_BITWISE_OR_0,
    A_IS_BITWISE_OR_0,
    A_BITWISE_OR_0,
    A_BITWISE_OR_1,
    A_BITWISE_XOR_0,
    A_BITWISE_XOR_1,
    A_BITWISE_AND_0,
    A_BITWISE_AND_1,
    A_SHIFT_EXPR_0,
    A_SHIFT_EXPR_1,
    A_SHIFT_EXPR_2,
    A_SUM_0,
    A_SUM_1,
    A_SUM_2,
    A_TERM_0,
    A_TERM_1,
    A_TERM_2,
    A_TERM_3,
    A_TERM_4,
    A_TERM_5,
    A_FACTOR_0,
    A_FACTOR_1,
    A_FACTOR_2,
    A_FACTOR_3,
    A_POWER_0,
    A_POWER_1,
    A_AWAIT_PRIMARY_0,
    A_AWAIT_PRIMARY_1,
    A_PRIMARY_0,
    A_PRIMARY_1,
    A_PRIMARY_2,
    A_PRIMARY_3,
    A_PRIMARY_4,
    A_SLICES_0,
    A_SLICES_1,
    A_SLICE_0,
    A_SLICE_1,
    A_ATOM_0,
    A_ATOM_1,
    A_ATOM_2,
    A_ATOM_3,
    A_ATOM_4,
    A_ATOM_5,
    A_ATOM_6,
    A_ATOM_7,
    A_ATOM_8,
    A_ATOM_9,
    A_ATOM_10,
    A_STRINGS_0,
    A_LIST_0,
    A_LISTCOMP_0,
    A_LISTCOMP_1,
    A_TUPLE_0,
    A_GROUP_0,
    A_GENEXP_0,
    A_GENEXP_1,
    A_SET_0,
    A_SETCOMP_0,
    A_SETCOMP_1,
    A_DICT_0,
    A_DICTCOMP_0,
    A_DICTCOMP_1,
    A_DOUBLE_STARRED_KVPAIRS_0,
    A_DOUBLE_STARRED_KVPAIR_0,
    A_DOUBLE_STARRED_KVPAIR_1,
    A_KVPAIR_0,
    A_FOR_IF_CLAUSES_0,
    A_FOR_IF_CLAUSE_0,
    A_FOR_IF_CLAUSE_1,
    A_YIELD_EXPR_0,
    A_YIELD_EXPR_1,
    A_ARGUMENTS_0,
    A_ARGUMENTS_1,
    A_ARGS_0,
    A_ARGS_1,
    A_ARGS_2,
    A_KWARGS_0,
    A_KWARGS_1,
    A_KWARGS_2,
    A_STARRED_EXPRESSION_0,
    A_KWARG_OR_STARRED_0,
    A_KWARG_OR_STARRED_1,
    A_KWARG_OR_STARRED_2,
    A_KWARG_OR_DOUBLE_STARRED_0,
    A_KWARG_OR_DOUBLE_STARRED_1,
    A_KWARG_OR_DOUBLE_STARRED_2,
    A_STAR_TARGETS_0,
    A_STAR_TARGETS_1,
    A_STAR_TARGETS_SEQ_0,
    A_STAR_TARGET_0,
    A_STAR_TARGET_1,
    A_STAR_TARGET_2,
    A_STAR_TARGET_3,
    A_STAR_ATOM_0,
    A_STAR_ATOM_1,
    A_STAR_ATOM_2,
    A_STAR_ATOM_3,
    A_SINGLE_TARGET_0,
    A_SINGLE_TARGET_1,
    A_SINGLE_TARGET_2,
    A_SINGLE_SUBSCRIPT_ATTRIBUTE_TARGET_0,
    A_SINGLE_SUBSCRIPT_ATTRIBUTE_TARGET_1,
    A_DEL_TARGETS_0,
    A_DEL_TARGET_0,
    A_DEL_TARGET_1,
    A_DEL_TARGET_2,
    A_DEL_T_ATOM_0,
    A_DEL_T_ATOM_1,
    A_DEL_T_ATOM_2,
    A_DEL_T_ATOM_3,
    A_DEL_T_ATOM_4,
    A_DEL_TARGET_END_0,
    A_DEL_TARGET_END_1,
    A_DEL_TARGET_END_2,
    A_DEL_TARGET_END_3,
    A_DEL_TARGET_END_4,
    A_TARGETS_0,
    A_TARGET_0,
    A_TARGET_1,
    A_TARGET_2,
    A_T_PRIMARY_0,
    A_T_PRIMARY_1,
    A_T_PRIMARY_2,
    A_T_PRIMARY_3,
    A_T_PRIMARY_4,
    A_T_LOOKAHEAD_0,
    A_T_LOOKAHEAD_1,
    A_T_LOOKAHEAD_2,
    A_T_ATOM_0,
    A_T_ATOM_1,
    A_T_ATOM_2,
    A_T_ATOM_3,
    A_INCORRECT_ARGUMENTS_0,
    A_INCORRECT_ARGUMENTS_1,
    A_INCORRECT_ARGUMENTS_2,
    A_INCORRECT_ARGUMENTS_3,
    A_INCORRECT_ARGUMENTS_4,
    A_INVALID_KWARG_0,
    A_INVALID_NAMED_EXPRESSION_0,
    A_INVALID_ASSIGNMENT_0,
    A_INVALID_ASSIGNMENT_1,
    A_INVALID_ASSIGNMENT_2,
    A_INVALID_ASSIGNMENT_3,
    A_INVALID_ASSIGNMENT_4,
    A_INVALID_ASSIGNMENT_5,
    A_INVALID_BLOCK_0,
    A_INVALID_COMPREHENSION_0,
    A_INVALID_DICT_COMPREHENSION_0,
    A_INVALID_PARAMETERS_0,
    A_INVALID_STAR_ETC_0,
    A_INVALID_STAR_ETC_1,
    A_INVALID_LAMBDA_STAR_ETC_0,
    A_INVALID_DOUBLE_TYPE_COMMENTS_0,
    A_INVALID_DEL_TARGET_0,
    A_INVALID_IMPORT_FROM_TARGETS_0,
    A__GATHER_3_0,
    A__GATHER_3_1,
    A__GATHER_4_0,
    A__GATHER_4_1,
    A__GATHER_5_0,
    A__GATHER_5_1,
    A__GATHER_6_0,
    A__GATHER_6_1,
    A__GATHER_8_0,
    A__GATHER_8_1,
    A__TMP_9_0,
    A__TMP_9_1,
    A__TMP_10_0,
    A__TMP_10_1,
    A__TMP_10_2,
    A__TMP_11_0,
    A__TMP_11_1,
    A__TMP_12_0,
    A__TMP_12_1,
    A__TMP_13_0,
    A__TMP_13_1,
    A__TMP_14_0,
    A__TMP_15_0,
    A__TMP_15_1,
    A__TMP_16_0,
    A__TMP_18_0,
    A__TMP_18_1,
    A__TMP_19_0,
    A__TMP_19_1,
    A__GATHER_20_0,
    A__GATHER_20_1,
    A__GATHER_21_0,
    A__GATHER_21_1,
    A__TMP_22_0,
    A__GATHER_25_0,
    A__GATHER_25_1,
    A__TMP_26_0,
    A__GATHER_27_0,
    A__GATHER_27_1,
    A__TMP_28_0,
    A__GATHER_29_0,
    A__GATHER_29_1,
    A__GATHER_30_0,
    A__GATHER_30_1,
    A__GATHER_31_0,
    A__GATHER_31_1,
    A__GATHER_32_0,
    A__GATHER_32_1,
    A__TMP_33_0,
    A__TMP_35_0,
    A__TMP_36_0,
    A__TMP_37_0,
    A__TMP_38_0,
    A__TMP_39_0,
    A__TMP_55_0,
    A__GATHER_56_0,
    A__GATHER_56_1,
    A__GATHER_58_0,
    A__GATHER_58_1,
    A__TMP_77_0,
    A__GATHER_78_0,
    A__GATHER_78_1,
    A__TMP_79_0,
    A__TMP_80_0,
    A__TMP_80_1,
    A__TMP_80_2,
    A__TMP_81_0,
    A__TMP_81_1,
    A__TMP_82_0,
    A__TMP_82_1,
    A__TMP_82_2,
    A__TMP_82_3,
    A__TMP_84_0,
    A__TMP_85_0,
    A__TMP_85_1,
    A__GATHER_86_0,
    A__GATHER_86_1,
    A__TMP_90_0,
    A__TMP_91_0,
    A__GATHER_92_0,
    A__GATHER_92_1,
    A__GATHER_93_0,
    A__GATHER_93_1,
    A__GATHER_94_0,
    A__GATHER_94_1,
    A__GATHER_95_0,
    A__GATHER_95_1,
    A__GATHER_97_0,
    A__GATHER_97_1,
    A__TMP_98_0,
    A__GATHER_99_0,
    A__GATHER_99_1,
    A__GATHER_100_0,
    A__GATHER_100_1,
    A__TMP_101_0,
    A__TMP_101_1,
    A__TMP_103_0,
    A__TMP_104_0,
    A__TMP_104_1,
    A__TMP_105_0,
    A__TMP_105_1,
    A__TMP_106_0,
    A__TMP_106_1,
    A__TMP_106_2,
    A__TMP_108_0,
    A__TMP_108_1,
    A__TMP_109_0,
    A__TMP_109_1,
    A__TMP_110_0,
    A__TMP_110_1,
    A__TMP_111_0,
    A__TMP_112_0,
    A__TMP_112_1,
    A__TMP_113_0,
    A__TMP_113_1,
    A__TMP_114_0,
    A__TMP_115_0,
    A__TMP_116_0,
    A__TMP_117_0,
    A__TMP_118_0,
    A__TMP_119_0,
    A__TMP_120_0,
    A__TMP_121_0,
    A__TMP_123_0,
    A__TMP_123_1,
    A__TMP_124_0,
    A__TMP_124_1,
};

static Rule all_rules[] = {
    {"file",
     R_FILE,
     {0, -1},
     {
        // statements? $
        OP_RULE, R_STATEMENTS,
        OP_OPTIONAL, 
        OP_TOKEN, ENDMARKER,
        OP_RETURN, A_FILE_0,

     },
    },
    {"interactive",
     R_INTERACTIVE,
     {0, -1},
     {
        // statement_newline
        OP_RULE, R_STATEMENT_NEWLINE,
        OP_RETURN, A_INTERACTIVE_0,

     },
    },
    {"eval",
     R_EVAL,
     {0, -1},
     {
        // expressions NEWLINE* $
        OP_RULE, R_EXPRESSIONS,
        OP_RULE, R__LOOP0_1,
        OP_TOKEN, ENDMARKER,
        OP_RETURN, A_EVAL_0,

     },
    },
    {"func_type",
     R_FUNC_TYPE,
     {0, -1},
     {
        // '(' type_expressions? ')' '->' expression NEWLINE* $
        OP_TOKEN, LPAR,
        OP_RULE, R_TYPE_EXPRESSIONS,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_TOKEN, RARROW,
        OP_RULE, R_EXPRESSION,
        OP_RULE, R__LOOP0_2,
        OP_TOKEN, ENDMARKER,
        OP_RETURN, A_FUNC_TYPE_0,

     },
    },
    {"fstring",
     R_FSTRING,
     {0, -1},
     {
        // star_expressions
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_RETURN, A_FSTRING_0,

     },
    },
    {"type_expressions",
     R_TYPE_EXPRESSIONS,
     {0, 16, 26, 36, 48, 54, 60, -1},
     {
        // ','.expression+ ',' '*' expression ',' '**' expression
        OP_RULE, R__GATHER_3,
        OP_TOKEN, COMMA,
        OP_TOKEN, STAR,
        OP_RULE, R_EXPRESSION,
        OP_TOKEN, COMMA,
        OP_TOKEN, DOUBLESTAR,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_TYPE_EXPRESSIONS_0,

        // ','.expression+ ',' '*' expression
        OP_RULE, R__GATHER_4,
        OP_TOKEN, COMMA,
        OP_TOKEN, STAR,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_TYPE_EXPRESSIONS_1,

        // ','.expression+ ',' '**' expression
        OP_RULE, R__GATHER_5,
        OP_TOKEN, COMMA,
        OP_TOKEN, DOUBLESTAR,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_TYPE_EXPRESSIONS_2,

        // '*' expression ',' '**' expression
        OP_TOKEN, STAR,
        OP_RULE, R_EXPRESSION,
        OP_TOKEN, COMMA,
        OP_TOKEN, DOUBLESTAR,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_TYPE_EXPRESSIONS_3,

        // '*' expression
        OP_TOKEN, STAR,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_TYPE_EXPRESSIONS_4,

        // '**' expression
        OP_TOKEN, DOUBLESTAR,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_TYPE_EXPRESSIONS_5,

        // ','.expression+
        OP_RULE, R__GATHER_6,
        OP_RETURN, A_TYPE_EXPRESSIONS_6,

     },
    },
    {"statements",
     R_STATEMENTS,
     {0, -1},
     {
        // statement+
        OP_RULE, R__LOOP1_7,
        OP_RETURN, A_STATEMENTS_0,

     },
    },
    {"statement",
     R_STATEMENT,
     {0, 4, -1},
     {
        // compound_stmt
        OP_RULE, R_COMPOUND_STMT,
        OP_RETURN, A_STATEMENT_0,

        // simple_stmt
        OP_RULE, R_SIMPLE_STMT,
        OP_RETURN, A_STATEMENT_1,

     },
    },
    {"statement_newline",
     R_STATEMENT_NEWLINE,
     {0, 6, 10, 14, -1},
     {
        // compound_stmt NEWLINE
        OP_RULE, R_COMPOUND_STMT,
        OP_TOKEN, NEWLINE,
        OP_RETURN, A_STATEMENT_NEWLINE_0,

        // simple_stmt
        OP_RULE, R_SIMPLE_STMT,
        OP_RETURN, A_STATEMENT_NEWLINE_1,

        // NEWLINE
        OP_TOKEN, NEWLINE,
        OP_RETURN, A_STATEMENT_NEWLINE_2,

        // $
        OP_TOKEN, ENDMARKER,
        OP_RETURN, A_STATEMENT_NEWLINE_3,

     },
    },
    {"simple_stmt",
     R_SIMPLE_STMT,
     {0, 10, -1},
     {
        // small_stmt !';' NEWLINE
        OP_RULE, R_SMALL_STMT,
        OP_SAVE_MARK, 
        OP_TOKEN, SEMI,
        OP_NEG_LOOKAHEAD, 
        OP_TOKEN, NEWLINE,
        OP_RETURN, A_SIMPLE_STMT_0,

        // ';'.small_stmt+ ';'? NEWLINE
        OP_RULE, R__GATHER_8,
        OP_TOKEN, SEMI,
        OP_OPTIONAL, 
        OP_TOKEN, NEWLINE,
        OP_RETURN, A_SIMPLE_STMT_1,

     },
    },
    {"small_stmt",
     R_SMALL_STMT,
     {0, 4, 8, 16, 24, 32, 36, 44, 52, 60, 64, 68, 76, -1},
     {
        // assignment
        OP_RULE, R_ASSIGNMENT,
        OP_RETURN, A_SMALL_STMT_0,

        // star_expressions
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_RETURN, A_SMALL_STMT_1,

        // &'return' return_stmt
        OP_SAVE_MARK, 
        OP_TOKEN, 500,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_RETURN_STMT,
        OP_RETURN, A_SMALL_STMT_2,

        // &('import' | 'from') import_stmt
        OP_SAVE_MARK, 
        OP_RULE, R__TMP_9,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_IMPORT_STMT,
        OP_RETURN, A_SMALL_STMT_3,

        // &'raise' raise_stmt
        OP_SAVE_MARK, 
        OP_TOKEN, 501,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_RAISE_STMT,
        OP_RETURN, A_SMALL_STMT_4,

        // 'pass'
        OP_TOKEN, 502,
        OP_RETURN, A_SMALL_STMT_5,

        // &'del' del_stmt
        OP_SAVE_MARK, 
        OP_TOKEN, 503,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_DEL_STMT,
        OP_RETURN, A_SMALL_STMT_6,

        // &'yield' yield_stmt
        OP_SAVE_MARK, 
        OP_TOKEN, 504,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_YIELD_STMT,
        OP_RETURN, A_SMALL_STMT_7,

        // &'assert' assert_stmt
        OP_SAVE_MARK, 
        OP_TOKEN, 505,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_ASSERT_STMT,
        OP_RETURN, A_SMALL_STMT_8,

        // 'break'
        OP_TOKEN, 506,
        OP_RETURN, A_SMALL_STMT_9,

        // 'continue'
        OP_TOKEN, 507,
        OP_RETURN, A_SMALL_STMT_10,

        // &'global' global_stmt
        OP_SAVE_MARK, 
        OP_TOKEN, 508,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_GLOBAL_STMT,
        OP_RETURN, A_SMALL_STMT_11,

        // &'nonlocal' nonlocal_stmt
        OP_SAVE_MARK, 
        OP_TOKEN, 509,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_NONLOCAL_STMT,
        OP_RETURN, A_SMALL_STMT_12,

     },
    },
    {"compound_stmt",
     R_COMPOUND_STMT,
     {0, 8, 16, 24, 32, 40, 48, -1},
     {
        // &('def' | '@' | ASYNC) function_def
        OP_SAVE_MARK, 
        OP_RULE, R__TMP_10,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_FUNCTION_DEF,
        OP_RETURN, A_COMPOUND_STMT_0,

        // &'if' if_stmt
        OP_SAVE_MARK, 
        OP_TOKEN, 510,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_IF_STMT,
        OP_RETURN, A_COMPOUND_STMT_1,

        // &('class' | '@') class_def
        OP_SAVE_MARK, 
        OP_RULE, R__TMP_11,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_CLASS_DEF,
        OP_RETURN, A_COMPOUND_STMT_2,

        // &('with' | ASYNC) with_stmt
        OP_SAVE_MARK, 
        OP_RULE, R__TMP_12,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_WITH_STMT,
        OP_RETURN, A_COMPOUND_STMT_3,

        // &('for' | ASYNC) for_stmt
        OP_SAVE_MARK, 
        OP_RULE, R__TMP_13,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_FOR_STMT,
        OP_RETURN, A_COMPOUND_STMT_4,

        // &'try' try_stmt
        OP_SAVE_MARK, 
        OP_TOKEN, 511,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_TRY_STMT,
        OP_RETURN, A_COMPOUND_STMT_5,

        // &'while' while_stmt
        OP_SAVE_MARK, 
        OP_TOKEN, 512,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_WHILE_STMT,
        OP_RETURN, A_COMPOUND_STMT_6,

     },
    },
    {"assignment",
     R_ASSIGNMENT,
     {0, 10, 21, 30, 38, -1},
     {
        // NAME ':' expression ['=' annotated_rhs]
        OP_NAME, 
        OP_TOKEN, COLON,
        OP_RULE, R_EXPRESSION,
        OP_RULE, R__TMP_14,
        OP_OPTIONAL, 
        OP_RETURN, A_ASSIGNMENT_0,

        // ('(' single_target ')' | single_subscript_attribute_target) ':' expression ['=' annotated_rhs]
        OP_RULE, R__TMP_15,
        OP_TOKEN, COLON,
        OP_RULE, R_EXPRESSION,
        OP_RULE, R__TMP_16,
        OP_OPTIONAL, 
        OP_RETURN, A_ASSIGNMENT_1,

        // ((star_targets '='))+ (yield_expr | star_expressions) TYPE_COMMENT?
        OP_RULE, R__LOOP1_17,
        OP_RULE, R__TMP_18,
        OP_TOKEN, TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_RETURN, A_ASSIGNMENT_2,

        // single_target augassign (yield_expr | star_expressions)
        OP_RULE, R_SINGLE_TARGET,
        OP_RULE, R_AUGASSIGN,
        OP_RULE, R__TMP_19,
        OP_RETURN, A_ASSIGNMENT_3,

        // invalid_assignment
        OP_RULE, R_INVALID_ASSIGNMENT,
        OP_RETURN, A_ASSIGNMENT_4,

     },
    },
    {"augassign",
     R_AUGASSIGN,
     {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, -1},
     {
        // '+='
        OP_TOKEN, PLUSEQUAL,
        OP_RETURN, A_AUGASSIGN_0,

        // '-='
        OP_TOKEN, MINEQUAL,
        OP_RETURN, A_AUGASSIGN_1,

        // '*='
        OP_TOKEN, STAREQUAL,
        OP_RETURN, A_AUGASSIGN_2,

        // '@='
        OP_TOKEN, ATEQUAL,
        OP_RETURN, A_AUGASSIGN_3,

        // '/='
        OP_TOKEN, SLASHEQUAL,
        OP_RETURN, A_AUGASSIGN_4,

        // '%='
        OP_TOKEN, PERCENTEQUAL,
        OP_RETURN, A_AUGASSIGN_5,

        // '&='
        OP_TOKEN, AMPEREQUAL,
        OP_RETURN, A_AUGASSIGN_6,

        // '|='
        OP_TOKEN, VBAREQUAL,
        OP_RETURN, A_AUGASSIGN_7,

        // '^='
        OP_TOKEN, CIRCUMFLEXEQUAL,
        OP_RETURN, A_AUGASSIGN_8,

        // '<<='
        OP_TOKEN, LEFTSHIFTEQUAL,
        OP_RETURN, A_AUGASSIGN_9,

        // '>>='
        OP_TOKEN, RIGHTSHIFTEQUAL,
        OP_RETURN, A_AUGASSIGN_10,

        // '**='
        OP_TOKEN, DOUBLESTAREQUAL,
        OP_RETURN, A_AUGASSIGN_11,

        // '//='
        OP_TOKEN, DOUBLESLASHEQUAL,
        OP_RETURN, A_AUGASSIGN_12,

     },
    },
    {"global_stmt",
     R_GLOBAL_STMT,
     {0, -1},
     {
        // 'global' ','.NAME+
        OP_TOKEN, 508,
        OP_RULE, R__GATHER_20,
        OP_RETURN, A_GLOBAL_STMT_0,

     },
    },
    {"nonlocal_stmt",
     R_NONLOCAL_STMT,
     {0, -1},
     {
        // 'nonlocal' ','.NAME+
        OP_TOKEN, 509,
        OP_RULE, R__GATHER_21,
        OP_RETURN, A_NONLOCAL_STMT_0,

     },
    },
    {"yield_stmt",
     R_YIELD_STMT,
     {0, -1},
     {
        // yield_expr
        OP_RULE, R_YIELD_EXPR,
        OP_RETURN, A_YIELD_STMT_0,

     },
    },
    {"assert_stmt",
     R_ASSERT_STMT,
     {0, -1},
     {
        // 'assert' expression [',' expression]
        OP_TOKEN, 505,
        OP_RULE, R_EXPRESSION,
        OP_RULE, R__TMP_22,
        OP_OPTIONAL, 
        OP_RETURN, A_ASSERT_STMT_0,

     },
    },
    {"del_stmt",
     R_DEL_STMT,
     {0, -1},
     {
        // 'del' del_targets
        OP_TOKEN, 503,
        OP_RULE, R_DEL_TARGETS,
        OP_RETURN, A_DEL_STMT_0,

     },
    },
    {"import_stmt",
     R_IMPORT_STMT,
     {0, 4, -1},
     {
        // import_name
        OP_RULE, R_IMPORT_NAME,
        OP_RETURN, A_IMPORT_STMT_0,

        // import_from
        OP_RULE, R_IMPORT_FROM,
        OP_RETURN, A_IMPORT_STMT_1,

     },
    },
    {"import_name",
     R_IMPORT_NAME,
     {0, -1},
     {
        // 'import' dotted_as_names
        OP_TOKEN, 513,
        OP_RULE, R_DOTTED_AS_NAMES,
        OP_RETURN, A_IMPORT_NAME_0,

     },
    },
    {"import_from",
     R_IMPORT_FROM,
     {0, 12, -1},
     {
        // 'from' (('.' | '...'))* dotted_name 'import' import_from_targets
        OP_TOKEN, 514,
        OP_RULE, R__LOOP0_23,
        OP_RULE, R_DOTTED_NAME,
        OP_TOKEN, 513,
        OP_RULE, R_IMPORT_FROM_TARGETS,
        OP_RETURN, A_IMPORT_FROM_0,

        // 'from' (('.' | '...'))+ 'import' import_from_targets
        OP_TOKEN, 514,
        OP_RULE, R__LOOP1_24,
        OP_TOKEN, 513,
        OP_RULE, R_IMPORT_FROM_TARGETS,
        OP_RETURN, A_IMPORT_FROM_1,

     },
    },
    {"import_from_targets",
     R_IMPORT_FROM_TARGETS,
     {0, 11, 19, 23, -1},
     {
        // '(' import_from_as_names ','? ')'
        OP_TOKEN, LPAR,
        OP_RULE, R_IMPORT_FROM_AS_NAMES,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_RETURN, A_IMPORT_FROM_TARGETS_0,

        // import_from_as_names !','
        OP_RULE, R_IMPORT_FROM_AS_NAMES,
        OP_SAVE_MARK, 
        OP_TOKEN, COMMA,
        OP_NEG_LOOKAHEAD, 
        OP_RETURN, A_IMPORT_FROM_TARGETS_1,

        // '*'
        OP_TOKEN, STAR,
        OP_RETURN, A_IMPORT_FROM_TARGETS_2,

        // invalid_import_from_targets
        OP_RULE, R_INVALID_IMPORT_FROM_TARGETS,
        OP_RETURN, A_IMPORT_FROM_TARGETS_3,

     },
    },
    {"import_from_as_names",
     R_IMPORT_FROM_AS_NAMES,
     {0, -1},
     {
        // ','.import_from_as_name+
        OP_RULE, R__GATHER_25,
        OP_RETURN, A_IMPORT_FROM_AS_NAMES_0,

     },
    },
    {"import_from_as_name",
     R_IMPORT_FROM_AS_NAME,
     {0, -1},
     {
        // NAME ['as' NAME]
        OP_NAME, 
        OP_RULE, R__TMP_26,
        OP_OPTIONAL, 
        OP_RETURN, A_IMPORT_FROM_AS_NAME_0,

     },
    },
    {"dotted_as_names",
     R_DOTTED_AS_NAMES,
     {0, -1},
     {
        // ','.dotted_as_name+
        OP_RULE, R__GATHER_27,
        OP_RETURN, A_DOTTED_AS_NAMES_0,

     },
    },
    {"dotted_as_name",
     R_DOTTED_AS_NAME,
     {0, -1},
     {
        // dotted_name ['as' NAME]
        OP_RULE, R_DOTTED_NAME,
        OP_RULE, R__TMP_28,
        OP_OPTIONAL, 
        OP_RETURN, A_DOTTED_AS_NAME_0,

     },
    },
    {"dotted_name",
     R_DOTTED_NAME,
     {0, 8, -1},
     {
        // dotted_name '.' NAME
        OP_SETUP_LEFT_REC, 
        OP_RULE, R_DOTTED_NAME,
        OP_TOKEN, DOT,
        OP_NAME, 
        OP_RETURN_LEFT_REC, A_DOTTED_NAME_0,

        // NAME
        OP_NAME, 
        OP_RETURN_LEFT_REC, A_DOTTED_NAME_1,

     },
    },
    {"if_stmt",
     R_IF_STMT,
     {0, 12, -1},
     {
        // 'if' named_expression ':' block elif_stmt
        OP_TOKEN, 510,
        OP_RULE, R_NAMED_EXPRESSION,
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RULE, R_ELIF_STMT,
        OP_RETURN, A_IF_STMT_0,

        // 'if' named_expression ':' block else_block?
        OP_TOKEN, 510,
        OP_RULE, R_NAMED_EXPRESSION,
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RULE, R_ELSE_BLOCK,
        OP_OPTIONAL, 
        OP_RETURN, A_IF_STMT_1,

     },
    },
    {"elif_stmt",
     R_ELIF_STMT,
     {0, 12, -1},
     {
        // 'elif' named_expression ':' block elif_stmt
        OP_TOKEN, 515,
        OP_RULE, R_NAMED_EXPRESSION,
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RULE, R_ELIF_STMT,
        OP_RETURN, A_ELIF_STMT_0,

        // 'elif' named_expression ':' block else_block?
        OP_TOKEN, 515,
        OP_RULE, R_NAMED_EXPRESSION,
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RULE, R_ELSE_BLOCK,
        OP_OPTIONAL, 
        OP_RETURN, A_ELIF_STMT_1,

     },
    },
    {"else_block",
     R_ELSE_BLOCK,
     {0, -1},
     {
        // 'else' ':' block
        OP_TOKEN, 516,
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RETURN, A_ELSE_BLOCK_0,

     },
    },
    {"while_stmt",
     R_WHILE_STMT,
     {0, -1},
     {
        // 'while' named_expression ':' block else_block?
        OP_TOKEN, 512,
        OP_RULE, R_NAMED_EXPRESSION,
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RULE, R_ELSE_BLOCK,
        OP_OPTIONAL, 
        OP_RETURN, A_WHILE_STMT_0,

     },
    },
    {"for_stmt",
     R_FOR_STMT,
     {0, 20, -1},
     {
        // 'for' star_targets 'in' star_expressions ':' TYPE_COMMENT? block else_block?
        OP_TOKEN, 517,
        OP_RULE, R_STAR_TARGETS,
        OP_TOKEN, 518,
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_TOKEN, COLON,
        OP_TOKEN, TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_RULE, R_BLOCK,
        OP_RULE, R_ELSE_BLOCK,
        OP_OPTIONAL, 
        OP_RETURN, A_FOR_STMT_0,

        // ASYNC 'for' star_targets 'in' star_expressions ':' TYPE_COMMENT? block else_block?
        OP_TOKEN, ASYNC,
        OP_TOKEN, 517,
        OP_RULE, R_STAR_TARGETS,
        OP_TOKEN, 518,
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_TOKEN, COLON,
        OP_TOKEN, TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_RULE, R_BLOCK,
        OP_RULE, R_ELSE_BLOCK,
        OP_OPTIONAL, 
        OP_RETURN, A_FOR_STMT_1,

     },
    },
    {"with_stmt",
     R_WITH_STMT,
     {0, 17, 30, 49, -1},
     {
        // 'with' '(' ','.with_item+ ','? ')' ':' block
        OP_TOKEN, 519,
        OP_TOKEN, LPAR,
        OP_RULE, R__GATHER_29,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RETURN, A_WITH_STMT_0,

        // 'with' ','.with_item+ ':' TYPE_COMMENT? block
        OP_TOKEN, 519,
        OP_RULE, R__GATHER_30,
        OP_TOKEN, COLON,
        OP_TOKEN, TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_RULE, R_BLOCK,
        OP_RETURN, A_WITH_STMT_1,

        // ASYNC 'with' '(' ','.with_item+ ','? ')' ':' block
        OP_TOKEN, ASYNC,
        OP_TOKEN, 519,
        OP_TOKEN, LPAR,
        OP_RULE, R__GATHER_31,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RETURN, A_WITH_STMT_2,

        // ASYNC 'with' ','.with_item+ ':' TYPE_COMMENT? block
        OP_TOKEN, ASYNC,
        OP_TOKEN, 519,
        OP_RULE, R__GATHER_32,
        OP_TOKEN, COLON,
        OP_TOKEN, TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_RULE, R_BLOCK,
        OP_RETURN, A_WITH_STMT_3,

     },
    },
    {"with_item",
     R_WITH_ITEM,
     {0, -1},
     {
        // expression ['as' target]
        OP_RULE, R_EXPRESSION,
        OP_RULE, R__TMP_33,
        OP_OPTIONAL, 
        OP_RETURN, A_WITH_ITEM_0,

     },
    },
    {"try_stmt",
     R_TRY_STMT,
     {0, 10, -1},
     {
        // 'try' ':' block finally_block
        OP_TOKEN, 511,
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RULE, R_FINALLY_BLOCK,
        OP_RETURN, A_TRY_STMT_0,

        // 'try' ':' block except_block+ else_block? finally_block?
        OP_TOKEN, 511,
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RULE, R__LOOP1_34,
        OP_RULE, R_ELSE_BLOCK,
        OP_OPTIONAL, 
        OP_RULE, R_FINALLY_BLOCK,
        OP_OPTIONAL, 
        OP_RETURN, A_TRY_STMT_1,

     },
    },
    {"except_block",
     R_EXCEPT_BLOCK,
     {0, 13, -1},
     {
        // 'except' expression ['as' NAME] ':' block
        OP_TOKEN, 520,
        OP_RULE, R_EXPRESSION,
        OP_RULE, R__TMP_35,
        OP_OPTIONAL, 
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RETURN, A_EXCEPT_BLOCK_0,

        // 'except' ':' block
        OP_TOKEN, 520,
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RETURN, A_EXCEPT_BLOCK_1,

     },
    },
    {"finally_block",
     R_FINALLY_BLOCK,
     {0, -1},
     {
        // 'finally' ':' block
        OP_TOKEN, 521,
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RETURN, A_FINALLY_BLOCK_0,

     },
    },
    {"return_stmt",
     R_RETURN_STMT,
     {0, -1},
     {
        // 'return' star_expressions?
        OP_TOKEN, 500,
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_OPTIONAL, 
        OP_RETURN, A_RETURN_STMT_0,

     },
    },
    {"raise_stmt",
     R_RAISE_STMT,
     {0, 9, -1},
     {
        // 'raise' expression ['from' expression]
        OP_TOKEN, 501,
        OP_RULE, R_EXPRESSION,
        OP_RULE, R__TMP_36,
        OP_OPTIONAL, 
        OP_RETURN, A_RAISE_STMT_0,

        // 'raise'
        OP_TOKEN, 501,
        OP_RETURN, A_RAISE_STMT_1,

     },
    },
    {"function_def",
     R_FUNCTION_DEF,
     {0, 6, -1},
     {
        // decorators function_def_raw
        OP_RULE, R_DECORATORS,
        OP_RULE, R_FUNCTION_DEF_RAW,
        OP_RETURN, A_FUNCTION_DEF_0,

        // function_def_raw
        OP_RULE, R_FUNCTION_DEF_RAW,
        OP_RETURN, A_FUNCTION_DEF_1,

     },
    },
    {"function_def_raw",
     R_FUNCTION_DEF_RAW,
     {0, 22, -1},
     {
        // 'def' NAME '(' params? ')' ['->' expression] ':' func_type_comment? block
        OP_TOKEN, 522,
        OP_NAME, 
        OP_TOKEN, LPAR,
        OP_RULE, R_PARAMS,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_RULE, R__TMP_37,
        OP_OPTIONAL, 
        OP_TOKEN, COLON,
        OP_RULE, R_FUNC_TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_RULE, R_BLOCK,
        OP_RETURN, A_FUNCTION_DEF_RAW_0,

        // ASYNC 'def' NAME '(' params? ')' ['->' expression] ':' func_type_comment? block
        OP_TOKEN, ASYNC,
        OP_TOKEN, 522,
        OP_NAME, 
        OP_TOKEN, LPAR,
        OP_RULE, R_PARAMS,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_RULE, R__TMP_38,
        OP_OPTIONAL, 
        OP_TOKEN, COLON,
        OP_RULE, R_FUNC_TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_RULE, R_BLOCK,
        OP_RETURN, A_FUNCTION_DEF_RAW_1,

     },
    },
    {"func_type_comment",
     R_FUNC_TYPE_COMMENT,
     {0, 10, 14, -1},
     {
        // NEWLINE TYPE_COMMENT &(NEWLINE INDENT)
        OP_TOKEN, NEWLINE,
        OP_TOKEN, TYPE_COMMENT,
        OP_SAVE_MARK, 
        OP_RULE, R__TMP_39,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_FUNC_TYPE_COMMENT_0,

        // invalid_double_type_comments
        OP_RULE, R_INVALID_DOUBLE_TYPE_COMMENTS,
        OP_RETURN, A_FUNC_TYPE_COMMENT_1,

        // TYPE_COMMENT
        OP_TOKEN, TYPE_COMMENT,
        OP_RETURN, A_FUNC_TYPE_COMMENT_2,

     },
    },
    {"params",
     R_PARAMS,
     {0, 4, -1},
     {
        // invalid_parameters
        OP_RULE, R_INVALID_PARAMETERS,
        OP_RETURN, A_PARAMS_0,

        // parameters
        OP_RULE, R_PARAMETERS,
        OP_RETURN, A_PARAMS_1,

     },
    },
    {"parameters",
     R_PARAMETERS,
     {0, 11, 20, 29, 36, -1},
     {
        // slash_no_default param_no_default* param_with_default* star_etc?
        OP_RULE, R_SLASH_NO_DEFAULT,
        OP_RULE, R__LOOP0_40,
        OP_RULE, R__LOOP0_41,
        OP_RULE, R_STAR_ETC,
        OP_OPTIONAL, 
        OP_RETURN, A_PARAMETERS_0,

        // slash_with_default param_with_default* star_etc?
        OP_RULE, R_SLASH_WITH_DEFAULT,
        OP_RULE, R__LOOP0_42,
        OP_RULE, R_STAR_ETC,
        OP_OPTIONAL, 
        OP_RETURN, A_PARAMETERS_1,

        // param_no_default+ param_with_default* star_etc?
        OP_RULE, R__LOOP1_43,
        OP_RULE, R__LOOP0_44,
        OP_RULE, R_STAR_ETC,
        OP_OPTIONAL, 
        OP_RETURN, A_PARAMETERS_2,

        // param_with_default+ star_etc?
        OP_RULE, R__LOOP1_45,
        OP_RULE, R_STAR_ETC,
        OP_OPTIONAL, 
        OP_RETURN, A_PARAMETERS_3,

        // star_etc
        OP_RULE, R_STAR_ETC,
        OP_RETURN, A_PARAMETERS_4,

     },
    },
    {"slash_no_default",
     R_SLASH_NO_DEFAULT,
     {0, 8, -1},
     {
        // param_no_default+ '/' ','
        OP_RULE, R__LOOP1_46,
        OP_TOKEN, SLASH,
        OP_TOKEN, COMMA,
        OP_RETURN, A_SLASH_NO_DEFAULT_0,

        // param_no_default+ '/' &')'
        OP_RULE, R__LOOP1_47,
        OP_TOKEN, SLASH,
        OP_SAVE_MARK, 
        OP_TOKEN, RPAR,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_SLASH_NO_DEFAULT_1,

     },
    },
    {"slash_with_default",
     R_SLASH_WITH_DEFAULT,
     {0, 10, -1},
     {
        // param_no_default* param_with_default+ '/' ','
        OP_RULE, R__LOOP0_48,
        OP_RULE, R__LOOP1_49,
        OP_TOKEN, SLASH,
        OP_TOKEN, COMMA,
        OP_RETURN, A_SLASH_WITH_DEFAULT_0,

        // param_no_default* param_with_default+ '/' &')'
        OP_RULE, R__LOOP0_50,
        OP_RULE, R__LOOP1_51,
        OP_TOKEN, SLASH,
        OP_SAVE_MARK, 
        OP_TOKEN, RPAR,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_SLASH_WITH_DEFAULT_1,

     },
    },
    {"star_etc",
     R_STAR_ETC,
     {0, 11, 22, 26, -1},
     {
        // '*' param_no_default param_maybe_default* kwds?
        OP_TOKEN, STAR,
        OP_RULE, R_PARAM_NO_DEFAULT,
        OP_RULE, R__LOOP0_52,
        OP_RULE, R_KWDS,
        OP_OPTIONAL, 
        OP_RETURN, A_STAR_ETC_0,

        // '*' ',' param_maybe_default+ kwds?
        OP_TOKEN, STAR,
        OP_TOKEN, COMMA,
        OP_RULE, R__LOOP1_53,
        OP_RULE, R_KWDS,
        OP_OPTIONAL, 
        OP_RETURN, A_STAR_ETC_1,

        // kwds
        OP_RULE, R_KWDS,
        OP_RETURN, A_STAR_ETC_2,

        // invalid_star_etc
        OP_RULE, R_INVALID_STAR_ETC,
        OP_RETURN, A_STAR_ETC_3,

     },
    },
    {"kwds",
     R_KWDS,
     {0, -1},
     {
        // '**' param_no_default
        OP_TOKEN, DOUBLESTAR,
        OP_RULE, R_PARAM_NO_DEFAULT,
        OP_RETURN, A_KWDS_0,

     },
    },
    {"param_no_default",
     R_PARAM_NO_DEFAULT,
     {0, 9, -1},
     {
        // param ',' TYPE_COMMENT?
        OP_RULE, R_PARAM,
        OP_TOKEN, COMMA,
        OP_TOKEN, TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_RETURN, A_PARAM_NO_DEFAULT_0,

        // param TYPE_COMMENT? &')'
        OP_RULE, R_PARAM,
        OP_TOKEN, TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_SAVE_MARK, 
        OP_TOKEN, RPAR,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_PARAM_NO_DEFAULT_1,

     },
    },
    {"param_with_default",
     R_PARAM_WITH_DEFAULT,
     {0, 11, -1},
     {
        // param default ',' TYPE_COMMENT?
        OP_RULE, R_PARAM,
        OP_RULE, R_DEFAULT,
        OP_TOKEN, COMMA,
        OP_TOKEN, TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_RETURN, A_PARAM_WITH_DEFAULT_0,

        // param default TYPE_COMMENT? &')'
        OP_RULE, R_PARAM,
        OP_RULE, R_DEFAULT,
        OP_TOKEN, TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_SAVE_MARK, 
        OP_TOKEN, RPAR,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_PARAM_WITH_DEFAULT_1,

     },
    },
    {"param_maybe_default",
     R_PARAM_MAYBE_DEFAULT,
     {0, 12, -1},
     {
        // param default? ',' TYPE_COMMENT?
        OP_RULE, R_PARAM,
        OP_RULE, R_DEFAULT,
        OP_OPTIONAL, 
        OP_TOKEN, COMMA,
        OP_TOKEN, TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_RETURN, A_PARAM_MAYBE_DEFAULT_0,

        // param default? TYPE_COMMENT? &')'
        OP_RULE, R_PARAM,
        OP_RULE, R_DEFAULT,
        OP_OPTIONAL, 
        OP_TOKEN, TYPE_COMMENT,
        OP_OPTIONAL, 
        OP_SAVE_MARK, 
        OP_TOKEN, RPAR,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_PARAM_MAYBE_DEFAULT_1,

     },
    },
    {"param",
     R_PARAM,
     {0, -1},
     {
        // NAME annotation?
        OP_NAME, 
        OP_RULE, R_ANNOTATION,
        OP_OPTIONAL, 
        OP_RETURN, A_PARAM_0,

     },
    },
    {"annotation",
     R_ANNOTATION,
     {0, -1},
     {
        // ':' expression
        OP_TOKEN, COLON,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_ANNOTATION_0,

     },
    },
    {"default",
     R_DEFAULT,
     {0, -1},
     {
        // '=' expression
        OP_TOKEN, EQUAL,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_DEFAULT_0,

     },
    },
    {"decorators",
     R_DECORATORS,
     {0, -1},
     {
        // (('@' named_expression NEWLINE))+
        OP_RULE, R__LOOP1_54,
        OP_RETURN, A_DECORATORS_0,

     },
    },
    {"class_def",
     R_CLASS_DEF,
     {0, 6, -1},
     {
        // decorators class_def_raw
        OP_RULE, R_DECORATORS,
        OP_RULE, R_CLASS_DEF_RAW,
        OP_RETURN, A_CLASS_DEF_0,

        // class_def_raw
        OP_RULE, R_CLASS_DEF_RAW,
        OP_RETURN, A_CLASS_DEF_1,

     },
    },
    {"class_def_raw",
     R_CLASS_DEF_RAW,
     {0, -1},
     {
        // 'class' NAME ['(' arguments? ')'] ':' block
        OP_TOKEN, 523,
        OP_NAME, 
        OP_RULE, R__TMP_55,
        OP_OPTIONAL, 
        OP_TOKEN, COLON,
        OP_RULE, R_BLOCK,
        OP_RETURN, A_CLASS_DEF_RAW_0,

     },
    },
    {"block",
     R_BLOCK,
     {0, 10, 14, -1},
     {
        // NEWLINE INDENT statements DEDENT
        OP_TOKEN, NEWLINE,
        OP_TOKEN, INDENT,
        OP_RULE, R_STATEMENTS,
        OP_TOKEN, DEDENT,
        OP_RETURN, A_BLOCK_0,

        // simple_stmt
        OP_RULE, R_SIMPLE_STMT,
        OP_RETURN, A_BLOCK_1,

        // invalid_block
        OP_RULE, R_INVALID_BLOCK,
        OP_RETURN, A_BLOCK_2,

     },
    },
    {"expressions_list",
     R_EXPRESSIONS_LIST,
     {0, -1},
     {
        // ','.star_expression+ ','?
        OP_RULE, R__GATHER_56,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_RETURN, A_EXPRESSIONS_LIST_0,

     },
    },
    {"star_expressions",
     R_STAR_EXPRESSIONS,
     {0, 9, 15, -1},
     {
        // star_expression ((',' star_expression))+ ','?
        OP_RULE, R_STAR_EXPRESSION,
        OP_RULE, R__LOOP1_57,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_RETURN, A_STAR_EXPRESSIONS_0,

        // star_expression ','
        OP_RULE, R_STAR_EXPRESSION,
        OP_TOKEN, COMMA,
        OP_RETURN, A_STAR_EXPRESSIONS_1,

        // star_expression
        OP_RULE, R_STAR_EXPRESSION,
        OP_RETURN, A_STAR_EXPRESSIONS_2,

     },
    },
    {"star_expression",
     R_STAR_EXPRESSION,
     {0, 6, -1},
     {
        // '*' bitwise_or
        OP_TOKEN, STAR,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_STAR_EXPRESSION_0,

        // expression
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_STAR_EXPRESSION_1,

     },
    },
    {"star_named_expressions",
     R_STAR_NAMED_EXPRESSIONS,
     {0, -1},
     {
        // ','.star_named_expression+ ','?
        OP_RULE, R__GATHER_58,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_RETURN, A_STAR_NAMED_EXPRESSIONS_0,

     },
    },
    {"star_named_expression",
     R_STAR_NAMED_EXPRESSION,
     {0, 6, -1},
     {
        // '*' bitwise_or
        OP_TOKEN, STAR,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_STAR_NAMED_EXPRESSION_0,

        // named_expression
        OP_RULE, R_NAMED_EXPRESSION,
        OP_RETURN, A_STAR_NAMED_EXPRESSION_1,

     },
    },
    {"named_expression",
     R_NAMED_EXPRESSION,
     {0, 7, 15, -1},
     {
        // NAME ':=' expression
        OP_NAME, 
        OP_TOKEN, COLONEQUAL,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_NAMED_EXPRESSION_0,

        // expression !':='
        OP_RULE, R_EXPRESSION,
        OP_SAVE_MARK, 
        OP_TOKEN, COLONEQUAL,
        OP_NEG_LOOKAHEAD, 
        OP_RETURN, A_NAMED_EXPRESSION_1,

        // invalid_named_expression
        OP_RULE, R_INVALID_NAMED_EXPRESSION,
        OP_RETURN, A_NAMED_EXPRESSION_2,

     },
    },
    {"annotated_rhs",
     R_ANNOTATED_RHS,
     {0, 4, -1},
     {
        // yield_expr
        OP_RULE, R_YIELD_EXPR,
        OP_RETURN, A_ANNOTATED_RHS_0,

        // star_expressions
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_RETURN, A_ANNOTATED_RHS_1,

     },
    },
    {"expressions",
     R_EXPRESSIONS,
     {0, 9, 15, -1},
     {
        // expression ((',' expression))+ ','?
        OP_RULE, R_EXPRESSION,
        OP_RULE, R__LOOP1_59,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_RETURN, A_EXPRESSIONS_0,

        // expression ','
        OP_RULE, R_EXPRESSION,
        OP_TOKEN, COMMA,
        OP_RETURN, A_EXPRESSIONS_1,

        // expression
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_EXPRESSIONS_2,

     },
    },
    {"expression",
     R_EXPRESSION,
     {0, 12, 16, -1},
     {
        // disjunction 'if' disjunction 'else' expression
        OP_RULE, R_DISJUNCTION,
        OP_TOKEN, 510,
        OP_RULE, R_DISJUNCTION,
        OP_TOKEN, 516,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_EXPRESSION_0,

        // disjunction
        OP_RULE, R_DISJUNCTION,
        OP_RETURN, A_EXPRESSION_1,

        // lambdef
        OP_RULE, R_LAMBDEF,
        OP_RETURN, A_EXPRESSION_2,

     },
    },
    {"lambdef",
     R_LAMBDEF,
     {0, -1},
     {
        // 'lambda' lambda_parameters? ':' expression
        OP_TOKEN, 524,
        OP_RULE, R_LAMBDA_PARAMETERS,
        OP_OPTIONAL, 
        OP_TOKEN, COLON,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_LAMBDEF_0,

     },
    },
    {"lambda_parameters",
     R_LAMBDA_PARAMETERS,
     {0, 11, 20, 29, 36, -1},
     {
        // lambda_slash_no_default lambda_param_no_default* lambda_param_with_default* lambda_star_etc?
        OP_RULE, R_LAMBDA_SLASH_NO_DEFAULT,
        OP_RULE, R__LOOP0_60,
        OP_RULE, R__LOOP0_61,
        OP_RULE, R_LAMBDA_STAR_ETC,
        OP_OPTIONAL, 
        OP_RETURN, A_LAMBDA_PARAMETERS_0,

        // lambda_slash_with_default lambda_param_with_default* lambda_star_etc?
        OP_RULE, R_LAMBDA_SLASH_WITH_DEFAULT,
        OP_RULE, R__LOOP0_62,
        OP_RULE, R_LAMBDA_STAR_ETC,
        OP_OPTIONAL, 
        OP_RETURN, A_LAMBDA_PARAMETERS_1,

        // lambda_param_no_default+ lambda_param_with_default* lambda_star_etc?
        OP_RULE, R__LOOP1_63,
        OP_RULE, R__LOOP0_64,
        OP_RULE, R_LAMBDA_STAR_ETC,
        OP_OPTIONAL, 
        OP_RETURN, A_LAMBDA_PARAMETERS_2,

        // lambda_param_with_default+ lambda_star_etc?
        OP_RULE, R__LOOP1_65,
        OP_RULE, R_LAMBDA_STAR_ETC,
        OP_OPTIONAL, 
        OP_RETURN, A_LAMBDA_PARAMETERS_3,

        // lambda_star_etc
        OP_RULE, R_LAMBDA_STAR_ETC,
        OP_RETURN, A_LAMBDA_PARAMETERS_4,

     },
    },
    {"lambda_slash_no_default",
     R_LAMBDA_SLASH_NO_DEFAULT,
     {0, 8, -1},
     {
        // lambda_param_no_default+ '/' ','
        OP_RULE, R__LOOP1_66,
        OP_TOKEN, SLASH,
        OP_TOKEN, COMMA,
        OP_RETURN, A_LAMBDA_SLASH_NO_DEFAULT_0,

        // lambda_param_no_default+ '/' &':'
        OP_RULE, R__LOOP1_67,
        OP_TOKEN, SLASH,
        OP_SAVE_MARK, 
        OP_TOKEN, COLON,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_LAMBDA_SLASH_NO_DEFAULT_1,

     },
    },
    {"lambda_slash_with_default",
     R_LAMBDA_SLASH_WITH_DEFAULT,
     {0, 10, -1},
     {
        // lambda_param_no_default* lambda_param_with_default+ '/' ','
        OP_RULE, R__LOOP0_68,
        OP_RULE, R__LOOP1_69,
        OP_TOKEN, SLASH,
        OP_TOKEN, COMMA,
        OP_RETURN, A_LAMBDA_SLASH_WITH_DEFAULT_0,

        // lambda_param_no_default* lambda_param_with_default+ '/' &':'
        OP_RULE, R__LOOP0_70,
        OP_RULE, R__LOOP1_71,
        OP_TOKEN, SLASH,
        OP_SAVE_MARK, 
        OP_TOKEN, COLON,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_LAMBDA_SLASH_WITH_DEFAULT_1,

     },
    },
    {"lambda_star_etc",
     R_LAMBDA_STAR_ETC,
     {0, 11, 22, 26, -1},
     {
        // '*' lambda_param_no_default lambda_param_maybe_default* lambda_kwds?
        OP_TOKEN, STAR,
        OP_RULE, R_LAMBDA_PARAM_NO_DEFAULT,
        OP_RULE, R__LOOP0_72,
        OP_RULE, R_LAMBDA_KWDS,
        OP_OPTIONAL, 
        OP_RETURN, A_LAMBDA_STAR_ETC_0,

        // '*' ',' lambda_param_maybe_default+ lambda_kwds?
        OP_TOKEN, STAR,
        OP_TOKEN, COMMA,
        OP_RULE, R__LOOP1_73,
        OP_RULE, R_LAMBDA_KWDS,
        OP_OPTIONAL, 
        OP_RETURN, A_LAMBDA_STAR_ETC_1,

        // lambda_kwds
        OP_RULE, R_LAMBDA_KWDS,
        OP_RETURN, A_LAMBDA_STAR_ETC_2,

        // invalid_lambda_star_etc
        OP_RULE, R_INVALID_LAMBDA_STAR_ETC,
        OP_RETURN, A_LAMBDA_STAR_ETC_3,

     },
    },
    {"lambda_kwds",
     R_LAMBDA_KWDS,
     {0, -1},
     {
        // '**' lambda_param_no_default
        OP_TOKEN, DOUBLESTAR,
        OP_RULE, R_LAMBDA_PARAM_NO_DEFAULT,
        OP_RETURN, A_LAMBDA_KWDS_0,

     },
    },
    {"lambda_param_no_default",
     R_LAMBDA_PARAM_NO_DEFAULT,
     {0, 6, -1},
     {
        // lambda_param ','
        OP_RULE, R_LAMBDA_PARAM,
        OP_TOKEN, COMMA,
        OP_RETURN, A_LAMBDA_PARAM_NO_DEFAULT_0,

        // lambda_param &':'
        OP_RULE, R_LAMBDA_PARAM,
        OP_SAVE_MARK, 
        OP_TOKEN, COLON,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_LAMBDA_PARAM_NO_DEFAULT_1,

     },
    },
    {"lambda_param_with_default",
     R_LAMBDA_PARAM_WITH_DEFAULT,
     {0, 8, -1},
     {
        // lambda_param default ','
        OP_RULE, R_LAMBDA_PARAM,
        OP_RULE, R_DEFAULT,
        OP_TOKEN, COMMA,
        OP_RETURN, A_LAMBDA_PARAM_WITH_DEFAULT_0,

        // lambda_param default &':'
        OP_RULE, R_LAMBDA_PARAM,
        OP_RULE, R_DEFAULT,
        OP_SAVE_MARK, 
        OP_TOKEN, COLON,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_LAMBDA_PARAM_WITH_DEFAULT_1,

     },
    },
    {"lambda_param_maybe_default",
     R_LAMBDA_PARAM_MAYBE_DEFAULT,
     {0, 9, -1},
     {
        // lambda_param default? ','
        OP_RULE, R_LAMBDA_PARAM,
        OP_RULE, R_DEFAULT,
        OP_OPTIONAL, 
        OP_TOKEN, COMMA,
        OP_RETURN, A_LAMBDA_PARAM_MAYBE_DEFAULT_0,

        // lambda_param default? &':'
        OP_RULE, R_LAMBDA_PARAM,
        OP_RULE, R_DEFAULT,
        OP_OPTIONAL, 
        OP_SAVE_MARK, 
        OP_TOKEN, COLON,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_LAMBDA_PARAM_MAYBE_DEFAULT_1,

     },
    },
    {"lambda_param",
     R_LAMBDA_PARAM,
     {0, -1},
     {
        // NAME
        OP_NAME, 
        OP_RETURN, A_LAMBDA_PARAM_0,

     },
    },
    {"disjunction",
     R_DISJUNCTION,
     {0, 6, -1},
     {
        // conjunction (('or' conjunction))+
        OP_RULE, R_CONJUNCTION,
        OP_RULE, R__LOOP1_74,
        OP_RETURN, A_DISJUNCTION_0,

        // conjunction
        OP_RULE, R_CONJUNCTION,
        OP_RETURN, A_DISJUNCTION_1,

     },
    },
    {"conjunction",
     R_CONJUNCTION,
     {0, 6, -1},
     {
        // inversion (('and' inversion))+
        OP_RULE, R_INVERSION,
        OP_RULE, R__LOOP1_75,
        OP_RETURN, A_CONJUNCTION_0,

        // inversion
        OP_RULE, R_INVERSION,
        OP_RETURN, A_CONJUNCTION_1,

     },
    },
    {"inversion",
     R_INVERSION,
     {0, 6, -1},
     {
        // 'not' inversion
        OP_TOKEN, 525,
        OP_RULE, R_INVERSION,
        OP_RETURN, A_INVERSION_0,

        // comparison
        OP_RULE, R_COMPARISON,
        OP_RETURN, A_INVERSION_1,

     },
    },
    {"comparison",
     R_COMPARISON,
     {0, 6, -1},
     {
        // bitwise_or compare_op_bitwise_or_pair+
        OP_RULE, R_BITWISE_OR,
        OP_RULE, R__LOOP1_76,
        OP_RETURN, A_COMPARISON_0,

        // bitwise_or
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_COMPARISON_1,

     },
    },
    {"compare_op_bitwise_or_pair",
     R_COMPARE_OP_BITWISE_OR_PAIR,
     {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, -1},
     {
        // eq_bitwise_or
        OP_RULE, R_EQ_BITWISE_OR,
        OP_RETURN, A_COMPARE_OP_BITWISE_OR_PAIR_0,

        // noteq_bitwise_or
        OP_RULE, R_NOTEQ_BITWISE_OR,
        OP_RETURN, A_COMPARE_OP_BITWISE_OR_PAIR_1,

        // lte_bitwise_or
        OP_RULE, R_LTE_BITWISE_OR,
        OP_RETURN, A_COMPARE_OP_BITWISE_OR_PAIR_2,

        // lt_bitwise_or
        OP_RULE, R_LT_BITWISE_OR,
        OP_RETURN, A_COMPARE_OP_BITWISE_OR_PAIR_3,

        // gte_bitwise_or
        OP_RULE, R_GTE_BITWISE_OR,
        OP_RETURN, A_COMPARE_OP_BITWISE_OR_PAIR_4,

        // gt_bitwise_or
        OP_RULE, R_GT_BITWISE_OR,
        OP_RETURN, A_COMPARE_OP_BITWISE_OR_PAIR_5,

        // notin_bitwise_or
        OP_RULE, R_NOTIN_BITWISE_OR,
        OP_RETURN, A_COMPARE_OP_BITWISE_OR_PAIR_6,

        // in_bitwise_or
        OP_RULE, R_IN_BITWISE_OR,
        OP_RETURN, A_COMPARE_OP_BITWISE_OR_PAIR_7,

        // isnot_bitwise_or
        OP_RULE, R_ISNOT_BITWISE_OR,
        OP_RETURN, A_COMPARE_OP_BITWISE_OR_PAIR_8,

        // is_bitwise_or
        OP_RULE, R_IS_BITWISE_OR,
        OP_RETURN, A_COMPARE_OP_BITWISE_OR_PAIR_9,

     },
    },
    {"eq_bitwise_or",
     R_EQ_BITWISE_OR,
     {0, -1},
     {
        // '==' bitwise_or
        OP_TOKEN, EQEQUAL,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_EQ_BITWISE_OR_0,

     },
    },
    {"noteq_bitwise_or",
     R_NOTEQ_BITWISE_OR,
     {0, -1},
     {
        // ('!=') bitwise_or
        OP_RULE, R__TMP_77,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_NOTEQ_BITWISE_OR_0,

     },
    },
    {"lte_bitwise_or",
     R_LTE_BITWISE_OR,
     {0, -1},
     {
        // '<=' bitwise_or
        OP_TOKEN, LESSEQUAL,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_LTE_BITWISE_OR_0,

     },
    },
    {"lt_bitwise_or",
     R_LT_BITWISE_OR,
     {0, -1},
     {
        // '<' bitwise_or
        OP_TOKEN, LESS,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_LT_BITWISE_OR_0,

     },
    },
    {"gte_bitwise_or",
     R_GTE_BITWISE_OR,
     {0, -1},
     {
        // '>=' bitwise_or
        OP_TOKEN, GREATEREQUAL,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_GTE_BITWISE_OR_0,

     },
    },
    {"gt_bitwise_or",
     R_GT_BITWISE_OR,
     {0, -1},
     {
        // '>' bitwise_or
        OP_TOKEN, GREATER,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_GT_BITWISE_OR_0,

     },
    },
    {"notin_bitwise_or",
     R_NOTIN_BITWISE_OR,
     {0, -1},
     {
        // 'not' 'in' bitwise_or
        OP_TOKEN, 525,
        OP_TOKEN, 518,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_NOTIN_BITWISE_OR_0,

     },
    },
    {"in_bitwise_or",
     R_IN_BITWISE_OR,
     {0, -1},
     {
        // 'in' bitwise_or
        OP_TOKEN, 518,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_IN_BITWISE_OR_0,

     },
    },
    {"isnot_bitwise_or",
     R_ISNOT_BITWISE_OR,
     {0, -1},
     {
        // 'is' 'not' bitwise_or
        OP_TOKEN, 526,
        OP_TOKEN, 525,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_ISNOT_BITWISE_OR_0,

     },
    },
    {"is_bitwise_or",
     R_IS_BITWISE_OR,
     {0, -1},
     {
        // 'is' bitwise_or
        OP_TOKEN, 526,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_IS_BITWISE_OR_0,

     },
    },
    {"bitwise_or",
     R_BITWISE_OR,
     {0, 9, -1},
     {
        // bitwise_or '|' bitwise_xor
        OP_SETUP_LEFT_REC, 
        OP_RULE, R_BITWISE_OR,
        OP_TOKEN, VBAR,
        OP_RULE, R_BITWISE_XOR,
        OP_RETURN_LEFT_REC, A_BITWISE_OR_0,

        // bitwise_xor
        OP_RULE, R_BITWISE_XOR,
        OP_RETURN_LEFT_REC, A_BITWISE_OR_1,

     },
    },
    {"bitwise_xor",
     R_BITWISE_XOR,
     {0, 9, -1},
     {
        // bitwise_xor '^' bitwise_and
        OP_SETUP_LEFT_REC, 
        OP_RULE, R_BITWISE_XOR,
        OP_TOKEN, CIRCUMFLEX,
        OP_RULE, R_BITWISE_AND,
        OP_RETURN_LEFT_REC, A_BITWISE_XOR_0,

        // bitwise_and
        OP_RULE, R_BITWISE_AND,
        OP_RETURN_LEFT_REC, A_BITWISE_XOR_1,

     },
    },
    {"bitwise_and",
     R_BITWISE_AND,
     {0, 9, -1},
     {
        // bitwise_and '&' shift_expr
        OP_SETUP_LEFT_REC, 
        OP_RULE, R_BITWISE_AND,
        OP_TOKEN, AMPER,
        OP_RULE, R_SHIFT_EXPR,
        OP_RETURN_LEFT_REC, A_BITWISE_AND_0,

        // shift_expr
        OP_RULE, R_SHIFT_EXPR,
        OP_RETURN_LEFT_REC, A_BITWISE_AND_1,

     },
    },
    {"shift_expr",
     R_SHIFT_EXPR,
     {0, 9, 17, -1},
     {
        // shift_expr '<<' sum
        OP_SETUP_LEFT_REC, 
        OP_RULE, R_SHIFT_EXPR,
        OP_TOKEN, LEFTSHIFT,
        OP_RULE, R_SUM,
        OP_RETURN_LEFT_REC, A_SHIFT_EXPR_0,

        // shift_expr '>>' sum
        OP_RULE, R_SHIFT_EXPR,
        OP_TOKEN, RIGHTSHIFT,
        OP_RULE, R_SUM,
        OP_RETURN_LEFT_REC, A_SHIFT_EXPR_1,

        // sum
        OP_RULE, R_SUM,
        OP_RETURN_LEFT_REC, A_SHIFT_EXPR_2,

     },
    },
    {"sum",
     R_SUM,
     {0, 9, 17, -1},
     {
        // sum '+' term
        OP_SETUP_LEFT_REC, 
        OP_RULE, R_SUM,
        OP_TOKEN, PLUS,
        OP_RULE, R_TERM,
        OP_RETURN_LEFT_REC, A_SUM_0,

        // sum '-' term
        OP_RULE, R_SUM,
        OP_TOKEN, MINUS,
        OP_RULE, R_TERM,
        OP_RETURN_LEFT_REC, A_SUM_1,

        // term
        OP_RULE, R_TERM,
        OP_RETURN_LEFT_REC, A_SUM_2,

     },
    },
    {"term",
     R_TERM,
     {0, 9, 17, 25, 33, 41, -1},
     {
        // term '*' factor
        OP_SETUP_LEFT_REC, 
        OP_RULE, R_TERM,
        OP_TOKEN, STAR,
        OP_RULE, R_FACTOR,
        OP_RETURN_LEFT_REC, A_TERM_0,

        // term '/' factor
        OP_RULE, R_TERM,
        OP_TOKEN, SLASH,
        OP_RULE, R_FACTOR,
        OP_RETURN_LEFT_REC, A_TERM_1,

        // term '//' factor
        OP_RULE, R_TERM,
        OP_TOKEN, DOUBLESLASH,
        OP_RULE, R_FACTOR,
        OP_RETURN_LEFT_REC, A_TERM_2,

        // term '%' factor
        OP_RULE, R_TERM,
        OP_TOKEN, PERCENT,
        OP_RULE, R_FACTOR,
        OP_RETURN_LEFT_REC, A_TERM_3,

        // term '@' factor
        OP_RULE, R_TERM,
        OP_TOKEN, AT,
        OP_RULE, R_FACTOR,
        OP_RETURN_LEFT_REC, A_TERM_4,

        // factor
        OP_RULE, R_FACTOR,
        OP_RETURN_LEFT_REC, A_TERM_5,

     },
    },
    {"factor",
     R_FACTOR,
     {0, 6, 12, 18, -1},
     {
        // '+' factor
        OP_TOKEN, PLUS,
        OP_RULE, R_FACTOR,
        OP_RETURN, A_FACTOR_0,

        // '-' factor
        OP_TOKEN, MINUS,
        OP_RULE, R_FACTOR,
        OP_RETURN, A_FACTOR_1,

        // '~' factor
        OP_TOKEN, TILDE,
        OP_RULE, R_FACTOR,
        OP_RETURN, A_FACTOR_2,

        // power
        OP_RULE, R_POWER,
        OP_RETURN, A_FACTOR_3,

     },
    },
    {"power",
     R_POWER,
     {0, 8, -1},
     {
        // await_primary '**' factor
        OP_RULE, R_AWAIT_PRIMARY,
        OP_TOKEN, DOUBLESTAR,
        OP_RULE, R_FACTOR,
        OP_RETURN, A_POWER_0,

        // await_primary
        OP_RULE, R_AWAIT_PRIMARY,
        OP_RETURN, A_POWER_1,

     },
    },
    {"await_primary",
     R_AWAIT_PRIMARY,
     {0, 6, -1},
     {
        // AWAIT primary
        OP_TOKEN, AWAIT,
        OP_RULE, R_PRIMARY,
        OP_RETURN, A_AWAIT_PRIMARY_0,

        // primary
        OP_RULE, R_PRIMARY,
        OP_RETURN, A_AWAIT_PRIMARY_1,

     },
    },
    {"primary",
     R_PRIMARY,
     {0, 8, 14, 25, 35, -1},
     {
        // primary '.' NAME
        OP_SETUP_LEFT_REC, 
        OP_RULE, R_PRIMARY,
        OP_TOKEN, DOT,
        OP_NAME, 
        OP_RETURN_LEFT_REC, A_PRIMARY_0,

        // primary genexp
        OP_RULE, R_PRIMARY,
        OP_RULE, R_GENEXP,
        OP_RETURN_LEFT_REC, A_PRIMARY_1,

        // primary '(' arguments? ')'
        OP_RULE, R_PRIMARY,
        OP_TOKEN, LPAR,
        OP_RULE, R_ARGUMENTS,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_RETURN_LEFT_REC, A_PRIMARY_2,

        // primary '[' slices ']'
        OP_RULE, R_PRIMARY,
        OP_TOKEN, LSQB,
        OP_RULE, R_SLICES,
        OP_TOKEN, RSQB,
        OP_RETURN_LEFT_REC, A_PRIMARY_3,

        // atom
        OP_RULE, R_ATOM,
        OP_RETURN_LEFT_REC, A_PRIMARY_4,

     },
    },
    {"slices",
     R_SLICES,
     {0, 8, -1},
     {
        // slice !','
        OP_RULE, R_SLICE,
        OP_SAVE_MARK, 
        OP_TOKEN, COMMA,
        OP_NEG_LOOKAHEAD, 
        OP_RETURN, A_SLICES_0,

        // ','.slice+ ','?
        OP_RULE, R__GATHER_78,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_RETURN, A_SLICES_1,

     },
    },
    {"slice",
     R_SLICE,
     {0, 13, -1},
     {
        // expression? ':' expression? [':' expression?]
        OP_RULE, R_EXPRESSION,
        OP_OPTIONAL, 
        OP_TOKEN, COLON,
        OP_RULE, R_EXPRESSION,
        OP_OPTIONAL, 
        OP_RULE, R__TMP_79,
        OP_OPTIONAL, 
        OP_RETURN, A_SLICE_0,

        // expression
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_SLICE_1,

     },
    },
    {"atom",
     R_ATOM,
     {0, 3, 7, 11, 15, 19, 26, 29, 37, 45, 53, -1},
     {
        // NAME
        OP_NAME, 
        OP_RETURN, A_ATOM_0,

        // 'True'
        OP_TOKEN, 527,
        OP_RETURN, A_ATOM_1,

        // 'False'
        OP_TOKEN, 528,
        OP_RETURN, A_ATOM_2,

        // 'None'
        OP_TOKEN, 529,
        OP_RETURN, A_ATOM_3,

        // '__new_parser__'
        OP_TOKEN, 530,
        OP_RETURN, A_ATOM_4,

        // &STRING strings
        OP_SAVE_MARK, 
        OP_STRING, 
        OP_POS_LOOKAHEAD, 
        OP_RULE, R_STRINGS,
        OP_RETURN, A_ATOM_5,

        // NUMBER
        OP_NUMBER, 
        OP_RETURN, A_ATOM_6,

        // &'(' (tuple | group | genexp)
        OP_SAVE_MARK, 
        OP_TOKEN, LPAR,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R__TMP_80,
        OP_RETURN, A_ATOM_7,

        // &'[' (list | listcomp)
        OP_SAVE_MARK, 
        OP_TOKEN, LSQB,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R__TMP_81,
        OP_RETURN, A_ATOM_8,

        // &'{' (dict | set | dictcomp | setcomp)
        OP_SAVE_MARK, 
        OP_TOKEN, LBRACE,
        OP_POS_LOOKAHEAD, 
        OP_RULE, R__TMP_82,
        OP_RETURN, A_ATOM_9,

        // '...'
        OP_TOKEN, ELLIPSIS,
        OP_RETURN, A_ATOM_10,

     },
    },
    {"strings",
     R_STRINGS,
     {0, -1},
     {
        // STRING+
        OP_RULE, R__LOOP1_83,
        OP_RETURN, A_STRINGS_0,

     },
    },
    {"list",
     R_LIST,
     {0, -1},
     {
        // '[' star_named_expressions? ']'
        OP_TOKEN, LSQB,
        OP_RULE, R_STAR_NAMED_EXPRESSIONS,
        OP_OPTIONAL, 
        OP_TOKEN, RSQB,
        OP_RETURN, A_LIST_0,

     },
    },
    {"listcomp",
     R_LISTCOMP,
     {0, 10, -1},
     {
        // '[' named_expression for_if_clauses ']'
        OP_TOKEN, LSQB,
        OP_RULE, R_NAMED_EXPRESSION,
        OP_RULE, R_FOR_IF_CLAUSES,
        OP_TOKEN, RSQB,
        OP_RETURN, A_LISTCOMP_0,

        // invalid_comprehension
        OP_RULE, R_INVALID_COMPREHENSION,
        OP_RETURN, A_LISTCOMP_1,

     },
    },
    {"tuple",
     R_TUPLE,
     {0, -1},
     {
        // '(' [star_named_expression ',' star_named_expressions?] ')'
        OP_TOKEN, LPAR,
        OP_RULE, R__TMP_84,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_RETURN, A_TUPLE_0,

     },
    },
    {"group",
     R_GROUP,
     {0, -1},
     {
        // '(' (yield_expr | named_expression) ')'
        OP_TOKEN, LPAR,
        OP_RULE, R__TMP_85,
        OP_TOKEN, RPAR,
        OP_RETURN, A_GROUP_0,

     },
    },
    {"genexp",
     R_GENEXP,
     {0, 10, -1},
     {
        // '(' expression for_if_clauses ')'
        OP_TOKEN, LPAR,
        OP_RULE, R_EXPRESSION,
        OP_RULE, R_FOR_IF_CLAUSES,
        OP_TOKEN, RPAR,
        OP_RETURN, A_GENEXP_0,

        // invalid_comprehension
        OP_RULE, R_INVALID_COMPREHENSION,
        OP_RETURN, A_GENEXP_1,

     },
    },
    {"set",
     R_SET,
     {0, -1},
     {
        // '{' expressions_list '}'
        OP_TOKEN, LBRACE,
        OP_RULE, R_EXPRESSIONS_LIST,
        OP_TOKEN, RBRACE,
        OP_RETURN, A_SET_0,

     },
    },
    {"setcomp",
     R_SETCOMP,
     {0, 10, -1},
     {
        // '{' expression for_if_clauses '}'
        OP_TOKEN, LBRACE,
        OP_RULE, R_EXPRESSION,
        OP_RULE, R_FOR_IF_CLAUSES,
        OP_TOKEN, RBRACE,
        OP_RETURN, A_SETCOMP_0,

        // invalid_comprehension
        OP_RULE, R_INVALID_COMPREHENSION,
        OP_RETURN, A_SETCOMP_1,

     },
    },
    {"dict",
     R_DICT,
     {0, -1},
     {
        // '{' double_starred_kvpairs? '}'
        OP_TOKEN, LBRACE,
        OP_RULE, R_DOUBLE_STARRED_KVPAIRS,
        OP_OPTIONAL, 
        OP_TOKEN, RBRACE,
        OP_RETURN, A_DICT_0,

     },
    },
    {"dictcomp",
     R_DICTCOMP,
     {0, 10, -1},
     {
        // '{' kvpair for_if_clauses '}'
        OP_TOKEN, LBRACE,
        OP_RULE, R_KVPAIR,
        OP_RULE, R_FOR_IF_CLAUSES,
        OP_TOKEN, RBRACE,
        OP_RETURN, A_DICTCOMP_0,

        // invalid_dict_comprehension
        OP_RULE, R_INVALID_DICT_COMPREHENSION,
        OP_RETURN, A_DICTCOMP_1,

     },
    },
    {"double_starred_kvpairs",
     R_DOUBLE_STARRED_KVPAIRS,
     {0, -1},
     {
        // ','.double_starred_kvpair+ ','?
        OP_RULE, R__GATHER_86,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_RETURN, A_DOUBLE_STARRED_KVPAIRS_0,

     },
    },
    {"double_starred_kvpair",
     R_DOUBLE_STARRED_KVPAIR,
     {0, 6, -1},
     {
        // '**' bitwise_or
        OP_TOKEN, DOUBLESTAR,
        OP_RULE, R_BITWISE_OR,
        OP_RETURN, A_DOUBLE_STARRED_KVPAIR_0,

        // kvpair
        OP_RULE, R_KVPAIR,
        OP_RETURN, A_DOUBLE_STARRED_KVPAIR_1,

     },
    },
    {"kvpair",
     R_KVPAIR,
     {0, -1},
     {
        // expression ':' expression
        OP_RULE, R_EXPRESSION,
        OP_TOKEN, COLON,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_KVPAIR_0,

     },
    },
    {"for_if_clauses",
     R_FOR_IF_CLAUSES,
     {0, -1},
     {
        // for_if_clause+
        OP_RULE, R__LOOP1_87,
        OP_RETURN, A_FOR_IF_CLAUSES_0,

     },
    },
    {"for_if_clause",
     R_FOR_IF_CLAUSE,
     {0, 14, -1},
     {
        // ASYNC 'for' star_targets 'in' disjunction (('if' disjunction))*
        OP_TOKEN, ASYNC,
        OP_TOKEN, 517,
        OP_RULE, R_STAR_TARGETS,
        OP_TOKEN, 518,
        OP_RULE, R_DISJUNCTION,
        OP_RULE, R__LOOP0_88,
        OP_RETURN, A_FOR_IF_CLAUSE_0,

        // 'for' star_targets 'in' disjunction (('if' disjunction))*
        OP_TOKEN, 517,
        OP_RULE, R_STAR_TARGETS,
        OP_TOKEN, 518,
        OP_RULE, R_DISJUNCTION,
        OP_RULE, R__LOOP0_89,
        OP_RETURN, A_FOR_IF_CLAUSE_1,

     },
    },
    {"yield_expr",
     R_YIELD_EXPR,
     {0, 8, -1},
     {
        // 'yield' 'from' expression
        OP_TOKEN, 504,
        OP_TOKEN, 514,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_YIELD_EXPR_0,

        // 'yield' star_expressions?
        OP_TOKEN, 504,
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_OPTIONAL, 
        OP_RETURN, A_YIELD_EXPR_1,

     },
    },
    {"arguments",
     R_ARGUMENTS,
     {0, 11, -1},
     {
        // args ','? &')'
        OP_RULE, R_ARGS,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_SAVE_MARK, 
        OP_TOKEN, RPAR,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_ARGUMENTS_0,

        // incorrect_arguments
        OP_RULE, R_INCORRECT_ARGUMENTS,
        OP_RETURN, A_ARGUMENTS_1,

     },
    },
    {"args",
     R_ARGS,
     {0, 7, 11, -1},
     {
        // starred_expression [',' args]
        OP_RULE, R_STARRED_EXPRESSION,
        OP_RULE, R__TMP_90,
        OP_OPTIONAL, 
        OP_RETURN, A_ARGS_0,

        // kwargs
        OP_RULE, R_KWARGS,
        OP_RETURN, A_ARGS_1,

        // named_expression [',' args]
        OP_RULE, R_NAMED_EXPRESSION,
        OP_RULE, R__TMP_91,
        OP_OPTIONAL, 
        OP_RETURN, A_ARGS_2,

     },
    },
    {"kwargs",
     R_KWARGS,
     {0, 8, 12, -1},
     {
        // ','.kwarg_or_starred+ ',' ','.kwarg_or_double_starred+
        OP_RULE, R__GATHER_92,
        OP_TOKEN, COMMA,
        OP_RULE, R__GATHER_93,
        OP_RETURN, A_KWARGS_0,

        // ','.kwarg_or_starred+
        OP_RULE, R__GATHER_94,
        OP_RETURN, A_KWARGS_1,

        // ','.kwarg_or_double_starred+
        OP_RULE, R__GATHER_95,
        OP_RETURN, A_KWARGS_2,

     },
    },
    {"starred_expression",
     R_STARRED_EXPRESSION,
     {0, -1},
     {
        // '*' expression
        OP_TOKEN, STAR,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_STARRED_EXPRESSION_0,

     },
    },
    {"kwarg_or_starred",
     R_KWARG_OR_STARRED,
     {0, 7, 11, -1},
     {
        // NAME '=' expression
        OP_NAME, 
        OP_TOKEN, EQUAL,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_KWARG_OR_STARRED_0,

        // starred_expression
        OP_RULE, R_STARRED_EXPRESSION,
        OP_RETURN, A_KWARG_OR_STARRED_1,

        // invalid_kwarg
        OP_RULE, R_INVALID_KWARG,
        OP_RETURN, A_KWARG_OR_STARRED_2,

     },
    },
    {"kwarg_or_double_starred",
     R_KWARG_OR_DOUBLE_STARRED,
     {0, 7, 13, -1},
     {
        // NAME '=' expression
        OP_NAME, 
        OP_TOKEN, EQUAL,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_KWARG_OR_DOUBLE_STARRED_0,

        // '**' expression
        OP_TOKEN, DOUBLESTAR,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_KWARG_OR_DOUBLE_STARRED_1,

        // invalid_kwarg
        OP_RULE, R_INVALID_KWARG,
        OP_RETURN, A_KWARG_OR_DOUBLE_STARRED_2,

     },
    },
    {"star_targets",
     R_STAR_TARGETS,
     {0, 8, -1},
     {
        // star_target !','
        OP_RULE, R_STAR_TARGET,
        OP_SAVE_MARK, 
        OP_TOKEN, COMMA,
        OP_NEG_LOOKAHEAD, 
        OP_RETURN, A_STAR_TARGETS_0,

        // star_target ((',' star_target))* ','?
        OP_RULE, R_STAR_TARGET,
        OP_RULE, R__LOOP0_96,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_RETURN, A_STAR_TARGETS_1,

     },
    },
    {"star_targets_seq",
     R_STAR_TARGETS_SEQ,
     {0, -1},
     {
        // ','.star_target+ ','?
        OP_RULE, R__GATHER_97,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_RETURN, A_STAR_TARGETS_SEQ_0,

     },
    },
    {"star_target",
     R_STAR_TARGET,
     {0, 6, 17, 31, -1},
     {
        // '*' (!'*' star_target)
        OP_TOKEN, STAR,
        OP_RULE, R__TMP_98,
        OP_RETURN, A_STAR_TARGET_0,

        // t_primary '.' NAME !t_lookahead
        OP_RULE, R_T_PRIMARY,
        OP_TOKEN, DOT,
        OP_NAME, 
        OP_SAVE_MARK, 
        OP_RULE, R_T_LOOKAHEAD,
        OP_NEG_LOOKAHEAD, 
        OP_RETURN, A_STAR_TARGET_1,

        // t_primary '[' slices ']' !t_lookahead
        OP_RULE, R_T_PRIMARY,
        OP_TOKEN, LSQB,
        OP_RULE, R_SLICES,
        OP_TOKEN, RSQB,
        OP_SAVE_MARK, 
        OP_RULE, R_T_LOOKAHEAD,
        OP_NEG_LOOKAHEAD, 
        OP_RETURN, A_STAR_TARGET_2,

        // star_atom
        OP_RULE, R_STAR_ATOM,
        OP_RETURN, A_STAR_TARGET_3,

     },
    },
    {"star_atom",
     R_STAR_ATOM,
     {0, 3, 11, 20, -1},
     {
        // NAME
        OP_NAME, 
        OP_RETURN, A_STAR_ATOM_0,

        // '(' star_target ')'
        OP_TOKEN, LPAR,
        OP_RULE, R_STAR_TARGET,
        OP_TOKEN, RPAR,
        OP_RETURN, A_STAR_ATOM_1,

        // '(' star_targets_seq? ')'
        OP_TOKEN, LPAR,
        OP_RULE, R_STAR_TARGETS_SEQ,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_RETURN, A_STAR_ATOM_2,

        // '[' star_targets_seq? ']'
        OP_TOKEN, LSQB,
        OP_RULE, R_STAR_TARGETS_SEQ,
        OP_OPTIONAL, 
        OP_TOKEN, RSQB,
        OP_RETURN, A_STAR_ATOM_3,

     },
    },
    {"single_target",
     R_SINGLE_TARGET,
     {0, 4, 7, -1},
     {
        // single_subscript_attribute_target
        OP_RULE, R_SINGLE_SUBSCRIPT_ATTRIBUTE_TARGET,
        OP_RETURN, A_SINGLE_TARGET_0,

        // NAME
        OP_NAME, 
        OP_RETURN, A_SINGLE_TARGET_1,

        // '(' single_target ')'
        OP_TOKEN, LPAR,
        OP_RULE, R_SINGLE_TARGET,
        OP_TOKEN, RPAR,
        OP_RETURN, A_SINGLE_TARGET_2,

     },
    },
    {"single_subscript_attribute_target",
     R_SINGLE_SUBSCRIPT_ATTRIBUTE_TARGET,
     {0, 11, -1},
     {
        // t_primary '.' NAME !t_lookahead
        OP_RULE, R_T_PRIMARY,
        OP_TOKEN, DOT,
        OP_NAME, 
        OP_SAVE_MARK, 
        OP_RULE, R_T_LOOKAHEAD,
        OP_NEG_LOOKAHEAD, 
        OP_RETURN, A_SINGLE_SUBSCRIPT_ATTRIBUTE_TARGET_0,

        // t_primary '[' slices ']' !t_lookahead
        OP_RULE, R_T_PRIMARY,
        OP_TOKEN, LSQB,
        OP_RULE, R_SLICES,
        OP_TOKEN, RSQB,
        OP_SAVE_MARK, 
        OP_RULE, R_T_LOOKAHEAD,
        OP_NEG_LOOKAHEAD, 
        OP_RETURN, A_SINGLE_SUBSCRIPT_ATTRIBUTE_TARGET_1,

     },
    },
    {"del_targets",
     R_DEL_TARGETS,
     {0, -1},
     {
        // ','.del_target+ ','?
        OP_RULE, R__GATHER_99,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_RETURN, A_DEL_TARGETS_0,

     },
    },
    {"del_target",
     R_DEL_TARGET,
     {0, 11, 25, -1},
     {
        // t_primary '.' NAME &del_target_end
        OP_RULE, R_T_PRIMARY,
        OP_TOKEN, DOT,
        OP_NAME, 
        OP_SAVE_MARK, 
        OP_RULE, R_DEL_TARGET_END,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_DEL_TARGET_0,

        // t_primary '[' slices ']' &del_target_end
        OP_RULE, R_T_PRIMARY,
        OP_TOKEN, LSQB,
        OP_RULE, R_SLICES,
        OP_TOKEN, RSQB,
        OP_SAVE_MARK, 
        OP_RULE, R_DEL_TARGET_END,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_DEL_TARGET_1,

        // del_t_atom
        OP_RULE, R_DEL_T_ATOM,
        OP_RETURN, A_DEL_TARGET_2,

     },
    },
    {"del_t_atom",
     R_DEL_T_ATOM,
     {0, 7, 15, 24, 33, -1},
     {
        // NAME &del_target_end
        OP_NAME, 
        OP_SAVE_MARK, 
        OP_RULE, R_DEL_TARGET_END,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_DEL_T_ATOM_0,

        // '(' del_target ')'
        OP_TOKEN, LPAR,
        OP_RULE, R_DEL_TARGET,
        OP_TOKEN, RPAR,
        OP_RETURN, A_DEL_T_ATOM_1,

        // '(' del_targets? ')'
        OP_TOKEN, LPAR,
        OP_RULE, R_DEL_TARGETS,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_RETURN, A_DEL_T_ATOM_2,

        // '[' del_targets? ']'
        OP_TOKEN, LSQB,
        OP_RULE, R_DEL_TARGETS,
        OP_OPTIONAL, 
        OP_TOKEN, RSQB,
        OP_RETURN, A_DEL_T_ATOM_3,

        // invalid_del_target
        OP_RULE, R_INVALID_DEL_TARGET,
        OP_RETURN, A_DEL_T_ATOM_4,

     },
    },
    {"del_target_end",
     R_DEL_TARGET_END,
     {0, 4, 8, 12, 16, -1},
     {
        // ')'
        OP_TOKEN, RPAR,
        OP_RETURN, A_DEL_TARGET_END_0,

        // ']'
        OP_TOKEN, RSQB,
        OP_RETURN, A_DEL_TARGET_END_1,

        // ','
        OP_TOKEN, COMMA,
        OP_RETURN, A_DEL_TARGET_END_2,

        // ';'
        OP_TOKEN, SEMI,
        OP_RETURN, A_DEL_TARGET_END_3,

        // NEWLINE
        OP_TOKEN, NEWLINE,
        OP_RETURN, A_DEL_TARGET_END_4,

     },
    },
    {"targets",
     R_TARGETS,
     {0, -1},
     {
        // ','.target+ ','?
        OP_RULE, R__GATHER_100,
        OP_TOKEN, COMMA,
        OP_OPTIONAL, 
        OP_RETURN, A_TARGETS_0,

     },
    },
    {"target",
     R_TARGET,
     {0, 11, 25, -1},
     {
        // t_primary '.' NAME !t_lookahead
        OP_RULE, R_T_PRIMARY,
        OP_TOKEN, DOT,
        OP_NAME, 
        OP_SAVE_MARK, 
        OP_RULE, R_T_LOOKAHEAD,
        OP_NEG_LOOKAHEAD, 
        OP_RETURN, A_TARGET_0,

        // t_primary '[' slices ']' !t_lookahead
        OP_RULE, R_T_PRIMARY,
        OP_TOKEN, LSQB,
        OP_RULE, R_SLICES,
        OP_TOKEN, RSQB,
        OP_SAVE_MARK, 
        OP_RULE, R_T_LOOKAHEAD,
        OP_NEG_LOOKAHEAD, 
        OP_RETURN, A_TARGET_1,

        // t_atom
        OP_RULE, R_T_ATOM,
        OP_RETURN, A_TARGET_2,

     },
    },
    {"t_primary",
     R_T_PRIMARY,
     {0, 12, 26, 36, 51, -1},
     {
        // t_primary '.' NAME &t_lookahead
        OP_SETUP_LEFT_REC, 
        OP_RULE, R_T_PRIMARY,
        OP_TOKEN, DOT,
        OP_NAME, 
        OP_SAVE_MARK, 
        OP_RULE, R_T_LOOKAHEAD,
        OP_POS_LOOKAHEAD, 
        OP_RETURN_LEFT_REC, A_T_PRIMARY_0,

        // t_primary '[' slices ']' &t_lookahead
        OP_RULE, R_T_PRIMARY,
        OP_TOKEN, LSQB,
        OP_RULE, R_SLICES,
        OP_TOKEN, RSQB,
        OP_SAVE_MARK, 
        OP_RULE, R_T_LOOKAHEAD,
        OP_POS_LOOKAHEAD, 
        OP_RETURN_LEFT_REC, A_T_PRIMARY_1,

        // t_primary genexp &t_lookahead
        OP_RULE, R_T_PRIMARY,
        OP_RULE, R_GENEXP,
        OP_SAVE_MARK, 
        OP_RULE, R_T_LOOKAHEAD,
        OP_POS_LOOKAHEAD, 
        OP_RETURN_LEFT_REC, A_T_PRIMARY_2,

        // t_primary '(' arguments? ')' &t_lookahead
        OP_RULE, R_T_PRIMARY,
        OP_TOKEN, LPAR,
        OP_RULE, R_ARGUMENTS,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_SAVE_MARK, 
        OP_RULE, R_T_LOOKAHEAD,
        OP_POS_LOOKAHEAD, 
        OP_RETURN_LEFT_REC, A_T_PRIMARY_3,

        // atom &t_lookahead
        OP_RULE, R_ATOM,
        OP_SAVE_MARK, 
        OP_RULE, R_T_LOOKAHEAD,
        OP_POS_LOOKAHEAD, 
        OP_RETURN_LEFT_REC, A_T_PRIMARY_4,

     },
    },
    {"t_lookahead",
     R_T_LOOKAHEAD,
     {0, 4, 8, -1},
     {
        // '('
        OP_TOKEN, LPAR,
        OP_RETURN, A_T_LOOKAHEAD_0,

        // '['
        OP_TOKEN, LSQB,
        OP_RETURN, A_T_LOOKAHEAD_1,

        // '.'
        OP_TOKEN, DOT,
        OP_RETURN, A_T_LOOKAHEAD_2,

     },
    },
    {"t_atom",
     R_T_ATOM,
     {0, 3, 11, 20, -1},
     {
        // NAME
        OP_NAME, 
        OP_RETURN, A_T_ATOM_0,

        // '(' target ')'
        OP_TOKEN, LPAR,
        OP_RULE, R_TARGET,
        OP_TOKEN, RPAR,
        OP_RETURN, A_T_ATOM_1,

        // '(' targets? ')'
        OP_TOKEN, LPAR,
        OP_RULE, R_TARGETS,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_RETURN, A_T_ATOM_2,

        // '[' targets? ']'
        OP_TOKEN, LSQB,
        OP_RULE, R_TARGETS,
        OP_OPTIONAL, 
        OP_TOKEN, RSQB,
        OP_RETURN, A_T_ATOM_3,

     },
    },
    {"incorrect_arguments",
     R_INCORRECT_ARGUMENTS,
     {0, 8, 19, 25, 35, -1},
     {
        // args ',' '*'
        OP_RULE, R_ARGS,
        OP_TOKEN, COMMA,
        OP_TOKEN, STAR,
        OP_RETURN, A_INCORRECT_ARGUMENTS_0,

        // expression for_if_clauses ',' [args | expression for_if_clauses]
        OP_RULE, R_EXPRESSION,
        OP_RULE, R_FOR_IF_CLAUSES,
        OP_TOKEN, COMMA,
        OP_RULE, R__TMP_101,
        OP_OPTIONAL, 
        OP_RETURN, A_INCORRECT_ARGUMENTS_1,

        // args for_if_clauses
        OP_RULE, R_ARGS,
        OP_RULE, R_FOR_IF_CLAUSES,
        OP_RETURN, A_INCORRECT_ARGUMENTS_2,

        // args ',' expression for_if_clauses
        OP_RULE, R_ARGS,
        OP_TOKEN, COMMA,
        OP_RULE, R_EXPRESSION,
        OP_RULE, R_FOR_IF_CLAUSES,
        OP_RETURN, A_INCORRECT_ARGUMENTS_3,

        // args ',' args
        OP_RULE, R_ARGS,
        OP_TOKEN, COMMA,
        OP_RULE, R_ARGS,
        OP_RETURN, A_INCORRECT_ARGUMENTS_4,

     },
    },
    {"invalid_kwarg",
     R_INVALID_KWARG,
     {0, -1},
     {
        // expression '='
        OP_RULE, R_EXPRESSION,
        OP_TOKEN, EQUAL,
        OP_RETURN, A_INVALID_KWARG_0,

     },
    },
    {"invalid_named_expression",
     R_INVALID_NAMED_EXPRESSION,
     {0, -1},
     {
        // expression ':=' expression
        OP_RULE, R_EXPRESSION,
        OP_TOKEN, COLONEQUAL,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A_INVALID_NAMED_EXPRESSION_0,

     },
    },
    {"invalid_assignment",
     R_INVALID_ASSIGNMENT,
     {0, 6, 12, 22, 33, 41, -1},
     {
        // list ':'
        OP_RULE, R_LIST,
        OP_TOKEN, COLON,
        OP_RETURN, A_INVALID_ASSIGNMENT_0,

        // tuple ':'
        OP_RULE, R_TUPLE,
        OP_TOKEN, COLON,
        OP_RETURN, A_INVALID_ASSIGNMENT_1,

        // star_named_expression ',' star_named_expressions* ':'
        OP_RULE, R_STAR_NAMED_EXPRESSION,
        OP_TOKEN, COMMA,
        OP_RULE, R__LOOP0_102,
        OP_TOKEN, COLON,
        OP_RETURN, A_INVALID_ASSIGNMENT_2,

        // expression ':' expression ['=' annotated_rhs]
        OP_RULE, R_EXPRESSION,
        OP_TOKEN, COLON,
        OP_RULE, R_EXPRESSION,
        OP_RULE, R__TMP_103,
        OP_OPTIONAL, 
        OP_RETURN, A_INVALID_ASSIGNMENT_3,

        // star_expressions '=' (yield_expr | star_expressions)
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_TOKEN, EQUAL,
        OP_RULE, R__TMP_104,
        OP_RETURN, A_INVALID_ASSIGNMENT_4,

        // star_expressions augassign (yield_expr | star_expressions)
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_RULE, R_AUGASSIGN,
        OP_RULE, R__TMP_105,
        OP_RETURN, A_INVALID_ASSIGNMENT_5,

     },
    },
    {"invalid_block",
     R_INVALID_BLOCK,
     {0, -1},
     {
        // NEWLINE !INDENT
        OP_TOKEN, NEWLINE,
        OP_SAVE_MARK, 
        OP_TOKEN, INDENT,
        OP_NEG_LOOKAHEAD, 
        OP_RETURN, A_INVALID_BLOCK_0,

     },
    },
    {"invalid_comprehension",
     R_INVALID_COMPREHENSION,
     {0, -1},
     {
        // ('[' | '(' | '{') starred_expression for_if_clauses
        OP_RULE, R__TMP_106,
        OP_RULE, R_STARRED_EXPRESSION,
        OP_RULE, R_FOR_IF_CLAUSES,
        OP_RETURN, A_INVALID_COMPREHENSION_0,

     },
    },
    {"invalid_dict_comprehension",
     R_INVALID_DICT_COMPREHENSION,
     {0, -1},
     {
        // '{' '**' bitwise_or for_if_clauses '}'
        OP_TOKEN, LBRACE,
        OP_TOKEN, DOUBLESTAR,
        OP_RULE, R_BITWISE_OR,
        OP_RULE, R_FOR_IF_CLAUSES,
        OP_TOKEN, RBRACE,
        OP_RETURN, A_INVALID_DICT_COMPREHENSION_0,

     },
    },
    {"invalid_parameters",
     R_INVALID_PARAMETERS,
     {0, -1},
     {
        // param_no_default* (slash_with_default | param_with_default+) param_no_default
        OP_RULE, R__LOOP0_107,
        OP_RULE, R__TMP_108,
        OP_RULE, R_PARAM_NO_DEFAULT,
        OP_RETURN, A_INVALID_PARAMETERS_0,

     },
    },
    {"invalid_star_etc",
     R_INVALID_STAR_ETC,
     {0, 6, -1},
     {
        // '*' (')' | ',' (')' | '**'))
        OP_TOKEN, STAR,
        OP_RULE, R__TMP_109,
        OP_RETURN, A_INVALID_STAR_ETC_0,

        // '*' ',' TYPE_COMMENT
        OP_TOKEN, STAR,
        OP_TOKEN, COMMA,
        OP_TOKEN, TYPE_COMMENT,
        OP_RETURN, A_INVALID_STAR_ETC_1,

     },
    },
    {"invalid_lambda_star_etc",
     R_INVALID_LAMBDA_STAR_ETC,
     {0, -1},
     {
        // '*' (':' | ',' (':' | '**'))
        OP_TOKEN, STAR,
        OP_RULE, R__TMP_110,
        OP_RETURN, A_INVALID_LAMBDA_STAR_ETC_0,

     },
    },
    {"invalid_double_type_comments",
     R_INVALID_DOUBLE_TYPE_COMMENTS,
     {0, -1},
     {
        // TYPE_COMMENT NEWLINE TYPE_COMMENT NEWLINE INDENT
        OP_TOKEN, TYPE_COMMENT,
        OP_TOKEN, NEWLINE,
        OP_TOKEN, TYPE_COMMENT,
        OP_TOKEN, NEWLINE,
        OP_TOKEN, INDENT,
        OP_RETURN, A_INVALID_DOUBLE_TYPE_COMMENTS_0,

     },
    },
    {"invalid_del_target",
     R_INVALID_DEL_TARGET,
     {0, -1},
     {
        // star_expression &del_target_end
        OP_RULE, R_STAR_EXPRESSION,
        OP_SAVE_MARK, 
        OP_RULE, R_DEL_TARGET_END,
        OP_POS_LOOKAHEAD, 
        OP_RETURN, A_INVALID_DEL_TARGET_0,

     },
    },
    {"invalid_import_from_targets",
     R_INVALID_IMPORT_FROM_TARGETS,
     {0, -1},
     {
        // import_from_as_names ','
        OP_RULE, R_IMPORT_FROM_AS_NAMES,
        OP_TOKEN, COMMA,
        OP_RETURN, A_INVALID_IMPORT_FROM_TARGETS_0,

     },
    },
    {"root",
     R_ROOT,
     {0, 3, -1},
     {
        // <Artificial alternative>
        OP_RULE, R_FILE,
        OP_SUCCESS, 

        // <Artificial alternative>
        OP_FAILURE, 

     },
    },
    {"_loop0_1",
     R__LOOP0_1,
     {0, 3, -1},
     {
        // NEWLINE
        OP_TOKEN, NEWLINE,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop0_2",
     R__LOOP0_2,
     {0, 3, -1},
     {
        // NEWLINE
        OP_TOKEN, NEWLINE,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_gather_3",
     R__GATHER_3,
     {0, 5, -1},
     {
        // expression ','
        OP_RULE, R_EXPRESSION,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // expression
        OP_RULE, R_EXPRESSION,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_gather_4",
     R__GATHER_4,
     {0, 5, -1},
     {
        // expression ','
        OP_RULE, R_EXPRESSION,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // expression
        OP_RULE, R_EXPRESSION,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_gather_5",
     R__GATHER_5,
     {0, 5, -1},
     {
        // expression ','
        OP_RULE, R_EXPRESSION,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // expression
        OP_RULE, R_EXPRESSION,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_gather_6",
     R__GATHER_6,
     {0, 5, -1},
     {
        // expression ','
        OP_RULE, R_EXPRESSION,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // expression
        OP_RULE, R_EXPRESSION,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_loop1_7",
     R__LOOP1_7,
     {0, 3, -1},
     {
        // statement
        OP_RULE, R_STATEMENT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_gather_8",
     R__GATHER_8,
     {0, 5, -1},
     {
        // small_stmt ';'
        OP_RULE, R_SMALL_STMT,
        OP_TOKEN, SEMI,
        OP_LOOP_ITERATE, 

        // small_stmt
        OP_RULE, R_SMALL_STMT,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_tmp_9",
     R__TMP_9,
     {0, 4, -1},
     {
        // 'import'
        OP_TOKEN, 513,
        OP_RETURN, A__TMP_9_0,

        // 'from'
        OP_TOKEN, 514,
        OP_RETURN, A__TMP_9_1,

     },
    },
    {"_tmp_10",
     R__TMP_10,
     {0, 4, 8, -1},
     {
        // 'def'
        OP_TOKEN, 522,
        OP_RETURN, A__TMP_10_0,

        // '@'
        OP_TOKEN, AT,
        OP_RETURN, A__TMP_10_1,

        // ASYNC
        OP_TOKEN, ASYNC,
        OP_RETURN, A__TMP_10_2,

     },
    },
    {"_tmp_11",
     R__TMP_11,
     {0, 4, -1},
     {
        // 'class'
        OP_TOKEN, 523,
        OP_RETURN, A__TMP_11_0,

        // '@'
        OP_TOKEN, AT,
        OP_RETURN, A__TMP_11_1,

     },
    },
    {"_tmp_12",
     R__TMP_12,
     {0, 4, -1},
     {
        // 'with'
        OP_TOKEN, 519,
        OP_RETURN, A__TMP_12_0,

        // ASYNC
        OP_TOKEN, ASYNC,
        OP_RETURN, A__TMP_12_1,

     },
    },
    {"_tmp_13",
     R__TMP_13,
     {0, 4, -1},
     {
        // 'for'
        OP_TOKEN, 517,
        OP_RETURN, A__TMP_13_0,

        // ASYNC
        OP_TOKEN, ASYNC,
        OP_RETURN, A__TMP_13_1,

     },
    },
    {"_tmp_14",
     R__TMP_14,
     {0, -1},
     {
        // '=' annotated_rhs
        OP_TOKEN, EQUAL,
        OP_RULE, R_ANNOTATED_RHS,
        OP_RETURN, A__TMP_14_0,

     },
    },
    {"_tmp_15",
     R__TMP_15,
     {0, 8, -1},
     {
        // '(' single_target ')'
        OP_TOKEN, LPAR,
        OP_RULE, R_SINGLE_TARGET,
        OP_TOKEN, RPAR,
        OP_RETURN, A__TMP_15_0,

        // single_subscript_attribute_target
        OP_RULE, R_SINGLE_SUBSCRIPT_ATTRIBUTE_TARGET,
        OP_RETURN, A__TMP_15_1,

     },
    },
    {"_tmp_16",
     R__TMP_16,
     {0, -1},
     {
        // '=' annotated_rhs
        OP_TOKEN, EQUAL,
        OP_RULE, R_ANNOTATED_RHS,
        OP_RETURN, A__TMP_16_0,

     },
    },
    {"_loop1_17",
     R__LOOP1_17,
     {0, 3, -1},
     {
        // (star_targets '=')
        OP_RULE, R__TMP_111,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_tmp_18",
     R__TMP_18,
     {0, 4, -1},
     {
        // yield_expr
        OP_RULE, R_YIELD_EXPR,
        OP_RETURN, A__TMP_18_0,

        // star_expressions
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_RETURN, A__TMP_18_1,

     },
    },
    {"_tmp_19",
     R__TMP_19,
     {0, 4, -1},
     {
        // yield_expr
        OP_RULE, R_YIELD_EXPR,
        OP_RETURN, A__TMP_19_0,

        // star_expressions
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_RETURN, A__TMP_19_1,

     },
    },
    {"_gather_20",
     R__GATHER_20,
     {0, 4, -1},
     {
        // NAME ','
        OP_NAME, 
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // NAME
        OP_NAME, 
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_gather_21",
     R__GATHER_21,
     {0, 4, -1},
     {
        // NAME ','
        OP_NAME, 
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // NAME
        OP_NAME, 
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_tmp_22",
     R__TMP_22,
     {0, -1},
     {
        // ',' expression
        OP_TOKEN, COMMA,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A__TMP_22_0,

     },
    },
    {"_loop0_23",
     R__LOOP0_23,
     {0, 3, -1},
     {
        // ('.' | '...')
        OP_RULE, R__TMP_112,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop1_24",
     R__LOOP1_24,
     {0, 3, -1},
     {
        // ('.' | '...')
        OP_RULE, R__TMP_113,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_gather_25",
     R__GATHER_25,
     {0, 5, -1},
     {
        // import_from_as_name ','
        OP_RULE, R_IMPORT_FROM_AS_NAME,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // import_from_as_name
        OP_RULE, R_IMPORT_FROM_AS_NAME,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_tmp_26",
     R__TMP_26,
     {0, -1},
     {
        // 'as' NAME
        OP_TOKEN, 531,
        OP_NAME, 
        OP_RETURN, A__TMP_26_0,

     },
    },
    {"_gather_27",
     R__GATHER_27,
     {0, 5, -1},
     {
        // dotted_as_name ','
        OP_RULE, R_DOTTED_AS_NAME,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // dotted_as_name
        OP_RULE, R_DOTTED_AS_NAME,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_tmp_28",
     R__TMP_28,
     {0, -1},
     {
        // 'as' NAME
        OP_TOKEN, 531,
        OP_NAME, 
        OP_RETURN, A__TMP_28_0,

     },
    },
    {"_gather_29",
     R__GATHER_29,
     {0, 5, -1},
     {
        // with_item ','
        OP_RULE, R_WITH_ITEM,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // with_item
        OP_RULE, R_WITH_ITEM,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_gather_30",
     R__GATHER_30,
     {0, 5, -1},
     {
        // with_item ','
        OP_RULE, R_WITH_ITEM,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // with_item
        OP_RULE, R_WITH_ITEM,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_gather_31",
     R__GATHER_31,
     {0, 5, -1},
     {
        // with_item ','
        OP_RULE, R_WITH_ITEM,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // with_item
        OP_RULE, R_WITH_ITEM,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_gather_32",
     R__GATHER_32,
     {0, 5, -1},
     {
        // with_item ','
        OP_RULE, R_WITH_ITEM,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // with_item
        OP_RULE, R_WITH_ITEM,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_tmp_33",
     R__TMP_33,
     {0, -1},
     {
        // 'as' target
        OP_TOKEN, 531,
        OP_RULE, R_TARGET,
        OP_RETURN, A__TMP_33_0,

     },
    },
    {"_loop1_34",
     R__LOOP1_34,
     {0, 3, -1},
     {
        // except_block
        OP_RULE, R_EXCEPT_BLOCK,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_tmp_35",
     R__TMP_35,
     {0, -1},
     {
        // 'as' NAME
        OP_TOKEN, 531,
        OP_NAME, 
        OP_RETURN, A__TMP_35_0,

     },
    },
    {"_tmp_36",
     R__TMP_36,
     {0, -1},
     {
        // 'from' expression
        OP_TOKEN, 514,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A__TMP_36_0,

     },
    },
    {"_tmp_37",
     R__TMP_37,
     {0, -1},
     {
        // '->' expression
        OP_TOKEN, RARROW,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A__TMP_37_0,

     },
    },
    {"_tmp_38",
     R__TMP_38,
     {0, -1},
     {
        // '->' expression
        OP_TOKEN, RARROW,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A__TMP_38_0,

     },
    },
    {"_tmp_39",
     R__TMP_39,
     {0, -1},
     {
        // NEWLINE INDENT
        OP_TOKEN, NEWLINE,
        OP_TOKEN, INDENT,
        OP_RETURN, A__TMP_39_0,

     },
    },
    {"_loop0_40",
     R__LOOP0_40,
     {0, 3, -1},
     {
        // param_no_default
        OP_RULE, R_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop0_41",
     R__LOOP0_41,
     {0, 3, -1},
     {
        // param_with_default
        OP_RULE, R_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop0_42",
     R__LOOP0_42,
     {0, 3, -1},
     {
        // param_with_default
        OP_RULE, R_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop1_43",
     R__LOOP1_43,
     {0, 3, -1},
     {
        // param_no_default
        OP_RULE, R_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop0_44",
     R__LOOP0_44,
     {0, 3, -1},
     {
        // param_with_default
        OP_RULE, R_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop1_45",
     R__LOOP1_45,
     {0, 3, -1},
     {
        // param_with_default
        OP_RULE, R_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop1_46",
     R__LOOP1_46,
     {0, 3, -1},
     {
        // param_no_default
        OP_RULE, R_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop1_47",
     R__LOOP1_47,
     {0, 3, -1},
     {
        // param_no_default
        OP_RULE, R_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop0_48",
     R__LOOP0_48,
     {0, 3, -1},
     {
        // param_no_default
        OP_RULE, R_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop1_49",
     R__LOOP1_49,
     {0, 3, -1},
     {
        // param_with_default
        OP_RULE, R_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop0_50",
     R__LOOP0_50,
     {0, 3, -1},
     {
        // param_no_default
        OP_RULE, R_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop1_51",
     R__LOOP1_51,
     {0, 3, -1},
     {
        // param_with_default
        OP_RULE, R_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop0_52",
     R__LOOP0_52,
     {0, 3, -1},
     {
        // param_maybe_default
        OP_RULE, R_PARAM_MAYBE_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop1_53",
     R__LOOP1_53,
     {0, 3, -1},
     {
        // param_maybe_default
        OP_RULE, R_PARAM_MAYBE_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop1_54",
     R__LOOP1_54,
     {0, 3, -1},
     {
        // ('@' named_expression NEWLINE)
        OP_RULE, R__TMP_114,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_tmp_55",
     R__TMP_55,
     {0, -1},
     {
        // '(' arguments? ')'
        OP_TOKEN, LPAR,
        OP_RULE, R_ARGUMENTS,
        OP_OPTIONAL, 
        OP_TOKEN, RPAR,
        OP_RETURN, A__TMP_55_0,

     },
    },
    {"_gather_56",
     R__GATHER_56,
     {0, 5, -1},
     {
        // star_expression ','
        OP_RULE, R_STAR_EXPRESSION,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // star_expression
        OP_RULE, R_STAR_EXPRESSION,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_loop1_57",
     R__LOOP1_57,
     {0, 3, -1},
     {
        // (',' star_expression)
        OP_RULE, R__TMP_115,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_gather_58",
     R__GATHER_58,
     {0, 5, -1},
     {
        // star_named_expression ','
        OP_RULE, R_STAR_NAMED_EXPRESSION,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // star_named_expression
        OP_RULE, R_STAR_NAMED_EXPRESSION,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_loop1_59",
     R__LOOP1_59,
     {0, 3, -1},
     {
        // (',' expression)
        OP_RULE, R__TMP_116,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop0_60",
     R__LOOP0_60,
     {0, 3, -1},
     {
        // lambda_param_no_default
        OP_RULE, R_LAMBDA_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop0_61",
     R__LOOP0_61,
     {0, 3, -1},
     {
        // lambda_param_with_default
        OP_RULE, R_LAMBDA_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop0_62",
     R__LOOP0_62,
     {0, 3, -1},
     {
        // lambda_param_with_default
        OP_RULE, R_LAMBDA_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop1_63",
     R__LOOP1_63,
     {0, 3, -1},
     {
        // lambda_param_no_default
        OP_RULE, R_LAMBDA_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop0_64",
     R__LOOP0_64,
     {0, 3, -1},
     {
        // lambda_param_with_default
        OP_RULE, R_LAMBDA_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop1_65",
     R__LOOP1_65,
     {0, 3, -1},
     {
        // lambda_param_with_default
        OP_RULE, R_LAMBDA_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop1_66",
     R__LOOP1_66,
     {0, 3, -1},
     {
        // lambda_param_no_default
        OP_RULE, R_LAMBDA_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop1_67",
     R__LOOP1_67,
     {0, 3, -1},
     {
        // lambda_param_no_default
        OP_RULE, R_LAMBDA_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop0_68",
     R__LOOP0_68,
     {0, 3, -1},
     {
        // lambda_param_no_default
        OP_RULE, R_LAMBDA_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop1_69",
     R__LOOP1_69,
     {0, 3, -1},
     {
        // lambda_param_with_default
        OP_RULE, R_LAMBDA_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop0_70",
     R__LOOP0_70,
     {0, 3, -1},
     {
        // lambda_param_no_default
        OP_RULE, R_LAMBDA_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop1_71",
     R__LOOP1_71,
     {0, 3, -1},
     {
        // lambda_param_with_default
        OP_RULE, R_LAMBDA_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop0_72",
     R__LOOP0_72,
     {0, 3, -1},
     {
        // lambda_param_maybe_default
        OP_RULE, R_LAMBDA_PARAM_MAYBE_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop1_73",
     R__LOOP1_73,
     {0, 3, -1},
     {
        // lambda_param_maybe_default
        OP_RULE, R_LAMBDA_PARAM_MAYBE_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop1_74",
     R__LOOP1_74,
     {0, 3, -1},
     {
        // ('or' conjunction)
        OP_RULE, R__TMP_117,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop1_75",
     R__LOOP1_75,
     {0, 3, -1},
     {
        // ('and' inversion)
        OP_RULE, R__TMP_118,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop1_76",
     R__LOOP1_76,
     {0, 3, -1},
     {
        // compare_op_bitwise_or_pair
        OP_RULE, R_COMPARE_OP_BITWISE_OR_PAIR,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_tmp_77",
     R__TMP_77,
     {0, -1},
     {
        // '!='
        OP_TOKEN, NOTEQUAL,
        OP_RETURN, A__TMP_77_0,

     },
    },
    {"_gather_78",
     R__GATHER_78,
     {0, 5, -1},
     {
        // slice ','
        OP_RULE, R_SLICE,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // slice
        OP_RULE, R_SLICE,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_tmp_79",
     R__TMP_79,
     {0, -1},
     {
        // ':' expression?
        OP_TOKEN, COLON,
        OP_RULE, R_EXPRESSION,
        OP_OPTIONAL, 
        OP_RETURN, A__TMP_79_0,

     },
    },
    {"_tmp_80",
     R__TMP_80,
     {0, 4, 8, -1},
     {
        // tuple
        OP_RULE, R_TUPLE,
        OP_RETURN, A__TMP_80_0,

        // group
        OP_RULE, R_GROUP,
        OP_RETURN, A__TMP_80_1,

        // genexp
        OP_RULE, R_GENEXP,
        OP_RETURN, A__TMP_80_2,

     },
    },
    {"_tmp_81",
     R__TMP_81,
     {0, 4, -1},
     {
        // list
        OP_RULE, R_LIST,
        OP_RETURN, A__TMP_81_0,

        // listcomp
        OP_RULE, R_LISTCOMP,
        OP_RETURN, A__TMP_81_1,

     },
    },
    {"_tmp_82",
     R__TMP_82,
     {0, 4, 8, 12, -1},
     {
        // dict
        OP_RULE, R_DICT,
        OP_RETURN, A__TMP_82_0,

        // set
        OP_RULE, R_SET,
        OP_RETURN, A__TMP_82_1,

        // dictcomp
        OP_RULE, R_DICTCOMP,
        OP_RETURN, A__TMP_82_2,

        // setcomp
        OP_RULE, R_SETCOMP,
        OP_RETURN, A__TMP_82_3,

     },
    },
    {"_loop1_83",
     R__LOOP1_83,
     {0, 2, -1},
     {
        // STRING
        OP_STRING, 
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_tmp_84",
     R__TMP_84,
     {0, -1},
     {
        // star_named_expression ',' star_named_expressions?
        OP_RULE, R_STAR_NAMED_EXPRESSION,
        OP_TOKEN, COMMA,
        OP_RULE, R_STAR_NAMED_EXPRESSIONS,
        OP_OPTIONAL, 
        OP_RETURN, A__TMP_84_0,

     },
    },
    {"_tmp_85",
     R__TMP_85,
     {0, 4, -1},
     {
        // yield_expr
        OP_RULE, R_YIELD_EXPR,
        OP_RETURN, A__TMP_85_0,

        // named_expression
        OP_RULE, R_NAMED_EXPRESSION,
        OP_RETURN, A__TMP_85_1,

     },
    },
    {"_gather_86",
     R__GATHER_86,
     {0, 5, -1},
     {
        // double_starred_kvpair ','
        OP_RULE, R_DOUBLE_STARRED_KVPAIR,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // double_starred_kvpair
        OP_RULE, R_DOUBLE_STARRED_KVPAIR,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_loop1_87",
     R__LOOP1_87,
     {0, 3, -1},
     {
        // for_if_clause
        OP_RULE, R_FOR_IF_CLAUSE,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_loop0_88",
     R__LOOP0_88,
     {0, 3, -1},
     {
        // ('if' disjunction)
        OP_RULE, R__TMP_119,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_loop0_89",
     R__LOOP0_89,
     {0, 3, -1},
     {
        // ('if' disjunction)
        OP_RULE, R__TMP_120,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_tmp_90",
     R__TMP_90,
     {0, -1},
     {
        // ',' args
        OP_TOKEN, COMMA,
        OP_RULE, R_ARGS,
        OP_RETURN, A__TMP_90_0,

     },
    },
    {"_tmp_91",
     R__TMP_91,
     {0, -1},
     {
        // ',' args
        OP_TOKEN, COMMA,
        OP_RULE, R_ARGS,
        OP_RETURN, A__TMP_91_0,

     },
    },
    {"_gather_92",
     R__GATHER_92,
     {0, 5, -1},
     {
        // kwarg_or_starred ','
        OP_RULE, R_KWARG_OR_STARRED,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // kwarg_or_starred
        OP_RULE, R_KWARG_OR_STARRED,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_gather_93",
     R__GATHER_93,
     {0, 5, -1},
     {
        // kwarg_or_double_starred ','
        OP_RULE, R_KWARG_OR_DOUBLE_STARRED,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // kwarg_or_double_starred
        OP_RULE, R_KWARG_OR_DOUBLE_STARRED,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_gather_94",
     R__GATHER_94,
     {0, 5, -1},
     {
        // kwarg_or_starred ','
        OP_RULE, R_KWARG_OR_STARRED,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // kwarg_or_starred
        OP_RULE, R_KWARG_OR_STARRED,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_gather_95",
     R__GATHER_95,
     {0, 5, -1},
     {
        // kwarg_or_double_starred ','
        OP_RULE, R_KWARG_OR_DOUBLE_STARRED,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // kwarg_or_double_starred
        OP_RULE, R_KWARG_OR_DOUBLE_STARRED,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_loop0_96",
     R__LOOP0_96,
     {0, 3, -1},
     {
        // (',' star_target)
        OP_RULE, R__TMP_121,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_gather_97",
     R__GATHER_97,
     {0, 5, -1},
     {
        // star_target ','
        OP_RULE, R_STAR_TARGET,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // star_target
        OP_RULE, R_STAR_TARGET,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_tmp_98",
     R__TMP_98,
     {0, -1},
     {
        // !'*' star_target
        OP_SAVE_MARK, 
        OP_TOKEN, STAR,
        OP_NEG_LOOKAHEAD, 
        OP_RULE, R_STAR_TARGET,
        OP_RETURN, A__TMP_98_0,

     },
    },
    {"_gather_99",
     R__GATHER_99,
     {0, 5, -1},
     {
        // del_target ','
        OP_RULE, R_DEL_TARGET,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // del_target
        OP_RULE, R_DEL_TARGET,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_gather_100",
     R__GATHER_100,
     {0, 5, -1},
     {
        // target ','
        OP_RULE, R_TARGET,
        OP_TOKEN, COMMA,
        OP_LOOP_ITERATE, 

        // target
        OP_RULE, R_TARGET,
        OP_LOOP_COLLECT_DELIMITED, 

     },
    },
    {"_tmp_101",
     R__TMP_101,
     {0, 4, -1},
     {
        // args
        OP_RULE, R_ARGS,
        OP_RETURN, A__TMP_101_0,

        // expression for_if_clauses
        OP_RULE, R_EXPRESSION,
        OP_RULE, R_FOR_IF_CLAUSES,
        OP_RETURN, A__TMP_101_1,

     },
    },
    {"_loop0_102",
     R__LOOP0_102,
     {0, 3, -1},
     {
        // star_named_expressions
        OP_RULE, R_STAR_NAMED_EXPRESSIONS,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_tmp_103",
     R__TMP_103,
     {0, -1},
     {
        // '=' annotated_rhs
        OP_TOKEN, EQUAL,
        OP_RULE, R_ANNOTATED_RHS,
        OP_RETURN, A__TMP_103_0,

     },
    },
    {"_tmp_104",
     R__TMP_104,
     {0, 4, -1},
     {
        // yield_expr
        OP_RULE, R_YIELD_EXPR,
        OP_RETURN, A__TMP_104_0,

        // star_expressions
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_RETURN, A__TMP_104_1,

     },
    },
    {"_tmp_105",
     R__TMP_105,
     {0, 4, -1},
     {
        // yield_expr
        OP_RULE, R_YIELD_EXPR,
        OP_RETURN, A__TMP_105_0,

        // star_expressions
        OP_RULE, R_STAR_EXPRESSIONS,
        OP_RETURN, A__TMP_105_1,

     },
    },
    {"_tmp_106",
     R__TMP_106,
     {0, 4, 8, -1},
     {
        // '['
        OP_TOKEN, LSQB,
        OP_RETURN, A__TMP_106_0,

        // '('
        OP_TOKEN, LPAR,
        OP_RETURN, A__TMP_106_1,

        // '{'
        OP_TOKEN, LBRACE,
        OP_RETURN, A__TMP_106_2,

     },
    },
    {"_loop0_107",
     R__LOOP0_107,
     {0, 3, -1},
     {
        // param_no_default
        OP_RULE, R_PARAM_NO_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT, 

     },
    },
    {"_tmp_108",
     R__TMP_108,
     {0, 4, -1},
     {
        // slash_with_default
        OP_RULE, R_SLASH_WITH_DEFAULT,
        OP_RETURN, A__TMP_108_0,

        // param_with_default+
        OP_RULE, R__LOOP1_122,
        OP_RETURN, A__TMP_108_1,

     },
    },
    {"_tmp_109",
     R__TMP_109,
     {0, 4, -1},
     {
        // ')'
        OP_TOKEN, RPAR,
        OP_RETURN, A__TMP_109_0,

        // ',' (')' | '**')
        OP_TOKEN, COMMA,
        OP_RULE, R__TMP_123,
        OP_RETURN, A__TMP_109_1,

     },
    },
    {"_tmp_110",
     R__TMP_110,
     {0, 4, -1},
     {
        // ':'
        OP_TOKEN, COLON,
        OP_RETURN, A__TMP_110_0,

        // ',' (':' | '**')
        OP_TOKEN, COMMA,
        OP_RULE, R__TMP_124,
        OP_RETURN, A__TMP_110_1,

     },
    },
    {"_tmp_111",
     R__TMP_111,
     {0, -1},
     {
        // star_targets '='
        OP_RULE, R_STAR_TARGETS,
        OP_TOKEN, EQUAL,
        OP_RETURN, A__TMP_111_0,

     },
    },
    {"_tmp_112",
     R__TMP_112,
     {0, 4, -1},
     {
        // '.'
        OP_TOKEN, DOT,
        OP_RETURN, A__TMP_112_0,

        // '...'
        OP_TOKEN, ELLIPSIS,
        OP_RETURN, A__TMP_112_1,

     },
    },
    {"_tmp_113",
     R__TMP_113,
     {0, 4, -1},
     {
        // '.'
        OP_TOKEN, DOT,
        OP_RETURN, A__TMP_113_0,

        // '...'
        OP_TOKEN, ELLIPSIS,
        OP_RETURN, A__TMP_113_1,

     },
    },
    {"_tmp_114",
     R__TMP_114,
     {0, -1},
     {
        // '@' named_expression NEWLINE
        OP_TOKEN, AT,
        OP_RULE, R_NAMED_EXPRESSION,
        OP_TOKEN, NEWLINE,
        OP_RETURN, A__TMP_114_0,

     },
    },
    {"_tmp_115",
     R__TMP_115,
     {0, -1},
     {
        // ',' star_expression
        OP_TOKEN, COMMA,
        OP_RULE, R_STAR_EXPRESSION,
        OP_RETURN, A__TMP_115_0,

     },
    },
    {"_tmp_116",
     R__TMP_116,
     {0, -1},
     {
        // ',' expression
        OP_TOKEN, COMMA,
        OP_RULE, R_EXPRESSION,
        OP_RETURN, A__TMP_116_0,

     },
    },
    {"_tmp_117",
     R__TMP_117,
     {0, -1},
     {
        // 'or' conjunction
        OP_TOKEN, 532,
        OP_RULE, R_CONJUNCTION,
        OP_RETURN, A__TMP_117_0,

     },
    },
    {"_tmp_118",
     R__TMP_118,
     {0, -1},
     {
        // 'and' inversion
        OP_TOKEN, 533,
        OP_RULE, R_INVERSION,
        OP_RETURN, A__TMP_118_0,

     },
    },
    {"_tmp_119",
     R__TMP_119,
     {0, -1},
     {
        // 'if' disjunction
        OP_TOKEN, 510,
        OP_RULE, R_DISJUNCTION,
        OP_RETURN, A__TMP_119_0,

     },
    },
    {"_tmp_120",
     R__TMP_120,
     {0, -1},
     {
        // 'if' disjunction
        OP_TOKEN, 510,
        OP_RULE, R_DISJUNCTION,
        OP_RETURN, A__TMP_120_0,

     },
    },
    {"_tmp_121",
     R__TMP_121,
     {0, -1},
     {
        // ',' star_target
        OP_TOKEN, COMMA,
        OP_RULE, R_STAR_TARGET,
        OP_RETURN, A__TMP_121_0,

     },
    },
    {"_loop1_122",
     R__LOOP1_122,
     {0, 3, -1},
     {
        // param_with_default
        OP_RULE, R_PARAM_WITH_DEFAULT,
        OP_LOOP_ITERATE, 

        // <Artificial alternative>
        OP_LOOP_COLLECT_NONEMPTY, 

     },
    },
    {"_tmp_123",
     R__TMP_123,
     {0, 4, -1},
     {
        // ')'
        OP_TOKEN, RPAR,
        OP_RETURN, A__TMP_123_0,

        // '**'
        OP_TOKEN, DOUBLESTAR,
        OP_RETURN, A__TMP_123_1,

     },
    },
    {"_tmp_124",
     R__TMP_124,
     {0, 4, -1},
     {
        // ':'
        OP_TOKEN, COLON,
        OP_RETURN, A__TMP_124_0,

        // '**'
        OP_TOKEN, DOUBLESTAR,
        OP_RETURN, A__TMP_124_1,

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
    case A_FILE_0:
        return _PyPegen_make_module ( p , _f->vals[0] );
    case A_INTERACTIVE_0:
        return Interactive ( ((asdl_seq*)_f->vals[0]) , p -> arena );
    case A_EVAL_0:
        return Expression ( ((expr_ty)_f->vals[0]) , p -> arena );
    case A_FUNC_TYPE_0:
        return FunctionType ( _f->vals[1] , ((expr_ty)_f->vals[4]) , p -> arena );
    case A_FSTRING_0:
    case A_DOTTED_NAME_1:
    case A_STAR_EXPRESSIONS_2:
    case A_STAR_EXPRESSION_1:
    case A_STAR_NAMED_EXPRESSION_1:
    case A_NAMED_EXPRESSION_1:
    case A_ANNOTATED_RHS_0:
    case A_ANNOTATED_RHS_1:
    case A_EXPRESSIONS_2:
    case A_EXPRESSION_1:
    case A_EXPRESSION_2:
    case A_DISJUNCTION_1:
    case A_CONJUNCTION_1:
    case A_INVERSION_1:
    case A_COMPARISON_1:
    case A_BITWISE_OR_1:
    case A_BITWISE_XOR_1:
    case A_BITWISE_AND_1:
    case A_SHIFT_EXPR_2:
    case A_SUM_2:
    case A_TERM_5:
    case A_FACTOR_3:
    case A_POWER_1:
    case A_AWAIT_PRIMARY_1:
    case A_PRIMARY_4:
    case A_SLICES_0:
    case A_SLICE_1:
    case A_ATOM_0:
    case A_ARGUMENTS_0:
    case A_STAR_TARGETS_0:
    case A_STAR_TARGET_3:
    case A_SINGLE_TARGET_0:
    case A_DEL_TARGET_2:
    case A_TARGET_2:
    case A_T_PRIMARY_4:
    case A__GATHER_3_0:
    case A__GATHER_3_1:
    case A__GATHER_4_0:
    case A__GATHER_4_1:
    case A__GATHER_5_0:
    case A__GATHER_5_1:
    case A__GATHER_6_0:
    case A__GATHER_6_1:
    case A__TMP_15_1:
    case A__TMP_18_0:
    case A__TMP_18_1:
    case A__TMP_19_0:
    case A__TMP_19_1:
    case A__GATHER_20_0:
    case A__GATHER_20_1:
    case A__GATHER_21_0:
    case A__GATHER_21_1:
    case A__GATHER_56_0:
    case A__GATHER_56_1:
    case A__GATHER_58_0:
    case A__GATHER_58_1:
    case A__GATHER_78_0:
    case A__GATHER_78_1:
    case A__TMP_80_0:
    case A__TMP_80_1:
    case A__TMP_80_2:
    case A__TMP_81_0:
    case A__TMP_81_1:
    case A__TMP_82_0:
    case A__TMP_82_1:
    case A__TMP_82_2:
    case A__TMP_82_3:
    case A__TMP_85_0:
    case A__TMP_85_1:
    case A__GATHER_97_0:
    case A__GATHER_97_1:
    case A__GATHER_99_0:
    case A__GATHER_99_1:
    case A__GATHER_100_0:
    case A__GATHER_100_1:
    case A__TMP_101_0:
    case A__TMP_101_1:
    case A__TMP_104_0:
    case A__TMP_104_1:
    case A__TMP_105_0:
    case A__TMP_105_1:
    case A__TMP_111_0:
        return ((expr_ty)_f->vals[0]);
    case A_TYPE_EXPRESSIONS_0:
        return _PyPegen_seq_append_to_end ( p , CHECK ( _PyPegen_seq_append_to_end ( p , _f->vals[0] , ((expr_ty)_f->vals[3]) ) ) , ((expr_ty)_f->vals[6]) );
    case A_TYPE_EXPRESSIONS_1:
    case A_TYPE_EXPRESSIONS_2:
        return _PyPegen_seq_append_to_end ( p , _f->vals[0] , ((expr_ty)_f->vals[3]) );
    case A_TYPE_EXPRESSIONS_3:
        return _PyPegen_seq_append_to_end ( p , CHECK ( _PyPegen_singleton_seq ( p , ((expr_ty)_f->vals[1]) ) ) , ((expr_ty)_f->vals[4]) );
    case A_TYPE_EXPRESSIONS_4:
    case A_TYPE_EXPRESSIONS_5:
        return _PyPegen_singleton_seq ( p , ((expr_ty)_f->vals[1]) );
    case A_TYPE_EXPRESSIONS_6:
    case A_SIMPLE_STMT_1:
    case A_SMALL_STMT_2:
    case A_SMALL_STMT_3:
    case A_SMALL_STMT_4:
    case A_SMALL_STMT_6:
    case A_SMALL_STMT_7:
    case A_SMALL_STMT_8:
    case A_SMALL_STMT_11:
    case A_SMALL_STMT_12:
    case A_COMPOUND_STMT_0:
    case A_COMPOUND_STMT_1:
    case A_COMPOUND_STMT_2:
    case A_COMPOUND_STMT_3:
    case A_COMPOUND_STMT_4:
    case A_COMPOUND_STMT_5:
    case A_COMPOUND_STMT_6:
    case A_ASSIGNMENT_4:
    case A_IMPORT_FROM_TARGETS_3:
    case A_IMPORT_FROM_AS_NAMES_0:
    case A_DOTTED_AS_NAMES_0:
    case A_FUNC_TYPE_COMMENT_1:
    case A_FUNC_TYPE_COMMENT_2:
    case A_PARAMS_0:
    case A_SLASH_NO_DEFAULT_0:
    case A_SLASH_NO_DEFAULT_1:
    case A_STAR_ETC_3:
    case A_DECORATORS_0:
    case A_BLOCK_2:
    case A_EXPRESSIONS_LIST_0:
    case A_STAR_NAMED_EXPRESSIONS_0:
    case A_NAMED_EXPRESSION_2:
    case A_LAMBDA_SLASH_NO_DEFAULT_0:
    case A_LAMBDA_SLASH_NO_DEFAULT_1:
    case A_LAMBDA_STAR_ETC_3:
    case A_ATOM_5:
    case A_ATOM_6:
    case A_ATOM_7:
    case A_ATOM_8:
    case A_ATOM_9:
    case A_LISTCOMP_1:
    case A_GENEXP_1:
    case A_SETCOMP_1:
    case A_DICTCOMP_1:
    case A_DOUBLE_STARRED_KVPAIRS_0:
    case A_FOR_IF_CLAUSES_0:
    case A_ARGUMENTS_1:
    case A_KWARGS_1:
    case A_KWARGS_2:
    case A_KWARG_OR_STARRED_2:
    case A_KWARG_OR_DOUBLE_STARRED_2:
    case A_STAR_TARGETS_SEQ_0:
    case A_DEL_TARGETS_0:
    case A_DEL_T_ATOM_4:
    case A_DEL_TARGET_END_4:
    case A_TARGETS_0:
    case A__TMP_10_2:
    case A__TMP_12_1:
    case A__TMP_13_1:
    case A__TMP_39_0:
    case A__TMP_98_0:
    case A__TMP_108_1:
        return _f->vals[0];
    case A_STATEMENTS_0:
        return _PyPegen_seq_flatten ( p , _f->vals[0] );
    case A_STATEMENT_0:
    case A_STATEMENT_NEWLINE_0:
    case A_SIMPLE_STMT_0:
        return _PyPegen_singleton_seq ( p , ((stmt_ty)_f->vals[0]) );
    case A_STATEMENT_1:
    case A_STATEMENT_NEWLINE_1:
    case A_IMPORT_FROM_TARGETS_1:
    case A_BLOCK_1:
        return ((asdl_seq*)_f->vals[0]);
    case A_STATEMENT_NEWLINE_2:
        return _PyPegen_singleton_seq ( p , CHECK ( _Py_Pass ( EXTRA ) ) );
    case A_STATEMENT_NEWLINE_3:
        return _PyPegen_interactive_exit ( p );
    case A_SMALL_STMT_0:
    case A_IMPORT_STMT_0:
    case A_IMPORT_STMT_1:
    case A_FUNCTION_DEF_1:
    case A_CLASS_DEF_1:
    case A__GATHER_8_0:
    case A__GATHER_8_1:
        return ((stmt_ty)_f->vals[0]);
    case A_SMALL_STMT_1:
    case A_YIELD_STMT_0:
        return _Py_Expr ( ((expr_ty)_f->vals[0]) , EXTRA );
    case A_SMALL_STMT_5:
        return _Py_Pass ( EXTRA );
    case A_SMALL_STMT_9:
        return _Py_Break ( EXTRA );
    case A_SMALL_STMT_10:
        return _Py_Continue ( EXTRA );
    case A_ASSIGNMENT_0:
        return CHECK_VERSION ( 6 , "Variable annotation syntax is" , _Py_AnnAssign ( CHECK ( _PyPegen_set_expr_context ( p , ((expr_ty)_f->vals[0]) , Store ) ) , ((expr_ty)_f->vals[2]) , _f->vals[3] , 1 , EXTRA ) );
    case A_ASSIGNMENT_1:
        return CHECK_VERSION ( 6 , "Variable annotations syntax is" , _Py_AnnAssign ( _f->vals[0] , ((expr_ty)_f->vals[2]) , _f->vals[3] , 0 , EXTRA ) );
    case A_ASSIGNMENT_2:
        return _Py_Assign ( _f->vals[0] , _f->vals[1] , NEW_TYPE_COMMENT ( p , _f->vals[2] ) , EXTRA );
    case A_ASSIGNMENT_3:
        return _Py_AugAssign ( ((expr_ty)_f->vals[0]) , ((AugOperator*)_f->vals[1]) -> kind , _f->vals[2] , EXTRA );
    case A_AUGASSIGN_0:
        return _PyPegen_augoperator ( p , Add );
    case A_AUGASSIGN_1:
        return _PyPegen_augoperator ( p , Sub );
    case A_AUGASSIGN_2:
        return _PyPegen_augoperator ( p , Mult );
    case A_AUGASSIGN_3:
        return CHECK_VERSION ( 5 , "The '@' operator is" , _PyPegen_augoperator ( p , MatMult ) );
    case A_AUGASSIGN_4:
        return _PyPegen_augoperator ( p , Div );
    case A_AUGASSIGN_5:
        return _PyPegen_augoperator ( p , Mod );
    case A_AUGASSIGN_6:
        return _PyPegen_augoperator ( p , BitAnd );
    case A_AUGASSIGN_7:
        return _PyPegen_augoperator ( p , BitOr );
    case A_AUGASSIGN_8:
        return _PyPegen_augoperator ( p , BitXor );
    case A_AUGASSIGN_9:
        return _PyPegen_augoperator ( p , LShift );
    case A_AUGASSIGN_10:
        return _PyPegen_augoperator ( p , RShift );
    case A_AUGASSIGN_11:
        return _PyPegen_augoperator ( p , Pow );
    case A_AUGASSIGN_12:
        return _PyPegen_augoperator ( p , FloorDiv );
    case A_GLOBAL_STMT_0:
        return _Py_Global ( CHECK ( _PyPegen_map_names_to_ids ( p , _f->vals[1] ) ) , EXTRA );
    case A_NONLOCAL_STMT_0:
        return _Py_Nonlocal ( CHECK ( _PyPegen_map_names_to_ids ( p , _f->vals[1] ) ) , EXTRA );
    case A_ASSERT_STMT_0:
        return _Py_Assert ( ((expr_ty)_f->vals[1]) , _f->vals[2] , EXTRA );
    case A_DEL_STMT_0:
        return _Py_Delete ( ((asdl_seq*)_f->vals[1]) , EXTRA );
    case A_IMPORT_NAME_0:
        return _Py_Import ( ((asdl_seq*)_f->vals[1]) , EXTRA );
    case A_IMPORT_FROM_0:
        return _Py_ImportFrom ( ((expr_ty)_f->vals[2]) -> v . Name . id , ((asdl_seq*)_f->vals[4]) , _PyPegen_seq_count_dots ( _f->vals[1] ) , EXTRA );
    case A_IMPORT_FROM_1:
        return _Py_ImportFrom ( NULL , ((asdl_seq*)_f->vals[3]) , _PyPegen_seq_count_dots ( _f->vals[1] ) , EXTRA );
    case A_IMPORT_FROM_TARGETS_0:
        return ((asdl_seq*)_f->vals[1]);
    case A_IMPORT_FROM_TARGETS_2:
        return _PyPegen_singleton_seq ( p , CHECK ( _PyPegen_alias_for_star ( p ) ) );
    case A_IMPORT_FROM_AS_NAME_0:
    case A_DOTTED_AS_NAME_0:
        return _Py_alias ( ((expr_ty)_f->vals[0]) -> v . Name . id , ( _f->vals[1] )   ? ( ( expr_ty ) _f->vals[1] ) -> v . Name . id : NULL , p -> arena );
    case A_DOTTED_NAME_0:
        return _PyPegen_join_names_with_dot ( p , ((expr_ty)_f->vals[0]) , ((expr_ty)_f->vals[2]) );
    case A_IF_STMT_0:
    case A_ELIF_STMT_0:
        return _Py_If ( ((expr_ty)_f->vals[1]) , ((asdl_seq*)_f->vals[3]) , CHECK ( _PyPegen_singleton_seq ( p , ((stmt_ty)_f->vals[4]) ) ) , EXTRA );
    case A_IF_STMT_1:
    case A_ELIF_STMT_1:
        return _Py_If ( ((expr_ty)_f->vals[1]) , ((asdl_seq*)_f->vals[3]) , _f->vals[4] , EXTRA );
    case A_ELSE_BLOCK_0:
    case A_FINALLY_BLOCK_0:
    case A_BLOCK_0:
        return ((asdl_seq*)_f->vals[2]);
    case A_WHILE_STMT_0:
        return _Py_While ( ((expr_ty)_f->vals[1]) , ((asdl_seq*)_f->vals[3]) , _f->vals[4] , EXTRA );
    case A_FOR_STMT_0:
        return _Py_For ( ((expr_ty)_f->vals[1]) , ((expr_ty)_f->vals[3]) , ((asdl_seq*)_f->vals[6]) , _f->vals[7] , NEW_TYPE_COMMENT ( p , _f->vals[5] ) , EXTRA );
    case A_FOR_STMT_1:
        return CHECK_VERSION ( 5 , "Async for loops are" , _Py_AsyncFor ( ((expr_ty)_f->vals[2]) , ((expr_ty)_f->vals[4]) , ((asdl_seq*)_f->vals[7]) , _f->vals[8] , NEW_TYPE_COMMENT ( p , _f->vals[6] ) , EXTRA ) );
    case A_WITH_STMT_0:
        return _Py_With ( _f->vals[2] , ((asdl_seq*)_f->vals[6]) , NULL , EXTRA );
    case A_WITH_STMT_1:
        return _Py_With ( _f->vals[1] , ((asdl_seq*)_f->vals[4]) , NEW_TYPE_COMMENT ( p , _f->vals[3] ) , EXTRA );
    case A_WITH_STMT_2:
        return CHECK_VERSION ( 5 , "Async with statements are" , _Py_AsyncWith ( _f->vals[3] , ((asdl_seq*)_f->vals[7]) , NULL , EXTRA ) );
    case A_WITH_STMT_3:
        return CHECK_VERSION ( 5 , "Async with statements are" , _Py_AsyncWith ( _f->vals[2] , ((asdl_seq*)_f->vals[5]) , NEW_TYPE_COMMENT ( p , _f->vals[4] ) , EXTRA ) );
    case A_WITH_ITEM_0:
        return _Py_withitem ( ((expr_ty)_f->vals[0]) , _f->vals[1] , p -> arena );
    case A_TRY_STMT_0:
        return _Py_Try ( ((asdl_seq*)_f->vals[2]) , NULL , NULL , ((asdl_seq*)_f->vals[3]) , EXTRA );
    case A_TRY_STMT_1:
        return _Py_Try ( ((asdl_seq*)_f->vals[2]) , _f->vals[3] , _f->vals[4] , _f->vals[5] , EXTRA );
    case A_EXCEPT_BLOCK_0:
        return _Py_ExceptHandler ( ((expr_ty)_f->vals[1]) , ( _f->vals[2] )   ? ( ( expr_ty ) _f->vals[2] ) -> v . Name . id : NULL , ((asdl_seq*)_f->vals[4]) , EXTRA );
    case A_EXCEPT_BLOCK_1:
        return _Py_ExceptHandler ( NULL , NULL , ((asdl_seq*)_f->vals[2]) , EXTRA );
    case A_RETURN_STMT_0:
        return _Py_Return ( _f->vals[1] , EXTRA );
    case A_RAISE_STMT_0:
        return _Py_Raise ( ((expr_ty)_f->vals[1]) , _f->vals[2] , EXTRA );
    case A_RAISE_STMT_1:
        return _Py_Raise ( NULL , NULL , EXTRA );
    case A_FUNCTION_DEF_0:
        return _PyPegen_function_def_decorators ( p , ((asdl_seq*)_f->vals[0]) , ((stmt_ty)_f->vals[1]) );
    case A_FUNCTION_DEF_RAW_0:
        return _Py_FunctionDef ( ((expr_ty)_f->vals[1]) -> v . Name . id , ( _f->vals[3] )   ? _f->vals[3] : CHECK ( _PyPegen_empty_arguments ( p ) ) , ((asdl_seq*)_f->vals[8]) , NULL , _f->vals[5] , NEW_TYPE_COMMENT ( p , _f->vals[7] ) , EXTRA );
    case A_FUNCTION_DEF_RAW_1:
        return CHECK_VERSION ( 5 , "Async functions are" , _Py_AsyncFunctionDef ( ((expr_ty)_f->vals[2]) -> v . Name . id , ( _f->vals[4] )   ? _f->vals[4] : CHECK ( _PyPegen_empty_arguments ( p ) ) , ((asdl_seq*)_f->vals[9]) , NULL , _f->vals[6] , NEW_TYPE_COMMENT ( p , _f->vals[8] ) , EXTRA ) );
    case A_FUNC_TYPE_COMMENT_0:
    case A_GROUP_0:
    case A__TMP_55_0:
    case A__TMP_79_0:
        return _f->vals[1];
    case A_PARAMS_1:
        return ((arguments_ty)_f->vals[0]);
    case A_PARAMETERS_0:
    case A_LAMBDA_PARAMETERS_0:
        return _PyPegen_make_arguments ( p , ((asdl_seq*)_f->vals[0]) , NULL , _f->vals[1] , _f->vals[2] , _f->vals[3] );
    case A_PARAMETERS_1:
    case A_LAMBDA_PARAMETERS_1:
        return _PyPegen_make_arguments ( p , NULL , ((SlashWithDefault*)_f->vals[0]) , NULL , _f->vals[1] , _f->vals[2] );
    case A_PARAMETERS_2:
    case A_LAMBDA_PARAMETERS_2:
        return _PyPegen_make_arguments ( p , NULL , NULL , _f->vals[0] , _f->vals[1] , _f->vals[2] );
    case A_PARAMETERS_3:
    case A_LAMBDA_PARAMETERS_3:
        return _PyPegen_make_arguments ( p , NULL , NULL , NULL , _f->vals[0] , _f->vals[1] );
    case A_PARAMETERS_4:
    case A_LAMBDA_PARAMETERS_4:
        return _PyPegen_make_arguments ( p , NULL , NULL , NULL , NULL , ((StarEtc*)_f->vals[0]) );
    case A_SLASH_WITH_DEFAULT_0:
    case A_SLASH_WITH_DEFAULT_1:
    case A_LAMBDA_SLASH_WITH_DEFAULT_0:
    case A_LAMBDA_SLASH_WITH_DEFAULT_1:
        return _PyPegen_slash_with_default ( p , _f->vals[0] , _f->vals[1] );
    case A_STAR_ETC_0:
    case A_LAMBDA_STAR_ETC_0:
        return _PyPegen_star_etc ( p , ((arg_ty)_f->vals[1]) , _f->vals[2] , _f->vals[3] );
    case A_STAR_ETC_1:
    case A_LAMBDA_STAR_ETC_1:
        return _PyPegen_star_etc ( p , NULL , _f->vals[2] , _f->vals[3] );
    case A_STAR_ETC_2:
    case A_LAMBDA_STAR_ETC_2:
        return _PyPegen_star_etc ( p , NULL , NULL , ((arg_ty)_f->vals[0]) );
    case A_KWDS_0:
    case A_LAMBDA_KWDS_0:
        return ((arg_ty)_f->vals[1]);
    case A_PARAM_NO_DEFAULT_0:
        return _PyPegen_add_type_comment_to_arg ( p , ((arg_ty)_f->vals[0]) , _f->vals[2] );
    case A_PARAM_NO_DEFAULT_1:
        return _PyPegen_add_type_comment_to_arg ( p , ((arg_ty)_f->vals[0]) , _f->vals[1] );
    case A_PARAM_WITH_DEFAULT_0:
        return _PyPegen_name_default_pair ( p , ((arg_ty)_f->vals[0]) , ((expr_ty)_f->vals[1]) , _f->vals[3] );
    case A_PARAM_WITH_DEFAULT_1:
        return _PyPegen_name_default_pair ( p , ((arg_ty)_f->vals[0]) , ((expr_ty)_f->vals[1]) , _f->vals[2] );
    case A_PARAM_MAYBE_DEFAULT_0:
        return _PyPegen_name_default_pair ( p , ((arg_ty)_f->vals[0]) , _f->vals[1] , _f->vals[3] );
    case A_PARAM_MAYBE_DEFAULT_1:
        return _PyPegen_name_default_pair ( p , ((arg_ty)_f->vals[0]) , _f->vals[1] , _f->vals[2] );
    case A_PARAM_0:
        return _Py_arg ( ((expr_ty)_f->vals[0]) -> v . Name . id , _f->vals[1] , NULL , EXTRA );
    case A_ANNOTATION_0:
    case A_DEFAULT_0:
    case A_SINGLE_TARGET_2:
    case A__TMP_14_0:
    case A__TMP_15_0:
    case A__TMP_16_0:
    case A__TMP_22_0:
    case A__TMP_26_0:
    case A__TMP_28_0:
    case A__TMP_33_0:
    case A__TMP_35_0:
    case A__TMP_36_0:
    case A__TMP_37_0:
    case A__TMP_38_0:
    case A__TMP_90_0:
    case A__TMP_91_0:
    case A__TMP_114_0:
    case A__TMP_115_0:
    case A__TMP_116_0:
    case A__TMP_117_0:
    case A__TMP_118_0:
    case A__TMP_119_0:
    case A__TMP_120_0:
    case A__TMP_121_0:
        return ((expr_ty)_f->vals[1]);
    case A_CLASS_DEF_0:
        return _PyPegen_class_def_decorators ( p , ((asdl_seq*)_f->vals[0]) , ((stmt_ty)_f->vals[1]) );
    case A_CLASS_DEF_RAW_0:
        return _Py_ClassDef ( ((expr_ty)_f->vals[1]) -> v . Name . id , ( _f->vals[2] )   ? ( ( expr_ty ) _f->vals[2] ) -> v . Call . args : NULL , ( _f->vals[2] )   ? ( ( expr_ty ) _f->vals[2] ) -> v . Call . keywords : NULL , ((asdl_seq*)_f->vals[4]) , NULL , EXTRA );
    case A_STAR_EXPRESSIONS_0:
    case A_EXPRESSIONS_0:
        return _Py_Tuple ( CHECK ( _PyPegen_seq_insert_in_front ( p , ((expr_ty)_f->vals[0]) , _f->vals[1] ) ) , Load , EXTRA );
    case A_STAR_EXPRESSIONS_1:
    case A_EXPRESSIONS_1:
        return _Py_Tuple ( CHECK ( _PyPegen_singleton_seq ( p , ((expr_ty)_f->vals[0]) ) ) , Load , EXTRA );
    case A_STAR_EXPRESSION_0:
    case A_STAR_NAMED_EXPRESSION_0:
    case A_STARRED_EXPRESSION_0:
        return _Py_Starred ( ((expr_ty)_f->vals[1]) , Load , EXTRA );
    case A_NAMED_EXPRESSION_0:
        return _Py_NamedExpr ( CHECK ( _PyPegen_set_expr_context ( p , ((expr_ty)_f->vals[0]) , Store ) ) , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_EXPRESSION_0:
        return _Py_IfExp ( ((expr_ty)_f->vals[2]) , ((expr_ty)_f->vals[0]) , ((expr_ty)_f->vals[4]) , EXTRA );
    case A_LAMBDEF_0:
        return _Py_Lambda ( ( _f->vals[1] )   ? _f->vals[1] : CHECK ( _PyPegen_empty_arguments ( p ) ) , ((expr_ty)_f->vals[3]) , EXTRA );
    case A_LAMBDA_PARAM_NO_DEFAULT_0:
    case A_LAMBDA_PARAM_NO_DEFAULT_1:
        return ((arg_ty)_f->vals[0]);
    case A_LAMBDA_PARAM_WITH_DEFAULT_0:
    case A_LAMBDA_PARAM_WITH_DEFAULT_1:
        return _PyPegen_name_default_pair ( p , ((arg_ty)_f->vals[0]) , ((expr_ty)_f->vals[1]) , NULL );
    case A_LAMBDA_PARAM_MAYBE_DEFAULT_0:
    case A_LAMBDA_PARAM_MAYBE_DEFAULT_1:
        return _PyPegen_name_default_pair ( p , ((arg_ty)_f->vals[0]) , _f->vals[1] , NULL );
    case A_LAMBDA_PARAM_0:
        return _Py_arg ( ((expr_ty)_f->vals[0]) -> v . Name . id , NULL , NULL , EXTRA );
    case A_DISJUNCTION_0:
        return _Py_BoolOp ( Or , CHECK ( _PyPegen_seq_insert_in_front ( p , ((expr_ty)_f->vals[0]) , _f->vals[1] ) ) , EXTRA );
    case A_CONJUNCTION_0:
        return _Py_BoolOp ( And , CHECK ( _PyPegen_seq_insert_in_front ( p , ((expr_ty)_f->vals[0]) , _f->vals[1] ) ) , EXTRA );
    case A_INVERSION_0:
        return _Py_UnaryOp ( Not , ((expr_ty)_f->vals[1]) , EXTRA );
    case A_COMPARISON_0:
        return _Py_Compare ( ((expr_ty)_f->vals[0]) , CHECK ( _PyPegen_get_cmpops ( p , _f->vals[1] ) ) , CHECK ( _PyPegen_get_exprs ( p , _f->vals[1] ) ) , EXTRA );
    case A_COMPARE_OP_BITWISE_OR_PAIR_0:
    case A_COMPARE_OP_BITWISE_OR_PAIR_1:
    case A_COMPARE_OP_BITWISE_OR_PAIR_2:
    case A_COMPARE_OP_BITWISE_OR_PAIR_3:
    case A_COMPARE_OP_BITWISE_OR_PAIR_4:
    case A_COMPARE_OP_BITWISE_OR_PAIR_5:
    case A_COMPARE_OP_BITWISE_OR_PAIR_6:
    case A_COMPARE_OP_BITWISE_OR_PAIR_7:
    case A_COMPARE_OP_BITWISE_OR_PAIR_8:
    case A_COMPARE_OP_BITWISE_OR_PAIR_9:
        return ((CmpopExprPair*)_f->vals[0]);
    case A_EQ_BITWISE_OR_0:
        return _PyPegen_cmpop_expr_pair ( p , Eq , ((expr_ty)_f->vals[1]) );
    case A_NOTEQ_BITWISE_OR_0:
        return _PyPegen_cmpop_expr_pair ( p , NotEq , ((expr_ty)_f->vals[1]) );
    case A_LTE_BITWISE_OR_0:
        return _PyPegen_cmpop_expr_pair ( p , LtE , ((expr_ty)_f->vals[1]) );
    case A_LT_BITWISE_OR_0:
        return _PyPegen_cmpop_expr_pair ( p , Lt , ((expr_ty)_f->vals[1]) );
    case A_GTE_BITWISE_OR_0:
        return _PyPegen_cmpop_expr_pair ( p , GtE , ((expr_ty)_f->vals[1]) );
    case A_GT_BITWISE_OR_0:
        return _PyPegen_cmpop_expr_pair ( p , Gt , ((expr_ty)_f->vals[1]) );
    case A_NOTIN_BITWISE_OR_0:
        return _PyPegen_cmpop_expr_pair ( p , NotIn , ((expr_ty)_f->vals[2]) );
    case A_IN_BITWISE_OR_0:
        return _PyPegen_cmpop_expr_pair ( p , In , ((expr_ty)_f->vals[1]) );
    case A_ISNOT_BITWISE_OR_0:
        return _PyPegen_cmpop_expr_pair ( p , IsNot , ((expr_ty)_f->vals[2]) );
    case A_IS_BITWISE_OR_0:
        return _PyPegen_cmpop_expr_pair ( p , Is , ((expr_ty)_f->vals[1]) );
    case A_BITWISE_OR_0:
        return _Py_BinOp ( ((expr_ty)_f->vals[0]) , BitOr , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_BITWISE_XOR_0:
        return _Py_BinOp ( ((expr_ty)_f->vals[0]) , BitXor , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_BITWISE_AND_0:
        return _Py_BinOp ( ((expr_ty)_f->vals[0]) , BitAnd , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_SHIFT_EXPR_0:
        return _Py_BinOp ( ((expr_ty)_f->vals[0]) , LShift , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_SHIFT_EXPR_1:
        return _Py_BinOp ( ((expr_ty)_f->vals[0]) , RShift , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_SUM_0:
        return _Py_BinOp ( ((expr_ty)_f->vals[0]) , Add , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_SUM_1:
        return _Py_BinOp ( ((expr_ty)_f->vals[0]) , Sub , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_TERM_0:
        return _Py_BinOp ( ((expr_ty)_f->vals[0]) , Mult , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_TERM_1:
        return _Py_BinOp ( ((expr_ty)_f->vals[0]) , Div , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_TERM_2:
        return _Py_BinOp ( ((expr_ty)_f->vals[0]) , FloorDiv , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_TERM_3:
        return _Py_BinOp ( ((expr_ty)_f->vals[0]) , Mod , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_TERM_4:
        return CHECK_VERSION ( 5 , "The '@' operator is" , _Py_BinOp ( ((expr_ty)_f->vals[0]) , MatMult , ((expr_ty)_f->vals[2]) , EXTRA ) );
    case A_FACTOR_0:
        return _Py_UnaryOp ( UAdd , ((expr_ty)_f->vals[1]) , EXTRA );
    case A_FACTOR_1:
        return _Py_UnaryOp ( USub , ((expr_ty)_f->vals[1]) , EXTRA );
    case A_FACTOR_2:
        return _Py_UnaryOp ( Invert , ((expr_ty)_f->vals[1]) , EXTRA );
    case A_POWER_0:
        return _Py_BinOp ( ((expr_ty)_f->vals[0]) , Pow , ((expr_ty)_f->vals[2]) , EXTRA );
    case A_AWAIT_PRIMARY_0:
        return CHECK_VERSION ( 5 , "Await expressions are" , _Py_Await ( ((expr_ty)_f->vals[1]) , EXTRA ) );
    case A_PRIMARY_0:
    case A_T_PRIMARY_0:
        return _Py_Attribute ( ((expr_ty)_f->vals[0]) , ((expr_ty)_f->vals[2]) -> v . Name . id , Load , EXTRA );
    case A_PRIMARY_1:
    case A_T_PRIMARY_2:
        return _Py_Call ( ((expr_ty)_f->vals[0]) , CHECK ( _PyPegen_singleton_seq ( p , ((expr_ty)_f->vals[1]) ) ) , NULL , EXTRA );
    case A_PRIMARY_2:
    case A_T_PRIMARY_3:
        return _Py_Call ( ((expr_ty)_f->vals[0]) , ( _f->vals[2] )   ? ( ( expr_ty ) _f->vals[2] ) -> v . Call . args : NULL , ( _f->vals[2] )   ? ( ( expr_ty ) _f->vals[2] ) -> v . Call . keywords : NULL , EXTRA );
    case A_PRIMARY_3:
    case A_T_PRIMARY_1:
        return _Py_Subscript ( ((expr_ty)_f->vals[0]) , ((expr_ty)_f->vals[2]) , Load , EXTRA );
    case A_SLICES_1:
        return _Py_Tuple ( _f->vals[0] , Load , EXTRA );
    case A_SLICE_0:
        return _Py_Slice ( _f->vals[0] , _f->vals[2] , _f->vals[3] , EXTRA );
    case A_ATOM_1:
        return _Py_Constant ( Py_True , NULL , EXTRA );
    case A_ATOM_2:
        return _Py_Constant ( Py_False , NULL , EXTRA );
    case A_ATOM_3:
        return _Py_Constant ( Py_None , NULL , EXTRA );
    case A_ATOM_4:
        return RAISE_SYNTAX_ERROR ( "You found it!" );
    case A_ATOM_10:
        return _Py_Constant ( Py_Ellipsis , NULL , EXTRA );
    case A_STRINGS_0:
        return _PyPegen_concatenate_strings ( p , _f->vals[0] );
    case A_LIST_0:
        return _Py_List ( _f->vals[1] , Load , EXTRA );
    case A_LISTCOMP_0:
        return _Py_ListComp ( ((expr_ty)_f->vals[1]) , ((asdl_seq*)_f->vals[2]) , EXTRA );
    case A_TUPLE_0:
        return _Py_Tuple ( _f->vals[1] , Load , EXTRA );
    case A_GENEXP_0:
        return _Py_GeneratorExp ( ((expr_ty)_f->vals[1]) , ((asdl_seq*)_f->vals[2]) , EXTRA );
    case A_SET_0:
        return _Py_Set ( ((asdl_seq*)_f->vals[1]) , EXTRA );
    case A_SETCOMP_0:
        return _Py_SetComp ( ((expr_ty)_f->vals[1]) , ((asdl_seq*)_f->vals[2]) , EXTRA );
    case A_DICT_0:
        return _Py_Dict ( CHECK ( _PyPegen_get_keys ( p , _f->vals[1] ) ) , CHECK ( _PyPegen_get_values ( p , _f->vals[1] ) ) , EXTRA );
    case A_DICTCOMP_0:
        return _Py_DictComp ( ((KeyValuePair*)_f->vals[1]) -> key , ((KeyValuePair*)_f->vals[1]) -> value , ((asdl_seq*)_f->vals[2]) , EXTRA );
    case A_DOUBLE_STARRED_KVPAIR_0:
        return _PyPegen_key_value_pair ( p , NULL , ((expr_ty)_f->vals[1]) );
    case A_DOUBLE_STARRED_KVPAIR_1:
    case A__GATHER_86_0:
    case A__GATHER_86_1:
        return ((KeyValuePair*)_f->vals[0]);
    case A_KVPAIR_0:
        return _PyPegen_key_value_pair ( p , ((expr_ty)_f->vals[0]) , ((expr_ty)_f->vals[2]) );
    case A_FOR_IF_CLAUSE_0:
        return CHECK_VERSION ( 6 , "Async comprehensions are" , _Py_comprehension ( ((expr_ty)_f->vals[2]) , ((expr_ty)_f->vals[4]) , _f->vals[5] , 1 , p -> arena ) );
    case A_FOR_IF_CLAUSE_1:
        return _Py_comprehension ( ((expr_ty)_f->vals[1]) , ((expr_ty)_f->vals[3]) , _f->vals[4] , 0 , p -> arena );
    case A_YIELD_EXPR_0:
        return _Py_YieldFrom ( ((expr_ty)_f->vals[2]) , EXTRA );
    case A_YIELD_EXPR_1:
        return _Py_Yield ( _f->vals[1] , EXTRA );
    case A_ARGS_0:
    case A_ARGS_2:
        return _Py_Call ( _PyPegen_dummy_name ( p ) , ( _f->vals[1] )   ? CHECK ( _PyPegen_seq_insert_in_front ( p , ((expr_ty)_f->vals[0]) , ( ( expr_ty ) _f->vals[1] ) -> v . Call . args ) ) : CHECK ( _PyPegen_singleton_seq ( p , ((expr_ty)_f->vals[0]) ) ) , ( _f->vals[1] )   ? ( ( expr_ty ) _f->vals[1] ) -> v . Call . keywords : NULL , EXTRA );
    case A_ARGS_1:
        return _Py_Call ( _PyPegen_dummy_name ( p ) , CHECK_NULL_ALLOWED ( _PyPegen_seq_extract_starred_exprs ( p , ((asdl_seq*)_f->vals[0]) ) ) , CHECK_NULL_ALLOWED ( _PyPegen_seq_delete_starred_exprs ( p , ((asdl_seq*)_f->vals[0]) ) ) , EXTRA );
    case A_KWARGS_0:
        return _PyPegen_join_sequences ( p , _f->vals[0] , _f->vals[2] );
    case A_KWARG_OR_STARRED_0:
    case A_KWARG_OR_DOUBLE_STARRED_0:
        return _PyPegen_keyword_or_starred ( p , CHECK ( _Py_keyword ( ((expr_ty)_f->vals[0]) -> v . Name . id , ((expr_ty)_f->vals[2]) , EXTRA ) ) , 1 );
    case A_KWARG_OR_STARRED_1:
        return _PyPegen_keyword_or_starred ( p , ((expr_ty)_f->vals[0]) , 0 );
    case A_KWARG_OR_DOUBLE_STARRED_1:
        return _PyPegen_keyword_or_starred ( p , CHECK ( _Py_keyword ( NULL , ((expr_ty)_f->vals[1]) , EXTRA ) ) , 1 );
    case A_STAR_TARGETS_1:
        return _Py_Tuple ( CHECK ( _PyPegen_seq_insert_in_front ( p , ((expr_ty)_f->vals[0]) , _f->vals[1] ) ) , Store , EXTRA );
    case A_STAR_TARGET_0:
        return _Py_Starred ( CHECK ( _PyPegen_set_expr_context ( p , _f->vals[1] , Store ) ) , Store , EXTRA );
    case A_STAR_TARGET_1:
    case A_SINGLE_SUBSCRIPT_ATTRIBUTE_TARGET_0:
    case A_TARGET_0:
        return _Py_Attribute ( ((expr_ty)_f->vals[0]) , ((expr_ty)_f->vals[2]) -> v . Name . id , Store , EXTRA );
    case A_STAR_TARGET_2:
    case A_SINGLE_SUBSCRIPT_ATTRIBUTE_TARGET_1:
    case A_TARGET_1:
        return _Py_Subscript ( ((expr_ty)_f->vals[0]) , ((expr_ty)_f->vals[2]) , Store , EXTRA );
    case A_STAR_ATOM_0:
    case A_SINGLE_TARGET_1:
    case A_T_ATOM_0:
        return _PyPegen_set_expr_context ( p , ((expr_ty)_f->vals[0]) , Store );
    case A_STAR_ATOM_1:
    case A_T_ATOM_1:
        return _PyPegen_set_expr_context ( p , ((expr_ty)_f->vals[1]) , Store );
    case A_STAR_ATOM_2:
    case A_T_ATOM_2:
        return _Py_Tuple ( _f->vals[1] , Store , EXTRA );
    case A_STAR_ATOM_3:
    case A_T_ATOM_3:
        return _Py_List ( _f->vals[1] , Store , EXTRA );
    case A_DEL_TARGET_0:
        return _Py_Attribute ( ((expr_ty)_f->vals[0]) , ((expr_ty)_f->vals[2]) -> v . Name . id , Del , EXTRA );
    case A_DEL_TARGET_1:
        return _Py_Subscript ( ((expr_ty)_f->vals[0]) , ((expr_ty)_f->vals[2]) , Del , EXTRA );
    case A_DEL_T_ATOM_0:
        return _PyPegen_set_expr_context ( p , ((expr_ty)_f->vals[0]) , Del );
    case A_DEL_T_ATOM_1:
        return _PyPegen_set_expr_context ( p , ((expr_ty)_f->vals[1]) , Del );
    case A_DEL_T_ATOM_2:
        return _Py_Tuple ( _f->vals[1] , Del , EXTRA );
    case A_DEL_T_ATOM_3:
        return _Py_List ( _f->vals[1] , Del , EXTRA );
    case A_DEL_TARGET_END_0:
    case A_DEL_TARGET_END_1:
    case A_DEL_TARGET_END_2:
    case A_DEL_TARGET_END_3:
    case A_T_LOOKAHEAD_0:
    case A_T_LOOKAHEAD_1:
    case A_T_LOOKAHEAD_2:
    case A__TMP_9_0:
    case A__TMP_9_1:
    case A__TMP_10_0:
    case A__TMP_10_1:
    case A__TMP_11_0:
    case A__TMP_11_1:
    case A__TMP_12_0:
    case A__TMP_13_0:
    case A__TMP_103_0:
    case A__TMP_106_0:
    case A__TMP_106_1:
    case A__TMP_106_2:
    case A__TMP_109_0:
    case A__TMP_109_1:
    case A__TMP_110_0:
    case A__TMP_110_1:
    case A__TMP_112_0:
    case A__TMP_112_1:
    case A__TMP_113_0:
    case A__TMP_113_1:
    case A__TMP_123_0:
    case A__TMP_123_1:
    case A__TMP_124_0:
    case A__TMP_124_1:
        return ((Token *)_f->vals[0]);
    case A_INCORRECT_ARGUMENTS_0:
        return RAISE_SYNTAX_ERROR ( "iterable argument unpacking follows keyword argument unpacking" );
    case A_INCORRECT_ARGUMENTS_1:
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION ( ((expr_ty)_f->vals[0]) , "Generator expression must be parenthesized" );
    case A_INCORRECT_ARGUMENTS_2:
        return _PyPegen_nonparen_genexp_in_call ( p , ((expr_ty)_f->vals[0]) );
    case A_INCORRECT_ARGUMENTS_3:
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION ( ((expr_ty)_f->vals[2]) , "Generator expression must be parenthesized" );
    case A_INCORRECT_ARGUMENTS_4:
        return _PyPegen_arguments_parsing_error ( p , ((expr_ty)_f->vals[0]) );
    case A_INVALID_KWARG_0:
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION ( ((expr_ty)_f->vals[0]) , "expression cannot contain assignment, perhaps you meant \"==\"?" );
    case A_INVALID_NAMED_EXPRESSION_0:
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION ( ((expr_ty)_f->vals[0]) , "cannot use assignment expressions with %s" , _PyPegen_get_expr_name ( ((expr_ty)_f->vals[0]) ) );
    case A_INVALID_ASSIGNMENT_0:
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION ( ((expr_ty)_f->vals[0]) , "only single target (not list) can be annotated" );
    case A_INVALID_ASSIGNMENT_1:
    case A_INVALID_ASSIGNMENT_2:
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION ( ((expr_ty)_f->vals[0]) , "only single target (not tuple) can be annotated" );
    case A_INVALID_ASSIGNMENT_3:
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION ( ((expr_ty)_f->vals[0]) , "illegal target for annotation" );
    case A_INVALID_ASSIGNMENT_4:
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION ( _PyPegen_get_invalid_target ( ((expr_ty)_f->vals[0]) ) , "cannot assign to %s" , _PyPegen_get_expr_name ( _PyPegen_get_invalid_target ( ((expr_ty)_f->vals[0]) ) ) );
    case A_INVALID_ASSIGNMENT_5:
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION ( ((expr_ty)_f->vals[0]) , "'%s' is an illegal expression for augmented assignment" , _PyPegen_get_expr_name ( ((expr_ty)_f->vals[0]) ) );
    case A_INVALID_BLOCK_0:
        return RAISE_INDENTATION_ERROR ( "expected an indented block" );
    case A_INVALID_COMPREHENSION_0:
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION ( ((expr_ty)_f->vals[1]) , "iterable unpacking cannot be used in comprehension" );
    case A_INVALID_DICT_COMPREHENSION_0:
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION ( ((Token *)_f->vals[1]) , "dict unpacking cannot be used in dict comprehension" );
    case A_INVALID_PARAMETERS_0:
        return RAISE_SYNTAX_ERROR ( "non-default argument follows default argument" );
    case A_INVALID_STAR_ETC_0:
    case A_INVALID_LAMBDA_STAR_ETC_0:
        return RAISE_SYNTAX_ERROR ( "named arguments must follow bare *" );
    case A_INVALID_STAR_ETC_1:
        return RAISE_SYNTAX_ERROR ( "bare * has associated type comment" );
    case A_INVALID_DOUBLE_TYPE_COMMENTS_0:
        return RAISE_SYNTAX_ERROR ( "Cannot have two type comments on def" );
    case A_INVALID_DEL_TARGET_0:
        return RAISE_SYNTAX_ERROR_KNOWN_LOCATION ( ((expr_ty)_f->vals[0]) , "cannot delete %s" , _PyPegen_get_expr_name ( ((expr_ty)_f->vals[0]) ) );
    case A_INVALID_IMPORT_FROM_TARGETS_0:
        return RAISE_SYNTAX_ERROR ( "trailing comma not allowed without surrounding parentheses" );
    case A__GATHER_25_0:
    case A__GATHER_25_1:
    case A__GATHER_27_0:
    case A__GATHER_27_1:
        return ((alias_ty)_f->vals[0]);
    case A__GATHER_29_0:
    case A__GATHER_29_1:
    case A__GATHER_30_0:
    case A__GATHER_30_1:
    case A__GATHER_31_0:
    case A__GATHER_31_1:
    case A__GATHER_32_0:
    case A__GATHER_32_1:
        return ((withitem_ty)_f->vals[0]);
    case A__TMP_77_0:
        return _PyPegen_check_barry_as_flufl ( p )   ? NULL : ((Token *)_f->vals[0]);
    case A__TMP_84_0:
        return _PyPegen_seq_insert_in_front ( p , ((expr_ty)_f->vals[0]) , _f->vals[2] );
    case A__GATHER_92_0:
    case A__GATHER_92_1:
    case A__GATHER_93_0:
    case A__GATHER_93_1:
    case A__GATHER_94_0:
    case A__GATHER_94_1:
    case A__GATHER_95_0:
    case A__GATHER_95_1:
        return ((KeywordOrStarred*)_f->vals[0]);
    case A__TMP_108_0:
        return ((SlashWithDefault*)_f->vals[0]);
    default:
        assert(0);
        RAISE_SYNTAX_ERROR("invalid opcode");
        return NULL;
    }
}
