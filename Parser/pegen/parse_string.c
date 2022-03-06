#include <Python.h>

#include "../tokenizer.h"
#include "pegen.h"
#include "parse_string.h"

//// STRING HANDLING FUNCTIONS ////

// These functions are ported directly from Python/ast.c with some modifications
// to account for the use of "Parser *p", the fact that don't have parser nodes
// to pass around and the usage of some specialized APIs present only in this
// file (like "_PyPegen_raise_syntax_error").

static int
warn_invalid_escape_sequence(Parser *p, unsigned char first_invalid_escape_char, Token *t)
{
    PyObject *msg =
        PyUnicode_FromFormat("invalid escape sequence \\%c", first_invalid_escape_char);
    if (msg == NULL) {
        return -1;
    }
    if (PyErr_WarnExplicitObject(PyExc_DeprecationWarning, msg, p->tok->filename,
                                 t->lineno, NULL, NULL) < 0) {
        if (PyErr_ExceptionMatches(PyExc_DeprecationWarning)) {
            /* Replace the DeprecationWarning exception with a SyntaxError
               to get a more accurate error report */
            PyErr_Clear();

            /* This is needed, in order for the SyntaxError to point to the token t,
               since _PyPegen_raise_error uses p->tokens[p->fill - 1] for the
               error location, if p->known_err_token is not set. */
            p->known_err_token = t;
            RAISE_SYNTAX_ERROR("invalid escape sequence \\%c", first_invalid_escape_char);
        }
        Py_DECREF(msg);
        return -1;
    }
    Py_DECREF(msg);
    return 0;
}

static PyObject *
decode_utf8(const char **sPtr, const char *end)
{
    const char *s, *t;
    t = s = *sPtr;
    while (s < end && (*s & 0x80)) {
        s++;
    }
    *sPtr = s;
    return PyUnicode_DecodeUTF8(t, s - t, NULL);
}

static PyObject *
decode_unicode_with_escapes(Parser *parser, const char *s, size_t len, Token *t)
{
    PyObject *v, *u;
    char *buf;
    char *p;
    const char *end;

    /* check for integer overflow */
    if (len > SIZE_MAX / 6) {
        return NULL;
    }
    /* "ä" (2 bytes) may become "\U000000E4" (10 bytes), or 1:5
       "\ä" (3 bytes) may become "\u005c\U000000E4" (16 bytes), or ~1:6 */
    u = PyBytes_FromStringAndSize((char *)NULL, len * 6);
    if (u == NULL) {
        return NULL;
    }
    p = buf = PyBytes_AsString(u);
    end = s + len;
    while (s < end) {
        if (*s == '\\') {
            *p++ = *s++;
            if (s >= end || *s & 0x80) {
                strcpy(p, "u005c");
                p += 5;
                if (s >= end) {
                    break;
                }
            }
        }
        if (*s & 0x80) {
            PyObject *w;
            int kind;
            void *data;
            Py_ssize_t len, i;
            w = decode_utf8(&s, end);
            if (w == NULL) {
                Py_DECREF(u);
                return NULL;
            }
            kind = PyUnicode_KIND(w);
            data = PyUnicode_DATA(w);
            len = PyUnicode_GET_LENGTH(w);
            for (i = 0; i < len; i++) {
                Py_UCS4 chr = PyUnicode_READ(kind, data, i);
                sprintf(p, "\\U%08x", chr);
                p += 10;
            }
            /* Should be impossible to overflow */
            assert(p - buf <= PyBytes_GET_SIZE(u));
            Py_DECREF(w);
        }
        else {
            *p++ = *s++;
        }
    }
    len = p - buf;
    s = buf;

    const char *first_invalid_escape;
    v = _PyUnicode_DecodeUnicodeEscape(s, len, NULL, &first_invalid_escape);

    if (v != NULL && first_invalid_escape != NULL) {
        if (warn_invalid_escape_sequence(parser, *first_invalid_escape, t) < 0) {
            /* We have not decref u before because first_invalid_escape points
               inside u. */
            Py_XDECREF(u);
            Py_DECREF(v);
            return NULL;
        }
    }
    Py_XDECREF(u);
    return v;
}

static PyObject *
decode_bytes_with_escapes(Parser *p, const char *s, Py_ssize_t len, Token *t)
{
    const char *first_invalid_escape;
    PyObject *result = _PyBytes_DecodeEscape(s, len, NULL, &first_invalid_escape);
    if (result == NULL) {
        return NULL;
    }

    if (first_invalid_escape != NULL) {
        if (warn_invalid_escape_sequence(p, *first_invalid_escape, t) < 0) {
            Py_DECREF(result);
            return NULL;
        }
    }
    return result;
}

/* s must include the bracketing quote characters, and r, b, u,
   &/or f prefixes (if any), and embedded escape sequences (if any).
   _PyPegen_parsestr parses it, and sets *result to decoded Python string object.
   If the string is an f-string, set *fstr and *fstrlen to the unparsed
   string object.  Return 0 if no errors occurred.  */
