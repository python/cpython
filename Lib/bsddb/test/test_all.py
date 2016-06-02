"""Run all test cases.
"""

import sys
import os
import unittest
try:
    # For Pythons w/distutils pybsddb
    import bsddb3 as bsddb
except ImportError:
    # For Python 2.3
    import bsddb


if sys.version_info[0] >= 3 :
    charset = "iso8859-1"  # Full 8 bit

    class logcursor_py3k(object) :
        def __init__(self, env) :
            self._logcursor = env.log_cursor()

        def __getattr__(self, v) :
            return getattr(self._logcursor, v)

        def __next__(self) :
            v = getattr(self._logcursor, "next")()
            if v is not None :
                v = (v[0], v[1].decode(charset))
            return v

        next = __next__

        def first(self) :
            v = self._logcursor.first()
            if v is not None :
                v = (v[0], v[1].decode(charset))
            return v

        def last(self) :
            v = self._logcursor.last()
            if v is not None :
                v = (v[0], v[1].decode(charset))
            return v

        def prev(self) :
            v = self._logcursor.prev()
            if v is not None :
                v = (v[0], v[1].decode(charset))
            return v

        def current(self) :
            v = self._logcursor.current()
            if v is not None :
                v = (v[0], v[1].decode(charset))
            return v

        def set(self, lsn) :
            v = self._logcursor.set(lsn)
            if v is not None :
                v = (v[0], v[1].decode(charset))
            return v

    class cursor_py3k(object) :
        def __init__(self, db, *args, **kwargs) :
            self._dbcursor = db.cursor(*args, **kwargs)

        def __getattr__(self, v) :
            return getattr(self._dbcursor, v)

        def _fix(self, v) :
            if v is None : return None
            key, value = v
            if isinstance(key, bytes) :
                key = key.decode(charset)
            return (key, value.decode(charset))

        def __next__(self) :
            v = getattr(self._dbcursor, "next")()
            return self._fix(v)

        next = __next__

        def previous(self) :
            v = self._dbcursor.previous()
            return self._fix(v)

        def last(self) :
            v = self._dbcursor.last()
            return self._fix(v)

        def set(self, k) :
            if isinstance(k, str) :
                k = bytes(k, charset)
            v = self._dbcursor.set(k)
            return self._fix(v)

        def set_recno(self, num) :
            v = self._dbcursor.set_recno(num)
            return self._fix(v)

        def set_range(self, k, dlen=-1, doff=-1) :
            if isinstance(k, str) :
                k = bytes(k, charset)
            v = self._dbcursor.set_range(k, dlen=dlen, doff=doff)
            return self._fix(v)

        def dup(self, flags=0) :
            cursor = self._dbcursor.dup(flags)
            return dup_cursor_py3k(cursor)

        def next_dup(self) :
            v = self._dbcursor.next_dup()
            return self._fix(v)

        def next_nodup(self) :
            v = self._dbcursor.next_nodup()
            return self._fix(v)

        def put(self, key, data, flags=0, dlen=-1, doff=-1) :
            if isinstance(key, str) :
                key = bytes(key, charset)
            if isinstance(data, str) :
                value = bytes(data, charset)
            return self._dbcursor.put(key, data, flags=flags, dlen=dlen,
                    doff=doff)

        def current(self, flags=0, dlen=-1, doff=-1) :
            v = self._dbcursor.current(flags=flags, dlen=dlen, doff=doff)
            return self._fix(v)

        def first(self) :
            v = self._dbcursor.first()
            return self._fix(v)

        def pget(self, key=None, data=None, flags=0) :
            # Incorrect because key can be a bare number,
            # but enough to pass testsuite
            if isinstance(key, int) and (data is None) and (flags == 0) :
                flags = key
                key = None
            if isinstance(key, str) :
                key = bytes(key, charset)
            if isinstance(data, int) and (flags==0) :
                flags = data
                data = None
            if isinstance(data, str) :
                data = bytes(data, charset)
            v=self._dbcursor.pget(key=key, data=data, flags=flags)
            if v is not None :
                v1, v2, v3 = v
                if isinstance(v1, bytes) :
                    v1 = v1.decode(charset)
                if isinstance(v2, bytes) :
                    v2 = v2.decode(charset)

                v = (v1, v2, v3.decode(charset))

            return v

        def join_item(self) :
            v = self._dbcursor.join_item()
            if v is not None :
                v = v.decode(charset)
            return v

        def get(self, *args, **kwargs) :
            l = len(args)
            if l == 2 :
                k, f = args
                if isinstance(k, str) :
                    k = bytes(k, "iso8859-1")
                args = (k, f)
            elif l == 3 :
                k, d, f = args
                if isinstance(k, str) :
                    k = bytes(k, charset)
                if isinstance(d, str) :
                    d = bytes(d, charset)
                args =(k, d, f)

            v = self._dbcursor.get(*args, **kwargs)
            if v is not None :
                k, v = v
                if isinstance(k, bytes) :
                    k = k.decode(charset)
                v = (k, v.decode(charset))
            return v

        def get_both(self, key, value) :
            if isinstance(key, str) :
                key = bytes(key, charset)
            if isinstance(value, str) :
                value = bytes(value, charset)
            v=self._dbcursor.get_both(key, value)
            return self._fix(v)

    class dup_cursor_py3k(cursor_py3k) :
        def __init__(self, dbcursor) :
            self._dbcursor = dbcursor

    class DB_py3k(object) :
        def __init__(self, *args, **kwargs) :
            args2=[]
            for i in args :
                if isinstance(i, DBEnv_py3k) :
                    i = i._dbenv
                args2.append(i)
            args = tuple(args2)
            for k, v in kwargs.items() :
                if isinstance(v, DBEnv_py3k) :
                    kwargs[k] = v._dbenv

            self._db = bsddb._db.DB_orig(*args, **kwargs)

        def __contains__(self, k) :
            if isinstance(k, str) :
                k = bytes(k, charset)
            return getattr(self._db, "has_key")(k)

        def __getitem__(self, k) :
            if isinstance(k, str) :
                k = bytes(k, charset)
            v = self._db[k]
            if v is not None :
                v = v.decode(charset)
            return v

        def __setitem__(self, k, v) :
            if isinstance(k, str) :
                k = bytes(k, charset)
            if isinstance(v, str) :
                v = bytes(v, charset)
            self._db[k] = v

        def __delitem__(self, k) :
            if isinstance(k, str) :
                k = bytes(k, charset)
            del self._db[k]

        def __getattr__(self, v) :
            return getattr(self._db, v)

        def __len__(self) :
            return len(self._db)

        def has_key(self, k, txn=None) :
            if isinstance(k, str) :
                k = bytes(k, charset)
            return self._db.has_key(k, txn=txn)

        def set_re_delim(self, c) :
            if isinstance(c, str) :  # We can use a numeric value byte too
                c = bytes(c, charset)
            return self._db.set_re_delim(c)

        def set_re_pad(self, c) :
            if isinstance(c, str) :  # We can use a numeric value byte too
                c = bytes(c, charset)
            return self._db.set_re_pad(c)

        def get_re_source(self) :
            source = self._db.get_re_source()
            return source.decode(charset)

        def put(self, key, data, txn=None, flags=0, dlen=-1, doff=-1) :
            if isinstance(key, str) :
                key = bytes(key, charset)
            if isinstance(data, str) :
                value = bytes(data, charset)
            return self._db.put(key, data, flags=flags, txn=txn, dlen=dlen,
                    doff=doff)

        def append(self, value, txn=None) :
            if isinstance(value, str) :
                value = bytes(value, charset)
            return self._db.append(value, txn=txn)

        def get_size(self, key) :
            if isinstance(key, str) :
                key = bytes(key, charset)
            return self._db.get_size(key)

        def exists(self, key, *args, **kwargs) :
            if isinstance(key, str) :
                key = bytes(key, charset)
            return self._db.exists(key, *args, **kwargs)

        def get(self, key, default="MagicCookie", txn=None, flags=0, dlen=-1, doff=-1) :
            if isinstance(key, str) :
                key = bytes(key, charset)
            if default != "MagicCookie" :  # Magic for 'test_get_none.py'
                v=self._db.get(key, default=default, txn=txn, flags=flags,
                        dlen=dlen, doff=doff)
            else :
                v=self._db.get(key, txn=txn, flags=flags,
                        dlen=dlen, doff=doff)
            if (v is not None) and isinstance(v, bytes) :
                v = v.decode(charset)
            return v

        def pget(self, key, txn=None) :
            if isinstance(key, str) :
                key = bytes(key, charset)
            v=self._db.pget(key, txn=txn)
            if v is not None :
                v1, v2 = v
                if isinstance(v1, bytes) :
                    v1 = v1.decode(charset)

                v = (v1, v2.decode(charset))
            return v

        def get_both(self, key, value, txn=None, flags=0) :
            if isinstance(key, str) :
                key = bytes(key, charset)
            if isinstance(value, str) :
                value = bytes(value, charset)
            v=self._db.get_both(key, value, txn=txn, flags=flags)
            if v is not None :
                v = v.decode(charset)
            return v

        def delete(self, key, txn=None) :
            if isinstance(key, str) :
                key = bytes(key, charset)
            return self._db.delete(key, txn=txn)

        def keys(self) :
            k = self._db.keys()
            if len(k) and isinstance(k[0], bytes) :
                return [i.decode(charset) for i in self._db.keys()]
            else :
                return k

        def items(self) :
            data = self._db.items()
            if not len(data) : return data
            data2 = []
            for k, v in data :
                if isinstance(k, bytes) :
                    k = k.decode(charset)
                data2.append((k, v.decode(charset)))
            return data2

        def associate(self, secondarydb, callback, flags=0, txn=None) :
            class associate_callback(object) :
                def __init__(self, callback) :
                    self._callback = callback

                def callback(self, key, data) :
                    if isinstance(key, str) :
                        key = key.decode(charset)
                    data = data.decode(charset)
                    key = self._callback(key, data)
                    if (key != bsddb._db.DB_DONOTINDEX) :
                        if isinstance(key, str) :
                            key = bytes(key, charset)
                        elif isinstance(key, list) :
                            key2 = []
                            for i in key :
                                if isinstance(i, str) :
                                    i = bytes(i, charset)
                                key2.append(i)
                            key = key2
                    return key

            return self._db.associate(secondarydb._db,
                    associate_callback(callback).callback, flags=flags,
                    txn=txn)

        def cursor(self, txn=None, flags=0) :
            return cursor_py3k(self._db, txn=txn, flags=flags)

        def join(self, cursor_list) :
            cursor_list = [i._dbcursor for i in cursor_list]
            return dup_cursor_py3k(self._db.join(cursor_list))

    class DBEnv_py3k(object) :
        def __init__(self, *args, **kwargs) :
            self._dbenv = bsddb._db.DBEnv_orig(*args, **kwargs)

        def __getattr__(self, v) :
            return getattr(self._dbenv, v)

        def log_cursor(self, flags=0) :
            return logcursor_py3k(self._dbenv)

        def get_lg_dir(self) :
            return self._dbenv.get_lg_dir().decode(charset)

        def get_tmp_dir(self) :
            return self._dbenv.get_tmp_dir().decode(charset)

        def get_data_dirs(self) :
            return tuple(
                (i.decode(charset) for i in self._dbenv.get_data_dirs()))

    class DBSequence_py3k(object) :
        def __init__(self, db, *args, **kwargs) :
            self._db=db
            self._dbsequence = bsddb._db.DBSequence_orig(db._db, *args, **kwargs)

        def __getattr__(self, v) :
            return getattr(self._dbsequence, v)

        def open(self, key, *args, **kwargs) :
            return self._dbsequence.open(bytes(key, charset), *args, **kwargs)

        def get_key(self) :
            return  self._dbsequence.get_key().decode(charset)

        def get_dbp(self) :
            return self._db

    bsddb._db.DBEnv_orig = bsddb._db.DBEnv
    bsddb._db.DB_orig = bsddb._db.DB
    if bsddb.db.version() <= (4, 3) :
        bsddb._db.DBSequence_orig = None
    else :
        bsddb._db.DBSequence_orig = bsddb._db.DBSequence

    def do_proxy_db_py3k(flag) :
        flag2 = do_proxy_db_py3k.flag
        do_proxy_db_py3k.flag = flag
        if flag :
            bsddb.DBEnv = bsddb.db.DBEnv = bsddb._db.DBEnv = DBEnv_py3k
            bsddb.DB = bsddb.db.DB = bsddb._db.DB = DB_py3k
            bsddb._db.DBSequence = DBSequence_py3k
        else :
            bsddb.DBEnv = bsddb.db.DBEnv = bsddb._db.DBEnv = bsddb._db.DBEnv_orig
            bsddb.DB = bsddb.db.DB = bsddb._db.DB = bsddb._db.DB_orig
            bsddb._db.DBSequence = bsddb._db.DBSequence_orig
        return flag2

    do_proxy_db_py3k.flag = False
    do_proxy_db_py3k(True)

