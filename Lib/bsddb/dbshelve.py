#!/bin/env python
#------------------------------------------------------------------------
#           Copyright (c) 1997-2001 by Total Control Software
#                         All Rights Reserved
#------------------------------------------------------------------------
#
# Module Name:  dbShelve.py
#
# Description:  A reimplementation of the standard shelve.py that
#               forces the use of cPickle, and DB.
#
# Creation Date:    11/3/97 3:39:04PM
#
# License:      This is free software.  You may use this software for any
#               purpose including modification/redistribution, so long as
#               this header remains intact and that you do not claim any
#               rights of ownership or authorship of this software.  This
#               software has been tested, but no warranty is expressed or
#               implied.
#
# 13-Dec-2000:  Updated to be used with the new bsddb3 package.
#               Added DBShelfCursor class.
#
#------------------------------------------------------------------------

"""Manage shelves of pickled objects using bsddb database files for the
storage.
"""

#------------------------------------------------------------------------

import pickle
try:
    from UserDict import DictMixin
except ImportError:
    # DictMixin is new in Python 2.3
    class DictMixin: pass
from . import db

_unspecified = object()

#------------------------------------------------------------------------


def open(filename, flags=db.DB_CREATE, mode=0o660, filetype=db.DB_HASH,
         dbenv=None, dbname=None):
    """
    A simple factory function for compatibility with the standard
    shleve.py module.  It can be used like this, where key is a string
    and data is a pickleable object:

        from bsddb import dbshelve
        db = dbshelve.open(filename)

        db[key] = data

        db.close()
    """
    if type(flags) == type(''):
        sflag = flags
        if sflag == 'r':
            flags = db.DB_RDONLY
        elif sflag == 'rw':
            flags = 0
        elif sflag == 'w':
            flags =  db.DB_CREATE
        elif sflag == 'c':
            flags =  db.DB_CREATE
        elif sflag == 'n':
            flags = db.DB_TRUNCATE | db.DB_CREATE
        else:
            raise db.DBError("flags should be one of 'r', 'w', 'c' or 'n' or use the bsddb.db.DB_* flags")

    d = DBShelf(dbenv)
    d.open(filename, dbname, filetype, flags, mode)
    return d

#---------------------------------------------------------------------------

class DBShelf(DictMixin):
    """A shelf to hold pickled objects, built upon a bsddb DB object.  It
    automatically pickles/unpickles data objects going to/from the DB.
    """
    def __init__(self, dbenv=None):
        self.db = db.DB(dbenv)
        self.binary = 1


    def __del__(self):
        self.close()


    def __getattr__(self, name):
        """Many methods we can just pass through to the DB object.
        (See below)
        """
        return getattr(self.db, name)


    #-----------------------------------
    # Dictionary access methods

    def __len__(self):
        return len(self.db)


    def __getitem__(self, key):
        data = self.db[key]
        return pickle.loads(data)


    def __setitem__(self, key, value):
        data = pickle.dumps(value, self.binary)
        self.db[key] = data


    def __delitem__(self, key):
        del self.db[key]


    def keys(self, txn=None):
        if txn != None:
            return self.db.keys(txn)
        else:
            return self.db.keys()


    def items(self, txn=None):
        if txn != None:
            items = self.db.items(txn)
        else:
            items = self.db.items()
        newitems = []

        for k, v in items:
            newitems.append( (k, pickle.loads(v)) )
        return newitems

    def values(self, txn=None):
        if txn != None:
            values = self.db.values(txn)
        else:
            values = self.db.values()

        return map(pickle.loads, values)

    #-----------------------------------
    # Other methods

    def __append(self, value, txn=None):
        data = pickle.dumps(value, self.binary)
        return self.db.append(data, txn)

    def append(self, value, txn=None):
        if self.get_type() != db.DB_RECNO:
            self.append = self.__append
            return self.append(value, txn=txn)
        raise db.DBError("append() only supported when dbshelve opened with filetype=dbshelve.db.DB_RECNO")


    def associate(self, secondaryDB, callback, flags=0):
        def _shelf_callback(priKey, priData, realCallback=callback):
            data = pickle.loads(priData)
            return realCallback(priKey, data)
        return self.db.associate(secondaryDB, _shelf_callback, flags)


    def get(self, key, default=_unspecified, txn=None, flags=0):
        # If no default is given, we must not pass one to the
        # extension module, so that an exception can be raised if
        # set_get_returns_none is turned off.
        if default is _unspecified:
            data = self.db.get(key, txn=txn, flags=flags)
            # if this returns, the default value would be None
            default = None
        else:
            data = self.db.get(key, default, txn=txn, flags=flags)
        if data is default:
            return data
        return pickle.loads(data)

    def get_both(self, key, value, txn=None, flags=0):
        data = pickle.dumps(value, self.binary)
        data = self.db.get(key, data, txn, flags)
        return pickle.loads(data)


    def cursor(self, txn=None, flags=0):
        c = DBShelfCursor(self.db.cursor(txn, flags))
        c.binary = self.binary
        return c


    def put(self, key, value, txn=None, flags=0):
        data = pickle.dumps(value, self.binary)
        return self.db.put(key, data, txn, flags)


    def join(self, cursorList, flags=0):
        raise NotImplementedError


    def __contains__(self, key):
        return self.db.has_key(key)


    #----------------------------------------------
    # Methods allowed to pass-through to self.db
    #
    #    close,  delete, fd, get_byteswapped, get_type, has_key,
    #    key_range, open, remove, rename, stat, sync,
    #    upgrade, verify, and all set_* methods.


