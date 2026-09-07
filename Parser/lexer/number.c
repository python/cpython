#include "Python.h"
#include "pycore_token.h"

#include "lexer_internal.h"
#include "../tokenizer/helpers.h"

#define MAKE_TOKEN(token_type) _PyLexer_token_setup(tok, token, token_type, p_start, p_end)

static int
lookahead(struct tok_state *tok, const char *test)
{
    const char *s = test;
    int res = 0;
    while (1) {
        int c = tok_nextc(tok);
        if (*s == 0) {
            res = !is_potential_identifier_char(c);
        }
        else if (c == *s) {
            s++;
            continue;
        }

        tok_backup(tok, c);
        while (s != test) {
            tok_backup(tok, *--s);
        }
        return res;
    }
}

static int
verify_end_of_number(struct tok_state *tok, int c, const char *kind) {
    if (tok->tok_extra_tokens) {
        // When we are parsing extra tokens, we don't want to emit warnings
        // about invalid literals, because we want to be a bit more liberal.
        return 1;
    }
    /* Emit a deprecation warning only if the numeric literal is immediately
     * followed by one of keywords which can occur after a numeric literal
     * in valid code: "and", "else", "for", "if", "in", "is" and "or".
     * It allows to gradually deprecate existing valid code without adding
     * warning before error in most cases of invalid numeric literal (which
     * would be confusing and break existing tests).
     * Raise a syntax error with slightly better message than plain
     * "invalid syntax" if the numeric literal is immediately followed by
     * other keyword or identifier.
     */
    int r = 0;
    if (c == 'a') {
        r = lookahead(tok, "nd");
    }
    else if (c == 'e') {
        r = lookahead(tok, "lse");
    }
    else if (c == 'f') {
        r = lookahead(tok, "or");
    }
    else if (c == 'i') {
        int c2 = tok_nextc(tok);
        if (c2 == 'f' || c2 == 'n' || c2 == 's') {
            r = 1;
        }
        tok_backup(tok, c2);
    }
    else if (c == 'o') {
        r = lookahead(tok, "r");
    }
    else if (c == 'n') {
        r = lookahead(tok, "ot");
    }
    if (r) {
        tok_backup(tok, c);
        if (_PyTokenizer_parser_warn(tok, PyExc_SyntaxWarning,
                "invalid %s literal", kind))
        {
            return 0;
        }
        tok_nextc(tok);
    }
    else /* In future releases, only error will remain. */
    if (c < 128 && is_potential_identifier_char(c)) {
        tok_backup(tok, c);
        _PyTokenizer_syntaxerror(tok, "invalid %s literal", kind);
        return 0;
    }
    return 1;
}

static int
tok_decimal_tail(struct tok_state *tok)
{
    int c;

    while (1) {
        do {
            c = tok_nextc(tok);
        } while (Py_ISDIGIT(c));
        if (c != '_') {
            break;
        }
        c = tok_nextc(tok);
        if (!Py_ISDIGIT(c)) {
            tok_backup(tok, c);
            _PyTokenizer_syntaxerror(tok, "invalid decimal literal");
            return 0;
        }
    }
    return c;
}

