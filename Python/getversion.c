/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.

******************************************************************/

/* Return the full version string. */

#include "Python.h"

#include "patchlevel.h"

const char *
Py_GetVersion()
{
	static char version[250];
	sprintf(version, "%.80s (%.80s) %.80s", PY_VERSION,
		Py_GetBuildInfo(), Py_GetCompiler());
	return version;
}
