import time
import unittest
import concurrent.futures

from unittest.mock import patch, ThreadingMock, call


class Something:
    def method_1(self):
        pass

    def method_2(self):
        pass


class TestThreadingMock(unittest.TestCase):
    def _call_after_delay(self, func, /, *args, **kwargs):
        time.sleep(kwargs.pop("delay"))
        func(*args, **kwargs)

    def setUp(self):
        self._executor = concurrent.futures.ThreadPoolExecutor(max_workers=5)

    def tearDown(self):
        self._executor.shutdown()

    def run_async(self, func, /, *args, delay=0, **kwargs):
        self._executor.submit(
            self._call_after_delay, func, *args, **kwargs, delay=delay
        )

    def _make_mock(self, *args, **kwargs):
        return ThreadingMock(*args, **kwargs)

    def test_spec(self):
        waitable_mock = self._make_mock(spec=Something)

        with patch(f"{__name__}.Something", waitable_mock) as m:
            something = m()

            self.assertIsInstance(something.method_1, ThreadingMock)
            self.assertIsInstance(something.method_1().method_2(), ThreadingMock)

            with self.assertRaises(AttributeError):
                m.test

    def test_side_effect(self):
        waitable_mock = self._make_mock()

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()
            something.method_1.side_effect = [1]

            self.assertEqual(something.method_1(), 1)

    def test_instance_check(self):
        waitable_mock = self._make_mock()

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()

            self.assertIsInstance(something.method_1, ThreadingMock)
            self.assertIsInstance(something.method_1().method_2(), ThreadingMock)

    def test_dynamic_child_mocks_are_threading_mocks(self):
        waitable_mock = self._make_mock()
        self.assertIsInstance(waitable_mock.child, ThreadingMock)

    def test_dynamic_child_mocks_inherit_timeout(self):
        mock1 = self._make_mock()
        self.assertIs(mock1._mock_wait_timeout, None)
        mock2 = self._make_mock(timeout=2)
        self.assertEqual(mock2._mock_wait_timeout, 2)
        mock3 = self._make_mock(timeout=3)
        self.assertEqual(mock3._mock_wait_timeout, 3)

        self.assertIs(mock1.child._mock_wait_timeout, None)
        self.assertEqual(mock2.child._mock_wait_timeout, 2)
        self.assertEqual(mock3.child._mock_wait_timeout, 3)

        self.assertEqual(mock2.really().__mul__().complex._mock_wait_timeout, 2)

    def test_no_name_clash(self):
        waitable_mock = self._make_mock()
        waitable_mock._event = "myevent"
        waitable_mock.event = "myevent"
        waitable_mock.timeout = "mytimeout"
        waitable_mock("works")
        waitable_mock.wait_until_called()
        waitable_mock.wait_until_any_call("works")

    def test_wait_success(self):
        waitable_mock = self._make_mock(spec=Something)

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()
            self.run_async(something.method_1, delay=0.01)
            something.method_1.wait_until_called()
            something.method_1.wait_until_any_call()
            something.method_1.assert_called()

    def test_wait_success_with_instance_timeout(self):
        waitable_mock = self._make_mock(timeout=1)

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()
            self.run_async(something.method_1, delay=0.01)
            something.method_1.wait_until_called()
            something.method_1.wait_until_any_call()
            something.method_1.assert_called()

    def test_wait_failed_with_instance_timeout(self):
        waitable_mock = self._make_mock(timeout=0.01)

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()
            self.run_async(something.method_1, delay=0.5)
            self.assertRaises(AssertionError, waitable_mock.method_1.wait_until_called)
            self.assertRaises(
                AssertionError, waitable_mock.method_1.wait_until_any_call
            )

    def test_wait_success_with_timeout_override(self):
        waitable_mock = self._make_mock(timeout=0.01)

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()
            self.run_async(something.method_1, delay=0.05)
            something.method_1.wait_until_called(timeout=1)

    def test_wait_failed_with_timeout_override(self):
        waitable_mock = self._make_mock(timeout=1)

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()
            self.run_async(something.method_1, delay=0.1)
            with self.assertRaises(AssertionError):
                something.method_1.wait_until_called(timeout=0.05)
            with self.assertRaises(AssertionError):
                something.method_1.wait_until_any_call(timeout=0.05)

    def test_wait_success_called_before(self):
        waitable_mock = self._make_mock()

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()
            something.method_1()
            something.method_1.wait_until_called()
            something.method_1.wait_until_any_call()
            something.method_1.assert_called()

    def test_wait_magic_method(self):
        waitable_mock = self._make_mock()

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()
            self.run_async(something.method_1.__str__, delay=0.01)
            something.method_1.__str__.wait_until_called()
            something.method_1.__str__.assert_called()

    def test_wait_until_any_call_positional(self):
        waitable_mock = self._make_mock(spec=Something)

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()
            self.run_async(something.method_1, 1, delay=0.1)
            self.run_async(something.method_1, 2, delay=0.2)
            self.run_async(something.method_1, 3, delay=0.3)
            self.assertNotIn(call(1), something.method_1.mock_calls)

            something.method_1.wait_until_any_call(1)
            something.method_1.assert_called_with(1)
            self.assertNotIn(call(2), something.method_1.mock_calls)
            self.assertNotIn(call(3), something.method_1.mock_calls)

            something.method_1.wait_until_any_call(3)
            self.assertIn(call(2), something.method_1.mock_calls)
            something.method_1.wait_until_any_call(2)

    def test_wait_until_any_call_keywords(self):
        waitable_mock = self._make_mock(spec=Something)

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()
            self.run_async(something.method_1, a=1, delay=0.1)
            self.run_async(something.method_1, b=2, delay=0.2)
            self.run_async(something.method_1, c=3, delay=0.3)
            self.assertNotIn(call(a=1), something.method_1.mock_calls)

            something.method_1.wait_until_any_call(a=1)
            something.method_1.assert_called_with(a=1)
            self.assertNotIn(call(b=2), something.method_1.mock_calls)
            self.assertNotIn(call(c=3), something.method_1.mock_calls)

            something.method_1.wait_until_any_call(c=3)
            self.assertIn(call(b=2), something.method_1.mock_calls)
            something.method_1.wait_until_any_call(b=2)

    def test_wait_until_any_call_no_argument_fails_when_called_with_arg(self):
        waitable_mock = self._make_mock(timeout=0.01)

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()
            something.method_1(1)

            something.method_1.assert_called_with(1)
            with self.assertRaises(AssertionError):
                something.method_1.wait_until_any_call()

            something.method_1()
            something.method_1.wait_until_any_call()

    def test_wait_until_any_call_global_default(self):
        with patch.object(ThreadingMock, "DEFAULT_TIMEOUT"):
            ThreadingMock.DEFAULT_TIMEOUT = 0.01
            m = self._make_mock()
            with self.assertRaises(AssertionError):
                m.wait_until_any_call()
            with self.assertRaises(AssertionError):
                m.wait_until_called()

            m()
            m.wait_until_any_call()
        assert ThreadingMock.DEFAULT_TIMEOUT != 0.01

    def test_wait_until_any_call_change_global_and_override(self):
        with patch.object(ThreadingMock, "DEFAULT_TIMEOUT"):
            ThreadingMock.DEFAULT_TIMEOUT = 0.01

            m1 = self._make_mock()
            self.run_async(m1, delay=0.1)
            with self.assertRaises(AssertionError):
                m1.wait_until_called()

            m2 = self._make_mock(timeout=1)
            self.run_async(m2, delay=0.1)
            m2.wait_until_called()

            m3 = self._make_mock()
            self.run_async(m3, delay=0.1)
            m3.wait_until_called(timeout=1)

            m4 = self._make_mock()
            self.run_async(m4, delay=0.1)
            m4.wait_until_called(timeout=None)

            m5 = self._make_mock(timeout=None)
            self.run_async(m5, delay=0.1)
            m5.wait_until_called()

        assert ThreadingMock.DEFAULT_TIMEOUT != 0.01

    def test_reset_mock_resets_wait(self):
        m = self._make_mock(timeout=0.01)

        with self.assertRaises(AssertionError):
            m.wait_until_called()
        with self.assertRaises(AssertionError):
            m.wait_until_any_call()
        m()
        m.wait_until_called()
        m.wait_until_any_call()
        m.assert_called_once()

        m.reset_mock()

        with self.assertRaises(AssertionError):
            m.wait_until_called()
        with self.assertRaises(AssertionError):
            m.wait_until_any_call()
        m()
        m.wait_until_called()
        m.wait_until_any_call()
        m.assert_called_once()


if __name__ == "__main__":
    unittest.main()