int
_PyPegen_parsestr(Parser *p, int *bytesmode, int *rawmode, PyObject **result,
                  const char **fstr, Py_ssize_t *fstrlen, Token *t)
{
    const char *s = PyBytes_AsString(t->bytes);
    if (s == NULL) {
        return -1;
    }

    size_t len;
    int quote = Py_CHARMASK(*s);
    int fmode = 0;
    *bytesmode = 0;
    *rawmode = 0;
    *result = NULL;
    *fstr = NULL;
    if (Py_ISALPHA(quote)) {
        while (!*bytesmode || !*rawmode) {
            if (quote == 'b' || quote == 'B') {
                quote = *++s;
                *bytesmode = 1;
            }
            else if (quote == 'u' || quote == 'U') {
                quote = *++s;
            }
            else if (quote == 'r' || quote == 'R') {
                quote = *++s;
                *rawmode = 1;
            }
            else if (quote == 'f' || quote == 'F') {
                quote = *++s;
                fmode = 1;
            }
            else {
                break;
            }
        }
    }

    /* fstrings are only allowed in Python 3.6 and greater */
    if (fmode && p->feature_version < 6) {
        p->error_indicator = 1;
        RAISE_SYNTAX_ERROR("Format strings are only supported in Python 3.6 and greater");
        return -1;
    }

    if (fmode && *bytesmode) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (quote != '\'' && quote != '\"') {
        PyErr_BadInternalCall();
        return -1;
    }
    /* Skip the leading quote char. */
    s++;
    len = strlen(s);
    if (len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "string to parse is too long");
        return -1;
    }
    if (s[--len] != quote) {
        /* Last quote char must match the first. */
        PyErr_BadInternalCall();
        return -1;
    }
    if (len >= 4 && s[0] == quote && s[1] == quote) {
        /* A triple quoted string. We've already skipped one quote at
           the start and one at the end of the string. Now skip the
           two at the start. */
        s += 2;
        len -= 2;
        /* And check that the last two match. */
        if (s[--len] != quote || s[--len] != quote) {
            PyErr_BadInternalCall();
            return -1;
        }
    }

    if (fmode) {
        /* Just return the bytes. The caller will parse the resulting
           string. */
        *fstr = s;
        *fstrlen = len;
        return 0;
    }

    /* Not an f-string. */
    /* Avoid invoking escape decoding routines if possible. */
    *rawmode = *rawmode || strchr(s, '\\') == NULL;
    if (*bytesmode) {
        /* Disallow non-ASCII characters. */
        const char *ch;
        for (ch = s; *ch; ch++) {
            if (Py_CHARMASK(*ch) >= 0x80) {
                RAISE_SYNTAX_ERROR(
                                   "bytes can only contain ASCII "
                                   "literal characters.");
                return -1;
            }
        }
        if (*rawmode) {
            *result = PyBytes_FromStringAndSize(s, len);
        }
        else {
            *result = decode_bytes_with_escapes(p, s, len, t);
        }
    }
    else {
        if (*rawmode) {
            *result = PyUnicode_DecodeUTF8Stateful(s, len, NULL, NULL);
        }
        else {
            *result = decode_unicode_with_escapes(p, s, len, t);
        }
    }
    return *result == NULL ? -1 : 0;
}



// FSTRING STUFF

static void fstring_shift_expr_locations(expr_ty n, int lineno, int col_offset);
static void fstring_shift_argument(expr_ty parent, arg_ty args, int lineno, int col_offset);


static inline void shift_expr(expr_ty parent, expr_ty n, int line, int col) {
    if (parent->lineno < n->lineno) {
        col = 0;
    }
    fstring_shift_expr_locations(n, line, col);
}

static inline void shift_arg(expr_ty parent, arg_ty n, int line, int col) {
    if (parent->lineno < n->lineno) {
        col = 0;
    }
    fstring_shift_argument(parent, n, line, col);
}

static void fstring_shift_seq_locations(expr_ty parent, asdl_seq *seq, int lineno, int col_offset) {
    for (Py_ssize_t i = 0, l = asdl_seq_LEN(seq); i < l; i++) {
        expr_ty expr = asdl_seq_GET(seq, i);
        if (expr == NULL){
            continue;
        }
        shift_expr(parent, expr, lineno, col_offset);
    }
}

static void fstring_shift_slice_locations(expr_ty parent, expr_ty slice, int lineno, int col_offset) {
    switch (slice->kind) {
        case Slice_kind:
            if (slice->v.Slice.lower) {
                shift_expr(parent, slice->v.Slice.lower, lineno, col_offset);
            }
            if (slice->v.Slice.upper) {
                shift_expr(parent, slice->v.Slice.upper, lineno, col_offset);
            }
            if (slice->v.Slice.step) {
                shift_expr(parent, slice->v.Slice.step, lineno, col_offset);
            }
            break;
        case Tuple_kind:
            fstring_shift_seq_locations(parent, slice->v.Tuple.elts, lineno, col_offset);
            break;
        default:
            break;
    }
}

static void fstring_shift_comprehension(expr_ty parent, comprehension_ty comp, int lineno, int col_offset) {
    shift_expr(parent, comp->target, lineno, col_offset);
    shift_expr(parent, comp->iter, lineno, col_offset);
    fstring_shift_seq_locations(parent, comp->ifs, lineno, col_offset);
}

static void fstring_shift_argument(expr_ty parent, arg_ty arg, int lineno, int col_offset) {
    if (arg->annotation != NULL){
        shift_expr(parent, arg->annotation, lineno, col_offset);
    }
    arg->col_offset = arg->col_offset + col_offset;
    arg->end_col_offset = arg->end_col_offset + col_offset;
    arg->lineno = arg->lineno + lineno;
    arg->end_lineno = arg->end_lineno + lineno;
}

static void fstring_shift_arguments(expr_ty parent, arguments_ty args, int lineno, int col_offset) {
    for (Py_ssize_t i = 0, l = asdl_seq_LEN(args->posonlyargs); i < l; i++) {
       arg_ty arg = asdl_seq_GET(args->posonlyargs, i);
       shift_arg(parent, arg, lineno, col_offset);
    }

    for (Py_ssize_t i = 0, l = asdl_seq_LEN(args->args); i < l; i++) {
       arg_ty arg = asdl_seq_GET(args->args, i);
       shift_arg(parent, arg, lineno, col_offset);
    }

    if (args->vararg != NULL) {
        shift_arg(parent, args->vararg, lineno, col_offset);
    }

    for (Py_ssize_t i = 0, l = asdl_seq_LEN(args->kwonlyargs); i < l; i++) {
       arg_ty arg = asdl_seq_GET(args->kwonlyargs, i);
       shift_arg(parent, arg, lineno, col_offset);
    }

    fstring_shift_seq_locations(parent, args->kw_defaults, lineno, col_offset);

    if (args->kwarg != NULL) {
        shift_arg(parent, args->kwarg, lineno, col_offset);
    }

    fstring_shift_seq_locations(parent, args->defaults, lineno, col_offset);
}

