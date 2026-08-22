import unittest
from dataclasses import dataclass

class TestDataclassInheritanceFieldOrder(unittest.TestCase):
    def test_required_after_optional_in_subclass(self):
        @dataclass
        class Base:
            x: int = 10  # optional field with default

        @dataclass
        class Sub(Base):
            y: int  # required field in subclass

        # Should require y but allow optional x to be omitted
        obj = Sub(y=5)
        self.assertEqual(obj.x, 10)
        self.assertEqual(obj.y, 5)

        # Should allow overriding x
        obj2 = Sub(x=42, y=9)
        self.assertEqual(obj2.x, 42)
        self.assertEqual(obj2.y, 9)

        # Missing y should raise TypeError
        with self.assertRaises(TypeError):
            Sub()
