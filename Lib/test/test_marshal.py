from test import support
from test.support import is_apple_mobile, os_helper, requires_debug_ranges, is_emscripten
from test.support import threading_helper
from test.support.script_helper import assert_python_ok
import array
import io
import marshal
import struct
import sys
import threading
import unittest
import os
import types
import textwrap

try:
    import _testcapi
except ImportError:
    _testcapi = None

class HelperMixin:
    def helper(self, sample, *extra):
        new = marshal.loads(marshal.dumps(sample, *extra))
        self.assertEqual(sample, new)
        try:
            with open(os_helper.TESTFN, "wb") as f:
                marshal.dump(sample, f, *extra)
            with open(os_helper.TESTFN, "rb") as f:
                new = marshal.load(f)
            self.assertEqual(sample, new)
        finally:
            os_helper.unlink(os_helper.TESTFN)

def omit_last_byte(data):
    """return data[:-1]"""
    # This file's code is used in CompatibilityTestCase,
    # but slices need marshal version 5.
    # Avoid the slice literal.
    return data[slice(0, -1)]

class IntTestCase(unittest.TestCase, HelperMixin):
    def test_ints(self):
        # Test a range of Python ints larger than the machine word size.
        n = sys.maxsize ** 2
        while n:
            for expected in (-n, n):
                self.helper(expected)
            n = n >> 1
        n = 1 << 100
        while n:
            for expected in (-n, -n+1, n-1, n):
                self.helper(expected)
            n = n >> 1

    def test_int64(self):
        # Simulate int marshaling with TYPE_INT64.
        maxint64 = (1 << 63) - 1
        minint64 = -maxint64-1
        for base in maxint64, minint64, -maxint64, -(minint64 >> 1):
            while base:
                s = b'I' + int.to_bytes(base, 8, 'little', signed=True)
                got = marshal.loads(s)
                self.assertEqual(base, got)
                if base == -1:  # a fixed-point for shifting right 1
                    base = 0
                else:
                    base >>= 1

        got = marshal.loads(b'I\xfe\xdc\xba\x98\x76\x54\x32\x10')
        self.assertEqual(got, 0x1032547698badcfe)
        got = marshal.loads(b'I\x01\x23\x45\x67\x89\xab\xcd\xef')
        self.assertEqual(got, -0x1032547698badcff)
        got = marshal.loads(b'I\x08\x19\x2a\x3b\x4c\x5d\x6e\x7f')
        self.assertEqual(got, 0x7f6e5d4c3b2a1908)
        got = marshal.loads(b'I\xf7\xe6\xd5\xc4\xb3\xa2\x91\x80')
        self.assertEqual(got, -0x7f6e5d4c3b2a1909)

    def test_bool(self):
        for b in (True, False):
            self.helper(b)

class FloatTestCase(unittest.TestCase, HelperMixin):
    def test_floats(self):
        # Test a few floats
        small = 1e-25
        n = sys.maxsize * 3.7e250
        while n > small:
            for expected in (-n, n):
                self.helper(float(expected))
            n /= 123.4567

        f = 0.0
        s = marshal.dumps(f, 2)
        got = marshal.loads(s)
        self.assertEqual(f, got)
        # and with version <= 1 (floats marshalled differently then)
        s = marshal.dumps(f, 1)
        got = marshal.loads(s)
        self.assertEqual(f, got)

        n = sys.maxsize * 3.7e-250
        while n < small:
            for expected in (-n, n):
                f = float(expected)
                self.helper(f)
                self.helper(f, 1)
            n *= 123.4567

class StringTestCase(unittest.TestCase, HelperMixin):
    def test_unicode(self):
        for s in ["", "Andr\xe8 Previn", "abc", " "*10000]:
            self.helper(marshal.loads(marshal.dumps(s)))

    def test_string(self):
        for s in ["", "Andr\xe8 Previn", "abc", " "*10000]:
            self.helper(s)

    def test_bytes(self):
        for s in [b"", b"Andr\xe8 Previn", b"abc", b" "*10000]:
            self.helper(s)

    @support.cpython_only
    def test_bytes_singleton(self):
        for version in range(marshal.version + 1):
            for sample in [b"", b"x"]:
                new = marshal.loads(marshal.dumps(sample, version))
                self.assertIs(new, sample)


class ExceptionTestCase(unittest.TestCase):
    def test_exceptions(self):
        new = marshal.loads(marshal.dumps(StopIteration))
        self.assertEqual(StopIteration, new)

class CodeTestCase(unittest.TestCase):
    def test_code(self):
        co = ExceptionTestCase.test_exceptions.__code__
        new = marshal.loads(marshal.dumps(co))
        self.assertEqual(co, new)

    def test_many_codeobjects(self):
        # Issue2957: bad recursion count on code objects
        # more than MAX_MARSHAL_STACK_DEPTH
        codes = (ExceptionTestCase.test_exceptions.__code__,) * 10_000
        marshal.loads(marshal.dumps(codes))

    def test_different_filenames(self):
        co1 = compile("x", "f1", "exec")
        co2 = compile("y", "f2", "exec")
        co1, co2 = marshal.loads(marshal.dumps((co1, co2)))
        self.assertEqual(co1.co_filename, "f1")
        self.assertEqual(co2.co_filename, "f2")

    def test_no_allow_code(self):
        data = {'a': [({0},)]}
        dump = marshal.dumps(data, allow_code=False)
        self.assertEqual(marshal.loads(dump, allow_code=False), data)

        f = io.BytesIO()
        marshal.dump(data, f, allow_code=False)
        f.seek(0)
        self.assertEqual(marshal.load(f, allow_code=False), data)

        co = ExceptionTestCase.test_exceptions.__code__
        data = {'a': [({co, 0},)]}
        dump = marshal.dumps(data, allow_code=True)
        self.assertEqual(marshal.loads(dump, allow_code=True), data)
        with self.assertRaises(ValueError):
            marshal.dumps(data, allow_code=False)
        with self.assertRaises(ValueError):
            marshal.loads(dump, allow_code=False)

        marshal.dump(data, io.BytesIO(), allow_code=True)
        self.assertEqual(marshal.load(io.BytesIO(dump), allow_code=True), data)
        with self.assertRaises(ValueError):
            marshal.dump(data, io.BytesIO(), allow_code=False)
        with self.assertRaises(ValueError):
            marshal.load(io.BytesIO(dump), allow_code=False)

    @requires_debug_ranges()
    def test_minimal_linetable_with_no_debug_ranges(self):
        # Make sure when demarshalling objects with `-X no_debug_ranges`
        # that the columns are None.
        co = ExceptionTestCase.test_exceptions.__code__
        code = textwrap.dedent("""
        import sys
        import marshal
        with open(sys.argv[1], 'rb') as f:
            co = marshal.load(f)
            positions = list(co.co_positions())
            assert positions[0][2] is None
            assert positions[0][3] is None
        """)

        try:
            with open(os_helper.TESTFN, 'wb') as f:
                marshal.dump(co, f)

            assert_python_ok('-X', 'no_debug_ranges',
                             '-c', code, os_helper.TESTFN)
        finally:
            os_helper.unlink(os_helper.TESTFN)

    @support.cpython_only
    def test_same_filename_used(self):
        s = """def f(): pass\ndef g(): pass"""
        co = compile(s, "myfile", "exec")
        co = marshal.loads(marshal.dumps(co))
        for obj in co.co_consts:
            if isinstance(obj, types.CodeType):
                self.assertIs(co.co_filename, obj.co_filename)

