import unittest
from unittest import mock


class SampleObject:

    def method_sample1(self): pass

    def method_sample2(self): pass


class TestSealable(unittest.TestCase):

    def test_attributes_return_more_mocks_by_default(self):
        m = mock.Mock()

        self.assertIsInstance(m.test, mock.Mock)
        self.assertIsInstance(m.test(), mock.Mock)
        self.assertIsInstance(m.test().test2(), mock.Mock)

    def test_new_attributes_cannot_be_accessed_on_seal(self):
        m = mock.Mock()

        mock.seal(m)
        with self.assertRaises(AttributeError):
            m.test
        with self.assertRaises(AttributeError):
            m()

    def test_new_attributes_cannot_be_set_on_seal(self):
        m = mock.Mock()

        mock.seal(m)
        with self.assertRaises(AttributeError):
            m.test = 1

    def test_existing_attributes_can_be_set_on_seal(self):
        m = mock.Mock()
        m.test.test2 = 1

        mock.seal(m)
        m.test.test2 = 2
        self.assertEqual(m.test.test2, 2)

    def test_new_attributes_cannot_be_set_on_child_of_seal(self):
        m = mock.Mock()
        m.test.test2 = 1

        mock.seal(m)
        with self.assertRaises(AttributeError):
            m.test.test3 = 1

    def test_existing_attributes_allowed_after_seal(self):
        m = mock.Mock()

        m.test.return_value = 3

        mock.seal(m)
        self.assertEqual(m.test(), 3)

    def test_initialized_attributes_allowed_after_seal(self):
        m = mock.Mock(test_value=1)

        mock.seal(m)
        self.assertEqual(m.test_value, 1)

    def test_call_on_sealed_mock_fails(self):
        m = mock.Mock()

        mock.seal(m)
        with self.assertRaises(AttributeError):
            m()

    def test_call_on_defined_sealed_mock_succeeds(self):
        m = mock.Mock(return_value=5)

        mock.seal(m)
        self.assertEqual(m(), 5)

    def test_seals_recurse_on_added_attributes(self):
        m = mock.Mock()

        m.test1.test2().test3 = 4

        mock.seal(m)
        self.assertEqual(m.test1.test2().test3, 4)
        with self.assertRaises(AttributeError):
            m.test1.test2().test4
        with self.assertRaises(AttributeError):
            m.test1.test3

    def test_seals_recurse_on_magic_methods(self):
        m = mock.MagicMock()

        m.test1.test2["a"].test3 = 4
        m.test1.test3[2:5].test3 = 4

        mock.seal(m)
        self.assertEqual(m.test1.test2["a"].test3, 4)
        self.assertEqual(m.test1.test2[2:5].test3, 4)
        with self.assertRaises(AttributeError):
            m.test1.test2["a"].test4
        with self.assertRaises(AttributeError):
            m.test1.test3[2:5].test4

    def test_seals_dont_recurse_on_manual_attributes(self):
        m = mock.Mock(name="root_mock")

        m.test1.test2 = mock.Mock(name="not_sealed")
        m.test1.test2.test3 = 4

        mock.seal(m)
        self.assertEqual(m.test1.test2.test3, 4)
        m.test1.test2.test4  # Does not raise
        m.test1.test2.test4 = 1  # Does not raise

    def test_integration_with_spec_att_definition(self):
        """You are not restricted when using mock with spec"""
        m = mock.Mock(SampleObject)

        m.attr_sample1 = 1
        m.attr_sample3 = 3

        mock.seal(m)
        self.assertEqual(m.attr_sample1, 1)
        self.assertEqual(m.attr_sample3, 3)
        with self.assertRaises(AttributeError):
            m.attr_sample2

    def test_integration_with_spec_method_definition(self):
        """You need to defin the methods, even if they are in the spec"""
        m = mock.Mock(SampleObject)

        m.method_sample1.return_value = 1

        mock.seal(m)
        self.assertEqual(m.method_sample1(), 1)
        with self.assertRaises(AttributeError):
            m.method_sample2()

    def test_integration_with_spec_method_definition_respects_spec(self):
        """You cannot define methods out of the spec"""
        m = mock.Mock(SampleObject)

        with self.assertRaises(AttributeError):
            m.method_sample3.return_value = 3

    def test_sealed_exception_has_attribute_name(self):
        m = mock.Mock()

        mock.seal(m)
        with self.assertRaises(AttributeError) as cm:
            m.SECRETE_name
        self.assertIn("SECRETE_name", str(cm.exception))

    def test_attribute_chain_is_maintained(self):
        m = mock.Mock(name="mock_name")
        m.test1.test2.test3.test4

        mock.seal(m)
        with self.assertRaises(AttributeError) as cm:
            m.test1.test2.test3.test4.boom
        self.assertIn("mock_name.test1.test2.test3.test4.boom", str(cm.exception))

    def test_call_chain_is_maintained(self):
        m = mock.Mock()
        m.test1().test2.test3().test4

        mock.seal(m)
        with self.assertRaises(AttributeError) as cm:
            m.test1().test2.test3().test4()
        self.assertIn("mock.test1().test2.test3().test4", str(cm.exception))

    def test_seal_with_attr_autospec(self):
        # https://bugs.python.org/issue45156
        class Foo:
            foo = 0

        for spec_set in (True, False):
            with self.subTest(spec_set=spec_set):
                foo = mock.create_autospec(Foo, spec_set=spec_set)
                mock.seal(foo)

                self.assertIsInstance(foo.foo, mock.NonCallableMagicMock)
                with self.assertRaises(TypeError):
                    foo.foo()
                with self.assertRaises(AttributeError):
                    foo.bar = 1
                with self.assertRaises(AttributeError):
                    foo.missing_attr
                with self.assertRaises(AttributeError):
                    foo.missing_method()

    def test_seal_with_method_autospec(self):
        # https://bugs.python.org/issue45156
        class Foo:
            def foo(self):
                return 1

        for spec_set in (True, False):
            with self.subTest(spec_set=spec_set):
                foo = mock.create_autospec(Foo, spec_set=spec_set)
                foo.foo.return_value = 0
                mock.seal(foo)

                self.assertIsInstance(foo.foo, mock.MagicMock)
                self.assertEqual(foo.foo(), 0)
                with self.assertRaises(AttributeError):
                    foo.bar = 1
                with self.assertRaises(AttributeError):
                    foo.missing_attr
                with self.assertRaises(AttributeError):
                    foo.missing_method()

    def test_seal_with_nested_class_autospec(self):
        # https://bugs.python.org/issue45156
        class Foo:
            class Baz:
                def baz(self):
                    return 1

        for spec_set in (True, False):
            with self.subTest(spec_set=spec_set):
                foo = mock.create_autospec(Foo, spec_set=spec_set)
                foo.Baz.baz.return_value = 0
                mock.seal(foo)

                self.assertIsInstance(foo.Baz.baz, mock.MagicMock)
                self.assertEqual(foo.Baz.baz(), 0)
                with self.assertRaises(AttributeError):
                    foo.bar = 1
                with self.assertRaises(AttributeError):
                    foo.Baz.bar = 1
                with self.assertRaises(AttributeError):
                    foo.Baz.missing_method()


if __name__ == "__main__":
    unittest.main()
