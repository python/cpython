"""TestCases for distributed transactions.
"""

import os
import unittest

from test_all import db, test_support, get_new_environment_path, \
        get_new_database_path

try :
    a=set()
except : # Python 2.3
    from sets import Set as set
else :
    del a

from test_all import verbose

#----------------------------------------------------------------------

class DBTxn_distributed(unittest.TestCase):
    num_txns=1234
    nosync=True
    must_open_db=False
    def _create_env(self, must_open_db) :
        self.dbenv = db.DBEnv()
        self.dbenv.set_tx_max(self.num_txns)
        self.dbenv.set_lk_max_lockers(self.num_txns*2)
        self.dbenv.set_lk_max_locks(self.num_txns*2)
        self.dbenv.set_lk_max_objects(self.num_txns*2)
        if self.nosync :
            self.dbenv.set_flags(db.DB_TXN_NOSYNC,True)
        self.dbenv.open(self.homeDir, db.DB_CREATE | db.DB_THREAD |
                db.DB_RECOVER |
                db.DB_INIT_TXN | db.DB_INIT_LOG | db.DB_INIT_MPOOL |
                db.DB_INIT_LOCK, 0666)
        self.db = db.DB(self.dbenv)
        self.db.set_re_len(db.DB_GID_SIZE)
        if must_open_db :
            if db.version() >= (4,2) :
                txn=self.dbenv.txn_begin()
                self.db.open(self.filename,
                        db.DB_QUEUE, db.DB_CREATE | db.DB_THREAD, 0666,
                        txn=txn)
                txn.commit()
            else :
                self.db.open(self.filename,
                        db.DB_QUEUE, db.DB_CREATE | db.DB_THREAD, 0666)

    def setUp(self) :
        self.homeDir = get_new_environment_path()
        self.filename = "test"
        return self._create_env(must_open_db=True)

    def _destroy_env(self):
        if self.nosync or (db.version()[:2] == (4,6)):  # Known bug
            self.dbenv.log_flush()
        self.db.close()
        self.dbenv.close()

    def tearDown(self):
        self._destroy_env()
        test_support.rmtree(self.homeDir)

    def _recreate_env(self,must_open_db) :
        self._destroy_env()
        self._create_env(must_open_db)

    def test01_distributed_transactions(self) :
        txns=set()
        adapt = lambda x : x
        import sys
        if sys.version_info[0] >= 3 :
            adapt = lambda x : bytes(x, "ascii")
    # Create transactions, "prepare" them, and
    # let them be garbage collected.
        for i in xrange(self.num_txns) :
            txn = self.dbenv.txn_begin()
            gid = "%%%dd" %db.DB_GID_SIZE
            gid = adapt(gid %i)
            self.db.put(i, gid, txn=txn, flags=db.DB_APPEND)
            txns.add(gid)
            txn.prepare(gid)
        del txn

        self._recreate_env(self.must_open_db)

    # Get "to be recovered" transactions but
    # let them be garbage collected.
        recovered_txns=self.dbenv.txn_recover()
        self.assertEqual(self.num_txns,len(recovered_txns))
        for gid,txn in recovered_txns :
            self.assertTrue(gid in txns)
        del txn
        del recovered_txns

        self._recreate_env(self.must_open_db)

    # Get "to be recovered" transactions. Commit, abort and
    # discard them.
        recovered_txns=self.dbenv.txn_recover()
        self.assertEqual(self.num_txns,len(recovered_txns))
        discard_txns=set()
        committed_txns=set()
        state=0
        for gid,txn in recovered_txns :
            if state==0 or state==1:
                committed_txns.add(gid)
                txn.commit()
            elif state==2 :
                txn.abort()
            elif state==3 :
                txn.discard()
                discard_txns.add(gid)
                state=-1
            state+=1
        del txn
        del recovered_txns

        self._recreate_env(self.must_open_db)

    # Verify the discarded transactions are still
    # around, and dispose them.
        recovered_txns=self.dbenv.txn_recover()
        self.assertEqual(len(discard_txns),len(recovered_txns))
        for gid,txn in recovered_txns :
            txn.abort()
        del txn
        del recovered_txns

        self._recreate_env(must_open_db=True)

    # Be sure there are not pending transactions.
    # Check also database size.
        recovered_txns=self.dbenv.txn_recover()
        self.assertTrue(len(recovered_txns)==0)
        self.assertEqual(len(committed_txns),self.db.stat()["nkeys"])

class DBTxn_distributedSYNC(DBTxn_distributed):
    nosync=False

class DBTxn_distributed_must_open_db(DBTxn_distributed):
    must_open_db=True

class DBTxn_distributedSYNC_must_open_db(DBTxn_distributed):
    nosync=False
    must_open_db=True

#----------------------------------------------------------------------

def test_suite():
    suite = unittest.TestSuite()
    if db.version() >= (4,5) :
        suite.addTest(unittest.makeSuite(DBTxn_distributed))
        suite.addTest(unittest.makeSuite(DBTxn_distributedSYNC))
    if db.version() >= (4,6) :
        suite.addTest(unittest.makeSuite(DBTxn_distributed_must_open_db))
        suite.addTest(unittest.makeSuite(DBTxn_distributedSYNC_must_open_db))
    return suite


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
