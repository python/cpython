#ifndef Py_INTERNAL_FILEUTILS_WINDOWS_H
#define Py_INTERNAL_FILEUTILS_WINDOWS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef MS_WINDOWS
#include <winternl.h>

#pragma region NTSTATUS
/* Unfortunately, we cannot include <ntstatus.h> due to many includes of <Windows.h> (macro redefinition conflicts). */
/* The proper solution would be to have a single WindowsHeaders.h header which uses WIN32_NO_STATUS. */

/* MessageId: STATUS_SUCCESS */
/* MessageText: */
/* STATUS_SUCCESS */
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)    // ntsubauth

/* MessageId: STATUS_BUFFER_OVERFLOW */
/* MessageText: */
/* The data was too large to fit into the specified buffer. */
#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)

/* MessageId: STATUS_NO_MORE_FILES */
/* MessageText: */
/* No more files were found which match the file specification. */
#define STATUS_NO_MORE_FILES             ((NTSTATUS)0x80000006L)

/* MessageId: STATUS_INVALID_INFO_CLASS */
/* MessageText: */
/* The specified information class is not a valid information class for the specified object. */
#define STATUS_INVALID_INFO_CLASS        ((NTSTATUS)0xC0000003L)    // ntsubauth

/* MessageId: STATUS_INFO_LENGTH_MISMATCH */
/* MessageText: */
/* The specified information record length does not match the length required for the specified information class. */
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)

/* MessageId: STATUS_BUFFER_TOO_SMALL */
/* MessageText: */
/* The buffer is too small to contain the entry. No information has been written to the buffer. */
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)

/* MessageId: STATUS_NOT_SUPPORTED */
/* MessageText: */
/* The request is not supported. */
#define STATUS_NOT_SUPPORTED             ((NTSTATUS)0xC00000BBL)

#pragma endregion

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

