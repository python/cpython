from string.templatelib import Template, Interpolation

from test.test_string._support import TStringTestCase, fstring


class TestTemplate(TStringTestCase):

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
