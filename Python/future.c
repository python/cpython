#include "Python.h"
#include "node.h"
#include "token.h"
#include "graminit.h"
#include "compile.h"
#include "symtable.h"

#define UNDEFINED_FUTURE_FEATURE "future feature %.100s is not defined"

static int
future_check_features(PyFutureFeatures *ff, node *n)
{
	int i;
	char *feature;

	REQ(n, import_stmt); /* must by from __future__ import ... */

	for (i = 3; i < NCH(n); ++i) {
		feature = STR(CHILD(CHILD(n, i), 0));
		if (strcmp(feature, FUTURE_NESTED_SCOPES) == 0) {
			ff->ff_nested_scopes = 1;
		} else {
			PyErr_Format(PyExc_SyntaxError,
				     UNDEFINED_FUTURE_FEATURE, feature);
			return -1;
		}
	}
	return 0;
}

/* Relevant portions of the grammar:

single_input: NEWLINE | simple_stmt | compound_stmt NEWLINE
file_input: (NEWLINE | stmt)* ENDMARKER
stmt: simple_stmt | compound_stmt
simple_stmt: small_stmt (';' small_stmt)* [';'] NEWLINE
small_stmt: expr_stmt | print_stmt  | del_stmt | pass_stmt | flow_stmt | import_stmt | global_stmt | exec_stmt | assert_stmt
import_stmt: 'import' dotted_as_name (',' dotted_as_name)* | 'from' dotted_name 'import' ('*' | import_as_name (',' import_as_name)*)
import_as_name: NAME [NAME NAME]
dotted_as_name: dotted_name [NAME NAME]
dotted_name: NAME ('.' NAME)*
*/

/* future_parse() return values:
   -1 indicates an error occurred, e.g. unknown feature name
   0 indicates no feature was found
   1 indicates a feature was found
*/

static int
future_parse(PyFutureFeatures *ff, node *n)
{
	int i, r, found;
 loop:

/*	fprintf(stderr, "future_parse(%d, %d, %s)\n",
		TYPE(n), NCH(n), (n == NULL) ? "NULL" : STR(n));
*/
	switch (TYPE(n)) {

	case file_input:
		for (i = 0; i < NCH(n); i++) {
			node *ch = CHILD(n, i);
			if (TYPE(ch) == stmt) {
				n = ch;
				goto loop;
			}
		}
		return 0;

	case simple_stmt:
		if (NCH(n) == 1) {
			REQ(CHILD(n, 0), small_stmt);
			n = CHILD(n, 0);
			goto loop;
		}
		found = 0;
		for (i = 0; i < NCH(n); ++i)
			if (TYPE(CHILD(n, i)) == small_stmt) {
				r = future_parse(ff, CHILD(n, i));
				if (r < 1) {
					ff->ff_last_lineno = n->n_lineno;
					ff->ff_n_simple_stmt = i;
					return r;
				} else
					found++;
			}
		if (found)
			return 1;
		else
			return 0;
	
	case stmt:
		if (TYPE(CHILD(n, 0)) == simple_stmt) {
			n = CHILD(n, 0);
			goto loop;
		} else {
			REQ(CHILD(n, 0), compound_stmt);
			ff->ff_last_lineno = n->n_lineno;
			return 0;
		}

	case small_stmt:
		n = CHILD(n, 0);
		goto loop;

	case import_stmt: {
		node *name;

		if (STR(CHILD(n, 0))[0] != 'f') { /* from */
			ff->ff_last_lineno = n->n_lineno;
			return 0;
		}
		name = CHILD(n, 1);
		if (strcmp(STR(CHILD(name, 0)), "__future__") != 0)
			return 0;
		if (future_check_features(ff, n) < 0)
			return -1;
		return 1;
	}

	default:
		ff->ff_last_lineno = n->n_lineno;
		return 0;
	}
}

PyFutureFeatures *
PyNode_Future(node *n, char *filename)
{
	PyFutureFeatures *ff;

	ff = (PyFutureFeatures *)PyMem_Malloc(sizeof(PyFutureFeatures));
	if (ff == NULL)
		return NULL;
	ff->ff_last_lineno = 0;
	ff->ff_n_simple_stmt = -1;
	ff->ff_nested_scopes = 0;

	if (future_parse(ff, n) < 0) {
		PyMem_Free((void *)ff);
		return NULL;
	}
	return ff;
}

