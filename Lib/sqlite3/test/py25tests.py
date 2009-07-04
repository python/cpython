#-*- coding: ISO-8859-1 -*-
# pysqlite2/test/regression.py: pysqlite regression tests
#
# Copyright (C) 2007 Gerhard Häring <gh@ghaering.de>
#
# This file is part of pysqlite.
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

from __future__ import with_statement
import unittest
import sqlite3 as sqlite

did_rollback = False

class MyConnection(sqlite.Connection):
    def rollback(self):
        global did_rollback
        did_rollback = True
        sqlite.Connection.rollback(self)

class ContextTests(unittest.TestCase):
    def setUp(self):
        global did_rollback
        self.con = sqlite.connect(":memory:", factory=MyConnection)
        self.con.execute("create table test(c unique)")
        did_rollback = False

    def tearDown(self):
        self.con.close()

    def CheckContextManager(self):
        """Can the connection be used as a context manager at all?"""
        with self.con:
            pass

    def CheckContextManagerCommit(self):
        """Is a commit called in the context manager?"""
        with self.con:
            self.con.execute("insert into test(c) values ('foo')")
        self.con.rollback()
        count = self.con.execute("select count(*) from test").fetchone()[0]
        self.assertEqual(count, 1)

    def CheckContextManagerRollback(self):
        """Is a rollback called in the context manager?"""
        global did_rollback
        self.assertEqual(did_rollback, False)
        try:
            with self.con:
                self.con.execute("insert into test(c) values (4)")
                self.con.execute("insert into test(c) values (4)")
        except sqlite.IntegrityError:
            pass
        self.assertEqual(did_rollback, True)

def suite():
    ctx_suite = unittest.makeSuite(ContextTests, "Check")
    return unittest.TestSuite((ctx_suite,))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
