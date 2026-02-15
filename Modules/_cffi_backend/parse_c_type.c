#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define _CFFI_INTERNAL
#include "parse_c_type.h"


enum token_e {
    TOK_STAR='*',
    TOK_OPEN_PAREN='(',
    TOK_CLOSE_PAREN=')',
    TOK_OPEN_BRACKET='[',
    TOK_CLOSE_BRACKET=']',
    TOK_COMMA=',',

    TOK_START=256,
    TOK_END,
    TOK_ERROR,
    TOK_IDENTIFIER,
    TOK_INTEGER,
    TOK_DOTDOTDOT,

    /* keywords */
    TOK__BOOL,
    TOK_CHAR,
    TOK__COMPLEX,
    TOK_CONST,
    TOK_DOUBLE,
    TOK_ENUM,
    TOK_FLOAT,
    //TOK__IMAGINARY,
    TOK_INT,
    TOK_LONG,
    TOK_SHORT,
    TOK_SIGNED,
    TOK_STRUCT,
    TOK_UNION,
    TOK_UNSIGNED,
    TOK_VOID,
    TOK_VOLATILE,

    TOK_CDECL,
    TOK_STDCALL,
};

typedef struct {
    struct _cffi_parse_info_s *info;
    const char *input, *p;
    size_t size;              // the next token is at 'p' and of length 'size'
    enum token_e kind;
    _cffi_opcode_t *output;
    size_t output_index;
} token_t;

static int is_space(char x)
{
    return (x == ' ' || x == '\f' || x == '\n' || x == '\r' ||
            x == '\t' || x == '\v');
}

static int is_ident_first(char x)
{
    return (('A' <= x && x <= 'Z') || ('a' <= x && x <= 'z') || x == '_' ||
            x == '$');   /* '$' in names is supported here, for the struct
                            names invented by cparser */
}

static int is_digit(char x)
{
    return ('0' <= x && x <= '9');
}

static int is_hex_digit(char x)
{
    return (('0' <= x && x <= '9') ||
            ('A' <= x && x <= 'F') ||
            ('a' <= x && x <= 'f'));
}

static int is_ident_next(char x)
{
    return (is_ident_first(x) || is_digit(x));
}

static char get_following_char(token_t *tok)
{
    const char *p = tok->p + tok->size;
    if (tok->kind == TOK_ERROR)
        return 0;
    while (is_space(*p))
        p++;
    return *p;
}

static int number_of_commas(token_t *tok)
{
    const char *p = tok->p;
    int result = 0;
    int nesting = 0;
    while (1) {
        switch (*p++) {
        case ',': result += !nesting; break;
        case '(': nesting++; break;
        case ')': if ((--nesting) < 0) return result; break;
        case 0:   return result;
        default:  break;
        }
    }
}

