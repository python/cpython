#!/usr/bin/env python
#
#-----------------------------------------------------------------------
# A test suite for the table interface built on bsddb.db
#-----------------------------------------------------------------------
#
# Copyright (C) 2000, 2001 by Autonomous Zone Industries
# Copyright (C) 2002 Gregory P. Smith
#
# March 20, 2000
#
# License:      This is free software.  You may use this software for any
#               purpose including modification/redistribution, so long as
#               this header remains intact and that you do not claim any
#               rights of ownership or authorship of this software.  This
#               software has been tested, but no warranty is expressed or
#               implied.
#
#   --  Gregory P. Smith <greg@krypto.org>
#
# $Id$

import shutil
import sys, os, re
import pickle
import tempfile

import unittest
from bsddb.test.test_all import verbose

try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db, dbtables
except ImportError:
    # For Python 2.3
    from bsddb import db, dbtables



#----------------------------------------------------------------------

class TableDBTestCase(unittest.TestCase):
    db_name = 'test-table.db'

    def setUp(self):
        self.homeDir = tempfile.mkdtemp()
        self.tdb = dbtables.bsdTableDB(
            filename='tabletest.db', dbhome=self.homeDir, create=1)

    def tearDown(self):
        self.tdb.close()
        shutil.rmtree(self.homeDir)

    def test01(self):
        tabname = "test01"
        colname = 'cool numbers'
        try:
            self.tdb.Drop(tabname)
        except dbtables.TableDBError:
            pass
        self.tdb.CreateTable(tabname, [colname])
        try:
            self.tdb.Insert(tabname, {colname: pickle.dumps(3.14159, 1)})
        except Exception:
            import traceback
            traceback.print_exc()

        if verbose:
            self.tdb._db_print()

        values = self.tdb.Select(
            tabname, [colname], conditions={colname: None})
        values = list(values)

        colval = pickle.loads(values[0][colname])
        self.assertTrue(colval > 3.141 and colval < 3.142)


    def test02(self):
        tabname = "test02"
        col0 = 'coolness factor'
        col1 = 'but can it fly?'
        col2 = 'Species'
        testinfo = [
            {col0: pickle.dumps(8, 1), col1: b'no', col2: b'Penguin'},
            {col0: pickle.dumps(-1, 1), col1: b'no', col2: b'Turkey'},
            {col0: pickle.dumps(9, 1), col1: b'yes', col2: b'SR-71A Blackbird'}
        ]

        try:
            self.tdb.Drop(tabname)
        except dbtables.TableDBError:
            pass
        self.tdb.CreateTable(tabname, [col0, col1, col2])
        for row in testinfo :
            self.tdb.Insert(tabname, row)

        values = self.tdb.Select(tabname, [col2],
            conditions={col0: lambda x: pickle.loads(x) >= 8})
        values = list(values)

        self.assertEquals(len(values), 2)
        if values[0]['Species'] == b'Penguin' :
            self.assertEquals(values[1]['Species'], b'SR-71A Blackbird')
        elif values[0]['Species'] == b'SR-71A Blackbird' :
            self.assertEquals(values[1]['Species'], b'Penguin')
        else :
            if verbose:
                print("values= %r" % (values,))
            self.fail("Wrong values returned!")

    def test03(self):
        tabname = "test03"
        try:
            self.tdb.Drop(tabname)
        except dbtables.TableDBError:
            pass
        if verbose:
            print('...before CreateTable...')
            self.tdb._db_print()
        self.tdb.CreateTable(tabname, ['a', 'b', 'c', 'd', 'e'])
        if verbose:
            print('...after CreateTable...')
            self.tdb._db_print()
        self.tdb.Drop(tabname)
        if verbose:
            print('...after Drop...')
            self.tdb._db_print()
        self.tdb.CreateTable(tabname, ['a', 'b', 'c', 'd', 'e'])

        try:
            self.tdb.Insert(tabname,
                            {'a': "",
                             'e': pickle.dumps([{4:5, 6:7}, 'foo'], 1),
                             'f': "Zero"})
            self.fail("exception not raised")
        except dbtables.TableDBError:
            pass

        try:
            self.tdb.Select(tabname, [], conditions={'foo': '123'})
            self.fail("exception not raised")
        except dbtables.TableDBError:
            pass

        self.tdb.Insert(tabname,
                        {'a': b'42',
                         'b': b'bad',
                         'c': b'meep',
                         'e': b'Fuzzy wuzzy was a bear'})
        self.tdb.Insert(tabname,
                        {'a': b'581750',
                         'b': b'good',
                         'd': b'bla',
                         'c': b'black',
                         'e': b'fuzzy was here'})
        self.tdb.Insert(tabname,
                        {'a': b'800000',
                         'b': b'good',
                         'd': b'bla',
                         'c': b'black',
                         'e': b'Fuzzy wuzzy is a bear'})

        if verbose:
            self.tdb._db_print()

        # this should return two rows
        values = self.tdb.Select(tabname, ['b', 'a', 'd'],
            conditions={'e': re.compile('wuzzy').search,
                        'a': re.compile('^[0-9]+$').match})
        self.assertEquals(len(values), 2)

        # now lets delete one of them and try again
        self.tdb.Delete(tabname, conditions={'b': dbtables.ExactCond('good')})
        values = self.tdb.Select(
            tabname, ['a', 'd', 'b'],
            conditions={'e': dbtables.PrefixCond('Fuzzy')})
        values = list(values)
        self.assertEquals(len(values), 1)
        self.assertEquals(values[0]['d'], None)

        values = self.tdb.Select(tabname, ['b'],
            conditions={'c': lambda c: c.decode("ascii") == 'meep'})
        values = list(values)
        self.assertEquals(len(values), 1)
        self.assertEquals(values[0]['b'], b"bad")


    def test04_MultiCondSelect(self):
        tabname = "test04_MultiCondSelect"
        try:
            self.tdb.Drop(tabname)
        except dbtables.TableDBError:
            pass
        self.tdb.CreateTable(tabname, ['a', 'b', 'c', 'd', 'e'])

        try:
            self.tdb.Insert(tabname,
                            {'a': b"",
                             'e': pickle.dumps([{4:5, 6:7}, 'foo'], 1),
                             'f': b"Zero"})
            self.fail("exception not raised")
        except dbtables.TableDBError:
            pass

        self.tdb.Insert(tabname, {'a': b"A", 'b': b"B", 'c': b"C",
                                  'd': b"D", 'e': b"E"})
        self.tdb.Insert(tabname, {'a': b"-A", 'b': b"-B", 'c': b"-C",
                                  'd': b"-D", 'e': b"-E"})
        self.tdb.Insert(tabname, {'a': b"A-", 'b': b"B-", 'c': b"C-",
                                  'd': b"D-", 'e': b"E-"})

        if verbose:
            self.tdb._db_print()

        # This select should return 0 rows.  it is designed to test
        # the bug identified and fixed in sourceforge bug # 590449
        # (Big Thanks to "Rob Tillotson (n9mtb)" for tracking this down
        # and supplying a fix!!  This one caused many headaches to say
        # the least...)
        values = self.tdb.Select(tabname, ['b', 'a', 'd'],
            conditions={'e': dbtables.ExactCond('E'),
                        'a': dbtables.ExactCond('A'),
                        'd': dbtables.PrefixCond('-')
                       } )
        self.assertEquals(len(values), 0, values)


    def test_CreateOrExtend(self):
        tabname = "test_CreateOrExtend"

        self.tdb.CreateOrExtendTable(
            tabname, ['name', 'taste', 'filling', 'alcohol content', 'price'])
        try:
            self.tdb.Insert(tabname,
                            {'taste': b'crap',
                             'filling': b'no',
                             'is it Guinness?': b'no'})
            self.fail("Insert should've failed due to bad column name")
        except:
            pass
        self.tdb.CreateOrExtendTable(tabname,
                                     ['name', 'taste', 'is it Guinness?'])

        # these should both succeed as the table should contain the union of both sets of columns.
        self.tdb.Insert(tabname, {'taste': b'crap', 'filling': b'no',
                                  'is it Guinness?': b'no'})
        self.tdb.Insert(tabname, {'taste': b'great', 'filling': b'yes',
                                  'is it Guinness?': b'yes',
                                  'name': b'Guinness'})


    def test_CondObjs(self):
        tabname = "test_CondObjs"

        self.tdb.CreateTable(tabname, ['a', 'b', 'c', 'd', 'e', 'p'])

        self.tdb.Insert(tabname, {'a': b"the letter A",
                                  'b': b"the letter B",
                                  'c': b"is for cookie"})
        self.tdb.Insert(tabname, {'a': b"is for aardvark",
                                  'e': b"the letter E",
                                  'c': b"is for cookie",
                                  'd': b"is for dog"})
        self.tdb.Insert(tabname, {'a': b"the letter A",
                                  'e': b"the letter E",
                                  'c': b"is for cookie",
                                  'p': b"is for Python"})

        values = self.tdb.Select(
            tabname, ['p', 'e'],
            conditions={'e': dbtables.PrefixCond('the l')})
        values = list(values)
        self.assertEquals(len(values), 2)
        self.assertEquals(values[0]['e'], values[1]['e'])
        self.assertNotEquals(values[0]['p'], values[1]['p'])

        values = self.tdb.Select(
            tabname, ['d', 'a'],
            conditions={'a': dbtables.LikeCond('%aardvark%')})
        values = list(values)
        self.assertEquals(len(values), 1)
        self.assertEquals(values[0]['d'], b"is for dog")
        self.assertEquals(values[0]['a'], b"is for aardvark")

        values = self.tdb.Select(tabname, None,
                                 {'b': dbtables.Cond(),
                                  'e':dbtables.LikeCond('%letter%'),
                                  'a':dbtables.PrefixCond('is'),
                                  'd':dbtables.ExactCond('is for dog'),
                                  'c':dbtables.PrefixCond('is for'),
                                  'p':lambda s: not s})
        values = list(values)
        self.assertEquals(len(values), 1)
        self.assertEquals(values[0]['d'], b"is for dog")
        self.assertEquals(values[0]['a'], b"is for aardvark")

    def test_Delete(self):
        tabname = "test_Delete"
        self.tdb.CreateTable(tabname, ['x', 'y', 'z'])

        # prior to 2001-05-09 there was a bug where Delete() would
        # fail if it encountered any rows that did not have values in
        # every column.
        # Hunted and Squashed by <Donwulff> (Jukka Santala - donwulff@nic.fi)
        self.tdb.Insert(tabname, {'x': b'X1', 'y':b'Y1'})
        self.tdb.Insert(tabname, {'x': b'X2', 'y':b'Y2', 'z': b'Z2'})

        self.tdb.Delete(tabname, conditions={'x': dbtables.PrefixCond('X')})
        values = self.tdb.Select(tabname, ['y'],
                                 conditions={'x': dbtables.PrefixCond('X')})
        self.assertEquals(len(values), 0)

    def test_Modify(self):
        tabname = "test_Modify"
        self.tdb.CreateTable(tabname, ['Name', 'Type', 'Access'])

        self.tdb.Insert(tabname, {'Name': b'Index to MP3 files.doc',
                                  'Type': b'Word', 'Access': b'8'})
        self.tdb.Insert(tabname, {'Name': b'Nifty.MP3', 'Access': b'1'})
        self.tdb.Insert(tabname, {'Type': b'Unknown', 'Access': b'0'})

        def set_type(type):
            if type == None:
                return b'MP3'
            return type

        def increment_access(count):
            return bytes(str(int(count)+1))

        def remove_value(value):
            return None

        self.tdb.Modify(tabname,
                        conditions={'Access': dbtables.ExactCond('0')},
                        mappings={'Access': remove_value})
        self.tdb.Modify(tabname,
                        conditions={'Name': dbtables.LikeCond('%MP3%')},
                        mappings={'Type': set_type})
        self.tdb.Modify(tabname,
                        conditions={'Name': dbtables.LikeCond('%')},
                        mappings={'Access': increment_access})

        try:
            self.tdb.Modify(tabname,
                            conditions={'Name': dbtables.LikeCond('%')},
                            mappings={'Access': b'What is your quest?'})
        except TypeError:
            # success, the string value in mappings isn't callable
            pass
        else:
            raise RuntimeError("why was TypeError not raised for bad callable?")

        # Delete key in select conditions
        values = self.tdb.Select(
            tabname, None,
            conditions={'Type': dbtables.ExactCond('Unknown')})
        values = list(values)
        self.assertEquals(len(values), 1)
        self.assertEquals(values[0]['Name'], None)
        self.assertEquals(values[0]['Access'], None)

        # Modify value by select conditions
        values = self.tdb.Select(
            tabname, None,
            conditions={'Name': dbtables.ExactCond('Nifty.MP3')})
        values = list(values)
        self.assertEquals(len(values), 1)
        self.assertEquals(values[0]['Type'], b"MP3")
        self.assertEquals(values[0]['Access'], b"2")

        # Make sure change applied only to select conditions
        values = self.tdb.Select(
            tabname, None, conditions={'Name': dbtables.LikeCond('%doc%')})
        values = list(values)
        self.assertEquals(len(values), 1)
        self.assertEquals(values[0]['Type'], b"Word")
        self.assertEquals(values[0]['Access'], b"9")


def test_suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TableDBTestCase))
    return suite


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
