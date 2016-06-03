import unittest
import string
from string import Template
from test import test_support, string_tests
from UserList import UserList

class StringTest(
    string_tests.CommonTest,
    string_tests.MixinStrStringUserStringTest
    ):

    type2test = str

    def checkequal(self, result, object, methodname, *args):
        realresult = getattr(string, methodname)(object, *args)
        self.assertEqual(
            result,
            realresult
        )

    def checkraises(self, exc, obj, methodname, *args):
        with self.assertRaises(exc) as cm:
            getattr(string, methodname)(obj, *args)
        self.assertNotEqual(cm.exception.args[0], '')

    def checkcall(self, object, methodname, *args):
        getattr(string, methodname)(object, *args)

    def test_join(self):
        # These are the same checks as in string_test.ObjectTest.test_join
        # but the argument order ist different
        self.checkequal('a b c d', ['a', 'b', 'c', 'd'], 'join', ' ')
        self.checkequal('abcd', ('a', 'b', 'c', 'd'), 'join', '')
        self.checkequal('w x y z', string_tests.Sequence(), 'join', ' ')
        self.checkequal('abc', ('abc',), 'join', 'a')
        self.checkequal('z', UserList(['z']), 'join', 'a')
        if test_support.have_unicode:
            self.checkequal(unicode('a.b.c'), ['a', 'b', 'c'], 'join', unicode('.'))
            self.checkequal(unicode('a.b.c'), [unicode('a'), 'b', 'c'], 'join', '.')
            self.checkequal(unicode('a.b.c'), ['a', unicode('b'), 'c'], 'join', '.')
            self.checkequal(unicode('a.b.c'), ['a', 'b', unicode('c')], 'join', '.')
            self.checkraises(TypeError, ['a', unicode('b'), 3], 'join', '.')
        for i in [5, 25, 125]:
            self.checkequal(
                ((('a' * i) + '-') * i)[:-1],
                ['a' * i] * i, 'join', '-')
            self.checkequal(
                ((('a' * i) + '-') * i)[:-1],
                ('a' * i,) * i, 'join', '-')

        self.checkraises(TypeError, string_tests.BadSeq1(), 'join', ' ')
        self.checkequal('a b c', string_tests.BadSeq2(), 'join', ' ')
        try:
            def f():
                yield 4 + ""
            self.fixtype(' ').join(f())
        except TypeError, e:
            if '+' not in str(e):
                self.fail('join() ate exception message')
        else:
            self.fail('exception not raised')


