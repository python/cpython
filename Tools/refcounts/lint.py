"""Lint Doc/data/refcounts.dat."""

from __future__ import annotations

import itertools
import re
import sys
import tomllib
from argparse import ArgumentParser, RawDescriptionHelpFormatter
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Callable, Iterable, Mapping
    from typing import Final, LiteralString, NoReturn

ROOT = Path(__file__).parent.parent.parent.resolve()
DEFAULT_REFCOUNT_DAT_PATH: Final[str] = str(ROOT / "Doc/data/refcounts.dat")
DEFAULT_STABLE_ABI_TOML_PATH: Final[str] = str(ROOT / "Misc/stable_abi.toml")

C_ELLIPSIS: LiteralString = "..."

MATCH_TODO: Callable[[str], re.Match[str] | None]
MATCH_TODO = re.compile(r"^#\s*TODO:\s*(\w+)$").match


def generate_object_types(object_types: Iterable[str]) -> Iterable[str]:
    """Generate the type declarations that expect a reference count."""
    for qualifier, object_type, suffix in itertools.product(
        ("const ", ""),
        object_types,
        (
            "*",
            "**", "* *",
            "*const*", "*const *", "* const*", "* const *",
        ),
    ):
        yield f"{qualifier}{object_type}{suffix}"
        yield f"{qualifier}{object_type} {suffix}"


#: Set of declarations whose variable has a reference count.
OBJECT_TYPES: frozenset[str] = frozenset(generate_object_types((
    "PyObject", "PyVarObject", "PyTypeObject",
    "PyLongObject", "PyCodeObject", "PyFrameObject",
    "PyModuleDef", "PyModuleObject",
)))

#: Set of functions part of the stable ABI that are not
#: in the refcounts.dat file if:
#:
#:  - They are ABI-only (either fully deprecated or even removed).
#:  - They are so internal that they should not be used at all because
#:    they raise a fatal error.
STABLE_ABI_IGNORE_LIST: frozenset[str] = frozenset((
    # part of the stable ABI but should not be used at all
    "PyUnicode_GetSize",
    # part of the stable ABI but completely removed
    "_PyState_AddModule",
))


def flno_(lineno: int) -> str:
    # Format the line so that users can C/C from the terminal
    # the line number and jump with their editor using Ctrl+G.
    return f"{lineno:>6} "


def is_c_parameter_name(name: str) -> bool:
    """Check if *name* is a valid C parameter name."""
    # C accepts the same identifiers as in Python
    return name == C_ELLIPSIS or name.isidentifier()


class Effect(Enum):
    """The reference count effect of a parameter or a return value."""

    UNKNOWN = None
    """Indicate an unparsable reference count effect.

    Such annotated entities are reported during the checking phase.
    """

    TODO = "?"
    """Indicate a reference count effect to be completed (not yet done).

    Such annotated entities are reported during the checking phase as TODOs.
    """

    UNUSED = ""
    """Indicate that the entity has no reference count."""

    DECREF = "-1"
    """Indicate that the reference count is decremented."""

    INCREF = "+1"
    """Indicate that the reference count is incremented."""

    BORROW = "0"
    """Indicate that the reference count is left unchanged.

    Parameters annotated with this effect do not steal a reference.
    """

    STEAL = "$"
    """Indicate that the reference count is left unchanged.

    Parameters annotated with this effect steal a reference.
    """

    STEAL_AND_DECREF = "$-1"
    """Indicate that the reference count is decremented.

    Parameters annotated with this effect steal a reference.
    """

    STEAL_AND_INCREF = "$+1"
    """Indicate that the reference count is incremented.

    Parameters annotated with this effect steal a reference.
    """

    NULL = "null"  # for return values only
    """Only used for a NULL return value.

    This is used by functions that always return NULL as a "PyObject *".
    """


