"""C accelerator wrappers for originally pure-Python parts of base64."""

from binascii import a2b_ascii85, a2b_base85, b2a_ascii85, b2a_base85

__all__ = ['a85encode', 'a85decode',
           'b85encode', 'b85decode',
           'z85encode', 'z85decode']


def a85encode(b, *, foldspaces=False, wrapcol=0, pad=False, adobe=False):
    return b2a_ascii85(b, fold_spaces=foldspaces,
                       wrap=adobe, width=wrapcol, pad=pad)


def a85decode(b, *, foldspaces=False, adobe=False, ignorechars=b' \t\n\r\v'):
    return a2b_ascii85(b, fold_spaces=foldspaces,
                       wrap=adobe, ignore=ignorechars)


def b85encode(b, pad=False):
    return b2a_base85(b, pad=pad, newline=False)


def b85decode(b):
    return a2b_base85(b, strict_mode=True)


def z85encode(s):
    return b2a_base85(s, newline=False, z85=True)


def z85decode(s):
    return a2b_base85(s, strict_mode=True, z85=True)
