import io
import itertools
import pickle
import pickletools
import tempfile
import textwrap
from test import support
from test.support import os_helper
from test.pickletester import AbstractPickleTests
import doctest
import unittest

class OptimizedPickleTests(AbstractPickleTests, unittest.TestCase):

    def dumps(self, arg, proto=None, **kwargs):
        return pickletools.optimize(pickle.dumps(arg, proto, **kwargs))

    def loads(self, buf, **kwds):
        return pickle.loads(buf, **kwds)

    # Test relies on precise output of dumps()
    test_pickle_to_2x = None

    # Test relies on writing by chunks into a file object.
    test_framed_write_sizes_with_delayed_writer = None

    def test_optimize_long_binget(self):
        data = [str(i) for i in range(257)]
        data.append(data[-1])
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            pickled = pickle.dumps(data, proto)
            unpickled = pickle.loads(pickled)
            self.assertEqual(unpickled, data)
            self.assertIs(unpickled[-1], unpickled[-2])

            pickled2 = pickletools.optimize(pickled)
            unpickled2 = pickle.loads(pickled2)
            self.assertEqual(unpickled2, data)
            self.assertIs(unpickled2[-1], unpickled2[-2])
            self.assertNotIn(pickle.LONG_BINGET, pickled2)
            self.assertNotIn(pickle.LONG_BINPUT, pickled2)

    def test_optimize_binput_and_memoize(self):
        pickled = (b'\x80\x04\x95\x15\x00\x00\x00\x00\x00\x00\x00'
                   b']\x94(\x8c\x04spamq\x01\x8c\x03ham\x94h\x02e.')
        #    0: \x80 PROTO      4
        #    2: \x95 FRAME      21
        #   11: ]    EMPTY_LIST
        #   12: \x94 MEMOIZE
        #   13: (    MARK
        #   14: \x8c     SHORT_BINUNICODE 'spam'
        #   20: q        BINPUT     1
        #   22: \x8c     SHORT_BINUNICODE 'ham'
        #   27: \x94     MEMOIZE
        #   28: h        BINGET     2
        #   30: e        APPENDS    (MARK at 13)
        #   31: .    STOP
        self.assertIn(pickle.BINPUT, pickled)
        unpickled = pickle.loads(pickled)
        self.assertEqual(unpickled, ['spam', 'ham', 'ham'])
        self.assertIs(unpickled[1], unpickled[2])

        pickled2 = pickletools.optimize(pickled)
        unpickled2 = pickle.loads(pickled2)
        self.assertEqual(unpickled2, ['spam', 'ham', 'ham'])
        self.assertIs(unpickled2[1], unpickled2[2])
        self.assertNotIn(pickle.BINPUT, pickled2)


class SimpleReader:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def read(self, n):
        data = self.data[self.pos: self.pos + n]
        self.pos += n
        return data

    def readline(self):
        nl = self.data.find(b'\n', self.pos) + 1
        if not nl:
            nl = len(self.data)
        data = self.data[self.pos: nl]
        self.pos = nl
        return data


