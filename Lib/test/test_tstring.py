import unittest

from test.test_string._support import TStringBaseCase, fstring


class TestTString(unittest.TestCase, TStringBaseCase):
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
        self.assertTStringEqual(t, ("Hello, ", ""), [(name, "name")])
        self.assertEqual(fstring(t), "Hello, Python")

        # Multiple interpolations
        first = "Python"
        last = "Developer"
        t = t"{first} {last}"
        self.assertTStringEqual(
            t, ("", " ", ""), [(first, 'first'), (last, 'last')]
        )
        self.assertEqual(fstring(t), "Python Developer")

        # Interpolation with expressions
        a = 10
        b = 20
        t = t"Sum: {a + b}"
        self.assertTStringEqual(t, ("Sum: ", ""), [(a + b, "a + b")])
        self.assertEqual(fstring(t), "Sum: 30")

        # Interpolation with function
        def square(x):
            return x * x
        t = t"Square: {square(5)}"
        self.assertTStringEqual(
            t, ("Square: ", ""), [(square(5), "square(5)")]
        )
        self.assertEqual(fstring(t), "Square: 25")

        # Test attribute access in expressions
        class Person:
            def __init__(self, name):
                self.name = name

            def upper(self):
                return self.name.upper()

        person = Person("Alice")
        t = t"Name: {person.name}"
        self.assertTStringEqual(
            t, ("Name: ", ""), [(person.name, "person.name")]
        )
        self.assertEqual(fstring(t), "Name: Alice")

        # Test method calls
        t = t"Name: {person.upper()}"
        self.assertTStringEqual(
            t, ("Name: ", ""), [(person.upper(), "person.upper()")]
        )
        self.assertEqual(fstring(t), "Name: ALICE")

        # Test dictionary access
        data = {"name": "Bob", "age": 30}
        t = t"Name: {data['name']}, Age: {data['age']}"
        self.assertTStringEqual(
            t, ("Name: ", ", Age: ", ""),
            [(data["name"], "data['name']"), (data["age"], "data['age']")],
        )
        self.assertEqual(fstring(t), "Name: Bob, Age: 30")

    def test_format_specifiers(self):
        # Test basic format specifiers
        value = 3.14159
        t = t"Pi: {value:.2f}"
        self.assertTStringEqual(
            t, ("Pi: ", ""), [(value, "value", None, ".2f")]
        )
        self.assertEqual(fstring(t), "Pi: 3.14")

    def test_conversions(self):
        # Test !s conversion (str)
        obj = object()
        t = t"Object: {obj!s}"
        self.assertTStringEqual(t, ("Object: ", ""), [(obj, "obj", "s")])
        self.assertEqual(fstring(t), f"Object: {str(obj)}")

        # Test !r conversion (repr)
        t = t"Data: {obj!r}"
        self.assertTStringEqual(t, ("Data: ", ""), [(obj, "obj", "r")])
        self.assertEqual(fstring(t), f"Data: {repr(obj)}")

        # Test !a conversion (ascii)
        text = "Café"
        t = t"ASCII: {text!a}"
        self.assertTStringEqual(t, ("ASCII: ", ""), [(text, "text", "a")])
        self.assertEqual(fstring(t), f"ASCII: {ascii(text)}")

        # Test !z conversion (error)
        num = 1
        with self.assertRaises(SyntaxError):
            eval("t'{num!z}'")

    def test_debug_specifier(self):
        # Test debug specifier
        value = 42
        t = t"Value: {value=}"
        self.assertTStringEqual(
            t, ("Value: value=", ""), [(value, "value", "r")]
        )
        self.assertEqual(fstring(t), "Value: value=42")

        # Test debug specifier with format (conversion default to !r)
        t = t"Value: {value=:.2f}"
        self.assertTStringEqual(
            t, ("Value: value=", ""), [(value, "value", None, ".2f")]
        )
        self.assertEqual(fstring(t), "Value: value=42.00")

        # Test debug specifier with conversion
        t = t"Value: {value=!s}"
        self.assertTStringEqual(
            t, ("Value: value=", ""), [(value, "value", "s")]
        )

        # Test white space in debug specifier
        t = t"Value: {value = }"
        self.assertTStringEqual(
            t, ("Value: value = ", ""), [(value, "value", "r")]
        )
        self.assertEqual(fstring(t), "Value: value = 42")

    def test_raw_tstrings(self):
        path = r"C:\Users"
        t = rt"{path}\Documents"
        self.assertTStringEqual(t, ("", r"\Documents"), [(path, "path")])
        self.assertEqual(fstring(t), r"C:\Users\Documents")

        # Test alternative prefix
        t = tr"{path}\Documents"
        self.assertTStringEqual(t, ("", r"\Documents"), [(path, "path")])

    def test_template_concatenation(self):
        # Test template + template
        t1 = t"Hello, "
        t2 = t"world"
        combined = t1 + t2
        self.assertTStringEqual(combined, ("Hello, world",), ())
        self.assertEqual(fstring(combined), "Hello, world")

        # Test template + string
        t1 = t"Hello"
        expected_msg = 'can only concatenate string.templatelib.Template ' \
            '\\(not "str"\\) to string.templatelib.Template'
        with self.assertRaisesRegex(TypeError, expected_msg):
            t1 + ", world"

        # Test template + template with interpolation
        name = "Python"
        t1 = t"Hello, "
        t2 = t"{name}"
        combined = t1 + t2
        self.assertTStringEqual(combined, ("Hello, ", ""), [(name, "name")])
        self.assertEqual(fstring(combined), "Hello, Python")

        # Test string + template
        expected_msg = 'can only concatenate str ' \
            '\\(not "string.templatelib.Template"\\) to str'
        with self.assertRaisesRegex(TypeError, expected_msg):
            "Hello, " + t"{name}"

    def test_nested_templates(self):
        # Test a template inside another template expression
        name = "Python"
        inner = t"{name}"
        t = t"Language: {inner}"

        t_interp = t.interpolations[0]
        self.assertEqual(t.strings, ("Language: ", ""))
        self.assertEqual(t_interp.value.strings, ("", ""))
        self.assertEqual(t_interp.value.interpolations[0].value, name)
        self.assertEqual(t_interp.value.interpolations[0].expression, "name")
        self.assertEqual(t_interp.value.interpolations[0].conversion, None)
        self.assertEqual(t_interp.value.interpolations[0].format_spec, "")
        self.assertEqual(t_interp.expression, "inner")
        self.assertEqual(t_interp.conversion, None)
        self.assertEqual(t_interp.format_spec, "")

    def test_syntax_errors(self):
        for case, err in (
            ("t'", "unterminated t-string literal"),
            ("t'''", "unterminated triple-quoted t-string literal"),
            ("t''''", "unterminated triple-quoted t-string literal"),
            ("t'{", "'{' was never closed"),
            ("t'{'", "t-string: expecting '}'"),
            ("t'{a'", "t-string: expecting '}'"),
            ("t'}'", "t-string: single '}' is not allowed"),
            ("t'{}'", "t-string: valid expression required before '}'"),
            ("t'{=x}'", "t-string: valid expression required before '='"),
            ("t'{!x}'", "t-string: valid expression required before '!'"),
            ("t'{:x}'", "t-string: valid expression required before ':'"),
            ("t'{x;y}'", "t-string: expecting '=', or '!', or ':', or '}'"),
            ("t'{x=y}'", "t-string: expecting '!', or ':', or '}'"),
            ("t'{x!s!}'", "t-string: expecting ':' or '}'"),
            ("t'{x!s:'", "t-string: expecting '}', or format specs"),
            ("t'{x!}'", "t-string: missing conversion character"),
            ("t'{x=!}'", "t-string: missing conversion character"),
            ("t'{x!z}'", "t-string: invalid conversion character 'z': "
                         "expected 's', 'r', or 'a'"),
            ("t'{lambda:1}'", "t-string: lambda expressions are not allowed "
                              "without parentheses"),
            ("t'{x:{;}}'", "t-string: expecting a valid expression after '{'"),
            ("t'{1:d\n}'", "t-string: newlines are not allowed in format specifiers")
        ):
            with self.subTest(case), self.assertRaisesRegex(SyntaxError, err):
                eval(case)

    def test_runtime_errors(self):
        # Test missing variables
        with self.assertRaises(NameError):
            eval("t'Hello, {name}'")

    def test_literal_concatenation(self):
        # Test concatenation of t-string literals
        t = t"Hello, " t"world"
        self.assertTStringEqual(t, ("Hello, world",), ())
        self.assertEqual(fstring(t), "Hello, world")

        # Test concatenation with interpolation
        name = "Python"
        t = t"Hello, " t"{name}"
        self.assertTStringEqual(t, ("Hello, ", ""), [(name, "name")])
        self.assertEqual(fstring(t), "Hello, Python")

        # Test disallowed mix of t-string and string/f-string (incl. bytes)
        what = 't'
        expected_msg = 'cannot mix t-string literals with string or bytes literals'
        for case in (
            "t'{what}-string literal' 'str literal'",
            "t'{what}-string literal' u'unicode literal'",
            "t'{what}-string literal' f'f-string literal'",
            "t'{what}-string literal' r'raw string literal'",
            "t'{what}-string literal' rf'raw f-string literal'",
            "t'{what}-string literal' b'bytes literal'",
            "t'{what}-string literal' br'raw bytes literal'",
            "'str literal' t'{what}-string literal'",
            "u'unicode literal' t'{what}-string literal'",
            "f'f-string literal' t'{what}-string literal'",
            "r'raw string literal' t'{what}-string literal'",
            "rf'raw f-string literal' t'{what}-string literal'",
            "b'bytes literal' t'{what}-string literal'",
            "br'raw bytes literal' t'{what}-string literal'",
        ):
            with self.subTest(case):
                with self.assertRaisesRegex(SyntaxError, expected_msg):
                    eval(case)

    def test_triple_quoted(self):
        # Test triple-quoted t-strings
        t = t"""
        Hello,
        world
        """
        self.assertTStringEqual(
            t, ("\n        Hello,\n        world\n        ",), ()
        )
        self.assertEqual(fstring(t), "\n        Hello,\n        world\n        ")

        # Test triple-quoted with interpolation
        name = "Python"
        t = t"""
        Hello,
        {name}
        """
        self.assertTStringEqual(
            t, ("\n        Hello,\n        ", "\n        "), [(name, "name")]
        )
        self.assertEqual(fstring(t), "\n        Hello,\n        Python\n        ")

if __name__ == '__main__':
    unittest.main()
