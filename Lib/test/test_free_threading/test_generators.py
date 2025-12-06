import concurrent.futures
import unittest
from threading import Barrier
from unittest import TestCase
import random
import time

from test.support import threading_helper, Py_GIL_DISABLED

threading_helper.requires_working_threading(module=True)


def random_sleep():
    delay_us = random.randint(50, 100)
    time.sleep(delay_us * 1e-6)

def random_string():
    return ''.join(random.choice('0123456789ABCDEF') for _ in range(10))

def set_gen_name(g, b):
    b.wait()
    random_sleep()
    g.__name__ = random_string()
    return g.__name__

def set_gen_qualname(g, b):
    b.wait()
    random_sleep()
    g.__qualname__ = random_string()
    return g.__qualname__


@unittest.skipUnless(Py_GIL_DISABLED, "Enable only in FT build")
class TestFTGenerators(TestCase):
    NUM_THREADS = 4

    def concurrent_write_with_func(self, func):
        gen = (x for x in range(42))
        for j in range(1000):
            with concurrent.futures.ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
                b = Barrier(self.NUM_THREADS)
                futures = {executor.submit(func, gen, b): i for i in range(self.NUM_THREADS)}
                for fut in concurrent.futures.as_completed(futures):
                    gen_name = fut.result()
                    self.assertEqual(len(gen_name), 10)

    def test_concurrent_write(self):
        with self.subTest(func=set_gen_name):
            self.concurrent_write_with_func(func=set_gen_name)
        with self.subTest(func=set_gen_qualname):
            self.concurrent_write_with_func(func=set_gen_qualname)
