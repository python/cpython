/* newmodule.c */
#include "allobjects.h"
#include "compile.h"
#include "modsupport.h"


object*
new_instancemethod(unused, args)
    object* unused;
    object* args;
{
    object* func;
    object* self;
    object* classObj;
  
    if (!getargs(args, "(OOO)", &func, &self, &classObj)) {
	return NULL;
    } else if (!is_funcobject(func) || !is_instanceobject(self) || !is_classobject(classObj)) {
	err_setstr(TypeError, "expected a function, instance and classobject as args");
	return NULL;
    }
    return newinstancemethodobject(func, self, classObj);
}
    

object*
new_function(unused, args)
    object* unused;
    object* args;
{
    object* code;
    object* globals;
    object* name;
    object* newfunc;
    object* argcount;
    object* argdefs;
  
    if (!getargs(args, "(OOOOO)", &code, &globals, &name, &argcount, &argdefs)) {
	return NULL;
    } else if (!is_codeobject(code) || !is_mappingobject(globals) || !is_stringobject(name) || !is_intobject(argcount) || !is_tupleobject(argdefs)) {
	err_setstr(TypeError, "expected a code object, a dict for globals, a string name, an integer default argument count and a tuple of default argument definitions.");
	return NULL;
    }

    newfunc = newfuncobject(code, globals);

    ((funcobject *)newfunc)->func_name = name;
    XINCREF( name );
    XDECREF( ((codeobject*)(((funcobject *)(newfunc))->func_code))->co_name );

    ((funcobject *)newfunc)->func_argcount = getintvalue(argcount);
    ((funcobject *)newfunc)->func_argdefs  = argdefs;
    XINCREF( argdefs );

    return newfunc;
}
    

object*
new_code(unused, args)
    object* unused;
    object* args;
{
    object* code;
    object* consts;
    object* names;
    object* filename;
    object* name;
  
    if (!getargs(args, "(OOOOO)", &code, &consts, &names, &filename, &name)) {
	return NULL;
    } else if (!is_stringobject(code) || !is_listobject(consts) ||    \
	       !is_listobject(names) || !is_stringobject(filename) || \
	       !is_stringobject(name)) {
	err_setstr(TypeError, "expected a string of compiled code, a list of constants,   \
                               a list of names used, a string filename, and a string name \
                               as args");
	return NULL;
    }
    return (object *)newcodeobject(code, consts, names, filename, name);
}
    

object*
new_module(unused, args)
    object* unused;
    object* args;
{
    object* name;
  
    if (!getargs(args, "S", &name)) {
	err_setstr(TypeError, "expected a string name as args");
	return NULL;
    }
    return newmoduleobject(getstringvalue(name));
}
    

static struct methodlist new_methods[] = {
    { "instancemethod", new_instancemethod },
    { "function",       new_function },
    { "code",           new_code },
    { "module",         new_module },
    {NULL,		NULL}		/* sentinel */
};

void
initnew()
{
    initmodule("new", new_methods);
}
