/* This is not a proper strtod() implementation, but sufficient for Python.
   Python won't detect floating point constant overflow, though. */

extern int errno;

extern int strlen();
extern double atof();

double
strtod(p, pp)
	char *p;
	char **pp;
{
	double res;

	if (pp)
		*pp = p + strlen(p);
	res = atof(p);
	errno = 0;
	return res;
	
}
