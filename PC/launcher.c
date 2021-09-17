/*
 * Copyright (C) 2011-2013 Vinay Sajip.
 * Licensed to PSF under a contributor agreement.
 *
 * Based on the work of:
 *
 * Mark Hammond (original author of Python version)
 * Curt Hagenlocher (job management)
 */

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <tchar.h>

#define BUFSIZE 256
#define MSGSIZE 1024

/* Build options. */
#define SKIP_PREFIX
#define SEARCH_PATH

/* Error codes */

#define RC_NO_STD_HANDLES   100
#define RC_CREATE_PROCESS   101
#define RC_BAD_VIRTUAL_PATH 102
#define RC_NO_PYTHON        103
#define RC_NO_MEMORY        104
/*
 * SCRIPT_WRAPPER is used to choose one of the variants of an executable built
 * from this source file. If not defined, the PEP 397 Python launcher is built;
 * if defined, a script launcher of the type used by setuptools is built, which
 * looks for a script name related to the executable name and runs that script
 * with the appropriate Python interpreter.
 *
 * SCRIPT_WRAPPER should be undefined in the source, and defined in a VS project
 * which builds the setuptools-style launcher.
 */
#if defined(SCRIPT_WRAPPER)
#define RC_NO_SCRIPT        105
#endif
/*
 * VENV_REDIRECT is used to choose the variant that looks for an adjacent or
 * one-level-higher pyvenv.cfg, and uses its "home" property to locate and
 * launch the original python.exe.
 */
#if defined(VENV_REDIRECT)
#define RC_NO_VENV_CFG      106
#define RC_BAD_VENV_CFG     107
#endif

/* Just for now - static definition */

static FILE * log_fp = NULL;

static wchar_t *
skip_whitespace(wchar_t * p)
{
    while (*p && isspace(*p))
        ++p;
    return p;
}

static void
debug(wchar_t * format, ...)
{
    va_list va;

    if (log_fp != NULL) {
        va_start(va, format);
        vfwprintf_s(log_fp, format, va);
        va_end(va);
    }
}

static void
winerror(int rc, wchar_t * message, int size)
{
    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, rc, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        message, size, NULL);
}

static void
error(int rc, wchar_t * format, ... )
{
    va_list va;
    wchar_t message[MSGSIZE];
    wchar_t win_message[MSGSIZE];
    int len;

    va_start(va, format);
    len = _vsnwprintf_s(message, MSGSIZE, _TRUNCATE, format, va);
    va_end(va);

    if (rc == 0) {  /* a Windows error */
        winerror(GetLastError(), win_message, MSGSIZE);
        if (len >= 0) {
            _snwprintf_s(&message[len], MSGSIZE - len, _TRUNCATE, L": %ls",
                         win_message);
        }
    }

#if !defined(_WINDOWS)
    fwprintf(stderr, L"%ls\n", message);
#else
    MessageBoxW(NULL, message, L"Python Launcher is sorry to say ...",
               MB_OK);
#endif
    exit(rc);
}

/*
 * This function is here to simplify memory management
 * and to treat blank values as if they are absent.
 */
static wchar_t * get_env(wchar_t * key)
{
    /* This is not thread-safe, just like getenv */
    static wchar_t buf[BUFSIZE];
    DWORD result = GetEnvironmentVariableW(key, buf, BUFSIZE);

    if (result >= BUFSIZE) {
        /* Large environment variable. Accept some leakage */
        wchar_t *buf2 = (wchar_t*)malloc(sizeof(wchar_t) * (result+1));
        if (buf2 == NULL) {
            error(RC_NO_MEMORY, L"Could not allocate environment buffer");
        }
        GetEnvironmentVariableW(key, buf2, result);
        return buf2;
    }

    if (result == 0)
        /* Either some error, e.g. ERROR_ENVVAR_NOT_FOUND,
           or an empty environment variable. */
        return NULL;

    return buf;
}

#if defined(_DEBUG)
/* Do not define EXECUTABLEPATH_VALUE in debug builds as it'll
   never point to the debug build. */
#if defined(_WINDOWS)

#define PYTHON_EXECUTABLE L"pythonw_d.exe"

#else

#define PYTHON_EXECUTABLE L"python_d.exe"

#endif
#else
#if defined(_WINDOWS)

#define PYTHON_EXECUTABLE L"pythonw.exe"
#define EXECUTABLEPATH_VALUE L"WindowedExecutablePath"

#else

#define PYTHON_EXECUTABLE L"python.exe"
#define EXECUTABLEPATH_VALUE L"ExecutablePath"

#endif
#endif

#define MAX_VERSION_SIZE    8

typedef struct {
    wchar_t version[MAX_VERSION_SIZE]; /* m.n */
    int bits;   /* 32 or 64 */
    wchar_t executable[MAX_PATH];
    wchar_t exe_display[MAX_PATH];
} INSTALLED_PYTHON;

/*
 * To avoid messing about with heap allocations, just assume we can allocate
 * statically and never have to deal with more versions than this.
 */
#define MAX_INSTALLED_PYTHONS   100

static INSTALLED_PYTHON installed_pythons[MAX_INSTALLED_PYTHONS];

static size_t num_installed_pythons = 0;

/*
 * To hold SOFTWARE\Python\PythonCore\X.Y...\InstallPath
 * The version name can be longer than MAX_VERSION_SIZE, but will be
 * truncated to just X.Y for comparisons.
 */
#define IP_BASE_SIZE 80
#define IP_VERSION_SIZE 8
#define IP_SIZE (IP_BASE_SIZE + IP_VERSION_SIZE)
#define CORE_PATH L"SOFTWARE\\Python\\PythonCore"
/*
 * Installations from the Microsoft Store will set the same registry keys,
 * but because of a limitation in Windows they cannot be enumerated normally
 * (unless you have no other Python installations... which is probably false
 * because that's the most likely way to get this launcher!)
 * This key is under HKEY_LOCAL_MACHINE
 */
#define LOOKASIDE_PATH L"SOFTWARE\\Microsoft\\AppModel\\Lookaside\\user\\Software\\Python\\PythonCore"

static wchar_t * location_checks[] = {
    L"\\",
    L"\\PCbuild\\win32\\",
    L"\\PCbuild\\amd64\\",
    /* To support early 32bit versions of Python that stuck the build binaries
    * directly in PCbuild... */
    L"\\PCbuild\\",
    NULL
};

static INSTALLED_PYTHON *
find_existing_python(const wchar_t * path)
{
    INSTALLED_PYTHON * result = NULL;
    size_t i;
    INSTALLED_PYTHON * ip;

    for (i = 0, ip = installed_pythons; i < num_installed_pythons; i++, ip++) {
        if (_wcsicmp(path, ip->executable) == 0) {
            result = ip;
            break;
        }
    }
    return result;
}

static INSTALLED_PYTHON *
find_existing_python2(int bits, const wchar_t * version)
{
    INSTALLED_PYTHON * result = NULL;
    size_t i;
    INSTALLED_PYTHON * ip;

    for (i = 0, ip = installed_pythons; i < num_installed_pythons; i++, ip++) {
        if (bits == ip->bits && _wcsicmp(version, ip->version) == 0) {
            result = ip;
            break;
        }
    }
    return result;
}

