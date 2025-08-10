import unittest

from test.support import threading_helper
from test.support.threading_helper import run_concurrently

from test import test_pwd


NTHREADS = 10


@threading_helper.requires_working_threading()
class TestPwd(unittest.TestCase):
    def setUp(self):
        self.test_pwd = test_pwd.PwdTest()

    def test_racing_test_values(self):
        # test_pwd.test_values() calls pwd.getpwall() and checks the entries
        run_concurrently(
            worker_func=self.test_pwd.test_values, nthreads=NTHREADS
        )

    def test_racing_test_values_extended(self):
        # test_pwd.test_values_extended() calls pwd.getpwall(), pwd.getpwnam(),
        # pwd.getpwduid() and checks the entries
        run_concurrently(
            worker_func=self.test_pwd.test_values_extended,
            nthreads=NTHREADS,
        )


if __name__ == "__main__":
    unittest.main()
