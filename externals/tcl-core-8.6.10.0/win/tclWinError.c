/*
 * tclWinError.c --
 *
 *	This file contains code for converting from Win32 errors to errno
 *	errors.
 *
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
/*
 * The following table contains the mapping from Win32 errors to errno errors.
 */

static const unsigned char errorTable[] = {
    0,
    EINVAL,	/* ERROR_INVALID_FUNCTION	1 */
    ENOENT,	/* ERROR_FILE_NOT_FOUND		2 */
    ENOENT,	/* ERROR_PATH_NOT_FOUND		3 */
    EMFILE,	/* ERROR_TOO_MANY_OPEN_FILES	4 */
    EACCES,	/* ERROR_ACCESS_DENIED		5 */
    EBADF,	/* ERROR_INVALID_HANDLE		6 */
    ENOMEM,	/* ERROR_ARENA_TRASHED		7 */
    ENOMEM,	/* ERROR_NOT_ENOUGH_MEMORY	8 */
    ENOMEM,	/* ERROR_INVALID_BLOCK		9 */
    E2BIG,	/* ERROR_BAD_ENVIRONMENT	10 */
    ENOEXEC,	/* ERROR_BAD_FORMAT		11 */
    EACCES,	/* ERROR_INVALID_ACCESS		12 */
    EINVAL,	/* ERROR_INVALID_DATA		13 */
    ENOMEM,	/* ERROR_OUT_OF_MEMORY		14 */
    ENOENT,	/* ERROR_INVALID_DRIVE		15 */
    EACCES,	/* ERROR_CURRENT_DIRECTORY	16 */
    EXDEV,	/* ERROR_NOT_SAME_DEVICE	17 */
    ENOENT,	/* ERROR_NO_MORE_FILES		18 */
    EROFS,	/* ERROR_WRITE_PROTECT		19 */
    ENXIO,	/* ERROR_BAD_UNIT		20 */
    EBUSY,	/* ERROR_NOT_READY		21 */
    EIO,	/* ERROR_BAD_COMMAND		22 */
    EIO,	/* ERROR_CRC			23 */
    EIO,	/* ERROR_BAD_LENGTH		24 */
    EIO,	/* ERROR_SEEK			25 */
    EIO,	/* ERROR_NOT_DOS_DISK		26 */
    ENXIO,	/* ERROR_SECTOR_NOT_FOUND	27 */
    EBUSY,	/* ERROR_OUT_OF_PAPER		28 */
    EIO,	/* ERROR_WRITE_FAULT		29 */
    EIO,	/* ERROR_READ_FAULT		30 */
    EIO,	/* ERROR_GEN_FAILURE		31 */
    EACCES,	/* ERROR_SHARING_VIOLATION	32 */
    EACCES,	/* ERROR_LOCK_VIOLATION		33 */
    ENXIO,	/* ERROR_WRONG_DISK		34 */
    ENFILE,	/* ERROR_FCB_UNAVAILABLE	35 */
    ENFILE,	/* ERROR_SHARING_BUFFER_EXCEEDED	36 */
    EINVAL,	/* 37 */
    EINVAL,	/* 38 */
    ENOSPC,	/* ERROR_HANDLE_DISK_FULL	39 */
    EINVAL,	/* 40 */
    EINVAL,	/* 41 */
    EINVAL,	/* 42 */
    EINVAL,	/* 43 */
    EINVAL,	/* 44 */
    EINVAL,	/* 45 */
    EINVAL,	/* 46 */
    EINVAL,	/* 47 */
    EINVAL,	/* 48 */
    EINVAL,	/* 49 */
    ENODEV,	/* ERROR_NOT_SUPPORTED		50 */
    EBUSY,	/* ERROR_REM_NOT_LIST		51 */
    EEXIST,	/* ERROR_DUP_NAME		52 */
    ENOENT,	/* ERROR_BAD_NETPATH		53 */
    EBUSY,	/* ERROR_NETWORK_BUSY		54 */
    ENODEV,	/* ERROR_DEV_NOT_EXIST		55 */
    EAGAIN,	/* ERROR_TOO_MANY_CMDS		56 */
    EIO,	/* ERROR_ADAP_HDW_ERR		57 */
    EIO,	/* ERROR_BAD_NET_RESP		58 */
    EIO,	/* ERROR_UNEXP_NET_ERR		59 */
    EINVAL,	/* ERROR_BAD_REM_ADAP		60 */
    EFBIG,	/* ERROR_PRINTQ_FULL		61 */
    ENOSPC,	/* ERROR_NO_SPOOL_SPACE		62 */
    ENOENT,	/* ERROR_PRINT_CANCELLED	63 */
    ENOENT,	/* ERROR_NETNAME_DELETED	64 */
    EACCES,	/* ERROR_NETWORK_ACCESS_DENIED	65 */
    ENODEV,	/* ERROR_BAD_DEV_TYPE		66 */
    ENOENT,	/* ERROR_BAD_NET_NAME		67 */
    ENFILE,	/* ERROR_TOO_MANY_NAMES		68 */
    EIO,	/* ERROR_TOO_MANY_SESS		69 */
    EAGAIN,	/* ERROR_SHARING_PAUSED		70 */
    EINVAL,	/* ERROR_REQ_NOT_ACCEP		71 */
    EAGAIN,	/* ERROR_REDIR_PAUSED		72 */
    EINVAL,	/* 73 */
    EINVAL,	/* 74 */
    EINVAL,	/* 75 */
    EINVAL,	/* 76 */
    EINVAL,	/* 77 */
    EINVAL,	/* 78 */
    EINVAL,	/* 79 */
    EEXIST,	/* ERROR_FILE_EXISTS		80 */
    EINVAL,	/* 81 */
    ENOSPC,	/* ERROR_CANNOT_MAKE		82 */
    EIO,	/* ERROR_FAIL_I24		83 */
    ENFILE,	/* ERROR_OUT_OF_STRUCTURES	84 */
    EEXIST,	/* ERROR_ALREADY_ASSIGNED	85 */
    EPERM,	/* ERROR_INVALID_PASSWORD	86 */
    EINVAL,	/* ERROR_INVALID_PARAMETER	87 */
    EIO,	/* ERROR_NET_WRITE_FAULT	88 */
    EAGAIN,	/* ERROR_NO_PROC_SLOTS		89 */
    EINVAL,	/* 90 */
    EINVAL,	/* 91 */
    EINVAL,	/* 92 */
    EINVAL,	/* 93 */
    EINVAL,	/* 94 */
    EINVAL,	/* 95 */
    EINVAL,	/* 96 */
    EINVAL,	/* 97 */
    EINVAL,	/* 98 */
    EINVAL,	/* 99 */
    EINVAL,	/* 100 */
    EINVAL,	/* 101 */
    EINVAL,	/* 102 */
    EINVAL,	/* 103 */
    EINVAL,	/* 104 */
    EINVAL,	/* 105 */
    EINVAL,	/* 106 */
    EXDEV,	/* ERROR_DISK_CHANGE		107 */
    EAGAIN,	/* ERROR_DRIVE_LOCKED		108 */
    EPIPE,	/* ERROR_BROKEN_PIPE		109 */
    ENOENT,	/* ERROR_OPEN_FAILED		110 */
    EINVAL,	/* ERROR_BUFFER_OVERFLOW	111 */
    ENOSPC,	/* ERROR_DISK_FULL		112 */
    EMFILE,	/* ERROR_NO_MORE_SEARCH_HANDLES	113 */
    EBADF,	/* ERROR_INVALID_TARGET_HANDLE	114 */
    EFAULT,	/* ERROR_PROTECTION_VIOLATION	115 */
    EINVAL,	/* 116 */
    EINVAL,	/* 117 */
    EINVAL,	/* 118 */
    EINVAL,	/* 119 */
    EINVAL,	/* 120 */
    EINVAL,	/* 121 */
    EINVAL,	/* 122 */
    ENOENT,	/* ERROR_INVALID_NAME		123 */
    EINVAL,	/* 124 */
    EINVAL,	/* 125 */
    EINVAL,	/* 126 */
    EINVAL,	/* ERROR_PROC_NOT_FOUND		127 */
    ECHILD,	/* ERROR_WAIT_NO_CHILDREN	128 */
    ECHILD,	/* ERROR_CHILD_NOT_COMPLETE	129 */
    EBADF,	/* ERROR_DIRECT_ACCESS_HANDLE	130 */
    EINVAL,	/* ERROR_NEGATIVE_SEEK		131 */
    ESPIPE,	/* ERROR_SEEK_ON_DEVICE		132 */
    EINVAL,	/* 133 */
    EINVAL,	/* 134 */
    EINVAL,	/* 135 */
    EINVAL,	/* 136 */
    EINVAL,	/* 137 */
    EINVAL,	/* 138 */
    EINVAL,	/* 139 */
    EINVAL,	/* 140 */
    EINVAL,	/* 141 */
    EAGAIN,	/* ERROR_BUSY_DRIVE		142 */
    EINVAL,	/* 143 */
    EINVAL,	/* 144 */
    EEXIST,	/* ERROR_DIR_NOT_EMPTY		145 */
    EINVAL,	/* 146 */
    EINVAL,	/* 147 */
    EINVAL,	/* 148 */
    EINVAL,	/* 149 */
    EINVAL,	/* 150 */
    EINVAL,	/* 151 */
    EINVAL,	/* 152 */
    EINVAL,	/* 153 */
    EINVAL,	/* 154 */
    EINVAL,	/* 155 */
    EINVAL,	/* 156 */
    EINVAL,	/* 157 */
    EACCES,	/* ERROR_NOT_LOCKED		158 */
    EINVAL,	/* 159 */
    EINVAL,	/* 160 */
    ENOENT,	/* ERROR_BAD_PATHNAME	        161 */
    EINVAL,	/* 162 */
    EINVAL,	/* 163 */
    EINVAL,	/* 164 */
    EINVAL,	/* 165 */
    EINVAL,	/* 166 */
    EACCES,	/* ERROR_LOCK_FAILED		167 */
    EINVAL,	/* 168 */
    EINVAL,	/* 169 */
    EINVAL,	/* 170 */
    EINVAL,	/* 171 */
    EINVAL,	/* 172 */
    EINVAL,	/* 173 */
    EINVAL,	/* 174 */
    EINVAL,	/* 175 */
    EINVAL,	/* 176 */
    EINVAL,	/* 177 */
    EINVAL,	/* 178 */
    EINVAL,	/* 179 */
    EINVAL,	/* 180 */
    EINVAL,	/* 181 */
    EINVAL,	/* 182 */
    EEXIST,	/* ERROR_ALREADY_EXISTS		183 */
    ECHILD,	/* ERROR_NO_CHILD_PROCESS	184 */
    EINVAL,	/* 185 */
    EINVAL,	/* 186 */
    EINVAL,	/* 187 */
    EINVAL,	/* 188 */
    EINVAL,	/* 189 */
    EINVAL,	/* 190 */
    EINVAL,	/* 191 */
    EINVAL,	/* 192 */
    EINVAL,	/* 193 */
    EINVAL,	/* 194 */
    EINVAL,	/* 195 */
    EINVAL,	/* 196 */
    EINVAL,	/* 197 */
    EINVAL,	/* 198 */
    EINVAL,	/* 199 */
    EINVAL,	/* 200 */
    EINVAL,	/* 201 */
    EINVAL,	/* 202 */
    EINVAL,	/* 203 */
    EINVAL,	/* 204 */
    EINVAL,	/* 205 */
    ENAMETOOLONG,/* ERROR_FILENAME_EXCED_RANGE	206 */
    EINVAL,	/* 207 */
    EINVAL,	/* 208 */
    EINVAL,	/* 209 */
    EINVAL,	/* 210 */
    EINVAL,	/* 211 */
    EINVAL,	/* 212 */
    EINVAL,	/* 213 */
    EINVAL,	/* 214 */
    EINVAL,	/* 215 */
    EINVAL,	/* 216 */
    EINVAL,	/* 217 */
    EINVAL,	/* 218 */
    EINVAL,	/* 219 */
    EINVAL,	/* 220 */
    EINVAL,	/* 221 */
    EINVAL,	/* 222 */
    EINVAL,	/* 223 */
    EINVAL,	/* 224 */
    EINVAL,	/* 225 */
    EINVAL,	/* 226 */
    EINVAL,	/* 227 */
    EINVAL,	/* 228 */
    EINVAL,	/* 229 */
    EPIPE,	/* ERROR_BAD_PIPE		230 */
    EAGAIN,	/* ERROR_PIPE_BUSY		231 */
    EPIPE,	/* ERROR_NO_DATA		232 */
    EPIPE,	/* ERROR_PIPE_NOT_CONNECTED	233 */
    EINVAL,	/* 234 */
    EINVAL,	/* 235 */
    EINVAL,	/* 236 */
    EINVAL,	/* 237 */
    EINVAL,	/* 238 */
    EINVAL,	/* 239 */
    EINVAL,	/* 240 */
    EINVAL,	/* 241 */
    EINVAL,	/* 242 */
    EINVAL,	/* 243 */
    EINVAL,	/* 244 */
    EINVAL,	/* 245 */
    EINVAL,	/* 246 */
    EINVAL,	/* 247 */
    EINVAL,	/* 248 */
    EINVAL,	/* 249 */
    EINVAL,	/* 250 */
    EINVAL,	/* 251 */
    EINVAL,	/* 252 */
    EINVAL,	/* 253 */
    EINVAL,	/* 254 */
    EINVAL,	/* 255 */
    EINVAL,	/* 256 */
    EINVAL,	/* 257 */
    EINVAL,	/* 258 */
    EINVAL,	/* 259 */
    EINVAL,	/* 260 */
    EINVAL,	/* 261 */
    EINVAL,	/* 262 */
    EINVAL,	/* 263 */
    EINVAL,	/* 264 */
    EINVAL,	/* 265 */
    EINVAL,	/* 266 */
    ENOTDIR	/* ERROR_DIRECTORY		267 */
};

