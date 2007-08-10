#----------------------------------------------------------------------
#  Copyright (c) 1999-2001, Digital Creations, Fredericksburg, VA, USA
#  and Andrew Kuchling. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are
#  met:
#
#    o Redistributions of source code must retain the above copyright
#      notice, this list of conditions, and the disclaimer that follows.
#
#    o Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions, and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#
#    o Neither the name of Digital Creations nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY DIGITAL CREATIONS AND CONTRIBUTORS *AS
#  IS* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
#  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
#  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL DIGITAL
#  CREATIONS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
#  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
#  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
#  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
#  DAMAGE.
#----------------------------------------------------------------------


"""Support for BerkeleyDB 3.3 through 4.4 with a simple interface.

For the full featured object oriented interface use the bsddb.db module
instead.  It mirrors the Sleepycat BerkeleyDB C API.
"""

try:
    if __name__ == 'bsddb3':
        # import _pybsddb binary as it should be the more recent version from
        # a standalone pybsddb addon package than the version included with
        # python as bsddb._bsddb.
        import _pybsddb
        _bsddb = _pybsddb
        from bsddb3.dbutils import DeadlockWrap as _DeadlockWrap
    else:
        import _bsddb
        from bsddb.dbutils import DeadlockWrap as _DeadlockWrap
except ImportError:
    # Remove ourselves from sys.modules
    import sys
    del sys.modules[__name__]
    raise

# bsddb3 calls it db, but provide _db for backwards compatibility
db = _db = _bsddb
__version__ = db.__version__

error = db.DBError  # So bsddb.error will mean something...

#----------------------------------------------------------------------

import sys, os, UserDict
from weakref import ref

class _iter_mixin(UserDict.DictMixin):
    def _make_iter_cursor(self):
        cur = _DeadlockWrap(self.db.cursor)
        key = id(cur)
        self._cursor_refs[key] = ref(cur, self._gen_cref_cleaner(key))
        return cur

    def _gen_cref_cleaner(self, key):
        # use generate the function for the weakref callback here
        # to ensure that we do not hold a strict reference to cur
        # in the callback.
        return lambda ref: self._cursor_refs.pop(key, None)

    def __iter__(self):
        try:
            cur = self._make_iter_cursor()

            # FIXME-20031102-greg: race condition.  cursor could
            # be closed by another thread before this call.

            # since we're only returning keys, we call the cursor
            # methods with flags=0, dlen=0, dofs=0
            key = _DeadlockWrap(cur.first, 0,0,0)[0]
            yield key

            next = cur.next
            while 1:
                try:
                    key = _DeadlockWrap(next, 0,0,0)[0]
                    yield key
                except _bsddb.DBCursorClosedError:
                    cur = self._make_iter_cursor()
                    # FIXME-20031101-greg: race condition.  cursor could
                    # be closed by another thread before this call.
                    _DeadlockWrap(cur.set, key,0,0,0)
                    next = cur.next
        except _bsddb.DBNotFoundError:
            return
        except _bsddb.DBCursorClosedError:
            # the database was modified during iteration.  abort.
            return

    def iteritems(self):
        if not self.db:
            return
        try:
            cur = self._make_iter_cursor()

            # FIXME-20031102-greg: race condition.  cursor could
            # be closed by another thread before this call.

            kv = _DeadlockWrap(cur.first)
            key = kv[0]
            yield kv

            next = cur.next
            while 1:
                try:
                    kv = _DeadlockWrap(next)
                    key = kv[0]
                    yield kv
                except _bsddb.DBCursorClosedError:
                    cur = self._make_iter_cursor()
                    # FIXME-20031101-greg: race condition.  cursor could
                    # be closed by another thread before this call.
                    _DeadlockWrap(cur.set, key,0,0,0)
                    next = cur.next
        except _bsddb.DBNotFoundError:
            return
        except _bsddb.DBCursorClosedError:
            # the database was modified during iteration.  abort.
            return