class ContainerTestCase(unittest.TestCase, HelperMixin):
    d = {'astring': 'foo@bar.baz.spam',
         'afloat': 7283.43,
         'anint': 2**20,
         'ashortlong': 2,
         'alist': ['.zyx.41'],
         'atuple': ('.zyx.41',)*10,
         'aboolean': False,
         'aunicode': "Andr\xe8 Previn"
         }

    def test_dict(self):
        self.helper(self.d)

    def test_list(self):
        self.helper(list(self.d.items()))

    def test_tuple(self):
        self.helper(tuple(self.d.keys()))

    def test_sets(self):
        for constructor in (set, frozenset):
            self.helper(constructor(self.d.keys()))


class BufferTestCase(unittest.TestCase, HelperMixin):

    def test_bytearray(self):
        b = bytearray(b"abc")
        self.helper(b)
        new = marshal.loads(marshal.dumps(b))
        self.assertEqual(type(new), bytes)

    def test_memoryview(self):
        b = memoryview(b"abc")
        self.helper(b)
        new = marshal.loads(marshal.dumps(b))
        self.assertEqual(type(new), bytes)

    def test_array(self):
        a = array.array('B', b"abc")
        new = marshal.loads(marshal.dumps(a))
        self.assertEqual(new, b"abc")


