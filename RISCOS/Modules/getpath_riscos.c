#include "Python.h"
#include "osdefs.h"

static char *prefix,*exec_prefix,*progpath,*module_search_path=0;

static void
calculate_path()
{ char *pypath=getenv("Python$Path");
  if(pypath)
  { module_search_path=malloc(strlen(pypath)+1);
    if (module_search_path) sprintf(module_search_path,"%s",pypath);
    else
    {  /* We can't exit, so print a warning and limp along */
       fprintf(stderr, "Not enough memory for dynamic PYTHONPATH.\n");
       fprintf(stderr, "Using default static PYTHONPATH.\n");
    }
  }
  if(!module_search_path) module_search_path = "<Python$Dir>.Lib";
  prefix="";
  exec_prefix=prefix;
  progpath="<Python$Dir>";
}

/* External interface */

char *
Py_GetPath()
{
	if (!module_search_path)
		calculate_path();
	return module_search_path;
}

char *
Py_GetPrefix()
{
	if (!module_search_path)
		calculate_path();
	return prefix;
}

char *
Py_GetExecPrefix()
{
	if (!module_search_path)
		calculate_path();
	return exec_prefix;
}

char *
Py_GetProgramFullPath()
{
	if (!module_search_path)
		calculate_path();
	return progpath;
}
