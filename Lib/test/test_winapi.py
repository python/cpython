# Test the Windows-only _winapi module

import os
import pathlib
import re
import unittest
from test.support import import_helper

_winapi = import_helper.import_module('_winapi', required_on=['win'])

class WinAPITests(unittest.TestCase):
    def test_getlongpathname(self):
        testfn = pathlib.Path(os.getenv("ProgramFiles")).parents[-1] / "PROGRA~1"
        if not os.path.isdir(testfn):
            raise unittest.SkipTest("require x:\\PROGRA~1 to test")

        # pathlib.Path will be rejected - only str is accepted
        with self.assertRaises(TypeError):
            _winapi.GetLongPathName(testfn)

        actual = _winapi.GetLongPathName(os.fsdecode(testfn))

        # Can't assume that PROGRA~1 expands to any particular variation, so
        # ensure it matches any one of them.
        candidates = set(testfn.parent.glob("Progra*"))
        self.assertIn(pathlib.Path(actual), candidates)

    def test_getshortpathname(self):
        testfn = pathlib.Path(os.getenv("ProgramFiles"))
        if not os.path.isdir(testfn):
            raise unittest.SkipTest("require '%ProgramFiles%' to test")

        # pathlib.Path will be rejected - only str is accepted
        with self.assertRaises(TypeError):
            _winapi.GetShortPathName(testfn)

        actual = _winapi.GetShortPathName(os.fsdecode(testfn))

        # Should contain "PROGRA~" but we can't predict the number
        self.assertIsNotNone(re.match(r".\:\\PROGRA~\d", actual.upper()), actual)
