import asyncio
import contextvars
import unittest
from test import support

support.requires_working_socket(module=True)


class MyException(Exception):
    pass


def tearDownModule():
    asyncio.set_event_loop_policy(None)


VAR = contextvars.ContextVar('VAR', default=())


class TestAsyncCase(unittest.TestCase):
    maxDiff = None

    def tearDown(self):
        # Ensure that IsolatedAsyncioTestCase instances are destroyed before
        # starting a new event loop
        support.gc_collect()

    def test_full_cycle(self):
        class Test(unittest.IsolatedAsyncioTestCase):
            def setUp(self):
                self.assertEqual(events, [])
                events.append('setUp')
                VAR.set(VAR.get() + ('setUp',))

            async def asyncSetUp(self):
                self.assertEqual(events, ['setUp'])
                events.append('asyncSetUp')
                VAR.set(VAR.get() + ('asyncSetUp',))
                self.addAsyncCleanup(self.on_cleanup1)

            async def test_func(self):
                self.assertEqual(events, ['setUp',
                                          'asyncSetUp'])
                events.append('test')
                VAR.set(VAR.get() + ('test',))
                self.addAsyncCleanup(self.on_cleanup2)

            async def asyncTearDown(self):
                self.assertEqual(events, ['setUp',
                                          'asyncSetUp',
                                          'test'])
                VAR.set(VAR.get() + ('asyncTearDown',))
                events.append('asyncTearDown')

            def tearDown(self):
                self.assertEqual(events, ['setUp',
                                          'asyncSetUp',
                                          'test',
                                          'asyncTearDown'])
                events.append('tearDown')
                VAR.set(VAR.get() + ('tearDown',))

            async def on_cleanup1(self):
                self.assertEqual(events, ['setUp',
                                          'asyncSetUp',
                                          'test',
                                          'asyncTearDown',
                                          'tearDown',
                                          'cleanup2'])
                events.append('cleanup1')
                VAR.set(VAR.get() + ('cleanup1',))
                nonlocal cvar
                cvar = VAR.get()

            async def on_cleanup2(self):
                self.assertEqual(events, ['setUp',
                                          'asyncSetUp',
                                          'test',
                                          'asyncTearDown',
                                          'tearDown'])
                events.append('cleanup2')
                VAR.set(VAR.get() + ('cleanup2',))

        events = []
        cvar = ()
        test = Test("test_func")
        result = test.run()
        self.assertEqual(result.errors, [])
        self.assertEqual(result.failures, [])
        expected = ['setUp', 'asyncSetUp', 'test',
                    'asyncTearDown', 'tearDown', 'cleanup2', 'cleanup1']
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
        try:
            test.debug()
        except MyException:
            pass
        else:
            self.fail('Expected a MyException exception')
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown'])
        test.doCleanups()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup'])

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
        # Issue 41322 - deprecate return of value!=None from a test
        class Test(unittest.IsolatedAsyncioTestCase):
            async def test1(self):
                return 1
            async def test2(self):
                yield 1

        with self.assertWarns(DeprecationWarning) as w:
            Test('test1').run()
        self.assertIn('It is deprecated to return a value!=None', str(w.warnings[0].message))
        self.assertIn('test1', str(w.warnings[0].message))
        self.assertEqual(w.warnings[0].filename, __file__)

        with self.assertWarns(DeprecationWarning) as w:
            Test('test2').run()
        self.assertIn('It is deprecated to return a value!=None', str(w.warnings[0].message))
        self.assertIn('test2', str(w.warnings[0].message))
        self.assertEqual(w.warnings[0].filename, __file__)

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
        try:
            test.debug()
        except MyException:
            pass
        else:
            self.fail('Expected a MyException exception')
        self.assertEqual(events, ['asyncSetUp', 'test'])
        test.doCleanups()
        self.assertEqual(events, ['asyncSetUp', 'test', 'cleanup'])


if __name__ == "__main__":
    unittest.main()
