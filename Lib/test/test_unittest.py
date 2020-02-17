import Lib.test.unittest_test as unittest_test
from test import support


def test_main():
    # used by regrtest
    support.run_unittest(unittest_test.suite())
    support.reap_children()

def load_tests(*_):
    # used by unittest
    return unittest_test.suite()

if __name__ == "__main__":
    test_main()
