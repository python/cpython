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
        """You need to define the methods, even if they are in the spec"""
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

    def test_seal_with_autospec(self):
        # https://bugs.python.org/issue45156
        class Foo:
            foo = 0
            def bar1(self):
                return 1
            def bar2(self):
                return 2

            class Baz:
                baz = 3
                def ban(self):
                    return 4

        for spec_set in (True, False):
            with self.subTest(spec_set=spec_set):
                foo = mock.create_autospec(Foo, spec_set=spec_set)
                foo.bar1.return_value = 'a'
                foo.Baz.ban.return_value = 'b'

                mock.seal(foo)

                self.assertIsInstance(foo.foo, mock.NonCallableMagicMock)
                self.assertIsInstance(foo.bar1, mock.MagicMock)
                self.assertIsInstance(foo.bar2, mock.MagicMock)
                self.assertIsInstance(foo.Baz, mock.MagicMock)
                self.assertIsInstance(foo.Baz.baz, mock.NonCallableMagicMock)
                self.assertIsInstance(foo.Baz.ban, mock.MagicMock)

                self.assertEqual(foo.bar1(), 'a')
                foo.bar1.return_value = 'new_a'
                self.assertEqual(foo.bar1(), 'new_a')
                self.assertEqual(foo.Baz.ban(), 'b')
                foo.Baz.ban.return_value = 'new_b'
                self.assertEqual(foo.Baz.ban(), 'new_b')

                with self.assertRaises(TypeError):
                    foo.foo()
                with self.assertRaises(AttributeError):
                    foo.bar = 1
                with self.assertRaises(AttributeError):
                    foo.bar2()

                foo.bar2.return_value = 'bar2'
                self.assertEqual(foo.bar2(), 'bar2')

                with self.assertRaises(AttributeError):
                    foo.missing_attr
                with self.assertRaises(AttributeError):
                    foo.missing_attr = 1
                with self.assertRaises(AttributeError):
                    foo.missing_method()
                with self.assertRaises(TypeError):
                    foo.Baz.baz()
                with self.assertRaises(AttributeError):
                    foo.Baz.missing_attr
                with self.assertRaises(AttributeError):
                    foo.Baz.missing_attr = 1
                with self.assertRaises(AttributeError):
                    foo.Baz.missing_method()


if __name__ == "__main__":
    unittest.main()
