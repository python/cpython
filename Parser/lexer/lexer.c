#include "Python.h"
#include "pycore_token.h"
#include "pycore_unicodeobject.h"
#include "errcode.h"

#include "state.h"
#include "../tokenizer/helpers.h"

/* Alternate tab spacing */
#define ALTTABSIZE 1

#define is_potential_identifier_start(c) (\
              (c >= 'a' && c <= 'z')\
               || (c >= 'A' && c <= 'Z')\
               || c == '_'\
               || (c >= 128))

#define is_potential_identifier_char(c) (\
              (c >= 'a' && c <= 'z')\
               || (c >= 'A' && c <= 'Z')\
               || (c >= '0' && c <= '9')\
               || c == '_'\
               || (c >= 128))

#ifdef Py_DEBUG
static inline tokenizer_mode* TOK_GET_MODE(struct tok_state* tok) {
    assert(tok->tok_mode_stack_index >= 0);
    assert(tok->tok_mode_stack_index < MAXFSTRINGLEVEL);
    return &(tok->tok_mode_stack[tok->tok_mode_stack_index]);
}
static inline tokenizer_mode* TOK_NEXT_MODE(struct tok_state* tok) {
    assert(tok->tok_mode_stack_index >= 0);
    assert(tok->tok_mode_stack_index + 1 < MAXFSTRINGLEVEL);
    return &(tok->tok_mode_stack[++tok->tok_mode_stack_index]);
}
#else
#define TOK_GET_MODE(tok) (&(tok->tok_mode_stack[tok->tok_mode_stack_index]))
#define TOK_NEXT_MODE(tok) (&(tok->tok_mode_stack[++tok->tok_mode_stack_index]))
#endif

#define MAKE_TOKEN(token_type) _PyLexer_token_setup(tok, token, token_type, p_start, p_end)
#define MAKE_TYPE_COMMENT_TOKEN(token_type, col_offset, end_col_offset) (\
                _PyLexer_type_comment_token_setup(tok, token, token_type, col_offset, end_col_offset, p_start, p_end))

/* Spaces in this constant are treated as "zero or more spaces or tabs" when
   tokenizing. */
static const char* type_comment_prefix = "# type: ";

static inline int
contains_null_bytes(const char* str, size_t size)
{
    return memchr(str, 0, size) != NULL;
}

/* Get next char, updating state; error code goes into tok->done */
static int
tok_nextc(struct tok_state *tok)
{
    int rc;
    for (;;) {
        if (tok->cur != tok->inp) {
            if ((unsigned int) tok->col_offset >= (unsigned int) INT_MAX) {
                tok->done = E_COLUMNOVERFLOW;
                return EOF;
            }
            tok->col_offset++;
            return Py_CHARMASK(*tok->cur++); /* Fast path */
        }
        if (tok->done != E_OK) {
            return EOF;
        }
        rc = tok->underflow(tok);
#if defined(Py_DEBUG)
        if (tok->debug) {
            fprintf(stderr, "line[%d] = ", tok->lineno);
            _PyTokenizer_print_escape(stderr, tok->cur, tok->inp - tok->cur);
            fprintf(stderr, "  tok->done = %d\n", tok->done);
        }
#endif
        if (!rc) {
            tok->cur = tok->inp;
            return EOF;
        }
        tok->line_start = tok->cur;

        if (contains_null_bytes(tok->line_start, tok->inp - tok->line_start)) {
            _PyTokenizer_syntaxerror(tok, "source code cannot contain null bytes");
            tok->cur = tok->inp;
            return EOF;
        }
    }
    Py_UNREACHABLE();
}

/* Back-up one character */
static void
tok_backup(struct tok_state *tok, int c)
{
    if (c != EOF) {
        if (--tok->cur < tok->buf) {
            Py_FatalError("tokenizer beginning of buffer");
        }
        if ((int)(unsigned char)*tok->cur != Py_CHARMASK(c)) {
            Py_FatalError("tok_backup: wrong character");
        }
        tok->col_offset--;
    }
}

static int
set_fstring_expr(struct tok_state* tok, struct token *token, char c) {
    assert(token != NULL);
    assert(c == '}' || c == ':' || c == '!');
    tokenizer_mode *tok_mode = TOK_GET_MODE(tok);

    if (!tok_mode->f_string_debug || token->metadata) {
        return 0;
    }
    PyObject *res = NULL;

    // Check if there is a # character in the expression
    int hash_detected = 0;
    for (Py_ssize_t i = 0; i < tok_mode->last_expr_size - tok_mode->last_expr_end; i++) {
        if (tok_mode->last_expr_buffer[i] == '#') {
            hash_detected = 1;
            break;
        }
    }

    if (hash_detected) {
        Py_ssize_t input_length = tok_mode->last_expr_size - tok_mode->last_expr_end;
        char *result = (char *)PyMem_Malloc((input_length + 1) * sizeof(char));
        if (!result) {
            return -1;
        }

        Py_ssize_t i = 0;
        Py_ssize_t j = 0;

        for (i = 0, j = 0; i < input_length; i++) {
            if (tok_mode->last_expr_buffer[i] == '#') {
                // Skip characters until newline or end of string
                while (tok_mode->last_expr_buffer[i] != '\0' && i < input_length) {
                    if (tok_mode->last_expr_buffer[i] == '\n') {
                        result[j++] = tok_mode->last_expr_buffer[i];
                        break;
                    }
                    i++;
                }
            } else {
                result[j++] = tok_mode->last_expr_buffer[i];
            }
        }

        result[j] = '\0';  // Null-terminate the result string
        res = PyUnicode_DecodeUTF8(result, j, NULL);
        PyMem_Free(result);
    } else {
        res = PyUnicode_DecodeUTF8(
            tok_mode->last_expr_buffer,
            tok_mode->last_expr_size - tok_mode->last_expr_end,
            NULL
        );

    }


   if (!res) {
        return -1;
    }
    token->metadata = res;
    return 0;
}

