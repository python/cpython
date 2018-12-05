#! /usr/bin/env python3

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
    for filename in args:
        treat_file(filename)
    if tags:
        with open('tags', 'w') as fp:
            tags.sort()
            for s in tags: fp.write(s)


expr = r'^[ \t]*(def|class)[ \t]+([a-zA-Z0-9_]+)[ \t]*[:\(]'
matcher = re.compile(expr)

def treat_file(filename):
    try:
        fp = open(filename, 'r')
    except:
        sys.stderr.write('Cannot open %s\n' % filename)
        return
    with fp:
        base = os.path.basename(filename)
        if base[-3:] == '.py':
            base = base[:-3]
        s = base + '\t' + filename + '\t' + '1\n'
        tags.append(s)
        while 1:
            line = fp.readline()
            if not line:
                break
            m = matcher.match(line)
            if m:
                content = m.group(0)
                name = m.group(2)
                s = name + '\t' + filename + '\t/^' + content + '/\n'
                tags.append(s)

if __name__ == '__main__':
    main()
