/*
 * mapdata_ko_KR.c: Map Provider for Korean Encodings
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: mapdata_ko_KR.c,v 1.3 2004/01/17 11:26:10 perky Exp $
 */

#include "Python.h"
#include "cjkcommon.h"
#include "map_ksx1001.h"
#include "map_cp949.h"
#include "map_cp949ext.h"

static struct dbcs_map mapholders[] = {
    {"ksx1001",         NULL,           ksx1001_decmap},
    {"cp949",           cp949_encmap,   NULL},
    {"cp949ext",        NULL,           cp949ext_decmap},
    {"",                NULL,           NULL},
};

static struct PyMethodDef __methods[] = {
    {NULL, NULL},
};

void
init_codecs_mapdata_ko_KR(void)
{
    struct dbcs_map *h;
    PyObject        *m;

    m = Py_InitModule("_codecs_mapdata_ko_KR", __methods);

    for (h = mapholders; h->charset[0] != '\0'; h++) {
        char     mhname[256] = "__map_";

        strcpy(mhname + sizeof("__map_") - 1, h->charset);
        PyModule_AddObject(m, mhname, PyCObject_FromVoidPtr(h, NULL));
    }

    if (PyErr_Occurred())
        Py_FatalError("can't initialize the _codecs_mapdata_ko_KR module");
}
