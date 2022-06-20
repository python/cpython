import os
import sys
import unittest


here = os.path.dirname(__file__)
loader = unittest.defaultTestLoader

def load_tests(*args):
    suite = unittest.TestSuite()
    for fn in os.listdir(here):
        if fn.startswith("test") and fn.endswith(".py"):
            modname = "unittest.test.testmock." + fn[:-3]
            __import__(modname)
            module = sys.modules[modname]
            suite.addTest(loader.loadTestsFromModule(module))
    return suite
