/*
 * Rewritten Python launcher for Windows
 *
 * This new rewrite properly handles PEP 514 and allows any registered Python
 * runtime to be launched. It also enables auto-install of versions when they
 * are requested but no installation can be found.
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
#define RC_BAD_VIRTUAL_PATH 102
#define RC_NO_PYTHON        103
#define RC_NO_MEMORY        104
#define RC_NO_SCRIPT        105
#define RC_NO_VENV_CFG      106
#define RC_BAD_VENV_CFG     107
#define RC_NO_COMMANDLINE   108
#define RC_INTERNAL_ERROR   109
#define RC_DUPLICATE_ITEM   110
#define RC_INSTALLING       111
#define RC_NO_PYTHON_AT_ALL 112
#define RC_NO_SHEBANG       113
#define RC_RECURSIVE_SHEBANG 114

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
        fputws(buffer, log_fp);
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
        _snwprintf_s(&message[len], MSGSIZE - len, _TRUNCATE, L": %s",
                     win_message);
    }

#if !defined(_WINDOWS)
    fwprintf(stderr, L"%s\n", message);
#else
    MessageBoxW(NULL, message, L"Python Launcher is sorry to say ...",
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
    fwprintf(stderr, L"%s\n", message);
#else
    MessageBoxW(NULL, message, L"Python Launcher is sorry to say ...",
               MB_OK);
#endif
}


typedef BOOL (*PIsWow64Process2)(HANDLE, USHORT*, USHORT*);


USHORT
_getNativeMachine(void)
{
    static USHORT _nativeMachine = IMAGE_FILE_MACHINE_UNKNOWN;
    if (_nativeMachine == IMAGE_FILE_MACHINE_UNKNOWN) {
        USHORT processMachine;
        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        PIsWow64Process2 IsWow64Process2 = kernel32 ?
            (PIsWow64Process2)GetProcAddress(kernel32, "IsWow64Process2") :
            NULL;
        if (!IsWow64Process2) {
            BOOL wow64Process;
            if (!IsWow64Process(NULL, &wow64Process)) {
                winerror(0, L"Checking process type");
            } else if (wow64Process) {
                // We should always be a 32-bit executable, so if running
                // under emulation, it must be a 64-bit host.
                _nativeMachine = IMAGE_FILE_MACHINE_AMD64;
            } else {
                // Not running under emulation, and an old enough OS to not
                // have IsWow64Process2, so assume it's x86.
                _nativeMachine = IMAGE_FILE_MACHINE_I386;
            }
        } else if (!IsWow64Process2(NULL, &processMachine, &_nativeMachine)) {
            winerror(0, L"Checking process type");
        }
    }
    return _nativeMachine;
}


bool
isAMD64Host(void)
{
    return _getNativeMachine() == IMAGE_FILE_MACHINE_AMD64;
}


bool
isARM64Host(void)
{
    return _getNativeMachine() == IMAGE_FILE_MACHINE_ARM64;
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


int
_compare(const wchar_t *x, int xLen, const wchar_t *y, int yLen)
{
    // Empty strings sort first
    if (!x || !xLen) {
        return (!y || !yLen) ? 0 : -1;
    } else if (!y || !yLen) {
        return 1;
    }
    switch (CompareStringEx(
        LOCALE_NAME_INVARIANT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS,
        x, xLen, y, yLen,
        NULL, NULL, 0
    )) {
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_EQUAL:
        return 0;
    case CSTR_GREATER_THAN:
        return 1;
    default:
        winerror(0, L"Error comparing '%.*s' and '%.*s' (compare)", xLen, x, yLen, y);
        return -1;
    }
}


int
_compareArgument(const wchar_t *x, int xLen, const wchar_t *y, int yLen)
{
    // Empty strings sort first
    if (!x || !xLen) {
        return (!y || !yLen) ? 0 : -1;
    } else if (!y || !yLen) {
        return 1;
    }
    switch (CompareStringEx(
        LOCALE_NAME_INVARIANT, 0,
        x, xLen, y, yLen,
        NULL, NULL, 0
    )) {
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_EQUAL:
        return 0;
    case CSTR_GREATER_THAN:
        return 1;
    default:
        winerror(0, L"Error comparing '%.*s' and '%.*s' (compareArgument)", xLen, x, yLen, y);
        return -1;
    }
}

int
_comparePath(const wchar_t *x, int xLen, const wchar_t *y, int yLen)
{
    // Empty strings sort first
    if (!x || !xLen) {
        return !y || !yLen ? 0 : -1;
    } else if (!y || !yLen) {
        return 1;
    }
    switch (CompareStringOrdinal(x, xLen, y, yLen, TRUE)) {
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_EQUAL:
        return 0;
    case CSTR_GREATER_THAN:
        return 1;
    default:
        winerror(0, L"Error comparing '%.*s' and '%.*s' (comparePath)", xLen, x, yLen, y);
        return -1;
    }
}


bool
_startsWith(const wchar_t *x, int xLen, const wchar_t *y, int yLen)
{
    if (!x || !y) {
        return false;
    }
    yLen = yLen < 0 ? (int)wcsnlen_s(y, MAXLEN) : yLen;
    xLen = xLen < 0 ? (int)wcsnlen_s(x, MAXLEN) : xLen;
    return xLen >= yLen && 0 == _compare(x, yLen, y, yLen);
}


bool
_startsWithArgument(const wchar_t *x, int xLen, const wchar_t *y, int yLen)
{
    if (!x || !y) {
        return false;
    }
    yLen = yLen < 0 ? (int)wcsnlen_s(y, MAXLEN) : yLen;
    xLen = xLen < 0 ? (int)wcsnlen_s(x, MAXLEN) : xLen;
    return xLen >= yLen && 0 == _compareArgument(x, yLen, y, yLen);
}


// Unlike regular startsWith, this function requires that the following
// character is either NULL (that is, the entire string matches) or is one of
// the characters in 'separators'.
bool
_startsWithSeparated(const wchar_t *x, int xLen, const wchar_t *y, int yLen, const wchar_t *separators)
{
    if (!x || !y) {
        return false;
    }
    yLen = yLen < 0 ? (int)wcsnlen_s(y, MAXLEN) : yLen;
    xLen = xLen < 0 ? (int)wcsnlen_s(x, MAXLEN) : xLen;
    if (xLen < yLen) {
        return false;
    }
    if (xLen == yLen) {
        return 0 == _compare(x, xLen, y, yLen);
    }
    return separators &&
        0 == _compare(x, yLen, y, yLen) &&
        wcschr(separators, x[yLen]) != NULL;
}



/******************************************************************************\
 ***                               HELP TEXT                                ***
\******************************************************************************/


