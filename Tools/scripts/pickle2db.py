#!/usr/bin/env python

"""
Synopsis: %(prog)s [-h|-b|-r|-a|-d] dbfile [ picklefile ]

Read the given picklefile as a series of key/value pairs and write to a new
bsddb database.  If the database already exists, any contents are deleted.
The optional flags indicate the type of the database (bsddb hash, bsddb
btree, bsddb recno, anydbm, dbm).  The default is hash.  If a pickle file is
named it is opened for read access.  If no pickle file is named, the pickle
input is read from standard input.

Note that recno databases can only contain numeric keys, so you can't dump a
hash or btree database using bsddb2pickle.py and reconstitute it to a recno
database with %(prog)s.
"""

import getopt
try:
    import bsddb
except ImportError:
    bsddb = None
try:
    import dbm
except ImportError:
    dbm = None
try:
    import anydbm
except ImportError:
    anydbm = None
import sys
try:
    import cPickle as pickle
except ImportError:
    import pickle

prog = sys.argv[0]

def usage():
   print >> sys.stderr, __doc__ % globals()

def main(args):
    try:
        opts, args = getopt.getopt(args, "hbrda",
                                   ["hash", "btree", "recno", "dbm", "anydbm"])
    except getopt.error:
        usage()
        return 1

    if len(args) == 0 or len(args) > 2:
        usage()
        return 1
    elif len(args) == 1:
        dbfile = args[0]
        pfile = sys.stdin
    else:
        dbfile = args[0]
        try:
            pfile = file(args[1], 'rb')
        except IOError:
            print >> sys.stderr, "Unable to open", args[1]
            return 1

    dbopen = None
    for opt, arg in opts:
        if opt in ("-h", "--hash"):
            try:
                dbopen = bsddb.hashopen
            except AttributeError:
                print >> sys.stderr, "bsddb module unavailable."
                return 1
        elif opt in ("-b", "--btree"):
            try:
                dbopen = bsddb.btopen
            except AttributeError:
                print >> sys.stderr, "bsddb module unavailable."
                return 1
        elif opt in ("-r", "--recno"):
            try:
                dbopen = bsddb.rnopen
            except AttributeError:
                print >> sys.stderr, "bsddb module unavailable."
                return 1
        elif opt in ("-a", "--anydbm"):
            try:
                dbopen = anydbm.open
            except AttributeError:
                print >> sys.stderr, "anydbm module unavailable."
                return 1
        elif opt in ("-d", "--dbm"):
            try:
                dbopen = dbm.open
            except AttributeError:
                print >> sys.stderr, "dbm module unavailable."
                return 1
    if dbopen is None:
        if bsddb is None:
            print >> sys.stderr, "bsddb module unavailable -"
            print >> sys.stderr, "must specify dbtype."
            return 1
        else:
            dbopen = bsddb.hashopen

    try:
        db = dbopen(dbfile, 'c')
    except bsddb.error:
        print >> sys.stderr, "Unable to open", dbfile,
        print >> sys.stderr, "Check for format or version mismatch."
        return 1
    else:
        for k in db.keys():
            del db[k]

    while 1:
        try:
            (key, val) = pickle.load(pfile)
        except EOFError:
            break
        db[key] = val

    db.close()
    pfile.close()

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
