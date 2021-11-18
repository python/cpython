
/* Frozen modules bootstrap */

/* This file is linked with "bootstrap Python"
   which is used (only) to run Tools/scripts/deepfreeze.py. */

#include "Python.h"
#include "pycore_import.h"

/* Includes for frozen modules: */
#include "frozen_modules/importlib._bootstrap.h"
#include "frozen_modules/importlib._bootstrap_external.h"
#include "frozen_modules/zipimport.h"
/* End includes */

/* Note that a negative size indicates a package. */

static const struct _frozen bootstrap_modules[] = {
    {"_frozen_importlib", _Py_M__importlib__bootstrap, (int)sizeof(_Py_M__importlib__bootstrap)},
    {"_frozen_importlib_external", _Py_M__importlib__bootstrap_external, (int)sizeof(_Py_M__importlib__bootstrap_external)},
    {"zipimport", _Py_M__zipimport, (int)sizeof(_Py_M__zipimport)},
    {0, 0, 0} /* bootstrap sentinel */
};
static const struct _frozen stdlib_modules[] = {
    {0, 0, 0} /* stdlib sentinel */
};
static const struct _frozen test_modules[] = {
    {0, 0, 0} /* test sentinel */
};
const struct _frozen *_PyImport_FrozenBootstrap = bootstrap_modules;
const struct _frozen *_PyImport_FrozenStdlib = stdlib_modules;
const struct _frozen *_PyImport_FrozenTest = test_modules;

static const struct _module_alias aliases[] = {
    {"_frozen_importlib", "importlib._bootstrap"},
    {"_frozen_importlib_external", "importlib._bootstrap_external"},
    {0, 0} /* aliases sentinel */
};
const struct _module_alias *_PyImport_FrozenAliases = aliases;


/* Embedding apps may change this pointer to point to their favorite
   collection of frozen modules: */

const struct _frozen *PyImport_FrozenModules = NULL;
