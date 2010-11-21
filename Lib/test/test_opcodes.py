# Python test set -- part 2, opcodes

from test.test_support import run_unittest, check_py3k_warnings
import unittest

class OpcodeTest(unittest.TestCase):

    def test_try_inside_for_loop(self):
        n = 0
        for i in range(10):
            n = n+i
            try: 1 // 0
            except NameError: pass
            except ZeroDivisionError: pass
            except TypeError: pass
            try: pass
            except: pass
            try: pass
            finally: pass
            n = n+i
        if n != 90:
            self.fail('try inside for')

    def test_raise_class_exceptions(self):

        class AClass: pass
        class BClass(AClass): pass
        class CClass: pass
        class DClass(AClass):
            def __init__(self, ignore):
                pass

        try: raise AClass()
        except: pass

        try: raise AClass()
        except AClass: pass

        try: raise BClass()
        except AClass: pass

        try: raise BClass()
        except CClass: self.fail()
        except: pass

        a = AClass()
        b = BClass()

        try: raise AClass, b
        except BClass, v:
            self.assertEqual(v, b)
        else: self.fail("no exception")

        try: raise b
        except AClass, v:
            self.assertEqual(v, b)
        else:
            self.fail("no exception")

        # not enough arguments
        try:  raise BClass, a
        except TypeError: pass
        else: self.fail("no exception")

        try:  raise DClass, a
        except DClass, v:
            self.assertIsInstance(v, DClass)
        else:
            self.fail("no exception")

    def test_compare_function_objects(self):

        f = eval('lambda: None')
        g = eval('lambda: None')
        self.assertNotEqual(f, g)

        f = eval('lambda a: a')
        g = eval('lambda a: a')
        self.assertNotEqual(f, g)

        f = eval('lambda a=1: a')
        g = eval('lambda a=1: a')
        self.assertNotEqual(f, g)

        f = eval('lambda: 0')
        g = eval('lambda: 1')
        self.assertNotEqual(f, g)

        f = eval('lambda: None')
        g = eval('lambda a: None')
        self.assertNotEqual(f, g)

        f = eval('lambda a: None')
        g = eval('lambda b: None')
        self.assertNotEqual(f, g)

        f = eval('lambda a: None')
        g = eval('lambda a=None: None')
        self.assertNotEqual(f, g)

        f = eval('lambda a=0: None')
        g = eval('lambda a=1: None')
        self.assertNotEqual(f, g)

    def test_modulo_of_string_subclasses(self):
        class MyString(str):
            def __mod__(self, value):
                return 42
        self.assertEqual(MyString() % 3, 42)


def test_main():
    with check_py3k_warnings(("exceptions must derive from BaseException",
                              DeprecationWarning),
                             ("catching classes that don't inherit "
                              "from BaseException is not allowed",
                              DeprecationWarning)):
        run_unittest(OpcodeTest)

if __name__ == '__main__':
    test_main()
