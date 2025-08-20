# pysqlite2/test/execute_json.py: tests for execute_json method
#
# Copyright (C) 2025 Python Software Foundation
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

import json
import sqlite3
import unittest



class ExecuteJsonTests(unittest.TestCase):
    def setUp(self):
        self.cx = sqlite3.connect(":memory:")
        self.cu = self.cx.cursor()
        self.cu.execute(
            "create table test(id integer primary key, name text, "
            "income number, unique_test text unique)"
        )
        self.cu.execute("insert into test(name, income) values (?, ?)", ("foo", 100))
        self.cu.execute("insert into test(name, income) values (?, ?)", ("bar", 200))
        self.cu.execute("insert into test(name, income) values (?, ?)", ("baz", 300))

    def tearDown(self):
        self.cu.close()
        self.cx.close()

    def test_execute_json_basic(self):
        # Test basic functionality of execute_json
        result = self.cu.execute_json("select * from test")
        data = json.loads(result)
        self.assertIsInstance(data, list)
        self.assertEqual(len(data), 3)
        self.assertEqual(data[0]["name"], "foo")
        self.assertEqual(data[0]["income"], 100)
        self.assertEqual(data[1]["name"], "bar")
        self.assertEqual(data[1]["income"], 200)
        self.assertEqual(data[2]["name"], "baz")
        self.assertEqual(data[2]["income"], 300)

    def test_execute_json_empty_result(self):
        # Test execute_json with empty result set
        result = self.cu.execute_json("select * from test where id > 1000")
        data = json.loads(result)
        self.assertIsInstance(data, list)
        self.assertEqual(len(data), 0)

    def test_execute_json_with_parameters(self):
        # Test execute_json with parameterized queries
        result = self.cu.execute_json("select * from test where income > ?", (150,))
        data = json.loads(result)
        self.assertIsInstance(data, list)
        self.assertEqual(len(data), 2)
        self.assertEqual(data[0]["name"], "bar")
        self.assertEqual(data[0]["income"], 200)
        self.assertEqual(data[1]["name"], "baz")
        self.assertEqual(data[1]["income"], 300)

    def test_execute_json_with_named_parameters(self):
        # Test execute_json with named parameters
        result = self.cu.execute_json("select * from test where income > :min_income",
                                     {"min_income": 150})
        data = json.loads(result)
        self.assertIsInstance(data, list)
        self.assertEqual(len(data), 2)
        self.assertEqual(data[0]["name"], "bar")
        self.assertEqual(data[0]["income"], 200)
        self.assertEqual(data[1]["name"], "baz")
        self.assertEqual(data[1]["income"], 300)

    def test_execute_json_invalid_sql(self):
        # Test execute_json with invalid SQL
        with self.assertRaises(sqlite3.OperationalError):
            self.cu.execute_json("select asdf")

    def test_execute_json_non_select(self):
        # Test execute_json with non-SELECT statement
        result = self.cu.execute_json("insert into test(name, income) values (?, ?)",
                                     ("new_entry", 400))
        data = json.loads(result)
        self.assertIsInstance(data, list)
        self.assertEqual(len(data), 0)


if __name__ == "__main__":
    unittest.main()
