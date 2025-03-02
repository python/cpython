"""This module includes tests of the code object representation.

>>> def f(x):
...     def g(y):
...         return x + y
...     return g
...

>>> dump(f.__code__)
name: f
argcount: 1
posonlyargcount: 0
kwonlyargcount: 0
names: ()
varnames: ('x', 'g')
cellvars: ('x',)
freevars: ()
nlocals: 2
flags: 3
consts: ('<code object g>',)

>>> dump(f(4).__code__)
name: g
argcount: 1
posonlyargcount: 0
kwonlyargcount: 0
names: ()
varnames: ('y',)
cellvars: ()
freevars: ('x',)
nlocals: 1
flags: 19
consts: ('None',)

>>> def h(x, y):
...     a = x + y
...     b = x - y
...     c = a * b
...     return c
...

>>> dump(h.__code__)
name: h
argcount: 2
posonlyargcount: 0
kwonlyargcount: 0
names: ()
varnames: ('x', 'y', 'a', 'b', 'c')
cellvars: ()
freevars: ()
nlocals: 5
flags: 3
consts: ('None',)

>>> def attrs(obj):
...     print(obj.attr1)
...     print(obj.attr2)
...     print(obj.attr3)

>>> dump(attrs.__code__)
name: attrs
argcount: 1
posonlyargcount: 0
kwonlyargcount: 0
names: ('print', 'attr1', 'attr2', 'attr3')
varnames: ('obj',)
cellvars: ()
freevars: ()
nlocals: 1
flags: 3
consts: ('None',)

>>> def optimize_away():
...     'doc string'
...     'not a docstring'
...     53
...     0x53

>>> dump(optimize_away.__code__)
name: optimize_away
argcount: 0
posonlyargcount: 0
kwonlyargcount: 0
names: ()
varnames: ()
cellvars: ()
freevars: ()
nlocals: 0
flags: 67108867
consts: ("'doc string'", 'None')

>>> def keywordonly_args(a,b,*,k1):
...     return a,b,k1
...

>>> dump(keywordonly_args.__code__)
name: keywordonly_args
argcount: 2
posonlyargcount: 0
kwonlyargcount: 1
names: ()
varnames: ('a', 'b', 'k1')
cellvars: ()
freevars: ()
nlocals: 3
flags: 3
consts: ('None',)

>>> def posonly_args(a,b,/,c):
...     return a,b,c
...

>>> dump(posonly_args.__code__)
name: posonly_args
argcount: 3
posonlyargcount: 2
kwonlyargcount: 0
names: ()
varnames: ('a', 'b', 'c')
cellvars: ()
freevars: ()
nlocals: 3
flags: 3
consts: ('None',)

>>> def has_docstring(x: str):
...     'This is a one-line doc string'
...     x += x
...     x += "hello world"
...     # co_flags should be 0x4000003 = 67108867
...     return x

>>> dump(has_docstring.__code__)
name: has_docstring
argcount: 1
posonlyargcount: 0
kwonlyargcount: 0
names: ()
varnames: ('x',)
cellvars: ()
freevars: ()
nlocals: 1
flags: 67108867
consts: ("'This is a one-line doc string'", "'hello world'")

>>> async def async_func_docstring(x: str, y: str):
...     "This is a docstring from async function"
...     import asyncio
...     await asyncio.sleep(1)
...     # co_flags should be 0x4000083 = 67108995
...     return x + y

>>> dump(async_func_docstring.__code__)
name: async_func_docstring
argcount: 2
posonlyargcount: 0
kwonlyargcount: 0
names: ('asyncio', 'sleep')
varnames: ('x', 'y', 'asyncio')
cellvars: ()
freevars: ()
nlocals: 3
flags: 67108995
consts: ("'This is a docstring from async function'", 'None')

>>> def no_docstring(x, y, z):
...     return x + "hello" + y + z + "world"

>>> dump(no_docstring.__code__)
name: no_docstring
argcount: 3
posonlyargcount: 0
kwonlyargcount: 0
names: ()
varnames: ('x', 'y', 'z')
cellvars: ()
freevars: ()
nlocals: 3
flags: 3
consts: ("'hello'", "'world'")

>>> class class_with_docstring:
...     '''This is a docstring for class'''
...     '''This line is not docstring'''
...     pass

>>> print(class_with_docstring.__doc__)
This is a docstring for class

>>> class class_without_docstring:
...     pass

>>> print(class_without_docstring.__doc__)
None
"""

