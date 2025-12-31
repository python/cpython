import unittest

from _colorize import ANSIColors, get_theme
from _pyrepl.fancycompleter import Completer, commonprefix

class MockPatch:
    def __init__(self):
        self.original_values = {}

    def setattr(self, obj, name, value):
        if obj not in self.original_values:
            self.original_values[obj] = {}
        if name not in self.original_values[obj]:
            self.original_values[obj][name] = getattr(obj, name)
        setattr(obj, name, value)

    def restore_all(self):
        for obj, attrs in self.original_values.items():
            for name, value in attrs.items():
                setattr(obj, name, value)

class FancyCompleterTests(unittest.TestCase):
    def setUp(self):
        self.mock_patch = MockPatch()

    def tearDown(self):
        self.mock_patch.restore_all()

    def test_commonprefix(self):
        self.assertEqual(commonprefix(['isalpha', 'isdigit', 'foo']), '')
        self.assertEqual(commonprefix(['isalpha', 'isdigit']), 'is')
        self.assertEqual(commonprefix([]), '')

    def test_complete_attribute(self):
        compl = Completer({'a': None}, use_colors=False)
        self.assertEqual(compl.attr_matches('a.'), ['a.__'])
        matches = compl.attr_matches('a.__')
        self.assertNotIn('a.__class__', matches)
        self.assertIn('__class__', matches)
        self.assertEqual(compl.attr_matches('a.__class'), ['a.__class__'])

    def test_complete_attribute_prefix(self):
        class C(object):
            attr = 1
            _attr = 2
            __attr__attr = 3
        compl = Completer({'a': C}, use_colors=False)
        self.assertEqual(compl.attr_matches('a.'), ['attr', 'mro'])
        self.assertEqual(compl.attr_matches('a._'), ['_C__attr__attr', '_attr', ' '])
        matches = compl.attr_matches('a.__')
        self.assertNotIn('a.__class__', matches)
        self.assertIn('__class__', matches)
        self.assertEqual(compl.attr_matches('a.__class'), ['a.__class__'])

        compl = Completer({'a': None}, use_colors=False)
        self.assertEqual(compl.attr_matches('a._'), ['a.__'])

    def test_complete_attribute_colored(self):
        theme = get_theme()
        compl = Completer({'a': 42}, use_colors=True)
        matches = compl.attr_matches('a.__')
        self.assertGreater(len(matches), 2)
        expected_color = theme.fancycompleter.type
        expected_part = f'{expected_color}__class__{ANSIColors.RESET}'
        for match in matches:
            if expected_part in match:
                break
        else:
            self.assertFalse(True, matches)
        self.assertIn(' ', matches)

    def test_complete_colored_single_match(self):
        """No coloring, via commonprefix."""
        compl = Completer({'foobar': 42}, use_colors=True)
        matches = compl.global_matches('foob')
        self.assertEqual(matches, ['foobar'])

    def test_does_not_color_single_match(self):
        class obj:
            msgs = []

        compl = Completer({'obj': obj}, use_colors=True)
        matches = compl.attr_matches('obj.msgs')
        self.assertEqual(matches, ['obj.msgs'])

    def test_complete_global(self):
        compl = Completer({'foobar': 1, 'foobazzz': 2}, use_colors=False)
        self.assertEqual(compl.global_matches('foo'), ['fooba'])
        matches = compl.global_matches('fooba')
        self.assertEqual(set(matches), set(['foobar', 'foobazzz']))
        self.assertEqual(compl.global_matches('foobaz'), ['foobazzz'])
        self.assertEqual(compl.global_matches('nothing'), [])

    def test_complete_global_colored(self):
        theme = get_theme()
        compl = Completer({'foobar': 1, 'foobazzz': 2}, use_colors=True)
        self.assertEqual(compl.global_matches('foo'), ['fooba'])
        matches = compl.global_matches('fooba')

        # these are the fake escape sequences which are needed so that
        # readline displays the matches in the proper order
        N0 = f"\x1b[000;00m"
        N1 = f"\x1b[001;00m"
        int_color = theme.fancycompleter.int
        self.assertEqual(set(matches), {
            ' ',
            f'{N0}{int_color}foobar{ANSIColors.RESET}',
            f'{N1}{int_color}foobazzz{ANSIColors.RESET}',
        })
        self.assertEqual(compl.global_matches('foobaz'), ['foobazzz'])
        self.assertEqual(compl.global_matches('nothing'), [])

    def test_complete_with_indexer(self):
        compl = Completer({'lst': [None, 2, 3]}, use_colors=False)
        self.assertEqual(compl.attr_matches('lst[0].'), ['lst[0].__'])
        matches = compl.attr_matches('lst[0].__')
        self.assertNotIn('lst[0].__class__', matches)
        self.assertIn('__class__', matches)
        self.assertEqual(compl.attr_matches('lst[0].__class'), ['lst[0].__class__'])

    def test_autocomplete(self):
        class A:
            aaa = None
            abc_1 = None
            abc_2 = None
            abc_3 = None
            bbb = None
        compl = Completer({'A': A}, use_colors=False)
        #
        # In this case, we want to display all attributes which start with
        # 'a'. Moreover, we also include a space to prevent readline to
        # automatically insert the common prefix (which will the the ANSI escape
        # sequence if we use colors).
        matches = compl.attr_matches('A.a')
        self.assertEqual(sorted(matches), [' ', 'aaa', 'abc_1', 'abc_2', 'abc_3'])
        #
        # If there is an actual common prefix, we return just it, so that readline
        # will insert it into place
        matches = compl.attr_matches('A.ab')
        self.assertEqual(matches, ['A.abc_'])
        #
        # Finally, at the next tab, we display again all the completions available
        # for this common prefix. Again, we insert a spurious space to prevent the
        # automatic completion of ANSI sequences.
        matches = compl.attr_matches('A.abc_')
        self.assertEqual(sorted(matches), [' ', 'abc_1', 'abc_2', 'abc_3'])

    def test_complete_exception(self):
        compl = Completer({}, use_colors=False)
        self.assertEqual(compl.attr_matches('xxx.'), [])

    def test_complete_invalid_attr(self):
        compl = Completer({'str': str}, use_colors=False)
        self.assertEqual(compl.attr_matches('str.xx'), [])

    def test_complete_function_skipped(self):
        compl = Completer({'str': str}, use_colors=False)
        self.assertEqual(compl.attr_matches('str.split().'), [])

    def test_unicode_in___dir__(self):
        class Foo(object):
            def __dir__(self):
                return ['hello', 'world']

        compl = Completer({'a': Foo()}, use_colors=False)
        matches = compl.attr_matches('a.')
        self.assertEqual(matches, ['hello', 'world'])
        self.assertIs(type(matches[0]), str)


if __name__ == "__main__":
    unittest.main()
