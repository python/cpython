import contextlib
import itertools
import logging
import os
import pathlib
import sys
import sysconfig
import tempfile
import tokenize
from collections.abc import Sequence
from typing import IO, Any

from pegen.c_generator import CParserGenerator
from pegen.grammar import Grammar, GrammarError
from pegen.grammar_parser import GeneratedParser as GrammarParser
from pegen.parser import Parser
from pegen.parser_generator import ParserGenerator
from pegen.python_generator import PythonParserGenerator
from pegen.tokenizer import Tokenizer

MOD_DIR = pathlib.Path(__file__).resolve().parent

TokenDefinitions = tuple[dict[int, str], dict[str, int], set[str]]
Incomplete = Any  # TODO: install `types-setuptools` and remove this alias


def get_extra_flags(compiler_flags: str, compiler_py_flags_nodist: str) -> list[str]:
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
    generated_source_path: pathlib.Path,
    build_dir: str | None = None,
    verbose: bool = False,
    keep_asserts: bool = True,
    disable_optimization: bool = False,
    library_dir: str | None = None,
    extra_generated_sources: Sequence[pathlib.Path] = (),
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

    *extra_generated_sources* are the C files generated for the included
    grammar files (see included_output_files()).
    """
    import setuptools.command.build_ext
    import setuptools.logging
    from setuptools import Distribution, Extension
    from setuptools._distutils.ccompiler import new_compiler
    from setuptools._distutils.sysconfig import customize_compiler
    from setuptools.modified import newer_group

    if verbose:
        setuptools.logging.set_threshold(logging.DEBUG)

    extension_name = generated_source_path.stem
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
        MOD_DIR.parent.parent.parent / "Python" / "Python-ast.c",
        MOD_DIR.parent.parent.parent / "Python" / "asdl.c",
        MOD_DIR.parent.parent.parent / "Parser" / "lexer" / "lexer.c",
        MOD_DIR.parent.parent.parent / "Parser" / "lexer" / "layout.c",
        MOD_DIR.parent.parent.parent / "Parser" / "lexer" / "number.c",
        MOD_DIR.parent.parent.parent / "Parser" / "lexer" / "state.c",
        MOD_DIR.parent.parent.parent / "Parser" / "lexer" / "string.c",
        MOD_DIR.parent.parent.parent / "Parser" / "tokenizer" / "decoder.c",
        MOD_DIR.parent.parent.parent / "Parser" / "tokenizer" / "api.c",
        MOD_DIR.parent.parent.parent / "Parser" / "tokenizer" / "reader.c",
        MOD_DIR.parent.parent.parent / "Parser" / "tokenizer" / "helpers.c",
        MOD_DIR.parent.parent.parent / "Parser" / "pegen.c",
        MOD_DIR.parent.parent.parent / "Parser" / "pegen_errors.c",
        MOD_DIR.parent.parent.parent / "Parser" / "action_helpers.c",
        MOD_DIR.parent.parent.parent / "Parser" / "string_parser.c",
        MOD_DIR.parent / "peg_extension" / "peg_extension.c",
    ]
    include_dirs = [
        MOD_DIR.parent.parent.parent / "Include" / "internal",
        MOD_DIR.parent.parent.parent / "Include" / "internal" / "mimalloc",
        MOD_DIR.parent.parent.parent / "Parser",
        MOD_DIR.parent.parent.parent / "Parser" / "lexer",
        MOD_DIR.parent.parent.parent / "Parser" / "tokenizer",
    ]
    library_dirs: list[pathlib.Path] = []
    if sys.platform == "win32":
        # HACK: The location of pyconfig.h has moved within our build, and
        # setuptools hasn't updated for it yet. So add the path manually for now
        include_dirs.append(pathlib.Path(sysconfig.get_config_h_filename()).parent)
        if sysconfig.is_python_build():
            # HACK: Our output directory for free-threaded builds has moved, and so
            # tests running in-tree require our sys.executable directory for libs
            library_dirs.append(pathlib.Path(sys.executable).parent)
    sources = [generated_source_path, *extra_generated_sources]
    extension = Extension(
        extension_name,
        sources=sources,
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
    )
    dist = Distribution({"name": extension_name, "ext_modules": [extension]})
    cmd = dist.get_command_obj("build_ext")
    assert isinstance(cmd, setuptools.command.build_ext.build_ext)
    fixup_build_ext(cmd)
    cmd.build_lib = str(generated_source_path.parent)
    cmd.include_dirs = [str(d) for d in include_dirs]
    cmd.library_dirs = [str(d) for d in library_dirs]
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
        sources[0:0] = common_sources

    # Compile the source code to object files.
    ext_path = cmd.get_ext_fullpath(extension_name)
    if newer_group(sources, ext_path, "newer"):
        objects = compiler.compile(
            sources,
            output_dir=cmd.build_temp,
            debug=cmd.debug,
            extra_postargs=extra_compile_args,
        )
    else:
        objects = compiler.object_filenames(sources, output_dir=cmd.build_temp)
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


def _parse_grammar_file(
    grammar_file: pathlib.Path, verbose_tokenizer: bool, verbose_parser: bool
) -> tuple[Grammar, Parser, Tokenizer]:
    with open(grammar_file) as file:
        tokenizer = Tokenizer(tokenize.generate_tokens(file.readline), verbose=verbose_tokenizer)
        parser = GrammarParser(tokenizer, verbose=verbose_parser)
        grammar = parser.start()

        if not grammar:
            raise parser.make_syntax_error(str(grammar_file))

    return grammar, parser, tokenizer


def _include_rules(
    root_grammar: Grammar,
    root_dir: pathlib.Path,
    includes: list[str],
    including_file: pathlib.Path,
    rule_files: dict[str, pathlib.Path],
    include_stack: list[pathlib.Path],
    included_from: dict[pathlib.Path, pathlib.Path],
    verbose_tokenizer: bool,
    verbose_parser: bool,
) -> None:
    # Append the rules of each included file (and, depth-first, of the files
    # it includes) to *root_grammar*, in order.
    for include in includes:
        path = (including_file.parent / include).resolve()
        if path in include_stack:
            raise GrammarError(f"Circular @include of '{path!s}' in '{including_file!s}'")
        if path in included_from:
            raise GrammarError(
                f"'{path}' is included more than once: "
                f"from '{included_from[path]}' and from '{including_file}'"
            )
        included_from[path] = including_file
        new_grammar, _, _ = _parse_grammar_file(path, verbose_tokenizer, verbose_parser)
        if new_grammar.metas:
            names = ", ".join(f"@{name}" for name in new_grammar.metas)
            raise GrammarError(f"Included grammar '{path!s}' cannot define metas: {names}")
        for name, rule in new_grammar.rules.items():
            if name in root_grammar.rules:
                raise GrammarError(
                    f"Repeated rule {name!r} in '{path!s}' "
                    f"(first defined in '{rule_files[name]!s}')"
                )
            rule.source = pathlib.Path(os.path.relpath(path, root_dir))
            root_grammar.rules[name] = rule
            rule_files[name] = path
        _include_rules(
            root_grammar,
            root_dir,
            new_grammar.includes,
            path,
            rule_files,
            include_stack + [path],
            included_from,
            verbose_tokenizer,
            verbose_parser,
        )


def build_parser(
    grammar_file: pathlib.Path, verbose_tokenizer: bool = False, verbose_parser: bool = False
) -> tuple[Grammar, Parser, Tokenizer]:
    grammar, parser, tokenizer = _parse_grammar_file(
        grammar_file, verbose_tokenizer, verbose_parser
    )
    resolve_includes(grammar, grammar_file, verbose_tokenizer, verbose_parser)
    return grammar, parser, tokenizer


def resolve_includes(
    grammar: Grammar,
    grammar_file: pathlib.Path,
    verbose_tokenizer: bool = False,
    verbose_parser: bool = False,
) -> None:
    """Add the rules of the files that *grammar* includes, and clear its includes.

    *grammar_file* is the file *grammar* was parsed from: its @include paths
    are relative to that file's directory.
    """
    _include_rules(
        grammar,
        grammar_file.resolve().parent,
        grammar.includes,
        grammar_file,
        dict.fromkeys(grammar.rules, grammar_file),
        [grammar_file.resolve()],
        {},
        verbose_tokenizer,
        verbose_parser,
    )
    grammar.includes = []


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


def included_output_files(
    grammar: Grammar, output_file: pathlib.Path
) -> dict[pathlib.Path, pathlib.Path]:
    stem, dot, rest = output_file.name.partition(".")
    outputs: dict[pathlib.Path, pathlib.Path] = {}
    for rule in grammar.rules.values():
        if rule.source is None or rule.source in outputs:
            continue
        if rule.source.is_absolute() or ".." in rule.source.parts:
            raise GrammarError(
                f"The C file for '{rule.source}' would be outside the output directory; "
                "included grammar files must be in the root grammar file's directory "
                "or its subdirectories"
            )
        path = (
            output_file.parent
            / rule.source.parent
            / f"{stem}_{rule.source.stem}{dot}{rest}"
        )
        if path in outputs.values():
            raise GrammarError(
                f"The C file for '{rule.source}' would be '{path}', "
                "which is already used by another grammar file"
            )
        outputs[rule.source] = path
    return outputs


def build_c_generator(
    grammar: Grammar,
    grammar_file: pathlib.Path,
    tokens_file: pathlib.Path,
    output_file: pathlib.Path,
    compile_extension: bool = False,
    verbose_c_extension: bool = False,
    keep_asserts_in_extension: bool = True,
    skip_actions: bool = False,
) -> ParserGenerator:
    with open(tokens_file) as tok_file:
        all_tokens, exact_tok, non_exact_tok = generate_token_definitions(tok_file)
    included_outputs = included_output_files(grammar, output_file)
    with contextlib.ExitStack() as stack:
        file: IO[str] = stack.enter_context(open(output_file, "w"))
        for path in included_outputs.values():
            path.parent.mkdir(parents=True, exist_ok=True)
        included_files: dict[pathlib.Path, IO[str]] = {
            source: stack.enter_context(open(path, "w"))
            for source, path in included_outputs.items()
        }
        gen: ParserGenerator = CParserGenerator(
            grammar,
            all_tokens,
            exact_tok,
            non_exact_tok,
            file,
            skip_actions=skip_actions,
            included_files=included_files,
        )
        gen.generate(grammar_file)

    if compile_extension:
        with tempfile.TemporaryDirectory() as build_dir:
            compile_c_extension(
                output_file,
                build_dir=build_dir,
                verbose=verbose_c_extension,
                keep_asserts=keep_asserts_in_extension,
                extra_generated_sources=list(included_outputs.values()),
            )
    return gen


def build_python_generator(
    grammar: Grammar,
    grammar_file: pathlib.Path,
    output_file: pathlib.Path,
    skip_actions: bool = False,
) -> ParserGenerator:
    with open(output_file, "w") as file:
        gen: ParserGenerator = PythonParserGenerator(grammar, file)  # TODO: skip_actions
        gen.generate(grammar_file)
    return gen


def build_c_parser_and_generator(
    grammar_file: pathlib.Path,
    tokens_file: pathlib.Path,
    output_file: pathlib.Path,
    compile_extension: bool = False,
    verbose_tokenizer: bool = False,
    verbose_parser: bool = False,
    verbose_c_extension: bool = False,
    keep_asserts_in_extension: bool = True,
    skip_actions: bool = False,
) -> tuple[Grammar, Parser, Tokenizer, ParserGenerator]:
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
    grammar_file: pathlib.Path,
    output_file: pathlib.Path,
    verbose_tokenizer: bool = False,
    verbose_parser: bool = False,
    skip_actions: bool = False,
) -> tuple[Grammar, Parser, Tokenizer, ParserGenerator]:
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