static void fstring_shift_children_locations(expr_ty n, int lineno, int col_offset) {
    switch (n->kind) {
        case BoolOp_kind:
            fstring_shift_seq_locations(n, n->v.BoolOp.values, lineno, col_offset);
            break;
        case NamedExpr_kind:
            shift_expr(n, n->v.NamedExpr.target, lineno, col_offset);
            shift_expr(n, n->v.NamedExpr.value, lineno, col_offset);
            break;
        case BinOp_kind:
            shift_expr(n, n->v.BinOp.left, lineno, col_offset);
            shift_expr(n, n->v.BinOp.right, lineno, col_offset);
            break;
        case UnaryOp_kind:
            shift_expr(n, n->v.UnaryOp.operand, lineno, col_offset);
            break;
        case Lambda_kind:
            fstring_shift_arguments(n, n->v.Lambda.args, lineno, col_offset);
            shift_expr(n, n->v.Lambda.body, lineno, col_offset);
            break;
        case IfExp_kind:
            shift_expr(n, n->v.IfExp.test, lineno, col_offset);
            shift_expr(n, n->v.IfExp.body, lineno, col_offset);
            shift_expr(n, n->v.IfExp.orelse, lineno, col_offset);
            break;
        case Dict_kind:
            fstring_shift_seq_locations(n, n->v.Dict.keys, lineno, col_offset);
            fstring_shift_seq_locations(n, n->v.Dict.values, lineno, col_offset);
            break;
        case Set_kind:
            fstring_shift_seq_locations(n, n->v.Set.elts, lineno, col_offset);
            break;
        case ListComp_kind:
            shift_expr(n, n->v.ListComp.elt, lineno, col_offset);
            for (Py_ssize_t i = 0, l = asdl_seq_LEN(n->v.ListComp.generators); i < l; i++) {
                comprehension_ty comp = asdl_seq_GET(n->v.ListComp.generators, i);
                fstring_shift_comprehension(n, comp, lineno, col_offset);
            }
            break;
        case SetComp_kind:
            shift_expr(n, n->v.SetComp.elt, lineno, col_offset);
            for (Py_ssize_t i = 0, l = asdl_seq_LEN(n->v.SetComp.generators); i < l; i++) {
                comprehension_ty comp = asdl_seq_GET(n->v.SetComp.generators, i);
                fstring_shift_comprehension(n, comp, lineno, col_offset);
            }
            break;
        case DictComp_kind:
            shift_expr(n, n->v.DictComp.key, lineno, col_offset);
            shift_expr(n, n->v.DictComp.value, lineno, col_offset);
            for (Py_ssize_t i = 0, l = asdl_seq_LEN(n->v.DictComp.generators); i < l; i++) {
                comprehension_ty comp = asdl_seq_GET(n->v.DictComp.generators, i);
                fstring_shift_comprehension(n, comp, lineno, col_offset);
            }
            break;
        case GeneratorExp_kind:
            shift_expr(n, n->v.GeneratorExp.elt, lineno, col_offset);
            for (Py_ssize_t i = 0, l = asdl_seq_LEN(n->v.GeneratorExp.generators); i < l; i++) {
                comprehension_ty comp = asdl_seq_GET(n->v.GeneratorExp.generators, i);
                fstring_shift_comprehension(n, comp, lineno, col_offset);
            }
            break;
        case Await_kind:
            shift_expr(n, n->v.Await.value, lineno, col_offset);
            break;
        case Yield_kind:
            shift_expr(n, n->v.Yield.value, lineno, col_offset);
            break;
        case YieldFrom_kind:
            shift_expr(n, n->v.YieldFrom.value, lineno, col_offset);
            break;
        case Compare_kind:
            shift_expr(n, n->v.Compare.left, lineno, col_offset);
            fstring_shift_seq_locations(n, n->v.Compare.comparators, lineno, col_offset);
            break;
        case Call_kind:
            shift_expr(n, n->v.Call.func, lineno, col_offset);
            fstring_shift_seq_locations(n, n->v.Call.args, lineno, col_offset);
            for (Py_ssize_t i = 0, l = asdl_seq_LEN(n->v.Call.keywords); i < l; i++) {
                keyword_ty keyword = asdl_seq_GET(n->v.Call.keywords, i);
                shift_expr(n, keyword->value, lineno, col_offset);
            }
            break;
        case Attribute_kind:
            shift_expr(n, n->v.Attribute.value, lineno, col_offset);
            break;
        case Subscript_kind:
            shift_expr(n, n->v.Subscript.value, lineno, col_offset);
            fstring_shift_slice_locations(n, n->v.Subscript.slice, lineno, col_offset);
            shift_expr(n, n->v.Subscript.slice, lineno, col_offset);
            break;
        case Starred_kind:
            shift_expr(n, n->v.Starred.value, lineno, col_offset);
            break;
        case List_kind:
            fstring_shift_seq_locations(n, n->v.List.elts, lineno, col_offset);
            break;
        case Tuple_kind:
            fstring_shift_seq_locations(n, n->v.Tuple.elts, lineno, col_offset);
            break;
        case JoinedStr_kind:
            fstring_shift_seq_locations(n, n->v.JoinedStr.values, lineno, col_offset);
            break;
        case FormattedValue_kind:
            shift_expr(n, n->v.FormattedValue.value, lineno, col_offset);
            if (n->v.FormattedValue.format_spec) {
                shift_expr(n, n->v.FormattedValue.format_spec, lineno, col_offset);
            }
            break;
        default:
            return;
    }
}

