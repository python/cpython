/*
 * venv redirector for Windows
 *
 * This launcher looks for a nearby pyvenv.cfg to find the correct home
 * directory, and then launches the original Python executable from it.
 * The name of this executable is passed as argv[0].
 */

#define __STDC_WANT_LIB_EXT1__ 1

#include <windows.h>
#include <pathcch.h>
#include <fcntl.h>
#include <io.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdbool.h>
#include <tchar.h>
#include <assert.h>

#define MS_WINDOWS
#include "patchlevel.h"

#define MAXLEN PATHCCH_MAX_CCH
#define MSGSIZE 1024

#define RC_NO_STD_HANDLES   100
#define RC_CREATE_PROCESS   101
#define RC_NO_PYTHON        103
#define RC_NO_MEMORY        104
#define RC_NO_VENV_CFG      106
#define RC_BAD_VENV_CFG     107
#define RC_NO_COMMANDLINE   108
#define RC_INTERNAL_ERROR   109

// This should always be defined when we build for real,
// but it's handy to have a definition for quick testing
#ifndef EXENAME
#define EXENAME L"python.exe"
#endif

#ifndef CFGNAME
#define CFGNAME L"pyvenv.cfg"
#endif

static FILE * log_fp = NULL;

void
debug(wchar_t * format, ...)
{
    va_list va;

    if (log_fp != NULL) {
        wchar_t buffer[MAXLEN];
        int r = 0;
        va_start(va, format);
        r = vswprintf_s(buffer, MAXLEN, format, va);
        va_end(va);

        if (r <= 0) {
            return;
        }
        fwprintf(log_fp, L"%ls\n", buffer);
        while (r && isspace(buffer[r])) {
            buffer[r--] = L'\0';
        }
        if (buffer[0]) {
            OutputDebugStringW(buffer);
        }
    }
}


void
formatWinerror(int rc, wchar_t * message, int size)
{
    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, rc, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        message, size, NULL);
}


void
winerror(int err, wchar_t * format, ... )
{
    va_list va;
    wchar_t message[MSGSIZE];
    wchar_t win_message[MSGSIZE];
    int len;

    if (err == 0) {
        err = GetLastError();
    }

    va_start(va, format);
    len = _vsnwprintf_s(message, MSGSIZE, _TRUNCATE, format, va);
    va_end(va);

    formatWinerror(err, win_message, MSGSIZE);
    if (len >= 0) {
        _snwprintf_s(&message[len], MSGSIZE - len, _TRUNCATE, L": %ls",
                     win_message);
    }

#if !defined(_WINDOWS)
    fwprintf(stderr, L"%ls\n", message);
#else
    MessageBoxW(NULL, message, L"Python venv launcher is sorry to say ...",
               MB_OK);
#endif
}


void
error(wchar_t * format, ... )
{
    va_list va;
    wchar_t message[MSGSIZE];

    va_start(va, format);
    _vsnwprintf_s(message, MSGSIZE, _TRUNCATE, format, va);
    va_end(va);

#if !defined(_WINDOWS)
    fwprintf(stderr, L"%ls\n", message);
#else
    MessageBoxW(NULL, message, L"Python venv launcher is sorry to say ...",
               MB_OK);
#endif
}


bool
isEnvVarSet(const wchar_t *name)
{
    /* only looking for non-empty, which means at least one character
       and the null terminator */
    return GetEnvironmentVariableW(name, NULL, 0) >= 2;
}


bool
join(wchar_t *buffer, size_t bufferLength, const wchar_t *fragment)
{
    if (SUCCEEDED(PathCchCombineEx(buffer, bufferLength, buffer, fragment, PATHCCH_ALLOW_LONG_PATHS))) {
        return true;
    }
    return false;
}


bool
split_parent(wchar_t *buffer, size_t bufferLength)
{
    return SUCCEEDED(PathCchRemoveFileSpec(buffer, bufferLength));
}


/*
 * Path calculation
 */

int
calculate_pyvenvcfg_path(wchar_t *pyvenvcfg_path, size_t maxlen)
{
    if (!pyvenvcfg_path) {
        error(L"invalid buffer provided");
        return RC_INTERNAL_ERROR;
    }
    if ((DWORD)maxlen != maxlen) {
        error(L"path buffer is too large");
        return RC_INTERNAL_ERROR;
    }
    if (!GetModuleFileNameW(NULL, pyvenvcfg_path, (DWORD)maxlen)) {
        winerror(GetLastError(), L"failed to read executable directory");
        return RC_NO_COMMANDLINE;
    }
    // Remove 'python.exe' from our path
    if (!split_parent(pyvenvcfg_path, maxlen)) {
        error(L"failed to remove segment from '%ls'", pyvenvcfg_path);
        return RC_NO_COMMANDLINE;
    }
    // Replace with 'pyvenv.cfg'
    if (!join(pyvenvcfg_path, maxlen, CFGNAME)) {
        error(L"failed to append '%ls' to '%ls'", CFGNAME, pyvenvcfg_path);
        return RC_NO_MEMORY;
    }
    // If it exists, return
    if (GetFileAttributesW(pyvenvcfg_path) != INVALID_FILE_ATTRIBUTES) {
        return 0;
    }
    // Otherwise, remove 'pyvenv.cfg' and (probably) 'Scripts'
    if (!split_parent(pyvenvcfg_path, maxlen) ||
        !split_parent(pyvenvcfg_path, maxlen)) {
        error(L"failed to remove segments from '%ls'", pyvenvcfg_path);
        return RC_NO_COMMANDLINE;
    }
    // Replace 'pyvenv.cfg'
    if (!join(pyvenvcfg_path, maxlen, CFGNAME)) {
        error(L"failed to append '%ls' to '%ls'", CFGNAME, pyvenvcfg_path);
        return RC_NO_MEMORY;
    }
    // If it exists, return
    if (GetFileAttributesW(pyvenvcfg_path) != INVALID_FILE_ATTRIBUTES) {
        return 0;
    }
    // Otherwise, we fail
    winerror(GetLastError(), L"failed to locate %ls", CFGNAME);
    return RC_NO_VENV_CFG;
}


