import unittest
import sys

import _pyrepl.readline
from _pyrepl.fancycompleter import Completer, DefaultConfig, commonprefix


class ConfigForTest(DefaultConfig):
    use_colors = False

class ColorConfig(DefaultConfig):
    use_colors = True

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
        self.assertEqual(commonprefix(['isalpha', 'isdigit', 'foo'], base='i'), 'is')
        self.assertEqual(commonprefix([]), '')
        self.assertEqual(commonprefix(['aaa', 'bbb'], base='x'), '')

    def test_complete_attribute(self):
        compl = Completer({'a': None}, ConfigForTest)
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
        compl = Completer({'a': C}, ConfigForTest)
        self.assertEqual(compl.attr_matches('a.'), ['attr', 'mro'])
        self.assertEqual(compl.attr_matches('a._'), ['_C__attr__attr', '_attr', ' '])
        matches = compl.attr_matches('a.__')
        self.assertNotIn('a.__class__', matches)
        self.assertIn('__class__', matches)
        self.assertEqual(compl.attr_matches('a.__class'), ['a.__class__'])

        compl = Completer({'a': None}, ConfigForTest)
        self.assertEqual(compl.attr_matches('a._'), ['a.__'])

    def test_complete_attribute_colored(self):
        compl = Completer({'a': 42}, ColorConfig)
        matches = compl.attr_matches('a.__')
        self.assertGreater(len(matches), 2)
        expected_color = compl.config.color_by_type.get(type(compl.__class__))
        self.assertEqual(expected_color, '35;01')
        expected_part = Color.set(expected_color, '__class__')
        for match in matches:
            if expected_part in match:
                break
        else:
            self.assertFalse(True, matches)
        self.assertIn(' ', matches)

    def test_complete_colored_single_match(self):
        """No coloring, via commonprefix."""
        compl = Completer({'foobar': 42}, ColorConfig)
        matches = compl.global_matches('foob')
        self.assertEqual(matches, ['foobar'])

    def test_does_not_color_single_match(self):
        class obj:
            msgs = []

        compl = Completer({'obj': obj}, ColorConfig)
        matches = compl.attr_matches('obj.msgs')
        self.assertEqual(matches, ['obj.msgs'])

    def test_complete_global(self):
        compl = Completer({'foobar': 1, 'foobazzz': 2}, ConfigForTest)
        self.assertEqual(compl.global_matches('foo'), ['fooba'])
        matches = compl.global_matches('fooba')
        self.assertEqual(set(matches), set(['foobar', 'foobazzz']))
        self.assertEqual(compl.global_matches('foobaz'), ['foobazzz'])
        self.assertEqual(compl.global_matches('nothing'), [])

    def test_complete_global_colored(self):
        compl = Completer({'foobar': 1, 'foobazzz': 2}, ColorConfig)
        self.assertEqual(compl.global_matches('foo'), ['fooba'])
        matches = compl.global_matches('fooba')
        self.assertEqual(set(matches), {
            ' ',
            '\x1b[001;00m\x1b[33;01mfoobazzz\x1b[00m',
            '\x1b[000;00m\x1b[33;01mfoobar\x1b[00m',
        })
        self.assertEqual(compl.global_matches('foobaz'), ['foobazzz'])
        self.assertEqual(compl.global_matches('nothing'), [])

    def test_complete_global_colored_exception(self):
        compl = Completer({'tryme': ValueError()}, ColorConfig)
        if sys.version_info >= (3, 6):
            self.assertEqual(compl.global_matches('try'), [
                '\x1b[000;00m\x1b[37mtry:\x1b[00m',
                '\x1b[001;00m\x1b[31;01mtryme\x1b[00m',
                ' '
            ])
        else:
            self.assertEqual(compl.global_matches('try'), [
                '\x1b[000;00m\x1b[37mtry\x1b[00m',
                '\x1b[001;00m\x1b[31;01mtryme\x1b[00m',
                ' '
            ])

    def test_color_for_obj(self):
        class Config(ColorConfig):
            color_by_type = {}

        compl = Completer({}, Config)
        self.assertEqual(compl.color_for_obj(1, "foo", "bar"),
                        "\x1b[001;00m\x1b[00mfoo\x1b[00m")

    def test_complete_with_indexer(self):
        compl = Completer({'lst': [None, 2, 3]}, ConfigForTest)
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
        compl = Completer({'A': A}, ConfigForTest)
        #
        # in this case, we want to display all attributes which start with
        # 'a'. MOREOVER, we also include a space to prevent readline to
        # automatically insert the common prefix (which will the the ANSI escape
        # sequence if we use colors)
        matches = compl.attr_matches('A.a')
        self.assertEqual(sorted(matches), [' ', 'aaa', 'abc_1', 'abc_2', 'abc_3'])
        #
        # IF there is an actual common prefix, we return just it, so that readline
        # will insert it into place
        matches = compl.attr_matches('A.ab')
        self.assertEqual(matches, ['A.abc_'])
        #
        # finally, at the next TAB, we display again all the completions available
        # for this common prefix. Agai, we insert a spurious space to prevent the
        # automatic completion of ANSI sequences
        matches = compl.attr_matches('A.abc_')
        self.assertEqual(sorted(matches), [' ', 'abc_1', 'abc_2', 'abc_3'])

    def test_complete_exception(self):
        compl = Completer({}, ConfigForTest)
        self.assertEqual(compl.attr_matches('xxx.'), [])

    def test_complete_invalid_attr(self):
        compl = Completer({'str': str}, ConfigForTest)
        self.assertEqual(compl.attr_matches('str.xx'), [])

    def test_complete_function_skipped(self):
        compl = Completer({'str': str}, ConfigForTest)
        self.assertEqual(compl.attr_matches('str.split().'), [])

    def test_unicode_in___dir__(self):
        class Foo(object):
            def __dir__(self):
                return [u'hello', 'world']

        compl = Completer({'a': Foo()}, ConfigForTest)
        matches = compl.attr_matches('a.')
        self.assertEqual(matches, ['hello', 'world'])
        self.assertIs(type(matches[0]), str)


if __name__ == "__main__":
    unittest.main()