int
_PyLexer_update_fstring_expr(struct tok_state *tok, char cur)
{
    assert(tok->cur != NULL);

    Py_ssize_t size = strlen(tok->cur);
    tokenizer_mode *tok_mode = TOK_GET_MODE(tok);

    switch (cur) {
       case 0:
            if (!tok_mode->last_expr_buffer || tok_mode->last_expr_end >= 0) {
                return 1;
            }
            char *new_buffer = PyMem_Realloc(
                tok_mode->last_expr_buffer,
                tok_mode->last_expr_size + size
            );
            if (new_buffer == NULL) {
                PyMem_Free(tok_mode->last_expr_buffer);
                goto error;
            }
            tok_mode->last_expr_buffer = new_buffer;
            strncpy(tok_mode->last_expr_buffer + tok_mode->last_expr_size, tok->cur, size);
            tok_mode->last_expr_size += size;
            break;
        case '{':
            if (tok_mode->last_expr_buffer != NULL) {
                PyMem_Free(tok_mode->last_expr_buffer);
            }
            tok_mode->last_expr_buffer = PyMem_Malloc(size);
            if (tok_mode->last_expr_buffer == NULL) {
                goto error;
            }
            tok_mode->last_expr_size = size;
            tok_mode->last_expr_end = -1;
            strncpy(tok_mode->last_expr_buffer, tok->cur, size);
            break;
        case '}':
        case '!':
        case ':':
            if (tok_mode->last_expr_end == -1) {
                tok_mode->last_expr_end = strlen(tok->start);
            }
            break;
        default:
            Py_UNREACHABLE();
    }
    return 1;
error:
    tok->done = E_NOMEM;
    return 0;
}

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

/* Verify that the identifier follows PEP 3131.
   All identifier strings are guaranteed to be "ready" unicode objects.
 */
static int
verify_identifier(struct tok_state *tok)
{
    if (tok->tok_extra_tokens) {
        return 1;
    }
    PyObject *s;
    if (tok->decoding_erred)
        return 0;
    s = PyUnicode_DecodeUTF8(tok->start, tok->cur - tok->start, NULL);
    if (s == NULL) {
        if (PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)) {
            tok->done = E_DECODE;
        }
        else {
            tok->done = E_ERROR;
        }
        return 0;
    }
    Py_ssize_t invalid = _PyUnicode_ScanIdentifier(s);
    if (invalid < 0) {
        Py_DECREF(s);
        tok->done = E_ERROR;
        return 0;
    }
    assert(PyUnicode_GET_LENGTH(s) > 0);
    if (invalid < PyUnicode_GET_LENGTH(s)) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(s, invalid);
        if (invalid + 1 < PyUnicode_GET_LENGTH(s)) {
            /* Determine the offset in UTF-8 encoded input */
            Py_SETREF(s, PyUnicode_Substring(s, 0, invalid + 1));
            if (s != NULL) {
                Py_SETREF(s, PyUnicode_AsUTF8String(s));
            }
            if (s == NULL) {
                tok->done = E_ERROR;
                return 0;
            }
            tok->cur = (char *)tok->start + PyBytes_GET_SIZE(s);
        }
        Py_DECREF(s);
        if (Py_UNICODE_ISPRINTABLE(ch)) {
            _PyTokenizer_syntaxerror(tok, "invalid character '%c' (U+%04X)", ch, ch);
        }
        else {
            _PyTokenizer_syntaxerror(tok, "invalid non-printable character U+%04X", ch);
        }
        return 0;
    }
    Py_DECREF(s);
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

static inline int
tok_continuation_line(struct tok_state *tok) {
    int c = tok_nextc(tok);
    if (c == '\r') {
        c = tok_nextc(tok);
    }
    if (c != '\n') {
        tok->done = E_LINECONT;
        return -1;
    }
    c = tok_nextc(tok);
    if (c == EOF) {
        tok->done = E_EOF;
        tok->cur = tok->inp;
        return -1;
    } else {
        tok_backup(tok, c);
    }
    return c;
}

