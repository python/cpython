"""TestCases for distributed transactions.
"""

import os
import time
import unittest

from test_all import db, test_support, have_threads, verbose, \
        get_new_environment_path, get_new_database_path


#----------------------------------------------------------------------

class DBReplicationManager(unittest.TestCase):
    import sys
    if sys.version_info[:3] < (2, 4, 0):
        def assertTrue(self, expr, msg=None):
            self.failUnless(expr,msg=msg)

    def setUp(self) :
        self.homeDirMaster = get_new_environment_path()
        self.homeDirClient = get_new_environment_path()

        self.dbenvMaster = db.DBEnv()
        self.dbenvClient = db.DBEnv()

        # Must use "DB_THREAD" because the Replication Manager will
        # be executed in other threads but will use the same environment.
        # http://forums.oracle.com/forums/thread.jspa?threadID=645788&tstart=0
        self.dbenvMaster.open(self.homeDirMaster, db.DB_CREATE | db.DB_INIT_TXN
                | db.DB_INIT_LOG | db.DB_INIT_MPOOL | db.DB_INIT_LOCK |
                db.DB_INIT_REP | db.DB_RECOVER | db.DB_THREAD, 0666)
        self.dbenvClient.open(self.homeDirClient, db.DB_CREATE | db.DB_INIT_TXN
                | db.DB_INIT_LOG | db.DB_INIT_MPOOL | db.DB_INIT_LOCK |
                db.DB_INIT_REP | db.DB_RECOVER | db.DB_THREAD, 0666)

        self.confirmed_master=self.client_startupdone=False
        def confirmed_master(a,b,c) :
            if b==db.DB_EVENT_REP_MASTER :
                self.confirmed_master=True

        def client_startupdone(a,b,c) :
            if b==db.DB_EVENT_REP_STARTUPDONE :
                self.client_startupdone=True

        self.dbenvMaster.set_event_notify(confirmed_master)
        self.dbenvClient.set_event_notify(client_startupdone)

        #self.dbenvMaster.set_verbose(db.DB_VERB_REPLICATION, True)
        #self.dbenvMaster.set_verbose(db.DB_VERB_FILEOPS_ALL, True)
        #self.dbenvClient.set_verbose(db.DB_VERB_REPLICATION, True)
        #self.dbenvClient.set_verbose(db.DB_VERB_FILEOPS_ALL, True)

        self.dbMaster = self.dbClient = None


    def tearDown(self):
        if self.dbClient :
            self.dbClient.close()
        if self.dbMaster :
            self.dbMaster.close()
        self.dbenvClient.close()
        self.dbenvMaster.close()
        test_support.rmtree(self.homeDirClient)
        test_support.rmtree(self.homeDirMaster)

    def test01_basic_replication(self) :
        master_port = test_support.find_unused_port()
        self.dbenvMaster.repmgr_set_local_site("127.0.0.1", master_port)
        client_port = test_support.find_unused_port()
        self.dbenvClient.repmgr_set_local_site("127.0.0.1", client_port)
        self.dbenvMaster.repmgr_add_remote_site("127.0.0.1", client_port)
        self.dbenvClient.repmgr_add_remote_site("127.0.0.1", master_port)
        self.dbenvMaster.rep_set_nsites(2)
        self.dbenvClient.rep_set_nsites(2)
        self.dbenvMaster.rep_set_priority(10)
        self.dbenvClient.rep_set_priority(0)

        self.dbenvMaster.rep_set_timeout(db.DB_REP_CONNECTION_RETRY,100123)
        self.dbenvClient.rep_set_timeout(db.DB_REP_CONNECTION_RETRY,100321)
        self.assertEquals(self.dbenvMaster.rep_get_timeout(
            db.DB_REP_CONNECTION_RETRY), 100123)
        self.assertEquals(self.dbenvClient.rep_get_timeout(
            db.DB_REP_CONNECTION_RETRY), 100321)

        self.dbenvMaster.rep_set_timeout(db.DB_REP_ELECTION_TIMEOUT, 100234)
        self.dbenvClient.rep_set_timeout(db.DB_REP_ELECTION_TIMEOUT, 100432)
        self.assertEquals(self.dbenvMaster.rep_get_timeout(
            db.DB_REP_ELECTION_TIMEOUT), 100234)
        self.assertEquals(self.dbenvClient.rep_get_timeout(
            db.DB_REP_ELECTION_TIMEOUT), 100432)

        self.dbenvMaster.rep_set_timeout(db.DB_REP_ELECTION_RETRY, 100345)
        self.dbenvClient.rep_set_timeout(db.DB_REP_ELECTION_RETRY, 100543)
        self.assertEquals(self.dbenvMaster.rep_get_timeout(
            db.DB_REP_ELECTION_RETRY), 100345)
        self.assertEquals(self.dbenvClient.rep_get_timeout(
            db.DB_REP_ELECTION_RETRY), 100543)

        self.dbenvMaster.repmgr_set_ack_policy(db.DB_REPMGR_ACKS_ALL)
        self.dbenvClient.repmgr_set_ack_policy(db.DB_REPMGR_ACKS_ALL)

        self.dbenvMaster.repmgr_start(1, db.DB_REP_MASTER);
        self.dbenvClient.repmgr_start(1, db.DB_REP_CLIENT);

        self.assertEquals(self.dbenvMaster.rep_get_nsites(),2)
        self.assertEquals(self.dbenvClient.rep_get_nsites(),2)
        self.assertEquals(self.dbenvMaster.rep_get_priority(),10)
        self.assertEquals(self.dbenvClient.rep_get_priority(),0)
        self.assertEquals(self.dbenvMaster.repmgr_get_ack_policy(),
                db.DB_REPMGR_ACKS_ALL)
        self.assertEquals(self.dbenvClient.repmgr_get_ack_policy(),
                db.DB_REPMGR_ACKS_ALL)

        # The timeout is necessary in BDB 4.5, since DB_EVENT_REP_STARTUPDONE
        # is not generated if the master has no new transactions.
        # This is solved in BDB 4.6 (#15542).
        import time
        timeout = time.time()+10
        while (time.time()<timeout) and not (self.confirmed_master and self.client_startupdone) :
            time.sleep(0.02)
        # this fails on Windows as self.client_startupdone never gets set
        # to True - see bug 3892.  BUT - even though this assertion
        # fails on Windows the rest of the test passes - so to prove
        # that we let the rest of the test run.  Sadly we can't
        # make use of raising TestSkipped() here (unittest still
        # reports it as an error), so we yell to stderr.
        import sys
        if sys.platform=="win32":
            print >> sys.stderr, \
                "XXX - windows bsddb replication fails on windows and is skipped"
            print >> sys.stderr, "XXX - Please see issue #3892"
        else:
            self.assertTrue(time.time()<timeout)

        d = self.dbenvMaster.repmgr_site_list()
        self.assertEquals(len(d), 1)
        self.assertEquals(d[0][0], "127.0.0.1")
        self.assertEquals(d[0][1], client_port)
        self.assertTrue((d[0][2]==db.DB_REPMGR_CONNECTED) or \
                (d[0][2]==db.DB_REPMGR_DISCONNECTED))

        d = self.dbenvClient.repmgr_site_list()
        self.assertEquals(len(d), 1)
        self.assertEquals(d[0][0], "127.0.0.1")
        self.assertEquals(d[0][1], master_port)
        self.assertTrue((d[0][2]==db.DB_REPMGR_CONNECTED) or \
                (d[0][2]==db.DB_REPMGR_DISCONNECTED))

        if db.version() >= (4,6) :
            d = self.dbenvMaster.repmgr_stat(flags=db.DB_STAT_CLEAR);
            self.assertTrue("msgs_queued" in d)

        self.dbMaster=db.DB(self.dbenvMaster)
        txn=self.dbenvMaster.txn_begin()
        self.dbMaster.open("test", db.DB_HASH, db.DB_CREATE, 0666, txn=txn)
        txn.commit()

        import time,os.path
        timeout=time.time()+10
        while (time.time()<timeout) and \
          not (os.path.exists(os.path.join(self.homeDirClient,"test"))) :
            time.sleep(0.01)

        self.dbClient=db.DB(self.dbenvClient)
        while True :
            txn=self.dbenvClient.txn_begin()
            try :
                self.dbClient.open("test", db.DB_HASH, flags=db.DB_RDONLY,
                        mode=0666, txn=txn)
            except db.DBRepHandleDeadError :
                txn.abort()
                self.dbClient.close()
                self.dbClient=db.DB(self.dbenvClient)
                continue

            txn.commit()
            break

        txn=self.dbenvMaster.txn_begin()
        self.dbMaster.put("ABC", "123", txn=txn)
        txn.commit()
        import time
        timeout=time.time()+10
        v=None
        while (time.time()<timeout) and (v==None) :
            txn=self.dbenvClient.txn_begin()
            v=self.dbClient.get("ABC", txn=txn)
            txn.commit()
            if v==None :
                time.sleep(0.02)
        self.assertTrue(time.time()<timeout)
        self.assertEquals("123", v)

        txn=self.dbenvMaster.txn_begin()
        self.dbMaster.delete("ABC", txn=txn)
        txn.commit()
        timeout=time.time()+10
        while (time.time()<timeout) and (v!=None) :
            txn=self.dbenvClient.txn_begin()
            v=self.dbClient.get("ABC", txn=txn)
            txn.commit()
            if v==None :
                time.sleep(0.02)
        self.assertTrue(time.time()<timeout)
        self.assertEquals(None, v)