static void
_locate_pythons_for_key(HKEY root, LPCWSTR subkey, REGSAM flags, int bits,
                        int display_name_only)
{
    HKEY core_root, ip_key;
    LSTATUS status = RegOpenKeyExW(root, subkey, 0, flags, &core_root);
    wchar_t message[MSGSIZE];
    DWORD i;
    size_t n;
    BOOL ok, append_name;
    DWORD type, data_size, attrs;
    INSTALLED_PYTHON * ip, * pip;
    wchar_t ip_version[IP_VERSION_SIZE];
    wchar_t ip_path[IP_SIZE];
    wchar_t * check;
    wchar_t ** checkp;
    wchar_t *key_name = (root == HKEY_LOCAL_MACHINE) ? L"HKLM" : L"HKCU";

    if (status != ERROR_SUCCESS)
        debug(L"locate_pythons_for_key: unable to open PythonCore key in %ls\n",
              key_name);
    else {
        ip = &installed_pythons[num_installed_pythons];
        for (i = 0; num_installed_pythons < MAX_INSTALLED_PYTHONS; i++) {
            status = RegEnumKeyW(core_root, i, ip_version, IP_VERSION_SIZE);
            if (status != ERROR_SUCCESS) {
                if (status != ERROR_NO_MORE_ITEMS) {
                    /* unexpected error */
                    winerror(status, message, MSGSIZE);
                    debug(L"Can't enumerate registry key for version %ls: %ls\n",
                          ip_version, message);
                }
                break;
            }
            else {
                wcsncpy_s(ip->version, MAX_VERSION_SIZE, ip_version,
                          MAX_VERSION_SIZE-1);
                /* Still treating version as "x.y" rather than sys.winver
                 * When PEP 514 tags are properly used, we shouldn't need
                 * to strip this off here.
                 */
                check = wcsrchr(ip->version, L'-');
                if (check && !wcscmp(check, L"-32")) {
                    *check = L'\0';
                }
                _snwprintf_s(ip_path, IP_SIZE, _TRUNCATE,
                             L"%ls\\%ls\\InstallPath", subkey, ip_version);
                status = RegOpenKeyExW(root, ip_path, 0, flags, &ip_key);
                if (status != ERROR_SUCCESS) {
                    winerror(status, message, MSGSIZE);
                    /* Note: 'message' already has a trailing \n*/
                    debug(L"%ls\\%ls: %ls", key_name, ip_path, message);
                    continue;
                }
                data_size = sizeof(ip->executable) - 1;
                append_name = FALSE;
#ifdef EXECUTABLEPATH_VALUE
                status = RegQueryValueExW(ip_key, EXECUTABLEPATH_VALUE, NULL, &type,
                                          (LPBYTE)ip->executable, &data_size);
#else
                status = ERROR_FILE_NOT_FOUND; /* actual error doesn't matter */
#endif
                if (status != ERROR_SUCCESS || type != REG_SZ || !data_size) {
                    append_name = TRUE;
                    data_size = sizeof(ip->executable) - 1;
                    status = RegQueryValueExW(ip_key, NULL, NULL, &type,
                                              (LPBYTE)ip->executable, &data_size);
                    if (status != ERROR_SUCCESS) {
                        winerror(status, message, MSGSIZE);
                        debug(L"%ls\\%ls: %ls\n", key_name, ip_path, message);
                        RegCloseKey(ip_key);
                        continue;
                    }
                }
                RegCloseKey(ip_key);
                if (type != REG_SZ) {
                    continue;
                }

                data_size = data_size / sizeof(wchar_t) - 1;  /* for NUL */
                if (ip->executable[data_size - 1] == L'\\')
                    --data_size; /* reg value ended in a backslash */
                /* ip->executable is data_size long */
                for (checkp = location_checks; *checkp; ++checkp) {
                    check = *checkp;
                    if (append_name) {
                        _snwprintf_s(&ip->executable[data_size],
                                     MAX_PATH - data_size,
                                     MAX_PATH - data_size,
                                     L"%ls%ls", check, PYTHON_EXECUTABLE);
                    }
                    attrs = GetFileAttributesW(ip->executable);
                    if (attrs == INVALID_FILE_ATTRIBUTES) {
                        winerror(GetLastError(), message, MSGSIZE);
                        debug(L"locate_pythons_for_key: %ls: %ls",
                              ip->executable, message);
                    }
                    else if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
                        debug(L"locate_pythons_for_key: '%ls' is a directory\n",
                              ip->executable, attrs);
                    }
                    else if (find_existing_python(ip->executable)) {
                        debug(L"locate_pythons_for_key: %ls: already found\n",
                              ip->executable);
                    }
                    else {
                        /* check the executable type. */
                        if (bits) {
                            ip->bits = bits;
                        } else {
                            ok = GetBinaryTypeW(ip->executable, &attrs);
                            if (!ok) {
                                debug(L"Failure getting binary type: %ls\n",
                                      ip->executable);
                            }
                            else {
                                if (attrs == SCS_64BIT_BINARY)
                                    ip->bits = 64;
                                else if (attrs == SCS_32BIT_BINARY)
                                    ip->bits = 32;
                                else
                                    ip->bits = 0;
                            }
                        }
                        if (ip->bits == 0) {
                            debug(L"locate_pythons_for_key: %ls: \
invalid binary type: %X\n",
                                  ip->executable, attrs);
                        }
                        else {
                            if (display_name_only) {
                                /* display just the executable name. This is
                                 * primarily for the Store installs */
                                const wchar_t *name = wcsrchr(ip->executable, L'\\');
                                if (name) {
                                    wcscpy_s(ip->exe_display, MAX_PATH, name+1);
                                }
                            }
                            if (wcschr(ip->executable, L' ') != NULL) {
                                /* has spaces, so quote, and set original as
                                 * the display name */
                                if (!ip->exe_display[0]) {
                                    wcscpy_s(ip->exe_display, MAX_PATH, ip->executable);
                                }
                                n = wcslen(ip->executable);
                                memmove(&ip->executable[1],
                                        ip->executable, n * sizeof(wchar_t));
                                ip->executable[0] = L'\"';
                                ip->executable[n + 1] = L'\"';
                                ip->executable[n + 2] = L'\0';
                            }
                            debug(L"locate_pythons_for_key: %ls \
is a %dbit executable\n",
                                ip->executable, ip->bits);
                            if (find_existing_python2(ip->bits, ip->version)) {
                                debug(L"locate_pythons_for_key: %ls-%i: already \
found\n", ip->version, ip->bits);
                            }
                            else {
                                ++num_installed_pythons;
                                pip = ip++;
                                if (num_installed_pythons >=
                                    MAX_INSTALLED_PYTHONS)
                                    break;
                            }
                        }
                    }
                }
            }
        }
        RegCloseKey(core_root);
    }
}

static int
compare_pythons(const void * p1, const void * p2)
{
    INSTALLED_PYTHON * ip1 = (INSTALLED_PYTHON *) p1;
    INSTALLED_PYTHON * ip2 = (INSTALLED_PYTHON *) p2;
    /* note reverse sorting on version */
    int result = CompareStringW(LOCALE_INVARIANT, SORT_DIGITSASNUMBERS,
                                ip2->version, -1, ip1->version, -1);
    switch (result) {
    case 0:
        error(0, L"CompareStringW failed");
        return 0;
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_EQUAL:
        return ip2->bits - ip1->bits; /* 64 before 32 */
    case CSTR_GREATER_THAN:
        return 1;
    default:
        return 0; // This should never be reached.
    }
}

static void
locate_pythons_for_key(HKEY root, REGSAM flags)
{
    _locate_pythons_for_key(root, CORE_PATH, flags, 0, FALSE);
}

static void
locate_store_pythons()
{
#if defined(_M_X64)
    /* 64bit process, so look in native registry */
    _locate_pythons_for_key(HKEY_LOCAL_MACHINE, LOOKASIDE_PATH,
                            KEY_READ, 64, TRUE);
#else
    /* 32bit process, so check that we're on 64bit OS */
    BOOL f64 = FALSE;
    if (IsWow64Process(GetCurrentProcess(), &f64) && f64) {
        _locate_pythons_for_key(HKEY_LOCAL_MACHINE, LOOKASIDE_PATH,
                                KEY_READ | KEY_WOW64_64KEY, 64, TRUE);
    }
#endif
}

static void
locate_venv_python()
{
    static wchar_t venv_python[MAX_PATH];
    INSTALLED_PYTHON * ip;
    wchar_t *virtual_env = get_env(L"VIRTUAL_ENV");
    DWORD attrs;

    /* Check for VIRTUAL_ENV environment variable */
    if (virtual_env == NULL || virtual_env[0] == L'\0') {
        return;
    }

    /* Check for a python executable in the venv */
    debug(L"Checking for Python executable in virtual env '%ls'\n", virtual_env);
    _snwprintf_s(venv_python, MAX_PATH, _TRUNCATE,
            L"%ls\\Scripts\\%ls", virtual_env, PYTHON_EXECUTABLE);
    attrs = GetFileAttributesW(venv_python);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        debug(L"Python executable %ls missing from virtual env\n", venv_python);
        return;
    }

    ip = &installed_pythons[num_installed_pythons++];
    wcscpy_s(ip->executable, MAX_PATH, venv_python);
    ip->bits = 0;
    wcscpy_s(ip->version, MAX_VERSION_SIZE, L"venv");
}

