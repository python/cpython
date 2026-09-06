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
from pegen.grammar import (
    Alt,
    Cut,
    Forced,
    Gather,
    Grammar,
    GrammarError,
    GrammarVisitor,
    Group,
    Lookahead,
    NamedItem,
    NameLeaf,
    NegativeLookahead,
    Opt,
    PositiveLookahead,
    Repeat0,
    Repeat1,
    Rhs,
    Rule,
    Splice,
    StringLeaf,
)
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
    generated_source_path: str,
    build_dir: str | None = None,
    verbose: bool = False,
    keep_asserts: bool = True,
    disable_optimization: bool = False,
    library_dir: str | None = None,
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
    from setuptools import Distribution, Extension
    from setuptools._distutils.ccompiler import new_compiler
    from setuptools._distutils.sysconfig import customize_compiler
    from setuptools.modified import newer_group

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
        str(MOD_DIR.parent.parent.parent / "Parser" / "lexer" / "number.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "lexer" / "state.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "lexer" / "string.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "lexer" / "buffer.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "tokenizer" / "decoder.c"),
        str(MOD_DIR.parent.parent.parent / "Parser" / "tokenizer" / "reader.c"),
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
    library_dirs: list[str] = []
    if sys.platform == "win32":
        # HACK: The location of pyconfig.h has moved within our build, and
        # setuptools hasn't updated for it yet. So add the path manually for now
        include_dirs.append(str(pathlib.Path(sysconfig.get_config_h_filename()).parent))
        if sysconfig.is_python_build():
            # HACK: Our output directory for free-threaded builds has moved, and so
            # tests running in-tree require our sys.executable directory for libs
            library_dirs.append(str(pathlib.Path(sys.executable).parent))
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
    cmd.library_dirs = library_dirs
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


def _is_splice(alt: Alt) -> bool:
    return len(alt.items) == 1 and isinstance(alt.items[0].item, Splice)


def _alt_key(alt: Alt) -> str:
    # A normalized representation of an alternative:
    # str() ignores actions and variable names.
    return str(alt)


class _HeadSymbolsVisitor(GrammarVisitor):
    """Compute the symbols which can start a match of a node.

    Returns a pair: the set of symbols (rule names, token names and
    string literals with quotes), and whether the node can match
    nothing.
    """

    def visit_NamedItem(self, node: NamedItem) -> tuple[set[str], bool]:
        return self.visit(node.item)

    def visit_NameLeaf(self, node: NameLeaf) -> tuple[set[str], bool]:
        return {node.value}, False

    def visit_StringLeaf(self, node: StringLeaf) -> tuple[set[str], bool]:
        return {node.value}, False

    def visit_Rhs(self, node: Rhs) -> tuple[set[str], bool]:
        symbols: set[str] = set()
        nullable = False
        for alt in node.alts:
            alt_nullable = True
            for item in alt.items:
                item_symbols, item_nullable = self.visit(item)
                symbols |= item_symbols
                if not item_nullable:
                    alt_nullable = False
                    break
            nullable = nullable or alt_nullable
        return symbols, nullable

    def visit_Group(self, node: Group) -> tuple[set[str], bool]:
        return self.visit(node.rhs)

    def visit_Opt(self, node: Opt) -> tuple[set[str], bool]:
        return self.visit(node.node)[0], True

    def visit_Repeat0(self, node: Repeat0) -> tuple[set[str], bool]:
        return self.visit(node.node)[0], True

    def visit_Repeat1(self, node: Repeat1) -> tuple[set[str], bool]:
        return self.visit(node.node)[0], False

    def visit_Gather(self, node: Gather) -> tuple[set[str], bool]:
        return self.visit(node.node)[0], False

    def visit_Forced(self, node: Forced) -> tuple[set[str], bool]:
        return self.visit(node.node)[0], False

    def visit_PositiveLookahead(self, node: PositiveLookahead) -> tuple[set[str], bool]:
        return set(), True

    def visit_NegativeLookahead(self, node: NegativeLookahead) -> tuple[set[str], bool]:
        return set(), True

    def visit_Cut(self, node: Cut) -> tuple[set[str], bool]:
        return set(), True

    def generic_visit(self, node: Any, *args: Any, **kwargs: Any) -> Any:
        raise TypeError(f"Unexpected grammar node {node!r}")


_head_symbols = _HeadSymbolsVisitor().visit


