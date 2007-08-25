#include "Python.h"
#include "Python-ast.h"
#include "node.h"
#include "token.h"
#include "graminit.h"
#include "code.h"
#include "compile.h"
#include "symtable.h"

#define UNDEFINED_FUTURE_FEATURE "future feature %.100s is not defined"

static int
future_check_features(PyFutureFeatures *ff, stmt_ty s, const char *filename)
{
	int i;
	asdl_seq *names;

	assert(s->kind == ImportFrom_kind);

	names = s->v.ImportFrom.names;
	for (i = 0; i < asdl_seq_LEN(names); i++) {
                alias_ty name = (alias_ty)asdl_seq_GET(names, i);
		const char *feature = PyUnicode_AsString(name->name);
		if (!feature)
			return 0;
		if (strcmp(feature, FUTURE_NESTED_SCOPES) == 0) {
			continue;
		} else if (strcmp(feature, FUTURE_GENERATORS) == 0) {
			continue;
		} else if (strcmp(feature, FUTURE_DIVISION) == 0) {
			continue;
		} else if (strcmp(feature, FUTURE_ABSOLUTE_IMPORT) == 0) {
			continue;
		} else if (strcmp(feature, FUTURE_WITH_STATEMENT) == 0) {
			continue;
		} else if (strcmp(feature, "braces") == 0) {
			PyErr_SetString(PyExc_SyntaxError,
					"not a chance");
			PyErr_SyntaxLocation(filename, s->lineno);
			return 0;
		} else {
			PyErr_Format(PyExc_SyntaxError,
				     UNDEFINED_FUTURE_FEATURE, feature);
			PyErr_SyntaxLocation(filename, s->lineno);
			return 0;
		}
	}
	return 1;
}

static int
future_parse(PyFutureFeatures *ff, mod_ty mod, const char *filename)
{
	int i, found_docstring = 0, done = 0, prev_line = 0;

	static PyObject *future;
	if (!future) {
		future = PyUnicode_InternFromString("__future__");
		if (!future)
			return 0;
	}

	if (!(mod->kind == Module_kind || mod->kind == Interactive_kind))
		return 1;

	/* A subsequent pass will detect future imports that don't
	   appear at the beginning of the file.  There's one case,
	   however, that is easier to handl here: A series of imports
	   joined by semi-colons, where the first import is a future
	   statement but some subsequent import has the future form
	   but is preceded by a regular import.
	*/
	   

	for (i = 0; i < asdl_seq_LEN(mod->v.Module.body); i++) {
		stmt_ty s = (stmt_ty)asdl_seq_GET(mod->v.Module.body, i);

		if (done && s->lineno > prev_line)
			return 1;
		prev_line = s->lineno;

		/* The tests below will return from this function unless it is
		   still possible to find a future statement.  The only things
		   that can precede a future statement are another future
		   statement and a doc string.
		*/

		if (s->kind == ImportFrom_kind) {
			if (s->v.ImportFrom.module == future) {
				if (done) {
					PyErr_SetString(PyExc_SyntaxError,
							ERR_LATE_FUTURE);
					PyErr_SyntaxLocation(filename, 
							     s->lineno);
					return 0;
				}
				if (!future_check_features(ff, s, filename))
					return 0;
				ff->ff_lineno = s->lineno;
			}
			else
				done = 1;
		}
		else if (s->kind == Expr_kind && !found_docstring) {
			expr_ty e = s->v.Expr.value;
			if (e->kind != Str_kind)
				done = 1;
			else
				found_docstring = 1;
		}
		else
			done = 1;
	}
	return 1;
}


PyFutureFeatures *
PyFuture_FromAST(mod_ty mod, const char *filename)
{
	PyFutureFeatures *ff;

	ff = (PyFutureFeatures *)PyObject_Malloc(sizeof(PyFutureFeatures));
	if (ff == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	ff->ff_features = 0;
	ff->ff_lineno = -1;

	if (!future_parse(ff, mod, filename)) {
		PyObject_Free(ff);
		return NULL;
	}
	return ff;
}
