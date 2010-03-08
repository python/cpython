"""This script generates a Python codec module from a Windows Code Page.

It uses the function MultiByteToWideChar to generate a decoding table.
"""

import ctypes
from ctypes import wintypes
from gencodec import codegen
import unicodedata

def genwinmap(codepage):
    MultiByteToWideChar = ctypes.windll.kernel32.MultiByteToWideChar
    MultiByteToWideChar.argtypes = [wintypes.UINT, wintypes.DWORD,
                                    wintypes.LPCSTR, ctypes.c_int,
                                    wintypes.LPWSTR, ctypes.c_int]
    MultiByteToWideChar.restype = ctypes.c_int

    enc2uni = {}

    for i in range(32) + [127]:
        enc2uni[i] = (i, 'CONTROL CHARACTER')

    for i in range(256):
        buf = ctypes.create_unicode_buffer(2)
        ret = MultiByteToWideChar(
            codepage, 0,
            chr(i), 1,
            buf, 2)
        assert ret == 1, "invalid code page"
        assert buf[1] == '\x00'
        try:
            name = unicodedata.name(buf[0])
        except ValueError:
            try:
                name = enc2uni[i][1]
            except KeyError:
                name = ''

        enc2uni[i] = (ord(buf[0]), name)

    return enc2uni

def genwincodec(codepage):
    import platform
    map = genwinmap(codepage)
    encodingname = 'cp%d' % codepage
    code = codegen("", map, encodingname)
    # Replace first lines with our own docstring
    code = '''\
"""Python Character Mapping Codec %s generated on Windows:
%s with the command:
  python Tools/unicode/genwincodec.py %s
"""#"
''' % (encodingname, ' '.join(platform.win32_ver()), codepage
      ) + code.split('"""#"', 1)[1]

    print code

if __name__ == '__main__':
    import sys
    genwincodec(int(sys.argv[1]))
