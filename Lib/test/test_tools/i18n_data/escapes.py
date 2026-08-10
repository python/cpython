import gettext as _


# Special characters that are always escaped in the POT file
_('"\t\n\r\\')

# All ascii characters 0-31
_('\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n'
  '\x0b\x0c\r\x0e\x0f\x10\x11\x12\x13\x14\x15'
  '\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f')

# All ascii characters 32-126
_(' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ'
  '[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~')

# ascii char 127
_('\x7f')

# some characters in the 128-255 range
_('\x80 \xa0 ÿ')

# some characters >= 256 encoded as 2, 3 and 4 bytes, respectively
_('α ㄱ 𓂀')
