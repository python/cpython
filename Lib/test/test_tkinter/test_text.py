import unittest
import tkinter
from tkinter import TclError
from test.support import requires
from test.test_tkinter.support import setUpModule  # noqa: F401
from test.test_tkinter.support import AbstractTkTest, requires_tk

requires('gui')

class TextTest(AbstractTkTest, unittest.TestCase):

    def setUp(self):
        super().setUp()
        self.text = tkinter.Text(self.root)
        self.text.pack()

    def test_debug(self):
        text = self.text
        olddebug = text.debug()
        try:
            text.debug(0)
            self.assertEqual(text.debug(), 0)
            text.debug(1)
            self.assertEqual(text.debug(), 1)
        finally:
            text.debug(olddebug)
            self.assertEqual(text.debug(), olddebug)

    def test_sync(self):
        text = self.text
        # sync() returns None and brings line metrics up to date.
        self.assertIsNone(text.sync())
        self.assertIs(text.pendingsync(), False)

        # sync(command) schedules a one-shot callback.
        events = []
        text.sync(command=lambda: events.append('synced'))
        text.update()
        self.assertEqual(events, ['synced'])

    def test_index(self):
        text = self.text
        text.insert('1.0', 'Lorem ipsum\ndolor sit amet')
        self.assertEqual(text.index('1.0'), '1.0')
        self.assertEqual(text.index('1.end'), '1.11')
        self.assertEqual(text.index('end'), '3.0')
        self.assertEqual(text.index('2.5'), '2.5')
        # Index past the end of a line is clamped to its end.
        self.assertEqual(text.index('1.100'), '1.11')
        self.assertRaises(TclError, text.index, 'invalid')
        self.assertRaises(TypeError, text.index)
        self.assertRaises(TypeError, text.index, '1.0', '2.5')

    def test_compare(self):
        text = self.text
        text.insert('1.0', 'Lorem ipsum\ndolor sit amet')
        self.assertIs(text.compare('1.0', '<', '2.0'), True)
        self.assertIs(text.compare('2.0', '<', '1.0'), False)
        self.assertIs(text.compare('1.5', '==', '1.5'), True)
        self.assertIs(text.compare('1.5', '!=', '1.5'), False)
        self.assertIs(text.compare('2.0', '>=', '2.0'), True)
        self.assertIs(text.compare('1.0', '<=', 'end'), True)
        self.assertRaises(TclError, text.compare, '1.0', 'invalid', '2.0')
        self.assertRaises(TypeError, text.compare, '1.0', '<')
        self.assertRaises(TypeError, text.compare, '1.0', '<', '2.0', '3.0')

    def test_insert_get_delete(self):
        text = self.text
        text.insert('1.0', 'Lorem ipsum')
        self.assertEqual(text.get('1.0', 'end'), 'Lorem ipsum\n')
        self.assertEqual(text.get('1.0', '1.5'), 'Lorem')
        self.assertEqual(text.get('1.6'), 'i')  # single character

        # insert before an existing index
        text.insert('1.0', '*** ')
        self.assertEqual(text.get('1.0', 'end'), '*** Lorem ipsum\n')

        text.delete('1.0', '1.4')
        self.assertEqual(text.get('1.0', 'end'), 'Lorem ipsum\n')
        text.delete('1.5')  # delete a single character
        self.assertEqual(text.get('1.0', 'end'), 'Loremipsum\n')
        self.assertRaises(TypeError, text.get)
        self.assertRaises(TypeError, text.get, '1.0', '1.5', 'end')
        self.assertRaises(TypeError, text.delete, '1.0', '1.5', 'end')

    def test_insert_with_tags(self):
        text = self.text
        text.insert('1.0', 'hello', 'a', ' ', ('a', 'b'), 'world', 'b')
        self.assertEqual(text.get('1.0', 'end'), 'hello world\n')
        self.assertEqual([str(i) for i in text.tag_ranges('a')],
                         ['1.0', '1.6'])
        self.assertEqual([str(i) for i in text.tag_ranges('b')],
                         ['1.5', '1.11'])

    def test_replace(self):
        text = self.text
        text.insert('1.0', 'Lorem ipsum')
        text.replace('1.0', '1.5', 'Hello')
        self.assertEqual(text.get('1.0', 'end'), 'Hello ipsum\n')
        text.replace('1.6', 'end - 1 char', 'world')
        self.assertEqual(text.get('1.0', 'end'), 'Hello world\n')
        self.assertRaises(TclError, text.replace, 'invalid', '1.5', 'x')
        # The first index must not be after the second.
        self.assertRaises(TclError, text.replace, '1.5', '1.0', 'x')
        self.assertRaises(TypeError, text.replace, '1.0', '1.5')

    def test_mark(self):
        text = self.text
        text.insert('1.0', 'Lorem ipsum\ndolor sit amet')
        # 'insert' and 'current' marks always exist.
        self.assertIn('insert', text.mark_names())
        self.assertIn('current', text.mark_names())

        text.mark_set('here', '1.5')
        self.assertIn('here', text.mark_names())
        self.assertEqual(text.index('here'), '1.5')
        text.mark_set('here', '2.3')
        self.assertEqual(text.index('here'), '2.3')

        text.mark_unset('here')
        self.assertNotIn('here', text.mark_names())

        # Unsetting a non-existent mark is not an error.
        text.mark_unset('nonexistent')
        self.assertRaises(TclError, text.mark_set, 'm', 'invalid')
        self.assertRaises(TypeError, text.mark_set, 'm')
        self.assertRaises(TypeError, text.mark_set, 'm', '1.0', '2.0')

    def test_mark_gravity(self):
        text = self.text
        text.insert('1.0', 'Lorem ipsum')
        text.mark_set('here', '1.5')
        # The default gravity is right.
        self.assertEqual(text.mark_gravity('here'), 'right')
        text.mark_gravity('here', 'left')
        self.assertEqual(text.mark_gravity('here'), 'left')
        # With left gravity the mark stays before inserted text.
        text.insert('here', 'XXX')
        self.assertEqual(text.index('here'), '1.5')
        # With right gravity the mark moves after inserted text.
        text.mark_gravity('here', 'right')
        text.insert('here', 'YYY')
        self.assertEqual(text.index('here'), '1.8')

        self.assertRaises(TclError, text.mark_gravity, 'nonexistent')
        self.assertRaises(TclError, text.mark_gravity, 'here', 'invalid')
        self.assertRaises(TypeError, text.mark_gravity)
        self.assertRaises(TypeError, text.mark_gravity, 'here', 'left', 'extra')

    def test_mark_next_previous(self):
        text = self.text
        text.insert('1.0', 'Lorem ipsum')
        # Keep the builtin 'insert' and 'current' marks at 1.0 so they do
        # not interfere with the queries below.
        text.mark_set('insert', '1.0')
        text.mark_set('current', '1.0')
        text.mark_set('m1', '1.3')
        text.mark_set('m2', '1.7')
        # mark_next finds a mark at a position at or after the index,
        # mark_previous finds one at a position strictly before it.
        self.assertEqual(text.mark_next('1.1'), 'm1')
        self.assertEqual(text.mark_next('1.3'), 'm1')
        self.assertEqual(text.mark_next('1.4'), 'm2')
        self.assertIsNone(text.mark_next('1.8'))
        self.assertEqual(text.mark_previous('1.4'), 'm1')
        self.assertEqual(text.mark_previous('1.7'), 'm1')
        self.assertEqual(text.mark_previous('end'), 'm2')
        self.assertIsNone(text.mark_previous('1.0'))

        self.assertRaises(TclError, text.mark_next, 'invalid')
        self.assertRaises(TclError, text.mark_previous, 'invalid')
        self.assertRaises(TypeError, text.mark_next)
        self.assertRaises(TypeError, text.mark_previous)
        self.assertRaises(TypeError, text.mark_next, '1.0', '2.0')
        self.assertRaises(TypeError, text.mark_previous, '1.0', '2.0')

    def test_tag_add_remove_ranges(self):
        text = self.text
        text.insert('1.0', 'Lorem ipsum\ndolor sit amet')
        self.assertEqual(text.tag_ranges('sel'), ())

        text.tag_add('big', '1.0', '1.5')
        self.assertEqual([str(i) for i in text.tag_ranges('big')],
                         ['1.0', '1.5'])
        # Add a second, disjoint range.
        text.tag_add('big', '2.0', '2.5')
        self.assertEqual([str(i) for i in text.tag_ranges('big')],
                         ['1.0', '1.5', '2.0', '2.5'])

        text.tag_remove('big', '1.0', '1.3')
        self.assertEqual([str(i) for i in text.tag_ranges('big')],
                         ['1.3', '1.5', '2.0', '2.5'])

        # tag_ranges of an undefined tag is empty, not an error.
        self.assertEqual(text.tag_ranges('nonexistent'), ())
        self.assertRaises(TclError, text.tag_add, 'big', 'invalid')
        self.assertRaises(TypeError, text.tag_add, 'big')
        self.assertRaises(TclError, text.tag_remove, 'big', 'invalid')
        self.assertRaises(TypeError, text.tag_remove, 'big', '1.0', '2.0', '3.0')
        self.assertRaises(TypeError, text.tag_ranges, 'big', 'extra')

    def test_tag_names(self):
        text = self.text
        text.insert('1.0', 'Lorem ipsum')
        text.tag_add('a', '1.0', '1.5')
        text.tag_add('b', '1.3', '1.8')
        # 'sel' always exists; order reflects priority (creation order).
        self.assertEqual(set(text.tag_names()), {'sel', 'a', 'b'})
        # Tags applied to the character at the given index.
        self.assertEqual(set(text.tag_names('1.4')), {'a', 'b'})
        self.assertEqual(set(text.tag_names('1.0')), {'a'})
        self.assertEqual(set(text.tag_names('1.10')), set())
        self.assertRaises(TclError, text.tag_names, 'invalid')
        self.assertRaises(TypeError, text.tag_names, '1.0', '2.0')

    def test_tag_nextrange_prevrange(self):
        text = self.text
        text.insert('1.0', 'Lorem ipsum dolor')
        text.tag_add('a', '1.0', '1.5')
        text.tag_add('a', '1.12', '1.17')

        self.assertEqual([str(i) for i in text.tag_nextrange('a', '1.0')],
                         ['1.0', '1.5'])
        self.assertEqual([str(i) for i in text.tag_nextrange('a', '1.5')],
                         ['1.12', '1.17'])
        self.assertEqual(text.tag_nextrange('a', '1.17'), ())

        self.assertEqual([str(i) for i in text.tag_prevrange('a', 'end')],
                         ['1.12', '1.17'])
        self.assertEqual([str(i) for i in text.tag_prevrange('a', '1.12')],
                         ['1.0', '1.5'])
        self.assertEqual(text.tag_prevrange('a', '1.0'), ())

        # An undefined tag has no ranges, but the index must be valid.
        self.assertEqual(text.tag_nextrange('nonexistent', '1.0'), ())
        self.assertRaises(TclError, text.tag_nextrange, 'a', 'invalid')
        self.assertRaises(TclError, text.tag_prevrange, 'a', 'invalid')
        self.assertRaises(TypeError, text.tag_nextrange, 'a')
        self.assertRaises(TypeError, text.tag_prevrange, 'a')
        self.assertRaises(TypeError, text.tag_nextrange, 'a', '1.0', '2.0', '3.0')
        self.assertRaises(TypeError, text.tag_prevrange, 'a', '1.0', '2.0', '3.0')

    def test_tag_delete(self):
        text = self.text
        text.insert('1.0', 'Lorem ipsum')
        text.tag_add('a', '1.0', '1.5')
        text.tag_add('b', '1.6', '1.11')
        self.assertEqual(set(text.tag_names()), {'sel', 'a', 'b'})
        text.tag_delete('a', 'b')
        self.assertEqual(set(text.tag_names()), {'sel'})
        self.assertEqual(text.tag_ranges('a'), ())

    def test_tag_raise_lower(self):
        text = self.text
        text.tag_configure('a', foreground='red')
        text.tag_configure('b', foreground='green')
        text.tag_configure('c', foreground='blue')
        # Creation order is lowest-to-highest priority; tag_names lists
        # them in increasing priority order.
        self.assertEqual(text.tag_names(), ('sel', 'a', 'b', 'c'))
        text.tag_raise('a')
        self.assertEqual(text.tag_names(), ('sel', 'b', 'c', 'a'))
        text.tag_lower('a')
        self.assertEqual(text.tag_names(), ('a', 'sel', 'b', 'c'))
        text.tag_raise('a', 'b')
        self.assertEqual(text.tag_names(), ('sel', 'b', 'a', 'c'))
        text.tag_lower('c', 'b')
        self.assertEqual(text.tag_names(), ('sel', 'c', 'b', 'a'))

        self.assertRaises(TclError, text.tag_raise, 'nonexistent')
        self.assertRaises(TclError, text.tag_lower, 'nonexistent')
        self.assertRaises(TclError, text.tag_raise, 'a', 'nonexistent')
        self.assertRaises(TypeError, text.tag_raise)
        self.assertRaises(TypeError, text.tag_raise, 'a', 'b', 'c')
        self.assertRaises(TypeError, text.tag_lower, 'a', 'b', 'c')

    def test_tag_cget_configure(self):
        text = self.text
        text.tag_configure('a', foreground='red', underline=True)
        self.assertEqual(str(text.tag_cget('a', 'foreground')), 'red')
        self.assertIn(text.tag_cget('a', 'underline'), (1, '1', True))
        # tag_cget normalizes the option name (no leading hyphen needed).
        self.assertEqual(str(text.tag_cget('a', '-foreground')), 'red')
        # configure() query returns the full option spec.
        self.assertEqual(text.tag_configure('a', 'foreground')[-1], 'red')
        text.tag_configure('a', foreground='blue')
        self.assertEqual(str(text.tag_cget('a', 'foreground')), 'blue')

        self.assertRaises(TclError, text.tag_cget, 'nonexistent',
                          'foreground')
        self.assertRaises(TypeError, text.tag_cget, 'a')
        self.assertRaises(TypeError, text.tag_cget, 'a', 'foreground', 'extra')

    def test_edit_modified(self):
        text = self.text
        self.assertEqual(text.edit_modified(), 0)
        text.insert('1.0', 'spam')
        self.assertEqual(text.edit_modified(), 1)
        text.edit_modified(False)
        self.assertEqual(text.edit_modified(), 0)
        text.edit_modified(True)
        self.assertEqual(text.edit_modified(), 1)

    def test_edit_undo_redo(self):
        text = self.text
        text.configure(undo=True)
        text.insert('1.0', 'spam')
        text.edit_separator()
        text.insert('end', ' eggs')
        self.assertEqual(text.get('1.0', 'end'), 'spam eggs\n')

        text.edit_undo()
        self.assertEqual(text.get('1.0', 'end'), 'spam\n')
        text.edit_redo()
        self.assertEqual(text.get('1.0', 'end'), 'spam eggs\n')

        # An empty undo stack raises.
        text.edit_reset()
        self.assertRaises(TclError, text.edit_undo)

    def test_edit_canundo_canredo(self):
        text = self.text
        text.configure(undo=True)

        self.assertIs(text.edit_canundo(), False)
        self.assertIs(text.edit_canredo(), False)

        text.insert('1.0', 'spam')
        self.assertIs(text.edit_canundo(), True)
        self.assertIs(text.edit_canredo(), False)

        text.edit_undo()
        self.assertIs(text.edit_canundo(), False)
        self.assertIs(text.edit_canredo(), True)

        text.edit_redo()
        self.assertIs(text.edit_canundo(), True)
        self.assertIs(text.edit_canredo(), False)

        text.edit_reset()
        self.assertIs(text.edit_canundo(), False)
        self.assertIs(text.edit_canredo(), False)

    def test_dump(self):
        text = self.text
        text.insert('1.0', 'hello')
        text.tag_add('a', '1.0', '1.3')
        text.mark_set('here', '1.2')

        dump = text.dump('1.0', 'end')
        self.assertIsInstance(dump, list)
        for item in dump:
            self.assertIsInstance(item, tuple)
            self.assertEqual(len(item), 3)
        kinds = {item[0] for item in dump}
        self.assertIn('text', kinds)
        self.assertIn('tagon', kinds)
        self.assertIn('mark', kinds)

        # Filtering by kind.
        text_only = text.dump('1.0', 'end', text=True)
        self.assertEqual({item[0] for item in text_only}, {'text'})
        self.assertEqual(''.join(item[1] for item in text_only), 'hello\n')

        # The command callback receives each triple and suppresses the result.
        collected = []
        result = text.dump('1.0', 'end', text=True,
                           command=lambda *a: collected.append(a))
        self.assertIsNone(result)
        self.assertEqual(''.join(key_value_index[1]
                                 for key_value_index in collected), 'hello\n')

        self.assertRaises(TclError, text.dump, 'invalid')
        self.assertRaises(TypeError, text.dump)
        self.assertRaises(TypeError, text.dump, '1.0', 'end', None, 'extra')

    def test_image(self):
        text = self.text
        image = tkinter.PhotoImage(master=self.root, width=10, height=10)
        text.insert('1.0', 'AB')
        name = text.image_create('1.1', image=image)
        self.assertIsInstance(name, str)
        self.assertIn(name, self.root.splitlist(text.image_names()))
        self.assertEqual(str(text.image_cget(name, 'image')), str(image))

        text.image_configure(name, align='top')
        self.assertEqual(str(text.image_cget(name, 'align')), 'top')

        # An embedded image occupies a single index position.
        self.assertEqual(text.index('end - 1 char'), '1.3')

        # Either a name or an image is required, and the index must be valid.
        self.assertRaises(TclError, text.image_create, '1.0')
        self.assertRaises(TclError, text.image_create, 'invalid',
                          image=image)
        self.assertRaises(TypeError, text.image_cget, name)
        self.assertRaises(TypeError, text.image_cget, name, 'image', 'extra')

    def test_window(self):
        text = self.text
        button = tkinter.Button(text, text='ok')
        text.insert('1.0', 'AB')
        text.window_create('1.1', window=button)
        self.assertEqual(self.root.splitlist(text.window_names()),
                         (str(button),))
        self.assertEqual(text.window_cget('1.1', 'window'), str(button))

        text.window_configure('1.1', padx=5)
        self.assertEqual(text.window_cget('1.1', 'padx'), 5)

        # No embedded window at this index, and the index must be valid.
        self.assertRaises(TclError, text.window_cget, '1.0', 'window')
        self.assertRaises(TclError, text.window_cget, 'invalid', 'window')
        self.assertRaises(TypeError, text.window_cget, '1.1')
        self.assertRaises(TypeError, text.window_cget, '1.1', 'window', 'extra')
        button.destroy()

    def test_tag_configure(self):
        text = self.text
        tag = 'a'
        getint = text.tk.getint
        getboolean = text.tk.getboolean

        # Color options.
        for opt in ('foreground', 'background'):
            text.tag_configure(tag, **{opt: '#ff0000'})
            self.assertEqual(str(text.tag_cget(tag, opt)), '#ff0000')

        # Stipple (bitmap) options.
        for opt in ('bgstipple', 'fgstipple'):
            text.tag_configure(tag, **{opt: 'gray50'})
            self.assertEqual(str(text.tag_cget(tag, opt)), 'gray50')

        # Enumerated options.
        for opt, value in (('justify', 'center'), ('wrap', 'word'),
                           ('relief', 'raised'), ('tabstyle', 'wordprocessor'),
                           ('offset', '5')):
            text.tag_configure(tag, **{opt: value})
        self.assertEqual(str(text.tag_cget(tag, 'justify')), 'center')
        self.assertEqual(str(text.tag_cget(tag, 'wrap')), 'word')
        self.assertEqual(str(text.tag_cget(tag, 'relief')), 'raised')
        self.assertEqual(str(text.tag_cget(tag, 'tabstyle')), 'wordprocessor')

        # Boolean options.
        for opt in ('underline', 'overstrike', 'elide'):
            text.tag_configure(tag, **{opt: True})
            self.assertIs(getboolean(text.tag_cget(tag, opt)), True)
            text.tag_configure(tag, **{opt: False})
            self.assertIs(getboolean(text.tag_cget(tag, opt)), False)

        # Screen-distance (pixel) options.
        for opt in ('borderwidth', 'lmargin1', 'lmargin2', 'rmargin',
                    'spacing1', 'spacing2', 'spacing3'):
            text.tag_configure(tag, **{opt: 7})
            self.assertEqual(getint(text.tag_cget(tag, opt)), 7)

        # Other options.
        text.tag_configure(tag, font='Helvetica 12')
        self.assertEqual(str(text.tag_cget(tag, 'font')), 'Helvetica 12')
        text.tag_configure(tag, tabs=(10.2, '1i'))
        self.assertEqual([str(x) for x in text.tag_ranges('sel')], [])

        self.assertRaises(TclError, text.tag_cget, tag, 'spam')

    @requires_tk(8, 6, 6)
    def test_tag_configure_colors(self):
        # Tag color options added in Tk 8.6.6.
        text = self.text
        tag = 'a'
        for opt in ('selectforeground', 'selectbackground',
                    'lmargincolor', 'rmargincolor',
                    'underlinefg', 'overstrikefg'):
            text.tag_configure(tag, **{opt: '#00ff00'})
            self.assertEqual(str(text.tag_cget(tag, opt)), '#00ff00')

    def test_tag_configure_query(self):
        text = self.text
        tag = 'a'
        # Querying all options returns a dict keyed by option name.
        cnf = text.tag_configure(tag)
        self.assertIsInstance(cnf, dict)
        self.assertIn('foreground', cnf)
        # The value is the full 5-tuple option specification.
        self.assertEqual(len(cnf['foreground']), 5)

        # Querying a single option returns its specification.
        spec = text.tag_configure(tag, 'foreground')
        self.assertEqual(spec[0], 'foreground')
        self.assertEqual(spec[-1], '')  # unset by default

        # Setting via keyword arguments and via a dict are equivalent.
        text.tag_configure(tag, foreground='red')
        self.assertEqual(str(text.tag_configure(tag, 'foreground')[-1]), 'red')
        text.tag_configure(tag, {'foreground': 'blue'})
        self.assertEqual(str(text.tag_cget(tag, 'foreground')), 'blue')

        # tag_config is an alias of tag_configure.
        self.assertEqual(text.tag_config, text.tag_configure)

    def test_image_configure(self):
        text = self.text
        image = tkinter.PhotoImage(master=self.root, width=10, height=10)
        text.insert('1.0', 'AB')
        name = text.image_create('1.1', image=image, name='img')
        self.assertEqual(name, 'img')

        self.assertEqual(str(text.image_cget(name, 'name')), 'img')
        self.assertEqual(str(text.image_cget(name, 'image')), str(image))
        for value in ('top', 'center', 'bottom', 'baseline'):
            text.image_configure(name, align=value)
            self.assertEqual(str(text.image_cget(name, 'align')), value)
        text.image_configure(name, padx=3, pady=4)
        self.assertEqual(text.tk.getint(text.image_cget(name, 'padx')), 3)
        self.assertEqual(text.tk.getint(text.image_cget(name, 'pady')), 4)

        # Querying returns the full option set.
        cnf = text.image_configure(name)
        self.assertIsInstance(cnf, dict)
        self.assertIn('align', cnf)
        self.assertRaises(TclError, text.image_cget, name, 'spam')

    def test_window_configure(self):
        text = self.text
        button = tkinter.Button(text, text='ok')
        text.insert('1.0', 'AB')
        text.window_create('1.1', window=button)

        self.assertEqual(text.window_cget('1.1', 'window'), str(button))
        for value in ('top', 'center', 'bottom', 'baseline'):
            text.window_configure('1.1', align=value)
            self.assertEqual(str(text.window_cget('1.1', 'align')), value)
        text.window_configure('1.1', padx=3, pady=4)
        self.assertEqual(text.tk.getint(text.window_cget('1.1', 'padx')), 3)
        self.assertEqual(text.tk.getint(text.window_cget('1.1', 'pady')), 4)
        text.window_configure('1.1', stretch=True)
        self.assertIs(text.tk.getboolean(text.window_cget('1.1', 'stretch')),
                      True)

        cnf = text.window_configure('1.1')
        self.assertIsInstance(cnf, dict)
        self.assertIn('stretch', cnf)
        self.assertRaises(TclError, text.window_cget, '1.1', 'spam')
        self.assertEqual(text.window_config, text.window_configure)
        button.destroy()

    def test_peer(self):
        text = self.text
        text.insert('1.0', 'Lorem ipsum')
        self.assertEqual(text.peer_names(), ())

        text.peer_create('.peer1')
        names = self.root.splitlist(text.tk.call('winfo', 'children', '.'))
        self.assertIn('.peer1', [str(n) for n in names])
        self.assertEqual([str(p) for p in text.peer_names()], ['.peer1'])
        # Peers share content.
        self.assertEqual(text.tk.call('.peer1', 'get', '1.0', 'end'),
                         'Lorem ipsum\n')
        text.tk.call('destroy', '.peer1')

    def test_bbox(self):
        text = self.text
        text.insert('1.0', 'hello')
        text.update()
        bbox = text.bbox('1.0')
        self.assertIsInstance(bbox, tuple)
        self.assertEqual(len(bbox), 4)
        for v in bbox:
            self.assertIsInstance(v, int)
        # A character that is not displayed has no bounding box.
        self.assertIsNone(text.bbox('end'))
        self.assertRaises(TclError, text.bbox, 'invalid')
        self.assertRaises(TypeError, text.bbox)
        self.assertRaises(TypeError, text.bbox, '1.0', '2.0')

    def test_dlineinfo(self):
        text = self.text
        text.insert('1.0', 'hello\nworld')
        text.update()
        info = text.dlineinfo('1.0')
        self.assertIsInstance(info, tuple)
        self.assertEqual(len(info), 5)
        for v in info:
            self.assertIsInstance(v, int)
        self.assertRaises(TclError, text.dlineinfo, 'invalid')
        self.assertRaises(TypeError, text.dlineinfo)
        self.assertRaises(TypeError, text.dlineinfo, '1.0', '2.0')

    def test_see(self):
        text = self.text
        text.insert('1.0', '\n'.join('line %d' % i for i in range(200)))
        text.update()
        # Initially the last line is not visible.
        self.assertIsNone(text.bbox('200.0'))
        text.see('200.0')
        text.update()
        self.assertIsNotNone(text.bbox('200.0'))
        self.assertRaises(TclError, text.see, 'invalid')
        self.assertRaises(TypeError, text.see)
        self.assertRaises(TypeError, text.see, '1.0', '2.0')

        # yview_pickplace is a deprecated way to make an index visible.
        text.yview_pickplace('1.0')
        text.update()
        self.assertIsNotNone(text.bbox('1.0'))

    def test_search(self):
        text = self.text

        # pattern and index are obligatory arguments.
        self.assertRaises(TclError, text.search, None, '1.0')
        self.assertRaises(TclError, text.search, 'a', None)
        self.assertRaises(TclError, text.search, None, None)

        # Invalid text index.
        self.assertRaises(TclError, text.search, '', 0)
        self.assertRaises(TclError, text.search, '', '')
        self.assertRaises(TclError, text.search, '', 'invalid')
        self.assertRaises(TclError, text.search, '', '1.0', 0)
        self.assertRaises(TclError, text.search, '', '1.0', '')
        self.assertRaises(TclError, text.search, '', '1.0', 'invalid')

        text.insert('1.0',
            'This is a test. This is only a test.\n'
            'Another line.\n'
            'Yet another line.\n'
            '64-bit')

        self.assertEqual(text.search('test', '1.0'), '1.10')
        self.assertEqual(text.search('test', '1.0', 'end'), '1.10')
        self.assertEqual(text.search('test', '1.0', '1.10'), '')
        self.assertEqual(text.search('test', '1.11'), '1.31')
        self.assertEqual(text.search('test', '1.32', 'end'), '')
        self.assertEqual(text.search('test', '1.32'), '1.10')

        self.assertEqual(text.search('', '1.0'), '1.0')  # empty pattern
        self.assertEqual(text.search('nonexistent', '1.0'), '')
        self.assertEqual(text.search('-bit', '1.0'), '4.2')  # starts with a hyphen

        self.assertEqual(text.search('line', '3.0'), '3.12')
        self.assertEqual(text.search('line', '3.0', forwards=True), '3.12')
        self.assertEqual(text.search('line', '3.0', backwards=True), '2.8')
        self.assertEqual(text.search('line', '3.0', forwards=True, backwards=True), '2.8')

        self.assertEqual(text.search('t.', '1.0'), '1.13')
        self.assertEqual(text.search('t.', '1.0', exact=True), '1.13')
        self.assertEqual(text.search('t.', '1.0', regexp=True), '1.10')
        self.assertEqual(text.search('t.', '1.0', exact=True, regexp=True), '1.10')

        self.assertEqual(text.search('TEST', '1.0'), '')
        self.assertEqual(text.search('TEST', '1.0', nocase=True), '1.10')

        self.assertEqual(text.search('.*line', '1.0', regexp=True), '2.0')
        self.assertEqual(text.search('.*line', '1.0', regexp=True, nolinestop=True), '1.0')

        self.assertEqual(text.search('test', '1.0', '1.13'), '1.10')
        self.assertEqual(text.search('test', '1.0', '1.13', strictlimits=True), '')
        self.assertEqual(text.search('test', '1.0', '1.14', strictlimits=True), '1.10')

        var = tkinter.Variable(self.root)
        self.assertEqual(text.search('test', '1.0', count=var), '1.10')
        self.assertEqual(var.get(), 4 if self.wantobjects else '4')

        # TODO: Add test for elide=True

    def test_search_all(self):
        text = self.text

        # pattern and index are obligatory arguments.
        self.assertRaises(TclError, text.search_all, None, '1.0')
        self.assertRaises(TclError, text.search_all, 'a', None)
        self.assertRaises(TclError, text.search_all, None, None)

        # Keyword-only arguments
        self.assertRaises(TypeError, text.search_all, 'a', '1.0', 'end', None)

        # Invalid text index.
        self.assertRaises(TclError, text.search_all, '', 0)
        self.assertRaises(TclError, text.search_all, '', '')
        self.assertRaises(TclError, text.search_all, '', 'invalid')
        self.assertRaises(TclError, text.search_all, '', '1.0', 0)
        self.assertRaises(TclError, text.search_all, '', '1.0', '')
        self.assertRaises(TclError, text.search_all, '', '1.0', 'invalid')

        def eq(res, expected):
            self.assertIsInstance(res, tuple)
            self.assertEqual([str(i) for i in res], expected)

        text.insert('1.0', 'ababa\naba\n64-bit')

        eq(text.search_all('aba', '1.0'), ['1.0', '2.0'])
        eq(text.search_all('aba', '1.0', 'end'), ['1.0', '2.0'])
        eq(text.search_all('aba', '1.1', 'end'), ['1.2', '2.0'])
        eq(text.search_all('aba', '1.1'), ['1.2', '2.0', '1.0'])

        res = text.search_all('', '1.0')  # empty pattern
        eq(res[:5], ['1.0', '1.1', '1.2', '1.3', '1.4'])
        eq(res[-5:], ['3.2', '3.3', '3.4', '3.5', '3.6'])
        eq(text.search_all('nonexistent', '1.0'), [])
        eq(text.search_all('-bit', '1.0'), ['3.2'])  # starts with a hyphen

        eq(text.search_all('aba', '1.0', 'end', forwards=True), ['1.0', '2.0'])
        eq(text.search_all('aba', 'end', '1.0', backwards=True), ['2.0', '1.2'])

        eq(text.search_all('aba', '1.0', overlap=True), ['1.0', '1.2', '2.0'])
        eq(text.search_all('aba', 'end', '1.0', overlap=True, backwards=True), ['2.0', '1.2', '1.0'])

        eq(text.search_all('aba', '1.0', exact=True), ['1.0', '2.0'])
        eq(text.search_all('a.a', '1.0', exact=True), [])
        eq(text.search_all('a.a', '1.0', regexp=True), ['1.0', '2.0'])

        eq(text.search_all('ABA', '1.0'), [])
        eq(text.search_all('ABA', '1.0', nocase=True), ['1.0', '2.0'])

        eq(text.search_all('a.a', '1.0', regexp=True), ['1.0', '2.0'])
        eq(text.search_all('a.a', '1.0', regexp=True, nolinestop=True), ['1.0', '1.4'])

        eq(text.search_all('aba', '1.0', '2.2'), ['1.0', '2.0'])
        eq(text.search_all('aba', '1.0', '2.2', strictlimits=True), ['1.0'])
        eq(text.search_all('aba', '1.0', '2.3', strictlimits=True), ['1.0', '2.0'])

        var = tkinter.Variable(self.root)
        eq(text.search_all('aba', '1.0', count=var), ['1.0', '2.0'])
        self.assertEqual(var.get(), (3, 3) if self.wantobjects else '3 3')

        # TODO: Add test for elide=True

    def test_count(self):
        text = self.text
        text.insert('1.0',
            'Lorem ipsum dolor sit amet,\n'
            'consectetur adipiscing elit,\n'
            'sed do eiusmod tempor incididunt\n'
            'ut labore et dolore magna aliqua.')

        options = ('chars', 'indices', 'lines',
                   'displaychars', 'displayindices', 'displaylines',
                   'xpixels', 'ypixels')
        self.assertEqual(len(text.count('1.0', 'end', *options, return_ints=True)), 8)
        self.assertEqual(len(text.count('1.0', 'end', *options)), 8)
        self.assertEqual(text.count('1.0', 'end', 'chars', 'lines', return_ints=True),
                         (124, 4))
        self.assertEqual(text.count('1.3', '4.5', 'chars', 'lines'), (92, 3))
        self.assertEqual(text.count('4.5', '1.3', 'chars', 'lines', return_ints=True),
                         (-92, -3))
        self.assertEqual(text.count('4.5', '1.3', 'chars', 'lines'), (-92, -3))
        self.assertEqual(text.count('1.3', '1.3', 'chars', 'lines', return_ints=True),
                         (0, 0))
        self.assertEqual(text.count('1.3', '1.3', 'chars', 'lines'), (0, 0))
        self.assertEqual(text.count('1.0', 'end', 'lines', return_ints=True), 4)
        self.assertEqual(text.count('1.0', 'end', 'lines'), (4,))
        self.assertEqual(text.count('end', '1.0', 'lines', return_ints=True), -4)
        self.assertEqual(text.count('end', '1.0', 'lines'), (-4,))
        self.assertEqual(text.count('1.3', '1.5', 'lines', return_ints=True), 0)
        self.assertEqual(text.count('1.3', '1.5', 'lines'), None)
        self.assertEqual(text.count('1.3', '1.3', 'lines', return_ints=True), 0)
        self.assertEqual(text.count('1.3', '1.3', 'lines'), None)
        # Count 'indices' by default.
        self.assertEqual(text.count('1.0', 'end', return_ints=True), 124)
        self.assertEqual(text.count('1.0', 'end'), (124,))
        self.assertEqual(text.count('1.0', 'end', 'indices', return_ints=True), 124)
        self.assertEqual(text.count('1.0', 'end', 'indices'), (124,))
        self.assertRaises(TclError, text.count, '1.0', 'end', 'spam')
        self.assertRaises(TclError, text.count, '1.0', 'end', '-lines')

        self.assertIsInstance(text.count('1.3', '1.5', 'ypixels', return_ints=True), int)
        self.assertIsInstance(text.count('1.3', '1.5', 'ypixels'), tuple)
        self.assertIsInstance(text.count('1.3', '1.5', 'update', 'ypixels', return_ints=True), int)
        self.assertIsInstance(text.count('1.3', '1.5', 'update', 'ypixels'), int)
        self.assertEqual(text.count('1.3', '1.3', 'update', 'ypixels', return_ints=True), 0)
        self.assertEqual(text.count('1.3', '1.3', 'update', 'ypixels'), None)
        self.assertEqual(text.count('1.3', '1.5', 'update', 'indices', return_ints=True), 2)
        self.assertEqual(text.count('1.3', '1.5', 'update', 'indices'), 2)
        self.assertEqual(text.count('1.3', '1.3', 'update', 'indices', return_ints=True), 0)
        self.assertEqual(text.count('1.3', '1.3', 'update', 'indices'), None)
        self.assertEqual(text.count('1.3', '1.5', 'update', return_ints=True), 2)
        self.assertEqual(text.count('1.3', '1.5', 'update'), (2,))
        self.assertEqual(text.count('1.3', '1.3', 'update', return_ints=True), 0)
        self.assertEqual(text.count('1.3', '1.3', 'update'), None)


if __name__ == "__main__":
    unittest.main()
