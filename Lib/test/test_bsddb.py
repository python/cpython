#! /usr/bin/env python
"""Test script for the bsddb C module by Roger E. Masse
   Adapted to unittest format and expanded scope by Raymond Hettinger
"""
import os, sys
import copy
import bsddb
import dbhash # Just so we know it's imported
import unittest
from test import test_support

class TestBSDDB(unittest.TestCase):
    openflag = 'c'

    def setUp(self):
        self.f = self.openmethod[0](self.fname, self.openflag, cachesize=32768)
        self.d = dict(q='Guido', w='van', e='Rossum', r='invented', t='Python', y='')
        for k, v in self.d.iteritems():
            self.f[k] = v

    def tearDown(self):
        self.f.sync()
        self.f.close()
        if self.fname is None:
            return
        try:
            os.remove(self.fname)
        except os.error:
            pass

    def test_getitem(self):
        for k, v in self.d.iteritems():
            self.assertEqual(self.f[k], v)

    def test_len(self):
        self.assertEqual(len(self.f), len(self.d))

    def test_change(self):
        self.f['r'] = 'discovered'
        self.assertEqual(self.f['r'], 'discovered')
        self.assert_('r' in self.f.keys())
        self.assert_('discovered' in self.f.values())

    def test_close_and_reopen(self):
        if self.fname is None:
            # if we're using an in-memory only db, we can't reopen it
            # so finish here.
            return
        self.f.close()
        self.f = self.openmethod[0](self.fname, 'w')
        for k, v in self.d.iteritems():
            self.assertEqual(self.f[k], v)

    def assertSetEquals(self, seqn1, seqn2):
        self.assertEqual(set(seqn1), set(seqn2))

    def test_mapping_iteration_methods(self):
        f = self.f
        d = self.d
        self.assertSetEquals(d, f)
        self.assertSetEquals(d.keys(), f.keys())
        self.assertSetEquals(d.values(), f.values())
        self.assertSetEquals(d.items(), f.items())
        self.assertSetEquals(d.iterkeys(), f.iterkeys())
        self.assertSetEquals(d.itervalues(), f.itervalues())
        self.assertSetEquals(d.iteritems(), f.iteritems())

    def test_iter_while_modifying_values(self):
        if not hasattr(self.f, '__iter__'):
            return

        di = iter(self.d)
        while 1:
            try:
                key = di.next()
                self.d[key] = 'modified '+key
            except StopIteration:
                break

        # it should behave the same as a dict.  modifying values
        # of existing keys should not break iteration.  (adding
        # or removing keys should)
        fi = iter(self.f)
        while 1:
            try:
                key = fi.next()
                self.f[key] = 'modified '+key
            except StopIteration:
                break

        self.test_mapping_iteration_methods()

    def test_iteritems_while_modifying_values(self):
        if not hasattr(self.f, 'iteritems'):
            return

        di = self.d.iteritems()
        while 1:
            try:
                k, v = di.next()
                self.d[k] = 'modified '+v
            except StopIteration:
                break

        # it should behave the same as a dict.  modifying values
        # of existing keys should not break iteration.  (adding
        # or removing keys should)
        fi = self.f.iteritems()
        while 1:
            try:
                k, v = fi.next()
                self.f[k] = 'modified '+v
            except StopIteration:
                break

        self.test_mapping_iteration_methods()

    def test_first_next_looping(self):
        items = [self.f.first()]
        for i in xrange(1, len(self.f)):
            items.append(self.f.next())
        self.assertSetEquals(items, self.d.items())

    def test_previous_last_looping(self):
        items = [self.f.last()]
        for i in xrange(1, len(self.f)):
            items.append(self.f.previous())
        self.assertSetEquals(items, self.d.items())

    def test_first_while_deleting(self):
        # Test for bug 1725856
        self.assert_(len(self.d) >= 2, "test requires >=2 items")
        for _ in self.d:
            key = self.f.first()[0]
            del self.f[key]
        self.assertEqual([], self.f.items(), "expected empty db after test")

    def test_last_while_deleting(self):
        # Test for bug 1725856's evil twin
        self.assert_(len(self.d) >= 2, "test requires >=2 items")
        for _ in self.d:
            key = self.f.last()[0]
            del self.f[key]
        self.assertEqual([], self.f.items(), "expected empty db after test")

    def test_set_location(self):
        self.assertEqual(self.f.set_location('e'), ('e', self.d['e']))

    def test_contains(self):
        for k in self.d:
            self.assert_(k in self.f)
        self.assert_('not here' not in self.f)

    def test_has_key(self):
        for k in self.d:
            self.assert_(self.f.has_key(k))
        self.assert_(not self.f.has_key('not here'))

    def test_clear(self):
        self.f.clear()
        self.assertEqual(len(self.f), 0)

    def test__no_deadlock_first(self, debug=0):
        # do this so that testers can see what function we're in in
        # verbose mode when we deadlock.
        sys.stdout.flush()

        # in pybsddb's _DBWithCursor this causes an internal DBCursor
        # object is created.  Other test_ methods in this class could
        # inadvertently cause the deadlock but an explicit test is needed.
        if debug: print "A"
        k,v = self.f.first()
        if debug: print "B", k
        self.f[k] = "deadlock.  do not pass go.  do not collect $200."
        if debug: print "C"
        # if the bsddb implementation leaves the DBCursor open during
        # the database write and locking+threading support is enabled
        # the cursor's read lock will deadlock the write lock request..

        # test the iterator interface (if present)
        if hasattr(self.f, 'iteritems'):
            if debug: print "D"
            i = self.f.iteritems()
            k,v = i.next()
            if debug: print "E"
            self.f[k] = "please don't deadlock"
            if debug: print "F"
            while 1:
                try:
                    k,v = i.next()
                except StopIteration:
                    break
            if debug: print "F2"

            i = iter(self.f)
            if debug: print "G"
            while i:
                try:
                    if debug: print "H"
                    k = i.next()
                    if debug: print "I"
                    self.f[k] = "deadlocks-r-us"
                    if debug: print "J"
                except StopIteration:
                    i = None
            if debug: print "K"

        # test the legacy cursor interface mixed with writes
        self.assert_(self.f.first()[0] in self.d)
        k = self.f.next()[0]
        self.assert_(k in self.d)
        self.f[k] = "be gone with ye deadlocks"
        self.assert_(self.f[k], "be gone with ye deadlocks")

    def test_for_cursor_memleak(self):
        if not hasattr(self.f, 'iteritems'):
            return

        # do the bsddb._DBWithCursor _iter_mixin internals leak cursors?
        nc1 = len(self.f._cursor_refs)
        # create iterator
        i = self.f.iteritems()
        nc2 = len(self.f._cursor_refs)
        # use the iterator (should run to the first yield, creating the cursor)
        k, v = i.next()
        nc3 = len(self.f._cursor_refs)
        # destroy the iterator; this should cause the weakref callback
        # to remove the cursor object from self.f._cursor_refs
        del i
        nc4 = len(self.f._cursor_refs)

        self.assertEqual(nc1, nc2)
        self.assertEqual(nc1, nc4)
        self.assert_(nc3 == nc1+1)

    def test_popitem(self):
        k, v = self.f.popitem()
        self.assert_(k in self.d)
        self.assert_(v in self.d.values())
        self.assert_(k not in self.f)
        self.assertEqual(len(self.d)-1, len(self.f))

    def test_pop(self):
        k = 'w'
        v = self.f.pop(k)
        self.assertEqual(v, self.d[k])
        self.assert_(k not in self.f)
        self.assert_(v not in self.f.values())
        self.assertEqual(len(self.d)-1, len(self.f))

    def test_get(self):
        self.assertEqual(self.f.get('NotHere'), None)
        self.assertEqual(self.f.get('NotHere', 'Default'), 'Default')
        self.assertEqual(self.f.get('q', 'Default'), self.d['q'])

    def test_setdefault(self):
        self.assertEqual(self.f.setdefault('new', 'dog'), 'dog')
        self.assertEqual(self.f.setdefault('r', 'cat'), self.d['r'])

    def test_update(self):
        new = dict(y='life', u='of', i='brian')
        self.f.update(new)
        self.d.update(new)
        for k, v in self.d.iteritems():
            self.assertEqual(self.f[k], v)

    def test_keyordering(self):
        if self.openmethod[0] is not bsddb.btopen:
            return
        keys = self.d.keys()
        keys.sort()
        self.assertEqual(self.f.first()[0], keys[0])
        self.assertEqual(self.f.next()[0], keys[1])
        self.assertEqual(self.f.last()[0], keys[-1])
        self.assertEqual(self.f.previous()[0], keys[-2])
        self.assertEqual(list(self.f), keys)

class TestBTree(TestBSDDB):
    fname = test_support.TESTFN
    openmethod = [bsddb.btopen]

class TestBTree_InMemory(TestBSDDB):
    fname = None
    openmethod = [bsddb.btopen]

class TestBTree_InMemory_Truncate(TestBSDDB):
    fname = None
    openflag = 'n'
    openmethod = [bsddb.btopen]

class TestHashTable(TestBSDDB):
    fname = test_support.TESTFN
    openmethod = [bsddb.hashopen]

class TestHashTable_InMemory(TestBSDDB):
    fname = None
    openmethod = [bsddb.hashopen]

##         # (bsddb.rnopen,'Record Numbers'), 'put' for RECNO for bsddb 1.85
##         #                                   appears broken... at least on
##         #                                   Solaris Intel - rmasse 1/97

def test_main(verbose=None):
    test_support.run_unittest(
        TestBTree,
        TestHashTable,
        TestBTree_InMemory,
        TestHashTable_InMemory,
        TestBTree_InMemory_Truncate,
    )

if __name__ == "__main__":
    test_main(verbose=True)
