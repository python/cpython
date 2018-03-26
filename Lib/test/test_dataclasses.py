# Deliberately use "from dataclasses import *".  Every name in __all__
# is tested, so they all must be present.  This is a way to catch
# missing ones.

from dataclasses import *

import pickle
import inspect
import unittest
from unittest.mock import Mock
from typing import ClassVar, Any, List, Union, Tuple, Dict, Generic, TypeVar
from collections import deque, OrderedDict, namedtuple
from functools import total_ordering

# Just any custom exception we can catch.
class CustomError(Exception): pass

class TestCase(unittest.TestCase):
    def test_no_fields(self):
        @dataclass
        class C:
            pass

        o = C()
        self.assertEqual(len(fields(C)), 0)

    def test_no_fields_but_member_variable(self):
        @dataclass
        class C:
            i = 0

        o = C()
        self.assertEqual(len(fields(C)), 0)

    def test_one_field_no_default(self):
        @dataclass
        class C:
            x: int

        o = C(42)
        self.assertEqual(o.x, 42)

    def test_named_init_params(self):
        @dataclass
        class C:
            x: int

        o = C(x=32)
        self.assertEqual(o.x, 32)

    def test_two_fields_one_default(self):
        @dataclass
        class C:
            x: int
            y: int = 0

        o = C(3)
        self.assertEqual((o.x, o.y), (3, 0))

        # Non-defaults following defaults.
        with self.assertRaisesRegex(TypeError,
                                    "non-default argument 'y' follows "
                                    "default argument"):
            @dataclass
            class C:
                x: int = 0
                y: int

        # A derived class adds a non-default field after a default one.
        with self.assertRaisesRegex(TypeError,
                                    "non-default argument 'y' follows "
                                    "default argument"):
            @dataclass
            class B:
                x: int = 0

            @dataclass
            class C(B):
                y: int

        # Override a base class field and add a default to
        #  a field which didn't use to have a default.
        with self.assertRaisesRegex(TypeError,
                                    "non-default argument 'y' follows "
                                    "default argument"):
            @dataclass
            class B:
                x: int
                y: int

            @dataclass
            class C(B):
                x: int = 0

    def test_overwrite_hash(self):
        # Test that declaring this class isn't an error.  It should
        #  use the user-provided __hash__.
        @dataclass(frozen=True)
        class C:
            x: int
            def __hash__(self):
                return 301
        self.assertEqual(hash(C(100)), 301)

        # Test that declaring this class isn't an error.  It should
        #  use the generated __hash__.
        @dataclass(frozen=True)
        class C:
            x: int
            def __eq__(self, other):
                return False
        self.assertEqual(hash(C(100)), hash((100,)))

        # But this one should generate an exception, because with
        #  unsafe_hash=True, it's an error to have a __hash__ defined.
        with self.assertRaisesRegex(TypeError,
                                    'Cannot overwrite attribute __hash__'):
            @dataclass(unsafe_hash=True)
            class C:
                def __hash__(self):
                    pass

        # Creating this class should not generate an exception,
        #  because even though __hash__ exists before @dataclass is
        #  called, (due to __eq__ being defined), since it's None
        #  that's okay.
        @dataclass(unsafe_hash=True)
        class C:
            x: int
            def __eq__(self):
                pass
        # The generated hash function works as we'd expect.
        self.assertEqual(hash(C(10)), hash((10,)))

        # Creating this class should generate an exception, because
        #  __hash__ exists and is not None, which it would be if it
        #  had been auto-generated due to __eq__ being defined.
        with self.assertRaisesRegex(TypeError,
                                    'Cannot overwrite attribute __hash__'):
            @dataclass(unsafe_hash=True)
            class C:
                x: int
                def __eq__(self):
                    pass
                def __hash__(self):
                    pass

    def test_overwrite_fields_in_derived_class(self):
        # Note that x from C1 replaces x in Base, but the order remains
        #  the same as defined in Base.
        @dataclass
        class Base:
            x: Any = 15.0
            y: int = 0

        @dataclass
        class C1(Base):
            z: int = 10
            x: int = 15

        o = Base()
        self.assertEqual(repr(o), 'TestCase.test_overwrite_fields_in_derived_class.<locals>.Base(x=15.0, y=0)')

        o = C1()
        self.assertEqual(repr(o), 'TestCase.test_overwrite_fields_in_derived_class.<locals>.C1(x=15, y=0, z=10)')

        o = C1(x=5)
        self.assertEqual(repr(o), 'TestCase.test_overwrite_fields_in_derived_class.<locals>.C1(x=5, y=0, z=10)')

    def test_field_named_self(self):
        @dataclass
        class C:
            self: str
        c=C('foo')
        self.assertEqual(c.self, 'foo')

        # Make sure the first parameter is not named 'self'.
        sig = inspect.signature(C.__init__)
        first = next(iter(sig.parameters))
        self.assertNotEqual('self', first)

        # But we do use 'self' if no field named self.
        @dataclass
        class C:
            selfx: str

        # Make sure the first parameter is named 'self'.
        sig = inspect.signature(C.__init__)
        first = next(iter(sig.parameters))
        self.assertEqual('self', first)

    def test_0_field_compare(self):
        # Ensure that order=False is the default.
        @dataclass
        class C0:
            pass

        @dataclass(order=False)
        class C1:
            pass

        for cls in [C0, C1]:
            with self.subTest(cls=cls):
                self.assertEqual(cls(), cls())
                for idx, fn in enumerate([lambda a, b: a < b,
                                          lambda a, b: a <= b,
                                          lambda a, b: a > b,
                                          lambda a, b: a >= b]):
                    with self.subTest(idx=idx):
                        with self.assertRaisesRegex(TypeError,
                                                    f"not supported between instances of '{cls.__name__}' and '{cls.__name__}'"):
                            fn(cls(), cls())

        @dataclass(order=True)
        class C:
            pass
        self.assertLessEqual(C(), C())
        self.assertGreaterEqual(C(), C())

    def test_1_field_compare(self):
        # Ensure that order=False is the default.
        @dataclass
        class C0:
            x: int

        @dataclass(order=False)
        class C1:
            x: int

        for cls in [C0, C1]:
            with self.subTest(cls=cls):
                self.assertEqual(cls(1), cls(1))
                self.assertNotEqual(cls(0), cls(1))
                for idx, fn in enumerate([lambda a, b: a < b,
                                          lambda a, b: a <= b,
                                          lambda a, b: a > b,
                                          lambda a, b: a >= b]):
                    with self.subTest(idx=idx):
                        with self.assertRaisesRegex(TypeError,
                                                    f"not supported between instances of '{cls.__name__}' and '{cls.__name__}'"):
                            fn(cls(0), cls(0))

        @dataclass(order=True)
        class C:
            x: int
        self.assertLess(C(0), C(1))
        self.assertLessEqual(C(0), C(1))
        self.assertLessEqual(C(1), C(1))
        self.assertGreater(C(1), C(0))
        self.assertGreaterEqual(C(1), C(0))
        self.assertGreaterEqual(C(1), C(1))

    def test_simple_compare(self):
        # Ensure that order=False is the default.
        @dataclass
        class C0:
            x: int
            y: int

        @dataclass(order=False)
        class C1:
            x: int
            y: int

        for cls in [C0, C1]:
            with self.subTest(cls=cls):
                self.assertEqual(cls(0, 0), cls(0, 0))
                self.assertEqual(cls(1, 2), cls(1, 2))
                self.assertNotEqual(cls(1, 0), cls(0, 0))
                self.assertNotEqual(cls(1, 0), cls(1, 1))
                for idx, fn in enumerate([lambda a, b: a < b,
                                          lambda a, b: a <= b,
                                          lambda a, b: a > b,
                                          lambda a, b: a >= b]):
                    with self.subTest(idx=idx):
                        with self.assertRaisesRegex(TypeError,
                                                    f"not supported between instances of '{cls.__name__}' and '{cls.__name__}'"):
                            fn(cls(0, 0), cls(0, 0))

        @dataclass(order=True)
        class C:
            x: int
            y: int

        for idx, fn in enumerate([lambda a, b: a == b,
                                  lambda a, b: a <= b,
                                  lambda a, b: a >= b]):
            with self.subTest(idx=idx):
                self.assertTrue(fn(C(0, 0), C(0, 0)))

        for idx, fn in enumerate([lambda a, b: a < b,
                                  lambda a, b: a <= b,
                                  lambda a, b: a != b]):
            with self.subTest(idx=idx):
                self.assertTrue(fn(C(0, 0), C(0, 1)))
                self.assertTrue(fn(C(0, 1), C(1, 0)))
                self.assertTrue(fn(C(1, 0), C(1, 1)))

        for idx, fn in enumerate([lambda a, b: a > b,
                                  lambda a, b: a >= b,
                                  lambda a, b: a != b]):
            with self.subTest(idx=idx):
                self.assertTrue(fn(C(0, 1), C(0, 0)))
                self.assertTrue(fn(C(1, 0), C(0, 1)))
                self.assertTrue(fn(C(1, 1), C(1, 0)))

    def test_compare_subclasses(self):
        # Comparisons fail for subclasses, even if no fields
        #  are added.
        @dataclass
        class B:
            i: int

        @dataclass
        class C(B):
            pass

        for idx, (fn, expected) in enumerate([(lambda a, b: a == b, False),
                                              (lambda a, b: a != b, True)]):
            with self.subTest(idx=idx):
                self.assertEqual(fn(B(0), C(0)), expected)

        for idx, fn in enumerate([lambda a, b: a < b,
                                  lambda a, b: a <= b,
                                  lambda a, b: a > b,
                                  lambda a, b: a >= b]):
            with self.subTest(idx=idx):
                with self.assertRaisesRegex(TypeError,
                                            "not supported between instances of 'B' and 'C'"):
                    fn(B(0), C(0))

    def test_eq_order(self):
        # Test combining eq and order.
        for (eq,    order, result   ) in [
            (False, False, 'neither'),
            (False, True,  'exception'),
            (True,  False, 'eq_only'),
            (True,  True,  'both'),
        ]:
            with self.subTest(eq=eq, order=order):
                if result == 'exception':
                    with self.assertRaisesRegex(ValueError, 'eq must be true if order is true'):
                        @dataclass(eq=eq, order=order)
                        class C:
                            pass
                else:
                    @dataclass(eq=eq, order=order)
                    class C:
                        pass

                    if result == 'neither':
                        self.assertNotIn('__eq__', C.__dict__)
                        self.assertNotIn('__lt__', C.__dict__)
                        self.assertNotIn('__le__', C.__dict__)
                        self.assertNotIn('__gt__', C.__dict__)
                        self.assertNotIn('__ge__', C.__dict__)
                    elif result == 'both':
                        self.assertIn('__eq__', C.__dict__)
                        self.assertIn('__lt__', C.__dict__)
                        self.assertIn('__le__', C.__dict__)
                        self.assertIn('__gt__', C.__dict__)
                        self.assertIn('__ge__', C.__dict__)
                    elif result == 'eq_only':
                        self.assertIn('__eq__', C.__dict__)
                        self.assertNotIn('__lt__', C.__dict__)
                        self.assertNotIn('__le__', C.__dict__)
                        self.assertNotIn('__gt__', C.__dict__)
                        self.assertNotIn('__ge__', C.__dict__)
                    else:
                        assert False, f'unknown result {result!r}'

    def test_field_no_default(self):
        @dataclass
        class C:
            x: int = field()

        self.assertEqual(C(5).x, 5)

        with self.assertRaisesRegex(TypeError,
                                    r"__init__\(\) missing 1 required "
                                    "positional argument: 'x'"):
            C()

    def test_field_default(self):
        default = object()
        @dataclass
        class C:
            x: object = field(default=default)

        self.assertIs(C.x, default)
        c = C(10)
        self.assertEqual(c.x, 10)

        # If we delete the instance attribute, we should then see the
        #  class attribute.
        del c.x
        self.assertIs(c.x, default)

        self.assertIs(C().x, default)

    def test_not_in_repr(self):
        @dataclass
        class C:
            x: int = field(repr=False)
        with self.assertRaises(TypeError):
            C()
        c = C(10)
        self.assertEqual(repr(c), 'TestCase.test_not_in_repr.<locals>.C()')

        @dataclass
        class C:
            x: int = field(repr=False)
            y: int
        c = C(10, 20)
        self.assertEqual(repr(c), 'TestCase.test_not_in_repr.<locals>.C(y=20)')

    def test_not_in_compare(self):
        @dataclass
        class C:
            x: int = 0
            y: int = field(compare=False, default=4)

        self.assertEqual(C(), C(0, 20))
        self.assertEqual(C(1, 10), C(1, 20))
        self.assertNotEqual(C(3), C(4, 10))
        self.assertNotEqual(C(3, 10), C(4, 10))

    def test_hash_field_rules(self):
        # Test all 6 cases of:
        #  hash=True/False/None
        #  compare=True/False
        for (hash_,    compare, result  ) in [
            (True,     False,   'field' ),
            (True,     True,    'field' ),
            (False,    False,   'absent'),
            (False,    True,    'absent'),
            (None,     False,   'absent'),
            (None,     True,    'field' ),
            ]:
            with self.subTest(hash=hash_, compare=compare):
                @dataclass(unsafe_hash=True)
                class C:
                    x: int = field(compare=compare, hash=hash_, default=5)

                if result == 'field':
                    # __hash__ contains the field.
                    self.assertEqual(hash(C(5)), hash((5,)))
                elif result == 'absent':
                    # The field is not present in the hash.
                    self.assertEqual(hash(C(5)), hash(()))
                else:
                    assert False, f'unknown result {result!r}'

    def test_init_false_no_default(self):
        # If init=False and no default value, then the field won't be
        #  present in the instance.
        @dataclass
        class C:
            x: int = field(init=False)

        self.assertNotIn('x', C().__dict__)

        @dataclass
        class C:
            x: int
            y: int = 0
            z: int = field(init=False)
            t: int = 10

        self.assertNotIn('z', C(0).__dict__)
        self.assertEqual(vars(C(5)), {'t': 10, 'x': 5, 'y': 0})

    def test_class_marker(self):
        @dataclass
        class C:
            x: int
            y: str = field(init=False, default=None)
            z: str = field(repr=False)

        the_fields = fields(C)
        # the_fields is a tuple of 3 items, each value
        #  is in __annotations__.
        self.assertIsInstance(the_fields, tuple)
        for f in the_fields:
            self.assertIs(type(f), Field)
            self.assertIn(f.name, C.__annotations__)

        self.assertEqual(len(the_fields), 3)

        self.assertEqual(the_fields[0].name, 'x')
        self.assertEqual(the_fields[0].type, int)
        self.assertFalse(hasattr(C, 'x'))
        self.assertTrue (the_fields[0].init)
        self.assertTrue (the_fields[0].repr)
        self.assertEqual(the_fields[1].name, 'y')
        self.assertEqual(the_fields[1].type, str)
        self.assertIsNone(getattr(C, 'y'))
        self.assertFalse(the_fields[1].init)
        self.assertTrue (the_fields[1].repr)
        self.assertEqual(the_fields[2].name, 'z')
        self.assertEqual(the_fields[2].type, str)
        self.assertFalse(hasattr(C, 'z'))
        self.assertTrue (the_fields[2].init)
        self.assertFalse(the_fields[2].repr)

    def test_field_order(self):
        @dataclass
        class B:
            a: str = 'B:a'
            b: str = 'B:b'
            c: str = 'B:c'

        @dataclass
        class C(B):
            b: str = 'C:b'

        self.assertEqual([(f.name, f.default) for f in fields(C)],
                         [('a', 'B:a'),
                          ('b', 'C:b'),
                          ('c', 'B:c')])

        @dataclass
        class D(B):
            c: str = 'D:c'

        self.assertEqual([(f.name, f.default) for f in fields(D)],
                         [('a', 'B:a'),
                          ('b', 'B:b'),
                          ('c', 'D:c')])

        @dataclass
        class E(D):
            a: str = 'E:a'
            d: str = 'E:d'

        self.assertEqual([(f.name, f.default) for f in fields(E)],
                         [('a', 'E:a'),
                          ('b', 'B:b'),
                          ('c', 'D:c'),
                          ('d', 'E:d')])

    def test_class_attrs(self):
        # We only have a class attribute if a default value is
        #  specified, either directly or via a field with a default.
        default = object()
        @dataclass
        class C:
            x: int
            y: int = field(repr=False)
            z: object = default
            t: int = field(default=100)

        self.assertFalse(hasattr(C, 'x'))
        self.assertFalse(hasattr(C, 'y'))
        self.assertIs   (C.z, default)
        self.assertEqual(C.t, 100)

    def test_disallowed_mutable_defaults(self):
        # For the known types, don't allow mutable default values.
        for typ, empty, non_empty in [(list, [], [1]),
                                      (dict, {}, {0:1}),
                                      (set, set(), set([1])),
                                      ]:
            with self.subTest(typ=typ):
                # Can't use a zero-length value.
                with self.assertRaisesRegex(ValueError,
                                            f'mutable default {typ} for field '
                                            'x is not allowed'):
                    @dataclass
                    class Point:
                        x: typ = empty


                # Nor a non-zero-length value
                with self.assertRaisesRegex(ValueError,
                                            f'mutable default {typ} for field '
                                            'y is not allowed'):
                    @dataclass
                    class Point:
                        y: typ = non_empty

                # Check subtypes also fail.
                class Subclass(typ): pass

                with self.assertRaisesRegex(ValueError,
                                            f"mutable default .*Subclass'>"
                                            ' for field z is not allowed'
                                            ):
                    @dataclass
                    class Point:
                        z: typ = Subclass()

                # Because this is a ClassVar, it can be mutable.
                @dataclass
                class C:
                    z: ClassVar[typ] = typ()

                # Because this is a ClassVar, it can be mutable.
                @dataclass
                class C:
                    x: ClassVar[typ] = Subclass()


    def test_deliberately_mutable_defaults(self):
        # If a mutable default isn't in the known list of
        #  (list, dict, set), then it's okay.
        class Mutable:
            def __init__(self):
                self.l = []

        @dataclass
        class C:
            x: Mutable

        # These 2 instances will share this value of x.
        lst = Mutable()
        o1 = C(lst)
        o2 = C(lst)
        self.assertEqual(o1, o2)
        o1.x.l.extend([1, 2])
        self.assertEqual(o1, o2)
        self.assertEqual(o1.x.l, [1, 2])
        self.assertIs(o1.x, o2.x)

    def test_no_options(self):
        # Call with dataclass().
        @dataclass()
        class C:
            x: int

        self.assertEqual(C(42).x, 42)

    def test_not_tuple(self):
        # Make sure we can't be compared to a tuple.
        @dataclass
        class Point:
            x: int
            y: int
        self.assertNotEqual(Point(1, 2), (1, 2))

        # And that we can't compare to another unrelated dataclass.
        @dataclass
        class C:
            x: int
            y: int
        self.assertNotEqual(Point(1, 3), C(1, 3))

    def test_not_tuple(self):
        # Test that some of the problems with namedtuple don't happen
        #  here.
        @dataclass
        class Point3D:
            x: int
            y: int
            z: int

        @dataclass
        class Date:
            year: int
            month: int
            day: int

        self.assertNotEqual(Point3D(2017, 6, 3), Date(2017, 6, 3))
        self.assertNotEqual(Point3D(1, 2, 3), (1, 2, 3))

        # Make sure we can't unpack.
        with self.assertRaisesRegex(TypeError, 'unpack'):
            x, y, z = Point3D(4, 5, 6)

        # Make sure another class with the same field names isn't
        #  equal.
        @dataclass
        class Point3Dv1:
            x: int = 0
            y: int = 0
            z: int = 0
        self.assertNotEqual(Point3D(0, 0, 0), Point3Dv1())

    def test_function_annotations(self):
        # Some dummy class and instance to use as a default.
        class F:
            pass
        f = F()

        def validate_class(cls):
            # First, check __annotations__, even though they're not
            #  function annotations.
            self.assertEqual(cls.__annotations__['i'], int)
            self.assertEqual(cls.__annotations__['j'], str)
            self.assertEqual(cls.__annotations__['k'], F)
            self.assertEqual(cls.__annotations__['l'], float)
            self.assertEqual(cls.__annotations__['z'], complex)

            # Verify __init__.

            signature = inspect.signature(cls.__init__)
            # Check the return type, should be None.
            self.assertIs(signature.return_annotation, None)

            # Check each parameter.
            params = iter(signature.parameters.values())
            param = next(params)
            # This is testing an internal name, and probably shouldn't be tested.
            self.assertEqual(param.name, 'self')
            param = next(params)
            self.assertEqual(param.name, 'i')
            self.assertIs   (param.annotation, int)
            self.assertEqual(param.default, inspect.Parameter.empty)
            self.assertEqual(param.kind, inspect.Parameter.POSITIONAL_OR_KEYWORD)
            param = next(params)
            self.assertEqual(param.name, 'j')
            self.assertIs   (param.annotation, str)
            self.assertEqual(param.default, inspect.Parameter.empty)
            self.assertEqual(param.kind, inspect.Parameter.POSITIONAL_OR_KEYWORD)
            param = next(params)
            self.assertEqual(param.name, 'k')
            self.assertIs   (param.annotation, F)
            # Don't test for the default, since it's set to MISSING.
            self.assertEqual(param.kind, inspect.Parameter.POSITIONAL_OR_KEYWORD)
            param = next(params)
            self.assertEqual(param.name, 'l')
            self.assertIs   (param.annotation, float)
            # Don't test for the default, since it's set to MISSING.
            self.assertEqual(param.kind, inspect.Parameter.POSITIONAL_OR_KEYWORD)
            self.assertRaises(StopIteration, next, params)


        @dataclass
        class C:
            i: int
            j: str
            k: F = f
            l: float=field(default=None)
            z: complex=field(default=3+4j, init=False)

        validate_class(C)

        # Now repeat with __hash__.
        @dataclass(frozen=True, unsafe_hash=True)
        class C:
            i: int
            j: str
            k: F = f
            l: float=field(default=None)
            z: complex=field(default=3+4j, init=False)

        validate_class(C)

    def test_missing_default(self):
        # Test that MISSING works the same as a default not being
        #  specified.
        @dataclass
        class C:
            x: int=field(default=MISSING)
        with self.assertRaisesRegex(TypeError,
                                    r'__init__\(\) missing 1 required '
                                    'positional argument'):
            C()
        self.assertNotIn('x', C.__dict__)

        @dataclass
        class D:
            x: int
        with self.assertRaisesRegex(TypeError,
                                    r'__init__\(\) missing 1 required '
                                    'positional argument'):
            D()
        self.assertNotIn('x', D.__dict__)

    def test_missing_default_factory(self):
        # Test that MISSING works the same as a default factory not
        #  being specified (which is really the same as a default not
        #  being specified, too).
        @dataclass
        class C:
            x: int=field(default_factory=MISSING)
        with self.assertRaisesRegex(TypeError,
                                    r'__init__\(\) missing 1 required '
                                    'positional argument'):
            C()
        self.assertNotIn('x', C.__dict__)

        @dataclass
        class D:
            x: int=field(default=MISSING, default_factory=MISSING)
        with self.assertRaisesRegex(TypeError,
                                    r'__init__\(\) missing 1 required '
                                    'positional argument'):
            D()
        self.assertNotIn('x', D.__dict__)

    def test_missing_repr(self):
        self.assertIn('MISSING_TYPE object', repr(MISSING))

    def test_dont_include_other_annotations(self):
        @dataclass
        class C:
            i: int
            def foo(self) -> int:
                return 4
            @property
            def bar(self) -> int:
                return 5
        self.assertEqual(list(C.__annotations__), ['i'])
        self.assertEqual(C(10).foo(), 4)
        self.assertEqual(C(10).bar, 5)
        self.assertEqual(C(10).i, 10)

    def test_post_init(self):
        # Just make sure it gets called
        @dataclass
        class C:
            def __post_init__(self):
                raise CustomError()
        with self.assertRaises(CustomError):
            C()

        @dataclass
        class C:
            i: int = 10
            def __post_init__(self):
                if self.i == 10:
                    raise CustomError()
        with self.assertRaises(CustomError):
            C()
        # post-init gets called, but doesn't raise. This is just
        #  checking that self is used correctly.
        C(5)

        # If there's not an __init__, then post-init won't get called.
        @dataclass(init=False)
        class C:
            def __post_init__(self):
                raise CustomError()
        # Creating the class won't raise
        C()

        @dataclass
        class C:
            x: int = 0
            def __post_init__(self):
                self.x *= 2
        self.assertEqual(C().x, 0)
        self.assertEqual(C(2).x, 4)

        # Make sure that if we're frozen, post-init can't set
        #  attributes.
        @dataclass(frozen=True)
        class C:
            x: int = 0
            def __post_init__(self):
                self.x *= 2
        with self.assertRaises(FrozenInstanceError):
            C()

    def test_post_init_super(self):
        # Make sure super() post-init isn't called by default.
        class B:
            def __post_init__(self):
                raise CustomError()

        @dataclass
        class C(B):
            def __post_init__(self):
                self.x = 5

        self.assertEqual(C().x, 5)

        # Now call super(), and it will raise.
        @dataclass
        class C(B):
            def __post_init__(self):
                super().__post_init__()

        with self.assertRaises(CustomError):
            C()

        # Make sure post-init is called, even if not defined in our
        #  class.
        @dataclass
        class C(B):
            pass

        with self.assertRaises(CustomError):
            C()

    def test_post_init_staticmethod(self):
        flag = False
        @dataclass
        class C:
            x: int
            y: int
            @staticmethod
            def __post_init__():
                nonlocal flag
                flag = True

        self.assertFalse(flag)
        c = C(3, 4)
        self.assertEqual((c.x, c.y), (3, 4))
        self.assertTrue(flag)

    def test_post_init_classmethod(self):
        @dataclass
        class C:
            flag = False
            x: int
            y: int
            @classmethod
            def __post_init__(cls):
                cls.flag = True

        self.assertFalse(C.flag)
        c = C(3, 4)
        self.assertEqual((c.x, c.y), (3, 4))
        self.assertTrue(C.flag)

    def test_class_var(self):
        # Make sure ClassVars are ignored in __init__, __repr__, etc.
        @dataclass
        class C:
            x: int
            y: int = 10
            z: ClassVar[int] = 1000
            w: ClassVar[int] = 2000
            t: ClassVar[int] = 3000

        c = C(5)
        self.assertEqual(repr(c), 'TestCase.test_class_var.<locals>.C(x=5, y=10)')
        self.assertEqual(len(fields(C)), 2)                 # We have 2 fields.
        self.assertEqual(len(C.__annotations__), 5)         # And 3 ClassVars.
        self.assertEqual(c.z, 1000)
        self.assertEqual(c.w, 2000)
        self.assertEqual(c.t, 3000)
        C.z += 1
        self.assertEqual(c.z, 1001)
        c = C(20)
        self.assertEqual((c.x, c.y), (20, 10))
        self.assertEqual(c.z, 1001)
        self.assertEqual(c.w, 2000)
        self.assertEqual(c.t, 3000)

    def test_class_var_no_default(self):
        # If a ClassVar has no default value, it should not be set on the class.
        @dataclass
        class C:
            x: ClassVar[int]

        self.assertNotIn('x', C.__dict__)

    def test_class_var_default_factory(self):
        # It makes no sense for a ClassVar to have a default factory. When
        #  would it be called? Call it yourself, since it's class-wide.
        with self.assertRaisesRegex(TypeError,
                                    'cannot have a default factory'):
            @dataclass
            class C:
                x: ClassVar[int] = field(default_factory=int)

            self.assertNotIn('x', C.__dict__)

    def test_class_var_with_default(self):
        # If a ClassVar has a default value, it should be set on the class.
        @dataclass
        class C:
            x: ClassVar[int] = 10
        self.assertEqual(C.x, 10)

        @dataclass
        class C:
            x: ClassVar[int] = field(default=10)
        self.assertEqual(C.x, 10)

    def test_class_var_frozen(self):
        # Make sure ClassVars work even if we're frozen.
        @dataclass(frozen=True)
        class C:
            x: int
            y: int = 10
            z: ClassVar[int] = 1000
            w: ClassVar[int] = 2000
            t: ClassVar[int] = 3000

        c = C(5)
        self.assertEqual(repr(C(5)), 'TestCase.test_class_var_frozen.<locals>.C(x=5, y=10)')
        self.assertEqual(len(fields(C)), 2)                 # We have 2 fields
        self.assertEqual(len(C.__annotations__), 5)         # And 3 ClassVars
        self.assertEqual(c.z, 1000)
        self.assertEqual(c.w, 2000)
        self.assertEqual(c.t, 3000)
        # We can still modify the ClassVar, it's only instances that are
        #  frozen.
        C.z += 1
        self.assertEqual(c.z, 1001)
        c = C(20)
        self.assertEqual((c.x, c.y), (20, 10))
        self.assertEqual(c.z, 1001)
        self.assertEqual(c.w, 2000)
        self.assertEqual(c.t, 3000)

    def test_init_var_no_default(self):
        # If an InitVar has no default value, it should not be set on the class.
        @dataclass
        class C:
            x: InitVar[int]

        self.assertNotIn('x', C.__dict__)

    def test_init_var_default_factory(self):
        # It makes no sense for an InitVar to have a default factory. When
        #  would it be called? Call it yourself, since it's class-wide.
        with self.assertRaisesRegex(TypeError,
                                    'cannot have a default factory'):
            @dataclass
            class C:
                x: InitVar[int] = field(default_factory=int)

            self.assertNotIn('x', C.__dict__)

    def test_init_var_with_default(self):
        # If an InitVar has a default value, it should be set on the class.
        @dataclass
        class C:
            x: InitVar[int] = 10
        self.assertEqual(C.x, 10)

        @dataclass
        class C:
            x: InitVar[int] = field(default=10)
        self.assertEqual(C.x, 10)

    def test_init_var(self):
        @dataclass
        class C:
            x: int = None
            init_param: InitVar[int] = None

            def __post_init__(self, init_param):
                if self.x is None:
                    self.x = init_param*2

        c = C(init_param=10)
        self.assertEqual(c.x, 20)

    def test_init_var_inheritance(self):
        # Note that this deliberately tests that a dataclass need not
        #  have a __post_init__ function if it has an InitVar field.
        #  It could just be used in a derived class, as shown here.
        @dataclass
        class Base:
            x: int
            init_base: InitVar[int]

        # We can instantiate by passing the InitVar, even though
        #  it's not used.
        b = Base(0, 10)
        self.assertEqual(vars(b), {'x': 0})

        @dataclass
        class C(Base):
            y: int
            init_derived: InitVar[int]

            def __post_init__(self, init_base, init_derived):
                self.x = self.x + init_base
                self.y = self.y + init_derived

        c = C(10, 11, 50, 51)
        self.assertEqual(vars(c), {'x': 21, 'y': 101})

    def test_default_factory(self):
        # Test a factory that returns a new list.
        @dataclass
        class C:
            x: int
            y: list = field(default_factory=list)

        c0 = C(3)
        c1 = C(3)
        self.assertEqual(c0.x, 3)
        self.assertEqual(c0.y, [])
        self.assertEqual(c0, c1)
        self.assertIsNot(c0.y, c1.y)
        self.assertEqual(astuple(C(5, [1])), (5, [1]))

        # Test a factory that returns a shared list.
        l = []
        @dataclass
        class C:
            x: int
            y: list = field(default_factory=lambda: l)

        c0 = C(3)
        c1 = C(3)
        self.assertEqual(c0.x, 3)
        self.assertEqual(c0.y, [])
        self.assertEqual(c0, c1)
        self.assertIs(c0.y, c1.y)
        self.assertEqual(astuple(C(5, [1])), (5, [1]))

        # Test various other field flags.
        # repr
        @dataclass
        class C:
            x: list = field(default_factory=list, repr=False)
        self.assertEqual(repr(C()), 'TestCase.test_default_factory.<locals>.C()')
        self.assertEqual(C().x, [])

        # hash
        @dataclass(unsafe_hash=True)
        class C:
            x: list = field(default_factory=list, hash=False)
        self.assertEqual(astuple(C()), ([],))
        self.assertEqual(hash(C()), hash(()))

        # init (see also test_default_factory_with_no_init)
        @dataclass
        class C:
            x: list = field(default_factory=list, init=False)
        self.assertEqual(astuple(C()), ([],))

        # compare
        @dataclass
        class C:
            x: list = field(default_factory=list, compare=False)
        self.assertEqual(C(), C([1]))

    def test_default_factory_with_no_init(self):
        # We need a factory with a side effect.
        factory = Mock()

        @dataclass
        class C:
            x: list = field(default_factory=factory, init=False)

        # Make sure the default factory is called for each new instance.
        C().x
        self.assertEqual(factory.call_count, 1)
        C().x
        self.assertEqual(factory.call_count, 2)

    def test_default_factory_not_called_if_value_given(self):
        # We need a factory that we can test if it's been called.
        factory = Mock()

        @dataclass
        class C:
            x: int = field(default_factory=factory)

        # Make sure that if a field has a default factory function,
        #  it's not called if a value is specified.
        C().x
        self.assertEqual(factory.call_count, 1)
        self.assertEqual(C(10).x, 10)
        self.assertEqual(factory.call_count, 1)
        C().x
        self.assertEqual(factory.call_count, 2)

    def test_default_factory_derived(self):
        # See bpo-32896.
        @dataclass
        class Foo:
            x: dict = field(default_factory=dict)

        @dataclass
        class Bar(Foo):
            y: int = 1

        self.assertEqual(Foo().x, {})
        self.assertEqual(Bar().x, {})
        self.assertEqual(Bar().y, 1)

        @dataclass
        class Baz(Foo):
            pass
        self.assertEqual(Baz().x, {})

    def test_intermediate_non_dataclass(self):
        # Test that an intermediate class that defines
        #  annotations does not define fields.

        @dataclass
        class A:
            x: int

        class B(A):
            y: int

        @dataclass
        class C(B):
            z: int

        c = C(1, 3)
        self.assertEqual((c.x, c.z), (1, 3))

        # .y was not initialized.
        with self.assertRaisesRegex(AttributeError,
                                    'object has no attribute'):
            c.y

        # And if we again derive a non-dataclass, no fields are added.
        class D(C):
            t: int
        d = D(4, 5)
        self.assertEqual((d.x, d.z), (4, 5))

    def test_classvar_default_factory(self):
        # It's an error for a ClassVar to have a factory function.
        with self.assertRaisesRegex(TypeError,
                                    'cannot have a default factory'):
            @dataclass
            class C:
                x: ClassVar[int] = field(default_factory=int)

    def test_is_dataclass(self):
        class NotDataClass:
            pass

        self.assertFalse(is_dataclass(0))
        self.assertFalse(is_dataclass(int))
        self.assertFalse(is_dataclass(NotDataClass))
        self.assertFalse(is_dataclass(NotDataClass()))

        @dataclass
        class C:
            x: int

        @dataclass
        class D:
            d: C
            e: int

        c = C(10)
        d = D(c, 4)

        self.assertTrue(is_dataclass(C))
        self.assertTrue(is_dataclass(c))
        self.assertFalse(is_dataclass(c.x))
        self.assertTrue(is_dataclass(d.d))
        self.assertFalse(is_dataclass(d.e))

    def test_helper_fields_with_class_instance(self):
        # Check that we can call fields() on either a class or instance,
        #  and get back the same thing.
        @dataclass
        class C:
            x: int
            y: float

        self.assertEqual(fields(C), fields(C(0, 0.0)))

    def test_helper_fields_exception(self):
        # Check that TypeError is raised if not passed a dataclass or
        #  instance.
        with self.assertRaisesRegex(TypeError, 'dataclass type or instance'):
            fields(0)

        class C: pass
        with self.assertRaisesRegex(TypeError, 'dataclass type or instance'):
            fields(C)
        with self.assertRaisesRegex(TypeError, 'dataclass type or instance'):
            fields(C())

    def test_helper_asdict(self):
        # Basic tests for asdict(), it should return a new dictionary.
        @dataclass
        class C:
            x: int
            y: int
        c = C(1, 2)

        self.assertEqual(asdict(c), {'x': 1, 'y': 2})
        self.assertEqual(asdict(c), asdict(c))
        self.assertIsNot(asdict(c), asdict(c))
        c.x = 42
        self.assertEqual(asdict(c), {'x': 42, 'y': 2})
        self.assertIs(type(asdict(c)), dict)

    def test_helper_asdict_raises_on_classes(self):
        # asdict() should raise on a class object.
        @dataclass
        class C:
            x: int
            y: int
        with self.assertRaisesRegex(TypeError, 'dataclass instance'):
            asdict(C)
        with self.assertRaisesRegex(TypeError, 'dataclass instance'):
            asdict(int)

    def test_helper_asdict_copy_values(self):
        @dataclass
        class C:
            x: int
            y: List[int] = field(default_factory=list)
        initial = []
        c = C(1, initial)
        d = asdict(c)
        self.assertEqual(d['y'], initial)
        self.assertIsNot(d['y'], initial)
        c = C(1)
        d = asdict(c)
        d['y'].append(1)
        self.assertEqual(c.y, [])

    def test_helper_asdict_nested(self):
        @dataclass
        class UserId:
            token: int
            group: int
        @dataclass
        class User:
            name: str
            id: UserId
        u = User('Joe', UserId(123, 1))
        d = asdict(u)
        self.assertEqual(d, {'name': 'Joe', 'id': {'token': 123, 'group': 1}})
        self.assertIsNot(asdict(u), asdict(u))
        u.id.group = 2
        self.assertEqual(asdict(u), {'name': 'Joe',
                                     'id': {'token': 123, 'group': 2}})

    def test_helper_asdict_builtin_containers(self):
        @dataclass
        class User:
            name: str
            id: int
        @dataclass
        class GroupList:
            id: int
            users: List[User]
        @dataclass
        class GroupTuple:
            id: int
            users: Tuple[User, ...]
        @dataclass
        class GroupDict:
            id: int
            users: Dict[str, User]
        a = User('Alice', 1)
        b = User('Bob', 2)
        gl = GroupList(0, [a, b])
        gt = GroupTuple(0, (a, b))
        gd = GroupDict(0, {'first': a, 'second': b})
        self.assertEqual(asdict(gl), {'id': 0, 'users': [{'name': 'Alice', 'id': 1},
                                                         {'name': 'Bob', 'id': 2}]})
        self.assertEqual(asdict(gt), {'id': 0, 'users': ({'name': 'Alice', 'id': 1},
                                                         {'name': 'Bob', 'id': 2})})
        self.assertEqual(asdict(gd), {'id': 0, 'users': {'first': {'name': 'Alice', 'id': 1},
                                                         'second': {'name': 'Bob', 'id': 2}}})

    def test_helper_asdict_builtin_containers(self):
        @dataclass
        class Child:
            d: object

        @dataclass
        class Parent:
            child: Child

        self.assertEqual(asdict(Parent(Child([1]))), {'child': {'d': [1]}})
        self.assertEqual(asdict(Parent(Child({1: 2}))), {'child': {'d': {1: 2}}})

    def test_helper_asdict_factory(self):
        @dataclass
        class C:
            x: int
            y: int
        c = C(1, 2)
        d = asdict(c, dict_factory=OrderedDict)
        self.assertEqual(d, OrderedDict([('x', 1), ('y', 2)]))
        self.assertIsNot(d, asdict(c, dict_factory=OrderedDict))
        c.x = 42
        d = asdict(c, dict_factory=OrderedDict)
        self.assertEqual(d, OrderedDict([('x', 42), ('y', 2)]))
        self.assertIs(type(d), OrderedDict)

    def test_helper_astuple(self):
        # Basic tests for astuple(), it should return a new tuple.
        @dataclass
        class C:
            x: int
            y: int = 0
        c = C(1)

        self.assertEqual(astuple(c), (1, 0))
        self.assertEqual(astuple(c), astuple(c))
        self.assertIsNot(astuple(c), astuple(c))
        c.y = 42
        self.assertEqual(astuple(c), (1, 42))
        self.assertIs(type(astuple(c)), tuple)

    def test_helper_astuple_raises_on_classes(self):
        # astuple() should raise on a class object.
        @dataclass
        class C:
            x: int
            y: int
        with self.assertRaisesRegex(TypeError, 'dataclass instance'):
            astuple(C)
        with self.assertRaisesRegex(TypeError, 'dataclass instance'):
            astuple(int)

    def test_helper_astuple_copy_values(self):
        @dataclass
        class C:
            x: int
            y: List[int] = field(default_factory=list)
        initial = []
        c = C(1, initial)
        t = astuple(c)
        self.assertEqual(t[1], initial)
        self.assertIsNot(t[1], initial)
        c = C(1)
        t = astuple(c)
        t[1].append(1)
        self.assertEqual(c.y, [])

    def test_helper_astuple_nested(self):
        @dataclass
        class UserId:
            token: int
            group: int
        @dataclass
        class User:
            name: str
            id: UserId
        u = User('Joe', UserId(123, 1))
        t = astuple(u)
        self.assertEqual(t, ('Joe', (123, 1)))
        self.assertIsNot(astuple(u), astuple(u))
        u.id.group = 2
        self.assertEqual(astuple(u), ('Joe', (123, 2)))

    def test_helper_astuple_builtin_containers(self):
        @dataclass
        class User:
            name: str
            id: int
        @dataclass
        class GroupList:
            id: int
            users: List[User]
        @dataclass
        class GroupTuple:
            id: int
            users: Tuple[User, ...]
        @dataclass
        class GroupDict:
            id: int
            users: Dict[str, User]
        a = User('Alice', 1)
        b = User('Bob', 2)
        gl = GroupList(0, [a, b])
        gt = GroupTuple(0, (a, b))
        gd = GroupDict(0, {'first': a, 'second': b})
        self.assertEqual(astuple(gl), (0, [('Alice', 1), ('Bob', 2)]))
        self.assertEqual(astuple(gt), (0, (('Alice', 1), ('Bob', 2))))
        self.assertEqual(astuple(gd), (0, {'first': ('Alice', 1), 'second': ('Bob', 2)}))

    def test_helper_astuple_builtin_containers(self):
        @dataclass
        class Child:
            d: object

        @dataclass
        class Parent:
            child: Child

        self.assertEqual(astuple(Parent(Child([1]))), (([1],),))
        self.assertEqual(astuple(Parent(Child({1: 2}))), (({1: 2},),))

    def test_helper_astuple_factory(self):
        @dataclass
        class C:
            x: int
            y: int
        NT = namedtuple('NT', 'x y')
        def nt(lst):
            return NT(*lst)
        c = C(1, 2)
        t = astuple(c, tuple_factory=nt)
        self.assertEqual(t, NT(1, 2))
        self.assertIsNot(t, astuple(c, tuple_factory=nt))
        c.x = 42
        t = astuple(c, tuple_factory=nt)
        self.assertEqual(t, NT(42, 2))
        self.assertIs(type(t), NT)

    def test_dynamic_class_creation(self):
        cls_dict = {'__annotations__': {'x': int, 'y': int},
                    }

        # Create the class.
        cls = type('C', (), cls_dict)

        # Make it a dataclass.
        cls1 = dataclass(cls)

        self.assertEqual(cls1, cls)
        self.assertEqual(asdict(cls(1, 2)), {'x': 1, 'y': 2})

    def test_dynamic_class_creation_using_field(self):
        cls_dict = {'__annotations__': {'x': int, 'y': int},
                    'y': field(default=5),
                    }

        # Create the class.
        cls = type('C', (), cls_dict)

        # Make it a dataclass.
        cls1 = dataclass(cls)

        self.assertEqual(cls1, cls)
        self.assertEqual(asdict(cls1(1)), {'x': 1, 'y': 5})

    def test_init_in_order(self):
        @dataclass
        class C:
            a: int
            b: int = field()
            c: list = field(default_factory=list, init=False)
            d: list = field(default_factory=list)
            e: int = field(default=4, init=False)
            f: int = 4

        calls = []
        def setattr(self, name, value):
            calls.append((name, value))

        C.__setattr__ = setattr
        c = C(0, 1)
        self.assertEqual(('a', 0), calls[0])
        self.assertEqual(('b', 1), calls[1])
        self.assertEqual(('c', []), calls[2])
        self.assertEqual(('d', []), calls[3])
        self.assertNotIn(('e', 4), calls)
        self.assertEqual(('f', 4), calls[4])

    def test_items_in_dicts(self):
        @dataclass
        class C:
            a: int
            b: list = field(default_factory=list, init=False)
            c: list = field(default_factory=list)
            d: int = field(default=4, init=False)
            e: int = 0

        c = C(0)
        # Class dict
        self.assertNotIn('a', C.__dict__)
        self.assertNotIn('b', C.__dict__)
        self.assertNotIn('c', C.__dict__)
        self.assertIn('d', C.__dict__)
        self.assertEqual(C.d, 4)
        self.assertIn('e', C.__dict__)
        self.assertEqual(C.e, 0)
        # Instance dict
        self.assertIn('a', c.__dict__)
        self.assertEqual(c.a, 0)
        self.assertIn('b', c.__dict__)
        self.assertEqual(c.b, [])
        self.assertIn('c', c.__dict__)
        self.assertEqual(c.c, [])
        self.assertNotIn('d', c.__dict__)
        self.assertIn('e', c.__dict__)
        self.assertEqual(c.e, 0)

    def test_alternate_classmethod_constructor(self):
        # Since __post_init__ can't take params, use a classmethod
        #  alternate constructor.  This is mostly an example to show
        #  how to use this technique.
        @dataclass
        class C:
            x: int
            @classmethod
            def from_file(cls, filename):
                # In a real example, create a new instance
                #  and populate 'x' from contents of a file.
                value_in_file = 20
                return cls(value_in_file)

        self.assertEqual(C.from_file('filename').x, 20)

    def test_field_metadata_default(self):
        # Make sure the default metadata is read-only and of
        #  zero length.
        @dataclass
        class C:
            i: int

        self.assertFalse(fields(C)[0].metadata)
        self.assertEqual(len(fields(C)[0].metadata), 0)
        with self.assertRaisesRegex(TypeError,
                                    'does not support item assignment'):
            fields(C)[0].metadata['test'] = 3

    def test_field_metadata_mapping(self):
        # Make sure only a mapping can be passed as metadata
        #  zero length.
        with self.assertRaises(TypeError):
            @dataclass
            class C:
                i: int = field(metadata=0)

        # Make sure an empty dict works.
        @dataclass
        class C:
            i: int = field(metadata={})
        self.assertFalse(fields(C)[0].metadata)
        self.assertEqual(len(fields(C)[0].metadata), 0)
        with self.assertRaisesRegex(TypeError,
                                    'does not support item assignment'):
            fields(C)[0].metadata['test'] = 3

        # Make sure a non-empty dict works.
        @dataclass
        class C:
            i: int = field(metadata={'test': 10, 'bar': '42', 3: 'three'})
        self.assertEqual(len(fields(C)[0].metadata), 3)
        self.assertEqual(fields(C)[0].metadata['test'], 10)
        self.assertEqual(fields(C)[0].metadata['bar'], '42')
        self.assertEqual(fields(C)[0].metadata[3], 'three')
        with self.assertRaises(KeyError):
            # Non-existent key.
            fields(C)[0].metadata['baz']
        with self.assertRaisesRegex(TypeError,
                                    'does not support item assignment'):
            fields(C)[0].metadata['test'] = 3

    def test_field_metadata_custom_mapping(self):
        # Try a custom mapping.
        class SimpleNameSpace:
            def __init__(self, **kw):
                self.__dict__.update(kw)

            def __getitem__(self, item):
                if item == 'xyzzy':
                    return 'plugh'
                return getattr(self, item)

            def __len__(self):
                return self.__dict__.__len__()

        @dataclass
        class C:
            i: int = field(metadata=SimpleNameSpace(a=10))

        self.assertEqual(len(fields(C)[0].metadata), 1)
        self.assertEqual(fields(C)[0].metadata['a'], 10)
        with self.assertRaises(AttributeError):
            fields(C)[0].metadata['b']
        # Make sure we're still talking to our custom mapping.
        self.assertEqual(fields(C)[0].metadata['xyzzy'], 'plugh')

    def test_generic_dataclasses(self):
        T = TypeVar('T')

        @dataclass
        class LabeledBox(Generic[T]):
            content: T
            label: str = '<unknown>'

        box = LabeledBox(42)
        self.assertEqual(box.content, 42)
        self.assertEqual(box.label, '<unknown>')

        # Subscripting the resulting class should work, etc.
        Alias = List[LabeledBox[int]]

    def test_generic_extending(self):
        S = TypeVar('S')
        T = TypeVar('T')

        @dataclass
        class Base(Generic[T, S]):
            x: T
            y: S

        @dataclass
        class DataDerived(Base[int, T]):
            new_field: str
        Alias = DataDerived[str]
        c = Alias(0, 'test1', 'test2')
        self.assertEqual(astuple(c), (0, 'test1', 'test2'))

        class NonDataDerived(Base[int, T]):
            def new_method(self):
                return self.y
        Alias = NonDataDerived[float]
        c = Alias(10, 1.0)
        self.assertEqual(c.new_method(), 1.0)

    def test_helper_replace(self):
        @dataclass(frozen=True)
        class C:
            x: int
            y: int

        c = C(1, 2)
        c1 = replace(c, x=3)
        self.assertEqual(c1.x, 3)
        self.assertEqual(c1.y, 2)

    def test_helper_replace_frozen(self):
        @dataclass(frozen=True)
        class C:
            x: int
            y: int
            z: int = field(init=False, default=10)
            t: int = field(init=False, default=100)

        c = C(1, 2)
        c1 = replace(c, x=3)
        self.assertEqual((c.x, c.y, c.z, c.t), (1, 2, 10, 100))
        self.assertEqual((c1.x, c1.y, c1.z, c1.t), (3, 2, 10, 100))


        with self.assertRaisesRegex(ValueError, 'init=False'):
            replace(c, x=3, z=20, t=50)
        with self.assertRaisesRegex(ValueError, 'init=False'):
            replace(c, z=20)
            replace(c, x=3, z=20, t=50)

        # Make sure the result is still frozen.
        with self.assertRaisesRegex(FrozenInstanceError, "cannot assign to field 'x'"):
            c1.x = 3

        # Make sure we can't replace an attribute that doesn't exist,
        #  if we're also replacing one that does exist.  Test this
        #  here, because setting attributes on frozen instances is
        #  handled slightly differently from non-frozen ones.
        with self.assertRaisesRegex(TypeError, r"__init__\(\) got an unexpected "
                                             "keyword argument 'a'"):
            c1 = replace(c, x=20, a=5)

    def test_helper_replace_invalid_field_name(self):
        @dataclass(frozen=True)
        class C:
            x: int
            y: int

        c = C(1, 2)
        with self.assertRaisesRegex(TypeError, r"__init__\(\) got an unexpected "
                                    "keyword argument 'z'"):
            c1 = replace(c, z=3)

    def test_helper_replace_invalid_object(self):
        @dataclass(frozen=True)
        class C:
            x: int
            y: int

        with self.assertRaisesRegex(TypeError, 'dataclass instance'):
            replace(C, x=3)

        with self.assertRaisesRegex(TypeError, 'dataclass instance'):
            replace(0, x=3)

    def test_helper_replace_no_init(self):
        @dataclass
        class C:
            x: int
            y: int = field(init=False, default=10)

        c = C(1)
        c.y = 20

        # Make sure y gets the default value.
        c1 = replace(c, x=5)
        self.assertEqual((c1.x, c1.y), (5, 10))

        # Trying to replace y is an error.
        with self.assertRaisesRegex(ValueError, 'init=False'):
            replace(c, x=2, y=30)
            with self.assertRaisesRegex(ValueError, 'init=False'):
                replace(c, y=30)

    def test_dataclassses_pickleable(self):
        global P, Q, R
        @dataclass
        class P:
            x: int
            y: int = 0
        @dataclass
        class Q:
            x: int
            y: int = field(default=0, init=False)
        @dataclass
        class R:
            x: int
            y: List[int] = field(default_factory=list)
        q = Q(1)
        q.y = 2
        samples = [P(1), P(1, 2), Q(1), q, R(1), R(1, [2, 3, 4])]
        for sample in samples:
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                with self.subTest(sample=sample, proto=proto):
                    new_sample = pickle.loads(pickle.dumps(sample, proto))
                    self.assertEqual(sample.x, new_sample.x)
                    self.assertEqual(sample.y, new_sample.y)
                    self.assertIsNot(sample, new_sample)
                    new_sample.x = 42
                    another_new_sample = pickle.loads(pickle.dumps(new_sample, proto))
                    self.assertEqual(new_sample.x, another_new_sample.x)
                    self.assertEqual(sample.y, another_new_sample.y)

    def test_helper_make_dataclass(self):
        C = make_dataclass('C',
                           [('x', int),
                            ('y', int, field(default=5))],
                           namespace={'add_one': lambda self: self.x + 1})
        c = C(10)
        self.assertEqual((c.x, c.y), (10, 5))
        self.assertEqual(c.add_one(), 11)


    def test_helper_make_dataclass_no_mutate_namespace(self):
        # Make sure a provided namespace isn't mutated.
        ns = {}
        C = make_dataclass('C',
                           [('x', int),
                            ('y', int, field(default=5))],
                           namespace=ns)
        self.assertEqual(ns, {})

    def test_helper_make_dataclass_base(self):
        class Base1:
            pass
        class Base2:
            pass
        C = make_dataclass('C',
                           [('x', int)],
                           bases=(Base1, Base2))
        c = C(2)
        self.assertIsInstance(c, C)
        self.assertIsInstance(c, Base1)
        self.assertIsInstance(c, Base2)

    def test_helper_make_dataclass_base_dataclass(self):
        @dataclass
        class Base1:
            x: int
        class Base2:
            pass
        C = make_dataclass('C',
                           [('y', int)],
                           bases=(Base1, Base2))
        with self.assertRaisesRegex(TypeError, 'required positional'):
            c = C(2)
        c = C(1, 2)
        self.assertIsInstance(c, C)
        self.assertIsInstance(c, Base1)
        self.assertIsInstance(c, Base2)

        self.assertEqual((c.x, c.y), (1, 2))

    def test_helper_make_dataclass_init_var(self):
        def post_init(self, y):
            self.x *= y

        C = make_dataclass('C',
                           [('x', int),
                            ('y', InitVar[int]),
                            ],
                           namespace={'__post_init__': post_init},
                           )
        c = C(2, 3)
        self.assertEqual(vars(c), {'x': 6})
        self.assertEqual(len(fields(c)), 1)

    def test_helper_make_dataclass_class_var(self):
        C = make_dataclass('C',
                           [('x', int),
                            ('y', ClassVar[int], 10),
                            ('z', ClassVar[int], field(default=20)),
                            ])
        c = C(1)
        self.assertEqual(vars(c), {'x': 1})
        self.assertEqual(len(fields(c)), 1)
        self.assertEqual(C.y, 10)
        self.assertEqual(C.z, 20)

    def test_helper_make_dataclass_other_params(self):
        C = make_dataclass('C',
                           [('x', int),
                            ('y', ClassVar[int], 10),
                            ('z', ClassVar[int], field(default=20)),
                            ],
                           init=False)
        # Make sure we have a repr, but no init.
        self.assertNotIn('__init__', vars(C))
        self.assertIn('__repr__', vars(C))

        # Make sure random other params don't work.
        with self.assertRaisesRegex(TypeError, 'unexpected keyword argument'):
            C = make_dataclass('C',
                               [],
                               xxinit=False)

    def test_helper_make_dataclass_no_types(self):
        C = make_dataclass('Point', ['x', 'y', 'z'])
        c = C(1, 2, 3)
        self.assertEqual(vars(c), {'x': 1, 'y': 2, 'z': 3})
        self.assertEqual(C.__annotations__, {'x': 'typing.Any',
                                             'y': 'typing.Any',
                                             'z': 'typing.Any'})

        C = make_dataclass('Point', ['x', ('y', int), 'z'])
        c = C(1, 2, 3)
        self.assertEqual(vars(c), {'x': 1, 'y': 2, 'z': 3})
        self.assertEqual(C.__annotations__, {'x': 'typing.Any',
                                             'y': int,
                                             'z': 'typing.Any'})


