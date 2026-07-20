#include "Python.h"
#include "pycore_token.h"
#include "errcode.h"

#include "lexer_internal.h"
#include "../tokenizer/helpers.h"

#define MAKE_TOKEN(token_type) _PyLexer_token_setup(tok, token, token_type, p_start, p_end)

int
_PyLexer_set_ftstring_expr(struct tok_state* tok, struct token *token, char c) {
    assert(token != NULL);
    assert(c == '}' || c == ':' || c == '!');
    tokenizer_mode *tok_mode = TOK_GET_MODE(tok);

    if (!(tok_mode->in_debug || tok_mode->string_kind == TSTRING) || token->metadata) {
        return 0;
    }
    PyObject *res = NULL;

    // Look for a # character outside of string literals
    int hash_detected = 0;
    int in_string = 0;
    char quote_char = 0;

    for (Py_ssize_t i = 0; i < tok_mode->last_expr_size - tok_mode->last_expr_end; i++) {
        char ch = tok_mode->last_expr_buffer[i];

        // Skip escaped characters
        if (ch == '\\') {
            i++;
            continue;
        }

        // Handle quotes
        if (ch == '"' || ch == '\'') {
            // The following if/else block works becase there is an off number
            // of quotes in STRING tokens and the lexer only ever reaches this
            // function with valid STRING tokens.
            // For example: """hello"""
            // First quote: in_string = 1
            // Second quote: in_string = 0
            // Third quote: in_string = 1
            if (!in_string) {
                in_string = 1;
                quote_char = ch;
            }
            else if (ch == quote_char) {
                in_string = 0;
            }
            continue;
        }

        // Check for # outside strings
        if (ch == '#' && !in_string) {
            hash_detected = 1;
            break;
        }
    }
    // If we found a # character in the expression, we need to handle comments
    if (hash_detected) {
        // Allocate buffer for processed result
        char *result = (char *)PyMem_Malloc((tok_mode->last_expr_size - tok_mode->last_expr_end + 1) * sizeof(char));
        if (!result) {
            return -1;
        }

        Py_ssize_t i = 0;  // Input position
        Py_ssize_t j = 0;  // Output position
        in_string = 0;     // Whether we're in a string
        quote_char = 0;    // Current string quote char

        // Process each character
        while (i < tok_mode->last_expr_size - tok_mode->last_expr_end) {
            char ch = tok_mode->last_expr_buffer[i];

            // Handle string quotes
            if (ch == '"' || ch == '\'') {
                // See comment above to understand this part
                if (!in_string) {
                    in_string = 1;
                    quote_char = ch;
                } else if (ch == quote_char) {
                    in_string = 0;
                }
                result[j++] = ch;
            }
            // Skip comments
            else if (ch == '#' && !in_string) {
                while (i < tok_mode->last_expr_size - tok_mode->last_expr_end &&
                       tok_mode->last_expr_buffer[i] != '\n') {
                    i++;
                }
                if (i < tok_mode->last_expr_size - tok_mode->last_expr_end) {
                    result[j++] = '\n';
                }
            }
            // Copy other chars
            else {
                result[j++] = ch;
            }
            i++;
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
_PyLexer_update_ftstring_expr(struct tok_state *tok, char cur)
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
            tok_mode->last_expr_end = strlen(tok->start);
            break;
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
    the_current_tok->start = tok->start;
    the_current_tok->multi_line_start = tok->line_start;
    the_current_tok->first_line = tok->lineno;
    the_current_tok->start_offset = -1;
    the_current_tok->multi_line_start_offset = -1;
    the_current_tok->last_expr_buffer = NULL;
    the_current_tok->last_expr_size = 0;
    the_current_tok->last_expr_end = -1;
    the_current_tok->in_format_spec = 0;
    the_current_tok->in_debug = 0;

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
                if (the_current_tok->quote == quote &&
                    the_current_tok->quote_size == quote_size) {
                    return MAKE_TOKEN(_PyTokenizer_syntaxerror(tok,
                        "%c-string: expecting '}'", TOK_GET_STRING_PREFIX(tok)));
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

int
_PyLexer_get_fstring_mode(struct tok_state *tok, tokenizer_mode* current_tok, struct token *token)
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

    if (current_tok->last_expr_buffer != NULL) {
        PyMem_Free(current_tok->last_expr_buffer);
        current_tok->last_expr_buffer = NULL;
        current_tok->last_expr_size = 0;
        current_tok->last_expr_end = -1;
    }

    p_start = tok->start;
    p_end = tok->cur;
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
            if (tok->decoding_erred) {
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
            tok->cur = (char *)current_tok->start;
            tok->cur++;
            tok->line_start = current_tok->multi_line_start;
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
            if (!_PyLexer_update_ftstring_expr(tok, c)) {
                return MAKE_TOKEN(ENDMARKER);
            }
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
