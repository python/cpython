#! python
#
# Constants and types for use with Windows API, used by serialwin32.py
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2001-2015 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause

# pylint: disable=invalid-name,too-few-public-methods,protected-access,too-many-instance-attributes

from __future__ import absolute_import

from ctypes import c_ulong, c_void_p, c_int64, c_char, \
                   WinDLL, sizeof, Structure, Union, POINTER
from ctypes.wintypes import HANDLE
from ctypes.wintypes import BOOL
from ctypes.wintypes import LPCWSTR
from ctypes.wintypes import DWORD
from ctypes.wintypes import WORD
from ctypes.wintypes import BYTE
_stdcall_libraries = {}
_stdcall_libraries['kernel32'] = WinDLL('kernel32')

INVALID_HANDLE_VALUE = HANDLE(-1).value


# some details of the windows API differ between 32 and 64 bit systems..
def is_64bit():
    """Returns true when running on a 64 bit system"""
    return sizeof(c_ulong) != sizeof(c_void_p)

# ULONG_PTR is a an ordinary number, not a pointer and contrary to the name it
# is either 32 or 64 bits, depending on the type of windows...
# so test if this a 32 bit windows...
if is_64bit():
    ULONG_PTR = c_int64
else:
    ULONG_PTR = c_ulong


class _SECURITY_ATTRIBUTES(Structure):
    pass
LPSECURITY_ATTRIBUTES = POINTER(_SECURITY_ATTRIBUTES)


try:
    CreateEventW = _stdcall_libraries['kernel32'].CreateEventW
except AttributeError:
    # Fallback to non wide char version for old OS...
    from ctypes.wintypes import LPCSTR
    CreateEventA = _stdcall_libraries['kernel32'].CreateEventA
    CreateEventA.restype = HANDLE
    CreateEventA.argtypes = [LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPCSTR]
    CreateEvent = CreateEventA

    CreateFileA = _stdcall_libraries['kernel32'].CreateFileA
    CreateFileA.restype = HANDLE
    CreateFileA.argtypes = [LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE]
    CreateFile = CreateFileA
else:
    CreateEventW.restype = HANDLE
    CreateEventW.argtypes = [LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPCWSTR]
    CreateEvent = CreateEventW  # alias

    CreateFileW = _stdcall_libraries['kernel32'].CreateFileW
    CreateFileW.restype = HANDLE
    CreateFileW.argtypes = [LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE]
    CreateFile = CreateFileW  # alias


class _OVERLAPPED(Structure):
    pass

OVERLAPPED = _OVERLAPPED


class _COMSTAT(Structure):
    pass

COMSTAT = _COMSTAT


class _DCB(Structure):
    pass

DCB = _DCB


class _COMMTIMEOUTS(Structure):
    pass

COMMTIMEOUTS = _COMMTIMEOUTS

GetLastError = _stdcall_libraries['kernel32'].GetLastError
GetLastError.restype = DWORD
GetLastError.argtypes = []

LPOVERLAPPED = POINTER(_OVERLAPPED)
LPDWORD = POINTER(DWORD)

GetOverlappedResult = _stdcall_libraries['kernel32'].GetOverlappedResult
GetOverlappedResult.restype = BOOL
GetOverlappedResult.argtypes = [HANDLE, LPOVERLAPPED, LPDWORD, BOOL]

ResetEvent = _stdcall_libraries['kernel32'].ResetEvent
ResetEvent.restype = BOOL
ResetEvent.argtypes = [HANDLE]

LPCVOID = c_void_p

WriteFile = _stdcall_libraries['kernel32'].WriteFile
WriteFile.restype = BOOL
WriteFile.argtypes = [HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED]

LPVOID = c_void_p

ReadFile = _stdcall_libraries['kernel32'].ReadFile
ReadFile.restype = BOOL
ReadFile.argtypes = [HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED]

CloseHandle = _stdcall_libraries['kernel32'].CloseHandle
CloseHandle.restype = BOOL
CloseHandle.argtypes = [HANDLE]

