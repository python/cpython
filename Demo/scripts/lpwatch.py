#! /usr/bin/env python

# Watch line printer queue(s).
# Intended for BSD 4.3 lpq.

import os
import sys
import time

DEF_PRINTER = 'psc'
DEF_DELAY = 10

def main():
    delay = DEF_DELAY # XXX Use getopt() later
    try:
        thisuser = os.environ['LOGNAME']
    except:
        thisuser = os.environ['USER']
    printers = sys.argv[1:]
    if printers:
        # Strip '-P' from printer names just in case
        # the user specified it...
        for i, name in enumerate(printers):
            if name[:2] == '-P':
                printers[i] = name[2:]
    else:
        if os.environ.has_key('PRINTER'):
            printers = [os.environ['PRINTER']]
        else:
            printers = [DEF_PRINTER]

    clearhome = os.popen('clear', 'r').read()

    while True:
        text = clearhome
        for name in printers:
            text += makestatus(name, thisuser) + '\n'
        print text
        time.sleep(delay)

def makestatus(name, thisuser):
    pipe = os.popen('lpq -P' + name + ' 2>&1', 'r')
    lines = []
    users = {}
    aheadbytes = 0
    aheadjobs = 0
    userseen = False
    totalbytes = 0
    totaljobs = 0
    for line in pipe:
        fields = line.split()
        n = len(fields)
        if len(fields) >= 6 and fields[n-1] == 'bytes':
            rank, user, job = fields[0:3]
            files = fields[3:-2]
            bytes = int(fields[n-2])
            if user == thisuser:
                userseen = True
            elif not userseen:
                aheadbytes += bytes
                aheadjobs += 1
            totalbytes += bytes
            totaljobs += 1
            ujobs, ubytes = users.get(user, (0, 0))
            ujobs += 1
            ubytes += bytes
            users[user] = ujobs, ubytes
        else:
            if fields and fields[0] != 'Rank':
                line = line.strip()
                if line == 'no entries':
                    line = name + ': idle'
                elif line[-22:] == ' is ready and printing':
                    line = name
                lines.append(line)

    if totaljobs:
        line = '%d K' % ((totalbytes+1023) // 1024)
        if totaljobs != len(users):
            line += ' (%d jobs)' % totaljobs
        if len(users) == 1:
            line += ' for %s' % (users.keys()[0],)
        else:
            line += ' for %d users' % len(users)
            if userseen:
                if aheadjobs == 0:
                    line += ' (%s first)' % thisuser
                else:
                    line += ' (%d K before %s)' % (
                        (aheadbytes+1023) // 1024, thisuser)
        lines.append(line)

    sts = pipe.close()
    if sts:
        lines.append('lpq exit status %r' % (sts,))
    return ': '.join(lines)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