static int
tok_get_normal_mode(struct tok_state *tok, tokenizer_mode* current_tok, struct token *token)
{
    int c;
    int blankline, nonascii;

    const char *p_start = NULL;
    const char *p_end = NULL;
  nextline:
    tok->start = NULL;
    tok->starting_col_offset = -1;
    blankline = 0;


    /* Get indentation level */
    if (tok->atbol) {
        int col = 0;
        int altcol = 0;
        tok->atbol = 0;
        int cont_line_col = 0;
        for (;;) {
            c = tok_nextc(tok);
            if (c == ' ') {
                col++, altcol++;
            }
            else if (c == '\t') {
                col = (col / tok->tabsize + 1) * tok->tabsize;
                altcol = (altcol / ALTTABSIZE + 1) * ALTTABSIZE;
            }
            else if (c == '\014')  {/* Control-L (formfeed) */
                col = altcol = 0; /* For Emacs users */
            }
            else if (c == '\\') {
                // Indentation cannot be split over multiple physical lines
                // using backslashes. This means that if we found a backslash
                // preceded by whitespace, **the first one we find** determines
                // the level of indentation of whatever comes next.
                cont_line_col = cont_line_col ? cont_line_col : col;
                if ((c = tok_continuation_line(tok)) == -1) {
                    return MAKE_TOKEN(ERRORTOKEN);
                }
            }
            else {
                break;
            }
        }
        tok_backup(tok, c);
        if (c == '#' || c == '\n' || c == '\r') {
            /* Lines with only whitespace and/or comments
               shouldn't affect the indentation and are
               not passed to the parser as NEWLINE tokens,
               except *totally* empty lines in interactive
               mode, which signal the end of a command group. */
            if (col == 0 && c == '\n' && tok->prompt != NULL) {
                blankline = 0; /* Let it through */
            }
            else if (tok->prompt != NULL && tok->lineno == 1) {
                /* In interactive mode, if the first line contains
                   only spaces and/or a comment, let it through. */
                blankline = 0;
                col = altcol = 0;
            }
            else {
                blankline = 1; /* Ignore completely */
            }
            /* We can't jump back right here since we still
               may need to skip to the end of a comment */
        }
        if (!blankline && tok->level == 0) {
            col = cont_line_col ? cont_line_col : col;
            altcol = cont_line_col ? cont_line_col : altcol;
            if (col == tok->indstack[tok->indent]) {
                /* No change */
                if (altcol != tok->altindstack[tok->indent]) {
                    return MAKE_TOKEN(_PyTokenizer_indenterror(tok));
                }
            }
            else if (col > tok->indstack[tok->indent]) {
                /* Indent -- always one */
                if (tok->indent+1 >= MAXINDENT) {
                    tok->done = E_TOODEEP;
                    tok->cur = tok->inp;
                    return MAKE_TOKEN(ERRORTOKEN);
                }
                if (altcol <= tok->altindstack[tok->indent]) {
                    return MAKE_TOKEN(_PyTokenizer_indenterror(tok));
                }
                tok->pendin++;
                tok->indstack[++tok->indent] = col;
                tok->altindstack[tok->indent] = altcol;
            }
            else /* col < tok->indstack[tok->indent] */ {
                /* Dedent -- any number, must be consistent */
                while (tok->indent > 0 &&
                    col < tok->indstack[tok->indent]) {
                    tok->pendin--;
                    tok->indent--;
                }
                if (col != tok->indstack[tok->indent]) {
                    tok->done = E_DEDENT;
                    tok->cur = tok->inp;
                    return MAKE_TOKEN(ERRORTOKEN);
                }
                if (altcol != tok->altindstack[tok->indent]) {
                    return MAKE_TOKEN(_PyTokenizer_indenterror(tok));
                }
            }
        }
    }

    tok->start = tok->cur;
    tok->starting_col_offset = tok->col_offset;

    /* Return pending indents/dedents */
    if (tok->pendin != 0) {
        if (tok->pendin < 0) {
            if (tok->tok_extra_tokens) {
                p_start = tok->cur;
                p_end = tok->cur;
            }
            tok->pendin++;
            return MAKE_TOKEN(DEDENT);
        }
        else {
            if (tok->tok_extra_tokens) {
                p_start = tok->buf;
                p_end = tok->cur;
            }
            tok->pendin--;
            return MAKE_TOKEN(INDENT);
        }
    }

    /* Peek ahead at the next character */
    c = tok_nextc(tok);
    tok_backup(tok, c);

 again:
    tok->start = NULL;
    /* Skip spaces */
    do {
        c = tok_nextc(tok);
    } while (c == ' ' || c == '\t' || c == '\014');

    /* Set start of current token */
    tok->start = tok->cur == NULL ? NULL : tok->cur - 1;
    tok->starting_col_offset = tok->col_offset - 1;

    /* Skip comment, unless it's a type comment */
    if (c == '#') {

        const char* p = NULL;
        const char *prefix, *type_start;
        int current_starting_col_offset;

        while (c != EOF && c != '\n' && c != '\r') {
            c = tok_nextc(tok);
        }

        if (tok->tok_extra_tokens) {
            p = tok->start;
        }

        if (tok->type_comments) {
            p = tok->start;
            current_starting_col_offset = tok->starting_col_offset;
            prefix = type_comment_prefix;
            while (*prefix && p < tok->cur) {
                if (*prefix == ' ') {
                    while (*p == ' ' || *p == '\t') {
                        p++;
                        current_starting_col_offset++;
                    }
                } else if (*prefix == *p) {
                    p++;
                    current_starting_col_offset++;
                } else {
                    break;
                }

                prefix++;
            }

            /* This is a type comment if we matched all of type_comment_prefix. */
            if (!*prefix) {
                int is_type_ignore = 1;
                // +6 in order to skip the word 'ignore'
                const char *ignore_end = p + 6;
                const int ignore_end_col_offset = current_starting_col_offset + 6;
                tok_backup(tok, c);  /* don't eat the newline or EOF */

                type_start = p;

                /* A TYPE_IGNORE is "type: ignore" followed by the end of the token
                 * or anything ASCII and non-alphanumeric. */
                is_type_ignore = (
                    tok->cur >= ignore_end && memcmp(p, "ignore", 6) == 0
                    && !(tok->cur > ignore_end
                         && ((unsigned char)ignore_end[0] >= 128 || Py_ISALNUM(ignore_end[0]))));

                if (is_type_ignore) {
                    p_start = ignore_end;
                    p_end = tok->cur;

                    /* If this type ignore is the only thing on the line, consume the newline also. */
                    if (blankline) {
                        tok_nextc(tok);
                        tok->atbol = 1;
                    }
                    return MAKE_TYPE_COMMENT_TOKEN(TYPE_IGNORE, ignore_end_col_offset, tok->col_offset);
                } else {
                    p_start = type_start;
                    p_end = tok->cur;
                    return MAKE_TYPE_COMMENT_TOKEN(TYPE_COMMENT, current_starting_col_offset, tok->col_offset);
                }
            }
        }
        if (tok->tok_extra_tokens) {
            tok_backup(tok, c);  /* don't eat the newline or EOF */
            p_start = p;
            p_end = tok->cur;
            tok->comment_newline = blankline;
            return MAKE_TOKEN(COMMENT);
        }
    }

    if (tok->done == E_INTERACT_STOP) {
        return MAKE_TOKEN(ENDMARKER);
    }

    /* Check for EOF and errors now */
    if (c == EOF) {
        if (tok->level) {
            return MAKE_TOKEN(ERRORTOKEN);
        }
        return MAKE_TOKEN(tok->done == E_EOF ? ENDMARKER : ERRORTOKEN);
    }

    /* Identifier (most frequent token!) */
    nonascii = 0;
    if (is_potential_identifier_start(c)) {
        /* Process the various legal combinations of b"", r"", u"", and f"". */
        int saw_b = 0, saw_r = 0, saw_u = 0, saw_f = 0;
        while (1) {
            if (!(saw_b || saw_u || saw_f) && (c == 'b' || c == 'B'))
                saw_b = 1;
            /* Since this is a backwards compatibility support literal we don't
               want to support it in arbitrary order like byte literals. */
            else if (!(saw_b || saw_u || saw_r || saw_f)
                     && (c == 'u'|| c == 'U')) {
                saw_u = 1;
            }
            /* ur"" and ru"" are not supported */
            else if (!(saw_r || saw_u) && (c == 'r' || c == 'R')) {
                saw_r = 1;
            }
            else if (!(saw_f || saw_b || saw_u) && (c == 'f' || c == 'F')) {
                saw_f = 1;
            }
            else {
                break;
            }
            c = tok_nextc(tok);
            if (c == '"' || c == '\'') {
                if (saw_f) {
                    goto f_string_quote;
                }
                goto letter_quote;
            }
        }
        while (is_potential_identifier_char(c)) {
            if (c >= 128) {
                nonascii = 1;
            }
            c = tok_nextc(tok);
        }
        tok_backup(tok, c);
        if (nonascii && !verify_identifier(tok)) {
            return MAKE_TOKEN(ERRORTOKEN);
        }

        p_start = tok->start;
        p_end = tok->cur;

        return MAKE_TOKEN(NAME);
    }

    if (c == '\r') {
        c = tok_nextc(tok);
    }

    /* Newline */
    if (c == '\n') {
        tok->atbol = 1;
        if (blankline || tok->level > 0) {
            if (tok->tok_extra_tokens) {
                if (tok->comment_newline) {
                    tok->comment_newline = 0;
                }
                p_start = tok->start;
                p_end = tok->cur;
                return MAKE_TOKEN(NL);
            }
            goto nextline;
        }
        if (tok->comment_newline && tok->tok_extra_tokens) {
            tok->comment_newline = 0;
            p_start = tok->start;
            p_end = tok->cur;
            return MAKE_TOKEN(NL);
        }
        p_start = tok->start;
        p_end = tok->cur - 1; /* Leave '\n' out of the string */
        tok->cont_line = 0;
        return MAKE_TOKEN(NEWLINE);
    }

    /* Period or number starting with period? */
    if (c == '.') {
        c = tok_nextc(tok);
        if (Py_ISDIGIT(c)) {
            goto fraction;
        } else if (c == '.') {
            c = tok_nextc(tok);
            if (c == '.') {
                p_start = tok->start;
                p_end = tok->cur;
                return MAKE_TOKEN(ELLIPSIS);
            }
            else {
                tok_backup(tok, c);
            }
            tok_backup(tok, '.');
        }
        else {
            tok_backup(tok, c);
        }
        p_start = tok->start;
        p_end = tok->cur;
        return MAKE_TOKEN(DOT);
    }

    /* Number */
    if (Py_ISDIGIT(c)) {
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

  f_string_quote:
    if (((Py_TOLOWER(*tok->start) == 'f' || Py_TOLOWER(*tok->start) == 'r') && (c == '\'' || c == '"'))) {
        int quote = c;
        int quote_size = 1;             /* 1 or 3 */

        /* Nodes of type STRING, especially multi line strings
           must be handled differently in order to get both
           the starting line number and the column offset right.
           (cf. issue 16806) */
        tok->first_lineno = tok->lineno;
        tok->multi_line_start = tok->line_start;

        /* Find the quote size and start of string */
        int after_quote = tok_nextc(tok);
        if (after_quote == quote) {
            int after_after_quote = tok_nextc(tok);
            if (after_after_quote == quote) {
                quote_size = 3;
            }
            else {
                // TODO: Check this
                tok_backup(tok, after_after_quote);
                tok_backup(tok, after_quote);
            }
        }
        if (after_quote != quote) {
            tok_backup(tok, after_quote);
        }


        p_start = tok->start;
        p_end = tok->cur;
        if (tok->tok_mode_stack_index + 1 >= MAXFSTRINGLEVEL) {
            return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "too many nested f-strings"));
        }
        tokenizer_mode *the_current_tok = TOK_NEXT_MODE(tok);
        the_current_tok->kind = TOK_FSTRING_MODE;
        the_current_tok->f_string_quote = quote;
        the_current_tok->f_string_quote_size = quote_size;
        the_current_tok->f_string_start = tok->start;
        the_current_tok->f_string_multi_line_start = tok->line_start;
        the_current_tok->f_string_line_start = tok->lineno;
        the_current_tok->f_string_start_offset = -1;
        the_current_tok->f_string_multi_line_start_offset = -1;
        the_current_tok->last_expr_buffer = NULL;
        the_current_tok->last_expr_size = 0;
        the_current_tok->last_expr_end = -1;
        the_current_tok->in_format_spec = 0;
        the_current_tok->f_string_debug = 0;

        switch (*tok->start) {
            case 'F':
            case 'f':
                the_current_tok->f_string_raw = Py_TOLOWER(*(tok->start + 1)) == 'r';
                break;
            case 'R':
            case 'r':
                the_current_tok->f_string_raw = 1;
                break;
            default:
                Py_UNREACHABLE();
        }

        the_current_tok->curly_bracket_depth = 0;
        the_current_tok->curly_bracket_expr_start_depth = -1;
        return MAKE_TOKEN(FSTRING_START);
    }

  letter_quote:
    /* String */
    if (c == '\'' || c == '"') {
        int quote = c;
        int quote_size = 1;             /* 1 or 3 */
        int end_quote_size = 0;
        int has_escaped_quote = 0;

        /* Nodes of type STRING, especially multi line strings
           must be handled differently in order to get both
           the starting line number and the column offset right.
           (cf. issue 16806) */
        tok->first_lineno = tok->lineno;
        tok->multi_line_start = tok->line_start;

        /* Find the quote size and start of string */
        c = tok_nextc(tok);
        if (c == quote) {
            c = tok_nextc(tok);
            if (c == quote) {
                quote_size = 3;
            }
            else {
                end_quote_size = 1;     /* empty string found */
            }
        }
        if (c != quote) {
            tok_backup(tok, c);
        }

        /* Get rest of string */
        while (end_quote_size != quote_size) {
            c = tok_nextc(tok);
            if (tok->done == E_ERROR) {
                return MAKE_TOKEN(ERRORTOKEN);
            }
            if (tok->done == E_DECODE) {
                break;
            }
            if (c == EOF || (quote_size == 1 && c == '\n')) {
                assert(tok->multi_line_start != NULL);
                // shift the tok_state's location into
                // the start of string, and report the error
                // from the initial quote character
                tok->cur = (char *)tok->start;
                tok->cur++;
                tok->line_start = tok->multi_line_start;
                int start = tok->lineno;
                tok->lineno = tok->first_lineno;

                if (INSIDE_FSTRING(tok)) {
                    /* When we are in an f-string, before raising the
                     * unterminated string literal error, check whether
                     * does the initial quote matches with f-strings quotes
                     * and if it is, then this must be a missing '}' token
                     * so raise the proper error */
                    tokenizer_mode *the_current_tok = TOK_GET_MODE(tok);
                    if (the_current_tok->f_string_quote == quote &&
                        the_current_tok->f_string_quote_size == quote_size) {
                        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "f-string: expecting '}'", start));
                    }
                }

                if (quote_size == 3) {
                    _PyTokenizer_syntaxerror(tok, "unterminated triple-quoted string literal"
                                     " (detected at line %d)", start);
                    if (c != '\n') {
                        tok->done = E_EOFS;
                    }
                    return MAKE_TOKEN(ERRORTOKEN);
                }
                else {
                    if (has_escaped_quote) {
                        _PyTokenizer_syntaxerror(
                            tok,
                            "unterminated string literal (detected at line %d); "
                            "perhaps you escaped the end quote?",
                            start
                        );
                    } else {
                        _PyTokenizer_syntaxerror(
                            tok, "unterminated string literal (detected at line %d)", start
                        );
                    }
                    if (c != '\n') {
                        tok->done = E_EOLS;
                    }
                    return MAKE_TOKEN(ERRORTOKEN);
                }
            }
            if (c == quote) {
                end_quote_size += 1;
            }
            else {
                end_quote_size = 0;
                if (c == '\\') {
                    c = tok_nextc(tok);  /* skip escaped char */
                    if (c == quote) {  /* but record whether the escaped char was a quote */
                        has_escaped_quote = 1;
                    }
                    if (c == '\r') {
                        c = tok_nextc(tok);
                    }
                }
            }
        }

        p_start = tok->start;
        p_end = tok->cur;
        return MAKE_TOKEN(STRING);
    }

    /* Line continuation */
    if (c == '\\') {
        if ((c = tok_continuation_line(tok)) == -1) {
            return MAKE_TOKEN(ERRORTOKEN);
        }
        tok->cont_line = 1;
        goto again; /* Read next line */
    }

    /* Punctuation character */
    int is_punctuation = (c == ':' || c == '}' || c == '!' || c == '{');
    if (is_punctuation && INSIDE_FSTRING(tok) && INSIDE_FSTRING_EXPR(current_tok)) {
        /* This code block gets executed before the curly_bracket_depth is incremented
         * by the `{` case, so for ensuring that we are on the 0th level, we need
         * to adjust it manually */
        int cursor = current_tok->curly_bracket_depth - (c != '{');
        int in_format_spec = current_tok->in_format_spec;
         int cursor_in_format_with_debug =
             cursor == 1 && (current_tok->f_string_debug || in_format_spec);
         int cursor_valid = cursor == 0 || cursor_in_format_with_debug;
        if ((cursor_valid) && !_PyLexer_update_fstring_expr(tok, c)) {
            return MAKE_TOKEN(ENDMARKER);
        }
        if ((cursor_valid) && c != '{' && set_fstring_expr(tok, token, c)) {
            return MAKE_TOKEN(ERRORTOKEN);
        }

        if (c == ':' && cursor == current_tok->curly_bracket_expr_start_depth) {
            current_tok->kind = TOK_FSTRING_MODE;
            current_tok->in_format_spec = 1;
            p_start = tok->start;
            p_end = tok->cur;
            return MAKE_TOKEN(_PyToken_OneChar(c));
        }
    }

    /* Check for two-character token */
    {
        int c2 = tok_nextc(tok);
        int current_token = _PyToken_TwoChars(c, c2);
        if (current_token != OP) {
            int c3 = tok_nextc(tok);
            int current_token3 = _PyToken_ThreeChars(c, c2, c3);
            if (current_token3 != OP) {
                current_token = current_token3;
            }
            else {
                tok_backup(tok, c3);
            }
            p_start = tok->start;
            p_end = tok->cur;
            return MAKE_TOKEN(current_token);
        }
        tok_backup(tok, c2);
    }

    /* Keep track of parentheses nesting level */
    switch (c) {
    case '(':
    case '[':
    case '{':
        if (tok->level >= MAXLEVEL) {
            return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "too many nested parentheses"));
        }
        tok->parenstack[tok->level] = c;
        tok->parenlinenostack[tok->level] = tok->lineno;
        tok->parencolstack[tok->level] = (int)(tok->start - tok->line_start);
        tok->level++;
        if (INSIDE_FSTRING(tok)) {
            current_tok->curly_bracket_depth++;
        }
        break;
    case ')':
    case ']':
    case '}':
        if (INSIDE_FSTRING(tok) && !current_tok->curly_bracket_depth && c == '}') {
            return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "f-string: single '}' is not allowed"));
        }
        if (!tok->tok_extra_tokens && !tok->level) {
            return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "unmatched '%c'", c));
        }
        if (tok->level > 0) {
            tok->level--;
            int opening = tok->parenstack[tok->level];
            if (!tok->tok_extra_tokens && !((opening == '(' && c == ')') ||
                                            (opening == '[' && c == ']') ||
                                            (opening == '{' && c == '}'))) {
                /* If the opening bracket belongs to an f-string's expression
                part (e.g. f"{)}") and the closing bracket is an arbitrary
                nested expression, then instead of matching a different
                syntactical construct with it; we'll throw an unmatched
                parentheses error. */
                if (INSIDE_FSTRING(tok) && opening == '{') {
                    assert(current_tok->curly_bracket_depth >= 0);
                    int previous_bracket = current_tok->curly_bracket_depth - 1;
                    if (previous_bracket == current_tok->curly_bracket_expr_start_depth) {
                        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "f-string: unmatched '%c'", c));
                    }
                }
                if (tok->parenlinenostack[tok->level] != tok->lineno) {
                    return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                            "closing parenthesis '%c' does not match "
                            "opening parenthesis '%c' on line %d",
                            c, opening, tok->parenlinenostack[tok->level]));
                }
                else {
                    return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                            "closing parenthesis '%c' does not match "
                            "opening parenthesis '%c'",
                            c, opening));
                }
            }
        }

        if (INSIDE_FSTRING(tok)) {
            current_tok->curly_bracket_depth--;
            if (current_tok->curly_bracket_depth < 0) {
                return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "f-string: unmatched '%c'", c));
            }
            if (c == '}' && current_tok->curly_bracket_depth == current_tok->curly_bracket_expr_start_depth) {
                current_tok->curly_bracket_expr_start_depth--;
                current_tok->kind = TOK_FSTRING_MODE;
                current_tok->in_format_spec = 0;
                current_tok->f_string_debug = 0;
            }
        }
        break;
    default:
        break;
    }

    if (!Py_UNICODE_ISPRINTABLE(c)) {
        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "invalid non-printable character U+%04X", c));
    }

    if( c == '=' && INSIDE_FSTRING_EXPR(current_tok)) {
        current_tok->f_string_debug = 1;
    }

    /* Punctuation character */
    p_start = tok->start;
    p_end = tok->cur;
    return MAKE_TOKEN(_PyToken_OneChar(c));
}

