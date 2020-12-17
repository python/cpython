#!/usr/bin/env python3

"""
Synopsis: %(prog)s [-h|-g|-b|-r|-a] dbfile [ picklefile ]

Convert the database file given on the command line to a pickle
representation.  The optional flags indicate the type of the database:

    -a - open using dbm (any supported format)
    -b - open as bsddb btree file
    -d - open as dbm file
    -g - open as gdbm file
    -h - open as bsddb hash file
    -r - open as bsddb recno file

The default is hash.  If a pickle file is named it is opened for write
access (deleting any existing data).  If no pickle file is named, the pickle
output is written to standard output.

"""

import getopt
try:
    import bsddb
except ImportError:
    bsddb = None
try:
    import dbm.ndbm as dbm
except ImportError:
    dbm = None
try:
    import dbm.gnu as gdbm
except ImportError:
    gdbm = None
try:
    import dbm.ndbm as anydbm
except ImportError:
    anydbm = None
import sys
try:
    import pickle as pickle
except ImportError:
    import pickle

prog = sys.argv[0]

def usage():
    sys.stderr.write(__doc__ % globals())

def main(args):
    try:
        opts, args = getopt.getopt(args, "hbrdag",
                                   ["hash", "btree", "recno", "dbm",
                                    "gdbm", "anydbm"])
    except getopt.error:
        usage()
        return 1

    if len(args) == 0 or len(args) > 2:
        usage()
        return 1
    elif len(args) == 1:
        dbfile = args[0]
        pfile = sys.stdout
    else:
        dbfile = args[0]
        try:
            pfile = open(args[1], 'wb')
        except IOError:
            sys.stderr.write("Unable to open %s\n" % args[1])
            return 1

    dbopen = None
    for opt, arg in opts:
        if opt in ("-h", "--hash"):
            try:
                dbopen = bsddb.hashopen
            except AttributeError:
                sys.stderr.write("bsddb module unavailable.\n")
                return 1
        elif opt in ("-b", "--btree"):
            try:
                dbopen = bsddb.btopen
            except AttributeError:
                sys.stderr.write("bsddb module unavailable.\n")
                return 1
        elif opt in ("-r", "--recno"):
            try:
                dbopen = bsddb.rnopen
            except AttributeError:
                sys.stderr.write("bsddb module unavailable.\n")
                return 1
        elif opt in ("-a", "--anydbm"):
            try:
                dbopen = anydbm.open
            except AttributeError:
                sys.stderr.write("dbm module unavailable.\n")
                return 1
        elif opt in ("-g", "--gdbm"):
            try:
                dbopen = gdbm.open
            except AttributeError:
                sys.stderr.write("dbm.gnu module unavailable.\n")
                return 1
        elif opt in ("-d", "--dbm"):
            try:
                dbopen = dbm.open
            except AttributeError:
                sys.stderr.write("dbm.ndbm module unavailable.\n")
                return 1
    if dbopen is None:
        if bsddb is None:
            sys.stderr.write("bsddb module unavailable - ")
            sys.stderr.write("must specify dbtype.\n")
            return 1
        else:
            dbopen = bsddb.hashopen

    try:
        db = dbopen(dbfile, 'r')
    except bsddb.error:
        sys.stderr.write("Unable to open %s.  " % dbfile)
        sys.stderr.write("Check for format or version mismatch.\n")
        return 1

    for k in db.keys():
        pickle.dump((k, db[k]), pfile, 1==1)

    db.close()
    pfile.close()

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