class _DBWithCursor(_iter_mixin):
    """
    A simple wrapper around DB that makes it look like the bsddbobject in
    the old module.  It uses a cursor as needed to provide DB traversal.
    """
    def __init__(self, db):
        self.db = db
        self.db.set_get_returns_none(0)

        # FIXME-20031101-greg: I believe there is still the potential
        # for deadlocks in a multithreaded environment if someone
        # attempts to use the any of the cursor interfaces in one
        # thread while doing a put or delete in another thread.  The
        # reason is that _checkCursor and _closeCursors are not atomic
        # operations.  Doing our own locking around self.dbc,
        # self.saved_dbc_key and self._cursor_refs could prevent this.
        # TODO: A test case demonstrating the problem needs to be written.

        # self.dbc is a DBCursor object used to implement the
        # first/next/previous/last/set_location methods.
        self.dbc = None
        self.saved_dbc_key = None

        # a collection of all DBCursor objects currently allocated
        # by the _iter_mixin interface.
        self._cursor_refs = {}

    def __del__(self):
        self.close()

    def _checkCursor(self):
        if self.dbc is None:
            self.dbc = _DeadlockWrap(self.db.cursor)
            if self.saved_dbc_key is not None:
                _DeadlockWrap(self.dbc.set, self.saved_dbc_key)
                self.saved_dbc_key = None

    # This method is needed for all non-cursor DB calls to avoid
    # BerkeleyDB deadlocks (due to being opened with DB_INIT_LOCK
    # and DB_THREAD to be thread safe) when intermixing database
    # operations that use the cursor internally with those that don't.
    def _closeCursors(self, save=1):
        if self.dbc:
            c = self.dbc
            self.dbc = None
            if save:
                try:
                    self.saved_dbc_key = _DeadlockWrap(c.current, 0,0,0)[0]
                except db.DBError:
                    pass
            _DeadlockWrap(c.close)
            del c
        for cref in self._cursor_refs.values():
            c = cref()
            if c is not None:
                _DeadlockWrap(c.close)

    def _checkOpen(self):
        if self.db is None:
            raise error, "BSDDB object has already been closed"

    def isOpen(self):
        return self.db is not None

    def __len__(self):
        self._checkOpen()
        return _DeadlockWrap(lambda: len(self.db))  # len(self.db)

    def __getitem__(self, key):
        self._checkOpen()
        return _DeadlockWrap(lambda: self.db[key])  # self.db[key]

    def __setitem__(self, key, value):
        self._checkOpen()
        self._closeCursors()
        def wrapF():
            self.db[key] = value
        _DeadlockWrap(wrapF)  # self.db[key] = value

    def __delitem__(self, key):
        self._checkOpen()
        self._closeCursors()
        def wrapF():
            del self.db[key]
        _DeadlockWrap(wrapF)  # del self.db[key]

    def close(self):
        self._closeCursors(save=0)
        if self.dbc is not None:
            _DeadlockWrap(self.dbc.close)
        v = 0
        if self.db is not None:
            v = _DeadlockWrap(self.db.close)
        self.dbc = None
        self.db = None
        return v

    def keys(self):
        self._checkOpen()
        return _DeadlockWrap(self.db.keys)

    def has_key(self, key):
        self._checkOpen()
        return _DeadlockWrap(self.db.has_key, key)

    __contains__ = has_key

    def set_location(self, key):
        self._checkOpen()
        self._checkCursor()
        return _DeadlockWrap(self.dbc.set_range, key)

    def next(self):
        self._checkOpen()
        self._checkCursor()
        rv = _DeadlockWrap(self.dbc.next)
        return rv

    def previous(self):
        self._checkOpen()
        self._checkCursor()
        rv = _DeadlockWrap(self.dbc.prev)
        return rv

    def first(self):
        self._checkOpen()
        self._checkCursor()
        rv = _DeadlockWrap(self.dbc.first)
        return rv

    def last(self):
        self._checkOpen()
        self._checkCursor()
        rv = _DeadlockWrap(self.dbc.last)
        return rv

    def sync(self):
        self._checkOpen()
        return _DeadlockWrap(self.db.sync)

class _ExposedProperties:
    @property
    def _cursor_refs(self):
        return self.db._cursor_refs

class StringKeys(UserDict.DictMixin, _ExposedProperties):
    """Wrapper around DB object that automatically encodes
    all keys as UTF-8; the keys must be strings."""

    def __init__(self, db):
        self.db = db

    def __len__(self):
        return len(self.db)

    def __getitem__(self, key):
        return self.db[key.encode("utf-8")]

    def __setitem__(self, key, value):
        self.db[key.encode("utf-8")] = value

    def __delitem__(self, key):
        del self.db[key.encode("utf-8")]

    def __iter__(self):
        for k in self.db:
            yield k.decode("utf-8")

    def close(self):
        self.db.close()

    def keys(self):
        for k in self.db.keys():
            yield k.decode("utf-8")

    def has_key(self, key):
        return self.db.has_key(key.encode("utf-8"))

    __contains__ = has_key

    def values(self):
        return self.db.values()

    def items(self):
        for k,v in self.db.items():
            yield k.decode("utf-8"), v

    def set_location(self, key):
        return self.db.set_location(key.encode("utf-8"))

    def next(self):
        key, value = self.db.next()
        return key.decode("utf-8"), value

    def previous(self):
        key, value = self.db.previous()
        return key.decode("utf-8"), value

    def first(self):
        key, value = self.db.first()
        return key.decode("utf-8"), value

    def last(self):
        key, value = self.db.last()
        return key.decode("utf-8"), value

    def set_location(self, key):
        key, value = self.db.set_location(key.encode("utf-8"))
        return key.decode("utf-8"), value

    def sync(self):
        return self.db.sync()