class TestFieldNoAnnotation(unittest.TestCase):
    def test_field_without_annotation(self):
        with self.assertRaisesRegex(TypeError,
                                    "'f' is a field but has no type annotation"):
            @dataclass
            class C:
                f = field()

    def test_field_without_annotation_but_annotation_in_base(self):
        @dataclass
        class B:
            f: int

        with self.assertRaisesRegex(TypeError,
                                    "'f' is a field but has no type annotation"):
            # This is still an error: make sure we don't pick up the
            #  type annotation in the base class.
            @dataclass
            class C(B):
                f = field()

    def test_field_without_annotation_but_annotation_in_base_not_dataclass(self):
        # Same test, but with the base class not a dataclass.
        class B:
            f: int

        with self.assertRaisesRegex(TypeError,
                                    "'f' is a field but has no type annotation"):
            # This is still an error: make sure we don't pick up the
            #  type annotation in the base class.
            @dataclass
            class C(B):
                f = field()


class TestDocString(unittest.TestCase):
    def assertDocStrEqual(self, a, b):
        # Because 3.6 and 3.7 differ in how inspect.signature work
        #  (see bpo #32108), for the time being just compare them with
        #  whitespace stripped.
        self.assertEqual(a.replace(' ', ''), b.replace(' ', ''))

    def test_existing_docstring_not_overridden(self):
        @dataclass
        class C:
            """Lorem ipsum"""
            x: int

        self.assertEqual(C.__doc__, "Lorem ipsum")

    def test_docstring_no_fields(self):
        @dataclass
        class C:
            pass

        self.assertDocStrEqual(C.__doc__, "C()")

    def test_docstring_one_field(self):
        @dataclass
        class C:
            x: int

        self.assertDocStrEqual(C.__doc__, "C(x:int)")

    def test_docstring_two_fields(self):
        @dataclass
        class C:
            x: int
            y: int

        self.assertDocStrEqual(C.__doc__, "C(x:int, y:int)")

    def test_docstring_three_fields(self):
        @dataclass
        class C:
            x: int
            y: int
            z: str

        self.assertDocStrEqual(C.__doc__, "C(x:int, y:int, z:str)")

    def test_docstring_one_field_with_default(self):
        @dataclass
        class C:
            x: int = 3

        self.assertDocStrEqual(C.__doc__, "C(x:int=3)")

    def test_docstring_one_field_with_default_none(self):
        @dataclass
        class C:
            x: Union[int, type(None)] = None

        self.assertDocStrEqual(C.__doc__, "C(x:Union[int, NoneType]=None)")

    def test_docstring_list_field(self):
        @dataclass
        class C:
            x: List[int]

        self.assertDocStrEqual(C.__doc__, "C(x:List[int])")

    def test_docstring_list_field_with_default_factory(self):
        @dataclass
        class C:
            x: List[int] = field(default_factory=list)

        self.assertDocStrEqual(C.__doc__, "C(x:List[int]=<factory>)")

    def test_docstring_deque_field(self):
        @dataclass
        class C:
            x: deque

        self.assertDocStrEqual(C.__doc__, "C(x:collections.deque)")

    def test_docstring_deque_field_with_default_factory(self):
        @dataclass
        class C:
            x: deque = field(default_factory=deque)

        self.assertDocStrEqual(C.__doc__, "C(x:collections.deque=<factory>)")


