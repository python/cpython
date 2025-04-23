import unittest
from string.templatelib import Interpolation


class TStringBaseCase:
    def assertTStringEqual(self, t, strings, interpolations):
        """Test template string literal equality.

        The *strings* argument must be a tuple of strings equal to *t.strings*.

        The *interpolations* argument must be a sequence of tuples which are
        compared against *t.interpolations*. Each tuple consists of
        (value, expression, conversion, format_spec), though the final two
        items may be omitted, and are assumed to be None and '' respectively.
        """
        self.assertEqual(t.strings, strings)
        self.assertEqual(len(t.interpolations), len(interpolations))

        for i, exp in zip(t.interpolations, interpolations, strict=True):
            if len(exp) == 4:
                actual = (i.value, i.expression, i.conversion, i.format_spec)
                self.assertEqual(actual, exp)
                continue

            if len(exp) == 3:
                self.assertEqual((i.value, i.expression, i.conversion), exp)
                self.assertEqual(i.format_spec, '')
                continue

            self.assertEqual((i.value, i.expression), exp)
            self.assertEqual(i.format_spec, '')
            self.assertIsNone(i.conversion)


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