static void
locate_all_pythons()
{
    /* venv Python is highest priority */
    locate_venv_python();
#if defined(_M_X64)
    /* If we are a 64bit process, first hit the 32bit keys. */
    debug(L"locating Pythons in 32bit registry\n");
    locate_pythons_for_key(HKEY_CURRENT_USER, KEY_READ | KEY_WOW64_32KEY);
    locate_pythons_for_key(HKEY_LOCAL_MACHINE, KEY_READ | KEY_WOW64_32KEY);
#else
    /* If we are a 32bit process on a 64bit Windows, first hit the 64bit keys.*/
    BOOL f64 = FALSE;
    if (IsWow64Process(GetCurrentProcess(), &f64) && f64) {
        debug(L"locating Pythons in 64bit registry\n");
        locate_pythons_for_key(HKEY_CURRENT_USER, KEY_READ | KEY_WOW64_64KEY);
        locate_pythons_for_key(HKEY_LOCAL_MACHINE, KEY_READ | KEY_WOW64_64KEY);
    }
#endif
    /* now hit the "native" key for this process bittedness. */
    debug(L"locating Pythons in native registry\n");
    locate_pythons_for_key(HKEY_CURRENT_USER, KEY_READ);
    locate_pythons_for_key(HKEY_LOCAL_MACHINE, KEY_READ);
    /* Store-installed Python is lowest priority */
    locate_store_pythons();
    qsort(installed_pythons, num_installed_pythons, sizeof(INSTALLED_PYTHON),
          compare_pythons);
}

static INSTALLED_PYTHON *
find_python_by_version(wchar_t const * wanted_ver)
{
    INSTALLED_PYTHON * result = NULL;
    INSTALLED_PYTHON * ip = installed_pythons;
    size_t i, n;
    size_t wlen = wcslen(wanted_ver);
    int bits = 0;

    if (wcsstr(wanted_ver, L"-32")) {
        bits = 32;
        wlen -= wcslen(L"-32");
    }
    else if (wcsstr(wanted_ver, L"-64")) { /* Added option to select 64 bit explicitly */
        bits = 64;
        wlen -= wcslen(L"-64");
    }
    for (i = 0; i < num_installed_pythons; i++, ip++) {
        n = wcslen(ip->version);
        if (n > wlen)
            n = wlen;
        if ((wcsncmp(ip->version, wanted_ver, n) == 0) &&
            /* bits == 0 => don't care */
            ((bits == 0) || (ip->bits == bits))) {
            result = ip;
            break;
        }
    }
    return result;
}


static wchar_t appdata_ini_path[MAX_PATH];
static wchar_t launcher_ini_path[MAX_PATH];

/*
 * Get a value either from the environment or a configuration file.
 * The key passed in will either be "python", "python2" or "python3".
 */
static wchar_t *
get_configured_value(wchar_t * key)
{
/*
 * Note: this static value is used to return a configured value
 * obtained either from the environment or configuration file.
 * This should be OK since there wouldn't be any concurrent calls.
 */
    static wchar_t configured_value[MSGSIZE];
    wchar_t * result = NULL;
    wchar_t * found_in = L"environment";
    DWORD size;

    /* First, search the environment. */
    _snwprintf_s(configured_value, MSGSIZE, _TRUNCATE, L"py_%ls", key);
    result = get_env(configured_value);
    if (result == NULL && appdata_ini_path[0]) {
        /* Not in environment: check local configuration. */
        size = GetPrivateProfileStringW(L"defaults", key, NULL,
                                        configured_value, MSGSIZE,
                                        appdata_ini_path);
        if (size > 0) {
            result = configured_value;
            found_in = appdata_ini_path;
        }
    }
    if (result == NULL && launcher_ini_path[0]) {
        /* Not in environment or local: check global configuration. */
        size = GetPrivateProfileStringW(L"defaults", key, NULL,
                                        configured_value, MSGSIZE,
                                        launcher_ini_path);
        if (size > 0) {
            result = configured_value;
            found_in = launcher_ini_path;
        }
    }
    if (result) {
        debug(L"found configured value '%ls=%ls' in %ls\n",
              key, result, found_in ? found_in : L"(unknown)");
    } else {
        debug(L"found no configured value for '%ls'\n", key);
    }
    return result;
}

static INSTALLED_PYTHON *
locate_python(wchar_t * wanted_ver, BOOL from_shebang)
{
    static wchar_t config_key [] = { L"pythonX" };
    static wchar_t * last_char = &config_key[sizeof(config_key) /
                                             sizeof(wchar_t) - 2];
    INSTALLED_PYTHON * result = NULL;
    size_t n = wcslen(wanted_ver);
    wchar_t * configured_value;

    if (num_installed_pythons == 0)
        locate_all_pythons();

    if (n == 1) {   /* just major version specified */
        *last_char = *wanted_ver;
        configured_value = get_configured_value(config_key);
        if (configured_value != NULL)
            wanted_ver = configured_value;
    }
    if (*wanted_ver) {
        result = find_python_by_version(wanted_ver);
        debug(L"search for Python version '%ls' found ", wanted_ver);
        if (result) {
            debug(L"'%ls'\n", result->executable);
        } else {
            debug(L"no interpreter\n");
        }
    }
    else {
        *last_char = L'\0'; /* look for an overall default */
        result = find_python_by_version(L"venv");
        if (result == NULL) {
            configured_value = get_configured_value(config_key);
            if (configured_value)
                result = find_python_by_version(configured_value);
        }
        /* Not found a value yet - try by major version.
         * If we're looking for an interpreter specified in a shebang line,
         * we want to try Python 2 first, then Python 3 (for Unix and backward
         * compatibility). If we're being called interactively, assume the user
         * wants the latest version available, so try Python 3 first, then
         * Python 2.
         */
        if (result == NULL)
            result = find_python_by_version(from_shebang ? L"2" : L"3");
        if (result == NULL)
            result = find_python_by_version(from_shebang ? L"3" : L"2");
        debug(L"search for default Python found ");
        if (result) {
            debug(L"version %ls at '%ls'\n",
                  result->version, result->executable);
        } else {
            debug(L"no interpreter\n");
        }
    }
    return result;
}

#if defined(SCRIPT_WRAPPER)
/*
 * Check for a script located alongside the executable
 */

#if defined(_WINDOWS)
#define SCRIPT_SUFFIX L"-script.pyw"
#else
#define SCRIPT_SUFFIX L"-script.py"
#endif

static wchar_t wrapped_script_path[MAX_PATH];

/* Locate the script being wrapped.
 *
 * This code should store the name of the wrapped script in
 * wrapped_script_path, or terminate the program with an error if there is no
 * valid wrapped script file.
 */
static void
locate_wrapped_script()
{
    wchar_t * p;
    size_t plen;
    DWORD attrs;

    plen = GetModuleFileNameW(NULL, wrapped_script_path, MAX_PATH);
    p = wcsrchr(wrapped_script_path, L'.');
    if (p == NULL) {
        debug(L"GetModuleFileNameW returned value has no extension: %ls\n",
              wrapped_script_path);
        error(RC_NO_SCRIPT, L"Wrapper name '%ls' is not valid.", wrapped_script_path);
    }

    wcsncpy_s(p, MAX_PATH - (p - wrapped_script_path) + 1, SCRIPT_SUFFIX, _TRUNCATE);
    attrs = GetFileAttributesW(wrapped_script_path);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        debug(L"File '%ls' non-existent\n", wrapped_script_path);
        error(RC_NO_SCRIPT, L"Script file '%ls' is not present.", wrapped_script_path);
    }

    debug(L"Using wrapped script file '%ls'\n", wrapped_script_path);
}
#endif

/*
 * Process creation code
 */

static BOOL
safe_duplicate_handle(HANDLE in, HANDLE * pout)
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
            debug(L"DuplicateHandle returned ERROR_INVALID_HANDLE\n");
            ok = TRUE;
        }
        else {
            debug(L"DuplicateHandle returned %d\n", rc);
        }
    }
    return ok;
}

