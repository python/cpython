# Argument Clinic
# Copyright 2012-2013 by Larry Hastings.
# Licensed to the PSF under a contributor agreement.

from functools import partial
from test import support, test_tools
from test.support import os_helper
from test.support.os_helper import TESTFN, unlink
from textwrap import dedent
from unittest import TestCase
import collections
import inspect
import os.path
import subprocess
import sys
import unittest

test_tools.skip_if_missing('clinic')
with test_tools.imports_under_tool('clinic'):
    import clinic
    from clinic import DSLParser


def restore_dict(converters, old_converters):
    converters.clear()
    converters.update(old_converters)


def save_restore_converters(testcase):
    testcase.addCleanup(restore_dict, clinic.converters,
                        clinic.converters.copy())
    testcase.addCleanup(restore_dict, clinic.legacy_converters,
                        clinic.legacy_converters.copy())
    testcase.addCleanup(restore_dict, clinic.return_converters,
                        clinic.return_converters.copy())


class _ParserBase(TestCase):
    maxDiff = None

    def expect_parser_failure(self, parser, _input):
        with support.captured_stdout() as stdout:
            with self.assertRaises(SystemExit):
                parser(_input)
        return stdout.getvalue()

    def parse_function_should_fail(self, _input):
        return self.expect_parser_failure(self.parse_function, _input)


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


class ClinicWholeFileTest(_ParserBase):
    def setUp(self):
        save_restore_converters(self)
        self.clinic = clinic.Clinic(clinic.CLanguage(None), filename="test.c")

    def expect_failure(self, raw):
        _input = dedent(raw).strip()
        return self.expect_parser_failure(self.clinic.parse, _input)

    def test_eol(self):
        # regression test:
        # clinic's block parser didn't recognize
        # the "end line" for the block if it
        # didn't end in "\n" (as in, the last)
        # byte of the file was '/'.
        # so it would spit out an end line for you.
        # and since you really already had one,
        # the last line of the block got corrupted.
        raw = "/*[clinic]\nfoo\n[clinic]*/"
        cooked = self.clinic.parse(raw).splitlines()
        end_line = cooked[2].rstrip()
        # this test is redundant, it's just here explicitly to catch
        # the regression test so we don't forget what it looked like
        self.assertNotEqual(end_line, "[clinic]*/[clinic]*/")
        self.assertEqual(end_line, "[clinic]*/")

    def test_mangled_marker_line(self):
        raw = """
            /*[clinic input]
            [clinic start generated code]*/
            /*[clinic end generated code: foo]*/
        """
        msg = (
            'Error in file "test.c" on line 3:\n'
            "Mangled Argument Clinic marker line: '/*[clinic end generated code: foo]*/'\n"
        )
        out = self.expect_failure(raw)
        self.assertEqual(out, msg)

    def test_checksum_mismatch(self):
        raw = """
            /*[clinic input]
            [clinic start generated code]*/
            /*[clinic end generated code: output=0123456789abcdef input=fedcba9876543210]*/
        """
        msg = (
            'Error in file "test.c" on line 3:\n'
            'Checksum mismatch!\n'
            'Expected: 0123456789abcdef\n'
            'Computed: da39a3ee5e6b4b0d\n'
        )
        out = self.expect_failure(raw)
        self.assertIn(msg, out)

    def test_garbage_after_stop_line(self):
        raw = """
            /*[clinic input]
            [clinic start generated code]*/foobarfoobar!
        """
        msg = (
            'Error in file "test.c" on line 2:\n'
            "Garbage after stop line: 'foobarfoobar!'\n"
        )
        out = self.expect_failure(raw)
        self.assertEqual(out, msg)

    def test_whitespace_before_stop_line(self):
        raw = """
            /*[clinic input]
             [clinic start generated code]*/
        """
        msg = (
            'Error in file "test.c" on line 2:\n'
            "Whitespace is not allowed before the stop line: ' [clinic start generated code]*/'\n"
        )
        out = self.expect_failure(raw)
        self.assertEqual(out, msg)

    def test_parse_with_body_prefix(self):
        clang = clinic.CLanguage(None)
        clang.body_prefix = "//"
        clang.start_line = "//[{dsl_name} start]"
        clang.stop_line = "//[{dsl_name} stop]"
        cl = clinic.Clinic(clang, filename="test.c")
        raw = dedent("""
            //[clinic start]
            //module test
            //[clinic stop]
        """).strip()
        out = cl.parse(raw)
        expected = dedent("""
            //[clinic start]
            //module test
            //
            //[clinic stop]
            /*[clinic end generated code: output=da39a3ee5e6b4b0d input=65fab8adff58cf08]*/
        """).lstrip()  # Note, lstrip() because of the newline
        self.assertEqual(out, expected)

    def test_cpp_monitor_fail_nested_block_comment(self):
        raw = """
            /* start
            /* nested
            */
            */
        """
        msg = (
            'Error in file "test.c" on line 2:\n'
            'Nested block comment!\n'
        )
        out = self.expect_failure(raw)
        self.assertEqual(out, msg)

    def test_cpp_monitor_fail_invalid_format_noarg(self):
        raw = """
            #if
            a()
            #endif
        """
        msg = (
            'Error in file "test.c" on line 1:\n'
            'Invalid format for #if line: no argument!\n'
        )
        out = self.expect_failure(raw)
        self.assertEqual(out, msg)

    def test_cpp_monitor_fail_invalid_format_toomanyargs(self):
        raw = """
            #ifdef A B
            a()
            #endif
        """
        msg = (
            'Error in file "test.c" on line 1:\n'
            'Invalid format for #ifdef line: should be exactly one argument!\n'
        )
        out = self.expect_failure(raw)
        self.assertEqual(out, msg)

    def test_cpp_monitor_fail_no_matching_if(self):
        raw = '#else'
        msg = (
            'Error in file "test.c" on line 1:\n'
            '#else without matching #if / #ifdef / #ifndef!\n'
        )
        out = self.expect_failure(raw)
        self.assertEqual(out, msg)

    def test_directive_output_unknown_preset(self):
        out = self.expect_failure("""
            /*[clinic input]
            output preset nosuchpreset
            [clinic start generated code]*/
        """)
        msg = "Unknown preset 'nosuchpreset'"
        self.assertIn(msg, out)

    def test_directive_output_cant_pop(self):
        out = self.expect_failure("""
            /*[clinic input]
            output pop
            [clinic start generated code]*/
        """)
        msg = "Can't 'output pop', stack is empty"
        self.assertIn(msg, out)

    def test_directive_output_print(self):
        raw = dedent("""
            /*[clinic input]
            output print 'I told you once.'
            [clinic start generated code]*/
        """)
        out = self.clinic.parse(raw)
        # The generated output will differ for every run, but we can check that
        # it starts with the clinic block, we check that it contains all the
        # expected fields, and we check that it contains the checksum line.
        self.assertTrue(out.startswith(dedent("""
            /*[clinic input]
            output print 'I told you once.'
            [clinic start generated code]*/
        """)))
        fields = {
            "cpp_endif",
            "cpp_if",
            "docstring_definition",
            "docstring_prototype",
            "impl_definition",
            "impl_prototype",
            "methoddef_define",
            "methoddef_ifndef",
            "parser_definition",
            "parser_prototype",
        }
        for field in fields:
            with self.subTest(field=field):
                self.assertIn(field, out)
        last_line = out.rstrip().split("\n")[-1]
        self.assertTrue(
            last_line.startswith("/*[clinic end generated code: output=")
        )

    def test_unknown_destination_command(self):
        out = self.expect_failure("""
            /*[clinic input]
            destination buffer nosuchcommand
            [clinic start generated code]*/
        """)
        msg = "unknown destination command 'nosuchcommand'"
        self.assertIn(msg, out)

    def test_no_access_to_members_in_converter_init(self):
        out = self.expect_failure("""
            /*[python input]
            class Custom_converter(CConverter):
                converter = "some_c_function"
                def converter_init(self):
                    self.function.noaccess
            [python start generated code]*/
            /*[clinic input]
            module test
            test.fn
                a: Custom
            [clinic start generated code]*/
        """)
        msg = (
            "Stepped on a land mine, trying to access attribute 'noaccess':\n"
            "Don't access members of self.function inside converter_init!"
        )
        self.assertIn(msg, out)


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


