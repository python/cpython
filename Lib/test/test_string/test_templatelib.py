import unittest
from string.templatelib import Template, Interpolation

from test.test_string._support import f


class TestTemplate(unittest.TestCase):

    def test_basic_creation(self):
        # Simple t-string creation
        t = t"Hello, world"
        self.assertTrue(isinstance(t, Template))
        self.assertEqual(t.strings, ("Hello, world",))
        self.assertEqual(len(t.interpolations), 0)
        self.assertEqual(f(t), "Hello, world")

        # Empty t-string
        t = t""
        self.assertEqual(t.strings, ("",))
        self.assertEqual(len(t.interpolations), 0)
        self.assertEqual(f(t), "")

        # Multi-line t-string
        t = t"""Hello,
world"""
        self.assertEqual(t.strings, ("Hello,\nworld",))
        self.assertEqual(len(t.interpolations), 0)
        self.assertEqual(f(t), "Hello,\nworld")

    def test_creation_interleaving(self):
        # Should add strings on either side
        t = Template(Interpolation("Maria", "name", None, ""))
        self.assertEqual(t.strings, ("", ""))
        self.assertEqual(t.interpolations[0].value, "Maria")
        self.assertEqual(t.interpolations[0].expression, "name")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Maria")

        # Should prepend empty string
        t = Template(Interpolation("Maria", "name", None, ""), " is my name")
        self.assertEqual(t.strings, ("", " is my name"))
        self.assertEqual(t.interpolations[0].value, "Maria")
        self.assertEqual(t.interpolations[0].expression, "name")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Maria is my name")

        # Should append empty string
        t = Template("Hello, ", Interpolation("Maria", "name", None, ""))
        self.assertEqual(t.strings, ("Hello, ", ""))
        self.assertEqual(t.interpolations[0].value, "Maria")
        self.assertEqual(t.interpolations[0].expression, "name")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Hello, Maria")

        # Should concatenate strings
        t = Template("Hello", ", ", Interpolation("Maria", "name", None, ""), "!")
        self.assertEqual(t.strings, ("Hello, ", "!"))
        self.assertEqual(t.interpolations[0].value, "Maria")
        self.assertEqual(t.interpolations[0].expression, "name")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Hello, Maria!")

        # Should add strings on either side and in between
        t = Template(Interpolation("Maria", "name", None, ""), Interpolation("Python", "language", None, ""))
        self.assertEqual(t.strings, ("", "", ""))
        self.assertEqual(t.interpolations[0].value, "Maria")
        self.assertEqual(t.interpolations[0].expression, "name")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(t.interpolations[1].value, "Python")
        self.assertEqual(t.interpolations[1].expression, "language")
        self.assertEqual(t.interpolations[1].conversion, None)
        self.assertEqual(t.interpolations[1].format_spec, "")
        self.assertEqual(f(t), "MariaPython")