import copy
import inspect
import sys
import threading
import doctest
import unittest
import textwrap
import weakref
import dis

try:
    import ctypes
except ImportError:
    ctypes = None
from test.support import (cpython_only,
                          check_impl_detail, requires_debug_ranges,
                          gc_collect, Py_GIL_DISABLED)
from test.support.script_helper import assert_python_ok
from test.support import threading_helper, import_helper
from test.support.bytecode_helper import instructions_with_positions
from opcode import opmap, opname
from _testcapi import code_offset_to_line

COPY_FREE_VARS = opmap['COPY_FREE_VARS']


def consts(t):
    """Yield a doctest-safe sequence of object reprs."""
    for elt in t:
        r = repr(elt)
        if r.startswith("<code object"):
            yield "<code object %s>" % elt.co_name
        else:
            yield r

def dump(co):
    """Print out a text representation of a code object."""
    for attr in ["name", "argcount", "posonlyargcount",
                 "kwonlyargcount", "names", "varnames",
                 "cellvars", "freevars", "nlocals", "flags"]:
        print("%s: %s" % (attr, getattr(co, "co_" + attr)))
    print("consts:", tuple(consts(co.co_consts)))

# Needed for test_closure_injection below
# Defined at global scope to avoid implicitly closing over __class__
def external_getitem(self, i):
    return f"Foreign getitem: {super().__getitem__(i)}"

