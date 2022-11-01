# Argument Clinic
# Copyright 2012-2013 by Larry Hastings.
# Licensed to the PSF under a contributor agreement.

from test import support, test_tools
from test.support import os_helper
from unittest import TestCase
import collections
import inspect
import os.path
import sys
import unittest

test_tools.skip_if_missing('clinic')
with test_tools.imports_under_tool('clinic'):
    import clinic
    from clinic import DSLParser


class FakeConverter:
    def __init__(self, name, args):
        self.name = name
        self.args = args


class FakeConverterFactory:
    def __init__(self, name):
        self.name = name

    def __call__(self, name, default, **kwargs):
        return FakeConverter(self.name, kwargs)


class FakeConvertersDict:
    def __init__(self):
        self.used_converters = {}

    def get(self, name, default):
        return self.used_converters.setdefault(name, FakeConverterFactory(name))

c = clinic.Clinic(language='C', filename = "file")

class FakeClinic:
    def __init__(self):
        self.converters = FakeConvertersDict()
        self.legacy_converters = FakeConvertersDict()
        self.language = clinic.CLanguage(None)
        self.filename = None
        self.destination_buffers = {}
        self.block_parser = clinic.BlockParser('', self.language)
        self.modules = collections.OrderedDict()
        self.classes = collections.OrderedDict()
        clinic.clinic = self
        self.name = "FakeClinic"
        self.line_prefix = self.line_suffix = ''
        self.destinations = {}
        self.add_destination("block", "buffer")
        self.add_destination("file", "buffer")
        self.add_destination("suppress", "suppress")
        d = self.destinations.get
        self.field_destinations = collections.OrderedDict((
            ('docstring_prototype', d('suppress')),
            ('docstring_definition', d('block')),
            ('methoddef_define', d('block')),
            ('impl_prototype', d('block')),
            ('parser_prototype', d('suppress')),
            ('parser_definition', d('block')),
            ('impl_definition', d('block')),
        ))

    def get_destination(self, name):
        d = self.destinations.get(name)
        if not d:
            sys.exit("Destination does not exist: " + repr(name))
        return d

    def add_destination(self, name, type, *args):
        if name in self.destinations:
            sys.exit("Destination already exists: " + repr(name))
        self.destinations[name] = clinic.Destination(name, type, self, *args)

    def is_directive(self, name):
        return name == "module"

    def directive(self, name, args):
        self.called_directives[name] = args

    _module_and_class = clinic.Clinic._module_and_class

class ClinicWholeFileTest(TestCase):
    def test_eol(self):
        # regression test:
        # clinic's block parser didn't recognize
        # the "end line" for the block if it
        # didn't end in "\n" (as in, the last)
        # byte of the file was '/'.
        # so it would spit out an end line for you.
        # and since you really already had one,
        # the last line of the block got corrupted.
        c = clinic.Clinic(clinic.CLanguage(None), filename="file")
        raw = "/*[clinic]\nfoo\n[clinic]*/"
        cooked = c.parse(raw).splitlines()
        end_line = cooked[2].rstrip()
        # this test is redundant, it's just here explicitly to catch
        # the regression test so we don't forget what it looked like
        self.assertNotEqual(end_line, "[clinic]*/[clinic]*/")
        self.assertEqual(end_line, "[clinic]*/")



class ClinicGroupPermuterTest(TestCase):
    def _test(self, l, m, r, output):
        computed = clinic.permute_optional_groups(l, m, r)
        self.assertEqual(output, computed)

    def test_range(self):
        self._test([['start']], ['stop'], [['step']],
          (
            ('stop',),
            ('start', 'stop',),
            ('start', 'stop', 'step',),
          ))

    def test_add_window(self):
        self._test([['x', 'y']], ['ch'], [['attr']],
          (
            ('ch',),
            ('ch', 'attr'),
            ('x', 'y', 'ch',),
            ('x', 'y', 'ch', 'attr'),
          ))

    def test_ludicrous(self):
        self._test([['a1', 'a2', 'a3'], ['b1', 'b2']], ['c1'], [['d1', 'd2'], ['e1', 'e2', 'e3']],
          (
          ('c1',),
          ('b1', 'b2', 'c1'),
          ('b1', 'b2', 'c1', 'd1', 'd2'),
          ('a1', 'a2', 'a3', 'b1', 'b2', 'c1'),
          ('a1', 'a2', 'a3', 'b1', 'b2', 'c1', 'd1', 'd2'),
          ('a1', 'a2', 'a3', 'b1', 'b2', 'c1', 'd1', 'd2', 'e1', 'e2', 'e3'),
          ))

    def test_right_only(self):
        self._test([], [], [['a'],['b'],['c']],
          (
          (),
          ('a',),
          ('a', 'b'),
          ('a', 'b', 'c')
          ))

    def test_have_left_options_but_required_is_empty(self):
        def fn():
            clinic.permute_optional_groups(['a'], [], [])
        self.assertRaises(ValueError, fn)


