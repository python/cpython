#! /usr/bin/env python
"""Create a TAGS file for Python programs, usable with GNU Emacs.

usage: eptags pyfiles...

The output TAGS file is usable with Emacs version 18, 19, 20.
Tagged are:
 - functions (even inside other defs or classes)
 - classes

eptags warns about files it cannot open.
eptags will not give warnings about duplicate tags.

BUGS:
   Because of tag duplication (methods with the same name in different
   classes), TAGS files are not very useful for most object-oriented
   python projects.
"""
import sys,re

expr = r'^[ \t]*(def|class)[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)[ \t]*[:\(]'
matcher = re.compile(expr)

def treat_file(file, outfp):
    """Append tags found in file named 'file' to the open file 'outfp'"""
    try:
        fp = open(file, 'r')
    except:
        sys.stderr.write('Cannot open %s\n'%file)
        return
    charno = 0
    lineno = 0
    tags = []
    size = 0
    while 1:
        line = fp.readline()
        if not line:
            break
        lineno = lineno + 1
        m = matcher.search(line)
        if m:
            tag = m.group(0) + '\177%d,%d\n'%(lineno,charno)
            tags.append(tag)
            size = size + len(tag)
        charno = charno + len(line)
    outfp.write('\f\n%s,%d\n'%(file,size))
    for tag in tags:
        outfp.write(tag)

def main():
    outfp = open('TAGS', 'w')
    for file in sys.argv[1:]:
        treat_file(file, outfp)

if __name__=="__main__":
    main()
