/* ------------------------------------------------------------------------

   unicodedatabase -- The Unicode 3.0 data base.

   Data was extracted from the Unicode 3.0 UnicodeData.txt file.

Written by Marc-Andre Lemburg (mal@lemburg.com).

Copyright (c) Corporation for National Research Initiatives.

   ------------------------------------------------------------------------ */

#include <stdlib.h>
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
