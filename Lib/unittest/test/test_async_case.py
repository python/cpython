import asyncio
import unittest


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class TestAsyncCase(unittest.TestCase):
    def test_full_cycle(self):
        events = []

        class Test(unittest.IsolatedAsyncioTestCase):
            def setUp(self):
                self.assertEqual(events, [])
                events.append('setUp')

            async def asyncSetUp(self):
                self.assertEqual(events, ['setUp'])
                events.append('asyncSetUp')

            async def test_func(self):
                self.assertEqual(events, ['setUp',
                                          'asyncSetUp'])
                events.append('test')
                self.addAsyncCleanup(self.on_cleanup)

            async def asyncTearDown(self):
                self.assertEqual(events, ['setUp',
                                          'asyncSetUp',
                                          'test'])
                events.append('asyncTearDown')

            def tearDown(self):
                self.assertEqual(events, ['setUp',
                                          'asyncSetUp',
                                          'test',
                                          'asyncTearDown'])
                events.append('tearDown')

            async def on_cleanup(self):
                self.assertEqual(events, ['setUp',
                                          'asyncSetUp',
                                          'test',
                                          'asyncTearDown',
                                          'tearDown'])
                events.append('cleanup')

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['setUp',
                                  'asyncSetUp',
                                  'test',
                                  'asyncTearDown',
                                  'tearDown',
                                  'cleanup'])

    def test_exception_in_setup(self):
        events = []

        class Test(unittest.IsolatedAsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')
                raise Exception()

            async def test_func(self):
                events.append('test')
                self.addAsyncCleanup(self.on_cleanup)

            async def asyncTearDown(self):
                events.append('asyncTearDown')

            async def on_cleanup(self):
                events.append('cleanup')


        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['asyncSetUp'])

    def test_exception_in_test(self):
        events = []

        class Test(unittest.IsolatedAsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')

            async def test_func(self):
                events.append('test')
                raise Exception()
                self.addAsyncCleanup(self.on_cleanup)

            async def asyncTearDown(self):
                events.append('asyncTearDown')

            async def on_cleanup(self):
                events.append('cleanup')

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown'])

    def test_exception_in_test_after_adding_cleanup(self):
        events = []

        class Test(unittest.IsolatedAsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')

            async def test_func(self):
                events.append('test')
                self.addAsyncCleanup(self.on_cleanup)
                raise Exception()

            async def asyncTearDown(self):
                events.append('asyncTearDown')

            async def on_cleanup(self):
                events.append('cleanup')

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup'])

    def test_exception_in_tear_down(self):
        events = []

        class Test(unittest.IsolatedAsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')

            async def test_func(self):
                events.append('test')
                self.addAsyncCleanup(self.on_cleanup)

            async def asyncTearDown(self):
                events.append('asyncTearDown')
                raise Exception()

            async def on_cleanup(self):
                events.append('cleanup')

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup'])


    def test_exception_in_tear_clean_up(self):
        events = []

        class Test(unittest.IsolatedAsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')

            async def test_func(self):
                events.append('test')
                self.addAsyncCleanup(self.on_cleanup)

            async def asyncTearDown(self):
                events.append('asyncTearDown')

            async def on_cleanup(self):
                events.append('cleanup')
                raise Exception()

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup'])

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




if __name__ == "__main__":
    unittest.main()
