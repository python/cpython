import unittest
from test.support import import_helper

ac_tester = import_helper.import_module('_testclinic')


class TestClinicFunctionality(unittest.TestCase):
    def test_gh_32092_oob(self):
        res = ac_tester.gh_32092_oob(1, 2, 3, 4, kw1=5, kw2=6)
        expect = (1, 2, (3, 4), 5, 6)
        self.assertEqual(res, expect)

    def test_gh_32092_kw_pass(self):
        res = ac_tester.gh_32092_kw_pass(1, 2, 3)
        expect = (1, (2, 3), None)
        self.assertEqual(res, expect)


class TestClinicFunctionalityC(unittest.TestCase):
    locals().update((name, getattr(ac_tester, name))
                    for name in dir(ac_tester) if name.startswith('test_'))