@dataclass(slots=True, frozen=True, kw_only=True)
class LineInfo:
    func: str
    """The cleaned function name."""

    ctype: str | None
    """"The cleaned parameter or return C type, or None if missing."""

    name: str | None
    """The cleaned parameter name or None for the return parameter."""

    effect: Effect
    """The reference count effect."""

    comment: str
    """Optional comment, stripped from surrounding whitespaces."""

    # The raw_* attributes contain the strings before they are cleaned,
    # and are only used when reporting an issue during the parsing phase.
    raw_func: str
    raw_ctype: str
    raw_name: str
    raw_effect: str

    # The strip_* attributes indicate whether surrounding whitespaces were
    # stripped or not from the corresponding strings. Such action will be
    # reported during the parsing phase.
    strip_func: bool
    strip_ctype: bool
    strip_name: bool
    strip_effect: bool


@dataclass(slots=True, frozen=True)
class Return:
    """Indicate a return value."""

    ctype: str | None
    """The return C type, if any.

    A missing type is reported during the checking phase.
    """

    effect: Effect
    """The reference count effect.

    A nonsensical reference count effect is reported during the checking phase.
    """

    comment: str
    """An optional comment."""

    lineno: int = field(kw_only=True)
    """The position in the file where the parameter is documented."""


@dataclass(slots=True, frozen=True)
class Param:
    """Indicate a formal parameter."""

    name: str
    """The parameter name."""

    ctype: str | None
    """The parameter C type, if any.

    A missing type is reported during the checking phase.
    """

    effect: Effect
    """The reference count effect.

    A nonsensical reference count effect is reported during the checking phase.
    """

    comment: str
    """An optional comment."""

    lineno: int = field(kw_only=True)
    """The position in the file where the parameter is documented."""


@dataclass(slots=True, frozen=True)
class Signature:
    """An entry in refcounts.dat."""

    name: str
    """The function name."""

    rparam: Return
    """The return value specifications."""

    params: dict[str, Param] = field(default_factory=dict)
    """The (ordered) formal parameters specifications"""

    lineno: int = field(kw_only=True)
    """The position in the file where the function is documented."""


@dataclass(slots=True, frozen=True)
class FileView:
    """View of the refcounts.dat file."""

    signatures: Mapping[str, Signature]
    """A mapping from function names to the corresponding signature."""

    incomplete: frozenset[str]
    """A set of function names that are yet to be documented."""

    has_errors: bool
    """Indicate whether errors were found during the parsing."""


def parse_line(line: str) -> LineInfo | None:
    parts = line.split(":", maxsplit=4)
    if len(parts) != 5:
        return None

    raw_func, raw_ctype, raw_name, raw_effect, comment = parts

    func = raw_func.strip()
    strip_func = func != raw_func
    if not func:
        return None

    clean_ctype = raw_ctype.strip()
    ctype = clean_ctype or None
    strip_ctype = clean_ctype != raw_ctype

    clean_name = raw_name.strip()
    name = clean_name or None
    strip_name = clean_name != raw_name

    clean_effect = raw_effect.strip()
    strip_effect = clean_effect != raw_effect
    try:
        effect = Effect(clean_effect)
    except ValueError:
        effect = Effect.UNKNOWN

    comment = comment.strip()
    return LineInfo(
        func=func, ctype=ctype, name=name, effect=effect, comment=comment,
        raw_func=raw_func, raw_ctype=raw_ctype,
        raw_name=raw_name, raw_effect=raw_effect,
        strip_func=strip_func, strip_ctype=strip_ctype,
        strip_name=strip_name, strip_effect=strip_effect,
    )


@dataclass(slots=True)
class ParserReporter:
    """Utility for printing messages during the parsing phase."""

    warnings_count: int = 0
    """The number of warnings emitted so far."""
    errors_count: int = 0
    """The number of errors emitted so far."""

    @property
    def issues_count(self) -> int:
        """The number of issues encountered so far."""
        return self.warnings_count + self.errors_count

    def _print(  # type: ignore[no-untyped-def]
        self, lineno: int, message: str, *, file=None,
    ) -> None:
        """Forward to print()."""
        print(f"{flno_(lineno)} {message}", file=file)

    def info(self, lineno: int, message: str) -> None:
        """Print a simple message."""
        self._print(lineno, message)

    def warn(self, lineno: int, message: str) -> None:
        """Print a warning message."""
        self.warnings_count += 1
        self._print(lineno, message)

    def error(self, lineno: int, message: str) -> None:
        """Print an error message."""
        self.errors_count += 1
        self._print(lineno, message, file=sys.stderr)