class ModuleTest(unittest.TestCase):

    def test_attrs(self):
        string.whitespace
        string.lowercase
        string.uppercase
        string.letters
        string.digits
        string.hexdigits
        string.octdigits
        string.punctuation
        string.printable

    def test_atoi(self):
        self.assertEqual(string.atoi(" 1 "), 1)
        self.assertRaises(ValueError, string.atoi, " 1x")
        self.assertRaises(ValueError, string.atoi, " x1 ")

    def test_atol(self):
        self.assertEqual(string.atol("  1  "), 1L)
        self.assertRaises(ValueError, string.atol, "  1x ")
        self.assertRaises(ValueError, string.atol, "  x1 ")

    def test_atof(self):
        self.assertAlmostEqual(string.atof("  1  "), 1.0)
        self.assertRaises(ValueError, string.atof, "  1x ")
        self.assertRaises(ValueError, string.atof, "  x1 ")

    def test_maketrans(self):
        transtable = '\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`xyzdefghijklmnopqrstuvwxyz{|}~\177\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377'

        self.assertEqual(string.maketrans('abc', 'xyz'), transtable)
        self.assertRaises(ValueError, string.maketrans, 'abc', 'xyzq')

    def test_capwords(self):
        self.assertEqual(string.capwords('abc def ghi'), 'Abc Def Ghi')
        self.assertEqual(string.capwords('abc\tdef\nghi'), 'Abc Def Ghi')
        self.assertEqual(string.capwords('abc\t   def  \nghi'), 'Abc Def Ghi')
        self.assertEqual(string.capwords('ABC DEF GHI'), 'Abc Def Ghi')
        self.assertEqual(string.capwords('ABC-DEF-GHI', '-'), 'Abc-Def-Ghi')
        self.assertEqual(string.capwords('ABC-def DEF-ghi GHI'), 'Abc-def Def-ghi Ghi')
        self.assertEqual(string.capwords('   aBc  DeF   '), 'Abc Def')
        self.assertEqual(string.capwords('\taBc\tDeF\t'), 'Abc Def')
        self.assertEqual(string.capwords('\taBc\tDeF\t', '\t'), '\tAbc\tDef\t')

    def test_formatter(self):
        fmt = string.Formatter()
        self.assertEqual(fmt.format("foo"), "foo")

        self.assertEqual(fmt.format("foo{0}", "bar"), "foobar")
        self.assertEqual(fmt.format("foo{1}{0}-{1}", "bar", 6), "foo6bar-6")
        self.assertEqual(fmt.format("-{arg!r}-", arg='test'), "-'test'-")

        # override get_value ############################################
        class NamespaceFormatter(string.Formatter):
            def __init__(self, namespace={}):
                string.Formatter.__init__(self)
                self.namespace = namespace

            def get_value(self, key, args, kwds):
                if isinstance(key, str):
                    try:
                        # Check explicitly passed arguments first
                        return kwds[key]
                    except KeyError:
                        return self.namespace[key]
                else:
                    string.Formatter.get_value(key, args, kwds)

        fmt = NamespaceFormatter({'greeting':'hello'})
        self.assertEqual(fmt.format("{greeting}, world!"), 'hello, world!')


        # override format_field #########################################
        class CallFormatter(string.Formatter):
            def format_field(self, value, format_spec):
                return format(value(), format_spec)

        fmt = CallFormatter()
        self.assertEqual(fmt.format('*{0}*', lambda : 'result'), '*result*')


        # override convert_field ########################################
        class XFormatter(string.Formatter):
            def convert_field(self, value, conversion):
                if conversion == 'x':
                    return None
                return super(XFormatter, self).convert_field(value, conversion)

        fmt = XFormatter()
        self.assertEqual(fmt.format("{0!r}:{0!x}", 'foo', 'foo'), "'foo':None")


        # override parse ################################################
        class BarFormatter(string.Formatter):
            # returns an iterable that contains tuples of the form:
            # (literal_text, field_name, format_spec, conversion)
            def parse(self, format_string):
                for field in format_string.split('|'):
                    if field[0] == '+':
                        # it's markup
                        field_name, _, format_spec = field[1:].partition(':')
                        yield '', field_name, format_spec, None
                    else:
                        yield field, None, None, None

        fmt = BarFormatter()
        self.assertEqual(fmt.format('*|+0:^10s|*', 'foo'), '*   foo    *')

        # test all parameters used
        class CheckAllUsedFormatter(string.Formatter):
            def check_unused_args(self, used_args, args, kwargs):
                # Track which arguments actually got used
                unused_args = set(kwargs.keys())
                unused_args.update(range(0, len(args)))

                for arg in used_args:
                    unused_args.remove(arg)

                if unused_args:
                    raise ValueError("unused arguments")

        fmt = CheckAllUsedFormatter()
        self.assertEqual(fmt.format("{0}", 10), "10")
        self.assertEqual(fmt.format("{0}{i}", 10, i=100), "10100")
        self.assertEqual(fmt.format("{0}{i}{1}", 10, 20, i=100), "1010020")
        self.assertRaises(ValueError, fmt.format, "{0}{i}{1}", 10, 20, i=100, j=0)
        self.assertRaises(ValueError, fmt.format, "{0}", 10, 20)
        self.assertRaises(ValueError, fmt.format, "{0}", 10, 20, i=100)
        self.assertRaises(ValueError, fmt.format, "{i}", 10, 20, i=100)

        # Alternate formatting is not supported
        self.assertRaises(ValueError, format, '', '#')
        self.assertRaises(ValueError, format, '', '#20')

    def test_format_keyword_arguments(self):
        fmt = string.Formatter()
        self.assertEqual(fmt.format("-{arg}-", arg='test'), '-test-')
        self.assertRaises(KeyError, fmt.format, "-{arg}-")
        self.assertEqual(fmt.format("-{self}-", self='test'), '-test-')
        self.assertRaises(KeyError, fmt.format, "-{self}-")
        self.assertEqual(fmt.format("-{format_string}-", format_string='test'),
                         '-test-')
        self.assertRaises(KeyError, fmt.format, "-{format_string}-")
        self.assertEqual(fmt.format(arg='test', format_string="-{arg}-"),
                         '-test-')

class BytesAliasTest(unittest.TestCase):

    def test_builtin(self):
        self.assertTrue(str is bytes)

    def test_syntax(self):
        self.assertEqual(b"spam", "spam")
        self.assertEqual(br"egg\foo", "egg\\foo")
        self.assertTrue(type(b""), str)
        self.assertTrue(type(br""), str)


# Template tests (formerly housed in test_pep292.py)

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

    def test_unicode_values(self):
        s = Template('$who likes $what')
        d = dict(who=u't\xffm', what=u'f\xfe\fed')
        self.assertEqual(s.substitute(d), u't\xffm likes f\xfe\x0ced')

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
    test_support.run_unittest(StringTest, ModuleTest, BytesAliasTest, TestTemplate)

if __name__ == '__main__':
    test_main()
