import pathlib
import shutil
import tokenize
import sysconfig
import tempfile
import itertools

from typing import Optional, Tuple, List, IO, Set, Dict

from pegen.c_generator import CParserGenerator
from pegen.grammar import Grammar
from pegen.grammar_parser import GeneratedParser as GrammarParser
from pegen.parser import Parser
from pegen.parser_generator import ParserGenerator
from pegen.python_generator import PythonParserGenerator
from pegen.tokenizer import Tokenizer

MOD_DIR = pathlib.Path(__file__).resolve().parent

TokenDefinitions = Tuple[Dict[int, str], Dict[str, int], Set[str]]


def get_extra_flags(compiler_flags: str, compiler_py_flags_nodist: str) -> List[str]:
    flags = sysconfig.get_config_var(compiler_flags)
    py_flags_nodist = sysconfig.get_config_var(compiler_py_flags_nodist)
    if flags is None or py_flags_nodist is None:
        return []
    return f"{flags} {py_flags_nodist}".split()


def compile_c_extension(
    generated_source_path: str,
    build_dir: Optional[str] = None,
    verbose: bool = False,
    keep_asserts: bool = True,
) -> str:
    """Compile the generated source for a parser generator into an extension module.

    The extension module will be generated in the same directory as the provided path
    for the generated source, with the same basename (in addition to extension module
    metadata). For example, for the source mydir/parser.c the generated extension
    in a darwin system with python 3.8 will be mydir/parser.cpython-38-darwin.so.

    If *build_dir* is provided, that path will be used as the temporary build directory
    of distutils (this is useful in case you want to use a temporary directory).
    """
    import distutils.log
    from distutils.core import Distribution, Extension
    from distutils.command.clean import clean  # type: ignore
    from distutils.command.build_ext import build_ext  # type: ignore
    from distutils.tests.support import fixup_build_ext  # type: ignore

    if verbose:
        distutils.log.set_verbosity(distutils.log.DEBUG)

    source_file_path = pathlib.Path(generated_source_path)
    extension_name = source_file_path.stem
    extra_compile_args = get_extra_flags("CFLAGS", "PY_CFLAGS_NODIST")
    extra_link_args = get_extra_flags("LDFLAGS", "PY_LDFLAGS_NODIST")
    if keep_asserts:
        extra_compile_args.append("-UNDEBUG")
    extension = [
        Extension(
            extension_name,
            sources=[
                str(MOD_DIR.parent.parent.parent / "Python" / "Python-ast.c"),
                str(MOD_DIR.parent.parent.parent / "Python" / "asdl.c"),
                str(MOD_DIR.parent.parent.parent / "Parser" / "tokenizer.c"),
                str(MOD_DIR.parent.parent.parent / "Parser" / "pegen.c"),
                str(MOD_DIR.parent.parent.parent / "Parser" / "string_parser.c"),
                str(MOD_DIR.parent / "peg_extension" / "peg_extension.c"),
                generated_source_path,
            ],
            include_dirs=[
                str(MOD_DIR.parent.parent.parent / "Include" / "internal"),
                str(MOD_DIR.parent.parent.parent / "Parser"),
            ],
            extra_compile_args=extra_compile_args,
            extra_link_args=extra_link_args,
        )
    ]
    dist = Distribution({"name": extension_name, "ext_modules": extension})
    cmd = build_ext(dist)
    fixup_build_ext(cmd)
    cmd.inplace = True
    if build_dir:
        cmd.build_temp = build_dir
        cmd.build_lib = build_dir
    cmd.ensure_finalized()
    cmd.run()

    extension_path = source_file_path.parent / cmd.get_ext_filename(extension_name)
    shutil.move(cmd.get_ext_fullpath(extension_name), extension_path)

    cmd = clean(dist)
    cmd.finalize_options()
    cmd.run()

    return extension_path


def build_parser(
    grammar_file: str, verbose_tokenizer: bool = False, verbose_parser: bool = False
) -> Tuple[Grammar, Parser, Tokenizer]:
    with open(grammar_file) as file:
        tokenizer = Tokenizer(tokenize.generate_tokens(file.readline), verbose=verbose_tokenizer)
        parser = GrammarParser(tokenizer, verbose=verbose_parser)
        grammar = parser.start()

        if not grammar:
            raise parser.make_syntax_error(grammar_file)

    return grammar, parser, tokenizer


