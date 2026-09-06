#include "Python.h"
#include "pycore_token.h"
#include "pycore_unicodeobject.h"
#include "errcode.h"

#include "lexer_internal.h"
#include "../tokenizer/helpers.h"
#include "../tokenizer/reader.h"

#define MAKE_TOKEN(token_type) _PyLexer_token_setup(tok, token, token_type, p_start, p_end)

/* Spaces in this constant are treated as "zero or more spaces or tabs" when
   tokenizing. */
static const char* type_comment_prefix = "# type: ";

static inline int
contains_null_bytes(const char* str, size_t size)
{
    return memchr(str, 0, size) != NULL;
}

int
_PyLexer_refill(struct tok_state *tok)
{
    if (tok->done != E_OK) {
        return 0;
    }
    int rc = _PyTok_ReaderUnderflow(tok);
#if defined(Py_DEBUG)
    if (tok->debug) {
        fprintf(stderr, "line[%d] = ", tok->lineno);
        _PyTokenizer_print_escape(stderr, _PyLexer_BufferPointer(tok, tok->cur),
                                  tok->inp - tok->cur);
        fprintf(stderr, "  tok->done = %d\n", tok->done);
    }
#endif
    if (!rc) {
        tok->cur = tok->inp;
        return 0;
    }
    tok->line_start = tok->cur;
    if (contains_null_bytes(_PyLexer_BufferPointer(tok, tok->line_start),
                            tok->inp - tok->line_start)) {
        _PyTokenizer_syntaxerror(tok, "source code cannot contain null bytes");
        tok->cur = tok->inp;
        return 0;
    }
    return 1;
}

/* Back-up one character */
void
_PyLexer_backup(struct tok_state *tok, int c)
{
    if (c != EOF) {
        if (--tok->cur < tok->buf_offset) {
            Py_FatalError("tokenizer beginning of buffer");
        }
        if ((int)(unsigned char)*_PyLexer_BufferPointer(tok, tok->cur) != Py_CHARMASK(c)) {
            Py_FatalError("tok_backup: wrong character");
        }
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
    if (tok_failed(tok))
        return 0;
    s = PyUnicode_DecodeUTF8(_PyLexer_BufferPointer(tok, tok->start), tok->cur - tok->start, NULL);
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
        const char *error_cursor = tok->cur;
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
            error_cursor = tok->start + PyBytes_GET_SIZE(s);
        }
        Py_DECREF(s);
        if (Py_UNICODE_ISPRINTABLE(ch)) {
            _PyTokenizer_syntaxerror_at(
                tok, tok->line_start,
                error_cursor - tok->line_start, tok->lineno, -1, -1,
                "invalid character '%c' (U+%04X)", ch, ch);
        }
        else {
            _PyTokenizer_syntaxerror_at(
                tok, tok->line_start,
                error_cursor - tok->line_start, tok->lineno, -1, -1,
                "invalid non-printable character U+%04X", ch);
        }
        return 0;
    }
    Py_DECREF(s);
    return 1;
}

