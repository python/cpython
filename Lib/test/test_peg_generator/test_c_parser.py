import contextlib
import subprocess
import sysconfig
import textwrap
import unittest
import os
import shutil
import tempfile
from pathlib import Path

from test import test_tools
from test import support
from test.support import os_helper, import_helper
from test.support.script_helper import assert_python_ok

if support.check_cflags_pgo():
    raise unittest.SkipTest("peg_generator test disabled under PGO build")

test_tools.skip_if_missing("peg_generator")
with test_tools.imports_under_tool("peg_generator"):
    from pegen.grammar_parser import GeneratedParser as GrammarParser
    from pegen.testutil import (
        parse_string,
        generate_parser_c_extension,
        generate_c_parser_source,
    )


TEST_TEMPLATE = """
tmp_dir = {extension_path!r}

import ast
import traceback
import sys
import unittest

from test import test_tools
with test_tools.imports_under_tool("peg_generator"):
    from pegen.ast_dump import ast_dump

sys.path.insert(0, tmp_dir)
import parse

class Tests(unittest.TestCase):

    def check_input_strings_for_grammar(
        self,
        valid_cases = (),
        invalid_cases = (),
    ):
        if valid_cases:
            for case in valid_cases:
                parse.parse_string(case, mode=0)

        if invalid_cases:
            for case in invalid_cases:
                with self.assertRaises(SyntaxError):
                    parse.parse_string(case, mode=0)

    def verify_ast_generation(self, stmt):
        expected_ast = ast.parse(stmt)
        actual_ast = parse.parse_string(stmt, mode=1)
        self.assertEqual(ast_dump(expected_ast), ast_dump(actual_ast))

    def test_parse(self):
        {test_source}

unittest.main()
"""