class TestInit(unittest.TestCase):
    def test_base_has_init(self):
        class B:
            def __init__(self):
                self.z = 100
                pass

        # Make sure that declaring this class doesn't raise an error.
        #  The issue is that we can't override __init__ in our class,
        #  but it should be okay to add __init__ to us if our base has
        #  an __init__.
        @dataclass
        class C(B):
            x: int = 0
        c = C(10)
        self.assertEqual(c.x, 10)
        self.assertNotIn('z', vars(c))

        # Make sure that if we don't add an init, the base __init__
        #  gets called.
        @dataclass(init=False)
        class C(B):
            x: int = 10
        c = C()
        self.assertEqual(c.x, 10)
        self.assertEqual(c.z, 100)

    def test_no_init(self):
        dataclass(init=False)
        class C:
            i: int = 0
        self.assertEqual(C().i, 0)

        dataclass(init=False)
        class C:
            i: int = 2
            def __init__(self):
                self.i = 3
        self.assertEqual(C().i, 3)

    def test_overwriting_init(self):
        # If the class has __init__, use it no matter the value of
        #  init=.

        @dataclass
        class C:
            x: int
            def __init__(self, x):
                self.x = 2 * x
        self.assertEqual(C(3).x, 6)

        @dataclass(init=True)
        class C:
            x: int
            def __init__(self, x):
                self.x = 2 * x
        self.assertEqual(C(4).x, 8)

        @dataclass(init=False)
        class C:
            x: int
            def __init__(self, x):
                self.x = 2 * x
        self.assertEqual(C(5).x, 10)


