/* Main program when embedded in a UWP application on Windows */

#include "Python.h"
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#include <winrt\Windows.ApplicationModel.h>
#include <winrt\Windows.Storage.h>

#ifdef PYTHONW
#ifdef _DEBUG
const wchar_t *PROGNAME = L"pythonw_d.exe";
#else
const wchar_t *PROGNAME = L"pythonw.exe";
#endif
#else
#ifdef _DEBUG
const wchar_t *PROGNAME = L"python_d.exe";
#else
const wchar_t *PROGNAME = L"python.exe";
#endif
#endif

static void
set_user_base()
{
    wchar_t envBuffer[2048];
    try {
        const auto appData = winrt::Windows::Storage::ApplicationData::Current();
        if (appData) {
            const auto localCache = appData.LocalCacheFolder();
            if (localCache) {
                auto path = localCache.Path();
                if (!path.empty() &&
                    !wcscpy_s(envBuffer, path.c_str()) &&
                    !wcscat_s(envBuffer, L"\\local-packages")
                ) {
                    _wputenv_s(L"PYTHONUSERBASE", envBuffer);
                }
            }
        }
    } catch (...) {
    }
}

static const wchar_t *
get_argv0(const wchar_t *argv0)
{
    winrt::hstring installPath;
    const wchar_t *launcherPath;
    wchar_t *buffer;
    size_t len;

    launcherPath = _wgetenv(L"__PYVENV_LAUNCHER__");
    if (launcherPath && launcherPath[0]) {
        len = wcslen(launcherPath) + 1;
        buffer = (wchar_t *)malloc(sizeof(wchar_t) * len);
        if (!buffer) {
            Py_FatalError("out of memory");
            return NULL;
        }
        if (wcscpy_s(buffer, len, launcherPath)) {
            Py_FatalError("failed to copy to buffer");
            return NULL;
        }
        return buffer;
    }

    try {
        const auto package = winrt::Windows::ApplicationModel::Package::Current();
        if (package) {
            const auto install = package.InstalledLocation();
            if (install) {
                installPath = install.Path();
            }
        }
    }
    catch (...) {
    }

    if (!installPath.empty()) {
        len = installPath.size() + wcslen(PROGNAME) + 2;
    } else {
        len = wcslen(argv0) + wcslen(PROGNAME) + 1;
    }

    buffer = (wchar_t *)malloc(sizeof(wchar_t) * len);
    if (!buffer) {
        Py_FatalError("out of memory");
        return NULL;
    }

    if (!installPath.empty()) {
        if (wcscpy_s(buffer, len, installPath.c_str())) {
            Py_FatalError("failed to copy to buffer");
            return NULL;
        }
        if (wcscat_s(buffer, len, L"\\")) {
            Py_FatalError("failed to concatenate backslash");
            return NULL;
        }
    } else {
        if (wcscpy_s(buffer, len, argv0)) {
            Py_FatalError("failed to copy argv[0]");
            return NULL;
        }

        wchar_t *name = wcsrchr(buffer, L'\\');
        if (name) {
            name[1] = L'\0';
        } else {
            buffer[0] = L'\0';
        }
    }

    if (wcscat_s(buffer, len, PROGNAME)) {
        Py_FatalError("failed to concatenate program name");
        return NULL;
    }

    return buffer;
}

static wchar_t *
get_process_name()
{
    DWORD bufferLen = MAX_PATH;
    DWORD len = bufferLen;
    wchar_t *r = NULL;

    while (!r) {
        r = (wchar_t *)malloc(bufferLen * sizeof(wchar_t));
        if (!r) {
            Py_FatalError("out of memory");
            return NULL;
        }
        len = GetModuleFileNameW(NULL, r, bufferLen);
        if (len == 0) {
            free((void *)r);
            return NULL;
        } else if (len == bufferLen &&
                   GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            free(r);
            r = NULL;
            bufferLen *= 2;
        }
    }

    return r;
}

int
wmain(int argc, wchar_t **argv)
{
    const wchar_t **new_argv;
    int new_argc;
    const wchar_t *exeName;

    new_argc = argc;
    new_argv = (const wchar_t**)malloc(sizeof(wchar_t *) * (argc + 2));
    if (new_argv == NULL) {
        Py_FatalError("out of memory");
        return -1;
    }

    exeName = get_process_name();

    new_argv[0] = get_argv0(exeName ? exeName : argv[0]);
    for (int i = 1; i < argc; ++i) {
        new_argv[i] = argv[i];
    }

    set_user_base();

    if (exeName) {
        const wchar_t *p = wcsrchr(exeName, L'\\');
        if (p) {
            const wchar_t *moduleName = NULL;
            if (*p++ == L'\\') {
                if (wcsnicmp(p, L"pip", 3) == 0) {
                    moduleName = L"pip";
                    _wputenv_s(L"PIP_USER", L"true");
                }
                else if (wcsnicmp(p, L"idle", 4) == 0) {
                    moduleName = L"idlelib";
                }
            }

            if (moduleName) {
                new_argc += 2;
                for (int i = argc; i >= 1; --i) {
                    new_argv[i + 2] = new_argv[i];
                }
                new_argv[1] = L"-m";
                new_argv[2] = moduleName;
            }
        }
    }

    /* Override program_full_path from here so that
       sys.executable is set correctly. */
    _Py_SetProgramFullPath(new_argv[0]);

    int result = Py_Main(new_argc, (wchar_t **)new_argv);

    free((void *)exeName);
    free((void *)new_argv);

    return result;
}

#ifdef PYTHONW

int WINAPI wWinMain(
    HINSTANCE hInstance,      /* handle to current instance */
    HINSTANCE hPrevInstance,  /* handle to previous instance */
    LPWSTR lpCmdLine,         /* pointer to command line */
    int nCmdShow              /* show state of window */
)
{
    return wmain(__argc, __wargv);
}

#endif
