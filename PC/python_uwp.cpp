/* Main program when embedded in a UWP application on Windows */

#include "Python.h"
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <string>

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

static std::wstring
get_user_base()
{
    try {
        const auto appData = winrt::Windows::Storage::ApplicationData::Current();
        if (appData) {
            const auto localCache = appData.LocalCacheFolder();
            if (localCache) {
                auto path = localCache.Path();
                if (!path.empty()) {
                    return std::wstring(path) + L"\\local-packages";
                }
            }
        }
    } catch (...) {
    }
    return std::wstring();
}

static std::wstring
get_package_family()
{
    try {
        const auto package = winrt::Windows::ApplicationModel::Package::Current();
        if (package) {
            const auto id = package.Id();
            if (id) {
                return std::wstring(id.FamilyName());
            }
        }
    }
    catch (...) {
    }

    return std::wstring();
}

static std::wstring
get_package_home()
{
    try {
        const auto package = winrt::Windows::ApplicationModel::Package::Current();
        if (package) {
            const auto path = package.InstalledLocation();
            if (path) {
                return std::wstring(path.Path());
            }
        }
    }
    catch (...) {
    }

    return std::wstring();
}

static int
set_process_name()
{
    const auto home = get_package_home();
    const auto family = get_package_family();

    std::wstring executable;

    /* If inside a package, use user's symlink name for executable */
    if (!family.empty()) {
        PWSTR localAppData;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0,
                                           NULL, &localAppData))) {
            executable = std::wstring(localAppData)
                         + L"\\Microsoft\\WindowsApps\\"
                         + family
                         + L"\\"
                         + PROGNAME;

            CoTaskMemFree(localAppData);
        }
    }

    /* Only use module filename if we don't have a home */
    if (home.empty() && executable.empty()) {
        executable.resize(MAX_PATH);
        while (true) {
            DWORD len = GetModuleFileNameW(
                NULL, executable.data(), (DWORD)executable.size());
            if (len == 0) {
                executable.clear();
                break;
            } else if (len == executable.size() &&
                       GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                executable.resize(len * 2);
            } else {
                executable.resize(len);
                break;
            }
        }
    }

    if (!home.empty()) {
        Py_SetPythonHome(home.c_str());
    }

    const wchar_t *launcherPath = _wgetenv(L"__PYVENV_LAUNCHER__");
    if (launcherPath) {
        if (!executable.empty()) {
            _wputenv_s(L"__PYVENV_BASE_EXECUTABLE__", executable.c_str());
        }
        _Py_SetProgramFullPath(launcherPath);
        /* bpo-35873: Clear the environment variable to avoid it being
         * inherited by child processes. */
        _wputenv_s(L"__PYVENV_LAUNCHER__", L"");
    } else if (!executable.empty()) {
        _Py_SetProgramFullPath(executable.c_str());
    }

    return 1;
}

int
wmain(int argc, wchar_t **argv)
{
    if (!set_process_name()) {
        return 121;
    }
    const wchar_t *p = _wgetenv(L"PYTHONUSERBASE");
    if (!p || !*p) {
        _wputenv_s(L"PYTHONUSERBASE", get_user_base().c_str());
    }

    p = wcsrchr(argv[0], L'\\');
    if (!p) {
        p = argv[0];
    }
    if (p) {
        if (*p == L'\\') {
            p++;
        }

        const wchar_t *moduleName = NULL;
        if (wcsnicmp(p, L"pip", 3) == 0) {
            moduleName = L"pip";
            /* No longer required when pip 19.1 is added */
            _wputenv_s(L"PIP_USER", L"true");
        } else if (wcsnicmp(p, L"idle", 4) == 0) {
            moduleName = L"idlelib";
        }

        if (moduleName) {
            /* Not even pretending we're going to free this memory.
             * The OS will clean it all up when our process exits
             */
            wchar_t **new_argv = (wchar_t **)PyMem_RawMalloc((argc + 2) * sizeof(wchar_t *));
            new_argv[0] = argv[0];
            new_argv[1] = _PyMem_RawWcsdup(L"-m");
            new_argv[2] = _PyMem_RawWcsdup(moduleName);
            for (int i = 1; i < argc; ++i) {
                new_argv[i + 2] = argv[i];
            }
            argv = new_argv;
            argc += 2;
        }
    }

    return Py_Main(argc, (wchar_t**)argv);
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