#if !defined(FILE_DIRECTORY_INFORMATION)
typedef struct _FILE_DIRECTORY_INFORMATION {
  ULONG         NextEntryOffset;
  ULONG         FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG         FileAttributes;
  ULONG         FileNameLength;
  WCHAR         FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;
#endif

/* Initial allocation size - includes buffer memory for FileName */
#define INITIAL_FILE_DIRECTORY_INFORMATION_ENTRY_SIE ((ULONG)(sizeof(FILE_DIRECTORY_INFORMATION) + MAX_PATH))
/* Paranoid overflow check. */
_STATIC_ASSERT(INITIAL_FILE_DIRECTORY_INFORMATION_ENTRY_SIE > MAX_PATH && INITIAL_FILE_DIRECTORY_INFORMATION_ENTRY_SIE > sizeof(FILE_DIRECTORY_INFORMATION));

typedef BOOL (WINAPI *PGetFileInformationByName)(
    PCWSTR FileName,
    FILE_INFO_BY_NAME_CLASS FileInformationClass,
    PVOID FileInfoBuffer,
    ULONG FileInfoBufferSize
);

typedef NTSTATUS (NTAPI *PNtCreateFile)(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength
);

typedef NTSTATUS (NTAPI *PNtQueryDirectoryFile)(
    HANDLE                 FileHandle,
    HANDLE                 Event,
    PIO_APC_ROUTINE        ApcRoutine,
    PVOID                  ApcContext,
    PIO_STATUS_BLOCK       IoStatusBlock,
    PVOID                  FileInformation,
    ULONG                  Length,
    FILE_INFORMATION_CLASS FileInformationClass,
    BOOLEAN                ReturnSingleEntry,
    PUNICODE_STRING        FileName,
    BOOLEAN                RestartScan
);

typedef NTSTATUS (NTAPI *PNtClose)(
    HANDLE Handle
);

typedef ULONG (NTAPI *PRtlNtStatusToDosError)(
    NTSTATUS Status
);

typedef void (*PRtlInitUnicodeString)(
    PUNICODE_STRING DestinationString,
    PCWSTR          SourceString
);

typedef void (*PRtlFreeUnicodeString)(
    PUNICODE_STRING UnicodeString
);

typedef NTSTATUS (*PRtlDosPathNameToNtPathName_U_WithStatus)(
    PCWSTR DosFileName,
    PUNICODE_STRING NtFileName,
    PWSTR *FilePart,
    PVOID Reserved
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

static inline NTSTATUS _Py_NtCreateFile(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength
) {
    static PNtCreateFile NtCreateFile = NULL;
    static int NtCreateFile_init = -1;

    if (NtCreateFile_init < 0) {
        HMODULE hMod = LoadLibraryW(L"ntdll.dll");
        NtCreateFile_init = 0;
        if (hMod) {
            NtCreateFile = (PNtCreateFile)GetProcAddress(
                hMod, "NtCreateFile");
            if (NtCreateFile) {
                NtCreateFile_init = 1;
            } else {
                FreeLibrary(hMod);
            }
        }
    }

    if (NtCreateFile_init <= 0) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return STATUS_NOT_SUPPORTED;
    }
    return NtCreateFile(
        FileHandle,
        DesiredAccess,
        ObjectAttributes,
        IoStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        EaBuffer,
        EaLength
    );
}

static inline NTSTATUS _Py_NtQueryDirectoryFile(
    HANDLE                 FileHandle,
    HANDLE                 Event,
    PIO_APC_ROUTINE        ApcRoutine,
    PVOID                  ApcContext,
    PIO_STATUS_BLOCK       IoStatusBlock,
    PVOID                  FileInformation,
    ULONG                  Length,
    FILE_INFORMATION_CLASS FileInformationClass,
    BOOLEAN                ReturnSingleEntry,
    PUNICODE_STRING        FileName,
    BOOLEAN                RestartScan
) {
    static PNtQueryDirectoryFile NtQueryDirectoryFile = NULL;
    static int NtQueryDirectoryFile_init = -1;

    if (NtQueryDirectoryFile_init < 0) {
        HMODULE hMod = LoadLibraryW(L"ntdll.dll");
        NtQueryDirectoryFile_init = 0;
        if (hMod) {
            NtQueryDirectoryFile = (PNtQueryDirectoryFile)GetProcAddress(
                hMod, "NtQueryDirectoryFile");
            if (NtQueryDirectoryFile) {
                NtQueryDirectoryFile_init = 1;
            } else {
                FreeLibrary(hMod);
            }
        }
    }

    if (NtQueryDirectoryFile_init <= 0) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return STATUS_NOT_SUPPORTED;
    }
    return NtQueryDirectoryFile(
        FileHandle,
        Event,
        ApcRoutine,
        ApcContext,
        IoStatusBlock,
        FileInformation,
        Length,
        FileInformationClass,
        ReturnSingleEntry,
        FileName,
        RestartScan
    );
}

static inline NTSTATUS _Py_NtClose(
    HANDLE Handle
) {
    static PNtClose NtClose = NULL;
    static int NtClose_init = -1;

    if (NtClose_init < 0) {
        HMODULE hMod = LoadLibraryW(L"ntdll.dll");
        NtClose_init = 0;
        if (hMod) {
            NtClose = (PNtClose)GetProcAddress(
                hMod, "NtClose");
            if (NtClose) {
                NtClose_init = 1;
            } else {
                FreeLibrary(hMod);
            }
        }
    }

    if (NtClose_init <= 0) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return STATUS_NOT_SUPPORTED;
    }
    return NtClose(Handle);
}