class GenopsTests(unittest.TestCase):
    def test_genops(self):
        it = pickletools.genops(b'(I123\nK\x12J\x12\x34\x56\x78t.')
        self.assertEqual([(item[0].name,) +  item[1:] for item in it], [
            ('MARK', None, 0),
            ('INT', 123, 1),
            ('BININT1', 0x12, 6),
            ('BININT', 0x78563412, 8),
            ('TUPLE', None, 13),
            ('STOP', None, 14),
        ])

    def test_from_file(self):
        f = io.BytesIO(b'prefix(I123\nK\x12J\x12\x34\x56\x78t.suffix')
        self.assertEqual(f.read(6), b'prefix')
        it = pickletools.genops(f)
        self.assertEqual([(item[0].name,) +  item[1:] for item in it], [
            ('MARK', None, 6),
            ('INT', 123, 7),
            ('BININT1', 0x12, 12),
            ('BININT', 0x78563412, 14),
            ('TUPLE', None, 19),
            ('STOP', None, 20),
        ])
        self.assertEqual(f.read(), b'suffix')

    def test_without_pos(self):
        f = SimpleReader(b'(I123\nK\x12J\x12\x34\x56\x78t.')
        it = pickletools.genops(f)
        self.assertEqual([(item[0].name,) +  item[1:] for item in it], [
            ('MARK', None, None),
            ('INT', 123, None),
            ('BININT1', 0x12, None),
            ('BININT', 0x78563412, None),
            ('TUPLE', None, None),
            ('STOP', None, None),
        ])

    def test_no_stop(self):
        it = pickletools.genops(b'N')
        item = next(it)
        self.assertEqual(item[0].name, 'NONE')
        with self.assertRaisesRegex(ValueError,
                'pickle exhausted before seeing STOP'):
            next(it)

    def test_truncated_data(self):
        it = pickletools.genops(b'I123')
        with self.assertRaisesRegex(ValueError,
                'no newline found when trying to read stringnl'):
            next(it)
        it = pickletools.genops(b'J\x12\x34')
        with self.assertRaisesRegex(ValueError,
                'not enough data in stream to read int4'):
            next(it)

    def test_unknown_opcode(self):
        it = pickletools.genops(b'N\xff')
        item = next(it)
        self.assertEqual(item[0].name, 'NONE')
        with self.assertRaisesRegex(ValueError,
                r"at position 1, opcode b'\\xff' unknown"):
            next(it)

    def test_unknown_opcode_without_pos(self):
        f = SimpleReader(b'N\xff')
        it = pickletools.genops(f)
        item = next(it)
        self.assertEqual(item[0].name, 'NONE')
        with self.assertRaisesRegex(ValueError,
                r"at position <unknown>, opcode b'\\xff' unknown"):
            next(it)


