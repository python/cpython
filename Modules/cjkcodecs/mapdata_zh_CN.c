/*
 * mapdata_zh_CN.c: Map Provider for Simplified Chinese Encodings
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: mapdata_zh_CN.c,v 1.3 2004/01/17 11:26:10 perky Exp $
 */

#include "Python.h"
#include "cjkcommon.h"
#include "map_gb2312.h"
#include "map_gbkext.h"
#include "map_gbcommon.h"
#include "map_gb18030ext.h"

static struct dbcs_map mapholders[] = {
    {"gb2312",      NULL,               gb2312_decmap},
    {"gbkext",      NULL,               gbkext_decmap},
    {"gbcommon",    gbcommon_encmap,    NULL},
    {"gb18030ext",  gb18030ext_encmap,  gb18030ext_decmap},
    {"",            NULL,               NULL},
};

static struct PyMethodDef __methods[] = {
    {NULL, NULL},
};

void
init_codecs_mapdata_zh_CN(void)
{
    struct dbcs_map     *h;
    PyObject            *m;

    m = Py_InitModule("_codecs_mapdata_zh_CN", __methods);

    for (h = mapholders; h->charset[0] != '\0'; h++) {
        char     mhname[256] = "__map_";

        strcpy(mhname + sizeof("__map_") - 1, h->charset);
        PyModule_AddObject(m, mhname, PyCObject_FromVoidPtr(h, NULL));
    }

    if (PyErr_Occurred())
        Py_FatalError("can't initialize the _codecs_mapdata_zh_CN module");
}
