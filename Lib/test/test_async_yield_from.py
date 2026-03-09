"""
Test suite for PEP 828 implementation

Adapted from the 'yield from' tests.
"""

import unittest
import inspect
from functools import partial

from test.support import captured_stderr, disable_gc, gc_collect, run_yielding_async_fn

_async_test = partial(partial, run_yielding_async_fn)


class TestPEP828Operation(unittest.TestCase):
    """
    Test semantics.
    """

    @_async_test
    async def test_delegation_of_initial_anext_to_subgenerator(self):
        """
        Test delegation of initial anext() call to subgenerator
        """
        trace = []
        async def g1():
            trace.append("Starting g1")
            async yield from g2()
            trace.append("Finishing g1")
        async def g2():
            trace.append("Starting g2")
            yield 42
            trace.append("Finishing g2")
        async for x in g1():
            trace.append("Yielded %s" % (x,))
        self.assertEqual(trace,[
            "Starting g1",
            "Starting g2",
            "Yielded 42",
            "Finishing g2",
            "Finishing g1",
        ])

    @_async_test
    async def test_raising_exception_in_initial_anext_call(self):
        """
        Test raising exception in initial anext() call
        """
        trace = []
        async def g1():
            try:
                trace.append("Starting g1")
                async yield from g2()
            finally:
                trace.append("Finishing g1")
        async def g2():
            try:
                trace.append("Starting g2")
                yield from ()
                raise ValueError("spanish inquisition occurred")
            finally:
                trace.append("Finishing g2")
        try:
            async for x in g1():
                trace.append("Yielded %s" % (x,))
        except ValueError as e:
            self.assertEqual(e.args[0], "spanish inquisition occurred")
        else:
            self.fail("subgenerator failed to raise ValueError")
        self.assertEqual(trace, [
            "Starting g1",
            "Starting g2",
            "Finishing g2",
            "Finishing g1",
        ])

    @_async_test
    async def test_delegation_of_anext_call_to_subgenerator(self):
        """
        Test delegation of anext() call to subgenerator
        """
        trace = []
        async def g1():
            trace.append("Starting g1")
            yield "g1 ham"
            async yield from g2()
            yield "g1 eggs"
            trace.append("Finishing g1")
        async def g2():
            trace.append("Starting g2")
            yield "g2 spam"
            yield "g2 more spam"
            trace.append("Finishing g2")
        async for x in g1():
            trace.append("Yielded %s" % (x,))
        self.assertEqual(trace,[
            "Starting g1",
            "Yielded g1 ham",
            "Starting g2",
            "Yielded g2 spam",
            "Yielded g2 more spam",
            "Finishing g2",
            "Yielded g1 eggs",
            "Finishing g1",
        ])

    @_async_test
    async def test_raising_exception_in_delegated_anext_call(self):
        """
        Test raising exception in delegated anext() call
        """
        trace = []
        async def g1():
            try:
                trace.append("Starting g1")
                yield "g1 ham"
                async yield from g2()
                yield "g1 eggs"
            finally:
                trace.append("Finishing g1")
        async def g2():
            try:
                trace.append("Starting g2")
                yield "g2 spam"
                raise ValueError("hovercraft is full of eels")
                yield "g2 more spam"
            finally:
                trace.append("Finishing g2")
        try:
            async for x in g1():
                trace.append("Yielded %s" % (x,))
        except ValueError as e:
            self.assertEqual(e.args[0], "hovercraft is full of eels")
        else:
            self.fail("subgenerator failed to raise ValueError")
        self.assertEqual(trace,[
            "Starting g1",
            "Yielded g1 ham",
            "Starting g2",
            "Yielded g2 spam",
            "Finishing g2",
            "Finishing g1",
        ])

    # @_async_test
    # async def test_delegation_of_asend(self):
    #     """
    #     Test delegation of send()
    #     """
    #     trace = []
    #     async def g1():
    #         trace.append("Starting g1")
    #         x = yield "g1 ham"
    #         trace.append("g1 received %s" % (x,))
    #         async yield from g2()
    #         x = yield "g1 eggs"
    #         trace.append("g1 received %s" % (x,))
    #         trace.append("Finishing g1")
    #     async def g2():
    #         trace.append("Starting g2")
    #         x = yield "g2 spam"
    #         trace.append("g2 received %s" % (x,))
    #         x = yield "g2 more spam"
    #         trace.append("g2 received %s" % (x,))
    #         trace.append("Finishing g2")
    #     g = g1()
    #     y = await anext(g)
    #     x = 1
    #     try:
    #         while 1:
    #             y = await g.asend(x)
    #             trace.append("Yielded %s" % (y,))
    #             x += 1
    #     except StopAsyncIteration:
    #         pass
    #     self.assertEqual(trace,[
    #         "Starting g1",
    #         "g1 received 1",
    #         "Starting g2",
    #         "Yielded g2 spam",
    #         "g2 received 2",
    #         "Yielded g2 more spam",
    #         "g2 received 3",
    #         "Finishing g2",
    #         "Yielded g1 eggs",
    #         "g1 received 4",
    #         "Finishing g1",
    #     ])

    # @_async_test
    # async def test_handling_exception_while_delegating_send(self):
    #     """
    #     Test handling exception while delegating 'send'
    #     """
    #     trace = []
    #     async def g1():
    #         trace.append("Starting g1")
    #         x = yield "g1 ham"
    #         trace.append("g1 received %s" % (x,))
    #         async yield from g2()
    #         x = yield "g1 eggs"
    #         trace.append("g1 received %s" % (x,))
    #         trace.append("Finishing g1")
    #     async def g2():
    #         trace.append("Starting g2")
    #         x = yield "g2 spam"
    #         trace.append("g2 received %s" % (x,))
    #         raise ValueError("hovercraft is full of eels")
    #         x = yield "g2 more spam"
    #         trace.append("g2 received %s" % (x,))
    #         trace.append("Finishing g2")
    #     async def run():
    #         g = g1()
    #         y = await anext(g)
    #         x = 1
    #         try:
    #             while 1:
    #                 y = await g.asend(x)
    #                 trace.append("Yielded %s" % (y,))
    #                 x += 1
    #         except StopAsyncIteration:
    #             trace.append("StopAsyncIteration")
    #     with self.assertRaises(ValueError):
    #         await run()
    #     self.assertEqual(trace,[
    #         "Starting g1",
    #         "g1 received 1",
    #         "Starting g2",
    #         "Yielded g2 spam",
    #         "g2 received 2",
    #     ])

    # @_async_test
    # async def test_delegating_aclose(self):
    #     """
    #     Test delegating 'aclose'
    #     """
    #     trace = []
    #     async def g1():
    #         try:
    #             trace.append("Starting g1")
    #             yield "g1 ham"
    #             async yield from g2()
    #             yield "g1 eggs"
    #         finally:
    #             trace.append("Finishing g1")
    #     async def g2():
    #         try:
    #             trace.append("Starting g2")
    #             yield "g2 spam"
    #             yield "g2 more spam"
    #         finally:
    #             trace.append("Finishing g2")
    #     g = g1()
    #     for i in range(2):
    #         x = await anext(g)
    #         trace.append("Yielded %s" % (x,))
    #     await g.aclose()
    #     self.assertEqual(trace,[
    #         "Starting g1",
    #         "Yielded g1 ham",
    #         "Starting g2",
    #         "Yielded g2 spam",
    #         "Finishing g2",
    #         "Finishing g1"
    #     ])

    # @_async_test
    # async def test_handing_exception_while_delegating_close(self):
    #     """
    #     Test handling exception while delegating 'close'
    #     """
    #     trace = []
    #     async def g1():
    #         try:
    #             trace.append("Starting g1")
    #             yield "g1 ham"
    #             async yield from g2()
    #             yield "g1 eggs"
    #         finally:
    #             trace.append("Finishing g1")
    #     async def g2():
    #         try:
    #             trace.append("Starting g2")
    #             yield "g2 spam"
    #             yield "g2 more spam"
    #         finally:
    #             trace.append("Finishing g2")
    #             raise ValueError("nybbles have exploded with delight")
    #     try:
    #         g = g1()
    #         for i in range(2):
    #             x = await anext(g)
    #             trace.append("Yielded %s" % (x,))
    #         await g.aclose()
    #     except ValueError as e:
    #         self.assertEqual(e.args[0], "nybbles have exploded with delight")
    #         self.assertIsInstance(e.__context__, GeneratorExit)
    #     else:
    #         self.fail("subgenerator failed to raise ValueError")
    #     self.assertEqual(trace,[
    #         "Starting g1",
    #         "Yielded g1 ham",
    #         "Starting g2",
    #         "Yielded g2 spam",
    #         "Finishing g2",
    #         "Finishing g1",
    #     ])

    # @_async_test
    # async def test_delegating_throw(self):
    #     """
    #     Test delegating 'throw'
    #     """
    #     trace = []
    #     async def g1():
    #         try:
    #             trace.append("Starting g1")
    #             yield "g1 ham"
    #             async yield from g2()
    #             yield "g1 eggs"
    #         finally:
    #             trace.append("Finishing g1")
    #     async def g2():
    #         try:
    #             trace.append("Starting g2")
    #             yield "g2 spam"
    #             yield "g2 more spam"
    #         finally:
    #             trace.append("Finishing g2")
    #     try:
    #         g = g1()
    #         for i in range(2):
    #             x = await anext(g)
    #             trace.append("Yielded %s" % (x,))
    #         e = ValueError("tomato ejected")
    #         await g.athrow(e)
    #     except ValueError as e:
    #         self.assertEqual(e.args[0], "tomato ejected")
    #     else:
    #         self.fail("subgenerator failed to raise ValueError")
    #     self.assertEqual(trace,[
    #         "Starting g1",
    #         "Yielded g1 ham",
    #         "Starting g2",
    #         "Yielded g2 spam",
    #         "Finishing g2",
    #         "Finishing g1",
    #     ])

    @_async_test
    async def test_value_attribute_of_StopAsyncIteration_exception(self):
        """
        Test 'value' attribute of StopAsyncIteration exception
        """
        trace = []
        async def pex(e):
            trace.append("%s: %s" % (e.__class__.__name__, e))
            trace.append("value = %s" % (e.value,))
        e = StopAsyncIteration()
        await pex(e)
        e = StopAsyncIteration("spam")
        await pex(e)
        e.value = "eggs"
        await pex(e)
        self.assertEqual(trace,[
            "StopAsyncIteration: ",
            "value = None",
            "StopAsyncIteration: spam",
            "value = spam",
            "StopAsyncIteration: spam",
            "value = eggs",
        ])

    # @_async_test
    # async def test_exception_value_crash(self):
    #     # There used to be a refcount error when the return value
    #     # stored in the StopAsyncIteration has a refcount of 1.
    #     async def g1():
    #         async yield from g2()
    #     async def g2():
    #         yield "g2"
    #         return [42]
    #     self.assertEqual([x async for x in g1()], ["g2"])


    # @_async_test
    # async def test_generator_return_value(self):
    #     """
    #     Test generator return value
    #     """
    #     trace = []
    #     async def g1():
    #         trace.append("Starting g1")
    #         yield "g1 ham"
    #         ret = async yield from g2()
    #         trace.append("g2 returned %r" % (ret,))
    #         for v in 1, (2,), StopAsyncIteration(3):
    #             ret = async yield from g2(v)
    #             trace.append("g2 returned %r" % (ret,))
    #         yield "g1 eggs"
    #         trace.append("Finishing g1")
    #     async def g2(v = None):
    #         trace.append("Starting g2")
    #         yield "g2 spam"
    #         yield "g2 more spam"
    #         trace.append("Finishing g2")
    #         if v:
    #             return v
    #     async for x in g1():
    #         trace.append("Yielded %s" % (x,))
    #     self.assertEqual(trace,[
    #         "Starting g1",
    #         "Yielded g1 ham",
    #         "Starting g2",
    #         "Yielded g2 spam",
    #         "Yielded g2 more spam",
    #         "Finishing g2",
    #         "g2 returned None",
    #         "Starting g2",
    #         "Yielded g2 spam",
    #         "Yielded g2 more spam",
    #         "Finishing g2",
    #         "g2 returned 1",
    #         "Starting g2",
    #         "Yielded g2 spam",
    #         "Yielded g2 more spam",
    #         "Finishing g2",
    #         "g2 returned (2,)",
    #         "Starting g2",
    #         "Yielded g2 spam",
    #         "Yielded g2 more spam",
    #         "Finishing g2",
    #         "g2 returned StopAsyncIteration(3)",
    #         "Yielded g1 eggs",
    #         "Finishing g1",
    #     ])

    @_async_test
    async def test_delegation_of_anext_to_non_generator(self):
        """
        Test delegation of anext() to non-generator
        """
        trace = []
        async def g():
            yield from range(3)
        async for x in g():
            trace.append("Yielded %s" % (x,))
        self.assertEqual(trace,[
            "Yielded 0",
            "Yielded 1",
            "Yielded 2",
        ])

    @_async_test
    async def test_conversion_of_asendNone_to_next(self):
        """
        Test conversion of asend(None) to next()
        """
        trace = []
        async def g():
            yield from range(3)
        gi = g()
        for x in range(3):
            y = await gi.asend(None)
            trace.append("Yielded: %s" % (y,))
        self.assertEqual(trace,[
            "Yielded: 0",
            "Yielded: 1",
            "Yielded: 2",
        ])

    @_async_test
    async def test_delegation_of_close_to_non_generator(self):
        """
        Test delegation of close() to non-generator
        """
        trace = []
        async def g():
            try:
                trace.append("starting g")
                yield from range(3)
                trace.append("g should not be here")
            finally:
                trace.append("finishing g")
        gi = g()
        await anext(gi)
        with captured_stderr() as output:
            await gi.aclose()
        self.assertEqual(output.getvalue(), '')
        self.assertEqual(trace,[
            "starting g",
            "finishing g",
        ])

    @_async_test
    async def test_delegating_throw_to_non_generator(self):
        """
        Test delegating 'throw' to non-generator
        """
        trace = []
        async def g():
            try:
                trace.append("Starting g")
                yield from range(10)
            finally:
                trace.append("Finishing g")
        try:
            gi = g()
            for i in range(5):
                x = await anext(gi)
                trace.append("Yielded %s" % (x,))
            e = ValueError("tomato ejected")
            await gi.athrow(e)
        except ValueError as e:
            self.assertEqual(e.args[0],"tomato ejected")
        else:
            self.fail("subgenerator failed to raise ValueError")
        self.assertEqual(trace,[
            "Starting g",
            "Yielded 0",
            "Yielded 1",
            "Yielded 2",
            "Yielded 3",
            "Yielded 4",
            "Finishing g",
        ])

    @_async_test
    async def test_attempting_to_send_to_non_generator(self):
        """
        Test attempting to send to non-generator
        """
        trace = []
        async def g():
            try:
                trace.append("starting g")
                yield from range(3)
                trace.append("g should not be here")
            finally:
                trace.append("finishing g")
        try:
            gi = g()
            await anext(gi)
            for x in range(3):
                y = await gi.asend(42)
                trace.append("Should not have yielded: %s" % (y,))
        except AttributeError as e:
            self.assertIn("send", e.args[0])
        else:
            self.fail("was able to send into non-generator")
        self.assertEqual(trace,[
            "starting g",
            "finishing g",
        ])

    # @_async_test
    # async def test_broken_getattr_handling(self):
    #     """
    #     Test subiterator with a broken getattr implementation
    #     """
    #     class Broken:
    #         def __aiter__(self):
    #             return self
    #         async def __anext__(self):
    #             return 1
    #         async def __getattr__(self, attr):
    #             1/0

    #     async def g():
    #         async yield from Broken()

    #     with self.assertRaises(ZeroDivisionError):
    #         gi = g()
    #         self.assertEqual(await anext(gi), 1)
    #         await gi.asend(1)

    #     with self.assertRaises(ZeroDivisionError):
    #         gi = g()
    #         self.assertEqual(await anext(gi), 1)
    #         await gi.athrow(AttributeError)

    #     with support.catch_unraisable_exception() as cm:
    #         gi = g()
    #         self.assertEqual(await anext(gi), 1)
    #         await gi.aclose()

    #         self.assertEqual(ZeroDivisionError, cm.unraisable.exc_type)

    @_async_test
    async def test_exception_in_initial_next_call(self):
        """
        Test exception in initial next() call
        """
        trace = []
        async def g1():
            trace.append("g1 about to yield from g2")
            async yield from g2()
            trace.append("g1 should not be here")
        async def g2():
            yield 1/0
        async def run():
            gi = g1()
            await anext(gi)
        with self.assertRaises(ZeroDivisionError):
            await run()
        self.assertEqual(trace,[
            "g1 about to yield from g2"
        ])

    @_async_test
    async def test_attempted_async_yield_from_loop(self):
        """
        Test attempted yield-from loop
        """
        trace = []
        async def g1():
            trace.append("g1: starting")
            yield "y1"
            trace.append("g1: about to yield from g2")
            async yield from g2()
            trace.append("g1 should not be here")

        async def g2():
            trace.append("g2: starting")
            yield "y2"
            trace.append("g2: about to yield from g1")
            async yield from gi
            trace.append("g2 should not be here")
        try:
            gi = g1()
            async for y in gi:
                trace.append("Yielded: %s" % (y,))
        except RuntimeError as e:
            self.assertEqual(e.args[0],"anext(): asynchronous generator is already running")
        else:
            self.fail("subgenerator didn't raise RuntimeError")
        self.assertEqual(trace,[
            "g1: starting",
            "Yielded: y1",
            "g1: about to yield from g2",
            "g2: starting",
            "Yielded: y2",
            "g2: about to yield from g1",
        ])

    # @_async_test
    # async def test_returning_value_from_delegated_throw(self):
    #     """
    #     Test returning value from delegated 'throw'
    #     """
    #     trace = []
    #     async def g1():
    #         try:
    #             trace.append("Starting g1")
    #             yield "g1 ham"
    #             async yield from g2()
    #             yield "g1 eggs"
    #         finally:
    #             trace.append("Finishing g1")
    #     async def g2():
    #         try:
    #             trace.append("Starting g2")
    #             yield "g2 spam"
    #             yield "g2 more spam"
    #         except LunchError:
    #             trace.append("Caught LunchError in g2")
    #             yield "g2 lunch saved"
    #             yield "g2 yet more spam"
    #     class LunchError(Exception):
    #         pass
    #     g = g1()
    #     for i in range(2):
    #         x = await anext(g)
    #         trace.append("Yielded %s" % (x,))
    #     e = LunchError("tomato ejected")
    #     await g.athrow(e)
    #     async for x in g:
    #         trace.append("Yielded %s" % (x,))
    #     self.assertEqual(trace,[
    #         "Starting g1",
    #         "Yielded g1 ham",
    #         "Starting g2",
    #         "Yielded g2 spam",
    #         "Caught LunchError in g2",
    #         "Yielded g2 yet more spam",
    #         "Yielded g1 eggs",
    #         "Finishing g1",
    #     ])

    # @_async_test
    # async def test_anext_and_return_with_value(self):
    #     """
    #     Test next and return with value
    #     """
    #     trace = []
    #     async def f(r):
    #         gi = g(r)
    #         await anext(gi)
    #         try:
    #             trace.append("f resuming g")
    #             await anext(gi)
    #             trace.append("f SHOULD NOT BE HERE")
    #         except StopAsyncIteration as e:
    #             trace.append("f caught %r" % (e,))
    #     async def g(r):
    #         trace.append("g starting")
    #         yield
    #         trace.append("g returning %r" % (r,))
    #         return r
    #     await f(None)
    #     await f(1)
    #     await f((2,))
    #     await f(StopAsyncIteration(3))
    #     self.assertEqual(trace,[
    #         "g starting",
    #         "f resuming g",
    #         "g returning None",
    #         "f caught StopAsyncIteration()",
    #         "g starting",
    #         "f resuming g",
    #         "g returning 1",
    #         "f caught StopAsyncIteration(1)",
    #         "g starting",
    #         "f resuming g",
    #         "g returning (2,)",
    #         "f caught StopAsyncIteration((2,))",
    #         "g starting",
    #         "f resuming g",
    #         "g returning StopAsyncIteration(3)",
    #         "f caught StopAsyncIteration(StopAsyncIteration(3))",
    #     ])

    # @_async_test
    # async def test_send_and_return_with_value(self):
    #     """
    #     Test send and return with value
    #     """
    #     trace = []
    #     async def f(r):
    #         gi = g(r)
    #         await anext(gi)
    #         try:
    #             trace.append("f sending spam to g")
    #             await gi.asend("spam")
    #             trace.append("f SHOULD NOT BE HERE")
    #         except StopAsyncIteration as e:
    #             trace.append("f caught %r" % (e,))
    #     async def g(r):
    #         trace.append("g starting")
    #         x = yield
    #         trace.append("g received %r" % (x,))
    #         trace.append("g returning %r" % (r,))
    #         return r
    #     await f(None)
    #     await f(1)
    #     await f((2,))
    #     await f(StopAsyncIteration(3))
    #     self.assertEqual(trace, [
    #         "g starting",
    #         "f sending spam to g",
    #         "g received 'spam'",
    #         "g returning None",
    #         "f caught StopAsyncIteration()",
    #         "g starting",
    #         "f sending spam to g",
    #         "g received 'spam'",
    #         "g returning 1",
    #         'f caught StopAsyncIteration(1)',
    #         'g starting',
    #         'f sending spam to g',
    #         "g received 'spam'",
    #         'g returning (2,)',
    #         'f caught StopAsyncIteration((2,))',
    #         'g starting',
    #         'f sending spam to g',
    #         "g received 'spam'",
    #         'g returning StopAsyncIteration(3)',
    #         'f caught StopAsyncIteration(StopAsyncIteration(3))'
    #     ])

    # @_async_test
    # async def test_catching_exception_from_subgen_and_returning(self):
    #     """
    #     Test catching an exception thrown into a
    #     subgenerator and returning a value
    #     """
    #     async def inner():
    #         try:
    #             yield 1
    #         except ValueError:
    #             trace.append("inner caught ValueError")
    #         return value

    #     async def outer():
    #         v = async yield from inner()
    #         trace.append("inner returned %r to outer" % (v,))
    #         yield v

    #     for value in 2, (2,), StopAsyncIteration(2):
    #         trace = []
    #         g = outer()
    #         trace.append(await anext(g))
    #         trace.append(repr(await g.athrow(ValueError)))
    #         self.assertEqual(trace, [
    #             1,
    #             "inner caught ValueError",
    #             "inner returned %r to outer" % (value,),
    #             repr(value),
    #         ])

    @_async_test
    async def test_throwing_GeneratorExit_into_subgen_that_returns(self):
        """
        Test throwing GeneratorExit into a subgenerator that
        catches it and returns normally.
        """
        trace = []
        async def f():
            try:
                trace.append("Enter f")
                yield
                trace.append("Exit f")
            except GeneratorExit:
                return
        async def g():
            trace.append("Enter g")
            async yield from f()
            trace.append("Exit g")
        try:
            gi = g()
            await anext(gi)
            await gi.athrow(GeneratorExit)
        except GeneratorExit:
            pass
        else:
            self.fail("subgenerator failed to raise GeneratorExit")
        self.assertEqual(trace,[
            "Enter g",
            "Enter f",
        ])

    # @_async_test
    # async def test_throwing_GeneratorExit_into_subgenerator_that_yields(self):
    #     """
    #     Test throwing GeneratorExit into a subgenerator that
    #     catches it and yields.
    #     """
    #     trace = []
    #     async def f():
    #         try:
    #             trace.append("Enter f")
    #             yield
    #             trace.append("Exit f")
    #         except GeneratorExit:
    #             yield
    #     async def g():
    #         trace.append("Enter g")
    #         async yield from f()
    #         trace.append("Exit g")
    #     try:
    #         gi = g()
    #         await anext(gi)
    #         await gi.athrow(GeneratorExit)
    #     except RuntimeError as e:
    #         self.assertEqual(e.args[0], "generator ignored GeneratorExit")
    #     else:
    #         self.fail("subgenerator failed to raise GeneratorExit")
    #     self.assertEqual(trace,[
    #         "Enter g",
    #         "Enter f",
    #     ])

    # @_async_test
    # async def test_throwing_GeneratorExit_into_subgen_that_raises(self):
    #     """
    #     Test throwing GeneratorExit into a subgenerator that
    #     catches it and raises a different exception.
    #     """
    #     trace = []
    #     async def f():
    #         try:
    #             trace.append("Enter f")
    #             yield
    #             trace.append("Exit f")
    #         except GeneratorExit:
    #             raise ValueError("Vorpal bunny encountered")
    #     async def g():
    #         trace.append("Enter g")
    #         async yield from f()
    #         trace.append("Exit g")
    #     try:
    #         gi = g()
    #         await anext(gi)
    #         await gi.athrow(GeneratorExit)
    #     except ValueError as e:
    #         self.assertEqual(e.args[0], "Vorpal bunny encountered")
    #         self.assertIsInstance(e.__context__, GeneratorExit)
    #     else:
    #         self.fail("subgenerator failed to raise ValueError")
    #     self.assertEqual(trace,[
    #         "Enter g",
    #         "Enter f",
    #     ])

    @_async_test
    async def test_yield_from_empty(self):
        async def g():
            yield from ()
        with self.assertRaises(StopAsyncIteration):
            await anext(g())

    @_async_test
    async def test_delegating_generators_claim_to_be_running(self):
        # Check with basic iteration
        async def one():
            yield 0
            async yield from two()
            yield 3
        async def two():
            yield 1
            try:
                async yield from g1
            except RuntimeError:
                pass
            yield 2
        g1 = one()
        self.assertEqual([e async for e in g1], [0, 1, 2, 3])

        # Check with asend
        g1 = one()
        res = [await anext(g1)]
        try:
            while True:
                res.append(await g1.asend(42))
        except StopAsyncIteration:
            pass
        self.assertEqual(res, [0, 1, 2, 3])

    # @_async_test
    # async def test_delegating_generators_claim_to_be_running_with_throw(self):
    #     # Check with throw
    #     class MyErr(Exception):
    #         pass
    #     async def one():
    #         try:
    #             yield 0
    #         except MyErr:
    #             pass
    #         async yield from two()
    #         try:
    #             yield 3
    #         except MyErr:
    #             pass
    #     async def two():
    #         try:
    #             yield 1
    #         except MyErr:
    #             pass
    #         try:
    #             async yield from g1
    #         except RuntimeError:
    #             pass
    #         try:
    #             yield 2
    #         except MyErr:
    #             pass
    #     g1 = one()
    #     res = [await anext(g1)]
    #     try:
    #         while True:
    #             res.append(await g1.athrow(MyErr))
    #     except StopAsyncIteration:
    #         pass
    #     except:
    #         self.assertEqual(res, [0, 1, 2, 3])
    #         raise

    # @_async_test
    # async def test_delegating_generators_claim_to_be_running_with_close(self):
    #     # Check with close
    #     class MyIt:
    #         def __iter__(self):
    #             return self
    #         async def __next__(self):
    #             return 42
    #         async def aclose(self_):
    #             self.assertTrue(g1.gi_running)
    #             with self.assertRaises(RuntimeError):
    #                 await anext(g1)
    #     async def one():
    #         async yield from MyIt()
    #     g1 = one()
    #     await anext(g1)
    #     await g1.aclose()

    @_async_test
    async def test_delegator_is_visible_to_debugger(self):
        async def call_stack():
            return [f[3] for f in inspect.stack()]

        async def gen():
            yield await call_stack()
            yield await call_stack()
            yield await call_stack()

        async def spam(g):
            async yield from g

        async def eggs(g):
            async yield from g

        async for stack in spam(gen()):
            self.assertTrue('spam' in stack)

        async for stack in spam(eggs(gen())):
            self.assertTrue('spam' in stack and 'eggs' in stack)

    # @_async_test
    # async def test_custom_iterator_return(self):
    #     # See issue #15568
    #     class MyIter:
    #         def __aiter__(self):
    #             return self
    #         async def __anext__(self):
    #             raise StopAsyncIteration(42)
    #     async def gen():
    #         nonlocal ret
    #         ret = async yield from MyIter()
    #     ret = None
    #     [e async for e in gen()]
    #     self.assertEqual(ret, 42)

    @_async_test
    async def test_close_with_cleared_frame(self):
        # See issue #17669.
        #
        # Create a stack of generators: outer() delegating to inner()
        # delegating to innermost(). The key point is that the instance of
        # inner is created first: this ensures that its frame appears before
        # the instance of outer in the GC linked list.
        #
        # At the gc.collect call:
        #   - frame_clear is called on the inner_gen frame.
        #   - gen_dealloc is called on the outer_gen generator (the only
        #     reference is in the frame's locals).
        #   - gen_close is called on the outer_gen generator.
        #   - gen_close_iter is called to close the inner_gen generator, which
        #     in turn calls gen_close, and gen_yf.
        #
        # Previously, gen_yf would crash since inner_gen's frame had been
        # cleared (and in particular f_stacktop was NULL).

        async def innermost():
            yield
        async def inner():
            outer_gen = yield
            async yield from innermost()
        async def outer():
            inner_gen = yield
            async yield from inner_gen

        with disable_gc():
            inner_gen = inner()
            outer_gen = outer()
            await outer_gen.asend(None)
            await outer_gen.asend(inner_gen)
            await outer_gen.asend(outer_gen)

            del outer_gen
            del inner_gen
            gc_collect()

    # @_async_test
    # async def test_send_tuple_with_custom_generator(self):
    #     # See issue #21209.
    #     class MyGen:
    #         def __aiter__(self):
    #             return self
    #         async def __anext__(self):
    #             return 42
    #         async def asend(self, what):
    #             nonlocal v
    #             v = what
    #             return None
    #     async def outer():
    #         v = async yield from MyGen()
    #     g = outer()
    #     await anext(g)
    #     v = None
    #     await g.asend((1, 2, 3, 4))
    #     self.assertEqual(v, (1, 2, 3, 4))