ClearCommBreak = _stdcall_libraries['kernel32'].ClearCommBreak
ClearCommBreak.restype = BOOL
ClearCommBreak.argtypes = [HANDLE]

LPCOMSTAT = POINTER(_COMSTAT)

ClearCommError = _stdcall_libraries['kernel32'].ClearCommError
ClearCommError.restype = BOOL
ClearCommError.argtypes = [HANDLE, LPDWORD, LPCOMSTAT]

SetupComm = _stdcall_libraries['kernel32'].SetupComm
SetupComm.restype = BOOL
SetupComm.argtypes = [HANDLE, DWORD, DWORD]

EscapeCommFunction = _stdcall_libraries['kernel32'].EscapeCommFunction
EscapeCommFunction.restype = BOOL
EscapeCommFunction.argtypes = [HANDLE, DWORD]

GetCommModemStatus = _stdcall_libraries['kernel32'].GetCommModemStatus
GetCommModemStatus.restype = BOOL
GetCommModemStatus.argtypes = [HANDLE, LPDWORD]

LPDCB = POINTER(_DCB)

GetCommState = _stdcall_libraries['kernel32'].GetCommState
GetCommState.restype = BOOL
GetCommState.argtypes = [HANDLE, LPDCB]

LPCOMMTIMEOUTS = POINTER(_COMMTIMEOUTS)

GetCommTimeouts = _stdcall_libraries['kernel32'].GetCommTimeouts
GetCommTimeouts.restype = BOOL
GetCommTimeouts.argtypes = [HANDLE, LPCOMMTIMEOUTS]

PurgeComm = _stdcall_libraries['kernel32'].PurgeComm
PurgeComm.restype = BOOL
PurgeComm.argtypes = [HANDLE, DWORD]

SetCommBreak = _stdcall_libraries['kernel32'].SetCommBreak
SetCommBreak.restype = BOOL
SetCommBreak.argtypes = [HANDLE]

SetCommMask = _stdcall_libraries['kernel32'].SetCommMask
SetCommMask.restype = BOOL
SetCommMask.argtypes = [HANDLE, DWORD]

SetCommState = _stdcall_libraries['kernel32'].SetCommState
SetCommState.restype = BOOL
SetCommState.argtypes = [HANDLE, LPDCB]

SetCommTimeouts = _stdcall_libraries['kernel32'].SetCommTimeouts
SetCommTimeouts.restype = BOOL
SetCommTimeouts.argtypes = [HANDLE, LPCOMMTIMEOUTS]

WaitForSingleObject = _stdcall_libraries['kernel32'].WaitForSingleObject
WaitForSingleObject.restype = DWORD
WaitForSingleObject.argtypes = [HANDLE, DWORD]

WaitCommEvent = _stdcall_libraries['kernel32'].WaitCommEvent
WaitCommEvent.restype = BOOL
WaitCommEvent.argtypes = [HANDLE, LPDWORD, LPOVERLAPPED]

CancelIoEx = _stdcall_libraries['kernel32'].CancelIoEx
CancelIoEx.restype = BOOL
CancelIoEx.argtypes = [HANDLE, LPOVERLAPPED]

ONESTOPBIT = 0  # Variable c_int
TWOSTOPBITS = 2  # Variable c_int
ONE5STOPBITS = 1

NOPARITY = 0  # Variable c_int
ODDPARITY = 1  # Variable c_int
EVENPARITY = 2  # Variable c_int
MARKPARITY = 3
SPACEPARITY = 4

RTS_CONTROL_HANDSHAKE = 2  # Variable c_int
RTS_CONTROL_DISABLE = 0  # Variable c_int
RTS_CONTROL_ENABLE = 1  # Variable c_int
RTS_CONTROL_TOGGLE = 3  # Variable c_int
SETRTS = 3
CLRRTS = 4

DTR_CONTROL_HANDSHAKE = 2  # Variable c_int
DTR_CONTROL_DISABLE = 0  # Variable c_int
DTR_CONTROL_ENABLE = 1  # Variable c_int
SETDTR = 5
CLRDTR = 6

