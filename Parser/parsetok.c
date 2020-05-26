
/* Parser-tokenizer link implementation */

#include "Python.h"
#include "tokenizer.h"
#include "node.h"
#include "grammar.h"
#include "parser.h"
#include "parsetok.h"
#include "errcode.h"
#include "graminit.h"


/* Forward */
static node *parsetok(struct tok_state *, grammar *, int, perrdetail *, int *);
static int initerr(perrdetail *err_ret, PyObject * filename);

typedef struct {
    struct {
        int lineno;
        char *comment;
    } *items;
    size_t size;
    size_t num_items;
} growable_comment_array;

static int
growable_comment_array_init(growable_comment_array *arr, size_t initial_size) {
    assert(initial_size > 0);
    arr->items = malloc(initial_size * sizeof(*arr->items));
    arr->size = initial_size;
    arr->num_items = 0;

    return arr->items != NULL;
}

static int
growable_comment_array_add(growable_comment_array *arr, int lineno, char *comment) {
    if (arr->num_items >= arr->size) {
        size_t new_size = arr->size * 2;
        void *new_items_array = realloc(arr->items, new_size * sizeof(*arr->items));
        if (!new_items_array) {
            return 0;
        }
        arr->items = new_items_array;
        arr->size = new_size;
    }

    arr->items[arr->num_items].lineno = lineno;
    arr->items[arr->num_items].comment = comment;
    arr->num_items++;
    return 1;
}

static void
growable_comment_array_deallocate(growable_comment_array *arr) {
    for (unsigned i = 0; i < arr->num_items; i++) {
        PyObject_FREE(arr->items[i].comment);
    }
    free(arr->items);
}

/* Parse input coming from a string.  Return error code, print some errors. */
node *
PyParser_ParseString(const char *s, grammar *g, int start, perrdetail *err_ret)
{
    return PyParser_ParseStringFlagsFilename(s, NULL, g, start, err_ret, 0);
}

node *
PyParser_ParseStringFlags(const char *s, grammar *g, int start,
                          perrdetail *err_ret, int flags)
{
    return PyParser_ParseStringFlagsFilename(s, NULL,
                                             g, start, err_ret, flags);
}

node *
PyParser_ParseStringFlagsFilename(const char *s, const char *filename,
                          grammar *g, int start,
                          perrdetail *err_ret, int flags)
{
    int iflags = flags;
    return PyParser_ParseStringFlagsFilenameEx(s, filename, g, start,
                                               err_ret, &iflags);
}

node *
PyParser_ParseStringObject(const char *s, PyObject *filename,
                           grammar *g, int start,
                           perrdetail *err_ret, int *flags)
{
    struct tok_state *tok;
    int exec_input = start == file_input;

    if (initerr(err_ret, filename) < 0)
        return NULL;

    if (PySys_Audit("compile", "yO", s, err_ret->filename) < 0) {
        err_ret->error = E_ERROR;
        return NULL;
    }

    if (*flags & PyPARSE_IGNORE_COOKIE)
        tok = PyTokenizer_FromUTF8(s, exec_input);
    else
        tok = PyTokenizer_FromString(s, exec_input);
    if (tok == NULL) {
        err_ret->error = PyErr_Occurred() ? E_DECODE : E_NOMEM;
        return NULL;
    }
    if (*flags & PyPARSE_TYPE_COMMENTS) {
        tok->type_comments = 1;
    }

    Py_INCREF(err_ret->filename);
    tok->filename = err_ret->filename;
    if (*flags & PyPARSE_ASYNC_HACKS)
        tok->async_hacks = 1;
    return parsetok(tok, g, start, err_ret, flags);
}

node *
PyParser_ParseStringFlagsFilenameEx(const char *s, const char *filename_str,
                          grammar *g, int start,
                          perrdetail *err_ret, int *flags)
{
    node *n;
    PyObject *filename = NULL;
    if (filename_str != NULL) {
        filename = PyUnicode_DecodeFSDefault(filename_str);
        if (filename == NULL) {
            err_ret->error = E_ERROR;
            return NULL;
        }
    }
    n = PyParser_ParseStringObject(s, filename, g, start, err_ret, flags);
    Py_XDECREF(filename);
    return n;
}

/* Parse input coming from a file.  Return error code, print some errors. */

node *
PyParser_ParseFile(FILE *fp, const char *filename, grammar *g, int start,
                   const char *ps1, const char *ps2,
                   perrdetail *err_ret)
{
    return PyParser_ParseFileFlags(fp, filename, NULL,
                                   g, start, ps1, ps2, err_ret, 0);
}

node *
PyParser_ParseFileFlags(FILE *fp, const char *filename, const char *enc,
                        grammar *g, int start,
                        const char *ps1, const char *ps2,
                        perrdetail *err_ret, int flags)
{
    int iflags = flags;
    return PyParser_ParseFileFlagsEx(fp, filename, enc, g, start, ps1,
                                     ps2, err_ret, &iflags);
}

