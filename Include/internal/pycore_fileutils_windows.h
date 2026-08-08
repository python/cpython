#ifndef Py_INTERNAL_FILEUTILS_WINDOWS_H
#define Py_INTERNAL_FILEUTILS_WINDOWS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef MS_WINDOWS

#if !defined(NTDDI_WIN10_NI) || !(NTDDI_VERSION >= NTDDI_WIN10_NI)
typedef struct _FILE_STAT_BASIC_INFORMATION {
    LARGE_INTEGER FileId;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
    ULONG ReparseTag;
    ULONG NumberOfLinks;
    ULONG DeviceType;
    ULONG DeviceCharacteristics;
    ULONG Reserved;
    LARGE_INTEGER VolumeSerialNumber;
    FILE_ID_128 FileId128;
} FILE_STAT_BASIC_INFORMATION;

typedef enum _FILE_INFO_BY_NAME_CLASS {
    FileStatByNameInfo,
    FileStatLxByNameInfo,
    FileCaseSensitiveByNameInfo,
    FileStatBasicByNameInfo,
    MaximumFileInfoByNameClass
} FILE_INFO_BY_NAME_CLASS;
#endif

typedef BOOL (WINAPI *PGetFileInformationByName)(
    PCWSTR FileName,
    FILE_INFO_BY_NAME_CLASS FileInformationClass,
    PVOID FileInfoBuffer,
    ULONG FileInfoBufferSize
);

static inline BOOL _Py_GetFileInformationByName(
    PCWSTR FileName,
    FILE_INFO_BY_NAME_CLASS FileInformationClass,
    PVOID FileInfoBuffer,
    ULONG FileInfoBufferSize
) {
    static PGetFileInformationByName GetFileInformationByName = NULL;
    static int GetFileInformationByName_init = -1;

    if (GetFileInformationByName_init < 0) {
#ifdef MS_WINDOWS_DESKTOP
        HMODULE hMod = LoadLibraryW(L"api-ms-win-core-file-l2-1-4");
#else
        HMODULE hMod = LoadPackagedLibrary(L"api-ms-win-core-file-l2-1-4", 0);
#endif
        GetFileInformationByName_init = 0;
        if (hMod) {
            GetFileInformationByName = (PGetFileInformationByName)GetProcAddress(
                hMod, "GetFileInformationByName");
            if (GetFileInformationByName) {
                GetFileInformationByName_init = 1;
            } else {
                FreeLibrary(hMod);
            }
        }
    }

    if (GetFileInformationByName_init <= 0) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }
    return GetFileInformationByName(FileName, FileInformationClass, FileInfoBuffer, FileInfoBufferSize);
}

static inline BOOL _Py_GetFileInformationByName_ErrorIsTrustworthy(int error)
{
    switch(error) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_NOT_READY:
        case ERROR_BAD_NET_NAME:
        case ERROR_BAD_NETPATH:
        case ERROR_BAD_PATHNAME:
        case ERROR_INVALID_NAME:
        case ERROR_FILENAME_EXCED_RANGE:
            return TRUE;
        case ERROR_NOT_SUPPORTED:
            return FALSE;
    }
    return FALSE;
}

// Convert absolute paths to relative paths for paths within the installation path
static const wchar_t* _Py_AbsolutePath_To_RelativePath(const wchar_t* abs_path)
{
    const wchar_t* rel_path = NULL;
    wchar_t process_path[512] = { 0 };
    if (GetModuleFileNameW(NULL, process_path, 512))
    {
        wchar_t* path = wcsrchr(process_path, L'\\');
        path[1] = L'\0'; // strip process name
        if (wcsstr(abs_path, process_path))
            rel_path = &abs_path[wcslen(process_path)];
    }
    return rel_path;
}

static inline HANDLE _Py_WinCreateFile(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
)
{
#ifndef MS_WINDOWS_DESKTOP
    if (dwShareMode == 0)
        dwShareMode = FILE_SHARE_READ;

    CREATEFILE2_EXTENDED_PARAMETERS ext;
    ZeroMemory(&ext, sizeof(CREATEFILE2_EXTENDED_PARAMETERS));
    ext.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
    ext.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    ext.dwFileFlags = dwFlagsAndAttributes & 0xFFFF0000;

    // UWP does not allow absolute paths due security restrictions and only has access to 'AppData'
    // folder outside of the install path.
    // If path is contained inside install path (sub folder), use the relative path instead.
    if (wcschr(lpFileName, L':')) {
        const wchar_t* rel_path = _Py_AbsolutePath_To_RelativePath(lpFileName);
        if (rel_path) {
            return CreateFile2(rel_path, dwDesiredAccess, dwShareMode, dwCreationDisposition, &ext);
        }
    }

    return CreateFile2(lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, &ext);
#else
    return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                       dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
#endif
}

#endif

#endif
