import interpreters
import _interpreters #remove when all methods are implemented
from collections import namedtuple
import contextlib
import itertools
import os
import pickle
import sys
from textwrap import dedent
import threading
import time
import unittest

from test import support
from test.support import script_helper

def _captured_script(script):
    r, w = os.pipe()
    indented = script.replace('\n', '\n                ')
    wrapped = dedent(f"""
        import contextlib
        with open({w}, 'w') as spipe:
            with contextlib.redirect_stdout(spipe):
                {indented}
        """)
    return wrapped, open(r)

def clean_up_interpreters():
    for id in interpreters.list_all():
        if id == 0:  # main
            continue
        try:
            interpreters.destroy(id)
        except RuntimeError:
            pass  # already destroyed


class TestBase(unittest.TestCase):

    def tearDown(self):
        clean_up_interpreters()


class ListAllTests(TestBase):

    def test_initial(self):
        main = interpreters.get_main()
        ids = interpreters.list_all()
        self.assertEqual(ids, [main])


class GetCurrentTests(TestBase):

    def test_get_current(self):
        main = interpreters.get_main()
        cur = interpreters.get_current()
        self.assertEqual(cur, main)


class GetMainTests(TestBase):

    def test_get_main(self):
        [expected] = interpreters.list_all()
        main = interpreters.get_main()
        self.assertEqual(main, expected)


class CreateTests(TestBase):

    def test_create(self):
        id = interpreters.create()
        self.assertIn(id, interpreters.list_all())

class DestroyTests(TestBase):

    def test_destroy(self):
        id1 = interpreters.create()
        id2 = interpreters.create()
        id3 = interpreters.create()
        self.assertIn(id2, interpreters.list_all())
        interpreters.destroy(id2)
        self.assertNotIn(id2, interpreters.list_all())


class RunStringTests(TestBase):

    def test_run_string(self):
        script, file = _captured_script('print("it worked!", end="")')
        id = interpreters.create()
        with file:
            interpreters.run_string(id, script, None)
            out = file.read()

        self.assertEqual(out, 'it worked!')


if __name__ == '__main__':
    unittest.main()