class TestRepr(unittest.TestCase):
    def test_repr(self):
        @dataclass
        class B:
            x: int

        @dataclass
        class C(B):
            y: int = 10

        o = C(4)
        self.assertEqual(repr(o), 'TestRepr.test_repr.<locals>.C(x=4, y=10)')

        @dataclass
        class D(C):
            x: int = 20
        self.assertEqual(repr(D()), 'TestRepr.test_repr.<locals>.D(x=20, y=10)')

        @dataclass
        class C:
            @dataclass
            class D:
                i: int
            @dataclass
            class E:
                pass
        self.assertEqual(repr(C.D(0)), 'TestRepr.test_repr.<locals>.C.D(i=0)')
        self.assertEqual(repr(C.E()), 'TestRepr.test_repr.<locals>.C.E()')

    def test_no_repr(self):
        # Test a class with no __repr__ and repr=False.
        @dataclass(repr=False)
        class C:
            x: int
        self.assertIn('test_dataclasses.TestRepr.test_no_repr.<locals>.C object at',
                      repr(C(3)))

        # Test a class with a __repr__ and repr=False.
        @dataclass(repr=False)
        class C:
            x: int
            def __repr__(self):
                return 'C-class'
        self.assertEqual(repr(C(3)), 'C-class')

    def test_overwriting_repr(self):
        # If the class has __repr__, use it no matter the value of
        #  repr=.

        @dataclass
        class C:
            x: int
            def __repr__(self):
                return 'x'
        self.assertEqual(repr(C(0)), 'x')

        @dataclass(repr=True)
        class C:
            x: int
            def __repr__(self):
                return 'x'
        self.assertEqual(repr(C(0)), 'x')

        @dataclass(repr=False)
        class C:
            x: int
            def __repr__(self):
                return 'x'
        self.assertEqual(repr(C(0)), 'x')


