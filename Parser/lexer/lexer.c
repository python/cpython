#include "Python.h"
#include "pycore_token.h"
#include "pycore_unicodeobject.h"
#include "errcode.h"

#include "lexer_internal.h"
#include "../tokenizer/helpers.h"

/* Alternate tab spacing */
#define ALTTABSIZE 1


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
int
_PyLexer_nextc(struct tok_state *tok)
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
void
_PyLexer_backup(struct tok_state *tok, int c)
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



/* Verify that the identifier follows PEP 3131. */
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
    assert(invalid >= 0);
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



int
_PyLexer_get_normal_mode(struct tok_state *tok, tokenizer_mode* current_tok, struct token *token)
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
            else if (c == EOF && PyErr_Occurred()) {
                return MAKE_TOKEN(ERRORTOKEN);
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
        int saw_b = 0, saw_r = 0, saw_u = 0, saw_f = 0, saw_t = 0;
        while (1) {
            if (!saw_b && (c == 'b' || c == 'B')) {
                saw_b = 1;
            }
            /* Since this is a backwards compatibility support literal we don't
               want to support it in arbitrary order like byte literals. */
            else if (!saw_u && (c == 'u'|| c == 'U')) {
                saw_u = 1;
            }
            /* ur"" and ru"" are not supported */
            else if (!saw_r && (c == 'r' || c == 'R')) {
                saw_r = 1;
            }
            else if (!saw_f && (c == 'f' || c == 'F')) {
                saw_f = 1;
            }
            else if (!saw_t && (c == 't' || c == 'T')) {
                saw_t = 1;
            }
            else {
                break;
            }
            c = tok_nextc(tok);
            if (c == '"' || c == '\'') {
                // Raise error on incompatible string prefixes:
                int status = _PyLexer_check_string_prefixes(
                    tok, saw_b, saw_r, saw_u, saw_f, saw_t);
                if (status < 0) {
                    return MAKE_TOKEN(ERRORTOKEN);
                }

                // Handle valid f or t string creation:
                if (saw_f || saw_t) {
                    return _PyLexer_scan_fstring_start(tok, token, c);
                }
                return _PyLexer_scan_string(tok, token, c);
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
            return _PyLexer_scan_number(tok, token, c, 1);
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
        return _PyLexer_scan_number(tok, token, c, 0);
    }

    /* String */
    if (c == '\'' || c == '"') {
        return _PyLexer_scan_string(tok, token, c);
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
             cursor == 1 && (current_tok->in_debug || in_format_spec);
         int cursor_valid = cursor == 0 || cursor_in_format_with_debug;
        if ((cursor_valid) && !_PyLexer_update_ftstring_expr(tok, c)) {
            return MAKE_TOKEN(ENDMARKER);
        }
        if ((cursor_valid) && c != '{' && _PyLexer_set_ftstring_expr(tok, token, c)) {
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
            return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                "%c-string: single '}' is not allowed", TOK_GET_STRING_PREFIX(tok)));
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
                        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                            "%c-string: unmatched '%c'", TOK_GET_STRING_PREFIX(tok), c));
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
                return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "%c-string: unmatched '%c'",
                    TOK_GET_STRING_PREFIX(tok), c));
            }
            if (c == '}' && current_tok->curly_bracket_depth == current_tok->curly_bracket_expr_start_depth) {
                current_tok->curly_bracket_expr_start_depth--;
                current_tok->kind = TOK_FSTRING_MODE;
                current_tok->in_format_spec = 0;
                current_tok->in_debug = 0;
            }
        }
        break;
    default:
        break;
    }

    if (!Py_UNICODE_ISPRINTABLE(c)) {
        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "invalid non-printable character U+%04X", c));
    }

    if( c == '=' && INSIDE_FSTRING_EXPR_AT_TOP(current_tok)) {
        current_tok->in_debug = 1;
    }

    /* Punctuation character */
    p_start = tok->start;
    p_end = tok->cur;
    return MAKE_TOKEN(_PyToken_OneChar(c));
}


static int
tok_get(struct tok_state *tok, struct token *token)
{
    tokenizer_mode *current_tok = TOK_GET_MODE(tok);
    if (current_tok->kind == TOK_REGULAR_MODE) {
        return _PyLexer_get_normal_mode(tok, current_tok, token);
    } else {
        return _PyLexer_get_fstring_mode(tok, current_tok, token);
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