class TestInterestingEdgeCases(unittest.TestCase):

    async def assert_stop_iteration(self, iterator):
        with self.assertRaises(StopAsyncIteration) as caught:
            await anext(iterator)
        self.assertIsNone(caught.exception.value)
        self.assertIsNone(caught.exception.__context__)

    def assert_generator_raised_stop_iteration(self):
        return self.assertRaisesRegex(RuntimeError, r"^generator raised StopAsyncIteration$")

    def assert_generator_ignored_generator_exit(self):
        return self.assertRaisesRegex(RuntimeError, r"^generator ignored GeneratorExit$")

    # @_async_test
    # async def test_close_and_throw_work(self):

    #     yielded_first = object()
    #     yielded_second = object()
    #     returned = object()

    #     async def inner():
    #         yield yielded_first
    #         yield yielded_second
    #         return returned

    #     async def outer():
    #         return (async yield from inner())

    #     with self.subTest("close"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         await g.aclose()
    #         await self.assert_stop_iteration(g)

    #     with self.subTest("throw GeneratorExit"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         thrown = GeneratorExit()
    #         with self.assertRaises(GeneratorExit) as caught:
    #             await g.athrow(thrown)
    #         self.assertIs(caught.exception, thrown)
    #         self.assertIsNone(caught.exception.__context__)
    #         await self.assert_stop_iteration(g)

    #     with self.subTest("throw StopAsyncIteration"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         thrown = StopAsyncIteration()
    #         with self.assert_generator_raised_stop_iteration() as caught:
    #             await g.athrow(thrown)
    #         self.assertIs(caught.exception.__context__, thrown)
    #         self.assertIsNone(caught.exception.__context__.__context__)
    #         await self.assert_stop_iteration(g)

    #     with self.subTest("throw BaseException"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         thrown = BaseException()
    #         with self.assertRaises(BaseException) as caught:
    #             await g.athrow(thrown)
    #         self.assertIs(caught.exception, thrown)
    #         self.assertIsNone(caught.exception.__context__)
    #         await self.assert_stop_iteration(g)

    #     with self.subTest("throw Exception"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         thrown = Exception()
    #         with self.assertRaises(Exception) as caught:
    #             await g.athrow(thrown)
    #         self.assertIs(caught.exception, thrown)
    #         self.assertIsNone(caught.exception.__context__)
    #         await self.assert_stop_iteration(g)

