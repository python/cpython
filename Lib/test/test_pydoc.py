from test import test_support
import unittest
import pydoc

class TestDescriptions(unittest.TestCase):
    def test_module(self):
        # Check that pydocfodder module can be described
        from test import pydocfodder
        doc = pydoc.render_doc(pydocfodder)
        assert "pydocfodder" in doc

    def test_class(self):
        class C(object): "New-style class"
        c = C()

        self.failUnlessEqual(pydoc.describe(C), 'class C')
        self.failUnlessEqual(pydoc.describe(c), 'C')
        self.failUnless('C in module test.test_pydoc object'
                        in pydoc.render_doc(c))

def test_main():
    test_support.run_unittest(TestDescriptions)

if __name__ == "__main__":
    unittest.main()
