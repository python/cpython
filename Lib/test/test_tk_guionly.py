from test import support
from tkinter.test import runtktests

def test_main(enable_gui=False):
    if enable_gui:
        if support.use_resources is None:
            support.use_resources = ['gui']
        elif 'gui' not in support.use_resources:
            support.use_resources.append('gui')

    support.run_unittest(*runtktests.get_tests(text=False))

if __name__ == '__main__':
    test_main(enable_gui=True)
