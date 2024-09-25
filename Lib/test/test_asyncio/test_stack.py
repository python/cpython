import asyncio
import unittest

import pprint


# To prevent a warning "test altered the execution environment"
def tearDownModule():
    asyncio.set_event_loop_policy(None)


def capture_test_stack(*, fut=None):

    def walk(s):
        ret = [
            f"T<{n}>" if '-' not in (n := s.future.get_name()) else 'T<anon>'
                if isinstance(s.future, asyncio.Task) else 'F'
        ]

        ret.append(
            [
                (
                    f"s {entry.frame.f_code.co_name}"
                        if isinstance(entry, asyncio.FrameCallStackEntry) else
                        (
                            f"a {entry.coroutine.cr_code.co_name}"
                            if hasattr(entry.coroutine, 'cr_code') else
                            f"ag {entry.coroutine.ag_code.co_name}"
                        )
                ) for entry in s.call_stack
            ]
        )

        ret.append(
            sorted([
                walk(ab) for ab in s.awaited_by
            ], key=lambda entry: entry[0])
        )

        return ret

    stack = asyncio.capture_call_stack(future=fut)
    return walk(stack)


class TestCallStack(unittest.IsolatedAsyncioTestCase):


    async def test_stack_tgroup(self):

        stack_for_c5 = None

        def c5():
            nonlocal stack_for_c5
            stack_for_c5 = capture_test_stack()

        async def c4():
            await asyncio.sleep(0)
            c5()

        async def c3():
            await c4()

        async def c2():
            await c3()

        async def c1(task):
            await task

        async def main():
            async with asyncio.TaskGroup() as tg:
                task = tg.create_task(c2(), name="c2_root")
                tg.create_task(c1(task), name="sub_main_1")
                tg.create_task(c1(task), name="sub_main_2")

        await main()

        self.assertEqual(stack_for_c5, [
            # task name
            'T<c2_root>',
            # call stack
            ['s capture_test_stack', 's c5', 'a c4', 'a c3', 'a c2'],
            # awaited by
            [
                ['T<anon>',
                     ['a __aexit__', 'a main', 'a test_stack_tgroup'], []
                ],
                ['T<sub_main_1>',
                    ['a c1'],
                    [
                        ['T<anon>',
                            ['a __aexit__', 'a main', 'a test_stack_tgroup'], []
                        ]
                    ]
                ],
                ['T<sub_main_2>',
                    ['a c1'],
                    [
                        ['T<anon>',
                            ['a __aexit__', 'a main', 'a test_stack_tgroup'], []
                        ]
                    ]
                ]
            ]
        ])

    async def test_stack_async_gen(self):

        stack_for_gen_nested_call = None

        async def gen_nested_call():
            nonlocal stack_for_gen_nested_call
            stack_for_gen_nested_call = capture_test_stack()

        async def gen():
            for num in range(2):
                yield num
                if num == 1:
                    await gen_nested_call()

        async def main():
            async for el in gen():
                pass

        await main()

        self.assertEqual(stack_for_gen_nested_call, [
            'T<anon>',
            [
                's capture_test_stack',
                'a gen_nested_call',
                'ag gen',
                'a main',
                'a test_stack_async_gen'
            ],
            []
        ])

    async def test_stack_gather(self):

        stack_for_deep = None

        async def deep():
            await asyncio.sleep(0)
            nonlocal stack_for_deep
            stack_for_deep = capture_test_stack()

        async def c1():
            await asyncio.sleep(0)
            await deep()

        async def c2():
            await asyncio.sleep(0)

        async def main():
            await asyncio.gather(c1(), c2())

        await main()

        self.assertEqual(stack_for_deep, [
            'T<anon>',
            ['s capture_test_stack', 'a deep', 'a c1'],
            [
                ['T<anon>', ['a main', 'a test_stack_gather'], []]
            ]
        ])

    async def test_stack_shield(self):

        stack_for_shield = None

        async def deep():
            await asyncio.sleep(0)
            nonlocal stack_for_shield
            stack_for_shield = capture_test_stack()

        async def c1():
            await asyncio.sleep(0)
            await deep()

        async def main():
            await asyncio.shield(c1())

        await main()

        self.assertEqual(stack_for_shield, [
            'T<anon>',
            ['s capture_test_stack', 'a deep', 'a c1'],
            [
                ['T<anon>', ['a main', 'a test_stack_shield'], []]
            ]
        ])

    async def test_stack_timeout(self):

        stack_for_inner = None

        async def inner():
            await asyncio.sleep(0)
            nonlocal stack_for_inner
            stack_for_inner = capture_test_stack()

        async def c1():
            async with asyncio.timeout(1):
                await asyncio.sleep(0)
                await inner()

        async def main():
            await asyncio.shield(c1())

        await main()

        self.assertEqual(stack_for_inner, [
            'T<anon>',
            ['s capture_test_stack', 'a inner', 'a c1'],
            [
                ['T<anon>', ['a main', 'a test_stack_timeout'], []]
            ]
        ])

    async def test_stack_wait(self):

        stack_for_inner = None

        async def inner():
            await asyncio.sleep(0)
            nonlocal stack_for_inner
            stack_for_inner = capture_test_stack()

        async def c1():
            async with asyncio.timeout(1):
                await asyncio.sleep(0)
                await inner()

        async def c2():
            for i in range(3):
                await asyncio.sleep(0)

        async def main(t1, t2):
            while True:
                _, pending = await asyncio.wait([t1, t2])
                if not pending:
                    break

        t1 = asyncio.create_task(c1())
        t2 = asyncio.create_task(c2())
        try:
            await main(t1, t2)
        finally:
            await t1
            await t2

        self.assertEqual(stack_for_inner, [
            'T<anon>',
            ['s capture_test_stack', 'a inner', 'a c1'],
            [
                ['T<anon>',
                    ['a _wait', 'a wait', 'a main', 'a test_stack_wait'],
                    []
                ]
            ]
        ])