class DisTests(unittest.TestCase):
    maxDiff = None

    def check_dis(self, data, expected, **kwargs):
        out = io.StringIO()
        pickletools.dis(data, out=out, **kwargs)
        self.assertEqual(out.getvalue(), expected)

    def check_dis_error(self, data, expected, expected_error, **kwargs):
        out = io.StringIO()
        with self.assertRaisesRegex(ValueError, expected_error):
            pickletools.dis(data, out=out, **kwargs)
        self.assertEqual(out.getvalue(), expected)

    def test_mark(self):
        self.check_dis(b'(N(tl.', '''\
    0: (    MARK
    1: N        NONE
    2: (        MARK
    3: t            TUPLE      (MARK at 2)
    4: l        LIST       (MARK at 0)
    5: .    STOP
highest protocol among opcodes = 0
''')

    def test_indentlevel(self):
        self.check_dis(b'(N(tl.', '''\
    0: (    MARK
    1: N      NONE
    2: (      MARK
    3: t        TUPLE      (MARK at 2)
    4: l      LIST       (MARK at 0)
    5: .    STOP
highest protocol among opcodes = 0
''', indentlevel=2)

    def test_mark_without_pos(self):
        self.check_dis(SimpleReader(b'(N(tl.'), '''\
(    MARK
N        NONE
(        MARK
t            TUPLE      (MARK at unknown opcode offset)
l        LIST       (MARK at unknown opcode offset)
.    STOP
highest protocol among opcodes = 0
''')

    def test_no_mark(self):
        self.check_dis_error(b'Nt.', '''\
    0: N    NONE
    1: t    TUPLE
''', 'no MARK exists on stack')

    def test_put(self):
        self.check_dis(b'Np0\nq\x01r\x02\x00\x00\x00\x94.', '''\
    0: N    NONE
    1: p    PUT        0
    4: q    BINPUT     1
    6: r    LONG_BINPUT 2
   11: \\x94 MEMOIZE    (as 3)
   12: .    STOP
highest protocol among opcodes = 4
''')

    def test_put_redefined(self):
        self.check_dis(b'Np1\np1\nq\x01r\x01\x00\x00\x00\x94.', '''\
    0: N    NONE
    1: p    PUT        1
    4: p    PUT        1
    7: q    BINPUT     1
    9: r    LONG_BINPUT 1
   14: \\x94 MEMOIZE    (as 1)
   15: .    STOP
highest protocol among opcodes = 4
''')

    def test_put_empty_stack(self):
        self.check_dis_error(b'p0\n', '''\
    0: p    PUT        0
''', "stack is empty -- can't store into memo")

    def test_put_markobject(self):
        self.check_dis_error(b'(p0\n', '''\
    0: (    MARK
    1: p        PUT        0
''', "can't store markobject in the memo")

    def test_get(self):
        self.check_dis(b'(Np1\ng1\nh\x01j\x01\x00\x00\x00t.', '''\
    0: (    MARK
    1: N        NONE
    2: p        PUT        1
    5: g        GET        1
    8: h        BINGET     1
   10: j        LONG_BINGET 1
   15: t        TUPLE      (MARK at 0)
   16: .    STOP
highest protocol among opcodes = 1
''')

    def test_get_without_put(self):
        self.check_dis_error(b'g1\n.', '''\
    0: g    GET        1
''', 'memo key 1 has never been stored into')
        self.check_dis_error(b'h\x01.', '''\
    0: h    BINGET     1
''', 'memo key 1 has never been stored into')
        self.check_dis_error(b'j\x01\x00\x00\x00.', '''\
    0: j    LONG_BINGET 1
''', 'memo key 1 has never been stored into')

    def test_memo(self):
        memo = {}
        self.check_dis(b'Np1\n.', '''\
    0: N    NONE
    1: p    PUT        1
    4: .    STOP
highest protocol among opcodes = 0
''', memo=memo)
        self.check_dis(b'g1\n.', '''\
    0: g    GET        1
    3: .    STOP
highest protocol among opcodes = 0
''', memo=memo)

    def test_mark_pop(self):
        self.check_dis(b'(N00N.', '''\
    0: (    MARK
    1: N        NONE
    2: 0        POP
    3: 0        POP        (MARK at 0)
    4: N    NONE
    5: .    STOP
highest protocol among opcodes = 0
''')

    def test_too_small_stack(self):
        self.check_dis_error(b'a', '''\
    0: a    APPEND
''', 'tries to pop 2 items from stack with only 0 items')
        self.check_dis_error(b']a', '''\
    0: ]    EMPTY_LIST
    1: a    APPEND
''', 'tries to pop 2 items from stack with only 1 items')

    def test_no_stop(self):
        self.check_dis_error(b'N', '''\
    0: N    NONE
''', 'pickle exhausted before seeing STOP')

    def test_truncated_data(self):
        self.check_dis_error(b'NI123', '''\
    0: N    NONE
''', 'no newline found when trying to read stringnl')
        self.check_dis_error(b'NJ\x12\x34', '''\
    0: N    NONE
''', 'not enough data in stream to read int4')

    def test_unknown_opcode(self):
        self.check_dis_error(b'N\xff', '''\
    0: N    NONE
''', r"at position 1, opcode b'\\xff' unknown")

    def test_stop_not_empty_stack(self):
        self.check_dis_error(b']N.', '''\
    0: ]    EMPTY_LIST
    1: N    NONE
    2: .    STOP
highest protocol among opcodes = 1
''', r'stack not empty after STOP: \[list\]')

    def test_annotate(self):
        self.check_dis(b'(Nt.', '''\
    0: (    MARK Push markobject onto the stack.
    1: N        NONE Push None on the stack.
    2: t        TUPLE      (MARK at 0) Build a tuple out of the topmost stack slice, after markobject.
    3: .    STOP                       Stop the unpickling machine.
highest protocol among opcodes = 0
''', annotate=1)
        self.check_dis(b'(Nt.', '''\
    0: (    MARK            Push markobject onto the stack.
    1: N        NONE        Push None on the stack.
    2: t        TUPLE      (MARK at 0) Build a tuple out of the topmost stack slice, after markobject.
    3: .    STOP                       Stop the unpickling machine.
highest protocol among opcodes = 0
''', annotate=20)
        self.check_dis(b'(((((((ttttttt.', '''\
    0: (    MARK            Push markobject onto the stack.
    1: (        MARK        Push markobject onto the stack.
    2: (            MARK    Push markobject onto the stack.
    3: (                MARK Push markobject onto the stack.
    4: (                    MARK Push markobject onto the stack.
    5: (                        MARK Push markobject onto the stack.
    6: (                            MARK Push markobject onto the stack.
    7: t                                TUPLE      (MARK at 6) Build a tuple out of the topmost stack slice, after markobject.
    8: t                            TUPLE      (MARK at 5) Build a tuple out of the topmost stack slice, after markobject.
    9: t                        TUPLE      (MARK at 4) Build a tuple out of the topmost stack slice, after markobject.
   10: t                    TUPLE      (MARK at 3)     Build a tuple out of the topmost stack slice, after markobject.
   11: t                TUPLE      (MARK at 2)         Build a tuple out of the topmost stack slice, after markobject.
   12: t            TUPLE      (MARK at 1)             Build a tuple out of the topmost stack slice, after markobject.
   13: t        TUPLE      (MARK at 0)                 Build a tuple out of the topmost stack slice, after markobject.
   14: .    STOP                                       Stop the unpickling machine.
highest protocol among opcodes = 0
''', annotate=20)

    def test_string(self):
        self.check_dis(b"S'abc'\n.", '''\
    0: S    STRING     'abc'
    7: .    STOP
highest protocol among opcodes = 0
''')
        self.check_dis(b'S"abc"\n.', '''\
    0: S    STRING     'abc'
    7: .    STOP
highest protocol among opcodes = 0
''')
        self.check_dis(b"S'\xc3\xb5'\n.", '''\
    0: S    STRING     '\\xc3\\xb5'
    6: .    STOP
highest protocol among opcodes = 0
''')

    def test_string_without_quotes(self):
        self.check_dis_error(b"Sabc'\n.", '',
                             'no string quotes around b"abc\'"')
        self.check_dis_error(b'Sabc"\n.', '',
                             "no string quotes around b'abc\"'")
        self.check_dis_error(b"S'abc\n.", '',
                             '''string quote b"'" not found at both ends of b"'abc"''')
        self.check_dis_error(b'S"abc\n.', '',
                             r"""string quote b'"' not found at both ends of b'"abc'""")
        self.check_dis_error(b"S'abc\"\n.", '',
                             r"""string quote b"'" not found at both ends of b'\\'abc"'""")
        self.check_dis_error(b"S\"abc'\n.", '',
                             r"""string quote b'"' not found at both ends of b'"abc\\''""")

    def test_binstring(self):
        self.check_dis(b"T\x03\x00\x00\x00abc.", '''\
    0: T    BINSTRING  'abc'
    8: .    STOP
highest protocol among opcodes = 1
''')
        self.check_dis(b"T\x02\x00\x00\x00\xc3\xb5.", '''\
    0: T    BINSTRING  '\\xc3\\xb5'
    7: .    STOP
highest protocol among opcodes = 1
''')

    def test_short_binstring(self):
        self.check_dis(b"U\x03abc.", '''\
    0: U    SHORT_BINSTRING 'abc'
    5: .    STOP
highest protocol among opcodes = 1
''')
        self.check_dis(b"U\x02\xc3\xb5.", '''\
    0: U    SHORT_BINSTRING '\\xc3\\xb5'
    4: .    STOP
highest protocol among opcodes = 1
''')

    def test_global(self):
        self.check_dis(b"cmodule\nname\n.", '''\
    0: c    GLOBAL     'module name'
   13: .    STOP
highest protocol among opcodes = 0
''')
        self.check_dis(b"cm\xc3\xb6dule\nn\xc3\xa4me\n.", '''\
    0: c    GLOBAL     'm\xf6dule n\xe4me'
   15: .    STOP
highest protocol among opcodes = 0
''')

    def test_inst(self):
        self.check_dis(b"(imodule\nname\n.", '''\
    0: (    MARK
    1: i        INST       'module name' (MARK at 0)
   14: .    STOP
highest protocol among opcodes = 0
''')

    def test_persid(self):
        self.check_dis(b"Pabc\n.", '''\
    0: P    PERSID     'abc'
    5: .    STOP
highest protocol among opcodes = 0
''')

    def test_constants(self):
        self.check_dis(b"(NI00\nI01\n\x89\x88t.", '''\
    0: (    MARK
    1: N        NONE
    2: I        INT        False
    6: I        INT        True
   10: \\x89     NEWFALSE
   11: \\x88     NEWTRUE
   12: t        TUPLE      (MARK at 0)
   13: .    STOP
highest protocol among opcodes = 2
''')

    def test_integers(self):
        self.check_dis(b"(I0\nI1\nI10\nI011\nL12\nL13L\nL014\nL015L\nt.", '''\
    0: (    MARK
    1: I        INT        0
    4: I        INT        1
    7: I        INT        10
   11: I        INT        11
   16: L        LONG       12
   20: L        LONG       13
   25: L        LONG       14
   30: L        LONG       15
   36: t        TUPLE      (MARK at 0)
   37: .    STOP
highest protocol among opcodes = 0
''')

    def test_nondecimal_integers(self):
        self.check_dis_error(b'I0b10\n.', '', 'invalid literal for int')
        self.check_dis_error(b'I0o10\n.', '', 'invalid literal for int')
        self.check_dis_error(b'I0x10\n.', '', 'invalid literal for int')
        self.check_dis_error(b'L0b10L\n.', '', 'invalid literal for int')
        self.check_dis_error(b'L0o10L\n.', '', 'invalid literal for int')
        self.check_dis_error(b'L0x10L\n.', '', 'invalid literal for int')


