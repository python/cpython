import unittest

from test.support import threading_helper

import cProfile
import pstats


NTHREADS = 10
INSERT_PER_THREAD = 1000


@threading_helper.requires_working_threading()
class TestCProfile(unittest.TestCase):
    def test_cprofile_racing_list_insert(self):
        def list_insert(lst):
            for i in range(INSERT_PER_THREAD):
                lst.insert(0, i)

        lst = []

        with cProfile.Profile() as pr:
            threading_helper.run_concurrently(
                worker_func=list_insert, nthreads=NTHREADS, args=(lst,)
            )
            pr.create_stats()
            ps = pstats.Stats(pr)
            stats_profile = ps.get_stats_profile()
            list_insert_profile = stats_profile.func_profiles[
                "<method 'insert' of 'list' objects>"
            ]
            # Even though there is no explicit recursive call to insert,
            # cProfile may record some calls as recursive due to limitations
            # in its handling of multithreaded programs. This issue is not
            # directly related to FT Python itself; however, it tends to be
            # more noticeable when using FT Python. Therefore, consider only
            # the calls section and disregard the recursive part.
            list_insert_ncalls = list_insert_profile.ncalls.split("/")[0]
            self.assertEqual(
                int(list_insert_ncalls), NTHREADS * INSERT_PER_THREAD
            )

        self.assertEqual(len(lst), NTHREADS * INSERT_PER_THREAD)