int
showHelpText(wchar_t ** argv)
{
    // The help text is stored in launcher-usage.txt, which is compiled into
    // the launcher and loaded at runtime if needed.
    //
    // The file must be UTF-8. There are two substitutions:
    //  %ls - PY_VERSION (as wchar_t*)
    //  %ls - argv[0] (as wchar_t*)
    HRSRC res = FindResourceExW(NULL, L"USAGE", MAKEINTRESOURCE(1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    HGLOBAL resData = res ? LoadResource(NULL, res) : NULL;
    const char *usage = resData ? (const char*)LockResource(resData) : NULL;
    if (usage == NULL) {
        winerror(0, L"Unable to load usage text");
        return RC_INTERNAL_ERROR;
    }

    DWORD cbData = SizeofResource(NULL, res);
    DWORD cchUsage = MultiByteToWideChar(CP_UTF8, 0, usage, cbData, NULL, 0);
    if (!cchUsage) {
        winerror(0, L"Unable to preprocess usage text");
        return RC_INTERNAL_ERROR;
    }

    cchUsage += 1;
    wchar_t *wUsage = (wchar_t*)malloc(cchUsage * sizeof(wchar_t));
    cchUsage = MultiByteToWideChar(CP_UTF8, 0, usage, cbData, wUsage, cchUsage);
    if (!cchUsage) {
        winerror(0, L"Unable to preprocess usage text");
        free((void *)wUsage);
        return RC_INTERNAL_ERROR;
    }
    // Ensure null termination
    wUsage[cchUsage] = L'\0';

    fwprintf(stdout, wUsage, (L"" PY_VERSION), argv[0]);
    fflush(stdout);

    free((void *)wUsage);

    return 0;
}


/******************************************************************************\
 ***                              SEARCH INFO                               ***
\******************************************************************************/


struct _SearchInfoBuffer {
    struct _SearchInfoBuffer *next;
    wchar_t buffer[0];
};


typedef struct {
    // the original string, managed by the OS
    const wchar_t *originalCmdLine;
    // pointer into the cmdline to mark what we've consumed
    const wchar_t *restOfCmdLine;
    // if known/discovered, the full executable path of our runtime
    const wchar_t *executablePath;
    // pointer and length into cmdline for the file to check for a
    // shebang line, if any. Length can be -1 if the string is null
    // terminated.
    const wchar_t *scriptFile;
    int scriptFileLength;
    // pointer and length into cmdline or a static string with the
    // name of the target executable. Length can be -1 if the string
    // is null terminated.
    const wchar_t *executable;
    int executableLength;
    // pointer and length into a string with additional interpreter
    // arguments to include before restOfCmdLine. Length can be -1 if
    // the string is null terminated.
    const wchar_t *executableArgs;
    int executableArgsLength;
    // pointer and length into cmdline or a static string with the
    // company name for PEP 514 lookup. Length can be -1 if the string
    // is null terminated.
    const wchar_t *company;
    int companyLength;
    // pointer and length into cmdline or a static string with the
    // tag for PEP 514 lookup. Length can be -1 if the string is
    // null terminated.
    const wchar_t *tag;
    int tagLength;
    // if true, treats 'tag' as a non-PEP 514 filter
    bool oldStyleTag;
    // if true, ignores 'tag' when a high priority environment is found
    // gh-92817: This is currently set when a tag is read from configuration,
    // the environment, or a shebang, rather than the command line, and the
    // only currently possible high priority environment is an active virtual
    // environment
    bool lowPriorityTag;
    // if true, allow PEP 514 lookup to override 'executable'
    bool allowExecutableOverride;
    // if true, allow a nearby pyvenv.cfg to locate the executable
    bool allowPyvenvCfg;
    // if true, allow defaults (env/py.ini) to clarify/override tags
    bool allowDefaults;
    // if true, prefer windowed (console-less) executable
    bool windowed;
    // if true, only list detected runtimes without launching
    bool list;
    // if true, only list detected runtimes with paths without launching
    bool listPaths;
    // if true, display help message before continuing
    bool help;
    // if set, limits search to registry keys with the specified Company
    // This is intended for debugging and testing only
    const wchar_t *limitToCompany;
    // dynamically allocated buffers to free later
    struct _SearchInfoBuffer *_buffer;
} SearchInfo;


wchar_t *
allocSearchInfoBuffer(SearchInfo *search, int wcharCount)
{
    struct _SearchInfoBuffer *buffer = (struct _SearchInfoBuffer*)malloc(
        sizeof(struct _SearchInfoBuffer) +
        wcharCount * sizeof(wchar_t)
    );
    if (!buffer) {
        return NULL;
    }
    buffer->next = search->_buffer;
    search->_buffer = buffer;
    return buffer->buffer;
}


void
freeSearchInfo(SearchInfo *search)
{
    struct _SearchInfoBuffer *b = search->_buffer;
    search->_buffer = NULL;
    while (b) {
        struct _SearchInfoBuffer *nextB = b->next;
        free((void *)b);
        b = nextB;
    }
}


void
_debugStringAndLength(const wchar_t *s, int len, const wchar_t *name)
{
    if (!s) {
        debug(L"%s: (null)\n", name);
    } else if (len == 0) {
        debug(L"%s: (empty)\n", name);
    } else if (len < 0) {
        debug(L"%s: %s\n", name, s);
    } else {
        debug(L"%s: %.*ls\n", name, len, s);
    }
}


void
dumpSearchInfo(SearchInfo *search)
{
    if (!log_fp) {
        return;
    }

#ifdef __clang__
#define DEBUGNAME(s) L # s
#else
#define DEBUGNAME(s) # s
#endif
#define DEBUG(s) debug(L"SearchInfo." DEBUGNAME(s) L": %s\n", (search->s) ? (search->s) : L"(null)")
#define DEBUG_2(s, sl) _debugStringAndLength((search->s), (search->sl), L"SearchInfo." DEBUGNAME(s))
#define DEBUG_BOOL(s) debug(L"SearchInfo." DEBUGNAME(s) L": %s\n", (search->s) ? L"True" : L"False")
    DEBUG(originalCmdLine);
    DEBUG(restOfCmdLine);
    DEBUG(executablePath);
    DEBUG_2(scriptFile, scriptFileLength);
    DEBUG_2(executable, executableLength);
    DEBUG_2(executableArgs, executableArgsLength);
    DEBUG_2(company, companyLength);
    DEBUG_2(tag, tagLength);
    DEBUG_BOOL(oldStyleTag);
    DEBUG_BOOL(lowPriorityTag);
    DEBUG_BOOL(allowDefaults);
    DEBUG_BOOL(allowExecutableOverride);
    DEBUG_BOOL(windowed);
    DEBUG_BOOL(list);
    DEBUG_BOOL(listPaths);
    DEBUG_BOOL(help);
    DEBUG(limitToCompany);
#undef DEBUG_BOOL
#undef DEBUG_2
#undef DEBUG
#undef DEBUGNAME
}


int
findArgv0Length(const wchar_t *buffer, int bufferLength)
{
    // Note: this implements semantics that are only valid for argv0.
    // Specifically, there is no escaping of quotes, and quotes within
    // the argument have no effect. A quoted argv0 must start and end
    // with a double quote character; otherwise, it ends at the first
    // ' ' or '\t'.
    int quoted = buffer[0] == L'"';
    for (int i = 1; bufferLength < 0 || i < bufferLength; ++i) {
        switch (buffer[i]) {
        case L'\0':
            return i;
        case L' ':
        case L'\t':
            if (!quoted) {
                return i;
            }
            break;
        case L'"':
            if (quoted) {
                return i + 1;
            }
            break;
        }
    }
    return bufferLength;
}


const wchar_t *
findArgv0End(const wchar_t *buffer, int bufferLength)
{
    return &buffer[findArgv0Length(buffer, bufferLength)];
}


/******************************************************************************\
 ***                          COMMAND-LINE PARSING                          ***
\******************************************************************************/

// Adapted from https://stackoverflow.com/a/65583702
typedef struct AppExecLinkFile { // For tag IO_REPARSE_TAG_APPEXECLINK
    DWORD reparseTag;
    WORD reparseDataLength;
    WORD reserved;
    ULONG version;
    wchar_t stringList[MAX_PATH * 4];  // Multistring (Consecutive UTF-16 strings each ending with a NUL)
    /* There are normally 4 strings here. Ex:
        Package ID:  L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe"
        Entry Point: L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe!PythonRedirector"
        Executable:  L"C:\Program Files\WindowsApps\Microsoft.DesktopAppInstaller_1.17.106910_x64__8wekyb3d8bbwe\AppInstallerPythonRedirector.exe"
        Applic. Type: L"0"   // Integer as ASCII. "0" = Desktop bridge application; Else sandboxed UWP application
    */
} AppExecLinkFile;


int
parseCommandLine(SearchInfo *search)
{
    if (!search || !search->originalCmdLine) {
        return RC_NO_COMMANDLINE;
    }

    const wchar_t *argv0End = findArgv0End(search->originalCmdLine, -1);
    const wchar_t *tail = argv0End; // will be start of the executable name
    const wchar_t *end = argv0End;  // will be end of the executable name
    search->restOfCmdLine = argv0End;   // will be first space after argv0
    while (--tail != search->originalCmdLine) {
        if (*tail == L'"' && end == argv0End) {
            // Move the "end" up to the quote, so we also allow moving for
            // a period later on.
            end = argv0End = tail;
        } else if (*tail == L'.' && end == argv0End) {
            end = tail;
        } else if (*tail == L'\\' || *tail == L'/') {
            ++tail;
            break;
        }
    }
    if (tail == search->originalCmdLine && tail[0] == L'"') {
        ++tail;
    }
    // Without special cases, we can now fill in the search struct
    int tailLen = (int)(end ? (end - tail) : wcsnlen_s(tail, MAXLEN));
    search->executableLength = -1;

    // Our special cases are as follows
#define MATCHES(s) (0 == _comparePath(tail, tailLen, (s), -1))
#define STARTSWITH(s) _startsWith(tail, tailLen, (s), -1)
    if (MATCHES(L"py")) {
        search->executable = L"python.exe";
        search->allowExecutableOverride = true;
        search->allowDefaults = true;
    } else if (MATCHES(L"pyw")) {
        search->executable = L"pythonw.exe";
        search->allowExecutableOverride = true;
        search->allowDefaults = true;
        search->windowed = true;
    } else if (MATCHES(L"py_d")) {
        search->executable = L"python_d.exe";
        search->allowExecutableOverride = true;
        search->allowDefaults = true;
    } else if (MATCHES(L"pyw_d")) {
        search->executable = L"pythonw_d.exe";
        search->allowExecutableOverride = true;
        search->allowDefaults = true;
        search->windowed = true;
    } else if (STARTSWITH(L"python3")) {
        search->executable = L"python.exe";
        search->tag = &tail[6];
        search->tagLength = tailLen - 6;
        search->allowExecutableOverride = true;
        search->oldStyleTag = true;
        search->allowPyvenvCfg = true;
    } else if (STARTSWITH(L"pythonw3")) {
        search->executable = L"pythonw.exe";
        search->tag = &tail[7];
        search->tagLength = tailLen - 7;
        search->allowExecutableOverride = true;
        search->oldStyleTag = true;
        search->allowPyvenvCfg = true;
        search->windowed = true;
    } else {
        search->executable = tail;
        search->executableLength = tailLen;
        search->allowPyvenvCfg = true;
    }
#undef STARTSWITH
#undef MATCHES

    // First argument might be one of our options. If so, consume it,
    // update flags and then set restOfCmdLine.
    const wchar_t *arg = search->restOfCmdLine;
    while(*arg && isspace(*arg)) { ++arg; }
#define MATCHES(s) (0 == _compareArgument(arg, argLen, (s), -1))
#define STARTSWITH(s) _startsWithArgument(arg, argLen, (s), -1)
    if (*arg && *arg == L'-' && *++arg) {
        tail = arg;
        while (*tail && !isspace(*tail)) { ++tail; }
        int argLen = (int)(tail - arg);
        if (argLen > 0) {
            if (STARTSWITH(L"2") || STARTSWITH(L"3")) {
                // All arguments starting with 2 or 3 are assumed to be version tags
                search->tag = arg;
                search->tagLength = argLen;
                search->oldStyleTag = true;
                search->restOfCmdLine = tail;
            } else if (STARTSWITH(L"V:") || STARTSWITH(L"-version:")) {
                // Arguments starting with 'V:' specify company and/or tag
                const wchar_t *argStart = wcschr(arg, L':') + 1;
                const wchar_t *tagStart = wcschr(argStart, L'/') ;
                if (tagStart) {
                    search->company = argStart;
                    search->companyLength = (int)(tagStart - argStart);
                    search->tag = tagStart + 1;
                } else {
                    search->tag = argStart;
                }
                search->tagLength = (int)(tail - search->tag);
                search->allowDefaults = false;
                search->restOfCmdLine = tail;
            } else if (MATCHES(L"0") || MATCHES(L"-list")) {
                search->list = true;
                search->restOfCmdLine = tail;
            } else if (MATCHES(L"0p") || MATCHES(L"-list-paths")) {
                search->listPaths = true;
                search->restOfCmdLine = tail;
            } else if (MATCHES(L"h") || MATCHES(L"-help")) {
                search->help = true;
                // Do not update restOfCmdLine so that we trigger the help
                // message from whichever interpreter we select
            }
        }
    }
#undef STARTSWITH
#undef MATCHES

    // Might have a script filename. If it looks like a filename, add
    // it to the SearchInfo struct for later reference.
    arg = search->restOfCmdLine;
    while(*arg && isspace(*arg)) { ++arg; }
    if (*arg && *arg != L'-') {
        search->scriptFile = arg;
        if (*arg == L'"') {
            ++search->scriptFile;
            while (*++arg && *arg != L'"') { }
        } else {
            while (*arg && !isspace(*arg)) { ++arg; }
        }
        search->scriptFileLength = (int)(arg - search->scriptFile);
    }

    return 0;
}


int
_decodeShebang(SearchInfo *search, const char *buffer, int bufferLength, bool onlyUtf8, wchar_t **decoded, int *decodedLength)
{
    DWORD cp = CP_UTF8;
    int wideLen = MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, buffer, bufferLength, NULL, 0);
    if (!wideLen) {
        cp = CP_ACP;
        wideLen = MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, buffer, bufferLength, NULL, 0);
        if (!wideLen) {
            debug(L"# Failed to decode shebang line (0x%08X)\n", GetLastError());
            return RC_BAD_VIRTUAL_PATH;
        }
    }
    wchar_t *b = allocSearchInfoBuffer(search, wideLen + 1);
    if (!b) {
        return RC_NO_MEMORY;
    }
    wideLen = MultiByteToWideChar(cp, 0, buffer, bufferLength, b, wideLen + 1);
    if (!wideLen) {
        debug(L"# Failed to decode shebang line (0x%08X)\n", GetLastError());
        return RC_BAD_VIRTUAL_PATH;
    }
    b[wideLen] = L'\0';
    *decoded = b;
    *decodedLength = wideLen;
    return 0;
}


