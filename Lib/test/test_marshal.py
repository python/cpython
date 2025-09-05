from test import support
from test.support import is_apple_mobile, os_helper, requires_debug_ranges, is_emscripten
from test.support.script_helper import assert_python_ok
import array
import io
import marshal
import sys
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

                for version in range(4):
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


if __name__ == "__main__":
    unittest.main()
