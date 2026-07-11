"""Tests for gh-148941: dataclass skips method generation when the class
already defines the method in its own __dict__, matching the design tables
documented at the top of dataclasses.py where the intersection of
'parameter=True' and 'class has method in __dict__' has a blank cell
(= no action / skip generation) for __init__, __repr__, and __eq__.

Methods with 'raise' cells (order, frozen) are intentionally NOT skipped
early — their existing TypeError-on-overwrite behavior is correct and
preserved.
"""

import unittest
from dataclasses import KW_ONLY, dataclass, field


class TestSkipMethodsWhenAlreadyDefined(unittest.TestCase):
    """Test that @dataclass skips generating __init__, __repr__, and __eq__
    when the class already defines them in its own __dict__."""

    # ---- __init__ (the reported bug: gh-148941) ----

    def test_init_skipped_when_already_defined(self):
        # Inherited fields with defaults followed by child fields without
        # used to trigger TypeError during _init_fn field-ordering validation,
        # even though the class's own __init__ would be preserved by
        # _set_new_attribute.

        @dataclass
        class Base:
            x: int = 0

        @dataclass
        class Child(Base):
            y: int
            def __init__(self, y):
                self.y = y

        obj = Child(5)
        self.assertEqual(obj.y, 5)

    def test_init_skipped_with_default_factory(self):
        @dataclass
        class Base:
            items: list = field(default_factory=list)

        @dataclass
        class Child(Base):
            name: str
            def __init__(self, name):
                self.name = name

        obj = Child("test")
        self.assertEqual(obj.name, "test")

    def test_init_skipped_with_kw_only(self):
        @dataclass
        class Base:
            a: int = 1
            _: KW_ONLY
            b: int = 2

        @dataclass
        class Child(Base):
            c: int
            def __init__(self, c):
                self.c = c

        obj = Child(5)
        self.assertEqual(obj.c, 5)

    def test_init_skipped_in_three_level_chain(self):
        @dataclass
        class A:
            a: int = 1

        @dataclass
        class B(A):
            b: int = 2

        @dataclass
        class C(B):
            c: int
            def __init__(self, c):
                self.c = c

        obj = C(5)
        self.assertEqual(obj.c, 5)

    def test_init_not_skipped_when_only_base_defines_it(self):
        # __init__ defined in a base class (not in cls.__dict__) should NOT
        # prevent generation in the child.

        @dataclass
        class Base:
            x: int = 0
            def __init__(self, x):
                self.x = x

        @dataclass
        class Child(Base):
            y: int = 1

        obj = Child(y=2)
        self.assertEqual(obj.y, 2)

    def test_init_still_generated_when_not_defined(self):
        @dataclass
        class Normal:
            x: int = 1
            y: int = 2

        obj = Normal()
        self.assertEqual(obj.x, 1)
        self.assertEqual(obj.y, 2)

        obj2 = Normal(10, 20)
        self.assertEqual(obj2.x, 10)
        self.assertEqual(obj2.y, 20)

    # ---- __repr__ ----

    def test_repr_skipped_when_already_defined(self):
        @dataclass
        class WithRepr:
            x: int
            def __repr__(self):
                return f"Custom({self.x})"

        obj = WithRepr(42)
        self.assertEqual(repr(obj), "Custom(42)")

    def test_repr_still_generated_when_not_defined(self):
        @dataclass
        class Normal:
            x: int = 5
        obj = Normal()
        self.assertIn("Normal(x=5)", repr(obj))

    # ---- __eq__ ----

    def test_eq_skipped_when_already_defined(self):
        @dataclass
        class WithEq:
            x: int
            def __eq__(self, other):
                return True  # all equal

        a = WithEq(1)
        b = WithEq(2)
        self.assertEqual(a, b)

    def test_eq_still_generated_when_not_defined(self):
        @dataclass
        class Normal:
            x: int
        a = Normal(1)
        b = Normal(1)
        c = Normal(2)
        self.assertEqual(a, b)
        self.assertNotEqual(a, c)

    # ---- order and frozen: deliberately NOT skipped (raise is correct) ----

    def test_order_still_raises_when_defined(self):
        # Design table says 'raise' for order=True + has comparison method.
        # The early guard does NOT apply here.
        with self.assertRaises(TypeError):
            @dataclass(order=True)
            class BadOrder:
                x: int
                def __lt__(self, other):
                    return True

    def test_frozen_still_raises_when_setattr_defined(self):
        # Design table says 'raise' for frozen=True + has __setattr__/__delattr__.
        with self.assertRaises(TypeError):
            @dataclass(frozen=True)
            class BadFrozen:
                x: int = 1
                def __setattr__(self, name, value):
                    object.__setattr__(self, name, value)


if __name__ == '__main__':
    unittest.main()
