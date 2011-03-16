# -*- encoding: utf-8 -*-
import sys
import unittest
import ttk

class MockTclObj(object):
    typename = 'test'

    def __init__(self, val):
        self.val = val

    def __str__(self):
        return unicode(self.val)


class MockStateSpec(object):
    typename = 'StateSpec'

    def __init__(self, *args):
        self.val = args

    def __str__(self):
        return ' '.join(self.val)


class InternalFunctionsTest(unittest.TestCase):

    def test_format_optdict(self):
        def check_against(fmt_opts, result):
            for i in range(0, len(fmt_opts), 2):
                self.assertEqual(result.pop(fmt_opts[i]), fmt_opts[i + 1])
            if result:
                self.fail("result still got elements: %s" % result)

        # passing an empty dict should return an empty object (tuple here)
        self.assertFalse(ttk._format_optdict({}))

        # check list formatting
        check_against(
            ttk._format_optdict({'fg': 'blue', 'padding': [1, 2, 3, 4]}),
            {'-fg': 'blue', '-padding': '1 2 3 4'})

        # check tuple formatting (same as list)
        check_against(
            ttk._format_optdict({'test': (1, 2, '', 0)}),
            {'-test': '1 2 {} 0'})

        # check untouched values
        check_against(
            ttk._format_optdict({'test': {'left': 'as is'}}),
            {'-test': {'left': 'as is'}})

        # check script formatting and untouched value(s)
        check_against(
            ttk._format_optdict(
                {'test': [1, -1, '', '2m', 0], 'nochange1': 3,
                 'nochange2': 'abc def'}, script=True),
            {'-test': '{1 -1 {} 2m 0}', '-nochange1': 3,
             '-nochange2': 'abc def' })

        opts = {u'αβγ': True, u'á': False}
        orig_opts = opts.copy()
        # check if giving unicode keys is fine
        check_against(ttk._format_optdict(opts), {u'-αβγ': True, u'-á': False})
        # opts should remain unchanged
        self.assertEqual(opts, orig_opts)

        # passing values with spaces inside a tuple/list
        check_against(
            ttk._format_optdict(
                {'option': ('one two', 'three')}),
            {'-option': '{one two} three'})

        # ignore an option
        amount_opts = len(ttk._format_optdict(opts, ignore=(u'á'))) // 2
        self.assertEqual(amount_opts, len(opts) - 1)

        # ignore non-existing options
        amount_opts = len(ttk._format_optdict(opts, ignore=(u'á', 'b'))) // 2
        self.assertEqual(amount_opts, len(opts) - 1)

        # ignore every option
        self.assertFalse(ttk._format_optdict(opts, ignore=opts.keys()))


    def test_format_mapdict(self):
        opts = {'a': [('b', 'c', 'val'), ('d', 'otherval'), ('', 'single')]}
        result = ttk._format_mapdict(opts)
        self.assertEqual(len(result), len(opts.keys()) * 2)
        self.assertEqual(result, ('-a', '{b c} val d otherval {} single'))
        self.assertEqual(ttk._format_mapdict(opts, script=True),
            ('-a', '{{b c} val d otherval {} single}'))

        self.assertEqual(ttk._format_mapdict({2: []}), ('-2', ''))

        opts = {u'üñíćódè': [(u'á', u'vãl')]}
        result = ttk._format_mapdict(opts)
        self.assertEqual(result, (u'-üñíćódè', u'á vãl'))

        # empty states
        valid = {'opt': [('', u'', 'hi')]}
        self.assertEqual(ttk._format_mapdict(valid), ('-opt', '{ } hi'))

        # when passing multiple states, they all must be strings
        invalid = {'opt': [(1, 2, 'valid val')]}
        self.assertRaises(TypeError, ttk._format_mapdict, invalid)
        invalid = {'opt': [([1], '2', 'valid val')]}
        self.assertRaises(TypeError, ttk._format_mapdict, invalid)
        # but when passing a single state, it can be anything
        valid = {'opt': [[1, 'value']]}
        self.assertEqual(ttk._format_mapdict(valid), ('-opt', '1 value'))
        # special attention to single states which evalute to False
        for stateval in (None, 0, False, '', set()): # just some samples
            valid = {'opt': [(stateval, 'value')]}
            self.assertEqual(ttk._format_mapdict(valid),
                ('-opt', '{} value'))

        # values must be iterable
        opts = {'a': None}
        self.assertRaises(TypeError, ttk._format_mapdict, opts)

        # items in the value must have size >= 2
        self.assertRaises(IndexError, ttk._format_mapdict,
            {'a': [('invalid', )]})


    def test_format_elemcreate(self):
        self.assertTrue(ttk._format_elemcreate(None), (None, ()))

        ## Testing type = image
        # image type expects at least an image name, so this should raise
        # IndexError since it tries to access the index 0 of an empty tuple
        self.assertRaises(IndexError, ttk._format_elemcreate, 'image')

        # don't format returned values as a tcl script
        # minimum acceptable for image type
        self.assertEqual(ttk._format_elemcreate('image', False, 'test'),
            ("test ", ()))
        # specifying a state spec
        self.assertEqual(ttk._format_elemcreate('image', False, 'test',
            ('', 'a')), ("test {} a", ()))
        # state spec with multiple states
        self.assertEqual(ttk._format_elemcreate('image', False, 'test',
            ('a', 'b', 'c')), ("test {a b} c", ()))
        # state spec and options
        self.assertEqual(ttk._format_elemcreate('image', False, 'test',
            ('a', 'b'), a='x', b='y'), ("test a b", ("-a", "x", "-b", "y")))
        # format returned values as a tcl script
        # state spec with multiple states and an option with a multivalue
        self.assertEqual(ttk._format_elemcreate('image', True, 'test',
            ('a', 'b', 'c', 'd'), x=[2, 3]), ("{test {a b c} d}", "-x {2 3}"))

        ## Testing type = vsapi
        # vsapi type expects at least a class name and a part_id, so this
        # should raise an ValueError since it tries to get two elements from
        # an empty tuple
        self.assertRaises(ValueError, ttk._format_elemcreate, 'vsapi')

        # don't format returned values as a tcl script
        # minimum acceptable for vsapi
        self.assertEqual(ttk._format_elemcreate('vsapi', False, 'a', 'b'),
            ("a b ", ()))
        # now with a state spec with multiple states
        self.assertEqual(ttk._format_elemcreate('vsapi', False, 'a', 'b',
            ('a', 'b', 'c')), ("a b {a b} c", ()))
        # state spec and option
        self.assertEqual(ttk._format_elemcreate('vsapi', False, 'a', 'b',
            ('a', 'b'), opt='x'), ("a b a b", ("-opt", "x")))
        # format returned values as a tcl script
        # state spec with a multivalue and an option
        self.assertEqual(ttk._format_elemcreate('vsapi', True, 'a', 'b',
            ('a', 'b', [1, 2]), opt='x'), ("{a b {a b} {1 2}}", "-opt x"))

        # Testing type = from
        # from type expects at least a type name
        self.assertRaises(IndexError, ttk._format_elemcreate, 'from')

        self.assertEqual(ttk._format_elemcreate('from', False, 'a'),
            ('a', ()))
        self.assertEqual(ttk._format_elemcreate('from', False, 'a', 'b'),
            ('a', ('b', )))
        self.assertEqual(ttk._format_elemcreate('from', True, 'a', 'b'),
            ('{a}', 'b'))


    def test_format_layoutlist(self):
        def sample(indent=0, indent_size=2):
            return ttk._format_layoutlist(
            [('a', {'other': [1, 2, 3], 'children':
                [('b', {'children':
                    [('c', {'children':
                        [('d', {'nice': 'opt'})], 'something': (1, 2)
                    })]
                })]
            })], indent=indent, indent_size=indent_size)[0]

        def sample_expected(indent=0, indent_size=2):
            spaces = lambda amount=0: ' ' * (amount + indent)
            return (
                "%sa -other {1 2 3} -children {\n"
                "%sb -children {\n"
                "%sc -something {1 2} -children {\n"
                "%sd -nice opt\n"
                "%s}\n"
                "%s}\n"
                "%s}" % (spaces(), spaces(indent_size),
                    spaces(2 * indent_size), spaces(3 * indent_size),
                    spaces(2 * indent_size), spaces(indent_size), spaces()))

        # empty layout
        self.assertEqual(ttk._format_layoutlist([])[0], '')

        # smallest (after an empty one) acceptable layout
        smallest = ttk._format_layoutlist([('a', None)], indent=0)
        self.assertEqual(smallest,
            ttk._format_layoutlist([('a', '')], indent=0))
        self.assertEqual(smallest[0], 'a')

        # testing indentation levels
        self.assertEqual(sample(), sample_expected())
        for i in range(4):
            self.assertEqual(sample(i), sample_expected(i))
            self.assertEqual(sample(i, i), sample_expected(i, i))

        # invalid layout format, different kind of exceptions will be
        # raised

        # plain wrong format
        self.assertRaises(ValueError, ttk._format_layoutlist,
            ['bad', 'format'])
        self.assertRaises(TypeError, ttk._format_layoutlist, None)
        # _format_layoutlist always expects the second item (in every item)
        # to act like a dict (except when the value evalutes to False).
        self.assertRaises(AttributeError,
            ttk._format_layoutlist, [('a', 'b')])
        # bad children formatting
        self.assertRaises(ValueError, ttk._format_layoutlist,
            [('name', {'children': {'a': None}})])


    def test_script_from_settings(self):
        # empty options
        self.assertFalse(ttk._script_from_settings({'name':
            {'configure': None, 'map': None, 'element create': None}}))

        # empty layout
        self.assertEqual(
            ttk._script_from_settings({'name': {'layout': None}}),
            "ttk::style layout name {\nnull\n}")

        configdict = {u'αβγ': True, u'á': False}
        self.assertTrue(
            ttk._script_from_settings({'name': {'configure': configdict}}))

        mapdict = {u'üñíćódè': [(u'á', u'vãl')]}
        self.assertTrue(
            ttk._script_from_settings({'name': {'map': mapdict}}))

        # invalid image element
        self.assertRaises(IndexError,
            ttk._script_from_settings, {'name': {'element create': ['image']}})

        # minimal valid image
        self.assertTrue(ttk._script_from_settings({'name':
            {'element create': ['image', 'name']}}))

        image = {'thing': {'element create':
            ['image', 'name', ('state1', 'state2', 'val')]}}
        self.assertEqual(ttk._script_from_settings(image),
            "ttk::style element create thing image {name {state1 state2} val} ")

        image['thing']['element create'].append({'opt': 30})
        self.assertEqual(ttk._script_from_settings(image),
            "ttk::style element create thing image {name {state1 state2} val} "
            "-opt 30")

        image['thing']['element create'][-1]['opt'] = [MockTclObj(3),
            MockTclObj('2m')]
        self.assertEqual(ttk._script_from_settings(image),
            "ttk::style element create thing image {name {state1 state2} val} "
            "-opt {3 2m}")


    def test_dict_from_tcltuple(self):
        fakettuple = ('-a', '{1 2 3}', '-something', 'foo')

        self.assertEqual(ttk._dict_from_tcltuple(fakettuple, False),
            {'-a': '{1 2 3}', '-something': 'foo'})

        self.assertEqual(ttk._dict_from_tcltuple(fakettuple),
            {'a': '{1 2 3}', 'something': 'foo'})

        # passing a tuple with a single item should return an empty dict,
        # since it tries to break the tuple by pairs.
        self.assertFalse(ttk._dict_from_tcltuple(('single', )))

        sspec = MockStateSpec('a', 'b')
        self.assertEqual(ttk._dict_from_tcltuple(('-a', (sspec, 'val'))),
            {'a': [('a', 'b', 'val')]})

        self.assertEqual(ttk._dict_from_tcltuple((MockTclObj('-padding'),
            [MockTclObj('1'), 2, MockTclObj('3m')])),
            {'padding': [1, 2, '3m']})


    def test_list_from_statespec(self):
        def test_it(sspec, value, res_value, states):
            self.assertEqual(ttk._list_from_statespec(
                (sspec, value)), [states + (res_value, )])

        states_even = tuple('state%d' % i for i in range(6))
        statespec = MockStateSpec(*states_even)
        test_it(statespec, 'val', 'val', states_even)
        test_it(statespec, MockTclObj('val'), 'val', states_even)

        states_odd = tuple('state%d' % i for i in range(5))
        statespec = MockStateSpec(*states_odd)
        test_it(statespec, 'val', 'val', states_odd)

        test_it(('a', 'b', 'c'), MockTclObj('val'), 'val', ('a', 'b', 'c'))


    def test_list_from_layouttuple(self):
        # empty layout tuple
        self.assertFalse(ttk._list_from_layouttuple(()))

        # shortest layout tuple
        self.assertEqual(ttk._list_from_layouttuple(('name', )),
            [('name', {})])

        # not so interesting ltuple
        sample_ltuple = ('name', '-option', 'value')
        self.assertEqual(ttk._list_from_layouttuple(sample_ltuple),
            [('name', {'option': 'value'})])

        # empty children
        self.assertEqual(ttk._list_from_layouttuple(
            ('something', '-children', ())),
            [('something', {'children': []})]
        )

        # more interesting ltuple
        ltuple = (
            'name', '-option', 'niceone', '-children', (
                ('otherone', '-children', (
                    ('child', )), '-otheropt', 'othervalue'
                )
            )
        )
        self.assertEqual(ttk._list_from_layouttuple(ltuple),
            [('name', {'option': 'niceone', 'children':
                [('otherone', {'otheropt': 'othervalue', 'children':
                    [('child', {})]
                })]
            })]
        )

        # bad tuples
        self.assertRaises(ValueError, ttk._list_from_layouttuple,
            ('name', 'no_minus'))
        self.assertRaises(ValueError, ttk._list_from_layouttuple,
            ('name', 'no_minus', 'value'))
        self.assertRaises(ValueError, ttk._list_from_layouttuple,
            ('something', '-children')) # no children
        self.assertRaises(ValueError, ttk._list_from_layouttuple,
            ('something', '-children', 'value')) # invalid children


    def test_val_or_dict(self):
        def func(opt, val=None):
            if val is None:
                return "test val"
            return (opt, val)

        options = {'test': None}
        self.assertEqual(ttk._val_or_dict(options, func), "test val")

        options = {'test': 3}
        self.assertEqual(ttk._val_or_dict(options, func), options)


    def test_convert_stringval(self):
        tests = (
            (0, 0), ('09', 9), ('a', 'a'), (u'áÚ', u'áÚ'), ([], '[]'),
            (None, 'None')
        )
        for orig, expected in tests:
            self.assertEqual(ttk._convert_stringval(orig), expected)

        if sys.getdefaultencoding() == 'ascii':
            self.assertRaises(UnicodeDecodeError,
                ttk._convert_stringval, 'á')