#     @_async_test
#     async def test_close_and_throw_raise_generator_exit(self):

#         yielded_first = object()
#         yielded_second = object()
#         returned = object()

#         async def inner():
#             try:
#                 yield yielded_first
#                 yield yielded_second
#                 return returned
#             finally:
#                 raise raised

#         async def outer():
#             return (yield from inner())

#         with self.subTest("close"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = GeneratorExit()
#             # GeneratorExit is suppressed. This is consistent with PEP 342:
#             # https://peps.python.org/pep-0342/#new-generator-method-close
#             await g.aclose()
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw GeneratorExit"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = GeneratorExit()
#             thrown = GeneratorExit()
#             with self.assertRaises(GeneratorExit) as caught:
#                 await g.athrow(thrown)
#             # The raised GeneratorExit is suppressed, but the thrown one
#             # propagates. This is consistent with PEP 380:
#             # https://peps.python.org/pep-0380/#proposal
#             self.assertIs(caught.exception, thrown)
#             self.assertIsNone(caught.exception.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw StopAsyncIteration"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = GeneratorExit()
#             thrown = StopAsyncIteration()
#             with self.assertRaises(GeneratorExit) as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception, raised)
#             self.assertIs(caught.exception.__context__, thrown)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw BaseException"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = GeneratorExit()
#             thrown = BaseException()
#             with self.assertRaises(GeneratorExit) as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception, raised)
#             self.assertIs(caught.exception.__context__, thrown)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw Exception"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = GeneratorExit()
#             thrown = Exception()
#             with self.assertRaises(GeneratorExit) as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception, raised)
#             self.assertIs(caught.exception.__context__, thrown)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

