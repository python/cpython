import time
import unittest
import concurrent.futures

from test.support import threading_helper
from unittest.mock import patch, ThreadingMock


threading_helper.requires_working_threading(module=True)

VERY_SHORT_TIMEOUT = 0.1


class Something:
    def method_1(self):
        pass  # pragma: no cover

    def method_2(self):
        pass  # pragma: no cover


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
        waitable_mock.wait_until_any_call_with("works")

    def test_patch(self):
        waitable_mock = self._make_mock(spec=Something)

        with patch(f"{__name__}.Something", waitable_mock):
            something = Something()
            something.method_1()
            something.method_1.wait_until_called()

    def test_wait_already_called_success(self):
        waitable_mock = self._make_mock(spec=Something)
        waitable_mock.method_1()
        waitable_mock.method_1.wait_until_called()
        waitable_mock.method_1.wait_until_any_call_with()
        waitable_mock.method_1.assert_called()

    def test_wait_until_called_success(self):
        waitable_mock = self._make_mock(spec=Something)
        self.run_async(waitable_mock.method_1, delay=VERY_SHORT_TIMEOUT)
        waitable_mock.method_1.wait_until_called()

    def test_wait_until_called_method_timeout(self):
        waitable_mock = self._make_mock(spec=Something)
        with self.assertRaises(AssertionError):
            waitable_mock.method_1.wait_until_called(timeout=VERY_SHORT_TIMEOUT)

    def test_wait_until_called_instance_timeout(self):
        waitable_mock = self._make_mock(spec=Something, timeout=VERY_SHORT_TIMEOUT)
        with self.assertRaises(AssertionError):
            waitable_mock.method_1.wait_until_called()

    def test_wait_until_called_global_timeout(self):
        with patch.object(ThreadingMock, "DEFAULT_TIMEOUT"):
            ThreadingMock.DEFAULT_TIMEOUT = VERY_SHORT_TIMEOUT
            waitable_mock = self._make_mock(spec=Something)
            with self.assertRaises(AssertionError):
                waitable_mock.method_1.wait_until_called()

    def test_wait_until_any_call_with_success(self):
        waitable_mock = self._make_mock()
        self.run_async(waitable_mock, delay=VERY_SHORT_TIMEOUT)
        waitable_mock.wait_until_any_call_with()

    def test_wait_until_any_call_with_instance_timeout(self):
        waitable_mock = self._make_mock(timeout=VERY_SHORT_TIMEOUT)
        with self.assertRaises(AssertionError):
            waitable_mock.wait_until_any_call_with()

    def test_wait_until_any_call_global_timeout(self):
        with patch.object(ThreadingMock, "DEFAULT_TIMEOUT"):
            ThreadingMock.DEFAULT_TIMEOUT = VERY_SHORT_TIMEOUT
            waitable_mock = self._make_mock()
            with self.assertRaises(AssertionError):
                waitable_mock.wait_until_any_call_with()

    def test_wait_until_any_call_positional(self):
        waitable_mock = self._make_mock(timeout=VERY_SHORT_TIMEOUT)
        waitable_mock.method_1(1, 2, 3)
        waitable_mock.method_1.wait_until_any_call_with(1, 2, 3)
        with self.assertRaises(AssertionError):
            waitable_mock.method_1.wait_until_any_call_with(2, 3, 1)
        with self.assertRaises(AssertionError):
            waitable_mock.method_1.wait_until_any_call_with()

    def test_wait_until_any_call_kw(self):
        waitable_mock = self._make_mock(timeout=VERY_SHORT_TIMEOUT)
        waitable_mock.method_1(a=1, b=2)
        waitable_mock.method_1.wait_until_any_call_with(a=1, b=2)
        with self.assertRaises(AssertionError):
            waitable_mock.method_1.wait_until_any_call_with(a=2, b=1)
        with self.assertRaises(AssertionError):
            waitable_mock.method_1.wait_until_any_call_with()

    def test_magic_methods_success(self):
        waitable_mock = self._make_mock()
        str(waitable_mock)
        waitable_mock.__str__.wait_until_called()
        waitable_mock.__str__.assert_called()

    def test_reset_mock_resets_wait(self):
        m = self._make_mock(timeout=VERY_SHORT_TIMEOUT)

        with self.assertRaises(AssertionError):
            m.wait_until_called()
        with self.assertRaises(AssertionError):
            m.wait_until_any_call_with()
        m()
        m.wait_until_called()
        m.wait_until_any_call_with()
        m.assert_called_once()

        m.reset_mock()

        with self.assertRaises(AssertionError):
            m.wait_until_called()
        with self.assertRaises(AssertionError):
            m.wait_until_any_call_with()
        m()
        m.wait_until_called()
        m.wait_until_any_call_with()
        m.assert_called_once()


if __name__ == "__main__":
    unittest.main()
