#!/usr/bin/env python3
""" Utility for parsing HTML entity definitions available from:

      http://www.w3.org/ as e.g.
      http://www.w3.org/TR/REC-html40/HTMLlat1.ent

    Input is read from stdin, output is written to stdout in form of a
    Python snippet defining a dictionary "entitydefs" mapping literal
    entity name to character or numeric entity.

    Marc-Andre Lemburg, mal@lemburg.com, 1999.
    Use as you like. NO WARRANTIES.

"""
import re,sys

entityRE = re.compile(r'<!ENTITY +(\w+) +CDATA +"([^"]+)" +-- +((?:.|\n)+?) *-->')

def parse(text,pos=0,endpos=None):

    pos = 0
    if endpos is None:
        endpos = len(text)
    d = {}
    while 1:
        m = entityRE.search(text,pos,endpos)
        if not m:
            break
        name,charcode,comment = m.groups()
        d[name] = charcode,comment
        pos = m.end()
    return d

def writefile(f,defs):

    f.write("entitydefs = {\n")
    items = sorted(defs.items())
    for name, (charcode,comment) in items:
        if charcode[:2] == '&#':
            code = int(charcode[2:-1])
            if code < 256:
                charcode = r"'\%o'" % code
            else:
                charcode = repr(charcode)
        else:
            charcode = repr(charcode)
        comment = ' '.join(comment.split())
        f.write("    '%s':\t%s,  \t# %s\n" % (name,charcode,comment))
    f.write('\n}\n')

if __name__ == '__main__':
    if len(sys.argv) > 1:
        infile = open(sys.argv[1])
    else:
        infile = sys.stdin
    if len(sys.argv) > 2:
        outfile = open(sys.argv[2],'w')
    else:
        outfile = sys.stdout
    text = infile.read()
    defs = parse(text)
    writefile(outfile,defs)
