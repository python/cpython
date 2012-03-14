import unittest
from warnings import catch_warnings

from unittest.test.testmock.support import is_instance
from unittest.mock import MagicMock, Mock, patch, sentinel, mock_open, call



something  = sentinel.Something
something_else  = sentinel.SomethingElse



class WithTest(unittest.TestCase):

    def test_with_statement(self):
        with patch('%s.something' % __name__, sentinel.Something2):
            self.assertEqual(something, sentinel.Something2, "unpatched")
        self.assertEqual(something, sentinel.Something)


    def test_with_statement_exception(self):
        try:
            with patch('%s.something' % __name__, sentinel.Something2):
                self.assertEqual(something, sentinel.Something2, "unpatched")
                raise Exception('pow')
        except Exception:
            pass
        else:
            self.fail("patch swallowed exception")
        self.assertEqual(something, sentinel.Something)


    def test_with_statement_as(self):
        with patch('%s.something' % __name__) as mock_something:
            self.assertEqual(something, mock_something, "unpatched")
            self.assertTrue(is_instance(mock_something, MagicMock),
                            "patching wrong type")
        self.assertEqual(something, sentinel.Something)


    def test_patch_object_with_statement(self):
        class Foo(object):
            something = 'foo'
        original = Foo.something
        with patch.object(Foo, 'something'):
            self.assertNotEqual(Foo.something, original, "unpatched")
        self.assertEqual(Foo.something, original)


    def test_with_statement_nested(self):
        with catch_warnings(record=True):
            with patch('%s.something' % __name__) as mock_something, patch('%s.something_else' % __name__) as mock_something_else:
                self.assertEqual(something, mock_something, "unpatched")
                self.assertEqual(something_else, mock_something_else,
                                 "unpatched")

        self.assertEqual(something, sentinel.Something)
        self.assertEqual(something_else, sentinel.SomethingElse)


    def test_with_statement_specified(self):
        with patch('%s.something' % __name__, sentinel.Patched) as mock_something:
            self.assertEqual(something, mock_something, "unpatched")
            self.assertEqual(mock_something, sentinel.Patched, "wrong patch")
        self.assertEqual(something, sentinel.Something)


    def testContextManagerMocking(self):
        mock = Mock()
        mock.__enter__ = Mock()
        mock.__exit__ = Mock()
        mock.__exit__.return_value = False

        with mock as m:
            self.assertEqual(m, mock.__enter__.return_value)
        mock.__enter__.assert_called_with()
        mock.__exit__.assert_called_with(None, None, None)


    def test_context_manager_with_magic_mock(self):
        mock = MagicMock()

        with self.assertRaises(TypeError):
            with mock:
                'foo' + 3
        mock.__enter__.assert_called_with()
        self.assertTrue(mock.__exit__.called)


    def test_with_statement_same_attribute(self):
        with patch('%s.something' % __name__, sentinel.Patched) as mock_something:
            self.assertEqual(something, mock_something, "unpatched")

            with patch('%s.something' % __name__) as mock_again:
                self.assertEqual(something, mock_again, "unpatched")

            self.assertEqual(something, mock_something,
                             "restored with wrong instance")

        self.assertEqual(something, sentinel.Something, "not restored")


    def test_with_statement_imbricated(self):
        with patch('%s.something' % __name__) as mock_something:
            self.assertEqual(something, mock_something, "unpatched")

            with patch('%s.something_else' % __name__) as mock_something_else:
                self.assertEqual(something_else, mock_something_else,
                                 "unpatched")

        self.assertEqual(something, sentinel.Something)
        self.assertEqual(something_else, sentinel.SomethingElse)


    def test_dict_context_manager(self):
        foo = {}
        with patch.dict(foo, {'a': 'b'}):
            self.assertEqual(foo, {'a': 'b'})
        self.assertEqual(foo, {})

        with self.assertRaises(NameError):
            with patch.dict(foo, {'a': 'b'}):
                self.assertEqual(foo, {'a': 'b'})
                raise NameError('Konrad')

        self.assertEqual(foo, {})



class TestMockOpen(unittest.TestCase):

    def test_mock_open(self):
        mock = mock_open()
        with patch('%s.open' % __name__, mock, create=True) as patched:
            self.assertIs(patched, mock)
            open('foo')

        mock.assert_called_once_with('foo')


    def test_mock_open_context_manager(self):
        mock = mock_open()
        handle = mock.return_value
        with patch('%s.open' % __name__, mock, create=True):
            with open('foo') as f:
                f.read()

        expected_calls = [call('foo'), call().__enter__(), call().read(),
                          call().__exit__(None, None, None)]
        self.assertEqual(mock.mock_calls, expected_calls)
        self.assertIs(f, handle)


    def test_explicit_mock(self):
        mock = MagicMock()
        mock_open(mock)

        with patch('%s.open' % __name__, mock, create=True) as patched:
            self.assertIs(patched, mock)
            open('foo')

        mock.assert_called_once_with('foo')


    def test_read_data(self):
        mock = mock_open(read_data='foo')
        with patch('%s.open' % __name__, mock, create=True):
            h = open('bar')
            result = h.read()

        self.assertEqual(result, 'foo')


if __name__ == '__main__':
    unittest.main()
