import os
from pathlib import PurePath
from typing import Any, Dict, Tuple
from textwrap import dedent

import pytest  # type: ignore

from pegen.grammar_parser import GeneratedParser as GrammarParser
from pegen.testutil import parse_string, generate_parser_c_extension

# fmt: off

FSTRINGS: Dict[str, Tuple[str, str]] = {
    'multiline_fstrings_same_line_with_brace': (
        """
            f'''
            {a$b}
            '''
        """,
        '(a$b)',
    ),
    'multiline_fstring_brace_on_next_line': (
        """
            f'''
            {a$b
            }'''
        """,
        '(a$b',
    ),
    'multiline_fstring_brace_on_previous_line': (
        """
            f'''
            {
            a$b}'''
        """,
        'a$b)',
    ),
}

# fmt: on


def create_tmp_extension(tmp_path: PurePath) -> Any:
    with open(os.path.join("data", "python.gram"), "r") as grammar_file:
        grammar_source = grammar_file.read()
    grammar = parse_string(grammar_source, GrammarParser)
    extension = generate_parser_c_extension(grammar, tmp_path)
    return extension


@pytest.fixture(scope="module")
def parser_extension(tmp_path_factory: Any) -> Any:
    tmp_path = tmp_path_factory.mktemp("extension")
    extension = create_tmp_extension(tmp_path)
    return extension


@pytest.mark.parametrize("fstring,error_line", FSTRINGS.values(), ids=tuple(FSTRINGS.keys()))
def test_fstring_syntax_error_tracebacks(
    parser_extension: Any, fstring: str, error_line: str
) -> None:
    try:
        parser_extension.parse_string(dedent(fstring))
    except SyntaxError as se:
        assert se.text == error_line
