import unittest

class TestLoadAttrCache(unittest.TestCase):
    def test_descriptor_added_after_optimization(self):
        class Descriptor:
            pass

        class C:
            def __init__(self):
                self.x = 1
            x = Descriptor()

        def f(o):
            return o.x

        o = C()
        for i in range(1025):
            assert f(o) == 1

        Descriptor.__get__ = lambda self, instance, value: 2
        Descriptor.__set__ = lambda *args: None

        self.assertEqual(f(o), 2)

    def test_metaclass_descriptor_added_after_optimization(self):
        class Descriptor:
            pass

        class Metaclass(type):
            attribute = Descriptor()

        class Class(metaclass=Metaclass):
            attribute = True

        def f():
            return Class.attribute

        for _ in range(1025):
            self.assertTrue(f())

        Descriptor.__get__ = lambda self, instance, value: False
        Descriptor.__set__ = lambda *args: None

        self.assertFalse(f())

    def test_metaclass_descriptor_shadows_class_attribute(self):
        class Metaclass(type):
            @property
            def attribute(self):
                return True

        class Class(metaclass=Metaclass):
            attribute = False

        def f():
            return Class.attribute

        for _ in range(1025):
            self.assertTrue(f())

    def test_metaclass_set_descriptor_after_optimization(self):
        class Metaclass(type):
            pass

        class Class(metaclass=Metaclass):
            attribute = True

        @property
        def attribute(self):
            return False

        def f():
            return Class.attribute

        for _ in range(1025):
            self.assertTrue(f())

        Metaclass.attribute = attribute
        self.assertFalse(f())

    def test_metaclass_del_descriptor_after_optimization(self):
        class Metaclass(type):
            @property
            def attribute(self):
                return True

        class Class(metaclass=Metaclass):
            attribute = False

        def f():
            return Class.attribute

        for _ in range(1025):
            self.assertTrue(f())
        
        del Metaclass.attribute
        self.assertFalse(f())

    def test_type_descriptor_shadows_attribute(self):
        class Class:
            __name__ = "Spam"

        def f():
            return Class.__name__

        for _ in range(1025):
            self.assertEqual(f(), "Class")

    def test_type_descriptor_shadows_attribute_method(self):
        class Class:
            mro = None

        def f():
            return Class.mro

        for _ in range(1025):
            self.assertIsNone(f())

    def test_type_descriptor_shadows_attribute_member(self):
        class Class:
            __base__ = None

        def f():
            return Class.__base__

        for _ in range(1025):
            self.assertIs(f(), object)

    def test_type_descriptor_shadows_attribute_getset(self):
        class Class:
            __name__ = "Spam"

        def f():
            return Class.__name__

        for _ in range(1025):
            self.assertEqual(f(), "Class")

    @unittest.skip("Not fixed yet!")
    def test_metaclass_getattribute(self):
        class Metaclass(type):
            def __getattribute__(self, name):
                return True

        class Class(metaclass=Metaclass):
            attribute = False

        def f():
            return Class.attribute

        for _ in range(1025):
            self.assertTrue(f())

class TestLoadMethodCache(unittest.TestCase):
    def test_descriptor_added_after_optimization(self):
        class Descriptor:
            pass

        class C:
            def __init__(self):
                self.x = lambda: 1
            x = Descriptor()

        def f(o):
            return o.x()

        o = C()
        for i in range(1025):
            assert f(o) == 1

        Descriptor.__get__ = lambda self, instance, value: lambda: 2
        Descriptor.__set__ = lambda *args: None

        self.assertEqual(f(o), 2)

    def test_metaclass_descriptor_added_after_optimization(self):
        class Descriptor:
            pass

        class Metaclass(type):
            attribute = Descriptor()

        class Class(metaclass=Metaclass):
            attribute = lambda: True

        def f():
            return Class.attribute()

        for _ in range(1025):
            self.assertTrue(f())

        Descriptor.__get__ = lambda self, instance, value: lambda: False
        Descriptor.__set__ = lambda *args: None

        self.assertFalse(f())

    def test_metaclass_descriptor_shadows_class_attribute(self):
        class Metaclass(type):
            @property
            def attribute(self):
                return lambda: True

        class Class(metaclass=Metaclass):
            attribute = lambda: False

        def f():
            return Class.attribute()

        for _ in range(1025):
            self.assertTrue(f())

    def test_metaclass_set_descriptor_after_optimization(self):
        class Metaclass(type):
            pass

        class Class(metaclass=Metaclass):
            attribute = lambda: True

        @property
        def attribute(self):
            return lambda: False

        def f():
            return Class.attribute()

        for _ in range(1025):
            self.assertTrue(f())

        Metaclass.attribute = attribute
        self.assertFalse(f())

    def test_metaclass_del_descriptor_after_optimization(self):
        class Metaclass(type):
            @property
            def attribute(self):
                return lambda: True

        class Class(metaclass=Metaclass):
            attribute = lambda: False

        def f():
            return Class.attribute()

        for _ in range(1025):
            self.assertTrue(f())
        
        del Metaclass.attribute
        self.assertFalse(f())

    def test_type_descriptor_shadows_attribute_method(self):
        class Class:
            mro = lambda: ["Spam", "eggs"]

        def f():
            return Class.mro()

        for _ in range(1025):
            self.assertEqual(f(), ["Spam", "eggs"])

    def test_type_descriptor_shadows_attribute_member(self):
        class Class:
            __base__ = lambda: "Spam"

        def f():
            return Class.__base__()

        for _ in range(1025):
            self.assertNotEqual(f(), "Spam")

    @unittest.skip("Not fixed yet!")
    def test_metaclass_getattribute(self):
        class Metaclass(type):
            def __getattribute__(self, name):
                return lambda: True

        class Class(metaclass=Metaclass):
            attribute = lambda: False

        def f():
            return Class.attribute()

        for _ in range(1025):
            self.assertTrue(f())

if __name__ == "__main__":
    import unittest
    unittest.main()