def generate_token_definitions(tokens: IO[str]) -> TokenDefinitions:
    all_tokens = {}
    exact_tokens = {}
    non_exact_tokens = set()
    numbers = itertools.count(0)

    for line in tokens:
        line = line.strip()

        if not line or line.startswith("#"):
            continue

        pieces = line.split()
        index = next(numbers)

        if len(pieces) == 1:
            (token,) = pieces
            non_exact_tokens.add(token)
            all_tokens[index] = token
        elif len(pieces) == 2:
            token, op = pieces
            exact_tokens[op.strip("'")] = index
            all_tokens[index] = token
        else:
            raise ValueError(f"Unexpected line found in Tokens file: {line}")

    return all_tokens, exact_tokens, non_exact_tokens


def build_c_generator(
    grammar: Grammar,
    grammar_file: str,
    tokens_file: str,
    output_file: str,
    compile_extension: bool = False,
    verbose_c_extension: bool = False,
    keep_asserts_in_extension: bool = True,
    skip_actions: bool = False,
) -> ParserGenerator:
    with open(tokens_file, "r") as tok_file:
        all_tokens, exact_tok, non_exact_tok = generate_token_definitions(tok_file)
    with open(output_file, "w") as file:
        gen: ParserGenerator = CParserGenerator(
            grammar, all_tokens, exact_tok, non_exact_tok, file, skip_actions=skip_actions
        )
        gen.generate(grammar_file)

    if compile_extension:
        with tempfile.TemporaryDirectory() as build_dir:
            compile_c_extension(
                output_file,
                build_dir=build_dir,
                verbose=verbose_c_extension,
                keep_asserts=keep_asserts_in_extension,
            )
    return gen


def build_python_generator(
    grammar: Grammar, grammar_file: str, output_file: str, skip_actions: bool = False,
) -> ParserGenerator:
    with open(output_file, "w") as file:
        gen: ParserGenerator = PythonParserGenerator(grammar, file)  # TODO: skip_actions
        gen.generate(grammar_file)
    return gen


def build_c_parser_and_generator(
    grammar_file: str,
    tokens_file: str,
    output_file: str,
    compile_extension: bool = False,
    verbose_tokenizer: bool = False,
    verbose_parser: bool = False,
    verbose_c_extension: bool = False,
    keep_asserts_in_extension: bool = True,
    skip_actions: bool = False,
) -> Tuple[Grammar, Parser, Tokenizer, ParserGenerator]:
    """Generate rules, C parser, tokenizer, parser generator for a given grammar

    Args:
        grammar_file (string): Path for the grammar file
        tokens_file (string): Path for the tokens file
        output_file (string): Path for the output file
        compile_extension (bool, optional): Whether to compile the C extension.
          Defaults to False.
        verbose_tokenizer (bool, optional): Whether to display additional output
          when generating the tokenizer. Defaults to False.
        verbose_parser (bool, optional): Whether to display additional output
          when generating the parser. Defaults to False.
        verbose_c_extension (bool, optional): Whether to display additional
          output when compiling the C extension . Defaults to False.
        keep_asserts_in_extension (bool, optional): Whether to keep the assert statements
          when compiling the extension module. Defaults to True.
        skip_actions (bool, optional): Whether to pretend no rule has any actions.
    """
    grammar, parser, tokenizer = build_parser(grammar_file, verbose_tokenizer, verbose_parser)
    gen = build_c_generator(
        grammar,
        grammar_file,
        tokens_file,
        output_file,
        compile_extension,
        verbose_c_extension,
        keep_asserts_in_extension,
        skip_actions=skip_actions,
    )

    return grammar, parser, tokenizer, gen


def build_python_parser_and_generator(
    grammar_file: str,
    output_file: str,
    verbose_tokenizer: bool = False,
    verbose_parser: bool = False,
    skip_actions: bool = False,
) -> Tuple[Grammar, Parser, Tokenizer, ParserGenerator]:
    """Generate rules, python parser, tokenizer, parser generator for a given grammar

    Args:
        grammar_file (string): Path for the grammar file
        output_file (string): Path for the output file
        verbose_tokenizer (bool, optional): Whether to display additional output
          when generating the tokenizer. Defaults to False.
        verbose_parser (bool, optional): Whether to display additional output
          when generating the parser. Defaults to False.
        skip_actions (bool, optional): Whether to pretend no rule has any actions.
    """
    grammar, parser, tokenizer = build_parser(grammar_file, verbose_tokenizer, verbose_parser)
    gen = build_python_generator(grammar, grammar_file, output_file, skip_actions=skip_actions,)
    return grammar, parser, tokenizer, gen
