#include "Python.h"
#include "pycore_token.h"
#include "errcode.h"

#include "lexer_internal.h"
#include "../tokenizer/helpers.h"

#define MAKE_TOKEN(token_type) _PyLexer_token_setup(tok, token, token_type, p_start, p_end)

static int
string_error_token(struct tok_state *tok, struct token *token,
                   _PyTok_Off start, _PyTok_Loc location)
{
    tok->diagnostic = (_PyTokenizer_Diagnostic){
        .location = {location.lineno, location.byte_col + 1},
        .text_span = {start - location.byte_col, tok->inp},
    };
    int type = _PyLexer_token_setup(tok, token, ERRORTOKEN, -1, -1);
    token->start_loc = location;
    token->end_loc = (_PyTok_Loc){location.lineno, -1};
    return type;
}

int
_PyLexer_record_ftstring_comment(struct tok_state *tok, ftstring_state *state,
                                 _PyTok_Off start, _PyTok_Off end)
{
    assert(state == _PyLexer_CurrentFTString(tok) && state->mode == FTSTRING_MODE_EXPRESSION);
    if (state->expr_span.end >= 0) {
        return 0;
    }
    assert(state->expr_span.start >= 0);
    tokenizer_comments *comments = state->comments;
    if (comments == NULL || comments->count == comments->capacity) {
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
        if (state->comments == NULL) {
            comments->count = 0;
        }
        comments->capacity = capacity;
        state->comments = comments;
    }
    comments->spans[comments->count++] =
        _PyTok_SpanFromBounds(start, end);
    return 0;
}

int
_PyLexer_finish_ftstring_expr(struct tok_state *tok, ftstring_state *state,
                              struct token *token)
{
    assert(token != NULL && state == _PyLexer_CurrentFTString(tok));
    assert(state->mode == FTSTRING_MODE_EXPRESSION && tok->start >= 0);

