#include "Python.h"
#include "pycore_token.h"
#include "errcode.h"

#include "lexer_internal.h"
#include "../tokenizer/helpers.h"

#define MAKE_TOKEN(token_type) _PyLexer_token_setup(tok, token, token_type, p_start, p_end)

static void
rewind_to_string_start(struct tok_state *tok, const char *start,
                       _PyTok_Loc location)
{
    tok->cur = (char *)start + 1;
    tok->line_start = start - location.byte_col;
    tok->lineno = location.lineno;
}

int
_PyLexer_record_ftstring_comment(struct tok_state *tok, const char *start,
                                 const char *end)
{
    tokenizer_mode *mode = TOK_GET_MODE(tok);
    if (mode->expr_span.end >= 0) {
        return 0;
    }
    assert(mode->expr_span.start >= 0);
    tokenizer_comments *comments = mode->comments;
    if (comments == NULL || comments->count == comments->capacity) {
        int create = comments == NULL;
        Py_ssize_t max_capacity = (PY_SSIZE_T_MAX -
            (Py_ssize_t)sizeof(*comments)) /
            (Py_ssize_t)sizeof(*comments->spans);
        if (comments != NULL && comments->capacity > max_capacity / 2) {
            PyErr_NoMemory();
            return -1;
        }
        Py_ssize_t capacity = comments == NULL ? 4 : comments->capacity * 2;
        size_t size = sizeof(*comments) +
            (size_t)capacity * sizeof(*comments->spans);
        tokenizer_comments *resized = PyMem_Realloc(comments, size);
        if (resized == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        comments = resized;
        if (create) {
            comments->count = 0;
        }
        comments->capacity = capacity;
        mode->comments = comments;
    }
    comments->spans[comments->count++] =
        _PyLexer_BufferSpan(tok, start, end);
    return 0;
}

int
_PyLexer_set_ftstring_expr_metadata(struct tok_state *tok, struct token *token)
{
    assert(token != NULL);
    tokenizer_mode *tok_mode = TOK_GET_MODE(tok);

    if (!(tok_mode->in_debug || tok_mode->string_kind == TSTRING) || token->metadata) {
        return 0;
    }
    Py_ssize_t expr_len;
    const char *expr = _PyLexer_BufferSpanView(
        tok, tok_mode->expr_span, &expr_len);
    tokenizer_comments *comments = tok_mode->comments;
    PyObject *res;
    if (comments != NULL && comments->count > 0) {
        Py_ssize_t stripped_size = expr_len;
        _PyTok_Off previous_end = tok_mode->expr_span.start;
        Py_ssize_t comment_count = 0;
        for (Py_ssize_t i = 0; i < comments->count; i++) {
            _PyTok_Span comment = comments->spans[i];
            assert(_PyTok_SpanIsValid(comment));
            assert(comment.start >= previous_end);
            if (comment.start >= tok_mode->expr_span.end) {
                break;
            }
            assert(comment.end <= tok_mode->expr_span.end);
            stripped_size -= comment.end - comment.start;
            previous_end = comment.end;
            comment_count++;
        }
        char *stripped = PyMem_Malloc((size_t)stripped_size);
        if (stripped == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        _PyTok_Off copied_to = tok_mode->expr_span.start;
        Py_ssize_t stripped_len = 0;
        for (Py_ssize_t i = 0; i < comment_count; i++) {
            _PyTok_Span comment = comments->spans[i];
            Py_ssize_t length = comment.start - copied_to;
            memcpy(stripped + stripped_len,
                   expr + copied_to - tok_mode->expr_span.start,
                   (size_t)length);
            stripped_len += length;
            copied_to = comment.end;
        }
        Py_ssize_t length = tok_mode->expr_span.end - copied_to;
        memcpy(stripped + stripped_len,
               expr + copied_to - tok_mode->expr_span.start,
               (size_t)length);
        stripped_len += length;
        res = PyUnicode_DecodeUTF8(stripped, stripped_len, NULL);
        PyMem_Free(stripped);
    }
    else {
        res = PyUnicode_DecodeUTF8(expr, expr_len, NULL);
    }

    if (!res) {
        return -1;
    }
    token->metadata = res;
    return 0;
}

void
_PyLexer_update_ftstring_expr(struct tok_state *tok, char cur)
{
    tokenizer_mode *tok_mode = TOK_GET_MODE(tok);

    switch (cur) {
        case '{':
            tok_mode->expr_span = (_PyTok_Span){
                _PyLexer_BufferOffset(tok, tok->cur), -1};
            tokenizer_comments *comments = tok_mode->comments;
            if (comments != NULL) {
                comments->count = 0;
            }
            break;
        case '}':
        case '!':
            tok_mode->expr_span.end = _PyLexer_BufferOffset(tok, tok->start);
            break;
        case ':':
            if (tok_mode->expr_span.end < 0) {
                tok_mode->expr_span.end =
                    _PyLexer_BufferOffset(tok, tok->start);
            }
            break;
        default:
            Py_UNREACHABLE();
    }
}

int
_PyLexer_check_string_prefixes(struct tok_state *tok,
                                             int saw_b, int saw_r, int saw_u,
                                             int saw_f, int saw_t) {
    // Supported: rb, rf, rt (in any order)
    // Unsupported: ub, ur, uf, ut, bf, bt, ft (in any order)

#define RETURN_SYNTAX_ERROR(PREFIX1, PREFIX2)                             \
    do {                                                                  \
        (void)_PyTokenizer_syntaxerror_known_range(                       \
            tok, (int)(tok->start + 1 - tok->line_start),                 \
            (int)(tok->cur - tok->line_start),                            \
            "'" PREFIX1 "' and '" PREFIX2 "' prefixes are incompatible"); \
        return -1;                                                        \
    } while (0)

    if (saw_u && saw_b) {
        RETURN_SYNTAX_ERROR("u", "b");
    }
    if (saw_u && saw_r) {
        RETURN_SYNTAX_ERROR("u", "r");
    }
    if (saw_u && saw_f) {
        RETURN_SYNTAX_ERROR("u", "f");
    }
    if (saw_u && saw_t) {
        RETURN_SYNTAX_ERROR("u", "t");
    }

    if (saw_b && saw_f) {
        RETURN_SYNTAX_ERROR("b", "f");
    }
    if (saw_b && saw_t) {
        RETURN_SYNTAX_ERROR("b", "t");
    }

    if (saw_f && saw_t) {
        RETURN_SYNTAX_ERROR("f", "t");
    }

#undef RETURN_SYNTAX_ERROR

    return 0;
}

int
_PyLexer_scan_fstring_start(struct tok_state *tok, struct token *token, int c)
{
    const char *p_start = NULL;
    const char *p_end = NULL;

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
        return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok, "too many nested f-strings or t-strings"));
    }
    tokenizer_mode *the_current_tok = TOK_NEXT_MODE(tok);
    the_current_tok->kind = TOK_FSTRING_MODE;
    the_current_tok->quote = quote;
    the_current_tok->quote_size = quote_size;
    the_current_tok->start = _PyLexer_BufferOffset(tok, tok->start);
    the_current_tok->multi_line_start =
        _PyLexer_BufferOffset(tok, tok->line_start);
    the_current_tok->first_line = tok->lineno;
    the_current_tok->expr_span = (_PyTok_Span){-1, -1};
    the_current_tok->in_format_spec = 0;
    the_current_tok->in_debug = 0;
    the_current_tok->comments = NULL;

    enum string_kind_t string_kind = FSTRING;
    switch (*tok->start) {
        case 'T':
        case 't':
            the_current_tok->raw = Py_TOLOWER(*(tok->start + 1)) == 'r';
            string_kind = TSTRING;
            break;
        case 'F':
        case 'f':
            the_current_tok->raw = Py_TOLOWER(*(tok->start + 1)) == 'r';
            break;
        case 'R':
        case 'r':
            the_current_tok->raw = 1;
            if (Py_TOLOWER(*(tok->start + 1)) == 't') {
                string_kind = TSTRING;
            }
            break;
        default:
            Py_UNREACHABLE();
    }

    the_current_tok->string_kind = string_kind;
    the_current_tok->curly_bracket_depth = 0;
    the_current_tok->curly_bracket_expr_start_depth = -1;
    return string_kind == TSTRING ? MAKE_TOKEN(TSTRING_START) : MAKE_TOKEN(FSTRING_START);
}