try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db, dbtables, dbutils, dbshelve, \
            hashopen, btopen, rnopen, dbobj
except ImportError:
    # For Python 2.3
    from bsddb import db, dbtables, dbutils, dbshelve, \
            hashopen, btopen, rnopen, dbobj

try:
    from bsddb3 import test_support
except ImportError:
    if sys.version_info[0] < 3 :
        from test import test_support
    else :
        from test import support as test_support


try:
    if sys.version_info[0] < 3 :
        from threading import Thread, currentThread
        del Thread, currentThread
    else :
        from threading import Thread, current_thread
        del Thread, current_thread
    have_threads = True
except ImportError:
    have_threads = False

verbose = 0
if 'verbose' in sys.argv:
    verbose = 1
    sys.argv.remove('verbose')

if 'silent' in sys.argv:  # take care of old flag, just in case
    verbose = 0
    sys.argv.remove('silent')


def print_versions():
    print
    print '-=' * 38
    print db.DB_VERSION_STRING
    print 'bsddb.db.version():   %s' % (db.version(), )
    if db.version() >= (5, 0) :
        print 'bsddb.db.full_version(): %s' %repr(db.full_version())
    print 'bsddb.db.__version__: %s' % db.__version__
    print 'bsddb.db.cvsid:       %s' % db.cvsid

    # Workaround for allowing generating an EGGs as a ZIP files.
    suffix="__"
    print 'py module:            %s' % getattr(bsddb, "__file"+suffix)
    print 'extension module:     %s' % getattr(bsddb, "__file"+suffix)

    print 'python version:       %s' % sys.version
    print 'My pid:               %s' % os.getpid()
    print '-=' * 38