/*
 * pyvenv.cfg parsing
 */

static int
find_home_value(const char *buffer, DWORD maxlen, const char **start, DWORD *length)
{
    if (!buffer || !start || !length) {
        error(L"invalid find_home_value parameters()");
        return 0;
    }
    for (const char *s = strstr(buffer, "home");
         s && (size_t)((ptrdiff_t)s - (ptrdiff_t)buffer) < maxlen;
         s = strstr(s + 1, "\nhome")
    ) {
        if (*s == '\n') {
            ++s;
        }
        for (int i = 4; i > 0 && *s; --i, ++s);

        while (*s && iswspace(*s)) {
            ++s;
        }
        if (*s != L'=') {
            continue;
        }

        do {
            ++s;
        } while (*s && iswspace(*s));

        *start = s;
        char *nl = strchr(s, '\n');
        if (nl) {
            while (nl != s && iswspace(nl[-1])) {
                --nl;
            }
            *length = (DWORD)((ptrdiff_t)nl - (ptrdiff_t)s);
        } else {
            *length = (DWORD)strlen(s);
        }
        return 1;
    }
    return 0;
}


int
read_home(const wchar_t *pyvenv_cfg, wchar_t *home_path, size_t maxlen)
{
    HANDLE hFile = CreateFileW(pyvenv_cfg, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        winerror(GetLastError(), L"failed to open '%ls'", pyvenv_cfg);
        return RC_BAD_VENV_CFG;
    }

    // 8192 characters ought to be enough for anyone
    // (doubled compared to the old implementation!)
    char buffer[8192];
    DWORD len;
    if (!ReadFile(hFile, buffer, sizeof(buffer) - 1, &len, NULL)) {
        winerror(GetLastError(), L"failed to read '%ls'", pyvenv_cfg);
        CloseHandle(hFile);
        return RC_BAD_VENV_CFG;
    }
    CloseHandle(hFile);
    // Ensure null termination
    buffer[len] = '\0';

    char *home;
    DWORD home_len;
    if (!find_home_value(buffer, sizeof(buffer), &home, &home_len)) {
        error(L"no home= specified in '%ls'", pyvenv_cfg);
        return RC_BAD_VENV_CFG;
    }

    if ((DWORD)maxlen != maxlen) {
        maxlen = 8192;
    }
    len = MultiByteToWideChar(CP_UTF8, 0, home, home_len, home_path, (DWORD)maxlen);
    if (!len) {
        winerror(GetLastError(), L"failed to decode home setting in '%ls'", pyvenv_cfg);
        return RC_BAD_VENV_CFG;
    }
    home_path[len] = L'\0';

    return 0;
}


int
locate_python(wchar_t *path, size_t maxlen)
{
    if (!join(path, maxlen, EXENAME)) {
        error(L"failed to append %ls to '%ls'", EXENAME, path);
        return RC_NO_MEMORY;
    }

    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) {
        winerror(GetLastError(), L"did not find executable at '%ls'", path);
        return RC_NO_PYTHON;
    }

    return 0;
}


int
smuggle_path()
{
    wchar_t buffer[MAXLEN];
    // We could use argv[0], but that may be wrong in certain rare cases (if the
    // user is doing something weird like symlinks to venv redirectors), and
    // what we _really_ want is the directory of the venv. We always copy the
    // redirectors, so if we've made the venv, this will be correct.
    DWORD len = GetModuleFileNameW(NULL, buffer, MAXLEN);
    if (!len) {
        winerror(GetLastError(), L"Failed to get own executable path");
        return RC_INTERNAL_ERROR;
    }
    buffer[len] = L'\0';
    debug(L"Setting __PYVENV_LAUNCHER__ = '%s'", buffer);

    if (!SetEnvironmentVariableW(L"__PYVENV_LAUNCHER__", buffer)) {
        winerror(GetLastError(), L"Failed to set launcher environment");
        return RC_INTERNAL_ERROR;
    }

    return 0;
}

/*
 * Process creation
 */