int
_PyLexer_scan_string(struct tok_state *tok, struct token *token, int c)
{
    const char *p_start = NULL;
    const char *p_end = NULL;

    int quote = c;
    int quote_size = 1;             /* 1 or 3 */
    int end_quote_size = 0;
    int has_escaped_quote = 0;

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
            int end_lineno = tok->lineno;
            rewind_to_string_start(tok, tok->start, tok->start_loc);

            if (INSIDE_FSTRING(tok)) {
                /* When we are in an f-string, before raising the
                 * unterminated string literal error, check whether
                 * does the initial quote matches with f-strings quotes
                 * and if it is, then this must be a missing '}' token
                 * so raise the proper error */
                tokenizer_mode *the_current_tok = TOK_GET_MODE(tok);
                if (the_current_tok->quote == quote &&
                    the_current_tok->quote_size == quote_size) {
                    return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                        "%c-string: expecting '}'", TOK_GET_STRING_PREFIX(tok)));
                }
            }

            if (quote_size == 3) {
                _PyTokenizer_syntaxerror(tok, "unterminated triple-quoted string literal"
                                 " (detected at line %d)", end_lineno);
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
                        end_lineno
                    );
                } else {
                    _PyTokenizer_syntaxerror(
                        tok, "unterminated string literal (detected at line %d)", end_lineno
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

int
_PyLexer_get_fstring_mode(struct tok_state *tok, tokenizer_mode* current_tok, struct token *token)
{
    const char *p_start = NULL;
    const char *p_end = NULL;
    int end_quote_size = 0;
    int unicode_escape = 0;

    tok->start = tok->cur;
    tok->start_loc = (_PyTok_Loc){tok->lineno, _PyLexer_ByteColumn(tok)};

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
                return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                    "%c-string: expressions nested too deeply", TOK_GET_STRING_PREFIX(tok)));
            }
            TOK_GET_MODE(tok)->kind = TOK_REGULAR_MODE;
            return _PyLexer_get_normal_mode(tok, current_tok, token);
        }
    }
    else {
        tok_backup(tok, start_char);
    }

    // Check if we are at the end of the string
    for (int i = 0; i < current_tok->quote_size; i++) {
        int quote = tok_nextc(tok);
        if (quote != current_tok->quote) {
            tok_backup(tok, quote);
            goto f_string_middle;
        }
    }

    p_start = tok->start;
    p_end = tok->cur;
    PyMem_Free(current_tok->comments);
    current_tok->comments = NULL;
    tok->tok_mode_stack_index--;
    return MAKE_TOKEN(FTSTRING_END(current_tok));

