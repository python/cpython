from unittest import TestCase

from _pyrepl.utils import str_width, wlen, prev_next_window


class TestUtils(TestCase):
    def test_str_width(self):
        characters = ['a', '1', '_', '!', '\x1a', '\u263A', '\uffb9']
        for c in characters:
            self.assertEqual(str_width(c), 1)

        characters = [chr(99989), chr(99999)]
        for c in characters:
            self.assertEqual(str_width(c), 2)

    def test_wlen(self):
        for c in ['a', 'b', '1', '!', '_']:
            self.assertEqual(wlen(c), 1)
        self.assertEqual(wlen('\x1a'), 2)

        char_east_asian_width_N = chr(3800)
        self.assertEqual(wlen(char_east_asian_width_N), 1)
        char_east_asian_width_W = chr(4352)
        self.assertEqual(wlen(char_east_asian_width_W), 2)

        self.assertEqual(wlen('hello'), 5)
        self.assertEqual(wlen('hello' + '\x1a'), 7)

    def test_prev_next_window(self):
        def gen_normal():
            yield 1
            yield 2
            yield 3
            yield 4

        pnw = prev_next_window(gen_normal())
        self.assertEqual(next(pnw), (None, 1, 2))
        self.assertEqual(next(pnw), (1, 2, 3))
        self.assertEqual(next(pnw), (2, 3, 4))
        self.assertEqual(next(pnw), (3, 4, None))
        with self.assertRaises(StopIteration):
            next(pnw)

        def gen_short():
            yield 1

        pnw = prev_next_window(gen_short())
        self.assertEqual(next(pnw), (None, 1, None))
        with self.assertRaises(StopIteration):
            next(pnw)

        def gen_raise():
            yield from gen_normal()
            1/0

        pnw = prev_next_window(gen_raise())
        self.assertEqual(next(pnw), (None, 1, 2))
        self.assertEqual(next(pnw), (1, 2, 3))
        self.assertEqual(next(pnw), (2, 3, 4))
        self.assertEqual(next(pnw), (3, 4, None))
        with self.assertRaises(ZeroDivisionError):
            next(pnw)