MS_DSR_ON = 32  # Variable c_ulong
EV_RING = 256  # Variable c_int
EV_PERR = 512  # Variable c_int
EV_ERR = 128  # Variable c_int
SETXOFF = 1  # Variable c_int
EV_RXCHAR = 1  # Variable c_int
GENERIC_WRITE = 1073741824  # Variable c_long
PURGE_TXCLEAR = 4  # Variable c_int
FILE_FLAG_OVERLAPPED = 1073741824  # Variable c_int
EV_DSR = 16  # Variable c_int
MAXDWORD = 4294967295  # Variable c_uint
EV_RLSD = 32  # Variable c_int

ERROR_SUCCESS = 0
ERROR_NOT_ENOUGH_MEMORY = 8
ERROR_OPERATION_ABORTED = 995
ERROR_IO_INCOMPLETE = 996
ERROR_IO_PENDING = 997  # Variable c_long
ERROR_INVALID_USER_BUFFER = 1784

MS_CTS_ON = 16  # Variable c_ulong
EV_EVENT1 = 2048  # Variable c_int
EV_RX80FULL = 1024  # Variable c_int
PURGE_RXABORT = 2  # Variable c_int
FILE_ATTRIBUTE_NORMAL = 128  # Variable c_int
PURGE_TXABORT = 1  # Variable c_int
SETXON = 2  # Variable c_int
OPEN_EXISTING = 3  # Variable c_int
MS_RING_ON = 64  # Variable c_ulong
EV_TXEMPTY = 4  # Variable c_int
EV_RXFLAG = 2  # Variable c_int
MS_RLSD_ON = 128  # Variable c_ulong
GENERIC_READ = 2147483648  # Variable c_ulong
EV_EVENT2 = 4096  # Variable c_int
EV_CTS = 8  # Variable c_int
EV_BREAK = 64  # Variable c_int
PURGE_RXCLEAR = 8  # Variable c_int
INFINITE = 0xFFFFFFFF

CE_RXOVER = 0x0001
CE_OVERRUN = 0x0002
CE_RXPARITY = 0x0004
CE_FRAME = 0x0008
CE_BREAK = 0x0010


class N11_OVERLAPPED4DOLLAR_48E(Union):
    pass


class N11_OVERLAPPED4DOLLAR_484DOLLAR_49E(Structure):
    pass


N11_OVERLAPPED4DOLLAR_484DOLLAR_49E._fields_ = [
    ('Offset', DWORD),
    ('OffsetHigh', DWORD),
]

PVOID = c_void_p

