/*
 * mapdata_ja_JP.c: Map Provider for Japanese Encodings
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: mapdata_ja_JP.c,v 1.3 2004/01/17 11:26:10 perky Exp $
 */

#include "Python.h"
#include "cjkcommon.h"
#include "map_jisx0208.h"
#include "map_jisx0212.h"
#include "map_jisx0213.h"
#include "map_jisxcommon.h"
#include "map_cp932ext.h"

static struct dbcs_map mapholders[] = {
    {"jisx0208",        NULL,               jisx0208_decmap},
    {"jisx0212",        NULL,               jisx0212_decmap},
    {"jisxcommon",      jisxcommon_encmap,  NULL},
    {"jisx0213_1_bmp",  NULL,               jisx0213_1_bmp_decmap},
    {"jisx0213_2_bmp",  NULL,               jisx0213_2_bmp_decmap},
    {"jisx0213_bmp",    jisx0213_bmp_encmap, NULL},
    {"jisx0213_1_emp",  NULL,               jisx0213_1_emp_decmap},
    {"jisx0213_2_emp",  NULL,               jisx0213_2_emp_decmap},
    {"jisx0213_emp",    jisx0213_emp_encmap, NULL},
    {"cp932ext",        cp932ext_encmap,    cp932ext_decmap},
    {"",                NULL,               NULL},
};

static struct PyMethodDef __methods[] = {
    {NULL, NULL},
};

void
init_codecs_mapdata_ja_JP(void)
{
    struct dbcs_map *h;
    PyObject        *m;

    m = Py_InitModule("_codecs_mapdata_ja_JP", __methods);

    for (h = mapholders; h->charset[0] != '\0'; h++) {
        char     mhname[256] = "__map_";

        strcpy(mhname + sizeof("__map_") - 1, h->charset);
        PyModule_AddObject(m, mhname, PyCObject_FromVoidPtr(h, NULL));
    }

    if (PyErr_Occurred())
        Py_FatalError("can't initialize the _codecs_mapdata_ja_JP module");
}