static BOOL
safe_duplicate_handle(HANDLE in, HANDLE * pout, const wchar_t *name)
{
    BOOL ok;
    HANDLE process = GetCurrentProcess();
    DWORD rc;

    *pout = NULL;
    ok = DuplicateHandle(process, in, process, pout, 0, TRUE,
                         DUPLICATE_SAME_ACCESS);
    if (!ok) {
        rc = GetLastError();
        if (rc == ERROR_INVALID_HANDLE) {
            debug(L"DuplicateHandle(%ls) returned ERROR_INVALID_HANDLE\n", name);
            ok = TRUE;
        }
        else {
            debug(L"DuplicateHandle(%ls) returned %d\n", name, rc);
        }
    }
    return ok;
}

static BOOL WINAPI
ctrl_c_handler(DWORD code)
{
    return TRUE;    /* We just ignore all control events. */
}

static int
launch(const wchar_t *executable, wchar_t *cmdline)
{
    HANDLE job;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
    DWORD rc;
    BOOL ok;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

#if defined(_WINDOWS)
    /*
    When explorer launches a Windows (GUI) application, it displays
    the "app starting" (the "pointer + hourglass") cursor for a number
    of seconds, or until the app does something UI-ish (eg, creating a
    window, or fetching a message).  As this launcher doesn't do this
    directly, that cursor remains even after the child process does these
    things.  We avoid that by doing a simple post+get message.
    See http://bugs.python.org/issue17290
    */
    MSG msg;

    PostMessage(0, 0, 0, 0);
    GetMessage(&msg, 0, 0, 0);
#endif

    debug(L"run_child: about to run '%ls' with '%ls'\n", executable, cmdline);
    job = CreateJobObject(NULL, NULL);
    ok = QueryInformationJobObject(job, JobObjectExtendedLimitInformation,
                                   &info, sizeof(info), &rc);
    if (!ok || (rc != sizeof(info)) || !job) {
        winerror(GetLastError(), L"Job information querying failed");
        return RC_CREATE_PROCESS;
    }
    info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
                                             JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
    ok = SetInformationJobObject(job, JobObjectExtendedLimitInformation, &info,
                                 sizeof(info));
    if (!ok) {
        winerror(GetLastError(), L"Job information setting failed");
        return RC_CREATE_PROCESS;
    }
    memset(&si, 0, sizeof(si));
    GetStartupInfoW(&si);
    ok = safe_duplicate_handle(GetStdHandle(STD_INPUT_HANDLE), &si.hStdInput, L"stdin");
    if (!ok) {
        return RC_NO_STD_HANDLES;
    }
    ok = safe_duplicate_handle(GetStdHandle(STD_OUTPUT_HANDLE), &si.hStdOutput, L"stdout");
    if (!ok) {
        return RC_NO_STD_HANDLES;
    }
    ok = safe_duplicate_handle(GetStdHandle(STD_ERROR_HANDLE), &si.hStdError, L"stderr");
    if (!ok) {
        return RC_NO_STD_HANDLES;
    }

    ok = SetConsoleCtrlHandler(ctrl_c_handler, TRUE);
    if (!ok) {
        winerror(GetLastError(), L"control handler setting failed");
        return RC_CREATE_PROCESS;
    }

    si.dwFlags = STARTF_USESTDHANDLES;
    ok = CreateProcessW(executable, cmdline, NULL, NULL, TRUE,
                        0, NULL, NULL, &si, &pi);
    if (!ok) {
        winerror(GetLastError(), L"Unable to create process using '%ls'", cmdline);
        return RC_CREATE_PROCESS;
    }
    AssignProcessToJobObject(job, pi.hProcess);
    CloseHandle(pi.hThread);
    WaitForSingleObjectEx(pi.hProcess, INFINITE, FALSE);
    ok = GetExitCodeProcess(pi.hProcess, &rc);
    if (!ok) {
        winerror(GetLastError(), L"Failed to get exit code of process");
        return RC_CREATE_PROCESS;
    }
    debug(L"child process exit code: %d", rc);
    return rc;
}


int
process(int argc, wchar_t ** argv)
{
    int exitCode;
    wchar_t pyvenvcfg_path[MAXLEN];
    wchar_t home_path[MAXLEN];

    if (isEnvVarSet(L"PYLAUNCHER_DEBUG")) {
        setvbuf(stderr, (char *)NULL, _IONBF, 0);
        log_fp = stderr;
    }

    exitCode = calculate_pyvenvcfg_path(pyvenvcfg_path, MAXLEN);
    if (exitCode) return exitCode;

    exitCode = read_home(pyvenvcfg_path, home_path, MAXLEN);
    if (exitCode) return exitCode;

    exitCode = locate_python(home_path, MAXLEN);
    if (exitCode) return exitCode;

    // We do not update argv[0] to point at the target runtime, and so we do not
    // pass through our original argv[0] in an environment variable.
    exitCode = smuggle_path();
    if (exitCode) return exitCode;

    exitCode = launch(home_path, GetCommandLineW());
    return exitCode;
}


#if defined(_WINDOWS)

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPWSTR lpstrCmd, int nShow)
{
    return process(__argc, __wargv);
}

#else

int cdecl wmain(int argc, wchar_t ** argv)
{
    return process(argc, argv);
}

#endif