static void next_token(token_t *tok)
{
    const char *p = tok->p + tok->size;
    if (tok->kind == TOK_ERROR)
        return;
    while (!is_ident_first(*p)) {
        if (is_space(*p)) {
            p++;
        }
        else if (is_digit(*p)) {
            tok->kind = TOK_INTEGER;
            tok->p = p;
            tok->size = 1;
            if (p[1] == 'x' || p[1] == 'X')
                tok->size = 2;
            while (is_hex_digit(p[tok->size]))
                tok->size++;
            return;
        }
        else if (p[0] == '.' && p[1] == '.' && p[2] == '.') {
            tok->kind = TOK_DOTDOTDOT;
            tok->p = p;
            tok->size = 3;
            return;
        }
        else if (*p) {
            tok->kind = *p;
            tok->p = p;
            tok->size = 1;
            return;
        }
        else {
            tok->kind = TOK_END;
            tok->p = p;
            tok->size = 0;
            return;
        }
    }
    tok->kind = TOK_IDENTIFIER;
    tok->p = p;
    tok->size = 1;
    while (is_ident_next(p[tok->size]))
        tok->size++;

    switch (*p) {
    case '_':
        if (tok->size == 5 && !memcmp(p, "_Bool", 5))  tok->kind = TOK__BOOL;
        if (tok->size == 7 && !memcmp(p,"__cdecl",7))  tok->kind = TOK_CDECL;
        if (tok->size == 9 && !memcmp(p,"__stdcall",9))tok->kind = TOK_STDCALL;
        if (tok->size == 8 && !memcmp(p,"_Complex",8)) tok->kind = TOK__COMPLEX;
        break;
    case 'c':
        if (tok->size == 4 && !memcmp(p, "char", 4))   tok->kind = TOK_CHAR;
        if (tok->size == 5 && !memcmp(p, "const", 5))  tok->kind = TOK_CONST;
        break;
    case 'd':
        if (tok->size == 6 && !memcmp(p, "double", 6)) tok->kind = TOK_DOUBLE;
        break;
    case 'e':
        if (tok->size == 4 && !memcmp(p, "enum", 4))   tok->kind = TOK_ENUM;
        break;
    case 'f':
        if (tok->size == 5 && !memcmp(p, "float", 5))  tok->kind = TOK_FLOAT;
        break;
    case 'i':
        if (tok->size == 3 && !memcmp(p, "int", 3))    tok->kind = TOK_INT;
        break;
    case 'l':
        if (tok->size == 4 && !memcmp(p, "long", 4))   tok->kind = TOK_LONG;
        break;
    case 's':
        if (tok->size == 5 && !memcmp(p, "short", 5))  tok->kind = TOK_SHORT;
        if (tok->size == 6 && !memcmp(p, "signed", 6)) tok->kind = TOK_SIGNED;
        if (tok->size == 6 && !memcmp(p, "struct", 6)) tok->kind = TOK_STRUCT;
        break;
    case 'u':
        if (tok->size == 5 && !memcmp(p, "union", 5))  tok->kind = TOK_UNION;
        if (tok->size == 8 && !memcmp(p,"unsigned",8)) tok->kind = TOK_UNSIGNED;
        break;
    case 'v':
        if (tok->size == 4 && !memcmp(p, "void", 4))   tok->kind = TOK_VOID;
        if (tok->size == 8 && !memcmp(p,"volatile",8)) tok->kind = TOK_VOLATILE;
        break;
    }
}

static int parse_error(token_t *tok, const char *msg)
{
    if (tok->kind != TOK_ERROR) {
        tok->kind = TOK_ERROR;
        tok->info->error_location = tok->p - tok->input;
        tok->info->error_message = msg;
    }
    return -1;
}

static int write_ds(token_t *tok, _cffi_opcode_t ds)
{
    size_t index = tok->output_index;
    if (index >= tok->info->output_size) {
        parse_error(tok, "internal type complexity limit reached");
        return -1;
    }
    tok->output[index] = ds;
    tok->output_index = index + 1;
    return index;
}

#define MAX_SSIZE_T  (((size_t)-1) >> 1)

static int parse_complete(token_t *tok);
static const char *get_common_type(const char *search, size_t search_len);
static int parse_common_type_replacement(token_t *tok, const char *replacement);