#     @_async_test
#     async def test_close_and_throw_raise_stop_iteration(self):

#         yielded_first = object()
#         yielded_second = object()
#         returned = object()

#         async def inner():
#             try:
#                 yield yielded_first
#                 yield yielded_second
#                 return returned
#             finally:
#                 raise raised

#         async def outer():
#             return (yield from inner())

#         with self.subTest("close"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = StopAsyncIteration()
#             # PEP 479:
#             with self.assert_generator_raised_stop_iteration() as caught:
#                 await g.aclose()
#             self.assertIs(caught.exception.__context__, raised)
#             self.assertIsInstance(caught.exception.__context__.__context__, GeneratorExit)
#             self.assertIsNone(caught.exception.__context__.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw GeneratorExit"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = StopAsyncIteration()
#             thrown = GeneratorExit()
#             # PEP 479:
#             with self.assert_generator_raised_stop_iteration() as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception.__context__, raised)
#             # This isn't the same GeneratorExit as thrown! It's the one created
#             # by calling inner.aclose():
#             self.assertIsInstance(caught.exception.__context__.__context__, GeneratorExit)
#             self.assertIsNone(caught.exception.__context__.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw StopAsyncIteration"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = StopAsyncIteration()
#             thrown = StopAsyncIteration()
#             # PEP 479:
#             with self.assert_generator_raised_stop_iteration() as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception.__context__, raised)
#             self.assertIs(caught.exception.__context__.__context__, thrown)
#             self.assertIsNone(caught.exception.__context__.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw BaseException"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = StopAsyncIteration()
#             thrown = BaseException()
#             # PEP 479:
#             with self.assert_generator_raised_stop_iteration() as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception.__context__, raised)
#             self.assertIs(caught.exception.__context__.__context__, thrown)
#             self.assertIsNone(caught.exception.__context__.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw Exception"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = StopAsyncIteration()
#             thrown = Exception()
#             # PEP 479:
#             with self.assert_generator_raised_stop_iteration() as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception.__context__, raised)
#             self.assertIs(caught.exception.__context__.__context__, thrown)
#             self.assertIsNone(caught.exception.__context__.__context__.__context__)
#             await self.assert_stop_iteration(g)

