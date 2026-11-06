import unittest
from test.support import import_helper, threading_helper

resource = import_helper.import_module("resource")


NTHREADS = 10
LOOP_PER_THREAD = 1000


@threading_helper.requires_working_threading()
class ResourceTest(unittest.TestCase):
    @unittest.skipUnless(hasattr(resource, "getrusage"), "needs getrusage")
    @unittest.skipUnless(
        hasattr(resource, "RUSAGE_THREAD"), "needs RUSAGE_THREAD"
    )
    def test_getrusage(self):
        ru_utime_lst = []

        def dummy_work(ru_utime_lst):
            for _ in range(LOOP_PER_THREAD):
                pass

            usage_process = resource.getrusage(resource.RUSAGE_SELF)
            usage_thread = resource.getrusage(resource.RUSAGE_THREAD)
            # Process user time should be greater than thread user time
            self.assertGreater(usage_process.ru_utime, usage_thread.ru_utime)
            ru_utime_lst.append(usage_thread.ru_utime)

        threading_helper.run_concurrently(
            worker_func=dummy_work, args=(ru_utime_lst,), nthreads=NTHREADS
        )

        usage_process = resource.getrusage(resource.RUSAGE_SELF)
        self.assertEqual(len(ru_utime_lst), NTHREADS)
        # Process user time should be greater than sum of all thread user times
        self.assertGreater(usage_process.ru_utime, sum(ru_utime_lst))


if __name__ == "__main__":
    unittest.main()