class TestFrozen(unittest.TestCase):
    def test_overwriting_frozen(self):
        # frozen uses __setattr__ and __delattr__.
        with self.assertRaisesRegex(TypeError,
                                    'Cannot overwrite attribute __setattr__'):
            @dataclass(frozen=True)
            class C:
                x: int
                def __setattr__(self):
                    pass

        with self.assertRaisesRegex(TypeError,
                                    'Cannot overwrite attribute __delattr__'):
            @dataclass(frozen=True)
            class C:
                x: int
                def __delattr__(self):
                    pass

        @dataclass(frozen=False)
        class C:
            x: int
            def __setattr__(self, name, value):
                self.__dict__['x'] = value * 2
        self.assertEqual(C(10).x, 20)


class TestEq(unittest.TestCase):
    def test_no_eq(self):
        # Test a class with no __eq__ and eq=False.
        @dataclass(eq=False)
        class C:
            x: int
        self.assertNotEqual(C(0), C(0))
        c = C(3)
        self.assertEqual(c, c)

        # Test a class with an __eq__ and eq=False.
        @dataclass(eq=False)
        class C:
            x: int
            def __eq__(self, other):
                return other == 10
        self.assertEqual(C(3), 10)

    def test_overwriting_eq(self):
        # If the class has __eq__, use it no matter the value of
        #  eq=.

        @dataclass
        class C:
            x: int
            def __eq__(self, other):
                return other == 3
        self.assertEqual(C(1), 3)
        self.assertNotEqual(C(1), 1)

        @dataclass(eq=True)
        class C:
            x: int
            def __eq__(self, other):
                return other == 4
        self.assertEqual(C(1), 4)
        self.assertNotEqual(C(1), 1)

        @dataclass(eq=False)
        class C:
            x: int
            def __eq__(self, other):
                return other == 5
        self.assertEqual(C(1), 5)
        self.assertNotEqual(C(1), 1)