bool
_shebangStartsWith(const wchar_t *buffer, int bufferLength, const wchar_t *prefix, const wchar_t **rest, int *firstArgumentLength)
{
    int prefixLength = (int)wcsnlen_s(prefix, MAXLEN);
    if (bufferLength < prefixLength || !_startsWithArgument(buffer, bufferLength, prefix, prefixLength)) {
        return false;
    }
    if (rest) {
        *rest = &buffer[prefixLength];
    }
    if (firstArgumentLength) {
        int i = prefixLength;
        while (i < bufferLength && !isspace(buffer[i])) {
            i += 1;
        }
        *firstArgumentLength = i - prefixLength;
    }
    return true;
}


int
ensure_no_redirector_stub(wchar_t* filename, wchar_t* buffer)
{
    // Make sure we didn't find a reparse point that will open the Microsoft Store
    // If we did, pretend there was no shebang and let normal handling take over
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(buffer, &findData);
    if (!hFind) {
        // Let normal handling take over
        debug(L"# Did not find %s on PATH\n", filename);
        return RC_NO_SHEBANG;
    }

    FindClose(hFind);

    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT &&
        findData.dwReserved0 & IO_REPARSE_TAG_APPEXECLINK)) {
        return 0;
    }

    HANDLE hReparsePoint = CreateFileW(buffer, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (!hReparsePoint) {
        // Let normal handling take over
        debug(L"# Did not find %s on PATH\n", filename);
        return RC_NO_SHEBANG;
    }

    AppExecLinkFile appExecLink;

    if (!DeviceIoControl(hReparsePoint, FSCTL_GET_REPARSE_POINT, NULL, 0, &appExecLink, sizeof(appExecLink), NULL, NULL)) {
        // Let normal handling take over
        debug(L"# Did not find %s on PATH\n", filename);
        CloseHandle(hReparsePoint);
        return RC_NO_SHEBANG;
    }

    CloseHandle(hReparsePoint);

    const wchar_t* redirectorPackageId = L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe";

    if (0 == wcscmp(appExecLink.stringList, redirectorPackageId)) {
        debug(L"# ignoring redirector that would launch store\n");
        return RC_NO_SHEBANG;
    }

    return 0;
}


int
searchPath(SearchInfo *search, const wchar_t *shebang, int shebangLength)
{
    if (isEnvVarSet(L"PYLAUNCHER_NO_SEARCH_PATH")) {
        return RC_NO_SHEBANG;
    }

    wchar_t *command;
    int commandLength;
    if (!_shebangStartsWith(shebang, shebangLength, L"/usr/bin/env ", &command, &commandLength)) {
        return RC_NO_SHEBANG;
    }

    if (!commandLength || commandLength == MAXLEN) {
        return RC_BAD_VIRTUAL_PATH;
    }

    int lastDot = commandLength;
    while (lastDot > 0 && command[lastDot] != L'.') {
        lastDot -= 1;
    }
    if (!lastDot) {
        lastDot = commandLength;
    }

    wchar_t filename[MAXLEN];
    if (wcsncpy_s(filename, MAXLEN, command, commandLength)) {
        return RC_BAD_VIRTUAL_PATH;
    }

    const wchar_t *ext = L".exe";
    // If the command already has an extension, we do not want to add it again
    if (!lastDot || _comparePath(&filename[lastDot], -1, ext, -1)) {
        if (wcscat_s(filename, MAXLEN, L".exe")) {
            return RC_BAD_VIRTUAL_PATH;
        }
    }

    debug(L"# Search PATH for %s\n", filename);

    wchar_t pathVariable[MAXLEN];
    int n = GetEnvironmentVariableW(L"PATH", pathVariable, MAXLEN);
    if (!n) {
        if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
            return RC_NO_SHEBANG;
        }
        winerror(0, L"Failed to read PATH\n", filename);
        return RC_INTERNAL_ERROR;
    }

    wchar_t buffer[MAXLEN];
    n = SearchPathW(pathVariable, filename, NULL, MAXLEN, buffer, NULL);
    if (!n) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            debug(L"# Did not find %s on PATH\n", filename);
            // If we didn't find it on PATH, let normal handling take over
            return RC_NO_SHEBANG;
        }
        // Other errors should cause us to break
        winerror(0, L"Failed to find %s on PATH\n", filename);
        return RC_BAD_VIRTUAL_PATH;
    }

    int result = ensure_no_redirector_stub(filename, buffer);
    if (result) {
        return result;
    }

    // Check that we aren't going to call ourselves again
    // If we are, pretend there was no shebang and let normal handling take over
    if (GetModuleFileNameW(NULL, filename, MAXLEN) &&
        0 == _comparePath(filename, -1, buffer, -1)) {
        debug(L"# ignoring recursive shebang command\n");
        return RC_RECURSIVE_SHEBANG;
    }

    wchar_t *buf = allocSearchInfoBuffer(search, n + 1);
    if (!buf || wcscpy_s(buf, n + 1, buffer)) {
        return RC_NO_MEMORY;
    }

    search->executablePath = buf;
    search->executableArgs = &command[commandLength];
    search->executableArgsLength = shebangLength - commandLength;
    debug(L"# Found %s on PATH\n", buf);

    return 0;
}


int
_readIni(const wchar_t *section, const wchar_t *settingName, wchar_t *buffer, int bufferLength)
{
    wchar_t iniPath[MAXLEN];
    int n;
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, iniPath)) &&
        join(iniPath, MAXLEN, L"py.ini")) {
        debug(L"# Reading from %s for %s/%s\n", iniPath, section, settingName);
        n = GetPrivateProfileStringW(section, settingName, NULL, buffer, bufferLength, iniPath);
        if (n) {
            debug(L"# Found %s in %s\n", settingName, iniPath);
            return n;
        } else if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            debug(L"# Did not find file %s\n", iniPath);
        } else {
            winerror(0, L"Failed to read from %s\n", iniPath);
        }
    }
    if (GetModuleFileNameW(NULL, iniPath, MAXLEN) &&
        SUCCEEDED(PathCchRemoveFileSpec(iniPath, MAXLEN)) &&
        join(iniPath, MAXLEN, L"py.ini")) {
        debug(L"# Reading from %s for %s/%s\n", iniPath, section, settingName);
        n = GetPrivateProfileStringW(section, settingName, NULL, buffer, MAXLEN, iniPath);
        if (n) {
            debug(L"# Found %s in %s\n", settingName, iniPath);
            return n;
        } else if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            debug(L"# Did not find file %s\n", iniPath);
        } else {
            winerror(0, L"Failed to read from %s\n", iniPath);
        }
    }
    return 0;
}


bool
_findCommand(SearchInfo *search, const wchar_t *command, int commandLength)
{
    wchar_t commandBuffer[MAXLEN];
    wchar_t buffer[MAXLEN];
    wcsncpy_s(commandBuffer, MAXLEN, command, commandLength);
    int n = _readIni(L"commands", commandBuffer, buffer, MAXLEN);
    if (!n) {
        return false;
    }
    wchar_t *path = allocSearchInfoBuffer(search, n + 1);
    if (!path) {
        return false;
    }
    wcscpy_s(path, n + 1, buffer);
    search->executablePath = path;
    return true;
}


int
_useShebangAsExecutable(SearchInfo *search, const wchar_t *shebang, int shebangLength)
{
    wchar_t buffer[MAXLEN];
    wchar_t script[MAXLEN];
    wchar_t command[MAXLEN];

    int commandLength = 0;
    int inQuote = 0;

    if (!shebang || !shebangLength) {
        return 0;
    }

    wchar_t *pC = command;
    for (int i = 0; i < shebangLength; ++i) {
        wchar_t c = shebang[i];
        if (isspace(c) && !inQuote) {
            commandLength = i;
            break;
        } else if (c == L'"') {
            inQuote = !inQuote;
        } else if (c == L'/' || c == L'\\') {
            *pC++ = L'\\';
        } else {
            *pC++ = c;
        }
    }
    *pC = L'\0';

    if (!GetCurrentDirectoryW(MAXLEN, buffer) ||
        wcsncpy_s(script, MAXLEN, search->scriptFile, search->scriptFileLength) ||
        FAILED(PathCchCombineEx(buffer, MAXLEN, buffer, script,
                                PATHCCH_ALLOW_LONG_PATHS)) ||
        FAILED(PathCchRemoveFileSpec(buffer, MAXLEN)) ||
        FAILED(PathCchCombineEx(buffer, MAXLEN, buffer, command,
                                PATHCCH_ALLOW_LONG_PATHS))
    ) {
        return RC_NO_MEMORY;
    }

    int n = (int)wcsnlen(buffer, MAXLEN);
    wchar_t *path = allocSearchInfoBuffer(search, n + 1);
    if (!path) {
        return RC_NO_MEMORY;
    }
    wcscpy_s(path, n + 1, buffer);
    search->executablePath = path;
    if (commandLength) {
        search->executableArgs = &shebang[commandLength];
        search->executableArgsLength = shebangLength - commandLength;
    }
    return 0;
}