class DBBaseReplication(DBReplicationManager):
    def setUp(self) :
        DBReplicationManager.setUp(self)
        def confirmed_master(a,b,c) :
            if (b == db.DB_EVENT_REP_MASTER) or (b == db.DB_EVENT_REP_ELECTED) :
                self.confirmed_master = True

        def client_startupdone(a,b,c) :
            if b == db.DB_EVENT_REP_STARTUPDONE :
                self.client_startupdone = True

        self.dbenvMaster.set_event_notify(confirmed_master)
        self.dbenvClient.set_event_notify(client_startupdone)

        import Queue
        self.m2c = Queue.Queue()
        self.c2m = Queue.Queue()

        # There are only two nodes, so we don't need to
        # do any routing decision
        def m2c(dbenv, control, rec, lsnp, envid, flags) :
            self.m2c.put((control, rec))

        def c2m(dbenv, control, rec, lsnp, envid, flags) :
            self.c2m.put((control, rec))

        self.dbenvMaster.rep_set_transport(13,m2c)
        self.dbenvMaster.rep_set_priority(10)
        self.dbenvClient.rep_set_transport(3,c2m)
        self.dbenvClient.rep_set_priority(0)

        self.assertEquals(self.dbenvMaster.rep_get_priority(),10)
        self.assertEquals(self.dbenvClient.rep_get_priority(),0)

        #self.dbenvMaster.set_verbose(db.DB_VERB_REPLICATION, True)
        #self.dbenvMaster.set_verbose(db.DB_VERB_FILEOPS_ALL, True)
        #self.dbenvClient.set_verbose(db.DB_VERB_REPLICATION, True)
        #self.dbenvClient.set_verbose(db.DB_VERB_FILEOPS_ALL, True)

        def thread_master() :
            return self.thread_do(self.dbenvMaster, self.c2m, 3,
                    self.master_doing_election, True)

        def thread_client() :
            return self.thread_do(self.dbenvClient, self.m2c, 13,
                    self.client_doing_election, False)

        from threading import Thread
        t_m=Thread(target=thread_master)
        t_c=Thread(target=thread_client)
        import sys
        if sys.version_info[0] < 3 :
            t_m.setDaemon(True)
            t_c.setDaemon(True)
        else :
            t_m.daemon = True
            t_c.daemon = True

        self.t_m = t_m
        self.t_c = t_c

        self.dbMaster = self.dbClient = None

        self.master_doing_election=[False]
        self.client_doing_election=[False]


    def tearDown(self):
        if self.dbClient :
            self.dbClient.close()
        if self.dbMaster :
            self.dbMaster.close()
        self.m2c.put(None)
        self.c2m.put(None)
        self.t_m.join()
        self.t_c.join()
        self.dbenvClient.close()
        self.dbenvMaster.close()
        test_support.rmtree(self.homeDirClient)
        test_support.rmtree(self.homeDirMaster)

    def basic_rep_threading(self) :
        self.dbenvMaster.rep_start(flags=db.DB_REP_MASTER)
        self.dbenvClient.rep_start(flags=db.DB_REP_CLIENT)

        def thread_do(env, q, envid, election_status, must_be_master) :
            while True :
                v=q.get()
                if v == None : return
                env.rep_process_message(v[0], v[1], envid)

        self.thread_do = thread_do

        self.t_m.start()
        self.t_c.start()

    def test01_basic_replication(self) :
        self.basic_rep_threading()

        # The timeout is necessary in BDB 4.5, since DB_EVENT_REP_STARTUPDONE
        # is not generated if the master has no new transactions.
        # This is solved in BDB 4.6 (#15542).
        import time
        timeout = time.time()+10
        while (time.time()<timeout) and not (self.confirmed_master and
                self.client_startupdone) :
            time.sleep(0.02)
        self.assertTrue(time.time()<timeout)

        self.dbMaster=db.DB(self.dbenvMaster)
        txn=self.dbenvMaster.txn_begin()
        self.dbMaster.open("test", db.DB_HASH, db.DB_CREATE, 0666, txn=txn)
        txn.commit()

        import time,os.path
        timeout=time.time()+10
        while (time.time()<timeout) and \
          not (os.path.exists(os.path.join(self.homeDirClient,"test"))) :
            time.sleep(0.01)

        self.dbClient=db.DB(self.dbenvClient)
        while True :
            txn=self.dbenvClient.txn_begin()
            try :
                self.dbClient.open("test", db.DB_HASH, flags=db.DB_RDONLY,
                        mode=0666, txn=txn)
            except db.DBRepHandleDeadError :
                txn.abort()
                self.dbClient.close()
                self.dbClient=db.DB(self.dbenvClient)
                continue

            txn.commit()
            break

        txn=self.dbenvMaster.txn_begin()
        self.dbMaster.put("ABC", "123", txn=txn)
        txn.commit()
        import time
        timeout=time.time()+10
        v=None
        while (time.time()<timeout) and (v==None) :
            txn=self.dbenvClient.txn_begin()
            v=self.dbClient.get("ABC", txn=txn)
            txn.commit()
            if v==None :
                time.sleep(0.02)
        self.assertTrue(time.time()<timeout)
        self.assertEquals("123", v)

        txn=self.dbenvMaster.txn_begin()
        self.dbMaster.delete("ABC", txn=txn)
        txn.commit()
        timeout=time.time()+10
        while (time.time()<timeout) and (v!=None) :
            txn=self.dbenvClient.txn_begin()
            v=self.dbClient.get("ABC", txn=txn)
            txn.commit()
            if v==None :
                time.sleep(0.02)
        self.assertTrue(time.time()<timeout)
        self.assertEquals(None, v)

    if db.version() >= (4,7) :
        def test02_test_request(self) :
            self.basic_rep_threading()
            (minimum, maximum) = self.dbenvClient.rep_get_request()
            self.dbenvClient.rep_set_request(minimum-1, maximum+1)
            self.assertEqual(self.dbenvClient.rep_get_request(),
                    (minimum-1, maximum+1))

    if db.version() >= (4,6) :
        def test03_master_election(self) :
            # Get ready to hold an election
            #self.dbenvMaster.rep_start(flags=db.DB_REP_MASTER)
            self.dbenvMaster.rep_start(flags=db.DB_REP_CLIENT)
            self.dbenvClient.rep_start(flags=db.DB_REP_CLIENT)

            def thread_do(env, q, envid, election_status, must_be_master) :
                while True :
                    v=q.get()
                    if v == None : return
                    r = env.rep_process_message(v[0],v[1],envid)
                    if must_be_master and self.confirmed_master :
                        self.dbenvMaster.rep_start(flags = db.DB_REP_MASTER)
                        must_be_master = False

                    if r[0] == db.DB_REP_HOLDELECTION :
                        def elect() :
                            while True :
                                try :
                                    env.rep_elect(2, 1)
                                    election_status[0] = False
                                    break
                                except db.DBRepUnavailError :
                                    pass
                        if not election_status[0] and not self.confirmed_master :
                            from threading import Thread
                            election_status[0] = True
                            t=Thread(target=elect)
                            import sys
                            if sys.version_info[0] < 3 :
                                t.setDaemon(True)
                            else :
                                t.daemon = True
                            t.start()

            self.thread_do = thread_do

            self.t_m.start()
            self.t_c.start()

            self.dbenvMaster.rep_set_timeout(db.DB_REP_ELECTION_TIMEOUT, 50000)
            self.dbenvClient.rep_set_timeout(db.DB_REP_ELECTION_TIMEOUT, 50000)
            self.client_doing_election[0] = True
            while True :
                try :
                    self.dbenvClient.rep_elect(2, 1)
                    self.client_doing_election[0] = False
                    break
                except db.DBRepUnavailError :
                    pass

            self.assertTrue(self.confirmed_master)

#----------------------------------------------------------------------

def test_suite():
    suite = unittest.TestSuite()
    if db.version() >= (4, 6) :
        dbenv = db.DBEnv()
        try :
            dbenv.repmgr_get_ack_policy()
            ReplicationManager_available=True
        except :
            ReplicationManager_available=False
        dbenv.close()
        del dbenv
        if ReplicationManager_available :
            suite.addTest(unittest.makeSuite(DBReplicationManager))

        if have_threads :
            suite.addTest(unittest.makeSuite(DBBaseReplication))

    return suite


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
