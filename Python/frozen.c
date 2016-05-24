
/* Dummy frozen modules initializer */

#include "Python.h"
#include "importlib.h"
#include "importlib_external.h"

/* In order to test the support for frozen modules, by default we
   define a single frozen module, __hello__.  Loading it will print
   some famous words... */

/* To regenerate this data after the bytecode or marshal format has changed,
   go to ../Tools/freeze/ and freeze the flag.py file; then copy and paste
   the appropriate bytes from M___main__.c. */

static unsigned char M___hello__[] = {
    227,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,
    0,64,0,0,0,115,16,0,0,0,100,0,90,0,101,1,
    100,1,131,1,1,0,100,2,83,0,41,3,84,122,12,72,
    101,108,108,111,32,119,111,114,108,100,33,78,41,2,218,11,
    105,110,105,116,105,97,108,105,122,101,100,218,5,112,114,105,
    110,116,169,0,114,3,0,0,0,114,3,0,0,0,250,22,
    46,47,84,111,111,108,115,47,102,114,101,101,122,101,47,102,
    108,97,103,46,112,121,218,8,60,109,111,100,117,108,101,62,
    1,0,0,0,115,2,0,0,0,4,1,
};

#define SIZE (int)sizeof(M___hello__)

static const struct _frozen _PyImport_FrozenModules[] = {
    /* importlib */
    {"_frozen_importlib", _Py_M__importlib, (int)sizeof(_Py_M__importlib)},
    {"_frozen_importlib_external", _Py_M__importlib_external,
        (int)sizeof(_Py_M__importlib_external)},
    /* Test module */
    {"__hello__", M___hello__, SIZE},
    /* Test package (negative size indicates package-ness) */
    {"__phello__", M___hello__, -SIZE},
    {"__phello__.spam", M___hello__, SIZE},
    {0, 0, 0} /* sentinel */
};

/* Embedding apps may change this pointer to point to their favorite
   collection of frozen modules: */

const struct _frozen *PyImport_FrozenModules = _PyImport_FrozenModules;
