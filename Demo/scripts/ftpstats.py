#! /usr/bin/env python

# Extract statistics from ftp daemon log.

# Usage:
# ftpstats [-m maxitems] [-s search] [file]
# -m maxitems: restrict number of items in "top-N" lists, default 25.
# -s string:   restrict statistics to lines containing this string.
# Default file is /usr/adm/ftpd;  a "-" means read standard input.

# The script must be run on the host where the ftp daemon runs.
# (At CWI this is currently buizerd.)

import os
import sys
import re
import string
import getopt

pat = '^([a-zA-Z0-9 :]*)!(.*)!(.*)!([<>].*)!([0-9]+)!([0-9]+)$'
prog = re.compile(pat)

def main():
    maxitems = 25
    search = None
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'm:s:')
    except getopt.error as msg:
        print(msg)
        print('usage: ftpstats [-m maxitems] [file]')
        sys.exit(2)
    for o, a in opts:
        if o == '-m':
            maxitems = string.atoi(a)
        if o == '-s':
            search = a
    file = '/usr/adm/ftpd'
    if args: file = args[0]
    if file == '-':
        f = sys.stdin
    else:
        try:
            f = open(file, 'r')
        except IOError as msg:
            print(file, ':', msg)
            sys.exit(1)
    bydate = {}
    bytime = {}
    byfile = {}
    bydir = {}
    byhost = {}
    byuser = {}
    bytype = {}
    lineno = 0
    try:
        while 1:
            line = f.readline()
            if not line: break
            lineno = lineno + 1
            if search and string.find(line, search) < 0:
                continue
            if prog.match(line) < 0:
                print('Bad line', lineno, ':', repr(line))
                continue
            items = prog.group(1, 2, 3, 4, 5, 6)
            (logtime, loguser, loghost, logfile, logbytes,
             logxxx2) = items
##                      print logtime
##                      print '-->', loguser
##                      print '--> -->', loghost
##                      print '--> --> -->', logfile
##                      print '--> --> --> -->', logbytes
##                      print '--> --> --> --> -->', logxxx2
##                      for i in logtime, loghost, logbytes, logxxx2:
##                              if '!' in i: print '???', i
            add(bydate, logtime[-4:] + ' ' + logtime[:6], items)
            add(bytime, logtime[7:9] + ':00-59', items)
            direction, logfile = logfile[0], logfile[1:]
            # The real path probably starts at the last //...
            while 1:
                i = string.find(logfile, '//')
                if i < 0: break
                logfile = logfile[i+1:]
            add(byfile, logfile + ' ' + direction, items)
            logdir = os.path.dirname(logfile)
##              logdir = os.path.normpath(logdir) + '/.'
            while 1:
                add(bydir, logdir + ' ' + direction, items)
                dirhead = os.path.dirname(logdir)
                if dirhead == logdir: break
                logdir = dirhead
            add(byhost, loghost, items)
            add(byuser, loguser, items)
            add(bytype, direction, items)
    except KeyboardInterrupt:
        print('Interrupted at line', lineno)
    show(bytype, 'by transfer direction', maxitems)
    show(bydir, 'by directory', maxitems)
    show(byfile, 'by file', maxitems)
    show(byhost, 'by host', maxitems)
    show(byuser, 'by user', maxitems)
    showbar(bydate, 'by date')
    showbar(bytime, 'by time of day')

def showbar(dict, title):
    n = len(title)
    print('='*((70-n)/2), title, '='*((71-n)/2))
    list = []
    keys = list(dict.keys())
    keys.sort()
    for key in keys:
        n = len(str(key))
        list.append((len(dict[key]), key))
    maxkeylength = 0
    maxcount = 0
    for count, key in list:
        maxkeylength = max(maxkeylength, len(key))
        maxcount = max(maxcount, count)
    maxbarlength = 72 - maxkeylength - 7
    for count, key in list:
        barlength = int(round(maxbarlength*float(count)/maxcount))
        bar = '*'*barlength
        print('%5d %-*s %s' % (count, maxkeylength, key, bar))

def show(dict, title, maxitems):
    if len(dict) > maxitems:
        title = title + ' (first %d)'%maxitems
    n = len(title)
    print('='*((70-n)/2), title, '='*((71-n)/2))
    list = []
    keys = list(dict.keys())
    for key in keys:
        list.append((-len(dict[key]), key))
    list.sort()
    for count, key in list[:maxitems]:
        print('%5d %s' % (-count, key))

def add(dict, key, item):
    if key in dict:
        dict[key].append(item)
    else:
        dict[key] = [item]

if __name__ == "__main__":
    main()
