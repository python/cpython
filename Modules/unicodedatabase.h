/* ------------------------------------------------------------------------

   unicodedatabase -- The Unicode 3.0 data base.

   Data was extracted from the Unicode 3.0 UnicodeData.txt file.

Written by Marc-Andre Lemburg (mal@lemburg.com).

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
    const char *decomposition;		/* pointer to the decomposition
					   string or NULL */
} _PyUnicode_DatabaseRecord;

/* --- Unicode category names --------------------------------------------- */

extern const char *_PyUnicode_CategoryNames[32];
extern const char *_PyUnicode_BidirectionalNames[21];

/* --- Unicode Database --------------------------------------------------- */

extern const _PyUnicode_DatabaseRecord *_PyUnicode_Database_GetRecord(int ch);
