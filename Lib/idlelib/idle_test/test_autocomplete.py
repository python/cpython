"Test autocomplete, coverage 93%."

import itertools
import unittest
from unittest.mock import Mock, patch, DEFAULT
from test.support import requires
from tkinter import Tk, Text
import os
import __main__

import idlelib.autocomplete as ac
import idlelib.autocomplete_w as acw
from idlelib.idle_test.mock_idle import Func
import idlelib.idle_test.mock_tk as mock_tk


class DummyEditwin:
    def __init__(self, root, text):
        self.root = root
        self.text = text
        self.indentwidth = 8
        self.tabwidth = 8
        self.prompt_last_line = '>>>'  # Currently not used by autocomplete.


class AutoCompleteTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.text = Text(cls.root)
        cls.editor = DummyEditwin(cls.root, cls.text)

    @classmethod
    def tearDownClass(cls):
        del cls.editor, cls.text
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.text.delete('1.0', 'end')
        self.autocomplete = ac.AutoComplete(self.editor)

    def test_init(self):
        self.assertEqual(self.autocomplete.editwin, self.editor)
        self.assertEqual(self.autocomplete.text, self.text)

    def test_make_autocomplete_window(self):
        testwin = self.autocomplete._make_autocomplete_window()
        self.assertIsInstance(testwin, acw.AutoCompleteWindow)

    def test_remove_autocomplete_window(self):
        acp = self.autocomplete
        acp.autocompletewindow = m = Mock()
        acp._remove_autocomplete_window()
        m.hide_window.assert_called_once()
        self.assertIsNone(acp.autocompletewindow)

    def test_force_open_completions_event(self):
        # Call _open_completions and break.
        acp = self.autocomplete
        open_c = Func()
        acp.open_completions = open_c
        self.assertEqual(acp.force_open_completions_event('event'), 'break')
        self.assertEqual(open_c.args[0], ac.FORCE)

    def test_autocomplete_event(self):
        Equal = self.assertEqual
        acp = self.autocomplete

        # Result of autocomplete event: If modified tab, None.
        ev = mock_tk.Event(mc_state=True)
        self.assertIsNone(acp.autocomplete_event(ev))
        del ev.mc_state

        # If tab after whitespace, None.
        self.text.insert('1.0', '        """Docstring.\n    ')
        self.assertIsNone(acp.autocomplete_event(ev))
        self.text.delete('1.0', 'end')

        # If active autocomplete window, complete() and 'break'.
        self.text.insert('1.0', 're.')
        acp.autocompletewindow = mock = Mock()
        mock.is_active = Mock(return_value=True)
        Equal(acp.autocomplete_event(ev), 'break')
        mock.complete.assert_called_once()
        acp.autocompletewindow = None

        # If no active autocomplete window, open_completions(), None/break.
        open_c = Func(result=False)
        acp.open_completions = open_c
        Equal(acp.autocomplete_event(ev), None)
        Equal(open_c.args[0], ac.TAB)
        open_c.result = True
        Equal(acp.autocomplete_event(ev), 'break')
        Equal(open_c.args[0], ac.TAB)

    def test_try_open_completions_event(self):
        Equal = self.assertEqual
        text = self.text
        acp = self.autocomplete
        trycompletions = acp.try_open_completions_event
        after = Func(result='after1')
        acp.text.after = after

        # If no text or trigger, after not called.
        trycompletions()
        Equal(after.called, 0)
        text.insert('1.0', 're')
        trycompletions()
        Equal(after.called, 0)

        # Attribute needed, no existing callback.
        text.insert('insert', ' re.')
        acp._delayed_completion_id = None
        trycompletions()
        Equal(acp._delayed_completion_index, text.index('insert'))
        Equal(after.args,
              (acp.popupwait, acp._delayed_open_completions, ac.TRY_A))
        cb1 = acp._delayed_completion_id
        Equal(cb1, 'after1')

        # File needed, existing callback cancelled.
        text.insert('insert', ' "./Lib/')
        after.result = 'after2'
        cancel = Func()
        acp.text.after_cancel = cancel
        trycompletions()
        Equal(acp._delayed_completion_index, text.index('insert'))
        Equal(cancel.args, (cb1,))
        Equal(after.args,
              (acp.popupwait, acp._delayed_open_completions, ac.TRY_F))
        Equal(acp._delayed_completion_id, 'after2')

    def test_delayed_open_completions(self):
        Equal = self.assertEqual
        acp = self.autocomplete
        open_c = Func()
        acp.open_completions = open_c
        self.text.insert('1.0', '"dict.')

        # Set autocomplete._delayed_completion_id to None.
        # Text index changed, don't call open_completions.
        acp._delayed_completion_id = 'after'
        acp._delayed_completion_index = self.text.index('insert+1c')
        acp._delayed_open_completions('dummy')
        self.assertIsNone(acp._delayed_completion_id)
        Equal(open_c.called, 0)

        # Text index unchanged, call open_completions.
        acp._delayed_completion_index = self.text.index('insert')
        acp._delayed_open_completions((1, 2, 3, ac.FILES))
        self.assertEqual(open_c.args[0], (1, 2, 3, ac.FILES))

    def test_oc_cancel_comment(self):
        none = self.assertIsNone
        acp = self.autocomplete

        # Comment is in neither code or string.
        acp._delayed_completion_id = 'after'
        after = Func(result='after')
        acp.text.after_cancel = after
        self.text.insert('1.0', '# comment')
        none(acp.open_completions(ac.TAB))  # From 'else' after 'elif'.
        none(acp._delayed_completion_id)


class FetchCompletionsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Mock()
        cls.text = mock_tk.Text(cls.root)
        cls.editor = DummyEditwin(cls.root, cls.text)

    def setUp(self):
        self.autocomplete = ac.AutoComplete(self.editor)

    def test_attrs(self):
        # Test that fetch_completions returns 2 lists:
        # 1. a small list containing all non-private variables
        # 2. a big list containing all variables
        acp = self.autocomplete

        # Test attributes
        s, b = acp.fetch_completions('', ac.ATTRS)
        self.assertLess(set(s), set(b))
        self.assertTrue(all([not x.startswith('_') for x in s]))

        if __main__.__file__ != ac.__file__:
            self.assertNotIn('AutoComplete', s)  # See issue 36405.

        # Test smalll should respect __all__.
        with patch.dict('__main__.__dict__', {'__all__': ['a', 'b']}):
            s, b = acp.fetch_completions('', ac.ATTRS)
            self.assertEqual(s, ['a', 'b'])
            self.assertIn('__name__', b)    # From __main__.__dict__
            self.assertIn('sum', b)         # From __main__.__builtins__.__dict__

        # Test attributes with name entity.
        mock = Mock()
        mock._private = Mock()
        with patch.dict('__main__.__dict__', {'foo': mock}):
            s, b = acp.fetch_completions('foo', ac.ATTRS)
            self.assertNotIn('_private', s)
            self.assertIn('_private', b)
            self.assertEqual(s, [i for i in sorted(dir(mock)) if i[0] != '_'])
            self.assertEqual(b, sorted(dir(mock)))

    def test_files(self):
        # Test that fetch_completions returns 2 lists:
        # 1. a small list containing files that do not start with '.'.
        # 2. a big list containing all files in the path
        acp = self.autocomplete

        def _listdir(path):
            # This will be patch and used in fetch_completions.
            if path == '.':
                return ['foo', 'bar', '.hidden']
            return ['monty', 'python', '.hidden']

        with patch.object(os, 'listdir', _listdir):
            s, b = acp.fetch_completions('', ac.FILES)
            self.assertEqual(s, ['bar', 'foo'])
            self.assertEqual(b, ['.hidden', 'bar', 'foo'])

            s, b = acp.fetch_completions('~', ac.FILES)
            self.assertEqual(s, ['monty', 'python'])
            self.assertEqual(b, ['.hidden', 'monty', 'python'])

    def test_dict_keys(self):
        # Test that fetch_completions returns 2 identical lists, containing all
        # keys of the dict of type str or bytes.
        acp = self.autocomplete

        # Test attributes with name entity.
        mock = Mock()
        mock._private = Mock()
        test_dict = {'one': 1, b'two': 2, 3: 3}
        with patch.dict('__main__.__dict__', {'test_dict': test_dict}):
            s, b = acp.fetch_completions('test_dict', ac.DICTKEYS)
        self.assertEqual(s, b)
        self.assertEqual(s, ['one', b'two'])

    def test_get_entity(self):
        # Test that a name is in the namespace of sys.modules and
        # __main__.__dict__.
        acp = self.autocomplete

        self.assertEqual(acp.get_entity('int'), int)

        # Test name from sys.modules.
        mock = Mock()
        with patch.dict('sys.modules', {'tempfile': mock}):
            self.assertEqual(acp.get_entity('tempfile'), mock)

        # Test name from __main__.__dict__.
        di = {'foo': 10, 'bar': 20}
        with patch.dict('__main__.__dict__', {'d': di}):
            self.assertEqual(acp.get_entity('d'), di)

        # Test name not in namespace.
        with patch.dict('__main__.__dict__', {}):
            with self.assertRaises(NameError):
                acp.get_entity('doesnt_exist')


class OpenCompletionsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Mock()
        cls.text = mock_tk.Text(cls.root)
        cls.editor = DummyEditwin(cls.root, cls.text)

    def setUp(self):
        self.autocomplete = ac.AutoComplete(self.editor)
        self.mock_acw = None

    def tearDown(self):
        self.text.delete('1.0', 'end')

    def make_acw(self):
        self.mock_acw = Mock()
        self.mock_acw.show_window = Mock(
            return_value=False, spec=acw.AutoCompleteWindow.show_window)
        return self.mock_acw

    def test_open_completions_files(self):
        acp = self.autocomplete

        self.text.insert('1.0', 'int.')
        with patch.object(acp, '_make_autocomplete_window', self.make_acw):
            acp.open_completions(ac.TAB)
        mock_acw = self.mock_acw

        self.assertIs(acp.autocompletewindow, mock_acw)
        mock_acw.show_window.assert_called_once()
        comp_lists, index, complete, mode, userWantsWin = \
            mock_acw.show_window.call_args[0]
        self.assertEqual(mode, ac.ATTRS)
        self.assertIn('bit_length', comp_lists[0])
        self.assertIn('bit_length', comp_lists[1])
        self.assertNotIn('__index__', comp_lists[0])
        self.assertIn('__index__', comp_lists[1])

    def test_open_completions_attrs(self):
        acp = self.autocomplete

        self.text.insert('1.0', '"t')
        def _listdir(path):
            return ['.hidden', 'monty', 'python']
        with patch.object(acp, '_make_autocomplete_window', self.make_acw):
            with patch('os.listdir', _listdir):
                acp.open_completions(ac.TAB)
        mock_acw = self.mock_acw

        mock_acw.show_window.assert_called_once()
        comp_lists, index, complete, mode, userWantsWin = \
            mock_acw.show_window.call_args[0]
        self.assertEqual(mode, ac.FILES)
        self.assertEqual(comp_lists[0], ['monty', 'python'])
        self.assertEqual(comp_lists[1], ['.hidden', 'monty', 'python'])

    def test_open_completions_dict_keys_only_opening_quote(self):
        # Note that dict key completion lists also include variables from
        # the global namespace.
        acp = self.autocomplete

        test_dict = {'one': 1, b'two': 2, 3: 3}

        for quote in ['"', "'", '"""', "'''"]:
            with self.subTest(quote=quote):
                self.text.insert('1.0', f'test_dict[{quote}')
                with patch.object(acp, '_make_autocomplete_window',
                                  self.make_acw):
                    with patch.dict('__main__.__dict__',
                                    {'test_dict': test_dict}):
                        acp.open_completions(ac.TAB)
                mock_acw = self.mock_acw

                mock_acw.show_window.assert_called_once()
                comp_lists, index, complete, mode, userWantsWin = \
                    mock_acw.show_window.call_args[0]
                self.assertEqual(mode, ac.DICTKEYS)
                self.assertLess(set(comp_lists[0]), set(comp_lists[1]))
                expected = [f'{quote}one{quote}', f'b{quote}two{quote}']
                self.assertLess(set(expected), set(comp_lists[0]))
                self.assertLess(set(expected), set(comp_lists[1]))
                self.text.delete('1.0', 'end')

    def test_open_completions_dict_keys_no_opening_quote(self):
        # Note that dict key completion lists also include variables from
        # the global namespace.
        acp = self.autocomplete

        test_dict = {'one': 1, b'two': 2, 3: 3}

        self.text.insert('1.0', f'test_dict[')
        with patch.object(acp, '_make_autocomplete_window',
                          self.make_acw):
            with patch.dict('__main__.__dict__',
                            {'test_dict': test_dict}):
                acp.open_completions(ac.TAB)
        mock_acw = self.mock_acw

        mock_acw.show_window.assert_called_once()
        comp_lists, index, complete, mode, userWantsWin = \
            mock_acw.show_window.call_args[0]
        self.assertEqual(mode, ac.DICTKEYS)
        self.assertLess(set(comp_lists[0]), set(comp_lists[1]))
        expected = [f'"one"', f'b"two"']
        self.assertLess(set(expected), set(comp_lists[0]))
        self.assertLess(set(expected), set(comp_lists[1]))
        self.text.delete('1.0', 'end')

    def test_no_list(self):
        acp = self.autocomplete
        fetch = Func(result=([],[]))
        acp.fetch_completions = fetch
        none = self.assertIsNone
        oc = acp.open_completions
        self.text.insert('1.0', 'object')
        none(oc(ac.TAB))
        self.text.insert('insert', '.')
        none(oc(ac.TAB))
        self.assertEqual(fetch.called, 2)

    def test_none(self):
        # Test other two None returns.
        none = self.assertIsNone
        acp = self.autocomplete
        oc = acp.open_completions

        # No object for attributes or need call not allowed.
        self.text.insert('1.0', '.')
        none(oc(ac.TAB))
        self.text.insert('insert', ' int().')
        none(oc(ac.TAB))

        # Blank or quote trigger 'if complete ...'.
        self.text.delete('1.0', 'end')
        self.assertFalse(oc(ac.TAB))
        self.text.insert('1.0', '"')
        self.assertFalse(oc(ac.TAB))
        self.text.delete('1.0', 'end')


class ShowWindowTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Mock()
        cls.text = mock_tk.Text(cls.root)
        cls.editor = DummyEditwin(cls.root, cls.text)

    def setUp(self):
        self.autocomplete = ac.AutoComplete(self.editor)

        patcher = patch.multiple(acw,
                                 Toplevel=DEFAULT,
                                 Scrollbar=DEFAULT,
                                 Listbox=mock_tk.Listbox)
        patcher.start()
        self.addCleanup(patcher.stop)

    def test_complete_dict_key_single_match(self):
        acp = self.autocomplete

        test_dict = {'one': 1, b'two': 2, 3: 3}

        for quote, args in itertools.product(
                ['"', "'", '"""', "'''"],
                [ac.TAB, ac.FORCE]
        ):
            for text in [f'test_dict[{quote}', f'test_dict[{quote}o']:
                with self.subTest(quote=quote, args=args, text=text):
                    self.text.delete('1.0', 'end')
                    self.text.insert('1.0', text)
                    with patch.dict('__main__.__dict__',
                                    {'test_dict': test_dict}):
                        result = acp.open_completions(args)

                    self.assertEqual(result, True)
                    # With one possible completion, the text should be updated
                    # only if the 'complete' flag is true.
                    expected_text = \
                        text if not args[1] else f'test_dict[{quote}one{quote}]'
                    self.assertEqual(self.text.get('1.0', '1.end'),
                                     expected_text)

    def test_complete_dict_key_multiple_matches(self):
        acp = self.autocomplete

        test_dict = {'twelve': 12, 'two': 2}

        for quote, args in itertools.product(
                ['"', "'", '"""', "'''"],
                [ac.TAB, ac.FORCE]
        ):
            for text in [f'test_dict[{quote}', f'test_dict[{quote}t']:
                with self.subTest(quote=quote, text=text, args=args):
                    self.text.delete('1.0', 'end')
                    self.text.insert('1.0', text)
                    with patch.dict('__main__.__dict__',
                                    {'test_dict': test_dict}):
                        result = acp.open_completions(args)

                    self.assertEqual(result, True)
                    # With more than one possible completion, the text
                    # should not be changed.
                    self.assertEqual(self.text.get('1.0', '1.end'), text)

    def test_complete_dict_key_no_matches(self):
        acp = self.autocomplete

        test_dict = {'one': 12, 'two': 2}

        for quote, args in itertools.product(
                ['"', "'", '"""', "'''"],
                [ac.TAB, ac.FORCE]
        ):
            for text in [f'test_dict[b{quote}', f'test_dict[b{quote}t']:
                with self.subTest(quote=quote, text=text, args=args):
                    self.text.delete('1.0', 'end')
                    self.text.insert('1.0', text)
                    with patch.dict('__main__.__dict__',
                                    {'test_dict': test_dict}):
                        result = acp.open_completions(args)

                    self.assertEqual(result, True)
                    # With no possible completions, the text
                    # should not be changed.
                    self.assertEqual(self.text.get('1.0', '1.end'), text)


