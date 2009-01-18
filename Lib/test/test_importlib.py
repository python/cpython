from test.support import run_unittest
import importlib.test


def test_main():
    run_unittest(importlib.test.test_suite('importlib.test'))


if __name__ == '__main__':
    test_main()
