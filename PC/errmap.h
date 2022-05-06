int
winerror_to_errno(int winerror)
{
    // Unwrap FACILITY_WIN32 HRESULT errors.
    if ((winerror & 0xFFFF0000) == 0x80070000) {
        winerror &= 0x0000FFFF;
    }

    // Winsock error codes (10000-11999) are errno values.
    if (winerror >= 10000 && winerror < 12000) {
        switch (winerror) {
        case WSAEINTR:
        case WSAEBADF:
        case WSAEACCES:
        case WSAEFAULT:
        case WSAEINVAL:
        case WSAEMFILE:
            // Winsock definitions of errno values. See WinSock2.h
            return winerror - 10000;
        default:
            return winerror;
        }
    }

    switch (winerror) {
    case ERROR_FILE_NOT_FOUND:            //    2
    case ERROR_PATH_NOT_FOUND:            //    3
    case ERROR_INVALID_DRIVE:             //   15
    case ERROR_NO_MORE_FILES:             //   18
    case ERROR_BAD_NETPATH:               //   53
    case ERROR_BAD_NET_NAME:              //   67
    case ERROR_BAD_PATHNAME:              //  161
    case ERROR_FILENAME_EXCED_RANGE:      //  206
        return ENOENT;

    case ERROR_BAD_ENVIRONMENT:           //   10
        return E2BIG;

    case ERROR_BAD_FORMAT:                //   11
    case ERROR_INVALID_STARTING_CODESEG:  //  188
    case ERROR_INVALID_STACKSEG:          //  189
    case ERROR_INVALID_MODULETYPE:        //  190
    case ERROR_INVALID_EXE_SIGNATURE:     //  191
    case ERROR_EXE_MARKED_INVALID:        //  192
    case ERROR_BAD_EXE_FORMAT:            //  193
    case ERROR_ITERATED_DATA_EXCEEDS_64k: //  194
    case ERROR_INVALID_MINALLOCSIZE:      //  195
    case ERROR_DYNLINK_FROM_INVALID_RING: //  196
    case ERROR_IOPL_NOT_ENABLED:          //  197
    case ERROR_INVALID_SEGDPL:            //  198
    case ERROR_AUTODATASEG_EXCEEDS_64k:   //  199
    case ERROR_RING2SEG_MUST_BE_MOVABLE:  //  200
    case ERROR_RELOC_CHAIN_XEEDS_SEGLIM:  //  201
    case ERROR_INFLOOP_IN_RELOC_CHAIN:    //  202
        return ENOEXEC;

    case ERROR_INVALID_HANDLE:            //    6
    case ERROR_INVALID_TARGET_HANDLE:     //  114
    case ERROR_DIRECT_ACCESS_HANDLE:      //  130
        return EBADF;

    case ERROR_WAIT_NO_CHILDREN:          //  128
    case ERROR_CHILD_NOT_COMPLETE:        //  129
        return ECHILD;

    case ERROR_NO_PROC_SLOTS:             //   89
    case ERROR_MAX_THRDS_REACHED:         //  164
    case ERROR_NESTING_NOT_ALLOWED:       //  215
        return EAGAIN;

    case ERROR_ARENA_TRASHED:             //    7
    case ERROR_NOT_ENOUGH_MEMORY:         //    8
    case ERROR_INVALID_BLOCK:             //    9
    case ERROR_NOT_ENOUGH_QUOTA:          // 1816
        return ENOMEM;

    case ERROR_ACCESS_DENIED:             //    5
    case ERROR_CURRENT_DIRECTORY:         //   16
    case ERROR_WRITE_PROTECT:             //   19
    case ERROR_BAD_UNIT:                  //   20
    case ERROR_NOT_READY:                 //   21
    case ERROR_BAD_COMMAND:               //   22
    case ERROR_CRC:                       //   23
    case ERROR_BAD_LENGTH:                //   24
    case ERROR_SEEK:                      //   25
    case ERROR_NOT_DOS_DISK:              //   26
    case ERROR_SECTOR_NOT_FOUND:          //   27
    case ERROR_OUT_OF_PAPER:              //   28
    case ERROR_WRITE_FAULT:               //   29
    case ERROR_READ_FAULT:                //   30
    case ERROR_GEN_FAILURE:               //   31
    case ERROR_SHARING_VIOLATION:         //   32
    case ERROR_LOCK_VIOLATION:            //   33
    case ERROR_WRONG_DISK:                //   34
    case ERROR_SHARING_BUFFER_EXCEEDED:   //   36
    case ERROR_NETWORK_ACCESS_DENIED:     //   65
    case ERROR_CANNOT_MAKE:               //   82
    case ERROR_FAIL_I24:                  //   83
    case ERROR_DRIVE_LOCKED:              //  108
    case ERROR_SEEK_ON_DEVICE:            //  132
    case ERROR_NOT_LOCKED:                //  158
    case ERROR_LOCK_FAILED:               //  167
    case 35:                              //   35 (undefined)
        return EACCES;

    case ERROR_FILE_EXISTS:               //   80
    case ERROR_ALREADY_EXISTS:            //  183
        return EEXIST;

    case ERROR_NOT_SAME_DEVICE:           //   17
        return EXDEV;

    case ERROR_DIRECTORY:                 //  267 (bpo-12802)
        return ENOTDIR;

    case ERROR_TOO_MANY_OPEN_FILES:       //    4
        return EMFILE;

    case ERROR_DISK_FULL:                 //  112
        return ENOSPC;

    case ERROR_BROKEN_PIPE:               //  109
    case ERROR_NO_DATA:                   //  232 (bpo-13063)
        return EPIPE;

    case ERROR_DIR_NOT_EMPTY:             //  145
        return ENOTEMPTY;

    case ERROR_NO_UNICODE_TRANSLATION:    // 1113
        return EILSEQ;

    case ERROR_INVALID_FUNCTION:          //    1
    case ERROR_INVALID_ACCESS:            //   12
    case ERROR_INVALID_DATA:              //   13
    case ERROR_INVALID_PARAMETER:         //   87
    case ERROR_NEGATIVE_SEEK:             //  131
    default:
        return EINVAL;
    }
}