def get_new_path(name) :
    get_new_path.mutex.acquire()
    try :
        import os
        path=os.path.join(get_new_path.prefix,
                name+"_"+str(os.getpid())+"_"+str(get_new_path.num))
        get_new_path.num+=1
    finally :
        get_new_path.mutex.release()
    return path

def get_new_environment_path() :
    path=get_new_path("environment")
    import os
    try:
        os.makedirs(path,mode=0700)
    except os.error:
        test_support.rmtree(path)
        os.makedirs(path)
    return path

def get_new_database_path() :
    path=get_new_path("database")
    import os
    if os.path.exists(path) :
        os.remove(path)
    return path


# This path can be overridden via "set_test_path_prefix()".
import os, os.path
get_new_path.prefix=os.path.join(os.environ.get("TMPDIR",
    os.path.join(os.sep,"tmp")), "z-Berkeley_DB")
get_new_path.num=0

def get_test_path_prefix() :
    return get_new_path.prefix

def set_test_path_prefix(path) :
    get_new_path.prefix=path

def remove_test_path_directory() :
    test_support.rmtree(get_new_path.prefix)

if have_threads :
    import threading
    get_new_path.mutex=threading.Lock()
    del threading
else :
    class Lock(object) :
        def acquire(self) :
            pass
        def release(self) :
            pass
    get_new_path.mutex=Lock()
    del Lock



