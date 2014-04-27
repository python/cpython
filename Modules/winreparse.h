#ifndef Py_WINREPARSE_H
#define Py_WINREPARSE_H

#ifdef MS_WINDOWS
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The following structure was copied from
   http://msdn.microsoft.com/en-us/library/ff552012.aspx as the required
   include doesn't seem to be present in the Windows SDK (at least as included
   with Visual Studio Express). */
typedef struct _REPARSE_DATA_BUFFER {
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG Flags;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;

        struct {
            USHORT SubstituteNameOffset;
            USHORT  SubstituteNameLength;
            USHORT  PrintNameOffset;
            USHORT  PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;

        struct {
            UCHAR  DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

#define REPARSE_DATA_BUFFER_HEADER_SIZE  FIELD_OFFSET(REPARSE_DATA_BUFFER,\
                                                      GenericReparseBuffer)
#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE  ( 16 * 1024 )

#ifdef __cplusplus
}
#endif

#endif /* MS_WINDOWS */

#endif /* !Py_WINREPARSE_H */
