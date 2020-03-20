import ast
from pathlib import PurePath
import textwrap
from typing import Optional, Sequence
import traceback

import pytest  # type: ignore

from pegen.grammar_parser import GeneratedParser as GrammarParser
from pegen.testutil import parse_string, generate_parser_c_extension, generate_c_parser_source


def check_input_strings_for_grammar(
    source: str,
    tmp_path: PurePath,
    valid_cases: Sequence[str] = (),
    invalid_cases: Sequence[str] = (),
) -> None:
    grammar = parse_string(source, GrammarParser)
    extension = generate_parser_c_extension(grammar, tmp_path)

    if valid_cases:
        for case in valid_cases:
            extension.parse_string(case)

    if invalid_cases:
        for case in invalid_cases:
            with pytest.raises(SyntaxError):
                extension.parse_string(case)


def verify_ast_generation(source: str, stmt: str, tmp_path: PurePath) -> None:
    grammar = parse_string(source, GrammarParser)
    extension = generate_parser_c_extension(grammar, tmp_path)

    expected_ast = ast.parse(stmt)
    actual_ast = extension.parse_string(stmt)
    assert ast.dump(expected_ast) == ast.dump(actual_ast)


def test_c_parser(tmp_path: PurePath) -> None:
    grammar_source = """
    start[mod_ty]: a=stmt* $ { Module(a, NULL, p->arena) }
    stmt[stmt_ty]: a=expr_stmt { a }
    expr_stmt[stmt_ty]: a=expression NEWLINE { _Py_Expr(a, EXTRA) }
    expression[expr_ty]: ( l=expression '+' r=term { _Py_BinOp(l, Add, r, EXTRA) }
                         | l=expression '-' r=term { _Py_BinOp(l, Sub, r, EXTRA) }
                         | t=term { t }
                         )
    term[expr_ty]: ( l=term '*' r=factor { _Py_BinOp(l, Mult, r, EXTRA) }
                   | l=term '/' r=factor { _Py_BinOp(l, Div, r, EXTRA) }
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
    grammar = parse_string(grammar_source, GrammarParser)
    extension = generate_parser_c_extension(grammar, tmp_path)

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
        the_ast = extension.parse_string(expr)
        expected_ast = ast.parse(expr)
        assert ast.dump(the_ast) == ast.dump(expected_ast)


def test_lookahead(tmp_path: PurePath) -> None:
    grammar = """
    start: NAME &NAME expr NEWLINE? ENDMARKER
    expr: NAME | NUMBER
    """
    valid_cases = ["foo bar"]
    invalid_cases = ["foo 34"]
    check_input_strings_for_grammar(grammar, tmp_path, valid_cases, invalid_cases)


def test_negative_lookahead(tmp_path: PurePath) -> None:
    grammar = """
    start: NAME !NAME expr NEWLINE? ENDMARKER
    expr: NAME | NUMBER
    """
    valid_cases = ["foo 34"]
    invalid_cases = ["foo bar"]
    check_input_strings_for_grammar(grammar, tmp_path, valid_cases, invalid_cases)


def test_cut(tmp_path: PurePath) -> None:
    grammar = """
    start: X ~ Y Z | X Q S
    X: 'x'
    Y: 'y'
    Z: 'z'
    Q: 'q'
    S: 's'
    """
    valid_cases = ["x y z"]
    invalid_cases = ["x q s"]
    check_input_strings_for_grammar(grammar, tmp_path, valid_cases, invalid_cases)


def test_gather(tmp_path: PurePath) -> None:
    grammar = """
    start: ';'.pass_stmt+ NEWLINE
    pass_stmt: 'pass'
    """
    valid_cases = ["pass", "pass; pass"]
    invalid_cases = ["pass;", "pass; pass;"]
    check_input_strings_for_grammar(grammar, tmp_path, valid_cases, invalid_cases)


def test_left_recursion(tmp_path: PurePath) -> None:
    grammar = """
    start: expr NEWLINE
    expr: ('-' term | expr '+' term | term)
    term: NUMBER
    """
    valid_cases = ["-34", "34", "34 + 12", "1 + 1 + 2 + 3"]
    check_input_strings_for_grammar(grammar, tmp_path, valid_cases)


def test_advanced_left_recursive(tmp_path: PurePath) -> None:
    grammar = """
    start: NUMBER | sign start
    sign: ['-']
    """
    valid_cases = ["23", "-34"]
    check_input_strings_for_grammar(grammar, tmp_path, valid_cases)


def test_mutually_left_recursive(tmp_path: PurePath) -> None:
    grammar = """
    start: foo 'E'
    foo: bar 'A' | 'B'
    bar: foo 'C' | 'D'
    """
    valid_cases = ["B E", "D A C A E"]
    check_input_strings_for_grammar(grammar, tmp_path, valid_cases)


def test_nasty_mutually_left_recursive(tmp_path: PurePath) -> None:
    grammar = """
    start: target '='
    target: maybe '+' | NAME
    maybe: maybe '-' | target
    """
    valid_cases = ["x ="]
    invalid_cases = ["x - + ="]
    check_input_strings_for_grammar(grammar, tmp_path, valid_cases, invalid_cases)


def test_return_stmt_noexpr_action(tmp_path: PurePath) -> None:
    grammar = """
    start[mod_ty]: a=[statements] ENDMARKER { Module(a, NULL, p->arena) }
    statements[asdl_seq*]: a=statement+ { a }
    statement[stmt_ty]: simple_stmt
    simple_stmt[stmt_ty]: small_stmt
    small_stmt[stmt_ty]: return_stmt
    return_stmt[stmt_ty]: a='return' NEWLINE { _Py_Return(NULL, EXTRA) }
    """
    stmt = "return"
    verify_ast_generation(grammar, stmt, tmp_path)


def test_gather_action_ast(tmp_path: PurePath) -> None:
    grammar = """
    start[mod_ty]: a=';'.pass_stmt+ NEWLINE ENDMARKER { Module(a, NULL, p->arena) }
    pass_stmt[stmt_ty]: a='pass' { _Py_Pass(EXTRA)}
    """
    stmt = "pass; pass"
    verify_ast_generation(grammar, stmt, tmp_path)


def test_pass_stmt_action(tmp_path: PurePath) -> None:
    grammar = """
    start[mod_ty]: a=[statements] ENDMARKER { Module(a, NULL, p->arena) }
    statements[asdl_seq*]: a=statement+ { a }
    statement[stmt_ty]: simple_stmt
    simple_stmt[stmt_ty]: small_stmt
    small_stmt[stmt_ty]: pass_stmt
    pass_stmt[stmt_ty]: a='pass' NEWLINE { _Py_Pass(EXTRA) }
    """
    stmt = "pass"
    verify_ast_generation(grammar, stmt, tmp_path)


def test_if_stmt_action(tmp_path: PurePath) -> None:
    grammar = """
    start[mod_ty]: a=[statements] ENDMARKER { Module(a, NULL, p->arena) }
    statements[asdl_seq*]: a=statement+ { seq_flatten(p, a) }
    statement[asdl_seq*]:  a=compound_stmt { singleton_seq(p, a) } | simple_stmt

    simple_stmt[asdl_seq*]: a=small_stmt b=further_small_stmt* [';'] NEWLINE { seq_insert_in_front(p, a, b) }
    further_small_stmt[stmt_ty]: ';' a=small_stmt { a }

    block: simple_stmt | NEWLINE INDENT a=statements DEDENT { a }

    compound_stmt: if_stmt

    if_stmt: 'if' a=full_expression ':' b=block { _Py_If(a, b, NULL, EXTRA) }

    small_stmt[stmt_ty]: pass_stmt

    pass_stmt[stmt_ty]: a='pass' { _Py_Pass(EXTRA) }

    full_expression: NAME
    """
    stmt = "pass"
    verify_ast_generation(grammar, stmt, tmp_path)


@pytest.mark.parametrize("stmt", ["from a import b as c", "from . import a as b"])
def test_same_name_different_types(stmt: str, tmp_path: PurePath) -> None:
    grammar = """
    start[mod_ty]: a=import_from+ NEWLINE ENDMARKER { Module(a, NULL, p->arena)}
    import_from[stmt_ty]: ( a='from' !'import' c=simple_name 'import' d=import_as_names_from {
                              _Py_ImportFrom(c->v.Name.id, d, 0, EXTRA) }
                          | a='from' '.' 'import' c=import_as_names_from {
                              _Py_ImportFrom(NULL, c, 1, EXTRA) }
                          )
    simple_name[expr_ty]: NAME
    import_as_names_from[asdl_seq*]: a=','.import_as_name_from+ { a }
    import_as_name_from[alias_ty]: a=NAME 'as' b=NAME { _Py_alias(((expr_ty) a)->v.Name.id, ((expr_ty) b)->v.Name.id, p->arena) }
    """
    verify_ast_generation(grammar, stmt, tmp_path)


def test_with_stmt_with_paren(tmp_path: PurePath) -> None:
    grammar_source = """
    start[mod_ty]: a=[statements] ENDMARKER { Module(a, NULL, p->arena) }
    statements[asdl_seq*]: a=statement+ { seq_flatten(p, a) }
    statement[asdl_seq*]: a=compound_stmt { singleton_seq(p, a) }
    compound_stmt[stmt_ty]: with_stmt
    with_stmt[stmt_ty]: (
        a='with' '(' b=','.with_item+ ')' ':' c=block {
            _Py_With(b, singleton_seq(p, c), NULL, EXTRA) }
    )
    with_item[withitem_ty]: (
        e=NAME o=['as' t=NAME { t }] { _Py_withitem(e, set_expr_context(p, o, Store), p->arena) }
    )
    block[stmt_ty]: a=pass_stmt NEWLINE { a } | NEWLINE INDENT a=pass_stmt DEDENT { a }
    pass_stmt[stmt_ty]: a='pass' { _Py_Pass(EXTRA) }
    """
    stmt = "with (\n    a as b,\n    c as d\n): pass"
    grammar = parse_string(grammar_source, GrammarParser)
    extension = generate_parser_c_extension(grammar, tmp_path)
    the_ast = extension.parse_string(stmt)
    assert ast.dump(the_ast).startswith(
        "Module(body=[With(items=[withitem(context_expr=Name(id='a', ctx=Load()), optional_vars=Name(id='b', ctx=Store())), "
        "withitem(context_expr=Name(id='c', ctx=Load()), optional_vars=Name(id='d', ctx=Store()))]"
    )


def test_ternary_operator(tmp_path: PurePath) -> None:
    grammar_source = """
    start[mod_ty]: a=expr ENDMARKER { Module(a, NULL, p->arena) }
    expr[asdl_seq*]: a=listcomp NEWLINE { singleton_seq(p, _Py_Expr(a, EXTRA)) }
    listcomp[expr_ty]: (
        a='[' b=NAME c=for_if_clauses d=']' { _Py_ListComp(b, c, EXTRA) }
    )
    for_if_clauses[asdl_seq*]: (
        a=(y=[ASYNC] 'for' a=NAME 'in' b=NAME c=('if' z=NAME { z })*
            { _Py_comprehension(_Py_Name(((expr_ty) a)->v.Name.id, Store, EXTRA), b, c, (y == NULL) ? 0 : 1, p->arena) })+ { a }
    )
    """
    stmt = "[i for i in a if b]"
    verify_ast_generation(grammar_source, stmt, tmp_path)


@pytest.mark.parametrize("text", ["a b 42 b a", "名 名 42 名 名"])
def test_syntax_error_for_string(text: str, tmp_path: PurePath) -> None:
    grammar_source = """
    start: expr+ NEWLINE? ENDMARKER
    expr: NAME
    """
    grammar = parse_string(grammar_source, GrammarParser)
    extension = generate_parser_c_extension(grammar, tmp_path)
    try:
        extension.parse_string(text)
    except SyntaxError as e:
        tb = traceback.format_exc()
    assert 'File "<string>", line 1' in tb
    assert f"{text}\n        ^" in tb


@pytest.mark.parametrize("text", ["a b 42 b a", "名 名 42 名 名"])
def test_syntax_error_for_file(text: str, tmp_path: PurePath) -> None:
    grammar_source = """
    start: expr+ NEWLINE? ENDMARKER
    expr: NAME
    """
    grammar = parse_string(grammar_source, GrammarParser)
    extension = generate_parser_c_extension(grammar, tmp_path)
    the_file = tmp_path / "some_file.py"
    with open(the_file, "w") as fd:
        fd.write(text)
    try:
        extension.parse_file(str(the_file))
    except SyntaxError as e:
        tb = traceback.format_exc()
    assert 'some_file.py", line 1' in tb
    assert f"{text}\n        ^" in tb


def test_headers_and_trailer(tmp_path: PurePath) -> None:
    grammar_source = """
    @header 'SOME HEADER'
    @subheader 'SOME SUBHEADER'
    @trailer 'SOME TRAILER'
    start: expr+ NEWLINE? ENDMARKER
    expr: x=NAME
    """
    grammar = parse_string(grammar_source, GrammarParser)
    parser_source = generate_c_parser_source(grammar)

    assert "SOME HEADER" in parser_source
    assert "SOME SUBHEADER" in parser_source
    assert "SOME TRAILER" in parser_source


def test_extension_name(tmp_path: PurePath) -> None:
    grammar_source = """
    @modulename 'alternative_name'
    start: expr+ NEWLINE? ENDMARKER
    expr: x=NAME
    """
    grammar = parse_string(grammar_source, GrammarParser)
    parser_source = generate_c_parser_source(grammar)

    assert "PyInit_alternative_name" in parser_source
    assert '.m_name = "alternative_name"' in parser_source


def test_error_in_rules(tmp_path: PurePath) -> None:
    grammar_source = """
    start: expr+ NEWLINE? ENDMARKER
    expr: NAME {PyTuple_New(-1)}
    """
    grammar = parse_string(grammar_source, GrammarParser)
    extension = generate_parser_c_extension(grammar, tmp_path)
    # PyTuple_New raises SystemError if an invalid argument was passed.
    with pytest.raises(SystemError):
        extension.parse_string("a")
