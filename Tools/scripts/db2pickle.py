#!/usr/bin/env python

"""
Synopsis: %(prog)s [-h|-b|-r] dbfile [ picklefile ]

Convert the bsddb database file given on the command like to a pickle
representation.  The optional flags indicate the type of the database (hash,
btree, recno).  The default is hash.  If a pickle file is named it is opened
for write access (deleting any existing data).  If no pickle file is named,
the pickle output is written to standard output.

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
        pfile = sys.stdout
    else:
        dbfile = args[0]
        try:
            pfile = file(args[1], 'wb')
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
        db = dbopen(dbfile, 'r')
    except bsddb.error:
        print >> sys.stderr, "Unable to open", dbfile,
        print >> sys.stderr, "Check for format or version mismatch."
        return 1
    
    for k in db.keys():
        pickle.dump((k, db[k]), pfile, 1==1)

    db.close()
    pfile.close()

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
