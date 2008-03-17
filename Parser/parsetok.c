
/* Parser-tokenizer link implementation */

#include "pgenheaders.h"
#include "tokenizer.h"
#include "node.h"
#include "grammar.h"
#include "parser.h"
#include "parsetok.h"
#include "errcode.h"
#include "graminit.h"

int Py_TabcheckFlag;


/* Forward */
static node *parsetok(struct tok_state *, grammar *, int, perrdetail *, int);
static void initerr(perrdetail *err_ret, const char* filename);

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
	struct tok_state *tok;

	initerr(err_ret, filename);

	if ((tok = PyTokenizer_FromString(s)) == NULL) {
		err_ret->error = PyErr_Occurred() ? E_DECODE : E_NOMEM;
		return NULL;
	}

        tok->filename = filename ? filename : "<string>";
	if (Py_TabcheckFlag >= 3)
		tok->alterror = 0;

	return parsetok(tok, g, start, err_ret, flags);
}

/* Parse input coming from a file.  Return error code, print some errors. */

node *
PyParser_ParseFile(FILE *fp, const char *filename, grammar *g, int start,
		   char *ps1, char *ps2, perrdetail *err_ret)
{
	return PyParser_ParseFileFlags(fp, filename, NULL, 
				       g, start, ps1, ps2, err_ret, 0);
}

node *
PyParser_ParseFileFlags(FILE *fp, const char *filename, const char* enc,
			grammar *g, int start,
			char *ps1, char *ps2, perrdetail *err_ret, int flags)
{
	struct tok_state *tok;

	initerr(err_ret, filename);

	if ((tok = PyTokenizer_FromFile(fp, (char *)enc, ps1, ps2)) == NULL) {
		err_ret->error = E_NOMEM;
		return NULL;
	}
	tok->filename = filename;
	if (Py_TabcheckFlag >= 3)
		tok->alterror = 0;

	return parsetok(tok, g, start, err_ret, flags);
}

#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
static char with_msg[] =
"%s:%d: Warning: 'with' will become a reserved keyword in Python 2.6\n";

static char as_msg[] =
"%s:%d: Warning: 'as' will become a reserved keyword in Python 2.6\n";

static void
warn(const char *msg, const char *filename, int lineno)
{
	if (filename == NULL)
		filename = "<string>";
	PySys_WriteStderr(msg, filename, lineno);
}
#endif

/* Parse input coming from the given tokenizer structure.
   Return error code. */

static node *
parsetok(struct tok_state *tok, grammar *g, int start, perrdetail *err_ret,
	 int flags)
{
	parser_state *ps;
	node *n;
	int started = 0, handling_import = 0, handling_with = 0;

	if ((ps = PyParser_New(g, start)) == NULL) {
		fprintf(stderr, "no mem for new parser\n");
		err_ret->error = E_NOMEM;
		PyTokenizer_Free(tok);
		return NULL;
	}
#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
	if (flags & PyPARSE_WITH_IS_KEYWORD)
		ps->p_flags |= CO_FUTURE_WITH_STATEMENT;
#endif

	for (;;) {
		char *a, *b;
		int type;
		size_t len;
		char *str;
		int col_offset;

		type = PyTokenizer_Get(tok, &a, &b);
		if (type == ERRORTOKEN) {
			err_ret->error = tok->done;
			break;
		}
		if (type == ENDMARKER && started) {
			type = NEWLINE; /* Add an extra newline */
			handling_with = handling_import = 0;
			started = 0;
			/* Add the right number of dedent tokens,
			   except if a certain flag is given --
			   codeop.py uses this. */
			if (tok->indent &&
			    !(flags & PyPARSE_DONT_IMPLY_DEDENT))
			{
				tok->pendin = -tok->indent;
				tok->indent = 0;
			}
		}
		else
			started = 1;
		len = b - a; /* XXX this may compute NULL - NULL */
		str = (char *) PyObject_MALLOC(len + 1);
		if (str == NULL) {
			fprintf(stderr, "no mem for next token\n");
			err_ret->error = E_NOMEM;
			break;
		}
		if (len > 0)
			strncpy(str, a, len);
		str[len] = '\0';

#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
		/* This is only necessary to support the "as" warning, but
		   we don't want to warn about "as" in import statements. */
		if (type == NAME &&
		    len == 6 && str[0] == 'i' && strcmp(str, "import") == 0)
			handling_import = 1;

		/* Warn about with as NAME */
		if (type == NAME &&
		    !(ps->p_flags & CO_FUTURE_WITH_STATEMENT)) {
		    if (len == 4 && str[0] == 'w' && strcmp(str, "with") == 0)
			warn(with_msg, err_ret->filename, tok->lineno);
		    else if (!(handling_import || handling_with) &&
		             len == 2 && str[0] == 'a' &&
			     strcmp(str, "as") == 0)
			warn(as_msg, err_ret->filename, tok->lineno);
		}
		else if (type == NAME &&
			 (ps->p_flags & CO_FUTURE_WITH_STATEMENT) &&
			 len == 4 && str[0] == 'w' && strcmp(str, "with") == 0)
			handling_with = 1;
#endif
		if (a >= tok->line_start)
			col_offset = a - tok->line_start;
		else
			col_offset = -1;
			
		if ((err_ret->error =
		     PyParser_AddToken(ps, (int)type, str, 
                                       tok->lineno, col_offset,
				       &(err_ret->expected))) != E_OK) {
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
	}
	else
		n = NULL;

	PyParser_Delete(ps);

	if (n == NULL) {
		if (tok->lineno <= 1 && tok->done == E_EOF)
			err_ret->error = E_EOF;
		err_ret->lineno = tok->lineno;
		if (tok->buf != NULL) {
			size_t len;
			assert(tok->cur - tok->buf < INT_MAX);
			err_ret->offset = (int)(tok->cur - tok->buf);
			len = tok->inp - tok->buf;
			err_ret->text = (char *) PyObject_MALLOC(len + 1);
			if (err_ret->text != NULL) {
				if (len > 0)
					strncpy(err_ret->text, tok->buf, len);
				err_ret->text[len] = '\0';
			}
		}
	} else if (tok->encoding != NULL) {
		node* r = PyNode_New(encoding_decl);
		if (!r) {
			err_ret->error = E_NOMEM;
			n = NULL;
			goto done;
		}
		r->n_str = tok->encoding;
		r->n_nchildren = 1;
		r->n_child = n;
		tok->encoding = NULL;
		n = r;
	}

done:
	PyTokenizer_Free(tok);

	return n;
}

static void
initerr(perrdetail *err_ret, const char *filename)
{
	err_ret->error = E_OK;
	err_ret->filename = filename;
	err_ret->lineno = 0;
	err_ret->offset = 0;
	err_ret->text = NULL;
	err_ret->token = -1;
	err_ret->expected = -1;
}
