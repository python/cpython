/* ------------------------------------------------------------------------

   unicodedatabase -- The Unicode 3.0 data base.

   Data was extracted from the Unicode 3.0 UnicodeData.txt file.

   Written by Marc-Andre Lemburg (mal@lemburg.com).
   Rewritten for Python 2.0 by Fredrik Lundh (fredrik@pythonware.com)

   Copyright (c) Corporation for National Research Initiatives.

   ------------------------------------------------------------------------ */

#include "Python.h"
#include "unicodedatabase.h"

/* read the actual data from a separate file! */
#include "unicodedata_db.h"

const _PyUnicode_DatabaseRecord *
_PyUnicode_Database_GetRecord(int code)
{
    int index;

    if (code < 0 || code >= 65536)
        index = 0;
    else {
        index = index1[(code>>SHIFT)];
        index = index2[(index<<SHIFT)+(code&((1<<SHIFT)-1))];
    }
    return &_PyUnicode_Database_Records[index];
}

const char *
_PyUnicode_Database_GetDecomposition(int code)
{
    int index;

    if (code < 0 || code >= 65536)
        index = 0;
    else {
        index = decomp_index1[(code>>DECOMP_SHIFT)];
        index = decomp_index2[(index<<DECOMP_SHIFT)+
                             (code&((1<<DECOMP_SHIFT)-1))];
    }
    return decomp_data[index];
}
