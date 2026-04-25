import importlib
import os
import types
import unittest

from _colorize import ANSIColors, get_theme
from _pyrepl.completing_reader import stripcolor
from _pyrepl.fancycompleter import Completer, commonprefix
from test.support.import_helper import ready_to_import

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
        self.assertNotIn('__class__', matches)
        self.assertIn('a.__class__', matches)
        match = compl.attr_matches('a.__class')
        self.assertEqual(len(match), 1)
        self.assertTrue(match[0].startswith('a.__class__'))

    def test_complete_attribute_prefix(self):
        class C(object):
            attr = 1
            _attr = 2
            __attr__attr = 3
        compl = Completer({'a': C}, use_colors=False)
        self.assertEqual(compl.attr_matches('a.'), ['a.attr', 'a.mro'])
        self.assertEqual(
            compl.attr_matches('a._'),
            ['a._C__attr__attr', 'a._attr'],
        )
        matches = compl.attr_matches('a.__')
        self.assertNotIn('__class__', matches)
        self.assertIn('a.__class__', matches)
        match = compl.attr_matches('a.__class')
        self.assertEqual(len(match), 1)
        self.assertTrue(match[0].startswith('a.__class__'))

        compl = Completer({'a': None}, use_colors=False)
        self.assertEqual(compl.attr_matches('a._'), ['a.__'])

    def test_complete_attribute_colored(self):
        theme = get_theme()
        compl = Completer({'a': 42}, use_colors=True)
        matches = compl.attr_matches('a.__')
        self.assertGreater(len(matches), 2)
        expected_color = theme.fancycompleter.type
        expected_part = f'{expected_color}a.__class__{ANSIColors.RESET}'
        for match in matches:
            if expected_part in match:
                break
        else:
            self.assertFalse(True, matches)
        self.assertNotIn(' ', matches)

    def test_preserves_callable_postfix_for_single_attribute_match(self):
        compl = Completer({'os': os}, use_colors=False)
        self.assertEqual(compl.attr_matches('os.getpid'), ['os.getpid()'])

    def test_property_method_not_called(self):
        class Foo:
            property_called = False

            @property
            def bar(self):
                self.property_called = True
                return 1

        foo = Foo()
        compl = Completer({'foo': foo}, use_colors=False)
        self.assertEqual(compl.attr_matches('foo.b'), ['foo.bar'])
        self.assertFalse(foo.property_called)

    def test_excessive_getattr(self):
        class Foo:
            calls = 0
            bar = ''

            def __getattribute__(self, name):
                if name == 'bar':
                    self.calls += 1
                    return None
                return super().__getattribute__(name)

        foo = Foo()
        compl = Completer({'foo': foo}, use_colors=False)
        self.assertEqual(compl.complete('foo.b', 0), 'foo.bar')
        self.assertEqual(foo.calls, 1)

    def test_uncreated_attr(self):
        class Foo:
            __slots__ = ('bar',)

        compl = Completer({'foo': Foo()}, use_colors=False)
        self.assertEqual(compl.complete('foo.', 0), 'foo.bar')

    def test_module_attributes_do_not_reify_lazy_imports(self):
        with ready_to_import("test_pyrepl_lazy_mod", "lazy import json\n") as (name, _):
            module = importlib.import_module(name)
            self.assertIs(type(module.__dict__["json"]), types.LazyImportType)

            compl = Completer({name: module}, use_colors=False)
            self.assertEqual(compl.attr_matches(f"{name}.j"), [f"{name}.json"])
            self.assertIs(type(module.__dict__["json"]), types.LazyImportType)

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

        int_color = theme.fancycompleter.int
        self.assertEqual(matches, [
            f'{int_color}foobar{ANSIColors.RESET}',
            f'{int_color}foobazzz{ANSIColors.RESET}',
        ])
        self.assertEqual(compl.global_matches('foobaz'), ['foobazzz'])
        self.assertEqual(compl.global_matches('nothing'), [])

    def test_colorized_match_is_stripped(self):
        compl = Completer({'a': 42}, use_colors=True)
        match = compl._color_for_obj('spam', 1)
        self.assertEqual(stripcolor(match), 'spam')

    def test_complete_with_indexer(self):
        compl = Completer({'lst': [None, 2, 3]}, use_colors=False)
        self.assertEqual(compl.attr_matches('lst[0].'), ['lst[0].__'])
        matches = compl.attr_matches('lst[0].__')
        self.assertNotIn('__class__', matches)
        self.assertIn('lst[0].__class__', matches)
        match = compl.attr_matches('lst[0].__class')
        self.assertEqual(len(match), 1)
        self.assertTrue(match[0].startswith('lst[0].__class__'))

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
        # 'a'.
        matches = compl.attr_matches('A.a')
        self.assertEqual(
            sorted(matches),
            ['A.aaa', 'A.abc_1', 'A.abc_2', 'A.abc_3'],
        )
        #
        # If there is an actual common prefix, we return just it, so that readline
        # will insert it into place
        matches = compl.attr_matches('A.ab')
        self.assertEqual(matches, ['A.abc_'])
        #
        # Finally, at the next tab, we display again all the completions
        # available for this common prefix.
        matches = compl.attr_matches('A.abc_')
        self.assertEqual(
            sorted(matches),
            ['A.abc_1', 'A.abc_2', 'A.abc_3'],
        )

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
        self.assertEqual(matches, ['a.hello', 'a.world'])
        self.assertIs(type(matches[0]), str)


if __name__ == "__main__":
    unittest.main()
