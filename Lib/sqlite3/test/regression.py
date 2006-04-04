#-*- coding: ISO-8859-1 -*-
# pysqlite2/test/regression.py: pysqlite regression tests
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

import unittest
import pysqlite2.dbapi2 as sqlite

class RegressionTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def tearDown(self):
        self.con.close()

    def CheckPragmaUserVersion(self):
        # This used to crash pysqlite because this pragma command returns NULL for the column name
        cur = self.con.cursor()
        cur.execute("pragma user_version")

def suite():
    regression_suite = unittest.makeSuite(RegressionTests, "Check")
    return unittest.TestSuite((regression_suite,))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
