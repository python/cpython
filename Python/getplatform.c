/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

#include "Python.h"

#ifndef PLATFORM
#define PLATFORM "unknown"
#endif

const char *
Py_GetPlatform(void)
{
	return PLATFORM;
}
