import unittest
from test.support import threading_helper

threading_helper.requires_working_threading(module=True)


class TestUnicodeFreeThreading(unittest.TestCase):
    @threading_helper.reap_threads
    def test_maketrans_dict_concurrent_modification(self):
        number_of_iterations = 5
        number_of_attempts = 100
        for _ in range(number_of_iterations):
            d = {2000: 'a'}
            
            def work_iterator(dct):
                for i in range(number_of_attempts):
                    str.maketrans(dct)
                    idx = 2000 + i
                    dct[idx] = chr(idx % 16)
                    dct.pop(idx, None)

            threading_helper.run_concurrently(
                work_iterator,
                nthreads=5,
                args=(d,),
            )