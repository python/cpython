import unittest

from unittest import TestCase

from test import support
from test.support import threading_helper
from threading import Thread

@threading_helper.requires_working_threading()
@support.requires_resource("cpu")
class TestGen(TestCase):
    def infinite_generator(self):
        def gen():
            while True:
                yield

        return gen()

    def stress_generator(self, *funcs):
        threads = []
        gen = self.infinite_generator()

        for func in funcs:
            for _ in range(10):
                def with_iterations(gen):
                    for _ in range(100):
                        func(gen)

                threads.append(Thread(target=with_iterations, args=(gen,)))

        with threading_helper.catch_threading_exception():
            # Errors might come up, but that's fine.
            # All we care about is that this doesn't crash.
            for thread in threads:
                thread.start()

            for thread in threads:
                thread.join()

    def test_generator_send(self):
        self.stress_generator(lambda gen: next(gen))

    def test_generator_close(self):
        self.stress_generator(lambda gen: gen.close())
        self.stress_generator(lambda gen: next(gen), lambda gen: gen.close())

    def test_generator_state(self):
        self.stress_generator(lambda gen: next(gen), lambda gen: gen.gi_running)
        self.stress_generator(lambda gen: gen.gi_isrunning, lambda gen: gen.close())

if __name__ == "__main__":
    unittest.main()