#---------------------------------------------------------------------------

class DBShelfCursor:
    """
    """
    def __init__(self, cursor):
        self.dbc = cursor

    def __del__(self):
        self.close()


    def __getattr__(self, name):
        """Some methods we can just pass through to the cursor object.  (See below)"""
        return getattr(self.dbc, name)


    #----------------------------------------------

    def dup(self, flags=0):
        return DBShelfCursor(self.dbc.dup(flags))


    def put(self, key, value, flags=0):
        data = pickle.dumps(value, self.binary)
        return self.dbc.put(key, data, flags)


    def get(self, *args):
        count = len(args)  # a method overloading hack
        method = getattr(self, 'get_%d' % count)
        method(*args)

    def get_1(self, flags):
        rec = self.dbc.get(flags)
        return self._extract(rec)

    def get_2(self, key, flags):
        rec = self.dbc.get(key, flags)
        return self._extract(rec)

    def get_3(self, key, value, flags):
        data = pickle.dumps(value, self.binary)
        rec = self.dbc.get(key, flags)
        return self._extract(rec)


    def current(self, flags=0): return self.get_1(flags|db.DB_CURRENT)
    def first(self, flags=0): return self.get_1(flags|db.DB_FIRST)
    def last(self, flags=0): return self.get_1(flags|db.DB_LAST)
    def next(self, flags=0): return self.get_1(flags|db.DB_NEXT)
    def prev(self, flags=0): return self.get_1(flags|db.DB_PREV)
    def consume(self, flags=0): return self.get_1(flags|db.DB_CONSUME)
    def next_dup(self, flags=0): return self.get_1(flags|db.DB_NEXT_DUP)
    def next_nodup(self, flags=0): return self.get_1(flags|db.DB_NEXT_NODUP)
    def prev_nodup(self, flags=0): return self.get_1(flags|db.DB_PREV_NODUP)


    def get_both(self, key, value, flags=0):
        data = pickle.dumps(value, self.binary)
        rec = self.dbc.get_both(key, flags)
        return self._extract(rec)


    def set(self, key, flags=0):
        rec = self.dbc.set(key, flags)
        return self._extract(rec)

    def set_range(self, key, flags=0):
        rec = self.dbc.set_range(key, flags)
        return self._extract(rec)

    def set_recno(self, recno, flags=0):
        rec = self.dbc.set_recno(recno, flags)
        return self._extract(rec)

    set_both = get_both

    def _extract(self, rec):
        if rec is None:
            return None
        else:
            key, data = rec
            return key, pickle.loads(data)

    #----------------------------------------------
    # Methods allowed to pass-through to self.dbc
    #
    # close, count, delete, get_recno, join_item


#---------------------------------------------------------------------------