/* Shift locations for the given node and all its children by adding `lineno`
   and `col_offset` to existing locations. Note that n is the already parsed
   expression. */
static void fstring_shift_expr_locations(expr_ty n, int lineno, int col_offset)
{
    n->col_offset = n->col_offset + col_offset;

    // The following is needed, in order for nodes spanning across multiple lines
    // to be shifted correctly. An example of such a node is a Call node, the closing
    // parenthesis of which is not on the same line as its name.
    if (n->lineno == n->end_lineno) {
        n->end_col_offset = n->end_col_offset + col_offset;
    }

    fstring_shift_children_locations(n, lineno, col_offset);
    n->lineno = n->lineno + lineno;
    n->end_lineno = n->end_lineno + lineno;
}

/* Fix locations for the given node and its children.

   `parent` is the enclosing node.
   `n` is the node which locations are going to be fixed relative to parent.
   `expr_str` is the child node's string representation, including braces.
*/
static void
fstring_fix_expr_location(Token *parent, expr_ty n, char *expr_str)
{
    char *substr = NULL;
    char *start;
    int lines = 0;
    int cols = 0;

    if (parent && parent->bytes) {
        char *parent_str = PyBytes_AsString(parent->bytes);
        if (!parent_str) {
            return;
        }
        substr = strstr(parent_str, expr_str);
        if (substr) {
            // The following is needed, in order to correctly shift the column
            // offset, in the case that (disregarding any whitespace) a newline
            // immediately follows the opening curly brace of the fstring expression.
            int newline_after_brace = 1;
            start = substr + 1;
            while (start && *start != '}' && *start != '\n') {
                if (*start != ' ' && *start != '\t' && *start != '\f') {
                    newline_after_brace = 0;
                    break;
                }
                start++;
            }

            // Account for the characters from the last newline character to our
            // left until the beginning of substr.
            if (!newline_after_brace) {
                start = substr;
                while (start > parent_str && *start != '\n') {
                    start--;
                }
                cols += (int)(substr - start);
            }
            /* adjust the start based on the number of newlines encountered
               before the f-string expression */
            for (char* p = parent_str; p < substr; p++) {
                if (*p == '\n') {
                    lines++;
                }
            }
        }
    }
    fstring_shift_expr_locations(n, lines, cols);
}


/* Compile this expression in to an expr_ty.  Add parens around the
   expression, in order to allow leading spaces in the expression. */
