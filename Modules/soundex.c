/*
  $Header$
  
  Perform soundex comparisons on strings.

  Soundex is an algorithm that hashes English strings into numerical value.
  Strings that sound the same are hashed to the same value.  This allows 
  for non-literal string matching.

  From: David Wayne Williams <dwwillia@iucf.indiana.edu>
*/

#include <string.h>
#include <ctype.h>
#include "Python.h"

void soundex_hash(char *str, char *result)
{
    char *sptr = str;           /* pointer into str */
    char *rptr = result;        /* pointer into result */
    
    if(*str == NULL)
    {
        strcpy(result,"000000");
        return;
    }
            
    /*  Preserve the first character of the input string.
     */
    *(rptr++) = toupper(*(sptr++));
    
    /* Translate the rest of the input string into result.  The following
       transformations are used:

       1) All vowles, W, and H, are skipped.

       2) BFPV = 1
          CGJKQSXZ = 2
          DT = 3
          L = 4
          MN = 5

       3) Only translate the first of adjacent equal translations.  I.E.
          remove duplicate digits.
    */

    for(;(rptr - result) < 6 &&  *sptr != NULL;sptr++)
    {
        switch (toupper(*sptr))
        {
        case 'W':
        case 'H':
        case 'A':
        case 'I':
        case 'O':
        case 'U':
        case 'Y':
            break;
        case 'B':
        case 'F':
        case 'P':
        case 'V':
            if(*(rptr - 1) != '1')
                *(rptr++) = '1';
            break;
        case 'C':
        case 'G':
        case 'J':
        case 'K':
        case 'Q':
        case 'S':
        case 'X':
        case 'Z':
            if(*(rptr - 1) != '2')
                *(rptr++) = '2';
            break;
        case 'D':
        case 'T':
            if(*(rptr - 1) != '3')
                *(rptr++) = '3';
            break;
        case 'L':
            if(*(rptr - 1) != '4')
                *(rptr++) = '4';
            break;
        case 'M':
        case 'N':
            if(*(rptr - 1) != '5')
                *(rptr++) = '5';
            break;
        default:
            break;
        }
    }

    /* Pad 0's on right side of string out to 6 characters.
     */
    for(; rptr < result + 6; rptr++)
        *rptr = '0';

    /* Terminate the result string.
     */
    *(result + 6) = NULL;
}


static PyObject *
sound_similar(PyObject *self, PyObject *args)
{
    char *str1, *str2;
    int return_value;
    char res1[7], res2[7];
    
    if(!PyArg_ParseTuple(args, "ss", &str1, &str2))
        return NULL;

    soundex_hash(str1, res1);
    soundex_hash(str2, res2);

    if(!strcmp(res1,res2))
        return Py_BuildValue("i",1);
    else
        return Py_BuildValue("i",0);
}

/* Python Method Table.
 */
static PyMethodDef SoundexMethods[] =
{
    {"sound_similar", sound_similar, 1},
    {NULL, NULL }               /* sentinel */
};


/* Register the method table.
 */
void
initsoundex()
{
    (void) Py_InitModule("soundex",SoundexMethods);
}
