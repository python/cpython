# Adapted with permission from the EdgeDB project.


import asyncio
import contextvars

from asyncio import taskgroups
import unittest


# To prevent a warning "test altered the execution environment"
def tearDownModule():
    asyncio.set_event_loop_policy(None)


class MyExc(Exception):
    pass


class MyBaseExc(BaseException):
    pass


def get_error_types(eg):
    return {type(exc) for exc in eg.exceptions}


class TestTaskGroup(unittest.IsolatedAsyncioTestCase):

    async def test_taskgroup_01(self):

        async def foo1():
            await asyncio.sleep(0.1)
            return 42

        async def foo2():
            await asyncio.sleep(0.2)
            return 11

        async with taskgroups.TaskGroup() as g:
            t1 = g.create_task(foo1())
            t2 = g.create_task(foo2())

        self.assertEqual(t1.result(), 42)
        self.assertEqual(t2.result(), 11)

    async def test_taskgroup_02(self):

        async def foo1():
            await asyncio.sleep(0.1)
            return 42

        async def foo2():
            await asyncio.sleep(0.2)
            return 11

        async with taskgroups.TaskGroup() as g:
            t1 = g.create_task(foo1())
            await asyncio.sleep(0.15)
            t2 = g.create_task(foo2())

        self.assertEqual(t1.result(), 42)
        self.assertEqual(t2.result(), 11)

    async def test_taskgroup_03(self):

        async def foo1():
            await asyncio.sleep(1)
            return 42

        async def foo2():
            await asyncio.sleep(0.2)
            return 11

        async with taskgroups.TaskGroup() as g:
            t1 = g.create_task(foo1())
            await asyncio.sleep(0.15)
            # cancel t1 explicitly, i.e. everything should continue
            # working as expected.
            t1.cancel()

            t2 = g.create_task(foo2())

        self.assertTrue(t1.cancelled())
        self.assertEqual(t2.result(), 11)

    async def test_taskgroup_04(self):

        NUM = 0
        t2_cancel = False
        t2 = None

        async def foo1():
            await asyncio.sleep(0.1)
            1 / 0

        async def foo2():
            nonlocal NUM, t2_cancel
            try:
                await asyncio.sleep(1)
            except asyncio.CancelledError:
                t2_cancel = True
                raise
            NUM += 1

        async def runner():
            nonlocal NUM, t2

            async with taskgroups.TaskGroup() as g:
                g.create_task(foo1())
                t2 = g.create_task(foo2())

            NUM += 10

        with self.assertRaises(ExceptionGroup) as cm:
            await asyncio.create_task(runner())

        self.assertEqual(get_error_types(cm.exception), {ZeroDivisionError})

        self.assertEqual(NUM, 0)
        self.assertTrue(t2_cancel)
        self.assertTrue(t2.cancelled())

    async def test_cancel_children_on_child_error(self):
        """
        When a child task raises an error, the rest of the children
        are cancelled and the errors are gathered into an EG.
        """

        NUM = 0
        t2_cancel = False
        runner_cancel = False

        async def foo1():
            await asyncio.sleep(0.1)
            1 / 0

        async def foo2():
            nonlocal NUM, t2_cancel
            try:
                await asyncio.sleep(5)
            except asyncio.CancelledError:
                t2_cancel = True
                raise
            NUM += 1

        async def runner():
            nonlocal NUM, runner_cancel

            async with taskgroups.TaskGroup() as g:
                g.create_task(foo1())
                g.create_task(foo1())
                g.create_task(foo1())
                g.create_task(foo2())
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    runner_cancel = True
                    raise

            NUM += 10

        # The 3 foo1 sub tasks can be racy when the host is busy - if the
        # cancellation happens in the middle, we'll see partial sub errors here
        with self.assertRaises(ExceptionGroup) as cm:
            await asyncio.create_task(runner())

        self.assertEqual(get_error_types(cm.exception), {ZeroDivisionError})
        self.assertEqual(NUM, 0)
        self.assertTrue(t2_cancel)
        self.assertTrue(runner_cancel)

    async def test_cancellation(self):

        NUM = 0

        async def foo():
            nonlocal NUM
            try:
                await asyncio.sleep(5)
            except asyncio.CancelledError:
                NUM += 1
                raise

        async def runner():
            async with taskgroups.TaskGroup() as g:
                for _ in range(5):
                    g.create_task(foo())

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError) as cm:
            await r

        self.assertEqual(NUM, 5)

    async def test_taskgroup_07(self):

        NUM = 0

        async def foo():
            nonlocal NUM
            try:
                await asyncio.sleep(5)
            except asyncio.CancelledError:
                NUM += 1
                raise

        async def runner():
            nonlocal NUM
            async with taskgroups.TaskGroup() as g:
                for _ in range(5):
                    g.create_task(foo())

                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    NUM += 10
                    raise

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await r

        self.assertEqual(NUM, 15)

    async def test_cancellation_in_body(self):

        async def foo():
            await asyncio.sleep(0.1)
            1 / 0

        async def runner():
            async with taskgroups.TaskGroup() as g:
                for _ in range(5):
                    g.create_task(foo())

                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    raise

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError) as cm:
            await r

    async def test_taskgroup_09(self):

        t1 = t2 = None

        async def foo1():
            await asyncio.sleep(1)
            return 42

        async def foo2():
            await asyncio.sleep(2)
            return 11

        async def runner():
            nonlocal t1, t2
            async with taskgroups.TaskGroup() as g:
                t1 = g.create_task(foo1())
                t2 = g.create_task(foo2())
                await asyncio.sleep(0.1)
                1 / 0

        try:
            await runner()
        except ExceptionGroup as t:
            self.assertEqual(get_error_types(t), {ZeroDivisionError})
        else:
            self.fail('ExceptionGroup was not raised')

        self.assertTrue(t1.cancelled())
        self.assertTrue(t2.cancelled())

    async def test_taskgroup_10(self):

        t1 = t2 = None

        async def foo1():
            await asyncio.sleep(1)
            return 42

        async def foo2():
            await asyncio.sleep(2)
            return 11

        async def runner():
            nonlocal t1, t2
            async with taskgroups.TaskGroup() as g:
                t1 = g.create_task(foo1())
                t2 = g.create_task(foo2())
                1 / 0

        try:
            await runner()
        except ExceptionGroup as t:
            self.assertEqual(get_error_types(t), {ZeroDivisionError})
        else:
            self.fail('ExceptionGroup was not raised')

        self.assertTrue(t1.cancelled())
        self.assertTrue(t2.cancelled())

    async def test_taskgroup_11(self):

        async def foo():
            await asyncio.sleep(0.1)
            1 / 0

        async def runner():
            async with taskgroups.TaskGroup():
                async with taskgroups.TaskGroup() as g2:
                    for _ in range(5):
                        g2.create_task(foo())

                    try:
                        await asyncio.sleep(10)
                    except asyncio.CancelledError:
                        raise

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await r

    async def test_taskgroup_12(self):

        async def foo():
            await asyncio.sleep(0.1)
            1 / 0

        async def runner():
            async with taskgroups.TaskGroup() as g1:
                g1.create_task(asyncio.sleep(10))

                async with taskgroups.TaskGroup() as g2:
                    for _ in range(5):
                        g2.create_task(foo())

                    try:
                        await asyncio.sleep(10)
                    except asyncio.CancelledError:
                        raise

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await r

    async def test_taskgroup_13(self):

        async def crash_after(t):
            await asyncio.sleep(t)
            raise ValueError(t)

        async def runner():
            async with taskgroups.TaskGroup() as g1:
                g1.create_task(crash_after(0.1))

                async with taskgroups.TaskGroup() as g2:
                    g2.create_task(crash_after(10))

        r = asyncio.create_task(runner())
        with self.assertRaises(ExceptionGroup) as cm:
            await r

        self.assertEqual(get_error_types(cm.exception), {ValueError})

    async def test_taskgroup_14(self):

        async def crash_after(t):
            await asyncio.sleep(t)
            raise ValueError(t)

        async def runner():
            async with taskgroups.TaskGroup() as g1:
                g1.create_task(crash_after(10))

                async with taskgroups.TaskGroup() as g2:
                    g2.create_task(crash_after(0.1))

        r = asyncio.create_task(runner())
        with self.assertRaises(ExceptionGroup) as cm:
            await r

        self.assertEqual(get_error_types(cm.exception), {ExceptionGroup})
        self.assertEqual(get_error_types(cm.exception.exceptions[0]), {ValueError})

    async def test_taskgroup_15(self):

        async def crash_soon():
            await asyncio.sleep(0.3)
            1 / 0

        async def runner():
            async with taskgroups.TaskGroup() as g1:
                g1.create_task(crash_soon())
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    await asyncio.sleep(0.5)
                    raise

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await r

    async def test_taskgroup_16(self):

        async def crash_soon():
            await asyncio.sleep(0.3)
            1 / 0

        async def nested_runner():
            async with taskgroups.TaskGroup() as g1:
                g1.create_task(crash_soon())
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    await asyncio.sleep(0.5)
                    raise

        async def runner():
            t = asyncio.create_task(nested_runner())
            await t

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await r

    async def test_taskgroup_17(self):
        NUM = 0

        async def runner():
            nonlocal NUM
            async with taskgroups.TaskGroup():
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    NUM += 10
                    raise

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await r

        self.assertEqual(NUM, 10)

    async def test_taskgroup_18(self):
        NUM = 0

        async def runner():
            nonlocal NUM
            async with taskgroups.TaskGroup():
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    NUM += 10
                    # This isn't a good idea, but we have to support
                    # this weird case.
                    raise MyExc

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()

        try:
            await r
        except ExceptionGroup as t:
            self.assertEqual(get_error_types(t),{MyExc})
        else:
            self.fail('ExceptionGroup was not raised')

        self.assertEqual(NUM, 10)

    async def test_taskgroup_19(self):
        async def crash_soon():
            await asyncio.sleep(0.1)
            1 / 0

        async def nested():
            try:
                await asyncio.sleep(10)
            finally:
                raise MyExc

        async def runner():
            async with taskgroups.TaskGroup() as g:
                g.create_task(crash_soon())
                await nested()

        r = asyncio.create_task(runner())
        try:
            await r
        except ExceptionGroup as t:
            self.assertEqual(get_error_types(t), {MyExc, ZeroDivisionError})
        else:
            self.fail('TasgGroupError was not raised')

    async def test_taskgroup_20(self):
        async def crash_soon():
            await asyncio.sleep(0.1)
            1 / 0

        async def nested():
            try:
                await asyncio.sleep(10)
            finally:
                raise KeyboardInterrupt

        async def runner():
            async with taskgroups.TaskGroup() as g:
                g.create_task(crash_soon())
                await nested()

        with self.assertRaises(KeyboardInterrupt):
            await runner()

    async def test_taskgroup_20a(self):
        async def crash_soon():
            await asyncio.sleep(0.1)
            1 / 0

        async def nested():
            try:
                await asyncio.sleep(10)
            finally:
                raise MyBaseExc

        async def runner():
            async with taskgroups.TaskGroup() as g:
                g.create_task(crash_soon())
                await nested()

        with self.assertRaises(BaseExceptionGroup) as cm:
            await runner()

        self.assertEqual(
            get_error_types(cm.exception), {MyBaseExc, ZeroDivisionError}
        )

    async def _test_taskgroup_21(self):
        # This test doesn't work as asyncio, currently, doesn't
        # correctly propagate KeyboardInterrupt (or SystemExit) --
        # those cause the event loop itself to crash.
        # (Compare to the previous (passing) test -- that one raises
        # a plain exception but raises KeyboardInterrupt in nested();
        # this test does it the other way around.)

        async def crash_soon():
            await asyncio.sleep(0.1)
            raise KeyboardInterrupt

        async def nested():
            try:
                await asyncio.sleep(10)
            finally:
                raise TypeError

        async def runner():
            async with taskgroups.TaskGroup() as g:
                g.create_task(crash_soon())
                await nested()

        with self.assertRaises(KeyboardInterrupt):
            await runner()

    async def test_taskgroup_21a(self):

        async def crash_soon():
            await asyncio.sleep(0.1)
            raise MyBaseExc

        async def nested():
            try:
                await asyncio.sleep(10)
            finally:
                raise TypeError

        async def runner():
            async with taskgroups.TaskGroup() as g:
                g.create_task(crash_soon())
                await nested()

        with self.assertRaises(BaseExceptionGroup) as cm:
            await runner()

        self.assertEqual(get_error_types(cm.exception), {MyBaseExc, TypeError})

    async def test_taskgroup_22(self):

        async def foo1():
            await asyncio.sleep(1)
            return 42

        async def foo2():
            await asyncio.sleep(2)
            return 11

        async def runner():
            async with taskgroups.TaskGroup() as g:
                g.create_task(foo1())
                g.create_task(foo2())

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.05)
        r.cancel()

        with self.assertRaises(asyncio.CancelledError):
            await r

    async def test_taskgroup_23(self):

        async def do_job(delay):
            await asyncio.sleep(delay)

        async with taskgroups.TaskGroup() as g:
            for count in range(10):
                await asyncio.sleep(0.1)
                g.create_task(do_job(0.3))
                if count == 5:
                    self.assertLess(len(g._tasks), 5)
            await asyncio.sleep(1.35)
            self.assertEqual(len(g._tasks), 0)

    async def test_taskgroup_24(self):

        async def root(g):
            await asyncio.sleep(0.1)
            g.create_task(coro1(0.1))
            g.create_task(coro1(0.2))

        async def coro1(delay):
            await asyncio.sleep(delay)

        async def runner():
            async with taskgroups.TaskGroup() as g:
                g.create_task(root(g))

        await runner()

    async def test_taskgroup_25(self):
        nhydras = 0

        async def hydra(g):
            nonlocal nhydras
            nhydras += 1
            await asyncio.sleep(0.01)
            g.create_task(hydra(g))
            g.create_task(hydra(g))

        async def hercules():
            while nhydras < 10:
                await asyncio.sleep(0.015)
            1 / 0

        async def runner():
            async with taskgroups.TaskGroup() as g:
                g.create_task(hydra(g))
                g.create_task(hercules())

        with self.assertRaises(ExceptionGroup) as cm:
            await runner()

        self.assertEqual(get_error_types(cm.exception), {ZeroDivisionError})
        self.assertGreaterEqual(nhydras, 10)

    async def test_taskgroup_task_name(self):
        async def coro():
            await asyncio.sleep(0)
        async with taskgroups.TaskGroup() as g:
            t = g.create_task(coro(), name="yolo")
            self.assertEqual(t.get_name(), "yolo")

    async def test_taskgroup_task_context(self):
        cvar = contextvars.ContextVar('cvar')

        async def coro(val):
            await asyncio.sleep(0)
            cvar.set(val)

        async with taskgroups.TaskGroup() as g:
            ctx = contextvars.copy_context()
            self.assertIsNone(ctx.get(cvar))
            t1 = g.create_task(coro(1), context=ctx)
            await t1
            self.assertEqual(1, ctx.get(cvar))
            t2 = g.create_task(coro(2), context=ctx)
            await t2
            self.assertEqual(2, ctx.get(cvar))


if __name__ == "__main__":
    unittest.main()