int
checkShebang(SearchInfo *search)
{
    // Do not check shebang if a tag was provided or if no script file
    // was found on the command line.
    if (search->tag || !search->scriptFile) {
        return 0;
    }

    if (search->scriptFileLength < 0) {
        search->scriptFileLength = (int)wcsnlen_s(search->scriptFile, MAXLEN);
    }

    wchar_t *scriptFile = (wchar_t*)malloc(sizeof(wchar_t) * (search->scriptFileLength + 1));
    if (!scriptFile) {
        return RC_NO_MEMORY;
    }

    wcsncpy_s(scriptFile, search->scriptFileLength + 1,
              search->scriptFile, search->scriptFileLength);

    HANDLE hFile = CreateFileW(scriptFile, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        debug(L"# Failed to open %s for shebang parsing (0x%08X)\n",
              scriptFile, GetLastError());
        free(scriptFile);
        return 0;
    }

    DWORD bytesRead = 0;
    unsigned char buffer[4096];
    if (!ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, NULL)) {
        debug(L"# Failed to read %s for shebang parsing (0x%08X)\n",
              scriptFile, GetLastError());
        free(scriptFile);
        return 0;
    }

    CloseHandle(hFile);
    debug(L"# Read %d bytes from %s to find shebang line\n", bytesRead, scriptFile);
    free(scriptFile);


    unsigned char *b = buffer;
    bool onlyUtf8 = false;
    if (bytesRead > 3 && *b == 0xEF) {
        if (*++b == 0xBB && *++b == 0xBF) {
            // Allow a UTF-8 BOM
            ++b;
            bytesRead -= 3;
            onlyUtf8 = true;
        } else {
            debug(L"# Invalid BOM in shebang line");
            return 0;
        }
    }
    if (bytesRead <= 2 || b[0] != '#' || b[1] != '!') {
        // No shebang (#!) at start of line
        debug(L"# No valid shebang line");
        return 0;
    }
    ++b;
    --bytesRead;
    while (--bytesRead > 0 && isspace(*++b)) { }
    const unsigned char *start = b;
    while (--bytesRead > 0 && *++b != '\r' && *b != '\n') { }
    wchar_t *shebang;
    int shebangLength;
    // We add 1 when bytesRead==0, as in that case we hit EOF and b points
    // to the last character in the file, not the newline
    int exitCode = _decodeShebang(search, (const char*)start, (int)(b - start + (bytesRead == 0)), onlyUtf8, &shebang, &shebangLength);
    if (exitCode) {
        return exitCode;
    }
    debug(L"Shebang: %s\n", shebang);

    // Handle shebangs that we should search PATH for
    int executablePathWasSetByUsrBinEnv = 0;
    exitCode = searchPath(search, shebang, shebangLength);
    if (exitCode == 0) {
        executablePathWasSetByUsrBinEnv = 1;
    } else if (exitCode != RC_NO_SHEBANG) {
        return exitCode;
    }

    // Handle some known, case-sensitive shebangs
    const wchar_t *command;
    int commandLength;
    // Each template must end with "python"
    static const wchar_t *shebangTemplates[] = {
        L"/usr/bin/env python",
        L"/usr/bin/python",
        L"/usr/local/bin/python",
        L"python",
        NULL
    };

    for (const wchar_t **tmpl = shebangTemplates; *tmpl; ++tmpl) {
        // Just to make sure we don't mess this up in the future
        assert(0 == wcscmp(L"python", (*tmpl) + wcslen(*tmpl) - 6));

        if (_shebangStartsWith(shebang, shebangLength, *tmpl, &command, &commandLength)) {
            // Search for "python{command}" overrides. All templates end with
            // "python", so we prepend it by jumping back 6 characters
            if (_findCommand(search, &command[-6], commandLength + 6)) {
                search->executableArgs = &command[commandLength];
                search->executableArgsLength = shebangLength - commandLength;
                debug(L"# Treating shebang command '%.*s' as %s\n",
                    commandLength + 6, &command[-6], search->executablePath);
                return 0;
            }

            search->tag = command;
            search->tagLength = commandLength;
            // If we had 'python3.12.exe' then we want to strip the suffix
            // off of the tag
            if (search->tagLength >= 4) {
                const wchar_t *suffix = &search->tag[search->tagLength - 4];
                if (0 == _comparePath(suffix, 4, L".exe", -1)) {
                    search->tagLength -= 4;
                }
            }
            // If we had 'python3_d' then we want to strip the '_d' (any
            // '.exe' is already gone)
            if (search->tagLength >= 2) {
                const wchar_t *suffix = &search->tag[search->tagLength - 2];
                if (0 == _comparePath(suffix, 2, L"_d", -1)) {
                    search->tagLength -= 2;
                }
            }
            search->oldStyleTag = true;
            search->lowPriorityTag = true;
            search->executableArgs = &command[commandLength];
            search->executableArgsLength = shebangLength - commandLength;
            if (search->tag && search->tagLength) {
                debug(L"# Treating shebang command '%.*s' as 'py -%.*s'\n",
                    commandLength, command, search->tagLength, search->tag);
            } else {
                debug(L"# Treating shebang command '%.*s' as 'py'\n",
                    commandLength, command);
            }
            return 0;
        }
    }

    // Didn't match a template, but we found it on PATH
    if (executablePathWasSetByUsrBinEnv) {
        return 0;
    }

    // Unrecognised executables are first tried as command aliases
    commandLength = 0;
    while (commandLength < shebangLength && !isspace(shebang[commandLength])) {
        commandLength += 1;
    }
    if (_findCommand(search, shebang, commandLength)) {
        search->executableArgs = &shebang[commandLength];
        search->executableArgsLength = shebangLength - commandLength;
        debug(L"# Treating shebang command '%.*s' as %s\n",
            commandLength, shebang, search->executablePath);
        return 0;
    }

    // Unrecognised commands are joined to the script's directory and treated
    // as the executable path
    return _useShebangAsExecutable(search, shebang, shebangLength);
}


int
checkDefaults(SearchInfo *search)
{
    if (!search->allowDefaults) {
        return 0;
    }

    // Only resolve old-style (or absent) tags to defaults
    if (search->tag && search->tagLength && !search->oldStyleTag) {
        return 0;
    }

    // If tag is only a major version number, expand it from the environment
    // or an ini file
    const wchar_t *iniSettingName = NULL;
    const wchar_t *envSettingName = NULL;
    if (!search->tag || !search->tagLength) {
        iniSettingName = L"python";
        envSettingName = L"py_python";
    } else if (0 == wcsncmp(search->tag, L"3", search->tagLength)) {
        iniSettingName = L"python3";
        envSettingName = L"py_python3";
    } else if (0 == wcsncmp(search->tag, L"2", search->tagLength)) {
        iniSettingName = L"python2";
        envSettingName = L"py_python2";
    } else {
        debug(L"# Cannot select defaults for tag '%.*s'\n", search->tagLength, search->tag);
        return 0;
    }

    // First, try to read an environment variable
    wchar_t buffer[MAXLEN];
    int n = GetEnvironmentVariableW(envSettingName, buffer, MAXLEN);

    // If none found, check in our two .ini files instead
    if (!n) {
        n = _readIni(L"defaults", iniSettingName, buffer, MAXLEN);
    }

    if (n) {
        wchar_t *tag = allocSearchInfoBuffer(search, n + 1);
        if (!tag) {
            return RC_NO_MEMORY;
        }
        wcscpy_s(tag, n + 1, buffer);
        wchar_t *slash = wcschr(tag, L'/');
        if (!slash) {
            search->tag = tag;
            search->tagLength = n;
            search->oldStyleTag = true;
        } else {
            search->company = tag;
            search->companyLength = (int)(slash - tag);
            search->tag = slash + 1;
            search->tagLength = n - (search->companyLength + 1);
            search->oldStyleTag = false;
        }
        // gh-92817: allow a high priority env to be selected even if it
        // doesn't match the tag
        search->lowPriorityTag = true;
    }

    return 0;
}

/******************************************************************************\
 ***                          ENVIRONMENT SEARCH                            ***
\******************************************************************************/

typedef struct EnvironmentInfo {
    /* We use a binary tree and sort on insert */
    struct EnvironmentInfo *prev;
    struct EnvironmentInfo *next;
    /* parent is only used when constructing */
    struct EnvironmentInfo *parent;
    const wchar_t *company;
    const wchar_t *tag;
    int internalSortKey;
    const wchar_t *installDir;
    const wchar_t *executablePath;
    const wchar_t *executableArgs;
    const wchar_t *architecture;
    const wchar_t *displayName;
    bool highPriority;
} EnvironmentInfo;


int
copyWstr(const wchar_t **dest, const wchar_t *src)
{
    if (!dest) {
        return RC_NO_MEMORY;
    }
    if (!src) {
        *dest = NULL;
        return 0;
    }
    size_t n = wcsnlen_s(src, MAXLEN - 1) + 1;
    wchar_t *buffer = (wchar_t*)malloc(n * sizeof(wchar_t));
    if (!buffer) {
        return RC_NO_MEMORY;
    }
    wcsncpy_s(buffer, n, src, n - 1);
    *dest = (const wchar_t*)buffer;
    return 0;
}


EnvironmentInfo *
newEnvironmentInfo(const wchar_t *company, const wchar_t *tag)
{
    EnvironmentInfo *env = (EnvironmentInfo *)malloc(sizeof(EnvironmentInfo));
    if (!env) {
        return NULL;
    }
    memset(env, 0, sizeof(EnvironmentInfo));
    int exitCode = copyWstr(&env->company, company);
    if (exitCode) {
        free((void *)env);
        return NULL;
    }
    exitCode = copyWstr(&env->tag, tag);
    if (exitCode) {
        free((void *)env->company);
        free((void *)env);
        return NULL;
    }
    return env;
}


void
freeEnvironmentInfo(EnvironmentInfo *env)
{
    if (env) {
        free((void *)env->company);
        free((void *)env->tag);
        free((void *)env->installDir);
        free((void *)env->executablePath);
        free((void *)env->executableArgs);
        free((void *)env->displayName);
        freeEnvironmentInfo(env->prev);
        env->prev = NULL;
        freeEnvironmentInfo(env->next);
        env->next = NULL;
        free((void *)env);
    }
}


/* Specific string comparisons for sorting the tree */

int
_compareCompany(const wchar_t *x, const wchar_t *y)
{
    if (!x && !y) {
        return 0;
    } else if (!x) {
        return -1;
    } else if (!y) {
        return 1;
    }

    bool coreX = 0 == _compare(x, -1, L"PythonCore", -1);
    bool coreY = 0 == _compare(y, -1, L"PythonCore", -1);
    if (coreX) {
        return coreY ? 0 : -1;
    } else if (coreY) {
        return 1;
    }
    return _compare(x, -1, y, -1);
}


int
_compareTag(const wchar_t *x, const wchar_t *y)
{
    if (!x && !y) {
        return 0;
    } else if (!x) {
        return -1;
    } else if (!y) {
        return 1;
    }

    // Compare up to the first dash. If not equal, that's our sort order
    const wchar_t *xDash = wcschr(x, L'-');
    const wchar_t *yDash = wcschr(y, L'-');
    int xToDash = xDash ? (int)(xDash - x) : -1;
    int yToDash = yDash ? (int)(yDash - y) : -1;
    int r = _compare(x, xToDash, y, yToDash);
    if (r) {
        return r;
    }
    // If we're equal up to the first dash, we want to sort one with
    // no dash *after* one with a dash. Otherwise, a reversed compare.
    // This works out because environments are sorted in descending tag
    // order, so that higher versions (probably) come first.
    // For PythonCore, our "X.Y" structure ensures that higher versions
    // come first. Everyone else will just have to deal with it.
    if (xDash && yDash) {
        return _compare(yDash, -1, xDash, -1);
    } else if (xDash) {
        return -1;
    } else if (yDash) {
        return 1;
    }
    return 0;
}


int
addEnvironmentInfo(EnvironmentInfo **root, EnvironmentInfo* parent, EnvironmentInfo *node)
{
    EnvironmentInfo *r = *root;
    if (!r) {
        *root = node;
        node->parent = parent;
        return 0;
    }
    // Sort by company name
    switch (_compareCompany(node->company, r->company)) {
    case -1:
        return addEnvironmentInfo(&r->prev, r, node);
    case 1:
        return addEnvironmentInfo(&r->next, r, node);
    case 0:
        break;
    }
    // Then by tag (descending)
    switch (_compareTag(node->tag, r->tag)) {
    case -1:
        return addEnvironmentInfo(&r->next, r, node);
    case 1:
        return addEnvironmentInfo(&r->prev, r, node);
    case 0:
        break;
    }
    // Then keep the one with the lowest internal sort key
    if (node->internalSortKey < r->internalSortKey) {
        // Replace the current node
        node->parent = r->parent;
        if (node->parent) {
            if (node->parent->prev == r) {
                node->parent->prev = node;
            } else if (node->parent->next == r) {
                node->parent->next = node;
            } else {
                debug(L"# Inconsistent parent value in tree\n");
                freeEnvironmentInfo(node);
                return RC_INTERNAL_ERROR;
            }
        } else {
            // If node has no parent, then it is the root.
            *root = node;
        }

        node->next = r->next;
        node->prev = r->prev;

        debug(L"# replaced %s/%s/%i in tree\n", node->company, node->tag, node->internalSortKey);
        freeEnvironmentInfo(r);
    } else {
        debug(L"# not adding %s/%s/%i to tree\n", node->company, node->tag, node->internalSortKey);
        return RC_DUPLICATE_ITEM;
    }
    return 0;
}


