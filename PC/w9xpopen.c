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
 * it's own redirected standard handles to the 16-bit child.

 * AKA solution to the problem described in KB: Q150956.
 */    

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

const char *usage =
"This program is used by Python's os.pipe function to\n"
"to work around a limitation in Windows 95/98.  It is\n"
"not designed to be used as stand-alone program.";

int main(int argc, char *argv[])
{
    BOOL bRet;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD exit_code=0;

    if (argc != 2) {
        MessageBox(NULL, usage, argv[0], MB_OK);
        return 1;
    }

    /* Make child process use this app's standard files. */
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    bRet = CreateProcess(
        NULL, argv[1],
        NULL, NULL,
        TRUE, 0,
        NULL, NULL,
        &si, &pi
        );

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
