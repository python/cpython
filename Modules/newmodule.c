/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Module new -- create new objects of various types */

#include "allobjects.h"
#include "compile.h"
#include "modsupport.h"

static char new_im_doc[] =
"Create a instance method object from (FUNCTION, INSTANCE, CLASS).";

static object *
new_instancemethod(unused, args)
	object* unused;
	object* args;
{
	object* func;
	object* self;
	object* classObj;

	if (!newgetargs(args, "O!O!O!",
			&Functype, &func,
			&Instancetype, &self,
			&Classtype, &classObj))
		return NULL;
	return newinstancemethodobject(func, self, classObj);
}

static char new_function_doc[] =
"Create a function object from (CODE, GLOBALS, [NAME, ARGCOUNT, ARGDEFS]).";

static object *
new_function(unused, args)
	object* unused;
	object* args;
{
	object* code;
	object* globals;
	object* name = None;
	int argcount = -1;
	object* argdefs = None;
	funcobject* newfunc;

	if (!newgetargs(args, "O!O!|SiO!",
			&Codetype, &code,
			&Mappingtype, &globals,
			&name,
			&argcount,
			&Tupletype, &argdefs))
		return NULL;

	newfunc = (funcobject *)newfuncobject(code, globals);
	if (newfunc == NULL)
		return NULL;

	if (name != None) {
		XINCREF(name);
		XDECREF(newfunc->func_name);
		newfunc->func_name = name;
	}
	newfunc->func_argcount = argcount;
	if (argdefs != NULL) {
		XINCREF(argdefs);
		XDECREF(newfunc->func_argdefs);
		newfunc->func_argdefs  = argdefs;
	}

	return (object *)newfunc;
}

static char new_code_doc[] =
"Create a code object from (CODESTRING, CONSTANTS, NAMES, FILENAME, NAME).";

static object *
new_code(unused, args)
	object* unused;
	object* args;
{
	object* code;
	object* consts;
	object* names;
	object* filename;
	object* name;
  
	if (!newgetargs(args, "SO!O!SS",
			&code, &Tupletype, &consts, &Tupletype, &names,
			&filename, &name))
		return NULL;
	return (object *)newcodeobject(code, consts, names, filename, name);
}

static char new_module_doc[] =
"Create a module object from (NAME).";

static object *
new_module(unused, args)
	object* unused;
	object* args;
{
	char *name;
  
	if (!newgetargs(args, "s", &name))
		return NULL;
	return newmoduleobject(name);
}

static struct methodlist new_methods[] = {
	{"instancemethod",	new_instancemethod,	1, new_im_doc},
	{"function",		new_function,		1, new_function_doc},
	{"code",		new_code,		1, new_code_doc},
	{"module",		new_module,		1, new_module_doc},
	{NULL,			NULL}		/* sentinel */
};

char new_doc[] =
"Functions to create new objects used by the interpreter.\n\
\n\
You need to know a great deal about the interpreter to use this!";

void
initnew()
{
	initmodule3("new", new_methods, new_doc, (object *)NULL);
}
