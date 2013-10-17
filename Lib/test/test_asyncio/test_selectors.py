"""Tests for selectors.py."""

import unittest
import unittest.mock

from asyncio import selectors


class FakeSelector(selectors.BaseSelector):
    """Trivial non-abstract subclass of BaseSelector."""

    def select(self, timeout=None):
        raise NotImplementedError


class BaseSelectorTests(unittest.TestCase):

    def test_fileobj_to_fd(self):
        self.assertEqual(10, selectors._fileobj_to_fd(10))

        f = unittest.mock.Mock()
        f.fileno.return_value = 10
        self.assertEqual(10, selectors._fileobj_to_fd(f))

        f.fileno.side_effect = AttributeError
        self.assertRaises(ValueError, selectors._fileobj_to_fd, f)

    def test_selector_key_repr(self):
        key = selectors.SelectorKey(10, 10, selectors.EVENT_READ, None)
        self.assertEqual(
            "SelectorKey(fileobj=10, fd=10, events=1, data=None)", repr(key))

    def test_register(self):
        fobj = unittest.mock.Mock()
        fobj.fileno.return_value = 10

        s = FakeSelector()
        key = s.register(fobj, selectors.EVENT_READ)
        self.assertIsInstance(key, selectors.SelectorKey)
        self.assertEqual(key.fd, 10)
        self.assertIs(key, s._fd_to_key[10])

    def test_register_unknown_event(self):
        s = FakeSelector()
        self.assertRaises(ValueError, s.register, unittest.mock.Mock(), 999999)

    def test_register_already_registered(self):
        fobj = unittest.mock.Mock()
        fobj.fileno.return_value = 10

        s = FakeSelector()
        s.register(fobj, selectors.EVENT_READ)
        self.assertRaises(KeyError, s.register, fobj, selectors.EVENT_READ)

    def test_unregister(self):
        fobj = unittest.mock.Mock()
        fobj.fileno.return_value = 10

        s = FakeSelector()
        s.register(fobj, selectors.EVENT_READ)
        s.unregister(fobj)
        self.assertFalse(s._fd_to_key)

    def test_unregister_unknown(self):
        fobj = unittest.mock.Mock()
        fobj.fileno.return_value = 10

        s = FakeSelector()
        self.assertRaises(KeyError, s.unregister, fobj)

    def test_modify_unknown(self):
        fobj = unittest.mock.Mock()
        fobj.fileno.return_value = 10

        s = FakeSelector()
        self.assertRaises(KeyError, s.modify, fobj, 1)

    def test_modify(self):
        fobj = unittest.mock.Mock()
        fobj.fileno.return_value = 10

        s = FakeSelector()
        key = s.register(fobj, selectors.EVENT_READ)
        key2 = s.modify(fobj, selectors.EVENT_WRITE)
        self.assertNotEqual(key.events, key2.events)
        self.assertEqual(
            selectors.SelectorKey(fobj, 10, selectors.EVENT_WRITE, None),
            s.get_key(fobj))

    def test_modify_data(self):
        fobj = unittest.mock.Mock()
        fobj.fileno.return_value = 10

        d1 = object()
        d2 = object()

        s = FakeSelector()
        key = s.register(fobj, selectors.EVENT_READ, d1)
        key2 = s.modify(fobj, selectors.EVENT_READ, d2)
        self.assertEqual(key.events, key2.events)
        self.assertNotEqual(key.data, key2.data)
        self.assertEqual(
            selectors.SelectorKey(fobj, 10, selectors.EVENT_READ, d2),
            s.get_key(fobj))

    def test_modify_same(self):
        fobj = unittest.mock.Mock()
        fobj.fileno.return_value = 10

        data = object()

        s = FakeSelector()
        key = s.register(fobj, selectors.EVENT_READ, data)
        key2 = s.modify(fobj, selectors.EVENT_READ, data)
        self.assertIs(key, key2)

    def test_select(self):
        s = FakeSelector()
        self.assertRaises(NotImplementedError, s.select)

    def test_close(self):
        s = FakeSelector()
        s.register(1, selectors.EVENT_READ)

        s.close()
        self.assertFalse(s._fd_to_key)

    def test_context_manager(self):
        s = FakeSelector()

        with s as sel:
            sel.register(1, selectors.EVENT_READ)

        self.assertFalse(s._fd_to_key)

    def test_key_from_fd(self):
        s = FakeSelector()
        key = s.register(1, selectors.EVENT_READ)

        self.assertIs(key, s._key_from_fd(1))
        self.assertIsNone(s._key_from_fd(10))

    if hasattr(selectors.DefaultSelector, 'fileno'):
        def test_fileno(self):
            self.assertIsInstance(selectors.DefaultSelector().fileno(), int)
