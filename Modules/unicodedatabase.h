/* ------------------------------------------------------------------------

   unicodedatabase -- The Unicode 3.0 data base.

   Data was extracted from the Unicode 3.0 UnicodeData.txt file.

   Written by Marc-Andre Lemburg (mal@lemburg.com).
   Modified for Python 2.0 by Fredrik Lundh (fredrik@pythonware.com)

   Copyright (c) Corporation for National Research Initiatives.

   ------------------------------------------------------------------------ */

/* --- Unicode database entry --------------------------------------------- */

typedef struct {
    const unsigned char category;	/* index into
					   _PyUnicode_CategoryNames */
    const unsigned char	combining; 	/* combining class value 0 - 255 */
    const unsigned char	bidirectional; 	/* index into
					   _PyUnicode_BidirectionalNames */
    const unsigned char mirrored;	/* true if mirrored in bidir mode */
} _PyUnicode_DatabaseRecord;

/* --- Unicode category names --------------------------------------------- */

extern const char *_PyUnicode_CategoryNames[];
extern const char *_PyUnicode_BidirectionalNames[];

/* --- Unicode Database --------------------------------------------------- */

extern const _PyUnicode_DatabaseRecord *_PyUnicode_Database_GetRecord(int ch);
extern const char *_PyUnicode_Database_GetDecomposition(int ch);