class TestOrdering(unittest.TestCase):
    def test_functools_total_ordering(self):
        # Test that functools.total_ordering works with this class.
        @total_ordering
        @dataclass
        class C:
            x: int
            def __lt__(self, other):
                # Perform the test "backward", just to make
                #  sure this is being called.
                return self.x >= other

        self.assertLess(C(0), -1)
        self.assertLessEqual(C(0), -1)
        self.assertGreater(C(0), 1)
        self.assertGreaterEqual(C(0), 1)

    def test_no_order(self):
        # Test that no ordering functions are added by default.
        @dataclass(order=False)
        class C:
            x: int
        # Make sure no order methods are added.
        self.assertNotIn('__le__', C.__dict__)
        self.assertNotIn('__lt__', C.__dict__)
        self.assertNotIn('__ge__', C.__dict__)
        self.assertNotIn('__gt__', C.__dict__)

        # Test that __lt__ is still called
        @dataclass(order=False)
        class C:
            x: int
            def __lt__(self, other):
                return False
        # Make sure other methods aren't added.
        self.assertNotIn('__le__', C.__dict__)
        self.assertNotIn('__ge__', C.__dict__)
        self.assertNotIn('__gt__', C.__dict__)

    def test_overwriting_order(self):
        with self.assertRaisesRegex(TypeError,
                                    'Cannot overwrite attribute __lt__'
                                    '.*using functools.total_ordering'):
            @dataclass(order=True)
            class C:
                x: int
                def __lt__(self):
                    pass

        with self.assertRaisesRegex(TypeError,
                                    'Cannot overwrite attribute __le__'
                                    '.*using functools.total_ordering'):
            @dataclass(order=True)
            class C:
                x: int
                def __le__(self):
                    pass

        with self.assertRaisesRegex(TypeError,
                                    'Cannot overwrite attribute __gt__'
                                    '.*using functools.total_ordering'):
            @dataclass(order=True)
            class C:
                x: int
                def __gt__(self):
                    pass

        with self.assertRaisesRegex(TypeError,
                                    'Cannot overwrite attribute __ge__'
                                    '.*using functools.total_ordering'):
            @dataclass(order=True)
            class C:
                x: int
                def __ge__(self):
                    pass

