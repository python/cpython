/*
 * Minimal set of shared macro definitions and declarations so that multiple
 * source files can make use of the parsing table in tclParse.c
 */

#define TYPE_NORMAL		0
#define TYPE_SPACE		0x1
#define TYPE_COMMAND_END	0x2
#define TYPE_SUBS		0x4
#define TYPE_QUOTE		0x8
#define TYPE_CLOSE_PAREN	0x10
#define TYPE_CLOSE_BRACK	0x20
#define TYPE_BRACE		0x40

#define CHAR_TYPE(c) (tclCharTypeTable+128)[(int)(c)]

MODULE_SCOPE const char tclCharTypeTable[];
