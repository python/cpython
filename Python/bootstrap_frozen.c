
/* Frozen modules bootstrap */

/* This file is linked with "bootstrap Python"
   which is used (only) to run Tools/scripts/deepfreeze.py. */

#include "Python.h"
#include "pycore_import.h"

#include "frozen_modules/importlib._bootstrap.h"
#include "frozen_modules/importlib._bootstrap_external.h"
#include "frozen_modules/zipimport.h"

static const struct _frozen _PyImport_FrozenModules[] = {
    /* import system */
    {"_frozen_importlib", _Py_M__importlib__bootstrap,
        (int)sizeof(_Py_M__importlib__bootstrap)},
    {"_frozen_importlib_external", _Py_M__importlib__bootstrap_external,
        (int)sizeof(_Py_M__importlib__bootstrap_external)},
    {"zipimport", _Py_M__zipimport, (int)sizeof(_Py_M__zipimport)},

    {0, 0, 0} /* sentinel */
};

static const struct _module_alias aliases[] = {
    {"_frozen_importlib", "importlib._bootstrap"},
    {"_frozen_importlib_external", "importlib._bootstrap_external"},
    {0, 0} /* aliases sentinel */
};
const struct _module_alias *_PyImport_FrozenAliases = aliases;

const struct _frozen *PyImport_FrozenModules = _PyImport_FrozenModules;
