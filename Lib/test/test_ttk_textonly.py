import os
from test import test_support

# Skip this test if _tkinter does not exist.
test_support.import_module('_tkinter')

this_dir = os.path.dirname(os.path.abspath(__file__))
lib_tk_test = os.path.abspath(os.path.join(this_dir, '..', 'lib-tk', 'test'))

with test_support.DirsOnSysPath(lib_tk_test):
    import runtktests

def test_main():
    with test_support.DirsOnSysPath(lib_tk_test):
        test_support.run_unittest(
            *runtktests.get_tests(gui=False, packages=['test_ttk']))

if __name__ == '__main__':
    test_main()
