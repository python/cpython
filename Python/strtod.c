/* This is not a proper strtod() implementation, but sufficient for Python.
   Python won't detect floating point constant overflow, though. */

extern int strlen();
extern double atof();

double
strtod(p, pp)
	char *p;
	char **pp;
{
	if (pp)
		*pp = p + strlen(p);
	return atof(p);
}
