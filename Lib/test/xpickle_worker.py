# This script is called by test_xpickle as a subprocess to load and dump
# pickles in a different Python version.
import importlib.util
import os
import pickle
import sys


# This allows the xpickle worker to import pickletester.py, which it needs
# since some of the pickle objects hold references to pickletester.py.
# Also provides the test library over the platform's native one since
# pickletester requires some test.support functions (such as os_helper)
# which are not available in versions below Python 3.10.
test_mod_path = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                             'picklecommon.py'))
spec = importlib.util.spec_from_file_location('test.picklecommon', test_mod_path)
test_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(test_module)
sys.modules['test.picklecommon'] = test_module


# To unpickle certain objects, the structure of the class needs to be known.
# These classes are mostly copies from pickletester.py.

class Nested:
    class A:
        class B:
            class C:
                pass
class C:
    pass

class D(C):
    pass

class E(C):
    pass

class H(object):
    pass

class K(object):
    pass

class Subclass(tuple):
    class Nested(str):
        pass

class PyMethodsTest:
    @staticmethod
    def cheese():
        pass

    @classmethod
    def wine(cls):
        pass

    def biscuits(self):
        pass

    class Nested:
        "Nested class"
        @staticmethod
        def ketchup():
            pass
        @classmethod
        def maple(cls):
            pass
        def pie(self):
            pass


class Recursive:
    pass


in_stream = sys.stdin.buffer
out_stream = sys.stdout.buffer

try:
    message = pickle.load(in_stream)
    protocol, obj = message
    pickle.dump(obj, out_stream, protocol)
except Exception as e:
    # dump the exception to stdout and write to stderr, then exit
    pickle.dump(e, out_stream)
    sys.stderr.write(repr(e))
    sys.exit(1)
