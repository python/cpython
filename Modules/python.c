/* Minimal main program -- everything is loaded from the library */

#include "Python.h"

int
main(int argc, char **argv)
{
	return Py_Main(argc, argv);
}
