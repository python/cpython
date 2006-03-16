#! /usr/bin/env python

"""Turn a pile of RCS log output into ChangeLog file entries.

"""

import sys
import string
import re
import getopt
import time

def main():
    args = sys.argv[1:]
    opts, args = getopt.getopt(args, 'p:')
    prefix = ''
    for o, a in opts:
        if p == '-p': prefix = a

    f = sys.stdin
    allrevs = []
    while 1:
        file = getnextfile(f)
        if not file: break
        revs = []
        while 1:
            rev = getnextrev(f, file)
            if not rev:
                break
            revs.append(rev)
        if revs:
            allrevs[len(allrevs):] = revs
    allrevs.sort()
    allrevs.reverse()
    for rev in allrevs:
        formatrev(rev, prefix)

parsedateprog = re.compile(
    '^date: ([0-9]+)/([0-9]+)/([0-9]+) ' +
    '([0-9]+):([0-9]+):([0-9]+);  author: ([^ ;]+)')

authormap = {
    'guido': 'Guido van Rossum  <guido@cnri.reston.va.us>',
    'jack': 'Jack Jansen  <jack@cwi.nl>',
    'sjoerd': 'Sjoerd Mullender  <sjoerd@cwi.nl>',
    }

def formatrev(rev, prefix):
    dateline, file, revline, log = rev
    if parsedateprog.match(dateline) >= 0:
        fields = parsedateprog.group(1, 2, 3, 4, 5, 6)
        author = parsedateprog.group(7)
        if authormap.has_key(author): author = authormap[author]
        tfields = map(string.atoi, fields) + [0, 0, 0]
        tfields[5] = tfields[5] - time.timezone
        t = time.mktime(tuple(tfields))
        print time.ctime(t), '', author
        words = string.split(log)
        words[:0] = ['*', prefix + file + ':']
        maxcol = 72-8
        col = maxcol
        for word in words:
            if col > 0 and col + len(word) >= maxcol:
                print
                print '\t' + word,
                col = -1
            else:
                print word,
            col = col + 1 + len(word)
        print
        print

startprog = re.compile("^Working file: (.*)$")

def getnextfile(f):
    while 1:
        line = f.readline()
        if not line: return None
        if startprog.match(line) >= 0:
            file = startprog.group(1)
            # Skip until first revision
            while 1:
                line = f.readline()
                if not line: return None
                if line[:10] == '='*10: return None
                if line[:10] == '-'*10: break
##              print "Skipped", line,
            return file
##      else:
##          print "Ignored", line,

def getnextrev(f, file):
    # This is called when we are positioned just after a '---' separator
    revline = f.readline()
    dateline = f.readline()
    log = ''
    while 1:
        line = f.readline()
        if not line: break
        if line[:10] == '='*10:
            # Ignore the *last* log entry for each file since it
            # is the revision since which we are logging.
            return None
        if line[:10] == '-'*10: break
        log = log + line
    return dateline, file, revline, log

if __name__ == '__main__':
    main()
