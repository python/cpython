/*
 * Helper program for killing lingering python[_d].exe processes before
 * building, thus attempting to avoid build failures due to files being
 * locked.
 */

#include <windows.h>
#include <wchar.h>
#include <tlhelp32.h>
#include <stdio.h>

#pragma comment(lib, "psapi")

#ifdef _DEBUG
#define PYTHON_EXE          (L"python_d.exe")
#define PYTHON_EXE_LEN      (12)
#define KILL_PYTHON_EXE     (L"kill_python_d.exe")
#define KILL_PYTHON_EXE_LEN (17)
#else
#define PYTHON_EXE          (L"python.exe")
#define PYTHON_EXE_LEN      (10)
#define KILL_PYTHON_EXE     (L"kill_python.exe")
#define KILL_PYTHON_EXE_LEN (15)
#endif

int
main(int argc, char **argv)
{
    HANDLE   hp, hsp, hsm; /* process, snapshot processes, snapshot modules */
    DWORD    dac, our_pid;
    size_t   len;
    wchar_t  path[MAX_PATH+1];

    MODULEENTRY32W  me;
    PROCESSENTRY32W pe;

    me.dwSize = sizeof(MODULEENTRY32W);
    pe.dwSize = sizeof(PROCESSENTRY32W);

    memset(path, 0, MAX_PATH+1);

    our_pid = GetCurrentProcessId();

    hsm = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, our_pid);
    if (hsm == INVALID_HANDLE_VALUE) {
        printf("CreateToolhelp32Snapshot[1] failed: %d\n", GetLastError());
        return 1;
    }

    if (!Module32FirstW(hsm, &me)) {
        printf("Module32FirstW[1] failed: %d\n", GetLastError());
        CloseHandle(hsm);
        return 1;
    }

    /*
     * Enumerate over the modules for the current process in order to find
     * kill_process[_d].exe, then take a note of the directory it lives in.
     */
    do {
        if (_wcsnicmp(me.szModule, KILL_PYTHON_EXE, KILL_PYTHON_EXE_LEN))
            continue;

        len = wcsnlen_s(me.szExePath, MAX_PATH) - KILL_PYTHON_EXE_LEN;
        wcsncpy_s(path, MAX_PATH+1, me.szExePath, len); 

        break;

    } while (Module32NextW(hsm, &me));

    CloseHandle(hsm);

    if (path == NULL) {
        printf("failed to discern directory of running process\n");
        return 1;
    }

    /*
     * Take a snapshot of system processes.  Enumerate over the snapshot,
     * looking for python processes.  When we find one, verify it lives
     * in the same directory we live in.  If it does, kill it.  If we're
     * unable to kill it, treat this as a fatal error and return 1.
     * 
     * The rationale behind this is that we're called at the start of the 
     * build process on the basis that we'll take care of killing any
     * running instances, such that the build won't encounter permission
     * denied errors during linking. If we can't kill one of the processes,
     * we can't provide this assurance, and the build shouldn't start.
     */

    hsp = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hsp == INVALID_HANDLE_VALUE) {
        printf("CreateToolhelp32Snapshot[2] failed: %d\n", GetLastError());
        return 1;
    }

    if (!Process32FirstW(hsp, &pe)) {
        printf("Process32FirstW failed: %d\n", GetLastError());
        CloseHandle(hsp);
        return 1;
    }

    dac = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE;
    do {

        /*
         * XXX TODO: if we really wanted to be fancy, we could check the 
         * modules for all processes (not just the python[_d].exe ones)
         * and see if any of our DLLs are loaded (i.e. python33[_d].dll),
         * as that would also inhibit our ability to rebuild the solution.
         * Not worth loosing sleep over though; for now, a simple check 
         * for just the python executable should be sufficient.
         */

        if (_wcsnicmp(pe.szExeFile, PYTHON_EXE, PYTHON_EXE_LEN))
            /* This isn't a python process. */
            continue;

        /* It's a python process, so figure out which directory it's in... */
        hsm = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pe.th32ProcessID);
        if (hsm == INVALID_HANDLE_VALUE)
            /* 
             * If our module snapshot fails (which will happen if we don't own
             * the process), just ignore it and continue.  (It seems different
             * versions of Windows return different values for GetLastError()
             * in this situation; it's easier to just ignore it and move on vs.
             * stopping the build for what could be a false positive.)
             */
             continue;

        if (!Module32FirstW(hsm, &me)) {
            printf("Module32FirstW[2] failed: %d\n", GetLastError());
            CloseHandle(hsp);
            CloseHandle(hsm);
            return 1;
        }

        do {
            if (_wcsnicmp(me.szModule, PYTHON_EXE, PYTHON_EXE_LEN))
                /* Wrong module, we're looking for python[_d].exe... */
                continue;

            if (_wcsnicmp(path, me.szExePath, len))
                /* Process doesn't live in our directory. */
                break;

            /* Python process residing in the right directory, kill it!  */
            hp = OpenProcess(dac, FALSE, pe.th32ProcessID);
            if (!hp) {
                printf("OpenProcess failed: %d\n", GetLastError());
                CloseHandle(hsp);
                CloseHandle(hsm);
                return 1;
            }

            if (!TerminateProcess(hp, 1)) {
                printf("TerminateProcess failed: %d\n", GetLastError());
                CloseHandle(hsp);
                CloseHandle(hsm);
                CloseHandle(hp);
                return 1;
            }

            CloseHandle(hp);
            break;

        } while (Module32NextW(hsm, &me));

        CloseHandle(hsm);

    } while (Process32NextW(hsp, &pe));

    CloseHandle(hsp);

    return 0;
}

/* vi: set ts=8 sw=4 sts=4 expandtab */
