#! /usr/bin/env python3

# pdeps
#
# Find dependencies between a bunch of Python modules.
#
# Usage:
#       pdeps file1.py file2.py ...
#
# Output:
# Four tables separated by lines like '--- Closure ---':
# 1) Direct dependencies, listing which module imports which other modules
# 2) The inverse of (1)
# 3) Indirect dependencies, or the closure of the above
# 4) The inverse of (3)
#
# To do:
# - command line options to select output type
# - option to automatically scan the Python library for referenced modules
# - option to limit output to particular modules


import sys
import re
import os


# Main program
#
def main():
    args = sys.argv[1:]
    if not args:
        print('usage: pdeps file.py file.py ...')
        return 2
    #
    table = {}
    for arg in args:
        process(arg, table)
    #
    print('--- Uses ---')
    printresults(table)
    #
    print('--- Used By ---')
    inv = inverse(table)
    printresults(inv)
    #
    print('--- Closure of Uses ---')
    reach = closure(table)
    printresults(reach)
    #
    print('--- Closure of Used By ---')
    invreach = inverse(reach)
    printresults(invreach)
    #
    return 0


# Compiled regular expressions to search for import statements
#
m_import = re.compile('^[ \t]*from[ \t]+([^ \t]+)[ \t]+')
m_from = re.compile('^[ \t]*import[ \t]+([^#]+)')


# Collect data from one file
#
def process(filename, table):
    with open(filename, encoding='utf-8') as fp:
        mod = os.path.basename(filename)
        if mod[-3:] == '.py':
            mod = mod[:-3]
        table[mod] = list = []
        while 1:
            line = fp.readline()
            if not line: break
            while line[-1:] == '\\':
                nextline = fp.readline()
                if not nextline: break
                line = line[:-1] + nextline
            m_found = m_import.match(line) or m_from.match(line)
            if m_found:
                (a, b), (a1, b1) = m_found.regs[:2]
            else: continue
            words = line[a1:b1].split(',')
            # print '#', line, words
            for word in words:
                word = word.strip()
                if word not in list:
                    list.append(word)


# Compute closure (this is in fact totally general)
#
def closure(table):
    modules = list(table.keys())
    #
    # Initialize reach with a copy of table
    #
    reach = {}
    for mod in modules:
        reach[mod] = table[mod][:]
    #
    # Iterate until no more change
    #
    change = 1
    while change:
        change = 0
        for mod in modules:
            for mo in reach[mod]:
                if mo in modules:
                    for m in reach[mo]:
                        if m not in reach[mod]:
                            reach[mod].append(m)
                            change = 1
    #
    return reach


# Invert a table (this is again totally general).
# All keys of the original table are made keys of the inverse,
# so there may be empty lists in the inverse.
#
def inverse(table):
    inv = {}
    for key in table.keys():
        if key not in inv:
            inv[key] = []
        for item in table[key]:
            store(inv, item, key)
    return inv


# Store "item" in "dict" under "key".
# The dictionary maps keys to lists of items.
# If there is no list for the key yet, it is created.
#
def store(dict, key, item):
    if key in dict:
        dict[key].append(item)
    else:
        dict[key] = [item]


# Tabulate results neatly
#
def printresults(table):
    modules = sorted(table.keys())
    maxlen = 0
    for mod in modules: maxlen = max(maxlen, len(mod))
    for mod in modules:
        list = sorted(table[mod])
        print(mod.ljust(maxlen), ':', end=' ')
        if mod in list:
            print('(*)', end=' ')
        for ref in list:
            print(ref, end=' ')
        print()


# Call main and honor exit status
if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(1)
