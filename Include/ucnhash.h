
#include <Python.h>
#include <stdlib.h>

/* --- C API ----------------------------------------------------*/
/* C API for usage by other Python modules */
typedef struct _Py_UCNHashAPI
{
    unsigned long cKeys;
    unsigned long cchMax;
    unsigned long (*hash)(const char *key, unsigned int cch);
    const void *(*getValue)(unsigned long iKey);
} _Py_UCNHashAPI;

typedef struct 
{
    const char *pszUCN;
    unsigned int uiValue;
} _Py_UnicodeCharacterName;

