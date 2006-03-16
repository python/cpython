#! /usr/bin/env python

# Convert  MH directories (1 message per file) or MMDF mailboxes (4x^A
# delimited) to unix mailbox (From ... delimited) on stdout.
# If -f is given, files contain one message per file (e.g. MH messages)

import rfc822
import sys
import time
import os
import stat
import getopt
import re

def main():
    dofile = mmdf
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'f')
    except getopt.error, msg:
        sys.stderr.write('%s\n' % msg)
        sys.exit(2)
    for o, a in opts:
        if o == '-f':
            dofile = message
    if not args:
        args = ['-']
    sts = 0
    for arg in args:
        if arg == '-' or arg == '':
            sts = dofile(sys.stdin) or sts
        elif os.path.isdir(arg):
            sts = mh(arg) or sts
        elif os.path.isfile(arg):
            try:
                f = open(arg)
            except IOError, msg:
                sys.stderr.write('%s: %s\n' % (arg, msg))
                sts = 1
                continue
            sts = dofile(f) or sts
            f.close()
        else:
            sys.stderr.write('%s: not found\n' % arg)
            sts = 1
    if sts:
        sys.exit(sts)

numeric = re.compile('[1-9][0-9]*')

def mh(dir):
    sts = 0
    msgs = os.listdir(dir)
    for msg in msgs:
        if numeric.match(msg) != len(msg):
            continue
        fn = os.path.join(dir, msg)
        try:
            f = open(fn)
        except IOError, msg:
            sys.stderr.write('%s: %s\n' % (fn, msg))
            sts = 1
            continue
        sts = message(f) or sts
    return sts

def mmdf(f):
    sts = 0
    while 1:
        line = f.readline()
        if not line:
            break
        if line == '\1\1\1\1\n':
            sts = message(f, line) or sts
        else:
            sys.stderr.write(
                    'Bad line in MMFD mailbox: %r\n' % (line,))
    return sts

counter = 0 # for generating unique Message-ID headers

def message(f, delimiter = ''):
    sts = 0
    # Parse RFC822 header
    m = rfc822.Message(f)
    # Write unix header line
    fullname, email = m.getaddr('From')
    tt = m.getdate('Date')
    if tt:
        t = time.mktime(tt)
    else:
        sys.stderr.write(
                'Unparseable date: %r\n' % (m.getheader('Date'),))
        t = os.fstat(f.fileno())[stat.ST_MTIME]
    print 'From', email, time.ctime(t)
    # Copy RFC822 header
    for line in m.headers:
        print line,
    # Invent Message-ID header if none is present
    if not m.has_key('message-id'):
        global counter
        counter = counter + 1
        msgid = "<%s.%d>" % (hex(t), counter)
        sys.stderr.write("Adding Message-ID %s (From %s)\n" %
                         (msgid, email))
        print "Message-ID:", msgid
    print
    # Copy body
    while 1:
        line = f.readline()
        if line == delimiter:
            break
        if not line:
            sys.stderr.write('Unexpected EOF in message\n')
            sts = 1
            break
        if line[:5] == 'From ':
            line = '>' + line
        print line,
    # Print trailing newline
    print
    return sts

if __name__ == "__main__":
    main()
