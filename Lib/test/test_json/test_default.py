import collections
import copyreg
import decimal
import types
from test.test_json import PyTest, CTest

class IndexLike:
    def __init__(self, value):
        self.value = value
    def __index__(self):
        return self.value

class FloatLike:
    def __init__(self, value):
        self.value = value
    def __float__(self):
        return self.value

class MappingLike:
    def __init__(self, dict):
        self.dict = dict
    def __len__(self):
        return len(self.dict)
    def __iter__(self):
        return iter(self.dict)
    def __getitem__(self, key):
        return self.dict[key]
    def keys(self):
        yield from self.dict.keys()
    def items(self):
        yield from self.dict.items()

class MyRawJSON:
    def __init__(self, value):
        self.value = value
    def __raw_json__(self):
        return self.value


class TestDefault:
    def test_default(self):
        self.assertEqual(
            self.dumps(type, default=repr),
            self.dumps(repr(type)))

    def test_bad_default(self):
        def default(obj):
            if obj is NotImplemented:
                raise ValueError
            if obj is ...:
                return NotImplemented
            if obj is type:
                return collections
            return [...]

        with self.assertRaises(ValueError) as cm:
            self.dumps(type, default=default)
        self.assertEqual(cm.exception.__notes__,
                         ['when serializing ellipsis object',
                          'when serializing list item 0',
                          'when serializing module object',
                          'when serializing type object'])

    def test_ordereddict(self):
        od = collections.OrderedDict(a=1, b=2, c=3, d=4)
        od.move_to_end('b')
        self.assertEqual(
            self.dumps(od),
            '{"a": 1, "c": 3, "d": 4, "b": 2}')
        self.assertEqual(
            self.dumps(od, sort_keys=True),
            '{"a": 1, "b": 2, "c": 3, "d": 4}')

    def test_deque(self):
        self.assertEqual(
            self.dumps(collections.deque([1, 2, 3])),
            '[1, 2, 3]')
        self.assertEqual(
            self.dumps(collections.deque([1, 2, 3], maxlen=2)),
            '[2, 3]')

    def test_mappingproxy(self):
        self.assertEqual(
            self.dumps(types.MappingProxyType(collections.OrderedDict(a=1, b=2))),
            '{"a": 1, "b": 2}')

    def test_chainmap(self):
        cm = collections.ChainMap({'a': 1, 'b': 2}, {'a': 3, 'c': 4})
        self.assertEqual(
            self.dumps(cm),
            '{"a": 1, "c": 4, "b": 2}')
        self.assertEqual(
            self.dumps(cm, sort_keys=True),
            '{"a": 1, "b": 2, "c": 4}')
        cm = collections.ChainMap({'a': 1, 'b': 2}, {'a': 3, 'c': 4})
        self.assertEqual(
            self.dumps(cm),
            '{"a": 1, "c": 4, "b": 2}')
        self.assertEqual(
            self.dumps(cm, sort_keys=True),
            '{"a": 1, "b": 2, "c": 4}')
        self.assertEqual(
            self.dumps(collections.ChainMap({}, {'a': 2})),
            '{"a": 2}')

    def test_userdict(self):
        ud = collections.UserDict({'b': 1, 'a': 2})
        self.assertEqual(
            self.dumps(ud),
            '{"b": 1, "a": 2}')
        self.assertEqual(
            self.dumps(ud, sort_keys=True),
            '{"a": 2, "b": 1}')
        ud = collections.UserDict(MappingLike({'b': 1, 'a': 2}))
        self.assertEqual(
            self.dumps(ud),
            '{"b": 1, "a": 2}')
        self.assertEqual(
            self.dumps(ud, sort_keys=True),
            '{"a": 2, "b": 1}')

    def test_userlist(self):
        self.assertEqual(
            self.dumps(collections.UserList([1, 2, 3])),
            '[1, 2, 3]')

        self.assertEqual(
            self.dumps(collections.UserList(collections.deque([1, 2, 3]))),
            '[1, 2, 3]')

    def test_userstring(self):
        self.assertEqual(
            self.dumps(collections.UserString('abc')),
            '"abc"')

    def test_copyreg_json(self):
        class A: pass
        a = A()
        self.assertRaises(TypeError, self.dumps, a)
        copyreg.json(A, repr)
        try:
            self.assertEqual(self.dumps(a), self.dumps(repr(a)))
        finally:
            del copyreg.json_dispatch_table[A]
        self.assertRaises(TypeError, self.dumps, a)

    def test_encoder_dispatch_table(self):
        class A: pass
        class Encoder(self.json.JSONEncoder):
            dispatch_table = {A: repr}
        a = A()
        encoder = Encoder()
        self.assertEqual(encoder.encode(a), encoder.encode(repr(a)))

    def test_json_method(self):
        for result in (None, True, False, 42, 1.25, 'string', [1, 2], {'a': 'b'}):
            with self.subTest(obj=result):
                class A:
                    def __json__(self):
                        return result
                a = A()
                self.assertEqual(self.dumps(a), self.dumps(result))
        for result, expected in [
                (IndexLike(42), '42'),
                (FloatLike(1.25), '1.25'),
                (iter([]), '[]'),
                (iter([1, 2]), '[1, 2]'),
                (MappingLike({}), '{}'),
                (MappingLike({'a': 1, 'b': 2}), '{"a": 1, "b": 2}'),
                (MyRawJSON('123.00'), '123.00'),
            ]:
            with self.subTest(obj=result):
                class A:
                    def __json__(self):
                        return result
                a = A()
                self.assertEqual(self.dumps(a), expected)

    def test_bad_raw_json(self):
        class A:
            def __json__(self):
                return MyRawJSON(42)
        with self.assertRaisesRegex(TypeError,
                r'__raw_json__\(\) must return a string, not int'):
            self.dumps(A())

    def test_decimal(self):
        d = decimal.Decimal('0.12345678901234567890')
        self.assertRaises(TypeError, self.dumps, d)
        copyreg.json(decimal.Decimal, str)
        try:
            self.assertEqual(self.dumps(d), '"0.12345678901234567890"')

            copyreg.json(decimal.Decimal,
                         lambda obj: copyreg.RawJSON(str(obj)))
            self.assertEqual(self.dumps(d), '0.12345678901234567890')
        finally:
            del copyreg.json_dispatch_table[decimal.Decimal]

    def test_namedtuple(self):
        A = collections.namedtuple('A', ('name', 'age'))
        a = A('Alice', '25')
        self.assertEqual(self.dumps(a), '["Alice", "25"]')
        copyreg.json(A, lambda obj: obj._asdict())
        try:
            self.assertEqual(self.dumps(a), '{"name": "Alice", "age": "25"}')
            self.assertEqual(self.dumps(a, sort_keys=True), '{"age": "25", "name": "Alice"}')
        finally:
            del copyreg.json_dispatch_table[A]


class TestPyDefault(TestDefault, PyTest): pass
class TestCDefault(TestDefault, CTest): pass
