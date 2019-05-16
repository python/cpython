import asyncio
import unittest


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class TestAsyncCase(unittest.TestCase):
    def test_basic(self):
        calls = 0

        class Test(unittest.AsyncioTestCase):
            async def setUp(self):
                nonlocal calls
                self.assertEqual(calls, 0)
                calls = 1

            async def test_func(self):
                nonlocal calls
                self.assertEqual(calls, 1)
                calls = 2
                self.addCleanup(self.on_cleanup)

            async def tearDown(self):
                nonlocal calls
                self.assertEqual(calls, 2)
                calls = 3

            async def on_cleanup(self):
                nonlocal calls
                self.assertEqual(calls, 3)
                calls = 4

        test = Test("test_func")
        test.run()
        self.assertEqual(calls, 4)


if __name__ == "__main__":
    unittest.main()