node *
PyParser_ParseFileObject(FILE *fp, PyObject *filename,
                         const char *enc, grammar *g, int start,
                         const char *ps1, const char *ps2,
                         perrdetail *err_ret, int *flags)
{
    struct tok_state *tok;

    if (initerr(err_ret, filename) < 0)
        return NULL;

    if (PySys_Audit("compile", "OO", Py_None, err_ret->filename) < 0) {
        return NULL;
    }

    if ((tok = PyTokenizer_FromFile(fp, enc, ps1, ps2)) == NULL) {
        err_ret->error = E_NOMEM;
        return NULL;
    }
    if (*flags & PyPARSE_TYPE_COMMENTS) {
        tok->type_comments = 1;
    }
    Py_INCREF(err_ret->filename);
    tok->filename = err_ret->filename;
    return parsetok(tok, g, start, err_ret, flags);
}

node *
PyParser_ParseFileFlagsEx(FILE *fp, const char *filename,
                          const char *enc, grammar *g, int start,
                          const char *ps1, const char *ps2,
                          perrdetail *err_ret, int *flags)
{
    node *n;
    PyObject *fileobj = NULL;
    if (filename != NULL) {
        fileobj = PyUnicode_DecodeFSDefault(filename);
        if (fileobj == NULL) {
            err_ret->error = E_ERROR;
            return NULL;
        }
    }
    n = PyParser_ParseFileObject(fp, fileobj, enc, g,
                                 start, ps1, ps2, err_ret, flags);
    Py_XDECREF(fileobj);
    return n;
}

/* Parse input coming from the given tokenizer structure.
   Return error code. */

