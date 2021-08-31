
/* Frozen modules bootstrap */

/* This file is linked with "bootstrap Python"
   which is used (only) to run Tools/codegen/codegen.py. */

#include "Python.h"

#include "frozen_modules/importlib__bootstrap.h"
#include "frozen_modules/importlib__bootstrap_external.h"
#include "frozen_modules/zipimport.h"

static const struct _frozen _PyImport_FrozenModules[] = {
    /* importlib */
    {"_frozen_importlib", _Py_M__importlib__bootstrap,
     (int)sizeof(_Py_M__importlib__bootstrap)},
    {"_frozen_importlib_external", _Py_M__importlib__bootstrap_external,
     (int)sizeof(_Py_M__importlib__bootstrap_external)},
    {"zipimport", _Py_M__zipimport, (int)sizeof(_Py_M__zipimport)},
    {0, 0, 0} /* sentinel */
};

const struct _frozen *PyImport_FrozenModules = _PyImport_FrozenModules;