class MiscTestCase(unittest.TestCase):
    def test__all__(self):
        not_exported = {
            'bytes_types',
            'UP_TO_NEWLINE', 'TAKEN_FROM_ARGUMENT1',
            'TAKEN_FROM_ARGUMENT4', 'TAKEN_FROM_ARGUMENT4U',
            'TAKEN_FROM_ARGUMENT8U', 'ArgumentDescriptor',
            'read_uint1', 'read_uint2', 'read_int4', 'read_uint4',
            'read_uint8', 'read_stringnl', 'read_stringnl_noescape',
            'read_stringnl_noescape_pair', 'read_string1',
            'read_string4', 'read_bytes1', 'read_bytes4',
            'read_bytes8', 'read_bytearray8', 'read_unicodestringnl',
            'read_unicodestring1', 'read_unicodestring4',
            'read_unicodestring8', 'read_decimalnl_short',
            'read_decimalnl_long', 'read_floatnl', 'read_float8',
            'read_long1', 'read_long4',
            'uint1', 'uint2', 'int4', 'uint4', 'uint8', 'stringnl',
            'stringnl_noescape', 'stringnl_noescape_pair', 'string1',
            'string4', 'bytes1', 'bytes4', 'bytes8', 'bytearray8',
            'unicodestringnl', 'unicodestring1', 'unicodestring4',
            'unicodestring8', 'decimalnl_short', 'decimalnl_long',
            'floatnl', 'float8', 'long1', 'long4',
            'StackObject',
            'pyint', 'pylong', 'pyinteger_or_bool', 'pybool', 'pyfloat',
            'pybytes_or_str', 'pystring', 'pybytes', 'pybytearray',
            'pyunicode', 'pynone', 'pytuple', 'pylist', 'pydict',
            'pyset', 'pyfrozenset', 'pybuffer', 'anyobject',
            'markobject', 'stackslice', 'OpcodeInfo', 'opcodes',
            'code2op',
        }
        support.check__all__(self, pickletools, not_exported=not_exported)