class TestHash(unittest.TestCase):
    def test_unsafe_hash(self):
        @dataclass(unsafe_hash=True)
        class C:
            x: int
            y: str
        self.assertEqual(hash(C(1, 'foo')), hash((1, 'foo')))

    def test_hash_rules(self):
        def non_bool(value):
            # Map to something else that's True, but not a bool.
            if value is None:
                return None
            if value:
                return (3,)
            return 0

        def test(case, unsafe_hash, eq, frozen, with_hash, result):
            with self.subTest(case=case, unsafe_hash=unsafe_hash, eq=eq,
                              frozen=frozen):
                if result != 'exception':
                    if with_hash:
                        @dataclass(unsafe_hash=unsafe_hash, eq=eq, frozen=frozen)
                        class C:
                            def __hash__(self):
                                return 0
                    else:
                        @dataclass(unsafe_hash=unsafe_hash, eq=eq, frozen=frozen)
                        class C:
                            pass

                # See if the result matches what's expected.
                if result == 'fn':
                    # __hash__ contains the function we generated.
                    self.assertIn('__hash__', C.__dict__)
                    self.assertIsNotNone(C.__dict__['__hash__'])

                elif result == '':
                    # __hash__ is not present in our class.
                    if not with_hash:
                        self.assertNotIn('__hash__', C.__dict__)

                elif result == 'none':
                    # __hash__ is set to None.
                    self.assertIn('__hash__', C.__dict__)
                    self.assertIsNone(C.__dict__['__hash__'])

                elif result == 'exception':
                    # Creating the class should cause an exception.
                    #  This only happens with with_hash==True.
                    assert(with_hash)
                    with self.assertRaisesRegex(TypeError, 'Cannot overwrite attribute __hash__'):
                        @dataclass(unsafe_hash=unsafe_hash, eq=eq, frozen=frozen)
                        class C:
                            def __hash__(self):
                                return 0

                else:
                    assert False, f'unknown result {result!r}'

        # There are 8 cases of:
        #  unsafe_hash=True/False
        #  eq=True/False
        #  frozen=True/False
        # And for each of these, a different result if
        #  __hash__ is defined or not.
        for case, (unsafe_hash,  eq,    frozen, res_no_defined_hash, res_defined_hash) in enumerate([
                  (False,        False, False,  '',                  ''),
                  (False,        False, True,   '',                  ''),
                  (False,        True,  False,  'none',              ''),
                  (False,        True,  True,   'fn',                ''),
                  (True,         False, False,  'fn',                'exception'),
                  (True,         False, True,   'fn',                'exception'),
                  (True,         True,  False,  'fn',                'exception'),
                  (True,         True,  True,   'fn',                'exception'),
                  ], 1):
            test(case, unsafe_hash, eq, frozen, False, res_no_defined_hash)
            test(case, unsafe_hash, eq, frozen, True,  res_defined_hash)

            # Test non-bool truth values, too.  This is just to
            #  make sure the data-driven table in the decorator
            #  handles non-bool values.
            test(case, non_bool(unsafe_hash), non_bool(eq), non_bool(frozen), False, res_no_defined_hash)
            test(case, non_bool(unsafe_hash), non_bool(eq), non_bool(frozen), True,  res_defined_hash)


    def test_eq_only(self):
        # If a class defines __eq__, __hash__ is automatically added
        #  and set to None.  This is normal Python behavior, not
        #  related to dataclasses.  Make sure we don't interfere with
        #  that (see bpo=32546).

        @dataclass
        class C:
            i: int
            def __eq__(self, other):
                return self.i == other.i
        self.assertEqual(C(1), C(1))
        self.assertNotEqual(C(1), C(4))

        # And make sure things work in this case if we specify
        #  unsafe_hash=True.
        @dataclass(unsafe_hash=True)
        class C:
            i: int
            def __eq__(self, other):
                return self.i == other.i
        self.assertEqual(C(1), C(1.0))
        self.assertEqual(hash(C(1)), hash(C(1.0)))

        # And check that the classes __eq__ is being used, despite
        #  specifying eq=True.
        @dataclass(unsafe_hash=True, eq=True)
        class C:
            i: int
            def __eq__(self, other):
                return self.i == 3 and self.i == other.i
        self.assertEqual(C(3), C(3))
        self.assertNotEqual(C(1), C(1))
        self.assertEqual(hash(C(1)), hash(C(1.0)))

    def test_0_field_hash(self):
        @dataclass(frozen=True)
        class C:
            pass
        self.assertEqual(hash(C()), hash(()))

        @dataclass(unsafe_hash=True)
        class C:
            pass
        self.assertEqual(hash(C()), hash(()))

    def test_1_field_hash(self):
        @dataclass(frozen=True)
        class C:
            x: int
        self.assertEqual(hash(C(4)), hash((4,)))
        self.assertEqual(hash(C(42)), hash((42,)))

        @dataclass(unsafe_hash=True)
        class C:
            x: int
        self.assertEqual(hash(C(4)), hash((4,)))
        self.assertEqual(hash(C(42)), hash((42,)))

    def test_hash_no_args(self):
        # Test dataclasses with no hash= argument.  This exists to
        #  make sure that if the @dataclass parameter name is changed
        #  or the non-default hashing behavior changes, the default
        #  hashability keeps working the same way.

        class Base:
            def __hash__(self):
                return 301

        # If frozen or eq is None, then use the default value (do not
        #  specify any value in the decorator).
        for frozen, eq,    base,   expected       in [
            (None,  None,  object, 'unhashable'),
            (None,  None,  Base,   'unhashable'),
            (None,  False, object, 'object'),
            (None,  False, Base,   'base'),
            (None,  True,  object, 'unhashable'),
            (None,  True,  Base,   'unhashable'),
            (False, None,  object, 'unhashable'),
            (False, None,  Base,   'unhashable'),
            (False, False, object, 'object'),
            (False, False, Base,   'base'),
            (False, True,  object, 'unhashable'),
            (False, True,  Base,   'unhashable'),
            (True,  None,  object, 'tuple'),
            (True,  None,  Base,   'tuple'),
            (True,  False, object, 'object'),
            (True,  False, Base,   'base'),
            (True,  True,  object, 'tuple'),
            (True,  True,  Base,   'tuple'),
            ]:

            with self.subTest(frozen=frozen, eq=eq, base=base, expected=expected):
                # First, create the class.
                if frozen is None and eq is None:
                    @dataclass
                    class C(base):
                        i: int
                elif frozen is None:
                    @dataclass(eq=eq)
                    class C(base):
                        i: int
                elif eq is None:
                    @dataclass(frozen=frozen)
                    class C(base):
                        i: int
                else:
                    @dataclass(frozen=frozen, eq=eq)
                    class C(base):
                        i: int

                # Now, make sure it hashes as expected.
                if expected == 'unhashable':
                    c = C(10)
                    with self.assertRaisesRegex(TypeError, 'unhashable type'):
                        hash(c)

                elif expected == 'base':
                    self.assertEqual(hash(C(10)), 301)

                elif expected == 'object':
                    # I'm not sure what test to use here.  object's
                    #  hash isn't based on id(), so calling hash()
                    #  won't tell us much.  So, just check the
                    #  function used is object's.
                    self.assertIs(C.__hash__, object.__hash__)

                elif expected == 'tuple':
                    self.assertEqual(hash(C(42)), hash((42,)))

                else:
                    assert False, f'unknown value for expected={expected!r}'