class CodeTest(unittest.TestCase):

    @cpython_only
    def test_newempty(self):
        _testcapi = import_helper.import_module("_testcapi")
        co = _testcapi.code_newempty("filename", "funcname", 15)
        self.assertEqual(co.co_filename, "filename")
        self.assertEqual(co.co_name, "funcname")
        self.assertEqual(co.co_firstlineno, 15)
        #Empty code object should raise, but not crash the VM
        with self.assertRaises(Exception):
            exec(co)

    @cpython_only
    def test_closure_injection(self):
        # From https://bugs.python.org/issue32176
        from types import FunctionType

        def create_closure(__class__):
            return (lambda: __class__).__closure__

        def new_code(c):
            '''A new code object with a __class__ cell added to freevars'''
            return c.replace(co_freevars=c.co_freevars + ('__class__',), co_code=bytes([COPY_FREE_VARS, 1])+c.co_code)

        def add_foreign_method(cls, name, f):
            code = new_code(f.__code__)
            assert not f.__closure__
            closure = create_closure(cls)
            defaults = f.__defaults__
            setattr(cls, name, FunctionType(code, globals(), name, defaults, closure))

        class List(list):
            pass

        add_foreign_method(List, "__getitem__", external_getitem)

        # Ensure the closure injection actually worked
        function = List.__getitem__
        class_ref = function.__closure__[0].cell_contents
        self.assertIs(class_ref, List)

        # Ensure the zero-arg super() call in the injected method works
        obj = List([1, 2, 3])
        self.assertEqual(obj[0], "Foreign getitem: 1")

    def test_constructor(self):
        def func(): pass
        co = func.__code__
        CodeType = type(co)

        # test code constructor
        CodeType(co.co_argcount,
                        co.co_posonlyargcount,
                        co.co_kwonlyargcount,
                        co.co_nlocals,
                        co.co_stacksize,
                        co.co_flags,
                        co.co_code,
                        co.co_consts,
                        co.co_names,
                        co.co_varnames,
                        co.co_filename,
                        co.co_name,
                        co.co_qualname,
                        co.co_firstlineno,
                        co.co_linetable,
                        co.co_exceptiontable,
                        co.co_freevars,
                        co.co_cellvars)

    def test_qualname(self):
        self.assertEqual(
            CodeTest.test_qualname.__code__.co_qualname,
            CodeTest.test_qualname.__qualname__
        )

    def test_replace(self):
        def func():
            x = 1
            return x
        code = func.__code__

        # Different co_name, co_varnames, co_consts.
        # Must have the same number of constants and
        # variables or we get crashes.
        def func2():
            y = 2
            return y
        code2 = func2.__code__

        for attr, value in (
            ("co_argcount", 0),
            ("co_posonlyargcount", 0),
            ("co_kwonlyargcount", 0),
            ("co_nlocals", 1),
            ("co_stacksize", 1),
            ("co_flags", code.co_flags | inspect.CO_COROUTINE),
            ("co_firstlineno", 100),
            ("co_code", code2.co_code),
            ("co_consts", code2.co_consts),
            ("co_names", ("myname",)),
            ("co_varnames", ('spam',)),
            ("co_freevars", ("freevar",)),
            ("co_cellvars", ("cellvar",)),
            ("co_filename", "newfilename"),
            ("co_name", "newname"),
            ("co_linetable", code2.co_linetable),
        ):
            with self.subTest(attr=attr, value=value):
                new_code = code.replace(**{attr: value})
                self.assertEqual(getattr(new_code, attr), value)
                new_code = copy.replace(code, **{attr: value})
                self.assertEqual(getattr(new_code, attr), value)

        new_code = code.replace(co_varnames=code2.co_varnames,
                                co_nlocals=code2.co_nlocals)
        self.assertEqual(new_code.co_varnames, code2.co_varnames)
        self.assertEqual(new_code.co_nlocals, code2.co_nlocals)
        new_code = copy.replace(code, co_varnames=code2.co_varnames,
                                co_nlocals=code2.co_nlocals)
        self.assertEqual(new_code.co_varnames, code2.co_varnames)
        self.assertEqual(new_code.co_nlocals, code2.co_nlocals)

    def test_nlocals_mismatch(self):
        def func():
            x = 1
            return x
        co = func.__code__
        assert co.co_nlocals > 0;

        # First we try the constructor.
        CodeType = type(co)
        for diff in (-1, 1):
            with self.assertRaises(ValueError):
                CodeType(co.co_argcount,
                         co.co_posonlyargcount,
                         co.co_kwonlyargcount,
                         # This is the only change.
                         co.co_nlocals + diff,
                         co.co_stacksize,
                         co.co_flags,
                         co.co_code,
                         co.co_consts,
                         co.co_names,
                         co.co_varnames,
                         co.co_filename,
                         co.co_name,
                         co.co_qualname,
                         co.co_firstlineno,
                         co.co_linetable,
                         co.co_exceptiontable,
                         co.co_freevars,
                         co.co_cellvars,
                         )
        # Then we try the replace method.
        with self.assertRaises(ValueError):
            co.replace(co_nlocals=co.co_nlocals - 1)
        with self.assertRaises(ValueError):
            co.replace(co_nlocals=co.co_nlocals + 1)

    def test_shrinking_localsplus(self):
        # Check that PyCode_NewWithPosOnlyArgs resizes both
        # localsplusnames and localspluskinds, if an argument is a cell.
        def func(arg):
            return lambda: arg
        code = func.__code__
        newcode = code.replace(co_name="func")  # Should not raise SystemError
        self.assertEqual(code, newcode)

    def test_empty_linetable(self):
        def func():
            pass
        new_code = code = func.__code__.replace(co_linetable=b'')
        self.assertEqual(list(new_code.co_lines()), [])

    def test_co_lnotab_is_deprecated(self):  # TODO: remove in 3.14
        def func():
            pass

        with self.assertWarns(DeprecationWarning):
            func.__code__.co_lnotab

    def test_invalid_bytecode(self):
        def foo():
            pass

        # assert that opcode 127 is invalid
        self.assertEqual(opname[127], '<127>')

        # change first opcode to 0x7f (=127)
        foo.__code__ = foo.__code__.replace(
            co_code=b'\x7f' + foo.__code__.co_code[1:])

        msg = "unknown opcode 127"
        with self.assertRaisesRegex(SystemError, msg):
            foo()

    @requires_debug_ranges()
    def test_co_positions_artificial_instructions(self):
        import dis

        namespace = {}
        exec(textwrap.dedent("""\
        try:
            1/0
        except Exception as e:
            exc = e
        """), namespace)

        exc = namespace['exc']
        traceback = exc.__traceback__
        code = traceback.tb_frame.f_code

        artificial_instructions = []
        for instr, positions in instructions_with_positions(
            dis.get_instructions(code), code.co_positions()
        ):
            # If any of the positions is None, then all have to
            # be None as well for the case above. There are still
            # some places in the compiler, where the artificial instructions
            # get assigned the first_lineno but they don't have other positions.
            # There is no easy way of inferring them at that stage, so for now
            # we don't support it.
            self.assertIn(positions.count(None), [0, 3, 4])

            if not any(positions):
                artificial_instructions.append(instr)

        self.assertEqual(
            [
                (instruction.opname, instruction.argval)
                for instruction in artificial_instructions
            ],
            [
                ("PUSH_EXC_INFO", None),
                ("LOAD_CONST", None), # artificial 'None'
                ("STORE_NAME", "e"),  # XX: we know the location for this
                ("DELETE_NAME", "e"),
                ("RERAISE", 1),
                ("COPY", 3),
                ("POP_EXCEPT", None),
                ("RERAISE", 1)
            ]
        )

    def test_endline_and_columntable_none_when_no_debug_ranges(self):
        # Make sure that if `-X no_debug_ranges` is used, there is
        # minimal debug info
        code = textwrap.dedent("""
            def f():
                pass

            positions = f.__code__.co_positions()
            for line, end_line, column, end_column in positions:
                assert line == end_line
                assert column is None
                assert end_column is None
            """)
        assert_python_ok('-X', 'no_debug_ranges', '-c', code)

    def test_endline_and_columntable_none_when_no_debug_ranges_env(self):
        # Same as above but using the environment variable opt out.
        code = textwrap.dedent("""
            def f():
                pass

            positions = f.__code__.co_positions()
            for line, end_line, column, end_column in positions:
                assert line == end_line
                assert column is None
                assert end_column is None
            """)
        assert_python_ok('-c', code, PYTHONNODEBUGRANGES='1')

    # co_positions behavior when info is missing.

    @requires_debug_ranges()
    def test_co_positions_empty_linetable(self):
        def func():
            x = 1
        new_code = func.__code__.replace(co_linetable=b'')
        positions = new_code.co_positions()
        for line, end_line, column, end_column in positions:
            self.assertIsNone(line)
            self.assertEqual(end_line, new_code.co_firstlineno + 1)

    def test_code_equality(self):
        def f():
            try:
                a()
            except:
                b()
            else:
                c()
            finally:
                d()
        code_a = f.__code__
        code_b = code_a.replace(co_linetable=b"")
        code_c = code_a.replace(co_exceptiontable=b"")
        code_d = code_b.replace(co_exceptiontable=b"")
        self.assertNotEqual(code_a, code_b)
        self.assertNotEqual(code_a, code_c)
        self.assertNotEqual(code_a, code_d)
        self.assertNotEqual(code_b, code_c)
        self.assertNotEqual(code_b, code_d)
        self.assertNotEqual(code_c, code_d)

    def test_code_hash_uses_firstlineno(self):
        c1 = (lambda: 1).__code__
        c2 = (lambda: 1).__code__
        self.assertNotEqual(c1, c2)
        self.assertNotEqual(hash(c1), hash(c2))
        c3 = c1.replace(co_firstlineno=17)
        self.assertNotEqual(c1, c3)
        self.assertNotEqual(hash(c1), hash(c3))

    def test_code_hash_uses_order(self):
        # Swapping posonlyargcount and kwonlyargcount should change the hash.
        c = (lambda x, y, *, z=1, w=1: 1).__code__
        self.assertEqual(c.co_argcount, 2)
        self.assertEqual(c.co_posonlyargcount, 0)
        self.assertEqual(c.co_kwonlyargcount, 2)
        swapped = c.replace(co_posonlyargcount=2, co_kwonlyargcount=0)
        self.assertNotEqual(c, swapped)
        self.assertNotEqual(hash(c), hash(swapped))

    def test_code_hash_uses_bytecode(self):
        c = (lambda x, y: x + y).__code__
        d = (lambda x, y: x * y).__code__
        c1 = c.replace(co_code=d.co_code)
        self.assertNotEqual(c, c1)
        self.assertNotEqual(hash(c), hash(c1))

    @cpython_only
    def test_code_equal_with_instrumentation(self):
        """ GH-109052

        Make sure the instrumentation doesn't affect the code equality
        The validity of this test relies on the fact that "x is x" and
        "x in x" have only one different instruction and the instructions
        have the same argument.

        """
        code1 = compile("x is x", "example.py", "eval")
        code2 = compile("x in x", "example.py", "eval")
        sys._getframe().f_trace_opcodes = True
        sys.settrace(lambda *args: None)
        exec(code1, {'x': []})
        exec(code2, {'x': []})
        self.assertNotEqual(code1, code2)
        sys.settrace(None)


