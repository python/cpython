import unittest, string
from test import support


class ModuleTest(unittest.TestCase):

    def test_attrs(self):
        string.whitespace
        string.ascii_lowercase
        string.ascii_uppercase
        string.ascii_letters
        string.digits
        string.hexdigits
        string.octdigits
        string.punctuation
        string.printable

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

    def test_basic_formatter(self):
        fmt = string.Formatter()
        self.assertEqual(fmt.format("foo"), "foo")
        self.assertEqual(fmt.format("foo{0}", "bar"), "foobar")
        self.assertEqual(fmt.format("foo{1}{0}-{1}", "bar", 6), "foo6bar-6")
        self.assertRaises(TypeError, fmt.format)
        self.assertRaises(TypeError, string.Formatter.format)

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

    def test_auto_numbering(self):
        fmt = string.Formatter()
        self.assertEqual(fmt.format('foo{}{}', 'bar', 6),
                         'foo{}{}'.format('bar', 6))
        self.assertEqual(fmt.format('foo{1}{num}{1}', None, 'bar', num=6),
                         'foo{1}{num}{1}'.format(None, 'bar', num=6))
        self.assertEqual(fmt.format('{:^{}}', 'bar', 6),
                         '{:^{}}'.format('bar', 6))
        self.assertEqual(fmt.format('{:^{}} {}', 'bar', 6, 'X'),
                         '{:^{}} {}'.format('bar', 6, 'X'))
        self.assertEqual(fmt.format('{:^{pad}}{}', 'foo', 'bar', pad=6),
                         '{:^{pad}}{}'.format('foo', 'bar', pad=6))

        with self.assertRaises(ValueError):
            fmt.format('foo{1}{}', 'bar', 6)

        with self.assertRaises(ValueError):
            fmt.format('foo{}{1}', 'bar', 6)

    def test_conversion_specifiers(self):
        fmt = string.Formatter()
        self.assertEqual(fmt.format("-{arg!r}-", arg='test'), "-'test'-")
        self.assertEqual(fmt.format("{0!s}", 'test'), 'test')
        self.assertRaises(ValueError, fmt.format, "{0!h}", 'test')
        # issue13579
        self.assertEqual(fmt.format("{0!a}", 42), '42')
        self.assertEqual(fmt.format("{0!a}",  string.ascii_letters),
            "'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'")
        self.assertEqual(fmt.format("{0!a}",  chr(255)), "'\\xff'")
        self.assertEqual(fmt.format("{0!a}",  chr(256)), "'\\u0100'")

    def test_name_lookup(self):
        fmt = string.Formatter()
        class AnyAttr:
            def __getattr__(self, attr):
                return attr
        x = AnyAttr()
        self.assertEqual(fmt.format("{0.lumber}{0.jack}", x), 'lumberjack')
        with self.assertRaises(AttributeError):
            fmt.format("{0.lumber}{0.jack}", '')

    def test_index_lookup(self):
        fmt = string.Formatter()
        lookup = ["eggs", "and", "spam"]
        self.assertEqual(fmt.format("{0[2]}{0[0]}", lookup), 'spameggs')
        with self.assertRaises(IndexError):
            fmt.format("{0[2]}{0[0]}", [])
        with self.assertRaises(KeyError):
            fmt.format("{0[2]}{0[0]}", {})

    def test_override_get_value(self):
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


    def test_override_format_field(self):
        class CallFormatter(string.Formatter):
            def format_field(self, value, format_spec):
                return format(value(), format_spec)

        fmt = CallFormatter()
        self.assertEqual(fmt.format('*{0}*', lambda : 'result'), '*result*')


    def test_override_convert_field(self):
        class XFormatter(string.Formatter):
            def convert_field(self, value, conversion):
                if conversion == 'x':
                    return None
                return super().convert_field(value, conversion)

        fmt = XFormatter()
        self.assertEqual(fmt.format("{0!r}:{0!x}", 'foo', 'foo'), "'foo':None")


    def test_override_parse(self):
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

    def test_check_unused_args(self):
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

    def test_vformat_recursion_limit(self):
        fmt = string.Formatter()
        args = ()
        kwargs = dict(i=100)
        with self.assertRaises(ValueError) as err:
            fmt._vformat("{i}", args, kwargs, set(), -1)
        self.assertIn("recursion", str(err.exception))


def test_main():
    support.run_unittest(ModuleTest)

if __name__ == "__main__":
    test_main()