static BOOL WINAPI
ctrl_c_handler(DWORD code)
{
    return TRUE;    /* We just ignore all control events. */
}

static void
run_child(wchar_t * cmdline)
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
    See http://bugs.python.org/issue17290 and
    https://bitbucket.org/vinay.sajip/pylauncher/issue/20/busy-cursor-for-a-long-time-when-running
    */
    MSG msg;

    PostMessage(0, 0, 0, 0);
    GetMessage(&msg, 0, 0, 0);
#endif

    debug(L"run_child: about to run '%ls'\n", cmdline);
    job = CreateJobObject(NULL, NULL);
    ok = QueryInformationJobObject(job, JobObjectExtendedLimitInformation,
                                  &info, sizeof(info), &rc);
    if (!ok || (rc != sizeof(info)) || !job)
        error(RC_CREATE_PROCESS, L"Job information querying failed");
    info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
                                             JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
    ok = SetInformationJobObject(job, JobObjectExtendedLimitInformation, &info,
                                 sizeof(info));
    if (!ok)
        error(RC_CREATE_PROCESS, L"Job information setting failed");
    memset(&si, 0, sizeof(si));
    GetStartupInfoW(&si);
    ok = safe_duplicate_handle(GetStdHandle(STD_INPUT_HANDLE), &si.hStdInput);
    if (!ok)
        error(RC_NO_STD_HANDLES, L"stdin duplication failed");
    ok = safe_duplicate_handle(GetStdHandle(STD_OUTPUT_HANDLE), &si.hStdOutput);
    if (!ok)
        error(RC_NO_STD_HANDLES, L"stdout duplication failed");
    ok = safe_duplicate_handle(GetStdHandle(STD_ERROR_HANDLE), &si.hStdError);
    if (!ok)
        error(RC_NO_STD_HANDLES, L"stderr duplication failed");

    ok = SetConsoleCtrlHandler(ctrl_c_handler, TRUE);
    if (!ok)
        error(RC_CREATE_PROCESS, L"control handler setting failed");

    si.dwFlags = STARTF_USESTDHANDLES;
    ok = CreateProcessW(NULL, cmdline, NULL, NULL, TRUE,
                        0, NULL, NULL, &si, &pi);
    if (!ok)
        error(RC_CREATE_PROCESS, L"Unable to create process using '%ls'", cmdline);
    AssignProcessToJobObject(job, pi.hProcess);
    CloseHandle(pi.hThread);
    WaitForSingleObjectEx(pi.hProcess, INFINITE, FALSE);
    ok = GetExitCodeProcess(pi.hProcess, &rc);
    if (!ok)
        error(RC_CREATE_PROCESS, L"Failed to get exit code of process");
    debug(L"child process exit code: %d\n", rc);
    exit(rc);
}

static void
invoke_child(wchar_t * executable, wchar_t * suffix, wchar_t * cmdline)
{
    wchar_t * child_command;
    size_t child_command_size;
    BOOL no_suffix = (suffix == NULL) || (*suffix == L'\0');
    BOOL no_cmdline = (*cmdline == L'\0');

    if (no_suffix && no_cmdline)
        run_child(executable);
    else {
        if (no_suffix) {
            /* add 2 for space separator + terminating NUL. */
            child_command_size = wcslen(executable) + wcslen(cmdline) + 2;
        }
        else {
            /* add 3 for 2 space separators + terminating NUL. */
            child_command_size = wcslen(executable) + wcslen(suffix) +
                                    wcslen(cmdline) + 3;
        }
        child_command = calloc(child_command_size, sizeof(wchar_t));
        if (child_command == NULL)
            error(RC_CREATE_PROCESS, L"unable to allocate %zd bytes for child command.",
                  child_command_size);
        if (no_suffix)
            _snwprintf_s(child_command, child_command_size,
                         child_command_size - 1, L"%ls %ls",
                         executable, cmdline);
        else
            _snwprintf_s(child_command, child_command_size,
                         child_command_size - 1, L"%ls %ls %ls",
                         executable, suffix, cmdline);
        run_child(child_command);
        free(child_command);
    }
}

typedef struct {
    wchar_t *shebang;
    BOOL search;
} SHEBANG;

static SHEBANG builtin_virtual_paths [] = {
    { L"/usr/bin/env python", TRUE },
    { L"/usr/bin/python", FALSE },
    { L"/usr/local/bin/python", FALSE },
    { L"python", FALSE },
    { NULL, FALSE },
};

/* For now, a static array of commands. */

#define MAX_COMMANDS 100

typedef struct {
    wchar_t key[MAX_PATH];
    wchar_t value[MSGSIZE];
} COMMAND;

static COMMAND commands[MAX_COMMANDS];
static int num_commands = 0;

#if defined(SKIP_PREFIX)

static wchar_t * builtin_prefixes [] = {
    /* These must be in an order that the longest matches should be found,
     * i.e. if the prefix is "/usr/bin/env ", it should match that entry
     * *before* matching "/usr/bin/".
     */
    L"/usr/bin/env ",
    L"/usr/bin/",
    L"/usr/local/bin/",
    NULL
};

static wchar_t * skip_prefix(wchar_t * name)
{
    wchar_t ** pp = builtin_prefixes;
    wchar_t * result = name;
    wchar_t * p;
    size_t n;

    for (; p = *pp; pp++) {
        n = wcslen(p);
        if (_wcsnicmp(p, name, n) == 0) {
            result += n;   /* skip the prefix */
            if (p[n - 1] == L' ') /* No empty strings in table, so n > 1 */
                result = skip_whitespace(result);
            break;
        }
    }
    return result;
}

#endif

#if defined(SEARCH_PATH)

static COMMAND path_command;

static COMMAND * find_on_path(wchar_t * name)
{
    wchar_t * pathext;
    size_t    varsize;
    wchar_t * context = NULL;
    wchar_t * extension;
    COMMAND * result = NULL;
    DWORD     len;
    errno_t   rc;

    wcscpy_s(path_command.key, MAX_PATH, name);
    if (wcschr(name, L'.') != NULL) {
        /* assume it has an extension. */
        len = SearchPathW(NULL, name, NULL, MSGSIZE, path_command.value, NULL);
        if (len) {
            result = &path_command;
        }
    }
    else {
        /* No extension - search using registered extensions. */
        rc = _wdupenv_s(&pathext, &varsize, L"PATHEXT");
        if (rc == 0) {
            extension = wcstok_s(pathext, L";", &context);
            while (extension) {
                len = SearchPathW(NULL, name, extension, MSGSIZE, path_command.value, NULL);
                if (len) {
                    result = &path_command;
                    break;
                }
                extension = wcstok_s(NULL, L";", &context);
            }
            free(pathext);
        }
    }
    return result;
}

#endif

static COMMAND * find_command(wchar_t * name)
{
    COMMAND * result = NULL;
    COMMAND * cp = commands;
    int i;

    for (i = 0; i < num_commands; i++, cp++) {
        if (_wcsicmp(cp->key, name) == 0) {
            result = cp;
            break;
        }
    }
#if defined(SEARCH_PATH)
    if (result == NULL)
        result = find_on_path(name);
#endif
    return result;
}

static void
update_command(COMMAND * cp, wchar_t * name, wchar_t * cmdline)
{
    wcsncpy_s(cp->key, MAX_PATH, name, _TRUNCATE);
    wcsncpy_s(cp->value, MSGSIZE, cmdline, _TRUNCATE);
}

static void
add_command(wchar_t * name, wchar_t * cmdline)
{
    if (num_commands >= MAX_COMMANDS) {
        debug(L"can't add %ls = '%ls': no room\n", name, cmdline);
    }
    else {
        COMMAND * cp = &commands[num_commands++];

        update_command(cp, name, cmdline);
    }
}