class TestQuoteClosesLiteral(unittest.TestCase):
    def check(self, start, quotechar):
        return acw.AutoCompleteWindow._quote_closes_literal(start, quotechar)
        
    def test_true_cases(self):
        true_cases = [
            # (start, quotechar)
            (f'{prefix}{quote}{content}{quote[:-1]}', quote[0])
            for prefix in ('', 'b', 'rb', 'u')
            for quote in ('"', "'", '"""', "'''")
            for content in ('', 'a', 'abc', '\\'*2, f'\\{quote[0]}', '\\n')
        ]
        
        for (start, quotechar) in true_cases:
            with self.subTest(start=start, quotechar=quotechar):
                self.assertTrue(self.check(start, quotechar))

    def test_false_cases(self):
        false_cases = [
            # (start, quotechar)
            (f'{prefix}{quote}{content}', quote[0])
            for prefix in ('', 'b', 'rb', 'u')
            for quote in ('"', "'", '"""', "'''")
            for content in ('\\', 'a\\', 'ab\\', '\\'*3, '\\'*5, 'ab\\\\\\')
        ]

        # test different quote char
        false_cases.extend([
            ('"', "'"),
            ('"abc', "'"),
            ('"""', "'"),
            ('"""abc', "'"),
            ("'", '"'),
            ("'abc", '"'),
            ("'''", '"'),
            ("'''abc", '"'),
        ])

        # test not yet closed triple-quotes
        for q in ['"', "'"]:
            false_cases.extend([
                (f"{q*3}", q),
                (f"{q*4}", q),
                (f"{q*3}\\{q}", q),
                (f"{q*3}\\{q*2}", q),
                (f"{q*3}\\\\\\{q*2}", q),
                (f"{q*3}abc", q),
                (f"{q*3}abc{q}", q),
                (f"{q*3}abc\\{q}", q),
                (f"{q*3}abc\\{q*2}", q),
                (f"{q*3}abc\\\\\\{q*2}", q),
            ])

        for (start, quotechar) in false_cases:
            with self.subTest(start=start, quotechar=quotechar):
                self.assertFalse(self.check(start, quotechar))


