import unittest, sys, gc
from ctypes import *
from ctypes import _pointer_type_cache

class LeakTestCase(unittest.TestCase):

    ################

    def make_noncyclic_structures(self, repeat):
        for i in xrange(repeat):
            class POINT(Structure):
                _fields_ = [("x", c_int), ("y", c_int)]
            class RECT(Structure):
                _fields_ = [("ul", POINT),
                            ("br", POINT)]

    if hasattr(sys, "gettotalrefcount"):

        def test_no_cycles_refcount(self):
            last_refcount = 0
            for x in xrange(20):
                self.make_noncyclic_structures(1000)
                while gc.collect():
                    pass
                total_refcount = sys.gettotalrefcount()
                if last_refcount >= total_refcount:
                    return # test passed
                last_refcount = total_refcount
            self.fail("leaking refcounts")

    def test_no_cycles_objcount(self):
        # not correct - gc.get_objects() returns only thos objects
        # that the garbage collector tracks.  Correct would be to use
        # sys.getobjects(), but this is only available in debug build.
        last_objcount = 0
        for x in xrange(20):
            self.make_noncyclic_structures(1000)
            while gc.collect():
                pass
            total_objcount = gc.get_objects()
            if last_objcount >= total_objcount:
                return # test passed
            last_objcount = total_objcount
        self.fail("leaking objects")

    ################

    def make_cyclic_structures(self, repeat):
        for i in xrange(repeat):
            PLIST = POINTER("LIST")
            class LIST(Structure):
                _fields_ = [("pnext", PLIST)]
            SetPointerType(PLIST, LIST)
            del _pointer_type_cache[LIST] # XXX should this be a weakkeydict?

    if hasattr(sys, "gettotalrefcount"):

        def test_cycles_refcount(self):
            last_refcount = 0
            for x in xrange(5):
                self.make_cyclic_structures(1000)
                while gc.collect():
                    pass
                total_refcount = sys.gettotalrefcount()
                if last_refcount >= total_refcount:
                    return
                last_refcount = total_refcount
            self.fail("leaking refcounts")

    else:

        def test_cycles_objcount(self):
            # not correct - gc.get_objects() returns only thos objects
            # that the garbage collector tracks.  Correct would be to use
            # sys.getobjects(), but this is only available in debug build.
            last_objcount = 0
            for x in xrange(8):
                self.make_cyclic_structures(1000)
                while gc.collect():
                    pass
                total_objcount = len(gc.get_objects())
                if last_objcount >= total_objcount:
                    return
                last_objcount = total_objcount
            self.fail("leaking objects")

if __name__ == "__main__":
    unittest.main()
