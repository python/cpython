import unittest

from unittest import TestCase

from test import support
from test.support import threading_helper
from test.support import import_helper
from threading import Thread

@threading_helper.requires_working_threading()
@support.requires_resource("cpu")
class TestGen(TestCase):
    def infinite_generator(self):
        def gen():
            while True:
                yield

        return gen()

    def infinite_coroutine(self):
        asyncio = import_helper.import_module("asyncio")

        async def coro():
            while True:
                await asyncio.sleep(0)

        return coro()

    def _stress_genlike(self, gen, *funcs):
        threads = []

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

    def stress_generator(self, *funcs):
        self._stress_genlike(self.infinite_generator(), *funcs)

    def stress_coroutine(self, *funcs):
        self._stress_genlike(self.infinite_coroutine(), *funcs)

    def test_generator_send(self):
        self.stress_generator(lambda gen: next(gen))

    def test_generator_close(self):
        self.stress_generator(lambda gen: gen.close())
        self.stress_generator(lambda gen: next(gen), lambda gen: gen.close())

    def test_generator_state(self):
        self.stress_generator(lambda gen: next(gen), lambda gen: gen.gi_running)
        self.stress_generator(lambda gen: gen.gi_isrunning, lambda gen: gen.close())

    def test_coroutine_send(self):
        def try_send_non_none(coro):
            try:
                coro.send(42)
            except ValueError:
                # Can't send non-None to just started coroutine
                coro.send(None)

        self.stress_coroutine(lambda coro: next(coro))
        self.stress_coroutine(try_send_non_none)

    def test_coroutine_attributes(self):
        self.stress_coroutine(
            lambda coro: next(coro),
            lambda coro: coro.cr_frame,
            lambda coro: coro.cr_suspended,
        )

if __name__ == "__main__":
    unittest.main()