/******************************************************************************\
 ***                            REGISTRY SEARCH                             ***
\******************************************************************************/


int
_registryReadString(const wchar_t **dest, HKEY root, const wchar_t *subkey, const wchar_t *value)
{
    // Note that this is bytes (hence 'cb'), not characters ('cch')
    DWORD cbData = 0;
    DWORD flags = RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ;

    if (ERROR_SUCCESS != RegGetValueW(root, subkey, value, flags, NULL, NULL, &cbData)) {
        return 0;
    }

    wchar_t *buffer = (wchar_t*)malloc(cbData);
    if (!buffer) {
        return RC_NO_MEMORY;
    }

    if (ERROR_SUCCESS == RegGetValueW(root, subkey, value, flags, NULL, buffer, &cbData)) {
        *dest = buffer;
    } else {
        free((void *)buffer);
    }
    return 0;
}


int
_combineWithInstallDir(const wchar_t **dest, const wchar_t *installDir, const wchar_t *fragment, int fragmentLength)
{
    wchar_t buffer[MAXLEN];
    wchar_t fragmentBuffer[MAXLEN];
    if (wcsncpy_s(fragmentBuffer, MAXLEN, fragment, fragmentLength)) {
        return RC_NO_MEMORY;
    }

    if (FAILED(PathCchCombineEx(buffer, MAXLEN, installDir, fragmentBuffer, PATHCCH_ALLOW_LONG_PATHS))) {
        return RC_NO_MEMORY;
    }

    return copyWstr(dest, buffer);
}


bool
_isLegacyVersion(EnvironmentInfo *env)
{
    // Check if backwards-compatibility is required.
    // Specifically PythonCore versions 2.X and 3.0 - 3.5 do not implement PEP 514.
    if (0 != _compare(env->company, -1, L"PythonCore", -1)) {
        return false;
    }

    int versionMajor, versionMinor;
    int n = swscanf_s(env->tag, L"%d.%d", &versionMajor, &versionMinor);
    if (n != 2) {
        debug(L"# %s/%s has an invalid version tag\n", env->company, env->tag);
        return false;
    }

    return versionMajor == 2
        || (versionMajor == 3 && versionMinor >= 0 && versionMinor <= 5);
}

int
_registryReadLegacyEnvironment(const SearchInfo *search, HKEY root, EnvironmentInfo *env, const wchar_t *fallbackArch)
{
    // Backwards-compatibility for PythonCore versions which do not implement PEP 514.
    int exitCode = _combineWithInstallDir(
        &env->executablePath,
        env->installDir,
        search->executable,
        search->executableLength
    );
    if (exitCode) {
        return exitCode;
    }

    if (search->windowed) {
        exitCode = _registryReadString(&env->executableArgs, root, L"InstallPath", L"WindowedExecutableArguments");
    }
    else {
        exitCode = _registryReadString(&env->executableArgs, root, L"InstallPath", L"ExecutableArguments");
    }
    if (exitCode) {
        return exitCode;
    }

    if (fallbackArch) {
        copyWstr(&env->architecture, fallbackArch);
    } else {
        DWORD binaryType;
        BOOL success = GetBinaryTypeW(env->executablePath, &binaryType);
        if (!success) {
            return RC_NO_PYTHON;
        }

        switch (binaryType) {
        case SCS_32BIT_BINARY:
            copyWstr(&env->architecture, L"32bit");
            break;
        case SCS_64BIT_BINARY:
            copyWstr(&env->architecture, L"64bit");
            break;
        default:
            return RC_NO_PYTHON;
        }
    }

    if (0 == _compare(env->architecture, -1, L"32bit", -1)) {
        size_t tagLength = wcslen(env->tag);
        if (tagLength <= 3 || 0 != _compare(&env->tag[tagLength - 3], 3, L"-32", 3)) {
            const wchar_t *rawTag = env->tag;
            wchar_t *realTag = (wchar_t*) malloc(sizeof(wchar_t) * (tagLength + 4));
            if (!realTag) {
                return RC_NO_MEMORY;
            }

            int count = swprintf_s(realTag, tagLength + 4, L"%s-32", env->tag);
            if (count == -1) {
                debug(L"# Failed to generate 32bit tag\n");
                free(realTag);
                return RC_INTERNAL_ERROR;
            }

            env->tag = realTag;
            free((void*)rawTag);
        }
    }

    wchar_t buffer[MAXLEN];
    if (swprintf_s(buffer, MAXLEN, L"Python %s", env->tag)) {
        copyWstr(&env->displayName, buffer);
    }

    return 0;
}


int
_registryReadEnvironment(const SearchInfo *search, HKEY root, EnvironmentInfo *env, const wchar_t *fallbackArch)
{
    int exitCode = _registryReadString(&env->installDir, root, L"InstallPath", NULL);
    if (exitCode) {
        return exitCode;
    }
    if (!env->installDir) {
        return RC_NO_PYTHON;
    }

    if (_isLegacyVersion(env)) {
        return _registryReadLegacyEnvironment(search, root, env, fallbackArch);
    }

    // If pythonw.exe requested, check specific value
    if (search->windowed) {
        exitCode = _registryReadString(&env->executablePath, root, L"InstallPath", L"WindowedExecutablePath");
        if (!exitCode && env->executablePath) {
            exitCode = _registryReadString(&env->executableArgs, root, L"InstallPath", L"WindowedExecutableArguments");
        }
    }
    if (exitCode) {
        return exitCode;
    }

    // Missing windowed path or non-windowed request means we use ExecutablePath
    if (!env->executablePath) {
        exitCode = _registryReadString(&env->executablePath, root, L"InstallPath", L"ExecutablePath");
        if (!exitCode && env->executablePath) {
            exitCode = _registryReadString(&env->executableArgs, root, L"InstallPath", L"ExecutableArguments");
        }
    }
    if (exitCode) {
        return exitCode;
    }

    if (!env->executablePath) {
        debug(L"# %s/%s has no executable path\n", env->company, env->tag);
        return RC_NO_PYTHON;
    }

    exitCode = _registryReadString(&env->architecture, root, NULL, L"SysArchitecture");
    if (exitCode) {
        return exitCode;
    }

    exitCode = _registryReadString(&env->displayName, root, NULL, L"DisplayName");
    if (exitCode) {
        return exitCode;
    }

    return 0;
}

int
_registrySearchTags(const SearchInfo *search, EnvironmentInfo **result, HKEY root, int sortKey, const wchar_t *company, const wchar_t *fallbackArch)
{
    wchar_t buffer[256];
    int err = 0;
    int exitCode = 0;
    for (int i = 0; exitCode == 0; ++i) {
        DWORD cchBuffer = sizeof(buffer) / sizeof(buffer[0]);
        err = RegEnumKeyExW(root, i, buffer, &cchBuffer, NULL, NULL, NULL, NULL);
        if (err) {
            if (err != ERROR_NO_MORE_ITEMS) {
                winerror(0, L"Failed to read installs (tags) from the registry");
            }
            break;
        }
        HKEY subkey;
        if (ERROR_SUCCESS == RegOpenKeyExW(root, buffer, 0, KEY_READ, &subkey)) {
            EnvironmentInfo *env = newEnvironmentInfo(company, buffer);
            env->internalSortKey = sortKey;
            exitCode = _registryReadEnvironment(search, subkey, env, fallbackArch);
            RegCloseKey(subkey);
            if (exitCode == RC_NO_PYTHON) {
                freeEnvironmentInfo(env);
                exitCode = 0;
            } else if (!exitCode) {
                exitCode = addEnvironmentInfo(result, NULL, env);
                if (exitCode) {
                    freeEnvironmentInfo(env);
                    if (exitCode == RC_DUPLICATE_ITEM) {
                        exitCode = 0;
                    }
                }
            }
        }
    }
    return exitCode;
}


int
registrySearch(const SearchInfo *search, EnvironmentInfo **result, HKEY root, int sortKey, const wchar_t *fallbackArch)
{
    wchar_t buffer[256];
    int err = 0;
    int exitCode = 0;
    for (int i = 0; exitCode == 0; ++i) {
        DWORD cchBuffer = sizeof(buffer) / sizeof(buffer[0]);
        err = RegEnumKeyExW(root, i, buffer, &cchBuffer, NULL, NULL, NULL, NULL);
        if (err) {
            if (err != ERROR_NO_MORE_ITEMS) {
                winerror(0, L"Failed to read distributors (company) from the registry");
            }
            break;
        }
        if (search->limitToCompany && 0 != _compare(search->limitToCompany, -1, buffer, cchBuffer)) {
            debug(L"# Skipping %s due to PYLAUNCHER_LIMIT_TO_COMPANY\n", buffer);
            continue;
        }
        HKEY subkey;
        if (ERROR_SUCCESS == RegOpenKeyExW(root, buffer, 0, KEY_READ, &subkey)) {
            exitCode = _registrySearchTags(search, result, subkey, sortKey, buffer, fallbackArch);
            RegCloseKey(subkey);
        }
    }
    return exitCode;
}


/******************************************************************************\
 ***                            APP PACKAGE SEARCH                          ***
\******************************************************************************/

int
appxSearch(const SearchInfo *search, EnvironmentInfo **result, const wchar_t *packageFamilyName, const wchar_t *tag, int sortKey)
{
    wchar_t realTag[32];
    wchar_t buffer[MAXLEN];
    const wchar_t *exeName = search->executable;
    if (!exeName || search->allowExecutableOverride) {
        exeName = search->windowed ? L"pythonw.exe" : L"python.exe";
    }

    // Failure to get LocalAppData may just mean we're running as a user who
    // doesn't have a profile directory.
    // In this case, return "not found", but don't fail.
    // Chances are they can't launch Store installs anyway.
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, buffer))) {
        return RC_NO_PYTHON;
    }

    if (!join(buffer, MAXLEN, L"Microsoft\\WindowsApps") ||
        !join(buffer, MAXLEN, packageFamilyName) ||
        !join(buffer, MAXLEN, exeName)) {
        debug(L"# Failed to construct App Execution Alias path\n");
        return RC_INTERNAL_ERROR;
    }

    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(buffer)) {
        return RC_NO_PYTHON;
    }

    // Assume packages are native architecture, which means we need to append
    // the '-arm64' on ARM64 host.
    wcscpy_s(realTag, 32, tag);
    if (isARM64Host()) {
        wcscat_s(realTag, 32, L"-arm64");
    }

    EnvironmentInfo *env = newEnvironmentInfo(L"PythonCore", realTag);
    if (!env) {
        return RC_NO_MEMORY;
    }
    env->internalSortKey = sortKey;
    if (isAMD64Host()) {
        copyWstr(&env->architecture, L"64bit");
    } else if (isARM64Host()) {
        copyWstr(&env->architecture, L"ARM64");
    }

    copyWstr(&env->executablePath, buffer);

    if (swprintf_s(buffer, MAXLEN, L"Python %s (Store)", tag)) {
        copyWstr(&env->displayName, buffer);
    }

    int exitCode = addEnvironmentInfo(result, NULL, env);
    if (exitCode) {
        freeEnvironmentInfo(env);
        if (exitCode == RC_DUPLICATE_ITEM) {
            exitCode = 0;
        }
    }


    return exitCode;
}