static int parse_sequel(token_t *tok, int outer)
{
    /* Emit opcodes for the "sequel", which is the optional part of a
       type declaration that follows the type name, i.e. everything
       with '*', '[ ]', '( )'.  Returns the entry point index pointing
       the innermost opcode (the one that corresponds to the complete
       type).  The 'outer' argument is the index of the opcode outside
       this "sequel".
     */
    int check_for_grouping, abi=0;
    _cffi_opcode_t result, *p_current;

 header:
    switch (tok->kind) {
    case TOK_STAR:
        outer = write_ds(tok, _CFFI_OP(_CFFI_OP_POINTER, outer));
        next_token(tok);
        goto header;
    case TOK_CONST:
        /* ignored for now */
        next_token(tok);
        goto header;
    case TOK_VOLATILE:
        /* ignored for now */
        next_token(tok);
        goto header;
    case TOK_CDECL:
    case TOK_STDCALL:
        /* must be in a function; checked below */
        abi = tok->kind;
        next_token(tok);
        goto header;
    default:
        break;
    }

    check_for_grouping = 1;
    if (tok->kind == TOK_IDENTIFIER) {
        next_token(tok);    /* skip a potential variable name */
        check_for_grouping = 0;
    }

    result = 0;
    p_current = &result;

    while (tok->kind == TOK_OPEN_PAREN) {
        next_token(tok);

        if (tok->kind == TOK_CDECL || tok->kind == TOK_STDCALL) {
            abi = tok->kind;
            next_token(tok);
        }

        if ((check_for_grouping--) == 1 && (tok->kind == TOK_STAR ||
                                            tok->kind == TOK_CONST ||
                                            tok->kind == TOK_VOLATILE ||
                                            tok->kind == TOK_OPEN_BRACKET)) {
            /* just parentheses for grouping.  Use a OP_NOOP to simplify */
            int x;
            assert(p_current == &result);
            x = tok->output_index;
            p_current = tok->output + x;

            write_ds(tok, _CFFI_OP(_CFFI_OP_NOOP, 0));

            x = parse_sequel(tok, x);
            result = _CFFI_OP(_CFFI_GETOP(0), x);
        }
        else {
            /* function type */
            int arg_total, base_index, arg_next, flags=0;

            if (abi == TOK_STDCALL) {
                flags = 2;
                /* note that an ellipsis below will overwrite this flags,
                   which is the goal: variadic functions are always cdecl */
            }
            abi = 0;

            if (tok->kind == TOK_VOID && get_following_char(tok) == ')') {
                next_token(tok);
            }

            /* (over-)estimate 'arg_total'.  May return 1 when it is really 0 */
            arg_total = number_of_commas(tok) + 1;

            *p_current = _CFFI_OP(_CFFI_GETOP(*p_current), tok->output_index);
            p_current = tok->output + tok->output_index;

            base_index = write_ds(tok, _CFFI_OP(_CFFI_OP_FUNCTION, 0));
            if (base_index < 0)
                return -1;
            /* reserve (arg_total + 1) slots for the arguments and the
               final FUNCTION_END */
            for (arg_next = 0; arg_next <= arg_total; arg_next++)
                if (write_ds(tok, _CFFI_OP(0, 0)) < 0)
                    return -1;

            arg_next = base_index + 1;

            if (tok->kind != TOK_CLOSE_PAREN) {
                while (1) {
                    int arg;
                    _cffi_opcode_t oarg;

                    if (tok->kind == TOK_DOTDOTDOT) {
                        flags = 1;   /* ellipsis */
                        next_token(tok);
                        break;
                    }
                    arg = parse_complete(tok);
                    switch (_CFFI_GETOP(tok->output[arg])) {
                    case _CFFI_OP_ARRAY:
                    case _CFFI_OP_OPEN_ARRAY:
                        arg = _CFFI_GETARG(tok->output[arg]);
                        /* fall-through */
                    case _CFFI_OP_FUNCTION:
                        oarg = _CFFI_OP(_CFFI_OP_POINTER, arg);
                        break;
                    default:
                        oarg = _CFFI_OP(_CFFI_OP_NOOP, arg);
                        break;
                    }
                    assert(arg_next - base_index <= arg_total);
                    tok->output[arg_next++] = oarg;
                    if (tok->kind != TOK_COMMA)
                        break;
                    next_token(tok);
                }
            }
            tok->output[arg_next] = _CFFI_OP(_CFFI_OP_FUNCTION_END, flags);
        }

        if (tok->kind != TOK_CLOSE_PAREN)
            return parse_error(tok, "expected ')'");
        next_token(tok);
    }

    if (abi != 0)
        return parse_error(tok, "expected '('");

    while (tok->kind == TOK_OPEN_BRACKET) {
        *p_current = _CFFI_OP(_CFFI_GETOP(*p_current), tok->output_index);
        p_current = tok->output + tok->output_index;

        next_token(tok);
        if (tok->kind != TOK_CLOSE_BRACKET) {
            size_t length;
            int gindex;
            char *endptr;

            switch (tok->kind) {

            case TOK_INTEGER:
                errno = 0;
                if (sizeof(length) > sizeof(unsigned long)) {
#ifdef MS_WIN32
# ifdef _WIN64
                    length = _strtoui64(tok->p, &endptr, 0);
# else
                    abort();  /* unreachable */
# endif
#else
                    length = strtoull(tok->p, &endptr, 0);
#endif
                }
                else
                    length = strtoul(tok->p, &endptr, 0);
                if (endptr != tok->p + tok->size)
                    return parse_error(tok, "invalid number");
                if (errno == ERANGE || length > MAX_SSIZE_T)
                    return parse_error(tok, "number too large");
                break;

            case TOK_IDENTIFIER:
                gindex = search_in_globals(tok->info->ctx, tok->p, tok->size);
                if (gindex >= 0) {
                    const struct _cffi_global_s *g;
                    g = &tok->info->ctx->globals[gindex];
                    if (_CFFI_GETOP(g->type_op) == _CFFI_OP_CONSTANT_INT ||
                        _CFFI_GETOP(g->type_op) == _CFFI_OP_ENUM) {
                        int neg;
                        struct _cffi_getconst_s gc;
                        gc.ctx = tok->info->ctx;
                        gc.gindex = gindex;
                        neg = ((int(*)(struct _cffi_getconst_s*))g->address)
                            (&gc);
                        if (neg == 0 && gc.value > MAX_SSIZE_T)
                            return parse_error(tok,
                                               "integer constant too large");
                        if (neg == 0 || gc.value == 0) {
                            length = (size_t)gc.value;
                            break;
                        }
                        if (neg != 1)
                            return parse_error(tok, "disagreement about"
                                               " this constant's value");
                    }
                }
                /* fall-through to the default case */
            default:
                return parse_error(tok, "expected a positive integer constant");
            }

            next_token(tok);

            write_ds(tok, _CFFI_OP(_CFFI_OP_ARRAY, 0));
            write_ds(tok, (_cffi_opcode_t)length);
        }
        else
            write_ds(tok, _CFFI_OP(_CFFI_OP_OPEN_ARRAY, 0));

        if (tok->kind != TOK_CLOSE_BRACKET)
            return parse_error(tok, "expected ']'");
        next_token(tok);
    }

    *p_current = _CFFI_OP(_CFFI_GETOP(*p_current), outer);
    return _CFFI_GETARG(result);
}