def _first_closure(rules: dict[str, Rule]) -> dict[str, set[str]]:
    """For every rule, compute the set of symbols which can start its match,
    including rule names reachable at the first position, transitively."""
    closure = {name: _head_symbols(rule.rhs)[0] for name, rule in rules.items()}
    changed = True
    while changed:
        changed = False
        for name, symbols in closure.items():
            new = symbols.union(*(closure.get(symbol, ()) for symbol in symbols))
            if len(new) != len(symbols):
                closure[name] = new
                changed = True
    return closure


class _PositionDeducer:
    """Deduce the position at which an alternative is inserted into a rule.

    It is placed before the leftmost base alternative which can succeed
    by matching a proper prefix of the code matched by the inserted
    alternative and would thus shadow it.
    """

    def __init__(self, rules: dict[str, Rule]):
        self.rules = rules
        self.closure = _first_closure(rules)

    def head_set(self, node: Any) -> set[str]:
        symbols = _head_symbols(node)[0]
        return symbols.union(*(self.closure.get(symbol, ()) for symbol in symbols))

    def overlap(self, node1: Any, node2: Any) -> bool:
        """Whether the nodes can match code starting with the same symbol."""
        symbols1, _ = _head_symbols(node1)
        symbols2, _ = _head_symbols(node2)
        return bool(
            self.head_set(node1) & symbols2 or symbols1 & self.head_set(node2)
        )

    def _referenced_rule(self, alt: Alt) -> Rule | None:
        """Return the rule if the alternative is a sole rule reference."""
        if len(alt.items) == 1 and isinstance(alt.items[0].item, NameLeaf):
            return self.rules.get(alt.items[0].item.value)
        return None

    def may_swallow(
        self, base_alt: Alt, inserted_alt: Alt, seen: frozenset[str] = frozenset()
    ) -> bool:
        """Whether base_alt may succeed by matching a proper prefix of
        the code matched by inserted_alt."""
        # A base alternative which is a sole rule reference matches
        # a proper prefix only if one of the rule's alternatives does.
        rule = self._referenced_rule(base_alt)
        if rule is not None and rule.name not in seen:
            return any(
                self.may_swallow(alt, inserted_alt, seen | {rule.name})
                for alt in rule.rhs.alts
            )
        base_items = [named_item.item for named_item in base_alt.items]
        inserted_items = [named_item.item for named_item in inserted_alt.items]
        return self._match_prefix(base_items, inserted_items, seen)

    def _match_prefix(
        self,
        base_items: list[Any],
        inserted_items: list[Any],
        seen: frozenset[str] = frozenset(),
    ) -> bool:
        bi = ei = 0
        while True:
            # Skip non-consuming inserted items (over-approximation).
            while ei < len(inserted_items) and isinstance(
                inserted_items[ei], (Lookahead, Cut)
            ):
                ei += 1
            if bi == len(base_items):
                # A proper prefix only if something is left to match.
                return ei < len(inserted_items)
            base_item = base_items[bi]
            if isinstance(base_item, Cut):
                # A failure after the cut fails the whole rule, so the
                # alternative must be inserted before this one.
                return True
            if isinstance(base_item, Lookahead):
                if ei < len(inserted_items):
                    matches = self.overlap(base_item.node, inserted_items[ei])
                    if isinstance(base_item, NegativeLookahead):
                        blocked = matches
                    else:
                        blocked = not matches
                    if blocked:
                        # The lookahead fails on the continuation.
                        return False
                bi += 1
                continue
            if isinstance(base_item, (Opt, Repeat0)):
                if self._match_prefix(base_items[bi + 1:], inserted_items[ei:], seen):
                    return True
                if ei == len(inserted_items):
                    return False
                if not self.overlap(base_item.node, inserted_items[ei]):
                    return False
                if isinstance(base_item, Opt):
                    bi += 1
                # A repetition stays and can consume more inserted items.
                ei += 1
                continue
            if ei == len(inserted_items):
                # The base alternative is not shorter.
                return False
            inserted_item = inserted_items[ei]
            if str(base_item) == str(inserted_item):
                bi += 1
                ei += 1
                continue
            if isinstance(base_item, NameLeaf):
                # A rule reference can match several inserted items;
                # check the rule's alternatives in its place.
                rule = self.rules.get(base_item.value)
                if rule is not None and rule.name not in seen:
                    rest = base_items[bi + 1:]
                    return any(
                        self._match_prefix(
                            [item.item for item in alt.items] + rest,
                            inserted_items[ei:],
                            seen | {rule.name},
                        )
                        for alt in rule.rhs.alts
                    )
            inserted_heads, _ = _head_symbols(inserted_item)
            if inserted_heads & self.head_set(base_item):
                # The base item is more general: it can match the same
                # code as the inserted item.
                bi += 1
                ei += 1
                continue
            base_heads, _ = _head_symbols(base_item)
            if base_heads & self.head_set(inserted_item):
                # The base item is more specific: it can match a proper
                # prefix of the code matched by the inserted item, but
                # then the base alternative must end there.
                return all(
                    isinstance(item, (Lookahead, Cut))
                    for item in base_items[bi + 1:]
                )
            return False

    def terminal_head_set(self, node: Any) -> set[str]:
        """The token and string symbols which can start a match of the node."""
        return {
            symbol for symbol in self.head_set(node) if symbol not in self.rules
        }

    def may_cover(self, inserted_alt: Alt, base_alt: Alt) -> bool:
        """Whether inserted_alt may match the same code as base_alt or
        its proper prefix (every item is at least as general as the
        corresponding base item).

        Inserting it before base_alt would report a false error on
        valid code re-parsed in the second pass.
        """
        base_items = [
            named_item.item for named_item in base_alt.items
            if not isinstance(named_item.item, (Lookahead, Cut))
        ]
        inserted_items = [
            named_item.item for named_item in inserted_alt.items
            if not isinstance(named_item.item, (Lookahead, Cut))
        ]
        if len(inserted_items) > len(base_items):
            return False
        for base_item, inserted_item in zip(base_items, inserted_items):
            if str(base_item) == str(inserted_item):
                continue
            if not (
                self.terminal_head_set(base_item)
                <= self.terminal_head_set(inserted_item)
            ):
                return False
        return True

    def deduce_index(self, base_alts: list[Alt], inserted_alt: Alt) -> int | None:
        rule = self._referenced_rule(inserted_alt)
        if rule is not None:
            inserted_alts = rule.rhs.alts
        else:
            inserted_alts = [inserted_alt]
        for index, base_alt in enumerate(base_alts):
            if base_alt.invalid:
                # An alternative inserted by a preceding extension
                # raises an error rather than succeeds, so it cannot
                # shadow the new alternative; their relative order is
                # defined by the order of the extensions.
                continue
            if not any(
                self.may_swallow(base_alt, alt) for alt in inserted_alts
            ):
                continue
            if any(self.may_cover(alt, base_alt) for alt in inserted_alts):
                continue
            return index
        return None