class StringValues(UserDict.DictMixin, _ExposedProperties):
    """Wrapper around DB object that automatically encodes
    all keys as UTF-8; the keys must be strings."""

    def __init__(self, db):
        self.db = db

    def __len__(self):
        return len(self.db)

    def __getitem__(self, key):
        return self.db[key].decode("utf-8")

    def __setitem__(self, key, value):
        self.db[key] = value.encode("utf-8")

    def __delitem__(self, key):
        del self.db[key]

    def __iter__(self):
        return iter(self.db)

    def close(self):
        self.db.close()

    def keys(self):
        return self.db.keys()

    def has_key(self, key):
        return self.db.has_key(key)

    __contains__ = has_key

    def values(self):
        for v in self.db.values():
            yield v.decode("utf-8")

    def items(self):
        for k,v in self.db.items():
            yield k, v.decode("utf-8")

    def set_location(self, key):
        return self.db.set_location(key)

    def next(self):
        key, value = self.db.next()
        return key, value.decode("utf-8")

    def previous(self):
        key, value = self.db.previous()
        return key, value.decode("utf-8")

    def first(self):
        key, value = self.db.first()
        return key, value.decode("utf-8")

    def last(self):
        key, value = self.db.last()
        return key, value.decode("utf-8")

    def set_location(self, key):
        key, value = self.db.set_location(key)
        return key, value.decode("utf-8")

    def sync(self):
        return self.db.sync()


#----------------------------------------------------------------------
# Compatibility object factory functions

def hashopen(file, flag='c', mode=0o666, pgsize=None, ffactor=None, nelem=None,
            cachesize=None, lorder=None, hflags=0):

    flags = _checkflag(flag, file)
    e = _openDBEnv(cachesize)
    d = db.DB(e)
    d.set_flags(hflags)
    if pgsize is not None:    d.set_pagesize(pgsize)
    if lorder is not None:    d.set_lorder(lorder)
    if ffactor is not None:   d.set_h_ffactor(ffactor)
    if nelem is not None:     d.set_h_nelem(nelem)
    d.open(file, db.DB_HASH, flags, mode)
    return _DBWithCursor(d)

#----------------------------------------------------------------------

def btopen(file, flag='c', mode=0o666,
            btflags=0, cachesize=None, maxkeypage=None, minkeypage=None,
            pgsize=None, lorder=None):

    flags = _checkflag(flag, file)
    e = _openDBEnv(cachesize)
    d = db.DB(e)
    if pgsize is not None: d.set_pagesize(pgsize)
    if lorder is not None: d.set_lorder(lorder)
    d.set_flags(btflags)
    if minkeypage is not None: d.set_bt_minkey(minkeypage)
    if maxkeypage is not None: d.set_bt_maxkey(maxkeypage)
    d.open(file, db.DB_BTREE, flags, mode)
    return _DBWithCursor(d)

#----------------------------------------------------------------------


def rnopen(file, flag='c', mode=0o666,
            rnflags=0, cachesize=None, pgsize=None, lorder=None,
            rlen=None, delim=None, source=None, pad=None):

    flags = _checkflag(flag, file)
    e = _openDBEnv(cachesize)
    d = db.DB(e)
    if pgsize is not None: d.set_pagesize(pgsize)
    if lorder is not None: d.set_lorder(lorder)
    d.set_flags(rnflags)
    if delim is not None: d.set_re_delim(delim)
    if rlen is not None: d.set_re_len(rlen)
    if source is not None: d.set_re_source(source)
    if pad is not None: d.set_re_pad(pad)
    d.open(file, db.DB_RECNO, flags, mode)
    return _DBWithCursor(d)

#----------------------------------------------------------------------

def _openDBEnv(cachesize):
    e = db.DBEnv()
    if cachesize is not None:
        if cachesize >= 20480:
            e.set_cachesize(0, cachesize)
        else:
            raise error, "cachesize must be >= 20480"
    e.set_lk_detect(db.DB_LOCK_DEFAULT)
    e.open('.', db.DB_PRIVATE | db.DB_CREATE | db.DB_THREAD | db.DB_INIT_LOCK | db.DB_INIT_MPOOL)
    return e

def _checkflag(flag, file):
    if flag == 'r':
        flags = db.DB_RDONLY
    elif flag == 'rw':
        flags = 0
    elif flag == 'w':
        flags =  db.DB_CREATE
    elif flag == 'c':
        flags =  db.DB_CREATE
    elif flag == 'n':
        flags = db.DB_CREATE
        #flags = db.DB_CREATE | db.DB_TRUNCATE
        # we used db.DB_TRUNCATE flag for this before but BerkeleyDB
        # 4.2.52 changed to disallowed truncate with txn environments.
        if file is not None and os.path.isfile(file):
            os.unlink(file)
    else:
        raise error, "flags should be one of 'r', 'w', 'c' or 'n', not "+repr(flag)
    return flags | db.DB_THREAD

#----------------------------------------------------------------------


# This is a silly little hack that allows apps to continue to use the
# DB_THREAD flag even on systems without threads without freaking out
# BerkeleyDB.
#
# This assumes that if Python was built with thread support then
# BerkeleyDB was too.

try:
    import thread
    del thread
    if db.version() < (3, 3, 0):
        db.DB_THREAD = 0
except ImportError:
    db.DB_THREAD = 0

#----------------------------------------------------------------------