/******************************************************************************\
 ***                      OVERRIDDEN EXECUTABLE PATH                        ***
\******************************************************************************/


int
explicitOverrideSearch(const SearchInfo *search, EnvironmentInfo **result)
{
    if (!search->executablePath) {
        return 0;
    }

    EnvironmentInfo *env = newEnvironmentInfo(NULL, NULL);
    if (!env) {
        return RC_NO_MEMORY;
    }
    env->internalSortKey = 10;
    int exitCode = copyWstr(&env->executablePath, search->executablePath);
    if (exitCode) {
        goto abort;
    }
    exitCode = copyWstr(&env->displayName, L"Explicit override");
    if (exitCode) {
        goto abort;
    }
    exitCode = addEnvironmentInfo(result, NULL, env);
    if (exitCode) {
        goto abort;
    }
    return 0;

abort:
    freeEnvironmentInfo(env);
    if (exitCode == RC_DUPLICATE_ITEM) {
        exitCode = 0;
    }
    return exitCode;
}


/******************************************************************************\
 ***                   ACTIVE VIRTUAL ENVIRONMENT SEARCH                    ***
\******************************************************************************/

int
virtualenvSearch(const SearchInfo *search, EnvironmentInfo **result)
{
    int exitCode = 0;
    EnvironmentInfo *env = NULL;
    wchar_t buffer[MAXLEN];
    int n = GetEnvironmentVariableW(L"VIRTUAL_ENV", buffer, MAXLEN);
    if (!n || !join(buffer, MAXLEN, L"Scripts") || !join(buffer, MAXLEN, search->executable)) {
        return 0;
    }

    DWORD attr = GetFileAttributesW(buffer);
    if (INVALID_FILE_ATTRIBUTES == attr && search->lowPriorityTag) {
        if (!split_parent(buffer, MAXLEN) || !join(buffer, MAXLEN, L"python.exe")) {
            return 0;
        }
        attr = GetFileAttributesW(buffer);
    }

    if (INVALID_FILE_ATTRIBUTES == attr) {
        debug(L"Python executable %s missing from virtual env\n", buffer);
        return 0;
    }

    env = newEnvironmentInfo(NULL, NULL);
    if (!env) {
        return RC_NO_MEMORY;
    }
    env->highPriority = true;
    env->internalSortKey = 20;
    exitCode = copyWstr(&env->displayName, L"Active venv");
    if (exitCode) {
        goto abort;
    }
    exitCode = copyWstr(&env->executablePath, buffer);
    if (exitCode) {
        goto abort;
    }
    exitCode = addEnvironmentInfo(result, NULL, env);
    if (exitCode) {
        goto abort;
    }
    return 0;

abort:
    freeEnvironmentInfo(env);
    if (exitCode == RC_DUPLICATE_ITEM) {
        return 0;
    }
    return exitCode;
}

/******************************************************************************\
 ***                           COLLECT ENVIRONMENTS                         ***
\******************************************************************************/


struct RegistrySearchInfo {
    // Registry subkey to search
    const wchar_t *subkey;
    // Registry hive to search
    HKEY hive;
    // Flags to use when opening the subkey
    DWORD flags;
    // Internal sort key to select between "identical" environments discovered
    // through different methods
    int sortKey;
    // Fallback value to assume for PythonCore entries missing a SysArchitecture value
    const wchar_t *fallbackArch;
};


struct RegistrySearchInfo REGISTRY_SEARCH[] = {
    {
        L"Software\\Python",
        HKEY_CURRENT_USER,
        KEY_READ,
        1,
        NULL
    },
    {
        L"Software\\Python",
        HKEY_LOCAL_MACHINE,
        KEY_READ | KEY_WOW64_64KEY,
        3,
        L"64bit"
    },
    {
        L"Software\\Python",
        HKEY_LOCAL_MACHINE,
        KEY_READ | KEY_WOW64_32KEY,
        4,
        L"32bit"
    },
    { NULL, 0, 0, 0, NULL }
};


struct AppxSearchInfo {
    // The package family name. Can be found for an installed package using the
    // Powershell "Get-AppxPackage" cmdlet
    const wchar_t *familyName;
    // The tag to treat the installation as
    const wchar_t *tag;
    // Internal sort key to select between "identical" environments discovered
    // through different methods
    int sortKey;
};


struct AppxSearchInfo APPX_SEARCH[] = {
    // Releases made through the Store
    { L"PythonSoftwareFoundation.Python.3.14_qbz5n2kfra8p0", L"3.14", 10 },
    { L"PythonSoftwareFoundation.Python.3.13_qbz5n2kfra8p0", L"3.13", 10 },
    { L"PythonSoftwareFoundation.Python.3.12_qbz5n2kfra8p0", L"3.12", 10 },
    { L"PythonSoftwareFoundation.Python.3.11_qbz5n2kfra8p0", L"3.11", 10 },
    { L"PythonSoftwareFoundation.Python.3.10_qbz5n2kfra8p0", L"3.10", 10 },
    { L"PythonSoftwareFoundation.Python.3.9_qbz5n2kfra8p0", L"3.9", 10 },
    { L"PythonSoftwareFoundation.Python.3.8_qbz5n2kfra8p0", L"3.8", 10 },

    // Side-loadable releases. Note that the publisher ID changes whenever we
    // change our code signing certificate subject, so the newer IDs have higher
    // priorities (lower sortKey)
    { L"PythonSoftwareFoundation.Python.3.14_3847v3x7pw1km", L"3.14", 11 },
    { L"PythonSoftwareFoundation.Python.3.13_3847v3x7pw1km", L"3.13", 11 },
    { L"PythonSoftwareFoundation.Python.3.12_3847v3x7pw1km", L"3.12", 11 },
    { L"PythonSoftwareFoundation.Python.3.11_3847v3x7pw1km", L"3.11", 11 },
    { L"PythonSoftwareFoundation.Python.3.11_hd69rhyc2wevp", L"3.11", 12 },
    { L"PythonSoftwareFoundation.Python.3.10_3847v3x7pw1km", L"3.10", 11 },
    { L"PythonSoftwareFoundation.Python.3.10_hd69rhyc2wevp", L"3.10", 12 },
    { L"PythonSoftwareFoundation.Python.3.9_3847v3x7pw1km", L"3.9", 11 },
    { L"PythonSoftwareFoundation.Python.3.9_hd69rhyc2wevp", L"3.9", 12 },
    { L"PythonSoftwareFoundation.Python.3.8_hd69rhyc2wevp", L"3.8", 12 },
    { NULL, NULL, 0 }
};


int
collectEnvironments(const SearchInfo *search, EnvironmentInfo **result)
{
    int exitCode = 0;
    HKEY root;
    EnvironmentInfo *env = NULL;

    if (!result) {
        debug(L"# collectEnvironments() was passed a NULL result\n");
        return RC_INTERNAL_ERROR;
    }
    *result = NULL;

    exitCode = explicitOverrideSearch(search, result);
    if (exitCode) {
        return exitCode;
    }

    exitCode = virtualenvSearch(search, result);
    if (exitCode) {
        return exitCode;
    }

    // If we aren't collecting all items to list them, we can exit now.
    if (env && !(search->list || search->listPaths)) {
        return 0;
    }

    for (struct RegistrySearchInfo *info = REGISTRY_SEARCH; info->subkey; ++info) {
        if (ERROR_SUCCESS == RegOpenKeyExW(info->hive, info->subkey, 0, info->flags, &root)) {
            exitCode = registrySearch(search, result, root, info->sortKey, info->fallbackArch);
            RegCloseKey(root);
        }
        if (exitCode) {
            return exitCode;
        }
    }

    if (search->limitToCompany) {
        debug(L"# Skipping APPX search due to PYLAUNCHER_LIMIT_TO_COMPANY\n");
        return 0;
    }

    for (struct AppxSearchInfo *info = APPX_SEARCH; info->familyName; ++info) {
        exitCode = appxSearch(search, result, info->familyName, info->tag, info->sortKey);
        if (exitCode && exitCode != RC_NO_PYTHON) {
            return exitCode;
        }
    }

    return 0;
}


/******************************************************************************\
 ***                           INSTALL ON DEMAND                            ***
\******************************************************************************/

struct StoreSearchInfo {
    // The tag a user is looking for
    const wchar_t *tag;
    // The Store ID for a package if it can be installed from the Microsoft
    // Store. These are obtained from the dashboard at
    // https://partner.microsoft.com/dashboard
    const wchar_t *storeId;
};


struct StoreSearchInfo STORE_SEARCH[] = {
    { L"3", /* 3.13 */ L"9PNRBTZXMB4Z" },
    { L"3.14", L"9NTRHQCBBPR8" },
    { L"3.13", L"9PNRBTZXMB4Z" },
    { L"3.12", L"9NCVDN91XZQP" },
    { L"3.11", L"9NRWMJP3717K" },
    { L"3.10", L"9PJPW5LDXLZ5" },
    { L"3.9", L"9P7QFQMJRFP7" },
    { L"3.8", L"9MSSZTT1N39L" },
    { NULL, NULL }
};


int
_installEnvironment(const wchar_t *command, const wchar_t *arguments)
{
    SHELLEXECUTEINFOW siw = {
        sizeof(SHELLEXECUTEINFOW),
        SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE,
        NULL, NULL,
        command, arguments, NULL,
        SW_SHOWNORMAL
    };

    debug(L"# Installing with %s %s\n", command, arguments);
    if (isEnvVarSet(L"PYLAUNCHER_DRYRUN")) {
        debug(L"# Exiting due to PYLAUNCHER_DRYRUN\n");
        fflush(stdout);
        int mode = _setmode(_fileno(stdout), _O_U8TEXT);
        if (arguments) {
            fwprintf_s(stdout, L"\"%s\" %s\n", command, arguments);
        } else {
            fwprintf_s(stdout, L"\"%s\"\n", command);
        }
        fflush(stdout);
        if (mode >= 0) {
            _setmode(_fileno(stdout), mode);
        }
        return RC_INSTALLING;
    }

    if (!ShellExecuteExW(&siw)) {
        return RC_NO_PYTHON;
    }

    if (!siw.hProcess) {
        return RC_INSTALLING;
    }

    WaitForSingleObjectEx(siw.hProcess, INFINITE, FALSE);
    DWORD exitCode = 0;
    if (GetExitCodeProcess(siw.hProcess, &exitCode) && exitCode == 0) {
        return 0;
    }
    return RC_INSTALLING;
}


