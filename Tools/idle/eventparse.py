#! /usr/bin/env python

"""Parse event definitions out of comments in source files."""

import re
import sys
import os
import string
import getopt
import glob
import fileinput
import pprint

def main():
    hits = []
    sublist = []
    args = sys.argv[1:]
    if not args:
        args = filter(lambda s: 'A' <= s[0] <= 'Z', glob.glob("*.py"))
        if not args:
            print "No arguments, no [A-Z]*.py files."
            return 1
    for line in fileinput.input(args):
        if line[:2] == '#$':
            if not sublist:
                sublist.append('file %s' % fileinput.filename())
                sublist.append('line %d' % fileinput.lineno())
            sublist.append(string.strip(line[2:-1]))
        else:
            if sublist:
                hits.append(sublist)
                sublist = []
    if sublist:
        hits.append(sublist)
        sublist = []
    dd = {}
    for sublist in hits:
        d = {}
        for line in sublist:
            words = string.split(line, None, 1)
            if len(words) != 2:
                continue
            tag = words[0]
            l = d.get(tag, [])
            l.append(words[1])
            d[tag] = l
        if d.has_key('event'):
            keys = d['event']
            if len(keys) != 1:
                print "Multiple event keys in", d
                print 'File "%s", line %d' % (d['file'], d['line'])
            key = keys[0]
            if dd.has_key(key):
                print "Duplicate event in", d
                print 'File "%s", line %d' % (d['file'], d['line'])
                return
            dd[key] = d
        else:
            print "No event key in", d
            print 'File "%s", line %d' % (d['file'], d['line'])
    winevents = getevents(dd, "win")
    unixevents = getevents(dd, "unix")
    save = sys.stdout
    f = open("keydefs.py", "w")
    try:
        sys.stdout = f
        print "windows_keydefs = \\"
        pprint.pprint(winevents)
        print
        print "unix_keydefs = \\"
        pprint.pprint(unixevents)
    finally:
        sys.stdout = save
    f.close()

def getevents(dd, key):
    res = {}
    events = dd.keys()
    events.sort()
    for e in events:
        d = dd[e]
        if d.has_key(key) or d.has_key("all"):
            list = []
            for x in d.get(key, []) + d.get("all", []):
                list.append(x)
                if key == "unix" and x[:5] == "<Alt-":
                    x = "<Meta-" + x[5:]
                    list.append(x)
            res[e] = list
    return res

if __name__ == '__main__':
    sys.exit(main())
