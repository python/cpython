#include "Python.h"
#include "osdefs.h"


#ifndef PYTHONPATH
#define PYTHONPATH ".:/usr/local/lib/python"
#endif


/* Return the initial python search path.  This is called once from
   initsys() to initialize sys.path.  The environment variable
   PYTHONPATH is fetched and the default path appended.  The default
   path may be passed to the preprocessor; if not, a system-dependent
   default is used. */

char *
getpythonpath()
{
	char *path = getenv("PYTHONPATH");
	char *defpath = PYTHONPATH;
	static char *buf = NULL;
	char *p;
	int n;

	if (path == NULL)
		path = "";
	n = strlen(path) + strlen(defpath) + 2;
	if (buf != NULL) {
		free(buf);
		buf = NULL;
	}
	buf = malloc(n);
	if (buf == NULL)
		Py_FatalError("not enough memory to copy module search path");
	strcpy(buf, path);
	p = buf + strlen(buf);
	if (p != buf)
		*p++ = DELIM;
	strcpy(p, defpath);
	return buf;
}