N11_OVERLAPPED4DOLLAR_48E._anonymous_ = ['_0']
N11_OVERLAPPED4DOLLAR_48E._fields_ = [
    ('_0', N11_OVERLAPPED4DOLLAR_484DOLLAR_49E),
    ('Pointer', PVOID),
]
_OVERLAPPED._anonymous_ = ['_0']
_OVERLAPPED._fields_ = [
    ('Internal', ULONG_PTR),
    ('InternalHigh', ULONG_PTR),
    ('_0', N11_OVERLAPPED4DOLLAR_48E),
    ('hEvent', HANDLE),
]
_SECURITY_ATTRIBUTES._fields_ = [
    ('nLength', DWORD),
    ('lpSecurityDescriptor', LPVOID),
    ('bInheritHandle', BOOL),
]
_COMSTAT._fields_ = [
    ('fCtsHold', DWORD, 1),
    ('fDsrHold', DWORD, 1),
    ('fRlsdHold', DWORD, 1),
    ('fXoffHold', DWORD, 1),
    ('fXoffSent', DWORD, 1),
    ('fEof', DWORD, 1),
    ('fTxim', DWORD, 1),
    ('fReserved', DWORD, 25),
    ('cbInQue', DWORD),
    ('cbOutQue', DWORD),
]
_DCB._fields_ = [
    ('DCBlength', DWORD),
    ('BaudRate', DWORD),
    ('fBinary', DWORD, 1),
    ('fParity', DWORD, 1),
    ('fOutxCtsFlow', DWORD, 1),
    ('fOutxDsrFlow', DWORD, 1),
    ('fDtrControl', DWORD, 2),
    ('fDsrSensitivity', DWORD, 1),
    ('fTXContinueOnXoff', DWORD, 1),
    ('fOutX', DWORD, 1),
    ('fInX', DWORD, 1),
    ('fErrorChar', DWORD, 1),
    ('fNull', DWORD, 1),
    ('fRtsControl', DWORD, 2),
    ('fAbortOnError', DWORD, 1),
    ('fDummy2', DWORD, 17),
    ('wReserved', WORD),
    ('XonLim', WORD),
    ('XoffLim', WORD),
    ('ByteSize', BYTE),
    ('Parity', BYTE),
    ('StopBits', BYTE),
    ('XonChar', c_char),
    ('XoffChar', c_char),
    ('ErrorChar', c_char),
    ('EofChar', c_char),
    ('EvtChar', c_char),
    ('wReserved1', WORD),
]
_COMMTIMEOUTS._fields_ = [
    ('ReadIntervalTimeout', DWORD),
    ('ReadTotalTimeoutMultiplier', DWORD),
    ('ReadTotalTimeoutConstant', DWORD),
    ('WriteTotalTimeoutMultiplier', DWORD),
    ('WriteTotalTimeoutConstant', DWORD),
]
__all__ = ['GetLastError', 'MS_CTS_ON', 'FILE_ATTRIBUTE_NORMAL',
           'DTR_CONTROL_ENABLE', '_COMSTAT', 'MS_RLSD_ON',
           'GetOverlappedResult', 'SETXON', 'PURGE_TXABORT',
           'PurgeComm', 'N11_OVERLAPPED4DOLLAR_48E', 'EV_RING',
           'ONESTOPBIT', 'SETXOFF', 'PURGE_RXABORT', 'GetCommState',
           'RTS_CONTROL_ENABLE', '_DCB', 'CreateEvent',
           '_COMMTIMEOUTS', '_SECURITY_ATTRIBUTES', 'EV_DSR',
           'EV_PERR', 'EV_RXFLAG', 'OPEN_EXISTING', 'DCB',
           'FILE_FLAG_OVERLAPPED', 'EV_CTS', 'SetupComm',
           'LPOVERLAPPED', 'EV_TXEMPTY', 'ClearCommBreak',
           'LPSECURITY_ATTRIBUTES', 'SetCommBreak', 'SetCommTimeouts',
           'COMMTIMEOUTS', 'ODDPARITY', 'EV_RLSD',
           'GetCommModemStatus', 'EV_EVENT2', 'PURGE_TXCLEAR',
           'EV_BREAK', 'EVENPARITY', 'LPCVOID', 'COMSTAT', 'ReadFile',
           'PVOID', '_OVERLAPPED', 'WriteFile', 'GetCommTimeouts',
           'ResetEvent', 'EV_RXCHAR', 'LPCOMSTAT', 'ClearCommError',
           'ERROR_IO_PENDING', 'EscapeCommFunction', 'GENERIC_READ',
           'RTS_CONTROL_HANDSHAKE', 'OVERLAPPED',
           'DTR_CONTROL_HANDSHAKE', 'PURGE_RXCLEAR', 'GENERIC_WRITE',
           'LPDCB', 'CreateEventW', 'SetCommMask', 'EV_EVENT1',
           'SetCommState', 'LPVOID', 'CreateFileW', 'LPDWORD',
           'EV_RX80FULL', 'TWOSTOPBITS', 'LPCOMMTIMEOUTS', 'MAXDWORD',
           'MS_DSR_ON', 'MS_RING_ON',
           'N11_OVERLAPPED4DOLLAR_484DOLLAR_49E', 'EV_ERR',
           'ULONG_PTR', 'CreateFile', 'NOPARITY', 'CloseHandle']