class ClinicLinearFormatTest(TestCase):
    def _test(self, input, output, **kwargs):
        computed = clinic.linear_format(input, **kwargs)
        self.assertEqual(output, computed)

    def test_empty_strings(self):
        self._test('', '')

    def test_solo_newline(self):
        self._test('\n', '\n')

    def test_no_substitution(self):
        self._test("""
          abc
          """, """
          abc
          """)

    def test_empty_substitution(self):
        self._test("""
          abc
          {name}
          def
          """, """
          abc
          def
          """, name='')

    def test_single_line_substitution(self):
        self._test("""
          abc
          {name}
          def
          """, """
          abc
          GARGLE
          def
          """, name='GARGLE')

    def test_multiline_substitution(self):
        self._test("""
          abc
          {name}
          def
          """, """
          abc
          bingle
          bungle

          def
          """, name='bingle\nbungle\n')

class InertParser:
    def __init__(self, clinic):
        pass

    def parse(self, block):
        pass

class CopyParser:
    def __init__(self, clinic):
        pass

    def parse(self, block):
        block.output = block.input


class ClinicBlockParserTest(TestCase):
    def _test(self, input, output):
        language = clinic.CLanguage(None)

        blocks = list(clinic.BlockParser(input, language))
        writer = clinic.BlockPrinter(language)
        for block in blocks:
            writer.print_block(block)
        output = writer.f.getvalue()
        assert output == input, "output != input!\n\noutput " + repr(output) + "\n\n input " + repr(input)

    def round_trip(self, input):
        return self._test(input, input)

    def test_round_trip_1(self):
        self.round_trip("""
    verbatim text here
    lah dee dah
""")
    def test_round_trip_2(self):
        self.round_trip("""
    verbatim text here
    lah dee dah
/*[inert]
abc
[inert]*/
def
/*[inert checksum: 7b18d017f89f61cf17d47f92749ea6930a3f1deb]*/
xyz
""")

    def _test_clinic(self, input, output):
        language = clinic.CLanguage(None)
        c = clinic.Clinic(language, filename="file")
        c.parsers['inert'] = InertParser(c)
        c.parsers['copy'] = CopyParser(c)
        computed = c.parse(input)
        self.assertEqual(output, computed)

    def test_clinic_1(self):
        self._test_clinic("""
    verbatim text here
    lah dee dah
/*[copy input]
def
[copy start generated code]*/
abc
/*[copy end generated code: output=03cfd743661f0797 input=7b18d017f89f61cf]*/
xyz
""", """
    verbatim text here
    lah dee dah
/*[copy input]
def
[copy start generated code]*/
def
/*[copy end generated code: output=7b18d017f89f61cf input=7b18d017f89f61cf]*/
xyz
""")