static void
read_config_file(wchar_t * config_path)
{
    wchar_t keynames[MSGSIZE];
    wchar_t value[MSGSIZE];
    DWORD read;
    wchar_t * key;
    COMMAND * cp;
    wchar_t * cmdp;

    read = GetPrivateProfileStringW(L"commands", NULL, NULL, keynames, MSGSIZE,
                                    config_path);
    if (read == MSGSIZE - 1) {
        debug(L"read_commands: %ls: not enough space for names\n", config_path);
    }
    key = keynames;
    while (*key) {
        read = GetPrivateProfileStringW(L"commands", key, NULL, value, MSGSIZE,
                                       config_path);
        if (read == MSGSIZE - 1) {
            debug(L"read_commands: %ls: not enough space for %ls\n",
                  config_path, key);
        }
        cmdp = skip_whitespace(value);
        if (*cmdp) {
            cp = find_command(key);
            if (cp == NULL)
                add_command(key, value);
            else
                update_command(cp, key, value);
        }
        key += wcslen(key) + 1;
    }
}

static void read_commands()
{
    if (launcher_ini_path[0])
        read_config_file(launcher_ini_path);
    if (appdata_ini_path[0])
        read_config_file(appdata_ini_path);
}

static BOOL
parse_shebang(wchar_t * shebang_line, int nchars, wchar_t ** command,
              wchar_t ** suffix, BOOL *search)
{
    BOOL rc = FALSE;
    SHEBANG * vpp;
    size_t plen;
    wchar_t * p;
    wchar_t zapped;
    wchar_t * endp = shebang_line + nchars - 1;
    COMMAND * cp;
    wchar_t * skipped;

    *command = NULL;    /* failure return */
    *suffix = NULL;
    *search = FALSE;

    if ((*shebang_line++ == L'#') && (*shebang_line++ == L'!')) {
        shebang_line = skip_whitespace(shebang_line);
        if (*shebang_line) {
            *command = shebang_line;
            for (vpp = builtin_virtual_paths; vpp->shebang; ++vpp) {
                plen = wcslen(vpp->shebang);
                if (wcsncmp(shebang_line, vpp->shebang, plen) == 0) {
                    rc = TRUE;
                    *search = vpp->search;
                    /* We can do this because all builtin commands contain
                     * "python".
                     */
                    *command = wcsstr(shebang_line, L"python");
                    break;
                }
            }
            if (vpp->shebang == NULL) {
                /*
                 * Not found in builtins - look in customized commands.
                 *
                 * We can't permanently modify the shebang line in case
                 * it's not a customized command, but we can temporarily
                 * stick a NUL after the command while searching for it,
                 * then put back the char we zapped.
                 */
#if defined(SKIP_PREFIX)
                skipped = skip_prefix(shebang_line);
#else
                skipped = shebang_line;
#endif
                p = wcspbrk(skipped, L" \t\r\n");
                if (p != NULL) {
                    zapped = *p;
                    *p = L'\0';
                }
                cp = find_command(skipped);
                if (p != NULL)
                    *p = zapped;
                if (cp != NULL) {
                    *command = cp->value;
                    if (p != NULL)
                        *suffix = skip_whitespace(p);
                }
            }
            /* remove trailing whitespace */
            while ((endp > shebang_line) && isspace(*endp))
                --endp;
            if (endp > shebang_line)
                endp[1] = L'\0';
        }
    }
    return rc;
}

/* #define CP_UTF8             65001 defined in winnls.h */
#define CP_UTF16LE          1200
#define CP_UTF16BE          1201
#define CP_UTF32LE          12000
#define CP_UTF32BE          12001

typedef struct {
    int length;
    char sequence[4];
    UINT code_page;
} BOM;

/*
 * Strictly, we don't need to handle UTF-16 and UTF-32, since Python itself
 * doesn't. Never mind, one day it might - there's no harm leaving it in.
 */
static BOM BOMs[] = {
    { 3, { 0xEF, 0xBB, 0xBF }, CP_UTF8 },           /* UTF-8 - keep first */
    /* Test UTF-32LE before UTF-16LE since UTF-16LE BOM is a prefix
     * of UTF-32LE BOM. */
    { 4, { 0xFF, 0xFE, 0x00, 0x00 }, CP_UTF32LE },  /* UTF-32LE */
    { 4, { 0x00, 0x00, 0xFE, 0xFF }, CP_UTF32BE },  /* UTF-32BE */
    { 2, { 0xFF, 0xFE }, CP_UTF16LE },              /* UTF-16LE */
    { 2, { 0xFE, 0xFF }, CP_UTF16BE },              /* UTF-16BE */
    { 0 }                                           /* sentinel */
};

static BOM *
find_BOM(char * buffer)
{
/*
 * Look for a BOM in the input and return a pointer to the
 * corresponding structure, or NULL if not found.
 */
    BOM * result = NULL;
    BOM *bom;

    for (bom = BOMs; bom->length; bom++) {
        if (strncmp(bom->sequence, buffer, bom->length) == 0) {
            result = bom;
            break;
        }
    }
    return result;
}

static char *
find_terminator(char * buffer, int len, BOM *bom)
{
    char * result = NULL;
    char * end = buffer + len;
    char  * p;
    char c;
    int cp;

    for (p = buffer; p < end; p++) {
        c = *p;
        if (c == '\r') {
            result = p;
            break;
        }
        if (c == '\n') {
            result = p;
            break;
        }
    }
    if (result != NULL) {
        cp = bom->code_page;

        /* adjustments to include all bytes of the char */
        /* no adjustment needed for UTF-8 or big endian */
        if (cp == CP_UTF16LE)
            ++result;
        else if (cp == CP_UTF32LE)
            result += 3;
        ++result; /* point just past terminator */
    }
    return result;
}

static BOOL
validate_version(wchar_t * p)
{
    /*
    Version information should start with the major version,
    Optionally followed by a period and a minor version,
    Optionally followed by a minus and one of 32 or 64.
    Valid examples:
      2
      3
      2.7
      3.6
      2.7-32
      The intent is to add to the valid patterns:
      3.10
      3-32
      3.6-64
      3-64
    */
    BOOL result = (p != NULL); /* Default to False if null pointer. */

    result = result && iswdigit(*p);  /* Result = False if first string element is not a digit. */

    while (result && iswdigit(*p))   /* Require a major version */
        ++p;  /* Skip all leading digit(s) */
    if (result && (*p == L'.'))     /* Allow . for major minor separator.*/
    {
        result = iswdigit(*++p);     /* Must be at least one digit */
        while (result && iswdigit(*++p)) ; /* Skip any more Digits */
    }
    if (result && (*p == L'-')) {   /* Allow - for Bits Separator */
        switch(*++p){
        case L'3':                            /* 3 is OK */
            result = (*++p == L'2') && !*++p; /* only if followed by 2 and ended.*/
            break;
        case L'6':                            /* 6 is OK */
            result = (*++p == L'4') && !*++p; /* only if followed by 4 and ended.*/
            break;
        default:
            result = FALSE;
            break;
        }
    }
    result = result && !*p; /* Must have reached EOS */
    return result;

}

typedef struct {
    unsigned short min;
    unsigned short max;
    wchar_t version[MAX_VERSION_SIZE];
} PYC_MAGIC;

static PYC_MAGIC magic_values[] = {
    { 50823, 50823, L"2.0" },
    { 60202, 60202, L"2.1" },
    { 60717, 60717, L"2.2" },
    { 62011, 62021, L"2.3" },
    { 62041, 62061, L"2.4" },
    { 62071, 62131, L"2.5" },
    { 62151, 62161, L"2.6" },
    { 62171, 62211, L"2.7" },
    { 3000, 3131, L"3.0" },
    { 3141, 3151, L"3.1" },
    { 3160, 3180, L"3.2" },
    { 3190, 3230, L"3.3" },
    { 3250, 3310, L"3.4" },
    { 3320, 3351, L"3.5" },
    { 3360, 3379, L"3.6" },
    { 3390, 3399, L"3.7" },
    { 3400, 3419, L"3.8" },
    { 3420, 3429, L"3.9" },
    { 3430, 3449, L"3.10" },
    { 3450, 3469, L"3.11" },
    { 0 }
};

static INSTALLED_PYTHON *
find_by_magic(unsigned short magic)
{
    INSTALLED_PYTHON * result = NULL;
    PYC_MAGIC * mp;

    for (mp = magic_values; mp->min; mp++) {
        if ((magic >= mp->min) && (magic <= mp->max)) {
            result = locate_python(mp->version, FALSE);
            if (result != NULL)
                break;
        }
    }
    return result;
}