static inline ULONG _Py_RtlNtStatusToDosError(
    NTSTATUS Status
) {
    static PRtlNtStatusToDosError RtlNtStatusToDosError = NULL;
    static int RtlNtStatusToDosError_init = -1;

    if (RtlNtStatusToDosError_init < 0) {
        HMODULE hMod = LoadLibraryW(L"ntdll.dll");
        RtlNtStatusToDosError_init = 0;
        if (hMod) {
            RtlNtStatusToDosError = (PRtlNtStatusToDosError)GetProcAddress(
                hMod, "RtlNtStatusToDosError");
            if (RtlNtStatusToDosError) {
                RtlNtStatusToDosError_init = 1;
            } else {
                FreeLibrary(hMod);
            }
        }
    }

    if (RtlNtStatusToDosError_init <= 0) {
        return ERROR_NOT_SUPPORTED;
    }
    return RtlNtStatusToDosError(Status);
}

static inline NTSTATUS _Py_RtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR          SourceString
) {
    static PRtlInitUnicodeString RtlInitUnicodeString = NULL;
    static int RtlInitUnicodeString_init = -1;

    if (RtlInitUnicodeString_init < 0) {
        HMODULE hMod = LoadLibraryW(L"ntdll.dll");
        RtlInitUnicodeString_init = 0;
        if (hMod) {
            RtlInitUnicodeString = (PRtlInitUnicodeString)GetProcAddress(
                hMod, "RtlInitUnicodeString");
            if (RtlInitUnicodeString) {
                RtlInitUnicodeString_init = 1;
            } else {
                FreeLibrary(hMod);
            }
        }
    }

    if (RtlInitUnicodeString_init <= 0) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return STATUS_NOT_SUPPORTED;
    }

    RtlInitUnicodeString(
        DestinationString,
        SourceString
    );
    return STATUS_SUCCESS;
}

static inline NTSTATUS _Py_RtlFreeUnicodeString(
    PUNICODE_STRING UnicodeString
) {
    static PRtlFreeUnicodeString RtlFreeUnicodeString = NULL;
    static int RtlFreeUnicodeString_init = -1;

    if (RtlFreeUnicodeString_init < 0) {
        HMODULE hMod = LoadLibraryW(L"ntdll.dll");
        RtlFreeUnicodeString_init = 0;
        if (hMod) {
            RtlFreeUnicodeString = (PRtlFreeUnicodeString)GetProcAddress(
                hMod, "RtlFreeUnicodeString");
            if (RtlFreeUnicodeString) {
                RtlFreeUnicodeString_init = 1;
            } else {
                FreeLibrary(hMod);
            }
        }
    }

    if (RtlFreeUnicodeString_init <= 0) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return STATUS_NOT_SUPPORTED;
    }

    RtlFreeUnicodeString(
        UnicodeString
    );
    return STATUS_SUCCESS;
}

static inline ULONG _Py_RtlDosPathNameToNtPathName_U_WithStatus(
    PCWSTR DosFileName,
    PUNICODE_STRING NtFileName,
    PWSTR *FilePart,
    PVOID Reserved
) {
    static PRtlDosPathNameToNtPathName_U_WithStatus RtlDosPathNameToNtPathName_U_WithStatus = NULL;
    static int RtlDosPathNameToNtPathName_U_WithStatus_init = -1;

    if (RtlDosPathNameToNtPathName_U_WithStatus_init < 0) {
        HMODULE hMod = LoadLibraryW(L"ntdll.dll");
        RtlDosPathNameToNtPathName_U_WithStatus_init = 0;
        if (hMod) {
            RtlDosPathNameToNtPathName_U_WithStatus = (PRtlDosPathNameToNtPathName_U_WithStatus)GetProcAddress(
                hMod, "RtlDosPathNameToNtPathName_U_WithStatus");
            if (RtlDosPathNameToNtPathName_U_WithStatus) {
                RtlDosPathNameToNtPathName_U_WithStatus_init = 1;
            } else {
                FreeLibrary(hMod);
            }
        }
    }

    if (RtlDosPathNameToNtPathName_U_WithStatus_init <= 0) {
        return ERROR_NOT_SUPPORTED;
    }
    return RtlDosPathNameToNtPathName_U_WithStatus(
        DosFileName,
        NtFileName,
        FilePart,
Reserved
    );
}

#endif

#endif