def isinterned(s):
    return s is sys.intern(('_' + s + '_')[1:-1])

class CodeConstsTest(unittest.TestCase):

    def find_const(self, consts, value):
        for v in consts:
            if v == value:
                return v
        self.assertIn(value, consts)  # raises an exception
        self.fail('Should never be reached')

    def assertIsInterned(self, s):
        if not isinterned(s):
            self.fail('String %r is not interned' % (s,))

    def assertIsNotInterned(self, s):
        if isinterned(s):
            self.fail('String %r is interned' % (s,))

    @cpython_only
    def test_interned_string(self):
        co = compile('res = "str_value"', '?', 'exec')
        v = self.find_const(co.co_consts, 'str_value')
        self.assertIsInterned(v)

    @cpython_only
    def test_interned_string_in_tuple(self):
        co = compile('res = ("str_value",)', '?', 'exec')
        v = self.find_const(co.co_consts, ('str_value',))
        self.assertIsInterned(v[0])

    @cpython_only
    def test_interned_string_in_frozenset(self):
        co = compile('res = a in {"str_value"}', '?', 'exec')
        v = self.find_const(co.co_consts, frozenset(('str_value',)))
        self.assertIsInterned(tuple(v)[0])

    @cpython_only
    def test_interned_string_default(self):
        def f(a='str_value'):
            return a
        self.assertIsInterned(f())

    @cpython_only
    @unittest.skipIf(Py_GIL_DISABLED, "free-threaded build interns all string constants")
    def test_interned_string_with_null(self):
        co = compile(r'res = "str\0value!"', '?', 'exec')
        v = self.find_const(co.co_consts, 'str\0value!')
        self.assertIsNotInterned(v)

    @cpython_only
    @unittest.skipUnless(Py_GIL_DISABLED, "does not intern all constants")
    def test_interned_constants(self):
        # compile separately to avoid compile time de-duping

        globals = {}
        exec(textwrap.dedent("""
            def func1():
                return (0.0, (1, 2, "hello"))
        """), globals)

        exec(textwrap.dedent("""
            def func2():
                return (0.0, (1, 2, "hello"))
        """), globals)

        self.assertTrue(globals["func1"]() is globals["func2"]())