static int
tok_get_fstring_mode(struct tok_state *tok, tokenizer_mode* current_tok, struct token *token)
{
    const char *p_start = NULL;
    const char *p_end = NULL;
    int end_quote_size = 0;
    int unicode_escape = 0;

    tok->start = tok->cur;
    tok->first_lineno = tok->lineno;
    tok->starting_col_offset = tok->col_offset;

    // If we start with a bracket, we defer to the normal mode as there is nothing for us to tokenize
    // before it.
    int start_char = tok_nextc(tok);
    if (start_char == '{') {
        int peek1 = tok_nextc(tok);
        tok_backup(tok, peek1);
        tok_backup(tok, start_char);
        if (peek1 != '{') {
            current_tok->curly_bracket_expr_start_depth++;
            if (current_tok->curly_bracket_expr_start_depth >= MAX_EXPR_NESTING) {
                return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "f-string: expressions nested too deeply"));
            }
            TOK_GET_MODE(tok)->kind = TOK_REGULAR_MODE;
            return tok_get_normal_mode(tok, current_tok, token);
        }
    }
    else {
        tok_backup(tok, start_char);
    }

    // Check if we are at the end of the string
    for (int i = 0; i < current_tok->f_string_quote_size; i++) {
        int quote = tok_nextc(tok);
        if (quote != current_tok->f_string_quote) {
            tok_backup(tok, quote);
            goto f_string_middle;
        }
    }

    if (current_tok->last_expr_buffer != NULL) {
        PyMem_Free(current_tok->last_expr_buffer);
        current_tok->last_expr_buffer = NULL;
        current_tok->last_expr_size = 0;
        current_tok->last_expr_end = -1;
    }

    p_start = tok->start;
    p_end = tok->cur;
    tok->tok_mode_stack_index--;
    return MAKE_TOKEN(FSTRING_END);

