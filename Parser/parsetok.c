
/* Parser-tokenizer link implementation */

#include "pgenheaders.h"
#include "tokenizer.h"
#include "node.h"
#include "grammar.h"
#include "parser.h"
#include "parsetok.h"
#include "errcode.h"

int Py_TabcheckFlag;


/* Forward */
static node *parsetok(struct tok_state *, grammar *, int, perrdetail *, int);
static void initerr(perrdetail *err_ret, char* filename);

/* Parse input coming from a string.  Return error code, print some errors. */
node *
PyParser_ParseString(char *s, grammar *g, int start, perrdetail *err_ret)
{
	return PyParser_ParseStringFlags(s, g, start, err_ret, 0);
}

node *
PyParser_ParseStringFlags(char *s, grammar *g, int start,
		          perrdetail *err_ret, int flags)
{
	struct tok_state *tok;

	initerr(err_ret, NULL);

	if ((tok = PyTokenizer_FromString(s)) == NULL) {
		err_ret->error = E_NOMEM;
		return NULL;
	}

	if (Py_TabcheckFlag || Py_VerboseFlag) {
		tok->filename = "<string>";
		tok->altwarning = (tok->filename != NULL);
		if (Py_TabcheckFlag >= 2)
			tok->alterror++;
	}

	return parsetok(tok, g, start, err_ret, flags);
}


/* Parse input coming from a file.  Return error code, print some errors. */

node *
PyParser_ParseFile(FILE *fp, char *filename, grammar *g, int start,
		   char *ps1, char *ps2, perrdetail *err_ret)
{
	return PyParser_ParseFileFlags(fp, filename, g, start, ps1, ps2,
				       err_ret, 0);
}

node *
PyParser_ParseFileFlags(FILE *fp, char *filename, grammar *g, int start,
			char *ps1, char *ps2, perrdetail *err_ret, int flags)
{
	struct tok_state *tok;

	initerr(err_ret, filename);

	if ((tok = PyTokenizer_FromFile(fp, ps1, ps2)) == NULL) {
		err_ret->error = E_NOMEM;
		return NULL;
	}
	if (Py_TabcheckFlag || Py_VerboseFlag) {
		tok->filename = filename;
		tok->altwarning = (filename != NULL);
		if (Py_TabcheckFlag >= 2)
			tok->alterror++;
	}


	return parsetok(tok, g, start, err_ret, flags);
}

/* Parse input coming from the given tokenizer structure.
   Return error code. */

#if 0 /* future keyword */
static char yield_msg[] =
"%s:%d: Warning: 'yield' will become a reserved keyword in the future\n";
#endif

static node *
parsetok(struct tok_state *tok, grammar *g, int start, perrdetail *err_ret,
	 int flags)
{
	parser_state *ps;
	node *n;
	int started = 0;

	if ((ps = PyParser_New(g, start)) == NULL) {
		fprintf(stderr, "no mem for new parser\n");
		err_ret->error = E_NOMEM;
		return NULL;
	}
#if 0 /* future keyword */
	if (flags & PyPARSE_YIELD_IS_KEYWORD)
		ps->p_generators = 1;
#endif

	for (;;) {
		char *a, *b;
		int type;
		size_t len;
		char *str;

		type = PyTokenizer_Get(tok, &a, &b);
		if (type == ERRORTOKEN) {
			err_ret->error = tok->done;
			break;
		}
		if (type == ENDMARKER && started) {
			type = NEWLINE; /* Add an extra newline */
			started = 0;
		}
		else
			started = 1;
		len = b - a; /* XXX this may compute NULL - NULL */
		str = PyMem_NEW(char, len + 1);
		if (str == NULL) {
			fprintf(stderr, "no mem for next token\n");
			err_ret->error = E_NOMEM;
			break;
		}
		if (len > 0)
			strncpy(str, a, len);
		str[len] = '\0';

#if 0 /* future keyword */
		/* Warn about yield as NAME */
		if (type == NAME && !ps->p_generators &&
		    len == 5 && str[0] == 'y' && strcmp(str, "yield") == 0)
			PySys_WriteStderr(yield_msg,
					  err_ret->filename==NULL ?
					  "<string>" : err_ret->filename,
					  tok->lineno);
#endif

		if ((err_ret->error =
		     PyParser_AddToken(ps, (int)type, str, tok->lineno,
				       &(err_ret->expected))) != E_OK) {
			if (err_ret->error != E_DONE)
				PyMem_DEL(str);
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
		err_ret->offset = tok->cur - tok->buf;
		if (tok->buf != NULL) {
			size_t len = tok->inp - tok->buf;
			err_ret->text = PyMem_NEW(char, len + 1);
			if (err_ret->text != NULL) {
				if (len > 0)
					strncpy(err_ret->text, tok->buf, len);
				err_ret->text[len] = '\0';
			}
		}
	}

	PyTokenizer_Free(tok);

	return n;
}

static void
initerr(perrdetail *err_ret, char* filename)
{
	err_ret->error = E_OK;
	err_ret->filename = filename;
	err_ret->lineno = 0;
	err_ret->offset = 0;
	err_ret->text = NULL;
	err_ret->token = -1;
	err_ret->expected = -1;
}
