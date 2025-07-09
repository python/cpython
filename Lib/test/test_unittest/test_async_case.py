import asyncio
import contextvars
import unittest
from test import support
from test.support import force_not_colorized

support.requires_working_socket(module=True)


class MyException(Exception):
    pass


def tearDownModule():
    asyncio._set_event_loop_policy(None)


class TestCM:
    def __init__(self, ordering, enter_result=None):
        self.ordering = ordering
        self.enter_result = enter_result

    async def __aenter__(self):
        self.ordering.append('enter')
        return self.enter_result

    async def __aexit__(self, *exc_info):
        self.ordering.append('exit')


class LacksEnterAndExit:
    pass
class LacksEnter:
    async def __aexit__(self, *exc_info):
        pass
class LacksExit:
    async def __aenter__(self):
        pass


VAR = contextvars.ContextVar('VAR', default=())


class TestAsyncCase(unittest.TestCase):
    maxDiff = None

    def setUp(self):
        # Ensure that IsolatedAsyncioTestCase instances are destroyed before
        # starting a new event loop
        self.addCleanup(support.gc_collect)

    def test_full_cycle(self):
        expected = ['setUp',
                    'asyncSetUp',
                    'test',
                    'asyncTearDown',
                    'tearDown',
                    'cleanup6',
                    'cleanup5',
                    'cleanup4',
                    'cleanup3',
                    'cleanup2',
                    'cleanup1']
        class Test(unittest.IsolatedAsyncioTestCase):
            def setUp(self):
                self.assertEqual(events, [])
                events.append('setUp')
                VAR.set(VAR.get() + ('setUp',))
                self.addCleanup(self.on_cleanup1)
                self.addAsyncCleanup(self.on_cleanup2)

            async def asyncSetUp(self):
                self.assertEqual(events, expected[:1])
                events.append('asyncSetUp')
                VAR.set(VAR.get() + ('asyncSetUp',))
                self.addCleanup(self.on_cleanup3)
                self.addAsyncCleanup(self.on_cleanup4)

            async def test_func(self):
                self.assertEqual(events, expected[:2])
                events.append('test')
                VAR.set(VAR.get() + ('test',))
                self.addCleanup(self.on_cleanup5)
                self.addAsyncCleanup(self.on_cleanup6)

            async def asyncTearDown(self):
                self.assertEqual(events, expected[:3])
                VAR.set(VAR.get() + ('asyncTearDown',))
                events.append('asyncTearDown')

            def tearDown(self):
                self.assertEqual(events, expected[:4])
                events.append('tearDown')
                VAR.set(VAR.get() + ('tearDown',))

            def on_cleanup1(self):
                self.assertEqual(events, expected[:10])
                events.append('cleanup1')
                VAR.set(VAR.get() + ('cleanup1',))
                nonlocal cvar
                cvar = VAR.get()

            async def on_cleanup2(self):
                self.assertEqual(events, expected[:9])
                events.append('cleanup2')
                VAR.set(VAR.get() + ('cleanup2',))

            def on_cleanup3(self):
                self.assertEqual(events, expected[:8])
                events.append('cleanup3')
                VAR.set(VAR.get() + ('cleanup3',))

            async def on_cleanup4(self):
                self.assertEqual(events, expected[:7])
                events.append('cleanup4')
                VAR.set(VAR.get() + ('cleanup4',))

            def on_cleanup5(self):
                self.assertEqual(events, expected[:6])
                events.append('cleanup5')
                VAR.set(VAR.get() + ('cleanup5',))

            async def on_cleanup6(self):
                self.assertEqual(events, expected[:5])
                events.append('cleanup6')
                VAR.set(VAR.get() + ('cleanup6',))

        events = []
        cvar = ()
        test = Test("test_func")
        result = test.run()
        self.assertEqual(result.errors, [])
        self.assertEqual(result.failures, [])
        self.assertEqual(events, expected)
        self.assertEqual(cvar, tuple(expected))

        events = []
        cvar = ()
        test = Test("test_func")
        test.debug()
        self.assertEqual(events, expected)
        self.assertEqual(cvar, tuple(expected))
        test.doCleanups()
        self.assertEqual(events, expected)
        self.assertEqual(cvar, tuple(expected))

    def test_exception_in_setup(self):
        class Test(unittest.IsolatedAsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')
                self.addAsyncCleanup(self.on_cleanup)
                raise MyException()

            async def test_func(self):
                events.append('test')

            async def asyncTearDown(self):
                events.append('asyncTearDown')

            async def on_cleanup(self):
                events.append('cleanup')


        events = []
        test = Test("test_func")
        result = test.run()
        self.assertEqual(events, ['asyncSetUp', 'cleanup'])
        self.assertIs(result.errors[0][0], test)
        self.assertIn('MyException', result.errors[0][1])

        events = []
        test = Test("test_func")
        self.addCleanup(test._tearDownAsyncioRunner)
        try:
            test.debug()
        except MyException:
            pass
        else:
            self.fail('Expected a MyException exception')
        self.assertEqual(events, ['asyncSetUp'])
        test.doCleanups()
        self.assertEqual(events, ['asyncSetUp', 'cleanup'])

    def test_exception_in_test(self):
        class Test(unittest.IsolatedAsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')

            async def test_func(self):
                events.append('test')
                self.addAsyncCleanup(self.on_cleanup)
                raise MyException()

            async def asyncTearDown(self):
                events.append('asyncTearDown')

            async def on_cleanup(self):
                events.append('cleanup')

        events = []
        test = Test("test_func")
        result = test.run()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup'])
        self.assertIs(result.errors[0][0], test)
        self.assertIn('MyException', result.errors[0][1])

        events = []
        test = Test("test_func")
        self.addCleanup(test._tearDownAsyncioRunner)
        try:
            test.debug()
        except MyException:
            pass
        else:
            self.fail('Expected a MyException exception')
        self.assertEqual(events, ['asyncSetUp', 'test'])
        test.doCleanups()
        self.assertEqual(events, ['asyncSetUp', 'test', 'cleanup'])

    def test_exception_in_tear_down(self):
        class Test(unittest.IsolatedAsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')

            async def test_func(self):
                events.append('test')
                self.addAsyncCleanup(self.on_cleanup)

            async def asyncTearDown(self):
                events.append('asyncTearDown')
                raise MyException()

            async def on_cleanup(self):
                events.append('cleanup')

        events = []
        test = Test("test_func")
        result = test.run()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup'])
        self.assertIs(result.errors[0][0], test)
        self.assertIn('MyException', result.errors[0][1])

        events = []
        test = Test("test_func")
        self.addCleanup(test._tearDownAsyncioRunner)
        try:
            test.debug()
        except MyException:
            pass
        else:
            self.fail('Expected a MyException exception')
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown'])
        test.doCleanups()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup'])

    @force_not_colorized
    def test_exception_in_tear_clean_up(self):
        class Test(unittest.IsolatedAsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')

            async def test_func(self):
                events.append('test')
                self.addAsyncCleanup(self.on_cleanup1)
                self.addAsyncCleanup(self.on_cleanup2)

            async def asyncTearDown(self):
                events.append('asyncTearDown')

            async def on_cleanup1(self):
                events.append('cleanup1')
                raise MyException('some error')

            async def on_cleanup2(self):
                events.append('cleanup2')
                raise MyException('other error')

        events = []
        test = Test("test_func")
        result = test.run()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup2', 'cleanup1'])
        self.assertIs(result.errors[0][0], test)
        self.assertIn('MyException: other error', result.errors[0][1])
        self.assertIn('MyException: some error', result.errors[1][1])

        events = []
        test = Test("test_func")
        self.addCleanup(test._tearDownAsyncioRunner)
        try:
            test.debug()
        except MyException:
            pass
        else:
            self.fail('Expected a MyException exception')
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup2'])
        test.doCleanups()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup2', 'cleanup1'])

    def test_deprecation_of_return_val_from_test(self):
        # Issue 41322 - deprecate return of value that is not None from a test
        class Nothing:
            def __eq__(self, o):
                return o is None
        class Test(unittest.IsolatedAsyncioTestCase):
            async def test1(self):
                return 1
            async def test2(self):
                yield 1
            async def test3(self):
                return Nothing()

        with self.assertWarns(DeprecationWarning) as w:
            Test('test1').run()
        self.assertIn('It is deprecated to return a value that is not None', str(w.warning))
        self.assertIn('test1', str(w.warning))
        self.assertEqual(w.filename, __file__)
        self.assertIn("returned 'int'", str(w.warning))

        with self.assertWarns(DeprecationWarning) as w:
            Test('test2').run()
        self.assertIn('It is deprecated to return a value that is not None', str(w.warning))
        self.assertIn('test2', str(w.warning))
        self.assertEqual(w.filename, __file__)
        self.assertIn("returned 'async_generator'", str(w.warning))

        with self.assertWarns(DeprecationWarning) as w:
            Test('test3').run()
        self.assertIn('It is deprecated to return a value that is not None', str(w.warning))
        self.assertIn('test3', str(w.warning))
        self.assertEqual(w.filename, __file__)
        self.assertIn(f'returned {Nothing.__name__!r}', str(w.warning))

    def test_cleanups_interleave_order(self):
        events = []

        class Test(unittest.IsolatedAsyncioTestCase):
            async def test_func(self):
                self.addAsyncCleanup(self.on_sync_cleanup, 1)
                self.addAsyncCleanup(self.on_async_cleanup, 2)
                self.addAsyncCleanup(self.on_sync_cleanup, 3)
                self.addAsyncCleanup(self.on_async_cleanup, 4)

            async def on_sync_cleanup(self, val):
                events.append(f'sync_cleanup {val}')

            async def on_async_cleanup(self, val):
                events.append(f'async_cleanup {val}')

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['async_cleanup 4',
                                  'sync_cleanup 3',
                                  'async_cleanup 2',
                                  'sync_cleanup 1'])

    def test_base_exception_from_async_method(self):
        events = []
        class Test(unittest.IsolatedAsyncioTestCase):
            async def test_base(self):
                events.append("test_base")
                raise BaseException()
                events.append("not it")

            async def test_no_err(self):
                events.append("test_no_err")

            async def test_cancel(self):
                raise asyncio.CancelledError()

        test = Test("test_base")
        output = test.run()
        self.assertFalse(output.wasSuccessful())

        test = Test("test_no_err")
        test.run()
        self.assertEqual(events, ['test_base', 'test_no_err'])

        test = Test("test_cancel")
        output = test.run()
        self.assertFalse(output.wasSuccessful())

    def test_cancellation_hanging_tasks(self):
        cancelled = False
        class Test(unittest.IsolatedAsyncioTestCase):
            async def test_leaking_task(self):
                async def coro():
                    nonlocal cancelled
                    try:
                        await asyncio.sleep(1)
                    except asyncio.CancelledError:
                        cancelled = True
                        raise

                # Leave this running in the background
                asyncio.create_task(coro())

        test = Test("test_leaking_task")
        output = test.run()
        self.assertTrue(cancelled)

    def test_enterAsyncContext(self):
        events = []

        class Test(unittest.IsolatedAsyncioTestCase):
            async def test_func(slf):
                slf.addAsyncCleanup(events.append, 'cleanup1')
                cm = TestCM(events, 42)
                self.assertEqual(await slf.enterAsyncContext(cm), 42)
                slf.addAsyncCleanup(events.append, 'cleanup2')
                events.append('test')

        test = Test('test_func')
        output = test.run()
        self.assertTrue(output.wasSuccessful(), output)
        self.assertEqual(events, ['enter', 'test', 'cleanup2', 'exit', 'cleanup1'])

    def test_enterAsyncContext_arg_errors(self):
        class Test(unittest.IsolatedAsyncioTestCase):
            async def test_func(slf):
                with self.assertRaisesRegex(TypeError, 'asynchronous context manager'):
                    await slf.enterAsyncContext(LacksEnterAndExit())
                with self.assertRaisesRegex(TypeError, 'asynchronous context manager'):
                    await slf.enterAsyncContext(LacksEnter())
                with self.assertRaisesRegex(TypeError, 'asynchronous context manager'):
                    await slf.enterAsyncContext(LacksExit())

        test = Test('test_func')
        output = test.run()
        self.assertTrue(output.wasSuccessful())

    def test_debug_cleanup_same_loop(self):
        class Test(unittest.IsolatedAsyncioTestCase):
            async def asyncSetUp(self):
                async def coro():
                    await asyncio.sleep(0)
                fut = asyncio.ensure_future(coro())
                self.addAsyncCleanup(self.cleanup, fut)
                events.append('asyncSetUp')

            async def test_func(self):
                events.append('test')
                raise MyException()

            async def asyncTearDown(self):
                events.append('asyncTearDown')

            async def cleanup(self, fut):
                try:
                    # Raises an exception if in different loop
                    await asyncio.wait([fut])
                    events.append('cleanup')
                except:
                    import traceback
                    traceback.print_exc()
                    raise

        events = []
        test = Test("test_func")
        result = test.run()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup'])
        self.assertIn('MyException', result.errors[0][1])

        events = []
        test = Test("test_func")
        self.addCleanup(test._tearDownAsyncioRunner)
        try:
            test.debug()
        except MyException:
            pass
        else:
            self.fail('Expected a MyException exception')
        self.assertEqual(events, ['asyncSetUp', 'test'])
        test.doCleanups()
        self.assertEqual(events, ['asyncSetUp', 'test', 'cleanup'])

    def test_setup_get_event_loop(self):
        # See https://github.com/python/cpython/issues/95736
        # Make sure the default event loop is not used
        asyncio.set_event_loop(None)

        class TestCase1(unittest.IsolatedAsyncioTestCase):
            def setUp(self):
                asyncio._get_event_loop_policy().get_event_loop()

            async def test_demo1(self):
                pass

        test = TestCase1('test_demo1')
        result = test.run()
        self.assertTrue(result.wasSuccessful())

    def test_loop_factory(self):
        asyncio._set_event_loop_policy(None)

        class TestCase1(unittest.IsolatedAsyncioTestCase):
            loop_factory = asyncio.EventLoop

            async def test_demo1(self):
                pass

        test = TestCase1('test_demo1')
        result = test.run()
        self.assertTrue(result.wasSuccessful())
        self.assertIsNone(support.maybe_get_event_loop_policy())

if __name__ == "__main__":
    unittest.main()
