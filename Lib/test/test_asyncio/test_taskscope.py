# license: PSFL.

import unittest
from unittest import mock

import asyncio
from asyncio import taskscope
from test.test_asyncio import utils as test_utils


# To prevent a warning "test altered the execution environment"
def tearDownModule():
    asyncio.set_event_loop_policy(None)


class MyExc(Exception):
    pass


class MyBaseExc(BaseException):
    pass


def get_error_types(eg):
    return {type(exc) for exc in eg.exceptions}


class TestTaskScope(unittest.IsolatedAsyncioTestCase):

    async def test_children_complete_on_child_error(self):
        async def zero_division():
            1 / 0

        async def foo1():
            await asyncio.sleep(0.1)
            return 42

        async def foo2():
            await asyncio.sleep(0.2)
            return 11

        loop = asyncio.get_running_loop()
        exc_handler = mock.Mock()
        with mock.patch.object(loop, 'call_exception_handler', exc_handler):
            async with taskscope.TaskScope() as g:
                t1 = g.create_task(foo1())
                t2 = g.create_task(foo2())
                t3 = g.create_task(zero_division())

        self.assertEqual(t1.result(), 42)
        self.assertEqual(t2.result(), 11)
        exc_handler.assert_called_with({
            'message': test_utils.MockPattern(
                '^Task .* has errored inside the parent .*'
            ),
            'exception': test_utils.MockInstanceOf(ZeroDivisionError),
            'task': t3,
        })

    async def test_inner_complete_on_child_error(self):
        async def zero_division():
            1 / 0

        async def foo1():
            await asyncio.sleep(0.1)
            return 42

        async def foo2():
            await asyncio.sleep(0.2)
            return 11

        loop = asyncio.get_running_loop()
        exc_handler = mock.Mock()
        with mock.patch.object(loop, 'call_exception_handler', exc_handler):
            async with taskscope.TaskScope() as g:
                t1 = g.create_task(foo1())
                t2 = g.create_task(zero_division())
                r1 = await foo2()

        self.assertEqual(t1.result(), 42)
        self.assertEqual(r1, 11)
        exc_handler.assert_called_with({
            'message': test_utils.MockPattern(
                '^Task .* has errored inside the parent .*'
            ),
            'exception': test_utils.MockInstanceOf(ZeroDivisionError),
            'task': t2,
        })

    async def test_children_exceptions_propagate(self):
        async def zero_division():
            1 / 0

        async def value_error():
            await asyncio.sleep(0.2)
            raise ValueError

        async def foo1():
            await asyncio.sleep(0.4)
            return 42

        loop = asyncio.get_running_loop()
        exc_handler = mock.Mock()
        with mock.patch.object(loop, 'call_exception_handler', exc_handler):
            async with taskscope.TaskScope() as g:
                t1 = g.create_task(zero_division())
                t2 = g.create_task(value_error())
                t3 = g.create_task(foo1())

        exc_handler.assert_has_calls(
            [
                mock.call({
                    'message': test_utils.MockPattern(
                        '^Task .* has errored inside the parent .*'
                    ),
                    'exception': test_utils.MockInstanceOf(ZeroDivisionError),
                    'task': t1,
                }),
                mock.call({
                    'message': test_utils.MockPattern(
                        '^Task .* has errored inside the parent .*'
                    ),
                    'exception': test_utils.MockInstanceOf(ValueError),
                    'task': t2,
                }),
            ],
            any_order=True,
        )
        self.assertEqual(t3.result(), 42)

    async def test_children_cancel_on_inner_failure(self):
        async def zero_division():
            1 / 0

        async def foo1():
            await asyncio.sleep(0.2)
            return 42

        with self.assertRaises(ZeroDivisionError):
            async with taskscope.TaskScope() as g:
                t1 = g.create_task(foo1())
                await zero_division()

        self.assertTrue(t1.cancelled())

    async def test_cancellation_01(self):

        NUM = 0

        async def foo():
            nonlocal NUM
            try:
                await asyncio.sleep(5)
            except asyncio.CancelledError:
                NUM += 1
                raise

        async def runner():
            async with taskscope.TaskScope() as g:
                for _ in range(5):
                    g.create_task(foo())

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError) as cm:
            await r

        self.assertEqual(NUM, 5)

    async def test_taskgroup_35(self):

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
            async with taskscope.TaskScope() as g:
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

    async def test_taskgroup_36(self):

        async def foo():
            try:
                await asyncio.sleep(10)
            finally:
                1 / 0

        async def runner():
            async with taskscope.TaskScope() as g:
                for _ in range(5):
                    g.create_task(foo())

                await asyncio.sleep(10)

        loop = asyncio.get_running_loop()
        exc_handler = mock.Mock()
        with mock.patch.object(loop, 'call_exception_handler', exc_handler):
            r = asyncio.create_task(runner())
            await asyncio.sleep(0.1)

            self.assertFalse(r.done())
            r.cancel()
            with self.assertRaises(asyncio.CancelledError):
                await r

        exc_handler.assert_called_with({
            'message': test_utils.MockPattern(
                '^Task .* has errored inside the parent .*'
            ),
            'exception': test_utils.MockInstanceOf(ZeroDivisionError),
            'task': mock.ANY,
        })
        self.assertEqual(len(exc_handler.call_args_list), 5)

    async def test_taskgroup_37(self):

        async def foo():
            try:
                await asyncio.sleep(10)
            finally:
                1 / 0

        async def runner():
            async with taskscope.TaskScope():
                async with taskscope.TaskScope() as g2:
                    for _ in range(5):
                        g2.create_task(foo())

                    await asyncio.sleep(10)

        loop = asyncio.get_running_loop()
        exc_handler = mock.Mock()
        with mock.patch.object(loop, 'call_exception_handler', exc_handler):
            r = asyncio.create_task(runner())
            await asyncio.sleep(0.1)

            self.assertFalse(r.done())
            r.cancel()
            with self.assertRaises(asyncio.CancelledError):
                await r

        exc_handler.assert_called_with({
            'message': test_utils.MockPattern(
                '^Task .* has errored inside the parent .*'
            ),
            'exception': test_utils.MockInstanceOf(ZeroDivisionError),
            'task': mock.ANY,
        })
        self.assertEqual(len(exc_handler.call_args_list), 5)

    async def test_taskgroup_38(self):

        async def foo():
            try:
                await asyncio.sleep(10)
            finally:
                1 / 0

        async def runner():
            async with taskscope.TaskScope() as g1:
                g1.create_task(asyncio.sleep(10))

                async with taskscope.TaskScope() as g2:
                    for _ in range(5):
                        g2.create_task(foo())

                    await asyncio.sleep(10)

        loop = asyncio.get_running_loop()
        exc_handler = mock.Mock()
        with mock.patch.object(loop, 'call_exception_handler', exc_handler):
            r = asyncio.create_task(runner())
            await asyncio.sleep(0.1)

            self.assertFalse(r.done())
            r.cancel()
            with self.assertRaises(asyncio.CancelledError):
                await r

        exc_handler.assert_called_with({
            'message': test_utils.MockPattern(
                '^Task .* has errored inside the parent .*'
            ),
            'exception': test_utils.MockInstanceOf(ZeroDivisionError),
            'task': mock.ANY,
        })
        self.assertEqual(len(exc_handler.call_args_list), 5)

    async def test_taskgroup_39(self):

        async def crash_soon():
            await asyncio.sleep(0.3)
            1 / 0

        async def runner():
            async with taskscope.TaskScope() as g1:
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
        with self.assertRaises(ExceptionGroup) as cm:
            await r
        self.assertEqual(get_error_types(cm.exception), {ZeroDivisionError})

    async def test_taskgroup_40(self):

        async def crash_soon():
            await asyncio.sleep(0.3)
            1 / 0

        async def nested_runner():
            async with taskscope.TaskScope() as g1:
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
        with self.assertRaises(ExceptionGroup) as cm:
            await r
        self.assertEqual(get_error_types(cm.exception), {ZeroDivisionError})

    async def test_taskgroup_41(self):

        NUM = 0

        async def runner():
            nonlocal NUM
            async with taskscope.TaskScope():
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

    async def test_taskgroup_42(self):

        NUM = 0

        async def runner():
            nonlocal NUM
            async with taskscope.TaskScope():
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

    async def test_taskgroup_43(self):

        async def crash_soon():
            await asyncio.sleep(0.1)
            1 / 0

        async def nested():
            try:
                await asyncio.sleep(10)
            finally:
                raise MyExc

        async def runner():
            async with taskscope.TaskScope() as g:
                g.create_task(crash_soon())
                await nested()

        r = asyncio.create_task(runner())
        try:
            await r
        except ExceptionGroup as t:
            self.assertEqual(get_error_types(t), {MyExc, ZeroDivisionError})
        else:
            self.fail('TasgGroupError was not raised')

    async def test_taskgroup_44(self):

        async def foo1():
            await asyncio.sleep(1)
            return 42

        async def foo2():
            await asyncio.sleep(2)
            return 11

        async def runner():
            async with taskscope.TaskScope() as g:
                g.create_task(foo1())
                g.create_task(foo2())

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.05)
        r.cancel()

        with self.assertRaises(asyncio.CancelledError):
            await r

    async def test_taskgroup_45(self):

        NUM = 0

        async def foo1():
            nonlocal NUM
            await asyncio.sleep(0.2)
            NUM += 1

        async def foo2():
            nonlocal NUM
            await asyncio.sleep(0.3)
            NUM += 2

        async def runner():
            async with taskscope.TaskScope() as g:
                g.create_task(foo1())
                g.create_task(foo2())

        r = asyncio.create_task(runner())
        await asyncio.sleep(0.05)
        r.cancel()

        with self.assertRaises(asyncio.CancelledError):
            await r

        self.assertEqual(NUM, 0)


