# Copyright (C) 2004 Python Software Foundation
# Author: barry@python.org (Barry Warsaw)
# License: http://www.opensource.org/licenses/PythonSoftFoundation.php

import unittest
from string import Template


class Bag:
    pass

class Mapping:
    def __getitem__(self, name):
        obj = self
        for part in name.split('.'):
            try:
                obj = getattr(obj, part)
            except AttributeError:
                raise KeyError(name)
        return obj


class TestTemplate(unittest.TestCase):
    def test_regular_templates(self):
        s = Template('$who likes to eat a bag of $what worth $$100')
        self.assertEqual(s.substitute(dict(who='tim', what='ham')),
                         'tim likes to eat a bag of ham worth $100')
        self.assertRaises(KeyError, s.substitute, dict(who='tim'))
        self.assertRaises(TypeError, Template.substitute)

    def test_regular_templates_with_braces(self):
        s = Template('$who likes ${what} for ${meal}')
        d = dict(who='tim', what='ham', meal='dinner')
        self.assertEqual(s.substitute(d), 'tim likes ham for dinner')
        self.assertRaises(KeyError, s.substitute,
                          dict(who='tim', what='ham'))

    def test_escapes(self):
        eq = self.assertEqual
        s = Template('$who likes to eat a bag of $$what worth $$100')
        eq(s.substitute(dict(who='tim', what='ham')),
           'tim likes to eat a bag of $what worth $100')
        s = Template('$who likes $$')
        eq(s.substitute(dict(who='tim', what='ham')), 'tim likes $')

    def test_percents(self):
        eq = self.assertEqual
        s = Template('%(foo)s $foo ${foo}')
        d = dict(foo='baz')
        eq(s.substitute(d), '%(foo)s baz baz')
        eq(s.safe_substitute(d), '%(foo)s baz baz')

    def test_stringification(self):
        eq = self.assertEqual
        s = Template('tim has eaten $count bags of ham today')
        d = dict(count=7)
        eq(s.substitute(d), 'tim has eaten 7 bags of ham today')
        eq(s.safe_substitute(d), 'tim has eaten 7 bags of ham today')
        s = Template('tim has eaten ${count} bags of ham today')
        eq(s.substitute(d), 'tim has eaten 7 bags of ham today')

    def test_tupleargs(self):
        eq = self.assertEqual
        s = Template('$who ate ${meal}')
        d = dict(who=('tim', 'fred'), meal=('ham', 'kung pao'))
        eq(s.substitute(d), "('tim', 'fred') ate ('ham', 'kung pao')")
        eq(s.safe_substitute(d), "('tim', 'fred') ate ('ham', 'kung pao')")

    def test_SafeTemplate(self):
        eq = self.assertEqual
        s = Template('$who likes ${what} for ${meal}')
        eq(s.safe_substitute(dict(who='tim')), 'tim likes ${what} for ${meal}')
        eq(s.safe_substitute(dict(what='ham')), '$who likes ham for ${meal}')
        eq(s.safe_substitute(dict(what='ham', meal='dinner')),
           '$who likes ham for dinner')
        eq(s.safe_substitute(dict(who='tim', what='ham')),
           'tim likes ham for ${meal}')
        eq(s.safe_substitute(dict(who='tim', what='ham', meal='dinner')),
           'tim likes ham for dinner')

    def test_invalid_placeholders(self):
        raises = self.assertRaises
        s = Template('$who likes $')
        raises(ValueError, s.substitute, dict(who='tim'))
        s = Template('$who likes ${what)')
        raises(ValueError, s.substitute, dict(who='tim'))
        s = Template('$who likes $100')
        raises(ValueError, s.substitute, dict(who='tim'))

    def test_idpattern_override(self):
        class PathPattern(Template):
            idpattern = r'[_a-z][._a-z0-9]*'
        m = Mapping()
        m.bag = Bag()
        m.bag.foo = Bag()
        m.bag.foo.who = 'tim'
        m.bag.what = 'ham'
        s = PathPattern('$bag.foo.who likes to eat a bag of $bag.what')
        self.assertEqual(s.substitute(m), 'tim likes to eat a bag of ham')

    def test_pattern_override(self):
        class MyPattern(Template):
            pattern = r"""
            (?P<escaped>@{2})                   |
            @(?P<named>[_a-z][._a-z0-9]*)       |
            @{(?P<braced>[_a-z][._a-z0-9]*)}    |
            (?P<invalid>@)
            """
        m = Mapping()
        m.bag = Bag()
        m.bag.foo = Bag()
        m.bag.foo.who = 'tim'
        m.bag.what = 'ham'
        s = MyPattern('@bag.foo.who likes to eat a bag of @bag.what')
        self.assertEqual(s.substitute(m), 'tim likes to eat a bag of ham')

        class BadPattern(Template):
            pattern = r"""
            (?P<badname>.*)                     |
            (?P<escaped>@{2})                   |
            @(?P<named>[_a-z][._a-z0-9]*)       |
            @{(?P<braced>[_a-z][._a-z0-9]*)}    |
            (?P<invalid>@)                      |
            """
        s = BadPattern('@bag.foo.who likes to eat a bag of @bag.what')
        self.assertRaises(ValueError, s.substitute, {})
        self.assertRaises(ValueError, s.safe_substitute, {})

    def test_braced_override(self):
        class MyTemplate(Template):
            pattern = r"""
            \$(?:
              (?P<escaped>$)                     |
              (?P<named>[_a-z][_a-z0-9]*)        |
              @@(?P<braced>[_a-z][_a-z0-9]*)@@   |
              (?P<invalid>)                      |
           )
           """

        tmpl = 'PyCon in $@@location@@'
        t = MyTemplate(tmpl)
        self.assertRaises(KeyError, t.substitute, {})
        val = t.substitute({'location': 'Cleveland'})
        self.assertEqual(val, 'PyCon in Cleveland')

    def test_braced_override_safe(self):
        class MyTemplate(Template):
            pattern = r"""
            \$(?:
              (?P<escaped>$)                     |
              (?P<named>[_a-z][_a-z0-9]*)        |
              @@(?P<braced>[_a-z][_a-z0-9]*)@@   |
              (?P<invalid>)                      |
           )
           """

        tmpl = 'PyCon in $@@location@@'
        t = MyTemplate(tmpl)
        self.assertEqual(t.safe_substitute(), tmpl)
        val = t.safe_substitute({'location': 'Cleveland'})
        self.assertEqual(val, 'PyCon in Cleveland')

    def test_invalid_with_no_lines(self):
        # The error formatting for invalid templates
        # has a special case for no data that the default
        # pattern can't trigger (always has at least '$')
        # So we craft a pattern that is always invalid
        # with no leading data.
        class MyTemplate(Template):
            pattern = r"""
              (?P<invalid>) |
              unreachable(
                (?P<named>)   |
                (?P<braced>)  |
                (?P<escaped>)
              )
            """
        s = MyTemplate('')
        with self.assertRaises(ValueError) as err:
            s.substitute({})
        self.assertIn('line 1, col 1', str(err.exception))

    def test_unicode_values(self):
        s = Template('$who likes $what')
        d = dict(who='t\xffm', what='f\xfe\fed')
        self.assertEqual(s.substitute(d), 't\xffm likes f\xfe\x0ced')

    def test_keyword_arguments(self):
        eq = self.assertEqual
        s = Template('$who likes $what')
        eq(s.substitute(who='tim', what='ham'), 'tim likes ham')
        eq(s.substitute(dict(who='tim'), what='ham'), 'tim likes ham')
        eq(s.substitute(dict(who='fred', what='kung pao'),
                        who='tim', what='ham'),
           'tim likes ham')
        s = Template('the mapping is $mapping')
        eq(s.substitute(dict(foo='none'), mapping='bozo'),
           'the mapping is bozo')
        eq(s.substitute(dict(mapping='one'), mapping='two'),
           'the mapping is two')

        s = Template('the self is $self')
        eq(s.substitute(self='bozo'), 'the self is bozo')

    def test_keyword_arguments_safe(self):
        eq = self.assertEqual
        raises = self.assertRaises
        s = Template('$who likes $what')
        eq(s.safe_substitute(who='tim', what='ham'), 'tim likes ham')
        eq(s.safe_substitute(dict(who='tim'), what='ham'), 'tim likes ham')
        eq(s.safe_substitute(dict(who='fred', what='kung pao'),
                        who='tim', what='ham'),
           'tim likes ham')
        s = Template('the mapping is $mapping')
        eq(s.safe_substitute(dict(foo='none'), mapping='bozo'),
           'the mapping is bozo')
        eq(s.safe_substitute(dict(mapping='one'), mapping='two'),
           'the mapping is two')
        d = dict(mapping='one')
        raises(TypeError, s.substitute, d, {})
        raises(TypeError, s.safe_substitute, d, {})

        s = Template('the self is $self')
        eq(s.safe_substitute(self='bozo'), 'the self is bozo')

    def test_delimiter_override(self):
        eq = self.assertEqual
        raises = self.assertRaises
        class AmpersandTemplate(Template):
            delimiter = '&'
        s = AmpersandTemplate('this &gift is for &{who} &&')
        eq(s.substitute(gift='bud', who='you'), 'this bud is for you &')
        raises(KeyError, s.substitute)
        eq(s.safe_substitute(gift='bud', who='you'), 'this bud is for you &')
        eq(s.safe_substitute(), 'this &gift is for &{who} &')
        s = AmpersandTemplate('this &gift is for &{who} &')
        raises(ValueError, s.substitute, dict(gift='bud', who='you'))
        eq(s.safe_substitute(), 'this &gift is for &{who} &')

        class PieDelims(Template):
            delimiter = '@'
        s = PieDelims('@who likes to eat a bag of @{what} worth $100')
        self.assertEqual(s.substitute(dict(who='tim', what='ham')),
                         'tim likes to eat a bag of ham worth $100')


def test_main():
    from test import support
    test_classes = [TestTemplate,]
    support.run_unittest(*test_classes)


if __name__ == '__main__':
    test_main()
