import unittest

from test.support import import_helper, threading_helper
from test.support.threading_helper import run_concurrently

grp = import_helper.import_module("grp")

from test import test_grp


NTHREADS = 10


@threading_helper.requires_working_threading()
class TestGrp(unittest.TestCase):
    def setUp(self):
        self.test_grp = test_grp.GroupDatabaseTestCase()

    def test_racing_test_values(self):
        # test_grp.test_values() calls grp.getgrall() and checks the entries
        run_concurrently(
            worker_func=self.test_grp.test_values, nthreads=NTHREADS
        )

    def test_racing_test_values_extended(self):
        # test_grp.test_values_extended() calls grp.getgrall(), grp.getgrgid(),
        # grp.getgrnam() and checks the entries
        run_concurrently(
            worker_func=self.test_grp.test_values_extended,
            nthreads=NTHREADS,
        )


if __name__ == "__main__":
    unittest.main()