static int search_sorted(const char *const *base,
                         size_t item_size, int array_len,
                         const char *search, size_t search_len)
{
    int left = 0, right = array_len;
    const char *baseptr = (const char *)base;

    while (left < right) {
        int middle = (left + right) / 2;
        const char *src = *(const char *const *)(baseptr + middle * item_size);
        int diff = strncmp(src, search, search_len);
        if (diff == 0 && src[search_len] == '\0')
            return middle;
        else if (diff >= 0)
            right = middle;
        else
            left = middle + 1;
    }
    return -1;
}

#define MAKE_SEARCH_FUNC(FIELD)                                         \
  static                                                                \
  int search_in_##FIELD(const struct _cffi_type_context_s *ctx,         \
                        const char *search, size_t search_len)          \
  {                                                                     \
      return search_sorted(&ctx->FIELD->name, sizeof(*ctx->FIELD),      \
                           ctx->num_##FIELD, search, search_len);       \
  }

MAKE_SEARCH_FUNC(globals)
MAKE_SEARCH_FUNC(struct_unions)
MAKE_SEARCH_FUNC(typenames)
MAKE_SEARCH_FUNC(enums)

#undef MAKE_SEARCH_FUNC


static
int search_standard_typename(const char *p, size_t size)
{
    if (size < 6 || p[size-2] != '_' || p[size-1] != 't')
        return -1;

    switch (p[4]) {

    case '1':
        if (size == 8 && !memcmp(p, "uint16", 6)) return _CFFI_PRIM_UINT16;
        if (size == 8 && !memcmp(p, "char16", 6)) return _CFFI_PRIM_CHAR16;
        break;

    case '2':
        if (size == 7 && !memcmp(p, "int32", 5)) return _CFFI_PRIM_INT32;
        break;

    case '3':
        if (size == 8 && !memcmp(p, "uint32", 6)) return _CFFI_PRIM_UINT32;
        if (size == 8 && !memcmp(p, "char32", 6)) return _CFFI_PRIM_CHAR32;
        break;

    case '4':
        if (size == 7 && !memcmp(p, "int64", 5)) return _CFFI_PRIM_INT64;
        break;

    case '6':
        if (size == 8 && !memcmp(p, "uint64", 6)) return _CFFI_PRIM_UINT64;
        if (size == 7 && !memcmp(p, "int16", 5)) return _CFFI_PRIM_INT16;
        break;

    case '8':
        if (size == 7 && !memcmp(p, "uint8", 5)) return _CFFI_PRIM_UINT8;
        break;

    case 'a':
        if (size == 8 && !memcmp(p, "intmax", 6)) return _CFFI_PRIM_INTMAX;
        break;

    case 'e':
        if (size == 7 && !memcmp(p, "ssize", 5)) return _CFFI_PRIM_SSIZE;
        break;

    case 'f':
        if (size == 11 && !memcmp(p, "int_fast8",   9)) return _CFFI_PRIM_INT_FAST8;
        if (size == 12 && !memcmp(p, "int_fast16", 10)) return _CFFI_PRIM_INT_FAST16;
        if (size == 12 && !memcmp(p, "int_fast32", 10)) return _CFFI_PRIM_INT_FAST32;
        if (size == 12 && !memcmp(p, "int_fast64", 10)) return _CFFI_PRIM_INT_FAST64;
        break;

    case 'i':
        if (size == 9 && !memcmp(p, "ptrdiff", 7)) return _CFFI_PRIM_PTRDIFF;
        if (size == 21 && !memcmp(p, "_cffi_float_complex", 19)) return _CFFI_PRIM_FLOATCOMPLEX;
        if (size == 22 && !memcmp(p, "_cffi_double_complex", 20)) return _CFFI_PRIM_DOUBLECOMPLEX;
        break;

    case 'l':
        if (size == 12 && !memcmp(p, "int_least8",  10)) return _CFFI_PRIM_INT_LEAST8;
        if (size == 13 && !memcmp(p, "int_least16", 11)) return _CFFI_PRIM_INT_LEAST16;
        if (size == 13 && !memcmp(p, "int_least32", 11)) return _CFFI_PRIM_INT_LEAST32;
        if (size == 13 && !memcmp(p, "int_least64", 11)) return _CFFI_PRIM_INT_LEAST64;
        break;

    case 'm':
        if (size == 9 && !memcmp(p, "uintmax", 7)) return _CFFI_PRIM_UINTMAX;
        break;

    case 'p':
        if (size == 9 && !memcmp(p, "uintptr", 7)) return _CFFI_PRIM_UINTPTR;
        break;

    case 'r':
        if (size == 7 && !memcmp(p, "wchar", 5)) return _CFFI_PRIM_WCHAR;
        break;

    case 't':
        if (size == 8 && !memcmp(p, "intptr", 6)) return _CFFI_PRIM_INTPTR;
        break;

    case '_':
        if (size == 6 && !memcmp(p, "size", 4)) return _CFFI_PRIM_SIZE;
        if (size == 6 && !memcmp(p, "int8", 4)) return _CFFI_PRIM_INT8;
        if (size >= 12) {
            switch (p[10]) {
            case '1':
                if (size == 14 && !memcmp(p, "uint_least16", 12)) return _CFFI_PRIM_UINT_LEAST16;
                break;
            case '2':
                if (size == 13 && !memcmp(p, "uint_fast32", 11)) return _CFFI_PRIM_UINT_FAST32;
                break;
            case '3':
                if (size == 14 && !memcmp(p, "uint_least32", 12)) return _CFFI_PRIM_UINT_LEAST32;
                break;
            case '4':
                if (size == 13 && !memcmp(p, "uint_fast64", 11)) return _CFFI_PRIM_UINT_FAST64;
                break;
            case '6':
                if (size == 14 && !memcmp(p, "uint_least64", 12)) return _CFFI_PRIM_UINT_LEAST64;
                if (size == 13 && !memcmp(p, "uint_fast16", 11)) return _CFFI_PRIM_UINT_FAST16;
                break;
            case '8':
                if (size == 13 && !memcmp(p, "uint_least8", 11)) return _CFFI_PRIM_UINT_LEAST8;
                break;
            case '_':
                if (size == 12 && !memcmp(p, "uint_fast8", 10)) return _CFFI_PRIM_UINT_FAST8;
                break;
            default:
                break;
            }
        }
        break;

    default:
        break;
    }
    return -1;
}


static int parse_complete(token_t *tok)
{
    unsigned int t0;
    _cffi_opcode_t t1;
    _cffi_opcode_t t1complex;
    int modifiers_length, modifiers_sign;

 qualifiers:
    switch (tok->kind) {
    case TOK_CONST:
        /* ignored for now */
        next_token(tok);
        goto qualifiers;
    case TOK_VOLATILE:
        /* ignored for now */
        next_token(tok);
        goto qualifiers;
    default:
        ;
    }

    modifiers_length = 0;
    modifiers_sign = 0;
 modifiers:
    switch (tok->kind) {

    case TOK_SHORT:
        if (modifiers_length != 0)
            return parse_error(tok, "'short' after another 'short' or 'long'");
        modifiers_length--;
        next_token(tok);
        goto modifiers;

    case TOK_LONG:
        if (modifiers_length < 0)
            return parse_error(tok, "'long' after 'short'");
        if (modifiers_length >= 2)
            return parse_error(tok, "'long long long' is too long");
        modifiers_length++;
        next_token(tok);
        goto modifiers;

    case TOK_SIGNED:
        if (modifiers_sign)
            return parse_error(tok, "multiple 'signed' or 'unsigned'");
        modifiers_sign++;
        next_token(tok);
        goto modifiers;

    case TOK_UNSIGNED:
        if (modifiers_sign)
            return parse_error(tok, "multiple 'signed' or 'unsigned'");
        modifiers_sign--;
        next_token(tok);
        goto modifiers;

    default:
        break;
    }

    t1complex = 0;

    if (modifiers_length || modifiers_sign) {

        switch (tok->kind) {

        case TOK_VOID:
        case TOK__BOOL:
        case TOK_FLOAT:
        case TOK_STRUCT:
        case TOK_UNION:
        case TOK_ENUM:
        case TOK__COMPLEX:
            return parse_error(tok, "invalid combination of types");

        case TOK_DOUBLE:
            if (modifiers_sign != 0 || modifiers_length != 1)
                return parse_error(tok, "invalid combination of types");
            next_token(tok);
            t0 = _CFFI_PRIM_LONGDOUBLE;
            break;

        case TOK_CHAR:
            if (modifiers_length != 0)
                return parse_error(tok, "invalid combination of types");
            modifiers_length = -2;
            /* fall-through */
        case TOK_INT:
            next_token(tok);
            /* fall-through */
        default:
            if (modifiers_sign >= 0)
                switch (modifiers_length) {
                case -2: t0 = _CFFI_PRIM_SCHAR; break;
                case -1: t0 = _CFFI_PRIM_SHORT; break;
                case 1:  t0 = _CFFI_PRIM_LONG; break;
                case 2:  t0 = _CFFI_PRIM_LONGLONG; break;
                default: t0 = _CFFI_PRIM_INT; break;
                }
            else
                switch (modifiers_length) {
                case -2: t0 = _CFFI_PRIM_UCHAR; break;
                case -1: t0 = _CFFI_PRIM_USHORT; break;
                case 1:  t0 = _CFFI_PRIM_ULONG; break;
                case 2:  t0 = _CFFI_PRIM_ULONGLONG; break;
                default: t0 = _CFFI_PRIM_UINT; break;
                }
        }
        t1 = _CFFI_OP(_CFFI_OP_PRIMITIVE, t0);
    }
    else {
        switch (tok->kind) {
        case TOK_INT:
            t1 = _CFFI_OP(_CFFI_OP_PRIMITIVE, _CFFI_PRIM_INT);
            break;
        case TOK_CHAR:
            t1 = _CFFI_OP(_CFFI_OP_PRIMITIVE, _CFFI_PRIM_CHAR);
            break;
        case TOK_VOID:
            t1 = _CFFI_OP(_CFFI_OP_PRIMITIVE, _CFFI_PRIM_VOID);
            break;
        case TOK__BOOL:
            t1 = _CFFI_OP(_CFFI_OP_PRIMITIVE, _CFFI_PRIM_BOOL);
            break;
        case TOK_FLOAT:
            t1 = _CFFI_OP(_CFFI_OP_PRIMITIVE, _CFFI_PRIM_FLOAT);
            t1complex = _CFFI_OP(_CFFI_OP_PRIMITIVE, _CFFI_PRIM_FLOATCOMPLEX);
            break;
        case TOK_DOUBLE:
            t1 = _CFFI_OP(_CFFI_OP_PRIMITIVE, _CFFI_PRIM_DOUBLE);
            t1complex = _CFFI_OP(_CFFI_OP_PRIMITIVE, _CFFI_PRIM_DOUBLECOMPLEX);
            break;
        case TOK_IDENTIFIER:
        {
            const char *replacement;
            int n = search_in_typenames(tok->info->ctx, tok->p, tok->size);
            if (n >= 0) {
                t1 = _CFFI_OP(_CFFI_OP_TYPENAME, n);
                break;
            }
            n = search_standard_typename(tok->p, tok->size);
            if (n >= 0) {
                t1 = _CFFI_OP(_CFFI_OP_PRIMITIVE, n);
                break;
            }
            replacement = get_common_type(tok->p, tok->size);
            if (replacement != NULL) {
                n = parse_common_type_replacement(tok, replacement);
                if (n < 0)
                    return parse_error(tok, "internal error, please report!");
                t1 = _CFFI_OP(_CFFI_OP_NOOP, n);
                break;
            }
            return parse_error(tok, "undefined type name");
        }
        case TOK_STRUCT:
        case TOK_UNION:
        {
            int n, kind = tok->kind;
            next_token(tok);
            if (tok->kind != TOK_IDENTIFIER)
                return parse_error(tok, "struct or union name expected");

            n = search_in_struct_unions(tok->info->ctx, tok->p, tok->size);
            if (n < 0) {
                if (kind == TOK_STRUCT && tok->size == 8 &&
                        !memcmp(tok->p, "_IO_FILE", 8))
                    n = _CFFI__IO_FILE_STRUCT;
                else
                    return parse_error(tok, "undefined struct/union name");
            }
            else if (((tok->info->ctx->struct_unions[n].flags & _CFFI_F_UNION)
                      != 0) ^ (kind == TOK_UNION))
                return parse_error(tok, "wrong kind of tag: struct vs union");

            t1 = _CFFI_OP(_CFFI_OP_STRUCT_UNION, n);
            break;
        }
        case TOK_ENUM:
        {
            int n;
            next_token(tok);
            if (tok->kind != TOK_IDENTIFIER)
                return parse_error(tok, "enum name expected");

            n = search_in_enums(tok->info->ctx, tok->p, tok->size);
            if (n < 0)
                return parse_error(tok, "undefined enum name");

            t1 = _CFFI_OP(_CFFI_OP_ENUM, n);
            break;
        }
        default:
            return parse_error(tok, "identifier expected");
        }
        next_token(tok);
    }
    if (tok->kind == TOK__COMPLEX)
    {
        if (t1complex == 0)
            return parse_error(tok, "_Complex type combination unsupported");
        t1 = t1complex;
        next_token(tok);
    }

    return parse_sequel(tok, write_ds(tok, t1));
}


static
int parse_c_type_from(struct _cffi_parse_info_s *info, size_t *output_index,
                      const char *input)
{
    int result;
    token_t token;

    token.info = info;
    token.kind = TOK_START;
    token.input = input;
    token.p = input;
    token.size = 0;
    token.output = info->output;
    token.output_index = *output_index;

    next_token(&token);
    result = parse_complete(&token);

    *output_index = token.output_index;
    if (token.kind != TOK_END)
        return parse_error(&token, "unexpected symbol");
    return result;
}

static
int parse_c_type(struct _cffi_parse_info_s *info, const char *input)
{
    size_t output_index = 0;
    return parse_c_type_from(info, &output_index, input);
}

static
int parse_common_type_replacement(token_t *tok, const char *replacement)
{
    return parse_c_type_from(tok->info, &tok->output_index, replacement);
}
