#! /usr/bin/env python

# ptags
#
# Create a tags file for Python programs, usable with vi.
# Tagged are:
# - functions (even inside other defs or classes)
# - classes
# - filenames
# Warns about files it cannot open.
# No warnings about duplicate tags.

import sys, re, os

tags = []    # Modified global variable!

def main():
    args = sys.argv[1:]
    for file in args: treat_file(file)
    if tags:
        fp = open('tags', 'w')
        tags.sort()
        for s in tags: fp.write(s)


expr = '^[ \t]*(def|class)[ \t]+([a-zA-Z0-9_]+)[ \t]*[:\(]'
matcher = re.compile(expr)

def treat_file(file):
    try:
        fp = open(file, 'r')
    except:
        sys.stderr.write('Cannot open %s\n' % file)
        return
    base = os.path.basename(file)
    if base[-3:] == '.py':
        base = base[:-3]
    s = base + '\t' + file + '\t' + '1\n'
    tags.append(s)
    while 1:
        line = fp.readline()
        if not line: 
            break
        m = matcher.match(line)
        if m:
            content = m.group(0)
            name = m.group(2)
            s = name + '\t' + file + '\t/^' + content + '/\n'
            tags.append(s)

main()
