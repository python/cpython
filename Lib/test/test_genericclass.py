import unittest


class TestSubclassBase(unittest.TestCase):
    def test_subclass_base_signature(self):
        tested = []
        class B: ...
        class C:
            def __subclass_base__(self, *args, **kwargs):
                tested.extend([args, kwargs])
                return C
        c = C()
        self.assertEqual(tested, [])
        class D(B, c): ...
        self.assertEqual(tested[0], ((B, c),))
        self.assertEqual(tested[1], {})

    def test_subclass_base(self):
        tested = []
        class A: ...
        class B: ...
        class C:
            def __subclass_base__(self, bases):
                tested.append(bases)
                return self.__class__
        c = C()
        self.assertEqual(tested, [])
        class D(A, c, B): ...
        self.assertEqual(tested[-1], (A, c, B))
        self.assertEqual(D.__bases__, (A, C, B))
        self.assertEqual(D.__orig_bases__, (A, c, B))
        self.assertEqual(D.__mro__, (D, A, C, B, object))
        d = D()
        class E(d): ...
        self.assertEqual(tested[-1], (d,))
        self.assertEqual(E.__bases__, (D,))

    def test_subclass_base_none(self):
        tested = []
        class A: ...
        class B: ...
        class C:
            def __subclass_base__(self, bases):
                tested.append(bases)
                return None
        c = C()
        self.assertEqual(tested, [])
        class D(A, c, B): ...
        self.assertEqual(tested[-1], (A, c, B))
        self.assertEqual(D.__bases__, (A, B))
        self.assertEqual(D.__orig_bases__, (A, c, B))
        self.assertEqual(D.__mro__, (D, A, B, object))
        class E(c): ...
        self.assertEqual(tested[-1], (c,))
        self.assertEqual(E.__bases__, (object,))
        self.assertEqual(E.__orig_bases__, (c,))
        self.assertEqual(E.__mro__, (E, object))

    def test_subclass_base_with_builtins(self):
        tested = []
        class A: ...
        class C:
            def __subclass_base__(self, bases):
                tested.append(bases)
                return dict
        c = C()
        self.assertEqual(tested, [])
        class D(A, c): ...
        self.assertEqual(tested[-1], (A, c))
        self.assertEqual(D.__bases__, (A, dict))
        self.assertEqual(D.__orig_bases__, (A, c))
        self.assertEqual(D.__mro__, (D, A, dict, object))

    def test_subclass_base_with_builtins_2(self):
        tested = []
        class C:
            def __subclass_base__(self, bases):
                tested.append(bases)
                return C
        c = C()
        self.assertEqual(tested, [])
        class D(c, dict): ...
        self.assertEqual(tested[-1], (c, dict))
        self.assertEqual(D.__bases__, (C, dict))
        self.assertEqual(D.__orig_bases__, (c, dict))
        self.assertEqual(D.__mro__, (D, C, dict, object))

    def test_subclass_base_errors(self):
        class C_too_many:
            def __subclass_base__(self, bases, something, other):
                return None
        c = C_too_many()
        with self.assertRaises(TypeError):
            class D(c): ...
        class C_too_few:
            def __subclass_base__(self, bases, something, other):
                return None
        d = C_too_few()
        with self.assertRaises(TypeError):
            class D(d): ...

    def test_subclass_base_errors_2(self):
        class C_not_callable:
            __subclass_base__ = "Surprise!"
        c = C_not_callable()
        with self.assertRaises(TypeError):
            class D(c): ...

    def test_subclass_base_metaclass(self):
        meta_args = []
        class Meta(type):
            def __new__(mcls, name, bases, ns):
                meta_args.extend([mcls, name, bases, ns])
                return super().__new__(mcls, name, bases, ns)
        class A: ...
        class C:
            def __subclass_base__(self, bases):
                return A
        c = C()
        class D(c, metaclass=Meta):
            x = 1
        self.assertEqual(meta_args[0], Meta)
        self.assertEqual(meta_args[1], 'D')
        self.assertEqual(meta_args[2], (A,))
        self.assertEqual(meta_args[3]['x'], 1)
        self.assertEqual(D.__bases__, (A,))
        self.assertEqual(D.__orig_bases__, (c,))
        self.assertEqual(D.__mro__, (D, A, object))
        self.assertEqual(D.__class__, Meta)

    def test_subclass_base_type_call(self):
        # Substitution should _not_ happen in direct type call
        class C:
            def __subclass_base__(self, bases):
                return None
        c = C()
        with self.assertRaises(TypeError):
            type('Bad', (c,), {})


class TestClassGetitem(unittest.TestCase):
    def test_class_getitem(self):
        pass
    
    def test_class_getitem_with_builtins(self):
        pass
    
    def test_class_getitem_errors(self):
        pass
    
    def test_class_getitem_inheritance(self):
        pass
    
    def test_class_getitem_metaclass(self):
        pass


if __name__ == "__main__":
    unittest.main()