class TclObjsToPyTest(unittest.TestCase):

    def test_unicode(self):
        adict = {'opt': u'välúè'}
        self.assertEqual(ttk.tclobjs_to_py(adict), {'opt': u'välúè'})

        adict['opt'] = MockTclObj(adict['opt'])
        self.assertEqual(ttk.tclobjs_to_py(adict), {'opt': u'välúè'})

    def test_multivalues(self):
        adict = {'opt': [1, 2, 3, 4]}
        self.assertEqual(ttk.tclobjs_to_py(adict), {'opt': [1, 2, 3, 4]})

        adict['opt'] = [1, 'xm', 3]
        self.assertEqual(ttk.tclobjs_to_py(adict), {'opt': [1, 'xm', 3]})

        adict['opt'] = (MockStateSpec('a', 'b'), u'válũè')
        self.assertEqual(ttk.tclobjs_to_py(adict),
            {'opt': [('a', 'b', u'válũè')]})

        self.assertEqual(ttk.tclobjs_to_py({'x': ['y z']}),
            {'x': ['y z']})

    def test_nosplit(self):
        self.assertEqual(ttk.tclobjs_to_py({'text': 'some text'}),
            {'text': 'some text'})

tests_nogui = (InternalFunctionsTest, TclObjsToPyTest)

if __name__ == "__main__":
    from test.test_support import run_unittest
    run_unittest(*tests_nogui)
