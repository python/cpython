
/* Portable fmod(x, y) implementation for systems that don't have it */

#include "pyconfig.h"

#include "pyport.h"
#include <errno.h>

double
fmod(double x, double y)
{
	double i, f;
	
	if (y == 0.0) {
		errno = EDOM;
		return 0.0;
	}
	
	/* return f such that x = i*y + f for some integer i
	   such that |f| < |y| and f has the same sign as x */
	
	i = floor(x/y);
	f = x - i*y;
	if ((x < 0.0) != (y < 0.0))
		f = f-y;
	return f;
}
