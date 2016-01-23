from test import support
import array
import io
import marshal
import sys
import unittest
import os
import types

try:
    import _testcapi
except ImportError:
    _testcapi = None

class HelperMixin:
    def helper(self, sample, *extra):
        new = marshal.loads(marshal.dumps(sample, *extra))
        self.assertEqual(sample, new)
        try:
            with open(support.TESTFN, "wb") as f:
                marshal.dump(sample, f, *extra)
            with open(support.TESTFN, "rb") as f:
                new = marshal.load(f)
            self.assertEqual(sample, new)
        finally:
            support.unlink(support.TESTFN)

class IntTestCase(unittest.TestCase, HelperMixin):
    def test_ints(self):
        # Test a range of Python ints larger than the machine word size.
        n = sys.maxsize ** 2
        while n:
            for expected in (-n, n):
                self.helper(expected)
            n = n >> 1

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
        count = 5000    # more than MAX_MARSHAL_STACK_DEPTH
        codes = (ExceptionTestCase.test_exceptions.__code__,) * count
        marshal.loads(marshal.dumps(codes))

    def test_different_filenames(self):
        co1 = compile("x", "f1", "exec")
        co2 = compile("y", "f2", "exec")
        co1, co2 = marshal.loads(marshal.dumps((co1, co2)))
        self.assertEqual(co1.co_filename, "f1")
        self.assertEqual(co2.co_filename, "f2")

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

    @support.cpython_only
    def test_empty_frozenset_singleton(self):
        # marshal.loads() must reuse the empty frozenset singleton
        obj = frozenset()
        obj2 = marshal.loads(marshal.dumps(obj))
        self.assertIs(obj2, obj)


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
        self.assertRaises(Exception, marshal.loads, '0')
        self.assertRaises(Exception, marshal.loads, 'f')
        self.assertRaises(Exception, marshal.loads, marshal.dumps(2**65)[:-1])

    def test_version_argument(self):
        # Python 2.4.0 crashes for any call to marshal.dumps(x, y)
        self.assertEqual(marshal.loads(marshal.dumps(5, 0)), 5)
        self.assertEqual(marshal.loads(marshal.dumps(5, 1)), 5)

    def test_fuzz(self):
        # simple test that it's at least not *totally* trivial to
        # crash from bad marshal data
        for c in [chr(i) for i in range(256)]:
            try:
                marshal.loads(c)
            except Exception:
                pass

    def test_loads_2x_code(self):
        s = b'c' + (b'X' * 4*4) + b'{' * 2**20
        self.assertRaises(ValueError, marshal.loads, s)

    def test_loads_recursion(self):
        s = b'c' + (b'X' * 4*5) + b'{' * 2**20
        self.assertRaises(ValueError, marshal.loads, s)

    def test_recursion_limit(self):
        # Create a deeply nested structure.
        head = last = []
        # The max stack depth should match the value in Python/marshal.c.
        if os.name == 'nt' and hasattr(sys, 'gettotalrefcount'):
            MAX_MARSHAL_STACK_DEPTH = 1000
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
                with open(support.TESTFN, 'wb') as f:
                    for d in data:
                        marshal.dump(d, f)
                        if ilen:
                            f.write(interleaved)
                        positions.append(f.tell())
                with open(support.TESTFN, 'rb') as f:
                    for i, d in enumerate(data):
                        self.assertEqual(d, marshal.load(f))
                        if ilen:
                            f.read(ilen)
                        self.assertEqual(positions[i], f.tell())
            finally:
                support.unlink(support.TESTFN)

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

    def _test_eof(self):
        data = marshal.dumps(("hello", "dolly", None))
        for i in range(len(data)):
            self.assertRaises(EOFError, marshal.loads, data[0: i])

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
    intobj = 123321
    floatobj = 1.2345
    strobj = "abcde"*3
    dictobj = {"hello":floatobj, "goodbye":floatobj, floatobj:"hello"}

    def helper3(self, rsample, recursive=False, simple=False):
        #we have two instances
        sample = (rsample, rsample)

        n0 = CollectObjectIDs(set(), sample)

        s3 = marshal.dumps(sample, 3)
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
        self.helper(self.intobj)
        self.helper3(self.intobj, simple=True)

    def testFloat(self):
        self.helper(self.floatobj)
        self.helper3(self.floatobj)

    def testStr(self):
        self.helper(self.strobj)
        self.helper3(self.strobj)

    def testDict(self):
        self.helper(self.dictobj)
        self.helper3(self.dictobj)

    def testModule(self):
        with open(__file__, "rb") as f:
            code = f.read()
        if __file__.endswith(".py"):
            code = compile(code, __file__, "exec")
        self.helper(code)
        self.helper3(code)

    def testRecursion(self):
        d = dict(self.dictobj)
        d["self"] = d
        self.helper3(d, recursive=True)
        l = [self.dictobj]
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

