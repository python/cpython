/*
 * mapdata_zh_TW.c: Map Provider for Traditional Chinese Encodings
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: mapdata_zh_TW.c,v 1.3 2004/01/17 11:26:10 perky Exp $
 */

#include "Python.h"
#include "cjkcommon.h"
#include "map_big5.h"
#include "map_cp950ext.h"

static struct dbcs_map mapholders[] = {
    {"big5",        big5_encmap,        big5_decmap},
    {"cp950ext",    cp950ext_encmap,    cp950ext_decmap},
    {"",            NULL,               NULL},
};

static struct PyMethodDef __methods[] = {
    {NULL, NULL},
};

void
init_codecs_mapdata_zh_TW(void)
{
    struct dbcs_map *h;
    PyObject        *m;

    m = Py_InitModule("_codecs_mapdata_zh_TW", __methods);

    for (h = mapholders; h->charset[0] != '\0'; h++) {
        char     mhname[256] = "__map_";

        strcpy(mhname + sizeof("__map_") - 1, h->charset);
        PyModule_AddObject(m, mhname, PyCObject_FromVoidPtr(h, NULL));
    }

    if (PyErr_Occurred())
        Py_FatalError("can't initialize the _codecs_mapdata_zh_TW module");
}
