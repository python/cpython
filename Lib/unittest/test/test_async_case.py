import asyncio
import unittest


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class TestAsyncCase(unittest.TestCase):
    def test_basic(self):
        events = []

        class Test(unittest.AsyncioTestCase):
            async def asyncSetUp(self):
                self.assertEqual(events, [])
                events.append('asyncSetUp')

            async def test_func(self):
                self.assertEqual(events, ['asyncSetUp'])
                events.append('test')
                self.addCleanup(self.on_cleanup)

            async def asyncTearDown(self):
                self.assertEqual(events, ['asyncSetUp', 'test'])
                events.append('asyncTearDown')

            async def on_cleanup(self):
                self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown'])
                events.append('cleanup')

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup'])


    def test_exception_in_setup(self):
        events = []

        class Test(unittest.AsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')
                raise Exception()

            async def test_func(self):
                events.append('test')
                self.addCleanup(self.on_cleanup)

            async def asyncTearDown(self):
                events.append('asyncTearDown')

            async def on_cleanup(self):
                events.append('cleanup')


        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['asyncSetUp'])


    def test_exception_in_test(self):
        events = []

        class Test(unittest.AsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')

            async def test_func(self):
                events.append('test')
                raise Exception()
                self.addCleanup(self.on_cleanup)

            async def asyncTearDown(self):
                events.append('asyncTearDown')

            async def on_cleanup(self):
                events.append('cleanup')

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown'])

    def test_exception_in_test_after_adding_cleanup(self):
        events = []

        class Test(unittest.AsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')

            async def test_func(self):
                events.append('test')
                self.addCleanup(self.on_cleanup)
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

        class Test(unittest.AsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')

            async def test_func(self):
                events.append('test')
                self.addCleanup(self.on_cleanup)

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

        class Test(unittest.AsyncioTestCase):
            async def asyncSetUp(self):
                events.append('asyncSetUp')

            async def test_func(self):
                events.append('test')
                self.addCleanup(self.on_cleanup)

            async def asyncTearDown(self):
                events.append('asyncTearDown')

            async def on_cleanup(self):
                events.append('cleanup')
                raise Exception()

        test = Test("test_func")
        test.run()
        self.assertEqual(events, ['asyncSetUp', 'test', 'asyncTearDown', 'cleanup'])


if __name__ == "__main__":
    unittest.main()
