import sys
import unittest

from test.support.import_helper import import_fresh_module

TESTS = 'test.datetimetester'


def load_tests(loader, tests, pattern):
    python_impl_tests = import_fresh_module(TESTS,
                                            fresh=['datetime', '_pydatetime', '_strptime'],
                                            blocked=['_datetime'])
    c_impl_tests = import_fresh_module(TESTS, fresh=['datetime', '_strptime'],
                                       blocked=['_pydatetime'])

    test_module_map = {
        '_Pure': python_impl_tests,
        '_Fast': c_impl_tests,
    }

    for suffix, module in test_module_map.items():
        test_classes = []

        for cls in module.__dict__.values():
            if not isinstance(cls, type):
                continue
            if issubclass(cls, unittest.TestCase):
                test_classes.append(cls)
            elif issubclass(cls, unittest.TestSuite):
                suit = cls()
                test_classes.extend(type(test) for test in suit)

        for cls in test_classes:
            cls.__name__ += suffix
            cls.__qualname__ += suffix

            @classmethod
            def setUpClass(cls_, module=module):
                cls_._save_sys_modules = sys.modules.copy()
                sys.modules[TESTS] = module  # for pickle tests
                sys.modules['datetime'] = module.datetime_module
                if hasattr(module, '_pydatetime'):
                    sys.modules['_pydatetime'] = module._pydatetime
                sys.modules['_strptime'] = module._strptime

            @classmethod
            def tearDownClass(cls_):
                sys.modules.clear()
                sys.modules.update(cls_._save_sys_modules)

            cls.setUpClass = setUpClass
            cls.tearDownClass = tearDownClass
            tests.addTests(loader.loadTestsFromTestCase(cls))

    return tests


if __name__ == "__main__":
    unittest.main()
