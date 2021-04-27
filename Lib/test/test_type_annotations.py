import unittest

class TypeAnnotationTests(unittest.TestCase):

    def test_lazy_create_annotations(self):
        # type objects lazy create their __annotations__ dict on demand.
        # the annotations dict is stored in type.__dict__.
        # a freshly created type shouldn't have an annotations dict yet.
        foo = type("Foo", (), {})
        for i in range(3):
            self.assertFalse("__annotations__" in foo.__dict__)
            d = foo.__annotations__
            self.assertTrue("__annotations__" in foo.__dict__)
            self.assertEqual(foo.__annotations__, d)
            self.assertEqual(foo.__dict__['__annotations__'], d)
            del foo.__annotations__

    def test_setting_annotations(self):
        foo = type("Foo", (), {})
        for i in range(3):
            self.assertFalse("__annotations__" in foo.__dict__)
            d = {'a': int}
            foo.__annotations__ = d
            self.assertTrue("__annotations__" in foo.__dict__)
            self.assertEqual(foo.__annotations__, d)
            self.assertEqual(foo.__dict__['__annotations__'], d)
            del foo.__annotations__

    def test_annotations_getset_raises(self):
        # builtin types don't have __annotations__ (yet!)
        with self.assertRaises(AttributeError):
            print(float.__annotations__)
        with self.assertRaises(TypeError):
            float.__annotations__ = {}
        with self.assertRaises(TypeError):
            del float.__annotations__

        # double delete
        foo = type("Foo", (), {})
        foo.__annotations__ = {}
        del foo.__annotations__
        with self.assertRaises(AttributeError):
            del foo.__annotations__

    def test_annotations_are_created_correctly(self):
        class C:
            a:int=3
            b:str=4
        self.assertTrue("__annotations__" in C.__dict__)
        del C.__annotations__
        self.assertFalse("__annotations__" in C.__dict__)