def extend_rule(
    base: Rule,
    extension: Rule,
    deducer: _PositionDeducer,
) -> Rule:
    """Weave the alternatives of an extension rule into the base rule.

    "..." stands for the alternatives of the base rule; the alternatives
    written before it are inserted before all of them.  The position of
    other alternatives is deduced automatically: before the leftmost
    base alternative which would shadow the inserted alternative, or
    after all base alternatives.
    """
    if extension.type is not None and extension.type != base.type:
        raise GrammarError(
            f"Extension of rule {base.name!r} has different type {extension.type!r}"
        )
    base_alts = base.rhs.alts
    base_keys = {_alt_key(alt) for alt in base_alts}
    ext_alts = extension.rhs.alts
    splice_indices = [i for i, alt in enumerate(ext_alts) if _is_splice(alt)]
    if len(splice_indices) > 1:
        raise GrammarError(
            f'Multiple "..." in the extension of rule {base.name!r}'
        )
    if splice_indices:
        splice_index = splice_indices[0]
        if splice_index == 0:
            raise GrammarError(
                f'Redundant "..." at the start of the extension of rule '
                f"{base.name!r}"
            )
        prepended = ext_alts[:splice_index]
        deduced = ext_alts[splice_index + 1:]
    else:
        prepended = []
        deduced = ext_alts
    appended: list[Alt] = []
    # Inserted alternatives to be emitted before the base alternative
    # with the corresponding index.
    inserted: list[list[Alt]] = [[] for _ in base_alts]
    for alt in prepended + deduced:
        if _alt_key(alt) in base_keys:
            raise GrammarError(
                f"Extension of rule {base.name!r} duplicates the alternative "
                f"{_alt_key(alt)!r}"
            )
        # Inserted alternatives are only used in the second parsing pass.
        alt.invalid = True
    for alt in deduced:
        index = deducer.deduce_index(base_alts, alt)
        if index is None:
            appended.append(alt)
        else:
            inserted[index].append(alt)
    new_alts = list(prepended)
    for inserted_alts, base_alt in zip(inserted, base_alts):
        new_alts.extend(inserted_alts)
        new_alts.append(base_alt)
    new_alts.extend(appended)
    return Rule(base.name, base.type, Rhs(new_alts), flags=base.flags)