static node *
parsetok(struct tok_state *tok, grammar *g, int start, perrdetail *err_ret,
         int *flags)
{
    parser_state *ps;
    node *n;
    int started = 0;
    int col_offset, end_col_offset;
    growable_comment_array type_ignores;

    if (!growable_comment_array_init(&type_ignores, 10)) {
        err_ret->error = E_NOMEM;
        PyTokenizer_Free(tok);
        return NULL;
    }

    if ((ps = PyParser_New(g, start)) == NULL) {
        err_ret->error = E_NOMEM;
        growable_comment_array_deallocate(&type_ignores);
        PyTokenizer_Free(tok);
        return NULL;
    }
#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
    if (*flags & PyPARSE_BARRY_AS_BDFL)
        ps->p_flags |= CO_FUTURE_BARRY_AS_BDFL;
    if (*flags & PyPARSE_TYPE_COMMENTS)
        ps->p_flags |= PyCF_TYPE_COMMENTS;
#endif

    for (;;) {
        const char *a, *b;
        int type;
        size_t len;
        char *str;
        col_offset = -1;
        int lineno;
        const char *line_start;

        type = PyTokenizer_Get(tok, &a, &b);

        len = (a != NULL && b != NULL) ? b - a : 0;
        str = (char *) PyObject_MALLOC(len + 1);
        if (str == NULL) {
            err_ret->error = E_NOMEM;
            break;
        }
        if (len > 0)
            strncpy(str, a, len);
        str[len] = '\0';

#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
        if (type == NOTEQUAL) {
            if (!(ps->p_flags & CO_FUTURE_BARRY_AS_BDFL) &&
                            strcmp(str, "!=")) {
                PyObject_FREE(str);
                err_ret->error = E_SYNTAX;
                break;
            }
            else if ((ps->p_flags & CO_FUTURE_BARRY_AS_BDFL) &&
                            strcmp(str, "<>")) {
                PyObject_FREE(str);
                err_ret->expected = NOTEQUAL;
                err_ret->error = E_SYNTAX;
                break;
            }
        }
#endif

        /* Nodes of type STRING, especially multi line strings
           must be handled differently in order to get both
           the starting line number and the column offset right.
           (cf. issue 16806) */
        lineno = type == STRING ? tok->first_lineno : tok->lineno;
        line_start = type == STRING ? tok->multi_line_start : tok->line_start;
        if (a != NULL && a >= line_start) {
            col_offset = Py_SAFE_DOWNCAST(a - line_start,
                                          intptr_t, int);
        }
        else {
            col_offset = -1;
        }

        if (b != NULL && b >= tok->line_start) {
            end_col_offset = Py_SAFE_DOWNCAST(b - tok->line_start,
                                              intptr_t, int);
        }
        else {
            end_col_offset = -1;
        }

        if (type == TYPE_IGNORE) {
            if (!growable_comment_array_add(&type_ignores, tok->lineno, str)) {
                err_ret->error = E_NOMEM;
                break;
            }
            continue;
        }

        if (type == ERRORTOKEN) {
            err_ret->error = tok->done;
            break;
        }
        if (type == ENDMARKER && started) {
            type = NEWLINE; /* Add an extra newline */
            started = 0;
            /* Add the right number of dedent tokens,
               except if a certain flag is given --
               codeop.py uses this. */
            if (tok->indent &&
                !(*flags & PyPARSE_DONT_IMPLY_DEDENT))
            {
                tok->pendin = -tok->indent;
                tok->indent = 0;
            }
        }
        else {
            started = 1;
        }

        if ((err_ret->error =
             PyParser_AddToken(ps, (int)type, str,
                               lineno, col_offset, tok->lineno, end_col_offset,
                               &(err_ret->expected))) != E_OK) {
            if (tok->done == E_EOF && !ISWHITESPACE(type)) {
                tok->done = E_SYNTAX;
            }
            if (err_ret->error != E_DONE) {
                PyObject_FREE(str);
                err_ret->token = type;
            }
            break;
        }
    }

    if (err_ret->error == E_DONE) {
        n = ps->p_tree;
        ps->p_tree = NULL;

        if (n->n_type == file_input) {
            /* Put type_ignore nodes in the ENDMARKER of file_input. */
            int num;
            node *ch;
            size_t i;

            num = NCH(n);
            ch = CHILD(n, num - 1);
            REQ(ch, ENDMARKER);

            for (i = 0; i < type_ignores.num_items; i++) {
                int res = PyNode_AddChild(ch, TYPE_IGNORE, type_ignores.items[i].comment,
                                          type_ignores.items[i].lineno, 0,
                                          type_ignores.items[i].lineno, 0);
                if (res != 0) {
                    err_ret->error = res;
                    PyNode_Free(n);
                    n = NULL;
                    break;
                }
                type_ignores.items[i].comment = NULL;
            }
        }

        /* Check that the source for a single input statement really
           is a single statement by looking at what is left in the
           buffer after parsing.  Trailing whitespace and comments
           are OK.  */
        if (err_ret->error == E_DONE && start == single_input) {
            const char *cur = tok->cur;
            char c = *tok->cur;

            for (;;) {
                while (c == ' ' || c == '\t' || c == '\n' || c == '\014')
                    c = *++cur;

                if (!c)
                    break;

                if (c != '#') {
                    err_ret->error = E_BADSINGLE;
                    PyNode_Free(n);
                    n = NULL;
                    break;
                }

                /* Suck up comment. */
                while (c && c != '\n')
                    c = *++cur;
            }
        }
    }
    else
        n = NULL;

    growable_comment_array_deallocate(&type_ignores);

#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
    *flags = ps->p_flags;
#endif
    PyParser_Delete(ps);

    if (n == NULL) {
        if (tok->done == E_EOF)
            err_ret->error = E_EOF;
        err_ret->lineno = tok->lineno;
        if (tok->buf != NULL) {
            size_t len;
            assert(tok->cur - tok->buf < INT_MAX);
            /* if we've managed to parse a token, point the offset to its start,
             * else use the current reading position of the tokenizer
             */
            err_ret->offset = col_offset != -1 ? col_offset + 1 : ((int)(tok->cur - tok->buf));
            len = tok->inp - tok->buf;
            err_ret->text = (char *) PyObject_MALLOC(len + 1);
            if (err_ret->text != NULL) {
                if (len > 0)
                    strncpy(err_ret->text, tok->buf, len);
                err_ret->text[len] = '\0';
            }
        }
    } else if (tok->encoding != NULL) {
        /* 'nodes->n_str' uses PyObject_*, while 'tok->encoding' was
         * allocated using PyMem_
         */
        node* r = PyNode_New(encoding_decl);
        if (r)
            r->n_str = PyObject_MALLOC(strlen(tok->encoding)+1);
        if (!r || !r->n_str) {
            err_ret->error = E_NOMEM;
            if (r)
                PyObject_FREE(r);
            n = NULL;
            goto done;
        }
        strcpy(r->n_str, tok->encoding);
        PyMem_FREE(tok->encoding);
        tok->encoding = NULL;
        r->n_nchildren = 1;
        r->n_child = n;
        n = r;
    }

done:
    PyTokenizer_Free(tok);

    if (n != NULL) {
        _PyNode_FinalizeEndPos(n);
    }
    return n;
}

static int
initerr(perrdetail *err_ret, PyObject *filename)
{
    err_ret->error = E_OK;
    err_ret->lineno = 0;
    err_ret->offset = 0;
    err_ret->text = NULL;
    err_ret->token = -1;
    err_ret->expected = -1;
    if (filename) {
        Py_INCREF(filename);
        err_ret->filename = filename;
    }
    else {
        err_ret->filename = PyUnicode_FromString("<string>");
        if (err_ret->filename == NULL) {
            err_ret->error = E_ERROR;
            return -1;
        }
    }
    return 0;
}
