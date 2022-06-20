import os
import sys
import unittest


here = os.path.dirname(__file__)
loader = unittest.defaultTestLoader

def suite():
    suite = unittest.TestSuite()
    for fn in os.listdir(here):
        if fn.startswith("test") and fn.endswith(".py"):
            modname = "test.test_unittest." + fn[:-3]
            try:
                __import__(modname)
            except unittest.SkipTest:
                continue
            module = sys.modules[modname]
            suite.addTest(loader.loadTestsFromModule(module))
    suite.addTest(loader.loadTestsFromName('test.test_unittest.testmock'))
    return suite


if __name__ == "__main__":
    unittest.main(defaultTest="suite")
