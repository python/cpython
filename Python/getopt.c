/*---------------------------------------------------------------------------*
 * <RCS keywords>
 *
 * C++ Library
 *
 * Copyright 1992-1994, David Gottner
 *
 *                    All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice, this permission notice and
 * the following disclaimer notice appear unmodified in all copies.
 *
 * I DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL I
 * BE LIABLE FOR ANY SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA, OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *---------------------------------------------------------------------------*/

/* Modified to support --help and --version, as well as /? on Windows
 * by Georg Brandl. */

#include <Python.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "pycore_getopt.h"        // struct _PyOS_GetOpt


/* Python command line short and long options */

#define SHORT_OPTS L"bBc:dEhiIm:OPqRsStuvVW:xX:?"

typedef struct {
    const wchar_t *name;
    int has_arg;
    int val;
} _PyOS_LongOption;

static const _PyOS_LongOption longopts[] = {
    /* name, has_arg, val (used in switch in initconfig.c) */
    {L"check-hash-based-pycs", 1, 1},
    {L"help-all", 0, 2},
    {L"help-env", 0, 3},
    {L"help-xoptions", 0, 4},
    {NULL, 0, -1},                     /* sentinel */
};


void
_PyOS_GetOpt_Init(struct _PyOS_GetOpt *getopt,
                  Py_ssize_t argc, wchar_t * const *argv)
{
    getopt->error = 1;
    getopt->index = 1;
    getopt->arg = NULL;
    getopt->ptr = L"";
    getopt->argc = argc;
    getopt->argv = argv;
}

// Parse a command line option.
//
// Return a character for short option (ex: return 'h' for -h).
// Return a number for long options (see 'longopts' array).
// Return '_' on unknown option or missing argument.
// Return -1 when done.
//
// Return 'h' for --help and return 'V' for --version.
//
// If an option has an argument, set getopt->arg to the argument.
// If getopt->error is non-error, write error messages to stderr.
int
_PyOS_GetOpt(struct _PyOS_GetOpt *getopt)
{
    // Local copy of read-only members to omit "getopt->"
    const Py_ssize_t argc = getopt->argc;
    wchar_t * const *argv = getopt->argv;
    const int error = getopt->error;

    if (*getopt->ptr == '\0') {
        if (getopt->index >= argc) {
            return -1;
        }

        const wchar_t *arg = argv[getopt->index];
#ifdef MS_WINDOWS
        if (wcscmp(arg, L"/?") == 0) {
            ++getopt->index;
            return 'h';
        }
#endif

        if (arg[0] != L'-' || arg[1] == L'\0' /* lone dash */ ) {
            return -1;
        }

        if (wcscmp(arg, L"--") == 0) {
            ++getopt->index;
            return -1;
        }
        if (wcscmp(arg, L"--help") == 0) {
            ++getopt->index;
            return 'h';
        }
        if (wcscmp(arg, L"--version") == 0) {
            ++getopt->index;
            return 'V';
        }

        getopt->ptr = &argv[getopt->index++][1];
    }

    wchar_t option = *getopt->ptr++;
    if (option == L'\0') {
        return -1;
    }

    if (option == L'-') {
        // Parse long option.
        if (*getopt->ptr == L'\0') {
            if (error) {
                fprintf(stderr, "Expected long option\n");
            }
            return -1;
        }
        int longindex = 0;
        const _PyOS_LongOption *opt;
        for (opt = &longopts[longindex]; opt->name; opt = &longopts[++longindex]) {
            if (wcscmp(opt->name, getopt->ptr) == 0) {
                break;
            }
        }

        if (!opt->name) {
            if (error) {
                fprintf(stderr, "Unknown option: %ls\n", argv[getopt->index - 1]);
            }
            return '_';
        }

        getopt->ptr = L"";
        if (!opt->has_arg) {
            return opt->val;
        }
        if (getopt->index >= argc) {
            if (error) {
                fprintf(stderr, "Argument expected for the %ls options\n",
                        argv[getopt->index - 1]);
            }
            return '_';
        }
        getopt->arg = argv[getopt->index++];
        return opt->val;
    }

    wchar_t *ptr = wcschr(SHORT_OPTS, option);
    if (ptr == NULL) {
        if (error) {
            fprintf(stderr, "Unknown option: -%lc\n", option);
        }
        return '_';
    }

    if (*(ptr + 1) == L':') {
        if (*getopt->ptr != L'\0') {
            getopt->arg  = getopt->ptr;
            getopt->ptr = L"";
        }
        else {
            if (getopt->index >= argc) {
                if (error) {
                    fprintf(stderr,
                        "Argument expected for the -%lc option\n", option);
                }
                return '_';
            }

            getopt->arg = argv[getopt->index++];
        }
    }

    return option;
}
