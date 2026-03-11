import concurrent.futures
import itertools
import threading
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

    def test_concurrent_send(self):
        def gen():
            yield 1
            yield 2
            yield 3
            yield 4
            yield 5

        def run_test(drive_generator):
            g = gen()
            values = []
            threading_helper.run_concurrently(drive_generator, self.NUM_THREADS, args=(g, values,))
            self.assertEqual(sorted(values), [1, 2, 3, 4, 5])

        def call_next(g, values):
            while True:
                try:
                    values.append(next(g))
                except ValueError:
                    continue
                except StopIteration:
                    break

        with self.subTest(method='next'):
            run_test(call_next)

        def call_send(g, values):
            while True:
                try:
                    values.append(g.send(None))
                except ValueError:
                    continue
                except StopIteration:
                    break

        with self.subTest(method='send'):
            run_test(call_send)

        def for_iter_gen(g, values):
            while True:
                try:
                    for value in g:
                        values.append(value)
                    else:
                        break
                except ValueError:
                    continue

        with self.subTest(method='for'):
            run_test(for_iter_gen)

    def test_concurrent_close(self):
        def gen():
            for i in range(10):
                yield i
                time.sleep(0.001)

        def drive_generator(g):
            while True:
                try:
                    for value in g:
                        if value == 5:
                            g.close()
                    else:
                        return
                except ValueError as e:
                    self.assertEqual(e.args[0], "generator already executing")

        g = gen()
        threading_helper.run_concurrently(drive_generator, self.NUM_THREADS, args=(g,))

    def test_concurrent_gi_yieldfrom(self):
        def gen_yield_from():
            yield from itertools.count()

        g = gen_yield_from()
        next(g)  # Put in FRAME_SUSPENDED_YIELD_FROM state

        def read_yieldfrom(gen):
            for _ in range(10000):
                self.assertIsNotNone(gen.gi_yieldfrom)

        threading_helper.run_concurrently(read_yieldfrom, self.NUM_THREADS, args=(g,))

    def test_gi_yieldfrom_close_race(self):
        def gen_yield_from():
            yield from itertools.count()

        g = gen_yield_from()
        next(g)

        done = threading.Event()

        def reader():
            while not done.is_set():
                g.gi_yieldfrom

        def closer():
            try:
                g.close()
            except ValueError:
                pass
            done.set()

        threading_helper.run_concurrently([reader, closer])
