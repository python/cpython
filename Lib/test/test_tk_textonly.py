from test import support
from tkinter.test import runtktests

def test_main():
    support.run_unittest(*runtktests.get_tests(gui=False))

if __name__ == '__main__':
    test_main()