static void
maybe_handle_shebang(wchar_t ** argv, wchar_t * cmdline)
{
/*
 * Look for a shebang line in the first argument.  If found
 * and we spawn a child process, this never returns.  If it
 * does return then we process the args "normally".
 *
 * argv[0] might be a filename with a shebang.
 */
    FILE * fp;
    errno_t rc = _wfopen_s(&fp, *argv, L"rb");
    char buffer[BUFSIZE];
    wchar_t shebang_line[BUFSIZE + 1];
    size_t read;
    char *p;
    char * start;
    char * shebang_alias = (char *) shebang_line;
    BOM* bom;
    int i, j, nchars = 0;
    int header_len;
    BOOL is_virt;
    BOOL search;
    wchar_t * command;
    wchar_t * suffix;
    COMMAND *cmd = NULL;
    INSTALLED_PYTHON * ip;

    if (rc == 0) {
        read = fread(buffer, sizeof(char), BUFSIZE, fp);
        debug(L"maybe_handle_shebang: read %zd bytes\n", read);
        fclose(fp);

        if ((read >= 4) && (buffer[3] == '\n') && (buffer[2] == '\r')) {
            ip = find_by_magic((((unsigned char)buffer[1]) << 8 |
                                (unsigned char)buffer[0]) & 0xFFFF);
            if (ip != NULL) {
                debug(L"script file is compiled against Python %ls\n",
                      ip->version);
                invoke_child(ip->executable, NULL, cmdline);
            }
        }
        /* Look for BOM */
        bom = find_BOM(buffer);
        if (bom == NULL) {
            start = buffer;
            debug(L"maybe_handle_shebang: BOM not found, using UTF-8\n");
            bom = BOMs; /* points to UTF-8 entry - the default */
        }
        else {
            debug(L"maybe_handle_shebang: BOM found, code page %u\n",
                  bom->code_page);
            start = &buffer[bom->length];
        }
        p = find_terminator(start, BUFSIZE, bom);
        /*
         * If no CR or LF was found in the heading,
         * we assume it's not a shebang file.
         */
        if (p == NULL) {
            debug(L"maybe_handle_shebang: No line terminator found\n");
        }
        else {
            /*
             * Found line terminator - parse the shebang.
             *
             * Strictly, we don't need to handle UTF-16 anf UTF-32,
             * since Python itself doesn't.
             * Never mind, one day it might.
             */
            header_len = (int) (p - start);
            switch(bom->code_page) {
            case CP_UTF8:
                nchars = MultiByteToWideChar(bom->code_page,
                                             0,
                                             start, header_len, shebang_line,
                                             BUFSIZE);
                break;
            case CP_UTF16BE:
                if (header_len % 2 != 0) {
                    debug(L"maybe_handle_shebang: UTF-16BE, but an odd number \
of bytes: %d\n", header_len);
                    /* nchars = 0; Not needed - initialised to 0. */
                }
                else {
                    for (i = header_len; i > 0; i -= 2) {
                        shebang_alias[i - 1] = start[i - 2];
                        shebang_alias[i - 2] = start[i - 1];
                    }
                    nchars = header_len / sizeof(wchar_t);
                }
                break;
            case CP_UTF16LE:
                if ((header_len % 2) != 0) {
                    debug(L"UTF-16LE, but an odd number of bytes: %d\n",
                          header_len);
                    /* nchars = 0; Not needed - initialised to 0. */
                }
                else {
                    /* no actual conversion needed. */
                    memcpy(shebang_line, start, header_len);
                    nchars = header_len / sizeof(wchar_t);
                }
                break;
            case CP_UTF32BE:
                if (header_len % 4 != 0) {
                    debug(L"UTF-32BE, but not divisible by 4: %d\n",
                          header_len);
                    /* nchars = 0; Not needed - initialised to 0. */
                }
                else {
                    for (i = header_len, j = header_len / 2; i > 0; i -= 4,
                                                                    j -= 2) {
                        shebang_alias[j - 1] = start[i - 2];
                        shebang_alias[j - 2] = start[i - 1];
                    }
                    nchars = header_len / sizeof(wchar_t);
                }
                break;
            case CP_UTF32LE:
                if (header_len % 4 != 0) {
                    debug(L"UTF-32LE, but not divisible by 4: %d\n",
                          header_len);
                    /* nchars = 0; Not needed - initialised to 0. */
                }
                else {
                    for (i = header_len, j = header_len / 2; i > 0; i -= 4,
                                                                    j -= 2) {
                        shebang_alias[j - 1] = start[i - 3];
                        shebang_alias[j - 2] = start[i - 4];
                    }
                    nchars = header_len / sizeof(wchar_t);
                }
                break;
            }
            if (nchars > 0) {
                shebang_line[--nchars] = L'\0';
                is_virt = parse_shebang(shebang_line, nchars, &command,
                                        &suffix, &search);
                if (command != NULL) {
                    debug(L"parse_shebang: found command: %ls\n", command);
                    if (!is_virt) {
                        invoke_child(command, suffix, cmdline);
                    }
                    else {
                        suffix = wcschr(command, L' ');
                        if (suffix != NULL) {
                            *suffix++ = L'\0';
                            suffix = skip_whitespace(suffix);
                        }
                        if (wcsncmp(command, L"python", 6))
                            error(RC_BAD_VIRTUAL_PATH, L"Unknown virtual \
path '%ls'", command);
                        command += 6;   /* skip past "python" */
                        if (search && ((*command == L'\0') || isspace(*command))) {
                            /* Command is eligible for path search, and there
                             * is no version specification.
                             */
                            debug(L"searching PATH for python executable\n");
                            cmd = find_on_path(PYTHON_EXECUTABLE);
                            debug(L"Python on path: %ls\n", cmd ? cmd->value : L"<not found>");
                            if (cmd) {
                                debug(L"located python on PATH: %ls\n", cmd->value);
                                invoke_child(cmd->value, suffix, cmdline);
                                /* Exit here, as we have found the command */
                                return;
                            }
                            /* FALL THROUGH: No python found on PATH, so fall
                             * back to locating the correct installed python.
                             */
                        }
                        if (*command && !validate_version(command))
                            error(RC_BAD_VIRTUAL_PATH, L"Invalid version \
specification: '%ls'.\nIn the first line of the script, 'python' needs to be \
followed by a valid version specifier.\nPlease check the documentation.",
                                  command);
                        /* TODO could call validate_version(command) */
                        ip = locate_python(command, TRUE);
                        if (ip == NULL) {
                            error(RC_NO_PYTHON, L"Requested Python version \
(%ls) is not installed", command);
                        }
                        else {
                            invoke_child(ip->executable, suffix, cmdline);
                        }
                    }
                }
            }
        }
    }
}

static wchar_t *
skip_me(wchar_t * cmdline)
{
    BOOL quoted;
    wchar_t c;
    wchar_t * result = cmdline;

    quoted = cmdline[0] == L'\"';
    if (!quoted)
        c = L' ';
    else {
        c = L'\"';
        ++result;
    }
    result = wcschr(result, c);
    if (result == NULL) /* when, for example, just exe name on command line */
        result = L"";
    else {
        ++result; /* skip past space or closing quote */
        result = skip_whitespace(result);
    }
    return result;
}

static DWORD version_high = 0;
static DWORD version_low = 0;

static void
get_version_info(wchar_t * version_text, size_t size)
{
    WORD maj, min, rel, bld;

    if (!version_high && !version_low)
        wcsncpy_s(version_text, size, L"0.1", _TRUNCATE);   /* fallback */
    else {
        maj = HIWORD(version_high);
        min = LOWORD(version_high);
        rel = HIWORD(version_low);
        bld = LOWORD(version_low);
        _snwprintf_s(version_text, size, _TRUNCATE, L"%d.%d.%d.%d", maj,
                     min, rel, bld);
    }
}

