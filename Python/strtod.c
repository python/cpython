/* This is not a proper strtod() implementation, but sufficient for Python.
   Python won't detect floating point constant overflow, though. */

#include <string.h>

extern double atof();

/*ARGSUSED*/
double
strtod(p, pp)
	char *p;
	char **pp;
{
	if (pp)
		*pp = strchr(p, '\0');
	return atof(p);
}
