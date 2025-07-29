import itertools
import logging
import os
import pathlib
import sys
import sysconfig
import tempfile
import tokenize
from typing import IO, Any, Dict, List, Optional, Set, Tuple

from pegen.c_generator import CParserGenerator
from pegen.grammar import Grammar
from pegen.grammar_parser import GeneratedParser as GrammarParser
from pegen.parser import Parser
from pegen.parser_generator import ParserGenerator
from pegen.python_generator import PythonParserGenerator
from pegen.tokenizer import Tokenizer

MOD_DIR = pathlib.Path(__file__).resolve().parent

TokenDefinitions = Tuple[Dict[int, str], Dict[str, int], Set[str]]
Incomplete = Any  # TODO: install `types-setuptools` and remove this alias


def get_extra_flags(compiler_flags: str, compiler_py_flags_nodist: str) -> List[str]:
    flags = sysconfig.get_config_var(compiler_flags)
    py_flags_nodist = sysconfig.get_config_var(compiler_py_flags_nodist)
    if flags is None or py_flags_nodist is None:
        return []
    return f"{flags} {py_flags_nodist}".split()


def fixup_build_ext(cmd: Incomplete) -> None:
    """Function needed to make build_ext tests pass.

    When Python was built with --enable-shared on Unix, -L. is not enough to
    find libpython<blah>.so, because regrtest runs in a tempdir, not in the
    source directory where the .so lives.

    When Python was built with in debug mode on Windows, build_ext commands
    need their debug attribute set, and it is not done automatically for
    some reason.

    This function handles both of these things.  Example use:

        cmd = build_ext(dist)
        support.fixup_build_ext(cmd)
        cmd.ensure_finalized()

    Unlike most other Unix platforms, Mac OS X embeds absolute paths
    to shared libraries into executables, so the fixup is not needed there.

    Taken from distutils (was part of the CPython stdlib until Python 3.11)
    """
    if os.name == "nt":
        cmd.debug = sys.executable.endswith("_d.exe")
    elif sysconfig.get_config_var("Py_ENABLE_SHARED"):
        # To further add to the shared builds fun on Unix, we can't just add
        # library_dirs to the Extension() instance because that doesn't get
        # plumbed through to the final compiler command.
        runshared = sysconfig.get_config_var("RUNSHARED")
        if runshared is None:
            cmd.library_dirs = ["."]
        else:
            if sys.platform == "darwin":
                cmd.library_dirs = []
            else:
                name, equals, value = runshared.partition("=")
                cmd.library_dirs = [d for d in value.split(os.pathsep) if d]


