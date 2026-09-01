import unittest

from test.support import import_helper
from test.support import threading_helper

readline = import_helper.import_module("readline")


@threading_helper.requires_working_threading()
class TestReadlineRaces(unittest.TestCase):
    def test_completer_delims_get_set(self):
        def worker():
            for _ in range(100):
                readline.get_completer_delims()
                readline.set_completer_delims(
                    ' \t\n`@#%^&*()=+[{]}\\|;:\'",<>?')
                readline.set_completer_delims(
                    ' \t\n`@#%^&*()=+[{]}\\|;:\'",<>?')
                readline.get_completer_delims()

        threading_helper.run_concurrently(worker, nthreads=40)

    # get_completer()/get_pre_input_hook() must take the module critical
    # section like their setters do; otherwise reading and Py_NewRef-ing the
    # stored hook races the setter replacing it (gh-153291).

    def test_completer_get_set(self):
        def setter():
            for _ in range(1000):
                readline.set_completer(lambda text, state: None)
                readline.set_completer(None)

        def getter():
            for _ in range(1000):
                readline.get_completer()

        original = readline.get_completer()
        self.addCleanup(readline.set_completer, original)
        threading_helper.run_concurrently([setter] * 2 + [getter] * 6)

    @unittest.skipUnless(hasattr(readline, "set_pre_input_hook"),
                         "needs readline.set_pre_input_hook")
    def test_pre_input_hook_get_set(self):
        def setter():
            for _ in range(1000):
                readline.set_pre_input_hook(lambda: None)
                readline.set_pre_input_hook(None)

        def getter():
            for _ in range(1000):
                readline.get_pre_input_hook()

        original = readline.get_pre_input_hook()
        self.addCleanup(readline.set_pre_input_hook, original)
        threading_helper.run_concurrently([setter] * 2 + [getter] * 6)


if __name__ == "__main__":
    unittest.main()