#     @_async_test
#     async def test_close_and_throw_raise_base_exception(self):

#         yielded_first = object()
#         yielded_second = object()
#         returned = object()

#         async def inner():
#             try:
#                 yield yielded_first
#                 yield yielded_second
#                 return returned
#             finally:
#                 raise raised

#         async def outer():
#             return (yield from inner())

#         with self.subTest("close"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = BaseException()
#             with self.assertRaises(BaseException) as caught:
#                 await g.aclose()
#             self.assertIs(caught.exception, raised)
#             self.assertIsInstance(caught.exception.__context__, GeneratorExit)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw GeneratorExit"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = BaseException()
#             thrown = GeneratorExit()
#             with self.assertRaises(BaseException) as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception, raised)
#             # This isn't the same GeneratorExit as thrown! It's the one created
#             # by calling inner.aclose():
#             self.assertIsInstance(caught.exception.__context__, GeneratorExit)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw StopAsyncIteration"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = BaseException()
#             thrown = StopAsyncIteration()
#             with self.assertRaises(BaseException) as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception, raised)
#             self.assertIs(caught.exception.__context__, thrown)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw BaseException"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = BaseException()
#             thrown = BaseException()
#             with self.assertRaises(BaseException) as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception, raised)
#             self.assertIs(caught.exception.__context__, thrown)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw Exception"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = BaseException()
#             thrown = Exception()
#             with self.assertRaises(BaseException) as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception, raised)
#             self.assertIs(caught.exception.__context__, thrown)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