@support.requires_subprocess()
class TestCParser(unittest.TestCase):

    _has_run = False

    @classmethod
    def setUpClass(cls):
        if cls._has_run:
            # Since gh-104798 (Use setuptools in peg-generator and reenable
            # tests), this test case has been producing ref leaks. Initial
            # debugging points to bug(s) in setuptools and/or importlib.
            # See gh-105063 for more info.
            raise unittest.SkipTest("gh-105063: can not rerun because of ref. leaks")
        cls._has_run = True

        # When running under regtest, a separate tempdir is used
        # as the current directory and watched for left-overs.
        # Reusing that as the base for temporary directories
        # ensures everything is cleaned up properly and
        # cleans up afterwards if not (with warnings).
        cls.tmp_base = os.getcwd()
        if os.path.samefile(cls.tmp_base, os_helper.SAVEDCWD):
            cls.tmp_base = None
        # Create a directory for the reuseable static library part of
        # the pegen extension build process.  This greatly reduces the
        # runtime overhead of spawning compiler processes.
        cls.library_dir = tempfile.mkdtemp(dir=cls.tmp_base)
        cls.addClassCleanup(shutil.rmtree, cls.library_dir)

        with contextlib.ExitStack() as stack:
            python_exe = stack.enter_context(support.setup_venv_with_pip_setuptools("venv"))
            sitepackages = subprocess.check_output(
                [python_exe, "-c", "import sysconfig; print(sysconfig.get_path('platlib'))"],
                text=True,
            ).strip()
            stack.enter_context(import_helper.DirsOnSysPath(sitepackages))
            cls.addClassCleanup(stack.pop_all().close)

    @support.requires_venv_with_pip()
    def setUp(self):
        self._backup_config_vars = dict(sysconfig._CONFIG_VARS)
        cmd = support.missing_compiler_executable()
        if cmd is not None:
            self.skipTest("The %r command is not found" % cmd)
        self.old_cwd = os.getcwd()
        self.tmp_path = tempfile.mkdtemp(dir=self.tmp_base)
        self.enterContext(os_helper.change_cwd(self.tmp_path))

    def tearDown(self):
        os.chdir(self.old_cwd)
        shutil.rmtree(self.tmp_path)
        sysconfig._CONFIG_VARS.clear()
        sysconfig._CONFIG_VARS.update(self._backup_config_vars)

    def build_extension(self, grammar_source):
        grammar = parse_string(grammar_source, GrammarParser)
        # Because setUp() already changes the current directory to the
        # temporary path, use a relative path here to prevent excessive
        # path lengths when compiling.
        generate_parser_c_extension(grammar, Path('.'), library_dir=self.library_dir)

    def run_test(self, grammar_source, test_source):
        self.build_extension(grammar_source)
        test_source = textwrap.indent(textwrap.dedent(test_source), 8 * " ")
        assert_python_ok(
            "-c",
            TEST_TEMPLATE.format(extension_path=self.tmp_path, test_source=test_source),
        )

    def test_c_parser(self) -> None:
        grammar_source = """
        start[mod_ty]: a[asdl_stmt_seq*]=stmt* $ { _PyAST_Module(a, NULL, p->arena) }
        stmt[stmt_ty]: a=expr_stmt { a }
        expr_stmt[stmt_ty]: a=expression NEWLINE { _PyAST_Expr(a, EXTRA) }
        expression[expr_ty]: ( l=expression '+' r=term { _PyAST_BinOp(l, Add, r, EXTRA) }
                            | l=expression '-' r=term { _PyAST_BinOp(l, Sub, r, EXTRA) }
                            | t=term { t }
                            )
        term[expr_ty]: ( l=term '*' r=factor { _PyAST_BinOp(l, Mult, r, EXTRA) }
                    | l=term '/' r=factor { _PyAST_BinOp(l, Div, r, EXTRA) }
                    | f=factor { f }
                    )
        factor[expr_ty]: ('(' e=expression ')' { e }
                        | a=atom { a }
                        )
        atom[expr_ty]: ( n=NAME { n }
                    | n=NUMBER { n }
                    | s=STRING { s }
                    )
        """
        test_source = """
        expressions = [
            "4+5",
            "4-5",
            "4*5",
            "1+4*5",
            "1+4/5",
            "(1+1) + (1+1)",
            "(1+1) - (1+1)",
            "(1+1) * (1+1)",
            "(1+1) / (1+1)",
        ]

        for expr in expressions:
            the_ast = parse.parse_string(expr, mode=1)
            expected_ast = ast.parse(expr)
            self.assertEqual(ast_dump(the_ast), ast_dump(expected_ast))
        """
        self.run_test(grammar_source, test_source)

    def test_lookahead(self) -> None:
        grammar_source = """
        start: NAME &NAME expr NEWLINE? ENDMARKER
        expr: NAME | NUMBER
        """
        test_source = """
        valid_cases = ["foo bar"]
        invalid_cases = ["foo 34"]
        self.check_input_strings_for_grammar(valid_cases, invalid_cases)
        """
        self.run_test(grammar_source, test_source)

    def test_negative_lookahead(self) -> None:
        grammar_source = """
        start: NAME !NAME expr NEWLINE? ENDMARKER
        expr: NAME | NUMBER
        """
        test_source = """
        valid_cases = ["foo 34"]
        invalid_cases = ["foo bar"]
        self.check_input_strings_for_grammar(valid_cases, invalid_cases)
        """
        self.run_test(grammar_source, test_source)

    def test_cut(self) -> None:
        grammar_source = """
        start: X ~ Y Z | X Q S
        X: 'x'
        Y: 'y'
        Z: 'z'
        Q: 'q'
        S: 's'
        """
        test_source = """
        valid_cases = ["x y z"]
        invalid_cases = ["x q s"]
        self.check_input_strings_for_grammar(valid_cases, invalid_cases)
        """
        self.run_test(grammar_source, test_source)

    def test_gather(self) -> None:
        grammar_source = """
        start: ';'.pass_stmt+ NEWLINE
        pass_stmt: 'pass'
        """
        test_source = """
        valid_cases = ["pass", "pass; pass"]
        invalid_cases = ["pass;", "pass; pass;"]
        self.check_input_strings_for_grammar(valid_cases, invalid_cases)
        """
        self.run_test(grammar_source, test_source)

    def test_left_recursion(self) -> None:
        grammar_source = """
        start: expr NEWLINE
        expr: ('-' term | expr '+' term | term)
        term: NUMBER
        """
        test_source = """
        valid_cases = ["-34", "34", "34 + 12", "1 + 1 + 2 + 3"]
        self.check_input_strings_for_grammar(valid_cases)
        """
        self.run_test(grammar_source, test_source)

    def test_advanced_left_recursive(self) -> None:
        grammar_source = """
        start: NUMBER | sign start
        sign: ['-']
        """
        test_source = """
        valid_cases = ["23", "-34"]
        self.check_input_strings_for_grammar(valid_cases)
        """
        self.run_test(grammar_source, test_source)

    def test_mutually_left_recursive(self) -> None:
        grammar_source = """
        start: foo 'E'
        foo: bar 'A' | 'B'
        bar: foo 'C' | 'D'
        """
        test_source = """
        valid_cases = ["B E", "D A C A E"]
        self.check_input_strings_for_grammar(valid_cases)
        """
        self.run_test(grammar_source, test_source)

    def test_nasty_mutually_left_recursive(self) -> None:
        grammar_source = """
        start: target '='
        target: maybe '+' | NAME
        maybe: maybe '-' | target
        """
        test_source = """
        valid_cases = ["x ="]
        invalid_cases = ["x - + ="]
        self.check_input_strings_for_grammar(valid_cases, invalid_cases)
        """
        self.run_test(grammar_source, test_source)

    def test_return_stmt_noexpr_action(self) -> None:
        grammar_source = """
        start[mod_ty]: a=[statements] ENDMARKER { _PyAST_Module(a, NULL, p->arena) }
        statements[asdl_stmt_seq*]: a[asdl_stmt_seq*]=statement+ { a }
        statement[stmt_ty]: simple_stmt
        simple_stmt[stmt_ty]: small_stmt
        small_stmt[stmt_ty]: return_stmt
        return_stmt[stmt_ty]: a='return' NEWLINE { _PyAST_Return(NULL, EXTRA) }
        """
        test_source = """
        stmt = "return"
        self.verify_ast_generation(stmt)
        """
        self.run_test(grammar_source, test_source)

    def test_gather_action_ast(self) -> None:
        grammar_source = """
        start[mod_ty]: a[asdl_stmt_seq*]=';'.pass_stmt+ NEWLINE ENDMARKER { _PyAST_Module(a, NULL, p->arena) }
        pass_stmt[stmt_ty]: a='pass' { _PyAST_Pass(EXTRA)}
        """
        test_source = """
        stmt = "pass; pass"
        self.verify_ast_generation(stmt)
        """
        self.run_test(grammar_source, test_source)

    def test_pass_stmt_action(self) -> None:
        grammar_source = """
        start[mod_ty]: a=[statements] ENDMARKER { _PyAST_Module(a, NULL, p->arena) }
        statements[asdl_stmt_seq*]: a[asdl_stmt_seq*]=statement+ { a }
        statement[stmt_ty]: simple_stmt
        simple_stmt[stmt_ty]: small_stmt
        small_stmt[stmt_ty]: pass_stmt
        pass_stmt[stmt_ty]: a='pass' NEWLINE { _PyAST_Pass(EXTRA) }
        """
        test_source = """
        stmt = "pass"
        self.verify_ast_generation(stmt)
        """
        self.run_test(grammar_source, test_source)

    def test_if_stmt_action(self) -> None:
        grammar_source = """
        start[mod_ty]: a=[statements] ENDMARKER { _PyAST_Module(a, NULL, p->arena) }
        statements[asdl_stmt_seq*]: a=statement+ { (asdl_stmt_seq*)_PyPegen_seq_flatten(p, a) }
        statement[asdl_stmt_seq*]:  a=compound_stmt { (asdl_stmt_seq*)_PyPegen_singleton_seq(p, a) } | simple_stmt

        simple_stmt[asdl_stmt_seq*]: a=small_stmt b=further_small_stmt* [';'] NEWLINE {
                                            (asdl_stmt_seq*)_PyPegen_seq_insert_in_front(p, a, b) }
        further_small_stmt[stmt_ty]: ';' a=small_stmt { a }

        block: simple_stmt | NEWLINE INDENT a=statements DEDENT { a }

        compound_stmt: if_stmt

        if_stmt: 'if' a=full_expression ':' b=block { _PyAST_If(a, b, NULL, EXTRA) }

        small_stmt[stmt_ty]: pass_stmt

        pass_stmt[stmt_ty]: a='pass' { _PyAST_Pass(EXTRA) }

        full_expression: NAME
        """
        test_source = """
        stmt = "pass"
        self.verify_ast_generation(stmt)
        """
        self.run_test(grammar_source, test_source)

    def test_same_name_different_types(self) -> None:
        grammar_source = """
        start[mod_ty]: a[asdl_stmt_seq*]=import_from+ NEWLINE ENDMARKER { _PyAST_Module(a, NULL, p->arena)}
        import_from[stmt_ty]: ( a='from' !'import' c=simple_name 'import' d=import_as_names_from {
                                _PyAST_ImportFrom(c->v.Name.id, d, 0, EXTRA) }
                            | a='from' '.' 'import' c=import_as_names_from {
                                _PyAST_ImportFrom(NULL, c, 1, EXTRA) }
                            )
        simple_name[expr_ty]: NAME
        import_as_names_from[asdl_alias_seq*]: a[asdl_alias_seq*]=','.import_as_name_from+ { a }
        import_as_name_from[alias_ty]: a=NAME 'as' b=NAME { _PyAST_alias(((expr_ty) a)->v.Name.id, ((expr_ty) b)->v.Name.id, EXTRA) }
        """
        test_source = """
        for stmt in ("from a import b as c", "from . import a as b"):
            expected_ast = ast.parse(stmt)
            actual_ast = parse.parse_string(stmt, mode=1)
            self.assertEqual(ast_dump(expected_ast), ast_dump(actual_ast))
        """
        self.run_test(grammar_source, test_source)

    def test_with_stmt_with_paren(self) -> None:
        grammar_source = """
        start[mod_ty]: a=[statements] ENDMARKER { _PyAST_Module(a, NULL, p->arena) }
        statements[asdl_stmt_seq*]: a=statement+ { (asdl_stmt_seq*)_PyPegen_seq_flatten(p, a) }
        statement[asdl_stmt_seq*]: a=compound_stmt { (asdl_stmt_seq*)_PyPegen_singleton_seq(p, a) }
        compound_stmt[stmt_ty]: with_stmt
        with_stmt[stmt_ty]: (
            a='with' '(' b[asdl_withitem_seq*]=','.with_item+ ')' ':' c=block {
                _PyAST_With(b, (asdl_stmt_seq*) _PyPegen_singleton_seq(p, c), NULL, EXTRA) }
        )
        with_item[withitem_ty]: (
            e=NAME o=['as' t=NAME { t }] { _PyAST_withitem(e, _PyPegen_set_expr_context(p, o, Store), p->arena) }
        )
        block[stmt_ty]: a=pass_stmt NEWLINE { a } | NEWLINE INDENT a=pass_stmt DEDENT { a }
        pass_stmt[stmt_ty]: a='pass' { _PyAST_Pass(EXTRA) }
        """
        test_source = """
        stmt = "with (\\n    a as b,\\n    c as d\\n): pass"
        the_ast = parse.parse_string(stmt, mode=1)
        self.assertTrue(ast_dump(the_ast).startswith(
            "Module(body=[With(items=[withitem(context_expr=Name(id='a', ctx=Load()), optional_vars=Name(id='b', ctx=Store())), "
            "withitem(context_expr=Name(id='c', ctx=Load()), optional_vars=Name(id='d', ctx=Store()))]"
        ))
        """
        self.run_test(grammar_source, test_source)

    def test_ternary_operator(self) -> None:
        grammar_source = """
        start[mod_ty]: a=expr ENDMARKER { _PyAST_Module(a, NULL, p->arena) }
        expr[asdl_stmt_seq*]: a=listcomp NEWLINE { (asdl_stmt_seq*)_PyPegen_singleton_seq(p, _PyAST_Expr(a, EXTRA)) }
        listcomp[expr_ty]: (
            a='[' b=NAME c=for_if_clauses d=']' { _PyAST_ListComp(b, c, EXTRA) }
        )
        for_if_clauses[asdl_comprehension_seq*]: (
            a[asdl_comprehension_seq*]=(y=['async'] 'for' a=NAME 'in' b=NAME c[asdl_expr_seq*]=('if' z=NAME { z })*
                { _PyAST_comprehension(_PyAST_Name(((expr_ty) a)->v.Name.id, Store, EXTRA), b, c, (y == NULL) ? 0 : 1, p->arena) })+ { a }
        )
        """
        test_source = """
        stmt = "[i for i in a if b]"
        self.verify_ast_generation(stmt)
        """
        self.run_test(grammar_source, test_source)

    def test_syntax_error_for_string(self) -> None:
        grammar_source = """
        start: expr+ NEWLINE? ENDMARKER
        expr: NAME
        """
        test_source = r"""
        for text in ("a b 42 b a", "\u540d \u540d 42 \u540d \u540d"):
            try:
                parse.parse_string(text, mode=0)
            except SyntaxError as e:
                tb = traceback.format_exc()
            self.assertTrue('File "<string>", line 1' in tb)
            self.assertTrue(f"SyntaxError: invalid syntax" in tb)
        """
        self.run_test(grammar_source, test_source)

    def test_headers_and_trailer(self) -> None:
        grammar_source = """
        @header 'SOME HEADER'
        @subheader 'SOME SUBHEADER'
        @trailer 'SOME TRAILER'
        start: expr+ NEWLINE? ENDMARKER
        expr: x=NAME
        """
        grammar = parse_string(grammar_source, GrammarParser)
        parser_source = generate_c_parser_source(grammar)

        self.assertTrue("SOME HEADER" in parser_source)
        self.assertTrue("SOME SUBHEADER" in parser_source)
        self.assertTrue("SOME TRAILER" in parser_source)

    def test_error_in_rules(self) -> None:
        grammar_source = """
        start: expr+ NEWLINE? ENDMARKER
        expr: NAME {PyTuple_New(-1)}
        """
        # PyTuple_New raises SystemError if an invalid argument was passed.
        test_source = """
        with self.assertRaises(SystemError):
            parse.parse_string("a", mode=0)
        """
        self.run_test(grammar_source, test_source)

    def test_no_soft_keywords(self) -> None:
        grammar_source = """
        start: expr+ NEWLINE? ENDMARKER
        expr: 'foo'
        """
        grammar = parse_string(grammar_source, GrammarParser)
        parser_source = generate_c_parser_source(grammar)
        assert "expect_soft_keyword" not in parser_source

    def test_soft_keywords(self) -> None:
        grammar_source = """
        start: expr+ NEWLINE? ENDMARKER
        expr: "foo"
        """
        grammar = parse_string(grammar_source, GrammarParser)
        parser_source = generate_c_parser_source(grammar)
        assert "expect_soft_keyword" in parser_source

    def test_soft_keywords_parse(self) -> None:
        grammar_source = """
        start: "if" expr '+' expr NEWLINE
        expr: NAME
        """
        test_source = """
        valid_cases = ["if if + if"]
        invalid_cases = ["if if"]
        self.check_input_strings_for_grammar(valid_cases, invalid_cases)
        """
        self.run_test(grammar_source, test_source)

    def test_soft_keywords_lookahead(self) -> None:
        grammar_source = """
        start: &"if" "if" expr '+' expr NEWLINE
        expr: NAME
        """
        test_source = """
        valid_cases = ["if if + if"]
        invalid_cases = ["if if"]
        self.check_input_strings_for_grammar(valid_cases, invalid_cases)
        """
        self.run_test(grammar_source, test_source)

    def test_forced(self) -> None:
        grammar_source = """
        start: NAME &&':' | NAME
        """
        test_source = """
        self.assertEqual(parse.parse_string("number :", mode=0), None)
        with self.assertRaises(SyntaxError) as e:
            parse.parse_string("a", mode=0)
        self.assertIn("expected ':'", str(e.exception))
        """
        self.run_test(grammar_source, test_source)

    def test_forced_with_group(self) -> None:
        grammar_source = """
        start: NAME &&(':' | ';') | NAME
        """
        test_source = """
        self.assertEqual(parse.parse_string("number :", mode=0), None)
        self.assertEqual(parse.parse_string("number ;", mode=0), None)
        with self.assertRaises(SyntaxError) as e:
            parse.parse_string("a", mode=0)
        self.assertIn("expected (':' | ';')", e.exception.args[0])
        """
        self.run_test(grammar_source, test_source)
