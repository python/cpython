import io
import textwrap
import unittest
from pathlib import Path, PurePath
from unittest import mock

from test import test_tools
from test.support import os_helper

test_tools.skip_if_missing("peg_generator")
with test_tools.imports_under_tool("peg_generator"):
    from pegen import grammar as grammar_module
    from pegen.build import build_parser, included_output_files
    from pegen.c_generator import CParserGenerator
    from pegen.c_generator_file import CParserEmitter
    from pegen.grammar import GrammarError, NamedItem, RuleKind
    from pegen.grammar_parser import GeneratedParser as GrammarParser
    from pegen.testutil import ALL_TOKENS, EXACT_TOKENS, NON_EXACT_TOKENS, parse_string


class TestCGenerator(unittest.TestCase):
    def make_generator(self, source):
        grammar = parse_string(source, GrammarParser)
        return CParserGenerator(
            grammar, ALL_TOKENS, EXACT_TOKENS, NON_EXACT_TOKENS, io.StringIO()
        )

    def emit_parser(self, parser):
        output = io.StringIO()
        CParserEmitter(parser, output).emit()
        return output.getvalue()

    def test_rule_types_distinguish_implicit_and_explicit_void_pointer(self):
        generator = self.make_generator("""
            start: &implicit implicit explicit
            implicit: NAME
            explicit[void*]: NAME
        """)
        generator.rules["explicit"].type = "void *"
        start, implicit, explicit = generator.prepare(PurePath("example.gram")).rules

        self.assertIsNone(implicit.signature.return_type)
        self.assertEqual(explicit.signature.return_type, "void *")
        self.assertEqual(implicit.signature.c_return_type, "void *")
        self.assertEqual(explicit.signature.c_return_type, "void *")
        self.assertEqual(
            implicit.signature.declaration(), "static void *implicit_rule(Parser *p);"
        )
        self.assertEqual(
            explicit.signature.declaration(), "static void * explicit_rule(Parser *p);"
        )
        call = start.alternatives[0].calls[0]
        self.assertEqual(call.function, "_PyPegen_lookahead")
        generator = self.make_generator("start: &explicit\nexplicit[void*]: NAME\n")
        with self.assertRaisesRegex(RuntimeError, "return type is incompatible"):
            generator.prepare(PurePath("example.gram"))

    def test_parser_plan_does_not_depend_on_compilation_state(self):
        generator = self.make_generator("""
            @header 'CUSTOM HEADER'
            @subheader 'CUSTOM SUBHEADER'
            @trailer 'CUSTOM TRAILER %(modulename)s %(mode)d'
            @modulename 'sample'
            @bytecode '1'
            start[mod_ty]: expr_without_invalid 'pass' "zsoft" "asoft" ('bb' | 'aa')* ENDMARKER
            expr_without_invalid[expr_ty] (memo): name=expr [NUMBER] { name }
            expr[expr_ty]: expr '+' NAME | NAME
        """)
        generator.debug = True
        parser = generator.prepare(PurePath("some/path/example.gram"))
        expected = self.emit_parser(parser)

        self.assertEqual(generator.file.getvalue(), "")
        self.assertEqual(parser.source_name, "example.gram")
        self.assertEqual(parser.headers, ("CUSTOM HEADER", "CUSTOM SUBHEADER"))
        self.assertEqual(parser.trailer, "CUSTOM TRAILER sample 2")
        self.assertEqual(parser.soft_keywords, ("asoft", "zsoft"))
        self.assertEqual([word for word, _ in parser.keyword_groups[2]], ["bb", "aa"])
        self.assertTrue(any(rule.signature.kind is RuleKind.LOOP0 for rule in parser.rules))
        with self.assertRaises(AttributeError):
            parser.rules[0].alternatives[0].calls[0].assigned_variable = "changed"

        generator.grammar.metas.clear()
        generator.grammar.metas["trailer"] = "%(missing)s"
        for rule in generator.all_rules.values():
            rule.name = "changed"
            rule.type = "changed_type"
            rule.flags = frozenset()
            rule.rhs.alts[0].action = "changed_action"
            rule.rhs.alts.clear()
        generator.rules.clear()
        generator.all_rules.clear()
        generator.keywords.clear()
        generator.soft_keywords.clear()
        generator.debug = False
        generator.skip_actions = True
        self.assertEqual(self.emit_parser(parser), expected)

    def test_repeated_preparation_keeps_variable_names_local(self):
        source = """
            start: 'run' expr term bindings other ENDMARKER
            expr: expr '+' NAME | NAME
            term: term '*' NUMBER | NUMBER
            bindings: (name_var=NUMBER) name_var[expr_ty]=(NAME) [NUMBER] (NAME | NUMBER) { name_var_1 }
            other: name_var=NUMBER name_var=NAME { name_var_1 }
        """
        generator = self.make_generator(source)
        parser = generator.prepare(PurePath("example.gram"))
        expected = self.emit_parser(parser)

        self.assertEqual(self.emit_parser(parser), expected)
        self.assertEqual(generator.prepare(PurePath("example.gram")), parser)
        other = self.make_generator(source).prepare(PurePath("example.gram"))
        self.assertEqual(other, parser)
        self.assertEqual(self.emit_parser(other), expected)
        self.assertEqual(expected.count("expr_ty name_var_1;"), 2)
        self.assertEqual(expected.count("_res = name_var_1;"), 2)
        self.assertNotIn("name_var_2", expected)

    def test_prepared_prefixes_preserve_reuse_and_repeatability(self):
        generator = self.make_generator("""
            start: prefix ':' NAME | prefix ':' NUMBER | NAME | prefix '=' NAME
            prefix[expr_ty] (memo): NAME
        """)
        parser = generator.prepare(PurePath("example.gram"))
        start = parser.rules[0]
        prefix, = start.prefixes
        self.assertEqual(prefix.type, "expr_ty")
        for alt in start.alternatives[:2]:
            self.assertIn("!p->call_invalid_rules", alt.calls[0].expression())
            self.assertIn(prefix.result, alt.calls[0].expression())
        self.assertEqual(start.alternatives[3].calls[0].expression(), "prefix_rule(p)")
        self.assertEqual(generator.prepare(PurePath("example.gram")), parser)
        expected = self.emit_parser(parser)
        generator.rules.clear()
        generator.all_rules.clear()
        self.assertEqual(self.emit_parser(parser), expected)

    def test_nullable_prefix_is_not_reused(self):
        generator = self.make_generator("""
            start: prefix ':' NAME | prefix ':' NUMBER
            prefix (memo): NAME?
        """)
        start = generator.prepare(PurePath("example.gram")).rules[0]
        self.assertEqual(start.prefixes, ())
        for alt in start.alternatives:
            self.assertEqual(alt.calls[0].expression(), "prefix_rule(p)")

    def test_invalid_trailer_fails_before_output(self):
        generator = self.make_generator("""
            @trailer '%(missing)s'
            start: NAME ENDMARKER
        """)
        with self.assertRaisesRegex(KeyError, "missing"):
            generator.generate(PurePath("example.gram"))
        self.assertEqual(generator.file.getvalue(), "")

    def test_empty_keyword_tables(self):
        parser = self.make_generator("start: NAME ENDMARKER\n").prepare(PurePath("example.gram"))
        source = self.emit_parser(parser)

        self.assertEqual(parser.keyword_groups, ())
        self.assertEqual(parser.soft_keywords, ())
        self.assertIn("static const int n_keyword_lists = 0;", source)
        self.assertIn(
            "static KeywordToken *reserved_keywords[] = {\n"
            "    (KeywordToken[]) {{NULL, -1}},\n"
            "};",
            source,
        )
        self.assertIn("static char *soft_keywords[] = {\n    NULL,\n};", source)

    def test_lowering_rejects_undiscovered_items(self):
        for replacement in ("missing", "(NAME NUMBER)", None):
            with self.subTest(replacement=replacement):
                generator = self.make_generator("start: NAME ENDMARKER\n")
                generator.collect_rules()
                lowerer = generator.callmakervisitor.make_lowerer()
                inventory = tuple(generator.all_rules)
                counter = generator.counter
                rule = generator.rules["start"]
                items = rule.rhs.alts[0].items
                if replacement is None:
                    items[0] = NamedItem(None, items[0].item)
                else:
                    grammar = parse_string(f"start: {replacement}\n", GrammarParser)
                    items[0].item = grammar.rules["start"].rhs.alts[0].items[0].item
                with self.assertRaisesRegex(RuntimeError, "not discovered"):
                    lowerer.prepare_rule(rule)
                self.assertEqual(tuple(generator.all_rules), inventory)
                self.assertEqual(generator.counter, counter)

    def test_helper_resolution_does_not_depend_on_display_settings(self):
        source = """
            start: NAME (a=NAME { a }) NAME* NAME+ ','.NAME+ ENDMARKER
        """
        for simple in (True, False):
            with self.subTest(simple=simple), mock.patch.object(
                grammar_module, "SIMPLE_STR", simple
            ):
                generator = self.make_generator(source)
                expected = generator.prepare(PurePath("example.gram")).rules
                with mock.patch.object(grammar_module, "SIMPLE_STR", not simple):
                    actual = generator.prepare(PurePath("example.gram")).rules
                self.assertEqual(len(actual), len(expected))
                for old, new in zip(expected, actual):
                    self.assertEqual(old.signature, new.signature)
                    self.assertEqual(
                        [alt.calls for alt in old.alternatives],
                        [alt.calls for alt in new.alternatives],
                    )

    def test_invalid_rule_gating_uses_references(self):
        cases = (
            ("invalid_example", True),
            ("value=invalid_example", True),
            ("[invalid_example]", True),
            ("invalid_example?", True),
            ("invalid_example*", True),
            ("invalid_example+", True),
            ("invalid_example.NAME+", True),
            ("[invalid_example.NAME+]", True),
            ("[invalid_example.(NAME NAME)+]", False),
            ("[[invalid_example.(NAME NAME)+]]", False),
            ("[invalid_example.(NAME | NUMBER)+]", False),
            ("&invalid_example", False),
            ("[invalid_example | NAME]", False),
            ("invalid_name=NAME", False),
        )
        for item, requires_invalid_rules in cases:
            for simple in (True, False):
                with self.subTest(item=item, simple=simple), mock.patch.object(
                    grammar_module, "SIMPLE_STR", simple
                ):
                    generator = self.make_generator(f"""
                        start: {item} {{ _PyPegen_dummy_name(p) }}
                        invalid_example: NAME
                    """)
                    start = generator.prepare(PurePath("example.gram")).rules[0]
                    self.assertEqual(
                        start.alternatives[0].requires_invalid_rules,
                        requires_invalid_rules,
                    )

    def test_lowering_preserves_legacy_named_call_types(self):
        generator = self.make_generator("""
            start: Mixed LPAR ENDMARKER
            Mixed[expr_ty]: NAME
        """)
        start, mixed = generator.prepare(PurePath("example.gram")).rules
        self.assertEqual(mixed.signature.return_type, "expr_ty")
        for call, name in zip(start.alternatives[0].calls, ("Mixed", "LPAR")):
            with self.subTest(name=name):
                self.assertEqual(call.function, f"{name}_rule")
                self.assertIsNone(call.return_type)

    def test_lowering_snapshots_symbols_and_tokens(self):
        grammar = parse_string("""
            start: 'pass' '+' atom ENDMARKER
            atom[expr_ty]: NAME
        """, GrammarParser)
        exact_tokens = dict(EXACT_TOKENS)
        non_exact_tokens = set(NON_EXACT_TOKENS)
        generator = CParserGenerator(
            grammar, ALL_TOKENS, exact_tokens, non_exact_tokens, io.StringIO()
        )
        generator.collect_rules()
        lowerer = generator.callmakervisitor.make_lowerer()
        start = generator.rules["start"]
        atom = generator.rules["atom"]
        expected = lowerer.prepare_rule(start)

        atom.type = "stmt_ty"
        generator.all_rules.clear()
        generator.tokens.clear()
        generator.keywords.clear()
        exact_tokens.clear()
        non_exact_tokens.clear()

        self.assertEqual(lowerer.prepare_rule(atom).signature.return_type, "expr_ty")
        self.assertEqual(lowerer.prepare_rule(start), expected)

    def test_keywords_are_numbered_once(self):
        generator = self.make_generator("""
            start: keywords | "soft" "soft"
            keywords: 'if' NAME | 'if' NUMBER | 'else' NAME | 'pass' | 'else' 'pass'
        """)
        generator.prepare(PurePath("example.gram"))
        self.assertEqual(generator.keywords, {"if": 500, "else": 501, "pass": 502})
        self.assertEqual(generator.soft_keywords, {"soft"})