def merge_grammars(grammars: Sequence[Grammar]) -> Grammar:
    """Merge grammars into a single grammar.

    A rule with the "extend" flag adds alternatives to the rule with
    the same name defined in another grammar (see extend_rule()).
    Other rule names and meta names must not be repeated between the
    grammars.
    """
    rules: dict[str, Rule] = {}
    metas: dict[str, str | None] = {}
    extensions: list[Rule] = []
    for grammar in grammars:
        for rule in grammar.rules.values():
            if rule.name in rules:
                raise GrammarError(f"Repeated rule {rule.name!r}")
            rules[rule.name] = rule
        extensions.extend(grammar.extensions)
        for name, value in grammar.metas.items():
            if name in metas:
                raise GrammarError(f"Repeated meta {name!r}")
            metas[name] = value
    if extensions:
        # Deduce insertion positions using the rules as defined,
        # before any extension is applied.
        deducer = _PositionDeducer(dict(rules))
        for extension in extensions:
            base = rules.get(extension.name)
            if base is None:
                raise GrammarError(
                    f"Extended rule {extension.name!r} is not defined"
                )
            rules[extension.name] = extend_rule(base, extension, deducer)
    return Grammar(rules.values(), metas.items())


def build_parser(
    grammar_files: str | Sequence[str],
    verbose_tokenizer: bool = False,
    verbose_parser: bool = False,
) -> tuple[Grammar, Parser, Tokenizer]:
    """Parse one or more grammar files and merge them into a single grammar.

    The returned parser and tokenizer are those of the last grammar file.
    """
    if isinstance(grammar_files, (str, os.PathLike)):
        grammar_files = [grammar_files]
    grammars: list[Grammar] = []
    for grammar_file in grammar_files:
        with open(grammar_file) as file:
            tokenizer = Tokenizer(
                tokenize.generate_tokens(file.readline), verbose=verbose_tokenizer
            )
            parser = GrammarParser(tokenizer, verbose=verbose_parser)
            grammar = parser.start()

            if not grammar:
                raise parser.make_syntax_error(grammar_file)
        grammars.append(grammar)

    if len(grammars) > 1:
        grammar = merge_grammars(grammars)
    else:
        grammar = grammars[0]
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
    with open(tokens_file) as tok_file:
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


def grammars_display_name(grammar_files: str | Sequence[str]) -> str:
    if isinstance(grammar_files, (str, os.PathLike)):
        return os.fspath(grammar_files)
    return ", ".join(os.path.basename(grammar_file) for grammar_file in grammar_files)


def build_c_parser_and_generator(
    grammar_files: str | Sequence[str],
    tokens_file: str,
    output_file: str,
    compile_extension: bool = False,
    verbose_tokenizer: bool = False,
    verbose_parser: bool = False,
    verbose_c_extension: bool = False,
    keep_asserts_in_extension: bool = True,
    skip_actions: bool = False,
) -> tuple[Grammar, Parser, Tokenizer, ParserGenerator]:
    """Generate rules, C parser, tokenizer, parser generator for a given grammar

    Args:
        grammar_files (string or sequence of strings): Path(s) for the grammar file(s)
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
    grammar, parser, tokenizer = build_parser(grammar_files, verbose_tokenizer, verbose_parser)
    gen = build_c_generator(
        grammar,
        grammars_display_name(grammar_files),
        tokens_file,
        output_file,
        compile_extension,
        verbose_c_extension,
        keep_asserts_in_extension,
        skip_actions=skip_actions,
    )

    return grammar, parser, tokenizer, gen


def build_python_parser_and_generator(
    grammar_files: str | Sequence[str],
    output_file: str,
    verbose_tokenizer: bool = False,
    verbose_parser: bool = False,
    skip_actions: bool = False,
) -> tuple[Grammar, Parser, Tokenizer, ParserGenerator]:
    """Generate rules, python parser, tokenizer, parser generator for a given grammar

    Args:
        grammar_files (string or sequence of strings): Path(s) for the grammar file(s)
        output_file (string): Path for the output file
        verbose_tokenizer (bool, optional): Whether to display additional output
          when generating the tokenizer. Defaults to False.
        verbose_parser (bool, optional): Whether to display additional output
          when generating the parser. Defaults to False.
        skip_actions (bool, optional): Whether to pretend no rule has any actions.
    """
    grammar, parser, tokenizer = build_parser(grammar_files, verbose_tokenizer, verbose_parser)
    gen = build_python_generator(
        grammar,
        grammars_display_name(grammar_files),
        output_file,
        skip_actions=skip_actions,
    )
    return grammar, parser, tokenizer, gen
