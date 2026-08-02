import collections
from test.test_json import PyTest, CTest


class ClassSpoof:
    def __init__(self, spoofed_class):
        self.spoofed_class = spoofed_class

    @property
    def __class__(self):
        return self.spoofed_class

    def __iter__(self):
        return iter(("spoofed",))

    def items(self):
        return (("spoofed", True),)


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

    def check_spoof_uses_default(
        self, value, fallback, expected, spoofed_obj
    ):
        calls = []

        def default(candidate):
            calls.append(candidate)
            return fallback

        encoded = self.dumps(value, default=default)
        self.assertEqual(encoded, self.dumps(expected))
        self.assertEqual(len(calls), 1)
        self.assertIs(calls[0], spoofed_obj)

    def test_spoofed_containers_use_default_at_top_level(self):
        for spoofed_class in (list, tuple, dict, frozendict):
            with self.subTest(spoofed_class=spoofed_class):
                obj = ClassSpoof(spoofed_class)
                fallback = {"defaulted": spoofed_class.__name__}
                self.check_spoof_uses_default(
                    obj, fallback, fallback, obj)

    def test_spoofed_containers_use_default_as_list_item(self):
        for spoofed_class in (list, tuple, dict, frozendict):
            with self.subTest(spoofed_class=spoofed_class):
                obj = ClassSpoof(spoofed_class)
                fallback = {"defaulted": spoofed_class.__name__}
                self.check_spoof_uses_default(
                    [obj], fallback, [fallback], obj)

    def test_spoofed_containers_use_default_as_dict_value(self):
        for spoofed_class in (list, tuple, dict, frozendict):
            with self.subTest(spoofed_class=spoofed_class):
                obj = ClassSpoof(spoofed_class)
                fallback = {"defaulted": spoofed_class.__name__}
                self.check_spoof_uses_default(
                    {"value": obj},
                    fallback, {"value": fallback}, obj)

    def test_spoofed_containers_without_default_raise_type_error(self):
        for spoofed_class in (list, tuple, dict, frozendict):
            with self.subTest(spoofed_class=spoofed_class):
                with self.assertRaises(TypeError):
                    self.dumps(ClassSpoof(spoofed_class))

    def test_genuine_container_subclasses_are_encoded(self):
        class ListSubclass(list):
            pass

        class TupleSubclass(tuple):
            pass

        class DictSubclass(dict):
            pass

        class FrozenDictSubclass(frozendict):
            pass

        values = (
            (ListSubclass((1, 2)), [1, 2]),
            (TupleSubclass((1, 2)), [1, 2]),
            (DictSubclass(a=1), {"a": 1}),
            (FrozenDictSubclass(a=1), {"a": 1}),
        )
        for value, expected in values:
            with self.subTest(value_type=type(value)):
                self.assertEqual(self.dumps(value), self.dumps(expected))


class TestPyDefault(TestDefault, PyTest): pass
class TestCDefault(TestDefault, CTest): pass