static void
show_help_text(wchar_t ** argv)
{
    wchar_t version_text [MAX_PATH];
#if defined(_M_X64)
    BOOL canDo64bit = TRUE;
#else
    /* If we are a 32bit process on a 64bit Windows, first hit the 64bit keys. */
    BOOL canDo64bit = FALSE;
    IsWow64Process(GetCurrentProcess(), &canDo64bit);
#endif

    get_version_info(version_text, MAX_PATH);
    fwprintf(stdout, L"\
Python Launcher for Windows Version %ls\n\n", version_text);
    fwprintf(stdout, L"\
usage:\n\
%ls [launcher-args] [python-args] [script [script-args]]\n\n", argv[0]);
    fputws(L"\
Launcher arguments:\n\n\
-2     : Launch the latest Python 2.x version\n\
-3     : Launch the latest Python 3.x version\n\
-X.Y   : Launch the specified Python version\n", stdout);
    if (canDo64bit) {
        fputws(L"\
     The above all default to 64 bit if a matching 64 bit python is present.\n\
-X.Y-32: Launch the specified 32bit Python version\n\
-X-32  : Launch the latest 32bit Python X version\n\
-X.Y-64: Launch the specified 64bit Python version\n\
-X-64  : Launch the latest 64bit Python X version", stdout);
    }
    fputws(L"\n-0  --list       : List the available pythons", stdout);
    fputws(L"\n-0p --list-paths : List with paths", stdout);
    fputws(L"\n\n If no script is specified the specified interpreter is opened.", stdout);
    fputws(L"\nIf an exact version is not given, using the latest version can be overridden by", stdout);
    fputws(L"\nany of the following, (in priority order):", stdout);
    fputws(L"\n An active virtual environment", stdout);
    fputws(L"\n A shebang line in the script (if present)", stdout);
    fputws(L"\n With -2 or -3 flag a matching PY_PYTHON2 or PY_PYTHON3 Environment variable", stdout);
    fputws(L"\n A PY_PYTHON Environment variable", stdout);
    fputws(L"\n From [defaults] in py.ini in your %LOCALAPPDATA%\\py.ini", stdout);
    fputws(L"\n From [defaults] in py.ini beside py.exe (use `where py` to locate)", stdout);
    fputws(L"\n\nThe following help text is from Python:\n\n", stdout);
    fflush(stdout);
}

static BOOL
show_python_list(wchar_t ** argv)
{
    /*
     * Display options -0
     */
    INSTALLED_PYTHON * result = NULL;
    INSTALLED_PYTHON * ip = installed_pythons; /* List of installed pythons */
    INSTALLED_PYTHON * defpy = locate_python(L"", FALSE);
    size_t i = 0;
    wchar_t *p = argv[1];
    wchar_t *ver_fmt = L"-%ls-%d";
    wchar_t *fmt = L"\n %ls";
    wchar_t *defind = L" *"; /* Default indicator */

    /*
    * Output informational messages to stderr to keep output
    * clean for use in pipes, etc.
    */
    fwprintf(stderr,
             L"Installed Pythons found by %s Launcher for Windows", argv[0]);
    if (!_wcsicmp(p, L"-0p") || !_wcsicmp(p, L"--list-paths"))
        fmt = L"\n %-15ls%ls"; /* include path */

    if (num_installed_pythons == 0) /* We have somehow got here without searching for pythons */
        locate_all_pythons(); /* Find them, Populates installed_pythons */

    if (num_installed_pythons == 0) /* No pythons found */
        fwprintf(stderr, L"\nNo Installed Pythons Found!");
    else
    {
        for (i = 0; i < num_installed_pythons; i++, ip++) {
            wchar_t version[BUFSIZ];
            if (wcscmp(ip->version, L"venv") == 0) {
                wcscpy_s(version, BUFSIZ, L"(venv)");
            }
            else {
                swprintf_s(version, BUFSIZ, ver_fmt, ip->version, ip->bits);
            }

            if (ip->exe_display[0]) {
                fwprintf(stdout, fmt, version, ip->exe_display);
            }
            else {
                fwprintf(stdout, fmt, version, ip->executable);
            }
            /* If there is a default indicate it */
            if (defpy == ip)
                fwprintf(stderr, defind);
        }
    }

    if ((defpy == NULL) && (num_installed_pythons > 0))
        /* We have pythons but none is the default */
        fwprintf(stderr, L"\n\nCan't find a Default Python.\n\n");
    else
        fwprintf(stderr, L"\n\n"); /* End with a blank line */
    return FALSE; /* If this has been called we cannot continue */
}

#if defined(VENV_REDIRECT)

static int
find_home_value(const char *buffer, const char **start, DWORD *length)
{
    for (const char *s = strstr(buffer, "home"); s; s = strstr(s + 1, "\nhome")) {
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
            *length = (DWORD)((ptrdiff_t)nl - (ptrdiff_t)s);
        } else {
            *length = (DWORD)strlen(s);
        }
        return 1;
    }
    return 0;
}
#endif

static wchar_t *
wcsdup_pad(const wchar_t *s, int padding, int *newlen)
{
    size_t len = wcslen(s);
    len += 1 + padding;
    wchar_t *r = (wchar_t *)malloc(len * sizeof(wchar_t));
    if (!r) {
        return NULL;
    }
    if (wcscpy_s(r, len, s)) {
        free(r);
        return NULL;
    }
    *newlen = len < MAXINT ? (int)len : MAXINT;
    return r;
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
            error(RC_NO_MEMORY, L"out of memory");
            return NULL;
        }
        len = GetModuleFileNameW(NULL, r, bufferLen);
        if (len == 0) {
            free(r);
            error(0, L"Failed to get module name");
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

static int
process(int argc, wchar_t ** argv)
{
    wchar_t * wp;
    wchar_t * command;
    wchar_t * executable;
    wchar_t * p;
    wchar_t * argv0;
    int rc = 0;
    INSTALLED_PYTHON * ip;
    BOOL valid;
    DWORD size, attrs;
    wchar_t message[MSGSIZE];
    void * version_data;
    VS_FIXEDFILEINFO * file_info;
    UINT block_size;
#if defined(VENV_REDIRECT)
    wchar_t * venv_cfg_path;
    int newlen;
#elif defined(SCRIPT_WRAPPER)
    wchar_t * newcommand;
    wchar_t * av[2];
    int newlen;
    HRESULT hr;
    int index;
#else
    HRESULT hr;
    int index;
#endif

    setvbuf(stderr, (char *)NULL, _IONBF, 0);
    wp = get_env(L"PYLAUNCH_DEBUG");
    if ((wp != NULL) && (*wp != L'\0'))
        log_fp = stderr;

#if defined(_M_X64)
    debug(L"launcher build: 64bit\n");
#else
    debug(L"launcher build: 32bit\n");
#endif
#if defined(_WINDOWS)
    debug(L"launcher executable: Windows\n");
#else
    debug(L"launcher executable: Console\n");
#endif
#if !defined(VENV_REDIRECT)
    /* Get the local appdata folder (non-roaming) */
    hr = SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA,
                          NULL, 0, appdata_ini_path);
    if (hr != S_OK) {
        debug(L"SHGetFolderPath failed: %X\n", hr);
        appdata_ini_path[0] = L'\0';
    }
    else {
        wcsncat_s(appdata_ini_path, MAX_PATH, L"\\py.ini", _TRUNCATE);
        attrs = GetFileAttributesW(appdata_ini_path);
        if (attrs == INVALID_FILE_ATTRIBUTES) {
            debug(L"File '%ls' non-existent\n", appdata_ini_path);
            appdata_ini_path[0] = L'\0';
        } else {
            debug(L"Using local configuration file '%ls'\n", appdata_ini_path);
        }
    }
#endif
    argv0 = get_process_name();
    size = GetFileVersionInfoSizeW(argv0, &size);
    if (size == 0) {
        winerror(GetLastError(), message, MSGSIZE);
        debug(L"GetFileVersionInfoSize failed: %ls\n", message);
    }
    else {
        version_data = malloc(size);
        if (version_data) {
            valid = GetFileVersionInfoW(argv0, 0, size,
                                        version_data);
            if (!valid)
                debug(L"GetFileVersionInfo failed: %X\n", GetLastError());
            else {
                valid = VerQueryValueW(version_data, L"\\",
                                       (LPVOID *) &file_info, &block_size);
                if (!valid)
                    debug(L"VerQueryValue failed: %X\n", GetLastError());
                else {
                    version_high = file_info->dwFileVersionMS;
                    version_low = file_info->dwFileVersionLS;
                }
            }
            free(version_data);
        }
    }

#if defined(VENV_REDIRECT)
    /* Allocate some extra space for new filenames */
    venv_cfg_path = wcsdup_pad(argv0, 32, &newlen);
    if (!venv_cfg_path) {
        error(RC_NO_MEMORY, L"Failed to copy module name");
    }
    p = wcsrchr(venv_cfg_path, L'\\');

    if (p == NULL) {
        error(RC_NO_VENV_CFG, L"No pyvenv.cfg file");
    }
    p[0] = L'\0';
    wcscat_s(venv_cfg_path, newlen, L"\\pyvenv.cfg");
    attrs = GetFileAttributesW(venv_cfg_path);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        debug(L"File '%ls' non-existent\n", venv_cfg_path);
        p[0] = '\0';
        p = wcsrchr(venv_cfg_path, L'\\');
        if (p != NULL) {
            p[0] = '\0';
            wcscat_s(venv_cfg_path, newlen, L"\\pyvenv.cfg");
            attrs = GetFileAttributesW(venv_cfg_path);
            if (attrs == INVALID_FILE_ATTRIBUTES) {
                debug(L"File '%ls' non-existent\n", venv_cfg_path);
                error(RC_NO_VENV_CFG, L"No pyvenv.cfg file");
            }
        }
    }
    debug(L"Using venv configuration file '%ls'\n", venv_cfg_path);
