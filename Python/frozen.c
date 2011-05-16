
/* Dummy frozen modules initializer */

#include "Python.h"

/* In order to test the support for frozen modules, by default we
   define a single frozen module, __hello__.  Loading it will print
   some famous words... */

/* To regenerate this data after the bytecode or marshal format has changed,
   go to ../Tools/freeze/ and freeze the flag.py file; then copy and paste
   the appropriate bytes from M___main__.c. */

static unsigned char M___hello__[] = {
    99,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,
    0,64,0,0,0,115,20,0,0,0,100,2,0,90,1,0,
    101,2,0,100,0,0,131,1,0,1,100,1,0,83,40,3,
    0,0,0,117,12,0,0,0,72,101,108,108,111,32,119,111,
    114,108,100,33,78,84,40,3,0,0,0,117,4,0,0,0,
    84,114,117,101,117,11,0,0,0,105,110,105,116,105,97,108,
    105,122,101,100,117,5,0,0,0,112,114,105,110,116,40,0,
    0,0,0,40,0,0,0,0,40,0,0,0,0,117,7,0,
    0,0,102,108,97,103,46,112,121,117,8,0,0,0,60,109,
    111,100,117,108,101,62,1,0,0,0,115,2,0,0,0,6,
    1,
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