f_string_middle:

    // TODO: This is a bit of a hack, but it works for now. We need to find a better way to handle
    // this.
    tok->multi_line_start = tok->line_start;
    while (end_quote_size != current_tok->quote_size) {
        int c = tok_nextc(tok);
        if (tok->done == E_ERROR || tok->done == E_DECODE) {
            return MAKE_TOKEN(ERRORTOKEN);
        }
        int in_format_spec = (
                current_tok->in_format_spec
                &&
                INSIDE_FSTRING_EXPR(current_tok)
        );

       if (c == EOF || (current_tok->quote_size == 1 && c == '\n')) {
            if (tok->input_error) {
                return MAKE_TOKEN(ERRORTOKEN);
            }

            // If we are in a format spec and we found a newline,
            // it means that the format spec ends here and we should
            // return to the regular mode.
            if (in_format_spec && c == '\n') {
                if (current_tok->quote_size == 1) {
                    return MAKE_TOKEN(
                        _PyTokenizer_syntaxerror(
                            tok,
                            "%c-string: newlines are not allowed in format specifiers for single quoted %c-strings",
                            TOK_GET_STRING_PREFIX(tok), TOK_GET_STRING_PREFIX(tok)
                        )
                    );
                }
                tok_backup(tok, c);
                TOK_GET_MODE(tok)->kind = TOK_REGULAR_MODE;
                current_tok->in_format_spec = 0;
                p_start = tok->start;
                p_end = tok->cur;
                return MAKE_TOKEN(FTSTRING_MIDDLE(current_tok));
            }

            assert(tok->multi_line_start != NULL);
            // shift the tok_state's location into
            // the start of string, and report the error
            // from the initial quote character
            tok->cur = _PyLexer_BufferPointer(tok, current_tok->start) + 1;
            tok->line_start = _PyLexer_BufferPointer(
                tok, current_tok->multi_line_start);
            int start = tok->lineno;

            tokenizer_mode *the_current_tok = TOK_GET_MODE(tok);
            tok->lineno = the_current_tok->first_line;

            if (current_tok->quote_size == 3) {
                _PyTokenizer_syntaxerror(tok,
                                    "unterminated triple-quoted %c-string literal"
                                    " (detected at line %d)",
                                    TOK_GET_STRING_PREFIX(tok), start);
                if (c != '\n') {
                    tok->done = E_EOFS;
                }
                return MAKE_TOKEN(ERRORTOKEN);
            }
            else {
                return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                                    "unterminated %c-string literal (detected at"
                                    " line %d)", TOK_GET_STRING_PREFIX(tok), start));
            }
        }

        if (c == current_tok->quote) {
            end_quote_size += 1;
            continue;
        } else {
            end_quote_size = 0;
        }

        if (c == '{') {
            _PyLexer_update_ftstring_expr(tok, c);
            int peek = tok_nextc(tok);
            if (peek != '{' || in_format_spec) {
                tok_backup(tok, peek);
                tok_backup(tok, c);
                current_tok->curly_bracket_expr_start_depth++;
                if (current_tok->curly_bracket_expr_start_depth >= MAX_EXPR_NESTING) {
                    return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                        "%c-string: expressions nested too deeply", TOK_GET_STRING_PREFIX(tok)));
                }
                TOK_GET_MODE(tok)->kind = TOK_REGULAR_MODE;
                current_tok->in_format_spec = 0;
                p_start = tok->start;
                p_end = tok->cur;
            } else {
                p_start = tok->start;
                p_end = tok->cur - 1;
            }
            return MAKE_TOKEN(FTSTRING_MIDDLE(current_tok));
        } else if (c == '}') {
            if (unicode_escape) {
                p_start = tok->start;
                p_end = tok->cur;
                return MAKE_TOKEN(FTSTRING_MIDDLE(current_tok));
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
            return MAKE_TOKEN(FTSTRING_MIDDLE(current_tok));
        } else if (c == '\\') {
            int peek = tok_nextc(tok);
            if (peek == '\r') {
                peek = tok_nextc(tok);
            }
            // Special case when the backslash is right before a curly
            // brace. We have to restore and return the control back
            // to the loop for the next iteration.
            if (peek == '{' || peek == '}') {
                if (!current_tok->raw) {
                    if (_PyTokenizer_warn_invalid_escape_sequence(tok, peek)) {
                        return MAKE_TOKEN(ERRORTOKEN);
                    }
                }
                tok_backup(tok, peek);
                continue;
            }

            if (!current_tok->raw) {
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
    for (int i = 0; i < current_tok->quote_size; i++) {
        tok_backup(tok, current_tok->quote);
    }
    p_start = tok->start;
    p_end = tok->cur;
    return MAKE_TOKEN(FTSTRING_MIDDLE(current_tok));
}