#     @_async_test
#     async def test_close_and_throw_raise_exception(self):

#         yielded_first = object()
#         yielded_second = object()
#         returned = object()

#         async def inner():
#             try:
#                 yield yielded_first
#                 yield yielded_second
#                 return returned
#             finally:
#                 raise raised

#         async def outer():
#             return (yield from inner())

#         with self.subTest("close"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = Exception()
#             with self.assertRaises(Exception) as caught:
#                 await g.aclose()
#             self.assertIs(caught.exception, raised)
#             self.assertIsInstance(caught.exception.__context__, GeneratorExit)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw GeneratorExit"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = Exception()
#             thrown = GeneratorExit()
#             with self.assertRaises(Exception) as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception, raised)
#             # This isn't the same GeneratorExit as thrown! It's the one created
#             # by calling inner.aclose():
#             self.assertIsInstance(caught.exception.__context__, GeneratorExit)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw StopAsyncIteration"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = Exception()
#             thrown = StopAsyncIteration()
#             with self.assertRaises(Exception) as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception, raised)
#             self.assertIs(caught.exception.__context__, thrown)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw BaseException"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = Exception()
#             thrown = BaseException()
#             with self.assertRaises(Exception) as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception, raised)
#             self.assertIs(caught.exception.__context__, thrown)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