int
_PyLexer_get_normal(struct tok_state *tok, ftstring_state *current, struct token *token)
{
    assert(current == NULL ||
           (current->mode == FTSTRING_MODE_EXPRESSION &&
            current->replacement_depth > 0));
    int c;
    int blankline, nonascii;

    _PyTok_Off p_start = -1;
    _PyTok_Off p_end = -1;
  nextline:
    tok->start = -1;
    tok->start_loc = (_PyTok_Loc){tok->lineno, -1};
    blankline = 0;


    if (tok->layout.at_bol) {
        blankline = _PyLexer_BeginLine(tok);
        if (blankline < 0) {
            return MAKE_TOKEN(ERRORTOKEN);
        }
    }

    tok->start = tok->cur;
    tok->start_loc = (_PyTok_Loc){
        tok->lineno, tok->line_start >= 0 ? _PyLexer_ByteColumn(tok) : -1};

    if (tok->layout.pending != 0) {
        return _PyLexer_IndentationToken(tok, token);
    }

    /* Peek ahead at the next character */
    c = tok_nextc(tok);
    tok_backup(tok, c);

 again:
    tok->start = -1;
    /* Skip spaces */
    do {
        c = tok_nextc(tok);
    } while (c == ' ' || c == '\t' || c == '\014');

    /* Set start of current token */
    tok->start = tok->cur - 1;
    tok->start_loc = (_PyTok_Loc){
        tok->lineno, tok->line_start >= 0 ? _PyLexer_ByteColumn(tok) - 1 : -1};

    /* Skip comment, unless it's a type comment */
    if (c == '#') {

        const char* p = NULL;
        const char *prefix, *type_start;
        int current_starting_col_offset;

        while (c != EOF && c != '\n' && c != '\r') {
            c = tok_nextc(tok);
        }

        if (current != NULL) {
            _PyTok_Off comment_end = tok->cur;
            if (c == '\n' || c == '\r') {
                comment_end--;
            }
            if (_PyLexer_record_ftstring_comment(
                    tok, current, tok->start, comment_end) < 0) {
                tok->done = E_NOMEM;
                return MAKE_TOKEN(ERRORTOKEN);
            }
        }

        if (tok->tok_extra_tokens) {
            p = _PyLexer_BufferPointer(tok, tok->start);
        }

        if (tok->type_comments) {
            p = _PyLexer_BufferPointer(tok, tok->start);
            current_starting_col_offset = tok->start_loc.byte_col;
            prefix = type_comment_prefix;
            while (*prefix && p < _PyLexer_BufferPointer(tok, tok->cur)) {
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
                    _PyLexer_BufferPointer(tok, tok->cur) >= ignore_end && memcmp(p, "ignore", 6) == 0
                    && !(_PyLexer_BufferPointer(tok, tok->cur) > ignore_end
                         && ((unsigned char)ignore_end[0] >= 128 || Py_ISALNUM(ignore_end[0]))));

                int type = is_type_ignore ? TYPE_IGNORE : TYPE_COMMENT;
                int start_col_offset = is_type_ignore
                    ? ignore_end_col_offset : current_starting_col_offset;
                p_end = tok->cur;
                if (is_type_ignore) {
                    p_start = _PyLexer_BufferOffset(tok, ignore_end);

                    /* If this type ignore is the only thing on the line, consume the newline also. */
                    if (blankline) {
                        tok_nextc(tok);
                        tok->layout.at_bol = 1;
                    }
                } else {
                    p_start = _PyLexer_BufferOffset(tok, type_start);
                }
                _PyLexer_token_setup(tok, token, type, p_start, p_end);
                token->start_loc = (_PyTok_Loc){tok->lineno, start_col_offset};
                token->end_loc = (_PyTok_Loc){tok->lineno,
                                              _PyLexer_ByteColumn(tok)};
                return type;
            }
        }
        if (tok->tok_extra_tokens) {
            tok_backup(tok, c);  /* don't eat the newline or EOF */
            p_start = _PyLexer_BufferOffset(tok, p);
            p_end = tok->cur;
            tok->layout.comment_newline = blankline;
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

    if (c == '\n') {
        int type = _PyLexer_Newline(tok, token, blankline);
        if (type == 0) {
            goto nextline;
        }
        return type;
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
        if ((c = _PyLexer_ContinueLine(tok)) == -1) {
            return MAKE_TOKEN(ERRORTOKEN);
        }
        goto again; /* Read next line */
    }

    /* Punctuation character */
    int is_punctuation = (c == ':' || c == '}' || c == '!');
    if (is_punctuation && current != NULL) {
        int type = _PyLexer_ftstring_punctuation(tok, current, token, c);
        if (type < 0) {
            return MAKE_TOKEN(ERRORTOKEN);
        }
        if (type != 0) {
            p_start = tok->start;
            p_end = tok->cur;
            return MAKE_TOKEN(type);
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
        break;
    case ')':
    case ']':
    case '}':
        if (current != NULL &&
                _PyLexer_FTStringBracketDepth(tok, current) == 0) {
            if (c == '}') {
                return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                    "%c-string: single '}' is not allowed",
                    _PyLexer_StringPrefix(current->kind)));
            }
            return MAKE_TOKEN(_PyTokenizer_syntaxerror(
                tok, "%c-string: unmatched '%c'",
                _PyLexer_StringPrefix(current->kind), c));
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
                /* Do not match a closer against the brace that opened the
                 * current replacement field. */
                if (current != NULL && opening == '{') {
                    int bracket_depth =
                        _PyLexer_FTStringBracketDepth(tok, current);
                    if (bracket_depth == current->replacement_depth - 1) {
                        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                            "%c-string: unmatched '%c'",
                            _PyLexer_StringPrefix(current->kind), c));
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
        if (current != NULL &&
                _PyLexer_close_ftstring_expr(tok, current, c) < 0) {
            return MAKE_TOKEN(ERRORTOKEN);
        }
        break;
    default:
        break;
    }

    if (!Py_UNICODE_ISPRINTABLE(c)) {
        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "invalid non-printable character U+%04X", c));
    }

    if (c == '=' && current != NULL) {
        _PyLexer_mark_ftstring_debug(tok, current);
    }

    /* Punctuation character */
    p_start = tok->start;
    p_end = tok->cur;
    return MAKE_TOKEN(_PyToken_OneChar(c));
}


void
_PyTokenizer_Get(struct tok_state *tok, struct token *token)
{
    _PyToken_Free(token);
    ftstring_state *current = _PyLexer_CurrentFTString(tok);
    int result;
    if (current == NULL) {
        result = _PyLexer_get_normal(tok, NULL, token);
    }
    else {
        switch (current->mode) {
        case FTSTRING_MODE_EXPRESSION:
            result = _PyLexer_get_normal(tok, current, token);
            break;
        case FTSTRING_MODE_MIDDLE:
        case FTSTRING_MODE_FORMAT_SPEC:
            result = _PyLexer_get_ftstring(tok, current, token);
            break;
        default:
            Py_UNREACHABLE();
        }
    }
    if (tok_failed(tok)) {
        result = ERRORTOKEN;
    }
    token->type = result;
}
