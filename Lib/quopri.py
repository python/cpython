"""Conversions to/from quoted-printable transport encoding as per RFC 1521."""

# (Dec 1991 version).

from binascii import a2b_qp, b2a_qp

__all__ = ["encode", "decode", "encodestring", "decodestring"]

ESCAPE = b'='
MAXLINESIZE = 76
HEX = b'0123456789ABCDEF'
EMPTYSTRING = b''


def needsquoting(c, quotetabs, header):
    """Decide whether a particular byte ordinal needs to be quoted.

    The 'quotetabs' flag indicates whether embedded tabs and spaces should be
    quoted.  Note that line-ending tabs and spaces are always encoded, as per
    RFC 1521.
    """
    assert isinstance(c, bytes)
    if c in b' \t':
        return quotetabs
    # if header, we have to escape _ because _ is used to escape space
    if c == b'_':
        return header
    return c == ESCAPE or not (b' ' <= c <= b'~')

def quote(c):
    """Quote a single character."""
    assert isinstance(c, bytes) and len(c)==1
    c = ord(c)
    return ESCAPE + bytes((HEX[c//16], HEX[c%16]))



def encode(input, output, quotetabs, header=False):
    """Read 'input', apply quoted-printable encoding, and write to 'output'.

    'input' and 'output' are binary file objects. The 'quotetabs' flag
    indicates whether embedded tabs and spaces should be quoted. Note that
    line-ending tabs and spaces are always encoded, as per RFC 1521.
    The 'header' flag indicates whether we are encoding spaces as _ as per RFC
    1522."""
    data = input.read()
    odata = b2a_qp(data, quotetabs=quotetabs, header=header)
    output.write(odata)


def encodestring(s, quotetabs=False, header=False):
    return b2a_qp(s, quotetabs=quotetabs, header=header)


def decode(input, output, header=False):
    """Read 'input', apply quoted-printable decoding, and write to 'output'.
    'input' and 'output' are binary file objects.
    If 'header' is true, decode underscore as space (per RFC 1522)."""
    data = input.read()
    odata = a2b_qp(data, header=header)
    output.write(odata)


def decodestring(s, header=False):
    return a2b_qp(s, header=header)


# Other helper functions
def ishex(c):
    """Return true if the byte ordinal 'c' is a hexadecimal digit in ASCII."""
    assert isinstance(c, bytes)
    return b'0' <= c <= b'9' or b'a' <= c <= b'f' or b'A' <= c <= b'F'

def unhex(s):
    """Get the integer value of a hexadecimal number."""
    bits = 0
    for c in s:
        c = bytes((c,))
        if b'0' <= c <= b'9':
            i = ord('0')
        elif b'a' <= c <= b'f':
            i = ord('a')-10
        elif b'A' <= c <= b'F':
            i = ord(b'A')-10
        else:
            assert False, "non-hex digit "+repr(c)
        bits = bits*16 + (ord(c) - i)
    return bits



def main():
    import sys
    import getopt
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'td')
    except getopt.error as msg:
        sys.stdout = sys.stderr
        print(msg)
        print("usage: quopri [-t | -d] [file] ...")
        print("-t: quote tabs")
        print("-d: decode; default encode")
        sys.exit(2)
    deco = False
    tabs = False
    for o, a in opts:
        if o == '-t': tabs = True
        if o == '-d': deco = True
    if tabs and deco:
        sys.stdout = sys.stderr
        print("-t and -d are mutually exclusive")
        sys.exit(2)
    if not args: args = ['-']
    sts = 0
    for file in args:
        if file == '-':
            fp = sys.stdin.buffer
        else:
            try:
                fp = open(file, "rb")
            except OSError as msg:
                sys.stderr.write("%s: can't open (%s)\n" % (file, msg))
                sts = 1
                continue
        try:
            if deco:
                decode(fp, sys.stdout.buffer)
            else:
                encode(fp, sys.stdout.buffer, tabs)
        finally:
            if file != '-':
                fp.close()
    if sts:
        sys.exit(sts)



if __name__ == '__main__':
    main()