class BugsTestCase(unittest.TestCase):
    def test_bug_5888452(self):
        # Simple-minded check for SF 588452: Debug build crashes
        marshal.dumps([128] * 1000)

    def test_patch_873224(self):
        self.assertRaises(Exception, marshal.loads, b'0')
        self.assertRaises(Exception, marshal.loads, b'f')
        self.assertRaises(Exception, marshal.loads,
                          omit_last_byte(marshal.dumps(2**65)))

    def test_version_argument(self):
        # Python 2.4.0 crashes for any call to marshal.dumps(x, y)
        self.assertEqual(marshal.loads(marshal.dumps(5, 0)), 5)
        self.assertEqual(marshal.loads(marshal.dumps(5, 1)), 5)

    def test_fuzz(self):
        # simple test that it's at least not *totally* trivial to
        # crash from bad marshal data
        for i in range(256):
            c = bytes([i])
            try:
                marshal.loads(c)
            except Exception:
                pass

    def test_loads_recursion(self):
        def run_tests(N, check):
            # (((...None...),),)
            check(b')\x01' * N + b'N')
            check(b'(\x01\x00\x00\x00' * N + b'N')
            # [[[...None...]]]
            check(b'[\x01\x00\x00\x00' * N + b'N')
            # {None: {None: {None: ...None...}}}
            check(b'{N' * N + b'N' + b'0' * N)
            # frozenset([frozenset([frozenset([...None...])])])
            check(b'>\x01\x00\x00\x00' * N + b'N')
        # Check that the generated marshal data is valid and marshal.loads()
        # works for moderately deep nesting
        run_tests(100, marshal.loads)
        # Very deeply nested structure shouldn't blow the stack
        def check(s):
            self.assertRaises(ValueError, marshal.loads, s)
        run_tests(2**20, check)

    def test_recursion_limit(self):
        # Create a deeply nested structure.
        head = last = []
        # The max stack depth should match the value in Python/marshal.c.
        # BUG: https://bugs.python.org/issue33720
        # Windows always limits the maximum depth on release and debug builds
        #if os.name == 'nt' and support.Py_DEBUG:
        if os.name == 'nt':
            MAX_MARSHAL_STACK_DEPTH = 1000
        elif sys.platform == 'wasi' or is_emscripten or is_apple_mobile:
            MAX_MARSHAL_STACK_DEPTH = 1500
        else:
            MAX_MARSHAL_STACK_DEPTH = 2000
        for i in range(MAX_MARSHAL_STACK_DEPTH - 2):
            last.append([0])
            last = last[-1]

        # Verify we don't blow out the stack with dumps/load.
        data = marshal.dumps(head)
        new_head = marshal.loads(data)
        # Don't use == to compare objects, it can exceed the recursion limit.
        self.assertEqual(len(new_head), len(head))
        self.assertEqual(len(new_head[0]), len(head[0]))
        self.assertEqual(len(new_head[-1]), len(head[-1]))

        last.append([0])
        self.assertRaises(ValueError, marshal.dumps, head)

    def test_reference_loop_list(self):
        a = []
        a.append(a)
        for v in range(3):
            self.assertRaises(ValueError, marshal.dumps, a, v)
        for v in range(3, marshal.version + 1):
            d = marshal.dumps(a, v)
            b = marshal.loads(d)
            self.assertIsInstance(b, list)
            self.assertIs(b[0], b)

    def test_reference_loop_dict(self):
        a = {}
        a[None] = a
        for v in range(3):
            self.assertRaises(ValueError, marshal.dumps, a, v)
        for v in range(3, marshal.version + 1):
            d = marshal.dumps(a, v)
            b = marshal.loads(d)
            self.assertIsInstance(b, dict)
            self.assertIs(b[None], b)

    def test_reference_loop_tuple(self):
        a = ([],)
        a[0].append(a)
        for v in range(3):
            self.assertRaises(ValueError, marshal.dumps, a, v)
        for v in range(3, marshal.version + 1):
            d = marshal.dumps(a, v)
            b = marshal.loads(d)
            self.assertIsInstance(b, tuple)
            self.assertIsInstance(b[0], list)
            self.assertIs(b[0][0], b)

    def test_reference_loop_code(self):
        def f():
            return 1234.5
        code = f.__code__
        a = []
        code = code.replace(co_consts=code.co_consts + (a,))
        # This test creates a reference loop which leads to reference leaks,
        # so we need to break the loop manually. See gh-148722.
        self.addCleanup(a.clear)
        a.append(code)
        for v in range(marshal.version + 1):
            self.assertRaises(ValueError, marshal.dumps, code, v)

    def test_reference_loop_slice(self):
        a = slice([], None)
        a.start.append(a)
        for v in range(marshal.version + 1):
            self.assertRaises(ValueError, marshal.dumps, a, v)

        a = slice(None, [])
        a.stop.append(a)
        for v in range(marshal.version + 1):
            self.assertRaises(ValueError, marshal.dumps, a, v)

        a = slice(None, None, [])
        a.step.append(a)
        for v in range(marshal.version + 1):
            self.assertRaises(ValueError, marshal.dumps, a, v)

    def test_reference_loop_frozendict(self):
        a = frozendict({None: []})
        a[None].append(a)
        for v in range(marshal.version + 1):
            self.assertRaises(ValueError, marshal.dumps, a, v)

    def test_shared_reference_frozendict(self):
        # A frozendict referenced more than once must round-trip with the
        # shared identity preserved, like frozenset.
        fd = frozendict({'a': 1, 'b': 2})
        out = marshal.loads(marshal.dumps([fd, fd]))
        self.assertEqual(out[0], fd)
        self.assertIs(out[0], out[1])
        nested = marshal.loads(marshal.dumps(frozendict({'x': fd, 'y': fd})))
        self.assertIs(nested['x'], nested['y'])

    def test_loads_reference_loop_list(self):
        data = b'\xdb\x01\x00\x00\x00r\x00\x00\x00\x00' # [<R>]
        a = marshal.loads(data)
        self.assertIsInstance(a, list)
        self.assertIs(a[0], a)

    def test_loads_reference_loop_dict(self):
        data = b'\xfbNr\x00\x00\x00\x000' # {None: <R>}
        a = marshal.loads(data)
        self.assertIsInstance(a, dict)
        self.assertIs(a[None], a)

    def test_loads_abnormal_reference_loops(self):
        # Indirect self-references of tuples.
        data = b'\xa8\x01\x00\x00\x00[\x01\x00\x00\x00r\x00\x00\x00\x00' # ([<R>],)
        a = marshal.loads(data)
        self.assertIsInstance(a, tuple)
        self.assertIsInstance(a[0], list)
        self.assertIs(a[0][0], a)

        data = b'\xa8\x01\x00\x00\x00{Nr\x00\x00\x00\x000' # ({None: <R>},)
        a = marshal.loads(data)
        self.assertIsInstance(a, tuple)
        self.assertIsInstance(a[0], dict)
        self.assertIs(a[0][None], a)

        # Direct self-reference which cannot be created in Python.
        # This creates a reference loop which cannot be collected.
        if False:
            data = b'\xa8\x01\x00\x00\x00r\x00\x00\x00\x00' # (<R>,)
            a = marshal.loads(data)
            self.assertIsInstance(a, tuple)
            self.assertIs(a[0], a)

        # Direct self-references which cannot be created in Python
        # because of unhashability.
        data = b'\xfbr\x00\x00\x00\x00N0' # {<R>: None}
        self.assertRaises(TypeError, marshal.loads, data)
        data = b'\xbc\x01\x00\x00\x00r\x00\x00\x00\x00' # {<R>}
        self.assertRaises(TypeError, marshal.loads, data)

        for data in [
            # Indirect self-references of immutable objects.
            b'\xba[\x01\x00\x00\x00r\x00\x00\x00\x00NN', # slice([<R>], None)
            b'\xbaN[\x01\x00\x00\x00r\x00\x00\x00\x00N', # slice(None, [<R>])
            b'\xbaNN[\x01\x00\x00\x00r\x00\x00\x00\x00', # slice(None, None, [<R>])
            b'\xba{Nr\x00\x00\x00\x000NN', # slice({None: <R>}, None)
            b'\xbaN{Nr\x00\x00\x00\x000N', # slice(None, {None: <R>})
            b'\xbaNN{Nr\x00\x00\x00\x000', # slice(None, None, {None: <R>})
            b'\xfdN[\x01\x00\x00\x00r\x00\x00\x00\x000', # frozendict({None: [<R>]})
            b'\xfdN{Nr\x00\x00\x00\x0000', # frozendict({None: {None: <R>})

            # Direct self-references which cannot be created in Python.
            b'\xbe\x01\x00\x00\x00r\x00\x00\x00\x00', # frozenset({<R>})
            b'\xfdNr\x00\x00\x00\x000', # frozendict({None: <R>})
            b'\xfdr\x00\x00\x00\x00N0', # frozendict({<R>: None})
            b'\xbar\x00\x00\x00\x00NN', # slice(<R>, None)
            b'\xbaNr\x00\x00\x00\x00N', # slice(None, <R>)
            b'\xbaNNr\x00\x00\x00\x00', # slice(None, None, <R>)
        ]:
            with self.subTest(data=data):
                self.assertRaises(ValueError, marshal.loads, data)

    def test_exact_type_match(self):
        # Former bug:
        #   >>> class Int(int): pass
        #   >>> type(loads(dumps(Int())))
        #   <type 'int'>
        for typ in (int, float, complex, tuple, list, dict, set, frozenset):
            # Note: str subclasses are not tested because they get handled
            # by marshal's routines for objects supporting the buffer API.
            subtyp = type('subtyp', (typ,), {})
            self.assertRaises(ValueError, marshal.dumps, subtyp())

    # Issue #1792 introduced a change in how marshal increases the size of its
    # internal buffer; this test ensures that the new code is exercised.
    def test_large_marshal(self):
        size = int(1e6)
        testString = 'abc' * size
        marshal.dumps(testString)

    def test_invalid_longs(self):
        # Issue #7019: marshal.loads shouldn't produce unnormalized PyLongs
        invalid_string = b'l\x02\x00\x00\x00\x00\x00\x00\x00'
        self.assertRaises(ValueError, marshal.loads, invalid_string)

    def test_multiple_dumps_and_loads(self):
        # Issue 12291: marshal.load() should be callable multiple times
        # with interleaved data written by non-marshal code
        # Adapted from a patch by Engelbert Gruber.
        data = (1, 'abc', b'def', 1.0, (2, 'a', ['b', b'c']))
        for interleaved in (b'', b'0123'):
            ilen = len(interleaved)
            positions = []
            try:
                with open(os_helper.TESTFN, 'wb') as f:
                    for d in data:
                        marshal.dump(d, f)
                        if ilen:
                            f.write(interleaved)
                        positions.append(f.tell())
                with open(os_helper.TESTFN, 'rb') as f:
                    for i, d in enumerate(data):
                        self.assertEqual(d, marshal.load(f))
                        if ilen:
                            f.read(ilen)
                        self.assertEqual(positions[i], f.tell())
            finally:
                os_helper.unlink(os_helper.TESTFN)

    def test_loads_reject_unicode_strings(self):
        # Issue #14177: marshal.loads() should not accept unicode strings
        unicode_string = 'T'
        self.assertRaises(TypeError, marshal.loads, unicode_string)

    def test_bad_reader(self):
        class BadReader(io.BytesIO):
            def readinto(self, buf):
                n = super().readinto(buf)
                if n is not None and n > 4:
                    n += 10**6
                return n
        for value in (1.0, 1j, b'0123456789', '0123456789'):
            self.assertRaises(ValueError, marshal.load,
                              BadReader(marshal.dumps(value)))

    def test_eof(self):
        data = marshal.dumps(("hello", "dolly", None))
        for i in range(len(data)):
            self.assertRaises(EOFError, marshal.loads, data[0: i])

    def test_deterministic_sets(self):
        # bpo-37596: To support reproducible builds, sets and frozensets need to
        # have their elements serialized in a consistent order (even when they
        # have been scrambled by hash randomization):
        for kind in ("set", "frozenset"):
            for elements in (
                "float('nan'), b'a', b'b', b'c', 'x', 'y', 'z'",
                # Also test for bad interactions with backreferencing:
                "('Spam', 0), ('Spam', 1), ('Spam', 2), ('Spam', 3), ('Spam', 4), ('Spam', 5)",
            ):
                s = f"{kind}([{elements}])"
                with self.subTest(s):
                    # First, make sure that our test case still has different
                    # orders under hash seeds 0 and 1. If this check fails, we
                    # need to update this test with different elements. Skip
                    # this part if we are configured to use any other hash
                    # algorithm (for example, using Py_HASH_EXTERNAL):
                    if sys.hash_info.algorithm in {"fnv", "siphash24"}:
                        args = ["-c", f"print({s})"]
                        _, repr_0, _ = assert_python_ok(*args, PYTHONHASHSEED="0")
                        _, repr_1, _ = assert_python_ok(*args, PYTHONHASHSEED="1")
                        self.assertNotEqual(repr_0, repr_1)
                    # Then, perform the actual test:
                    args = ["-c", f"import marshal; print(marshal.dumps({s}))"]
                    _, dump_0, _ = assert_python_ok(*args, PYTHONHASHSEED="0")
                    _, dump_1, _ = assert_python_ok(*args, PYTHONHASHSEED="1")
                    self.assertEqual(dump_0, dump_1)

    def test_unmarshallable(self):
        # Check no crash after encountering unmarshallable objects.
        # See https://github.com/python/cpython/issues/106287.
        fset = frozenset([int])
        code = compile("a = 1", "<string>", "exec")
        code = code.replace(co_consts=(1, fset, None))
        cases = (('tuple', (fset,)),
                 ('list', [fset]),
                 ('set', fset),
                 ('dict key', {fset: 'x'}),
                 ('dict value', {'x': fset}),
                 ('dict key & value', {fset: fset}),
                 ('slice', slice(fset, fset)),
                 ('code', code))
        for name, arg in cases:
            with self.subTest(name, arg=arg):
                with self.assertRaisesRegex(ValueError, "unmarshallable object"):
                    marshal.dumps((arg, memoryview(b'')))

    def test_unorderable_set_items(self):
        # Set items are sorted by their marshalled representation, they
        # are not compared, even if NaNs which are distinct objects have
        # the same marshalled representation.
        nan = float('nan')
        for kind in set, frozenset:
            for version in range(marshal.version + 1):
                with self.subTest(kind=kind, version=version):
                    sample = kind([complex(nan, 0), complex(nan, 0), 1j,
                                   (complex(nan, 0),), (complex(nan, 0),),
                                   float('nan'), float('nan')])
                    self.assertEqual(len(sample), 7)
                    new = marshal.loads(marshal.dumps(sample, version))
                    self.assertIs(type(new), kind)
                    self.assertEqual(sorted(map(repr, new)),
                                     sorted(map(repr, sample)))

    def test_unorderable_set_items_in_code(self):
        # A constant set containing NaNs which are distinct objects.
        # Run in a subprocess: constants of code objects are immortal
        # in the free-threaded build.
        script = textwrap.dedent("""
            import marshal
            source = 'x in {(1e999-1e999)*1j, (1e999-1e999)*2j, 1.5}'
            code = compile(source, '<marshal test>', 'eval')
            def consts(code):
                return [sorted(map(repr, x)) if isinstance(x, frozenset) else x
                        for x in code.co_consts]
            assert ['(nan+nanj)', '(nan+nanj)', '1.5'] in consts(code)
            for version in range(marshal.version + 1):
                new = marshal.loads(marshal.dumps(code, version))
                assert new.co_code == code.co_code
                assert consts(new) == consts(code)
                assert eval(new, {'x': 1.5}) is True
                assert eval(new, {'x': complex('nan')}) is False
            """)
        assert_python_ok('-c', script)

    def test_set_changed_size(self):
        # A set is modified while its items are being marshalled.
        class Item:
            def __hash__(self):
                return 0
            def __buffer__(self, flags):
                change(sample)
                return memoryview(b'abc')
        def grow(sample):
            sample.update(range(1000, 1400))
        def shrink(sample):
            sample.difference_update(range(1, 60))
        for change in grow, shrink:
            for version in range(marshal.version + 1):
                with self.subTest(change=change.__name__, version=version):
                    # The first item of the set modifies it.
                    sample = {Item()}
                    sample.update(range(1, 60))
                    self.assertIsInstance(next(iter(sample)), Item)
                    with self.assertRaisesRegex(RuntimeError,
                                                "set changed size"):
                        marshal.dumps(sample, version)