const wchar_t *WINGET_COMMAND = L"Microsoft\\WindowsApps\\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\\winget.exe";
const wchar_t *WINGET_ARGUMENTS = L"install -q %s --exact --accept-package-agreements --source msstore";

const wchar_t *MSSTORE_COMMAND = L"ms-windows-store://pdp/?productid=%s";

int
installEnvironment(const SearchInfo *search)
{
    // No tag? No installing
    if (!search->tag || !search->tagLength) {
        debug(L"# Cannot install Python with no tag specified\n");
        return RC_NO_PYTHON;
    }

    // PEP 514 tag but not PythonCore? No installing
    if (!search->oldStyleTag &&
        search->company && search->companyLength &&
        0 != _compare(search->company, search->companyLength, L"PythonCore", -1)) {
        debug(L"# Cannot install for company %.*s\n", search->companyLength, search->company);
        return RC_NO_PYTHON;
    }

    const wchar_t *storeId = NULL;
    for (struct StoreSearchInfo *info = STORE_SEARCH; info->tag; ++info) {
        if (0 == _compare(search->tag, search->tagLength, info->tag, -1)) {
            storeId = info->storeId;
            break;
        }
    }

    if (!storeId) {
        return RC_NO_PYTHON;
    }

    int exitCode;
    wchar_t command[MAXLEN];
    wchar_t arguments[MAXLEN];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, command)) &&
        join(command, MAXLEN, WINGET_COMMAND) &&
        swprintf_s(arguments, MAXLEN, WINGET_ARGUMENTS, storeId)) {
        if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(command)) {
            formatWinerror(GetLastError(), arguments, MAXLEN);
            debug(L"# Skipping %s: %s\n", command, arguments);
        } else {
            fputws(L"Launching winget to install Python. The following output is from the install process\n\
***********************************************************************\n", stdout);
            exitCode = _installEnvironment(command, arguments);
            if (exitCode == RC_INSTALLING) {
                fputws(L"***********************************************************************\n\
Please check the install status and run your command again.", stderr);
                return exitCode;
            } else if (exitCode) {
                return exitCode;
            }
            fputws(L"***********************************************************************\n\
Install appears to have succeeded. Searching for new matching installs.\n", stdout);
            return 0;
        }
    }

    if (swprintf_s(command, MAXLEN, MSSTORE_COMMAND, storeId)) {
        fputws(L"Opening the Microsoft Store to install Python. After installation, "
               L"please run your command again.\n", stderr);
        exitCode = _installEnvironment(command, NULL);
        if (exitCode) {
            return exitCode;
        }
        return 0;
    }

    return RC_NO_PYTHON;
}

/******************************************************************************\
 ***                           ENVIRONMENT SELECT                           ***
\******************************************************************************/

bool
_companyMatches(const SearchInfo *search, const EnvironmentInfo *env)
{
    if (!search->company || !search->companyLength) {
        return true;
    }
    return 0 == _compare(env->company, -1, search->company, search->companyLength);
}


bool
_tagMatches(const SearchInfo *search, const EnvironmentInfo *env, int searchTagLength)
{
    if (searchTagLength < 0) {
        searchTagLength = search->tagLength;
    }
    if (!search->tag || !searchTagLength) {
        return true;
    }
    return _startsWithSeparated(env->tag, -1, search->tag, searchTagLength, L".-");
}


bool
_is32Bit(const EnvironmentInfo *env)
{
    if (env->architecture) {
        return 0 == _compare(env->architecture, -1, L"32bit", -1);
    }
    return false;
}


int
_selectEnvironment(const SearchInfo *search, EnvironmentInfo *env, EnvironmentInfo **best)
{
    int exitCode = 0;
    while (env) {
        exitCode = _selectEnvironment(search, env->prev, best);

        if (exitCode && exitCode != RC_NO_PYTHON) {
            return exitCode;
        } else if (!exitCode && *best) {
            return 0;
        }

        if (env->highPriority && search->lowPriorityTag) {
            // This environment is marked high priority, and the search allows
            // it to be selected even though a tag is specified, so select it
            // gh-92817: this allows an active venv to be selected even when a
            // default tag has been found in py.ini or the environment
            *best = env;
            return 0;
        }

        if (!search->oldStyleTag) {
            if (_companyMatches(search, env) && _tagMatches(search, env, -1)) {
                // Because of how our sort tree is set up, we will walk up the
                // "prev" side and implicitly select the "best" best. By
                // returning straight after a match, we skip the entire "next"
                // branch and won't ever select a "worse" best.
                *best = env;
                return 0;
            }
        } else if (0 == _compare(env->company, -1, L"PythonCore", -1)) {
            // Old-style tags can only match PythonCore entries

            // If the tag ends with -64, we want to exclude 32-bit runtimes
            // (If the tag ends with -32, it will be filtered later)
            int tagLength = search->tagLength;
            bool exclude32Bit = false, only32Bit = false;
            if (tagLength > 3) {
                if (0 == _compareArgument(&search->tag[tagLength - 3], 3, L"-64", 3)) {
                    tagLength -= 3;
                    exclude32Bit = true;
                } else if (0 == _compareArgument(&search->tag[tagLength - 3], 3, L"-32", 3)) {
                    tagLength -= 3;
                    only32Bit = true;
                }
            }

            if (_tagMatches(search, env, tagLength)) {
                if (exclude32Bit && _is32Bit(env)) {
                    debug(L"# Excluding %s/%s because it looks like 32bit\n", env->company, env->tag);
                } else if (only32Bit && !_is32Bit(env)) {
                    debug(L"# Excluding %s/%s because it doesn't look 32bit\n", env->company, env->tag);
                } else {
                    *best = env;
                    return 0;
                }
            }
        }

        env = env->next;
    }
    return RC_NO_PYTHON;
}

int
selectEnvironment(const SearchInfo *search, EnvironmentInfo *root, EnvironmentInfo **best)
{
    if (!best) {
        debug(L"# selectEnvironment() was passed a NULL best\n");
        return RC_INTERNAL_ERROR;
    }
    if (!root) {
        *best = NULL;
        return RC_NO_PYTHON_AT_ALL;
    }

    EnvironmentInfo *result = NULL;
    int exitCode = _selectEnvironment(search, root, &result);
    if (!exitCode) {
        *best = result;
    }

    return exitCode;
}


/******************************************************************************\
 ***                            LIST ENVIRONMENTS                           ***
\******************************************************************************/

#define TAGWIDTH 16

int
_printEnvironment(const EnvironmentInfo *env, FILE *out, bool showPath, const wchar_t *argument)
{
    if (showPath) {
        if (env->executablePath && env->executablePath[0]) {
            if (env->executableArgs && env->executableArgs[0]) {
                fwprintf(out, L" %-*s %s %s\n", TAGWIDTH, argument, env->executablePath, env->executableArgs);
            } else {
                fwprintf(out, L" %-*s %s\n", TAGWIDTH, argument, env->executablePath);
            }
        } else if (env->installDir && env->installDir[0]) {
            fwprintf(out, L" %-*s %s\n", TAGWIDTH, argument, env->installDir);
        } else {
            fwprintf(out, L" %s\n", argument);
        }
    } else if (env->displayName) {
        fwprintf(out, L" %-*s %s\n", TAGWIDTH, argument, env->displayName);
    } else {
        fwprintf(out, L" %s\n", argument);
    }
    return 0;
}


int
_listAllEnvironments(EnvironmentInfo *env, FILE * out, bool showPath, EnvironmentInfo *defaultEnv)
{
    wchar_t buffer[256];
    const int bufferSize = 256;
    while (env) {
        int exitCode = _listAllEnvironments(env->prev, out, showPath, defaultEnv);
        if (exitCode) {
            return exitCode;
        }

        if (!env->company || !env->tag) {
            buffer[0] = L'\0';
        } else if (0 == _compare(env->company, -1, L"PythonCore", -1)) {
            swprintf_s(buffer, bufferSize, L"-V:%s", env->tag);
        } else {
            swprintf_s(buffer, bufferSize, L"-V:%s/%s", env->company, env->tag);
        }

        if (env == defaultEnv) {
            wcscat_s(buffer, bufferSize, L" *");
        }

        if (buffer[0]) {
            exitCode = _printEnvironment(env, out, showPath, buffer);
            if (exitCode) {
                return exitCode;
            }
        }

        env = env->next;
    }
    return 0;
}


int
listEnvironments(EnvironmentInfo *env, FILE * out, bool showPath, EnvironmentInfo *defaultEnv)
{
    if (!env) {
        fwprintf_s(stdout, L"No installed Pythons found!\n");
        return 0;
    }

    /* TODO: Do we want to display these?
       In favour, helps users see that '-3' is a good option
       Against, repeats the next line of output
    SearchInfo majorSearch;
    EnvironmentInfo *major;
    int exitCode;

    if (showPath) {
        memset(&majorSearch, 0, sizeof(majorSearch));
        majorSearch.company = L"PythonCore";
        majorSearch.companyLength = -1;
        majorSearch.tag = L"3";
        majorSearch.tagLength = -1;
        majorSearch.oldStyleTag = true;
        major = NULL;
        exitCode = selectEnvironment(&majorSearch, env, &major);
        if (!exitCode && major) {
            exitCode = _printEnvironment(major, out, showPath, L"-3 *");
            isDefault = false;
            if (exitCode) {
                return exitCode;
            }
        }
        majorSearch.tag = L"2";
        major = NULL;
        exitCode = selectEnvironment(&majorSearch, env, &major);
        if (!exitCode && major) {
            exitCode = _printEnvironment(major, out, showPath, L"-2");
            if (exitCode) {
                return exitCode;
            }
        }
    }
    */

    int mode = _setmode(_fileno(out), _O_U8TEXT);
    int exitCode = _listAllEnvironments(env, out, showPath, defaultEnv);
    fflush(out);
    if (mode >= 0) {
        _setmode(_fileno(out), mode);
    }
    return exitCode;
}


/******************************************************************************\
 ***                           INTERPRETER LAUNCH                           ***
\******************************************************************************/


