#include "Python.h"
#include "node.h"
#include "token.h"
#include "graminit.h"
#include "compile.h"
#include "symtable.h"

#define UNDEFINED_FUTURE_FEATURE "future feature %.100s is not defined"
#define FUTURE_IMPORT_STAR "future statement does not support import *"

/* FUTURE_POSSIBLE() is provided to accomodate doc strings, which is
   the only statement that can occur before a future statement.
*/
#define FUTURE_POSSIBLE(FF) ((FF)->ff_last_lineno == -1)

static int
future_check_features(PyFutureFeatures *ff, node *n, const char *filename)
{
	int i;
	char *feature;
	node *ch, *nn;

	REQ(n, import_from);
	nn = CHILD(n, 3 + (TYPE(CHILD(n, 3)) == LPAR));
	if (TYPE(nn) == STAR) {
		PyErr_SetString(PyExc_SyntaxError, FUTURE_IMPORT_STAR);
		PyErr_SyntaxLocation(filename, nn->n_lineno);
		return -1;
	}
	REQ(nn, import_as_names);
	for (i = 0; i < NCH(nn); i += 2) {
		ch = CHILD(nn, i);
		REQ(ch, import_as_name);
		feature = STR(CHILD(ch, 0));
		if (strcmp(feature, FUTURE_NESTED_SCOPES) == 0) {
			continue;
		} else if (strcmp(feature, FUTURE_GENERATORS) == 0) {
			continue;
		} else if (strcmp(feature, FUTURE_DIVISION) == 0) {
			ff->ff_features |= CO_FUTURE_DIVISION;
		} else if (strcmp(feature, "braces") == 0) {
			PyErr_SetString(PyExc_SyntaxError,
					"not a chance");
			PyErr_SyntaxLocation(filename, CHILD(ch, 0)->n_lineno);
			return -1;
		} else {
			PyErr_Format(PyExc_SyntaxError,
				     UNDEFINED_FUTURE_FEATURE, feature);
			PyErr_SyntaxLocation(filename, CHILD(ch, 0)->n_lineno);
			return -1;
		}
	}
	return 0;
}

static void
future_error(node *n, const char *filename)
{
	PyErr_SetString(PyExc_SyntaxError,
			"from __future__ imports must occur at the "
			"beginning of the file");
	PyErr_SyntaxLocation(filename, n->n_lineno);
}

/* Relevant portions of the grammar:

single_input: NEWLINE | simple_stmt | compound_stmt NEWLINE
file_input: (NEWLINE | stmt)* ENDMARKER
stmt: simple_stmt | compound_stmt
simple_stmt: small_stmt (';' small_stmt)* [';'] NEWLINE
small_stmt: expr_stmt | print_stmt  | del_stmt | pass_stmt | flow_stmt 
    | import_stmt | global_stmt | exec_stmt | assert_stmt
import_stmt: 'import' dotted_as_name (',' dotted_as_name)* 
    | 'from' dotted_name 'import' ('*' | import_as_name (',' import_as_name)*)
import_as_name: NAME [NAME NAME]
dotted_as_name: dotted_name [NAME NAME]
dotted_name: NAME ('.' NAME)*
*/

/* future_parse() finds future statements at the beginnning of a
   module.  The function calls itself recursively, rather than
   factoring out logic for different kinds of statements into
   different routines.

   Return values:
   -1 indicates an error occurred, e.g. unknown feature name
   0 indicates no feature was found
   1 indicates a feature was found
*/

static int
future_parse(PyFutureFeatures *ff, node *n, const char *filename)
{
	int i, r;
 loop:

	switch (TYPE(n)) {

	case single_input:
		if (TYPE(CHILD(n, 0)) == simple_stmt) {
			n = CHILD(n, 0);
			goto loop;
		}
		return 0;

	case file_input:
		/* Check each statement in the file, starting with the
		   first, and continuing until the first statement
		   that isn't a future statement.
		*/
		for (i = 0; i < NCH(n); i++) {
			node *ch = CHILD(n, i);
			if (TYPE(ch) == stmt) {
				r = future_parse(ff, ch, filename);
				/* Need to check both conditions below
				   to accomodate doc strings, which
				   causes r < 0.
				*/
				if (r < 1 && !FUTURE_POSSIBLE(ff))
					return r;
			}
		}
		return 0;

	case simple_stmt:
		if (NCH(n) == 2) {
			REQ(CHILD(n, 0), small_stmt);
			n = CHILD(n, 0);
			goto loop;
		} else {
			/* Deal with the special case of a series of
			   small statements on a single line.  If a
			   future statement follows some other
			   statement, the SyntaxError is raised here.
			   In all other cases, the symtable pass
			   raises the exception.
			*/
			int found = 0, end_of_future = 0;

			for (i = 0; i < NCH(n); i += 2) {
				if (TYPE(CHILD(n, i)) == small_stmt) {
					r = future_parse(ff, CHILD(n, i), 
							 filename);
					if (r < 1)
						end_of_future = 1;
					else {
						found = 1;
						if (end_of_future) {
							future_error(n, 
								     filename);
							return -1;
						}
					}
				}
			}

			/* If we found one and only one, then the
			   current lineno is legal. 
			*/
			if (found)
				ff->ff_last_lineno = n->n_lineno + 1;
			else
				ff->ff_last_lineno = n->n_lineno;

			if (end_of_future && found)
				return 1;
			else 
				return 0;
		}
	
	case stmt:
		if (TYPE(CHILD(n, 0)) == simple_stmt) {
			n = CHILD(n, 0);
			goto loop;
		} else if (TYPE(CHILD(n, 0)) == expr_stmt) {
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

		n = CHILD(n, 0);
		if (TYPE(n) != import_from) {
			ff->ff_last_lineno = n->n_lineno;
			return 0;
		}
		name = CHILD(n, 1);
		if (strcmp(STR(CHILD(name, 0)), "__future__") != 0)
			return 0;
		if (future_check_features(ff, n, filename) < 0)
			return -1;
		ff->ff_last_lineno = n->n_lineno + 1;
		return 1;
	}

	/* The cases below -- all of them! -- are necessary to find
	   and skip doc strings. */
	case expr_stmt:
	case testlist:
	case test:
	case and_test:
	case not_test:
	case comparison:
	case expr:
	case xor_expr:
	case and_expr:
	case shift_expr:
	case arith_expr:
	case term:
	case factor:
	case power:
		if (NCH(n) == 1) {
			n = CHILD(n, 0);
			goto loop;
		}
		break;

	case atom:
		if (TYPE(CHILD(n, 0)) == STRING 
		    && ff->ff_found_docstring == 0) {
			ff->ff_found_docstring = 1;
			return 0;
		}
		ff->ff_last_lineno = n->n_lineno;
		return 0;

	default:
		ff->ff_last_lineno = n->n_lineno;
		return 0;
	}
	return 0;
}

PyFutureFeatures *
PyNode_Future(node *n, const char *filename)
{
	PyFutureFeatures *ff;

	ff = (PyFutureFeatures *)PyMem_Malloc(sizeof(PyFutureFeatures));
	if (ff == NULL)
		return NULL;
	ff->ff_found_docstring = 0;
	ff->ff_last_lineno = -1;
	ff->ff_features = 0;

	if (future_parse(ff, n, filename) < 0) {
		PyMem_Free((void *)ff);
		return NULL;
	}
	return ff;
}

