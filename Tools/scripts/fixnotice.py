#! /usr/bin/env python3

"""(Ostensibly) fix copyright notices in files.

Actually, this script will simply replace a block of text in a file from one
string to another.  It will only do this once though, i.e. not globally
throughout the file.  It writes a backup file and then does an os.rename()
dance for atomicity.

Usage: fixnotices.py [options] [filenames]
Options:
    -h / --help
        Print this message and exit

    --oldnotice=file
        Use the notice in the file as the old (to be replaced) string, instead
        of the hard coded value in the script.

    --newnotice=file
        Use the notice in the file as the new (replacement) string, instead of
        the hard coded value in the script.

    --dry-run
        Don't actually make the changes, but print out the list of files that
        would change.  When used with -v, a status will be printed for every
        file.

    -v / --verbose
        Print a message for every file looked at, indicating whether the file
        is changed or not.
"""

OLD_NOTICE = """/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/
"""
import os
import sys
import getopt

NEW_NOTICE = ""
DRYRUN = 0
VERBOSE = 0


def usage(code, msg=''):
    print(__doc__ % globals())
    if msg:
        print(msg)
    sys.exit(code)


def main():
    global DRYRUN, OLD_NOTICE, NEW_NOTICE, VERBOSE
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hv',
                                   ['help', 'oldnotice=', 'newnotice=',
                                    'dry-run', 'verbose'])
    except getopt.error as msg:
        usage(1, msg)

    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage(0)
        elif opt in ('-v', '--verbose'):
            VERBOSE = 1
        elif opt == '--dry-run':
            DRYRUN = 1
        elif opt == '--oldnotice':
            fp = open(arg)
            OLD_NOTICE = fp.read()
            fp.close()
        elif opt == '--newnotice':
            fp = open(arg)
            NEW_NOTICE = fp.read()
            fp.close()

    for arg in args:
        process(arg)


def process(file):
    f = open(file)
    data = f.read()
    f.close()
    i = data.find(OLD_NOTICE)
    if i < 0:
        if VERBOSE:
            print('no change:', file)
        return
    elif DRYRUN or VERBOSE:
        print('   change:', file)
    if DRYRUN:
        # Don't actually change the file
        return
    data = data[:i] + NEW_NOTICE + data[i+len(OLD_NOTICE):]
    new = file + ".new"
    backup = file + ".bak"
    f = open(new, "w")
    f.write(data)
    f.close()
    os.rename(file, backup)
    os.rename(new, file)


if __name__ == '__main__':
    main()
