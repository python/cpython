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


"""Support for BerkeleyDB 3.1 through 4.1.
"""

try:
    import _bsddb
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


class _DBWithCursor:
    """
    A simple wrapper around DB that makes it look like the bsddbobject in
    the old module.  It uses a cursor as needed to provide DB traversal.
    """
    def __init__(self, db):
        self.db = db
        self.dbc = None
        self.db.set_get_returns_none(0)

    def __del__(self):
        self.close()

    def _checkCursor(self):
        if self.dbc is None:
            self.dbc = self.db.cursor()

    def _checkOpen(self):
        if self.db is None:
            raise error, "BSDDB object has already been closed"

    def isOpen(self):
        return self.db is not None

    def __len__(self):
        self._checkOpen()
        return len(self.db)

    def __getitem__(self, key):
        self._checkOpen()
        return self.db[key]

    def __setitem__(self, key, value):
        self._checkOpen()
        self.db[key] = value

    def __delitem__(self, key):
        self._checkOpen()
        del self.db[key]

    def close(self):
        if self.dbc is not None:
            self.dbc.close()
        v = 0
        if self.db is not None:
            v = self.db.close()
        self.dbc = None
        self.db = None
        return v

    def keys(self):
        self._checkOpen()
        return self.db.keys()

    def has_key(self, key):
        self._checkOpen()
        return self.db.has_key(key)

    def set_location(self, key):
        self._checkOpen()
        self._checkCursor()
        return self.dbc.set(key)

    def next(self):
        self._checkOpen()
        self._checkCursor()
        rv = self.dbc.next()
        return rv

    def previous(self):
        self._checkOpen()
        self._checkCursor()
        rv = self.dbc.prev()
        return rv

    def first(self):
        self._checkOpen()
        self._checkCursor()
        rv = self.dbc.first()
        return rv

    def last(self):
        self._checkOpen()
        self._checkCursor()
        rv = self.dbc.last()
        return rv

    def sync(self):
        self._checkOpen()
        return self.db.sync()


#----------------------------------------------------------------------
# Compatibility object factory functions

def hashopen(file, flag='c', mode=0666, pgsize=None, ffactor=None, nelem=None,
            cachesize=None, lorder=None, hflags=0):

    flags = _checkflag(flag)
    d = db.DB()
    d.set_flags(hflags)
    if cachesize is not None: d.set_cachesize(0, cachesize)
    if pgsize is not None:    d.set_pagesize(pgsize)
    if lorder is not None:    d.set_lorder(lorder)
    if ffactor is not None:   d.set_h_ffactor(ffactor)
    if nelem is not None:     d.set_h_nelem(nelem)
    d.open(file, db.DB_HASH, flags, mode)
    return _DBWithCursor(d)

#----------------------------------------------------------------------

def btopen(file, flag='c', mode=0666,
            btflags=0, cachesize=None, maxkeypage=None, minkeypage=None,
            pgsize=None, lorder=None):

    flags = _checkflag(flag)
    d = db.DB()
    if cachesize is not None: d.set_cachesize(0, cachesize)
    if pgsize is not None: d.set_pagesize(pgsize)
    if lorder is not None: d.set_lorder(lorder)
    d.set_flags(btflags)
    if minkeypage is not None: d.set_bt_minkey(minkeypage)
    if maxkeypage is not None: d.set_bt_maxkey(maxkeypage)
    d.open(file, db.DB_BTREE, flags, mode)
    return _DBWithCursor(d)

#----------------------------------------------------------------------


def rnopen(file, flag='c', mode=0666,
            rnflags=0, cachesize=None, pgsize=None, lorder=None,
            rlen=None, delim=None, source=None, pad=None):

    flags = _checkflag(flag)
    d = db.DB()
    if cachesize is not None: d.set_cachesize(0, cachesize)
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


def _checkflag(flag):
    if flag == 'r':
        flags = db.DB_RDONLY
    elif flag == 'rw':
        flags = 0
    elif flag == 'w':
        flags =  db.DB_CREATE
    elif flag == 'c':
        flags =  db.DB_CREATE
    elif flag == 'n':
        flags = db.DB_CREATE | db.DB_TRUNCATE
    else:
        raise error, "flags should be one of 'r', 'w', 'c' or 'n'"
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
except ImportError:
    db.DB_THREAD = 0


#----------------------------------------------------------------------
