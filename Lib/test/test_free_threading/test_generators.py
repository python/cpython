import unittest

from unittest import TestCase

from test.support import threading_helper

@threading_helper.requires_working_threading()
class TestGen(TestCase):
    def test_generator_send(self):
        import threading

        def gen():
            for _ in range(5000):
                yield


        it = gen()
        with threading_helper.catch_threading_exception() as cm:
            # next(it) is equivalent to it.send(None)
            with threading_helper.start_threads(
                (threading.Thread(target=lambda: next(it)) for _ in range(10))
            ):
                pass

            self.assertIsNone(cm.exc_value)


if __name__ == "__main__":
    unittest.main()