int
calculateCommandLine(const SearchInfo *search, const EnvironmentInfo *launch, wchar_t *buffer, int bufferLength)
{
    int exitCode = 0;
    const wchar_t *executablePath = NULL;

    // Construct command line from a search override, or else the selected
    // environment's executablePath
    if (search->executablePath) {
        executablePath = search->executablePath;
    } else if (launch && launch->executablePath) {
        executablePath = launch->executablePath;
    }

    // If we have an executable path, put it at the start of the command, but
    // only if the search allowed an override.
    // Otherwise, use the environment's installDir and the search's default
    // executable name.
    if (executablePath && search->allowExecutableOverride) {
        if (wcschr(executablePath, L' ') && executablePath[0] != L'"') {
            buffer[0] = L'"';
            exitCode = wcscpy_s(&buffer[1], bufferLength - 1, executablePath);
            if (!exitCode) {
                exitCode = wcscat_s(buffer, bufferLength, L"\"");
            }
        } else {
            exitCode = wcscpy_s(buffer, bufferLength, executablePath);
        }
    } else if (launch) {
        if (!launch->installDir) {
            fwprintf_s(stderr, L"Cannot launch %s %s because no install directory was specified",
                       launch->company, launch->tag);
            exitCode = RC_NO_PYTHON;
        } else if (!search->executable || !search->executableLength) {
            fwprintf_s(stderr, L"Cannot launch %s %s because no executable name is available",
                       launch->company, launch->tag);
            exitCode = RC_NO_PYTHON;
        } else {
            wchar_t executable[256];
            wcsncpy_s(executable, 256, search->executable, search->executableLength);
            if ((wcschr(launch->installDir, L' ') && launch->installDir[0] != L'"') ||
                (wcschr(executable, L' ') && executable[0] != L'"')) {
                buffer[0] = L'"';
                exitCode = wcscpy_s(&buffer[1], bufferLength - 1, launch->installDir);
                if (!exitCode) {
                    exitCode = join(buffer, bufferLength, executable) ? 0 : RC_NO_MEMORY;
                }
                if (!exitCode) {
                    exitCode = wcscat_s(buffer, bufferLength, L"\"");
                }
            } else {
                exitCode = wcscpy_s(buffer, bufferLength, launch->installDir);
                if (!exitCode) {
                    exitCode = join(buffer, bufferLength, executable) ? 0 : RC_NO_MEMORY;
                }
            }
        }
    } else {
        exitCode = RC_NO_PYTHON;
    }

    if (!exitCode && launch && launch->executableArgs) {
        exitCode = wcscat_s(buffer, bufferLength, L" ");
        if (!exitCode) {
            exitCode = wcscat_s(buffer, bufferLength, launch->executableArgs);
        }
    }

    if (!exitCode && search->executableArgs) {
        if (search->executableArgsLength < 0) {
            exitCode = wcscat_s(buffer, bufferLength, search->executableArgs);
        } else if (search->executableArgsLength > 0) {
            int end = (int)wcsnlen_s(buffer, MAXLEN);
            if (end < bufferLength - (search->executableArgsLength + 1)) {
                exitCode = wcsncpy_s(&buffer[end], bufferLength - end,
                    search->executableArgs, search->executableArgsLength);
            }
        }
    }

    if (!exitCode && search->restOfCmdLine) {
        exitCode = wcscat_s(buffer, bufferLength, search->restOfCmdLine);
    }

    return exitCode;
}



BOOL
_safeDuplicateHandle(HANDLE in, HANDLE * pout, const wchar_t *nameForError)
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
            winerror(0, L"Failed to duplicate %s handle", nameForError);
        }
    }
    return ok;
}

BOOL WINAPI
ctrl_c_handler(DWORD code)
{
    return TRUE;    /* We just ignore all control events. */
}


int
launchEnvironment(const SearchInfo *search, const EnvironmentInfo *launch, wchar_t *launchCommand)
{
    HANDLE job;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
    DWORD rc;
    BOOL ok;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    // If this is a dryrun, do not actually launch
    if (isEnvVarSet(L"PYLAUNCHER_DRYRUN")) {
        debug(L"LaunchCommand: %s\n", launchCommand);
        debug(L"# Exiting due to PYLAUNCHER_DRYRUN variable\n");
        fflush(stdout);
        int mode = _setmode(_fileno(stdout), _O_U8TEXT);
        fwprintf(stdout, L"%s\n", launchCommand);
        fflush(stdout);
        if (mode >= 0) {
            _setmode(_fileno(stdout), mode);
        }
        return 0;
    }

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

    debug(L"# about to run: %s\n", launchCommand);
    job = CreateJobObject(NULL, NULL);
    ok = QueryInformationJobObject(job, JobObjectExtendedLimitInformation,
                                  &info, sizeof(info), &rc);
    if (!ok || (rc != sizeof(info)) || !job) {
        winerror(0, L"Failed to query job information");
        return RC_CREATE_PROCESS;
    }
    info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
                                             JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
    ok = SetInformationJobObject(job, JobObjectExtendedLimitInformation, &info,
                                 sizeof(info));
    if (!ok) {
        winerror(0, L"Failed to update job information");
        return RC_CREATE_PROCESS;
    }
    memset(&si, 0, sizeof(si));
    GetStartupInfoW(&si);
    if (!_safeDuplicateHandle(GetStdHandle(STD_INPUT_HANDLE), &si.hStdInput, L"stdin") ||
        !_safeDuplicateHandle(GetStdHandle(STD_OUTPUT_HANDLE), &si.hStdOutput, L"stdout") ||
        !_safeDuplicateHandle(GetStdHandle(STD_ERROR_HANDLE), &si.hStdError, L"stderr")) {
        return RC_NO_STD_HANDLES;
    }

    ok = SetConsoleCtrlHandler(ctrl_c_handler, TRUE);
    if (!ok) {
        winerror(0, L"Failed to update Control-C handler");
        return RC_NO_STD_HANDLES;
    }

    si.dwFlags = STARTF_USESTDHANDLES;
    ok = CreateProcessW(NULL, launchCommand, NULL, NULL, TRUE,
                        0, NULL, NULL, &si, &pi);
    if (!ok) {
        winerror(0, L"Unable to create process using '%s'", launchCommand);
        return RC_CREATE_PROCESS;
    }
    AssignProcessToJobObject(job, pi.hProcess);
    CloseHandle(pi.hThread);
    WaitForSingleObjectEx(pi.hProcess, INFINITE, FALSE);
    ok = GetExitCodeProcess(pi.hProcess, &rc);
    if (!ok) {
        winerror(0, L"Failed to get exit code of process");
        return RC_CREATE_PROCESS;
    }
    debug(L"child process exit code: %d\n", rc);
    return rc;
}


/******************************************************************************\
 ***                           PROCESS CONTROLLER                           ***
\******************************************************************************/


int
performSearch(SearchInfo *search, EnvironmentInfo **envs)
{
    // First parse the command line for options
    int exitCode = parseCommandLine(search);
    if (exitCode) {
        return exitCode;
    }

    // Check for a shebang line in our script file
    // (or return quickly if no script file was specified)
    exitCode = checkShebang(search);
    switch (exitCode) {
    case 0:
    case RC_NO_SHEBANG:
    case RC_RECURSIVE_SHEBANG:
        break;
    default:
        return exitCode;
    }

    // Resolve old-style tags (possibly from a shebang) against py.ini entries
    // and environment variables.
    exitCode = checkDefaults(search);
    if (exitCode) {
        return exitCode;
    }

    // If debugging is enabled, list our search criteria
    dumpSearchInfo(search);

    // Find all matching environments
    exitCode = collectEnvironments(search, envs);
    if (exitCode) {
        return exitCode;
    }

    return 0;
}


int
process(int argc, wchar_t ** argv)
{
    int exitCode = 0;
    int searchExitCode = 0;
    SearchInfo search = {0};
    EnvironmentInfo *envs = NULL;
    EnvironmentInfo *env = NULL;
    wchar_t launchCommand[MAXLEN];

    memset(launchCommand, 0, sizeof(launchCommand));

    if (isEnvVarSet(L"PYLAUNCHER_DEBUG")) {
        setvbuf(stderr, (char *)NULL, _IONBF, 0);
        log_fp = stderr;
        debug(L"argv0: %s\nversion: %S\n", argv[0], PY_VERSION);
    }

    DWORD len = GetEnvironmentVariableW(L"PYLAUNCHER_LIMIT_TO_COMPANY", NULL, 0);
    if (len > 1) {
        wchar_t *limitToCompany = allocSearchInfoBuffer(&search, len);
        if (!limitToCompany) {
            exitCode = RC_NO_MEMORY;
            winerror(0, L"Failed to allocate internal buffer");
            goto abort;
        }
        search.limitToCompany = limitToCompany;
        if (0 == GetEnvironmentVariableW(L"PYLAUNCHER_LIMIT_TO_COMPANY", limitToCompany, len)) {
            exitCode = RC_INTERNAL_ERROR;
            winerror(0, L"Failed to read PYLAUNCHER_LIMIT_TO_COMPANY variable");
            goto abort;
        }
    }

    search.originalCmdLine = GetCommandLineW();

    exitCode = performSearch(&search, &envs);
    if (exitCode) {
        goto abort;
    }

    // Display the help text, but only exit on error
    if (search.help) {
        exitCode = showHelpText(argv);
        if (exitCode) {
            goto abort;
        }
    }

    // Select best environment
    // This is early so that we can show the default when listing, but all
    // responses to any errors occur later.
    searchExitCode = selectEnvironment(&search, envs, &env);

    // List all environments, then exit
    if (search.list || search.listPaths) {
        exitCode = listEnvironments(envs, stdout, search.listPaths, env);
        goto abort;
    }

    // When debugging, list all discovered environments anyway
    if (log_fp) {
        exitCode = listEnvironments(envs, log_fp, true, NULL);
        if (exitCode) {
            goto abort;
        }
    }

    // We searched earlier, so if we didn't find anything, now we react
    exitCode = searchExitCode;
    // If none found, and if permitted, install it
    if (exitCode == RC_NO_PYTHON && isEnvVarSet(L"PYLAUNCHER_ALLOW_INSTALL") ||
        isEnvVarSet(L"PYLAUNCHER_ALWAYS_INSTALL")) {
        exitCode = installEnvironment(&search);
        if (!exitCode) {
            // Successful install, so we need to re-scan and select again
            env = NULL;
            exitCode = performSearch(&search, &envs);
            if (exitCode) {
                goto abort;
            }
            exitCode = selectEnvironment(&search, envs, &env);
        }
    }
    if (exitCode == RC_NO_PYTHON) {
        fputws(L"No suitable Python runtime found\n", stderr);
        fputws(L"Pass --list (-0) to see all detected environments on your machine\n", stderr);
        if (!isEnvVarSet(L"PYLAUNCHER_ALLOW_INSTALL") && search.oldStyleTag) {
            fputws(L"or set environment variable PYLAUNCHER_ALLOW_INSTALL to use winget\n"
                   L"or open the Microsoft Store to the requested version.\n", stderr);
        }
        goto abort;
    }
    if (exitCode == RC_NO_PYTHON_AT_ALL) {
        fputws(L"No installed Python found!\n", stderr);
        goto abort;
    }
    if (exitCode) {
        goto abort;
    }

    if (env) {
        debug(L"env.company: %s\nenv.tag: %s\n", env->company, env->tag);
    } else {
        debug(L"env.company: (null)\nenv.tag: (null)\n");
    }

    exitCode = calculateCommandLine(&search, env, launchCommand, sizeof(launchCommand) / sizeof(launchCommand[0]));
    if (exitCode) {
        goto abort;
    }

    // Launch selected runtime
    exitCode = launchEnvironment(&search, env, launchCommand);

abort:
    freeSearchInfo(&search);
    freeEnvironmentInfo(envs);
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
