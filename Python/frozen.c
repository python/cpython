
/* Dummy frozen modules initializer */

#include "Python.h"

/* In order to test the support for frozen modules, by default we
   define a single frozen module, __hello__.  Loading it will print
   some famous words... */

static unsigned char M___hello__[] = {
	99,0,0,0,0,1,0,0,0,115,15,0,0,0,127,0,
	0,127,1,0,100,0,0,71,72,100,1,0,83,40,2,0,
	0,0,115,14,0,0,0,72,101,108,108,111,32,119,111,114,
	108,100,46,46,46,78,40,0,0,0,0,40,0,0,0,0,
	115,8,0,0,0,104,101,108,108,111,46,112,121,115,1,0,
	0,0,63,1,0,115,0,0,0,0,
};

static struct _frozen _PyImport_FrozenModules[] = {
	/* Test module */
	{"__hello__", M___hello__, 90},
	/* Test package (negative size indicates package-ness) */
	{"__phello__", M___hello__, -90},
	{"__phello__.spam", M___hello__, 90},
	{0, 0, 0} /* sentinel */
};

/* Embedding apps may change this pointer to point to their favorite
   collection of frozen modules: */

struct _frozen *PyImport_FrozenModules = _PyImport_FrozenModules;