f_string_middle:

    // TODO: This is a bit of a hack, but it works for now. We need to find a better way to handle
    // this.
    tok->multi_line_start = tok->line_start;
    while (end_quote_size != current_tok->f_string_quote_size) {
        int c = tok_nextc(tok);
        if (tok->done == E_ERROR || tok->done == E_DECODE) {
            return MAKE_TOKEN(ERRORTOKEN);
        }
        int in_format_spec = (
                current_tok->in_format_spec
                &&
                INSIDE_FSTRING_EXPR(current_tok)
        );

       if (c == EOF || (current_tok->f_string_quote_size == 1 && c == '\n')) {
            if (tok->decoding_erred) {
                return MAKE_TOKEN(ERRORTOKEN);
            }

            // If we are in a format spec and we found a newline,
            // it means that the format spec ends here and we should
            // return to the regular mode.
            if (in_format_spec && c == '\n') {
                tok_backup(tok, c);
                TOK_GET_MODE(tok)->kind = TOK_REGULAR_MODE;
                current_tok->in_format_spec = 0;
                p_start = tok->start;
                p_end = tok->cur;
                return MAKE_TOKEN(FSTRING_MIDDLE);
            }

            assert(tok->multi_line_start != NULL);
            // shift the tok_state's location into
            // the start of string, and report the error
            // from the initial quote character
            tok->cur = (char *)current_tok->f_string_start;
            tok->cur++;
            tok->line_start = current_tok->f_string_multi_line_start;
            int start = tok->lineno;

            tokenizer_mode *the_current_tok = TOK_GET_MODE(tok);
            tok->lineno = the_current_tok->f_string_line_start;

            if (current_tok->f_string_quote_size == 3) {
                _PyTokenizer_syntaxerror(tok,
                                    "unterminated triple-quoted f-string literal"
                                    " (detected at line %d)", start);
                if (c != '\n') {
                    tok->done = E_EOFS;
                }
                return MAKE_TOKEN(ERRORTOKEN);
            }
            else {
                return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                                    "unterminated f-string literal (detected at"
                                    " line %d)", start));
            }
        }

        if (c == current_tok->f_string_quote) {
            end_quote_size += 1;
            continue;
        } else {
            end_quote_size = 0;
        }

        if (c == '{') {
            if (!_PyLexer_update_fstring_expr(tok, c)) {
                return MAKE_TOKEN(ENDMARKER);
            }
            int peek = tok_nextc(tok);
            if (peek != '{' || in_format_spec) {
                tok_backup(tok, peek);
                tok_backup(tok, c);
                current_tok->curly_bracket_expr_start_depth++;
                if (current_tok->curly_bracket_expr_start_depth >= MAX_EXPR_NESTING) {
                    return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "f-string: expressions nested too deeply"));
                }
                TOK_GET_MODE(tok)->kind = TOK_REGULAR_MODE;
                current_tok->in_format_spec = 0;
                p_start = tok->start;
                p_end = tok->cur;
            } else {
                p_start = tok->start;
                p_end = tok->cur - 1;
            }
            return MAKE_TOKEN(FSTRING_MIDDLE);
        } else if (c == '}') {
            if (unicode_escape) {
                p_start = tok->start;
                p_end = tok->cur;
                return MAKE_TOKEN(FSTRING_MIDDLE);
            }
            int peek = tok_nextc(tok);

            // The tokenizer can only be in the format spec if we have already completed the expression
            // scanning (indicated by the end of the expression being set) and we are not at the top level
            // of the bracket stack (-1 is the top level). Since format specifiers can't legally use double
            // brackets, we can bypass it here.
            int cursor = current_tok->curly_bracket_depth;
            if (peek == '}' && !in_format_spec && cursor == 0) {
                p_start = tok->start;
                p_end = tok->cur - 1;
            } else {
                tok_backup(tok, peek);
                tok_backup(tok, c);
                TOK_GET_MODE(tok)->kind = TOK_REGULAR_MODE;
                current_tok->in_format_spec = 0;
                p_start = tok->start;
                p_end = tok->cur;
            }
            return MAKE_TOKEN(FSTRING_MIDDLE);
        } else if (c == '\\') {
            int peek = tok_nextc(tok);
            if (peek == '\r') {
                peek = tok_nextc(tok);
            }
            // Special case when the backslash is right before a curly
            // brace. We have to restore and return the control back
            // to the loop for the next iteration.
            if (peek == '{' || peek == '}') {
                if (!current_tok->f_string_raw) {
                    if (_PyTokenizer_warn_invalid_escape_sequence(tok, peek)) {
                        return MAKE_TOKEN(ERRORTOKEN);
                    }
                }
                tok_backup(tok, peek);
                continue;
            }

            if (!current_tok->f_string_raw) {
                if (peek == 'N') {
                    /* Handle named unicode escapes (\N{BULLET}) */
                    peek = tok_nextc(tok);
                    if (peek == '{') {
                        unicode_escape = 1;
                    } else {
                        tok_backup(tok, peek);
                    }
                }
            } /* else {
                skip the escaped character
            }*/
        }
    }

    // Backup the f-string quotes to emit a final FSTRING_MIDDLE and
    // add the quotes to the FSTRING_END in the next tokenizer iteration.
    for (int i = 0; i < current_tok->f_string_quote_size; i++) {
        tok_backup(tok, current_tok->f_string_quote);
    }
    p_start = tok->start;
    p_end = tok->cur;
    return MAKE_TOKEN(FSTRING_MIDDLE);
}

static int
tok_get(struct tok_state *tok, struct token *token)
{
    tokenizer_mode *current_tok = TOK_GET_MODE(tok);
    if (current_tok->kind == TOK_REGULAR_MODE) {
        return tok_get_normal_mode(tok, current_tok, token);
    } else {
        return tok_get_fstring_mode(tok, current_tok, token);
    }
}

int
_PyTokenizer_Get(struct tok_state *tok, struct token *token)
{
    int result = tok_get(tok, token);
    if (tok->decoding_erred) {
        result = ERRORTOKEN;
        tok->done = E_DECODE;
    }
    return result;
}
