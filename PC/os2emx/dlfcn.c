/* -*- C -*- ***********************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* This library implements dlopen() - Unix-like dynamic linking
 * emulation functions for OS/2 using DosLoadModule() and company.
 */

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSSESMGR
#define INCL_WINPROGRAMLIST
#define INCL_WINFRAMEMGR
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

typedef struct _track_rec {
    char *name;
    HMODULE handle;
    void *id;
    struct _track_rec *next;
} tDLLchain, *DLLchain;

static DLLchain dlload = NULL;  /* A simple chained list of DLL names */
static char dlerr [256];        /* last error text string */
static void *last_id;

static DLLchain find_id(void *id)
{
    DLLchain tmp;

    for (tmp = dlload; tmp; tmp = tmp->next)
        if (id == tmp->id)
            return tmp;

    return NULL;
}

/* load a dynamic-link library and return handle */
void *dlopen(char *filename, int flags)
{
    HMODULE hm;
    DLLchain tmp;
    char err[256];
    char *errtxt;
    int rc = 0, set_chain = 0;

    for (tmp = dlload; tmp; tmp = tmp->next)
        if (strnicmp(tmp->name, filename, 999) == 0)
            break;

    if (!tmp)
    {
        tmp = (DLLchain) malloc(sizeof(tDLLchain));
        if (!tmp)
            goto nomem;
        tmp->name = strdup(filename);
        tmp->next = dlload;
        set_chain = 1;
    }

    switch (rc = DosLoadModule((PSZ)&err, sizeof(err), filename, &hm))
    {
        case NO_ERROR:
            tmp->handle = hm;
            if (set_chain)
            {
                do
                    last_id++;
                while ((last_id == 0) || (find_id(last_id)));
                tmp->id = last_id;
                dlload = tmp;
            }
            return tmp->id;
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            errtxt = "module `%s' not found";
            break;
        case ERROR_TOO_MANY_OPEN_FILES:
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_SHARING_BUFFER_EXCEEDED:
nomem:
            errtxt = "out of system resources";
            break;
        case ERROR_ACCESS_DENIED:
            errtxt = "access denied";
            break;
        case ERROR_BAD_FORMAT:
        case ERROR_INVALID_SEGMENT_NUMBER:
        case ERROR_INVALID_ORDINAL:
        case ERROR_INVALID_MODULETYPE:
        case ERROR_INVALID_EXE_SIGNATURE:
        case ERROR_EXE_MARKED_INVALID:
        case ERROR_ITERATED_DATA_EXCEEDS_64K:
        case ERROR_INVALID_MINALLOCSIZE:
        case ERROR_INVALID_SEGDPL:
        case ERROR_AUTODATASEG_EXCEEDS_64K:
        case ERROR_RELOCSRC_CHAIN_EXCEEDS_SEGLIMIT:
            errtxt = "invalid module format";
            break;
        case ERROR_INVALID_NAME:
            errtxt = "filename doesn't match module name";
            break;
        case ERROR_SHARING_VIOLATION:
        case ERROR_LOCK_VIOLATION:
            errtxt = "sharing violation";
            break;
        case ERROR_INIT_ROUTINE_FAILED:
            errtxt = "module initialization failed";
            break;
        default:
            errtxt = "cause `%s', error code = %d";
            break;
    }
    snprintf(dlerr, sizeof(dlerr), errtxt, &err, rc);
    if (tmp)
    {
        if (tmp->name)
            free(tmp->name);
        free(tmp);
    }
    return 0;
}

/* return a pointer to the `symbol' in DLL */
void *dlsym(void *handle, char *symbol)
{
    int rc = 0;
    PFN addr;
    char *errtxt;
    int symord = 0;
    DLLchain tmp = find_id(handle);

    if (!tmp)
        goto inv_handle;

    if (*symbol == '#')
        symord = atoi(symbol + 1);

    switch (rc = DosQueryProcAddr(tmp->handle, symord, symbol, &addr))
    {
        case NO_ERROR:
            return (void *)addr;
        case ERROR_INVALID_HANDLE:
inv_handle:
            errtxt = "invalid module handle";
            break;
        case ERROR_PROC_NOT_FOUND:
        case ERROR_INVALID_NAME:
            errtxt = "no symbol `%s' in module";
            break;
        default:
            errtxt = "symbol `%s', error code = %d";
            break;
    }
    snprintf(dlerr, sizeof(dlerr), errtxt, symbol, rc);
    return NULL;
}

/* free dynamically-linked library */
int dlclose(void *handle)
{
    int rc;
    DLLchain tmp = find_id(handle);

    if (!tmp)
        goto inv_handle;

    switch (rc = DosFreeModule(tmp->handle))
    {
        case NO_ERROR:
            free(tmp->name);
            dlload = tmp->next;
            free(tmp);
            return 0;
        case ERROR_INVALID_HANDLE:
inv_handle:
            strcpy(dlerr, "invalid module handle");
            return -1;
        case ERROR_INVALID_ACCESS:
            strcpy(dlerr, "access denied");
            return -1;
        default:
            return -1;
    }
}

/* return a string describing last occurred dl error */
char *dlerror()
{
    return dlerr;
}