class PrintInfoFakeTest(unittest.TestCase):
    def testPrintVersions(self):
        print_versions()


# This little hack is for when this module is run as main and all the
# other modules import it so they will still be able to get the right
# verbose setting.  It's confusing but it works.
if sys.version_info[0] < 3 :
    import test_all
    test_all.verbose = verbose
else :
    import sys
    print >>sys.stderr, "Work to do!"


def suite(module_prefix='', timing_check=None):
    test_modules = [
        'test_associate',
        'test_basics',
        'test_dbenv',
        'test_db',
        'test_compare',
        'test_compat',
        'test_cursor_pget_bug',
        'test_dbobj',
        'test_dbshelve',
        'test_dbtables',
        'test_distributed_transactions',
        'test_early_close',
        'test_fileid',
        'test_get_none',
        'test_join',
        'test_lock',
        'test_misc',
        'test_pickle',
        'test_queue',
        'test_recno',
        'test_replication',
        'test_sequence',
        'test_thread',
        ]

    alltests = unittest.TestSuite()
    for name in test_modules:
        #module = __import__(name)
        # Do it this way so that suite may be called externally via
        # python's Lib/test/test_bsddb3.
        module = __import__(module_prefix+name, globals(), locals(), name)

        alltests.addTest(module.test_suite())
        if timing_check:
            alltests.addTest(unittest.makeSuite(timing_check))
    return alltests


def test_suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(PrintInfoFakeTest))
    return suite


if __name__ == '__main__':
    print_versions()
    unittest.main(defaultTest='suite')
