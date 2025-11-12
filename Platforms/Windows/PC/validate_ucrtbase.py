'''
This script gets the version number from ucrtbased.dll and checks
whether it is a version with a known issue.
'''

import sys

from ctypes import (c_buffer, POINTER, byref, create_unicode_buffer,
                    Structure, WinDLL)
from ctypes.wintypes import DWORD, HANDLE

class VS_FIXEDFILEINFO(Structure):
    _fields_ = [
        ("dwSignature", DWORD),
        ("dwStrucVersion", DWORD),
        ("dwFileVersionMS", DWORD),
        ("dwFileVersionLS", DWORD),
        ("dwProductVersionMS", DWORD),
        ("dwProductVersionLS", DWORD),
        ("dwFileFlagsMask", DWORD),
        ("dwFileFlags", DWORD),
        ("dwFileOS", DWORD),
        ("dwFileType", DWORD),
        ("dwFileSubtype", DWORD),
        ("dwFileDateMS", DWORD),
        ("dwFileDateLS", DWORD),
    ]

kernel32 = WinDLL('kernel32')
version = WinDLL('version')

if len(sys.argv) < 2:
    print('Usage: validate_ucrtbase.py <ucrtbase|ucrtbased>')
    sys.exit(2)

try:
    ucrtbased = WinDLL(sys.argv[1])
except OSError:
    print('Cannot find ucrtbased.dll')
    # This likely means that VS is not installed, but that is an
    # obvious enough problem if you're trying to produce a debug
    # build that we don't need to fail here.
    sys.exit(0)

# We will immediately double the length up to MAX_PATH, but the
# path may be longer, so we retry until the returned string is
# shorter than our buffer.
name_len = actual_len = 130
while actual_len == name_len:
    name_len *= 2
    name = create_unicode_buffer(name_len)
    actual_len = kernel32.GetModuleFileNameW(HANDLE(ucrtbased._handle),
                                             name, len(name))
    if not actual_len:
        print('Failed to get full module name.')
        sys.exit(2)

size = version.GetFileVersionInfoSizeW(name, None)
if not size:
    print('Failed to get size of version info.')
    sys.exit(2)

ver_block = c_buffer(size)
if (not version.GetFileVersionInfoW(name, None, size, ver_block) or
    not ver_block):
    print('Failed to get version info.')
    sys.exit(2)

pvi = POINTER(VS_FIXEDFILEINFO)()
if not version.VerQueryValueW(ver_block, "", byref(pvi), byref(DWORD())):
    print('Failed to get version value from info.')
    sys.exit(2)

ver = (
    pvi.contents.dwProductVersionMS >> 16,
    pvi.contents.dwProductVersionMS & 0xFFFF,
    pvi.contents.dwProductVersionLS >> 16,
    pvi.contents.dwProductVersionLS & 0xFFFF,
)

print('{} is version {}.{}.{}.{}'.format(name.value, *ver))

if ver < (10, 0, 10586):
    print('WARN: ucrtbased contains known issues. '
          'Please update the Windows 10 SDK.')
    print('See:')
    print('  http://bugs.python.org/issue27705')
    print('  https://developer.microsoft.com/en-US/windows/downloads/windows-10-sdk')
    sys.exit(1)
