import sqlite3
import unittest
from sqlite3 import _completer


class CompleterTests(unittest.TestCase):
    def test_schema_with_double_quote(self):
        con = sqlite3.connect(':memory:')
        self.addCleanup(con.close)
        con.execute('ATTACH DATABASE \':memory:\' AS \'weird"name\'')
        matches = []
        state = 0
        while True:
            match = _completer._complete(con, 'a', state)
            if match is None:
                break
            matches.append(match)
            state += 1
        self.assertTrue(matches)