#         with self.subTest("throw Exception"):
#             g = outer()
#             self.assertIs(await anext(g), yielded_first)
#             raised = Exception()
#             thrown = Exception()
#             with self.assertRaises(Exception) as caught:
#                 await g.athrow(thrown)
#             self.assertIs(caught.exception, raised)
#             self.assertIs(caught.exception.__context__, thrown)
#             self.assertIsNone(caught.exception.__context__.__context__)
#             await self.assert_stop_iteration(g)

    # @_async_test
    # async def test_close_and_throw_yield(self):

    #     yielded_first = object()
    #     yielded_second = object()
    #     returned = object()

    #     async def inner():
    #         try:
    #             yield yielded_first
    #         finally:
    #             yield yielded_second
    #         return returned

    #     async def outer():
    #         return (async yield from inner())

    #     with self.subTest("close"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         # No chaining happens. This is consistent with PEP 342:
    #         # https://peps.python.org/pep-0342/#new-generator-method-close
    #         with self.assert_generator_ignored_generator_exit() as caught:
    #             await g.aclose()
    #         self.assertIsNone(caught.exception.__context__)
    #         await self.assert_stop_iteration(g)

    #     with self.subTest("throw GeneratorExit"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         thrown = GeneratorExit()
    #         # No chaining happens. This is consistent with PEP 342:
    #         # https://peps.python.org/pep-0342/#new-generator-method-close
    #         with self.assert_generator_ignored_generator_exit() as caught:
    #             await g.athrow(thrown)
    #         self.assertIsNone(caught.exception.__context__)
    #         await self.assert_stop_iteration(g)

    #     with self.subTest("throw StopAsyncIteration"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         thrown = StopAsyncIteration()
    #         self.assertEqual(await g.athrow(thrown), yielded_second)
    #         # PEP 479:
    #         with self.assert_generator_raised_stop_iteration() as caught:
    #             await anext(g)
    #         self.assertIs(caught.exception.__context__, thrown)
    #         self.assertIsNone(caught.exception.__context__.__context__)
    #         await self.assert_stop_iteration(g)

    #     with self.subTest("throw BaseException"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         thrown = BaseException()
    #         self.assertEqual(await g.athrow(thrown), yielded_second)
    #         with self.assertRaises(BaseException) as caught:
    #             await anext(g)
    #         self.assertIs(caught.exception, thrown)
    #         self.assertIsNone(caught.exception.__context__)
    #         await self.assert_stop_iteration(g)

    #     with self.subTest("throw Exception"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         thrown = Exception()
    #         self.assertEqual(await g.athrow(thrown), yielded_second)
    #         with self.assertRaises(Exception) as caught:
    #             await anext(g)
    #         self.assertIs(caught.exception, thrown)
    #         self.assertIsNone(caught.exception.__context__)
    #         await self.assert_stop_iteration(g)

    # @_async_test
    # async def test_close_and_throw_return(self):
    #     yielded_first = object()
    #     yielded_second = object()
    #     returned = object()

    #     async def inner():
    #         try:
    #             yield yielded_first
    #             yield yielded_second
    #         except:
    #             pass
    #         return returned

    #     async def outer():
    #         return (async yield from inner())

    #     with self.subTest("close"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         # StopAsyncIteration is suppressed. This is consistent with PEP 342:
    #         # https://peps.python.org/pep-0342/#new-generator-method-close
    #         await g.aclose()
    #         await self.assert_stop_iteration(g)

    #     with self.subTest("throw GeneratorExit"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         thrown = GeneratorExit()
    #         # StopAsyncIteration is suppressed. This is consistent with PEP 342:
    #         # https://peps.python.org/pep-0342/#new-generator-method-close
    #         with self.assertRaises(GeneratorExit) as caught:
    #             await g.athrow(thrown)
    #         self.assertIs(caught.exception, thrown)
    #         self.assertIsNone(caught.exception.__context__)
    #         await self.assert_stop_iteration(g)

    #     with self.subTest("throw StopAsyncIteration"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         thrown = StopAsyncIteration()
    #         with self.assertRaises(StopAsyncIteration) as caught:
    #             await g.athrow(thrown)
    #         self.assertIs(caught.exception.value, returned)
    #         self.assertIsNone(caught.exception.__context__)
    #         await self.assert_stop_iteration(g)

    #     with self.subTest("throw BaseException"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         thrown = BaseException()
    #         with self.assertRaises(StopAsyncIteration) as caught:
    #             await g.athrow(thrown)
    #         self.assertIs(caught.exception.value, returned)
    #         self.assertIsNone(caught.exception.__context__)
    #         await self.assert_stop_iteration(g)

    #     with self.subTest("throw Exception"):
    #         g = outer()
    #         self.assertIs(await anext(g), yielded_first)
    #         thrown = Exception()
    #         with self.assertRaises(StopAsyncIteration) as caught:
    #             await g.athrow(thrown)
    #         self.assertIs(caught.exception.value, returned)
    #         self.assertIsNone(caught.exception.__context__)
    #         await self.assert_stop_iteration(g)

    @_async_test
    async def test_throws_in_iter(self):
        # See GH-126366: NULL pointer dereference if __iter__
        # threw an exception.
        class Silly:
            async def __aiter__(self):
                yield from ()
                raise RuntimeError("nobody expects the spanish inquisition")

        async def my_generator():
            async yield from Silly()

        with self.assertRaisesRegex(RuntimeError, "nobody expects the spanish inquisition"):
            await anext(my_generator())


if __name__ == '__main__':
    unittest.main()
