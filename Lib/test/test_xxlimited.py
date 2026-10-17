import unittest
from test.support import import_helper

xxlimited = import_helper.import_module('xxlimited')

# if import of xxlimited succeeded, the other ones should be importable.
import xxlimited_3_13
import xxlimited_35

MODULES = {
    (3, 15): xxlimited,
    (3, 13): xxlimited_3_13,
    (3, 5): xxlimited_35,
}

def test_with_xxlimited_modules(since=None, until=None):
    def _decorator(func):
        def _wrapper(self, *args, **kwargs):
            for version, module in MODULES.items():
                if since and version < since:
                    continue
                if until and version >= until:
                    continue
                with self.subTest(version=version):
                    func(self, module, *args, **kwargs)
        return _wrapper
    return _decorator

class XXLimitedTests(unittest.TestCase):
    @test_with_xxlimited_modules()
    def test_xxo_new(self, module):
        xxo = module.Xxo()

    @test_with_xxlimited_modules()
    def test_xxo_attributes(self, module):
        xxo = module.Xxo()
        with self.assertRaises(AttributeError):
            xxo.foo
        with self.assertRaises(AttributeError):
            del xxo.foo

        xxo.foo = 1234
        self.assertEqual(xxo.foo, 1234)

        del xxo.foo
        with self.assertRaises(AttributeError):
            xxo.foo

    @test_with_xxlimited_modules()
    def test_foo(self, module):
        # the foo function adds 2 numbers
        self.assertEqual(module.foo(1, 2), 3)

    @test_with_xxlimited_modules()
    def test_str(self, module):
        self.assertIsSubclass(module.Str, str)
        self.assertIsNot(module.Str, str)

        custom_string = module.Str("abcd")
        self.assertEqual(custom_string, "abcd")
        self.assertEqual(custom_string.upper(), "ABCD")

    @test_with_xxlimited_modules()
    def test_new(self, module):
        xxo = module.new()
        self.assertEqual(xxo.demo("abc"), "abc")

    @test_with_xxlimited_modules()
    def test_xxo_demo(self, module):
        xxo = module.Xxo()
        self.assertEqual(xxo.demo("abc"), "abc")
        self.assertEqual(xxo.demo(0), None)
        self.assertEqual(xxo.__module__, module.__name__)
        with self.assertRaises(TypeError):
            module.Xxo('arg')
        with self.assertRaises(TypeError):
            module.Xxo(kwarg='arg')

    @test_with_xxlimited_modules(since=(3, 13))
    def test_xxo_demo_extra(self, module):
        xxo = module.Xxo()
        other = module.Xxo()
        self.assertEqual(xxo.demo(xxo), xxo)
        self.assertEqual(xxo.demo(other), other)

    @test_with_xxlimited_modules(since=(3, 15))
    def test_xxo_subclass(self, module):
        class Sub(module.Xxo):
            pass
        sub = Sub()
        sub.a = 123
        self.assertEqual(sub.a, 123)
        with self.assertRaisesRegex(AttributeError, "cannot set 'reserved'"):
            sub.reserved = 123

    @test_with_xxlimited_modules(since=(3, 13))
    def test_error(self, module):
        with self.assertRaises(module.Error):
            raise module.Error

    @test_with_xxlimited_modules(since=(3, 13))
    def test_buffer(self, module):
        xxo = module.Xxo()
        self.assertEqual(xxo.x_exports, 0)
        b1 = memoryview(xxo)
        self.assertEqual(xxo.x_exports, 1)
        b2 = memoryview(xxo)
        self.assertEqual(xxo.x_exports, 2)
        b1[0] = 1
        self.assertEqual(b1[0], 1)
        self.assertEqual(b2[0], 1)

    @test_with_xxlimited_modules(until=(3, 5))
    def test_roj(self):
        # the roj function always fails
        with self.assertRaises(SystemError):
            self.module.roj(0)

    @test_with_xxlimited_modules(until=(3, 5))
    def test_null(self):
        null1 = self.module.Null()
        null2 = self.module.Null()
        self.assertNotEqual(null1, null2)
