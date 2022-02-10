#
# This source file is part of the EdgeDB open source project.
#
# Copyright 2016-present MagicStack Inc. and the EdgeDB authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


import asyncio

from edb.common import taskgroup
from edb.testbase import server as tb


class MyExc(Exception):
    pass


class TestTaskGroup(tb.TestCase):

    async def test_taskgroup_01(self):

        async def foo1():
            await asyncio.sleep(0.1)
            return 42

        async def foo2():
            await asyncio.sleep(0.2)
            return 11

        async with taskgroup.TaskGroup() as g:
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

        async with taskgroup.TaskGroup() as g:
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

        async with taskgroup.TaskGroup() as g:
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

            async with taskgroup.TaskGroup() as g:
                g.create_task(foo1())
                t2 = g.create_task(foo2())

            NUM += 10

        with self.assertRaisesRegex(taskgroup.TaskGroupError,
                                    r'1 sub errors: \(ZeroDivisionError\)'):
            await self.loop.create_task(runner())

        self.assertEqual(NUM, 0)
        self.assertTrue(t2_cancel)
        self.assertTrue(t2.cancelled())

    async def test_taskgroup_05(self):

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

            async with taskgroup.TaskGroup() as g:
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
        with self.assertRaisesRegex(
            taskgroup.TaskGroupError,
            r'(1|2|3) sub errors: \(ZeroDivisionError\)',
        ):
            await self.loop.create_task(runner())

        self.assertEqual(NUM, 0)
        self.assertTrue(t2_cancel)
        self.assertTrue(runner_cancel)

    async def test_taskgroup_06(self):

        NUM = 0

        async def foo():
            nonlocal NUM
            try:
                await asyncio.sleep(5)
            except asyncio.CancelledError:
                NUM += 1
                raise

        async def runner():
            async with taskgroup.TaskGroup() as g:
                for _ in range(5):
                    g.create_task(foo())

        r = self.loop.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError):
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
            async with taskgroup.TaskGroup() as g:
                for _ in range(5):
                    g.create_task(foo())

                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    NUM += 10
                    raise

        r = self.loop.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await r

        self.assertEqual(NUM, 15)

    async def test_taskgroup_08(self):

        async def foo():
            await asyncio.sleep(0.1)
            1 / 0

        async def runner():
            async with taskgroup.TaskGroup() as g:
                for _ in range(5):
                    g.create_task(foo())

                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    raise

        r = self.loop.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError):
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
            async with taskgroup.TaskGroup() as g:
                t1 = g.create_task(foo1())
                t2 = g.create_task(foo2())
                await asyncio.sleep(0.1)
                1 / 0

        try:
            await runner()
        except taskgroup.TaskGroupError as t:
            self.assertEqual(t.get_error_types(), {ZeroDivisionError})
        else:
            self.fail('TaskGroupError was not raised')

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
            async with taskgroup.TaskGroup() as g:
                t1 = g.create_task(foo1())
                t2 = g.create_task(foo2())
                1 / 0

        try:
            await runner()
        except taskgroup.TaskGroupError as t:
            self.assertEqual(t.get_error_types(), {ZeroDivisionError})
        else:
            self.fail('TaskGroupError was not raised')

        self.assertTrue(t1.cancelled())
        self.assertTrue(t2.cancelled())

    async def test_taskgroup_11(self):

        async def foo():
            await asyncio.sleep(0.1)
            1 / 0

        async def runner():
            async with taskgroup.TaskGroup():
                async with taskgroup.TaskGroup() as g2:
                    for _ in range(5):
                        g2.create_task(foo())

                    try:
                        await asyncio.sleep(10)
                    except asyncio.CancelledError:
                        raise

        r = self.loop.create_task(runner())
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
            async with taskgroup.TaskGroup() as g1:
                g1.create_task(asyncio.sleep(10))

                async with taskgroup.TaskGroup() as g2:
                    for _ in range(5):
                        g2.create_task(foo())

                    try:
                        await asyncio.sleep(10)
                    except asyncio.CancelledError:
                        raise

        r = self.loop.create_task(runner())
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
            async with taskgroup.TaskGroup(name='g1') as g1:
                g1.create_task(crash_after(0.1))

                async with taskgroup.TaskGroup(name='g2') as g2:
                    g2.create_task(crash_after(0.2))

        r = self.loop.create_task(runner())
        with self.assertRaisesRegex(taskgroup.TaskGroupError, r'1 sub errors'):
            await r

    async def test_taskgroup_14(self):

        async def crash_after(t):
            await asyncio.sleep(t)
            raise ValueError(t)

        async def runner():
            async with taskgroup.TaskGroup(name='g1') as g1:
                g1.create_task(crash_after(0.2))

                async with taskgroup.TaskGroup(name='g2') as g2:
                    g2.create_task(crash_after(0.1))

        r = self.loop.create_task(runner())
        with self.assertRaisesRegex(taskgroup.TaskGroupError, r'1 sub errors'):
            await r

    async def test_taskgroup_15(self):

        async def crash_soon():
            await asyncio.sleep(0.3)
            1 / 0

        async def runner():
            async with taskgroup.TaskGroup(name='g1') as g1:
                g1.create_task(crash_soon())
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    await asyncio.sleep(0.5)
                    raise

        r = self.loop.create_task(runner())
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
            async with taskgroup.TaskGroup(name='g1') as g1:
                g1.create_task(crash_soon())
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    await asyncio.sleep(0.5)
                    raise

        async def runner():
            t = self.loop.create_task(nested_runner())
            await t

        r = self.loop.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await r

    async def test_taskgroup_17(self):
        NUM = 0

        async def runner():
            nonlocal NUM
            async with taskgroup.TaskGroup():
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    NUM += 10
                    raise

        r = self.loop.create_task(runner())
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
            async with taskgroup.TaskGroup():
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    NUM += 10
                    # This isn't a good idea, but we have to support
                    # this weird case.
                    raise MyExc

        r = self.loop.create_task(runner())
        await asyncio.sleep(0.1)

        self.assertFalse(r.done())
        r.cancel()

        try:
            await r
        except taskgroup.TaskGroupError as t:
            self.assertEqual(t.get_error_types(), {MyExc})
        else:
            self.fail('TaskGroupError was not raised')

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
            async with taskgroup.TaskGroup() as g:
                g.create_task(crash_soon())
                await nested()

        r = self.loop.create_task(runner())
        try:
            await r
        except taskgroup.TaskGroupError as t:
            self.assertEqual(t.get_error_types(), {MyExc, ZeroDivisionError})
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
            async with taskgroup.TaskGroup() as g:
                g.create_task(crash_soon())
                await nested()

        with self.assertRaises(KeyboardInterrupt):
            await runner()

    async def _test_taskgroup_21(self):
        # This test doesn't work as asyncio, currently, doesn't
        # know how to handle BaseExceptions.

        async def crash_soon():
            await asyncio.sleep(0.1)
            raise KeyboardInterrupt

        async def nested():
            try:
                await asyncio.sleep(10)
            finally:
                raise TypeError

        async def runner():
            async with taskgroup.TaskGroup() as g:
                g.create_task(crash_soon())
                await nested()

        with self.assertRaises(KeyboardInterrupt):
            await runner()

    async def test_taskgroup_22(self):

        async def foo1():
            await asyncio.sleep(1)
            return 42

        async def foo2():
            await asyncio.sleep(2)
            return 11

        async def runner():
            async with taskgroup.TaskGroup() as g:
                g.create_task(foo1())
                g.create_task(foo2())

        r = self.loop.create_task(runner())
        await asyncio.sleep(0.05)
        r.cancel()

        with self.assertRaises(asyncio.CancelledError):
            await r

    async def test_taskgroup_23(self):

        async def do_job(delay):
            await asyncio.sleep(delay)

        async with taskgroup.TaskGroup() as g:
            for count in range(10):
                await asyncio.sleep(0.1)
                g.create_task(do_job(0.3))
                if count == 5:
                    self.assertLess(len(g._tasks), 5)
            await asyncio.sleep(1.35)
            self.assertEqual(len(g._tasks), 0)
