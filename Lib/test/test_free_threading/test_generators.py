import unittest

from unittest import TestCase

from test import support
from test.support import threading_helper
from test.support import import_helper
from threading import Thread
import types

@threading_helper.requires_working_threading()
@support.requires_resource("cpu")
class GeneratorFreeThreadingTests(TestCase):
    def infinite_generator(self):
        def gen():
            while True:
                yield

        return gen()

    def infinite_coroutine(self, from_gen: bool = False):
        asyncio = import_helper.import_module("asyncio")

        if from_gen:
            @types.coroutine
            def coro():
                while True:
                    yield from asyncio.sleep(0)
        else:
            async def coro():
                while True:
                    await asyncio.sleep(0)

        return coro()

    def infinite_asyncgen(self):
        asyncio = import_helper.import_module("asyncio")

        async def async_gen():
            while True:
                await asyncio.sleep(0)
                yield 42

        return async_gen()

    def _stress_object(self, gen, *funcs):
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
        self._stress_object(self.infinite_generator(), *funcs)

    def stress_coroutine(self, *funcs, from_gen: bool = False):
        self._stress_object(self.infinite_coroutine(from_gen=from_gen), *funcs)

    def stress_asyncgen(self, *funcs):
        self._stress_object(self.infinite_asyncgen(), *funcs)

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

    def test_generator_coroutine(self):
        self.stress_coroutine(lambda coro: next(coro), from_gen=True)

    def test_async_gen(self):
        self.stress_asyncgen(lambda ag: next(ag), lambda ag: ag.send(None))
        self.stress_asyncgen(lambda ag: next(ag), lambda ag: ag.throw(RuntimeError))
        self.stress_asyncgen(lambda ag: next(ag), lambda ag: ag.close())
        self.stress_asyncgen(lambda ag: ag.throw(RuntimeError), lambda ag: ag.close())

if __name__ == "__main__":
    unittest.main()