class CodeWeakRefTest(unittest.TestCase):

    def test_basic(self):
        # Create a code object in a clean environment so that we know we have
        # the only reference to it left.
        namespace = {}
        exec("def f(): pass", globals(), namespace)
        f = namespace["f"]
        del namespace

        self.called = False
        def callback(code):
            self.called = True

        # f is now the last reference to the function, and through it, the code
        # object.  While we hold it, check that we can create a weakref and
        # deref it.  Then delete it, and check that the callback gets called and
        # the reference dies.
        coderef = weakref.ref(f.__code__, callback)
        self.assertTrue(bool(coderef()))
        del f
        gc_collect()  # For PyPy or other GCs.
        self.assertFalse(bool(coderef()))
        self.assertTrue(self.called)

# Python implementation of location table parsing algorithm
def read(it):
    return next(it)

def read_varint(it):
    b = read(it)
    val = b & 63;
    shift = 0;
    while b & 64:
        b = read(it)
        shift += 6
        val |= (b&63) << shift
    return val

def read_signed_varint(it):
    uval = read_varint(it)
    if uval & 1:
        return -(uval >> 1)
    else:
        return uval >> 1

def parse_location_table(code):
    line = code.co_firstlineno
    it = iter(code.co_linetable)
    while True:
        try:
            first_byte = read(it)
        except StopIteration:
            return
        code = (first_byte >> 3) & 15
        length = (first_byte & 7) + 1
        if code == 15:
            yield (code, length, None, None, None, None)
        elif code == 14:
            line_delta = read_signed_varint(it)
            line += line_delta
            end_line = line + read_varint(it)
            col = read_varint(it)
            if col == 0:
                col = None
            else:
                col -= 1
            end_col = read_varint(it)
            if end_col == 0:
                end_col = None
            else:
                end_col -= 1
            yield (code, length, line, end_line, col, end_col)
        elif code == 13: # No column
            line_delta = read_signed_varint(it)
            line += line_delta
            yield (code, length, line, line, None, None)
        elif code in (10, 11, 12): # new line
            line_delta = code - 10
            line += line_delta
            column = read(it)
            end_column = read(it)
            yield (code, length, line, line, column, end_column)
        else:
            assert (0 <= code < 10)
            second_byte = read(it)
            column = code << 3 | (second_byte >> 4)
            yield (code, length, line, line, column, column + (second_byte & 15))

