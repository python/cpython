from test import support
from test.test_json import PyTest, CTest


class JSONTestObject:
    pass


class TestRecursion:
    def test_listrecursion(self):
        x = []
        x.append(x)
        try:
            self.dumps(x)
        except ValueError as exc:
            self.assertEqual(exc.__notes__, ["when serializing list item 0"])
        else:
            self.fail("didn't raise ValueError on list recursion")
        x = []
        y = [x]
        x.append(y)
        try:
            self.dumps(x)
        except ValueError as exc:
            self.assertEqual(exc.__notes__, ["when serializing list item 0"]*2)
        else:
            self.fail("didn't raise ValueError on alternating list recursion")
        y = []
        x = [y, y]
        # ensure that the marker is cleared
        self.dumps(x)

    def test_dictrecursion(self):
        x = {}
        x["test"] = x
        try:
            self.dumps(x)
        except ValueError as exc:
            self.assertEqual(exc.__notes__, ["when serializing dict item 'test'"])
        else:
            self.fail("didn't raise ValueError on dict recursion")
        x = {}
        y = {"a": x, "b": x}
        # ensure that the marker is cleared
        self.dumps(x)

    def test_defaultrecursion(self):
        class RecursiveJSONEncoder(self.json.JSONEncoder):
            recurse = False
            def default(self, o):
                if o is JSONTestObject:
                    if self.recurse:
                        return [JSONTestObject]
                    else:
                        return 'JSONTestObject'
                return self.json.JSONEncoder.default(o)

        enc = RecursiveJSONEncoder()
        self.assertEqual(enc.encode(JSONTestObject), '"JSONTestObject"')
        enc.recurse = True
        try:
            enc.encode(JSONTestObject)
        except ValueError as exc:
            self.assertEqual(exc.__notes__,
                             ["when serializing list item 0",
                              "when serializing type object"])
        else:
            self.fail("didn't raise ValueError on default recursion")


    @support.skip_if_pgo_task  # fails during PGO training w/ some stack sizes
    @support.run_with_limited_c_stack(500_000)
    @support.skip_emscripten_stack_overflow()
    @support.skip_wasi_stack_overflow()
    def test_highly_nested_objects_decoding(self):
        very_deep = 500_000
        # test that loading highly-nested objects doesn't segfault when C
        # accelerations are used. See #12017
        with self.assertRaises(RecursionError):
            with support.infinite_recursion():
                self.loads('{"a":' * very_deep + '1' + '}' * very_deep)
        with self.assertRaises(RecursionError):
            with support.infinite_recursion():
                self.loads('{"a":' * very_deep + '[1]' + '}' * very_deep)
        with self.assertRaises(RecursionError):
            with support.infinite_recursion():
                self.loads('[' * very_deep + '1' + ']' * very_deep)

    @support.skip_if_unlimited_stack_size
    @support.skip_wasi_stack_overflow()
    @support.skip_emscripten_stack_overflow()
    @support.requires_resource('cpu')
    def test_highly_nested_objects_encoding(self):
        # See issue gh-56260

        # With max_depth=2_000 on Linux, the Python implementation takes around
        # 2 seconds to raise RecursionError, whereas the C implementation only
        # takes around 160 ms.
        max_depth = 2_000
        l, d = [], {}
        for x in range(max_depth * 10):
            l, d = [l], {'k':d}

        # Don't use assertRaises(RecursionError): on FreeBSD 16 with a default
        # stack size of 512 MiB, the C implementation doesn't raise
        # RecursionError.
        try:
            with support.infinite_recursion(max_depth):
                self.dumps(l)
        except RecursionError:
            pass
        try:
            with support.infinite_recursion(max_depth):
                self.dumps(d)
        except RecursionError:
            pass

    @support.skip_if_huge_c_stack()
    @support.skip_emscripten_stack_overflow()
    @support.skip_wasi_stack_overflow()
    def test_endless_recursion(self):
        # See issue gh-56260
        class EndlessJSONEncoder(self.json.JSONEncoder):
            def default(self, o):
                """If check_circular is False, this will keep adding another list."""
                return [o]

        with self.assertRaises(RecursionError):
            with support.infinite_recursion(1000):
                EndlessJSONEncoder(check_circular=False).encode(5j)


class TestPyRecursion(TestRecursion, PyTest): pass
class TestCRecursion(TestRecursion, CTest): pass
