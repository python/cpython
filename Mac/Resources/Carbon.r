/*
 *  Permit this Carbon application to launch on OS X
 *
 *  by Josef W. Wankerl
 *  04/11/00
 */
#define USE_CARB_RESOURCE
#ifdef USE_CARB_RESOURCE
/*----------------------------carb € Carbon on OS X launch information --------------------------*/
type 'carb' {
};


resource 'carb'(0) {
};
#else

#include "macbuildno.h"

#include "patchlevel.h"

type 'plst'
{
    string;
};

resource 'plst' (0)
{
    "CFBundleName = \"MacPython\";\n"  /* short name for Mac OS X */
    "CFBundleShortVersionString = \""PY_VERSION"\";\n"
    "CFBundleGetInfoString = \"MacPython "PY_VERSION".\";\n"
};
#endif