class TestIncludedCFiles(unittest.TestCase):
    def setUp(self):
        self.tmp_dir = self.enterContext(os_helper.temp_dir())

    def write(self, name, source):
        path = Path(self.tmp_dir, name)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(textwrap.dedent(source))
        return path

    def generate(self, grammar_file):
        grammar, _, _ = build_parser(grammar_file)
        output = io.StringIO()
        included = {source: io.StringIO() for source in {
            rule.source for rule in grammar.rules.values() if rule.source is not None
        }}
        generator = CParserGenerator(
            grammar, ALL_TOKENS, EXACT_TOKENS, NON_EXACT_TOKENS, output,
            included_files=included,
        )
        generator.generate(grammar_file)
        return output.getvalue(), {
            source.name: file.getvalue() for source, file in included.items()
        }

    def test_included_rules_go_to_their_own_file(self):
        main = self.write("main.gram", """\
            @include "extra.gram"
            start: item+ NEWLINE
            item: NAME ('+' | '-') | invalid_item
            """)
        self.write("extra.gram", """\
            invalid_item: NUMBER ('+' | '-') item
            """)
        root, included = self.generate(main)
        extra = included["extra.gram"]

        self.assertTrue(root.startswith("// @generated by pegen from main.gram\n"))
        self.assertTrue(extra.startswith("// @generated by pegen from extra.gram\n"))
        # Each rule is defined in the C file of its grammar file, and only
        # the root file has the keyword tables and the trailer.
        self.assertIn("\nstart_rule(Parser *p)\n{", root)
        self.assertNotIn("\ninvalid_item_rule(Parser *p)\n{", root)
        self.assertIn("\ninvalid_item_rule(Parser *p)\n{", extra)
        self.assertNotIn("\nitem_rule(Parser *p)\n{", extra)
        self.assertIn("reserved_keywords", root)
        self.assertNotIn("reserved_keywords", extra)
        self.assertIn("_PyPegen_parse", root)
        self.assertNotIn("_PyPegen_parse", extra)
        # Rule type ids stay unique across the files.
        self.assertIn("#define start_type 1000\n", root)
        self.assertIn("#define item_type 1001\n", root)
        self.assertIn("#define invalid_item_type 1002\n", extra)
        # Rules called across files are not static and get a prefixed
        # linker name; the others stay static.
        for output in root, extra:
            self.assertIn("#define item_rule _PyPegen_item_rule\n", output)
            self.assertIn("#define invalid_item_rule _PyPegen_invalid_item_rule\n", output)
            self.assertNotIn("_PyPegen_start_rule", output)
        self.assertIn("static asdl_seq *_loop1_", root)
        self.assertIn("static void *\nstart_rule(Parser *p)", root)
        self.assertIn("\nPy_LOCAL_SYMBOL void *\nitem_rule(Parser *p)", root)
        self.assertIn("\nPy_LOCAL_SYMBOL void *\ninvalid_item_rule(Parser *p)", extra)
        # A helper for an identical subexpression is not shared between
        # files: each one has its own static copy.
        self.assertIn("static void *_tmp_", root)
        self.assertIn("static void *_tmp_", extra)

    def test_trailer_can_call_rules_of_included_files(self):
        # The default trailer calls start_rule().
        main = self.write("main.gram", """\
            @include "rules.gram"
            item: NAME
            """)
        self.write("rules.gram", "start: item+ NEWLINE\n")
        root, included = self.generate(main)
        rules = included["rules.gram"]
        self.assertIn("return start_rule(p);", root)
        for output in root, rules:
            self.assertIn("#define start_rule _PyPegen_start_rule\n", output)
            self.assertIn("\nPy_LOCAL_SYMBOL void *start_rule(Parser *p);\n", output)
        self.assertIn("\nPy_LOCAL_SYMBOL void *\nstart_rule(Parser *p)\n{", rules)

    def test_actions_can_call_rules_of_included_files(self):
        main = self.write("main.gram", """\
            @include "extra.gram"
            start: NAME NEWLINE { extra_rule(p) }
            """)
        self.write("extra.gram", "extra: NAME\n")
        root, included = self.generate(main)
        extra = included["extra.gram"]
        for output in root, extra:
            self.assertIn("#define extra_rule _PyPegen_extra_rule\n", output)
            self.assertIn("\nPy_LOCAL_SYMBOL void *extra_rule(Parser *p);\n", output)
        self.assertIn("\nPy_LOCAL_SYMBOL void *\nextra_rule(Parser *p)\n{", extra)

    def test_without_includes_all_rules_are_static(self):
        main = self.write("main.gram", """\
            start: item NEWLINE
            item: NAME
            """)
        root, included = self.generate(main)
        self.assertEqual(included, {})
        self.assertNotIn("Rules shared", root)
        self.assertNotIn("_PyPegen_item_rule", root)
        self.assertIn("static void *\nitem_rule(Parser *p)", root)

    def test_included_output_files(self):
        main = self.write("main.gram", """\
            @include "extra.gram"
            @include "sub/more.rules.gram"
            @include "a/same.gram"
            @include "b/same.gram"
            start: extra more one two NEWLINE
            """)
        self.write("extra.gram", "extra: NAME\n")
        self.write("sub/more.rules.gram", "more: NAME\n")
        self.write("a/same.gram", "one: NAME\n")
        self.write("b/same.gram", "two: NAME\n")
        grammar, _, _ = build_parser(main)
        out = Path("out", "parser.c.new")
        self.assertEqual(included_output_files(grammar, out), {
            Path("extra.gram"): Path("out", "parser_extra.c.new"),
            Path("sub", "more.rules.gram"): Path("out", "sub", "parser_more.rules.c.new"),
            Path("a", "same.gram"): Path("out", "a", "parser_same.c.new"),
            Path("b", "same.gram"): Path("out", "b", "parser_same.c.new"),
        })

    def test_included_output_files_outside_output_directory(self):
        main = self.write("grammar/main.gram", """\
            @include "../outside.gram"
            start: outside NEWLINE
            """)
        self.write("outside.gram", "outside: NAME\n")
        grammar, _, _ = build_parser(main)
        with self.assertRaisesRegex(GrammarError, "would be outside the output directory"):
            included_output_files(grammar, Path("parser.c"))


if __name__ == "__main__":
    unittest.main()