def compile_c_extension(
    generated_source_path: str,
    build_dir: Optional[str] = None,
    verbose: bool = False,
    keep_asserts: bool = True,
    disable_optimization: bool = False,
    library_dir: Optional[str] = None,
) -> pathlib.Path:
    """Compile the generated source for a parser generator into an extension module.

    The extension module will be generated in the same directory as the provided path
    for the generated source, with the same basename (in addition to extension module
    metadata). For example, for the source mydir/parser.c the generated extension
    in a darwin system with python 3.8 will be mydir/parser.cpython-38-darwin.so.

    If *build_dir* is provided, that path will be used as the temporary build directory
    of distutils (this is useful in case you want to use a temporary directory).

    If *library_dir* is provided, that path will be used as the directory for a
    static library of the common parser sources (this is useful in case you are
    creating multiple extensions).
    """
    import setuptools.command.build_ext
    import setuptools.logging

    from setuptools import Extension, Distribution
    from setuptools.modified import newer_group
    from setuptools._distutils.ccompiler import new_compiler
    from setuptools._distutils.sysconfig import customize_compiler

    if verbose:
        setuptools.logging.set_threshold(logging.DEBUG)

    source_file_path = pathlib.Path(generated_source_path)
    extension_name = source_file_path.stem
    extra_compile_args = get_extra_flags("CFLAGS", "PY_CFLAGS_NODIST")
    extra_compile_args.append("-DPy_BUILD_CORE_MODULE")
    # Define _Py_TEST_PEGEN to not call PyAST_Validate() in Parser/pegen.c
    extra_compile_args.append("-D_Py_TEST_PEGEN")
    if sys.platform == "win32" and sysconfig.get_config_var("Py_GIL_DISABLED"):
        extra_compile_args.append("-DPy_GIL_DISABLED")
    extra_link_args = get_extra_flags("LDFLAGS", "PY_LDFLAGS_NODIST")
    if keep_asserts:
        extra_compile_args.append("-UNDEBUG")
    if disable_optimization:
        if sys.platform == "win32":
            extra_compile_args.append("/Od")
            extra_link_args.append("/LTCG:OFF")
        else:
            extra_compile_args.append("-O0")
            if sysconfig.get_config_var("GNULD") == "yes":
                extra_link_args.append("-fno-lto")

    common_sources = [
        str(MOD_DIR.parent.parent.parent / "Python" / "Python-ast.c"),
        str(MOD_DIR.parent.parent.parent / "Python" / "asdl.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "lexer" / "lexer.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "lexer" / "state.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "lexer" / "buffer.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "tokenizer" / "string_tokenizer.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "tokenizer" / "file_tokenizer.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "tokenizer" / "utf8_tokenizer.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "tokenizer" / "readline_tokenizer.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "tokenizer" / "helpers.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "pegen.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "pegen_errors.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "action_helpers.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "string_parser.c"),
        str(MOD_DIR.parent / "peg_extension" / "peg_extension.c"),
    ]
    include_dirs = [
        str(MOD_DIR.parent.parent.parent / "Include" / "internal"),
        str(MOD_DIR.parent.parent.parent / "Include" / "internal" / "mimalloc"),
        str(MOD_DIR.parent.parent.parent / "Parser"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "lexer"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "tokenizer"),
    ]
    if sys.platform == "win32":
        # HACK: The location of pyconfig.h has moved within our build, and
        # setuptools hasn't updated for it yet. So add the path manually for now
        include_dirs.append(pathlib.Path(sysconfig.get_config_h_filename()).parent)
    extension = Extension(
        extension_name,
        sources=[generated_source_path],
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
    )
    dist = Distribution({"name": extension_name, "ext_modules": [extension]})
    cmd = dist.get_command_obj("build_ext")
    assert isinstance(cmd, setuptools.command.build_ext.build_ext)
    fixup_build_ext(cmd)
    cmd.build_lib = str(source_file_path.parent)
    cmd.include_dirs = include_dirs
    if build_dir:
        cmd.build_temp = build_dir
    cmd.ensure_finalized()

    compiler = new_compiler()
    customize_compiler(compiler)
    compiler.set_include_dirs(cmd.include_dirs)
    compiler.set_library_dirs(cmd.library_dirs)
    # build static lib
    if library_dir:
        library_filename = compiler.library_filename(extension_name, output_dir=library_dir)
        if newer_group(common_sources, library_filename, "newer"):
            if sys.platform == "win32":
                assert compiler.static_lib_format
                pdb = compiler.static_lib_format % (extension_name, ".pdb")
                compile_opts = [f"/Fd{library_dir}\\{pdb}"]
                compile_opts.extend(extra_compile_args)
            else:
                compile_opts = extra_compile_args
            objects = compiler.compile(
                common_sources,
                output_dir=library_dir,
                debug=cmd.debug,
                extra_postargs=compile_opts,
            )
            compiler.create_static_lib(
                objects, extension_name, output_dir=library_dir, debug=cmd.debug
            )
        if sys.platform == "win32":
            compiler.add_library_dir(library_dir)
            extension.libraries = [extension_name]
        elif sys.platform == "darwin":
            compiler.set_link_objects(
                [
                    "-Wl,-force_load",
                    library_filename,
                ]
            )
        else:
            compiler.set_link_objects(
                [
                    "-Wl,--whole-archive",
                    library_filename,
                    "-Wl,--no-whole-archive",
                ]
            )
    else:
        extension.sources[0:0] = common_sources

    # Compile the source code to object files.
    ext_path = cmd.get_ext_fullpath(extension_name)
    if newer_group(extension.sources, ext_path, "newer"):
        objects = compiler.compile(
            extension.sources,
            output_dir=cmd.build_temp,
            debug=cmd.debug,
            extra_postargs=extra_compile_args,
        )
    else:
        objects = compiler.object_filenames(extension.sources, output_dir=cmd.build_temp)
    # The cmd.get_libraries() call needs a valid compiler attribute or we will
    # get an incorrect library name on the free-threaded Windows build.
    cmd.compiler = compiler
    # Now link the object files together into a "shared object"
    compiler.link_shared_object(
        objects,
        ext_path,
        libraries=cmd.get_libraries(extension),
        extra_postargs=extra_link_args,
        export_symbols=cmd.get_export_symbols(extension),  # type: ignore[no-untyped-call]
        debug=cmd.debug,
        build_temp=cmd.build_temp,
    )

    return pathlib.Path(ext_path)


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
    grammar: Grammar,
    grammar_file: str,
    output_file: str,
    skip_actions: bool = False,
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
    gen = build_python_generator(
        grammar,
        grammar_file,
        output_file,
        skip_actions=skip_actions,
    )
    return grammar, parser, tokenizer, gen