class ClinicParserTest(_ParserBase):
    def checkDocstring(self, fn, expected):
        self.assertTrue(hasattr(fn, "docstring"))
        self.assertEqual(fn.docstring.strip(),
                         dedent(expected).strip())

    def test_trivial(self):
        parser = DSLParser(FakeClinic())
        block = clinic.Block("""
            module os
            os.access
        """)
        parser.parse(block)
        module, function = block.signatures
        self.assertEqual("access", function.name)
        self.assertEqual("os", module.name)

    def test_ignore_line(self):
        block = self.parse(dedent("""
            #
            module os
            os.access
        """))
        module, function = block.signatures
        self.assertEqual("access", function.name)
        self.assertEqual("os", module.name)

    def test_param(self):
        function = self.parse_function("""
            module os
            os.access
                path: int
        """)
        self.assertEqual("access", function.name)
        self.assertEqual(2, len(function.parameters))
        p = function.parameters['path']
        self.assertEqual('path', p.name)
        self.assertIsInstance(p.converter, clinic.int_converter)

    def test_param_default(self):
        function = self.parse_function("""
            module os
            os.access
                follow_symlinks: bool = True
        """)
        p = function.parameters['follow_symlinks']
        self.assertEqual(True, p.default)

    def test_param_with_continuations(self):
        function = self.parse_function(r"""
            module os
            os.access
                follow_symlinks: \
                bool \
                = \
                True
        """)
        p = function.parameters['follow_symlinks']
        self.assertEqual(True, p.default)

    def test_param_default_expression(self):
        function = self.parse_function("""
            module os
            os.access
                follow_symlinks: int(c_default='MAXSIZE') = sys.maxsize
            """)
        p = function.parameters['follow_symlinks']
        self.assertEqual(sys.maxsize, p.default)
        self.assertEqual("MAXSIZE", p.converter.c_default)

        expected_msg = (
            "Error on line 0:\n"
            "When you specify a named constant ('sys.maxsize') as your default value,\n"
            "you MUST specify a valid c_default.\n"
        )
        out = self.parse_function_should_fail("""
            module os
            os.access
                follow_symlinks: int = sys.maxsize
        """)
        self.assertEqual(out, expected_msg)

    def test_param_no_docstring(self):
        function = self.parse_function("""
            module os
            os.access
                follow_symlinks: bool = True
                something_else: str = ''
        """)
        self.assertEqual(3, len(function.parameters))
        conv = function.parameters['something_else'].converter
        self.assertIsInstance(conv, clinic.str_converter)

    def test_param_default_parameters_out_of_order(self):
        expected_msg = (
            "Error on line 0:\n"
            "Can't have a parameter without a default ('something_else')\n"
            "after a parameter with a default!\n"
        )
        out = self.parse_function_should_fail("""
            module os
            os.access
                follow_symlinks: bool = True
                something_else: str""")
        self.assertEqual(out, expected_msg)

    def disabled_test_converter_arguments(self):
        function = self.parse_function("""
            module os
            os.access
                path: path_t(allow_fd=1)
        """)
        p = function.parameters['path']
        self.assertEqual(1, p.converter.args['allow_fd'])

    def test_function_docstring(self):
        function = self.parse_function("""
            module os
            os.stat as os_stat_fn

               path: str
                   Path to be examined

            Perform a stat system call on the given path.
        """)
        self.checkDocstring(function, """
            stat($module, /, path)
            --

            Perform a stat system call on the given path.

              path
                Path to be examined
        """)

    def test_explicit_parameters_in_docstring(self):
        function = self.parse_function(dedent("""
            module foo
            foo.bar
              x: int
                 Documentation for x.
              y: int

            This is the documentation for foo.

            Okay, we're done here.
        """))
        self.checkDocstring(function, """
            bar($module, /, x, y)
            --

            This is the documentation for foo.

              x
                Documentation for x.

            Okay, we're done here.
        """)

    def test_parser_regression_special_character_in_parameter_column_of_docstring_first_line(self):
        function = self.parse_function(dedent("""
            module os
            os.stat
                path: str
            This/used to break Clinic!
        """))
        self.checkDocstring(function, """
            stat($module, /, path)
            --

            This/used to break Clinic!
        """)

    def test_c_name(self):
        function = self.parse_function("""
            module os
            os.stat as os_stat_fn
        """)
        self.assertEqual("os_stat_fn", function.c_basename)

    def test_return_converter(self):
        function = self.parse_function("""
            module os
            os.stat -> int
        """)
        self.assertIsInstance(function.return_converter, clinic.int_return_converter)

    def test_star(self):
        function = self.parse_function("""
            module os
            os.access
                *
                follow_symlinks: bool = True
        """)
        p = function.parameters['follow_symlinks']
        self.assertEqual(inspect.Parameter.KEYWORD_ONLY, p.kind)
        self.assertEqual(0, p.group)

    def test_group(self):
        function = self.parse_function("""
            module window
            window.border
                [
                ls: int
                ]
                /
        """)
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
        dataset = (
            ('y', -1), ('x', -1),
            ('ch', 0),
            ('attr', 1),
        )
        for name, group in dataset:
            with self.subTest(name=name, group=group):
                p = function.parameters[name]
                self.assertEqual(p.group, group)
                self.assertEqual(p.kind, inspect.Parameter.POSITIONAL_ONLY)
        self.checkDocstring(function, """
            addch([y, x,] ch, [attr])


              y
                Y-coordinate.
              x
                X-coordinate.
              ch
                Character to add.
              attr
                Attributes for the character.
        """)

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
        dataset = (
            ('y1', -2), ('y2', -2),
            ('x1', -1), ('x2', -1),
            ('ch', 0),
            ('attr1', 1), ('attr2', 1), ('attr3', 1),
            ('attr4', 2), ('attr5', 2), ('attr6', 2),
        )
        for name, group in dataset:
            with self.subTest(name=name, group=group):
                p = function.parameters[name]
                self.assertEqual(p.group, group)
                self.assertEqual(p.kind, inspect.Parameter.POSITIONAL_ONLY)

        self.checkDocstring(function, """
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
        """)

    def parse_function_should_fail(self, s):
        with support.captured_stdout() as stdout:
            with self.assertRaises(SystemExit):
                self.parse_function(s)
        return stdout.getvalue()

    def test_disallowed_grouping__two_top_groups_on_left(self):
        expected_msg = (
            'Error on line 0:\n'
            'Function two_top_groups_on_left has an unsupported group '
            'configuration. (Unexpected state 2.b)\n'
        )
        out = self.parse_function_should_fail("""
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
        self.assertEqual(out, expected_msg)

    def test_disallowed_grouping__two_top_groups_on_right(self):
        out = self.parse_function_should_fail("""
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
        msg = (
            "Function two_top_groups_on_right has an unsupported group "
            "configuration. (Unexpected state 6.b)"
        )
        self.assertIn(msg, out)

    def test_disallowed_grouping__parameter_after_group_on_right(self):
        out = self.parse_function_should_fail("""
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
        msg = (
            "Function parameter_after_group_on_right has an unsupported group "
            "configuration. (Unexpected state 6.a)"
        )
        self.assertIn(msg, out)

    def test_disallowed_grouping__group_after_parameter_on_left(self):
        out = self.parse_function_should_fail("""
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
        msg = (
            "Function group_after_parameter_on_left has an unsupported group "
            "configuration. (Unexpected state 2.b)"
        )
        self.assertIn(msg, out)

    def test_disallowed_grouping__empty_group_on_left(self):
        out = self.parse_function_should_fail("""
            module foo
            foo.empty_group
                [
                [
                ]
                group2 : int
                ]
                param: int
        """)
        msg = (
            "Function empty_group has an empty group.\n"
            "All groups must contain at least one parameter."
        )
        self.assertIn(msg, out)

    def test_disallowed_grouping__empty_group_on_right(self):
        out = self.parse_function_should_fail("""
            module foo
            foo.empty_group
                param: int
                [
                [
                ]
                group2 : int
                ]
        """)
        msg = (
            "Function empty_group has an empty group.\n"
            "All groups must contain at least one parameter."
        )
        self.assertIn(msg, out)

    def test_disallowed_grouping__no_matching_bracket(self):
        out = self.parse_function_should_fail("""
            module foo
            foo.empty_group
                param: int
                ]
                group2: int
                ]
        """)
        msg = "Function empty_group has a ] without a matching [."
        self.assertIn(msg, out)

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
        out = self.parse_function_should_fail("""
            module foo
            foo.bar => int
                /
        """)
        msg = "Illegal function name: foo.bar => int"
        self.assertIn(msg, out)

    def test_illegal_c_basename(self):
        out = self.parse_function_should_fail("""
            module foo
            foo.bar as 935
                /
        """)
        msg = "Illegal C basename: 935"
        self.assertIn(msg, out)

    def test_single_star(self):
        out = self.parse_function_should_fail("""
            module foo
            foo.bar
                *
                *
        """)
        self.assertIn("Function bar uses '*' more than once.", out)

    def test_parameters_required_after_star(self):
        dataset = (
            "module foo\nfoo.bar\n  *",
            "module foo\nfoo.bar\n  *\nDocstring here.",
            "module foo\nfoo.bar\n  this: int\n  *",
            "module foo\nfoo.bar\n  this: int\n  *\nDocstring.",
        )
        msg = "Function bar specifies '*' without any parameters afterwards."
        for block in dataset:
            with self.subTest(block=block):
                out = self.parse_function_should_fail(block)
                self.assertIn(msg, out)

    def test_single_slash(self):
        out = self.parse_function_should_fail("""
            module foo
            foo.bar
                /
                /
        """)
        msg = (
            "Function bar has an unsupported group configuration. "
            "(Unexpected state 0.d)"
        )
        self.assertIn(msg, out)

    def test_double_slash(self):
        out = self.parse_function_should_fail("""
            module foo
            foo.bar
                a: int
                /
                b: int
                /
        """)
        msg = "Function bar uses '/' more than once."
        self.assertIn(msg, out)

    def test_mix_star_and_slash(self):
        out = self.parse_function_should_fail("""
            module foo
            foo.bar
               x: int
               y: int
               *
               z: int
               /
        """)
        msg = (
            "Function bar mixes keyword-only and positional-only parameters, "
            "which is unsupported."
        )
        self.assertIn(msg, out)

    def test_parameters_not_permitted_after_slash_for_now(self):
        out = self.parse_function_should_fail("""
            module foo
            foo.bar
                /
                x: int
        """)
        msg = (
            "Function bar has an unsupported group configuration. "
            "(Unexpected state 0.d)"
        )
        self.assertIn(msg, out)

    def test_parameters_no_more_than_one_vararg(self):
        expected_msg = (
            "Error on line 0:\n"
            "Too many var args\n"
        )
        out = self.parse_function_should_fail("""
            module foo
            foo.bar
               *vararg1: object
               *vararg2: object
        """)
        self.assertEqual(out, expected_msg)

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
        self.checkDocstring(function, """
            bar($module, /, x, *, y)
            --

            Not at column 0!

              x
                Nested docstring here, goeth.
        """)

    def test_indent_stack_no_tabs(self):
        out = self.parse_function_should_fail("""
            module foo
            foo.bar
               *vararg1: object
            \t*vararg2: object
        """)
        msg = "Tab characters are illegal in the Clinic DSL."
        self.assertIn(msg, out)

    def test_indent_stack_illegal_outdent(self):
        out = self.parse_function_should_fail("""
            module foo
            foo.bar
              a: object
             b: object
        """)
        self.assertIn("Illegal outdent", out)

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
        conv = (function.parameters['path']).converter
        self.assertIsInstance(conv, clinic.str_converter)

    def test_legacy_converters_non_string_constant_annotation(self):
        expected_failure_message = (
            "Error on line 0:\n"
            "Annotations must be either a name, a function call, or a string.\n"
        )
        dataset = (
            'module os\nos.access\n   path: 42',
            'module os\nos.access\n   path: 42.42',
            'module os\nos.access\n   path: 42j',
            'module os\nos.access\n   path: b"42"',
        )
        for block in dataset:
            with self.subTest(block=block):
                out = self.parse_function_should_fail(block)
                self.assertEqual(out, expected_failure_message)

    def test_other_bizarre_things_in_annotations_fail(self):
        expected_failure_message = (
            "Error on line 0:\n"
            "Annotations must be either a name, a function call, or a string.\n"
        )
        dataset = (
            'module os\nos.access\n   path: {"some": "dictionary"}',
            'module os\nos.access\n   path: ["list", "of", "strings"]',
            'module os\nos.access\n   path: (x for x in range(42))',
        )
        for block in dataset:
            with self.subTest(block=block):
                out = self.parse_function_should_fail(block)
                self.assertEqual(out, expected_failure_message)

    def test_kwarg_splats_disallowed_in_function_call_annotations(self):
        expected_error_msg = (
            "Error on line 0:\n"
            "Cannot use a kwarg splat in a function-call annotation\n"
        )
        dataset = (
            'module fo\nfo.barbaz\n   o: bool(**{None: "bang!"})',
            'module fo\nfo.barbaz -> bool(**{None: "bang!"})',
            'module fo\nfo.barbaz -> bool(**{"bang": 42})',
            'module fo\nfo.barbaz\n   o: bool(**{"bang": None})',
        )
        for fn in dataset:
            with self.subTest(fn=fn):
                out = self.parse_function_should_fail(fn)
                self.assertEqual(out, expected_error_msg)

    def test_self_param_placement(self):
        expected_error_msg = (
            "Error on line 0:\n"
            "A 'self' parameter, if specified, must be the very first thing "
            "in the parameter block.\n"
        )
        block = """
            module foo
            foo.func
                a: int
                self: self(type="PyObject *")
        """
        out = self.parse_function_should_fail(block)
        self.assertEqual(out, expected_error_msg)

    def test_self_param_cannot_be_optional(self):
        expected_error_msg = (
            "Error on line 0:\n"
            "A 'self' parameter cannot be marked optional.\n"
        )
        block = """
            module foo
            foo.func
                self: self(type="PyObject *") = None
        """
        out = self.parse_function_should_fail(block)
        self.assertEqual(out, expected_error_msg)

    def test_defining_class_param_placement(self):
        expected_error_msg = (
            "Error on line 0:\n"
            "A 'defining_class' parameter, if specified, must either be the "
            "first thing in the parameter block, or come just after 'self'.\n"
        )
        block = """
            module foo
            foo.func
                self: self(type="PyObject *")
                a: int
                cls: defining_class
        """
        out = self.parse_function_should_fail(block)
        self.assertEqual(out, expected_error_msg)

    def test_defining_class_param_cannot_be_optional(self):
        expected_error_msg = (
            "Error on line 0:\n"
            "A 'defining_class' parameter cannot be marked optional.\n"
        )
        block = """
            module foo
            foo.func
                cls: defining_class(type="PyObject *") = None
        """
        out = self.parse_function_should_fail(block)
        self.assertEqual(out, expected_error_msg)

    def test_slot_methods_cannot_access_defining_class(self):
        block = """
            module foo
            class Foo "" ""
            Foo.__init__
                cls: defining_class
                a: object
        """
        msg = "Slot methods cannot access their defining class."
        with self.assertRaisesRegex(ValueError, msg):
            self.parse_function(block)

    def test_new_must_be_a_class_method(self):
        expected_error_msg = (
            "Error on line 0:\n"
            "__new__ must be a class method!\n"
        )
        out = self.parse_function_should_fail("""
            module foo
            class Foo "" ""
            Foo.__new__
        """)
        self.assertEqual(out, expected_error_msg)

    def test_init_must_be_a_normal_method(self):
        expected_error_msg = (
            "Error on line 0:\n"
            "__init__ must be a normal method, not a class or static method!\n"
        )
        out = self.parse_function_should_fail("""
            module foo
            class Foo "" ""
            @classmethod
            Foo.__init__
        """)
        self.assertEqual(out, expected_error_msg)

    def test_unused_param(self):
        block = self.parse("""
            module foo
            foo.func
                fn: object
                k: float
                i: float(unused=True)
                /
                *
                flag: bool(unused=True) = False
        """)
        sig = block.signatures[1]  # Function index == 1
        params = sig.parameters
        conv = lambda fn: params[fn].converter
        dataset = (
            {"name": "fn", "unused": False},
            {"name": "k", "unused": False},
            {"name": "i", "unused": True},
            {"name": "flag", "unused": True},
        )
        for param in dataset:
            name, unused = param.values()
            with self.subTest(name=name, unused=unused):
                p = conv(name)
                # Verify that the unused flag is parsed correctly.
                self.assertEqual(unused, p.unused)

                # Now, check that we'll produce correct code.
                decl = p.simple_declaration(in_parser=False)
                if unused:
                    self.assertIn("Py_UNUSED", decl)
                else:
                    self.assertNotIn("Py_UNUSED", decl)

                # Make sure the Py_UNUSED macro is not used in the parser body.
                parser_decl = p.simple_declaration(in_parser=True)
                self.assertNotIn("Py_UNUSED", parser_decl)

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
        expected = (
            'Error in file "clown.txt" on line 69:\n'
            'The igloos are melting!\n'
        )
        with support.captured_stdout() as stdout:
            with self.assertRaises(SystemExit):
                clinic.fail('The igloos are melting!',
                            filename='clown.txt', line_number=69)
        actual = stdout.getvalue()
        self.assertEqual(actual, expected)


class ClinicExternalTest(TestCase):
    maxDiff = None
    clinic_py = os.path.join(test_tools.toolsdir, "clinic", "clinic.py")

    def setUp(self):
        save_restore_converters(self)

    def _do_test(self, *args, expect_success=True):
        with subprocess.Popen(
            [sys.executable, "-Xutf8", self.clinic_py, *args],
            encoding="utf-8",
            bufsize=0,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        ) as proc:
            proc.wait()
            if expect_success and proc.returncode:
                self.fail("".join([*proc.stdout, *proc.stderr]))
            stdout = proc.stdout.read()
            stderr = proc.stderr.read()
            # Clinic never writes to stderr.
            self.assertEqual(stderr, "")
            return stdout

    def expect_success(self, *args):
        return self._do_test(*args)

    def expect_failure(self, *args):
        return self._do_test(*args, expect_success=False)

    def test_external(self):
        CLINIC_TEST = 'clinic.test.c'
        source = support.findfile(CLINIC_TEST)
        with open(source, encoding='utf-8') as f:
            orig_contents = f.read()

        # Run clinic CLI and verify that it does not complain.
        self.addCleanup(unlink, TESTFN)
        out = self.expect_success("-f", "-o", TESTFN, source)
        self.assertEqual(out, "")

        with open(TESTFN, encoding='utf-8') as f:
            new_contents = f.read()

        self.assertEqual(new_contents, orig_contents)

    def test_no_change(self):
        # bpo-42398: Test that the destination file is left unchanged if the
        # content does not change. Moreover, check also that the file
        # modification time does not change in this case.
        code = dedent("""
            /*[clinic input]
            [clinic start generated code]*/
            /*[clinic end generated code: output=da39a3ee5e6b4b0d input=da39a3ee5e6b4b0d]*/
        """)
        with os_helper.temp_dir() as tmp_dir:
            fn = os.path.join(tmp_dir, "test.c")
            with open(fn, "w", encoding="utf-8") as f:
                f.write(code)
            pre_mtime = os.stat(fn).st_mtime_ns
            self.expect_success(fn)
            post_mtime = os.stat(fn).st_mtime_ns
        # Don't change the file modification time
        # if the content does not change
        self.assertEqual(pre_mtime, post_mtime)

    def test_cli_force(self):
        invalid_input = dedent("""
            /*[clinic input]
            output preset block
            module test
            test.fn
                a: int
            [clinic start generated code]*/

            const char *hand_edited = "output block is overwritten";
            /*[clinic end generated code: output=bogus input=bogus]*/
        """)
        fail_msg = dedent("""
            Checksum mismatch!
            Expected: bogus
            Computed: 2ed19
            Suggested fix: remove all generated code including the end marker,
            or use the '-f' option.
        """)
        with os_helper.temp_dir() as tmp_dir:
            fn = os.path.join(tmp_dir, "test.c")
            with open(fn, "w", encoding="utf-8") as f:
                f.write(invalid_input)
            # First, run the CLI without -f and expect failure.
            # Note, we cannot check the entire fail msg, because the path to
            # the tmp file will change for every run.
            out = self.expect_failure(fn)
            self.assertTrue(out.endswith(fail_msg))
            # Then, force regeneration; success expected.
            out = self.expect_success("-f", fn)
            self.assertEqual(out, "")
            # Verify by checking the checksum.
            checksum = (
                "/*[clinic end generated code: "
                "output=2124c291eb067d76 input=9543a8d2da235301]*/\n"
            )
            with open(fn, encoding='utf-8') as f:
                generated = f.read()
            self.assertTrue(generated.endswith(checksum))

    def test_cli_make(self):
        c_code = dedent("""
            /*[clinic input]
            [clinic start generated code]*/
        """)
        py_code = "pass"
        c_files = "file1.c", "file2.c"
        py_files = "file1.py", "file2.py"

        def create_files(files, srcdir, code):
            for fn in files:
                path = os.path.join(srcdir, fn)
                with open(path, "w", encoding="utf-8") as f:
                    f.write(code)

        with os_helper.temp_dir() as tmp_dir:
            # add some folders, some C files and a Python file
            create_files(c_files, tmp_dir, c_code)
            create_files(py_files, tmp_dir, py_code)

            # create C files in externals/ dir
            ext_path = os.path.join(tmp_dir, "externals")
            with os_helper.temp_dir(path=ext_path) as externals:
                create_files(c_files, externals, c_code)

                # run clinic in verbose mode with --make on tmpdir
                out = self.expect_success("-v", "--make", "--srcdir", tmp_dir)

            # expect verbose mode to only mention the C files in tmp_dir
            for filename in c_files:
                with self.subTest(filename=filename):
                    path = os.path.join(tmp_dir, filename)
                    self.assertIn(path, out)
            for filename in py_files:
                with self.subTest(filename=filename):
                    path = os.path.join(tmp_dir, filename)
                    self.assertNotIn(path, out)
            # don't expect C files from the externals dir
            for filename in c_files:
                with self.subTest(filename=filename):
                    path = os.path.join(ext_path, filename)
                    self.assertNotIn(path, out)

    def test_cli_verbose(self):
        with os_helper.temp_dir() as tmp_dir:
            fn = os.path.join(tmp_dir, "test.c")
            with open(fn, "w", encoding="utf-8") as f:
                f.write("")
            out = self.expect_success("-v", fn)
            self.assertEqual(out.strip(), fn)

    def test_cli_help(self):
        out = self.expect_success("-h")
        self.assertIn("usage: clinic.py", out)

    def test_cli_converters(self):
        prelude = dedent("""
            Legacy converters:
                B C D L O S U Y Z Z#
                b c d f h i l p s s# s* u u# w* y y# y* z z# z*

            Converters:
        """)
        expected_converters = (
            "bool",
            "byte",
            "char",
            "defining_class",
            "double",
            "fildes",
            "float",
            "int",
            "long",
            "long_long",
            "object",
            "Py_buffer",
            "Py_complex",
            "Py_ssize_t",
            "Py_UNICODE",
            "PyByteArrayObject",
            "PyBytesObject",
            "self",
            "short",
            "size_t",
            "slice_index",
            "str",
            "unicode",
            "unsigned_char",
            "unsigned_int",
            "unsigned_long",
            "unsigned_long_long",
            "unsigned_short",
        )
        finale = dedent("""
            Return converters:
                bool()
                double()
                float()
                init()
                int()
                long()
                Py_ssize_t()
                size_t()
                unsigned_int()
                unsigned_long()

            All converters also accept (c_default=None, py_default=None, annotation=None).
            All return converters also accept (py_default=None).
        """)
        out = self.expect_success("--converters")
        # We cannot simply compare the output, because the repr of the *accept*
        # param may change (it's a set, thus unordered). So, let's compare the
        # start and end of the expected output, and then assert that the
        # converters appear lined up in alphabetical order.
        self.assertTrue(out.startswith(prelude), out)
        self.assertTrue(out.endswith(finale), out)

        out = out.removeprefix(prelude)
        out = out.removesuffix(finale)
        lines = out.split("\n")
        for converter, line in zip(expected_converters, lines):
            line = line.lstrip()
            with self.subTest(converter=converter):
                self.assertTrue(
                    line.startswith(converter),
                    f"expected converter {converter!r}, got {line!r}"
                )

    def test_cli_fail_converters_and_filename(self):
        out = self.expect_failure("--converters", "test.c")
        msg = (
            "Usage error: can't specify --converters "
            "and a filename at the same time"
        )
        self.assertIn(msg, out)

    def test_cli_fail_no_filename(self):
        out = self.expect_failure()
        self.assertIn("usage: clinic.py", out)

    def test_cli_fail_output_and_multiple_files(self):
        out = self.expect_failure("-o", "out.c", "input.c", "moreinput.c")
        msg = "Usage error: can't use -o with multiple filenames"
        self.assertIn(msg, out)

    def test_cli_fail_filename_or_output_and_make(self):
        for opts in ("-o", "out.c"), ("filename.c",):
            with self.subTest(opts=opts):
                out = self.expect_failure("--make", *opts)
                msg = "Usage error: can't use -o or filenames with --make"
                self.assertIn(msg, out)

    def test_cli_fail_make_without_srcdir(self):
        out = self.expect_failure("--make", "--srcdir", "")
        msg = "Usage error: --srcdir must not be empty with --make"
        self.assertIn(msg, out)


try:
    import _testclinic as ac_tester
except ImportError:
    ac_tester = None

@unittest.skipIf(ac_tester is None, "_testclinic is missing")
class ClinicFunctionalTest(unittest.TestCase):
    locals().update((name, getattr(ac_tester, name))
                    for name in dir(ac_tester) if name.startswith('test_'))

    def test_objects_converter(self):
        with self.assertRaises(TypeError):
            ac_tester.objects_converter()
        self.assertEqual(ac_tester.objects_converter(1, 2), (1, 2))
        self.assertEqual(ac_tester.objects_converter([], 'whatever class'), ([], 'whatever class'))
        self.assertEqual(ac_tester.objects_converter(1), (1, None))

    def test_bytes_object_converter(self):
        with self.assertRaises(TypeError):
            ac_tester.bytes_object_converter(1)
        self.assertEqual(ac_tester.bytes_object_converter(b'BytesObject'), (b'BytesObject',))

    def test_byte_array_object_converter(self):
        with self.assertRaises(TypeError):
            ac_tester.byte_array_object_converter(1)
        byte_arr = bytearray(b'ByteArrayObject')
        self.assertEqual(ac_tester.byte_array_object_converter(byte_arr), (byte_arr,))

    def test_unicode_converter(self):
        with self.assertRaises(TypeError):
            ac_tester.unicode_converter(1)
        self.assertEqual(ac_tester.unicode_converter('unicode'), ('unicode',))

    def test_bool_converter(self):
        with self.assertRaises(TypeError):
            ac_tester.bool_converter(False, False, 'not a int')
        self.assertEqual(ac_tester.bool_converter(), (True, True, True))
        self.assertEqual(ac_tester.bool_converter('', [], 5), (False, False, True))
        self.assertEqual(ac_tester.bool_converter(('not empty',), {1: 2}, 0), (True, True, False))

    def test_char_converter(self):
        with self.assertRaises(TypeError):
            ac_tester.char_converter(1)
        with self.assertRaises(TypeError):
            ac_tester.char_converter(b'ab')
        chars = [b'A', b'\a', b'\b', b'\t', b'\n', b'\v', b'\f', b'\r', b'"', b"'", b'?', b'\\', b'\000', b'\377']
        expected = tuple(ord(c) for c in chars)
        self.assertEqual(ac_tester.char_converter(), expected)
        chars = [b'1', b'2', b'3', b'4', b'5', b'6', b'7', b'8', b'9', b'0', b'a', b'b', b'c', b'd']
        expected = tuple(ord(c) for c in chars)
        self.assertEqual(ac_tester.char_converter(*chars), expected)

    def test_unsigned_char_converter(self):
        from _testcapi import UCHAR_MAX
        with self.assertRaises(OverflowError):
            ac_tester.unsigned_char_converter(-1)
        with self.assertRaises(OverflowError):
            ac_tester.unsigned_char_converter(UCHAR_MAX + 1)
        with self.assertRaises(OverflowError):
            ac_tester.unsigned_char_converter(0, UCHAR_MAX + 1)
        with self.assertRaises(TypeError):
            ac_tester.unsigned_char_converter([])
        self.assertEqual(ac_tester.unsigned_char_converter(), (12, 34, 56))
        self.assertEqual(ac_tester.unsigned_char_converter(0, 0, UCHAR_MAX + 1), (0, 0, 0))
        self.assertEqual(ac_tester.unsigned_char_converter(0, 0, (UCHAR_MAX + 1) * 3 + 123), (0, 0, 123))

    def test_short_converter(self):
        from _testcapi import SHRT_MIN, SHRT_MAX
        with self.assertRaises(OverflowError):
            ac_tester.short_converter(SHRT_MIN - 1)
        with self.assertRaises(OverflowError):
            ac_tester.short_converter(SHRT_MAX + 1)
        with self.assertRaises(TypeError):
            ac_tester.short_converter([])
        self.assertEqual(ac_tester.short_converter(-1234), (-1234,))
        self.assertEqual(ac_tester.short_converter(4321), (4321,))

    def test_unsigned_short_converter(self):
        from _testcapi import USHRT_MAX
        with self.assertRaises(ValueError):
            ac_tester.unsigned_short_converter(-1)
        with self.assertRaises(OverflowError):
            ac_tester.unsigned_short_converter(USHRT_MAX + 1)
        with self.assertRaises(OverflowError):
            ac_tester.unsigned_short_converter(0, USHRT_MAX + 1)
        with self.assertRaises(TypeError):
            ac_tester.unsigned_short_converter([])
        self.assertEqual(ac_tester.unsigned_short_converter(), (12, 34, 56))
        self.assertEqual(ac_tester.unsigned_short_converter(0, 0, USHRT_MAX + 1), (0, 0, 0))
        self.assertEqual(ac_tester.unsigned_short_converter(0, 0, (USHRT_MAX + 1) * 3 + 123), (0, 0, 123))

    def test_int_converter(self):
        from _testcapi import INT_MIN, INT_MAX
        with self.assertRaises(OverflowError):
            ac_tester.int_converter(INT_MIN - 1)
        with self.assertRaises(OverflowError):
            ac_tester.int_converter(INT_MAX + 1)
        with self.assertRaises(TypeError):
            ac_tester.int_converter(1, 2, 3)
        with self.assertRaises(TypeError):
            ac_tester.int_converter([])
        self.assertEqual(ac_tester.int_converter(), (12, 34, 45))
        self.assertEqual(ac_tester.int_converter(1, 2, '3'), (1, 2, ord('3')))

    def test_unsigned_int_converter(self):
        from _testcapi import UINT_MAX
        with self.assertRaises(ValueError):
            ac_tester.unsigned_int_converter(-1)
        with self.assertRaises(OverflowError):
            ac_tester.unsigned_int_converter(UINT_MAX + 1)
        with self.assertRaises(OverflowError):
            ac_tester.unsigned_int_converter(0, UINT_MAX + 1)
        with self.assertRaises(TypeError):
            ac_tester.unsigned_int_converter([])
        self.assertEqual(ac_tester.unsigned_int_converter(), (12, 34, 56))
        self.assertEqual(ac_tester.unsigned_int_converter(0, 0, UINT_MAX + 1), (0, 0, 0))
        self.assertEqual(ac_tester.unsigned_int_converter(0, 0, (UINT_MAX + 1) * 3 + 123), (0, 0, 123))

    def test_long_converter(self):
        from _testcapi import LONG_MIN, LONG_MAX
        with self.assertRaises(OverflowError):
            ac_tester.long_converter(LONG_MIN - 1)
        with self.assertRaises(OverflowError):
            ac_tester.long_converter(LONG_MAX + 1)
        with self.assertRaises(TypeError):
            ac_tester.long_converter([])
        self.assertEqual(ac_tester.long_converter(), (12,))
        self.assertEqual(ac_tester.long_converter(-1234), (-1234,))

    def test_unsigned_long_converter(self):
        from _testcapi import ULONG_MAX
        with self.assertRaises(ValueError):
            ac_tester.unsigned_long_converter(-1)
        with self.assertRaises(OverflowError):
            ac_tester.unsigned_long_converter(ULONG_MAX + 1)
        with self.assertRaises(OverflowError):
            ac_tester.unsigned_long_converter(0, ULONG_MAX + 1)
        with self.assertRaises(TypeError):
            ac_tester.unsigned_long_converter([])
        self.assertEqual(ac_tester.unsigned_long_converter(), (12, 34, 56))
        self.assertEqual(ac_tester.unsigned_long_converter(0, 0, ULONG_MAX + 1), (0, 0, 0))
        self.assertEqual(ac_tester.unsigned_long_converter(0, 0, (ULONG_MAX + 1) * 3 + 123), (0, 0, 123))

    def test_long_long_converter(self):
        from _testcapi import LLONG_MIN, LLONG_MAX
        with self.assertRaises(OverflowError):
            ac_tester.long_long_converter(LLONG_MIN - 1)
        with self.assertRaises(OverflowError):
            ac_tester.long_long_converter(LLONG_MAX + 1)
        with self.assertRaises(TypeError):
            ac_tester.long_long_converter([])
        self.assertEqual(ac_tester.long_long_converter(), (12,))
        self.assertEqual(ac_tester.long_long_converter(-1234), (-1234,))

    def test_unsigned_long_long_converter(self):
        from _testcapi import ULLONG_MAX
        with self.assertRaises(ValueError):
            ac_tester.unsigned_long_long_converter(-1)
        with self.assertRaises(OverflowError):
            ac_tester.unsigned_long_long_converter(ULLONG_MAX + 1)
        with self.assertRaises(OverflowError):
            ac_tester.unsigned_long_long_converter(0, ULLONG_MAX + 1)
        with self.assertRaises(TypeError):
            ac_tester.unsigned_long_long_converter([])
        self.assertEqual(ac_tester.unsigned_long_long_converter(), (12, 34, 56))
        self.assertEqual(ac_tester.unsigned_long_long_converter(0, 0, ULLONG_MAX + 1), (0, 0, 0))
        self.assertEqual(ac_tester.unsigned_long_long_converter(0, 0, (ULLONG_MAX + 1) * 3 + 123), (0, 0, 123))

    def test_py_ssize_t_converter(self):
        from _testcapi import PY_SSIZE_T_MIN, PY_SSIZE_T_MAX
        with self.assertRaises(OverflowError):
            ac_tester.py_ssize_t_converter(PY_SSIZE_T_MIN - 1)
        with self.assertRaises(OverflowError):
            ac_tester.py_ssize_t_converter(PY_SSIZE_T_MAX + 1)
        with self.assertRaises(TypeError):
            ac_tester.py_ssize_t_converter([])
        self.assertEqual(ac_tester.py_ssize_t_converter(), (12, 34, 56))
        self.assertEqual(ac_tester.py_ssize_t_converter(1, 2, None), (1, 2, 56))

    def test_slice_index_converter(self):
        from _testcapi import PY_SSIZE_T_MIN, PY_SSIZE_T_MAX
        with self.assertRaises(TypeError):
            ac_tester.slice_index_converter([])
        self.assertEqual(ac_tester.slice_index_converter(), (12, 34, 56))
        self.assertEqual(ac_tester.slice_index_converter(1, 2, None), (1, 2, 56))
        self.assertEqual(ac_tester.slice_index_converter(PY_SSIZE_T_MAX, PY_SSIZE_T_MAX + 1, PY_SSIZE_T_MAX + 1234),
                         (PY_SSIZE_T_MAX, PY_SSIZE_T_MAX, PY_SSIZE_T_MAX))
        self.assertEqual(ac_tester.slice_index_converter(PY_SSIZE_T_MIN, PY_SSIZE_T_MIN - 1, PY_SSIZE_T_MIN - 1234),
                         (PY_SSIZE_T_MIN, PY_SSIZE_T_MIN, PY_SSIZE_T_MIN))

    def test_size_t_converter(self):
        with self.assertRaises(ValueError):
            ac_tester.size_t_converter(-1)
        with self.assertRaises(TypeError):
            ac_tester.size_t_converter([])
        self.assertEqual(ac_tester.size_t_converter(), (12,))

    def test_float_converter(self):
        with self.assertRaises(TypeError):
            ac_tester.float_converter([])
        self.assertEqual(ac_tester.float_converter(), (12.5,))
        self.assertEqual(ac_tester.float_converter(-0.5), (-0.5,))

    def test_double_converter(self):
        with self.assertRaises(TypeError):
            ac_tester.double_converter([])
        self.assertEqual(ac_tester.double_converter(), (12.5,))
        self.assertEqual(ac_tester.double_converter(-0.5), (-0.5,))

    def test_py_complex_converter(self):
        with self.assertRaises(TypeError):
            ac_tester.py_complex_converter([])
        self.assertEqual(ac_tester.py_complex_converter(complex(1, 2)), (complex(1, 2),))
        self.assertEqual(ac_tester.py_complex_converter(complex('-1-2j')), (complex('-1-2j'),))
        self.assertEqual(ac_tester.py_complex_converter(-0.5), (-0.5,))
        self.assertEqual(ac_tester.py_complex_converter(10), (10,))

    def test_str_converter(self):
        with self.assertRaises(TypeError):
            ac_tester.str_converter(1)
        with self.assertRaises(TypeError):
            ac_tester.str_converter('a', 'b', 'c')
        with self.assertRaises(ValueError):
            ac_tester.str_converter('a', b'b\0b', 'c')
        self.assertEqual(ac_tester.str_converter('a', b'b', 'c'), ('a', 'b', 'c'))
        self.assertEqual(ac_tester.str_converter('a', b'b', b'c'), ('a', 'b', 'c'))
        self.assertEqual(ac_tester.str_converter('a', b'b', 'c\0c'), ('a', 'b', 'c\0c'))

    def test_str_converter_encoding(self):
        with self.assertRaises(TypeError):
            ac_tester.str_converter_encoding(1)
        self.assertEqual(ac_tester.str_converter_encoding('a', 'b', 'c'), ('a', 'b', 'c'))
        with self.assertRaises(TypeError):
            ac_tester.str_converter_encoding('a', b'b\0b', 'c')
        self.assertEqual(ac_tester.str_converter_encoding('a', b'b', bytearray([ord('c')])), ('a', 'b', 'c'))
        self.assertEqual(ac_tester.str_converter_encoding('a', b'b', bytearray([ord('c'), 0, ord('c')])),
                         ('a', 'b', 'c\x00c'))
        self.assertEqual(ac_tester.str_converter_encoding('a', b'b', b'c\x00c'), ('a', 'b', 'c\x00c'))

    def test_py_buffer_converter(self):
        with self.assertRaises(TypeError):
            ac_tester.py_buffer_converter('a', 'b')
        self.assertEqual(ac_tester.py_buffer_converter('abc', bytearray([1, 2, 3])), (b'abc', b'\x01\x02\x03'))

    def test_keywords(self):
        self.assertEqual(ac_tester.keywords(1, 2), (1, 2))
        self.assertEqual(ac_tester.keywords(1, b=2), (1, 2))
        self.assertEqual(ac_tester.keywords(a=1, b=2), (1, 2))

    def test_keywords_kwonly(self):
        with self.assertRaises(TypeError):
            ac_tester.keywords_kwonly(1, 2)
        self.assertEqual(ac_tester.keywords_kwonly(1, b=2), (1, 2))
        self.assertEqual(ac_tester.keywords_kwonly(a=1, b=2), (1, 2))

    def test_keywords_opt(self):
        self.assertEqual(ac_tester.keywords_opt(1), (1, None, None))
        self.assertEqual(ac_tester.keywords_opt(1, 2), (1, 2, None))
        self.assertEqual(ac_tester.keywords_opt(1, 2, 3), (1, 2, 3))
        self.assertEqual(ac_tester.keywords_opt(1, b=2), (1, 2, None))
        self.assertEqual(ac_tester.keywords_opt(1, 2, c=3), (1, 2, 3))
        self.assertEqual(ac_tester.keywords_opt(a=1, c=3), (1, None, 3))
        self.assertEqual(ac_tester.keywords_opt(a=1, b=2, c=3), (1, 2, 3))

    def test_keywords_opt_kwonly(self):
        self.assertEqual(ac_tester.keywords_opt_kwonly(1), (1, None, None, None))
        self.assertEqual(ac_tester.keywords_opt_kwonly(1, 2), (1, 2, None, None))
        with self.assertRaises(TypeError):
            ac_tester.keywords_opt_kwonly(1, 2, 3)
        self.assertEqual(ac_tester.keywords_opt_kwonly(1, b=2), (1, 2, None, None))
        self.assertEqual(ac_tester.keywords_opt_kwonly(1, 2, c=3), (1, 2, 3, None))
        self.assertEqual(ac_tester.keywords_opt_kwonly(a=1, c=3), (1, None, 3, None))
        self.assertEqual(ac_tester.keywords_opt_kwonly(a=1, b=2, c=3, d=4), (1, 2, 3, 4))

    def test_keywords_kwonly_opt(self):
        self.assertEqual(ac_tester.keywords_kwonly_opt(1), (1, None, None))
        with self.assertRaises(TypeError):
            ac_tester.keywords_kwonly_opt(1, 2)
        self.assertEqual(ac_tester.keywords_kwonly_opt(1, b=2), (1, 2, None))
        self.assertEqual(ac_tester.keywords_kwonly_opt(a=1, c=3), (1, None, 3))
        self.assertEqual(ac_tester.keywords_kwonly_opt(a=1, b=2, c=3), (1, 2, 3))

    def test_posonly_keywords(self):
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords(1)
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords(a=1, b=2)
        self.assertEqual(ac_tester.posonly_keywords(1, 2), (1, 2))
        self.assertEqual(ac_tester.posonly_keywords(1, b=2), (1, 2))

    def test_posonly_kwonly(self):
        with self.assertRaises(TypeError):
            ac_tester.posonly_kwonly(1)
        with self.assertRaises(TypeError):
            ac_tester.posonly_kwonly(1, 2)
        with self.assertRaises(TypeError):
            ac_tester.posonly_kwonly(a=1, b=2)
        self.assertEqual(ac_tester.posonly_kwonly(1, b=2), (1, 2))

    def test_posonly_keywords_kwonly(self):
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_kwonly(1)
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_kwonly(1, 2, 3)
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_kwonly(a=1, b=2, c=3)
        self.assertEqual(ac_tester.posonly_keywords_kwonly(1, 2, c=3), (1, 2, 3))
        self.assertEqual(ac_tester.posonly_keywords_kwonly(1, b=2, c=3), (1, 2, 3))

    def test_posonly_keywords_opt(self):
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_opt(1)
        self.assertEqual(ac_tester.posonly_keywords_opt(1, 2), (1, 2, None, None))
        self.assertEqual(ac_tester.posonly_keywords_opt(1, 2, 3), (1, 2, 3, None))
        self.assertEqual(ac_tester.posonly_keywords_opt(1, 2, 3, 4), (1, 2, 3, 4))
        self.assertEqual(ac_tester.posonly_keywords_opt(1, b=2), (1, 2, None, None))
        self.assertEqual(ac_tester.posonly_keywords_opt(1, 2, c=3), (1, 2, 3, None))
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_opt(a=1, b=2, c=3, d=4)
        self.assertEqual(ac_tester.posonly_keywords_opt(1, b=2, c=3, d=4), (1, 2, 3, 4))

    def test_posonly_opt_keywords_opt(self):
        self.assertEqual(ac_tester.posonly_opt_keywords_opt(1), (1, None, None, None))
        self.assertEqual(ac_tester.posonly_opt_keywords_opt(1, 2), (1, 2, None, None))
        self.assertEqual(ac_tester.posonly_opt_keywords_opt(1, 2, 3), (1, 2, 3, None))
        self.assertEqual(ac_tester.posonly_opt_keywords_opt(1, 2, 3, 4), (1, 2, 3, 4))
        with self.assertRaises(TypeError):
            ac_tester.posonly_opt_keywords_opt(1, b=2)
        self.assertEqual(ac_tester.posonly_opt_keywords_opt(1, 2, c=3), (1, 2, 3, None))
        self.assertEqual(ac_tester.posonly_opt_keywords_opt(1, 2, c=3, d=4), (1, 2, 3, 4))
        with self.assertRaises(TypeError):
            ac_tester.posonly_opt_keywords_opt(a=1, b=2, c=3, d=4)

    def test_posonly_kwonly_opt(self):
        with self.assertRaises(TypeError):
            ac_tester.posonly_kwonly_opt(1)
        with self.assertRaises(TypeError):
            ac_tester.posonly_kwonly_opt(1, 2)
        self.assertEqual(ac_tester.posonly_kwonly_opt(1, b=2), (1, 2, None, None))
        self.assertEqual(ac_tester.posonly_kwonly_opt(1, b=2, c=3), (1, 2, 3, None))
        self.assertEqual(ac_tester.posonly_kwonly_opt(1, b=2, c=3, d=4), (1, 2, 3, 4))
        with self.assertRaises(TypeError):
            ac_tester.posonly_kwonly_opt(a=1, b=2, c=3, d=4)

    def test_posonly_opt_kwonly_opt(self):
        self.assertEqual(ac_tester.posonly_opt_kwonly_opt(1), (1, None, None, None))
        self.assertEqual(ac_tester.posonly_opt_kwonly_opt(1, 2), (1, 2, None, None))
        with self.assertRaises(TypeError):
            ac_tester.posonly_opt_kwonly_opt(1, 2, 3)
        with self.assertRaises(TypeError):
            ac_tester.posonly_opt_kwonly_opt(1, b=2)
        self.assertEqual(ac_tester.posonly_opt_kwonly_opt(1, 2, c=3), (1, 2, 3, None))
        self.assertEqual(ac_tester.posonly_opt_kwonly_opt(1, 2, c=3, d=4), (1, 2, 3, 4))

    def test_posonly_keywords_kwonly_opt(self):
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_kwonly_opt(1)
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_kwonly_opt(1, 2)
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_kwonly_opt(1, b=2)
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_kwonly_opt(1, 2, 3)
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_kwonly_opt(a=1, b=2, c=3)
        self.assertEqual(ac_tester.posonly_keywords_kwonly_opt(1, 2, c=3), (1, 2, 3, None, None))
        self.assertEqual(ac_tester.posonly_keywords_kwonly_opt(1, b=2, c=3), (1, 2, 3, None, None))
        self.assertEqual(ac_tester.posonly_keywords_kwonly_opt(1, 2, c=3, d=4), (1, 2, 3, 4, None))
        self.assertEqual(ac_tester.posonly_keywords_kwonly_opt(1, 2, c=3, d=4, e=5), (1, 2, 3, 4, 5))

    def test_posonly_keywords_opt_kwonly_opt(self):
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_opt_kwonly_opt(1)
        self.assertEqual(ac_tester.posonly_keywords_opt_kwonly_opt(1, 2), (1, 2, None, None, None))
        self.assertEqual(ac_tester.posonly_keywords_opt_kwonly_opt(1, b=2), (1, 2, None, None, None))
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_opt_kwonly_opt(1, 2, 3, 4)
        with self.assertRaises(TypeError):
            ac_tester.posonly_keywords_opt_kwonly_opt(a=1, b=2)
        self.assertEqual(ac_tester.posonly_keywords_opt_kwonly_opt(1, 2, c=3), (1, 2, 3, None, None))
        self.assertEqual(ac_tester.posonly_keywords_opt_kwonly_opt(1, b=2, c=3), (1, 2, 3, None, None))
        self.assertEqual(ac_tester.posonly_keywords_opt_kwonly_opt(1, 2, 3, d=4), (1, 2, 3, 4, None))
        self.assertEqual(ac_tester.posonly_keywords_opt_kwonly_opt(1, 2, c=3, d=4), (1, 2, 3, 4, None))
        self.assertEqual(ac_tester.posonly_keywords_opt_kwonly_opt(1, 2, 3, d=4, e=5), (1, 2, 3, 4, 5))
        self.assertEqual(ac_tester.posonly_keywords_opt_kwonly_opt(1, 2, c=3, d=4, e=5), (1, 2, 3, 4, 5))

    def test_posonly_opt_keywords_opt_kwonly_opt(self):
        self.assertEqual(ac_tester.posonly_opt_keywords_opt_kwonly_opt(1), (1, None, None, None))
        self.assertEqual(ac_tester.posonly_opt_keywords_opt_kwonly_opt(1, 2), (1, 2, None, None))
        with self.assertRaises(TypeError):
            ac_tester.posonly_opt_keywords_opt_kwonly_opt(1, b=2)
        self.assertEqual(ac_tester.posonly_opt_keywords_opt_kwonly_opt(1, 2, 3), (1, 2, 3, None))
        self.assertEqual(ac_tester.posonly_opt_keywords_opt_kwonly_opt(1, 2, c=3), (1, 2, 3, None))
        self.assertEqual(ac_tester.posonly_opt_keywords_opt_kwonly_opt(1, 2, 3, d=4), (1, 2, 3, 4))
        self.assertEqual(ac_tester.posonly_opt_keywords_opt_kwonly_opt(1, 2, c=3, d=4), (1, 2, 3, 4))
        with self.assertRaises(TypeError):
            ac_tester.posonly_opt_keywords_opt_kwonly_opt(1, 2, 3, 4)

    def test_keyword_only_parameter(self):
        with self.assertRaises(TypeError):
            ac_tester.keyword_only_parameter()
        with self.assertRaises(TypeError):
            ac_tester.keyword_only_parameter(1)
        self.assertEqual(ac_tester.keyword_only_parameter(a=1), (1,))

    def test_posonly_vararg(self):
        with self.assertRaises(TypeError):
            ac_tester.posonly_vararg()
        self.assertEqual(ac_tester.posonly_vararg(1, 2), (1, 2, ()))
        self.assertEqual(ac_tester.posonly_vararg(1, b=2), (1, 2, ()))
        self.assertEqual(ac_tester.posonly_vararg(1, 2, 3, 4), (1, 2, (3, 4)))

    def test_vararg_and_posonly(self):
        with self.assertRaises(TypeError):
            ac_tester.vararg_and_posonly()
        with self.assertRaises(TypeError):
            ac_tester.vararg_and_posonly(1, b=2)
        self.assertEqual(ac_tester.vararg_and_posonly(1, 2, 3, 4), (1, (2, 3, 4)))

    def test_vararg(self):
        with self.assertRaises(TypeError):
            ac_tester.vararg()
        with self.assertRaises(TypeError):
            ac_tester.vararg(1, b=2)
        self.assertEqual(ac_tester.vararg(1, 2, 3, 4), (1, (2, 3, 4)))

    def test_vararg_with_default(self):
        with self.assertRaises(TypeError):
            ac_tester.vararg_with_default()
        self.assertEqual(ac_tester.vararg_with_default(1, b=False), (1, (), False))
        self.assertEqual(ac_tester.vararg_with_default(1, 2, 3, 4), (1, (2, 3, 4), False))
        self.assertEqual(ac_tester.vararg_with_default(1, 2, 3, 4, b=True), (1, (2, 3, 4), True))

    def test_vararg_with_only_defaults(self):
        self.assertEqual(ac_tester.vararg_with_only_defaults(), ((), None))
        self.assertEqual(ac_tester.vararg_with_only_defaults(b=2), ((), 2))
        self.assertEqual(ac_tester.vararg_with_only_defaults(1, b=2), ((1, ), 2))
        self.assertEqual(ac_tester.vararg_with_only_defaults(1, 2, 3, 4), ((1, 2, 3, 4), None))
        self.assertEqual(ac_tester.vararg_with_only_defaults(1, 2, 3, 4, b=5), ((1, 2, 3, 4), 5))

    def test_gh_32092_oob(self):
        ac_tester.gh_32092_oob(1, 2, 3, 4, kw1=5, kw2=6)

    def test_gh_32092_kw_pass(self):
        ac_tester.gh_32092_kw_pass(1, 2, 3)

    def test_gh_99233_refcount(self):
        arg = '*A unique string is not referenced by anywhere else.*'
        arg_refcount_origin = sys.getrefcount(arg)
        ac_tester.gh_99233_refcount(arg)
        arg_refcount_after = sys.getrefcount(arg)
        self.assertEqual(arg_refcount_origin, arg_refcount_after)

    def test_gh_99240_double_free(self):
        expected_error = r'gh_99240_double_free\(\) argument 2 must be encoded string without null bytes, not str'
        with self.assertRaisesRegex(TypeError, expected_error):
            ac_tester.gh_99240_double_free('a', '\0b')

    def test_cloned_func_exception_message(self):
        incorrect_arg = -1  # f1() and f2() accept a single str
        with self.assertRaisesRegex(TypeError, "clone_f1"):
            ac_tester.clone_f1(incorrect_arg)
        with self.assertRaisesRegex(TypeError, "clone_f2"):
            ac_tester.clone_f2(incorrect_arg)

    def test_cloned_func_with_converter_exception_message(self):
        for name in "clone_with_conv_f1", "clone_with_conv_f2":
            with self.subTest(name=name):
                func = getattr(ac_tester, name)
                self.assertEqual(func(), name)

    def test_meth_method_no_params(self):
        obj = ac_tester.TestClass()
        meth = obj.meth_method_no_params
        check = partial(self.assertRaisesRegex, TypeError, "no arguments")
        check(meth, 1)
        check(meth, a=1)

    def test_meth_method_no_params_capi(self):
        from _testcapi import pyobject_vectorcall
        obj = ac_tester.TestClass()
        meth = obj.meth_method_no_params
        pyobject_vectorcall(meth, None, None)
        pyobject_vectorcall(meth, (), None)
        pyobject_vectorcall(meth, (), ())
        pyobject_vectorcall(meth, None, ())

        check = partial(self.assertRaisesRegex, TypeError, "no arguments")
        check(pyobject_vectorcall, meth, (1,), None)
        check(pyobject_vectorcall, meth, (1,), ("a",))


class PermutationTests(unittest.TestCase):
    """Test permutation support functions."""

    def test_permute_left_option_groups(self):
        expected = (
            (),
            (3,),
            (2, 3),
            (1, 2, 3),
        )
        data = list(zip([1, 2, 3]))  # Generate a list of 1-tuples.
        actual = tuple(clinic.permute_left_option_groups(data))
        self.assertEqual(actual, expected)

    def test_permute_right_option_groups(self):
        expected = (
            (),
            (1,),
            (1, 2),
            (1, 2, 3),
        )
        data = list(zip([1, 2, 3]))  # Generate a list of 1-tuples.
        actual = tuple(clinic.permute_right_option_groups(data))
        self.assertEqual(actual, expected)

    def test_permute_optional_groups(self):
        empty = {
            "left": (), "required": (), "right": (),
            "expected": ((),),
        }
        noleft1 = {
            "left": (), "required": ("b",), "right": ("c",),
            "expected": (
                ("b",),
                ("b", "c"),
            ),
        }
        noleft2 = {
            "left": (), "required": ("b", "c",), "right": ("d",),
            "expected": (
                ("b", "c"),
                ("b", "c", "d"),
            ),
        }
        noleft3 = {
            "left": (), "required": ("b", "c",), "right": ("d", "e"),
            "expected": (
                ("b", "c"),
                ("b", "c", "d"),
                ("b", "c", "d", "e"),
            ),
        }
        noright1 = {
            "left": ("a",), "required": ("b",), "right": (),
            "expected": (
                ("b",),
                ("a", "b"),
            ),
        }
        noright2 = {
            "left": ("a",), "required": ("b", "c"), "right": (),
            "expected": (
                ("b", "c"),
                ("a", "b", "c"),
            ),
        }
        noright3 = {
            "left": ("a", "b"), "required": ("c",), "right": (),
            "expected": (
                ("c",),
                ("b", "c"),
                ("a", "b", "c"),
            ),
        }
        leftandright1 = {
            "left": ("a",), "required": ("b",), "right": ("c",),
            "expected": (
                ("b",),
                ("a", "b"),  # Prefer left.
                ("a", "b", "c"),
            ),
        }
        leftandright2 = {
            "left": ("a", "b"), "required": ("c", "d"), "right": ("e", "f"),
            "expected": (
                ("c", "d"),
                ("b", "c", "d"),       # Prefer left.
                ("a", "b", "c", "d"),  # Prefer left.
                ("a", "b", "c", "d", "e"),
                ("a", "b", "c", "d", "e", "f"),
            ),
        }
        dataset = (
            empty,
            noleft1, noleft2, noleft3,
            noright1, noright2, noright3,
            leftandright1, leftandright2,
        )
        for params in dataset:
            with self.subTest(**params):
                left, required, right, expected = params.values()
                permutations = clinic.permute_optional_groups(left, required, right)
                actual = tuple(permutations)
                self.assertEqual(actual, expected)


class FormatHelperTests(unittest.TestCase):

    def test_strip_leading_and_trailing_blank_lines(self):
        dataset = (
            # Input lines, expected output.
            ("a\nb",            "a\nb"),
            ("a\nb\n",          "a\nb"),
            ("a\nb ",           "a\nb"),
            ("\na\nb\n\n",      "a\nb"),
            ("\n\na\nb\n\n",    "a\nb"),
            ("\n\na\n\nb\n\n",  "a\n\nb"),
            # Note, leading whitespace is preserved:
            (" a\nb",               " a\nb"),
            (" a\nb ",              " a\nb"),
            (" \n \n a\nb \n \n ",  " a\nb"),
        )
        for lines, expected in dataset:
            with self.subTest(lines=lines, expected=expected):
                out = clinic.strip_leading_and_trailing_blank_lines(lines)
                self.assertEqual(out, expected)

    def test_normalize_snippet(self):
        snippet = """
            one
            two
            three
        """

        # Expected outputs:
        zero_indent = (
            "one\n"
            "two\n"
            "three"
        )
        four_indent = (
            "    one\n"
            "    two\n"
            "    three"
        )
        eight_indent = (
            "        one\n"
            "        two\n"
            "        three"
        )
        expected_outputs = {0: zero_indent, 4: four_indent, 8: eight_indent}
        for indent, expected in expected_outputs.items():
            with self.subTest(indent=indent):
                actual = clinic.normalize_snippet(snippet, indent=indent)
                self.assertEqual(actual, expected)

    def test_accumulator(self):
        acc = clinic.text_accumulator()
        self.assertEqual(acc.output(), "")
        acc.append("a")
        self.assertEqual(acc.output(), "a")
        self.assertEqual(acc.output(), "")
        acc.append("b")
        self.assertEqual(acc.output(), "b")
        self.assertEqual(acc.output(), "")
        acc.append("c")
        acc.append("d")
        self.assertEqual(acc.output(), "cd")
        self.assertEqual(acc.output(), "")

    def test_quoted_for_c_string(self):
        dataset = (
            # input,    expected
            (r"abc",    r"abc"),
            (r"\abc",   r"\\abc"),
            (r"\a\bc",  r"\\a\\bc"),
            (r"\a\\bc", r"\\a\\\\bc"),
            (r'"abc"',  r'\"abc\"'),
            (r"'a'",    r"\'a\'"),
        )
        for line, expected in dataset:
            with self.subTest(line=line, expected=expected):
                out = clinic.quoted_for_c_string(line)
                self.assertEqual(out, expected)

    def test_rstrip_lines(self):
        lines = (
            "a \n"
            "b\n"
            " c\n"
            " d \n"
        )
        expected = (
            "a\n"
            "b\n"
            " c\n"
            " d\n"
        )
        out = clinic.rstrip_lines(lines)
        self.assertEqual(out, expected)

    def test_format_escape(self):
        line = "{}, {a}"
        expected = "{{}}, {{a}}"
        out = clinic.format_escape(line)
        self.assertEqual(out, expected)

    def test_indent_all_lines(self):
        # Blank lines are expected to be unchanged.
        self.assertEqual(clinic.indent_all_lines("", prefix="bar"), "")

        lines = (
            "one\n"
            "two"  # The missing newline is deliberate.
        )
        expected = (
            "barone\n"
            "bartwo"
        )
        out = clinic.indent_all_lines(lines, prefix="bar")
        self.assertEqual(out, expected)

        # If last line is empty, expect it to be unchanged.
        lines = (
            "\n"
            "one\n"
            "two\n"
            ""
        )
        expected = (
            "bar\n"
            "barone\n"
            "bartwo\n"
            ""
        )
        out = clinic.indent_all_lines(lines, prefix="bar")
        self.assertEqual(out, expected)

    def test_suffix_all_lines(self):
        # Blank lines are expected to be unchanged.
        self.assertEqual(clinic.suffix_all_lines("", suffix="foo"), "")

        lines = (
            "one\n"
            "two"  # The missing newline is deliberate.
        )
        expected = (
            "onefoo\n"
            "twofoo"
        )
        out = clinic.suffix_all_lines(lines, suffix="foo")
        self.assertEqual(out, expected)

        # If last line is empty, expect it to be unchanged.
        lines = (
            "\n"
            "one\n"
            "two\n"
            ""
        )
        expected = (
            "foo\n"
            "onefoo\n"
            "twofoo\n"
            ""
        )
        out = clinic.suffix_all_lines(lines, suffix="foo")
        self.assertEqual(out, expected)


if __name__ == "__main__":
    unittest.main()