LARGE_SIZE = 2**31
pointer_size = 8 if sys.maxsize > 0xFFFFFFFF else 4

class NullWriter:
    def write(self, s):
        pass

@unittest.skipIf(LARGE_SIZE > sys.maxsize, "test cannot run on 32-bit systems")
class LargeValuesTestCase(unittest.TestCase):
    def check_unmarshallable(self, data):
        self.assertRaises(ValueError, marshal.dump, data, NullWriter())

    @support.bigmemtest(size=LARGE_SIZE, memuse=2, dry_run=False)
    def test_bytes(self, size):
        self.check_unmarshallable(b'x' * size)

    @support.bigmemtest(size=LARGE_SIZE, memuse=2, dry_run=False)
    def test_str(self, size):
        self.check_unmarshallable('x' * size)

    @support.bigmemtest(size=LARGE_SIZE, memuse=pointer_size + 1, dry_run=False)
    def test_tuple(self, size):
        self.check_unmarshallable((None,) * size)

    @support.bigmemtest(size=LARGE_SIZE, memuse=pointer_size + 1, dry_run=False)
    def test_list(self, size):
        self.check_unmarshallable([None] * size)

    @support.bigmemtest(size=LARGE_SIZE,
            memuse=pointer_size*12 + sys.getsizeof(LARGE_SIZE-1),
            dry_run=False)
    def test_set(self, size):
        self.check_unmarshallable(set(range(size)))

    @support.bigmemtest(size=LARGE_SIZE,
            memuse=pointer_size*12 + sys.getsizeof(LARGE_SIZE-1),
            dry_run=False)
    def test_frozenset(self, size):
        self.check_unmarshallable(frozenset(range(size)))

    @support.bigmemtest(size=LARGE_SIZE, memuse=2, dry_run=False)
    def test_bytearray(self, size):
        self.check_unmarshallable(bytearray(size))

def CollectObjectIDs(ids, obj):
    """Collect object ids seen in a structure"""
    if id(obj) in ids:
        return
    ids.add(id(obj))
    if isinstance(obj, (list, tuple, set, frozenset)):
        for e in obj:
            CollectObjectIDs(ids, e)
    elif isinstance(obj, dict):
        for k, v in obj.items():
            CollectObjectIDs(ids, k)
            CollectObjectIDs(ids, v)
    return len(ids)

