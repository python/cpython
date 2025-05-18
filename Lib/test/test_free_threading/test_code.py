import unittest

from threading import Thread
from unittest import TestCase

from test.support import threading_helper

@threading_helper.requires_working_threading()
class TestCode(TestCase):
    def test_code_attrs(self):
        """Test concurrent accesses to lazily initialized code attributes"""
        code_objects = []
        for _ in range(1000):
            code_objects.append(compile("a + b", "<string>", "eval"))

        def run_in_thread():
            for code in code_objects:
                self.assertIsInstance(code.co_code, bytes)
                self.assertIsInstance(code.co_freevars, tuple)
                self.assertIsInstance(code.co_varnames, tuple)

        threads = [Thread(target=run_in_thread) for _ in range(2)]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()


if __name__ == "__main__":
    unittest.main()
