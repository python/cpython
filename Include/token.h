
/* Token types */
#ifndef Py_LIMITED_API
#ifndef Py_TOKEN_H
#define Py_TOKEN_H
#ifdef __cplusplus
extern "C" {
#endif

#undef TILDE   /* Prevent clash of our definition with system macro. Ex AIX, ioctl.h */

#define ENDMARKER       0
#define NAME            1
#define NUMBER          2
#define STRING          3
#define NEWLINE         4
#define INDENT          5
#define DEDENT          6
#define LPAR            7
#define RPAR            8
#define LSQB            9
#define RSQB            10
#define COLON           11
#define COMMA           12
#define SEMI            13
#define PLUS            14
#define MINUS           15
#define STAR            16
#define SLASH           17
#define VBAR            18
#define AMPER           19
#define LESS            20
#define GREATER         21
#define EQUAL           22
#define DOT             23
#define PERCENT         24
#define LBRACE          25
#define RBRACE          26
#define EQEQUAL         27
#define NOTEQUAL        28
#define LESSEQUAL       29
#define GREATEREQUAL    30
#define TILDE           31
#define CIRCUMFLEX      32
#define LEFTSHIFT       33
#define RIGHTSHIFT      34
#define DOUBLESTAR      35
#define PLUSEQUAL       36
#define MINEQUAL        37
#define STAREQUAL       38
#define SLASHEQUAL      39
#define PERCENTEQUAL    40
#define AMPEREQUAL      41
#define VBAREQUAL       42
#define CIRCUMFLEXEQUAL 43
#define LEFTSHIFTEQUAL  44
#define RIGHTSHIFTEQUAL 45
#define DOUBLESTAREQUAL 46
#define DOUBLESLASH     47
#define DOUBLESLASHEQUAL 48
#define AT              49
#define ATEQUAL         50
#define RARROW          51
#define ELLIPSIS        52
/* Don't forget to update the table _PyParser_TokenNames in tokenizer.c! */
#define OP              53
#define ERRORTOKEN      54
/* These aren't used by the C tokenizer but are needed for tokenize.py */
#define COMMENT         55
#define NL              56
#define ENCODING                57
#define N_TOKENS        58

/* Special definitions for cooperation with parser */

#define NT_OFFSET               256

#define ISTERMINAL(x)           ((x) < NT_OFFSET)
#define ISNONTERMINAL(x)        ((x) >= NT_OFFSET)
#define ISEOF(x)                ((x) == ENDMARKER)


PyAPI_DATA(const char *) _PyParser_TokenNames[]; /* Token names */
PyAPI_FUNC(int) PyToken_OneChar(int);
PyAPI_FUNC(int) PyToken_TwoChars(int, int);
PyAPI_FUNC(int) PyToken_ThreeChars(int, int, int);

#ifdef __cplusplus
}
#endif
#endif /* !Py_TOKEN_H */
#endif /* Py_LIMITED_API */