class TestFrozen(unittest.TestCase):
    def test_frozen(self):
        @dataclass(frozen=True)
        class C:
            i: int

        c = C(10)
        self.assertEqual(c.i, 10)
        with self.assertRaises(FrozenInstanceError):
            c.i = 5
        self.assertEqual(c.i, 10)

    def test_inherit(self):
        @dataclass(frozen=True)
        class C:
            i: int

        @dataclass(frozen=True)
        class D(C):
            j: int

        d = D(0, 10)
        with self.assertRaises(FrozenInstanceError):
            d.i = 5
        with self.assertRaises(FrozenInstanceError):
            d.j = 6
        self.assertEqual(d.i, 0)
        self.assertEqual(d.j, 10)

    # Test both ways: with an intermediate normal (non-dataclass)
    #  class and without an intermediate class.
    def test_inherit_nonfrozen_from_frozen(self):
        for intermediate_class in [True, False]:
            with self.subTest(intermediate_class=intermediate_class):
                @dataclass(frozen=True)
                class C:
                    i: int

                if intermediate_class:
                    class I(C): pass
                else:
                    I = C

                with self.assertRaisesRegex(TypeError,
                                            'cannot inherit non-frozen dataclass from a frozen one'):
                    @dataclass
                    class D(I):
                        pass

    def test_inherit_frozen_from_nonfrozen(self):
        for intermediate_class in [True, False]:
            with self.subTest(intermediate_class=intermediate_class):
                @dataclass
                class C:
                    i: int

                if intermediate_class:
                    class I(C): pass
                else:
                    I = C

                with self.assertRaisesRegex(TypeError,
                                            'cannot inherit frozen dataclass from a non-frozen one'):
                    @dataclass(frozen=True)
                    class D(I):
                        pass

    def test_inherit_from_normal_class(self):
        for intermediate_class in [True, False]:
            with self.subTest(intermediate_class=intermediate_class):
                class C:
                    pass

                if intermediate_class:
                    class I(C): pass
                else:
                    I = C

                @dataclass(frozen=True)
                class D(I):
                    i: int

            d = D(10)
            with self.assertRaises(FrozenInstanceError):
                d.i = 5

    def test_non_frozen_normal_derived(self):
        # See bpo-32953.

        @dataclass(frozen=True)
        class D:
            x: int
            y: int = 10

        class S(D):
            pass

        s = S(3)
        self.assertEqual(s.x, 3)
        self.assertEqual(s.y, 10)
        s.cached = True

        # But can't change the frozen attributes.
        with self.assertRaises(FrozenInstanceError):
            s.x = 5
        with self.assertRaises(FrozenInstanceError):
            s.y = 5
        self.assertEqual(s.x, 3)
        self.assertEqual(s.y, 10)
        self.assertEqual(s.cached, True)


class TestSlots(unittest.TestCase):
    def test_simple(self):
        @dataclass
        class C:
            __slots__ = ('x',)
            x: Any

        # There was a bug where a variable in a slot was assumed to
        #  also have a default value (of type
        #  types.MemberDescriptorType).
        with self.assertRaisesRegex(TypeError,
                                    r"__init__\(\) missing 1 required positional argument: 'x'"):
            C()

        # We can create an instance, and assign to x.
        c = C(10)
        self.assertEqual(c.x, 10)
        c.x = 5
        self.assertEqual(c.x, 5)

        # We can't assign to anything else.
        with self.assertRaisesRegex(AttributeError, "'C' object has no attribute 'y'"):
            c.y = 5

    def test_derived_added_field(self):
        # See bpo-33100.
        @dataclass
        class Base:
            __slots__ = ('x',)
            x: Any

        @dataclass
        class Derived(Base):
            x: int
            y: int

        d = Derived(1, 2)
        self.assertEqual((d.x, d.y), (1, 2))

        # We can add a new field to the derived instance.
        d.z = 10

class TestDescriptors(unittest.TestCase):
    def test_set_name(self):
        # See bpo-33141.

        # Create a descriptor.
        class D:
            def __set_name__(self, owner, name):
                self.name = name
            def __get__(self, instance, owner):
                if instance is not None:
                    return 1
                return self

        # This is the case of just normal descriptor behavior, no
        #  dataclass code is involved in initializing the descriptor.
        @dataclass
        class C:
            c: int=D()
        self.assertEqual(C.c.name, 'c')

        # Now test with a default value and init=False, which is the
        #  only time this is really meaningful.  If not using
        #  init=False, then the descriptor will be overwritten, anyway.
        @dataclass
        class C:
            c: int=field(default=D(), init=False)
        self.assertEqual(C.c.name, 'c')
        self.assertEqual(C().c, 1)

    def test_non_descriptor(self):
        # PEP 487 says __set_name__ should work on non-descriptors.
        # Create a descriptor.

        class D:
            def __set_name__(self, owner, name):
                self.name = name

        @dataclass
        class C:
            c: int=field(default=D(), init=False)
        self.assertEqual(C.c.name, 'c')


if __name__ == '__main__':
    unittest.main()
