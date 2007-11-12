/* hypot() replacement */

#include "Python.h"

double hypot(double x, double y)
{
	double yx;

	x = fabs(x);
	y = fabs(y);
	if (x < y) {
		double temp = x;
		x = y;
		y = temp;
	}
	if (x == 0.)
		return 0.;
	else {
		yx = y/x;
		return x*sqrt(1.+yx*yx);
	}
}
