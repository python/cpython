import pickle
import unittest
from collections.abc import Iterator, Iterable
from string.templatelib import Template, Interpolation

from test.test_string._support import TStringBaseCase, fstring


class TestTemplate(unittest.TestCase, TStringBaseCase):

    def test_common(self):
        self.assertEqual(type(t'').__name__, 'Template')
        self.assertEqual(type(t'').__qualname__, 'Template')
        self.assertEqual(type(t'').__module__, 'string.templatelib')

        a = 'a'
        i = t'{a}'.interpolations[0]
        self.assertEqual(type(i).__name__, 'Interpolation')
        self.assertEqual(type(i).__qualname__, 'Interpolation')
        self.assertEqual(type(i).__module__, 'string.templatelib')

    def test_final_types(self):
        with self.assertRaisesRegex(TypeError, 'is not an acceptable base type'):
            class Sub(Template): ...

        with self.assertRaisesRegex(TypeError, 'is not an acceptable base type'):
            class Sub(Interpolation): ...

    def test_basic_creation(self):
        # Simple t-string creation
        t = t'Hello, world'
        self.assertIsInstance(t, Template)
        self.assertTStringEqual(t, ('Hello, world',), ())
        self.assertEqual(fstring(t), 'Hello, world')

        # Empty t-string
        t = t''
        self.assertTStringEqual(t, ('',), ())
        self.assertEqual(fstring(t), '')

        # Multi-line t-string
        t = t"""Hello,
world"""
        self.assertEqual(t.strings, ('Hello,\nworld',))
        self.assertEqual(len(t.interpolations), 0)
        self.assertEqual(fstring(t), 'Hello,\nworld')

    def test_creation_interleaving(self):
        # Should add strings on either side
        t = Template(Interpolation('Maria', 'name', None, ''))
        self.assertTStringEqual(t, ('', ''), [('Maria', 'name')])
        self.assertEqual(fstring(t), 'Maria')

        # Should prepend empty string
        t = Template(Interpolation('Maria', 'name', None, ''), ' is my name')
        self.assertTStringEqual(t, ('', ' is my name'), [('Maria', 'name')])
        self.assertEqual(fstring(t), 'Maria is my name')

        # Should append empty string
        t = Template('Hello, ', Interpolation('Maria', 'name', None, ''))
        self.assertTStringEqual(t, ('Hello, ', ''), [('Maria', 'name')])
        self.assertEqual(fstring(t), 'Hello, Maria')

        # Should concatenate strings
        t = Template('Hello', ', ', Interpolation('Maria', 'name', None, ''),
                     '!')
        self.assertTStringEqual(t, ('Hello, ', '!'), [('Maria', 'name')])
        self.assertEqual(fstring(t), 'Hello, Maria!')

        # Should add strings on either side and in between
        t = Template(Interpolation('Maria', 'name', None, ''),
                     Interpolation('Python', 'language', None, ''))
        self.assertTStringEqual(
            t, ('', '', ''), [('Maria', 'name'), ('Python', 'language')]
        )
        self.assertEqual(fstring(t), 'MariaPython')

    def test_template_values(self):
        t = t'Hello, world'
        self.assertEqual(t.values, ())

        name = "Lys"
        t = t'Hello, {name}'
        self.assertEqual(t.values, ("Lys",))

        country = "GR"
        age = 0
        t = t'Hello, {name}, {age} from {country}'
        self.assertEqual(t.values, ("Lys", 0, "GR"))

    def test_pickle_template(self):
        user = 'test'
        for template in (
            t'',
            t"No values",
            t'With inter {user}',
            t'With ! {user!r}',
            t'With format {1 / 0.3:.2f}',
            Template(),
            Template('a'),
            Template(Interpolation('Nikita', 'name', None, '')),
            Template('a', Interpolation('Nikita', 'name', 'r', '')),
        ):
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                with self.subTest(proto=proto, template=template):
                    pickled = pickle.dumps(template, protocol=proto)
                    unpickled = pickle.loads(pickled)

                    self.assertEqual(unpickled.values, template.values)
                    self.assertEqual(fstring(unpickled), fstring(template))

    def test_pickle_interpolation(self):
        for interpolation in (
            Interpolation('Nikita', 'name', None, ''),
            Interpolation('Nikita', 'name', 'r', ''),
            Interpolation(1/3, 'x', None, '.2f'),
        ):
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                with self.subTest(proto=proto, interpolation=interpolation):
                    pickled = pickle.dumps(interpolation, protocol=proto)
                    unpickled = pickle.loads(pickled)

                    self.assertEqual(unpickled.value, interpolation.value)
                    self.assertEqual(unpickled.expression, interpolation.expression)
                    self.assertEqual(unpickled.conversion, interpolation.conversion)
                    self.assertEqual(unpickled.format_spec, interpolation.format_spec)


class TemplateIterTests(unittest.TestCase):
    def test_abc(self):
        self.assertIsInstance(iter(t''), Iterable)
        self.assertIsInstance(iter(t''), Iterator)

    def test_final(self):
        TemplateIter = type(iter(t''))
        with self.assertRaisesRegex(TypeError, 'is not an acceptable base type'):
            class Sub(TemplateIter): ...

    def test_iter(self):
        x = 1
        res = list(iter(t'abc {x} yz'))

        self.assertEqual(res[0], 'abc ')
        self.assertIsInstance(res[1], Interpolation)
        self.assertEqual(res[1].value, 1)
        self.assertEqual(res[1].expression, 'x')
        self.assertEqual(res[1].conversion, None)
        self.assertEqual(res[1].format_spec, '')
        self.assertEqual(res[2], ' yz')

    def test_exhausted(self):
        # See https://github.com/python/cpython/issues/134119.
        template_iter = iter(t"{1}")
        self.assertIsInstance(next(template_iter), Interpolation)
        self.assertRaises(StopIteration, next, template_iter)
        self.assertRaises(StopIteration, next, template_iter)


if __name__ == '__main__':
    unittest.main()
