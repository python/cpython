/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Module new -- create new objects of various types */

#include "allobjects.h"
#include "compile.h"

static char new_instance_doc[] =
"Create an instance object from (CLASS, DICT) without calling its __init__().";

static object *
new_instance(unused, args)
	object* unused;
	object* args;
{
	object* klass;
	object *dict;
	instanceobject *inst;
	if (!newgetargs(args, "O!O!",
			&Classtype, &klass,
			&Dicttype, &dict))
		return NULL;
	inst = NEWOBJ(instanceobject, &Instancetype);
	if (inst == NULL)
		return NULL;
	INCREF(klass);
	INCREF(dict);
	inst->in_class = (classobject *)klass;
	inst->in_dict = dict;
	return (object *)inst;
}

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
"Create a function object from (CODE, GLOBALS, [NAME, ARGDEFS]).";

static object *
new_function(unused, args)
	object* unused;
	object* args;
{
	object* code;
	object* globals;
	object* name = None;
	object* defaults = None;
	funcobject* newfunc;

	if (!newgetargs(args, "O!O!|SO!",
			&Codetype, &code,
			&Mappingtype, &globals,
			&name,
			&Tupletype, &defaults))
		return NULL;

	newfunc = (funcobject *)newfuncobject(code, globals);
	if (newfunc == NULL)
		return NULL;

	if (name != None) {
		XINCREF(name);
		XDECREF(newfunc->func_name);
		newfunc->func_name = name;
	}
	if (defaults != NULL) {
		XINCREF(defaults);
		XDECREF(newfunc->func_defaults);
		newfunc->func_defaults  = defaults;
	}

	return (object *)newfunc;
}

static char new_code_doc[] =
"Create a code object from (ARGCOUNT, NLOCALS, FLAGS, CODESTRING, CONSTANTS, NAMES, VARNAMES, FILENAME, NAME).";

static object *
new_code(unused, args)
	object* unused;
	object* args;
{
	int argcount;
	int nlocals;
	int flags;
	object* code;
	object* consts;
	object* names;
	object* varnames;
	object* filename;
	object* name;
  
	if (!newgetargs(args, "iiiSO!O!O!SS",
			&argcount, &nlocals, &flags,	/* These are new */
			&code, &Tupletype, &consts, &Tupletype, &names,
			&Tupletype, &varnames,		/* These are new */
			&filename, &name))
		return NULL;
	return (object *)newcodeobject(argcount, nlocals, flags,
		code, consts, names, varnames, filename, name);
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

static char new_class_doc[] =
"Create a class object from (NAME, BASE_CLASSES, DICT).";

static object *
new_class(unused, args)
	object* unused;
	object* args;
{
	object * name;
	object * classes;
	object * dict;
  
	if (!newgetargs(args, "SO!O!", &name, &Tupletype, &classes,
			&Mappingtype, &dict))
		return NULL;
	return newclassobject(classes, dict, name);
}

static struct methodlist new_methods[] = {
	{"instance",		new_instance,		1, new_instance_doc},
	{"instancemethod",	new_instancemethod,	1, new_im_doc},
	{"function",		new_function,		1, new_function_doc},
	{"code",		new_code,		1, new_code_doc},
	{"module",		new_module,		1, new_module_doc},
	{"classobj",		new_class,		1, new_class_doc},
	{NULL,			NULL}		/* sentinel */
};

char new_doc[] =
"Functions to create new objects used by the interpreter.\n\
\n\
You need to know a great deal about the interpreter to use this!";

void
initnew()
{
	initmodule4("new", new_methods, new_doc, (object *)NULL,
		    PYTHON_API_VERSION);
}
