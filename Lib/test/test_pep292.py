# Copyright (C) 2004 Python Software Foundation
# Author: barry@python.org (Barry Warsaw)
# License: http://www.opensource.org/licenses/PythonSoftFoundation.php

import unittest
from string import Template, SafeTemplate

class TestTemplate(unittest.TestCase):

    def test_regular_templates(self):
        s = Template('$who likes to eat a bag of $what worth $$100')
        self.assertEqual(s % dict(who='tim', what='ham'),
                         'tim likes to eat a bag of ham worth $100')
        self.assertRaises(KeyError, lambda s, d: s % d, s, dict(who='tim'))

    def test_regular_templates_with_braces(self):
        s = Template('$who likes ${what} for ${meal}')
        self.assertEqual(s % dict(who='tim', what='ham', meal='dinner'),
                         'tim likes ham for dinner')
        self.assertRaises(KeyError, lambda s, d: s % d,
                          s, dict(who='tim', what='ham'))

    def test_escapes(self):
        eq = self.assertEqual
        s = Template('$who likes to eat a bag of $$what worth $$100')
        eq(s % dict(who='tim', what='ham'),
           'tim likes to eat a bag of $what worth $100')
        s = Template('$who likes $$')
        eq(s % dict(who='tim', what='ham'), 'tim likes $')

    def test_percents(self):
        s = Template('%(foo)s $foo ${foo}')
        self.assertEqual(s % dict(foo='baz'), '%(foo)s baz baz')
        s = SafeTemplate('%(foo)s $foo ${foo}')
        self.assertEqual(s % dict(foo='baz'), '%(foo)s baz baz')

    def test_stringification(self):
        s = Template('tim has eaten $count bags of ham today')
        self.assertEqual(s % dict(count=7),
                         'tim has eaten 7 bags of ham today')
        s = SafeTemplate('tim has eaten $count bags of ham today')
        self.assertEqual(s % dict(count=7),
                         'tim has eaten 7 bags of ham today')
        s = SafeTemplate('tim has eaten ${count} bags of ham today')
        self.assertEqual(s % dict(count=7),
                         'tim has eaten 7 bags of ham today')

    def test_SafeTemplate(self):
        eq = self.assertEqual
        s = SafeTemplate('$who likes ${what} for ${meal}')
        eq(s % dict(who='tim'),
           'tim likes ${what} for ${meal}')
        eq(s % dict(what='ham'),
           '$who likes ham for ${meal}')
        eq(s % dict(what='ham', meal='dinner'),
           '$who likes ham for dinner')
        eq(s % dict(who='tim', what='ham'),
           'tim likes ham for ${meal}')
        eq(s % dict(who='tim', what='ham', meal='dinner'),
           'tim likes ham for dinner')

    def test_invalid_placeholders(self):
        raises = self.assertRaises
        s = Template('$who likes $')
        raises(ValueError, lambda s, d: s % d, s, dict(who='tim'))
        s = Template('$who likes ${what)')
        raises(ValueError, lambda s, d: s % d, s, dict(who='tim'))
        s = Template('$who likes $100')
        raises(ValueError, lambda s, d: s % d, s, dict(who='tim'))


def suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestTemplate))
    return suite


def test_main():
    from test import test_support
    test_support.run_suite(suite())


if __name__ == '__main__':
    unittest.main()