class CommandLineTest(unittest.TestCase):
    def setUp(self):
        self.filename = tempfile.mktemp()
        self.addCleanup(os_helper.unlink, self.filename)

    @staticmethod
    def text_normalize(string):
        return textwrap.dedent(string).strip()

    def set_pickle_data(self, data):
        with open(self.filename, 'wb') as f:
            pickle.dump(data, f)

    def invoke_pickletools(self, *flags):
        with (
            support.captured_stdout() as stdout,
            support.captured_stderr() as stderr,
        ):
            pickletools._main(args=[*flags, self.filename])
        self.assertEqual(stderr.getvalue(), '')
        return self.text_normalize(stdout.getvalue())

    def check_output(self, data, expect, *flags):
        with self.subTest(data=data, flags=flags):
            self.set_pickle_data(data)
            res = self.invoke_pickletools(*flags)
            expect = self.text_normalize(expect)
            self.assertListEqual(res.splitlines(), expect.splitlines())

    def test_invocation(self):
        # test various combinations of parameters
        output_file = tempfile.mktemp()
        self.addCleanup(os_helper.unlink, output_file)
        base_flags = [
            (f'-o={output_file}', f'--output={output_file}'),
            ('-m', '--memo'),
            ('-l=2', '--indentlevel=2'),
            ('-a', '--annotate'),
            ('-p="Another:"', '--preamble="Another:"'),
        ]
        data = { 'a', 'b', 'c' }

        self.set_pickle_data(data)

        for r in range(1, len(base_flags) + 1):
            for choices in itertools.combinations(base_flags, r=r):
                for args in itertools.product(*choices):
                    with self.subTest(args=args[1:]):
                        self.invoke_pickletools(*args)

    def test_unknown_flag(self):
        with self.assertRaises(SystemExit):
            with support.captured_stderr() as stderr:
                pickletools._main(args=['--unknown'])
            self.assertStartsWith(stderr.getvalue(), 'usage: ')

    def test_output_flag(self):
        # test 'python -m pickletools -o/--output'
        output_file = tempfile.mktemp()
        self.addCleanup(os_helper.unlink, output_file)
        data = ('fake_data',)
        expect = r'''
            0: \x80 PROTO      5
            2: \x95 FRAME      15
           11: \x8c SHORT_BINUNICODE 'fake_data'
           22: \x94 MEMOIZE    (as 0)
           23: \x85 TUPLE1
           24: \x94 MEMOIZE    (as 1)
           25: .    STOP
        highest protocol among opcodes = 4
        '''
        for flag in [f'-o={output_file}', f'--output={output_file}']:
            with self.subTest(data=data, flags=flag):
                self.set_pickle_data(data)
                res = self.invoke_pickletools(flag)
                with open(output_file, 'r') as f:
                    res_from_file = self.text_normalize(f.read())
                expect = self.text_normalize(expect)

                self.assertListEqual(res.splitlines(), [])
                self.assertListEqual(res_from_file.splitlines(),
                                     expect.splitlines())

    def test_memo_flag(self):
        # test 'python -m pickletools -m/--memo'
        data = ('fake_data',)
        expect = r'''
            0: \x80 PROTO      5
            2: \x95 FRAME      15
           11: \x8c SHORT_BINUNICODE 'fake_data'
           22: \x94 MEMOIZE    (as 0)
           23: \x85 TUPLE1
           24: \x94 MEMOIZE    (as 1)
           25: .    STOP
        highest protocol among opcodes = 4
        '''
        for flag in ['-m', '--memo']:
            self.check_output(data, expect, flag)

    def test_indentlevel_flag(self):
        # test 'python -m pickletools -l/--indentlevel'
        data = ('fake_data',)
        expect = r'''
            0: \x80 PROTO      5
            2: \x95 FRAME      15
           11: \x8c SHORT_BINUNICODE 'fake_data'
           22: \x94 MEMOIZE    (as 0)
           23: \x85 TUPLE1
           24: \x94 MEMOIZE    (as 1)
           25: .    STOP
        highest protocol among opcodes = 4
        '''
        for flag in ['-l=2', '--indentlevel=2']:
            self.check_output(data, expect, flag)

    def test_annotate_flag(self):
        # test 'python -m pickletools -a/--annotate'
        data = ('fake_data',)
        expect = r'''
            0: \x80 PROTO      5              Protocol version indicator.
            2: \x95 FRAME      15             Indicate the beginning of a new frame.
           11: \x8c SHORT_BINUNICODE 'fake_data' Push a Python Unicode string object.
           22: \x94 MEMOIZE    (as 0)            Store the stack top into the memo.  The stack is not popped.
           23: \x85 TUPLE1                       Build a one-tuple out of the topmost item on the stack.
           24: \x94 MEMOIZE    (as 1)            Store the stack top into the memo.  The stack is not popped.
           25: .    STOP                         Stop the unpickling machine.
        highest protocol among opcodes = 4
        '''
        for flag in ['-a', '--annotate']:
            self.check_output(data, expect, flag)

    def test_preamble_flag(self):
        # test 'python -m pickletools -p/--preamble'
        data = ('fake_data',)
        expect = r'''
        Another:
            0: \x80 PROTO      5
            2: \x95 FRAME      15
           11: \x8c SHORT_BINUNICODE 'fake_data'
           22: \x94 MEMOIZE    (as 0)
           23: \x85 TUPLE1
           24: \x94 MEMOIZE    (as 1)
           25: .    STOP
        highest protocol among opcodes = 4
        Another:
            0: \x80 PROTO      5
            2: \x95 FRAME      15
           11: \x8c SHORT_BINUNICODE 'fake_data'
           22: \x94 MEMOIZE    (as 0)
           23: \x85 TUPLE1
           24: \x94 MEMOIZE    (as 1)
           25: .    STOP
        highest protocol among opcodes = 4
        '''
        for flag in ['-p=Another:', '--preamble=Another:']:
            with self.subTest(data=data, flags=flag):
                self.set_pickle_data(data)
                with support.captured_stdout() as stdout:
                    pickletools._main(args=[flag, self.filename, self.filename])
                res = self.text_normalize(stdout.getvalue())
                expect = self.text_normalize(expect)
                self.assertListEqual(res.splitlines(), expect.splitlines())


def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite(pickletools))
    return tests


if __name__ == "__main__":
    unittest.main()