    if (state->expr_span.end >= 0) {
        return 0;
    }
    assert(state->expr_span.start >= 0);
    state->expr_span.end = tok->start;
    int tstring_interpolation = _PyLexer_IsTString(state->kind) &&
        state->replacement_depth == 1;
    if (!(state->debug_expr || tstring_interpolation) || token->metadata) {
        return 0;
    }
    Py_ssize_t expr_len;
    const char *expr = _PyLexer_BufferSpanView(
        tok, state->expr_span, &expr_len);
    tokenizer_comments *comments = state->comments;
    PyObject *res;
    if (comments != NULL && comments->count > 0) {
        Py_ssize_t stripped_size = expr_len;
        _PyTok_Off previous_end = state->expr_span.start;
        Py_ssize_t comment_count = 0;
        for (Py_ssize_t i = 0; i < comments->count; i++) {
            _PyTok_Span comment = comments->spans[i];
            assert(_PyTok_SpanIsValid(comment));
            assert(comment.start >= previous_end);
            if (comment.start >= state->expr_span.end) {
                break;
            }
            assert(comment.end <= state->expr_span.end);
            stripped_size -= comment.end - comment.start;
            previous_end = comment.end;
            comment_count++;
        }
        char *stripped = PyMem_Malloc((size_t)stripped_size);
        if (stripped == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        _PyTok_Off copied_to = state->expr_span.start;
        Py_ssize_t stripped_len = 0;
        for (Py_ssize_t i = 0; i < comment_count; i++) {
            _PyTok_Span comment = comments->spans[i];
            Py_ssize_t length = comment.start - copied_to;
            memcpy(stripped + stripped_len,
                   expr + copied_to - state->expr_span.start,
                   (size_t)length);
            stripped_len += length;
            copied_to = comment.end;
        }
        Py_ssize_t length = state->expr_span.end - copied_to;
        memcpy(stripped + stripped_len,
               expr + copied_to - state->expr_span.start,
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
    _PyTok_Off p_start = -1;
    _PyTok_Off p_end = -1;

    int quote = c;
    int quote_size = 1;             /* 1 or 3 */

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
    ftstring_state *state = _PyLexer_PushFTString(tok);
    if (state == NULL) {
        return MAKE_TOKEN(ERRORTOKEN);
    }
    state->mode = FTSTRING_MODE_MIDDLE;
    state->quote = quote;
    state->quote_size = quote_size;
    state->paren_level = tok->level;
    state->start = tok->start;
    state->start_loc = tok->start_loc;
    state->expr_span = (_PyTok_Span){-1, -1};

    int raw = 0;
    int tstring = 0;
    const char *prefix = _PyLexer_BufferPointer(tok, tok->start);
    switch (*prefix) {
        case 'T':
        case 't':
            raw = Py_TOLOWER(prefix[1]) == 'r';
            tstring = 1;
            break;
        case 'F':
        case 'f':
            raw = Py_TOLOWER(prefix[1]) == 'r';
            break;
        case 'R':
        case 'r':
            raw = 1;
            tstring = Py_TOLOWER(prefix[1]) == 't';
            break;
        default:
            Py_UNREACHABLE();
    }
    state->kind = tstring
        ? (raw ? RAW_TSTRING : TSTRING)
        : (raw ? RAW_FSTRING : FSTRING);
    return tstring ? MAKE_TOKEN(TSTRING_START) : MAKE_TOKEN(FSTRING_START);
}

int
_PyLexer_scan_string(struct tok_state *tok, struct token *token, int c)
{
    _PyTok_Off p_start = -1;
    _PyTok_Off p_end = -1;

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
            _PyTok_Loc location = tok->start_loc;
            const char *line = _PyLexer_BufferPointer(
                tok, tok->start - location.byte_col);
            Py_ssize_t cursor_offset = (Py_ssize_t)location.byte_col + 1;

            const ftstring_state *state = _PyLexer_CurrentFTString(tok);
            if (state != NULL) {
                /* A matching quote belongs to the surrounding formatted
                 * string, so the expression is missing its closing brace. */
                if (state->quote == quote && state->quote_size == quote_size) {
                    _PyTokenizer_syntaxerror_at(
                        tok, line, cursor_offset, location.lineno, -1, -1,
                        "%c-string: expecting '}'",
                        _PyLexer_StringPrefix(state->kind));
                    return string_error_token(tok, token, tok->start, location);
                }
            }

            if (quote_size == 3) {
                _PyTokenizer_syntaxerror_at(
                    tok, line, cursor_offset, location.lineno, -1, -1,
                    "unterminated triple-quoted string literal"
                    " (detected at line %d)", end_lineno);
                if (c != '\n') {
                    tok->done = E_EOFS;
                }
                return string_error_token(tok, token, tok->start, location);
            }
            else {
                if (has_escaped_quote) {
                    _PyTokenizer_syntaxerror_at(
                        tok, line, cursor_offset, location.lineno, -1, -1,
                        "unterminated string literal (detected at line %d); "
                        "perhaps you escaped the end quote?",
                        end_lineno
                    );
                } else {
                    _PyTokenizer_syntaxerror_at(
                        tok, line, cursor_offset, location.lineno, -1, -1,
                        "unterminated string literal (detected at line %d)", end_lineno
                    );
                }
                if (c != '\n') {
                    tok->done = E_EOLS;
                }
                return string_error_token(tok, token, tok->start, location);
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
_PyLexer_get_ftstring(struct tok_state *tok, ftstring_state *current, struct token *token)
{
    assert(current == _PyLexer_CurrentFTString(tok) && current->mode != FTSTRING_MODE_EXPRESSION);
    assert((current->quote_size == 1 || current->quote_size == 3) &&
           current->replacement_depth <= MAX_EXPR_NESTING);
    _PyTok_Off p_start = -1;
    _PyTok_Off p_end = -1;
    int end_quote_size = 0;
    int unicode_escape = 0;
    int quote = current->quote;
    int quote_size = current->quote_size;
    int in_format_spec = current->mode == FTSTRING_MODE_FORMAT_SPEC;
    int raw = _PyLexer_IsRawString(current->kind);
    int token_type;

    tok->start = tok->cur;
    tok->start_loc = (_PyTok_Loc){tok->lineno, _PyLexer_ByteColumn(tok)};

    while (end_quote_size != quote_size) {
        int c = tok_nextc(tok);
        if (tok->done == E_ERROR || tok->done == E_DECODE) {
            return MAKE_TOKEN(ERRORTOKEN);
        }

        if (c == EOF || (quote_size == 1 && c == '\n')) {
            if (tok_failed(tok)) {
                return MAKE_TOKEN(ERRORTOKEN);
            }

            if (in_format_spec && c == '\n') {
                return MAKE_TOKEN(_PyTokenizer_syntaxerror(
                    tok,
                    "%c-string: newlines are not allowed in format specifiers for single quoted %c-strings",
                    _PyLexer_StringPrefix(current->kind),
                    _PyLexer_StringPrefix(current->kind)));
            }

            int end_lineno = tok->lineno;
            _PyTok_Loc location = current->start_loc;
            const char *line = _PyLexer_BufferPointer(
                tok, current->start - location.byte_col);
            Py_ssize_t cursor_offset = (Py_ssize_t)location.byte_col + 1;

            if (quote_size == 3) {
                _PyTokenizer_syntaxerror_at(
                    tok, line, cursor_offset, location.lineno, -1, -1,
                    "unterminated triple-quoted %c-string literal"
                    " (detected at line %d)",
                    _PyLexer_StringPrefix(current->kind), end_lineno);
                if (c != '\n') {
                    tok->done = E_EOFS;
                }
                return string_error_token(tok, token, current->start, location);
            }
            else {
                _PyTokenizer_syntaxerror_at(
                    tok, line, cursor_offset, location.lineno, -1, -1,
                    "unterminated %c-string literal (detected at line %d)",
                    _PyLexer_StringPrefix(current->kind), end_lineno);
                return string_error_token(tok, token, current->start, location);
            }
        }

        if (c == quote) {
            end_quote_size += 1;
            continue;
        } else {
            end_quote_size = 0;
        }

        if (c == '{') {
            int peek = tok_nextc(tok);
            if (peek != '{' || in_format_spec) {
                tok_backup(tok, peek);
                current->expr_span = (_PyTok_Span){tok->cur, -1};
                if (current->comments != NULL) {
                    current->comments->count = 0;
                }
                tok_backup(tok, c);
                if (current->replacement_depth >= MAX_EXPR_NESTING) {
                    _PyTokenizer_syntaxerror(
                        tok, "%c-string: expressions nested too deeply",
                        _PyLexer_StringPrefix(current->kind));
                    return MAKE_TOKEN(ERRORTOKEN);
                }
                current->replacement_depth++;
                current->mode = FTSTRING_MODE_EXPRESSION;
                current->debug_expr = 0;
                p_start = tok->start;
                p_end = tok->cur;
                if (p_start == p_end) {
                    return _PyLexer_get_normal(tok, current, token);
                }
            } else {
                p_start = tok->start;
                p_end = tok->cur - 1;
            }
            goto emit_middle;
        } else if (c == '}') {
            if (unicode_escape) {
                p_start = tok->start;
                p_end = tok->cur;
                goto emit_middle;
            }
            int peek = tok_nextc(tok);

            int bracket_depth = _PyLexer_FTStringBracketDepth(tok, current);
            if (peek == '}' && !in_format_spec && bracket_depth == 0) {
                p_start = tok->start;
                p_end = tok->cur - 1;
            }
            else {
                tok_backup(tok, peek);
                if (!in_format_spec && bracket_depth == 0) {
                    if (tok->start == tok->cur - 1) {
                        return MAKE_TOKEN(_PyTokenizer_syntaxerror(
                            tok, "%c-string: single '}' is not allowed",
                            _PyLexer_StringPrefix(current->kind)));
                    }
                    tok_backup(tok, c);
                    p_start = tok->start;
                    p_end = tok->cur;
                    goto emit_middle;
                }
                tok_backup(tok, c);
                current->mode = FTSTRING_MODE_EXPRESSION;
                p_start = tok->start;
                p_end = tok->cur;
            }
            goto emit_middle;
        } else if (c == '\\') {
            int peek = tok_nextc(tok);
            if (peek == '\r') {
                peek = tok_nextc(tok);
            }
            if (peek == '{' || peek == '}') {
                if (!raw) {
                    if (_PyTokenizer_warn_invalid_escape_sequence(tok, peek)) {
                        return MAKE_TOKEN(ERRORTOKEN);
                    }
                }
                tok_backup(tok, peek);
                continue;
            }

            if (!raw) {
                if (peek == 'N') {
                    /* Handle named unicode escapes (\N{BULLET}) */
                    peek = tok_nextc(tok);
                    if (peek == '{') {
                        unicode_escape = 1;
                    } else {
                        tok_backup(tok, peek);
                    }
                }
            }
        }
    }

    p_start = tok->start;
    p_end = tok->cur;
    if (p_end - quote_size == p_start) {
        int end_token = FTSTRING_END(current);
        _PyLexer_PopFTString(tok);
        return MAKE_TOKEN(end_token);
    }
    for (int i = 0; i < quote_size; i++) {
        tok_backup(tok, quote);
    }
    p_end = tok->cur;
emit_middle:
    token_type = MAKE_TOKEN(FTSTRING_MIDDLE(current));
    token->is_raw = raw;
    return token_type;
}