def parse(lines: Iterable[str]) -> FileView:
    signatures: dict[str, Signature] = {}
    incomplete: set[str] = set()

    r = ParserReporter()

    for lineno, line in enumerate(map(str.strip, lines), 1):
        if not line:
            continue
        if line.startswith("#"):
            if match := MATCH_TODO(line):
                incomplete.add(match.group(1))
            continue

        e = parse_line(line)
        if e is None:
            r.error(lineno, f"cannot parse: {line!r}")
            continue

        if e.strip_func:
            r.warn(lineno, f"[func] whitespaces around {e.raw_func!r}")
        if e.strip_ctype:
            r.warn(lineno, f"[type] whitespaces around {e.raw_ctype!r}")
        if e.strip_name:
            r.warn(lineno, f"[name] whitespaces around {e.raw_name!r}")
        if e.strip_effect:
            r.warn(lineno, f"[ref] whitespaces around {e.raw_effect!r}")

        func, name = e.func, e.name
        ctype, effect = e.ctype, e.effect
        comment = e.comment

        if func not in signatures:
            # process return value
            if name is not None:
                r.warn(lineno, f"named return value in {line!r}")
            ret_param = Return(ctype, effect, comment, lineno=lineno)
            signatures[func] = Signature(func, ret_param, lineno=lineno)
        else:
            # process parameter
            if name is None:
                r.error(lineno, f"missing parameter name in {line!r}")
                continue
            sig: Signature = signatures[func]
            params = sig.params
            if name in params:
                r.error(lineno, f"duplicated parameter name in {line!r}")
                continue
            params[name] = Param(name, ctype, effect, comment, lineno=lineno)

    if has_errors := (r.issues_count > 0):
        print()
        print(f"Found {r.issues_count} issue(s)")

    return FileView(signatures, frozenset(incomplete), has_errors)


@dataclass(slots=True)
class CheckerReporter:
    """Utility for emitting messages during the checking phrase."""

    warnings_count: int = 0
    """The number of warnings emitted so far."""
    todo_count: int = 0
    """The number of TODOs emitted so far."""

    def _print_block(self, sig: Signature, message: str) -> None:
        """Print a message for the given signature."""
        print(f"{flno_(sig.lineno)} {sig.name:50} {message}")

    def _print_param(self, sig: Signature, param: Param, message: str) -> None:
        """Print a message for the given formal parameter."""
        fullname = f"{sig.name}[{param.name}]"
        print(f"{flno_(param.lineno)} {fullname:50} {message}")

    def warn_block(self, sig: Signature, message: str) -> None:
        """Print a warning message for the given signature."""
        self.warnings_count += 1
        self._print_block(sig, message)

    def todo_block(self, sig: Signature, message: str) -> None:
        """Print a TODO message for the given signature."""
        self.todo_count += 1
        self._print_block(sig, message)

    def warn_param(self, sig: Signature, param: Param, message: str) -> None:
        """Print a warning message for the given formal parameter."""
        self.warnings_count += 1
        self._print_param(sig, param, message)

    def todo_param(self, sig: Signature, param: Param, message: str) -> None:
        """Print a TODO message for the given formal parameter."""
        self.todo_count += 1
        self._print_param(sig, param, message)


def check_signature(r: CheckerReporter, sig: Signature) -> None:
    rparam = sig.rparam
    if not rparam.ctype:
        r.warn_block(sig, "missing return value type")
    match rparam.effect:
        case Effect.TODO:
            r.todo_block(sig, "incomplete reference count effect")
        case Effect.STEAL | Effect.STEAL_AND_DECREF | Effect.STEAL_AND_INCREF:
            r.warn_block(sig, "stolen reference on return value")
        case Effect.UNKNOWN:
            r.warn_block(sig, "unknown return value type")


