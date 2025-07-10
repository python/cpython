import unittest
from string.templatelib import Interpolation


class TStringBaseCase:
    def assertInterpolationEqual(self, i, exp):
        """Test Interpolation equality.

        The *i* argument must be an Interpolation instance.

        The *exp* argument must be a tuple of the form
        (value, expression, conversion, format_spec) where the final three
        items may be omitted and are assumed to be '', None and '' respectively.
        """
        if len(exp) == 4:
            actual = (i.value, i.expression, i.conversion, i.format_spec)
            self.assertEqual(actual, exp)
        elif len(exp) == 3:
            self.assertEqual((i.value, i.expression, i.conversion), exp)
            self.assertEqual(i.format_spec, "")
        elif len(exp) == 2:
            self.assertEqual((i.value, i.expression), exp)
            self.assertEqual(i.conversion, None)
            self.assertEqual(i.format_spec, "")
        elif len(exp) == 1:
            self.assertEqual((i.value,), exp)
            self.assertEqual(i.expression, "")
            self.assertEqual(i.conversion, None)
            self.assertEqual(i.format_spec, "")

    def assertTStringEqual(self, t, strings, interpolations):
        """Test template string literal equality.

        The *strings* argument must be a tuple of strings equal to *t.strings*.

        The *interpolations* argument must be a sequence of tuples which are
        compared against *t.interpolations*. Each tuple must match the form
        described in the `assertInterpolationEqual` method.
        """
        self.assertEqual(t.strings, strings)
        self.assertEqual(len(t.interpolations), len(interpolations))

        for i, exp in zip(t.interpolations, interpolations, strict=True):
            self.assertInterpolationEqual(i, exp)


def convert(value, conversion):
    if conversion == "a":
        return ascii(value)
    elif conversion == "r":
        return repr(value)
    elif conversion == "s":
        return str(value)
    return value


def fstring(template):
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