def positions_from_location_table(code):
    for _, length, line, end_line, col, end_col in parse_location_table(code):
        for _ in range(length):
            yield (line, end_line, col, end_col)

def dedup(lst, prev=object()):
    for item in lst:
        if item != prev:
            yield item
            prev = item

def lines_from_postions(positions):
    return dedup(l for (l, _, _, _) in positions)

def misshappen():
    """





    """
    x = (


        4

        +

        y

    )
    y = (
        a
        +
            b
                +

                d
        )
    return q if (

        x

        ) else p

def bug93662():
    example_report_generation_message= (
            """
            """
    ).strip()
    raise ValueError()


class CodeLocationTest(unittest.TestCase):

    def check_positions(self, func):
        pos1 = list(func.__code__.co_positions())
        pos2 = list(positions_from_location_table(func.__code__))
        for l1, l2 in zip(pos1, pos2):
            self.assertEqual(l1, l2)
        self.assertEqual(len(pos1), len(pos2))

    def test_positions(self):
        self.check_positions(parse_location_table)
        self.check_positions(misshappen)
        self.check_positions(bug93662)

    def check_lines(self, func):
        co = func.__code__
        lines1 = [line for _, _, line in co.co_lines()]
        self.assertEqual(lines1, list(dedup(lines1)))
        lines2 = list(lines_from_postions(positions_from_location_table(co)))
        for l1, l2 in zip(lines1, lines2):
            self.assertEqual(l1, l2)
        self.assertEqual(len(lines1), len(lines2))

    def test_lines(self):
        self.check_lines(parse_location_table)
        self.check_lines(misshappen)
        self.check_lines(bug93662)

    @cpython_only
    def test_code_new_empty(self):
        # If this test fails, it means that the construction of PyCode_NewEmpty
        # needs to be modified! Please update this test *and* PyCode_NewEmpty,
        # so that they both stay in sync.
        def f():
            pass
        PY_CODE_LOCATION_INFO_NO_COLUMNS = 13
        f.__code__ = f.__code__.replace(
            co_stacksize=1,
            co_firstlineno=42,
            co_code=bytes(
                [
                    dis.opmap["RESUME"], 0,
                    dis.opmap["LOAD_COMMON_CONSTANT"], 0,
                    dis.opmap["RAISE_VARARGS"], 1,
                ]
            ),
            co_linetable=bytes(
                [
                    (1 << 7)
                    | (PY_CODE_LOCATION_INFO_NO_COLUMNS << 3)
                    | (3 - 1),
                    0,
                ]
            ),
        )
        self.assertRaises(AssertionError, f)
        self.assertEqual(
            list(f.__code__.co_positions()),
            3 * [(42, 42, None, None)],
        )

    @cpython_only
    def test_docstring_under_o2(self):
        code = textwrap.dedent('''
            def has_docstring(x, y):
                """This is a first-line doc string"""
                """This is a second-line doc string"""
                a = x + y
                b = x - y
                return a, b


            def no_docstring(x):
                def g(y):
                    return x + y
                return g


            async def async_func():
                """asynf function doc string"""
                pass


            for func in [has_docstring, no_docstring(4), async_func]:
                assert(func.__doc__ is None)
            ''')

        rc, out, err = assert_python_ok('-OO', '-c', code)

    def test_co_branches(self):

        def get_line_branches(func):
            code = func.__code__
            base = code.co_firstlineno
            return [
                (
                    code_offset_to_line(code, src) - base,
                    code_offset_to_line(code, left) - base,
                    code_offset_to_line(code, right) - base
                ) for (src, left, right) in
                code.co_branches()
            ]

        def simple(x):
            if x:
                A
            else:
                B

        self.assertEqual(
            get_line_branches(simple),
            [(1,2,4)])

        def with_extended_args(x):
            if x:
                A.x; A.x; A.x; A.x; A.x; A.x;
                A.x; A.x; A.x; A.x; A.x; A.x;
                A.x; A.x; A.x; A.x; A.x; A.x;
                A.x; A.x; A.x; A.x; A.x; A.x;
                A.x; A.x; A.x; A.x; A.x; A.x;
            else:
                B

        self.assertEqual(
            get_line_branches(with_extended_args),
            [(1,2,8)])

