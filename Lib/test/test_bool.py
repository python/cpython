# Test properties of bool promised by PEP 285

import unittest
from test import test_support

import os

class BoolTest(unittest.TestCase):

    def assertIs(self, a, b):
        self.assert_(a is b)

    def assertIsNot(self, a, b):
        self.assert_(a is not b)

    def test_subclass(self):
        try:
            class C(bool):
                pass
        except TypeError:
            pass
        else:
            self.fail("bool should not be subclassable")

        self.assertRaises(TypeError, int.__new__, bool, 0)

    def test_print(self):
        try:
            fo = open(test_support.TESTFN, "wb")
            print >> fo, False, True
            fo.close()
            fo = open(test_support.TESTFN, "rb")
            self.assertEqual(fo.read(), 'False True\n')
        finally:
            fo.close()
            os.remove(test_support.TESTFN)

    def test_repr(self):
        self.assertEqual(repr(False), 'False')
        self.assertEqual(repr(True), 'True')
        self.assertEqual(eval(repr(False)), False)
        self.assertEqual(eval(repr(True)), True)

    def test_str(self):
        self.assertEqual(str(False), 'False')
        self.assertEqual(str(True), 'True')

    def test_int(self):
        self.assertEqual(int(False), 0)
        self.assertIsNot(int(False), False)
        self.assertEqual(int(True), 1)
        self.assertIsNot(int(True), True)

    def test_math(self):
        self.assertEqual(+False, 0)
        self.assertIsNot(+False, False)
        self.assertEqual(-False, 0)
        self.assertIsNot(-False, False)
        self.assertEqual(abs(False), 0)
        self.assertIsNot(abs(False), False)
        self.assertEqual(+True, 1)
        self.assertIsNot(+True, True)
        self.assertEqual(-True, -1)
        self.assertEqual(abs(True), 1)
        self.assertIsNot(abs(True), True)
        self.assertEqual(~False, -1)
        self.assertEqual(~True, -2)

        self.assertEqual(False+2, 2)
        self.assertEqual(True+2, 3)
        self.assertEqual(2+False, 2)
        self.assertEqual(2+True, 3)

        self.assertEqual(False+False, 0)
        self.assertIsNot(False+False, False)
        self.assertEqual(False+True, 1)
        self.assertIsNot(False+True, True)
        self.assertEqual(True+False, 1)
        self.assertIsNot(True+False, True)
        self.assertEqual(True+True, 2)

        self.assertEqual(True-True, 0)
        self.assertIsNot(True-True, False)
        self.assertEqual(False-False, 0)
        self.assertIsNot(False-False, False)
        self.assertEqual(True-False, 1)
        self.assertIsNot(True-False, True)
        self.assertEqual(False-True, -1)

        self.assertEqual(True*1, 1)
        self.assertEqual(False*1, 0)
        self.assertIsNot(False*1, False)

        self.assertEqual(True//1, 1)
        self.assertIsNot(True//1, True)
        self.assertEqual(False//1, 0)
        self.assertIsNot(False//1, False)

        for b in False, True:
            for i in 0, 1, 2:
                self.assertEqual(b**i, int(b)**i)
                self.assertIsNot(b**i, bool(int(b)**i))

        for a in False, True:
            for b in False, True:
                self.assertIs(a&b, bool(int(a)&int(b)))
                self.assertIs(a|b, bool(int(a)|int(b)))
                self.assertIs(a^b, bool(int(a)^int(b)))
                self.assertEqual(a&int(b), int(a)&int(b))
                self.assertIsNot(a&int(b), bool(int(a)&int(b)))
                self.assertEqual(a|int(b), int(a)|int(b))
                self.assertIsNot(a|int(b), bool(int(a)|int(b)))
                self.assertEqual(a^int(b), int(a)^int(b))
                self.assertIsNot(a^int(b), bool(int(a)^int(b)))
                self.assertEqual(int(a)&b, int(a)&int(b))
                self.assertIsNot(int(a)&b, bool(int(a)&int(b)))
                self.assertEqual(int(a)|b, int(a)|int(b))
                self.assertIsNot(int(a)|b, bool(int(a)|int(b)))
                self.assertEqual(int(a)^b, int(a)^int(b))
                self.assertIsNot(int(a)^b, bool(int(a)^int(b)))

        self.assertIs(1==1, True)
        self.assertIs(1==0, False)
        self.assertIs(0<1, True)
        self.assertIs(1<0, False)
        self.assertIs(0<=0, True)
        self.assertIs(1<=0, False)
        self.assertIs(1>0, True)
        self.assertIs(1>1, False)
        self.assertIs(1>=1, True)
        self.assertIs(0>=1, False)
        self.assertIs(0!=1, True)
        self.assertIs(0!=0, False)

        x = [1]
        self.assertIs(x is x, True)
        self.assertIs(x is not x, False)

        self.assertIs(1 in x, True)
        self.assertIs(0 in x, False)
        self.assertIs(1 not in x, False)
        self.assertIs(0 not in x, True)

        x = {1: 2}
        self.assertIs(x is x, True)
        self.assertIs(x is not x, False)

        self.assertIs(1 in x, True)
        self.assertIs(0 in x, False)
        self.assertIs(1 not in x, False)
        self.assertIs(0 not in x, True)

        self.assertIs(not True, False)
        self.assertIs(not False, True)

    def test_convert(self):
        self.assertRaises(TypeError, bool, 42, 42)
        self.assertIs(bool(10), True)
        self.assertIs(bool(1), True)
        self.assertIs(bool(-1), True)
        self.assertIs(bool(0), False)
        self.assertIs(bool("hello"), True)
        self.assertIs(bool(""), False)
        self.assertIs(bool(), False)

    def test_hasattr(self):
        self.assertIs(hasattr([], "append"), True)
        self.assertIs(hasattr([], "wobble"), False)

    def test_callable(self):
        with test_support._check_py3k_warnings():
            self.assertIs(callable(len), True)
            self.assertIs(callable(1), False)

    def test_isinstance(self):
        self.assertIs(isinstance(True, bool), True)
        self.assertIs(isinstance(False, bool), True)
        self.assertIs(isinstance(True, int), True)
        self.assertIs(isinstance(False, int), True)
        self.assertIs(isinstance(1, bool), False)
        self.assertIs(isinstance(0, bool), False)

    def test_issubclass(self):
        self.assertIs(issubclass(bool, int), True)
        self.assertIs(issubclass(int, bool), False)

    def test_haskey(self):
        self.assertIs(1 in {}, False)
        self.assertIs(1 in {1:1}, True)
        with test_support._check_py3k_warnings():
            self.assertIs({}.has_key(1), False)
            self.assertIs({1:1}.has_key(1), True)

    def test_string(self):
        self.assertIs("xyz".endswith("z"), True)
        self.assertIs("xyz".endswith("x"), False)
        self.assertIs("xyz0123".isalnum(), True)
        self.assertIs("@#$%".isalnum(), False)
        self.assertIs("xyz".isalpha(), True)
        self.assertIs("@#$%".isalpha(), False)
        self.assertIs("0123".isdigit(), True)
        self.assertIs("xyz".isdigit(), False)
        self.assertIs("xyz".islower(), True)
        self.assertIs("XYZ".islower(), False)
        self.assertIs(" ".isspace(), True)
        self.assertIs("XYZ".isspace(), False)
        self.assertIs("X".istitle(), True)
        self.assertIs("x".istitle(), False)
        self.assertIs("XYZ".isupper(), True)
        self.assertIs("xyz".isupper(), False)
        self.assertIs("xyz".startswith("x"), True)
        self.assertIs("xyz".startswith("z"), False)

        if test_support.have_unicode:
            self.assertIs(unicode("xyz", 'ascii').endswith(unicode("z", 'ascii')), True)
            self.assertIs(unicode("xyz", 'ascii').endswith(unicode("x", 'ascii')), False)
            self.assertIs(unicode("xyz0123", 'ascii').isalnum(), True)
            self.assertIs(unicode("@#$%", 'ascii').isalnum(), False)
            self.assertIs(unicode("xyz", 'ascii').isalpha(), True)
            self.assertIs(unicode("@#$%", 'ascii').isalpha(), False)
            self.assertIs(unicode("0123", 'ascii').isdecimal(), True)
            self.assertIs(unicode("xyz", 'ascii').isdecimal(), False)
            self.assertIs(unicode("0123", 'ascii').isdigit(), True)
            self.assertIs(unicode("xyz", 'ascii').isdigit(), False)
            self.assertIs(unicode("xyz", 'ascii').islower(), True)
            self.assertIs(unicode("XYZ", 'ascii').islower(), False)
            self.assertIs(unicode("0123", 'ascii').isnumeric(), True)
            self.assertIs(unicode("xyz", 'ascii').isnumeric(), False)
            self.assertIs(unicode(" ", 'ascii').isspace(), True)
            self.assertIs(unicode("XYZ", 'ascii').isspace(), False)
            self.assertIs(unicode("X", 'ascii').istitle(), True)
            self.assertIs(unicode("x", 'ascii').istitle(), False)
            self.assertIs(unicode("XYZ", 'ascii').isupper(), True)
            self.assertIs(unicode("xyz", 'ascii').isupper(), False)
            self.assertIs(unicode("xyz", 'ascii').startswith(unicode("x", 'ascii')), True)
            self.assertIs(unicode("xyz", 'ascii').startswith(unicode("z", 'ascii')), False)

    def test_boolean(self):
        self.assertEqual(True & 1, 1)
        self.assert_(not isinstance(True & 1, bool))
        self.assertIs(True & True, True)

        self.assertEqual(True | 1, 1)
        self.assert_(not isinstance(True | 1, bool))
        self.assertIs(True | True, True)

        self.assertEqual(True ^ 1, 0)
        self.assert_(not isinstance(True ^ 1, bool))
        self.assertIs(True ^ True, False)

    def test_fileclosed(self):
        try:
            f = file(test_support.TESTFN, "w")
            self.assertIs(f.closed, False)
            f.close()
            self.assertIs(f.closed, True)
        finally:
            os.remove(test_support.TESTFN)

    def test_operator(self):
        import operator
        self.assertIs(operator.truth(0), False)
        self.assertIs(operator.truth(1), True)
        self.assertIs(operator.isCallable(0), False)
        self.assertIs(operator.isCallable(len), True)
        self.assertIs(operator.isNumberType(None), False)
        self.assertIs(operator.isNumberType(0), True)
        self.assertIs(operator.not_(1), False)
        self.assertIs(operator.not_(0), True)
        self.assertIs(operator.isSequenceType(0), False)
        self.assertIs(operator.isSequenceType([]), True)
        self.assertIs(operator.contains([], 1), False)
        self.assertIs(operator.contains([1], 1), True)
        self.assertIs(operator.isMappingType(1), False)
        self.assertIs(operator.isMappingType({}), True)
        self.assertIs(operator.lt(0, 0), False)
        self.assertIs(operator.lt(0, 1), True)
        self.assertIs(operator.is_(True, True), True)
        self.assertIs(operator.is_(True, False), False)
        self.assertIs(operator.is_not(True, True), False)
        self.assertIs(operator.is_not(True, False), True)

    def test_marshal(self):
        import marshal
        self.assertIs(marshal.loads(marshal.dumps(True)), True)
        self.assertIs(marshal.loads(marshal.dumps(False)), False)

    def test_pickle(self):
        import pickle
        self.assertIs(pickle.loads(pickle.dumps(True)), True)
        self.assertIs(pickle.loads(pickle.dumps(False)), False)
        self.assertIs(pickle.loads(pickle.dumps(True, True)), True)
        self.assertIs(pickle.loads(pickle.dumps(False, True)), False)

    def test_cpickle(self):
        import cPickle
        self.assertIs(cPickle.loads(cPickle.dumps(True)), True)
        self.assertIs(cPickle.loads(cPickle.dumps(False)), False)
        self.assertIs(cPickle.loads(cPickle.dumps(True, True)), True)
        self.assertIs(cPickle.loads(cPickle.dumps(False, True)), False)

    def test_mixedpickle(self):
        import pickle, cPickle
        self.assertIs(pickle.loads(cPickle.dumps(True)), True)
        self.assertIs(pickle.loads(cPickle.dumps(False)), False)
        self.assertIs(pickle.loads(cPickle.dumps(True, True)), True)
        self.assertIs(pickle.loads(cPickle.dumps(False, True)), False)

        self.assertIs(cPickle.loads(pickle.dumps(True)), True)
        self.assertIs(cPickle.loads(pickle.dumps(False)), False)
        self.assertIs(cPickle.loads(pickle.dumps(True, True)), True)
        self.assertIs(cPickle.loads(pickle.dumps(False, True)), False)

    def test_picklevalues(self):
        import pickle, cPickle

        # Test for specific backwards-compatible pickle values
        self.assertEqual(pickle.dumps(True), "I01\n.")
        self.assertEqual(pickle.dumps(False), "I00\n.")
        self.assertEqual(cPickle.dumps(True), "I01\n.")
        self.assertEqual(cPickle.dumps(False), "I00\n.")
        self.assertEqual(pickle.dumps(True, True), "I01\n.")
        self.assertEqual(pickle.dumps(False, True), "I00\n.")
        self.assertEqual(cPickle.dumps(True, True), "I01\n.")
        self.assertEqual(cPickle.dumps(False, True), "I00\n.")

    def test_convert_to_bool(self):
        # Verify that TypeError occurs when bad things are returned
        # from __nonzero__().  This isn't really a bool test, but
        # it's related.
        check = lambda o: self.assertRaises(TypeError, bool, o)
        class Foo(object):
            def __nonzero__(self):
                return self
        check(Foo())

        class Bar(object):
            def __nonzero__(self):
                return "Yes"
        check(Bar())

        class Baz(int):
            def __nonzero__(self):
                return self
        check(Baz())


def test_main():
    test_support.run_unittest(BoolTest)

if __name__ == "__main__":
    test_main()
