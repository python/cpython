import unittest
import sys
from test.support import import_fresh_module, run_unittest
TESTS = 'test.datetimetester'
# XXX: import_fresh_module() is supposed to leave sys.module cache untouched,
# XXX: but it does not, so we have to save and restore it ourselves.
save_sys_modules = sys.modules.copy()
try:
    pure_tests = import_fresh_module(TESTS, fresh=['datetime', '_strptime'],
                                     blocked=['_datetime'])
    fast_tests = import_fresh_module(TESTS, fresh=['datetime',
                                                   '_datetime', '_strptime'])
finally:
    sys.modules.clear()
    sys.modules.update(save_sys_modules)
test_modules = [pure_tests, fast_tests]
test_suffixes = ["_Pure", "_Fast"]

for module, suffix in zip(test_modules, test_suffixes):
    for name, cls in module.__dict__.items():
        if isinstance(cls, type) and issubclass(cls, unittest.TestCase):
            name += suffix
            cls.__name__ = name
            globals()[name] = cls
            def setUp(self, module=module, setup=cls.setUp):
                self._save_sys_modules = sys.modules.copy()
                sys.modules[TESTS] = module
                sys.modules['datetime'] = module.datetime_module
                sys.modules['_strptime'] = module._strptime
                setup(self)
            def tearDown(self, teardown=cls.tearDown):
                teardown(self)
                sys.modules.clear()
                sys.modules.update(self._save_sys_modules)
            cls.setUp = setUp
            cls.tearDown = tearDown

def test_main():
    run_unittest(__name__)

if __name__ == "__main__":
    test_main()