class InstancingTestCase(unittest.TestCase, HelperMixin):
    keys = (123, 1.2345, 'abc', (123, 'abc'), frozenset({123, 'abc'}))

    def helper3(self, rsample, recursive=False, simple=False):
        #we have two instances
        sample = (rsample, rsample)

        n0 = CollectObjectIDs(set(), sample)

        for v in range(3, marshal.version + 1):
            s3 = marshal.dumps(sample, v)
            n3 = CollectObjectIDs(set(), marshal.loads(s3))

            #same number of instances generated
            self.assertEqual(n3, n0)

        if not recursive:
            #can compare with version 2
            s2 = marshal.dumps(sample, 2)
            n2 = CollectObjectIDs(set(), marshal.loads(s2))
            #old format generated more instances
            self.assertGreater(n2, n0)

            #if complex objects are in there, old format is larger
            if not simple:
                self.assertGreater(len(s2), len(s3))
            else:
                self.assertGreaterEqual(len(s2), len(s3))

    def testInt(self):
        intobj = 123321
        self.helper(intobj)
        self.helper3(intobj, simple=True)

    def testFloat(self):
        floatobj = 1.2345
        self.helper(floatobj)
        self.helper3(floatobj)

    def testStr(self):
        strobj = "abcde"*3
        self.helper(strobj)
        self.helper3(strobj)

    def testBytes(self):
        bytesobj = b"abcde"*3
        self.helper(bytesobj)
        self.helper3(bytesobj)

    def testList(self):
        for obj in self.keys:
            listobj = [obj, obj]
            self.helper(listobj)
            self.helper3(listobj)

    def testTuple(self):
        for obj in self.keys:
            tupleobj = (obj, obj)
            self.helper(tupleobj)
            self.helper3(tupleobj)

    def testSet(self):
        for obj in self.keys:
            setobj = {(obj, 1), (obj, 2)}
            self.helper(setobj)
            self.helper3(setobj)

    def testFrozenSet(self):
        for obj in self.keys:
            frozensetobj = frozenset({(obj, 1), (obj, 2)})
            self.helper(frozensetobj)
            self.helper3(frozensetobj)

    def testDict(self):
        for obj in self.keys:
            dictobj = {"hello": obj, "goodbye": obj, obj: "hello"}
            self.helper(dictobj)
            self.helper3(dictobj)

    def testFrozenDict(self):
        for obj in self.keys:
            dictobj = frozendict({"hello": obj, "goodbye": obj, obj: "hello"})
            self.helper(dictobj)

            for version in range(6):
                with self.assertRaises(ValueError):
                    marshal.dumps(dictobj, version)

    def testModule(self):
        with open(__file__, "rb") as f:
            code = f.read()
        if __file__.endswith(".py"):
            code = compile(code, __file__, "exec")
        self.helper(code)
        self.helper3(code)

    def testRecursion(self):
        obj = 1.2345
        d = {"hello": obj, "goodbye": obj, obj: "hello"}
        d["self"] = d
        self.helper3(d, recursive=True)
        l = [obj, obj]
        l.append(l)
        self.helper3(l, recursive=True)

class CompatibilityTestCase(unittest.TestCase):
    def _test(self, version):
        with open(__file__, "rb") as f:
            code = f.read()
        if __file__.endswith(".py"):
            code = compile(code, __file__, "exec")
        data = marshal.dumps(code, version)
        marshal.loads(data)

    def test0To3(self):
        self._test(0)

    def test1To3(self):
        self._test(1)

    def test2To3(self):
        self._test(2)

    def test3To3(self):
        self._test(3)

class InterningTestCase(unittest.TestCase, HelperMixin):
    strobj = "this is an interned string"
    strobj = sys.intern(strobj)

    def testIntern(self):
        s = marshal.loads(marshal.dumps(self.strobj))
        self.assertEqual(s, self.strobj)
        self.assertEqual(id(s), id(self.strobj))
        s2 = sys.intern(s)
        self.assertEqual(id(s2), id(s))

    def testNoIntern(self):
        s = marshal.loads(marshal.dumps(self.strobj, 2))
        self.assertEqual(s, self.strobj)
        self.assertNotEqual(id(s), id(self.strobj))
        s2 = sys.intern(s)
        self.assertNotEqual(id(s2), id(s))

class SliceTestCase(unittest.TestCase, HelperMixin):
    def test_slice(self):
        for obj in (
            slice(None), slice(1), slice(1, 2), slice(1, 2, 3),
            slice({'set'}, ('tuple', {'with': 'dict'}, ), self.helper.__code__)
        ):
            with self.subTest(obj=str(obj)):
                self.helper(obj)

                for version in range(5):
                    with self.assertRaises(ValueError):
                        marshal.dumps(obj, version)

