/* Python Inline Platform Utilities Implementation */
#include "python_inline_platform.h"
#include "Python.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef MS_WINDOWS
int 
python_inline_convert_args(int argc, char **argv_bytes, wchar_t ***argv_out)
{
    if (!argv_bytes || !argv_out) {
        return PYTHON_INLINE_ERROR_ARGS;
    }
    
    /* Allocate array of wchar_t* pointers */
    wchar_t **argv = PyMem_Malloc(argc * sizeof(wchar_t*));
    if (!argv) {
        fprintf(stderr, "Memory allocation failed\n");
        return PYTHON_INLINE_ERROR_MEMORY;
    }
    
    /* Convert each string from char* to wchar_t* */
    for (int i = 0; i < argc; i++) {
        size_t len = strlen(argv_bytes[i]) + 1;
        argv[i] = PyMem_Malloc(len * sizeof(wchar_t));
        if (!argv[i]) {
            /* Clean up previously allocated strings */
            for (int j = 0; j < i; j++) {
                PyMem_Free(argv[j]);
            }
            PyMem_Free(argv);
            fprintf(stderr, "Memory allocation failed\n");
            return PYTHON_INLINE_ERROR_MEMORY;
        }
        
        /* Convert using standard library function */
        if (mbstowcs(argv[i], argv_bytes[i], len) == (size_t)-1) {
            /* Clean up on conversion error */
            for (int j = 0; j <= i; j++) {
                PyMem_Free(argv[j]);
            }
            PyMem_Free(argv);
            fprintf(stderr, "Character conversion failed\n");
            return PYTHON_INLINE_ERROR_ARGS;
        }
    }
    
    *argv_out = argv;
    return PYTHON_INLINE_SUCCESS;
}

void 
python_inline_free_converted_args(int argc, wchar_t **argv)
{
    if (!argv) return;
    
    /* Free each converted string */
    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
            PyMem_Free(argv[i]);
        }
    }
    
    /* Free the array itself */
    PyMem_Free(argv);
}
#endif /* !MS_WINDOWS */
