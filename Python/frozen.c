
/* Frozen modules initializer */

#include "Python.h"
#include "importlib.h"
#include "importlib_external.h"
#include "importlib_zipimport.h"

/* In order to test the support for frozen modules, by default we
   define a single frozen module, __hello__.  Loading it will print
   some famous words... */

/* Run "make regen-frozen" to regen the file below (e.g. after a bytecode
 * format change).  The include file defines _Py_M__hello as an array of bytes.
 */
#include "frozen_hello.h"

#define SIZE (int)sizeof(_Py_M__hello)

static const struct _frozen _PyImport_FrozenModules[] = {
    /* importlib */
    {"_frozen_importlib", _Py_M__importlib_bootstrap,
        (int)sizeof(_Py_M__importlib_bootstrap)},
    {"_frozen_importlib_external", _Py_M__importlib_bootstrap_external,
        (int)sizeof(_Py_M__importlib_bootstrap_external)},
    {"zipimport", _Py_M__zipimport,
        (int)sizeof(_Py_M__zipimport)},
    /* Test module */
    {"__hello__", _Py_M__hello, SIZE},
    /* Test package (negative size indicates package-ness) */
    {"__phello__", _Py_M__hello, -SIZE},
    {"__phello__.spam", _Py_M__hello, SIZE},
    {0, 0, 0} /* sentinel */
};

/* Embedding apps may change this pointer to point to their favorite
   collection of frozen modules: */

const struct _frozen *PyImport_FrozenModules = _PyImport_FrozenModules;