@support.cpython_only
@unittest.skipUnless(_testcapi, 'requires _testcapi')
class CAPI_TestCase(unittest.TestCase, HelperMixin):

    def test_write_long_to_file(self):
        for v in range(marshal.version + 1):
            _testcapi.pymarshal_write_long_to_file(0x12345678, os_helper.TESTFN, v)
            with open(os_helper.TESTFN, 'rb') as f:
                data = f.read()
            os_helper.unlink(os_helper.TESTFN)
            self.assertEqual(data, b'\x78\x56\x34\x12')

    def test_write_object_to_file(self):
        obj = ('\u20ac', b'abc', 123, 45.6, 7+8j, 'long line '*1000)
        for v in range(marshal.version + 1):
            _testcapi.pymarshal_write_object_to_file(obj, os_helper.TESTFN, v)
            with open(os_helper.TESTFN, 'rb') as f:
                data = f.read()
            os_helper.unlink(os_helper.TESTFN)
            self.assertEqual(marshal.loads(data), obj)
            self.assertEqual(data, marshal.dumps(obj, v))

            # Larger than a stdio buffer, with references to objects
            # written long before.
            shared = (float('1.5'), b'bytes' * 10)
            big = [shared, 'long line ' * 10_000, shared, [shared] * 10_000]
            _testcapi.pymarshal_write_object_to_file(big, os_helper.TESTFN, v)
            with open(os_helper.TESTFN, 'rb') as f:
                data = f.read()
            os_helper.unlink(os_helper.TESTFN)
            self.assertEqual(data, marshal.dumps(big, v))
            obj2 = marshal.loads(data)
            self.assertEqual(obj2, big)
            if v >= 3:
                self.assertIs(obj2[2], obj2[0])
                self.assertIs(obj2[3][-1], obj2[0])

            # Nothing is written if the object cannot be marshalled,
            # even if the error is only detected after more data than
            # fits in a stdio buffer.
            for bad in (object(),
                        [1, 'abc', object()],
                        [shared, 'long line ' * 10_000, shared, object()]):
                with self.assertRaisesRegex(ValueError,
                                            'unmarshallable object'):
                    _testcapi.pymarshal_write_object_to_file(
                        bad, os_helper.TESTFN, v)
                with open(os_helper.TESTFN, 'rb') as f:
                    data = f.read()
                os_helper.unlink(os_helper.TESTFN)
                self.assertEqual(data, b'')

    def test_read_short_from_file(self):
        with open(os_helper.TESTFN, 'wb') as f:
            f.write(b'\x34\x12xxxx')
        r, p = _testcapi.pymarshal_read_short_from_file(os_helper.TESTFN)
        os_helper.unlink(os_helper.TESTFN)
        self.assertEqual(r, 0x1234)
        self.assertEqual(p, 2)

        with open(os_helper.TESTFN, 'wb') as f:
            f.write(b'\x12')
        with self.assertRaises(EOFError):
            _testcapi.pymarshal_read_short_from_file(os_helper.TESTFN)
        os_helper.unlink(os_helper.TESTFN)

    def test_read_long_from_file(self):
        with open(os_helper.TESTFN, 'wb') as f:
            f.write(b'\x78\x56\x34\x12xxxx')
        r, p = _testcapi.pymarshal_read_long_from_file(os_helper.TESTFN)
        os_helper.unlink(os_helper.TESTFN)
        self.assertEqual(r, 0x12345678)
        self.assertEqual(p, 4)

        with open(os_helper.TESTFN, 'wb') as f:
            f.write(b'\x56\x34\x12')
        with self.assertRaises(EOFError):
            _testcapi.pymarshal_read_long_from_file(os_helper.TESTFN)
        os_helper.unlink(os_helper.TESTFN)

    def test_read_last_object_from_file(self):
        obj = ('\u20ac', b'abc', 123, 45.6, 7+8j)
        for v in range(marshal.version + 1):
            data = marshal.dumps(obj, v)
            with open(os_helper.TESTFN, 'wb') as f:
                f.write(data + b'xxxx')
            r, p = _testcapi.pymarshal_read_last_object_from_file(os_helper.TESTFN)
            os_helper.unlink(os_helper.TESTFN)
            self.assertEqual(r, obj)

            with open(os_helper.TESTFN, 'wb') as f:
                f.write(omit_last_byte(data))
            with self.assertRaises(EOFError):
                _testcapi.pymarshal_read_last_object_from_file(os_helper.TESTFN)
            os_helper.unlink(os_helper.TESTFN)

    def test_read_object_from_file(self):
        obj = ('\u20ac', b'abc', 123, 45.6, 7+8j)
        for v in range(marshal.version + 1):
            data = marshal.dumps(obj, v)
            with open(os_helper.TESTFN, 'wb') as f:
                f.write(data + b'xxxx')
            r, p = _testcapi.pymarshal_read_object_from_file(os_helper.TESTFN)
            os_helper.unlink(os_helper.TESTFN)
            self.assertEqual(r, obj)
            self.assertEqual(p, len(data))

            with open(os_helper.TESTFN, 'wb') as f:
                f.write(omit_last_byte(data))
            with self.assertRaises(EOFError):
                _testcapi.pymarshal_read_object_from_file(os_helper.TESTFN)
            os_helper.unlink(os_helper.TESTFN)


def reachable_objects(obj):
    """Return a list of all objects marshalled as a part of obj."""
    result = []
    seen = set()
    def visit(x):
        if id(x) in seen:
            return
        seen.add(id(x))
        result.append(x)
        if isinstance(x, (tuple, list, set, frozenset)):
            for item in x:
                visit(item)
        elif isinstance(x, (dict, frozendict)):
            for key, value in x.items():
                visit(key)
                visit(value)
        elif isinstance(x, slice):
            visit(x.start)
            visit(x.stop)
            visit(x.step)
        elif isinstance(x, types.CodeType):
            for name in ('co_code', 'co_consts', 'co_names', 'co_varnames',
                         'co_freevars', 'co_cellvars', 'co_filename',
                         'co_name', 'co_qualname', 'co_linetable',
                         'co_exceptiontable'):
                visit(getattr(x, name))
    visit(obj)
    return result


