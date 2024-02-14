import unittest

from test.support.import_helper import import_module
from test.test_idle import load_tests

tk = import_module('tkinter')  # Also imports _tkinter.

tk.NoDefaultRoot()
unittest.main(exit=False)
tk._support_default_root = True
tk._default_root = None
