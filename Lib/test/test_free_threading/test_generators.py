import unittest
import concurrent.futures

from unittest import TestCase

from test.support import threading_helper

@threading_helper.requires_working_threading()
class TestGen(TestCase):
    def test_generators_basic(self):
        def gen():
            for _ in range(5000):
                yield


        it = gen()
        with concurrent.futures.ThreadPoolExecutor() as executor:
            for _ in range(5000):
                executor.submit(lambda: next(it))


if __name__ == "__main__":
    unittest.main()