def check_parameter(r: CheckerReporter, sig: Signature, param: Param) -> None:
    ctype, effect = param.ctype, param.effect
    if effect is Effect.TODO:
        r.todo_param(sig, param, "incomplete reference count effect")
    if ctype in OBJECT_TYPES and effect is Effect.UNUSED:
        r.warn_param(sig, param, "missing reference count effect")
    if ctype not in OBJECT_TYPES and effect is not Effect.UNUSED:
        r.warn_param(sig, param, "unused reference count effect")
    if not is_c_parameter_name(param.name):
        r.warn_param(sig, param, "invalid parameter name")


def check(view: FileView, *, strict: bool = False) -> int:
    r = CheckerReporter()

    for sig in view.signatures.values():  # type: Signature
        check_signature(r, sig)
        for param in sig.params.values():  # type: Param
            check_parameter(r, sig, param)

    exit_code = 0
    if r.warnings_count:
        print()
        print(f"Found {r.warnings_count} issue(s)")
        exit_code = 1
    if r.todo_count:
        print()
        print(f"Found {r.todo_count} todo(s)")
        exit_code = 1 if exit_code or strict else 0
    names = view.signatures.keys()
    if sorted(names) != list(names):
        print("Entries are not sorted")
        exit_code = 1 if exit_code or strict else 0
    return exit_code


def check_structure(view: FileView, stable_abi_file: str) -> int:
    print(f"Stable ABI file: {stable_abi_file}")
    print()
    stable_abi_str = Path(stable_abi_file).read_text(encoding="utf-8")
    stable_abi = tomllib.loads(stable_abi_str)
    expect = stable_abi["function"].keys()
    # check if there are missing entries (those marked as "TODO" are ignored)
    actual = STABLE_ABI_IGNORE_LIST | view.incomplete | view.signatures.keys()
    if missing := (expect - actual):
        print(f"Missing {len(missing)} stable ABI entries:")
        for name in sorted(missing):
            print(name)
        return 1
    return 0


_STABLE_ABI_PATH_SENTINEL: Final = object()


def create_parser() -> ArgumentParser:
    parser = ArgumentParser(
        prog="lint.py", formatter_class=RawDescriptionHelpFormatter,
        description=(
            "Lint the refcounts.dat file.\n\n"
            "Use --abi or --abi=FILE to check against the stable ABI."
        ),
    )
    parser.add_argument(
        "file", nargs="?", default=DEFAULT_REFCOUNT_DAT_PATH,
        help="the refcounts.dat file to check (default: %(default)s)",
    )
    parser.add_argument(
        "--abi", nargs="?", default=_STABLE_ABI_PATH_SENTINEL,
        help=(f"check against the given stable_abi.toml file "
              f"(default: {DEFAULT_STABLE_ABI_TOML_PATH})"),
    )
    parser.add_argument(
        "--strict", action="store_true",
        help="treat warnings and TODOs as errors",
    )

    return parser


def main() -> NoReturn:
    parser = create_parser()
    args = parser.parse_args()
    lines = Path(args.file).read_text(encoding="utf-8").splitlines()
    print(" PARSING ".center(80, "-"))
    view = parse(lines)
    exit_code = 1 if view.has_errors else 0
    print(" CHECKING ".center(80, "-"))
    exit_code |= check(view, strict=args.strict)
    if args.abi is not _STABLE_ABI_PATH_SENTINEL:
        abi = args.abi or DEFAULT_STABLE_ABI_TOML_PATH
        print(" CHECKING STABLE ABI ".center(80, "-"))
        check_abi_exit_code = check_structure(view, abi)
        if args.strict:
            exit_code = exit_code | check_abi_exit_code
    raise SystemExit(exit_code)


if __name__ == "__main__":
    main()