class ClinicParserTest(TestCase):
    def test_trivial(self):
        parser = DSLParser(FakeClinic())
        block = clinic.Block("module os\nos.access")
        parser.parse(block)
        module, function = block.signatures
        self.assertEqual("access", function.name)
        self.assertEqual("os", module.name)

    def test_ignore_line(self):
        block = self.parse("#\nmodule os\nos.access")
        module, function = block.signatures
        self.assertEqual("access", function.name)
        self.assertEqual("os", module.name)

    def test_param(self):
        function = self.parse_function("module os\nos.access\n   path: int")
        self.assertEqual("access", function.name)
        self.assertEqual(2, len(function.parameters))
        p = function.parameters['path']
        self.assertEqual('path', p.name)
        self.assertIsInstance(p.converter, clinic.int_converter)

    def test_param_default(self):
        function = self.parse_function("module os\nos.access\n    follow_symlinks: bool = True")
        p = function.parameters['follow_symlinks']
        self.assertEqual(True, p.default)

    def test_param_with_continuations(self):
        function = self.parse_function("module os\nos.access\n    follow_symlinks: \\\n   bool \\\n   =\\\n    True")
        p = function.parameters['follow_symlinks']
        self.assertEqual(True, p.default)

    def test_param_default_expression(self):
        function = self.parse_function("module os\nos.access\n    follow_symlinks: int(c_default='MAXSIZE') = sys.maxsize")
        p = function.parameters['follow_symlinks']
        self.assertEqual(sys.maxsize, p.default)
        self.assertEqual("MAXSIZE", p.converter.c_default)

        s = self.parse_function_should_fail("module os\nos.access\n    follow_symlinks: int = sys.maxsize")
        self.assertEqual(s, "Error on line 0:\nWhen you specify a named constant ('sys.maxsize') as your default value,\nyou MUST specify a valid c_default.\n")

    def test_param_no_docstring(self):
        function = self.parse_function("""
module os
os.access
    follow_symlinks: bool = True
    something_else: str = ''""")
        p = function.parameters['follow_symlinks']
        self.assertEqual(3, len(function.parameters))
        self.assertIsInstance(function.parameters['something_else'].converter, clinic.str_converter)

    def test_param_default_parameters_out_of_order(self):
        s = self.parse_function_should_fail("""
module os
os.access
    follow_symlinks: bool = True
    something_else: str""")
        self.assertEqual(s, """Error on line 0:
Can't have a parameter without a default ('something_else')
after a parameter with a default!
""")

    def disabled_test_converter_arguments(self):
        function = self.parse_function("module os\nos.access\n    path: path_t(allow_fd=1)")
        p = function.parameters['path']
        self.assertEqual(1, p.converter.args['allow_fd'])

    def test_function_docstring(self):
        function = self.parse_function("""
module os
os.stat as os_stat_fn

   path: str
       Path to be examined

Perform a stat system call on the given path.""")
        self.assertEqual("""
stat($module, /, path)
--

Perform a stat system call on the given path.

  path
    Path to be examined
""".strip(), function.docstring)

    def test_explicit_parameters_in_docstring(self):
        function = self.parse_function("""
module foo
foo.bar
  x: int
     Documentation for x.
  y: int

This is the documentation for foo.

Okay, we're done here.
""")
        self.assertEqual("""
bar($module, /, x, y)
--

This is the documentation for foo.

  x
    Documentation for x.

Okay, we're done here.
""".strip(), function.docstring)

    def test_parser_regression_special_character_in_parameter_column_of_docstring_first_line(self):
        function = self.parse_function("""
module os
os.stat
    path: str
This/used to break Clinic!
""")
        self.assertEqual("stat($module, /, path)\n--\n\nThis/used to break Clinic!", function.docstring)

    def test_c_name(self):
        function = self.parse_function("module os\nos.stat as os_stat_fn")
        self.assertEqual("os_stat_fn", function.c_basename)

    def test_return_converter(self):
        function = self.parse_function("module os\nos.stat -> int")
        self.assertIsInstance(function.return_converter, clinic.int_return_converter)

    def test_star(self):
        function = self.parse_function("module os\nos.access\n    *\n    follow_symlinks: bool = True")
        p = function.parameters['follow_symlinks']
        self.assertEqual(inspect.Parameter.KEYWORD_ONLY, p.kind)
        self.assertEqual(0, p.group)

    def test_group(self):
        function = self.parse_function("module window\nwindow.border\n [\n ls : int\n ]\n /\n")
        p = function.parameters['ls']
        self.assertEqual(1, p.group)

    def test_left_group(self):
        function = self.parse_function("""
module curses
curses.addch
   [
   y: int
     Y-coordinate.
   x: int
     X-coordinate.
   ]
   ch: char
     Character to add.
   [
   attr: long
     Attributes for the character.
   ]
   /
""")
        for name, group in (
            ('y', -1), ('x', -1),
            ('ch', 0),
            ('attr', 1),
            ):
            p = function.parameters[name]
            self.assertEqual(p.group, group)
            self.assertEqual(p.kind, inspect.Parameter.POSITIONAL_ONLY)
        self.assertEqual(function.docstring.strip(), """
addch([y, x,] ch, [attr])


  y
    Y-coordinate.
  x
    X-coordinate.
  ch
    Character to add.
  attr
    Attributes for the character.
            """.strip())

    def test_nested_groups(self):
        function = self.parse_function("""
module curses
curses.imaginary
   [
   [
   y1: int
     Y-coordinate.
   y2: int
     Y-coordinate.
   ]
   x1: int
     X-coordinate.
   x2: int
     X-coordinate.
   ]
   ch: char
     Character to add.
   [
   attr1: long
     Attributes for the character.
   attr2: long
     Attributes for the character.
   attr3: long
     Attributes for the character.
   [
   attr4: long
     Attributes for the character.
   attr5: long
     Attributes for the character.
   attr6: long
     Attributes for the character.
   ]
   ]
   /
""")
        for name, group in (
            ('y1', -2), ('y2', -2),
            ('x1', -1), ('x2', -1),
            ('ch', 0),
            ('attr1', 1), ('attr2', 1), ('attr3', 1),
            ('attr4', 2), ('attr5', 2), ('attr6', 2),
            ):
            p = function.parameters[name]
            self.assertEqual(p.group, group)
            self.assertEqual(p.kind, inspect.Parameter.POSITIONAL_ONLY)

        self.assertEqual(function.docstring.strip(), """
imaginary([[y1, y2,] x1, x2,] ch, [attr1, attr2, attr3, [attr4, attr5,
          attr6]])


  y1
    Y-coordinate.
  y2
    Y-coordinate.
  x1
    X-coordinate.
  x2
    X-coordinate.
  ch
    Character to add.
  attr1
    Attributes for the character.
  attr2
    Attributes for the character.
  attr3
    Attributes for the character.
  attr4
    Attributes for the character.
  attr5
    Attributes for the character.
  attr6
    Attributes for the character.
                """.strip())

    def parse_function_should_fail(self, s):
        with support.captured_stdout() as stdout:
            with self.assertRaises(SystemExit):
                self.parse_function(s)
        return stdout.getvalue()

    def test_disallowed_grouping__two_top_groups_on_left(self):
        s = self.parse_function_should_fail("""
module foo
foo.two_top_groups_on_left
    [
    group1 : int
    ]
    [
    group2 : int
    ]
    param: int
            """)
        self.assertEqual(s,
            ('Error on line 0:\n'
            'Function two_top_groups_on_left has an unsupported group configuration. (Unexpected state 2.b)\n'))

    def test_disallowed_grouping__two_top_groups_on_right(self):
        self.parse_function_should_fail("""
module foo
foo.two_top_groups_on_right
    param: int
    [
    group1 : int
    ]
    [
    group2 : int
    ]
            """)

    def test_disallowed_grouping__parameter_after_group_on_right(self):
        self.parse_function_should_fail("""
module foo
foo.parameter_after_group_on_right
    param: int
    [
    [
    group1 : int
    ]
    group2 : int
    ]
            """)

    def test_disallowed_grouping__group_after_parameter_on_left(self):
        self.parse_function_should_fail("""
module foo
foo.group_after_parameter_on_left
    [
    group2 : int
    [
    group1 : int
    ]
    ]
    param: int
            """)

    def test_disallowed_grouping__empty_group_on_left(self):
        self.parse_function_should_fail("""
module foo
foo.empty_group
    [
    [
    ]
    group2 : int
    ]
    param: int
            """)

    def test_disallowed_grouping__empty_group_on_right(self):
        self.parse_function_should_fail("""
module foo
foo.empty_group
    param: int
    [
    [
    ]
    group2 : int
    ]
            """)

    def test_no_parameters(self):
        function = self.parse_function("""
module foo
foo.bar

Docstring

""")
        self.assertEqual("bar($module, /)\n--\n\nDocstring", function.docstring)
        self.assertEqual(1, len(function.parameters)) # self!

    def test_init_with_no_parameters(self):
        function = self.parse_function("""
module foo
class foo.Bar "unused" "notneeded"
foo.Bar.__init__

Docstring

""", signatures_in_block=3, function_index=2)
        # self is not in the signature
        self.assertEqual("Bar()\n--\n\nDocstring", function.docstring)
        # but it *is* a parameter
        self.assertEqual(1, len(function.parameters))

    def test_illegal_module_line(self):
        self.parse_function_should_fail("""
module foo
foo.bar => int
    /
""")

    def test_illegal_c_basename(self):
        self.parse_function_should_fail("""
module foo
foo.bar as 935
    /
""")

    def test_single_star(self):
        self.parse_function_should_fail("""
module foo
foo.bar
    *
    *
""")

    def test_parameters_required_after_star_without_initial_parameters_or_docstring(self):
        self.parse_function_should_fail("""
module foo
foo.bar
    *
""")

    def test_parameters_required_after_star_without_initial_parameters_with_docstring(self):
        self.parse_function_should_fail("""
module foo
foo.bar
    *
Docstring here.
""")

    def test_parameters_required_after_star_with_initial_parameters_without_docstring(self):
        self.parse_function_should_fail("""
module foo
foo.bar
    this: int
    *
""")

    def test_parameters_required_after_star_with_initial_parameters_and_docstring(self):
        self.parse_function_should_fail("""
module foo
foo.bar
    this: int
    *
Docstring.
""")

    def test_single_slash(self):
        self.parse_function_should_fail("""
module foo
foo.bar
    /
    /
""")

    def test_mix_star_and_slash(self):
        self.parse_function_should_fail("""
module foo
foo.bar
   x: int
   y: int
   *
   z: int
   /
""")

    def test_parameters_not_permitted_after_slash_for_now(self):
        self.parse_function_should_fail("""
module foo
foo.bar
    /
    x: int
""")

    def test_function_not_at_column_0(self):
        function = self.parse_function("""
  module foo
  foo.bar
    x: int
      Nested docstring here, goeth.
    *
    y: str
  Not at column 0!
""")
        self.assertEqual("""
bar($module, /, x, *, y)
--

Not at column 0!

  x
    Nested docstring here, goeth.
""".strip(), function.docstring)

    def test_directive(self):
        c = FakeClinic()
        parser = DSLParser(c)
        parser.flag = False
        parser.directives['setflag'] = lambda : setattr(parser, 'flag', True)
        block = clinic.Block("setflag")
        parser.parse(block)
        self.assertTrue(parser.flag)

    def test_legacy_converters(self):
        block = self.parse('module os\nos.access\n   path: "s"')
        module, function = block.signatures
        self.assertIsInstance((function.parameters['path']).converter, clinic.str_converter)

    def parse(self, text):
        c = FakeClinic()
        parser = DSLParser(c)
        block = clinic.Block(text)
        parser.parse(block)
        return block

    def parse_function(self, text, signatures_in_block=2, function_index=1):
        block = self.parse(text)
        s = block.signatures
        self.assertEqual(len(s), signatures_in_block)
        assert isinstance(s[0], clinic.Module)
        assert isinstance(s[function_index], clinic.Function)
        return s[function_index]

    def test_scaffolding(self):
        # test repr on special values
        self.assertEqual(repr(clinic.unspecified), '<Unspecified>')
        self.assertEqual(repr(clinic.NULL), '<Null>')

        # test that fail fails
        with support.captured_stdout() as stdout:
            with self.assertRaises(SystemExit):
                clinic.fail('The igloos are melting!', filename='clown.txt', line_number=69)
        self.assertEqual(stdout.getvalue(), 'Error in file "clown.txt" on line 69:\nThe igloos are melting!\n')


class ClinicExternalTest(TestCase):
    maxDiff = None

    def test_external(self):
        # bpo-42398: Test that the destination file is left unchanged if the
        # content does not change. Moreover, check also that the file
        # modification time does not change in this case.
        source = support.findfile('clinic.test')
        with open(source, 'r', encoding='utf-8') as f:
            orig_contents = f.read()

        with os_helper.temp_dir() as tmp_dir:
            testfile = os.path.join(tmp_dir, 'clinic.test.c')
            with open(testfile, 'w', encoding='utf-8') as f:
                f.write(orig_contents)
            old_mtime_ns = os.stat(testfile).st_mtime_ns

            clinic.parse_file(testfile)

            with open(testfile, 'r', encoding='utf-8') as f:
                new_contents = f.read()
            new_mtime_ns = os.stat(testfile).st_mtime_ns

        self.assertEqual(new_contents, orig_contents)
        # Don't change the file modification time
        # if the content does not change
        self.assertEqual(new_mtime_ns, old_mtime_ns)


if __name__ == "__main__":
    unittest.main()