class TestDictKeyReprs(unittest.TestCase):
    def call(self, comp_start, comp_list):
        return ac.AutoComplete._dict_key_reprs(comp_start, comp_list)

    def check(self, comp_start, comp_list, expected):
        self.assertEqual(self.call(comp_start, comp_list), expected)

    def test_empty_strings(self):
        self.check('', [''], ['""'])
        self.check('', [b''], ['b""'])
        self.check('', ['', b''], ['""', 'b""'])

    def test_empty_strings_with_only_prefix(self):
        self.check('b', [''], ['""'])
        self.check('b', [b''], ['b""'])

        self.check('r', [''], ['r""'])
        self.check('rb', [b''], ['rb""'])
        self.check('br', [b''], ['br""'])
        self.check('rb', [''], ['r""'])
        self.check('br', [''], ['r""'])

        self.check('u', [''], ['u""'])
        self.check('u', [b''], ['b""'])

    def test_empty_strings_with_only_quotes(self):
        self.check('"', [''], ['""'])
        self.check("'", [''], ["''"])
        self.check('"""', [''], ['""""""'])
        self.check("'''", [''], ["''''''"])

        self.check('"', [b''], ['b""'])
        self.check("'", [b''], ["b''"])
        self.check('"""', [b''], ['b""""""'])
        self.check("'''", [b''], ["b''''''"])

    def test_backslash_escape(self):
        self.check('"', ['ab\\c'], [r'"ab\\c"'])
        self.check('"', ['ab\\\\c'], [r'"ab\\\\c"'])
        self.check('"', ['ab\\nc'], [r'"ab\\nc"'])

    def test_quote_escape(self):
        self.check('"', ['"', "'"], [r'"\""', '"\'"'])
        self.check("'", ['"', "'"], ["'\"'", r"'\''"])
        self.check('"""', ['"', "'"], [r'"""\""""', '"""\'"""'])
        self.check("'''", ['"', "'"], ["'''\"'''", r"'''\''''"])

        # With triple quotes, only a final single quote should be escaped.
        self.check('"""', ['"ab'], ['""""ab"""'])
        self.check('"""', ['a"b'], ['"""a"b"""'])
        self.check('"""', ['ab"'], ['"""ab\\""""'])
        self.check('"""', ['ab""'], ['"""ab"\\""""'])

        self.check("'''", ["'ab"], ["''''ab'''"])
        self.check("'''", ["a'b"], ["'''a'b'''"])
        self.check("'''", ["ab'"], ["'''ab\\''''"])
        self.check("'''", ["ab''"], ["'''ab'\\''''"])

        # Identical single quotes should be escaped.
        self.check('"', ['ab"c'], [r'"ab\"c"'])
        self.check("'", ["ab'c"], [r"'ab\'c'"])

        # Different single quotes shouldn't be escaped.
        self.check('"', ["ab'c"], ['"ab\'c"'])
        self.check("'", ['ab"c'], ["'ab\"c'"])

    def test_control_char_escape(self):
        custom_escapes = {0: '\\0', 9: '\\t', 10: '\\n', 13: '\\r'}
        for ord_ in range(14):
            with self.subTest(ord_=ord_):
                escape = custom_escapes.get(ord_, f'\\x{ord_:02x}')
                self.check('"', [chr(ord_), bytes([ord_])],
                           [f'"{escape}"', f'b"{escape}"'])

    def test_high_bytes_value_escape(self):
        self.check('"', [b'\x80'], [r'b"\x80"'])
        self.check('"', [b'\xc3'], [r'b"\xc3"'])
        self.check('"', [b'\xff'], [r'b"\xff"'])
        self.check('"', [b'\x80\xc3\xff'], [r'b"\x80\xc3\xff"'])

        # Such characters should only be escaped in bytes, not str.
        self.check('"', ['\x80\xc3\xff'], ['"\x80\xc3\xff"'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
