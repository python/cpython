import asyncio
import unittest


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class TestAsyncCase(unittest.TestCase):
    def test_basic(self):
        events = []

        class Test(unittest.AsyncioTestCase):
            async def setUp(self):
                self.assertEqual(events, [])
                events.append('setUp')

            async def test_func(self):
                self.assertEqual(events, ['setUp'])
                events.append('test')
                self.addCleanup(self.on_cleanup)

            async def tearDown(self):
                self.assertEqual(events, ['setUp', 'test'])
                events.append('tearDown')

            async def on_cleanup(self):
                self.assertEqual(events, ['setUp', 'test', 'tearDown'])
                events.append('cleanup')

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['setUp', 'test', 'tearDown', 'cleanup'])


    def test_exception_in_setup(self):
        events = []

        class Test(unittest.AsyncioTestCase):
            async def setUp(self):
                events.append('setUp')
                raise Exception()

            async def test_func(self):
                events.append('test')
                self.addCleanup(self.on_cleanup)

            async def tearDown(self):
                events.append('tearDown')

            async def on_cleanup(self):
                events.append('cleanup')


        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['setUp'])


    def test_exception_in_test(self):
        events = []

        class Test(unittest.AsyncioTestCase):
            async def setUp(self):
                events.append('setUp')

            async def test_func(self):
                events.append('test')
                raise Exception()
                self.addCleanup(self.on_cleanup)

            async def tearDown(self):
                events.append('tearDown')

            async def on_cleanup(self):
                events.append('cleanup')

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['setUp', 'test', 'tearDown'])

    def test_exception_in_test_after_adding_cleanup(self):
        events = []

        class Test(unittest.AsyncioTestCase):
            async def setUp(self):
                events.append('setUp')

            async def test_func(self):
                events.append('test')
                self.addCleanup(self.on_cleanup)
                raise Exception()

            async def tearDown(self):
                events.append('tearDown')

            async def on_cleanup(self):
                events.append('cleanup')

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['setUp', 'test', 'tearDown', 'cleanup'])

    def test_exception_in_tear_down(self):
        events = []

        class Test(unittest.AsyncioTestCase):
            async def setUp(self):
                events.append('setUp')

            async def test_func(self):
                events.append('test')
                self.addCleanup(self.on_cleanup)

            async def tearDown(self):
                events.append('tearDown')
                raise Exception()

            async def on_cleanup(self):
                events.append('cleanup')

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['setUp', 'test', 'tearDown', 'cleanup'])


    def test_exception_in_tear_clean_up(self):
        events = []

        class Test(unittest.AsyncioTestCase):
            async def setUp(self):
                events.append('setUp')

            async def test_func(self):
                events.append('test')
                self.addCleanup(self.on_cleanup)

            async def tearDown(self):
                events.append('tearDown')

            async def on_cleanup(self):
                events.append('cleanup')
                raise Exception()

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['setUp', 'test', 'tearDown', 'cleanup'])


if __name__ == "__main__":
    unittest.main()
