/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* Portable fmod(x, y) implementation for systems that don't have it */

#include "config.h"

#include "mymath.h"
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