static expr_ty
fstring_compile_expr(Parser *p, const char *expr_start, const char *expr_end,
                     Token *t)
{
    expr_ty expr = NULL;
    char *str;
    Py_ssize_t len;
    const char *s;
    expr_ty result = NULL;

    assert(expr_end >= expr_start);
    assert(*(expr_start-1) == '{');
    assert(*expr_end == '}' || *expr_end == '!' || *expr_end == ':' ||
           *expr_end == '=');

    /* If the substring is all whitespace, it's an error.  We need to catch this
       here, and not when we call PyParser_SimpleParseStringFlagsFilename,
       because turning the expression '' in to '()' would go from being invalid
       to valid. */
    for (s = expr_start; s != expr_end; s++) {
        char c = *s;
        /* The Python parser ignores only the following whitespace
           characters (\r already is converted to \n). */
        if (!(c == ' ' || c == '\t' || c == '\n' || c == '\f')) {
            break;
        }
    }
    if (s == expr_end) {
        RAISE_SYNTAX_ERROR("f-string: empty expression not allowed");
        return NULL;
    }

    len = expr_end - expr_start;
    /* Allocate 3 extra bytes: open paren, close paren, null byte. */
    str = PyMem_RawMalloc(len + 3);
    if (str == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    str[0] = '(';
    memcpy(str+1, expr_start, len);
    str[len+1] = ')';
    str[len+2] = 0;

    struct tok_state* tok = PyTokenizer_FromString(str, 1);
    if (tok == NULL) {
        return NULL;
    }
    tok->filename = PyUnicode_FromString("<fstring>");
    if (!tok->filename) {
        PyTokenizer_Free(tok);
        return NULL;
    }

    Parser *p2 = _PyPegen_Parser_New(tok, Py_fstring_input, p->flags, p->feature_version,
                                     NULL, p->arena);
    p2->starting_lineno = p->starting_lineno + p->tok->first_lineno - 1;
    p2->starting_col_offset = p->tok->first_lineno == p->tok->lineno
                              ? p->starting_col_offset + t->col_offset : 0;

    expr = _PyPegen_run_parser(p2);

    if (expr == NULL) {
        goto exit;
    }

    /* Reuse str to find the correct column offset. */
    str[0] = '{';
    str[len+1] = '}';
    fstring_fix_expr_location(t, expr, str);

    result = expr;

exit:
    _PyPegen_Parser_Free(p2);
    PyTokenizer_Free(tok);
    return result;
}

/* Return -1 on error.

   Return 0 if we reached the end of the literal.

   Return 1 if we haven't reached the end of the literal, but we want
   the caller to process the literal up to this point. Used for
   doubled braces.
*/
static int
fstring_find_literal(Parser *p, const char **str, const char *end, int raw,
                     PyObject **literal, int recurse_lvl, Token *t)
{
    /* Get any literal string. It ends when we hit an un-doubled left
       brace (which isn't part of a unicode name escape such as
       "\N{EULER CONSTANT}"), or the end of the string. */

    const char *s = *str;
    const char *literal_start = s;
    int result = 0;

    assert(*literal == NULL);
    while (s < end) {
        char ch = *s++;
        if (!raw && ch == '\\' && s < end) {
            ch = *s++;
            if (ch == 'N') {
                if (s < end && *s++ == '{') {
                    while (s < end && *s++ != '}') {
                    }
                    continue;
                }
                break;
            }
            if (ch == '{' && warn_invalid_escape_sequence(p, ch, t) < 0) {
                return -1;
            }
        }
        if (ch == '{' || ch == '}') {
            /* Check for doubled braces, but only at the top level. If
               we checked at every level, then f'{0:{3}}' would fail
               with the two closing braces. */
            if (recurse_lvl == 0) {
                if (s < end && *s == ch) {
                    /* We're going to tell the caller that the literal ends
                       here, but that they should continue scanning. But also
                       skip over the second brace when we resume scanning. */
                    *str = s + 1;
                    result = 1;
                    goto done;
                }

                /* Where a single '{' is the start of a new expression, a
                   single '}' is not allowed. */
                if (ch == '}') {
                    *str = s - 1;
                    RAISE_SYNTAX_ERROR("f-string: single '}' is not allowed");
                    return -1;
                }
            }
            /* We're either at a '{', which means we're starting another
               expression; or a '}', which means we're at the end of this
               f-string (for a nested format_spec). */
            s--;
            break;
        }
    }
    *str = s;
    assert(s <= end);
    assert(s == end || *s == '{' || *s == '}');
done:
    if (literal_start != s) {
        if (raw)
            *literal = PyUnicode_DecodeUTF8Stateful(literal_start,
                                                    s - literal_start,
                                                    NULL, NULL);
        else
            *literal = decode_unicode_with_escapes(p, literal_start,
                                                   s - literal_start, t);
        if (!*literal)
            return -1;
    }
    return result;
}

/* Forward declaration because parsing is recursive. */
static expr_ty
fstring_parse(Parser *p, const char **str, const char *end, int raw, int recurse_lvl,
              Token *first_token, Token* t, Token *last_token);

/* Parse the f-string at *str, ending at end.  We know *str starts an
   expression (so it must be a '{'). Returns the FormattedValue node, which
   includes the expression, conversion character, format_spec expression, and
   optionally the text of the expression (if = is used).

   Note that I don't do a perfect job here: I don't make sure that a
   closing brace doesn't match an opening paren, for example. It
   doesn't need to error on all invalid expressions, just correctly
   find the end of all valid ones. Any errors inside the expression
   will be caught when we parse it later.

   *expression is set to the expression.  For an '=' "debug" expression,
   *expr_text is set to the debug text (the original text of the expression,
   including the '=' and any whitespace around it, as a string object).  If
   not a debug expression, *expr_text set to NULL. */
static int
fstring_find_expr(Parser *p, const char **str, const char *end, int raw, int recurse_lvl,
                  PyObject **expr_text, expr_ty *expression, Token *first_token,
                  Token *t, Token *last_token)
{
    /* Return -1 on error, else 0. */

    const char *expr_start;
    const char *expr_end;
    expr_ty simple_expression;
    expr_ty format_spec = NULL; /* Optional format specifier. */
    int conversion = -1; /* The conversion char.  Use default if not
                            specified, or !r if using = and no format
                            spec. */

    /* 0 if we're not in a string, else the quote char we're trying to
       match (single or double quote). */
    char quote_char = 0;

    /* If we're inside a string, 1=normal, 3=triple-quoted. */
    int string_type = 0;

    /* Keep track of nesting level for braces/parens/brackets in
       expressions. */
    Py_ssize_t nested_depth = 0;
    char parenstack[MAXLEVEL];

    *expr_text = NULL;

    /* Can only nest one level deep. */
    if (recurse_lvl >= 2) {
        RAISE_SYNTAX_ERROR("f-string: expressions nested too deeply");
        goto error;
    }

    /* The first char must be a left brace, or we wouldn't have gotten
       here. Skip over it. */
    assert(**str == '{');
    *str += 1;

    expr_start = *str;
    for (; *str < end; (*str)++) {
        char ch;

        /* Loop invariants. */
        assert(nested_depth >= 0);
        assert(*str >= expr_start && *str < end);
        if (quote_char)
            assert(string_type == 1 || string_type == 3);
        else
            assert(string_type == 0);

        ch = **str;
        /* Nowhere inside an expression is a backslash allowed. */
        if (ch == '\\') {
            /* Error: can't include a backslash character, inside
               parens or strings or not. */
            RAISE_SYNTAX_ERROR(
                      "f-string expression part "
                      "cannot include a backslash");
            goto error;
        }
        if (quote_char) {
            /* We're inside a string. See if we're at the end. */
            /* This code needs to implement the same non-error logic
               as tok_get from tokenizer.c, at the letter_quote
               label. To actually share that code would be a
               nightmare. But, it's unlikely to change and is small,
               so duplicate it here. Note we don't need to catch all
               of the errors, since they'll be caught when parsing the
               expression. We just need to match the non-error
               cases. Thus we can ignore \n in single-quoted strings,
               for example. Or non-terminated strings. */
            if (ch == quote_char) {
                /* Does this match the string_type (single or triple
                   quoted)? */
                if (string_type == 3) {
                    if (*str+2 < end && *(*str+1) == ch && *(*str+2) == ch) {
                        /* We're at the end of a triple quoted string. */
                        *str += 2;
                        string_type = 0;
                        quote_char = 0;
                        continue;
                    }
                } else {
                    /* We're at the end of a normal string. */
                    quote_char = 0;
                    string_type = 0;
                    continue;
                }
            }
        } else if (ch == '\'' || ch == '"') {
            /* Is this a triple quoted string? */
            if (*str+2 < end && *(*str+1) == ch && *(*str+2) == ch) {
                string_type = 3;
                *str += 2;
            } else {
                /* Start of a normal string. */
                string_type = 1;
            }
            /* Start looking for the end of the string. */
            quote_char = ch;
        } else if (ch == '[' || ch == '{' || ch == '(') {
            if (nested_depth >= MAXLEVEL) {
                RAISE_SYNTAX_ERROR("f-string: too many nested parenthesis");
                goto error;
            }
            parenstack[nested_depth] = ch;
            nested_depth++;
        } else if (ch == '#') {
            /* Error: can't include a comment character, inside parens
               or not. */
            RAISE_SYNTAX_ERROR("f-string expression part cannot include '#'");
            goto error;
        } else if (nested_depth == 0 &&
                   (ch == '!' || ch == ':' || ch == '}' ||
                    ch == '=' || ch == '>' || ch == '<')) {
            /* See if there's a next character. */
            if (*str+1 < end) {
                char next = *(*str+1);

                /* For "!=". since '=' is not an allowed conversion character,
                   nothing is lost in this test. */
                if ((ch == '!' && next == '=') ||   /* != */
                    (ch == '=' && next == '=') ||   /* == */
                    (ch == '<' && next == '=') ||   /* <= */
                    (ch == '>' && next == '=')      /* >= */
                    ) {
                    *str += 1;
                    continue;
                }
                /* Don't get out of the loop for these, if they're single
                   chars (not part of 2-char tokens). If by themselves, they
                   don't end an expression (unlike say '!'). */
                if (ch == '>' || ch == '<') {
                    continue;
                }
            }

            /* Normal way out of this loop. */
            break;
        } else if (ch == ']' || ch == '}' || ch == ')') {
            if (!nested_depth) {
                RAISE_SYNTAX_ERROR("f-string: unmatched '%c'", ch);
                goto error;
            }
            nested_depth--;
            int opening = parenstack[nested_depth];
            if (!((opening == '(' && ch == ')') ||
                  (opening == '[' && ch == ']') ||
                  (opening == '{' && ch == '}')))
            {
                RAISE_SYNTAX_ERROR(
                          "f-string: closing parenthesis '%c' "
                          "does not match opening parenthesis '%c'",
                          ch, opening);
                goto error;
            }
        } else {
            /* Just consume this char and loop around. */
        }
    }
    expr_end = *str;
    /* If we leave this loop in a string or with mismatched parens, we
       don't care. We'll get a syntax error when compiling the
       expression. But, we can produce a better error message, so
       let's just do that.*/
    if (quote_char) {
        RAISE_SYNTAX_ERROR("f-string: unterminated string");
        goto error;
    }
    if (nested_depth) {
        int opening = parenstack[nested_depth - 1];
        RAISE_SYNTAX_ERROR("f-string: unmatched '%c'", opening);
        goto error;
    }

    if (*str >= end)
        goto unexpected_end_of_string;

    /* Compile the expression as soon as possible, so we show errors
       related to the expression before errors related to the
       conversion or format_spec. */
    simple_expression = fstring_compile_expr(p, expr_start, expr_end, t);
    if (!simple_expression)
        goto error;

    /* Check for =, which puts the text value of the expression in
       expr_text. */
    if (**str == '=') {
        *str += 1;

        /* Skip over ASCII whitespace.  No need to test for end of string
           here, since we know there's at least a trailing quote somewhere
           ahead. */
        while (Py_ISSPACE(**str)) {
            *str += 1;
        }

        /* Set *expr_text to the text of the expression. */
        *expr_text = PyUnicode_FromStringAndSize(expr_start, *str-expr_start);
        if (!*expr_text) {
            goto error;
        }
    }

    /* Check for a conversion char, if present. */
    if (**str == '!') {
        *str += 1;
        if (*str >= end)
            goto unexpected_end_of_string;

        conversion = **str;
        *str += 1;

        /* Validate the conversion. */
        if (!(conversion == 's' || conversion == 'r' || conversion == 'a')) {
            RAISE_SYNTAX_ERROR(
                      "f-string: invalid conversion character: "
                      "expected 's', 'r', or 'a'");
            goto error;
        }

    }

    /* Check for the format spec, if present. */
    if (*str >= end)
        goto unexpected_end_of_string;
    if (**str == ':') {
        *str += 1;
        if (*str >= end)
            goto unexpected_end_of_string;

        /* Parse the format spec. */
        format_spec = fstring_parse(p, str, end, raw, recurse_lvl+1,
                                    first_token, t, last_token);
        if (!format_spec)
            goto error;
    }

    if (*str >= end || **str != '}')
        goto unexpected_end_of_string;

    /* We're at a right brace. Consume it. */
    assert(*str < end);
    assert(**str == '}');
    *str += 1;

    /* If we're in = mode (detected by non-NULL expr_text), and have no format
       spec and no explicit conversion, set the conversion to 'r'. */
    if (*expr_text && format_spec == NULL && conversion == -1) {
        conversion = 'r';
    }

    /* And now create the FormattedValue node that represents this
       entire expression with the conversion and format spec. */
    //TODO: Fix this
    *expression = FormattedValue(simple_expression, conversion,
                                 format_spec, first_token->lineno,
                                 first_token->col_offset, last_token->end_lineno,
                                 last_token->end_col_offset, p->arena);
    if (!*expression)
        goto error;

    return 0;

unexpected_end_of_string:
    RAISE_SYNTAX_ERROR("f-string: expecting '}'");
    /* Falls through to error. */

error:
    Py_XDECREF(*expr_text);
    return -1;

}

/* Return -1 on error.

   Return 0 if we have a literal (possible zero length) and an
   expression (zero length if at the end of the string.

   Return 1 if we have a literal, but no expression, and we want the
   caller to call us again. This is used to deal with doubled
   braces.

   When called multiple times on the string 'a{{b{0}c', this function
   will return:

   1. the literal 'a{' with no expression, and a return value
      of 1. Despite the fact that there's no expression, the return
      value of 1 means we're not finished yet.

   2. the literal 'b' and the expression '0', with a return value of
      0. The fact that there's an expression means we're not finished.

   3. literal 'c' with no expression and a return value of 0. The
      combination of the return value of 0 with no expression means
      we're finished.
*/
static int
fstring_find_literal_and_expr(Parser *p, const char **str, const char *end, int raw,
                              int recurse_lvl, PyObject **literal,
                              PyObject **expr_text, expr_ty *expression,
                              Token *first_token, Token *t, Token *last_token)
{
    int result;

    assert(*literal == NULL && *expression == NULL);

    /* Get any literal string. */
    result = fstring_find_literal(p, str, end, raw, literal, recurse_lvl, t);
    if (result < 0)
        goto error;

    assert(result == 0 || result == 1);

    if (result == 1)
        /* We have a literal, but don't look at the expression. */
        return 1;

    if (*str >= end || **str == '}')
        /* We're at the end of the string or the end of a nested
           f-string: no expression. The top-level error case where we
           expect to be at the end of the string but we're at a '}' is
           handled later. */
        return 0;

    /* We must now be the start of an expression, on a '{'. */
    assert(**str == '{');

    if (fstring_find_expr(p, str, end, raw, recurse_lvl, expr_text,
                          expression, first_token, t, last_token) < 0)
        goto error;

    return 0;

error:
    Py_CLEAR(*literal);
    return -1;
}

#ifdef NDEBUG
#define ExprList_check_invariants(l)
#else
static void
ExprList_check_invariants(ExprList *l)
{
    /* Check our invariants. Make sure this object is "live", and
       hasn't been deallocated. */
    assert(l->size >= 0);
    assert(l->p != NULL);
    if (l->size <= EXPRLIST_N_CACHED)
        assert(l->data == l->p);
}
#endif

static void
ExprList_Init(ExprList *l)
{
    l->allocated = EXPRLIST_N_CACHED;
    l->size = 0;

    /* Until we start allocating dynamically, p points to data. */
    l->p = l->data;

    ExprList_check_invariants(l);
}

static int
ExprList_Append(ExprList *l, expr_ty exp)
{
    ExprList_check_invariants(l);
    if (l->size >= l->allocated) {
        /* We need to alloc (or realloc) the memory. */
        Py_ssize_t new_size = l->allocated * 2;

        /* See if we've ever allocated anything dynamically. */
        if (l->p == l->data) {
            Py_ssize_t i;
            /* We're still using the cached data. Switch to
               alloc-ing. */
            l->p = PyMem_RawMalloc(sizeof(expr_ty) * new_size);
            if (!l->p)
                return -1;
            /* Copy the cached data into the new buffer. */
            for (i = 0; i < l->size; i++)
                l->p[i] = l->data[i];
        } else {
            /* Just realloc. */
            expr_ty *tmp = PyMem_RawRealloc(l->p, sizeof(expr_ty) * new_size);
            if (!tmp) {
                PyMem_RawFree(l->p);
                l->p = NULL;
                return -1;
            }
            l->p = tmp;
        }

        l->allocated = new_size;
        assert(l->allocated == 2 * l->size);
    }

    l->p[l->size++] = exp;

    ExprList_check_invariants(l);
    return 0;
}

static void
ExprList_Dealloc(ExprList *l)
{
    ExprList_check_invariants(l);

    /* If there's been an error, or we've never dynamically allocated,
       do nothing. */
    if (!l->p || l->p == l->data) {
        /* Do nothing. */
    } else {
        /* We have dynamically allocated. Free the memory. */
        PyMem_RawFree(l->p);
    }
    l->p = NULL;
    l->size = -1;
}

static asdl_seq *
ExprList_Finish(ExprList *l, PyArena *arena)
{
    asdl_seq *seq;

    ExprList_check_invariants(l);

    /* Allocate the asdl_seq and copy the expressions in to it. */
    seq = _Py_asdl_seq_new(l->size, arena);
    if (seq) {
        Py_ssize_t i;
        for (i = 0; i < l->size; i++)
            asdl_seq_SET(seq, i, l->p[i]);
    }
    ExprList_Dealloc(l);
    return seq;
}

#ifdef NDEBUG
#define FstringParser_check_invariants(state)
#else
static void
FstringParser_check_invariants(FstringParser *state)
{
    if (state->last_str)
        assert(PyUnicode_CheckExact(state->last_str));
    ExprList_check_invariants(&state->expr_list);
}
#endif

void
_PyPegen_FstringParser_Init(FstringParser *state)
{
    state->last_str = NULL;
    state->fmode = 0;
    ExprList_Init(&state->expr_list);
    FstringParser_check_invariants(state);
}

void
_PyPegen_FstringParser_Dealloc(FstringParser *state)
{
    FstringParser_check_invariants(state);

    Py_XDECREF(state->last_str);
    ExprList_Dealloc(&state->expr_list);
}

/* Make a Constant node, but decref the PyUnicode object being added. */
static expr_ty
make_str_node_and_del(Parser *p, PyObject **str, Token* first_token, Token *last_token)
{
    PyObject *s = *str;
    PyObject *kind = NULL;
    *str = NULL;
    assert(PyUnicode_CheckExact(s));
    if (PyArena_AddPyObject(p->arena, s) < 0) {
        Py_DECREF(s);
        return NULL;
    }
    const char* the_str = PyBytes_AsString(first_token->bytes);
    if (the_str && the_str[0] == 'u') {
        kind = _PyPegen_new_identifier(p, "u");
    }

    if (kind == NULL && PyErr_Occurred()) {
        return NULL;
    }

    return Constant(s, kind, first_token->lineno, first_token->col_offset,
                    last_token->end_lineno, last_token->end_col_offset, p->arena);

}


/* Add a non-f-string (that is, a regular literal string). str is
   decref'd. */
int
_PyPegen_FstringParser_ConcatAndDel(FstringParser *state, PyObject *str)
{
    FstringParser_check_invariants(state);

    assert(PyUnicode_CheckExact(str));

    if (PyUnicode_GET_LENGTH(str) == 0) {
        Py_DECREF(str);
        return 0;
    }

    if (!state->last_str) {
        /* We didn't have a string before, so just remember this one. */
        state->last_str = str;
    } else {
        /* Concatenate this with the previous string. */
        PyUnicode_AppendAndDel(&state->last_str, str);
        if (!state->last_str)
            return -1;
    }
    FstringParser_check_invariants(state);
    return 0;
}

/* Parse an f-string. The f-string is in *str to end, with no
   'f' or quotes. */
int
_PyPegen_FstringParser_ConcatFstring(Parser *p, FstringParser *state, const char **str,
                            const char *end, int raw, int recurse_lvl,
                            Token *first_token, Token* t, Token *last_token)
{
    FstringParser_check_invariants(state);
    state->fmode = 1;

    /* Parse the f-string. */
    while (1) {
        PyObject *literal = NULL;
        PyObject *expr_text = NULL;
        expr_ty expression = NULL;

        /* If there's a zero length literal in front of the
           expression, literal will be NULL. If we're at the end of
           the f-string, expression will be NULL (unless result == 1,
           see below). */
        int result = fstring_find_literal_and_expr(p, str, end, raw, recurse_lvl,
                                                   &literal, &expr_text,
                                                   &expression, first_token, t, last_token);
        if (result < 0)
            return -1;

        /* Add the literal, if any. */
        if (literal && _PyPegen_FstringParser_ConcatAndDel(state, literal) < 0) {
            Py_XDECREF(expr_text);
            return -1;
        }
        /* Add the expr_text, if any. */
        if (expr_text && _PyPegen_FstringParser_ConcatAndDel(state, expr_text) < 0) {
            return -1;
        }

        /* We've dealt with the literal and expr_text, their ownership has
           been transferred to the state object.  Don't look at them again. */

        /* See if we should just loop around to get the next literal
           and expression, while ignoring the expression this
           time. This is used for un-doubling braces, as an
           optimization. */
        if (result == 1)
            continue;

        if (!expression)
            /* We're done with this f-string. */
            break;

        /* We know we have an expression. Convert any existing string
           to a Constant node. */
        if (!state->last_str) {
            /* Do nothing. No previous literal. */
        } else {
            /* Convert the existing last_str literal to a Constant node. */
            expr_ty str = make_str_node_and_del(p, &state->last_str, first_token, last_token);
            if (!str || ExprList_Append(&state->expr_list, str) < 0)
                return -1;
        }

        if (ExprList_Append(&state->expr_list, expression) < 0)
            return -1;
    }

    /* If recurse_lvl is zero, then we must be at the end of the
       string. Otherwise, we must be at a right brace. */

    if (recurse_lvl == 0 && *str < end-1) {
        RAISE_SYNTAX_ERROR("f-string: unexpected end of string");
        return -1;
    }
    if (recurse_lvl != 0 && **str != '}') {
        RAISE_SYNTAX_ERROR("f-string: expecting '}'");
        return -1;
    }

    FstringParser_check_invariants(state);
    return 0;
}

/* Convert the partial state reflected in last_str and expr_list to an
   expr_ty. The expr_ty can be a Constant, or a JoinedStr. */
expr_ty
_PyPegen_FstringParser_Finish(Parser *p, FstringParser *state, Token* first_token,
                     Token *last_token)
{
    asdl_seq *seq;

    FstringParser_check_invariants(state);

    /* If we're just a constant string with no expressions, return
       that. */
    if (!state->fmode) {
        assert(!state->expr_list.size);
        if (!state->last_str) {
            /* Create a zero length string. */
            state->last_str = PyUnicode_FromStringAndSize(NULL, 0);
            if (!state->last_str)
                goto error;
        }
        return make_str_node_and_del(p, &state->last_str, first_token, last_token);
    }

    /* Create a Constant node out of last_str, if needed. It will be the
       last node in our expression list. */
    if (state->last_str) {
        expr_ty str = make_str_node_and_del(p, &state->last_str, first_token, last_token);
        if (!str || ExprList_Append(&state->expr_list, str) < 0)
            goto error;
    }
    /* This has already been freed. */
    assert(state->last_str == NULL);

    seq = ExprList_Finish(&state->expr_list, p->arena);
    if (!seq)
        goto error;

    return _Py_JoinedStr(seq, first_token->lineno, first_token->col_offset,
                         last_token->end_lineno, last_token->end_col_offset, p->arena);

error:
    _PyPegen_FstringParser_Dealloc(state);
    return NULL;
}

/* Given an f-string (with no 'f' or quotes) that's in *str and ends
   at end, parse it into an expr_ty.  Return NULL on error.  Adjust
   str to point past the parsed portion. */
static expr_ty
fstring_parse(Parser *p, const char **str, const char *end, int raw,
              int recurse_lvl, Token *first_token, Token* t, Token *last_token)
{
    FstringParser state;

    _PyPegen_FstringParser_Init(&state);
    if (_PyPegen_FstringParser_ConcatFstring(p, &state, str, end, raw, recurse_lvl,
                                    first_token, t, last_token) < 0) {
        _PyPegen_FstringParser_Dealloc(&state);
        return NULL;
    }

    return _PyPegen_FstringParser_Finish(p, &state, t, t);
}