#else
    /* Allocate some extra space for new filenames */
    if (wcscpy_s(launcher_ini_path, MAX_PATH, argv0)) {
        error(RC_NO_MEMORY, L"Failed to copy module name");
    }
    p = wcsrchr(launcher_ini_path, L'\\');

    if (p == NULL) {
        debug(L"GetModuleFileNameW returned value has no backslash: %ls\n",
              launcher_ini_path);
        launcher_ini_path[0] = L'\0';
    }
    else {
        p[0] = L'\0';
        wcscat_s(launcher_ini_path, MAX_PATH, L"\\py.ini");
        attrs = GetFileAttributesW(launcher_ini_path);
        if (attrs == INVALID_FILE_ATTRIBUTES) {
            debug(L"File '%ls' non-existent\n", launcher_ini_path);
            launcher_ini_path[0] = L'\0';
        } else {
            debug(L"Using global configuration file '%ls'\n", launcher_ini_path);
        }
    }
#endif

    command = skip_me(GetCommandLineW());
    debug(L"Called with command line: %ls\n", command);

#if !defined(VENV_REDIRECT)
    /* bpo-35811: The __PYVENV_LAUNCHER__ variable is used to
     * override sys.executable and locate the original prefix path.
     * However, if it is silently inherited by a non-venv Python
     * process, that process will believe it is running in the venv
     * still. This is the only place where *we* can clear it (that is,
     * when py.exe is being used to launch Python), so we do.
     */
    SetEnvironmentVariableW(L"__PYVENV_LAUNCHER__", NULL);
#endif

#if defined(SCRIPT_WRAPPER)
    /* The launcher is being used in "script wrapper" mode.
     * There should therefore be a Python script named <exename>-script.py in
     * the same directory as the launcher executable.
     * Put the script name into argv as the first (script name) argument.
     */

    /* Get the wrapped script name - if the script is not present, this will
     * terminate the program with an error.
     */
    locate_wrapped_script();

    /* Add the wrapped script to the start of command */
    newlen = wcslen(wrapped_script_path) + wcslen(command) + 2; /* ' ' + NUL */
    newcommand = malloc(sizeof(wchar_t) * newlen);
    if (!newcommand) {
        error(RC_NO_MEMORY, L"Could not allocate new command line");
    }
    else {
        wcscpy_s(newcommand, newlen, wrapped_script_path);
        wcscat_s(newcommand, newlen, L" ");
        wcscat_s(newcommand, newlen, command);
        debug(L"Running wrapped script with command line '%ls'\n", newcommand);
        read_commands();
        av[0] = wrapped_script_path;
        av[1] = NULL;
        maybe_handle_shebang(av, newcommand);
        /* Returns if no shebang line - pass to default processing */
        command = newcommand;
        valid = FALSE;
    }
#elif defined(VENV_REDIRECT)
    {
        FILE *f;
        char buffer[4096]; /* 4KB should be enough for anybody */
        char *start;
        DWORD len, cch, cch_actual;
        size_t cb;
        if (_wfopen_s(&f, venv_cfg_path, L"r")) {
            error(RC_BAD_VENV_CFG, L"Cannot read '%ls'", venv_cfg_path);
        }
        cb = fread_s(buffer, sizeof(buffer), sizeof(buffer[0]),
                     sizeof(buffer) / sizeof(buffer[0]), f);
        fclose(f);

        if (!find_home_value(buffer, &start, &len)) {
            error(RC_BAD_VENV_CFG, L"Cannot find home in '%ls'",
                  venv_cfg_path);
        }

        cch = MultiByteToWideChar(CP_UTF8, 0, start, len, NULL, 0);
        if (!cch) {
            error(0, L"Cannot determine memory for home path");
        }
        cch += (DWORD)wcslen(PYTHON_EXECUTABLE) + 1 + 1; /* include sep and null */
        executable = (wchar_t *)malloc(cch * sizeof(wchar_t));
        if (executable == NULL) {
            error(RC_NO_MEMORY, L"A memory allocation failed");
        }
        cch_actual = MultiByteToWideChar(CP_UTF8, 0, start, len, executable, cch);
        if (!cch_actual) {
            error(RC_BAD_VENV_CFG, L"Cannot decode home path in '%ls'",
                  venv_cfg_path);
        }
        if (executable[cch_actual - 1] != L'\\') {
            executable[cch_actual++] = L'\\';
            executable[cch_actual] = L'\0';
        }
        if (wcscat_s(executable, cch, PYTHON_EXECUTABLE)) {
            error(RC_BAD_VENV_CFG, L"Cannot create executable path from '%ls'",
                  venv_cfg_path);
        }
        if (GetFileAttributesW(executable) == INVALID_FILE_ATTRIBUTES) {
            error(RC_NO_PYTHON, L"No Python at '%ls'", executable);
        }
        if (!SetEnvironmentVariableW(L"__PYVENV_LAUNCHER__", argv0)) {
            error(0, L"Failed to set launcher environment");
        }
        valid = 1;
    }
#else
    if (argc <= 1) {
        valid = FALSE;
        p = NULL;
    }
    else {
        p = argv[1];
        if ((argc == 2) && // list version args
            (!wcsncmp(p, L"-0", wcslen(L"-0")) ||
            !wcsncmp(p, L"--list", wcslen(L"--list"))))
        {
            show_python_list(argv);
            return rc;
        }
        valid = valid && (*p == L'-') && validate_version(&p[1]);
        if (valid) {
            ip = locate_python(&p[1], FALSE);
            if (ip == NULL)
            {
                fwprintf(stdout, \
                         L"Python %ls not found!\n", &p[1]);
                valid = show_python_list(argv);
                error(RC_NO_PYTHON, L"Requested Python version (%ls) not \
installed, use -0 for available pythons", &p[1]);
            }
            executable = ip->executable;
            command += wcslen(p);
            command = skip_whitespace(command);
        }
        else {
            for (index = 1; index < argc; ++index) {
                if (*argv[index] != L'-')
                    break;
            }
            if (index < argc) {
                read_commands();
                maybe_handle_shebang(&argv[index], command);
            }
        }
    }
#endif

    if (!valid) {
        if ((argc == 2) && (!_wcsicmp(p, L"-h") || !_wcsicmp(p, L"--help")))
            show_help_text(argv);
        if ((argc == 2) &&
            (!_wcsicmp(p, L"-0") || !_wcsicmp(p, L"--list") ||
            !_wcsicmp(p, L"-0p") || !_wcsicmp(p, L"--list-paths")))
        {
            executable = NULL; /* Info call only */
        }
        else {
            /* look for the default Python */
            ip = locate_python(L"", FALSE);
            if (ip == NULL)
                error(RC_NO_PYTHON, L"Can't find a default Python.");
            executable = ip->executable;
        }
    }
    if (executable != NULL)
        invoke_child(executable, NULL, command);
    else
        rc = RC_NO_PYTHON;
    return rc;
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
