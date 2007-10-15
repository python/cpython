
/* Dummy frozen modules initializer */

#include "Python.h"

/* In order to test the support for frozen modules, by default we
   define a single frozen module, __hello__.  Loading it will print
   some famous words... */

/* To regenerate this data after the bytecode or marshal format has changed,
   go to ../Tools/freeze/ and freeze the hello.py file; then copy and paste
   the appropriate bytes from M___main__.c. */

static unsigned char M___hello__[] = {
	99,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,
	0,64,0,0,0,115,14,0,0,0,101,0,0,100,0,0,
	131,1,0,1,100,1,0,83,40,2,0,0,0,117,14,0,
	0,0,72,101,108,108,111,32,119,111,114,108,100,46,46,46,
	78,40,1,0,0,0,117,5,0,0,0,112,114,105,110,116,
	40,0,0,0,0,40,0,0,0,0,40,0,0,0,0,117,
	8,0,0,0,104,101,108,108,111,46,112,121,117,8,0,0,
	0,60,109,111,100,117,108,101,62,1,0,0,0,115,0,0,
	0,0,
};

#define SIZE (int)sizeof(M___hello__)

static struct _frozen _PyImport_FrozenModules[] = {
	/* Test module */
	{"__hello__", M___hello__, SIZE},
	/* Test package (negative size indicates package-ness) */
	{"__phello__", M___hello__, -SIZE},
	{"__phello__.spam", M___hello__, SIZE},
	{0, 0, 0} /* sentinel */
};

/* Embedding apps may change this pointer to point to their favorite
   collection of frozen modules: */

struct _frozen *PyImport_FrozenModules = _PyImport_FrozenModules;
