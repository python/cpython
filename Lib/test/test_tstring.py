import ast
import unittest

from string.templatelib import Template, Interpolation


def convert(value, conversion):
    if conversion == "a":
        return ascii(value)
    elif conversion == "r":
        return repr(value)
    elif conversion == "s":
        return str(value)
    return value


def f(template):
    parts = []
    for item in template:
        match item:
            case str() as s:
                parts.append(s)
            case Interpolation(value, _, conversion, format_spec):
                value = convert(value, conversion)
                value = format(value, format_spec)
                parts.append(value)
    return "".join(parts)


class TestTString(unittest.TestCase):
    def assertAllRaise(self, exception_type, regex, error_strings):
        for s in error_strings:
            with self.subTest(s=s):
                with self.assertRaisesRegex(exception_type, regex):
                    eval(s)

    def test_template_basic_creation(self):
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

    def test_template_creation_interleaving(self):
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

    def test_string_representation(self):
        # Test __repr__
        t = t"Hello"
        self.assertEqual(repr(t), "Template(strings=('Hello',), interpolations=())")

        name = "Python"
        t = t"Hello, {name}"
        self.assertEqual(repr(t),
            "Template(strings=('Hello, ', ''), "
            "interpolations=(Interpolation('Python', 'name', None, ''),))"
        )

    def test_interpolation_basics(self):
        # Test basic interpolation
        name = "Python"
        t = t"Hello, {name}"
        self.assertEqual(t.strings, ("Hello, ", ""))
        self.assertEqual(t.interpolations[0].value, name)
        self.assertEqual(t.interpolations[0].expression, "name")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Hello, Python")

        # Multiple interpolations
        first = "Python"
        last = "Developer"
        t = t"{first} {last}"
        self.assertEqual(t.strings, ("", " ", ""))
        self.assertEqual(t.interpolations[0].value, first)
        self.assertEqual(t.interpolations[0].expression, "first")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(t.interpolations[1].value, last)
        self.assertEqual(t.interpolations[1].expression, "last")
        self.assertEqual(t.interpolations[1].conversion, None)
        self.assertEqual(t.interpolations[1].format_spec, "")
        self.assertEqual(f(t), "Python Developer")

        # Interpolation with expressions
        a = 10
        b = 20
        t = t"Sum: {a + b}"
        self.assertEqual(t.strings, ("Sum: ", ""))
        self.assertEqual(t.interpolations[0].value, a + b)
        self.assertEqual(t.interpolations[0].expression, "a + b")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Sum: 30")

        # Interpolation with function
        def square(x):
            return x * x
        t = t"Square: {square(5)}"
        self.assertEqual(t.strings, ("Square: ", ""))
        self.assertEqual(t.interpolations[0].value, square(5))
        self.assertEqual(t.interpolations[0].expression, "square(5)")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Square: 25")

        # Test attribute access in expressions
        class Person:
            def __init__(self, name):
                self.name = name

            def upper(self):
                return self.name.upper()

        person = Person("Alice")
        t = t"Name: {person.name}"
        self.assertEqual(t.strings, ("Name: ", ""))
        self.assertEqual(t.interpolations[0].value, person.name)
        self.assertEqual(t.interpolations[0].expression, "person.name")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Name: Alice")

        # Test method calls
        t = t"Name: {person.upper()}"
        self.assertEqual(t.strings, ("Name: ", ""))
        self.assertEqual(t.interpolations[0].value, person.upper())
        self.assertEqual(t.interpolations[0].expression, "person.upper()")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Name: ALICE")

        # Test dictionary access
        data = {"name": "Bob", "age": 30}
        t = t"Name: {data['name']}, Age: {data['age']}"
        self.assertEqual(t.strings, ("Name: ", ", Age: ", ""))
        self.assertEqual(t.interpolations[0].value, data["name"])
        self.assertEqual(t.interpolations[0].expression, "data['name']")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(t.interpolations[1].value, data["age"])
        self.assertEqual(t.interpolations[1].expression, "data['age']")
        self.assertEqual(t.interpolations[1].conversion, None)
        self.assertEqual(t.interpolations[1].format_spec, "")
        self.assertEqual(f(t), "Name: Bob, Age: 30")

    def test_format_specifiers(self):
        # Test basic format specifiers
        value = 3.14159
        t = t"Pi: {value:.2f}"
        self.assertEqual(t.strings, ("Pi: ", ""))
        self.assertEqual(t.interpolations[0].value, value)
        self.assertEqual(t.interpolations[0].expression, "value")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, ".2f")
        self.assertEqual(f(t), "Pi: 3.14")

    def test_conversions(self):
        # Test !s conversion (str)
        obj = object()
        t = t"Object: {obj!s}"
        self.assertEqual(t.strings, ("Object: ", ""))
        self.assertEqual(t.interpolations[0].value, obj)
        self.assertEqual(t.interpolations[0].expression, "obj")
        self.assertEqual(t.interpolations[0].conversion, "s")
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), f"Object: {str(obj)}")

        # Test !r conversion (repr)
        t = t"Data: {obj!r}"
        self.assertEqual(t.strings, ("Data: ", ""))
        self.assertEqual(t.interpolations[0].value, obj)
        self.assertEqual(t.interpolations[0].expression, "obj")
        self.assertEqual(t.interpolations[0].conversion, "r")
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), f"Data: {repr(obj)}")

        # Test !a conversion (ascii)
        text = "Café"
        t = t"ASCII: {text!a}"
        self.assertEqual(t.strings, ("ASCII: ", ""))
        self.assertEqual(t.interpolations[0].value, text)
        self.assertEqual(t.interpolations[0].expression, "text")
        self.assertEqual(t.interpolations[0].conversion, "a")
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), f"ASCII: {ascii(text)}")

        # Test !z conversion (error)
        num = 1
        with self.assertRaises(SyntaxError):
            eval("t'{num!z}'")

    def test_debug_specifier(self):
        # Test debug specifier
        value = 42
        t = t"Value: {value=}"
        self.assertEqual(t.strings, ("Value: value=", ""))
        self.assertEqual(t.interpolations[0].value, value)
        self.assertEqual(t.interpolations[0].expression, "value")
        self.assertEqual(t.interpolations[0].conversion, "r")
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Value: value=42")

        # Test debug specifier with format (conversion default to !r)
        t = t"Value: {value=:.2f}"
        self.assertEqual(t.strings, ("Value: value=", ""))
        self.assertEqual(t.interpolations[0].value, value)
        self.assertEqual(t.interpolations[0].expression, "value")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, ".2f")
        self.assertEqual(f(t), "Value: value=42.00")

        # Test debug specifier with conversion
        t = t"Value: {value=!s}"
        self.assertEqual(t.strings, ("Value: value=", ""))
        self.assertEqual(t.interpolations[0].value, value)
        self.assertEqual(t.interpolations[0].expression, "value")
        self.assertEqual(t.interpolations[0].conversion, "s")
        self.assertEqual(t.interpolations[0].format_spec, "")

        # Test white space in debug specifier
        t = t"Value: {value = }"
        self.assertEqual(t.strings, ("Value: value = ", ""))
        self.assertEqual(t.interpolations[0].value, value)
        self.assertEqual(t.interpolations[0].expression, "value")
        self.assertEqual(t.interpolations[0].conversion, "r")
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Value: value = 42")

    def test_raw_tstrings(self):
        path = r"C:\Users"
        t = rt"{path}\Documents"
        self.assertEqual(t.strings, ("", r"\Documents",))
        self.assertEqual(t.interpolations[0].value, path)
        self.assertEqual(t.interpolations[0].expression, "path")
        self.assertEqual(f(t), r"C:\Users\Documents")

        # Test alternative prefix
        t = tr"{path}\Documents"
        self.assertEqual(t.strings, ("", r"\Documents",))
        self.assertEqual(t.interpolations[0].value, path)


    def test_template_concatenation(self):
        # Test template + template
        t1 = t"Hello, "
        t2 = t"world"
        combined = t1 + t2
        self.assertEqual(combined.strings, ("Hello, world",))
        self.assertEqual(len(combined.interpolations), 0)
        self.assertEqual(f(combined), "Hello, world")

        # Test template + string
        t1 = t"Hello"
        combined = t1 + ", world"
        self.assertEqual(combined.strings, ("Hello, world",))
        self.assertEqual(len(combined.interpolations), 0)
        self.assertEqual(f(combined), "Hello, world")

        # Test template + template with interpolation
        name = "Python"
        t1 = t"Hello, "
        t2 = t"{name}"
        combined = t1 + t2
        self.assertEqual(combined.strings, ("Hello, ", ""))
        self.assertEqual(combined.interpolations[0].value, name)
        self.assertEqual(combined.interpolations[0].expression, "name")
        self.assertEqual(combined.interpolations[0].conversion, None)
        self.assertEqual(combined.interpolations[0].format_spec, "")
        self.assertEqual(f(combined), "Hello, Python")

        # Test string + template
        t = "Hello, " + t"{name}"
        self.assertEqual(t.strings, ("Hello, ", ""))
        self.assertEqual(t.interpolations[0].value, name)
        self.assertEqual(t.interpolations[0].expression, "name")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Hello, Python")

    def test_nested_templates(self):
        # Test a template inside another template expression
        name = "Python"
        inner = t"{name}"
        t = t"Language: {inner}"

        self.assertEqual(t.strings, ("Language: ", ""))
        self.assertEqual(t.interpolations[0].value.strings, ("", ""))
        self.assertEqual(t.interpolations[0].value.interpolations[0].value, name)
        self.assertEqual(t.interpolations[0].value.interpolations[0].expression, "name")
        self.assertEqual(t.interpolations[0].value.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].value.interpolations[0].format_spec, "")
        self.assertEqual(t.interpolations[0].expression, "inner")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")

    def test_ast_structure(self):
        # Test AST structure for simple t-string
        tree = ast.parse('t"Hello"')
        self.assertIsInstance(tree.body[0].value, ast.TemplateStr)
        self.assertIsInstance(tree.body[0].value.values[0], ast.Constant)

        # Test AST for t-string with interpolation
        tree = ast.parse('t"Hello {name}"')
        self.assertIsInstance(tree.body[0].value, ast.TemplateStr)
        self.assertIsInstance(tree.body[0].value.values[0], ast.Constant)
        self.assertIsInstance(tree.body[0].value.values[1], ast.Interpolation)

    def test_error_conditions(self):
        # Test syntax errors
        with self.assertRaisesRegex(SyntaxError, "'{' was never closed"):
            eval("t'{")

        with self.assertRaisesRegex(SyntaxError, "t-string: expecting '}'"):
            eval("t'{a'")

        with self.assertRaisesRegex(SyntaxError, "t-string: single '}' is not allowed"):
            eval("t'}'")

        # Test missing variables
        with self.assertRaises(NameError):
            eval("t'Hello, {name}'")

        # Test invalid conversion
        num = 1
        with self.assertRaises(SyntaxError):
            eval("t'{num!z}'")

    def test_literal_concatenation(self):
        # Test concatenation of t-string literals
        t = t"Hello, " t"world"
        self.assertEqual(t.strings, ("Hello, world",))
        self.assertEqual(len(t.interpolations), 0)
        self.assertEqual(f(t), "Hello, world")

        # Test concatenation with interpolation
        name = "Python"
        t = t"Hello, " t"{name}"
        self.assertEqual(t.strings, ("Hello, ", ""))
        self.assertEqual(t.interpolations[0].value, name)
        self.assertEqual(t.interpolations[0].expression, "name")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Hello, Python")

        # Test concatenation with string literal
        name = "Python"
        t = t"Hello, {name}" "and welcome!"
        self.assertEqual(t.strings, ("Hello, ", "and welcome!"))
        self.assertEqual(t.interpolations[0].value, name)
        self.assertEqual(t.interpolations[0].expression, "name")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "Hello, Pythonand welcome!")

    def test_triple_quoted(self):
        # Test triple-quoted t-strings
        t = t"""
        Hello,
        world
        """
        self.assertEqual(t.strings, ("\n        Hello,\n        world\n        ",))
        self.assertEqual(len(t.interpolations), 0)
        self.assertEqual(f(t), "\n        Hello,\n        world\n        ")

        # Test triple-quoted with interpolation
        name = "Python"
        t = t"""
        Hello,
        {name}
        """
        self.assertEqual(t.strings, ("\n        Hello,\n        ", "\n        "))
        self.assertEqual(t.interpolations[0].value, name)
        self.assertEqual(t.interpolations[0].expression, "name")
        self.assertEqual(t.interpolations[0].conversion, None)
        self.assertEqual(t.interpolations[0].format_spec, "")
        self.assertEqual(f(t), "\n        Hello,\n        Python\n        ")

if __name__ == '__main__':
    unittest.main()
