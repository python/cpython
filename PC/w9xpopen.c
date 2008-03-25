/*
 * w9xpopen.c
 *
 * Serves as an intermediate stub Win32 console application to
 * avoid a hanging pipe when redirecting 16-bit console based
 * programs (including MS-DOS console based programs and batch
 * files) on Window 95 and Windows 98.
 *
 * This program is to be launched with redirected standard
 * handles. It will launch the command line specified 16-bit
 * console based application in the same console, forwarding
 * its own redirected standard handles to the 16-bit child.

 * AKA solution to the problem described in KB: Q150956.
 */    

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>  /* for malloc and its friends */

const char *usage =
"This program is used by Python's os.popen function\n"
"to work around a limitation in Windows 95/98.  It is\n"
"not designed to be used as a stand-alone program.";

int main(int argc, char *argv[])
{
    BOOL bRet;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD exit_code=0;
    size_t cmdlen = 0;
    int i;
    char *cmdline, *cmdlinefill;

    if (argc < 2) {
        if (GetFileType(GetStdHandle(STD_INPUT_HANDLE))==FILE_TYPE_CHAR)
            /* Attached to a console, and therefore not executed by Python
               Display a message box for the inquisitive user
            */
            MessageBox(NULL, usage, argv[0], MB_OK);
        else {
            /* Eeek - executed by Python, but args are screwed!
               Write an error message to stdout so there is at
               least some clue for the end user when it appears
               in their output.
               A message box would be hidden and blocks the app.
             */
            fprintf(stdout, "Internal popen error - no args specified\n%s\n", usage);
        }
        return 1;
    }
    /* Build up the command-line from the args.
       Args with a space are quoted, existing quotes are escaped.
       To keep things simple calculating the buffer size, we assume
       every character is a quote - ie, we allocate double what we need
       in the worst case.  As this is only double the command line passed
       to us, there is a good chance this is reasonably small, so the total 
       allocation will almost always be < 512 bytes.
    */
    for (i=1;i<argc;i++)
        cmdlen += strlen(argv[i])*2 + 3; /* one space, maybe 2 quotes */
    cmdline = cmdlinefill = (char *)malloc(cmdlen+1);
    if (cmdline == NULL)
        return -1;
    for (i=1;i<argc;i++) {
        const char *arglook;
        int bQuote = strchr(argv[i], ' ') != NULL;
        if (bQuote)
            *cmdlinefill++ = '"';
        /* escape quotes */
        for (arglook=argv[i];*arglook;arglook++) {
            if (*arglook=='"')
                *cmdlinefill++ = '\\';
            *cmdlinefill++ = *arglook;
        }
        if (bQuote)
            *cmdlinefill++ = '"';
        *cmdlinefill++ = ' ';
    }
    *cmdlinefill = '\0';

    /* Make child process use this app's standard files. */
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    bRet = CreateProcess(
        NULL, cmdline,
        NULL, NULL,
        TRUE, 0,
        NULL, NULL,
        &si, &pi
        );

    free(cmdline);

    if (bRet) {
        if (WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_FAILED) {
	    GetExitCodeProcess(pi.hProcess, &exit_code);
	}
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return exit_code;
    }

    return 1;
}
