/*
** Create the 'vers' version resource from information in the
** Python include files.
*/

#include "Types.r"

#include "patchlevel.h"

/* Invent the Mac version from the Python version */
#if PY_RELEASE_LEVEL == PY_RELEASE_LEVEL_ALPHA
#define V_RELEASE alpha
#endif
#if PY_RELEASE_LEVEL == PY_RELEASE_LEVEL_BETA
#define V_RELEASE beta
#endif
#ifndef V_RELEASE
#define V_RELEASE final
#endif

resource 'vers' (1) {
	PY_MAJOR_VERSION,
	(PY_MINOR_VERSION<<4) | (PY_MICRO_VERSION),
	V_RELEASE,
	PY_RELEASE_SERIAL,
	0,
	PY_VERSION,
	$$Format("%s, © Python Software Foundation %s",
			PY_VERSION, $$Date)
};