if check_impl_detail(cpython=True) and ctypes is not None:
    py = ctypes.pythonapi
    freefunc = ctypes.CFUNCTYPE(None,ctypes.c_voidp)

    RequestCodeExtraIndex = py.PyUnstable_Eval_RequestCodeExtraIndex
    RequestCodeExtraIndex.argtypes = (freefunc,)
    RequestCodeExtraIndex.restype = ctypes.c_ssize_t

    SetExtra = py.PyUnstable_Code_SetExtra
    SetExtra.argtypes = (ctypes.py_object, ctypes.c_ssize_t, ctypes.c_voidp)
    SetExtra.restype = ctypes.c_int

    GetExtra = py.PyUnstable_Code_GetExtra
    GetExtra.argtypes = (ctypes.py_object, ctypes.c_ssize_t,
                         ctypes.POINTER(ctypes.c_voidp))
    GetExtra.restype = ctypes.c_int

    LAST_FREED = None
    def myfree(ptr):
        global LAST_FREED
        LAST_FREED = ptr

    FREE_FUNC = freefunc(myfree)
    FREE_INDEX = RequestCodeExtraIndex(FREE_FUNC)

    class CoExtra(unittest.TestCase):
        def get_func(self):
            # Defining a function causes the containing function to have a
            # reference to the code object.  We need the code objects to go
            # away, so we eval a lambda.
            return eval('lambda:42')

        def test_get_non_code(self):
            f = self.get_func()

            self.assertRaises(SystemError, SetExtra, 42, FREE_INDEX,
                              ctypes.c_voidp(100))
            self.assertRaises(SystemError, GetExtra, 42, FREE_INDEX,
                              ctypes.c_voidp(100))

        def test_bad_index(self):
            f = self.get_func()
            self.assertRaises(SystemError, SetExtra, f.__code__,
                              FREE_INDEX+100, ctypes.c_voidp(100))
            self.assertEqual(GetExtra(f.__code__, FREE_INDEX+100,
                              ctypes.c_voidp(100)), 0)

        def test_free_called(self):
            # Verify that the provided free function gets invoked
            # when the code object is cleaned up.
            f = self.get_func()

            SetExtra(f.__code__, FREE_INDEX, ctypes.c_voidp(100))
            del f
            gc_collect()  # For free-threaded build
            self.assertEqual(LAST_FREED, 100)

        def test_get_set(self):
            # Test basic get/set round tripping.
            f = self.get_func()

            extra = ctypes.c_voidp()

            SetExtra(f.__code__, FREE_INDEX, ctypes.c_voidp(200))
            # reset should free...
            SetExtra(f.__code__, FREE_INDEX, ctypes.c_voidp(300))
            self.assertEqual(LAST_FREED, 200)

            extra = ctypes.c_voidp()
            GetExtra(f.__code__, FREE_INDEX, extra)
            self.assertEqual(extra.value, 300)
            del f

        @threading_helper.requires_working_threading()
        def test_free_different_thread(self):
            # Freeing a code object on a different thread then
            # where the co_extra was set should be safe.
            f = self.get_func()
            class ThreadTest(threading.Thread):
                def __init__(self, f, test):
                    super().__init__()
                    self.f = f
                    self.test = test
                def run(self):
                    del self.f
                    gc_collect()
                    # gh-117683: In the free-threaded build, the code object's
                    # destructor may still be running concurrently in the main
                    # thread.
                    if not Py_GIL_DISABLED:
                        self.test.assertEqual(LAST_FREED, 500)

            SetExtra(f.__code__, FREE_INDEX, ctypes.c_voidp(500))
            tt = ThreadTest(f, self)
            del f
            tt.start()
            tt.join()
            gc_collect()  # For free-threaded build
            self.assertEqual(LAST_FREED, 500)


def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite())
    return tests


if __name__ == "__main__":
    unittest.main()