int
_PyLexer_scan_number(struct tok_state *tok, struct token *token, int c,
                     int leading_dot)
{
    const char *p_start = NULL;
    const char *p_end = NULL;

    if (leading_dot) {
        goto fraction;
    }
    if (c == '0') {
        /* Hex, octal or binary -- maybe. */
        c = tok_nextc(tok);
        if (c == 'x' || c == 'X') {
            /* Hex */
            c = tok_nextc(tok);
            do {
                if (c == '_') {
                    c = tok_nextc(tok);
                }
                if (!Py_ISXDIGIT(c)) {
                    tok_backup(tok, c);
                    return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "invalid hexadecimal literal"));
                }
                do {
                    c = tok_nextc(tok);
                } while (Py_ISXDIGIT(c));
            } while (c == '_');
            if (!verify_end_of_number(tok, c, "hexadecimal")) {
                return MAKE_TOKEN(ERRORTOKEN);
            }
        }
        else if (c == 'o' || c == 'O') {
            /* Octal */
            c = tok_nextc(tok);
            do {
                if (c == '_') {
                    c = tok_nextc(tok);
                }
                if (c < '0' || c >= '8') {
                    if (Py_ISDIGIT(c)) {
                        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                                "invalid digit '%c' in octal literal", c));
                    }
                    else {
                        tok_backup(tok, c);
                        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "invalid octal literal"));
                    }
                }
                do {
                    c = tok_nextc(tok);
                } while ('0' <= c && c < '8');
            } while (c == '_');
            if (Py_ISDIGIT(c)) {
                return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                        "invalid digit '%c' in octal literal", c));
            }
            if (!verify_end_of_number(tok, c, "octal")) {
                return MAKE_TOKEN(ERRORTOKEN);
            }
        }
        else if (c == 'b' || c == 'B') {
            /* Binary */
            c = tok_nextc(tok);
            do {
                if (c == '_') {
                    c = tok_nextc(tok);
                }
                if (c != '0' && c != '1') {
                    if (Py_ISDIGIT(c)) {
                        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "invalid digit '%c' in binary literal", c));
                    }
                    else {
                        tok_backup(tok, c);
                        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "invalid binary literal"));
                    }
                }
                do {
                    c = tok_nextc(tok);
                } while (c == '0' || c == '1');
            } while (c == '_');
            if (Py_ISDIGIT(c)) {
                return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "invalid digit '%c' in binary literal", c));
            }
            if (!verify_end_of_number(tok, c, "binary")) {
                return MAKE_TOKEN(ERRORTOKEN);
            }
        }
        else {
            int nonzero = 0;
            /* maybe old-style octal; c is first char of it */
            /* in any case, allow '0' as a literal */
            while (1) {
                if (c == '_') {
                    c = tok_nextc(tok);
                    if (!Py_ISDIGIT(c)) {
                        tok_backup(tok, c);
                        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "invalid decimal literal"));
                    }
                }
                if (c != '0') {
                    break;
                }
                c = tok_nextc(tok);
            }
            char* zeros_end = tok->cur;
            if (Py_ISDIGIT(c)) {
                nonzero = 1;
                c = tok_decimal_tail(tok);
                if (c == 0) {
                    return MAKE_TOKEN(ERRORTOKEN);
                }
            }
            if (c == '.') {
                c = tok_nextc(tok);
                goto fraction;
            }
            else if (c == 'e' || c == 'E') {
                goto exponent;
            }
            else if (c == 'j' || c == 'J') {
                goto imaginary;
            }
            else if (nonzero && !tok->tok_extra_tokens) {
                /* Old-style octal: now disallowed. */
                tok_backup(tok, c);
                return MAKE_TOKEN(_PyTokenizer_syntaxerror_known_range(
                        tok, (int)(tok->start + 1 - tok->line_start),
                        (int)(zeros_end - tok->line_start),
                        "leading zeros in decimal integer "
                        "literals are not permitted; "
                        "use an 0o prefix for octal integers"));
            }
            if (!verify_end_of_number(tok, c, "decimal")) {
                return MAKE_TOKEN(ERRORTOKEN);
            }
        }
    }
    else {
        /* Decimal */
        c = tok_decimal_tail(tok);
        if (c == 0) {
            return MAKE_TOKEN(ERRORTOKEN);
        }
        {
            /* Accept floating-point numbers. */
            if (c == '.') {
                c = tok_nextc(tok);
    fraction:
                /* Fraction */
                if (Py_ISDIGIT(c)) {
                    c = tok_decimal_tail(tok);
                    if (c == 0) {
                        return MAKE_TOKEN(ERRORTOKEN);
                    }
                }
            }
            if (c == 'e' || c == 'E') {
                int e;
              exponent:
                e = c;
                /* Exponent part */
                c = tok_nextc(tok);
                if (c == '+' || c == '-') {
                    c = tok_nextc(tok);
                    if (!Py_ISDIGIT(c)) {
                        tok_backup(tok, c);
                        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "invalid decimal literal"));
                    }
                } else if (!Py_ISDIGIT(c)) {
                    tok_backup(tok, c);
                    if (!verify_end_of_number(tok, e, "decimal")) {
                        return MAKE_TOKEN(ERRORTOKEN);
                    }
                    tok_backup(tok, e);
                    p_start = tok->start;
                    p_end = tok->cur;
                    return MAKE_TOKEN(NUMBER);
                }
                c = tok_decimal_tail(tok);
                if (c == 0) {
                    return MAKE_TOKEN(ERRORTOKEN);
                }
            }
            if (c == 'j' || c == 'J') {
                /* Imaginary part */
    imaginary:
                c = tok_nextc(tok);
                if (!verify_end_of_number(tok, c, "imaginary")) {
                    return MAKE_TOKEN(ERRORTOKEN);
                }
            }
            else if (!verify_end_of_number(tok, c, "decimal")) {
                return MAKE_TOKEN(ERRORTOKEN);
            }
        }
    }
    tok_backup(tok, c);
    p_start = tok->start;
    p_end = tok->cur;
    return MAKE_TOKEN(NUMBER);
}