/*
 * The following table contains the mapping from WinSock errors to
 * errno errors.
 */

static const unsigned char wsaErrorTable[] = {
    EWOULDBLOCK,	/* WSAEWOULDBLOCK */
    EINPROGRESS,	/* WSAEINPROGRESS */
    EALREADY,		/* WSAEALREADY */
    ENOTSOCK,		/* WSAENOTSOCK */
    EDESTADDRREQ,	/* WSAEDESTADDRREQ */
    EMSGSIZE,		/* WSAEMSGSIZE */
    EPROTOTYPE,		/* WSAEPROTOTYPE */
    ENOPROTOOPT,	/* WSAENOPROTOOPT */
    EPROTONOSUPPORT,	/* WSAEPROTONOSUPPORT */
    ESOCKTNOSUPPORT,	/* WSAESOCKTNOSUPPORT */
    EOPNOTSUPP,		/* WSAEOPNOTSUPP */
    EPFNOSUPPORT,	/* WSAEPFNOSUPPORT */
    EAFNOSUPPORT,	/* WSAEAFNOSUPPORT */
    EADDRINUSE,		/* WSAEADDRINUSE */
    EADDRNOTAVAIL,	/* WSAEADDRNOTAVAIL */
    ENETDOWN,		/* WSAENETDOWN */
    ENETUNREACH,	/* WSAENETUNREACH */
    ENETRESET,		/* WSAENETRESET */
    ECONNABORTED,	/* WSAECONNABORTED */
    ECONNRESET,		/* WSAECONNRESET */
    ENOBUFS,		/* WSAENOBUFS */
    EISCONN,		/* WSAEISCONN */
    ENOTCONN,		/* WSAENOTCONN */
    ESHUTDOWN,		/* WSAESHUTDOWN */
    ETOOMANYREFS,	/* WSAETOOMANYREFS */
    ETIMEDOUT,		/* WSAETIMEDOUT */
    ECONNREFUSED,	/* WSAECONNREFUSED */
    ELOOP,		/* WSAELOOP */
    ENAMETOOLONG,	/* WSAENAMETOOLONG */
    EHOSTDOWN,		/* WSAEHOSTDOWN */
    EHOSTUNREACH,	/* WSAEHOSTUNREACH */
    ENOTEMPTY,		/* WSAENOTEMPTY */
    EAGAIN,		/* WSAEPROCLIM */
    EUSERS,		/* WSAEUSERS */
    EDQUOT,		/* WSAEDQUOT */
    ESTALE,		/* WSAESTALE */
    EREMOTE		/* WSAEREMOTE */
};

