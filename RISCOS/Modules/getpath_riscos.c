#include "Python.h"
#include "osdefs.h"

static char *prefix, *exec_prefix, *progpath, *module_search_path=NULL;

static void
calculate_path()
{ 
	char *pypath = getenv("Python$Path");
	if (pypath) {
		int pathlen = strlen(pypath);
		module_search_path = malloc(pathlen + 1);
		if (module_search_path) 
			strncpy(module_search_path, pypath, pathlen + 1);
		else {
			fprintf(stderr, 
				"Not enough memory for dynamic PYTHONPATH.\n"
				"Using default static PYTHONPATH.\n");
		}
	}
	if (!module_search_path) 
		module_search_path = "<Python$Dir>.Lib";
	prefix = "<Python$Dir>";
	exec_prefix = prefix;
	progpath = Py_GetProgramName();
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