class ReferenceTestCase(unittest.TestCase):
    # The output must only depend on the marshalled objects, not on
    # the number of references to them in the rest of the process:
    # an object is written with FLAG_REF if and only if it occurs more
    # than once in the marshalled object graph.

    FLAG_REF = 0x80
    # The tests comparing the output with literal data use this version.
    VERSION = 6

    @staticmethod
    def make_sample(version=marshal.version):
        # Create new objects on every call, so that nothing else
        # refers to them.
        bigint = int('1234567890' * 4)
        flt = float('1.25')
        cplx = complex('1.5+2.5j')
        data = bytes(range(16))
        text = ' '.join(['spam', 'eggs'])
        nonascii = '\u20ac' + text
        inner = (bigint, flt, cplx, data, text, nonascii)
        sample = [
            inner,
            (flt, (data, (bigint,))),
            [text, [cplx]],
            {flt: inner, text: [data]},
            {(bigint, 1), (data, 2)},
            frozenset([(text, 1), (flt, 2), nonascii]),
            (inner, bigint, inner),
        ]
        if version >= 5:
            sample.append(slice(bigint, (flt, text), data))
        if version >= 6:
            sample.append(frozendict({text: inner, 'key': (cplx,)}))
        return sample

    def check_extra_references(self, obj, version):
        expected = marshal.dumps(obj, version)
        extra = reachable_objects(obj)
        self.assertEqual(marshal.dumps(obj, version), expected)
        extra2 = [list(extra), tuple(extra)]
        self.assertEqual(marshal.dumps(obj, version), expected)
        del extra, extra2
        self.assertEqual(marshal.dumps(obj, version), expected)
        return expected

    def test_extra_references(self):
        for version in range(marshal.version + 1):
            with self.subTest(version=version):
                sample = self.make_sample(version)
                data = self.check_extra_references(sample, version)
                # Equal graphs give equal output.
                self.assertEqual(marshal.dumps(self.make_sample(version),
                                               version), data)
                self.assertEqual(marshal.loads(data), sample)

    def test_extra_references_code(self):
        source = textwrap.dedent("""
            def f(a, b=(1, 2.5, 'spam eggs')):
                x = {'ham', 12345678901234567890, b'bytes'}
                def g(c):
                    return (a, b, c, x, (1, 2.5, 'spam eggs'), 6.25j)
                return g
            class C:
                attr = (None, ..., 2**70)
                def method(self):
                    return self.attr, 'spam eggs'
            """)
        for version in range(marshal.version + 1):
            with self.subTest(version=version):
                code = compile(source, '<marshal test>', 'exec')
                data = self.check_extra_references(code, version)
                self.assertEqual(marshal.loads(data), code)

    @support.cpython_only
    def test_single_occurrence_is_not_flagged(self):
        # No FLAG_REF for an object which occurs only once, even if it is
        # immortal, interned or referenced from elsewhere.
        version = self.VERSION
        interned = sys.intern('marshal_interned_name')
        referenced = self.make_sample(version)
        keep = reachable_objects(referenced)
        for obj, typecode in [
            (1, 'i'),
            (2**100, 'l'),
            (1.5, 'g'),
            (1.5j, 'y'),
            (b'', 's'),
            (b'bytes', 's'),
            ('', 'Z'),
            ('a', 'Z'),
            (interned, 'Z'),
            (''.join(['\u20ac', 'uro']), 'u'),
            (' '.join(['not', 'interned']), 'z'),
            (sys.intern('\u20ac\u20ac'), 't'),
            ((), ')'),
            (frozenset(), '>'),
            (referenced[0], ')'),
            (referenced[2], '['),
            (referenced[3], '{'),
            (referenced[5], '>'),
            (referenced[7], ':'),
            (referenced[8], '}'),
            (self.make_sample.__code__, 'c'),
        ]:
            with self.subTest(obj=obj):
                data = marshal.dumps(obj, version)
                self.assertEqual(data[0], ord(typecode))
        data = marshal.dumps(referenced, version)
        self.assertEqual(marshal.dumps(self.make_sample(version), version),
                         data)
        del keep
        self.assertEqual(marshal.dumps(referenced, version), data)

        a = float('1.5')
        b = float('2.5')
        keep = [a, b, a, b]
        data_a = b'g' + struct.pack('<d', a)
        data_b = b'g' + struct.pack('<d', b)
        self.assertEqual(marshal.dumps([a, b], version),
                         b'[\x02\0\0\0' + data_a + data_b)
        self.assertEqual(marshal.dumps((1, 'a', interned, None), version),
                         b')\x04i\x01\0\0\0Z\x01a'
                         b'Z\x15marshal_interned_nameN')
        # Also if other objects are referenced.
        self.assertEqual(marshal.dumps([a, 1, 'a', interned, b, a], version),
                         b'[\x06\0\0\0\xe7' + struct.pack('<d', a) +
                         b'i\x01\0\0\0Z\x01a'
                         b'Z\x15marshal_interned_name' +
                         data_b + b'r\0\0\0\0')

    @support.cpython_only
    def test_interned_strings(self):
        # An interned string is written like any other object: with
        # FLAG_REF if and only if it occurs more than once.
        version = self.VERSION
        once = sys.intern('marshal_interned_once')
        twice = sys.intern('marshal_interned_twice')
        nonascii = sys.intern('marshal_interned_\u20ac')
        data_once = b'Z\x15marshal_interned_once'
        data_twice = b'\x16marshal_interned_twice'
        data_nonascii = b'\x14\0\0\0' + nonascii.encode()
        ref0 = b'r\0\0\0\0'
        self.assertEqual(marshal.dumps((twice, once, twice), version),
                         b')\x03\xda' + data_twice + data_once + ref0)
        self.assertEqual(marshal.dumps((once, twice, twice), version),
                         b')\x03' + data_once + b'\xda' + data_twice + ref0)
        self.assertEqual(marshal.dumps((twice, nonascii, twice), version),
                         b')\x03\xda' + data_twice +
                         b't' + data_nonascii + ref0)
        self.assertEqual(marshal.dumps((nonascii, twice, nonascii), version),
                         b')\x03\xf4' + data_nonascii +
                         b'Z' + data_twice + ref0)

        # A name which occurs once in a code object.
        code = compile('marshal_name_once', '<marshal test>', 'eval')
        self.assertEqual(code.co_names, ('marshal_name_once',))
        self.assertIs(code.co_names[0], sys.intern('marshal_name_once'))
        data = marshal.dumps(code, version)
        self.assertEqual(data.count(b'\x11marshal_name_once'), 1)
        self.assertIn(b'Z\x11marshal_name_once', data)
        new = marshal.loads(data)
        self.assertEqual(new, code)
        self.assertIs(new.co_names[0], code.co_names[0])

    @support.cpython_only
    def test_reference_index(self):
        # Only the objects occurring more than once are numbered, in
        # the order of their first occurrence.
        a = float('1.5')
        b = float('2.5')
        c = float('3.5')
        data_a = b'g' + struct.pack('<d', a)
        data_b = b'g' + struct.pack('<d', b)
        data_c = b'g' + struct.pack('<d', c)
        def flagged(data):
            data = bytearray(data)
            data[0] |= self.FLAG_REF
            return bytes(data)
        def ref(index):
            return b'r' + struct.pack('<i', index)
        def dumps(obj):
            return marshal.dumps(obj, self.VERSION)

        self.assertEqual(dumps([a, a]),
                         b'[\x02\0\0\0' + flagged(data_a) + ref(0))
        self.assertEqual(dumps([a, b, b]),
                         b'[\x03\0\0\0' + data_a + flagged(data_b) + ref(0))
        self.assertEqual(dumps([a, b, c, c, a, c]),
                         b'[\x06\0\0\0' + flagged(data_a) + data_b +
                         flagged(data_c) + ref(1) + ref(0) + ref(1))
        # A container is numbered before its items.
        t = (a, b, a)
        self.assertEqual(dumps([c, t, t, c]),
                         b'[\x04\0\0\0' + flagged(data_c) +
                         b'\xa9\x03' + flagged(data_a) + data_b + ref(2) +
                         ref(1) + ref(0))
        # Small ints and interned strings are not special.
        self.assertEqual(dumps((1, 2, 1, 'a', 'a')),
                         b')\x05\xe9\x01\0\0\0i\x02\0\0\0r\0\0\0\0'
                         b'\xda\x01ar\x01\0\0\0')
        # A recursive list refers to itself.
        lst = [a]
        lst.append(lst)
        self.assertEqual(dumps(lst),
                         b'\xdb\x02\0\0\0' + data_a + ref(0))

    @support.cpython_only
    def test_many_references(self):
        # References at every offset in the output, next to each other, far
        # from the objects they refer to and from each other.
        a = float('1.5')
        b = float('2.5')
        c = float('3.5')
        keep = [a, b, c]
        data_a = b'g' + struct.pack('<d', a)
        data_b = b'g' + struct.pack('<d', b)
        data_c = b'g' + struct.pack('<d', c)
        def flagged(data):
            data = bytearray(data)
            data[0] |= self.FLAG_REF
            return bytes(data)
        def ref(index):
            return b'r' + struct.pack('<i', index)
        def dumped(data):
            return b's' + struct.pack('<i', len(data)) + data

        # b occurs only once: it is not numbered.
        self.assertEqual(marshal.dumps([b, a] + [a] * 100_000, 4),
                         b'[' + struct.pack('<i', 100_002) + data_b +
                         flagged(data_a) + ref(0) * 100_000)
        self.assertEqual(marshal.dumps([a, b, c] + [c, a] * 50_000, 4),
                         b'[' + struct.pack('<i', 100_003) +
                         flagged(data_a) + data_b + flagged(data_c) +
                         (ref(1) + ref(0)) * 50_000)
        for count in (1, 2, 3, 254, 255, 256, 257, 510, 511, 512):
            with self.subTest(count=count):
                sample = [a, c] + [a] * count + [b] + [c] * count + [a]
                self.assertEqual(marshal.dumps(sample, 4),
                                 b'[' + struct.pack('<i', len(sample)) +
                                 flagged(data_a) + flagged(data_c) +
                                 ref(0) * count + data_b + ref(1) * count +
                                 ref(0))
        for size in (*range(70), *range(235, 260), 1000, 100_000, 1_000_000):
            with self.subTest(size=size):
                pad1 = b'1' + bytes(size)
                pad2 = b'2' + bytes(size)
                pad3 = b'3' + bytes(size * 3)
                sample = [b, pad1, a, c, a, pad2, a, c, pad3, c]
                self.assertEqual(marshal.dumps(sample, 4),
                                 b'[\x0a\0\0\0' + data_b + dumped(pad1) +
                                 flagged(data_a) + flagged(data_c) + ref(0) +
                                 dumped(pad2) + ref(0) + ref(1) +
                                 dumped(pad3) + ref(1))

    def test_shared_objects(self):
        # Objects occurring more than once are loaded as a single object.
        code = compile('x = 1', '<marshal test>', 'exec')
        objects = [
            int('1234567890' * 4), float('1.25'), complex('1.5+2.5j'),
            bytes(range(16)), ' '.join(['spam', 'eggs']),
            '\u20ac' * 3, (float('1.25'), 'x'), [1, 2], {1: 2}, {1, 2},
            frozenset([float('1.25'), 2]), code, code.co_code,
        ]
        for version in range(3, marshal.version + 1):
            if version >= 5:
                objects.append(slice(1, float('1.25')))
            if version >= 6:
                objects.append(frozendict(x=[1]))
            for obj in objects:
                with self.subTest(version=version, obj=obj):
                    for sample in ([obj, obj], (obj, [obj], obj),
                                   {'a': obj, 'b': (obj,)},
                                   [obj] * 300):
                        new = marshal.loads(marshal.dumps(sample, version))
                        self.assertEqual(new, sample)
                        new = reachable_objects(new)
                        self.assertEqual(
                            sum(type(x) is type(obj) and x == obj
                                for x in new), 1)
            # All together, in a different order.
            sample = [objects, list(reversed(objects)), tuple(objects)]
            new = marshal.loads(marshal.dumps(sample, version))
            self.assertEqual(new, sample)
            for x, y, z in zip(new[0], reversed(new[1]), new[2]):
                self.assertIs(x, y)
                self.assertIs(x, z)

    def test_shared_objects_old_versions(self):
        sample = self.make_sample(2)
        for version in range(3):
            with self.subTest(version=version):
                data = marshal.dumps([sample, sample], version)
                for obj in reachable_objects(sample):
                    self.assertFalse(
                        marshal.dumps(obj, version)[0] & self.FLAG_REF)
                new = marshal.loads(data)
                self.assertEqual(new, [sample, sample])
                self.assertIsNot(new[0], new[1])

    def test_nan_in_set(self):
        # Distinct NaNs are distinct elements of a set, and they have
        # the same marshalled representation.
        for kind in set, frozenset:
            for version in range(marshal.version + 1):
                with self.subTest(kind=kind, version=version):
                    nan1 = float('nan')
                    nan2 = float('nan')
                    nan3 = float('nan')
                    s = kind([nan1, 1.5, nan2, nan3])
                    for sample in (s, [s], [nan1, s], [s, nan2],
                                   [nan3, s, nan1], [nan1, nan2, nan3, s],
                                   [s, s], (nan2, [s, nan2], s)):
                        data = marshal.dumps(sample, version)
                        self.assertEqual(marshal.dumps(sample, version), data)
                        new = marshal.loads(data)
                        self.check_same_structure(new, sample, version >= 3)

    def check_same_structure(self, new, sample, shared):
        # Both object graphs have the same values and, if shared is true,
        # share the same objects.  Elements of sets are matched in any
        # order.
        def shape(obj):
            seen = {}
            result = []
            def number(x):
                if shared:
                    return seen.setdefault(id(x), len(seen))
            def known(x):
                if shared:
                    return seen.get(id(x), -1)
            def visit(x):
                if isinstance(x, float):
                    result.append((repr(x), number(x)))
                elif isinstance(x, (set, frozenset)):
                    # Elements are only numbered if they occur elsewhere.
                    result.append((type(x), number(x),
                                   sorted((repr(y), known(y)) for y in x)))
                else:
                    result.append((type(x), len(x)))
                    for y in x:
                        visit(y)
            visit(obj)
            # Again, with all objects numbered.
            result.clear()
            visit(obj)
            return result
        self.assertEqual(shape(new), shape(sample))

    def test_shared_recursion(self):
        # An object which is referenced several times is not mistaken
        # for a recursive one, and a recursive one is still detected.
        a = (float('1.25'),)
        b = (a, a)
        sample = [a, b, (b, a), b]
        keep = reachable_objects(sample)
        new = marshal.loads(marshal.dumps(sample))
        self.assertEqual(new, sample)
        self.assertIs(new[1][0], new[0])
        self.assertIs(new[2][0], new[1])

        lst = []
        rec = slice(a, b, lst)
        lst.append(rec)
        for sample in (rec, [a, b, rec], (rec, rec)):
            with self.assertRaises(ValueError):
                marshal.dumps(sample)
        lst.clear()

    def test_dump_to_file(self):
        # dump() writes the same data as dumps(), also if the data is larger
        # than a buffer and contains references to objects written before.
        sample = self.make_sample()
        sample = [sample, 'x' * 100_000, sample, [sample[0]] * 10_000]
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        for version in range(marshal.version + 1):
            with self.subTest(version=version):
                if version < 6:
                    sample[0] = sample[2] = self.make_sample(version)
                data = marshal.dumps(sample, version)
                with open(os_helper.TESTFN, 'wb') as f:
                    marshal.dump(sample, f, version)
                with open(os_helper.TESTFN, 'rb') as f:
                    self.assertEqual(f.read(), data)
                with open(os_helper.TESTFN, 'rb') as f:
                    self.assertEqual(marshal.load(f), sample)
                f = io.BytesIO()
                marshal.dump(sample, f, version)
                self.assertEqual(f.getvalue(), data)

    @threading_helper.requires_working_threading()
    def test_threads(self):
        # The output does not depend on other threads using the same objects.
        code = compile(textwrap.dedent("""
            def f(a, b=(1, 2.5, 'spam eggs')):
                return [a, b, {'ham', 12345678901234567890}, b'bytes']
            """), '<marshal test>', 'exec')
        sample = [self.make_sample(), code]
        expected = marshal.dumps(sample)
        results = []
        def work():
            for i in range(20):
                keep = reachable_objects(sample)
                results.append(marshal.dumps(sample))
        threads = [threading.Thread(target=work) for i in range(4)]
        with threading_helper.start_threads(threads):
            pass
        self.assertEqual(len(results), 80)
        self.assertEqual(set(results), {expected})


if __name__ == "__main__":
    unittest.main()