@support.cpython_only
@unittest.skipUnless(_testcapi, 'requires _testcapi')
class CAPI_TestCase(unittest.TestCase, HelperMixin):

    def test_write_long_to_file(self):
        for v in range(marshal.version + 1):
            _testcapi.pymarshal_write_long_to_file(0x12345678, support.TESTFN, v)
            with open(support.TESTFN, 'rb') as f:
                data = f.read()
            support.unlink(support.TESTFN)
            self.assertEqual(data, b'\x78\x56\x34\x12')

    def test_write_object_to_file(self):
        obj = ('\u20ac', b'abc', 123, 45.6, 7+8j, 'long line '*1000)
        for v in range(marshal.version + 1):
            _testcapi.pymarshal_write_object_to_file(obj, support.TESTFN, v)
            with open(support.TESTFN, 'rb') as f:
                data = f.read()
            support.unlink(support.TESTFN)
            self.assertEqual(marshal.loads(data), obj)

    def test_read_short_from_file(self):
        with open(support.TESTFN, 'wb') as f:
            f.write(b'\x34\x12xxxx')
        r, p = _testcapi.pymarshal_read_short_from_file(support.TESTFN)
        support.unlink(support.TESTFN)
        self.assertEqual(r, 0x1234)
        self.assertEqual(p, 2)

        with open(support.TESTFN, 'wb') as f:
            f.write(b'\x12')
        with self.assertRaises(EOFError):
            _testcapi.pymarshal_read_short_from_file(support.TESTFN)
        support.unlink(support.TESTFN)

    def test_read_long_from_file(self):
        with open(support.TESTFN, 'wb') as f:
            f.write(b'\x78\x56\x34\x12xxxx')
        r, p = _testcapi.pymarshal_read_long_from_file(support.TESTFN)
        support.unlink(support.TESTFN)
        self.assertEqual(r, 0x12345678)
        self.assertEqual(p, 4)

        with open(support.TESTFN, 'wb') as f:
            f.write(b'\x56\x34\x12')
        with self.assertRaises(EOFError):
            _testcapi.pymarshal_read_long_from_file(support.TESTFN)
        support.unlink(support.TESTFN)

    def test_read_last_object_from_file(self):
        obj = ('\u20ac', b'abc', 123, 45.6, 7+8j)
        for v in range(marshal.version + 1):
            data = marshal.dumps(obj, v)
            with open(support.TESTFN, 'wb') as f:
                f.write(data + b'xxxx')
            r, p = _testcapi.pymarshal_read_last_object_from_file(support.TESTFN)
            support.unlink(support.TESTFN)
            self.assertEqual(r, obj)

            with open(support.TESTFN, 'wb') as f:
                f.write(data[:1])
            with self.assertRaises(EOFError):
                _testcapi.pymarshal_read_last_object_from_file(support.TESTFN)
            support.unlink(support.TESTFN)

    def test_read_object_from_file(self):
        obj = ('\u20ac', b'abc', 123, 45.6, 7+8j)
        for v in range(marshal.version + 1):
            data = marshal.dumps(obj, v)
            with open(support.TESTFN, 'wb') as f:
                f.write(data + b'xxxx')
            r, p = _testcapi.pymarshal_read_object_from_file(support.TESTFN)
            support.unlink(support.TESTFN)
            self.assertEqual(r, obj)
            self.assertEqual(p, len(data))

            with open(support.TESTFN, 'wb') as f:
                f.write(data[:1])
            with self.assertRaises(EOFError):
                _testcapi.pymarshal_read_object_from_file(support.TESTFN)
            support.unlink(support.TESTFN)


if __name__ == "__main__":
    unittest.main()
