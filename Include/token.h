#ifndef Py_TOKEN_H
#define Py_TOKEN_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* Token types */

#define ENDMARKER	0
#define NAME		1
#define NUMBER		2
#define STRING		3
#define NEWLINE		4
#define INDENT		5
#define DEDENT		6
#define LPAR		7
#define RPAR		8
#define LSQB		9
#define RSQB		10
#define COLON		11
#define COMMA		12
#define SEMI		13
#define PLUS		14
#define MINUS		15
#define STAR		16
#define SLASH		17
#define VBAR		18
#define AMPER		19
#define LESS		20
#define GREATER		21
#define EQUAL		22
#define DOT		23
#define PERCENT		24
#define BACKQUOTE	25
#define LBRACE		26
#define RBRACE		27
#define EQEQUAL		28
#define NOTEQUAL	29
#define LESSEQUAL	30
#define GREATEREQUAL	31
#define TILDE		32
#define CIRCUMFLEX	33
#define LEFTSHIFT	34
#define RIGHTSHIFT	35
#define DOUBLESTAR	36
/* Don't forget to update the table _PyParser_TokenNames in tokenizer.c! */
#define OP		37
#define ERRORTOKEN	38
#define N_TOKENS	39

/* Special definitions for cooperation with parser */

#define NT_OFFSET		256

#define ISTERMINAL(x)		((x) < NT_OFFSET)
#define ISNONTERMINAL(x)	((x) >= NT_OFFSET)
#define ISEOF(x)		((x) == ENDMARKER)


extern DL_IMPORT(char *) _PyParser_TokenNames[]; /* Token names */
extern DL_IMPORT(int) PyToken_OneChar Py_PROTO((int));
extern DL_IMPORT(int) PyToken_TwoChars Py_PROTO((int, int));

#ifdef __cplusplus
}
#endif
#endif /* !Py_TOKEN_H */
