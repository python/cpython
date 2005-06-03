"""
TestCases for python DB Btree key comparison function.
"""

import sys, os
import test_all

import unittest
from bsddb3 import db

def lexical_cmp (db, left, right):
  return cmp (left, right)

def lowercase_cmp(db, left, right):
  return cmp (left.lower(), right.lower())

def make_reverse_comparator (cmp):
  def reverse (db, left, right, delegate=cmp):
    return - delegate (db, left, right)
  return reverse

_expected_lexical_test_data = ['', 'CCCP', 'a', 'aaa', 'b', 'c', 'cccce', 'ccccf']
_expected_lowercase_test_data = ['', 'a', 'aaa', 'b', 'c', 'CC', 'cccce', 'ccccf', 'CCCP']

class ComparatorTests (unittest.TestCase):
  def comparator_test_helper (self, comparator, expected_data):
    data = expected_data[:]
    data.sort (lambda l, r, cmp=comparator: cmp (None, l, r))
    self.failUnless (data == expected_data,
                     "comparator `%s' is not right: %s vs. %s"
                     % (comparator, expected_data, data))
  def test_lexical_comparator (self):
    self.comparator_test_helper (lexical_cmp, _expected_lexical_test_data)
  def test_reverse_lexical_comparator (self):
    rev = _expected_lexical_test_data[:]
    rev.reverse ()
    self.comparator_test_helper (make_reverse_comparator (lexical_cmp),
                                 rev)
  def test_lowercase_comparator (self):
    self.comparator_test_helper (lowercase_cmp,
                                 _expected_lowercase_test_data)

class AbstractBtreeKeyCompareTestCase (unittest.TestCase):
  env = None
  db = None
  
  def setUp (self):
    self.filename = self.__class__.__name__ + '.db'
    homeDir = os.path.join (os.path.dirname (sys.argv[0]), 'db_home')
    self.homeDir = homeDir
    try:
      os.mkdir (homeDir)
    except os.error:
      pass

    env = db.DBEnv ()
    env.open (homeDir,
              db.DB_CREATE | db.DB_INIT_MPOOL
              | db.DB_INIT_LOCK | db.DB_THREAD)
    self.env = env

  def tearDown (self):
    self.closeDB ()
    if self.env is not None:
      self.env.close ()
      self.env = None
    import glob
    map (os.remove, glob.glob (os.path.join (self.homeDir, '*')))

  def addDataToDB (self, data):
    i = 0
    for item in data:
      self.db.put (item, str (i))
      i = i + 1

  def createDB (self, key_comparator):
    self.db = db.DB (self.env)
    self.setupDB (key_comparator)
    self.db.open (self.filename, "test", db.DB_BTREE, db.DB_CREATE)

  def setupDB (self, key_comparator):
    self.db.set_bt_compare (key_comparator)

  def closeDB (self):
    if self.db is not None:
      self.db.close ()
      self.db = None

  def startTest (self):
    pass
  
  def finishTest (self, expected = None):
    if expected is not None:
      self.check_results (expected)
    self.closeDB ()

  def check_results (self, expected):
    curs = self.db.cursor ()
    try:
      index = 0
      rec = curs.first ()
      while rec:
        key, ignore = rec
        self.failUnless (index < len (expected),
                         "to many values returned from cursor")
        self.failUnless (expected[index] == key,
                         "expected value `%s' at %d but got `%s'"
                         % (expected[index], index, key))
        index = index + 1
        rec = curs.next ()
      self.failUnless (index == len (expected),
                       "not enough values returned from cursor")
    finally:
      curs.close ()

class BtreeKeyCompareTestCase (AbstractBtreeKeyCompareTestCase):
  def runCompareTest (self, comparator, data):
    self.startTest ()
    self.createDB (comparator)
    self.addDataToDB (data)
    self.finishTest (data)

  def test_lexical_ordering (self):
    self.runCompareTest (lexical_cmp, _expected_lexical_test_data)

  def test_reverse_lexical_ordering (self):
    expected_rev_data = _expected_lexical_test_data[:]
    expected_rev_data.reverse ()
    self.runCompareTest (make_reverse_comparator (lexical_cmp),
                         expected_rev_data)

  def test_compare_function_useless (self):
    self.startTest ()
    def socialist_comparator (db, l, r):
      return 0
    self.createDB (socialist_comparator)
    self.addDataToDB (['b', 'a', 'd'])
    # all things being equal the first key will be the only key
    # in the database...  (with the last key's value fwiw)
    self.finishTest (['b'])

    
class BtreeExceptionsTestCase (AbstractBtreeKeyCompareTestCase):
  def test_raises_non_callable (self):
    self.startTest ()
    self.assertRaises (TypeError, self.createDB, 'abc')
    self.assertRaises (TypeError, self.createDB, None)
    self.finishTest ()

  def test_set_bt_compare_with_function (self):
    self.startTest ()
    self.createDB (lexical_cmp)
    self.finishTest ()

  def check_results (self, results):
    pass

  def test_compare_function_incorrect (self):
    self.startTest ()
    def bad_comparator (db, l, r):
      return 1
    # verify that set_bt_compare checks that comparator(db, '', '') == 0
    self.assertRaises (TypeError, self.createDB, bad_comparator)
    self.finishTest ()

  def test_compare_function_exception (self):
    self.startTest ()
    def bad_comparator (db, l, r):
      if l == r:
	# pass the set_bt_compare test
	return 0
      raise RuntimeError, "i'm a naughty comparison function"
    self.createDB (bad_comparator)
    print "\n*** this test should print 2 uncatchable tracebacks ***"
    self.addDataToDB (['a', 'b', 'c'])  # this should raise, but...
    self.finishTest ()

  def test_compare_function_bad_return (self):
    self.startTest ()
    def bad_comparator (db, l, r):
      if l == r:
	# pass the set_bt_compare test
	return 0
      return l
    self.createDB (bad_comparator)
    print "\n*** this test should print 2 errors about returning an int ***"
    self.addDataToDB (['a', 'b', 'c'])  # this should raise, but...
    self.finishTest ()


  def test_cannot_assign_twice (self):

    def my_compare (db, a, b):
      return 0
    
    self.startTest ()
    self.createDB (my_compare)
    try:
      self.db.set_bt_compare (my_compare)
      assert False, "this set should fail"

    except RuntimeError, msg:
      pass
    
def test_suite ():
  res = unittest.TestSuite ()

  res.addTest (unittest.makeSuite (ComparatorTests))
  if db.version () >= (3, 3, 11):
    res.addTest (unittest.makeSuite (BtreeExceptionsTestCase))
    res.addTest (unittest.makeSuite (BtreeKeyCompareTestCase))
  return res

if __name__ == '__main__':
  unittest.main (defaultTest = 'suite')
