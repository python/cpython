#-*- coding: ISO-8859-1 -*-
# pysqlite2/test/hooks.py: tests for various SQLite-specific hooks
#
# Copyright (C) 2006 Gerhard Häring <gh@ghaering.de>
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

import os, unittest
import sqlite3 as sqlite

class CollationTests(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def CheckCreateCollationNotCallable(self):
        con = sqlite.connect(":memory:")
        try:
            con.create_collation("X", 42)
            self.fail("should have raised a TypeError")
        except TypeError, e:
            self.failUnlessEqual(e.args[0], "parameter must be callable")

    def CheckCreateCollationNotAscii(self):
        con = sqlite.connect(":memory:")
        try:
            con.create_collation("collä", cmp)
            self.fail("should have raised a ProgrammingError")
        except sqlite.ProgrammingError, e:
            pass

    def CheckCollationIsUsed(self):
        if sqlite.version_info < (3, 2, 1):  # old SQLite versions crash on this test
            return
        def mycoll(x, y):
            # reverse order
            return -cmp(x, y)

        con = sqlite.connect(":memory:")
        con.create_collation("mycoll", mycoll)
        sql = """
            select x from (
            select 'a' as x
            union
            select 'b' as x
            union
            select 'c' as x
            ) order by x collate mycoll
            """
        result = con.execute(sql).fetchall()
        if result[0][0] != "c" or result[1][0] != "b" or result[2][0] != "a":
            self.fail("the expected order was not returned")

        con.create_collation("mycoll", None)
        try:
            result = con.execute(sql).fetchall()
            self.fail("should have raised an OperationalError")
        except sqlite.OperationalError, e:
            self.failUnlessEqual(e.args[0].lower(), "no such collation sequence: mycoll")

    def CheckCollationRegisterTwice(self):
        """
        Register two different collation functions under the same name.
        Verify that the last one is actually used.
        """
        con = sqlite.connect(":memory:")
        con.create_collation("mycoll", cmp)
        con.create_collation("mycoll", lambda x, y: -cmp(x, y))
        result = con.execute("""
            select x from (select 'a' as x union select 'b' as x) order by x collate mycoll
            """).fetchall()
        if result[0][0] != 'b' or result[1][0] != 'a':
            self.fail("wrong collation function is used")

    def CheckDeregisterCollation(self):
        """
        Register a collation, then deregister it. Make sure an error is raised if we try
        to use it.
        """
        con = sqlite.connect(":memory:")
        con.create_collation("mycoll", cmp)
        con.create_collation("mycoll", None)
        try:
            con.execute("select 'a' as x union select 'b' as x order by x collate mycoll")
            self.fail("should have raised an OperationalError")
        except sqlite.OperationalError, e:
            if not e.args[0].startswith("no such collation sequence"):
                self.fail("wrong OperationalError raised")

def suite():
    collation_suite = unittest.makeSuite(CollationTests, "Check")
    return unittest.TestSuite((collation_suite,))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
