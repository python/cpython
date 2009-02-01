import os.path
import sys
import unittest


def test_suite(package=__package__, directory=os.path.dirname(__file__)):
    suite = unittest.TestSuite()
    for name in os.listdir(directory):
        if name.startswith('.'):
            continue
        path = os.path.join(directory, name)
        if (os.path.isfile(path) and name.startswith('test_') and
                name.endswith('.py')):
            submodule_name = os.path.splitext(name)[0]
            module_name = "{0}.{1}".format(package, submodule_name)
            __import__(module_name, level=0)
            module_tests = unittest.findTestCases(sys.modules[module_name])
            suite.addTest(module_tests)
        elif os.path.isdir(path):
            package_name = "{0}.{1}".format(package, name)
            __import__(package_name, level=0)
            package_tests = getattr(sys.modules[package_name], 'test_suite')()
            suite.addTest(package_tests)
        else:
            continue
    return suite


if __name__ == '__main__':
    from test.support import run_unittest
    run_unittest(test_suite('importlib.test'))
