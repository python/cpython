#ifndef Py_INTERNAL_FILEUTILS_WINDOWS_H
#define Py_INTERNAL_FILEUTILS_WINDOWS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "Py_BUILD_CORE must be defined to include this header"
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
        HMODULE hMod = LoadLibraryW(L"api-ms-win-core-file-l2-1-4");
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

#endif

#endif