/*
 *----------------------------------------------------------------------
 *
 * TclWinConvertError --
 *
 *	This routine converts a Win32 error into an errno value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the errno global variable.
 *
 *----------------------------------------------------------------------
 */

void
TclWinConvertError(
    DWORD errCode)		/* Win32 error code. */
{
    if (errCode >= sizeof(errorTable)/sizeof(errorTable[0])) {
	errCode -= WSAEWOULDBLOCK;
	if (errCode >= sizeof(wsaErrorTable)/sizeof(wsaErrorTable[0])) {
	    Tcl_SetErrno(errorTable[1]);
	} else {
	    Tcl_SetErrno(wsaErrorTable[errCode]);
	}
    } else {
	Tcl_SetErrno(errorTable[errCode]);
    }
}

#ifdef __CYGWIN__
/*
 *----------------------------------------------------------------------
 *
 * tclWinDebugPanic --
 *
 *	Display a message. If a debugger is present, present it directly to
 *	the debugger, otherwise send it to stderr.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TCL_NORETURN void
tclWinDebugPanic(
    const char *format, ...)
{
#define TCL_MAX_WARN_LEN 1024
    va_list argList;
    va_start(argList, format);

    if (IsDebuggerPresent()) {
	WCHAR msgString[TCL_MAX_WARN_LEN];
	char buf[TCL_MAX_WARN_LEN * TCL_UTF_MAX];

	vsnprintf(buf, sizeof(buf), format, argList);
	msgString[TCL_MAX_WARN_LEN-1] = L'\0';
	MultiByteToWideChar(CP_UTF8, 0, buf, -1, msgString, TCL_MAX_WARN_LEN);

	/*
	 * Truncate MessageBox string if it is too long to not overflow the buffer.
	 */

	if (msgString[TCL_MAX_WARN_LEN-1] != L'\0') {
	    memcpy(msgString + (TCL_MAX_WARN_LEN - 5), L" ...", 5 * sizeof(WCHAR));
	}
	OutputDebugStringW(msgString);
    } else {
	vfprintf(stderr, format, argList);
	fprintf(stderr, "\n");
	fflush(stderr);
    }
#   if defined(__GNUC__)
    __builtin_trap();
#   else
    DebugBreak();
#   endif
    abort();
}
#endif
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * End:
 */
