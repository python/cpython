#! /usr/bin/env python

"""Conversions to/from quoted-printable transport encoding as per RFC 1521."""

# (Dec 1991 version).

__all__ = ["encode", "decode", "encodestring", "decodestring"]

ESCAPE = '='
MAXLINESIZE = 76
HEX = '0123456789ABCDEF'
EMPTYSTRING = ''



def needsquoting(c, quotetabs):
    """Decide whether a particular character needs to be quoted.

    The 'quotetabs' flag indicates whether embedded tabs and spaces should be
    quoted.  Note that line-ending tabs and spaces are always encoded, as per
    RFC 1521.
    """
    if c in ' \t':
        return quotetabs
    return c == ESCAPE or not (' ' <= c <= '~')

def quote(c):
    """Quote a single character."""
    i = ord(c)
    return ESCAPE + HEX[i/16] + HEX[i%16]



def encode(input, output, quotetabs):
    """Read 'input', apply quoted-printable encoding, and write to 'output'.

    'input' and 'output' are files with readline() and write() methods.
    The 'quotetabs' flag indicates whether embedded tabs and spaces should be
    quoted.  Note that line-ending tabs and spaces are always encoded, as per
    RFC 1521.
    """
    def write(s, output=output, lineEnd='\n'):
        # RFC 1521 requires that the line ending in a space or tab must have
        # that trailing character encoded.
        if s and s[-1:] in ' \t':
            output.write(s[:-1] + quote(s[-1]) + lineEnd)
        else:
            output.write(s + lineEnd)

    prevline = None
    linelen = 0
    while 1:
        line = input.readline()
        if not line:
            break
        outline = []
        # Strip off any readline induced trailing newline
        stripped = ''
        if line[-1:] == '\n':
            line = line[:-1]
            stripped = '\n'
        for c in line:
            if needsquoting(c, quotetabs):
                c = quote(c)
            # Have we hit the RFC 1521 encoded line maximum?
            if linelen + len(c) >= MAXLINESIZE:
                # Write out the previous line
                if prevline is not None:
                    write(prevline)
                prevline = EMPTYSTRING.join(outline)
                linelen = 0
                outline = []
            outline.append(c)
            linelen += len(c)
        # Write out the current line
        if prevline is not None:
            write(prevline)
        prevline = EMPTYSTRING.join(outline)
        linelen = 0
        outline = []
    # Write out the last line, without a trailing newline
    if prevline is not None:
        write(prevline, lineEnd=stripped)

def encodestring(s, quotetabs=0):
    from cStringIO import StringIO
    infp = StringIO(s)
    outfp = StringIO()
    encode(infp, outfp, quotetabs)
    return outfp.getvalue()



def decode(input, output):
    """Read 'input', apply quoted-printable decoding, and write to 'output'.

    'input' and 'output' are files with readline() and write() methods."""
    new = ''
    while 1:
        line = input.readline()
        if not line: break
        i, n = 0, len(line)
        if n > 0 and line[n-1] == '\n':
            partial = 0; n = n-1
            # Strip trailing whitespace
            while n > 0 and line[n-1] in " \t\r":
                n = n-1
        else:
            partial = 1
        while i < n:
            c = line[i]
            if c != ESCAPE:
                new = new + c; i = i+1
            elif i+1 == n and not partial:
                partial = 1; break
            elif i+1 < n and line[i+1] == ESCAPE:
                new = new + ESCAPE; i = i+2
            elif i+2 < n and ishex(line[i+1]) and ishex(line[i+2]):
                new = new + chr(unhex(line[i+1:i+3])); i = i+3
            else: # Bad escape sequence -- leave it in
                new = new + c; i = i+1
        if not partial:
            output.write(new + '\n')
            new = ''
    if new:
        output.write(new)

def decodestring(s):
    from cStringIO import StringIO
    infp = StringIO(s)
    outfp = StringIO()
    decode(infp, outfp)
    return outfp.getvalue()



# Other helper functions
def ishex(c):
    """Return true if the character 'c' is a hexadecimal digit."""
    return '0' <= c <= '9' or 'a' <= c <= 'f' or 'A' <= c <= 'F'

def unhex(s):
    """Get the integer value of a hexadecimal number."""
    bits = 0
    for c in s:
        if '0' <= c <= '9':
            i = ord('0')
        elif 'a' <= c <= 'f':
            i = ord('a')-10
        elif 'A' <= c <= 'F':
            i = ord('A')-10
        else:
            break
        bits = bits*16 + (ord(c) - i)
    return bits



def main():
    import sys
    import getopt
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'td')
    except getopt.error, msg:
        sys.stdout = sys.stderr
        print msg
        print "usage: quopri [-t | -d] [file] ..."
        print "-t: quote tabs"
        print "-d: decode; default encode"
        sys.exit(2)
    deco = 0
    tabs = 0
    for o, a in opts:
        if o == '-t': tabs = 1
        if o == '-d': deco = 1
    if tabs and deco:
        sys.stdout = sys.stderr
        print "-t and -d are mutually exclusive"
        sys.exit(2)
    if not args: args = ['-']
    sts = 0
    for file in args:
        if file == '-':
            fp = sys.stdin
        else:
            try:
                fp = open(file)
            except IOError, msg:
                sys.stderr.write("%s: can't open (%s)\n" % (file, msg))
                sts = 1
                continue
        if deco:
            decode(fp, sys.stdout)
        else:
            encode(fp, sys.stdout, tabs)
        if fp is not sys.stdin:
            fp.close()
    if sts:
        sys.exit(sts)



if __name__ == '__main__':
    main()
